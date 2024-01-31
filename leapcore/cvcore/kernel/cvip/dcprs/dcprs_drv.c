// SPDX-License-Identifier: GPL-2.0-only
//
// Copyright (C) 2022 Magic Leap, Inc. All rights reserved.
//

//
//
// CVIP Decompression driver.
// We expose a kernel api (see dcprs_api.h) for normal production use and an ioctl interface for standalone test apps.
//
//

// pr_fmt will insert our module name into the pr_info() macro.
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/iommu.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/mutex.h>
#include <linux/uaccess.h>
#include <linux/list.h>
#include <linux/cdev.h>
#include <linux/debugfs.h>
#include <linux/sched/task.h>
#include <linux/dma-buf.h>
#include <linux/dma-mapping.h>
#include <linux/version.h>
#include "ion/ion.h"
#include "linux/pci_cvip.h"

#include "dcprs.h"
#include "dcprs_lib.h"
#include "dcprs_api.h"
#include "dcprs_config.h"
#include "dcprs_desc.h"
#include "cast.h"

#define CREATE_TRACE_POINTS
#include "dcprs_trace.h"

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Magic Leap, Inc.");
MODULE_AUTHOR("imagyar@magicleap.com");
MODULE_DESCRIPTION("CVIP Decompression Engine");

#define irq_dcprs_axi_err_interrupt_reg_freeze      0
#define irq_dcprs_axi_err_interrupt_reg_halt        1
#define irq_dcprs_axi_err_interrupt_reg_intr        2
#define irq_dcprs_mp0_func_event_reg_freeze         3
#define irq_dcprs_mp0_func_event_reg_halt           4
#define irq_dcprs_mp0_func_event_reg_intr           5
#define irq_dcprs_mp0_err_interrupt_reg_freeze      6
#define irq_dcprs_mp0_err_interrupt_reg_halt        7
#define irq_dcprs_mp0_err_interrupt_reg_intr        8
#define irq_dcprs_mp1_func_event_reg_freeze         9
#define irq_dcprs_mp1_func_event_reg_halt           10
#define irq_dcprs_mp1_func_event_reg_intr           11
#define irq_dcprs_mp1_err_interrupt_reg_freeze      12
#define irq_dcprs_mp1_err_interrupt_reg_halt        13
#define irq_dcprs_mp1_err_interrupt_reg_intr        14


#define DPRINT  if (g_debug) pr_info

#define CLASS_NAME "dcprs"
#define CDEV_NUM 255

#define MAX_DCPRS	(2)

#define BUF_IX_G    (0)
#define BUF_IX_RB   (1)

#define IMG_SIZE_12MP_10bpp     (4096 * 3072 * 10 / 8)

extern struct dma_buf *get_dma_buf_for_shr(uint64_t shr_name);

#ifdef CONFIG_DEBUG_FS
static void dcprs_dump(void);
#endif

static int  dcprs_open(struct inode *, struct file *);
static int  dcprs_release(struct inode *, struct file *);
static long dcprs_ioctl(struct file *, unsigned int, unsigned long);

static void     *g_buf[2];
       uint32_t g_debug;
static uint32_t g_cnt[2];

// Device-global data.
static struct dcprs_dev {
    struct device       *dev;
    struct device       *sys_dev_root;
    void                *regs;
#ifdef CONFIG_DEBUG_FS
    struct dentry       *dir;
#endif
    dev_t               devnum;
    dev_t               minor;
    struct cdev         cdev;
    int                 irq_first;
    struct mutex        dev_lock;
    wait_queue_head_t   ev_waitq;
    uint32_t            opened;
    uint32_t            opened_by_pid;
    uint32_t            grayscale;
    char                opened_by_name[TASK_COMM_LEN + 1];
    int                 configured;
    uint32_t            slice_length;
    uint32_t            bpp;
    atomic_t            busy;

    struct dcprs_cxt    *hsi_cxt;

    struct {
        uint32_t   slice_length;
        uint32_t   slice_count;
        uint32_t   byte_count_g;
        uint32_t   byte_count_rb;
        int        complete;
        dma_addr_t dst_addr_io;
        dma_addr_t src_addr_io[DCPRS_MAX_SRC_BUFS];
    } pend;

    struct {
        uint64_t    count;
    } stats;

    int debug;

} g_dev[MAX_DCPRS];


static struct file_operations const g_fops_dcprs = {
    .owner          = THIS_MODULE,
    .open           = dcprs_open,
    .release        = dcprs_release,
    .unlocked_ioctl = dcprs_ioctl,
    .compat_ioctl   = dcprs_ioctl,
};

static int g_major;
static struct class *g_class;
#ifdef CONFIG_DEBUG_FS
static struct dentry *g_dir;
#endif

struct hsi_buf {
    struct {
        struct dma_buf *dma_buf;
        struct dma_buf_attachment *dma_attach;
        struct sg_table *sg_tbl;
        dma_addr_t ioaddr;
    } g, rb, dst;
} g_hsibuf;

#if 0
static int map_secure_region(struct device *dev, char *region_name)
{
    struct iommu_domain *domain;
    struct device_node *node;
    struct resource res;
    size_t mem_size;
    int err;

    domain = iommu_get_domain_for_dev(dev);
    if (IS_ERR(domain)) {
        pr_err("Failed to get domain for dcprs\n");
        return -1;
    }

    node = of_parse_phandle(dev->of_node, region_name, 0);
    if (!node) {
        pr_err("can't get base of region %s\n", region_name);
        return -1;
    }

    err = of_address_to_resource(node, 0, &res);
    if (err) {
        pr_info("can't get resource info for region %s\n", region_name);
        return -1;
    }

    mem_size = res.end - res.start + 1;

    pr_info("iommu pass region %s from 0x%llx to 0x%llx, size=0x%lx (%ld)\n", region_name, res.start, res.end, mem_size, mem_size);

    return iommu_map(domain, res.start, res.start, mem_size, IOMMU_READ | IOMMU_WRITE);
}
#endif

static int iommu_map_buf(struct device *dev, struct sg_table *sg, dma_addr_t base)
{
    pr_info("%s: skipping iommu mapping\n", __func__);

    return 0;
#if 0
    struct iommu_domain *domain_ret;
    uint32_t mapped_size = 0;
    int iommu_prot;

    domain_ret = iommu_get_domain_for_dev(dev);
    if (IS_ERR(domain_ret)) {
        pr_err("Failed to get domain for dcprs\n");
        return PTR_ERR(domain_ret);
    }

    iommu_prot = IOMMU_READ | IOMMU_WRITE;
    //mapped_size = iommu_map_sg(domain_ret, (unsigned long)page_to_phys(sg_page(sg->sgl)), sg->sgl, sg->nents, iommu_prot);
    mapped_size = iommu_map_sg(domain_ret, (unsigned long)base, sg->sgl, sg->nents, iommu_prot);
    if (mapped_size <= 0) {
        pr_err("Failed to map dcprs buf with sg\n");
        return -EFAULT;
    }

    return mapped_size;
#endif
}


