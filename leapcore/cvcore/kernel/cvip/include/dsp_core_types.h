/* SPDX-License-Identifier: GPL-2.0-only
 *
 * Copyright (c) 2023, Magic Leap, Inc. All rights reserved.
 */
#ifndef __DSP_CORE_TYPES_H
#define __DSP_CORE_TYPES_H

#include <linux/mutex.h>
#include <linux/types.h>
#include <linux/cdev.h>

#include "cvcore_processor_common.h"
#include "dsp_core_uapi.h"

struct dsp_core_t {
	struct mutex mtx;
	struct class *char_class;
	struct device *char_device;
	struct list_head managers;
};

struct stream_manager_t {
	struct mutex mtx;
	pid_t pid;
	uint32_t stream_id;
	uint16_t active_cores;
	bool is_bound;
	struct list_head node;
	struct list_head tcontrol_list;
};

struct task_control_t {
	dev_t dev;
	struct cdev cdev;
	struct device *char_device;
};

struct task_manager_t {
	dev_t dev;
	struct cdev cdev;
	struct device *char_device;
};

struct task_locator {
	uint8_t core_id;
	uint8_t unique_id;
};

struct task_control_handle_t {
	struct mutex mtx;
	spinlock_t s_lck;
	wait_queue_head_t wait_queue;
	struct stream_manager_t *owner;
	struct task_locator locator;
	uint64_t run_id;
	bool is_attached;
	bool is_started;
	bool is_exited;
	bool is_aborted;
	enum dsp_core_task_run_state run_state;
	uint32_t exit_code;
	struct list_head node;
};

struct dsp_core_task_state {
	uint32_t exit_code;
	uint32_t task_id;
	uint16_t stream_id;
	uint8_t run_state;
};

static inline void INIT_TASK_LOCATOR(struct task_locator *locator)
{
	locator->core_id = 0xFF;
	locator->unique_id = 0xFF;
}

static inline int task_locator_valid(const struct task_locator *locator)
{
	if (locator->core_id < CVCORE_DSP_ID_MIN ||
	    locator->core_id > CVCORE_DSP_ID_MAX)
		return 0;
	return 1;
}

#endif
