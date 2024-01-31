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
#include <linux/aer.h>
#include <linux/workqueue.h>
#include <linux/slab.h>
#include <linux/ioctl.h>
#include <linux/interrupt.h>
#include <linux/sched.h>
#include <linux/mm.h>
#include <linux/delay.h>
#include <linux/tty.h>
#include <linux/tty_driver.h>
#include <linux/firmware.h>
#include <linux/debugfs.h>
#include <linux/pm_runtime.h>
#include <linux/version.h>
#include <linux/irq.h>
#include <linux/notifier.h>
#include <linux/wait.h>

#include <linux/cvip/cvip_plt/cvip_plt.h>

#include "mem_map.h"
#include "shmem_map.h"
#include "sram_map.h"
#include "hydra.h"
#include "nwl.h"
#include "hbcp.h"
#include "hsi.h"
#include "hsi_priv.h"
#include "wearable.h"
#include "vcam.h"
#include "pcie_descriptor.h"

#include "trace.h"
#include "hsi_trace.h"

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Magic Leap, Inc.");
MODULE_AUTHOR("imagyar@magicleap.com");
MODULE_DESCRIPTION("Hydra Interface");


// Main driver major & minor versions.
#define HYDRA_MODULE_MAJOR_VERSION   "1"
#define HYDRA_MODULE_MINOR_VERSION   "2"

#define ACP_SRAM_SIZE       (1024 * 1280)
#define DIM_MEM_SIZE        (1024 * 32)
#define SYSMEM_SRAM_SIZE    (1024 * 1024 * 4)
#define MLDMA_SRAM_SIZE     (1024 * 512)
#define MLDMA_SRAM_OFF      (0x00300000)
#define MLDMA_TS_SIZE       (PAGE_SIZE * 2)
#define DBWIN_SIZE          (1024 * 16)

#define HYDRA_A0_CHIP_ID    (0x03072019)
#define HYDRA_B0_CHIP_ID    (0x25072020)

#define SERIAL_SIZE             (12)
#define MAX_HYDRAS              (2)

// TODO(tshahaf): move these to common file with hydra bl
#define RSA_SIG_SIZE (256)
#define BYPASS_SERIALS (16)
#define BYPASS_MAX_FILE_SIZE    ((SERIAL_SIZE * BYPASS_SERIALS) + RSA_SIG_SIZE)

#define HYDRA_LAST_IRQ_VEC      (g_irq_min - 1)
#define HYDRA_HCP_IRQ           (HYDRA_LAST_IRQ_VEC)
#define HYDRA_CRIT_EV_IRQ       (HYDRA_LAST_IRQ_VEC - 1)

#define FW_NAME_LEN             (256)
#define ALOG_RING_LEN           (1024*512)
#define HYDRA_BOARD_ID_STR_LEN  (32)

#define SHDDR_SIZE              (1024 * 1024 * 2)


#define SERIAL_OFF_BOOT_VERS    (0)
#define SERIAL_OFF_BOARD_ID     (2)
#define SERIAL_OFF_CHIP_ID      (4)

#define HIX_NAME(hix)           ((hix == HIX_HYDRA_PRI) ? "hydra_p" : "'hydra_s")


static int   __init hydra_init(void);
static void  __exit hydra_uninit(void);
static int          hydra_probe(struct pci_dev *pPciDev, const struct pci_device_id *pPciEntry);
static void         hydra_remove(struct pci_dev *pPciDev);
static int          hydra_pm_suspend(struct device *device);
static int          hydra_pm_resume(struct device *device);
static int          hydra_pm_idle(struct device *device);
static pci_ers_result_t hydra_error_detected(struct pci_dev *pdev, pci_channel_state_t state);

static int hbcp_cmd_send(uint32_t hix, uint32_t vreg_off, uint8_t cmd, uint16_t *arg);

static void hydra_tmr_sec(struct timer_list *t);


uint32_t g_hydra_dbg_mask = 0;
uint64_t g_scm_base_addr_io, g_acp_base_addr_io;
uint32_t g_scm_len, g_acp_len;
uint32_t g_use_otp = 0;
uint32_t g_stand_down[2] = { 0, 0 };
void (*g_hcp_resp_handler)(uint32_t) = 0;

static ulong  g_msix     = 0;
static ulong  g_irq_min  = 16;
static ulong  g_irq_max  = 16;
static ulong  g_irq_dbg  = 0;
static ulong  g_irq_thr  = 0;
static ulong  g_irq_aff  = 0;
static ulong  g_skip_fw  = 0;
static ulong  g_fw_pcie  = 0;
static uint   g_alog_scrape = 0;

module_param_named(msix,     g_msix,     ulong,  0644);
module_param_named(irq_min,  g_irq_min,  ulong,  0644);
module_param_named(irq_max,  g_irq_max,  ulong,  0644);
module_param_named(irq_dbg,  g_irq_dbg,  ulong,  0644);
module_param_named(irq_thr,  g_irq_thr,  ulong,  0644);
module_param_named(irq_aff,  g_irq_aff,  ulong,  0644);
module_param_named(skip_fw,  g_skip_fw,  ulong,  0644);
module_param_named(fw_pcie,  g_fw_pcie,  ulong,  0644);

module_param_named(dbg_mask,    g_hydra_dbg_mask,  uint,  0644);
module_param_named(alog_scrape, g_alog_scrape,  uint,  0644);

static struct pci_device_id hydra_dev_tab[] = {
    {PCIE_ML_VID, PCIE_EP_HYDRA_MAIN_FW_DID,     PCI_ANY_ID, PCI_ANY_ID},
    {PCIE_ML_VID, PCIE_EP_HYDRA_BOOTLOADER_DID, PCI_ANY_ID, PCI_ANY_ID},
    {0},
};

MODULE_DEVICE_TABLE(pci, hydra_dev_tab);

static const struct pci_error_handlers hydra_err_handler = {
    .error_detected = hydra_error_detected,
#if 0
    .slot_reset = hns3_slot_reset,
    .reset_prepare = hns3_reset_prepare,
    .reset_done = hns3_reset_done,
#endif
};


static const struct dev_pm_ops hydra_pm_ops = {
    SET_RUNTIME_PM_OPS(hydra_pm_suspend, hydra_pm_resume, hydra_pm_idle)
};

static struct pci_driver g_hydra_drvr = {
    .name        = "Hydra",
    .id_table    = hydra_dev_tab,
    .probe       = hydra_probe,
    .remove      = hydra_remove,
    .err_handler = &hydra_err_handler,
    .driver.pm   = &hydra_pm_ops,
};

module_init(hydra_init);
module_exit(hydra_uninit);

// dbg_mask
static ssize_t dbg_mask_store(struct device *dev,
                              struct device_attribute *attr, const char *buf, size_t count);
static ssize_t dbg_mask_show(struct device *dev,
                             struct device_attribute *attr, char *buf);
static DEVICE_ATTR_RW(dbg_mask);

// stand_down
static ssize_t stand_down_store(struct device *dev,
                              struct device_attribute *attr, const char *buf, size_t count);
static ssize_t stand_down_show(struct device *dev,
                             struct device_attribute *attr, char *buf);
static DEVICE_ATTR_RW(stand_down);

// hbcp
static ssize_t hbcp_store(struct device *dev,
                          struct device_attribute *attr, const char *buf, size_t count);
static ssize_t hbcp_show(struct device *dev,
                         struct device_attribute *attr, char *buf);
static DEVICE_ATTR_RW(hbcp);

// mldma_log_trig
static ssize_t mldma_log_trig_store(struct device *dev,
                                    struct device_attribute *attr, const char *buf, size_t count);
static ssize_t mldma_log_trig_show(struct device *dev,
                                   struct device_attribute *attr, char *buf);
static DEVICE_ATTR_RW(mldma_log_trig);


// board_id
static ssize_t board_id_show(struct device *dev,
                           struct device_attribute *attr, char *buf);
static DEVICE_ATTR_RO(board_id);

// bl_vers
static ssize_t bl_vers_show(struct device *dev,
                           struct device_attribute *attr, char *buf);
static DEVICE_ATTR_RO(bl_vers);

// serial_tmp
static ssize_t serial_tmp_show(struct device *dev,
                           struct device_attribute *attr, char *buf);
static DEVICE_ATTR_RO(serial_tmp);

// serial_ltw
static ssize_t serial_ltw_show(struct device *dev,
                           struct device_attribute *attr, char *buf);
static DEVICE_ATTR_RO(serial_ltw);

// addr
static ssize_t addr_show(struct device *dev,
                         struct device_attribute *attr, char *buf);
static DEVICE_ATTR_RO(addr);

// speed
static ssize_t speed_show(struct device *dev,
                          struct device_attribute *attr, char *buf);
static DEVICE_ATTR_RO(speed);

// recoverable PCIe errors
static ssize_t pci_err_show(struct device *dev,
                            struct device_attribute *attr, char *buf);
static DEVICE_ATTR_RO(pci_err);

// critical event notification
static ssize_t crit_ev_store(struct device *dev,
                             struct device_attribute *attr, const char *buf, size_t count);
static ssize_t crit_ev_show(struct device *dev,
                            struct device_attribute *attr, char *buf);
static DEVICE_ATTR_RW(crit_ev);

// guc version
static ssize_t guc_version_show(struct device *dev,
                          struct device_attribute *attr, char *buf);
static DEVICE_ATTR_RO(guc_version);

// hydra watchdog control
static ssize_t watchdog_enb_store(struct device *dev,
                             struct device_attribute *attr, const char *buf, size_t count);
static ssize_t watchdog_enb_show(struct device *dev,
                            struct device_attribute *attr, char *buf);
static DEVICE_ATTR_RW(watchdog_enb);

// firmware (name of)
static ssize_t fw_name_store(struct device *dev,
                             struct device_attribute *attr, const char *buf, size_t count);
static ssize_t fw_name_show(struct device *dev,
                            struct device_attribute *attr, char *buf);
static DEVICE_ATTR_RW(fw_name);

// gac
static ssize_t gac_store(struct device *dev,
                         struct device_attribute *attr, const char *buf, size_t count);
static ssize_t gac_show(struct device *dev,
                        struct device_attribute *attr, char *buf);
static DEVICE_ATTR_RW(gac);

#if (DBWIN_SIZE > 0)
static ssize_t dbwin_ctrl_store(struct device *dev,
                                struct device_attribute *attr, const char *buf, size_t count);
static ssize_t dbwin_ctrl_show(struct device *dev,
                               struct device_attribute *attr, char *buf);
static DEVICE_ATTR_RW(dbwin_ctrl);
#endif

// test
static ssize_t test_store(struct device *dev,
                          struct device_attribute *attr, const char *buf, size_t count);
static ssize_t test_show(struct device *dev,
                         struct device_attribute *attr, char *buf);
static DEVICE_ATTR_RW(test);

// boot_stat
static ssize_t boot_stat_store(struct device *dev,
                          struct device_attribute *attr, const char *buf, size_t count);
static ssize_t boot_stat_show(struct device *dev,
                         struct device_attribute *attr, char *buf);
static DEVICE_ATTR_RW(boot_stat);

// pci_aer_enable
static ssize_t pci_aer_enable_store(struct device *dev,
                                    struct device_attribute *attr, const char *buf, size_t count);
static ssize_t pci_aer_enable_show(struct device *dev,
                                   struct device_attribute *attr, char *buf);
static DEVICE_ATTR_RW(pci_aer_enable);

// ready
static ssize_t ready_show(struct device *dev,
                         struct device_attribute *attr, char *buf);
static DEVICE_ATTR_RO(ready);


static struct attribute *hydra_attributes[] = {
    &dev_attr_dbg_mask.attr,
    &dev_attr_mldma_log_trig.attr,
    &dev_attr_serial_tmp.attr,
    &dev_attr_bl_vers.attr,
    &dev_attr_board_id.attr,
    &dev_attr_serial_ltw.attr,
    &dev_attr_addr.attr,
    &dev_attr_speed.attr,
    &dev_attr_pci_err.attr,
    &dev_attr_test.attr,
    &dev_attr_boot_stat.attr,
    &dev_attr_crit_ev.attr,
    &dev_attr_guc_version.attr,
    &dev_attr_pci_aer_enable.attr,
    &dev_attr_hbcp.attr,
    &dev_attr_watchdog_enb.attr,
    &dev_attr_fw_name.attr,
    &dev_attr_gac.attr,
    &dev_attr_stand_down.attr,
#if (DBWIN_SIZE > 0)
    &dev_attr_dbwin_ctrl.attr,
#endif
    NULL
};

static const struct attribute_group hydra_attr_group = {
    .attrs = hydra_attributes,
};


static ssize_t blog_read(struct file *filp, struct kobject *kobj,
                         struct bin_attribute *attr,
                         char *buf, loff_t off, size_t count);


static ssize_t alog_read(struct file *filp, struct kobject *kobj,
                         struct bin_attribute *attr,
                         char *buf, loff_t off, size_t count);

static ssize_t hcpmem_read(struct file *filp, struct kobject *kobj,
                           struct bin_attribute *attr,
                           char *buf, loff_t off, size_t count);

static int hcpmem_mmap(struct file *filp, struct kobject *kobj,
                       struct bin_attribute *attr, struct vm_area_struct *vma);

static struct bin_attribute attr_hcpmem = {
    .attr.name      = "hcpmem",
    .attr.mode      = S_IRUSR | S_IWUSR | S_IWGRP | S_IRGRP | S_IWOTH | S_IROTH,
    .read           = hcpmem_read,
    .mmap           = hcpmem_mmap,
    .size           = HCP_DATA_LEN
};


static ssize_t imumem_read(struct file *filp, struct kobject *kobj,
                           struct bin_attribute *attr,
                           char *buf, loff_t off, size_t count);

static int imumem_mmap(struct file *filp, struct kobject *kobj,
                       struct bin_attribute *attr, struct vm_area_struct *vma);

static struct bin_attribute attr_imumem = {
    .attr.name      = "imumem",
    .attr.mode      = S_IRUSR | S_IWUSR | S_IWGRP | S_IRGRP | S_IWOTH | S_IROTH,
    .read           = imumem_read,
    .mmap           = imumem_mmap,
    .size           = IMU_SHMEM_SIZE
};


#define FW_MAX_LENGTH   (1024 * 1024 * 2)
#define FW_SRAM_OFFSET  (0x200000)
static ssize_t firmware_write(struct file *filp, struct kobject *kobj,
                              struct bin_attribute *attr, char *buf, loff_t off, size_t count);

static ssize_t firmware_read(struct file *filp, struct kobject *kobj,
                             struct bin_attribute *attr, char *buf, loff_t off, size_t count);

static struct bin_attribute attr_firmware = {
    .attr.name = "firmware",
    .attr.mode = S_IRUSR | S_IWUSR | S_IWGRP | S_IRGRP | S_IWOTH | S_IROTH,
    .write = firmware_write,
    .read = firmware_read,
    .size = FW_MAX_LENGTH
};

#if (ACP_SRAM_SIZE > 0)
static ssize_t acpmem_read(struct file *filp, struct kobject *kobj,
                           struct bin_attribute *attr,
                           char *buf, loff_t off, size_t count);

static int acpmem_mmap(struct file *filp, struct kobject *kobj,
                       struct bin_attribute *attr, struct vm_area_struct *vma);

static struct bin_attribute attr_acpmem = {
    .attr.name = "acpmem",
    .attr.mode = S_IRUSR | S_IWUSR | S_IWGRP | S_IRGRP | S_IWOTH | S_IROTH,
    .read = acpmem_read,
    .mmap = acpmem_mmap,
    .size = ACP_SRAM_SIZE
};
#endif


#if (DIM_MEM_SIZE > 0)
static ssize_t dim_mem_read(struct file *filp, struct kobject *kobj,
                            struct bin_attribute *attr,
                            char *buf, loff_t off, size_t count);

static int dim_mem_mmap(struct file *filp, struct kobject *kobj,
                        struct bin_attribute *attr, struct vm_area_struct *vma);

static struct bin_attribute attr_dimmem = {
    .attr.name = "dimmermem",
    .attr.mode = S_IRUSR | S_IWUSR | S_IWGRP | S_IRGRP | S_IWOTH | S_IROTH,
    .read = dim_mem_read,
    .mmap = dim_mem_mmap,
    .size = DIM_MEM_SIZE
};
#endif


#if (DBWIN_SIZE > 0)
#if 0
static ssize_t dbwin_mem_read(struct file *filp, struct kobject *kobj,
                              struct bin_attribute *attr,
                              char *buf, loff_t off, size_t count);

static ssize_t dbwin_mem_write(struct file *filp, struct kobject *kobj,
                               struct bin_attribute *attr,
                               char *buf, loff_t off, size_t count);
#endif
static int dbwin_mmap(struct file *filp, struct kobject *kobj,
                      struct bin_attribute *attr, struct vm_area_struct *vma);
static struct bin_attribute attr_dbwin0_mem = {
    .attr.name = "dbwin0",
    .attr.mode = S_IRUSR | S_IWUSR | S_IWGRP | S_IRGRP | S_IWOTH | S_IROTH,
    .mmap = dbwin_mmap,
    .size = DBWIN_SIZE
};
static struct bin_attribute attr_dbwin1_mem = {
    .attr.name = "dbwin1",
    .attr.mode = S_IRUSR | S_IWUSR | S_IWGRP | S_IRGRP | S_IWOTH | S_IROTH,
    .mmap = dbwin_mmap,
    .size = DBWIN_SIZE
};
#endif


#if (MLDMA_SRAM_SIZE > 0)
static ssize_t mldmamem_read(struct file *filp, struct kobject *kobj,
                             struct bin_attribute *attr,
                             char *buf, loff_t off, size_t count);

static int mldmamem_mmap(struct file *filp, struct kobject *kobj,
                         struct bin_attribute *attr, struct vm_area_struct *vma);

static struct bin_attribute attr_mldmamem = {
    .attr.name = "mldma_log",
    .attr.mode = S_IRUSR | S_IWUSR | S_IWGRP | S_IRGRP | S_IWOTH | S_IROTH,
    .read = mldmamem_read,
    .mmap = mldmamem_mmap,
    .size = MLDMA_SRAM_SIZE
};
#endif

#if (SYSMEM_SRAM_SIZE > 0)
static ssize_t sysmem_read(struct file *filp, struct kobject *kobj,
                           struct bin_attribute *attr,
                           char *buf, loff_t off, size_t count);

static int sysmem_mmap(struct file *filp, struct kobject *kobj,
                       struct bin_attribute *attr, struct vm_area_struct *vma);

static struct bin_attribute attr_sysmem = {
    .attr.name = "sysmem",
    .attr.mode = S_IRUSR | S_IWUSR | S_IWGRP | S_IRGRP | S_IWOTH | S_IROTH,
    .read = sysmem_read,
    .mmap = sysmem_mmap,
    .size = SYSMEM_SRAM_SIZE
};
#endif


static ssize_t timestamp_read(struct file *filp, struct kobject *kobj,
                              struct bin_attribute *attr,
                              char *buf, loff_t off, size_t count);

static int timestamp_mmap(struct file *filp, struct kobject *kobj,
                          struct bin_attribute *attr, struct vm_area_struct *vma);


DECLARE_WAIT_QUEUE_HEAD(alog_ring_wait);

static struct bin_attribute attr_timestamp = {
    .attr.name = "timestampmem",
    .attr.mode = S_IRUSR | S_IWUSR | S_IWGRP | S_IRGRP | S_IWOTH | S_IROTH,
    .read = timestamp_read,
    .mmap = timestamp_mmap,
    .size = MLDMA_TS_SIZE
};


static struct bin_attribute attr_alog = {
    .attr.name      = "alog",
    .attr.mode      = S_IRUGO,
    .read           = alog_read,
    .write          = 0,
    .size           = CLOG_APP_SIZE,
};


static struct bin_attribute attr_blog = {
    .attr.name      = "blog",
    .attr.mode      = S_IRUGO,
    .read           = blog_read,
    .write          = 0,
    .size           = CLOG_BOOT_SIZE,
};


