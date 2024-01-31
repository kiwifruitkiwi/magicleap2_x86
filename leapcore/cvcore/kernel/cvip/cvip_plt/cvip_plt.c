// SPDX-License-Identifier: GPL-2.0-only
/*
 * Platform driver for the CVIP SOC.
 *
 * Copyright (C) 2022-2023 Magic Leap, Inc. All rights reserved.
 */
#include <linux/types.h>
#include <linux/err.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/uaccess.h>
#include <linux/miscdevice.h>
#include <linux/iommu.h>
#include <linux/syscalls.h>
#include <linux/gpio.h>
#include <linux/version.h>
#include <linux/completion.h>
#include <linux/scpi_protocol.h>
#include <mero_scpi.h>
#include "mero_ion.h"
#include <linux/cvip/cvip_plt/cvip_plt.h>
#include "linux/mero/cvip/cvip_nova_common.h"

#define CDEV_NAME					"cvip_plt"
#define GPIO_WEARABLE_POWER_CNTRL	(8)
#define NIC_CLK_SMN_REG_ADDR		(0x10005CAA8)
#define NIC_CLK_SMN_REG_SIZE		(0x4)

#define SMMU_TCU_SCR_REG_ADDR		(0xD1802DF8)
#define SMMU_TBU_SCR_REG_BASE_ADDR	(0xD1842DF8)
#define SMMU_SCR_REG_SIZE			(0x4)

#define CVIP_S_INTF_X86_SID 0x30002

enum {
	CVIP_PLT = 0,
	CVIP,
	CVIP_MEM,
	CVIP_XPORT,
	CVIP_PROPERTIES,
	CVIP_CRUMB,
	CVIP_PCI,
	MAX_CVIP_DEVS,
};

static int pm_state = PM_STATE_NORMAL_OPERATION;
static int last_pm_state = PM_STATE_NORMAL_OPERATION;
static struct completion power_completion = {0};
static uint32_t pm_notif_cnt;
static union boot_config bc;
/* always assume the device is locked, early param will override */
static uint32_t device_locked = 1;

struct cvip_plt_device {
	char name[16];  /* should only be PEQ or mortable */
	uint32_t boot_reason;
	uint32_t watchdog_count;
	atomic_t s2_idle;
	atomic_t nova_online;
	atomic_t xport_shared_ready;
	atomic_t xport_mlnet_ready;
	uint32_t device_locked;
	struct mutex lock;
	struct class *class;
	int major;
	struct device *dev[MAX_CVIP_DEVS];
};

static struct cvip_plt_device *cplt_dev;

struct atomic_notifier_head nova_event_notifier_list;
EXPORT_SYMBOL_GPL(nova_event_notifier_list);

struct atomic_notifier_head xport_ready_notifier_list;
EXPORT_SYMBOL_GPL(xport_ready_notifier_list);

struct atomic_notifier_head s2_idle_notifier_list;
EXPORT_SYMBOL_GPL(s2_idle_notifier_list);

struct atomic_notifier_head resume_notifier_list;
EXPORT_SYMBOL_GPL(resume_notifier_list);

static int event;
#ifdef CONFIG_CRASH_DUMP
static uint32_t dump_kernel;
#endif
static phys_addr_t shared_addr;
static phys_addr_t dump_addr;
static phys_addr_t mlnet_addr;
static struct mutex cvip_lock;

#ifdef CONFIG_CVIP_IPC
extern void send_cvip_alive(void);
#endif

#ifdef CONFIG_COMMON_CLK_SCPI
extern int cvip_fclk_init(struct kobject *kobj);
extern void cvip_fclk_exit(void);
#endif

static struct kernfs_node *cvip_sysfs_dirent;
static struct kernfs_node *s2idle_sysfs_dirent;

// We only parse the address and size from the fmr10b once, at init time.
// We need to store them so we can service sysfs requests.
static phys_addr_t crumb_addr;
static size_t crumb_size;

phys_addr_t cvip_plt_crumb_get_addr(void)
{
	return crumb_addr;
}

size_t cvip_plt_crumb_get_size(void)
{
	return crumb_size;
}

static int cvip_plt_crumb_get_from_fmr(void)
{
	struct device_node *crumb_node;
	struct resource crumb_res;
	int err;

	// Our crumb log is stored in fmr10b.  Look up
	// the location and size in the device tree.
	crumb_node = of_find_node_by_name(NULL, "fmr10b");
	if (crumb_node == NULL) {
		pr_err("cvip_plt: find_node_by_name for fmr10b returned NULL\n");
		return -ENOMEM;
	}

	err = of_address_to_resource(crumb_node, 0, &crumb_res);
	if (err) {
		pr_err("cvip_plt: can't get resource info for region fmr10b\b");
		return -ENOMEM;
	}

	crumb_size = crumb_res.end - crumb_res.start + 1;

	crumb_addr = crumb_res.start;

	return 0;
}

void cvip_plt_get_reboot_info(uint32_t *boot_reason, uint32_t *watchdog_count)
{
	*boot_reason = cplt_dev->boot_reason;
	*watchdog_count = cplt_dev->watchdog_count;

	// This was printed out at boot by read_reboot_reason(), so we don't need
	// to print it here unless we are trying to trace the code.
	dev_dbg(cplt_dev->dev[CVIP_PLT], "reboot reason %x wd_count %x\n", *
							boot_reason, *watchdog_count);
}

static int read_reboot_reason(struct cvip_plt_device *cplt)
{
	void __iomem *bcb = NULL;
	void __iomem *p2cmsg = NULL;
	int ret = 0;


	cplt_dev->boot_reason = 0;
	cplt_dev->watchdog_count = 0;

	/* BCB offset to check for boot reason */
	bcb = ioremap(BCB_OFFSET, 0x8);
	if (!bcb) {
		dev_info(cplt_dev->dev[CVIP_PLT], "Failed to iomap BCB\n");
		ret = -1;
		goto error_out;
	}

	/* P2C msg0 to check for watchdog reboot count */
	p2cmsg = ioremap(P2C_MSG0_OFFSET, 0x4);
	if (!p2cmsg) {
		dev_info(cplt_dev->dev[CVIP_PLT], "Failed to iomap PC2MSG\n");
		ret = -1;
		goto error_out;
	}

	/*
	 * The assumption here is this is checked only for the first time
	 * the cvip is booted, we should clear the boot reason after the
	 * first check.
	 */
	if (bcb) {
		cplt_dev->boot_reason = readl_relaxed(bcb + 0x4);
		if (cplt_dev->boot_reason == WATCHDOG_REBOOT && p2cmsg)
			cplt_dev->watchdog_count = readl_relaxed(p2cmsg);
		writel_relaxed(cplt_dev->boot_reason & ~WATCHDOG_REBOOT, bcb + 0x4);
	}
	dev_err(cplt_dev->dev[CVIP_PLT], "reboot reason %x wd_count %x\n",
						cplt_dev->boot_reason, cplt_dev->watchdog_count);

error_out:
	if (bcb)
		iounmap(bcb);
	if (p2cmsg)
		iounmap(p2cmsg);

	return ret;
}

