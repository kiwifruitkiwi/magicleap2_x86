// SPDX-License-Identifier: GPL-2.0
//
// (C) Copyright 2023
// Magic Leap, Inc. (COMPANY)
//
#define pr_fmt(fmt) "%s: " fmt, KBUILD_MODNAME

#include <linux/printk.h>

#include "dsp_core_response_packets.h"

#include "cvcore_status.h"
#include "dsp_core_common.h"
#include "dsp_core_types.h"
#include "dsp_core_uapi.h"

int task_manager_response_get_status(struct task_manager_response_packet *rsp)
{
	return rsp->bytes[20] == CVCORE_STATUS_SUCCESS ? 0 : rsp->bytes[20];
}

void set_type_and_status_from_bytes(struct task_manager_response_packet *rsp)
{
	rsp->type = rsp->bytes[26];
	rsp->status = *((CvcoreStatus *)&rsp->bytes[20]);
}

void set_type_and_status(struct task_manager_response_packet *rsp,
			 uint8_t type_, CvcoreStatus status_)
{
	rsp->bytes[26] = (uint8_t)type_;
	*((uint32_t *)&rsp->bytes[20]) = (uint32_t)status_;
}

enum task_manager_response_id
task_manager_response_get_type(struct task_manager_response_packet *rsp)
{
	return (enum task_manager_response_id)rsp->bytes[26];
}

void initialize_task_manager_response_bytes_from_packet(
	const struct task_manager_response_packet *packet, uint8_t *bytes)
{
	memcpy(bytes, packet->bytes, 27);
}

struct task_manager_get_info_response *
get_manager_info_response(struct task_manager_response_packet *packet)
{
	set_type_and_status_from_bytes(packet);
	packet->get_info_response.info.task_locator_first.core_id =
		packet->bytes[0];
	packet->get_info_response.info.task_locator_first.unique_id =
		packet->bytes[1];
	packet->get_info_response.info.task_locator_last.core_id =
		packet->bytes[2];
	packet->get_info_response.info.task_locator_last.unique_id =
		packet->bytes[3];
	packet->get_info_response.info.capabilities =
		(enum dsp_core_task_manager_capabilities)packet->bytes[4];
	packet->get_info_response.info.max_tasks = packet->bytes[5];
	packet->get_info_response.info.num_executive_tasks = packet->bytes[6];
	packet->get_info_response.info.num_user_tasks = packet->bytes[7];
	packet->get_info_response.info.num_resident_executive_tasks =
		packet->bytes[8];
	packet->get_info_response.info.num_resident_user_tasks =
		packet->bytes[9];
	packet->get_info_response.info.num_non_resident_executive_tasks =
		packet->bytes[10];
	packet->get_info_response.info.num_non_resident_user_tasks =
		packet->bytes[11];
	packet->get_info_response.info.num_built_in_tasks = packet->bytes[12];
	packet->get_info_response.info.num_built_in_delegates =
		packet->bytes[13];
	return &packet->get_info_response;
}

void print_manager_info(struct dsp_core_task_manager_info *info)
{
	DSP_CORE_INFO("------------------Task Manager Discovery -------------------\n");
	DSP_CORE_INFO("Task Manager Info:\t task_locator_first 0x%0X.0x%0X\n",
		info->task_locator_first.core_id,
		info->task_locator_first.unique_id);
	DSP_CORE_INFO("Task Manager Info:\t task_locator_last 0x%0X.0x%0X\n",
		info->task_locator_first.core_id,
		info->task_locator_first.unique_id);
	DSP_CORE_INFO("Task Manager Info:\t max_tasks %d\n", info->max_tasks);
	DSP_CORE_INFO("Task Manager Info:\t num_executive_tasks %d\n",
		info->num_executive_tasks);
	DSP_CORE_INFO("Task Manager Info:\t num_user_tasks %d\n",
		info->num_user_tasks);
	DSP_CORE_INFO("Task Manager Info:\t num_resident_executive_tasks %d\n",
		info->num_resident_executive_tasks);
	DSP_CORE_INFO("Task Manager Info:\t num_resident_user_tasks %d\n",
		info->num_resident_user_tasks);
	DSP_CORE_INFO("Task Manager Info:\t num_non_resident_executive_tasks %d\n",
		info->num_non_resident_executive_tasks);
	DSP_CORE_INFO("Task Manager Info:\t num_non_resident_user_tasks %d\n",
		info->num_non_resident_user_tasks);
	DSP_CORE_INFO("Task Manager Info:\t num_registered_built_in_tasks %d\n",
		info->num_built_in_tasks);
	DSP_CORE_INFO("Task Manager Info:\t num_registered_built_in_delegates %d\n",
		info->num_built_in_delegates);
	DSP_CORE_INFO("------------------------------------------------------------\n");
}

