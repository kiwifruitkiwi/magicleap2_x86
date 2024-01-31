/* SPDX-License-Identifier: GPL-2.0-only
 *
 * Copyright (c) 2020-2022, Magic Leap, Inc. All rights reserved.
 */

#ifndef __XSTAT_INTERNAL_H
#define __XSTAT_INTERNAL_H

#include <linux/device.h>

#include "mero_xpc_common.h"
#include "xstat.h"

#define XSTAT_KTHREAD_NAME "xstat"
#define XSTAT_DEFAULT_TIMEOUT_MS (100)
#define XSTAT_KTHREAD_SLEEP_SEC (15)
#define PING_MAGIC_KEY (0xBEEFF00D)
#define PONG_MAGIC_KEY (0x8888BEEF)

 // xstat sysfs entry enum
enum xstat_sysfs_entry {
	XSTAT_CVIP_SYSFS = 0,
	XSTAT_X86_SYSFS,
	XSTAT_WEARABLE_SYSFS,
	XSTAT_SYSFS_ENTRY_COUNT
};

enum xstat_kthread_state { THREAD_IDLE, THREAD_RUN_REQUEST, THREAD_RUNNING };

#define XSTAT_STR_DEF(e_def, e_str) e_str,
static const char* const xstat_str_list[] = { XSTAT_STATE_LIST(XSTAT_STR_DEF) };
static const char* const xstat_wstate_str_list[] = { XSTAT_WEARABLE_STATE_LIST(XSTAT_STR_DEF) };

union xstat_state_changed {
	uint32_t state_changed;
	struct {
		uint32_t cvip_state : 1;
		uint32_t x86_state : 1;
		uint32_t wearable_state : 1;
	};
};
struct xstat_struct {
	struct xstat_core_info cvip_core_info;
	struct xstat_core_info x86_core_info;
	union xstat_state_changed state_changed_info;
	struct task_struct* task;
	wait_queue_head_t wq;
	spinlock_t lock;

	uint32_t wait_count;
	enum xstat_kthread_state state;

	struct {
		int target_core;
		struct xstat_core_info core_info;
		int ret_code;
	} req;
};

/* xstat message */
struct xstat_request {
	uint32_t key;
};

struct xstat_response {
	uint32_t key;
	struct xstat_core_info core_info;
};

/* xstat notification */
struct xstat_state_change_cmd {
	uint32_t key;
	union xstat_state_changed state_changed_info;
	struct xstat_core_info core_info;
};

struct xstat_state_change_rsp {
	uint32_t key;
};

extern struct xstat_struct xstat_struct;

int xstat_init(struct class* xpc_class);

int xstat_request_core_info(uint8_t target_core, struct xstat_core_info* info,
	uint32_t timeout_ms);

int xstat_message_handler(union xpc_platform_channel_message* cmd_msg,
	union xpc_platform_channel_message* rsp_msg);

int xstat_state_change_handler(union xpc_platform_channel_message* cmd_msg,
	union xpc_platform_channel_message* rsp_msg);

int xstat_kthread(void* arg);

ssize_t cvip_state_show(struct class* class, struct class_attribute* attr,
	char* buf);

ssize_t __attribute__((unused))
cvip_state_store(struct class* class, struct class_attribute* attr,
	const char* buf, size_t count);

ssize_t x86_state_show(struct class* class, struct class_attribute* attr,
	char* buf);

ssize_t __attribute__((unused))
x86_state_store(struct class* class, struct class_attribute* attr,
	const char* buf, size_t count);

ssize_t wearable_state_show(struct class* class, struct class_attribute* attr,
	char* buf);

ssize_t __attribute__((unused))
wearable_state_store(struct class* class, struct class_attribute* attr,
	const char* buf, size_t count);

#ifndef __KERNEL__
#error "Can't include kernel header in userspace"
#endif

#endif