int enable_usb_gadget(union boot_config *bc, uint8_t override)
{
#ifdef CONFIG_CVIP_BUILD_TYPE_USER
	sprintf(cplt_dev->name, "%s", PEQ);
	return 0;
#else
	void __iomem *usb_reg1, __iomem *usb_reg2;
	int ret = -1;

	if ((bc->bits.board_id <= HW_MORTABLE_RANGE_A1_C2) ||
	    (bc->bits.board_id == HW_MORTABLE_A1_SEC)      ||
	    (bc->bits.board_id == HW_MORTABLE_A2)          ||
	    (override))   {
		dev_err(cplt_dev->dev[CVIP_PLT], "Starting USB-Gadget\n");
		sprintf(cplt_dev->name, "%s", MORTY);
		usb_reg1 = ioremap(USB_GADGET_REG1, PAGE_SIZE);
		if (!usb_reg1) {
			dev_err(cplt_dev->dev[CVIP_PLT],
					    "failed to map USB gadget reg1\n");
			goto error_out;
		}
		iowrite8(USB_ENABLE, usb_reg1);
		iounmap(usb_reg1);

		usb_reg2 = ioremap(USB_GADGET_REG2, PAGE_SIZE);
		if (!usb_reg2) {
			dev_err(cplt_dev->dev[CVIP_PLT],
					    "failed to map USB gadget reg2\n");
			goto error_out;
		}
		iowrite8(USB_ENABLE, usb_reg2);
		ret = 0;
		iounmap(usb_reg2);
	} else {
		dev_err(cplt_dev->dev[CVIP_PLT],
			  "Non Mortable Detected - Not Starting USB-Gadget\n");
		sprintf(cplt_dev->name, "%s", PEQ);
		ret = 0;
	}

error_out:
	return ret;
#endif
}

static int enable_tbu_tcu_scr_obsrv(void)
{
	void __iomem *tbu_scr_reg;
	void __iomem *tcu_scr_reg;

	const __u32 smmu_scr_enable_val = 0x80000003;
	const __u32 tbu_index_offset = 0x2 << 16;

	const int max_tbu_cnt = 6;
	int ret = -1, tbu_idx;


	for (tbu_idx = 0; tbu_idx < max_tbu_cnt; tbu_idx++) {
		tbu_scr_reg = ioremap_nocache(SMMU_TBU_SCR_REG_BASE_ADDR + (tbu_index_offset * tbu_idx),
			SMMU_SCR_REG_SIZE);
		if (!tbu_scr_reg) {
			pr_err("Could not get va of tbu %d scr reg\n", tbu_idx);
			return ret;
		}
		writel_relaxed(smmu_scr_enable_val, tbu_scr_reg);
		iounmap(tbu_scr_reg);
	}

	tcu_scr_reg = ioremap_nocache(SMMU_TCU_SCR_REG_ADDR, SMMU_SCR_REG_SIZE);
	if (!tcu_scr_reg) {
		pr_err("Could not get va of tcu scr reg\n");
		return ret;
	}
	writel_relaxed(smmu_scr_enable_val, tcu_scr_reg);
	iounmap(tcu_scr_reg);

	return 0;
}


void notify_nova_online(void)
{
	event = NOVA_ONLINE;
	atomic_set(&cplt_dev->nova_online, 1);
	atomic_notifier_call_chain(&nova_event_notifier_list, 0, &event);

	sysfs_notify(&cplt_dev->dev[CVIP]->kobj, "cvip", "nova_alive");
}

void notify_xport_ready(uint32_t type)
{
	static uint32_t heap_type;

	heap_type = type;
	dev_info(cplt_dev->dev[CVIP_PLT], "xport heap type %d\n", type);

	switch (type) {
	case mlnet:
		atomic_set(&cplt_dev->xport_mlnet_ready, 1);
		break;
	default:
		atomic_set(&cplt_dev->xport_shared_ready, 1);
		break;
	}
	atomic_notifier_call_chain(&xport_ready_notifier_list,
				   0, (void *)&heap_type);

}

void cvip_plt_biommu_sid_ready(u32 sid)
{
	extern int arm_smmu_enable_fault_irq(void);

	if (sid == CVIP_S_INTF_X86_SID)
		arm_smmu_enable_fault_irq();
}
EXPORT_SYMBOL_GPL(cvip_plt_biommu_sid_ready);

void cvip_notify_resume(void)
{
	atomic_set(&cplt_dev->s2_idle, 0);
	if (s2idle_sysfs_dirent != NULL)
		sysfs_notify_dirent(s2idle_sysfs_dirent);

	atomic_notifier_call_chain(&resume_notifier_list, 0, NULL);
}

void cvip_notify_s2idle(void)
{
	atomic_set(&cplt_dev->s2_idle, 1);
	if (s2idle_sysfs_dirent != NULL)
		sysfs_notify_dirent(s2idle_sysfs_dirent);

	atomic_notifier_call_chain(&s2_idle_notifier_list, 0, NULL);
}

int get_nova_status(void)
{
	int status = 0;

	status = atomic_read(&cplt_dev->nova_online);

	return status;
}
EXPORT_SYMBOL_GPL(get_nova_status);

int get_xport_status(uint32_t type)
{
	int status = 0;

	switch (type) {
	case MLNET_HEAP:
		status = atomic_read(&cplt_dev->xport_mlnet_ready);
		dev_info(cplt_dev->dev[CVIP_PLT],
			"cvip mlnet xport ready %d\n", status);
		break;
	case X86_SHARED_HEAP:
		status = atomic_read(&cplt_dev->xport_shared_ready);
		dev_info(cplt_dev->dev[CVIP_PLT],
			"cvip shared xport ready %d\n", status);
		break;
	default:
		dev_err(cplt_dev->dev[CVIP_PLT], "unknown heap type\n");
		status = -1;
		break;
	}

	return status;
}
EXPORT_SYMBOL_GPL(get_xport_status);

