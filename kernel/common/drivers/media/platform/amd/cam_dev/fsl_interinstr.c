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
#include "fsl_string.h"
#include "fsl_log.h"

int init_instr_context(parser_context_t *context, int max_instrs,
			int max_labels, int max_funcs)
{
	if (!context) {
		set_error_info(context, "instruction context is NULL\n");
		return -1;
	}

	memset(context, 0, sizeof(parser_context_t));
	context->max_instrs = max_instrs;
	context->max_labels = max_labels;
	context->max_funcs = max_funcs;
	context->instr_num = 0;
	context->label_num = 0;
	context->func_num = 0;
	context->in_func = 0;
	context->curr_func_idx = 0;
	return 0;
}

static int parse_arg(parser_context_t *context, const char *arg,
			instr_arg_ext_t *instr_arg_ext)
{
	int len;
	char indicator;
	const char *value;

	len = (int)strlen(arg);

	if (len < 2) {
		set_error_info(context,
		"arg length at least 2 characters start with R, $, @\n");
		return -1;
	}

	indicator = arg[0];
	value = arg + 1;

	if (indicator == 'R') {
		int reg_index;

		/*it is an register arg */
		if (!is_digital(value, 0, NULL)) {
			set_error_info(context,
		"arg after with R is not a digital string, from 0 to %d\n",
				(MAX_REGISTERS - 1));
			return -1;
		}

		reg_index = simple_strtol(value, NULL, 10);

		if ((reg_index < 0) || (reg_index > (MAX_REGISTERS - 1))) {
			set_error_info(context,
			"register %s is not supported, range R0~R%d\n",
				arg, (MAX_REGISTERS - 1));
			return -1;
		}

		instr_arg_ext->instr_arg.type = INSTR_ARG_TYPE_REG;
		instr_arg_ext->instr_arg.value.reg_index = reg_index;
	} else if (indicator == '$') {
		int imm_value;
		int is_hex;
		/*it is an immediator number */

		if (!is_digital(value, 1, &is_hex)) {
			set_error_info(context,
				"arg after with $ is not a digital string,");
			set_error_info(context,
				"a dec value or hex value start with 0x/0X\n");
			return -1;
		}

		if (is_hex)
			imm_value = simple_strtol(value, NULL, 16);
		else
			imm_value = simple_strtol(value, NULL, 10);

		instr_arg_ext->instr_arg.type = INSTR_ARG_TYPE_IMM;
		instr_arg_ext->instr_arg.value.imm_value = imm_value;
	} else if (indicator == '@') {
		int label_len;

		/*it is an label */

		label_len = (int)strlen(value);

		if (label_len >= MAX_LABEL_NAME_LEN) {
			set_error_info(context,
				"unsupported label characters large than:%d\n",
					MAX_LABEL_NAME_LEN);
			return -1;
		}

		instr_arg_ext->instr_arg.type = INSTR_ARG_TYPE_LABEL;
		strncpy(instr_arg_ext->symbol_name, value, MAX_LABEL_NAME_LEN);
		instr_arg_ext->symbol_name[label_len] = 0;
	} else if (indicator == 'M') {
		int value_len;
		int mem_idx;
		char mem_content_idx[MAX_WORD_LEN];
		char mem_indicator;
		const char *mem_value;

		/*it is a memory */
		value_len = (int)strlen(value);

		if (value_len < 5) {
			set_error_info(context,
			"memory type error, should be:mx[ry] or mx[imm]\n");
			return -1;
		}

		if ((value[1] != '[') || (value[value_len - 1] != ']')) {
			set_error_info(context,
			"memory type error, should be:mx[ry] or mx[imm]\n");
			return -1;
		}

		if (value[0] == '0')
			mem_idx = 0;
		else if (value[0] == '1')
			mem_idx = 1;
		else {
			set_error_info(context,
				"only M0 and M1 are supported\n");
			return -1;
		}

		if (value_len > MAX_WORD_LEN) {
			set_error_info(context,
				"too long size for memory type\n");
			return -1;
		}

		strncpy(mem_content_idx, value + 2, MAX_WORD_LEN);
		value_len = (int)strlen(mem_content_idx);
		//remove the ']' character.
		mem_content_idx[value_len - 1] = 0;

		mem_indicator = mem_content_idx[0];
		mem_value = &mem_content_idx[1];

		if (mem_indicator == 'R') {
			int reg_index;

			/*the idx is a register */
			if (!is_digital(mem_value, 0, NULL)) {
				set_error_info(context,
		"arg after with R is not a digital string, from 0 to %d\n",
					(MAX_REGISTERS - 1));
				return -1;
			}

			reg_index = simple_strtol(value, NULL, 10);

			if ((reg_index < 0)
				|| (reg_index > (MAX_REGISTERS - 1))) {
				set_error_info(context,
				"register %s is not supported,range R0~R%d\n",
					arg, (MAX_REGISTERS - 1));
				return -1;
			}

			instr_arg_ext->instr_arg.type = INSTR_ARG_TYPE_MEM;
			instr_arg_ext->instr_arg.value.mem_value.mem_idx_type =
				INSTR_ARG_TYPE_REG;
			instr_arg_ext->instr_arg.value.mem_value.mem_idx =
				mem_idx;
			instr_arg_ext->instr_arg.value.mem_value.idx.reg_idx =
				reg_index;
		} else if (mem_indicator == '$') {
			int imm_value;
			int is_hex;
			/*it is an immediator number */

			if (!is_digital(mem_value, 1, &is_hex)) {
				set_error_info(context,
				"arg after with $ is not a digital string,");
				set_error_info(context,
				"a dec value or hex value start with 0x/0X\n");
				return -1;
			}

			if (is_hex)
				imm_value = simple_strtol(mem_value, NULL, 16);
			else
				imm_value = simple_strtol(mem_value, NULL, 10);

			instr_arg_ext->instr_arg.type = INSTR_ARG_TYPE_MEM;
			instr_arg_ext->instr_arg.value.mem_value.mem_idx_type =
				INSTR_ARG_TYPE_IMM;
			instr_arg_ext->instr_arg.value.mem_value.mem_idx =
				mem_idx;
			instr_arg_ext->instr_arg.value.mem_value.idx.imm_value
				= imm_value;
		} else {
			set_error_info(context,
				"the memory index is not start with R, $\n");
			return -1;
		}
	} else {
		set_error_info(context, "the arg is not start with R, $, @\n");
		return -1;
	}

	return 0;
}

