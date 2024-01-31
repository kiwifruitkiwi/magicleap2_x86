/* SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note */
/*
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

#ifndef __CVIP__
#define __CVIP__

#include <linux/ioctl.h>

struct cvip_status {
	__u8 is_alive;
	__u32 wdt_count;
	__u32 reboot_count;
	__u32 panic_count;
	__u32 boot_reason;
	__u32 wd_reboot_count;
};

void cvip_status_get_status(struct cvip_status *stats);

#define CVIP_STATUS_IOCTL_MAGIC      ('!')
#define CVIP_STATUS_IOCTL_GET_STATUS (_IOWR('!', 0, struct cvip_status))

#endif // __CVIP__
