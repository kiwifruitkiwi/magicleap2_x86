/*
 * copyright 2014~2015 advanced micro devices, inc.
 *
 * permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "software"),
 * to deal in the software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the software, and to permit persons to whom the
 * software is furnished to do so, subject to the following conditions:
 *
 * the above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#ifndef _AMD_CAM_COMMON_H_
#define _AMD_CAM_COMMON_H_

#include "os_base_type.h"
#include "isp_cam_if.h"

enum flash_mode_t {
//	FLASH_MODE_INVALID = 1,
	FLASH_MODE_TORCH = 2,
	FLASH_MODE_FLASH = 3,
//	FLASH_MODE_MAX
};

enum flash_max_torch_curr_t {
	FLASH_MAX_TORCH_CURR_LEVEL0,
	FLASH_MAX_TORCH_CURR_LEVEL1,
	FLASH_MAX_TORCH_CURR_LEVEL2,
	FLASH_MAX_TORCH_CURR_LEVEL3,
	FLASH_MAX_TORCH_CURR_LEVEL4,
	FLASH_MAX_TORCH_CURR_LEVEL5,
	FLASH_MAX_TORCH_CURR_LEVEL6,
	FLASH_MAX_TORCH_CURR_LEVEL7
};

enum flash_max_flash_curr_t {
	FLASH_MAX_FLASH_CURR_LEVEL0,
	FLASH_MAX_FLASH_CURR_LEVEL1,
	FLASH_MAX_FLASH_CURR_LEVEL2,
	FLASH_MAX_FLASH_CURR_LEVEL3,
	FLASH_MAX_FLASH_CURR_LEVEL4,
	FLASH_MAX_FLASH_CURR_LEVEL5,
	FLASH_MAX_FLASH_CURR_LEVEL6,
	FLASH_MAX_FLASH_CURR_LEVEL7,
	FLASH_MAX_FLASH_CURR_LEVEL8,
	FLASH_MAX_FLASH_CURR_LEVEL9,
	FLASH_MAX_FLASH_CURR_LEVEL10,
	FLASH_MAX_FLASH_CURR_LEVEL11,
	FLASH_MAX_FLASH_CURR_LEVEL12,
	FLASH_MAX_FLASH_CURR_LEVEL13,
	FLASH_MAX_FLASH_CURR_LEVEL14,
	FLASH_MAX_FLASH_CURR_LEVEL15,
};

// GROUP0 -> Sensor
// GROUP1 -> PHY
// GROUP2 -> VCM?

#define SENSOR_ID0_GROUP0_BUS 0
#define SENSOR_ID0_GROUP1_BUS 2	//TODO
#define SENSOR_ID0_GROUP2_BUS 0

#define SENSOR_ID1_GROUP0_BUS 1	//TODO
#define SENSOR_ID1_GROUP1_BUS 3	//TODO
#define SENSOR_ID1_GROUP2_BUS 0

#define SENSOR_ID2_GROUP0_BUS 2
#define SENSOR_ID2_GROUP1_BUS 5
#define SENSOR_ID2_GROUP2_BUS 2

#define FLASH_ID0_GROUP0_BUS 3
#define FLASH_ID1_GROUP0_BUS 3
#define FLASH_ID2_GROUP0_BUS 3

#define I2C_SLAVE_ADDRESS_MODE_7BIT 0
#define I2C_SLAVE_ADDRESS_MODE_10BIT 1

#define GROUP1_I2C_WRITE(bus, reg_addr, value)\
	(get_ifl_interface()->i2c_write_reg(get_ifl_interface()->context, \
					bus, \
					GROUP1_I2C_SLAVE_ADDR, \
					GROUP1_I2C_SLAVE_ADDR_MODE, \
					reg_addr, GROUP1_I2C_REG_ADDR_SIZE, \
					value, GROUP1_I2C_REG_SIZE))

#define GROUP1_I2C_READ(bus, reg_addr, pvalue)\
	(get_ifl_interface()->i2c_read_reg(get_ifl_interface()->context, \
					bus, \
					GROUP1_I2C_SLAVE_ADDR, \
					GROUP1_I2C_SLAVE_ADDR_MODE, 1, \
					reg_addr, GROUP1_I2C_REG_ADDR_SIZE, \
					pvalue, GROUP1_I2C_REG_SIZE))

#define GROUP0_I2C_WRITE_FW(id, bus, reg_addr, value)\
	(get_ifl_interface()->i2c_write_reg_fw(get_ifl_interface()->context, \
					id, bus, GROUP0_I2C_SLAVE_ADDR, \
					GROUP0_I2C_SLAVE_ADDR_MODE, \
					reg_addr, GROUP0_I2C_REG_ADDR_SIZE, \
					value, GROUP0_I2C_REG_SIZE))

#define GROUP0_I2C_WRITE(bus, reg_addr, value)\
	(get_ifl_interface()->i2c_write_reg(get_ifl_interface()->context, \
					bus, \
					GROUP0_I2C_SLAVE_ADDR, \
					GROUP0_I2C_SLAVE_ADDR_MODE, reg_addr,\
					GROUP0_I2C_REG_ADDR_SIZE, value, \
					GROUP0_I2C_REG_SIZE))

#define GROUP0_I2C_READ_FW(id, bus, reg_addr, pvalue, byte_size)\
	(get_ifl_interface()->i2c_read_reg_fw(get_ifl_interface()->context, \
					id, bus, GROUP0_I2C_SLAVE_ADDR, \
					GROUP0_I2C_SLAVE_ADDR_MODE, 1, \
					reg_addr, GROUP0_I2C_REG_ADDR_SIZE, \
					pvalue, byte_size))

#define GROUP0_I2C_READ(bus, reg_addr, pvalue)\
	(get_ifl_interface()->i2c_read_reg(get_ifl_interface()->context, \
					bus, \
					GROUP0_I2C_SLAVE_ADDR, \
					GROUP0_I2C_SLAVE_ADDR_MODE, 1, \
					reg_addr, GROUP0_I2C_REG_ADDR_SIZE, \
					pvalue, GROUP0_I2C_REG_SIZE))

#define GROUP2_I2C_WRITE(bus, reg_addr, value)\
	(get_ifl_interface()->i2c_write_reg(get_ifl_interface()->context, \
					bus, \
					GROUP2_I2C_SLAVE_ADDR, \
					GROUP2_I2C_SLAVE_ADDR_MODE, reg_addr, \
					GROUP2_I2C_REG_ADDR_SIZE, \
					value, GROUP2_I2C_REG_SIZE))

#define GROUP2_I2C_READ(bus, reg_addr, pvalue)\
	(get_ifl_interface()->i2c_read_reg(get_ifl_interface()->context, \
					bus, \
					GROUP2_I2C_SLAVE_ADDR, \
					GROUP2_I2C_SLAVE_ADDR_MODE, 1, \
					reg_addr, GROUP2_I2C_REG_ADDR_SIZE, \
					pvalue, GROUP2_I2C_REG_SIZE))

#define SENSOR_I2C_READ			GROUP0_I2C_READ
#define SENSOR_I2C_WRITE		GROUP0_I2C_WRITE
#define SENSOR_I2C_READ_FW		GROUP0_I2C_READ_FW
#define SENSOR_I2C_WRITE_FW		GROUP0_I2C_WRITE_FW
#define SENSOR_PHY_I2C_READ		GROUP1_I2C_READ
#define SENSOR_PHY_I2C_WRITE		GROUP1_I2C_WRITE
#define SENSOR_VCM_I2C_READ		GROUP2_I2C_READ
#define SENSOR_VCM_I2C_WRITE		GROUP2_I2C_WRITE

#define ISP_WRITE_REG(A, V) get_ifl_interface()->isp_reg_write32(\
	get_ifl_interface()->context, A, V)
#define ISP_READ_REG(A) get_ifl_interface()->isp_reg_read32(\
	get_ifl_interface()->context, A)

#define drv_context_res			drv_context->res
#define drv_context_frame_length_lines	drv_context->frame_length_lines
#define drv_context_line_length_pck	drv_context->line_length_pck
#define drv_context_t_line_numerator	drv_context->t_line_numerator
#define drv_context_t_line_denominator	drv_context->t_line_denominator
#define drv_context_bitrate		drv_context->bitrate
#define drv_context_mode		drv_context->mode
#define drv_context_max_torch_curr	drv_context->max_torch_curr
#define drv_context_max_flash_curr	drv_context->max_flash_curr

#define UPDATE_FW_GV_ITIME(g0, g1, g2) \
	do {			\
		memset(&fw_gv, 0, sizeof(fw_gv)); \
		fw_gv.args[0] = g0; \
		fw_gv.args[1] = g1; \
		fw_gv.args[2] = g2; \
		get_ifl_interface()->set_fw_gv(get_ifl_interface()->context, \
					module->id, FW_GV_AE_ITIME, &fw_gv); \
	} while (0)

struct drv_context {
	uint32 res;

	uint32 frame_length_lines;
	uint32 line_length_pck;
	uint32 t_line_numerator;
	uint32 t_line_denominator;
	uint32 bitrate;

	uint32 flag;

	enum flash_mode_t mode;
	enum flash_max_torch_curr_t max_torch_curr;
	enum flash_max_flash_curr_t max_flash_curr;
};

#endif
