/**************************************************************************
 *copyright 2014~2020 advanced micro devices, inc.
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
#include "buffer_mgr.h"
#include "isp_fw_if.h"
#include "log.h"
#include "isp_soc_adpt.h"
#include "isp_module_if_imp.h"

#include "amdgpu_ih.h"
#include "amdgpu_drm.h"
#include "amd_stream.h"

struct isp_context g_isp_context;
EXPORT_SYMBOL(g_isp_context);

struct isp_fw_resp_thread_para isp_resp_para[MAX_REAL_FW_RESP_STREAM_NUM];
struct hwip_to_fw_name_mapping isp_hwip_to_fw_name[] = {
	{2, 0, 0, "atiisp_rv.dat"},
	{2, 0, 1, "atiisp_rv.dat"}
};

struct isp_registry_name_mapping
	g_isp_registry_name_mapping[MAX_ISP_REGISTRY_PARA_NUM] = {
	// VALUE INDEX          | REGISTRY VALUE NAME  | DEFAULT VALUE
	{ENABLE_SWISP_FWLOADING, "EnableSwISPFWLoading", 0},
};
uint32 g_rgbir_raw_count_in_fw;
AeIso_t ISO_MAX_VALUE;
AeIso_t ISO_MIN_VALUE;
AeEv_t EV_MAX_VALUE;
AeEv_t EV_MIN_VALUE;
AeEv_t EV_STEP_VALUE;
uint32 g_num_i2c;

#define aidt_hal_write_reg(addr, value) \
	isp_hw_reg_write32(((uint32)(addr)), value)
#define aidt_hal_read_reg(addr) isp_hw_reg_read32(((uint32)(addr)))
#define MODIFYFLD(var, reg, field, val) \
	(var = (var & (~reg##__##field##_MASK)) | (((uint32)(val) << \
	reg##__##field##__SHIFT) & reg##__##field##_MASK))

enum isp_environment_setting g_isp_env_setting = ISP_ENV_SILICON;

SensorId_t get_i_s_p_sensor_id_for_sensor_id(enum camera_id cam_id)
{
	switch (cam_id) {
	case CAMERA_ID_REAR:
		return SENSOR_ID_A;
	case CAMERA_ID_FRONT_LEFT:
		return SENSOR_ID_B;
	case CAMERA_ID_FRONT_RIGHT:
		return SENSOR_ID_B;
	default:
		return SENSOR_ID_MAX;
	};
}

void isp_reset_str_info(struct isp_context *isp, enum camera_id cid,
			enum stream_id sid, bool pause)
{
	struct sensor_info *sif;
	struct isp_stream_info *str_info;

	if (!is_para_legal(isp, cid) || (sid > STREAM_ID_NUM))
		return;
	sif = &isp->sensor_info[cid];
	str_info = &sif->str_info[sid];
	if (!pause) {
		str_info->format = PVT_IMG_FMT_NV21;//PVT_IMG_FMT_INVALID;
		str_info->width = 0;
		str_info->height = 0;
		str_info->luma_pitch_set = 0;
		str_info->chroma_pitch_set = 0;
		str_info->max_fps_numerator = 0;
		str_info->max_fps_denominator = 0;
		str_info->is_perframe_ctl = false;
	}
	str_info->start_status = START_STATUS_NOT_START;
}

void isp_reset_camera_info(struct isp_context *isp, enum camera_id cid)
{
	struct sensor_info *info;
	enum stream_id str_id;

	if (!is_para_legal(isp, cid))
		return;
	info = &isp->sensor_info[cid];

	info->cid = cid;
	//memset(&info->sensor_cfg, 0, sizeof(info->sensor_cfg));
	info->status = START_STATUS_NOT_START;
	info->aec_mode = AE_MODE_AUTO;
	info->af_mode = AF_MODE_CONTINUOUS_VIDEO;
	info->flash_assist_mode = FOCUS_ASSIST_MODE_OFF;
	info->flash_assist_pwr = 20;
	memset(&info->lens_range, 0, sizeof(info->lens_range));
	info->awb_mode = AWB_MODE_AUTO;
	info->flick_type_set = AE_FLICKER_TYPE_50HZ;
	info->flick_type_cur = AE_FLICKER_TYPE_50HZ;
	info->para_iso_set.iso = AE_ISO_AUTO;
	info->para_iso_cur.iso = AE_ISO_AUTO;
	memset(&info->ae_roi, 0, sizeof(info->ae_roi));
	memset(&info->af_roi, 0, sizeof(info->af_roi));
	memset(&info->awb_region, 0, sizeof(info->awb_region));
	info->flash_mode = FLASH_MODE_AUTO;
	info->flash_pwr = 80;
	info->redeye_mode = RED_EYE_MODE_OFF;
	info->color_temperature = 0;	/*1000; */
	info->asprw_wind.h_offset = 0;
	info->asprw_wind.v_offset = 0;
	info->asprw_wind.h_size = 0;
	info->asprw_wind.v_size = 0;
	for (str_id = STREAM_ID_PREVIEW; str_id <= STREAM_ID_NUM; str_id++)
		isp_reset_str_info(isp, cid, str_id, false);

	info->zoom_info.h_offset = 0;
	info->zoom_info.v_offset = 0;
	info->zoom_info.ratio = ZOOM_DEF_VALUE;
	info->cur_res_fps_id = 1;
	/*info->cproc_enable = 0; */
	info->para_brightness_cur = BRIGHTNESS_DEF_VALUE;
	info->para_brightness_set = BRIGHTNESS_DEF_VALUE;
	info->para_contrast_cur = CONTRAST_DEF_VALUE;
	info->para_contrast_set = CONTRAST_DEF_VALUE;
	info->para_hue_cur = HUE_DEF_VALUE;
	info->para_hue_set = HUE_DEF_VALUE;
	info->para_satuaration_cur = SATURATION_DEF_VALUE;
	info->para_satuaration_set = SATURATION_DEF_VALUE;
	info->para_color_enable_cur = 1;
	info->para_color_enable_set = 1;
	info->calib_enable = true;
	info->tnr_enable = false;	/*true; */
	info->start_str_cmd_sent = false;
	info->stream_id = FW_CMD_RESP_STREAM_ID_MAX;
	//info->cproc_value = NULL;
	info->wb_idx = NULL;
	info->scene_tbl = NULL;
	info->fps_tab.fps = 0;
	info->fps_tab.HighValue = 0;
	info->fps_tab.LowValue = 0;
	info->fps_tab.HighAddr = (uint16) I2C_REGADDR_NULL;
	info->fps_tab.LowAddr = (uint16) I2C_REGADDR_NULL;
	info->enable_dynamic_frame_rate = TRUE;
	info->face_auth_mode = FALSE;
}

void destroy_all_camera_info(struct isp_context *pisp_context)
{
#ifndef USING_KGD_CGS
	enum camera_id cam_id;
	struct sensor_info *info;
	struct isp_stream_info *str_info;
	uint32 sid;

	for (cam_id = CAMERA_ID_REAR; cam_id < CAMERA_ID_MAX; cam_id++) {
		info = &pisp_context->sensor_info[cam_id];
		isp_list_destroy(&info->take_one_pic_buf_list, NULL);
		isp_list_destroy(&info->rgbir_raw_output_in_fw, NULL);
		isp_list_destroy(&info->rgbraw_input_in_fw, NULL);
		isp_list_destroy(&info->irraw_input_in_fw, NULL);
		isp_list_destroy(&info->rgbir_frameinfo_input_in_fw, NULL);
		info->take_one_pic_left_cnt = 0;
		isp_list_destroy(&info->take_one_raw_buf_list, NULL);
		info->take_one_raw_left_cnt = 0;
		isp_list_destroy(&info->meta_data_free, NULL);
		isp_list_destroy(&info->meta_data_in_fw, NULL);
		for (sid = STREAM_ID_PREVIEW; sid <= STREAM_ID_NUM; sid++) {
			str_info = &info->str_info[sid];
			isp_list_destroy(&str_info->buf_free, NULL);
			isp_list_destroy(&str_info->buf_in_fw, NULL);
		};
	};
#endif
}

void init_all_camera_info(struct isp_context *pisp_context)
{
	enum camera_id cam_id;
	struct sensor_info *info;
	struct isp_stream_info *str_info;
	uint32 sid;

	for (cam_id = CAMERA_ID_REAR; cam_id < CAMERA_ID_MAX; cam_id++) {
		info = &pisp_context->sensor_info[cam_id];
		isp_reset_camera_info(pisp_context, cam_id);
		info->raw_width = 0;
		info->raw_height = 0;

		isp_event_init(&info->resume_success_evt, 1, 1);
		isp_list_init(&info->take_one_pic_buf_list);
		isp_list_init(&info->rgbir_raw_output_in_fw);
		isp_list_init(&info->rgbraw_input_in_fw);
		isp_list_init(&info->irraw_input_in_fw);
		isp_list_init(&info->rgbir_frameinfo_input_in_fw);
		info->take_one_pic_left_cnt = 0;
		isp_list_init(&info->take_one_raw_buf_list);
		info->take_one_raw_left_cnt = 0;
		for (sid = STREAM_ID_PREVIEW; sid <= STREAM_ID_NUM; sid++) {
			str_info = &info->str_info[sid];
			isp_list_init(&str_info->buf_free);
			isp_list_init(&str_info->buf_in_fw);
		};
	};
}

void init_all_camera_dev_info(struct isp_context *isp)
{
	struct camera_dev_info *info;

	info = &isp->cam_dev_info[CAMERA_ID_REAR];
	strncpy(info->cam_name, "imx208", CAMERA_DEVICE_NAME_LEN);
	info->type = CAMERA_TYPE_RGB_BAYER;
	info->focus_len = 1;

	info = &isp->cam_dev_info[CAMERA_ID_FRONT_LEFT];
	strncpy(info->cam_name, "imx208", CAMERA_DEVICE_NAME_LEN);
	info->type = CAMERA_TYPE_RGB_BAYER;
	info->focus_len = 1;

	info = &isp->cam_dev_info[CAMERA_ID_FRONT_RIGHT];
	strncpy(info->cam_name, "imx208", CAMERA_DEVICE_NAME_LEN);
	info->type = CAMERA_TYPE_RGB_BAYER;
	info->focus_len = 1;

//	info = &isp->cam_dev_info[CAMERA_ID_MEM];
//	strncpy(info->cam_name, "mem", CAMERA_DEVICE_NAME_LEN);
//	info->type = CAMERA_TYPE_MEM;
//	info->focus_len = 1;
};

void isp_init_drv_setting(struct isp_context *isp)
{
	isp->drv_settings.sensor_frame_rate = 255;
	isp->drv_settings.rear_sensor_type = -1;
	isp->drv_settings.frontl_sensor_type = -1;
	isp->drv_settings.frontr_sensor_type = -1;
	isp->drv_settings.af_default_auto = -1;
	isp->drv_settings.ae_default_auto = -1;
	isp->drv_settings.awb_default_auto = -1;
	isp->drv_settings.fw_ctrl_3a = -1;
	isp->drv_settings.set_again_by_sensor_drv = -1;
	isp->drv_settings.set_itime_by_sensor_drv = -1;
	isp->drv_settings.set_lens_pos_by_sensor_drv = -1;
	isp->drv_settings.do_fpga_to_sys_mem_cpy = -1;
	isp->drv_settings.hdmi_support_enable = 0;
	isp->drv_settings.fw_log_cfg_en = 0;
	isp->drv_settings.fw_log_cfg_A = 0;
	isp->drv_settings.fw_log_cfg_B = 0;
	isp->drv_settings.fw_log_cfg_C = 0;
	isp->drv_settings.fw_log_cfg_D = 0;
	isp->drv_settings.fw_log_cfg_E = 0;
	isp->drv_settings.fix_wb_lighting_index = -1;
	isp->drv_settings.enable_stage2_temp_buf = 0;
	isp->drv_settings.ignore_meta_at_stage2 = 0;
	isp->drv_settings.driver_load_fw = 0;
	isp->drv_settings.disable_dynamic_aspect_wnd = 0;
	isp->drv_settings.tnr_enable = 0;
	isp->drv_settings.snr_enable = 0;
	isp->drv_settings.vfr_enable = 0;
	isp->drv_settings.low_light_fps = 15;
	isp->drv_settings.enable_high_video_capabilities = 0;
	isp->drv_settings.dpf_enable = 1;
	isp->drv_settings.output_metadata_in_log = 0;
	isp->drv_settings.demosaic_thresh = 256;
	isp->drv_settings.enable_frame_drop_solution_for_frame_pair = 0;
	isp->drv_settings.disable_ir_illuminator = 0;
	isp->drv_settings.ir_illuminator_ctrl = 2;
	isp->drv_settings.min_ae_roi_size_filter = 0;
	isp->drv_settings.reset_small_ae_roi = 0;
	isp->drv_settings.process_3a_by_host = 0;
	isp->drv_settings.drv_log_level = 3;
	isp->drv_settings.enable_using_fb_for_taking_raw = 0;
}

void isp_sw_init_capabilities(struct isp_context *pisp_context)
{
	uint32 i;
	struct isp_capabilities *cap;

	cap = &pisp_context->isp_cap;
	for (i = 0; i < CAMERA_ID_MAX; i++) {
		memset(&pisp_context->photo_seq_info[i], 0,
			sizeof(struct photo_seque_info));
		pisp_context->photo_seq_info[i].max_history_frame_cnt =
			MAX_PHOTO_SEQUENCE_HOSTORY_FRAME_COUNT;
		pisp_context->photo_seq_info[i].max_fps =
			MAX_PHOTO_SEQUENCE_FPS;
		pisp_context->photo_seq_info[i].running = false;
	}

	/*todo: get from CMD_ID_AE_GET_EV_CAPABILITY
	 *
	 *cap->ev_compensation.min = EVCOMPENSATION_MIN_VALUE;
	 *cap->ev_compensation.max = EVCOMPENSATION_MAX_VALUE;
	 *cap->ev_compensation.step = EVCOMPENSATION_STEP_VALUE;
	 *cap->ev_compensation.def = EVCOMPENSATION_DEF_VALUE;
	 */

	cap->brightness.min = BRIGHTNESS_MIN_VALUE;
	cap->brightness.max = BRIGHTNESS_MAX_VALUE;
	cap->brightness.step = BRIGHTNESS_STEP_VALUE;
	cap->brightness.def = BRIGHTNESS_DEF_VALUE;

	cap->contrast.min = CONTRAST_MIN_VALUE;
	cap->contrast.max = CONTRAST_MAX_VALUE;
	cap->contrast.step = CONTRAST_STEP_VALUE;
	cap->contrast.def = CONTRAST_DEF_VALUE;

	cap->gamma.min = GAMA_MIN_VALUE;
	cap->gamma.max = GAMA_MAX_VALUE;
	cap->gamma.step = GAMA_STEP_VALUE;
	cap->gamma.def = GAMA_DEF_VALUE;

	cap->hue.min = HUE_MIN_VALUE;
	cap->hue.max = HUE_MAX_VALUE;
	cap->hue.step = HUE_STEP_VALUE;
	cap->hue.def = HUE_DEF_VALUE;

	cap->saturation.min = SATURATION_MIN_VALUE;
	cap->saturation.max = SATURATION_MAX_VALUE;
	cap->saturation.step = SATURATION_STEP_VALUE;
	cap->saturation.def = SATURATION_DEF_VALUE;

	cap->sharpness.min = SHARPNESS_MIN_VALUE;
	cap->sharpness.max = SHARPNESS_MAX_VALUE;
	cap->sharpness.step = SHARPNESS_STEP_VALUE;
	cap->sharpness.def = SHARPNESS_DEF_VALUE;

	/*todo: get from CMD_ID_AE_GET_ISO_CAPABILITY
	 *
	 *cap->iso.min = ISO_MIN_VALUE;
	 *cap->iso.max = ISO_MAX_VALUE;
	 *cap->iso.step = ISO_STEP_VALUE;
	 *cap->iso.def = ISO_DEF_VALUE;
	 */

	cap->zoom.min = ZOOM_MIN_VALUE;
	cap->zoom.max = ZOOM_MAX_VALUE;
	cap->zoom.step = ZOOM_STEP_VALUE;
	cap->zoom.def = ZOOM_DEF_VALUE;

	cap->zoom_h_off.min = ZOOM_MIN_H_OFF;
	cap->zoom_h_off.max = ZOOM_MAX_H_OFF;
	cap->zoom_h_off.step = ZOOM_H_OFF_STEP_VALUE;
	cap->zoom_h_off.def = ZOOM_H_OFF_DEF_VALUE;

	cap->zoom_v_off.min = ZOOM_MIN_V_OFF;
	cap->zoom_v_off.max = ZOOM_MAX_V_OFF;
	cap->zoom_v_off.step = ZOOM_V_OFF_STEP_VALUE;
	cap->zoom_v_off.def = ZOOM_V_OFF_DEF_VALUE;
	/*real gain value multiple 1000 */
	/*cap->gain; */
	/*real integration time multiple 1000000 */
	/*cap->itime; */
}

result_t isp_start_resp_proc_threads(struct isp_context *isp)
{
	uint32 i;

	isp_dbg_print_info("-> %s\n", __func__);
	for (i = 0; i < MAX_REAL_FW_RESP_STREAM_NUM; i++) {
		isp_resp_para[i].idx = i;
		isp_resp_para[i].isp = isp;
		if (create_work_thread(&isp->fw_resp_thread[i],
		isp_fw_resp_thread_wrapper, &isp_resp_para[i]) !=
		RET_SUCCESS) {
			isp_dbg_print_err("In %s, [%u]fail\n", __func__, i);
			goto fail;
		}
	}
	isp_dbg_print_info("<- %s suc\n", __func__);
	return RET_SUCCESS;
fail:
	isp_stop_resp_proc_threads(isp);
	isp_dbg_print_err("<- %s fail\n", __func__);
	return RET_FAILURE;
};

result_t isp_stop_resp_proc_threads(struct isp_context *isp)
{
	uint32 i;

	isp_dbg_print_info("-> %s\n", __func__);
	for (i = 0; i < MAX_REAL_FW_RESP_STREAM_NUM; i++)
		stop_work_thread(&isp->fw_resp_thread[i]);

	isp_dbg_print_info("<- %s suc\n", __func__);
	return RET_SUCCESS;
};

int isp_isr(void *private_data, unsigned int src_id,
		const uint32_t *iv_entry)
{
	struct isp_context *isp = (struct isp_context *)(private_data);
	uint64 irq_enable_id = 0;
	int32 idx = -1;

	isp_dbg_print_info("-><- %s\n", __func__);

	if (private_data == NULL) {
		isp_dbg_print_info("-><- %s, bad para, %p\n",
			__func__, private_data);
		return -1;
	};

	switch (src_id) {
	case IRQ_SOURCE_ISP_RINGBUFFER_WPT5:
		irq_enable_id = isp->irq_enable_id[0];
		idx = 0;
		break;
	case IRQ_SOURCE_ISP_RINGBUFFER_WPT6:
		irq_enable_id = isp->irq_enable_id[1];
		idx = 1;
		break;
	case IRQ_SOURCE_ISP_RINGBUFFER_WPT7:
		irq_enable_id = isp->irq_enable_id[2];
		idx = 2;
		break;
	case IRQ_SOURCE_ISP_RINGBUFFER_WPT8:
		irq_enable_id = isp->irq_enable_id[3];
		idx = 3;
		break;
	default:
		break;
	};
	if (-1 == idx) {
		isp_dbg_print_info("-><- %s, fail bad irq src %u\n",
			__func__, src_id);
		return -1;
	}

	isp_event_signal(0, &isp->fw_resp_thread[idx].wakeup_evt);

	return 0;
}

result_t isp_isr_set_callback(void *private_data, unsigned int src_id,
				unsigned int type, int enabled)
{
	return RET_SUCCESS;
}

result_t isp_register_isr(struct isp_context *isp)
{
	int32 tempi = 0;
	enum cgs_result cgs_ret = CGS_RESULT__ERROR_GENERIC;
	unsigned int client_id = AMDGPU_IH_CLIENTID_ISP;

	if ((g_cgs_srv->os_ops->add_irq_source == NULL)
		|| (g_cgs_srv->os_ops->irq_get == NULL)
		|| (g_cgs_srv->os_ops->irq_put == NULL)) {
		isp_dbg_print_err("-><- %s,reg fail no func\n", __func__);
		return RET_FAILURE;
	}

	for (tempi = 0; tempi < MAX_REAL_FW_RESP_STREAM_NUM; tempi++) {
		cgs_ret = g_cgs_srv->os_ops->add_irq_source(g_cgs_srv,
			client_id, g_irq_src[tempi], 2,
			isp_isr_set_callback, isp_isr, isp);
		if (cgs_ret != CGS_RESULT__OK) {
			isp_dbg_print_err
			("-><- %s,reg ISP_RINGBUFFER_WPT%u fail %u\n",
				__func__, tempi + 5, cgs_ret);
			return RET_FAILURE;
		}

		isp_dbg_print_info("-><- %s,reg ISP_RINGBUFFER_WPT%u suc\n",
			__func__, tempi + 5);

		cgs_ret = g_cgs_srv->os_ops->irq_get(g_cgs_srv, client_id,
			g_irq_src[tempi], 1);
		if (cgs_ret != CGS_RESULT__OK) {
			isp_dbg_print_err
			("-><- %s, put ISP_RINGBUFFER_WPT%u fail %u\n",
				__func__, tempi + 5, cgs_ret);
			return RET_FAILURE;
		}

		isp_dbg_print_info("-><- %s,put ISP_RINGBUFFER_WPT%u suc\n",
				__func__, tempi + 5);
		isp->irq_enable_id[tempi] = 0;
	}
	return RET_SUCCESS;
};


result_t isp_unregister_isr(struct isp_context *isp_context)
{
	enum cgs_result cgs_ret;
	uint32 tempi;
	unsigned int client_id = AMDGPU_IH_CLIENTID_ISP;

	if (g_cgs_srv->os_ops->irq_put == NULL) {
		isp_dbg_print_err("-><- %s fail no func\n", __func__);
		return RET_FAILURE;
	};

	for (tempi = 0; tempi < MAX_REAL_FW_RESP_STREAM_NUM; tempi++) {
		cgs_ret = g_cgs_srv->os_ops->irq_put
		(g_cgs_srv, client_id, g_irq_src[tempi], 1);
		isp_context->irq_enable_id[tempi] = 0;
		if (cgs_ret != CGS_RESULT__OK) {
			isp_dbg_print_err
				("-><-%s,fail unreg ISP_RINGBUFFER_WPT%u %u\n",
				__func__, tempi + 5, cgs_ret);
		}

		isp_dbg_print_info("-><- %s suc unreg ISP_RINGBUFFER_WPT%u\n",
			__func__, tempi + 5);
	};
	return RET_SUCCESS;
}
uint32 isp_fb_get_emb_data_size(void)
{
	return EMB_DATA_BUF_SIZE;
}

result_t ispm_sw_init(struct isp_context *isp_info,
			uint32 init_flag, void *gart_range_hdl)
{
	enum camera_id cam_id;
	result_t ret;
	uint32 rsv_fb_size;

	uint64 rsv_fw_work_buf_sys;
	uint64 rsv_fw_work_buf_mc;

	if (isp_info == NULL)
		return RET_INVALID_PARAM;
	if (isp_info->sw_isp_context == NULL)
		isp_info->sw_isp_context = isp_info;
	init_calib_items(isp_info);
	isp_info->fw_ctrl_3a = 1;

#ifdef ISP_TUNING_TOOL_SUPPORT
	isp_info->fw_ctrl_3a = 0;
#endif

	isp_info->sensor_count = CAMERA_ID_MAX;
	isp_list_init(&isp_info->take_photo_seq_buf_list);
	isp_list_init(&isp_info->work_item_list);
	isp_mutex_init(&isp_info->ops_mutex);
	isp_mutex_init(&isp_info->map_unmap_mutex);
	isp_mutex_init(&isp_info->cmd_q_mtx);
	isp_mutex_init(&isp_info->take_pic_mutex);
	init_all_camera_info(isp_info);
	init_all_camera_dev_info(isp_info);
	isp_init_drv_setting(isp_info);
	isp_sw_init_capabilities(isp_info);
	isp_mutex_init(&isp_info->command_mutex);
	isp_mutex_init(&isp_info->response_mutex);

	/*isp_sw_init_all_evts(isp_info); */
	isp_pwr_unit_init(&isp_info->isp_pu_isp);
	isp_pwr_unit_init(&isp_info->isp_pu_dphy);
	isp_snr_meta_data_init(isp_info);

	for (cam_id = CAMERA_ID_REAR; cam_id < CAMERA_ID_MAX; cam_id++)
		isp_pwr_unit_init(&isp_info->isp_pu_cam[cam_id]);

	isp_info->host2fw_seq_num = 1;
	ISP_SET_STATUS(isp_info, ISP_STATUS_UNINITED);

	isp_dbg_print_info("-> %s\n", __func__);
	isp_dbg_print_info("sizeof(CmdSetSensorProp_t) %lu\n",
		sizeof(CmdSetSensorProp_t));
	isp_dbg_print_info("sizeof(CmdCaptureYuv_t) %lu\n",
		sizeof(CmdCaptureYuv_t));
	isp_dbg_print_info("sizeof(CmdSetDeviceScript_t) %lu\n",
		sizeof(CmdSetDeviceScript_t));
	isp_dbg_print_info("sizeof(Mode3FrameInfo_t) %lu\n",
		sizeof(Mode3FrameInfo_t));
	isp_dbg_print_info("sizeof(struct psp_fw_header) %lu\n",
		sizeof(struct psp_fw_header));
/*
 *	isp_dbg_print_info
 *		("sizeof(SensorTdb_t) %d,sizeof(package_td_header) %d\n",
 *		sizeof(SensorTdb_t), sizeof(package_td_header));
 *	isp_dbg_print_info("sizeof(profile_header) %d,offset %u\n",
 *		sizeof(profile_header), FIELD_OFFSET(package_td_header, pro));
 */

	rsv_fb_size = ISP_FW_WORK_BUF_SIZE;

/*
 * In case of running in KMD, the init_flag is not initilized
 * But in AVStream, it will be initialized to a magic value as 0x446e6942
 */
	if (init_flag != 0x446e6942) {
		lfb_resv_imp(isp_info->sw_isp_context,
			rsv_fb_size, NULL, NULL);

		isp_dbg_print_info("%s resv fw buf %uM\n", __func__,
			(rsv_fb_size) / (1024 * 1024));
		goto quit;
	};

	if (create_work_thread(&isp_info->work_item_thread,
	work_item_thread_wrapper, isp_info) != RET_SUCCESS) {
		isp_dbg_print_err("in %s fail for create work_item_thread\n",
			__func__);
		goto fail;
	}

	isp_dbg_print_info("in %s, create work thread ok\n", __func__);

	ret = lfb_resv_imp(isp_info->sw_isp_context, rsv_fb_size,
			&rsv_fw_work_buf_sys, &rsv_fw_work_buf_mc);

	if (ret != RET_SUCCESS) {
		isp_dbg_print_err("in %s, isp_lfb_resv %uM fail\n",
			__func__, rsv_fb_size / (1024 * 1024));
		goto fail;
	};

	isp_info->fw_work_buf_hdl =
		isp_fw_work_buf_init(rsv_fw_work_buf_sys, rsv_fw_work_buf_mc);

	if (isp_info->fw_work_buf_hdl == NULL) {
		isp_dbg_print_err
			("in %s, isp_fw_work_buf_init fail\n", __func__);
		goto fail;
	}

	isp_dbg_print_info("in %s, isp_fw_work_buf_init suc\n", __func__);

	if (isp_start_resp_proc_threads(isp_info) != RET_SUCCESS) {
		isp_dbg_print_err
			("%s, isp_start_resp_proc_threads fail\n", __func__);
		goto fail;
	}

	isp_dbg_print_info("in %s, create resp threads ok\n", __func__);

quit:
	ISP_SET_STATUS(isp_info, ISP_STATUS_INITED);
	isp_dbg_print_info("<- %s succ\n", __func__);
	return RET_SUCCESS;
fail:
	ispm_sw_uninit(isp_info);
	isp_dbg_print_info("<- %s fail\n", __func__);
	return RET_FAILURE;
}

result_t ispm_sw_uninit(struct isp_context *isp_context)
{
	isp_dbg_print_info("-> %s\n", __func__);

	isp_unregister_isr(isp_context);

	isp_stop_resp_proc_threads(isp_context);
	stop_work_thread(&isp_context->work_item_thread);

	isp_list_destroy(&isp_context->take_photo_seq_buf_list, NULL);
	isp_list_destroy(&isp_context->work_item_list, NULL);

//	destroy_all_camera_info(isp_context);
	isp_clear_cmdq(isp_context);

	if (isp_context == isp_context->sw_isp_context) {
		isp_dbg_print_info("%s, free resv fw buf\n", __func__);

		lfb_free_imp(isp_context);
	} else {
		isp_dbg_print_info("%s, no need free resv fw buf\n", __func__);
	}

	isp_context->fw_mem_hdl = NULL;

	if (isp_context->fw_data) {
		isp_sys_mem_free(isp_context->fw_data);
		isp_context->fw_data = NULL;
		isp_context->fw_len = 0;
	}

	if (isp_context->fw_work_buf_hdl) {
		isp_fw_work_buf_unit(isp_context->fw_work_buf_hdl);
		isp_context->fw_work_buf_hdl = NULL;
	}

	uninit_calib_items(isp_context);
	isp_snr_meta_data_uninit(isp_context);

	ISP_SET_STATUS(isp_context, ISP_STATUS_UNINITED);
	isp_dbg_print_info("<- %s\n", __func__);
	return RET_SUCCESS;
}

void unit_isp(void)
{
	struct sensor_info *sensor_info;
	uint32 i = 0;
	result_t result;
	struct isp_context *isp_context = &g_isp_context;

	i = 0;
	result = RET_SUCCESS;
	sensor_info = NULL;
	isp_fw_stop_all_stream(isp_context);

	for (i = 0; i < isp_context->sensor_count; i++) {
		sensor_info = &isp_context->sensor_info[i];
		result = isp_snr_pwr_set(isp_context, i, false);
		if (result != RET_SUCCESS) {
			isp_dbg_print_err("in %s, pwr down sensor %d fail\n",
				__func__, i);
		}
	};

	isp_ip_pwr_off(isp_context);

	//isp_hw_ccpu_rst(isp_context);
	ispm_sw_uninit(isp_context);
}


uint32 isp_cal_buf_len_by_para(enum pvt_img_fmt fmt, uint32 width,
			uint32 height, uint32 l_pitch, uint32 c_pitch,
			uint32 *y_len, uint32 *u_len, uint32 *v_len)
{
	uint32 channel_a_width;
	uint32 channel_a_height;
	uint32 channel_a_stride;

	uint32 channel_b_width;
	uint32 channel_b_height;
	uint32 channel_b_stride;

	uint32 channel_c_width;
	uint32 channel_c_height;
	uint32 channel_c_stride;

	uint32 buffer_size;

	switch (fmt) {
	case PVT_IMG_FMT_NV12:
	case PVT_IMG_FMT_NV21:
		channel_a_width = width;
		channel_a_stride = ISP_ADDR_ALIGN_UP(width, 16);
		channel_a_height = height;
		channel_b_width = width;
		channel_b_stride = ISP_ADDR_ALIGN_UP(width, 16);
		channel_b_height = height / 2;
		channel_c_width = 0;
		channel_c_stride = 0;
		channel_c_height = 0;
		buffer_size = channel_a_stride * channel_a_height +
				channel_b_stride * channel_b_height +
				channel_c_stride * channel_c_height;
		break;
	case PVT_IMG_FMT_L8:
		channel_a_width = width;
		channel_a_stride = ISP_ADDR_ALIGN_UP(width, 16);
		channel_a_height = height;
		channel_b_width = 0;
		channel_b_stride = 0;
		channel_b_height = 0;
		channel_c_width = 0;
		channel_c_stride = 0;
		channel_c_height = 0;
		buffer_size = channel_a_stride * channel_a_height +
				channel_b_stride * channel_b_height +
				channel_c_stride * channel_c_height;
		break;
	case PVT_IMG_FMT_YV12:
	case PVT_IMG_FMT_I420:
		channel_a_width = width;
		channel_a_stride = ISP_ADDR_ALIGN_UP(width, 16);
		channel_a_height = height;
		channel_b_width = width / 2;
//		channel_b_stride = ISP_ADDR_ALIGN_UP(channel_a_stride, 16);
		channel_b_stride = ISP_ADDR_ALIGN_UP(channel_a_stride / 2, 16);
		channel_b_height = height / 2;
		channel_c_width = width / 2;
//		channel_c_stride = ISP_ADDR_ALIGN_UP(channel_a_stride, 16);
		channel_c_stride = channel_b_stride;
		channel_c_height = height / 2;
		buffer_size = channel_a_stride * channel_a_height +
				channel_b_stride * channel_b_height +
				channel_c_stride * channel_c_height;
		break;
	case PVT_IMG_FMT_YUV422P:
		channel_a_width = width;
		channel_a_stride = ISP_ADDR_ALIGN_UP(width, 16);
		channel_a_height = height;
		channel_b_width = width / 2;
		channel_b_stride = ISP_ADDR_ALIGN_UP(channel_a_stride / 2, 16);
		channel_b_height = height;
		channel_c_width = width / 2;
		channel_c_stride = ISP_ADDR_ALIGN_UP(channel_a_stride / 2, 16);
		channel_c_height = height;
		buffer_size = channel_a_stride * channel_a_height +
				channel_b_stride * channel_b_height +
				channel_c_stride * channel_c_height;
		break;
	case PVT_IMG_FMT_YUV422_SEMIPLANAR:
		channel_a_width = width;
		channel_a_stride = ISP_ADDR_ALIGN_UP(width, 16);
		channel_a_height = height;
		channel_b_width = width;
		channel_b_stride = ISP_ADDR_ALIGN_UP(width, 16);
		channel_b_height = height;
		channel_c_width = 0;
		channel_c_stride = 0;
		channel_c_height = 0;
		break;
	case PVT_IMG_FMT_YUV422_INTERLEAVED:
		channel_a_width = width;
		channel_a_stride = width * 2;
		channel_a_height = height;
		channel_b_width = 0;
		channel_b_stride = 0;
		channel_b_height = 0;
		channel_c_width = 0;
		channel_c_stride = 0;
		channel_c_height = 0;
		buffer_size = channel_a_stride * channel_a_height;
		break;
	default:
		isp_dbg_print_err("-><- %s, bad fmt %d\n", __func__, fmt);
		return 0;
	}

	channel_a_stride = l_pitch;
	if (width > l_pitch) {
		isp_dbg_print_err("-><- %s, bad l_pitch %d\n",
			__func__, l_pitch);
		return 0;
	};
	if (channel_b_height) {
		channel_b_stride = c_pitch;
		if (!c_pitch) {
			isp_dbg_print_err("-><- %s,bad cpitch %d\n",
				__func__, c_pitch);
			return 0;
		};
	}
	if (channel_c_height)
		channel_c_stride = c_pitch;
	if (y_len)
		*y_len = channel_a_stride * channel_a_height;

	if (u_len)
		*u_len = channel_b_stride * channel_b_height;

	if (v_len)
		*v_len = channel_c_stride * channel_c_height;

	buffer_size = channel_a_stride * channel_a_height +
			channel_b_stride * channel_b_height +
			channel_c_stride * channel_c_height;
	return buffer_size;
};

bool_t isp_get_pic_buf_param(struct isp_stream_info *str_info,
				struct isp_picture_buffer_param *buffer_param)
{
	uint32 width = 0;
	uint32 height = 0;
//	isp_out_format_t fw_img_format;

	if ((str_info == NULL) || (buffer_param == NULL))
		return 0;

	buffer_param->color_format = str_info->format;
	width = str_info->width;
	height = str_info->height;

	switch (str_info->format) {
	case PVT_IMG_FMT_NV12:
	case PVT_IMG_FMT_NV21:
		buffer_param->channel_a_width = width;
		buffer_param->channel_a_stride = ISP_ADDR_ALIGN_UP(width, 16);
		buffer_param->channel_a_height = height;
		buffer_param->channel_b_width = width;
		buffer_param->channel_b_stride = ISP_ADDR_ALIGN_UP(width, 16);
		buffer_param->channel_b_height = height / 2;
		buffer_param->channel_c_width = 0;
		buffer_param->channel_c_stride = 0;
		buffer_param->channel_c_height = 0;
		buffer_param->buffer_size =
			buffer_param->channel_a_stride *
			buffer_param->channel_a_height +
			buffer_param->channel_b_stride *
			buffer_param->channel_b_height +
			buffer_param->channel_c_stride *
			buffer_param->channel_c_height;
		break;
	case PVT_IMG_FMT_L8:
		buffer_param->channel_a_width = width;
		buffer_param->channel_a_stride = ISP_ADDR_ALIGN_UP(width, 16);
		buffer_param->channel_a_height = height;
		buffer_param->channel_b_width = 0;
		buffer_param->channel_b_stride = 0;
		buffer_param->channel_b_height = 0;
		buffer_param->channel_c_width = 0;
		buffer_param->channel_c_stride = 0;
		buffer_param->channel_c_height = 0;
		buffer_param->buffer_size =
			buffer_param->channel_a_stride *
			buffer_param->channel_a_height +
			buffer_param->channel_b_stride *
			buffer_param->channel_b_height +
			buffer_param->channel_c_stride *
			buffer_param->channel_c_height;
		break;
	case PVT_IMG_FMT_YV12:
	case PVT_IMG_FMT_I420:
		buffer_param->channel_a_width = width;
		buffer_param->channel_a_stride = ISP_ADDR_ALIGN_UP(width, 16);
		buffer_param->channel_a_height = height;
		buffer_param->channel_b_width = width / 2;
//		buffer_param->channel_b_stride =
//			ISP_ADDR_ALIGN_UP(buffer_param->channel_a_stride, 16);
		buffer_param->channel_b_stride = ISP_ADDR_ALIGN_UP(
			buffer_param->channel_a_stride / 2, 16);
		buffer_param->channel_b_height = height / 2;
		buffer_param->channel_c_width = width / 2;
//		buffer_param->channel_c_stride =
//			ISP_ADDR_ALIGN_UP(buffer_param->channel_a_stride, 16);
		buffer_param->channel_c_stride =
			buffer_param->channel_b_stride;
		buffer_param->channel_c_height = height / 2;
		buffer_param->buffer_size =
			buffer_param->channel_a_stride *
			buffer_param->channel_a_height +
			buffer_param->channel_b_stride *
			buffer_param->channel_b_height +
			buffer_param->channel_c_stride *
			buffer_param->channel_c_height;
		break;
	case PVT_IMG_FMT_YUV422P:
		buffer_param->channel_a_width = width;
		buffer_param->channel_a_stride = ISP_ADDR_ALIGN_UP(width, 16);
		buffer_param->channel_a_height = height;
		buffer_param->channel_b_width = width / 2;
		buffer_param->channel_b_stride =
		    ISP_ADDR_ALIGN_UP(buffer_param->channel_a_stride / 2, 16);
		buffer_param->channel_b_height = height;
		buffer_param->channel_c_width = width / 2;
		buffer_param->channel_c_stride =
		    ISP_ADDR_ALIGN_UP(buffer_param->channel_a_stride / 2, 16);
		buffer_param->channel_c_height = height;
		buffer_param->buffer_size =
			buffer_param->channel_a_stride *
			buffer_param->channel_a_height +
			buffer_param->channel_b_stride *
			buffer_param->channel_b_height +
			buffer_param->channel_c_stride *
			buffer_param->channel_c_height;
		break;
	case PVT_IMG_FMT_YUV422_SEMIPLANAR:
		buffer_param->channel_a_width = width;
		buffer_param->channel_a_stride = ISP_ADDR_ALIGN_UP(width, 16);
		buffer_param->channel_a_height = height;
		buffer_param->channel_b_width = width;
		buffer_param->channel_b_stride = ISP_ADDR_ALIGN_UP(width, 16);
		buffer_param->channel_b_height = height;
		buffer_param->channel_c_width = 0;
		buffer_param->channel_c_stride = 0;
		buffer_param->channel_c_height = 0;
		break;
	case PVT_IMG_FMT_YUV422_INTERLEAVED:
		buffer_param->channel_a_width = width;
		buffer_param->channel_a_stride = width * 2;
		buffer_param->channel_a_height = height;
		buffer_param->channel_b_width = 0;
		buffer_param->channel_b_stride = 0;
		buffer_param->channel_b_height = 0;
		buffer_param->channel_c_width = 0;
		buffer_param->channel_c_stride = 0;
		buffer_param->channel_c_height = 0;
		buffer_param->buffer_size =
		    buffer_param->channel_a_stride *
		    buffer_param->channel_a_height;
		break;
	default:
		isp_dbg_print_err("%s unsupported picture color format:%d\n",
			__func__, str_info->format);
		return 0;
	}

	if (str_info->luma_pitch_set)
		buffer_param->channel_a_stride = str_info->luma_pitch_set;
	if (str_info->chroma_pitch_set) {
		if (buffer_param->channel_b_height)
			buffer_param->channel_b_stride =
				str_info->chroma_pitch_set;
		if (buffer_param->channel_c_height)
			buffer_param->channel_c_stride =
				str_info->chroma_pitch_set;
	}

	buffer_param->buffer_size =
		buffer_param->channel_a_stride *
		buffer_param->channel_a_height +
		buffer_param->channel_b_stride *
		buffer_param->channel_b_height +
		buffer_param->channel_c_stride *
		buffer_param->channel_c_height;

	return 1;
}

ImageFormat_t isp_trans_to_fw_img_fmt(enum pvt_img_fmt format)
{
	ImageFormat_t fmt;

	switch (format) {
	case PVT_IMG_FMT_NV12:
		fmt = IMAGE_FORMAT_NV12;
		break;
	case PVT_IMG_FMT_L8:
		fmt = IMAGE_FORMAT_NV12;
		break;
	case PVT_IMG_FMT_NV21:
		fmt = IMAGE_FORMAT_NV21;
		break;
	case PVT_IMG_FMT_YV12:
		fmt = IMAGE_FORMAT_YV12;
		break;
	case PVT_IMG_FMT_I420:
		fmt = IMAGE_FORMAT_I420;
		break;
	case PVT_IMG_FMT_YUV422P:
		fmt = IMAGE_FORMAT_YUV422PLANAR;
		break;
	case PVT_IMG_FMT_YUV422_SEMIPLANAR:
		fmt = IMAGE_FORMAT_YUV422SEMIPLANAR;
		break;
	case PVT_IMG_FMT_YUV422_INTERLEAVED:
		fmt = IMAGE_FORMAT_YUV422INTERLEAVED;
		break;
	default:
		fmt = IMAGE_FORMAT_INVALID;
		break;
	};
	return fmt;
};

bool isp_get_str_out_prop(struct isp_stream_info *str_info,
				ImageProp_t *out_prop)
{
	uint32 width = 0;
	uint32 height = 0;
//	isp_out_format_t fw_img_format;

	if ((str_info == NULL) || (out_prop == NULL))
		return 0;

	width = str_info->width;
	height = str_info->height;

	switch (str_info->format) {
	case PVT_IMG_FMT_NV12:
		out_prop->imageFormat = IMAGE_FORMAT_NV12;
		out_prop->width = width;
		out_prop->height = height;
		out_prop->lumaPitch = str_info->luma_pitch_set;
		out_prop->chromaPitch = str_info->chroma_pitch_set;
		break;
	case PVT_IMG_FMT_L8:
		out_prop->imageFormat = IMAGE_FORMAT_NV12;
		out_prop->width = width;
		out_prop->height = height;
		out_prop->lumaPitch = str_info->luma_pitch_set;
		out_prop->chromaPitch = str_info->luma_pitch_set;
		break;
	case PVT_IMG_FMT_NV21:
		out_prop->imageFormat = IMAGE_FORMAT_NV21;
		out_prop->width = width;
		out_prop->height = height;
		out_prop->lumaPitch = str_info->luma_pitch_set;
		out_prop->chromaPitch = str_info->chroma_pitch_set;
		break;
	case PVT_IMG_FMT_YV12:
		out_prop->imageFormat = IMAGE_FORMAT_YV12;
		out_prop->width = width;
		out_prop->height = height;
		out_prop->lumaPitch = str_info->luma_pitch_set;
		out_prop->chromaPitch = str_info->chroma_pitch_set;
		break;
	case PVT_IMG_FMT_I420:
		out_prop->imageFormat = IMAGE_FORMAT_I420;
		out_prop->width = width;
		out_prop->height = height;
		out_prop->lumaPitch = str_info->luma_pitch_set;
		out_prop->chromaPitch = str_info->chroma_pitch_set;
		break;
	case PVT_IMG_FMT_YUV422P:
		out_prop->imageFormat = IMAGE_FORMAT_YUV422PLANAR;
		out_prop->width = width;
		out_prop->height = height;
		out_prop->lumaPitch = str_info->luma_pitch_set;
		out_prop->chromaPitch = str_info->chroma_pitch_set;
		break;
	case PVT_IMG_FMT_YUV422_SEMIPLANAR:
		out_prop->imageFormat = IMAGE_FORMAT_YUV422SEMIPLANAR;
		out_prop->width = width;
		out_prop->height = height;
		out_prop->lumaPitch = str_info->luma_pitch_set;
		out_prop->chromaPitch = str_info->chroma_pitch_set;
		break;
	case PVT_IMG_FMT_YUV422_INTERLEAVED:
		out_prop->imageFormat = IMAGE_FORMAT_YUV422INTERLEAVED;
		out_prop->width = width;
		out_prop->height = height;
		out_prop->lumaPitch = width * 2; /*str_info->luma_pitch_set;*/
		out_prop->chromaPitch = 0; /*str_info->chroma_pitch_set; */
		break;
	default:
		isp_dbg_print_err("%s unsupported picture color format:%d\n",
			__func__, str_info->format);
		return 0;
	}

	return 1;
}

uint32 g_now;
isp_time_tick_t g_last_fw_response_time_tick = { 0 };


void isp_idle_detect(struct isp_context *isp)
{
	enum camera_id cam_id;
	struct isp_pwr_unit *pwr_unit;
	bool_t there_is_snr_on = false;

	if (isp == NULL)
		return;
	if (isp->isp_pu_isp.pwr_status == ISP_PWR_UNIT_STATUS_OFF)
		return;
	for (cam_id = CAMERA_ID_REAR; cam_id < CAMERA_ID_MAX; cam_id++) {
		pwr_unit = &isp->isp_pu_cam[cam_id];
		if (isp_pwr_unit_should_pwroff
			(pwr_unit, SENSOR_PWR_OFF_DEALY_TIME)) {
			isp_snr_pwr_set(isp, cam_id, false);
		}

		if (pwr_unit->pwr_status == ISP_PWR_UNIT_STATUS_ON)
			there_is_snr_on = true;
	};

	if (!there_is_snr_on) {
		pwr_unit = &isp->isp_pu_isp;
		if (isp_pwr_unit_should_pwroff
			(pwr_unit, ISP_PWR_OFF_DELAY_TIME)) {
			isp_dphy_pwr_off(isp);
			if (!isp->isp_flash_torch_on) {
				isp_ip_pwr_off(isp);
			};
		};
	};
}

/*refer to aidt_api_disable_ccpu*/
void isp_hw_ccpu_disable(struct isp_context *isp_context)
{
	uint32 regVal;

	isp_context = isp_context;
	isp_dbg_print_info("-> %s\n", __func__);
#ifdef FPGA_VERSION_ISP1P1
	//For the old fpga version, we use 0x1fc0c reg
	//in the fpga scope to do ccpu stall.
	isp_hw_reg_write32(0x1fc0c, 0x1);
#endif
	//For the new fpga version,
	//we use mmISP_CCPU_CNTL bit 19 to do ccpu stall
	regVal = isp_hw_reg_read32(mmISP_CCPU_CNTL);
	regVal |= (0x1 << 19);
	isp_hw_reg_write32(mmISP_CCPU_CNTL, regVal);
	//bdu remove it msleep(100);
//	isp_hw_reg_write32(mmISP_SOFT_RESET, 0x1);//Disable CCPU
#ifdef FPGA_VERSION_ISP1P1
	isp_hw_reg_write32(mmISP_CCPU_CNTL, 0x0); //Disable CCPU CLK
#endif
	isp_hw_reg_write32(mmISP_SOFT_RESET, 0x1); //Disable CCPU
	//bdu remove it thread_sleep(100);
	isp_dbg_print_info("<- %s\n", __func__);
}

/*refer to aidt_api_enable_ccpu*/
void isp_hw_ccpu_enable(struct isp_context *isp_context)
{
	uint32 regVal;

	isp_context = isp_context;
	isp_dbg_print_info("-> %s\n", __func__);
#ifdef FPGA_VERSION_ISP1P1
	isp_hw_reg_write32(0x1fc0c, 0x0);
#endif
	isp_hw_reg_write32(mmISP_SOFT_RESET, 0x0);
	regVal = isp_hw_reg_read32(mmISP_CCPU_CNTL);
	regVal &= (~(0x1 << 19));
	isp_hw_reg_write32(mmISP_CCPU_CNTL, regVal);
	//bdu remove it msleep(100);
#ifdef FPGA_VERSION_ISP1P1
	isp_hw_reg_write32(mmISP_CCPU_CNTL, 0x1); //Enable CCPU CLK
#endif
//	isp_hw_reg_write32(mmISP_SOFT_RESET, 0x0);//Enable CCPU
	isp_dbg_print_info("<- %s\n", __func__);
}

void isp_hw_ccpu_rst(struct isp_context *isp_context)
{
	uint32 reg_val;

	unreferenced_parameter(isp_context);
	reg_val = isp_hw_reg_read32(mmISP_SOFT_RESET);
	reg_val |= ISP_SOFT_RESET__CCPU_SOFT_RESET_MASK;
	isp_hw_reg_write32(mmISP_SOFT_RESET, reg_val);
	isp_dbg_print_info("-><- %s\n", __func__);
}

void isp_hw_ccpu_stall(struct isp_context *isp_context)
{
	uint32 reg_val;

	unreferenced_parameter(isp_context);
	reg_val = isp_hw_reg_read32(mmISP_CCPU_CNTL);
	reg_val |= ISP_CCPU_CNTL__CCPU_STALL_MASK;
	isp_hw_reg_write32(mmISP_CCPU_CNTL, reg_val);
	isp_dbg_print_info("-><- %s\n", __func__);
}

void isp_init_fw_rb_log_buffer(struct isp_context *isp_context,
			uint32 fw_rb_log_base_lo,
			uint32 fw_rb_log_base_hi, uint32 fw_rb_log_size)
{
	bool_t enable = true;

	isp_context = isp_context;
	if (enable) {
		isp_hw_reg_write32(FW_LOG_RB_BASE_LO_REG, fw_rb_log_base_lo);
		isp_hw_reg_write32(FW_LOG_RB_BASE_HI_REG, fw_rb_log_base_hi);
		isp_hw_reg_write32(FW_LOG_RB_SIZE_REG, fw_rb_log_size);
		isp_hw_reg_write32(FW_LOG_RB_WPTR_REG, 0x0);
		isp_hw_reg_write32(FW_LOG_RB_RPTR_REG, 0x0);
	} else {
		isp_hw_reg_write32(FW_LOG_RB_BASE_LO_REG, 0x0);
		isp_hw_reg_write32(FW_LOG_RB_BASE_HI_REG, 0x0);
		isp_hw_reg_write32(FW_LOG_RB_SIZE_REG, 0x0);
		isp_hw_reg_write32(FW_LOG_RB_WPTR_REG, 0x0);
		isp_hw_reg_write32(FW_LOG_RB_RPTR_REG, 0x0);
	}
}

result_t isp_snr_open(struct isp_context *isp_context, enum camera_id cam_id,
			int32 res_fps_id, uint32 flag)
{
	result_t result;
	struct sensor_info *sensor_info;
	struct isp_pwr_unit *pwr_unit;
	uint32 index;

	if ((isp_context == NULL) || (cam_id >= CAMERA_ID_MAX)) {
		isp_dbg_print_err("-><- %s fail for illegal para %d\n",
			__func__, cam_id);
		return RET_FAILURE;
	};

	sensor_info = &isp_context->sensor_info[cam_id];
	pwr_unit = &isp_context->isp_pu_cam[cam_id];

	if (pwr_unit->pwr_status == ISP_PWR_UNIT_STATUS_ON) {
		if ((res_fps_id == sensor_info->cur_res_fps_id) &&
		(flag == sensor_info->open_flag)) {
			isp_dbg_print_info("%s succ, do none\n", __func__);
			return RET_SUCCESS;
		}
		result = isp_snr_stop(isp_context, cam_id);
		if (result != RET_SUCCESS) {
			isp_dbg_print_info
				("-><- %s fail for isp_snr_stop, pwr off it\n",
				__func__);
			goto fail;
		}
	}

	isp_dbg_print_info("-> %s,cid %d,res_fps_id %d,flag 0x%x\n",
		__func__, cam_id, res_fps_id, flag);

	isp_mutex_lock(&pwr_unit->pwr_status_mutex);
	result = isp_snr_pwr_set(isp_context, cam_id, true);
	if (result != RET_SUCCESS) {
		isp_dbg_print_err("%s fail for isp_snr_pwr_set\n", __func__);
		goto fail_unlock;
	}

	isp_context->isp_pu_cam[cam_id].pwr_status =
						ISP_PWR_UNIT_STATUS_ON;

	result = isp_snr_cfg(isp_context, cam_id, res_fps_id, flag);
	if (result != RET_SUCCESS) {
		isp_dbg_print_err("%s fail for isp_snr_cfg fail\n", __func__);
		goto fail_unlock;
	}

	result = isp_snr_get_caps(isp_context, cam_id,
				&sensor_info->sensor_cfg);
	if (result != RET_SUCCESS) {
		isp_dbg_print_err
			("-><- %s fail for isp_snr_get_caps fail %d\n",
			__func__, result);
		goto fail_unlock;
	};

	if (sensor_info->cam_type == CAMERA_TYPE_RGBIR) {
		isp_context->sensor_info[CAMERA_ID_MEM].sensor_cfg =
			sensor_info->sensor_cfg;
	}

	if ((g_isp_env_setting == ISP_ENV_SILICON) &&
	(sensor_info->sensor_cfg.prop.intfType) == SENSOR_INTF_TYPE_MIPI) {
		isp_dbg_print_info
			("%s,cid %u,bitrate %u,laneNum %u\n", __func__,
			cam_id, sensor_info->sensor_cfg.mipi_bitrate,
			sensor_info->sensor_cfg.prop.intfProp.mipi.numLanes);
		start_internal_synopsys_phy(isp_context, cam_id,
			sensor_info->sensor_cfg.mipi_bitrate,
			sensor_info->sensor_cfg.prop.intfProp.mipi.numLanes);
	}

	if (isp_snr_get_fralenline_regaddr(isp_context, cam_id,
					&sensor_info->fps_tab.HighAddr,
					&sensor_info->fps_tab.LowAddr) !=
					RET_SUCCESS)
		isp_dbg_print_err("-><- %s, fail for get fralenline regaddr\n",
			__func__);

	result = isp_snr_start(isp_context, cam_id);
	if (result != RET_SUCCESS) {
		isp_dbg_print_err("<- %s fail for isp_snr_start\n", __func__);
		goto fail_unlock;
	};
	/*
	 *if (isp_context->fw_ctrl_3a) {
	 *	result = isp_set_script_to_fw(isp_context, cam_id);
	 *	if (result != RET_SUCCESS) {
	 *		isp_dbg_print_err
	 *			("%s,isp_set_script_to_fw fail\n", __func__);
	 *		isp_context->fw_ctrl_3a = 0;
	 *		goto fail_unlock;
	 *	};
	 *}
	 */
	sensor_info->cur_res_fps_id = (char)res_fps_id;
	index = get_index_from_res_fps_id(isp_context, cam_id, res_fps_id);
	sensor_info->raw_height = sensor_info->res_fps.res_fps[index].height;
	sensor_info->raw_width = sensor_info->res_fps.res_fps[index].width;

	sensor_info->stream_inited = 0;
	sensor_info->open_flag = flag;
	sensor_info->hdr_enable = flag & OPEN_CAMERA_FLAG_HDR;
	isp_mutex_unlock(&pwr_unit->pwr_status_mutex);
	isp_dbg_print_info("<- %s suc\n", __func__);
	return RET_SUCCESS;

 fail_unlock:
	isp_snr_pwr_set(isp_context, cam_id, false);
	isp_context->isp_pu_cam[cam_id].pwr_status = ISP_PWR_UNIT_STATUS_OFF;
	isp_mutex_unlock(&pwr_unit->pwr_status_mutex);
 fail:
	return RET_FAILURE;
};

result_t isp_snr_close(struct isp_context *isp, enum camera_id cid)
{
	struct sensor_info *sensor_info;
	struct isp_pwr_unit *pwr_unit;

	if (!is_para_legal(isp, cid)) {
		isp_dbg_print_err
			("-><- %s fail for illegal para %d\n", __func__, cid);
		return RET_FAILURE;
	};

	sensor_info = &isp->sensor_info[cid];
	pwr_unit = &isp->isp_pu_cam[cid];

	if (pwr_unit->pwr_status == ISP_PWR_UNIT_STATUS_OFF) {
		isp_dbg_print_info("-><- %s suc, do none\n", __func__);
		return RET_SUCCESS;
	};

	isp_dbg_print_info("-> %s, cid %d\n", __func__, cid);
	isp_mutex_lock(&pwr_unit->pwr_status_mutex);
	if ((g_isp_env_setting == ISP_ENV_SILICON)
	&& (sensor_info->sensor_cfg.prop.intfType == SENSOR_INTF_TYPE_MIPI)) {
		isp_dbg_print_info
		("%s for shutdown_internal_synopsys_phy,cid %u,laneNum %u\n",
			__func__, cid,
			sensor_info->sensor_cfg.prop.intfProp.mipi.numLanes);
		shutdown_internal_synopsys_phy(cid,
			sensor_info->sensor_cfg.prop.intfProp.mipi.numLanes);
	}
	isp_snr_stop(isp, cid);
	isp_snr_pwr_set(isp, cid, false);
	sensor_info->cur_res_fps_id = -1;
	isp_mutex_unlock(&pwr_unit->pwr_status_mutex);
	isp_dbg_print_info("<- %s suc\n", __func__);
	return RET_SUCCESS;
}

void isp_acpi_set_sensor_pwr(struct isp_context *isp,
				enum camera_id cid, bool on)
{
	uint32_t acpi_method;
	uint32_t acpi_function;
	uint32_t input_arg = 0;
	enum cgs_result result;

	acpi_function = (uint32_t)0;
	//(uint32_t)COS_ACPI_TARGET_DISPLAY_ADAPTER;

	if (cid == CAMERA_ID_REAR) {
		if (on)
			acpi_method = FOURCC('R', 'C', 'P', 'U');
		else
			acpi_method = FOURCC('R', 'C', 'P', 'D');
	} else {
		if (on)
			acpi_method = FOURCC('F', 'C', 'P', 'U');
		else
			acpi_method = FOURCC('F', 'C', 'P', 'D');
	}
	result = g_cgs_srv->ops->call_acpi_method(g_cgs_srv, acpi_method,
				acpi_function, &input_arg, NULL, 0, 1, 0);
	if (result == CGS_RESULT__OK) {
		isp_dbg_print_info("-><- %s suc cid %u,on %u\n",
			__func__, cid, on);
	} else {
		isp_dbg_print_err("-><- %s fail with %x cid %u,on %u\n",
			__func__, result, cid, on);
	}
}

result_t isp_snr_pwr_toggle(struct isp_context *isp, enum camera_id cid,
				bool on)
{	//todo
	uint32_t regVal;

	if (!is_para_legal(isp, cid)) {
		isp_dbg_print_err("-><- %s,fail para cid %u, on:%u\n",
			__func__, cid, on);
		return RET_FAILURE;
	};

	if (cid == CAMERA_ID_REAR) {
		if (isp_is_fpga()) {
			// for phy reset
			aidt_hal_write_reg(0xc00, 0x0);
			msleep(100);
			aidt_hal_write_reg(0xc00, 0x1);
			msleep(100);
			aidt_hal_write_reg(0xc00, 0x3);
		} else {
			if (!on) {
			// for snps phy shutdown
			//lane_num = g_phy_context[device_id].lane_num;
			//shutdown_internal_synopsys_phy(device_id, lane_num);
			}
		}

		if (on) {
			// for sensor reset
			regVal =
				aidt_hal_read_reg(mmISP_GPIO_SHUTDOWN_SENSOR0);
			regVal &= (~(0x1 << 4));
			aidt_hal_write_reg(mmISP_GPIO_SHUTDOWN_SENSOR0,
					regVal);
			regVal |= (0x1 << 4);
			aidt_hal_write_reg(mmISP_GPIO_SHUTDOWN_SENSOR0,
					regVal);
		} else {
			// for sensor shutdown
			regVal =
				aidt_hal_read_reg(mmISP_GPIO_SHUTDOWN_SENSOR0);
			regVal &= (~(0x1 << 4));
			aidt_hal_write_reg(mmISP_GPIO_SHUTDOWN_SENSOR0,
					regVal);
		}
	} else if (cid == CAMERA_ID_FRONT_LEFT) {
		if (isp_is_fpga()) {
			// for phy reset
			aidt_hal_write_reg(0xc08, 0x0);
			msleep(100);
			aidt_hal_write_reg(0xc08, 0x1);
			msleep(100);
			aidt_hal_write_reg(0xc08, 0x3);
		} else {
			if (!on) {
			// for snps phy shutdown
			//lane_num = g_phy_context[device_id].lane_num;
			//shutdown_internal_synopsys_phy(device_id, lane_num);
			}
		}

		if (on) {
			// for sensor reset
			regVal =
				aidt_hal_read_reg(mmISP_GPIO_SHUTDOWN_SENSOR1);
			regVal &= (~(0x1 << 4));
			aidt_hal_write_reg(mmISP_GPIO_SHUTDOWN_SENSOR1,
					regVal);
			regVal |= (0x1 << 4);
			aidt_hal_write_reg(mmISP_GPIO_SHUTDOWN_SENSOR1,
					regVal);
		} else {
			// for sensor shutdown
			regVal =
				aidt_hal_read_reg(mmISP_GPIO_SHUTDOWN_SENSOR1);
			regVal &= (~(0x1 << 4));
			aidt_hal_write_reg(mmISP_GPIO_SHUTDOWN_SENSOR1,
					regVal);
		}
	} else {
		isp_dbg_print_info("-><- %s,fail cid %u,on:%u\n",
			__func__, cid, on);
		return RET_FAILURE;
	}

	if (!isp_is_fpga())
		isp_acpi_set_sensor_pwr(isp, cid, on);

	isp_dbg_print_info("-><- %s,suc cid %u, on:%u\n",
		__func__, cid, on);

	return RET_SUCCESS;
}

result_t isp_snr_clk_toggle(struct isp_context *isp, enum camera_id cid,
				bool on)
{	//todo
	uint32 reg;

	if (isp_is_fpga()) {
		isp_dbg_print_info("-><- %s suc, cid %u, on:%u\n",
			__func__, cid, on);
		return RET_SUCCESS;
	};
	if (!is_para_legal(isp, cid)) {
		isp_dbg_print_err("-><- %s,fail para cid %u, on:%u\n",
			__func__, cid, on);
		return RET_FAILURE;
	};
	reg = isp_hw_reg_read32(mmISP_CLOCK_GATING_A);
	if (cid == CAMERA_ID_REAR) {
		if (on)
			reg |= (1 << 8);
		else
			reg &= ~(1 << 8);
	} else {
		if (on)
			reg |= (1 << 9);
		else
			reg &= ~(1 << 9);
	};
	isp_hw_reg_write32(mmISP_CLOCK_GATING_A, reg);
	isp_dbg_print_info("-><- %s,suc cid %u, on:%u\n",
		__func__, cid, on);
	return RET_SUCCESS;
};


result_t isp_send_switch_preview_to_video_cmd(struct isp_context *isp_context)
{
	result_t result = RET_SUCCESS;

	unreferenced_parameter(isp_context);

	/*result = isp_send_fw_cmd(isp_context,
	 *	ISP_HOST2FW_COMMAND_ID__SWITCH_FROM_PREVIEW_TO_VIDEO2D,
	 *	NULL, 0);
	 */

	if (result != RET_SUCCESS) {
		isp_dbg_print_err("-><- %s fail for ins cmd fail %d\n",
			__func__, result);
		return RET_FAILURE;
	}

	isp_dbg_print_err("-><- %s success\n", __func__);

	return RET_SUCCESS;
}

result_t fw_send_switch_video2d_to_preview_cmd(struct isp_context *isp_context)
{
	result_t result = RET_SUCCESS;

	unreferenced_parameter(isp_context);

	/*result = isp_send_fw_cmd(isp_context,
	 *	ISP_HOST2FW_COMMAND_ID__SWITCH_FROM_VIDEO2D_TO_PREVIEW,
	 *	NULL, 0);
	 */

	if (result != RET_SUCCESS) {
		isp_dbg_print_err("-><- %s fail for ins cmd fail %d\n",
			__func__, result);
		return RET_FAILURE;
	}

	/*result = isp_event_wait(
	 *		&isp_context->switch_from_video_to_preview_event,
	 *		2000);
	 */

	if (result != RET_SUCCESS) {
		isp_dbg_print_err("-><- %s fail for wait results fail %d\n",
			__func__, result);
		return RET_FAILURE;
	};
	return RET_SUCCESS;
};



enum isp_pipe_used_status calc_used_pipe_status_for_cam(enum camera_id cam_id)
{
	return cam_id + 1;
}


result_t isp_start_stream_from_idle(struct isp_context *isp,
				    enum camera_id cam_id, int32 pipe_id,
				    enum stream_id str_id)
{
	isp = isp;
	cam_id = cam_id;
	pipe_id = pipe_id;
	str_id = str_id;

	return RET_FAILURE;
}

result_t isp_start_stream_from_busy(struct isp_context *isp,
				    enum camera_id cam_id, int32 pipe_id,
				    enum stream_id str_id)
{
	isp = isp;
	cam_id = cam_id;
	pipe_id = pipe_id;
	str_id = str_id;

	return RET_FAILURE;
}


bool_t is_para_legal(pvoid context, enum camera_id cam_id)
{
	if (context == NULL)
		return 0;

	switch (cam_id) {
	case CAMERA_ID_REAR:
	case CAMERA_ID_FRONT_LEFT:
	case CAMERA_ID_FRONT_RIGHT:
	case CAMERA_ID_MEM:
		break;
	default:
		return 0;
	}

	return 1;
}

result_t isp_set_snr_info_2_fw(struct isp_context *isp, enum camera_id cid)
{
	result_t result;
	struct sensor_info *snr_info;
	CmdSetSensorProp_t snr_prop;
	CmdSetSensorEmbProp_t snr_embprop;
	bool emb_support = 0;

	if (!is_para_legal(isp, cid)) {
		isp_dbg_print_err("-><- %s fail, bad para,isp:%p,cid:%d\n",
			__func__, isp, cid);
		return RET_FAILURE;
	};
	snr_info = &isp->sensor_info[cid];

	isp_dbg_print_info("-> %s,cid:%d\n", __func__, cid);

	if ((cid == CAMERA_ID_MEM)
	|| (isp->sensor_info[cid].cam_type == CAMERA_TYPE_MEM))
		goto no_need_get_cap;
	result = isp_snr_get_caps(isp, cid, &snr_info->sensor_cfg);
	if (result != RET_SUCCESS) {
		isp_dbg_print_err
			("<- %s fail for isp_snr_get_caps\n", __func__);
		return RET_FAILURE;
	};
	snr_info->lens_range.minLens = snr_info->sensor_cfg.lens_pos.min_pos;
	snr_info->lens_range.maxLens = snr_info->sensor_cfg.lens_pos.max_pos;
	snr_info->lens_range.stepLens = 1;

 no_need_get_cap:
	snr_prop.sensorId = isp_get_fw_sensor_id(isp, cid);
	memcpy(&snr_prop.sensorProp, &snr_info->sensor_cfg.prop,
		sizeof(snr_prop.sensorProp));
	dbg_show_sensor_prop(&snr_prop);
	dbg_show_sensor_caps(&snr_info->sensor_cfg);

	if (snr_prop.sensorId == SENSOR_ID_INVALID) {
		isp_dbg_print_err
			("<- %s, fail invalid sensor id\n", __func__);
		return RET_FAILURE;
	} else if (snr_prop.sensorId == SENSOR_ID_RDMA) {
		isp_dbg_print_info
			("in %s, no need set snr prop for mem\n", __func__);
	} else if (isp_send_fw_cmd(isp, CMD_ID_SET_SENSOR_PROP,
				FW_CMD_RESP_STREAM_ID_GLOBAL,
				FW_CMD_PARA_TYPE_INDIRECT,
				&snr_prop,
				sizeof(snr_prop)) != RET_SUCCESS) {
		isp_dbg_print_err("<- %s,CMD_ID_SET_SENSOR_PROP fail,cid %d\n",
			__func__, snr_prop.sensorId);
		return RET_FAILURE;
	}

	isp_dbg_print_info("in %s,CMD_ID_SET_SENSOR_PROP suc,cid %d\n",
		__func__, snr_prop.sensorId);

	if (snr_info->hdr_enable) {
		snr_info->hdr_prop_set_suc = 0;
		if (!snr_info->sensor_cfg.hdr_valid) {
			isp_dbg_print_err
				("in %s snr not support hdr\n", __func__);
		} else {
			CmdSetSensorHdrProp_t hdr_prop;

			hdr_prop.sensorId = cid;
			memcpy(&hdr_prop.hdrProp, &snr_info->sensor_cfg.hdrProp,
				sizeof(hdr_prop.hdrProp));
			if (isp_send_fw_cmd(isp, CMD_ID_SET_SENSOR_HDR_PROP,
					FW_CMD_RESP_STREAM_ID_GLOBAL,
					FW_CMD_PARA_TYPE_INDIRECT,
					&hdr_prop,
					sizeof(hdr_prop)) != RET_SUCCESS) {
				isp_dbg_print_err
					("%s fail CMD_ID_SET_SENSOR_HDR_PROP\n"
					, __func__);
			} else {
				snr_info->hdr_prop_set_suc = 1;
				isp_dbg_print_info
					("in %s,hdr prop set suc\n",
					__func__);
			}
		};
	} else {
		isp_dbg_print_info
			("in %s, hdr not enable\n", __func__);
	}

	snr_embprop.sensorId = isp_get_fw_sensor_id(isp, cid);
	if (cid >= CAMERA_ID_MEM) {
		isp_dbg_print_info
			("in %s, no need set emb for mem\n", __func__);
	} else if (isp_snr_get_emb_prop(isp, cid, &emb_support,
				&snr_embprop.embProp) != RET_SUCCESS) {
		isp_dbg_print_err("in %s fail for isp_snr_get_emb_prop\n",
			__func__);
	} else {
		if (!emb_support) {
			isp_dbg_print_info
				("in %s, not support emb\n", __func__);
		} else if (isp_send_fw_cmd(isp, CMD_ID_SET_SENSOR_EMB_PROP,
					FW_CMD_RESP_STREAM_ID_GLOBAL,
					FW_CMD_PARA_TYPE_INDIRECT,
					&snr_embprop,
					sizeof(snr_embprop)) != RET_SUCCESS) {
			isp_dbg_print_err
				("%s,CMD_ID_SET_SENSOR_EMB_PROP fail,cid %d\n",
				__func__, snr_embprop.sensorId);
			return RET_FAILURE;
		}

		isp_dbg_print_info("in %s,set emb prop suc\n", __func__);
	}

	if (isp_set_calib_2_fw(isp, cid) != RET_SUCCESS) {
		isp_dbg_print_err
			("in %s isp_set_calib_2_fw fail\n", __func__);
	};

	/*todo add M2M configure */

	if (isp->fw_ctrl_3a) {
		result = isp_set_script_to_fw(isp, cid);
		if (result != RET_SUCCESS) {
			isp_dbg_print_err
				("<- %s, fail set script\n", __func__);
			return RET_FAILURE;
		}

		isp_dbg_print_info("in %s,set fw script suc\n", __func__);
	} else {
		isp_dbg_print_info
			("in %s,not fw crl 3a\n", __func__);
	};

	isp_dbg_print_info("<- %s suc\n", __func__);
	return RET_SUCCESS;
};

result_t isp_set_calib_2_fw(struct isp_context *isp, enum camera_id cid)
{
	struct sensor_info *snr_info;
	struct calib_date_item *item;
	CmdSetSensorCalibdb_t calib;
	struct isp_gpu_mem_info *gpu_mem_info;

	if (!is_para_legal(isp, cid)) {
		isp_dbg_print_err("-><- %s fail, bad para,isp:%p,cid:%d\n",
			__func__, isp, cid);
		return RET_FAILURE;
	};

	snr_info = &isp->sensor_info[cid];
	snr_info->calib_set_suc = 0;
	isp_dbg_print_info("-> %s, cid %u\n", __func__, cid);
	item = isp_get_calib_item(isp, cid);

	snr_info->cur_calib_item = item;

	if (item == NULL) {
		isp_dbg_print_info("%s suc, do none for no calib\n", __func__);
		return RET_SUCCESS;
	}

	isp_dbg_print_info("in %s, calib %p(%u),bef memcpy\n",
		__func__, item->data, item->len);

	/*
	 *if (!snr_info->calib_enable) {
	 *isp_dbg_print_info("<- %s,do none for no en\n", __func__);
	 *return RET_SUCCESS;
	 *};
	 */

	if (snr_info->calib_gpu_mem) {
		isp_gpu_mem_free(snr_info->calib_gpu_mem);
		snr_info->calib_gpu_mem = NULL;
	};


	gpu_mem_info = isp_gpu_mem_alloc(item->len, ISP_GPU_MEM_TYPE_NLFB);

	snr_info->calib_gpu_mem = gpu_mem_info;

	if (gpu_mem_info == NULL) {
		isp_dbg_print_err("<- %s fail for isp_gpu_mem_alloc,ignore\n",
			__func__);
		return RET_SUCCESS;
	};

	memcpy(gpu_mem_info->sys_addr, item->data, item->len);

	//calib.sensorId = isp_get_fw_sensor_id(isp, cid);;
	calib.streamId = isp_get_fw_stream_from_drv_stream
		(isp_get_stream_id_from_cid(isp, cid));
	calib.bufSize = item->len;
	isp_split_addr64(gpu_mem_info->gpu_mc_addr, &calib.bufAddrLo,
			&calib.bufAddrHi);
	calib.checkSum = compute_check_sum(item->data, item->len);

	if (isp_send_fw_cmd(isp, CMD_ID_SET_SENSOR_CALIBDB,
				FW_CMD_RESP_STREAM_ID_GLOBAL,
				FW_CMD_PARA_TYPE_DIRECT,
				&calib, sizeof(calib)) != RET_SUCCESS) {
		isp_dbg_print_err
			("<- %s(cid:%d),fwstream %u fail send cmd,ignore\n",
			__func__, cid, calib.streamId);
		return RET_SUCCESS;
	};

	snr_info->calib_set_suc = 1;
	isp_dbg_print_info("<- %s(cid:%d),fwstream %u suc\n",
		__func__, cid, calib.streamId);
	return RET_SUCCESS;
}

enum fw_cmd_resp_stream_id isp_get_stream_id_from_cid(struct isp_context *isp,
				enum camera_id cid)
{
	isp = isp;

	if (isp->sensor_info[cid].stream_id != FW_CMD_RESP_STREAM_ID_MAX)
		return isp->sensor_info[cid].stream_id;

	switch (cid) {
	case CAMERA_ID_REAR:
		isp->sensor_info[cid].stream_id = FW_CMD_RESP_STREAM_ID_1;
		break;
	case CAMERA_ID_FRONT_LEFT:
		isp->sensor_info[cid].stream_id = FW_CMD_RESP_STREAM_ID_2;
		break;
	case CAMERA_ID_FRONT_RIGHT:
		isp->sensor_info[cid].stream_id = FW_CMD_RESP_STREAM_ID_3;
		break;
	case CAMERA_ID_MEM:
		/*if (CAMERA_TYPE_RGBIR ==
		 *isp->sensor_info[CAMERA_ID_REAR].cam_type)
		 *isp->sensor_info[cid].stream_id = FW_CMD_RESP_STREAM_ID_2;
		 *else if(CAMERA_TYPE_RGBIR ==
		 *isp->sensor_info[CAMERA_ID_FRONT_LEFT].cam_type)
		 *isp->sensor_info[cid].stream_id = FW_CMD_RESP_STREAM_ID_1;
		 *break;
		 */
	default:
		isp->sensor_info[cid].stream_id = FW_CMD_RESP_STREAM_ID_3;
		break;
	};
	return isp->sensor_info[cid].stream_id;
}

enum camera_id isp_get_cid_from_stream_id(struct isp_context *isp,
					enum fw_cmd_resp_stream_id stream_id)
{
	enum camera_id cid;

	for (cid = CAMERA_ID_REAR; cid < CAMERA_ID_MAX; cid++) {
		if (stream_id == isp->sensor_info[cid].stream_id)
			return cid;
	};
	return CAMERA_ID_MAX;
};

enum camera_id isp_get_rgbir_cid(struct isp_context *isp)
{
	enum camera_id cid;

	for (cid = CAMERA_ID_REAR; cid < CAMERA_ID_MEM; cid++) {
		if (isp->sensor_info[cid].cam_type == CAMERA_TYPE_RGBIR)
			return cid;
	};
	return CAMERA_ID_MAX;
}

void isp_turn_on_ir_illuminator(struct isp_context *isp, enum camera_id cid)
{

};

void isp_turn_off_ir_illuminator(struct isp_context *isp, enum camera_id cid)
{

}

result_t isp_setup_stream(struct isp_context *isp, enum camera_id cid)
{
	CmdSetStreamMode_t stream_mode;
	CmdSetInputSensor_t input_snr;
//	CmdSetAcqWindow_t acqw;
//	CmdSetAspectRatioWindow_t aprw;
	CmdSetFrameRateInfo_t frame_rate;
	CmdSetIRIlluConfig_t IR;
	enum fw_cmd_resp_stream_id stream_id;
	enum camera_id tmp_cid = cid;

	if (!is_para_legal(isp, cid)) {
		isp_dbg_print_err
			("-><- %s fail, bad para,cid:%d\n", __func__, cid);
		return RET_FAILURE;
	};

	stream_id = isp_get_stream_id_from_cid(isp, cid);
	switch (isp->sensor_info[cid].cam_type) {
	case CAMERA_TYPE_RGB_BAYER:
		if (isp->drv_settings.process_3a_by_host)
			stream_mode.streamMode = STREAM_MODE_3;
		else
			stream_mode.streamMode = STREAM_MODE_2;

		break;
	case CAMERA_TYPE_IR:
		stream_mode.streamMode = STREAM_MODE_2;
		break;
	case CAMERA_TYPE_RGBIR:
		stream_mode.streamMode = STREAM_MODE_3;
		break;
	case CAMERA_TYPE_MEM:
		stream_mode.streamMode = STREAM_MODE_4;
		break;
	default:
		isp_dbg_print_err("-><- %s fail, cid:%d, bad type %u\n",
			__func__, cid, isp->sensor_info[cid].cam_type);
		return RET_FAILURE;
	}
	isp_dbg_print_info("-> %s,cid %u,stream %d, STREAM_MODE_%d\n",
		__func__, cid, stream_id, stream_mode.streamMode - 1);
	if (isp_send_fw_cmd(isp, CMD_ID_SET_STREAM_MODE, stream_id,
				FW_CMD_PARA_TYPE_DIRECT, &stream_mode,
				sizeof(stream_mode)) != RET_SUCCESS) {
		isp_dbg_print_err
			("<- %s fail for SET_STREAM_MODE\n", __func__);
		return RET_FAILURE;
	}

	isp_dbg_print_info("in %s, set mode suc\n", __func__);

	if (cid == CAMERA_ID_MEM) {
		enum camera_id l_cid;

		for (l_cid = CAMERA_ID_REAR; l_cid < CAMERA_ID_FRONT_RIGHT;
		l_cid++) {
			if (isp->sensor_info[l_cid].cam_type ==
			CAMERA_TYPE_RGBIR) {
				tmp_cid = l_cid;
				isp_dbg_print_info
					("in %s,change cid from %u to %u\n",
					__func__, cid, l_cid);
				break;
			};
		}
	};

	input_snr.sensorId = isp_get_fw_sensor_id(isp, tmp_cid);

	if (isp_send_fw_cmd(isp, CMD_ID_SET_INPUT_SENSOR, stream_id,
				FW_CMD_PARA_TYPE_DIRECT, &input_snr,
				sizeof(input_snr)) != RET_SUCCESS) {
		isp_dbg_print_err
			("<- %s fail set input cid %d snrId %u\n",
			__func__, cid, input_snr.sensorId);
		return RET_FAILURE;
	}

	isp_dbg_print_info
		("in %s, set input cid %d snrId %u suc\n",
		__func__, cid, input_snr.sensorId);

	if (!isp_is_mem_camera(isp, cid)) {
		frame_rate.frameRate =
			min(isp->sensor_info[cid].sensor_cfg.frame_rate,
			((uint32) isp->drv_settings.sensor_frame_rate) *
			1000);
//		frame_rate.frameRate = 1;
		if (isp_send_fw_cmd(isp, CMD_ID_SET_FRAME_RATE_INFO, stream_id,
					FW_CMD_PARA_TYPE_DIRECT, &frame_rate,
					sizeof(frame_rate)) != RET_SUCCESS) {
			isp_dbg_print_err
				("<- %s fail for SET_FRAME_RATE_INFO\n",
				__func__);
			return RET_FAILURE;
		}

		isp_dbg_print_info("in %s, set fps %u\n",
			__func__, frame_rate.frameRate);
	} else
		frame_rate.frameRate = 1000;

	//bdu comment it for its definition changed in new fw interface
	// King for OV7251 bring up
	if ((cid < CAMERA_ID_MEM)
	&& (isp->sensor_info[cid].cam_type != CAMERA_TYPE_MEM)
	&& (isp->sensor_info[cid].sensor_cfg.prop.cfaPattern) >=
	CFA_PATTERN_PURE_IR) {

// This value need to match hardware clk configration, current is 66MHz
#define PPI_CLK (24000000)
		uint32 delay_ratio = 0;
		uint32 duration_ratio = 0;
		struct sensor_hw_parameter para = { 0 };
		const uint32 one_frame_count =
				PPI_CLK / (frame_rate.frameRate / 1000);

		isp_snr_get_hw_parameter(isp, cid, &para);

//		IR.IRIlluConfig.bEnable =
//			isp->drv_settings.disable_ir_illuminator == 0 ?
//				true : false;
		IR.IRIlluConfig.bEnable =
			isp->drv_settings.ir_illuminator_ctrl == 2 ?
				true : false;

		IR.IRIlluConfig.bUseDropFrameMethod =
		isp->drv_settings.enable_frame_drop_solution_for_frame_pair
			== 1 ? true : false;
		IR.IRIlluConfig.sensorShutterType = para.sensorShutterType;

		if (IR.IRIlluConfig.bUseDropFrameMethod) {
			delay_ratio = para.IRControl_drop_daley;
			duration_ratio = para.IRControl_drop_durarion;
		} else {
			delay_ratio = para.IRControl_delay;
			duration_ratio = para.IRControl_duration;
		}

		IR.IRIlluConfig.time =
			(uint32) (one_frame_count / 1000 * duration_ratio);
		IR.IRIlluConfig.delay =
			(uint32) (one_frame_count / 1000 * delay_ratio);

		isp_dbg_print_info
		("%s IRIlluConfig  en:%u,duration_ratio:%d, delay_ratio:%d\n",
			__func__,
			IR.IRIlluConfig.bEnable, duration_ratio, delay_ratio);

		if (isp_send_fw_cmd(isp, CMD_ID_SET_IR_ILLU_CONFIG, stream_id,
		FW_CMD_PARA_TYPE_INDIRECT, &IR, sizeof(IR)) != RET_SUCCESS) {
			isp_dbg_print_err
			("%s fail for CMD_ID_SET_IR_ILLU_CONFIG %u %u %u\n",
				__func__,
				isp->sclk, isp->iclk, frame_rate.frameRate);
			return RET_FAILURE;
		}

		isp_dbg_print_info
			("%s, set IRIlluConfig success, %u %u %u\n",
			__func__,
			isp->sclk, isp->iclk, frame_rate.frameRate);

		/*
		 *if (1 == isp->drv_settings.ir_illuminator_ctrl) {
		 *isp_turn_on_ir_illuminator(isp, cid);
		 *}
		 */
	}

	// Get the crop window from tuning data for
	// IR steam in a RGBIR camera and set it to FW
	// The value comes from tuning team and SW needs
	// to calculate the window offset to make it fit to the center
	// This is for RGBIR camera only
	if (cid == CAMERA_ID_MEM) {
		Window_t asprw;
		Window_t irZoomWindow;

		if (!isp->calib_data[cid]) {
			isp_dbg_print_err
				("in %s fail no calib for ir\n", __func__);
			goto quit_set_zoom;
		}

		if (isp_get_rgbir_crop_window(isp, cid, &irZoomWindow) !=
			RET_SUCCESS) {
			goto quit_set_zoom;
		}
		// RAW resolution for IR stream
		asprw.h_size = isp->sensor_info[cid].raw_width;
		asprw.v_size = isp->sensor_info[cid].raw_height;

		if ((irZoomWindow.h_size > asprw.h_size)
		|| (irZoomWindow.v_size > asprw.v_size)) {
			isp_dbg_print_err
				("%s fail ir crop too large [%u,%u]>[%u,%u]\n",
				__func__,
				irZoomWindow.h_size, irZoomWindow.v_size,
				asprw.h_size, asprw.v_size);
			goto quit_set_zoom;
		}

		irZoomWindow.h_offset =
			(asprw.h_size - irZoomWindow.h_size) / 2;
		irZoomWindow.v_offset =
			(asprw.v_size - irZoomWindow.v_size) / 2;

		if (isp_send_fw_cmd(isp, CMD_ID_SET_ZOOM_WINDOW, stream_id,
					FW_CMD_PARA_TYPE_DIRECT, &irZoomWindow,
					sizeof(irZoomWindow)) != RET_SUCCESS) {
			isp_dbg_print_err
			("%s fail set IR Zoom Windows x:y:w:h %u:%u:%u:%u\n",
				__func__,
				irZoomWindow.h_offset, irZoomWindow.v_offset,
				irZoomWindow.h_size, irZoomWindow.v_size);
			return RET_FAILURE;
		}

		isp_dbg_print_info
			("in %s, IR Zoom Windows x:y:w:h %u:%u:%u:%u suc\n",
				__func__,
				irZoomWindow.h_offset, irZoomWindow.v_offset,
				irZoomWindow.h_size, irZoomWindow.v_size);

 quit_set_zoom:
		;
	};

	/* TODO: Enable SNR per tuning team's request */
//	if (isp_is_mem_camera(isp, cid))
//		goto no_set_tnr_snr;

	if (isp->drv_settings.demosaic_thresh < 256) {
		CmdDemosaicSetThresh_t dmt;

		dmt.thresh.thresh = (uint8) isp->drv_settings.demosaic_thresh;
		if (isp_send_fw_cmd(isp, CMD_ID_DEMOSAIC_SET_THRESH, stream_id,
					FW_CMD_PARA_TYPE_DIRECT, &dmt,
					sizeof(dmt)) != RET_SUCCESS) {
			isp_dbg_print_err
			("in %s CMD_ID_DEMOSAIC_SET_THRESH to (%u) fail\n",
				__func__, dmt.thresh.thresh);
		} else {
			isp_dbg_print_info
			("in %s CMD_ID_DEMOSAIC_SET_THRESH to (%u) suc\n",
				__func__, dmt.thresh.thresh);
		};

	}

	if (isp->drv_settings.dpf_enable) {
		if (isp_send_fw_cmd(isp, CMD_ID_DPF_ENABLE, stream_id,
					FW_CMD_PARA_TYPE_DIRECT,
					NULL, 0) != RET_SUCCESS) {
			isp_dbg_print_err
				("in %s CMD_ID_DPF_ENABLE fail\n", __func__);
		} else {
			isp_dbg_print_info
				("in %s CMD_ID_DPF_ENABLE suc\n", __func__);
		};
	} else {
		if (isp_send_fw_cmd(isp, CMD_ID_DPF_DISABLE, stream_id,
					FW_CMD_PARA_TYPE_DIRECT,
					NULL, 0) != RET_SUCCESS) {
			isp_dbg_print_err
				("in %s CMD_ID_DPF_DISABLE fail\n", __func__);
		} else {
			isp_dbg_print_info
				("in %s CMD_ID_DPF_DISABLE suc\n", __func__);
		};
	}

	if (isp->drv_settings.snr_enable) {
		if (isp_send_fw_cmd(isp, CMD_ID_SNR_ENABLE, stream_id,
					FW_CMD_PARA_TYPE_DIRECT,
					NULL, 0) != RET_SUCCESS) {
			isp_dbg_print_err
				("in %s CMD_ID_SNR_ENABLE fail\n", __func__);
		} else {
			isp_dbg_print_info
				("in %s CMD_ID_SNR_ENABLE suc\n", __func__);
		};
	} else {
		if (isp_send_fw_cmd(isp, CMD_ID_SNR_DISABLE, stream_id,
					FW_CMD_PARA_TYPE_DIRECT,
					NULL, 0) != RET_SUCCESS) {
			isp_dbg_print_err
				("in %s CMD_ID_SNR_DISABLE fail\n", __func__);
		} else {
			isp_dbg_print_info
				("in %s CMD_ID_SNR_DISABLE suc\n", __func__);
		};
	}

//no_set_tnr_snr:
	if (isp->sensor_info[cid].calib_enable
	&& isp->sensor_info[cid].calib_set_suc) {
		if (isp_send_fw_cmd(isp, CMD_ID_ENABLE_CALIB, stream_id,
					FW_CMD_PARA_TYPE_DIRECT,
					NULL, 0) != RET_SUCCESS) {
			isp_dbg_print_err("%s fail for CMD_ID_ENABLE_CALIB\n",
				__func__);
			return RET_FAILURE;
		}

		isp_dbg_print_info
			("%s suc CMD_ID_ENABLE_CALIB suc\n", __func__);
	} else {
		if (isp_send_fw_cmd(isp, CMD_ID_DISABLE_CALIB, stream_id,
					FW_CMD_PARA_TYPE_DIRECT,
					NULL, 0) != RET_SUCCESS) {
			isp_dbg_print_err("%s fail for CMD_ID_DISABLE_CALIB\n",
				__func__);
			return RET_FAILURE;
		}

		isp_dbg_print_info
			("%s suc CMD_ID_DISABLE_CALIB suc\n", __func__);
	};
	isp_dbg_print_info("<- %s suc\n", __func__);
	return RET_SUCCESS;
}

result_t isp_setup_metadata_buf(struct isp_context *isp, enum camera_id cid)
{
	struct sensor_info *snr_info;
	uint32 i;
	uint32 len[STREAM_META_BUF_COUNT] = { 0 };
	CmdSendBuffer_t buf_type;
	result_t ret;
	enum fw_cmd_resp_stream_id stream_id;

	if (!is_para_legal(isp, cid)) {
		isp_dbg_print_err("-><- %s fail, bad para,cid:%d\n",
			__func__, cid);
		return RET_FAILURE;
	};
	stream_id = isp_get_stream_id_from_cid(isp, cid);
	snr_info = &isp->sensor_info[cid];

	for (i = 0; i < STREAM_META_BUF_COUNT; i++) {
		ret = isp_fw_get_nxt_cmd_pl(isp->fw_work_buf_hdl, NULL,
					&snr_info->meta_mc[i], &len[i]);
		if (ret != RET_SUCCESS) {
			isp_dbg_print_err("-><- %s(%u) fail, aloc buf(%u)\n",
				__func__, cid, i);
			goto fail;
		}
	};

	for (i = 0; i < STREAM_META_BUF_COUNT; i++) {
		memset(&buf_type, 0, sizeof(buf_type));
		buf_type.bufferType = BUFFER_TYPE_META_INFO;
		buf_type.buffer.bufTags = 0;
		buf_type.buffer.vmidSpace = 4;
		isp_split_addr64(snr_info->meta_mc[i],
				 &buf_type.buffer.bufBaseALo,
				 &buf_type.buffer.bufBaseAHi);
		buf_type.buffer.bufSizeA = len[i];
		if (isp_send_fw_cmd(isp, CMD_ID_SEND_BUFFER, stream_id,
					FW_CMD_PARA_TYPE_DIRECT, &buf_type,
					sizeof(buf_type)) != RET_SUCCESS) {
			isp_dbg_print_err("-><- %s(%u) fail, send meta(%u)\n",
				__func__, cid, i);
			goto fail;
		};
	};

	isp_dbg_print_info("-><- %s(%u), cnt %u suc\n",
		__func__, STREAM_META_BUF_COUNT, cid);
	return RET_SUCCESS;

 fail:
	for (i = 0; i < STREAM_META_BUF_COUNT; i++) {
		if (snr_info->meta_mc[i]) {
			isp_fw_ret_pl(isp->fw_work_buf_hdl,
				snr_info->meta_mc[i]);
			snr_info->meta_mc[i] = 0;
		};
	};
	return RET_FAILURE;
};

result_t isp_setup_stream_tmp_buf(struct isp_context *isp, enum camera_id cid)
{
	struct sensor_info *snr_info;
	uint32 i;
	uint32 size;
	CmdSendBuffer_t buf_type;
	enum fw_cmd_resp_stream_id stream_id;

	if (!is_para_legal(isp, cid)) {
		isp_dbg_print_err("-><- %s fail, bad para,cid:%d\n",
			__func__, cid);
		return RET_FAILURE;
	};
	if (isp_is_mem_camera(isp, cid)) {
		isp_dbg_print_err
			("-><- %s suc do none for mem sensor\n", __func__);
		return RET_SUCCESS;
	};
	stream_id = isp_get_stream_id_from_cid(isp, cid);
	snr_info = &isp->sensor_info[cid];
	size = snr_info->raw_height * snr_info->raw_width * 2;
	isp_dbg_print_info("-> %s, cid:%d, buf cnt %d\n",
		__func__, cid, STREAM_TMP_BUF_COUNT);
	if (!isp_is_host_processing_raw(isp, cid)
	|| (snr_info->cam_type == CAMERA_TYPE_IR)) {
		uint32 align_32k = 1;

		for (i = 0; i < STREAM_TMP_BUF_COUNT; i++) {

			if (snr_info->stream_tmp_buf[i])
				isp_gpu_mem_free(snr_info->stream_tmp_buf[i]);
			snr_info->stream_tmp_buf[i] =
				isp_gpu_mem_alloc(size, ISP_GPU_MEM_TYPE_NLFB);

			if (snr_info->stream_tmp_buf[i] == NULL) {
				isp_dbg_print_err("in %s,aloc tmp(%u) fail\n",
					__func__, i);
				goto fail;
			}
		};

		for (i = 0; i < STREAM_TMP_BUF_COUNT; i++) {
			memset(&buf_type, 0, sizeof(buf_type));
			buf_type.bufferType = BUFFER_TYPE_RAW_TEMP;
			buf_type.buffer.bufTags = 0;
			buf_type.buffer.vmidSpace = 4;
			isp_split_addr64(
				snr_info->stream_tmp_buf[i]->gpu_mc_addr,
					&buf_type.buffer.bufBaseALo,
					&buf_type.buffer.bufBaseAHi);
			buf_type.buffer.bufSizeA =
				(uint32) snr_info->stream_tmp_buf[i]->mem_size;
			/*
			 *memset(snr_info->stream_tmp_buf[i]->sys_addr, 0,
			 *(size_t)snr_info->stream_tmp_buf[i]->mem_size);
			 */
			if (isp_send_fw_cmd(isp, CMD_ID_SEND_BUFFER, stream_id,
					FW_CMD_PARA_TYPE_DIRECT, &buf_type,
					sizeof(buf_type)) != RET_SUCCESS) {
				isp_dbg_print_err("in %s, send tmp(%u) fail\n",
					__func__, i);
				goto fail;
			};

			if ((buf_type.buffer.bufBaseALo % (32 * 1024)))
				align_32k = 0;
		};
		isp_dbg_print_info("in %s, send tmp raw ok\n", __func__);
	}

	if (!isp_is_host_processing_raw(isp, cid)
	|| (snr_info->cam_type == CAMERA_TYPE_IR)
	|| (snr_info->cam_type == CAMERA_TYPE_RGBIR)) {
		if (snr_info->stage2_tmp_buf) {

			isp_gpu_mem_free(snr_info->stage2_tmp_buf);

			snr_info->stage2_tmp_buf = NULL;
		}

		if (isp->drv_settings.enable_stage2_temp_buf) {
			size = snr_info->raw_height * snr_info->raw_width * 2;

			snr_info->stage2_tmp_buf =
				isp_gpu_mem_alloc(size, ISP_GPU_MEM_TYPE_NLFB);

			if (snr_info->stage2_tmp_buf) {
				memset(&buf_type, 0, sizeof(buf_type));
				buf_type.bufferType =
					BUFFER_TYPE_STAGE2_RECYCLE;
				buf_type.buffer.bufTags = 0;
				buf_type.buffer.vmidSpace = 4;
				isp_split_addr64(
					snr_info->stage2_tmp_buf->gpu_mc_addr,
					&buf_type.buffer.bufBaseALo,
					&buf_type.buffer.bufBaseAHi);
				buf_type.buffer.bufSizeA = (uint32)
					snr_info->stage2_tmp_buf->mem_size;

				buf_type.buffer.bufBaseBHi =
					buf_type.buffer.bufBaseAHi;
				buf_type.buffer.bufBaseBLo =
					buf_type.buffer.bufBaseALo;
				buf_type.buffer.bufSizeB =
					buf_type.buffer.bufSizeA;

				buf_type.buffer.bufBaseCHi =
					buf_type.buffer.bufBaseAHi;
				buf_type.buffer.bufBaseCLo =
					buf_type.buffer.bufBaseALo;
				buf_type.buffer.bufSizeC =
					buf_type.buffer.bufSizeA;

				if (isp_send_fw_cmd(isp, CMD_ID_SEND_BUFFER,
						stream_id,
						FW_CMD_PARA_TYPE_DIRECT,
						&buf_type, sizeof(buf_type))
						!= RET_SUCCESS) {

					isp_gpu_mem_free
						(snr_info->stage2_tmp_buf);

					snr_info->stage2_tmp_buf = NULL;
					isp_dbg_print_err
					("in %s suc, send stage2 tmp fail\n",
						__func__);
				} else {
					isp_dbg_print_info
					("in %s suc, send stage2 tmp ok\n",
						__func__);
				};
			} else {
				isp_dbg_print_err
				("in %s, aloc stage2 tmp fail\n", __func__);
			};
		}
	}
	//setup iic control buf
	if (snr_info->iic_tmp_buf == NULL) {
		size = IIC_CONTROL_BUF_SIZE;

		snr_info->iic_tmp_buf =
			isp_gpu_mem_alloc(size, ISP_GPU_MEM_TYPE_NLFB);

		if (snr_info->iic_tmp_buf) {
			isp_dbg_print_info
				("in %s, aloc iic control buf ok\n", __func__);
		} else {
			isp_dbg_print_err
				("%s, aloc iic control buf fail\n", __func__);
		}
	}


	//send emb data buf
	size = isp_fb_get_emb_data_size() / EMB_DATA_BUF_NUM;
	for (i = 0; i < EMB_DATA_BUF_NUM; i++) {

		snr_info->emb_data_buf[i] =
			isp_gpu_mem_alloc(size, ISP_GPU_MEM_TYPE_NLFB);

		if (snr_info->emb_data_buf[i]) {
			memset(&buf_type, 0, sizeof(buf_type));
			buf_type.bufferType = BUFFER_TYPE_EMB_DATA;
			buf_type.buffer.bufTags = 0;
			buf_type.buffer.vmidSpace = 4;
			isp_split_addr64(snr_info->emb_data_buf[i]->gpu_mc_addr,
					 &buf_type.buffer.bufBaseALo,
					 &buf_type.buffer.bufBaseAHi);
			buf_type.buffer.bufSizeA =
			    (uint32) snr_info->emb_data_buf[i]->mem_size;

			if (isp_send_fw_cmd(isp, CMD_ID_SEND_BUFFER, stream_id,
					FW_CMD_PARA_TYPE_DIRECT, &buf_type,
					sizeof(buf_type)) != RET_SUCCESS) {

				isp_gpu_mem_free(snr_info->emb_data_buf[i]);

				snr_info->emb_data_buf[i] = NULL;
				isp_dbg_print_err
					("%s suc,send emb fail\n", __func__);
			} else {
				isp_dbg_print_info
					("in %s suc, send emb ok\n", __func__);
			};
		} else {
			isp_dbg_print_err
				("in %s suc, aloc emb fail\n", __func__);
		}
	}


	if ((snr_info->cam_type != CAMERA_TYPE_MEM)
	&& isp->drv_settings.tnr_enable) {
		snr_info->tnr_tmp_buf_set = 0;
		if (snr_info->tnr_tmp_buf) {

			isp_gpu_mem_free(snr_info->tnr_tmp_buf);

			snr_info->tnr_tmp_buf = NULL;
		}
		size = snr_info->raw_height * snr_info->raw_width * 2;

		snr_info->tnr_tmp_buf =
			isp_gpu_mem_alloc(size, ISP_GPU_MEM_TYPE_NLFB);

		if (snr_info->tnr_tmp_buf) {
			memset(&buf_type, 0, sizeof(buf_type));
			buf_type.bufferType = BUFFER_TYPE_TNR_REF;
			buf_type.buffer.bufTags = 0;
			buf_type.buffer.vmidSpace = 4;
			isp_split_addr64(snr_info->tnr_tmp_buf->gpu_mc_addr,
					&buf_type.buffer.bufBaseALo,
					&buf_type.buffer.bufBaseAHi);
			buf_type.buffer.bufSizeA = (uint32)
				snr_info->tnr_tmp_buf->mem_size;

			if (isp_send_fw_cmd(isp, CMD_ID_SEND_BUFFER, stream_id,
					FW_CMD_PARA_TYPE_DIRECT, &buf_type,
					sizeof(buf_type)) != RET_SUCCESS) {

				isp_gpu_mem_free(snr_info->tnr_tmp_buf);

				snr_info->tnr_tmp_buf = NULL;
				isp_dbg_print_err
					("in %s suc, send tnr fail size %u\n",
					__func__, size);
			} else {
				snr_info->tnr_tmp_buf_set = 1;
				isp_dbg_print_info
					("in %s suc, send tnr ok size %u\n",
					__func__, size);
				tnr_enable_imp(isp, cid, 1);
			};
		} else {
			isp_dbg_print_info
				("in %s, aloc tnr fail\n", __func__);

		}
	}

	isp_dbg_print_info("<- %s suc\n", __func__);
	return RET_SUCCESS;

 fail:
	for (i = 0; i < STREAM_TMP_BUF_COUNT; i++) {
		if (snr_info->stream_tmp_buf[i]) {

			isp_gpu_mem_free(snr_info->stream_tmp_buf[i]);

			snr_info->stream_tmp_buf[i] = NULL;
		}
	};
	isp_dbg_print_err("<- %s fail\n", __func__);
	return RET_FAILURE;
}

result_t isp_uninit_stream(struct isp_context *isp, enum camera_id cam_id)
{
	struct sensor_info *snr_info;
	uint32 i;
	enum fw_cmd_resp_stream_id stream;
	struct isp_cmd_element *ele = NULL;
	struct isp_mapped_buf_info *img_info = NULL;
	enum stream_id str_id;
	enum camera_id cid = cam_id;
	uint32 cmd;
	CmdStopStream_t stop_stream_para;
	uint32 timeout;
	uint32 out_cnt;
	enum fw_cmd_resp_stream_id stream_id;

	if (!is_para_legal(isp, cam_id)) {
		isp_dbg_print_err
			("-><- %s fail, bad para,cid:%d\n", __func__, cam_id);
		return RET_FAILURE;
	};
	if (!isp->sensor_info[cam_id].stream_inited) {
		isp_dbg_print_info("-><- %s(cid:%d) do none for not started\n",
			__func__, cam_id);
		return RET_SUCCESS;
	}
	isp_dbg_print_info("-> %s(cid:%d)\n", __func__, cam_id);
	isp_get_stream_output_bits(isp, cid, &out_cnt);
	if (out_cnt > 0) {
		isp_dbg_print_info
			("<- %s(cid:%d) fail for there is still %u output\n",
			__func__, cam_id, out_cnt);
		return RET_FAILURE;
	}

	isp_dbg_print_pt("[PT][Cam][IM][%s][%s][%s]\n", PT_EVT_SC, PT_CMD_ES,
			PT_STATUS_SUCC);
	stream_id = isp_get_stream_id_from_cid(isp, cid);
	cmd = CMD_ID_STOP_STREAM;
	stop_stream_para.destroyBuffers = true;

	timeout = (g_isp_env_setting == ISP_ENV_SILICON) ?
			(1000 * 2) : (1000 * 30);

#ifdef DO_SYNCHRONIZED_STOP_STREAM
	if (isp_send_fw_cmd_sync(isp, cmd, stream_id,
				FW_CMD_PARA_TYPE_DIRECT,
				&stop_stream_para,
				sizeof(stop_stream_para),
				timeout,
				NULL, NULL) != RET_SUCCESS) {
#else
	if (isp_send_fw_cmd(isp, cmd, stream_id, FW_CMD_PARA_TYPE_DIRECT,
				&stop_stream_para,
				sizeof(stop_stream_para)) != RET_SUCCESS) {
#endif
		isp_dbg_print_err("in %s,send stop steam fail\n", __func__);
	} else {
		isp_dbg_print_info("in %s, wait stop stream suc\n", __func__);
	};
	isp_dbg_print_pt("[PT][Cam][IM][%s][%s][%s]\n", PT_EVT_RCD, PT_CMD_ES,
			PT_STATUS_SUCC);
	isp->sensor_info[cam_id].stream_inited = 0;

	//if (1 == isp->drv_settings.ir_illuminator_ctrl) {
	/*Force to turn off ir illuminator in pure ir
	 *or rgbir case cause we found sometimes
	 *FW has some issue to turn it off correctly,
	 *so this is kinda wordaround.
	 */
	isp_turn_off_ir_illuminator(isp, cam_id);
	//}
	isp_reset_camera_info(isp, cam_id);
	snr_info = &isp->sensor_info[cam_id];
	stream = isp_get_stream_id_from_cid(isp, cam_id);
	do {
		ele = isp_rm_cmd_from_cmdq_by_stream(isp, stream, false);
		if (ele == NULL)
			break;
		if (ele->mc_addr)
			isp_fw_ret_pl(isp->fw_work_buf_hdl, ele->mc_addr);

		if (ele->gpu_pkg)
			isp_gpu_mem_free(ele->gpu_pkg);

		isp_sys_mem_free(ele);
	} while (ele != NULL);

	for (i = 0; i < STREAM_META_BUF_COUNT; i++) {
		if (snr_info->meta_mc[i]) {
			isp_fw_ret_pl(isp->fw_work_buf_hdl,
				snr_info->meta_mc[i]);
			snr_info->meta_mc[i] = 0;
		}
	}

	for (i = 0; i < STREAM_TMP_BUF_COUNT; i++) {
		if (snr_info->stream_tmp_buf[i]) {
			isp_gpu_mem_free(snr_info->stream_tmp_buf[i]);
			snr_info->stream_tmp_buf[i] = NULL;
		}
	};
	if (snr_info->iic_tmp_buf) {
		isp_gpu_mem_free(snr_info->iic_tmp_buf);
		snr_info->iic_tmp_buf = NULL;
	}

	if (snr_info->tnr_tmp_buf) {
		isp_gpu_mem_free(snr_info->tnr_tmp_buf);
		snr_info->tnr_tmp_buf = NULL;
	}
	if (snr_info->cmd_resp_buf) {
		isp_gpu_mem_free(snr_info->cmd_resp_buf);
		snr_info->cmd_resp_buf = NULL;
	};

	for (str_id = STREAM_ID_PREVIEW; str_id < STREAM_ID_NUM; str_id++) {
		if (snr_info->str_info[str_id].uv_tmp_buf) {
			isp_gpu_mem_free(snr_info->str_info[str_id].uv_tmp_buf);
			snr_info->str_info[str_id].uv_tmp_buf = NULL;
		}
	};
	//

	if (snr_info->tnr_tmp_buf) {
		isp_gpu_mem_free(snr_info->tnr_tmp_buf);
		snr_info->tnr_tmp_buf = NULL;
	}

	if (snr_info->hdr_stats_buf) {
		isp_gpu_mem_free(snr_info->hdr_stats_buf);
		snr_info->hdr_stats_buf = NULL;
	};

	if (snr_info->stage2_tmp_buf) {
		isp_gpu_mem_free(snr_info->stage2_tmp_buf);
		snr_info->stage2_tmp_buf = NULL;
	}

	for (i = 0; i < EMB_DATA_BUF_NUM; i++) {
		if (snr_info->emb_data_buf[i]) {
			isp_gpu_mem_free(snr_info->emb_data_buf[i]);
			snr_info->emb_data_buf[i] = NULL;
		}
	}

	img_info = NULL;
	do {
		if (img_info) {
			isp_unmap_sys_2_mc(isp, img_info);
			isp_sys_mem_free(img_info);
		};
		img_info = (struct isp_mapped_buf_info *)
			isp_list_get_first(&snr_info->take_one_pic_buf_list);
	} while (img_info);
	snr_info->take_one_pic_left_cnt = 0;

	img_info = NULL;
	do {
		if (img_info) {
			isp_unmap_sys_2_mc(isp, img_info);
			isp_sys_mem_free(img_info);
		};
		img_info = (struct isp_mapped_buf_info *)
			isp_list_get_first(&snr_info->take_one_raw_buf_list);
	} while (img_info);
	snr_info->take_one_raw_left_cnt = 0;

	img_info = NULL;
	do {
		if (img_info) {
			isp_unmap_sys_2_mc(isp, img_info);
			isp_sys_mem_free(img_info);
		};
		img_info = (struct isp_mapped_buf_info *)
			isp_list_get_first(&snr_info->rgbir_raw_output_in_fw);
	} while (img_info);

	img_info = NULL;
	do {
		if (img_info) {
			isp_unmap_sys_2_mc(isp, img_info);
			isp_sys_mem_free(img_info);
		};
		img_info = (struct isp_mapped_buf_info *)
			isp_list_get_first(&snr_info->rgbraw_input_in_fw);
	} while (img_info);

	do {
		if (img_info) {
			isp_unmap_sys_2_mc(isp, img_info);
			isp_sys_mem_free(img_info);
		};
		img_info = (struct isp_mapped_buf_info *)
			isp_list_get_first(&snr_info->irraw_input_in_fw);
	} while (img_info);

	img_info = NULL;
	do {
		if (img_info) {
			isp_unmap_sys_2_mc(isp, img_info);
			isp_sys_mem_free(img_info);
		};
		img_info = (struct isp_mapped_buf_info *)
			isp_list_get_first
				(&snr_info->rgbir_frameinfo_input_in_fw);
	} while (img_info);
	isp_dbg_print_info("<- %s suc\n", __func__);
	return RET_SUCCESS;
}

result_t isp_init_stream(struct isp_context *isp, enum camera_id cam_id)
{
	CmdSetRawPktFmt_t raw_fmt;
	enum fw_cmd_resp_stream_id stream_id;

	if (!is_para_legal(isp, cam_id)) {
		isp_dbg_print_err
			("-><- %s fail, bad para,cid:%d\n", __func__, cam_id);
		return RET_FAILURE;
	};

	if (isp->sensor_info[cam_id].stream_inited) {
		isp_dbg_print_info
			("-><- %s(cid:%d),suc do none\n", __func__, cam_id);
		return RET_SUCCESS;
	}

	isp_dbg_print_info("-> %s(cid:%d)\n", __func__, cam_id);

	if (isp_set_clk_info_2_fw(isp) != RET_SUCCESS) {
		isp_dbg_print_err
			("<- %s fail for isp_set_clk_info_2_fw\n", __func__);
		return RET_FAILURE;
	};

	if (isp_set_snr_info_2_fw(isp, cam_id) != RET_SUCCESS) {
		isp_dbg_print_err
			("in %s fail for isp_set_snr_info_2_fw\n", __func__);
		isp->sensor_info[cam_id].calib_enable = false;
	};

	if (isp_setup_stream(isp, cam_id) != RET_SUCCESS) {
		isp_dbg_print_err
			("<- %s fail for isp_setup_stream\n", __func__);
		return RET_FAILURE;
	};

	if (isp_setup_stream_tmp_buf(isp, cam_id) != RET_SUCCESS) {
		isp_dbg_print_err
			("%s fail for isp_setup_stream_tmp_buf\n", __func__);
		return RET_FAILURE;
	};

	if (isp_setup_metadata_buf(isp, cam_id) != RET_SUCCESS) {
		isp_dbg_print_err
			("<- %s fail for isp_setup_metadata_buf\n", __func__);
		return RET_FAILURE;
	};

	stream_id = isp_get_stream_id_from_cid(isp, cam_id);

	if (isp->drv_settings.ignore_meta_at_stage2) {
		CmdIgnoreMeta_t cmd;

		cmd.ignore = 1;
		if (isp_send_fw_cmd(isp, CMD_ID_IGNORE_META,
					stream_id,
					FW_CMD_PARA_TYPE_DIRECT,
					&cmd,
					sizeof(CmdIgnoreMeta_t))
					!= RET_SUCCESS) {
			isp_dbg_print_err
				("in %s, fail ignore meta\n", __func__);
		} else {
			isp_dbg_print_info
				("in %s, ignore meta suc\n", __func__);
		};
	};

	raw_fmt.rawPktFmt = RAW_PKT_FMT_0;
	if (isp->sensor_info[cam_id].cam_type == CAMERA_TYPE_RGBIR)
		raw_fmt.rawPktFmt = RAW_PKT_FMT_6;
	if (isp_send_fw_cmd(isp, CMD_ID_SET_RAW_PKT_FMT,
				stream_id, FW_CMD_PARA_TYPE_DIRECT,
				&raw_fmt,
				sizeof(CmdSetRawPktFmt_t)) != RET_SUCCESS) {
		isp_dbg_print_err("in %s, fail raw_fmt to %i\n",
			__func__, raw_fmt.rawPktFmt);
	} else {
		isp_dbg_print_info("in %s, raw_fmt to %i suc\n",
			__func__, raw_fmt.rawPktFmt);
	};
	isp->sensor_info[cam_id].raw_pkt_fmt = raw_fmt.rawPktFmt;

	isp->sensor_info[cam_id].stream_inited = 1;
	isp_dbg_print_info("<- %s suc\n", __func__);
	return RET_SUCCESS;
}

bool_t isp_is_host_processing_raw(struct isp_context *isp,
					enum camera_id cid)
{
	struct sensor_info *sif;

	sif = &isp->sensor_info[cid];
	if (sif->cam_type == CAMERA_TYPE_RGBIR)
		return TRUE;
	if ((sif->cam_type == CAMERA_TYPE_RGB_BAYER)
	&& isp->drv_settings.process_3a_by_host)
		return TRUE;
	return FALSE;
};


void isp_calc_aspect_ratio_wnd(struct isp_context *isp,
				enum camera_id cid, uint32 w, uint32 h,
				Window_t *aprw)
{
	uint32 raw_height, raw_width;
	struct sensor_info *sif;

	sif = &isp->sensor_info[cid];
	raw_height = sif->raw_height;
	raw_width = sif->raw_width;
	isp_dbg_print_info("in %s %u %u %u %u %u %u\n", __func__,
		raw_width, raw_height, sif->sensor_cfg.window.h_offset,
		sif->sensor_cfg.window.v_offset,
		sif->sensor_cfg.window.h_size,
		sif->sensor_cfg.window.v_size);

	if (((sif->sensor_cfg.window.h_offset != 0)
	|| (sif->sensor_cfg.window.v_offset != 0))
	&& (sif->sensor_cfg.window.h_size != 0)
	&& (sif->sensor_cfg.window.v_size != 0))
		*aprw = sif->sensor_cfg.window;
	else {
		uint32 raw_w_new;
		uint32 raw_h_new;

		aprw->h_offset = 0;
		aprw->v_offset = 0;
		raw_w_new = (w * raw_height) / h;
		if (raw_w_new <= raw_width) {
			aprw->h_size = raw_w_new;
			aprw->v_size = raw_height;
		} else {
			raw_h_new = (raw_width * h) / w;
			aprw->h_size = raw_width;
			aprw->v_size = raw_h_new;
		};
	};

	if (isp->drv_settings.disable_dynamic_aspect_wnd) {
		aprw->h_offset = 0;
		aprw->v_offset = 0;
		aprw->h_size = raw_width;
		aprw->v_size = raw_height;
	};

	if ((aprw->h_size % 2) != 0) {
		aprw->h_size--;
		isp_dbg_print_info("change aprw.window.h_size to %u\n",
			aprw->h_size);
	};

	if ((aprw->v_size % 2) != 0) {
		aprw->v_size--;
		isp_dbg_print_info("change aprw.window.v_size to %u\n",
			aprw->v_size);
	};

	if (isp_is_host_processing_raw(isp, cid)
	&& sif->str_info[STREAM_ID_RGBIR].width
	&& sif->str_info[STREAM_ID_RGBIR].height) {
		aprw->h_offset = 0;
		aprw->v_offset = 0;
		aprw->h_size = sif->str_info[STREAM_ID_RGBIR].width;
		aprw->v_size = sif->str_info[STREAM_ID_RGBIR].height;
		isp_dbg_print_info("rgbir rechange aprw to %u:%u\n",
				aprw->h_size, aprw->v_size);
	};

	if (aprw->h_size < raw_width) {
		aprw->h_offset = (raw_width - aprw->h_size) / 2;
		if ((aprw->h_offset % 2) != 0)
			aprw->h_offset--;
	};

	if (aprw->v_size < raw_height) {
		aprw->v_offset = (raw_height - aprw->v_size) / 2;
		if ((aprw->v_offset % 2) != 0)
			aprw->v_offset--;
	};

}


result_t isp_setup_aspect_ratio_wnd(struct isp_context *isp,
				enum camera_id cid, uint32 w, uint32 h)
{
	CmdSetAspectRatioWindow_t aprw;
	uint32 raw_height, raw_width;
	enum fw_cmd_resp_stream_id stream_id;
	struct sensor_info *sif;

	sif = &isp->sensor_info[cid];
	raw_height = sif->raw_height;
	raw_width = sif->raw_width;

	isp_calc_aspect_ratio_wnd(isp, cid, w, h, &aprw.window);
	//TODO: below line will crash, need to dig why
	//isp->sensor_info[cid].asprw_wind = aprw.window;
	isp->sensor_info[cid].asprw_wind.h_offset = aprw.window.h_offset;
	isp->sensor_info[cid].asprw_wind.v_offset = aprw.window.v_offset;
	isp->sensor_info[cid].asprw_wind.h_size = aprw.window.h_size;
	isp->sensor_info[cid].asprw_wind.v_size = aprw.window.v_size;

	stream_id = isp_get_stream_id_from_cid(isp, cid);
	if (isp_is_mem_camera(isp, cid)) {
		isp_dbg_print_info
			("><- %s suc,do none for memcam\n", __func__);
		return RET_SUCCESS;
	} else if (isp_send_fw_cmd(isp, CMD_ID_SET_ASPECT_RATIO_WINDOW,
				stream_id, FW_CMD_PARA_TYPE_DIRECT,
				&aprw, sizeof(aprw)) != RET_SUCCESS) {
		isp_dbg_print_err
			("-><- %s fail for SET_ASP_RAT_WINDOW\n", __func__);
		return RET_FAILURE;
	}

	isp_dbg_print_info
		("><- %s suc, aspr x:y %u:%u,w:h %u:%u\n", __func__,
		aprw.window.h_offset, aprw.window.v_offset,
		aprw.window.h_size, aprw.window.v_size);
	return RET_SUCCESS;
}


result_t isp_setup_output(struct isp_context *isp, enum camera_id cid,
				enum stream_id str_id)
{
	enum fw_cmd_resp_stream_id stream_id;
	CmdSetPreviewOutProp_t preview_prop;
	CmdSetPreviewFrameRateRatio_t preview_ratio;
	CmdSetVideoOutProp_t video_prop;
	CmdSetVideoFrameRateRatio_t video_ratio;
	CmdSetZslOutProp_t zsl_prop;
	CmdSetZslFrameRateRatio_t zsl_ratio;
	struct isp_stream_info *sif;

	uint32 set_out_prop_cmd;
	void *set_out_pkg;
	ImageProp_t *out_prop;
	uint32 set_out_pkg_len;
	uint32 set_ratio_cmd;
	void *set_ratio_pkg;
	uint32 set_ratio_pkg_len;
	uint32 enable_cmd;
	char *pt_cmd_str = "";

	if (str_id == STREAM_ID_PREVIEW)
		pt_cmd_str = PT_CMD_SP;
	else if (str_id == STREAM_ID_VIDEO)
		pt_cmd_str = PT_CMD_SV;
	else if (str_id == STREAM_ID_ZSL)
		pt_cmd_str = PT_CMD_SZ;

	isp_dbg_print_pt("[PT][Cam][IM][%s][%s][%s]\n", PT_EVT_SC, pt_cmd_str,
			PT_STATUS_SUCC);

	if (!is_para_legal(isp, cid)) {
		isp_dbg_print_err("-><- %s fail, bad para,cid:%d,str:%d\n",
			__func__, cid, str_id);
		return RET_FAILURE;
	};

	if (str_id == STREAM_ID_RGBIR) {
		isp_dbg_print_info("-><-%s,cid:%u suc,none for RGBIR stream\n",
			__func__, cid);
		return RET_SUCCESS;
	}
	stream_id = isp_get_stream_id_from_cid(isp, cid);
	sif = &isp->sensor_info[cid].str_info[str_id];
	isp_dbg_print_info("-> %s cid:%d,str:%d\n", __func__, cid, str_id);

	if (sif->start_status == START_STATUS_STARTED) {
		isp_dbg_print_info("<- %s,suc do none\n", __func__);
		return RET_SUCCESS;
	};
	if (sif->start_status == START_STATUS_START_FAIL) {
		isp_dbg_print_info("<- %s,fail do none\n", __func__);
		return RET_SUCCESS;
	};

	sif->start_status = START_STATUS_STARTING;
	if (str_id == STREAM_ID_PREVIEW) {
		set_out_prop_cmd = CMD_ID_SET_PREVIEW_OUT_PROP;
		set_out_pkg = &preview_prop;
		set_out_pkg_len = sizeof(preview_prop);
		set_ratio_cmd = CMD_ID_SET_PREVIEW_FRAME_RATE_RATIO;
		set_ratio_pkg = &preview_ratio;
		set_ratio_pkg_len = sizeof(preview_ratio);
		if (sif->max_fps_denominator == 0) {
			preview_ratio.ratio =
				isp->sensor_info[cid].sensor_cfg.frame_rate
				/ 30;
			preview_ratio.ratio /= 1000;
			if (preview_ratio.ratio == 0)
				preview_ratio.ratio = 1;
		} else {
			preview_ratio.ratio =
				isp->sensor_info[cid].sensor_cfg.frame_rate /
				(sif->max_fps_numerator /
				sif->max_fps_denominator);
			preview_ratio.ratio /= 1000;
			if (preview_ratio.ratio == 0)
				preview_ratio.ratio = 1;
		}
		enable_cmd = CMD_ID_ENABLE_PREVIEW;
		out_prop = &preview_prop.outProp;
		if (isp->sensor_info[cid].sensor_cfg.frame_rate) {
			uint32 tmp_ratio =
				isp->sensor_info[cid].sensor_cfg.frame_rate
				/ (30 * 1000);
			if (tmp_ratio > preview_ratio.ratio)
				preview_ratio.ratio = tmp_ratio;
		};
	} else if (str_id == STREAM_ID_VIDEO) {
		set_out_prop_cmd = CMD_ID_SET_VIDEO_OUT_PROP;
		set_out_pkg = &video_prop;
		set_out_pkg_len = sizeof(video_prop);
		set_ratio_cmd = CMD_ID_SET_VIDEO_FRAME_RATE_RATIO;
		set_ratio_pkg = &video_ratio;
		set_ratio_pkg_len = sizeof(video_ratio);
		video_ratio.ratio = 1;
		if (sif->max_fps_denominator == 0) {
			video_ratio.ratio = 1;
		} else {
			video_ratio.ratio =
				isp->sensor_info[cid].sensor_cfg.frame_rate /
				(sif->max_fps_numerator /
				sif->max_fps_denominator);
			video_ratio.ratio /= 1000;
			if (video_ratio.ratio == 0)
				video_ratio.ratio = 1;
		}
		enable_cmd = CMD_ID_ENABLE_VIDEO;
		out_prop = &video_prop.outProp;
	} else if (str_id == STREAM_ID_ZSL) {
		set_out_prop_cmd = CMD_ID_SET_ZSL_OUT_PROP;
		set_out_pkg = &zsl_prop;
		set_out_pkg_len = sizeof(zsl_prop);
		set_ratio_cmd = CMD_ID_SET_ZSL_FRAME_RATE_RATIO;
		set_ratio_pkg = &zsl_ratio;
		set_ratio_pkg_len = sizeof(zsl_ratio);
		zsl_ratio.ratio = 1;
		if (sif->max_fps_denominator == 0) {
			zsl_ratio.ratio = 1;
		} else {
			zsl_ratio.ratio =
				isp->sensor_info[cid].sensor_cfg.frame_rate /
				(sif->max_fps_numerator /
				sif->max_fps_denominator);
			zsl_ratio.ratio /= 1000;
			if (zsl_ratio.ratio == 0)
				zsl_ratio.ratio = 1;
		}
		enable_cmd = CMD_ID_ENABLE_ZSL;
		out_prop = &zsl_prop.outProp;
	} else if (str_id == STREAM_ID_RGBIR_IR) {
		out_prop = &video_prop.outProp;
		if (!isp_get_str_out_prop(sif, out_prop)) {
			isp_dbg_print_err("%s fail,get out prop\n", __func__);
			return RET_FAILURE;
		}
		goto do_kick_off;
	} else {
		sif->start_status = START_STATUS_START_FAIL;
		isp_dbg_print_err
			("-><- %s fail, bad str %u\n", __func__, str_id);
		return RET_FAILURE;
	}

	if (!isp_get_str_out_prop(sif, out_prop)) {
		isp_dbg_print_err("<- %s fail,get out prop\n", __func__);
		return RET_FAILURE;
	};


	isp_dbg_print_info("%s,cid %d, stream %d\n", __func__, cid, stream_id);

	isp_dbg_print_info("in %s,fmt %s,w:h=%u:%u,lp:%u,cp%u\n", __func__,
		isp_dbg_get_out_fmt_str(out_prop->imageFormat),
		out_prop->width, out_prop->height,
		out_prop->lumaPitch, out_prop->chromaPitch);

	if (!sif->is_perframe_ctl) {
		if (isp_send_fw_cmd(isp, set_out_prop_cmd, stream_id,
				FW_CMD_PARA_TYPE_DIRECT, set_out_pkg,
				set_out_pkg_len) != RET_SUCCESS) {
			sif->start_status = START_STATUS_START_FAIL;
			isp_dbg_print_err("%s fail,set out prop\n", __func__);
			return RET_FAILURE;
		};
	}

	if (isp_send_fw_cmd(isp, set_ratio_cmd, stream_id,
			FW_CMD_PARA_TYPE_DIRECT, set_ratio_pkg,
			set_ratio_pkg_len) != RET_SUCCESS) {
		sif->start_status = START_STATUS_START_FAIL;
		isp_dbg_print_err("<- %s fail,set ratio\n", __func__);
		return RET_FAILURE;
	}

	isp_dbg_print_err("in %s,set ratio %d\n",
		__func__, *((uint32 *) set_ratio_pkg));


	if ((sif->height == 0) || (sif->width == 0)) {
		isp_dbg_print_info("<- %s,suc, to be enabled\n", __func__);
		return RET_SUCCESS;
	};

	if (!sif->is_perframe_ctl) {
		if (isp_send_fw_cmd(isp, enable_cmd, stream_id,
					FW_CMD_PARA_TYPE_DIRECT,
					NULL, 0) != RET_SUCCESS) {
			sif->start_status = START_STATUS_START_FAIL;
			isp_dbg_print_err("<- %s,enable fail\n", __func__);
			return RET_FAILURE;
		};
	};
 do_kick_off:
	if (!isp->sensor_info[cid].start_str_cmd_sent) {
		if (isp_kickoff_stream(isp, cid, out_prop->width,
					out_prop->height) != RET_SUCCESS) {
			isp_dbg_print_err
				("in %s, kickoff stream fail\n", __func__);
		} else {
			isp->sensor_info[cid].status = START_STATUS_STARTED;
			isp_dbg_print_info
				("in %s, kickoff stream suc\n", __func__);
		};

		isp->sensor_info[cid].start_str_cmd_sent = 1;
	}
	sif->start_status = START_STATUS_STARTED;
	isp_dbg_print_info("<- %s,suc\n", __func__);
	isp_dbg_print_pt("[PT][Cam][IM][%s][%s][%s]\n", PT_EVT_RCD, pt_cmd_str,
			PT_STATUS_SUCC);
	return RET_SUCCESS;
}

result_t isp_setup_ae_range(struct isp_context *isp, enum camera_id cid)
{
	result_t ret = RET_FAILURE;
	CmdAeSetAnalogGainRange_t a_gain;
	CmdAeSetDigitalGainRange_t d_gain;
	CmdAeSetItimeRange_t i_time = {0};
	enum fw_cmd_resp_stream_id stream_id;
	struct sensor_info *sif;

	if (!is_para_legal(isp, cid)) {
		isp_dbg_print_err("-><- %s fail, bad para,isp:%p,cid:%d\n",
			__func__, isp, cid);
		goto quit;
	};
	stream_id = isp_get_stream_id_from_cid(isp, cid);
	sif = &isp->sensor_info[cid];

	if ((sif->aec_mode != AE_MODE_AUTO)
	|| !sif->calib_enable || !sif->calib_set_suc) {
		isp_dbg_print_info
			("-><- %s suc,do none,cid %u\n", __func__, cid);
		ret = RET_SUCCESS;
		goto quit;
	};

	isp_dbg_print_info("-> %s, cid %u\n", __func__, cid);
	a_gain.minGain = sif->sensor_cfg.exposure.min_gain;
	a_gain.maxGain = sif->sensor_cfg.exposure.max_gain;

	if ((a_gain.maxGain == 0) || (a_gain.minGain > a_gain.maxGain)) {
		isp_dbg_print_err
			("in %s fail bad a_gain\n", __func__);
	} else if (isp_send_fw_cmd(isp, CMD_ID_AE_SET_ANALOG_GAIN_RANGE,
				stream_id, FW_CMD_PARA_TYPE_DIRECT,
				&a_gain, sizeof(a_gain)) != RET_SUCCESS) {
		isp_dbg_print_err("in %s fail set a_gain\n", __func__);
	} else {
		isp_dbg_print_info("in %s ana gain range (%d,%d)\n",
			__func__, a_gain.minGain, a_gain.maxGain);
	};

	d_gain.minGain = sif->sensor_cfg.exposure.min_digi_gain;
	d_gain.maxGain = sif->sensor_cfg.exposure.max_digi_gain;
	if ((d_gain.maxGain == 0) || (d_gain.minGain > d_gain.maxGain)) {
		isp_dbg_print_err("in %s fail bad d_gain\n", __func__);
	} else if (isp_send_fw_cmd(isp, CMD_ID_AE_SET_DIGITAL_GAIN_RANGE,
				stream_id, FW_CMD_PARA_TYPE_DIRECT,
				&d_gain, sizeof(d_gain)) != RET_SUCCESS) {
		isp_dbg_print_err
			("in %s fail set d_gain\n", __func__);
	} else {
		isp_dbg_print_info("in %s dig gain range (%d,%d)\n",
			__func__, d_gain.minGain, d_gain.maxGain);
	};

	i_time.minItime = sif->sensor_cfg.exposure.min_integration_time;
	i_time.maxItime = sif->sensor_cfg.exposure.max_integration_time;

	if (isp->drv_settings.low_light_fps) {
		i_time.maxItimeLowLight = 1000000 /
			isp->drv_settings.low_light_fps;
		isp_dbg_print_info("set maxItimeLowLight to %u\n",
			i_time.maxItimeLowLight);
		//Set it again for Registry control to
		//avoid overridden by Scene mode
		sif->enable_dynamic_frame_rate = TRUE;
	};

	if ((i_time.maxItime == 0) || (i_time.minItime > i_time.maxItime)
	|| (i_time.maxItimeLowLight &&
	(i_time.maxItime > i_time.maxItimeLowLight))) {
		isp_dbg_print_err
		("in %s fail bad itime range normal(%d,%d),lowlight(%d,%d)\n",
			__func__,
			i_time.minItime, i_time.maxItime,
			i_time.minItimeLowLight, i_time.maxItimeLowLight);
	} else if (isp_send_fw_cmd(isp, CMD_ID_AE_SET_ITIME_RANGE,
				stream_id, FW_CMD_PARA_TYPE_DIRECT,
				&i_time, sizeof(i_time)) != RET_SUCCESS) {
		isp_dbg_print_err("in %s fail set i_time\n", __func__);
	} else {
		isp_dbg_print_info
			("in %s itime range normal(%d,%d),lowlight(%d,%d)\n",
			__func__,
			i_time.minItime, i_time.maxItime,
			i_time.minItimeLowLight, i_time.maxItimeLowLight);
	};

	ret = RET_SUCCESS;
	isp_dbg_print_info("<- %s suc\n", __func__);
 quit:
	return ret;
}

result_t isp_setup_ae(struct isp_context *isp, enum camera_id cid)
{
	result_t ret = RET_FAILURE;
	enum fw_cmd_resp_stream_id stream_id;
	struct sensor_info *sif;
	CmdAeSetFlicker_t flick;
	CmdAeSetRegion_t aeroi;

	if (!is_para_legal(isp, cid)) {
		isp_dbg_print_err("-><- %s fail, bad para,isp:%p,cid:%d\n",
			__func__, isp, cid);
		goto quit;
	};
	isp_dbg_print_info("-> %s, cid %u\n", __func__, cid);
	stream_id = isp_get_stream_id_from_cid(isp, cid);
	sif = &isp->sensor_info[cid];

	while (true) {
		CmdAeSetMode_t ae;

		if (isp->drv_settings.ae_default_auto == 0)
			sif->aec_mode = AE_MODE_MANUAL;

		if (sif->aec_mode != AE_MODE_AUTO) {
			isp_dbg_print_info("in %s ae not auto(%u), do none\n",
				__func__, sif->aec_mode);
			break;
		};
		if (!sif->calib_enable || !sif->calib_set_suc) {
			isp_dbg_print_info
				("in %s not set ae to auto for no calib\n",
				__func__);
			break;
		};
		ae.mode = sif->aec_mode;
		if (isp_send_fw_cmd(isp, CMD_ID_AE_SET_MODE, stream_id,
				FW_CMD_PARA_TYPE_DIRECT, &ae,
				sizeof(ae)) != RET_SUCCESS) {
			isp_dbg_print_err("in %s fail set ae mode(%s)\n",
				__func__, isp_dbg_get_ae_mode_str(ae.mode));
		} else {
			isp_dbg_print_info("in %s set ae mode(%s) suc\n",
				__func__, isp_dbg_get_ae_mode_str(ae.mode));
		};
		break;
	}

	if ((sif->flick_type_set == AE_FLICKER_TYPE_50HZ)
	|| (sif->flick_type_set == AE_FLICKER_TYPE_60HZ)) {
		flick.flickerType = sif->flick_type_set;
		if (isp_send_fw_cmd(isp, CMD_ID_AE_SET_FLICKER, stream_id,
				FW_CMD_PARA_TYPE_DIRECT,
				&flick, sizeof(flick)) != RET_SUCCESS) {
			isp_dbg_print_err("in %s fail set flick\n", __func__);
		} else {
			isp_dbg_print_info("in %s set flick %d suc\n",
				__func__, flick.flickerType);
		};
	} else
		isp_dbg_print_info("in %s no set flick for off\n", __func__);

	aeroi.window.h_offset = sif->ae_roi.h_offset;
	aeroi.window.v_offset = sif->ae_roi.v_offset;
	aeroi.window.h_size = sif->ae_roi.h_size;
	aeroi.window.v_size = sif->ae_roi.v_size;

	// Overwrite by Registry settings
	switch (cid) {
	case CAMERA_ID_REAR:
		if (roi_window_isvalid(&isp->drv_settings.rear_roi_ae_wnd))
			aeroi.window = isp->drv_settings.rear_roi_ae_wnd;
		break;
	case CAMERA_ID_FRONT_LEFT:
		if (roi_window_isvalid(&isp->drv_settings.frontl_roi_ae_wnd))
			aeroi.window = isp->drv_settings.frontl_roi_ae_wnd;
		break;
	case CAMERA_ID_FRONT_RIGHT:
		if (roi_window_isvalid(&isp->drv_settings.frontr_roi_ae_wnd))
			aeroi.window = isp->drv_settings.frontr_roi_ae_wnd;
		break;
	case CAMERA_ID_MEM:
		break;
	case CAMERA_ID_MAX:
		isp_dbg_print_err("in %s wrong cid %d\n", __func__, cid);
		break;
	}

	if ((aeroi.window.h_size == 0) || (aeroi.window.v_size == 0)) {
		if (sif->sensor_cfg.prop.cfaPattern == CFA_PATTERN_PURE_IR) {
			struct sensor_hw_parameter para;

			if (isp_snr_get_hw_parameter(isp, cid, &para)
							== RET_SUCCESS) {
				aeroi.window = para.ae_roi_wnd;
			};
		} else if (cid == CAMERA_ID_MEM) {
			enum camera_id cb_cid = isp_get_rgbir_cid(isp);
			struct sensor_hw_parameter para;

			if (isp_snr_get_hw_parameter(isp, cb_cid, &para)
							== RET_SUCCESS) {
				aeroi.window = para.ae_roi_wnd;
			};

		} else if (sif->cur_calib_item) {
			// If no customeized ROI, we will set
			// AE ROI as the whole frame resolution
			aeroi.window.h_offset = 0;
			aeroi.window.v_offset = 0;
			aeroi.window.h_size = 0;
			aeroi.window.v_size = 0;
		}
	};

	if ((aeroi.window.h_size == 0) || (aeroi.window.v_size == 0)) {
		isp_dbg_print_err
			("in %s NO initial AE ROI\n", __func__);
	} else if (isp_send_fw_cmd(isp, CMD_ID_AE_SET_REGION, stream_id,
				FW_CMD_PARA_TYPE_DIRECT,
				&aeroi, sizeof(aeroi)) != RET_SUCCESS) {
		isp_dbg_print_err
			("in %s fail set stream_id[%u] aeroi[%u,%u,%u,%u]\n",
			__func__, stream_id,
			aeroi.window.h_offset, aeroi.window.v_offset,
			aeroi.window.h_size, aeroi.window.v_size);
	} else {
		isp_dbg_print_info
			("in %s set stream_id[%u] aeroi [%u,%u,%u,%u] suc\n",
			__func__, stream_id,
			aeroi.window.h_offset, aeroi.window.v_offset,
			aeroi.window.h_size, aeroi.window.v_size);
	};

	if ((sif->aec_mode == AE_MODE_AUTO) && sif->calib_enable
	&& sif->calib_set_suc)
		isp_setup_ae_range(isp, cid);
	else
		isp_dbg_print_info("in %s no need set ae range\n", __func__);

	/*todo to be add later for 0.6 fw doesn't support */

	sif->hdr_enabled_suc = 0;
	if (sif->hdr_enable && sif->hdr_prop_set_suc) {
		if (isp_send_fw_cmd(isp, CMD_ID_AE_ENABLE_HDR, stream_id,
				FW_CMD_PARA_TYPE_DIRECT,
				NULL, 0) != RET_SUCCESS) {
			isp_dbg_print_err("in %s fail enable hdr\n", __func__);
		} else {
			sif->hdr_enabled_suc = 1;
			isp_dbg_print_err("in %s enable hdr suc\n", __func__);
		}
	} else
		isp_dbg_print_info("in %s no set hdr\n", __func__);

	/*TODO, to be add later for ISP2.1 doesn't support Flasglight */

	ret = RET_SUCCESS;
	isp_dbg_print_info("<- %s, suc\n", __func__);
 quit:
	return ret;
};

result_t isp_setup_af(struct isp_context *isp, enum camera_id cid)
{
	result_t ret = RET_FAILURE;
	enum fw_cmd_resp_stream_id stream_id;
	struct sensor_info *sif;
	CmdAfSetRegion_t rgn;
	uint32 i;
	uint32 rgn_cnt;

	if (!is_para_legal(isp, cid)) {
		isp_dbg_print_err("-><- %s fail, bad para,isp:%p,cid:%d\n",
			__func__, isp, cid);
		goto quit;
	};

	stream_id = isp_get_stream_id_from_cid(isp, cid);
	sif = &isp->sensor_info[cid];
	if (!sif->calib_enable || !sif->calib_set_suc) {
		ret = RET_SUCCESS;
		isp_dbg_print_err("-><- %s,cid %u,suc do none\n",
			__func__, cid);
		goto quit;
	}

	isp_dbg_print_info("-> %s, cid %u\n", __func__, cid);
	isp_dbg_print_info("mode %s, focus limit(%d,%d), step %d\n",
			isp_dbg_get_af_mode_str(sif->af_mode),
			sif->lens_range.minLens,
			sif->lens_range.maxLens, sif->lens_range.stepLens);
	if (sif->lens_range.maxLens == 0) {
		isp_dbg_print_info
			("<- %s suc,do none for 0 maxLen\n", __func__);
		ret = RET_SUCCESS;
		goto quit;
	}

	if (isp->drv_settings.af_default_auto == 0)
		sif->af_mode = AF_MODE_MANUAL;

	if ((sif->af_mode == AF_MODE_ONE_SHOT)
	|| (sif->af_mode == AF_MODE_CONTINUOUS_PICTURE)
	|| (sif->af_mode == AF_MODE_CONTINUOUS_VIDEO)) {
		CmdAfSetLensRange_t r;

		r.lensRange.minLens = sif->lens_range.minLens;
		r.lensRange.maxLens = sif->lens_range.maxLens;
		r.lensRange.stepLens = sif->lens_range.stepLens;
		isp_dbg_print_info("focus limit(%d,%d), step %d\n",
				r.lensRange.minLens, r.lensRange.maxLens,
				r.lensRange.stepLens);
		if (r.lensRange.maxLens == 0) {
			isp_dbg_print_info("maxLens is 0, do nothing\n");
		} else if (isp_send_fw_cmd(isp, CMD_ID_AF_SET_LENS_RANGE,
					stream_id, FW_CMD_PARA_TYPE_DIRECT,
					&r, sizeof(r)) != RET_SUCCESS) {
			isp_dbg_print_err
				("in %s,fail set lens range [%u,%u]%u)\n",
				__func__, r.lensRange.minLens,
				r.lensRange.maxLens, r.lensRange.stepLens);
		} else {
			isp_dbg_print_info
				("in %s,suc set lens range [%u,%u]%u)\n",
				__func__, r.lensRange.minLens,
				r.lensRange.maxLens, r.lensRange.stepLens);
		}
	};

	if (sif->af_mode != AF_MODE_MANUAL) {
		CmdAfSetMode_t m;

		m.mode = sif->af_mode;
		if (isp_send_fw_cmd(isp, CMD_ID_AF_SET_MODE, stream_id,
				FW_CMD_PARA_TYPE_DIRECT, &m,
				sizeof(m)) != RET_SUCCESS) {
			isp_dbg_print_err("in %s,fail set afmode mode(%s)\n",
				__func__, isp_dbg_get_af_mode_str(m.mode));
		} else {
			isp_dbg_print_info("in %s,suc set afmode mode(%s)\n",
				__func__, isp_dbg_get_af_mode_str(m.mode));

			if (isp_send_fw_cmd(isp, CMD_ID_AF_TRIG, stream_id,
					FW_CMD_PARA_TYPE_DIRECT,
					NULL, 0) != RET_SUCCESS) {
				isp_dbg_print_err
					("in %s,fail set af trig\n", __func__);
			} else {
				isp_dbg_print_info
					("in %s,suc set af trig\n", __func__);
			};
		}
	};

	rgn_cnt = 0;

	for (i = 0; i < MAX_AF_ROI_NUM; i++) {
		rgn.window = sif->af_roi[i];

		// Overwrite by Registry settings
		switch (cid) {
		case CAMERA_ID_REAR:
			if (roi_window_isvalid
			(&isp->drv_settings.rear_roi_af_wnd[i])) {
				rgn.window =
					isp->drv_settings.rear_roi_af_wnd[i];
			}
			break;
		case CAMERA_ID_FRONT_LEFT:
			if (roi_window_isvalid
			(&isp->drv_settings.frontl_roi_af_wnd[i])) {
				rgn.window =
					isp->drv_settings.frontl_roi_af_wnd[i];
			}
			break;
		case CAMERA_ID_FRONT_RIGHT:
			if (roi_window_isvalid
			(&isp->drv_settings.frontr_roi_af_wnd[i])) {
				rgn.window =
					isp->drv_settings.frontr_roi_af_wnd[i];
			}
			break;
		default:
			break;
		}

		if (i == 0)
			rgn.windowId = AF_WINDOW_ID_A;
		else if (i == 1)
			rgn.windowId = AF_WINDOW_ID_B;
		else
			rgn.windowId = AF_WINDOW_ID_C;

		if ((rgn.window.h_size == 0) || (rgn.window.v_size == 0))
			continue;
		rgn_cnt++;
		if (isp_send_fw_cmd(isp, CMD_ID_AF_SET_REGION, stream_id,
				FW_CMD_PARA_TYPE_DIRECT,
				&rgn, sizeof(rgn)) != RET_SUCCESS) {
			isp_dbg_print_err
				("in %s fail set af rgn%u[%u,%u,%u,%u]\n",
				__func__, i,
				rgn.window.h_offset, rgn.window.v_offset,
				rgn.window.h_size, rgn.window.v_size);
		} else {
			isp_dbg_print_info
				("in %s suc set af rgn%u[%u,%u,%u,%u]\n",
				__func__, i,
				rgn.window.h_offset, rgn.window.v_offset,
				rgn.window.h_size, rgn.window.v_size);
		};
	}

//	don't set, use fw default value
//	If no customeized ROI, we will set AF ROI as the whole frame resolution

	if ((sif->flash_assist_mode == FOCUS_ASSIST_MODE_ON)
	|| (sif->flash_assist_mode == FOCUS_ASSIST_MODE_AUTO)) {
		CmdAfSetFocusAssistMode_t m;

		m.mode = sif->flash_assist_mode;
		if (isp_send_fw_cmd(isp, CMD_ID_AF_SET_FOCUS_ASSIST_MODE,
				stream_id, FW_CMD_PARA_TYPE_DIRECT, &m,
				sizeof(m)) != RET_SUCCESS) {
			isp_dbg_print_err("in %s fail set af assist %u\n",
				__func__, m.mode);
		} else {
			isp_dbg_print_info("in %s suc set af assist %u\n",
				__func__, m.mode);
		};
	};

	if (sif->flash_assist_pwr != 20) {
		CmdAfSetFocusAssistPowerLevel_t pwr;

		pwr.powerLevel = sif->flash_assist_pwr;
		if (isp_send_fw_cmd(isp,
				CMD_ID_AF_SET_FOCUS_ASSIST_POWER_LEVEL,
				stream_id, FW_CMD_PARA_TYPE_DIRECT,
				&pwr, sizeof(pwr)) != RET_SUCCESS) {
			isp_dbg_print_err("in %s,fail set asspwr(%u)\n",
				__func__, sif->flash_assist_pwr);
		} else {
			isp_dbg_print_info("in %s,suc set asspwr(%u)\n",
				__func__, sif->flash_assist_pwr);
		};
	};

	ret = RET_SUCCESS;
	isp_dbg_print_info("<- %s, suc\n", __func__);
 quit:
	return ret;
}

result_t isp_setup_awb(struct isp_context *isp, enum camera_id cid)
{
	result_t ret = RET_FAILURE;
	enum fw_cmd_resp_stream_id stream_id;
	struct sensor_info *sif;
	CmdAwbSetRegion_t rgn;

	if (!is_para_legal(isp, cid)) {
		isp_dbg_print_err("-><- %s fail, bad para,isp:%p,cid:%d\n",
			__func__, isp, cid);
		goto quit;
	};

	stream_id = isp_get_stream_id_from_cid(isp, cid);
	sif = &isp->sensor_info[cid];

	isp_dbg_print_info("%s, cid:%u,stream:%u\n", __func__, cid, stream_id);

	if (isp->drv_settings.awb_default_auto == 0)
		sif->awb_mode = AWB_MODE_MANUAL;

	/*Disable AWB in case of pure IR camera */
	if (sif->sensor_cfg.prop.cfaPattern == CFA_PATTERN_PURE_IR)
		sif->awb_mode = AWB_MODE_MANUAL;

	if (sif->awb_mode != AWB_MODE_MANUAL) {
		CmdAwbSetMode_t m;

		m.mode = sif->awb_mode;
		if (isp_send_fw_cmd(isp, CMD_ID_AWB_SET_MODE, stream_id,
				FW_CMD_PARA_TYPE_DIRECT, &m,
				sizeof(m)) != RET_SUCCESS) {
			isp_dbg_print_err("in %s,fail set mode(%s)\n",
				__func__, isp_dbg_get_awb_mode_str(m.mode));
		} else {
			isp_dbg_print_info("in %s,suc set mode(%s)\n",
				__func__, isp_dbg_get_awb_mode_str(m.mode));
		}
	};

	rgn.window.h_offset = sif->awb_region.h_offset;
	rgn.window.v_offset = sif->awb_region.v_offset;
	rgn.window.h_size = sif->awb_region.h_size;
	rgn.window.v_size = sif->awb_region.v_size;

	// Overwrite by Registry settings
	switch (cid) {
	case CAMERA_ID_REAR:
		if (roi_window_isvalid(&isp->drv_settings.rear_roi_awb_wnd))
			rgn.window = isp->drv_settings.rear_roi_awb_wnd;
		break;
	case CAMERA_ID_FRONT_LEFT:
		if (roi_window_isvalid
				(&isp->drv_settings.frontl_roi_awb_wnd)) {
			rgn.window = isp->drv_settings.frontl_roi_awb_wnd;
		}
		break;
	case CAMERA_ID_FRONT_RIGHT:
		if (roi_window_isvalid
				(&isp->drv_settings.frontr_roi_awb_wnd)) {
			rgn.window = isp->drv_settings.frontr_roi_awb_wnd;
		}
		break;
	default:
		break;
	}

//	If no customeized ROI, we will set WB ROI as the whole frame resolution
	if ((rgn.window.h_size == 0) || (rgn.window.v_size == 0)) {
		if (sif->cur_calib_item) {
			rgn.window.h_offset = 0;
			rgn.window.v_offset = 0;
			rgn.window.h_size = sif->cur_calib_item->width;
			rgn.window.v_size = sif->cur_calib_item->height;
		}
	};

	if ((rgn.window.h_size == 0) || (rgn.window.v_size == 0)) {
		isp_dbg_print_err
			("in %s NO initial WB ROI\n", __func__);
	} else if (isp_send_fw_cmd(isp, CMD_ID_AWB_SET_REGION, stream_id,
				FW_CMD_PARA_TYPE_DIRECT, &rgn,
				sizeof(rgn)) != RET_SUCCESS) {
		isp_dbg_print_err
			("in %s fail set roi\n", __func__);
	} else {
		isp_dbg_print_info("in %s suc set roi [%u, %u, %u, %u]\n",
			__func__, rgn.window.h_offset, rgn.window.v_offset
			, rgn.window.h_size, rgn.window.v_size);
	};

	if ((sif->awb_mode == AWB_MODE_MANUAL) && sif->color_temperature) {
		CmdAwbSetColorTemperature_t t;

		t.colorTemperature = sif->color_temperature;
		if (isp_send_fw_cmd(isp, CMD_ID_AWB_SET_COLOR_TEMPERATURE,
				stream_id, FW_CMD_PARA_TYPE_DIRECT, &t,
				sizeof(t)) != RET_SUCCESS) {
			isp_dbg_print_err
				("in %s, fail set color temperature(%u)\n",
				__func__, t.colorTemperature);
		} else {
			isp_dbg_print_info
				("in %s, suc set color temperature(%u)\n",
				__func__, t.colorTemperature);
		};
	} else
		isp_dbg_print_info("in %s suc no need set temper\n", __func__);

	if (sif->awb_mode == AWB_MODE_MANUAL
	&& isp->drv_settings.fix_wb_lighting_index != -1) {
		if (set_wb_light_imp(isp, cid,
				isp->drv_settings.fix_wb_lighting_index)
				!= IMF_RET_SUCCESS) {
			isp_dbg_print_err
				("in %s, fail set light index\n", __func__);
		} else {
			isp_dbg_print_info
				("in %s, suc set light index (%u)\n", __func__,
				isp->drv_settings.fix_wb_lighting_index);
		};
	}

	ret = RET_SUCCESS;
	isp_dbg_print_info("<- %s (%s), suc\n", __func__,
		isp_dbg_get_awb_mode_str(sif->awb_mode));
 quit:
	return ret;
}

result_t isp_setup_ctls(struct isp_context *isp, enum camera_id cid)
{
	result_t ret = RET_SUCCESS;
	enum fw_cmd_resp_stream_id stream_id;
	struct sensor_info *sif;

	if (!is_para_legal(isp, cid)) {
		isp_dbg_print_err("-><- %s fail, bad para,isp:%p,cid:%d\n",
			__func__, isp, cid);
		return RET_FAILURE;
	};

	stream_id = isp_get_stream_id_from_cid(isp, cid);
	sif = &isp->sensor_info[cid];

	isp_dbg_print_err("%s, cid:%u,stream:%u\n", __func__, cid, stream_id);
	if (sif->para_brightness_set != BRIGHTNESS_DEF_VALUE)
		fw_if_set_brightness(isp, cid, sif->para_brightness_set);
	if (sif->para_contrast_set != CONTRAST_DEF_VALUE)
		fw_if_set_contrast(isp, cid, sif->para_contrast_set);
	if (sif->para_hue_set != HUE_DEF_VALUE)
		fw_if_set_hue(isp, cid, sif->para_hue_set);
	if (sif->para_satuaration_set != SATURATION_DEF_VALUE)
		fw_if_set_satuation(isp, cid, sif->para_satuaration_set);

	isp_dbg_print_info("<- %s, suc\n", __func__);
	return ret;
}

result_t isp_kickoff_stream(struct isp_context *isp,
				enum camera_id cid, uint32 w, uint32 h)
{
	enum fw_cmd_resp_stream_id stream_id;
	struct sensor_info *snrif;
	CmdSetAcqWindow_t acqw;
	CmdAeSetStartExposure_t sse;
	uint32 again = 0;
	uint32 itime = 0;

	if (!is_para_legal(isp, cid)) {
		isp_dbg_print_err("-><- %s fail for para,cid:%d\n",
			__func__, cid);
		return RET_FAILURE;
	};
	snrif = &isp->sensor_info[cid];

	if (snrif->status == START_STATUS_STARTED) {
		isp_dbg_print_info("-><- %s suc, do none\n", __func__);
		return RET_SUCCESS;
	} else if (snrif->status == START_STATUS_START_FAIL) {
		isp_dbg_print_err
			("-><- %s fail for stream fail status\n", __func__);
		return RET_FAILURE;
	}

	isp_dbg_print_info("-> %s cid:%d,w:%u,h:%u\n", __func__, cid, w, h);
	stream_id = isp_get_stream_id_from_cid(isp, cid);

	acqw.window.h_offset = 0;
	acqw.window.v_offset = 0;
	acqw.window.h_size = isp->sensor_info[cid].raw_width;
	acqw.window.v_size = isp->sensor_info[cid].raw_height;
	isp->sensor_info[cid].status = START_STATUS_START_FAIL;
	if ((acqw.window.h_size == 0)
	|| (acqw.window.v_size == 0)) {
		isp_dbg_print_info("in %s, fail bad acq w:h %u:%u\n",
			__func__, acqw.window.h_size, acqw.window.v_size);
	};
	if (isp_is_mem_camera(isp, cid)) {
		isp_dbg_print_info
			("in %s no set acq for memcam\n", __func__);
	} else if (isp_send_fw_cmd(isp, CMD_ID_SET_ACQ_WINDOW, stream_id,
				FW_CMD_PARA_TYPE_DIRECT,
				&acqw, sizeof(acqw)) != RET_SUCCESS) {
		isp_dbg_print_err
			("<- %s fail for SET_ACQ_WINDOW\n", __func__);
		return RET_FAILURE;
	}

	isp_dbg_print_info("in %s, acq w:h %u:%u\n",
		__func__, acqw.window.h_size, acqw.window.v_size);

	isp_setup_aspect_ratio_wnd(isp, cid, w, h);
	set_scene_mode_imp(isp, cid, ISP_SCENE_MODE_AUTO);

	sse.startExposure = 264 * 1000;
	if (isp_is_mem_camera(isp, cid)) {
		isp_dbg_print_info("%s,no need get current exposure for mem\n",
			__func__);
	} else if (isp_snr_get_current_exposure(isp, cid, &again, &itime)
							!= RET_SUCCESS) {
		isp_dbg_print_err("in %s, fail to get current exposure\n",
			__func__);
	} else
		sse.startExposure = (uint32_t) ((float)itime * again / 1000);

	if (isp_send_fw_cmd(isp, CMD_ID_AE_SET_START_EXPOSURE, stream_id,
				FW_CMD_PARA_TYPE_DIRECT,
				&sse, sizeof(sse)) != RET_SUCCESS) {
		isp_dbg_print_err("in %s, fail SET_START_EXPOSURE to %u\n",
			__func__, sse.startExposure);
	} else {
		isp_dbg_print_info("in %s, SET_START_EXPOSURE to %u suc\n",
			__func__, sse.startExposure);
	};

	if (isp_send_fw_cmd(isp, CMD_ID_START_STREAM, stream_id,
			FW_CMD_PARA_TYPE_DIRECT, NULL, 0) != RET_SUCCESS) {
		isp_dbg_print_err("<- %s fail for START_STREAM\n", __func__);
		return RET_FAILURE;
	}

	isp_dbg_print_info("<- %s suc\n", __func__);
	isp->sensor_info[cid].status = START_STATUS_STARTED;
	return RET_SUCCESS;
}

result_t isp_setup_3a(struct isp_context *isp, enum camera_id cam_id)
{
	result_t ret;
	struct sensor_info *snrif;

	if (!is_para_legal(isp, cam_id)) {
		isp_dbg_print_err
			("-><- %s fail for para,cid:%d\n", __func__, cam_id);
		ret = RET_FAILURE;
		goto quit;
	};
	snrif = &isp->sensor_info[cam_id];
	if (isp_is_mem_camera(isp, cam_id)) {
		isp_dbg_print_info("-> %s suc do ae only for mem cam %u\n",
			__func__, cam_id);
		ret = RET_SUCCESS;
		snrif->status = START_STATUS_START_FAIL;
		if (isp_setup_ae(isp, cam_id) != RET_SUCCESS) {
			isp_dbg_print_err
				("<- %s fail for set up ae\n", __func__);
			ret = RET_FAILURE;
			goto quit;
		};
		isp_dbg_print_info("<- %s suc\n", __func__);
		snrif->status = START_STATUS_STARTING;
		goto quit;
	};

	isp_dbg_print_info("-> %s cid:%d\n", __func__, cam_id);
	if ((snrif->status == START_STATUS_STARTED)
	|| (snrif->status == START_STATUS_STARTING)) {
		ret = RET_SUCCESS;
		isp_dbg_print_info("<- %s suc, do none\n", __func__);
		goto quit;
	} else if (snrif->status == START_STATUS_START_FAIL) {
		isp_dbg_print_err
			("<- %s fail for stream fail status\n", __func__);
		ret = RET_FAILURE;
		goto quit;
	}

	snrif->status = START_STATUS_START_FAIL;
	if (isp_setup_ae(isp, cam_id) != RET_SUCCESS) {
		isp_dbg_print_err("<- %s fail for set up ae\n", __func__);
		ret = RET_FAILURE;
		goto quit;
	};

//	comment out since AF is not supported.
//	if (isp_setup_af(isp, cam_id) != RET_SUCCESS) {
//		isp_dbg_print_err("<- %s fail for set up af\n", __func__);
//		ret = RET_FAILURE;
//		goto quit;
//	};

	if (isp_setup_awb(isp, cam_id) != RET_SUCCESS) {
		isp_dbg_print_err("<- %s fail for set up awb\n", __func__);
		ret = RET_FAILURE;
		goto quit;
	};
	snrif->status = START_STATUS_STARTING;
	ret = RET_SUCCESS;

	isp_dbg_print_info("<- %s suc\n", __func__);
 quit:

	return ret;
};

result_t isp_start_stream(struct isp_context *isp, enum camera_id cam_id,
			enum stream_id str_id, bool is_perframe_ctl)
{
	result_t ret;
	struct isp_stream_info *sif = NULL;
	struct sensor_info *snrif = NULL;

	if (!is_para_legal(isp, cam_id) || (str_id > STREAM_ID_NUM)) {
		isp_dbg_print_err("-><- %s fail, bad para,cid:%d,str:%d\n",
			__func__, cam_id, str_id);
		ret = RET_FAILURE;
		goto quit;
	};
	snrif = &isp->sensor_info[cam_id];
	if ((str_id == STREAM_ID_RGBIR)
	&& !isp_is_host_processing_raw(isp, cam_id)) {
		isp_dbg_print_err
			("-><- %s fail, not rgbir for the stream\n", __func__);
		ret = RET_FAILURE;
		goto quit;
	};
	if (str_id < STREAM_ID_RGBIR) {
		sif = &isp->sensor_info[cam_id].str_info[str_id];
		sif->is_perframe_ctl = is_perframe_ctl;
	}
	isp_dbg_print_info("-> %s cid:%d, str:%d\n",
		__func__, cam_id, str_id);

	if (isp_init_stream(isp, cam_id) != RET_SUCCESS) {
		isp_dbg_print_err
			("<- %s fail for isp_init_stream\n", __func__);
		ret = RET_FAILURE;
		goto quit;
	};

	if (isp_setup_3a(isp, cam_id) != RET_SUCCESS) {
		isp_dbg_print_err
			("<- %s fail for isp_setup_3a\n", __func__);
		ret = RET_FAILURE;
		goto quit;
	};
	if (sif == NULL) {
		ret = RET_SUCCESS;
		isp_dbg_print_info
			("<- %s suc,setupout later for rgbir\n", __func__);
		goto quit;
	} else if ((sif->start_status == START_STATUS_NOT_START)
	|| (sif->start_status == START_STATUS_STARTING)) {
		if (sif->width && sif->height && sif->luma_pitch_set)
			goto do_out_setup;
		else {
			sif->start_status = START_STATUS_STARTING;
			ret = RET_SUCCESS;
			isp_dbg_print_info
				("<- %s suc,setup out later\n", __func__);
			goto quit;
		}
	} else if (sif->start_status == START_STATUS_STARTED) {
		ret = RET_SUCCESS;
		isp_dbg_print_info("<- %s suc,do none\n", __func__);
		goto quit;
	} else if (sif->start_status == START_STATUS_START_FAIL) {
		ret = RET_FAILURE;
		isp_dbg_print_err("<- %s fail,previous fail\n", __func__);
		goto quit;
	} else if (sif->start_status == START_STATUS_START_STOPPING) {
		ret = RET_FAILURE;
		isp_dbg_print_err("<- %s fail,in stopping\n", __func__);
		goto quit;
	} else {
		ret = RET_FAILURE;
		isp_dbg_print_err("<- %s fail,bad stat %d\n",
			__func__, sif->start_status);
		goto quit;
	}
 do_out_setup:
	if (isp_setup_output(isp, cam_id, str_id) != RET_SUCCESS) {
		isp_dbg_print_err("<- %s fail for setup out\n", __func__);
		ret = RET_FAILURE;
	} else {
		ret = RET_SUCCESS;
		isp_dbg_print_info("<- %s suc,setup out suc\n", __func__);
	};
 quit:
	return ret;
}


result_t isp_set_stream_data_fmt(struct isp_context *isp_context,
					enum camera_id cam_id,
					enum stream_id stream_type,
					enum pvt_img_fmt img_fmt)
{
	struct isp_stream_info *sif;

	if (!is_para_legal(isp_context, cam_id)
	|| (stream_type > STREAM_ID_NUM)) {
		isp_dbg_print_err("-><- %s,fail para,isp%p,cid%d,sid%d\n",
			__func__, isp_context, cam_id, stream_type);
		return RET_FAILURE;
	};

	if ((img_fmt == PVT_IMG_FMT_INVALID) || (img_fmt >= PVT_IMG_FMT_MAX)) {
		isp_dbg_print_err("-><- %s,fail fmt,cid%d,sid%d,fmt%d\n",
			__func__, cam_id, stream_type, img_fmt);
		return RET_FAILURE;
	};
	sif = &isp_context->sensor_info[cam_id].str_info[stream_type];

	if (sif->start_status != START_STATUS_NOT_START) {
		if (sif->format == img_fmt) {
			isp_dbg_print_info
				("-><- %s suc,cid%d,str%d,fmt%s,do none\n",
				__func__, cam_id, stream_type,
				isp_dbg_get_pvt_fmt_str(img_fmt));
			return RET_SUCCESS;
		}

		{
			isp_dbg_print_info
				("-><- %s fail,cid%d,str%d,fmt%s,bad stat%d\n",
				__func__, cam_id, stream_type,
				isp_dbg_get_pvt_fmt_str(img_fmt),
				sif->start_status);
			return RET_FAILURE;
		}
	};

	sif->format = img_fmt;

	isp_dbg_print_info("-><- %s suc,cid %d,str %d,fmt %s\n",
		__func__, cam_id, stream_type,
		isp_dbg_get_pvt_fmt_str(img_fmt));
	return RET_SUCCESS;
};


void isp_pause_stream_if_needed(struct isp_context *isp,
	enum camera_id cid, uint32 w, uint32 h)
{
	uint32 running_stream_count = 0;
	CmdStopStream_t stop_stream_para;
	enum fw_cmd_resp_stream_id stream_id;
	uint32 cmd;
	uint32 timeout;
	enum stream_id str_id;
	struct sensor_info *snr_info;
	Window_t aprw;

	if ((isp == NULL) || (cid > CAMERA_ID_FRONT_RIGHT))
		return;
	snr_info = &isp->sensor_info[cid];
	if (snr_info->status != START_STATUS_STARTED)
		return;
	for (str_id = STREAM_ID_PREVIEW; str_id < STREAM_ID_RGBIR; str_id++) {
		if (snr_info->str_info[str_id].start_status
					== START_STATUS_STARTED) {
			running_stream_count++;
		}
	};
	if (running_stream_count > 0)
		return;
	isp_calc_aspect_ratio_wnd(isp, cid, w, h, &aprw);
	if ((snr_info->asprw_wind.h_size == aprw.h_size)
	&& (snr_info->asprw_wind.v_size == aprw.v_size)) {
		return;
	};
	isp_dbg_print_info
		("-><- %s,cid %d,aspr from [%u,%u,%u,%u] to [%u,%u,%u,%u]\n",
		__func__, cid,
		snr_info->asprw_wind.h_offset, snr_info->asprw_wind.v_offset,
		snr_info->asprw_wind.h_size, snr_info->asprw_wind.v_size,
		aprw.h_offset, aprw.v_offset, aprw.h_size, aprw.v_size);

	snr_info->start_str_cmd_sent = 0;
	snr_info->status = START_STATUS_STARTING;
	cmd = CMD_ID_STOP_STREAM;
	stop_stream_para.destroyBuffers = false;
	stream_id = isp_get_stream_id_from_cid(isp, cid);
	timeout = (g_isp_env_setting == ISP_ENV_SILICON) ?
			(1000 * 2) : (1000 * 30);
	isp_send_fw_cmd_sync(isp, cmd, stream_id, FW_CMD_PARA_TYPE_DIRECT,
		&stop_stream_para, sizeof(stop_stream_para), timeout,
						NULL, NULL);
	//isp_setup_stream_tmp_buf(isp, cid);
	//isp_setup_metadata_buf(isp, cid);
}


result_t isp_set_str_res_fps_pitch(struct isp_context *isp_context,
					enum camera_id cam_id,
					enum stream_id stream_type,
					struct pvt_img_res_fps_pitch *value)
{
	uint32 width;
	uint32 height;
	uint32 fps;
	uint32 luma_pitch;
	uint32 chroma_pitch;
	struct isp_stream_info *sif;
	result_t ret;

	if ((isp_context == NULL) || (cam_id >= CAMERA_ID_MAX)
	|| (stream_type > STREAM_ID_NUM) || (value == NULL)) {
		isp_dbg_print_err
			("-><- %s,fail para,isp %p,cid %d,sid %d\n",
			__func__, isp_context, cam_id, stream_type);
		return RET_FAILURE;
	}
	width = value->width;
	height = value->height;
	fps = value->fps;
	luma_pitch = abs(value->luma_pitch);
	chroma_pitch = abs(value->chroma_pitch);

	if ((width == 0) || (height == 0) || (luma_pitch == 0)) {
		isp_dbg_print_err
			("-><- %s,fail para,cid%d,sid%d,w:h:p %d:%d:%d\n",
			__func__, cam_id,
			stream_type, width, height, luma_pitch);
		return RET_FAILURE;
	};

	sif = &isp_context->sensor_info[cam_id].str_info[stream_type];

	isp_dbg_print_info
("%s,cid%d,sid%d,lp%d,cp%d,w:%d,h:%d,fpsId:%d,streamStatus:%u,chanStatus %u\n",
		__func__, cam_id, stream_type,
		luma_pitch, chroma_pitch, width, height, fps,
		isp_context->sensor_info[cam_id].status,
		sif->start_status);

	if ((isp_context->sensor_info[cam_id].status == START_STATUS_STARTED)
	|| (sif->start_status == START_STATUS_STARTING))
		isp_pause_stream_if_needed(isp_context, cam_id, width, height);

	if (sif->start_status == START_STATUS_NOT_START) {
		sif->width = width;
		sif->height = height;
		sif->fps = fps;
		sif->luma_pitch_set = luma_pitch;
		sif->chroma_pitch_set = chroma_pitch;
		ret = RET_SUCCESS;
		isp_dbg_print_info("<- %s suc, store\n", __func__);
	} else if (sif->start_status == START_STATUS_STARTING) {
		sif->width = width;
		sif->height = height;
		sif->fps = fps;
		sif->luma_pitch_set = luma_pitch;
		sif->chroma_pitch_set = chroma_pitch;

		ret = isp_setup_output(isp_context, cam_id, stream_type);
		if (ret == RET_SUCCESS) {
			isp_dbg_print_info
				("<- %s suc aft setup out\n", __func__);
			ret = RET_SUCCESS;
			goto quit;
		}

		isp_dbg_print_err("<- %s fail for setup out\n", __func__);
		ret = RET_FAILURE;
		goto quit;
	} else {
		if ((sif->width != width) || (sif->height != height)
		|| (sif->fps != fps) || (sif->luma_pitch_set != luma_pitch)
		|| (sif->chroma_pitch_set != chroma_pitch)) {
			isp_dbg_print_err
				("<- %s fail for non-consis\n", __func__);
			ret = RET_FAILURE;
			goto quit;
		}

		isp_dbg_print_info("<- %s suc, do none\n", __func__);
		ret = RET_SUCCESS;
		goto quit;
	}
 quit:

	return ret;
};

bool_t is_camera_started(struct isp_context *isp_context,
				enum camera_id cam_id)
{
	struct isp_pwr_unit *pwr_unit;

	if ((isp_context == NULL) || (cam_id >= CAMERA_ID_MAX)) {
		isp_dbg_print_err("-><- %s fail for illegal para %d\n",
			__func__, cam_id);
		return 0;
	};
	pwr_unit = &isp_context->isp_pu_cam[cam_id];
	if (pwr_unit->pwr_status == ISP_PWR_UNIT_STATUS_ON)
		return 1;
	else
		return 0;
}


void isp_take_back_str_buf(struct isp_context *isp,
			struct isp_stream_info *str,
			enum camera_id cid, enum stream_id sid)
{
	struct isp_mapped_buf_info *img_info = NULL;
	struct frame_done_cb_para cb = { 0 };
	struct buf_done_info *done_info;
	enum camera_id cb_cid = cid;

	if ((isp == NULL) || (str == NULL))
		return;
	switch (sid) {
	case STREAM_ID_PREVIEW:
		done_info = &cb.preview;
		break;
	case STREAM_ID_VIDEO:
		done_info = &cb.video;
		break;
	case STREAM_ID_ZSL:
		done_info = &cb.zsl;
		break;
	case STREAM_ID_RGBIR:
		done_info = (struct buf_done_info *)&cb.raw;
		break;
	default:
		done_info = NULL;
	}

	if (cid == CAMERA_ID_MEM)
		cb_cid = isp_get_rgbir_cid(isp);

	do {
		memset(&cb, 0, sizeof(cb));
		cb.cam_id = cid;
		if (img_info) {
#ifdef REPORT_STREAM_BUFF_BACK_TO_UPLAYER
			if (done_info
			&& (cb_cid < CAMERA_ID_MAX)
			&& isp->evt_cb[cb_cid]) {
				done_info->buf = *img_info->sys_img_buf_hdl;
				done_info->status = BUF_DONE_STATUS_FAILED;
				isp->evt_cb[cb_cid](
					isp->evt_cb_context[cb_cid],
					CB_EVT_ID_FRAME_DONE, &cb);
			};
#endif
			isp_unmap_sys_2_mc(isp, img_info);
			isp_sys_mem_free(img_info);
		};
		img_info = (struct isp_mapped_buf_info *)
			isp_list_get_first(&str->buf_in_fw);
	} while (img_info);

	img_info = NULL;
	do {
		memset(&cb, 0, sizeof(cb));
		cb.cam_id = cid;
		if (img_info) {
#ifdef REPORT_STREAM_BUFF_BACK_TO_UPLAYER
			if (done_info
			&& (cb_cid < CAMERA_ID_MAX)
			&& isp->evt_cb[cb_cid]) {
				done_info->buf = *img_info->sys_img_buf_hdl;
				done_info->status = BUF_DONE_STATUS_FAILED;
				isp->evt_cb[cb_cid](
					isp->evt_cb_context[cb_cid],
					CB_EVT_ID_FRAME_DONE, &cb);
			};
#endif
			isp_unmap_sys_2_mc(isp, img_info);
			isp_sys_mem_free(img_info);
		};
		img_info = (struct isp_mapped_buf_info *)
		    isp_list_get_first(&str->buf_free);
	} while (img_info);

};

void isp_take_back_metadata_buf(struct isp_context *isp, enum camera_id cam_id)
{
	sys_img_buf_handle_t orig_buf = NULL;
	struct isp_mapped_buf_info *isp_general_img_buf;

	if (!is_para_legal(isp, cam_id)) {
		isp_dbg_print_err("-><- %s bad para\n", __func__);
		return;
	}
	do {
		isp_general_img_buf = (struct isp_mapped_buf_info *)
			isp_list_get_first(
				&isp->sensor_info[cam_id].meta_data_free);
		if (isp_general_img_buf == NULL)
			break;

		orig_buf = isp_general_img_buf->sys_img_buf_hdl;
		if (orig_buf)
			sys_img_buf_handle_free(orig_buf);
		isp_unmap_sys_2_mc(isp, isp_general_img_buf);
		isp_sys_mem_free(isp_general_img_buf);
	} while (isp_general_img_buf);

	do {
		isp_general_img_buf = (struct isp_mapped_buf_info *)
			isp_list_get_first(
			&isp->sensor_info[cam_id].meta_data_in_fw);
		if (isp_general_img_buf == NULL)
			break;

		orig_buf = isp_general_img_buf->sys_img_buf_hdl;
		if (orig_buf)
			sys_img_buf_handle_free(orig_buf);
		isp_unmap_sys_2_mc(isp, isp_general_img_buf);
		isp_sys_mem_free(isp_general_img_buf);
	} while (isp_general_img_buf);
}

struct isp_mapped_buf_info *isp_map_sys_2_mc(struct isp_context *isp,
			sys_img_buf_handle_t sys_img_buf_hdl, uint32 mc_align,
			uint16 cam_id, uint16 str_id,
			uint32 y_len, uint32 u_len, uint32 v_len)
{
	struct isp_mapped_buf_info *new_buffer;
	sys_img_buf_handle_t hdl = sys_img_buf_hdl;
	uint32 mc_total_len;
	uint64 y_mc;
	uint64 u_mc;
	uint64 v_mc;
	void *vaddr = NULL;

	vaddr = vaddr;
	isp = isp;

	new_buffer = isp_sys_mem_alloc(sizeof(struct isp_mapped_buf_info));
	if (new_buffer == NULL) {
		isp_dbg_print_err("-><- %s fail for alloc\n", __func__);
		return NULL;
	};
	memset(new_buffer, 0, sizeof(struct isp_mapped_buf_info));
	new_buffer->sys_img_buf_hdl = hdl;
	new_buffer->camera_id = (uint8) cam_id;
	new_buffer->str_id = (uint8) str_id;

	mc_total_len = y_len + u_len + v_len;


	y_mc = hdl->physical_addr;
	vaddr = hdl->virtual_addr;

	u_mc = y_mc + y_len;
	v_mc = u_mc + u_len;
	new_buffer->y_map_info.len = y_len;
	new_buffer->y_map_info.mc_addr = y_mc;
	new_buffer->y_map_info.sys_addr = (uint64) vaddr;
	if (u_len) {
		new_buffer->u_map_info.len = u_len;
		new_buffer->u_map_info.mc_addr = u_mc;
		new_buffer->u_map_info.sys_addr = (uint64) vaddr + y_len;
	};
	if (v_len) {
		new_buffer->v_map_info.len = v_len;
		new_buffer->v_map_info.mc_addr = v_mc;
		new_buffer->v_map_info.sys_addr = (uint64) vaddr +
			y_len + u_len;
	};

	goto quit;
 quit:
	return new_buffer;
}

void isp_unmap_sys_2_mc(struct isp_context *isp,
			struct isp_mapped_buf_info *buff)
{
//	uint32_t total_len;

	if (buff == NULL) {
		isp_dbg_print_err("in %s buff is NULL\n", __func__);
		return;
	}

	if (buff->sys_img_buf_hdl == NULL) {
		isp_dbg_print_err
			("in %s sys_img_buf_hdl is NULL\n", __func__);
		return;
	}

	isp = isp;

	goto quit;

 quit:
	isp_dbg_print_info("in %s, quit\n", __func__);
}




sys_img_buf_handle_t sys_img_buf_handle_cpy(sys_img_buf_handle_t hdl_in)
{
	sys_img_buf_handle_t ret;

	if (hdl_in == NULL)
		return NULL;

	ret = isp_sys_mem_alloc(sizeof(struct sys_img_buf_handle));

	if (ret) {
		memcpy(ret, hdl_in, sizeof(struct sys_img_buf_handle));
	} else {
		isp_dbg_print_err("%s fail", __func__);
	};

	return ret;
};

void sys_img_buf_handle_free(sys_img_buf_handle_t hdl)
{
	if (hdl)
		isp_sys_mem_free(hdl);
};

uint32 isp_calc_total_mc_len(uint32 y_len, uint32 u_len, uint32 v_len,
			uint32 mc_align, uint32 *u_offset, uint32 *v_offset)
{
	uint32 end = 0;

	end = y_len;
	end = ISP_ADDR_ALIGN_UP(end, mc_align);
	if (u_offset)
		*u_offset = end;
	end += u_len;
	end = ISP_ADDR_ALIGN_UP(end, mc_align);
	if (v_offset)
		*v_offset = end;
	end += v_len;
	return end;
};

int32 isp_get_res_fps_id_for_str(struct isp_context *isp,
				enum camera_id cam_id, enum stream_id str_id)
{
	uint32 width;
	uint32 height;
	uint32 fps;
	uint32 i;

	if ((isp == NULL) || (cam_id >= CAMERA_ID_MAX)
	|| (str_id > STREAM_ID_NUM)) {
		isp_dbg_print_err("-><- %s fail for para", __func__);
		return -1;
	}
	if (!isp->snr_res_fps_info_got[cam_id]) {
		isp_dbg_print_err
			("-><- %s fail for !snr_res_fps_info_got", __func__);
		return -1;
	}

	width = isp->sensor_info[cam_id].str_info[str_id].width;
	height = isp->sensor_info[cam_id].str_info[str_id].height;
	fps = isp->sensor_info[cam_id].str_info[str_id].fps;

	for (i = 0; i < isp->sensor_info[cam_id].res_fps.count; i++) {
		if (fps == isp->sensor_info[cam_id].res_fps.res_fps[i].fps) {
			if ((isp->sensor_info[cam_id].res_fps.res_fps[i].height
			>= height) &&
			(isp->sensor_info[cam_id].res_fps.res_fps[i].width
			>= width)) {
				isp_dbg_print_info
					("-><- %s suc, ret %d", __func__, i);
				return i;
			}
		}
	}
	isp_dbg_print_err("-><- %s fail no find\n", __func__);
	return -1;
};


result_t isp_snr_meta_data_init(struct isp_context *isp)
{
	enum camera_id cam_id;

	if (isp == NULL) {
		isp_dbg_print_info("-><- %s fail for para\n", __func__);
		return RET_FAILURE;
	}

	for (cam_id = CAMERA_ID_REAR; cam_id < CAMERA_ID_MAX; cam_id++) {
		isp_list_init(&isp->sensor_info[cam_id].meta_data_free);
		isp_list_init(&isp->sensor_info[cam_id].meta_data_in_fw);
	}

	isp_dbg_print_info("<- %s success\n", __func__);
	return RET_SUCCESS;
};

result_t isp_snr_meta_data_uninit(struct isp_context *isp)
{
	if (isp == NULL) {
		isp_dbg_print_info("-><- %s fail for para\n", __func__);
		return RET_FAILURE;
	}

	isp_dbg_print_info("<- %s success\n", __func__);
	return RET_SUCCESS;
};

result_t isp_set_script_to_fw(struct isp_context *isp, enum camera_id cid)
{
	uint32 sensor_len;
	uint32 vcm_len;
	uint32 flash_len;
	uint8 *buf = NULL;
	isp_fw_script_ctrl_t ctrl = {0};
	result_t result;
//	malloc buffer for this, because it should be sent to firmware
//	instr_seq_t bin;
	CmdSetDeviceScript_t *script_cmd;
//	struct isp_mapped_buf_info *script_bin_buf = NULL;

	if (!is_para_legal(isp, cid)) {
		isp_dbg_print_err
			("-><- %sfail for para, id %d\n", __func__, cid);
		return RET_FAILURE;
	}

	if ((cid == CAMERA_ID_MEM)
	|| (isp->sensor_info[cid].cam_type == CAMERA_TYPE_MEM)) {
		isp_dbg_print_info("-><- %s suc,no need for mem camera %u\n",
			__func__, cid);
		return RET_SUCCESS;
	}

	isp_dbg_print_info("-> %s\n", __func__);

	if ((isp->p_sensor_ops == NULL)
/*	|| (isp->p_vcm_ops == NULL)
 *	|| (isp->p_flash_ops == NULL)
 */	|| (isp->fw_parser[cid] == NULL)
	|| (isp->fw_parser[cid]->parse == NULL)) {
		isp_dbg_print_err("<- %s fail fo no ops\n", __func__);
		return RET_FAILURE;
	}

	if ((isp->p_sensor_ops[cid]->get_fw_script_len == NULL)
	|| (isp->p_sensor_ops[cid]->get_fw_script_ctrl == NULL)
/*	|| (isp->p_vcm_ops->get_fw_script_len == NULL)
 *	|| (isp->p_vcm_ops->get_fw_script_ctrl == NULL)
 *	|| (isp->p_flash_ops->get_fw_script_len == NULL)
 *	|| (isp->p_flash_ops->get_fw_script_ctrl == NULL)
 */
	) {
		isp_dbg_print_err
			("<- %s fail for no fw script func\n", __func__);
		return RET_FAILURE;
	}

	script_cmd = &isp->sensor_info[cid].script_cmd;
	script_cmd->sensorId = cid;
	result = isp->p_sensor_ops[cid]->get_fw_script_len(cid, &sensor_len);
	if (result != RET_SUCCESS) {
		isp_dbg_print_err
			("<- %s fail for snr get_fw_script_len\n", __func__);
		goto fail;
	};
	vcm_len = 0;
	flash_len = 0;
	buf = isp_sys_mem_alloc(sensor_len + vcm_len + flash_len);
	if (buf == NULL) {
		isp_dbg_print_err
			("<- %s fail for isp_sys_mem_alloc\n", __func__);
		goto fail;
	};


	ctrl.script_buf = buf;
	ctrl.length = sensor_len;
	result = isp->p_sensor_ops[cid]->get_fw_script_ctrl(cid, &ctrl);
	if (result != RET_SUCCESS) {
		isp_dbg_print_err
			("<- %s fail for snr get_fw_script_ctrl\n", __func__);
		goto fail;
	};

	result =
		isp->fw_parser[cid]->parse(buf,
			sensor_len + vcm_len + flash_len,
			(InstrSeq_t *) &script_cmd->deviceScript.instrSeq);
	if (result != RET_SUCCESS) {
		isp_dbg_print_err("<- %s fail for parse\n", __func__);
		goto fail;
	};

	script_cmd->deviceScript.argSensor = ctrl.argAe;
	script_cmd->deviceScript.argLens = ctrl.argAf;
	script_cmd->deviceScript.argFlash = ctrl.argFl;
	script_cmd->deviceScript.i2cSensor = ctrl.i2cAe;
	script_cmd->deviceScript.i2cLens = ctrl.i2cAf;
	script_cmd->deviceScript.i2cFlash = ctrl.i2cFl;
	switch (cid) {
	case CAMERA_ID_REAR:	/*SENSOR_ID_A */
		script_cmd->deviceScript.i2cSensor.deviceId = I2C_DEVICE_ID_A;
		script_cmd->deviceScript.i2cLens.deviceId = I2C_DEVICE_ID_A;
		script_cmd->deviceScript.i2cFlash.deviceId = I2C_DEVICE_ID_A;
		break;
	case CAMERA_ID_FRONT_LEFT:	/*SENSOR_ID_B */
		script_cmd->deviceScript.i2cSensor.deviceId = I2C_DEVICE_ID_B;
		script_cmd->deviceScript.i2cLens.deviceId = I2C_DEVICE_ID_B;
		script_cmd->deviceScript.i2cFlash.deviceId = I2C_DEVICE_ID_B;
		break;
	default:
		isp_dbg_print_err("<- %s fail for cid %d\n",
			__func__, cid);
		goto fail;
	}
	result = isp_fw_set_script(isp, script_cmd);
	if (result != RET_SUCCESS) {
		isp_dbg_print_err
			("<- %s fail for isp_fw_set_script\n", __func__);
		goto fail;
	};

	isp_dbg_print_info("<- %s suc\n", __func__);
	if (buf)
		isp_sys_mem_free(buf);
	return RET_SUCCESS;

 fail:

	if (buf)
		isp_sys_mem_free(buf);
	return RET_FAILURE;
}

uint8 g_fw_log_buf[ISP_FW_TRACE_BUF_SIZE + 32];
#define MAX_ONE_TIME_LOG_INFO_LEN 510
void isp_fw_log_print(struct isp_context *isp)
{
	uint32 cnt;

	isp_mutex_lock(&isp->command_mutex);
	cnt = isp_fw_get_fw_rb_log(isp, g_fw_log_buf);
	isp_mutex_unlock(&isp->command_mutex);

	if (cnt) {
		uint32 len = 0;
		char temp_ch;
		char *str;
		char *end;

		g_fw_log_buf[cnt] = 0;
		len = strlen((char *)g_fw_log_buf);
		str = (char *)g_fw_log_buf;
		end = ((char *)g_fw_log_buf + cnt);
		while (str < end) {
			if ((str + MAX_ONE_TIME_LOG_INFO_LEN) >= end) {
				if (isp->drv_settings.fw_log_cfg_en != 0) {
					char *tmpend = (end - 1);

					while ('\n' == *tmpend) {
						*tmpend = ' ';
						tmpend--;
					}

					isp_dbg_print_info_fw("%s", str);
				}
				break;
			}

			{
				uint32 tmp_len = MAX_ONE_TIME_LOG_INFO_LEN - 4;

				while (tmp_len > 0) {
					if ((str[tmp_len] == '[')
						&& (str[tmp_len + 1] == 'F')
						&& (str[tmp_len + 2] == 'W')
						&& (str[tmp_len + 3] == ':'))
						break;

					tmp_len--;
				};

				if (tmp_len == 0)
					tmp_len = MAX_ONE_TIME_LOG_INFO_LEN;
				temp_ch = str[tmp_len];
				str[tmp_len] = 0;
				if (isp->drv_settings.fw_log_cfg_en != 0) {
					uint32 len1 = tmp_len - 1;

					while ('\n' == str[len1]) {
						str[len1] = ' ';
						len1--;
					}

					isp_dbg_print_info_fw("%s", str);
				}
				str[tmp_len] = temp_ch;
				str = &str[tmp_len];
			}
		}
	}
};

uint32 isp_get_len_from_zoom(uint32 max_len, uint32 min_len,
			uint32 max_zoom, uint32 min_zoom, uint32 cur_zoom)
{
	uint32 len;

	if (min_zoom >= max_zoom)
		return 0;
	if (min_len >= max_len)
		return 0;
	if (cur_zoom < min_zoom)
		return 0;
	if (cur_zoom > max_zoom)
		return 0;

	len = (max_len - min_len) * (max_zoom - cur_zoom) /
			(max_zoom - min_zoom) + min_len;

	if ((len % 2) != 0)
		len--;

	return len;
};

result_t isp_af_set_mode(struct isp_context *isp, enum camera_id cam_id,
			 AfMode_t *af_mode)
{
	isp = isp;
	cam_id = cam_id;
	af_mode = af_mode;

	return IMF_RET_FAIL;
	/*todo  to be added later */
}

int32 work_item_thread_wrapper(pvoid context)
{
	struct isp_context *isp = context;

	while (true) {
		isp_event_wait(&isp->work_item_thread.wakeup_evt,
			WORK_ITEM_INTERVAL);
		work_item_function(isp, isp->work_item_thread.stop_flag);
		if (thread_should_stop(&isp->work_item_thread)) {
			stop_work_thread(&isp->work_item_thread);
			break;
		}
	}

	return 0;
}

void isp_fw_resp_cmd_done_extra(struct isp_context *isp,
				enum camera_id cid, RespCmdDone_t *para,
				struct isp_cmd_element *ele)
{
	uint32 major;
	uint32 minor;
	uint32 rev;
	uint32 *ver;
	uint8 *payload;
	AeIsoCapability_t *iso_cap;
	AeEvCapability_t *ev_cap;
	struct sensor_info *snr_info;

	if ((isp == NULL) || (para == NULL) || (ele == NULL)) {
		isp_dbg_print_err
			("-><- %s null pointer\n", __func__);
		return;
	}

	payload = para->payload;
	if (payload == NULL) {
		isp_dbg_print_err("%s RespCmdDone_t payload null pointer\n",
			__func__);
		return;
	}

	switch (para->cmdId) {
	case CMD_ID_GET_FW_VERSION:
		ver = (uint32 *) payload;
		major = (*ver) >> 24;
		minor = ((*ver) >> 16) & 0xff;
		rev = (*ver) & 0xffff;
		isp->isp_fw_ver = *ver;
		isp_dbg_print_info("fw version,maj:min:rev %u:%u:%u\n",
			major, minor, rev);
		break;
	case CMD_ID_START_STREAM:
		isp_dbg_print_info("fw start stream rsp to host\n");
		if (!isp_is_mem_camera(isp, cid)
		&& isp_snr_stream_on(isp, cid) != RET_SUCCESS) {
			isp_dbg_print_err
				("%s fail for isp_snr_stream_on, cid %u\n",
				__func__, cid);
		};

		if (isp->drv_settings.ir_illuminator_ctrl == 1)
			isp_turn_on_ir_illuminator(isp, cid);

		break;
	case CMD_ID_AE_GET_ISO_CAPABILITY:
		iso_cap = (AeIsoCapability_t *) payload;
		if (is_para_legal(isp, cid))
			isp->isp_cap.iso[cid] = *iso_cap;
		isp_dbg_print_info("iso capability cid %d,min %u, max %u\n",
			cid, iso_cap->minIso.iso, iso_cap->maxIso.iso);
		break;
	case CMD_ID_AE_GET_EV_CAPABILITY:
		ev_cap = (AeEvCapability_t *) payload;
		if (is_para_legal(isp, cid))
			isp->isp_cap.ev_compensation[cid] = *ev_cap;
		isp_dbg_print_info
		("ev capability cid%d,min %u/%u,max %u/%u,step %u/%u\n", cid,
			ev_cap->minEv.numerator, ev_cap->minEv.denominator,
			ev_cap->maxEv.numerator, ev_cap->maxEv.denominator,
			ev_cap->stepEv.numerator, ev_cap->stepEv.denominator);
		break;
	case CMD_ID_SEND_I2C_MSG:
		snr_info = &isp->sensor_info[ele->cam_id];
		if (snr_info == NULL) {
			isp_dbg_print_err
				("-><- %s snr_info null pointer\n", __func__);
			break;
		}
		if (snr_info->iic_tmp_buf == NULL
		|| snr_info->iic_tmp_buf->sys_addr == NULL) {
			isp_dbg_print_err("-><- %s iic_tmp_buf null pointer\n",
				__func__);
			break;
		}

		if (ele->I2CRegAddr == I2C_REGADDR_NULL) {
			isp_dbg_print_err
				("-><- %s invalid regaddr\n", __func__);
			break;
		}

		if (ele->I2CRegAddr == snr_info->fps_tab.HighAddr
/*		|| ele->I2CRegAddr == snr_info->fps_tab.LowAddr */
		) {
			uint8 temp[2] = { 0 };

			memcpy(&temp, snr_info->iic_tmp_buf->sys_addr, 2);
			isp_dbg_print_info("-><- %s i2c read: %x, %x\n",
				__func__, temp[0], temp[1]);
			snr_info->fps_tab.HighValue = temp[0];
			snr_info->fps_tab.LowValue = temp[1];
/*
 *			if(ele->I2CRegAddr == snr_info->fps_tab.HighAddr)
 *				snr_info->fps_tab.HighValue = temp;
 *			else if(ele->I2CRegAddr == snr_info->fps_tab.LowAddr)
 *				snr_info->fps_tab.LowValue = temp;
 */
			if (snr_info->fps_tab.HighValue
			&& snr_info->fps_tab.LowValue) {
				isp_snr_get_runtime_fps(isp, ele->cam_id,
					snr_info->fps_tab.HighValue,
					snr_info->fps_tab.LowValue,
					&snr_info->fps_tab.fps);
//				isp_dbg_print_info("-><- %s fps : %d\n",
//					__func__,
//					(uint32) snr_info->fps_tab.fps);

				snr_info->fps_tab.HighValue = 0;
				snr_info->fps_tab.LowValue = 0;
			}
		} else {
			isp_dbg_print_err
				("%s i2c_msg invalid response\n", __func__);
		}
		break;
	default:
		break;
	};
}

void isp_fw_resp_cmd_done(struct isp_context *isp,
			  enum fw_cmd_resp_stream_id stream_id,
			  RespCmdDone_t *para)
{
	struct isp_cmd_element *ele;
	enum camera_id cid;

	cid = isp_get_cid_from_stream_id(isp, stream_id);
	ele = isp_rm_cmd_from_cmdq(isp, para->cmdSeqNum, para->cmdId, false);

	if (ele == NULL) {
		isp_dbg_print_info
			("%s,stream %d,cmd %s(0x%08x)(%d),seq %u,no orig\n",
			__func__, stream_id,
			isp_dbg_get_cmd_str(para->cmdId), para->cmdId,
			para->cmdStatus, para->cmdSeqNum);
	} else {
		struct cmd_done_cb_para cb = { 0 };

		if (ele->resp_payload && ele->resp_payload_len) {
			*ele->resp_payload_len =
				min((uint32)*ele->resp_payload_len,
					(uint32)36);
			memcpy(ele->resp_payload, para->payload,
				*ele->resp_payload_len);
		};

		isp_dbg_print_info
			("%s,stream %d,cmd %s(0x%08x)(%d),seq %u, cid %u\n",
			__func__, stream_id,
			isp_dbg_get_cmd_str(para->cmdId), para->cmdId,
			para->cmdStatus, para->cmdSeqNum, cid);
		if (para->cmdStatus == 0)
			isp_fw_resp_cmd_done_extra(isp, cid, para, ele);
		if (ele->evt) {
			isp_dbg_print_info("in %s, signal event %p\n",
				__func__, ele->evt);
			isp_event_signal(para->cmdStatus, ele->evt);
		}
		cb.status = para->cmdStatus;
		if (cid >= CAMERA_ID_MAX) {
			if (stream_id != FW_CMD_RESP_STREAM_ID_GLOBAL)
				isp_dbg_print_err("%s,fail cid %d, sid %u\n",
					__func__, cid, stream_id);
		} else if ((para->cmdId == CMD_ID_SEND_MODE3_FRAME_INFO)
		|| (para->cmdId == CMD_ID_SEND_MODE4_FRAME_INFO)) {
			struct isp_mapped_buf_info *raw = NULL;
			enum camera_id cb_cid = cid;

			cb.cam_id = cid;
			if (para->cmdId == CMD_ID_SEND_MODE4_FRAME_INFO)
				cb_cid = isp_get_rgbir_cid(isp);
			if (para->cmdId == CMD_ID_SEND_MODE3_FRAME_INFO) {
				cb.cmd = ISP_CMD_ID_SET_RGBIR_INPUT_BUF;
				raw = (struct isp_mapped_buf_info *)
					isp_list_get_first(
				&isp->sensor_info[cid].rgbraw_input_in_fw);
			} else {
				cb.cmd = ISP_CMD_ID_SET_MEM_CAMERA_INPUT_BUF;
				raw = (struct isp_mapped_buf_info *)
					isp_list_get_first(
				&isp->sensor_info[cid].irraw_input_in_fw);
			}

			if (raw) {
				cb.para.buf = *raw->sys_img_buf_hdl;
				if ((cb_cid < CAMERA_ID_MAX)
				&& isp->evt_cb[cb_cid])
					isp->evt_cb[cb_cid]
						(isp->evt_cb_context[cb_cid],
						CB_EVT_ID_CMD_DONE, &cb);
				else
					isp_dbg_print_err
						("%s,fail empty cb cid%u\n",
						__func__, cb_cid);
				isp_unmap_sys_2_mc(isp, raw);
				if (raw->sys_img_buf_hdl)
					isp_sys_mem_free(raw->sys_img_buf_hdl);
				isp_sys_mem_free(raw);
			} else if (cb_cid < CAMERA_ID_MAX) {
				//rgbir case not loopback
				isp_dbg_print_info
					("in %s,fail no raw,cid %u\n",
					__func__, cid);
			};
		}

		if (ele->mc_addr)
			isp_fw_ret_pl(isp->fw_work_buf_hdl, ele->mc_addr);

		if (ele->gpu_pkg)
			isp_gpu_mem_free(ele->gpu_pkg);

		isp_sys_mem_free(ele);
	}
};

void isp_post_proc_str_buf(struct isp_context *isp,
				struct isp_mapped_buf_info *buf)
{
	sys_img_buf_handle_t orig;

	if ((isp == NULL) || (buf == NULL)) {
		isp_dbg_print_err("in %s,fail,bad para\n", __func__);
		return;
	};

	orig = buf->sys_img_buf_hdl;
	if (orig == NULL) {
		isp_dbg_print_err("in %s,fail,null orig\n", __func__);
		return;
	}

	return;
};

enum raw_image_type get_raw_img_type(ImageFormat_t type)
{
	switch (type) {
	case IMAGE_FORMAT_RGBBAYER8:
		return RAW_IMAGE_TYPE_8BIT;
	case IMAGE_FORMAT_RGBBAYER10:
		return RAW_IMAGE_TYPE_10BIT;
	case IMAGE_FORMAT_RGBBAYER12:
		return RAW_IMAGE_TYPE_12BIT;
	default:
		isp_dbg_print_err("-><- %s, fail type %d\n",
			__func__, type);
		return RAW_IMAGE_TYPE_IVALID;
	}
}

/*outpgm should be >= len + 64*/
/*rawhead should be*/
void isp_tras_raw_to_pgm(char *raw, uint32 len, char *outpgm,
			uint32 *outlen, enum raw_image_type raw_type,
			uint32 width, uint32 height)
{
	char *rawhead;
	uint32 i;
	char *pBufOut;
	char *pTmp;
	char *pTmpOut;
	uint8 in0;
	uint8 in1;
	uint32 size = len;
	ImageFormat_t raw_format = raw_type;

	if ((raw == NULL) || (len == 0) || (outpgm == NULL) || (outlen == NULL)
	|| (width == 0) || (height == 0) || ((len + 64) > *outlen)) {
		isp_dbg_print_err("-><- %s fail bad para\n", __func__);
		if (outlen)
			*outlen = 0;
		return;
	};
	rawhead = (char *)outpgm;

	if ((int)raw_format == (int)RAW_IMAGE_TYPE_12BIT)
		snprintf(rawhead, 64, "P5 %d %d 4095\n", width, height);
	else if ((int)raw_format == (int)RAW_IMAGE_TYPE_10BIT)
		snprintf(rawhead, 64, "P5 %d %d 1023\n", width, height);
	else if ((int)raw_format == (int)RAW_IMAGE_TYPE_8BIT)
		snprintf(rawhead, 64, "P5 %d %d 255\n", width, height);
	else
		isp_dbg_print_err("bad raw_type %u\n", raw_format);

	pBufOut = (char *)outpgm + strlen(rawhead);
	pTmp = raw;
	pTmpOut = pBufOut;
	if (outlen) {
		*outlen = (uint32) strlen(rawhead) + len;
		isp_dbg_print_info("rawhead %s(%u), len to %u\n", rawhead,
			(uint32) (strlen(rawhead)), *outlen);
	};

	if ((int)raw_format == (int)RAW_IMAGE_TYPE_12BIT) {
		for (i = 0; i < size; i = i + 2) {
			in0 = *pTmp++;	//a0[11:4]
			in1 = *pTmp++;	//a0[3:0]<<4

			//12bit
			*pTmpOut++ = (in0 >> 4) & 0x0f;	//11~8bit
			*pTmpOut++ = ((in0 << 4) & 0xf0) | ((in1 >> 4) & 0x0f);
		}
	} else if ((int)raw_format == (int)RAW_IMAGE_TYPE_10BIT) {
		for (i = 0; i < size; i = i + 2) {
			in0 = *pTmp++;	//a0[11:4]
			in1 = *pTmp++;	//a0[3:0]<<4

			//12bit
			*pTmpOut++ = (in0 >> 6) & 0x03;	//11~8bit
			*pTmpOut++ = ((in0 << 2) & 0xfc) | ((in1 >> 6) & 0x03);
		}
	} else if ((int)raw_format == (int)RAW_IMAGE_TYPE_8BIT) {
		memcpy(pBufOut, raw, size);
	};
};

//uint32 dbg_raw_count;

void isp_fw_resp_frame_done(struct isp_context *isp,
			enum fw_cmd_resp_stream_id stream_id,
			RespParamPackage_t *para)
{
	MetaInfo_t *meta;
	uint64 mc;
	enum camera_id cid;
	struct isp_mapped_buf_info *prev = NULL;
	struct isp_mapped_buf_info *video = NULL;
	struct isp_mapped_buf_info *zsl = NULL;
	struct isp_mapped_buf_info *raw = NULL;
	struct frame_done_cb_para cb = { 0 };
	struct sensor_info *sif;
	bool_t take_one_pic_done = 0;
	bool_t take_one_raw_done = 0;
	char *pt_cmd = "";

	cid = isp_get_cid_from_stream_id(isp, stream_id);
	if (cid == CAMERA_ID_MAX) {
		isp_dbg_print_err
			("-><- %s,fail,bad cid,streamid %d\n",
			__func__, stream_id);
		return;
	};
	sif = &isp->sensor_info[cid];
	mc = isp_join_addr64(para->packageAddrLo, para->packageAddrHi);

	meta = (MetaInfo_t *)
		isp_fw_buf_get_sys_from_mc(isp->fw_work_buf_hdl, mc);

	if ((mc == 0) || (meta == NULL)) {
		isp_dbg_print_err
			("-><- %s,fail,bad mc,streamid %d,mc %p\n",
			__func__, stream_id, meta);
		return;
	};

	cb.poc = meta->poc;
	cb.cam_id = cid;

	memcpy(&cb.metaInfo, meta, sizeof(MetaInfo_t));
	cb.scene_mode = sif->scene_mode;
	if (cb.metaInfo.zoomWindow.h_size * cb.metaInfo.zoomWindow.v_size == 0)
		cb.zoom_factor = 100;
	else
		cb.zoom_factor =
			sif->asprw_wind.h_size * sif->asprw_wind.v_size * 100 /
			(cb.metaInfo.zoomWindow.h_size *
			cb.metaInfo.zoomWindow.v_size);
	cb.timeStamp =
		((uint64) meta->timeStampLo) |
		(((uint64) meta->timeStampHi) << 32);

	isp_dbg_print_info
		("%s,cid:%d,sid:%d,poc:%u,again:%u,itime:%u us,", __func__,
		cid, stream_id, meta->poc, meta->ae.aGain, meta->ae.itime);

	isp_dbg_print_info("iso:%u,ev:%u/%u,colorT:%u,flicker:%u\n",
		meta->ae.iso.iso,
		meta->ae.ev.numerator, meta->ae.ev.denominator,
		meta->awb.colorTemperature, meta->ae.flickerType);

	cb.preview.status = BUF_DONE_STATUS_ABSENT;
	if (meta->preview.enabled
	&& ((meta->preview.statusEx.status == BUFFER_STATUS_SKIPPED)
	|| (meta->preview.statusEx.status == BUFFER_STATUS_DONE)
	|| (meta->preview.statusEx.status == BUFFER_STATUS_DIRTY))) {
//		dbg_raw_count++;
		prev = (struct isp_mapped_buf_info *) isp_list_get_first
		(&isp->sensor_info[cid].str_info[STREAM_ID_PREVIEW].buf_in_fw);

		if (prev == NULL) {
			isp_dbg_print_err
				("%s,fail null prev\n", __func__);
		} else if (prev->sys_img_buf_hdl == NULL) {
			isp_dbg_print_err
				("%s,fail null prev orig\n", __func__);
		} else {
			cb.preview.buf = *prev->sys_img_buf_hdl;

			if ((meta->preview.statusEx.status ==
			BUFFER_STATUS_SKIPPED)
			|| (meta->preview.statusEx.status ==
			BUFFER_STATUS_DIRTY)) {
				cb.preview.status = BUF_DONE_STATUS_FAILED;
			} else {
				cb.preview.status = BUF_DONE_STATUS_SUCCESS;

				isp_post_proc_str_buf(isp, prev);
#ifdef DUMP_IMG_TO_FILE
				if (dbg_raw_count == 5) {
					ulong len;
					struct file *fp;

					len = prev->sys_img_buf_hdl->len;

					fp = filp_open("/data/temp5.yuv",
					O_RDWR | O_APPEND | O_CREAT, 0666);
					if (IS_ERR(fp)) {
						isp_dbg_print_err
							("file open failed\n");
					} else {
						isp_write_file_test(fp,
						prev->sys_img_buf_hdl->virtual_addr,
						&len);
					}
				}
#endif
			}
		}
	} else if (meta->preview.enabled) {
		isp_dbg_print_err("%s,fail bad preview status %u(%s)\n",
			__func__, meta->preview.statusEx.status,
		isp_dbg_get_buf_done_str(meta->preview.statusEx.status));
	};

	cb.video.status = BUF_DONE_STATUS_ABSENT;
	if (meta->video.enabled
	&& ((meta->video.statusEx.status == BUFFER_STATUS_SKIPPED)
	|| (meta->video.statusEx.status == BUFFER_STATUS_DONE)
	|| (meta->video.statusEx.status == BUFFER_STATUS_DIRTY))) {
		video = (struct isp_mapped_buf_info *)
			isp_list_get_first
		(&isp->sensor_info[cid].str_info[STREAM_ID_VIDEO].buf_in_fw);

		if (video == NULL) {
			isp_dbg_print_err
				("%s,fail null video\n", __func__);
		} else if (video->sys_img_buf_hdl == NULL) {
			isp_dbg_print_err
				("%s,fail null video orig\n", __func__);
		} else {
			cb.video.buf = *video->sys_img_buf_hdl;
			if ((meta->video.statusEx.status ==
			BUFFER_STATUS_SKIPPED)
			|| (meta->video.statusEx.status == BUFFER_STATUS_DIRTY)
			) {
				cb.video.status = BUF_DONE_STATUS_FAILED;
			} else {
				cb.video.status = BUF_DONE_STATUS_SUCCESS;
				isp_post_proc_str_buf(isp, video);
			}
		}
	} else if (meta->video.enabled) {
		isp_dbg_print_err("%s,fail bad video status %u(%s)\n",
			__func__, meta->video.statusEx.status,
			isp_dbg_get_buf_done_str(meta->video.statusEx.status));
	};

	cb.zsl.status = BUF_DONE_STATUS_ABSENT;
	if ((meta->zsl.statusEx.status == BUFFER_STATUS_SKIPPED)
	|| (meta->zsl.statusEx.status == BUFFER_STATUS_DONE)
	|| (meta->zsl.statusEx.status == BUFFER_STATUS_DIRTY)) {
		char *src = "";
		uint32 cnt = 0;

		if (meta->zsl.source == BUFFER_SOURCE_CMD_CAPTURE) {
			pt_cmd = PT_CMD_SCY;
			src = "take_one_pic";

			zsl = (struct isp_mapped_buf_info *)
				isp_list_get_first
				(&sif->take_one_pic_buf_list);
			cnt = sif->take_one_pic_buf_list.count;
			if (zsl)
				take_one_pic_done = 1;
		} else if (meta->zsl.enabled) {
			pt_cmd = PT_CMD_ZB;
			src = "zsl";
			zsl = (struct isp_mapped_buf_info *)
				isp_list_get_first
				(&sif->str_info[STREAM_ID_ZSL].buf_in_fw);
			cnt = sif->str_info[STREAM_ID_ZSL].buf_in_fw.count;
		} else {
			isp_dbg_print_err("in %s,fail here,src %d\n",
				__func__, meta->zsl.source);
		}

		if (zsl == NULL) {
			isp_dbg_print_err
				("%s,fail null %s\n", __func__, src);
		} else if (zsl->sys_img_buf_hdl == NULL) {
			isp_dbg_print_err
				("%s,fail null %s orig\n", __func__, src);
		} else {
			isp_dbg_print_info
		("[PT][Cam][IM][%s][%s][%s][%p][%llx],node %p,cnt %u\n",
				PT_EVT_RCD, pt_cmd, PT_STATUS_SUCC,
				zsl->sys_img_buf_hdl->virtual_addr,
				(((uint64) meta->zsl.buffer.bufBaseAHi)
				<< 32) |
				((uint64) meta->zsl.buffer.bufBaseALo),
				zsl, cnt);

			cb.zsl.buf = *zsl->sys_img_buf_hdl;
			if ((meta->zsl.statusEx.status
			== BUFFER_STATUS_SKIPPED)
			|| (meta->zsl.statusEx.status
			== BUFFER_STATUS_DIRTY))
				cb.zsl.status = BUF_DONE_STATUS_FAILED;
			else {
				cb.zsl.status = BUF_DONE_STATUS_SUCCESS;
				isp_post_proc_str_buf(isp, zsl);
			}
#ifdef USING_PREVIEW_TO_TRIGGER_ZSL
			cb.preview.buf = cb.zsl.buf;
			cb.preview.status = cb.zsl.status;

			cb.zsl.buf.virtual_addr = 0;
			cb.zsl.buf.len = 0;
			cb.zsl.status = BUF_DONE_STATUS_ABSENT;
#endif
		}
	} else if (meta->zsl.statusEx.status == BUFFER_STATUS_EXIST) {
		isp_dbg_print_err("%s,fail bad zsl status %u(%s)\n",
			__func__, meta->zsl.statusEx.status,
			isp_dbg_get_buf_done_str(meta->zsl.statusEx.status));
	}

	cb.raw.status = BUF_DONE_STATUS_ABSENT;
	if ((meta->raw.source == BUFFER_SOURCE_CMD_CAPTURE)
	&& ((meta->raw.statusEx.status == BUFFER_STATUS_SKIPPED)
	|| (meta->raw.statusEx.status == BUFFER_STATUS_DONE)
	|| (meta->raw.statusEx.status == BUFFER_STATUS_DIRTY))) {
		raw = (struct isp_mapped_buf_info *) isp_list_get_first
			(&isp->sensor_info[cid].take_one_raw_buf_list);

		if (raw == NULL) {
			isp_dbg_print_err
				("%s,fail null raw\n", __func__);
		} else if (raw->sys_img_buf_hdl == NULL) {
			isp_dbg_print_err
				("%s,fail null raw orig\n", __func__);
		} else {
			take_one_raw_done = 1;
			cb.raw.buf = *raw->sys_img_buf_hdl;
			if ((meta->raw.statusEx.status
			== BUFFER_STATUS_SKIPPED)
			|| (meta->raw.statusEx.status
			== BUFFER_STATUS_DIRTY)) {
				cb.raw.status = BUF_DONE_STATUS_FAILED;
				isp_dbg_print_err
					("raw BUF_DONE_STATUS_FAILED\n");
			} else {
				uint32 y_len_back;

				cb.raw.width = sif->asprw_wind.h_size;
				cb.raw.height = sif->asprw_wind.v_size;
				cb.raw.type = get_raw_img_type
					(meta->raw.imageProp.imageFormat);
				cb.raw.raw_img_format = meta->rawPktFmt;
				cb.raw.cfa_pattern =
					sif->sensor_cfg.prop.cfaPattern;
				cb.raw.status = BUF_DONE_STATUS_SUCCESS;

				isp_dbg_print_info("raw(w:h)%u:%u,rawType %u\n",
					cb.raw.width, cb.raw.height,
					cb.raw.type);

				isp_dbg_print_info
				("aemode %d,again %u,itime %u microsecond\n",
					meta->ae.mode, meta->ae.aGain,
					meta->ae.itime);
				y_len_back = raw->y_map_info.len;
				raw->y_map_info.len = min(raw->y_map_info.len,
							cb.raw.width *
							cb.raw.height * 2);
				isp_post_proc_str_buf(isp, raw);
				raw->y_map_info.len = y_len_back;
			}
		}
	} else if (meta->raw.statusEx.status == BUFFER_STATUS_EXIST) {
		isp_dbg_print_err
			("%s,fail bad raw status %u(%s)\n",
			__func__, meta->raw.statusEx.status,
			isp_dbg_get_buf_done_str(meta->raw.statusEx.status));
	};

	if (cb.preview.buf.virtual_addr)
		isp_dbg_show_bufmeta_info("prev", cid,
			&meta->preview, &cb.preview.buf);

	if (cb.video.buf.virtual_addr)
		isp_dbg_show_bufmeta_info("video", cid,
			&meta->video, &cb.video.buf);

	if (cb.zsl.buf.virtual_addr)
		isp_dbg_show_bufmeta_info("zsl", cid, &meta->zsl, &cb.zsl.buf);

	if (cb.raw.buf.virtual_addr)
		isp_dbg_show_bufmeta_info("raw", cid, &meta->raw, &cb.raw.buf);

/*
 *	if (cb.preview.buf.virtual_addr)
 *		isp_dbg_print_info("[PT][Cam][IM][%s][%s][%s][%p]\n",
 *			PT_EVT_SCD, PT_CMD_PB, PT_STATUS_SUCC,
 *			cb.preview.buf.virtual_addr);
 *	if (cb.video.buf.virtual_addr)
 *		isp_dbg_print_info("[PT][Cam][IM][%s][%s][%s][%p]\n",
 *			PT_EVT_SCD, PT_CMD_VB, PT_STATUS_SUCC,
 *			cb.video.buf.virtual_addr);
 *	if (cb.zsl.buf.virtual_addr)
 *		isp_dbg_print_info("[PT][Cam][IM][%s][%s][%s][%p]\n",
 *			PT_EVT_SCD, pt_cmd, PT_STATUS_SUCC,
 *			cb.zsl.buf.virtual_addr);
 *	if (cb.raw.buf.virtual_addr)
 *		isp_dbg_print_info("[PT][Cam][IM][%s][%s][%s][%p]\n",
 *			PT_EVT_SCD, PT_CMD_SCR, PT_STATUS_SUCC,
 *			cb.raw.buf.virtual_addr);
 */
	if (isp->drv_settings.dump_pin_output_to_file) {
		dump_pin_outputs_to_file(isp, cid, &cb, meta,
			isp->drv_settings.dump_pin_output_to_file);
	}

	if ((cid == CAMERA_ID_MEM)
	&& (cb.video.buf.virtual_addr == NULL)) {
		isp_dbg_print_info
		("%s,not call cb for empty video buffer for CAMERA_ID_MEM\n",
			__func__);
	} else if (isp->evt_cb[cid]) {
#ifndef USING_KGD_CGS
		if ((cb.preview.buf.virtual_addr)
		|| (cb.video.buf.virtual_addr)
		|| (cb.zsl.buf.virtual_addr)
		|| (cb.raw.buf.virtual_addr)) {

			isp->evt_cb[cid] (isp->evt_cb_context[cid],
				CB_EVT_ID_FRAME_DONE, &cb);
		}
#else
		if (cb.preview.buf.virtual_addr) {
			isp->evt_cb[cid](isp->evt_cb_context[cid],
				CB_EVT_ID_METADATA_DONE, &cb);
			isp->evt_cb[cid](isp->evt_cb_context[cid],
				CB_EVT_ID_PREV_BUF_DONE, &cb);

			isp_dbg_print_err("in %s, preview", __func__);
		}

		if (cb.zsl.buf.virtual_addr) {
			//isp->evt_cb[cid](isp->evt_cb_context[cid],
				//CB_EVT_ID_METADATA_DONE, &cb);
			isp->evt_cb[cid](isp->evt_cb_context[cid],
				CB_EVT_ID_ZSL_BUF_DONE, &cb);
			isp_dbg_print_err("in %s, zsl image", __func__);
		}

		if (cb.video.buf.virtual_addr) {
//			isp->evt_cb[cid](isp->evt_cb_context[cid],
//				CB_EVT_ID_METADATA_DONE, &cb);
			isp->evt_cb[cid](isp->evt_cb_context[cid],
				CB_EVT_ID_REC_BUF_DONE, &cb);
			isp_dbg_print_err("in %s, video", __func__);
		}
#endif
	} else if (cid == CAMERA_ID_MEM) {
		enum camera_id cb_cid = isp_get_rgbir_cid(isp);

		if (isp->evt_cb[cb_cid])
			isp->evt_cb[cb_cid] (isp->evt_cb_context[cb_cid],
				CB_EVT_ID_FRAME_DONE, &cb);
		else
			isp_dbg_print_err
				("in %s,fail empty cb for rgbircid %u\n",
				__func__, cb_cid);
	} else {
		isp_dbg_print_err
			("in %s,fail empty cb for cid %u\n",
			__func__, cid);
	}

	if (prev) {
		isp_unmap_sys_2_mc(isp, prev);
		if (prev->sys_img_buf_hdl)
			isp_sys_mem_free(prev->sys_img_buf_hdl);
		isp_sys_mem_free(prev);
	};
	if (video) {
		isp_unmap_sys_2_mc(isp, video);
		if (video->sys_img_buf_hdl)
			isp_sys_mem_free(video->sys_img_buf_hdl);
		isp_sys_mem_free(video);
	};
	if (zsl) {
		isp_unmap_sys_2_mc(isp, zsl);
		if (zsl->sys_img_buf_hdl)
			isp_sys_mem_free(zsl->sys_img_buf_hdl);
		isp_sys_mem_free(zsl);
	};
	if (raw) {
		isp_unmap_sys_2_mc(isp, raw);

		if (raw->sys_img_buf_hdl)
			isp_sys_mem_free(raw->sys_img_buf_hdl);
		isp_sys_mem_free(raw);
	};

	if ((isp->sensor_info[cid].status == START_STATUS_STARTED)
	|| (isp->sensor_info[cid].status == START_STATUS_STARTING)) {
		CmdSendBuffer_t buf_type;

		memset(&buf_type, 0, sizeof(buf_type));
		buf_type.bufferType = BUFFER_TYPE_META_INFO;
		buf_type.buffer.bufTags = 0;
		buf_type.buffer.vmidSpace = 4;
		isp_split_addr64(mc, &buf_type.buffer.bufBaseALo,
			&buf_type.buffer.bufBaseAHi);
		buf_type.buffer.bufSizeA = isp_get_cmd_pl_size();
		if (isp_send_fw_cmd(isp, CMD_ID_SEND_BUFFER, stream_id,
				FW_CMD_PARA_TYPE_DIRECT, &buf_type,
				sizeof(buf_type)) != RET_SUCCESS) {
			isp_dbg_print_err("<- %s(%u) fail, send meta\n",
				__func__, cid);
			isp_fw_ret_pl(isp->fw_work_buf_hdl, mc);
		} else {
			isp_dbg_print_info
				("<- %s, resend meta\n", __func__);
		};
	} else {
		isp_fw_ret_pl(isp->fw_work_buf_hdl, mc);
		isp_dbg_print_info("<- %s,no send meta in status %d\n",
			__func__, isp->sensor_info[cid].status);
	};

	if (take_one_pic_done)
		sif->take_one_pic_left_cnt--;
	if (take_one_raw_done)
		sif->take_one_raw_left_cnt--;
};

void isp_fw_resp_frame_info(struct isp_context *isp,
			enum fw_cmd_resp_stream_id stream_id,
			RespParamPackage_t *para)
{
	uint64 mc;
	enum camera_id cid;
	struct isp_mapped_buf_info *rgbir_output = NULL;
	struct frame_info_cb_para cb = { 0 };
	struct sensor_info *sif;

	cid = isp_get_cid_from_stream_id(isp, stream_id);
	if (cid == CAMERA_ID_MAX) {
		isp_dbg_print_err("-><- %s,fail,bad cid from streamid %d\n",
			__func__, stream_id);
		return;
	};

	sif = &isp->sensor_info[cid];
	if (!isp_is_host_processing_raw(isp, cid)) {
		isp_dbg_print_err
			("-><- %s,fail cid type %u(%u) from streamid %d\n",
			__func__, cid, sif->cam_type, stream_id);
		return;
	};
	mc = isp_join_addr64(para->packageAddrLo, para->packageAddrHi);

	rgbir_output = (struct isp_mapped_buf_info *) isp_list_get_first
		(&isp->sensor_info[cid].rgbir_raw_output_in_fw);

	if (rgbir_output == NULL) {
		isp_dbg_print_err("-><- %s,cid %u,streamid %d fail,no orig\n",
			__func__, cid, stream_id);
		return;
	} else if (rgbir_output->sys_img_buf_hdl == NULL) {
		isp_dbg_print_err("-><- %s,cid %u,streamid %d fail NUL orig\n",
			__func__, cid, stream_id);
		return;
	} else if ((rgbir_output->sys_img_buf_hdl->virtual_addr == NULL)
	|| (rgbir_output->sys_img_buf_hdl->len == 0)) {
		isp_dbg_print_err("-><- %s,cid %u,streamid %d fail bad orig\n",
			__func__, cid, stream_id);
		return;
	}

	if (rgbir_output->v_map_info.mc_addr != mc) {
		isp_dbg_print_err("-><- %s,cid %u,streamid %d fail mc\n",
			__func__, cid, stream_id);
	};
	isp_dbg_print_info
	("%s,cid:%d,poc %u,sid:%d,raw cnt %u,orig %p(%u),mc:%llx,%u:%u:%u\n",
		__func__, cid, rgbir_output->v_map_info.sys_addr ?
		((Mode3FrameInfo_t *)
		(uint64)rgbir_output->v_map_info.sys_addr)->poc : 0,
		stream_id, --g_rgbir_raw_count_in_fw,
		rgbir_output->sys_img_buf_hdl->virtual_addr,
		rgbir_output->sys_img_buf_hdl->len,
		rgbir_output->y_map_info.mc_addr,
		rgbir_output->y_map_info.len,
		rgbir_output->u_map_info.len, rgbir_output->v_map_info.len);

	if (isp_is_fpga()) {
		isp_hw_mem_read_(rgbir_output->y_map_info.mc_addr,
			rgbir_output->sys_img_buf_hdl->virtual_addr,
			rgbir_output->sys_img_buf_hdl->len);
		isp_dbg_print_info("in %s,aft isp_hw_mem_read %u\n",
			__func__, rgbir_output->sys_img_buf_hdl->len);
	}

	{
		uint8 *pFI = (uint8 *)
			rgbir_output->sys_img_buf_hdl->virtual_addr +
			rgbir_output->y_map_info.len +
			rgbir_output->u_map_info.len;
		if (rgbir_output->v_map_info.len) {
			pFI = pFI;
			//isp_dbg_show_frame3_info(pFI, "out");
		}
	}

	cb.cam_id = cid;
	cb.buf = *rgbir_output->sys_img_buf_hdl;
	cb.bayer_raw_buf_len = rgbir_output->y_map_info.len;
	cb.ir_raw_buf_len = rgbir_output->u_map_info.len;
	cb.frame_info_buf_len = rgbir_output->v_map_info.len;
	isp->evt_cb[cid] (isp->evt_cb_context[cid], CB_EVT_ID_FRAME_INFO, &cb);

	if (rgbir_output) {
		isp_unmap_sys_2_mc(isp, rgbir_output);

		if (rgbir_output->sys_img_buf_hdl)
			isp_sys_mem_free(rgbir_output->sys_img_buf_hdl);
		isp_sys_mem_free(rgbir_output);
	};

	isp_dbg_print_info("<- %s suc\n", __func__);
}

void isp_fw_resp_error(struct isp_context *isp,
		enum fw_cmd_resp_stream_id stream_id, RespError_t *para)
{
	isp = isp;
	stream_id = stream_id;
	para = para;
}

void isp_fw_resp_heart_beat(struct isp_context *isp)
{
	isp = isp;
//	isp_dbg_print_err("-><- %s\n", __func__);
};

void isp_fw_resp_func(struct isp_context *isp,
			enum fw_cmd_resp_stream_id stream_id)
{
	result_t result;
	Resp_t resp;

	if (ISP_GET_STATUS(isp) < ISP_STATUS_FW_RUNNING)
		return;
	isp_fw_log_print(isp);
	result = isp_get_f2h_resp(isp, stream_id, &resp);
	if (result != RET_SUCCESS)
		return;
	switch (resp.respId) {
	case RESP_ID_CMD_DONE:
		isp_fw_resp_cmd_done(isp, stream_id,
			(RespCmdDone_t *) resp.respParam);
		return;
	case RESP_ID_FRAME_DONE:
		isp_fw_resp_frame_done(isp, stream_id,
			(RespParamPackage_t *) resp.respParam);
		return;
	case RESP_ID_FRAME_INFO:
		isp_fw_resp_frame_info(isp, stream_id,
			(RespParamPackage_t *) resp.respParam);
		return;
	case RESP_ID_ERROR:
		isp_fw_resp_error(isp, stream_id,
			(RespError_t *) resp.respParam);
		return;

	case RESP_ID_HEART_BEAT:
		isp_fw_resp_heart_beat(isp);
		return;

	default:
		isp_dbg_print_err("-><- %s,fail respid %s(0x%x)\n", __func__,
			isp_dbg_get_resp_str(resp.respId), resp.respId);
		break;
	}
};

int32 isp_fw_resp_thread_wrapper(pvoid context)
{
	struct isp_fw_resp_thread_para *para = context;
	struct isp_context *isp;
	enum fw_cmd_resp_stream_id stream_id;

	if (para == NULL) {
		isp_dbg_print_err("-><- %s fail para\n", __func__);
		goto quit;
	};
	switch (para->idx) {
	case 0:
		stream_id = FW_CMD_RESP_STREAM_ID_GLOBAL;
		break;
	case 1:
		stream_id = FW_CMD_RESP_STREAM_ID_1;
		break;
	case 2:
		stream_id = FW_CMD_RESP_STREAM_ID_2;
		break;
	case 3:
		stream_id = FW_CMD_RESP_STREAM_ID_3;
		break;
	default:
		isp_dbg_print_err("-><- %s fail idx[%u]\n",
			__func__, para->idx);
		goto quit;
	};

	isp = para->isp;
	isp_dbg_print_info("-><- %s[%u] started\n", __func__, para->idx);

	while (true) {
		isp_event_wait(&isp->fw_resp_thread[para->idx].wakeup_evt,
			WORK_ITEM_INTERVAL);
		if (thread_should_stop(&isp->fw_resp_thread[para->idx])) {
			isp_dbg_print_info
				("-><- %s[%u] quit\n", __func__, para->idx);
			stop_work_thread(&isp->fw_resp_thread[para->idx]);
			break;
		};

		isp_mutex_lock(&isp->fw_resp_thread[para->idx].mutex);
		isp_fw_resp_func(isp, stream_id);
		isp_mutex_unlock(&isp->fw_resp_thread[para->idx].mutex);
	}
 quit:
	return 0;
};

uint32 isp_get_stream_output_bits(struct isp_context *isp,
				enum camera_id cam_id, uint32 *stream_cnt)
{
	uint32 ret = 0;
	uint32 cnt = 0;
	enum start_status stat;

	if (stream_cnt)
		*stream_cnt = 0;
	if (!is_para_legal(isp, cam_id)) {
		isp_dbg_print_err("-><- %s, fail for bad para", __func__);
		return 0;
	}

	stat =
	isp->sensor_info[cam_id].str_info[STREAM_ID_PREVIEW].start_status;
	if ((stat == START_STATUS_STARTED)
	|| (stat == START_STATUS_STARTING)) {
		ret |= STREAM__PREVIEW_OUTPUT_BIT;
		cnt++;
	}

	stat = isp->sensor_info[cam_id].str_info[STREAM_ID_VIDEO].start_status;
	if ((stat == START_STATUS_STARTED)
	|| (stat == START_STATUS_STARTING)) {
		ret |= STREAM__VIDEO_OUTPUT_BIT;
		cnt++;
	}

	stat = isp->sensor_info[cam_id].str_info[STREAM_ID_ZSL].start_status;
	if ((stat == START_STATUS_STARTED)
	|| (stat == START_STATUS_STARTING)) {
		ret |= STREAM__ZSL_OUTPUT_BIT;
		cnt++;
	};

	if (isp->sensor_info[cam_id].take_one_pic_left_cnt) {
		ret |= STREAM__TAKE_ONE_YUV_OUTPUT_BIT;
		cnt++;
	};

	if (isp->sensor_info[cam_id].take_one_raw_left_cnt) {
		ret |= STREAM__TAKE_ONE_RAW_OUTPUT_BIT;
		cnt++;
	};

	if (stream_cnt)
		*stream_cnt = cnt;

	return ret;
}

uint32 isp_get_started_stream_count(struct isp_context *isp)
{
	uint32 cnt = 0;
	enum camera_id cid;

	if (isp == NULL)
		return 0;
	for (cid = CAMERA_ID_REAR; cid < CAMERA_ID_MAX; cid++) {
		if (isp->sensor_info[cid].status == START_STATUS_STARTED)
			cnt++;
	};

	return cnt;
}

result_t isp_fw_work_mem_rsv(struct isp_context *isp, uint64 *sys_addr,
			uint64 *mc_addr)
{
	cgs_handle_t handle = 0;
	int ret;

	if (isp == NULL) {
		isp_dbg_print_info
			("-><- %s fail by invalid para\n", __func__);
		return RET_INVALID_PARAM;
	};

	isp_dbg_print_info("-> %s\n", __func__);
	memset(&isp->fw_work_buf, 0, sizeof(struct isp_gpu_mem_info));
	ret = g_cgs_srv->ops->alloc_gpu_mem(g_cgs_srv,
			CGS_GPU_MEM_TYPE__GART_WRITECOMBINE,
			ISP_FW_WORK_BUF_SIZE, ISP_MC_ADDR_ALIGN, 0,
			(uint64) 4 * 1024 * 1024 * 1024, &handle);
	if (ret != 0) {
		isp_dbg_print_err
			("<- %s fail for alloc_gpu_mem\n", __func__);
		handle = 0;
		goto fail;
	};

	ret = g_cgs_srv->ops->gmap_gpu_mem(g_cgs_srv, handle,
				&isp->fw_work_buf.gpu_mc_addr);
	if (ret != 0) {
		isp_dbg_print_err
			("<- %s fail for gmap_gpu_mem\n", __func__);
		isp->fw_work_buf.gpu_mc_addr = 0;
		goto fail;
	};

	ret = g_cgs_srv->ops->kmap_gpu_mem(g_cgs_srv, handle,
					&isp->fw_work_buf.sys_addr);
	if (ret != 0) {
		isp_dbg_print_err
			("<- %s fail for kmap_gpu_mem\n", __func__);
		isp->fw_work_buf.sys_addr = NULL;
		goto fail;
	};

	if (sys_addr)
		*sys_addr = (uint64) isp->fw_work_buf.sys_addr;
	if (mc_addr)
		*mc_addr = isp->fw_work_buf.gpu_mc_addr;
	isp->fw_work_buf.handle = handle;

	isp_dbg_print_info("<- %s,succ\n", __func__);

	return RET_SUCCESS;
fail:
	if (handle && isp->fw_work_buf.sys_addr)
		g_cgs_srv->ops->kunmap_gpu_mem(g_cgs_srv, handle);
	if (handle && isp->fw_work_buf.gpu_mc_addr)
		g_cgs_srv->ops->gunmap_gpu_mem(g_cgs_srv, handle);
	if (handle)
		g_cgs_srv->ops->free_gpu_mem(g_cgs_srv, handle);
	isp->fw_work_buf.handle = 0;

	return RET_FAILURE;
}


result_t isp_fw_work_mem_free(struct isp_context *isp)
{
	cgs_handle_t handle = 0;

	if (isp == NULL) {
		isp_dbg_print_info
			("-><- %s fail by invalid para\n", __func__);
		return RET_INVALID_PARAM;
	};
	isp_dbg_print_info("-> %s\n", __func__);
	handle = isp->fw_work_buf.handle;
	if (handle && isp->fw_work_buf.sys_addr)
		g_cgs_srv->ops->kunmap_gpu_mem(g_cgs_srv, handle);
	if (handle && isp->fw_work_buf.gpu_mc_addr)
		g_cgs_srv->ops->gunmap_gpu_mem(g_cgs_srv, handle);
	if (handle)
		g_cgs_srv->ops->free_gpu_mem(g_cgs_srv, handle);
	isp->fw_work_buf.handle = 0;

	isp_dbg_print_info("<- %s,succ\n", __func__);
	return RET_SUCCESS;
};


void init_cam_calib_item(struct isp_context *isp, enum camera_id cid)
{
	uint32 i;

	for (i = 0; i < MAX_CALIB_ITEM_COUNT; i++)
		isp->calib_items[cid][i] = NULL;
};

void uninit_cam_calib_item(struct isp_context *isp, enum camera_id cid)
{
	uint32 i;

	struct calib_date_item *cur, *next;

	for (i = 0; i < MAX_CALIB_ITEM_COUNT; i++) {
		cur = isp->calib_items[cid][i];
		isp->calib_items[cid][i] = NULL;
		while (cur != NULL) {
			next = cur->next;
			isp_sys_mem_free(cur);
			cur = next;
		}
	};
	if (isp->calib_data[cid])
		isp_sys_mem_free(isp->calib_data[cid]);
	isp->calib_data[cid] = NULL;
	isp->calib_data_len[cid] = 0;
};

void init_calib_items(struct isp_context *isp)
{
	enum camera_id cid;

	uninit_calib_items(isp);
	for (cid = CAMERA_ID_REAR; cid < CAMERA_ID_MAX; cid++)
		init_cam_calib_item(isp, cid);
};

void uninit_calib_items(struct isp_context *isp)
{
	enum camera_id cid;

	for (cid = CAMERA_ID_REAR; cid < CAMERA_ID_MAX; cid++)
		uninit_cam_calib_item(isp, cid);
};

bool calib_crc_check(struct calib_pkg_td_header *pkg)
{
	pkg = pkg;
	return true;
}

struct calib_date_item *isp_get_calib_item(struct isp_context *isp,
					enum camera_id cid)
{
	struct calib_date_item *item;
	struct res_fps_t *res_fps;
	uint32 i;

	if (!is_para_legal(isp, cid)) {
		isp_dbg_print_err
			("-><- %s fail bad para, isp:%p,cid:%d\n",
			__func__, isp, cid);
		return NULL;
	};
	res_fps =
		&isp->sensor_info[cid].res_fps.res_fps[
		(int)(isp->sensor_info[cid].cur_res_fps_id)];

	isp_dbg_print_info
		("-> %s, cid:%d,w:h=%u:%u,fps:%u,hdr:%u\n", __func__, cid,
		res_fps->width, res_fps->height, res_fps->fps,
		isp->sensor_info[cid].hdr_enable);

	for (i = 0; i < MAX_CALIB_ITEM_COUNT; i++) {
		item = isp->calib_items[cid][i];
		if (item == NULL) {
			break;
		};

		if ((res_fps->height == item->height)
		&& (res_fps->width == item->width)
		&& (res_fps->fps == item->fps)
		&& ((isp->sensor_info[cid].hdr_enable && item->hdr)
		|| (!isp->sensor_info[cid].hdr_enable && !item->hdr))) {
			isp_dbg_print_info
		("<- %s suc, perfect cid:%d,%p(%u),w:h=%u:%u,fps:%u,hdr:%u\n",
				__func__, cid, item->data, item->len,
				item->width, item->height,
				item->fps, item->hdr);
			return item;
		};
	};

	for (i = 0; i < MAX_CALIB_ITEM_COUNT; i++) {
		item = isp->calib_items[cid][i];
		if (item == NULL) {
			break;
		};
		if ((res_fps->height > item->height)
		&& (res_fps->width > item->width)
		&& (res_fps->fps == item->fps)
		&& ((isp->sensor_info[cid].hdr_enable && item->hdr)
		|| (!isp->sensor_info[cid].hdr_enable && !item->hdr))) {
			isp_dbg_print_info
		("<- %s suc, 2nd cid:%d,%p(%u),w:h=%u:%u,fps:%u,hdr:%u\n",
				__func__, cid, item->data, item->len,
				item->width, item->height,
				item->fps, item->hdr);
			return item;
		};
	};

	for (i = 0; i < MAX_CALIB_ITEM_COUNT; i++) {
		item = isp->calib_items[cid][i];
		if (item == NULL) {
			break;
		};
		if ((res_fps->height >= item->height)
		&& (res_fps->width >= item->width)
		&& ((isp->sensor_info[cid].hdr_enable && item->hdr)
		|| (!isp->sensor_info[cid].hdr_enable && !item->hdr))) {
			isp_dbg_print_info
		("<- %s suc, 3rd cid:%d,%p(%u),w:h=%u:%u,fps:%u,hdr:%u\n",
				__func__, cid, item->data, item->len,
				item->width, item->height,
				item->fps, item->hdr);
			return item;
		};
	};

	for (i = 0; i < MAX_CALIB_ITEM_COUNT; i++) {
		item = isp->calib_items[cid][i];
		if (item == NULL) {
			break;
		};
		if ((res_fps->height >= item->height)
		&& (res_fps->width >= item->width)
		&& (res_fps->fps == item->fps)) {
			isp_dbg_print_info
		("<- %s suc, 4th cid:%d,%p(%u),w:h=%u:%u,fps:%u,hdr:%u\n",
				__func__, cid, item->data, item->len,
				item->width, item->height,
				item->fps, item->hdr);
			return item;
		};
	};

	for (i = 0; i < MAX_CALIB_ITEM_COUNT; i++) {
		item = isp->calib_items[cid][i];
		if (item == NULL) {
			break;
		};
		if ((res_fps->height >= item->height)
		&& (res_fps->width >= item->width)) {
			isp_dbg_print_info
		("<- %s suc, 5th cid:%d,%p(%u),w:h=%u:%u,fps:%u,hdr:%u\n",
				__func__, cid, item->data, item->len,
				item->width, item->height,
				item->fps, item->hdr);
			return item;
		};
	}

	item = isp->calib_items[cid][0];
	if (item == NULL) {
		isp_dbg_print_err("%s fail cid:%d, nofound\n", __func__, cid);
		return item;
	}

	isp_dbg_print_info
		("<- %s suc, no match cid:%d,%p(%u),w:h=%u:%u,fps:%u,hdr:%u\n",
		__func__, cid, item->data, item->len,
		item->width, item->height,
		item->fps, item->hdr);
	return item;
}

void add_calib_item(struct isp_context *isp, enum camera_id cid,
			profile_header *profile, void *data, uint32 len)
{
	uint32 arr_size;
//	struct calib_date_item *cur, *next;
	uint32 i;
//	uint32 w, h;
	struct calib_date_item *item;

	if (profile == NULL)
		return;

	arr_size = MAX_CALIB_ITEM_COUNT;
	for (i = 0; i < arr_size; i++) {
		item = isp->calib_items[cid][i];
		if (item == NULL) {
			item = isp_sys_mem_alloc
				(sizeof(struct calib_date_item));
			if (item == NULL) {
				isp_dbg_print_err("-><- %s fail", __func__);
				return;
			}
			isp->calib_items[cid][i] = item;
			item->next = NULL;
			item->data = data;
			item->len = len;
			item->width = profile->width;
			item->height = profile->height;
			item->fps = profile->fps;
			item->hdr = profile->hdr;
			break;
		};
	}

	isp_dbg_print_info
		("-><- %s suc cid %u,w:h=%u:%u,fps:%u,hdr:%u,buf:%p(%u)\n",
		__func__, cid, profile->width, profile->height,
		profile->fps, profile->hdr, data, len);

	if (1) {
		uint32 *db = (uint32 *) data;

		isp_dbg_print_info("%08x %08x %08x %08x %08x %08x %08x %08x\n",
		db[0], db[1], db[2], db[3], db[4], db[5], db[6], db[7]);
	}
}


unsigned char parse_calib_data(struct isp_context *isp, enum camera_id cam_id,
			uint8 *data, uint32 len)
{
	uint32_t tempi;
	package_td_header *ph;
	struct sensor_info *sif;
	profile_header profile;

	if (!is_para_legal(isp, cam_id)) {
		isp_dbg_print_err("-><- %s fail bad para, isp:%p,cid:%d\n",
			__func__, isp, cam_id);
		return false;
	};

	if ((len == 0) || (data == NULL)) {
		isp_dbg_print_info("-><- %s fail calib,cid:%d,len %d\n",
			__func__, cam_id, len);
		return false;
	};

	if (isp->calib_data[cam_id])
		isp_sys_mem_free(isp->calib_data[cam_id]);
	isp->calib_data[cam_id] = isp_sys_mem_alloc(len);
	if (isp->calib_data[cam_id] == NULL) {
		isp_dbg_print_info("-><- %s fail, aloc fail,cid:%d,len %d\n",
			__func__, cam_id, len);
		return false;
	}
	sif = &isp->sensor_info[cam_id];
	memcpy(isp->calib_data[cam_id], data, len);
	data = isp->calib_data[cam_id];
	isp->calib_data_len[cam_id] = len;
	ph = (package_td_header *) data;
//	sif->cproc_value = NULL;
	sif->wb_idx = NULL;
	sif->scene_tbl = NULL;
	isp_dbg_print_info("-> %s cid:%d,len %d\n", __func__, cam_id, len);

	if ((ph->magic_no_high != 0x33ffffff)
	|| (ph->magic_no_low != 0xffffffff)) {
		isp_dbg_print_info("in %s,old format\n", __func__);
		goto old_tuning_data;
	} else {
		isp_dbg_print_info("in %s,new format,cropWnd %u:%u\n",
			__func__, ph->crop_wnd_width, ph->crop_wnd_height);
	};
	for (tempi = 0;
	tempi < min((uint32)ph->pro_no, (uint32)MAX_PROFILE_NO); tempi++) {
		uint8_t *tb;

		tb = data + ph->pro[tempi].offset;
		if ((ph->pro[tempi].offset + ph->pro[tempi].size) > len) {
			isp_dbg_print_info
				("in %s fail tuning data,%u,%u should< %u\n",
				__func__, tempi,
				(ph->pro[tempi].offset + ph->pro[tempi].size),
				len);
			goto old_tuning_data;
		};
	};
	for (tempi = 0;
	tempi < min((uint32)ph->pro_no, (uint32)MAX_PROFILE_NO); tempi++) {
		uint8_t *tb;

		tb = data + ph->pro[tempi].offset;
		add_calib_item(isp, cam_id, &ph->pro[tempi],
			tb, ph->pro[tempi].size);
	}
	//sif->cproc_value = &ph->cproc_value;
	sif->wb_idx = &ph->wb_idx;
	sif->scene_tbl = &ph->scene_mode_table;
	isp_dbg_print_info("<- %s suc\n", __func__);
	return true;
 old_tuning_data:
	profile.width = 1936;
	profile.height = 1096;
	profile.fps = 15;
	profile.hdr = 0;
	add_calib_item(isp, cam_id, &profile, isp->calib_data[cam_id], len);
	profile.width = 1920;
	profile.height = 1080;
	profile.fps = 15;
	profile.hdr = 0;
	add_calib_item(isp, cam_id, &profile, isp->calib_data[cam_id], len);
	profile.width = 1312;
	profile.height = 816;
	profile.fps = 15;
	profile.hdr = 0;
	add_calib_item(isp, cam_id, &profile, isp->calib_data[cam_id], len);
	profile.width = 640;
	profile.height = 480;
	profile.fps = 15;
	profile.hdr = 0;
	add_calib_item(isp, cam_id, &profile, isp->calib_data[cam_id], len);
	profile.width = 648;
	profile.height = 488;
	profile.fps = 30;
	profile.hdr = 0;
	add_calib_item(isp, cam_id, &profile, isp->calib_data[cam_id], len);
	isp_dbg_print_info("<- %s suc\n", __func__);
	return true;
}

void isp_clear_cmdq(struct isp_context *isp)
{
	struct isp_cmd_element *cur;
	struct isp_cmd_element *nxt;

	isp_mutex_lock(&isp->cmd_q_mtx);
	cur = isp->cmd_q;
	isp->cmd_q = NULL;
	while (cur != NULL) {
		nxt = cur->next;
		if (cur->mc_addr)
			isp_fw_ret_pl(isp->fw_work_buf_hdl, cur->mc_addr);
		isp_sys_mem_free(cur);
		cur = nxt;
	};
	isp_mutex_unlock(&isp->cmd_q_mtx);

	isp_dbg_print_info("-><- %s\n", __func__);
}

void isp_test_reg_rw(struct isp_context *isp)
{
	uint32 r1;
	uint32 r2;
	uint32 w1 = 0x55555555;

	isp = isp;
	isp_dbg_print_info("-> %s\n", __func__);
	r1 = isp_hw_reg_read32(mmISP_VERSION);
	isp_dbg_print_info("mmISP_VERSION 0x%x\n", r1);
	r1 = isp_hw_reg_read32(mmISP_CCPU_SCRATCH6);
	isp_hw_reg_write32(mmISP_CCPU_SCRATCH6, w1);
	r2 = isp_hw_reg_read32(mmISP_CCPU_SCRATCH6);
	isp_dbg_print_info("mmISP_CCPU_SCRATCH6 r:0x%x,w:0x%x,r:0x%x\n",
		r1, w1, r2);
	isp_dbg_print_info("<- %s suc\n", __func__);
};

void init_frame_ctl_cmd(struct sensor_info *sif, FrameControl_t *fc,
			struct frame_ctl_info *ctl_info,
			struct isp_mapped_buf_info *zsl_buf)
{
	ActionType_t default_act = ACTION_TYPE_USE_STREAM;
	ActionType_t new_act = ACTION_TYPE_USE_STREAM;
	bool_t check_ae_mode = false;
	bool_t check_af_mode = false;

	if (fc == NULL) {
		isp_dbg_print_info("%s, bad para\n", __func__);
		return;
	}
	memset(fc, 0, sizeof(FrameControl_t));
	fc->controlType = FRAME_CONTROL_TYPE_HALF;

	fc->ae.aeMode.actionType = default_act;
	fc->ae.aeFlickerType.actionType = default_act;
	fc->ae.aeRegion.actionType = default_act;
	fc->ae.aeLock.actionType = default_act;
	fc->ae.aeExposure.actionType = default_act;
	fc->ae.aeIso.actionType = default_act;
	fc->ae.aeEv.actionType = default_act;
	fc->ae.aeFlashMode.actionType = default_act;
	fc->ae.aeFlashPowerLevel.actionType = default_act;
	fc->ae.aeRedEyeMode.actionType = default_act;

	fc->af.afMode.actionType = default_act;
	fc->af.afRegion.actionType = default_act;
	fc->af.afLensPos.actionType = default_act;
	fc->af.afTrig.actionType = default_act;
	fc->af.afCancel.actionType = default_act;
	fc->af.afFocusAssist.actionType = default_act;

	fc->awb.awbMode.actionType = default_act;
	fc->awb.awbLock.actionType = default_act;
	fc->awb.awbGain.actionType = default_act;
	fc->awb.awbCcMatrix.actionType = default_act;
	fc->awb.awbCcOffset.actionType = default_act;
	fc->awb.awbColorTemp.actionType = default_act;

	if ((ctl_info == NULL) || (sif == NULL))
		return;

	if (ctl_info->set_ae_mode) {
		if ((ctl_info->ae_mode != AE_MODE_INVALID)
		&& (ctl_info->ae_mode != AE_MODE_MAX)) {
			fc->ae.aeMode.actionType = new_act;
			fc->ae.aeMode.mode = ctl_info->ae_mode;
			isp_dbg_print_info("%s,ae mode %d\n",
				__func__, fc->ae.aeMode.mode);
		} else {
			isp_dbg_print_info("%s,ignore ae mode %d\n",
				__func__, fc->ae.aeMode.mode);
		}
	};

	if (ctl_info->set_itime) {
		if (ctl_info->ae_para.itime
		&& (sif->sensor_cfg.exposure.min_integration_time <=
		ctl_info->ae_para.itime)
		&& (sif->sensor_cfg.exposure.max_integration_time >=
		ctl_info->ae_para.itime)) {
			fc->ae.aeExposure.actionType = new_act;
			fc->ae.aeExposure.exposure.itime =
				ctl_info->ae_para.itime;
			check_ae_mode = true;
			isp_dbg_print_info("%s,itime %u\n",
				__func__, ctl_info->ae_para.itime);
		} else {
			isp_dbg_print_info("%s,ignore itime %u[%u,%u]\n",
				__func__, ctl_info->ae_para.itime,
				sif->sensor_cfg.exposure.min_integration_time,
				sif->sensor_cfg.exposure.max_integration_time);
		}
	}

	if (ctl_info->set_flash_mode) {
		if ((ctl_info->flash_mode != FLASH_MODE_INVALID)
		&& (ctl_info->flash_mode != FLASH_MODE_MAX)) {
			fc->ae.aeFlashMode.actionType = new_act;
			fc->ae.aeFlashMode.flashMode = ctl_info->flash_mode;
			check_ae_mode = true;
			isp_dbg_print_info("%s,flashMode %u\n",
				__func__, ctl_info->flash_mode);
		} else {
			isp_dbg_print_info("%s,ignore flashMode %u\n",
				__func__, ctl_info->flash_mode);
		}
	};

	if (ctl_info->set_falsh_pwr) {
		fc->ae.aeFlashPowerLevel.actionType = new_act;
		fc->ae.aeFlashPowerLevel.powerLevel = ctl_info->flash_pwr;
		check_ae_mode = true;
		isp_dbg_print_info("%s,powerLevel %u\n",
			__func__, ctl_info->flash_pwr);
	};

	if (ctl_info->set_ec) {
		fc->ae.aeEv.actionType = new_act;
		fc->ae.aeEv.ev = ctl_info->ae_ec;
		check_ae_mode = true;
		isp_dbg_print_info("%s,ae_ec %d/%u\n", __func__,
			ctl_info->ae_ec.numerator,
			ctl_info->ae_ec.denominator);
	};

	if (ctl_info->set_iso) {
		fc->ae.aeIso.actionType = new_act;
		fc->ae.aeIso.iso = ctl_info->iso;
		check_ae_mode = true;
		isp_dbg_print_info("%s,iso %u\n", __func__, ctl_info->iso.iso);
	};

	if (check_ae_mode && (fc->ae.aeMode.actionType == new_act)
	&& (fc->ae.aeMode.mode != AE_MODE_MANUAL)) {
		fc->ae.aeMode.mode = AE_MODE_MANUAL;
		isp_dbg_print_info
			("%s, change aemode from auto to manual\n", __func__);
	};

	if (ctl_info->set_af_mode) {
		if ((ctl_info->af_mode == AF_MODE_INVALID)
		|| (ctl_info->af_mode == AF_MODE_MAX)) {
			isp_dbg_print_info("%s,ignore af_mode %u\n",
				__func__, ctl_info->af_mode);
		} else if (sif->lens_range.maxLens == 0) {
			isp_dbg_print_info
				("%s,ignore af_mode %u for maxlen is 0\n",
				__func__, ctl_info->af_mode);
		} else {
			fc->af.afMode.actionType = new_act;
			fc->af.afMode.mode = ctl_info->af_mode;
			isp_dbg_print_info("%s,af_mode %u\n",
				__func__, ctl_info->af_mode);
		}
	};

	if (ctl_info->set_af_lens_pos) {
		if (ctl_info->af_lens_pos
		&& (ctl_info->af_lens_pos >= sif->lens_range.minLens)
		&& (ctl_info->af_lens_pos <= sif->lens_range.maxLens)) {
			fc->af.afLensPos.actionType = new_act;
			fc->af.afLensPos.lensPos = ctl_info->af_lens_pos;
			check_af_mode = true;
			isp_dbg_print_info("%s,lensPos %u\n",
				__func__, ctl_info->af_lens_pos);
		} else {
			isp_dbg_print_info
				("%s,ignore lens pos %u[%u,%u]\n", __func__,
				ctl_info->af_lens_pos,
				sif->lens_range.minLens,
				sif->lens_range.maxLens);
		}
	};

	if (check_af_mode && (fc->af.afMode.actionType == new_act)
	&& (fc->af.afMode.mode != AF_MODE_MANUAL)) {
		fc->af.afMode.mode = AF_MODE_MANUAL;
		isp_dbg_print_info("%s, change afmode to manual\n", __func__);
	};

	if (zsl_buf == NULL)
		return;

	fc->zslBuf.enabled = 1;
	fc->zslBuf.buffer.vmidSpace = 4;
	isp_split_addr64(zsl_buf->y_map_info.mc_addr,
			&fc->zslBuf.buffer.bufBaseALo,
			&fc->zslBuf.buffer.bufBaseAHi);
	fc->zslBuf.buffer.bufSizeA = zsl_buf->y_map_info.len;

	isp_split_addr64(zsl_buf->u_map_info.mc_addr,
			&fc->zslBuf.buffer.bufBaseBLo,
			&fc->zslBuf.buffer.bufBaseBHi);
	fc->zslBuf.buffer.bufSizeB = zsl_buf->u_map_info.len;

	isp_split_addr64(zsl_buf->v_map_info.mc_addr,
			&fc->zslBuf.buffer.bufBaseCLo,
			&fc->zslBuf.buffer.bufBaseCHi);
	fc->zslBuf.buffer.bufSizeC = zsl_buf->v_map_info.len;
}


result_t isp_send_rgbir_raw_info_buf(struct isp_context *isp,
				struct isp_mapped_buf_info *buffer,
				enum camera_id cam_id)
{
	CmdSendBuffer_t cmd;
	enum fw_cmd_resp_stream_id stream;
	result_t result;

	if (!is_para_legal(isp, cam_id)
	|| (buffer == NULL)) {
		isp_dbg_print_err
			("-><- %s,fail para,isp %p,buf %p,cid %u\n",
			__func__, isp, buffer, cam_id);
		return RET_FAILURE;
	};

	memset(&cmd, 0, sizeof(cmd));
	cmd.bufferType = BUFFER_TYPE_RAW_TEMP;

	stream = isp_get_stream_id_from_cid(isp, cam_id);
	cmd.buffer.vmidSpace = 4;
	isp_split_addr64(buffer->y_map_info.mc_addr,
			&cmd.buffer.bufBaseALo, &cmd.buffer.bufBaseAHi);
	cmd.buffer.bufSizeA = buffer->y_map_info.len;

	isp_split_addr64(buffer->u_map_info.mc_addr,
			&cmd.buffer.bufBaseBLo, &cmd.buffer.bufBaseBHi);
	cmd.buffer.bufSizeB = buffer->u_map_info.len;

	result = isp_send_fw_cmd(isp, CMD_ID_SEND_BUFFER, stream,
				FW_CMD_PARA_TYPE_DIRECT, &cmd, sizeof(cmd));

	if (result != RET_SUCCESS) {
		isp_dbg_print_err
			("-><- %s,fail send raw %p,cid %u\n",
			__func__, buffer, cam_id);
		return RET_FAILURE;
	};

	memset(&cmd, 0, sizeof(cmd));
	cmd.bufferType = BUFFER_TYPE_FRAME_INFO;

	stream = isp_get_stream_id_from_cid(isp, cam_id);
	cmd.buffer.vmidSpace = 4;

	isp_split_addr64(buffer->v_map_info.mc_addr,
			&cmd.buffer.bufBaseALo, &cmd.buffer.bufBaseAHi);
	cmd.buffer.bufSizeA = buffer->v_map_info.len;

	result = isp_send_fw_cmd(isp, CMD_ID_SEND_BUFFER, stream,
				FW_CMD_PARA_TYPE_DIRECT, &cmd, sizeof(cmd));

	if (result != RET_SUCCESS) {
		isp_dbg_print_err
			("-><- %s,fail send frameinfo %p,cid %u\n",
			__func__, buffer, cam_id);
		return RET_FAILURE;
	};

	isp_dbg_print_verb("-><- %s,suc,buf %p,cid %u\n",
		__func__, buffer, cam_id);
	return RET_SUCCESS;
}


SensorId_t isp_get_fw_sensor_id(struct isp_context *isp, enum camera_id cid)
{
	if (cid >= CAMERA_ID_MAX)
		return SENSOR_ID_INVALID;
	if (cid == CAMERA_ID_MEM)
		return SENSOR_ID_RDMA;
	if (isp->sensor_info[cid].cam_type == CAMERA_TYPE_MEM)
		return SENSOR_ID_RDMA;
	if (cid == CAMERA_ID_REAR)
		return SENSOR_ID_A;
	return SENSOR_ID_B;
}

StreamId_t isp_get_fw_stream_from_drv_stream(enum fw_cmd_resp_stream_id stream)
{
	switch (stream) {
	case FW_CMD_RESP_STREAM_ID_1:
		return STREAM_ID_1;
	case FW_CMD_RESP_STREAM_ID_2:
		return STREAM_ID_2;
	case FW_CMD_RESP_STREAM_ID_3:
		return STREAM_ID_3;
	default:
		return (uint16) STREAM_ID_INVALID;
	}

}

bool_t isp_is_mem_camera(struct isp_context *isp, enum camera_id cid)
{
	if (cid >= CAMERA_ID_MAX)
		return 0;
	if (cid == CAMERA_ID_MEM)
		return 1;
	if (isp->sensor_info[cid].cam_type == CAMERA_TYPE_MEM)
		return 1;
	return 0;
};


bool isp_is_fpga(void)
{
	if (g_isp_env_setting == ISP_ENV_SILICON)
		return 0;
	else
		return 1;
}

result_t isp_hw_mem_read_(uint64 mem_addr, pvoid p_read_buffer,
			uint32 byte_size)
{
#ifndef USING_KGD_CGS
	if (isp_is_fpga() && g_cgs_srv.cgs_gpumemcopy) {
		//g_cgs_srv
		struct cgs_gpu_mem_copy_input input;

		input.mc_dest = (uint64) p_read_buffer;
		input.mc_src = mem_addr;
		input.size = byte_size;
		g_cgs_srv.cgs_gpumemcopy(g_cgs_srv.cgs_device, &input);
	}
#endif
	return RET_SUCCESS;
};

result_t isp_hw_mem_write_(uint64 mem_addr, pvoid p_write_buffer,
				uint32 byte_size)
{
#ifndef USING_KGD_CGS
	if (isp_is_fpga() && g_cgs_srv.cgs_gpumemcopy) {
		//g_cgs_srv
		struct cgs_gpu_mem_copy_input input;

		input.mc_dest = mem_addr;
		input.mc_src = (uint64) p_write_buffer;
		input.size = byte_size;
		g_cgs_srv.cgs_gpumemcopy(g_cgs_srv.cgs_device, &input);
	}
#endif
	return RET_SUCCESS;
};

//dphy initializaion code
//refer to start_internal_synopsys_phy
#define ENABLE_MIPI_PHY_DDL_TUNING
#define MIPI_PHY0_REG_BASE (mmISP_PHY0_CFG_REG0)
#define MIPI_PHY1_REG_BASE (mmISP_PHY1_CFG_REG0)

typedef enum _mipi_phy_id_t {
	MIPI_PHY_ID_0 = 0,
	MIPI_PHY_ID_1 = 1,
	MIPI_PHY_ID_MAX
} mipi_phy_id_t;

typedef struct {
	uint32 isp_phy_cfg_reg0;
	uint32 isp_phy_cfg_reg1;
	uint32 isp_phy_cfg_reg2;
	uint32 isp_phy_cfg_reg3;
	uint32 isp_phy_cfg_reg4;
	uint32 isp_phy_cfg_reg5;
	uint32 isp_phy_cfg_reg6;
	uint32 isp_phy_cfg_reg7;
	uint32 isp_phy_cfg_reserved[52];
	uint32 isp_phy_testinterf_cmd;
	uint32 isp_phy_testinterf_ack;
} mipi_phy_reg_t;

mipi_phy_reg_t *g_isp_mipi_phy_reg_base[MIPI_PHY_ID_MAX] = {
	(mipi_phy_reg_t *) MIPI_PHY0_REG_BASE,
	(mipi_phy_reg_t *) MIPI_PHY1_REG_BASE,
};

const uint32_t g_isp_mipi_status_mask[4] = { 0x1, 0x3, 0x7, 0xf };


//refer to start_internal_synopsys_phy
//bit_rate is in unit of Mbit/s
#define g_mipi_phy_reg_base g_isp_mipi_phy_reg_base
#define mipi_status_mask g_isp_mipi_status_mask
result_t start_internal_synopsys_phy(struct isp_context *pcontext,
				uint32 cam_id,
				uint32 bit_rate/*in Mbps */,
				uint32 lane_num)
{
	uint32_t regVal;
	uint32_t timeout = 0;
	uint32_t phy_reg_check_timeout = 10;	// 1 second
	uint32_t cmd_data;
	uint32_t mipi_phy_id;

	pcontext = pcontext;
	//=================================================
	//   Start Programming
	//=================================================

	// program PHY0
	if (cam_id == CAMERA_ID_REAR)
		mipi_phy_id = 0;
	else
		mipi_phy_id = 1;

	// shutdown MIPI PHY
	regVal = 0x0;
	MODIFYFLD(regVal, ISP_PHY0_CFG_REG0, shutdownz, 0x0);
	MODIFYFLD(regVal, ISP_PHY0_CFG_REG0, rstz, 0x0);
	aidt_hal_write_reg((uint64_t)
		(&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_cfg_reg0), regVal);

	// set testclr to 0x1
	regVal = 0x0;
	MODIFYFLD(regVal, ISP_PHY0_CFG_REG7, testclr, 0x1);
	aidt_hal_write_reg((uint64_t)
		(&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_cfg_reg7), regVal);

	//FIXME: adjust to 1ms later!!!
	usleep_range(1000, 2000);

	// set testclr to 0x0
	regVal = 0x0;
	aidt_hal_write_reg((uint64_t)
		(&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_cfg_reg7), regVal);

	// enable the clock lane and corresponding data lanes
	regVal = 0x0;
	MODIFYFLD(regVal, ISP_PHY0_CFG_REG4, enableclk, 0x1);
	if (lane_num == 1) {
		MODIFYFLD(regVal, ISP_PHY0_CFG_REG4, enable_0, 0x1);
	} else if (lane_num == 2) {
		MODIFYFLD(regVal, ISP_PHY0_CFG_REG4, enable_0, 0x1);
		MODIFYFLD(regVal, ISP_PHY0_CFG_REG4, enable_1, 0x1);
	} else if (lane_num == 3) {
		MODIFYFLD(regVal, ISP_PHY0_CFG_REG4, enable_0, 0x1);
		MODIFYFLD(regVal, ISP_PHY0_CFG_REG4, enable_1, 0x1);
		MODIFYFLD(regVal, ISP_PHY0_CFG_REG4, enable_2, 0x1);
	} else if (lane_num == 4) {
		MODIFYFLD(regVal, ISP_PHY0_CFG_REG4, enable_0, 0x1);
		MODIFYFLD(regVal, ISP_PHY0_CFG_REG4, enable_1, 0x1);
		MODIFYFLD(regVal, ISP_PHY0_CFG_REG4, enable_2, 0x1);
		MODIFYFLD(regVal, ISP_PHY0_CFG_REG4, enable_3, 0x1);
	} else {
		isp_dbg_print_err("%s, Lane num error\n", __func__);
		return RET_FAILURE;
	}
	aidt_hal_write_reg((uint64_t)
		(&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_cfg_reg4), regVal);

	// program cfgclkfreqrange
	aidt_hal_write_reg((uint64_t)
		(&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_cfg_reg3), 0x1C);

	// program forcerxmode = 0
	aidt_hal_write_reg((uint64_t)
		(&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_cfg_reg5), 0x0);

	// program turndisable = 0
	aidt_hal_write_reg((uint64_t)
		(&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_cfg_reg6), 0x0);

	// program hsfreqrange
	regVal = 0x0;
	if ((bit_rate >= 83.125) && ((bit_rate <= 118.125)))
		regVal = 0x20;
	else if ((bit_rate >= 149.625) && ((bit_rate <= 191.625)))
		regVal = 0x12;
	else if ((bit_rate >= 182.875) && ((bit_rate <= 228.375)))
		regVal = 0x3;
	else if ((bit_rate >= 273.125) && ((bit_rate <= 328.125)))
		regVal = 0x14;
	else if ((bit_rate >= 320.625) && ((bit_rate <= 380.625)))
		regVal = 0x35;
	else if ((bit_rate >= 368.125) && ((bit_rate <= 433.125)))
		regVal = 0x5;
	else if ((bit_rate >= 463.125) && ((bit_rate <= 538.125)))
		regVal = 0x26;
	else if ((bit_rate >= 558.125) && ((bit_rate <= 643.125)))
		regVal = 0x28;
	else if ((bit_rate >= 653.125) && ((bit_rate <= 748.125)))
		regVal = 0x28;
	else if ((bit_rate >= 748.125) && ((bit_rate <= 853.125)))
		regVal = 0x9;
	// default 900Mbps
	else if ((bit_rate >= 843.125) && ((bit_rate <= 958.125)))
		regVal = 0x29;
	// default 960Mbps
	else if ((bit_rate >= 890.625) && ((bit_rate <= 1010.625)))
		regVal = 0x3a;
	else if ((bit_rate >= 985.625) && ((bit_rate <= 1115.625)))
		regVal = 0x1a;
	// default 1.2Gbps
	else if ((bit_rate >= 1128.125) && ((bit_rate <= 1273.125)))
		regVal = 0x1C;
	else {
		isp_dbg_print_err
			("%s, Invalid data bitrate for sensor\n", __func__);
		return RET_FAILURE;
	}
	aidt_hal_write_reg((uint64_t)
		(&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_cfg_reg2), regVal);

#ifdef ENABLE_MIPI_PHY_DDL_TUNING
	// config DDL tuning
	if (((mipi_phy_id == 0) && (lane_num == 4))
	|| ((mipi_phy_id == 1) && (lane_num == 2))) {
	// case for all lanes enabled
		// read 0xE4 register
		regVal = 0x0;
		MODIFYFLD(regVal, ISP_PHY0_TESTINTERF_CMD, CMD_WRITE, 0x0);
		MODIFYFLD(regVal, ISP_PHY0_TESTINTERF_CMD, CMD_ADDR, 0xE4);

		aidt_hal_write_reg((uint64_t)
		(&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_cmd),
			regVal);

		while (timeout < phy_reg_check_timeout) {

			regVal = aidt_hal_read_reg((uint64_t)
		(&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_ack));

			if (regVal & ISP_PHY0_TESTINTERF_ACK__CMD_ACK_MASK) {
				isp_dbg_print_info
					("Cmd finished with read count = %d\n",
					timeout);
				break;
			}

			usleep_range(1000, 2000);

			timeout = timeout + 1;
		}
		if (timeout == phy_reg_check_timeout) {
			isp_dbg_print_err("Timeout for reading register 0xE4");
			return RET_FAILURE;
		}
		cmd_data = regVal & ISP_PHY0_TESTINTERF_ACK__CMD_RDATA_MASK;

		// write 0xE4 register
		regVal = 0x0;
		MODIFYFLD(regVal, ISP_PHY0_TESTINTERF_CMD, CMD_WRITE, 0x1);
		MODIFYFLD(regVal, ISP_PHY0_TESTINTERF_CMD, CMD_ADDR, 0xE4);
		MODIFYFLD(regVal, ISP_PHY0_TESTINTERF_CMD, CMD_WDATA,
			cmd_data | 0x1);

		aidt_hal_write_reg((uint64_t)
		(&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_cmd),
			regVal);

		timeout = 0x0;
		while (timeout < phy_reg_check_timeout) {

			regVal = aidt_hal_read_reg((uint64_t)
		(&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_ack));

			if (regVal & ISP_PHY0_TESTINTERF_ACK__CMD_ACK_MASK) {
				isp_dbg_print_info
					("Cmd finished with read count = %d\n",
					timeout);
				break;
			}

			usleep_range(1000, 2000);

			timeout = timeout + 1;
		}
		if (timeout == phy_reg_check_timeout) {
			isp_dbg_print_err("Timeout for writing register 0xE4");
			return RET_FAILURE;
		}
		// write 0xE2 register
		regVal = 0x0;
		MODIFYFLD(regVal, ISP_PHY0_TESTINTERF_CMD, CMD_WRITE, 0x1);
		MODIFYFLD(regVal, ISP_PHY0_TESTINTERF_CMD, CMD_ADDR, 0xE2);
		MODIFYFLD(regVal, ISP_PHY0_TESTINTERF_CMD, CMD_WDATA, 0x32);

		aidt_hal_write_reg((uint64_t)
		(&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_cmd),
			regVal);

		timeout = 0x0;
		while (timeout < phy_reg_check_timeout) {
			regVal = aidt_hal_read_reg((uint64_t)
		(&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_ack));
			if (regVal & ISP_PHY0_TESTINTERF_ACK__CMD_ACK_MASK) {
				isp_dbg_print_info
					("Cmd finished with read count = %d\n",
					timeout);
				break;
			}

			usleep_range(1000, 2000);

			timeout = timeout + 1;
		}
		if (timeout == phy_reg_check_timeout) {
			isp_dbg_print_err("Timeout when writing register 0xE2");
			return RET_FAILURE;
		}
		// write 0xE3 register
		regVal = 0x0;
		MODIFYFLD(regVal, ISP_PHY0_TESTINTERF_CMD, CMD_WRITE, 0x1);
		MODIFYFLD(regVal, ISP_PHY0_TESTINTERF_CMD, CMD_ADDR, 0xE3);
		MODIFYFLD(regVal, ISP_PHY0_TESTINTERF_CMD, CMD_WDATA, 0x1);
		aidt_hal_write_reg((uint64_t)
		(&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_cmd),
			regVal);

		timeout = 0x0;
		while (timeout < phy_reg_check_timeout) {

			regVal = aidt_hal_read_reg((uint64_t)
		(&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_ack));

			if (regVal & ISP_PHY0_TESTINTERF_ACK__CMD_ACK_MASK) {
				isp_dbg_print_info
					("Cmd finished with read count = %d\n",
					timeout);
				break;
			}

			usleep_range(1000, 2000);

			timeout = timeout + 1;
		}
		if (timeout == phy_reg_check_timeout) {
			isp_dbg_print_err("Timeout for writing register 0xE3");
			return RET_FAILURE;
		}
	} else if ((mipi_phy_id == 0) && (lane_num == 2)) {
	// case for MIPI PHY 0 with 2-lane enabled
	// read 0x60C register
		regVal = 0x0;
		MODIFYFLD(regVal, ISP_PHY0_TESTINTERF_CMD, CMD_WRITE, 0x0);
		MODIFYFLD(regVal, ISP_PHY0_TESTINTERF_CMD, CMD_ADDR, 0x60C);

		aidt_hal_write_reg((uint64_t)
		(&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_cmd),
			regVal);

		while (timeout < phy_reg_check_timeout) {

			regVal = aidt_hal_read_reg((uint64_t)
		(&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_ack));
			if (regVal & ISP_PHY0_TESTINTERF_ACK__CMD_ACK_MASK) {
				isp_dbg_print_info
					("Cmd finished with read count = %d\n",
					timeout);
				break;
			}

			usleep_range(1000, 2000);

			timeout = timeout + 1;
		}
		if (timeout == phy_reg_check_timeout) {
			isp_dbg_print_err
				("Timeout for reading register 0x60C");
			return RET_FAILURE;
		}
		cmd_data = regVal & ISP_PHY0_TESTINTERF_ACK__CMD_RDATA_MASK;

		// write 0x60C register
		regVal = 0x0;
		MODIFYFLD(regVal, ISP_PHY0_TESTINTERF_CMD, CMD_WRITE, 0x1);
		MODIFYFLD(regVal, ISP_PHY0_TESTINTERF_CMD, CMD_ADDR, 0x60C);
		MODIFYFLD(regVal, ISP_PHY0_TESTINTERF_CMD, CMD_WDATA,
							cmd_data | 0x1);
		aidt_hal_write_reg((uint64_t)
		(&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_cmd),
			regVal);

		timeout = 0x0;
		while (timeout < phy_reg_check_timeout) {
			regVal = aidt_hal_read_reg((uint64_t)
		(&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_ack));

			if (regVal & ISP_PHY0_TESTINTERF_ACK__CMD_ACK_MASK) {
				isp_dbg_print_info
					("Cmd finished with read count = %d\n",
					timeout);
				break;
			}

			usleep_range(1000, 2000);

			timeout = timeout + 1;
		}
		if (timeout == phy_reg_check_timeout) {
			isp_dbg_print_err
				("Timeout when writing register 0x60C");
			return RET_FAILURE;
		}
		// write 0x60A register
		regVal = 0x0;
		MODIFYFLD(regVal, ISP_PHY0_TESTINTERF_CMD, CMD_WRITE, 0x1);
		MODIFYFLD(regVal, ISP_PHY0_TESTINTERF_CMD, CMD_ADDR, 0x60A);
		MODIFYFLD(regVal, ISP_PHY0_TESTINTERF_CMD, CMD_WDATA, 0x32);

		aidt_hal_write_reg((uint64_t)
		(&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_cmd),
				regVal);

		timeout = 0x0;
		while (timeout < phy_reg_check_timeout) {

			regVal = aidt_hal_read_reg((uint64_t)
		(&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_ack));

			if (regVal & ISP_PHY0_TESTINTERF_ACK__CMD_ACK_MASK) {
				isp_dbg_print_info
					("Cmd finished with read count = %d\n",
					timeout);
				break;
			}

			usleep_range(1000, 2000);

			timeout = timeout + 1;
		}
		if (timeout == phy_reg_check_timeout) {
			isp_dbg_print_err
				("Timeout when writing register 0x60A");
			return RET_FAILURE;
		}
		// write 0x60B register
		regVal = 0x0;
		MODIFYFLD(regVal, ISP_PHY0_TESTINTERF_CMD, CMD_WRITE, 0x1);
		MODIFYFLD(regVal, ISP_PHY0_TESTINTERF_CMD, CMD_ADDR, 0x60B);
		MODIFYFLD(regVal, ISP_PHY0_TESTINTERF_CMD, CMD_WDATA, 0x1);

		aidt_hal_write_reg((uint64_t)
		(&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_cmd),
				regVal);

		timeout = 0x0;
		while (timeout < phy_reg_check_timeout) {

			regVal = aidt_hal_read_reg((uint64_t)
		(&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_ack));

			if (regVal & ISP_PHY0_TESTINTERF_ACK__CMD_ACK_MASK) {
				isp_dbg_print_info
					("Cmd finished with read count = %d\n",
					timeout);
				break;
			}

			usleep_range(1000, 2000);

			timeout = timeout + 1;
		}
		if (timeout == phy_reg_check_timeout) {
			isp_dbg_print_err
				("Timeout when writing register 0x60B");
			return RET_FAILURE;
		}
		// read 0x80C register
		regVal = 0x0;
		MODIFYFLD(regVal, ISP_PHY0_TESTINTERF_CMD, CMD_WRITE, 0x0);
		MODIFYFLD(regVal, ISP_PHY0_TESTINTERF_CMD, CMD_ADDR, 0x80C);

		aidt_hal_write_reg((uint64_t)
		(&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_cmd),
				regVal);

		while (timeout < phy_reg_check_timeout) {

			regVal = aidt_hal_read_reg((uint64_t)
		(&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_ack));

			if (regVal & ISP_PHY0_TESTINTERF_ACK__CMD_ACK_MASK) {
				isp_dbg_print_info
					("Cmd finished with read count = %d\n",
					timeout);
				break;
			}

			usleep_range(1000, 2000);

			timeout = timeout + 1;
		}
		if (timeout == phy_reg_check_timeout) {
			isp_dbg_print_err
				("Timeout when reading register 0x80C");
			return RET_FAILURE;
		}
		cmd_data = regVal & ISP_PHY1_TESTINTERF_ACK__CMD_RDATA_MASK;

		// write 0x80C register
		regVal = 0x0;
		MODIFYFLD(regVal, ISP_PHY0_TESTINTERF_CMD, CMD_WRITE, 0x1);
		MODIFYFLD(regVal, ISP_PHY0_TESTINTERF_CMD, CMD_ADDR, 0x80C);
		MODIFYFLD(regVal, ISP_PHY0_TESTINTERF_CMD, CMD_WDATA,
						cmd_data | 0x1);
		aidt_hal_write_reg((uint64_t)
		(&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_cmd),
					regVal);

		timeout = 0x0;
		while (timeout < phy_reg_check_timeout) {

			regVal = aidt_hal_read_reg((uint64_t)
		(&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_ack));

			if (regVal & ISP_PHY0_TESTINTERF_ACK__CMD_ACK_MASK) {
				isp_dbg_print_info
					("Cmd finished with read count = %d\n",
					timeout);
				break;
			}

			usleep_range(1000, 2000);

			timeout = timeout + 1;
		}
		if (timeout == phy_reg_check_timeout) {
			isp_dbg_print_err
				("Timeout when writing register 0x80C");
			return RET_FAILURE;
		}
		// write 0x80A register
		regVal = 0x0;
		MODIFYFLD(regVal, ISP_PHY0_TESTINTERF_CMD, CMD_WRITE, 0x1);
		MODIFYFLD(regVal, ISP_PHY0_TESTINTERF_CMD, CMD_ADDR, 0x80A);
		MODIFYFLD(regVal, ISP_PHY0_TESTINTERF_CMD, CMD_WDATA, 0x32);

		aidt_hal_write_reg((uint64_t)
		(&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_cmd),
				regVal);

		timeout = 0x0;
		while (timeout < phy_reg_check_timeout) {

			regVal = aidt_hal_read_reg((uint64_t)
		(&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_ack));

			if (regVal & ISP_PHY0_TESTINTERF_ACK__CMD_ACK_MASK) {
				isp_dbg_print_info
					("Cmd finished with read count = %d\n",
					timeout);
				break;
			}

			usleep_range(1000, 2000);

			timeout = timeout + 1;
		}
		if (timeout == phy_reg_check_timeout) {
			isp_dbg_print_err
				("Timeout when writing register 0x80A");
			return RET_FAILURE;
		}
		// write 0x80B register
		regVal = 0x0;
		MODIFYFLD(regVal, ISP_PHY0_TESTINTERF_CMD, CMD_WRITE, 0x1);
		MODIFYFLD(regVal, ISP_PHY0_TESTINTERF_CMD, CMD_ADDR, 0x80B);
		MODIFYFLD(regVal, ISP_PHY0_TESTINTERF_CMD, CMD_WDATA, 0x1);

		aidt_hal_write_reg((uint64_t)
		(&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_cmd),
					regVal);

		timeout = 0x0;
		while (timeout < phy_reg_check_timeout) {

			regVal = aidt_hal_read_reg((uint64_t)
		(&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_ack));

			if (regVal & ISP_PHY0_TESTINTERF_ACK__CMD_ACK_MASK) {
				isp_dbg_print_info
					("Cmd finished with read count = %d\n",
					timeout);
				break;
			}

			usleep_range(1000, 2000);

			timeout = timeout + 1;
		}
		if (timeout == phy_reg_check_timeout) {
			isp_dbg_print_err
				("Timeout when writing register 0x80B");
			return RET_FAILURE;
		}
	} else if (lane_num == 1) {
	// case for PHY0/PHY1 with 1-lane enabled
		// read 0x60C register
		regVal = 0x0;
		MODIFYFLD(regVal, ISP_PHY0_TESTINTERF_CMD, CMD_WRITE, 0x0);
		MODIFYFLD(regVal, ISP_PHY0_TESTINTERF_CMD, CMD_ADDR, 0x60C);

		aidt_hal_write_reg((uint64_t)
		(&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_cmd),
					regVal);

		while (timeout < phy_reg_check_timeout) {

			regVal = aidt_hal_read_reg((uint64_t)
		(&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_ack));

			if (regVal & ISP_PHY0_TESTINTERF_ACK__CMD_ACK_MASK) {
				isp_dbg_print_info
					("Cmd finished with read count = %d\n",
					timeout);
				break;
			}

			usleep_range(1000, 2000);

			timeout = timeout + 1;
		}
		if (timeout == phy_reg_check_timeout) {
			isp_dbg_print_err
				("Timeout when reading register 0x60C");
			return RET_FAILURE;
		}
		cmd_data = regVal & ISP_PHY0_TESTINTERF_ACK__CMD_RDATA_MASK;

		// write 0x60C register
		regVal = 0x0;
		MODIFYFLD(regVal, ISP_PHY0_TESTINTERF_CMD, CMD_WRITE, 0x1);
		MODIFYFLD(regVal, ISP_PHY0_TESTINTERF_CMD, CMD_ADDR, 0x60C);
		MODIFYFLD(regVal, ISP_PHY0_TESTINTERF_CMD, CMD_WDATA,
							cmd_data | 0x1);
		aidt_hal_write_reg((uint64_t)
		(&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_cmd),
					regVal);

		timeout = 0x0;
		while (timeout < phy_reg_check_timeout) {

			regVal = aidt_hal_read_reg((uint64_t)
		(&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_ack));

			if (regVal & ISP_PHY0_TESTINTERF_ACK__CMD_ACK_MASK) {
				isp_dbg_print_info
					("Cmd finished with read count = %d\n",
					timeout);
				break;
			}

			usleep_range(1000, 2000);

			timeout = timeout + 1;
		}
		if (timeout == phy_reg_check_timeout) {
			isp_dbg_print_err
				("Timeout when writing register 0x60C");
			return RET_FAILURE;
		}
		// write 0x60A register
		regVal = 0x0;
		MODIFYFLD(regVal, ISP_PHY0_TESTINTERF_CMD, CMD_WRITE, 0x1);
		MODIFYFLD(regVal, ISP_PHY0_TESTINTERF_CMD, CMD_ADDR, 0x60A);
		MODIFYFLD(regVal, ISP_PHY0_TESTINTERF_CMD, CMD_WDATA, 0x32);

		aidt_hal_write_reg((uint64_t)
		(&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_cmd),
					regVal);

		timeout = 0x0;
		while (timeout < phy_reg_check_timeout) {

			regVal = aidt_hal_read_reg((uint64_t)
		(&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_ack));

			if (regVal & ISP_PHY0_TESTINTERF_ACK__CMD_ACK_MASK) {
				isp_dbg_print_info
					("Cmd finished with read count = %d\n",
					timeout);
				break;
			}

			usleep_range(1000, 2000);

			timeout = timeout + 1;
		}
		if (timeout == phy_reg_check_timeout) {
			isp_dbg_print_err
				("Timeout when writing register 0x60A");
			return RET_FAILURE;
		}
		// write 0x60B register
		regVal = 0x0;
		MODIFYFLD(regVal, ISP_PHY0_TESTINTERF_CMD, CMD_WRITE, 0x1);
		MODIFYFLD(regVal, ISP_PHY0_TESTINTERF_CMD, CMD_ADDR, 0x60B);
		MODIFYFLD(regVal, ISP_PHY0_TESTINTERF_CMD, CMD_WDATA, 0x1);

		aidt_hal_write_reg((uint64_t)
		(&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_cmd),
				regVal);

		timeout = 0x0;
		while (timeout < phy_reg_check_timeout) {

			regVal = aidt_hal_read_reg((uint64_t)
		(&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_ack));

			if (regVal & ISP_PHY0_TESTINTERF_ACK__CMD_ACK_MASK) {
				isp_dbg_print_info
					("Cmd finished with read count = %d\n",
					timeout);
				break;
			}

			usleep_range(1000, 2000);

			timeout = timeout + 1;
		}
		if (timeout == phy_reg_check_timeout) {
			isp_dbg_print_err
				("Timeout when writing register 0x60B");
			return RET_FAILURE;
		}
	} else {
		isp_dbg_print_info("Invalid lane num configuration");
		return RET_FAILURE;
	}
#endif

	// kick off MIPI PHY by 2 steps
	// release shutdownz first
	regVal = 0x0;
	MODIFYFLD(regVal, ISP_PHY0_CFG_REG0, shutdownz, 0x1);
	aidt_hal_write_reg((uint64_t)
		(&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_cfg_reg0), regVal);

	// release rstz secondly
	regVal = 0x0;
	MODIFYFLD(regVal, ISP_PHY0_CFG_REG0, shutdownz, 0x1);
	MODIFYFLD(regVal, ISP_PHY0_CFG_REG0, rstz, 0x1);
	aidt_hal_write_reg((uint64_t)
		(&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_cfg_reg0), regVal);

	// wait 1ms
	usleep_range(1000, 2000);

//==========================================================================
// Check PHY auto calibration status with 4 steps:
// 1. check termination calibration sequence with rext
// 2. check clock lane offset calibration
// 3. check data lane 0/1/2/3 offset calibration
// 4. check DDL tuning calibration for lane 0/1/2/3
// !!!No need to do force calibration since we suppose PHY auto calibration
// must be successful. Otherwise, MIPI PHY should have some design issue and
// need the PHY designer to investigate the issue. We can duduce that PHY has
// functional issue and ISP cannot work with sensor input.
//==========================================================================

	//==== 1. check termination calibration status
	// read 0x221 register
	regVal = 0x0;
	MODIFYFLD(regVal, ISP_PHY0_TESTINTERF_CMD, CMD_WRITE, 0x0);
	MODIFYFLD(regVal, ISP_PHY0_TESTINTERF_CMD, CMD_ADDR, 0x221);

	aidt_hal_write_reg((uint64_t)
	(&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_cmd), regVal);

	regVal = aidt_hal_read_reg((uint64_t)
		(&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_ack));

	cmd_data = regVal & ISP_PHY0_TESTINTERF_ACK__CMD_RDATA_MASK;
	// check calibration done status. cmd_data[7] == 0x1
	if (0x80 == (cmd_data & 0x80)) {
		// read 0x222 register
		regVal = 0x0;
		MODIFYFLD(regVal, ISP_PHY0_TESTINTERF_CMD, CMD_WRITE, 0x0);
		MODIFYFLD(regVal, ISP_PHY0_TESTINTERF_CMD, CMD_ADDR, 0x222);
		aidt_hal_write_reg((uint64_t)
		(&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_cmd),
				regVal);

		regVal = aidt_hal_read_reg((uint64_t)
		(&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_ack));

		cmd_data = regVal & ISP_PHY0_TESTINTERF_ACK__CMD_RDATA_MASK;
		if ((cmd_data & 0x1) == 0x0) {
			isp_dbg_print_warn
				("MIPI PHY termination calibration passes!\n");
		} else {
			isp_dbg_print_err
				("MIPI PHY termination calibration fails!\n");
		}
	}

	//==== 2. check clock lane offset calibration status
	// read 0x39D register
	regVal = 0x0;
	MODIFYFLD(regVal, ISP_PHY0_TESTINTERF_CMD, CMD_WRITE, 0x0);
	MODIFYFLD(regVal, ISP_PHY0_TESTINTERF_CMD, CMD_ADDR, 0x39D);

	aidt_hal_write_reg((uint64_t)
	(&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_cmd), regVal);

	regVal = aidt_hal_read_reg((uint64_t)
		(&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_ack));

	cmd_data = regVal & ISP_PHY0_TESTINTERF_ACK__CMD_RDATA_MASK;
	// chack calibration done status. cmd_data[0] == 0x1
	if ((cmd_data & 0x1) == 0x1) {
		if (((cmd_data & 0x1E) >> 0x1) != 0x8) {
			isp_dbg_print_warn
			("MIPI PHY clock lane offset calibration passes!\n");
		} else {
			isp_dbg_print_err
			("MIPI PHY clock lane offset calibration fails!\n");
		}
	}

	//==== 3. check data lane offset calibration status
	// read 0x59F register
	if (lane_num == 1) {
		regVal = 0x0;
		MODIFYFLD(regVal, ISP_PHY0_TESTINTERF_CMD, CMD_WRITE, 0x0);
		MODIFYFLD(regVal, ISP_PHY0_TESTINTERF_CMD, CMD_ADDR, 0x59F);

		aidt_hal_write_reg((uint64_t)
		(&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_cmd),
				regVal);

		regVal = aidt_hal_read_reg((uint64_t)
		(&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_ack));

		cmd_data = regVal & ISP_PHY0_TESTINTERF_ACK__CMD_RDATA_MASK;

		// cmd_data[1] == 1
		if ((cmd_data & 0x2) == 0x2) {
			// cmd_data[2] == 0
			if ((cmd_data & 0x4) == 0x0) {
				isp_dbg_print_warn
			("MIPI PHY data lane offset calibration passes!\n");
			} else {
				isp_dbg_print_err
			("MIPI PHY data lane offset calibration fails!\n");
			}
		}
	} else if (lane_num == 2) {
		// check data lane 0 offset calibration status
		regVal = 0x0;
		MODIFYFLD(regVal, ISP_PHY0_TESTINTERF_CMD, CMD_WRITE, 0x0);
		MODIFYFLD(regVal, ISP_PHY0_TESTINTERF_CMD, CMD_ADDR, 0x59F);

		aidt_hal_write_reg((uint64_t)
		(&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_cmd),
					regVal);

		regVal = aidt_hal_read_reg((uint64_t)
		(&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_ack));

		cmd_data = regVal & ISP_PHY0_TESTINTERF_ACK__CMD_RDATA_MASK;

		// cmd_data[1] == 1
		if ((cmd_data & 0x2) == 0x2) {
			// cmd_data[2] == 0
			if ((cmd_data & 0x4) == 0x0) {
				isp_dbg_print_warn
			("MIPI PHY data lane 0 offset calibration passes!\n");
			} else {
				isp_dbg_print_err
			("MIPI PHY data lane 0 offset calibration fails!\n");
			}
		}
		// check data lane 1 offset calibration status
		regVal = 0x0;
		MODIFYFLD(regVal, ISP_PHY0_TESTINTERF_CMD, CMD_WRITE, 0x0);
		MODIFYFLD(regVal, ISP_PHY0_TESTINTERF_CMD, CMD_ADDR, 0x79F);

		aidt_hal_write_reg((uint64_t)
		(&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_cmd),
				regVal);

		regVal = aidt_hal_read_reg((uint64_t)
		(&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_ack));

		cmd_data = regVal & ISP_PHY0_TESTINTERF_ACK__CMD_RDATA_MASK;

		// cmd_data[1] == 1
		if ((cmd_data & 0x2) == 0x2) {
			// cmd_data[2] == 0
			if ((cmd_data & 0x4) == 0x0) {
				isp_dbg_print_warn
			("MIPI PHY data lane 1 offset calibration passes!\n");
			} else {
				isp_dbg_print_err
			("MIPI PHY data lane 1 offset calibration fails!\n");
			}
		}
	} else if (lane_num == 4) {
		// check data lane 0 offset calibration status
		regVal = 0x0;
		MODIFYFLD(regVal, ISP_PHY0_TESTINTERF_CMD, CMD_WRITE, 0x0);
		MODIFYFLD(regVal, ISP_PHY0_TESTINTERF_CMD, CMD_ADDR, 0x59F);

		aidt_hal_write_reg((uint64_t)
		(&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_cmd),
					regVal);

		regVal = aidt_hal_read_reg((uint64_t)
		(&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_ack));

		cmd_data = regVal & ISP_PHY0_TESTINTERF_ACK__CMD_RDATA_MASK;

		// cmd_data[1] == 1
		if ((cmd_data & 0x2) == 0x2) {
			// cmd_data[2] == 0
			if ((cmd_data & 0x4) == 0x0) {
				isp_dbg_print_warn
			("MIPI PHY data lane 0 offset calibration passes!\n");
			} else {
				isp_dbg_print_err
			("MIPI PHY data lane 0 offset calibration fails!\n");
			}
		}
		// check data lane 1 offset calibration status
		regVal = 0x0;
		MODIFYFLD(regVal, ISP_PHY0_TESTINTERF_CMD, CMD_WRITE, 0x0);
		MODIFYFLD(regVal, ISP_PHY0_TESTINTERF_CMD, CMD_ADDR, 0x79F);

		aidt_hal_write_reg((uint64_t)
		(&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_cmd),
					regVal);

		regVal = aidt_hal_read_reg((uint64_t)
		(&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_ack));

		cmd_data = regVal & ISP_PHY0_TESTINTERF_ACK__CMD_RDATA_MASK;

		// cmd_data[1] == 1
		if ((cmd_data & 0x2) == 0x2) {
			// cmd_data[2] == 0
			if ((cmd_data & 0x4) == 0x0) {
				isp_dbg_print_warn
			("MIPI PHY data lane 1 offset calibration passes!\n");
			} else {
				isp_dbg_print_err
			("MIPI PHY data lane 1 offset calibration fails!\n");
			}
		}
		// check data lane 2 offset calibration status
		regVal = 0x0;
		MODIFYFLD(regVal, ISP_PHY0_TESTINTERF_CMD, CMD_WRITE, 0x0);
		MODIFYFLD(regVal, ISP_PHY0_TESTINTERF_CMD, CMD_ADDR, 0x99F);

		aidt_hal_write_reg((uint64_t)
		(&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_cmd),
					regVal);

		regVal = aidt_hal_read_reg((uint64_t)
		(&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_ack));

		cmd_data = regVal & ISP_PHY0_TESTINTERF_ACK__CMD_RDATA_MASK;

		// cmd_data[1] == 1
		if ((cmd_data & 0x2) == 0x2) {
			// cmd_data[2] == 0
			if ((cmd_data & 0x4) == 0x0) {
				isp_dbg_print_warn
			("MIPI PHY data lane 2 offset calibration passes!\n");
			} else {
				isp_dbg_print_err
			("MIPI PHY data lane 2 offset calibration fails!\n");
			}
		}
		// check data lane 3 offset calibration status
		regVal = 0x0;
		MODIFYFLD(regVal, ISP_PHY0_TESTINTERF_CMD, CMD_WRITE, 0x0);
		MODIFYFLD(regVal, ISP_PHY0_TESTINTERF_CMD, CMD_ADDR, 0xB9F);

		aidt_hal_write_reg((uint64_t)
		(&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_cmd),
					regVal);

		regVal = aidt_hal_read_reg((uint64_t)
		(&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_ack));

		cmd_data = regVal & ISP_PHY0_TESTINTERF_ACK__CMD_RDATA_MASK;

		// cmd_data[1] == 1
		if ((cmd_data & 0x2) == 0x2) {
			// cmd_data[2] == 0
			if ((cmd_data & 0x4) == 0x0) {
				isp_dbg_print_warn
			("MIPI PHY data lane 3 offset calibration passes!\n");
			} else {
				isp_dbg_print_err
			("MIPI PHY data lane 3 offset calibration fails!\n");
			}
		}
	}

	//==== 4. check DDL tuning calibration status
	// read 0x5E0 register
	if (lane_num == 1) {
		regVal = 0x0;
		MODIFYFLD(regVal, ISP_PHY0_TESTINTERF_CMD, CMD_WRITE, 0x0);
		MODIFYFLD(regVal, ISP_PHY0_TESTINTERF_CMD, CMD_ADDR, 0x5E0);

		aidt_hal_write_reg((uint64_t)
		(&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_cmd),
					regVal);

		regVal = aidt_hal_read_reg((uint64_t)
		(&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_ack));

		cmd_data = regVal & ISP_PHY0_TESTINTERF_ACK__CMD_RDATA_MASK;

		// cmd_data[2] == 1
		if ((cmd_data & 0x4) == 0x4) {
			// cmd_data[1] == 0
			if ((cmd_data & 0x2) == 0x0) {
				isp_dbg_print_warn
			("MIPI PHY DDL tuning calibration passes!\n");
			} else {
				isp_dbg_print_err
			("MIPI PHY DDL tuning calibration fails!\n");
			}
		}
	} else if (lane_num == 2) {
		// check data lane 0 DDL tuning calibration status
		regVal = 0x0;
		MODIFYFLD(regVal, ISP_PHY0_TESTINTERF_CMD, CMD_WRITE, 0x0);
		MODIFYFLD(regVal, ISP_PHY0_TESTINTERF_CMD, CMD_ADDR, 0x5E0);

		aidt_hal_write_reg((uint64_t)
		(&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_cmd),
					regVal);

		regVal = aidt_hal_read_reg((uint64_t)
		(&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_ack));

		cmd_data = regVal & ISP_PHY0_TESTINTERF_ACK__CMD_RDATA_MASK;

		// cmd_data[2] == 1
		if ((cmd_data & 0x4) == 0x4) {
			// cmd_data[1] == 0
			if ((cmd_data & 0x2) == 0x0) {
				isp_dbg_print_warn
		("MIPI PHY data lane 0 DDL tuning calibration passes!\n");
			} else {
				isp_dbg_print_err
		("MIPI PHY data lane 0 DDL tuning calibration fails!\n");
			}
		}
		// check data lane 1 DDL tuning calibration status
		regVal = 0x0;
		MODIFYFLD(regVal, ISP_PHY0_TESTINTERF_CMD, CMD_WRITE, 0x0);
		MODIFYFLD(regVal, ISP_PHY0_TESTINTERF_CMD, CMD_ADDR, 0x7E0);

		aidt_hal_write_reg((uint64_t)
		(&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_cmd),
					regVal);

		regVal = aidt_hal_read_reg((uint64_t)
		(&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_ack));

		cmd_data = regVal & ISP_PHY0_TESTINTERF_ACK__CMD_RDATA_MASK;

		// cmd_data[2] == 1
		if ((cmd_data & 0x4) == 0x4) {
			// cmd_data[1] == 0
			if ((cmd_data & 0x2) == 0x0) {
				isp_dbg_print_warn
		("MIPI PHY data lane 1 DDL tuning calibration passes!\n");
			} else {
				isp_dbg_print_err
		("MIPI PHY data lane 1 DDL tuning calibration fails!\n");
			}
		}
	} else if (lane_num == 4) {
		// check data lane 0 DDL tuning calibration status
		regVal = 0x0;
		MODIFYFLD(regVal, ISP_PHY0_TESTINTERF_CMD, CMD_WRITE, 0x0);
		MODIFYFLD(regVal, ISP_PHY0_TESTINTERF_CMD, CMD_ADDR, 0x5E0);

		aidt_hal_write_reg((uint64_t)
		(&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_cmd),
					regVal);

		regVal = aidt_hal_read_reg((uint64_t)
		(&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_ack));

		cmd_data = regVal & ISP_PHY0_TESTINTERF_ACK__CMD_RDATA_MASK;

		// cmd_data[2] == 1
		if ((cmd_data & 0x4) == 0x4) {
			// cmd_data[1] == 0
			if ((cmd_data & 0x2) == 0x0) {
				isp_dbg_print_warn
		("MIPI PHY data lane 0 DDL tuning calibration passes!\n");
			} else {
				isp_dbg_print_err
		("MIPI PHY data lane 0 DDL tuning calibration fails!\n");
			}
		}
		// check data lane 1 DDL tuning calibration status
		regVal = 0x0;
		MODIFYFLD(regVal, ISP_PHY0_TESTINTERF_CMD, CMD_WRITE, 0x0);
		MODIFYFLD(regVal, ISP_PHY0_TESTINTERF_CMD, CMD_ADDR, 0x7E0);

		aidt_hal_write_reg((uint64_t)
		(&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_cmd),
					regVal);

		regVal = aidt_hal_read_reg((uint64_t)
		(&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_ack));

		cmd_data = regVal & ISP_PHY0_TESTINTERF_ACK__CMD_RDATA_MASK;

		// cmd_data[2] == 1
		if ((cmd_data & 0x4) == 0x4) {
			// cmd_data[1] == 0
			if ((cmd_data & 0x2) == 0x0) {
				isp_dbg_print_warn
		("MIPI PHY data lane 1 DDL tuning calibration passes!\n");
			} else {
				isp_dbg_print_err
		("MIPI PHY data lane 1 DDL tuning calibration fails!\n");
			}
		}
		// check data lane 2 DDL tuning calibration status
		regVal = 0x0;
		MODIFYFLD(regVal, ISP_PHY0_TESTINTERF_CMD, CMD_WRITE, 0x0);
		MODIFYFLD(regVal, ISP_PHY0_TESTINTERF_CMD, CMD_ADDR, 0x9E0);

		aidt_hal_write_reg((uint64_t)
		(&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_cmd),
					regVal);

		regVal = aidt_hal_read_reg((uint64_t)
		(&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_ack));

		cmd_data = regVal & ISP_PHY0_TESTINTERF_ACK__CMD_RDATA_MASK;

		// cmd_data[2] == 1
		if ((cmd_data & 0x4) == 0x4) {
			// cmd_data[1] == 0
			if ((cmd_data & 0x2) == 0x0) {
				isp_dbg_print_warn
		("MIPI PHY data lane 2 DDL tuning calibration passes!\n");
			} else {
				isp_dbg_print_err
		("MIPI PHY data lane 2 DDL tuning calibration fails!\n");
			}
		}
		// check data lane 3 DDL tuning calibration status
		regVal = 0x0;
		MODIFYFLD(regVal, ISP_PHY0_TESTINTERF_CMD, CMD_WRITE, 0x0);
		MODIFYFLD(regVal, ISP_PHY0_TESTINTERF_CMD, CMD_ADDR, 0xBE0);

		aidt_hal_write_reg((uint64_t)
		(&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_cmd),
					regVal);

		regVal = aidt_hal_read_reg((uint64_t)
		(&g_mipi_phy_reg_base[mipi_phy_id]->isp_phy_testinterf_ack));

		cmd_data = regVal & ISP_PHY0_TESTINTERF_ACK__CMD_RDATA_MASK;

		// cmd_data[2] == 1
		if ((cmd_data & 0x4) == 0x4) {
			// cmd_data[1] == 0
			if ((cmd_data & 0x2) == 0x0) {
				isp_dbg_print_warn
		("MIPI PHY data lane 3 DDL tuning calibration passes!\n");
			} else {
				isp_dbg_print_err
		("MIPI PHY data lane 3 DDL tuning calibration fails!\n");
			}
		}
	}

	return RET_SUCCESS;
}

result_t shutdown_internal_synopsys_phy(uint32_t device_id, uint32_t lane_num)
{
	uint32_t regVal;
	uint32_t timeout = 0;
	uint32_t phy_reg_check_timeout = 2;	// 1 second

	//=================================================
	// SNPS PHY shutdown sequence
	//=================================================
	if (device_id == 0) {
		// step 1: check the stop status
		while (timeout < phy_reg_check_timeout) {
			regVal = aidt_hal_read_reg(mmISP_MIPI_MIPI0_STATUS);
			if ((regVal &
			ISP_MIPI_MIPI0_STATUS__MRV_MIPI_S_STOPSTATE_CLK_MASK)
			&& ((regVal & (mipi_status_mask[lane_num - 1] <<
			ISP_MIPI_MIPI0_STATUS__MRV_MIPI_STOPSTATE__SHIFT))
			== (mipi_status_mask[lane_num - 1] <<
			ISP_MIPI_MIPI0_STATUS__MRV_MIPI_STOPSTATE__SHIFT))) {
				isp_dbg_print_warn
					("MIPI PHY 0 has been stopped\n");
				break;
			}

			usleep_range(1000, 2000);

			timeout = timeout + 1;
		}
		if (timeout == phy_reg_check_timeout) {
			isp_dbg_print_err
				("Timeout in shutting down MIPI PHY 0");
			return RET_FAILURE;
		}
		// step 2: shutdown MIPI PHY
		regVal = 0x0;
		MODIFYFLD(regVal, ISP_PHY0_CFG_REG0, shutdownz, 0x0);
		MODIFYFLD(regVal, ISP_PHY0_CFG_REG0, rstz, 0x0);
		aidt_hal_write_reg(mmISP_PHY0_CFG_REG0, regVal);

		// step 3: assert testclr signal
		regVal = 0x0;
		MODIFYFLD(regVal, ISP_PHY0_CFG_REG7, testclr, 0x1);
		aidt_hal_write_reg(mmISP_PHY0_CFG_REG7, regVal);
	} else if (device_id == 1) {
		// step 1: check the stop status
		while (timeout < phy_reg_check_timeout) {
			regVal = aidt_hal_read_reg(mmISP_MIPI_MIPI1_STATUS);
			if ((regVal &
			ISP_MIPI_MIPI1_STATUS__MRV_MIPI_S_STOPSTATE_CLK_MASK)
			&& ((regVal & (mipi_status_mask[lane_num - 1] <<
			ISP_MIPI_MIPI1_STATUS__MRV_MIPI_STOPSTATE__SHIFT))
			== (mipi_status_mask[lane_num - 1] <<
			ISP_MIPI_MIPI1_STATUS__MRV_MIPI_STOPSTATE__SHIFT))) {
				isp_dbg_print_warn
					("MIPI PHY 1 has been stopped\n");
				break;
			}

			usleep_range(1000, 2000);

			timeout = timeout + 1;
		}
		if (timeout == phy_reg_check_timeout) {
			isp_dbg_print_err
				("Timeout in shutting down MIPI PHY 1");
			return RET_FAILURE;
		}
		// step 2: shutdown MIPI PHY
		regVal = 0x0;
		MODIFYFLD(regVal, ISP_PHY1_CFG_REG0, shutdownz, 0x0);
		MODIFYFLD(regVal, ISP_PHY1_CFG_REG0, rstz, 0x0);
		aidt_hal_write_reg(mmISP_PHY1_CFG_REG0, regVal);

		// step 3: assert testclr signal
		regVal = 0x0;
		MODIFYFLD(regVal, ISP_PHY0_CFG_REG7, testclr, 0x1);
		aidt_hal_write_reg(mmISP_PHY1_CFG_REG7, regVal);
	}

	return RET_SUCCESS;
}

long IspMapValueFromRange(long srcLow, long srcHigh, long srcValue, long desLow,
			long desHigh)
{
	long srcRange = srcHigh - srcLow;
	long desRange = desHigh - desLow;

	if (srcRange == 0)
		return (desLow + desHigh) / 2;

	return srcValue * desRange * 100 / srcRange / 100;
}

/*
 *	0: disable the dumping
 *	bit0: 1 means to dump preview of rear camera;
 *	bit1: 1 means to dump video of rear camera;
 *	bit2: 1 means to dump zsl of rear camera;
 *	bit3: 1 means to dump raw of rear camera;
 *	bit8: 1 means to dump preview of front camera;
 *	bit9: 1 means to dump video of front camera;
 *	bit10: 1 means to dump zsl of front camera;
 *	bit11: 1 means to dump raw of front camera;
 *	bit16: 1 means to dump the metadata
 */

#define DUMP_PREVIEW_PIN_MASK	1
#define DUMP_VIDEO_PIN_MASK	2
#define DUMP_STILL_PIN_MASK	4
#define DUMP_RAW_PIN_MASK	8
#define DUMP_META_DATA_MASK	(1<<16)

void dump_pin_outputs_to_file(struct isp_context *isp,
				enum camera_id cid,
				struct frame_done_cb_para *cb,
				MetaInfo_t *meta, uint32 dump_flag)
{
	uint8 ctl_flag = 0;

	if (cid == CAMERA_ID_REAR)
		ctl_flag = (uint8) (dump_flag & 0xff);
	else if (cid == CAMERA_ID_FRONT_LEFT)
		ctl_flag = (uint8) ((dump_flag >> 8) & 0xff);
	else {
		isp_dbg_print_info
			("-><- %s fail bad cid %u\n", __func__, cid);
		return;
	}

	isp = isp;
	cid = cid;
	cb = cb;
	meta = meta;
}

bool roi_window_isvalid(Window_t *wnd)
{
	if (wnd->h_size != 0 && wnd->v_size != 0)
		return true;
	return false;
};

void isp_init_loopback_camera_info(struct isp_context *isp, enum camera_id cid)
{
	struct sensor_info *sif;
	enum camera_type lb_cam_type = CAMERA_TYPE_MEM;
	char *lb_cam_name = "mem";

	if (!is_para_legal(isp, cid)) {
		isp_dbg_print_err("-><- %s,fail for cid %u\n", __func__, cid);
		return;
	};
	sif = &isp->sensor_info[cid];
	sif->cur_res_fps_id = 0;
	sif->res_fps.count = 1;
	sif->res_fps.res_fps[0].index = 0;
	sif->res_fps.res_fps[0].fps = 30;
	sif->res_fps.res_fps[0].none_hdr_support = 1;
	sif->res_fps.res_fps[0].hdr_support = 0;
	sif->res_fps.res_fps[0].width = 1920;
	sif->res_fps.res_fps[0].height = 1080;
	sif->res_fps.res_fps[0].valid = 1;
	sif->res_fps.res_fps[0].crop_wnd.h_offset = 0;
	sif->res_fps.res_fps[0].crop_wnd.v_offset = 0;
	sif->res_fps.res_fps[0].crop_wnd.h_size = sif->res_fps.res_fps[0].width;
	sif->res_fps.res_fps[0].crop_wnd.v_size =
	    sif->res_fps.res_fps[0].height;
	sif->raw_width = sif->res_fps.res_fps[0].width;
	sif->raw_height = sif->res_fps.res_fps[0].height;

	if (cid == CAMERA_ID_REAR) {
		strncpy(isp->drv_settings.rear_sensor,
			lb_cam_name, CAMERA_DEVICE_NAME_LEN);
		isp->drv_settings.rear_sensor_type = lb_cam_type;
//		strncpy(isp->drv_settings.rear_vcm,
//			lb_cam_name, CAMERA_DEVICE_NAME_LEN);
//		strncpy(isp->drv_settings.rear_flash,
//			lb_cam_name, CAMERA_DEVICE_NAME_LEN);
	} else if (cid == CAMERA_ID_FRONT_LEFT) {
		strncpy(isp->drv_settings.frontl_sensor, lb_cam_name,
			CAMERA_DEVICE_NAME_LEN);
		isp->drv_settings.frontl_sensor_type = lb_cam_type;
	} else if (cid == CAMERA_ID_FRONT_RIGHT) {
		strncpy(isp->drv_settings.frontr_sensor, lb_cam_name,
			CAMERA_DEVICE_NAME_LEN);
		isp->drv_settings.frontr_sensor_type = lb_cam_type;
	};

	isp_dbg_print_info("-><- %s suc\n", __func__);
	return;

};

void isp_uninit_loopback_camera_info(struct isp_context *isp,
					enum camera_id cid)
{
	struct sensor_info *sif;
	char lb_cam_type = -1;
	char *lb_cam_name = "";

	if (!is_para_legal(isp, cid)) {
		isp_dbg_print_err("-><- %s,fail for cid %u\n", __func__, cid);
		return;
	};
	sif = &isp->sensor_info[cid];

	if (cid == CAMERA_ID_REAR) {
		strncpy(isp->drv_settings.rear_sensor,
			lb_cam_name, CAMERA_DEVICE_NAME_LEN);
		isp->drv_settings.rear_sensor_type = lb_cam_type;
//		strncpy(isp->drv_settings.rear_vcm,
//			lb_cam_name, CAMERA_DEVICE_NAME_LEN);
//		strncpy(isp->drv_settings.rear_flash,
//			lb_cam_name, CAMERA_DEVICE_NAME_LEN);
	} else if (cid == CAMERA_ID_FRONT_LEFT) {
		strncpy(isp->drv_settings.frontl_sensor, lb_cam_name,
			CAMERA_DEVICE_NAME_LEN);
		isp->drv_settings.frontl_sensor_type = lb_cam_type;
	} else if (cid == CAMERA_ID_FRONT_RIGHT) {
		strncpy(isp->drv_settings.frontr_sensor, lb_cam_name,
			CAMERA_DEVICE_NAME_LEN);
		isp->drv_settings.frontr_sensor_type = lb_cam_type;
	};

	isp_dbg_print_info("-> %s, cid %u\n", __func__, cid);
	isp->snr_res_fps_info_got[cid] = 0;
	reg_snr_op_imp(isp, cid, isp->p_sensor_ops[cid]);
	isp_dbg_print_info("<- %s suc\n", __func__);

	return;

}

void isp_alloc_loopback_buf(struct isp_context *isp,
			    enum camera_id cam_id, uint32 raw_size)
{
	struct sensor_info *sif;

	sif = &isp->sensor_info[cam_id];
	if (sif->loopback_raw_buf
	&& (sif->loopback_raw_buf->mem_size != raw_size)) {
		struct isp_gpu_mem_info *infback = sif->loopback_raw_info;

		sif->loopback_raw_info = NULL;
		isp_free_loopback_buf(isp, cam_id);
		sif->loopback_raw_info = infback;
	}

	if (sif->loopback_raw_buf == NULL) {
		sif->loopback_raw_buf =
			isp_gpu_mem_alloc(raw_size, ISP_GPU_MEM_TYPE_NLFB);
	}

	if (sif->loopback_raw_info == NULL) {
		sif->loopback_raw_info = isp_gpu_mem_alloc
			(sizeof(Mode4FrameInfo_t), ISP_GPU_MEM_TYPE_NLFB);
	}

	isp_dbg_print_info("-><- %s suc,cid:%d\n", __func__, cam_id);
	return;

};

void isp_free_loopback_buf(struct isp_context *isp, enum camera_id cam_id)
{
	struct sensor_info *sif;

	sif = &isp->sensor_info[cam_id];

	if (sif->loopback_raw_buf) {
		isp_gpu_mem_free(sif->loopback_raw_buf);
		sif->loopback_raw_buf = NULL;
	};

	if (sif->loopback_raw_info) {
		isp_gpu_mem_free(sif->loopback_raw_info);
		sif->loopback_raw_info = NULL;
	};

	isp_dbg_print_info("-><- %s suc,cid:%d\n", __func__, cam_id);
};

result_t loopback_set_raw_info_2_fw(struct isp_context *isp,
				enum camera_id cam_id,
				struct isp_gpu_mem_info *raw,
				struct isp_gpu_mem_info *raw_info)
{
//	struct isp_mapped_buf_info *gen_img = NULL;
//	struct isp_mapped_buf_info *gen_img1 = NULL;
	CmdSendMode4FrameInfo_t *frame_info_cmd;
	enum fw_cmd_resp_stream_id stream;
	result_t result;
	int32 ret = RET_FAILURE;

	if (!is_para_legal(isp, cam_id)
	|| (raw == NULL)
	|| (raw->sys_addr == NULL)
	|| (raw_info == NULL)
	|| (raw_info->sys_addr == NULL)) {
		isp_dbg_print_err
			("-><- %s fail bad para,isp:%p,cid:%d\n",
			__func__, isp, cam_id);
		return RET_FAILURE;
	}

	if ((cam_id != CAMERA_ID_MEM)
	&& (isp->sensor_info[cam_id].cam_type) != CAMERA_TYPE_MEM) {
		isp_dbg_print_err
			("-><- %s,cid:%d fail not mem sensor\n",
			__func__, cam_id);
		return RET_FAILURE;
	};

	isp_mutex_lock(&isp->ops_mutex);
	isp_dbg_print_info("-> %s,cid %d,rgbir:inIR\n", __func__, cam_id);

	if (ISP_GET_STATUS(isp) < ISP_STATUS_FW_RUNNING) {
		isp_dbg_print_info
			("<- %s,fail fsm %d\n", __func__, ISP_GET_STATUS(isp));
		isp_mutex_unlock(&isp->ops_mutex);
		return ret;
	}

	frame_info_cmd = (CmdSendMode4FrameInfo_t *) raw_info->sys_addr;
	memset(&frame_info_cmd->frameInfo.rawBufferFrameInfo.buffer,
		0, sizeof(Buffer_t));
	frame_info_cmd->frameInfo.rawBufferFrameInfo.buffer.bufTags = 23;
	frame_info_cmd->frameInfo.rawBufferFrameInfo.buffer.bufSizeA =
		(uint32) raw->mem_size;
	frame_info_cmd->frameInfo.rawBufferFrameInfo.buffer.vmidSpace = 4;

	isp_split_addr64(raw->gpu_mc_addr,
	&frame_info_cmd->frameInfo.rawBufferFrameInfo.buffer.bufBaseALo,
	&frame_info_cmd->frameInfo.rawBufferFrameInfo.buffer.bufBaseAHi);

	stream = isp_get_stream_id_from_cid(isp, cam_id);
	result = isp_send_fw_cmd(isp, CMD_ID_SEND_MODE4_FRAME_INFO, stream,
			FW_CMD_PARA_TYPE_INDIRECT, frame_info_cmd,
			sizeof(CmdSendMode4FrameInfo_t));

	if (result != RET_SUCCESS) {
		isp_dbg_print_err("<-%s fail for send frame info\n", __func__);
		goto quit;
	};

	ret = RET_SUCCESS;

	isp_dbg_print_info("<- %s suc\n", __func__);
 quit:

	isp_mutex_unlock(&isp->ops_mutex);

	return ret;
};

struct isp_gpu_mem_info *isp_get_work_buf(struct isp_context *isp,
					enum camera_id cid,
					enum isp_work_buf_type buf_type,
					uint32 idx)
{
	struct isp_gpu_mem_info *ret_buf = NULL;
	static uint8 indir_fw_cmd_pkg_idx[CAMERA_ID_MAX] = { 0 };

	if (!is_para_legal(isp, cid)) {
		isp_dbg_print_err("-><- %s fail para cid %u\n", __func__, cid);
		return NULL;
	};
	switch (buf_type) {
	case ISP_WORK_BUF_TYPE_CALIB:
		ret_buf = isp->work_buf.sensor_work_buf[cid].calib_buf;
		break;
	case ISP_WORK_BUF_TYPE_STREAM_TMP:
		if (idx < STREAM_TMP_BUF_COUNT)
			ret_buf =
			isp->work_buf.sensor_work_buf[cid].stream_tmp_buf[idx];
		break;
	case ISP_WORK_BUF_TYPE_CMD_RESP:
		ret_buf = isp->work_buf.sensor_work_buf[cid].cmd_resp_buf;
		break;
	case ISP_WORK_BUF_TYPE_LB_RAW_DATA:
		ret_buf = isp->work_buf.sensor_work_buf[cid].loopback_raw_buf;
		break;
	case ISP_WORK_BUF_TYPE_LB_RAW_INFO:
		ret_buf = isp->work_buf.sensor_work_buf[cid].loopback_raw_info;
		break;
	case ISP_WORK_BUF_TYPE_TNR_TMP:
		ret_buf = isp->work_buf.sensor_work_buf[cid].tnr_tmp_buf;
		break;
	case ISP_WORK_BUF_TYPE_HDR_STATUS:
		ret_buf = isp->work_buf.sensor_work_buf[cid].hdr_stats_buf;
		break;
	case ISP_WORK_BUF_TYPE_IIC_TMP:
		ret_buf = isp->work_buf.sensor_work_buf[cid].iic_tmp_buf;
		break;
	case ISP_WORK_BUF_TYPE_STAGE2_TMP:
		ret_buf = isp->work_buf.sensor_work_buf[cid].stage2_tmp_buf;
		break;
	case ISP_WORK_BUF_TYPE_UV_TMP:
		ret_buf = isp->work_buf.uv_tmp_buf;
		break;
	case ISP_WORK_BUF_TYPE_INDIR_FW_CMD_PKG:
		ret_buf =
isp->work_buf.sensor_work_buf[cid].indir_fw_cmd_pkg[indir_fw_cmd_pkg_idx[cid]];

		indir_fw_cmd_pkg_idx[cid]++;
		if (indir_fw_cmd_pkg_idx[cid] >= INDIRECT_BUF_CNT)
			indir_fw_cmd_pkg_idx[cid] = 0;
		break;
	case ISP_WORK_BUF_TYPE_EMB_DATA:
		if (idx < EMB_DATA_BUF_NUM)
			ret_buf =
			isp->work_buf.sensor_work_buf[cid].emb_data_buf[idx];
		break;
	default:
		isp_dbg_print_err("%s fail buf type %u\n", __func__, buf_type);
		return NULL;

	};
	if (ret_buf) {
		isp_dbg_print_info
			("-><- %s suc,cid %u,%s(%u),idx %u,ret %p(%llu)\n",
			__func__, cid, isp_dbg_get_work_buf_str(buf_type),
			buf_type, idx, ret_buf, ret_buf->mem_size);
	} else {
		isp_dbg_print_err
			("-><- %s fail,cid %u,%s(%u),idx %u,ret %p\n",
			__func__, cid, isp_dbg_get_work_buf_str(buf_type),
			buf_type, idx, ret_buf);
	}
	return ret_buf;
};

void isp_alloc_work_buf(struct isp_context *isp)
{
	enum camera_id cid;
	uint32 i = 0;
	uint32 total = 0;
	uint32 size = 0;

	if (isp == NULL) {
		isp_dbg_print_err("-><- %s fail para\n", __func__);
		return;
	};

	for (cid = CAMERA_ID_REAR; cid < CAMERA_ID_FRONT_RIGHT; cid++) {
		size = CALIB_DATA_SIZE;
		total += size;
		isp->work_buf.sensor_work_buf[cid].calib_buf =
			isp_gpu_mem_alloc(size, ISP_GPU_MEM_TYPE_NLFB);
		for (i = 0; i < STREAM_TMP_BUF_COUNT; i++) {
			size = (cid == CAMERA_ID_REAR) ?
			REAR_TEMP_RAW_BUF_SIZE : FRONT_TEMP_RAW_BUF_SIZE;

			total += size;
			isp->work_buf.sensor_work_buf[cid].stream_tmp_buf[i] =
				isp_gpu_mem_alloc(size, ISP_GPU_MEM_TYPE_NLFB);
		}
		size = CMD_RESPONSE_BUF_SIZE;
		total += size;
		isp->work_buf.sensor_work_buf[cid].cmd_resp_buf =
			isp_gpu_mem_alloc(size, ISP_GPU_MEM_TYPE_NLFB);

		isp->work_buf.sensor_work_buf[cid].loopback_raw_buf =
	isp->work_buf.sensor_work_buf[CAMERA_ID_REAR].stream_tmp_buf[0];

		isp->work_buf.sensor_work_buf[cid].loopback_raw_info =
	isp->work_buf.sensor_work_buf[CAMERA_ID_REAR].stream_tmp_buf[1];

		size = (cid == CAMERA_ID_REAR) ?
			REAR_TEMP_RAW_BUF_SIZE : FRONT_TEMP_RAW_BUF_SIZE;
		total += size;
		isp->work_buf.sensor_work_buf[cid].tnr_tmp_buf =
			isp_gpu_mem_alloc(size, ISP_GPU_MEM_TYPE_NLFB);
		size = HDR_STATS_BUF_SIZE;
		total += size;
		isp->work_buf.sensor_work_buf[cid].hdr_stats_buf =
			isp_gpu_mem_alloc(size, ISP_GPU_MEM_TYPE_NLFB);
		size = IIC_CONTROL_BUF_SIZE;
		total += size;
		isp->work_buf.sensor_work_buf[cid].iic_tmp_buf =
			isp_gpu_mem_alloc(size, ISP_GPU_MEM_TYPE_NLFB);
		size = (cid == CAMERA_ID_REAR) ?
			REAR_TEMP_RAW_BUF_SIZE * 3 / 4 :
			FRONT_TEMP_RAW_BUF_SIZE * 3 / 4;
		total += size;
		isp->work_buf.sensor_work_buf[cid].stage2_tmp_buf =
			isp_gpu_mem_alloc(size, ISP_GPU_MEM_TYPE_NLFB);
		for (i = 0; i < INDIRECT_BUF_CNT; i++) {
			size = INDIRECT_BUF_SIZE;
			total += size;
			isp->work_buf.sensor_work_buf[cid].indir_fw_cmd_pkg[i]
			= isp_gpu_mem_alloc(size, ISP_GPU_MEM_TYPE_NLFB);
		}
		for (i = 0; i < EMB_DATA_BUF_NUM; i++) {
			size = EMB_DATA_BUF_SIZE;
			total += size;
			isp->work_buf.sensor_work_buf[cid].emb_data_buf[i] =
				isp_gpu_mem_alloc(size, ISP_GPU_MEM_TYPE_NLFB);
		}

	};
	size = (max(REAR_TEMP_RAW_BUF_SIZE, FRONT_TEMP_RAW_BUF_SIZE)) / 4;
	total += size;
	isp->work_buf.uv_tmp_buf =
		isp_gpu_mem_alloc(size, ISP_GPU_MEM_TYPE_NLFB);
	isp_dbg_print_info("-><- %s suc, %u allocated\n", __func__, total);
};

void isp_free_work_buf(struct isp_context *isp)
{
	enum camera_id cid;
	uint32 i = 0;

	if (isp == NULL) {
		isp_dbg_print_err("-><- %s fail para\n", __func__);
		return;
	};

	for (cid = CAMERA_ID_REAR; cid < CAMERA_ID_FRONT_RIGHT; cid++) {
		isp_gpu_mem_free(isp->work_buf.sensor_work_buf[cid].calib_buf);
		isp->work_buf.sensor_work_buf[cid].calib_buf = NULL;
		for (i = 0; i < STREAM_TMP_BUF_COUNT; i++) {
			isp_gpu_mem_free(
			isp->work_buf.sensor_work_buf[cid].stream_tmp_buf[i]);

			isp->work_buf.sensor_work_buf[cid].stream_tmp_buf[i] =
				NULL;
		}
		isp_gpu_mem_free
			(isp->work_buf.sensor_work_buf[cid].cmd_resp_buf);
		isp->work_buf.sensor_work_buf[cid].cmd_resp_buf = NULL;
		isp->work_buf.sensor_work_buf[cid].loopback_raw_buf = NULL;
		isp->work_buf.sensor_work_buf[cid].loopback_raw_info = NULL;
		isp_gpu_mem_free
			(isp->work_buf.sensor_work_buf[cid].tnr_tmp_buf);
		isp->work_buf.sensor_work_buf[cid].tnr_tmp_buf = NULL;
		isp_gpu_mem_free
			(isp->work_buf.sensor_work_buf[cid].hdr_stats_buf);
		isp->work_buf.sensor_work_buf[cid].hdr_stats_buf = NULL;
		isp_gpu_mem_free
			(isp->work_buf.sensor_work_buf[cid].iic_tmp_buf);
		isp->work_buf.sensor_work_buf[cid].iic_tmp_buf = NULL;
		isp_gpu_mem_free
			(isp->work_buf.sensor_work_buf[cid].stage2_tmp_buf);
		isp->work_buf.sensor_work_buf[cid].stage2_tmp_buf = NULL;
		for (i = 0; i < INDIRECT_BUF_CNT; i++) {
			isp_gpu_mem_free(
		isp->work_buf.sensor_work_buf[cid].indir_fw_cmd_pkg[i]);

			isp->work_buf.sensor_work_buf[cid].indir_fw_cmd_pkg[i]
				= NULL;
		};
		for (i = 0; i < EMB_DATA_BUF_NUM; i++) {
			isp_gpu_mem_free
			(isp->work_buf.sensor_work_buf[cid].emb_data_buf[i]);

			isp->work_buf.sensor_work_buf[cid].emb_data_buf[i] =
				NULL;
		}
	};
	isp_gpu_mem_free(isp->work_buf.uv_tmp_buf);
	isp->work_buf.uv_tmp_buf = NULL;
	isp_dbg_print_info("-><- %s suc\n", __func__);
};

result_t isp_get_rgbir_crop_window(struct isp_context *isp,
				enum camera_id cid, Window_t *crop_window)
{
	package_td_header *ph = NULL;

	memset(crop_window, 0, sizeof(Window_t));

	if (isp->sensor_info[cid].cam_type != CAMERA_TYPE_MEM) {
		isp_dbg_print_err
			("in %s fail, not a RGBIR camera\n", __func__);
		return RET_FAILURE;
	}

	ph = (package_td_header *) isp->calib_data[cid];

	if ((ph->crop_wnd_width == 0)
	|| (ph->crop_wnd_height == 0)) {
		isp_dbg_print_err
			("in %s fail, no valid ir corp wnd w:h %u:%u\n",
			__func__, ph->crop_wnd_width, ph->crop_wnd_height);
		return RET_FAILURE;
	}

	crop_window->h_size = ph->crop_wnd_width;
	crop_window->v_size = ph->crop_wnd_height;

	return RET_SUCCESS;
}

/**codes to support psp fw loading***/
char *isp_get_fw_name(uint32 major, uint32 minor, uint32 rev)
{
	char *name = NULL;
	uint32 i;

	uint32 count = ARRAY_SIZE(isp_hwip_to_fw_name);

	for (i = 0; i < count; i++) {
		if ((major == isp_hwip_to_fw_name[i].major)
		&& (minor == isp_hwip_to_fw_name[i].minor)
		&& (rev == isp_hwip_to_fw_name[i].rev)) {
			name = (char *)isp_hwip_to_fw_name[i].fw_name;
			break;
		}
	}
	if (name) {
		isp_dbg_print_info("-><- %s ret %s for %u.%u.%u\n",
			__func__, name, major, minor, rev);
	} else {
		isp_dbg_print_info("-><- %s fail for %u.%u.%u\n",
			__func__, major, minor, rev);
	}
	return name;

};

void isp_get_fw_version_from_psp_header(void *fw_buf,
					uint32 *has_psp_header,
					uint32 *fw_version)
{
	struct psp_fw_header *fw;

	if (fw_buf == NULL)
		return;
	fw = (struct psp_fw_header *)fw_buf;
	if ((fw->image_id[0] == FW_IMAGE_ID_0)
	&& (fw->image_id[1] == FW_IMAGE_ID_1)
	&& (fw->image_id[2] == FW_IMAGE_ID_2)
	&& (fw->image_id[3] == FW_IMAGE_ID_3)) {
		if (has_psp_header)
			*has_psp_header = 1;
		if (fw_version)
			*fw_version = fw->fw_version;
	} else {
		if (has_psp_header)
			*has_psp_header = 0;
	}

};

/*
 * Program DS4 bit for ISP in PCTL_MMHUB_DEEPSLEEP
 * Set it to busy (0x0) when FW is running
 * Set it to idle (0x1) in other cases
 */
int32 isp_program_mmhub_ds_reg(bool_t b_busy)
{
	/*
	 * Put ISP DS bit to busy
	 * 0x0 means that the IP is busy
	 * 0x1 means that the IP is idle
	 */
	const uint32 PCTL_MMHUB_DEEPSLEEP = 0x00068E04;
	const uint8 MMHUB_DS_ISP_BIT = 4;
	const uint8 MMHUB_DS_SETCLEAR_BIT = 31;
	uint32 reg_value;

	reg_value = 1U << MMHUB_DS_ISP_BIT;

	if (b_busy == TRUE)
		reg_value |= 1U << MMHUB_DS_SETCLEAR_BIT;
	else
		reg_value &= ~(1U << MMHUB_DS_SETCLEAR_BIT);

	isp_hw_reg_write32(PCTL_MMHUB_DEEPSLEEP, reg_value);
	return IMF_RET_SUCCESS;
}

#ifdef USING_KGD_CGS
void isp_split_addr64(uint64 addr, uint32 *lo, uint32 *hi)
{
	if (lo)
		*lo = (uint32)(addr & 0xffffffff);
	if (hi)
		*hi = (uint32)(addr >> 32);
};

uint64 isp_join_addr64(uint32 lo, uint32 hi)
{
	return (((uint64)hi) << 32) | (uint64)lo;
}

uint32 isp_get_cmd_pl_size(void)
{
	uint32 ret = 1024;

	ret = MAX(ret, sizeof(MetaInfo_t));
	ret = MAX(ret, sizeof(Mode3FrameInfo_t));
	ret = MAX(ret, sizeof(CmdSetSensorProp_t));
	ret = MAX(ret, sizeof(CmdSetSensorHdrProp_t));
	ret = MAX(ret, sizeof(CmdSetDeviceScript_t));
	ret = MAX(ret, sizeof(CmdAwbSetLscMatrix_t));
	ret = MAX(ret, sizeof(CmdAwbSetLscSector_t));
	ret = MAX(ret, sizeof(CmdFrameControl_t));
	ret = MAX(ret, sizeof(CmdCaptureYuv_t));
	ret = ISP_ADDR_ALIGN_UP(ret, ISP_FW_CMD_PAY_LOAD_BUF_ALIGN);
	return ret;
};
#endif

