/* SPDX-License-Identifier: GPL-2.0-only
 *
 * Copyright (c) 2022-2023, Magic Leap, Inc. All rights reserved.
 */

#ifndef __MERO_XPC_COMMON_H
#define __MERO_XPC_COMMON_H

#include <linux/completion.h>
#include <linux/interrupt.h>
#include <linux/spinlock_types.h>
#include <linux/wait.h>

#include "mero_xpc_kapi.h"
#include "mero_xpc_types.h"
#include "mero_xpc_uapi.h"
#include "pl320_types.h"

#define TIMEOUT_INFINITE (-1)

#define NUM_MAILBOXES (XPC_IPC_NUM_MAILBOXES * XPC_NUM_IPC_DEVICES)

#define IPC0_ADDRESS (0xDA000000U)
#define IPC1_ADDRESS (0xDA800000U)
#define IPC2_ADDRESS (0xDB000000U)
#define IPC3_ADDRESS (0xDB800000U)
#define IPC4_ADDRESS (0xDC000000U)

#define MODULE_NAME "mero_xpc"

#define XPC_REQUEST_IS_TARGET (true)
#define XPC_REQUEST_IS_SOURCE (!(XPC_REQUEST_IS_TARGET))

#define XPC_REMASK(mask)                                                     \
	(uint16_t)(((mask) & 0xFF00) |                                           \
		   (uint8_t)((((mask) & 0x3) << 6) | (((mask) & 0xFC) >> 2)))

#define XPC_UNREMASK(mask)                                                   \
	(uint16_t)(((mask) & 0xFF00) |                                           \
		   (uint8_t)((((mask) & 0xC0) >> 6) | (((mask) & 0x3F) << 2)))

 /**
  * Calculates offset of vchannel/schannel for use with
  * XpcQueueChannelMask type.
  */
#define CHANNEL_MASK_OFFSET(vchannel, schannel)                                \
	(((vchannel) * XPC_NUM_SCHANNELS / 8) + ((schannel) / 8))

  // Creates sub channel mask byte
#define SUB_CHANNEL_MASK(schannel) (1 << ((schannel)&0x7))

// Calculates the index of the byte this schannel is located at
#define SUB_CHANNEL_BYTE(schannel) ((schannel) / 8)

// Using -1 to represent an invalid XPC client Process ID (PID)
#define XPC_INVALID_CLIENT_PID  (-1)
#define XPC_VALID_CLIENT_PID    (((XPC_INVALID_CLIENT_PID) + 1))

extern int xpc_debug_flag;

