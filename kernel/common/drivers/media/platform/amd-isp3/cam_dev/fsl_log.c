/*
 * Copyright (C) 2019-2020 Advanced Micro Devices, Inc. All rights reserved.
 */

#include "fsl_log.h"
#include "fsl_interinstr.h"
#include "sensor_if.h"

static int g_log_level;
static char g_log_buf[MAX_LOG_BUF];

void fsl_log(enum _fsl_log_level_t level, const char *format, ...)
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

void fsl_set_log_level(enum _fsl_log_level_t level)
{
	g_log_level = level;
}
