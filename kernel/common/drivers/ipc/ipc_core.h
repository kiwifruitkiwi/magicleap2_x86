/*
 * Header file for IPC library core implementation
 *
 * Copyright (C) 2019-2020 Advanced Micro Devices, Inc. All rights reserved.
 */

#ifndef __IPC_CORE_H__
#define __IPC_CORE_H__

#include "ipc_types.h"
#include "ipc_isp.h"

bool match(struct ipc_message_range *range,
	   struct callback_list *node);
bool overlap(struct ipc_message_range *range, struct callback_list *node);
struct mbox_message *get_mbox_message(struct ipc_chan *chan);
void put_mbox_message(struct ipc_chan *chan, struct mbox_message *msg);
void log_message(struct mbox_message *msg);
int prepare_message(struct ipc_context *ipc, struct ipc_chan *chan,
		    struct mbox_message *mbox_msg, struct ipc_message *msg);
struct ipc_chan *message_to_mailbox(struct ipc_context *ipc,
				    struct ipc_message *msg);
client_callback_t cb1;
client_callback_t cb2;
#ifdef CONFIG_IPC_XPORT
client_callback_t handle_xport_req;
#endif
void dump_cblist(struct ipc_context *ipc);
int alloc_xfer_list(struct ipc_chan *chan);
int dealloc_xfer_list(struct ipc_chan *chan);
void mbox_tx_done(struct mbox_client *cl, void *mssg, int r);
void cvip_mbx_ringCallback(struct mbox_client *client, void *message);
void cvip_mb1_rx_callback(struct mbox_client *client, void *message);
void cvip_mb2_tx_callback(struct mbox_client *client, void *message);
void cvip_mb3_rx_callback(struct mbox_client *client, void *message);
void x86_mbx_ringCallback(struct mbox_client *client, void *message);
void x86_mbx_rxCallback(struct mbox_client *client, void *message);
void x86_rx_callback(struct mbox_client *client, void *message);
void x86_mb1_tx_callback(struct mbox_client *client, void *message);
void x86_mb2_rx_callback(struct mbox_client *client, void *message);
void x86_mb3_tx_callback(struct mbox_client *client, void *message);
void smu_rx_callback(struct mbox_client *client, void *message);

int handle_isp_client_callback(struct isp_ipc_device *ipc, u32 msg_id,
			       void *msg);
int handle_client_callback(struct ipc_context *ipc, struct mbox_message *msg);
int handle_panic(struct mbox_message *msg);
int handle_alive(struct mbox_message *msg);
int handle_message(struct mbox_message *msg);
int rx_worker_thread_func(void *data);
int rx_ring_worker_thread_func(void *data);

#ifdef CONFIG_IPC_XPORT
#define ISP2CVIPADDR(addr) ((addr) + MERO_CVIP_ADDR_XPORT_BASE)
#else
#define ISP2CVIPADDR(addr) (addr)
#endif

#endif /* __IPC_CORE_H__ */