void set_nova_shared_base(phys_addr_t addr)
{
	mutex_lock(&cvip_lock);
	shared_addr = addr;
	mutex_unlock(&cvip_lock);
}

void set_mlnet_base(phys_addr_t addr)
{
	mutex_lock(&cvip_lock);
	mlnet_addr = addr;
	mutex_unlock(&cvip_lock);
}

void set_dump_buf_base(phys_addr_t addr)
{
	mutex_lock(&cvip_lock);
	dump_addr = addr;
	mutex_unlock(&cvip_lock);
}

int get_cvip_pm_state(void)
{
	return pm_state;
}

int cvip_pm_wait_for_completion(void)
{
	/* NOTE: This timeout needs to be greater than:
	 *       TIMER_TO_WAIT_FOR_ACKS_SLEEP_RESUME_SECONDS in the file CvipPmState.cpp (5s)
	 *       This timeout needs to be less than:
	 *       The timeout in the kernel resume function in cvip.c in the x86 kernel (6s)
	 */
	if (wait_for_completion_timeout(&power_completion, msecs_to_jiffies(5500)) == 0)
		return -ETIMEDOUT;

	return 0;
}

void cvip_pm_reinit_completion(void)
{
	reinit_completion(&power_completion);
}

#ifdef CONFIG_CRASH_DUMP
static int install_dump_kernel(struct device *dev, phys_addr_t paddr)
{
	char *argv[4];
	static const char * const envp[] = {"HOME=/",
						"TERM=linux",
						"PATH=/sbin:/bin:/usr/sbin:/usr/bin",
						 NULL};
	char cmd_string[128] = {0};
	int ret = 0;

	snprintf(cmd_string, 128, "kdump-prepare.sh start %pa\n", &paddr);
	pr_info("cmd_string = %s\n", cmd_string);

	/* call kdump-prepare.sh to install dump kernel */
	argv[0] = "/bin/bash";
	argv[1] = "-c";
	argv[2] = cmd_string;
	argv[3] = NULL;

	ret = call_usermodehelper(argv[0], argv, (char **)envp, UMH_WAIT_PROC);
	if (ret < 0) {
		pr_err("%s: call_usermodehelper failed %d\n", __func__, ret);
		return ret;
	}
	dev_err(dev, "dump kernel installed");

	return 0;
}

static ssize_t install_show(struct device *dev, struct device_attribute *attr,
				char *buf)
{
	return sprintf(buf, "%u\n", dump_kernel);
}

static ssize_t install_store(struct device *dev, struct device_attribute *attr,
				 const char *buf, size_t count)
{
	int ret;

	if (!dump_addr) {
		dev_err(dev, "dump heap address not set\n");
		return count;
	}

	if (!dump_kernel) {
		ret = install_dump_kernel(dev, dump_addr);
		if (ret < 0)
			dev_err(dev, "error installing dump kernel\n");
	}

	return count;
}
#endif

static ssize_t s2_idle_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int status = 0;

	status = atomic_read(&cplt_dev->s2_idle);

	return sprintf(buf, "%d\n", status);
}

static ssize_t s2_idle_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	/* set s2 idle */
	atomic_set(&cplt_dev->s2_idle, 1);
	atomic_notifier_call_chain(&s2_idle_notifier_list, 0, NULL);

	return count;
}

static ssize_t resume_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int status = 0;

	return sprintf(buf, "%d\n", status);
}

static ssize_t resume_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	/* set resume */
	atomic_set(&cplt_dev->s2_idle, 0);
	atomic_notifier_call_chain(&resume_notifier_list, 0, NULL);
	return count;
}

static ssize_t nova_alive_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int status = 0;

	status = atomic_read(&cplt_dev->nova_online);

	return sprintf(buf, "%d\n", status);
}

static ssize_t nova_alive_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	return count;
}

static ssize_t enable_usb_gagdet_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	enable_usb_gadget(&bc, 1);
	return count;
}

static ssize_t rsmu_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	void *rsmu_base;

	/*
	 * This maps more than we need to, but keeps us page aligned.
	 * We only expose the data from two uint32_t's, so the extra
	 * mapping doesn't create a security issue.
	 */

	rsmu_base = memremap(
		RSMU_BASE, RSMU_BASE_SIZE, MEMREMAP_WB);
	if (rsmu_base == NULL) {
		pr_err("%s: memremap failed (%x, %u)\n",
			__func__, RSMU_BASE, RSMU_BASE_SIZE);
		return 0;
	}

	memcpy(buf, rsmu_base + RSMU_REG_MSB_OFFSET, 2 * RSMU_REG_WIDTH);

	memunmap(rsmu_base);

	return (2 * RSMU_REG_WIDTH);
}

static ssize_t rsmu_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	return count;
}

static int get_boot_config(union boot_config *bc)
{
	void *bc_base;

	bc_base = memremap(BOOT_CONFIG_ADDR, BOOT_CONFIG_SIZE, MEMREMAP_WB);
	if (bc_base == NULL) {
		dev_err(cplt_dev->dev[CVIP_PLT], "%s: memremap failed (%x, %lu)\n",
					__func__, BOOT_CONFIG_ADDR, BOOT_CONFIG_SIZE);
		return 0;
	}
	memcpy(&bc->raw, bc_base, BOOT_CONFIG_SIZE);

	memunmap(bc_base);

	return BOOT_CONFIG_SIZE;
}

static ssize_t boot_config_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int ret = BOOT_CONFIG_SIZE;

	ret = get_boot_config(&bc);

	if (ret)
		memcpy(buf, (void *)&bc.raw, ret);

	return ret;
}

static ssize_t boot_config_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	return count;
}

static ssize_t board_name_show(struct device *dev,
	    struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", cplt_dev->name);
}

static ssize_t device_locked_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", cplt_dev->device_locked);
}


static ssize_t board_id_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", bc.bits.board_id);
}

static ssize_t board_type_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", bc.bits.board_type);
}

static ssize_t cold_boot_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", bc.bits.cold);
}

static ssize_t property_size_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%lu\n", cvip_plt_property_get_size());
}

static ssize_t property_size_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	return count;
}

static ssize_t crumb_size_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%lu\n", cvip_plt_crumb_get_size());
}

static ssize_t crumb_size_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	return count;
}

static ssize_t pm_state_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d", pm_state);
}

