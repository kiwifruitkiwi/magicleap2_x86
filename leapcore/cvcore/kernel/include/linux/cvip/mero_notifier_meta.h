/* SPDX-License-Identifier: GPL-2.0-only
 *
 * Copyright (c) 2020-2022, Magic Leap, Inc. All rights reserved.
 */

#pragma once

#define MERO_NOTIFIER_FLAG_BIT(bit)	(1UL << (bit))

// Max number of cvcore names a task can be interested in polling for
// OR can signal at once.
#define MAX_CVCORE_NAMES_PER_TASK         	(128)
#define MAX_CVCORE_NAMES_PER_META_INSTANCE	(32)

// PL320 supports 28 bytes data per channel interrupt.
// Out of 28, 1 byte is saved for storing total payload size
// and other meta by the IPC layer, and another byte for
// header identifying to which system (such as soft semaphore,
// mero notifier) the interrupt and data belongs to, and
// potentially other meta if required in future.
// We then use another 8 bytes to store the cvcore name.
// And so, at max we have (28 - 1 - 1 - 8) = 18 bytes
// for actual data a user can send per cvcore name.
#define MAX_PAYLOAD_SIZE_BYTES		(18)

// Use this flag if a cvcore name is shared
// between ARM and DSPs.
#define DSP_SHARED			MERO_NOTIFIER_FLAG_BIT(0)

// Use this flag if a cvcore name is shared
// with x86 from the ARM.
#define X86_SHARED			MERO_NOTIFIER_FLAG_BIT(1)

// Use this flag if a cvcore name is shared
// with ARM from the x86.
#define ARM_SHARED			MERO_NOTIFIER_FLAG_BIT(2)

// Use this flag with notifier APIs (wait as well as
// signal) if you want to wake up only one task out
// of many that are waiting on the same cvcore name.
// The wake up order tries to follow FIFO ordering.
// NOTE: Make sure to use this flag with wait as well
// as signal notifier API. This flag is supported
// and has been tested on cvip only.
#define NOTIFY_ONLY_ONE_MODE		MERO_NOTIFIER_FLAG_BIT(3)

// Use this flag to suppress retry sending using mero_xpc
// if mero_xpc returns a failure, such that a mero_xpc failure
// returns immediately.
#define NOTIFY_FAST_FAIL                MERO_NOTIFIER_FLAG_BIT(4)


struct mero_notifier_meta {
	uint64_t cvcore_names_register_list[MAX_CVCORE_NAMES_PER_META_INSTANCE];
	uint64_t cvcore_names_ready_list[MAX_CVCORE_NAMES_PER_META_INSTANCE];
	uint64_t payload_buf_addr[MAX_CVCORE_NAMES_PER_META_INSTANCE];
	uint8_t payload_len_bytes[MAX_CVCORE_NAMES_PER_META_INSTANCE];
	int32_t nr_of_ready_cvcore_names;
	int32_t nr_of_registered_cvcore_names;
	uint32_t flags;
};

#ifndef __KERNEL__
#error "Can't include kernel header in userspace"
#endif
