/*
 * userspace controlled mailbox driver for AMD Mero SoC CVIP Subsystem.
 *
 * Copyright (C) 2019-2021 Advanced Micro Devices, Inc. All rights reserved.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/fs.h>
#include <linux/io.h>
#include <linux/sched/signal.h>
#include <linux/err.h>
#include <linux/of_device.h>
#include <linux/cdev.h>
#include <linux/poll.h>
#include <linux/mailbox_controller.h>
#include <linux/mailbox_client.h>
#include <uapi/linux/mero_ipcdev.h>

#include "mero_ipc_i.h"

#define CON_MAX 5
#define MBOX_MAX 32
/* buffer count for each char dev */
#define BUF_CNT 32
#define BUF_CNT_MASK (BUF_CNT - 1)

#define CLASS_NAME "ipc_class"
#define CHR_NAME "ipc_dev"

#define BUF_FLAG_OK	0
#define BUF_FLAG_RX	BUF_FLAG_OK
#define BUF_FLAG_TXDONE	BUF_FLAG_OK
#define BUF_FLAG_TXTOUT	3

/**
 * struct ipcdev - s/w representation of a mailbox user
 * @cl:		mbox client which requests a mbox channel
 * @chan:	mbox channel requested from mailbox framework
 * @icdev:	embed char device
 * @mlock:	protect ipcdev
 * @open_count:	ipcdev supports single opened
 * @conf:	store configurations after IOCCONFIG
 * @tx_pos, tx_count:	control @txbuf
 * @txbuf:	for a tx and non-block channel, it's used to buffer send data
 * @rx_rp, rx_wp:	control @rxbuf
 * @rxbuf:	any data from rx/tx callback will be buffered here
 * @waitq:	block readers until something is received into rxbuf
 * @slock:	protect rxbuf and txbuf
 * @node:	hook into list of ipcdev_list
 */
struct ipcdev {
	struct mbox_client	cl;
	struct mbox_chan	*chan;
	struct cdev		icdev;

	struct mutex		mlock;
	int			open_count;
	struct ipc_config	conf;

	int			tx_pos, tx_count;
	char			*txbuf;

	u32			rx_rp, rx_wp;
	char			*rxbuf;
	wait_queue_head_t	waitq;

	spinlock_t		slock;

	struct list_head	node;
};

static struct class *ipcdev_class;
static unsigned int ipc_major;
/* store controller pointer */
static struct mbox_controller *ipc_cons[CON_MAX];
static bool ready;
static LIST_HEAD(ipcdev_list);

static int txbuf_in(struct ipcdev *idev, char *msg)
{
	size_t elem_len = idev->conf.soft_link * DATA_SIZE;

	if (idev->tx_count == BUF_CNT)
		return -ENOBUFS;

	memcpy(idev->txbuf + idev->tx_pos * elem_len, msg, elem_len);
	idev->tx_count++;
	idev->tx_pos = (idev->tx_pos + 1) & BUF_CNT_MASK;
	return 0;
}

static inline void txbuf_out(struct ipcdev *idev)
{
	idev->tx_count--;
}

static inline void rxbuf_update_rpos(struct ipcdev *idev)
{
	idev->rx_rp = (idev->rx_rp + 1) & BUF_CNT_MASK;
}

static inline void rxbuf_update_wpos(struct ipcdev *idev)
{
	idev->rx_wp = (idev->rx_wp + 1) & BUF_CNT_MASK;
}

static inline u32 rxbuf_spacefree(struct ipcdev *idev)
{
	return (BUF_CNT - 1) - ((idev->rx_wp - idev->rx_rp) & BUF_CNT_MASK);
}

static inline bool rxbuf_data_ready(struct ipcdev *idev)
{
	return (idev->rx_rp != idev->rx_wp);
}

static void mbox_rxdata(struct mbox_client *cl, void *msg)
{
	struct ipcdev *idev = container_of(cl, struct ipcdev, cl);
	struct device *dev = cl->dev;
	unsigned long flags;
	char *buf;
	size_t elem_len;

	spin_lock_irqsave(&idev->slock, flags);
	if (!rxbuf_spacefree(idev)) {
		spin_unlock_irqrestore(&idev->slock, flags);
		dev_warn(dev, "no buffer to store rx message\n");
		return;
	}

	elem_len = idev->conf.soft_link * DATA_SIZE + 1;
	buf = idev->rxbuf + idev->rx_wp * elem_len;
	buf[0] = BUF_FLAG_RX;
	memcpy(buf + 1, msg, elem_len - 1);

	rxbuf_update_wpos(idev);
	spin_unlock_irqrestore(&idev->slock, flags);

	wake_up_interruptible(&idev->waitq);
}

