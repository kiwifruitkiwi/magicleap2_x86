/*
 * Copyright (C) 2019-2020 Advanced Micro Devices, Inc. All rights reserved.
 */

#ifndef _FSL_LOG_H_
#define _FSL_LOG_H_

#include "fsl_interinstr.h"

enum _fsl_log_level_t {
	FSL_LOG_LEVEL_INVALID = -1,
	FSL_LOG_LEVEL_ERROR = 0,
	FSL_LOG_LEVEL_WARN = 1,
	FSL_LOG_LEVEL_INFO = 2,
	FSL_LOG_LEVEL_DEBUG = 3,
	FSL_LOG_LEVEL_DEBUGR = 4,
	FSL_LOG_LEVEL_DEBUGI = 5,
	FSL_LOG_LEVEL_MAX
};

void fsl_log(enum _fsl_log_level_t level, const char *format, ...);
void fsl_set_log_level(enum _fsl_log_level_t level);

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
