// SPDX-License-Identifier: GPL-2.0
//
// (C) Copyright 2020-2022
// Magic Leap, Inc. (COMPANY)
//

#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/version.h>
#include <asm/io.h>
#include <linux/pci.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs_struct.h>
#include <linux/uaccess.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/pci.h>
#include <linux/errno.h>  // EINVAL

#include "gsm.h"
#include "gsm_jrtc.h"
#include "gsm_cvip.h"
#include "gsm_spinlock.h"

static unsigned int mydebug;
module_param(mydebug, uint, 0644);
MODULE_PARM_DESC(mydebug, "Activates local debug info");

static unsigned int gactest;
module_param(gactest, uint, 0644);
MODULE_PARM_DESC(gactest, "Enables gac test on module loading");

static unsigned int gac_read_iters = 100;
module_param(gac_read_iters, uint, 0644);
MODULE_PARM_DESC(gac_read_iters, "Number of gac reads if gactest is enabled");

static unsigned int gsm_spinlock_init = 1;
module_param(gsm_spinlock_init, uint, 0644);
MODULE_PARM_DESC(gsm_spinlock_init, "Enables init of gsm spinlock system.");


static unsigned int gsm_semaphore_init = 1;
module_param(gsm_semaphore_init, uint, 0644);
MODULE_PARM_DESC(gsm_semaphore_init, "Enables init of gsm semaphore system.");

static unsigned int gsm_dsp_clk_init = 1;
module_param(gsm_dsp_clk_init, uint, 0644);
MODULE_PARM_DESC(gsm_dsp_clk_init, "Enables init of gsm dsp clk config.");
#define MODULE_NAME "gsm"

#if defined(__i386__) || defined(__amd64__)
//provided by amd
extern void *get_sintf_base(int);
#endif

// TODO(sjain): add meaningful log level defines rather than
// just using literals for dprintk