static int parse_call_arg(parser_context_t *context, const char *arg,
			instr_arg_ext_t *instr_arg_ext)
{
	int len;
	char indicator;
	const char *value;

	len = (int)strlen(arg);

	if (len < 2) {
		set_error_info(context,
	"arg length for call instruction at least 2 characters start @\n");
		return -1;
	}

	indicator = arg[0];
	value = arg + 1;

	if (indicator == '@') {
		int label_len;

		/*it is an label */

		label_len = (int)strlen(value);

		if (label_len >= MAX_FUNC_NAME_LEN) {
			set_error_info(context,
			"unsupported function characters large than:%d\n",
					MAX_FUNC_NAME_LEN);
			return -1;
		}

		instr_arg_ext->instr_arg.type = INSTR_ARG_TYPE_FUNC;
		strncpy(instr_arg_ext->symbol_name, value, MAX_LABEL_NAME_LEN);
		instr_arg_ext->symbol_name[label_len] = 0;
	} else {
		set_error_info(context,
			"the arg for call instruction is not start with @\n");
		return -1;
	}

	return 0;
}

int add_instr_wi2c(parser_context_t *context, const char *arg[], int num_args)
{
	int i;
	instr_ext_t instr_ext;
	instr_arg_ext_t instr_arg_ext;

	if (!context->in_func) {
		set_error_info(context,
			"the instruction is not in a funmctin\n");
		return -1;
	}

	if (num_args != 3) {
		set_error_info(context, "the args number is not 3\n");
		return -1;
	}

	instr_ext.op = INSTR_OP_WI2C;
	instr_ext.arg_num = num_args;

	for (i = 0; i < num_args; i++) {
		if (parse_arg(context, arg[i], &instr_arg_ext) != 0)
			return -1;

		instr_ext.args[i] = instr_arg_ext;
	}

	if ((instr_ext.args[0].instr_arg.type != INSTR_ARG_TYPE_REG) &&
		(instr_ext.args[0].instr_arg.type != INSTR_ARG_TYPE_IMM)) {
		set_error_info(context,
			"the first arg is not a register,");
		set_error_info(context,
			"or a immediate value for WI2C instruction\n");
		return -1;
	}

	if ((instr_ext.args[2].instr_arg.type != INSTR_ARG_TYPE_REG) &&
		(instr_ext.args[2].instr_arg.type != INSTR_ARG_TYPE_IMM)) {
		set_error_info(context,
		"the length arg of WI2C must be a reg or a immediate value\n");
		return -1;
	}

	if (context->instr_num >= context->max_instrs) {
		set_error_info(context, "too many instruction, max:%d\n",
			context->max_instrs);
		return -1;
	}

	context->instrs[context->instr_num++] = instr_ext;
	return 0;
}

