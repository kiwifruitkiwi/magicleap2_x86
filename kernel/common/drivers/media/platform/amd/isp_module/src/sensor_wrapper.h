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

#ifndef _SENSOR_H_
#define _SENSOR_H_

#include "os_base_type.h"
#include "isp_fw_if/drv_isp_if.h"
#include "sensor_if.h"

struct isp_context;
struct para_capability_u32;
struct sensor_operation_t *isp_snr_get_snr_drv(struct isp_context *pcontext,
			uint32 cam_id);
result_t isp_snr_pwr_set(struct isp_context *pcontext, uint32 cam_id,
			bool_t on);
result_t isp_snr_rst(struct isp_context *pcontext, uint32 cam_id);
result_t isp_snr_cfg(struct isp_context *pcontext, uint32 cam_id,
			uint32 res_fps_id, uint32 flag);
result_t isp_snr_start(struct isp_context *isp, uint32 cam_id);
result_t isp_snr_stop(struct isp_context *pcontext, uint32 cam_id);
result_t isp_snr_stream_on(struct isp_context *pcontext, uint32 camera_id);
result_t isp_snr_get_caps(struct isp_context *pcontext, uint32 cam_id,
			isp_sensor_prop_t *caps);
result_t isp_snr_exposure_ctrl(struct isp_context *pcontext, uint32 cam_id,
			uint32 new_gain,
			uint32 new_integration_time,
			uint32 *set_gain, uint32 *set_integration);
result_t isp_snr_get_current_exposure(struct isp_context *pcontext,
			uint32 cam_id, uint32 *gain,
			uint32 *integration_time);
int32 isp_snr_get_analog_gain(struct isp_context *pcontext, uint32 cam_id,
			uint32_t *gain);
int32 isp_snr_get_itime(struct isp_context *pcontext, uint32 cam_id,
			uint32_t *gain);
result_t isp_snr_get_gain_caps(struct isp_context *pcontext, uint32 cam_id,
			struct para_capability_u32 *ana,
			struct para_capability_u32 *dig);
result_t isp_snr_get_itime_caps(struct isp_context *pcontext, uint32 cam_id,
				struct para_capability_u32 *itme);
result_t isp_snr_get_hw_parameter(struct isp_context *pcontext,
			uint32 cam_id,
			struct sensor_hw_parameter *para);
result_t isp_snr_get_runtime_fps(struct isp_context *pcontext, uint32 cam_id,
			uint16 hi_value, uint16 lo_value, float *fps);
result_t isp_snr_get_fralenline_regaddr(struct isp_context *pcontext,
					uint32 cam_id, uint16 *hi_addr,
					uint16 *lo_addr);
result_t isp_snr_get_emb_prop(struct isp_context *pcontext, uint32 cam_id,
			bool *supported, SensorEmbProp_t *embProp);
result_t start_internal_synopsys_phy(struct isp_context *pcontext,
			uint32 cam_id,
			uint32 bitrate /*in Mbps */,
			uint32 lane_num);
result_t shutdown_internal_synopsys_phy(uint32_t device_id, uint32_t lane_num);

#endif
