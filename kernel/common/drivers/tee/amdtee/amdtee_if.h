/* SPDX-License-Identifier: MIT */

/*
 * Copyright 2019 Advanced Micro Devices, Inc.
 */

/*
 * This file has definitions related to Host and AMD-TEE Trusted OS interface.
 * These definitions must match the definitions on the TEE side.
 */

#ifndef AMDTEE_IF_H
#define AMDTEE_IF_H

#include <linux/types.h>

/*****************************************************************************
 ** TEE Param
 ******************************************************************************/
#define TEE_MAX_PARAMS		4

/**
 * struct memref - memory reference structure
 * @buf_id:    buffer ID of the buffer mapped by TEE_CMD_ID_MAP_SHARED_MEM
 * @offset:    offset in bytes from beginning of the buffer
 * @size:      data size in bytes
 */
struct memref {
	u32 buf_id;
	u32 offset;
	u32 size;
};

struct value {
	u32 a;
	u32 b;
};

/*
 * Parameters passed to open_session or invoke_command
 */
union tee_op_param {
	struct memref mref;
	struct value val;
};

struct tee_operation {
	u32 param_types;
	union tee_op_param params[TEE_MAX_PARAMS];
};

/* Must be same as in GP TEE specification */
#define TEE_OP_PARAM_TYPE_NONE                  0
#define TEE_OP_PARAM_TYPE_VALUE_INPUT           1
#define TEE_OP_PARAM_TYPE_VALUE_OUTPUT          2
#define TEE_OP_PARAM_TYPE_VALUE_INOUT           3
#define TEE_OP_PARAM_TYPE_INVALID               4
#define TEE_OP_PARAM_TYPE_MEMREF_INPUT          5
#define TEE_OP_PARAM_TYPE_MEMREF_OUTPUT         6
#define TEE_OP_PARAM_TYPE_MEMREF_INOUT          7

#define TEE_PARAM_TYPE_GET(t, i)        (((t) >> ((i) * 4)) & 0xF)
#define TEE_PARAM_TYPES(t0, t1, t2, t3) \
	((t0) | ((t1) << 4) | ((t2) << 8) | ((t3) << 12))

/*****************************************************************************
 ** TEE Commands
 *****************************************************************************/

/*
 * The shared memory between rich world and secure world may be physically
 * non-contiguous. Below structures are meant to describe a shared memory region
 * via scatter/gather (sg) list
 */

/**
 * struct tee_sg_desc - sg descriptor for a physically contiguous buffer
 * @low_addr: [in] bits[31:0] of buffer's physical address. Must be 4KB aligned
 * @hi_addr:  [in] bits[63:32] of the buffer's physical address
 * @size:     [in] size in bytes (must be multiple of 4KB)
 */
struct tee_sg_desc {
	u32 low_addr;
	u32 hi_addr;
	u32 size;
};

/**
 * struct tee_sg_list - structure describing a scatter/gather list
 * @count:   [in] number of sg descriptors
 * @size:    [in] total size of all buffers in the list. Must be multiple of 4KB
 * @buf:     [in] list of sg buffer descriptors
 */
#define TEE_MAX_SG_DESC 64
struct tee_sg_list {
	u32 count;
	u32 size;
	struct tee_sg_desc buf[TEE_MAX_SG_DESC];
};

/**
 * struct tee_cmd_map_shared_mem - command to map shared memory
 * @buf_id:    [out] return buffer ID value
 * @sg_list:   [in] list describing memory to be mapped
 */
struct tee_cmd_map_shared_mem {
	u32 buf_id;
	struct tee_sg_list sg_list;
};

/**
 * struct tee_cmd_unmap_shared_mem - command to unmap shared memory
 * @buf_id:    [in] buffer ID of memory to be unmapped
 */
struct tee_cmd_unmap_shared_mem {
	u32 buf_id;
};

/**
 * struct tee_cmd_load_ta - load Trusted Application (TA) binary into TEE
 * @low_addr:    [in] bits [31:0] of the physical address of the TA binary
 * @hi_addr:     [in] bits [63:32] of the physical address of the TA binary
 * @size:        [in] size of TA binary in bytes
 * @ta_handle:   [out] return handle of the loaded TA
 */
struct tee_cmd_load_ta {
	u32 low_addr;
	u32 hi_addr;
	u32 size;
	u32 ta_handle;
};

/**
 * struct tee_cmd_unload_ta - command to unload TA binary from TEE environment
 * @ta_handle:    [in] handle of the loaded TA to be unloaded
 */
