/*
 * Mailbox test driver for AMD Mero SoC CVIP Subsystem.
 *
 * Copyright (C) 2019 Advanced Micro Devices, Inc. All rights reserved.
 */

#include <linux/debugfs.h>
#include <linux/err.h>
#include <linux/fs.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/mailbox_client.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/poll.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/workqueue.h>
#include <linux/sched/signal.h>
#include <linux/delay.h>

#define MBOX_MAX 32
#define CON_MAX 5

/* RX buffer for each channel, this is max size mero supports */
#define RX_CHAN_MSG_SIZE (4 * 7 * 3)

#define CHUNK_SIZE 128
#define CHUNK_COUNT 20

enum chan_type { RX, TX };

/**
 * struct mero_mbox_cl - s/w representation of a communication channel
 * @node:	hook into list of mero_mbox_cl.
 * @type:	tx or rx
 * @cl:		mbox client which requests a mbox channel
 * @chan:	mbox channel requested from mailbox framework
 * @conid:	controller index this channel belongs to, rang [0, con_count-1]
 * @mboxid:	mbox index within a controller, rang [0, mbox_count-1]
 * @repeats:	how many times to send @txbuf
 * @stop_tx:	indicate this tx channel should be freed
 *		basically it's only set when write to free file node
 *		and it will only be set to true
 * @work:	@txbuf is sent in a work context
 * @txbuf:	if @type is tx, this is the space to store tx data
 */
struct mero_mbox_cl {
	struct list_head	node;

	/* common to rx and tx */
	enum chan_type		type;
	struct mbox_client	cl;
	struct mbox_chan	*chan;
	u32			conid;
	u32			mboxid;

	/* tx specific */
	u32			repeats;
	bool			stop_tx;
	struct work_struct	work;
	char			txbuf[0];
};

/**
 * struct mero_mbox_test_device - representation of a platform device
 * @dev:		backing device pointer
 * @mboxes_prop:	dynamically created dts property "mboxes", used to
 *			request mbox channel via mailbox framework api
 * @con_phandle:	phandle of each controller that configured in dts
 * @con_count:		how many controllers for test are configured in dts
 * @mbox_count:		how many mbox channels within a controller
 * @root_debugfs_dir:	debugfs root dentry
 * @cl_lock:		protect list of requested mero_mbox_cls
 * @cl_list_head:	list of requested mero_mbox_cls
 * @rx_rp, rx_wp:	r/w position to access rxbuf
 * @rxbuf:		pre-allocated rx circular buffer, stores data that
 *			received from each rx channel.
 * @rxlock:		protect rxbuf
 * @waitq:		block readers until something is received into rxbuf.
 * @async_queue:	async notification for readers.
 * @txwq:		workqueue to send tx and clean tx.
 * @tx_cleanup_work:	clean up work to clean tx.
 */
struct mero_mbox_test_device {
	struct device		*dev;
	struct property		*mboxes_prop;

	phandle			con_phandle[CON_MAX];
	int			con_count;
	int			mbox_count;

	struct dentry		*root_debugfs_dir;

	struct mutex		cl_lock;

	struct list_head	cl_list_head;

	/* rx stuff */
	int			rx_rp, rx_wp;
	char			*rxbuf;
	spinlock_t		rxlock;
	wait_queue_head_t	waitq;
	struct fasync_struct	*async_queue;

	/* tx stuff */
	struct workqueue_struct	*txwq;
	struct work_struct	tx_cleanup_work;
};

static void rxbuf_update_rpos(struct mero_mbox_test_device *tdev)
{
	tdev->rx_rp++;
	if (tdev->rx_rp >= CHUNK_COUNT)
		tdev->rx_rp = 0;
}

static void rxbuf_update_wpos(struct mero_mbox_test_device *tdev)
{
	tdev->rx_wp++;
	if (tdev->rx_wp >= CHUNK_COUNT)
		tdev->rx_wp = 0;
}

/* how many writable chunks */
static u32 rxbuf_spacefree(struct mero_mbox_test_device *tdev)
{
	return (tdev->rx_rp - tdev->rx_wp + CHUNK_COUNT - 1) % CHUNK_COUNT;
}

