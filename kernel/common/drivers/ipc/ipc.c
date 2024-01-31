/*
 * IPC library main interface
 *
 * Copyright (C) 2019-2021 Advanced Micro Devices, Inc. All rights reserved.
 */

#include <linux/device.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/mailbox_controller.h>
#include <linux/timer.h>
#include <linux/workqueue.h>
#include <linux/sched/clock.h>
#include <uapi/linux/sched/types.h>
#include <linux/delay.h>
#include <linux/eventfd.h>
#ifdef CONFIG_IPC_XPORT
#include "ion.h"
#include "mero_ion.h"
#endif

#include "ipc.h"
#include "mero_ipc_i.h"

#define TIMER_PERIOD 125000
#define INTR_PERIOD 200000
#define OVERHEAD 50000
#define HIST_SIZE 16
#define BCB_OFFSET 0xa000
#define P2C_MSG0_OFFSET 0x103810680
#define WATCHDOG_REBOOT 0x2

struct histogram_latency {
	u32 limit;
	u32 count;
};

static struct ipc_context *s_ipc;
atomic_t ipc_ref;
static bool ipc_channel_ready;
static phys_addr_t share_mem_paddr;
#ifdef CONFIG_CVIP_IPC
/* alive IPC handling lock */
DEFINE_MUTEX(alive_lock);
static struct task_struct *alive_thread;
static struct task_struct *dev_state_thread;
static struct timer_list intr_test_timer;
static struct completion intr_wait_event;
static int intr_test_counter;
static struct task_struct *test_task;
static u64 min_intr_time, max_intr_time;
static u64 test_time;
static int overhead_time;
static int dev_state_type;
static struct histogram_latency hist_intr[HIST_SIZE] = {
	{ 10000, 0 },
	{ 20000, 0 },
	{ 30000, 0 },
	{ 40000, 0 },
	{ 50000, 0 },
	{ 60000, 0 },
	{ 70000, 0 },
	{ 80000, 0 },
	{ 90000, 0 },
	{ 100000, 0 },
	{ 200000, 0 },
	{ 400000, 0 },
	{ 800000, 0 },
	{ 1600000, 0 },
	{ 3200000, 0 },
	{ 6400000, 0 },
};
#endif

static int __init get_share_mem_paddr(char *str)
{
	if (kstrtoll(str, 16, &share_mem_paddr))
		pr_err("Unable to get shared memory physical address\n");

	return 1;
}
__setup("DUMPADDR=", get_share_mem_paddr);

#ifdef CONFIG_CVIP_IPC
static int intr_test(void *data)
{
	struct ipc_message test_message = {0};
	struct sched_param sched_priority = { .sched_priority = MAX_RT_PRIO - 2 };
	s64 t1, t2;
	int i;

	/*
	 * Set thread to high priority for increased accuracy of the interrupt
	 * periodic generation
	 */
	if (sched_setscheduler_nocheck(current, SCHED_FIFO, &sched_priority))
		pr_err("Failed to set %s scheduler!\n", __func__);

	min_intr_time = 0xFFFFFFFFFFFFFFFFULL;
	max_intr_time = 0;
	intr_test_counter = 0;

	/* Clear histogram stats */
	for (i = 0; i < HIST_SIZE; i++)
		hist_intr[i].count = 0;

	while (!kthread_should_stop()) {
		/* wait for event */
		wait_for_completion(&intr_wait_event);
		reinit_completion(&intr_wait_event);

		t1 = local_clock();

		/* send test interrupt to x86 */
		test_message.dest = X86;
		test_message.channel_idx = 1;
		test_message.length = 1;
		test_message.message_id = INTR_TEST;
		test_message.tx_tout = 20;
		test_message.tx_block = 1;
		test_message.payload[0] = ++intr_test_counter;

		if (ipc_sendmsg(s_ipc, &test_message) < 0)
			pr_err("Timeout (20ms) sending INTR_TEST!!\n");

		t2 = local_clock() - t1;

		if (t2 < min_intr_time)
			min_intr_time = t2;
		if (t2 > max_intr_time)
			max_intr_time = t2;

		/* Collect histogram for IPC round trip latency */
		for (i = 1; i < HIST_SIZE; i++) {
			if (t2 < hist_intr[i].limit) {
				++hist_intr[i - 1].count;
				break;
			}
		}
		if (i == HIST_SIZE)
			++hist_intr[i - 1].count;
	}

	return 0;
}

