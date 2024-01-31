/*
 * Common part of mailbox driver for AMD Mero SoC CVIP Subsystem.
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
#include <linux/mailbox_controller.h>
#include <linux/mailbox_client.h>
#include <linux/of_irq.h>
#include <linux/sched.h>
#include <linux/bitops.h>
#include <uapi/linux/sched/types.h>
#include <linux/spinlock.h>
#include "mero_ipc_i.h"

#define IPCMxSOURCE(m)		((m) * 0x40)
#define IPCMxDSET(m)		(((m) * 0x40) + 0x004)
#define IPCMxDCLEAR(m)		(((m) * 0x40) + 0x008)
#define IPCMxDSTATUS(m)		(((m) * 0x40) + 0x00C)
#define IPCMxMODE(m)		(((m) * 0x40) + 0x010)
#define IPCMxMSET(m)		(((m) * 0x40) + 0x014)
#define IPCMxMCLEAR(m)		(((m) * 0x40) + 0x018)
#define IPCMxMSTATUS(m)		(((m) * 0x40) + 0x01C)
#define IPCMxSEND(m)		(((m) * 0x40) + 0x020)

/* There are 7 data registers for one mailbox.
 * But soft mailbox link can use all data registers of
 * linked mailboxes.
 * This macro helps to get address of discrete data reagisters.
 */
#define IPCMxDR(m, dr)		\
	({ typeof(dr) dr_ = (dr); \
	   ((((m) + ((dr_) / 7)) * 0x40) + \
	   (((dr_) % 7) * 4) + 0x024); })

#define IPCMMIS(irq)		(((irq) * 8) + 0x800)
#define IPCMRIS(irq)		(((irq) * 8) + 0x804)

#define MBOX_MASK(n)		(1 << (n))
#define CHAN_MASK(n)		(1 << (n))

/*
 * Parameter 'mode' has below definition:
 * Bit[0]: Same as HW specification. enable/disable auto ack.
 * Bit[1]: Same as HW specification. enable/disable auto link.
 * Bit[16]: 1:slave side of IPC controller(No IPC initialization).
 *          0:master side of IPC controller.
 * Bit[17]: 1: current core should be the destination core and Rx
 *             interrupt can be received.
 * Bit[18]: 1: current core should be the source core and Tx
 *             interrupt can be received. If both bit[17] and bit[18]
 *             are 0, it means both Tx and Rx should be handled like
 *             general IPC use case. It is meaningless, if both bit[17]
 *             and bit[18] are 1.
 * Bit[20]: 0: sync mode, controller driver at receiver side clear interrupt
 *             and trigger ack to sender
 *          1: async mode, controller driver at receiver side only clear
 *             interrupt, client driver decide when to trigger ack to sender
 * Bit[24]: 'soft_mailbox_link' defines a capability flag in driver software.
 *          If it is enabled, It means all data registers of several
 *          continuous mailboxes can be used for data transaction of
 *          head mailbox. It should be ignored while Bit[1] is enabled.
 * Bit[25:29]: It defines number of continue mailboxes.
 *             It should be ignored, if soft_mailbox_link is not enabled.
 */
#define IPC_MODE_HW_MASK		(0x3)
#define IPC_MODE_HW_AUTO_ACK		BIT(0)
#define IPC_MODE_HW_AUTO_LINK		BIT(1)
#define IPC_MODE_SLAVE			BIT(16)
#define IPC_MODE_READ_ONLY		BIT(17)
#define IPC_MODE_WRITE_ONLY		BIT(18)
#define IPC_MODE_ACK_BY_USER		BIT(20)
#define IPC_MODE_SOFT_MB_LINK		BIT(24)
#define IPC_MODE_SOFT_MB_LINK_NUM(m)	(((m) >> 25) & 0x1F)
#define IPC_MODE_SOFT_MB_LINK_SET(cnt)	(((cnt) & 0x1F) << 25)
#define IPC_MODE_LOOPBACK		(IPC_MODE_READ_ONLY | IPC_MODE_WRITE_ONLY)

static void reset_user_soft_owner(struct mero_ipc *ipc, int start, int count)
{
	struct mero_ipc_link *link;

	while (count-- > 0) {
		link = &ipc->links[start++];
		if (link->owner != OWNER_USER_SOFT) {
			dev_warn(ipc->controller.dev,
				 "reset non user soft channel %d\n", start - 1);
			continue;
		}
		link->owner = OWNER_NON;
	}
}

