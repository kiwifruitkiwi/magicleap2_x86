// SPDX-License-Identifier: GPL-2.0-only
/*
 * mlnet virtual network driver for the CVIP SOC.
 *
 * Copyright (C) 2023 Magic Leap, Inc. All rights reserved.
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/skbuff.h>
#include <linux/ethtool.h>
#include <linux/string.h>
#include <linux/circ_buf.h>
#include <linux/kthread.h>
#include <linux/dma-buf.h>
#include <linux/completion.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/version.h>
#include <asm/barrier.h>
#include "shregion_common.h"
#include "shregion_types.h"
#include "mero_xpc_kapi.h"
#include "cvcore_xchannel_map.h"
#include "cvcore_processor_common.h"
#include "gsm.h"
#include "gsm_cvip.h"
#include "mlnet.h"
#include "gsm_jrtc.h"

#if defined(CONFIG_ML_CVIP_ARM)
#include <linux/reboot.h>
#include <linux/delay.h>
#include "ion/ion.h"
#include "uapi/mero_ion.h"
#include "linux/cvip/cvip_plt/cvip_plt.h"
#endif

#define DPRINT(level, ...) do { if (dbg_level >= level) pr_info(__VA_ARGS__); } while (0)
#define DHPRINT(level, ...) do { if (dbg_level >= level) print_hex_dump(__VA_ARGS__); } while (0)

// Main driver major & minor versions.
// Minor version changes should be backward compatible
#define MLNET_MODULE_MAJOR_VERSION "4"
#define MLNET_MODULE_MINOR_VERSION "0"

#define POLL_MODE_CVIP_TO_NOVA_NIBBLE_OFFSET (0)
#define POLL_MODE_NOVA_TO_CVIP_NIBBLE_OFFSET  (1)

#define GSM_REGION_MASK ((0x1U << GSM_REGION_SHIFT) - 1)

#define mlnet_gsm_nibble_atomic_test_and_set(offset, nibble)                            \
	(*(volatile uint32_t *)((uintptr_t)gsm_backing +                                    \
		(((GSMI_READ_COMMAND_NIBBLE_TEST_AND_SET_BASE + nibble) << GSM_REGION_SHIFT) |  \
		((uintptr_t)(offset)&GSM_REGION_MASK))))

#define mlnet_gsm_nibble_write(offset, nibble, val)                                     \
	((*(volatile uint32_t *)((uintptr_t)gsm_backing +                                   \
		(((GSMI_WRITE_COMMAND_NIBBLE_WRITE_BASE + nibble) << GSM_REGION_SHIFT) |        \
		((uintptr_t)(offset)&GSM_REGION_MASK)))) = (val))

#define mlnet_gsm_nibble_read(offset, nibble)                                           \
	(((*(volatile uint32_t *)((uintptr_t)gsm_backing +                                  \
		(((GSMI_READ_COMMAND_RAW) << GSM_REGION_SHIFT) |                                \
		((uintptr_t)(offset)&GSM_REGION_MASK)))) >> (nibble * 4)) & 0xF)

/* TODO(ggallagher): these should be in a private struct */
static uint32_t mem_ready;
static uint32_t dbg_level;
static uint64_t schd_cnt;
static uint64_t xmit_cnt;
static uint32_t cvip_online;
static uint32_t x86_online;
static uint32_t x86_ready;
static uint32_t cvip_ready;
static uint32_t wait_count;
static uint32_t xport_wait_count;
static char    *ver_str = MLNET_MODULE_MAJOR_VERSION "." MLNET_MODULE_MINOR_VERSION;
static uint32_t rx_intr_counter;
static uint32_t tx_intr_counter;
static uint32_t hard_stop;
static uint32_t soft_stop;
static uint32_t notif_send_error;
static uint32_t nbd_server_ready;
static uint32_t nfs_server_ready;
static uint32_t packet_test;
static uint32_t mlnet_setup_complete;
static struct task_struct *alloc_thread;
static uint32_t last_intr_send_count;
static uint64_t mem_cnt_to_arm;
static uint64_t mem_cnt_to_x86;

struct mlnet_kpi_stats {
	uint64_t jrtc_start_offset_us;
	uint64_t nova_alive_time_us;
	uint64_t mem_ready_time_us;
	uint64_t ready_start_time_us;
        uint64_t ready_time_us;
};
static struct mlnet_kpi_stats kpi;

#define MAX_INTR_SEND_COUNT	(32)

#if defined(CONFIG_ML_CVIP_ARM)
static struct completion xport_ready;
static struct completion nova_alive;
#define RETRY_TIMEOUT		200000
#define MAX_MEM_TIMEOUT		(msecs_to_jiffies(2000))
#define MAX_NOVA_TIMEOUT	(msecs_to_jiffies(500))
#define RETRY_COUNT		6
static const char mac_address[ETH_ALEN] = { 0x02, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa };
#else
#define MAX_MEM_TIMEOUT_CVIP		(msecs_to_jiffies(200000))
static const char mac_address[ETH_ALEN] = { 0x02, 0x86, 0x86, 0x86, 0x86, 0x86 };
#endif

// Link States
enum mlnet_link_state {
	MLNET_LINK_INIT,
	MLNET_LINK_UP,
	MLNET_LINK_DOWN
};

// Mailbox locations for ARM and X86
enum mlnet_mbox_index {
	MLNET_ARM_INDEX = 0,
	MLNET_X86_INDEX
};

enum ml_devices {
	CVIP = 0,
	NOVA
};

enum mlnet_msg {
	MLNET_CVIP_UP = 0,
	MLNET_X86_UP,
	MLNET_CVIP_DOWN,
	MLNET_X86_DOWN,
	MLNET_NBD_SERVER_READY,
	MLNET_NFS_SERVER_READY,
	MLNET_INT,
	MLNET_CVIP_MEM_RDY,
	MLNET_X86_MEM_RDY,
	MLNET_CVIP_RDY,
	MLNET_X86_RDY,
	MLNET_MLNET_SETUP_COMPLETE,
};

/* max packet size should be 64k */
struct data_buffer {
	struct device *dev;
	int res_len;
	int fd;
	struct dma_buf *dmabuf;
	struct dma_buf_attachment *dma_attach;
	struct sg_table *sg_table;
	void *vaddr;
	phys_addr_t paddr;
	int  datalen;
};

/*
 * A structure representing an in-flight packet.
 */
struct ml_packet {
	struct  net_device *dev;
	struct  data_buffer _dpkt;
	uint8_t *data;
};

struct meta_data {
	uint32_t shared_version;
	uint32_t local_version;
	uint8_t nova_qlen;
	uint8_t nova_head;
	uint8_t nova_tail;
	uint8_t cvip_qlen;
	uint8_t cvip_head;
	uint8_t cvip_tail;
	uint32_t data_len_cvip[MLNET_MBOX_SLOTS];
	uint32_t data_len_nova[MLNET_MBOX_SLOTS];
};

struct ml_buffer {
	spinlock_t rx_lock;
	spinlock_t tx_lock;
	struct ml_packet buffer[MLNET_MBOX_SLOTS];
};

struct ml_dev_buffers {
	struct ml_buffer cvip;  /* List of incoming packets */
	struct ml_buffer nova;
};

int pool_size = MLNET_MBOX_SLOTS;
/*
 * This structure is private to each device. It is used to pass
 * packets in and out, so there is place for a packet
 */
struct ml_priv {
	struct net_device *netdev;
	struct net_device_stats stats;
	struct napi_struct napi;
	enum mlnet_link_state link_state;
	struct ml_dev_buffers data;
	struct completion mem_ready;
	atomic_t mlnet_online;
        atomic_t s2_idle;
};

struct meta_data *mdata;
struct ml_priv priv;

static struct mlnet_dev {
	struct device *dev;
	struct net_device *netdev;
	struct ml_priv *priv;
} mdev;

enum {
	CVIP_ONLINE = 0,
	X86_ONLINE,
	NBD_SERVER,
	NFS_SERVER,
	NOTIFY_CNT
};
static struct kernfs_node *mlnet_sysfs_dirent[NOTIFY_CNT];
static const char *const mlnet_notify_names[NOTIFY_CNT] = {
	"cvip_online",
	"x86_online",
	"nbd_server_ready",
	"nfs_server_ready",
};


static int mlnet_register_xpc_handlers(void);
static int mlnet_deregister_xpc_handlers(void);

extern int pl330_share_dma_device(struct device *sh_dev);

