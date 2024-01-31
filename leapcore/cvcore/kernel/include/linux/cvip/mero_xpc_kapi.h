/* SPDX-License-Identifier: GPL-2.0-only
 *
 * \file mero_xpc_kapi.h
 *
 * \brief  mero_xpc API definitions.
 *
 * Copyright (c) 2020-2023, Magic Leap, Inc. All rights reserved.
 */

#ifndef __MERO_XPC_KAPI_H
#define __MERO_XPC_KAPI_H

#include <linux/kernel.h>
#include <linux/types.h>

#include "mero_xpc.h"
#include "mero_xpc_types.h"

int xpc_register_notification_handler(XpcChannel channel,
	XpcNotificationHandler handler,
	XpcHandlerArgs args);

int xpc_register_notification_handler_hi(XpcChannel channel,
	XpcNotificationHandler handler,
	XpcHandlerArgs args);

int xpc_register_command_handler(XpcChannel channel,
	XpcCommandHandler handler,
	XpcHandlerArgs args);

int xpc_register_command_handler_hi(XpcChannel channel,
	XpcCommandHandler handler,
	XpcHandlerArgs args);

int xpc_command_send_no_response(XpcChannel channel, XpcTarget target,
	uint8_t* command, size_t command_length);

#ifndef __KERNEL__
#error "Can't include kernel header in userspace"
#endif

#endif
