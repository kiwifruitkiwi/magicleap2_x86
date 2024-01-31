/*
 * Copyright (C) 2019-2021 Advanced Micro Devices, Inc. All rights reserved.
 */

#ifndef _SENSOR_H_
#define _SENSOR_H_

#include "os_base_type.h"
#include "isp_fw_if/drv_isp_if.h"
#include "sensor_if.h"

struct isp_context;
struct para_capability_u32;
struct sensor_operation_t *isp_snr_get_snr_drv(
		struct isp_context *pcontext,
		unsigned int cam_id);
int isp_snr_pwr_set(struct isp_context *pcontext, unsigned int cam_id,
			int on);
int isp_snr_rst(struct isp_context *pcontext, unsigned int cam_id);
int isp_snr_cfg(struct isp_context *pcontext, unsigned int cam_id,
			unsigned int res_fps_id, unsigned int flag);
int isp_snr_start(struct isp_context *isp, unsigned int cam_id);
int isp_snr_stop(struct isp_context *pcontext, unsigned int cam_id);
int isp_snr_stream_on(struct isp_context *pcontext,
		unsigned int camera_id);
int isp_snr_get_caps(struct isp_context *pcontext, unsigned int cam_id,
		     int prf_id, struct _isp_sensor_prop_t *caps);
int isp_snr_exposure_ctrl(struct isp_context *pcontext,
		unsigned int cam_id,
		unsigned int new_gain,
		unsigned int new_integration_time,
		unsigned int *set_gain, unsigned int *set_integration);
int isp_snr_get_current_exposure(struct isp_context *pcontext,
			unsigned int cam_id, unsigned int *gain,
			unsigned int *integration_time);
int isp_snr_get_analog_gain(struct isp_context *pcontext,
		unsigned int cam_id,
		uint32_t *gain);
int isp_snr_get_itime(struct isp_context *pcontext, unsigned int cam_id,
			uint32_t *gain);
int isp_snr_get_gain_caps(struct isp_context *pcontext,
		unsigned int cam_id,
		struct para_capability_u32 *ana,
		struct para_capability_u32 *dig);
int isp_snr_get_itime_caps(struct isp_context *pcontext,
		unsigned int cam_id,
		struct para_capability_u32 *itme);
int isp_snr_get_hw_parameter(struct isp_context *pcontext,
			unsigned int cam_id,
			struct sensor_hw_parameter *para);
int isp_snr_get_fralenline_regaddr(struct isp_context *pcontext,
	unsigned int cam_id, unsigned short *hi_addr,
	unsigned short *lo_addr);
int isp_snr_get_emb_prop(struct isp_context *pcontext,
	unsigned int cam_id,
	bool *supported, struct _SensorEmbProp_t *embProp);
int isp_snr_set_test_pattern(struct isp_context *pcontext,
			unsigned int cam_id,
			enum sensor_test_pattern pattern);
int isp_snr_get_m2m_data(struct isp_context *pcontext,
			unsigned int cam_id,
			struct _M2MTdb_t *pM2MTdb);

#endif
