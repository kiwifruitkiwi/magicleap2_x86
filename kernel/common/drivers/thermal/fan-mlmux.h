/* SPDX-License-Identifier: GPL-2.0
 * Copyright (c) 2020, Magic Leap, Inc. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#ifndef __MLMUX_FAN_H__
#define __MLMUX_FAN_H__

#define FAN_MLMUX_INSTANCE_ID_GENERIC		(0xFF)

/**
 * enum fan_mlmux_tx_type - TX message type
 * @FAN_MLMUX_TX_SET_RPM: Set RPM
 * @FAN_MLMUX_TX_SET_DIR: Set Direction
 * @FAN_MLMUX_TX_GET_STATUS: Request the current fan status
 * @FAN_MLMUX_TX_SUSPEND: Notify EC for x86 suspension
 * @FAN_MLMUX_TX_RESUME: Notify EC for x86 resume
 */
enum fan_mlmux_tx_type {
	FAN_MLMUX_TX_REGISTER_ACK,
	FAN_MLMUX_TX_REGISTER_FAILED,
	FAN_MLMUX_TX_SET_RPM,
	FAN_MLMUX_TX_SET_DIR,
	FAN_MLMUX_TX_GET_STATUS,
	FAN_MLMUX_TX_SUSPEND,
	FAN_MLMUX_TX_RESUME,
};

enum fan_mlmux_dir {
	FAN_MLMUX_DIR_FORWARD,
	FAN_MLMUX_DIR_REVERSE,
};

struct fan_mlmux_tx_msg_hdr {
	u8 instance_id;
	u8 msg_type;
} __attribute__((__packed__));

struct fan_mlmux_tx_msg {
	struct fan_mlmux_tx_msg_hdr hdr;
	union {
		__le16 rpm;
		u8 dir;
		struct {
			__le16 rpm;
			u8 dir;
		} config;
	} __attribute__((__packed__));
} __attribute__((__packed__));

enum fan_mlmux_rx_type {
	FAN_MLMUX_RX_REGISTER,
	FAN_MLMUX_RX_STATUS,
	FAN_MLMUX_RX_FAULT,
	FAN_MLMUX_RX_RPM_ACK,
	FAN_MLMUX_RX_DIR_ACK,
	FAN_MLMUX_RX_SUSPEND_ACK,
	FAN_MLMUX_RX_RESUME_ACK,
};

struct fan_mlmux_rx_msg_hdr {
	u8 instance_id;
	u8 msg_type;
} __attribute__((__packed__));

struct fan_mlmux_rx_status {
	u8 dir;
	__le16 rpm;
} __attribute__((__packed__));

enum fan_mlmux_fault {
	FAN_MLMUX_FAULT_START,
	FAN_MLMUX_FAULT_RAMP_UP,
	FAN_MLMUX_FAULT_MONITOR,
	FAN_MLMUX_FAULT_HARDWARE,
};

struct fan_mlmux_rx_msg {
	struct fan_mlmux_rx_msg_hdr hdr;
	union {
		struct fan_mlmux_rx_status status;
		u8 fault;
		u8 ack_status;
	} __attribute__((__packed__));
} __attribute__((__packed__));

#endif /* __MLMUX_FAN_H__ */
