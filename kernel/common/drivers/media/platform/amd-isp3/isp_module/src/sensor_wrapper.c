/*
 * Copyright (C) 2019-2021 Advanced Micro Devices, Inc. All rights reserved.
 */

#include "isp_common.h"
#include "sensor_wrapper.h"
#include "log.h"
#include "hal.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "[ISP][sensor_wrapper]"

void reset_sensor_phy_fpga(unsigned int device_id)
{
	unsigned int reg_value;

	ENTER();

	if (device_id == 0) {
		//For PHY reset
		// FPGA TO CONTROL
		//phy0_cs 0, phy0_resx 0 cam0_pwdn 0, cam0_resetb 0
		isp_hw_reg_write32(0xc00, 0x0);
		msleep(20);//msleep(1);
		//phy0_cs 1, phy0_resx 0 cam0_pwdn 0, cam0_resetb 0
		isp_hw_reg_write32(0xc00, 0x1);
		msleep(20);//msleep(1);
		//phy0_cs 1, phy0_resx 1 cam0_pwdn 0, cam0_resetb 0
		isp_hw_reg_write32(0xc00, 0x3);
		msleep(20);//msleep(1);

		//ISP INTERNAL REGISTER CONTROL
		//For sensor reset
		reg_value = isp_hw_reg_read32(mmISP_GPIO_SHUTDOWN_SENSOR0);
		reg_value &= (~(0x1 << 4));
		isp_hw_reg_write32(mmISP_GPIO_SHUTDOWN_SENSOR0, reg_value);
		msleep(20);//msleep(1);
		reg_value |= (0x1 << 4);
		isp_hw_reg_write32(mmISP_GPIO_SHUTDOWN_SENSOR0, reg_value);
		msleep(20);//msleep(5);

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
		ISP_PR_ERR("device_id %d unsupported\n", device_id);
	}

	EXIT();
}

void reset_sensor_shutdown(unsigned int device_id)
{
	unsigned int reg_value;

	ENTER();

	if (device_id == 0) {
		//ISP INTERNAL REGISTER CONTROL
		//For sensor reset
		reg_value = isp_hw_reg_read32(mmISP_GPIO_SHUTDOWN_SENSOR0);
		reg_value &= (~(0x1 << 4));
		//cam0_pwdn 0
		isp_hw_reg_write32(mmISP_GPIO_SHUTDOWN_SENSOR0, reg_value);
		usleep_range(5000, 6000);
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
		ISP_PR_ERR("device_id %d unsupported\n", device_id);
	}

	EXIT();
}

struct sensor_operation_t *isp_snr_get_snr_drv(
			struct isp_context *pcontext,
			unsigned int camera_id)
{
	if ((pcontext == NULL) || (camera_id >= CAMERA_ID_MAX)) {
		ISP_PR_ERR("illegal parameter %d\n", camera_id);
		return NULL;
	}
	/*return &pcontext->sensor_ops[camera_id]; */
	return pcontext->p_sensor_ops[camera_id];
}

struct vcm_operation_t *isp_snr_get_vcm_drv(
			struct isp_context *pcontext,
			unsigned int camera_id)
{
	if ((pcontext == NULL) || (camera_id >= CAMERA_ID_MAX)) {
		ISP_PR_ERR("illegal parameter %d\n", camera_id);
		return NULL;
	}

	return pcontext->p_vcm_ops[camera_id];
}

