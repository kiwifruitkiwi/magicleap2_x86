/* SPDX-License-Identifier: GPL-2.0-only
 *
 * Copyright (c) 2020-2023, Magic Leap, Inc. All rights reserved.
 */

#ifndef __CVCORE_TRACE_UAPI_H
#define __CVCORE_TRACE_UAPI_H

#include <linux/types.h>

#define DEVICE_NAME "cvcore_trace"
#define CVCORE_TRACE_MAGIC 't'
#define CVCORE_TRACE_FILE "/dev/cvcore_trace"

struct cvcore_trace_add_event_info {
	uint64_t trace_id;
	uint8_t data[32];
	int length;
};

struct cvcore_trace_create_info {
	uint64_t trace_id;
	uint32_t event_size;
	uint32_t max_events;
};

#define IOCTL_CVCORE_TRACE_INITIALIZE _IO(CVCORE_TRACE_MAGIC, 0xA0)
#define IOCTL_CVCORE_TRACE_ADD_EVENT                                           \
	_IOW(CVCORE_TRACE_MAGIC, 0xA1, struct cvcore_trace_add_event_info)
#define IOCTL_CVCORE_TRACE_CREATE                                              \
	_IOW(CVCORE_TRACE_MAGIC, 0xA2, struct cvcore_trace_create_info)
#define IOCTL_CVCORE_TRACE_TEST _IO(CVCORE_TRACE_MAGIC, 0xA3)

#ifndef __KERNEL__
#error "Can't include kernel header in userspace"
#endif

#endif
