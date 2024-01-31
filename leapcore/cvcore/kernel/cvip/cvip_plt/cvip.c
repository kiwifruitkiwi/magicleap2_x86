/*
 * CVIP driver for AMD SoC CVIP Subsystem.
 *
 * Copyright (C) 2019-2021 Advanced Micro Devices, Inc. All rights reserved.
 */

#include <linux/types.h>
#include <linux/err.h>
#include <linux/delay.h>
#include <linux/export.h>
#include <linux/io.h>
#include <linux/interrupt.h>
#include <linux/mutex.h>
#include <linux/device.h>
#include <linux/pci.h>
#include <linux/module.h>
#include <linux/completion.h>
#include <linux/suspend.h>
#include <linux/syscalls.h>
#include <asm/page.h>
#include <linux/timer.h>
#include <linux/rtc.h>
#include <linux/pm_wakeup.h>
#include <linux/pm_wakeirq.h>
#include <linux/iommu.h>
#include <linux/iommu-helper.h>
#include <linux/iova.h>
#include <linux/amd-iommu.h>
#include <linux/miscdevice.h>
#include <trace/events/power.h>
#include <asm/fpu/api.h>

#include "mero_ipc_i.h"
#include "ipc.h"
#include "cvip.h"
#include "ion.h"
#include "mero_ion.h"
#include "cvip_s_intf.h"

#define PCI_DEVICE_ID_CVIP	0x149d
#define CVIP_SHARED_BUF_SIZE	(64 * 1024 * 1024)
#define COREDUMP_SIZE		(32 * 1024 * 1024)
#define CVIP_XPORT_BUFSIZE	(CVIP_SHARED_BUF_SIZE - COREDUMP_SIZE)
#define CVIP_COREDUMP		"/data/coredump"
#define CDEV_NAME		"cvip"
#define CDEV_XPORT_NAME		"cvipx"
#define CVIP_MOVNTDQA_SIZE	16
#define MAX_MOVNTDQA_OPS	4

#define TO_PFN(addr)		((addr) >> PAGE_SHIFT)

/*
 * mailbox link parameters: {mailbox_ID, src_ID, dest_ID, mode}
 * CVIP device is responsible for initializing mailbox registers.
 */
static u32 mb_param[4][4] = {
	{0, 24, 25, 0x30000},
	{2, 24, 25, 0x30000},
	{3, 26, 27, 0x50000},
	{1, 26, 27, 0x50000}
	/* to add more */
};

/**
 * struct cvip_buffer - ion buffer
 * @fd: buffer fd returned from ion_alloc();
 * @buffer: ion buffer pointer
 * @dma_buf: dma buffer pointer
 * @paddr: physical address
 * @buf_rgd: buffer registration state
 * @dma_addr: iommu mapped address
 * @npages: number of pages
 */
struct cvip_buffer {
	int fd;
	struct ion_buffer *buffer;
	struct dma_buf *dma_buf;
	phys_addr_t paddr;
	bool buf_rgd;
	dma_addr_t dma_addr;
	unsigned int npages;
};

enum cvip_buffer_type {
	shared,
	x86
};

/**
 * struct cvip_dev - per CVIP PCI device context
 * @pci_dev: PCI driver node
 * @ipc_dev: IPC mailbox device
 * @completion: used for sync between cvip and x86
 * @power_completion: used for sync power messaging between cvip and x86
 * @is_state_d3: used to check if CVIP entered D3
 * @domain: iommu domain
 * @iovad: reserved iova domain
 * @base: base address of S-interface MMIO BAR
 * @res_len: S-interface MMIO size
 */
struct cvip_dev {
	struct pci_dev *pci_dev;
	struct mero_ipc_dev ipc_dev;
	struct completion completion;
	struct completion power_completion;
	struct cvip_buffer shared_buf;
	struct cvip_buffer x86_buf;
	u32 is_state_d3;
	struct iommu_domain *domain;
	struct iova_domain *iovad;
	void __iomem *base;
	resource_size_t res_len;
};

static struct cvip_dev *cvip_p;

static char *cvip_event[] = {
	"ON_HEAD",
	"OFF_HEAD",
	"S2_IDLE",
	"CVIP_RESUME"
};

static struct ipc_context *ipc_cvip;
static bool cvip_reboot;

static int ipc_send_reboot_message(void)
{
	struct ipc_message pm_message;

	pm_message.dest = 1;
	pm_message.channel_idx = 3;
	pm_message.length = 0;
	pm_message.message_id = REBOOT_REQ;

	return ipc_sendmsg(ipc_cvip, &pm_message);
}

static int ipc_send_uevent(enum cvip_event_type event)
{
	char *envp[2];

	if (IS_ERR_OR_NULL(cvip_p))
		return -EINVAL;

	envp[0] = cvip_event[event];
	envp[1] = NULL;
	kobject_uevent_env(&cvip_p->ipc_dev.dev->kobj, KOBJ_CHANGE, envp);

	return 0;
}

static int cvip_panic_notify(struct notifier_block *nb,
			     unsigned long event, void *data)
{
	ipc_send_reboot_message();

	return NOTIFY_OK;
}

static struct notifier_block cvip_panic_nb = {
	.notifier_call = cvip_panic_notify,
};

int cvip_get_mbid(u32 idx)
{
	if (idx >= 0 && idx < sizeof(mb_param)/sizeof(mb_param[0]))
		return mb_param[idx][0];
	else
		return -EINVAL;
}
EXPORT_SYMBOL(cvip_get_mbid);

struct device *cvip_get_mb_device(void)
{
	if (IS_ERR_OR_NULL(cvip_p))
		return ERR_PTR(-EINVAL);
	else
		return cvip_p->ipc_dev.dev;
}
EXPORT_SYMBOL(cvip_get_mb_device);

int cvip_get_num_chans(void)
{
	if (IS_ERR_OR_NULL(cvip_p))
		return -EINVAL;
	else
		return cvip_p->ipc_dev.nr_mbox;
}
EXPORT_SYMBOL(cvip_get_num_chans);

static unsigned long cvip_alloc_iova(struct cvip_dev *cvip, unsigned int pages, u64 dma_mask)
{
	unsigned long pfn = 0;

	pages = __roundup_pow_of_two(pages);

	if (dma_mask > DMA_BIT_MASK(32))
		pfn = alloc_iova_fast(cvip->iovad, pages, TO_PFN(DMA_BIT_MASK(32)), false);
	else
		pfn = alloc_iova_fast(cvip->iovad, pages, TO_PFN(dma_mask), true);

	return (pfn << PAGE_SHIFT);
}

