/*
 * IPC library receive queue implementation
 *
 * Copyright (C) 2019-2021 Advanced Micro Devices, Inc. All rights reserved.
 */

#include "ipc.h"

int queue_init(struct ipc_context *ipc)
{
	INIT_LIST_HEAD(&ipc->head);
	return 0;
}

int queue_cleanup(struct ipc_context *ipc)
{
	struct rx_queue *node;

	if (list_empty(&ipc->head))
		return 0;
	spin_lock(&ipc->rx_queue_lock);
	list_for_each_entry(node, &ipc->head, queue_list) {
		list_del(&node->queue_list);
		kfree(node);
	}
	spin_unlock(&ipc->rx_queue_lock);
	return 0;
}

/* enqueue message */
int enqueue(struct mbox_message *msg, struct ipc_context *ipc)
{
	struct rx_queue *node;
	unsigned long flags;

	node = kmalloc(sizeof(struct rx_queue), GFP_ATOMIC);
	if (!node)
		return -ENOMEM;

	memcpy(&node->msg, msg, sizeof(struct mbox_message));
	spin_lock_irqsave(&ipc->rx_queue_lock, flags);
	list_add_tail(&node->queue_list, &ipc->head);
	spin_unlock_irqrestore(&ipc->rx_queue_lock, flags);
	return 0;
}

/* dequeue message */
struct mbox_message *dequeue(struct ipc_context *ipc)
{
	struct rx_queue *node;
	unsigned long flags;

	if (list_empty(&ipc->head))
		return NULL;
	spin_lock_irqsave(&ipc->rx_queue_lock, flags);
	node = list_first_entry(&ipc->head,
			struct rx_queue, queue_list);
	list_del(&node->queue_list);
	spin_unlock_irqrestore(&ipc->rx_queue_lock, flags);
	return &(node->msg);
}
