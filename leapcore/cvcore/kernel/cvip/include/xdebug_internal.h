/* SPDX-License-Identifier: GPL-2.0-only
 *
 * Copyright (c) 2020-2023, Magic Leap, Inc. All rights reserved.
 */

#ifndef __XDEBUG_INTERNAL_H
#define __XDEBUG_INTERNAL_H

#include <linux/delay.h>
#include <linux/device.h>

#include "pl320_hal.h"
#include "pl320_types.h"

#define XDEBUG_KTHREAD_NAME "xdebug"

#define XDEBUG_DEFAULT_CONTENTION_THRESHOLD (24)
#define XDEBUG_SAMPLE_PERIOD_MINIMUM_USEC (250)

#define XDEBUG_DEFAULT_RATELIMIT_THRESHOLD (4)
#define XDEBUG_RATELIMIT_THRESHOLD_MINIMUM (1)
#define XDEBUG_DEFAULT_RATELIMIT_TIMEOUT_US (5000000) // 5 seconds
#define XDEBUG_RATELIMIT_TIMEOUT_MINIMUM_US (1000000) // 1 second

enum xdebug_kthread_state {
	XD_THREAD_IDLE,
	XD_THREAD_RUN_REQUEST,
	XD_THREAD_STOP_REQUEST,
	XD_THREAD_RUNNING
};

struct xdebug_struct {
	struct task_struct* task;
	wait_queue_head_t wq;
	spinlock_t lock;
	atomic_t contention_thresh;
	atomic_t sample_period_us;
	atomic_t ipc_dev_mask;
	atomic_t ratelimit_event_count;
	atomic_t ratelimit_timeout_us;
	enum xdebug_kthread_state state;

	pl320_debug_info debug_info;
};

extern struct xdebug_struct xdebug_struct;

void xdebug_init(void);
void xdebug_stop(void);
int xdebug_parse_command(const char* buf);
void xdebug_event_dump(uint8_t ipc_idx, uint8_t mb_idx);

void __xdebug_event_record(uint8_t ipc_idx, uint8_t mb_idx, uint8_t event_id,
	uint32_t value);
void __xdebug_event_record_timed(uint8_t ipc_idx, uint8_t mb_idx,
	uint8_t event_id, uint32_t value, uint64_t ts);
void __xdebug_event_record_all(uint8_t ipc_idx, uint8_t event_id,
	uint32_t value);

#ifdef XDEBUG_HAS_EVENT_TRACE
static inline uint64_t ccnt_read(void)
{
	uint64_t cc = 0;
#if defined(__amd64__) || defined(__i386__)
	cc = ktime_get_ns();
#else
	asm volatile("mrs %0, cntvct_el0\n\t"
		"mrs x5, cntvct_el0\n\t"
		"cmp %0, x5\n\t"
		"b.hi 8\n\t"
		"mov x5, %0\n\t"
		"nop"
		: "=r" (cc)
		:
		: "x5", "cc");
#endif
	return cc;
}

#define xdebug_event_get_ts() ccnt_read()

#define xdebug_event_record(ipc_idx, mb_idx, event_id, value)                  \
	__xdebug_event_record((ipc_idx), (mb_idx), (event_id), (value))

#define xdebug_event_record_timed(ipc_idx, mb_idx, event_id, value, ts)        \
	__xdebug_event_record_timed((ipc_idx), (mb_idx), (event_id), (value),  \
				    (ts))

#define xdebug_event_record_all(ipc_idx, event_id, value)                      \
	__xdebug_event_record_all((ipc_idx), (event_id), (value))

#else // no xdebug event tracing
#define xdebug_event_get_ts() 0UL
#define xdebug_event_record(ipc_idx, mb_idx, event_id, value) { 				\
	(void) ipc_idx; (void) mb_idx; (void) event_id; (void) value;				\
}
#define xdebug_event_record_timed(ipc_idx, mb_idx, event_id, value, ts) { 		\
	(void) ipc_idx; (void) mb_idx; (void) event_id; (void) value; (void) ts;	\
}
#define xdebug_event_record_all(ipc_idx, event_id, value) { \
	(void) ipc_idx; (void) event_id; (void) value;			\
}

#endif

#define XPC_EVT_NONE (0)
#define XPC_EVT_SET_SOURCE (1)
#define XPC_EVT_SET_DEST (2)
#define XPC_EVT_SET_MASK (3)
#define XPC_EVT_CLR_MASK_START (4)
#define XPC_EVT_CLR_MASK_END (5)
#define XPC_EVT_SET_MODE (6)
#define XPC_EVT_SEND (7)
#define XPC_EVT_ACK (8)
#define XPC_EVT_RELEASE_START (9)
#define XPC_EVT_RELEASE_END (10)
#define XPC_EVT_CLR_DEST_START (11)
#define XPC_EVT_CLR_DEST_END (12)
#define XPC_EVT_RD_MASK_START (13)
#define XPC_EVT_RD_MASK_END (14)
#define XPC_EVT_RD_MODE_START (15)
#define XPC_EVT_RD_MODE_END (16)
#define XPC_EVT_MAILBOX_ACQ (17)
#define XPC_EVT_RELEASE_CID_MASK_START (18)
#define XPC_EVT_RELEASE_CID_MASK_END (19)
#define XPC_EVT_SET_CID_MASK_START (20)
#define XPC_EVT_SET_CID_MASK_END (21)
#define XPC_EVT_IRQ_HANDLER_EXIT_EARLY (22)
#define XPC_EVT_IRQ_COMMAND_ACK_SOURCE_COMPLETED_START (23)
#define XPC_EVT_IRQ_COMMAND_ACK_SOURCE_COMPLETED_END (24)
#define XPC_EVT_IRQ_NOTIFICATION_ACK_SOURCE_CLOSED (25)
#define XPC_EVT_IRQ_NOTIFICATION_ACK_SOURCE_COMPLETED_START (26)
#define XPC_EVT_IRQ_NOTIFICATION_ACK_SOURCE_COMPLETED_END (27)
#define XPC_EVT_IRQ_QUEUE_ACK_SOURCE_CLOSED (28)
#define XPC_EVT_IRQ_COMMAND_IN_FAST_HANDLE_START (29)
#define XPC_EVT_IRQ_COMMAND_IN_FAST_HANDLE_END (30)
#define XPC_EVT_IRQ_NOTIFICATION_IN_FAST_HANDLE_START (31)
#define XPC_EVT_IRQ_NOTIFICATION_IN_FAST_HANDLE_END (32)
#define XPC_EVT_IRQ_WORK_SCHEDULED (33)
#define XPC_EVT_IRQ_ENTER (34)
#define XPC_EVT_NOTIFICATION_WAIT_FOR_COMPLETION_START (35)
#define XPC_EVT_NOTIFICATION_WAIT_FOR_COMPLETION_END (36)
#define XPC_EVT_NOTIFICATION_WAIT_FOR_COMPLETION_RESTARTSYS (37)
#define XPC_EVT_TASKLET_START (38)
#define XPC_EVT_REPLY (39)
#define XPC_EVT_NOTIFICATION_MAILBOX_ACQ (40)
#define XPC_EVT_CMD_WAKEUP_QUEUE (41)
#define XPC_EVT_TASKLET_END (42)
#define XPC_EVT_CMD_WAITER_WOKE (43)

#ifndef __KERNEL__
#error "Can't include kernel header in userspace"
#endif

#endif