int add_instr_ri2c(parser_context_t *context, const char *arg[], int num_args)
{
	int i;
	instr_ext_t instr_ext;
	instr_arg_ext_t instr_arg_ext;

	if (!context->in_func) {
		set_error_info(context,
			"the instruction is not in a funmctin\n");
		return -1;
	}

	if (num_args != 3) {
		set_error_info(context, "the args number is not 3\n");
		return -1;
	}

	instr_ext.op = INSTR_OP_RI2C;
	instr_ext.arg_num = num_args;

	for (i = 0; i < num_args; i++) {
		if (parse_arg(context, arg[i], &instr_arg_ext) != 0)
			return -1;

		instr_ext.args[i] = instr_arg_ext;
	}

	if ((instr_ext.args[0].instr_arg.type != INSTR_ARG_TYPE_REG) &&
		(instr_ext.args[0].instr_arg.type != INSTR_ARG_TYPE_MEM)) {
		set_error_info(context,
	"the first arg is not a register or a memory for RI2C instruction\n");
		return -1;
	}

	if (context->instr_num >= context->max_instrs) {
		set_error_info(context, "too many instruction, max:%d\n",
			context->max_instrs);
		return -1;
	}

	context->instrs[context->instr_num++] = instr_ext;
	return 0;
}

int add_instr_arg3(parser_context_t *context, instr_op_t instr_op,
			const char *arg[], int num_args)
{
	int i;
	instr_ext_t instr_ext;
	instr_arg_ext_t instr_arg_ext;

	/*if no function declare before this instruction, error!!! */
	if (!context->in_func) {
		set_error_info(context,
			"the instruction is not in a function\n");
		return -1;
	}

	if (num_args != 3) {
		set_error_info(context, "the args number is not 3\n");
		return -1;
	}

	instr_ext.op = instr_op;
	instr_ext.arg_num = num_args;

	for (i = 0; i < num_args; i++) {
		if (parse_arg(context, arg[i], &instr_arg_ext) != 0)
			return -1;

		instr_ext.args[i] = instr_arg_ext;
	}

	if (instr_ext.args[0].instr_arg.type != INSTR_ARG_TYPE_REG) {
		set_error_info(context, "the first arg is not a register\n");
		return -1;
	}

	if ((instr_ext.args[1].instr_arg.type != INSTR_ARG_TYPE_REG) &&
		(instr_ext.args[1].instr_arg.type != INSTR_ARG_TYPE_IMM)) {
		set_error_info(context,
			"the second arg must be a reg or an imm\n");
		return -1;
	}

	if ((instr_ext.args[2].instr_arg.type != INSTR_ARG_TYPE_REG) &&
		(instr_ext.args[2].instr_arg.type != INSTR_ARG_TYPE_IMM)) {
		set_error_info(context,
			"the third arg must be a reg or an imm\n");
		return -1;
	}

	if (context->instr_num >= context->max_instrs) {
		set_error_info(context, "too many instruction, max:%d\n",
			context->max_instrs);
		return -1;
	}

	context->instrs[context->instr_num++] = instr_ext;
	return 0;
}

