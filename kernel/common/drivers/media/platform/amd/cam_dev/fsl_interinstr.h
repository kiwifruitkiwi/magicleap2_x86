/*
 * copyright 2014~2015 advanced micro devices, inc.
 *
 * permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "software"),
 * to deal in the software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the software, and to permit persons to whom the
 * software is furnished to do so, subject to the following conditions:
 *
 * the above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#ifndef _FSL_INTERINSTR_H_
#define _FSL_INTERINSTR_H_

#include "fsl_config.h"
#include "fsl_common.h"

typedef enum _InterInstrOp_t {
	INTER_INSTR_OP_INVALID,
	INTER_INSTR_OP_ADD,
	INTER_INSTR_OP_SUB,
	INTER_INSTR_OP_MUL,
	INTER_INSTR_OP_DIV,
	INTER_INSTR_OP_SHL,
	INTER_INSTR_OP_SHR,
	INTER_INSTR_OP_AND,
	INTER_INSTR_OP_OR,
	INTER_INSTR_OP_XOR,
	INTER_INSTR_OP_MOVB,
	INTER_INSTR_OP_MOVW,
	INTER_INSTR_OP_MOVDW,
	INTER_INSTR_OP_JNZ,
	INTER_INSTR_OP_JEZ,
	INTER_INSTR_OP_JLZ,
	INTER_INSTR_OP_JGZ,
	INTER_INSTR_OP_JGEZ,
	INTER_INSTR_OP_JLEZ,
	INTER_INSTR_OP_CALL,
	INTER_INSTR_OP_WI2C,
	INTER_INSTR_OP_RI2C,
	INTER_INSTR_OP_MAX
} InterInstrOp_t;

typedef enum _InterInstrArgType_t {
	INTER_INSTR_ARG_TYPE_INVALID,
	INTER_INSTR_ARG_TYPE_REG,
	INTER_INSTR_ARG_TYPE_MEM,
	INTER_INSTR_ARG_TYPE_IMM,
	INTER_INSTR_ARG_TYPE_LABEL,
	INTER_INSTR_ARG_TYPE_FUNC,
	INTER_INSTR_ARG_TYPE_MAX
} InterInstrArgType_t;

typedef struct _label_map_t {
	char label_name[MAX_LABEL_NAME_LEN];
	//the index to the instruction associated to this label
	int index;
	//the index to the function map associated to this label
	int func_idx;
} label_map_t;

/*
 * script                  context->instrx
 *---------------------   |----------------|         start_idx
 *|.function example1 |   |MOV R1, $12     |<-------|
 *|  MOV R1, $12      |   |MOV R2, $0      |        |
 *|  MOV R2, $0       |   |ADD R2, R2, R1  |<-------|--------------------------
 *|LOOP1:             |   |SUB R1, R1, $1  |        |--context->funcs[0]      |
 *|  ADD R2, R2, R1   |   |JNZ R1, @LOOP1  |        |                         |
 *|  SUB R1, R1, $1   |   |CALL @example2  |<-------|end_idx                  |
 *|  JNZ R1, @LOOP1   |   |MOV R0, $0      |<-------|--context->func[1]       |
 *|  CALL @example2   |   |----------------|                                  |
 *|.endfunction       |                                                       |
 *|                   |    context->labels[0].label_name = LOOP1              |
 *|                   |    context->labels[0].index = 2------------------------
 *|                   |    context->labels[0].func_idx = 0
 *|.function example2 |
 *|  MOV R0, $0       |
 *|.endfunction       |
 *---------------------
 */
typedef struct _instr_arg_ext_t {
	instr_arg_t instr_arg;
	char symbol_name[MAX_LABEL_NAME_LEN];
} instr_arg_ext_t;

typedef struct _instr_ext_t {
	instr_op_t op;
	int arg_num;
	instr_arg_ext_t args[MAX_ARG_NUM];
} instr_ext_t;

typedef struct _parser_context_t {
	int max_instrs;
	int max_labels;
	int max_funcs;
	instr_ext_t instrs[MAX_INSTRS];
	label_map_t labels[MAX_LABELS];
	func_map_t funcs[MAX_FUNCS];

	int instr_num;
	int label_num;
	int func_num;

	int in_func;
	int curr_func_idx;

	char err_info[MAX_ERROR_INFO_LEN];
	int err_line;

	char line_buf[MAX_LINE_LEN];
	char words_buf[MAX_WORDS][MAX_WORD_LEN];
} parser_context_t;