//refer to reset_sensor_phy
int isp_snr_pwr_set(struct isp_context *pcontext, unsigned int camera_id,
			 int on)
{
	struct sensor_operation_t *sensor_driver;
	int ret;
	struct isp_pwr_unit *pwr_unit;
	unsigned int lane_num;

	pwr_unit = &pcontext->isp_pu_cam[camera_id];
	if (on && (pwr_unit->pwr_status == ISP_PWR_UNIT_STATUS_ON)) {
		ret = RET_SUCCESS;
		ISP_PR_INFO("pwr on suc by do none,cam_id %d,on:%u\n",
			camera_id, on);
		goto quit;
	}

	if (!on && (pwr_unit->pwr_status == ISP_PWR_UNIT_STATUS_OFF)) {
		ret = RET_SUCCESS;
		ISP_PR_INFO("pwr off suc by do none,cam_id %d,on:%u\n",
			camera_id, on);
		goto quit;
	}

	sensor_driver = isp_snr_get_snr_drv(pcontext, camera_id);
	if (sensor_driver == NULL) {
		ret = RET_FAILURE;
		ISP_PR_ERR("no snr drv,cam_id %d,on:%u\n",
			camera_id, on);
		goto quit;
	}

	if (sensor_driver->poweron_sensor == NULL) {
		ret = RET_FAILURE;
		ISP_PR_ERR("fail for no pwr func,cam_id %d,on:%u\n",
			camera_id, on);
		goto quit;
	}

	lane_num =
	pcontext->sensor_info[camera_id].sensor_cfg.prop.intfProp.mipi.numLanes;

	ISP_PR_DBG("cam_id  %u, on:%u,isfpga %u,laneNum %u\n",
			camera_id, on, isp_is_fpga(), lane_num);

	if (g_prop->cvip_enable == CVIP_STATUS_ENABLE) {
		ret = RET_SUCCESS;
	} else {
		reset_sensor_phy(camera_id, (bool)on, isp_is_fpga(), lane_num);
		ret = sensor_driver->poweron_sensor(camera_id, (bool)on);
	}

	if (ret == RET_SUCCESS) {
		ISP_PR_INFO("sensor poweron suc\n");
		if (on) {
			pwr_unit->pwr_status = ISP_PWR_UNIT_STATUS_ON;
			isp_get_cur_time_tick(&pwr_unit->on_time);
			pwr_unit->idle_start_time =
				pwr_unit->on_time + MS_TO_TIME_TICK(2000);
		} else {
			pwr_unit->pwr_status = ISP_PWR_UNIT_STATUS_OFF;
		}
	} else {
		ISP_PR_ERR("sensor poweron fail\n");
	}

	ISP_PR_PC("cam[%u] power %s suc\n",
		camera_id, (on == 1) ? "on" : "off");

quit:
	if (!on)
		pwr_unit->pwr_status = ISP_PWR_UNIT_STATUS_OFF;
	return ret;
}

int isp_snr_rst(struct isp_context *pcontext, unsigned int camera_id)
{
	struct sensor_operation_t *sensor_driver;

	sensor_driver = isp_snr_get_snr_drv(pcontext, camera_id);
	if (sensor_driver == NULL)
		return RET_FAILURE;

	if (sensor_driver->reset_sensor == NULL) {
		ISP_PR_ERR("reset_sensor not supported\n");
		return RET_FAILURE;
	}

	if (g_isp_env_setting != ISP_ENV_SILICON) {
		reset_sensor_phy_fpga(camera_id);
	} else {
		reset_sensor_shutdown(camera_id);
	};

	return sensor_driver->reset_sensor(camera_id);
}

int isp_snr_cfg(struct isp_context *pcontext, unsigned int camera_id,
			unsigned int res_fps_id, unsigned int flag)
{
	struct sensor_operation_t *sensor_driver;
	int result;

	sensor_driver = isp_snr_get_snr_drv(pcontext, camera_id);
	if (sensor_driver == NULL)
		return RET_FAILURE;

	if (sensor_driver->config_sensor == NULL) {
		ISP_PR_ERR("config_sensor not supported\n");
		return RET_FAILURE;
	}

	if (g_prop->cvip_enable == CVIP_STATUS_ENABLE)
		flag |= CAMERA_FLAG_NOT_ACCESS_DEV;

	result = sensor_driver->config_sensor(camera_id, res_fps_id, flag);
	if (result != RET_SUCCESS)
		ISP_PR_ERR("config sensor[%d] failed\n", camera_id)
	else
		ISP_PR_PC("cam[%d] config suc!\n", camera_id);

	return result;
}

int isp_snr_start(struct isp_context *pcontext, unsigned int camera_id)
{
	struct sensor_operation_t *sensor_driver;
	int result;
	struct sensor_info *sensor_info;

	if ((pcontext == NULL) || (camera_id >= CAMERA_ID_MAX)) {
		ISP_PR_ERR("failed for illegal para %d\n", camera_id);
		return RET_FAILURE;
	};

	ISP_PR_PC("cam[%d] start.\n", camera_id);
	sensor_info = &pcontext->sensor_info[camera_id];
	sensor_driver = isp_snr_get_snr_drv(pcontext, camera_id);
	if (sensor_driver == NULL)
		return RET_FAILURE;

	if (sensor_driver->start_sensor == NULL) {
		ISP_PR_ERR("start_sensor not supported\n");
		return RET_FAILURE;
	}

	if (g_prop->cvip_enable == CVIP_STATUS_ENABLE) {
		result = RET_SUCCESS;
	} else {
		result = sensor_driver->start_sensor(camera_id,
						     g_isp_env_setting ==
						     ISP_ENV_SILICON ?
						     FALSE : TRUE);
	}

	if (result != RET_SUCCESS) {
		//add this line to trick CP
		ISP_PR_ERR("Start sensor[%d] failed\n", camera_id);
	} else {
		//add this line to trick CP
		ISP_PR_PC("cam[%d] start suc!\n", camera_id);
	}
	return result;
}