static void mbox_txdone(struct mbox_client *cl, void *msg, int r)
{
	struct ipcdev *idev = container_of(cl, struct ipcdev, cl);
	struct device *dev = cl->dev;
	unsigned long flags;
	char *buf;
	size_t elem_len;

	spin_lock_irqsave(&idev->slock, flags);
	if (!rxbuf_spacefree(idev)) {
		spin_unlock_irqrestore(&idev->slock, flags);
		dev_warn(dev, "no buffer to store txdone message\n");
		return;
	}

	elem_len = idev->conf.soft_link * DATA_SIZE + 1;
	buf = idev->rxbuf + idev->rx_wp * elem_len;
	buf[0] = !r ? BUF_FLAG_TXDONE : r;
	if (r != -ETIME)
		mbox_get_ack_message(idev->chan, buf + 1);

	rxbuf_update_wpos(idev);

	if (!idev->conf.tx_block)
		txbuf_out(idev);

	spin_unlock_irqrestore(&idev->slock, flags);
	wake_up_interruptible(&idev->waitq);
}

static int ipcdev_init_client(struct ipcdev *idev, struct ipc_config *conf)
{
	if (conf->rx) {
		idev->cl.rx_callback = mbox_rxdata;
		idev->cl.tx_tout = 0;
		idev->cl.tx_done = NULL;
		idev->cl.tx_block = true;
		idev->cl.knows_txdone = false;
		idev->cl.data_ack = 1;

		if (conf->rx == IPC_LOOPBACK) {
			idev->cl.tx_tout = conf->tx_tout;
			idev->cl.tx_done = mbox_txdone;
			/* force tx_block in conf for loopback test */
			conf->tx_block = 1;
		}
	} else {
		idev->cl.rx_callback = NULL;
		idev->cl.tx_tout = conf->tx_tout;
		idev->cl.tx_done = mbox_txdone;
		idev->cl.tx_block = conf->tx_block;
		idev->cl.knows_txdone = false;
		idev->cl.data_ack = 1;
	}

	return 0;
}

static struct mbox_chan *ipcdev_get_chan(struct ipcdev *idev,
					 int conid, int mboxid)
{
	struct device_node *np = dev_of_node(ipc_cons[conid]->dev);

	return mbox_request_channel_ofcontroller(&idev->cl, np, mboxid);
}

static void ipcdev_put_chan(struct ipcdev *idev)
{
	if (idev->chan) {
		mbox_free_channel(idev->chan);
		idev->chan = NULL;
	}
}

static void ipcdev_free_buf(struct ipcdev *idev)
{
	kfree(idev->txbuf);
	kfree(idev->rxbuf);
	idev->txbuf = NULL;
	idev->rxbuf = NULL;
}

static int ipcdev_alloc_buf(struct ipcdev *idev, struct ipc_config *conf)
{
	size_t elem_len = conf->soft_link * DATA_SIZE;

	if (conf->rx == IPC_LOOPBACK || (!conf->rx && !conf->tx_block)) {
		idev->txbuf = kcalloc(BUF_CNT, elem_len, GFP_KERNEL);
		if (!idev->txbuf)
			goto err;
	}

	idev->rxbuf = kcalloc(BUF_CNT, elem_len + 1, GFP_KERNEL);
	if (!idev->rxbuf)
		goto err;

	return 0;
err:
	ipcdev_free_buf(idev);
	return -ENOMEM;
}

static int ipcdev_check_config(struct ipc_config *conf)
{
	if ((conf->destid >= MBOX_MAX && !conf->auto_ack) ||
	    conf->srcid >= MBOX_MAX ||
	    conf->soft_link > MBOX_MAX)
		return -EINVAL;

	if (!conf->soft_link)
		conf->soft_link = 1;

	return 0;
}

static ssize_t
ipcdev_write(struct file *filp, const char __user *ubuf,
	     size_t count, loff_t *ppos)
{
	struct ipcdev *idev = filp->private_data;
	int ret = 0;
	unsigned long flags = 0;
	size_t elem_len;
	char *tmp;

	if (!idev->chan)
		return -ENODEV;

	elem_len = idev->conf.soft_link * DATA_SIZE;
	count = min(elem_len, count);
	tmp = kzalloc(count, GFP_KERNEL);
	if (!tmp)
		return -ENOMEM;

	if (copy_from_user(tmp, ubuf, count)) {
		ret = -EFAULT;
		goto out;
	}

	if (idev->conf.rx && idev->conf.rx != IPC_LOOPBACK) {
		if (idev->conf.rx_ackbyuser)
			ipc_ack_msg(idev->chan, tmp);
		else
			ret = -EBADF;
	} else {
		if (!idev->conf.tx_block) {
			spin_lock_irqsave(&idev->slock, flags);
			ret = txbuf_in(idev, tmp);
			if (ret < 0) {
				spin_unlock_irqrestore(&idev->slock, flags);
				goto out;
			}
		}
		ret = mbox_send_message(idev->chan, tmp);
		if (!idev->conf.tx_block) {
			if (ret < 0)
				txbuf_out(idev);
			spin_unlock_irqrestore(&idev->slock, flags);
		}
	}

out:
	kfree(tmp);
	return (ret < 0) ? ret : count;
}