#if defined(CONFIG_ML_CVIP_ARM)
static int mlnet_shutdown(struct notifier_block *nb, unsigned long code, void *unused)
{
	pr_err("mlnet shutting down....\n");
	if (mdev.netdev) {
		if (mdev.netdev->reg_state == NETREG_REGISTERED)
			unregister_netdev(mdev.netdev);
	}
	while (1) {
		/* block mlnet from doing anything
		 * kernel will shutdown anyway, this
		 * stops the mem abort
		 */
		msleep(100);
	}

	return 0;
}

static struct notifier_block mlnet_notifier = {
	.notifier_call = mlnet_shutdown,
};
#endif

static int begin_cpu_access_to_dev(struct ml_packet *pkt)
{
	int rc = 0;

#if defined(CONFIG_ML_CVIP_ARM)
	rc = dma_buf_begin_cpu_access_no_lock(pkt->_dpkt.dmabuf, DMA_TO_DEVICE);
	if (rc) {
		netdev_err(mdev.netdev, "start cpu access failed %d\n", rc);
		return rc;
	}
#endif
	return rc;
}

static int end_cpu_access_to_dev(struct ml_packet *pkt)
{
	int rc = 0;

#if defined (CONFIG_ML_CVIP_ARM)
	rc = dma_buf_end_cpu_access_no_lock(pkt->_dpkt.dmabuf, DMA_TO_DEVICE);
	if (rc) {
		netdev_err(mdev.netdev, "start cpu access failed %d\n", rc);
		return rc;
	}
#endif
	return rc;
}


static int begin_cpu_access_from_dev(struct ml_packet *pkt)
{
	int rc = 0;
#if defined (CONFIG_ML_CVIP_ARM)
	rc = dma_buf_begin_cpu_access_no_lock(pkt->_dpkt.dmabuf, DMA_FROM_DEVICE);
	if (rc) {
		netdev_err(mdev.netdev, "start cpu access failed %d\n", rc);
		return rc;
	}
#endif
	return rc;
}

static int end_cpu_access_from_dev(struct ml_packet *pkt)
{
	int rc = 0;
#if defined (CONFIG_ML_CVIP_ARM)
	rc = dma_buf_end_cpu_access_no_lock(pkt->_dpkt.dmabuf, DMA_FROM_DEVICE);
	if (rc) {
		netdev_err(mdev.netdev, "start cpu access failed %d\n", rc);
		return rc;
	}
#endif
	return rc;
}

#if defined(CONFIG_ML_CVIP_ARM)

extern int get_xport_status(uint32_t type);
extern int get_nova_status(void);
static struct data_buffer meta_buf;
/*
 * Only call complete if the heap that's ready is the heap for mlnet.
 * Some xport heaps can be ready before the mlnet heap, if we call
 * complete when the first xport heap is ready that doesn't mean the mlnet heap
 * is also ready.  Ensure we have the right heap ready before calling
 * complete().
 */
static int xport_created_notify(struct notifier_block *self,
				unsigned long v, void *p)
{
	int ret;

	ret = get_xport_status(MLNET_HEAP);
	if (ret)
		complete(&xport_ready);

	return 0;
}

static int s2_idle_notify(struct notifier_block *self,
				unsigned long v, void *p)
{
	atomic_set(&mdev.priv->s2_idle, 1);

	return 0;
}

static int resume_notify(struct notifier_block *self,
				unsigned long v, void *p)
{
	atomic_set(&mdev.priv->s2_idle, 0);

	mdata->cvip_tail = 0;
	mdata->cvip_head = 0;
	mdata->nova_tail = 0;
	mdata->nova_head = 0;

	return 0;
}

static int nova_alive_notify(struct notifier_block *self,
				   unsigned long v, void *p)
{
	complete(&nova_alive);
	return 0;
}

static struct notifier_block xport_notifier = {
	.notifier_call = xport_created_notify,
};

static struct notifier_block s2_idle_notifier = {
	.notifier_call = s2_idle_notify,
};

static struct notifier_block resume_notifier = {
	.notifier_call = resume_notify,
};

static struct notifier_block nova_alive_notifier = {
	.notifier_call = nova_alive_notify,
};

static int register_xport_notify(void)
{
	atomic_notifier_chain_register(&xport_ready_notifier_list,
				       &xport_notifier);
	return 0;
}

static int register_s2_resume_notifiers(void)
{
	atomic_notifier_chain_register(&s2_idle_notifier_list,
				       &s2_idle_notifier);

        atomic_notifier_chain_register(&resume_notifier_list,
				       &resume_notifier);
        return 0;
}

static int register_nova_alive_notify(void)
{
	atomic_notifier_chain_register(&nova_event_notifier_list,
				       &nova_alive_notifier);
	return 0;
}

static int alloc_shared(struct data_buffer *mb, size_t size, unsigned int flags)
{
	int fd;
	struct ion_buffer *ion_buf = NULL;
	phys_addr_t phys_addr;

	/*
	 * since there is no IOMMU device on the x-port interface, this is
	 * a hack to share the dma330 IOMMU device, just to use the proper
	 * dma_buf ops when performing dma_buf_attach and
	 * dma_buf_map_attachment
	 */

	mb->dev = mdev.dev;
	if (pl330_share_dma_device(mb->dev))
		return -ENOMEM;

	mb->fd = -1;
	fd = ion_alloc(size, 1 << ION_HEAP_X86_CVIP_MLNET_ID, flags);
	if (fd < 0) {
		dev_err(mb->dev, "Fail to allocate ION xport shared mem\n");
		return -ENOMEM;
	}

	mb->fd = fd;
	mb->res_len = MLNET_MAX_MTU;
	mb->dmabuf = NULL;
	mb->dma_attach = NULL;
	mb->sg_table = NULL;

	mb->dmabuf = dma_buf_get(fd);
	if (IS_ERR(mb->dmabuf)) {
		dev_err(mb->dev, "Fail to get dmabuf from fd:%d\n", fd);
		mb->dmabuf = NULL;
		return -EINVAL;
	}

	mb->dma_attach = dma_buf_attach(mb->dmabuf, mb->dev);
	if (IS_ERR(mb->dma_attach)) {
		dev_err(mb->dev, "Fail to attach to dmabuf\n");
		mb->dma_attach = NULL;
		return -EINVAL;
	}

	// presume all 64 bits are dma capable.
	mb->dev->coherent_dma_mask = DMA_BIT_MASK(64);
	mb->dev->dma_mask = &mb->dev->coherent_dma_mask;

	mb->sg_table = dma_buf_map_attachment(mb->dma_attach, DMA_BIDIRECTIONAL);
	if (IS_ERR(mb->sg_table)) {
		dev_err(mb->dev, "Fail to get sgtable for dmabuf\n");
		mb->sg_table = NULL;
		return -EINVAL;
	}

	ion_buf = mb->dmabuf->priv;
	phys_addr = page_to_phys(sg_page(ion_buf->sg_table->sgl));

	mb->paddr = phys_addr;

	mb->vaddr = dma_buf_kmap(mb->dmabuf, 0);

	return 0;
}

static int alloc_meta_data(struct mlnet_dev *dev)
{
	int err = 0;

	err = alloc_shared(&meta_buf, PAGE_SIZE, 0);
	if (err) {
		DPRINT(0, "Error allocating meta data\n");
	} else {
		mdata = (struct meta_data *)meta_buf.vaddr;
		DPRINT(0, "meta mlnet phys addr info %llx\n", (uint64_t)meta_buf.paddr);
		DPRINT(4, "meta mlnet virt addr info %llx\n", (uint64_t)meta_buf.vaddr);
		DPRINT(4, "meta info virt addr  %llx\n", (uint64_t)(mdata));
	}
	return err;
}

static int alloc_packet(struct ml_packet *pkt, int index, uint32_t queue)
{
	int status = 0;

	status = alloc_shared(&pkt->_dpkt, MLNET_MAX_MTU, ION_FLAG_CACHED);
	pkt->data = (uint8_t *)(pkt->_dpkt.vaddr);

	return status;
}

#else

/* memory layout for ion allocation is shregions meta data block and then the
 * core dump block, mlnet follows and then the rest is memory allocated by
 * shregions.
 */
extern void *mlnet_shared_ddr_base_addr;
static void *mlnet_start_ddr_addr;
static void *mdata_ptr;
static void *cvip_ptr;
static void *nova_ptr;
static bool x86_mem_init;

static int memory_init(void)
{

	DPRINT(0, "mlnet shared x86 address %llx\n", (uint64_t)mlnet_shared_ddr_base_addr);
	mlnet_start_ddr_addr = (void *)mlnet_shared_ddr_base_addr;

	DPRINT(0, "mlnet shared x86 address phys %llx\n",
			(uint64_t)virt_to_phys(mlnet_start_ddr_addr));

	/* get info struct 4K */
	mdata_ptr = mlnet_start_ddr_addr;

	/* setup the packet pool 16 packets, 64k each */
	cvip_ptr = mlnet_start_ddr_addr + (uint64_t)(MLNET_META_SZ);
	nova_ptr = cvip_ptr + (uint64_t)(QUEUE_SZ);

	x86_mem_init = true;
	return 0;
}

