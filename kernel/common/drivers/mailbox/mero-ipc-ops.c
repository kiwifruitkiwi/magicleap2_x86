/*
 * Copyright (C) 2021 Advanced Micro Devices, Inc. All rights reserved.
 */
#include <linux/types.h>
#include <linux/err.h>
#include <linux/device.h>
#include <linux/io.h>
#include <linux/mailbox_controller.h>
#include <linux/mailbox_client.h>
#include "mero_ipc_i.h"

#define IPC_SEND(m)		(((m) * 0x40) + 0x020)

void mero_ipc_send_interrupt(struct mbox_chan *chan)
{
	struct mero_ipc_link *link = chan->con_priv;
	struct mero_ipc *ipc = to_mero_ipc(chan->mbox);

	writel_relaxed(SEND_REG_SET, ipc->base + IPC_SEND(link->idx));
}

void mero_ipc_ack_interrupt(struct mbox_chan *chan)
{
	struct mero_ipc_link *link = chan->con_priv;
	struct mero_ipc *ipc = to_mero_ipc(chan->mbox);

	writel_relaxed(SEND_REG_ACK, ipc->base + IPC_SEND(link->idx));
}
