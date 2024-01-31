/*
 * Copyright (C) 2019-2021 Advanced Micro Devices, Inc. All rights reserved.
 */

#include "sensor_if.h"
#include "fsl_error.h"
#include "fsl_interinstr.h"
#include "fsl_string.h"
#include "fsl_log.h"

int InitInstrContext(struct _SvmParserContext_t *context,
			int maxInstrs, int maxLabels,
			int maxFuncs)
{
	if (!context) {
		SetErrorInfo(context, "instruction context is NULL\n");
		return -1;
	}

	memset(context, 0, sizeof(struct _SvmParserContext_t));
	context->maxInstrs = maxInstrs;
	context->maxLabels = maxLabels;
	context->maxFuncs = maxFuncs;
	context->instrNum = 0;
	context->labelNum = 0;
	context->funcNum = 0;
	context->inFunc = 0;
	context->currFuncIdx = 0;
	return 0;
}

static int ParseArg(struct _SvmParserContext_t *context, const char *arg,
			struct _InterInstrArg_t *instrArg)
{
	int len;
	char indicator;
	const char *value;

	len = (int)strlen(arg);
	if (len < 2) {
		SetErrorInfo(context,
		"Arg length at least 2 characters start with R, $, @\n");
		return -1;
	}

	indicator = arg[0];
	value = arg + 1;