static struct bin_attribute *g_bin_tab[] = {
    &attr_alog,
    &attr_blog,
    &attr_hcpmem,
    &attr_imumem,
    &attr_firmware,
    &attr_timestamp,
#if (ACP_SRAM_SIZE > 0)
    &attr_acpmem,
#endif
#if (DIM_MEM_SIZE > 0)
    &attr_dimmem,
#endif
#if (DBWIN_SIZE > 0)
    &attr_dbwin0_mem,
    &attr_dbwin1_mem,
#endif
#if (MLDMA_SRAM_SIZE > 0)
    &attr_mldmamem,
#endif
#if (SYSMEM_SRAM_SIZE > 0)
    &attr_sysmem,
#endif
    0
};


static int alogx_open(struct inode *, struct file *);
static int alogx_release(struct inode *, struct file *);
static ssize_t alogx_read(struct file *pFile, char __user *, size_t count, loff_t *off);
static const struct file_operations fops_alogx = {
    .owner          = THIS_MODULE,
    .open           = alogx_open,
    .release        = alogx_release,
    .read           = alogx_read,
};

static ssize_t dfs_msi_send_write(struct file *file, const char __user *data, size_t count, loff_t *ppos);
static const struct file_operations dfs_msi_send_fops = {
    .open   = simple_open,
    .write  = dfs_msi_send_write,
    .llseek = generic_file_llseek,
};


static void crit_event_top_test(uint32_t hix);

// Device-global data.
static struct hydra_dev {
    uint32_t                version;
    uint32_t                index;

    // most recent Critical Event Notification
    uint32_t                crit_event_0;
    uint32_t                crit_event_1;

    struct device           *hydra_sys_dev;

    struct dentry           *ddir;

    struct cdev             cdev_hydra;

    uint32_t                this_proc_ix;
    uint32_t                this_proc;
    uint32_t                this_domain;

    unsigned long           intr_mask;

    struct {
        struct pci_dev          *pci_hydra;
        struct pci_dev          *pci_us;
        struct pci_dev          *pci_dse;
        struct pci_dev          *pci_dsi;

        void                    *hydra_sram_base_addr;
        void                    *hydra_sram_ioaddr;
        unsigned int            hydra_sram_ioaddr_size;

        void                    *hydra_aon_base_addr;
        void                    *hydra_aon_ioaddr;

        void                    *hydra_exp_core_base_addr;
        void                    *hydra_exp_core_ioaddr;
        unsigned int            hydra_exp_core_ioaddr_size;

        uint32_t                vdr_off;

        uint32_t                bl_vers;
        uint16_t                board_id;
        uint32_t                chip_id;

        uint32_t                irq_count;

        uint32_t                pci_err_count;

        struct msix_entry       *msix_tab;
        struct {
            uint32_t vector;
            void     *cookie;
        }                       irq_info[32];

        struct tasklet_struct   tsklt[HSI_MAX_STREAMS];

        struct workqueue_struct *workq;
        struct work_job {
            struct work_struct      work;
            int                     hix_to_probe;
            int                     intr_hix;
            int                     intr_irq;
            void                    *intr_id;
            void                    *stuff;
        } workq_job;
        struct work_job_alog {
            struct work_struct      work;
            int                     hix;
        } work_alog_job;
#if (ACP_SRAM_SIZE > 0)
        dma_addr_t   acp_bus_addr_align;
        dma_addr_t   acp_bus_addr;
        void         *acp_addr;
        void         *acp_addr_align;
#endif
#if (DIM_MEM_SIZE > 0)
        dma_addr_t   dim_bus_addr_align;
        dma_addr_t   dim_bus_addr;
        void         *dim_addr;
        void         *dim_addr_align;
#endif
    struct sram_map             *h_sram;
    } pci;

    int                         hsi_probe_done;
    uint8_t                     alog_tmp_buff[CLOG_APP_SIZE];
    uint32_t                    alog_tail;
    char                        alog_buff[ALOG_RING_LEN];
    uint32_t                    alog_buff_ptr;
    char                        fw_name[FW_NAME_LEN];

} g_dev[MAX_HYDRAS];

struct alogx_priv {
    struct hydra_dev *dev;
    uint32_t current_off;
};

struct alog {
    struct class       *class;
    struct cdev         cdev;
    struct device       *dev;
    dev_t               devt;
} g_alog[MAX_HYDRAS];

static struct blocking_notifier_head g_notifiers;

static struct {
    dma_addr_t              fw_bus_addr;
    dma_addr_t              fw_bus_addr_align;
    void                    *fw_addr;
    void                    *fw_addr_align;
    uint32_t                fw_length;
} g_fw[MAX_HYDRAS];

static int load_fw(uint32_t hix, int have_name);

// TODO(imagyar): The shddr vars below will need to be changed to per-device versions to support multiple Hydras.
dma_addr_t              g_shddr_bus_addr;
dma_addr_t              g_shddr_bus_addr_align;
void                    *g_shddr_addr;
void                    *g_shddr_addr_align;
uint32_t                g_shddr_length;

// Debugfs stuff.
#ifdef CONFIG_DEBUG_FS
struct dentry *g_hydra_dev_ddir;
static uint32_t g_dfs_hz = HZ;
#endif

static struct timer_list g_timer_sec;
static int               g_timer_count;

// TODO(imagyar): Find a cleaner way to export this that works for ML20 & ML21.
struct shmem_map        *g_shddr_map;
EXPORT_SYMBOL(g_shddr_map);

static void disable_all_irqs(uint32_t hix);

//-------------------------------------------------------------------------------------------
// hydra_timer_start()  Initialize and start the 1-second timer.
//-------------------------------------------------------------------------------------------
static void hydra_timer_start(void)
{
    if (g_timer_count == 0) {
        pr_info("start timer\n");
        timer_setup(&g_timer_sec, hydra_tmr_sec, 0);
        g_timer_sec.expires = jiffies + HZ;
        add_timer(&g_timer_sec);
        g_timer_count++;
    }
}


//-------------------------------------------------------------------------------------------
// hydra_timer_stop()  Stop the 1-second timer.
//-------------------------------------------------------------------------------------------
static void hydra_timer_stop(void)
{
    if (g_timer_count == 1) {
        pr_info("stop timer\n");
        del_timer_sync(&g_timer_sec);
        g_timer_count--;
    }
}


//-------------------------------------------------------------------------------------------
// hydra_error_detected()  Called on bus error.
//-------------------------------------------------------------------------------------------
static pci_ers_result_t hydra_error_detected(struct pci_dev *pdev, pci_channel_state_t state)
{
    struct hydra_dev *dev = pci_get_drvdata(pdev);

    dev->pci.pci_err_count++;

    sysfs_notify(&dev->hydra_sys_dev->kobj, NULL, "pci_err");

    if (state == pci_channel_io_perm_failure) {

        pr_err("UNRECOVERABLE PCIe error detected, inst=%d, state=%d, count=%d\n", dev->index, state, dev->pci.pci_err_count);

        return PCI_ERS_RESULT_DISCONNECT;
    }

    pr_err("RECOVERABLE PCIe error detected, inst=%d, state=%d, count=%d\n", dev->index, state, dev->pci.pci_err_count);

    return PCI_ERS_RESULT_NONE;
}


//-------------------------------------------------------------------------------------------
// mldma_dump()  Dump the mldma log for the specified device.
//-------------------------------------------------------------------------------------------
static int mldma_dump(uint32_t hix, uint32_t mldma_ix)
{
    struct sram_map *sram;
    struct file *file;
    loff_t pos = 0;
    ssize_t ret;

    sram = (struct sram_map *)g_dev[hix].pci.hydra_sram_base_addr;

    if (sram->mldma_log[mldma_ix].offset && sram->mldma_log[mldma_ix].size) {
        char fname[256];

        snprintf(fname, sizeof(fname), "/data/mldma%d_%c.core", mldma_ix, (hix == 0) ? 'p' : 's');

        file = filp_open(fname, O_RDWR | O_CREAT | O_TRUNC, 0600);
        if (IS_ERR(file)) {
            pr_err("filp_open(%s) for mldma log failed\n", fname);
            return -ENODEV;
        }

        pr_info("Writing %d bytes of mldma(%d) log to %s\n", sram->mldma_log[mldma_ix].size, mldma_ix, fname);

        ret = kernel_write(file,
                           (uint8_t *)g_dev[hix].pci.hydra_sram_base_addr + sram->mldma_log[mldma_ix].offset,
                           sram->mldma_log[mldma_ix].size,
                           &pos);
        if (ret != sram->mldma_log[mldma_ix].size) {
            pr_err("failed to write mldma(%d) log: %ld\n", mldma_ix, ret);
        }
        filp_close(file, NULL);
    } else {
        pr_err("mldma(%d) log is empty\n", mldma_ix);
    }

    return 0;
}


//-------------------------------------------------------------------------------------------
// get_bl_version()  Get bootloader version from serial number.
//-------------------------------------------------------------------------------------------
static int get_bl_version(uint32_t hix)
{
    uint32_t pos;
    uint8_t bl_vers;

    if (! g_dev[hix].pci.pci_hydra) {
        pr_err("get_bl_version: no device\n");
        return 0;
    }

    pos = pci_find_ext_capability(g_dev[hix].pci.pci_hydra, PCI_EXT_CAP_ID_DSN);
    pci_read_config_byte(g_dev[hix].pci.pci_hydra, pos + 4 + SERIAL_OFF_BOOT_VERS, &bl_vers);

    return bl_vers;
}


//-------------------------------------------------------------------------------------------
// get_board_id()  Get board id from serial number.
//-------------------------------------------------------------------------------------------
static int get_board_id(uint32_t hix)
{
    uint32_t pos;
    uint16_t bid, bid_prop;
    char board_id[HYDRA_BOARD_ID_STR_LEN];
    char *cur;

    if (!g_dev[hix].pci.pci_hydra) {
        pr_err("get_board_id: no device\n");
        return 0;
    }

    pos = pci_find_ext_capability(g_dev[hix].pci.pci_hydra, PCI_EXT_CAP_ID_DSN);
    pci_read_config_word(g_dev[hix].pci.pci_hydra, pos + 4 + SERIAL_OFF_BOARD_ID, &bid);
    // Let's see if the board id has been overridden
    board_id[0] = '0';
    cvip_plt_property_get_safe("persist.kernel.hydra.board_id.override", board_id, sizeof(board_id), "0");
    if (board_id[0] != '0') {
        // We got the property value
        bid_prop = (uint16_t) simple_strtol(board_id, &cur, 0);
        if (bid_prop != 0) {
            pr_info("Overriding the board id from [0x%04X] to [0x%04X]", bid, bid_prop);
            bid = bid_prop;
        }
    }

    return bid;
}

//-------------------------------------------------------------------------------------------
// get_chip_id()  Get chip id from serial number.
//-------------------------------------------------------------------------------------------
static int get_chip_id(uint32_t hix)
{
    uint32_t pos;
    uint32_t cid;

    if (!g_dev[hix].pci.pci_hydra) {
        pr_err("get_chip_id: no device\n");
        return 0;
    }

    pos = pci_find_ext_capability(g_dev[hix].pci.pci_hydra, PCI_EXT_CAP_ID_DSN);
    pci_read_config_dword(g_dev[hix].pci.pci_hydra, pos + 4 + SERIAL_OFF_CHIP_ID, &cid);

    return cid;
}


//--------------------------------------------------------------------------
// hydra_aon_scratch_get()
//--------------------------------------------------------------------------
static void hydra_aon_scratch_get(uint32_t hydra_ix, uint32_t *val)
{
    uint8_t *scratch_base = NULL;

    if (g_dev[hydra_ix].pci.chip_id == HYDRA_B0_CHIP_ID) {
        scratch_base = (uint8_t *)g_dev[hydra_ix].pci.hydra_aon_base_addr + 0x1000 + 0x3b4;
    } else {
        pr_err("Unknown Hydra chip id\n");
        return;
    }

    if (g_dev[hydra_ix].pci.hydra_aon_base_addr) {
        *val = *((uint32_t *)scratch_base);
    } else {
        pr_err("Unable to get hydra scratch reg!\n");
    }
}


//--------------------------------------------------------------------------
// hydra_aon_scratch_set()
//--------------------------------------------------------------------------
static void hydra_aon_scratch_set(uint32_t hydra_ix, uint32_t val)
{
    uint8_t *scratch_base = NULL;

    if (g_dev[hydra_ix].pci.chip_id == HYDRA_A0_CHIP_ID) {
        scratch_base = (uint8_t *)g_dev[hydra_ix].pci.hydra_aon_base_addr + 0x100; //aon_msi_scratch_reg
    } else if (g_dev[hydra_ix].pci.chip_id == HYDRA_B0_CHIP_ID) {
        scratch_base = (uint8_t *)g_dev[hydra_ix].pci.hydra_aon_base_addr + 0x28; //aon_msi_scratch0_reg
    } else {
        pr_err("Unknown Hydra chip id\n");
        return;
    }

    if (g_dev[hydra_ix].pci.hydra_aon_base_addr) {
        *((uint32_t *)scratch_base) = val;
    } else {
        pr_err("Unable to set hydra scratch reg!\n");
    }
}

//-------------------------------------------------------------------------------------------
// dbg_mask_store()  Set debug mask.
//-------------------------------------------------------------------------------------------
static ssize_t dbg_mask_store(struct device *dev,
                              struct device_attribute *attr,
                              const char *buf, size_t count)
{
    char *cur;
    uint32_t x = simple_strtol(buf, &cur, 0);

    pr_info("Changing debug level to 0x%08x\n", x);

    g_hydra_dbg_mask = x;

    return count;
}


//-------------------------------------------------------------------------------------------
// ready_show()  Show debug mask.
//-------------------------------------------------------------------------------------------
static ssize_t ready_show(struct device *dev,
                             struct device_attribute *attr, char *buf)
{
    char *bbuf = buf;

    buf += snprintf(buf, PAGE_SIZE, "%d\n", 1);

    return buf - bbuf;
}


//-------------------------------------------------------------------------------------------
// dbg_mask_show()  Show debug mask.
//-------------------------------------------------------------------------------------------
static ssize_t dbg_mask_show(struct device *dev,
                             struct device_attribute *attr, char *buf)
{
    char *bbuf = buf;

    buf += snprintf(buf, PAGE_SIZE, "%d\n", g_hydra_dbg_mask);

    return buf - bbuf;
}


//-------------------------------------------------------------------------------------------
// stand_down_store()  Warn driver that device is about to be unplugged.
//-------------------------------------------------------------------------------------------
static ssize_t stand_down_store(struct device *dev,
                              struct device_attribute *attr,
                              const char *buf, size_t count)
{
    struct hydra_dev *hdev = dev_get_drvdata(dev);
    uint32_t x;
    char *cur;

    if (!hdev) {
        pr_err("%s: no device!\n", __func__);
        return -EIO;
    }

    x   = simple_strtol(buf, &cur, 0);

    if (x) {
        g_stand_down[0] = x;
        g_stand_down[1] = x;
        smp_wmb();

        // For now, this is all the prep we need to do.
        // There'll probably be more in the future though...
        hydra_timer_stop();

        pr_info("disable interrupts\n");
        disable_all_irqs(0);
        disable_all_irqs(1);
        blocking_notifier_call_chain(&g_notifiers, 0, (void *)0);
        blocking_notifier_call_chain(&g_notifiers, 1, (void *)0);
    }

    return count;
}


//-------------------------------------------------------------------------------------------
// stand_down_show()  Show unplug preparation status.
//-------------------------------------------------------------------------------------------
static ssize_t stand_down_show(struct device *dev,
                             struct device_attribute *attr, char *buf)
{
    struct hydra_dev *hdev = dev_get_drvdata(dev);
    char *bbuf = buf;

    buf += snprintf(buf, PAGE_SIZE, "%d\n", g_stand_down[hdev->index]);

    return buf - bbuf;
}


//-------------------------------------------------------------------------------------------
// hbcp_store()  Send an HBCP command to the bootloader.
//-------------------------------------------------------------------------------------------
static ssize_t hbcp_store(struct device *dev,
                          struct device_attribute *attr,
                          const char *buf, size_t count)
{
    struct hydra_dev *hdev = dev_get_drvdata(dev);
    uint16_t vdr_arg;
    char *cur;
    uint32_t reg = 0;
    uint32_t x = simple_strtol(buf, &cur, 0);

    if (!hdev) {
        pr_err("hbcp_store: no device!\n");
        return -EIO;
    }

    pci_read_config_dword(hdev->pci.pci_hydra, hdev->pci.vdr_off + 8, &reg);
    if (reg & HBCP_E_MODE_FW_BOOT) {
        pr_err("HBCP command not sent, hydra not in bootloader mode");
        return -EINVAL;
    }

    pr_info("Sending HBCP command %d", x);

    vdr_arg = 0;
    if (hbcp_cmd_send(hdev->index, hdev->pci.vdr_off + 8, x & 0xff, &vdr_arg) < 0) {
        pr_err("HBCP error");
    }

    return count;
}


//-------------------------------------------------------------------------------------------
// hbcp_show()  Show HBCP status.
//-------------------------------------------------------------------------------------------
static ssize_t hbcp_show(struct device *dev,
                         struct device_attribute *attr, char *buf)
{
    struct hydra_dev *hdev = dev_get_drvdata(dev);
    char *bbuf = buf;
    uint32_t reg = 0;

    if (!hdev) {
        pr_err("hbcp_show: no device!\n");
        return -EIO;
    }

    pci_read_config_dword(hdev->pci.pci_hydra, hdev->pci.vdr_off + 8, &reg);
    buf += snprintf(buf, PAGE_SIZE, "0x%x\n", reg);

    return buf - bbuf;
}


//-------------------------------------------------------------------------------------------
// pci_aer_enable_store()  Toggle PCIe AER upstream error reporting.
//-------------------------------------------------------------------------------------------
static ssize_t pci_aer_enable_store(struct device *dev,
                                    struct device_attribute *attr,
                                    const char *buf, size_t count)
{
    struct hydra_dev *hdev = dev_get_drvdata(dev);
    struct pci_dev *pPciDev;
    char *cur;
    uint32_t x = simple_strtol(buf, &cur, 0);

    if (!hdev) {
        pr_err("pci_aer_enable_store: no device!\n");
        return -EIO;
    }

    pPciDev = hdev->pci.pci_hydra;

    if (x) {
        pr_info("Enabling AER reporting on %s", HIX_NAME(hdev->index));

        // Enable upstream error reporting.
        pci_enable_pcie_error_reporting(pPciDev);
        pci_enable_pcie_error_reporting(pci_upstream_bridge(pPciDev));
        pci_enable_pcie_error_reporting(pci_upstream_bridge(pci_upstream_bridge(pPciDev)));
    } else {
        pr_info("Disabling AER reporting on %s", HIX_NAME(hdev->index));

        // Disable upstream error reporting.
        pci_disable_pcie_error_reporting(pPciDev);
        pci_disable_pcie_error_reporting(pci_upstream_bridge(pPciDev));
        pci_disable_pcie_error_reporting(pci_upstream_bridge(pci_upstream_bridge(pPciDev)));;
    }

    return count;
}


//-------------------------------------------------------------------------------------------
// pci_aer_enable_show()  Show AER reporting status.
//-------------------------------------------------------------------------------------------
static ssize_t pci_aer_enable_show(struct device *dev,
                                   struct device_attribute *attr, char *buf)
{
    struct hydra_dev *hdev = dev_get_drvdata(dev);
    struct pci_dev *pPciDev;
    char *bbuf = buf;
    u16 reg16 = 0;
    int pos;

    if (!hdev) {
        pr_err("pci_aer_enable_show: no device!\n");
        return -EIO;
    }

    pPciDev = hdev->pci.pci_hydra;

    pos = pci_find_ext_capability(pPciDev, PCI_EXT_CAP_ID_ERR);
    if (!pos) {
        return -EIO;
    }

    pos = pci_pcie_cap(pPciDev);
    if (!pos) {
        return -EIO;
    }

    pci_read_config_word(pPciDev, pos + PCI_EXP_DEVCTL, &reg16);

    buf += snprintf(buf, PAGE_SIZE, "%x\n", reg16 & (PCI_EXP_DEVCTL_CERE | PCI_EXP_DEVCTL_NFERE | PCI_EXP_DEVCTL_FERE | PCI_EXP_DEVCTL_URRE));