int add_instr_arg2(parser_context_t *context, instr_op_t instr_op,
			const char *arg[], int num_args)
{
	int i;
	instr_ext_t instr_ext;
	instr_arg_ext_t instr_arg_ext;

	/*if no function declare before this instruction, error!!! */
	if (!context->in_func) {
		set_error_info(context,
			"the instruction is not in a function\n");
		return -1;
	}

	if (num_args != 2) {
		set_error_info(context, "the args number is not 2\n");
		return -1;
	}

	instr_ext.op = instr_op;
	instr_ext.arg_num = num_args;

	for (i = 0; i < num_args; i++) {
		if (parse_arg(context, arg[i], &instr_arg_ext) != 0)
			return -1;

		instr_ext.args[i] = instr_arg_ext;
	}

	if (instr_ext.args[0].instr_arg.type != INSTR_ARG_TYPE_REG) {
		set_error_info(context, "the first arg is not a register\n");
		return -1;
	}

	if ((instr_ext.args[1].instr_arg.type != INSTR_ARG_TYPE_REG) &&
		(instr_ext.args[1].instr_arg.type != INSTR_ARG_TYPE_IMM)) {
		set_error_info(context,
			"the second arg is not a register or an imm\n");
		return -1;
	}

	if (context->instr_num >= context->max_instrs) {
		set_error_info(context, "too many instruction, max:%d\n",
			context->max_instrs);
		return -1;
	}

	context->instrs[context->instr_num++] = instr_ext;
	return 0;
}

int add_instr_mov(parser_context_t *context, instr_op_t instr_op,
			const char *arg[], int num_args)
{
	int i;
	instr_ext_t instr_ext;
	instr_arg_ext_t instr_arg_ext;

	/*if no function declare before this instruction, error!!! */
	if (!context->in_func) {
		set_error_info(context,
			"the instruction is not in a function\n");
		return -1;
	}

	if (num_args != 2) {
		set_error_info(context, "the args number is not 2\n");
		return -1;
	}

	instr_ext.op = instr_op;
	instr_ext.arg_num = num_args;

	for (i = 0; i < num_args; i++) {
		if (parse_arg(context, arg[i], &instr_arg_ext) != 0)
			return -1;

		instr_ext.args[i] = instr_arg_ext;
	}

	if ((instr_ext.args[0].instr_arg.type != INSTR_ARG_TYPE_REG) &&
		(instr_ext.args[0].instr_arg.type != INSTR_ARG_TYPE_MEM)) {
		set_error_info(context,
			"the first arg is not a register or a memory\n");
		return -1;
	}

	if ((instr_ext.args[1].instr_arg.type != INSTR_ARG_TYPE_REG) &&
		(instr_ext.args[1].instr_arg.type != INSTR_ARG_TYPE_MEM) &&
		(instr_ext.args[1].instr_arg.type != INSTR_ARG_TYPE_IMM)) {
		set_error_info(context,
			"the second arg must be reg, mem or imm\n");
		return -1;
	}

	if (context->instr_num >= context->max_instrs) {
		set_error_info(context, "too many instruction, max:%d\n",
			context->max_instrs);
		return -1;
	}

	context->instrs[context->instr_num++] = instr_ext;
	return 0;
}

int add_instr_j(parser_context_t *context, instr_op_t instr_op,
		const char *arg[], int num_args)
{
	int i;
	instr_ext_t instr_ext;
	instr_arg_ext_t instr_arg_ext;

	/*if no function declare before this instruction, error!!! */
	if (!context->in_func) {
		set_error_info(context,
			"the instruction is not in a function\n");
		return -1;
	}

	if (num_args != 2) {
		set_error_info(context, "the args number is not 2\n");
		return -1;
	}

	instr_ext.op = instr_op;
	instr_ext.arg_num = num_args;

	for (i = 0; i < num_args; i++) {
		if (parse_arg(context, arg[i], &instr_arg_ext) != 0)
			return -1;

		instr_ext.args[i] = instr_arg_ext;
	}

	if ((instr_ext.args[0].instr_arg.type != INSTR_ARG_TYPE_REG) &&
		(instr_ext.args[0].instr_arg.type != INSTR_ARG_TYPE_IMM)) {
		set_error_info(context,
			"the first arg is not a register or an imm\n");
		return -1;
	}

	if (instr_ext.args[1].instr_arg.type != INSTR_ARG_TYPE_LABEL) {
		set_error_info(context, "the second arg must be a label\n");
		return -1;
	}

	if (context->instr_num >= context->max_instrs) {
		set_error_info(context, "too many instruction, max:%d\n",
			context->max_instrs);
		return -1;
	}

	context->instrs[context->instr_num++] = instr_ext;
	return 0;
}

