/*
 * Header for IPC library types definitions
 *
 * Copyright (C) 2019-2021 Advanced Micro Devices, Inc. All rights reserved.
 */

#ifndef __IPC_TYPES_H__
#define __IPC_TYPES_H__

#include <linux/mailbox_client.h>
#include "ipc_messageid.h"

/* mailbox client rx callback function definition */
typedef void (*mb_rx_callback_t)(struct mbox_client *cl, void *mssg);

#define RX_QUEUE_LEN			256
#define NUM_OF_CVIP_MAILBOX_CHANS	5
#define NUM_OF_ISP_MAILBOX_CHANS	3
#define THREAD_EXIT_TIMEOUT		8000
#define MAX_SYNC_TIMEOUT		(msecs_to_jiffies(120*1000))
#define MAX_MESSAGE_COUNT		10

/* ipc destination */
#define X86				0
#define CVIP				1
#define ISP				2

/* ipc base channel */
#define ISP_CHANNEL_BASE		3
#define ISP_CAM_ID_MASK			0x1

#define MSG_LEN_MASK			GENMASK(31, 24)
#define MSG_SEQNUM_MASK			GENMASK(23, 16)
#define MSG_MESSAGE_ID_MASK		GENMASK(15, 0)
#define MSG_LEN_SHIFT			(24)
#define MSG_SEQNUM_SHIFT		(16)
#define MSG_MESSAGE_ID_SHIFT		(0)
#define MAILBOX_PAYLOAD_LEN		(7)
#define MAX_MAILBOX_MSG_BODY_LEN	(3*MAILBOX_PAYLOAD_LEN - 1)
#define MAX_IPCMSG_PAYLOAD_LEN		(MAILBOX_PAYLOAD_LEN - 1)

enum ipc_test_ids {
	IPC_X86_ALIVE_MSG	= 7,
	IPC_START_INTRTEST_MSG	= 8,
	IPC_STOP_INTRTEST_MSG	= 9,
	IPC_OFF_HEAD_MSG	= 10,
	IPC_ON_HEAD_MSG		= 11,
};

enum isp_test_ids {
	ISP_TEST_NONE		= 0,
	ISP_TEST_PREPARE	= 1,
	ISP_TEST_START		= 10,
	ISP_TEST_FMT		= 20,
	ISP_TEST_STREAMON	= 30,
	ISP_TEST_AGAIN0		= 31,
	ISP_TEST_DQBUF		= 40,
	ISP_TEST_QBUF		= 50,
	ISP_TEST_PREPOST	= 60,
	ISP_TEST_WDMA		= 70,
	ISP_TERR_FW_TOUT	= 100,
	ISP_TERR_QOS_TOUT	= 110,
	ISP_TERR_DROP_FRAME	= 120,
	ISP_TERR_DISORDER_FRAME	= 130,
	ISP_TERR_OVERFLOW	= 140,
	ISP_TERR_CAM_RESET	= 150,
	ISP_TEST_STREAMOFF	= 200,
	ISP_TEST_STOP		= 210,
};

/* bit for condition check, figure that we don't need too many bit in future */
#define ISP_COND_PREPARE		0
#define ISP_COND_WAIT_STREAMON		1
#define ISP_COND_WAIT_AGAIN0		2
#define ISP_COND_DQBUF_WAIT_FRAME	3
#define ISP_COND_DQBUF_WAIT_STATS	4
#define ISP_COND_QBUF_WAIT		5
#define ISP_COND_PREPOST_WAIT_PRE	6
#define ISP_COND_PREPOST_WAIT_POST	7
#define ISP_COND_WAIT_WDMA		8
#define ISP_CERR_FW_TOUT_WAIT_DROP	9
#define ISP_CERR_FW_TOUT_WAIT_RECOVER	10
#define ISP_CERR_QOS_TOUT_WAIT_DROP	11
#define ISP_CERR_QOS_TOUT_WAIT_RECOVER	12
#define ISP_CERR_DROP_FRAME_WAIT	13
#define ISP_CERR_DISORDER_WAIT_SKIP	14
#define ISP_CERR_DISORDER_WAIT_DROP	15
#define ISP_CERR_DISORDER_WAIT_RECOVER	16
#define ISP_CERR_OVERFLOW_WAIT		17
#define ISP_COND_WAIT_STREAMOFF		18
#define ISP_COND_MAX			19

/**
 * struct mbox_message - mbox payload
 * first 32bits are used as message id as the following format:
 * | length | seq_number | message_id |
 * @message_id: message id
 * @msg_body:   message payload
 */
