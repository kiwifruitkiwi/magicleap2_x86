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

#ifndef _FSL_LOG_H_
#define _FSL_LOG_H_

#include "fsl_interinstr.h"

typedef enum _fsl_log_level_t {
	FSL_LOG_LEVEL_INVALID = -1,
	FSL_LOG_LEVEL_ERROR = 0,
	FSL_LOG_LEVEL_WARN = 1,
	FSL_LOG_LEVEL_INFO = 2,
	FSL_LOG_LEVEL_DEBUG = 3,
	FSL_LOG_LEVEL_DEBUGR = 4,
	FSL_LOG_LEVEL_DEBUGI = 5,
	FSL_LOG_LEVEL_MAX
} fsl_log_level_t;

void fsl_log(fsl_log_level_t level, const char *format, ...);
void fsl_set_log_level(fsl_log_level_t level);

#define FSL_LOG_INFO(expr ...)\
	fsl_log(FSL_LOG_LEVEL_INFO, expr)

#define FSL_LOG_WARN(expr ...)\
	fsl_log(FSL_LOG_LEVEL_WARN, expr)

#define FSL_LOG_ERROR(expr ...)\
	fsl_log(FSL_LOG_LEVEL_ERROR, expr)

#define FSL_LOG_DEBUG(expr ...)\
	fsl_log(FSL_LOG_LEVEL_DEBUG, expr)

#define FSL_LOG_DEBUGR(expr ...)\
	fsl_log(FSL_LOG_LEVEL_DEBUGR, expr)

#define FSL_LOG_DEBUGI(expr ...)\
	fsl_log(FSL_LOG_LEVEL_DEBUGI, expr)

#endif
