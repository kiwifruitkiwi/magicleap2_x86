/*
 * IPC cvip smu channel configuration
 *
 * Copyright (C) 2020-2021 Advanced Micro Devices, Inc. All rights reserved.
 */

#include "ipc.h"
#include "mero_scpi.h"
#include "cvip.h"

#define SMU_THROTTLE_EVENT_VAR "THROTTLING="
#define SMU_THROTTLE_EVENT_STRING_SIZE 20
static const char * const smu_throttle_event[] = {
	"Start",
	"Stop",
};

#define SMU_THROTTLE_TYPE_VAR "TYPE="
#define SMU_THROTTLE_TYPE_STRING_SIZE 20
static const char * const smu_throttle_type[] = {
	"Power",
	"Current",
	"Thermal",
};

static bool smu_pm_enabled;

void smu_rx_callback(struct mbox_client *client, void *message)
{
	struct mbox_message *msg = (struct mbox_message *)message;
	u32 mid = msg->message_id;
	char throttle_str[SMU_THROTTLE_EVENT_STRING_SIZE];
	char throttle_type[SMU_THROTTLE_TYPE_STRING_SIZE];
	char *envp[3] = { throttle_str, throttle_type, NULL };
	int ret = 0;

	pr_debug(" %s: received data from SMU mailbox\n", __func__);
	switch (mid) {
	case SMU_M1SEND_SEND_READY:
		pr_debug("%s: received smu_pm enable from SMU mailbox\n",
			 __func__);
		smu_pm_enabled = true;
		/* Complete SMU boot handshake */
		set_smu_dpm_ready(1);
		break;
	case SMU_M1SEND_THROTTLE_STATUS:
		if (msg->msg_body[0] >= ARRAY_SIZE(smu_throttle_event)) {
			pr_err("unknown Throttling event\n");
			return;
		};
		if (msg->msg_body[1] >= ARRAY_SIZE(smu_throttle_type)) {
			pr_err("unknown Throttling type\n");
			return;
		};
		pr_debug("Throttle state = 0x%x, type = 0x%x\n",
			 msg->msg_body[0], msg->msg_body[1]);
		snprintf(throttle_str, sizeof(throttle_str),
			 "%s%s", SMU_THROTTLE_EVENT_VAR,
			 smu_throttle_event[msg->msg_body[0]]);
		snprintf(throttle_type, sizeof(throttle_type),
			 "%s%s", SMU_THROTTLE_TYPE_VAR,
			 smu_throttle_type[msg->msg_body[1]]);
		ret = kobject_uevent_env(&client->dev->kobj, KOBJ_CHANGE, envp);
		if (ret)
			pr_err("%s: kobject_uevent_env failed\n", __func__);
		set_smu_dpm_throttle(msg->msg_body[0], msg->msg_body[1]);
		break;
	default:
		pr_err("unknown SMU message\n");
	}
}

bool is_smu_pm_enabled(void)
{
	return smu_pm_enabled;
}
EXPORT_SYMBOL(is_smu_pm_enabled);