static bool rxbuf_data_ready(struct mero_mbox_test_device *tdev)
{
	return (tdev->rx_rp != tdev->rx_wp);
}

static void receive_message(struct mbox_client *client, void *message)
{
	struct device *dev = client->dev;
	struct mero_mbox_test_device *tdev = dev_get_drvdata(dev);
	struct mero_mbox_cl *mero_cl =
			container_of(client, struct mero_mbox_cl, cl);
	unsigned long flags;
	char *chunk, *msg;
	int length = 0;

	spin_lock_irqsave(&tdev->rxlock, flags);

	if (!rxbuf_spacefree(tdev)) {
		spin_unlock_irqrestore(&tdev->rxlock, flags);
		dev_warn(dev, "no buffer to store new message\n");
		return;
	}

	/* first byte of a chunk stores length of following valid data */
	chunk = tdev->rxbuf + CHUNK_SIZE * tdev->rx_wp;
	msg = chunk + 1;

	/* finnal data format is "conid:mboxid:message\n\0" */
	length += snprintf(msg, CHUNK_SIZE, "%u:%u:",
			   mero_cl->conid, mero_cl->mboxid);
	memcpy(msg + length, message, RX_CHAN_MSG_SIZE);
	length += RX_CHAN_MSG_SIZE;
	memcpy(msg + length, "\n", 2);
	length += 2;
	chunk[0] = length;

	rxbuf_update_wpos(tdev);

	spin_unlock_irqrestore(&tdev->rxlock, flags);

	wake_up_interruptible(&tdev->waitq);

	kill_fasync(&tdev->async_queue, SIGIO, POLL_IN);
}

static void tx_cleanup(struct work_struct *work)
{
	struct mero_mbox_test_device *tdev = container_of(work,
						struct mero_mbox_test_device,
						tx_cleanup_work);
	struct mero_mbox_cl *mero_cl, *next;

	mutex_lock(&tdev->cl_lock);
	list_for_each_entry_safe(mero_cl, next, &tdev->cl_list_head, node) {
		if (mero_cl->stop_tx) {
			mbox_free_channel(mero_cl->chan);
			cancel_work_sync(&mero_cl->work);
			list_del(&mero_cl->node);
			devm_kfree(tdev->dev, mero_cl);
		}
	}
	mutex_unlock(&tdev->cl_lock);
}

static void message_txdone(struct mbox_client *client, void *message, int r)
{
	struct mero_mbox_cl *mero_cl =
			container_of(client, struct mero_mbox_cl, cl);
	struct mero_mbox_test_device *tdev = dev_get_drvdata(mero_cl->cl.dev);

	if (r)
		pr_warn("%d:%d:%d - message sent err %d\n",
			mero_cl->conid, mero_cl->mboxid, mero_cl->repeats, r);
	else
		pr_debug("%d:%d:%d - message sent ok\n",
			 mero_cl->conid, mero_cl->mboxid, mero_cl->repeats);

	mero_cl->repeats--;

	queue_work(tdev->txwq, &mero_cl->work);
}

static void tx_worker(struct work_struct *work)
{
	struct mero_mbox_cl *mero_cl =
		   container_of(work, struct mero_mbox_cl, work);
	struct mero_mbox_test_device *tdev = dev_get_drvdata(mero_cl->cl.dev);

	if (mero_cl->stop_tx || mero_cl->repeats <= 0) {
		mero_cl->stop_tx = true;
		queue_work(tdev->txwq, &tdev->tx_cleanup_work);
	} else {
		mbox_send_message(mero_cl->chan, mero_cl->txbuf);
	}
}

