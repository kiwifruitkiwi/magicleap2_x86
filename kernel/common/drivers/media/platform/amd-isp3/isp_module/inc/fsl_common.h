/*
 * Copyright (C) 2019-2020 Advanced Micro Devices, Inc. All rights reserved.
 */

#ifndef _FSL_COMMON_H_
#define _FSL_COMMON_H_

#include "fsl_config.h"
#include "isp_fw_if/paramtypes.h"

struct _mem_idx_value_t {
	/*if the type is INSTR_ARG_TYPE_MEM,
	 *this field indictor the the memory index type,
	 *only REG/IMM supported
	 */
	enum _instr_arg_type_t mem_idx_type;
	int mem_idx;		/*M0 or M1 */
	union {
		int reg_idx;	/*the memory type is mn[rx] */
		int imm_value;	/*the memory type is mn[$imm] */
	} idx;			/*the memory index */
};

struct _instr_arg_t {
	enum _instr_arg_type_t type;
	union {
		int reg_index;	/*the arg type is REG */
		int imm_value;	/*the arg type is IMM */
		struct _mem_idx_value_t mem_value;	/*the arg type is MEM */
		int label_instr_idx;	/*the arg type is LABEL */
		int func_idx;	/*the arg type is FUNC */
	} value;
};

struct _instr_t {
	enum _instr_op_t op;
	int arg_num;
	struct _instr_arg_t args[MAX_ARG_NUM];
};

struct _func_map_t {
	char func_name[MAX_FUNC_NAME_LEN];
	/*the start instruction idx associated to this function */
	int start_idx;
	/*the end instruction idx associated to this function */
	int end_idx;
};

struct _instr_seq_t {
	struct _instr_t instrs[MAX_INSTRS];
	struct _func_map_t funcs[MAX_FUNCS];
	int instr_num;
	int func_num;
};

#endif