static void cvip_free_iova(struct cvip_dev *cvip, unsigned long address, unsigned int pages)
{
	pages = __roundup_pow_of_two(pages);

	free_iova_fast(cvip->iovad, TO_PFN(address), pages);
}

static dma_addr_t cvip_get_iova(struct cvip_dev *cvip, struct cvip_buffer *shared_buf,
				u32 bufsize)
{
	u64 dma_mask;
	unsigned int npages;
	dma_addr_t address;
	struct device *dev = &cvip->pci_dev->dev;

	dma_mask = *dev->dma_mask;
	npages = iommu_num_pages(shared_buf->paddr, bufsize, PAGE_SIZE);
	address = cvip_alloc_iova(cvip, npages, dma_mask);
	shared_buf->npages = npages;
	shared_buf->dma_addr = address;
	dev_dbg(dev, "allocate iova:0x%llx, bufsize:%llx, npages:%lld\n",
		address, bufsize, npages);

	return address;
}

static struct ion_buffer *cvip_alloc_buffer(struct cvip_dev *cvip,
				enum cvip_buffer_type type)
{
	int fd;
	size_t size;
	struct device *dev;
	void *vaddr;
	struct dma_buf *dma_buf = NULL;
	struct ion_buffer *buffer = NULL;

	if (IS_ERR_OR_NULL(cvip))
		return ERR_PTR(-EINVAL);

	if (type == shared)
		size = CVIP_SHARED_BUF_SIZE;
	else if (type == x86)
		size = CVIP_X86_BUF_SIZE;
	else
		return ERR_PTR(-EINVAL);

	dev = &cvip_p->pci_dev->dev;
	fd = ion_alloc(size, 1 << ION_HEAP_X86_CMA_ID, ION_FLAG_CACHED);
	if (fd < 0) {
		dev_err(dev, "Failed to allocate buffer\n");
		return ERR_PTR(-ENOMEM);
	}

	dma_buf = dma_buf_get(fd);
	if (IS_ERR(dma_buf)) {
		dev_err(dev, "Failed to get dma buffer\n");
		goto dma_err;
	}

	buffer = dma_buf->priv;
	vaddr = dma_buf_kmap(dma_buf, 0);
	if (IS_ERR_OR_NULL(vaddr)) {
		dev_err(dev, "Failed to map buffer\n");
		goto map_err;
	}

	buffer->vaddr = vaddr;

	if (type == shared) {
		cvip->shared_buf.fd = fd;
		cvip->shared_buf.dma_buf = dma_buf;
		cvip->shared_buf.buffer = buffer;
	} else {
		cvip->x86_buf.fd = fd;
		cvip->x86_buf.dma_buf = dma_buf;
		cvip->x86_buf.buffer = buffer;
	}

	return buffer;

map_err:
	dma_buf_put(dma_buf);
dma_err:
	ksys_close(fd);
	return ERR_PTR(-EINVAL);

}

static void cvip_release_buffer(struct cvip_dev *cvip, enum cvip_buffer_type type)
{
	int fd = 0;
	struct dma_buf *dma_buf = NULL;
	struct ion_buffer *buffer = NULL;

	if (IS_ERR_OR_NULL(cvip))
		return;

	if (type == shared) {
		dma_buf = cvip->shared_buf.dma_buf;
		buffer = cvip->shared_buf.buffer;
		fd = cvip->shared_buf.fd;
	} else if (type == x86) {
		dma_buf = cvip->x86_buf.dma_buf;
		buffer = cvip->x86_buf.buffer;
		fd = cvip->x86_buf.fd;
	} else
		return;

	if (IS_ERR_OR_NULL(dma_buf) || IS_ERR_OR_NULL(buffer) || fd < 0)
		return;

	dma_buf_kunmap(dma_buf, 0, buffer->vaddr);
	dma_buf_put(dma_buf);
	ion_buffer_destroy(buffer);
	ksys_close(fd);

	if (type == shared) {
		cvip->shared_buf.dma_buf = NULL;
		cvip->shared_buf.buffer = NULL;
		cvip->shared_buf.fd = -1;
	} else {
		cvip->x86_buf.dma_buf = NULL;
		cvip->x86_buf.buffer = NULL;
		cvip->x86_buf.fd = -1;
	}
}

static int cvip_reg_buf(void)
{
	struct ipc_message reg_message;
	u32 addr_hi, addr_low, buf_size;
	int ret;
	phys_addr_t paddr;
	dma_addr_t iommu_paddr = 0;
	struct ion_buffer *buffer = NULL;
	struct device *dev;

	if (IS_ERR_OR_NULL(ipc_cvip) || IS_ERR_OR_NULL(cvip_p))
		return -EINVAL;

	dev = &cvip_p->pci_dev->dev;
	/* The share buffer should be kept during CVIP reboot */
	if (!cvip_reboot) {
		buffer = cvip_alloc_buffer(cvip_p, shared);
		if (IS_ERR_OR_NULL(buffer))
			return -EINVAL;

		paddr = page_to_phys(sg_page(buffer->sg_table->sgl));
		cvip_p->shared_buf.paddr = paddr;
		cvip_p->shared_buf.dma_addr = paddr;

		if (cvip_p->domain) {
			iommu_paddr = cvip_get_iova(cvip_p, &cvip_p->shared_buf,
						    CVIP_SHARED_BUF_SIZE);
			if (!iommu_paddr) {
				dev_err(dev, "failed to allocate iova\n");
				ret = -ENOMEM;
				goto msg_err;
			}

			ret = iommu_map(cvip_p->domain, iommu_paddr, paddr,
					CVIP_SHARED_BUF_SIZE, IOMMU_READ | IOMMU_WRITE);
			if (ret) {
				dev_err(dev, "iommu mapping failed\n");
				cvip_p->shared_buf.dma_addr = 0;
				goto msg_err;
			}
			dev_dbg(dev, "iova=0x%llx, paddr=0x%llx\n", iommu_paddr, paddr);
		}
	}
	addr_hi = cvip_p->shared_buf.dma_addr >> 32;
	addr_low = cvip_p->shared_buf.dma_addr;
	buf_size = CVIP_SHARED_BUF_SIZE;
	/* async tx channel */
	reg_message.dest = 1;
	reg_message.channel_idx = 2;
	reg_message.length = 3;
	reg_message.message_id = REGISTER_BUFFER;
	reg_message.payload[0] = addr_hi;
	reg_message.payload[1] = addr_low;
	reg_message.payload[2] = buf_size;

	ret = ipc_sendmsg(ipc_cvip, &reg_message);
	if (ret < 0)
		goto msg_err;

	return 0;
msg_err:
	cvip_release_buffer(cvip_p, shared);
	return ret;
}