#define xpc_debug(level, fmt, arg...)                                          \
    do {                                                                       \
        if (level <= xpc_debug_flag)                                           \
            pr_info(MODULE_NAME ": " fmt, ##arg);                              \
    } while (0);

typedef enum {
	XPC_ERROR_RECV_ERROR = 0,
} XPC_ERROR;

typedef enum {
	MAILBOX_STATE_FREE,
	// XPC Command related states
	MAILBOX_STATE_COMMAND_SENT,
	MAILBOX_STATE_COMMAND_WAIT_RESPONSE,
	MAILBOX_STATE_COMMAND_WAIT_NO_RESPONSE,
	MAILBOX_STATE_COMMAND_WAIT_INTERRUPTED,
	MAILBOX_STATE_COMMAND_PENDING,
	MAILBOX_STATE_COMMAND_CLAIMED,
	MAILBOX_STATE_COMMAND_RESPONDED,
	// XPC Notification related states
	MAILBOX_STATE_NOTIFICATION_WAIT_POSTED,
	MAILBOX_STATE_NOTIFICATION_WAIT_NON_POSTED,
	MAILBOX_STATE_NOTIFICATION_PENDING,
	MAILBOX_STATE_NOTIFICATION_CLAIMED,
	MAILBOX_STATE_NOTIFICATION_ACKED,
	MAILBOX_STATE_NOTIFICATION_WAIT_INTERRUPTED,
	// XPC Queue related states
	MAILBOX_STATE_QUEUE_SENT,
	MAILBOX_STATE_QUEUE_PENDING,
	MAILBOX_STATE_QUEUE_CLAIMED,
	MAILBOX_STATE_QUEUE_ACKED
} MAILBOX_STATE;

/**
 * XPC Message types
 */
enum xpc_platform_channel_message_type {
	XPC_XSTAT_REQ,
	XPC_XSTAT_RSP,
	XPC_XSTAT_STATE_CHANGE_CMD,
	XPC_XSTAT_STATE_CHANGE_RSP,
	XPC_MSG_UNKNOWN
};

/**
 * An enum for describing the type of
 * transaction to which a message belongs
 */
typedef enum {
	XPC_MODE_COMMAND_RESPONSE = 0x0,
	XPC_MODE_NOTIFICATION = 0x1,
	XPC_MODE_QUEUE = 0x2,
	XPC_MODE_COMMAND = 0x3
} XpcMode;

/**
 * A union that represents the first 4 bytes
 * of a message packet
 */
typedef union {
	// A structure that defines the single byte
	// header of a message packet.
	struct {
		XpcChannel vchannel : 4;
		XpcMode mode : 2;
		uint8_t rsvd : 1;
		uint8_t error : 1;
	} fields;
	uint8_t byte_;
} XpcMessageHeader;

/**
 * A bitmask for registering on vchannel/schannel for xpc queue mode.
 */
typedef uint8_t XpcQueueChannelMask[XPC_NUM_VCHANNELS][XPC_NUM_SCHANNELS / 8];

/**
 * struct xpc_client_t
 *
 * A structure used to represent each client that has access to xpc. Will
 * contain any information about the client.
 *
 * @registered_subchannels: A bit mask of subchannels this client is registered on.
 */
struct xpc_client_t {
	XpcQueueChannelMask registered_subchannels;
	volatile unsigned long active_sends[XPC_NUM_IPC_DEVICES];
	volatile unsigned long active_recvs[XPC_NUM_IPC_DEVICES];
	pid_t tid;
	bool use_kernel_api;
};

enum xpc_client_api_mode {
	XPC_CLIENT_IS_UNKNOWN,
	XPC_CLIENT_IS_DIRECT,
	XPC_CLIENT_IS_INDIRECT
};

struct xpc_info {
	XpcMode xpc_mode;
	int32_t xpc_channel;
	int32_t global_mailbox_number;
	int32_t error;
	XpcSubChannel xpc_sub_channel;
} __attribute__((aligned));

struct xpc_mailbox_info {
	Pl320MailboxInfo pl320_info;
	XpcChannel xpc_channel;
	Pl320MailboxAckMode xpc_mode;
	int8_t global_mailbox_number;
	uint32_t send_register;
	MAILBOX_STATE source_state;
	MAILBOX_STATE target_state;
	pid_t source_pid;
	pid_t target_pid;
	enum xpc_client_api_mode source_client_mode;
	enum xpc_client_api_mode target_client_mode;
	int source_restarts;
	int target_restarts;
	atomic_t status;
};

union xpc_data {
	uint32_t u32[XPC_MAILBOX_DATA_SIZE / 4];
	uint16_t u16[XPC_MAILBOX_DATA_SIZE / 2];
	uint8_t u8[XPC_MAILBOX_DATA_SIZE];
};

struct xpc_mailbox {
	spinlock_t mailbox_lock;
	wait_queue_head_t wait_queue;
	struct completion target_replied;
	union xpc_data mailbox_data;
	struct xpc_mailbox_info mailbox_info;
	struct tasklet_struct tasklet;
};

struct xpc_route {
	int ipc_id;
	int source_id;
	uint32_t source_mask;
	uint32_t destination_mask;
};

struct xpc_route_info {
	uint8_t ipc_id;
	uint32_t ipc_source_irq_mask;
	uint32_t ipc_target_irq_mask;
};

struct xpc_route_solution {
	uint8_t valid_routes;
	struct xpc_route_info first_route;
	struct xpc_route_info routes[5];
};

struct xpc_queue {
	atomic_t mailbox_count;
	// spinlock_t lock;
	wait_queue_head_t wait_queue;
};

union __attribute__((packed)) xpc_platform_channel_message {
	uint8_t bytes[XPC_MAX_COMMAND_LENGTH];
	struct {
		uint8_t xpc_command_type;
		uint8_t source_core;
		uint8_t target_core;
		uint8_t message_data[XPC_MAX_COMMAND_LENGTH - 3];
	};
};

// Sanity check that the size is as expected
static_assert(sizeof(union xpc_platform_channel_message) ==
	XPC_MAX_COMMAND_LENGTH);

int xpc_platform_channel_command_handler(
	XpcChannel channel, XpcHandlerArgs args, uint8_t* command_buffer,
	size_t command_buffer_length, uint8_t* response_buffer,
	size_t response_buffer_length, size_t* response_length);

void xpc_initialize_global_mailboxes(void);
void xpc_close_global_mailbox(int mailbox_idx);
void xpc_close_ipc_mailbox(int ipc_id, int mailbox_idx);
int xpc_wait_mailbox_reply(int global_mailbox_number, int* timeout_us);
int xpc_check_mailbox_count(struct xpc_queue* queue_ptr);
int xpc_get_global_mailbox(struct xpc_mailbox_info* info);
void xpc_get_ipc_id_and_mailbox(int global_mailbox_idx, int* ipc_id,
	int* mailbox_idx);
void xpc_get_global_mailbox_idx_from_ipc_id_and_mailbox_idx(
	int ipc_id, int mailbox_idx, int* global_mailbox_idx);
struct xpc_mailbox*
	xpc_get_global_mailbox_from_ipc_id_and_mailbox_idx(int ipc_id, int mailbox_idx);
struct xpc_mailbox*
	xpc_get_global_mailbox_from_global_mailbox_idx(int global_mailbox_idx);
int xpc_claim_global_mailbox(struct xpc_info info);
int xpc_hal_status_to_errno(Pl320HalStatus status);
bool xpc_initialized(void);
void xpc_set_global_mailbox_pid(int global_mailbox_idx, pid_t pid,
	bool is_target,
	enum xpc_client_api_mode client_mode);
void xpc_clr_global_mailbox_pid(int global_mailbox_idx, bool is_target);
bool xpc_check_indirect_clients_active(void);
bool xpc_check_global_mailbox_pid(int global_mailbox_idx, pid_t pid,
	bool is_target);
void xpc_print_global_mailbox(int global_mailbox_idx, bool is_target);
extern struct xpc_mailbox mailboxes[NUM_MAILBOXES];
extern unsigned long enabled_mailboxes;
extern spinlock_t registered_queue_lock;
extern spinlock_t notification_waits_lock;
extern spinlock_t command_waits_lock;

static inline
uint8_t mask_32b_to_idx(uint32_t mask) {
	uint8_t t;

	if (mask == 0)
		return 0xFF;

	for (t = 0; t < 32; t++) {
		if ((mask >> t) & 0x1) {
			return t;
		}
	}
	return 0xFF;
}

#ifndef __KERNEL__
#error "Can't include kernel header in userspace"
#endif

#endif