    return buf - bbuf;
}


//-------------------------------------------------------------------------------------------
// mldma_log_trig_store()  Dump an mldma log.
//-------------------------------------------------------------------------------------------
static ssize_t mldma_log_trig_store(struct device *dev,
                                    struct device_attribute *attr,
                                    const char *buf, size_t count)
{
    struct hydra_dev *hdev = dev_get_drvdata(dev);
    char *cur;
    uint32_t hix, x;

    if (!hdev) {
        pr_err("mldma_log_trig_store: no device!\n");
        return -EIO;
    }
    hix = hdev->index;

    x = simple_strtol(buf, &cur, 0);
    if (!g_dev[hix].pci.pci_hydra) {
        pr_err("mldma_log_trig_store: no device [2]!\n");
        return -1;
    }

    if (x >= MLDMA_COUNT) {
        pr_err("illegal mldma device: %d\n", x);
        return -1;
    }

    mldma_dump(hix, x);

    return count;
}


//-------------------------------------------------------------------------------------------
// mldma_log_trig_show()
//-------------------------------------------------------------------------------------------
static ssize_t mldma_log_trig_show(struct device *dev,
                                   struct device_attribute *attr, char *buf)
{
    return 0;
}


//-------------------------------------------------------------------------------------------
// watchdog_enb_store()
//-------------------------------------------------------------------------------------------
static ssize_t watchdog_enb_store(struct device *dev,
                             struct device_attribute *attr,
                             const char *buf, size_t count)
{

    struct hydra_dev *hdev = dev_get_drvdata(dev);
    char *cur;
    uint32_t hix, x;

    if (!hdev) {
        pr_err("%s: no device!\n", __func__);
        return -EIO;
    }
    hix = hdev->index;
    x = simple_strtol(buf, &cur, 0);

    g_shddr_map->hydra[hix].hydra_wd_enable = x;
    smp_wmb();

    return count;
}


//-------------------------------------------------------------------------------------------
// watchdog_enb_show()
//-------------------------------------------------------------------------------------------
static ssize_t watchdog_enb_show(struct device *dev,
                            struct device_attribute *attr, char *buf)
{
    struct hydra_dev *hdev = dev_get_drvdata(dev);
    char *bbuf = buf;
    int hix;

    hix = hdev->index;
    buf += snprintf(buf, PAGE_SIZE, "%d\n", g_shddr_map->hydra[hix].hydra_wd_enable);

    return buf - bbuf;
}


//-------------------------------------------------------------------------------------------
// fw_name_store()
//-------------------------------------------------------------------------------------------
static ssize_t fw_name_store(struct device *dev,
                             struct device_attribute *attr,
                             const char *buf, size_t count)
{
    struct hydra_dev *hdev = dev_get_drvdata(dev);
    int hix = hdev->index;
    uint16_t vdr_arg = 0;

    if (count >= sizeof(hdev->fw_name)) {
        pr_err("Firmware filename too long!\n");
        return count;
    }

    memset(hdev->fw_name, 0, sizeof(hdev->fw_name));

    strncpy(hdev->fw_name, buf, count - 1);

    // Load specified firmware.
    if (load_fw(hix, 1) == 0) {
        g_shddr_map->hydra[hix].fw_io_addr = g_fw[hix].fw_bus_addr_align;
        g_shddr_map->hydra[hix].fw_length  = g_fw[hix].fw_length;
        pr_info("Loaded %lld bytes of firmware at 0x%016llx\n", g_shddr_map->hydra[hix].fw_length, g_shddr_map->hydra[hix].fw_io_addr);

        // Send Boot command.
        smp_wmb();
        hbcp_cmd_send(hix, g_dev[hix].pci.vdr_off + 8, HBCP_CMD_BOOT, &vdr_arg);
    }

    return count;
}


//-------------------------------------------------------------------------------------------
// fw_name_show()
//-------------------------------------------------------------------------------------------
static ssize_t fw_name_show(struct device *dev,
                            struct device_attribute *attr, char *buf)
{
    struct hydra_dev *hdev = dev_get_drvdata(dev);
    char *bbuf = buf;

    buf += snprintf(buf, strlen(hdev->fw_name), "%s\n", hdev->fw_name);

    return buf - bbuf;
}


//-------------------------------------------------------------------------------------------
// gac_store()
//-------------------------------------------------------------------------------------------
static ssize_t gac_store(struct device *dev,
                         struct device_attribute *attr,
                         const char *buf, size_t count)
{
    return -EPERM;
}


static uint64_t timestamp_b0_rescale(uint64_t ts)
{
    static const unsigned int scale_factor = 125; //(32 / 32.768) * (1 << 7)
    static const unsigned int shift_bits = 7;

    uint64_t low = ts & 0xffffffff;
    uint64_t high = ts >> 32;
    uint32_t hi_carry = 0;

    low *= scale_factor;
    high *= scale_factor;
    low >>= shift_bits;

    hi_carry = (high & ((1 << shift_bits) - 1)) << (32 - shift_bits);
    low += hi_carry;
    high >>= shift_bits;

    //low might contain some high bits now
    return (high << 32) + low;
}

//-------------------------------------------------------------------------------------------
// gac_show()
//-------------------------------------------------------------------------------------------
static ssize_t gac_show(struct device *dev,
                        struct device_attribute *attr, char *buf)
{
    struct hydra_dev *hdev = dev_get_drvdata(dev);
    uint8_t *tstamper_base;
    char *bbuf = buf;
    uint64_t ts;
    unsigned int tstamper_offset = 0;
    int b0 = 0;

    if (!hdev) {
        pr_err("_store: no device!\n");
        return -EIO;
    }

    if (g_dev[hdev->index].pci.chip_id == HYDRA_A0_CHIP_ID) {
        tstamper_offset = 0xD0;
        b0 = 0;
    } else if (g_dev[hdev->index].pci.chip_id == HYDRA_B0_CHIP_ID) {
        tstamper_offset = 0xF8;
        b0 = 1;
    } else {
        pr_err("Unknown Hydra chip id\n");
        return -EIO;
    }
    // Try and fetch the timestamp in a single PCIe read operation.
    // This bit is ugly but temporary, so I'll leave it as is.
    // To make what we're doing slightly clearer though:
    //
    //     The 8KB region starting at hydra_aon_base_addr maps a swiss-cheesed version of Hydra AXI space, like so:
    //
    //     hydra_aon_base_addr + 0x0000 := Hydra AXI address of 0x097000.
    //     hydra_aon_base_addr + 0x1000 := Hydra AXI address of 0x4FC000.
    //
    tstamper_base = (uint8_t *)g_dev[hdev->index].pci.hydra_aon_base_addr + 0x1000;
    ts = *(uint64_t *)&tstamper_base[tstamper_offset];

    if (b0) {
        ts = timestamp_b0_rescale(ts);
    }

    buf += snprintf(buf, PAGE_SIZE, "%016llX\n", ts);

    return buf - bbuf;
}


//-------------------------------------------------------------------------------------------
// boot_stat_store()
//-------------------------------------------------------------------------------------------
static ssize_t boot_stat_store(struct device *dev,
                          struct device_attribute *attr,
                          const char *buf, size_t count)
{
    struct hydra_dev *hdev = dev_get_drvdata(dev);
    uint32_t val = 0;
    char *cur;

    if (!hdev) {
        pr_err("%s: no device!\n", __func__);
        return -EIO;
    }

    val = simple_strtol(buf, &cur, 0);

    hydra_aon_scratch_set(hdev->index, val);

    return count;
}


//-------------------------------------------------------------------------------------------
// boot_stat_show()
//-------------------------------------------------------------------------------------------
static ssize_t boot_stat_show(struct device *dev,
                         struct device_attribute *attr, char *buf)
{
    struct hydra_dev *hdev = dev_get_drvdata(dev);
    char *bbuf = buf;
    uint32_t val = 0;

    if (!hdev) {
        pr_err("%s: no device!\n", __func__);
        return -EIO;
    }
    hydra_aon_scratch_get(hdev->index, &val);

    buf += snprintf(buf, PAGE_SIZE, "0x%08X\n", val);

    return buf - bbuf;
}


//-------------------------------------------------------------------------------------------
// test_store()
//-------------------------------------------------------------------------------------------
static ssize_t test_store(struct device *dev,
                          struct device_attribute *attr,
                          const char *buf, size_t count)
{
    struct hydra_dev *hdev = dev_get_drvdata(dev);

    if (!hdev) {
        pr_err("test_store: no device!\n");
        return -EIO;
    }

    pr_info("test_store!\n");

    // Wake up the device.
    pm_runtime_get_sync(&g_dev[hdev->index].pci.pci_hydra->dev);

    pr_err("Bump pci err count!");

    hdev->pci.pci_err_count++;

    sysfs_notify(&hdev->hydra_sys_dev->kobj, NULL, "pci_err");

    pm_runtime_put_sync(&g_dev[hdev->index].pci.pci_hydra->dev);

    return count;
}


//-------------------------------------------------------------------------------------------
// bl_vers_show()
//-------------------------------------------------------------------------------------------
static ssize_t bl_vers_show(struct device *dev,
                         struct device_attribute *attr, char *buf)
{
    struct hydra_dev *hdev = dev_get_drvdata(dev);
    char *bbuf = buf;

    if (!hdev) {
        pr_err("%s: no device!\n", __func__);
        return -EIO;
    }

    buf += snprintf(buf, PAGE_SIZE, "0x%X\n", g_dev[hdev->index].pci.bl_vers);

    return buf - bbuf;
}


//-------------------------------------------------------------------------------------------
// board_id_show()
//-------------------------------------------------------------------------------------------
static ssize_t board_id_show(struct device *dev,
                         struct device_attribute *attr, char *buf)
{
    struct hydra_dev *hdev = dev_get_drvdata(dev);
    char *bbuf = buf;

    if (!hdev) {
        pr_err("%s: no device!\n", __func__);
        return -EIO;
    }

    buf += snprintf(buf, PAGE_SIZE, "0x%04X\n", g_dev[hdev->index].pci.board_id);

    return buf - bbuf;
}


//-------------------------------------------------------------------------------------------
// test_show()
//-------------------------------------------------------------------------------------------
static ssize_t test_show(struct device *dev,
                         struct device_attribute *attr, char *buf)
{
    return 0;
}


//-------------------------------------------------------------------------------------------
// serial_tmp_show()  Get temple serial number.
//-------------------------------------------------------------------------------------------
static ssize_t serial_tmp_show(struct device *dev,
                           struct device_attribute *attr, char *buf)
{
    struct hydra_dev *hdev = dev_get_drvdata(dev);
    char *bbuf = buf;
    uint32_t hix, pos;
    uint8_t serial[SERIAL_SIZE];
    if (! hdev) {
        pr_err("%s: no device!\n", __func__);
        return -EIO;
    }

    hix = hdev->index;
    if (! g_dev[hix].pci.pci_hydra) {
        pr_err("%s: no device [2]!\n", __func__);
        return -EIO;
    }

    pos = pci_find_ext_capability(g_dev[hix].pci.pci_us, PCI_EXT_CAP_ID_DSN);
    if (pos) {
        int i;

        pos += 4;
        for (i = 0; i < (sizeof(serial) - 4); i++) {
            pci_read_config_byte(g_dev[hix].pci.pci_us, pos + i, &serial[i]);
        }
    }

    pos = pci_find_ext_capability(g_dev[hix].pci.pci_dsi, PCI_EXT_CAP_ID_DSN);
    if (pos) {
        int i;

        pos += 4;
        for (i = 0; i < (sizeof(serial) - 8); i++) {
            pci_read_config_byte(g_dev[hix].pci.pci_dsi, pos + i, &serial[i + 8]);
        }
    }

    buf += snprintf(buf, PAGE_SIZE, "%c%c%c%c%c%c%c%c%c%c%c%c\n",
                    (int)serial[0], (int)serial[1], (int)serial[2], (int)serial[3],
                    (int)serial[4], (int)serial[5], (int)serial[6], (int)serial[7],
                    (int)serial[8], (int)serial[9], (int)serial[10], (int)serial[11]);

    return buf - bbuf;
}


//-------------------------------------------------------------------------------------------
// serial_ltw_show()  Get lightware serial number.
//-------------------------------------------------------------------------------------------
static ssize_t serial_ltw_show(struct device *dev,
                           struct device_attribute *attr, char *buf)
{
    struct hydra_dev *hdev = dev_get_drvdata(dev);
    char *bbuf = buf;
    uint32_t hix, pos;
    uint8_t serial[SERIAL_SIZE];
    if (! hdev) {
        pr_err("%s: no device!\n", __func__);
        return -EIO;
    }

    if ((hix = hdev->index) == HIX_HYDRA_PRI) {
        if (! g_dev[hix].pci.pci_hydra) {
            pr_err("%s: no device [2]!\n", __func__);
            return -EIO;
        }

        pos = pci_find_ext_capability(g_dev[hix].pci.pci_dse, PCI_EXT_CAP_ID_DSN);
        if (pos) {
            int i;

            pos += 4;
            for (i = 0; i < (sizeof(serial) - 4); i++) {
                pci_read_config_byte(g_dev[hix].pci.pci_dse, pos + i, &serial[i]);
            }
        }

        pos = pci_find_ext_capability(g_dev[hix].pci.pci_dsi, PCI_EXT_CAP_ID_DSN);
        if (pos) {
            int i;

            pos += 4;
            for (i = 0; i < (sizeof(serial) - 8); i++) {
                pci_read_config_byte(g_dev[hix].pci.pci_dsi, pos + 4 + i, &serial[i + 8]);
            }
        }

        buf += snprintf(buf, PAGE_SIZE, "%c%c%c%c%c%c%c%c%c%c%c%c\n",
                        (int)serial[0], (int)serial[1], (int)serial[2], (int)serial[3],
                        (int)serial[4], (int)serial[5], (int)serial[6], (int)serial[7],
                        (int)serial[8], (int)serial[9], (int)serial[10], (int)serial[11]);

    } else {
        buf += snprintf(buf, PAGE_SIZE, "serial_ltw stored only on hydra_p!\n");
    }

    return buf - bbuf;
}


//-------------------------------------------------------------------------------------------
// addr()  Dump domain/bus/device/function info for this device.
//-------------------------------------------------------------------------------------------
static ssize_t addr_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    char *bbuf = buf;
    struct pci_dev *bridge;
    struct hydra_dev *hdev = dev_get_drvdata(dev);

    if (! hdev) {
        pr_err("addr_show: no device!\n");
        return -EIO;
    }

    bridge = pci_upstream_bridge(hdev->pci.pci_hydra);
    if (! bridge) {
        pr_err("addr_show: no upstream bridge!\n");
        return 0;
    }

    buf += snprintf(buf, PAGE_SIZE, "hydra  : %02d:%02d.%d\n",
                    hdev->pci.pci_hydra->bus->number, PCI_SLOT(hdev->pci.pci_hydra->devfn), PCI_FUNC(hdev->pci.pci_hydra->devfn));

    buf += snprintf(buf, PAGE_SIZE, "bridge : %02d:%02d.%d\n",
                    bridge->bus->number, PCI_SLOT(bridge->devfn), PCI_FUNC(bridge->devfn));

    return buf - bbuf;
}


//-------------------------------------------------------------------------------------------
// speed()  Dump link speed.
//-------------------------------------------------------------------------------------------
static ssize_t speed_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    char *bbuf = buf;
    uint16_t linksta;
    struct hydra_dev *hdev = dev_get_drvdata(dev);

    if (! hdev) {
        pr_err("speed_show: no device!\n");
        return -EIO;
    }

    pcie_capability_read_word(pci_upstream_bridge(pci_upstream_bridge(hdev->pci.pci_hydra)), PCI_EXP_LNKSTA, &linksta);

    linksta &= 0xf;

    switch (linksta) {
        case PCI_EXP_LNKSTA_CLS_2_5GB:
            buf += snprintf(buf, PAGE_SIZE, "(Gen1)\n");
            break;

        case PCI_EXP_LNKSTA_CLS_5_0GB:
            buf += snprintf(buf, PAGE_SIZE, "(Gen2)\n");
            break;

        case PCI_EXP_LNKSTA_CLS_8_0GB:
            buf += snprintf(buf, PAGE_SIZE, "(Gen3)\n");
            break;

        default:
            buf += snprintf(buf, PAGE_SIZE, "(unknown speed)\n");
            break;
    }

    return buf - bbuf;
}


//-------------------------------------------------------------------------------------------
// pci_err()  Dump recoverable PCIe error count.
//-------------------------------------------------------------------------------------------
static ssize_t pci_err_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    char *bbuf = buf;
    struct hydra_dev *hdev = dev_get_drvdata(dev);

    if (! hdev) {
        pr_err("pci_err_show: no device!\n");
        return -EIO;
    }

    buf += snprintf(buf, PAGE_SIZE, "%d\n", hdev->pci.pci_err_count);

    return buf - bbuf;
}


//-------------------------------------------------------------------------------------------
// crit_ev_store()  Trigger a fake critical event.
//-------------------------------------------------------------------------------------------
static ssize_t crit_ev_store(struct device *dev,
                                struct device_attribute *attr,
                                const char *buf, size_t count)
{
    struct hydra_dev *hdev = dev_get_drvdata(dev);
    char *cur;
    int hix = hdev->index;
    uint32_t x = simple_strtol(buf, &cur, 0);

    if (x == 666) {
        g_shddr_map->hydra[hix].hydra_ce_arg0 = 0xdeadbeef;
        g_shddr_map->hydra[hix].hydra_ce_arg1 = 0xcafedead;
        crit_event_top_test(hix);
    } else {
        hdev->crit_event_0 = hdev->crit_event_1 = x;
    }

    return count;
}


//-------------------------------------------------------------------------------------------
// crit_ev()  Dump critical event notification.
//-------------------------------------------------------------------------------------------
static ssize_t crit_ev_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    char *bbuf = buf;
    struct hydra_dev *hdev = dev_get_drvdata(dev);

    if (! hdev) {
        pr_err("crit_ev_show: no device!\n");
        return 0;
    }

    buf += snprintf(buf, PAGE_SIZE, "%08X%08X\n", hdev->crit_event_0, hdev->crit_event_1);

    // Auto-clear after reading.
    hdev->crit_event_0 = hdev->crit_event_1 = 0;

    return buf - bbuf;
}


//-------------------------------------------------------------------------------------------
// guc_version()  Dump GUC info.
//-------------------------------------------------------------------------------------------
static ssize_t guc_version_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    char *bbuf = buf;
    struct hydra_dev *hdev = dev_get_drvdata(dev);

    if (! hdev) {
        pr_err("guc_version_show: no device!\n");
        return 0;
    }

    if (hdev->index == HIX_HYDRA_PRI) {
        buf += snprintf(buf, PAGE_SIZE, "0x%08x\n", g_shddr_map->hydra_p_guc_version);
    } else {
        buf += snprintf(buf, PAGE_SIZE, "0x%08x\n", g_shddr_map->hydra_s_guc_version);
    }

    return buf - bbuf;
}


//--------------------------------------------------------------------------
// blog_read()  Dump bootloader console log.
//--------------------------------------------------------------------------
static ssize_t blog_read(struct file *filp, struct kobject *kobj,
                         struct bin_attribute *attr,
                         char *buf, loff_t off, size_t count)
{
    struct device *dev = container_of(kobj, struct device, kobj);
    struct hydra_dev *hdev = dev_get_drvdata(dev);
    struct sram_map *sram;
    uint32_t hix;

    if (!hdev) {
        pr_err("blog_read: no device!\n");
        return -EIO;
    }

    hix = hdev->index;

    sram = (struct sram_map *)g_dev[hix].pci.hydra_sram_base_addr;

    memcpy(buf, &sram->clog_boot[off], count);

    return count;
}


