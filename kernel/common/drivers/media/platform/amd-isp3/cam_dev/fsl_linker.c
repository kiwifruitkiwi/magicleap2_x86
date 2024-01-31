/*
 * Copyright (C) 2019-2021 Advanced Micro Devices, Inc. All rights reserved.
 */

#include "sensor_if.h"
#include "fsl_error.h"
#include "fsl_interinstr.h"
#include "fsl_log.h"

static int FindLabelInstrIndex(struct _SvmParserContext_t *context,
			int funcIdx, const char *labelName)
{
	int i;
	int labelNum;

	labelNum = context->labelNum;

	for (i = 0; i < labelNum; i++) {
		if (context->labels[i].funcIdx == funcIdx) {
			if (!strcmp(labelName, context->labels[i].labelName))
				return context->labels[i].index;
		}
	}

	return -1;
}

static int FindFuncIndex(struct _SvmParserContext_t *context,
				const char *funcName)
{
	int i;
	int funcNum;

	funcNum = context->funcNum;

	for (i = 0; i < funcNum; i++) {
		if (!strcmp(funcName, context->funcs[i].funcName))
			return i;
	}
	return -1;
}

int SvmInterInstrLink(struct _SvmParserContext_t *context)
{
	int i, j;
	int instrNum;
	int argNum;
	int instrIdx;
	int startIdx, endIdx;
	int funcIdx;

	instrNum = context->instrNum;

	for (funcIdx = 0; funcIdx < context->funcNum; funcIdx++) {
		startIdx = context->funcs[funcIdx].startIdx;
		endIdx = context->funcs[funcIdx].endIdx;

//		SVM_LOG_DEBUG("Link function:%s, startIdx=%d, endIdx=%d:\n",
//			context->funcs[funcIdx].funcName,
//			context->funcs[funcIdx].startIdx,
//			context->funcs[funcIdx].endIdx);

		for (i = startIdx; i < endIdx; i++) {
			argNum = context->instrs[i].argNum;
			for (j = 0; j < argNum; j++) {
				if (context->instrs[i].args[j].type ==
					INTER_INSTR_ARG_TYPE_LABEL) {
					instrIdx =
					FindLabelInstrIndex(context, funcIdx,
					context->instrs[i].args[j].symbolName);

//					SVM_LOG_DEBUG
//				("Link label:%s to instruction index:%d\n",
//					context->instrs[i].args[j].symbolName,
//					instrIdx);

					if (instrIdx < 0) {
						SetErrorInfo
						(context,
						"Can't find label:%s\n",
					context->instrs[i].args[j].symbolName);
						return -1;
					}
				context->instrs[i].args[j].value.labelInstrIdx
						= instrIdx;
				} else if (
					context->instrs[i].args[j].type ==
						INTER_INSTR_ARG_TYPE_FUNC) {
					int callFuncIdx =
						FindFuncIndex(context,
					context->instrs[i].args[j].symbolName);

//					SVM_LOG_DEBUG
//				("Link function:%s to instruction index:%d\n",
//					context->instrs[i].args[j].symbolName,
//					callFuncIdx);

					if (callFuncIdx < 0) {
						SetErrorInfo(context,
						"Can't find func:%s\n",
					context->instrs[i].args[j].symbolName);
						return -1;
					}
					context->instrs[i].args[j].value.funcIdx
						= callFuncIdx;
				}
			}
		}
	}
	return 0;
}