static int map_out(uint64_t sh_name_g, uint64_t sh_name_rb, uint64_t sh_name_dst, struct device *dev, struct hsi_buf *hsibuf, int use_ion)
{
    struct ion_buffer *ion_buf;
    struct sg_table *sg;

    if (! use_ion) {
        pr_info("Use SHREGION buffers %llx, %llx, and %llx\n", sh_name_dst, sh_name_g, sh_name_rb);

        // Get dma_buf for dst data.
        if ((hsibuf->dst.dma_buf = get_dma_buf_for_shr(sh_name_dst)) == NULL) {
            pr_err("bad shregion name lookup on 0x%016llx\n", sh_name_dst);
            return -1;
        }

        // Get dma_buf for G data.
        if ((hsibuf->g.dma_buf = get_dma_buf_for_shr(sh_name_g)) == NULL) {
            pr_err("bad shregion name lookup on 0x%016llx\n", sh_name_g);
            return -1;
        }

        // Get dma_buf for RB data.
        if ((hsibuf->rb.dma_buf = get_dma_buf_for_shr(sh_name_rb)) == NULL) {
            pr_err("bad shregion name lookup on 0x%016llx\n", sh_name_rb);
            return -1;
        }
    } else {
        pr_info("Use ION buffers %lld, %lld, and %lld\n", sh_name_dst, sh_name_g, sh_name_rb);

        // Get dma_buf for dst data.
        if ((hsibuf->dst.dma_buf = dma_buf_get((int)sh_name_dst)) == NULL) {
            pr_err("bad lookup on 0x%016llx\n", sh_name_dst);
            return -1;
        }

        // Get dma_buf for G data.
        if ((hsibuf->g.dma_buf = dma_buf_get((int)sh_name_g)) == NULL) {
            pr_err("bad lookup on 0x%016llx\n", sh_name_g);
            return -1;
        }

        // Get dma_buf for RB data.
        if ((hsibuf->rb.dma_buf = dma_buf_get((int)sh_name_rb)) == NULL) {
            pr_err("bad lookup on 0x%016llx\n", sh_name_rb);
            return -1;
        }
    }

    // Attach DST dmabuf to iommu dev.
    hsibuf->dst.dma_attach = dma_buf_attach(hsibuf->dst.dma_buf, dev);
    if (IS_ERR(hsibuf->dst.dma_attach)) {
        pr_err("failed to attach DST buffer to DMA device\n");
        return -1;
    }

    // Attach G dmabuf to iommu dev.
    hsibuf->g.dma_attach = dma_buf_attach(hsibuf->g.dma_buf, dev);
    if (IS_ERR(hsibuf->g.dma_attach)) {
        pr_err("failed to attach G buffer to DMA device\n");
        return -1;
    }

    // Attach RB dmabuf to iommu dev.
    hsibuf->rb.dma_attach = dma_buf_attach(hsibuf->rb.dma_buf, dev);
    if (IS_ERR(hsibuf->rb.dma_attach)) {
        pr_err("failed to attach RB buffer to DMA device\n");
        return -1;
    }

    ion_buf = hsibuf->dst.dma_buf->priv;
    if (ion_buf->heap->id == ION_HEAP_CVIP_HYDRA_FMR2_ID) {
        pr_info("This is FMR2\n");

        sg = ion_sg_table(hsibuf->dst.dma_buf);
        hsibuf->dst.ioaddr = page_to_phys(sg_page(sg->sgl)) - 0;
        iommu_map_buf(dev, sg, hsibuf->dst.ioaddr);

        sg = ion_sg_table(hsibuf->g.dma_buf);
        hsibuf->g.ioaddr = page_to_phys(sg_page(sg->sgl)) - 0;
        iommu_map_buf(dev, sg, hsibuf->g.ioaddr);

        sg = ion_sg_table(hsibuf->rb.dma_buf);
        hsibuf->rb.ioaddr = page_to_phys(sg_page(sg->sgl)) - 0;
        iommu_map_buf(dev, sg, hsibuf->rb.ioaddr);

        return 0;
    } else {
        pr_info("This is NOT FMR2\n");

        // Map the DST dmabuf to obtain the sg_table.
        hsibuf->dst.sg_tbl = dma_buf_map_attachment(hsibuf->dst.dma_attach, DMA_FROM_DEVICE);
        if (IS_ERR(hsibuf->dst.sg_tbl)) {
            pr_err("error for dma DST buf sg table...\n");
            return -1;
        }
        hsibuf->dst.ioaddr = hsibuf->dst.sg_tbl->sgl->dma_address;

        // Map the G dmabuf to obtain the sg_table.
        hsibuf->g.sg_tbl = dma_buf_map_attachment(hsibuf->g.dma_attach, DMA_TO_DEVICE);
        if (IS_ERR(hsibuf->g.sg_tbl)) {
            pr_err("error for dma G buf sg table...\n");
            return -1;
        }
        hsibuf->g.ioaddr = hsibuf->g.sg_tbl->sgl->dma_address;

        // Map the RB dmabuf to obtain the sg_table.
        hsibuf->rb.sg_tbl = dma_buf_map_attachment(hsibuf->rb.dma_attach, DMA_TO_DEVICE);
        if (IS_ERR(hsibuf->rb.sg_tbl)) {
            pr_err("error for dma RB buf sg table...\n");
            return -1;
        }
        hsibuf->rb.ioaddr = hsibuf->rb.sg_tbl->sgl->dma_address;

        return 0;
    }
}


//--------------------------------------------------------------------------
// dcprs_open()
//--------------------------------------------------------------------------
static int dcprs_open(struct inode *pInode, struct file *pFile)
{
    struct dcprs_dev *ddev;
    unsigned int mn;

    DPRINT("open device");

    mn = iminor(pInode);
    ddev = &g_dev[mn];
    pFile->private_data = ddev;

    mutex_lock(&ddev->dev_lock);

    // Limit access to this device to one process at a time.
    if (ddev->opened) {
        pr_err("DCPRS dev already opened by another process\n");
        mutex_unlock(&ddev->dev_lock);
        return -EBUSY;
    }
    ddev->configured = 0;
    ddev->opened = 1;
    ddev->opened_by_pid = current->pid;
    memcpy(ddev->opened_by_name, current->comm, TASK_COMM_LEN);

    mutex_unlock(&ddev->dev_lock);

    return 0;
}


//--------------------------------------------------------------------------
// dcprs_release()
//--------------------------------------------------------------------------
static int dcprs_release(struct inode *pInode, struct file *pFile)
{
    struct dcprs_dev *ddev = (struct dcprs_dev *)pFile->private_data;

    ddev->configured = 0;
    ddev->opened = 0;

    return 0;
}


static int32_t dcprs_wait_for_finish2(int chained_engines)
{
    int count = 0;
    int32_t rval = SUCCESS;
    //uint32_t to = jiffies + (HZ * 2); // time in 2 secs
    volatile dcprs_mp_rgf_dcprs_func_event_reg_t event_reg;

    //pr_info("Wait2 loop\n");

    if (1) {
        event_reg.register_value = readl(&default_mp->dcprs_mp.dcprs_func_event_reg);
        if (event_reg.bitfield.e1_exec_end == 1) {
            pr_info("Engine #1 (RB) is done!, count=%d", count);
            //break;
        }
        count++;

        if (event_reg.bitfield.e0_exec_end == 1) {
            pr_info("Engine #0 (G) is done!");
            if (! chained_engines) {
                pr_info("Poll Done");
                //break;
            }
        }
    }

    // Clear decompression complete event from both engines
    event_reg.bitfield.e0_exec_end = 1;
    event_reg.bitfield.e1_exec_end = 1;
    writel(event_reg.register_value, &default_mp->dcprs_mp.dcprs_func_event_reg);

    return rval;
}


