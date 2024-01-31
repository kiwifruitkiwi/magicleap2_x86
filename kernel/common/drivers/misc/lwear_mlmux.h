/* SPDX-License-Identifier: GPL-2.0
 * Copyright (c) 2022, Magic Leap, Inc. All rights reserved.
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

/* This file defines structures and enums used in IPC between X86 and EC.
 * Make sure they are in sync with their counterparts on EC side
 */
#ifndef LWEAR_MLMUX_H
#define LWEAR_MLMUX_H

enum lwear_tx_id {
	LWEAR_STATE_REPORT_REQ,
};

struct lwear_tx_msg {
	uint8_t id; /* value from enum lwear_tx_id */
} __packed;


enum lwear_state_report {
	LWEAR_STATE_REPORT_ON,
	LWEAR_STATE_REPORT_OFF,
	LWEAR_STATE_REPORT_NC,
	LWEAR_STATE_REPORT_FAULT,
};

enum lwear_rx_id {
	LWEAR_STATE_REPORT_ACK,
	LWEAR_INVALID_REQ_ACK,
};

struct lwear_rx_msg {
	uint8_t id;      /* value from enum lwear_rx_id */
	uint8_t state;   /* value from enum lwear_state_report */
} __packed;

#endif /* LWEAR_MLMUX_H */
