// SPDX-License-Identifier: GPL-2.0-only
//
// Copyright (C) 2022 Magic Leap, Inc. All rights reserved.
//

// pr_fmt will insert our module name into the pr_info() macro.
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
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
#include <linux/iommu.h>
#include <linux/pci_cvip.h>
#include <linux/hashtable.h>
#include <ion/ion.h>

#define CREATE_TRACE_POINTS // don't move, needs to be here!
#include "cdp_trace.h"

#include <linux/version.h>

#include "cvcore_name_stub.h"
#include "cdp.h"
#include "cdp_lib.h"
#include "cdp_desc.h"
#include "cdp_top.h"
#include "cdp_config.h"
#include "shregion_types.h"


MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Magic Leap, Inc.");
MODULE_AUTHOR("imagyar@magicleap.com");
MODULE_DESCRIPTION("CVIP Corner Detection Pipeline (CDP) Engine");

// CDP interrupt sources
#define irq_cdp_master_intr          0
#define irq_cdp_master_freeze_intr   1
#define irq_cdp_master_halt_intr     2

#define DPRINT  if (g_dbg_mask) pr_info

#define OUTBUF_SIZE (1024 * 1024 * 2)

#define NUM_MAPPED_REGIONS  (8)

#define CLASS_NAME "cdp"
#define CDEV_NUM 255
#define CDP_IS_ERR(x) (x < 0)
#define SAMPLE_SIZE (16)
#define STAGES      (3)



struct cdp_fast_sum {
    uint16_t    scores_s0;
    uint16_t    scores_s1;
    uint16_t    scores_s2;
};


extern int get_shregion_vaddr(struct shregion_metadata *meta, void **vaddr);
extern int kalloc_shregion(struct shregion_metadata *meta, void **vaddr);
extern int get_shregion_cdp_dcprs_ioaddr(uint64_t shr_name, ShregionCdpDcprsIOaddr *shr_cdp_dcprs_vaddr);
extern struct dma_buf *get_dma_buf_for_shr(uint64_t shr_name);
extern void cdp_debugfs_init(struct dentry *root, struct cdp_hw_err *errs);

static int  cdp_open(struct inode *, struct file *);
static int  cdp_release(struct inode *, struct file *);
static long cdp_ioctl(struct file *, unsigned int, unsigned long);
static int cdp_mmap(struct file *filp, struct vm_area_struct *vma);
static int alloc_out_buf(struct device *dev, uint64_t shname, uint32_t size, void **kaddr, uint64_t *io_addr);

static cdp_stage_desc_t *g_stage_descr_tab[3] = { &cdp_default_desc.stage0_desc, &cdp_default_desc.stage1_desc, &cdp_default_desc.stage2_desc };


// Device-global data.
static struct cdp_dev {
    struct resource     *res;
    struct device       *dev;
    void                *regs;
#ifdef CONFIG_DEBUG_FS
    struct dentry       *dir;
    uint32_t            norun;
#endif
    dev_t               minor;
    struct cdev         cdev;
    int                 irq_first;
    struct mutex        dev_lock;
    wait_queue_head_t   ev_waitq;
    uint32_t            opened;
    uint32_t            opened_by_pid;
    char                opened_by_name[TASK_COMM_LEN + 1];
    int                 configured;

    uint64_t mmap_threshold_offset, mmap_threshold_size;

    struct  cdp_hw_err err;

    uint32_t mapped_region_nxt;
    struct {
        uint64_t start;
        uint64_t end;
    } mapped_region[NUM_MAPPED_REGIONS];

    struct {
        dma_addr_t                  axi_io;
        void                        *axi;
        dma_addr_t                  fast_io;
        void                        *fast;
        struct debugfs_blob_wrapper blob_axi;
        struct debugfs_blob_wrapper blob_fast;
    } buf_out[STAGES];

    void                        *data_out_sum;
    dma_addr_t                  data_out_sum_io;

    struct debugfs_blob_wrapper blob_sum;

    struct {
        int        complete;
        dma_addr_t dst_addr_io;
        dma_addr_t src_addr_io;
    } pend;

} g_dev;


static struct file_operations const g_fops_cdp = {
    .owner          = THIS_MODULE,
    .open           = cdp_open,
    .release        = cdp_release,
    .mmap           = cdp_mmap,
    .unlocked_ioctl = cdp_ioctl,
    .compat_ioctl   = cdp_ioctl,
};

static int g_dbg_mask;
static int g_reg_dump;
static int g_major;
static struct class *g_class;
#ifdef CONFIG_DEBUG_FS
static struct dentry *g_dir;
#endif

module_param_named(dbg, g_dbg_mask, uint,  0644);
module_param_named(reg_dump, g_reg_dump, uint,  0644);


static void *vaddr(uint64_t cvcore_name)
{
    struct shregion_metadata meta;
    void *vaddr;

    meta.name = cvcore_name;
    get_shregion_vaddr(&meta, &vaddr);

    return vaddr;
}