int add_call(parser_context_t *context, const char *arg[], int num_args)
{
	instr_ext_t instr_ext;
	instr_arg_ext_t instr_arg_ext;

	/*if no function declare before this instruction, error!!! */
	if (!context->in_func) {
		set_error_info(context,
			"the instruction is not in a function\n");
		return -1;
	}

	if (num_args != 1) {
		set_error_info(context, "the args number is not 2\n");
		return -1;
	}

	instr_ext.op = INSTR_OP_CALL;
	instr_ext.arg_num = 1;

	if (parse_call_arg(context, arg[0], &instr_arg_ext) != 0)
		return -1;

	instr_ext.args[0] = instr_arg_ext;

	if (context->instr_num >= context->max_instrs) {
		set_error_info(context, "too many instruction, max:%d\n",
			context->max_instrs);
		return -1;
	}

	context->instrs[context->instr_num++] = instr_ext;
	return 0;
}

int add_label(parser_context_t *context, const char *label)
{
	int len;

	/*if no function declare before this instruction, error!!! */
	if (!context->in_func) {
		set_error_info(context, "the label is not in a function\n");
		return -1;
	}

	if (context->label_num >= context->max_labels) {
		set_error_info(context, "too many labels, max:%d\n",
			context->max_labels);
		return -1;
	}

	len = (int)strlen(label);

	context->labels[context->label_num].index = context->instr_num;
	strncpy(context->labels[context->label_num].label_name, label,
		MAX_LABEL_NAME_LEN);
	context->labels[context->label_num].label_name[len - 1] = 0;
	context->labels[context->label_num].func_idx = context->curr_func_idx;
	context->label_num++;
	return 0;
}

int add_func(parser_context_t *context, const char *function_name)
{
	if (context->in_func) {
		/*already in function, function nested is not supported */
		set_error_info(context, "function nested is not supported:%s\n",
			function_name);
		return -1;
	}

	if (context->func_num >= context->max_funcs) {
		set_error_info(context, "too many functions, max:%d\n",
			context->max_funcs);
		return -1;
	}

	context->curr_func_idx = context->func_num;

	context->funcs[context->curr_func_idx].start_idx = context->instr_num;
	strncpy(context->funcs[context->curr_func_idx].func_name,
		function_name, MAX_FUNC_NAME_LEN);

	context->func_num++;
	context->in_func = 1;
	return 0;
}

int end_func(parser_context_t *context)
{
	/*if no function declare before this instruction, error!!! */
	if (!context->in_func) {
		set_error_info(context, "the endfunc is not in a function\n");
		return -1;
	}

	context->funcs[context->curr_func_idx].end_idx = context->instr_num;
	context->in_func = 0;
	return 0;
}

/*add following by bdu*/

