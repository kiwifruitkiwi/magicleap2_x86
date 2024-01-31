/*
 * IPC library core implementation
 *
 * Copyright (C) 2019-2022 Advanced Micro Devices, Inc. All rights reserved.
 */

#include "ipc.h"
#include "cvip.h"
#ifdef CONFIG_IPC_XPORT
#include "ion.h"
#include "mero_ion.h"
#endif
#include <linux/init.h>
#include <linux/dma-buf.h>
#include <linux/suspend.h>
#include <linux/reboot.h>
#include <linux/smu_protocol.h>
#include <linux/cvip_event_logger.h>
#include <linux/umh.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/syscalls.h>
#ifdef CONFIG_CVIP_IPC
#include <dsp_manager_internal.h>


#define DPM1    1
#define DPM3    3
#endif

static int intr_test_count;
static u64 gpuva;
static u64 x86_buf_base;
static bool allow_reboot;
#ifdef CONFIG_IPC_XPORT
static int alloc_fd = -1;
#endif

bool match(struct ipc_message_range *range,
	   struct callback_list *node)
{
	if ((range->start == node->range.start)
	   && (range->end == node->range.end)) {
		return true;
	} else
		return false;
}

static int __init set_allow_reboot(char *str)
{
	allow_reboot = true;

	return 1;
}
__setup_param("ALLOW_REBOOT", set_allow_reboot, set_allow_reboot, 0);

bool overlap(struct ipc_message_range *range, struct callback_list *node)
{
	if (((range->start > node->range.start)
	     && (range->start < node->range.end))
	    || ((range->end > node->range.start)
		&& (range->end < node->range.end))) {
		return true;
	} else
		return false;
}

void log_message(struct mbox_message *msg)
{
	int i, length, seq;

	pr_debug("received new message, id = 0x%x\n", msg->message_id);
	length = (msg->message_id & MSG_LEN_MASK) >> MSG_LEN_SHIFT;
	seq = (msg->message_id & MSG_SEQNUM_MASK) >> MSG_SEQNUM_SHIFT;
	pr_debug("message seq is 0x%x", seq);
	for (i = 0; i < length; i++)
		pr_debug("message is 0x%x", msg->msg_body[i]);
	pr_debug("\n");
}

void log_payload(char *tag, u32 *payload, int length)
{
	int i;
	char pl[256];
	size_t size = sizeof(pl) - 1;
	size_t n = 0;

	n = snprintf(pl, size, "%s", tag);
	for (i = 0; i < length && n < size - 1; i++)
		n += snprintf(pl + n, size - n, " 0x%x", payload[i]);

	pr_debug("%s\n", pl);
}

int alloc_xfer_list(struct ipc_chan *chan)
{
	struct message_list *msg;
	int i;

	msg = kcalloc(MAX_MESSAGE_COUNT, sizeof(*msg), GFP_KERNEL);
	if (!msg)
		return -ENOMEM;
	chan->msg = msg;
	for (i = 0; i < MAX_MESSAGE_COUNT; i++) {
		init_completion(&msg->done);
		list_add_tail(&msg->list_node, &chan->msg_list);
		msg++;
	}
	return 0;
}

int dealloc_xfer_list(struct ipc_chan *chan)
{
	kfree(chan->msg);
	return 0;
}

void mbox_tx_done(struct mbox_client *cl, void *mssg, int r)
{
	unsigned long flags;
	struct message_list *msg;
	struct ipc_chan *chan = container_of(cl, struct ipc_chan, cl);

	if (!chan->sync) {
		/* for async message put mbox message back for recycle */
		put_mbox_message(chan, (struct mbox_message *)mssg);
	} else {
		spin_lock_irqsave(&chan->rx_lock, flags);
		if (list_empty(&chan->rx_pending)) {
			spin_unlock_irqrestore(&chan->rx_lock, flags);
			return;
		}
		msg = list_first_entry(&chan->rx_pending, struct message_list,
				       list_node);
		list_del(&msg->list_node);
		if (msg && !completion_done(&msg->done)) {
			/* for sync channel, ack and status will be set in
			 * destination core, need to retrieve the mailbox
			 * message through payload registers
			 */
			mbox_get_ack_message(chan->chan, &msg->msg);
			handle_message(&msg->msg);
			log_message(&msg->msg);
			complete(&msg->done);
		}
		spin_unlock_irqrestore(&chan->rx_lock, flags);
	}
}

struct mbox_message *get_mbox_message(struct ipc_chan *chan)
{
	struct message_list *msg;

	mutex_lock(&chan->xfer_lock);
	if (list_empty(&chan->msg_list)) {
		mutex_unlock(&chan->xfer_lock);
		return NULL;
	}
	msg = list_first_entry(&chan->msg_list, struct message_list, list_node);
	list_del(&msg->list_node);
	mutex_unlock(&chan->xfer_lock);
	return &(msg->msg);
}

void put_mbox_message(struct ipc_chan *chan, struct mbox_message *msg)
{
	struct message_list *msg_list =
		 container_of(msg, struct message_list, msg);

	mutex_lock(&chan->xfer_lock);
	list_add_tail(&msg_list->list_node, &chan->msg_list);
	mutex_unlock(&chan->xfer_lock);
}

void cb1(void *message)
{
	pr_info(" %s\n", __func__);
};

void cb2(void *message)
{
	pr_info(" %s\n", __func__);
};

int prepare_message(struct ipc_context *ipc, struct ipc_chan *chan,
	    struct mbox_message *mbox_msg, struct ipc_message *msg)
{
	unsigned long flags;
	struct message_list *msg_list =
		 container_of(mbox_msg, struct message_list, msg);

	memcpy((void *)mbox_msg->msg_body,
		(void *)msg->payload, msg->length*sizeof(u32));
	spin_lock(&ipc->seq_lock);
	mbox_msg->message_id = ((msg->length << MSG_LEN_SHIFT) & MSG_LEN_MASK)
	  | ((ipc->seq << MSG_SEQNUM_SHIFT) & MSG_SEQNUM_MASK)
	  | ((msg->message_id << MSG_MESSAGE_ID_SHIFT) & MSG_MESSAGE_ID_MASK);
	ipc->seq++;
	spin_unlock(&ipc->seq_lock);
	if (chan->sync) {
		spin_lock_irqsave(&chan->rx_lock, flags);
		list_add_tail(&msg_list->list_node, &chan->rx_pending);
		spin_unlock_irqrestore(&chan->rx_lock, flags);
	}
	return 0;
}

struct ipc_chan *message_to_mailbox(struct ipc_context *ipc,
				    struct ipc_message *msg)
{
	struct ipc_chan *chan = NULL;

	switch (msg->dest) {
	case X86:
	case CVIP:
		chan = ipc->channels + msg->channel_idx;
		break;
	case ISP:
		chan = ipc->channels + ISP_CHANNEL_BASE + msg->channel_idx;
		break;
	}
	if (!chan) {
		pr_info(" %s chan is NULL\n", __func__);
		switch (msg->message_id) {
		case ALIVE:
		case PANIC:
			chan = ipc->channels + msg->channel_idx;
			break;
		default:
			break;
		}
	}
	return chan;
}

void x86_mbx_ringCallback(struct mbox_client *client, void *message)
{
}

void cvip_mbx_ringCallback(struct mbox_client *client, void *message)
{
}

void x86_mbx_rxCallback(struct mbox_client *client, void *message)
{
}

void isp_rx_callback(struct mbox_client *client, void *message)
{
	struct ipc_context *ipc;

	ipc = dev_get_drvdata(client->dev);
	pr_debug(" %s: received data from SMU mailbox\n", __func__);
	/* todo PM state notification */
}

void cvip_mb1_rx_callback(struct mbox_client *client, void *message)
{
	struct ipc_context *ipc;

	pr_info(" %s: callback triggered!!\n", __func__);
	ipc = dev_get_drvdata(client->dev);
	enqueue(message, ipc);
	wake_up_process(ipc->rx_worker_thread);
}

void cvip_mb2_tx_callback(struct mbox_client *client, void *message)
{
}

void x86_rx_callback(struct mbox_client *client, void *message)
{
	struct ipc_context *ipc;

	ipc = dev_get_drvdata(client->dev);
	enqueue(message, ipc);
	wake_up_process(ipc->rx_worker_thread);
}

void x86_mb1_tx_callback(struct mbox_client *client, void *message)
{
}