static int alloc_meta_data(struct mlnet_dev *dev)
{
	int ret = 0;

	if (x86_mem_init == false)
		ret = memory_init();

	mdata = (struct meta_data *)(mdata_ptr);

	return ret;
}

static int alloc_packet(struct ml_packet *pkt, int index, uint32_t queue)
{
	void *queue_ptr = NULL;
	int ret = 0;

	if (x86_mem_init == false)
		memory_init();

	switch (queue) {
	case CVIP:
		queue_ptr = cvip_ptr;
		break;
	case NOVA:
		queue_ptr = nova_ptr;
		break;
	default:
		DPRINT(1, "mlnet unknown queue given\n");
		ret = -1;
		break;
	}
	pkt->_dpkt.vaddr = queue_ptr + (index * MLNET_MAX_MTU);
	pkt->data = (uint8_t *)(pkt->_dpkt.vaddr);

	return ret;
}

#endif

void free_data_pkt(struct ml_packet *pkt)
{
}

int mlnet_setup_queue(struct net_device *dev, struct ml_buffer *mbuffer, int size, uint32_t type)
{
	int i, ret;

	for (i = 0; i < size; i++) {
		mbuffer->buffer[i].dev = dev;
		ret = alloc_packet(&mbuffer->buffer[i], i, type);
		if (ret) {
			DPRINT(0, "error allocating packet data\n");
			return -1;
		}
	}

	return 0;
}

int mlnet_alloc_queues(struct net_device *dev)
{
	int ret;

	/* check for failure */
	ret = mlnet_setup_queue(mdev.netdev, &mdev.priv->data.cvip, MLNET_MBOX_SLOTS, CVIP);
	ret = mlnet_setup_queue(mdev.netdev, &mdev.priv->data.nova, MLNET_MBOX_SLOTS, NOVA);

	spin_lock_init(&mdev.priv->data.cvip.tx_lock);
	spin_lock_init(&mdev.priv->data.cvip.rx_lock);

	spin_lock_init(&mdev.priv->data.nova.tx_lock);
	spin_lock_init(&mdev.priv->data.nova.rx_lock);

	mdata->cvip_qlen = MLNET_MBOX_SLOTS;
	mdata->nova_qlen = MLNET_MBOX_SLOTS;

	mdata->cvip_tail = 0;
	mdata->cvip_head = 0;
	mdata->nova_tail = 0;
	mdata->nova_head = 0;

	return ret;
}

void mailbox_check(void);
//--------------------------------------------------------------------------------------
// mlnet_notification_handler    XPC Notification Handler
// This will handle a basic message to the x86 that the ARM is ready
//--------------------------------------------------------------------------------------
static int mlnet_notification_handler(XpcChannel channel, XpcHandlerArgs args,
	       uint8_t *notification_buffer, size_t notification_buffer_length)
{
	uint32_t msg;

	DPRINT(2, "[%s] received xpc notification interrupt\n", __func__);

	if (channel != CVCORE_XCHANNEL_MLNET) {
		pr_err("[%s] received notification on incorrect vchannel: %d.\n",
				__func__, channel);
		return 0;
	}

	rx_intr_counter++;
	msg = *((uint32_t *)notification_buffer);

	/* GG: Eventually this will handle the virtual interrupt but for now
	 * it's only handling a message that the CVIP or x86 is ready.
	 */
	switch (msg) {
	case MLNET_CVIP_RDY:
		pr_err("mlnet : CVIP driver has been probed\n");
		cvip_ready = 1;
		break;
	case MLNET_X86_RDY:
		pr_err("mlnet: x86 driver has been probed\n");
		x86_ready = 1;
		break;
	case MLNET_CVIP_UP:
		cvip_online = 1;
		sysfs_notify_dirent(mlnet_sysfs_dirent[CVIP_ONLINE]);
		break;
	case MLNET_X86_UP:
		x86_online = 1;
		sysfs_notify_dirent(mlnet_sysfs_dirent[X86_ONLINE]);
		break;
	case MLNET_CVIP_DOWN:
		cvip_online = 0;
		sysfs_notify_dirent(mlnet_sysfs_dirent[CVIP_ONLINE]);
		break;
	case MLNET_X86_DOWN:
		x86_online = 0;
		sysfs_notify_dirent(mlnet_sysfs_dirent[X86_ONLINE]);
		break;
	case MLNET_NBD_SERVER_READY:
		nbd_server_ready = 1;
		break;
	case MLNET_NFS_SERVER_READY:
		nfs_server_ready = 1;
		break;
	case MLNET_INT:
		mailbox_check();
		break;
	case MLNET_CVIP_MEM_RDY:
	case MLNET_X86_MEM_RDY:
		pr_info("mlnet hit mem_ready\n");
		complete(&mdev.priv->mem_ready);
		break;
	case MLNET_MLNET_SETUP_COMPLETE:
		mlnet_setup_complete = 1;
	default:
		break;
	}

	return 0;
}

//--------------------------------------------------------------------------------------
// mlnet_send_notification    Send Notification
//--------------------------------------------------------------------------------------
static void mlnet_send_notification(uint32_t msg)
{
	int status;
	int xpc_target_mask;
#if defined(CONFIG_ML_CVIP_ARM)
	int idle;

	idle = atomic_read(&mdev.priv->s2_idle);
	if (idle)
		return;

	xpc_target_mask = (1 << CORE_ID_X86);
#else
	xpc_target_mask = (1 << CORE_ID_A55);
#endif

	status = xpc_notification_send(CVCORE_XCHANNEL_MLNET,
			xpc_target_mask,
			(uint8_t *)&msg,
			sizeof(uint32_t),
			XPC_NOTIFICATION_MODE_POSTED);

	if (status != 0)
		notif_send_error++;
	else
		tx_intr_counter++;
}

//--------------------------------------------------------------------------------------
// mlnet_register_xpc_handlers    Register xpc handlers
//--------------------------------------------------------------------------------------
static int mlnet_register_xpc_handlers(void)
{
	int ret;
	int msg;

	// Register notifier notification handler with xpc
	ret = xpc_register_notification_handler_hi(CVCORE_XCHANNEL_MLNET,
			&mlnet_notification_handler, NULL);
	if (ret != 0) {
		pr_err("[%s] failed to register xpc notification handler: %d\n", __func__, ret);
		return -1;
	}

	#if defined(CONFIG_ML_CVIP_ARM)
		msg = MLNET_CVIP_RDY;
		cvip_ready = 1;
	#else
		msg = MLNET_X86_RDY;
		x86_ready = 1;
	#endif
	mlnet_send_notification(msg);
	return 0;
}

//--------------------------------------------------------------------------------------
// mlnet_deregister_xpc_handlers    Deregister xpc handlers
//--------------------------------------------------------------------------------------
static int mlnet_deregister_xpc_handlers(void)
{
	int ret;

	// Deregister notifier notification handler with xpc
	ret = xpc_register_notification_handler_hi(CVCORE_XCHANNEL_MLNET, NULL, NULL);
	if (ret != 0) {
		pr_err("[%s] failed to deregister xpc notification handler: %d\n", __func__, ret);
		return -1;
	}

	return 0;
}

static inline void queue_inc_head(uint8_t head, uint32_t device)
{
	switch (device) {
	case CVIP:
		smp_store_release(&mdata->cvip_head, (head + 1) & (MLNET_MBOX_SLOTS - 1));
		break;
	case NOVA:
		smp_store_release(&mdata->nova_head, (head + 1) & (MLNET_MBOX_SLOTS - 1));
		break;
	default:
		DPRINT(1, "unknown device given\n");
		break;
	}
}

static inline uint8_t queue_inc_tail(uint32_t device, uint8_t tail)
{
	switch (device) {
	default:
	case CVIP:
		smp_store_release(&mdata->cvip_tail, (tail + 1) & (MLNET_MBOX_SLOTS - 1));
		break;
	case NOVA:
		smp_store_release(&mdata->nova_tail, (tail + 1) & (MLNET_MBOX_SLOTS - 1));
		break;
	}

	return tail;
}

static inline int queue_has_space(uint32_t device)
{
	int ret = 0;

	switch (device) {
	case CVIP:
		ret = CIRC_SPACE(mdata->cvip_head, mdata->cvip_tail, MLNET_MBOX_SLOTS);
		break;
	case NOVA:
		ret = CIRC_SPACE(mdata->nova_head, mdata->nova_tail, MLNET_MBOX_SLOTS);
		break;
	default:
		DPRINT(1, "unknown device given\n");
		break;
	}

	return ret;
}

