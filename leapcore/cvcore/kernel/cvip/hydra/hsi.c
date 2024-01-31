// SPDX-License-Identifier: GPL-2.0-only
//
// Copyright (C) 2022 Magic Leap, Inc. All rights reserved.
//

// pr_fmt will insert our module name into the pr_info() macro.
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/spinlock.h>
#include <linux/console.h>
#include <linux/pci.h>
#include <linux/workqueue.h>
#include <linux/slab.h>
#include <linux/ioctl.h>
#include <linux/interrupt.h>
#include <linux/sched.h>
#include <linux/mm.h>
#include <linux/delay.h>
#include <linux/poll.h>
#include <linux/timekeeping.h>
#include <linux/tty.h>
#include <linux/tty_driver.h>
#include <linux/uaccess.h>
#include <linux/version.h>
#include <linux/firmware.h>
#include <linux/circ_buf.h>
#include <linux/dma-buf.h>
#include <linux/dma-mapping.h>
#include <linux/debugfs.h>
#include <linux/hashtable.h>

#ifdef __aarch64__
#include "linux/pci_cvip.h"
#include "ion/ion.h"
#include "uapi/mero_ion.h"
#endif
#include "shmem_map.h"
#include "sram_map.h"
#include "hsi_ioctl.h"
#include "hsi_priv.h"
#include "vcam.h"
#include "hydra.h"

#define MAX_FLUSH_WAIT_US   (100)


// NWL status.
#define NWL_XSTAT_COMPL     (0x00000001)
#define NWL_XSTAT_SRC_ERR   (0x00000002)
#define NWL_XSTAT_DST_ERR   (0x00000004)
#define NWL_XSTAT_INT_ERR   (0x00000008)
#define NWL_XSTAT_ANY_ERR   (NWL_XSTAT_SRC_ERR | NWL_XSTAT_DST_ERR | NWL_XSTAT_INT_ERR)


#define CREATE_TRACE_POINTS
struct tr_args_intr {
    uint32_t hix, vix, dix, ssix, bufix, tail, dlen, slice, xstat, uhand, fcnt, pass;
    uint16_t uid, uidr;
    unsigned long cpu, count;
    uint64_t ref;
    const char *dev_name;
};
#include "hsi_trace.h"

#if LINUX_VERSION_CODE < KERNEL_VERSION(5,3,0)
#define ktime_get_boottime_ns ktime_get_boot_ns
#endif

#define CLASS_NAME          ("hsi")

#define HSI_MAX_DEVS        (HSI_MAX_STREAMS * HSI_MAX_VSTREAMS)
#define HSI_MAX_BUFS        (8 * HSI_MAX_STREAMS) // max number of buffers that can be submitted per hydra
#define HSI_MAX_INSTANCES   (2)                   // max number of hsi instances == max number of hydras
#define HSI_MAX_IRQ         (1024 * 4)            // max number of HSIs
#define HSI_MAX_EVENTS      (16)                  // max number of HSI user events per device

#define HSI_UID_XSPILLWAY   (0xdead)              // associated with dropped frames
#define SPILLWAY_SIZE       (1024 * 1024 * 8)

#define hioread16(x)      ioread16((void *) x)
#define hiowrite16(x, y)  iowrite16(x, (void *)y)
#define hioread32(x)      ioread32((void *) x)
#define hiowrite32(x, y)  iowrite32(x, (void *)y)

extern struct dma_buf *get_dma_buf_for_shr(uint64_t shr_name);        // from shregions driver
extern int get_shregion_cdp_dcprs_ioaddr(uint64_t shr_name, uint64_t *shr_cdp_dcprs_vaddr);
extern int shregion_arm_cache_invalidate_range(uint64_t shr_name, struct dma_buf *dmabuf, unsigned long offset, size_t size);
extern int hydra_irq_req(int hix, int chan_id, char *name);
extern int cvip_pci_flush_dram_try(int tries);


struct h_node {
    uint64_t          sh_name;
    phys_addr_t       sh_paddr_io;
    phys_addr_t       sh_paddr;
    void              *sh_vaddr;
    struct dma_buf    *dma_buf;
    struct hlist_node node;
};
DECLARE_HASHTABLE(g_shregion_tbl, 5);

static int      g_vcam_slice_size_kb = 0;
module_param_named(vcam_slice_size_kb,  g_vcam_slice_size_kb, int, 0444);

static int      g_vcam_bpp = 10;
module_param_named(vcam_bpp,  g_vcam_bpp, int, 0444);

static int hsi_open(struct inode *, struct file *);
static int hsi_release(struct inode *, struct file *);
static long hsi_ioctl(struct file *, unsigned int, unsigned long);
static unsigned int hsi_poll(struct file *pFile, poll_table *wait);

static uint32_t g_dfs_dcprs_img_save_count = 0;

enum hsi_stream_state { HEADER, DATA, FOOTER };

// snapshot
static ssize_t snapshot_store(struct device *dev,
                              struct device_attribute *attr, const char *buf, size_t count);
static DEVICE_ATTR_WO(snapshot);


// stats
static ssize_t stats_show(struct device *dev,
                          struct device_attribute *attr, char *buf);
static DEVICE_ATTR_RO(stats);


// stats
static ssize_t irq_show(struct device *dev,
                        struct device_attribute *attr, char *buf);
static DEVICE_ATTR_RO(irq);


//vcam_bpp
static ssize_t bpp_store(struct device *dev,
                         struct device_attribute *attr, const char *buf, size_t count);
static ssize_t bpp_show(struct device *dev,
                        struct device_attribute *attr, char *buf);
static DEVICE_ATTR_RW(bpp);

//ignore
static ssize_t ignore_store(struct device *dev,
                            struct device_attribute *attr, const char *buf, size_t count);
static ssize_t ignore_show(struct device *dev,
                           struct device_attribute *attr, char *buf);
static DEVICE_ATTR_RW(ignore);

static struct attribute *hsi_attributes[] = {
    &dev_attr_snapshot.attr,
    &dev_attr_ignore.attr,
    &dev_attr_stats.attr,
    &dev_attr_irq.attr,
    NULL
};

static const struct attribute_group hsi_attr_group = {
    .attrs = hsi_attributes,
};


// streamtab attr
static ssize_t streamtab_show(struct device *dev,
                              struct device_attribute *attr, char *buf);
static DEVICE_ATTR_RO(streamtab);


// hsitab attr
static ssize_t hsitab_show(struct device *dev,
                           struct device_attribute *attr, char *buf);
static DEVICE_ATTR_RO(hsitab);

static ssize_t hsilog_read(struct file *filp, struct kobject *kobj,
                           struct bin_attribute *attr,
                           char *buf, loff_t off, size_t count);


static ssize_t dfs_hsi_probe_write(struct file *file, const char __user *data, size_t count, loff_t *ppos);
static const struct file_operations dfs_hsi_probe_fops = {
    .open   = simple_open,
    .write  = dfs_hsi_probe_write,
    .llseek = generic_file_llseek,
};


static int dfs_hsi_active_devs_open(struct inode *inode, struct file *file);
static const struct file_operations dfs_hsi_active_devs_fops = {
    .owner   = THIS_MODULE,
    .open    = dfs_hsi_active_devs_open,
    .read    = seq_read,
    .llseek  = seq_lseek,
    .release = single_release,
};

static struct file_operations const fops_hsi = {
    .owner          = THIS_MODULE,
    .open           = hsi_open,
    .release        = hsi_release,
    .unlocked_ioctl = hsi_ioctl,
    .compat_ioctl   = hsi_ioctl,
    .poll           = hsi_poll,
};


static struct hydra_dev {
    struct hsi_root        *hsi_root;

    dev_t                   maj_num;

    struct pci_dev          *pdev;
    uint32_t                next_dix;

    struct {
        uint32_t            ints_per_sec;
        uint32_t            ints;
        struct {
            uint32_t            irq_count_tot;
            uint32_t            active[HSI_MAX_DEVS];
            uint32_t            irq_count[HSI_MAX_DEVS];
        } clr_on_tick;
    } dfs;

    struct hsi_dev {
        struct device           *dev;
        struct cdev             cdev;
        dev_t                   devno;
        int                     is_virtual;
        int                     is_vcam;
        uint32_t                spills_new;
        uint32_t                hix;
        uint32_t                dix;
        uint32_t                six;
        uint32_t                vix;
        uint32_t                sub_count;
        uint32_t                data_buf_len;
        uint32_t                meta_length;
        uint32_t                opened;
        uint32_t                closed;
        uint32_t                opened_by_pid;
        char                    opened_by_name[TASK_COMM_LEN + 1];
        struct mutex            dev_lock;
        struct list_head        rdq;
        spinlock_t              pendq_lock;
        spinlock_t              rdq_lock;
        wait_queue_head_t       rdq_waitq;

        enum hsi_stream_state   nxt_str_state0;
        enum hsi_stream_state   nxt_str_state1; // used only for vcam's 2nd substream

        // For tracking frame counters.
        uint32_t                frame_counter_last;
        uint32_t                frame_counter_valid;

        uint32_t                irq;
        uint32_t                irq_per_sec;

        uint32_t                snapshot_count;
        uint32_t                snapshot_last;
        uint32_t                ignore;

        // Per-device spillway address.
        void                    *spillway;
        uint64_t                spillway_io;

        struct bin_attribute    attr_hsi_log;

        char                    name[64];

        // Stats
        struct {
            uint32_t ints;
            uint32_t empty_queue;
            uint32_t frames;
            uint32_t frames_dropped;
            uint32_t frames_orphaned;
        } stats;

        // Define a substream. Each substream is associate with its own dma channel.
        struct hsi_substream {
            volatile struct substream    *ss;                    // table entry in hydra memory
            uint32_t                     slice_size;
            uint32_t                     irq;                    // msi (non zero-based)
            spinlock_t                   irq_lock;

        }    sub[HSI_MAX_SUBSTREAMS];

        // HSI user events.
        struct hsi_event {
            struct hsi_dev *dev;
            struct vcam_buf *vp;
            struct h_node  *hnp_data;
            struct h_node  *hnp_meta;
            void           *data;
            uint64_t       submit_ts;
            uint64_t       intr_ts;
            uint64_t       wake_ts;
            uint64_t       ref;
            uint64_t       irq;
            uint32_t       evix;
            uint32_t       dcprs_err;
            uint32_t       length;
            uint32_t       slice;
            uint32_t       flags;
            uint32_t       xstat;
            uint32_t       spills;
            uint32_t       ss_data_length[HSI_MAX_SUBSTREAMS];
            uint32_t       ss_meta_length[HSI_MAX_SUBSTREAMS];
            struct list_head list;
        } hsi_event_tab[HSI_MAX_EVENTS];
        spinlock_t hsi_event_tab_lock;
        struct list_head hsi_event_free_list;

    }    devs[HSI_MAX_DEVS];

    // Keeps track of which hsi device and substream are associated with a given interrupt.
    struct {
        uint32_t hsi_dev_ix;
        uint32_t hsi_ss_ix;
    }   irq_tab[HSI_MAX_IRQ];

    // A table of hsi buffers for all this hydra's streams.
    // As hydra fills up buffers, the table elements are strung together onto a list
    // that the ioctl(WAIT) function pulls them from and hands them back to user space.
    struct hsi_buf {
        spinlock_t          lock;

        struct dma_buf            *dma_dst_buf_data;
        struct dma_buf            *dma_dst_buf_meta;
        struct dma_buf_attachment *dma_dst_attach;
        struct sg_table           *sg_tbl_dst;
        void                      *vaddr_dst_data;
        void                      *vaddr_dst_meta;
        dma_addr_t                sh_base_pa_io_data;
        dma_addr_t                sh_base_pa_io_meta;
        dma_addr_t                sh_base_pa_data;
        dma_addr_t                sh_base_pa_meta;
        struct h_node             *hnp_data;
        struct h_node             *hnp_meta;

        uint32_t            magic;
#define HSIBUF_MAGIC        (0xdeadcafe)
        uint64_t            submit_ts;
        uint32_t            hix;
        uint64_t            ref;
        uint32_t            hsibuf_ix;
        uint32_t            data_len;
        uint32_t            dcprs_err;
        uint32_t            data_buf_len;
        uint32_t            meta_data_len;
        uint32_t            meta_data_buf_len;
        uint32_t            size_meta_header;
        uint32_t            size_meta_footer;
        uint32_t            ss_data_size;
        uint32_t            ss_meta_size;
        atomic_t            ss_data_count;
        atomic_t            ss_head_count;
        atomic_t            ss_foot_count;
        uint32_t            ss_data_length[HSI_MAX_SUBSTREAMS];
        uint32_t            ss_meta_length[HSI_MAX_SUBSTREAMS];
        void               *data;
        void               *meta_data;
        dma_addr_t          data_io;
        dma_addr_t          meta_io;
        uint32_t            active;
        struct vcam_buf     *vp;
        struct hsi_dev      *dev;
        struct list_head    list;
    } buf_tab[HSI_MAX_BUFS];
    spinlock_t       hsi_buf_tab_lock;
    struct list_head hsi_buf_free_list;

