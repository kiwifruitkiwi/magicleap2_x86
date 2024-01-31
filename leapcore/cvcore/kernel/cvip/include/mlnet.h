/* SPDX-License-Identifier: GPL-2.0-only
 *
 * Copyright (c) 2019-2022, Magic Leap, Inc. All rights reserved.
 */

#ifndef __MLNET_H__
#define __MLNET_H__

// Larget data payload size
#define MLNET_MAX_MTU (8 * PAGE_SIZE)

// Number of slots in a mailbox. This must be a power of 2 for circular buffer api to work
#define MLNET_MBOX_SLOTS (128)

// The weight is the maximum number of incoming packets we want to process
// during a poll instance. It is used as the budget value in mlnet_poll.
#define MLNET_NAPI_WEIGHT (MLNET_MBOX_SLOTS)

#define MLNET_META_SZ           (PAGE_SIZE)

#define QUEUE_SZ                (MLNET_MBOX_SLOTS * MLNET_MAX_MTU)

/* MLNET data is approx 4MB */
#define MLNET_DATA_SZ           (MLNET_META_SZ + QUEUE_SZ)

#ifndef __KERNEL__
#error "Can't include kernel header in userspace"
#endif

#endif
