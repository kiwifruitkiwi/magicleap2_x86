/*
 * CVIP driver for AMD SoC CVIP Subsystem.
 *
 * Copyright (C) 2019-2022 Advanced Micro Devices, Inc. All rights reserved.
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
#include <uapi/linux/cvip_status.h>
#include <linux/fs.h>
#include <linux/string.h>

#include "mero_ipc_i.h"
#include "ipc.h"
#include "cvip.h"
#include "ion.h"
#include "mero_ion.h"
#include "cvip_s_intf.h"

#define PCI_DEVICE_ID_CVIP	0x149d
#define CVIP_MLNET_BUF_SIZE	(10 * 1024 * 1024)
#define CVIP_SHARED_BUF_SIZE	(64 * 1024 * 1024)
#if defined(CONFIG_CVIP_CRASH_KERNEL)
#define COREDUMP_SIZE		(32 * 1024 * 1024)
#else
#define COREDUMP_SIZE		(1 * 1024 * 1024)
#endif
#define CVIP_XPORT_BUFSIZE	(CVIP_SHARED_BUF_SIZE - COREDUMP_SIZE)
#define CVIP_COREDUMP		("/data/cvip/crash/kernel")
#define CVIP_COREDUMP_EXT	(".kcore")
#define CVIP_CORE_MAX_COUNT	(5)
#define CVIP_CORE_MAX_NLEN	(30)
#define CDEV_NAME		"cvip"
#define CDEV_SPORT_NAME		"cvips"
#define CDEV_XPORT_NAME		"cvipx"
#define CDEV_STATUS_NAME	"cvip_status"
#define CVIP_MOVNTDQA_SIZE	16
#define MAX_MOVNTDQA_OPS	4
#define X86_IRQ_WAKE_SRC	25

#define AID_ROOT             (0)
#define AID_SYSTEM           (1000)
#define CVIP_PANIC_FILE_PERM (0660)

#define TO_PFN(addr)		((addr) >> PAGE_SHIFT)

void *x86_cvip_shared_ddr_base_addr;
EXPORT_SYMBOL(x86_cvip_shared_ddr_base_addr);

void *mlnet_shared_ddr_base_addr;
EXPORT_SYMBOL(mlnet_shared_ddr_base_addr);

void *dump_ddr_base_addr;
EXPORT_SYMBOL(dump_ddr_base_addr);

/*
 * Data structure used when a core-dump of cvip is being generated
 */
struct cvip_dir_context {
	struct dir_context ctx;
	char kcores[CVIP_CORE_MAX_COUNT][CVIP_CORE_MAX_NLEN];
	uint8_t found_files;
	uint8_t index;
	uint16_t max_id;
};

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
	dma_addr_t iommu_paddr;
	unsigned int npages;
};