	if (indicator == 'R') {
		int tolret;
		long regIndex;

		//It is an register arg
		if (!IsDigital(value, 0, NULL)) {
			SetErrorInfo(context,
		"Arg after with R is not a digital string, from 0 to %d\n",
				(MAX_REGISTERS - 1));
			return -1;
		}

		//regIndex = simple_strtol(value, NULL, 10);
		tolret = kstrtol(value, 10, &regIndex);
		if ((regIndex < 0) || (regIndex > (MAX_REGISTERS - 1))) {
			SetErrorInfo(context,
				"Register %s is not supported,range R0~R%d\n",
				arg, (MAX_REGISTERS - 1));
			return -1;
		}

		instrArg->type = INTER_INSTR_ARG_TYPE_REG;
		instrArg->value.regIndex = regIndex;
	} else if (indicator == '$') {
		int tolret;
		long immValue;
		int isHex;
		//It is an immediator number

		if (!IsDigital(value, 1, &isHex)) {
			SetErrorInfo(context,
				"Arg after with $ is not a digital string,");
			SetErrorInfo(context,
				"a dec value or hex value start with 0x/0X\n");
			return -1;
		}

//		if (isHex)
//			immValue = simple_strtol(value, NULL, 16);
//		else
//			immValue = simple_strtol(value, NULL, 10);
		if (isHex)
			tolret = kstrtol(value, 16, &immValue);
		else
			tolret = kstrtol(value, 10, &immValue);

		instrArg->type = INTER_INSTR_ARG_TYPE_IMM;
		instrArg->value.immValue = immValue;
	} else if (indicator == '@') {
		int labelLen;

		//It is an label
		labelLen = (int)strlen(value);
		if (labelLen >= MAX_LABEL_NAME_LEN) {
			SetErrorInfo(context,
				"Unsupported label characters large than:%d\n",
				MAX_LABEL_NAME_LEN);
			return -1;
		}

		instrArg->type = INTER_INSTR_ARG_TYPE_LABEL;
		strncpy(instrArg->symbolName, value, MAX_LABEL_NAME_LEN);
		instrArg->symbolName[labelLen] = 0;
	} else if (indicator == 'M') {
		int valueLen;
		int memIdx;
		char memContentIdx[MAX_WORD_LEN];
		char memIndicator;
		const char *memValue;

		//It is a memory
		valueLen = (int)strlen(value);
		if (valueLen < 5) {
			SetErrorInfo(context,
			"Memory type error, should be:Mx[Ry] or Mx[imm]\n");
			return -1;
		}

		if ((value[1] != '[') || (value[valueLen - 1] != ']')) {
			SetErrorInfo(context,
			"Memory type error, should be:Mx[Ry] or Mx[imm]\n");
			return -1;
		}

		if (value[0] == '0')
			memIdx = 0;
		else if (value[0] == '1')
			memIdx = 1;
		else {
			SetErrorInfo(context, "Only M0 and M1 are supported\n");
			return -1;
		}

		if (valueLen > MAX_WORD_LEN) {
			SetErrorInfo(context,
				"Too long size for Memory type\n");
			return -1;
		}
		strncpy(memContentIdx, value + 2, MAX_WORD_LEN);
		valueLen = (int)strlen(memContentIdx);
		memContentIdx[valueLen - 1] = 0; //Remove the ']' character.

		memIndicator = memContentIdx[0];
		memValue = &memContentIdx[1];

		if (memIndicator == 'R') {
			int tolret;
			long regIndex;

			//the idx is a register
			if (!IsDigital(memValue, 0, NULL)) {
				SetErrorInfo(context,
		"Arg after with R is not a digital string, from 0 to %d\n",
					(MAX_REGISTERS - 1));
				return -1;
			}
			//regIndex = simple_strtol(value, NULL, 10);
			tolret = kstrtol(value, 10, &regIndex);
			if ((regIndex < 0) ||
				(regIndex > (MAX_REGISTERS - 1))) {
				SetErrorInfo(context,
			"Register %s is not supported, range R0~R%d\n",
					arg, (MAX_REGISTERS - 1));
				return -1;
			}
			instrArg->type = INTER_INSTR_ARG_TYPE_MEM;
			instrArg->value.memValue.memIdxType =
				INTER_INSTR_ARG_TYPE_REG;
			instrArg->value.memValue.memIdx = memIdx;
			instrArg->value.memValue.idx.regIdx = regIndex;
		} else if (memIndicator == '$') {
			int tolret;
			long immValue;
			int isHex;
			//It is an immediator number

			if (!IsDigital(memValue, 1, &isHex)) {
				SetErrorInfo(context,
				"Arg after with $ is not a digital string,");
				SetErrorInfo(context,
				"a dec value or hex value start with 0x/0X\n");
				return -1;
			}

//			if (isHex)
//				immValue = simple_strtol(memValue, NULL, 16);
//			else
//				immValue = simple_strtol(memValue, NULL, 10);
			if (isHex)
				tolret = kstrtol(memValue, 16, &immValue);
			else
				tolret = kstrtol(memValue, 10, &immValue);

			instrArg->type = INTER_INSTR_ARG_TYPE_MEM;
			instrArg->value.memValue.memIdxType =
				INTER_INSTR_ARG_TYPE_IMM;
			instrArg->value.memValue.memIdx = memIdx;
			instrArg->value.memValue.idx.immValue = immValue;
		} else {
			SetErrorInfo(context,
				"The Memory index is not start with R, $\n");
			return -1;
		}
	} else {
		SetErrorInfo(context, "The arg is not start with R, $, @\n");
		return -1;
	}
	return 0;
}

static int ParseCallArg(struct _SvmParserContext_t *context,
			const char *arg, struct _InterInstrArg_t *instrArg)
{
	int len;
	char indicator;
	const char *value;

	len = (int)strlen(arg);
	if (len < 2) {
		SetErrorInfo(context,
	"Arg length for Call instruction at least 2 characters start @\n");
		return -1;
	}

	indicator = arg[0];
	value = arg + 1;

	if (indicator == '@') {
		int labelLen;

		//It is an label

		labelLen = (int)strlen(value);
		if (labelLen >= MAX_FUNC_NAME_LEN) {
			SetErrorInfo(context,
			"Unsupported function characters large than:%d\n",
				MAX_FUNC_NAME_LEN);
			return -1;
		}

		instrArg->type = INTER_INSTR_ARG_TYPE_FUNC;
		strncpy(instrArg->symbolName, value, MAX_LABEL_NAME_LEN);
		instrArg->symbolName[labelLen] = 0;
	} else {
		SetErrorInfo(context,
			"The arg for Call instruction is not start with @\n");
		return -1;
	}
	return 0;
}

