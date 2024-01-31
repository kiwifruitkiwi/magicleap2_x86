/* SPDX-License-Identifier: GPL-2.0-only
 *
 * Copyright (c) 2020-2023, Magic Leap, Inc. All rights reserved.
 */

#ifndef __MERO_XPC_NOTIFICATION_H
#define __MERO_XPC_NOTIFICATION_H

#include "mero_xpc_common.h"
#include "mero_xpc_types.h"

extern XpcNotificationHandler notification_handlers[XPC_NUM_VCHANNELS];
extern XpcHandlerArgs notification_handler_args[XPC_NUM_VCHANNELS];
extern struct xpc_queue notification_mailbox[XPC_NUM_VCHANNELS];

int xpc_send_notification_internal(struct XpcNotificationInfo* message,
	enum xpc_client_api_mode client_mode);
int xpc_acknowledge_notification_internal(int mailbox_id);
bool xpc_notification_channel_is_hi_priority(int channel);

#ifndef __KERNEL__
#error "Can't include kernel header in userspace"
#endif

#endif