static int set_user_soft_owner(struct mero_ipc *ipc, int start, int count)
{
	struct mero_ipc_link *link;
	int i;

	for (i = 0; i < count; i++) {
		link = &ipc->links[start + i];
		if (link->owner != OWNER_NON)
			goto err;
		link->owner = OWNER_USER_SOFT;
	}

	return 0;
err:
	reset_user_soft_owner(ipc, start, i);
	return -EBUSY;
}

static inline int find_chanid_by_link(struct mero_ipc_link *link)
{
	int chanid = -EINVAL;

	if (link->mode & IPC_MODE_READ_ONLY)
		chanid = link->destid;
	else if (link->mode & IPC_MODE_WRITE_ONLY)
		chanid = link->srcid;
	/* TODO: handle case for !(ro|wo) */
	else
		WARN_ONCE(1, "mero ipc: incorrect rw mode\n");

	return chanid;

}

static inline int mb_data_size(struct mero_ipc_link *link)
{
	int data_size;

	if (link->mode & IPC_MODE_SOFT_MB_LINK) {
		data_size = IPC_MODE_SOFT_MB_LINK_NUM(link->mode);
		data_size *= DATA_REG_NUM;
	} else {
		data_size = DATA_REG_NUM;
	}

	return data_size;
}

static inline void __clear_interrupt(struct mbox_chan *chan)
{
	struct mero_ipc_link *link = chan->con_priv;
	struct mero_ipc *ipc = to_mero_ipc(chan->mbox);

	writel_relaxed(SEND_REG_CLEAR, ipc->base + IPCMxSEND(link->idx));
}

static inline void __auto_ack_interrupt(struct mbox_chan *chan)
{
	struct mero_ipc_link *link = chan->con_priv;
	struct mero_ipc *ipc = to_mero_ipc(chan->mbox);
	int chanid = find_chanid_by_link(link);

	writel_relaxed(BIT(chanid), ipc->base + IPCMxDCLEAR(link->idx));
}

static void mero_ipc_rx(struct mbox_chan *chan)
{
	struct mero_ipc_link *link = chan->con_priv;
	struct mero_ipc *ipc = to_mero_ipc(chan->mbox);
	int i;
	int data_size;

	data_size = mb_data_size(link);

	/* perform optimized read for non-link data for x86 only */
	if (IS_ENABLED(CONFIG_X86_IPC) && data_size == DATA_REG_NUM) {
		/*
		 * the src data must be aligned to 128bit (16 bytes)
		 * so we will read the SEND register + 7 data registers
		 * all together
		 */
		u64 *src = (u64 *)(ipc->base + IPCMxSEND(link->idx));
		u64 *dst = (u64 *)link->data;

		/* reduce MB data read for MB2 KPI test */
		if (IS_ENABLED(CONFIG_MERO_X86_MB2_TEST) && link->idx == 2) {
			*dst++ = *src++;
			*dst = *src;
		} else {
			*dst++ = *src++;
			*dst++ = *src++;
			*dst++ = *src++;
			*dst = *src;
		}
		/* only call when there are client registered on this chan */
		if (chan->cl)
			mbox_chan_received_data(chan, &link->data[1]);
	} else {
		for (i = 0; i < data_size; i++)
			link->data[i] =
				readl_relaxed(ipc->base + IPCMxDR(link->idx, i));

		/* only call when there are client registered on this chan */
		if (chan->cl)
			mbox_chan_received_data(chan, link->data);
	}

	/*
	 * as per system design, currently there are no real use cases for
	 * autoack and autolink mode. might improve in future.
	 */
	if (link->mode & IPC_MODE_ACK_BY_USER)
		return;
	if (link->mode & IPC_MODE_HW_AUTO_ACK) {
		__auto_ack_interrupt(chan);
		return;
	}

	mero_ipc_ack_interrupt(chan);
}

static void mero_ipc_tx(struct mbox_chan *chan)
{
	/* only call when there are client registered on this chan */
	if (chan->cl)
		mbox_chan_txdone(chan, 0);
}

static void mero_ipc_loopback(struct mbox_chan *chan)
{
	struct mero_ipc_link *link = chan->con_priv;
	struct mero_ipc *ipc = to_mero_ipc(chan->mbox);
	u32 reg;

	reg = readl_relaxed(ipc->base + IPCMxSEND(link->idx));

	if (reg == SEND_REG_SET) {
		mero_ipc_rx(chan);
	} else if (reg == SEND_REG_ACK) {
		mero_ipc_tx(chan);
		__clear_interrupt(chan);
	}
}

