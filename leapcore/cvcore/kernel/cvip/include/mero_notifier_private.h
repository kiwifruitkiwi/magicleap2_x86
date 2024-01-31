/* SPDX-License-Identifier: GPL-2.0-only
 *
 * Copyright (c) 2020-2022, Magic Leap, Inc. All rights reserved.
 */

#ifndef MERO_NOTIFIER_PRIV_H
#define MERO_NOTIFIER_PRIV_H

#include "mero_notifier_meta.h"
#if defined(ARM_CVIP)
#include "gsm_spinlock.h"
#endif
#include "mero_xpc_types.h"

#define NOTIFIER_XPC_DATA_LEN_BYTES             (XPC_MAILBOX_DATA_SIZE - 1)
#define CVCORE_NAME_SIZE_BYTES                  (sizeof(uint64_t))
#define MAX_NR_OF_DSP_SHARED_NAMES              (192)
// Max number of tasks that could be
// interested in this cvcore name.
#define MAX_TASKS_PER_CVCORE_NAME               (16)

// Mero Notifier expects a max payload size of 18 bytes
// that can be represented using lower 5 bits.
// Lower 5 bits are used for representing payload length.
#define NOTFIER_PAYLOAD_LEN_NR_OF_BITS          (5)
#define NOTIFIER_PAYLOAD_LEN_MASK               ((1UL << NOTFIER_PAYLOAD_LEN_NR_OF_BITS) - 1)
// Using the remaining upper 3 bits for identifying the notification type.
#define NOTIFIER_NOTIF_TYPE_MASK                ((~(0UL)) << NOTFIER_PAYLOAD_LEN_NR_OF_BITS)

#define NOTIFIER_NOTIF_TYPE_PAYLOAD             (0UL << NOTFIER_PAYLOAD_LEN_NR_OF_BITS)
#define NOTIFIER_NOTIF_TYPE_PID_DEATH           (1UL << NOTFIER_PAYLOAD_LEN_NR_OF_BITS)

struct mero_notifier_meta_drv {
	uint64_t timeout_us;
	uint64_t meta_ptr;
	bool wait_all;
};

// IOCTL cmds.
#define MERO_NOTIFIER_IOC_MAGIC                 ('n')

// Wait any.
#define MERO_NOTIFIER_IOC_WAIT_ANY              _IOWR(MERO_NOTIFIER_IOC_MAGIC, 1, struct mero_notifier_meta_drv)

// Timed wait any.
#define MERO_NOTIFIER_IOC_TIMED_WAIT_ANY        _IOWR(MERO_NOTIFIER_IOC_MAGIC, 2, struct mero_notifier_meta_drv)

// Signal list of ready cvcore names.
#define MERO_NOTIFIER_IOC_SIGNAL                _IOW(MERO_NOTIFIER_IOC_MAGIC, 3, struct mero_notifier_meta)

// Status codes returned by the mero notifier misc driver.
#define MERO_NOTIFIER_STATUS_BASE               (0x4200)

// Number of PIDs interested in a particular cvcore name
// has exceeded max limit.
#define MERO_NOTIFIER_TASK_NR_QUOTA_EXCEEDED    (0x4201)

// Number of cvcore names that a PID is interested
// in has execeeded max limit.
#define MERO_NOTIFIER_NAME_NR_QUOTA_EXCEEDED    (0x4202)

#if defined(ARM_CVIP)

#define NR_OF_CVIP_CORES	(9)
struct __attribute__ ((__packed__)) dsp_notifier_metadata {
	struct __attribute__ ((__packed__)) {
		uint64_t cvcore_name;
		uint64_t interested_cores;
		uint64_t nr_of_tasks[NR_OF_CVIP_CORES];
	} cvcore_name_meta[MAX_NR_OF_DSP_SHARED_NAMES];

	struct gsm_spinlock_context gsm_spinlock_ctx;

	uint32_t nr_of_valid_cvcore_names;
};

#endif

struct __attribute__ ((__packed__)) dsp_notifier_xpc_data {
	// notif_type (upper 3 bits) + payload size (lower 5 bits)
	// payload size is only valid if notif type is
	// NOTIFIER_NOTIF_TYPE_PAYLOAD
	uint8_t notif_meta;
	uint64_t cvcore_name;

	union {
		uint16_t sid; // stream id
		uint8_t payload[MAX_PAYLOAD_SIZE_BYTES];
	};
};

_Static_assert(sizeof(struct dsp_notifier_xpc_data) == NOTIFIER_XPC_DATA_LEN_BYTES,
	"ERR: sizeof(struct NOTIFIER_XPC_DATA_LEN_BYTES) != NOTIFIER_XPC_DATA_LEN_BYTES");

#ifndef __KERNEL__
#error "Can't include kernel header in userspace"
#endif

#endif /* MERO_NOTIFIER_PRIV_H */
