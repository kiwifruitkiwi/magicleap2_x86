/* SPDX-License-Identifier: GPL-2.0-only
 *
 * Copyright (c) 2020-2022, Magic Leap, Inc. All rights reserved.
 */

#ifndef __PL320_TYPES_H
#define __PL320_TYPES_H

#include <linux/types.h>

 /**
  * Structure for representing a single pl320 device
  */
typedef struct {
	uint64_t paddress;
	uint32_t mailbox_enabled;
	uint32_t source_channels;
	uint32_t channel_id_mask[32];
	uint8_t ipc_id;
	uint8_t is_enabled : 1;
	// GSM extensions
	bool gsm_ext_inited;
	uintptr_t ipc_ext_mailbox_base;
	uintptr_t ipc_ext_global_base;
	bool use_fast_ext;
	bool use_safe_ext;
} pl320_device_info;

typedef enum {
	PL320_MAILBOX_MODE_AUTO_ACK = 0x1,
	PL320_MAILBOX_MODE_AUTO_LINK = 0x2,
	PL320_MAILBOX_MODE_DEFAULT = 0x0,
	PL320_MAILBOX_MODE_UNKNOWN = 0xF
} Pl320MailboxAckMode;

typedef enum {
	PL320_IRQ_REASON_UNKNOWN = 0xFF,
	PL320_IRQ_REASON_NONE = 0x0,
	PL320_IRQ_REASON_DESTINATION_INT = 0x1,
	PL320_IRQ_REASON_SOURCE_ACK = 0x2
} Pl320MailboxIRQReason;

typedef enum {
	PL320_STATUS_SUCCESS = 0x00,
	PL320_STATUS_FAILURE_DEVICE_UNAVAILABLE,
	PL320_STATUS_FAILURE_DEVICE_INVALID,
	PL320_STATUS_FAILURE_CHANNEL_UNAVAILABLE,
	PL320_STATUS_FAILURE_MAILBOX_UNAVAILABLE,
	PL320_STATUS_FAILURE_MAILBOX_OFFLINE,
	PL320_STATUS_FAILURE_NULL_POINTER,
	PL320_STATUS_FAILURE_ALIGNEMENT,
	PL320_STATUS_FAILURE_DATA_LENGTH,
	PL320_STATUS_FAILURE_DATA_NULL,
	PL320_STATUS_FAILURE_DATA_LOAD,
	PL320_STATUS_FAILURE_INVALID_PARAMETER,
	PL320_STATUS_FAILURE_INVALID_DESTINATION,
	PL320_STATUS_FAILURE_INITIALIZATION,
	PL320_STATUS_FAILURE_UNSPECIFIED
} Pl320HalStatus;

typedef struct {
	uint8_t ipc_id;
	uint64_t ipc_paddress;
	uint32_t ipc_irq_id;
	uint8_t mailbox_idx;
	Pl320MailboxAckMode mailbox_mode;
	uint32_t mailbox_irq_mask;
	Pl320MailboxIRQReason irq_cause;
	uint32_t sequence_id;
} Pl320MailboxInfo;

typedef struct {
	int num_in_use;
	uint8_t ipc_id;
	uint32_t irq_mask[32];
	uint32_t irq_raw[32];
	uint32_t active_source[32];
	uint32_t active_target[32];
	uint32_t data_reg_0_1_2_6[32][4];
	uint32_t channel_id_mask[32];
	uint32_t mode_reg[32];
	uint32_t mstatus_reg[32];
	uint32_t send_reg[32];

} pl320_debug_info;

#define PL320_MAILBOX_INFO_STRUCT_INIT(p)                                      \
	do {                                                                   \
		p.ipc_id = 0xFF;                                               \
		p.ipc_paddress = 0xFFFFFFFFFFFFFFFF;                           \
		p.ipc_irq_id = 0xFFFFFFFF;                                     \
		p.mailbox_idx = 0xFF;                                          \
		p.mailbox_mode = PL320_MAILBOX_MODE_UNKNOWN;                   \
		p.mailbox_irq_mask = 0xFFFFFFFF;                               \
		p.irq_cause = PL320_IRQ_REASON_UNKNOWN;                        \
		p.sequence_id = 0;                                             \
	} while (0)

#ifndef __KERNEL__
#error "Can't include kernel header in userspace"
#endif

#endif
