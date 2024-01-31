/* SPDX-License-Identifier: GPL-2.0-only
 *
 * Copyright (c) 2023, Magic Leap, Inc. All rights reserved.
 */
#ifndef __DSP_CORE_COMMON_H
#define __DSP_CORE_COMMON_H

#include "dsp_core_control_packets.h"
#include "dsp_core_response_packets.h"
#include "dsp_core_status_packets.h"
#include "dsp_core_types.h"

#define DSP_CORE_DBG(fmt, ...)                                                 \
	pr_debug("%s: " fmt, __func__, ##__VA_ARGS__)
#define DSP_CORE_ERROR(fmt, ...)                                               \
	pr_err("%s: " fmt, __func__, ##__VA_ARGS__)
#define DSP_CORE_INFO(fmt, ...)                                                \
	pr_info("%s: " fmt, __func__, ##__VA_ARGS__)

extern struct dsp_core_t *stream_manager_master;

int task_control_create_fs_device(void);
int task_control_destroy_fs_device(void);
int task_manager_create_fs_device(void);
int task_manager_destroy_fs_device(void);
int task_control_detach_from_parent(struct task_control_handle_t *handle);
int task_control_handle_task_status_message(
	struct task_control_task_status_message *message);
int submit_command_packet(uint8_t tgt_core_id,
			  struct task_manager_control_packet *cmd_packet,
			  struct task_manager_response_packet *rsp_packet);
int submit_command_packet_no_response(
	uint8_t tgt_core_id, struct task_manager_control_packet *cmd_packet);
int query_task_manager(uint8_t core_id,
		       struct dsp_core_task_manager_info *info);

int discover_built_in_symbols(uint8_t core_id, uint64_t *task_ptrs,
			      uint64_t *delegate_ptrs);

bool is_valid_core(uint8_t core_id);

#endif
