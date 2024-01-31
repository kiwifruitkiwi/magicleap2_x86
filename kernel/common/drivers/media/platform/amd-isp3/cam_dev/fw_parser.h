/*
 * Copyright (C) 2019-2020 Advanced Micro Devices, Inc. All rights reserved.
 */

#ifndef _AMD_FW_PARSER_H_
#define _AMD_FW_PARSER_H_

#include "drv_isp_if.h"
#include "sensor_if.h"

struct fw_parser {
	int32_t (*parse)(uint8_t *script, uint32_t len,
			 struct _InstrSeq_t *bin);
};


#endif