    dev_t          hsi_maj_num;

    struct device *sysdev;
    void          *hydradev;

    struct dentry *ddir;

} g_hsi[HSI_MAX_INSTANCES];

void      *g_spillway;
uint64_t  g_spillway_io;

extern uint32_t g_stand_down[2];

struct class *g_hsi_class;

static dev_t g_devs[HSI_MAX_INSTANCES];

#if 0
static void shregion_sync(struct hsi_buf *hsibuf);
#endif

static enum hsi_stream_state next_state(struct hsi_dev *dev, enum hsi_stream_state cur)
{
    if (cur == HEADER) {
        return DATA;
    }
    if (cur == FOOTER) {
        return HEADER;
    }
    // cur has to be DATA
    return (dev->meta_length) ? FOOTER : DATA;
}


static enum hsi_stream_state current_state(uint16_t uid)
{
    if (uid & HSI_UID_HEADER) {
        return HEADER;
    }
    if (uid & HSI_UID_FOOTER) {
        return FOOTER;
    }
    return DATA;
}


//--------------------------------------------------------------------------
// hsi_devnode()  Called by udev subsytem when device is created.
//--------------------------------------------------------------------------
static char *hsi_devnode(struct device *dev, umode_t *mode)
{
    if (mode) {
        *mode = 0666;
    }

    return kasprintf(GFP_KERNEL, "hsi/%s", dev_name(dev));
}


//--------------------------------------------------------------------------
// hsi_buf_pool_init()  Initialize a device's pool of HSI buffers.
//--------------------------------------------------------------------------
static void hsi_buf_pool_init(uint32_t hix)
{
    int i;

    INIT_LIST_HEAD(&g_hsi[hix].hsi_buf_free_list);

    spin_lock_init(&g_hsi[hix].hsi_buf_tab_lock);

    for (i = 0; i < HSI_MAX_BUFS; i++) {
        spin_lock_init(&g_hsi[hix].buf_tab[i].lock);
        INIT_LIST_HEAD(&g_hsi[hix].buf_tab[i].list);
        g_hsi[hix].buf_tab[i].hsibuf_ix = i;
        g_hsi[hix].buf_tab[i].magic = HSIBUF_MAGIC;
        list_add_tail(&g_hsi[hix].buf_tab[i].list, &g_hsi[hix].hsi_buf_free_list);
    }
}


//--------------------------------------------------------------------------
// hsi_buf_alloc()  Allocate an HSI buffer.
//--------------------------------------------------------------------------
static struct hsi_buf *hsi_buf_alloc(struct hsi_dev *dev, uint64_t ref)
{
    struct hsi_buf *buf;
    unsigned long flags;
    uint32_t hix = dev->hix;

    buf = 0;
    spin_lock_irqsave(&g_hsi[hix].hsi_buf_tab_lock, flags);
    if (!list_empty(&g_hsi[hix].hsi_buf_free_list)) {
        buf = list_first_entry(&g_hsi[hix].hsi_buf_free_list, struct hsi_buf, list);
        list_del_init(&buf->list);
    } else {
        pr_err("no hsi_bufs\n");
        spin_unlock_irqrestore(&g_hsi[hix].hsi_buf_tab_lock, flags);
        return 0;
    }

    if (buf->magic != HSIBUF_MAGIC) {
        pr_err("bad hsibuf magic (0x%08x)!\n", buf->magic);
        spin_unlock_irqrestore(&g_hsi[hix].hsi_buf_tab_lock, flags);
        return 0;
    }

    if (buf->active) {
        pr_err("Allocated an active hsi buf!\n");
        spin_unlock_irqrestore(&g_hsi[hix].hsi_buf_tab_lock, flags);
        return 0;
    }

    buf->active            = 1;
    buf->dev               = dev;
    buf->hix               = hix;
    buf->ref               = ref;

    buf->meta_io           = 0;
    buf->data_io           = 0;
    buf->dcprs_err         = 0;
    buf->hnp_data          = 0;
    buf->hnp_meta          = 0;

    atomic_set(&buf->ss_data_count, 0);
    atomic_set(&buf->ss_head_count, 0);
    atomic_set(&buf->ss_foot_count, 0);
    buf->ss_data_length[0] = 0;
    buf->ss_data_length[1] = 0;
    buf->ss_meta_length[0] = 0;
    buf->ss_meta_length[1] = 0;
    if (dev->sub_count > 1) {
        // Implicitly assuming that all substreams have the same data/meta sizes.
        buf->ss_data_size = dev->sub[0].ss->buf_size;
        buf->ss_meta_size = dev->sub[0].ss->meta_size;
    }

    spin_unlock_irqrestore(&g_hsi[hix].hsi_buf_tab_lock, flags);

    trace_ml_hsi_hsibuf_alloc(buf, buf->hix, buf->hsibuf_ix, buf->active, dev_name(dev->dev), ref);

    return buf;
}


//--------------------------------------------------------------------------
// hsi_buf_free()  Free an HSI buffer.
//--------------------------------------------------------------------------
static void hsi_buf_free(struct hsi_buf *buf, uint32_t cxt)
{
    unsigned long flags;
    uint32_t hix = buf->hix;

    trace_ml_hsi_hsibuf_free(buf, buf->hix, buf->hsibuf_ix, buf->active, dev_name(buf->dev->dev), buf->ref, buf->vp, cxt);
    DPRINT(5, "hsi_buf_free:: hix=%d, buf=%px, bufix=%d, active=%d, cxt=%d\n", buf->hix, buf, buf->hsibuf_ix, buf->active, cxt);

    spin_lock_irqsave(&g_hsi[hix].hsi_buf_tab_lock, flags);

    if (! buf->active) {
        pr_warn("Freeing free hsi buf at %p, hix=%d, bix=%d", buf, hix, buf->hsibuf_ix);
    }
    buf->active = 0;
    buf->dev    = 0;

    // Unmap PCI buffers.
    if (buf->meta_io) {
        //pr_info("unmap meta at 0x%llx, size=%d\n", buf->meta_io, buf->meta_data_buf_len);
#ifndef __aarch64__
        pci_unmap_single(g_hsi[hix].pdev, buf->meta_io, buf->meta_data_buf_len, PCI_DMA_FROMDEVICE);
#endif
    }
    if (buf->data_io) {
        //pr_info("unmap data at 0x%llx, size=%d\n", buf->data_io, buf->data_buf_len);
#ifndef __aarch64__
        pci_unmap_single(g_hsi[hix].pdev, buf->data_io, buf->data_buf_len, PCI_DMA_FROMDEVICE);
#endif
    }
    buf->meta_io = 0;
    buf->data_io = 0;

    list_add_tail(&buf->list, &g_hsi[hix].hsi_buf_free_list);
    spin_unlock_irqrestore(&g_hsi[hix].hsi_buf_tab_lock, flags);
}


//--------------------------------------------------------------------------
// hsi_buf_pool_init()  Release all the hsi_buf's associated with a device.
//--------------------------------------------------------------------------
static void hsi_buf_dev_release(struct hsi_dev *dev)
{
    uint32_t hix = dev->hix;
    int i;

    pr_info("Release all hsi_bufs for dev=%px, hix=%d, dix=%d (%s)\n", dev, dev->hix, dev->dix, dev_name(dev->dev));

    for (i = 0; i < HSI_MAX_BUFS; i++) {
        if (g_hsi[hix].buf_tab[i].dev == dev) {
            pr_info("Release hix=%d, bix=%d\n", hix, i);
            hsi_buf_free(&g_hsi[hix].buf_tab[i], 100+i);
        }
    }
}


//--------------------------------------------------------------------------
// hsi_ev_pool_init()  Initialize a device's pool of HSI events.
//--------------------------------------------------------------------------
static void hsi_ev_pool_init(uint32_t hix)
{
    int devnum, i;

    for (devnum = 0; devnum < HSI_MAX_DEVS; devnum++) {
        spin_lock_init(&g_hsi[hix].devs[devnum].hsi_event_tab_lock);
        INIT_LIST_HEAD(&g_hsi[hix].devs[devnum].hsi_event_free_list);

        for (i = 0; i < HSI_MAX_EVENTS; i++) {
            INIT_LIST_HEAD(&g_hsi[hix].devs[devnum].hsi_event_tab[i].list);
            list_add_tail(&g_hsi[hix].devs[devnum].hsi_event_tab[i].list, &g_hsi[hix].devs[devnum].hsi_event_free_list);
            g_hsi[hix].devs[devnum].hsi_event_tab[i].dev = &g_hsi[hix].devs[devnum];
            g_hsi[hix].devs[devnum].hsi_event_tab[i].evix = i;
        }
    }

}


//--------------------------------------------------------------------------
// hsi_ev_alloc()  Allocate an HSI event.
//--------------------------------------------------------------------------
static struct hsi_event *hsi_ev_alloc(struct hsi_dev *dev, uint16_t uid, uint64_t ref, uint32_t data_length, uint32_t xstat)
{
    struct hsi_event *ev;
    unsigned long flags;

    ev = 0;
    spin_lock_irqsave(&dev->hsi_event_tab_lock, flags);
    if (!list_empty(&dev->hsi_event_free_list)) {
        ev = list_first_entry(&dev->hsi_event_free_list, struct hsi_event, list);
        list_del_init(&ev->list);
    }
    spin_unlock_irqrestore(&dev->hsi_event_tab_lock, flags);

    if (ev) {
        ev->ss_data_length[0] = 0;
        ev->ss_data_length[1] = 0;
        ev->ss_meta_length[0] = 0;
        ev->ss_meta_length[1] = 0;
        ev->ref               = ref;
        ev->vp                = 0;
        ev->length            = data_length;
        ev->xstat             = xstat;
        ev->dcprs_err         = 0;
        ev->hnp_data          = 0;
        ev->hnp_meta          = 0;
    } else {
        trace_ml_hsi_ev_alloc_fail(dev->hix, dev->dix, ref, data_length, xstat, uid);
    }

    return ev;
}


//--------------------------------------------------------------------------
// hsi_ev_free()  Free an HSI event.
//--------------------------------------------------------------------------
static void hsi_ev_free(struct hsi_event *ev)
{
    unsigned long flags;

    //trace_ml_hsi_ev_free_int(ev->dev->hix, ev->dev->dix, ev);
    spin_lock_irqsave(&ev->dev->hsi_event_tab_lock, flags);
    list_add_tail(&ev->list, &ev->dev->hsi_event_free_list);
    spin_unlock_irqrestore(&ev->dev->hsi_event_tab_lock, flags);
}


//--------------------------------------------------------------------------
// hsi_create_class()  Create hsi class and alloc chrdev.
//--------------------------------------------------------------------------
int32_t hsi_create_class(void)
{
    int32_t result = 0;

    // Create a single class for all the HSI devices we're going to expose.
    pr_info("create class %s\n", CLASS_NAME);

    g_hsi_class = class_create(THIS_MODULE, CLASS_NAME);

    if (IS_ERR(g_hsi_class)) {
        pr_err("class_create failed\n");
        result = -EPERM;
    }
    g_hsi_class->devnode = hsi_devnode;

    // Allocate a major device number and a range of minor device numbers.
    pr_info("register HSI region");
    result = alloc_chrdev_region(&g_devs[0], 0, HSI_MAX_DEVS, "hsi_p");
    if (result < 0) {
        pr_err("can't allocate hsi major dev num primary. (%d)\n", result);
        class_destroy(g_hsi_class);
        g_hsi_class = 0;
        return result;
    }

    result = alloc_chrdev_region(&g_devs[1], 0, HSI_MAX_DEVS, "hsi_s");
    if (result < 0) {
        pr_err("can't allocate hsi major dev num secondary. (%d)\n", result);
        unregister_chrdev_region(g_devs[0], HSI_MAX_DEVS);
        class_destroy(g_hsi_class);
        g_hsi_class = 0;
        return result;
    }

    return 0;
}


//--------------------------------------------------------------------------
// hsi_destroy_class()  Destroy hsi class and unregister chrdev.
//--------------------------------------------------------------------------
void hsi_destroy_class(void)
{
    int hix;
    pr_info("unregister HSI region");
    for (hix = 0; hix < HSI_MAX_INSTANCES; hix++) {
        unregister_chrdev_region(g_devs[hix], HSI_MAX_DEVS);
        g_devs[hix] = 0;
    }

    pr_info("removing HSI class");
    class_destroy(g_hsi_class);
    g_hsi_class = 0;
}


