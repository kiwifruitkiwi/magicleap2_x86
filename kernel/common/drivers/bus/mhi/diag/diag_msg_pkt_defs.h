/* SPDX-License-Identifier: BSD-3-Clause-Clear */
/**
 * Copyright (c) 2019 The Linux Foundation. All rights reserved.
 */

#ifndef DIAG_MSG_PKT_DEFS_H
#define DIAG_MSG_PKT_DEFS_H

#include <linux/types.h>

#define DIAG_MSG_CMD_CODE_OFFSET    0
#define DIAG_MSG_SUB_CMD_OFFSET     1

#pragma pack(1)

/**
 * Structure that defines the request packet sent by external device
 * to set the run-time masks corresponding to a range of SSIDs.
 *
 * command-code DIAG_EXT_MSG_CONFIG_F (125)
 * sub-command MSG_EXT_SUBCMD_SET_RT_MASK (4)
 */

struct msg_set_rt_mask_req_type {
	u8  cmd_code;      /* Command code */
	u8  sub_cmd;       /* Message Sub-command */
	u16 ssid_start;    /* Start of subsystem ID range */
	u16 ssid_end;      /* End of subsystem ID range */
	u16 pad;           /* Unused */
	u32 rt_mask[1];    /* Array of runtime masks for subsystems in range. */
			     /* Array size is: 'ssid_end - ssid_start + 1' */
} __packed;

struct msg_set_toggle_config_req_type {
	u8 cmd_code;
	u8 enable;
} __packed;

struct msg_set_event_mask_req_type {
	u8 cmd_code;
	u8 status;
	u16 padding;
	u16 num_bits;
} __packed;

struct msg_set_log_mask_req_type {
	u8 cmd_code;
	u8 padding[3];
	u32 sub_cmd;
	u32 equip_id;
	u32 num_items;
} __packed;

/**
 * This is the message HEADER type. It contains the beginning fields of the
 * packet and is of fixed length. These fields are filled by the calling task.
 */
struct msg_hdr_type {
	u8  cmd_code;    /* Command code */
	u8  ts_type;     /* Time stamp type */
	u8  num_args;    /* Number of arguments in message */
	/* number of messages dropped since last successful message */
	u8  drop_cnt;
	u64 ts;          /* Time stamp */
} __packed;

/**
 * This structure is stored in ROM and is copied blindly by the phone.
 * The values for the fields of this structure are known at compile time.
 * So this is to be defined as a "static const" in the MACRO, so it ends up
 * being defined and initialized at compile time for each and every message
 * in the software. This minimizes the amount of work to do during run time.
 * So this structure is to be used in the "caller's" context. "Caller" is the
 * client of the Message Services.
 */
struct msg_desc_type {
	u16 line;       /* Line number in source file */
	u16 ss_id;      /* Subsystem ID */
	u32 ss_mask;    /* Subsystem Mask */
} __packed;

/**
 * This structure defines the Debug message packet :command-code 121,
 * DIAG_MSG_EXT_F.
 * Provides debug messages with NULL terminated filename and format fields, a
 * variable number of 32-bit parameters, an originator-specific timestamp
 * format and filterability via subsystem ids and masks.
 *
 * For simplicity, only 'long' arguments are supported. This packet
 * allows for N arguments, though the macros support a finite number.
 *
 * This is the structure that is used to represent the final structure that is
 * sent to the external device. 'msg_ext_store_type' is expanded to this
 * structure in DIAG task context at the time it is sent to the communication
 * layer.
 */
struct msg_ext_type {
	struct msg_hdr_type  hdr;        /* Header */
	struct msg_desc_type desc;       /* line number, SSID, mask */
	/* Array of long args, specified by 'hdr.num_args'
	 * Followed by NULL terminated format and file strings
	 */
	u32 args[1];
} __packed;

#pragma pack()

#endif  /* DIAG_MSG_PKT_DEFS_H */