static int cvip_unreg_buf(void)
{
	struct ipc_message unreg_message;
	int ret;
	struct device *dev;

	if (IS_ERR_OR_NULL(ipc_cvip) || IS_ERR_OR_NULL(cvip_p))
		return -EINVAL;

	dev = &cvip_p->pci_dev->dev;
	/* sync tx channel */
	unreg_message.dest = 1;
	unreg_message.channel_idx = 3;
	unreg_message.length = 0;
	unreg_message.message_id = UNREGISTER_BUFFER;

	ret = ipc_sendmsg(ipc_cvip, &unreg_message);
	if (ret < 0)
		return ret;

	/*  iommu_unmap will be performed on next cvip_alive msg
	 *  in order to avoid the async data abort in CVIP
	 */
	cvip_release_buffer(cvip_p, shared);

	return 0;
}

static int install_dump_kernel(u32 dump_size)
{
	struct ipc_message install_message;
	int ret;

	if (IS_ERR_OR_NULL(ipc_cvip) || IS_ERR_OR_NULL(cvip_p))
		return -EINVAL;

	/* async tx channel */
	install_message.dest = 1;
	install_message.channel_idx = 2;
	install_message.length = 1;
	install_message.message_id = INSTALL_DUMP_KERNEL;
	install_message.payload[0] = dump_size;

	ret = ipc_sendmsg(ipc_cvip, &install_message);
	return ret;
}

int cvip_gpu_get_pages(struct page **pages, u32 *npages)
{
	struct page *first_page;
	u32 first_pfn;
	int i = 0;

	if (IS_ERR_OR_NULL(cvip_p) || IS_ERR_OR_NULL(cvip_p->x86_buf.buffer))
		return -EINVAL;

	first_page = sg_page(cvip_p->x86_buf.buffer->sg_table->sgl);
	first_pfn = page_to_pfn(first_page);
	*npages = CVIP_X86_BUF_SIZE >> PAGE_SHIFT;

	for (i = 0; i < *npages; i++)
		pages[i] = pfn_to_page(first_pfn + i);

	return 0;
}
EXPORT_SYMBOL(cvip_gpu_get_pages);

int cvip_set_gpuva(u64 gpuva)
{
	struct ipc_message gpuva_message;
	struct device *dev;
	phys_addr_t paddr;
	dma_addr_t iommu_paddr = 0;
	int ret = 0;

	if (IS_ERR_OR_NULL(ipc_cvip) || IS_ERR_OR_NULL(cvip_p))
		return -EINVAL;

	dev = &cvip_p->pci_dev->dev;

	paddr = page_to_phys(sg_page(cvip_p->x86_buf.buffer->sg_table->sgl));
	cvip_p->x86_buf.paddr = paddr;
	cvip_p->x86_buf.dma_addr = paddr;
	iommu_paddr = paddr;
	if (cvip_p->domain) {
		iommu_paddr = cvip_get_iova(cvip_p, &cvip_p->x86_buf,
					    CVIP_SHARED_BUF_SIZE);
		if (!iommu_paddr) {
			dev_err(dev, "fail to allocate iova\n");
			return -ENOMEM;
		}

		ret = iommu_map(cvip_p->domain, iommu_paddr, paddr,
				CVIP_SHARED_BUF_SIZE, IOMMU_READ | IOMMU_WRITE);
		if (ret) {
			dev_err(dev, "iommu mapping failed\n");
			return ret;
		}
	}

	gpuva_message.dest = 1;
	gpuva_message.channel_idx = 2;
	gpuva_message.length = 4;
	gpuva_message.message_id = SET_GPUVA;
	gpuva_message.payload[0] = gpuva >> 32;
	gpuva_message.payload[1] = gpuva;
	gpuva_message.payload[2] = iommu_paddr >> 32;
	gpuva_message.payload[3] = iommu_paddr;

	ret = ipc_sendmsg(ipc_cvip, &gpuva_message);
	if (ret < 0)
		dev_err(dev, "Failed to send set gpuva message\n");

	return ret;
}
EXPORT_SYMBOL(cvip_set_gpuva);

int cvip_is_alive(struct ipc_msg *msg)
{
	int ret = 0;
	struct ion_buffer *buffer;
	struct device *dev;
	u32 reboot_reason;
	u32 watchdog_count;

	if (IS_ERR_OR_NULL(cvip_p) || IS_ERR_OR_NULL(msg))
		return -EINVAL;

	reboot_reason = msg->msg.p.data[0];
	watchdog_count = msg->msg.p.data[1];

	dev = &cvip_p->pci_dev->dev;
	/*
	 * Complete the waiting of alive message
	 * and save ipc ring buffer info.
	 */
	complete(&cvip_p->completion);

	if (reboot_reason == CVIP_WATCHDOG_REBOOT) {
		cvip_p->shared_buf.buf_rgd = false;
		cvip_reboot = true;
		dev_err(dev, "CVIP watchdog reboot, watchdog reboot counter:%d\n",
			watchdog_count);
	}

	if (!ipc_cvip)
		ipc_init(&ipc_cvip);

	if (!cvip_p->x86_buf.buffer) {
		buffer = cvip_alloc_buffer(cvip_p, x86);
		if (IS_ERR_OR_NULL(buffer))
			return -ENOMEM;
	}

	if (!cvip_p->shared_buf.buf_rgd) {
		/* Don't unmap buffer in cvip reboot case */
		if (!cvip_reboot) {
			if (cvip_p->shared_buf.dma_addr) {
				ret = iommu_unmap(cvip_p->domain,
						  cvip_p->shared_buf.dma_addr,
						  CVIP_SHARED_BUF_SIZE);
				if (ret) {
					dev_err(dev, "iommu unmapping failed\n");
				} else {
					cvip_free_iova(cvip_p, cvip_p->shared_buf.dma_addr,
						       cvip_p->shared_buf.npages);
					cvip_p->shared_buf.dma_addr = 0;
					cvip_p->shared_buf.npages = 0;
				}
			}
		}
		ret = cvip_reg_buf();
		if (ret)
			return ret;
		cvip_p->shared_buf.buf_rgd = true;
		ret = install_dump_kernel(COREDUMP_SIZE);
		if (ret < 0)
			dev_warn(dev, "Failed to send dump kernel installing request\n");
		cvip_reboot = false;
	}

	return 0;
}
EXPORT_SYMBOL(cvip_is_alive);