static int mero_ipc_set_ack_message(struct mbox_chan *chan, void *mssg)
{
	struct mero_ipc *ipc;
	struct mero_ipc_link *link;
	int i, data_size;
	u32 *arg;

	if (!chan || !mssg) {
		pr_err("can't set ack data: no channel or data\n");
		return -EINVAL;
	}

	ipc = to_mero_ipc(chan->mbox);
	link = chan->con_priv;
	arg = mssg;

	data_size = mb_data_size(link);
	for (i = 0; i < data_size; i++)
		writel_relaxed(arg[i], ipc->base + IPCMxDR(link->idx, i));

	/* make sure all data register write out */
	wmb();

	return 0;
}

static int mero_ipc_get_ack_message(struct mbox_chan *chan, void *mssg)
{
	struct mero_ipc_link *link;
	struct mero_ipc *ipc;
	int i, data_size;
	u32 *arg;

	if (!chan || !mssg) {
		pr_err("can't get ack data: no channel or data\n");
		return -EINVAL;
	}

	ipc = to_mero_ipc(chan->mbox);
	link = chan->con_priv;
	arg = mssg;

	data_size = mb_data_size(link);
	memset(mssg, 0, data_size);
	for (i = 0; i < data_size; i++)
		arg[i] = readl_relaxed(ipc->base + IPCMxDR(link->idx, i));

	return 0;
}

void mero_ipc_tx_cleanup(struct mbox_chan *chan)
{
	int success = 0;
	struct mero_ipc_link *link;
	struct mero_ipc *ipc;

	if (!chan)
		goto err;

	link = chan->con_priv;
	if (!link)
		goto err;

	ipc = container_of(chan->mbox, struct mero_ipc, controller);
	success = 1;

err:
	if (success)
		writel_relaxed(SEND_REG_CLEAR, ipc->base + IPCMxSEND(link->idx));
	else
		pr_err("failed mero_ipc_tx_cleanup...\n");
}
EXPORT_SYMBOL(mero_ipc_tx_cleanup);

static irqreturn_t mero_ipc_mb2_irq_handler(int irq, void *p)
{
	struct mero_ipc_irq *ipcirq = p;
	struct mero_ipc *ipc = ipcirq->ipc;
	struct mero_ipc_link *link;
	struct mbox_chan *chan;
	unsigned long flags;
	int chanid;

	chanid = ipcirq->chanid;
	if (!(ipc->irq_requested & BIT(chanid)))
		return IRQ_NONE;

	spin_lock_irqsave(&ipcirq->lock, flags);
	chan = &ipc->chans[2];
	link = chan->con_priv;

	if (link->active && (ipc->mbid_mask[chanid] & BIT(link->idx))) {
		if (link->mode & IPC_MODE_READ_ONLY) {
			mero_ipc_rx(chan);
		} else if (link->mode & IPC_MODE_WRITE_ONLY) {
			__clear_interrupt(chan);
			mero_ipc_tx(chan);
		}
	} else {
		__clear_interrupt(chan);
	}

	spin_unlock_irqrestore(&ipcirq->lock, flags);
	return IRQ_HANDLED;
}