static void ipc_intr_timer_handler(struct timer_list *t)
{
	if (overhead_time >= TIMER_PERIOD) {
		mod_timer(&intr_test_timer, jiffies + nsecs_to_jiffies(TIMER_PERIOD - OVERHEAD));
		overhead_time -= TIMER_PERIOD;
	} else {
		mod_timer(&intr_test_timer, jiffies + nsecs_to_jiffies(INTR_PERIOD));
		overhead_time += OVERHEAD;
	}

	complete(&intr_wait_event);
}
#endif

/**
 * is_ipc_channel_ready - check is smu ipc channel is setup properly
 * Return:	true on ready, otherwise false
 */
bool is_ipc_channel_ready(void)
{
	return ipc_channel_ready;
}
EXPORT_SYMBOL(is_ipc_channel_ready);

/**
 * ipc_unregister_client_callback - unregister client callback
 * @ipc:	ipc_context
 * @range:	ipc_message_range
 * Return:	0 on succeed, error code on failure
 */
int ipc_unregister_client_callback(struct ipc_context *ipc,
				   struct ipc_message_range *range)
{
	struct callback_list *node;
	int ret = -EINVAL;

	/* todo check parameters valid*/
	pr_debug(" %s\n", __func__);
	if (ipc != s_ipc)
		ret = -ENODEV;

	spin_lock(&ipc->cb_list_lock);
	list_for_each_entry(node, &ipc->cb_list_head, cb_list) {
		if (match(range, node)) {
			list_del(&node->cb_list);
			kfree(node);
			ret = 0;
			break;
		}
	}
	spin_unlock(&ipc->cb_list_lock);
	return ret;
}
EXPORT_SYMBOL_GPL(ipc_unregister_client_callback);

/**
 * ipc_register_client_callback - register client callback
 * @ipc:	ipc_context
 * @range:	ipc_message_range
 * @cb:		client_callback_t
 * Return:	0 on succeed, error code on failure
 */
int ipc_register_client_callback(struct ipc_context *ipc,
				 struct ipc_message_range *range,
				 client_callback_t cb)
{
	struct callback_list *node;
	struct callback_list *new_node;
	struct list_head *head;
	bool need_to_add = true;
	int ret = 0;

	pr_debug(" %s\n", __func__);
	if (ipc != s_ipc)
		ret = -ENODEV;

	spin_lock(&ipc->cb_list_lock);
	if (list_empty(&(ipc->cb_list_head))) {
		head = &(ipc->cb_list_head);
	} else {
		head = (&(ipc->cb_list_head))->prev;
		list_for_each_entry(node, &ipc->cb_list_head, cb_list) {
			if (!overlap(range, node)) {
				if (range->end < node->range.start) {
					head = (&(node->cb_list))->prev;
					break;
				}
			} else {
				need_to_add = false;
				ret = -EINVAL;
				break;
			}
		}
	}

	if (need_to_add) {
		new_node = kmalloc(sizeof(struct callback_list), GFP_KERNEL);
		if (new_node) {
			new_node->range.start = range->start;
			new_node->range.end = range->end;
			new_node->cb_func = cb;
			list_add(&new_node->cb_list, head);
		} else
			ret = -ENOMEM;
	}

	spin_unlock(&ipc->cb_list_lock);
	/* only dump when debug is enabled */
	dump_cblist(ipc);
	return ret;
}
EXPORT_SYMBOL_GPL(ipc_register_client_callback);

/**
 * ipc_sendmsg_timeout - Send IPC message with timeout
 * @ipc:	ipc_context
 * @msg:	ipc_message
 * @timeout:	timeout
 * Return:	0 on succeed, error code on failure
 *
 */