void x86_mb2_rx_callback(struct mbox_client *client, void *message)
{
	struct ipc_context *ipc;
	struct mbox_message *msg = message;

	/* no need to send out msg for INTR test */
	if ((msg->message_id & MSG_MESSAGE_ID_MASK) == INTR_TEST)
		return;

	ipc = dev_get_drvdata(client->dev);
	enqueue(message, ipc);
	wake_up_process(ipc->rx_worker_thread);
}

void cvip_mb3_rx_callback(struct mbox_client *client, void *message)
{
	struct ipc_context *ipc;

	pr_debug("%s: callback triggered!!\n", __func__);

	ipc = dev_get_drvdata(client->dev);
	enqueue(message, ipc);
	wake_up_process(ipc->rx_worker_thread);
}

void x86_mb3_tx_callback(struct mbox_client *client, void *message)
{
}

void dump_cblist(struct ipc_context *ipc)
{
	struct callback_list *node;

	list_for_each_entry(node, &ipc->cb_list_head, cb_list) {
		pr_info(" %s: callback list start = 0x%x, end = 0x%x\n",
			 __func__, node->range.start, node->range.end);
	}
}

int handle_client_callback(struct ipc_context *ipc, struct mbox_message *msg)
{
	struct callback_list *node;
	int ret = 0;
	client_callback_t *cb = NULL;
	u32 msg_id = msg->message_id & MSG_MESSAGE_ID_MASK;

	list_for_each_entry(node, &ipc->cb_list_head, cb_list) {
		if ((msg_id >= node->range.start)
		   && (msg_id <= node->range.end)) {
			cb = node->cb_func;
			break;
		}
		if (msg_id < node->range.start)
			break;
	}

	if (cb)
		cb(msg);
	return ret;
}

static struct ipc_context *g_ipc;

int handle_panic(struct mbox_message *msg)
{
	struct ipc_buf coredump;

	pr_info("%s: buf_hi = 0x%x, buf_low = 0x%x, buf_size = 0x%x\n",
		__func__, msg->msg_body[0],
		msg->msg_body[1], msg->msg_body[2]);
	coredump.buf_addr[0] = msg->msg_body[0];
	coredump.buf_addr[1] = msg->msg_body[1];
	coredump.buf_size = msg->msg_body[2];

	cvip_handle_panic(&coredump);

	return 0;
}

int handle_cvip_s2idle_allowed(struct mbox_message *msg)
{
	if (msg->msg_body[1] == CVIP_ALLOW_S2IDLE) {
		pr_info("CVIP allow s2idle message received\n");
		cvip_set_allow_s2idle(true);
	} else {
		pr_info("CVIP block s2idle message received\n");
		cvip_set_allow_s2idle(false);
	}

	cvip_set_power_completion();

	return 0;
}

int handle_alive(struct mbox_message *msg)
{
	struct ipc_msg ipcmsg;

	pr_debug("%s reboot reason = 0x%x, watchdog counter = 0x%x\n",
		 __func__, msg->msg_body[0], msg->msg_body[1]);
	ipcmsg.msg.p.data[0] = msg->msg_body[0];
	ipcmsg.msg.p.data[1] = msg->msg_body[1];

	cvip_is_alive(&ipcmsg);

	return 0;
}

int handle_device_state(struct mbox_message *msg)
{
	enum cvip_event_type event;
	u32 mid = (msg->message_id) & MSG_MESSAGE_ID_MASK;
	int ret = 0;

	switch (mid) {
	case OFF_HEAD:
		pr_debug("receive offhead message.\n");
		event = off_head;
		break;
	case ON_HEAD:
		pr_debug("receive onhead message.\n");
		event = on_head;
		break;
	default:
		return  -EINVAL;
	}

	ret = cvip_device_state_uevent(event);

	return ret;
}

int handle_s2ram_req(struct mbox_message *msg)
{
	struct ipc_message pm_message = {0};
	int ret = 0;

	pr_info(" %s\n", __func__);
	ret = pm_suspend(PM_SUSPEND_MEM);
	if (ret) {
		pr_err("CVIP failed to enter s2ram\n");
		pm_message.dest = 0;
		pm_message.channel_idx = 1;
		pm_message.length = 1;
		pm_message.message_id = LOW_POWER_ACK;
		pm_message.payload[0] = CVIP_S3_FAIL;
		ipc_sendmsg(g_ipc, &pm_message);
	}
	return 0;
}

void handle_cvip_panic(void)
{
	cvip_update_panic_count();
}

void handle_reboot(struct mbox_message *msg)
{
	if (allow_reboot)
		kernel_restart(NULL);
	else
		pr_info("Remote reboot disabled\n");
}

int handle_s2ram_ack(struct mbox_message *msg)
{
	u32 delay;
	int ret = 0;

	delay = msg->msg_body[0];

	pr_info(" %s, CVIP power will be removed after %d msecs.\n", __func__, delay);
	return ret;
}

/* Work around to bring CVIP device back to D0 after x86
 * entered idle stae, so that PL320 interrupt can wake up x86.
 */
#define PMI_STATUS_dev1_func1	0x110149054
#define POWER_STATE_MASK	0x3U
#define POWER_STATE_D3		0x3
#define POWER_STATE_D0		0x0

#ifdef CONFIG_CVIP_IPC

static int handle_low_power(u32 mode)
{
	int ret = 0;
	struct ipc_message pm_message;
	struct smu_msg msg;

	switch (mode) {
	case X86_S0:
	{
		int ret = 0;

		pr_info(" %s: X86 is resuming...\n", __func__);
		smu_msg(&msg, CVSMC_MSG_cvipoffheadstateindication,
			OFFHEAD_EXIT);
		if (!smu_send_single_msg(&msg)) {
			pr_err("%s: fail to send smu msg\n", __func__);
			return -EBUSY;
		}

		/* Setting Q6clk to DPM3 rate, Can change the clk and freq */
		ret = dsp_clk_dpm_cntrl(Q6CLK_ID, DPM3);
		if (ret)
			pr_err("Failed to submit request for Q6 clk.\n");

		/* Setting C5clk to DPM3 rate, Can change the clk and freq */
		ret = dsp_clk_dpm_cntrl(C5CLK_ID, DPM3);
		if (ret)
			pr_err("Failed to submit request for C5 clk.\n");

		break;
	}
	case X86_IDLE:
	{
		int ret = 0;

		pr_info(" %s: X86 is entering idle...\n", __func__);

		pm_message.dest = 0;
		pm_message.channel_idx = 1;
		pm_message.length = 0;
		pm_message.message_id = LOW_POWER_ACK;
		if (ipc_sendmsg(g_ipc, &pm_message) < 0)
			pr_err(" %s: failed to send idle ack to x86\n", __func__);

		/* Setting Q6clk to DPM1 rate, Can change the clk and freq */
		ret = dsp_clk_dpm_cntrl(Q6CLK_ID, DPM1);
		if (ret)
			pr_err("Failed to submit request for Q6 clk.\n");

		/* Setting C5clk to DPM1 rate, Can change the clk and freq */
		ret = dsp_clk_dpm_cntrl(C5CLK_ID, DPM1);
		if (ret)
			pr_err("Failed to submit request for C5 clk.\n");

		smu_msg(&msg, CVSMC_MSG_cvipoffheadstateindication,
			OFFHEAD_ENTER);
		if (!smu_send_single_msg(&msg)) {
			pr_err("%s: fail to send smu msg\n", __func__);
			return -EBUSY;
		}
		break;
	}
	case X86_STANDBY:
		pr_info(" %s: X86 is entering standby...\n", __func__);
		break;
	case X86_S3:
		pr_info(" %s: X86 is entering s2ram...\n", __func__);
		ret = pm_suspend(PM_SUSPEND_MEM);
		if (ret) {
			pr_err("CVIP failed to enter s2ram\n");
			pm_message.dest = 0;
			pm_message.channel_idx = 1;
			pm_message.length = 1;
			pm_message.message_id = LOW_POWER_ACK;
			pm_message.payload[0] = CVIP_S3_FAIL;
			ipc_sendmsg(g_ipc, &pm_message);
		}
		break;
	default:
		ret = -EINVAL;
	}

	return ret;
}
#endif

