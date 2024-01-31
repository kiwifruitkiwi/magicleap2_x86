// SPDX-License-Identifier: GPL-2.0
//
// (C) Copyright 2023
// Magic Leap, Inc. (COMPANY)
//
#include "dsp_core_control_packets.h"

static inline void set_type(struct task_manager_control_packet *packet,
			    enum task_manager_command_id type_)
{
	packet->bytes[26] = (uint8_t)type_;
}

static inline void
set_type_from_bytes(struct task_manager_control_packet *packet)
{
	packet->type = packet->bytes[26];
}

enum task_manager_command_id
task_manager_command_get_type(struct task_manager_control_packet *packet)
{
	return (enum task_manager_command_id)packet->bytes[26];
}

void initialize_task_manager_control_packet_from_bytes(
	struct task_manager_control_packet *packet, uint8_t *bytes)
{
	memcpy(packet->bytes, bytes, 27);
}

struct task_manager_get_info_command *
get_manager_info_command(struct task_manager_control_packet *packet)
{
	set_type_from_bytes(packet);
	packet->get_info_command.unused = packet->bytes[0];
	return &packet->get_info_command;
}

struct task_manager_get_info_command *
initialize_manager_info_command(struct task_manager_control_packet *packet)
{
	set_type(packet, kCmdManagerGetInfo);
	packet->bytes[0] = 0;
	return get_manager_info_command(packet);
}

struct task_manager_get_command_info_command *
get_manager_command_info_command(struct task_manager_control_packet *packet)
{
	set_type_from_bytes(packet);
	packet->get_command_info_command.command_id = packet->bytes[0];
	return &packet->get_command_info_command;
}

struct task_manager_get_command_info_command *
initialize_manager_command_info_command(
	struct task_manager_control_packet *packet,
	/* The command to lookup support. */
	uint8_t command_id_)
{
	set_type(packet, kCmdManagerGetCommands);
	packet->bytes[0] = command_id_;
	return get_manager_command_info_command(packet);
}

struct task_manager_get_built_in_symbol_command *
get_manager_built_in_symbol_command(struct task_manager_control_packet *packet)
{
	set_type_from_bytes(packet);
	packet->get_built_in_symbol_command.task_id = packet->bytes[0];
	packet->get_built_in_symbol_command.delegate_id = packet->bytes[1];
	return &packet->get_built_in_symbol_command;
}

struct task_manager_get_built_in_symbol_command *
initialize_manager_get_built_in_task_command(
	struct task_manager_control_packet *packet,
	/** The index of a built-in task to retrieve. */
	uint8_t task_id_)
{
	set_type(packet, kCmdManagerGetBuiltInTaskOrDelegate);
	packet->bytes[0] = task_id_;
	packet->bytes[1] = kBuiltInDelegateID_UNDEFINED;
	return get_manager_built_in_symbol_command(packet);
}

struct task_manager_get_built_in_symbol_command *
initialize_manager_get_built_in_delegate_command(
	struct task_manager_control_packet *packet,
	/** The index of a built-in delegate to retrieve. */
	uint8_t delegate_id_)
{
	set_type(packet, kCmdManagerGetBuiltInTaskOrDelegate);
	packet->bytes[0] = kBuiltInTaskID_UNDEFINED;
	packet->bytes[1] = delegate_id_;
	return get_manager_built_in_symbol_command(packet);
}

struct task_manager_abort_stream_id_command *
get_task_manager_abort_stream_id_command(
	struct task_manager_control_packet *packet)
{
	set_type_from_bytes(packet);
	*((uint16_t *)&packet->abort_stream_id_command.stream_id) =
		*((uint16_t *)&packet->bytes[0]);
	return &packet->abort_stream_id_command;
}

struct task_manager_abort_stream_id_command *
initialize_task_manager_abort_stream_id_command(
	struct task_manager_control_packet *packet,
	/** The stream id of tasks to be aborted. */
	uint16_t stream_id_)
{
	set_type(packet, kCmdManagerAbortStreamID);

	*((uint16_t *)&packet->bytes[0]) = *((uint16_t *)&stream_id_);
	return get_task_manager_abort_stream_id_command(packet);
}

struct task_manager_task_create_command *
get_task_manager_task_create_command(struct task_manager_control_packet *packet)
{
	set_type_from_bytes(packet);
	packet->task_create_command.task_info_delegate_ptr =
		*((uint64_t *)&packet->bytes[0]);
	packet->task_create_command.task_unique_id =
		*((uint64_t *)&packet->bytes[8]);
	packet->task_create_command.stack_size =
		*(uint32_t *)&packet->bytes[16]; // uint32_t
	packet->task_create_command.task_creation_parameters =
		*(uint32_t *)&packet->bytes[20]; // uint32_t
	packet->task_create_command.priority = *((uint8_t *)&packet->bytes[24]);
	return &packet->task_create_command;
}

struct task_manager_task_create_command *
initialize_task_manager_task_create_command(
	struct task_manager_control_packet *packet, uint64_t task_creator_,
	uint64_t task_unique_id_, uint32_t stack_size_,
	uint32_t task_creation_parameters_, uint8_t priority_)
{
	set_type(packet, kCmdTaskCreate);

	*((uint64_t *)&packet->bytes[0]) = *((uint64_t *)&task_creator_);
	*((uint64_t *)&packet->bytes[8]) = *((uint64_t *)&task_unique_id_);
	*((uint32_t *)&packet->bytes[16]) = *((uint32_t *)&stack_size_);
	*((uint32_t *)&packet->bytes[20]) =
		task_creation_parameters_; // uint32_t
	*((uint8_t *)&packet->bytes[24]) = *((uint8_t *)&priority_);
	return get_task_manager_task_create_command(packet);
}

