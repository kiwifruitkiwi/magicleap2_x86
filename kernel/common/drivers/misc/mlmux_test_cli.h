/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (c) 2019, Magic Leap, Inc. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef _ML_MUX_TEST_H
#define _ML_MUX_TEST_H

#include <linux/types.h>
#include <linux/workqueue.h>
#include <linux/ml-mux-client.h>

#define MLMUX_TEST_DATA_STR_LEN (64)

/* RX mlmux messages */
enum mlmux_test_rx_type {
	MLMUX_TEST_RX_TEST,
};

struct mlmux_test_rx_test {
	__le32 data;
	const char str[MLMUX_TEST_DATA_STR_LEN];
} __packed;

struct mlmux_test_rx_msg {
	uint8_t type;
	union {
		struct mlmux_test_rx_test test_msg;
	} u;
} __packed;

/* TX mlmux messages */
enum mlmux_test_tx_type {
	MLMUX_TEST_TX_TEST,
};

struct mlmux_test_tx_test {
	__le32 data;
	char str[MLMUX_TEST_DATA_STR_LEN];
} __packed;

struct mlmux_test_tx_msg {
	uint8_t type;
	union {
		struct mlmux_test_tx_test test_msg;
	} u;
} __packed;


#endif /* _ML_MUX_TEST_H */