static ssize_t pm_state_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	int ret;

	ret = kstrtoint(buf, 10, &pm_state);
	if (ret < 0)
		return ret;

	++pm_notif_cnt;
	dev_info(cplt_dev->dev[CVIP_PLT], "CVIP PM state = %d, last state = %d, notif cnt = %u\n",
		pm_state, last_pm_state, pm_notif_cnt);

	/* Complete when 1) going into S2Idle 2) returning to Normal from S2Idle
	 * 3) returning back to Doze from S2Idle
	 */
	if ((pm_state == PM_STATE_NORMAL_OPERATION && last_pm_state == PM_STATE_S2IDLE_SLEEP) ||
		(pm_state == PM_STATE_STANDBY_DOZE && last_pm_state == PM_STATE_S2IDLE_SLEEP) ||
		pm_state == PM_STATE_S2IDLE_SLEEP)
		complete(&power_completion);

	if (pm_state == PM_STATE_S2IDLE_SLEEP)
		atomic_notifier_call_chain(&s2_idle_notifier_list, 0, NULL);

	/* Store last state after our last_pm_state check above */
	last_pm_state = pm_state;

	return count;
}

static inline unsigned long size_inside_page(unsigned long start,
						 unsigned long size)
{
	unsigned long sz;

	sz = PAGE_SIZE - (start & (PAGE_SIZE - 1));

	return min(sz, size);
}

static inline bool should_stop_iteration(void)
{
	if (need_resched())
		cond_resched();
	return fatal_signal_pending(current);
}

static const struct vm_operations_struct mmap_mem_ops = {
#ifdef CONFIG_HAVE_IOREMAP_PROT
	.access = generic_access_phys
#endif
};

static int cvip_plt_do_mmap(struct file *file, struct vm_area_struct *vma, uint8_t wc)
{
	size_t size = vma->vm_end - vma->vm_start;
	phys_addr_t offset = (phys_addr_t)vma->vm_pgoff << PAGE_SHIFT;

	/* Does it even fit in phys_addr_t? */
	if (offset >> PAGE_SHIFT != vma->vm_pgoff)
		return -EINVAL;

	/* It's illegal to wrap around the end of the physical address space. */
	if (offset + (phys_addr_t)size - 1 < offset)
		return -EINVAL;

	if (!valid_mmap_phys_addr_range(vma->vm_pgoff, size))
		return -EINVAL;

	if (!phys_mem_access_prot_allowed(file, vma->vm_pgoff, size,
						&vma->vm_page_prot))
		return -EINVAL;

	if (wc)
		vma->vm_page_prot = pgprot_writecombine(vma->vm_page_prot);
	else
		vma->vm_page_prot = phys_mem_access_prot(file, vma->vm_pgoff,
							 size,
							 vma->vm_page_prot);

	vma->vm_ops = &mmap_mem_ops;

	/* Remap-pfn-range will mark the range VM_IO */
	if (remap_pfn_range(vma,
				vma->vm_start,
				vma->vm_pgoff,
				size,
				vma->vm_page_prot)) {
		return -EAGAIN;
	}
	return 0;
}


static int cvip_plt_mmap_mem(struct file *file, struct vm_area_struct *vma)
{

	return cvip_plt_do_mmap(file, vma, 0);
}

static ssize_t cvip_plt_write_mem(struct file *file, const char __user *buf,
					 size_t count, loff_t *ppos)
{
		phys_addr_t p = *ppos;
		ssize_t written, sz;
		unsigned long copied;
		void *ptr;

		if (p != *ppos)
			return -EFBIG;

		if (!valid_phys_addr_range(p, count))
			return -EFAULT;

		written = 0;
		while (count > 0) {

			sz = size_inside_page(p, count);
			ptr = xlate_dev_mem_ptr(p);
			if (!ptr) {
				if (written)
					break;
				return -EFAULT;
			}
			copied = copy_from_user(ptr, buf, sz);
			unxlate_dev_mem_ptr(p, ptr);
			if (copied) {
				written += sz - copied;
				if (written)
					break;
				return -EFAULT;
			}
			buf += sz;
			p += sz;
			count -= sz;
			written += sz;
			if (should_stop_iteration())
				break;
		}
		*ppos += written;
		return written;
}


/*
 *  This function reads the *physical* memory.
 *  The f_pos points directly to the memory location.
 */
static ssize_t cvip_plt_read_mem(struct file *file, char __user *buf,
					size_t count, loff_t *ppos)
{
	phys_addr_t p = *ppos;
	ssize_t read, sz;
	void *ptr;
	char *bounce;
	int err;

	if (p != *ppos)
		return 0;

	if (!valid_phys_addr_range(p, count))
		return -EFAULT;
	read = 0;
	bounce = kmalloc(PAGE_SIZE, GFP_KERNEL);
	if (!bounce)
		return -ENOMEM;

	while (count > 0) {
		unsigned long remaining;
		int probe;

		sz = size_inside_page(p, count);
		err = -EPERM;

		ptr = xlate_dev_mem_ptr(p);
		if (!ptr)
			goto failed;
		probe = probe_kernel_read(bounce, ptr, sz);
		unxlate_dev_mem_ptr(p, ptr);
		if (probe)
			goto failed;
		remaining = copy_to_user(buf, bounce, sz);

		if (remaining)
			goto failed;
		buf += sz;
		p += sz;
		count -= sz;
		read += sz;

		if (should_stop_iteration())
			break;
	}
	kfree(bounce);

	*ppos += read;
	return read;

failed:
	kfree(bounce);
	return err;
}


/* TODO(ggallagher): we may not support this */
/*
 * The memory devices use the full 32/64 bits of the offset, and so we cannot
 * check against negative addresses: they are ok. The return value is weird,
 * though, in that case (0).
 *
 * also note that seeking relative to the "end of file" isn't supported:
 * it has no meaning, so it returns -EINVAL.
 **/
static loff_t cvip_plt_mem_lseek(struct file *file, loff_t offset, int orig)
{
	loff_t ret;

	inode_lock(file_inode(file));
	switch (orig) {
	case SEEK_CUR:
		offset += file->f_pos;
		fallthrough;
	case SEEK_SET:
		/* to avoid userland mistaking f_pos=-9 as -EBADF=-9 */
		if ((unsigned long long)offset >= -MAX_ERRNO) {
			ret = -EOVERFLOW;
		break;

		}
		file->f_pos = offset;
		ret = file->f_pos;
		force_successful_syscall_return();
		break;
	default:
		ret = -EINVAL;
	}
	inode_unlock(file_inode(file));

	return ret;
}