int handle_pm_event(struct mbox_message *msg)
{
	int ret = 0;
	u32 event, power_state;

	pr_debug(" %s\n", __func__);
	event = msg->msg_body[0];
	power_state = msg->msg_body[1];

	pr_info("PM IPC msg received: %X, %X\n", event, power_state);

	switch (event) {
	case DEVICE_STATE:
		pr_debug(" %s: Device state message received\n", __func__);
		break;
	case CVIP_QUERY_S2IDLE:
		pr_debug("Query s2idle pl320 ack message received\n");
		break;
	case CVIP_QUERY_S2IDLE_ACK:
		pr_debug("Query s2idle cvip ack message received\n");
		handle_cvip_s2idle_allowed(msg);
		break;
#ifdef CONFIG_CVIP_IPC
	case LOW_POWER_MODE:
		pr_debug(" %s: Low power mode message received\n", __func__);
		ret = handle_low_power(power_state);
		break;
#endif
	default:
		ret = -EINVAL;
	}

	return ret;
}

int handle_low_power_ack(struct mbox_message *msg)
{
	u32 state = 0;

	pr_info(" %s\n", __func__);
	state = msg->msg_body[0];
	cvip_low_power_ack(state);

	return 0;
}

int handle_intrtest(struct mbox_message *msg)
{
	++intr_test_count;

	if (intr_test_count >= 5000) {
		pr_debug("---> %d intr hit\n", msg->msg_body[0]);
		intr_test_count = 0;
	}

	return 0;
}

#ifdef CONFIG_IPC_XPORT
int handle_install_dump_kernel(struct mbox_message *msg)
{
	u32 dump_size;
	struct dma_buf *dma_buf;
	struct sg_table *sg;
	phys_addr_t paddr;
	char *argv[4];
	static const char * const envp[] = {"HOME=/", "TERM=linux", "PATH=/sbin:/bin:/usr/sbin:/usr/bin", NULL};
	char cmd_string[128] = {0};
	int ret = 0;

	pr_info(" %s\n", __func__);
	dump_size = msg->msg_body[0];

	alloc_fd = ion_alloc(dump_size, 1 << ION_HEAP_X86_CVIP_SHARED_ID,
			     ION_FLAG_CACHED);
	if (alloc_fd < 0) {
		pr_err("Failed to allocate ion buffer for dump file\n");
		return -ENOMEM;
	}

	dma_buf = dma_buf_get(alloc_fd);
	if (IS_ERR(dma_buf)) {
		pr_err("%s: failed to get dma buffer\n", __func__);
		return -EFAULT;
	}

	sg = ion_sg_table(dma_buf);
	if (IS_ERR(sg)) {
		pr_err("%s: failed to get sg_table\n", __func__);
		dma_buf_put(dma_buf);
		return -EFAULT;
	}

	paddr = page_to_phys(sg_page(sg->sgl));

	snprintf(cmd_string, 128, "kdump-prepare.sh start %pa\n", &paddr);
	pr_info("cmd_string = %s\n", cmd_string);
	dma_buf_put(dma_buf);

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

	return 0;
}

static int uninstall_dump_kernel(void)
{
	char *argv[4];
	static const char * const envp[] = {"HOME=/", "TERM=linux",
					    "PATH=/sbin:/bin:/usr/sbin:/usr/bin",
					     NULL};
	char cmd_string[128] = {0};
	int ret = 0;

	if (alloc_fd < 0) {
		pr_err("Ion buffer for dump file doesn't exist\n");
		return -EFAULT;
	}

	snprintf(cmd_string, sizeof(cmd_string) - 1,
		 "kdump-prepare.sh stop\n");
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

	return 0;
}
#endif

#ifdef CONFIG_CVIP_IPC
#define IPC_EFD_SIGNAL_OK	1
#define IPC_EFD_SIGNAL_ERR	2

static void handle_panic_ack(struct mbox_message *msg)
{
	pr_debug("%s\n", __func__);

	if (msg->msg_body[1] == IPC_X86_SAVE_DATA_ERR)
		ipc_eventfd_signal(IPC_EFD_SIGNAL_ERR);
	else
		ipc_eventfd_signal(IPC_EFD_SIGNAL_OK);
}
#endif

static int handle_set_gpuva(struct mbox_message *msg)
{
	gpuva = msg->msg_body[0];
	gpuva <<= 32;
	gpuva += msg->msg_body[1];

	x86_buf_base = msg->msg_body[2];
	x86_buf_base <<= 32;
	x86_buf_base += msg->msg_body[3];

	pr_debug("%s: gpuva=%llx, base=%llx\n", __func__, gpuva, x86_buf_base);

	return 0;
}

int handle_message(struct mbox_message *msg)
{
	int ret = 0;
	u32 mid = (msg->message_id)&MSG_MESSAGE_ID_MASK;

	if (!g_ipc) {
		ipc_init(&g_ipc);
		if (IS_ERR_OR_NULL(g_ipc))
			return -ENODEV;
	}

	switch (mid) {
	case ALIVE:
		ret = handle_alive(msg);
		break;
	case PANIC:
		ret = handle_panic(msg);
		break;
	case S2RAM_REQ:
		ret = handle_s2ram_req(msg);
		break;
	case S2RAM_ACK:
		ret = handle_s2ram_ack(msg);
		break;
	case SEND_PM_EVENT:
		ret = handle_pm_event(msg);
		break;
	case LOW_POWER_ACK:
		ret = handle_low_power_ack(msg);
		break;
	case INTR_TEST:
		ret = handle_intrtest(msg);
		break;
	case OFF_HEAD:
		ret = handle_device_state(msg);
		break;
	case ON_HEAD:
		ret = handle_device_state(msg);
		break;
#ifdef CONFIG_IPC_XPORT
	case INSTALL_DUMP_KERNEL:
		ret = handle_install_dump_kernel(msg);
		break;
#endif
#ifdef CONFIG_CVIP_IPC
	case PANIC_ACK:
		handle_panic_ack(msg);
		break;
	case REBOOT_REQ:
		handle_reboot(msg);
		break;
#endif
	case CVIP_PANIC:
		handle_cvip_panic();
		break;
	case SET_GPUVA:
		ret = handle_set_gpuva(msg);
		break;
	default:
		ret = -EINVAL;
	}
	return ret;
}
#ifdef CONFIG_IPC_XPORT
void handle_xport_req(void *message)
{
	struct mbox_message *msg;
	u32 msg_id;
	int ret;

	msg = (struct mbox_message *)message;
	msg_id = (msg->message_id) & MSG_MESSAGE_ID_MASK;

	switch (msg_id) {
	case REGISTER_BUFFER:
	{
		phys_addr_t xport_heap_paddr;
		size_t xport_heap_size;

		xport_heap_paddr  = msg->msg_body[0];
		xport_heap_paddr <<= 32;
		xport_heap_paddr += msg->msg_body[1];
		/* shift address to x-interface aperture */
		xport_heap_paddr += MERO_CVIP_ADDR_XPORT_BASE;
		xport_heap_size   = msg->msg_body[2];

		pr_debug("%s: xport_heap_paddr %pa, size %zx\n",
			 __func__, &xport_heap_paddr, xport_heap_size);
		mero_ion_heap_xport_create(xport_heap_paddr, xport_heap_size);
		break;
	}
	case UNREGISTER_BUFFER:
		pr_debug("%s: unregister xport heap\n", __func__);
		ret = uninstall_dump_kernel();
		if (ret < 0) {
			pr_err("Failed to uninstall dump kernel\n");
			break;
		}
		if (alloc_fd >= 0) {
			ksys_close(alloc_fd);
			/* Sleep to allow ksys_close to complete
			 * and free the memory before we call destroy.
			 */
			msleep(200);
			alloc_fd = -1;
		}
		mero_ion_heap_xport_destroy();
		break;
	default:
		pr_debug("%s: invalid msg_id!\n", __func__);
		break;
	}
}
#endif
int rx_worker_thread_func(void *data)
{
	struct ipc_context *instance = data;
	struct mbox_message *msg;
	struct rx_queue *node;
	int ret = 0;

	complete(&instance->thread_started);
	set_current_state(TASK_INTERRUPTIBLE);
	pr_info("rx worker thread started\n");

	while (!kthread_should_stop()) {
		schedule();
		msg = dequeue(instance);
		set_current_state(TASK_RUNNING);
		while (msg) {
			handle_message(msg);
			handle_client_callback(instance, msg);
			log_message(msg);
			node = to_rx_queue(msg);
			kfree(node);
			msg = dequeue(instance);
		};
		/* back to sleep */
		set_current_state(TASK_INTERRUPTIBLE);
	}
	pr_info("rx worker thread exit\n");
	complete_and_exit(&instance->thread_exited, ret);
}

int rx_ring_worker_thread_func(void *data)
{
	return 0;
}

#ifdef CONFIG_CVIP_IPC
/* special case for android when config pre/post memory layout
 * utilize pre/post memory to pass configuration of in/out image
 */