int InitInstrContext(SvmParserContext_t *context, int maxInstrs, int maxLabels,
			int maxFuncs)
{
	if (!context) {
		SetErrorInfo(context, "instruction context is NULL\n");
		return -1;
	}

	memset(context, 0, sizeof(SvmParserContext_t));
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

static int ParseArg(SvmParserContext_t *context, const char *arg,
			InterInstrArg_t *instrArg)
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
		int regIndex;

		//It is an register arg
		if (!IsDigital(value, 0, NULL)) {
			SetErrorInfo(context,
		"Arg after with R is not a digital string, from 0 to %d\n",
				(MAX_REGISTERS - 1));
			return -1;
		}

		regIndex = simple_strtol(value, NULL, 10);
		if ((regIndex < 0) || (regIndex > (MAX_REGISTERS - 1))) {
			SetErrorInfo(context,
				"Register %s is not supported,range R0~R%d\n",
				arg, (MAX_REGISTERS - 1));
			return -1;
		}

		instrArg->type = INTER_INSTR_ARG_TYPE_REG;
		instrArg->value.regIndex = regIndex;
	} else if (indicator == '$') {
		int immValue;
		int isHex;
		//It is an immediator number

		if (!IsDigital(value, 1, &isHex)) {
			SetErrorInfo(context,
				"Arg after with $ is not a digital string,");
			SetErrorInfo(context,
				"a dec value or hex value start with 0x/0X\n");
			return -1;
		}

		if (isHex)
			immValue = simple_strtol(value, NULL, 16);
		else
			immValue = simple_strtol(value, NULL, 10);

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
			int regIndex;

			//the idx is a register
			if (!IsDigital(memValue, 0, NULL)) {
				SetErrorInfo(context,
		"Arg after with R is not a digital string, from 0 to %d\n",
					(MAX_REGISTERS - 1));
				return -1;
			}
			regIndex = simple_strtol(value, NULL, 10);
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
			int immValue;
			int isHex;
			//It is an immediator number

			if (!IsDigital(memValue, 1, &isHex)) {
				SetErrorInfo(context,
				"Arg after with $ is not a digital string,");
				SetErrorInfo(context,
				"a dec value or hex value start with 0x/0X\n");
				return -1;
			}

			if (isHex)
				immValue = simple_strtol(memValue, NULL, 16);
			else
				immValue = simple_strtol(memValue, NULL, 10);

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

static int ParseCallArg(SvmParserContext_t *context, const char *arg,
			InterInstrArg_t *instrArg)
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

int AddInstrWi2c(SvmParserContext_t *context, const char *arg[], int numArgs)
{
	int i;
	InterInstr_t instr;
	InterInstrArg_t instrArg;

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

int AddInstrRi2c(SvmParserContext_t *context, const char *arg[], int numArgs)
{
	int i;
	InterInstr_t instr;
	InterInstrArg_t instrArg;

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

int AddInstrArg3(SvmParserContext_t *context, InterInstrOp_t instrOp,
		 const char *arg[], int numArgs)
{
	int i;
	InterInstr_t instr;
	InterInstrArg_t instrArg;

	//If no function declare before this instruction, error!!!
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

int AddInstrArg2(SvmParserContext_t *context, InterInstrOp_t instrOp,
		 const char *arg[], int numArgs)
{
	int i;
	InterInstr_t instr;
	InterInstrArg_t instrArg;

	//If no function declare before this instruction, error!!!
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

int AddInstrMov(SvmParserContext_t *context, InterInstrOp_t instrOp,
		const char *arg[], int numArgs)
{
	int i;
	InterInstr_t instr;
	InterInstrArg_t instrArg;

	//If no function declare before this instruction, error!!!
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

int AddInstrJ(SvmParserContext_t *context, InterInstrOp_t instrOp,
		const char *arg[], int numArgs)
{
	int i;
	InterInstr_t instr;
	InterInstrArg_t instrArg;

	//If no function declare before this instruction, error!!!
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

int AddCall(SvmParserContext_t *context, const char *arg[], int numArgs)
{
	InterInstr_t instr;
	InterInstrArg_t instrArg;

	//If no function declare before this instruction, error!!!
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

int AddLabel(SvmParserContext_t *context, const char *label)
{
	int len;

	//If no function declare before this instruction, error!!!
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

int AddFunc(SvmParserContext_t *context, const char *functionName)
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

int EndFunc(SvmParserContext_t *context)
{
	//If no function declare before this instruction, error!!!
	if (!context->inFunc) {
		//NoFuncError();
		SetErrorInfo(context, "The endfunc is not in a function\n");
		return -1;
	}

	context->funcs[context->currFuncIdx].endIdx = context->instrNum;
	context->inFunc = 0;
	return 0;
}
