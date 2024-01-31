/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (c) 2020, Magic Leap, Inc. All rights reserved.
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

#ifndef _ML_PWR_MON_H
#define _ML_PWR_MON_H

#include <linux/types.h>
#include <linux/workqueue.h>
#include <linux/mutex.h>
#include <linux/ml-mux-client.h>

#define PWR_MON_NAME_SIZE		(12)
#define PWR_MON_CHAN_NAME_SIZE		(20)

enum pwr_mon_channels {
	PWR_MON_CHANNEL1,
	PWR_MON_CHANNEL2,
	PWR_MON_CHANNEL3,
	PWR_MON_CHANNEL4,
	PWR_MON_NUM_CHANNELS
};

struct pwr_mon_input {
	char label[PWR_MON_CHAN_NAME_SIZE + 1];
	int32_t volt;
	int32_t curr;
	int32_t power;
	bool disconnected;
	bool valid;
};

struct pwr_mon_dev_data {
	struct device *dev;
	struct ml_mux_client client;
	const char *chan_name;
	struct workqueue_struct *work_q;
	struct completion suspend;
	struct completion resume;
	struct completion read;
	struct mutex lock;
	struct idr pm_idr;
	bool ch_open;
};

struct pwr_mon {
	struct device dev;
	char name[PWR_MON_NAME_SIZE + 1];
	int16_t id;
	struct pwr_mon_input inputs[PWR_MON_NUM_CHANNELS];
	struct device *hwmon_dev;
	struct pwr_mon_dev_data *pmdata;
	uint32_t num_avg;
	uint32_t pwr_overlimit;
	int32_t boot_current;
};

struct pwr_mon_work {
	struct work_struct work;
	struct pwr_mon_dev_data *pmdata;
	void *data;
};

enum pwr_mon_rx_type {
	PWR_MON_RX_REG,
	PWR_MON_RX_UPDATE,
	PWR_MON_RX_SUSPEND_ACK,
	PWR_MON_RX_RESUME_ACK,
	PWR_MON_RX_SET_AVG_ACK,
	PWR_MON_RX_GET_AVG_PWR_ACK,
	PWR_MON_RX_SET_PWR_LMT_ACK,
};

struct pwr_mon_rx_reg {
	char pm_name[PWR_MON_NAME_SIZE];
	char chan_name[PWR_MON_NUM_CHANNELS][PWR_MON_CHAN_NAME_SIZE];
	uint32_t num_avg;
	uint32_t pwr_overlimit;
	int32_t boot_current;
} __packed;

enum pwr_mon_rx_chan_type {
	PWR_MON_TYPE_VOLT,
	PWR_MON_TYPE_CURR,
	PWR_MON_TYPE_PWR,
};

struct pwr_mon_rx_update {
	int16_t id;
	bool valid;
	int32_t data;
	int16_t channel;
	int16_t chan_type;
} __packed;

struct pwr_mon_rx_avg_data {
	int16_t id;
	int8_t channel;
	int32_t data;
} __packed;

struct pwr_mon_rx_msg {
	uint8_t type;
	union {
		int32_t data;
		struct pwr_mon_rx_reg reg;
		struct pwr_mon_rx_update update;
		struct pwr_mon_rx_avg_data avg_data;
	} __packed u;
} __packed;
#define PWR_MON_RX_SIZE(msg) \
	(sizeof(struct pwr_mon_rx_msg) - \
	sizeof(((struct pwr_mon_rx_msg *)0)->u) + \
	sizeof(((struct pwr_mon_rx_msg *)0)->u.msg))

enum pwr_mon_tx_type {
	PWR_MON_TX_REG_ACK,
	PWR_MON_TX_UPDATE,
	PWR_MON_TX_SUSPEND,
	PWR_MON_TX_RESUME,
	PWR_MON_TX_SET_AVG,
	PWR_MON_TX_GET_AVG_PWR,
	PWR_MON_TX_SET_PWR_LMT,
};

struct pwr_mon_tx {
	char name[PWR_MON_NAME_SIZE];
	int16_t id;
	int32_t data;
	int16_t chan_type;
} __packed;

struct pwr_mon_tx_msg {
	uint8_t type;
	union {
		int32_t data;
		struct pwr_mon_tx msg;
	} __packed u;
} __packed;
#define PWR_MON_TX_MSG_SIZE(msg) \
	(sizeof(struct pwr_mon_tx_msg) - \
	sizeof(((struct pwr_mon_tx_msg *)0)->u) + \
	sizeof(((struct pwr_mon_tx_msg *)0)->u.msg))

#endif /* _ML_PWR_MON_H */