static void set_ioimg_fmt(struct isp_ipc_device *dev, int cam_id)
{
	struct isp_drv_to_cvip_ctl *p =
		(struct isp_drv_to_cvip_ctl *)dev->camera[cam_id].data_base;
	struct camera *cam = &dev->camera[cam_id];

	if (p->type == TYPE_UBUNTU) {
		return;
	} else if (p->type == TYPE_ANDROID) {
		cam->isp_ioimg_setting = *p;
	} else if (p->type == TYPE_NONE) {
		/* already configured in ubuntu env via LTP */
		if (cam->isp_ioimg_setting.type == TYPE_UBUNTU)
			return;

		pr_warn("%s: os type is none\n", __func__);
		/* set default values */
		cam->isp_ioimg_setting.raw_fmt = RAW_PKT_FMT_4;
		cam->isp_ioimg_setting.raw_slice_num = 1;
		cam->isp_ioimg_setting.output_slice_num = 1;
		cam->isp_ioimg_setting.output_width = 1920;
		cam->isp_ioimg_setting.output_height = 1080;
		cam->isp_ioimg_setting.output_fmt = IMAGE_FORMAT_NV12;
		cam->outslice.slice_num = 1;
		cam->outslice.slice_height[SLICE_FIRST] = 1080;
		return;
	}

	pr_debug("isp_drv_to_cvip_ctl=(%u,%u,%u,%u,%u,%u)\n",
		 p->raw_fmt, p->raw_slice_num, p->output_slice_num,
		 p->output_width, p->output_height, p->output_fmt);
}

static void unset_ioimg_fmt(struct isp_ipc_device *dev, int cam_id)
{
	/* ubuntu case is handled via LTP, so bypass it here */
	if (dev->camera[cam_id].isp_ioimg_setting.type == TYPE_UBUNTU)
		return;

	dev->camera[cam_id].isp_ioimg_setting.type = TYPE_NONE;
}

static int get_prepost_mem(struct isp_ipc_device *dev, int cam_id)
{
	struct camera *cam = &dev->camera[cam_id];
	struct reg_cam_ctrl_info *info = &cam->cam_ctrl_info;
	u64 paddr;

	if (!gpuva || !x86_buf_base)
		return -1;

	paddr = ((u64)info->prepost_hi << 32) | info->prepost_lo;
	paddr = ISP2CVIPADDR(x86_buf_base) + paddr - gpuva;
	cam->data_base =
		devm_memremap(dev->dev, paddr, info->prepost_size, MEMREMAP_WB);
	if (IS_ERR(cam->data_base)) {
		dev_err(dev->dev, "failed to map prepost addr %llx\n", paddr);
		cam->data_base = NULL;
		info->prepost_hi = 0;
		info->prepost_lo = 0;
		info->prepost_size = 0;
		return -1;
	}

	cam->data_buf_offset = paddr;
	cam->data_buf_size = info->prepost_size;

	dev_dbg(dev->dev, "%s: pa:%llx, size:%x\n",
		__func__, paddr, info->prepost_size);

	set_ioimg_fmt(dev, cam_id);

	return 0;
}

static void put_prepost_mem(struct isp_ipc_device *dev, int cam_id)
{
	struct camera *cam = &dev->camera[cam_id];
	struct reg_cam_ctrl_info *info = &cam->cam_ctrl_info;

	if (cam->data_base)
		devm_memunmap(dev->dev, cam->data_base);
	cam->data_base = NULL;
	info->prepost_hi = 0;
	info->prepost_lo = 0;
	info->prepost_size = 0;
	cam->data_buf_offset = 0;
	cam->data_buf_size = 0;

	unset_ioimg_fmt(dev, cam_id);
}

static int get_stats_mem(struct isp_ipc_device *dev, int cam_id)
{
	struct camera *cam = &dev->camera[cam_id];
	struct reg_cam_ctrl_info *info = &cam->cam_ctrl_info;
	u64 paddr;

	if (!gpuva || !x86_buf_base)
		return -1;

	paddr = ((u64)info->stats_hi << 32) | info->stats_lo;
	paddr = ISP2CVIPADDR(x86_buf_base) + paddr - gpuva;
	cam->stats_base =
		devm_memremap(dev->dev, paddr, info->stats_size, MEMREMAP_WB);
	if (IS_ERR(cam->stats_base)) {
		dev_err(dev->dev, "failed to map stats addr %llx\n", paddr);
		cam->stats_base = NULL;
		info->stats_hi = 0;
		info->stats_lo = 0;
		info->stats_size = 0;
		return -1;
	}

	cam->stats_buf_offset = paddr;
	cam->stats_buf_size = info->stats_size;

	dev_dbg(dev->dev, "%s: pa:%llx, size:%x\n",
		__func__, paddr, info->stats_size);

	return 0;
}

static void put_stats_mem(struct isp_ipc_device *dev, int cam_id)
{
	struct camera *cam = &dev->camera[cam_id];
	struct reg_cam_ctrl_info *info = &cam->cam_ctrl_info;

	if (cam->stats_base)
		devm_memunmap(dev->dev, cam->stats_base);
	cam->stats_base = NULL;
	info->stats_hi = 0;
	info->stats_lo = 0;
	info->stats_size = 0;
	cam->stats_buf_offset = 0;
	cam->stats_buf_size = 0;
}

static int
calculate_oslice(struct isp_ipc_device *dev, int cam_id, u32 frame_id)
{
	struct camera *cam = &dev->camera[cam_id];
	struct reg_cam_ctrl_info *info = &cam->cam_ctrl_info;
	bool do_cal = false;

	pr_debug("%s, transition type %d\n", __func__, info->oslice_sync.tran);

	switch (info->oslice_sync.tran) {
	case TRAN_SLICE_INFO:
		/* happens before stream on, in cam ctrl irq */
		if (frame_id == FRAMEID_INVALID)
			do_cal = true;
		break;
	case TRAN_OUTPUT:
	case TRAN_BOTH:
		/* happens during streaming, do real handling in outbuf irq */
		if (frame_id != FRAMEID_INVALID &&
		    frame_id >= info->oslice_sync.frame)
			do_cal = true;
		break;
	default:
		break;
	}

	if (do_cal) {
		/* reset transition type to avoid re-cal */
		info->oslice_sync.tran = TRAN_UNTOUCHED;
		if (calculate_output_slice_info(cam) < 0) {
			pr_err("%s: calculate oslice info error\n", __func__);
			return -1;
		}
		isp_setup_output_rings(cam);
	}

	return 0;
}

static int change_af_lens_range(struct isp_ipc_device *dev, int cam_id)
{
	struct camera *cam = &dev->camera[cam_id];
	struct reg_cam_ctrl_info *info = &cam->cam_ctrl_info;

	isp_data_u2f(info->af_p1.far,
		     &cam->pre_data.afmetadata.distancefar);
	isp_data_u2f(info->af_p2.near,
		     &cam->pre_data.afmetadata.distancenear);
	return 0;
}

static int change_af_mode(struct isp_ipc_device *dev, int cam_id)
{
	struct camera *cam = &dev->camera[cam_id];
	struct reg_cam_ctrl_info *info = &cam->cam_ctrl_info;

	switch (info->af_p0.af_mode) {
	case CVIP_AF_CONTROL_MODE_MASK_AUTO:
		cam->pre_data.afmetadata.mode = CVIP_AF_CONTROL_MODE_AUTO;
		break;
	case CVIP_AF_CONTROL_MODE_MASK_MACRO:
		cam->pre_data.afmetadata.mode = CVIP_AF_CONTROL_MODE_MACRO;
		break;
	case CVIP_AF_CONTROL_MODE_MASK_CONT_VIDEO:
		cam->pre_data.afmetadata.mode =
			CVIP_AF_CONTROL_MODE_CONTINUOUS_VIDEO;
		break;
	case CVIP_AF_CONTROL_MODE_MASK_CONT_PICTURE:
		cam->pre_data.afmetadata.mode =
			CVIP_AF_CONTROL_MODE_CONTINUOUS_PICTURE;
		break;
	case CVIP_AF_CONTROL_MODE_MASK_OFF:
	default:
		cam->pre_data.afmetadata.mode = CVIP_AF_CONTROL_MODE_OFF;
		break;
	}

	return 0;
}

static int change_af_trigger(struct isp_ipc_device *dev, int cam_id)
{
	struct camera *cam = &dev->camera[cam_id];
	struct reg_cam_ctrl_info *info = &cam->cam_ctrl_info;

	cam->pre_data.afmetadata.trigger = info->af_p0.af_trigger;
	return 0;
}