int cvip_device_state_uevent(enum cvip_event_type event)
{
	int ret = 0;

	ret = ipc_send_uevent(event);

	return ret;
}
EXPORT_SYMBOL(cvip_device_state_uevent);

int cvip_handle_panic(struct ipc_buf *buf)
{
	phys_addr_t coredump_paddr;
	dma_addr_t coredump_dma;
	size_t coredump_len;
	void *coredump_vaddr;
	struct device *dev;
	struct timespec64 ts;
	struct rtc_time tm;
	struct ipc_message ack_message;
	char cored_file[64] = {0};
	int ret = 0, flag = IPC_X86_SAVE_DATA_ERR;
	static int is_new_file = 1;
	static struct file *fp;
	static loff_t off;

	dev = &cvip_p->pci_dev->dev;

	if (IS_ERR_OR_NULL(cvip_p) || IS_ERR_OR_NULL(buf) ||
	    IS_ERR_OR_NULL(ipc_cvip)) {
		ret = -EINVAL;
		goto ack;
	}

	if (buf->buf_size == 0) {
		flag = IPC_X86_SAVE_DATA_OK;
		is_new_file = 1;
		cvip_reboot = true;
		cvip_p->shared_buf.buf_rgd = false;
		goto ack;
	}

	coredump_dma = buf->buf_addr[0];
	coredump_dma <<= 32;
	coredump_dma += buf->buf_addr[1];
	coredump_len = buf->buf_size;

	if (coredump_dma < cvip_p->shared_buf.dma_addr ||
	    (coredump_dma + coredump_len) > (cvip_p->shared_buf.dma_addr + CVIP_SHARED_BUF_SIZE)) {
		dev_err(dev, "Coredump memory is outside of shared memory's boundaries\n");
		ret = -EINVAL;
		goto ack;
	}

	coredump_paddr = (coredump_dma - cvip_p->shared_buf.dma_addr) +
			 cvip_p->shared_buf.paddr;
	coredump_vaddr = memremap(coredump_paddr, coredump_len, MEMREMAP_WB);
	if (IS_ERR_OR_NULL(coredump_vaddr)) {
		dev_err(dev, "Failed to remap coredump buffer\n");
		ret = -ENOMEM;
		goto ack;
	}

	/* Use local time as suffix of coredump file name */
	ktime_get_real_ts64(&ts);
	ts.tv_sec -= sys_tz.tz_minuteswest * 60;
	rtc_time_to_tm(ts.tv_sec, &tm);

	if (is_new_file == 1) {
		is_new_file = 0;
		snprintf(cored_file, 64, "%s_%d-%d-%d_%d:%d:%d",
			 CVIP_COREDUMP, (tm.tm_year + 1900),
			 (tm.tm_mon + 1), tm.tm_mday, tm.tm_hour,
			 tm.tm_min, tm.tm_sec);

		fp = filp_open(cored_file, O_RDWR | O_TRUNC | O_CREAT, 0600);
		if (IS_ERR(fp)) {
			dev_err(dev, "Failed to open coredump file\n");
			ret = -EBADF;
			goto unmap_mem;
		}
	}

	ret = kernel_write(fp, coredump_vaddr, coredump_len, &off);
	if (ret < 0)
		dev_err(dev, "Failed to write coredump file\n");
	else
		flag = IPC_X86_SAVE_DATA_OK;

unmap_mem:
	memunmap(coredump_vaddr);

ack:
	/* sync tx channel */
	ack_message.dest = 1;
	ack_message.channel_idx = 2;
	ack_message.length = 2;
	ack_message.payload[0] = buf->buf_size;
	ack_message.payload[1] = flag;
	ack_message.message_id = PANIC_ACK;

	if (flag == IPC_X86_SAVE_DATA_ERR)
		is_new_file = 1;

	if (is_new_file == 1 && !IS_ERR(fp)) {
		filp_close(fp, NULL);
		off = 0;
	}

	if (ipc_sendmsg(ipc_cvip, &ack_message) < 0)
		dev_err(dev, "Failed to send panic ack message\n");

	return ret;
}
EXPORT_SYMBOL(cvip_handle_panic);

void cvip_low_power_ack(u32 state)
{
	trace_power_timing(__func__, "entry");
	if (IS_ERR_OR_NULL(cvip_p))
		return;
	complete(&cvip_p->power_completion);

	if (pm_suspend_target_state == PM_SUSPEND_MEM)
		cvip_p->is_state_d3 = state;
	trace_power_timing(__func__, "exit");
}
EXPORT_SYMBOL(cvip_low_power_ack);

#ifdef CONFIG_PM
static int cvip_pci_suspend(struct device *dev)
{
	struct ipc_message pm_message;
	int i;
	int ret = 0;

	trace_power_timing(__func__, "entry");
	if (IS_ERR_OR_NULL(cvip_p))
		return -EINVAL;
	/* Skip in x86 single boot case */
	if (IS_ERR_OR_NULL(ipc_cvip))
		return 0;

	dev_dbg(dev, "Entering suspend\n");
	for (i = 0; i < IPC_CHN_NUM; i++) {
		if (cvip_p->ipc_dev.vecs[i])
			irq_set_irq_wake(cvip_p->ipc_dev.vecs[i], 1);
	}

	/* sync tx channel */
	pm_message.dest = 1;
	pm_message.channel_idx = 3;
	pm_message.length = 2;
	pm_message.message_id = SEND_PM_EVENT;
	pm_message.payload[0] = LOW_POWER_MODE;

	if (pm_suspend_target_state == PM_SUSPEND_TO_IDLE)
		pm_message.payload[1] = X86_IDLE;
	else if (pm_suspend_target_state == PM_SUSPEND_STANDBY)
		pm_message.payload[1] = X86_STANDBY;
	else if (pm_suspend_target_state == PM_SUSPEND_MEM)
		pm_message.payload[1] = X86_S3;

	trace_power_timing(__func__, "ipc_sendmsg");
	reinit_completion(&cvip_p->power_completion);
	ret = ipc_sendmsg(ipc_cvip, &pm_message);
	if (ret < 0) {
		dev_err(dev, "Failed to send suspend ipc message\n");
		return ret;
	}

	trace_power_timing(__func__, "wait_for_completion");
	ret = wait_for_completion_timeout(&cvip_p->power_completion, 10 * HZ);
	if (!ret) {
		dev_err(dev, "Failed to receive low power ack from CVIP\n");
		return -EINVAL;
	}
	trace_power_timing(__func__, "wait_for_completion done");

	if (pm_suspend_target_state == PM_SUSPEND_MEM) {
		if (cvip_p->is_state_d3) {
			dev_err(dev, "CVIP failed to enter D3\n");
			return -EINVAL;
		}
		pci_save_state(cvip_p->pci_dev);
		pci_disable_device(cvip_p->pci_dev);
		pci_set_power_state(cvip_p->pci_dev, PCI_D3hot);
	}
	trace_power_timing(__func__, "exit");
	return 0;
}

