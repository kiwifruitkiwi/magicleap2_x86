/* SPDX-License-Identifier: GPL-2.0-only
 *
 * Copyright (c) 2023, Magic Leap, Inc. All rights reserved.
 */
#ifndef __DSP_CORE_STATUS_PACKETS_H
#define __DSP_CORE_STATUS_PACKETS_H

#include <linux/types.h>
#include <stdalign.h>

#include "cvcore_status.h"
#include "dsp_core_types.h"
#include "dsp_core_uapi.h"

enum task_manager_status_type_id {
	kStatTaskState = 0x1,
	kStatTaskException,
	kStatLast,
	kStatInvalid
};

struct task_control_task_status_message {
	uint64_t task_run_id;
	uint32_t exit_code;
	uint32_t task_id;
	struct task_locator locator;
	uint16_t stream_id;
	uint8_t run_state;
};

struct task_manager_status_packet {
	__attribute__((aligned(8))) uint8_t bytes[26];
	union {
		struct task_control_task_status_message task_status_message;
	};

	uint8_t type_id;
};

enum task_manager_status_type_id
task_manager_status_get_type(struct task_manager_status_packet *packet);

struct task_control_task_status_message *
get_task_control_task_status_message(struct task_manager_status_packet *packet);

#endif
