/* SPDX-License-Identifier: GPL-2.0-only
 *
 * Copyright (c) 2020-2022, Magic Leap, Inc. All rights reserved.
 */

#ifndef __MERO_XPC_TYPES_H
#define __MERO_XPC_TYPES_H

#include <linux/types.h>

#define XPC_MAILBOX_DATA_SIZE (28)
#define XPC_MAILBOX_HEADER_LOC (XPC_MAILBOX_DATA_SIZE - 1)
#define XPC_MAX_PAYLOAD_LENGTH (XPC_MAILBOX_DATA_SIZE - 1)
#define XPC_MAX_COMMAND_LENGTH (XPC_MAILBOX_DATA_SIZE - 1)
#define XPC_MAX_NOTIFICATION_LENGTH (XPC_MAILBOX_DATA_SIZE - 1)
#define XPC_MIN_NOTIFICATION_LENGTH (XPC_MAILBOX_DATA_SIZE - 1)
#define XPC_MAX_QUEUE_LENGTH (XPC_MAILBOX_DATA_SIZE-2)
#define XPC_MIN_QUEUE_LENGTH (XPC_MAILBOX_DATA_SIZE-2)
#define XPC_MAX_RESPONSE_LENGTH (XPC_MAILBOX_DATA_SIZE - 1)
#define XPC_IPC_ID_INVALID (0xFF)
#define XPC_IPC_CHANNEL_INVALID (0xFF)
#define XPC_IPC_MAILBOX_INVALID (0xFF)
#define XPC_IPC_NUM_CHANNELS (32)
#define XPC_IPC_NUM_MAILBOXES (32)
#define XPC_IRQ_INVALID (0xFF)
#define XPC_TICKET_INVALID (-1)
#define XPC_NUM_DESTINATIONS (16)
#define XPC_DESTINATIONS_NONE (0x0)
#define XPC_NUM_IPC_DEVICES (5)
#define XPC_NUM_CORES (10)
#define XPC_NUM_VCHANNELS (16)

#define XPC_NUM_SCHANNELS (256)
#define XPC_SUB_CHANNEL_BYTE (26)

#define XPC_NUM_SOURCE_CORES (16)
#define XPC_NUM_TARGET_CORES (16)

#define CORE_ID_MIN (0)
#define CORE_ID_A55 (8)
#define CORE_ID_X86 (9)
#define CORE_ID_MAX (9)

#define XPC_MIN_CORE_ID (0x0)
#define XPC_MAX_CORE_ID (0xF)

#define IPC0_IDX (0)
#define IPC1_IDX (1)
#define IPC2_IDX (2)
#define IPC3_IDX (3)
#define IPC4_IDX (4)

#define IPC0_MASK (0x1 << IPC0_IDX)
#define IPC1_MASK (0x1 << IPC1_IDX)
#define IPC2_MASK (0x1 << IPC2_IDX)
#define IPC3_MASK (0x1 << IPC3_IDX)
#define IPC4_MASK (0x1 << IPC4_IDX)

 /**
  * Arguments for various xpc related callbacks
  */
typedef void* XpcHandlerArgs;

/**
 * Specifies one of 16 application channels that
 * a command or notification may be sent.
 */
typedef uint8_t XpcChannel;

/**
 * Specifies one of 256 application sub channels that
 * a queue may be sent.
 */
typedef uint8_t XpcSubChannel;

/**
 * A mask that specifies one or more target cores.
 */
typedef uint32_t XpcTargetMask;

/**
 * An index that specifies a single target core for
 * command send operations.
 */
typedef uint8_t XpcTarget;

/**
 * A ticket used to relate split command/response
 * operations. The ticket is initialized by an
 * asynchronous send operation and may be used to
 * synchronize on a future response.
 */
typedef int32_t XpcAsyncTicket;

/**
 * Callback definition for a command handler that is invoked on arrival of
 * commands. The callback may fill in a supplied response buffer with data to be
 * returned as a reply to the initiator of the command.
 */
typedef int (*XpcCommandHandler)(XpcChannel channel, XpcHandlerArgs args,
	uint8_t* command_buffer,
	size_t command_buffer_length,
	uint8_t* response_buffer,
	size_t resize_buffer_length,
	size_t* response_length);

/**
 * Callback definition for a notification handler that is invoked on arrival of
 * notifications. The callback is passed a buffer filled with the notification
 * data.
 */
typedef int (*XpcNotificationHandler)(XpcChannel channel, XpcHandlerArgs args,
	uint8_t* notification_buffer,
	size_t notification_buffer_length);

/**
 * Enumeration that specifies notification delivery mode.
 */
typedef enum {
	XPC_NOTIFICATION_MODE_WAIT_ACK, /**< Wait for all recipients to acknowledge.
									 */
									 XPC_NOTIFICATION_MODE_POSTED    /**< Do not wait for any recipients to
																		acknowledge. */
} XpcNotificationMode;

#ifndef __KERNEL__
#error "Can't include kernel header in userspace"
#endif

#endif