/* lock to update property */
static struct mero_mbox_cl *
request_mero_channel(struct mero_mbox_test_device *tdev,
		     u32 conid, u32 mboxid, u32 repeats)
{
	struct device *dev = tdev->dev;
	struct mero_mbox_cl *mero_cl;
	u32 prop_val[2];
	int size = sizeof(*mero_cl) + (repeats ? CHUNK_SIZE : 0);

	if (conid >= tdev->con_count || mboxid >= tdev->mbox_count) {
		dev_err(dev, "invalid id %u:%u\n", conid, mboxid);
		return NULL;
	}

	list_for_each_entry(mero_cl, &tdev->cl_list_head, node) {
		if (mero_cl->conid == conid && mero_cl->mboxid == mboxid) {
			dev_warn(dev, "already requested %u:%u as %s\n",
				 conid, mboxid,
				 (mero_cl->type == TX) ? "tx" : "rx");
			return NULL;
		}
	}

	mero_cl = devm_kzalloc(dev, size, GFP_KERNEL);
	if (!mero_cl)
		return NULL;

	mero_cl->cl.dev		= dev;
	mero_cl->conid		= conid;
	mero_cl->mboxid		= mboxid;
	mero_cl->repeats		= repeats;
	if (repeats) {
		mero_cl->type		= TX;
		mero_cl->cl.tx_tout	= 500;
		mero_cl->cl.tx_done	= message_txdone;
		mero_cl->cl.tx_block	= true;
		mero_cl->cl.knows_txdone = false;
		INIT_WORK(&mero_cl->work, tx_worker);
	} else {
		mero_cl->type		= RX;
		mero_cl->cl.rx_callback = receive_message;
	}

	prop_val[0] = cpu_to_be32(tdev->con_phandle[conid]);
	prop_val[1] = cpu_to_be32(mboxid);
	memcpy(tdev->mboxes_prop->value, prop_val, sizeof(prop_val));

	mero_cl->chan = mbox_request_channel(&mero_cl->cl, 0);
	if (IS_ERR(mero_cl->chan)) {
		dev_err(dev, "can't request %u:%u chan\n", conid, mboxid);
		devm_kfree(dev, mero_cl);
		return NULL;
	}

	return mero_cl;
}

static int mero_mbox_test_rx_fasync(int fd, struct file *filp, int on)
{
	struct mero_mbox_test_device *tdev = filp->private_data;

	return fasync_helper(fd, filp, on, &tdev->async_queue);
}

static ssize_t mero_mbox_test_rx_read(struct file *filp, char __user *userbuf,
				      size_t count, loff_t *ppos)
{
	struct mero_mbox_test_device *tdev = filp->private_data;
	unsigned long flags;
	char *touser, *chunk, *msg;
	int ret;

	DECLARE_WAITQUEUE(wait, current);

	touser = kzalloc(CHUNK_SIZE, GFP_KERNEL);
	if (!touser)
		return -ENOMEM;

	spin_lock_irqsave(&tdev->rxlock, flags);

	if (!rxbuf_data_ready(tdev)) {
		if (filp->f_flags & O_NONBLOCK) {
			spin_unlock_irqrestore(&tdev->rxlock, flags);
			ret = -EAGAIN;
			goto out;
		}

		ret = 0;
		add_wait_queue(&tdev->waitq, &wait);
		while (!rxbuf_data_ready(tdev)) {
			set_current_state(TASK_INTERRUPTIBLE);
			spin_unlock_irqrestore(&tdev->rxlock, flags);
			schedule();
			if (signal_pending(current)) {
				ret = -ERESTARTSYS;
				break;
			}
			spin_lock_irqsave(&tdev->rxlock, flags);
		}
		remove_wait_queue(&tdev->waitq, &wait);
		if (ret)
			goto out;
	}

	/* return 1 chunk data */
	chunk = tdev->rxbuf + CHUNK_SIZE * tdev->rx_rp;
	/* first byte of a chunk stores length of valid data */
	count = min((size_t)(chunk[0]), count);
	msg = chunk + 1;
	memcpy(touser, msg, count);

	rxbuf_update_rpos(tdev);

	spin_unlock_irqrestore(&tdev->rxlock, flags);

	ret = copy_to_user(userbuf, touser, count);
	if (ret == count) {
		ret = -EFAULT;
	} else {
		ret = count - ret;
		*ppos += ret;
	}

out:
	kfree(touser);
	return ret;
}