int ipc_sendmsg_timeout(struct ipc_context *ipc,
			struct ipc_message *msg,
			unsigned long timeout)
{
	struct ipc_chan *chan;
	struct mbox_message *mbox_msg;
	struct message_list *msg_list;
	int ret = 0;

	pr_debug(" %s\n", __func__);
	if (ipc != s_ipc)
		ret = -ENODEV;

	chan = message_to_mailbox(ipc, msg);
	if (!chan) {
		pr_err("no available ipc channel for this message\n");
		return -EBUSY;
	}

	mbox_msg = get_mbox_message(chan);
	if (!mbox_msg) {
		pr_err("failed to allocate mbox message\n");
		return -ENOMEM;
	}
	ret = prepare_message(ipc, chan, mbox_msg, msg);
	if (ret < 0) {
		pr_err("unrecognized message format\n");
		put_mbox_message(chan, mbox_msg);
		return -EINVAL;
	}
	msg_list = container_of(mbox_msg, struct message_list, msg);
	if (chan->sync) {
		reinit_completion(&msg_list->done);

		ret = mbox_send_message(chan->chan, (void *)mbox_msg);
		if (ret >= 0) {
			if (timeout &&
			    !wait_for_completion_timeout(&msg_list->done,
							 timeout)) {
				pr_err("timed out waiting for completion\n");
				ret = -ETIMEDOUT;
			}
		}
		put_mbox_message(chan, mbox_msg);
	} else if (msg->tx_block) {
		chan->chan->cl->tx_block = msg->tx_block;
		chan->chan->cl->tx_tout = msg->tx_tout;
		ret = mbox_send_message(chan->chan, (void *)mbox_msg);
		if (ret < 0)
			mero_ipc_tx_cleanup(chan->chan);
	} else {
		ret = mbox_send_message(chan->chan, (void *)mbox_msg);
	}

	/* only log when debug is enabled */
	log_message(mbox_msg);
	return ret;
}
EXPORT_SYMBOL_GPL(ipc_sendmsg_timeout);

/**
 * ipc_sendmsg - Send IPC message
 * @ipc:	ipc_context
 * @msg:	ipc_message
 * Return:	0 on succeed, error code on failure
 *
 */
int ipc_sendmsg(struct ipc_context *ipc, struct ipc_message *msg)
{
	return ipc_sendmsg_timeout(ipc, msg, MAX_SYNC_TIMEOUT);
}
EXPORT_SYMBOL_GPL(ipc_sendmsg);

/**
 * ipc_mboxes_cleanup - clean up mailboxes
 * @ipc:	ipc_context
 * @type:	type of the channels
 */
void ipc_mboxes_cleanup(struct ipc_context *ipc, enum chan_type type)
{
	int idx = 0;
	int count = 0;
	struct isp_chan *ispchan;
	struct ipc_chan *ipcphan;

	if (type == IPC_CHAN) {
		count = ipc->num_ipc_chans;
		for (idx = 0; idx < count; idx++) {
			ipcphan = ipc->channels + idx;
			mero_ipc_tx_cleanup(ipcphan->chan);
		}

	} else if (type == ISP_CHAN) {
		count = ipc->num_isp_chans;
		for (idx = 0; idx < count; idx++) {
			ispchan = ipc->isp_channels + idx;
			mero_ipc_tx_cleanup(ispchan->chan);
		}
	}
}
EXPORT_SYMBOL_GPL(ipc_mboxes_cleanup);

/**
 * ipc_deinit - deinitialize IPC subsystem, cleanup resource
 * @ipc:	ipc_context
 * Return:	0 on succeed, error code on failure
 */
int ipc_deinit(struct ipc_context *ipc)
{
	int ret = 0;

	pr_debug(" %s\n", __func__);
	if (ipc != s_ipc)
		return -ENODEV;
	if (atomic_dec_if_positive(&ipc_ref) < 0)
		ret = -ENODEV;
	return ret;
}
EXPORT_SYMBOL_GPL(ipc_deinit);

/**
 * ipc_init - initialize IPC subsystem
 * @ipc:	ipc_context created
 * Return:	0 on succeed, error code on failure
 */
int ipc_init(struct ipc_context **ipc)
{
	int ret = 0;

	pr_debug("%s\n", __func__);

	if (s_ipc) {
		*ipc = s_ipc;
		atomic_inc(&ipc_ref);
	} else
		ret = -ENODEV;

	return ret;
}
EXPORT_SYMBOL_GPL(ipc_init);

struct kobject *ipccore_kobject;

static ssize_t ipctest_show(struct kobject *kobj,
			    struct kobj_attribute *attr, char *buf)
{
	pr_debug(" %s\n", __func__);

	if (s_ipc != NULL)
		return sprintf(buf, "%u\n", s_ipc->seq);
	else
		return 0;
}

