/* SPDX-License-Identifier: BSD-3-Clause-Clear */
/**
 * Copyright (c) 2019 The Linux Foundation. All rights reserved.
 */

#ifndef DIAG_MSG_H
#define DIAG_MSG_H

#include <linux/types.h>
#include "diag_msg_pkt_defs.h"

#define DIAG_CMD_EVENT_TOGGLE	0x60
#define DIAG_CMD_LOG_CONFIG     0x73
#define DIAG_CMD_SET_EVENT_MASK	0x82
#define DIAG_CMD_MSG_CONFIG	    0x7D

#define DIAG_CMD_OP_SET_LOG_MASK	3
#define DIAG_CMD_OP_SET_MSG_MASK	4

#define EVENT_LAST_ID               0xB27

#define LOG_GET_ITEM_NUM(xx_code)        ((xx_code) & 0x0FFF)

#define LOG_EQUIP_ID_1X             1
/* for eqiupment ID 0x1*/
#define LOG_1X_BASE_C               ((u16)0x1000)
#define LOG_1X_LAST_C               ((0xA11) + LOG_1X_BASE_C)
#define LOG_LAST_C                  (LOG_1X_LAST_C & 0xFFF)
#define LOG_EQUIP_ID_1_LAST_CODE    LOG_LAST_C

#define MAX_DIAG_PARAM        128

struct diag_reg_param {
	u16 ssid;
	u8  mask;
};

int diag_create_msg_set_rt_mask_req(struct diag_reg_param *diag_param,
				    struct msg_set_rt_mask_req_type *req,
				    int *req_len,
				    u16 ssid_start,
				    u16 ssid_end,
				    u32 mask);

int diag_create_msg_event_toggle_req(struct msg_set_toggle_config_req_type *req,
				     int *req_len,
				     u8 enable);

int diag_create_msg_set_event_mask_req(struct msg_set_event_mask_req_type *req,
				       int *req_len,
				       u8 mask);

int diag_create_msg_set_log_mask_req(struct msg_set_log_mask_req_type *req,
				     int *req_len,
				     u8 mask);

#define DIAG_PRINT_MAX_SIZE       512

#define DIAG_CMD_EXT_MSG        0x79
#define DIAG_CMD_MSG_CONFIG	    0x7D

void diag_local_ext_msg_report_handler(u8 *buf, int len);

#endif
