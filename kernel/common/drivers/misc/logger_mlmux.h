/* SPDX-License-Identifier: GPL-2.0
 * Copyright (c) 2021, Magic Leap, Inc. All rights reserved.
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

#ifndef LOGGER_MLMUX_H
#define LOGGER_MLMUX_H

enum ec_ack_codes {
	LOGGER_RX_FLUSH_ACK = 0,
	LOGGER_RX_LGR_DUMP_ACK,
	LOGGER_RX_INVALID_ACK,
};

enum ec_logger_cmds {
	LOGGER_TX_FLUSH,
	LOGGER_TX_BUF_DUMP,
	/* add logger cmds above this only */
	MAX_CMDS,
};

struct logger_tx_msg {
	uint8_t type;
} __packed;

struct logger_rx_msg {
	uint8_t type;
} __packed;

#endif /* LOGGER_MLMUX_H */