//--------------------------------------------------------------------------
// dcprs_ioctl()
//--------------------------------------------------------------------------
static long dcprs_ioctl(struct file *pFile, unsigned int cmd, unsigned long arg)
{
    int use_ion, to, retval = 0;
    struct dcprs_ioctl io_arg, *parg = &io_arg;
    struct dcprs_dev *ddev = (struct dcprs_dev *)pFile->private_data;

    // Get a valid ioctl control struct.
    if (!access_ok((void *)arg, sizeof(io_arg))) {
        pr_err("ioctl: bad io_arg address\n");
        return -EPERM;
    }
    if (copy_from_user(&io_arg, (void __user *)arg, sizeof(io_arg))) {
        pr_err("efault");
        return -EFAULT;
    }

    if (cmd != DCPRS_WAIT) {
        mutex_lock(&ddev->dev_lock);
    }

    switch (cmd) {

        // Configure the device.
        case DCPRS_CONFIG:
            DPRINT("Got a CONFIG request");

            // A slice must be divisible by 16 bytes.
            if (parg->slice_length & (16 - 1)) {
                pr_err("illegal slice length: 0x%x", parg->slice_length);
                retval = -EINVAL;
                break;
            }

            // Initialize device.
            dcprs_lib_init(ddev->regs, 10);

            ddev->pend.slice_length = parg->slice_length;
            DPRINT("slice_length=%d, watermark=%d\n", ddev->pend.slice_length, ddev->pend.slice_length >> 4);

            // Reset slice and section counters.
            ddev->pend.slice_count   = 0;
            ddev->pend.byte_count_g  = 0;
            ddev->pend.byte_count_rb = 0;

            ddev->configured = 1;

            if (ddev->pend.slice_length) {
                dcprs_rgf_dcprs_axi_err_interrupt_reg__enable_t int_enable_reg;
                dcprs_rgf_watermark_threshold_reg_t wmrk_th_reg;

                DPRINT("Configure for slicing");

                // Configure watermark threshold
                wmrk_th_reg.register_value = 0;
                wmrk_th_reg.bitfield.wmrk_th = ddev->pend.slice_length >> 4; // threshold counts in 16-byte units
                write_reg(dcprs->dcprs_mptop.rgf.m0e0_watermark_threshold_reg, wmrk_th_reg.register_value);

                // Enable watermark interrupt
                int_enable_reg.register_value = 0;
                int_enable_reg.bitfield.m0e0_wmrk = 1;
                write_reg(dcprs->dcprs_mptop.rgf.dcprs_axi_err_interrupt_reg_int_en, int_enable_reg.register_value);
            }
            break;

        // Request to decompress the specified buffer(s).
        case DCPRS_DECOMPRESS:

            //pr_err("Got a DCPRS_DECOMPRESS request");
            //pr_info("G sect length = %d, RB sect length = %d", parg->src[BUF_IX_G].sect_length, parg->src[BUF_IX_RB].sect_length);

            use_ion = parg->flags & DCPRS_F_ION;

            // Make sure that we had an ioctl(DCPRS_CONFIG) already.
            if (! ddev->configured) {
                pr_err("device hasn't been configured yet");
                retval = -EPERM;
                break;
            }

            ddev->grayscale = parg->flags & DCPRS_F_GRAYSCALE;

            // If "sectioning" is enabled, then we're feeding frame data to the engine a section at a time,
            // instead of handing it complete but compressed frames.
            //
            // If "slicing" is enabled, then the engine is configured to output data a slice at a time,
            // instead of outputting a fully decompressed frame.
            //
            // The key thing is that "sectioning" and "slicing" are mutually independent.
            //

            // If first section or we're in full-frame mode....
            if ((ddev->pend.byte_count_g + ddev->pend.byte_count_rb) == 0) {

                if (!(parg->flags & DCPRS_F_NOMAP)) {
                    map_out(parg->src[DCPRS_ENGINE_G].shname, parg->src[DCPRS_ENGINE_RB].shname, parg->dst_shname, ddev->dev, &g_hsibuf, use_ion);
                } else {
                     g_hsibuf.dst.ioaddr = parg->dst_addr;
                     g_hsibuf.g.ioaddr   = parg->src[DCPRS_ENGINE_G].addr;
                     g_hsibuf.rb.ioaddr  = parg->src[DCPRS_ENGINE_RB].addr;
                }

                // Configure descriptors
                //DPRINT("Configuring descriptors: g=0x%llx, rb=0x%llx, dst=0x%llx\n", g_hsibuf.g.ioaddr, g_hsibuf.rb.ioaddr, g_hsibuf.dst.ioaddr);
                dcprs_config_descriptor(g_hsibuf.g.ioaddr,  g_hsibuf.dst.ioaddr, DCPRS_ENGINE_G);
                if (! ddev->grayscale) {
                    dcprs_config_descriptor(g_hsibuf.rb.ioaddr, 0, DCPRS_ENGINE_RB);
                }

                // Are we in full-frame (non-sectioning) mode?
                if ((parg->flags & DCPRS_F_SECT_MODE) == 0) {
                    uint32_t plen_g, plen_rb;
                    //DPRINT("Configuring full-frame (no sections) mode\n");
                    writel(DCPRS_FRAME_SEC_MODE_DISABLED, &default_mp->dcprs_mp.engine0_frm_sec_enb_reg);
                    writel(DCPRS_FRAME_SEC_MODE_DISABLED, &default_mp->dcprs_mp.engine1_frm_sec_enb_reg);

                    // Set compressed data size.
                    plen_g  = ALIGN(parg->src[BUF_IX_G].length, 32);
                    plen_rb = ALIGN(parg->src[BUF_IX_RB].length, 32);
                    plen_g  = parg->src[BUF_IX_G].length + 16;
                    plen_rb = parg->src[BUF_IX_RB].length + 16;
                    DPRINT("rlen_g=%d, plen_g=%d\n", parg->src[BUF_IX_G].length, plen_g);
                    DPRINT("rlen_rb=%d, plen_rb=%d\n", parg->src[BUF_IX_RB].length, plen_rb);
                    dcprs_default_mp0_eng0_desc.word4.bitfield.axirdch0rs = plen_g;
                    if (! ddev->grayscale) {
                        dcprs_default_mp0_eng1_desc.word4.bitfield.axirdch0rs = plen_rb;
                    }
                } else {
                    // Sectioning mode.
                    writel(DCPRS_FRAME_SEC_MODE_ENABLED, &default_mp->dcprs_mp.engine0_frm_sec_enb_reg);
                    writel(DCPRS_FRAME_SEC_MODE_ENABLED, &default_mp->dcprs_mp.engine1_frm_sec_enb_reg);

                    DPRINT("Configured sectioning mode\n");
                }

                ddev->pend.complete = 0;

                // Update both descriptors.
                dcprs_write_descriptor(&dcprs_default_mp0_eng0_desc, DCPRS_DEFAULT_MP_INDEX, DCPRS_ENGINE_G,  DCPRS_DEFAULT_DESCRIPTOR_INDEX);
                if (! ddev->grayscale) {
                    dcprs_write_descriptor(&dcprs_default_mp0_eng1_desc, DCPRS_DEFAULT_MP_INDEX, DCPRS_ENGINE_RB, DCPRS_DEFAULT_DESCRIPTOR_INDEX);

                    // Execute descriptor
                    trace_ml_dcprs_start(g_hsibuf.dst.ioaddr, g_hsibuf.g.ioaddr, g_hsibuf.rb.ioaddr, 0, 0, 0);
                    DPRINT("Executing chained descriptor\n");

                    dcprs_exec_descriptor(1, 1);
                } else {
                    // Change mode to grayscale, and set image size (no longer using sec mode size)
                    dcprs_default_mp0_eng0_desc.word0.bitfield.mergemod = DCPRS_MERGEMOD_GRAYSCALE_NO_SPLIT;
                    dcprs_default_mp0_eng0_desc.word4.bitfield.axirdch0rs = parg->src[DCPRS_ENGINE_G].length;
                    dcprs_write_descriptor(&dcprs_default_mp0_eng0_desc, DCPRS_DEFAULT_MP_INDEX, DCPRS_ENGINE_G, DCPRS_DEFAULT_DESCRIPTOR_INDEX);

                    // Execute NON-CHAINED descriptor
                    DPRINT("Executing NON-chained descriptor\n");
                    trace_ml_dcprs_start(g_hsibuf.dst.ioaddr, g_hsibuf.g.ioaddr, g_hsibuf.rb.ioaddr, 0, 0, 0);
                    dcprs_exec_descriptor(0, 1);
                }

                // Write the first section.
                if (parg->flags & DCPRS_F_SECT_MODE) {
                    dcprs_section_received(DCPRS_ENGINE_G,  parg->src[BUF_IX_G].sect_length);
                    dcprs_section_received(DCPRS_ENGINE_RB, parg->src[BUF_IX_RB].sect_length);

                    ddev->pend.byte_count_g  += parg->src[BUF_IX_G].sect_length;
                    ddev->pend.byte_count_rb += parg->src[BUF_IX_RB].sect_length;

                    DPRINT("Wrote first section, g_len=%d, rb_len=%d\n", parg->src[BUF_IX_G].sect_length, parg->src[BUF_IX_RB].sect_length);
                }
                break;
            }

            //
            // If we got here, then we're in sectioning mode and this is NOT the first section of either buffer.
            //

            // Are we adding a section to the G data?
            if (parg->src[BUF_IX_G].sect_length) {
                DPRINT("adding %d G bytes, total=%d", parg->src[BUF_IX_G].sect_length, ddev->pend.byte_count_g);

                ddev->pend.byte_count_g += parg->src[BUF_IX_G].sect_length;

                // Force EOF on last section.
                if (parg->flags & DCPRS_F_SECT_LAST_G) {
                    uint32_t sz_align = ALIGN(parg->src[BUF_IX_G].sect_length, 16);
                    //sz_align = (parg->src[BUF_IX_G].sect_length / 16) * 16 + 16;

                    pr_info("last G section, size=%d, aligned_size=%d!, total=%d\n", parg->src[BUF_IX_G].sect_length, sz_align, ddev->pend.byte_count_g);
                    ddev->pend.byte_count_g = 0;
                    dcprs_section_received(DCPRS_ENGINE_G, parg->src[BUF_IX_G].sect_length);
                    dcprs_eof_received(DCPRS_ENGINE_G);

                    dcprs_wait_for_finish2(1);

                } else {
                    pr_info("pass in %d NON-last bytes, total=%d\n", parg->src[BUF_IX_G].sect_length, ddev->pend.byte_count_g);
                    dcprs_section_received(DCPRS_ENGINE_G, parg->src[BUF_IX_G].sect_length);
                }
            }

            // Are we adding a section to the RB data?
            if (parg->src[BUF_IX_RB].sect_length) {
                DPRINT("adding %d RB bytes, total=%d", parg->src[BUF_IX_RB].sect_length, ddev->pend.byte_count_rb);

                ddev->pend.byte_count_rb += parg->src[BUF_IX_RB].sect_length;

                // Force EOF on last section.
                if (parg->flags & DCPRS_F_SECT_LAST_RB) {
                    uint32_t sz_align = ALIGN(parg->src[BUF_IX_RB].sect_length, 16);

                    pr_info("last RB section, size=%d, aligned_size=%d, total=%d!\n", parg->src[BUF_IX_RB].sect_length, sz_align, ddev->pend.byte_count_rb);

                    ddev->pend.byte_count_rb = 0;

                    dcprs_section_received(DCPRS_ENGINE_RB, parg->src[BUF_IX_RB].sect_length);
                    dcprs_eof_received(DCPRS_ENGINE_RB);
                } else {
                    pr_info("pass in %d NON-last bytes, total=%d\n", parg->src[BUF_IX_RB].sect_length, ddev->pend.byte_count_rb);
                    dcprs_section_received(DCPRS_ENGINE_RB, parg->src[BUF_IX_RB].sect_length);
                }
            }

            break;

        case DCPRS_WAIT:
            // Wait up to 1 sec for completion.
            to = wait_event_interruptible_timeout(ddev->ev_waitq, (ddev->pend.complete), HZ * 2);
            ddev->pend.complete = 0;
            if (to == 0) {
                uint32_t axi_err, cprs_err0, cprs_err1;
                uint32_t axi_err_i, cprs_err0_i, cprs_err1_i;

                axi_err     = readl(&dcprs->dcprs_mptop.rgf.dcprs_axi_err_status_reg);
                cprs_err0   = readl(&dcprs->dcprs_mptop.dcprs_mp0.dcprs_mp.dcprs_err_status_reg);
                cprs_err1   = readl(&dcprs->dcprs_mptop.dcprs_mp1.dcprs_mp.dcprs_err_status_reg);
                axi_err_i   = readl(&dcprs->dcprs_mptop.rgf.dcprs_axi_err_interrupt_reg);
                cprs_err0_i = readl(&dcprs->dcprs_mptop.dcprs_mp0.dcprs_mp.dcprs_err_interrupt_reg);
                cprs_err1_i = readl(&dcprs->dcprs_mptop.dcprs_mp1.dcprs_mp.dcprs_err_interrupt_reg);
                pr_err("Timeout!: axi_err=0x%08X, cprs_err0=0x%08X, cprs_err1=0x%08X\n", axi_err, cprs_err0, cprs_err1);
                pr_err("Timeout!: axi_err_i=0x%08X, cprs_err0_i=0x%08X, cprs_err1_i=0x%08X\n", axi_err_i, cprs_err0_i, cprs_err1_i);

                dump_jpegls();
#ifdef CONFIG_DEBUG_FS
                dcprs_dump();
#endif
                pr_info("Dump Desc #0:\n");
                dcprs_dump_descriptor(&dcprs_default_mp0_eng0_desc);
                pr_info("Dump Desc #1:\n");
                dcprs_dump_descriptor(&dcprs_default_mp0_eng1_desc);

                pr_info("Flushing\n");
                dcprs_engine_flush();
                return -ETIME;
            }
            if (to < 0) {
                if (to != -ERESTARTSYS) {
                    pr_err("wait error: %d\n", to);
                }
                return to;
            }

            io_arg.uncomp_length = (dcprs->dcprs_mptop.rgf.m0e0_watermark_last_cntr_reg.register_value - 1) * 16;
            DPRINT("[W] %d\n", io_arg.uncomp_length);
            break;

        default:
            pr_err("illegal ioctl request");
            retval = -EINVAL;
            break;
    }

    // On a successful operation, update control struct.
    if (copy_to_user((void __user *)arg, &io_arg, sizeof(io_arg))) {
        retval = -EFAULT;
    }

    if (cmd != DCPRS_WAIT) {
        mutex_unlock(&ddev->dev_lock);
    }

    return retval;
}


