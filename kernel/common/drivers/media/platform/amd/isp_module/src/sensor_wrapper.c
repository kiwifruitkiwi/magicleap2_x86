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
#include "sensor_wrapper.h"
#include "log.h"

void reset_sensor_phy_fpga(uint32 device_id)
{
	uint32 reg_value;

	isp_dbg_print_info("-> %s, device_id = %d\n", __func__, device_id);

	if (device_id == 0) {
		//For PHY reset
		// FPGA TO CONTROL
		//phy0_cs 0, phy0_resx 0 cam0_pwdn 0, cam0_resetb 0
		isp_hw_reg_write32(0xc00, 0x0);
		usleep_range(1000, 2000);
		//phy0_cs 1, phy0_resx 0 cam0_pwdn 0, cam0_resetb 0
		isp_hw_reg_write32(0xc00, 0x1);
		usleep_range(1000, 2000);
		//phy0_cs 1, phy0_resx 1 cam0_pwdn 0, cam0_resetb 0
		isp_hw_reg_write32(0xc00, 0x3);
		usleep_range(1000, 2000);

		//ISP INTERNAL REGISTER CONTROL
		//For sensor reset
		reg_value = isp_hw_reg_read32(mmISP_GPIO_SHUTDOWN_SENSOR0);
		reg_value &= (~(0x1 << 4));
		isp_hw_reg_write32(mmISP_GPIO_SHUTDOWN_SENSOR0, reg_value);
		usleep_range(1000, 2000);
		reg_value |= (0x1 << 4);
		isp_hw_reg_write32(mmISP_GPIO_SHUTDOWN_SENSOR0, reg_value);
		usleep_range(1000, 2000);

		/*
		 *reg_value = isp_hw_reg_read32(mmISP_SENSOR_FLASH_CTRL);
		 *reg_value &= (~(0x1 << 0));
		 *isp_hw_reg_write32(mmISP_SENSOR_FLASH_CTRL, reg_value);
		 *reg_value |= (0x1 << 0);
		 *isp_hw_reg_write32(mmISP_SENSOR_FLASH_CTRL, reg_value);
		 */
	} else if (device_id == 1) {
		//For PHY reset
		isp_hw_reg_write32(0xc08, 0x0);
		isp_hw_reg_write32(0xc08, 0x1);
		isp_hw_reg_write32(0xc08, 0x3);

		//For sensor reset
		reg_value = isp_hw_reg_read32(mmISP_GPIO_SHUTDOWN_SENSOR1);
		reg_value &= (~(0x1 << 4));
		isp_hw_reg_write32(mmISP_GPIO_SHUTDOWN_SENSOR1, reg_value);
		reg_value |= (0x1 << 4);
		isp_hw_reg_write32(mmISP_GPIO_SHUTDOWN_SENSOR1, reg_value);

		/*
		 *reg_value = isp_hw_reg_read32(mmISP_SENSOR_FLASH_CTRL);
		 *reg_value &= (~(0x1 << 0));
		 *isp_hw_reg_write32(mmISP_SENSOR_FLASH_CTRL, reg_value);
		 *reg_value |= (0x1 << 0);
		 *isp_hw_reg_write32(mmISP_SENSOR_FLASH_CTRL, reg_value);
		 */
	} else {
		isp_dbg_print_err("%s device_id %d unsupported\n",
			__func__, device_id);
	}

	isp_dbg_print_info("<- %s done\n", __func__);
}