static irqreturn_t mero_ipc_irq_handler(int irq, void *p)
{
	struct mero_ipc_irq *ipcirq = p;
	struct mero_ipc *ipc = ipcirq->ipc;
	struct mero_ipc_link *link;
	struct mbox_chan *chan;
	unsigned long flags;
	int chanid;
	u32 mis;
	int ffs_data;

	chanid = ipcirq->chanid;
	if (!(ipc->irq_requested & BIT(chanid)))
		return IRQ_NONE;

	mis = readl_relaxed(ipc->base + IPCMMIS(chanid));
	if (!mis)
		return IRQ_NONE;

	spin_lock_irqsave(&ipcirq->lock, flags);
	ipcirq->mis = mis;

	/*
	 * only use IRQ handler for x86 IPC processing for now, since CVIP
	 * IPC handling mainly focus for ISP<->CVIP interaction, and will
	 * need longer processing time and MUST use irq_thread
	 */
	if (!IS_ENABLED(CONFIG_X86_IPC))
		goto wake_thread;

	/*
	 * check if MIS has more than 1 bit, if that is the case, use
	 * irq_thread to handle it because that will take more time to
	 * process multiple IPC irq.
	 */
	ffs_data = ffs(mis);
	if (ffs_data != fls(mis))
		goto wake_thread;

	chan = &ipc->chans[ffs_data - 1];
	link = chan->con_priv;

	if (link->active && (ipc->mbid_mask[chanid] & BIT(link->idx))) {
		if ((link->mode & IPC_MODE_LOOPBACK) == IPC_MODE_LOOPBACK) {
			mero_ipc_loopback(chan);
		} else if (link->mode & IPC_MODE_READ_ONLY) {
			mero_ipc_rx(chan);
			if (link->mode & (IPC_MODE_ACK_BY_USER | IPC_MODE_HW_AUTO_ACK))
				__clear_interrupt(chan);
		} else if (link->mode & IPC_MODE_WRITE_ONLY) {
			__clear_interrupt(chan);
			mero_ipc_tx(chan);
		}
	} else {
		__clear_interrupt(chan);
	}

	spin_unlock_irqrestore(&ipcirq->lock, flags);
	return IRQ_HANDLED;

wake_thread:
	spin_unlock_irqrestore(&ipcirq->lock, flags);
	return IRQ_WAKE_THREAD;
}

static irqreturn_t mero_ipc_thread_fn(int irq, void *p)
{
	struct mero_ipc_irq *ipcirq = p;
	struct mero_ipc *ipc = ipcirq->ipc;
	struct mero_ipc_link *link;
	struct mbox_chan *chan;
	unsigned long flags, mis;
	int chanid, mboxid;

	spin_lock_irqsave(&ipcirq->lock, flags);
	chanid = ipcirq->chanid;
	mis = (unsigned long)ipcirq->mis;
	spin_unlock_irqrestore(&ipcirq->lock, flags);

	mboxid = 0;
	for_each_set_bit(mboxid, &mis, IPC_CHN_NUM) {
		chan = &ipc->chans[mboxid];
		link = chan->con_priv;

		mutex_lock(&link->lock);
		if (link->active && (ipc->mbid_mask[chanid] & BIT(link->idx))) {
			if ((link->mode & IPC_MODE_LOOPBACK) == IPC_MODE_LOOPBACK) {
				mero_ipc_loopback(chan);
			} else if (link->mode & IPC_MODE_READ_ONLY) {
				mero_ipc_rx(chan);
				if (link->mode & (IPC_MODE_ACK_BY_USER | IPC_MODE_HW_AUTO_ACK))
					__clear_interrupt(chan);
			} else if (link->mode & IPC_MODE_WRITE_ONLY) {
				spin_lock_irqsave(&chan->lock, flags);
				__clear_interrupt(chan);
				spin_unlock_irqrestore(&chan->lock, flags);
				mero_ipc_tx(chan);
			}
			/* TODO: handle case for !(ro|wo) */

			pr_debug("irq[%d] for MB[%s] chan[%d] mode(0x%x)\n",
				 irq,
				 dev_name(ipc->controller.dev),
				 mboxid,
				 link->mode);
		} else {
			__clear_interrupt(chan);
		}
		mutex_unlock(&link->lock);
	}

	/* setup irq thread to higher thread priority */
	if (!ipcirq->sched_fifo) {
		struct sched_param params;

		params.sched_priority = 5;
		if (sched_setscheduler(current, SCHED_FIFO, &params))
			pr_err("Failed to set FIFO sched\n");
		else
			ipcirq->sched_fifo = 1;
	}

	return IRQ_HANDLED;
}

static int mero_ipc_send_data(struct mbox_chan *chan, void *data)
{
	struct mero_ipc *ipc;
	struct mero_ipc_link *link;
	int i, data_size;
	u32 *arg;

	if (!chan || !data) {
		pr_err("can't send data: no channel or data\n");
		return -EINVAL;
	}

	ipc = to_mero_ipc(chan->mbox);
	link = chan->con_priv;
	arg = data;

	data_size = mb_data_size(link);
	for (i = 0; i < data_size; i++)
		writel_relaxed(arg[i], ipc->base + IPCMxDR(link->idx, i));

	/* make sure all data register write out */
	wmb();

	/*Generate interrupt to destination*/
	mero_ipc_send_interrupt(chan);

	return 0;
}