static int dcprs_mkdev(struct dcprs_dev *ddev)
{
    int ret;
    struct cdev *cdev;
    static dev_t index = 0;
    struct device *device;

    cdev = &ddev->cdev;
    ddev->minor = index++;
    cdev_init(cdev, &g_fops_dcprs);
    cdev->owner = THIS_MODULE;
    ret = cdev_add(cdev, MKDEV(g_major, ddev->minor), 1);
    if (ret) {
        pr_err("failed to add cdev.\n");
        return ret;
    }

    device = device_create(g_class, ddev->dev,
                           MKDEV(g_major, ddev->minor),
                           NULL, "dcprs");
    if (IS_ERR(device)) {
        cdev_del(cdev);
        pr_err("failed to create the device.\n");
        return PTR_ERR(device);
    }

    ddev->devnum = MKDEV(g_major, ddev->minor);

    mutex_init(&ddev->dev_lock);

    return 0;
}


static void dcprs_rmdev(struct dcprs_dev *ddev)
{
    device_destroy(g_class, ddev->devnum);
    cdev_del(&ddev->cdev);
}


static irqreturn_t dcprs_intr(int irq, void *dev)
{
    struct dcprs_dev *ddev = dev;
    uint32_t irq_ix, err_dcp;

    irq_ix = irq - ddev->irq_first;

    trace_ml_dcprs_intr(irq, irq_ix, (dcprs->dcprs_mptop.rgf.m0e0_watermark_last_cntr_reg.register_value - 1) * 16);

    if (irq_ix == irq_dcprs_mp0_err_interrupt_reg_freeze) {
        pr_info("    MP0_ERR_INTR\n");
    }

    if (irq_ix == irq_dcprs_axi_err_interrupt_reg_intr) {
        dcprs_rgf_dcprs_axi_err_interrupt_reg_t irq_stat;

        irq_stat.register_value = readl(&dcprs->dcprs_mptop.rgf.dcprs_axi_err_interrupt_reg.register_value);
        if (irq_stat.bitfield.m0e0_wmrk) {
            //pr_err("        GOT WATERMARK!\n");
        } else {
            pr_err("got axi err: 0x%08x\n", irq_stat.register_value);
        }
        writel(irq_stat.register_value, &dcprs->dcprs_mptop.rgf.dcprs_axi_err_interrupt_reg);
    }

    if (irq_ix == irq_dcprs_mp0_func_event_reg_intr) {
        volatile dcprs_mp_rgf_dcprs_func_event_reg_t event_reg;

        event_reg.register_value = readl(&default_mp->dcprs_mp.dcprs_func_event_reg);
        writel(event_reg.register_value, &default_mp->dcprs_mp.dcprs_func_event_reg);

        g_dev[0].stats.count++;

        ddev->pend.complete = 1;
        wake_up_interruptible(&ddev->ev_waitq);
    }

    // Error handling.
    err_dcp = readl(&default_mp->dcprs_mp.dcprs_err_interrupt_reg);

    // Clear error interrupts.
    if (err_dcp) {
        pr_err("err=0x%08X\n", err_dcp);
        writel(err_dcp, &default_mp->dcprs_mp.dcprs_err_interrupt_reg);
    }

    return IRQ_HANDLED;
}


#ifdef CONFIG_DEBUG_FS

static int dcprs_log_show(struct seq_file *f, void *offset)
{
    struct dcprs_dev *ddev = f->private;
    volatile dcprs_t *dd = (volatile dcprs_t *)ddev->regs;

    seq_printf(f, "Device Top                              : %px\n",    dd);
    seq_printf(f, "dcprs_mp0.dcprs_mp.bayer_green_first_reg: 0x%08x\n", dd->dcprs_mptop.dcprs_mp0.dcprs_mp.bayer_green_first_reg.register_value);
    seq_printf(f, "dcprs_mp.dcprs_func_event_reg           : 0x%08x\n", dd->dcprs_mptop.dcprs_mp0.dcprs_mp.dcprs_func_event_reg.register_value);
    seq_printf(f, "    in0_ispb_sof         : %d\n", dd->dcprs_mptop.dcprs_mp0.dcprs_mp.dcprs_func_event_reg.bitfield.in0_ispb_sof);
    seq_printf(f, "    in0_ispb_eof         : %d\n", dd->dcprs_mptop.dcprs_mp0.dcprs_mp.dcprs_func_event_reg.bitfield.in0_ispb_eof);
    seq_printf(f, "    in1_ispb_sof         : %d\n", dd->dcprs_mptop.dcprs_mp0.dcprs_mp.dcprs_func_event_reg.bitfield.in1_ispb_sof);
    seq_printf(f, "    in1_ispb_eof         : %d\n", dd->dcprs_mptop.dcprs_mp0.dcprs_mp.dcprs_func_event_reg.bitfield.in1_ispb_eof);
    seq_printf(f, "    out0_ispb_sof        : %d\n", dd->dcprs_mptop.dcprs_mp0.dcprs_mp.dcprs_func_event_reg.bitfield.out0_ispb_sof);
    seq_printf(f, "    out0_ispb_eof        : %d\n", dd->dcprs_mptop.dcprs_mp0.dcprs_mp.dcprs_func_event_reg.bitfield.out0_ispb_eof);
    seq_printf(f, "    out1_ispb_sof        : %d\n", dd->dcprs_mptop.dcprs_mp0.dcprs_mp.dcprs_func_event_reg.bitfield.out1_ispb_sof);
    seq_printf(f, "    out1_ispb_eof        : %d\n", dd->dcprs_mptop.dcprs_mp0.dcprs_mp.dcprs_func_event_reg.bitfield.out1_ispb_eof);
    seq_printf(f, "    e0_new_desc          : %d\n", dd->dcprs_mptop.dcprs_mp0.dcprs_mp.dcprs_func_event_reg.bitfield.e0_new_desc);
    seq_printf(f, "    e0_exec_end          : %d\n", dd->dcprs_mptop.dcprs_mp0.dcprs_mp.dcprs_func_event_reg.bitfield.e0_exec_end);
    seq_printf(f, "    e1_new_desc          : %d\n", dd->dcprs_mptop.dcprs_mp0.dcprs_mp.dcprs_func_event_reg.bitfield.e1_new_desc);
    seq_printf(f, "    e1_exec_end          : %d\n", dd->dcprs_mptop.dcprs_mp0.dcprs_mp.dcprs_func_event_reg.bitfield.e1_exec_end);
    seq_printf(f, "    e0_sequence_end      : %d\n", dd->dcprs_mptop.dcprs_mp0.dcprs_mp.dcprs_func_event_reg.bitfield.e0_sequence_end);
    seq_printf(f, "    e1_sequence_end      : %d\n", dd->dcprs_mptop.dcprs_mp0.dcprs_mp.dcprs_func_event_reg.bitfield.e1_sequence_end);
    seq_printf(f, "    frame_end_both       : %d\n", dd->dcprs_mptop.dcprs_mp0.dcprs_mp.dcprs_func_event_reg.bitfield.frame_end_both);
    seq_printf(f, "    e0_gracefull_stop    : %d\n", dd->dcprs_mptop.dcprs_mp0.dcprs_mp.dcprs_func_event_reg.bitfield.e0_gracefull_stop);
    seq_printf(f, "    e1_gracefull_stop    : %d\n", dd->dcprs_mptop.dcprs_mp0.dcprs_mp.dcprs_func_event_reg.bitfield.e1_gracefull_stop);
    seq_printf(f, "    e0_wmrk              : %d\n", dd->dcprs_mptop.dcprs_mp0.dcprs_mp.dcprs_func_event_reg.bitfield.e0_wmrk);
    seq_printf(f, "    e1_wmrk              : %d\n", dd->dcprs_mptop.dcprs_mp0.dcprs_mp.dcprs_func_event_reg.bitfield.e1_wmrk);

    return 0;
}


