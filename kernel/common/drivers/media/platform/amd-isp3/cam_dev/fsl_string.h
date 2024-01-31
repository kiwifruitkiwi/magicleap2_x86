/*
 * Copyright (C) 2019-2021 Advanced Micro Devices, Inc. All rights reserved.
 */

#ifndef _FSL_STRING_H_
#define _FSL_STRING_H_

#include "fsl_interinstr.h"

#define END_OF_STREAM (1073741824)

struct _SvmStringStream_t {
	char *buf;
	int len;
	int readPos;
};

int InitStringStream(struct _SvmParserContext_t *context,
		struct _SvmStringStream_t *stringStream, char *buf, int len);
int GetLine(struct _SvmParserContext_t *context,
		struct _SvmStringStream_t *stringStream, char *buf, int len);
int Split(struct _SvmParserContext_t *context, char *line, int len,
		char *words, int maxWords, int maxWordLen);
int IsDigital(const char *string, int enHex, int *isHex);

#endif
