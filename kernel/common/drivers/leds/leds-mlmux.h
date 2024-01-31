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

#ifndef _LEDS_MLMUX_H
#define _LEDS_MLMUX_H

enum mlmux_led_colors {
	MLMUX_LED_RED,
	MLMUX_LED_GREEN,
	MLMUX_LED_BLUE,
	MLMUX_LED_COLOR_COUNT
};

/* mlmux rx message types */
enum mlmux_led_rx_type {
	MLMUX_LED_RX_REG_CMD,
	MLMUX_LED_RX_ERROR,
	MLMUX_LED_RX_BRIGHTNESS_WR_ACK,
	MLMUX_LED_RX_BRIGHTNESS_RD_ACK,
	MLMUX_LED_RX_PATTERN_WR_ACK,
	MLMUX_LED_RX_SUSPEND_ACK,
	MLMUX_LED_RX_RESUME_ACK,
	MLMUX_LED_RX_COUNT
};

/* mlmux tx message types */
enum mlmux_led_tx_type {
	MLMUX_LED_TX_BRIGHTNESS_WR,
	MLMUX_LED_TX_BRIGHTNESS_RD,
	MLMUX_LED_TX_PATTERN_WR,
	MLMUX_LED_TX_SUSPEND,
	MLMUX_LED_TX_RESUME,
	MLMUX_LED_TX_COUNT
};

/**
 * struct led_data - led info sent/received over mlmux
 * @led_grp: 1 based. 0 is reserved for future use if need to specify all leds
 * @rgb_idx: 0 based for red, green, blue
 * @brightness: pwm value for led
 */
struct led_data {
	uint8_t led_grp;
	uint8_t rgb_idx;
	uint8_t brightness;
} __packed;

/**
 * struct led_engine_data - led info sent/received over mlmux
 * @pattern_type: 0 based.
 */
struct led_engine_data {
	uint8_t pattern_type;
} __packed;

struct mlmux_led_rx_msg {
	uint8_t type;
	union {
		uint8_t led_grp_count;
		struct led_data data;
		struct led_engine_data e_data;
	} __packed content;
} __packed;

#define MLMUX_LED_RX_SIZE(msg) \
	(sizeof(struct mlmux_led_rx_msg) - \
	sizeof(((struct mlmux_led_rx_msg *)0)->content) + \
	sizeof(((struct mlmux_led_rx_msg *)0)->content.msg))

struct mlmux_led_tx_msg {
	uint8_t type;
	union {
		struct led_data data;
		struct led_engine_data e_data;
	} __packed u;
} __packed;

#endif /* _LEDS_MLMUX_H */

