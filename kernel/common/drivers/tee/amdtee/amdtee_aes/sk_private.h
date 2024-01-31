/* SPDX-License-Identifier: MIT */
/*
 * Copyright (C) 2020-2020 Advanced Micro Devices, Inc. All rights reserved.
 */

#ifndef SK_PRIVATE_H_
#define SK_PRIVATE_H_

/*
 * Trusted Application Command IDs
 */
enum _TA_AES_CMDS {
	CMD_AES_ECB_ENC  = 1,    /* AES-ECB Encrypt */
	CMD_AES_ECB_DEC  = 2,    /* AES-ECB Decrypt */
	CMD_AES_ECB_SET_KEY = 3,
};

/*
 * Return Error Codes
 */
#define TA_AES_ERR_ALLOCATE_TRANSIENT_OBJ_FAIL      0x100
#define TA_AES_ERR_INIT_REF_ATTRIBUTE_FAIL          0x101
#define TA_AES_ERR_POPULATE_TRANSIENT_OBJ           0x102
#define TA_AES_ERR_ALLOCATE_OPERATION_FAIL          0x103
#define TA_AES_ERR_SET_OPERATION_KEY_FAIL           0x104
#define TA_AES_ERR_UNKNOWN_CMD                      0x105

/*
 * Defines the number of available memory references in an open session or
 * invoke command operation payload.
 */
#define TEEC_CONFIG_PAYLOAD_REF_COUNT 4

#endif