//--------------------------------------------------------------------------
// alog_read()  Dump application console log.
//--------------------------------------------------------------------------
static ssize_t alog_read(struct file *filp, struct kobject *kobj,
                         struct bin_attribute *attr,
                         char *buf, loff_t off, size_t count)
{
    struct device *dev = container_of(kobj, struct device, kobj);
    struct hydra_dev *hdev = dev_get_drvdata(dev);
    struct sram_map *sram;
    uint32_t hix, head, nxt, cnt;
    static uint8_t tmp[CLOG_APP_SIZE];

    if (count > CLOG_APP_SIZE) {
        return -1;
    }

    if (!hdev) {
        pr_err("blog_read: no device!\n");
        return -EIO;
    }

    hix = hdev->index;

    sram = (struct sram_map *)g_dev[hix].pci.hydra_sram_base_addr;

    head = sram->clog_app_off;
    nxt = (head + 1) & (CLOG_APP_SIZE - 1);
    head &= (CLOG_APP_SIZE - 1);

    if (sram->clog_app[nxt] == 0) {
        // Log hasn't wrapped yet, so just save off what's been written.
        cnt = (count > head) ? head : count;
        memcpy(buf, &sram->clog_app[off], cnt);
        return cnt;
    } else {
        // It has. Copy whole alog to temp space.
        cnt = CLOG_APP_SIZE - head;
        memcpy(tmp, &sram->clog_app[nxt], cnt);
        memcpy(tmp + cnt, &sram->clog_app[0], head);

        memcpy(buf, &tmp[off], count);
        return count;
    }
}


//--------------------------------------------------------------------------
// hcpmem_read()  Read from the HCP block.
//--------------------------------------------------------------------------
static ssize_t hcpmem_read(struct file *filp, struct kobject *kobj,
                           struct bin_attribute *attr,
                           char *buf, loff_t off, size_t count)
{
    struct device *dev = container_of(kobj, struct device, kobj);
    struct hydra_dev *hdev = dev_get_drvdata(dev);
    uint32_t hix;

    if (! hdev) {
        pr_err("hcpmem_read: no device!\n");
        return -EIO;
    }

    hix = hdev->index;

    // And also check that we're not accessing past the end of the structure.
    if ((off + count) > attr->size) {
        pr_err("illegal read: off=%lld, size=%ld\n", off, attr->size);
        return -1;
    }

    memcpy(buf, (uint8_t *)g_dev[hix].pci.hydra_sram_base_addr + OFF_HCP_HYDRA + off, count);

    return count;
}


//--------------------------------------------------------------------------
// hcpmem_mmap()  Map the HCP block into user space.
//--------------------------------------------------------------------------
static int hcpmem_mmap(struct file *filp, struct kobject *kobj,
                       struct bin_attribute *attr, struct vm_area_struct *vma)
{
    struct device *dev = container_of(kobj, struct device, kobj);
    struct hydra_dev *hdev = dev_get_drvdata(dev);
    uint32_t hix;
    unsigned long size;
    uint64_t hcp_base;

    hix = hdev->index;

    hcp_base = (uint64_t)g_dev[hix].pci.hydra_sram_ioaddr + OFF_HCP_HYDRA;

    // Make sure the user isn't trying to get more than what they're entitled to.
    size = ((attr->size - 1) >> PAGE_SHIFT) + 1;
    if (vma->vm_pgoff + vma_pages(vma) > size) {
        pr_err("can't allocate bigger vma space than the HCP block (%ld vs %ld)\n", vma->vm_pgoff + vma_pages(vma), size);
        return -EINVAL;
    }

    vma->vm_page_prot = pgprot_device(vma->vm_page_prot);

    vma->vm_pgoff += (hcp_base >> PAGE_SHIFT);

    return io_remap_pfn_range(vma, vma->vm_start, vma->vm_pgoff, vma->vm_end - vma->vm_start, vma->vm_page_prot);
}


//--------------------------------------------------------------------------
// imumem_read()  Read from the shared imu sram block.
//--------------------------------------------------------------------------
static ssize_t imumem_read(struct file *filp, struct kobject *kobj,
                           struct bin_attribute *attr,
                           char *buf, loff_t off, size_t count)
{
    struct device *dev = container_of(kobj, struct device, kobj);
    struct hydra_dev *hdev = dev_get_drvdata(dev);
    uint32_t hix;

    if (! hdev) {
        pr_err("imumem_read: no device!\n");
        return -EIO;
    }

    hix = hdev->index;

    // And also check that we're not accessing past the end of the structure.
    if ((off + count) > attr->size) {
        pr_err("illegal read: off=%lld, size=%ld\n", off, attr->size);
        return -1;
    }

    memcpy(buf, (uint8_t *)g_dev[hix].pci.hydra_sram_base_addr + OFF_IMU_SHMEM + off, count);

    return count;
}


//--------------------------------------------------------------------------
// imumem_mmap()  Map the shared imu sram block into user space.
//--------------------------------------------------------------------------
static int imumem_mmap(struct file *filp, struct kobject *kobj,
                       struct bin_attribute *attr, struct vm_area_struct *vma)
{
    struct device *dev = container_of(kobj, struct device, kobj);
    struct hydra_dev *hdev = dev_get_drvdata(dev);
    uint32_t hix;
    unsigned long size;
    uint64_t imu_base;

    hix = hdev->index;

    imu_base = (uint64_t)g_dev[hix].pci.hydra_sram_ioaddr + OFF_IMU_SHMEM;

    // Make sure the user isn't trying to get more than what they're entitled to.
    size = ((attr->size - 1) >> PAGE_SHIFT) + 1;
    if (vma->vm_pgoff + vma_pages(vma) > size) {
        pr_err("can't allocate bigger vma space than the imu block (%ld vs %ld)\n", vma->vm_pgoff + vma_pages(vma), size);
        return -EINVAL;
    }

    vma->vm_page_prot = pgprot_device(vma->vm_page_prot);

    vma->vm_pgoff += (imu_base >> PAGE_SHIFT);

    return io_remap_pfn_range(vma, vma->vm_start, vma->vm_pgoff, vma->vm_end - vma->vm_start, vma->vm_page_prot);
}


//--------------------------------------------------------------------------
// firmware_write()  Download hydra firmware.
//--------------------------------------------------------------------------
static ssize_t firmware_write(struct file *filp, struct kobject *kobj,
                              struct bin_attribute *attr, char *buf, loff_t off, size_t count)
{
    struct device *dev = container_of(kobj, struct device, kobj);
    struct hydra_dev *hdev = dev_get_drvdata(dev);
    uint32_t hix;
    uint16_t vdr_arg;
    static ssize_t max_len;

    if (! hdev) {
        pr_err("firmware_write: no device!\n");
        return 0;
    }

    hix = hdev->index;

    // Can we do this?
    if (g_dev[hix].pci.bl_vers < 0x0A) {
        pr_err("Firmware push isn't supported on this hydra bootloader!\n");
        return -1;
    }

    // And also check that we're not accessing past the end of the structure.
    if ((off + count) > attr->size) {
        pr_err("illegal write: off=%lld, size=%ld\n", off, attr->size);
        return -1;
    }

    memcpy((uint8_t *)g_dev[hix].pci.hydra_sram_base_addr + FW_SRAM_OFFSET + off, buf, count);

    // Assume that a short block at the end signifies the end of the image.
    // This'll break if the image is a multiple of the block-size, so make sure it isn't!
    if (off == 0) {
        pr_info("Downloading firmware to hydra #%d, bs=%ld\n", hix, count);
        max_len = count;
    }

    if (count < max_len) {
        pr_info("Sending BOOT command to hydra #%d.\n", hix);

        // Send Boot command.
        vdr_arg = 0;
        hbcp_cmd_send(hix, g_dev[hix].pci.vdr_off + 8, HBCP_CMD_BOOT, &vdr_arg);
    }

    return count;
}


//--------------------------------------------------------------------------
// firmware_read()  Read hydra firmware.
//--------------------------------------------------------------------------
static ssize_t firmware_read(struct file *filp, struct kobject *kobj,
                             struct bin_attribute *attr, char *buf, loff_t off, size_t count)
{
    struct device *dev = container_of(kobj, struct device, kobj);
    struct hydra_dev *hdev = dev_get_drvdata(dev);
    uint32_t hix;

    if (! hdev) {
        pr_err("firmware_write: no device!\n");
        return -EIO;
    }

    hix = hdev->index;

    // And also check that we're not accessing past the end of the structure.
    if ((off + count) > attr->size) {
        pr_err("illegal write: off=%lld, size=%ld\n", off, attr->size);
        return -EFAULT;
    }

    memcpy(buf, (uint8_t *)g_dev[hix].pci.hydra_sram_base_addr + FW_SRAM_OFFSET + off, count);

    return count;
}


#if (DIM_MEM_SIZE > 0)
//--------------------------------------------------------------------------
// dim_mem_read()  Read from the dimmer block.
//--------------------------------------------------------------------------
static ssize_t dim_mem_read(struct file *filp, struct kobject *kobj,
                            struct bin_attribute *attr,
                            char *buf, loff_t off, size_t count)
{
    struct device *dev = container_of(kobj, struct device, kobj);
    struct hydra_dev *hdev = dev_get_drvdata(dev);
    uint32_t hix;

    if (!hdev) {
        pr_err("dim_mem_read: no device!\n");
        return -EIO;
    }

    hix = hdev->index;

    // Check that we're not accessing past the end of the region.
    if ((off + count) > attr->size) {
        pr_err("illegal read: off=%lld, size=%lu\n", off, attr->size);
        return -EFAULT;
    }

    memcpy(buf, (uint8_t *)g_dev[hix].pci.dim_addr_align + off, count);

    return count;
}


//--------------------------------------------------------------------------
// dim_mem_mmap()  Map the dimmer block into user space.
//--------------------------------------------------------------------------
static int dim_mem_mmap(struct file *filp, struct kobject *kobj,
                        struct bin_attribute *attr, struct vm_area_struct *vma)
{
    struct device *dev = container_of(kobj, struct device, kobj);
    struct hydra_dev *hdev = dev_get_drvdata(dev);
    struct page *page = NULL;
    uint32_t hix;
    int length_pg;

    // Make sure we got a page-aligned length.
    if (((vma->vm_end - vma->vm_start) % PAGE_SIZE) != 0) {
        pr_err("bad length\n");
        return -EINVAL;
    }

    // Make sure the user isn't trying to get more than what they're entitled to.
    length_pg = ((attr->size - 1) >> PAGE_SHIFT) + 1;
    if (vma->vm_pgoff + vma_pages(vma) > length_pg) {
        pr_err("illegal dim_mem size (%ld) vs (%d)\n", vma->vm_pgoff + vma_pages(vma), length_pg);
        return -EPERM;
    }

    hix = hdev->index;

    pr_info("dim_mmap: hix=%d, length=%d, physaddr=%p\n", hix, length_pg, g_dev[hix].pci.dim_addr_align);

    page = virt_to_page((unsigned long)g_dev[hix].pci.dim_addr_align + (vma->vm_pgoff << PAGE_SHIFT));
    vma->vm_flags |= VM_DONTEXPAND | VM_DONTDUMP | VM_SHARED;
    if (remap_pfn_range(vma,
                        vma->vm_start,
                        page_to_pfn(page),
                        vma->vm_end - vma->vm_start,
                        vma->vm_page_prot)) {
        pr_err("mmap: remap_pfn_range: failed\n");
        return -EAGAIN;
    }

    return 0;
}
#endif


#if (DBWIN_SIZE > 0)

#define DBWIN_OFF_X11           (0x000C0000)
#define DBWIN_OFF_HIFI_AUDIO    (0x000D0000)
#define DBWIN_OFF_HIFI_PERCEPT  (0x00500000)
#define DBWIN_OFF_P6            (0x00100000)

//-------------------------------------------------------------------------------------------
// dbwin_ctrl_store()
//-------------------------------------------------------------------------------------------
static ssize_t dbwin_ctrl_store(struct device *dev,
                                struct device_attribute *attr,
                                const char *buf, size_t count)
{
    struct hydra_dev *hdev = dev_get_drvdata(dev);
    uint32_t x;
    char *cur;
    int win;

    if (!hdev) {
        pr_err("dbwin_ctrl_store: no device!\n");
        return -EIO;
    }

    if (buf[0] == '0') {
        win = 0;
    } else if (buf[0] == '1') {
        win = 1;
    } else {
        pr_err("must indicate win number 0 or 1");
        return -EINVAL;
    }
    if (buf[1] != ':') {
        pr_err("Usage: win_number:core/address");
        return -EINVAL;
    }

    if (buf[2] == '@') {
        // Core name was specified.
        if (!strncmp(&buf[3], "x11", strlen("x11"))) {
            x = DBWIN_OFF_X11;
        } else if (!strncmp(&buf[3], "hifi_perception", strlen("hifi_perception"))) {
            x = DBWIN_OFF_HIFI_PERCEPT;
        } else if (!strncmp(&buf[3], "hifi_audio", strlen("hifi_audio"))) {
            x = DBWIN_OFF_HIFI_AUDIO;
        } else if (!strncmp(&buf[3], "p6", strlen("p6"))) {
            x = DBWIN_OFF_P6;
        } else {
            pr_err("Supported cores: x11, hifi_perception, hifi_audio, p6");
            return -EINVAL;
        }
    } else {
        // Address was specified.
        x = simple_strtol(&buf[2], &cur, 0);
    }

    pci_xlat_cfg(hdev->pci.hydra_exp_core_base_addr, win + 6, XLAT_INGRESS, XLAT_WIN_SIZE_16K,
                 (uint64_t)(uintptr_t)hdev->pci.hydra_aon_ioaddr + 0x200000 + (win * 0x100000), x);

    return count;
}


//-------------------------------------------------------------------------------------------
// dbwin_ctrl_show()
//-------------------------------------------------------------------------------------------
static ssize_t dbwin_ctrl_show(struct device *dev,
                               struct device_attribute *attr, char *buf)
{
    return 0;
}


static ssize_t dfs_msi_send_write(struct file *file, const char __user *data, size_t count, loff_t *ppos)
{
    struct hydra_dev *hdev = file->private_data;
    uint32_t vec;
    char buf[8], *cur;

    if (copy_from_user(buf, data, count > sizeof(buf) ? sizeof(buf) : count) != 0) {
        pr_err("dfs_msi_send_write: bad copy");
        return count;
    }

    vec = simple_strtol(buf, &cur, 0);

    pr_info("Sending MSI #%d from Hydra %d", vec, hdev->index);

    pci_msi_send(hdev->pci.hydra_exp_core_base_addr, vec);

    return count;
}


//--------------------------------------------------------------------------
// dbwin_mmap()  Map a dbg window into user space.
//--------------------------------------------------------------------------
static int dbwin_mmap(struct file *filp, struct kobject *kobj,
                      struct bin_attribute *attr, struct vm_area_struct *vma)
{
    struct device *dev = container_of(kobj, struct device, kobj);
    struct hydra_dev *hdev = dev_get_drvdata(dev);
    int win = (attr == &attr_dbwin0_mem) ? 0 : 1;
    unsigned long size;
    uint64_t base;

    base = (uint64_t)hdev->pci.hydra_aon_ioaddr + 0x200000 + (win * 0x100000);

    // Make sure the user isn't trying to get more than what they're entitled to.
    size = ((attr->size - 1) >> PAGE_SHIFT) + 1;
    if (vma->vm_pgoff + vma_pages(vma) > size) {
        pr_err("can't allocate bigger vma space than the HCP block (%ld vs %ld)\n", vma->vm_pgoff + vma_pages(vma), size);
        return -EINVAL;
    }

    vma->vm_page_prot = pgprot_device(vma->vm_page_prot);

    vma->vm_pgoff += (base >> PAGE_SHIFT);

    return io_remap_pfn_range(vma, vma->vm_start, vma->vm_pgoff, vma->vm_end - vma->vm_start, vma->vm_page_prot);
}
#endif

#if (ACP_SRAM_SIZE > 0)
//--------------------------------------------------------------------------
// acpmem_read()  Read from the HCP block.
//--------------------------------------------------------------------------
static ssize_t acpmem_read(struct file *filp, struct kobject *kobj,
                           struct bin_attribute *attr,
                           char *buf, loff_t off, size_t count)
{
    struct device *dev = container_of(kobj, struct device, kobj);
    struct hydra_dev *hdev = dev_get_drvdata(dev);
    uint32_t hix;

    if (!hdev) {
        pr_err("acpmem_read: no device!\n");
        return -EIO;
    }

    hix = hdev->index;

    // Check that we're not accessing past the end of the region.
    if ((off + count) > attr->size) {
        pr_err("illegal read: off=%lld, size=%lu\n", off, attr->size);
        return -EFAULT;
    }

    memcpy(buf, (uint8_t *)g_dev[hix].pci.acp_addr_align + off, count);

    return count;
}


//--------------------------------------------------------------------------
// acpmem_mmap()  Map the HCP block into user space.
//--------------------------------------------------------------------------
static int acpmem_mmap(struct file *filp, struct kobject *kobj,
                       struct bin_attribute *attr, struct vm_area_struct *vma)
{
    struct device *dev = container_of(kobj, struct device, kobj);
    struct hydra_dev *hdev = dev_get_drvdata(dev);
    struct page *page = NULL;
    uint32_t hix;
    int length_pg;

    // Make sure we got a page-aligned length.
    if (((vma->vm_end - vma->vm_start) % PAGE_SIZE) != 0) {
        pr_err("bad length\n");
        return -EINVAL;
    }

    // Make sure the user isn't trying to get more than what they're entitled to.
    length_pg = ((attr->size - 1) >> PAGE_SHIFT) + 1;
    if (vma->vm_pgoff + vma_pages(vma) > length_pg) {
        pr_err("illegal acpmem size (%ld) vs (%d)\n", vma->vm_pgoff + vma_pages(vma), length_pg);
        return -EPERM;
    }

    hix = hdev->index;

    pr_info("acp_mmap: hix=%d, length=%d, physaddr=%p\n", hix, length_pg, g_dev[hix].pci.acp_addr_align);

    page = virt_to_page((unsigned long)g_dev[hix].pci.acp_addr_align + (vma->vm_pgoff << PAGE_SHIFT));
    vma->vm_flags |= VM_DONTEXPAND | VM_DONTDUMP | VM_SHARED;
    if (remap_pfn_range(vma,
                        vma->vm_start,
                        page_to_pfn(page),
                        vma->vm_end - vma->vm_start,
                        vma->vm_page_prot)) {
        pr_err("mmap: remap_pfn_range: failed\n");
        return -EAGAIN;
    }

    return 0;
}
#endif


#if (MLDMA_SRAM_SIZE > 0)
//--------------------------------------------------------------------------
// mldmamem_read()  Read from the HCP block.
//--------------------------------------------------------------------------
static ssize_t mldmamem_read(struct file *filp, struct kobject *kobj,
                             struct bin_attribute *attr,
                             char *buf, loff_t off, size_t count)
{
    struct device *dev = container_of(kobj, struct device, kobj);
    struct hydra_dev *hdev = dev_get_drvdata(dev);
    uint32_t hix;

    if (!hdev) {
        pr_err("mldmamem_read: no device!\n");
        return -EIO;
    }

    hix = hdev->index;

    // And also check that we're not accessing past the end of the structure.
    if ((off + count) > attr->size) {
        pr_err("illegal read: off=%lld, size=%ld\n", off, attr->size);
        return -1;
    }

    memcpy(buf, (uint8_t *)g_dev[hix].pci.hydra_sram_base_addr + MLDMA_SRAM_OFF + off, count);

    return count;
}


//--------------------------------------------------------------------------
// mldmamem_mmap()  Map the HCP block into user space.
//--------------------------------------------------------------------------
static int mldmamem_mmap(struct file *filp, struct kobject *kobj,
                         struct bin_attribute *attr, struct vm_area_struct *vma)
{
    struct device *dev = container_of(kobj, struct device, kobj);
    struct hydra_dev *hdev = dev_get_drvdata(dev);
    uint32_t hix;
    unsigned long size;
    uint64_t sram_base;

    hix = hdev->index;

    sram_base = (uint64_t)g_dev[hix].pci.hydra_sram_ioaddr + MLDMA_SRAM_OFF;