static inline unsigned int queue_count(uint32_t device)
{
	int ret = 0;

	switch (device) {
	case CVIP:
		ret = CIRC_CNT_TO_END(mdata->cvip_head, mdata->cvip_tail, MLNET_MBOX_SLOTS);
		break;
	case NOVA:
		ret = CIRC_CNT_TO_END(mdata->nova_head, mdata->nova_tail, MLNET_MBOX_SLOTS);
		break;
	default:
		DPRINT(1, "unknown device given\n");
		break;
	}

	return ret;
}

static inline uint8_t queue_acquire_head(uint32_t device)
{
	uint8_t head = 0;

	switch (device) {
	default:
	case CVIP:
		head = smp_load_acquire(&mdata->cvip_head);
		break;
	case NOVA:
		head = smp_load_acquire(&mdata->nova_head);
		break;
	}

	return head;
}

static inline uint8_t queue_acquire_tail(uint32_t device)
{
	uint8_t tail = 0;

	switch (device) {
	default:
	case CVIP:
		tail = READ_ONCE(mdata->cvip_tail);
		break;
	case NOVA:
		tail = READ_ONCE(mdata->nova_tail);
		break;
	}

	return tail;
}


static inline uint8_t queue_get_head(uint32_t device)
{
	uint8_t head = 0;

	switch (device) {
	default:
	case CVIP:
		head = mdata->cvip_head;
		break;
	case NOVA:
		head = mdata->nova_head;
		break;
	}

	return head;
}


static inline uint8_t queue_get_tail(uint32_t device)
{
	uint8_t tail = 0;

	switch (device) {
	default:
	case CVIP:
		tail = mdata->cvip_tail;
		break;
	case NOVA:
		tail = mdata->nova_tail;
		break;
	}

	return tail;
}

//--------------------------------------------------------------------------------------
// mlnet_free_shared_mem()    Free shared memory
//--------------------------------------------------------------------------------------
static int mlnet_free_shared_mem(struct net_device *netdev)
{
	// The shregion memory is allocated as SHREGION_PERSISTENT so maybe we don't
	// need to clean-up.

	return 0;
}

//--------------------------------------------------------------------------------------
// mlnet_up()    Enable tx and rx transimission
//--------------------------------------------------------------------------------------
static int mlnet_up(struct net_device *netdev)
{
	struct ml_priv *priv = netdev_priv(netdev);

	// Enables transmission for the device
	netif_start_queue(netdev);
	// Enable NAPI scheduling aka polling interface
	napi_enable(&priv->napi);

	priv->link_state = MLNET_LINK_UP;

	DPRINT(0, "[%s] MLNET is UP\n", __func__);

	// Kick off polling
	napi_schedule(&priv->napi);

	return 0;
}

//--------------------------------------------------------------------------------------
// mlnet_up()    Disable tx and rx transimission
//--------------------------------------------------------------------------------------
static int mlnet_down(struct net_device *netdev)
{
	struct ml_priv *priv = netdev_priv(netdev);

	// Disable NAPI scheduling
	napi_disable(&priv->napi);
	// Disable transmission for the device
	netif_stop_queue(netdev);

	priv->link_state = MLNET_LINK_DOWN;

	DPRINT(0, "[%s] MLNET is DOWN\n", __func__);

	return 0;
}

static int mlnet_net_init(void);

static int mlnet_alloc_mem(void *data)
{
	int ret = 0;
	uint32_t msg;
	long maj, min;

#if defined(CONFIG_ML_CVIP_ARM)
	/* if nova isn't alive we need to wait for it */
	ret = get_nova_status();
	if (!ret) {
		while (1) {
			if (!wait_for_completion_timeout(&nova_alive, MAX_NOVA_TIMEOUT)) {
				wait_count++;
			} else {
				break;
			}
			if (wait_count > RETRY_TIMEOUT) {
				pr_err("mlnet timed out waiting for nova\n");
				break;
			}
		}
	}
	kpi.nova_alive_time_us = gsm_jrtc_get_time_us() - kpi.jrtc_start_offset_us;

	/* if xport ready happens before mlnet is loaded we won't get the
	 * callback we are expecting, so check first
	 */
	ret = get_xport_status(MLNET_HEAP);
	if (!ret) {
		while (1) {
			if (!wait_for_completion_timeout(&xport_ready, MAX_MEM_TIMEOUT)) {
				xport_wait_count++;
			} else {
				pr_err("mlnet: xport now ready\n");
				break;
			}
			if (xport_wait_count > RETRY_TIMEOUT) {
				pr_err("mlnet timed out waiting for xport\n");
				break;
			}

		}
	}
	kpi.mem_ready_time_us = gsm_jrtc_get_time_us() - kpi.jrtc_start_offset_us;
#else
	pr_info("[%s] waiting-for-cvip.\n", __func__);
	if (!completion_done(&mdev.priv->mem_ready)) {
		if (!wait_for_completion_timeout(&mdev.priv->mem_ready, MAX_MEM_TIMEOUT_CVIP)) {
			pr_err("mlnet timed out waiting for cvip\n");
			return -1;
		}
	}
	pr_info("[%s] waiting-complete!\n", __func__);
#endif
	ret = alloc_meta_data(&mdev);
	if (ret < 0) {
		DPRINT(0, "mlnet error allocating meta data\n");
		return ret;
	}

	if (kstrtol(MLNET_MODULE_MAJOR_VERSION, 10, &maj)) {
		pr_err("MLNET Major version could not be parsed\n");
		return -EINVAL;
	}

	if (kstrtol(MLNET_MODULE_MINOR_VERSION, 10, &min)) {
		pr_err("MLNET Minor version could not be parsed\n");
		return -EINVAL;
	}

	mdata->local_version = (maj << 16) | (min & 0xffff);
#if defined(CONFIG_ML_CVIP_ARM)
	mdata->shared_version = mdata->local_version;

	/* Signal to other side */
	msg = MLNET_CVIP_MEM_RDY;
#else
	msg = MLNET_X86_MEM_RDY;
#endif

	ret = mlnet_alloc_queues(mdev.netdev);
	if (ret < 0) {
		DPRINT(0, "error allocating memory pool\n");
		return ret;
	}

	mlnet_send_notification(msg);

#if defined(CONFIG_ML_CVIP_ARM)
	pr_info("[%s] waiting-for-nova.\n", __func__);
	if (!completion_done(&mdev.priv->mem_ready)) {
		pr_info("[%s] waiting!\n", __func__);
		wait_for_completion(&mdev.priv->mem_ready);
	}
	pr_err("mlnet waiting-complete!\n");

	/* Signal to other side in case nova wasn't ready when we sent
	 * the first one.
	 */
	msg = MLNET_CVIP_UP;
	mlnet_send_notification(msg);
#endif
	atomic_set(&mdev.priv->mlnet_online, 1);
	pr_err("mlnet: mem ready\n");
	mem_ready = 1;
	sysfs_notify(&mdev.netdev->dev.kobj, NULL, "mem_ready");

	return 0;
}

//--------------------------------------------------------------------------------------
// mlnet_open()    Bring interface up.
//--------------------------------------------------------------------------------------
static int mlnet_open(struct net_device *netdev)
{
	int ret;
	uint32_t msg;
	int online = 0;

	if (!mdata) {
		pr_err("mlnet meta data not ready\n");
		return -ENODEV;
	}

	online = atomic_read(&mdev.priv->mlnet_online);
	if (!online) {
		pr_err("mlnet is no online yet\n");
		return -ENODEV;
	}

	if (mdata->local_version != mdata->shared_version) {
		DPRINT(0, "mlnet version mismatch: local [0x%X] vs. remote [0x%X]\n",
				mdata->local_version, mdata->shared_version);
		return -EINVAL;
        }

	ret = mlnet_up(netdev);
	if (ret != 0)
		pr_err("[%s] Failed to bring mlnet up: %d\n", __func__, ret);

	/* Signal to other side */
#if defined(CONFIG_ML_CVIP_ARM)
	msg = MLNET_CVIP_UP;
	cvip_online = 1;
	sysfs_notify(&netdev->dev.kobj, NULL, "cvip_online");
#else
	msg = MLNET_X86_UP;
	x86_online = 1;
	sysfs_notify(&netdev->dev.kobj, NULL, "x86_online");
#endif

	mlnet_send_notification(msg);

        kpi.ready_time_us = gsm_jrtc_get_time_us() - kpi.jrtc_start_offset_us;
        kpi.ready_time_us = kpi.ready_time_us - kpi.ready_start_time_us;

	return ret;
}

