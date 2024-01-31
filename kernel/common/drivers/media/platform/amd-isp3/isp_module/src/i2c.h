/*
 * Copyright (C) 2019-2021 Advanced Micro Devices, Inc. All rights reserved.
 */

#ifndef I2C_H
#define I2C_H

/*this file referenced isp_i2c_driver.h*/

#include "dw_common.h"
#include "dw_apb_i2c_public.h"
#include "dw_apb_i2c_private.h"

#define MAX_I2C_TIMEOUT 2000	/*in ms */

int isp_i2c_init(unsigned int input_clk);

/*refer to _aidt_hal_read_i2c_mem*/
int i2_c_read_mem(unsigned char bus_num, unsigned short slave_addr,
			enum _i2c_slave_addr_mode_t slave_addr_mode,
			int enable_restart, unsigned int reg_addr,
			unsigned char reg_addr_size,
			unsigned char *p_read_buffer,
			unsigned int byte_size);

/*refer to _aidt_hal_write_i2c_mem*/
int i2_c_write_mem(unsigned char bus_num, unsigned short slave_addr,
			enum _i2c_slave_addr_mode_t slave_addr_mode,
			unsigned int reg_addr, unsigned int reg_addr_size,
			unsigned char *p_write_buffer, unsigned int byte_size);

/*refer to _aidt_hal_read_i2c_reg*/
int i2c_read_reg(unsigned char bus_num, unsigned short slave_addr,
			enum _i2c_slave_addr_mode_t slave_addr_mode,
			int enable_restart, unsigned int reg_addr,
			unsigned char reg_addr_size, void *preg_value,
			unsigned char reg_size);

/*refer to _aidt_hal_write_i2c_reg*/
int i2c_write_reg(unsigned char bus_num, unsigned short slave_addr,
			enum _i2c_slave_addr_mode_t slave_addr_mode,
			unsigned int reg_addr,
			unsigned char reg_addr_size, unsigned int reg_value,
			unsigned char reg_size);

/*refer to _aidt_hal_write_i2c_reg*/
int i2c_write_reg_group(unsigned char bus_num,
		unsigned short __maybe_unused slave_addr,
		enum _i2c_slave_addr_mode_t __maybe_unused slave_addr_mode,
		unsigned int reg_addr,
		unsigned char reg_addr_size, unsigned int *reg_value,
		unsigned char reg_size);

#endif
