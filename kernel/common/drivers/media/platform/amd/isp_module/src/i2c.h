/**************************************************************************
 *copyright 2014~2015 advanced micro devices, inc.
 *
 *permission is hereby granted, free of charge, to any person obtaining a
 *copy of this software and associated documentation files (the "software"),
 *to deal in the software without restriction, including without limitation
 *the rights to use, copy, modify, merge, publish, distribute, sublicense,
 *and/or sell copies of the software, and to permit persons to whom the
 *software is furnished to do so, subject to the following conditions:
 *
 *the above copyright notice and this permission notice shall be included in
 *all copies or substantial portions of the software.
 *
 *THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL
 *THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 *OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 *ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 *OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

#ifndef I2C_H
#define I2C_H

/*this file referenced isp_i2c_driver.h*/

#include "dw_common.h"
#include "dw_apb_i2c_public.h"
#include "dw_apb_i2c_private.h"

#define MAX_I2C_TIMEOUT 2000	/*in ms */

int32 isp_i2c_init(uint32 input_clk);

/*refer to _aidt_hal_read_i2c_mem*/
result_t i2_c_read_mem(uint8 bus_num, uint16 slave_addr,
			i2c_slave_addr_mode_t slave_addr_mode,
			bool_t enable_restart, uint32 reg_addr,
			uint8 reg_addr_size, uint8 *p_read_buffer,
			uint32 byte_size);
result_t i2_c_read_mem_fw(pvoid context, uint32 id,
			uint8 bus_num, uint16 slave_addr,
			i2c_slave_addr_mode_t slave_addr_mode,
			bool_t enable_restart, uint32 reg_addr,
			uint8 reg_addr_size, uint8 *p_read_buffer,
			uint32 byte_size);

/*refer to _aidt_hal_write_i2c_mem*/
result_t i2_c_write_mem(uint8 bus_num, uint16 slave_addr,
			i2c_slave_addr_mode_t slave_addr_mode,
			uint32 reg_addr, uint32 reg_addr_size,
			uint8 *p_write_buffer, uint32 byte_size);

result_t i2_c_write_mem_fw(pvoid context, uint32 id,
			uint8 bus_num, uint16 slave_addr,
			i2c_slave_addr_mode_t slave_addr_mode,
			uint32 reg_addr, uint32 reg_addr_size,
			uint8 *p_write_buffer, uint32 byte_size);

/*refer to _aidt_hal_read_i2c_reg*/
result_t i2c_read_reg(uint8 bus_num, uint16 slave_addr,
			i2c_slave_addr_mode_t slave_addr_mode,
			bool_t enable_restart, uint32 reg_addr,
			uint8 reg_addr_size, void *preg_value, uint8 reg_size);
result_t i2c_read_reg_fw(pvoid context, uint32 id,
			 uint8 bus_num, uint16 slave_addr,
			 i2c_slave_addr_mode_t slave_addr_mode,
			 bool_t enable_restart, uint32 reg_addr,
			 uint8 reg_addr_size, void *preg_value, uint8 reg_size);

/*refer to _aidt_hal_write_i2c_reg*/
result_t i2c_write_reg(uint8 bus_num, uint16 slave_addr,
			i2c_slave_addr_mode_t slave_addr_mode, uint32 reg_addr,
			uint8 reg_addr_size, uint32 reg_value, uint8 reg_size);

result_t i2c_write_reg_fw(pvoid context, uint32 id,
			uint8 bus_num, uint16 slave_addr,
			i2c_slave_addr_mode_t slave_addr_mode,
			uint32 reg_addr, uint8 reg_addr_size,
			uint32 reg_value, uint8 reg_size);

#endif
