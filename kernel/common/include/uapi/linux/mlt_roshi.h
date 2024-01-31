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

#ifndef __ML_ROSHI_H__
#define __ML_ROSHI_H__

#define ROSHI_ID		0xc201

struct roshi_input_evt {
	/* 2 bytes [0:1] */
	__u8	totem_id	: 2;
	__u8	buttons		: 4;
	__u8	touch_state	: 2;
	__u8	gesture		: 2;
	__u16	unused		: 6;

	/* 1 byte [2] */
	__u8	trigger;

	/* 4 bytes [3:6] */
	__u32	touch_x		: 12;
	__u32	touch_y		: 12;
	__u32	touch_force	: 8;
} __packed;

#endif /* __ML_ROSHI_H__ */