void reset_sensor_shutdown(uint32 device_id)
{
	uint32 reg_value;

	isp_dbg_print_info("-> %s, device_id = %d\n", __func__, device_id);

	if (device_id == 0) {
		//ISP INTERNAL REGISTER CONTROL
		//For sensor reset
		reg_value = isp_hw_reg_read32(mmISP_GPIO_SHUTDOWN_SENSOR0);
		reg_value &= (~(0x1 << 4));
		//cam0_pwdn 0
		isp_hw_reg_write32(mmISP_GPIO_SHUTDOWN_SENSOR0, reg_value);
		usleep_range(1000, 2000);
		reg_value |= (0x1 << 4);
		//cam0_pwdn 1
		isp_hw_reg_write32(mmISP_GPIO_SHUTDOWN_SENSOR0, reg_value);
		usleep_range(5000, 6000);
	} else if (device_id == 1) {
		//For sensor reset
		reg_value = isp_hw_reg_read32(mmISP_GPIO_SHUTDOWN_SENSOR1);
		reg_value &= (~(0x1 << 4));
		isp_hw_reg_write32(mmISP_GPIO_SHUTDOWN_SENSOR1, reg_value);
		reg_value |= (0x1 << 4);
		isp_hw_reg_write32(mmISP_GPIO_SHUTDOWN_SENSOR1, reg_value);
	} else {
		isp_dbg_print_err("%s: device_id %d unsupported\n",
			__func__, device_id);
	}

	isp_dbg_print_info("<- %s done\n", __func__);
}

struct sensor_operation_t *isp_snr_get_snr_drv(struct isp_context *pcontext,
			uint32 camera_id)
{
	if ((pcontext == NULL) || (camera_id >= CAMERA_ID_MAX)) {
		isp_dbg_print_err
			("illegal parameter in %s, %d\n",
			__func__, camera_id);
		return NULL;
	}
	/*return &pcontext->sensor_ops[camera_id]; */
	return pcontext->p_sensor_ops[camera_id];
}

struct vcm_operation_t *isp_snr_get_vcm_drv(struct isp_context *pcontext,
			uint32 camera_id)
{
	if ((pcontext == NULL) || (camera_id >= CAMERA_ID_MAX)) {
		isp_dbg_print_err
			("illegal parameter in %s, %d\n",
			__func__, camera_id);
		return NULL;
	}

	return pcontext->p_vcm_ops[camera_id];
}

result_t isp_snr_pwr_set(struct isp_context *pcontext, uint32 camera_id,
			 bool_t on)
{
	struct sensor_operation_t *sensor_driver;
	result_t ret;
	struct isp_pwr_unit *pwr_unit;

	pwr_unit = &pcontext->isp_pu_cam[camera_id];
	if (on && (pwr_unit->pwr_status == ISP_PWR_UNIT_STATUS_ON)) {
		ret = RET_SUCCESS;
		isp_dbg_print_info
		("-><- %s, succ on do none,cam_id %d,on:%u\n",
			__func__, camera_id, on);
		goto quit;
	}

	if (!on && (pwr_unit->pwr_status == ISP_PWR_UNIT_STATUS_OFF)) {
		ret = RET_SUCCESS;
		isp_dbg_print_info
		("-><- %s, succ off do none,cam_id %d,on:%u\n",
			__func__, camera_id, on);
		goto quit;
	}

	sensor_driver = isp_snr_get_snr_drv(pcontext, camera_id);
	if (sensor_driver == NULL) {
		ret = RET_FAILURE;
		isp_dbg_print_err
		("-><- %s, fail for no snr drv,cam_id %d,on:%u\n",
			__func__, camera_id, on);
		goto quit;
	}

	if (sensor_driver->poweron_sensor == NULL) {
		ret = RET_FAILURE;
		isp_dbg_print_err
		("-><-%s, fail for no pwr func,cam_id %d,on:%u\n",
			__func__, camera_id, on);
		goto quit;
	}

	isp_dbg_print_err("-> %s,cam_id %d,on:%u\n",
		__func__, camera_id, on);
	if (g_isp_env_setting != ISP_ENV_SILICON) {
		reset_sensor_phy_fpga(camera_id);
	} else {
		reset_sensor_shutdown(camera_id);
	};
	ret = sensor_driver->poweron_sensor(camera_id, on);
	if (ret == RET_SUCCESS) {
		isp_dbg_print_info("<- %s,suc\n", __func__);
		if (on) {
			pwr_unit->pwr_status = ISP_PWR_UNIT_STATUS_ON;
			isp_get_cur_time_tick(&pwr_unit->on_time);
			pwr_unit->idle_start_time =
				pwr_unit->on_time + MS_TO_TIME_TICK(2000);
		} else {
			pwr_unit->pwr_status = ISP_PWR_UNIT_STATUS_OFF;
		}
	} else {
		isp_dbg_print_err("<- %s,fail\n", __func__);
	}

quit:
	if (!on)
		pwr_unit->pwr_status = ISP_PWR_UNIT_STATUS_OFF;
	return ret;
}