//--------------------------------------------------------------------------
// hsi_init()  Initialize everything.
//--------------------------------------------------------------------------
int32_t hsi_init(uint32_t hix, struct pci_dev *pcidev, struct device *sysdev, struct dentry *ddir, struct hsi_root *hsitab, void *hydradev)
{
    int32_t result = 0;

    pr_info("hsi_init: hix=%d, pcidev=%px, hsitab=%px\n", hix, pcidev, hsitab);

    memset(&g_hsi[hix], 0, sizeof(g_hsi[hix]));

    g_hsi[hix].pdev     = pcidev;

    g_hsi[hix].hsi_root = hsitab;

    g_hsi[hix].sysdev   = sysdev;

    g_hsi[hix].ddir     = ddir;

    g_hsi[hix].hydradev = hydradev;

    // Initialize the hashtable.
    hash_init(g_shregion_tbl);

    // hsitab
    result = sysfs_create_file(&g_hsi[hix].sysdev->kobj, &dev_attr_hsitab.attr);
    if (result < 0) {
        pr_err("bad sysfs_create_file (%d)\n", result);
        goto dev_attr_hsitab_create_file_fail;
    }

    // streamtab
    result = sysfs_create_file(&g_hsi[hix].sysdev->kobj, &dev_attr_streamtab.attr);
    if (result < 0) {
        pr_err("bad sysfs_create_file (2) (%d)\n", result);
        goto dev_attr_streamtab_create_file_fail;
    }

    // Debugfs init.
    debugfs_create_file("hsi_probe",   0200, g_hsi[hix].ddir, &g_hsi[hix], &dfs_hsi_probe_fops);
    debugfs_create_file("active_devs", 0200, g_hsi[hix].ddir, &g_hsi[hix], &dfs_hsi_active_devs_fops);
    debugfs_create_u32("ints_per_sec", 0444, g_hsi[hix].ddir, &g_hsi[hix].dfs.ints_per_sec);
    debugfs_create_u32("ints",         0444, g_hsi[hix].ddir, &g_hsi[hix].dfs.ints);
    debugfs_create_u32("dev_maj",      0444, g_hsi[hix].ddir, &g_hsi[hix].maj_num);

    // vcam debug for primary hydra only
    if (hix == HIX_HYDRA_PRI) {
        debugfs_create_u32("dcprs_err_saves", 0666, g_hsi[hix].ddir->d_parent, &g_dfs_dcprs_img_save_count);
    }

    g_hsi[hix].maj_num = MAJOR(g_devs[hix]);
    pr_info("HSI device is 0x%x, major number is %d\n", g_devs[hix], g_hsi[hix].maj_num);

    // Init the event & hsibuf pools.
    hsi_ev_pool_init(hix);
    hsi_buf_pool_init(hix);

    return result;
dev_attr_streamtab_create_file_fail:
    sysfs_remove_file(&g_hsi[hix].sysdev->kobj, &dev_attr_hsitab.attr);
dev_attr_hsitab_create_file_fail:
    return result;
}

//--------------------------------------------------------------------------
// hsi_uninit()  Terminate HSI device instance.
//--------------------------------------------------------------------------
void hsi_uninit(uint32_t hix)
{
    int ix;

    pr_info("HSI uninit, instance=%d\n", hix);

    // Free the spillway.
    if (g_spillway) {
        pr_info("Free spillway\n");
        dma_free_coherent(&g_hsi[hix].pdev->dev, SPILLWAY_SIZE, g_spillway, g_spillway_io);
        g_spillway = 0;
    }

    // Remove all the HSI devices associated with this instance.
    for (ix = 0; ix < HSI_MAX_DEVS; ix++) {
        if (g_hsi[hix].devs[ix].dev) {
            pr_info("Remove HSI device #%d [%s]\n", ix, g_hsi[hix].devs[ix].dev->kobj.name);
            sysfs_remove_group(&g_hsi[hix].devs[ix].dev->kobj, &hsi_attr_group);
            if (g_hsi[hix].devs[ix].attr_hsi_log.size) {
                sysfs_remove_bin_file(&g_hsi[hix].devs[ix].dev->kobj, &g_hsi[hix].devs[ix].attr_hsi_log);
            }
            cdev_del(&g_hsi[hix].devs[ix].cdev);
            device_destroy(g_hsi_class, g_hsi[hix].devs[ix].devno);
        }
    }

    if (hix == HIX_HYDRA_PRI) {
        debugfs_remove(debugfs_lookup("dcprs_err_saves", g_hsi[hix].ddir->d_parent));
    }

    if (g_hsi[hix].sysdev) {
        pr_info("Remove HSI table");
        sysfs_remove_file(&g_hsi[hix].sysdev->kobj, &dev_attr_hsitab.attr);
        sysfs_remove_file(&g_hsi[hix].sysdev->kobj, &dev_attr_streamtab.attr);
    }

    g_hsi[hix].maj_num = 0;
}


//--------------------------------------------------------------------------
// hsi_dev_create()  Create the HSI device instance.
//--------------------------------------------------------------------------
int hsi_dev_create(uint32_t hix, uint32_t dix, uint32_t data_buf_len, uint32_t meta_buf_len, const char *name, const char *vsname, uint32_t vs_ix, uint32_t s_ix, dev_t *devno, int chan_id)
{
    int ret;

    *devno = MKDEV(g_hsi[hix].maj_num, dix);

    // Empty out list of buffers to read.
    INIT_LIST_HEAD(&g_hsi[hix].devs[dix].rdq);
    spin_lock_init(&g_hsi[hix].devs[dix].rdq_lock);
    spin_lock_init(&g_hsi[hix].devs[dix].pendq_lock);
    init_waitqueue_head(&g_hsi[hix].devs[dix].rdq_waitq);
    mutex_init(&g_hsi[hix].devs[dix].dev_lock);

    // Register it.
    cdev_init(&g_hsi[hix].devs[dix].cdev, &fops_hsi);
    g_hsi[hix].devs[dix].cdev.owner = THIS_MODULE;
    ret = cdev_add(&g_hsi[hix].devs[dix].cdev, *devno, 1);
    if (ret) {
        pr_err("bad cdev_add (%d)\n", ret);
        return -1;
    }

    // Create it.
    if (vsname) {
        pr_info("            create HSI device instance [%s.%s], dix=%d, devno=0x%x, vix=%d\n", name, vsname, dix, *devno, vs_ix);
        g_hsi[hix].devs[dix].dev = device_create(g_hsi_class, NULL, *devno, &g_hsi[hix].devs[dix], "%s.%s", name, vsname);
        snprintf(g_hsi[hix].devs[dix].name, sizeof(g_hsi[hix].devs[dix].name), "%s.%s", name, vsname);
    } else {
        pr_info("            create HSI device instance [%s], dix=%d, devno=0x%x\n", name, dix, *devno);
        g_hsi[hix].devs[dix].dev = device_create(g_hsi_class, NULL, *devno, &g_hsi[hix].devs[dix], "%s", name);
        snprintf(g_hsi[hix].devs[dix].name, sizeof(g_hsi[hix].devs[dix].name), "%s", name);
    }
    if (IS_ERR(g_hsi[hix].devs[dix].dev)) {
        pr_err("bad dev create.\n");
        cdev_del(&g_hsi[hix].devs[dix].cdev);
        return -1;
    }

    dev_set_drvdata(g_hsi[hix].devs[dix].dev, &g_hsi[hix].devs[dix]);

    // Allocate a global spillway if it hasn't been allocated already.
    if (! g_spillway) {
        g_spillway = dma_alloc_coherent(&g_hsi[hix].pdev->dev, SPILLWAY_SIZE, &g_spillway_io, GFP_KERNEL);
        if (! g_spillway) {
            pr_err("                Failed to allocate global spillway buffer of size %d", SPILLWAY_SIZE);
            device_destroy(g_hsi_class, *devno);
            cdev_del(&g_hsi[hix].devs[dix].cdev);
            return -1;
        } else {
            pr_err("                Allocated global spillway buffer of size %d", SPILLWAY_SIZE);
        }
    }
    g_hsi[hix].devs[dix].spillway    = g_spillway;
    g_hsi[hix].devs[dix].spillway_io = g_spillway_io;
    pr_info("                Using spillway of size %d at %px, io=0x%016llX\n", SPILLWAY_SIZE, g_hsi[hix].devs[dix].spillway, g_hsi[hix].devs[dix].spillway_io);

    // Save meta length.
    g_hsi[hix].devs[dix].meta_length = meta_buf_len;

    // And the sysfs attributes.
    ret = sysfs_create_group(&g_hsi[hix].devs[dix].dev->kobj, &hsi_attr_group);
    if (ret < 0) {
        pr_err("bad hsi sysfs_create_group (%d)\n", ret);
        cdev_del(&g_hsi[hix].devs[dix].cdev);
        device_destroy(g_hsi_class, *devno);
        return -1;
    }

    // Allow access to spillway for streams that appear to be (based on the name) log streams.
    if ((name[0] == 'l') && (name[1] == 'o') && (name[2] == 'g') ) {
        g_hsi[hix].devs[dix].attr_hsi_log.size      = data_buf_len;
        g_hsi[hix].devs[dix].attr_hsi_log.read      = hsilog_read;
        g_hsi[hix].devs[dix].attr_hsi_log.write     = 0;
        g_hsi[hix].devs[dix].attr_hsi_log.attr.name = "log";
        g_hsi[hix].devs[dix].attr_hsi_log.attr.mode = S_IRUGO;
        if ((ret = sysfs_create_bin_file(&g_hsi[hix].devs[dix].dev->kobj, &g_hsi[hix].devs[dix].attr_hsi_log)) < 0) {
            pr_err("                bad sysfs_create_bin_file, ret=%d\n", ret);
            cdev_del(&g_hsi[hix].devs[dix].cdev);
            device_destroy(g_hsi_class, *devno);
            return -1;
        }
    } else {
        g_hsi[hix].devs[dix].attr_hsi_log.size = 0;
    }

    g_hsi[hix].devs[dix].data_buf_len = data_buf_len;

    g_hsi[hix].devs[dix].six = s_ix;

    g_hsi[hix].devs[dix].vix = vs_ix;

    g_hsi[hix].devs[dix].dix = dix;

    g_hsi[hix].devs[dix].hix = hix;

    g_hsi[hix].devs[dix].is_virtual = vsname ? 1 : 0;

    g_hsi[hix].devs[dix].devno = *devno;

    hydra_irq_req(hix, chan_id, g_hsi[hix].devs[dix].name);

    // Special handling for vcam.
    if (! strcmp(name, "vcam")) {
        g_hsi[hix].devs[dix].is_vcam = 1;
        ret = sysfs_create_file(&g_hsi[hix].devs[dix].dev->kobj, &dev_attr_bpp.attr);
        if (ret < 0) {
            pr_err("can't create bpp file (%d)\n", ret);
        }
        hydra_irq_req(hix, ++chan_id, "vcam2");
    }
    return 0;
}