static int pci_ready_notify(struct notifier_block *self,
				unsigned long v, void *p)
{

#ifdef CONFIG_CVIP_BUILD_TYPE_FACTORY
	dev_info(cplt_dev->dev[CVIP_PLT], "factory build detected - skipping pin toggle\n");
#else
	int ret;

	// the PSP set the pin high or we had an error case and
	// cvip's bootloader sets it high either way, we only need
	// to set it low here

	dev_info(cplt_dev->dev[CVIP_PLT], "capturing pin for wearable power toggle\n");

	ret = gpio_request_one(GPIO_WEARABLE_POWER_CNTRL,
						GPIOF_OUT_INIT_LOW, "wm-power");
	if (ret < 0) {
		dev_err(cplt_dev->dev[CVIP_PLT], "can't get WM-POWER gpio [%d], ret=[%d]\n",
				GPIO_WEARABLE_POWER_CNTRL, ret);
		return 1;
	}

	gpio_set_value(GPIO_WEARABLE_POWER_CNTRL, 0);
	gpio_free(GPIO_WEARABLE_POWER_CNTRL);

	dev_info(cplt_dev->dev[CVIP_PLT], "ec-control pin set low\n");
#endif

	return 0;
}

static struct notifier_block pci_ready_notifier = {
	.notifier_call = pci_ready_notify,
};

static int cvip_plt_open_mem(struct inode *inode, struct file *filp)
{

	if (!capable(CAP_SYS_RAWIO))
		return -EPERM;

	return 0;
}

/* CVIP_PLT needs to depend on CONFIG_MMU */
static const struct file_operations cvip_mem_fops = {
	.llseek		= cvip_plt_mem_lseek,
	.read		= cvip_plt_read_mem,
	.write		= cvip_plt_write_mem,
	.mmap		= cvip_plt_mmap_mem,
	.open		= cvip_plt_open_mem,
};


static int cvip_xport_mmap(struct file *file, struct vm_area_struct *vma)
{
	return cvip_plt_do_mmap(file, vma, 1);
}

/* CVIP_PLT needs to depend on CONFIG_MMU */
static const struct file_operations cvip_xport_fops = {
	.mmap		= cvip_xport_mmap,
};

static int cvip_properties_mmap(struct file *file, struct vm_area_struct *vma)
{
	size_t size;

	size = vma->vm_end - vma->vm_start;

	/* Force the address to be our properites location */
	vma->vm_pgoff = cvip_plt_property_get_addr() >> PAGE_SHIFT;

	/* Is the size exactly right? */
	if (size != cvip_plt_property_get_size())
		return -EINVAL;

	return cvip_plt_do_mmap(file, vma, 1);
}

static const struct file_operations cvip_properties_fops = {
	.mmap		= cvip_properties_mmap,
};

static int cvip_crumb_mmap(struct file *file, struct vm_area_struct *vma)
{
	size_t size;

	size = vma->vm_end - vma->vm_start;

	/* Force the address to be our crumb location */
	vma->vm_pgoff = cvip_plt_crumb_get_addr() >> PAGE_SHIFT;

	/* Is the size exactly right? */
	if (size != cvip_plt_crumb_get_size())
		return -EINVAL;

	return cvip_plt_do_mmap(file, vma, 1);
}

static const struct file_operations cvip_crumb_fops = {
	.mmap		= cvip_crumb_mmap,
};

static long cvip_pci_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct cvip_plat_ioctl ioctl_arg;
	int result = 0;

	result = access_ok((void *)arg, sizeof(ioctl_arg));

	if (!result) {
		pr_err("ioctl: access_ok failed [%d]\n", result);
		return -EFAULT;
	}

	if (copy_from_user(&ioctl_arg, (void __user *)arg, sizeof(ioctl_arg))) {
		pr_err("ioctl: copy_from_user failed\n");
		return -EFAULT;
	}

	switch (cmd) {
	case CVIP_PLAT_PCI_SCAN_REQUEST:
		if (cvip_plt_pci_scan(&(ioctl_arg.pci)))
			result = -EIO;
		break;
	case CVIP_PLAT_PCI_CAP_GET:
		if (cvip_plt_pci_cap_get(&(ioctl_arg.pci)))
			result = -EIO;
		break;
	case CVIP_PLAT_PCI_CAP_SET:
		if (cvip_plt_pci_cap_set(&(ioctl_arg.pci)))
			result = -EIO;
		break;
	default:
		pr_err("ioctl: illegal command: 0x%x\n", cmd);
		result = -EIO;
		break;
	}

	// success or failure, copy ioctl info back.
	if (copy_to_user((void __user *)arg, &ioctl_arg, sizeof(ioctl_arg))) {
		pr_err("ioctl: failed to copy back to user\n");
		return -EFAULT;
	}

	return result;
}


static const struct file_operations cvip_pci_fops = {
	.unlocked_ioctl		= cvip_pci_ioctl,
};

static int cvip_plt_open(struct inode *inode, struct file *filp);

static int cvip_plt_release(struct inode *inode, struct file *file)
{
	return 0;
}

static ssize_t cvip_plt_read(struct file *file, char __user *buf,
				 size_t count, loff_t *ppos)
{
	struct cvip_plt_device *cpdev;

	cpdev = (struct cvip_plt_device *)file->private_data;
	if (!cpdev)
		return -ENODEV;

	return 0;
}

static ssize_t cvip_plt_write(struct file *file, const char __user *buf,
				size_t count, loff_t *ppos)
{
	struct cvip_plt_device *cpdev;

	cpdev = (struct cvip_plt_device *)file->private_data;
	if (!cpdev)
		return -ENODEV;

	return 0;
}

static long cvip_plt_ioctl(struct file *file, unsigned int cmd,
			   unsigned long arg)
{
	return 0;
}

static const struct file_operations cvip_plt_fops = {
	.owner = THIS_MODULE,
	.open = cvip_plt_open,
	.read = cvip_plt_read,
	.write = cvip_plt_write,
	.unlocked_ioctl = cvip_plt_ioctl,
	.release = cvip_plt_release,
	.llseek = noop_llseek,
};

static ssize_t cvip_read(struct file *file, char __user *buf,
				 size_t count, loff_t *ppos)
{
	struct cvip_plt_device *cpdev;
	int ret = 0;

	cpdev = (struct cvip_plt_device *)file->private_data;
	if (!cpdev)
		return -ENODEV;

	return ret;
}

static ssize_t cvip_write(struct file *file, const char __user *buf,
				size_t count, loff_t *ppos)
{
	struct cvip_plt_device *cpdev;
	int ret = 0;

	cpdev = (struct cvip_plt_device *)file->private_data;
	if (!cpdev)
		return -ENODEV;

	return ret;
}