result_t isp_snr_rst(struct isp_context *pcontext, uint32 camera_id)
{
	struct sensor_operation_t *sensor_driver;

	sensor_driver = isp_snr_get_snr_drv(pcontext, camera_id);
	if (sensor_driver == NULL)
		return RET_FAILURE;

	if (sensor_driver->reset_sensor == NULL) {
		isp_dbg_print_err
			("the sensor driver don't supported reset_sensor\n");
		return RET_FAILURE;
	}

	if (g_isp_env_setting != ISP_ENV_SILICON) {
		reset_sensor_phy_fpga(camera_id);
	} else {
		reset_sensor_shutdown(camera_id);
	};

	return sensor_driver->reset_sensor(camera_id);
}

result_t isp_snr_cfg(struct isp_context *pcontext, uint32 camera_id,
			uint32 res_fps_id, uint32 flag)
{
	struct sensor_operation_t *sensor_driver;

	sensor_driver = isp_snr_get_snr_drv(pcontext, camera_id);
	if (sensor_driver == NULL)
		return RET_FAILURE;

	if (sensor_driver->config_sensor == NULL) {
		isp_dbg_print_err
			("the sensor driver don't supported config_sensor\n");
		return RET_FAILURE;
	}

	return sensor_driver->config_sensor(camera_id, res_fps_id, flag);
}

result_t isp_snr_start(struct isp_context *pcontext, uint32 camera_id)
{
	struct sensor_operation_t *sensor_driver;
	result_t result;
	struct sensor_info *sensor_info;

	if ((pcontext == NULL) || (camera_id >= CAMERA_ID_MAX)) {
		isp_dbg_print_err
			("-><- %s fail for illegal para %d\n",
			__func__, camera_id);
		return RET_FAILURE;
	};

	sensor_info = &pcontext->sensor_info[camera_id];
	sensor_driver = isp_snr_get_snr_drv(pcontext, camera_id);
	if (sensor_driver == NULL)
		return RET_FAILURE;

	if (sensor_driver->start_sensor == NULL) {
		isp_dbg_print_err
("-><- %s fail supported start_sensor\n", __func__);
		return RET_FAILURE;
	}
	isp_dbg_print_info("-> %s for %d\n", __func__, camera_id);
	result =
		sensor_driver->start_sensor(camera_id,
				(g_isp_env_setting ==
				ISP_ENV_SILICON ? FALSE : TRUE));
	if (result != RET_SUCCESS) {
		isp_dbg_print_err
			("<- %s fail for start_sensor fail %d\n",
			__func__, camera_id);
	} else {
		isp_dbg_print_info("<- %s success, id:%d\n",
			__func__, camera_id);
	}
	return result;
}

result_t isp_snr_stop(struct isp_context *pcontext, uint32 camera_id)
{
	struct sensor_operation_t *sensor_driver;

	sensor_driver = isp_snr_get_snr_drv(pcontext, camera_id);
	if (sensor_driver == NULL)
		return RET_FAILURE;

	if (sensor_driver->stop_sensor == NULL) {
		isp_dbg_print_err
			("the sensor driver don't supported stop_sensor\n");
		return RET_FAILURE;
	}

	return sensor_driver->stop_sensor(camera_id);
}

result_t isp_snr_stream_on(struct isp_context *pcontext, uint32 camera_id)
{
	struct sensor_operation_t *sensor_driver;

	sensor_driver = isp_snr_get_snr_drv(pcontext, camera_id);
	if (sensor_driver == NULL)
		return RET_FAILURE;

	if (sensor_driver->stream_on_sensor == NULL) {
		isp_dbg_print_err
		("the sensor driver don't supported stream_on_sensor\n");
		return RET_FAILURE;
	}

	return sensor_driver->stream_on_sensor(camera_id);
}