//--------------------------------------------------------------------------
// cdp_open()
//--------------------------------------------------------------------------
static int cdp_open(struct inode *pInode, struct file *pFile)
{
    pFile->private_data = &g_dev;

    mutex_lock(&(g_dev.dev_lock));

    // Limit access to this device to one process at a time.
    if (g_dev.opened) {
        pr_err("CDP dev already opened by another process\n");
        mutex_unlock(&(g_dev.dev_lock));
        return -EBUSY;
    }
    g_dev.configured = 0;
    g_dev.opened = 1;
    g_dev.opened_by_pid = current->pid;
    memcpy(g_dev.opened_by_name, current->comm, TASK_COMM_LEN);

    mutex_unlock(&(g_dev.dev_lock));

    return 0;
}


//--------------------------------------------------------------------------
// cdp_release()
//--------------------------------------------------------------------------
static int cdp_release(struct inode *pInode, struct file *pFile)
{
    struct cdp_dev *ddev = (struct cdp_dev *)pFile->private_data;

    ddev->configured = 0;
    ddev->opened = 0;

    return 0;
}


static void config_per_image(struct cdp_dev *ddev, struct cdp_ioctl *parg)
{
    static cdp_stage_desc_t *sp[3] = { &cdp_default_desc.stage0_desc, &cdp_default_desc.stage1_desc, &cdp_default_desc.stage2_desc };
    int i;

    // Copy per-image config data.
    for (i = 0; i < CDP_N_STAGES; i++) {

        DPRINT("Per/Image Configuration for stage #%d\n", i);

        g_stage_descr_tab[i]->word8.bitfield.cand_max_per_win = parg->run.nms_max_results_per_win[i];

        g_stage_descr_tab[i]->word6.bitfield.fast_thold_h_win_size = parg->run.thresh_wins->grid_size_x;
        g_stage_descr_tab[i]->word6.bitfield.fast_thold_v_win_size = parg->run.thresh_wins->grid_size_y;

        g_stage_descr_tab[i]->word6.bitfield.fast_seqln = parg->run.fast_seq_len[i];

        if (parg->run.thresh_wins[i].n_thresholds) {
            DPRINT("    Configuring %d windows for stage %d\n", parg->run.thresh_wins[i].n_thresholds, i);
            cdp_config_thresholds(i, &parg->run.thresh_wins[i]);
            sp[i]->word0.bitfield.fast_no_mem = 0;
            g_stage_descr_tab[i]->word13.bitfield.fast_thold_no_mem = 0;
        } else {
            DPRINT("    Configuring global threshold for stage %d\n", i);
            sp[i]->word0.bitfield.fast_no_mem = 1;
            g_stage_descr_tab[i]->word13.bitfield.fast_thold_no_mem = parg->run.fast_thresh_glob[i];
        }

        // Gaussian blur enable and coefficients.
        g_stage_descr_tab[i]->word0.bitfield.gauss_en               = parg->run.gauss->enable;
        g_stage_descr_tab[i]->word3.bitfield.gauss_coeff_bbb        = parg->run.gauss->bbb;
        g_stage_descr_tab[i]->word3.bitfield.gauss_coeff_center     = parg->run.gauss->center;
        g_stage_descr_tab[i]->word4.bitfield.gauss_coeff_cross_in   = parg->run.gauss->cross_in;
        g_stage_descr_tab[i]->word4.bitfield.gauss_coeff_cross_out  = parg->run.gauss->cross_out;
        g_stage_descr_tab[i]->word5.bitfield.gauss_coeff_corner_in  = parg->run.gauss->corner_in;
        g_stage_descr_tab[i]->word5.bitfield.gauss_coeff_corner_out = parg->run.gauss->corner_out;
        g_stage_descr_tab[i]->word5.bitfield.gauss_shiftback        = parg->run.gauss->shift;

        // Enable/Disable blur+downrez on FAST pipe.
        g_stage_descr_tab[i]->word0.bitfield.alg_bypass = parg->run.pyr_gauss_dis_fast[i];

        // NMS sliding window size.
        g_stage_descr_tab[i]->word7.bitfield.use_3x3_win = (parg->run.nms_slide_win[i] == WIN_3x3) ? 1 : 0;

        // NMS results window size.
        g_stage_descr_tab[i]->word7.bitfield.cand_h_win_size = parg->run.nms_results_win_size_x[i];
        g_stage_descr_tab[i]->word7.bitfield.cand_v_win_size = parg->run.nms_results_win_size_y[i];

        if (parg->run.roi[i].cir.enable) {
            DPRINT("    Enable circle: r=%d, x=%d, y=%d\n", parg->run.roi[i].cir.radius, parg->run.roi[i].cir.center_x, parg->run.roi[i].cir.center_y);
            g_stage_descr_tab[i]->word9.bitfield.roi_cir_en    = 1;
            g_stage_descr_tab[i]->word9.bitfield.roi_cir_r     = parg->run.roi[i].cir.radius * parg->run.roi[i].cir.radius;
            g_stage_descr_tab[i]->word10.bitfield.roi_cir_cxcx = parg->run.roi[i].cir.center_x * parg->run.roi[i].cir.center_x;
            g_stage_descr_tab[i]->word11.bitfield.roi_cir_cycy = parg->run.roi[i].cir.center_y * parg->run.roi[i].cir.center_y;
            g_stage_descr_tab[i]->word12.bitfield.roi_cir_cx   = parg->run.roi[i].cir.center_x;
            g_stage_descr_tab[i]->word12.bitfield.roi_cir_cy   = parg->run.roi[i].cir.center_y;
        } else {
            g_stage_descr_tab[i]->word9.bitfield.roi_cir_en    = 0;
        }

        if (parg->run.roi[i].rec.enable) {
            DPRINT("    Enable rec: border=%d\n", parg->run.roi[i].rec.border);
            g_stage_descr_tab[i]->word8.bitfield.roi_en               = 1;
            g_stage_descr_tab[i]->word8.bitfield.roi_remove_pix_width = parg->run.roi[i].rec.border;
        } else {
            g_stage_descr_tab[i]->word8.bitfield.roi_en    = 0;
        }
    }
}


