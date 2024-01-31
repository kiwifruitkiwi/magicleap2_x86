/*
 * Copyright (C) 2019-2021 Advanced Micro Devices, Inc. All rights reserved.
 */

#include "isp_common.h"
#include "i2c.h"
#include "log.h"
#include "isp_i2c_driver.h"
#include "isp_i2c_driver_private.h"
#include "isp_fw_if.h"
#include "isp_module_if.h"
#include "isp_soc_adpt.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "[ISP][i2c]"

int isp_i2c_init(unsigned int input_clk)
{
	unsigned char i;
	struct _isp_i2c_configure_t i2c_configure;

	g_num_i2c = (g_isp_env_setting == ISP_ENV_SILICON) ?
		NUM_I2C_ASIC : NUM_I2C_FPGA;
	/*=============================================*/
	/*do I2C controller initialize. */
	/*=============================================*/
	ISP_PR_INFO("clk %u, i2c num %u\n", input_clk, g_num_i2c);
	for (i = 0; i < g_num_i2c; i++) {
		i2c_configure.run_mode = isp_i2c_driver_run_mode_poll;
		i2c_configure.device_index = i;
		i2c_configure.address_mode = isp_i2c_addr_mode_7bit;
		i2c_configure.speed = 400;
		i2c_configure.scl_duty_ratio = 60;
		i2c_configure.input_clk = input_clk;
		if (isp_i2c_err_code_success !=
			isp_i2c_configure(&i2c_configure)) {
			ISP_PR_ERR("fail for isp_i2c_configure %d\n", i);
			return RET_FAILURE;
		}

		ISP_PR_INFO("isp_i2c_configure %d suc\n", i);

	}
	ISP_PR_PC("ISP I2C init suc!\n");
	return RET_SUCCESS;
};

int i2c_read_reg(unsigned char bus_num, unsigned short slave_addr,
		enum _i2c_slave_addr_mode_t slave_addr_mode,
		int enable_restart, unsigned int reg_addr,
		unsigned char reg_addr_size, void *preg_value,
		unsigned char reg_size)
{
	if ((reg_size < 1) || (reg_size > 4))
		return RET_INVALID_PARM;

	/*return RET_INVALID_PARM; */
	return i2_c_read_mem(bus_num, slave_addr, slave_addr_mode,
			enable_restart, reg_addr, reg_addr_size,
			(unsigned char *) preg_value, reg_size);
}

int i2c_write_reg(unsigned char bus_num, unsigned short slave_addr,
		enum _i2c_slave_addr_mode_t slave_addr_mode,
		unsigned int reg_addr,
		unsigned char reg_addr_size, unsigned int reg_value,
		unsigned char reg_size)
{
	if ((reg_size < 1) || (reg_size > 4))
		return RET_INVALID_PARM;

	/*return RET_INVALID_PARM; */
	return i2_c_write_mem(bus_num, slave_addr, slave_addr_mode, reg_addr,
			reg_addr_size, (unsigned char *)&reg_value, reg_size);
}

//only enable NEED_VERIF_REG in debug phase to check if write is successful
//#define NEED_VERIF_REG
int i2c_write_reg_group(unsigned char bus_num,
		unsigned short __maybe_unused slave_addr,
		enum _i2c_slave_addr_mode_t __maybe_unused slave_addr_mode,
		unsigned int reg_addr,
		unsigned char reg_addr_size, unsigned int *reg_value,
		unsigned char reg_size)
{
	int ret = RET_FAILURE;
	u32 __maybe_unused reg_rd = 0;
	u32 reg_wr = 0;

	reg_wr = *reg_value;
	switch (reg_size) {
	case 1:
		reg_wr &= 0xff;
		break;
	case 2:
		reg_wr &= 0xffff;
		break;
	case 3:
		reg_wr &= 0xffffff;
		break;
	case 4:
		break;
	default:
		ISP_PR_ERR("fail bad size %u", reg_size);
		return RET_INVALID_PARM;
		break;
	}

	ret = i2_c_write_mem(bus_num, slave_addr, slave_addr_mode, reg_addr,
			     reg_addr_size, (unsigned char *)&reg_wr,
			     reg_size);

#ifdef NEED_VERIF_REG
	i2_c_read_mem(bus_num, slave_addr, slave_addr_mode, 1, reg_addr,
		      reg_addr_size, (unsigned char *)&reg_rd, reg_size);
	if (ret != RET_SUCCESS || reg_wr != reg_rd) {
		ISP_PR_ERR("reg[%x] write fail, ret %u, %x!=%x, rewrite",
			   reg_addr, ret, reg_wr, reg_rd);
		ret = i2_c_write_mem(bus_num, slave_addr, slave_addr_mode,
				     reg_addr, reg_addr_size,
				     (unsigned char *)&reg_wr, reg_size);
	}

#endif
	if (ret == RET_SUCCESS)
		ISP_PR_DBG("write reg[%x]=%x suc", reg_addr, reg_wr)
	else
		ISP_PR_ERR("write reg[%x]=%x fail %u", reg_addr, reg_wr, ret);
	return ret;
}

