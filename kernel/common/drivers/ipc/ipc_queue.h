/*
 * Header file for IPC library queue implementation
 *
 * Copyright (C) 2019 Advanced Micro Devices, Inc. All rights reserved.
 */

#ifndef __IPC_QUEUE_H__
#define __IPC_QUEUE_H__

#include "ipc_types.h"

int queue_init(struct ipc_context *ipc);
int queue_cleanup(struct ipc_context *ipc);
int enqueue(struct mbox_message *msg, struct ipc_context *ipc);
struct mbox_message *dequeue(struct ipc_context *ipc);

#endif /* __IPC_QUEUE_H__ */