static int cvip_pci_resume(struct device *dev)
{
	struct ipc_message pm_message;
	int i;
	int ret = 0;

	trace_power_timing(__func__, "entry");
	if (IS_ERR_OR_NULL(cvip_p))
		return -EINVAL;
	/* Skip in x86 single boot case */
	if (IS_ERR_OR_NULL(ipc_cvip))
		return 0;

	dev_dbg(dev, "Resuming from suspend\n");

	ipc_mboxes_cleanup(ipc_cvip, IPC_CHAN);

	/* reset the ipc irq affinity after S3 */
	if (pm_suspend_target_state == PM_SUSPEND_MEM)
		ipc_irq_affinity_restore(&cvip_p->ipc_dev);

	trace_power_timing(__func__, "mailbox cleanup done");
	for (i = 0; i < IPC_CHN_NUM; i++) {
		if (cvip_p->ipc_dev.vecs[i])
			irq_set_irq_wake(cvip_p->ipc_dev.vecs[i], 0);
	}

	trace_power_timing(__func__, "set_irq done");
	pci_set_power_state(cvip_p->pci_dev, PCI_D0);
	trace_power_timing(__func__, "set pci_d0");
	pci_restore_state(cvip_p->pci_dev);
	trace_power_timing(__func__, "restor_pci_state");
	ret = pci_enable_device(cvip_p->pci_dev);
	if (ret) {
		dev_err(dev, "Failed to enable cvip device after resuming\n");
		return ret;
	}

	trace_power_timing(__func__, "pci_state_change done");
	/* sync tx channel */
	pm_message.dest = 1;
	pm_message.channel_idx = 3;
	pm_message.length = 2;
	pm_message.message_id = SEND_PM_EVENT;
	pm_message.payload[0] = LOW_POWER_MODE;
	pm_message.payload[1] = X86_S0;

	ret = ipc_sendmsg(ipc_cvip, &pm_message);
	if (ret < 0) {
		dev_err(dev, "Failed to send ipc message after resuming\n");
		return ret;
	}

	trace_power_timing(__func__, "ipc_send backto S0");
	if (mem_sleep_current == PM_SUSPEND_MEM)
		ipc_send_uevent(cvip_resume);

	trace_power_timing(__func__, "exit");
	return 0;
}
#endif

static UNIVERSAL_DEV_PM_OPS(cvip_pm_ops, cvip_pci_suspend,
			    cvip_pci_resume, NULL);

static ssize_t ipc_reboot_show(struct device *dev,
			       struct device_attribute *attr,
			       char *buf)
{
	pr_debug(" %s\n", __func__);

	return 0;
}

static ssize_t ipc_reboot_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	u32 val;
	int ret = 0;

	if (IS_ERR_OR_NULL(ipc_cvip) || IS_ERR_OR_NULL(cvip_p))
		return -EINVAL;

	dev = &cvip_p->pci_dev->dev;
	ret = kstrtou32(buf, 10, &val);
	if (ret)
		return ret;

	if (val) {
		ret = ipc_send_reboot_message();
		if (ret < 0) {
			cvip_p->shared_buf.buf_rgd = false;
			cvip_reboot = true;
		}
	}

	ipc_deinit(ipc_cvip);
	ipc_cvip = NULL;

	return count;
}

static ssize_t ipc_s2ram_show(struct device *dev,
			       struct device_attribute *attr,
			       char *buf)
{
	pr_debug(" %s\n", __func__);

	return 0;
}

static ssize_t ipc_s2ram_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	u32 val;
	struct ipc_message pm_message;
	int ret = 0;

	if (IS_ERR_OR_NULL(ipc_cvip))
		return -EINVAL;

	dev = &cvip_p->pci_dev->dev;
	ret = kstrtou32(buf, 10, &val);
	if (ret)
		return ret;

	if (val) {
		/* async tx channel */
		pm_message.dest = 1;
		pm_message.channel_idx = 2;
		pm_message.length = 0;
		pm_message.message_id = S2RAM_REQ;
		ipc_sendmsg(ipc_cvip, &pm_message);
	}

	ret = wait_for_completion_timeout(&cvip_p->completion, 10 * HZ);
	if (!ret) {
		dev_err(dev, "Failed to receive response from CVIP\n");
		return -EINVAL;
	}

	if (cvip_p->is_state_d3) {
		dev_err(dev, "CVIP failed to enter D3\n");
		return -EINVAL;
	}

	return count;
}

static DEVICE_ATTR_RW(ipc_reboot);
static DEVICE_ATTR_RW(ipc_s2ram);

struct attribute *ipc_attrs[] = {
	&dev_attr_ipc_reboot.attr,
	&dev_attr_ipc_s2ram.attr,
	NULL,
};

struct attribute_group ipc_attr_group = {
	.attrs = ipc_attrs,
};

