/*
 *copyright 2014~2015 advanced micro devices, inc.
 *
 *permission is hereby granted, free of charge, to any person obtaining a
 *copy of this software and associated documentation files (the "software"),
 *to deal in the software without restriction, including without limitation
 *the rights to use, copy, modify, merge, publish, distribute, sublicense,
 *and/or sell copies of the software, and to permit persons to whom the
 *software is furnished to do so, subject to the following conditions:
 *
 *the above copyright notice and this permission notice shall be included in
 *all copies or substantial portions of the software.
 *
 *THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL
 *THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 *OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 *ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 *OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#ifndef _FSL_COMMON_H_
#define _FSL_COMMON_H_

#include "fsl_config.h"
#include "isp_fw_if/paramtypes.h"

typedef struct _mem_idx_value_t {
	/*if the type is INSTR_ARG_TYPE_MEM,
	 *this field indictor the the memory index type,
	 *only REG/IMM supported
	 */
	instr_arg_type_t mem_idx_type;
	int mem_idx;		/*M0 or M1 */
	union {
		int reg_idx;	/*the memory type is mn[rx] */
		int imm_value;	/*the memory type is mn[$imm] */
	} idx;			/*the memory index */
} mem_idx_value_t;

typedef struct _instr_arg_t {
	instr_arg_type_t type;
	union {
		int reg_index;	/*the arg type is REG */
		int imm_value;	/*the arg type is IMM */
		mem_idx_value_t mem_value;	/*the arg type is MEM */
		int label_instr_idx;	/*the arg type is LABEL */
		int func_idx;	/*the arg type is FUNC */
	} value;
} instr_arg_t;

typedef struct _instr_t {
	instr_op_t op;
	int arg_num;
	instr_arg_t args[MAX_ARG_NUM];
} instr_t;

typedef struct _func_map_t {
	char func_name[MAX_FUNC_NAME_LEN];
	/*the start instruction idx associated to this function */
	int start_idx;
	/*the end instruction idx associated to this function */
	int end_idx;
} func_map_t;

typedef struct _instr_seq_t {
	instr_t instrs[MAX_INSTRS];
	func_map_t funcs[MAX_FUNCS];
	int instr_num;
	int func_num;
} instr_seq_t;

#endif
