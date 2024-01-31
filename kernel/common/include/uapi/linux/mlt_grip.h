/* SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note */
/*
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
 *
 */

#ifndef __ML_GRIP_H__
#define __ML_GRIP_H__

#define GRIP_ID			0xc001

struct grip_imu_evt {
	/* 4 bytes [0:3] */
	__u32	usused		: 2;
	__u32	timesync_id	: 6;
	__u32	tstamp		: 24;

	/* 18 bytes [4:21] */
	__s16	accel[3];
	__s16	gyro[3];
	__s16	mgnt[3];
} __packed;

struct grip_ctrl_evt {
	/* 2 bytes [0:1] */
	__u8	unused		: 2;
	__u8	buttons		: 2;
	__u8	touch_state	: 2;
	__u8	gesture		: 2;
	__u8	trigger;

	/* 4 bytes [2:5] */
	__u32	touch_x		: 12;
	__u32	touch_y		: 12;
	__u32	touch_force	: 8;
} __packed;

enum GRIP_INPUT_EVENT_TYPE {
	GRIP_INPUT_IMU = 0,
	GRIP_INPUT_CTRL = 1,
};

struct grip_input_evt {
	__u8				type;
	union {
		struct grip_imu_evt	imu;
		struct grip_ctrl_evt	ctrl;
	} u;
} __packed;

#endif /* __ML_GRIP_H__ */
