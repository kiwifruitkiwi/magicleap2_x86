/* SPDX-License-Identifier: GPL-2.0-only
 *
 * Copyright (c) 2023, Magic Leap, Inc. All rights reserved.
 */

#ifndef __SHREGIONCTL_UAPI_H
#define __SHREGIONCTL_UAPI_H

#include <linux/types.h>

#define SHREGIONCTL_DEVICE_NAME "shregionctl"
#define SHREGIONCTL_MAGIC 'x'
#define SHREGIONCTL_FILE "/dev/shregionctl"

//operations
#define SHREGION_PID_GRANT    1
#define SHREGION_PID_REVOKE   2

struct shregion_permissions {
	uint64_t shr_name;
	uint32_t pid;
	uint32_t operation;
};

#define SHREGION_IOC_SET_PID_PERM _IOW(SHREGIONCTL_MAGIC, 0xA0, struct shregion_permissions)

#ifndef __KERNEL__
#error "Can't include kernel header in userspace"
#endif

#endif