    // Make sure the user isn't trying to get more than what they're entitled to.
    size = ((attr->size - 1) >> PAGE_SHIFT) + 1;
    if (vma->vm_pgoff + vma_pages(vma) > size) {
        pr_err("can't allocate bigger vma space than the sram block (%ld vs %ld)\n", vma->vm_pgoff + vma_pages(vma), size);
        return -EINVAL;
    }

    vma->vm_page_prot = pgprot_device(vma->vm_page_prot);

    vma->vm_pgoff += (sram_base >> PAGE_SHIFT);

    return io_remap_pfn_range(vma, vma->vm_start, vma->vm_pgoff, vma->vm_end - vma->vm_start, vma->vm_page_prot);
}
#endif


#if (SYSMEM_SRAM_SIZE > 0)
//--------------------------------------------------------------------------
// sysmem_read()  Read from Hydra SRAM.
//--------------------------------------------------------------------------
static ssize_t sysmem_read(struct file *filp, struct kobject *kobj,
                           struct bin_attribute *attr,
                           char *buf, loff_t off, size_t count)
{
    struct device *dev = container_of(kobj, struct device, kobj);
    struct hydra_dev *hdev = dev_get_drvdata(dev);
    uint32_t hix;

    if (!hdev) {
        pr_err("sysmem_read: no device!\n");
        return -EIO;
    }

    hix = hdev->index;

    // And also check that we're not accessing past the end of the structure.
    if ((off + count) > attr->size) {
        pr_err("illegal read: off=%lld, size=%ld\n", off, attr->size);
        return -1;
    }

    memcpy(buf, (uint8_t *)g_dev[hix].pci.hydra_sram_base_addr + off, count);

    return count;
}


//--------------------------------------------------------------------------
// sysmem_mmap()  Map Hydra SRAM into user space.
//--------------------------------------------------------------------------
static int sysmem_mmap(struct file *filp, struct kobject *kobj,
                       struct bin_attribute *attr, struct vm_area_struct *vma)
{
    struct device *dev = container_of(kobj, struct device, kobj);
    struct hydra_dev *hdev = dev_get_drvdata(dev);
    uint32_t hix;
    unsigned long size;
    uint64_t sram_base;

    hix = hdev->index;

    sram_base = (uint64_t)g_dev[hix].pci.hydra_sram_ioaddr;

    // Make sure the user isn't trying to get more than what they're entitled to.
    size = ((attr->size - 1) >> PAGE_SHIFT) + 1;
    if (vma->vm_pgoff + vma_pages(vma) > size) {
        pr_err("can't allocate bigger vma space than the sram block (%ld vs %ld)\n", vma->vm_pgoff + vma_pages(vma), size);
        return -EINVAL;
    }

    vma->vm_page_prot = pgprot_device(vma->vm_page_prot);

    vma->vm_pgoff += (sram_base >> PAGE_SHIFT);

    return io_remap_pfn_range(vma, vma->vm_start, vma->vm_pgoff, vma->vm_end - vma->vm_start, vma->vm_page_prot);
}
#endif

//
//--------------------------------------------------------------------------
// timestamp_read()  Read from Hydra timestamp.
//--------------------------------------------------------------------------
static ssize_t timestamp_read(struct file *filp, struct kobject *kobj,
                              struct bin_attribute *attr,
                              char *buf, loff_t off, size_t count)
{
    struct device *dev = container_of(kobj, struct device, kobj);
    struct hydra_dev *hdev = dev_get_drvdata(dev);
    uint32_t hix;
    uint64_t ts;
    unsigned int tstamper_offset = 0;
    uint8_t *tstamper_base = NULL;
    unsigned int cp_count = count < sizeof(ts) ? count : sizeof(ts);
    int b0 = 0;

    if (!hdev) {
        pr_err("timestamp_read: no device!\n");
        return -EIO;
    }

    hix = hdev->index;

    if (g_dev[hix].pci.chip_id == HYDRA_A0_CHIP_ID) {
        tstamper_offset = 0xD0;
        b0 = 0;
    } else if (g_dev[hix].pci.chip_id == HYDRA_B0_CHIP_ID) {
        tstamper_offset = 0xF8;
        b0 = 1;
    } else {
        pr_err("Unknown Hydra chip id\n");
        return -EIO;
    }
    tstamper_base = (uint8_t *)g_dev[hix].pci.hydra_aon_base_addr + 0x1000;

    ts = *(uint64_t *)(&tstamper_base[tstamper_offset]);

    if (b0) {
        ts = timestamp_b0_rescale(ts);
    }

    memcpy(buf, (uint8_t *)&ts, cp_count);

    return cp_count;
}


//--------------------------------------------------------------------------
// timestamp_mmap()  Map Hydra timestamp register into user space.
//--------------------------------------------------------------------------
static int timestamp_mmap(struct file *filp, struct kobject *kobj,
                          struct bin_attribute *attr, struct vm_area_struct *vma)
{
    struct device *dev = container_of(kobj, struct device, kobj);
    struct hydra_dev *hdev = dev_get_drvdata(dev);
    uint32_t hix;
    unsigned long size;
    uint64_t tstamper_base;

    hix = hdev->index;

    // Enforce read-only access.
    if (vma->vm_flags & VM_WRITE) {
        pr_err("write access to timestamp mem is denied.");
        return -EACCES;
    }

    tstamper_base = (uint64_t)g_dev[hix].pci.hydra_aon_ioaddr + 0x1000;

    // Make sure the user isn't trying to get more than what they're entitled to.
    size = ((attr->size - 1) >> PAGE_SHIFT) + 1;
    if (vma->vm_pgoff + vma_pages(vma) > size) {
        pr_err("can't allocate bigger vma space than the timestamp block (%ld vs %ld)\n", vma->vm_pgoff + vma_pages(vma), size);
        return -EINVAL;
    }

    vma->vm_page_prot = pgprot_device(vma->vm_page_prot);

    vma->vm_pgoff += (tstamper_base >> PAGE_SHIFT);

    return io_remap_pfn_range(vma, vma->vm_start, vma->vm_pgoff, vma->vm_end - vma->vm_start, vma->vm_page_prot);
}


//--------------------------------------------------------------------------
// alog_devnode()
//--------------------------------------------------------------------------
static char *alog_devnode(struct device *dev, umode_t *mode)
{
    struct hydra_dev *hdev = dev_get_drvdata(dev);

    if (!mode) {
        return NULL;
    }
    *mode = 0666;

    return kasprintf(GFP_KERNEL, (hdev->index == 0) ? "hydra_p/%s" : "hydra_s/%s", "alog");
}


//--------------------------------------------------------------------------
// hydra_alog_dev_create()
//--------------------------------------------------------------------------
static int hydra_alog_dev_create(int ix)
{
    int ret;
    char class[64];

    snprintf(class, sizeof(class), "hydra_alog_%c", ((ix == HIX_HYDRA_PRI) ? 'p' : 's'));

    if (g_alog[ix].dev)
    {
        // already created
        return 0;
    }

    // Allocate a major device numbers.
    ret = alloc_chrdev_region(&g_alog[ix].devt, 0, 1, (ix == 0) ? "hydra_p" : "hydra_s");
    if (ret < 0) {
        pr_err("can't allocate hydra major dev num. (%d)\n", ret);
        return ret;
    }

    g_alog[ix].class = class_create(THIS_MODULE, class);
    if (IS_ERR(g_alog[ix].class)) {
        pr_err("class_create failed\n");
        return -EPERM;
    }
    g_alog[ix].class->devnode = alog_devnode;

    // Register it.
    cdev_init(&g_alog[ix].cdev, &fops_alogx);
    g_alog[ix].cdev.owner = THIS_MODULE;
    ret = cdev_add(&g_alog[ix].cdev, g_alog[ix].devt, 1);
    if (ret) {
        pr_err("bad cdev_add (%d)\n", ret);

        class_destroy(g_alog[ix].class);
        return ret;
    }

    g_dev[ix].index = ix;
    g_alog[ix].dev = device_create(g_alog[ix].class, NULL, g_alog[ix].devt, &g_dev[ix], "%s", "alog");

    if (IS_ERR(g_alog[ix].dev)) {
        pr_err("device_create failed\n");

        cdev_del(&g_alog[ix].cdev);
        class_destroy(g_alog[ix].class);

        g_alog[ix].dev = NULL;
        return -EPERM;
    }

    pr_info("Hydra[%d] alog dev created %d\n", ix, g_alog[ix].dev == NULL);

    return 0;
}

static void hydra_alog_dev_destroy(int ix)
{
    if (g_alog[ix].dev)
    {
        wake_up_interruptible(&alog_ring_wait);
        device_destroy(g_alog[ix].class, g_alog[ix].devt);
        class_destroy(g_alog[ix].class);
        cdev_del(&g_alog[ix].cdev);
        g_alog[ix].dev = NULL;
    }
}

//--------------------------------------------------------------------------
// hydra_dev_create()  Create the hydra device instance.
//--------------------------------------------------------------------------
static int hydra_sysfs_devs_create(int ix)
{
    static char *names[2] = { "hydra_p", "hydra_s" };
    int i, ret;

    // Create /sys/devices/hydra
    g_dev[ix].hydra_sys_dev = root_device_register(names[ix]);
    if (IS_ERR(g_dev[ix].hydra_sys_dev)) {
        pr_err("bad sys_dev create.\n");
        return -1;
    }
    dev_set_drvdata(g_dev[ix].hydra_sys_dev, &g_dev[ix]);

    // Add attributes to it.
    ret = sysfs_create_group(&g_dev[ix].hydra_sys_dev->kobj, &hydra_attr_group);
    if (ret < 0) {
        pr_err("bad sysfs_create_group (%d)\n", ret);
        root_device_unregister(g_dev[ix].hydra_sys_dev);
        g_dev[ix].hydra_sys_dev = 0;
        return -1;
    }

    // Create all the binary sysfs entries.
    i = 0;
    while (g_bin_tab[i]) {
        if ((ret = sysfs_create_bin_file(&g_dev[ix].hydra_sys_dev->kobj, g_bin_tab[i])) < 0) {
            pr_err("bad sysfs_create_bin_file #%d (%d)\n", i, ret);

            // Delete all the others we already created.
            if (i--) {
                while (i >= 0) {
                    sysfs_remove_bin_file(&g_dev[ix].hydra_sys_dev->kobj, g_bin_tab[i]);
                    i--;
                }
            }
            sysfs_remove_group(&g_dev[ix].hydra_sys_dev->kobj, &hydra_attr_group);
            root_device_unregister(g_dev[ix].hydra_sys_dev);
            g_dev[ix].hydra_sys_dev = 0;
            return -1;
        }
        i++;
    }

    // An entry in debugfs too.
#ifdef CONFIG_DEBUG_FS
    g_dev[ix].ddir = debugfs_create_dir(names[ix], g_hydra_dev_ddir);

    // And local debugfs stuff.
    debugfs_create_file("msi_send", 0200, g_dev[ix].ddir, &g_dev[ix], &dfs_msi_send_fops);
#endif

    return 0;
}

//--------------------------------------------------------------------------
// hydra_dev_destroy()  Destroy the hydra device instance.
//--------------------------------------------------------------------------
static void hydra_sysfs_devs_destroy(int ix)
{
    int i = 0;
    // Remove from sysfs
    if (g_dev[ix].hydra_sys_dev) {
        pr_info("Removing main sysfs entries\n");
        sysfs_remove_group(&g_dev[ix].hydra_sys_dev->kobj, &hydra_attr_group);

        // Delete all the binary sysfs entries.
        while (g_bin_tab[i]) {
            sysfs_remove_bin_file(&g_dev[ix].hydra_sys_dev->kobj, g_bin_tab[i]);
            i++;
        }

        root_device_unregister(g_dev[ix].hydra_sys_dev);
        g_dev[ix].hydra_sys_dev = 0;
    }

#ifdef CONFIG_DEBUG_FS
    // Remove from debugfs.
    debugfs_remove_recursive(g_dev[ix].ddir);
#endif
}

//--------------------------------------------------------------------------
// alog_tick()
//--------------------------------------------------------------------------
static void alog_tick(int hix)
{
    if (g_dev[hix].pci.h_sram->clog_app_off != g_dev[hix].alog_tail) {
        g_dev[hix].pci.work_alog_job.hix = hix;
        schedule_work(&g_dev[hix].pci.work_alog_job.work);
    }
}


//--------------------------------------------------------------------------
// hydra_tmr_sec()
//--------------------------------------------------------------------------
static void hydra_tmr_sec(struct timer_list *t)
{
    hsi_tick();

    if (g_alog_scrape) {
        alog_tick(HIX_HYDRA_PRI);
        alog_tick(HIX_HYDRA_SEC);
    }

    mod_timer(&g_timer_sec, jiffies + HZ);
}


//--------------------------------------------------------------------------
// hydra_init()
//--------------------------------------------------------------------------
static int __init hydra_init(void)
{
    struct pci_dev *dev = NULL;
    int32_t result = 0;

    memset(&g_alog, 0, sizeof(g_alog));

#ifdef CONFIG_DEBUG_FS
    g_hydra_dev_ddir = debugfs_create_dir("hydra", NULL);
    if (IS_ERR_OR_NULL(g_hydra_dev_ddir)) {
        pr_err("no debugfs!");
        return PTR_ERR_OR_ZERO(g_hydra_dev_ddir) ?: -EINVAL;
    }
    debugfs_create_u32("hz", 0444, g_hydra_dev_ddir, &g_dfs_hz);
#endif

    // Get PCIe io-addresses into interesting CVIP places.
    g_acp_base_addr_io = 0;
    g_scm_base_addr_io = 0;
    while ((dev = pci_get_device(PCIE_AMD_VID, 0x163d, dev))) {
        g_acp_base_addr_io = pci_bus_address(dev, 0);
        g_acp_len          = pci_resource_len(dev, 0);
        g_scm_base_addr_io = pci_bus_address(dev, 2) + 0x00200000;
        g_scm_len          = pci_resource_len(dev, 2);
        pr_info("ACP is at PCIe address 0x%08llx, len=%d\n", g_acp_base_addr_io, g_acp_len);
        pr_info("SCM is at PCIe address 0x%08llx, len=%d\n", g_scm_base_addr_io, g_scm_len);
    }

    // Create generic wearable device.
    wearable_create();

    result = pci_register_driver(&g_hydra_drvr);
    if (result < 0) {
        pr_err("%d, hydra driver not installed.\n", result);
        wearable_destroy();
#ifdef CONFIG_DEBUG_FS
        debugfs_remove_recursive(g_hydra_dev_ddir);
#endif
        return result;
    }

    result = hsi_create_class();
    if (result < 0) {
        pr_err("%d, failed with hsi class create\n", result);
        pci_unregister_driver(&g_hydra_drvr);
        wearable_destroy();
#ifdef CONFIG_DEBUG_FS
        debugfs_remove_recursive(g_hydra_dev_ddir);
#endif
        return result;
    }

    pr_info("Hydra driver is registered\n");

    BLOCKING_INIT_NOTIFIER_HEAD(&g_notifiers);

    return 0;
}


//--------------------------------------------------------------------------
// hydra_uninit()
//--------------------------------------------------------------------------
void __exit hydra_uninit(void)
{
    unsigned int ix;

    for (ix = 0; ix < MAX_HYDRAS; ix++)
    {
        hydra_alog_dev_destroy(ix);
    }

    hsi_destroy_class();

    wearable_destroy();

    pci_unregister_driver(&g_hydra_drvr);

#ifdef CONFIG_DEBUG_FS
    debugfs_remove_recursive(g_hydra_dev_ddir);
#endif

    return;
}


//--------------------------------------------------------------------------
// hbcp_cmd_send()
//--------------------------------------------------------------------------
static int hbcp_cmd_send(uint32_t hix, uint32_t vreg_off, uint8_t cmd, uint16_t *arg)
{
    uint32_t reg, creg;
    uint64_t to, ts;
    int loops = 0;

    // Wait for one of the RDY flags to come up, to indicate that Hydra is ready to receive a command.
    // Normally, the A and B RDY's will be taking turns, changing after every command.
    ts = jiffies;
    to = jiffies + (HZ * 2);
    loops = 0;
    do {
        loops++;
        pci_read_config_dword(g_dev[hix].pci.pci_hydra, vreg_off, &reg);

    } while ((jiffies < to) && (!(reg & (HBCP_E_RDY_A | HBCP_E_RDY_B))));
    if (!(reg & (HBCP_E_RDY_A | HBCP_E_RDY_B))) {
        pr_info("\nhbcp: time out at command start, ts=%lld, to=%lld, loops=%d, reg=0x%08X\n", ts, to, loops, reg);
        return -ETIME;
    }
    //pr_info("\nhbcp: at command start, initial vdr reg is 0x%08X\n", reg);

    if (reg & HBCP_E_RDY_A) {
        reg  = HBCP_H_CMD_A;
        creg = HBCP_E_RDY_B;
    } else {
        reg  = HBCP_H_CMD_B;
        creg = HBCP_E_RDY_A;
    }

    reg |= HBCP_SET_CMD(cmd) | HBCP_SET_ARG(*arg);
    //pr_info("hbcp: writing vdr 0x%08X, creg=0x%08X\n", reg, creg);
    pci_write_config_dword(g_dev[hix].pci.pci_hydra, vreg_off, reg);

    // Now wait for a completion.
    ts = jiffies;
    to = jiffies + (HZ * 2);
    loops = 0;
    do {
        pci_read_config_dword(g_dev[hix].pci.pci_hydra, vreg_off, &reg);
        //pr_info("cpl: reg=0x%08x, creg=0x%08x\n", reg, creg);
        loops++;
    } while ((jiffies < to) && (!(reg & creg)));
    if (!(reg & creg)) {
        pr_info("\nhbcp: time out at command finish, ts=%lld, to=%lld, loops=%d, reg=0x%08X\n", ts, to, loops, reg);
        return -ETIME;
    }

    *arg = HBCP_GET_ARG(reg);

    return 0;
}


//--------------------------------------------------------------------------
// map_init()
//--------------------------------------------------------------------------
static int map_init(uint32_t hix, uint32_t vdr_off)
{
    uint16_t vdr_arg;

    pr_info("setting up SH-MEM Regions on hydra #%d, vdr=%d, shmem_addr=0x%llx\n", hix, vdr_off, g_shddr_bus_addr_align);

    vdr_arg = hix;
    if (hbcp_cmd_send(hix, vdr_off + 8, HBCP_CMD_IDENT_SET, &vdr_arg) < 0) {
        pr_err("HBCP command IDENT_SET timed out!\n");
        return -ETIME;
    }

    vdr_arg = ((uint64_t)g_shddr_bus_addr_align >> 48) & 0xffff;
    if(hbcp_cmd_send(hix, vdr_off + 8, HBCP_CMD_SHMEM_MAP_0, &vdr_arg) < 0) {
        pr_err("HBCP command BAR2_0 timed out!\n");
        return -ETIME;
    }

    vdr_arg = ((uint64_t)g_shddr_bus_addr_align >> 32) & 0xffff;
    if (hbcp_cmd_send(hix, vdr_off + 8, HBCP_CMD_SHMEM_MAP_1, &vdr_arg) < 0) {
        pr_err("HBCP command BAR2_1 timed out!\n");
        return -ETIME;
    }

    vdr_arg = ((uint64_t)g_shddr_bus_addr_align >> 16) & 0xffff;
    if (hbcp_cmd_send(hix, vdr_off + 8, HBCP_CMD_SHMEM_MAP_2, &vdr_arg) < 0) {
        pr_err("HBCP command BAR2_2 timed out!\n");
        return -ETIME;
    }

    vdr_arg = ((uint64_t)g_shddr_bus_addr_align >> 0) & 0xffff;
    if (hbcp_cmd_send(hix, vdr_off + 8, HBCP_CMD_SHMEM_MAP_3, &vdr_arg) < 0) {
        pr_err("HBCP command BAR2_3 timed out!\n");
        return -ETIME;
    }

    // Give the Hydra a chance to set up the mapping.
    msleep(20);

    return 0;
}