//--------------------------------------------------------------------------------------
// mlnet_close()    Bring interface down.
//--------------------------------------------------------------------------------------
static int mlnet_close(struct net_device *netdev)
{
	int ret;
	uint32_t msg;

	DPRINT(1, "[%s] dev name: %s\n", __func__, netdev->name);

	ret = mlnet_down(netdev);
	if (ret < 0) {
		pr_err("[%s] Failed to bring mlnet down: %d\n", __func__, ret);
		return ret;
	}

	ret = mlnet_free_shared_mem(netdev);
	if (ret < 0)
		pr_err("[%s] Failed to free shared memory: %d\n", __func__, ret);

	/* Signal to other side */
#if defined(CONFIG_ML_CVIP_ARM)
	msg = MLNET_CVIP_DOWN;
	cvip_online = 0;
	sysfs_notify(&netdev->dev.kobj, NULL, "cvip_online");
#else
	msg = MLNET_X86_DOWN;
	x86_online = 0;
	sysfs_notify(&netdev->dev.kobj, NULL, "x86_online");
#endif

	mlnet_send_notification(msg);

	return ret;
}

//--------------------------------------------------------------------------------------
// mlnet_check_mailbox()   Check our mailbox for posted messages
//--------------------------------------------------------------------------------------
static int mlnet_check_mailbox(struct ml_priv *priv, int budget)
{
	int work_done = 0;
	struct ml_buffer *buffer_ptr = NULL;
	uint32_t *data_len_ptr;
	uint8_t device;

#if defined(CONFIG_ML_CVIP_ARM)
	buffer_ptr = &mdev.priv->data.cvip;
	device = CVIP;
	data_len_ptr = mdata->data_len_cvip;
#else
	buffer_ptr = &mdev.priv->data.nova;
	device = NOVA;
	data_len_ptr = mdata->data_len_nova;
#endif

	// Only process up to the budget
	while (work_done < budget) {
		uint8_t *data;
		uint32_t data_len;
		struct sk_buff *skb;
		unsigned long flags;
		uint8_t head, tail;


		// Only one consumer is allowed
		spin_lock_irqsave(&buffer_ptr->rx_lock, flags);

		head = queue_acquire_head(device);
		tail = queue_get_tail(device);

		if (CIRC_CNT(head, tail, MLNET_MBOX_SLOTS) < 1) {
			// Nothing in mailbox
			DPRINT(4, "nothing in mailbox\n");
			spin_unlock_irqrestore(&buffer_ptr->rx_lock, flags);
			break;
		}

		begin_cpu_access_from_dev(&buffer_ptr->buffer[tail]);
		data_len = data_len_ptr[tail];
		data = buffer_ptr->buffer[tail].data;

		DPRINT(2, "[%s] You got mail, head:%u tail:%u addr:%px len:%u\n",
			__func__, head, tail, data, data_len);
		DHPRINT(2, KERN_WARNING, "pkt rx: ", DUMP_PREFIX_ADDRESS,
				16, 1, data, (data_len > 32) ? 32 : data_len, false);

		/* this is all the same as we just build skb around it */
		skb = dev_alloc_skb(data_len + NET_IP_ALIGN);
		if (!skb) {
			pr_err_ratelimited("[%s] low on mem - packet dropped\n", __func__);
			priv->stats.rx_dropped++;
			spin_unlock_irqrestore(&buffer_ptr->rx_lock, flags);
			break;
		}

		skb_reserve(skb, NET_IP_ALIGN);
		memcpy(skb_put(skb, data_len), data, data_len);
#if defined(CONFIG_ML_CVIP_ARM)
		mem_cnt_to_arm += data_len;
#else
		mem_cnt_to_x86 += data_len;
#endif
		end_cpu_access_from_dev(&buffer_ptr->buffer[tail]);

		// Increment and potentially wrap tail
		queue_inc_tail(device, tail);

		spin_unlock_irqrestore(&buffer_ptr->rx_lock, flags);

		skb->dev = priv->netdev;
		skb->protocol = eth_type_trans(skb, priv->netdev);
		skb->ip_summed = CHECKSUM_UNNECESSARY;
		priv->stats.rx_packets++;
		priv->stats.rx_bytes += data_len;
		netif_receive_skb(skb);
		work_done++;
	}

	return work_done;
}


//--------------------------------------------------------------------------------------
// mlnet_tx_slots_available()   Returns how many transmit slots are available in opposing mailbox
//--------------------------------------------------------------------------------------
static int mlnet_tx_slots_available(struct ml_priv *priv)
{
	uint8_t tail = 0;
	uint8_t head = 0, space = 0;
	unsigned long flags = 0;
	struct ml_buffer *buffer_ptr = NULL;
	uint32_t device = 0;

	/* get the correct queue */
#if defined(CONFIG_ML_CVIP_ARM)
	buffer_ptr = &mdev.priv->data.nova;
	device = NOVA;
#else
	buffer_ptr = &mdev.priv->data.cvip;
	device = CVIP;
#endif

	spin_lock_irqsave(&buffer_ptr->tx_lock, flags);

	head = queue_get_head(device);
	tail = queue_acquire_tail(device);

	space = CIRC_SPACE(head, tail, MLNET_MBOX_SLOTS);

	spin_unlock_irqrestore(&buffer_ptr->tx_lock, flags);

	DPRINT(1, "space in the queue %d\n", space);
	return space;
}


//--------------------------------------------------------------------------------------
// mlnet_poll()    NAPI poll function
//--------------------------------------------------------------------------------------
static int mlnet_poll(struct napi_struct *napi, int budget)
{
	struct ml_priv *priv = container_of(napi, struct ml_priv, napi);
	unsigned int work_done = 0;

	// Check if we need to resume the transmission queue. (We stop it if slots are low)
	// TODO(sadadi): Also consider adding this to some type of watchdog
	if (netif_queue_stopped(priv->netdev)) {
		// Only wake up if there are slots available
		if (mlnet_tx_slots_available(priv) > 1) {
			netif_carrier_on(priv->netdev);
			netif_wake_queue(priv->netdev);
		}
	}

	work_done = mlnet_check_mailbox(priv, budget);

	// If budget not fully consumed, exit the polling mode
	if (work_done < budget) {

		if (napi_complete_done(napi, work_done)) {
			#if defined(CONFIG_ML_CVIP_ARM)
				mlnet_gsm_nibble_write(
					GSM_RESERVED_MLNET_CONTROL_OFFSET,
					POLL_MODE_CVIP_TO_NOVA_NIBBLE_OFFSET, 0);
			#else
				mlnet_gsm_nibble_write(
					GSM_RESERVED_MLNET_CONTROL_OFFSET,
					POLL_MODE_NOVA_TO_CVIP_NIBBLE_OFFSET, 0);
			#endif
		}
	}

	return work_done;
}