int AddInstrWi2c(struct _SvmParserContext_t *context,
			const char *arg[], int numArgs)
{
	int i;
	struct _InterInstr_t instr;
	struct _InterInstrArg_t instrArg;

	if (!context->inFunc) {
		SetErrorInfo(context, "The instruction is not in a funmctin\n");
		return -1;
	}

	if (numArgs != 3) {
		SetErrorInfo(context, "The args number is not 3\n");
		return -1;
	}

	instr.op = INTER_INSTR_OP_WI2C;
	instr.argNum = numArgs;
	for (i = 0; i < numArgs; i++) {
		if (ParseArg(context, arg[i], &instrArg) != 0)
			return -1;
		instr.args[i] = instrArg;
	}

	if ((instr.args[0].type != INTER_INSTR_ARG_TYPE_REG) &&
		(instr.args[0].type != INTER_INSTR_ARG_TYPE_IMM)) {
		SetErrorInfo(context,
			"The first arg is not a register,");
		SetErrorInfo(context,
			"or a immediate value for WI2C instruction\n");
		return -1;
	}

	if ((instr.args[2].type != INTER_INSTR_ARG_TYPE_REG) &&
		(instr.args[2].type != INTER_INSTR_ARG_TYPE_IMM)) {
		SetErrorInfo(context,
		"The length arg of WI2C must be a reg or a immediate value\n");
		return -1;
	}

	if (context->instrNum >= context->maxInstrs) {
		SetErrorInfo(context, "Too many instruction, max:%d\n",
			context->maxInstrs);
		return -1;
	}

	context->instrs[context->instrNum++] = instr;
	return 0;
}

int AddInstrRi2c(struct _SvmParserContext_t *context,
			const char *arg[], int numArgs)
{
	int i;
	struct _InterInstr_t instr;
	struct _InterInstrArg_t instrArg;

	if (!context->inFunc) {
		SetErrorInfo(context, "The instruction is not in a funmctin\n");
		return -1;
	}

	if (numArgs != 3) {
		SetErrorInfo(context, "The args number is not 3\n");
		return -1;
	}

	instr.op = INTER_INSTR_OP_RI2C;
	instr.argNum = numArgs;
	for (i = 0; i < numArgs; i++) {
		if (ParseArg(context, arg[i], &instrArg) != 0)
			return -1;
		instr.args[i] = instrArg;
	}

	if ((instr.args[0].type != INTER_INSTR_ARG_TYPE_REG) &&
		(instr.args[0].type != INTER_INSTR_ARG_TYPE_MEM)) {
		SetErrorInfo(context,
	"The first arg is not a register or a memory for RI2C instruction\n");
		return -1;
	}

	if (context->instrNum >= context->maxInstrs) {
		SetErrorInfo(context, "Too many instruction, max:%d\n",
			context->maxInstrs);
		return -1;
	}

	context->instrs[context->instrNum++] = instr;
	return 0;
}

int AddInstrArg3(struct _SvmParserContext_t *context,
		enum _InterInstrOp_t instrOp,
		 const char *arg[], int numArgs)
{
	int i;
	struct _InterInstr_t instr;
	struct _InterInstrArg_t instrArg;

	if (!context->inFunc) {
		SetErrorInfo(context, "The instruction is not in a function\n");
		return -1;
	}

	if (numArgs != 3) {
		SetErrorInfo(context, "The args number is not 3\n");
		return -1;
	}

	instr.op = instrOp;
	instr.argNum = numArgs;
	for (i = 0; i < numArgs; i++) {
		if (ParseArg(context, arg[i], &instrArg) != 0)
			return -1;
		instr.args[i] = instrArg;
	}

	if (instr.args[0].type != INTER_INSTR_ARG_TYPE_REG) {
		SetErrorInfo(context, "The first arg is not a register\n");
		return -1;
	}

	if ((instr.args[1].type != INTER_INSTR_ARG_TYPE_REG) &&
		(instr.args[1].type != INTER_INSTR_ARG_TYPE_IMM)) {
		SetErrorInfo(context,
			"The second arg must be a reg or an imm\n");
		return -1;
	}

	if ((instr.args[2].type != INTER_INSTR_ARG_TYPE_REG) &&
		(instr.args[2].type != INTER_INSTR_ARG_TYPE_IMM)) {
		SetErrorInfo(context,
			"The third arg must be a reg or an imm\n");
		return -1;
	}

	if (context->instrNum >= context->maxInstrs) {
		SetErrorInfo(context, "Too many instruction, max:%d\n",
			context->maxInstrs);
		return -1;
	}

	context->instrs[context->instrNum++] = instr;
	return 0;
}