static void config_once(struct cdp_dev *ddev, struct cdp_ioctl *parg)
{
    int i;

    DPRINT("Configuring device\n");

    cdp_default_desc.global_desc.word2.bitfield.next_frame_rs = parg->cfg.pyr[0].x_in;

    // Allocate the output buffer for the NMS summary.
    alloc_out_buf(ddev->dev, CVCORE_NAME_STATIC(CDP_DRV_SHREGION_OUTPUT), OUTBUF_SIZE, &g_dev.data_out_sum,   &g_dev.data_out_sum_io);
    DPRINT("FAST summary: data_out_sum=%px,  data_out_sum_io=0x%llx\n", g_dev.data_out_sum, g_dev.data_out_sum_io);
#ifdef CONFIG_DEBUG_FS
    g_dev.blob_sum.data = (void *)g_dev.data_out_sum;
#endif

    for (i = 0; i < CDP_N_STAGES; i++) {
        // Pyramid resizing config.
        if (parg->cfg.pyr[i].scale == CDP_PYRAMID_RESIZE_1_1) {
            g_stage_descr_tab[i]->word0.bitfield.nxt_pyr_en = 0;
            g_stage_descr_tab[i]->word0.bitfield.alg_pyr_en = 0;
            g_stage_descr_tab[i]->word0.bitfield.isp_pyr_en = 0;
        }
        g_stage_descr_tab[i]->word2.bitfield.resize_sel              = parg->cfg.pyr[i].scale;
        g_stage_descr_tab[i]->word1.bitfield.in_image_num_of_lines   = parg->cfg.pyr[i].y_in;
        g_stage_descr_tab[i]->word1.bitfield.in_image_num_of_pixels  = parg->cfg.pyr[i].x_in;
        g_stage_descr_tab[i]->word2.bitfield.out_image_num_of_lines  = parg->cfg.pyr[i].y_out;
        g_stage_descr_tab[i]->word2.bitfield.out_image_num_of_pixels = parg->cfg.pyr[i].x_out;
    }
}


static int process(struct cdp_dev *ddev, struct cdp_ioctl *parg)
{
    struct cdp_fast_sum *psum;
    void *in_va = 0;
    void *out_va[CDP_N_STAGES];
    int to, retval;

    in_va = vaddr(parg->stage_bufs[0].input.shname);

    DPRINT("s0: inp=0x%llX   axi=0x%llX   fast=0x%llX\n", parg->stage_bufs[0].input.addr, parg->stage_bufs[0].out_axi.addr, parg->stage_bufs[0].out_fast.addr);
    DPRINT("s1: inp=0x%llX   axi=0x%llX   fast=0x%llX\n", parg->stage_bufs[1].input.addr, parg->stage_bufs[1].out_axi.addr, parg->stage_bufs[1].out_fast.addr);
    DPRINT("s0: inp=0x%llX   axi=0x%llX   fast=0x%llX\n", parg->stage_bufs[2].input.addr, parg->stage_bufs[2].out_axi.addr, parg->stage_bufs[2].out_fast.addr);

    out_va[0] = vaddr(parg->stage_bufs[0].out_axi.shname);
    out_va[1] = vaddr(parg->stage_bufs[1].out_axi.shname);
    out_va[2] = vaddr(parg->stage_bufs[2].out_axi.shname);

    cdp_config_stage_descriptor(CDP_DESC_STAGE_0, parg->stage_bufs[0].out_axi.addr, parg->stage_bufs[0].out_fast.addr);
    cdp_config_stage_descriptor(CDP_DESC_STAGE_1, parg->stage_bufs[1].out_axi.addr, parg->stage_bufs[1].out_fast.addr);
    cdp_config_stage_descriptor(CDP_DESC_STAGE_2, parg->stage_bufs[2].out_axi.addr, parg->stage_bufs[2].out_fast.addr);

    cdp_config_global_descriptor(parg->stage_bufs[0].input.addr, g_dev.data_out_sum_io);

    // Per-image config.
    config_per_image(ddev, parg);

    cdp_write_descriptor(CDP_DEFAULT_DESCRIPTOR_INDEX, CDP_DESC_FULL, &cdp_default_desc);

#ifdef CONFIG_DEBUG_FS
    if (g_dev.norun) {
        pr_info("Skipping descriptor execution.\n");
        return 0;
    }
#endif

    trace_ml_cdp_start(parg->stage_bufs[0].input.addr, parg->stage_bufs[1].input.addr, parg->stage_bufs[2].input.addr);

    ddev->pend.complete = 0;
    retval = cdp_exec_descriptor();
    if (CDP_IS_ERR(retval)) {
        pr_err("failed to execute descriptor.\n");
        return retval;
    }

    to = wait_event_interruptible_timeout(ddev->ev_waitq, (ddev->pend.complete), HZ * 1);
    if (to == 0) {
        pr_err("timeout\n");
        return -ETIME;
    }
    if (to < 0) {
        if (to != -ERESTARTSYS) {
            pr_err("wait error: %d\n", to);
        }
        return to;
    }
    if (to > 0) {
        psum = (struct cdp_fast_sum *)g_dev.data_out_sum;
        DPRINT("f0=%d, f1=%d, f2=%d\n", psum->scores_s0, psum->scores_s1, psum->scores_s2);

        // Export number of rows in FAST output tables.
        parg->out.fast_count[0] = psum->scores_s0;
        parg->out.fast_count[1] = psum->scores_s1;
        parg->out.fast_count[2] = psum->scores_s2;

        // And error status.
        parg->out.hw_err.err_global = g_dev.err.global;
        parg->out.hw_err.err_master = g_dev.err.master;
        parg->out.hw_err.err[0] = g_dev.err.err[0];
        parg->out.hw_err.err[1] = g_dev.err.err[1];
        parg->out.hw_err.err[2] = g_dev.err.err[2];
    }

    return 0;
}


