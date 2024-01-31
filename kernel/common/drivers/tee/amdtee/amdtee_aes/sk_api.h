/* SPDX-License-Identifier: MIT */
/*
 * Copyright (C) 2020-2020 Advanced Micro Devices, Inc. All rights reserved.
 */

#ifndef SK_API_H_
#define SK_API_H_

#include <linux/types.h>
#include <linux/tee_drv.h>

#ifdef DLM_SUPPORT
#include <linux/sysfs.h>
#include "../amdtee_if.h"
#endif

struct amdtee_alg_ctx {
	struct list_head list;
	u32 key_len;

	u32 session_id;
	struct tee_shm *shm_in;
	struct tee_shm *shm_out;

#ifdef DLM_SUPPORT
	u32 dlm_sid;
#endif
};

struct amdtee_alg_dev {
	struct mutex dev_mutex;
	struct list_head tfm_list;
	u32 alg_cnt;

	struct device *dev;
	struct tee_context *ctx;

#ifdef DLM_SUPPORT
	struct debug_token {
		u8 *dlm_token;
		u32 dlm_token_size;
	} dt;

	struct bin_attribute attr;
#endif
};

/**
 * This type contains a Universally Unique Resource Identifier (UUID) type as
 * defined in RFC4122. These UUID values are used to identify Trusted
 * Applications.
 */
struct TEEC_UUID {
	u32 timeLow;
	u16 timeMid;
	u16 timeHiAndVersion;
	u8 clockSeqAndNode[8];
};

static const struct TEEC_UUID ta_uuid = {
	0xc0fc39b1, 0xdd2e, 0x4753,
		{ 0xa6, 0x66, 0x00, 0xba, 0x98, 0x76, 0x69, 0xf6 }
};

typedef u32 TEEC_Result;

/**
 * Return values. Type is TEEC_Result
 *
 * TEEC_SUCCESS                 The operation was successful.
 * TEEC_ERROR_GENERIC           Non-specific cause.
 * TEEC_ERROR_ACCESS_DENIED     Access privileges are not sufficient.
 * TEEC_ERROR_CANCEL            The operation was canceled.
 * TEEC_ERROR_ACCESS_CONFLICT   Concurrent accesses caused conflict.
 * TEEC_ERROR_EXCESS_DATA       Too much data for the requested operation was
 *                              passed.
 * TEEC_ERROR_BAD_FORMAT        Input data was of invalid format.
 * TEEC_ERROR_BAD_PARAMETERS    Input parameters were invalid.
 * TEEC_ERROR_BAD_STATE         Operation is not valid in the current state.
 * TEEC_ERROR_ITEM_NOT_FOUND    The requested data item is not found.
 * TEEC_ERROR_NOT_IMPLEMENTED   The requested operation should exist but is not
 *                              yet implemented.
 * TEEC_ERROR_NOT_SUPPORTED     The requested operation is valid but is not
 *                              supported in this implementation.
 * TEEC_ERROR_NO_DATA           Expected data was missing.
 * TEEC_ERROR_OUT_OF_MEMORY     System ran out of resources.
 * TEEC_ERROR_BUSY              The system is busy working on something else.
 * TEEC_ERROR_COMMUNICATION     Communication with a remote party failed.
 * TEEC_ERROR_SECURITY          A security fault was detected.
 * TEEC_ERROR_SHORT_BUFFER      The supplied buffer is too short for the
 *                              generated output.
 * TEEC_ERROR_TARGET_DEAD       Trusted Application has panicked
 *                              during the operation.
 */

/**
 *  Standard defined error codes.
 */
#define TEEC_SUCCESS                0x00000000
#define TEEC_ERROR_GENERIC          0xFFFF0000
#define TEEC_ERROR_ACCESS_DENIED    0xFFFF0001
#define TEEC_ERROR_CANCEL           0xFFFF0002
#define TEEC_ERROR_ACCESS_CONFLICT  0xFFFF0003
#define TEEC_ERROR_EXCESS_DATA      0xFFFF0004
#define TEEC_ERROR_BAD_FORMAT       0xFFFF0005
#define TEEC_ERROR_BAD_PARAMETERS   0xFFFF0006
#define TEEC_ERROR_BAD_STATE        0xFFFF0007
#define TEEC_ERROR_ITEM_NOT_FOUND   0xFFFF0008
#define TEEC_ERROR_NOT_IMPLEMENTED  0xFFFF0009
#define TEEC_ERROR_NOT_SUPPORTED    0xFFFF000A
#define TEEC_ERROR_NO_DATA          0xFFFF000B
#define TEEC_ERROR_OUT_OF_MEMORY    0xFFFF000C
#define TEEC_ERROR_BUSY             0xFFFF000D
#define TEEC_ERROR_COMMUNICATION    0xFFFF000E
#define TEEC_ERROR_SECURITY         0xFFFF000F
#define TEEC_ERROR_SHORT_BUFFER     0xFFFF0010
#define TEEC_ERROR_EXTERNAL_CANCEL  0xFFFF0011
#define TEEC_ERROR_TARGET_DEAD      0xFFFF3024

#ifdef DLM_SUPPORT
ssize_t sk_dlm_token_data_read(struct file *filp, struct kobject *kobj,
				struct bin_attribute *bin_attr, char *buffer,
		loff_t pos, size_t count);
ssize_t sk_dlm_token_data_write(struct file *filp, struct kobject *kobj,
				struct bin_attribute *bin_attr, char *buffer,
				loff_t pos, size_t count);
static const struct bin_attribute amdtee_clint_data_attr = {
	.attr = {
		.name = "dlm_debug_token",
		.mode = 0664,
	},
	.size = MAX_DEBUG_TOKEN_SIZE,
	.write = sk_dlm_token_data_write,
	.read = sk_dlm_token_data_read,
};

TEEC_Result sk_dlm_get_dbg_token(struct debug_token *dt);
TEEC_Result sk_dlm_start_session(const struct tee_context *tee_ctx,
				const struct debug_token *dt,
				const struct TEEC_UUID *ta_uuid,
				uint32_t *dlm_sid);
TEEC_Result sk_dlm_fetch_dbg_string(const struct tee_context *tee_ctx,
					uint32_t dlm_sid);
TEEC_Result sk_dlm_stop_session(const struct tee_context *tee_ctx,
				uint32_t dlm_sid);
#endif

#endif