//--------------------------------------------------------------------------
// hsi_probe()  Scan the stream table to find HSI devices.
//--------------------------------------------------------------------------
int hsi_probe(uint32_t hix)
{
    int dix, six, vix;
    dev_t devno;
    uint32_t irq;
    volatile struct hsi_root *table = g_hsi[hix].hsi_root;
    volatile struct substream *ssp  = 0;

    pr_info("Probing for stream devices on hydra #%d\n", hix);

    // Make sure the table is there and seems sane?
    if (table->hsi_magic != HSI_MAGIC) {
        pr_err("HSI device table is bad: 0x%08X\n", table->hsi_magic);
        return -1;
    }

    // For every stream in the table....
    for (six = 0, dix = 0; six < HSI_MAX_STREAMS; six++) {
        if (table->tab[six].present == 1) {
            int sub;

            pr_info("Found HSI stream #%d, dix=%d, [%s], substreams=%d\n", six, dix, table->tab[six].name, table->tab[six].subs_count);

            // Check for simple case with no vstreams.
            if (table->tab[six].sub[0].vstream_count == 0) {

                // If this is the vcam, allocate bufs for compressed data.
                if (table->tab[six].subs_count > 1) {
                    pr_info("Allocate VCAM buffers of size %d\n", table->tab[six].sub[0].buf_size);
                    vcam_init(&g_hsi[hix].pdev->dev, g_vcam_bpp, table->tab[six].sub[0].buf_size, 3, 0);
                }

                for (sub = 0; sub < table->tab[six].subs_count; sub++) {
                    ssp = &table->tab[six].sub[sub];

                    // Map MSI to irq.
                    if (!(irq = hydra_get_irq(hix, ssp->chan_id))) {
                        pr_info("Bad HSI channel\n");
                        return -1;
                    }

                    pr_info("        Sub %d: irq=%d, bufsize=%d, meta-size=%d, slices=%d\n",
                            sub, irq, ssp->buf_size, ssp->meta_size, ssp->slices);

                    // Save the irq.
                    g_hsi[hix].devs[dix].irq = irq;

                    // Save the number of substreams for this device.
                    g_hsi[hix].devs[dix].sub_count = table->tab[six].subs_count;

                    // Remember stream table entry.
                    g_hsi[hix].devs[dix].sub[sub].ss = ssp;

                    // And the MSI for this substream.
                    g_hsi[hix].devs[dix].sub[sub].irq = ssp->chan_id;
                    spin_lock_init(&g_hsi[hix].devs[dix].sub[sub].irq_lock);

                    // And the device and substream numbers.
                    g_hsi[hix].irq_tab[irq].hsi_dev_ix = dix;
                    g_hsi[hix].irq_tab[irq].hsi_ss_ix  = sub;

                    // Set slice size for vcam stream.
                    if (g_vcam_slice_size_kb && (table->tab[six].subs_count == 2)) {
                        pr_info("Setting slice size to %dKB on dix=%d\n", g_vcam_slice_size_kb, dix);
                        table->tab[six].sub[0].slices = g_vcam_slice_size_kb * 1024;
                        table->tab[six].sub[1].slices = g_vcam_slice_size_kb * 1024;
                        g_hsi[hix].devs[dix].sub[sub].slice_size = g_vcam_slice_size_kb * 1024;
                    } else {
                        g_hsi[hix].devs[dix].sub[sub].slice_size = (ssp->slices == 1) ? 0 : ssp->slices;
                    }
                }
                if (hsi_dev_create(hix, dix, ssp->buf_size, ssp->meta_size, (char *)table->tab[six].name, 0, 0, six, &devno, g_hsi[hix].devs[dix].sub[0].ss->chan_id)) {
                    pr_err("Failed to create HSI device\n");
                }
                // Propogate the (same) spillway to all the substreams of this stream.
                for (sub = 0; sub < table->tab[six].subs_count; sub++) {
                    ssp = &table->tab[six].sub[sub];
                    ssp->spillway_buf_addr[0] = g_hsi[hix].devs[dix].spillway_io;
                }
                dix++;
                continue;
            }

            // Check for single-substream and some vstreams.
            if ((table->tab[six].subs_count == 1) && (table->tab[six].sub[0].vstream_count > 0)) {
                ssp = &table->tab[six].sub[0];

                // Map MSI to irq.
                if (!(irq = hydra_get_irq(hix, ssp->chan_id))) {
                    pr_info("Bad HSI channel\n");
                    return -1;
                }

                // Save the irq.
                g_hsi[hix].devs[dix].irq = irq;

                // Note that all vstreams share the same irq. The irq maps to the first vstream device.
                g_hsi[hix].irq_tab[irq].hsi_dev_ix = dix;
                g_hsi[hix].irq_tab[irq].hsi_ss_ix  = 0;

                pr_info("        Sub %d: irq=%d, bufsize=%d, meta-size=%d, slices=%d, vstream_cnt=%d\n",
                        0, irq, ssp->buf_size, ssp->meta_size, ssp->slices, ssp->vstream_count);

                for (vix = 0; vix < ssp->vstream_count; vix++) {
                    pr_info("            Vstream(%d) := [%s]", vix, ssp->vstream_name[vix]);

                    // Remember stream table entry.
                    g_hsi[hix].devs[dix].sub[0].ss = ssp;

                    // And the MSI for this substream.
                    g_hsi[hix].devs[dix].sub[0].irq = ssp->chan_id;
                    spin_lock_init(&g_hsi[hix].devs[dix].sub[0].irq_lock);
                    g_hsi[hix].devs[dix].irq = irq;

                    // Save the number of substreams for this device.
                    g_hsi[hix].devs[dix].sub_count = table->tab[six].subs_count;

                    // Number of slices.
                    g_hsi[hix].devs[dix].sub[0].slice_size = (ssp->slices == 1) ? 0 : ssp->slices;

                    if (hsi_dev_create(hix, dix, ssp->buf_size, ssp->meta_size, (char *)table->tab[six].name, (char *)ssp->vstream_name[vix], vix, six, &devno, ssp->chan_id)) {
                        pr_err("Failed to create HSI vstream device\n");
                    }
                    ssp->spillway_buf_addr[vix] = g_hsi[hix].devs[dix].spillway_io;

                    dix++;
                }
                continue;
            }
        }
    }

    // Tell hydra we're done processing the table.
    table->host_proc_complete = 1;
    smp_wmb();

    return 0;
}


//--------------------------------------------------------------------------
// hsi_tick()  Called at 1 HZ.
//--------------------------------------------------------------------------
void hsi_tick(void)
{
    int i;

    g_hsi[0].dfs.ints_per_sec = g_hsi[0].dfs.clr_on_tick.irq_count_tot;
    g_hsi[1].dfs.ints_per_sec = g_hsi[1].dfs.clr_on_tick.irq_count_tot;

    for (i = 0; i < HSI_MAX_DEVS; i++) {
        g_hsi[0].devs[i].irq_per_sec = g_hsi[0].dfs.clr_on_tick.irq_count[i];
        g_hsi[1].devs[i].irq_per_sec = g_hsi[1].dfs.clr_on_tick.irq_count[i];
    }

    memset(&g_hsi[0].dfs.clr_on_tick, 0, sizeof(g_hsi[0].dfs.clr_on_tick));
    memset(&g_hsi[1].dfs.clr_on_tick, 0, sizeof(g_hsi[1].dfs.clr_on_tick));
}


//--------------------------------------------------------------------------
// uid_decode()  Stringify a completion type.
//--------------------------------------------------------------------------
char *uid_decode(uint16_t uid)
{
    if (uid == HSI_UID_XSPILLWAY) {
        return "DROP";
    }
    if (uid & HSI_UID_HEADER) {
        return "HEAD";
    }
    if (uid & HSI_UID_FOOTER) {
        return "FOOT";
    }
    if (!(uid & (HSI_UID_HEADER | HSI_UID_FOOTER))) {
        return "DATA";
    }
    return "UNKN";
}


#if 0
//--------------------------------------------------------------------------
// shregion_sync()  Sync up cache with shregion device mem.
//--------------------------------------------------------------------------
static void shregion_sync(struct hsi_buf *hsibuf)
{
#ifdef __aarch64__
    // Flush PCIe pipeline to DDR.
    if (cvip_pci_flush_dram_try(2)) {
        pr_err("too many dram flush tries\n");
    }

    dma_buf_begin_cpu_access(hsibuf->dma_dst_buf_data, DMA_FROM_DEVICE);
    dma_buf_end_cpu_access(hsibuf->dma_dst_buf_data, DMA_FROM_DEVICE);
#endif
}
#endif


//--------------------------------------------------------------------------
// shregion_map()  Set up shregion mapping to device.
//--------------------------------------------------------------------------
#ifdef __aarch64__
static struct h_node *shregion_map(uint32_t hix, uint64_t shregion_name)
{
    struct h_node *hnp = 0, *cnp;

    // Have we seen this one before?
    hash_for_each_possible(g_shregion_tbl, cnp, node, shregion_name) {
        if (cnp->sh_name == shregion_name) {
            hnp = cnp;
        }
    }
    if (! hnp) {
        struct sg_table *sg;

        hnp = devm_kzalloc(&g_hsi[hix].pdev->dev, sizeof(*hnp), GFP_KERNEL);
        hnp->sh_name = shregion_name;

        // Temporarily pass shregion name in flags param.
        if ((hnp->dma_buf = get_dma_buf_for_shr(shregion_name)) == NULL) {
            pr_err("bad shregion name lookup on 0x%016llx\n", shregion_name);
            return 0;
        }

        sg = ion_sg_table(hnp->dma_buf);
        if (IS_ERR(sg)) {
            pr_err("failed to get sg table\n");
            return 0;
        }
        WARN_ON(sg->nents > 1);

        // Get aperture physicaladdress.
        hnp->sh_paddr = page_to_phys(sg_page(sg->sgl));

        // Get SOC address.
        hnp->sh_paddr_io = page_to_phys(sg_page(sg->sgl)) - 0x800000000ULL;

        hnp->sh_vaddr = dma_buf_kmap(hnp->dma_buf, 0);

        pr_info("%s: paddr_io=0x%llx, vaddr=0x%px, vaddr2=0x%px\n", __func__, hnp->sh_paddr, hnp->sh_vaddr, phys_to_virt(hnp->sh_paddr));
        if (!hnp->sh_vaddr) {
            pr_err("failed to kmap\n");
            return 0;
        }

        hash_add(g_shregion_tbl, &hnp->node, shregion_name);
    }

    DPRINT(2, "mapout: shregion pa=0x%llx, pa_io=0x%llx\n", hnp->sh_paddr, hnp->sh_paddr_io);
    return hnp;
}
#endif


