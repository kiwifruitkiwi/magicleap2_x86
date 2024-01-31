/* SPDX-License-Identifier: GPL-2.0-only
 *
 * Copyright (c) 2023, Magic Leap, Inc. All rights reserved.
 */
#ifndef __DSP_CORE_CONTROL_PACKETS_H
#define __DSP_CORE_CONTROL_PACKETS_H

#include <linux/types.h>
#include <stdalign.h>

#include "dsp_core_types.h"

enum task_manager_command_id {
	kCmdTaskCreate = 0,
	kCmdTaskStart,
	kCmdTaskAbort,
	kCmdTaskGetStatus,
	kCmdTaskGetPriority,
	kCmdTaskSetPriority,
	kCmdTaskGetArgumentInfo,
	kCmdTaskFind,
	kCmdDelegateStart,
	kCmdManagerGetBuiltInTaskOrDelegate,
	kCmdManagerGetInfo,
	kCmdManagerGetCommands,
	kCmdManagerAbortStreamID,
	kCmdWorkQueueEnqueue,
	kCmdShutdown,
	kCmdLast,
	kCmdInvalid
};
#define TM_COMMAND_FIRST (kCmdTaskCreate)
#define TM_COMMAND_LAST (kCmdLast - 1)
#define TM_COMMAND_COUNT (kCmdLast)

struct task_manager_abort_stream_id_command {
	/** The stream id to be associated with the task. */
	uint16_t stream_id; //2-bytes
};

/**
* Structure that encapsulates a command to retrieve
* configuration information about a task_manager
* instance
*/
struct task_manager_get_info_command {
	uint8_t unused;
};

/**
* Structure that encapsulates a command to retrieve
* supported commands between task_control and
* task_manager
*/
struct task_manager_get_command_info_command {
	uint8_t command_id;
};

/**
* Structure that encapsulates a command to retrieve
* the address of a creator for a built-in task
* specified by the provided index
 */
struct task_manager_get_built_in_symbol_command {
	uint8_t task_id;
	uint8_t delegate_id;
};

/**
* \brief Structure that encapsulates Task creation parameters
*/
struct task_manager_task_create_command {
	/** A pointer to a function that returns info about a task to be created. */
	uint64_t task_info_delegate_ptr;

	/** A unique id to refer to the task. */
	uint64_t task_unique_id;

	/** The required stack size for the task. */
	uint32_t stack_size;

	/** Task creation parameters */
	uint32_t task_creation_parameters;

	/** The starting priority of the task*/
	uint8_t priority;
};

struct task_control_task_find_command {
	/** A unique id of the task being searched. */
	uint64_t task_unique_id;
};

/**
* \brief Structure that encapsulates Task abort parameters
*/
struct task_control_task_abort_command {
	/** An identifier that distinguishes a specific task run. */
	uint64_t task_run_id;

	/** The unique identifier of the task to modify. */
	struct task_locator locator;
};

/**
* \brief Structure that encapsulates Task start parameters
*/
struct task_control_task_start_command {
	/** If specified, a named event on the DSP that will trigger the start of the task. */
	uint64_t trigger_event_name; //8-bytes

	/** An application specific handle to arguments to pass to the task at start. */
	uint64_t arguments; //8-bytes

	/** The max time in microseconds that the task is allowed to run. Zero for indefinite.*/
	uint32_t timeout_us; //4-bytes

	/** The unique_locator of the task to start. */
	struct task_locator locator; //2-bytes

	/** The stream id to be associated with the task. */
	uint16_t stream_id; //2-bytes

	/** The priority level to run the task. Priority can range from 1 to TASK_MAX_PRIORITY
	 ** with 0 being the least urgent. */
	uint8_t priority; //1-byte

	/** Task flags to use on start */
	uint8_t flags; //1-byte
};

/**
* \brief Structure that encapsulates Task get status command
*/
struct task_control_task_get_status_command {
	/** An identifier that distinguishes a specific task run. */
	uint64_t task_run_id;

	/** The unique identifier of the task to perform inquiry. */
	struct task_locator locator; //2-bytes
};

struct task_manager_control_packet {
	__attribute__((aligned(8))) uint8_t bytes[27];

