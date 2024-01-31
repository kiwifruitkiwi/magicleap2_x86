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
#ifndef __ML2_EC_CTRL_MLMUX_H__
#define __ML2_EC_CTRL_MLMUX_H__

#define ML2_EC_VER_SIZE 16

struct ml2_ec_data {
	struct device *dev;
	struct ml_mux_client client;
	struct dentry *dentry;
	struct workqueue_struct *work_q;
	char ec_ver[ML2_EC_VER_SIZE];
	struct completion comp;
	uint8_t dev_sku;
	uint8_t display_state;
	uint8_t init_boot;
	bool is_shutdown;
};

enum ml2_ec_mlmux_tx_type {
	ML2_EC_MLMUX_TX_PING_ACK,
	ML2_EC_MLMUX_TX_QUIET,
	ML2_EC_MLMUX_TX_RESUME,
	ML2_EC_MLMUX_TX_DISPLAY_UPDATE,
	ML2_EC_MLMUX_TX_SLEEP_TIMEOUT,
	ML2_EC_MLMUX_TX_IS_SHUTDOWN,
	ML2_EC_MLMUX_TX_SHUTDOWN,
};

enum ml2_ec_mlmux_rx_type {
	ML2_EC_MLMUX_RX_PING,
	ML2_EC_MLMUX_RX_EC_VER,
	ML2_EC_MLMUX_RX_QUIET_ACK,
	ML2_EC_MLMUX_RX_RESUME_ACK,
	ML2_EC_MLMUX_RX_DEV_SKU,
	ML2_EC_MLMUX_RX_EC_BOOT,
	ML2_EC_MLMUX_RX_SHUTDOWN,
	ML2_EC_MLMUX_RX_IS_SHUTDOWN_ACK,
};

enum ml2_ec_mlmux_dev_sku {
	ML2_EC_SKU_ENT = 0x01,
	ML2_EC_SKU_MED = 0x02,
};

enum ml2_ec_mlmux_display_state {
	ML2_EC_DISPLAY_OFF,
	ML2_EC_DISPLAY_ON,
	ML2_EC_DISPLAY_UNKNOWN,
};

struct ml2_ec_mlmux_work {
	struct work_struct work;
	struct ml2_ec_data *dev_data;
	void *msg_data;
};

struct ml2_ec_mlmux_tx_msg {
	uint8_t type;
	union {
		uint8_t display_state;
		int32_t sleep_timeout;
	} __attribute__((__packed__));
} __attribute__((__packed__));

struct ml2_ec_mlmux_rx_msg {
	uint8_t type;
	union {
		uint8_t data;
		char ec_ver[ML2_EC_VER_SIZE];
	} __attribute__((__packed__));
} __attribute__((__packed__));

#endif /* __ML2_EC_CTRL_MLMUX_H__ */