//--------------------------------------------------------------------------
// hsi_intr()  Process a stream MSI interrupt.
//--------------------------------------------------------------------------
void hsi_intr(uint32_t hix, uint32_t irq)
{
    volatile struct substream *ssp;
    enum hsi_stream_state cur_state, *exp_state;
    struct tr_args_intr trv;
    struct hsi_substream *hss;
    struct hsi_buf *hsibuf;
    struct hsi_event *ev;
    uint32_t vix, dix, ssix, bufix, tail, head, dlen, slice, xstat, uhand, fcnt;
    uint64_t intr_ts, submit_ts;
    uint16_t uid, uidr;
    unsigned long cpu, flags, qrt;

    if (g_stand_down[hix]) {
        pr_warn("unexpected intr in stand_down mode: hix=%d, irq=%d\n", hix, irq);
        return;
    }

    intr_ts = ktime_get_boottime_ns();

    // Get the substream that's associated with this dma channel.
    trv.dix  = dix  = g_hsi[hix].irq_tab[irq].hsi_dev_ix;  // device index: this is probably WRONG in case of a vstream
    trv.ssix = ssix = g_hsi[hix].irq_tab[irq].hsi_ss_ix;   // substream index
    hss  = &g_hsi[hix].devs[dix].sub[ssix];

    g_hsi[hix].dfs.clr_on_tick.irq_count_tot++;
    g_hsi[hix].dfs.clr_on_tick.active[dix]++;
    g_hsi[hix].dfs.clr_on_tick.irq_count[dix]++;
    g_hsi[hix].dfs.ints++;

    // Note that in the case of a vstream transfer, the device index above may be wrong,
    // as it will always indicate the device associated with the first vstream in the stream.
    // Since all the vstream devices in a stream point to the same completion queue, it doesn't matter.

    trv.cpu  = cpu  = smp_processor_id();
    trv.pass = 0;
    trv.hix  = hix;

    // Get shortcut to the substream.
    ssp = hss->ss;

    // Handle atypical case where interrupt beats the queue header update.
    smp_rmb();
    qrt = 0;
    do {
        head = hioread32(&ssp->cplq_hdr.head);
        tail = hioread32(&ssp->cplq_hdr.tail);
    } while ((CIRC_CNT(head, tail, HSI_MAX_QELEM) < 1) && (qrt++ < 10));
    if (qrt >= 10) {
        if (g_hydra_dbg_mask) {
            pr_warn("empty queue on cpu=%ld, hix=%d, irq=%d, dix=%d, ssix=%d\n", cpu, hix, irq, dix, ssix);
        }
        g_hsi[hix].devs[dix].stats.empty_queue++;
    }

    DPRINT(6, "INTR(%ld):: BOT ENTER hix=%d, irq=%d, dix=%d, ssix=%d\n", cpu, hix, irq, dix, ssix);
    trace_ml_hsi_intr0(cpu, hix, irq, dix, ssix, head, tail, qrt);

    while (1) {
        struct hsi_compl_qelem qel;
        uint32_t user_handle_c, user_id_c;
        ev = 0;

        // Protect from the same MSI hitting another core.
        spin_lock(&g_hsi[hix].devs[dix].sub[ssix].irq_lock);

        // Check for non-empty completion queue.
        head = hioread32(&ssp->cplq_hdr.head);
        tail = hioread32(&ssp->cplq_hdr.tail);
        if (CIRC_CNT(head, tail, HSI_MAX_QELEM) < 1) {
            spin_unlock(&g_hsi[hix].devs[dix].sub[ssix].irq_lock);
            break;
        }

        // Dequeue the next buffer completed.
        trv.tail   = tail  = tail & (HSI_MAX_QELEM - 1);

        user_handle_c = hioread16(&ssp->cmplq[tail].user_handle);
        user_id_c     = hioread16(&ssp->cmplq[tail].user_id);
        trv.uhand     = uhand = user_handle_c;
        trv.bufix     = bufix = user_handle_c & 0xff;
        trv.vix       = vix   = (user_id_c & HSI_UID_VSTREAM_MASK) >> HSI_UID_VSTREAM_MASK_S;
        trv.uidr      = uidr  = user_id_c;
        trv.uid       = uid   = uidr & HSI_UID_MASK;
        trv.slice     = slice = user_id_c & HSI_UID_SLICE_ID;
        qel.status.register_value = hioread32(&ssp->cmplq[tail].status.register_value);
        trv.dlen   = dlen  = qel.status.bitfield.COMP_BYTE_COUNT;
        trv.fcnt   = fcnt  = qel.status.bitfield.FRAME_COUNT;
        trv.xstat  = xstat = qel.status.register_value;
        trv.dev_name = dev_name(g_hsi[hix].devs[dix+vix].dev);


        // Update queue tail.
        hiowrite32(tail + 1, &ssp->cplq_hdr.tail);
        spin_unlock(&g_hsi[hix].devs[dix].sub[ssix].irq_lock);

        // Make sure the buffer index is legit.
        if ((bufix >= HSI_MAX_BUFS) && (uhand != HSI_UID_XSPILLWAY)) {
            pr_err("Illegal buffer index in completion queue: %d (0x%x), dix=%d (%s)\n", bufix, bufix, dix, dev_name(g_hsi[hix].devs[dix+vix].dev));
            trace_ml_hsi_intr_bad_buf_ix(cpu, hix, ssix, dix+vix, bufix);
            return;
        }
        trv.ref = g_hsi[hix].buf_tab[bufix].ref;

        DPRINT(5, "INTR(%ld):: TOP pass=%d, ssix=%d, tail=%d, t=%s, buf_ix=%duser_id=0x%x, user_handle=0x%x, vix=%d, length=%d, slice=%d\n", cpu, trv.pass, ssix, tail, uid_decode(uid), bufix, uidr, uhand, vix, dlen, slice);
        trace_ml_hsi_intr1(&trv);

        // Count passes through loop.0
        trv.pass++;

        // Update per-device stats
        g_hsi[hix].devs[dix+vix].stats.ints++;

        // Check for a Hydra-side transfer error.
        if (xstat & NWL_XSTAT_ANY_ERR) {
            trace_ml_hsi_intr_dma_err(hix, dix+vix, xstat, tail, bufix);
            pr_err("Hydra DMA transfer error: %08X, hix=%d, dix+vix=%d, tail=%d, buf_ix=%d, user_id=%04x, user_handle=%04x\n", xstat, hix, dix+vix, tail, bufix, user_id_c, user_handle_c);
        }

        // Check for a spillway completion.
        if (uhand == HSI_UID_XSPILLWAY) {
            g_hsi[hix].devs[dix+vix].stats.frames_dropped++;
            g_hsi[hix].devs[dix+vix].spills_new++;
            DPRINT(5, "intr(%ld):: hix=%d, dix+vix=%d, ssix=%d, dropped_frame=%d\n", cpu, hix, dix+vix, ssix, g_hsi[hix].devs[dix+vix].stats.frames_dropped);
            trace_ml_hsi_intr_spill(cpu, ssix, dix+vix, dlen, g_hsi[hix].devs[dix+vix].stats.frames_dropped);
            continue;
        }

        // If we receive frames on a stream that's since been closed, drop it.
        // The buffer pair will already have been marked for release when the stream was closed.
        if (! g_hsi[hix].devs[dix+vix].opened) {
            g_hsi[hix].devs[dix+vix].stats.frames_orphaned++;
            trace_ml_hsi_intr_orphaned_hsi_buf(cpu, hix, ssix, dix+vix, dev_name(g_hsi[hix].devs[dix+vix].dev), bufix, dlen, tail);
            DPRINT(1, "data from closed stream: %s, hix=%d, dix+vix=%d, buf_ix=%d, len=%d\n", dev_name(g_hsi[hix].devs[dix+vix].dev), hix, dix+vix, bufix, dlen);
            continue;
        }

        // Make sure the buffer itself seems reasonably sane.
        hsibuf = &g_hsi[hix].buf_tab[bufix];
        if (hsibuf->magic == HSIBUF_MAGIC) {
            if (hsibuf->active) {
                if ( (!hsibuf->data) || (!hsibuf->data_io) ) {
                    pr_err("Corrupted hsi buffer! (hix=%d, dix+vix=%d, buf_ix=%d, tail=%d, magic=0x%08X, active=%d)\n", hix, dix+vix, bufix, tail, hsibuf->magic, hsibuf->active);
                    trace_ml_hsi_intr_corrupt_hsi_buf(cpu, hix, ssix, dix+vix, bufix, tail, hsibuf->magic, hsibuf->active);
                    return;
                }
            } else {
                // It's a stale buffer.
                trace_ml_hsi_intr_stale_hsi_buf(cpu, hix, ssix, dix+vix, bufix, tail);
                DPRINT(1, "stale HSI buffer: hix=%d, dix+vix=%d, buf_ix=%d\n", hix, dix+vix, bufix);
                continue;
            }
        } else {
            pr_err("Malformed HSI buffer! magic=0x%08x\n", hsibuf->magic);
            continue;
        }

        submit_ts = hsibuf->submit_ts;

        // Make sure that we got what we expected (header vs footer vs data) from this device.
        // Not yet sure what the best way to deal with these errors is, but for now at least
        // flag it since they usually lead to a crash.
        exp_state = (ssix == 0) ? &g_hsi[hix].devs[dix+vix].nxt_str_state0 : &g_hsi[hix].devs[dix+vix].nxt_str_state1;
        cur_state = current_state(uid);
        if (*exp_state != cur_state) {
            trace_ml_hsi_bad_state(cpu, hix, ssix, dix+vix, (int)cur_state, *exp_state, g_hsi[hix].devs[dix+vix].is_vcam);
            pr_err("bad stream state on device %s, got %d, expected %d\n", dev_name(g_hsi[hix].devs[dix+vix].dev), (int)cur_state, *exp_state);
        }
        *exp_state = next_state(&g_hsi[hix].devs[dix+vix], cur_state);

        // What kind of data is this?
        if (uid & HSI_UID_HEADER) {

            spin_lock_irqsave(&hsibuf->lock, flags);

            hsibuf->ss_meta_length[ssix] += dlen;

            // If we've received headers on all substreams...
            if (atomic_inc_return(&hsibuf->ss_head_count) == g_hsi[hix].devs[dix+vix].sub_count) {

                spin_unlock_irqrestore(&hsibuf->lock, flags);
#ifndef __aarch64__
                dma_sync_single_for_cpu(&g_hsi[hix].pdev->dev,
                                        hsibuf->meta_io + (ssix * hsibuf->ss_meta_size) + hsibuf->ss_meta_length[ssix] - dlen,
                                        dlen,
                                        PCI_DMA_FROMDEVICE);
#endif
                // Get an event and set it up.
                if ((ev = hsi_ev_alloc(&g_hsi[hix].devs[dix+vix], uid, hsibuf->ref, hsibuf->ss_meta_length[ssix], xstat & NWL_XSTAT_ANY_ERR)) == 0) {
                    pr_err("can't alloc hsi event\n");
                    return;
                }

                DPRINT(5, "intr(%ld):: Got final HEAD, ssix=%d, hsibuf=%px, ev=%px, hdr_cnt=%d\n", cpu, ssix, hsibuf, ev, atomic_read(&hsibuf->ss_head_count));
                trace_ml_hsi_intr_meta("Got final HEAD", cpu, hix, dix, dev_name(g_hsi[hix].devs[dix+vix].dev), ssix, hsibuf, hsibuf->ref, ev, ev->evix, atomic_read(&hsibuf->ss_head_count));

                // Completed metadata header.
                ev->flags  = HSI_EVFLAG_META_HEADER;
                ev->data   = hsibuf->meta_data;
                ev->slice  = 0;
                if (! g_hsi[hix].devs[dix+vix].sub[ssix].slice_size) {
                    // If we're not slicing, make sensor_core think that all meta's coming on the first substream.
                    ev->ss_meta_length[0] = hsibuf->ss_meta_length[ssix];
                } else {
                    ev->ss_meta_length[ssix] = hsibuf->ss_meta_length[ssix];
                }

                // Free up buffer pair on dma error.
                if (ev && ev->xstat) {
                    hsi_buf_free(hsibuf, 1);
                }
            } else {
                // We haven't received headers on all substreams yet.
                spin_unlock_irqrestore(&hsibuf->lock, flags);
                DPRINT(5, "intr(%ld):: Got partial HEAD, ssix=%d, hsibuf=%px, hdr_cnt=%d\n", cpu, ssix, hsibuf, atomic_read(&hsibuf->ss_head_count));
                trace_ml_hsi_intr_meta("Got partial HEAD", cpu, hix, dix+vix, dev_name(g_hsi[hix].devs[dix+vix].dev), ssix, hsibuf, hsibuf->ref, 0, 0, atomic_read(&hsibuf->ss_head_count));
            }

        } else if (uid & HSI_UID_FOOTER) {
            spin_lock_irqsave(&hsibuf->lock, flags);

            hsibuf->ss_meta_length[ssix] += dlen;

            // If we've received footers on all substreams...
            if (atomic_inc_return(&hsibuf->ss_foot_count) == g_hsi[hix].devs[dix+vix].sub_count) {

                spin_unlock_irqrestore(&hsibuf->lock, flags);

                // Get an event and set it up.
                if ((ev = hsi_ev_alloc(&g_hsi[hix].devs[dix+vix], uid, hsibuf->ref, hsibuf->ss_meta_length[ssix], xstat & NWL_XSTAT_ANY_ERR)) == 0) {
                    pr_err("can't alloc hsi event\n");
                    return;
                }

                DPRINT(5, "intr(%ld):: Got final FOOT, ssix=%d, hsibuf=%px, ev=%px, foot_cnt=%d\n", cpu, ssix, hsibuf, ev, atomic_read(&hsibuf->ss_foot_count));
                trace_ml_hsi_intr_meta("Got final FOOT", cpu, hix, dix, dev_name(g_hsi[hix].devs[dix+vix].dev), ssix, hsibuf, hsibuf->ref, ev, ev->evix, atomic_read(&hsibuf->ss_foot_count));

                g_hsi[hix].devs[dix+vix].stats.frames++;

                // Completed metadata footer.
                ev->flags = HSI_EVFLAG_META_FOOTER;
                ev->data  = hsibuf->meta_data + hsibuf->ss_meta_length[ssix];
                ev->slice = 0;
                ev->dcprs_err = hsibuf->dcprs_err;
                if (! g_hsi[hix].devs[dix+vix].sub[ssix].slice_size) {
                    // If we're not slicing, make sensor_core think that all meta's coming on the first substream.
                    ev->ss_meta_length[0] = hsibuf->ss_meta_length[ssix];
                } else {
                    ev->ss_meta_length[ssix] = hsibuf->ss_meta_length[ssix];
                }

                // The complete frame has been received, so free up the hsibuf.
                if (atomic_read(&hsibuf->ss_foot_count) == g_hsi[hix].devs[dix+vix].sub_count) {
                    hsi_buf_free(hsibuf, 2);
                }
            } else {
                // Slicing is disabled and we haven't gotten footers on all substreams yet.
                spin_unlock_irqrestore(&hsibuf->lock, flags);
                DPRINT(5, "intr(%ld):: Got partial FOOT, ssix=%d, hsibuf=%px, foot_cnt=%d\n", cpu, ssix, hsibuf, atomic_read(&hsibuf->ss_foot_count));
                trace_ml_hsi_intr_meta("Got partial FOOT", cpu, hix, dix+vix, dev_name(g_hsi[hix].devs[dix+vix].dev), ssix, hsibuf, hsibuf->ref, 0, 0, atomic_read(&hsibuf->ss_foot_count));
            }

        } else if (!(uid & (HSI_UID_HEADER | HSI_UID_FOOTER))) { // Neither header nor footer. I.E., data.

            spin_lock_irqsave(&hsibuf->lock, flags);

            // Length per substream.
            hsibuf->ss_data_length[ssix] += dlen;

            // If this is a slice....
            if (g_hsi[hix].devs[dix+vix].sub[ssix].slice_size) {
                static int dcprs_started = 0;
                int final = (uid & HSI_UID_LAST_SLICE) ? 1 : 0;

                DPRINT(5, "intr(%ld):: Got %s SLICE, ssix=%d, hsibuf=%px, slice=%d, dlen=%d, count=%d\n",
                       cpu, final ? "(Final)" : "a", ssix, hsibuf, slice, dlen, atomic_read(&hsibuf->ss_data_count));
                trace_ml_hsi_intr_data(final ? "Got (Final) SLICE" : "Got a SLICE", cpu, dix+vix, dev_name(g_hsi[hix].devs[dix+vix].dev), ssix, hsibuf, 0, 0, slice, dlen, hsibuf->ss_data_length[ssix], 0, 0);

                // For the first slice on whichever channel comes first...
                if (slice == 1 && (dcprs_started == 0)) {
                    if (vcam_decompress_sect_start(hsibuf->vp->buf_g, hsibuf->vp->buf_rb,
                                                   hsibuf->vp->buf_dst_io, hsibuf->vp->buf_g_io, hsibuf->vp->buf_rb_io) < 0) {
                        pr_err("can't decompress first slice\n");
                        return;
                    }
                    dcprs_started++;
                }

                vcam_decompress_sect_next(ssix, dlen);

                // Is this the last slice?
                if (final) {
                    // Flush
                    vcam_decompress_sect_next(ssix, 0);
                    dcprs_started = 0;

                    //ev->flags |= (uid & HSI_UID_LAST_SLICE) ? (HSI_EVFLAG_SLICE | HSI_EVFLAG_LAST_SLICE) : HSI_EVFLAG_SLICE;
                    if (atomic_inc_return(&hsibuf->ss_data_count) == 2) {
                        // The whole frame has been submitted to the engine.

                        // Get an event and set it up.
                        if ((ev = hsi_ev_alloc(&g_hsi[hix].devs[dix+vix], uid, hsibuf->ref, hsibuf->ss_data_length[ssix], xstat & NWL_XSTAT_ANY_ERR)) == 0) {
                            pr_err("can't alloc hsi event\n");
                            return;
                        }

                        // Completed data (or slice) payload.
                        ev->flags = HSI_EVFLAG_DATA;
                        ev->data  = hsibuf->data;
                        ev->vp    = hsibuf->vp;
                        hsibuf->vp->size_g  = hsibuf->ss_data_length[0];
                        hsibuf->vp->size_rb = hsibuf->ss_data_length[1];
                        hsibuf->vp->buf_dst = hsibuf->vaddr_dst_data;
                        hsibuf->vp->off_dst = hsibuf->data_io - (dma_addr_t)hsibuf->sh_base_pa_io_data;

                        atomic_set(&hsibuf->ss_data_count, 0);
                    }
                }
                spin_unlock_irqrestore(&hsibuf->lock, flags);
            }

            // Figure out if we've received an entire (and possibly, compressed) frame's worth of data.
            // If slicing is enabled, that means getting a slice of compressed data on either substream.
            // Note that "hsibuf->ss_data_count" is useless if slicing is enabled since we don't know the full slice count ahead of time.
            //
            // If slicing is disabled and we got compressed frame data on all substreams.... OR if slicing is enabled....
            if ((g_hsi[hix].devs[dix+vix].sub[ssix].slice_size == 0) &&
                (atomic_inc_return(&hsibuf->ss_data_count) == g_hsi[hix].devs[dix+vix].sub_count)) {
                spin_unlock_irqrestore(&hsibuf->lock, flags);

                // Get an event and set it up.
                if ((ev = hsi_ev_alloc(&g_hsi[hix].devs[dix+vix], uid, hsibuf->ref, hsibuf->ss_data_length[ssix], xstat & NWL_XSTAT_ANY_ERR)) == 0) {
                    pr_err("can't alloc hsi event\n");
                    return;
                }

                // Completed data (or slice) payload.
                ev->flags    = HSI_EVFLAG_DATA;
                ev->data     = hsibuf->data;
                ev->slice    = slice;
                ev->vp       = hsibuf->vp;
                ev->hnp_data = hsibuf->hnp_data;
                ev->hnp_meta = hsibuf->hnp_meta;
                if (! g_hsi[hix].devs[dix+vix].sub[ssix].slice_size) {
                    // If we're not slicing, make sensor_core think that all data's coming on the first substream.
                    ev->ss_data_length[0] = hsibuf->ss_data_length[ssix];
                } else {
                    ev->ss_data_length[ssix] = hsibuf->ss_data_length[ssix];
                }

                // If this is NOT a slice....
                if (g_hsi[hix].devs[dix+vix].sub[ssix].slice_size == 0) {
                    // We're not slicing, but we got our data on all the substreams.
                    DPRINT(5, "intr(%ld):: Got final DATA, ssix=%d, hsibuf=%px, ev=%px, dlen=%d, data_cnt=%d (%d+%d)\n", cpu, ssix, hsibuf, ev, dlen, hsibuf->ss_data_length[ssix], ev->ss_data_length[0], ev->ss_data_length[1]);
                    trace_ml_hsi_intr_data("Got final DATA", cpu, dix+vix, dev_name(g_hsi[hix].devs[dix+vix].dev), ssix, hsibuf, ev, ev->evix, slice, dlen, hsibuf->ss_data_length[ssix], ev->ss_data_length[0], ev->ss_data_length[1]);

                    // If this is a VCAM image, decompress it.
                    if (g_hsi[hix].devs[dix+vix].sub_count > 1) {

                        if (! hsibuf->vp) {
                            trace_ml_hsi_buf_no_vp(hix, ssix, dix+vix, bufix, hsibuf, hsibuf->active);
                            pr_err("no hsibuf vp!\n");
                        } else {
                            int dret;

                            DPRINT(5, "intr(%ld):: dcprs(dst=0x%llx, g_io=0x%llx, g_phys=0x%llx, rb_io=0x%llx, rb_phys=0x%llx, g_len=%d, rb_len=%d\n",
                                   cpu, hsibuf->data_io, hsibuf->vp->buf_g_io, hsibuf->vp->buf_g_phys, hsibuf->vp->buf_rb_io, hsibuf->vp->buf_rb_phys, hsibuf->ss_data_length[0], hsibuf->ss_data_length[1]);

                            ev->irq = irq;

                            hsibuf->vp->t_decomp_start = ktime_get_boottime_ns();
                            hsibuf->vp->size_g  = hsibuf->ss_data_length[0];
                            hsibuf->vp->size_rb = hsibuf->ss_data_length[1];
                            hsibuf->vp->buf_dst = hsibuf->vaddr_dst_data;
                            hsibuf->vp->off_dst = hsibuf->data_io - (dma_addr_t)hsibuf->sh_base_pa_io_data;

                            //trace_ml_hsi_utrace0("vcam_buf", 555, (uint64_t)hsibuf->vp, 0, 0);

                            // Invalidate the first few bytes in each image component so we can sanity check it before decompression.
                            shregion_arm_cache_invalidate_range(hsibuf->vp->sh_name_g,  hsibuf->vp->dmabuf_g, 0, 32);
                            shregion_arm_cache_invalidate_range(hsibuf->vp->sh_name_rb, hsibuf->vp->dmabuf_rb, 0, 32);

                            dret = vcam_buf_decompress_start(hsibuf->vp->buf_g, hsibuf->vp->buf_rb, hsibuf->vaddr_dst_data,
                                                             hsibuf->vp->buf_dst_io, hsibuf->vp->buf_g_io, hsibuf->vp->buf_rb_io,
                                                             hsibuf->ss_data_length[0], hsibuf->ss_data_length[1]);
                            if (dret < 0) {
                                pr_err("dcprs error\n");
                                if (g_hsi[hix].devs[dix+vix].closed && (dret == -EBUSY)) {
                                    pr_err("device has been closed, dcprs is busy!\n");
                                }
                                ev->dcprs_err = 1;
                            }
                        }
                    }

                    // Free up buffer if we're not expecting a footer after this.
                    if (! hsibuf->meta_io) {
                        g_hsi[hix].devs[dix+vix].stats.frames++;
                        hsi_buf_free(hsibuf, 3);
                    }
                }
            } else {
                if (g_hsi[hix].devs[dix+vix].sub[ssix].slice_size == 0) {
                    // Haven't gotten data on all substreams yet, and we're not slicing.
                    spin_unlock_irqrestore(&hsibuf->lock, flags);
                    DPRINT(5, "intr(%ld):: Got partial DATA, ssix=%d, hsibuf=%px, data_cnt=%d, slice=%d\n", cpu, ssix, hsibuf, hsibuf->ss_data_length[ssix], slice);
                    trace_ml_hsi_intr_data("Got partial DATA", cpu, dix+vix, dev_name(g_hsi[hix].devs[dix+vix].dev), ssix, hsibuf, 0, 0, slice, dlen, hsibuf->ss_data_length[ssix], 0, 0);
                }
            }
        } else {
            // Unrecognized UID
            pr_err("Got an unknown transfer of type 0x%x\n", uid);
            return;
        }

        if (ev) {
            DPRINT(5, "intr(%ld):: Pass up event %px, evix=%d, ref=%llx\n", cpu, ev, ev->evix, ev->ref);
            trace_ml_hsi_intr_wake(cpu, dix+vix, dev_name(g_hsi[hix].devs[dix+vix].dev), ev, ev->evix, ev->slice, ev->ref, ev->data, ev->ss_meta_length[0], ev->ss_meta_length[1], ev->ss_data_length[0], ev->ss_data_length[1]);

            // Pass event to user.
            spin_lock_irqsave(&g_hsi[hix].devs[dix+vix].rdq_lock, flags);
            list_add_tail(&ev->list, &g_hsi[hix].devs[dix+vix].rdq);
            spin_unlock_irqrestore(&g_hsi[hix].devs[dix+vix].rdq_lock, flags);

            // Tell user how many spills he's had since the last time.
            ev->spills = g_hsi[hix].devs[dix+vix].spills_new;
            g_hsi[hix].devs[dix+vix].spills_new = 0;

            // Wake up waiting proc.
            ev->submit_ts = submit_ts;
            ev->intr_ts   = intr_ts;
            ev->wake_ts   = ktime_get_boottime_ns();
            wake_up_interruptible(&g_hsi[hix].devs[dix+vix].rdq_waitq);
        }
    }

    trace_ml_hsi_intr_exit(trv.pass, ktime_get_boottime_ns() - intr_ts);
}


