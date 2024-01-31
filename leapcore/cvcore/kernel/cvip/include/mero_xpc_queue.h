/* SPDX-License-Identifier: GPL-2.0-only
 *
 * Copyright (c) 2020-2022, Magic Leap, Inc. All rights reserved.
 */

#pragma once

#include "mero_xpc_common.h"
#include "mero_xpc_types.h"

extern struct xpc_queue queue_mailbox[XPC_NUM_VCHANNELS];

extern XpcQueueChannelMask registered_user_subchannels;
extern XpcQueueChannelMask registered_kernel_subchannels;
extern spinlock_t registered_queue_lock;

int xpc_send_queue_internal(struct XpcQueueInfo* message,
	enum xpc_client_api_mode client_mode);

int xpc_queue_register_user(XpcChannel vchannel, XpcSubChannel schannel,
	uint8_t* process_subchannels);

int xpc_queue_deregister_user(XpcChannel vchannel, XpcSubChannel schannel,
	uint8_t* process_subchannels);

int xpc_queue_check_and_claim_mailbox(struct xpc_queue* queue,
	XpcChannel xpc_channel,
	XpcSubChannel xpc_sub_channel,
	int* claimed_mailbox);

int xpc_queue_check_user_registration(XpcChannel vchannel,
	XpcSubChannel schannel,
	uint8_t* process_subchannels);

int xpc_queue_check_kernel_registration(XpcChannel vchannel,
	XpcSubChannel schannel);

int xpc_acknowledge_queue_internal(int mailbox_id);

#ifndef __KERNEL__
#error "Can't include kernel header in userspace"
#endif