static long cvip_ion_buf_iommu_map(struct ion_client *client,
				   unsigned int cmd, unsigned long arg)
{
	struct device *dev = &cvip_p->pci_dev->dev;
	struct iommu_domain *domain = cvip_p->domain;
	struct ion_buf_mapping_data buf_mapping_data = {0};
	struct dma_buf *dma_buf;
	struct sg_table *sg;
	struct dma_buf_attachment *dma_attach = NULL;
	unsigned int npages;
	dma_addr_t address = 0;
	unsigned int mapped_size;
	int ret = -EFAULT;

	if (_IOC_SIZE(cmd) > sizeof(buf_mapping_data))
		goto err_param;
	if (_IOC_DIR(cmd) & _IOC_WRITE) {
		if (copy_from_user(&buf_mapping_data,
				   (void __user *)arg,
				   _IOC_SIZE(cmd)))
			goto err_param;
	}

	if (!domain) {
		dev_err(dev, "Invalid cvip iommu domain\n");
		goto err_param;
	}

	dma_buf = dma_buf_get(buf_mapping_data.fd);
	if (IS_ERR(dma_buf)) {
		dev_err(dev, "Failed to get dma buffer\n");
		goto err_dma_buf_get;
	}

	dma_attach = dma_buf_attach(dma_buf, dev);
	if (IS_ERR(dma_attach)) {
		dev_err(dev, "Failed to attach device to dmabuf\n");
		goto err_dma_buf_attach;
	}
	buf_mapping_data.map_attach = (void *)dma_attach;

	sg = dma_buf_map_attachment(dma_attach, DMA_BIDIRECTIONAL);
	if (IS_ERR(sg)) {
		dev_err(dev, "Failed to get sg_table\n");
		goto err_dma_buf_map_attachment;
	}
	buf_mapping_data.map_table = (void *)sg;

	if (!buf_mapping_data.map_addr) {
		/* if map_addr is not provided by user, allocate iova */
		npages = PAGE_ALIGN(dma_buf->size) / PAGE_SIZE;
		address = cvip_alloc_iova(cvip_p, npages, *dev->dma_mask);
		buf_mapping_data.map_addr = (void *)address;
	}
	mapped_size = iommu_map_sg(domain,
				   (unsigned long)buf_mapping_data.map_addr,
				   sg->sgl,
				   sg->nents,
				   IOMMU_READ | IOMMU_WRITE);
	if (mapped_size <= 0) {
		dev_err(dev, "Failed to iommu map sg\n");
		goto err_iommu_map_sg;
	}

	buf_mapping_data.map_size = mapped_size;
	dma_buf_put(dma_buf);

	if (_IOC_DIR(cmd) & _IOC_READ) {
		if (copy_to_user((void __user *)arg,
				 &buf_mapping_data,
				 _IOC_SIZE(cmd)))
			goto err_param_cpy;
	}

	return 0;

err_param_cpy:
	iommu_unmap(domain,
		    (unsigned long)buf_mapping_data.map_addr,
		    buf_mapping_data.map_size);
	if (address)
		cvip_free_iova(cvip_p, address, npages);
err_iommu_map_sg:
	dma_buf_unmap_attachment(dma_attach, sg, DMA_BIDIRECTIONAL);
err_dma_buf_map_attachment:
	dma_buf_detach(dma_buf, dma_attach);
err_dma_buf_attach:
	dma_buf_put(dma_buf);
err_dma_buf_get:
err_param:
	return ret;
}

static long cvip_ion_buf_iommu_unmap(struct ion_client *client,
				     unsigned int cmd, unsigned long arg)
{
	struct device *dev = &cvip_p->pci_dev->dev;
	struct iommu_domain *domain = cvip_p->domain;
	struct ion_buf_mapping_data buf_mapping_data = {0};
	struct dma_buf *dma_buf;
	struct sg_table *sg;
	struct dma_buf_attachment *dma_attach = NULL;
	int ret = -EFAULT;

	if (_IOC_SIZE(cmd) > sizeof(buf_mapping_data))
		goto err_param;
	if (_IOC_DIR(cmd) & _IOC_WRITE) {
		if (copy_from_user(&buf_mapping_data,
				   (void __user *)arg,
				   _IOC_SIZE(cmd)))
			goto err_param;
	}

	if (!domain) {
		dev_err(dev, "Invalid cvip iommu domain\n");
		goto err_param;
	}

	dma_attach = (struct dma_buf_attachment *)buf_mapping_data.map_attach;
	if (!dma_attach) {
		dev_err(dev, "Failed to get dma_buf_attachment\n");
		goto err_dma_attach;
	}

	dma_buf = dma_attach->dmabuf;
	sg = (struct sg_table *)buf_mapping_data.map_table;
	if (!dma_buf || !sg) {
		dev_err(dev, "Invalid %s retrieved\n",
			(!dma_buf ? "dma_buf" : "sg_table"));
		goto err_dma_attach;
	}

	if (!buf_mapping_data.map_addr) {
		dev_err(dev, "Invalid map_addr provided\n");
		goto err_dma_attach;
	}
	if (!buf_mapping_data.map_size) {
		dev_err(dev, "Invalid map_size provided\n");
		goto err_dma_attach;
	}

	iommu_unmap(domain,
		    (unsigned long)buf_mapping_data.map_addr,
		    buf_mapping_data.map_size);

	cvip_free_iova(cvip_p, (unsigned long)buf_mapping_data.map_addr,
		       PAGE_ALIGN(dma_buf->size) / PAGE_SIZE);

	dma_buf_unmap_attachment(dma_attach, sg, DMA_BIDIRECTIONAL);
	dma_buf_detach(dma_buf, dma_attach);

	if (_IOC_DIR(cmd) & _IOC_READ) {
		if (copy_to_user((void __user *)arg,
				 &buf_mapping_data,
				 _IOC_SIZE(cmd)))
			goto err_param;
	}

	return 0;

err_dma_attach:
err_param:
	return ret;
}

static long mero_ion_custom_ioctl(struct ion_client *client,
				  unsigned int cmd,
				  unsigned long arg)
{
	struct ion_buf_physdata buf_data;
	struct device *dev = &cvip_p->pci_dev->dev;

	switch (cmd) {
	case ION_IOC_MERO_GET_BUFFER_PADDR:
	{
		struct dma_buf *dma_buf;
		struct sg_table *sg;

		if (_IOC_SIZE(cmd) > sizeof(buf_data)) {
			return -EINVAL;
		} else if (_IOC_DIR(cmd) & _IOC_WRITE) {
			if (copy_from_user(&buf_data,
					   (void __user *)arg,
					   _IOC_SIZE(cmd)))
				return -EFAULT;
		}

		dma_buf = dma_buf_get(buf_data.fd);
		if (IS_ERR(dma_buf)) {
			dev_err(dev, "Failed to get dma buffer\n");
			return -EFAULT;
		}

		sg = ion_sg_table(dma_buf);
		if (IS_ERR(sg)) {
			dev_err(dev, "Failed to get sg_table\n");
			dma_buf_put(dma_buf);
			return -EFAULT;
		}

		buf_data.paddr = page_to_phys(sg_page(sg->sgl));
		dma_buf_put(dma_buf);

		if (_IOC_DIR(cmd) & _IOC_READ) {
			if (copy_to_user((void __user *)arg,
					 &buf_data,
					 _IOC_SIZE(cmd)))
				return -EFAULT;
		}
		break;
	}
	case ION_IOC_MERO_IOMMU_MAP:
		/* IOWR
		 * Params: fd,       ion buffer fd
		 *         map_addr, address to be mapped (if NULL, 1:1 mapping)
		 *         map_size, mapped size
		 * Return: Pass or Fail
		 */
		return cvip_ion_buf_iommu_map(client, cmd, arg);
	case ION_IOC_MERO_IOMMU_UNMAP:
		/* IOWR
		 * Params: fd,       ion buffer fd
		 *         map_addr, address to be unmapped
		 *         map_size, size to be unmapped
		 * Return: Pass or Fail
		 */
		return cvip_ion_buf_iommu_unmap(client, cmd, arg);
	default:
		return -ENOTTY;
	}

	return 0;
}