static long cvip_ioctl(struct file *file, unsigned int cmd,
			   unsigned long arg)
{
	return -ENOIOCTLCMD;
}

static const struct file_operations cvip_fops = {
	.owner = THIS_MODULE,
	.read = cvip_read,
	.write = cvip_write,
	.unlocked_ioctl = cvip_ioctl,
};


static const struct cvip_plt_devs {
	const char *name;
	umode_t mode;
	const struct file_operations *fops;
	fmode_t fmode;
	int minor;
} devlist[] = {
	/* no 0 index device here */
	[CVIP] = { "cvip", 0, &cvip_fops, FMODE_UNSIGNED_OFFSET},
	[CVIP_MEM] = { "cvip_mem", 0, &cvip_mem_fops, FMODE_UNSIGNED_OFFSET},
	[CVIP_XPORT] = { "cvip_xport", 0, &cvip_xport_fops,
			 FMODE_UNSIGNED_OFFSET},
	[CVIP_PROPERTIES] = { "cvip_properties", 0,
			 &cvip_properties_fops, FMODE_UNSIGNED_OFFSET},
	[CVIP_CRUMB] = { "cvip_crumb", 0, &cvip_crumb_fops,
			 FMODE_UNSIGNED_OFFSET},
	[CVIP_PCI] = { "cvip_pci", 0, &cvip_pci_fops,
			 FMODE_UNSIGNED_OFFSET},
};

static int cvip_plt_open(struct inode *inode, struct file *filp)
{
	int minor;
	const struct cvip_plt_devs *dev;

	minor = iminor(inode);

	if (minor >= ARRAY_SIZE(devlist))
		return -ENXIO;

	dev = &devlist[minor];
	if (!dev->fops)
		return -ENXIO;

	filp->f_op = dev->fops;
	filp->f_mode |= dev->fmode;

	if (dev->fops->open)
		return dev->fops->open(inode, filp);


	return 0;
}

static DEVICE_ATTR_RW(resume);
static DEVICE_ATTR_RW(s2_idle);
static DEVICE_ATTR_RW(nova_alive);
static DEVICE_ATTR_WO(enable_usb_gagdet);
static DEVICE_ATTR_RW(rsmu);
static DEVICE_ATTR_RW(boot_config);
static DEVICE_ATTR_RW(property_size);
static DEVICE_ATTR_RW(crumb_size);
static DEVICE_ATTR_RW(pm_state);
static DEVICE_ATTR_RO(board_id);
static DEVICE_ATTR_RO(device_locked);
static DEVICE_ATTR_RO(board_name);
static DEVICE_ATTR_RO(board_type);
static DEVICE_ATTR_RO(cold_boot);

#ifdef CONFIG_CRASH_DUMP
static DEVICE_ATTR_RW(install);

static const struct attribute *const dk_attrs[] = {
	&dev_attr_install.attr,
	NULL,
};

static const struct attribute_group dk_group = {
	.name = "dump_kernel",
	.attrs = (struct attribute **)dk_attrs,
};
#endif

static const struct attribute *const cvip_attrs[] = {
	&dev_attr_enable_usb_gagdet.attr,
	&dev_attr_nova_alive.attr,
	&dev_attr_s2_idle.attr,
	&dev_attr_resume.attr,
	NULL,
};

static const struct attribute_group cvip_group = {
	.name = "cvip",
	.attrs = (struct attribute **)cvip_attrs,
};


static const struct attribute *const rsmu_attrs[] = {
	&dev_attr_rsmu.attr,
	NULL,
};

static const struct attribute_group rsmu_group = {
	.name = "rsmu",
	.attrs = (struct attribute **)rsmu_attrs,
};


static const struct attribute *const boot_config_attrs[] = {
	&dev_attr_boot_config.attr,
	&dev_attr_board_id.attr,
	&dev_attr_board_name.attr,
	&dev_attr_board_type.attr,
	&dev_attr_cold_boot.attr,
	&dev_attr_device_locked.attr,
	NULL,
};

static const struct attribute_group boot_config_group = {
	.name = "boot_config",
	.attrs = (struct attribute **)boot_config_attrs,
};

static const struct attribute *const prop_attrs[] = {
	&dev_attr_property_size.attr,
	NULL,
};

static const struct attribute_group prop_group = {
	.name = "property",
	.attrs = (struct attribute **)prop_attrs,
};

static const struct attribute *const crumb_attrs[] = {
	&dev_attr_crumb_size.attr,
	NULL,
};

static const struct attribute_group crumb_group = {
	.name = "crumb",
	.attrs = (struct attribute **)crumb_attrs,
};

static const struct attribute_group scpi_group = {
	.name = "scpi",
	.attrs = (struct attribute **)scpi_attrs,
};

static const struct attribute *const power_mgr_attrs[] = {
	&dev_attr_pm_state.attr,
	NULL,
};

static const struct attribute_group power_mgr_group = {
	.name = "power_mgr",
	.attrs = (struct attribute **)power_mgr_attrs,
};

static char *cvip_plt_devnode(struct device *dev, umode_t *mode)
{
	if (mode && devlist[MINOR(dev->devt)].mode)
		*mode = devlist[MINOR(dev->devt)].mode;

	return NULL;
}


static int cvip_plt_reg_dev(struct cvip_plt_device *cpdev)
{
	int major, i;

	major = register_chrdev(0, CDEV_NAME, &cvip_plt_fops);
	if (!major) {
		dev_err(cpdev->dev[CVIP_PLT], "failed to register cvip driver\n");
		return -ENODEV;
	}
	cpdev->major = major;
	cpdev->class = class_create(THIS_MODULE, CDEV_NAME);
	if (IS_ERR(cpdev->class))
		return PTR_ERR(cpdev->class);

	cpdev->class->devnode = cvip_plt_devnode;

	for (i = 1; i < MAX_CVIP_DEVS; i++) {
		cpdev->dev[i] = device_create(cpdev->class, NULL,
			MKDEV(major, i),
			NULL, devlist[i].name);
		dev_info(cpdev->dev[CVIP_PLT],
			 "Device node %s initialized successfully.\n",
			 devlist[i].name);
	}

	return 0;
}

static void cvip_plt_remove_node(void)
{
	/* destroy devices too */
	mutex_destroy(&cplt_dev->lock);
}

static const int decompress_clk =  600000000;
static const int a55_dsu_clk = 1133000000;