//--------------------------------------------------------------------------
// hsi_open()
//--------------------------------------------------------------------------
static int hsi_open(struct inode *pInode, struct file *pFile)
{
    struct hsi_dev *dev;
    unsigned int mj, mn, hix;

    mj = imajor(pInode);    // indicates which hydra
    mn = iminor(pInode);    // indicates which hsi device on that hydra

    if (mn >= HSI_MAX_DEVS) {
        pr_err("hsi_open: illegal device index: %d\n", mn);
        return -1;
    }

    // Which hydra does this device belong to?
    hix = (g_hsi[0].maj_num == mj) ? 0 : 1;

    // And which HSI stream.
    dev = &g_hsi[hix].devs[mn];
    pFile->private_data = dev;

    pr_info("hsi_open(%s, maj=%d, min=%d, hix=%d, dix=%d, vix=%d)\n", dev_name(dev->dev), mj, mn, hix, dev->dix, dev->vix);

    memcpy(dev->opened_by_name, current->comm, TASK_COMM_LEN);
    trace_ml_hsi_open(dev_name(dev->dev), dev->hix, dev->dix, dev->opened_by_name);

    // Make sure only a single process can open this.
    mutex_lock(&dev->dev_lock);
    if (dev->opened) {
        pr_err("HSI dev already opened by another process\n");
        mutex_unlock(&dev->dev_lock);
        return -EBUSY;
    }

    // Was this device closed earlier? Release internal resources if it has.
    if (dev->closed) {
        unsigned long flags;
        struct list_head *csr, *nxt;
        struct hsi_event *ev = 0;

        // If this is vcam, release vcam-specific resources.
        if (! strncmp(dev_name(dev->dev), "vcam", 4)) {
            vcam_buf_free_all();
        }

        // Release all event buffers for this device.
        spin_lock_irqsave(&dev->rdq_lock, flags);
        list_for_each_safe(csr, nxt, &dev->rdq) {
            ev = list_entry(csr, struct hsi_event, list);
            list_del_init(&ev->list);
            hsi_ev_free(ev);
        }
        spin_unlock_irqrestore(&dev->rdq_lock, flags);
    }

    // Release all HSI buffers associated with this device.
    hsi_buf_dev_release(dev);

    dev->closed = 0;
    dev->opened = 1;
    dev->opened_by_pid = current->pid;

    dev->nxt_str_state0 = (dev->meta_length) ? HEADER : DATA;
    dev->nxt_str_state1 = (dev->meta_length) ? HEADER : DATA;

    mutex_unlock(&dev->dev_lock);

    return 0;
}


//--------------------------------------------------------------------------
// hsi_release()
//--------------------------------------------------------------------------
static int hsi_release(struct inode *pInode, struct file *pFile)
{
    struct hsi_dev *dev = (struct hsi_dev *)pFile->private_data;
    const char *dname = dev_name(dev->dev);

    trace_ml_hsi_close(dev_name(dev->dev), dev->hix, dev->dix);

    pr_info("hsi_release(%s)\n", dname);

    if (dev->opened == 0) {
        pr_err("closing closed device\n!!!");
        return 0;
    }

    dev->closed = 1;
    dev->opened = 0;
    smp_wmb();

    return 0;
}


//--------------------------------------------------------------------------
// hsi_poll()
//--------------------------------------------------------------------------
static unsigned int hsi_poll(struct file *pFile, poll_table *wait)
{
    struct hsi_dev *dev = (struct hsi_dev *)pFile->private_data;

    poll_wait(pFile, &dev->rdq_waitq, wait);

    return POLLIN | POLLRDNORM;
}


static void hiowrite64(u64 val,  volatile void __iomem *addr)
{
    volatile uint32_t *ip = addr;

    hiowrite32((u32)val,  (volatile void __iomem *)ip);
    hiowrite32(val >> 32, (volatile void __iomem *)(ip + 1));
}


static void pendq_write(volatile struct substream *ssp, uint32_t vix, dma_addr_t addr_data, dma_addr_t addr_meta, uint32_t handle)
{
    uint32_t head, head_raw;

    head_raw = hioread32(&ssp->pendq_hdr[vix].head);
    head     = head_raw & (HSI_MAX_QELEM - 1);

    hiowrite64(addr_data, &ssp->pendq[vix][head].addr_data);
    hiowrite64(addr_meta, &ssp->pendq[vix][head].addr_meta);
    hiowrite16(handle,    &ssp->pendq[vix][head].user_handle);
    hiowrite32(head_raw + 1, &ssp->pendq_hdr[vix].head);
}


static int snap_save(struct hsi_dev *dev, void *vaddr, uint32_t length)
{
    char name[64];

    snprintf(name,   sizeof(name),   "/tmp/%s%02d.bin", dev->name, dev->snapshot_last++);

    pr_info("Save off %d image bytes from vaddr=0x%px to file %s\n", length, vaddr, name);

    image_save(name, vaddr, length);

    return 0;
}


