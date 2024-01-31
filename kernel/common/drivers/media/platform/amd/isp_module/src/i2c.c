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

#include "isp_common.h"
#include "i2c.h"
#include "log.h"
#include "isp_i2c_driver.h"
#include "isp_i2c_driver_private.h"
#include "isp_fw_if.h"
#include "isp_module_if.h"
#include "isp_soc_adpt.h"

int32 isp_i2c_init(uint32 input_clk)
{
	uint8 i;
	isp_i2c_configure_t i2c_configure;

	g_num_i2c = (g_isp_env_setting == ISP_ENV_SILICON) ?
		NUM_I2C_ASIC : NUM_I2C_FPGA;
	/*=================================================*/
	/*do I2C controller initialize. */
	/*=================================================*/
	isp_dbg_print_info("-> %s, clk %u, i2c num %u\n", __func__, input_clk,
			g_num_i2c);
	for (i = 0; i < g_num_i2c; i++) {
		i2c_configure.run_mode = isp_i2c_driver_run_mode_poll;
		i2c_configure.device_index = i;
		i2c_configure.address_mode = isp_i2c_addr_mode_7bit;
		i2c_configure.speed = 400;
		i2c_configure.scl_duty_ratio = 60;
		i2c_configure.input_clk = input_clk;
		if (isp_i2c_err_code_success !=
			isp_i2c_configure(&i2c_configure)) {
			isp_dbg_print_err
			("<- %s fail for isp_i2c_configure %d\n",
				__func__, i);
			return RET_FAILURE;
		}

		isp_dbg_print_info("isp_i2c_configure %d suc\n", i);

	}
	isp_dbg_print_info("<- %s suc\n", __func__);
	return RET_SUCCESS;
};

result_t i2c_read_reg(uint8 bus_num, uint16 slave_addr,
		i2c_slave_addr_mode_t slave_addr_mode,
		bool_t enable_restart, uint32 reg_addr,
		uint8 reg_addr_size, void *preg_value, uint8 reg_size)
{
	if ((reg_size < 1) || (reg_size > 4))
		return RET_INVALID_PARM;

	/*return RET_INVALID_PARM; */
	return i2_c_read_mem(bus_num, slave_addr, slave_addr_mode,
			enable_restart, reg_addr, reg_addr_size,
			(uint8 *) preg_value, reg_size);
}

result_t i2c_read_reg_fw(pvoid context, uint32 id,
			uint8 bus_num, uint16 slave_addr,
			i2c_slave_addr_mode_t slave_addr_mode,
			bool_t enable_restart, uint32 reg_addr,
			uint8 reg_addr_size, void *preg_value, uint8 reg_size)
{
	if ((reg_size < 1) || (reg_size > 4))
		return RET_INVALID_PARM;

	/*return RET_INVALID_PARM; */
	return i2_c_read_mem_fw(context, id, bus_num, slave_addr,
				slave_addr_mode, enable_restart, reg_addr,
				reg_addr_size, (uint8 *) preg_value, reg_size);
}

result_t i2c_write_reg(uint8 bus_num, uint16 slave_addr,
			i2c_slave_addr_mode_t slave_addr_mode, uint32 reg_addr,
			uint8 reg_addr_size, uint32 reg_value, uint8 reg_size)
{
	if ((reg_size < 1) || (reg_size > 4))
		return RET_INVALID_PARM;

	/*return RET_INVALID_PARM; */
	return i2_c_write_mem(bus_num, slave_addr, slave_addr_mode, reg_addr,
			reg_addr_size, (uint8 *)&reg_value, reg_size);
}

result_t i2c_write_reg_fw(pvoid context, uint32 id,
			uint8 bus_num, uint16 slave_addr,
			i2c_slave_addr_mode_t slave_addr_mode,
			uint32 reg_addr, uint8 reg_addr_size,
			uint32 reg_value, uint8 reg_size)
{
	if ((reg_size < 1) || (reg_size > 4))
		return RET_INVALID_PARM;

	return i2_c_write_mem_fw(context, id, bus_num, slave_addr,
				slave_addr_mode, reg_addr, reg_addr_size,
				(uint8 *)&reg_value, reg_size);
}