static void
camera_control_notification(struct isp_ipc_device *dev, int cam_id, void *resp)
{
	struct reg_cam_ctrl_info *pdata = (struct reg_cam_ctrl_info *)resp;
	struct camera *cam = &dev->camera[cam_id];
	struct reg_cam_ctrl_info *sdata = &cam->cam_ctrl_info;
	struct reg_cam_ctrl_resp *sresp = &cam->cam_ctrl_resp;
	enum reg_stream_status cur_mode, req_mode;
	bool req_stream_on = false;
	bool req_resume = false;
	bool req_stream_off = false;
	bool req_stream_stdby = false;
	bool req_again0_wr = false;

	log_payload("CAM ctrl", (u32 *)pdata,
		    sizeof(struct reg_cam_ctrl_info) / sizeof(u32));

	/* cvip will subsequently respond to isp fw with the saved request id */
	sdata->req_id = pdata->req_id;
	sresp->resp_id = pdata->req_id;

	sdata->req.response = pdata->req.response;

	if (pdata->req.stream_op == REQ_OP) {
		if (pdata->req.stream_rw == REQ_WRITE) {
			cur_mode = sdata->strm.stream;
			req_mode = pdata->strm.stream;
			if (cur_mode != STREAM_ON && req_mode == STREAM_ON) {
				req_stream_on = true;
			} else if (cur_mode != STREAM_OFF &&
				   req_mode == STREAM_OFF) {
				req_stream_off = true;
			} else if (req_mode == STREAM_STDBY) {
				req_stream_stdby = true;
				if (cur_mode == STREAM_ON)
					req_resume = true;
				CVIP_EVTLOG(dev->evlog_idx,
					    "request standby, resume [1]",
					    req_resume);
				sensor_simulation(dev, cam_id, STREAM_STDBY,
						  req_resume);
			}

			if (pdata->strm.stream != STREAM_UNTOUCHED) {
				CVIP_EVTLOG(dev->evlog_idx,
					    "stream_status [1] -> [2]",
					    sdata->strm.stream,
					    pdata->strm.stream);
				sdata->strm.stream = pdata->strm.stream;
				sresp->strm.stream =
					to_stream_resp(sdata->strm.stream);
			}

			if (pdata->strm.res != RES_UNTOUCHED) {
				CVIP_EVTLOG(dev->evlog_idx,
					    "resolution_mode [1] -> [2]",
					    sdata->strm.res,
					    pdata->strm.res);
				sdata->strm.res = pdata->strm.res;
				sresp->strm.res = to_res_resp(sdata->strm.res);
			}

			if (pdata->strm.hdr != HDR_UNTOUCHED) {
				CVIP_EVTLOG(dev->evlog_idx,
					    "hdr_mode [1] -> [2]",
					    sdata->strm.hdr,
					    pdata->strm.hdr);
				sdata->strm.hdr = pdata->strm.hdr;
				sresp->strm.hdr = to_hdr_resp(sdata->strm.hdr);
			}

			if (pdata->strm.tp != TP_UNTOUCHED) {
				CVIP_EVTLOG(dev->evlog_idx,
					    "test_pattern [1] -> [2]",
					    sdata->strm.tp,
					    pdata->strm.tp);
				sdata->strm.tp = pdata->strm.tp;
				sresp->strm.tp = to_tp_resp(sdata->strm.tp);
			}

			if (pdata->strm.bd != BD_UNTOUCHED) {
				CVIP_EVTLOG(dev->evlog_idx,
					    "bit_depth [1] -> [2]",
					    sdata->strm.bd,
					    pdata->strm.bd);
				sdata->strm.bd = pdata->strm.bd;
				sresp->strm.bd = to_bd_resp(pdata->strm.bd);
			}
		}
		sresp->resp.stream = RESP_SUCCEED;
	}
	if (pdata->req.res_op == REQ_OP) {
		if (pdata->req.res_rw == REQ_WRITE)
			sdata->res = pdata->res;
		sresp->res.value = sdata->res.value;
		sresp->resp.res = RESP_SUCCEED;
	}
	if (pdata->req.fpks_op == REQ_OP) {
		if (pdata->req.fpks_rw == REQ_WRITE)
			sdata->fpks = pdata->fpks;
		sresp->fpks = sdata->fpks;
		sresp->resp.fpks = RESP_SUCCEED;
	}
	if (pdata->req.again0_op == REQ_OP) {
		if (pdata->req.again0_rw == REQ_WRITE) {
			sdata->again0 = pdata->again0;
			req_again0_wr = true;
		}
		sresp->again0 = sdata->again0;
		sresp->resp.again0 = RESP_SUCCEED;
	}
	if (pdata->req.dgain0_op == REQ_OP) {
		if (pdata->req.dgain0_rw == REQ_WRITE)
			sdata->dgain0 = pdata->dgain0;
		sresp->dgain0 = sdata->dgain0;
		sresp->resp.dgain0 = RESP_SUCCEED;
	}
	if (pdata->req.itime0_op == REQ_OP) {
		if (pdata->req.itime0_rw == REQ_WRITE)
			sdata->itime0 = pdata->itime0;
		sresp->itime0 = sdata->itime0;
		sresp->resp.itime0 = RESP_SUCCEED;
	}
	if (pdata->req.info_sync_op == REQ_OP) {
		if (pdata->req.info_sync_rw == REQ_WRITE) {
			sdata->oslice_sync.value = pdata->oslice_sync.value;
			sdata->oslice_param.value = pdata->oslice_param.value;
			calculate_oslice(dev, cam_id, FRAMEID_INVALID);
		}
		sresp->oslice_sync.value = sdata->oslice_sync.value;
		sresp->oslice_param.value = sdata->oslice_param.value;
		sresp->resp.info_sync = RESP_SUCCEED;
	}
	if (pdata->req.af_lens_rng_op == REQ_OP) {
		if (pdata->req.af_lens_rng_rw == REQ_WRITE) {
			sdata->af_p1.value = pdata->af_p1.value;
			sdata->af_p2.value = pdata->af_p2.value;
			change_af_lens_range(dev, cam_id);
		}
		sresp->af_p1.value = sdata->af_p1.value;
		sresp->af_p2.value = sdata->af_p2.value;
		sresp->resp.af_lens_rng = RESP_SUCCEED;
	}
	if (pdata->req.lens_pos_op == REQ_OP) {
		if (pdata->req.lens_pos_rw == REQ_WRITE) {
			sdata->af_p0.lens_pos = pdata->af_p0.lens_pos;
			change_lens_pos(dev, cam_id);
		}
		sresp->af_p0.lens_pos = sdata->af_p0.lens_pos;
		sresp->resp.lens_pos = RESP_SUCCEED;
	}
	if (pdata->req.af_mode_op == REQ_OP) {
		if (pdata->req.af_mode_rw == REQ_WRITE) {
			CVIP_EVTLOG(dev->evlog_idx,
				    "AF mode [1] -> [2]",
				    sdata->af_p0.af_mode,
				    pdata->af_p0.af_mode);
			sdata->af_p0.af_mode = pdata->af_p0.af_mode;
			change_af_mode(dev, cam_id);
		}
		sresp->af_p0.af_mode = sdata->af_p0.af_mode;
		sresp->resp.af_mode = RESP_SUCCEED;
	}
	if (pdata->req.frame_duration_op == REQ_OP) {
		if (pdata->req.frame_duration_rw == REQ_WRITE)
			sdata->frame_duration = pdata->frame_duration;
		sresp->frame_duration = sdata->frame_duration;
		sresp->resp.frame_duration = RESP_SUCCEED;
	}
	if (pdata->req.prepost_mem_op == REQ_OP) {
		if (pdata->req.prepost_mem_rw == REQ_WRITE) {
			CVIP_EVTLOG(dev->evlog_idx,
				    "set ppmem to [1],[2],[3]",
				    pdata->prepost_hi, pdata->prepost_lo,
				    pdata->prepost_size);
			sdata->prepost_hi = pdata->prepost_hi;
			sdata->prepost_lo = pdata->prepost_lo;
			sdata->prepost_size = pdata->prepost_size;
			if (get_prepost_mem(dev, cam_id) < 0) {
				/* return succeed, since we focus much more on
				 * if cv can receive the interrupt or not
				 */
				sresp->resp.prepost_mem = RESP_SUCCEED;
				put_prepost_mem(dev, cam_id);
			} else {
				sresp->resp.prepost_mem = RESP_SUCCEED;
			}
		} else {
			if (!sdata->prepost_size)
				sresp->resp.prepost_mem = RESP_SUCCEED;
			else
				sresp->resp.prepost_mem = RESP_SUCCEED;
		}
	}
	if (pdata->req.stats_mem_op == REQ_OP) {
		if (pdata->req.stats_mem_rw == REQ_WRITE) {
			CVIP_EVTLOG(dev->evlog_idx,
				    "set statsmem to [1],[2],[3]",
				    pdata->stats_hi, pdata->stats_lo,
				    pdata->stats_size);
			sdata->stats_hi = pdata->stats_hi;
			sdata->stats_lo = pdata->stats_lo;
			sdata->stats_size = pdata->stats_size;
			if (get_stats_mem(dev, cam_id) < 0) {
				/* return succeed, since we focus much more on
				 * if cv can receive the interrupt or not
				 */
				sresp->resp.stats_mem = RESP_SUCCEED;
				put_stats_mem(dev, cam_id);
			} else {
				sresp->resp.stats_mem = RESP_SUCCEED;
			}
		} else {
			if (!sdata->stats_size)
				sresp->resp.stats_mem = RESP_SUCCEED;
			else
				sresp->resp.stats_mem = RESP_SUCCEED;
		}
	}
	if (pdata->req.af_trig_op == REQ_OP) {
		if (pdata->req.af_trig_rw == REQ_WRITE) {
			CVIP_EVTLOG(dev->evlog_idx,
				    "AF Trigger [1] -> [2]",
				    sdata->af_p0.af_trigger,
				    pdata->af_p0.af_trigger);
			sdata->af_p0.af_trigger = pdata->af_p0.af_trigger;
			change_af_trigger(dev, cam_id);
		}
		sresp->af_p0.af_trigger = sdata->af_p0.af_trigger;
		sresp->resp.af_trig = RESP_SUCCEED;
	}

	if (req_stream_on) {
		CVIP_EVTLOG(dev->evlog_idx, "request stream on");
		sensor_simulation(dev, cam_id, STREAM_ON, req_resume);
	}

	if (req_stream_off) {
		CVIP_EVTLOG(dev->evlog_idx, "request stream off");
		sensor_simulation(dev, cam_id, STREAM_OFF, req_resume);
		put_prepost_mem(dev, cam_id);
		put_stats_mem(dev, cam_id);
		isp_reset_output_rings(cam);
	}

	dev_dbg(dev->dev, "Camera control: test_cond=0x%lx, req=%s\n",
		cam->test_condition,
		req_stream_on ? "on" :
		req_stream_off ? "off" :
		req_stream_stdby ? "standby" : "NA");
	if (req_stream_on &&
	    test_and_clear_bit(ISP_COND_WAIT_STREAMON, &cam->test_condition)) {
		wake_up_interruptible(&cam->test_wq);
	} else if (req_stream_off &&
		   test_and_clear_bit(ISP_COND_WAIT_STREAMOFF,
				      &cam->test_condition)) {
		wake_up_interruptible(&cam->test_wq);
	} else if (req_again0_wr &&
		   test_and_clear_bit(ISP_COND_WAIT_AGAIN0,
				      &cam->test_condition)) {
		wake_up_interruptible(&cam->test_wq);
	}
}

