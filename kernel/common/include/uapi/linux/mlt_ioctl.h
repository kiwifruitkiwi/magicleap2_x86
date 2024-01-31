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

#ifndef __MLT_IOCTL_H__
#define __MLT_IOCTL_H__

#include <linux/ioctl.h>

#define MLT_BUS_ANY		0xffff

struct mlt_req_create {
	__u32 vendor;
	__u32 product;
	__u32 version;
	__u16 bus;
	__u8 name[32];
} __attribute__((__packed__));

#define MLT_MAGIC_TYPE 'C'
#define MLT_IOCTL_CREATE	_IOW(MLT_MAGIC_TYPE, 0, struct mlt_req_create)
#define MLT_IOCTL_DESTROY	_IO(MLT_MAGIC_TYPE, 1)
#define MLT_IOCTL_INPUT		_IO(MLT_MAGIC_TYPE, 2)

#endif /* __MLT_IOCTL_H__ */
