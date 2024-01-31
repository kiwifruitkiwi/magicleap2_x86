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

#include "fsl_config.h"
#include "fsl_log.h"
#include "fsl_interinstr.h"
#include "sensor_if.h"

void set_error_info(parser_context_t *context, const char *format, ...)
{
	int cnt;
	va_list ap;

	va_start(ap, format);
	cnt = snprintf(context->err_info, MAX_ERROR_INFO_LEN, format, ap);
	va_end(ap);

	FSL_LOG_ERROR("%s", context->err_info);
}

char *get_err_info(parser_context_t *context)
{
	return context->err_info;
}

int get_err_line_num(parser_context_t *context)
{
	return context->err_line;
}

void set_err_line_num(parser_context_t *context, int line)
{
	context->err_line = line;
}

/*add following by bdu*/
void SetErrorInfo(SvmParserContext_t *context, const char *format, ...)
{
	int cnt;
	va_list ap;

	va_start(ap, format);
	cnt = snprintf(context->errInfo, MAX_ERROR_INFO_LEN, format, ap);
	va_end(ap);

	FSL_LOG_ERROR("%s", context->errInfo);
	return;
};

char *GetErrInfo(SvmParserContext_t *context)
{
	return context->errInfo;
}

int GetErrLineNum(SvmParserContext_t *context)
{
	return context->errLine;
}

void SetErrLineNum(SvmParserContext_t *context, int line)
{
	context->errLine = line;
}