static size_t cvip_memcpy_movntdqa(void *dst, void *src, size_t count)
{
	size_t movcnt = count / CVIP_MOVNTDQA_SIZE;

	kernel_fpu_begin();

	__uaccess_begin();
	while (movcnt >= MAX_MOVNTDQA_OPS) {
		asm("movntdqa (%0), %%xmm0\n"
		    "movntdqa 16(%0), %%xmm1\n"
		    "movntdqa 32(%0), %%xmm2\n"
		    "movntdqa 48(%0), %%xmm3\n"
		    "movaps %%xmm0, (%1)\n"
		    "movaps %%xmm1, 16(%1)\n"
		    "movaps %%xmm2, 32(%1)\n"
		    "movaps %%xmm3, 48(%1)\n"
		    :: "r" (src), "r" (dst) : "memory");
		src += (CVIP_MOVNTDQA_SIZE * 4);
		dst += (CVIP_MOVNTDQA_SIZE * 4);
		movcnt -= MAX_MOVNTDQA_OPS;
	}

	while (movcnt--) {
		asm("movntdqa (%0), %%xmm0\n"
		    "movaps %%xmm0, (%1)\n"
		    :: "r" (src), "r" (dst) : "memory");
		src += CVIP_MOVNTDQA_SIZE;
		dst += CVIP_MOVNTDQA_SIZE;
	}
	__uaccess_end();

	kernel_fpu_end();

	return 0;
}

static int cvip_s_intf_open(struct inode *inode, struct file *file)
{
	file->private_data = cvip_p;
	return 0;
}

static int cvip_s_intf_release(struct inode *inode, struct file *file)
{
	return 0;
}

static ssize_t cvip_s_intf_read(struct file *file, char __user *buf,
				size_t count, loff_t *ppos)
{
	struct cvip_dev *cvip_dev;

	cvip_dev = (struct cvip_dev *)file->private_data;
	if (!cvip_dev)
		return -ENODEV;

	if (*ppos > cvip_dev->res_len || count > cvip_dev->res_len)
		return -EINVAL;

	if (unlikely(((unsigned long)buf | (unsigned long)(cvip_dev->base + *ppos) | count) & 15))
		return simple_read_from_buffer(buf, count, ppos, cvip_dev->base, count);
	else
		return cvip_memcpy_movntdqa(buf, cvip_dev->base + *ppos, count);
}

static ssize_t cvip_s_intf_write(struct file *file, const char __user *buf,
				 size_t count, loff_t *ppos)
{
	struct cvip_dev *cvip_dev;

	cvip_dev = (struct cvip_dev *)file->private_data;
	if (!cvip_dev)
		return -ENODEV;

	if (*ppos > cvip_dev->res_len || count > cvip_dev->res_len)
		return -EINVAL;

	return simple_write_to_buffer(cvip_dev->base, count, ppos, buf, count);
}

static long cvip_s_intf_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	return -ENOIOCTLCMD;
}

static const struct file_operations cvip_s_intf_fops = {
	.owner = THIS_MODULE,
	.open = cvip_s_intf_open,
	.read = cvip_s_intf_read,
	.write = cvip_s_intf_write,
	.unlocked_ioctl = cvip_s_intf_ioctl,
	.release = cvip_s_intf_release,
	.llseek = default_llseek,
};

static struct miscdevice cvip_s_intf_dev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = CDEV_NAME,
	.fops = &cvip_s_intf_fops,
};

static int cvip_xport_open(struct inode *inode, struct file *file)
{
	file->private_data = cvip_p;
	i_size_write(inode, CVIP_XPORT_BUFSIZE);

	return 0;
}

static int cvip_xport_release(struct inode *inode, struct file *file)
{
	return 0;
}

static ssize_t cvip_xport_read(struct file *file, char __user *buf,
			       size_t count, loff_t *ppos)
{
	struct cvip_dev *cvip_dev;
	struct cvip_buffer *sbuf;
	void *vaddr;

	cvip_dev = (struct cvip_dev *)file->private_data;
	if (!cvip_dev)
		return -ENODEV;
	if (*ppos > CVIP_XPORT_BUFSIZE || count > CVIP_XPORT_BUFSIZE) {
		pr_err("%s: ppos:%d, count:%d > XPORT_SIZE:%d\n",
		       __func__, (int)*ppos, (int)count, CVIP_XPORT_BUFSIZE);
		return -EINVAL;
	}

	sbuf = &cvip_dev->shared_buf;
	vaddr = sbuf->buffer->vaddr + COREDUMP_SIZE;

	return simple_read_from_buffer(buf, count, ppos, vaddr, count);
}

static ssize_t cvip_xport_write(struct file *file, const char __user *buf,
				size_t count, loff_t *ppos)
{
	struct cvip_dev *cvip_dev;
	struct cvip_buffer *sbuf;
	void *vaddr;

	cvip_dev = (struct cvip_dev *)file->private_data;
	if (!cvip_dev)
		return -ENODEV;

	if (*ppos > CVIP_XPORT_BUFSIZE || count > CVIP_XPORT_BUFSIZE) {
		pr_err("%s: ppos:%d, count:%d > XPORT_SIZE:%d\n",
		       __func__, (int)*ppos, (int)count, CVIP_XPORT_BUFSIZE);
		return -EINVAL;
	}

	sbuf = &cvip_dev->shared_buf;
	vaddr = sbuf->buffer->vaddr + COREDUMP_SIZE;

	return simple_write_to_buffer(vaddr, count, ppos, buf, count);
}

static long cvip_xport_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	return -ENOIOCTLCMD;
}

static const struct file_operations cvip_xport_fops = {
	.owner = THIS_MODULE,
	.open = cvip_xport_open,
	.read = cvip_xport_read,
	.write = cvip_xport_write,
	.unlocked_ioctl = cvip_xport_ioctl,
	.release = cvip_xport_release,
	.llseek = default_llseek,
};

