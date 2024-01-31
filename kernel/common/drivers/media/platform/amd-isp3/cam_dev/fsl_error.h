/*
 * Copyright (C) 2019-2021 Advanced Micro Devices, Inc. All rights reserved.
 */

#ifndef _FSL_ERROR_H_
#define _FSL_ERROR_H_

#include "fsl_config.h"
#include "fsl_interinstr.h"

void SetErrorInfo(struct _SvmParserContext_t *context, const char *format, ...);
char *GetErrInfo(struct _SvmParserContext_t *context);
int GetErrLineNum(struct _SvmParserContext_t *context);
void SetErrLineNum(struct _SvmParserContext_t *context, int line);

#endif