static __poll_t
mero_mbox_test_rx_poll(struct file *filp, struct poll_table_struct *wait)
{
	struct mero_mbox_test_device *tdev = filp->private_data;
	__poll_t mask = 0;
	unsigned long flags;

	poll_wait(filp, &tdev->waitq, wait);

	spin_lock_irqsave(&tdev->rxlock, flags);
	if (rxbuf_data_ready(tdev))
		mask |= (EPOLLIN | EPOLLRDNORM);
	spin_unlock_irqrestore(&tdev->rxlock, flags);

	return mask;
}

static const struct file_operations mero_mbox_test_rx_ops = {
	.read	= mero_mbox_test_rx_read,
	.fasync	= mero_mbox_test_rx_fasync,
	.poll	= mero_mbox_test_rx_poll,
	.open	= simple_open,
	.llseek = no_llseek,
};

static ssize_t mero_mbox_test_rxreq_write(struct file *filp,
					  const char __user *userbuf,
					  size_t count, loff_t *ppos)
{
	struct mero_mbox_test_device *tdev = filp->private_data;
	struct mero_mbox_cl *mero_cl;
	char id[10] = {0};
	u32 conid, mboxid;

	if (copy_from_user(id, userbuf, 10))
		return -EFAULT;

	if (sscanf(id, "%u:%u", &conid, &mboxid) != 2)
		return -EINVAL;

	mutex_lock(&tdev->cl_lock);

	mero_cl = request_mero_channel(tdev, conid, mboxid, 0);
	if (!mero_cl) {
		mutex_unlock(&tdev->cl_lock);
		return -EINVAL;
	}

	list_add_tail(&mero_cl->node, &tdev->cl_list_head);

	mutex_unlock(&tdev->cl_lock);
	return count;
}

static const struct file_operations mero_mbox_test_rxreq_ops = {
	.write	= mero_mbox_test_rxreq_write,
	.open	= simple_open,
	.llseek = no_llseek,
};

static ssize_t mero_mbox_test_txreq_write(struct file *filp,
					  const char __user *userbuf,
					  size_t count, loff_t *ppos)
{
	struct mero_mbox_test_device *tdev = filp->private_data;
	struct mero_mbox_cl *mero_cl;
	char buf[CHUNK_SIZE] = {0};
	char *msg;
	u32 conid, mboxid, repeats;
	int len;

	len = strncpy_from_user(buf, userbuf, CHUNK_SIZE);
	if (len < 0)
		return len;
	if (len == CHUNK_SIZE)
		buf[CHUNK_SIZE - 1] = '\0';

	/* accepted data format is "conid:mboxid:repeats:message" */
	if (sscanf(buf, "%u:%u:%u", &conid, &mboxid, &repeats) != 3)
		return -EINVAL;

	if (!repeats)
		return -EINVAL;

	msg = strchr(buf, ':');
	msg = strchr(msg + 1, ':');
	msg = strchr(msg + 1, ':');
	if (!msg) /* no data to send */
		return -EINVAL;
	if (*++msg == '\0') /* invalid data */
		return -EINVAL;

	mutex_lock(&tdev->cl_lock);

	mero_cl = request_mero_channel(tdev, conid, mboxid, repeats);
	if (!mero_cl) {
		mutex_unlock(&tdev->cl_lock);
		return -EINVAL;
	}
	list_add_tail(&mero_cl->node, &tdev->cl_list_head);

	mutex_unlock(&tdev->cl_lock);

	strncpy(mero_cl->txbuf, msg, CHUNK_SIZE);
	queue_work(tdev->txwq, &mero_cl->work);

	return count;
}

static const struct file_operations mero_mbox_test_txreq_ops = {
	.write	= mero_mbox_test_txreq_write,
	.open	= simple_open,
	.llseek = no_llseek,
};

