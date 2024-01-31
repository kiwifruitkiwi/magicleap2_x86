/*
 * Copyright (C) 2019-2020 Advanced Micro Devices, Inc. All rights reserved.
 */

#ifndef DW_COMMON_IO_H
#define DW_COMMON_IO_H

#include "hal.h"

#define DW_OUT32_32P(v, p)	\
	isp_hw_reg_write32((unsigned int)((unsigned long long)&p),\
				(unsigned int)v)

#define DW_OUT16P(v, p)	DW_OUT32_32P(v, p)

#define DW_IN32_32P(p)	\
	isp_hw_reg_read32((unsigned int)((unsigned long long)&p))

#define DW_IN16P(p)	\
	((unsigned short)(isp_hw_reg_read32((unsigned int)(\
	(unsigned long long)&p)) & 0xffff))

#define DW_IN8P(p)	\
	((unsigned char)(isp_hw_reg_read32(\
	(unsigned int)((unsigned long long)&p)) & 0xff))

#define DW_INP(p)	DW_IN32_32P(p)

#endif				/*DW_COMMON_IO_H */