int AddInstrArg2(struct _SvmParserContext_t *context,
		enum _InterInstrOp_t instrOp, const char *arg[], int numArgs)
{
	int i;
	struct _InterInstr_t instr;
	struct _InterInstrArg_t instrArg;

	if (!context->inFunc) {
		SetErrorInfo(context, "The instruction is not in a function\n");
		return -1;
	}

	if (numArgs != 2) {
		SetErrorInfo(context, "The args number is not 2\n");
		return -1;
	}

	instr.op = instrOp;
	instr.argNum = numArgs;
	for (i = 0; i < numArgs; i++) {
		if (ParseArg(context, arg[i], &instrArg) != 0)
			return -1;
		instr.args[i] = instrArg;
	}

	if (instr.args[0].type != INTER_INSTR_ARG_TYPE_REG) {
		SetErrorInfo(context, "The first arg is not a register\n");
		return -1;
	}

	if ((instr.args[1].type != INTER_INSTR_ARG_TYPE_REG) &&
		(instr.args[1].type != INTER_INSTR_ARG_TYPE_IMM)) {
		SetErrorInfo(context,
			"The second arg is not a register or an imm\n");
		return -1;
	}

	if (context->instrNum >= context->maxInstrs) {
		SetErrorInfo(context, "Too many instruction, max:%d\n",
			context->maxInstrs);
		return -1;
	}

	context->instrs[context->instrNum++] = instr;
	return 0;
}

int AddInstrMov(struct _SvmParserContext_t *context,
		enum _InterInstrOp_t instrOp, const char *arg[], int numArgs)
{
	int i;
	struct _InterInstr_t instr;
	struct _InterInstrArg_t instrArg;

	if (!context->inFunc) {
		SetErrorInfo(context, "The instruction is not in a function\n");
		return -1;
	}

	if (numArgs != 2) {
		SetErrorInfo(context, "The args number is not 2\n");
		return -1;
	}

	instr.op = instrOp;
	instr.argNum = numArgs;
	for (i = 0; i < numArgs; i++) {
		if (ParseArg(context, arg[i], &instrArg) != 0)
			return -1;
		instr.args[i] = instrArg;
	}

	if ((instr.args[0].type != INTER_INSTR_ARG_TYPE_REG) &&
		(instr.args[0].type != INTER_INSTR_ARG_TYPE_MEM)) {
		SetErrorInfo(context,
			"The first arg is not a register or a memory\n");
		return -1;
	}

	if ((instr.args[1].type != INTER_INSTR_ARG_TYPE_REG) &&
		(instr.args[1].type != INTER_INSTR_ARG_TYPE_MEM) &&
		(instr.args[1].type != INTER_INSTR_ARG_TYPE_IMM)) {
		SetErrorInfo(context,
			"The second arg must be reg, mem or imm\n");
		return -1;
	}

	if (context->instrNum >= context->maxInstrs) {
		SetErrorInfo(context, "Too many instruction, max:%d\n",
			context->maxInstrs);
		return -1;
	}

	context->instrs[context->instrNum++] = instr;
	return 0;
}

int AddInstrJ(struct _SvmParserContext_t *context,
		enum _InterInstrOp_t instrOp, const char *arg[], int numArgs)
{
	int i;
	struct _InterInstr_t instr;
	struct _InterInstrArg_t instrArg;