struct task_manager_get_command_info_response *
get_manager_command_info_response(struct task_manager_response_packet *packet)
{
	set_type_and_status_from_bytes(packet);
	packet->get_command_info_response.command_channels =
		*((uint16_t *)&packet->bytes[0]);
	packet->get_command_info_response.command_id = packet->bytes[2];
	return &packet->get_command_info_response;
}

struct task_manager_get_built_in_symbol_response *
get_manager_built_in_symbol_response(struct task_manager_response_packet *packet)
{
	set_type_and_status_from_bytes(packet);
	packet->get_built_in_task_response.ptr =
		*((uint64_t *)&packet->bytes[0]);
	return &packet->get_built_in_task_response;
}

struct task_manager_abort_stream_id_response *
get_manager_abort_stream_id_response(struct task_manager_response_packet *packet)
{
	set_type_and_status_from_bytes(packet);
	packet->abort_stream_id_response.num_aborted = packet->bytes[0];
	return &packet->abort_stream_id_response;
}

struct task_manager_task_create_response *get_task_manager_task_create_response(
	struct task_manager_response_packet *packet)
{
	set_type_and_status_from_bytes(packet);
	packet->task_create_response.task_argument_area_base =
		*((uint64_t *)&packet->bytes[0]);
	packet->task_create_response.task_argument_area_size =
		*((uint16_t *)&packet->bytes[8]);
	packet->task_create_response.task_locator.core_id = packet->bytes[10];
	packet->task_create_response.task_locator.unique_id = packet->bytes[11];
	return &packet->task_create_response;
}

struct task_control_task_find_response *
get_task_control_task_find_response(struct task_manager_response_packet *packet)
{
	set_type_and_status_from_bytes(packet);
	packet->task_find_response.task_argument_area_base =
		*((uint64_t *)&packet->bytes[0]);
	packet->task_find_response.task_argument_area_size =
		*((uint16_t *)&packet->bytes[8]);
	packet->task_find_response.task_locator.core_id = packet->bytes[10];
	packet->task_find_response.task_locator.unique_id = packet->bytes[11];
	return &packet->task_find_response;
}

struct task_control_task_abort_response *get_task_control_task_abort_response(
	struct task_manager_response_packet *packet)
{
	set_type_and_status_from_bytes(packet);
	packet->task_abort_response.task_state_on_abort.exit_code =
		*((uint32_t *)&packet->bytes[0]);
	packet->task_abort_response.task_state_on_abort.stream_id =
		*((uint16_t *)&packet->bytes[8]);
	packet->task_abort_response.task_state_on_abort.run_state =
		packet->bytes[12];
	return &packet->task_abort_response;
}

struct task_control_task_start_response *get_task_control_task_start_response(
	struct task_manager_response_packet *packet)
{
	set_type_and_status_from_bytes(packet);
	packet->task_start_response.task_run_id =
		*((uint64_t *)&packet->bytes[0]);
	packet->task_start_response.locator.core_id = packet->bytes[8];
	packet->task_start_response.locator.unique_id = packet->bytes[9];
	return &packet->task_start_response;
}

struct task_control_task_get_status_response *
get_task_control_task_get_status_response(
	struct task_manager_response_packet *packet)
{
	set_type_and_status_from_bytes(packet);
	packet->task_get_status_response.exit_code =
		*((uint32_t *)&packet->bytes[0]);
	packet->task_get_status_response.task_id =
		*((uint32_t *)&packet->bytes[4]);
	packet->task_get_status_response.stream_id =
		*((uint16_t *)&packet->bytes[8]);
	packet->task_get_status_response.run_state = packet->bytes[12];
	return &packet->task_get_status_response;
}

int get_task_manager_response_packet_task_locator(
	const struct task_manager_response_packet *packet,
	enum task_manager_response_id rsp_id, struct task_locator *locator)
{
	return 0;
}

int get_task_manager_response_packet_task_run_id(
	const struct task_manager_response_packet *packet,
	enum task_manager_response_id rsp_id, uint64_t *run_id)
{
	return 0;
}
