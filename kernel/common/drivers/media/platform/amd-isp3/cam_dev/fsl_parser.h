/*
 * Copyright (C) 2019-2021 Advanced Micro Devices, Inc. All rights reserved.
 */

#ifndef _PARSER_H_
#define _PARSER_H_

#include "fsl_interinstr.h"
#include "fsl_common.h"

int SvmInitParser(struct _SvmParserContext_t *context);
int SvmParseScript(struct _SvmParserContext_t *context, char *buf, int len);
int SvmGetInstrSeq(struct _SvmParserContext_t *context,
			struct _InstrSeq_t *instrSeq);

#endif