static ssize_t mero_mbox_test_free_read(struct file *filp,
					char __user *userbuf,
					size_t count, loff_t *ppos)
{
	struct mero_mbox_test_device *tdev = filp->private_data;
	struct mero_mbox_cl *mero_cl;
	char *buf;
	int n, out;

	buf = (char *)__get_free_page(GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	n = snprintf(buf, PAGE_SIZE, "Requested mailbox list:\n");
	mutex_lock(&tdev->cl_lock);
	list_for_each_entry(mero_cl, &tdev->cl_list_head, node) {
		char *dir = (mero_cl->type == TX) ? "tx" : "rx";

		n += snprintf(buf + n, PAGE_SIZE - n, "%s:%u:%u\n",
			      dir, mero_cl->conid, mero_cl->mboxid);
	}
	mutex_unlock(&tdev->cl_lock);
	buf[n] = '\0';

	out = simple_read_from_buffer(userbuf, count, ppos, buf, n);

	free_page((unsigned long)buf);

	return out;
}

static ssize_t mero_mbox_test_free_write(struct file *filp,
					 const char __user *userbuf,
					 size_t count, loff_t *ppos)
{
	struct mero_mbox_test_device *tdev = filp->private_data;
	struct mero_mbox_cl *mero_cl;
	char id[10] = {0};
	u32 conid, mboxid;

	if (copy_from_user(id, userbuf, 10))
		return -EFAULT;

	if (sscanf(id, "%u:%u", &conid, &mboxid) != 2)
		return -EINVAL;

	mutex_lock(&tdev->cl_lock);

	list_for_each_entry(mero_cl, &tdev->cl_list_head, node) {
		if (mero_cl->conid == conid && mero_cl->mboxid == mboxid) {
			if (mero_cl->type == TX) {
				mero_cl->stop_tx = true;
				queue_work(tdev->txwq, &tdev->tx_cleanup_work);
			} else {
				mbox_free_channel(mero_cl->chan);
				list_del(&mero_cl->node);
				devm_kfree(tdev->dev, mero_cl);
			}
			break;
		}
	}

	mutex_unlock(&tdev->cl_lock);
	return count;
}

static const struct file_operations mero_mbox_test_free_ops = {
	.read	= mero_mbox_test_free_read,
	.write	= mero_mbox_test_free_write,
	.open	= simple_open,
	.llseek = no_llseek,
};

/**
 * add_debugfs - add 4 file nodes, explanation:
 *
 * "rx": read only, receive any data sent over channels u previously requested
 *	 format: "conid:mboxid:received_text\n\0"
 *	 example: cat rx
 *
 * "rx_request": write only, request which channels as rx
 *		 format: <controller_id>:<mailbox_id>
 *		 example: echo -n "4:7" > rx_request
 *
 * "tx_request": write only, request which channels as tx
 *		 format: <controller_id>:<mailbox_id>:<repeats>:<text to send>
 *		 example: echo -n "1:10:1000:hello" > tx_request
 *
 * "free": release any previously requested channels
 *	   format: <controller_id>:<mailbox_id>
 *	   example: echo -n "4:7" > free
 */
static struct dentry *add_debugfs(struct mero_mbox_test_device *tdev)
{
	struct device *dev = tdev->dev;
	struct dentry *root;

	if (!debugfs_initialized())
		return NULL;

	root = debugfs_create_dir("mero-mbox", NULL);
	if (!root) {
		dev_err(dev, "failed to create mero mailbox debugfs\n");
		return NULL;
	}

	debugfs_create_file("rx", 0400, root, tdev, &mero_mbox_test_rx_ops);

	debugfs_create_file("rx_request", 0200, root, tdev,
			    &mero_mbox_test_rxreq_ops);

	debugfs_create_file("tx_request", 0200, root, tdev,
			    &mero_mbox_test_txreq_ops);

	debugfs_create_file("free", 0600, root, tdev, &mero_mbox_test_free_ops);

	return root;
}

static int parse_con_phandle(struct mero_mbox_test_device *tdev)
{
	struct device *dev = tdev->dev;
	struct device_node *np = dev->of_node;
	struct device_node *np_con;
	int n;

	/* save phandle of designated mbox controllers */
	for (n = 0; n < CON_MAX; n++) {
		np_con = of_parse_phandle(np, "con_phandle", n);
		if (!np_con) {
			dev_err(dev, "can't get phandle%d\n", n);
			return -EINVAL;
		}
		if (!of_device_is_compatible(np_con, "amd,mero-ipc")) {
			dev_err(dev, "phandle%d is not a mero ipc\n", n);
			return -EINVAL;
		}
		tdev->con_phandle[n] = np_con->phandle;
		of_node_put(np_con);
	}
	if (!n) {
		dev_err(dev, "no controller device found\n");
		return -ENODEV;
	}

	tdev->con_count = n;
	tdev->mbox_count = MBOX_MAX;
	return 0;
}

static struct property *init_mboxes_prop(struct mero_mbox_test_device *tdev)
{
	struct device_node *np = tdev->dev->of_node;
	struct property *pp;
	int length = 0;

	pp = of_find_property(np, "mboxes", &length);
	if (length == (sizeof(u32) * 2)) {
		memset(pp->value, 0, length);
		return pp;
	}

	pp = kzalloc(sizeof(*pp), GFP_KERNEL);
	if (!pp)
		return NULL;

	pp->length = sizeof(u32) * 2;
	pp->name = kstrdup("mboxes", GFP_KERNEL);
	pp->value = kzalloc(pp->length, GFP_KERNEL);
	if (pp->name && pp->value) {
		if (!of_update_property(np, pp))
			return pp;
	}

	kfree(pp->name);
	kfree(pp->value);
	kfree(pp);
	return NULL;
}

static int mero_mbox_test_probe(struct platform_device *pdev)
{
	struct mero_mbox_test_device *tdev;
	struct device *dev = &pdev->dev;
	int ret;
	int length = sizeof(*tdev) + CHUNK_SIZE * CHUNK_COUNT;

	tdev = devm_kzalloc(dev, length, GFP_KERNEL);
	if (!tdev)
		return -ENOMEM;

	tdev->dev = dev;

	tdev->mboxes_prop = init_mboxes_prop(tdev);
	if (!tdev->mboxes_prop)
		return -ENOMEM;

	ret = parse_con_phandle(tdev);
	if (ret)
		return ret;

	INIT_LIST_HEAD(&tdev->cl_list_head);

	/* rx stuff initialization */
	tdev->rx_rp = tdev->rx_wp = 0;
	tdev->rxbuf = (char *)(tdev + 1);
	spin_lock_init(&tdev->rxlock);
	init_waitqueue_head(&tdev->waitq);

	tdev->txwq = alloc_workqueue("mero_mbox_tx",
				     WQ_MEM_RECLAIM | WQ_UNBOUND, 0);
	if (!tdev->txwq)
		return -ENOMEM;
	INIT_WORK(&tdev->tx_cleanup_work, tx_cleanup);

	mutex_init(&tdev->cl_lock);
	platform_set_drvdata(pdev, tdev);

	tdev->root_debugfs_dir = add_debugfs(tdev);
	if (!tdev->root_debugfs_dir)
		return -ENODEV;

	dev_info(dev, "Successfully registered\n");
	return 0;
}

static int mero_mbox_test_remove(struct platform_device *pdev)
{
	struct mero_mbox_test_device *tdev = platform_get_drvdata(pdev);
	struct mero_mbox_cl *mero_cl;

	debugfs_remove_recursive(tdev->root_debugfs_dir);

	mutex_lock(&tdev->cl_lock);
	list_for_each_entry(mero_cl, &tdev->cl_list_head, node)
		mero_cl->stop_tx = true;
	mutex_unlock(&tdev->cl_lock);

	queue_work(tdev->txwq, &tdev->tx_cleanup_work);
	flush_workqueue(tdev->txwq);
	destroy_workqueue(tdev->txwq);

	list_for_each_entry(mero_cl, &tdev->cl_list_head, node)
		mbox_free_channel(mero_cl->chan);

	return 0;
}

static const struct of_device_id mero_mbox_test_match[] = {
	{ .compatible = "amd,mero-mbox-test" },
	{},
};
MODULE_DEVICE_TABLE(of, mero_mbox_test_match);

static struct platform_driver mero_mbox_test_driver = {
	.driver = {
		.name = "mero_mbox_test",
		.of_match_table = mero_mbox_test_match,
	},
	.probe  = mero_mbox_test_probe,
	.remove = mero_mbox_test_remove,
};
module_platform_driver(mero_mbox_test_driver);

MODULE_DESCRIPTION("mero mailbox test driver");