//--------------------------------------------------------------------------------------
// mlnet_xmit_frame()    Transmit a packet (called by the kernel)
//--------------------------------------------------------------------------------------
static netdev_tx_t mlnet_xmit_frame(struct sk_buff *skb, struct net_device *netdev)
{
	static int dropped;
	uint8_t head, tail, space;
	unsigned long flags;
	struct ml_priv *priv = netdev_priv(netdev);
	uint32_t *data_len_ptr = NULL;
	struct ml_buffer *buffer_ptr = NULL;
	uint32_t device;
	uint32_t msg;
	int poll_mode;
#if defined(CONFIG_ML_CVIP_ARM)
	int idle;

	idle = atomic_read(&mdev.priv->s2_idle);
	if (idle)
		return NETDEV_TX_BUSY;
#endif


	if (skb->len > MLNET_MAX_MTU) {
		pr_err("[%s] tx pkt too long: %d (mtu=%lu)\n", __func__, skb->len, MLNET_MAX_MTU);
		return NETDEV_TX_BUSY;
	}
	xmit_cnt++;

	/* get the correct queue */
#if defined(CONFIG_ML_CVIP_ARM)
	buffer_ptr = &mdev.priv->data.nova;
	device = NOVA;
	data_len_ptr = mdata->data_len_nova;
#else
	buffer_ptr = &mdev.priv->data.cvip;
	device = CVIP;
	data_len_ptr = mdata->data_len_cvip;
#endif

	spin_lock_irqsave(&buffer_ptr->tx_lock, flags);
	head = queue_get_head(device);
	tail = queue_acquire_tail(device);
	begin_cpu_access_to_dev(&buffer_ptr->buffer[head]);

	DPRINT(2, "[%s] Sending mail, head:%u tail:%u addr:%px len:%u\n",
		 __func__, head, tail, buffer_ptr->buffer[head].data, skb->len);
	DHPRINT(2, KERN_WARNING, "pkt tx: ", DUMP_PREFIX_ADDRESS,
			16, 1, skb->data, (skb->len > 32) ? 32 : skb->len, false);

	space = CIRC_SPACE(head, tail, MLNET_MBOX_SLOTS);
	if (space == 1) {
		// The receiver is almost out of space in the mailbox. This is a soft error.
		// Allow the packet to be queued, but stop transmission attempts.
		DPRINT(1, "[%s] receiver's mailbox is almost full!\n", __func__);
		netif_stop_queue(netdev);
		netif_carrier_off(netdev);
		soft_stop++;
	} else if (space == 0) {
		// The receiver is out of space in the mailbox. This is a hard error.
		if (dropped == 0)
			DPRINT(1, "[%s] receiver's mailbox is full!\n", __func__);
		dropped++;
		hard_stop++;
		netif_stop_queue(netdev);
		netif_carrier_off(netdev);
		spin_unlock_irqrestore(&buffer_ptr->tx_lock, flags);
		end_cpu_access_to_dev(&buffer_ptr->buffer[head]);
		return NETDEV_TX_BUSY;
	}

	if (dropped) {
		pr_err("[%s] Dropped %d messages!\n", __func__, dropped);
		dropped = 0;
	}

	data_len_ptr[head] = skb->len;
	memcpy(buffer_ptr->buffer[head].data, skb->data, skb->len);
#if defined(CONFIG_ML_CVIP_ARM)
		mem_cnt_to_x86 += skb->len;
#else
		mem_cnt_to_arm += skb->len;
#endif
	end_cpu_access_to_dev(&buffer_ptr->buffer[head]);

	skb_tx_timestamp(skb);
	queue_inc_head(head, device);

	// Packet has been copied already.
	dev_kfree_skb_any(skb);

	// Update stats.
	priv->stats.tx_packets++;
	priv->stats.tx_bytes += skb->len;

	/* Signal to other side */
	msg = MLNET_INT;
	mlnet_send_notification(msg);

#if defined(CONFIG_ML_CVIP_ARM)
	poll_mode = mlnet_gsm_nibble_read(GSM_RESERVED_MLNET_CONTROL_OFFSET,
					POLL_MODE_CVIP_TO_NOVA_NIBBLE_OFFSET);
#else
	poll_mode = mlnet_gsm_nibble_read(GSM_RESERVED_MLNET_CONTROL_OFFSET,
						POLL_MODE_NOVA_TO_CVIP_NIBBLE_OFFSET);
#endif

	if (!poll_mode || (last_intr_send_count > MAX_INTR_SEND_COUNT)) {
		mlnet_send_notification(msg);
		last_intr_send_count = 0;
	} else {
		last_intr_send_count++;
	}

	spin_unlock_irqrestore(&buffer_ptr->tx_lock, flags);


	return NETDEV_TX_OK;
}

//--------------------------------------------------------------------------
// mlnet_ioctl()    Ioctl handling.
//--------------------------------------------------------------------------
static int mlnet_ioctl(struct net_device *netdev, struct ifreq *ifr, int cmd)
{
	DPRINT(1, "[%s] cmd = %d / 0x%08X:", __func__, cmd, cmd);

	switch (cmd) {
	/* Get address of MII PHY in use. */
	case SIOCGMIIPHY:
		DPRINT(1, "(SIOCGMIIPHY)\n");
		break;
	/* Read MII PHY register. */
	case SIOCGMIIREG:
		DPRINT(1, "(SIOCGMIIREG)\n");
		break;
	/* Write MII PHY register. */
	case SIOCSMIIREG:
		DPRINT(1, "(SIOCSMIIREG)\n");
		break;
	default:
		DPRINT(1, "(unknown)\n");
		break;
	}

	return 0;
}

//--------------------------------------------------------------------------
// mlnet_get_stats()    Return stats info.
//--------------------------------------------------------------------------
static struct net_device_stats *mlnet_get_stats(struct net_device *dev)
{
	struct ml_priv *priv = netdev_priv(dev);

	return &priv->stats;
}

//--------------------------------------------------------------------------
// mlnet_xmit_timeout()    Called when tx takes too long.
//--------------------------------------------------------------------------
#if LINUX_VERSION_CODE <= KERNEL_VERSION(5,15,105)
static void mlnet_xmit_timeout(struct net_device *dev)
#else
static void mlnet_xmit_timeout(struct net_device *dev, unsigned int data)
#endif
{
	struct ml_priv *priv = netdev_priv(dev);

	pr_err("[%s] tx timeout!\n", __func__);
	priv->stats.tx_errors++;
}

//--------------------------------------------------------------------------
// msglevel_set()    Set debug level.
//--------------------------------------------------------------------------
static void msglevel_set(struct net_device *dev, u32 dbg)
{
	dbg_level = dbg;
}

//--------------------------------------------------------------------------
// msglevel_get()    Get debug level.
//--------------------------------------------------------------------------
static u32 msglevel_get(struct net_device *dev)
{
	return dbg_level;
}

static const struct ethtool_ops ethtool_ops = {
	.get_msglevel           = msglevel_get,
	.set_msglevel           = msglevel_set,
};

static const struct net_device_ops mlnet_netdev_ops = {
	.ndo_do_ioctl   = mlnet_ioctl,
	.ndo_open       = mlnet_open,
	.ndo_stop       = mlnet_close,
	.ndo_start_xmit = mlnet_xmit_frame,
	.ndo_tx_timeout = mlnet_xmit_timeout,
	.ndo_get_stats  = mlnet_get_stats,
};

/* sysfs attributes and functions */
static ssize_t get_debug_level(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", dbg_level);
}

static ssize_t set_debug_level(struct device *dev,
			 struct device_attribute *attr, const char *buf, size_t count)
{
	uint32_t level;
	int ret;

	ret = kstrtouint(buf, 0, &level);
	if (ret < 0)
		return ret;

	dbg_level = level;
	return count;
}

static ssize_t get_xport_wait_cnt(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%u\n", xport_wait_count);
}

static ssize_t get_wait_cnt(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%u\n", wait_count);
}

static ssize_t get_mem_cnt_to_arm(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%llu\n", mem_cnt_to_arm);
}

static ssize_t get_mem_cnt_to_x86(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%llu\n", mem_cnt_to_x86);
}


static ssize_t get_version(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", ver_str);
}

static ssize_t get_schd_cnt(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%llu\n", schd_cnt);
}

static ssize_t set_schd_cnt(struct device *dev,
			 struct device_attribute *attr, const char *buf, size_t count)
{
	return count;
}

static ssize_t get_xmit_cnt(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%llu\n", xmit_cnt);
}

static ssize_t set_xmit_cnt(struct device *dev,
			 struct device_attribute *attr, const char *buf, size_t count)
{
	return count;
}

static ssize_t get_rx_intr_counter(struct device *dev,
					struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%u\n", rx_intr_counter);
}

static ssize_t set_rx_intr_counter(struct device *dev,
			 struct device_attribute *attr, const char *buf, size_t count)
{
	return count;
}

static ssize_t get_tx_intr_counter(struct device *dev,
					struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%u\n", tx_intr_counter);
}

static ssize_t set_tx_intr_counter(struct device *dev,
			 struct device_attribute *attr, const char *buf, size_t count)
{
	return count;
}

static ssize_t get_hard_stop(struct device *dev,
					struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%u\n", hard_stop);
}

static ssize_t set_hard_stop(struct device *dev,
			 struct device_attribute *attr, const char *buf, size_t count)
{
	return count;
}

static ssize_t get_soft_stop(struct device *dev,
					struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%u\n", soft_stop);
}

static ssize_t set_soft_stop(struct device *dev,
			 struct device_attribute *attr, const char *buf, size_t count)
{
	soft_stop = 0;
	return count;
}

static ssize_t get_notif_send_error(struct device *dev,
					struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%u\n", notif_send_error);
}

static ssize_t set_notif_send_error(struct device *dev,
			 struct device_attribute *attr, const char *buf, size_t count)
{
	return count;
}

static ssize_t get_cvip_online(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", cvip_online);
}

static ssize_t get_x86_online(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", x86_online);
}

static ssize_t get_nbd_server_ready(struct device *dev,
						      struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%u\n", nbd_server_ready);
}

static ssize_t set_nbd_server_ready(struct device *dev,
			    struct device_attribute *attr, const char *buf, size_t count)
{

#if defined(CONFIG_ML_CVIP_X86)
	mlnet_send_notification(MLNET_NBD_SERVER_READY);
#endif

	nbd_server_ready = 1;
	return count;
}

static ssize_t get_nfs_server_ready(struct device *dev,
						      struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%u\n", nfs_server_ready);
}