static void dcprs_dump(void)
{
    struct dcprs_dev *ddev = &g_dev[0];
    volatile dcprs_t *dd = (volatile dcprs_t *)ddev->regs;

    pr_info("Device Top                              : %px\n",    dd);
    pr_info("dcprs_mp0.dcprs_mp.bayer_green_first_reg: 0x%08x\n", dd->dcprs_mptop.dcprs_mp0.dcprs_mp.bayer_green_first_reg.register_value);
    pr_info("dcprs_mp.dcprs_func_event_reg           : 0x%08x\n", dd->dcprs_mptop.dcprs_mp0.dcprs_mp.dcprs_func_event_reg.register_value);
    pr_info("    in0_ispb_sof         : %d\n", dd->dcprs_mptop.dcprs_mp0.dcprs_mp.dcprs_func_event_reg.bitfield.in0_ispb_sof);
    pr_info("    in0_ispb_eof         : %d\n", dd->dcprs_mptop.dcprs_mp0.dcprs_mp.dcprs_func_event_reg.bitfield.in0_ispb_eof);
    pr_info("    in1_ispb_sof         : %d\n", dd->dcprs_mptop.dcprs_mp0.dcprs_mp.dcprs_func_event_reg.bitfield.in1_ispb_sof);
    pr_info("    in1_ispb_eof         : %d\n", dd->dcprs_mptop.dcprs_mp0.dcprs_mp.dcprs_func_event_reg.bitfield.in1_ispb_eof);
    pr_info("    out0_ispb_sof        : %d\n", dd->dcprs_mptop.dcprs_mp0.dcprs_mp.dcprs_func_event_reg.bitfield.out0_ispb_sof);
    pr_info("    out0_ispb_eof        : %d\n", dd->dcprs_mptop.dcprs_mp0.dcprs_mp.dcprs_func_event_reg.bitfield.out0_ispb_eof);
    pr_info("    out1_ispb_sof        : %d\n", dd->dcprs_mptop.dcprs_mp0.dcprs_mp.dcprs_func_event_reg.bitfield.out1_ispb_sof);
    pr_info("    out1_ispb_eof        : %d\n", dd->dcprs_mptop.dcprs_mp0.dcprs_mp.dcprs_func_event_reg.bitfield.out1_ispb_eof);
    pr_info("    e0_new_desc          : %d\n", dd->dcprs_mptop.dcprs_mp0.dcprs_mp.dcprs_func_event_reg.bitfield.e0_new_desc);
    pr_info("    e0_exec_end          : %d\n", dd->dcprs_mptop.dcprs_mp0.dcprs_mp.dcprs_func_event_reg.bitfield.e0_exec_end);
    pr_info("    e1_new_desc          : %d\n", dd->dcprs_mptop.dcprs_mp0.dcprs_mp.dcprs_func_event_reg.bitfield.e1_new_desc);
    pr_info("    e1_exec_end          : %d\n", dd->dcprs_mptop.dcprs_mp0.dcprs_mp.dcprs_func_event_reg.bitfield.e1_exec_end);
    pr_info("    e0_sequence_end      : %d\n", dd->dcprs_mptop.dcprs_mp0.dcprs_mp.dcprs_func_event_reg.bitfield.e0_sequence_end);
    pr_info("    e1_sequence_end      : %d\n", dd->dcprs_mptop.dcprs_mp0.dcprs_mp.dcprs_func_event_reg.bitfield.e1_sequence_end);
    pr_info("    frame_end_both       : %d\n", dd->dcprs_mptop.dcprs_mp0.dcprs_mp.dcprs_func_event_reg.bitfield.frame_end_both);
    pr_info("    e0_gracefull_stop    : %d\n", dd->dcprs_mptop.dcprs_mp0.dcprs_mp.dcprs_func_event_reg.bitfield.e0_gracefull_stop);
    pr_info("    e1_gracefull_stop    : %d\n", dd->dcprs_mptop.dcprs_mp0.dcprs_mp.dcprs_func_event_reg.bitfield.e1_gracefull_stop);
    pr_info("    e0_wmrk              : %d\n", dd->dcprs_mptop.dcprs_mp0.dcprs_mp.dcprs_func_event_reg.bitfield.e0_wmrk);
    pr_info("    e1_wmrk              : %d\n", dd->dcprs_mptop.dcprs_mp0.dcprs_mp.dcprs_func_event_reg.bitfield.e1_wmrk);
}


static int _dcprs_desc_show(struct seq_file *f, void *offset, uint32_t eng_num)
{
    struct dcprs_dev *ddev = f->private;
    volatile dcprs_t *dd = (volatile dcprs_t *)ddev->regs;
    dcprs_desc_t *desc;


    if (eng_num == 0) {
        desc = (dcprs_desc_t *)&dd->dcprs_mptop.dcprs_mp0.dcprs_mp.dcprs_eng0_desc_mem;
    } else {
        desc = (dcprs_desc_t *)&dd->dcprs_mptop.dcprs_mp0.dcprs_mp.dcprs_eng1_desc_mem;
    }
    seq_printf(f, "Descriptor Word #00 (0x%08X)\n", desc->word0.register_value);
    seq_printf(f, "    frps    : %d\n",       desc->word0.bitfield.frps);
    seq_printf(f, "    lcpo    : %d\n",       desc->word0.bitfield.lcpo);
    seq_printf(f, "    fcpo    : %d\n",       desc->word0.bitfield.fcpo);
    seq_printf(f, "    lrpo    : %d\n",       desc->word0.bitfield.lrpo);
    seq_printf(f, "    frpo    : %d\n",       desc->word0.bitfield.frpo);
    seq_printf(f, "    cdpe    : %d\n",       desc->word0.bitfield.cdpe);
    seq_printf(f, "    padonly : %d\n",       desc->word0.bitfield.padonly);
    seq_printf(f, "    mergemod: %d\n",       desc->word0.bitfield.mergemod);
    seq_printf(f, "    ftype   : %d\n",       desc->word0.bitfield.ftype);
    seq_printf(f, "    ed      : %d\n",       desc->word0.bitfield.ed);

    seq_printf(f, "Descriptor Word #01 (0x%08X)\n", desc->word1.register_value);
    seq_printf(f, "    lcps    : %d\n",       desc->word1.bitfield.lcps);
    seq_printf(f, "    fcps    : %d\n",       desc->word1.bitfield.fcps);
    seq_printf(f, "    lrps    : %d\n",       desc->word1.bitfield.lrps);

    seq_printf(f, "Descriptor Word #02 (0x%08X)\n", desc->word2.register_value);
    seq_printf(f, "    nrow    : %d\n",       desc->word2.bitfield.nrow);
    seq_printf(f, "    ncol    : %d\n",       desc->word2.bitfield.ncol);


    seq_printf(f, "Descriptor Word #03 (0x%08X)\n", desc->word3.register_value);
    seq_printf(f, "    axiwrch0aie     : %d\n",       desc->word3.bitfield.axiwrch0aie);
    seq_printf(f, "    axiwrch0vflip   : %d\n",       desc->word3.bitfield.axiwrch0vflip);
    seq_printf(f, "    axiwrch0dpck    : %d\n",       desc->word3.bitfield.axiwrch0dpck);
    seq_printf(f, "    axiwrch0fifoen  : %d\n",       desc->word3.bitfield.axiwrch0fifoen);
    seq_printf(f, "    axiwrch0mod     : %d\n",       desc->word3.bitfield.axiwrch0mod);
    seq_printf(f, "    axirdch0obpp    : %d\n",       desc->word3.bitfield.axirdch0obpp);
    seq_printf(f, "    axirdch0ibpp    : %d\n",       desc->word3.bitfield.axirdch0ibpp);
    seq_printf(f, "    axirdch0aie     : %d\n",       desc->word3.bitfield.axirdch0aie);
    seq_printf(f, "    axirdch0bppcsfl : %d\n",       desc->word3.bitfield.axirdch0bppcsfl);
    seq_printf(f, "    axirdch0bppcsfr : %d\n",       desc->word3.bitfield.axirdch0bppcsfr);
    seq_printf(f, "    axirdch0dpck    : %d\n",       desc->word3.bitfield.axirdch0dpck);
    seq_printf(f, "    axirdch0fifoen  : %d\n",       desc->word3.bitfield.axirdch0fifoen);
    seq_printf(f, "    axirdch0mod     : %d\n",       desc->word3.bitfield.axirdch0mod);
    seq_printf(f, "    cdpdescptr      : %d\n",       desc->word3.bitfield.cdpdescptr);

    seq_printf(f, "Descriptor Word #04 (0x%08X)\n", desc->word4.register_value);
    seq_printf(f, "    axirdch0rs      : %d\n",       desc->word4.bitfield.axirdch0rs);

    seq_printf(f, "Descriptor Word #05 (0x%08X)\n", desc->word5.register_value);
    seq_printf(f, "    axirdch0ais     : %d\n",       desc->word5.bitfield.axirdch0ais);
    seq_printf(f, "    axirdch0rgap    : %d\n",      desc->word5.bitfield.axirdch0rgap);

    seq_printf(f, "Descriptor Word #06 (0x%08X)\n", desc->word6.register_value);
    seq_printf(f, "    axirdch0ba_l    : 0x%08x\n",       desc->word6.bitfield.axirdch0ba_l);

    seq_printf(f, "Descriptor Word #07 (0x%08X)\n", desc->word7.register_value);
    seq_printf(f, "    axirdch0ba_h    : 0x%08x\n",       desc->word7.bitfield.axirdch0ba_h);

    seq_printf(f, "Descriptor Word #08 (0x%08X)\n", desc->word8.register_value);
    seq_printf(f, "    axiwrch0ais     : %d\n",       desc->word8.bitfield.axiwrch0ais);

    seq_printf(f, "Descriptor Word #09 (0x%08X)\n", desc->word9.register_value);
    seq_printf(f, "    axiwrch0ba_l    : 0x%08x\n",       desc->word9.bitfield.axiwrch0ba_l);

    seq_printf(f, "Descriptor Word #10 (0x%08X)\n", desc->word10.register_value);
    seq_printf(f, "    axiwrch0ba_h    : 0x%08x\n",       desc->word10.bitfield.axiwrch0ba_h);

    return 0;
}

