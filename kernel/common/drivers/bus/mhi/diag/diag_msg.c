// SPDX-License-Identifier: BSD-3-Clause-Clear
/**
 * Copyright (c) 2019 The Linux Foundation. All rights reserved.
 */

#include <linux/kernel.h>
#include "diag_msg.h"

int diag_create_msg_set_rt_mask_req(struct diag_reg_param *diag_param,
				    struct msg_set_rt_mask_req_type *req,
				    int *req_len,
				    u16 ssid_start,
				    u16 ssid_end,
				    u32 mask)
{
	int i, index = 0, range = ssid_end - ssid_start + 1;
	*req_len = sizeof(struct msg_set_rt_mask_req_type) +
		   (ssid_end - ssid_start) * sizeof(uint32_t);

	if (!req) {
		pr_err("%s: Invalid request\n", __func__);
		return -1;
	}

	req->cmd_code   = DIAG_CMD_MSG_CONFIG;
	req->sub_cmd    = DIAG_CMD_OP_SET_MSG_MASK;
	req->ssid_start = ssid_start;
	req->ssid_end   = ssid_end;
	req->pad        = 0;

	for (i = 0; i < range; i++)
		req->rt_mask[i] = mask;

	pr_debug("%s req->rt_mask=%d\n", __func__, req->rt_mask[0]);
	//update mask from registry key value.
	for (i = 0; i < MAX_DIAG_PARAM; i++) {
		if (diag_param[i].ssid == 0)
			break;

		if (diag_param[i].ssid >= req->ssid_start &&
		    diag_param[i].ssid <= req->ssid_end){
			index = diag_param[i].ssid - req->ssid_start;
			req->rt_mask[index] = diag_param[i].mask;
		}
	}

	return 0;
}

int diag_create_msg_event_toggle_req(struct msg_set_toggle_config_req_type *req,
				     int *req_len,
				     u8 enable)
{
	*req_len = sizeof(struct msg_set_toggle_config_req_type);
	req->cmd_code = DIAG_CMD_EVENT_TOGGLE;
	req->enable = enable;

	return 0;
}

int diag_create_msg_set_event_mask_req(struct msg_set_event_mask_req_type *req,
				       int *req_len,
				       u8 mask)
{
	int ret = 0;
	u8 *mask_ptr = NULL;
	u16 num_bytes = 0;
	int i = 0;

	mask_ptr = (u8 *)(req + 1);

	req->cmd_code = DIAG_CMD_SET_EVENT_MASK;
	req->status = 0;
	req->padding = 0;
	req->num_bits = EVENT_LAST_ID;
	num_bytes = (req->num_bits + 7) / 8;

	*req_len = sizeof(struct msg_set_event_mask_req_type) + num_bytes;

	for (i = 0; i < num_bytes; i++)
		mask_ptr[i] = mask;

	return ret;
}

int diag_create_msg_set_log_mask_req(struct msg_set_log_mask_req_type *req,
				     int *req_len,
				     u8 mask)
{
	u8 *mask_ptr = NULL;
	u32 num_bytes = 0;
	u32 i = 0;

	mask_ptr = (uint8_t *)(req + 1);

	req->cmd_code = DIAG_CMD_LOG_CONFIG;
	req->padding[0] = 0;
	req->padding[1] = 0;
	req->padding[2] = 0;
	req->sub_cmd = DIAG_CMD_OP_SET_LOG_MASK;
	req->equip_id = LOG_EQUIP_ID_1X;
	req->num_items = LOG_GET_ITEM_NUM(LOG_EQUIP_ID_1_LAST_CODE);
	num_bytes = (req->num_items / 8) + 1;

	*req_len = sizeof(struct msg_set_log_mask_req_type) + num_bytes;

	/* temporarily enable all bits for equipment ID 0x1 */
	for (i = 0; i < num_bytes; i++)
		mask_ptr[i] = mask;

	return 0;
}
