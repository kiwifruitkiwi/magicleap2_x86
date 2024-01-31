/* SPDX-License-Identifier: GPL-2.0-only
 *
 * Copyright (c) 2020-2023, Magic Leap, Inc. All rights reserved.
 */

#ifndef __MERO_XPC_UAPI_H
#define __MERO_XPC_UAPI_H

#include <stdalign.h>

#define DEVICE_NAME "mero_xpc"
#define MERO_XPC_MAGIC 'x'
#define MERO_XPC_FILE "/dev/mero_xpc"

#include "mero_xpc_types.h"

struct XpcNotificationInfo {
    uint8_t data[XPC_MAILBOX_DATA_SIZE];
    XpcAsyncTicket ticket[XPC_NUM_IPC_DEVICES];
    int timeout_us;
    uint32_t src_id;
    uint32_t dst_mask;
    XpcNotificationMode mode;
    uint8_t length;
    uint8_t channel;
};

struct XpcCommandInfo {
    uint8_t data[XPC_MAILBOX_DATA_SIZE];
    XpcAsyncTicket ticket;
    uint32_t src_id;
    uint32_t dst_mask;
    uint8_t length;
    uint8_t channel;
};

struct XpcResponseInfo {
    uint8_t data[XPC_MAILBOX_DATA_SIZE];
    int timeout_us;
    XpcAsyncTicket ticket;
    size_t length;
    uint8_t has_error;
};

struct XpcCommandResponseInfo {
    struct XpcCommandInfo command;
    struct XpcResponseInfo response;
};

struct XpcReadMailboxInfo {
    uint8_t data[XPC_MAILBOX_DATA_SIZE];
    XpcAsyncTicket ticket;
    uint8_t length;
    uint8_t channel;
};

struct XpcQueueInfo {
    uint8_t data[XPC_MAILBOX_DATA_SIZE];
    XpcAsyncTicket ticket[XPC_NUM_IPC_DEVICES];
    uint32_t src_id;
    uint32_t dst_mask;
    uint8_t length;
    uint8_t channel;
    uint8_t sub_channel;
};

struct XpcQueueConfigInfo {
    uint8_t channel;
    uint8_t sub_channel;
};

struct XpcMailboxUsageQuery {
    uint32_t mailboxes[XPC_NUM_IPC_DEVICES];
    int query_pid;
    unsigned int total_in_use;
    unsigned int total_restarts;
};

#define XPC_COMMAND_SEND_COMMAND _IOWR(MERO_XPC_MAGIC, 0xA0, struct XpcCommandInfo)

#define XPC_COMMAND_SEND_RESPONSE                                              \
	_IOWR(MERO_XPC_MAGIC, 0xA1, struct XpcResponseInfo)

#define XPC_COMMAND_WAIT_RESPONSE                                              \
	_IOWR(MERO_XPC_MAGIC, 0xA2, struct XpcResponseInfo)

#define XPC_COMMAND_SEND_COMMAND_AND_WAIT_RESPONSE                             \
    _IOWR(MERO_XPC_MAGIC, 0xA3, struct XpcCommandResponseInfo)

#define XPC_COMMAND_WAIT_RECEIVE                                               \
	_IOWR(MERO_XPC_MAGIC, 0xA4, struct XpcReadMailboxInfo)

#define XPC_NOTIFICATION_SEND                                                  \
	_IOWR(MERO_XPC_MAGIC, 0xB0, struct XpcNotificationInfo)

#define XPC_NOTIFICATION_WAIT_RECEIVE                                          \
	_IOWR(MERO_XPC_MAGIC, 0xB1, struct XpcReadMailboxInfo)

#define XPC_QUEUE_SEND _IOW(MERO_XPC_MAGIC, 0xC0, struct XpcQueueInfo)

#define XPC_QUEUE_WAIT_RECEIVE                                                 \
	_IOWR(MERO_XPC_MAGIC, 0xC1, struct XpcQueueInfo)

#define XPC_QUEUE_REGISTER _IOW(MERO_XPC_MAGIC, 0xC2, struct XpcQueueConfigInfo)

#define XPC_QUEUE_DEREGISTER                                                   \
	_IOW(MERO_XPC_MAGIC, 0xC3, struct XpcQueueConfigInfo)

#define XPC_QUERY_MAILBOX_USAGE                                               \
	_IOW(MERO_XPC_MAGIC, 0xC4, struct XpcMailboxUsageQuery)

#ifndef __KERNEL__
#error "Can't include kernel header in userspace"
#endif

#endif