static int mero_ipc_startup(struct mbox_chan *chan)
{
	struct mero_ipc *ipc;
	struct mero_ipc_link *link;
	int ret = 0, i, link_mb, destid = 0;

	if (!chan) {
		pr_err("failed to startup channel: no channel\n");
		return -EINVAL;
	}

	ipc = to_mero_ipc(chan->mbox);
	link = chan->con_priv;

	mutex_lock(&link->lock);

	if (link->owner != OWNER_USER && link->owner != OWNER_KERNEL) {
		pr_err("failed to startup channel: no owner\n");
		ret = -EINVAL;
		goto out;
	}

	if (link->owner == OWNER_USER &&
	    chan->cl && chan->cl->dev && chan->cl->dev->of_node) {
		pr_err("of device can't startup a user ownered channel\n");
		ret = -EPERM;
		goto out;
	}

	link->active = true;

	if (link->mode & IPC_MODE_SLAVE)
		goto out;

	/*Set the Source Register.*/
	writel_relaxed(CHAN_MASK(link->srcid),
			ipc->base + IPCMxSOURCE(link->idx));
	if (link->mode & IPC_MODE_SOFT_MB_LINK) {
		link_mb = IPC_MODE_SOFT_MB_LINK_NUM(link->mode);
		for (i = 1; i < link_mb; i++) {
			writel_relaxed(CHAN_MASK(link->srcid),
				       ipc->base + IPCMxSOURCE(link->idx + i));
		}
	}

	/*Set the Mailbox Mode Register for Auto Link & Auto Acknowledge.*/
	writel_relaxed(link->mode & IPC_MODE_HW_MASK,
			ipc->base + IPCMxMODE(link->idx));

	/*no chan_mask if auto_ack mode*/
	if (link->mode & IPC_MODE_HW_AUTO_ACK)
		destid = link->destid;
	else
		destid = CHAN_MASK(link->destid);

	/*Set Mailbox Mask Status to enable the interrupts for each core.*/
	writel_relaxed(CHAN_MASK(link->srcid) | destid,
		       ipc->base + IPCMxMSET(link->idx));

	/*Set the Destination Register.*/
	writel_relaxed(destid, ipc->base + IPCMxDSET(link->idx));

out:
	mutex_unlock(&link->lock);
	return ret;
}

static void ipc_chan_irq_free(struct mero_ipc *ipc, struct mero_ipc_link *link,
			      struct mbox_chan *chan, int chanid)
{
	int link_mb = 0;
	int irq;

	if (link->owner == OWNER_USER) {
		ipc->mbid_mask[chanid] &= ~(BIT(link->idx));
		link->owner = OWNER_NON;
		link_mb = IPC_MODE_SOFT_MB_LINK_NUM(link->mode);
	}

	irq = ipc->irqmap[chanid];
	if ((ipc->irq_requested & BIT(chanid)) &&
	    (atomic_sub_return(1, &ipc->links[chanid].ipc_irq.refcount) == 0)) {
		irq_set_affinity_hint(irq, NULL);
		devm_free_irq(chan->mbox->dev, irq, &ipc->links[chanid].ipc_irq);
		ipc->irq_requested &= ~BIT(chanid);
	}

	if (link->owner == OWNER_USER)
		reset_user_soft_owner(ipc, link->idx + 1, link_mb - 1);
}

static void mero_ipc_shutdown(struct mbox_chan *chan)
{
	struct mero_ipc *ipc;
	struct mero_ipc_link *link;
	int i, link_mb = 0, chanid = 0;

	if (!chan) {
		pr_warn("failed to shutdown channel: no channel\n");
		return;
	}

	ipc = to_mero_ipc(chan->mbox);
	link = chan->con_priv;

	mutex_lock(&link->lock);
	link->active = false;

	if (link->owner != OWNER_USER && link->owner != OWNER_KERNEL) {
		pr_warn("failed to shutdown channel: no owner\n");
		goto out;
	}

	if (link->mode & IPC_MODE_SLAVE)
		goto freeirq;

	/*Clearing the Source Register.*/
	writel_relaxed(0x0, ipc->base + IPCMxSOURCE(link->idx));
	if (link->mode & IPC_MODE_SOFT_MB_LINK) {
		link_mb = IPC_MODE_SOFT_MB_LINK_NUM(link->mode);
		for (i = 1; i < link_mb; i++)
			writel_relaxed(0x0,
				       ipc->base + IPCMxSOURCE(link->idx + i));
	}

freeirq:
	chanid = find_chanid_by_link(link);
	ipc_chan_irq_free(ipc, link, chan, chanid);

	/*
	 * special handling for loopback mode, which needs to clear both
	 * src and dst IRQ
	 */
	if ((link->mode & IPC_MODE_LOOPBACK) == IPC_MODE_LOOPBACK)
		ipc_chan_irq_free(ipc, link, chan, link->srcid);
out:
	mutex_unlock(&link->lock);
}

