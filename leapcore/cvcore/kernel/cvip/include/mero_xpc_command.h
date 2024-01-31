/* SPDX-License-Identifier: GPL-2.0-only
 *
 * Copyright (c) 2020-2023, Magic Leap, Inc. All rights reserved.
 */

#ifndef __MERO_XPC_COMMAND_H
#define __MERO_XPC_COMMAND_H

#include "mero_xpc_common.h"
#include "mero_xpc_types.h"

extern XpcCommandHandler command_handlers[XPC_NUM_VCHANNELS];
extern XpcHandlerArgs command_handler_args[XPC_NUM_VCHANNELS];
extern struct xpc_queue command_mailbox[XPC_NUM_VCHANNELS];

int xpc_send_command_internal(struct XpcCommandInfo* message,
	bool expect_response,
	enum xpc_client_api_mode client_mode);
int xpc_wait_command_response_internal(struct XpcResponseInfo* message);
int xpc_send_command_response_internal(struct XpcResponseInfo* message);
bool xpc_command_channel_is_hi_priority(int channel);
#endif

#ifndef __KERNEL__
#error "Can't include kernel header in userspace"
#endif
