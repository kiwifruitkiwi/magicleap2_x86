/*
 * Copyright (C) 2019-2021 Advanced Micro Devices, Inc. All rights reserved.
 */

#include "fsl_config.h"
#include "fsl_log.h"
#include "fsl_interinstr.h"
#include "sensor_if.h"

void SetErrorInfo(struct _SvmParserContext_t *context,
	const char *format, ...)
{
	int cnt;
	va_list ap;

	va_start(ap, format);
	cnt = snprintf(context->errInfo, MAX_ERROR_INFO_LEN, format, ap);
	va_end(ap);

	FSL_LOG_ERROR("%s", context->errInfo);
	return;
};

char *GetErrInfo(struct _SvmParserContext_t *context)
{
	return context->errInfo;
}

int GetErrLineNum(struct _SvmParserContext_t *context)
{
	return context->errLine;
}

void SetErrLineNum(struct _SvmParserContext_t *context, int line)
{
	context->errLine = line;
}