int init_instr_context(parser_context_t *context, int max_instrs,
		int max_labels, int max_funcs);
int add_instr_wi2c(parser_context_t *context, const char *arg[], int num_args);
int add_instr_ri2c(parser_context_t *context, const char *arg[], int num_args);
int add_instr_mov(parser_context_t *context, instr_op_t instr_op,
		const char *arg[], int num_args);
int add_instr_j(parser_context_t *context, instr_op_t instr_op,
		const char *arg[], int num_args);
int add_instr_arg2(parser_context_t *context, instr_op_t instr_op,
		const char *arg[], int num_args);
int add_instr_arg3(parser_context_t *context, instr_op_t instr_op,
		const char *arg[], int num_args);
int add_label(parser_context_t *context, const char *label);
int add_func(parser_context_t *context, const char *function_name);
int end_func(parser_context_t *context);
int add_call(parser_context_t *context, const char *arg[], int num_args);

/*added by bdu*/
typedef struct _MemIdxValue_t {
	//If the type is INSTR_ARG_TYPE_MEM,
	//this field indictor the the memory index type,
	//only REG/IMM supported
	InterInstrArgType_t memIdxType;
	int memIdx;		//M0 or M1
	union {
		int regIdx;	//The memory type is Mn[Rx]
		int immValue;	//The memory type is Mn[$imm]
	} idx;			//The memory index
} MemIdxValue_t;

typedef struct _InterInstrArg_t {
	InterInstrArgType_t type;
	union {
		int regIndex;	//The arg type is REG
		int immValue;	//The arg type is IMM
		MemIdxValue_t memValue;	//The arg type is MEM
		int labelInstrIdx;	//The arg type is LABEL
		int funcIdx;	//The arg type is FUNC
	} value;

	char symbolName[MAX_LABEL_NAME_LEN];
} InterInstrArg_t;

typedef struct _InterInstr_t {
	InterInstrOp_t op;
	int argNum;
	InterInstrArg_t args[MAX_ARG_NUM];
} InterInstr_t;

typedef struct _InterFuncMap_t {
	char funcName[MAX_FUNC_NAME_LEN];
	int startIdx;	//The start instruction idx associated to this function
	int endIdx;	//The end instruction idx associated to this function
} InterFuncMap_t;

typedef struct _LabelMap_t {
	char labelName[MAX_LABEL_NAME_LEN];
	int index;//The index to the instruction associated to this label
	int funcIdx;//The index to the function map associated to this label
} LabelMap_t;

typedef struct _SvmParserContext_t {
	int maxInstrs;
	int maxLabels;
	int maxFuncs;
	InterInstr_t instrs[MAX_INSTRS];
	LabelMap_t labels[MAX_LABELS];
	InterFuncMap_t funcs[MAX_FUNCS];

	int instrNum;
	int labelNum;
	int funcNum;

	int inFunc;	//Weather the current context is in a function!
	int currFuncIdx;//The current function idx

	char errInfo[MAX_ERROR_INFO_LEN];
	int errLine;

	char lineBuf[MAX_LINE_LEN];
	char wordsBuf[MAX_WORDS][MAX_WORD_LEN];
} SvmParserContext_t;

int InitInstrContext(SvmParserContext_t *context, int maxInstrs, int maxLabels,
		int maxFuncs);
int AddInstrWi2c(SvmParserContext_t *context, const char *arg[], int numArgs);
int AddInstrRi2c(SvmParserContext_t *context, const char *arg[], int numArgs);
int AddInstrMov(SvmParserContext_t *context, InterInstrOp_t instrOp,
		const char *arg[], int numArgs);
int AddInstrJ(SvmParserContext_t *context, InterInstrOp_t instrOp,
	const char *arg[], int numArgs);
int AddInstrArg2(SvmParserContext_t *context, InterInstrOp_t instrOp,
		const char *arg[], int numArgs);
int AddInstrArg3(SvmParserContext_t *context, InterInstrOp_t instrOp,
		const char *arg[], int numArgs);
int AddLabel(SvmParserContext_t *context, const char *label);
int AddFunc(SvmParserContext_t *context, const char *functionName);
int EndFunc(SvmParserContext_t *context);
int AddCall(SvmParserContext_t *context, const char *arg[], int numArgs);

#endif
