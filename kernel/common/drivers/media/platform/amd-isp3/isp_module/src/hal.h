/*
 * Copyright (C) 2019-2020 Advanced Micro Devices, Inc. All rights reserved.
 */

#ifndef HAL_H
#define HAL_H

#include "os_base_type.h"
/*#include "isp_common.h"*/
#include "buffer_mgr.h"

#if !defined(ISP3_SILICON)
unsigned int isp_hw_reg_read32(unsigned int addr);
void isp_hw_reg_write32(unsigned int addr, unsigned int value);
#endif

int isp_hw_mem_write_(unsigned long long mem_addr, void *p_write_buffer,
			   unsigned int byte_size);
int isp_hw_mem_read_(unsigned long long mem_addr, void *p_read_buffer,
			  unsigned int byte_size);

#endif