struct task_control_task_find_command *
get_task_control_task_find_command(struct task_manager_control_packet *packet)
{
	set_type_from_bytes(packet);

	*((uint64_t *)&packet->task_find_command.task_unique_id) =
		*((uint64_t *)&packet->bytes[0]);
	return &packet->task_find_command;
}

struct task_control_task_find_command *
initialize_task_control_task_find_command(
	struct task_manager_control_packet *packet,
	/** Unique id of that task being searched*/
	uint64_t unique_id_)
{
	set_type(packet, kCmdTaskFind);
	*((uint64_t *)&packet->bytes[0]) = unique_id_;

	return get_task_control_task_find_command(packet);
}

struct task_control_task_abort_command *
get_task_control_task_abort_command(struct task_manager_control_packet *packet)
{
	set_type_from_bytes(packet);
	packet->task_abort_command.task_run_id =
		*((uint64_t *)&packet->bytes[0]);
	packet->task_abort_command.locator.core_id = packet->bytes[8];
	packet->task_abort_command.locator.unique_id = packet->bytes[9];
	return &packet->task_abort_command;
}

struct task_control_task_abort_command *
initialize_task_control_task_abort_command(
	struct task_manager_control_packet *packet,
	/** The unique identifier of the task to abort. */
	struct task_locator task_locator_, uint64_t task_run_id_)
{
	set_type(packet, kCmdTaskAbort);
	*((uint64_t *)&packet->bytes[0]) = task_run_id_;
	packet->bytes[8] = task_locator_.core_id;
	packet->bytes[9] = task_locator_.unique_id;
	return get_task_control_task_abort_command(packet);
}

struct task_control_task_start_command *
get_task_control_task_start_command(struct task_manager_control_packet *packet)
{
	set_type_from_bytes(packet);
	*((uint64_t *)&packet->task_start_command.trigger_event_name) =
		*((uint64_t *)&packet->bytes[0]);
	*((uint64_t *)&packet->task_start_command.arguments) =
		*((uint64_t *)&packet->bytes[8]);
	*((uint32_t *)&packet->task_start_command.timeout_us) =
		*((uint32_t *)&packet->bytes[16]);
	packet->task_start_command.locator.core_id = packet->bytes[20];
	packet->task_start_command.locator.unique_id = packet->bytes[21];
	*((uint16_t *)&packet->task_start_command.stream_id) =
		*((uint16_t *)&packet->bytes[22]);
	packet->task_start_command.priority = packet->bytes[24];
	packet->task_start_command.flags = packet->bytes[25];
	return &packet->task_start_command;
}

struct task_control_task_start_command *
initialize_task_control_task_start_command(
	struct task_manager_control_packet *packet,
	struct task_locator task_locator_, uint8_t flags_, uint8_t *arguments_,
	uint64_t trigger_event_name_, uint32_t timeout_us_, uint16_t stream_id_,
	uint8_t priority_)
{
	int b;

	set_type(packet, kCmdTaskStart);
	*((uint64_t *)&packet->bytes[0]) = *((uint64_t *)&trigger_event_name_);

	if (arguments_) {
		for (b = 0; b < 8; b++) {
			packet->bytes[8 + b] = arguments_[b];
		}
	}

	*((uint32_t *)&packet->bytes[16]) = *((uint32_t *)&timeout_us_);
	packet->bytes[20] = task_locator_.core_id;
	packet->bytes[21] = task_locator_.unique_id;
	*((uint16_t *)&packet->bytes[22]) = *((uint16_t *)&stream_id_);
	packet->bytes[24] = priority_;
	packet->bytes[25] = (uint8_t)flags_;
	return get_task_control_task_start_command(packet);
}

struct task_control_task_get_status_command *
get_task_control_task_get_status_command(
	struct task_manager_control_packet *packet)
{
	set_type_from_bytes(packet);
	*((uint64_t *)&packet->task_get_status_command.task_run_id) =
		*((uint64_t *)&packet->bytes[0]);
	packet->task_get_status_command.locator.core_id = packet->bytes[8];
	packet->task_get_status_command.locator.unique_id = packet->bytes[9];
	return &packet->task_get_status_command;
}

struct task_control_task_get_status_command *
initialize_task_control_task_get_status_command(
	struct task_manager_control_packet *packet,
	/** A unique identifier for the task to query. */
	struct task_locator task_locator_,
	/** An identifier that distinguishes a specific task run. */
	uint64_t task_run_id_)
{
	set_type(packet, kCmdTaskGetStatus);
	*((uint64_t *)&packet->bytes[0]) = *((uint64_t *)&task_run_id_);
	packet->bytes[8] = task_locator_.core_id;
	packet->bytes[9] = task_locator_.unique_id;
	return get_task_control_task_get_status_command(packet);
}

int set_task_manager_control_packet_stream_id(
	struct task_manager_control_packet *packet,
	enum task_manager_command_id cmd_id, uint16_t stream_id)
{
	return 0;
}

int set_task_manager_control_packet_task_locator(
	struct task_manager_control_packet *packet,
	enum task_manager_command_id cmd_id, const struct task_locator *locator)
{
	return 0;
}

int set_task_manager_control_packet_task_run_id(
	struct task_manager_control_packet *packet,
	enum task_manager_command_id cmd_id, uint64_t run_id)
{
	return 0;
}