static void
rgb_stat_notification(struct isp_ipc_device *dev, int cam_id, void *resp)
{
	struct reg_stats_buf *pdata = (struct reg_stats_buf *)resp;
	u8 *src_rgb, *dest_rgb, *af_roi;
	u32 size_rgb;
	int frame_id = pdata->id.frame;
	u8 idx = 0;
	struct isp_output_frame *frame;
	struct camera *cam = &dev->camera[cam_id];

	log_payload("RGB stat", (u32 *)pdata,
		    sizeof(struct reg_stats_buf) / sizeof(u32));

	dev_dbg(dev->dev, "RGB stat: frame_id=%d, lock_stats_id=%d\n",
		frame_id, cam->test_dqbuf_resp.lock_stats_id);
	idx = frame_id % MAX_OUTPUT_FRAME;
	frame = &cam->output_frames[idx];
	frame->stat_type = RGBSTATS;

	/* locked, don't override */
	if (cam->test_dqbuf_resp.lock_stats_id >= 0)
		goto out;
	if (!cam->out_stats_base  || !cam->stats_base) {
		dev_dbg(dev->dev, "stats_base is not available.\n");
		goto out;
	}

	/* statistic info data: |AaaStatsData | afdata | */
	af_roi = cam->stats_base + pdata->addr + STATS_SIZE;
	memcpy(&cam->pre_data.afmetadata.cvipafroi, af_roi, AFROI_SIZE);
	__flush_dcache_area(af_roi, AFROI_SIZE);

	src_rgb = cam->stats_base + pdata->addr;
	dest_rgb = cam->out_stats_base + frame->stat_addr;
	size_rgb = frame->stat_size;
	dev_dbg(dev->dev, "RGB STATS Stream:src=0x%llx,dest=0x%llx,size=0x%x\n",
		(u64)src_rgb, (u64)dest_rgb, size_rgb);
	if (cam->dmabuf_stats)
		dma_buf_begin_cpu_access(cam->dmabuf_stats, DMA_BIDIRECTIONAL);
	memcpy(dest_rgb, src_rgb, size_rgb);
	if (cam->dmabuf_stats)
		dma_buf_end_cpu_access(cam->dmabuf_stats, DMA_BIDIRECTIONAL);

out:
	cam->isp_stat_frame = frame_id;
	if (test_and_clear_bit(ISP_COND_DQBUF_WAIT_STATS,
			       &cam->test_condition)) {
		/* lock_stats_id == lock_frame_id ? */
		cam->test_dqbuf_resp.lock_stats_id = frame_id;
		wake_up_interruptible(&cam->test_wq);
	}
}

static void
ir_stat_notification(struct isp_ipc_device *dev, int cam_id, void *resp)
{
	struct reg_stats_buf *pdata = (struct reg_stats_buf *)resp;
	u8 *src_ir, *dest_ir;
	u32 size_ir;
	int frame_id = pdata->id.frame;
	u8 idx = 0;
	struct isp_output_frame *frame;
	struct camera *cam = &dev->camera[cam_id];

	log_payload("IR stat", (u32 *)pdata,
		    sizeof(struct reg_stats_buf) / sizeof(u32));

	idx = frame_id % MAX_OUTPUT_FRAME;
	frame = &cam->output_frames[idx];
	frame->stat_type = IRSTATS;

	/* locked, don't override */
	if (cam->test_dqbuf_resp.lock_stats_id >= 0)
		goto out;
	if (!cam->out_stats_base || !cam->stats_base)
		goto out;

	src_ir = cam->stats_base + pdata->addr;
	dest_ir = cam->out_stats_base + frame->stat_addr;
	size_ir = frame->stat_size;
	dev_dbg(dev->dev, "IR STATS Stream:src=0x%llx,dest=0x%llx,size=0x%x\n",
		(u64)src_ir, (u64)dest_ir, size_ir);
	if (cam->dmabuf_stats)
		dma_buf_begin_cpu_access(cam->dmabuf_stats, DMA_BIDIRECTIONAL);
	memcpy(dest_ir, src_ir, size_ir);
	if (cam->dmabuf_stats)
		dma_buf_end_cpu_access(cam->dmabuf_stats, DMA_BIDIRECTIONAL);

out:
	cam->isp_stat_frame = frame_id;
	if (test_and_clear_bit(ISP_COND_DQBUF_WAIT_STATS,
			       &cam->test_condition)) {
		/* lock_stats_id == lock_frame_id ? */
		cam->test_dqbuf_resp.lock_stats_id = frame_id;
		wake_up_interruptible(&cam->test_wq);
	}
}

