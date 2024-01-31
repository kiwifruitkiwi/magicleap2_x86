/* SPDX-License-Identifier: GPL-2.0-only
 *
 * Copyright (c) 2021-2022, Magic Leap, Inc. All rights reserved.
 */

#include <linux/device.h>

int xctrl_create_fs_device(struct class* xpc_class);
int xctrl_destory_fs_device(struct class* xpc_class);

#ifndef __KERNEL__
#error "Can't include kernel header in userspace"
#endif
