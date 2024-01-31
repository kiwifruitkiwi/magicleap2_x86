/*
 * Copyright (C) 2019-2020 Advanced Micro Devices, Inc. All rights reserved.
 */

#pragma once
#define aidt_hal_write_reg(addr, value) \
	isp_hw_reg_write32(((unsigned int)(addr)), (unsigned int)value)
#define aidt_hal_read_reg(addr) isp_hw_reg_read32(((unsigned int)(addr)))
#define aidt_sleep(ms) msleep(ms)
#define GET_REG(reg)                     aidt_hal_read_reg(mm##reg)
#define SET_REG(reg, value)              aidt_hal_write_reg(mm##reg, value)
#define aidt_api_disable_ccpu() isp_hw_ccpu_disable(isp)