result_t isp_snr_get_caps(struct isp_context *pcontext, uint32 camera_id,
			isp_sensor_prop_t *caps)
{
	struct sensor_operation_t *sensor_driver;

	sensor_driver = isp_snr_get_snr_drv(pcontext, camera_id);
	if (sensor_driver == NULL)
		return RET_FAILURE;

	if (sensor_driver->get_caps == NULL) {
		isp_dbg_print_err("the sensor driver don't support get_caps\n");
		return RET_FAILURE;
	}

	return sensor_driver->get_caps(camera_id, caps);
}

result_t isp_snr_exposure_ctrl(struct isp_context *pcontext, uint32 cam_id,
			uint32 new_gain,
			uint32 new_integration_time,
			uint32 *set_gain, uint32 *set_integration)
{
	struct sensor_operation_t *sensor_driver;

	sensor_driver = isp_snr_get_snr_drv(pcontext, cam_id);
	if (sensor_driver == NULL)
		return RET_FAILURE;

	if (sensor_driver->exposure_control == NULL) {
		isp_dbg_print_err
			("the sensor driver don't support exposure_control\n");
		return RET_FAILURE;
	}

	return sensor_driver->exposure_control(cam_id, new_gain,
			new_integration_time,
			set_gain, set_integration);
};

result_t isp_snr_get_current_exposure(struct isp_context *pcontext,
			uint32 cam_id, uint32 *gain,
			uint32 *integration_time)
{
	struct sensor_operation_t *sensor_driver;

	sensor_driver = isp_snr_get_snr_drv(pcontext, cam_id);
	if (sensor_driver == NULL)
		return RET_FAILURE;

	if (sensor_driver->get_current_exposure == NULL) {
		isp_dbg_print_err
		("the sensor driver don't support get_current_exposure\n");
		return RET_FAILURE;
	}

	return sensor_driver->get_current_exposure(cam_id, gain,
				integration_time);
};

int32 isp_snr_get_analog_gain(struct isp_context *pcontext, uint32 cam_id,
			uint32_t *gain)
{
	struct sensor_operation_t *sensor_driver;

	sensor_driver = isp_snr_get_snr_drv(pcontext, cam_id);
	if (sensor_driver == NULL)
		return RET_FAILURE;

	if (sensor_driver->get_analog_gain == NULL) {
		isp_dbg_print_err
			("the sensor driver don't support get again\n");
		return RET_FAILURE;
	}

	return sensor_driver->get_analog_gain(cam_id, gain);
};

int32 isp_snr_get_itime(struct isp_context *pcontext, uint32 cam_id,
			uint32_t *itime)
{
	struct sensor_operation_t *sensor_driver;

	sensor_driver = isp_snr_get_snr_drv(pcontext, cam_id);
	if (sensor_driver == NULL)
		return RET_FAILURE;

	if (sensor_driver->get_itime == NULL) {
		isp_dbg_print_err
			("the sensor driver don't support get itime\n");
		return RET_FAILURE;
	}

	return sensor_driver->get_itime(cam_id, itime);
};

result_t isp_snr_get_gain_caps(struct isp_context *pcontext, uint32 cam_id,
			struct para_capability_u32 *ana,
			struct para_capability_u32 *dig)
{
	struct sensor_operation_t *sensor_driver;
	uint32 min;
	uint32 max;
	result_t result;

	sensor_driver = isp_snr_get_snr_drv(pcontext, cam_id);
	if (sensor_driver == NULL)
		return RET_FAILURE;

	if (sensor_driver->get_gain_limit == NULL) {
		isp_dbg_print_err
		("the sensor driver don't supported exposure_control\n");
		return RET_FAILURE;
	}

	result = sensor_driver->get_gain_limit(cam_id, &min, &max);
	if (result == RET_SUCCESS) {
		if (ana) {
			ana->min = min;
			ana->max = max;
			ana->step = 1;
		};
		if (dig) {	/*todo */
			dig->min = min;
			dig->max = max;
			dig->step = 1;
		}
	}
	return result;
}