static const struct mbox_chan_ops mero_ipc_ops = {
	.send_data = mero_ipc_send_data,
	.startup = mero_ipc_startup,
	.shutdown = mero_ipc_shutdown,
	.set_ack_message = mero_ipc_set_ack_message,
	.get_ack_message = mero_ipc_get_ack_message,
};

/**/
static int ipc_config_chan_irq(struct device *dev, struct mero_ipc *ipc,
			       struct mero_ipc_link *link,
			       struct ipc_config *conf, int chanid, int mbidx)
{
	int irq, ret;

	irq = ipc->irqmap[chanid];
	/* request once for each irq */
	if (!(ipc->irq_requested & BIT(chanid))) {
		struct mero_ipc_irq *ipcirq;
		const char *irqname;
		u32 cpu;

		ipcirq = &ipc->links[chanid].ipc_irq;
		ipcirq->chanid = chanid;
		ipcirq->ipc = ipc;
		ipcirq->mis = 0;
		ipcirq->sched_fifo = 0;
		spin_lock_init(&ipcirq->lock);
		if (of_property_read_string_index(dev->of_node,
						  "interrupt-names",
						  chanid, &irqname) < 0)
			irqname = "mero_ipc";
		ret = devm_request_threaded_irq(dev, irq,
						mero_ipc_irq_handler,
						mero_ipc_thread_fn,
						IRQF_ONESHOT, irqname,
						ipcirq);
		if (ret) {
			dev_err(dev, "fail to request irq\n");
			if (link->mode & IPC_MODE_SOFT_MB_LINK)
				reset_user_soft_owner(ipc, mbidx + 1,
						      conf->soft_link - 1);
			link->owner = OWNER_NON;
			return ret;
		}
		if (of_property_read_u32_index(dev->of_node,
					       "affinity",
					       chanid, &cpu) < 0)
			cpu = chanid % nr_cpu_ids;
		irq_set_affinity_hint(irq, cpumask_of(cpu));
		atomic_set(&ipcirq->refcount, 1);
	} else {
		/*
		 * Increase ref count if channel IRQ is already requested.
		 * This is possible because other MB can use the same channel
		 * IRQ. The ref count is to make sure the IRQ will only get
		 * free when the last MB owner shutdown
		 */
		atomic_add(1, &ipc->links[chanid].ipc_irq.refcount);
	}

	ipc->irq_requested |= BIT(chanid);
	ipc->mbid_mask[chanid] |= BIT(mbidx);
	link->irq = irq;

	return 0;
}

int ipc_config_chan(struct mbox_controller *con, unsigned int idx,
		    struct ipc_config *conf)
{
	struct mero_ipc *ipc = to_mero_ipc(con);
	struct mero_ipc_link *link;
	struct device *dev = con->dev;
	int chanid, ret;

	if (idx < 0 || idx >= IPC_CHN_NUM ||
	    (idx + conf->soft_link) > IPC_CHN_NUM) {
		dev_err(dev, "index out of range\n");
		return -EINVAL;
	}

	link = &ipc->links[idx];
	if (link->owner != OWNER_NON) {
		dev_err(dev, "user channel already configured\n");
		return -EBUSY;
	}

	link->owner = OWNER_USER;
	link->srcid = conf->srcid;
	link->mode = 0;
	if (conf->slave)
		link->mode |= IPC_MODE_SLAVE;
	if (conf->rx == IPC_LOOPBACK)
		link->mode |= IPC_MODE_LOOPBACK;
	else if (conf->rx)
		link->mode |= IPC_MODE_READ_ONLY;
	else
		link->mode |= IPC_MODE_WRITE_ONLY;
	if (conf->rx && conf->rx_ackbyuser && !conf->auto_ack)
		link->mode |= IPC_MODE_ACK_BY_USER;
	if (conf->auto_ack) {
		link->mode |= IPC_MODE_HW_AUTO_ACK;
		link->destid = conf->destmask;
	} else {
		link->destid = conf->destid;
	}
	if (conf->soft_link > 1) {
		if (set_user_soft_owner(ipc, idx + 1, conf->soft_link - 1)) {
			dev_err(dev, "failed to get soft link channels\n");
			link->owner = OWNER_NON;
			return -EBUSY;
		}
		link->mode |= IPC_MODE_SOFT_MB_LINK;
		link->mode |= IPC_MODE_SOFT_MB_LINK_SET(conf->soft_link);
	}