//--------------------------------------------------------------------------
// hydra_intr_bot()        Hydra bottom driver funcionality.
//--------------------------------------------------------------------------
static void hydra_intr_bot(unsigned long id)
{
    int irq_ix, hix;

    hix = id & 0xff;
    irq_ix = id >> 8;

#ifdef HYDRA_PM
    pm_runtime_get_sync(&g_dev[hix].pci.pci_hydra->dev);
#endif

    hsi_intr(hix, irq_ix + g_dev[hix].pci.msix_tab[0].vector);
#if 0
    for_each_set_bit(irq_ix, &g_dev[hix].intr_mask, HSI_MAX_STREAMS) {
        clear_bit(irq_ix, &g_dev[hix].intr_mask);
        hsi_intr(hix, irq_ix + g_dev[hix].pci.msix_tab[0].vector);
    }
#endif
#ifdef HYDRA_PM
    pm_runtime_mark_last_busy(&g_dev[hix].pci.pci_hydra->dev);
    pm_runtime_put_autosuspend(&g_dev[hix].pci.pci_hydra->dev);
#endif
}


//--------------------------------------------------------------------------
// hydra_work_exec()        Process a work item.
//--------------------------------------------------------------------------
static void hydra_work_exec(struct work_struct *work)
{
    struct work_job *job = (struct work_job *)work;

    // Free up the probe interrupt so it can be reallocated for actual device i/o by the hsitab probe.
    hydra_free_irq(job->hix_to_probe, 0, job->intr_irq, job->intr_id);

    hsi_probe(job->hix_to_probe);
}

//--------------------------------------------------------------------------
// hydra_work_alog_exec()      Do alog updates.
//--------------------------------------------------------------------------
static void hydra_work_alog_exec(struct work_struct *work)
{
    struct work_job_alog *job = (struct work_job_alog *)work;
    uint32_t head, tail;
    int hix = job->hix;
    uint8_t * tmp_ptr = g_dev[hix].alog_tmp_buff;
    unsigned int size = 0;
    unsigned int size_left = 0;

    if (ALOG_RING_LEN <= CLOG_APP_SIZE)
    {
        //shouldn't happen
        return;
    }

    // lock h_sram until end of function
    if (!g_dev[hix].pci.h_sram) {
        pr_err("hydra_work_alog_exec: no device!\n");
        return;
    }

    head = g_dev[hix].pci.h_sram->clog_app_off;
    tail = g_dev[hix].alog_tail;

    //read from hydra ring buffer
    if (tail > head) {
        memcpy_fromio(tmp_ptr, &g_dev[hix].pci.h_sram->clog_app[tail], CLOG_APP_SIZE - tail);
        tmp_ptr += CLOG_APP_SIZE - tail;
        size += CLOG_APP_SIZE - tail;
        tail = 0;
    }
    if (head > tail) {
        memcpy_fromio(tmp_ptr, &g_dev[hix].pci.h_sram->clog_app[tail], head - tail);
        size += head - tail;
    }

    g_dev[hix].alog_tail = head;

    if (size > 0)
    {
        tmp_ptr = g_dev[hix].alog_tmp_buff;
        size_left = ALOG_RING_LEN - g_dev[hix].alog_buff_ptr;

        //write to driver ring buffer
        if (size_left < size)
        {
            memcpy(g_dev[hix].alog_buff + g_dev[hix].alog_buff_ptr, tmp_ptr, size_left);
            tmp_ptr += size_left;
            g_dev[hix].alog_buff_ptr = 0;
            size -= size_left;
        }
        if (size > 0)
        {
            memcpy(g_dev[hix].alog_buff + g_dev[hix].alog_buff_ptr, tmp_ptr, size);
            g_dev[hix].alog_buff_ptr += size;
        }

        //pr_info("ring_work hix: %d, log_buff_ptr: %u, sizeleft %u, size: %u, head %u, tail %u\n", hix, g_dev[hix].alog_buff_ptr, size_left, size, head, tail);

        wake_up_interruptible(&alog_ring_wait);
    }
}

//--------------------------------------------------------------------------
// hydra_intr()
//--------------------------------------------------------------------------
static irqreturn_t hydra_intr(int32_t irq, void *dev_id)
{
    uint32_t hix = ((uint32_t)(uintptr_t)dev_id) & 0xff;
    int irq_ix   = ((uint32_t)(uintptr_t)dev_id) >> 8;

    DPRINT(6, "HYDRA-INT!: cpu=%d, irq=%d, irq_ix=%d, hix=%d\n", smp_processor_id(), irq, irq_ix, hix);

    trace_ml_hsi_intr_top(smp_processor_id(), irq, irq_ix);

    if (g_irq_dbg) {
        pr_info("HYDRA-INT!: cpu=%d, irq=%d, hix=%d\n", smp_processor_id(), irq, hix);
        return (g_irq_thr) ? IRQ_NONE : IRQ_HANDLED ;
    }

    // If we get an HSI interrupt but we've never probed the stream table, do it now.
    if (! g_dev[hix].hsi_probe_done) {
        g_dev[hix].hsi_probe_done = 1;
        g_dev[hix].pci.workq_job.intr_irq = irq;
        g_dev[hix].pci.workq_job.intr_id = dev_id;
        g_dev[hix].pci.workq_job.hix_to_probe = hix;
        queue_work(g_dev[hix].pci.workq, &g_dev[hix].pci.workq_job.work);
        return (g_irq_thr) ? IRQ_NONE : IRQ_HANDLED ;
    }

    if (! g_irq_thr) {
        tasklet_hi_schedule(&g_dev[hix].pci.tsklt[irq_ix]);
    }

    DPRINT(6, "HYDRA-INT-EXIT!: cpu=%d, irq=%d, irq_ix=%d, hix=%d\n", smp_processor_id(), irq, irq_ix, hix);

    return (g_irq_thr) ? IRQ_WAKE_THREAD :IRQ_HANDLED ;
}


//--------------------------------------------------------------------------
// hydra_intr_thr()
//--------------------------------------------------------------------------
static irqreturn_t hydra_intr_thr(int32_t irq, void *dev_id)
{
    uint32_t hix = ((uint32_t)(uintptr_t)dev_id) & 0xff;
    int irq_ix   = ((uint32_t)(uintptr_t)dev_id) >> 8;

    hsi_intr(hix, irq_ix + g_dev[hix].pci.msix_tab[0].vector);

    return IRQ_HANDLED;
}


//--------------------------------------------------------------------------
// crit_event_top()  Handle a critical event notification.
//--------------------------------------------------------------------------
static irqreturn_t crit_event_top(int32_t irq, void *dev_id)
{
    uint32_t hix = (uint32_t)(uintptr_t)dev_id;

    // Get event info from shmem.
    g_dev[hix].crit_event_0 = g_shddr_map->hydra[hix].hydra_ce_arg0;
    g_dev[hix].crit_event_1 = g_shddr_map->hydra[hix].hydra_ce_arg1;

    // Clear the event in case hydra wants to send another.
    g_shddr_map->hydra[hix].hydra_ce_arg1 = 0;
    smp_wmb();
    g_shddr_map->hydra[hix].hydra_ce_arg0 = 0;

    pr_err("CRITICAL EVENT on %s : %08x%08x\n", HIX_NAME(hix), g_dev[hix].crit_event_0, g_dev[hix].crit_event_1);

    sysfs_notify(&g_dev[hix].hydra_sys_dev->kobj, NULL, "crit_ev");

    return IRQ_HANDLED;
}

static irqreturn_t hcp_resp_intr(int32_t irq, void *dev_id)
{
    uint32_t hix = (uint32_t)(uintptr_t)dev_id;

    if (g_hcp_resp_handler) {
        g_hcp_resp_handler(hix);
    }

    return IRQ_HANDLED;
}

void hydra_register_hcp_resp_handler(void (*handler)(uint32_t))
{
    g_hcp_resp_handler = handler;
}

EXPORT_SYMBOL(hydra_register_hcp_resp_handler);
//
//--------------------------------------------------------------------------
// crit_event_top_test()
//--------------------------------------------------------------------------
static void crit_event_top_test(uint32_t hix)
{
    crit_event_top(0, (void *)(uintptr_t)hix);
}


//
//--------------------------------------------------------------------------
// irq_init()
//--------------------------------------------------------------------------
static int irq_init(uint32_t hix)
{
    int i, ret = 0;

    g_dev[hix].pci.irq_count = pci_msix_vec_count(g_dev[hix].pci.pci_hydra);
    pr_info("irq count is %d\n", g_dev[hix].pci.irq_count);

    g_dev[hix].pci.msix_tab = kmalloc_array(g_dev[hix].pci.irq_count, sizeof(struct msix_entry), GFP_KERNEL);
    if (! g_dev[hix].pci.msix_tab) {
        return -ENOMEM;
    }
    for (i = 0; i < g_dev[hix].pci.irq_count; i++) {
        g_dev[hix].pci.msix_tab[i].entry  = i;
        g_dev[hix].pci.msix_tab[i].vector = 0;
    }

    pr_info("Trying to allocate at least %d of %d %s PCIe interrupts.", (int)g_irq_min, (int)g_irq_max, g_msix ? "MSI-X" : "MSI");
    if ((ret = pci_alloc_irq_vectors(g_dev[hix].pci.pci_hydra, g_irq_min, g_irq_max, g_msix ? PCI_IRQ_MSIX : PCI_IRQ_MSI)) < 0) {
        pr_err("can't get %ld msi irqs, got=%d instead", g_irq_min, ret);
        kfree(g_dev[hix].pci.msix_tab);
        return ret;
    }
    g_dev[hix].pci.irq_count = ret;
    pr_info("Got %d irqs\n", g_dev[hix].pci.irq_count);

    // Temporarily set up the interrupt that hydra notifies us to indicate hsitab readiness.
    tasklet_init(&g_dev[hix].pci.tsklt[0], hydra_intr_bot, hix | (0 << 8));
    if (hydra_req_irq(hix, 0, pci_irq_vector(g_dev[hix].pci.pci_hydra, 0), hydra_intr, 0, 0, "hydra", (void *)(uintptr_t)(hix | (0 << 8)))) {
        pr_err("Can't alloc probe irq, irq=%d, id=%px\n", pci_irq_vector(g_dev[hix].pci.pci_hydra, 0), (void *)(uintptr_t)(hix | (0 << 8)));
        return -EIO;
    }

    // Get the Critical Event Notification interrupt.
    g_dev[hix].pci.msix_tab[HYDRA_CRIT_EV_IRQ].vector = pci_irq_vector(g_dev[hix].pci.pci_hydra, HYDRA_CRIT_EV_IRQ);
    if ((ret = hydra_req_irq(hix, HYDRA_CRIT_EV_IRQ, g_dev[hix].pci.msix_tab[HYDRA_CRIT_EV_IRQ].vector, crit_event_top, 0, 1, "crit_ev", (void *)(uintptr_t)hix)) == 0) {
        pr_info("Req crit_ev irq #%ld, vec=%d for %s\n", HYDRA_CRIT_EV_IRQ, g_dev[hix].pci.msix_tab[HYDRA_CRIT_EV_IRQ].vector, HIX_NAME(hix));
    } else {
        pr_info("FAILED to get crit_ev irq #%ld, vec=%d for %s\n", HYDRA_CRIT_EV_IRQ, g_dev[hix].pci.msix_tab[HYDRA_CRIT_EV_IRQ].vector, HIX_NAME(hix));
        return -EIO;
    }

    // Get the HCP response Notification interrupt.
    g_dev[hix].pci.msix_tab[HYDRA_HCP_IRQ].vector = pci_irq_vector(g_dev[hix].pci.pci_hydra, HYDRA_HCP_IRQ);
    if ((ret = hydra_req_irq(hix, HYDRA_HCP_IRQ, g_dev[hix].pci.msix_tab[HYDRA_HCP_IRQ].vector, hcp_resp_intr, 0, 0, "hcp_resp", (void *)(uintptr_t)hix)) == 0) {
        pr_info("Req hcp_resp irq #%ld, vec=%d for %s\n", HYDRA_HCP_IRQ, g_dev[hix].pci.msix_tab[HYDRA_HCP_IRQ].vector, HIX_NAME(hix));
    } else {
        pr_info("FAILED to get hcp_resp irq #%ld, vec=%d for %s\n", HYDRA_HCP_IRQ, g_dev[hix].pci.msix_tab[HYDRA_HCP_IRQ].vector, HIX_NAME(hix));
        return -EIO;
    }

    return 0;
}

//
//--------------------------------------------------------------------------
// irq_uninit()
//--------------------------------------------------------------------------
static void irq_uninit(uint32_t hix)
{
    int i;
    if (g_dev[hix].pci.msix_tab) {
        pr_info("releasing interrupts\n");
        for (i = 0; i < g_dev[hix].pci.irq_count; i++) {
            if (g_dev[hix].pci.irq_info[i].vector) {
                pr_info("releasing per/dev irq #%d = %d, cookie=0x%llx\n", i, g_dev[hix].pci.msix_tab[i].vector, (uint64_t)g_dev[hix].pci.irq_info[i].cookie);
                hydra_free_irq(hix, i, g_dev[hix].pci.irq_info[i].vector, g_dev[hix].pci.irq_info[i].cookie);
            }
        }
        pci_free_irq_vectors(g_dev[hix].pci.pci_hydra);
        kfree(g_dev[hix].pci.msix_tab);
    }
}

//--------------------------------------------------------------------------
// shmem_align()    Figure out shared host region alignment requirements so that Hydra can map it in.
//--------------------------------------------------------------------------
static int shmem_align(uint32_t size)
{
    int i, align = 0;

    align = 0;
    for (i = 0; i < 31; i++) {
        if ((1 << i) >= size) {
            align = 1 << i;
            break;
        }
    }

    return align;
}


#ifndef __aarch64__
#if (ACP_SRAM_SIZE > 0)
//--------------------------------------------------------------------------
// acp_sram_alloc()    Allocate some fake ACP sram for audio use.
//                     On Mero, this space isn't part of CVIP, but we'll
//                     still have to tell Hydra about it the same way.
//--------------------------------------------------------------------------
static int acp_sram_alloc(uint32_t hix)
{
    int align;

    // Allocate some fake ACP SRAM.
    align = shmem_align(ACP_SRAM_SIZE);
    pr_info("Aligned ACP sram size is %d\n", align);
    g_dev[hix].pci.acp_addr = dma_alloc_coherent(&g_dev[hix].pci.pci_hydra->dev, align * 2, &g_dev[hix].pci.acp_bus_addr, GFP_KERNEL);
    if (!g_dev[hix].pci.acp_addr) {
        printk(KERN_ERR "hydra: cannot allocate acp sram of size %d\n", ACP_SRAM_SIZE);
        return -ENOMEM;
    }
    g_dev[hix].pci.acp_bus_addr_align = (g_dev[hix].pci.acp_bus_addr + align) & ~(align - 1);
    g_dev[hix].pci.acp_addr_align     = (void *)(((uint64_t)g_dev[hix].pci.acp_addr + align) & ~(align - 1));
    pr_info("ACP SRAM starts at %px (bus=0x%016llx)\n", g_dev[hix].pci.acp_addr_align, g_dev[hix].pci.acp_bus_addr_align);

    return 0;
}
#endif
#endif


#if (DIM_MEM_SIZE > 0)
//--------------------------------------------------------------------------
// dimmer_mem_alloc()  Allocate some DDR for dimmer transport use.
//                     This is a temporary space used only until Orion is working.
//                     N.B. It's only for use with the PRIMARY Hydra.
//--------------------------------------------------------------------------
static int dimmer_mem_alloc(uint32_t hix)
{
    int align;

    align = shmem_align(DIM_MEM_SIZE);
    pr_info("Allocating memory for dimmer; aligned size is %d\n", align);
    g_dev[hix].pci.dim_addr = dma_alloc_coherent(&g_dev[hix].pci.pci_hydra->dev, align * 2, &g_dev[hix].pci.dim_bus_addr, GFP_KERNEL);
    if (!g_dev[hix].pci.dim_addr) {
        printk(KERN_ERR "hydra: cannot allocate dimmer mem of size %d\n", DIM_MEM_SIZE);
        return -ENOMEM;
    }
    g_dev[hix].pci.dim_bus_addr_align = ((uint64_t)g_dev[hix].pci.dim_bus_addr + (uint64_t)align) & ~((uint64_t)align - 1);
    g_dev[hix].pci.dim_addr_align     = (void *)(((uint64_t)g_dev[hix].pci.dim_addr + (uint64_t)align) & ~((uint64_t)align - 1));

    pr_info("Dimmer Mem starts at %px (bus=0x%016llx)\n", g_dev[hix].pci.dim_addr_align, g_dev[hix].pci.dim_bus_addr_align);

    // So that hydra/x86 can validate they're in the right region.
    memset((void *)g_dev[hix].pci.dim_addr_align, 0xDD, sizeof(uint64_t));
    memset((void *)g_dev[hix].pci.dim_addr_align + DIM_MEM_SIZE - sizeof(uint64_t), 0xDD, sizeof(uint64_t));

    return 0;
}
#endif


//--------------------------------------------------------------------------
// load_fw()
//--------------------------------------------------------------------------
static int load_fw(uint32_t hix, int have_name)
{
    const struct firmware *firmware, *bypass;
    char fw_fname[256];
    int retval, align;
    int valid_bypass;

    g_fw[hix].fw_length  = 0;

    // Are we guessing the name?
    if (! have_name) {
        // Try and load the board-specific firmware image.
        // If that fails, try the legacy "generic" version instead.
        snprintf(fw_fname, sizeof(fw_fname), "hydra_%c_%04x.fw",
                 ((hix == HIX_HYDRA_PRI) ? 'p' : 's'), g_dev[hix].pci.board_id);
        retval = request_firmware(&firmware, fw_fname, &g_dev[hix].pci.pci_hydra->dev);
        if (retval != 0) {
            pr_err("Board-specific firmware request for hydra failed (%s), err=%d\n", fw_fname, -retval);

            strcpy(fw_fname, (hix == HIX_HYDRA_PRI) ? "hydra_p.fw" : "hydra_s.fw");
            retval = request_firmware(&firmware, fw_fname, &g_dev[hix].pci.pci_hydra->dev);
            if (retval != 0) {
                pr_err("Board-generic firmware request for hydra failed also (%s), err=%d\n", fw_fname, -retval);

                return retval;
            }
        }
    } else {
        // We were told what name to use.
        retval = request_firmware(&firmware, g_dev[hix].fw_name, &g_dev[hix].pci.pci_hydra->dev);
        if (retval != 0) {
            pr_err("Requested firmware image (%s) not found, err=%d\n", g_dev[hix].fw_name, -retval);
            return retval;
        }
    }

    retval = firmware_request_nowarn(&bypass, "hydra_bypass.fw", &g_dev[hix].pci.pci_hydra->dev);
    if (retval != 0) {
        valid_bypass = 0;
        bypass = NULL;
    } else if (bypass->size != BYPASS_MAX_FILE_SIZE) {
        pr_err("Bypass file wrong size, size=%lu, expected=%u\n", bypass->size, BYPASS_MAX_FILE_SIZE);
        valid_bypass = 0;
    } else {
        pr_info("Bypass file=[%s], data=%px, size=%zu", "hydra_bypass.fw", bypass->data, bypass->size);
        valid_bypass = 1;
    }

    // Figure out the alignment requirements so that Hydra can map it in.
    align = shmem_align(BYPASS_MAX_FILE_SIZE + firmware->size);

    pr_info("Hydra FW file=[%s], data=%px, size=%zu, bufsize=%d\n", have_name ? g_dev[hix].fw_name : fw_fname, firmware->data, firmware->size, align);

    if (! g_fw[hix].fw_addr) {
        g_fw[hix].fw_addr = dma_alloc_coherent(&g_dev[hix].pci.pci_hydra->dev, align * 2, &g_fw[hix].fw_bus_addr, GFP_KERNEL);
        if (! g_fw[hix].fw_addr) {
            printk(KERN_ERR "hydra: cannot allocate fw buffer of size %d\n", align * 2);
            release_firmware(firmware);
            if (bypass) {
                release_firmware(bypass);
            }
            return -ENOMEM;
        }
        g_fw[hix].fw_bus_addr_align = (g_fw[hix].fw_bus_addr + align) & ~(align - 1);
        g_fw[hix].fw_addr_align     = (void *)(((uint64_t)g_fw[hix].fw_addr + align) & ~(align - 1));
    } else {
        pr_info("Recycling previous firmware memory.\n");
    }

    // recopy the firmware data - the file content may have changed
    memcpy(g_fw[hix].fw_addr_align + BYPASS_MAX_FILE_SIZE, firmware->data, firmware->size);
    if (valid_bypass) {
        memcpy(g_fw[hix].fw_addr_align, bypass->data, bypass->size);
    }

    pr_info("Hydra firmware starts at bus address 0x%016llx (aligned=0x%016llx)\n", g_fw[hix].fw_bus_addr, g_fw[hix].fw_bus_addr_align);

    g_fw[hix].fw_length = firmware->size;

    release_firmware(firmware);
    if (bypass) {
        release_firmware(bypass);
    }

    return 0;
}


