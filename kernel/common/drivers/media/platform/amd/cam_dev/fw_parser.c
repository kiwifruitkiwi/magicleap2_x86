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
#include "fsl_parser.h"
#include "fsl_log.h"
#include "fsl_error.h"

static int32 parse(uint8 *script, uint32 len, InstrSeq_t *bin)
{
	int32 status = 0;
	SvmParserContext_t *instr_context = NULL;

	fsl_set_log_level(FSL_LOG_LEVEL_MAX);

	instr_context = (SvmParserContext_t *)
		kmalloc(sizeof(SvmParserContext_t), GFP_KERNEL);

	if (SvmInitParser(instr_context) != 0) {
		SENSOR_LOG_INFO("initialize parser fail\n");
		status = -1;
		goto exit;
	}

	status = SvmParseScript(instr_context, (char *)script, len);

	if (status != 0) {
		SENSOR_LOG_INFO("ERROR LINE:%d\n",
				GetErrLineNum(instr_context));
		SENSOR_LOG_INFO("ERROR INFO:%s\n", GetErrInfo(instr_context));
		goto exit;
	}

	if (SvmGetInstrSeq(instr_context, bin) != 0) {
		SENSOR_LOG_INFO("get inter instrs fail\n");
		status = -1;
		goto exit;
	}

 exit:

	if (instr_context)
		kfree(instr_context);

	return status;
}

static struct fw_parser_operation_t g_fw_parser_interface = {
	parse,
};

struct fw_parser_operation_t *get_fw_parser_interface(void)
{
	return &g_fw_parser_interface;
}
EXPORT_SYMBOL(get_fw_parser_interface);