	if (conf->rx && conf->auto_ack)
		chanid = conf->destmask;
	else if (conf->rx)
		chanid = conf->destid;
	else
		chanid = conf->srcid;

	ret = ipc_config_chan_irq(dev, ipc, link, conf, chanid, idx);
	if (ret)
		return ret;

	if (conf->rx == IPC_LOOPBACK)
		ret = ipc_config_chan_irq(dev, ipc, link, conf, conf->srcid, idx);

	return 0;
}

void ipc_ack_msg(struct mbox_chan *chan, char *buf)
{
	struct mero_ipc_link *link = chan->con_priv;
	struct device *dev = chan->mbox->dev;

	if (link->owner != OWNER_USER ||
	    !(link->mode & IPC_MODE_READ_ONLY) ||
	    !(link->mode & IPC_MODE_ACK_BY_USER)) {
		dev_err(dev, "can't ack, wrong config %d\n", link->idx);
		return;
	}

	mero_ipc_set_ack_message(chan, buf);
	mero_ipc_ack_interrupt(chan);
}

void ipc_irq_affinity_restore(struct mero_ipc_dev *ipc_dev)
{
	int i, chanid;
	struct mero_ipc *ipc;
	struct mero_ipc_link *link;
	u32 cpu;

	ipc = ipc_dev->ipc;
	for (i = 0; i < IPC_CHN_NUM; i++) {
		link = &ipc->links[i];
		if (link->active) {
			chanid = find_chanid_by_link(link);
			if (ipc->irq_requested & BIT(chanid)) {
				if (of_property_read_u32_index(ipc_dev->dev->of_node,
							       "affinity",
							       chanid, &cpu) < 0)
					cpu = chanid % nr_cpu_ids;
				irq_set_affinity_hint(link->irq, cpumask_of(cpu));
			}
		}
	}
}

int mero_ipc_restore(struct mero_ipc_dev *ipc_dev)
{
	struct mbox_chan *chan;
	int ret = 0;
	int chan_num = 0;
	struct mero_ipc *ipc = ipc_dev->ipc;

	if (IS_ERR_OR_NULL(ipc))
		return -ENODEV;

	for (chan_num = 0; chan_num < IPC_CHN_NUM; chan_num++) {
		chan = &ipc->chans[chan_num];
		if (chan->cl) {
			ret = mero_ipc_startup(chan);
			if (ret < 0) {
				pr_err("failed to startup channel:%d\n", chan_num);
				break;
			}
		}
	}

	ipc_irq_affinity_restore(ipc_dev);

	return ret;
}