#ifdef CONFIG_CVIP_IPC
static int alive_thread_func(void *data)
{
	struct ipc_context *instance = data;
	struct ipc_message *test_message;
	int alive_sent = 0;
	void __iomem *bcb;
	void __iomem *p2cmsg;
	u32 boot_reason = 0;
	u32 watchdog_count = 0;

	/* BCB offset to check for boot reason */
	bcb = ioremap(BCB_OFFSET, 0x8);
	if (!bcb)
		pr_err("ALIVE: Failed to iomap BCB\n");

	/* P2C msg0 to check for watchdog reboot count */
	p2cmsg = ioremap(P2C_MSG0_OFFSET, 0x4);
	if (!p2cmsg)
		pr_err("ALIVE: Failed to iomap PC2MSG\n");

	if (mutex_lock_interruptible(&alive_lock)) {
		pr_err("ALIVE: failed to obtain mutex\n");
		if (bcb)
			iounmap(bcb);
		if (p2cmsg)
			iounmap(p2cmsg);
		return 0;
	}

	/*
	 * alive thread is started by systemd ALIVE service. The assumption
	 * here this is checked only for the first time alive service is used.
	 * When user manually trigger alive service through /sys/ipc, we should
	 * not consider the boot reason anymore. With that assumption, we
	 * should clear the boot reason after the first check.
	 */
	if (bcb) {
		boot_reason = readl_relaxed(bcb + 0x4);
		if (boot_reason == WATCHDOG_REBOOT && p2cmsg)
			watchdog_count = readl_relaxed(p2cmsg);
		writel_relaxed(boot_reason & ~WATCHDOG_REBOOT, bcb + 0x4);
	}

	while (!alive_sent) {
		pr_debug("test cvip to x86 alive msg\n");
		test_message = kmalloc(sizeof(*test_message), GFP_KERNEL);

		test_message->dest = 0;
		test_message->channel_idx = 1;
		test_message->length = 3;
		test_message->message_id = ALIVE;
		test_message->payload[0] = boot_reason;
		test_message->payload[1] = watchdog_count;
		test_message->tx_block = 1;
		test_message->tx_tout = 250;

		if (ipc_sendmsg(instance, test_message) < 0)
			pr_debug("ALIVE message timed out. Resending...\n");
		else
			alive_sent = 1;

		kfree(test_message);
	}

	mutex_unlock(&alive_lock);
	if (bcb)
		iounmap(bcb);
	if (p2cmsg)
		iounmap(p2cmsg);

	pr_debug("x86 ALIVE message sent SUCCESS\n");
	return 0;
}

static int dev_state_msg_func(void *data)
{
	struct ipc_message *test_message;
	int msg_id = *(int *)data;

	test_message = kmalloc(sizeof(*test_message), GFP_KERNEL);

	test_message->dest = 0;
	test_message->channel_idx = 1;
	test_message->length = 0;
	test_message->message_id = msg_id;
	test_message->tx_block = 1;
	test_message->tx_tout = 20;

	pr_debug("Send device state message to x86, id = 0x%x\n", msg_id);

	if (ipc_sendmsg(s_ipc, test_message) < 0)
		pr_err("Message sending timed out...\n");

	kfree(test_message);

	return 0;
}
#endif

static ssize_t ipctest_store(struct kobject *kobj,
			     struct kobj_attribute *attr,
			     const char *buf, size_t n)
{
	char *p;
	int len;
	char *input, *sid;
	unsigned long test_id = 0;

	p = memchr(buf, '\n', n);
	len = p ? p - buf : n;
	input = kcalloc(len+1, sizeof(char), GFP_KERNEL);
	if (!input)
		goto r_ret;
	strncpy(input, buf, len);
	sid = strsep(&input, " ");
	if (sid) {
		if (kstrtoul(sid, 10, &test_id))
			goto r_free;
	}

	switch (test_id) {
#ifdef CONFIG_CVIP_IPC
	case IPC_X86_ALIVE_MSG:
		alive_thread = kthread_run(alive_thread_func, (void *)s_ipc,
					   "x86 ALIVE message thread");
		break;
	case IPC_START_INTRTEST_MSG:
		init_completion(&intr_wait_event);
		test_task = kthread_run(intr_test, NULL, "intr_test_thread");

		timer_setup(&intr_test_timer, ipc_intr_timer_handler, 0);
		mod_timer(&intr_test_timer, jiffies + usecs_to_jiffies(INTR_PERIOD));

		test_time = local_clock();
		break;
	case IPC_STOP_INTRTEST_MSG:
	{
		char str[256];
		int i, n = 0;

		if (test_task) {
			kthread_stop(test_task);
			test_task = NULL;
			del_timer(&intr_test_timer);
		}

		test_time = local_clock() - test_time;
		test_time /= 1000000;

		pr_info("\nMin intr send time: %llu\n", min_intr_time);
		pr_info("Max intr send time: %llu\n", max_intr_time);
		pr_info("Total intr:%d, total time(ms):%llu, intr/sec:%llu\n",
			intr_test_counter, test_time,
			intr_test_counter * 1000 / test_time);
		pr_info("Interrupt round-trip latency (ns) histogram\n");
		for (i = 0, n = 0; i < HIST_SIZE; i++)
			n += snprintf(&str[n], sizeof(str) - n, "%8d ", hist_intr[i].limit);
		pr_info("%s\n", str);
		for (i = 0, n = 0; i < HIST_SIZE; i++)
			n += snprintf(&str[n], sizeof(str) - n, "%8d ", hist_intr[i].count);
		pr_info("%s\n", str);
		break;
	}
	case IPC_OFF_HEAD_MSG:
		dev_state_type = OFF_HEAD;
		dev_state_thread = kthread_run(dev_state_msg_func, &dev_state_type,
					       "x86 OFF_HEAD message thread");
		break;
	case IPC_ON_HEAD_MSG:
		dev_state_type = ON_HEAD;
		dev_state_thread = kthread_run(dev_state_msg_func, &dev_state_type,
					       "x86 ON_HEAD message thread");
		break;

#endif
	default:
		pr_debug("ipc_default test\n");
		break;
	}

r_free:
	kfree(input);
r_ret:
	return n;
}

