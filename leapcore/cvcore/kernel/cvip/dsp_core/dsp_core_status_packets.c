// SPDX-License-Identifier: GPL-2.0
//
// (C) Copyright 2023
// Magic Leap, Inc. (COMPANY)
//
#define pr_fmt(fmt) "%s: " fmt, KBUILD_MODNAME

#include <linux/printk.h>

#include "dsp_core_status_packets.h"

#include "cvcore_status.h"
#include "dsp_core_types.h"
#include "dsp_core_uapi.h"

static void set_type_from_bytes(struct task_manager_status_packet *packet)
{
	packet->type_id = packet->bytes[25];
}

enum task_manager_status_type_id
task_manager_status_get_type(struct task_manager_status_packet *packet)
{
	return (enum task_manager_status_type_id)packet->bytes[25];
}

struct task_control_task_status_message *
get_task_control_task_status_message(struct task_manager_status_packet *packet)
{
	set_type_from_bytes(packet);
	packet->task_status_message.task_run_id =
		*((uint64_t *)&packet->bytes[0]);
	packet->task_status_message.exit_code =
		*((uint32_t *)&packet->bytes[8]);
	packet->task_status_message.task_id = *((uint32_t *)&packet->bytes[12]);
	packet->task_status_message.locator.core_id = packet->bytes[16];
	packet->task_status_message.locator.unique_id = packet->bytes[17];
	packet->task_status_message.stream_id =
		*((uint16_t *)&packet->bytes[18]);
	packet->task_status_message.run_state = packet->bytes[20];

	return &packet->task_status_message;
}