	union {
		struct task_manager_abort_stream_id_command
			abort_stream_id_command;
		struct task_manager_get_info_command get_info_command;
		struct task_manager_get_command_info_command
			get_command_info_command;
		struct task_manager_get_built_in_symbol_command
			get_built_in_symbol_command;
		struct task_manager_task_create_command task_create_command;
		struct task_control_task_find_command task_find_command;
		struct task_control_task_abort_command task_abort_command;
		struct task_control_task_start_command task_start_command;
		struct task_control_task_get_status_command
			task_get_status_command;
	};

	uint8_t type;
};

enum task_manager_command_id
task_manager_command_get_type(struct task_manager_control_packet *packet);

struct task_manager_get_info_command *
get_manager_info_command(struct task_manager_control_packet *packet);

struct task_manager_get_info_command *
initialize_manager_info_command(struct task_manager_control_packet *packet);

struct task_manager_get_command_info_command *
get_manager_command_info_command(struct task_manager_control_packet *packet);

struct task_manager_get_command_info_command *
initialize_manager_command_info_command(
	struct task_manager_control_packet *packet, uint8_t command_id_);

struct task_manager_get_built_in_symbol_command *
get_manager_built_in_symbol_command(struct task_manager_control_packet *packet);

struct task_manager_get_built_in_symbol_command *
initialize_manager_get_built_in_task_command(
	struct task_manager_control_packet *packet, uint8_t task_id_);

struct task_manager_get_built_in_symbol_command *
initialize_manager_get_built_in_delegate_command(
	struct task_manager_control_packet *packet, uint8_t delegate_id_);

struct task_manager_abort_stream_id_command *
get_task_manager_abort_stream_id_command(
	struct task_manager_control_packet *packet);

struct task_manager_abort_stream_id_command *
initialize_task_manager_abort_stream_id_command(
	struct task_manager_control_packet *packet,
	/** The stream id of tasks to be aborted. */
	uint16_t stream_id_);

struct task_manager_task_create_command *get_task_manager_task_create_command(
	struct task_manager_control_packet *packet);

struct task_manager_task_create_command *
initialize_task_manager_task_create_command(
	struct task_manager_control_packet *packet, uint64_t task_creator_,
	uint64_t task_unique_id_, uint32_t stack_size_,
	uint32_t task_creation_parameters_, uint8_t priority_);

struct task_control_task_find_command *
get_task_control_task_find_command(struct task_manager_control_packet *packet);

struct task_control_task_find_command *
initialize_task_control_task_find_command(
	struct task_manager_control_packet *packet,
	/** Unique id of that task being searched*/
	uint64_t unique_id_);

struct task_control_task_abort_command *
get_task_control_task_abort_command(struct task_manager_control_packet *packet);

struct task_control_task_abort_command *
initialize_task_control_task_abort_command(
	struct task_manager_control_packet *packet,
	/** The unique identifier of the task to abort. */
	struct task_locator task_locator_, uint64_t task_run_id_);

struct task_control_task_start_command *
get_task_control_task_start_command(struct task_manager_control_packet *packet);

struct task_control_task_start_command *
initialize_task_control_task_start_command(
	struct task_manager_control_packet *packet,
	struct task_locator task_locator_, uint8_t flags_, uint8_t *arguments_,
	uint64_t trigger_event_name_, uint32_t timeout_us_, uint16_t stream_id_,
	uint8_t priority_);

struct task_control_task_get_status_command *
get_task_control_task_get_status_command(
	struct task_manager_control_packet *packet);
struct task_control_task_get_status_command *
initialize_task_control_task_get_status_command(
	struct task_manager_control_packet *packet,
	/** A unique identifier for the task to query. */
	struct task_locator task_locator_,
	/** An identifier that distinguishes a specific task run. */
	uint64_t task_run_id_);

////////////////////////////////////////////////////////////////////////

int set_task_manager_control_packet_stream_id(
	struct task_manager_control_packet *packet,
	enum task_manager_command_id cmd_id, uint16_t stream_id);

int set_task_manager_control_packet_task_locator(
	struct task_manager_control_packet *packet,
	enum task_manager_command_id cmd_id,
	const struct task_locator *locator);

int set_task_manager_control_packet_task_run_id(
	struct task_manager_control_packet *packet,
	enum task_manager_command_id cmd_id, uint64_t run_id);

void initialize_task_manager_control_packet_from_bytes(
	struct task_manager_control_packet *packet, uint8_t *bytes);

#endif
