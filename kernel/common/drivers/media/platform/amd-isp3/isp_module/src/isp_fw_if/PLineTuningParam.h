/*
 * Copyright (C) 2019-2020 Advanced Micro Devices, Inc. All rights reserved.
 */

#pragma once

#include "os_base_type.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define MAX_PLINE_ENTRY (20)

struct _PLineEntry_t {
	unsigned int  exposure;   // us based exposure time
	unsigned int  gain;       // 1000 based gain value
};

struct _PLineTable_t {
	unsigned int  size;
	struct _PLineEntry_t entries[MAX_PLINE_ENTRY];
};

const struct _PLineTable_t *PLineGetDefaultTuningData(void);

#ifdef __cplusplus
}
#endif