static ssize_t
ipcdev_read(struct file *filp, char __user *ubuf, size_t count, loff_t *ppos)
{
	struct ipcdev *idev = filp->private_data;
	unsigned long flags;
	char *buf, *tmp;
	int ret = 0;
	size_t elem_len;

	DECLARE_WAITQUEUE(wait, current);

	if (!idev->chan)
		return -ENODEV;

	elem_len = idev->conf.soft_link * DATA_SIZE + 1;
	count = min(elem_len - 1, count);
	tmp = kzalloc(count, GFP_KERNEL);
	if (!tmp)
		return -ENOMEM;

	spin_lock_irqsave(&idev->slock, flags);
	if (!rxbuf_data_ready(idev)) {
		if (filp->f_flags & O_NONBLOCK) {
			spin_unlock_irqrestore(&idev->slock, flags);
			ret = -EAGAIN;
			goto out;
		}

		ret = 0;
		add_wait_queue(&idev->waitq, &wait);
		while (!rxbuf_data_ready(idev)) {
			set_current_state(TASK_INTERRUPTIBLE);
			spin_unlock_irqrestore(&idev->slock, flags);
			schedule();
			if (signal_pending(current)) {
				ret = -ERESTARTSYS;
				break;
			}
			spin_lock_irqsave(&idev->slock, flags);
		}
		remove_wait_queue(&idev->waitq, &wait);
		if (ret)
			goto out;
	}

	buf = idev->rxbuf + idev->rx_rp * elem_len;
	rxbuf_update_rpos(idev);
	if (buf[0] == BUF_FLAG_TXTOUT) {
		spin_unlock_irqrestore(&idev->slock, flags);
		ret = -ETIME;
		goto out;
	}
	memcpy(tmp, buf + 1, count);

	spin_unlock_irqrestore(&idev->slock, flags);

	ret = copy_to_user(ubuf, tmp, count);
	if (ret == count) {
		ret = -EFAULT;
	} else {
		ret = count - ret;
		*ppos += ret;
	}

out:
	kfree(tmp);
	return ret;
}

static __poll_t
ipcdev_poll(struct file *filp, struct poll_table_struct *wait)
{
	struct ipcdev *idev = filp->private_data;
	__poll_t mask = 0;
	unsigned long flags;

	if (!idev->chan)
		return mask;

	spin_lock_irqsave(&idev->slock, flags);
	if (rxbuf_data_ready(idev)) {
		mask |= (EPOLLIN | EPOLLRDNORM);
		goto out;
	}
	spin_unlock_irqrestore(&idev->slock, flags);

	poll_wait(filp, &idev->waitq, wait);

	spin_lock_irqsave(&idev->slock, flags);
	if (rxbuf_data_ready(idev))
		mask |= (EPOLLIN | EPOLLRDNORM);

out:
	spin_unlock_irqrestore(&idev->slock, flags);
	return mask;
}

static long
ipcdev_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	long ret;
	void __user *argp = (void __user *)arg;
	struct ipcdev *idev = filp->private_data;
	unsigned int minor_num = iminor(filp->f_inode);
	unsigned int conid = minor_num / MBOX_MAX;
	unsigned int mboxid = minor_num % MBOX_MAX;
	struct mbox_controller *con = ipc_cons[conid];

	pr_debug("%s cmd 0x%x iminor %u\n", __func__, cmd, minor_num);

	ret = mutex_lock_interruptible(&idev->mlock);
	if (ret)
		return ret;

	switch (cmd) {
	case IPC_IOCCONFIG:
	{
		struct ipc_config r;

		if (idev->chan) {
			ret = -EBUSY;
			goto out;
		}

		if (copy_from_user(&r, argp, sizeof(r))) {
			ret = -EFAULT;
			goto out;
		}

		if (ipcdev_check_config(&r)) {
			ret = -EINVAL;
			goto out;
		}

		if (ipcdev_alloc_buf(idev, &r)) {
			ret = -ENOMEM;
			goto out;
		}

		ipcdev_init_client(idev, &r);

		ret = ipc_config_chan(con, mboxid, &r);
		if (ret) {
			ipcdev_free_buf(idev);
			goto out;
		}

		idev->chan = ipcdev_get_chan(idev, conid, mboxid);
		if (IS_ERR(idev->chan)) {
			ret = PTR_ERR(idev->chan);
			pr_err("can't request chan %ld\n", ret);
			ipcdev_free_buf(idev);
			idev->chan = NULL;
			goto out;
		}

		memcpy(&idev->conf, &r, sizeof(r));
		idev->tx_pos = idev->tx_count = idev->rx_rp = idev->rx_wp = 0;

		break;
	}
	default:
		ret = -ENOTTY;
	}