static int dcprs_desc0_show(struct seq_file *f, void *offset)
{
    return _dcprs_desc_show(f, offset, 0);
}


static int dcprs_desc1_show(struct seq_file *f, void *offset)
{
    return _dcprs_desc_show(f, offset, 1);
}


static int dcprs_desc0_open(struct inode *inode, struct file *file)
{
    return single_open(file, dcprs_desc0_show, inode->i_private);
}

static const struct file_operations dcprs_desc0_fops = {
    .owner		= THIS_MODULE,
    .open		= dcprs_desc0_open,
    .read		= seq_read,
    .llseek		= seq_lseek,
    .release	= single_release,
};

static int dcprs_desc1_open(struct inode *inode, struct file *file)
{
    return single_open(file, dcprs_desc1_show, inode->i_private);
}

static const struct file_operations dcprs_desc1_fops = {
    .owner		= THIS_MODULE,
    .open		= dcprs_desc1_open,
    .read		= seq_read,
    .llseek		= seq_lseek,
    .release	= single_release,
};

//======================================================================//


static int dcprs_log_open(struct inode *inode, struct file *file)
{
    return single_open(file, dcprs_log_show, inode->i_private);
}


static ssize_t dcprs_log_write(struct file *file, const char *buf, size_t length, loff_t *pOff)
{
    char *cur, kbuf[8];
    uint32_t x;

    if (copy_from_user(kbuf, (void __user *)buf, sizeof(kbuf))) {
        pr_err("efault");
        return length;
    }

    x = simple_strtol(kbuf, &cur, 0);

    pr_info("Got command: %d", x);

    return length;
}


static const struct file_operations dcprs_log_fops = {
    .owner		= THIS_MODULE,
    .open		= dcprs_log_open,
    .write		= dcprs_log_write,
    .read		= seq_read,
    .llseek		= seq_lseek,
    .release	= single_release,
};


static void mkblob(struct device *dev, char *name, uint32_t sz, void *p, struct dentry *dp)
{
    struct debugfs_blob_wrapper *desc_blob;

    desc_blob = devm_kzalloc(dev, sizeof(*desc_blob), GFP_KERNEL);
    if (desc_blob) {
        desc_blob->data = p;
        desc_blob->size = sz;
        debugfs_create_blob(name, S_IFREG | 0400, dp, desc_blob);
    } else {
        pr_err("mkblob: no core");
    }
}


#define dump_register(nm, off)       \
{                               \
        .name   = #nm,          \
        .offset = off,           \
}
static const struct debugfs_reg32 desc_regs[] = {
    dump_register(dcprs_desc_word00_t, 0x00),
    dump_register(dcprs_desc_word01_t, 0x04),
    dump_register(dcprs_desc_word02_t, 0x08),
    dump_register(dcprs_desc_word03_t, 0x0C),
    dump_register(dcprs_desc_word04_t, 0x10),
    dump_register(dcprs_desc_word05_t, 0x14),
    dump_register(dcprs_desc_word06_t, 0x18),
    dump_register(dcprs_desc_word07_t, 0x1C),
    dump_register(dcprs_desc_word08_t, 0x20),
    dump_register(dcprs_desc_word09_t, 0x24),
    dump_register(dcprs_desc_word10_t, 0x28),
};

static void mkregset(struct device *dev, char *name, uint32_t sz, void *p, struct dentry *d)
{
    struct debugfs_regset32 *regset;

    regset = (struct debugfs_regset32 *)devm_kzalloc(dev, sizeof(*regset), GFP_KERNEL);

    regset->regs  = desc_regs;
    regset->nregs = ARRAY_SIZE(desc_regs);
    regset->base  = p;

    debugfs_create_regset32(name, 0400, d, regset);
}

#endif // CONFIG_DEBUG_FS


static int check_frame(void *addr, uint32_t len, int arg)
{
    uint32_t  *wp = (uint32_t *)addr;

    if (*wp != 0xFFD8FFF7) {
        trace_ml_dcprs_hdr_err(arg, 0xFFD8FFF7, *wp);
        pr_err("no SOF on %s chan, expected 0xFFD8FFF7, got 0x%08X\n", addr == 0 ? "G" : "RB", *wp);
        return -1;
    }

    return 0;
}


void dcprs_conf(uint32_t slice_length, uint32_t bpp)
{
    struct dcprs_dev *ddev = &g_dev[0];

    pr_info("DCPRS config: configure for slice length of %d, bpp=%d\n", slice_length, bpp);

    dcprs_lib_init(ddev->regs, bpp);

    // Reset slice and section counters.
    ddev->pend.slice_length = 0;
    ddev->pend.slice_count   = 0;
    ddev->pend.byte_count_g  = 0;
    ddev->pend.byte_count_rb = 0;

    ddev->slice_length = slice_length;
    ddev->bpp          = bpp;

    ddev->grayscale = 0;

    ddev->configured = 1;
}
EXPORT_SYMBOL(dcprs_conf);


int dcprs_is_busy(void)
{
    struct dcprs_dev *ddev = &g_dev[0];

    return atomic_read(&ddev->busy);
}
EXPORT_SYMBOL(dcprs_is_busy);