result_t i2_c_read_mem(uint8 bus_num, uint16 slave_addr,
			i2c_slave_addr_mode_t slave_addr_mode,
			bool_t enable_restart, uint32 reg_addr,
			uint8 reg_addr_size, uint8 *p_read_buffer,
			uint32 byte_size)
{
	result_t result = RET_SUCCESS;
	/*aidt_hal_context_t *p_ctx; */
	isp_i2c_err_code_e i2c_result;
	isp_i2c_subaddr_mode_e subaddr_mode;

	unreferenced_parameter(slave_addr);
	unreferenced_parameter(slave_addr_mode);
	/*aidt_hal_handle_t hal_handle; */

	/*hal_handle = g_hal_handle; */

	if (/*(hal_handle == NULL) */
		(p_read_buffer == NULL)) {
		isp_dbg_print_err
			("-><- %s, NULL pointer parameter\n", __func__);
		return RET_NULL_POINTER;
	}

	if ((bus_num >= g_num_i2c) || (reg_addr_size > 2)) {
		isp_dbg_print_err("-><- %s, invalid parameter\n", __func__);
		return RET_INVALID_PARM;
	}

	if (reg_addr_size == 1)
		subaddr_mode = isp_i2c_subaddr_mode_8bit;
	else if (reg_addr_size == 2)
		subaddr_mode = isp_i2c_subaddr_mode_16bit;
	else
		subaddr_mode = isp_i2c_subaddr_mode_none;

	/*isp_dbg_print_err("in i2_c_read_mem, enable_restart %d,\
	 *bus_num(device_index) %d\n", enable_restart, bus_num);
	 */
	if (enable_restart) {
		i2c_result =
			isp_cci_sequential_random_read(bus_num, slave_addr,
					(uint16) reg_addr,
					subaddr_mode, p_read_buffer,
					byte_size);
	} else {
		i2c_result =
			isp_i2c_sequential_random_read(bus_num, slave_addr,
					(uint16)reg_addr,
					subaddr_mode, p_read_buffer,
					byte_size);
	}
	if (i2c_result != isp_i2c_err_code_success)
		result = RET_FAILURE;

	return result;
}

result_t i2_c_read_mem_fw(pvoid context, uint32 id,
			uint8 bus_num, uint16 slave_addr,
			i2c_slave_addr_mode_t slave_addr_mode,
			bool_t enable_restart, uint32 reg_addr,
			uint8 reg_addr_size, uint8 *p_read_buffer,
			uint32 byte_size)
{
	struct isp_context *isp = context;
	struct sensor_info *snr_info;
	isp_i2c_subaddr_mode_e subaddr_mode;
	CmdSendI2cMsg_t cmdSendI2cMsg;
	result_t ret = RET_SUCCESS;

	unreferenced_parameter(slave_addr_mode);

	if (p_read_buffer == NULL) {
		isp_dbg_print_err("-><- %s, fail NULL pointer\n", __func__);
		return RET_NULL_POINTER;
	}

	if ((bus_num >= g_num_i2c) || (reg_addr_size > 2)) {
		isp_dbg_print_err("-><- %s, fail bad para\n", __func__);
		return RET_INVALID_PARM;
	}

	snr_info = &isp->sensor_info[id];

	/* iic control buff alloc  */
	if (snr_info->iic_tmp_buf == NULL) {

		snr_info->iic_tmp_buf =
			isp_gpu_mem_alloc(IIC_CONTROL_BUF_SIZE,
			ISP_GPU_MEM_TYPE_NLFB);

		if (snr_info->iic_tmp_buf) {
			isp_dbg_print_info
			("in %s, aloc iic control buf ok\n", __func__);
		} else {
			isp_dbg_print_err
				("-><- %s, fail aloc i2c buf\n", __func__);
			return RET_NULL_POINTER;
		}
	}

	if (reg_addr_size == 1)
		subaddr_mode = isp_i2c_subaddr_mode_8bit;
	else if (reg_addr_size == 2)
		subaddr_mode = isp_i2c_subaddr_mode_16bit;
	else
		subaddr_mode = isp_i2c_subaddr_mode_none;

	cmdSendI2cMsg.deviceId = bus_num;
	cmdSendI2cMsg.msg.msgType = I2C_MSG_TYPE_READ;
	cmdSendI2cMsg.msg.enRestart = enable_restart;
	cmdSendI2cMsg.msg.deviceAddrType = I2C_DEVICE_ADDR_TYPE_7BIT;
	cmdSendI2cMsg.msg.regAddrType = subaddr_mode;
	cmdSendI2cMsg.msg.deviceAddr = slave_addr;
	cmdSendI2cMsg.msg.regAddr = (uint16) reg_addr;
	cmdSendI2cMsg.msg.dataLen = (uint16) byte_size;
	// firmware don't care this value
	cmdSendI2cMsg.msg.dataCheckSum = *p_read_buffer;

	isp_split_addr64(snr_info->iic_tmp_buf->gpu_mc_addr,
			 &cmdSendI2cMsg.msg.dataAddrLo,
			 &cmdSendI2cMsg.msg.dataAddrHi);

	if (isp_send_fw_cmd_cid(isp, id, CMD_ID_SEND_I2C_MSG,
				FW_CMD_RESP_STREAM_ID_GLOBAL,
				FW_CMD_PARA_TYPE_DIRECT,
				&cmdSendI2cMsg,
				sizeof(cmdSendI2cMsg)) != RET_SUCCESS) {
		isp_dbg_print_err("-><- %s, send cmdfail\n", __func__);
		return RET_FAILURE;
	}
	return ret;
}