static void input_pixel_buff_release_notification(struct isp_ipc_device *dev,
						  int cam_id, void *resp)
{
	struct reg_inbuf_release *pdata = (struct reg_inbuf_release *)resp;
	int frame_id = pdata->id.frame;
	int status = pdata->id.field;
	struct camera *cam = &dev->camera[cam_id];

	log_payload("Pixel input buff", (u32 *)pdata,
		    sizeof(struct reg_inbuf_release) / sizeof(u32));

	dev_dbg(dev->dev, "inbuf release, cond=0x%lx\n", cam->test_condition);
	if (status == FRAME_DROPPED) {
		dev_dbg(dev->dev, "input_pixel, frame %x drop by ISP!!\n",
			frame_id);
		if (frame_id == cam->terr_base.drop_frame &&
		    test_and_clear_bit(ISP_CERR_FW_TOUT_WAIT_DROP,
				       &cam->test_condition)) {
			wake_up_interruptible(&cam->test_wq);
		} else if (frame_id == cam->terr_base.drop_frame &&
			   test_and_clear_bit(ISP_CERR_QOS_TOUT_WAIT_DROP,
					      &cam->test_condition)) {
			cam->terr_base.drop_frame = frame_id;
			wake_up_interruptible(&cam->test_wq);
		} else if (frame_id == cam->terr_base.drop_frame &&
			   test_and_clear_bit(ISP_CERR_DROP_FRAME_WAIT,
					      &cam->test_condition)) {
			wake_up_interruptible(&cam->test_wq);
		} else if (frame_id == cam->terr_base.drop_frame &&
			   test_and_clear_bit(ISP_CERR_DISORDER_WAIT_DROP,
					      &cam->test_condition)) {
			wake_up_interruptible(&cam->test_wq);
		} else if (test_and_clear_bit(ISP_CERR_OVERFLOW_WAIT,
					      &cam->test_condition)) {
			wake_up_interruptible(&cam->test_wq);
		}
	} else {
		if (!test_bit(ISP_CERR_FW_TOUT_WAIT_DROP,
			      &cam->test_condition) &&
		    test_and_clear_bit(ISP_CERR_FW_TOUT_WAIT_RECOVER,
				       &cam->test_condition)) {
			/* recover should happen after drop */
			cam->terr_base.recover_frame = frame_id;
			wake_up_interruptible(&cam->test_wq);
		} else if (!test_bit(ISP_CERR_QOS_TOUT_WAIT_DROP,
				     &cam->test_condition) &&
			   test_and_clear_bit(ISP_CERR_QOS_TOUT_WAIT_RECOVER,
					      &cam->test_condition)) {
			/* recover should happen after drop */
			cam->terr_base.recover_frame = frame_id;
			wake_up_interruptible(&cam->test_wq);
		} else if (!test_bit(ISP_CERR_DISORDER_WAIT_DROP,
				     &cam->test_condition) &&
			   test_and_clear_bit(ISP_CERR_DISORDER_WAIT_RECOVER,
					      &cam->test_condition)) {
			/* recover should happen after drop */
			cam->terr_base.recover_frame = frame_id;
			wake_up_interruptible(&cam->test_wq);
		}
	}

	/* only need to update the running frame */
	cam->isp_input_frame = frame_id;
}

static void pre_data_buff_buff_release_notification(struct isp_ipc_device *dev,
						    int cam_id, void *resp)
{
	struct reg_prepost_release *pdata = (struct reg_prepost_release *)resp;
	int frame_id = pdata->id.frame;
	int status = pdata->id.field;
	struct camera *cam = &dev->camera[cam_id];

	log_payload("Pre data", (u32 *)pdata,
		    sizeof(struct reg_prepost_release) / sizeof(u32));
	if (status == FRAME_DROPPED)
		dev_dbg(dev->dev, "pre_data, frame %x drop by ISP!!\n",
			frame_id);

	/* running idx for predata */
	cam->isp_predata_frame = frame_id;
	if (test_and_clear_bit(ISP_COND_PREPOST_WAIT_PRE,
			       &cam->test_condition))
		wake_up_interruptible(&cam->test_wq);
}

static void post_data_buff_buff_release_notification(struct isp_ipc_device *dev,
						     int cam_id, void *resp)
{
	struct reg_prepost_release *pdata = (struct reg_prepost_release *)resp;
	int frame_id = pdata->id.frame;
	int status = pdata->id.field;
	struct camera *cam = &dev->camera[cam_id];

	log_payload("Post data", (u32 *)pdata,
		    sizeof(struct reg_prepost_release) / sizeof(u32));
	if (status == FRAME_DROPPED)
		dev_dbg(dev->dev, "post_data, frame %x drop by ISP!!\n",
			frame_id);

	/* running idx for postdata */
	cam->isp_postdata_frame = frame_id;
	if (test_and_clear_bit(ISP_COND_PREPOST_WAIT_POST,
			       &cam->test_condition))
		wake_up_interruptible(&cam->test_wq);
}

static inline u32
cal_offset(u8 cur_idx, u32 size[SLICE_MAX])
{
	if (!cur_idx)
		return 0;

	return (size[SLICE_FIRST] + size[SLICE_MID] * (cur_idx - 1));
}

static inline u32
cal_size(u8 start_idx, u8 stop_idx, u8 last_idx, u32 size[SLICE_MAX])
{
	u32 ret;

	if (!stop_idx)
		return size[SLICE_FIRST];

	ret = (stop_idx == last_idx) ? size[SLICE_LAST] : size[SLICE_MID];
	ret += cal_offset(stop_idx, size) - cal_offset(start_idx, size);

	return ret;
}

static void output_yuv_stream_notification(struct isp_ipc_device *dev,
					   int cam_id, void *resp)
{
	struct reg_outbuf *pdata = (struct reg_outbuf *)resp;
	u8 *src_y, *src_u, *src_v;
	u8 *dest_y, *dest_u, *dest_v;
	u32 size_y, size_uv;
	u32 y_offset, uv_offset;
	int frame_id = pdata->id.frame;
	int slice = pdata->id.field;
	u8 idx = 0;
	struct isp_output_frame *frame;
	struct camera *cam = &dev->camera[cam_id];

	log_payload("YUV output stream", (u32 *)pdata,
		    sizeof(struct reg_outbuf) / sizeof(u32));

	/* copy slice to stream yuv outbuffer, output ring buffer */
	idx = frame_id % MAX_OUTPUT_FRAME;
	frame = &cam->output_frames[idx];
	idx = frame->total_slices - 1;
	if (frame->total_slices < 1) {
		dev_warn(dev->dev, "YUV out: output rings not configured\n");
		return;
	}
	calculate_oslice(dev, cam_id, frame_id);

	dev_dbg(dev->dev, "YUV out: frame_id=%d, slice=%d, lock_frame_id=%d\n",
		frame_id, slice, cam->test_dqbuf_resp.lock_frame_id);

	if (slice == 0 &&
	    test_and_clear_bit(ISP_COND_QBUF_WAIT, &cam->test_condition)) {
		cam->test_dqbuf_resp.lock_frame_id = FRAMEID_INVALID;
		cam->test_dqbuf_resp.lock_stats_id = FRAMEID_INVALID;
		wake_up_interruptible(&cam->test_wq);
	}

	/* locked, don't override */
	if (cam->test_dqbuf_resp.lock_frame_id >= 0)
		goto out;
	if (!cam->out_frame_base || !cam->output_base)
		goto out;

	if (cam->dmabuf_frame)
		dma_buf_begin_cpu_access(cam->dmabuf_frame, DMA_BIDIRECTIONAL);
	switch (cam->isp_ioimg_setting.output_fmt) {
	case IMAGE_FORMAT_I420:
	case IMAGE_FORMAT_YV12:
		/* for YUV planar format, YUV offset*/
		y_offset = cal_offset(frame->cur_slice, frame->y_slice_size);
		uv_offset = cal_offset(frame->cur_slice, frame->uv_slice_size);

		src_y = cam->output_base + pdata->y_addr + y_offset;
		src_u = cam->output_base + pdata->u_addr + uv_offset;
		src_v = cam->output_base + pdata->v_addr + uv_offset;

		dest_y = cam->out_frame_base + frame->data_addr + y_offset;
		dest_u = cam->out_frame_base + frame->data_addr +
			 frame->y_frame_size + uv_offset;
		dest_v = cam->out_frame_base + frame->data_addr +
			 frame->y_frame_size + frame->uv_frame_size + uv_offset;

		size_y = cal_size(frame->cur_slice, slice,
				  idx, frame->y_slice_size);
		size_uv = cal_size(frame->cur_slice, slice,
				   idx, frame->uv_slice_size);
		dev_dbg(dev->dev, "YUVPL: YStream:src=0x%llx, dest=0x%llx, size=0x%x\n",
			(u64)src_y, (u64)dest_y, size_y);
		dev_dbg(dev->dev, "YUVPL: UStream:src=0x%llx, dest=0x%llx, size=0x%x\n",
			(u64)src_u, (u64)dest_u, size_uv);
		dev_dbg(dev->dev, "YUVPL: VStream:src=0x%llx, dest=0x%llx, size=0x%x\n",
			(u64)src_v, (u64)dest_v, size_uv);
		memcpy(dest_y, src_y, size_y);
		memcpy(dest_u, src_u, size_uv);
		memcpy(dest_v, src_v, size_uv);
		break;
	case IMAGE_FORMAT_NV12:
	case IMAGE_FORMAT_NV21:
	case IMAGE_FORMAT_P010:
		/* for YUV semi planar format, YU offset*/
		y_offset = cal_offset(frame->cur_slice, frame->y_slice_size);
		uv_offset = cal_offset(frame->cur_slice, frame->uv_slice_size);

		src_y = cam->output_base + pdata->y_addr + y_offset;
		src_u = cam->output_base + pdata->u_addr + uv_offset;

		dest_y = cam->out_frame_base + frame->data_addr + y_offset;
		dest_u = cam->out_frame_base + frame->data_addr
			 + frame->y_frame_size + uv_offset;

		size_y = cal_size(frame->cur_slice, slice,
				  idx, frame->y_slice_size);
		size_uv = cal_size(frame->cur_slice, slice,
				   idx, frame->uv_slice_size);

		dev_dbg(dev->dev, "YUVSP: YStream:src=0x%llx, dest=0x%llx, size=0x%x\n",
			(u64)src_y, (u64)dest_y, size_y);
		dev_dbg(dev->dev, "YUVSP: UStream:src=0x%llx, dest=0x%llx, size=0x%x\n",
			(u64)src_u, (u64)dest_u, size_uv);
		memcpy(dest_y, src_y, size_y);
		memcpy(dest_u, src_u, size_uv);
		break;
	case IMAGE_FORMAT_YUV422INTERLEAVED:
		/* YUV interleave only Y offset is present*/
		y_offset = 2 * cal_offset(frame->cur_slice,
					  frame->y_slice_size);
		src_y = cam->output_base + pdata->y_addr + y_offset;
		dest_y = cam->out_frame_base + frame->data_addr + y_offset;
		size_y = 2 * cal_size(frame->cur_slice, slice,
				      idx, frame->y_slice_size);
		dev_dbg(dev->dev, "YUVIL: YStream:src=0x%llx, dest=0x%llx, size=0x%x\n",
			(u64)src_y, (u64)dest_y, size_y);
		memcpy(dest_y, src_y, size_y);
		break;
	default:
		dev_err(dev->dev, "Unsupported output YUV format\n");
	}
	if (cam->dmabuf_frame)
		dma_buf_end_cpu_access(cam->dmabuf_frame, DMA_BIDIRECTIONAL);

out:
	/* update cur_slice to slice */
	frame->cur_slice = slice + 1;
	if (frame->cur_slice == frame->total_slices) {
		/* frame copy is done, updat the running output frame idx */
		cam->isp_output_frame = frame_id;
		frame->cur_slice = 0;
		if (test_and_clear_bit(ISP_COND_DQBUF_WAIT_FRAME,
				       &cam->test_condition)) {
			cam->test_dqbuf_resp.lock_frame_id = frame_id;
			wake_up_interruptible(&cam->test_wq);
		}
	}
}