#ifdef CONFIG_CVIP_IPC
static struct eventfd_ctx *efd_ctx;

static ssize_t ipc_efd_store(struct kobject *kobj,
			     struct kobj_attribute *attr,
			     const char *buf, size_t n)
{
	unsigned long val;
	int ret;

	ret = kstrtoul(buf, 10, &val);
	if (unlikely(ret))
		goto err;

	if (val == 0) {
		efd_ctx = NULL;
		ret = n;
		goto err;
	}

	efd_ctx = eventfd_ctx_fdget(val);
	if (!efd_ctx) {
		pr_err("Cannot find valid file description\n");
		ret = -1;
		goto err;
	}

	ret = n;
err:
	return ret;
}

void ipc_eventfd_signal(int val)
{
	if (efd_ctx)
		eventfd_signal(efd_ctx, val);
}
EXPORT_SYMBOL(ipc_eventfd_signal);

#ifdef CONFIG_IPC_XPORT
static ssize_t ipc_kdump_store(struct kobject *kobj,
			       struct kobj_attribute *attr,
			       const char *buf, size_t n)
{
	struct ipc_message *ipc_message;
	unsigned long val;
	phys_addr_t paddr;
	int ret;

	if (share_mem_paddr == 0)
		return -1;

	paddr = share_mem_paddr - MERO_CVIP_ADDR_XPORT_BASE;

	ret = kstrtoul(buf, 10, &val);
	if (unlikely(ret))
		goto err;

	ipc_message = kmalloc(sizeof(*ipc_message), GFP_KERNEL);

	ipc_message->dest = 0;
	ipc_message->channel_idx = 1;
	ipc_message->length = 3;
	ipc_message->message_id = PANIC;
	ipc_message->payload[0] = upper_32_bits(paddr);
	ipc_message->payload[1] = lower_32_bits(paddr);
	ipc_message->payload[2] = val;

	ipc_sendmsg(s_ipc, ipc_message);
	kfree(ipc_message);

	ret = n;
err:
	return ret;
}

ipc_attr_wo(ipc_kdump);
#endif
ipc_attr_wo(ipc_efd);
#endif
ipc_attr(ipctest);

struct attribute *ipctest_attrs[] = {
	&ipctest_attr.attr,
#ifdef CONFIG_CVIP_IPC
	&ipc_efd_attr.attr,
#ifdef CONFIG_IPC_XPORT
	&ipc_kdump_attr.attr,
#endif
#endif
	NULL,
};

struct attribute_group ipctest_attr_group = {
	.attrs = ipctest_attrs,
};

#ifdef CONFIG_IPC_XPORT
static void ipc_register_xport_cb(void)
{
	struct ipc_message_range range;

	range.start = REGISTER_BUFFER;
	range.end = UNREGISTER_BUFFER;
	ipc_register_client_callback(s_ipc, &range, handle_xport_req);
}
#endif

static int ipc_core_init(void)
{
	ipccore_kobject = kobject_create_and_add("ipc", NULL);

	if (!ipccore_kobject) {
		pr_err("Cannot create ipc kobject\n");
		goto r_kobj;
	}

	if (sysfs_create_group(ipccore_kobject, &ipctest_attr_group)) {
		pr_err("Cannot create sysfs file\n");
		goto r_sysfs;
	}

	return 0;

r_sysfs:
	kobject_put(ipccore_kobject);
r_kobj:
	return -1;
}

