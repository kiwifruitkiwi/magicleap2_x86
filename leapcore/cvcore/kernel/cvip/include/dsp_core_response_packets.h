/* SPDX-License-Identifier: GPL-2.0-only
 *
 * Copyright (c) 2023, Magic Leap, Inc. All rights reserved.
 */
#ifndef __DSP_CORE_RESPONSE_PACKETS_H
#define __DSP_CORE_RESPONSE_PACKETS_H

#include <linux/types.h>
#include <stdalign.h>

#include "cvcore_status.h"
#include "dsp_core_types.h"
#include "dsp_core_uapi.h"

enum task_manager_response_id {
	kRspTaskCreate = 0x80,
	kRspTaskStart,
	kRspTaskAbort,
	kRspTaskGetStatus,
	kRspTaskGetPriority,
	kRspTaskSetPriority,
	kRspTaskGetArgumentInfo,
	kRspTaskFind,
	kRspDelegateStart,
	kRspManagerGetBuiltInTask,
	kRspManagerGetInfo,
	kRspManagerGetCommands,
	kRspManagerAbortStreamID,
	kRspWorkQueueEnqueue,
	kRspShutdown,
	kRspLast,
	kRspInvalid
};

/**
* \brief Structure that encapsulates a task_manager info query response
*/
struct task_manager_get_info_response {
	struct dsp_core_task_manager_info info;
};

/**
* \brief Structure that encapsulates a task_manager supported commands query response
*/
struct task_manager_get_command_info_response {
	uint16_t command_channels;
	uint8_t command_id;
};

/**
* \brief Structure that encapsulates a task manager built-in task query response
*/
struct task_manager_get_built_in_symbol_response {
	uint64_t ptr;
};

/**
* Structure that encapsulates stream abort parameters
*/
struct task_manager_abort_stream_id_response {
	uint8_t num_aborted;
};

struct task_manager_task_create_response {
	/** A unique identifier for a task. */
	struct dsp_core_task_locator task_locator;

	/** If non-zero, a memory region to pass task arguments. */
	uint64_t task_argument_area_base;

	/** If non-zero, the size of the memory region for passing task arguments. */
	uint16_t task_argument_area_size;
};

/**
* \brief Structure that encapsulates Task find command response
*/
struct task_control_task_find_response {
	/** If non-zero, a memory region to pass task arguments. */
	uint64_t task_argument_area_base;

	/** If non-zero, the size of the memory region for passing task arguments. */
	uint16_t task_argument_area_size;

	/** A unique system-wide identifier for the located task. */
	struct dsp_core_task_locator task_locator;
};

/**
* \brief Structure that encapsulates Task abort command response
*/
struct task_control_task_abort_response {
	/** The state the task was in when aborted. */
	struct dsp_core_task_state task_state_on_abort;
};

/**
* \brief Structure that encapsulates Task start command response
*/
struct task_control_task_start_response {
	/** An identifier that distinguishes a specific task run. */
	uint64_t task_run_id;
	/** A unique system-wide identifier for a task. */
	struct task_locator locator;
};

/**
* \brief Structure that encapsulates Task get status response
*/
struct task_control_task_get_status_response {
	uint32_t exit_code;
	uint32_t task_id;
	uint16_t stream_id;
	uint8_t run_state;
};

struct task_manager_response_packet {
	__attribute__((aligned(8))) uint8_t bytes[27];

	union {
		struct task_manager_get_info_response get_info_response;
		struct task_manager_get_command_info_response
			get_command_info_response;
		struct task_manager_get_built_in_symbol_response
			get_built_in_task_response;
		struct task_manager_abort_stream_id_response
			abort_stream_id_response;
		struct task_manager_task_create_response task_create_response;
		struct task_control_task_find_response task_find_response;
		struct task_control_task_abort_response task_abort_response;
		struct task_control_task_start_response task_start_response;
		struct task_control_task_get_status_response
			task_get_status_response;
	};

	uint8_t type;
	CvcoreStatus status;
};

enum task_manager_response_id
task_manager_response_get_type(struct task_manager_response_packet *packet);

struct task_manager_get_info_response *
get_manager_info_response(struct task_manager_response_packet *packet);

void print_manager_info(struct dsp_core_task_manager_info *rsp);

struct task_manager_get_command_info_response *
get_manager_command_info_response(struct task_manager_response_packet *packet);

struct task_manager_get_built_in_symbol_response *
get_manager_built_in_symbol_response(
	struct task_manager_response_packet *packet);

int task_manager_response_get_status(struct task_manager_response_packet *rsp);
struct task_manager_abort_stream_id_response *
get_manager_abort_stream_id_response(struct task_manager_response_packet *rsp);

struct task_manager_task_create_response *get_task_manager_task_create_response(
	struct task_manager_response_packet *packet);

struct task_control_task_find_response *get_task_control_task_find_response(
	struct task_manager_response_packet *packet);

struct task_control_task_abort_response *get_task_control_task_abort_response(
	struct task_manager_response_packet *packet);

struct task_control_task_start_response *get_task_control_task_start_response(
	struct task_manager_response_packet *packet);

struct task_control_task_get_status_response *
get_task_control_task_get_status_response(
	struct task_manager_response_packet *packet);
/// ///////////////////////////////////////////////////////////

int get_task_manager_response_packet_task_locator(
	const struct task_manager_response_packet *packet,
	enum task_manager_response_id rsp_id, struct task_locator *locator);

int get_task_manager_response_packet_task_run_id(
	const struct task_manager_response_packet *packet,
	enum task_manager_response_id rsp_id, uint64_t *run_id);

void initialize_task_manager_response_bytes_from_packet(
	const struct task_manager_response_packet *packet, uint8_t *bytes);

#endif