static void output_ir_stream_notification(struct isp_ipc_device *dev,
					  int cam_id, void *resp)
{
	struct reg_outbuf *pdata = (struct reg_outbuf *)resp;
	u8 *src_ir, *dest_ir;
	u32 size_ir, ir_offset;
	int frame_id = pdata->id.frame;
	int slice = pdata->id.field;
	u8 idx = 0;
	struct isp_output_frame *frame;
	struct camera *cam = &dev->camera[cam_id];

	log_payload("IR stream", (u32 *)pdata,
		    sizeof(struct reg_outbuf) / sizeof(u32));

	idx = frame_id % MAX_OUTPUT_FRAME;
	frame = &cam->output_frames[idx];
	idx = frame->total_slices - 1;
	if (frame->total_slices < 1) {
		dev_warn(dev->dev, "IR out: output rings not configured\n");
		return;
	}
	calculate_oslice(dev, cam_id, frame_id);

	if (slice == 0 &&
	    test_and_clear_bit(ISP_COND_QBUF_WAIT, &cam->test_condition)) {
		cam->test_dqbuf_resp.lock_frame_id = FRAMEID_INVALID;
		cam->test_dqbuf_resp.lock_stats_id = FRAMEID_INVALID;
		wake_up_interruptible(&cam->test_wq);
	}

	/* locked, don't override */
	if (cam->test_dqbuf_resp.lock_frame_id >= 0)
		goto out;
	if (!cam->out_frame_base || !cam->output_base)
		goto out;

	ir_offset = cal_offset(frame->cur_slice, frame->ir_slice_size);
	src_ir = cam->output_base + pdata->ir_addr + ir_offset;
	dest_ir = cam->out_frame_base + frame->data_addr + ir_offset;
	size_ir = cal_size(frame->cur_slice, slice, idx, frame->ir_slice_size);
	dev_dbg(dev->dev, "IR Stream:src=0x%llx, dest=0x%llx, size=0x%x\n",
		(u64)src_ir, (u64)dest_ir, size_ir);
	if (cam->dmabuf_frame)
		dma_buf_begin_cpu_access(cam->dmabuf_frame, DMA_BIDIRECTIONAL);
	/* copy slice to IR outbuffer, output ring buffer */
	memcpy(dest_ir, src_ir, size_ir);
	if (cam->dmabuf_frame)
		dma_buf_end_cpu_access(cam->dmabuf_frame, DMA_BIDIRECTIONAL);

out:
	/* update cur_slice to slice */
	frame->cur_slice = slice + 1;
	if (frame->cur_slice == frame->total_slices) {
		/* frame copy is done, updat the running output frame idx */
		cam->isp_output_frame = frame_id;
		frame->cur_slice = 0;
		if (test_and_clear_bit(ISP_COND_DQBUF_WAIT_FRAME,
				       &cam->test_condition)) {
			cam->test_dqbuf_resp.lock_frame_id = frame_id;
			wake_up_interruptible(&cam->test_wq);
		}
	}
}

static void
dma_status_report(struct isp_ipc_device *dev, int cam_id, void *resp)
{
	struct reg_wdma_report *pdata = (struct reg_wdma_report *)resp;
	struct camera *cam = &dev->camera[cam_id];

	log_payload("DMA status", (u32 *)pdata,
		    sizeof(struct reg_wdma_report) / sizeof(u32));

	if (test_and_clear_bit(ISP_COND_WAIT_WDMA, &cam->test_condition))
		wake_up_interruptible(&cam->test_wq);
}

/* call if there is any PL320 related response task needed */
int handle_isp_client_callback(struct isp_ipc_device *dev,
			       u32 msg_id, void *resp)
{
	int cam_id;
	int ret = 0;

	cam_id = msg_id & ISP_CAM_ID_MASK;

	switch (msg_id) {

	case CAM0_CONTROL:
	case CAM1_CONTROL:
		camera_control_notification(dev, cam_id, resp);
		break;
	case CAM0_RGB_STAT:
	case CAM1_RGB_STAT:
		rgb_stat_notification(dev, cam_id, resp);
		break;
	case CAM0_IR_STAT:
	case CAM1_IR_STAT:
		ir_stat_notification(dev, cam_id, resp);
		break;
	case CAM0_INPUT_BUFF:
	case CAM1_INPUT_BUFF:
		input_pixel_buff_release_notification(dev, cam_id, resp);
		break;
	case CAM0_PRE_DATA_BUFF:
	case CAM1_PRE_DATA_BUFF:
		pre_data_buff_buff_release_notification(dev, cam_id, resp);
		break;
	case CAM0_POST_DATA_BUFF:
	case CAM1_POST_DATA_BUFF:
		post_data_buff_buff_release_notification(dev, cam_id, resp);
		break;
	case CAM0_OUTPUT_YUV_0:
	case CAM0_OUTPUT_YUV_1:
	case CAM0_OUTPUT_YUV_2:
	case CAM0_OUTPUT_YUV_3:
	case CAM1_OUTPUT_YUV_0:
	case CAM1_OUTPUT_YUV_1:
	case CAM1_OUTPUT_YUV_2:
	case CAM1_OUTPUT_YUV_3:
		output_yuv_stream_notification(dev, cam_id, resp);
		break;
	case CAM0_OUTPUT_IR:
	case CAM1_OUTPUT_IR:
		output_ir_stream_notification(dev, cam_id, resp);
		break;
	case CAM0_WDMA_STATUS:
	case CAM1_WDMA_STATUS:
		dma_status_report(dev, cam_id, resp);
		break;
	default:
		pr_err("unknown message received, ID is 0x%x\n", msg_id);
		break;
	};

	return ret;
}
#endif // CONFIG_CVIP_IPC
