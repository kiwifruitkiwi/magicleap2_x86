/*
 * Copyright (C) 2019-2021 Advanced Micro Devices, Inc. All rights reserved.
 */

#ifndef _FSL_INTERINSTR_H_
#define _FSL_INTERINSTR_H_

#include "fsl_config.h"
#include "fsl_common.h"

enum _InterInstrOp_t {
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
};

enum _InterInstrArgType_t {
	INTER_INSTR_ARG_TYPE_INVALID,
	INTER_INSTR_ARG_TYPE_REG,
	INTER_INSTR_ARG_TYPE_MEM,
	INTER_INSTR_ARG_TYPE_IMM,
	INTER_INSTR_ARG_TYPE_LABEL,
	INTER_INSTR_ARG_TYPE_FUNC,
	INTER_INSTR_ARG_TYPE_MAX
};

/*
 * script                  context->instrx
 *---------------------   |----------------|         start_idx
 *|.function example1 |   |MOV R1, $12     |<-------|
 *|  MOV R1, $12      |   |MOV R2, $0      |        |
 *|  MOV R2, $0       |   |ADD R2, R2, R1  |<-------|----------------------
 *|LOOP1:             |   |SUB R1, R1, $1  |        |--context->funcs[0]      |
 *|  ADD R2, R2, R1   |   |JNZ R1, @LOOP1  |        |                         |
 *|  SUB R1, R1, $1   |   |CALL @example2  |<-------|end_idx                  |
 *|  JNZ R1, @LOOP1   |   |MOV R0, $0      |<-------|--context->func[1]       |
 *|  CALL @example2   |   |----------------|                                  |
 *|.endfunction       |                                                       |
 *|                   |    context->labels[0].label_name = LOOP1              |
 *|                   |    context->labels[0].index = 2-----------------------
 *|                   |    context->labels[0].func_idx = 0
 *|.function example2 |
 *|  MOV R0, $0       |
 *|.endfunction       |
 *---------------------
 */

struct _MemIdxValue_t {
	//If the type is INSTR_ARG_TYPE_MEM,
	//this field indictor the the memory index type,
	//only REG/IMM supported
	enum _InterInstrArgType_t memIdxType;
	int memIdx;		//M0 or M1
	union {
		int regIdx;	//The memory type is Mn[Rx]
		int immValue;	//The memory type is Mn[$imm]
	} idx;			//The memory index
};

struct _InterInstrArg_t {
	enum _InterInstrArgType_t type;
	union {
		int regIndex;	//The arg type is REG
		int immValue;	//The arg type is IMM
		struct _MemIdxValue_t memValue;	//The arg type is MEM
		int labelInstrIdx;	//The arg type is LABEL
		int funcIdx;	//The arg type is FUNC
	} value;

	char symbolName[MAX_LABEL_NAME_LEN];
};

struct _InterInstr_t {
	enum _InterInstrOp_t op;
	int argNum;
	struct _InterInstrArg_t args[MAX_ARG_NUM];
};

struct _InterFuncMap_t {
	char funcName[MAX_FUNC_NAME_LEN];
	int startIdx;	//The start instruction idx associated to this function
	int endIdx;	//The end instruction idx associated to this function
};

struct _LabelMap_t {
	char labelName[MAX_LABEL_NAME_LEN];
	int index;//The index to the instruction associated to this label
	int funcIdx;//The index to the function map associated to this label
};

struct _SvmParserContext_t {
	int maxInstrs;
	int maxLabels;
	int maxFuncs;
	struct _InterInstr_t instrs[MAX_INSTRS];
	struct _LabelMap_t labels[MAX_LABELS];
	struct _InterFuncMap_t funcs[MAX_FUNCS];

	int instrNum;
	int labelNum;
	int funcNum;

	int inFunc;	//Weather the current context is in a function!
	int currFuncIdx;//The current function idx

	char errInfo[MAX_ERROR_INFO_LEN];
	int errLine;

	char lineBuf[MAX_LINE_LEN];
	char wordsBuf[MAX_WORDS][MAX_WORD_LEN];
};

int InitInstrContext(struct _SvmParserContext_t *context,
		int maxInstrs, int maxLabels,
		int maxFuncs);
int AddInstrWi2c(struct _SvmParserContext_t *context,
		const char *arg[], int numArgs);
int AddInstrRi2c(struct _SvmParserContext_t *context,
		const char *arg[], int numArgs);
int AddInstrMov(struct _SvmParserContext_t *context,
		enum _InterInstrOp_t instrOp,
		const char *arg[], int numArgs);
int AddInstrJ(struct _SvmParserContext_t *context,
	enum _InterInstrOp_t instrOp,
	const char *arg[], int numArgs);
int AddInstrArg2(struct _SvmParserContext_t *context,
		enum _InterInstrOp_t instrOp,
		const char *arg[], int numArgs);
int AddInstrArg3(struct _SvmParserContext_t *context,
		enum _InterInstrOp_t instrOp,
		const char *arg[], int numArgs);
int AddLabel(struct _SvmParserContext_t *context, const char *label);
int AddFunc(struct _SvmParserContext_t *context, const char *functionName);
int EndFunc(struct _SvmParserContext_t *context);
int AddCall(struct _SvmParserContext_t *context,
			const char *arg[], int numArgs);

#endif