int mero_mailbox_init(struct mero_ipc_dev *ipc_dev)
{
	struct mero_ipc *ipc;
	struct mero_ipc_link *link;
	int ret = -ENODEV, irq, chanid, num, link_mb, idx;
	unsigned int i;

	ipc = devm_kzalloc(ipc_dev->dev, sizeof(*ipc), GFP_KERNEL);
	if (!ipc)
		return -ENOMEM;

	ipc_dev->ipc = ipc;
	ipc->base = ipc_dev->base;
	/* phase0: pre-setup */
	for (i = 0; i < IPC_CHN_NUM; i++) {
		ipc->links[i].owner = OWNER_NON;
		ipc->links[i].active = false;
		mutex_init(&ipc->links[i].lock);
		ipc->links[i].irq = -EINVAL;
		ipc->links[i].idx = i;
		ipc->irqmap[i] = ipc_dev->vecs[i];
		if (ipc->irqmap[i] < 0) {
			dev_err(ipc_dev->dev, "failed to get irq for mb%d\n", i);
			goto kfree_err;
		}
		ipc->chans[i].con_priv = &ipc->links[i];
	}

	/* phase1: init all dts configured channels whose owner is kernel */
	num = ipc_dev->nr_mbox;
	for (i = 0; i < num; i++) {
		idx = ipc_dev->param[i * PARAM_CELLS];
		link = &ipc->links[idx];
		if (link->owner == OWNER_KERNEL_SOFT)
			continue;

		link->srcid = ipc_dev->param[i * PARAM_CELLS + 1];
		link->destid = ipc_dev->param[i * PARAM_CELLS + 2];
		link->mode = ipc_dev->param[i * PARAM_CELLS + 3];
		link->owner = OWNER_KERNEL;

		if (link->mode & IPC_MODE_HW_AUTO_LINK) {
			if (idx == IPC_CHN_NUM - 1)
				link->mode &= ~IPC_MODE_HW_AUTO_LINK;
			else
				link->mode &= ~IPC_MODE_SOFT_MB_LINK;
		}

		if (link->mode & IPC_MODE_SOFT_MB_LINK) {
			link_mb = IPC_MODE_SOFT_MB_LINK_NUM(link->mode);

			if (link_mb <= 1 || (link_mb + idx) > IPC_CHN_NUM) {
				link->mode &= ~IPC_MODE_SOFT_MB_LINK;
				dev_warn(ipc_dev->dev, "Wrong MB num setting in mode\n");
			} else {
				int n;

				for (n = 1; n < link_mb; n++) {
					ipc->links[idx + n].owner =
							OWNER_KERNEL_SOFT;
				}
			}
		}

		ret = chanid = find_chanid_by_link(link);
		if (chanid < 0) {
			dev_warn(ipc_dev->dev, "Wrong channel id for MB%d\n", idx);
			if (i == 0)
				goto kfree_err;
			continue;
		}

		irq = ipc->irqmap[chanid];
		/* request once for each irq */
		if (!(ipc->irq_requested & BIT(chanid))) {
			struct mero_ipc_irq *ipcirq;
			const char *irqname;
			u32 cpu;

			ipcirq = &ipc->links[chanid].ipc_irq;
			ipcirq->chanid = chanid;
			ipcirq->ipc = ipc;
			ipcirq->mis = 0;
			ipcirq->sched_fifo = 0;
			spin_lock_init(&ipcirq->lock);
			if (of_property_read_string_index(ipc_dev->dev->of_node,
							  "interrupt-names",
							  chanid, &irqname) < 0)
				irqname = "mero_ipc";

			/* only use dedicate IRQ handler for MB2 KPI testing */
			if (IS_ENABLED(CONFIG_MERO_X86_MB2_TEST) && idx == 2)
				ret = devm_request_threaded_irq(ipc_dev->dev, irq,
								mero_ipc_mb2_irq_handler,
								mero_ipc_thread_fn,
								IRQF_ONESHOT, irqname,
								ipcirq);
			else
				ret = devm_request_threaded_irq(ipc_dev->dev, irq,
								mero_ipc_irq_handler,
								mero_ipc_thread_fn,
								IRQF_ONESHOT, irqname,
								ipcirq);

			if (ret) {
				dev_err(ipc_dev->dev,
					"fail to request irq for mb%d\n", idx);
				if (i == 0)
					goto kfree_err;
				continue;
			}
			if (of_property_read_u32_index(ipc_dev->dev->of_node,
						       "affinity",
						       chanid, &cpu) < 0)
				cpu = chanid % nr_cpu_ids;
			irq_set_affinity_hint(irq, cpumask_of(cpu));
			atomic_set(&ipcirq->refcount, 1);
		}

		link->irq = irq;
		ipc->irq_requested |= BIT(chanid);
		ipc->mbid_mask[chanid] |= BIT(idx);
	}

	/* phase2: register mailbox controller */
	ipc->controller.dev = ipc_dev->dev;
	ipc->controller.ops = &mero_ipc_ops;
	ipc->controller.num_chans = IPC_CHN_NUM;
	ipc->controller.chans = ipc->chans;
	ipc->controller.txdone_irq = true;

	ret = devm_mbox_controller_register(ipc_dev->dev, &ipc->controller);
	if (ret) {
		dev_err(ipc_dev->dev, "Failed to register mailboxes %d\n", ret);
		goto kfree_err;
	}

	/* phase3: init all non-dts configured channels */
	for (i = 0; i < IPC_CHN_NUM; i++) {
		link = &ipc->links[i];
		if (link->owner != OWNER_NON)
			continue;

		if (ipc_dev_add(&ipc->controller, i))
			dev_warn(ipc_dev->dev, "err add cdev for MB%d\n", i);
	}

	return 0;

kfree_err:
	return ret;
}