//--------------------------------------------------------------------------
// hydra_probe()
//--------------------------------------------------------------------------
static int hydra_probe(struct pci_dev *pPciDev, const struct pci_device_id *pPciEntry)
{
    struct sram_map *sram;
    uint8_t found;
    int32_t result = 0;
    uint32_t hix;
    uint32_t bar_hi, bar_lo;
    uint32_t vdr_off, vdr, reg;
    long maj, min;
    struct sram_map *hydra_sram;
    struct pci_dev *bridge, *tmpd;
    char alog_enb_val[32];

    // Figure out if we're the primary or secondary.
    // Only the secondary's upstream bridge's parent will have a NWL vendor id; the primary won't.
    bridge = pci_upstream_bridge(pci_upstream_bridge(pci_upstream_bridge(pPciDev)));
    if (bridge->vendor != PCIE_NWL_VID) {
        hix = HIX_HYDRA_PRI;
    } else {
        hix = HIX_HYDRA_SEC;
    }

    pr_info("hydra_probe: hix=%d, devid=0x%04x, dev=%px, pdev=%px, bdf=%02x:%02x.%d, pvid=%04x\n",
            hix, pPciDev->device, &pPciDev->dev, pPciDev, pPciDev->bus->number, PCI_SLOT(pPciDev->devfn), PCI_FUNC(pPciDev->devfn), bridge->vendor);

    // Recognize which hydra personality we're looking at.
    switch (pPciEntry->device) {
        case PCIE_EP_HYDRA_BOOTLOADER_DID:
            pr_info("Found B0BC enum Hydra: aka 1st enum OTP or 2nd enum sram bootloader\n");
            break;
        case PCIE_EP_HYDRA_MAIN_FW_DID:
            pr_info("Found B0BB enum Hydra: aka 2nd enum OTP or 3nd enum sram bootloader, now in main fw\n");
            break;
        default:
            pr_err("Unknown hydra enumeration!\n");
            break;
    }

    // Clear out globals.
    memset(&g_dev[hix], 0, sizeof(g_dev[hix]));
    g_dev[hix].index = hix;

    // Disable stand_down mode in case it was on.
    g_stand_down[hix] = 0;

    // Save pci device.
    g_dev[hix].pci.pci_hydra = pPciDev;

    // Save other hydra bridges.
    g_dev[hix].pci.pci_dsi = pci_upstream_bridge(pPciDev);
    g_dev[hix].pci.pci_us  = pci_upstream_bridge(g_dev[hix].pci.pci_dsi);

    //
    // Ideally we DSE secondary would never enumerate because 1- it's useless and 2- we want them power savings.
    // For b0bc and b0bb enumerations - which are the only ones hydra.ko supports - this will usually be true.
    // But the world is imperfect: on old OTP devices, b0bc will come up with DSE secondary. So we loop through
    // until we find primary.
    //
    if (hix == HIX_HYDRA_PRI) {
        tmpd = 0;
        found = 0;
        while ((tmpd = pci_get_device(PCIE_NWL_VID, PCIE_DSE_DID, tmpd)) != NULL && !found) {
            struct pci_dev *cb = pci_upstream_bridge(pci_upstream_bridge(tmpd));
            if (cb->vendor != PCIE_NWL_VID) {
                g_dev[HIX_HYDRA_PRI].pci.pci_dse = tmpd;
                found = 1;
            }
        }
        if (!found) {
            pr_err("DSE has not been set for hydra primary\n");
        }
    }

    // Connect the device to the driver.
    pci_set_drvdata(pPciDev, &g_dev[hix]);

    // Enable access to the device.
    result = pci_enable_device(pPciDev);
    if (result) {
        pr_err("Cannot probe FPGA device %s: error %d\n", pci_name(pPciDev), result);
        goto pci_enable_device_fail;
    }
    pci_set_master(pPciDev);

    // Get the bootloader version.
    g_dev[hix].pci.bl_vers = get_bl_version(hix);
    pr_info("Hydra[%d] bootloader version is %d\n", hix, g_dev[hix].pci.bl_vers);

    // Get the board-id.
    g_dev[hix].pci.board_id = get_board_id(hix);
    pr_info("Hydra[%d] board-id 0x%04X\n", hix, g_dev[hix].pci.board_id);

    // Get the chip-id.
    g_dev[hix].pci.chip_id = get_chip_id(hix);
    pr_info("Hydra[%d] chip-id 0x%04X\n", hix, g_dev[hix].pci.chip_id);

    // Debug info to see if we are matching Hydra values
    pr_info("Hydra[%d] sram_map version: (Not implemented yet), sizeof: %ld\n",
            hix, sizeof(struct sram_map));
    pr_info("Hydra[%d] hsi version: (Not implemented yet), offset: 0x%lx sizeof: %ld\n",
            hix, OFF_HSI_TABLE, sizeof(struct hsi_root));
    pr_info("Hydra[%d] hcp version: (Not implemented yet), offset: 0x%lx sizeof: %ld\n",
            hix, OFF_HCP_HYDRA, HCP_DATA_LEN);

    // Find the correct (user) VDR.
    vdr_off = 0;
    while ((vdr_off = pci_find_next_ext_capability(pPciDev, vdr_off, PCI_EXT_CAP_ID_VNDR)) > 0) {
        pci_read_config_dword(pPciDev, vdr_off + 4, &vdr);
        if ((vdr & 0xffff) == 0) {
            pr_info("Got the right VDR!\n");
            break;
        }
    }
    if (vdr == 0) {
        pr_err("Didn't find com registers!\n");
        result = -ENOSYS;
        goto find_vdr_fail;
    }
    g_dev[hix].pci.vdr_off = vdr_off;

    pr_info("About to request regions\n");
    result = pci_request_regions(pPciDev, "hydra");
    if (result < 0) {
        pr_err("hydra: cannot request regions\n");
        result = -EBUSY;
        goto pci_request_regions_fail;
    }

    // Hydra is fine with 64 bit bus addresses.
    if (pci_set_dma_mask(pPciDev, DMA_BIT_MASK(64)) == 0) {
        result = pci_set_consistent_dma_mask(pPciDev, DMA_BIT_MASK(64));
        if (result != 0) {
            pr_err("pci_set_consistent_dma_mask failed\n");
            result = -ENOSYS;
            goto pci_set_dma_mask_fail;
        }
    }

    // Init workq.
    g_dev[hix].pci.workq = create_workqueue("hsi");
    if (! g_dev[hix].pci.workq) {
        pr_err("can't create workq\n");
        result = -EBUSY;
        goto create_workqueue_fail;
    }
    INIT_WORK(&g_dev[hix].pci.workq_job.work, hydra_work_exec);
    INIT_WORK(&g_dev[hix].pci.work_alog_job.work, hydra_work_alog_exec);

    // Enable alog scraping if asked.
    cvip_plt_property_get_safe("persist.kernel.hsi.alog_enable", alog_enb_val, sizeof(alog_enb_val), "0");
    if (alog_enb_val[0] == '1') {
        g_alog_scrape = 1;
        pr_warn("Enabling alog scraping.\n");
    }

    // Get base of NWL space.
    g_dev[hix].pci.hydra_exp_core_ioaddr = (void *)pci_resource_start(pPciDev, 0);
    g_dev[hix].pci.hydra_exp_core_ioaddr_size = pci_resource_len(pPciDev, 0);
    g_dev[hix].pci.hydra_exp_core_base_addr = ioremap((resource_size_t)g_dev[hix].pci.hydra_exp_core_ioaddr, g_dev[hix].pci.hydra_exp_core_ioaddr_size);
    if (! g_dev[hix].pci.hydra_exp_core_base_addr) {
        printk(KERN_ERR "hydra: cannot iomap exp_core region of size %d\n", g_dev[hix].pci.hydra_exp_core_ioaddr_size);
        result = -EBUSY;
        goto ioremap_hydra_exp_core_base_addr_fail;
    }
    pr_info("Hydra NWL base bus addr = %px\n",  g_dev[hix].pci.hydra_exp_core_ioaddr);
    pr_info("Hydra NWL base vaddr    = %px\n",  g_dev[hix].pci.hydra_exp_core_base_addr);

    // Get base of hydra system-ram.
    g_dev[hix].pci.hydra_sram_ioaddr = (void *)pci_resource_start(pPciDev, 2);
    g_dev[hix].pci.hydra_sram_ioaddr_size = pci_resource_len(pPciDev, 2);
    g_dev[hix].pci.hydra_sram_base_addr = ioremap((resource_size_t)g_dev[hix].pci.hydra_sram_ioaddr, g_dev[hix].pci.hydra_sram_ioaddr_size);
    if (! g_dev[hix].pci.hydra_sram_base_addr) {
        printk(KERN_ERR "hydra: cannot iomap sram region of size %d\n", g_dev[hix].pci.hydra_sram_ioaddr_size);
        result = -EBUSY;
        goto ioremap_hydra_sram_base_addr_fail;
    }

    g_dev[hix].pci.h_sram = (struct sram_map *)g_dev[hix].pci.hydra_sram_base_addr;
    pr_info("Hydra sram base bus addr = %px\n",  g_dev[hix].pci.hydra_sram_ioaddr);
    pr_info("Hydra sram base vaddr    = %px\n",  g_dev[hix].pci.hydra_sram_base_addr);
    pr_info("Hydra sram region size   = %d\n",   g_dev[hix].pci.hydra_sram_ioaddr_size);

    // Get base of hydra AON space.
    g_dev[hix].pci.hydra_aon_ioaddr = (void *)pci_resource_start(pPciDev, 4);
    g_dev[hix].pci.hydra_aon_base_addr = ioremap((resource_size_t)g_dev[hix].pci.hydra_aon_ioaddr, pci_resource_len(pPciDev, 4));
    if (! g_dev[hix].pci.hydra_aon_base_addr) {
        printk(KERN_ERR "hydra: cannot iomap aon region\n");
        result = -EBUSY;
        goto ioremap_hydra_aon_base_addr_fail;
    }
    pr_info("Hydra AON base vaddr    = %px\n",  g_dev[hix].pci.hydra_aon_base_addr);

    // Allocate shared region out of host DDR.
    if (! g_shddr_addr) {
        g_shddr_length = shmem_align(SHDDR_SIZE);
        pr_info("Request %d bytes of coherent space from %px\n", g_shddr_length * 2, &pPciDev->dev);
        g_shddr_addr = dma_alloc_coherent(&pPciDev->dev, g_shddr_length * 2, &g_shddr_bus_addr, GFP_KERNEL);
        if (! g_shddr_addr) {
            printk(KERN_ERR "hydra: cannot allocate shddr region of size %d\n", g_shddr_length);
            result = -ENOMEM;
            goto dma_alloc_coherent_fail;
        }
        memset(g_shddr_addr, 0, g_shddr_length * 2);
        g_shddr_bus_addr_align = ((uint64_t)g_shddr_bus_addr + (uint64_t)g_shddr_length) & ~((uint64_t)g_shddr_length - 1);
        g_shddr_addr_align     = (void *)(((uint64_t)g_shddr_addr + (uint64_t)g_shddr_length) & ~((uint64_t)g_shddr_length - 1));
    } else {
        pr_info("Already have %d bytes of coherent space from %px\n", g_shddr_length * 2, &pPciDev->dev);
    }

    // Populate the shddr map. Possibly for the nth time, but the values written will always be the same.
    g_shddr_map = (struct shmem_map *)g_shddr_addr_align;
    g_shddr_map->shmem_length    = SHDDR_SIZE;
    g_shddr_map->shmem_io_addr   = g_shddr_bus_addr_align;
    g_shddr_map->vers            = SHMEM_VERS;

    smp_wmb();
    g_shddr_map->magic                    = SHMEM_MAP_MAGIC;
    smp_wmb();

    pr_info("SH-DDR (coherent ddr) starts at %px, bus_addr=0x%llx, size is %d\n", g_shddr_addr_align, g_shddr_bus_addr_align, SHDDR_SIZE);

    // Populate per/hydra info.
    // Read the bar addresses straight out of config space instead of calling pci_read_config_dword()
    // since that gets us the cvip aperture addresses and not the true PCI address.
    if (g_dev[hix].pci.bl_vers >= 0x0A) {
        pr_info("Using new resource mapping\n");
        pci_read_config_dword(g_dev[hix].pci.pci_hydra, PCI_BASE_ADDRESS_0, &bar_lo);
        pci_read_config_dword(g_dev[hix].pci.pci_hydra, PCI_BASE_ADDRESS_1, &bar_hi);
        g_shddr_map->hydra[hix].bar0 = ((uint64_t)bar_hi << 32) | (bar_lo & 0xfffffff0);
        pr_info("BAR0 = 0x%llx", g_shddr_map->hydra[hix].bar0);

        pci_read_config_dword(g_dev[hix].pci.pci_hydra, PCI_BASE_ADDRESS_2, &bar_lo);
        pci_read_config_dword(g_dev[hix].pci.pci_hydra, PCI_BASE_ADDRESS_3, &bar_hi);
        g_shddr_map->hydra[hix].bar2 = ((uint64_t)bar_hi << 32) | (bar_lo & 0xfffffff0);
        pr_info("BAR2 = 0x%llx", g_shddr_map->hydra[hix].bar2);

        pci_read_config_dword(g_dev[hix].pci.pci_hydra, PCI_BASE_ADDRESS_4, &bar_lo);
        pci_read_config_dword(g_dev[hix].pci.pci_hydra, PCI_BASE_ADDRESS_5, &bar_hi);
        g_shddr_map->hydra[hix].bar4 = ((uint64_t)bar_hi << 32) | (bar_lo & 0xfffffff0);
        pr_info("BAR4 = 0x%llx", g_shddr_map->hydra[hix].bar4);
    } else {
        pr_info("Using legacy resource mapping\n");

        // Old bootloaders will look in the wrong place for the bars, so oblige them if needed.
        g_shddr_map->hydra[0].bar0 = pci_resource_start(pPciDev, 0);
        g_shddr_map->hydra[0].bar2 = pci_resource_start(pPciDev, 2);
        g_shddr_map->hydra[0].bar4 = pci_resource_start(pPciDev, 4);
    }

    // Load hydra app code.
    if ((g_skip_fw == 0) && (load_fw(hix, 0) == 0)) {
        g_shddr_map->hydra[hix].fw_io_addr = g_fw[hix].fw_bus_addr_align;
        g_shddr_map->hydra[hix].fw_length  = g_fw[hix].fw_length;
    } else {
        pr_info("Bypassing firmware download\n");
        g_shddr_map->hydra[hix].fw_io_addr = 0;
        g_shddr_map->hydra[hix].fw_length  = 0;
    }

#ifndef __aarch64__
#if (ACP_SRAM_SIZE > 0)
    // Allocate emulated ACP sram and pass info about it.
    if (! acp_sram_alloc(hix)) {
        g_shddr_map->acpmem_io_addr = g_dev[hix].pci.acp_bus_addr_align;
        g_shddr_map->acpmem_length  = ACP_SRAM_SIZE;
    }
#endif
#else
    // Point it to the real Mero ACP region.
    g_shddr_map->acpmem_io_addr = g_acp_base_addr_io;
    g_shddr_map->acpmem_length  = g_acp_len;

    // Only map the first 1MB, alignment issues preclude easily mapping more.
    g_shddr_map->acpmem_length  = 1024 * 1024;
#endif

#if (DIM_MEM_SIZE > 0)
    // Allocate dimmer memory if this is the primary hydra.
    if ((hix == HIX_HYDRA_PRI) && (! dimmer_mem_alloc(hix))) {
        g_shddr_map->dim_mem_io_addr = g_dev[hix].pci.dim_bus_addr_align;
        g_shddr_map->dim_mem_length  = DIM_MEM_SIZE;
    }
#endif

    // SCM info.
    g_shddr_map->scm_mem_io_addr = g_scm_base_addr_io;
    g_shddr_map->scm_mem_length  = g_scm_len;
    smp_wmb();

    // Attempt to pass the shddr map address to the device, if it hasn't been done already.
    pci_read_config_dword(g_dev[hix].pci.pci_hydra, vdr_off + 8, &reg);
    if (! (reg & HBCP_E_MODE_MASK)) {
        pr_info("Start HBCP protocol\n");
        result = map_init(hix, vdr_off);
        if (result < 0) {
            pr_err("communication with Hydra bootloader failed!\n");
            pr_err("cleaning up device at %p\n", pPciDev);
            result = -ENODEV;
            goto map_init_fail;
        }
    } else {
        pr_info("Skipping HBCP negotiation\n");
    }

    // Create the hydra device.
    pr_info("creating Hydra sysfs devices\n");
    if (hydra_sysfs_devs_create(hix) < 0) {
        result = -ENXIO;
        goto hydra_sysfs_devs_create_fail;
    }

    // Get access to hydra sram map.
    sram = (struct sram_map *)g_dev[hix].pci.hydra_sram_base_addr;

    // On an OTP boot, this is as far as we go.
    if (pPciEntry->device == PCIE_EP_HYDRA_BOOTLOADER_DID) {
        pr_info("completed initial OTP device setup\n");
        return 0;
    } else {
        sram->boot_status_cmd = g_fw_pcie ? BOOT_STATUS_CMD_PCIE_ONLY : BOOT_STATUS_CMD_NORMAL;
        pr_info("Sent GO command to main fw (%lu)\n", g_fw_pcie);
    }
    hydra_timer_start();

    // Sanity check, make sure the device seems to be mapped ok.
    pr_err("Checking for magic num at %px", sram);
    if (sram->magic != SRAM_MAGIC) {
        pr_err("Hydra bootloader hasn't run! (0x%08x)\n", sram->magic);
        result = -ENODEV;
        goto sram_magic_fail;
    }

    // Save some useful info from the device.
    pr_info("HSI table starts at offset 0x%08x, length is %d\n", sram->hsi_tab_off, sram->hsi_tab_size);

    // Get version info.
    if (kstrtol(HYDRA_MODULE_MAJOR_VERSION, 10, &maj)) {
        pr_err("Major version could not be parsed\n");
        result = -EINVAL;
        goto kstrtol_fail;
    }

    if (kstrtol(HYDRA_MODULE_MINOR_VERSION, 10, &min)) {
        pr_err("Minor version could not be parsed\n");
        result = -EINVAL;
        goto kstrtol_fail;
    }
    g_dev[hix].version = (maj << 16) | (min & 0xffff);

    // Set up the HSI subsystem.
    hydra_sram = (struct sram_map *)g_dev[hix].pci.hydra_sram_base_addr;
    result = hsi_init(hix,
                      g_dev[hix].pci.pci_hydra,
                      g_dev[hix].hydra_sys_dev,
                      g_dev[hix].ddir,
                      (struct hsi_root *)((uint8_t *)(hydra_sram) + hydra_sram->hsi_tab_off),
                      &g_dev[hix]);
    if (result < 0) {
        goto hsi_init_fail;
    }

    // Enable Hydra interrupts.
    result = irq_init(hix);
    if (result < 0) {
        goto irq_init_fail;
    }

#ifdef HYDRA_PM
#if 0
    // The lines below are SOP in other drivers, but don't work here for some reason.
    // They don't appear necessary but leave here for now.
    if ((result = pm_runtime_set_active(&pPciDev->dev))) {
        pr_err("pm set active failed!: %d\n", result);
    }
    pm_runtime_enable(&pPciDev->dev);
#endif
    pm_runtime_allow(&pPciDev->dev);
    pm_runtime_set_autosuspend_delay(&pPciDev->dev, 500);
    pm_runtime_use_autosuspend(&pPciDev->dev);
    pm_runtime_resume(&pPciDev->dev);


    pm_runtime_mark_last_busy(&pPciDev->dev);
    pm_runtime_put_autosuspend(&pPciDev->dev);
#endif

    blocking_notifier_call_chain(&g_notifiers, hix, (void *)1);

    pr_info("Hydra module (v%s) successfully initialized.\n",
            HYDRA_MODULE_MAJOR_VERSION "." HYDRA_MODULE_MINOR_VERSION);

    if ((result = sysfs_create_file(&g_dev[hix].hydra_sys_dev->kobj, &dev_attr_ready.attr)) < 0) {
        pr_err("bad sysfs_create_file for rdy file (%d)\n", result);
    }

    // Add an alog scraping node for this device.
    if (g_alog_scrape) {
        hydra_alog_dev_create(hix);
    }

    return 0;
irq_init_fail:
    hsi_uninit(hix);
hsi_init_fail:
kstrtol_fail:
sram_magic_fail:
    hydra_sysfs_devs_destroy(hix);
hydra_sysfs_devs_create_fail:
map_init_fail:
#if (DIM_MEM_SIZE > 0)
    if (g_dev[hix].pci.dim_addr) {
        dma_free_coherent(&g_dev[hix].pci.pci_hydra->dev, DIM_MEM_SIZE, g_dev[hix].pci.dim_addr, g_dev[hix].pci.dim_bus_addr);
    }
#endif
#ifndef __aarch64__
#if (ACP_SRAM_SIZE > 0)
    if (g_dev[hix].pci.acp_addr) {
        dma_free_coherent(&g_dev[hix].pci.pci_hydra->dev, ACP_SRAM_SIZE, g_dev[hix].pci.acp_addr, g_dev[hix].pci.acp_bus_addr);
    }
#endif
#endif
dma_alloc_coherent_fail:
    iounmap(g_dev[hix].pci.hydra_aon_base_addr);
ioremap_hydra_aon_base_addr_fail:
    iounmap(g_dev[hix].pci.hydra_sram_base_addr);
ioremap_hydra_sram_base_addr_fail:
    iounmap(g_dev[hix].pci.hydra_exp_core_base_addr);
ioremap_hydra_exp_core_base_addr_fail:
    flush_workqueue(g_dev[hix].pci.workq);
    destroy_workqueue(g_dev[hix].pci.workq);
create_workqueue_fail:
pci_set_dma_mask_fail:
    pci_release_regions(pPciDev);
pci_request_regions_fail:
find_vdr_fail:
    pci_disable_device(pPciDev);
pci_enable_device_fail:
    g_dev[hix].pci.pci_hydra = 0;
    return result;
}