struct mbox_message {
	u32 message_id;
	u32 msg_body[MAX_MAILBOX_MSG_BODY_LEN];
};

/**
 * struct ipc_message_range - ipc client callback range definition
 * if received message id is sit between this range,
 * corrensponding client callback will be called.
 * @start:      start message idx
 * @end:        end message idx
 */
struct ipc_message_range {
	u32 start;
	u32 end;
};

/* ipc client callback function definition */
typedef void client_callback_t(void *message);

/**
 * struct rx_queue - ipc receive queue
 * @queue_list:		received message queue list
 * @msg:		mbox message
 */
struct rx_queue {
	struct list_head queue_list;
	struct mbox_message msg;
};

#define to_rx_queue(x)		container_of(x, struct rx_queue, msg)

/**
 * struct message_list - ipc sync_message list
 * @list_node:  sync message list.
 * @msg:	sync message.
 * @done:	message is handled and responsed from remote
 *		used for sync ipc communication.
 */
struct message_list {
	struct list_head list_node;
	struct mbox_message msg;
	struct completion done;
};

/**
 * struct ipc_chans - ipc chan
 * @cl:			mbox client.
 * @chan:		mbox channel pointer.
 * @sync:		flag to indicates whether this channel is synchronized.
 * @rx_pending:		pending sync message list.
 * @msg_list:		transfer message list.
 * @msg:		mbox message.
 * @rx_lock:		lock for sync message list.
 * @xfer_lock:		lock for mbox message list.
 */
struct ipc_chan {
	struct mbox_client cl;
	struct mbox_chan *chan;
	bool sync;
	struct list_head rx_pending;
	struct list_head msg_list;
	struct message_list *msg;
	spinlock_t rx_lock;
	struct mutex xfer_lock;
};

/**
 * struct isp_chans - isp chan
 * @cl:			mbox client.
 * @chan:		mbox channel pointer.
 */
struct isp_chan {
	struct mbox_client cl;
	struct mbox_chan *chan;
};

/**
 * struct callback_list - ipc client callback list
 * @cb_list:            client callback list.
 * @range:		client callback range.
 * @cb_func:		client callback function.
 */
struct callback_list {
	struct list_head cb_list;
	struct ipc_message_range range;
	client_callback_t *cb_func;
};

/**
 * struct ipc_message - ipc message definition
 * @dest:	ipc message destination
 * @channel_idx:ipc channel used, if not specify, async tx channel will be used
 * @length:     message length
 * @message_id: ipc message id.
 * @payload:	message payload
 * @tx_block:	send IPC message until response
 * @tx_tout:	time set for timeout block. default MAX_SYNC_TIMEOUT
 */
struct ipc_message {
	u32 dest;
	u32 channel_idx;
	u32 length;
	u32 message_id;
	u32 payload[MAX_IPCMSG_PAYLOAD_LEN];
	u32 tx_block;
	u32 tx_tout;
};

/**
 * struct ipc_context - ipc library context
 * @rx_queue_lock:		lock for receive queue
 * @rx_queue:			receive queue
 * @rx_head:			receive queue head
 * @seq_lock:			lock for sequence number
 * @seq:			sequence number
 * @cb_list_lock:		lock for client callback
 * @cb_list:			client callback list
 * @cb_list_head:		client callback list head
 * @channels:			list of ipc channels
 * @isp_channels:		list of isp channels
 * @num_ipc_chans:		number of ipc channels
 * @num_isp_chans:		number of isp channels
 * @rx_worker_thread:		receive worker thread
 * @thread_started:		receive thread started event
 * @thread_exited:		receive thread exit event
 * @rx_ring_worker_thread:	ringbuffer worker thread
 * @ring_base:			ringbuffer base
 * @wp:				ringbuffer write pointer
 * @fence_base:			fence base address
 * @fence_index:		fence index
 */
struct ipc_context {
	spinlock_t rx_queue_lock;
	struct rx_queue rx_queue;
	struct list_head head;
	spinlock_t seq_lock;
	u8 seq;

	spinlock_t cb_list_lock;
	struct callback_list cb_list;
	struct list_head cb_list_head;

	struct task_struct *rx_worker_thread;
	struct completion thread_started;
	struct completion thread_exited;

	struct ipc_chan *channels;
	struct isp_chan *isp_channels;
	u32 num_ipc_chans;
	u32 num_isp_chans;
#ifdef CONFIG_RINGBUFFER
	struct task_struct *rx_ring_worker_thread;
	u32 ring_base;
	u32 wp;
	u32 fence_base;
	u32 fence_index;
#endif
};

#endif /* __IPC_TYPES_H__ */