struct tee_cmd_unload_ta {
	u32 ta_handle;
};

/**
 * struct tee_cmd_open_session - command to call TA_OpenSessionEntryPoint in TA
 * @ta_handle:      [in] handle of the loaded TA
 * @session_info:   [out] pointer to TA allocated session data
 * @op:             [in/out] operation parameters
 * @return_origin:  [out] origin of return code after TEE processing
 */
struct tee_cmd_open_session {
	u32 ta_handle;
	u32 session_info;
	struct tee_operation op;
	u32 return_origin;
};

/**
 * struct tee_cmd_close_session - command to call TA_CloseSessionEntryPoint()
 *                                in TA
 * @ta_handle:      [in] handle of the loaded TA
 * @session_info:   [in] pointer to TA allocated session data
 */
struct tee_cmd_close_session {
	u32 ta_handle;
	u32 session_info;
};

/**
 * struct tee_cmd_invoke_cmd - command to call TA_InvokeCommandEntryPoint() in
 *                             TA
 * @ta_handle:     [in] handle of the loaded TA
 * @cmd_id:        [in] TA command ID
 * @session_info:  [in] pointer to TA allocated session data
 * @op:            [in/out] operation parameters
 * @return_origin: [out] origin of return code after TEE processing
 */
struct tee_cmd_invoke_cmd {
	u32 ta_handle;
	u32 cmd_id;
	u32 session_info;
	struct tee_operation op;
	u32 return_origin;
};
/*****************************************************************************
 ** DLM specific commands
 ******************************************************************************/
#define TEE_DLM_TA_UUID_SIZE 0x10
#define TEE_DLM_TOKEN_SIZE   320
#define DLM_STRING_SIZE      256
#define DLM_NUM_STRINGS      127    /* must be multiple of 16 minus 1 */
#define MAX_DEBUG_TOKEN_SIZE 4096
/*
 * Get debug Token
 * Command to get debug token from TEE OS. This token is built by
 * the OS and returned to the host client. Then client signs the token
 * using the same key that was used for signing TA and sends the token
 * as part of command to attach DLM buffer to the TA
 */
struct tee_cmd_get_dbg_token {
	u8 token[TEE_DLM_TOKEN_SIZE];   /* [out] debug token buffer */
};

/*
 * Start TA debugging
 * Command to start TA debug session.
 * Host driver sends signed debug token and DLM (Debug Log Message) buffer
 * to TEE OS. If token validation passes, the debug messages from specified
 * TA will be written to DLM buffer by TEE OS.
 */
struct tee_cmd_start_ta_dbg {
	u8  ta_uuid[TEE_DLM_TA_UUID_SIZE];
	u32 low_addr;
	u32 hi_addr;
	u32 size;
	u32 sid;
};

/* Stop TA debugging */
struct tee_cmd_stop_ta_dbg {
	u32 sid;
};

struct amdtee_dlm_string {
	char string[DLM_STRING_SIZE]; /* DLM string of maximum size */
};

/* DLM buffer */
struct amdtee_dlm_buf {
	u32 dlm_size;       /* Total size of DLM buffer in bytes */
	u32 num_strings;    /* Total no. of entries in dlm_string array */
	u32 rptr;       /* Read pointer (index of the dlm_string array) */
	u32 wptr;       /* Write pointer (index of dlm_string array) */
	u8 reserved[240];   /* Reserved, must be 0 */
	/*
	 * Array where DLM strings are written by OS. Although it is defined
	 * with fixed size, it can be set to bigger or smaller value by the host
	 * driver. The actual size is indicated in num_strings and dlm_size.
	 */
	struct amdtee_dlm_string dlm_string[DLM_NUM_STRINGS];

	/* Signed debug token buffer (padded with 0s) */
	u8 debug_token[MAX_DEBUG_TOKEN_SIZE];

	/*
	 * Total size (as defined) is 36 KB. If host driver needs smaller or
	 * bigger buffer, the size must be made multiple of 4 KB (the
	 * DLM_NUM_STRINGS must be multiple of 16 minus 1; i.e. 15, 31,
	 * 47, 63, 79, 127, 159, 255 etc. ).
	 */
};

static_assert((sizeof(struct amdtee_dlm_buf) & (PAGE_SIZE - 1)) == 0,
	      "size of amdtee_dlm_buf must be multiple of 4K");

#endif /*AMDTEE_IF_H*/