int isp_snr_stop(struct isp_context *pcontext, unsigned int camera_id)
{
	struct sensor_operation_t *sensor_driver;
	int result;

	sensor_driver = isp_snr_get_snr_drv(pcontext, camera_id);
	if (sensor_driver == NULL)
		return RET_FAILURE;

	if (sensor_driver->stop_sensor == NULL) {
		ISP_PR_ERR("stop_sensor not supported\n");
		return RET_FAILURE;
	}

	result = sensor_driver->stop_sensor(camera_id);
	if (result != RET_SUCCESS) {
		//add this line to trick CP
		ISP_PR_ERR("Stop sensor[%d] failed\n", camera_id);
	} else {
		//add this line to trick CP
		ISP_PR_PC("cam[%d] stop suc!\n", camera_id);
	}

	return result;
}

int isp_snr_stream_on(struct isp_context *pcontext,
	unsigned int camera_id)
{
	struct sensor_operation_t *sensor_driver;

	sensor_driver = isp_snr_get_snr_drv(pcontext, camera_id);
	if (sensor_driver == NULL)
		return RET_FAILURE;

	if (sensor_driver->stream_on_sensor == NULL) {
		ISP_PR_ERR("stream_on_sensor not supported\n");
		return RET_FAILURE;
	}

	return sensor_driver->stream_on_sensor(camera_id);
}

int isp_snr_get_caps(struct isp_context *pcontext, unsigned int camera_id,
		     int prf_id, struct _isp_sensor_prop_t *caps)
{
	struct sensor_operation_t *sensor_driver;

	sensor_driver = isp_snr_get_snr_drv(pcontext, camera_id);
	if (sensor_driver == NULL)
		return RET_FAILURE;

	if (sensor_driver->get_caps == NULL) {
		ISP_PR_ERR("get_caps not supported\n");
		return RET_FAILURE;
	}

	return sensor_driver->get_caps(camera_id, prf_id, caps);
}

int isp_snr_exposure_ctrl(struct isp_context *pcontext,
			unsigned int cam_id,
			unsigned int new_gain,
			unsigned int new_integration_time,
			unsigned int *set_gain, unsigned int *set_integration)
{
	struct sensor_operation_t *sensor_driver;

	sensor_driver = isp_snr_get_snr_drv(pcontext, cam_id);
	if (sensor_driver == NULL)
		return RET_FAILURE;

	if (sensor_driver->exposure_control == NULL) {
		ISP_PR_ERR("exposure_control not supported\n");
		return RET_FAILURE;
	}

	return sensor_driver->exposure_control(cam_id, new_gain,
			new_integration_time,
			set_gain, set_integration);
};

int isp_snr_get_current_exposure(struct isp_context *pcontext,
			unsigned int cam_id, unsigned int *gain,
			unsigned int *integration_time)
{
	struct sensor_operation_t *sensor_driver;

	sensor_driver = isp_snr_get_snr_drv(pcontext, cam_id);
	if (sensor_driver == NULL)
		return RET_FAILURE;

	if (sensor_driver->get_current_exposure == NULL) {
		ISP_PR_ERR("get_current_exposure not supported\n");
		return RET_FAILURE;
	}

	return sensor_driver->get_current_exposure(cam_id, gain,
				integration_time);
};

int isp_snr_get_analog_gain(struct isp_context *pcontext,
			unsigned int cam_id,
			uint32_t *gain)
{
	struct sensor_operation_t *sensor_driver;

	sensor_driver = isp_snr_get_snr_drv(pcontext, cam_id);
	if (sensor_driver == NULL)
		return RET_FAILURE;

	if (sensor_driver->get_analog_gain == NULL) {
		ISP_PR_ERR("get_analog_gain not supported\n");
		return RET_FAILURE;
	}

	return sensor_driver->get_analog_gain(cam_id, gain);
};

int isp_snr_get_itime(struct isp_context *pcontext, unsigned int cam_id,
			uint32_t *itime)
{
	struct sensor_operation_t *sensor_driver;

	sensor_driver = isp_snr_get_snr_drv(pcontext, cam_id);
	if (sensor_driver == NULL)
		return RET_FAILURE;

	if (sensor_driver->get_itime == NULL) {
		ISP_PR_ERR("get_itime not supported\n");
		return RET_FAILURE;
	}

	return sensor_driver->get_itime(cam_id, itime);
};

int isp_snr_get_gain_caps(struct isp_context *pcontext,
			unsigned int cam_id,
			struct para_capability_u32 *ana,
			struct para_capability_u32 *dig)
{
	struct sensor_operation_t *sensor_driver;
	unsigned int min;
	unsigned int max;
	int result;