int dcprs_decompress_start(void *addr_g, void *addr_rb, void *addr_dst, dma_addr_t ioaddr_dst, dma_addr_t ioaddr_g, dma_addr_t ioaddr_rb, uint32_t len_g, uint32_t len_rb)
{
    dcprs_rgf_watermark_threshold_reg_t wmrk_th_reg;
    dcprs_rgf_dcprs_axi_err_interrupt_reg__enable_t wm_int_enable_reg;

    static int count = 0;
    uint32_t wmark;
    struct dcprs_dev *ddev = &g_dev[0];

    if ((check_frame(addr_g, len_g, 0) < 0) || (check_frame(addr_rb, len_rb, 1) < 0)) {
        pr_err("bad image!\n");
        ddev->pend.complete = 1;
        wake_up_interruptible(&ddev->ev_waitq);
        return -1;
    }

    // Pad out end of image to get around hydra bug.
    memset(addr_g  + len_g,  0, 32);
    memset(addr_rb + len_rb, 0, 32);

    // Make sure it's not still busy.
    rmb();
    if (atomic_read(&ddev->busy)) {
        uint32_t err, event;

        trace_ml_dcprs_start_busy(0);
        event = readl(&default_mp->dcprs_mp.dcprs_func_event_reg);
        err   = readl(&dcprs->dcprs_mptop.rgf.dcprs_axi_err_interrupt_reg.register_value);
        pr_err("dcprs not ready: pend_cmpl=%d, err=0x%08x, event=0x%08x\n",
                        ddev->pend.complete, err, event);
        return -EBUSY;
    }
    atomic_set(&ddev->busy, 1);

    count++;

    // Make sure that dcprs_conf() was called already.
    if (! ddev->configured) {
        pr_err("device hasn't been configured yet");
        return -EPERM;
    }

    // Configure watermark threshold
    wmrk_th_reg.register_value       = 0;
    wm_int_enable_reg.register_value = 0;
    if (ddev->slice_length) {
        wmark = ALIGN(ddev->slice_length,  16) - 16;

        // Enable watermark interrupt
        wm_int_enable_reg.bitfield.m0e0_wmrk = 1;
        write_reg(dcprs->dcprs_mptop.rgf.dcprs_axi_err_interrupt_reg_int_en, wm_int_enable_reg.register_value);
    } else {
        wmark = (1024 * 1024 * 14); // threshold counts in 16-byte units
    }
    wmrk_th_reg.bitfield.wmrk_th = wmark >> 4; // threshold counts in 16-byte units
    write_reg(dcprs->dcprs_mptop.rgf.m0e0_watermark_threshold_reg, wmrk_th_reg.register_value);

    dcprs_config_descriptor(ioaddr_g,  ioaddr_dst, DCPRS_ENGINE_G);
    dcprs_config_descriptor(ioaddr_rb, 0, DCPRS_ENGINE_RB);

    writel(DCPRS_FRAME_SEC_MODE_DISABLED, &default_mp->dcprs_mp.engine0_frm_sec_enb_reg);
    writel(DCPRS_FRAME_SEC_MODE_DISABLED, &default_mp->dcprs_mp.engine1_frm_sec_enb_reg);

    // Set compressed data size.
    dcprs_default_mp0_eng0_desc.word4.bitfield.axirdch0rs = len_g  + 16;
    dcprs_default_mp0_eng1_desc.word4.bitfield.axirdch0rs = len_rb + 16;

    dcprs_write_descriptor(&dcprs_default_mp0_eng0_desc, DCPRS_DEFAULT_MP_INDEX, DCPRS_ENGINE_G,  DCPRS_DEFAULT_DESCRIPTOR_INDEX);
    dcprs_write_descriptor(&dcprs_default_mp0_eng1_desc, DCPRS_DEFAULT_MP_INDEX, DCPRS_ENGINE_RB, DCPRS_DEFAULT_DESCRIPTOR_INDEX);

    trace_ml_dcprs_start(ioaddr_dst, ioaddr_g, ioaddr_rb, len_g, len_rb, wmark);

    // Execute descriptor
    ddev->pend.complete = 0;
    dcprs_exec_descriptor(1, 1);

    return 0;
}
EXPORT_SYMBOL(dcprs_decompress_start);


int dcprs_decompress_sect_start(void *addr_g, void *addr_rb, dma_addr_t ioaddr_dst, dma_addr_t ioaddr_g, dma_addr_t ioaddr_rb)
{
    dcprs_rgf_watermark_threshold_reg_t wmrk_th_reg;
    struct dcprs_dev *ddev = &g_dev[0];

    trace_ml_dcprs_start(ioaddr_dst, ioaddr_g, ioaddr_rb, 0, 0, 0);

    rmb();
    if (atomic_read(&ddev->busy)) {
        trace_ml_dcprs_start_busy(0);
        pr_err("DCPRS BUSY\n");
        return -EBUSY;
    }
    atomic_set(&ddev->busy, 1);

    g_buf[0] = addr_g;
    g_buf[1] = addr_rb;
    g_cnt[0] = 0;
    g_cnt[1] = 0;

    // Configure watermark threshold
    wmrk_th_reg.register_value = 0;
    wmrk_th_reg.bitfield.wmrk_th = (1024 * 1024 * 14) >> 4; // threshold counts in 16-byte units
    write_reg(dcprs->dcprs_mptop.rgf.m0e0_watermark_threshold_reg, wmrk_th_reg.register_value);

    dcprs_config_descriptor(ioaddr_g,  ioaddr_dst, DCPRS_ENGINE_G);
    dcprs_config_descriptor(ioaddr_rb, 0, DCPRS_ENGINE_RB);

    writel(DCPRS_FRAME_SEC_MODE_ENABLED, &default_mp->dcprs_mp.engine0_frm_sec_enb_reg);
    writel(DCPRS_FRAME_SEC_MODE_ENABLED, &default_mp->dcprs_mp.engine1_frm_sec_enb_reg);

    dcprs_write_descriptor(&dcprs_default_mp0_eng0_desc, DCPRS_DEFAULT_MP_INDEX, DCPRS_ENGINE_G,  DCPRS_DEFAULT_DESCRIPTOR_INDEX);
    dcprs_write_descriptor(&dcprs_default_mp0_eng1_desc, DCPRS_DEFAULT_MP_INDEX, DCPRS_ENGINE_RB, DCPRS_DEFAULT_DESCRIPTOR_INDEX);

    // Execute descriptor
    ddev->pend.complete = 0;
    dcprs_exec_descriptor(1, 1);

    return 0;
}
EXPORT_SYMBOL(dcprs_decompress_sect_start);


int dcprs_decompress_sect_next(uint32_t eng_ix, uint32_t len)
{
    if ( (eng_ix != DCPRS_ENGINE_G) && (eng_ix != DCPRS_ENGINE_RB)) {
        pr_err("%s: bad engine id: %d\n", __func__, eng_ix);
        return -1;
    }

    trace_ml_dcprs_sect_next(eng_ix, len, g_cnt[eng_ix]);

    if (len == 0) {
        memset(g_buf[eng_ix] + g_cnt[eng_ix],  0, 32);
        dcprs_section_received(eng_ix, 16);
        dcprs_eof_received(eng_ix);
    } else {
        g_cnt[eng_ix] += len;
        dcprs_section_received(eng_ix,  len);
    }

    return 0;
}
EXPORT_SYMBOL(dcprs_decompress_sect_next);


int dcprs_decompress_end(uint32_t *count)
{
    struct dcprs_dev *ddev = &g_dev[0];
    int to;

    //pr_info("[W0]\n");

    // Wait up to 1 sec for completion.
    trace_ml_dcprs_end_wait_begin(0);
    to = wait_event_interruptible_timeout(ddev->ev_waitq, (ddev->pend.complete), HZ * 2);

    // Since the watermark count is only right for 8bpp, hardwire the alternative for 10bpp for now.
    *count = (ddev->bpp == 8) ? (dcprs->dcprs_mptop.rgf.m0e0_watermark_last_cntr_reg.register_value - 1) * 16 : IMG_SIZE_12MP_10bpp;

    trace_ml_dcprs_end_wait_done(to, *count);
    if (to == 0) {
        pr_err("Timeout!\n");
#ifdef CONFIG_DEBUG_FS
        dcprs_dump();
#endif
        dump_jpegls();
        pr_info("Dump Desc #0:\n");
        dcprs_dump_descriptor(&dcprs_default_mp0_eng0_desc);
        pr_info("Dump Desc #1:\n");
        dcprs_dump_descriptor(&dcprs_default_mp0_eng1_desc);
        return -ETIME;
    }
    if (to < 0) {
        if (to != -ERESTARTSYS) {
            pr_err("wait error: %d\n", to);
        }
        atomic_set(&ddev->busy, 0);
        return to;
    }

    atomic_set(&ddev->busy, 0);
    wmb();
    //pr_info("[W1]\n");

    return 0;
}
EXPORT_SYMBOL(dcprs_decompress_end);