result_t isp_snr_get_itime_caps(struct isp_context *pcontext, uint32 cam_id,
				struct para_capability_u32 *itme)
{
	struct sensor_operation_t *sensor_driver;
	uint32 min;
	uint32 max;
	result_t result;

	sensor_driver = isp_snr_get_snr_drv(pcontext, cam_id);
	if (sensor_driver == NULL)
		return RET_FAILURE;

	if (sensor_driver->get_integration_time_limit == NULL) {
		isp_dbg_print_err
	("the sensor driver don't supported exposure_control\n");
		return RET_FAILURE;
	}

	result = sensor_driver->get_integration_time_limit(cam_id, &min, &max);
	if (result == RET_SUCCESS) {
		if (itme) {
			itme->min = min;
			itme->max = max;
			itme->step = 1;
			itme->def = (min + max) / 2;
		};
	}
	return result;
};

result_t isp_snr_get_hw_parameter(struct isp_context *pcontext,
			uint32 cam_id,
			struct sensor_hw_parameter *para)
{
	struct sensor_operation_t *sensor_driver;
	result_t result;

	sensor_driver = isp_snr_get_snr_drv(pcontext, cam_id);
	if ((para == NULL) || (sensor_driver == NULL))
		return RET_FAILURE;

	if (sensor_driver->get_sensor_hw_parameter == NULL) {
		isp_dbg_print_err
	("the sensor driver don't supported get_sensor_hw_parameter\n");
		para->type = CAMERA_TYPE_RGB_BAYER;
		para->focus_len = 1;
		return RET_FAILURE;
	}

	result = sensor_driver->get_sensor_hw_parameter(cam_id, para);
	if (result == RET_SUCCESS) {
		if (para->focus_len == 0)
			para->focus_len = 1;
	};
	return result;
}

result_t isp_snr_get_runtime_fps(struct isp_context *pcontext,
				 uint32 cam_id, uint16 hi_value,
				 uint16 lo_value, float *fps)
{
	struct sensor_operation_t *sensor_driver;

	sensor_driver = isp_snr_get_snr_drv(pcontext, cam_id);
	if ((fps == NULL) || (sensor_driver == NULL))
		return RET_FAILURE;

	if (sensor_driver->get_runtime_fps == NULL) {
		isp_dbg_print_err
	("the sensor driver don't supported %s\n", __func__);
		return RET_FAILURE;
	}

	return sensor_driver->get_runtime_fps(cam_id, hi_value, lo_value, fps);
}

result_t isp_snr_get_fralenline_regaddr(struct isp_context *pcontext,
					uint32 cam_id, uint16 *hi_addr,
					uint16 *lo_addr)
{
	struct sensor_operation_t *sensor_driver;

	sensor_driver = isp_snr_get_snr_drv(pcontext, cam_id);
	if ((hi_addr == NULL) || (lo_addr == NULL) || (sensor_driver == NULL))
		return RET_FAILURE;

	if (sensor_driver->get_fralenline_regaddr == NULL) {
		isp_dbg_print_err
	("the sensor driver don't supported %s\n", __func__);
		return RET_FAILURE;
	}

	return sensor_driver->get_fralenline_regaddr(cam_id, hi_addr, lo_addr);
}

result_t isp_snr_get_emb_prop(struct isp_context *pcontext,
			uint32 cam_id, bool *supported,
			SensorEmbProp_t *embProp)
{
	struct sensor_operation_t *sensor_driver;

	sensor_driver = isp_snr_get_snr_drv(pcontext, cam_id);
	if ((supported == NULL) || (embProp == NULL) || (sensor_driver == NULL))
		return RET_FAILURE;

	if (sensor_driver->get_emb_prop == NULL) {
		isp_dbg_print_err
		("the sensor driver don't supported %s\n", __func__);
		return RET_FAILURE;
	}

	return sensor_driver->get_emb_prop(cam_id, supported, embProp);
}