result_t i2_c_write_mem(uint8 bus_num, uint16 slave_addr,
			i2c_slave_addr_mode_t slave_addr_mode,
			uint32 reg_addr, uint32 reg_addr_size,
			uint8 *p_write_buffer, uint32 byte_size)
{
	result_t result = RET_SUCCESS;
	/*aidt_hal_context_t *p_ctx; */
	isp_i2c_err_code_e i2c_result;
	isp_i2c_subaddr_mode_e subaddr_mode;
	/*aidt_hal_handle_t hal_handle; */
	unreferenced_parameter(slave_addr);
	unreferenced_parameter(slave_addr_mode);
	/*hal_handle = g_hal_handle; */

	if (/*(hal_handle == NULL) */
		(p_write_buffer == NULL)) {
		isp_dbg_print_err
			("-><- %s, NULL pointer parameter\n", __func__);
		return RET_NULL_POINTER;
	}

	if ((bus_num >= g_num_i2c) || (reg_addr_size > 2)) {
		isp_dbg_print_err("-><- %s, invalid parameter\n", __func__);
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
						(uint16) reg_addr,
						subaddr_mode,
						p_write_buffer, byte_size);
	}
	if (i2c_result != isp_i2c_err_code_success)
		result = RET_FAILURE;

	return result;
}

result_t i2_c_write_mem_fw(pvoid context, uint32 id,
			uint8 bus_num, uint16 slave_addr,
			i2c_slave_addr_mode_t slave_addr_mode,
			uint32 reg_addr, uint32 reg_addr_size,
			uint8 *p_write_buffer, uint32 byte_size)
{
	struct isp_context *isp = context;
	struct sensor_info *snr_info;
	isp_i2c_subaddr_mode_e subaddr_mode;
	CmdSendI2cMsg_t cmdSendI2cMsg;
	result_t ret = RET_SUCCESS;

	unreferenced_parameter(slave_addr_mode);

	if (/*(hal_handle == NULL) */
		(p_write_buffer == NULL)) {
		isp_dbg_print_err
			("-><- %s, fail NULL pointer\n", __func__);
		return RET_NULL_POINTER;
	}

	if ((bus_num >= g_num_i2c) || (reg_addr_size > 2)) {
		isp_dbg_print_err("-><- %s, fail bad para\n", __func__);
		return RET_INVALID_PARM;
	}

	snr_info = &isp->sensor_info[id];

	/* iic control buff alloc  */
	if (snr_info->iic_tmp_buf == NULL) {

		snr_info->iic_tmp_buf =
			isp_gpu_mem_alloc(IIC_CONTROL_BUF_SIZE,
				ISP_GPU_MEM_TYPE_NLFB);

		if (snr_info->iic_tmp_buf) {
			isp_dbg_print_info
			("in %s, aloc iic control buf ok\n", __func__);
		} else {
			isp_dbg_print_err
			("-><- %s, fail aloc i2c buf\n", __func__);
			return RET_NULL_POINTER;
		}
	}

	if (reg_addr_size == 1)
		subaddr_mode = isp_i2c_subaddr_mode_8bit;
	else if (reg_addr_size == 2)
		subaddr_mode = isp_i2c_subaddr_mode_16bit;
	else
		subaddr_mode = isp_i2c_subaddr_mode_none;

	cmdSendI2cMsg.deviceId = bus_num;
	cmdSendI2cMsg.msg.msgType = I2C_MSG_TYPE_WRITE;
	cmdSendI2cMsg.msg.enRestart = 0;
	cmdSendI2cMsg.msg.deviceAddrType = I2C_DEVICE_ADDR_TYPE_7BIT;
	cmdSendI2cMsg.msg.regAddrType = subaddr_mode;
	cmdSendI2cMsg.msg.deviceAddr = slave_addr;
	cmdSendI2cMsg.msg.regAddr = (uint16) reg_addr;
	cmdSendI2cMsg.msg.dataLen = (uint16) byte_size;
	// firmware don't care this value
	cmdSendI2cMsg.msg.dataCheckSum = *p_write_buffer;

	isp_split_addr64(snr_info->iic_tmp_buf->gpu_mc_addr,
			 &cmdSendI2cMsg.msg.dataAddrLo,
			 &cmdSendI2cMsg.msg.dataAddrHi);

	memcpy(snr_info->iic_tmp_buf->sys_addr, p_write_buffer,
		min((uint32)byte_size, (uint32)CMD_RESPONSE_BUF_SIZE));

	if (isp_send_fw_cmd_sync(isp, CMD_ID_SEND_I2C_MSG,
					FW_CMD_RESP_STREAM_ID_GLOBAL,
					FW_CMD_PARA_TYPE_DIRECT,
					&cmdSendI2cMsg,
					sizeof(cmdSendI2cMsg),
					1000 * 10, NULL, 0) != RET_SUCCESS) {
		isp_dbg_print_err("-><- %s, send cmdfail\n", __func__);
		return RET_FAILURE;
	}

	isp_dbg_print_info("-><- %s, suc\n", __func__);

	return ret;
}
