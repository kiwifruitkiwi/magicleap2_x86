/*
 * Copyright (C) 2019-2020 Advanced Micro Devices, Inc. All rights reserved.
 */

#include "sensor_if.h"
#include "fsl_parser.h"
#include "fsl_log.h"
#include "fsl_error.h"

static int parse(unsigned char *script, unsigned int len,
	struct _InstrSeq_t *bin)
{
	int status = 0;
	struct _SvmParserContext_t *instr_context = NULL;

	fsl_set_log_level(FSL_LOG_LEVEL_MAX);

	instr_context = (struct _SvmParserContext_t *)
		kmalloc(sizeof(struct _SvmParserContext_t), GFP_KERNEL);

	if (SvmInitParser(instr_context) != 0) {
		ISP_PR_ERR("initialize parser fail\n");
		status = -1;
		goto exit;
	}

	status = SvmParseScript(instr_context, (char *)script, len);

	if (status != 0) {
		ISP_PR_ERR("ERROR LINE:%d\n",
				GetErrLineNum(instr_context));
		ISP_PR_ERR("ERROR INFO:%s\n", GetErrInfo(instr_context));
		goto exit;
	}

	if (SvmGetInstrSeq(instr_context, bin) != 0) {
		ISP_PR_ERR("get inter instrs fail\n");
		status = -1;
		goto exit;
	}

 exit:

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