static ssize_t set_nfs_server_ready(struct device *dev,
			    struct device_attribute *attr, const char *buf, size_t count)
{

#if defined(CONFIG_ML_CVIP_X86)
	mlnet_send_notification(MLNET_NFS_SERVER_READY);
#endif

	nfs_server_ready = 1;
	return count;
}

static int write_queue(uint32_t queue, int data, uint32_t size)
{
	int i = 0;
	char *ptr;
	struct ml_buffer *buffer_ptr = NULL;
	uint32_t *data_len_ptr;
	int qlen;
	int ret = 0;

	switch (queue) {
	case CVIP:
		buffer_ptr = &mdev.priv->data.cvip;
		qlen = mdata->cvip_qlen;
		data_len_ptr = mdata->data_len_cvip;
		break;
	case NOVA:
		buffer_ptr = &mdev.priv->data.nova;
		qlen = mdata->nova_qlen;
		data_len_ptr = mdata->data_len_nova;
		break;
	default:
		DPRINT(0, "mlnet unknown queue given\n");
		ret = -1;
		qlen = 0;
		break;
	}

	for (i = 0; i < qlen; i++) {
		DPRINT(1, "node %d\n", i);
		ptr = (char *)buffer_ptr->buffer[i].data;
		DPRINT(1, "memset %x\n", data);
		memset(ptr, 'a', size);
		data_len_ptr[i] = size;
		DPRINT(1, "datalen %x\n", data_len_ptr[i]);
		wmb();
	}

	return ret;
}

static int read_queue(uint32_t queue, uint32_t size)
{
	int i = 0;
	char *ptr;
	struct ml_buffer *buffer_ptr = NULL;
	uint32_t *data_len_ptr;
	int ret = 0;
	int qlen;

	(void)ptr;
	(void)buffer_ptr;
	(void)data_len_ptr;

	switch (queue) {
	case CVIP:
		buffer_ptr = &mdev.priv->data.cvip;
		qlen = mdata->cvip_qlen;
		data_len_ptr = mdata->data_len_cvip;
		break;
	case NOVA:
		buffer_ptr = &mdev.priv->data.nova;
		qlen = mdata->nova_qlen;
		data_len_ptr = mdata->data_len_nova;
		break;
	default:
		ret = -1;
		qlen = 0;
		break;
	}

	rmb();
	for (i = 0; i < qlen; i++)
		ptr = (char *)buffer_ptr->buffer[i].data;

	return ret;
}

static ssize_t set_packet_cvip_test(struct device *dev,
			    struct device_attribute *attr, const char *buf, size_t count)
{
	int data = 'x';

	packet_test = write_queue(CVIP, data, MLNET_MAX_MTU);

	return count;
}


static ssize_t get_packet_cvip_test(struct device *dev, struct device_attribute *attr, char *buf)
{
	int ret = 0;

	ret = read_queue(CVIP, 16);

	return sprintf(buf, "%u\n", ret);
}

static ssize_t set_packet_nova_test(struct device *dev,
			    struct device_attribute *attr, const char *buf, size_t count)
{
	int data = 'x';

	packet_test = write_queue(NOVA, data, MLNET_MAX_MTU);

	return count;
}


static ssize_t get_packet_nova_test(struct device *dev, struct device_attribute *attr, char *buf)
{
	int ret = 0;

	ret = read_queue(NOVA, 16);

	return sprintf(buf, "%u\n", ret);
}

static ssize_t get_nova_head(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%u\n", mdata->nova_head);
}

static ssize_t get_nova_tail(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%u\n", mdata->nova_tail);
}

static ssize_t get_mlnet_setup_complete(struct device *dev,
						      struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%u\n", mlnet_setup_complete);
}

static ssize_t set_mlnet_setup_complete(struct device *dev,
			    struct device_attribute *attr, const char *buf, size_t count)
{

#if defined(CONFIG_ML_CVIP_X86)
	mlnet_send_notification(MLNET_MLNET_SETUP_COMPLETE);
#endif

	mlnet_setup_complete = 1;
	return count;
}

static ssize_t get_mem_ready(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", mem_ready);
}

static ssize_t get_s2_idle(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	int idle;

	idle = atomic_read(&mdev.priv->s2_idle);

	return sprintf(buf, "%d\n", idle);
}

static ssize_t get_nova_alive_time(struct device *dev,
				 struct device_attribute *attr, char *buf)
{

	return sprintf(buf, "%lld\n", kpi.nova_alive_time_us);
}

static ssize_t get_mem_ready_time(struct device *dev,
				 struct device_attribute *attr, char *buf)
{

	return sprintf(buf, "%lld\n", kpi.mem_ready_time_us);
}

static ssize_t get_ready_time(struct device *dev,
				 struct device_attribute *attr, char *buf)
{

	return sprintf(buf, "%lld\n", kpi.ready_time_us);
}



static DEVICE_ATTR(mem_ready_time, 0644, get_mem_ready_time, NULL);
static DEVICE_ATTR(nova_alive_time, 0644, get_nova_alive_time, NULL);
static DEVICE_ATTR(s2_idle, 0644, get_s2_idle, NULL);
static DEVICE_ATTR(ready_time, 0644, get_ready_time, NULL);
static DEVICE_ATTR(mem_ready, 0644, get_mem_ready, NULL);
static DEVICE_ATTR(xport_wait_cnt, 0644, get_xport_wait_cnt, NULL);
static DEVICE_ATTR(wait_cnt, 0644, get_wait_cnt, NULL);
static DEVICE_ATTR(mem_cnt_to_arm, 0644, get_mem_cnt_to_arm, NULL);
static DEVICE_ATTR(mem_cnt_to_x86, 0644, get_mem_cnt_to_x86, NULL);
static DEVICE_ATTR(debug_level, 0644, get_debug_level, set_debug_level);
static DEVICE_ATTR(version, 0444, get_version, NULL);
static DEVICE_ATTR(schd_cnt, 0644, get_schd_cnt, set_schd_cnt);
static DEVICE_ATTR(xmit_cnt, 0644, get_xmit_cnt, set_xmit_cnt);
static DEVICE_ATTR(rx_intr_cnt, 0644, get_rx_intr_counter, set_rx_intr_counter);
static DEVICE_ATTR(tx_intr_cnt, 0644, get_tx_intr_counter, set_tx_intr_counter);
static DEVICE_ATTR(hard_stop, 0644, get_hard_stop, set_hard_stop);
static DEVICE_ATTR(soft_stop, 0644, get_soft_stop, set_soft_stop);
static DEVICE_ATTR(notif_send_error, 0644, get_notif_send_error, set_notif_send_error);
static DEVICE_ATTR(cvip_online, 0644, get_cvip_online, NULL);
static DEVICE_ATTR(x86_online, 0644, get_x86_online, NULL);
static DEVICE_ATTR(nbd_server_ready, 0644, get_nbd_server_ready, set_nbd_server_ready);
static DEVICE_ATTR(nfs_server_ready, 0644, get_nfs_server_ready, set_nfs_server_ready);
static DEVICE_ATTR(packet_test_cvip, 0644, get_packet_cvip_test, set_packet_cvip_test);
static DEVICE_ATTR(packet_test_nova, 0644, get_packet_nova_test, set_packet_nova_test);
static DEVICE_ATTR(show_head_nova, 0644, get_nova_head, NULL);
static DEVICE_ATTR(show_tail_nova, 0644, get_nova_tail, NULL);
static DEVICE_ATTR(mlnet_setup_complete, 0644, get_mlnet_setup_complete, set_mlnet_setup_complete);


static const struct attribute *const netdev_sysfs_attrs[] = {
        &dev_attr_mem_ready_time.attr,
        &dev_attr_nova_alive_time.attr,
	&dev_attr_s2_idle.attr,
	&dev_attr_mem_ready.attr,
	&dev_attr_ready_time.attr,
        &dev_attr_xport_wait_cnt.attr,
	&dev_attr_mem_cnt_to_arm.attr,
	&dev_attr_mem_cnt_to_x86.attr,
	&dev_attr_wait_cnt.attr,
	&dev_attr_debug_level.attr,
	&dev_attr_version.attr,
	&dev_attr_schd_cnt.attr,
	&dev_attr_xmit_cnt.attr,
	&dev_attr_rx_intr_cnt.attr,
	&dev_attr_tx_intr_cnt.attr,
	&dev_attr_hard_stop.attr,
	&dev_attr_soft_stop.attr,
	&dev_attr_notif_send_error.attr,
	&dev_attr_cvip_online.attr,
	&dev_attr_x86_online.attr,
	&dev_attr_nbd_server_ready.attr,
	&dev_attr_nfs_server_ready.attr,
	&dev_attr_packet_test_cvip.attr,
	&dev_attr_packet_test_nova.attr,
	&dev_attr_show_head_nova.attr,
	&dev_attr_show_tail_nova.attr,
	&dev_attr_mlnet_setup_complete.attr,
	NULL,
};