enum cvip_buffer_type {
	shared,
	x86,
	mlnet,
	dump,
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
	struct cvip_buffer mlnet_buf;
	struct cvip_buffer dump_buf;
	struct cvip_buffer x86_buf;
	u32 is_state_d3;
	struct iommu_domain *domain;
	struct iova_domain *iovad;
	void __iomem *base;
	resource_size_t res_len;
	struct cvip_status status;
	/* Used to signal completion from display state */
	wait_queue_head_t wq;
	bool flag;
	bool drm_dpms;
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
static bool cvip_allow_s2idle;

static int cvip_wdt_notify(struct notifier_block *nb,
			     unsigned long event, void *data)
{
	cvip_p->status.wdt_count++;

	return NOTIFY_OK;
}

static struct notifier_block cvip_wdt_nb = {
	.notifier_call = cvip_wdt_notify,
};

void cvip_update_panic_count(void)
{
	cvip_p->status.panic_count++;
}

static int ipc_send_reboot_message(void)
{
	struct ipc_message pm_message;

	if (cvip_p->status.panic_count) {
		// CVIP is probably in a bad state, even if it's up,
		// don't send the reboot message just shut the system down
		return 0;
	}

	pm_message.dest = 1;
	pm_message.channel_idx = 3;
	pm_message.length = 0;
	pm_message.message_id = REBOOT_REQ;

	return ipc_sendmsg_timeout(ipc_cvip, &pm_message, 0);
}

static int ipc_send_panic_message(void)
{
	struct ipc_message pm_message;

	pm_message.dest = 1;
	pm_message.channel_idx = 3;
	pm_message.length = 0;
	pm_message.message_id = NOVA_PANIC;

	return ipc_sendmsg_timeout(ipc_cvip, &pm_message, 0);
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
	ipc_send_panic_message();

	return NOTIFY_OK;
}

static struct notifier_block cvip_panic_nb = {
	.notifier_call = cvip_panic_notify,
};

const struct cvip_status *cvip_get_status(void)
{
	return (const struct cvip_status *)&cvip_p->status;
}
EXPORT_SYMBOL(cvip_get_status);

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
	dev_err(dev, "allocate iova:0x%llx, bufsize:%llx, npages:%lld\n",
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

	dev = &cvip_p->pci_dev->dev;
	switch (type) {
	case shared:
		size = CVIP_SHARED_BUF_SIZE;
		break;
	case x86:
		size = CVIP_X86_BUF_SIZE;
		break;
	case mlnet:
		size = CVIP_MLNET_BUF_SIZE;
		break;
	case dump:
		size = COREDUMP_SIZE;
		break;
	default:
		dev_err(dev, "unknown heap type %d\n", type);
		return ERR_PTR(-EINVAL);
	}

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

	switch (type) {
	case shared:
		cvip->shared_buf.fd = fd;
		cvip->shared_buf.dma_buf = dma_buf;
		cvip->shared_buf.buffer = buffer;
		x86_cvip_shared_ddr_base_addr = vaddr;
		break;
	case x86:
		cvip->x86_buf.fd = fd;
		cvip->x86_buf.dma_buf = dma_buf;
		cvip->x86_buf.buffer = buffer;
		break;
	case mlnet:
		cvip->mlnet_buf.fd = fd;
		cvip->mlnet_buf.dma_buf = dma_buf;
		cvip->mlnet_buf.buffer = buffer;
		mlnet_shared_ddr_base_addr = vaddr;
		break;
	case dump:
		cvip->dump_buf.fd = fd;
		cvip->dump_buf.dma_buf = dma_buf;
		cvip->dump_buf.buffer = buffer;
		dump_ddr_base_addr = vaddr;
		break;
	default:
		dev_err(dev, "unknown buffer type\n");
		goto map_err;
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

	switch (type) {
	case shared:
		dma_buf = cvip->shared_buf.dma_buf;
		buffer = cvip->shared_buf.buffer;
		fd = cvip->shared_buf.fd;
		break;
	case x86:
		dma_buf = cvip->x86_buf.dma_buf;
		buffer = cvip->x86_buf.buffer;
		fd = cvip->x86_buf.fd;
		break;
	case mlnet:
		dma_buf = cvip->mlnet_buf.dma_buf;
		buffer = cvip->mlnet_buf.buffer;
		fd = cvip->mlnet_buf.fd;
		break;
	case dump:
		dma_buf = cvip->dump_buf.dma_buf;
		buffer = cvip->dump_buf.buffer;
		fd = cvip->dump_buf.fd;
		break;
	default:
		return;
	}

	if (IS_ERR_OR_NULL(dma_buf) || IS_ERR_OR_NULL(buffer) || fd < 0)
		return;

	dma_buf_kunmap(dma_buf, 0, buffer->vaddr);
	dma_buf_put(dma_buf);
	ion_buffer_destroy(buffer);
	ksys_close(fd);

	switch (type) {
	case shared:
		cvip->shared_buf.dma_buf = NULL;
		cvip->shared_buf.buffer = NULL;
		cvip->shared_buf.fd = -1;
		x86_cvip_shared_ddr_base_addr = NULL;
		break;
	case x86:
		cvip->x86_buf.dma_buf = NULL;
		cvip->x86_buf.buffer = NULL;
		cvip->x86_buf.fd = -1;
		break;
	case mlnet:
		cvip->mlnet_buf.dma_buf = NULL;
		cvip->mlnet_buf.buffer = NULL;
		cvip->mlnet_buf.fd = -1;
		mlnet_shared_ddr_base_addr = NULL;
		break;
	case dump:
		cvip->dump_buf.dma_buf = NULL;
		cvip->dump_buf.buffer = NULL;
		cvip->dump_buf.fd = -1;
		dump_ddr_base_addr = NULL;
		break;
	default:
		/* never should get here */
		return;
	}
}

static int cvip_reg_buf(enum cvip_buffer_type buffer_type)
{
	struct ipc_message reg_message;
	u32 addr_hi, addr_low, buf_size = 0;
	int ret;
	phys_addr_t paddr;
	dma_addr_t iommu_paddr = 0;
	struct ion_buffer *buffer = NULL;
	struct device *dev;
	struct cvip_buffer *cvip_buf;

	if (IS_ERR_OR_NULL(ipc_cvip) || IS_ERR_OR_NULL(cvip_p))
		return -EINVAL;

	dev = &cvip_p->pci_dev->dev;

	switch (buffer_type) {
	case mlnet:
		buf_size = CVIP_MLNET_BUF_SIZE;
		cvip_buf = &cvip_p->mlnet_buf;
		break;
	case shared:
		buf_size = CVIP_SHARED_BUF_SIZE;
		cvip_buf = &cvip_p->shared_buf;
		break;
	case dump:
		buf_size = COREDUMP_SIZE;
		cvip_buf = &cvip_p->dump_buf;
		break;
	default:
		dev_err(dev, "invalid buffer_type %d\n", buffer_type);
		return -EINVAL;
	}

	/* The share buffer should be kept during CVIP reboot */
	if (!cvip_reboot) {
		buffer = cvip_alloc_buffer(cvip_p, buffer_type);
		if (IS_ERR_OR_NULL(buffer))
			return -EINVAL;

		paddr = page_to_phys(sg_page(buffer->sg_table->sgl));
		dev_info(dev, "cvip: %d dma buf %llx\n", buffer_type,
			 (uint64_t)paddr);

		cvip_buf->paddr = paddr;
		cvip_buf->dma_addr = paddr;

		if (cvip_p->domain) {
			iommu_paddr = cvip_get_iova(cvip_p,
						    cvip_buf,
						    buf_size);
			if (!iommu_paddr) {
				dev_err(dev, "failed to allocate iova\n");
				ret = -ENOMEM;
				goto msg_err;
			}

			ret = iommu_map(cvip_p->domain, iommu_paddr, paddr,
					buf_size, IOMMU_READ | IOMMU_WRITE);
			if (ret) {
				dev_err(dev, "iommu mapping failed\n");
				cvip_buf->dma_addr = 0;
				goto msg_err;
			}
			dev_info(dev, "heap %d iova=0x%llx, paddr=0x%llx\n",
				 buffer_type, iommu_paddr, paddr);
		}
	}

	addr_hi = cvip_buf->dma_addr >> 32;
	addr_low = cvip_buf->dma_addr;
	/* async tx channel */
	reg_message.dest = 1;
	reg_message.channel_idx = 2;
	reg_message.length = 4;
	reg_message.message_id = REGISTER_BUFFER;
	reg_message.payload[0] = addr_hi;
	reg_message.payload[1] = addr_low;
	reg_message.payload[2] = buf_size;
	reg_message.payload[3] = buffer_type;

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

static int send_nova_alive_msg(void)
{
	struct ipc_message nova_alive_msg;
	int ret;

	if (IS_ERR_OR_NULL(ipc_cvip) || IS_ERR_OR_NULL(cvip_p))
		return -EINVAL;

	nova_alive_msg.dest = 1;
	nova_alive_msg.channel_idx = 2;
	nova_alive_msg.length = 0;
	nova_alive_msg.message_id = NOVA_ALIVE;

	ret = ipc_sendmsg(ipc_cvip, &nova_alive_msg);

	return ret;
}


static int __maybe_unused install_dump_kernel(u32 dump_size)
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

int cvip_map_gpuva(void)
{
	struct device *dev;
	int ret = 0;
	phys_addr_t paddr;
	dma_addr_t iommu_paddr = 0;

	if (IS_ERR_OR_NULL(cvip_p))
		return -EINVAL;

	dev = &cvip_p->pci_dev->dev;

	paddr = page_to_phys(sg_page(cvip_p->x86_buf.buffer->sg_table->sgl));
	cvip_p->x86_buf.paddr = paddr;
	cvip_p->x86_buf.dma_addr = paddr;
	cvip_p->x86_buf.iommu_paddr = 0;
	iommu_paddr = paddr;
	if (cvip_p->domain) {
		iommu_paddr = cvip_get_iova(cvip_p,
					    &cvip_p->x86_buf,
					    CVIP_X86_BUF_SIZE);
		if (!iommu_paddr) {
			dev_err(dev, "fail to allocate iova\n");
			return -ENOMEM;
		}

		ret = iommu_map(cvip_p->domain, iommu_paddr, paddr,
				CVIP_X86_BUF_SIZE, IOMMU_READ | IOMMU_WRITE);
		if (ret) {
			dev_err(dev, "iommu mapping failed\n");
			return ret;
		}
		cvip_p->x86_buf.iommu_paddr = iommu_paddr;
	}

	return ret;
}

int cvip_set_gpuva(u64 gpuva)
{
	struct ipc_message gpuva_message;
	struct device *dev;
	int ret = 0;

	if (IS_ERR_OR_NULL(ipc_cvip) || IS_ERR_OR_NULL(cvip_p))
		return -EINVAL;

	dev = &cvip_p->pci_dev->dev;

	if (!cvip_p->x86_buf.iommu_paddr) {
		dev_err(dev, "gpu buffer is not mapped\n");
		return -EINVAL;
	}

	gpuva_message.dest = 1;
	gpuva_message.channel_idx = 2;
	gpuva_message.length = 4;
	gpuva_message.message_id = SET_GPUVA;
	gpuva_message.payload[0] = gpuva >> 32;
	gpuva_message.payload[1] = gpuva;
	gpuva_message.payload[2] = cvip_p->x86_buf.iommu_paddr >> 32;
	gpuva_message.payload[3] = cvip_p->x86_buf.iommu_paddr;

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

	if (IS_ERR_OR_NULL(cvip_p) || IS_ERR_OR_NULL(msg))
		return -EINVAL;

	cvip_p->status.boot_reason = msg->msg.p.data[0];
	cvip_p->status.wd_reboot_count = msg->msg.p.data[1];

	/* if already alive, this was a reboot but not a panic */
	if (cvip_p->status.is_alive)
		cvip_p->status.reboot_count += 1;

	cvip_p->status.is_alive = 0;

	dev = &cvip_p->pci_dev->dev;
	/*
	 * Complete the waiting of alive message
	 * and save ipc ring buffer info.
	 */
	complete(&cvip_p->completion);

	if (cvip_p->status.boot_reason == CVIP_WATCHDOG_REBOOT) {
		cvip_p->shared_buf.buf_rgd = false;
		cvip_p->mlnet_buf.buf_rgd = false;
		cvip_p->dump_buf.buf_rgd = false;
		cvip_reboot = true;
		dev_err(dev, "CVIP watchdog reboot, watchdog reboot counter:%d\n",
			cvip_p->status.wd_reboot_count);
	}

	if (!ipc_cvip)
		ipc_init(&ipc_cvip);

	/* send msg that nova is alive */
	ret = send_nova_alive_msg();
	if (ret < 0)
		dev_err(dev, "error sending nova alive message\n");

	if (!cvip_p->x86_buf.buffer) {
		buffer = cvip_alloc_buffer(cvip_p, x86);
		if (IS_ERR_OR_NULL(buffer))
			return -ENOMEM;
		ret = cvip_map_gpuva();
		if (ret) {
			dev_err(dev, "x86 gpu buffer failed to be mapped\n");
			return -ENOMEM;
		}
	}

	/* Don't unmap buffer in cvip reboot case */
	if (!cvip_p->shared_buf.buf_rgd) {
		if (!cvip_reboot) {
			if (cvip_p->shared_buf.dma_addr) {
				ret = iommu_unmap(cvip_p->domain,
						cvip_p->shared_buf.dma_addr,
						CVIP_SHARED_BUF_SIZE);
				if (ret) {
					dev_err(dev, "shared buf error\n");
					dev_err(dev, "unmapping failed\n");
				} else {
					cvip_free_iova(cvip_p,
						   cvip_p->shared_buf.dma_addr,
						   cvip_p->shared_buf.npages);
					cvip_p->shared_buf.dma_addr = 0;
					cvip_p->shared_buf.npages = 0;
				}
			}
		}
		ret = cvip_reg_buf(shared);
		if (ret)
			return ret;
		cvip_p->shared_buf.buf_rgd = true;
	}


	if (!cvip_p->mlnet_buf.buf_rgd) {
		if (!cvip_reboot) {
			if (cvip_p->mlnet_buf.dma_addr) {
				ret = iommu_unmap(cvip_p->domain,
						cvip_p->mlnet_buf.dma_addr,
						CVIP_MLNET_BUF_SIZE);
				if (ret) {
					dev_err(dev, "error: mlnet buf\n");
					dev_err(dev, "unmapping failed\n");
				} else {
					cvip_free_iova(cvip_p,
						    cvip_p->mlnet_buf.dma_addr,
						    cvip_p->mlnet_buf.npages);
					cvip_p->mlnet_buf.dma_addr = 0;
					cvip_p->mlnet_buf.npages = 0;
				}
			}
		}

		ret = cvip_reg_buf(mlnet);
		if (ret)
			return ret;
		cvip_p->mlnet_buf.buf_rgd = true;
	}

	if (cvip_p->mlnet_buf.buf_rgd && cvip_p->shared_buf.buf_rgd)
		cvip_reboot = false;

	cvip_p->status.is_alive = 1;

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

static int cvip_filldir(struct dir_context *ctx,
	const char *name,
	int namlen,
	loff_t offset,
	u64 ino,
	unsigned int d_type)
{

	struct cvip_dir_context *cctx;
	char idstr[CVIP_CORE_MAX_NLEN];
	long id_num = 0;
	uint8_t id_len;

	if (ctx == NULL) {
		pr_err(" %s: ctx was null\n", __func__);
		return -EFAULT;
	}

	if (name == NULL) {
		pr_err(" %s: name was null\n", __func__);
		return -EFAULT;
	}

	if (namlen == 0) {
		pr_err(" %s: namlen len was 0\n", __func__);
		return -EFAULT;
	}

	if (namlen >= CVIP_CORE_MAX_NLEN) {
		pr_err(" %s: namlen exceeds allowed length\n", __func__);
		return -EFAULT;
	}

	// if the file has .kcore in it, then we need to process it
	// if it doesn't, don't print or do anything; '.' and '..' also appear
	if (strstr(name, CVIP_COREDUMP_EXT) != NULL) {
		cctx = container_of(ctx, struct cvip_dir_context, ctx);
		cctx->found_files += 1;
		// ideally, the crash dumps will 'simply' numbered
		// from 0 to x (based on max-id)
		// during the transition to this new scheme however,
		// there may be older dumps in the directory
		// the id number doesn't matter as much as the total count,
		// so as long as the total number of files is below the max,
		// we will be okay.
		id_len = namlen - strlen(CVIP_COREDUMP_EXT);
		if (id_len < (sizeof(idstr)-1)) {
			memcpy(idstr, name, id_len);
			idstr[id_len] = '\0';
			if (kstrtol(idstr, 10, &id_num) != 0)
				pr_err("%s: failed to parse idstr [%s] into integer\n",
						__func__, idstr);
		} else {
			pr_err("%s: id-len [%d] exceeds buffer size [%d]\n",
					__func__, id_len, sizeof(idstr));
		}

		if (id_num > cctx->max_id)
			cctx->max_id = id_num;

		if (cctx->index < CVIP_CORE_MAX_COUNT) {
			memcpy(cctx->kcores[cctx->index], name, namlen);
			cctx->kcores[cctx->index][namlen] = '\0';
			cctx->index += 1;
		} else {
			pr_err(" %s: found more kcores then expected\n",
					__func__);
		}
	}

	return 0;
}

int cvip_handle_panic(struct ipc_buf *buf)
{
	phys_addr_t coredump_paddr;
	dma_addr_t coredump_dma;
	size_t coredump_len;
	void *coredump_vaddr;
	struct device *dev;
	struct ipc_message ack_message;
	char cored_file[64] = {0};
	int ret = 0, flag = IPC_X86_SAVE_DATA_ERR;
	static int is_new_file = 1;
	static struct file *fp;
	static loff_t off;
	struct cvip_dir_context cvip_fill;
	struct file *dir;

	dev = &cvip_p->pci_dev->dev;

	if (IS_ERR_OR_NULL(cvip_p) || IS_ERR_OR_NULL(buf) ||
	    IS_ERR_OR_NULL(ipc_cvip)) {
		ret = -EINVAL;
		goto ack;
	}

	cvip_p->status.is_alive = 0;
	cvip_p->status.reboot_count += 1;

	if (buf->buf_size == 0) {
		flag = IPC_X86_SAVE_DATA_OK;
		is_new_file = 1;
		cvip_reboot = true;
		cvip_p->status.reboot_count += 1;
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

	if (is_new_file == 1) {
		memset(&cvip_fill, 0, sizeof(cvip_fill));

		dir = filp_open(CVIP_COREDUMP, O_RDONLY, 0);
		if (IS_ERR(dir)) {
			dev_err(dev, "Failed to open dump directory for file scan\n");
			ret = -EBADF;
			goto unmap_mem;
		}
		cvip_fill.ctx.actor = cvip_filldir;

		if (iterate_dir(dir, &(cvip_fill.ctx)))
			dev_err(dev, "Failed to scan kernel dump directory for files\n");

		filp_close(dir, NULL);

		is_new_file = 0;

		// if the max indexes is less then the maximum allowed file
		// count, we increment the maximum that we have and create
		// a new file if not, we need to overwrite an existing file
		if (cvip_fill.index < CVIP_CORE_MAX_COUNT) {
			// 0 is a valid id, if we found no files, let 0 be used
			if (cvip_fill.found_files)
				cvip_fill.max_id += 1;
		} else {
			dev_dbg(dev, " at .kcore count limit, re-using [%lu]\n",
					cvip_fill.max_id);
		}

		snprintf(cored_file, 64, "%s/%lu%s", CVIP_COREDUMP,
				cvip_fill.max_id, CVIP_COREDUMP_EXT);

		fp = filp_open(cored_file, O_RDWR | O_TRUNC | O_CREAT,
			      CVIP_PANIC_FILE_PERM);
		if (IS_ERR(fp)) {
			dev_err(dev, "Failed to open coredump file\n");
			ret = -EBADF;
			goto unmap_mem;
		}
		ksys_chown(cored_file, AID_ROOT, AID_SYSTEM);
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
bool set_drm_dpms(bool state)
{
	cvip_p->drm_dpms = state;
	cvip_p->flag = true;
	wake_up(&cvip_p->wq);
	return cvip_p->drm_dpms;
}
EXPORT_SYMBOL(set_drm_dpms);

struct device_link *cvip_consumer_device_link_add(struct device *supplier_dev)
{
	return device_link_add(&cvip_p->pci_dev->dev, supplier_dev,
		DL_FLAG_STATELESS);
}
EXPORT_SYMBOL(cvip_consumer_device_link_add);

void cvip_consumer_device_link_del(struct device_link *link)
{
	if (link)
		device_link_del(link);
}
EXPORT_SYMBOL(cvip_consumer_device_link_del);

void cvip_set_power_completion(void)
{
	complete(&cvip_p->power_completion);
}

void cvip_set_allow_s2idle(bool is_allowed)
{
	cvip_allow_s2idle = is_allowed;
}

int cvip_check_power_state(struct device *dev)
{
	int ret = 0;
	struct ipc_message pm_message;

	if (pm_suspend_target_state != PM_SUSPEND_TO_IDLE)
		return 0;

	pm_message.dest = 1;
	pm_message.channel_idx = 3;
	pm_message.length = 1;
	pm_message.message_id = SEND_PM_EVENT;
	pm_message.payload[0] = CVIP_QUERY_S2IDLE;

	dev_info(dev, "Checking if CVIP is already in Doze mode\n");

	reinit_completion(&cvip_p->power_completion);

	ret = ipc_sendmsg(ipc_cvip, &pm_message);
	if (ret < 0) {
		dev_err(dev, "Failed to send PM IPC\n");
		return ret;
	}

	ret = wait_for_completion_timeout(&cvip_p->power_completion, 2 * HZ);
	if (!ret) {
		dev_err(dev, "Failed to receive response from CVIP\n");
		return -ETIMEDOUT;
	}

	if (!cvip_allow_s2idle) {
		dev_err(dev, "Cannot enter S2Idle because CVIP is in Doze\n");
		return -EPERM;
	}

	dev_info(dev, "CVIP not in Doze, x86 advancing to S2Idle\n");
	return 0;
}

static int cvip_pci_suspend(struct device *dev)
{
	struct ipc_message pm_message;
	int ret = 0;

	trace_power_timing(__func__, "entry");
	if (IS_ERR_OR_NULL(cvip_p))
		return -EINVAL;
	/* Skip in x86 single boot case */
	if (IS_ERR_OR_NULL(ipc_cvip))
		return 0;

	dev_dbg(dev, "Entering suspend\n");
	/* Enable IPC Channel 25 IRQ which is used for x86 Wakeup */
	if (cvip_p->ipc_dev.vecs[X86_IRQ_WAKE_SRC])
		irq_set_irq_wake(cvip_p->ipc_dev.vecs[X86_IRQ_WAKE_SRC], 1);

	/* sync tx channel */
	pm_message.dest = 1;
	pm_message.channel_idx = 3;
	pm_message.length = 2;
	pm_message.message_id = SEND_PM_EVENT;
	pm_message.payload[0] = LOW_POWER_MODE;

	if (pm_suspend_target_state == PM_SUSPEND_TO_IDLE) {
		dev_info(dev, "Entering PM_SUSPEND_TO_IDLE\n");
		pm_message.payload[1] = X86_IDLE;
	} else if (pm_suspend_target_state == PM_SUSPEND_STANDBY) {
		dev_info(dev, "Entering PM_SUSPEND_STANDBY\n");
		pm_message.payload[1] = X86_STANDBY;
	} else if (pm_suspend_target_state == PM_SUSPEND_MEM) {
		dev_info(dev, "Entering PM_SUSPEND_MEM\n");
		ret = wait_event_timeout(cvip_p->wq, cvip_p->flag != false,
					 msecs_to_jiffies(100));
		dev_dbg(dev, "CVIP suspend!\n");
		cvip_p->flag = false;
		if (!ret) {
			dev_err(dev, "Timeout waiting for checking display state!\n");
			return -EINVAL;
		}
		if (cvip_p->drm_dpms) {
			dev_err(dev, "CVIP can not enter s3 while display on!!\n");
			return -EINVAL;
		}
		pm_message.payload[1] = X86_S3;
	}

	trace_power_timing(__func__, "ipc_sendmsg");
	reinit_completion(&cvip_p->power_completion);
	ret = ipc_sendmsg(ipc_cvip, &pm_message);
	if (ret < 0) {
		dev_err(dev, "Failed to send suspend ipc message\n");
		return ret;
	}

	trace_power_timing(__func__, "wait_for_completion");
	ret = wait_for_completion_timeout(&cvip_p->power_completion,
		msecs_to_jiffies(6000));
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
	int ret = 0;

	trace_power_timing(__func__, "entry");
	if (IS_ERR_OR_NULL(cvip_p))
		return -EINVAL;
	/* Skip in x86 single boot case */
	if (IS_ERR_OR_NULL(ipc_cvip))
		return 0;

	dev_dbg(dev, "Resuming from suspend\n");

	ipc_mboxes_cleanup(ipc_cvip, IPC_CHAN);

	/* Hotplug add the VCAM */
	trace_power_timing(__func__, "cvip_power_hot_plug_request");
	ret = cvip_power_hot_plug_request(true);
	if (ret < 0)
		dev_err(dev, "Failed to hotplug add VCAM\n");
	trace_power_timing(__func__, "cvip_power_hot_plug_request done");

	/* reset the ipc irq affinity after S3 */
	if (pm_suspend_target_state == PM_SUSPEND_MEM)
		ipc_irq_affinity_restore(&cvip_p->ipc_dev);

	trace_power_timing(__func__, "mailbox cleanup done");
	/* Disable IPC Channel 25 IRQ which is used for x86 Wakeup */
	if (cvip_p->ipc_dev.vecs[X86_IRQ_WAKE_SRC])
		irq_set_irq_wake(cvip_p->ipc_dev.vecs[X86_IRQ_WAKE_SRC], 0);

	trace_power_timing(__func__, "set_irq done");

	if (pm_suspend_target_state == PM_SUSPEND_MEM) {
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
	}

	/* sync tx channel */
	pm_message.dest = 1;
	pm_message.channel_idx = 3;
	pm_message.length = 2;
	pm_message.message_id = SEND_PM_EVENT;
	pm_message.payload[0] = LOW_POWER_MODE;
	pm_message.payload[1] = X86_S0;

	reinit_completion(&cvip_p->power_completion);

	trace_power_timing(__func__, "ipc_sendmsg");
	ret = ipc_sendmsg(ipc_cvip, &pm_message);
	if (ret < 0) {
		dev_err(dev, "Failed to send ipc message after resuming\n");
		return ret;
	}

	trace_power_timing(__func__, "wait_for_completion");
	ret = wait_for_completion_timeout(&cvip_p->power_completion,
		msecs_to_jiffies(6000));
	if (!ret) {
		dev_err(dev, "Failed to receive low power ack from CVIP\n");
		return -EINVAL;
	}
	trace_power_timing(__func__, "wait_for_completion done");

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
			cvip_p->status.reboot_count += 1;
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

static ssize_t is_alive_show(struct device *dev,
			       struct device_attribute *attr,
			       char *buf)
{
	if (IS_ERR_OR_NULL(cvip_p))
		return -EINVAL;

	return sprintf(buf, "%d\n", cvip_p->status.is_alive);
}

static ssize_t wdt_count_show(struct device *dev,
			       struct device_attribute *attr,
			       char *buf)
{
	if (IS_ERR_OR_NULL(cvip_p))
		return -EINVAL;

	return sprintf(buf, "%d\n", cvip_p->status.wdt_count);
}


static ssize_t reboot_count_show(struct device *dev,
			       struct device_attribute *attr,
			       char *buf)
{
	if (IS_ERR_OR_NULL(cvip_p))
		return -EINVAL;

	return sprintf(buf, "%d\n", cvip_p->status.reboot_count);
}

static ssize_t panic_count_show(struct device *dev,
			       struct device_attribute *attr,
			       char *buf)
{
	return sprintf(buf, "%d\n", cvip_p->status.panic_count);
}

static ssize_t boot_reason_show(struct device *dev,
			       struct device_attribute *attr,
			       char *buf)
{
	if (IS_ERR_OR_NULL(cvip_p))
		return -EINVAL;

	return sprintf(buf, "%d\n", cvip_p->status.boot_reason);
}

static ssize_t wd_reboot_count_show(struct device *dev,
			       struct device_attribute *attr,
			       char *buf)
{
	return sprintf(buf, "%d\n", cvip_p->status.wd_reboot_count);
}



static DEVICE_ATTR_RW(ipc_reboot);
static DEVICE_ATTR_RW(ipc_s2ram);
static DEVICE_ATTR_RO(wdt_count);
static DEVICE_ATTR_RO(is_alive);
static DEVICE_ATTR_RO(reboot_count);
static DEVICE_ATTR_RO(panic_count);
static DEVICE_ATTR_RO(boot_reason);
static DEVICE_ATTR_RO(wd_reboot_count);

struct attribute *sysfs_attrs[] = {
	&dev_attr_ipc_reboot.attr,
	&dev_attr_ipc_s2ram.attr,
	&dev_attr_wdt_count.attr,
	&dev_attr_is_alive.attr,
	&dev_attr_reboot_count.attr,
	&dev_attr_panic_count.attr,
	&dev_attr_boot_reason.attr,
	&dev_attr_wd_reboot_count.attr,
	NULL,
};

struct attribute_group sysfs_attr_group = {
	.attrs = sysfs_attrs,
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
	unsigned int npages = 0;
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
	dev_err(dev, "allocate iova:0x%llx, bufsize:%llx, npages:%lld\n",
		address, dma_buf->size, npages);

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

static int cvip_s_intf_mmap(struct file *file, struct vm_area_struct *vma)
{
	int ret;
	__u32 vma_length;


	vma_length = vma->vm_end - vma->vm_start;

	ret = pci_mmap_resource_range(cvip_p->pci_dev, CVIP_S_INTF_BAR,
			      vma, pci_mmap_mem, 0);

	if (ret < 0) {
		pr_err("pci mmap failed with err %d\n", ret);
		return ret;
	}

	return 0;
}


static int cvip_base_mmap(struct file *file, struct vm_area_struct *vma)
{
	int ret;
	__u32 vma_length;
	uint64_t phys_addr;


	vma_length = vma->vm_end - vma->vm_start;

	if (!vma->vm_pgoff) {
		pr_err("vm_pgoff shouldn't be 0");
		return -EINVAL;
	}

	phys_addr = (vma->vm_pgoff * PAGE_SIZE) -
			  MERO_CVIP_XPORT_APERTURE_SIZE;

	ret = remap_pfn_range(vma, vma->vm_start, phys_addr >> PAGE_SHIFT,
			      vma_length, vma->vm_page_prot);

	if (ret < 0) {
		pr_err("mmap failed with err %d\n", ret);
		return ret;
	}

	return 0;
}

static const struct file_operations cvip_s_intf_fops = {
	.owner = THIS_MODULE,
	.mmap = cvip_s_intf_mmap,
};

static struct miscdevice cvip_s_intf_dev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = CDEV_SPORT_NAME,
	.fops = &cvip_s_intf_fops,
};

static const struct file_operations cvip_fops = {
	.owner = THIS_MODULE,
	.mmap = cvip_base_mmap,
};

static struct miscdevice cvip_base_dev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = CDEV_NAME,
	.fops = &cvip_fops,
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

static int cvip_status_open(struct inode *inode, struct file *file)
{
	file->private_data = cvip_p;

	return 0;
}

static int cvip_status_release(struct inode *inode, struct file *file)
{
	return 0;
}

void cvip_status_get_status(struct cvip_status *stats)
{
	memcpy(stats, &cvip_p->status, sizeof(struct cvip_status));
}
EXPORT_SYMBOL(cvip_status_get_status);

static long cvip_status_ioctl(struct file *file, unsigned int cmd,
			      unsigned long arg)
{
	struct cvip_dev *cvip;
	struct cvip_status status;
	struct device *dev;
	long ret = 0;

	cvip = (struct cvip_dev *)file->private_data;
	dev = &cvip->pci_dev->dev;

	switch (cmd) {
	case CVIP_STATUS_IOCTL_GET_STATUS:
		cvip_status_get_status(&status);
		ret = copy_to_user((void __user *)arg, &status, sizeof(status));
		if (ret) {
			dev_err(dev, "%s: copy to user failed, %d\n",
				__func__, ret);
			return ret;
		}
		break;
	default:
		return -EINVAL;
	}
	return ret;
}

static struct file_operations const cvip_status_fops = {
	.owner = THIS_MODULE,
	.open = cvip_status_open,
	.unlocked_ioctl = cvip_status_ioctl,
	.release = cvip_status_release,
};

static struct miscdevice cvip_status_dev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = CDEV_STATUS_NAME,
	.fops = &cvip_status_fops,
};

static int cvip_add_device_node(struct cvip_dev *cvip_dev)
{
	int ret;
	struct device *dev;

	dev = cvip_dev->ipc_dev.dev;

	ret = misc_register(&cvip_base_dev);
	if (ret) {
		dev_err(dev, "misc_register failed, err = %d\n", ret);
		return ret;
	}

	dev_info(dev, "Device node %s has initialized successfully.\n",
			CDEV_NAME);

	ret = misc_register(&cvip_s_intf_dev);
	if (ret) {
		dev_err(dev, "misc_register failed, err = %d\n", ret);
		return ret;
	}

	dev_info(dev, "Device node %s has initialized successfully.\n",
			CDEV_SPORT_NAME);

	ret = misc_register(&cvip_xport_dev);
	if (ret) {
		dev_err(dev, "misc_register failed for cvipx, err = %d\n", ret);
		return ret;
	}

	dev_info(dev, "Device node %s has initialized successfully.\n",
			CDEV_XPORT_NAME);

	ret = misc_register(&cvip_status_dev);
	if (ret) {
		dev_err(dev, "misc_register failed for cvip_status, err = %d\n",
			ret);
		return ret;
	}
	memset(&(cvip_dev->status), 0, sizeof(struct cvip_status));

	dev_info(dev, "Device node %s has initialized successfully.\n",
			 CDEV_STATUS_NAME);

	return 0;
}

static void cvip_remove_node(struct cvip_dev *cvip_dev)
{
	misc_deregister(&cvip_status_dev);
	misc_deregister(&cvip_s_intf_dev);
	misc_deregister(&cvip_base_dev);
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
	cvip_p->status.panic_count = 0;
	cvip_p->flag = false;
	init_waitqueue_head(&cvip_p->wq);
	cvip_p->drm_dpms = false;
	cvip->pci_dev = pci_dev;

	atomic_notifier_chain_register(&cvip_wdt_notifier_list,
				       &cvip_wdt_nb);
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

	ret = cvip_add_device_node(cvip);
	if (ret)
		goto init_err;

	ret = sysfs_create_group(&cvip_status_dev.this_device->kobj,
				 &sysfs_attr_group);
	if (ret) {
		dev_err(dev, "Failed create sysfs group\n");
		goto init_err;
	}

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

	sysfs_remove_group(&cvip_status_dev.this_device->kobj,
						&sysfs_attr_group);
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