static int cvip_plt_probe(struct platform_device *pdev)
{
	struct cvip_plt_device *cpdev;
	int ret;

	ATOMIC_INIT_NOTIFIER_HEAD(&nova_event_notifier_list);
	ATOMIC_INIT_NOTIFIER_HEAD(&xport_ready_notifier_list);
	ATOMIC_INIT_NOTIFIER_HEAD(&s2_idle_notifier_list);
	ATOMIC_INIT_NOTIFIER_HEAD(&resume_notifier_list);

	mutex_init(&cvip_lock);
	cpdev = devm_kzalloc(&pdev->dev, sizeof(*cpdev), GFP_KERNEL);
	if (!cpdev)
		return -ENOMEM;
	platform_set_drvdata(pdev, cpdev);

	cpdev->dev[CVIP_PLT] = &pdev->dev;
	cpdev->s2_idle = (atomic_t)ATOMIC_INIT(0);
	cpdev->nova_online = (atomic_t)ATOMIC_INIT(0);
	cpdev->xport_shared_ready = (atomic_t)ATOMIC_INIT(0);
	cpdev->xport_mlnet_ready = (atomic_t)ATOMIC_INIT(0);
	cpdev->device_locked = device_locked;

	ret = cvip_plt_reg_dev(cpdev);
	if (ret) {
		dev_err(cpdev->dev[CVIP_PLT], "Fail to add cvip platform device node\n");
		return ret;
	}

	cplt_dev = cpdev;
	mutex_init(&cplt_dev->lock);

	ret = cvip_plt_property_get_from_fmr();
	if (ret) {
		dev_err(cpdev->dev[CVIP_PLT], "Failed to read persistent property metadata\n");
		return ret;
	}

	ret = cvip_plt_crumb_get_from_fmr();
	if (ret) {
		dev_err(cpdev->dev[CVIP_PLT], "Failed to read crumb metadata\n");
		return ret;
	}

#ifdef CONFIG_CRASH_DUMP
	ret = sysfs_create_group(&cpdev->dev[CVIP]->kobj, &dk_group);
	if (ret < 0)
		dev_err(cplt_dev->dev[CVIP_PLT], "dump kernel sysfs group failed\n");
#endif
	ret = sysfs_create_group(&cpdev->dev[CVIP]->kobj, &cvip_group);
	if (ret < 0)
		dev_err(cplt_dev->dev[CVIP_PLT], "cvip sysfs group failed\n");

	ret = sysfs_create_group(&cpdev->dev[CVIP]->kobj, &rsmu_group);
	if (ret < 0)
		dev_err(cplt_dev->dev[CVIP_PLT], "rsmu sysfs group failed\n");

	ret = sysfs_create_group(&cpdev->dev[CVIP]->kobj, &boot_config_group);
	if (ret < 0)
		dev_err(cplt_dev->dev[CVIP_PLT], "boot_config sysfs group failed\n");

	ret = sysfs_create_group(&cpdev->dev[CVIP]->kobj, &prop_group);
	if (ret < 0)
		dev_err(cplt_dev->dev[CVIP_PLT], "property sysfs group failed\n");

	ret = sysfs_create_group(&cpdev->dev[CVIP]->kobj, &crumb_group);
	if (ret < 0)
		dev_err(cplt_dev->dev[CVIP_PLT], "crumb sysfs group failed\n");

	ret = sysfs_create_group(&cpdev->dev[CVIP]->kobj, &scpi_group);
	if (ret < 0)
		dev_err(cplt_dev->dev[CVIP_PLT], "scpi sysfs group failed\n");

	ret = sysfs_create_group(&cpdev->dev[CVIP]->kobj, &power_mgr_group);
	if (ret < 0)
		dev_err(cplt_dev->dev[CVIP_PLT], "power_mgr sysfs group failed\n");

	/* get the cvip directory first then the sub directory */
	cvip_sysfs_dirent = sysfs_get_dirent(cplt_dev->dev[CVIP]->kobj.sd, "cvip");
	if (cvip_sysfs_dirent == NULL) {
		dev_err(cplt_dev->dev[CVIP_PLT], "failed to get cvip sysfs dirent\n");
	} else {
		s2idle_sysfs_dirent = sysfs_get_dirent(cvip_sysfs_dirent, "s2_idle");
		if (s2idle_sysfs_dirent == NULL)
			dev_err(cplt_dev->dev[CVIP_PLT], "failed to get s2idle sysfs dirent\n");
	}

	atomic_notifier_chain_register(&cvip_pci_ready_notifier_list,
		&pci_ready_notifier);

	ret = read_reboot_reason(cplt_dev);
	if (ret < 0)
		dev_err(cplt_dev->dev[CVIP_PLT], "error reading reboot reason\n");
#ifdef CONFIG_CVIP_IPC
	dev_info(cpdev->dev[CVIP_PLT], "Sending alive message to x86\n");
	send_cvip_alive();
	dev_info(cpdev->dev[CVIP_PLT], "Sent alive message to x86\n");
#endif

#ifdef CONFIG_COMMON_CLK_SCPI
	ret = cvip_fclk_init(&cpdev->dev[CVIP]->kobj);
	if (ret)
		dev_err(cplt_dev->dev[CVIP_PLT], "error initialising dsp fclk switching\n");
#endif

	init_completion(&power_completion);

	cvip_plt_scpi_set_a55dsu(a55_dsu_clk);
	cvip_plt_scpi_set_decompress(decompress_clk);

	ret = get_boot_config(&bc);
	if (!ret)
		dev_err(cplt_dev->dev[CVIP_PLT], "error mapping boot config\n");

	ret = enable_usb_gadget(&bc, 0);
	if (ret < 0)
		dev_err(cplt_dev->dev[CVIP_PLT], "error enabling usb gadget\n");

	ret = enable_tbu_tcu_scr_obsrv();
	if (ret < 0)
		dev_err(cplt_dev->dev[CVIP_PLT], "error enable smmu tbu tcu secure observation\n");


	dev_info(cpdev->dev[CVIP_PLT], "Successfully probed cvip platform device\n");
	return 0;
}

static inline uint64_t ccnt_read(void)
{
	uint64_t cc = 0;

	asm volatile("mrs %0, cntvct_el0" : "=r"(cc));
	return cc;
}

static const uint64_t big_advance = 8;
static const int big_limit = 7;
static int a55speed_test(void)
{
	uint64_t samples[10];
	uint64_t delta = 0;
	int big_count = 0;
	int i;

	samples[0] = ccnt_read();
	samples[1] = ccnt_read();
	samples[2] = ccnt_read();
	samples[3] = ccnt_read();
	samples[4] = ccnt_read();
	samples[5] = ccnt_read();
	samples[6] = ccnt_read();
	samples[7] = ccnt_read();
	samples[8] = ccnt_read();
	samples[9] = ccnt_read();

	for (i = 0; i < 10; i++) {
		if (i > 0) {
			delta = samples[i] - samples[i - 1];
			if (delta > big_advance)
				big_count++;
		}
	}

	if (big_count > big_limit)
		return 1;
	return 0;
}


