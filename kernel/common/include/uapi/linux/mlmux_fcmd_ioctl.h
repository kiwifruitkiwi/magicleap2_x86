/* SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note */
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

/*
 * This file defines interface to communicate with mlmux_fcmd
 * kernel driver from user space.
 */

#ifndef __MLMUX_FCMD_IOCTL__
#define __MLMUX_FCMD_IOCTL__

#include <linux/ioctl.h>

#define MLMUX_FCMD_BUFFER_SIZE 128

struct file_data_t {
	unsigned int message_size;
	char message[MLMUX_FCMD_BUFFER_SIZE];
};

#define MLMUX_FCMD_IOCTL_MAGIC 99
#define MLMUX_FCMD_IOCTL_MSG  _IOWR(MLMUX_FCMD_IOCTL_MAGIC, 0, \
		struct file_data_t)

#endif /* ___MLMUX_FCMD_IOCTL__ */