static const struct attribute_group netdev_sysfs_group = {
	.name = NULL,
	.attrs = (struct attribute **)netdev_sysfs_attrs,
};

void mailbox_check(void)
{
	struct ml_priv *priv = netdev_priv(mdev.netdev);
#if defined(CONFIG_ML_CVIP_ARM)
	int idle;

	idle = atomic_read(&mdev.priv->s2_idle);
	if (idle)
		return;
#endif
        /* Schedule the polling task to start */
	if (priv->link_state == MLNET_LINK_UP) {
		schd_cnt++;
		#if defined(CONFIG_ML_CVIP_ARM)
			mlnet_gsm_nibble_write(
				GSM_RESERVED_MLNET_CONTROL_OFFSET,
				POLL_MODE_CVIP_TO_NOVA_NIBBLE_OFFSET, 1);
		#else
			mlnet_gsm_nibble_write(
				GSM_RESERVED_MLNET_CONTROL_OFFSET,
				POLL_MODE_NOVA_TO_CVIP_NIBBLE_OFFSET, 1);
		#endif
		napi_schedule(&priv->napi);
	}
}

//--------------------------------------------------------------------------
// mlnet_netdev_init()    Set up net-device struct.
//--------------------------------------------------------------------------
void mlnet_netdev_init(struct net_device *dev)
{
	struct ml_priv *priv;

	// Go with standard ethernet params.
	ether_setup(dev);

	// Override others.
	dev->mtu            = MLNET_MAX_MTU - ETH_HLEN;
	dev->max_mtu        = MLNET_MAX_MTU - ETH_HLEN;
	dev->watchdog_timeo = HZ * 2;
	dev->netdev_ops     = &mlnet_netdev_ops;
	dev->ethtool_ops    = &ethtool_ops;

	priv = netdev_priv(dev);
	memset(priv, 0, sizeof(*priv));

	netif_napi_add(dev, &priv->napi, mlnet_poll, MLNET_NAPI_WEIGHT);
	priv->netdev = dev;
	priv->link_state = MLNET_LINK_INIT;
}

//--------------------------------------------------------------------------
// mlnet_init()    Driver init.
//--------------------------------------------------------------------------
static int mlnet_net_init(void)
{
	int err, i;

	// Allocate the device and initialize it.
	mdev.netdev = alloc_netdev(sizeof(struct ml_priv),
					"mlnet%d", NET_NAME_UNKNOWN, mlnet_netdev_init);
	if (!mdev.netdev) {
		pr_err("[%s] Error unable to alloc netdev\n", __func__);
		return -ENOMEM;
	}

	// Choose mac address depending on who we are.
	memcpy(mdev.netdev->dev_addr, mac_address, ETH_ALEN);

	// Register device.
	err = register_netdev(mdev.netdev);
	if (err) {
		pr_err("[%s] Cannot register net device, aborting\n", __func__);
		goto err_out;
	}

	if (sysfs_create_group(&mdev.netdev->dev.kobj, &netdev_sysfs_group) < 0) {
		netdev_alert(mdev.netdev, "sysfs group failed\n");
		goto err_out;
	}

	for (i = 0; i < NOTIFY_CNT; i++) {
		mlnet_sysfs_dirent[i] = sysfs_get_dirent(
					mdev.netdev->dev.kobj.sd,
					mlnet_notify_names[i]);
		if (mlnet_sysfs_dirent[i] == NULL) {
			pr_err("sysfs_get_dirent is NULL for %s\n", mlnet_notify_names[i]);
			goto err_out;
		}
	}

	return 0;

err_out:
	for (i = 0; i < NOTIFY_CNT; i++) {
		if (mlnet_sysfs_dirent[i] != NULL)
			sysfs_put(mlnet_sysfs_dirent[i]);
	}

	free_netdev(mdev.netdev);
	mdev.netdev = 0;

	return err;
}

//--------------------------------------------------------------------------
// mlnet_init_module()    Module entry.
//--------------------------------------------------------------------------
static int mlnet_probe(struct platform_device *pdev)
{
	int status = 0;

	DPRINT(0, "[%s] Starting mlnet v%s.\n", __func__, ver_str);

	// Clear out globals.
	memset(&mdev, 0, sizeof(mdev));
	memset(&priv, 0, sizeof(priv));

	mdev.priv = &priv;
	mdev.dev = &pdev->dev;

	#if defined(CONFIG_ML_CVIP_ARM)
		DPRINT(0, "[%s] Current proc is ARM\n", __func__);
	#else
		DPRINT(0, "[%s] Current proc is x86\n", __func__);
	#endif

	mdev.priv->mlnet_online = (atomic_t)ATOMIC_INIT(0);
	mdev.priv->s2_idle = (atomic_t)ATOMIC_INIT(0);

        status = mlnet_net_init();
	if (status < 0)
		return status;

	// Register XPC notification handlers
	status = mlnet_register_xpc_handlers();
	if (status < 0)
		return status;

	init_completion(&mdev.priv->mem_ready);
#if defined(CONFIG_ML_CVIP_ARM)
	kpi.jrtc_start_offset_us = gsm_jrtc_get_time_us();
        init_completion(&xport_ready);
	init_completion(&nova_alive);
	register_xport_notify();
	register_s2_resume_notifiers();
	register_nova_alive_notify();

	status = register_reboot_notifier(&mlnet_notifier);
	if (status) {
		pr_err("mlnet : failed to register reboot notifier (err=%d)\n", status);
		return status;
	}
        kpi.ready_start_time_us = gsm_jrtc_get_time_us() - kpi.jrtc_start_offset_us;
#endif
	mem_cnt_to_arm = 0;
	mem_cnt_to_x86 = 0;
	mem_ready = 0;
	alloc_thread = kthread_run(mlnet_alloc_mem, NULL, "mlnet alloc mem thread");
	if (IS_ERR(alloc_thread)) {
		pr_err("mlnet : failed to start alloc thread\n");
		status = PTR_ERR(alloc_thread);
	}

	return status;
}

//--------------------------------------------------------------------------
// mlnet_cleanup
//--------------------------------------------------------------------------
static void mlnet_cleanup(void)
{
	int i;

	DPRINT(0, "[%s] mlnet is exiting...!\n", __func__);

	mlnet_deregister_xpc_handlers();

	for (i = 0; i < NOTIFY_CNT; i++) {
		if (mlnet_sysfs_dirent[i] != NULL)
			sysfs_put(mlnet_sysfs_dirent[i]);
	}

	if (mdev.netdev)
		if (mdev.netdev->reg_state == NETREG_REGISTERED) {
			unregister_netdev(mdev.netdev);

		if (mdev.netdev) {
			mlnet_free_shared_mem(mdev.netdev);
			free_netdev(mdev.netdev);
			mdev.netdev = 0;
		}
	}
}

static int mlnet_remove(struct platform_device *pdev)
{
	mlnet_cleanup();
	sysfs_remove_group(&mdev.netdev->dev.kobj, &netdev_sysfs_group);
	return 0;
}

static const struct of_device_id mlnet_of_match[] = {
	{ .compatible = "amd,cvip-mlnet", },
	{ },
};

static struct platform_driver mlnet_driver = {
	.probe = mlnet_probe,
	.remove = mlnet_remove,
	.driver = {
		.name = "mlnet",
		.of_match_table = mlnet_of_match,
	},
};

#if defined(CONFIG_ML_CVIP_ARM)
static int __init mlnet_init(void)
{
	int ret;

	ret = platform_driver_register(&mlnet_driver);
	if (ret)
		return ret;

	return 0;
}

static void __exit mlnet_exit(void)
{
	platform_driver_unregister(&mlnet_driver);
}

#else

module_platform_driver(mlnet_driver);
static struct platform_device *mlnet_dev;

static int __init mlnet_init(void)
{
	int ret = 0;

	mlnet_dev = platform_device_register_simple("mlnet", 0, NULL, 0);
	if (IS_ERR(mlnet_dev))
		ret = PTR_ERR(mlnet_dev);

	return ret;
}

static void __exit mlnet_exit(void)
{
	platform_device_unregister(mlnet_dev);
}
#endif

#if defined(CONFIG_ML_CVIP_ARM)
late_initcall_sync(mlnet_init);
#else
module_init(mlnet_init);
#endif
module_exit(mlnet_exit);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Magic Leap, Inc.");
MODULE_AUTHOR("sadadi_tklabs@magicleap.com");
MODULE_DESCRIPTION("MLNET Network Interface");