static struct miscdevice cvip_xport_dev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = CDEV_XPORT_NAME,
	.fops = &cvip_xport_fops,
};

static int cvip_add_device_node(struct cvip_dev *cvip_dev)
{
	int ret;
	struct device *dev;

	dev = cvip_dev->ipc_dev.dev;

	ret = misc_register(&cvip_s_intf_dev);
	if (ret) {
		dev_err(dev, "misc_register failed, err = %d\n", ret);
		return ret;
	}

	dev_info(dev, "Device node %s has initialized successfully.\n", CDEV_NAME);

	ret = misc_register(&cvip_xport_dev);
	if (ret) {
		dev_err(dev, "misc_register failed for cvipx, err = %d\n", ret);
		return ret;
	}

	dev_info(dev, "Device node %s has initialized successfully.\n", CDEV_XPORT_NAME);
	return 0;
}

static void cvip_remove_node(struct cvip_dev *cvip_dev)
{
	misc_deregister(&cvip_s_intf_dev);
	misc_deregister(&cvip_xport_dev);
}

static int cvip_probe(struct pci_dev *pci_dev,
			     const struct pci_device_id *id)
{
	struct cvip_dev *cvip;
	struct device *dev = &pci_dev->dev;
	int ret = -ENODEV;
	int nr_vecs, i;
	struct iommu_domain *domain = NULL;

	cvip = devm_kzalloc(dev, sizeof(*cvip), GFP_KERNEL);
	if (!cvip)
		return -ENOMEM;

	cvip_p = cvip;
	cvip->pci_dev = pci_dev;

	atomic_notifier_chain_register(&panic_notifier_list,
				       &cvip_panic_nb);

	/* Retrieve resources */
	ret = pcim_enable_device(pci_dev);
	if (ret) {
		dev_err(dev, "Failed to enable CVIP PCI device (%d)\n",
			ret);
		return ret;
	}

	cvip->ipc_dev.dev = &pci_dev->dev;

	/* ioremap different S-interface regions */
	ret = cvip_s_intf_ioremap(pci_dev);
	if (ret)
		return ret;

	/* /dev/cvip mapping use write-combined for fast memory copy */
	cvip->res_len = DATA0_SIZE;
	cvip->base = get_sintf_base(SINTF_DATA0);
	if (!cvip->base) {
		dev_err(dev, "I/O memory remapping failed\n");
		return -ENOMEM;
	}

	/* IPC register access must use NC mapping */
	cvip->ipc_dev.base = get_sintf_base(SINTF_IPC);
	if (!cvip->ipc_dev.base) {
		dev_err(dev, "IPC register ioremap failed\n");
		return -ENOMEM;
	}
	cvip->ipc_dev.nr_mbox = sizeof(mb_param)/sizeof(mb_param[0]);
	memcpy(cvip->ipc_dev.param, mb_param, sizeof(mb_param));

	nr_vecs = pci_msix_vec_count(pci_dev);
	ret = pci_alloc_irq_vectors(pci_dev, nr_vecs, nr_vecs, PCI_IRQ_MSIX);
	if (ret < 0) {
		dev_err(dev, "msix vectors %d alloc failed\n", nr_vecs);
		goto init_err;
	}

	for (i = 0; i < nr_vecs && i < IPC_CHN_NUM; i ++)
		cvip->ipc_dev.vecs[i] = pci_irq_vector(pci_dev, i);

	ret = mero_mailbox_init(&cvip->ipc_dev);
	if (ret) {
		dev_err(dev, "Failed to initialize mailboxes\n");
		goto init_err;
	}
	if (device_iommu_mapped(dev)) {
		dev_info(dev, "CVIP is iommu mapped\n");
		domain = iommu_get_domain_for_dev(dev);
		if (!domain) {
			dev_err(dev, "Failed to get iommu domain\n");
			goto init_err;
		}
		cvip->domain = domain;

		/* copy amd iommu iova domain to locate all reserved iova */
		cvip->iovad = amd_get_iova_domain(cvip->domain);
	}

	ret = sysfs_create_group(&dev->kobj, &ipc_attr_group);
	if (ret) {
		dev_err(dev, "Failed create sysfs group\n");
		goto init_err;
	}

	ret = cvip_add_device_node(cvip);
	if (ret)
		goto init_err;

	pci_set_master(pci_dev);
	init_completion(&cvip->completion);
	init_completion(&cvip->power_completion);
	ion_device_create(mero_ion_custom_ioctl);
	device_init_wakeup(dev, true);
	pci_set_drvdata(pci_dev, cvip);
	dev_info(dev, "Successfully probed CVIP device\n");

	return 0;
init_err:
	pcim_iounmap_regions(pci_dev, 1 << 0);
	return ret;
}

static void cvip_remove(struct pci_dev *pci_dev)
{
	pci_iounmap(pci_dev, cvip_p->base);
	pci_set_drvdata(pci_dev, NULL);
	cvip_unreg_buf();
	cvip_release_buffer(cvip_p, x86);
	ion_device_destroy();
	cvip_remove_node(cvip_p);
	atomic_notifier_chain_unregister(&panic_notifier_list,
					 &cvip_panic_nb);
	cvip_p = NULL;
}

static void cvip_shutdown(struct pci_dev *pci_dev)
{
	if (IS_ERR_OR_NULL(ipc_cvip))
		return;

	ipc_send_reboot_message();
}

static const struct pci_device_id cvip_pci_tbl[] = {
	{PCI_VDEVICE(AMD, PCI_DEVICE_ID_CVIP)},
	{0}
};
MODULE_DEVICE_TABLE(pci, cvip_pci_tbl);

static struct pci_driver cvip_driver = {
	.name		= "cvip",
	.id_table	= cvip_pci_tbl,
	.probe		= cvip_probe,
	.remove		= cvip_remove,
	.shutdown	= cvip_shutdown,
	.driver         = {
		.pm     = &cvip_pm_ops,
	},
};

static int __init cvip_driver_init(void)
{
	if (ipc_dev_pre_init())
		pr_warn("ipc_dev_pre_init err\n");

	return pci_register_driver(&cvip_driver);
}

static void __exit cvip_driver_exit(void)
{
	ipc_dev_cleanup();
	pci_unregister_driver(&cvip_driver);
}

module_init(cvip_driver_init);
module_exit(cvip_driver_exit);

MODULE_DESCRIPTION("CVIP driver");