//--------------------------------------------------------------------------
// hydra_remove()
//--------------------------------------------------------------------------
static void hydra_remove(struct pci_dev *pPciDev)
{
    struct hydra_dev *hdev = pci_get_drvdata(pPciDev);
    uint32_t hix;

    if (! hdev) {
        pr_err("hydra_remove: no device!\n");
        return;
    }

    hix = hdev->index;

    pr_info("hydra_remove instance #%d\n", hix);

    hydra_timer_stop();

    if (pPciDev->device == PCIE_EP_HYDRA_BOOTLOADER_DID) {
        pr_info("OTP device was pulled\n");
    }

    hdev = pci_get_drvdata(pPciDev);

    if (pPciDev->device != PCIE_EP_HYDRA_BOOTLOADER_DID) {

        sysfs_remove_file(&g_dev[hix].hydra_sys_dev->kobj, &dev_attr_ready.attr);

        blocking_notifier_call_chain(&g_notifiers, hix, (void *)0);

        // Clean up HSI devices.
        hsi_uninit(hix);
    }

#ifndef __aarch64__
#if (ACP_SRAM_SIZE > 0)
    if (g_dev[hix].pci.acp_addr) {
        dma_free_coherent(&g_dev[hix].pci.pci_hydra->dev, ACP_SRAM_SIZE, g_dev[hix].pci.acp_addr, g_dev[hix].pci.acp_bus_addr);
    }
#endif
#endif

#if (DIM_MEM_SIZE > 0)
    if (g_dev[hix].pci.dim_addr) {
        dma_free_coherent(&g_dev[hix].pci.pci_hydra->dev, DIM_MEM_SIZE, g_dev[hix].pci.dim_addr, g_dev[hix].pci.dim_bus_addr);
    }
#endif

    hydra_sysfs_devs_destroy(hix);

    // PCI cleanup.
    if (g_dev[hix].pci.pci_hydra) {
        pr_info("removing hydra interface\n");

        // Undo the interrupts.
        irq_uninit(hix);

        // Workq cleanup.
        if (g_dev[hix].pci.workq) {
            flush_workqueue(g_dev[hix].pci.workq);
            destroy_workqueue(g_dev[hix].pci.workq);
        }

        if (pPciDev->device != PCIE_EP_HYDRA_BOOTLOADER_DID) {
            // Vcam clean-up.
            pr_info("vcam cleanup\n");
            vcam_uninit();
        }

#if 0
        //
        // DON'T WANT TO BE DOING THIS FOR HOT-PLUG SCENARIOS!
        //
        // Free GSM region.
        if (g_shddr_bus_addr) {
            // Make sure this only gets freed after the last hydra is removed.
            if ((g_dev[0].pci.pci_hydra == 0) || (g_dev[1].pci.pci_hydra == 0)) {
                pr_info("freeing SH-DDR\n");
                dma_free_coherent(&g_dev[hix].pci.pci_hydra->dev, SHDDR_SIZE, g_shddr_addr, g_shddr_bus_addr);
                g_shddr_addr = 0;
            } else {
                pr_info("postponing freeing of SH-DDR\n");
            }
        }
#endif
        pr_info("removing hydra device\n");
        iounmap(g_dev[hix].pci.hydra_exp_core_base_addr);
        iounmap(g_dev[hix].pci.hydra_sram_base_addr);
        iounmap(g_dev[hix].pci.hydra_aon_base_addr);
        pci_release_regions(g_dev[hix].pci.pci_hydra);
        pci_disable_device(g_dev[hix].pci.pci_hydra);

        g_dev[hix].pci.pci_hydra = 0;
    }
}


//--------------------------------------------------------------------------
// hydra_pm_suspend()
//--------------------------------------------------------------------------
static int hydra_pm_suspend(struct device *device)
{
    pr_warn("**************************** SUSPENDING\n");

    return 0;
}


//--------------------------------------------------------------------------
// hydra_pm_resume()
//--------------------------------------------------------------------------
static int hydra_pm_resume(struct device *device)
{
    pr_warn("**************************** RESUMING\n");

    return 0;
}


//--------------------------------------------------------------------------
// hydra_pm_idle()
//--------------------------------------------------------------------------
static int hydra_pm_idle(struct device *device)
{
    pr_warn("**************************** IDLE\n");
#if 0
    PDEVICE_EXTENSION pDevExt = pci_get_drvdata(pPciDev);
    INT32 pmStatus = 0;

    pci_set_power_state(pPciDev, PCI_D0);
    pci_restore_state(pPciDev);

    if (pDevExt != NULL) {
        //  ExpressoResume_operation(pDevExt);
        DBG_VERBOSE("ExpressoResume\n");
    }
    return pmStatus;
#endif
    return 0;
}


//-------------------------------------------------------------------------------------------
// hydra_irq_req()
//-------------------------------------------------------------------------------------------
int hydra_irq_req(int hix, int chan_id, char *name)
{
    struct irq_desc *irq_desc;
    struct cpumask mask = {0};
    int iret;
#ifdef HYDRA_PM
    int pm_ret;
#endif

    if (g_dev[hix].pci.msix_tab[chan_id].vector) {
        return -1;
    }

    g_dev[hix].pci.msix_tab[chan_id].vector = pci_irq_vector(g_dev[hix].pci.pci_hydra, chan_id);
    g_dev[hix].pci.irq_info[chan_id].vector = g_dev[hix].pci.msix_tab[chan_id].vector;
    g_dev[hix].pci.irq_info[chan_id].cookie = (void *)(uintptr_t)(hix | (chan_id << 8));

    if (g_irq_thr) {
        iret = hydra_req_irq(hix, chan_id, g_dev[hix].pci.msix_tab[chan_id].vector, hydra_intr, hydra_intr_thr, 1, name, g_dev[hix].pci.irq_info[chan_id].cookie);
    } else {
        iret = hydra_req_irq(hix, chan_id, g_dev[hix].pci.msix_tab[chan_id].vector, hydra_intr, 0, 0, name, g_dev[hix].pci.irq_info[chan_id].cookie);
    }

#ifdef HYDRA_PM
    /* pm interrupt can wake up system */
    pm_ret = enable_irq_wake(g_dev[hix].pci.msix_tab[chan_id].vector);
    if (pm_ret) {
        pr_err("failed to set irq wake on #%d (%d): %d\n", g_dev[hix].pci.msix_tab[chan_id].vector, chan_id, pm_ret);
    }
#endif
    // Make irq name a little more informative.
    irq_desc = irq_to_desc(g_dev[hix].pci.msix_tab[chan_id].vector);
    irq_desc->action->name = name;

    // If requested, change the cpu that stream irq handlers are assigned to.
    if (g_irq_aff) {
        pr_info("Change irq cpu to %ld\n", g_irq_aff);
        cpumask_set_cpu(g_irq_aff, &mask);
        irq_set_affinity_hint(g_dev[hix].pci.msix_tab[chan_id].vector, &mask);
    }

    if (iret == 0) {
        if ((!chan_id) || g_irq_dbg) {
            pr_info("    MSI #%d, vec=%d\n", chan_id, g_dev[hix].pci.msix_tab[chan_id].vector);
        }
        tasklet_init(&g_dev[hix].pci.tsklt[chan_id], hydra_intr_bot, hix | (chan_id << 8));
    } else {
        pr_err("FAILED to get irq #%d (%d), iret=%d\n", chan_id, g_dev[hix].pci.msix_tab[chan_id].vector, iret);
    }

    return iret;
}


//--------------------------------------------------------------------------
// alogx_open()
//--------------------------------------------------------------------------
static int alogx_open(struct inode *pInode, struct file *pFile)
{
    int hix, maj;
    struct alogx_priv *priv;

    maj = imajor(pInode);
    hix = (maj == MAJOR(g_alog[0].devt)) ? 0 : 1;

    pr_info("Open alog for hydra #%d, major=%d\n", hix, maj);

    priv = kmalloc(sizeof(struct alogx_priv),GFP_KERNEL);
    if (!priv) {
        return -ENOMEM;
    }

    priv->dev = &g_dev[hix];
    priv->current_off = (uint32_t)-1;

    pFile->private_data = priv;

    return 0;
}


//--------------------------------------------------------------------------
// alogx_release()
//--------------------------------------------------------------------------
static int alogx_release(struct inode *pInode, struct file *pFile)
{
    kfree(pFile->private_data);
    return 0;
}


//--------------------------------------------------------------------------
// alogx_read()
//--------------------------------------------------------------------------
static ssize_t alogx_read(struct file *filp, char __user *buf, size_t count, loff_t *off)
{
    struct alogx_priv *priv = (struct alogx_priv *)filp->private_data;
    struct hydra_dev *hdev = priv->dev;
    uint32_t current_off = priv->current_off;

    uint32_t left_to_copy = 0;
    uint32_t to_copy = 0;
    uint32_t log_buff_ptr = 0;
    int wait_ret = 0;

    if (!hdev) {
        pr_err("alogx_read: no device!\n");
        return -EIO;
    }

    if (count > ALOG_RING_LEN)
    {
        count = ALOG_RING_LEN;
    }

    log_buff_ptr = hdev->alog_buff_ptr;

    if (current_off == (uint32_t)-1)
    {
        current_off = log_buff_ptr + 1;
    }

    if (current_off == log_buff_ptr && (filp->f_flags & O_NONBLOCK))
    {
        return -EAGAIN;
    }

    wait_ret = wait_event_interruptible(alog_ring_wait, current_off != hdev->alog_buff_ptr);
    if (wait_ret)
    {
        return wait_ret;
    }

    //re-read
    log_buff_ptr = hdev->alog_buff_ptr;

    //pr_info("ring_read hix: %d, count: %lu, log_buff_ptr: %u, %p %p, current_off: %u\n", hdev->index, count, log_buff_ptr, buf, hdev->alog_buff, current_off);

    left_to_copy = count;
    if (current_off > log_buff_ptr)
    {
        to_copy = min(left_to_copy, ALOG_RING_LEN - current_off);
        if (copy_to_user(buf, hdev->alog_buff + current_off, to_copy))
        {
            return -EFAULT;
        }
        left_to_copy -= to_copy;
        current_off = 0;
        buf += to_copy;
    }

    if (left_to_copy > 0 && current_off < log_buff_ptr)
    {
        to_copy = min(left_to_copy, log_buff_ptr - current_off);
        if (copy_to_user(buf, hdev->alog_buff + current_off, to_copy))
        {
            return -EFAULT;
        }
        current_off += to_copy;
        left_to_copy -= to_copy;
    }

    priv->current_off = current_off;

    return count - left_to_copy;
}


static void disable_all_irqs(uint32_t hix)
{
    int i;

    for (i = 0; i < g_dev[hix].pci.irq_count; i++) {
        if (g_dev[hix].pci.irq_info[i].vector) {
            disable_irq(g_dev[hix].pci.irq_info[i].vector);
        }
    }
}


//--------------------------------------------------------------------------
// hydra_devices_attached()
//--------------------------------------------------------------------------
int hydra_devices_attached(struct shmem_map **map)
{
    int count = 0;

    *map = g_shddr_map;

    if (g_dev[HIX_HYDRA_PRI].pci.pci_hydra) {
        count++;
    }

    if (g_dev[HIX_HYDRA_SEC].pci.pci_hydra) {
        count++;
    }

    return count;
}
EXPORT_SYMBOL(hydra_devices_attached);


//--------------------------------------------------------------------------
// hydra_get_irq()
//--------------------------------------------------------------------------
uint32_t hydra_get_irq(uint32_t hydra_ix, uint32_t chan)
{
    if (hydra_ix > HIX_HYDRA_SEC) {
        pr_err("hydra_get_irq: bad hix = %d\n", hydra_ix);
        return 0;
    }
    if (chan > HYDRA_LAST_IRQ_VEC) {
        pr_err("hydra_get_irq: bad chan = %d\n", chan);
        return 0;
    }

    if (g_dev[hydra_ix].pci.pci_hydra) {
        return pci_irq_vector(g_dev[hydra_ix].pci.pci_hydra, chan);
    }

    return 0;
}
EXPORT_SYMBOL(hydra_get_irq);


//--------------------------------------------------------------------------
// hydra_req_irq()
//--------------------------------------------------------------------------
uint32_t hydra_req_irq(int hix, int irq_ix, uint32_t vector, irq_handler_t handler, irq_handler_t th_handler, int threaded, const char *devname, void *cookie)
{
    int iret;

    //pr_info("%s:  hix=%d, ix=%d, vec=%d, cookie=0x%llx\n", __func__, hix, irq_ix, vector, (uint64_t)cookie);

    if (threaded) {
        iret = request_threaded_irq(vector, handler, th_handler, 0, devname, cookie);
    } else {
        iret = request_irq(vector, handler, 0, devname, cookie);
    }

    if (! iret) {
        g_dev[hix].pci.irq_info[irq_ix].cookie = cookie;
        g_dev[hix].pci.irq_info[irq_ix].vector = vector;
    }

    return iret;
}
EXPORT_SYMBOL(hydra_req_irq);


//--------------------------------------------------------------------------
// hydra_free_irq()
//--------------------------------------------------------------------------
const void *hydra_free_irq(int hix, int irq_ix, uint32_t vector, void *cookie)
{
    const void *iret;

    //pr_info("%s: hix=%d, ix=%d, vec=%d, cookie=0x%llx\n", __func__, hix, irq_ix, vector, (uint64_t)cookie);

    iret = free_irq(vector, cookie);

    g_dev[hix].pci.irq_info[irq_ix].cookie = 0;
    g_dev[hix].pci.irq_info[irq_ix].vector = 0;

    return iret;
}
EXPORT_SYMBOL(hydra_free_irq);


//--------------------------------------------------------------------------
// hydra_irq_post()
//--------------------------------------------------------------------------
void hydra_irq_post(uint32_t hydra_ix, uint32_t irq)
{
    if (g_dev[hydra_ix].pci.hydra_aon_base_addr) {
        //pr_info("Sending int to hydra #%d (%p)!\n", hydra_ix, (uint32_t *)g_dev[hydra_ix].pci.hydra_aon_base_addr + irq);
        if (g_dev[hydra_ix].pci.chip_id == HYDRA_A0_CHIP_ID) {
            *((uint32_t *)g_dev[hydra_ix].pci.hydra_aon_base_addr + irq) = 0xffffffff; //pci_host_intr0
        } else if (g_dev[hydra_ix].pci.chip_id == HYDRA_B0_CHIP_ID) {
            *((uint32_t *)((uint8_t *)g_dev[hydra_ix].pci.hydra_aon_base_addr + 0x10)) = 1 << irq; //pci_host_intr_reg_status
        } else {
            pr_err("Unknown Hydra chip id\n");
        }
    } else {
        pr_err("Unable to send int to hydra!\n");
    }
}
EXPORT_SYMBOL(hydra_irq_post);

void hydra_hcp_irq_post(uint32_t hydra_ix) {
    hydra_irq_post(hydra_ix, 0);
}
EXPORT_SYMBOL(hydra_hcp_irq_post);

//-------------------------------------------------------------------------------------------
// gac_time_get()
//-------------------------------------------------------------------------------------------
uint64_t gac_time_get(void)
{
    unsigned int tstamper_offset = 0;
    uint64_t ts = 0;
    int b0 = 0;
    if (g_dev[HIX_HYDRA_PRI].pci.chip_id == HYDRA_A0_CHIP_ID) {
        tstamper_offset = 0xD0;
        b0 = 0;
    } else if (g_dev[HIX_HYDRA_PRI].pci.chip_id == HYDRA_B0_CHIP_ID) {
        tstamper_offset = 0xF8;
        b0 = 1;
    } else {
        pr_err("Unknown Hydra chip id\n");
        return 0;
    }
    // Refer to gac_show() for info on how/why the temporary ugliness below works.
    // This function always returns the timestamp from the primary hydra,
    // while gac_show() will return it from whichever hydra it's called for.
    if (g_dev[HIX_HYDRA_PRI].pci.hydra_aon_base_addr) {
        uint8_t *tstamper_base;
        tstamper_base = (uint8_t *)g_dev[HIX_HYDRA_PRI].pci.hydra_aon_base_addr + 0x1000;
        ts = *(uint64_t *)&tstamper_base[tstamper_offset];

        if (b0) {
            ts = timestamp_b0_rescale(ts);
        }
    }

    return ts;
}
EXPORT_SYMBOL(gac_time_get);


//-------------------------------------------------------------------------------------------
// register_hydra_notifier()
//-------------------------------------------------------------------------------------------
int register_hydra_notifier(struct notifier_block *nb)
{
    blocking_notifier_chain_register(&g_notifiers, nb);
    return 0;
}
EXPORT_SYMBOL(register_hydra_notifier);


//-------------------------------------------------------------------------------------------
// unregister_hydra_notifier()
//-------------------------------------------------------------------------------------------
int unregister_hydra_notifier(struct notifier_block *nb)
{
    blocking_notifier_chain_unregister(&g_notifiers, nb);
    return 0;
}
EXPORT_SYMBOL(unregister_hydra_notifier);

void hydra_get_sram_hcp_ptr(int hix, void **ptr)
{
    *ptr = (void *)((uint64_t)g_dev[hix].pci.hydra_sram_base_addr + OFF_HCP_HYDRA);
}
EXPORT_SYMBOL(hydra_get_sram_hcp_ptr);