//--------------------------------------------------------------------------
// cdp_mmap()  Map the threshold memories into user space.
//--------------------------------------------------------------------------
static int cdp_mmap(struct file *filp, struct vm_area_struct *vma)
{
    struct cdp_dev *ddev = (struct cdp_dev *)filp->private_data;
    unsigned long size;

    // Make sure the user isn't trying to get more than what they're entitled to.
    size = ddev->mmap_threshold_size >> PAGE_SHIFT;
    if (vma->vm_pgoff + vma_pages(vma) != size) {
        pr_err("must allocate size of %ld\n", size);
        return -EINVAL;
    }

    vma->vm_page_prot = pgprot_device(vma->vm_page_prot);

    vma->vm_pgoff += (ddev->res->start >> PAGE_SHIFT);

    return io_remap_pfn_range(vma, vma->vm_start, vma->vm_pgoff, vma->vm_end - vma->vm_start, vma->vm_page_prot);
}


//--------------------------------------------------------------------------
// cdp_ioctl()
//--------------------------------------------------------------------------
static long cdp_ioctl(struct file *pFile, unsigned int cmd, unsigned long arg)
{
    struct cdp_dev *ddev = (struct cdp_dev *)pFile->private_data;
    static struct cdp_ioctl io_arg;
    struct cdp_ioctl *parg = &io_arg;
    int retval = 0;

    // Get a valid ioctl control struct.
    if (!access_ok((void *)arg, sizeof(io_arg))) {
        pr_err("ioctl: bad io_arg address\n");
        return -EPERM;
    }
    if (copy_from_user(&io_arg, (void __user *)arg, sizeof(io_arg))) {
        DPRINT("ioctl: copy_from_user");
        return -EFAULT;
    }

    mutex_lock(&(ddev->dev_lock));
    switch (cmd) {

        case CDP_MMAP:
            parg->mmap.threshold_offset = ddev->mmap_threshold_offset;
            parg->mmap.map_size         = ddev->mmap_threshold_size;
            break;

        case CDP_RUN:
            process(ddev, parg);
            break;

        case CDP_CONFIG:
            config_once(ddev, parg);
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

    mutex_unlock(&ddev->dev_lock);

    return retval;
}


static int cdp_mkdev(struct cdp_dev *ddev)
{
    int ret;
    struct cdev *cdev;
    struct device *device;

    cdev = &ddev->cdev;
    ddev->minor = 0;
    cdev_init(cdev, &g_fops_cdp);
    cdev->owner = THIS_MODULE;
    ret = cdev_add(cdev, MKDEV(g_major, ddev->minor), 1);
    if (ret) {
        pr_err("failed to add cdev.\n");
        return ret;
    }

    device = device_create(g_class, ddev->dev,
                           MKDEV(g_major, ddev->minor),
                           NULL, "cdp");
    if (IS_ERR(device)) {
        cdev_del(cdev);
        pr_err("failed to create the device.\n");
        return PTR_ERR(device);
    }

    ddev->dev = device;

    mutex_init(&ddev->dev_lock);

    return 0;
}


static irqreturn_t cdp_intr(int irq, void *dev)
{
    struct cdp_dev *ddev = dev;
    cdp_rgf_cdp_global_interrupt_reg_t intr_reg;
    uint32_t irq_ix;

    irq_ix = irq - ddev->irq_first;

    trace_ml_cdp_intr(irq, irq_ix, 0);

    DPRINT("CDP interrupt %d (#%d)\n", irq, irq_ix);

    if (irq_ix != irq_cdp_master_intr) {
        pr_err("Unexpected interrupt: #%d\n", irq_ix);
        return IRQ_HANDLED;
    }

    // Clear intr.
    intr_reg.register_value = 0;
    intr_reg.bitfield.cdp_done_intr = 1;
    writelX(intr_reg.register_value, &(cdp->eng.cdp_rgf.cdp_global_interrupt_reg));

    g_dev.err.err[0] = readl(&(cdp->eng.cdp_rgf.cdp_s0_interrupt_reg));
    g_dev.err.err[1] = readl(&(cdp->eng.cdp_rgf.cdp_s1_interrupt_reg));
    g_dev.err.err[2] = readl(&(cdp->eng.cdp_rgf.cdp_s2_interrupt_reg));
    g_dev.err.global = readl(&(cdp->eng.cdp_rgf.cdp_global_interrupt_reg));
    g_dev.err.master = readl(&(cdp->eng.cdp_rgf.cdp_master_interrupt_reg));

    // Clear error interrupts.
    if (g_dev.err.err[0] || g_dev.err.err[0] || g_dev.err.err[0] || g_dev.err.global) {
        DPRINT("s0_err=0x%08X, s1_err=0x%08X, s2_err=0x%08X, global_err=0x%08X, master_err=0x%08X\n",
               g_dev.err.err[0], g_dev.err.err[1], g_dev.err.err[2], g_dev.err.global, g_dev.err.master);
        writelX(g_dev.err.err[0], &(cdp->eng.cdp_rgf.cdp_s0_interrupt_reg));
        writelX(g_dev.err.err[1], &(cdp->eng.cdp_rgf.cdp_s1_interrupt_reg));
        writelX(g_dev.err.err[2], &(cdp->eng.cdp_rgf.cdp_s2_interrupt_reg));
        writelX(g_dev.err.global, &(cdp->eng.cdp_rgf.cdp_global_interrupt_reg));
    }

    // Handle this one separately, since writing a zero to this register seems to break the world.
    if (g_dev.err.master) {
        writelX(g_dev.err.master, &(cdp->eng.cdp_rgf.cdp_master_interrupt_reg));
    }

    ddev->pend.complete = 1;
    wake_up_interruptible(&ddev->ev_waitq);

    return IRQ_HANDLED;
}


#ifdef CONFIG_DEBUG_FS

static int cdp_glob_desc_show(struct seq_file *f, void *offset)
{
    struct cdp_dev *cdev = f->private;
    cdp_global_desc_t *desc = &cdp_default_desc.global_desc;

    if (! cdev) {
        pr_err("bad CDP device");
        return 0;
    }

    seq_printf(f, "\nw0: 0x%08X\n", desc->word0.register_value);
    seq_printf(f, "    next_frame_dba_msb   = 0x%08X\n", desc->word0.bitfield.next_frame_dba_msb);

    seq_printf(f, "\nw1: 0x%08X\n", desc->word1.register_value);
    seq_printf(f, "    next_frame_dba_lsb   = 0x%08X\n", desc->word1.bitfield.next_frame_dba_lsb);

    seq_printf(f, "\nw2: 0x%08X\n", desc->word2.register_value);
    seq_printf(f, "    next_frame_rs       = %d\n", desc->word2.bitfield.next_frame_rs);
    seq_printf(f, "    next_frame_shift_r  = %d\n", desc->word2.bitfield.next_frame_shift_r);
    seq_printf(f, "    next_frame_shift_l  = %d\n", desc->word2.bitfield.next_frame_shift_l);
    seq_printf(f, "    next_frame_ibpp     = %d\n", desc->word2.bitfield.next_frame_ibpp);
    seq_printf(f, "    next_frame_en       = %d\n", desc->word2.bitfield.next_frame_en);
    seq_printf(f, "    next_frame_dpck     = %d\n", desc->word2.bitfield.next_frame_dpck);

    seq_printf(f, "\nw3: 0x%08X\n", desc->word3.register_value);
    seq_printf(f, "    next_frame_rgap     = %d\n", desc->word3.bitfield.next_frame_rgap);

    seq_printf(f, "\nw4: 0x%08X\n", desc->word4.register_value);
    seq_printf(f, "    cand_cnt_dba_msb    = 0x%08X\n", desc->word4.bitfield.cand_cnt_dba_msb);

    seq_printf(f, "\nw5: 0x%08X\n", desc->word5.register_value);
    seq_printf(f, "    cand_cnt_dba_lsb    = 0x%08X\n", desc->word5.bitfield.cand_cnt_dba_lsb);

    return 0;
}


int dump_stage_desc(struct seq_file *f, cdp_stage_desc_t *stage_desc)
{
    seq_printf(f, "\nw0: 0x%08X\n", stage_desc->word0.register_value);
    seq_printf(f, "    isp_bypass_en   = %d\n", stage_desc->word0.bitfield.isp_bypass_en);
    seq_printf(f, "    isp_bypass      = %d\n", stage_desc->word0.bitfield.isp_bypass);
    seq_printf(f, "    isp_pyr_en      = %d\n", stage_desc->word0.bitfield.isp_pyr_en);
    seq_printf(f, "    alg_bypass_en   = %d\n", stage_desc->word0.bitfield.alg_bypass_en);
    seq_printf(f, "    alg_bypass      = %d\n", stage_desc->word0.bitfield.alg_bypass);
    seq_printf(f, "    alg_pyr_en      = %d\n", stage_desc->word0.bitfield.alg_pyr_en);
    seq_printf(f, "    nxt_bypass_en   = %d\n", stage_desc->word0.bitfield.nxt_bypass_en);
    seq_printf(f, "    nxt_bypass      = %d\n", stage_desc->word0.bitfield.nxt_bypass);
    seq_printf(f, "    nxt_pyr_en      = %d\n", stage_desc->word0.bitfield.nxt_pyr_en);
    seq_printf(f, "    nms_vld_en      = %d\n", stage_desc->word0.bitfield.nms_vld_en);
    seq_printf(f, "    fast            = %d\n", stage_desc->word0.bitfield.fast);
    seq_printf(f, "    camera_rotation = %d\n", stage_desc->word0.bitfield.camera_rotation);
    seq_printf(f, "    gauss_en        = %d\n", stage_desc->word0.bitfield.gauss_en);
    seq_printf(f, "    fast_th_mem_sel = %d\n", stage_desc->word0.bitfield.fast_th_mem_sel);
    seq_printf(f, "    fast_no_mem     = %d\n", stage_desc->word0.bitfield.fast_no_mem);
    seq_printf(f, "    frame_dpck      = %d\n", stage_desc->word0.bitfield.frame_dpck);
    seq_printf(f, "    frame_frot      = %d\n", stage_desc->word0.bitfield.frame_frot);
    seq_printf(f, "    frame_chmod     = %d\n", stage_desc->word0.bitfield.frame_chmod);
    seq_printf(f, "    frame_obpp      = %d\n", stage_desc->word0.bitfield.frame_obpp);
    seq_printf(f, "    data_shift_r    = %d\n", stage_desc->word0.bitfield.data_shift_r);
    seq_printf(f, "    data_shift_l    = %d\n", stage_desc->word0.bitfield.data_shift_l);

    seq_printf(f, "\nw1: 0x%08X\n", stage_desc->word1.register_value);
    seq_printf(f, "    in_image_num_of_lines  = %d\n", stage_desc->word1.bitfield.in_image_num_of_lines);
    seq_printf(f, "    in_image_num_of_pixels = %d\n", stage_desc->word1.bitfield.in_image_num_of_pixels);

    seq_printf(f, "\nw2: 0x%08X\n", stage_desc->word2.register_value);
    seq_printf(f, "    out_image_num_of_lines  = %d\n", stage_desc->word2.bitfield.out_image_num_of_lines);
    seq_printf(f, "    out_image_num_of_pixels = %d\n", stage_desc->word2.bitfield.out_image_num_of_pixels);
    seq_printf(f, "    resize_sel              = %d\n", stage_desc->word2.bitfield.resize_sel);

    seq_printf(f, "\nw3: 0x%08X\n", stage_desc->word3.register_value);
    seq_printf(f, "    gauss_coeff_bbb         = %d\n", stage_desc->word3.bitfield.gauss_coeff_bbb);
    seq_printf(f, "    gauss_coeff_center      = %d\n", stage_desc->word3.bitfield.gauss_coeff_center);

    seq_printf(f, "\nw4: 0x%08X\n", stage_desc->word4.register_value);
    seq_printf(f, "    gauss_coeff_cross_in    = %d\n", stage_desc->word4.bitfield.gauss_coeff_cross_in);
    seq_printf(f, "    gauss_coeff_cross_out   = %d\n", stage_desc->word4.bitfield.gauss_coeff_cross_out);

    seq_printf(f, "\nw5: 0x%08X\n", stage_desc->word5.register_value);
    seq_printf(f, "    gauss_coeff_corner_in   = %d\n", stage_desc->word5.bitfield.gauss_coeff_corner_in);
    seq_printf(f, "    gauss_coeff_corner_out  = %d\n", stage_desc->word5.bitfield.gauss_coeff_corner_out);
    seq_printf(f, "    gauss_shiftback         = %d\n", stage_desc->word5.bitfield.gauss_shiftback);

    seq_printf(f, "\nw6: 0x%08X\n", stage_desc->word6.register_value);
    seq_printf(f, "    fast_thold_h_win_size   = %d\n", stage_desc->word6.bitfield.fast_thold_h_win_size);
    seq_printf(f, "    fast_thold_v_win_size   = %d\n", stage_desc->word6.bitfield.fast_thold_v_win_size);
    seq_printf(f, "    fast_seqln              = %d\n", stage_desc->word6.bitfield.fast_seqln);

    seq_printf(f, "\nw7: 0x%08X\n", stage_desc->word7.register_value);
    seq_printf(f, "    cand_h_win_size         = %d\n", stage_desc->word7.bitfield.cand_h_win_size);
    seq_printf(f, "    cand_v_win_size         = %d\n", stage_desc->word7.bitfield.cand_v_win_size);

    seq_printf(f, "\nw8: 0x%08X\n", stage_desc->word8.register_value);
    seq_printf(f, "    cand_max_per_win        = %d\n", stage_desc->word8.bitfield.cand_max_per_win);
    seq_printf(f, "    roi_remove_pix_width    = %d\n", stage_desc->word8.bitfield.roi_remove_pix_width);
    seq_printf(f, "    roi_en                  = %d\n", stage_desc->word8.bitfield.roi_en);

    seq_printf(f, "\nw9: 0x%08X\n", stage_desc->word9.register_value);
    seq_printf(f, "    roi_cir_r               = %d\n", stage_desc->word9.bitfield.roi_cir_r);
    seq_printf(f, "    roi_cir_en              = %d\n", stage_desc->word9.bitfield.roi_cir_en);

    seq_printf(f, "\nw10: 0x%08X\n", stage_desc->word10.register_value);
    seq_printf(f, "    roi_cir_cxcx            = %d\n", stage_desc->word10.bitfield.roi_cir_cxcx);

    seq_printf(f, "\nw11: 0x%08X\n", stage_desc->word11.register_value);
    seq_printf(f, "    roi_cir_cycy            = %d\n", stage_desc->word11.bitfield.roi_cir_cycy);

    seq_printf(f, "\nw12: 0x%08X\n", stage_desc->word12.register_value);
    seq_printf(f, "    roi_cir_cx              = %d\n", stage_desc->word12.bitfield.roi_cir_cx);
    seq_printf(f, "    roi_cir_cy              = %d\n", stage_desc->word12.bitfield.roi_cir_cy);

    seq_printf(f, "\nw13: 0x%08X\n", stage_desc->word13.register_value);
    seq_printf(f, "    fast_thold_no_mem       = %d\n", stage_desc->word13.bitfield.fast_thold_no_mem);

    seq_printf(f, "\nw14: 0x%08X\n", stage_desc->word14.register_value);
    seq_printf(f, "    frame_dba_msb           = 0x%08X\n", stage_desc->word14.bitfield.frame_dba_msb);

    seq_printf(f, "\nw15: 0x%08X\n", stage_desc->word15.register_value);
    seq_printf(f, "    frame_dba_lsb           = 0x%08X\n", stage_desc->word15.bitfield.frame_dba_lsb);

    seq_printf(f, "\nw16: 0x%08X\n", stage_desc->word16.register_value);
    seq_printf(f, "    cand_dba_msb            = 0x%08X\n", stage_desc->word16.bitfield.cand_dba_msb);

    seq_printf(f, "\nw17: 0x%08X\n", stage_desc->word17.register_value);
    seq_printf(f, "    cand_dba_lsb            = 0x%08X\n", stage_desc->word17.bitfield.cand_dba_lsb);

    return 0;
}


static int cdp_glob_desc_open(struct inode *inode, struct file *file)
{
    return single_open(file, cdp_glob_desc_show, inode->i_private);
}


static const struct file_operations cdp_glob_desc_fops = {
    .owner		= THIS_MODULE,
    .open		= cdp_glob_desc_open,
    .read		= seq_read,
    .llseek		= seq_lseek,
    .release	= single_release,
};
#endif // CONFIG_DEBUG_FS


static int alloc_out_buf(struct device *dev, uint64_t shname, uint32_t size, void **kaddr, uint64_t *io_addr)
{
    struct shregion_metadata meta;
    ShregionCdpDcprsIOaddr ioaddr;
    int s;

    memset(&meta, 0, sizeof(meta));

    meta.name       = shname;
    meta.type       = SHREGION_DDR_PRIVATE_CONTIG;
    meta.size_bytes = size;
    meta.flags      = SHREGION_PERSISTENT | SHREGION_CDP_AND_DCPRS_SHAREABLE | CDP_AND_DCPRS_MMU_MAP_PERSIST ;

    if ((s = kalloc_shregion(&meta, kaddr)) != 0) {
        pr_err("kalloc_shregion error: %d\n", s);
        return -ENOMEM;
    }
    get_shregion_cdp_dcprs_ioaddr(shname, &ioaddr);
    *io_addr = ioaddr;
#ifdef USE_ION
    struct dma_buf *dbuf;
    struct sg_table *sg;
    int fd;

#if 0
    *kaddr = dmam_alloc_coherent(dev, size, io_addr, GFP_KERNEL);
    if (! *kaddr) {
        pr_err("%s: Failed to allocate buf_g of size %d", __func__, size);
        return -ENOMEM;
    }
#endif
    fd = ion_alloc(size, 1 << ION_HEAP_CVIP_HYDRA_FMR2_ID, 0);
    if (! fd) {
        pr_err("%s: failed to allocate ion buffer: %d\n", __func__, fd);
        return -ENOMEM;
    }

    dbuf = dma_buf_get(fd);
    if (IS_ERR(dbuf)) {
        pr_err("%s: failed to get dma buffer\n", __func__);
        return -EFAULT;
    }

    sg = ion_sg_table(dbuf);
    if (IS_ERR(sg)) {
        pr_err("failed to get sg table\n");
        return -1;
    }
    WARN_ON(sg->nents > 1);

    if (kaddr) {
        *kaddr = dma_buf_kmap(dbuf, 0);
        memset(*kaddr, 0, size);
    }
    *io_addr = page_to_phys(sg_page(sg->sgl));
#endif

    return 0;
}


static int cdp_probe(struct platform_device *pdev)
{
    int ret, irq, irq_count, i;
    struct cdp_dev *ddev = &g_dev;

    // Get device address base.
    if ((ddev->res = platform_get_resource(pdev, IORESOURCE_MEM, 0))) {
        DPRINT("CDP io base is at 0x%016llx, ends at 0x%016llx", ddev->res->start, ddev->res->end);
    } else {
        pr_err("can't get CDP base address");
        return -1;
    }

    if ((irq_count = platform_irq_count(pdev)) < 1) {
        pr_err("can't get irq count");
        return -1;
    }

    // Get our interrupt lines.
    DPRINT("Total irq count is %d", irq_count);
    for (i = 0; i < irq_count; i++) {
        if ((irq = platform_get_irq(pdev, i))) {
            if ((ret = devm_request_irq(&pdev->dev, irq, cdp_intr, 0,  dev_name(&pdev->dev), ddev))) {
                pr_err("error in request_irq: %d\n", ret);
                return -1;
            }
            DPRINT("alloc irq #%d: %d\n", i, irq);
            if (! i) {
                ddev->irq_first = irq;
            }
        } else {
            pr_err("can't get CDP irq");
            return -1;
        }
    }
    pr_info("First acquired irq is %d\n", ddev->irq_first);

    // Create device.
    if (cdp_mkdev(ddev)) {
        pr_err("couldn't create device");
        return -1;
    }
    ddev->dev = &pdev->dev;

    // Map in device.
    ddev->regs = devm_ioremap_resource(ddev->dev, ddev->res);
    cdp = (cdp_t *)ddev->regs;
    DPRINT("Mapped in device, ddev->dev is %px, regs vaddr=%px", ddev->dev, ddev->regs);
    if (IS_ERR(ddev->regs)) {
        pr_err("failed to map device");
        return PTR_ERR(ddev->regs);
    }

#ifdef CONFIG_DEBUG_FS
    ddev->dir = debugfs_create_dir(dev_name(&pdev->dev), g_dir);
    if (IS_ERR(ddev->dir)) {
        pr_warn("Failed to create debugfs: %ld\n",
                PTR_ERR(ddev->dir));
        ddev->dir = NULL;
    }

    cdp_debugfs_init(ddev->dir, &ddev->err);

    debugfs_create_u32("norun",      0600, ddev->dir, &g_dev.norun);
    debugfs_create_u32("dbg",        0600, ddev->dir, &g_dbg_mask);
    debugfs_create_file("glob_desc", 0400, ddev->dir, ddev, &cdp_glob_desc_fops);
#endif

#ifdef CONFIG_DEBUG_FS
    g_dev.blob_sum.size = OUTBUF_SIZE;
    debugfs_create_blob("fast_summary", S_IFREG | 0400, ddev->dir, &g_dev.blob_sum);
#endif
    platform_set_drvdata(pdev, &g_dev);

    init_waitqueue_head(&ddev->ev_waitq);

    // Zero out the threshold memory.
    for (i = 0; i < FAST_MAX_WINS; i++) {
        cdp->eng.cdp_rgf.cdp_s0_fast_thold_mem[i].register_value = 0;
        cdp->eng.cdp_rgf.cdp_s1_fast_thold_mem[i].register_value = 0;
        cdp->eng.cdp_rgf.cdp_s2_fast_thold_mem[i].register_value = 0;
    }

    DPRINT("CDP device initialized\n");

    cdp_lib_init(ddev->regs);

    // Compute info for mmaping threshold memories.
    ddev->mmap_threshold_offset = (uint64_t)cdp->eng.cdp_rgf.cdp_s0_fast_thold_mem - (uint64_t)cdp;
    ddev->mmap_threshold_size   = PAGE_SIZE * 4;

    return 0;
}


static int cdp_remove(struct platform_device *pdev)
{
    return 0;
}


static const struct of_device_id cdp_of_match[] = {
    { .compatible = "amd,cdp", },
    { },
};

static struct platform_driver g_drv = {
    .probe  = cdp_probe,
    .remove = cdp_remove,
    .driver = {
        .name = "cdp",
        .of_match_table = cdp_of_match,
    },
};


static int __init cdp_init(void)
{
    int ret;
    dev_t devt;

    /* Register the device class */
    g_class = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(g_class)) {
        pr_err("cdp: Failed to register device class.\n");
        return PTR_ERR(g_class);
    }

    ret = alloc_chrdev_region(&devt, 0, CDEV_NUM, "cdp");
    if (ret) {
        class_destroy(g_class);
        pr_err("cdp: failed to alloc_chrdev_region\n");
        return -EBUSY;
    }
    g_major = MAJOR(devt);

#ifdef CONFIG_DEBUG_FS
    g_dir = debugfs_create_dir("cdp", NULL);
    if (! g_dir) {
        unregister_chrdev_region(devt, CDEV_NUM);
        class_destroy(g_class);
        return -ENODEV;
    }
#endif

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


static void __exit cdp_exit(void)
{
    platform_driver_unregister(&g_drv);
#ifdef CONFIG_DEBUG_FS
    debugfs_remove_recursive(g_dir);
#endif
    unregister_chrdev_region(g_major, CDEV_NUM);
    class_destroy(g_class);
}


void writelX(uint32_t value, volatile void *addr)
{
    if (g_reg_dump) {
        pr_info("wr 0x%08x << %08x\n", (uint32_t)((uint64_t)addr - (uint64_t)cdp), value);
    }
    writel(value, addr);
}

module_init(cdp_init);
module_exit(cdp_exit);

MODULE_DESCRIPTION("CVIP CDP Driver");