extern int cvip_plt_scpi_set_a55two(unsigned long freq);
static const int a55_clock_fast = 1700000000;
static const int a55_clock_enough = 1000000000;
static const int a55_clock_medium = 600000000;
static const int period_1_ms = 50;             // 20 Hz
static int watch_cpu_freq(void *data)
{
	int rc;
	int last_rc;
	int period_ms;
	int cycles = 0;
	unsigned long cpu0clock;
	struct scpi_drvinfo *scpi_info;

	period_ms = period_1_ms;
	rc = a55speed_test();
	if (rc)
		pr_info("A55 clock starting out SLOW");
	else
		pr_info("A55 clock starting out good");
	last_rc = rc;
	for (;;) {
		rc = a55speed_test();
		if (rc != last_rc) {
			if (rc) {
				scpi_info = get_scpi_drvinfo();
				if (scpi_info) {
					cpu0clock =
						scpi_info->scpi_ops->clk_get_val(
							A55TWOCLK) *
						SCPI_1MHZ;
					pr_info("CVCORE-2532: A55 changed from good to SLOW at cycle %d (%lu)",
						cycles, cpu0clock);
					if (cpu0clock > a55_clock_enough) {
						// We should be at a fast clock speed, but aren't.
						cvip_plt_scpi_set_a55two(
							a55_clock_medium);
						msleep_interruptible(1);
						cvip_plt_scpi_set_a55two(
							cpu0clock);
						pr_info("CVCORE-2532: Applied workaround");
					}
				}
			} else {
				pr_info("A55 clock changed to good at cycle %d",
					cycles);
			}
		}
		last_rc = rc;
		msleep_interruptible(period_ms);
		cycles++;
	}
	return 0;
}

/*
 * TODO(ggallagher): This reaches into mero specific kernel
 * driver. Can we reference gsm_jrtc.h so we can remove
 * this extern?
 */
extern bool gsm_jrtc_check_is_valid(void);

/*
 * TODO(ggallagher): Should we create a cvip_plat_scpi.h so
 * we can remove this extern?
 */
extern int cvip_plt_scpi_set_nic(unsigned long freq);

static const int nic_clock_slow = 400000000;
static const int nic_clock_medi = 600000000;
static const int nic_clock_fast = 850000000;

static int watch_nic_freq(void *data)
{
	bool nic_status;
	bool last_nic_status;
	int period_ms;
	int cycles = 0;
	int i = 0;
	__iomem void *nic_clk_smn_reg;
	__u32 nic_clk_reg_val;

	nic_clk_smn_reg =
		ioremap_nocache(NIC_CLK_SMN_REG_ADDR, NIC_CLK_SMN_REG_SIZE);

	if (nic_clk_smn_reg) {
		nic_clk_reg_val = readl_relaxed(nic_clk_smn_reg);
		pr_info("CVCORE-2955: Early nic_clk SMN reg = %d\n",
			nic_clk_reg_val);
	} else {
		pr_err("CVCORE-2955: Could not get va of nic_clk SMN reg\n");
	}

	/* Assume nic clock starts ok */
	last_nic_status = true;
	period_ms = period_1_ms;
	for (;;) {
		nic_status = gsm_jrtc_check_is_valid();
		if (nic_status != last_nic_status) {
			if (!nic_status) {
				pr_info("CVCORE-2955: NIC changed from good to SLOW at cycle %d",
					cycles);

				if (nic_clk_smn_reg) {
					for (i = 0; i < 8; i++) {
						nic_clk_reg_val = readl_relaxed(
							nic_clk_smn_reg);
						pr_info("CVCORE-2955: PoF nic_clk SMN reg = %d\n",
							nic_clk_reg_val);
						udelay(5);
					}
				}
				// We should be at a fast clock speed, but aren't.
				cvip_plt_scpi_set_nic(nic_clock_slow);
				cvip_plt_scpi_set_nic(nic_clock_medi);
				cvip_plt_scpi_set_nic(nic_clock_fast);
				pr_info("CVCORE-2955: NIC clock workaround applied");
			} else {
				pr_info("CVCORE-2955: NIC clock changed to good at cycle %d",
					cycles);
			}
		}
		last_nic_status = nic_status;
		msleep_interruptible(period_ms);
		cycles++;
	}
	return 0;
}

static int cvip_plt_remove(struct platform_device *pdev)
{
	sysfs_put(s2idle_sysfs_dirent);
	sysfs_put(cvip_sysfs_dirent);
#ifdef CONFIG_CRASH_DUMP
	sysfs_remove_group(&cplt_dev->dev[CVIP]->kobj, &dk_group);
#endif
	sysfs_remove_group(&cplt_dev->dev[CVIP]->kobj, &scpi_group);
#ifdef CONFIG_COMMON_CLKC_SCPI
	cvip_fclk_exit();
#endif
	cvip_plt_remove_node();
	return 0;
}

static const struct of_device_id cvip_of_match[] = {
	{ .compatible = "amd,cvip-plt", },
	{ },
};

static struct platform_driver cvip_driver = {
	.probe = cvip_plt_probe,
	.remove = cvip_plt_remove,
	.driver = {
		.name = "cvip_plt",
		.of_match_table = cvip_of_match,
	},
};

/* check if uart needs to be silent */
static int __init param_is_device_locked(char *buf)
{
	int err;

	err = kstrtouint(buf, 10, &device_locked);

	pr_err("device_locked %d\n", device_locked);
	return err;
}
early_param("device_locked", param_is_device_locked);

static struct task_struct *a55_hz_watcher_thread;
static struct task_struct *nic_hz_watcher_thread;
static int __init cvip_plt_init(void)
{
	int ret;

	a55_hz_watcher_thread = kthread_run(watch_cpu_freq,
		NULL, "diagnostic A55 speed");

	nic_hz_watcher_thread = kthread_run(watch_nic_freq,
		NULL, "diagnostic NIC speed");

	ret = platform_driver_register(&cvip_driver);
	if (ret)
		return ret;

	return 0;
}

static void __exit cvip_plt_exit(void)
{
	platform_driver_unregister(&cvip_driver);
}

module_init(cvip_plt_init);
module_exit(cvip_plt_exit);

MODULE_DESCRIPTION("cvip platform device");