	sensor_driver = isp_snr_get_snr_drv(pcontext, cam_id);
	if (sensor_driver == NULL)
		return RET_FAILURE;

	if (sensor_driver->get_gain_limit == NULL) {
		ISP_PR_ERR("get_gain_limit not supported\n");
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

int isp_snr_get_itime_caps(struct isp_context *pcontext,
				unsigned int cam_id,
				struct para_capability_u32 *itme)
{
	struct sensor_operation_t *sensor_driver;
	unsigned int min;
	unsigned int max;
	int result;

	sensor_driver = isp_snr_get_snr_drv(pcontext, cam_id);
	if (sensor_driver == NULL)
		return RET_FAILURE;

	if (sensor_driver->get_integration_time_limit == NULL) {
		ISP_PR_ERR("get_integration_time_limit not supported\n");
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

int isp_snr_get_hw_parameter(struct isp_context *pcontext,
			unsigned int cam_id,
			struct sensor_hw_parameter *para)
{
	struct sensor_operation_t *sensor_driver;
	int result;

	sensor_driver = isp_snr_get_snr_drv(pcontext, cam_id);
	if ((para == NULL) || (sensor_driver == NULL))
		return RET_FAILURE;

	if (sensor_driver->get_sensor_hw_parameter == NULL) {
		ISP_PR_ERR("get_sensor_hw_parameter not supported\n");
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

int isp_snr_get_fralenline_regaddr(struct isp_context *pcontext,
					unsigned int cam_id,
					unsigned short *hi_addr,
					unsigned short *lo_addr)
{
	struct sensor_operation_t *sensor_driver;

	sensor_driver = isp_snr_get_snr_drv(pcontext, cam_id);
	if ((hi_addr == NULL) || (lo_addr == NULL) || (sensor_driver == NULL))
		return RET_FAILURE;

	if (sensor_driver->get_fralenline_regaddr == NULL) {
		ISP_PR_ERR("get_fralenline_regadd not supported\n");
		return RET_FAILURE;
	}

	return sensor_driver->get_fralenline_regaddr(cam_id, hi_addr, lo_addr);
}

int isp_snr_get_emb_prop(struct isp_context *pcontext,
			unsigned int cam_id, bool *supported,
			struct _SensorEmbProp_t *embProp)
{
	struct sensor_operation_t *sensor_driver;

	sensor_driver = isp_snr_get_snr_drv(pcontext, cam_id);
	if ((supported == NULL) || (embProp == NULL) ||
	(sensor_driver == NULL))
		return RET_FAILURE;

	if (sensor_driver->get_emb_prop == NULL) {
		ISP_PR_ERR("get_emb_prop not supported\n");
		return RET_FAILURE;
	}

	return sensor_driver->get_emb_prop(cam_id, supported, embProp);
}

int isp_snr_set_test_pattern(struct isp_context *pcontext, unsigned int cam_id,
			     enum sensor_test_pattern pattern)
{
	int result;
	struct sensor_operation_t *sensor_driver;

	sensor_driver = isp_snr_get_snr_drv(pcontext, cam_id);
	if (!sensor_driver)
		return RET_FAILURE;

	if (!sensor_driver->set_test_pattern) {
		ISP_PR_ERR("not supported");
		return RET_FAILURE;
	}

	result = sensor_driver->set_test_pattern(cam_id, pattern);
	if (result != RET_SUCCESS) {
		//add this line to trick CP
		ISP_PR_ERR("cam[%d] failed", cam_id);
	} else {
		//add this line to trick CP
		ISP_PR_PC("cam[%d] suc!", cam_id);
	}

	return result;
}

int isp_snr_get_m2m_data(struct isp_context *pcontext,
			unsigned int cam_id,
			struct _M2MTdb_t *pM2MTdb)
{
	struct sensor_operation_t *sensor_driver;
	int status = 0;

	sensor_driver = isp_snr_get_snr_drv(pcontext, cam_id);
	if (!pM2MTdb || !sensor_driver || !sensor_driver->get_m2m_data)
		status = RET_FAILURE;
	else {
		status = sensor_driver->get_m2m_data(cam_id, pM2MTdb);
		if (status == RET_SUCCESS) {
			ISP_PR_INFO("get_m2m_data success\n");
		} else if (status == RET_NOTSUPP) {
			ISP_PR_INFO("get_m2m_data not supported\n");
		} else {
			ISP_PR_ERR("get_m2m_data fail\n");
			status = RET_FAILURE;
		}
	}
	return status;
}
