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

#ifndef _FSL_ERROR_H_
#define _FSL_ERROR_H_

#include "fsl_config.h"
#include "fsl_interinstr.h"

void set_error_info(parser_context_t *context, const char *format, ...);
char *get_err_info(parser_context_t *context);
int get_err_line_num(parser_context_t *context);
void set_err_line_num(parser_context_t *context, int line);

/*add following by bdu*/
void SetErrorInfo(SvmParserContext_t *context, const char *format, ...);
char *GetErrInfo(SvmParserContext_t *context);
int GetErrLineNum(SvmParserContext_t *context);
void SetErrLineNum(SvmParserContext_t *context, int line);

#endif