static int mero_ipc_core_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	int ret = 0;

	s_ipc = devm_kzalloc(dev, sizeof(struct ipc_context), GFP_KERNEL);
	if (!s_ipc)
		return -ENOMEM;

	atomic_set(&ipc_ref, 0);
	spin_lock_init(&s_ipc->rx_queue_lock);
	spin_lock_init(&s_ipc->seq_lock);
	init_completion(&s_ipc->thread_started);
	init_completion(&s_ipc->thread_exited);

	s_ipc->seq = 0;
	ret = queue_init(s_ipc);
	if (ret < 0) {
		pr_err("failed to init mailbox message queue\n");
		goto err_message_queue;
	}

	spin_lock_init(&s_ipc->cb_list_lock);
	INIT_LIST_HEAD(&s_ipc->cb_list_head);

	s_ipc->rx_worker_thread =
		kthread_create(rx_worker_thread_func, (void *)s_ipc,
			       "IPC rxworker thread");
	if (IS_ERR(s_ipc->rx_worker_thread)) {
		pr_err("unable to create IPC rxworker thread\n");
		ret = PTR_ERR(s_ipc->rx_worker_thread);
		goto err_rxthread;
	}
	wake_up_process(s_ipc->rx_worker_thread);
	pr_debug("wake up rxworker thread\n");
	wait_for_completion(&s_ipc->thread_started);
	pr_debug("rxworker thread start failure\n");

	ret = ipc_chans_setup(dev, s_ipc);
	if (ret < 0) {
		pr_err("mailbox setup failure\n");
		goto err_mbox_setup;
	}

	pr_debug("ipc channel setup\n");
	platform_set_drvdata(pdev, s_ipc);

	ret = ipc_core_init();
	if (ret < 0) {
		pr_err("ipc core init failure\n");
		goto err_mbox_setup;
	}
	pr_debug("mailbox device init done successfule\n");
#ifdef CONFIG_IPC_XPORT
	ipc_register_xport_cb();
#endif
	ipc_channel_ready = true;
	return 0;
err_mbox_setup:
	kthread_stop(s_ipc->rx_worker_thread);
	wait_for_completion_interruptible_timeout(
		&s_ipc->thread_exited, THREAD_EXIT_TIMEOUT);
err_rxthread:
	queue_cleanup(s_ipc);
err_message_queue:
	return ret;
}

static int mero_ipc_core_remove(struct platform_device *pdev)
{
	int ret = 0;

	if (atomic_read(&ipc_ref) == 0) {
		ipc_chans_cleanup(s_ipc);
		kthread_stop(s_ipc->rx_worker_thread);
		wait_for_completion_interruptible_timeout(
		    &s_ipc->thread_exited, THREAD_EXIT_TIMEOUT);
		queue_cleanup(s_ipc);
		ipc_channel_ready = false;
		s_ipc = NULL;
	} else
		ret = -EBUSY;
	return ret;
}

static const struct of_device_id mero_ipc_of_match[] = {
	{.compatible = "amd,ipc-core"},
	{},
};

MODULE_DEVICE_TABLE(of, mero_ipc_of_match);

static struct platform_driver mero_ipc_driver = {
	.driver = {
		.name = "mero_ipc_core",
		.of_match_table = mero_ipc_of_match,
	},
	.probe = mero_ipc_core_probe,
	.remove = mero_ipc_core_remove,
};

#ifdef CONFIG_CVIP_IPC
static int __init mero_ipc_core_init(void)
{
	return platform_driver_register(&mero_ipc_driver);
}

static void __exit mero_ipc_core_exit(void)
{
	platform_driver_unregister(&mero_ipc_driver);
}

core_initcall(mero_ipc_core_init);
module_exit(mero_ipc_core_exit);
#else
module_platform_driver(mero_ipc_driver);
#endif

#ifdef CONFIG_X86_IPC
static struct platform_device *ipc_core_device;

static int __init mero_ipc_core_init(void)
{
	int ret = 0;

	ipc_core_device = platform_device_register_simple("mero_ipc_core", 0,
		NULL, 0);

	if (IS_ERR(ipc_core_device))
		ret = PTR_ERR(ipc_core_device);

	return ret;
}

static void __exit mero_ipc_core_exit(void)
{
	platform_device_unregister(ipc_core_device);
}

module_init(mero_ipc_core_init);
module_exit(mero_ipc_core_exit);
#endif
