/*
 * Copyright (C) 2019-2021 Advanced Micro Devices, Inc. All rights reserved.
 */

#ifndef _MERO_IPC_I_H_
#define _MERO_IPC_I_H_

#include <linux/mailbox_controller.h>
#include <uapi/linux/mero_ipcdev.h>

#define DATA_REG_NUM	7
#define IPC_CHN_NUM	32
#define PARAM_CELLS	4
#define SEND_REG_CLEAR	0x0
#define SEND_REG_SET	0x1
#define SEND_REG_ACK	0x2

/*
 * Tells that kernel or userspace owns a channel, only KERNEL and USER mode
 * channels can be used via mailbox framework apis.
 * OWNER_NON:		means the channel is not configured.
 * OWNER_KERNEL:	DTS provided channels are configured as owner kernel
 * OWNER_KERNEL_SOFT:	DTS provided channels and of soft link mode will set
 *			kernel soft mode
 * OWNER_USER:		non-DTS provided channels are configured as owner user
 * OWNER_USER_SOFT:	non-DTS provided channels and of soft link mode will
 *			set user soft mode
 */
enum mbox_owner {
	OWNER_NON,
	OWNER_KERNEL,
	OWNER_KERNEL_SOFT,
	OWNER_USER,
	OWNER_USER_SOFT,
};

/*
 * Mailbox channel irq info
 *
 * @sched_fifo:	flag to indicate scheduling priority has been set
 * @mis:	mailbox interrupt source
 * @chanid:	channel id of the irq
 * @refcount:	ref counter for total irq allocation
 * @lock:	protect this channel
 * @ipc:	mailbox controller data
 */
struct mero_ipc_irq {
	int sched_fifo;
	u32 mis;
	u32 chanid;
	atomic_t refcount;
	/* for locking ipc irq */
	spinlock_t lock;
	struct mero_ipc *ipc;
};

/*
 * Mailbox channel information
 *
 * @owner:	owner that controls this channel.
 * @lock:	protect this channel
 * @active:	if this channel has been started up by a client driver
 * @irq:	Interrupt number of the channel
 * @idx:	Index of the channel, starting from 0
 * @srcid:	Source core id
 * @destid:	Destination core id
 * @mode:	Mode setting for Auto Link and Auto Ack
 * @ipc_irq:	channel irq info
 * @data:	Received data storage
 */
struct mero_ipc_link {
	enum mbox_owner owner;
	struct mutex lock;
	bool active;
	int irq;
	unsigned int idx;
	unsigned int srcid;
	unsigned int destid;
	unsigned int mode;
	struct mero_ipc_irq ipc_irq;
	u64 dummy; /* for alignment */
	u32 data[(DATA_REG_NUM + 1) * IPC_CHN_NUM];
} __aligned(16);

/*
 * Mailbox controller data
 *
 * @controller:	Representation of a communication channel controller
 * @chans:	Representation of channels in mailbox controller
 * @links:	Representation of channel info
 * @base:	Base address of the register mapping region
 * @irq_requested:	Indicate which irq line has been requested
 * @mbid_mask:	bitfield of mbid_mask[n] tells which mailboxes share irq line n
 * @irqmap:	save map relations from linux-irq-number to mailbox-channel-id
 */
struct mero_ipc {
	struct mbox_controller controller;
	struct mbox_chan chans[IPC_CHN_NUM];
	struct mero_ipc_link links[IPC_CHN_NUM];
	void __iomem *base;
	u32 irq_requested;
	u32 mbid_mask[IPC_CHN_NUM];
	int irqmap[IPC_CHN_NUM];
};

/**
 * struct mero_ipc_dev - IPC mailbox device resources
 * @dev device pointer
 * @base: base register address of ipc controller
 * @nr_mbox: number of mailboxs in use
 * @vecs: interrupt vectors
 * @param: mailbox configurations
 * @ipc: pointer to ipc
 */
struct mero_ipc_dev {
	struct device *dev;
	void __iomem *base;
	u32 nr_mbox;
	u32 vecs[IPC_CHN_NUM];
	u32 param[IPC_CHN_NUM * PARAM_CELLS];
	struct mero_ipc *ipc;
};

/* exported by ipc-controller driver */
int ipc_config_chan(struct mbox_controller *con, unsigned int idx,
		    struct ipc_config *conf);
void ipc_ack_msg(struct mbox_chan *chan, char *buf);
int mero_mailbox_init(struct mero_ipc_dev *ipc_dev);
void mero_ipc_tx_cleanup(struct mbox_chan *chan);
int mero_ipc_restore(struct mero_ipc_dev *ipc_dev);
void mero_ipc_send_interrupt(struct mbox_chan *chan);
void mero_ipc_ack_interrupt(struct mbox_chan *chan);
void ipc_irq_affinity_restore(struct mero_ipc_dev *ipc_dev);

static inline struct mero_ipc *to_mero_ipc(struct mbox_controller *mbox)
{
	return container_of(mbox, struct mero_ipc, controller);
}

/* exported by ipc-dev driver */
#ifdef CONFIG_MERO_MBOX_DEV
int ipc_dev_add(struct mbox_controller *con, unsigned int idx);
int ipc_dev_pre_init(void);
void ipc_dev_cleanup(void);
#else
static inline int ipc_dev_add(struct mbox_controller *con, unsigned int idx)
{
	return 0;
}

static inline int ipc_dev_pre_init(void)
{
	return 0;
}

static inline void ipc_dev_cleanup(void)
{
}
#endif

#endif //_MERO_IPC_I_H_
