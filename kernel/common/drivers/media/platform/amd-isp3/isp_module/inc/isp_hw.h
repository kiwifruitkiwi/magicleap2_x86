/*
 * Copyright (C) 2019-2020 Advanced Micro Devices, Inc. All rights reserved.
 */

#ifndef HAL_HW
#define HAL_HW

#include "os_base_type.h"
#include "buffer_mgr.h"

#if defined(ISP3_SILICON)
#define isp_hw_reg_read32(addr) \
	g_cgs_srv->ops->read_register(g_cgs_srv, ((addr) >> 2))
#define isp_hw_reg_write32(addr, value) \
	g_cgs_srv->ops->write_register(g_cgs_srv, ((addr) >> 2), value)
#endif
int isp_hw_mem_write_(unsigned long long mem_addr, void *p_write_buffer,
			unsigned int byte_size);
int isp_hw_mem_read_(unsigned long long mem_addr, void *p_read_buffer,
			unsigned int byte_size);

#endif