int i2_c_read_mem(unsigned char bus_num,
	unsigned short __maybe_unused slave_addr,
	enum _i2c_slave_addr_mode_t __maybe_unused slave_addr_mode,
	int enable_restart, unsigned int reg_addr,
	unsigned char reg_addr_size,
	unsigned char *p_read_buffer,
	unsigned int byte_size)
{
	int result = RET_SUCCESS;
	enum _isp_i2c_err_code_e i2c_result;
	enum _isp_i2c_subaddr_mode_e subaddr_mode;

	if (p_read_buffer == NULL) {
		ISP_PR_ERR("NULL pointer parameter\n");
		return RET_NULL_POINTER;
	}

	if ((bus_num >= g_num_i2c) || (reg_addr_size > 2)) {
		ISP_PR_ERR("invalid parameter\n");
		return RET_INVALID_PARM;
	}

	if (reg_addr_size == 1)
		subaddr_mode = isp_i2c_subaddr_mode_8bit;
	else if (reg_addr_size == 2)
		subaddr_mode = isp_i2c_subaddr_mode_16bit;
	else
		subaddr_mode = isp_i2c_subaddr_mode_none;

	/*ISP_PR_ERR("in i2_c_read_mem, enable_restart %d,\
	 *bus_num(device_index) %d\n", enable_restart, bus_num);
	 */
	if (enable_restart) {
		i2c_result =
			isp_cci_sequential_random_read(bus_num, slave_addr,
					(unsigned short) reg_addr,
					subaddr_mode, p_read_buffer,
					byte_size);
	} else {
		i2c_result =
			isp_i2c_sequential_random_read(bus_num, slave_addr,
					(unsigned short)reg_addr,
					subaddr_mode, p_read_buffer,
					byte_size);
	}
	if (i2c_result != isp_i2c_err_code_success)
		result = RET_FAILURE;

	return result;
}

int i2_c_write_mem(unsigned char bus_num,
	unsigned short __maybe_unused slave_addr,
	enum _i2c_slave_addr_mode_t __maybe_unused slave_addr_mode,
	unsigned int reg_addr, unsigned int reg_addr_size,
	unsigned char *p_write_buffer, unsigned int byte_size)
{
	int result = RET_SUCCESS;
	/*aidt_hal_context_t *p_ctx; */
	enum _isp_i2c_err_code_e i2c_result;
	enum _isp_i2c_subaddr_mode_e subaddr_mode;

	if (p_write_buffer == NULL) {
		ISP_PR_ERR("NULL pointer parameter\n");
		return RET_NULL_POINTER;
	}

	if ((bus_num >= g_num_i2c) || (reg_addr_size > 2)) {
		ISP_PR_ERR("invalid parameter\n");
		return RET_INVALID_PARM;
	}

	if (reg_addr_size == 1)
		subaddr_mode = isp_i2c_subaddr_mode_8bit;
	else if (reg_addr_size == 2)
		subaddr_mode = isp_i2c_subaddr_mode_16bit;
	else
		subaddr_mode = isp_i2c_subaddr_mode_none;

	if (subaddr_mode == isp_i2c_subaddr_mode_none) {
		i2c_result =
			isp_i2c_write(bus_num, slave_addr, p_write_buffer,
				byte_size);
	} else {
		i2c_result =
			isp_cci_sequential_random_write(bus_num, slave_addr,
						(unsigned short) reg_addr,
						subaddr_mode,
						p_write_buffer, byte_size);
	}
	if (i2c_result != isp_i2c_err_code_success)
		result = RET_FAILURE;

	return result;
}
