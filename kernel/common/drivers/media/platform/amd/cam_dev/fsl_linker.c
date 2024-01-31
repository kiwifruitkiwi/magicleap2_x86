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

#include "sensor_if.h"
#include "fsl_error.h"
#include "fsl_interinstr.h"
#include "fsl_log.h"

static int find_label_instr_index(parser_context_t *context, int func_idx,
				const char *label_name)
{
	int i;
	int label_num;

	label_num = context->label_num;

	for (i = 0; i < label_num; i++) {
		if (context->labels[i].func_idx == func_idx) {
			if (!strcmp(label_name, context->labels[i].label_name))
				return context->labels[i].index;
		}
	}

	return -1;
}

static int find_func_index(parser_context_t *context, const char *func_name)
{
	int i;
	int func_num;

	func_num = context->func_num;

	for (i = 0; i < func_num; i++) {
		if (!strcmp(func_name, context->funcs[i].func_name))
			return i;
	}

	return -1;
}

int inter_instr_link(parser_context_t *context)
{
	int i, j;
	int instr_num;
	int arg_num;
	int instr_idx;
	int start_idx, end_idx;
	int func_idx;

	instr_num = context->instr_num;

	for (func_idx = 0; func_idx < context->func_num; func_idx++) {
		start_idx = context->funcs[func_idx].start_idx;
		end_idx = context->funcs[func_idx].end_idx;

		FSL_LOG_DEBUG("link function:%s, start_idx=%d, end_idx=%d:\n",
			context->funcs[func_idx].func_name,
			context->funcs[func_idx].start_idx,
			context->funcs[func_idx].end_idx);

		for (i = start_idx; i < end_idx; i++) {
			arg_num = context->instrs[i].arg_num;

			for (j = 0; j < arg_num; j++) {
				if (context->instrs[i].args[j].instr_arg.type
				== INSTR_ARG_TYPE_LABEL) {
					instr_idx =
						find_label_instr_index(context,
						func_idx,
				      context->instrs[i].args[j].symbol_name);
					FSL_LOG_DEBUG
				("link label:%s to instruction index:%d\n",
					context->instrs[i].args[j].symbol_name,
					instr_idx);

					if (instr_idx < 0) {
						set_error_info(context,
						"can't find label:%s\n",
				context->instrs[i].args[j].symbol_name);
						return -1;
					}

		    context->instrs[i].args[j].instr_arg.value.label_instr_idx
						= instr_idx;
				} else if (
				    context->instrs[i].args[j].instr_arg.type
					== INSTR_ARG_TYPE_FUNC) {
					int call_func_idx =
						find_func_index(context,
				    context->instrs[i].args[j].symbol_name);
					FSL_LOG_DEBUG
				("link function:%s to instruction index:%d\n",
					context->instrs[i].args[j].symbol_name,
					call_func_idx);

					if (call_func_idx < 0) {
						set_error_info(context,
							"can't find func:%s\n",
				context->instrs[i].args[j].symbol_name);
						return -1;
					}

			context->instrs[i].args[j].instr_arg.value.func_idx
					= call_func_idx;
				}
			}
		}
	}

	return 0;
}

/*add following by bdu*/
static int FindLabelInstrIndex(SvmParserContext_t *context, int funcIdx,
			const char *labelName)
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

static int FindFuncIndex(SvmParserContext_t *context, const char *funcName)
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

int SvmInterInstrLink(SvmParserContext_t *context)
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
				} else if (context->instrs[i].args[j].type ==
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