//--------------------------------------------------------------------------
// hsi_ioctl()
//--------------------------------------------------------------------------
static long hsi_ioctl(struct file *pFile, unsigned int cmd, unsigned long arg)
{
    struct hsi_ioctl io_arg, *parg = &io_arg;
    struct hsi_dev *dev = (struct hsi_dev *)pFile->private_data;
    struct hsi_buf *hsibuf;
    struct hsi_event *ev;
#ifdef __aarch64__
    struct h_node *hnp;
#endif
    volatile struct substream *ssp;
    unsigned long flags, ssix;
    int i, hix, vix, to, nslots, retval = 0;

    if (g_stand_down[dev->hix]) {
        pr_warn("ioctl in stad_down mode\n");
        return -ENODEV;
    }

    memset(&io_arg, 0, sizeof(io_arg));

    // Make sure we got a valid ioctl control struct.
    if (arg && (!access_ok((void *)arg, sizeof(io_arg)))) {
        pr_err("ioctl: bad io_arg address\n");
        return -EPERM;
    }

    // Get user's ioctl info.
    if (arg && copy_from_user(&io_arg, (void __user *)arg, sizeof(io_arg))) {
        return -EFAULT;
    }

    // Make sure the target substream is legit.
    ssix = parg->substream;
    if (ssix > (HSI_MAX_SUBSTREAMS - 1)) {
        pr_err("using illegal substream number: %ld\n", ssix);
        return -ENXIO;
    }
    ssp = dev->sub[ssix].ss;

    // Get the hydra this stream belongs to.
    hix = dev->hix;

    // And the virtual stream.
    vix = dev->vix;

    switch (cmd) {

        case HSI_UTRACE_0:
            trace_ml_hsi_utrace0(dev_name(dev->dev), parg->stats.ts[0], parg->stats.ts[1], parg->stats.ts[2], parg->stats.ts[3]);
            break;
        case HSI_UTRACE_1:
            trace_ml_hsi_utrace1(dev_name(dev->dev), parg->stats.ts[0], parg->stats.ts[1], parg->stats.ts[2], parg->stats.ts[3]);
            break;
        case HSI_UTRACE_2:
            trace_ml_hsi_utrace2(dev_name(dev->dev), parg->stats.ts[0], parg->stats.ts[1], parg->stats.ts[2], parg->stats.ts[3]);
            break;
        case HSI_UTRACE_3:
            trace_ml_hsi_utrace3(dev_name(dev->dev), parg->stats.ts[0], parg->stats.ts[1], parg->stats.ts[2], parg->stats.ts[3]);
            break;

        case HSI_DEV_IS_BUSY:
            if (dev->sub_count > 1) {
                int bs = vcam_is_busy();
                trace_ml_hsi_busy_check(dev->dix, bs);
                if (bs) {
                    pr_warn("vcam/dcprs is busy\n");
                    return -EBUSY;
                }
            }
            break;

        // Get device properties.
        case HSI_GET_DEV_PROPS:
            pr_info("ioctl(PROPS):: hix=%d, dix=%d, dsize=%d, msize=%d, subs=%d\n", hix, dev->dix, ssp->buf_size, ssp->meta_size, dev->sub_count);

            parg->props.substream_count = dev->sub_count;

            for (i = 0; i < dev->sub_count; i++) {
                DPRINT(1, "i=%d, dsize=%d, msize=%d\n", i, ssp->buf_size, ssp->meta_size);
                parg->props.substreams[i].slices        = ssp->slices;
                parg->props.substreams[i].size_data_buf = ssp->buf_size;
                parg->props.substreams[i].size_meta_buf = ssp->meta_size;
                parg->props.substreams[i].max_buf_count = ssp->buf_count_max;
            }
            break;

        // Wait for the next buffer completion event.
        case HSI_WAIT:
            trace_ml_hsi_ioctl_wait_event(dev->hix, dev->dix, dev_name(dev->dev));
            if (parg->rd_timeout_ms == 0) {
                to = wait_event_interruptible(dev->rdq_waitq, !(list_empty(&dev->rdq)));
            } else {
                to = wait_event_interruptible_timeout(dev->rdq_waitq, !(list_empty(&dev->rdq)), parg->rd_timeout_ms * HZ / 1000);
                if (to == 0) {
                    trace_ml_hsi_ioctl_wait_event_timeout(dev->hix, dev->dix);
                    return -ETIME;
                }
            }
            if (to < 0) {
                if (to != -ERESTARTSYS) { // when sensor app does ctrl-c
                    pr_err("wait error: %d\n", to);
                }
                return to;
            }

            parg->stats.ts[HSI_TS_WAIT_WAKE] = ktime_get_boottime_ns();

            // Pull off next event.
            ev = 0;
            spin_lock_irqsave(&dev->rdq_lock, flags);
            if (! list_empty(&dev->rdq)) {
                ev = list_first_entry(&dev->rdq, struct hsi_event, list);
                list_del_init(&ev->list);
            }
            spin_unlock_irqrestore(&dev->rdq_lock, flags);

            if (ev) {
                parg->size_data = ev->length;
                parg->addr_data = (uint64_t)ev->data;
                parg->ref       = ev->ref;
                parg->flags     = ev->flags;
                parg->slice     = ev->slice;
                parg->spills    = ev->spills;
                parg->status    = 0;

                parg->ss_data_length[0] = ev->ss_data_length[0];
                parg->ss_data_length[1] = ev->ss_data_length[1];
                parg->ss_meta_length[0] = ev->ss_meta_length[0];
                parg->ss_meta_length[1] = ev->ss_meta_length[1];

                parg->stats.ts[HSI_TS_SUBMIT] = ev->submit_ts;
                parg->stats.ts[HSI_TS_INTR] = ev->intr_ts;
                parg->stats.ts[HSI_TS_WAKE] = ev->wake_ts;

                if (ev->xstat) {
                    retval = -EIO;
                }

                // Snapshot support for non-vcam streams.
                if ((ev->flags == HSI_EVFLAG_DATA) && (dev->snapshot_count > 0) && (! ev->vp)) {
                    dev->snapshot_count--;
                    pr_info("Saving image data from physaddr=0x%px, offset=0x%lx\n", ev->data, ((uint8_t *)ev->data - (uint8_t *)ev->hnp_data->sh_paddr));
                    snap_save(dev, (uint8_t *)ev->hnp_data->sh_vaddr + ((uint8_t *)ev->data - (uint8_t *)ev->hnp_data->sh_paddr), ev->ss_data_length[0]);
                }

                if (ev->vp) {
                    int len, sval;
                    //uint32_t *wp = (uint32_t *)(ev->vp->buf_dst + ev->vp->off_dst);
                    sval = vcam_buf_decompress_end(&len);

                    if (ev->dcprs_err) {
                        retval = -EILSEQ;
                        parg->status |= HSI_STATUS_DCPRS_ERR;
                    }

                    // Support vcam snapshots.
                    if ((ev->flags == HSI_EVFLAG_DATA) && (dev->snapshot_count > 0)) {
                        dev->snapshot_count--;
                        ev->vp->size_dst = len;
                        vcam_save(ev->vp);
                    }

                    if ((ev->flags == HSI_EVFLAG_DATA) && ( (ev->dcprs_err) || (sval < 0) ) ) {
                        if (g_dfs_dcprs_img_save_count) {
                            g_dfs_dcprs_img_save_count--;
                            pr_err("Saving DCPRS bad input images!\n");
                            ev->vp->buf_dst = 0; // Disable saving of output buffer.
                            ev->vp->size_dst = 0;
                            vcam_save(ev->vp);
                        }
                    }

                    ev->vp->t_decomp_end = ktime_get_boottime_ns();

                    DPRINT(1, "ioctl(WAIT):: free vbuf vp=%px, ev=%px, diff=%lld ms\n", ev->vp, ev, (ev->vp->t_decomp_end - ev->vp->t_decomp_start) / 1000 / 1000);
                    vcam_buf_free(ev->vp);
                }

                trace_ml_hsi_ioctl_wake_event(dev->hix, dev->dix, dev_name(dev->dev), parg->addr_data, parg->slice, parg->size_data, parg->ref, ev, ev->evix);
                DPRINT(1, "ioctl(WAIT):: size=%d, addr_data=0x%llx, slice=%d, ref=0x%llx, ev=%px\n", parg->size_data, parg->addr_data, parg->slice, parg->ref, ev);
            } else {
                pr_err("empty list!\n");
                return -1;
            }

            // Free the event.
            hsi_ev_free(ev);

            break;

        // Submit a buffer(pair) to hydra for reading/writing.
        case HSI_SUBMIT_BUFS:

            // Allocate the new hsibuf.
            if (!(hsibuf = hsi_buf_alloc(dev, parg->ref))) {
                pr_err("no free hsi bufs!\n");
                return -1;
            }
            hsibuf->submit_ts = ktime_get_boottime_ns();

            // Initialize it.
            hsibuf->data_buf_len      = ssp->buf_size;
            hsibuf->meta_data_buf_len = ssp->meta_size;
            hsibuf->data              = (void *)parg->addr_data; // we're saving physaddr here
            hsibuf->meta_data         = (void *)parg->addr_meta; // ditto

#ifdef __aarch64__
            // Massage the addresses coming from user space from aperture address to SOC.
            if (parg->addr_meta) {
                parg->addr_meta -= 0x800000000ULL;
            }
            parg->addr_data -= 0x800000000ULL;

            DPRINT(1, "ioctl(SUB0):: hix=%d, dix=%d, Data_addr=0x%08llx, Meta_addr=0x%08llx, Sh_name_data=0x%llx, Sh_name_meta=0x%llx\n",
                   dev->hix, dev->dix, parg->addr_data, parg->addr_meta, parg->sh_name_data, parg->sh_name_meta);

            hsibuf->meta_io = parg->addr_meta;
            hsibuf->data_io = parg->addr_data;

            // Map out data buffers.
            hnp = shregion_map(hix, parg->sh_name_data);
            hsibuf->hnp_data           = hnp;
            hsibuf->dma_dst_buf_data   = hnp->dma_buf;
            hsibuf->sh_base_pa_io_data = hnp->sh_paddr_io;
            hsibuf->sh_base_pa_data    = hnp->sh_paddr;
            hsibuf->vaddr_dst_data     = hnp->sh_vaddr;

            // Map out metadata buffers.
            if (parg->sh_name_data != parg->sh_name_meta) {
                hnp = shregion_map(hix, parg->sh_name_meta);
            }
            hsibuf->hnp_meta           = hnp;
            hsibuf->dma_dst_buf_meta   = hnp->dma_buf;
            hsibuf->sh_base_pa_io_meta = hnp->sh_paddr_io;
            hsibuf->sh_base_pa_meta    = hnp->sh_paddr;
            hsibuf->vaddr_dst_meta     = hnp->sh_vaddr;
#else
            // For Raven compat....
            hsibuf->data_io = pci_map_single(g_hsi[hix].pdev, phys_to_virt(parg->addr_data), ssp->buf_size, DMA_FROM_DEVICE);
            if (parg->addr_meta) {
                hsibuf->meta_io = pci_map_single(g_hsi[hix].pdev, phys_to_virt(parg->addr_meta), ssp->meta_size, DMA_FROM_DEVICE);
            } else {
                hsibuf->meta_io = 0;
            }

            hsibuf->sh_base_pa_io_data = hsibuf->data_io;
            hsibuf->sh_base_pa_data    = hsibuf->data_io;
            hsibuf->vaddr_dst_data     = phys_to_virt(parg->addr_data);

            hsibuf->sh_base_pa_io_meta = hsibuf->meta_io;
            hsibuf->sh_base_pa_meta    = hsibuf->meta_io;
            hsibuf->vaddr_dst_meta     = phys_to_virt(parg->addr_meta);
#endif

            // Make sure that this particular device's pending queue has room for a new buffer.
            smp_rmb();
            spin_lock_irqsave(&dev->pendq_lock, flags);
            if ((nslots = CIRC_SPACE(hioread32(&ssp->pendq_hdr[vix].head), hioread32(&ssp->pendq_hdr[vix].tail), HSI_MAX_QELEM)) < 1) {
                spin_unlock_irqrestore(&dev->pendq_lock, flags);
                pr_err("no more room on pendq for hsibuf at %p\n", hsibuf);
                hsi_buf_free(hsibuf, 4);
                return -1;
            }

            // Handle case of vcam substreams.
            if (dev->sub_count > 1) {
                struct vcam_buf *vp;
                uint64_t sh_base_io;

                if ((vp = vcam_buf_alloc()) == 0) {
                    trace_ml_hsi_vcam_nobufs(dev->hix, dev->dix);
                    pr_err("%s: no vcam buf!\n", __func__);
                    break;
                }
                hsibuf->vp = vp;

                // Need to get the iommu address of the FMR5 page we were just handed.
                spin_unlock_irqrestore(&dev->pendq_lock, flags);
                get_shregion_cdp_dcprs_ioaddr(parg->sh_name_data, &sh_base_io);
                spin_lock_irqsave(&dev->pendq_lock, flags);
                vp->buf_dst_io = sh_base_io + (hsibuf->data_io - hsibuf->sh_base_pa_io_data);

                trace_ml_hsi_ioctl_submit(dev->hix, dev->dix, dev_name(dev->dev), parg->ref, parg->addr_data, parg->addr_meta, hsibuf, hsibuf->hsibuf_ix, vp, hioread32(&ssp->pendq_hdr[vix].head) & (HSI_MAX_QELEM - 1), HSI_MAX_QELEM - nslots);
                DPRINT(1, "ioctl(SUB1v):: hix=%d, dix=%d, Data_addr=0x%08llx, Meta_addr=0x%08llx, G_Data_Io=0x%llx, G_Data_Phys=0x%llx, RB_Data_Io=0x%llx, RB_Data_Phys=0x%llx, sz=%d, hsi=%px(%d), vcamp=%px\n",
                       dev->hix, dev->dix, parg->addr_data, parg->addr_meta, vp->buf_g_io, vp->buf_g_phys, vp->buf_rb_io, vp->buf_rb_phys, vp->size,
                       hsibuf, hsibuf->hsibuf_ix, vp);

                // Populate a pending queue entry with the G and RB buffers.
                pendq_write(ssp + 0, vix, vp->buf_g_phys,  hsibuf->meta_io, hsibuf->hsibuf_ix);
                pendq_write(ssp + 1, vix, vp->buf_rb_phys, hsibuf->meta_io, hsibuf->hsibuf_ix);
            } else {
                // Non-vcam!
                hsibuf->vp = 0;
                trace_ml_hsi_ioctl_submit(dev->hix, dev->dix, dev_name(dev->dev), parg->ref, parg->addr_data, parg->addr_meta, hsibuf, hsibuf->hsibuf_ix, 0, hioread32(&ssp->pendq_hdr[vix].head) & (HSI_MAX_QELEM - 1), HSI_MAX_QELEM - nslots);
                DPRINT(1, "ioctl(SUB1):: hix=%d, dix=%d, Data_addr = 0x%08llx, Meta_addr = 0x%08llx, hsi=%px(%d), head=%d\n",
                       dev->hix, dev->dix, parg->addr_data, parg->addr_meta, hsibuf, hsibuf->hsibuf_ix, hioread32(&ssp->pendq_hdr[vix].head) & (HSI_MAX_QELEM - 1));

                // Populate a pending queue entry with these buffers.
                pendq_write(ssp + 0, vix, hsibuf->data_io,  hsibuf->meta_io, hsibuf->hsibuf_ix);
            }
            spin_unlock_irqrestore(&dev->pendq_lock, flags);
            break;

        default:
            break;
    }

    // On a successful operation, update control struct.
    if (arg && copy_to_user((void __user *)arg, &io_arg, sizeof(io_arg))) {
        retval = -EFAULT;
    }

    return retval;
}