out:
	mutex_unlock(&idev->mlock);
	return ret;
}

static int
ipcdev_open(struct inode *inode, struct file *filp)
{
	struct ipcdev *idev = container_of(inode->i_cdev, struct ipcdev, icdev);
	int ret;

	ret = mutex_lock_interruptible(&idev->mlock);
	if (ret)
		return ret;

	if (idev->open_count) {
		ret = -EBUSY;
		goto out;
	}

	idev->open_count++;
	filp->private_data = idev;

out:
	mutex_unlock(&idev->mlock);
	return ret;
}

static int
ipcdev_release(struct inode *inode, struct file *filp)
{
	struct ipcdev *idev = container_of(inode->i_cdev, struct ipcdev, icdev);

	mutex_lock(&idev->mlock);
	idev->open_count--;
	ipcdev_put_chan(idev);
	ipcdev_free_buf(idev);
	filp->private_data = NULL;
	mutex_unlock(&idev->mlock);
	return 0;
}

static const struct file_operations ipc_dev_fops = {
	.owner		= THIS_MODULE,
	.write		= ipcdev_write,
	.read		= ipcdev_read,
	.poll		= ipcdev_poll,
	.unlocked_ioctl	= ipcdev_ioctl,
	.open		= ipcdev_open,
	.release	= ipcdev_release,
	.llseek		= no_llseek,
};

static unsigned int get_minor_start(struct mbox_controller *con)
{
	unsigned int idx;

	for (idx = 0; idx < CON_MAX; idx++) {
		if (!ipc_cons[idx]) {
			ipc_cons[idx] = con;
			goto out;
		}

		if (ipc_cons[idx] == con)
			goto out;
	}

out:
	return (idx * MBOX_MAX);
}

int ipc_dev_add(struct mbox_controller *con, unsigned int idx)
{
	int ret = -ENODEV;
	unsigned int devt, minor;
	struct ipcdev *idev;
	struct device *dev = con->dev;

	if (!ready)
		return -ENODEV;

	idev = kzalloc(sizeof(*idev), GFP_KERNEL);
	if (!idev)
		return -ENOMEM;

	minor = get_minor_start(con) + idx;
	devt = MKDEV(ipc_major, minor);
	cdev_init(&idev->icdev, &ipc_dev_fops);
	idev->icdev.owner = THIS_MODULE;
	ret = cdev_add(&idev->icdev, devt, 1);
	if (ret)
		goto err_chrdev;

	idev->cl.dev = device_create(ipcdev_class, dev, devt, NULL,
				     "%s!%s!%d", CHR_NAME, dev_name(dev), idx);
	if (IS_ERR(idev->cl.dev)) {
		ret = PTR_ERR(idev->cl.dev);
		goto err_sysdev;
	}

	mutex_init(&idev->mlock);
	spin_lock_init(&idev->slock);
	init_waitqueue_head(&idev->waitq);

	list_add_tail(&idev->node, &ipcdev_list);

	return 0;
err_sysdev:
	cdev_del(&idev->icdev);
err_chrdev:
	kfree(idev);
	return ret;
}

int ipc_dev_pre_init(void)
{
	int ret;
	dev_t devt;
	int count = MBOX_MAX * CON_MAX;

	if (ready)
		return 0;

	pr_info("%s start\n", __func__);

	ipcdev_class = class_create(THIS_MODULE, CLASS_NAME);
	if (IS_ERR(ipcdev_class)) {
		ret = PTR_ERR(ipcdev_class);
		goto err;
	}

	ret = alloc_chrdev_region(&devt, 0, count, "/dev/" CHR_NAME);
	if (ret)
		goto err_alloc;

	ipc_major = MAJOR(devt);

	ready = true;

	pr_info("%s done\n", __func__);

	return 0;
err_alloc:
	class_destroy(ipcdev_class);
err:
	return ret;
}

void ipc_dev_cleanup(void)
{
	struct ipcdev *idev, *next;

	if (!ready)
		return;
	ready = false;

	list_for_each_entry_safe(idev, next, &ipcdev_list, node) {
		list_del(&idev->node);
		device_del(idev->cl.dev);
		cdev_del(&idev->icdev);
		kfree(idev);
	}
	unregister_chrdev_region(MKDEV(ipc_major, 0), MBOX_MAX * CON_MAX);
	ipc_major = 0;

	memset(ipc_cons, 0, sizeof(ipc_cons));

	class_unregister(ipcdev_class);
}
