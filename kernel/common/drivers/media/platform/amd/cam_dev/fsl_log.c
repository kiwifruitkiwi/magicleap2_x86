/*
 * copyright 2014~2020 advanced micro devices, inc.
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

#include "fsl_log.h"
#include "fsl_interinstr.h"
#include "sensor_if.h"

static int g_log_level;
static char g_log_buf[MAX_LOG_BUF];

void fsl_log(fsl_log_level_t level, const char *format, ...)
{
	int cnt;
	char *level_string;
	char *log_buf;
	va_list ap;

	if (level == FSL_LOG_LEVEL_DEBUGI) {
		log_buf = g_log_buf;
		cnt = 0;
		goto debugi;
	}

	if (level > g_log_level)
		return;

	switch (level) {
	case FSL_LOG_LEVEL_INFO:
		level_string = "(INFO )";
		break;

	case FSL_LOG_LEVEL_WARN:
		level_string = "(WARN )";
		break;

	case FSL_LOG_LEVEL_ERROR:
		level_string = "(ERROR)";
		break;

	case FSL_LOG_LEVEL_DEBUG:
		level_string = "(DEBUG)";
		break;

	case FSL_LOG_LEVEL_DEBUGR:
		level_string = "(DEBUGR )";
		break;

	default:
		level_string = "(UNKNOWN )";
		break;
	}

	log_buf = g_log_buf;
	cnt = snprintf(log_buf, MAX_LOG_BUF, "%s: ", level_string);
	if (cnt < 0)
		cnt = 0;
	log_buf += cnt;

debugi:
	va_start(ap, format);
	cnt = snprintf(log_buf, MAX_LOG_BUF - cnt, format, ap);
	va_end(ap);

//	pr_info("%s", g_log_buf);
}

void fsl_set_log_level(fsl_log_level_t level)
{
	g_log_level = level;
}