	if (!context->inFunc) {
		SetErrorInfo(context, "The instruction is not in a function\n");
		return -1;
	}

	if (numArgs != 2) {
		SetErrorInfo(context, "The args number is not 2\n");
		return -1;
	}

	instr.op = instrOp;
	instr.argNum = numArgs;
	for (i = 0; i < numArgs; i++) {
		if (ParseArg(context, arg[i], &instrArg) != 0)
			return -1;
		instr.args[i] = instrArg;
	}

	if ((instr.args[0].type != INTER_INSTR_ARG_TYPE_REG) &&
		(instr.args[0].type != INTER_INSTR_ARG_TYPE_IMM)) {
		SetErrorInfo(context,
			"The first arg is not a register or an imm\n");
		return -1;
	}

	if (instr.args[1].type != INTER_INSTR_ARG_TYPE_LABEL) {
		SetErrorInfo(context, "The second arg must be a label\n");
		return -1;
	}

	if (context->instrNum >= context->maxInstrs) {
		SetErrorInfo(context, "Too many instruction, max:%d\n",
			context->maxInstrs);
		return -1;
	}

	context->instrs[context->instrNum++] = instr;
	return 0;
}

int AddCall(struct _SvmParserContext_t *context,
				const char *arg[], int numArgs)
{
	struct _InterInstr_t instr;
	struct _InterInstrArg_t instrArg;

	if (!context->inFunc) {
		SetErrorInfo(context, "The instruction is not in a function\n");
		return -1;
	}

	if (numArgs != 1) {
		SetErrorInfo(context, "The args number is not 2\n");
		return -1;
	}

	instr.op = INTER_INSTR_OP_CALL;
	instr.argNum = 1;
	if (ParseCallArg(context, arg[0], &instrArg) != 0)
		return -1;
	instr.args[0] = instrArg;

	if (context->instrNum >= context->maxInstrs) {
		SetErrorInfo(context, "Too many instruction, max:%d\n",
			context->maxInstrs);
		return -1;
	}

	context->instrs[context->instrNum++] = instr;
	return 0;
}

int AddLabel(struct _SvmParserContext_t *context, const char *label)
{
	int len;

	if (!context->inFunc) {
		SetErrorInfo(context, "The label is not in a function\n");
		return -1;
	}

	if (context->labelNum >= context->maxLabels) {
		SetErrorInfo(context, "Too many labels, max:%d\n",
			context->maxLabels);
		return -1;
	}

	len = (int)strlen(label);

	context->labels[context->labelNum].index = context->instrNum;
	strncpy(context->labels[context->labelNum].labelName, label,
		MAX_LABEL_NAME_LEN);

	context->labels[context->labelNum].labelName[len - 1] = 0;
	context->labels[context->labelNum].funcIdx = context->currFuncIdx;
	context->labelNum++;
	return 0;
}

int AddFunc(struct _SvmParserContext_t *context, const char *functionName)
{
	if (context->inFunc) {
		//Already in function, function nested is not supported
		SetErrorInfo(context, "Function nested is not supported:%s\n",
			functionName);
		return -1;
	}

	if (context->funcNum >= context->maxFuncs) {
		SetErrorInfo(context, "Too many functions, max:%d\n",
			context->maxFuncs);
		return -1;
	}

	context->currFuncIdx = context->funcNum;

	context->funcs[context->currFuncIdx].startIdx = context->instrNum;
	strncpy(context->funcs[context->currFuncIdx].funcName,
		functionName, MAX_FUNC_NAME_LEN);

	context->funcNum++;
	context->inFunc = 1;
	return 0;
}

int EndFunc(struct _SvmParserContext_t *context)
{
	if (!context->inFunc) {
		//NoFuncError();
		SetErrorInfo(context, "The endfunc is not in a function\n");
		return -1;
	}

	context->funcs[context->currFuncIdx].endIdx = context->instrNum;
	context->inFunc = 0;
	return 0;
}