#define dprintk(level, fmt, arg...)             \
  do {                                          \
    if (level <= mydebug)                       \
      pr_info(MODULE_NAME ": "fmt, ## arg);     \
  } while (0)

#define dprintkrl(level, fmt, arg...)           \
  do {                                          \
    if (level <= mydebug)                       \
      printk_ratelimited(KERN_INFO MODULE_NAME ": " fmt, ## arg); \
  } while (0)

uint8_t *gsm_backing = NULL;
EXPORT_SYMBOL(gsm_backing);

#define DEVICE_NAME "gsm"
#define GSM_FILE_COUNT 5
const char *device_names[GSM_FILE_COUNT] = {
	"jrtc_ns",
	"jrtc_us",
	"jrtc_ms",
	"gsm",
	"gsm_mem"
};
enum gsm_device_types {
	// JRTC devices
	GSM_DEVICE_JRTC_NS,
	GSM_DEVICE_JRTC_US,
	GSM_DEVICE_JRTC_MS,
	GSM_DEVICE_JRTC_LAST = GSM_DEVICE_JRTC_MS,
	// GSM memory devices
	GSM_DEVICE_GSM,
	GSM_DEVICE_GSM_MEM,
	// Last enum
	GSM_DEVICE_LAST
};
// Assert that the file count matches the enums
static_assert(GSM_DEVICE_LAST == GSM_FILE_COUNT);

#if defined(__i386__) || defined(__amd64__)
static struct pci_dev *pci_dev = NULL;
#endif

#define CVIP_S_INTF_BAR 2
#define CVCORE_GSM_SLOW_CLK_REG_OFFSET 0x40
#ifndef CVCORE_GSM_REG_SIF_VADDR
#define CVCORE_GSM_REG_SIF_VADDR 0x500000
#endif

// GSM AXI protection address offsets
#define GSM_REGION_1_PROTECT_CONFIG_OFFSET             (0x580)
#define GSM_REGION_1_PROTECT_RANGE_START_REG           (0x0)
#define GSM_REGION_1_PROTECT_RANGE_END_REG             (0x8)
#define GSM_REGION_1_PROTECT_READ_MASK_REG             (0x10)
#define GSM_REGION_1_PROTECT_WRITE_MASK_REG            (0x12)
#define GSM_REGION_1_PROTECT_STATUS_REG                (0x20)
// GSM AXI protection configuration values
#define GSM_REGION_1_BASE_OFFSET_START                 (0x0)
#define GSM_REGION_1_BASE_OFFSET_END                   (0x7FFFFF)
#define GSM_REGION_1_ALL_CORES_READ_ACCESS_MASK        (0xFFFF)
#define GSM_REGION_1_ALL_CORES_WRITE_ACCESS_MASK       (0xFFFF)
#define GSM_REGION_1_ENABLE_RESTRICTION_POLICY         (0x1)

uint8_t cvip_up;
static struct class *class = NULL;
static dev_t dev = 0;
static int major_number;
static struct device *device;
static struct cdev cdev;

uint8_t gsm_core_is_cvip_up(void)
{
	int iteration_limit = 100;

	// if something happened and gsm wasn't setup, for whatever reason
	// auto-return with 0 (null-ptr crash otherwise)
	if(NULL == gsm_backing)
		return 0;

	while(--iteration_limit)
	{
		if(0x0 != *(unsigned int *)(gsm_backing + CVCORE_GSM_SLOW_CLK_REG_OFFSET)){
			return 1;
		}
	}
	return 0;
}
EXPORT_SYMBOL(gsm_core_is_cvip_up);

#if defined(__i386__) || defined(__amd64__)
int gsm_mmap_x86(struct file *filp, struct vm_area_struct *vma){
	unsigned int minor_num = iminor(filp->f_inode);
	int status = 0;

	switch(minor_num) {
		case GSM_DEVICE_GSM:{ // /dev/gsm
			vma->vm_page_prot = pgprot_device(vma->vm_page_prot);
			vma->vm_pgoff = (CVCORE_GSM_REG_SIF_VADDR) >> PAGE_SHIFT;
			status = pci_mmap_resource_range(pci_dev, CVIP_S_INTF_BAR, vma, pci_mmap_mem, 0);
			return status;
		}
		case GSM_DEVICE_GSM_MEM:{ // /dev/gsm_mem
			unsigned long len = vma->vm_end - vma->vm_start;
			unsigned long offset = vma->vm_pgoff << PAGE_SHIFT;

			dprintk(2, "gsm_mem mmap request. offset: 0x%lx len: %lu\n", offset, len);

			// Offset check
			if (offset < GSM_X86_DEV_GSM_MEM_BASE ||
				offset >= (GSM_X86_DEV_GSM_MEM_BASE + GSM_X86_DEV_GSM_MEM_SIZE)) {
				dprintk(0, "Invalid gsm_mem mmap offset. offset: 0x%lx len: %lu\n", offset, len);
				return -EINVAL;
			}

			// Length check
			if (len > PAGE_ALIGN(GSM_X86_DEV_GSM_MEM_SIZE)) {
				dprintk(0, "Invalid gsm_mem mmap length. offset: 0x%lx len: %lu\n", offset, len);
				return -EINVAL;
			}

			dprintk(2, "gsm_mem mmap calculated addr: 0x%lx\n", (CVCORE_GSM_MEM_SIF_VADDR + offset));
			vma->vm_page_prot = pgprot_device(vma->vm_page_prot);
			vma->vm_pgoff = (CVCORE_GSM_MEM_SIF_VADDR + offset) >> PAGE_SHIFT;
			status = pci_mmap_resource_range(pci_dev, CVIP_S_INTF_BAR, vma, pci_mmap_mem, 0);
			return status;
		}
		default:{
			printk(KERN_INFO "GSM " "Minor Num Invalid\n");
			return -42;
		}
	}
}
#else
int gsm_mmap_arm(struct file *filp, struct vm_area_struct *vma) {
	int ret;
	unsigned int minor_num;
	unsigned long vma_len, offset, pfn_gsm;

	vma_len = vma->vm_end - vma->vm_start;
	offset = vma->vm_pgoff << PAGE_SHIFT;
	minor_num = iminor(filp->f_inode);

	dprintk(2, "%s: mmap request, minor: %d offset: 0x%lx len: %lu\n",
		__func__, minor_num, offset, vma_len);

	switch(minor_num) {
		// /dev/gsm - Maps GSM memory for the userspace GSM API
		case GSM_DEVICE_GSM:
			// Sanity check range
			if ((offset + vma_len) > CVCORE_GSM_SIZE) {
				dprintk(0, "%s: Invalid map range! offset 0x%lx len: %lu\n",
					__func__, offset, vma_len);
				return -EINVAL;
			}

			// Set as device type
			vma->vm_page_prot = pgprot_device(vma->vm_page_prot);
			// Physical address of gsm memory
			pfn_gsm = (CVCORE_GSM_PADDR >> PAGE_SHIFT) + vma->vm_pgoff;

			dprintk(2, "%s: Mapping GSM addr: 0x%lx, len: %lu\n",
				__func__, pfn_gsm << PAGE_SHIFT, vma_len);

			ret = remap_pfn_range(vma,
				vma->vm_start,
				pfn_gsm,
				vma_len,
				vma->vm_page_prot);
			if (ret) {
				dprintk(0, "%s: remap_pfn_range failed! ret: %d\n",
					__func__, ret);
				return ret;
			}
			break;
		default:
			dprintk(0, "Invalid minor number: %d\n", minor_num);
			return -EINVAL;
	}

	return 0;
}
#endif

static ssize_t gsm_read(struct file *filp, char __user *buf, size_t count, loff_t *pos) {
	unsigned int minor_num = iminor(filp->f_inode);
	uint64_t value;
	//uint64_t device_address;
	char kernel_buffer[64];
	// Only allow the jrtc devices
	if(minor_num > GSM_DEVICE_JRTC_LAST) {
		return 0;
	}
	if(!cvip_up){
		cvip_up = gsm_core_is_cvip_up();
	}
	if(cvip_up){
		switch(minor_num) {
			case GSM_DEVICE_JRTC_NS:
				value = gsm_jrtc_get_time_ns();
				break;
			case GSM_DEVICE_JRTC_US:
				value = gsm_jrtc_get_time_us();
				break;
			case GSM_DEVICE_JRTC_MS:
				value = gsm_jrtc_get_time_ms();
				break;
			default:
				return 0;
		}
	}else{
		dprintk(0, "/dev/jrtc cvip not up\n");
		value = 0x0;
	}
	snprintf(kernel_buffer, 64, "%lli\n", value);
	if(*pos == 0)
		return simple_read_from_buffer(buf, count, pos, kernel_buffer, strlen(kernel_buffer));
	return 0;
}

static struct file_operations f_ops = {
	.owner = THIS_MODULE,
	.read = gsm_read,
#if defined(__i386__) || defined(__amd64__)
	.mmap = gsm_mmap_x86
#else
	.mmap = gsm_mmap_arm
#endif
};

static void destroy_fs_device(void) {
	int i;
    cdev_del(&cdev);
	for(i=0; i<GSM_FILE_COUNT;++i)
	{
    	device_destroy(class, MKDEV(major_number, i));
	}
    class_destroy(class);
    unregister_chrdev_region(dev, GSM_FILE_COUNT);
}

static void create_fs_device(void) {
    int err = 0;
	int i = 0;
    err = alloc_chrdev_region(&dev, 0, GSM_FILE_COUNT, DEVICE_NAME);
    if (err < 0) {
    }
    major_number = MAJOR(dev);
    class = class_create(THIS_MODULE, DEVICE_NAME);
    if (IS_ERR(class)) {
        err = PTR_ERR(class);
        unregister_chrdev_region(dev, 1);
    }
    cdev_init(&cdev, &f_ops);
    cdev.owner = THIS_MODULE;
	dev = MKDEV(major_number, 0);
	err = cdev_add(&cdev, dev, GSM_FILE_COUNT);
	if (err < 0) {
		cdev_del(&cdev);
		class_destroy(class);
		unregister_chrdev_region(dev, GSM_FILE_COUNT);
		return;
	}
	/* 16 different gsm regions */
	for(i=0;i<GSM_FILE_COUNT; ++i){
		dev = MKDEV(major_number, i);
		device = device_create(class, NULL, dev, NULL, "%s", device_names[i]);
		if (IS_ERR(&device)) {
		}
	}
}

#if defined(__i386__) || defined(__amd64__)
#define PCI_DEVICE_ID_CVIP 0x149d


#else
static void gsm_semaphore_subsystem_init(void)
{
	uint32_t index;
	for (index = 0; index < GSM_SEM_CTRL_WORD_COUNT; index++)
		gsm_raw_write_32(GSM_RESERVED_GSM_SEM_BASE + (index * 4), 0);
}

static void gsm_spinlock_subsystem_init(void)
{
	uint32_t index;

	// Clear the CVIP mutex bitmap memory region
	for (index = 0; index < SPINLOCK_CTRL_WORDS; index++)
		gsm_raw_write_32(GSM_RESERVED_CVIP_SPINLOCK_BITMAP_BASE + (index * SPINLOCK_CTRL_WORD_SIZE), 0);

	// Clear the CVIP mutex TAS nibble memory region
	for (index = 0; index < SPINLOCK_NIBBLE_WORDS; index++)
		gsm_raw_write_32(GSM_RESERVED_CVIP_SPINLOCK_NIBBLE_BASE + (index * SPINLOCK_NIBBLE_WORD_SIZE), 0);
}

static void cvip_spinlock_subsystem_init(void)
{
	uint32_t index;

	/**
	* Clear the CVIP spinlock bitmap memory region
	*/
	for (index = 0; index < TAS_CTRL_WORDS; index++) {
		gsm_raw_write_32(GSM_RESERVED_CVIP_MUTEX_BITMAP_BASE + (index * TAS_CTRL_WORD_SIZE), 0);
	}
	/**
	* Clear the CVIP spinlock TAS nibble memory region
	*/
	for (index = 0; index < TAS_NIBBLE_WORDS; index++) {
		gsm_raw_write_32(GSM_RESERVED_CVIP_MUTEX_NIBBLE_BASE + (index * TAS_NIBBLE_WORD_SIZE), 0);
	}
	/**
	* Clear the CVCORE predefined spinlock memory region
	*/
	for (index = 0; index < GSM_PREDEF_CVCORE_LOCKS_SIZE; index += 4) {
		gsm_raw_write_32(GSM_RESERVED_PREDEF_CVCORE_LOCKS_1 + index, 0);
	}
}

static void cvip_integer_subsystem_init(void)
{
	uint32_t index;

	/**
	* Clear the control bitmap region specifically for non-predef integers,
	* as Non-predefs are not managed by the control bitmap
	*/
	for (index = 0; index < CVIP_INTEGER_CTRL_WORDS; index++) {
		gsm_raw_write_32(GSM_RESERVED_CVIP_INTEGER_BITMAP_BASE + (index * CVIP_INTEGER_CTRL_WORD_SIZE), 0);
	}
}

static void gsm_dsp_clk_config_init(void)
{
	uint32_t index;
	for (index = 0; index < (GSM_RESERVED_DSP_CLK_CONFIG_SIZE/4); index++)
		gsm_raw_write_32(GSM_RESERVED_DSP_CLK_CONFIG + (index * 4), 0);
}

static void gsm_dsp_kpi_stats_init(void)
{
	uint32_t index;
	for (index = 0; index < (GSM_RESERVED_DSP_KPI_SIZE/4); index++)
		gsm_raw_write_32(GSM_RESERVED_DSP_KPI_MEM_BASE + (index * 4), 0);

	for (index = 0; index < (GSM_DSP_PANIC_RECOVERY_STATS_SIZE/4); index++)
		gsm_raw_write_32(GSM_RESERVED_DSP_PANIC_RECOVERY_STATS + (index * 4), 0);
}

// NOTE: Currently, we are providing R/W access from all the cores to the entire GSM space
// and configuring only region 1 registers to acheive this. There are 8 regions in total
// that can be used (configured) to suit our restriction policy needs in future.
// The API will be updated in the future when more restricted access policy will be put into place.
// The API usage and implementation is based on the fact that this is a one-time configuration.
// For more info, refer to "AXI Protection Subsystem (AXIP)" section in
// https://drive.google.com/drive/u/0/folders/1mjXQ3EVxWTIB1eSuEB4YWxdZ4_j4OUFI
// The doc could be outdated but provides enough information on the functioning of AXIP unit
// in GSM and it's registers.
static void gsm_configure_axi_protection(void)
{
	void *config_base = gsm_backing + GSM_REGION_1_PROTECT_CONFIG_OFFSET;

	writel_relaxed(GSM_REGION_1_BASE_OFFSET_START, config_base + GSM_REGION_1_PROTECT_RANGE_START_REG);
	writel_relaxed(GSM_REGION_1_BASE_OFFSET_END, config_base + GSM_REGION_1_PROTECT_RANGE_END_REG);
	writew_relaxed(GSM_REGION_1_ALL_CORES_READ_ACCESS_MASK, config_base + GSM_REGION_1_PROTECT_READ_MASK_REG);
	writew_relaxed(GSM_REGION_1_ALL_CORES_WRITE_ACCESS_MASK, config_base + GSM_REGION_1_PROTECT_WRITE_MASK_REG);
	writeb_relaxed(GSM_REGION_1_ENABLE_RESTRICTION_POLICY, config_base + GSM_REGION_1_PROTECT_STATUS_REG);
}

#endif

static int __init gsm_init(void)
{
	// Set JRTC address via pcie
	// See: apu_s_intf_x86 in ml-mero-cvip-mortable-common.dtsi
#if defined(__i386__) || defined(__amd64__)
	int status = 0;
	dprintk(2, "jrtc pcie_driver_register status %i\n", status);
	pci_dev = pci_get_device(PCI_VENDOR_ID_AMD, PCI_DEVICE_ID_CVIP, pci_dev);
	if (!pci_dev) {
		dprintk(0, "failed to find PCIe. pci_get_device failed.\n");
		return -ENODEV;
	}
	gsm_backing = (uint8_t *)get_sintf_base(3);
	if (!gsm_backing) {
		dprintk(0, "failed to get gsm_backing address from sintf_base\n");
		return -ENXIO;
	}
	dprintk(0, "gsm_backing kernel virtual addr = %p\n", gsm_backing);
#else
	int i;
	extern void gsm_jrtc_init(void);

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 8, 0)
	gsm_backing = ioremap_nocache(CVCORE_GSM_PADDR, CVCORE_GSM_SIZE);
#else
	gsm_backing = ioremap_uc(CVCORE_GSM_PADDR, CVCORE_GSM_SIZE);
#endif

	if (!gsm_backing) {
		dprintk(0, "failed to load gsm module; ioremap failed\n");
		return -1;
	}
	if (gactest) {
		dprintk(0, "gac ts read, iters=%d\n", gac_read_iters);
		for (i = 0; i < gac_read_iters; i++)
			dprintk(0, "%llx\n", *(uint64_t *)(gsm_backing + 0x20));
		dprintk(0, "..done..\n");
	}
	dprintk(2, "gsm_backing kernel virtual addr = %p\n", gsm_backing);
	if (gsm_spinlock_init) {
		gsm_spinlock_subsystem_init();
	}
	if (gsm_semaphore_init) {
		gsm_semaphore_subsystem_init();
	}
	if (gsm_dsp_clk_init) {
		gsm_dsp_clk_config_init();
	}

	gsm_dsp_kpi_stats_init();

	cvip_spinlock_subsystem_init();

	cvip_integer_subsystem_init();

	gsm_configure_axi_protection();

	gsm_jrtc_init();
#endif
	create_fs_device();
	dprintk(0, "Successfully loaded gsm module\n");
	return 0;
}

static void __exit gsm_exit(void)
{
	destroy_fs_device();
#if defined(__i386__) || defined(__amd64__)

#else
	iounmap(gsm_backing);
#endif
	dprintk(0, "Unloading gsm module\n");
}

#if defined(__i386__) || defined(__amd64__)
module_init(gsm_init);
#else
core_initcall(gsm_init);
#endif
module_exit(gsm_exit);

MODULE_AUTHOR("Sumit Jain<sjain@magicleap.com>");
MODULE_DESCRIPTION("gsm module");
MODULE_LICENSE("GPL");
#if defined(__i386__) || defined(__amd64__)
MODULE_SOFTDEP("pre: cvip");
#endif