//--------------------------------------------------------------------------
// hsilog_read()  Dump spillway-based log.
//--------------------------------------------------------------------------
static ssize_t hsilog_read(struct file *filp, struct kobject *kobj,
                           struct bin_attribute *attr,
                           char *buf, loff_t off, size_t count)
{
    struct device *dev = container_of(kobj, struct device, kobj);
    struct hsi_dev *hdev = dev_get_drvdata(dev);

    // Check that we're not accessing past the end of the log buffer.
    if ((off + count) > attr->size) {
        pr_err("illegal read: off=%lld, size=%ld\n", off, attr->size);
        return -1;
    }

    dma_sync_single_for_cpu(&g_hsi[hdev->hix].pdev->dev, hdev->spillway_io + off, count, PCI_DMA_FROMDEVICE);
    memcpy(buf, hdev->spillway + off, count);

    return count;
}


//-------------------------------------------------------------------------------------------
// irq_show()  Show the irq for a device.
//-------------------------------------------------------------------------------------------
static ssize_t irq_show(struct device *dev,
                        struct device_attribute *attr, char *buf)
{
    char *bbuf = buf;
    struct hsi_dev *hdev = dev_get_drvdata(dev);

    buf += snprintf(buf, PAGE_SIZE, "%d\n", hdev->irq);

    return buf - bbuf;
}


//-------------------------------------------------------------------------------------------
// stats_show()  Show per/sensor stats.
//-------------------------------------------------------------------------------------------
static ssize_t stats_show(struct device *dev,
                          struct device_attribute *attr, char *buf)
{
    char *bbuf = buf;
    struct hsi_dev *hdev = dev_get_drvdata(dev);

    buf += snprintf(buf, PAGE_SIZE, "Interrupts         : %d\n", hdev->stats.ints);
    buf += snprintf(buf, PAGE_SIZE, "Frames (received)  : %d\n", hdev->stats.frames);
    buf += snprintf(buf, PAGE_SIZE, "Frames (dropped)   : %d\n", hdev->stats.frames_dropped);
    buf += snprintf(buf, PAGE_SIZE, "Frames (orphaned)  : %d\n", hdev->stats.frames_orphaned);
    buf += snprintf(buf, PAGE_SIZE, "Empty queue        : %d\n", hdev->stats.empty_queue);

    return buf - bbuf;
}


//-------------------------------------------------------------------------------------------
// snapshot_store()  Take a snaphot.
//-------------------------------------------------------------------------------------------
static ssize_t snapshot_store(struct device *dev,
                              struct device_attribute *attr, const char *buf, size_t count)
{
    struct hsi_dev *hdev = dev_get_drvdata(dev);
    uint32_t snaps;
    char *cur;

    snaps = simple_strtol(buf, &cur, 0);
    pr_info("Taking %d snapshots on hydra_num=%d, hsi_dev=%d\n", snaps, hdev->hix, hdev->dix);
    hdev->snapshot_count = snaps;

    return count;
}


//-------------------------------------------------------------------------------------------
// vcam_bpp_store()  Set vcam color depth.
//-------------------------------------------------------------------------------------------
static ssize_t bpp_store(struct device *dev,
                         struct device_attribute *attr, const char *buf, size_t count)
{
    uint32_t bpp;
    char *cur;

    bpp = simple_strtol(buf, &cur, 0);
    if ( (bpp == 8) || (bpp == 10) || (bpp == 12) || (bpp == 16) ) {
        g_vcam_bpp = bpp;
    } else {
        pr_err("unsupported vcam color depth: %d\n", bpp);
    }

    return count;
}


//-------------------------------------------------------------------------------------------
// vcam_bpp_show()  Show vcam color depth.
//-------------------------------------------------------------------------------------------
static ssize_t bpp_show(struct device *dev,
                        struct device_attribute *attr, char *buf)
{
    char *bbuf = buf;

    buf += snprintf(buf, PAGE_SIZE, "%d\n", g_vcam_bpp);

    return buf - bbuf;
}


//-------------------------------------------------------------------------------------------
// ignore_store()  Ignore this stream.
//-------------------------------------------------------------------------------------------
static ssize_t ignore_store(struct device *dev,
                            struct device_attribute *attr, const char *buf, size_t count)
{
    struct hsi_dev *hdev = dev_get_drvdata(dev);
    char *cur;

    hdev->ignore = simple_strtol(buf, &cur, 0);
    if (hdev->ignore) {
        disable_irq(hdev->irq);
    } else {
        enable_irq(hdev->irq);
    }

    return count;
}


//-------------------------------------------------------------------------------------------
// ignore_show()  Show ignorance. :)
//-------------------------------------------------------------------------------------------
static ssize_t ignore_show(struct device *dev,
                           struct device_attribute *attr, char *buf)
{
    struct hsi_dev *hdev = dev_get_drvdata(dev);
    char *bbuf = buf;

    buf += snprintf(buf, PAGE_SIZE, "irq%d ==> %d\n", hdev->irq, hdev->ignore);

    return buf - bbuf;
}


//-------------------------------------------------------------------------------------------
// streamtab_show()  Dump stream table.
//-------------------------------------------------------------------------------------------
static ssize_t streamtab_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    void *hydradev = dev_get_drvdata(dev);
    struct hsi_dev *hdev;
    int i, hix;
    char *bbuf = buf;

    if (!hydradev) {
        pr_err("streamtab_show: no device!\n");
        return 0;
    }

    // Which device?
    if (hydradev == g_hsi[0].hydradev) {
        hix = 0;
    } else if (hydradev == g_hsi[1].hydradev) {
        hix = 1;
    } else {
        pr_err("Bad hydradev\n");
        return 0;
    }

    for (i = 0; i < HSI_MAX_DEVS; i++) {
        hdev = &g_hsi[hix].devs[i];
        if (hdev->name[0]) {
            buf += snprintf(buf, PAGE_SIZE, "%c %02d: %22s %c    %3d    [%d]\n",
                            g_hsi[hix].dfs.clr_on_tick.active[i] ? '*':' ',
                            i,
                            hdev->name,
                            hdev->opened ? 'O':'C',
                            hdev->irq,
                            hdev->irq_per_sec);
        }
    }

    return buf - bbuf;
}


//-------------------------------------------------------------------------------------------
// hsitab_show()  Dump HSI table.
//-------------------------------------------------------------------------------------------
static ssize_t hsitab_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    char *bbuf = buf;
    void *hydradev = dev_get_drvdata(dev);
    int hix, ix, vix, sub;
    struct hsi_root *table;

    if (!hydradev) {
        pr_err("hsitab_show: no device!\n");
        return 0;
    }

    // Which device?
    if (hydradev == g_hsi[0].hydradev) {
        hix = 0;
    } else if (hydradev == g_hsi[1].hydradev) {
        hix = 1;
    } else {
        pr_err("Bad hydradev\n");
        return 0;
    }

    table = g_hsi[hix].hsi_root;

    if (table->hsi_magic != HSI_MAGIC) {
        buf += snprintf(buf, PAGE_SIZE, "Bad magic number: 0x%08X\n", table->hsi_magic);
        return buf - bbuf;
    }

    for (ix = 0; ix < HSI_MAX_STREAMS; ix++) {
        if (table->tab[ix].present == 1) {
            buf += snprintf(buf, PAGE_SIZE, "Device #%02d (%s)\n", ix, table->tab[ix].name);

            buf += snprintf(buf, PAGE_SIZE, "\tSubstream Count: %d\n", table->tab[ix].subs_count);

            // For each substream....
            for (sub = 0; sub < table->tab[ix].subs_count; sub++) {
                struct substream *ssp = &table->tab[ix].sub[sub];

                buf += snprintf(buf, PAGE_SIZE, "\tSubstream #%02d\n", sub);

                buf += snprintf(buf, PAGE_SIZE, "\t\tDMA Channel     : %d\n", ssp->chan_id);
                buf += snprintf(buf, PAGE_SIZE, "\t\tData Buffer Size: %d\n", ssp->buf_size);
                buf += snprintf(buf, PAGE_SIZE, "\t\tMeta Buffer Size: %d\n", ssp->meta_size);
                buf += snprintf(buf, PAGE_SIZE, "\t\tMax Entries     : %d\n", ssp->buf_count_max);
                buf += snprintf(buf, PAGE_SIZE, "\t\tSlices          : %d\n", ssp->slices);
                buf += snprintf(buf, PAGE_SIZE, "\t\tCompressed      : %s\n", ssp->compressed ? "yes" : "no");
                buf += snprintf(buf, PAGE_SIZE, "\t\tVirtual Streams : %d\n", ssp->vstream_count);

                if (ssp->vstream_count) {
                    for (vix = 0; vix < ssp->vstream_count; vix++) {
                        buf += snprintf(buf, PAGE_SIZE, "\t\t\t [%s]\n", ssp->vstream_name[vix]);
                        buf += snprintf(buf, PAGE_SIZE, "\t\t\t\tSpillway : 0x%016llX\n", ssp->spillway_buf_addr[vix]);
                    }
                } else {
                    buf += snprintf(buf, PAGE_SIZE, "\t\tSpillway        : 0x%016llX\n", ssp->spillway_buf_addr[0]);
                }
            }
        }
    }

    buf += snprintf(buf, PAGE_SIZE, "\n");

    return buf - bbuf;
}


static ssize_t dfs_hsi_probe_write(struct file *file, const char __user *data, size_t count, loff_t *ppos)
{
    struct hsi_dev *hdev = file->private_data;

    pr_info("hsi_probe: hdev=%px, hix=%d", hdev, hdev->hix);

    return count;
}


static int dfs_hsi_active_devs_show(struct seq_file *file, void *offset)
{
    struct hydra_dev *hdev = file->private;
    int dix;

    for (dix = 0; dix < HSI_MAX_DEVS; dix++) {
        if (hdev->dfs.clr_on_tick.active[dix]) {
            seq_printf(file, "%s\n", hdev->devs[dix].name);
        }
    }

    return 0;
}


static int dfs_hsi_active_devs_open(struct inode *inode, struct file *file)
{
    return single_open(file, dfs_hsi_active_devs_show, inode->i_private);
}