static int dcprs_probe(struct platform_device *pdev)
{
    volatile dcprs_t *dp;
    int ret, irq, irq_count, i;
    struct resource *mem;
    struct dcprs_dev *ddev = &g_dev[0];

    atomic_set(&ddev->busy, 0);

    // Get device address base.
    if ((mem = platform_get_resource(pdev, IORESOURCE_MEM, 0))) {
        DPRINT("DCPRS io base is at 0x%016llx, ends at 0x%016llx", mem->start, mem->end);
    } else {
        pr_err("can't get DCPRS base address");
        return -1;
    }

    if ((irq_count = platform_irq_count(pdev)) < 1) {
        pr_err("can't get irq count");
        return -1;
    }

    // Get our interrupt lines.
    pr_info("Total irq count is %d", irq_count);
    for (i = 0; i < irq_count; i++) {
        if ((irq = platform_get_irq(pdev, i))) {
            if ((ret = devm_request_irq(&pdev->dev, irq, dcprs_intr, 0,  dev_name(&pdev->dev), ddev))) {
                pr_err("error in request_irq: %d\n", ret);
                continue;
                return -1;
            }
            if (! i) {
                ddev->irq_first = irq;
            }
        } else {
            pr_err("can't get DCPRS irq");
            return -1;
        }
    }
    pr_info("First acquired irq is %d\n", ddev->irq_first);

    // Create device entry.
    if (dcprs_mkdev(ddev)) {
        pr_err("couldn't create device");
        return -1;
    }

    // Map in device.
    ddev->regs = devm_ioremap_resource(&pdev->dev, mem);
    if (IS_ERR(ddev->regs)) {
        pr_err("failed to map device");
        dcprs_rmdev(ddev);
        return PTR_ERR(ddev->regs);
    }

#ifdef CONFIG_DEBUG_FS
    ddev->dir = debugfs_create_dir(dev_name(&pdev->dev), g_dir);
    if (IS_ERR(ddev->dir)) {
        pr_warn("Failed to create debugfs: %ld\n",
                PTR_ERR(ddev->dir));
        ddev->dir = NULL;
    } else {
        volatile dcprs_t *dd = (volatile dcprs_t *)ddev->regs;
        cast_t *cp0;
        struct dentry *dir0, *regs0, *eng0, *eng1;

        debugfs_create_x64("base_va", 0400, ddev->dir, (uint64_t *)&ddev->regs);

        debugfs_create_u32("debug",  0600, ddev->dir, (uint32_t *)&g_debug);

        dir0  = debugfs_create_dir("mp0",  ddev->dir);
        eng0  = debugfs_create_dir("eng0", dir0);
        eng1  = debugfs_create_dir("eng1", dir0);
        regs0 = debugfs_create_dir("regs", dir0);

        debugfs_create_file("log", 0400, ddev->dir, ddev, &dcprs_log_fops);
        debugfs_create_file("desc0", 0400, ddev->dir, ddev, &dcprs_desc0_fops);
        debugfs_create_file("desc1", 0400, ddev->dir, ddev, &dcprs_desc1_fops);

        mkblob(&pdev->dev, "dcprs_eng0_desc_mem", sizeof(dd->dcprs_mptop.dcprs_mp0.dcprs_mp.dcprs_eng0_desc_mem), (void *)&dd->dcprs_mptop.dcprs_mp0.dcprs_mp.dcprs_eng0_desc_mem, eng0);
        mkblob(&pdev->dev, "dcprs_eng1_desc_mem", sizeof(dd->dcprs_mptop.dcprs_mp0.dcprs_mp.dcprs_eng1_desc_mem), (void *)&dd->dcprs_mptop.dcprs_mp0.dcprs_mp.dcprs_eng1_desc_mem, eng1);

        debugfs_create_u64("count",  0400, ddev->dir, (uint64_t *)&g_dev->stats.count);

        debugfs_create_x32("dcprjpeglsdec_cfg_mems_func_event_reg",  0400, eng0, (uint32_t *)&dd->dcprs_mptop.dcprs_mp0.eng0.jpeglsdec_cfg_mem->register_value);
        debugfs_create_x32("dcprjpeglsdec_cfg_mems_func_event_reg",  0400, eng1, (uint32_t *)&dd->dcprs_mptop.dcprs_mp0.eng1.jpeglsdec_cfg_mem->register_value);

        debugfs_create_x32("dcprs_func_event_reg",    0400, regs0, (uint32_t *)&dd->dcprs_mptop.dcprs_mp0.dcprs_mp.dcprs_func_event_reg.register_value);
        debugfs_create_x32("dcprs_desc_status_reg",   0400, regs0, (uint32_t *)&dd->dcprs_mptop.dcprs_mp0.dcprs_mp.dcprs_desc_status_reg.register_value);

        debugfs_create_x32("master128apb_status_reg", 0600, regs0, (uint32_t *)&dd->axim.status.register_value);
        debugfs_create_x32("master128apb_configx_reg", 0600, regs0, (uint32_t *)&dd->axim.configx.register_value);
        debugfs_create_x32("master128apb_max_burst_len_wr_reg",   0600, regs0, (uint32_t *)&dd->axim.max_burst_len_wr.register_value);
        debugfs_create_x32("master128apb_max_burst_len_rd_reg",   0600, regs0, (uint32_t *)&dd->axim.max_burst_len_rd.register_value);

        debugfs_create_x32("dcprs_axird_burst_reg",   0400, regs0, (uint32_t *)&dd->dcprs_mptop.rgf.dcprs_axird_burst_reg.register_value);
        debugfs_create_x32("dcprs_axiwr_burst_reg",   0400, regs0, (uint32_t *)&dd->dcprs_mptop.rgf.dcprs_axiwr_burst_reg.register_value);

        debugfs_create_x32("dcprs_axiwr_bp_reg", 0600, regs0, (uint32_t *)&dd->dcprs_mptop.rgf.dcprs_axiwr_bp_reg.register_value);

        cp0 = (cast_t *)dd->dcprs_mptop.dcprs_mp0.eng0.jpeglsdec_cfg_mem;
        if (cp0->version != CAST_VER) {
            pr_info("dcprs core version doesn't match, expected 0x%08X but got 0x%08X\n", CAST_VER, cp0->version);
        }

        mkregset(&pdev->dev, "desc", sizeof(dcprs_desc_t), (void *)&dd->dcprs_mptop.dcprs_mp0.dcprs_mp.dcprs_eng0_desc_mem, eng0);
        mkregset(&pdev->dev, "desc", sizeof(dcprs_desc_t), (void *)&dd->dcprs_mptop.dcprs_mp0.dcprs_mp.dcprs_eng1_desc_mem, eng1);
    }
#endif

    // Fix Backpressure Threshold Registers
    dp = (volatile dcprs_t *)ddev->regs;
    write_reg(dp->dcprs_mptop.rgf.dcprs_axiwr_bp_reg, 530);
    write_reg(dp->dcprs_mptop.rgf.dcprs_axird_burst_reg.register_value, 0x80);
    write_reg(dp->axim.max_burst_len_rd.register_value, 0x80);

    ddev->dev = &pdev->dev;

    platform_set_drvdata(pdev, &g_dev);

    init_waitqueue_head(&ddev->ev_waitq);

    DPRINT("DCPRS device initialized\n");

    return 0;
}


static int dcprs_remove(struct platform_device *pdev)
{
    return 0;
}


// count
static ssize_t count_show(struct device *dev, struct device_attribute *attr, char *buf);
static DEVICE_ATTR_RO(count);

static struct attribute *dcprs_attributes[] = {
    &dev_attr_count.attr,
    NULL
};
static const struct attribute_group dcprs_attr_group = {
    .attrs = dcprs_attributes,
};

static const struct of_device_id dcprs_of_match[] = {
    { .compatible = "amd,dcprs", },
    { },
};

static struct platform_driver g_drv = {
    .probe  = dcprs_probe,
    .remove = dcprs_remove,
    .driver = {
        .name = "dcprs",
        .of_match_table = dcprs_of_match,
    },
};


static ssize_t count_show(struct device *dev,
                          struct device_attribute *attr, char *buf)
{
    char *bbuf = buf;

    // g_dev->stats.count);
    buf += snprintf(buf, PAGE_SIZE, "%lld\n", g_dev[0].stats.count);

    return buf - bbuf;
}


static int __init dcprs_init(void)
{
    int ret;
    dev_t devt;

    DPRINT("DCPRS module init");

    /* Register the device class */
    g_class = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(g_class)) {
        pr_err("dcprs: Failed to register device class.\n");
        return PTR_ERR(g_class);
    }

    ret = alloc_chrdev_region(&devt, 0, CDEV_NUM, "dcprs");
    if (ret) {
        pr_err("dcprs: failed to alloc_chrdev_region\n");
        class_destroy(g_class);
        return -EBUSY;
    }
    g_major = MAJOR(devt);

#ifdef CONFIG_DEBUG_FS
    DPRINT("debugfs_create_dir");
    g_dir = debugfs_create_dir("dcprs", NULL);
    if (! g_dir) {
        pr_err("Can't create debugfs dir");
        unregister_chrdev_region(devt, CDEV_NUM);
        class_destroy(g_class);
        return -ENODEV;
    }
#endif

    g_dev[0].sys_dev_root = root_device_register("dcprs");
    if (IS_ERR(g_dev[0].sys_dev_root)) {
        pr_err("bad sys_dev create.\n");
        return -1;
    }
    ret = sysfs_create_group(&g_dev[0].sys_dev_root->kobj, &dcprs_attr_group);
    if (ret < 0) {
        pr_err("bad sysfs_create_group (%d)\n", ret);
        root_device_unregister(g_dev[0].sys_dev_root);
        g_dev[0].sys_dev_root = 0;
        return -1;
    }

    ret = platform_driver_register(&g_drv);
    if (ret) {
#ifdef CONFIG_DEBUG_FS
        debugfs_remove_recursive(g_dir);
#endif
        unregister_chrdev_region(devt, CDEV_NUM);
        class_destroy(g_class);
    }

    return ret;
}


static void __exit dcprs_exit(void)
{
    platform_driver_unregister(&g_drv);
#ifdef CONFIG_DEBUG_FS
    debugfs_remove_recursive(g_dir);
#endif

    if (g_dev[0].sys_dev_root) {
        sysfs_remove_group(&g_dev[0].sys_dev_root->kobj, &dcprs_attr_group);
        root_device_unregister(g_dev[0].sys_dev_root);
        g_dev[0].sys_dev_root = 0;
    }

    unregister_chrdev_region(g_major, CDEV_NUM);
    class_destroy(g_class);
}


module_init(dcprs_init);
module_exit(dcprs_exit);
MODULE_DEVICE_TABLE(of, dcprs_of_match);

MODULE_DESCRIPTION("CVIP DCPRS Driver");

