/*
 * Copyright (C) 2019-2023 Advanced Micro Devices, Inc. All rights reserved.
 */

#include "isp_common.h"
#include "buffer_mgr.h"
#include "isp_fw_if.h"
#include "log.h"
#include "isp_soc_adpt.h"
#include "isp_module_if_imp.h"

#include "amdgpu_ih.h"
#include "amdgpu_drm.h"

#include "phy.h"
#include "hal.h"
#include "isp_fpga.h"

#include <linux/sched.h>
#include <linux/mm.h>
#include <asm/pgtable.h>

#include <asm/page.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/videodev2.h>

#include <linux/init.h>
#include <linux/dma-mapping.h>
#include <linux/uaccess.h>

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "[ISP][isp_common]"

struct isp_context g_isp_context;
EXPORT_SYMBOL(g_isp_context);
struct swisp_services g_swisp_svr;
EXPORT_SYMBOL(g_swisp_svr);
u64 g_gpu_addr;
void *g_cpu_addr;
struct s_properties *g_prop;
int g_done_isp_pwroff_aft_boot;
enum mode_profile_name g_profile_id[CAM_IDX_MAX];
struct s_info g_snr_info[CAM_IDX_MAX];
struct _M2MTdb_t g_otp[CAM_IDX_MAX];

#define ALIGN_UP(addr, align)  (((addr) + ((align)-1)) & ~((align)-1))

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
unsigned int g_rgbir_raw_count_in_fw;
struct _AeIso_t ISO_MAX_VALUE;
struct _AeIso_t ISO_MIN_VALUE;
struct _AeEv_t EV_MAX_VALUE;
struct _AeEv_t EV_MIN_VALUE;
struct _AeEv_t EV_STEP_VALUE;
unsigned int g_num_i2c;

#define aidt_hal_write_reg(addr, value) \
	isp_hw_reg_write32(((unsigned int)(addr)), value)
#define aidt_hal_read_reg(addr) isp_hw_reg_read32(((unsigned int)(addr)))
#define MODIFYFLD(var, reg, field, val) \
	(var = (var & (~reg##__##field##_MASK)) | (((unsigned int)(val) << \
	reg##__##field##__SHIFT) & reg##__##field##_MASK))

#if	defined(ISP3_SILICON)
enum isp_environment_setting g_isp_env_setting = ISP_ENV_SILICON;
#else
enum isp_environment_setting g_isp_env_setting = ISP_ENV_MAXIMUS;
#endif

enum _SensorId_t get_isp_sensor_id_for_sensor_id(enum camera_id cam_id)
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

enum pixel_color_format map_cfa_pattern_fmt(enum _CFAPattern_t fmt)
{
	switch (fmt) {
	case CFA_PATTERN_RGGB:
		return PIXEL_COLOR_FORMAT_RGGB;
	case CFA_PATTERN_GRBG:
		return PIXEL_COLOR_FORMAT_GRBG;
	case CFA_PATTERN_GBRG:
		return PIXEL_COLOR_FORMAT_GBRG;
	case CFA_PATTERN_BGGR:
		return PIXEL_COLOR_FORMAT_BGGR;
	case CFA_PATTERN_PURE_IR:
		return PIXEL_COLOR_FORMAT_PURE_IR;
	case CFA_PATTERN_RIGB:
		return PIXEL_COLOR_FORMAT_RIGB;
	case CFA_PATTERN_RGIB:
		return PIXEL_COLOR_FORMAT_RGIB;
	case CFA_PATTERN_IRBG:
		return PIXEL_COLOR_FORMAT_IRBG;
	case CFA_PATTERN_GRBI:
		return PIXEL_COLOR_FORMAT_GRBI;
	case CFA_PATTERN_IBRG:
		return PIXEL_COLOR_FORMAT_IBRG;
	case CFA_PATTERN_GBRI:
		return PIXEL_COLOR_FORMAT_GBRI;
	case CFA_PATTERN_BIGR:
		return PIXEL_COLOR_FORMAT_BIGR;
	case CFA_PATTERN_BGIR:
		return PIXEL_COLOR_FORMAT_BGIR;
	case CFA_PATTERN_BGRGGIGI:
		return PIXEL_COLOR_FORMAT_BGRGGIGI;
	case CFA_PATTERN_RGBGGIGI:
		return PIXEL_COLOR_FORMAT_RGBGGIGI;
	default:
		return PIXEL_COLOR_FORMAT_INVALID;
	}
}

enum _CFAPattern_t unmap_cfa_pattern_fmt(enum pixel_color_format fmt)
{
	switch (fmt) {
	case PIXEL_COLOR_FORMAT_RGGB:
		return CFA_PATTERN_RGGB;
	case PIXEL_COLOR_FORMAT_GRBG:
		return CFA_PATTERN_GRBG;
	case PIXEL_COLOR_FORMAT_GBRG:
		return CFA_PATTERN_GBRG;
	case PIXEL_COLOR_FORMAT_BGGR:
		return CFA_PATTERN_BGGR;
	case PIXEL_COLOR_FORMAT_PURE_IR:
		return CFA_PATTERN_PURE_IR;
	case PIXEL_COLOR_FORMAT_RIGB:
		return CFA_PATTERN_RIGB;
	case PIXEL_COLOR_FORMAT_RGIB:
		return CFA_PATTERN_RGIB;
	case PIXEL_COLOR_FORMAT_IRBG:
		return CFA_PATTERN_IRBG;
	case PIXEL_COLOR_FORMAT_GRBI:
		return CFA_PATTERN_GRBI;
	case PIXEL_COLOR_FORMAT_IBRG:
		return CFA_PATTERN_IBRG;
	case PIXEL_COLOR_FORMAT_GBRI:
		return CFA_PATTERN_GBRI;
	case PIXEL_COLOR_FORMAT_BIGR:
		return CFA_PATTERN_BIGR;
	case PIXEL_COLOR_FORMAT_BGIR:
		return CFA_PATTERN_BGIR;
	case PIXEL_COLOR_FORMAT_BGRGGIGI:
		return CFA_PATTERN_BGRGGIGI;
	case PIXEL_COLOR_FORMAT_RGBGGIGI:
		return CFA_PATTERN_RGBGGIGI;
	default:
		return CFA_PATTERN_INVALID;
	}
}

enum pvt_img_fmt map_raw_fmt(enum _MipiDataType_t fmt)
{
	switch (fmt) {
	case MIPI_DATA_TYPE_RAW_8:
		return PVT_IMG_FMT_RAW8;
	case MIPI_DATA_TYPE_RAW_10:
		return PVT_IMG_FMT_RAW10;
	case MIPI_DATA_TYPE_RAW_12:
		return PVT_IMG_FMT_RAW12;
	case MIPI_DATA_TYPE_RAW_14:
		return PVT_IMG_FMT_RAW14;
	default:
		return PVT_IMG_FMT_INVALID;
	}
}

enum _MipiDataType_t unmap_raw_fmt(enum pvt_img_fmt fmt)
{
	switch (fmt) {
	case PVT_IMG_FMT_RAW8:
		return MIPI_DATA_TYPE_RAW_8;
	case PVT_IMG_FMT_RAW10:
		return MIPI_DATA_TYPE_RAW_10;
	case PVT_IMG_FMT_RAW12:
		return MIPI_DATA_TYPE_RAW_12;
	case PVT_IMG_FMT_RAW14:
		return MIPI_DATA_TYPE_RAW_14;
	default:
		return MIPI_DATA_TYPE_NULL;
	}
}

int get_bit_from_raw_fmt(enum pvt_img_fmt fmt)
{
	switch (fmt) {
	case PVT_IMG_FMT_RAW8:
		return 8;
	case PVT_IMG_FMT_RAW10:
		return 10;
	case PVT_IMG_FMT_RAW12:
		return 12;
	default:
		return 0;
	}
}

enum bin_mode get_bin_from_mode(enum mode_profile_name id)
{
	switch (id) {
	case PROFILE_NOHDR_2X2BINNING_3MP_15:
	case PROFILE_NOHDR_2X2BINNING_3MP_30:
	case PROFILE_NOHDR_2X2BINNING_3MP_60:
		return BINNING_MODE_2x2;
	case PROFILE_NOHDR_V2BINNING_6MP_30:
	case PROFILE_NOHDR_V2BINNING_6MP_60:
		return BINNING_MODE_2_VERTICAL;
	case PROFILE_NOHDR_NOBINNING_12MP_30:
		return BINNING_MODE_NONE;
	default:
		return BINNING_MODE_NONE;
	}
}

enum sensor_ae_prop_type map_ae_type(enum _SensorAePropType_t type)
{
	switch (type) {
	case SENSOR_AE_PROP_TYPE_INVALID:
		return AE_PROP_TYPE_INVALID;
	case SENSOR_AE_PROP_TYPE_S0NY:
		return AE_PROP_TYPE_SONY;
	case SENSOR_AE_PROP_TYPE_OV:
		return AE_PROP_TYPE_OV;
	case SENSOR_AE_PROP_TYPE_SCRIPT:
		return AE_PROP_TYPE_SCRIPT;
	default:
		return AE_PROP_TYPE_INVALID;
	}
}

enum _SensorAePropType_t unmap_ae_type(enum sensor_ae_prop_type type)
{
	switch (type) {
	case AE_PROP_TYPE_INVALID:
		return SENSOR_AE_PROP_TYPE_INVALID;
	case AE_PROP_TYPE_SONY:
		return SENSOR_AE_PROP_TYPE_S0NY;
	case AE_PROP_TYPE_OV:
		return SENSOR_AE_PROP_TYPE_OV;
	case AE_PROP_TYPE_SCRIPT:
		return SENSOR_AE_PROP_TYPE_SCRIPT;
	default:
		return SENSOR_AE_PROP_TYPE_INVALID;
	}
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
		str_info->format = PVT_IMG_FMT_NV12;
		/*FMT_4CC_YV12; */
		str_info->width = 0;
		/* 1920;// 320; // */
		str_info->height = 0;
		/* 1080;// 240; // */
		str_info->luma_pitch_set = 0;
		/*str_info->width; */
		str_info->chroma_pitch_set = 0;
		/*str_info->width / 2; */
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
	if (cid == CAMERA_ID_MEM)
		info->actual_cid = CAMERA_ID_FRONT_LEFT;
	else
		info->actual_cid = cid;

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
//	info->flash_mode = FLASH_MODE_AUTO;
//	info->flash_pwr = 80;
//	info->redeye_mode = RED_EYE_MODE_OFF;
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
	info->cur_res_fps_id = -1;
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
	info->tnr_enable = true;
	info->start_str_cmd_sent = false;
	info->stream_id = FW_CMD_RESP_STREAM_ID_MAX;
	//info->cproc_value = NULL;
	info->wb_idx = NULL;
	info->scene_tbl = NULL;
	info->fps_tab.fps = 0;
	info->fps_tab.HighValue = 0;
	info->fps_tab.LowValue = 0;
	info->fps_tab.HighAddr = (unsigned short) I2C_REGADDR_NULL;
	info->fps_tab.LowAddr = (unsigned short) I2C_REGADDR_NULL;
	info->enable_dynamic_frame_rate = TRUE;
	info->face_auth_mode = FALSE;
	info->sensor_opened = 0;
}

int isp_get_id_for_prf(enum camera_id cid, enum mode_profile_name prf_id)
{
	int id;

	for (id = 0; id < PROFILE_MAX; id++) {
		if (g_snr_info[cid].modes[id].mode_id == prf_id)
			break;
	}

	if (id == PROFILE_MAX) {
		ISP_PR_ERR("cam_%d cannot get id for prf_%d", cid, prf_id);
		id = 0;
	}

	return id;
}

int isp_get_caps_for_prf_id(struct isp_context *isp, enum camera_id cid,
			    enum mode_profile_name id)
{
	int i;
	int ret = RET_SUCCESS;
	struct profile_info *info;
	struct _isp_sensor_prop_t *caps;

	for (i = 0; i < PROFILE_MAX; i++) {
		info = &isp->prf_info[cid][i];
		if (info->id == id)
			break;
	}

	if (i == PROFILE_MAX) {
		ISP_PR_ERR("cam_%d cannot get the prf_%d info", cid, id);
		return RET_FAILURE;
	}

	caps = &isp->sensor_info[cid].sensor_cfg;
	caps->window.h_size = info->h_size;
	caps->window.v_size = info->v_size;
	caps->frame_rate = info->frame_rate;
	caps->prop.intfProp.mipi.numLanes = info->num_lanes;
	caps->prop.ae.timeOfLine = info->t_line;
	caps->prop.ae.initItime = info->init_itime;
	caps->prop.cfaPattern = info->cfa_fmt;
	caps->prop.intfProp.mipi.dataType = info->raw_fmt;
	caps->prop.ae.frameLengthLines = info->frame_length_lines;
	caps->hdr_valid = info->hdr_valid;
	caps->mipi_bitrate = info->mipi_bitrate;

	return ret;
}

void destroy_all_camera_info(struct isp_context *pisp_context)
{
#ifndef USING_KGD_CGS
	enum camera_id cam_id;
	struct sensor_info *info;
	struct isp_stream_info *str_info;
	unsigned int sid;

	for (cam_id = CAMERA_ID_REAR; cam_id < CAMERA_ID_MAX; cam_id++) {
		info = &pisp_context->sensor_info[cam_id];
		isp_list_destroy(&info->take_one_pic_buf_list, NULL);
		isp_list_destroy(&info->rgbir_raw_output_in_fw, NULL);
		isp_list_destroy(&info->rgbraw_input_in_fw, NULL);
		isp_list_destroy(&info->irraw_input_in_fw, NULL);
		isp_list_destroy(&info->rgbir_frameinfo_input_in_fw, NULL);
		info->take_one_pic_left_cnt = 0;
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
	unsigned int sid;

	for (cam_id = CAMERA_ID_REAR; cam_id < CAMERA_ID_MAX; cam_id++) {
		info = &pisp_context->sensor_info[cam_id];
		isp_reset_camera_info(pisp_context, cam_id);
		info->raw_width = 0;
		info->raw_height = 0;
		info->dump_raw_enable = 0;
		info->dump_raw_count = 0;
		info->per_frame_ctrl_cnt = 0;

		isp_event_init(&info->resume_success_evt, 1, 1);
		isp_list_init(&info->take_one_pic_buf_list);
		isp_list_init(&info->rgbir_raw_output_in_fw);
		isp_list_init(&info->rgbraw_input_in_fw);
		isp_list_init(&info->irraw_input_in_fw);
		isp_list_init(&info->rgbir_frameinfo_input_in_fw);
		info->take_one_pic_left_cnt = 0;
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
	strncpy(info->cam_name, "imx577", CAMERA_DEVICE_NAME_LEN);
//	strncpy(info->vcm_name, "ad5816", CAMERA_DEVICE_NAME_LEN);
//	strncpy(info->flash_name, "lm3646", CAMERA_DEVICE_NAME_LEN);
//	strncpy(info->storage_name, "ad5816", CAMERA_DEVICE_NAME_LEN);
	info->type = CAMERA_TYPE_RGB_BAYER;
	info->focus_len = 1;

	info = &isp->cam_dev_info[CAMERA_ID_REAR];
	strncpy(info->cam_name, "S5K3L6", CAMERA_DEVICE_NAME_LEN);
//	strncpy(info->vcm_name, "ad5816", CAMERA_DEVICE_NAME_LEN);
//	strncpy(info->flash_name, "lm3646", CAMERA_DEVICE_NAME_LEN);
//	strncpy(info->storage_name, "ad5816", CAMERA_DEVICE_NAME_LEN);
	info->type = CAMERA_TYPE_RGB_BAYER;
	info->focus_len = 1;

	info = &isp->cam_dev_info[CAMERA_ID_FRONT_LEFT];
	strncpy(info->cam_name, "imx208", CAMERA_DEVICE_NAME_LEN);
//	strncpy(info->vcm_name, "ad5816", CAMERA_DEVICE_NAME_LEN);
//	strncpy(info->flash_name, "lm3646", CAMERA_DEVICE_NAME_LEN);
//	strncpy(info->storage_name, "ad5816", CAMERA_DEVICE_NAME_LEN);
	info->type = CAMERA_TYPE_RGB_BAYER;
	info->focus_len = 1;

	info = &isp->cam_dev_info[CAMERA_ID_FRONT_RIGHT];
	strncpy(info->cam_name, "imx208", CAMERA_DEVICE_NAME_LEN);
//	strncpy(info->vcm_name, "ad5816", CAMERA_DEVICE_NAME_LEN);
//	strncpy(info->flash_name, "lm3646", CAMERA_DEVICE_NAME_LEN);
//	strncpy(info->storage_name, "ad5816", CAMERA_DEVICE_NAME_LEN);
	info->type = CAMERA_TYPE_RGB_BAYER;
	info->focus_len = 1;

//	info = &isp->cam_dev_info[CAMERA_ID_MEM];
//	strncpy(info->cam_name, "mem", CAMERA_DEVICE_NAME_LEN);
//	strncpy(info->vcm_name, "mem", CAMERA_DEVICE_NAME_LEN);
//	strncpy(info->flash_name, "mem", CAMERA_DEVICE_NAME_LEN);
//	strncpy(info->storage_name, "mem", CAMERA_DEVICE_NAME_LEN);
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
	isp->drv_settings.set_dgain_by_sensor_drv = -1;
	isp->drv_settings.set_lens_pos_by_sensor_drv = -1;
	isp->drv_settings.do_fpga_to_sys_mem_cpy = -1;
	isp->drv_settings.hdmi_support_enable = 0;
	isp->drv_settings.fw_log_cfg_en = 0;
	isp->drv_settings.fw_log_cfg_A = 0x33333333;
	isp->drv_settings.fw_log_cfg_B = 0x33333333;
	isp->drv_settings.fw_log_cfg_C = 0x33333333;
	isp->drv_settings.fw_log_cfg_D = 0x33333333;
	isp->drv_settings.fw_log_cfg_E = 0x33333333;
	isp->drv_settings.fw_log_cfg_F = 0x33333333;
	isp->drv_settings.fw_log_cfg_G = 0x33333333;
	isp->drv_settings.fix_wb_lighting_index = -1;
	isp->drv_settings.enable_stage2_temp_buf = 0;
	isp->drv_settings.ignore_meta_at_stage2 = 0;
	isp->drv_settings.driver_load_fw = 1;
	isp->drv_settings.disable_dynamic_aspect_wnd = 0;
	isp->drv_settings.tnr_enable = 1;
	isp->drv_settings.snr_enable = 1;
	isp->drv_settings.vfr_enable = 0;
	isp->drv_settings.low_light_fps = 3;
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
	unsigned int i;
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

int isp_start_resp_proc_threads(struct isp_context *isp)
{
	unsigned int i;
	int ret = RET_SUCCESS;

	ENTER();
	for (i = 0; i < MAX_REAL_FW_RESP_STREAM_NUM; i++) {
		isp_resp_para[i].idx = i;
		isp_resp_para[i].isp = isp;
		if (create_work_thread(&isp->fw_resp_thread[i],
		isp_fw_resp_thread_wrapper,
		&isp_resp_para[i]) != RET_SUCCESS) {
			ISP_PR_ERR("create_work_thread [%u] failed\n", i);
			ret = RET_FAILURE;
			goto fail;
		}
	}

	RET(ret);
	return ret;
fail:
	isp_stop_resp_proc_threads(isp);
	RET(ret);
	return ret;
};

int isp_stop_resp_proc_threads(struct isp_context *isp)
{
	unsigned int i;

	ENTER();
	for (i = 0; i < MAX_REAL_FW_RESP_STREAM_NUM; i++)
		stop_work_thread(&isp->fw_resp_thread[i]);

	RET(RET_SUCCESS);
	return RET_SUCCESS;
};

int isp_isr(void *private_data, unsigned int src_id, const uint32_t *iv_entry)
{
	struct isp_context *isp = (struct isp_context *)(private_data);
	unsigned long long irq_enable_id = 0;
	int idx = -1;

	ENTER();

	if (private_data == NULL) {
		ISP_PR_ERR("bad para, %p\n", private_data);
		return -1;
	};
	switch (src_id) {
	case IRQ_SOURCE_ISP_RINGBUFFER_WPT9:
		irq_enable_id = isp->irq_enable_id[0];
		idx = 0;
		break;
	case IRQ_SOURCE_ISP_RINGBUFFER_WPT10:
		irq_enable_id = isp->irq_enable_id[1];
		idx = 1;
		break;
	case IRQ_SOURCE_ISP_RINGBUFFER_WPT11:
		irq_enable_id = isp->irq_enable_id[2];
		idx = 2;
		break;
	case IRQ_SOURCE_ISP_RINGBUFFER_WPT12:
		irq_enable_id = isp->irq_enable_id[3];
		idx = 3;
		break;
	default:
		break;
	};
	if (-1 == idx) {
		ISP_PR_ERR("bad irq src %u\n", src_id);
		return -1;
	}

	isp_event_signal(0, &isp->fw_resp_thread[idx].wakeup_evt);

	return 0;
}

int isp_isr_set_callback(void *private_data, unsigned int src_id,
			 unsigned int type, int enabled)
{
	return RET_SUCCESS;
}

int isp_register_isr(struct isp_context *isp)
{
	int tempi = 0;
	enum cgs_result cgs_ret = CGS_RESULT__ERROR_GENERIC;
	unsigned int client_id = AMDGPU_IH_CLIENTID_ISP;

	if (!g_cgs_srv->os_ops->add_irq_source || !g_cgs_srv->os_ops->irq_get ||
	    !g_cgs_srv->os_ops->irq_put) {
		ISP_PR_ERR("failed no func\n");
		return RET_FAILURE;
	}
	for (tempi = 0; tempi < MAX_REAL_FW_RESP_STREAM_NUM; tempi++) {
		cgs_ret = g_cgs_srv->os_ops->add_irq_source(g_cgs_srv,
			client_id, g_irq_src[tempi], 2,
			isp_isr_set_callback, isp_isr, isp);
		if (cgs_ret != CGS_RESULT__OK) {
			ISP_PR_ERR("reg ISP_RINGBUFFER_WPT%u fail %u\n",
				   tempi + 9, cgs_ret);
			return RET_FAILURE;
		}

		ISP_PR_DBG("reg ISP_RINGBUFFER_WPT%u suc\n", tempi + 9);
		cgs_ret = g_cgs_srv->os_ops->irq_get(g_cgs_srv, client_id,
						     g_irq_src[tempi], 1);

		if (cgs_ret != CGS_RESULT__OK) {
			ISP_PR_ERR("put ISP_RINGBUFFER_WPT%u fail %u\n",
				   tempi + 9, cgs_ret);
			return RET_FAILURE;
		}

		ISP_PR_DBG("put ISP_RINGBUFFER_WPT%u suc\n", tempi + 9);
		isp->irq_enable_id[tempi] = 0;
	}
	return RET_SUCCESS;
};


int isp_unregister_isr(struct isp_context *isp_context)
{
	enum cgs_result cgs_ret;
	unsigned int tempi;
	unsigned int client_id = AMDGPU_IH_CLIENTID_ISP;

	if (g_cgs_srv->os_ops->irq_put == NULL) {
		ISP_PR_ERR("irq_put not supported\n");
		return RET_FAILURE;
	};

	for (tempi = 0; tempi < MAX_REAL_FW_RESP_STREAM_NUM; tempi++) {
		cgs_ret = g_cgs_srv->os_ops->irq_put
		(g_cgs_srv, client_id, g_irq_src[tempi], 1);
		isp_context->irq_enable_id[tempi] = 0;
		if (cgs_ret != CGS_RESULT__OK) {
			ISP_PR_ERR("fail unreg ISP_RINGBUFFER_WPT%u %u\n",
				   tempi + 9, cgs_ret);
		}

		ISP_PR_DBG("suc unreg ISP_RINGBUFFER_WPT%u\n", tempi + 9);
	};
	return RET_SUCCESS;
}
unsigned int isp_fb_get_emb_data_size(void)
{
	return EMB_DATA_BUF_SIZE;
}

int ispm_sw_init(struct isp_context *isp_info, unsigned int init_flag,
		 void *gart_range_hdl)
{
	enum camera_id cam_id;
	int ret;
	unsigned int rsv_fb_size;

	unsigned long long rsv_fw_work_buf_sys;
	unsigned long long rsv_fw_work_buf_mc;

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

	ISP_PR_INFO("sizeof(struct _CmdSetSensorProp_t) %lu\n",
		    sizeof(struct _CmdSetSensorProp_t));
	ISP_PR_INFO("sizeof(struct _CmdCaptureYuv_t) %lu\n",
		    sizeof(struct _CmdCaptureYuv_t));
	ISP_PR_INFO("sizeof(struct _CmdSetDeviceScript_t) %lu\n",
		    sizeof(struct _CmdSetDeviceScript_t));
	ISP_PR_INFO("sizeof(struct _FrameInfo_t) %lu\n",
		    sizeof(struct _FrameInfo_t));
	ISP_PR_INFO("sizeof(struct psp_fw_header) %lu\n",
		    sizeof(struct psp_fw_header));

	rsv_fb_size = ISP_PREALLOC_FB_SIZE;

	/*
	 *In case of running in KMD, the init_flag is not initilized
	 *But in AVStream, it will be initialized to a magic value as 0x446e6942
	 */
	if (init_flag != 0x446e6942) {
		lfb_resv_imp(isp_info->sw_isp_context, rsv_fb_size, NULL, NULL);
		ISP_PR_DBG("resv fw buf %uM\n", (rsv_fb_size) / (1024 * 1024));
		goto quit;
	};


	ret = lfb_resv_imp(isp_info->sw_isp_context, rsv_fb_size,
			   &rsv_fw_work_buf_sys, &rsv_fw_work_buf_mc);

	if (ret != RET_SUCCESS) {
		ISP_PR_ERR("lfb_resv %uM fail", rsv_fb_size / (1024 * 1024));
		goto fail;
	};


	isp_info->fw_work_buf_hdl =
		isp_fw_work_buf_init(rsv_fw_work_buf_sys, rsv_fw_work_buf_mc);

	if (isp_info->fw_work_buf_hdl == NULL) {
		ISP_PR_ERR("isp_fw_work_buf_init failed\n");
		goto fail;
	}

	ISP_PR_DBG("isp_fw_work_buf_init suc\n");
	isp_register_isr(isp_info);

quit:
	ISP_SET_STATUS(isp_info, ISP_STATUS_INITED);
	ISP_PR_INFO("suc\n");
	return RET_SUCCESS;
fail:
	ispm_sw_uninit(isp_info);
	ISP_PR_INFO("failed\n");
	return RET_FAILURE;
}

int ispm_sw_uninit(struct isp_context *isp_context)
{
	ENTER();

	isp_unregister_isr(isp_context);

	isp_list_destroy(&isp_context->take_photo_seq_buf_list, NULL);
	isp_list_destroy(&isp_context->work_item_list, NULL);

//	destroy_all_camera_info(isp_context);
	isp_clear_cmdq(isp_context);

	if (isp_context == isp_context->sw_isp_context) {
		ISP_PR_DBG("free resv fw buf\n");
		lfb_free_imp(isp_context);
	} else {
		ISP_PR_DBG("no need free resv fw buf\n");
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

	RET(RET_SUCCESS);
	return RET_SUCCESS;
}

void unit_isp(void)
{
	struct sensor_info *sensor_info;
	unsigned int i = 0;
	int result;
	struct isp_context *isp_context = &g_isp_context;

	i = 0;
	result = RET_SUCCESS;
	sensor_info = NULL;
	isp_fw_stop_all_stream(isp_context);

	for (i = 0; i < isp_context->sensor_count; i++) {
		sensor_info = &isp_context->sensor_info[i];
		result = isp_snr_pwr_set(isp_context, i, false);
		if (result != RET_SUCCESS)
			ISP_PR_ERR("pwr down sensor[%d] failed\n", i);
	};

	isp_ip_pwr_off(isp_context);

	//isp_hw_ccpu_rst(isp_context);
	ispm_sw_uninit(isp_context);
}


unsigned int isp_cal_buf_len_by_para(enum pvt_img_fmt fmt, unsigned int width,
				     unsigned int height, unsigned int l_pitch,
				     unsigned int c_pitch, unsigned int *y_len,
				     unsigned int *u_len, unsigned int *v_len)
{
	unsigned int channel_a_width;
	unsigned int channel_a_height;
	unsigned int channel_a_stride;

	unsigned int channel_b_width;
	unsigned int channel_b_height;
	unsigned int channel_b_stride;

	unsigned int channel_c_width;
	unsigned int channel_c_height;
	unsigned int channel_c_stride;

	unsigned int buffer_size;

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
		ISP_PR_ERR("unknown fmt %d\n", fmt);
		return 0;
	}

	channel_a_stride = l_pitch;
	if (width > l_pitch) {
		ISP_PR_ERR("bad l_pitch %d\n", l_pitch);
		return 0;
	};
	if (channel_b_height) {
		channel_b_stride = c_pitch;
		if (!c_pitch) {
			ISP_PR_ERR("bad cpitch %d\n", c_pitch);
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

int isp_get_pic_buf_param(struct isp_stream_info *str_info,
			  struct isp_picture_buffer_param *buffer_param)
{
	unsigned int width = 0;
	unsigned int height = 0;
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
		ISP_PR_ERR("unsupported picture color format:%d\n",
			str_info->format);
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

enum _ImageFormat_t isp_trans_to_fw_img_fmt(enum pvt_img_fmt format)
{
	enum _ImageFormat_t fmt;

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

enum _RawPktFmt_t isp_trans_to_fw_raw_pkt_fmt(enum cvip_raw_pkt_fmt format)
{
	enum _RawPktFmt_t fmt;

	switch (format) {
	case CVIP_RAW_PKT_FMT_0:
		fmt = RAW_PKT_FMT_0;
		break;
	case CVIP_RAW_PKT_FMT_1:
		fmt = RAW_PKT_FMT_1;
		break;
	case CVIP_RAW_PKT_FMT_2:
		fmt = RAW_PKT_FMT_2;
		break;
	case CVIP_RAW_PKT_FMT_3:
		fmt = RAW_PKT_FMT_3;
		break;
	case CVIP_RAW_PKT_FMT_4:
		fmt = RAW_PKT_FMT_4;
		break;
	case CVIP_RAW_PKT_FMT_5:
		fmt = RAW_PKT_FMT_5;
		break;
	case CVIP_RAW_PKT_FMT_6:
		fmt = RAW_PKT_FMT_6;
		break;
	default:
		fmt = RAW_PKT_FMT_4;
		break;
	}
	return fmt;
}

bool isp_get_str_out_prop(struct isp_stream_info *str_info,
				struct _ImageProp_t *out_prop)
{
	unsigned int width = 0;
	unsigned int height = 0;
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
		ISP_PR_ERR("unsupported color format:%d", str_info->format);
		return 0;
	}

	return 1;
}

unsigned int g_now;
long long g_last_fw_response_time_tick = { 0 };


void isp_idle_detect(struct isp_context *isp)
{
	enum camera_id cam_id;
	struct isp_pwr_unit *pwr_unit;
	int there_is_snr_on = false;

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

/*refer to aidt_api_stall_ccpu*/
void isp_hw_ccpu_stall(struct isp_context __maybe_unused *isp_context)
{
	unsigned int r1 = 0;
	unsigned int r2 = 0;
	unsigned int r3 = 0;
	unsigned int r4 = 0;
	unsigned int  wait_cnt = 100;

	r1 = isp_hw_reg_read32(mmISP_CCPU_CNTL);
	r1 |= ISP_CCPU_CNTL__CCPU_STALL_MASK;
	isp_hw_reg_write32(mmISP_CCPU_CNTL, r1);
	usleep_range(1000, 1100);
	while (wait_cnt)	{
		r1 = isp_hw_reg_read32(mmISP_STATUS);
		if ((r1 & ISP_STATUS__CCPU_RD_AXI_BUS_CLEAN_MASK) &&
		    (r1 & ISP_STATUS__CCPU_WR_AXI_BUS_CLEAN_MASK) &&
		    (r1 & ISP_STATUS__IDMA_RD_AXI_BUS_CLEAN_MASK) &&
		    (r1 & ISP_STATUS__IDMA_WR_AXI_BUS_CLEAN_MASK)) {
			break;
		}
		usleep_range(10000, 11000);
		wait_cnt--;
	}
	r2 = isp_hw_reg_read32(mmISP_CCPU_CNTL);
	r3 = isp_hw_reg_read32(mmISP_CCPU_PDEBUG_STATUS);
	r4 = isp_hw_reg_read32(mmISP_CCPU_PDEBUG_PC);
	if (wait_cnt) {
		ISP_PR_PC("Ccpu bus clean,STATUS %x,CNTL %x,dSt %x,PdPc %x\n",
			  r1, r2, r3, r4);
	} else {
		ISP_PR_ERR("Ccpu clean fail,STATUS %x,CNTL %x,dSt %x,PdPc %x\n",
			   r1, r2, r3, r4);
	}
}

/*refer to aidt_api_disable_ccpu*/
#define ISP_SW_RESET_MASK (ISP_SOFT_RESET__IMC_CC_SOFT_RESET_MASK |\
	ISP_SOFT_RESET__IMC_INBUF_SOFT_RESET_MASK |\
	ISP_SOFT_RESET__IMC_OUTBUF_SOFT_RESET_MASK)

void isp_hw_ccpu_disable(struct isp_context __maybe_unused *isp_context)
{
	unsigned int regVal;

	ENTER();

	regVal = isp_hw_reg_read32(mmISP_CCPU_CNTL);
	regVal |= ISP_CCPU_CNTL__CCPU_HOST_SOFT_RST_MASK;

	isp_hw_reg_write32(mmISP_CCPU_CNTL, regVal);
	usleep_range(1000, 1100);

	//double check register value, will remove it later.
	regVal = isp_hw_reg_read32(mmISP_CCPU_CNTL);
	ISP_PR_DBG("mmISP_CCPU_CNTL[%d]", regVal);

	regVal = isp_hw_reg_read32(mmISP_SOFT_RESET);
	regVal |= ISP_SOFT_RESET__CCPU_SOFT_RESET_MASK;
	regVal |= ISP_SW_RESET_MASK;

	isp_hw_reg_write32(mmISP_SOFT_RESET, regVal); // disable CCPU/
	usleep_range(1000, 1100);

	//double check the register value, will remove it later.
	regVal = isp_hw_reg_read32(mmISP_SOFT_RESET);
	ISP_PR_DBG("mmISP_CCPU_RESET[%d]", regVal);

	EXIT();
}

/*refer to aidt_api_enable_ccpu*/
void isp_hw_ccpu_enable(struct isp_context __maybe_unused *isp_context)
{
	unsigned int regVal;

	ENTER();

	regVal = isp_hw_reg_read32(mmISP_SOFT_RESET);
	regVal &= (~ISP_SOFT_RESET__CCPU_SOFT_RESET_MASK);
	regVal &= (~ISP_SW_RESET_MASK);
	isp_hw_reg_write32(mmISP_SOFT_RESET, regVal); //bus reset
	usleep_range(1000, 1100);
	regVal = isp_hw_reg_read32(mmISP_CCPU_CNTL);
	regVal &= (~ISP_CCPU_CNTL__CCPU_HOST_SOFT_RST_MASK);
	regVal &= (~ISP_CCPU_CNTL__CCPU_STALL_MASK);
	isp_hw_reg_write32(mmISP_CCPU_CNTL, regVal);
	usleep_range(1000, 1100);

	EXIT();
}


void isp_hw_ccpu_rst(struct isp_context __maybe_unused *isp_context)
{
	unsigned int reg_val;

	ENTER();

	reg_val = isp_hw_reg_read32(mmISP_SOFT_RESET);
	reg_val |= ISP_SOFT_RESET__CCPU_SOFT_RESET_MASK;
	isp_hw_reg_write32(mmISP_SOFT_RESET, reg_val);

	EXIT();
}

void isp_init_fw_rb_log_buffer(struct isp_context __maybe_unused *isp_context,
	unsigned int fw_rb_log_base_lo,
	unsigned int fw_rb_log_base_hi,
	unsigned int fw_rb_log_size)
{
	int enable = true;

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

int isp_snr_open(struct isp_context *isp_context, enum camera_id cam_id,
		 int res_fps_id, unsigned int flag)
{
	int result;
	struct sensor_info *sensor_info;
	struct isp_pwr_unit *pwr_unit;
	unsigned int index;

	ENTER();
	if ((isp_context == NULL) || (cam_id >= CAMERA_ID_MAX)) {
		ISP_PR_ERR("fail for illegal para %d\n", cam_id);
		return RET_FAILURE;
	};

	sensor_info = &isp_context->sensor_info[cam_id];
	pwr_unit = &isp_context->isp_pu_cam[cam_id];

	if (pwr_unit->pwr_status == ISP_PWR_UNIT_STATUS_ON) {
		if ((res_fps_id == sensor_info->cur_res_fps_id) &&
		    (flag == sensor_info->open_flag)) {
			ISP_PR_INFO("suc, do none\n");
			return RET_SUCCESS;
		}

		if (g_prop->cvip_enable == CVIP_STATUS_ENABLE)
			result = RET_SUCCESS;
		else
			result = isp_snr_stop(isp_context, cam_id);

		if (result != RET_SUCCESS) {
			ISP_PR_ERR("isp_snr_stop failed, pwr off it\n");
			goto fail;
		}
	}

	ISP_PR_INFO("cid %d,res_fps_id %d,flag 0x%x", cam_id, res_fps_id, flag);

	isp_mutex_lock(&pwr_unit->pwr_status_mutex);
	result = isp_snr_pwr_set(isp_context, cam_id, true);
	if (result != RET_SUCCESS) {
		ISP_PR_ERR("isp_snr_pwr_set failed\n");
		goto fail_unlock;
	}

	isp_context->isp_pu_cam[cam_id].pwr_status = ISP_PWR_UNIT_STATUS_ON;
	result = isp_snr_cfg(isp_context, cam_id, res_fps_id, flag);
	if (result != RET_SUCCESS) {
		ISP_PR_ERR("isp_snr_cfg failed\n");
		goto fail_unlock;
	}

	result = isp_get_caps_for_prf_id(isp_context, cam_id,
					 g_profile_id[cam_id]);
	if (result != RET_SUCCESS) {
		ISP_PR_ERR("cam_%d failed", cam_id);
		goto fail_unlock;
	};

	if (sensor_info->cam_type == CAMERA_TYPE_RGBIR ||
	    sensor_info->cam_type == CAMERA_TYPE_RGBIR_HWDIR) {
		isp_context->sensor_info[CAMERA_ID_MEM].sensor_cfg =
			sensor_info->sensor_cfg;
		isp_context->sensor_info[CAMERA_ID_MEM].res_fps =
			sensor_info->res_fps;
	}

	if (g_prop->cvip_enable == CVIP_STATUS_DISABLE &&
	    g_isp_env_setting == ISP_ENV_SILICON &&
	    sensor_info->sensor_cfg.prop.intfType == SENSOR_INTF_TYPE_MIPI) {
		ISP_PR_DBG("cid %u,bitrate %u,laneNum %u\n",
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
		ISP_PR_ERR("get fralenline regaddr failed\n");

	result = isp_snr_start(isp_context, cam_id);
	if (result != RET_SUCCESS) {
		ISP_PR_ERR("isp_snr_start failed\n");
		goto fail_unlock;
	};

	/*
	 *if (isp_context->fw_ctrl_3a) {
	 *	result = isp_set_script_to_fw(isp_context, cam_id);
	 *	if (result != RET_SUCCESS) {
	 *		ISP_PR_ERR("isp_set_script_to_fw fail\n");
	 *		isp_context->fw_ctrl_3a = 0;
	 *		goto fail_unlock;
	 *	};
	 *}
	 */

	sensor_info->cur_res_fps_id = (char)res_fps_id;
	index = get_index_from_res_fps_id(isp_context, cam_id, res_fps_id);
	sensor_info->raw_height = sensor_info->res_fps.res_fps[index].height;
	sensor_info->raw_width = sensor_info->res_fps.res_fps[index].width;
	ISP_PR_PC("w:%d, h:%d", sensor_info->raw_width,
				sensor_info->raw_height);

	sensor_info->stream_inited = 0;
	sensor_info->open_flag = flag;
	sensor_info->hdr_enable = flag & OPEN_CAMERA_FLAG_HDR;

	if (sensor_info->cam_type == CAMERA_TYPE_RGBIR_HWDIR) {
		struct sensor_info *sensor_info_mem =
			&isp_context->sensor_info[CAMERA_ID_MEM];

		sensor_info_mem->cur_res_fps_id = sensor_info->cur_res_fps_id;
		sensor_info_mem->raw_height = sensor_info->raw_height / 2;
		sensor_info_mem->raw_width = sensor_info->raw_width / 2;
		sensor_info_mem->open_flag = sensor_info->open_flag;
		sensor_info_mem->hdr_enable = sensor_info->hdr_enable;
	}
	isp_mutex_unlock(&pwr_unit->pwr_status_mutex);

	ISP_PR_PC("cam[%d] open suc!", cam_id);
	return RET_SUCCESS;

 fail_unlock:
	isp_snr_pwr_set(isp_context, cam_id, false);
	isp_context->isp_pu_cam[cam_id].pwr_status = ISP_PWR_UNIT_STATUS_OFF;
	isp_mutex_unlock(&pwr_unit->pwr_status_mutex);
 fail:
	RET(RET_FAILURE);
	return RET_FAILURE;
};

int isp_snr_close(struct isp_context *isp, enum camera_id cid)
{
	struct sensor_info *sensor_info;
	struct isp_pwr_unit *pwr_unit;

	ENTER();
	ISP_PR_INFO("cid %d\n", cid);

	if (!is_para_legal(isp, cid)) {
		ISP_PR_ERR("fail for illegal para %d\n", cid);
		return RET_FAILURE;
	};

	sensor_info = &isp->sensor_info[cid];
	pwr_unit = &isp->isp_pu_cam[cid];

	if (pwr_unit->pwr_status == ISP_PWR_UNIT_STATUS_OFF) {
		ISP_PR_INFO("suc, do none\n");
		return RET_SUCCESS;
	};

	isp_mutex_lock(&pwr_unit->pwr_status_mutex);

	if (g_prop->cvip_enable == CVIP_STATUS_DISABLE) {
		if (g_isp_env_setting == ISP_ENV_SILICON &&
		    sensor_info->sensor_cfg.prop.intfType ==
		     SENSOR_INTF_TYPE_MIPI) {
			ISP_PR_INFO("cid %u, laneNum %u", cid,
			sensor_info->sensor_cfg.prop.intfProp.mipi.numLanes);
			shutdown_internal_synopsys_phy(cid,
			sensor_info->sensor_cfg.prop.intfProp.mipi.numLanes);
		}
		isp_snr_stop(isp, cid);
	}

	isp_snr_pwr_set(isp, cid, false);
	sensor_info->cur_res_fps_id = -1;
	isp_mutex_unlock(&pwr_unit->pwr_status_mutex);

	ISP_PR_PC("cam[%d] close suc!", cid);
	return RET_SUCCESS;
}

void isp_acpi_set_sensor_pwr(struct isp_context *isp,
				enum camera_id cid, bool on)
{
	uint32_t acpi_method;
	uint32_t acpi_function;
//	uint32_t input_arg = 0;
//	enum cgs_result result;

	if (isp_is_fpga()) {
		ISP_PR_DBG("suc, cid %u, on:%u\n", cid, on);
		return;
	}

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
//comment this out since the acpi interfaces not supported yet.
//	result = g_cgs_srv->ops->call_acpi_method(g_cgs_srv, acpi_method,
//				acpi_function, &input_arg, NULL, 0, 1, 0);
//	if (result == CGS_RESULT__OK) {
//		ISP_PR_DBG("suc cid %u,on %u\n",
//			cid, on);
//	} else {
//		ISP_PR_ERR("failed with %x cid %u,on %u\n",
//			result, cid, on);
//	}
}

//refer to reset_sensor_phy
int isp_snr_pwr_toggle(struct isp_context *isp, enum camera_id cid,
				bool on)
{
	uint32_t lane_num;

	if (!is_para_legal(isp, cid)) {
		ISP_PR_ERR("bad para cid %u, on:%u\n", cid, on);
		return RET_FAILURE;
	}

	lane_num = isp->sensor_info[cid].sensor_cfg.prop.intfProp.mipi.numLanes;
	ISP_PR_DBG("toggle get lane num: %d\n", lane_num);
	//reset_sensor_phy(cid, on, isp_is_fpga(), lane_num);
	ISP_PR_INFO("ISP bus is ready!\n");
	if (!isp_is_fpga())
		isp_acpi_set_sensor_pwr(isp, cid, on);

	ISP_PR_INFO("cam_module_pwr_on, suc\n");

	return RET_SUCCESS;
}

int isp_snr_clk_toggle(struct isp_context *isp, enum camera_id cid,
				bool on)
{	//todo
	unsigned int reg;

	if (isp_is_fpga()) {
		ISP_PR_DBG("for fpga cid %u, on:%u\n", cid, on);
		return RET_SUCCESS;
	};
	if (!is_para_legal(isp, cid)) {
		ISP_PR_ERR("bad para cid %u, on:%u\n", cid, on);
		return RET_FAILURE;
	};
#if	defined(ISP3_SILICON)
	reg = 0x3ff;
	isp_hw_reg_write32(mmISP_CLOCK_GATING_A, reg);
	ISP_PR_DBG("write ISP_CLOCK_GATING_A(0x%x): 0x%x\n",
		mmISP_CLOCK_GATING_A, reg);
#else
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
#endif
	ISP_PR_PC("cam[%u] set clk %s suc\n", cid, (on == 1) ? "on" : "off");
	return RET_SUCCESS;
}


int is_para_legal(void *context, enum camera_id cam_id)
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

void isp_init_sensor_info(enum camera_id cid)
{
	if (g_prop->sensor_id == SENSOR_S5K3L6) {
		strcpy(g_snr_info[cid].sensor_name, "S5K3L6");
		strcpy(g_snr_info[cid].sensor_manufacturer, "SAMSUNG");
		strcpy(g_snr_info[cid].module_part_number, "OHA2388");
		/* OHA2388 PD A0 V1.2 20200819.pdf */
		g_snr_info[cid].apertures[0] = 2.0;
		g_snr_info[cid].focal_lengths[0] = 3.55;
		/* Tuning_Requirements p32 */
		g_snr_info[cid].hyperfocal_distance = 0.125;
		g_snr_info[cid].min_focus_distance = 5;
		g_snr_info[cid].min_increment = 0.125;
		g_snr_info[cid].physical_size_w = 4730;
		/* Approval Sheet_REV0.02 p29 */
		g_snr_info[cid].physical_size_h = 3512;
		g_snr_info[cid].black_level_pattern[0] = 0xFF;
		g_snr_info[cid].black_level_pattern[1] = 0xFF;
		g_snr_info[cid].black_level_pattern[2] = 0xFF;
		g_snr_info[cid].black_level_pattern[3] = 0xFF;
		g_snr_info[cid].test_pattern_modes_cnt = 4;
		g_snr_info[cid].test_pattern_modes[0] = TEST_PATTERN_MODE_OFF;
		g_snr_info[cid].test_pattern_modes[1] =
			TEST_PATTERN_MODE_SOLID_COLOR;
		g_snr_info[cid].test_pattern_modes[2] =
			TEST_PATTERN_MODE_COLOR_BARS;
		g_snr_info[cid].test_pattern_modes[3] =
			TEST_PATTERN_MODE_COLOR_BARS_FADE_TO_GRAY;
	} else {
		strcpy(g_snr_info[cid].sensor_name, "IMX577");
		strcpy(g_snr_info[cid].sensor_manufacturer, "S0NY");
		strcpy(g_snr_info[cid].module_part_number, "0000");
		g_snr_info[cid].apertures[0] = 0;
		g_snr_info[cid].focal_lengths[0] = 0;
		g_snr_info[cid].hyperfocal_distance = 0;
		g_snr_info[cid].min_focus_distance = 0;
		g_snr_info[cid].min_increment = 0;
		g_snr_info[cid].physical_size_w = 6287;
		g_snr_info[cid].physical_size_h = 4715;
		g_snr_info[cid].black_level_pattern[0] = 0x101;
		g_snr_info[cid].black_level_pattern[1] = 0x101;
		g_snr_info[cid].black_level_pattern[2] = 0x101;
		g_snr_info[cid].black_level_pattern[3] = 0x101;
		g_snr_info[cid].test_pattern_modes_cnt = 2;
		g_snr_info[cid].test_pattern_modes[0] = TEST_PATTERN_MODE_OFF;
		g_snr_info[cid].test_pattern_modes[1] =
			TEST_PATTERN_MODE_COLOR_BARS;
	}

	strcpy(g_snr_info[cid].module_manufacture, "O-Film");
	/* ML CHANGE MMF-3873 */
	g_snr_info[cid].s_position = SENSOR_POSITION_REAR;
	g_snr_info[cid].m_ori = MODULE_ORIENTATION_0;
	g_snr_info[cid].pre_embedded_size = sizeof(struct _PreData_t);
	g_snr_info[cid].post_embedded_size = sizeof(struct _PostData_t);
	g_snr_info[cid].otp_data_size = 0;
	g_snr_info[cid].otp_buffer = NULL;
	g_snr_info[cid].filter_densities[0] = 0;
	g_snr_info[cid].optical_stabilization[0] =
		OPTICAL_STABILIZATION_MODE_OFF;
	g_snr_info[cid].optical_black_regions_cnt = 0;
	g_snr_info[cid].shared_again = 0;
}

void isp_update_caps_from_sinfo(struct isp_context *isp, enum camera_id cid)
{
	int ret = RET_SUCCESS;
	int i;
	unsigned int iso2gain;
	struct mode_info *prf;
	struct profile_info *info;
	struct _isp_sensor_prop_t *caps;

	caps = &isp->sensor_info[cid].sensor_cfg;
	for (i = 0; i < g_snr_info[cid].modes_cnt; i++) {
		prf = &g_snr_info[cid].modes[i];
		ret = isp_snr_get_caps(isp, cid, prf->mode_id, caps);
		if (ret != RET_SUCCESS)
			break;

		info = &isp->prf_info[cid][i];
		info->id = prf->mode_id;
		info->h_size = prf->w;
		info->v_size = prf->h;
		info->frame_rate = prf->min_frame_rate *
					SENSOR_FPS_ACCUACY_FACTOR;
		info->t_line = prf->t_line;
		info->init_itime = prf->init_itime;
		info->frame_length_lines = prf->frame_length_lines;
		info->cfa_fmt = unmap_cfa_pattern_fmt(prf->fmt_layout);
		info->raw_fmt = unmap_raw_fmt(prf->fmt_bit);
		info->hdr_valid = (prf->hdr == HDR_MODE_INVALID) ? 0 : 1;
		info->num_lanes = caps->prop.intfProp.mipi.numLanes;
		info->mipi_bitrate = caps->mipi_bitrate;
	}

	iso2gain = GAIN_ACCURACY_FACTOR / g_snr_info[cid].base_iso;
	caps->prop.ae.type = unmap_ae_type(g_snr_info[cid].ae_type);
	caps->prop.ae.minExpoLine = g_snr_info[cid].min_expo_line;
	caps->prop.ae.maxExpoLine = g_snr_info[cid].max_expo_line;
	caps->prop.ae.expoLineAlpha = g_snr_info[cid].expo_line_alpha;
	caps->prop.ae.minAnalogGain = g_snr_info[cid].min_again * iso2gain;
	caps->prop.ae.maxAnalogGain = g_snr_info[cid].max_again * iso2gain;
	caps->prop.ae.sharedAgain = (g_snr_info[cid].shared_again != 0);
	caps->prop.ae.initAgain = g_snr_info[cid].init_again * iso2gain;
	caps->exposure.min_digi_gain = g_snr_info[cid].min_dgain * iso2gain;
	caps->exposure.max_digi_gain = g_snr_info[cid].max_dgain * iso2gain;
	caps->prop.ae.formula.weight1 = g_snr_info[cid].formula.weight1;
	caps->prop.ae.formula.weight2 = g_snr_info[cid].formula.weight2;
	caps->prop.ae.formula.minShift = g_snr_info[cid].formula.min_shift;
	caps->prop.ae.formula.maxShift = g_snr_info[cid].formula.max_shift;
	caps->prop.ae.formula.minParam = g_snr_info[cid].formula.min_param;
	caps->prop.ae.formula.maxParam = g_snr_info[cid].formula.max_param;
	caps->prop.ae.baseIso = g_snr_info[cid].base_iso;
	caps->exposure_orig = caps->exposure;
	caps->prop.hasEmbeddedData = (g_snr_info[cid].pre_embedded_size > 0);
}

void isp_get_sinfo_from_caps(struct isp_context *isp, enum camera_id cid)
{
	int ret = RET_SUCCESS;
	int id = PROFILE_NOHDR_2X2BINNING_3MP_30, i = 0;
	unsigned int iso2gain;
	struct mode_info *prf;
	struct profile_info *info;
	struct _isp_sensor_prop_t *caps;

	caps = &isp->sensor_info[cid].sensor_cfg;
	isp_init_sensor_info(cid);
	for (; id < PROFILE_MAX; id++, i++) {
		ret = isp_snr_get_caps(isp, cid, id, caps);
		if (ret != RET_SUCCESS)
			break;

		if (!caps->prop.ae.timeOfLine) {
			ISP_PR_WARN("prf_%d not implemented, ignore", id);
			i--;
			continue;
		}

		info = &isp->prf_info[cid][i];
		info->id = id;
		info->h_size = caps->window.h_size;
		info->v_size = caps->window.v_size;
		info->frame_rate = caps->frame_rate;
		info->t_line = caps->prop.ae.timeOfLine;
		info->init_itime = caps->prop.ae.initItime;
		info->hdr_valid = caps->hdr_valid;
		info->num_lanes = caps->prop.intfProp.mipi.numLanes;
		info->cfa_fmt = caps->prop.cfaPattern;
		info->raw_fmt = caps->prop.intfProp.mipi.dataType;
		info->frame_length_lines = caps->prop.ae.frameLengthLines;
		info->mipi_bitrate = caps->mipi_bitrate;

		prf = &g_snr_info[cid].modes[i];
		prf->mode_id = id;
		prf->bin = get_bin_from_mode(id);
		prf->w = caps->window.h_size;
		prf->h = caps->window.v_size;
		prf->active_array_size_w = SIZE_ALIGN_DOWN(prf->w);
		prf->active_array_size_h = SIZE_ALIGN_DOWN(prf->h);
		prf->pixel_array_size_w = SIZE_ALIGN_DOWN(prf->w);
		prf->pixel_array_size_h = SIZE_ALIGN_DOWN(prf->h);
		prf->pre_correction_size_w = SIZE_ALIGN_DOWN(prf->w);
		prf->pre_correction_size_h = SIZE_ALIGN_DOWN(prf->h);
		prf->fmt_layout = map_cfa_pattern_fmt(caps->prop.cfaPattern);
		prf->fmt_bit = map_raw_fmt(caps->prop.intfProp.mipi.dataType);
		prf->bits_per_pixel = get_bit_from_raw_fmt(prf->fmt_bit);
		prf->t_line = caps->prop.ae.timeOfLine;
		prf->frame_length_lines = caps->prop.ae.frameLengthLines;
		prf->min_frame_rate = caps->frame_rate /
						SENSOR_FPS_ACCUACY_FACTOR;
		prf->max_frame_rate = prf->min_frame_rate;
		prf->init_itime = caps->prop.ae.initItime;
		prf->hdr = (caps->hdr_valid == 1) ?
					HDR_MODE_SME : HDR_MODE_INVALID;
	}

	iso2gain = GAIN_ACCURACY_FACTOR / caps->prop.ae.baseIso;
	g_snr_info[cid].modes_cnt = i;
	g_snr_info[cid].ae_type = map_ae_type(caps->prop.ae.type);
	g_snr_info[cid].min_expo_line = caps->prop.ae.minExpoLine;
	g_snr_info[cid].max_expo_line = caps->prop.ae.maxExpoLine;
	g_snr_info[cid].expo_line_alpha = caps->prop.ae.expoLineAlpha;
	g_snr_info[cid].min_again = caps->prop.ae.minAnalogGain / iso2gain;
	g_snr_info[cid].max_again = caps->prop.ae.maxAnalogGain / iso2gain;
	g_snr_info[cid].init_again = caps->prop.ae.initAgain / iso2gain;
	g_snr_info[cid].min_dgain = caps->exposure.min_digi_gain / iso2gain;
	g_snr_info[cid].max_dgain = caps->exposure.max_digi_gain / iso2gain;
	g_snr_info[cid].formula.weight1 = caps->prop.ae.formula.weight1;
	g_snr_info[cid].formula.weight2 = caps->prop.ae.formula.weight2;
	g_snr_info[cid].formula.min_shift = caps->prop.ae.formula.minShift;
	g_snr_info[cid].formula.max_shift = caps->prop.ae.formula.maxShift;
	g_snr_info[cid].formula.min_param = caps->prop.ae.formula.minParam;
	g_snr_info[cid].formula.max_param = caps->prop.ae.formula.maxParam;
	g_snr_info[cid].base_iso = caps->prop.ae.baseIso;
}

int isp_get_caps(struct isp_context *isp, enum camera_id cid)
{
	int ret = RET_SUCCESS;

	ENTER();
	if (!is_para_legal(isp, cid)) {
		ISP_PR_ERR("bad para,isp:%p,cid:%d\n", isp, cid);
		return RET_FAILURE;
	}

	if (g_snr_info[cid].modes_cnt > 0)
		isp_update_caps_from_sinfo(isp, cid);
	else
		isp_get_sinfo_from_caps(isp, cid);

	return ret;
}

int isp_set_snr_info_2_fw(struct isp_context *isp, enum camera_id cid)
{
	int result;
	struct sensor_info *snr_info;
	struct _CmdSetSensorProp_t snr_prop;
	struct _CmdSetSensorEmbProp_t snr_embprop;
	enum camera_id actual_cid = cid;
	bool emb_support = 0;

	ENTER();
	if (!is_para_legal(isp, cid)) {
		ISP_PR_ERR("bad para,isp:%p,cid:%d\n", isp, cid);
		return RET_FAILURE;
	}

	if (cid == CAMERA_ID_MEM) {
		enum camera_id l_cid;

		for (l_cid = CAMERA_ID_REAR; l_cid < CAMERA_ID_FRONT_RIGHT;
		l_cid++) {
			if (CAMERA_TYPE_RGBIR_HWDIR ==
			isp->sensor_info[l_cid].cam_type) {
				actual_cid = l_cid;
				ISP_PR_DBG("change cid from %u to %u\n",
					cid, l_cid);
				break;
			};
		}
	};
	snr_info = &isp->sensor_info[actual_cid];

	result = isp_get_caps_for_prf_id(isp, actual_cid, g_profile_id[cid]);
	if (result != RET_SUCCESS) {
		ISP_PR_ERR("cam_%d failed", cid);
		return RET_FAILURE;
	};
	snr_info->lens_range.minLens = snr_info->sensor_cfg.lens_pos.min_pos;
	snr_info->lens_range.maxLens = snr_info->sensor_cfg.lens_pos.max_pos;
	snr_info->lens_range.stepLens = 1;

 //no_need_get_cap:
	snr_prop.sensorId = isp_get_fw_sensor_id(isp, actual_cid);
	memcpy(&snr_prop.sensorProp, &snr_info->sensor_cfg.prop,
	       sizeof(snr_prop.sensorProp));
	dbg_show_sensor_prop(&snr_prop);
	dbg_show_sensor_caps(&snr_info->sensor_cfg);

	if (snr_prop.sensorId == SENSOR_ID_INVALID) {
		ISP_PR_ERR("invalid sensor id\n");
		return RET_FAILURE;
	} else if (snr_prop.sensorId == SENSOR_ID_RDMA) {
		ISP_PR_INFO("no need set snr prop for mem\n");
	} else if (isp_fw_set_sensor_prop(isp, actual_cid, snr_prop) !=
		   RET_SUCCESS) {
		ISP_PR_ERR("set sensor prop fail,cid %d\n", snr_prop.sensorId);
		return RET_FAILURE;
	}

	//send frame rate info to fw
	if (isp_fw_set_frame_rate_info(isp, actual_cid,
		snr_info->sensor_cfg.frame_rate) != RET_SUCCESS) {
		ISP_PR_ERR("set frame rate fail,cid %d\n", cid);
		return RET_FAILURE;
	}

	if (snr_info->hdr_enable) {
		snr_info->hdr_prop_set_suc = 0;
		if (!snr_info->sensor_cfg.hdr_valid) {
			ISP_PR_ERR("hdr not supported\n");
		} else {
			struct _CmdSetSensorHdrProp_t hdr_prop;

			hdr_prop.sensorId = cid;
			memcpy(&hdr_prop.hdrProp, &snr_info->sensor_cfg.hdrProp,
				sizeof(hdr_prop.hdrProp));
			if (isp_fw_set_hdr_prop(isp, actual_cid,
					hdr_prop) != RET_SUCCESS) {
				//add this line to trick CP
				ISP_PR_ERR("set hdr prop failed\n");
			} else {
				snr_info->hdr_prop_set_suc = 1;
				ISP_PR_INFO("set hdr prop suc\n");
			}
		};
	} else {
		ISP_PR_INFO("hdr not enabled\n");
	}

	snr_embprop.sensorId = isp_get_fw_sensor_id(isp, actual_cid);
	if (isp_snr_get_emb_prop(isp, actual_cid, &emb_support,
				&snr_embprop.embProp) != RET_SUCCESS) {
		ISP_PR_ERR("isp_snr_get_emb_prop failed\n");
	} else {
		if (!emb_support) {
			//add this line to trick CP
			ISP_PR_INFO("emb not supported\n");
		} else if (isp_fw_set_emb_prop(isp, actual_cid,
					snr_embprop) != RET_SUCCESS) {
			ISP_PR_ERR("set emb prop fail,cid %d\n",
				snr_embprop.sensorId);
			return RET_FAILURE;
		}

		ISP_PR_INFO("set emb prop suc\n");
	}

	if (isp->fw_ctrl_3a) {
		result = isp_set_script_to_fw(isp, actual_cid);
		if (result != RET_SUCCESS) {
			ISP_PR_ERR("failed to set script\n");
			return RET_FAILURE;
		}

		ISP_PR_INFO("set fw script suc\n");
	} else {
		ISP_PR_INFO("fw_crl_3a not supported\n");
	};
	isp->snr_info_set_2_fw = 1;

	RET(RET_SUCCESS);
	return RET_SUCCESS;
};

int isp_set_calib_2_fw(struct isp_context *isp, enum camera_id cid)
{
	struct sensor_info *snr_info;
	struct calib_date_item *item;
	struct _CmdSetSensorCalibdb_t calib;
	struct isp_gpu_mem_info *gpu_mem_info;
	int tmpi;

	ENTER();
	if (!is_para_legal(isp, cid)) {
		ISP_PR_ERR("fail, bad para,isp:%p,cid:%d\n",
			isp, cid);
		return RET_FAILURE;
	};

	snr_info = &isp->sensor_info[cid];
	snr_info->calib_set_suc = 0;

	for (tmpi = 0; tmpi < MAX_CALIB_NUM_IN_FW; tmpi++) {
		item = isp->calib_items[cid][tmpi];
		if (!item)
			break;
		if (snr_info->calib_gpu_mem[tmpi]) {
			isp_gpu_mem_free(snr_info->calib_gpu_mem[tmpi]);
			snr_info->calib_gpu_mem[tmpi] = NULL;
		};
		gpu_mem_info = isp_gpu_mem_alloc(item->len,
						 ISP_GPU_MEM_TYPE_NLFB);

		snr_info->calib_gpu_mem[tmpi] = gpu_mem_info;

		if (!gpu_mem_info) {
			ISP_PR_WARN("isp_gpu_mem_alloc failed, ignore\n");
			break;
		};
		ISP_PR_DBG("calib %p(%u),bef memcpy\n", item->data, item->len);
		memcpy(gpu_mem_info->sys_addr, item->data, item->len);
		calib.streamId = isp_get_fw_stream_from_drv_stream
					(isp_get_stream_id_from_cid(isp, cid));
		calib.bufSize = item->len;
		isp_split_addr64(gpu_mem_info->gpu_mc_addr, &calib.bufAddrLo,
				 &calib.bufAddrHi);
		calib.checkSum = compute_check_sum(item->data, item->len);
		calib.tdbIdx = tmpi;
		calib.fps = item->fps;
		calib.width = item->width;
		calib.height = item->height;
		calib.expoLimit = (item->expo_limit == EXPO_LIMIT_TYPE_SHORT) ?
			SWITCH_LIMIT_SHORT : SWITCH_LIMIT_LONG;
		if (isp_fw_set_calib_data(isp, cid, calib) != RET_SUCCESS) {
			ISP_PR_ERR("cid:%d,set calib failed,ignore\n",
				   cid);
			break;
		}
		ISP_PR_INFO("cid:%d,calib %u %ux%d@%u,expo %s\n", cid,
			    calib.tdbIdx, calib.width, calib.height,
			    calib.fps,
			    calib.expoLimit == SWITCH_LIMIT_SHORT ? "shot"
			    : "long");
		snr_info->calib_set_suc = 1;
	}
	RET(RET_SUCCESS);
	return RET_SUCCESS;
}

/**
 * Read M2M data from sensor and set to device.
 *
 * @param pcontext isp context.
 * @param cid camera id from which the M2M data reads.
 * @return RET_SUCCESS for success and RET_FAILURE for fail
 */
int isp_set_m2m_calib_2_fw(struct isp_context *isp, enum camera_id cid)
{
	struct sensor_info *snr_info;
	struct _CmdParamPackage_t pkt;
	struct _CmdM2MConfig_t *cmd;
	struct isp_gpu_mem_info *gpu_mem_info;
	enum fw_cmd_resp_stream_id stream_id;
	int status = 0;
	unsigned int size = 0;
	void *otp = NULL;

	ENTER();
	if (!is_para_legal(isp, cid)) {
		ISP_PR_ERR("fail, bad para,isp:%p,cid:%d\n",
			isp, cid);
		return RET_FAILURE;
	};
	snr_info = &isp->sensor_info[cid];
	if (snr_info->m2m_calib_gpu_mem) {
		isp_gpu_mem_free(snr_info->m2m_calib_gpu_mem);
		snr_info->m2m_calib_gpu_mem = NULL;
	};

	gpu_mem_info = isp_gpu_mem_alloc(sizeof(struct _CmdM2MConfig_t),
					 ISP_GPU_MEM_TYPE_NLFB);
	snr_info->m2m_calib_gpu_mem = gpu_mem_info;
	if (gpu_mem_info == NULL) {
		ISP_PR_WARN("isp_gpu_mem_alloc failed, ignore\n");
		status = RET_FAILURE;
		goto fail;
	};
	cmd = (struct _CmdM2MConfig_t *)gpu_mem_info->sys_addr;
	if (cmd == NULL) {
		ISP_PR_WARN("isp_gpu_mem_alloc failed1, ignore\n");
		status = RET_FAILURE;
		goto fail;
	}

	if (g_prop->cvip_enable == CVIP_STATUS_ENABLE) {
		size = g_snr_info[cid].otp_data_size;
		otp = g_snr_info[cid].otp_buffer;
		if (size == 0 || !otp) {
			status = RET_NOTSUPP;
		} else if (size != sizeof(struct _M2MTdb_t)) {
			ISP_PR_ERR("cvip otp %p error size %d ", otp, size);
			status = RET_FAILURE;
		} else {
			memcpy(&cmd->m2mParams, &g_otp[cid], size);
			status = RET_SUCCESS;
		}
	} else {
		status = isp_snr_get_m2m_data(isp, cid, &cmd->m2mParams);
	}

	if (status == RET_SUCCESS) {
		ISP_PR_INFO("get m2m data success\n");
	} else if (status == RET_NOTSUPP) {
		ISP_PR_INFO("sensor doesn't support m2m\n");
		goto fail;
	} else {
		ISP_PR_ERR("get m2m data fail\n");
		goto fail;
	}
	pkt.packageSize = sizeof(struct _CmdM2MConfig_t);
	isp_split_addr64(gpu_mem_info->gpu_mc_addr, &pkt.packageAddrLo,
			 &pkt.packageAddrHi);
	pkt.packageCheckSum =
		compute_check_sum((unsigned char *)&cmd->m2mParams,
				  sizeof(struct _CmdM2MConfig_t));
	stream_id = isp_get_stream_id_from_cid(isp, cid);
	if (isp_send_fw_cmd(isp, CMD_ID_M2M_CONFIG,
				stream_id,
				FW_CMD_PARA_TYPE_DIRECT,
				&pkt, sizeof(struct _CmdParamPackage_t))
				!= RET_SUCCESS) {
		ISP_PR_ERR("fail set m2m calib data, ignore\n");
		status = RET_FAILURE;
		goto fail;
	} else {
		ISP_PR_INFO("set m2m calib data suc\n");
	};

	RET(RET_SUCCESS);
	return RET_SUCCESS;

fail:
	if (snr_info->m2m_calib_gpu_mem) {
		isp_gpu_mem_free(snr_info->m2m_calib_gpu_mem);
		snr_info->m2m_calib_gpu_mem = NULL;
	};
	return status;
}

enum fw_cmd_resp_stream_id isp_get_stream_id_from_cid(
		struct isp_context __maybe_unused *isp,
		enum camera_id cid)
{
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
		 */
		break;
	default:
		ISP_PR_ERR("Invalid cid[%d].", cid);
		return FW_CMD_RESP_STREAM_ID_MAX;
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
		if (isp->sensor_info[cid].cam_type == CAMERA_TYPE_RGBIR
			|| isp->sensor_info[cid].cam_type ==
			CAMERA_TYPE_RGBIR_HWDIR)
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
//refer to aidt_api_set_stream_path
int isp_set_stream_path(struct isp_context *isp, enum camera_id cid,
			enum fw_cmd_resp_stream_id stream_id)
{
	enum _StreamMode_t stream_mode;
	struct _CmdSetStreamPath_t stream_path_cmd;
	struct _StreamPathCfg_t *cfg;

	if (!is_para_legal(isp, cid)) {
		ISP_PR_ERR("bad para,cid:%d\n", cid);
		return RET_FAILURE;
	};
	memset(&stream_path_cmd, 0, sizeof(stream_path_cmd));

	stream_path_cmd.pathCfg.mipiPipePathCfg.sensorId =
		isp_get_fw_sensor_id(isp, cid);
	stream_path_cmd.pathCfg.mipiPipePathCfg.bEnable = true;
	if (g_prop->cvip_enable == CVIP_STATUS_ENABLE)
		stream_path_cmd.pathCfg.mipiPipePathCfg.bCvpCase = true;
	else
		stream_path_cmd.pathCfg.mipiPipePathCfg.bCvpCase = false;
	switch (isp->sensor_info[cid].cam_type) {
	case CAMERA_TYPE_RGB_BAYER:
		if (isp->drv_settings.process_3a_by_host) {
			stream_mode = STREAM_MODE_3;
			stream_path_cmd.pathCfg.mipiPipePathCfg.mipiPathOutType
				= MIPI_PATH_OUT_TYPE_DMABUF_TO_HOST;
		} else {
			stream_mode = STREAM_MODE_2;
			stream_path_cmd.pathCfg.mipiPipePathCfg.mipiPathOutType
				= MIPI_PATH_OUT_TYPE_DMABUF_TO_ISP;
		}
		break;
	case CAMERA_TYPE_IR:
	case CAMERA_TYPE_RGBIR_HWDIR:
		stream_mode = STREAM_MODE_2;
		stream_path_cmd.pathCfg.mipiPipePathCfg.mipiPathOutType =
			MIPI_PATH_OUT_TYPE_DMABUF_TO_ISP;
		break;
	case CAMERA_TYPE_RGBIR:
		stream_mode = STREAM_MODE_3;
		stream_path_cmd.pathCfg.mipiPipePathCfg.mipiPathOutType =
			MIPI_PATH_OUT_TYPE_DMABUF_TO_HOST;
		break;
	case CAMERA_TYPE_MEM:
		stream_mode = STREAM_MODE_4;
		stream_path_cmd.pathCfg.mipiPipePathCfg.bEnable = false;
		stream_path_cmd.pathCfg.mipiPipePathCfg.mipiPathOutType =
			MIPI_PATH_OUT_TYPE_INVALID;
		break;
	default:
		ISP_PR_ERR("cid[%d], bad cam type[%u]\n",
			cid, isp->sensor_info[cid].cam_type);
		return RET_FAILURE;
	}
	ISP_PR_DBG("cid[%u],stream[%d], STREAM_MODE_%d\n",
		cid, stream_id, stream_mode - 1);

	if (isp_fw_set_stream_path(isp, cid, stream_path_cmd) != RET_SUCCESS) {
		ISP_PR_ERR("set stream path failed\n");
		return RET_FAILURE;
	}
	cfg = &stream_path_cmd.pathCfg;

	ISP_PR_DBG(
		"enable %u,cvpcase %u,sensorId %u,mipioutpath %s\n",
		cfg->mipiPipePathCfg.bEnable,
		cfg->mipiPipePathCfg.bCvpCase,
		cfg->mipiPipePathCfg.sensorId,
		isp_dbg_get_mipi_out_path_str
			(cfg->mipiPipePathCfg.mipiPathOutType));
//	ISP_PR_DBG(
//		"prevEn %u,videoEn %u,stillEn %u,CvpCvEn %u,"
//		"X86CvEn %u,irEn %u,rawEn %u,fullstill %u,fullraw: %u\n",
//		cfg->ispPipePathCfg.bEnablePreview,
//		cfg->ispPipePathCfg.bEnableVideo,
//		cfg->ispPipePathCfg.bEnableStill,
//		cfg->ispPipePathCfg.bEnableCvpCv,
//		cfg->ispPipePathCfg.bEnableX86Cv,
//		cfg->ispPipePathCfg.bEnableIr,
//		cfg->ispPipePathCfg.bEnableRaw,
//		cfg->ispPipePathCfg.bEnableFullStill,
//		cfg->ispPipePathCfg.bEnableFullRaw);
	RET(RET_SUCCESS);
	return RET_SUCCESS;
}

int isp_setup_stream(struct isp_context *isp, enum camera_id cid)
{
	struct _CmdSetIRIlluConfig_t IR;
	enum fw_cmd_resp_stream_id stream_id;
	enum camera_id tmp_cid = cid;
	unsigned int frame_rate = 0;
	unsigned int one_frame_count;

	ENTER();
	if (!is_para_legal(isp, cid)) {
		ISP_PR_ERR("bad para,cid[%d]\n", cid);
		return RET_FAILURE;
	};

	stream_id = isp_get_stream_id_from_cid(isp, cid);
	if (isp_set_stream_path(isp, cid, stream_id) != RET_SUCCESS) {
		ISP_PR_ERR("set_stream_path failed\n");
		return RET_FAILURE;
	};

	if (g_prop->cvip_enable != CVIP_STATUS_ENABLE) {
		int status = 0;

		status = isp_set_m2m_calib_2_fw(isp, cid);
		if (status != RET_SUCCESS && status != RET_NOTSUPP) {
			//not fatal error, jut log it and continue
			ISP_PR_ERR("isp_set_m2m_calib_2_fw failed\n");
		};
	}
	//bdu comment it for its definition changed in new fw interface
	// King for OV7251 bring up
	if ((cid < CAMERA_ID_MEM)
	&& (isp->sensor_info[cid].cam_type != CAMERA_TYPE_MEM)
	&& (isp->sensor_info[cid].sensor_cfg.prop.cfaPattern) >=
	CFA_PATTERN_PURE_IR) {

// This value need to match hardware clk configration, current is 66MHz
#define PPI_CLK (100000000)
		unsigned int delay_ratio = 0;
		unsigned int duration_ratio = 0;
		struct sensor_hw_parameter para = { 0 };

		isp_snr_get_hw_parameter(isp, tmp_cid, &para);
		frame_rate = min(isp->sensor_info[cid].sensor_cfg.frame_rate,
			((unsigned int)isp->drv_settings.sensor_frame_rate)
			* 1000);
		IR.IRIlluConfig.mode = IR_ILLU_MODE_AUTO;
		if (isp->drv_settings.ir_illuminator_ctrl == 2)
			IR.IRIlluConfig.mode = IR_ILLU_MODE_AUTO;
		else if (isp->drv_settings.ir_illuminator_ctrl == 1)
			IR.IRIlluConfig.mode = IR_ILLU_MODE_ON;
		else
			IR.IRIlluConfig.mode = IR_ILLU_MODE_OFF;

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

		one_frame_count =
			PPI_CLK / (frame_rate / 1000);
		IR.IRIlluConfig.time =
			(unsigned int) (one_frame_count / 1000 *
			duration_ratio);
		IR.IRIlluConfig.delay =
			(unsigned int) (one_frame_count / 1000 * delay_ratio);

		ISP_PR_DBG("IRIlluConfig mode[%u],duration_ratio[%d]",
			IR.IRIlluConfig.mode, duration_ratio);
		ISP_PR_DBG("delay_ratio[%d]\n", delay_ratio);
	}

	// Get the crop window from tuning data for
	// IR steam in a RGBIR camera and set it to FW
	// The value comes from tuning team and SW needs
	// to calculate the window offset to make it fit to the center
	// This is for RGBIR camera only
	if (cid == CAMERA_ID_MEM) {
		struct _Window_t asprw;
		struct _Window_t irZoomWindow;

		if (!isp->calib_data[cid]) {
			ISP_PR_ERR("failed, no calib for ir\n");
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
			ISP_PR_ERR("ir crop too large [%u,%u]>[%u,%u]\n",
				irZoomWindow.h_size, irZoomWindow.v_size,
				asprw.h_size, asprw.v_size);
			goto quit_set_zoom;
		}

		irZoomWindow.h_offset =
			(asprw.h_size - irZoomWindow.h_size) / 2;
		irZoomWindow.v_offset =
			(asprw.v_size - irZoomWindow.v_size) / 2;

		if (isp_fw_set_zoom_window(isp, cid,
					irZoomWindow) != RET_SUCCESS) {
			ISP_PR_ERR("set window failed x:y:w:h %u:%u:%u:%u\n",
				irZoomWindow.h_offset, irZoomWindow.v_offset,
				irZoomWindow.h_size, irZoomWindow.v_size);
			return RET_FAILURE;
		}

		ISP_PR_DBG("IR Zoom Windows x:y:w:h %u:%u:%u:%u suc\n",
			irZoomWindow.h_offset, irZoomWindow.v_offset,
			irZoomWindow.h_size, irZoomWindow.v_size);

 quit_set_zoom:
		;
	}

	if (isp_fw_set_snr_enable(isp, cid,
			isp->drv_settings.snr_enable) != RET_SUCCESS) {
		ISP_PR_ERR("set snr enable(%d) fail\n",
			isp->drv_settings.snr_enable);
	} else {
		ISP_PR_INFO("set snr enable(%d) suc\n",
			isp->drv_settings.snr_enable);
	}

	RET(RET_SUCCESS);
	return RET_SUCCESS;
}

int isp_setup_metadata_buf(struct isp_context *isp, enum camera_id cid)
{
	struct sensor_info *snr_info;
	unsigned int i;
	unsigned int len[STREAM_META_BUF_COUNT] = { 0 };
	//struct _CmdSendBuffer_t buf_type;
	int ret;
	enum fw_cmd_resp_stream_id stream_id;

	if (!is_para_legal(isp, cid)) {
		ISP_PR_ERR("bad para,cid[%d]\n", cid);
		return RET_FAILURE;
	}
	stream_id = isp_get_stream_id_from_cid(isp, cid);
	snr_info = &isp->sensor_info[cid];

	for (i = 0; i < STREAM_META_BUF_COUNT; i++) {
		ret = isp_fw_get_nxt_cmd_pl(isp->fw_work_buf_hdl, NULL,
					&snr_info->meta_mc[i], &len[i]);
		if (ret != RET_SUCCESS) {
			ISP_PR_ERR("cid[%d],failed to aloc buf(%u)\n", cid, i);
			goto fail;
		}
	}

	ISP_PR_INFO("cid[%u], cnt[%u] suc\n", cid, STREAM_META_BUF_COUNT);
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

int isp_setup_tnr_ref_buf(struct isp_context *isp, enum camera_id cid,
			  enum fw_cmd_resp_stream_id stream_id,
			  struct sensor_info *snr_info)
{
	int ret;
	char disable_tnr = 0;
	unsigned int i, id;
	unsigned int width;
	unsigned int height;
	unsigned int size;
	unsigned int size_with_gap;
	struct _CmdSendBuffer_t buf_type;

	if (snr_info->cam_type == CAMERA_TYPE_MEM) {
		ISP_PR_INFO("suc, do none for mem sensor");
		return RET_SUCCESS;
	}

	ISP_PR_DBG("cid[%u] Tnr En %u", cid, isp->drv_settings.tnr_enable);
	if (g_prop->cvip_enable == CVIP_STATUS_ENABLE) {
		disable_tnr = 0;
		if (!disable_tnr) {
		//for cvip case, TNR ref buf is in FMR
		//so no need to set ref buf before enable it
			struct _CmdTnrEnable_t param;

			param.isUsingTdbEn = 1;
			param.isFullStill = 0;
			param.mode = CMD_TEMPER_MODE_2;
			param.refNum = 1;
			ret = isp_send_fw_cmd(isp, CMD_ID_TNR_ENABLE,
					      stream_id,
					      FW_CMD_PARA_TYPE_DIRECT, &param,
					      sizeof(param));
			if (ret != RET_SUCCESS) {
				//add this line to trick CP
				ISP_PR_ERR("fail to enable tnr!");
			} else {
				//add this line to trick CP
				ISP_PR_INFO("suc to enable tnr");
			}
		}
		goto goon;
	}

	disable_tnr = !isp->drv_settings.tnr_enable;
	snr_info->tnr_tmp_buf_set = 0;
	id = isp_get_id_for_prf(cid, PROFILE_NOHDR_NOBINNING_12MP_30);
	width = g_snr_info[cid].modes[id].w;
	height = g_snr_info[cid].modes[id].h;
	size = ISP_ADDR_ALIGN_UP_1K((width * 20 + 7) / 8) * height;
	size_with_gap = size + ISP_PREFETCH_GAP;
	for (i = 0; i < MAX_TNR_REF_BUF_CNT; i++) {
		if (snr_info->tnr_tmp_buf[i]) {
			isp_gpu_mem_free(snr_info->tnr_tmp_buf[i]);
			snr_info->tnr_tmp_buf[i] = NULL;
		}

		snr_info->tnr_tmp_buf[i] =
			isp_gpu_mem_alloc(size_with_gap, ISP_GPU_MEM_TYPE_NLFB);
		ISP_PR_DBG("cid:%d, tnr buf cnt %d, size %d", cid,
			   MAX_TNR_REF_BUF_CNT, size_with_gap);
	}

	if (!snr_info->tnr_tmp_buf[0] && !snr_info->tnr_tmp_buf[1]) {
		ISP_PR_ERR("fail no ref, disable tnr!");
		disable_tnr = 1;
	} else if (snr_info->tnr_tmp_buf[0] && snr_info->tnr_tmp_buf[1]) {
		//two tnr ref buffer
		memset(&buf_type, 0, sizeof(buf_type));
		buf_type.bufferType = BUFFER_TYPE_TNR_REF;
		buf_type.buffer.bufTags = 0;
		buf_type.buffer.vmidSpace.bit.vmid = 0;
		buf_type.buffer.vmidSpace.bit.space = ADDR_SPACE_TYPE_GPU_VA;
		isp_split_addr64(snr_info->tnr_tmp_buf[0]->gpu_mc_addr,
				 &buf_type.buffer.bufBaseALo,
				 &buf_type.buffer.bufBaseAHi);
		buf_type.buffer.bufSizeA = size;
		isp_split_addr64(snr_info->tnr_tmp_buf[1]->gpu_mc_addr,
				 &buf_type.buffer.bufBaseBLo,
				 &buf_type.buffer.bufBaseBHi);
		buf_type.buffer.bufSizeB = size;
		ret = isp_send_fw_cmd(isp, CMD_ID_SEND_BUFFER, stream_id,
				      FW_CMD_PARA_TYPE_DIRECT, &buf_type,
				      sizeof(buf_type));
		if (ret != RET_SUCCESS) {
			isp_gpu_mem_free(snr_info->tnr_tmp_buf[0]);
			snr_info->tnr_tmp_buf[0] = NULL;
			isp_gpu_mem_free(snr_info->tnr_tmp_buf[1]);
			snr_info->tnr_tmp_buf[1] = NULL;
			ISP_PR_ERR("fail to send 2 ref, disable tnr!");
			disable_tnr = 1;
		} else {
			struct _CmdTnrEnable_t param;

			ISP_PR_INFO("suc to send 2 ref");
			param.isUsingTdbEn = 1;
			param.isFullStill = 0;
			param.mode = CMD_TEMPER_MODE_TDB;
			param.refNum = 2;
			ret = isp_send_fw_cmd(isp, CMD_ID_TNR_ENABLE, stream_id,
					      FW_CMD_PARA_TYPE_DIRECT, &param,
					      sizeof(param));
			if (ret != RET_SUCCESS) {
				disable_tnr = 1;
				ISP_PR_ERR("fail to enable 2 buf, disable tnr");
			} else {
				snr_info->tnr_tmp_buf_set = 2;
				ISP_PR_INFO("suc to enable 2 buf");
			}
		}
	} else {
		//one tnr ref buffer
		struct isp_gpu_mem_info *ref_buf;

		ref_buf = snr_info->tnr_tmp_buf[0];
		if (!ref_buf)
			ref_buf = snr_info->tnr_tmp_buf[1];

		if (!ref_buf) {
			disable_tnr = 1;
			ISP_PR_ERR("fail! shouldn't be here");
			goto goon;
		}

		memset(&buf_type, 0, sizeof(buf_type));
		buf_type.bufferType = BUFFER_TYPE_TNR_REF;
		buf_type.buffer.bufTags = 0;
		buf_type.buffer.vmidSpace.bit.vmid = 0;
		buf_type.buffer.vmidSpace.bit.space = ADDR_SPACE_TYPE_GPU_VA;
		isp_split_addr64(ref_buf->gpu_mc_addr,
				 &buf_type.buffer.bufBaseALo,
				 &buf_type.buffer.bufBaseAHi);
		buf_type.buffer.bufSizeA = size;
		ret = isp_send_fw_cmd(isp, CMD_ID_SEND_BUFFER, stream_id,
				      FW_CMD_PARA_TYPE_DIRECT, &buf_type,
				      sizeof(buf_type));
		if (ret != RET_SUCCESS) {
			ISP_PR_ERR("fail to send 1 ref buf, disable tnr!");
			disable_tnr = 1;
		} else {
			struct _CmdTnrEnable_t param;

			ISP_PR_INFO("suc to send 1 ref buf");

			param.isUsingTdbEn = 1;
			param.isFullStill = 0;
			param.mode = CMD_TEMPER_MODE_2;
			param.refNum = 1;
			ret = isp_send_fw_cmd(isp, CMD_ID_TNR_ENABLE, stream_id,
					      FW_CMD_PARA_TYPE_DIRECT, &param,
					      sizeof(param));
			if (ret != RET_SUCCESS) {
				disable_tnr = 1;
				ISP_PR_ERR("fail to enable 1 buf, disable tnr");
			} else {
				snr_info->tnr_tmp_buf_set = 1;
				ISP_PR_INFO("suc to enable 1 buf");
			}
		}
	}
goon:
	if (disable_tnr) {
		//no tnr ref buffer
		struct _CmdTnrDisable_t param;

		param.isFullStill = 0;
		ret = isp_send_fw_cmd(isp, CMD_ID_TNR_DISABLE, stream_id,
				      FW_CMD_PARA_TYPE_DIRECT, &param,
				      sizeof(param));
		if (ret != RET_SUCCESS) {
			//add this line to trick CP
			ISP_PR_ERR("fail to disable tnr!");
		} else {
			//add this line to trick CP
			ISP_PR_INFO("suc to disable tnr");
		}
	}

	RET(ret);
	return RET_SUCCESS;
}

int isp_setup_stream_tmp_buf(struct isp_context *isp, enum camera_id cid)
{
	struct sensor_info *snr_info;
	unsigned int i, id;
	unsigned int size;
	unsigned int size_with_gap;
	struct _CmdSendBuffer_t buf_type;
	enum fw_cmd_resp_stream_id stream_id;

	ENTER();
	if (!is_para_legal(isp, cid)) {
		ISP_PR_ERR("bad para,cid:%d\n", cid);
		return RET_FAILURE;
	};
	stream_id = isp_get_stream_id_from_cid(isp, cid);
	snr_info = &isp->sensor_info[cid];
	id = isp_get_id_for_prf(cid, PROFILE_NOHDR_NOBINNING_12MP_30);
	size = g_snr_info[cid].modes[id].w * g_snr_info[cid].modes[id].h * 2;
	//as required by prefetch, ISP_PREFETCH_GAP is needed between
	//different raw buffers
	size_with_gap = size + ISP_PREFETCH_GAP;
	ISP_PR_DBG("cid:%d, buf cnt %d, size %d", cid, STREAM_TMP_BUF_COUNT,
		    size_with_gap);

	if (!isp_is_host_processing_raw(isp, cid)
	|| (snr_info->cam_type == CAMERA_TYPE_IR)) {
		unsigned int align_32k = 1;
#if	!defined(ISP_BRINGUP_WORKAROUND)
		for (i = 0; i < STREAM_TMP_BUF_COUNT; i++) {

			if (snr_info->stream_tmp_buf[i])
				isp_gpu_mem_free(snr_info->stream_tmp_buf[i]);
			snr_info->stream_tmp_buf[i] =
			isp_gpu_mem_alloc(size_with_gap, ISP_GPU_MEM_TYPE_NLFB);

			if (snr_info->stream_tmp_buf[i] == NULL) {
				ISP_PR_ERR("alloc tmp buf[%u] failed\n", i);
				goto fail;
			}
		};
#endif

		if (g_prop->cvip_enable == CVIP_STATUS_ENABLE)
			goto goon;

		for (i = 0; i < STREAM_TMP_BUF_COUNT; i++) {
			/*unsigned int *pbuf;
			 *pbuf = (unsigned int *)
			 *	snr_info->stream_tmp_buf[i]->sys_addr;
			 *if (pbuf) {
			 *	pbuf[0] = 0x11223344;
			 *	pbuf[1] = 0x55667788;
			 *	pbuf[2] = 0x99aabbcc;
			 *	pbuf[3] = 0xddeeff00;
			 *	ISP_PR_DBG("0x%x 0x%x 0x%x 0x%x\n",
			 *		pbuf[0], pbuf[1], pbuf[2], pbuf[3]);
			 *}
			 */
			memset(&buf_type, 0, sizeof(buf_type));
			buf_type.bufferType = BUFFER_TYPE_RAW_TEMP;
			buf_type.buffer.bufTags = 0;
			buf_type.buffer.vmidSpace.bit.vmid = 0;
			buf_type.buffer.vmidSpace.bit.space =
				ADDR_SPACE_TYPE_GPU_VA;
			isp_split_addr64(
				snr_info->stream_tmp_buf[i]->gpu_mc_addr,
					&buf_type.buffer.bufBaseALo,
					&buf_type.buffer.bufBaseAHi);
			buf_type.buffer.bufSizeA = size;
			/*
			 *memset(snr_info->stream_tmp_buf[i]->sys_addr, 0,
			 *(size_t)snr_info->stream_tmp_buf[i]->mem_size);
			 */
			if (isp_fw_set_send_buffer(isp, cid,
					buf_type.bufferType,
					buf_type.buffer) != RET_SUCCESS) {
				ISP_PR_ERR("send tmp buf[%u] failed\n", i);
				goto fail;
			};
			#if defined(PER_FRAME_CTRL)
			buf_type.bufferType = BUFFER_TYPE_FULL_RAW_TEMP;
			if (isp_fw_set_send_buffer(isp, cid,
					buf_type.bufferType,
					buf_type.buffer) != RET_SUCCESS) {
				ISP_PR_ERR("send tmp buf[%u] failed\n", i);
				goto fail;
			}
			#endif
			if ((buf_type.buffer.bufBaseALo % (32 * 1024)))
				align_32k = 0;
		};

		ISP_PR_INFO("send tmp raw suc\n");
	}

goon:
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
		}
	}
	//setup iic control buf
	if (snr_info->iic_tmp_buf == NULL) {
		size = IIC_CONTROL_BUF_SIZE;

		snr_info->iic_tmp_buf =
			isp_gpu_mem_alloc(size, ISP_GPU_MEM_TYPE_NLFB);

		if (snr_info->iic_tmp_buf) {
			//add this line to trick CP
			ISP_PR_INFO("alloc iic control buf suc\n");
		} else {
			//add this line to trick CP
			ISP_PR_ERR("alloc iic control buf fail\n");
		}
	}

	//send emb data buf
	size = isp_fb_get_emb_data_size();
	for (i = 0; i < EMB_DATA_BUF_NUM; i++) {
		snr_info->emb_data_buf[i] =
			isp_gpu_mem_alloc(size, ISP_GPU_MEM_TYPE_NLFB);
		if (!snr_info->emb_data_buf[i]) {
			ISP_PR_ERR("alloc embdata[%u] fail\n", i);
			continue;
		}

		memset(&buf_type, 0, sizeof(buf_type));
		buf_type.bufferType = BUFFER_TYPE_EMB_DATA;
		buf_type.buffer.bufTags = 0;
		buf_type.buffer.vmidSpace.bit.vmid = 0;
		buf_type.buffer.vmidSpace.bit.space = ADDR_SPACE_TYPE_GPU_VA;
		isp_split_addr64(snr_info->emb_data_buf[i]->gpu_mc_addr,
				 &buf_type.buffer.bufBaseALo,
				 &buf_type.buffer.bufBaseAHi);
		buf_type.buffer.bufSizeA = snr_info->emb_data_buf[i]->mem_size;
		if (isp_send_fw_cmd(isp, CMD_ID_SEND_BUFFER, stream_id,
				    FW_CMD_PARA_TYPE_DIRECT, &buf_type,
			sizeof(buf_type)) != RET_SUCCESS) {
			isp_gpu_mem_free(snr_info->emb_data_buf[i]);
			snr_info->emb_data_buf[i] = NULL;
			ISP_PR_ERR("send embdata[%u] fail\n", i);
		} else {
			ISP_PR_INFO("send embdata[%u] suc\n", i);
		};
	}
	//send tnr buf
	//enable tnr
	isp_setup_tnr_ref_buf(isp, cid, stream_id, snr_info);

	RET(RET_SUCCESS);
	return RET_SUCCESS;

 fail:
	for (i = 0; i < STREAM_TMP_BUF_COUNT; i++) {
		if (snr_info->stream_tmp_buf[i]) {
			isp_gpu_mem_free(snr_info->stream_tmp_buf[i]);
			snr_info->stream_tmp_buf[i] = NULL;
		}
	};

	RET(RET_FAILURE);
	return RET_FAILURE;
}

int isp_uninit_stream(struct isp_context *isp, enum camera_id cam_id)
{
	struct sensor_info *snr_info;
	unsigned int i;
	enum fw_cmd_resp_stream_id stream;
	struct isp_cmd_element *ele = NULL;
	struct isp_mapped_buf_info *img_info = NULL;
	enum stream_id str_id;
	enum camera_id cid = cam_id;
	struct _CmdStopStream_t stop_stream_para;
	unsigned int timeout;
	unsigned int out_cnt;
	enum fw_cmd_resp_stream_id stream_id;

	ENTER();
	if (!is_para_legal(isp, cam_id)) {
		ISP_PR_ERR("bad para, cid[%d]\n", cam_id);
		return RET_FAILURE;
	};
	if (!isp->sensor_info[cam_id].stream_inited) {
		ISP_PR_WARN("cid[%d], do none for not started\n", cam_id);
		return RET_SUCCESS;
	}

	isp_get_stream_output_bits(isp, cid, &out_cnt);
	if (out_cnt > 0) {
		ISP_PR_ERR("cid[%d] fail for there is still %u output\n",
			cam_id, out_cnt);
		return RET_FAILURE;
	}

	stream_id = isp_get_stream_id_from_cid(isp, cid);
	stop_stream_para.destroyBuffers = true;

	timeout = (g_isp_env_setting == ISP_ENV_SILICON) ?
			(1000 * 2) : (1000 * 30);

	if (isp->sensor_info[cam_id].per_frame_ctrl_cnt >=
		PFC_CNT_BEFORE_START_STREAM) {
		if (g_prop->cvip_enable == CVIP_STATUS_ENABLE)
			isp_stop_cvip_sensor(isp, cam_id);
#ifdef DO_SYNCHRONIZED_STOP_STREAM
		if (isp_send_fw_cmd_sync(isp, CMD_ID_STOP_STREAM, stream_id,
					 FW_CMD_PARA_TYPE_DIRECT,
					 &stop_stream_para,
					 sizeof(stop_stream_para),
					 timeout,
					 NULL, NULL) != RET_SUCCESS) {
#else
		if (isp_fw_set_stop_stream(isp, cid,
					   stop_stream_para.destroyBuffers) !=
		    RET_SUCCESS) {
#endif
			ISP_PR_ERR("send stop stream fail\n");
		} else {
			ISP_PR_INFO("wait stop stream suc\n");
		}
	}

	isp->sensor_info[cam_id].stream_inited = 0;
	isp->sensor_info[cam_id].per_frame_ctrl_cnt = 0;

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

#if	!defined(ISP_BRINGUP_WORKAROUND)
	for (i = 0; i < STREAM_TMP_BUF_COUNT; i++) {
		if (snr_info->stream_tmp_buf[i]) {
			isp_gpu_mem_free(snr_info->stream_tmp_buf[i]);
			snr_info->stream_tmp_buf[i] = NULL;
		}
	};
#endif

	if (snr_info->iic_tmp_buf) {
		isp_gpu_mem_free(snr_info->iic_tmp_buf);
		snr_info->iic_tmp_buf = NULL;
	}

	for (i = 0; i < EMB_DATA_BUF_NUM; i++) {
		if (snr_info->emb_data_buf[i]) {
			isp_gpu_mem_free(snr_info->emb_data_buf[i]);
			snr_info->emb_data_buf[i] = NULL;
		}
	}

	snr_info->tnr_tmp_buf_set = 0;
	for (i = 0; i < MAX_TNR_REF_BUF_CNT; i++) {
		if (snr_info->tnr_tmp_buf[i]) {
			isp_gpu_mem_free(snr_info->tnr_tmp_buf[i]);
			snr_info->tnr_tmp_buf[i] = NULL;
		}
	}
	if (snr_info->cmd_resp_buf) {
		isp_gpu_mem_free(snr_info->cmd_resp_buf);
		snr_info->cmd_resp_buf = NULL;
	}

	for (str_id = STREAM_ID_PREVIEW; str_id <= STREAM_ID_NUM; str_id++) {
		if (snr_info->str_info[str_id].uv_tmp_buf) {
			isp_gpu_mem_free(snr_info->str_info[str_id].uv_tmp_buf);
			snr_info->str_info[str_id].uv_tmp_buf = NULL;
		}
	};

	if (snr_info->hdr_stats_buf) {
		isp_gpu_mem_free(snr_info->hdr_stats_buf);
		snr_info->hdr_stats_buf = NULL;
	}

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

	RET(RET_SUCCESS);
	return RET_SUCCESS;
}

int isp_init_stream(struct isp_context *isp, enum camera_id cam_id)
{
	struct _CmdSetRawPktFmt_t raw_fmt;
	enum fw_cmd_resp_stream_id stream_id;

	ENTER();
	if (!is_para_legal(isp, cam_id)) {
		ISP_PR_ERR("bad para,cid:%d\n", cam_id);
		return RET_FAILURE;
	};

	if (isp->sensor_info[cam_id].stream_inited) {
		ISP_PR_WARN("cid[%d], suc do none\n", cam_id);
		return RET_SUCCESS;
	}

	if (isp_set_clk_info_2_fw(isp) != RET_SUCCESS) {
		ISP_PR_ERR("fail for isp_set_clk_info_2_fw\n");
		return RET_FAILURE;
	};

	if (isp_set_snr_info_2_fw(isp, cam_id) != RET_SUCCESS) {
		ISP_PR_ERR("fail for isp_set_snr_info_2_fw\n");
	};

	if (g_prop->cvip_enable == CVIP_STATUS_ENABLE) {
		if (isp_config_cvip_sensor(isp, cam_id, TEST_PATTERN_NO) !=
					   RET_SUCCESS) {
			ISP_PR_ERR("fail for isp_config_cvip_sensor\n");
			return RET_FAILURE;
		}

		if (isp_setup_cvip_buf(isp, cam_id) != RET_SUCCESS) {
			ISP_PR_ERR("fail for isp_setup_cvip_buf\n");
			return RET_FAILURE;
		}

		if (isp_set_cvip_sensor_slice_num(isp, cam_id) != RET_SUCCESS) {
			ISP_PR_ERR("fail for isp_set_sensor_slice_num\n");
			return RET_FAILURE;
		}
	}

	if (isp_setup_stream(isp, cam_id) != RET_SUCCESS) {
		ISP_PR_ERR("fail for isp_setup_stream\n");
		return RET_FAILURE;
	};

	if (isp_setup_stream_tmp_buf(isp, cam_id) != RET_SUCCESS) {
		ISP_PR_ERR("fail for isp_setup_stream_tmp_buf\n");
		return RET_FAILURE;
	};

	stream_id = isp_get_stream_id_from_cid(isp, cam_id);
	if (g_prop->cvip_enable == CVIP_STATUS_ENABLE) {
		raw_fmt.rawPktFmt = isp_trans_to_fw_raw_pkt_fmt
					(g_prop->cvip_raw_fmt);
	} else {
		raw_fmt.rawPktFmt = RAW_PKT_FMT_4;
	}

	if (isp->sensor_info[cam_id].cam_type == CAMERA_TYPE_RGBIR)
		raw_fmt.rawPktFmt = RAW_PKT_FMT_6;
	if (isp_fw_set_raw_fmt(isp, cam_id, raw_fmt) != RET_SUCCESS) {
		ISP_PR_ERR("failed to set raw_fmt to %u\n", raw_fmt.rawPktFmt);
	} else {
		ISP_PR_INFO("set raw_fmt to %u suc\n", raw_fmt.rawPktFmt);
	};
	isp->sensor_info[cam_id].raw_pkt_fmt = raw_fmt.rawPktFmt;

	isp->sensor_info[cam_id].stream_inited = 1;

	RET(RET_SUCCESS);
	return RET_SUCCESS;
}

int isp_is_host_processing_raw(struct isp_context *isp, enum camera_id cid)
{
	struct sensor_info *sif;

	sif = &isp->sensor_info[cid];
	if (sif->cam_type == CAMERA_TYPE_RGBIR)
		return TRUE;
	if ((sif->cam_type == CAMERA_TYPE_RGB_BAYER)
	&& isp->drv_settings.process_3a_by_host)
		return TRUE;

	if ((sif->cam_type == CAMERA_TYPE_RGBIR_HWDIR)
		&& isp->drv_settings.process_3a_by_host)
		return TRUE;
	return FALSE;
};

void isp_calc_aspect_ratio_wnd(struct isp_context *isp,
				enum camera_id cid, unsigned int w,
				unsigned int h,
				struct _Window_t *aprw)
{
	unsigned int raw_height, raw_width;
	struct sensor_info *sif;

	ENTER();
	sif = &isp->sensor_info[cid];
	raw_height = sif->raw_height;
	raw_width = sif->raw_width;

	if (isp_is_mem_camera(isp, cid)) {
		raw_height *= 2;
		raw_width *= 2;
	}
	ISP_PR_DBG("%u %u %u %u %u %u\n",
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
		unsigned int raw_w_new;
		unsigned int raw_h_new;

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
		ISP_PR_DBG("change aprw.window.h_size to %u\n",
			aprw->h_size);
	};

	if ((aprw->v_size % 2) != 0) {
		aprw->v_size--;
		ISP_PR_DBG("change aprw.window.v_size to %u\n",
			aprw->v_size);
	};

	if (isp_is_host_processing_raw(isp, cid)
	&& sif->str_info[STREAM_ID_RGBIR].width
	&& sif->str_info[STREAM_ID_RGBIR].height) {
		aprw->h_offset = 0;
		aprw->v_offset = 0;
		aprw->h_size = sif->str_info[STREAM_ID_RGBIR].width;
		aprw->v_size = sif->str_info[STREAM_ID_RGBIR].height;
		ISP_PR_DBG("rgbir rechange aprw to %u:%u\n",
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

	EXIT();
}


int isp_setup_aspect_ratio_wnd(struct isp_context *isp,
				enum camera_id cid, unsigned int w,
				unsigned int h)
{
#if	defined(DELETED)
	struct _CmdSetAspectRatioWindow_t aprw;
	//unsigned int raw_height, raw_width;
	enum fw_cmd_resp_stream_id stream_id;
	struct sensor_info *sif;

	sif = &isp->sensor_info[cid];
	//raw_height = sif->raw_height;
	//raw_width = sif->raw_width;

	isp_calc_aspect_ratio_wnd(isp, cid, w, h, &aprw.window);
#if	defined(ISP_LOCAL_TEST)
	aprw.window.h_offset = 0;
	aprw.window.h_size = 640;//w
	aprw.window.v_offset = 0;
	aprw.window.v_size = 480;	//h
#endif
	isp->sensor_info[cid].asprw_wind = aprw.window;
	stream_id = isp_get_stream_id_from_cid(isp, cid);
	if (isp_fw_set_aspect_ratio_window(isp, cid,
			aprw) != RET_SUCCESS) {
		ISP_PR_ERR("fail for set ratio window\n");
		return RET_FAILURE;
	}

	ISP_PR_DBG("aspr x:y %u:%u,w:h %u:%u\n",
		aprw.window.h_offset, aprw.window.v_offset,
		aprw.window.h_size, aprw.window.v_size);
#endif

	return RET_SUCCESS;
}


int isp_setup_output(struct isp_context *isp, enum camera_id cid)
{
	int ret = RET_FAILURE;

	ISP_PR_INFO("enter, cid[%d]", cid);
	if (!is_para_legal(isp, cid)) {
		ISP_PR_ERR("bad para, cid[%d]", cid);
		goto quit;
	}

	if (!isp->sensor_info[cid].start_str_cmd_sent) {
		ret = isp_kickoff_stream(isp, cid, 0, 0);
		if (ret != RET_SUCCESS) {
			//add this line to trick CP
			ISP_PR_ERR("cid[%d] kickoff stream fail", cid);
		} else {
			isp->sensor_info[cid].status = START_STATUS_STARTED;
			ISP_PR_INFO("cid[%d] kickoff stream suc", cid);
		}

		isp->sensor_info[cid].start_str_cmd_sent = 1;
	}
quit:
	RET(ret);
	return ret;
}


int isp_setup_ae_range(struct isp_context *isp, enum camera_id cid)
{
#if	defined(PER_FRAME_CTRL)
	int ret = RET_FAILURE;
	CmdAeSetAnalogGainRange_t a_gain;
	CmdAeSetDigitalGainRange_t d_gain;
	CmdAeSetItimeRange_t i_time = {0};
	enum fw_cmd_resp_stream_id stream_id;
	struct sensor_info *sif;

	ENTER();
	if (!is_para_legal(isp, cid)) {
		ISP_PR_ERR("bad para,isp:%p,cid:%d\n", isp, cid);
		goto quit;
	};
	stream_id = isp_get_stream_id_from_cid(isp, cid);
	sif = &isp->sensor_info[cid];

	if ((sif->aec_mode != AE_MODE_AUTO)
	|| !sif->calib_enable || !sif->calib_set_suc) {
		ISP_PR_WARN("suc,do none,cid %u\n", cid);
		ret = RET_SUCCESS;
		goto quit;
	};

	a_gain.minGain = sif->sensor_cfg.exposure.min_gain;
	a_gain.maxGain = sif->sensor_cfg.exposure.max_gain;

	d_gain.minGain = sif->sensor_cfg.exposure.min_digi_gain;
	d_gain.maxGain = sif->sensor_cfg.exposure.max_digi_gain;

	i_time.minItime = sif->sensor_cfg.exposure.min_integration_time;
	i_time.maxItime = sif->sensor_cfg.exposure.max_integration_time;

	if (isp->drv_settings.low_light_fps) {
		i_time.maxItimeLowLight = 1000000 /
			isp->drv_settings.low_light_fps;
		ISP_PR_INFO("set maxItimeLowLight to %u\n",
			i_time.maxItimeLowLight);
		//Set it again for Registry control to
		//avoid overridden by Scene mode
		sif->enable_dynamic_frame_rate = TRUE;
	}

	RET(RET_SUCCESS);
	ret = RET_SUCCESS;
 quit:
	return ret;
#endif
	return 0;
}

int isp_setup_ae(struct isp_context *isp, enum camera_id cid)
{
	int ret = RET_FAILURE;
	enum fw_cmd_resp_stream_id stream_id;
	struct sensor_info *sif;
	struct _CmdAeSetFlicker_t flick;
	//CmdAeSetRegion_t aeroi;

	ENTER();
	if (!is_para_legal(isp, cid)) {
		ISP_PR_ERR("bad para,isp:%p,cid:%d\n", isp, cid);
		goto quit;
	};

	stream_id = isp_get_stream_id_from_cid(isp, cid);
	sif = &isp->sensor_info[cid];

	while (true) {
		struct _CmdAeSetMode_t ae;

		if (isp->drv_settings.ae_default_auto == 0)
			sif->aec_mode = AE_MODE_MANUAL;

		if (sif->aec_mode != AE_MODE_AUTO) {
			ISP_PR_WARN("ae not auto(%u), do none\n",
				sif->aec_mode);
			break;
		};
		if (!sif->calib_enable || !sif->calib_set_suc) {
			ISP_PR_WARN("not set ae to auto for no calib\n");
			break;
		};
		ae.mode = sif->aec_mode;
		if (isp_fw_set_exposure_mode(isp, cid, ae.mode) !=
		RET_SUCCESS) {
			ISP_PR_ERR("failed to set ae mode(%s)\n",
				isp_dbg_get_ae_mode_str(ae.mode));
		} else {
			ISP_PR_INFO("set ae mode(%s) suc\n",
				isp_dbg_get_ae_mode_str(ae.mode));
		};
		break;
	}

	if ((sif->flick_type_set == AE_FLICKER_TYPE_50HZ)
	|| (sif->flick_type_set == AE_FLICKER_TYPE_60HZ)) {
		flick.flickerType = sif->flick_type_set;
		if (isp_fw_set_ae_flicker(isp, cid,
				flick.flickerType) != RET_SUCCESS) {
			ISP_PR_ERR("failed to set flick\n");
		} else {
			ISP_PR_INFO("set flick %d suc\n",
				flick.flickerType);
		};
	} else
		ISP_PR_INFO("set flick to off\n");

#if	defined(PER_FRAME_CTRL)
	//aeroi.window = sif->ae_roi;
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
	case CAMERA_ID_MEM:
		if (roi_window_isvalid(&isp->drv_settings.frontl_roi_ae_wnd))
			aeroi.window = isp->drv_settings.frontl_roi_ae_wnd;
		break;
	case CAMERA_ID_FRONT_RIGHT:
		if (roi_window_isvalid(&isp->drv_settings.frontr_roi_ae_wnd))
			aeroi.window = isp->drv_settings.frontr_roi_ae_wnd;
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
		ISP_PR_ERR("NO initial AE ROI\n");
	} else if (isp_fw_set_ae_region(isp, cid,
			aeroi) != RET_SUCCESS) {
		ISP_PR_ERR("failed to set stream_id[%u] aeroi[%u,%u,%u,%u]\n",
			stream_id,
			aeroi.window.h_offset, aeroi.window.v_offset,
			aeroi.window.h_size, aeroi.window.v_size);
	} else {
		ISP_PR_INFO("set stream_id[%u] aeroi [%u,%u,%u,%u] suc\n",
			stream_id,
			aeroi.window.h_offset, aeroi.window.v_offset,
			aeroi.window.h_size, aeroi.window.v_size);
	};
#endif

	if ((sif->aec_mode == AE_MODE_AUTO) && sif->calib_enable
	&& sif->calib_set_suc)
		isp_setup_ae_range(isp, cid);
	else
		ISP_PR_INFO("no need set ae range\n");

	/*todo to be add later for 0.6 fw doesn't support */

	sif->hdr_enabled_suc = 0;
	if (isp_fw_set_hdr_enable(isp, cid,
			sif->hdr_enable && sif->hdr_prop_set_suc)
			!= RET_SUCCESS) {
		ISP_PR_ERR("fail enable hdr\n");
	} else {
		sif->hdr_enabled_suc =
			sif->hdr_enable && sif->hdr_prop_set_suc;
		ISP_PR_ERR("enable hdr suc\n");
	}

	ret = RET_SUCCESS;
 quit:
	RET(ret);
	return ret;
};

int isp_setup_af(struct isp_context __maybe_unused *isp,
	enum camera_id __maybe_unused cid)
{
	return RET_SUCCESS;
}

int isp_setup_awb(struct isp_context *isp, enum camera_id cid)
{
	int ret = RET_FAILURE;
	struct sensor_info *sif;
	//CmdAwbSetRegion_t rgn;

	ISP_PR_INFO("enter, cid[%u]", cid);
	if (!is_para_legal(isp, cid)) {
		ISP_PR_ERR("bad para,isp:%p,cid:%d\n", isp, cid);
		goto quit;
	};

	sif = &isp->sensor_info[cid];

	if (isp->drv_settings.awb_default_auto == 0)
		sif->awb_mode = AWB_MODE_MANUAL;

	/*Disable AWB in case of pure IR camera */
	if (sif->sensor_cfg.prop.cfaPattern == CFA_PATTERN_PURE_IR)
		sif->awb_mode = AWB_MODE_MANUAL;

	if (sif->awb_mode != AWB_MODE_MANUAL) {
		struct _CmdAwbSetMode_t m;

		m.mode = sif->awb_mode;
		if (isp_fw_set_wb_mode(isp, cid,
				m.mode) != RET_SUCCESS) {
			ISP_PR_ERR("failed to set mode[%s]\n",
				isp_dbg_get_awb_mode_str(m.mode));
		} else {
			ISP_PR_INFO("set mode[%s] suc\n",
				isp_dbg_get_awb_mode_str(m.mode));
		}
	}

#if	defined(PER_FRAME_CTRL)
	//rgn.window = sif->awb_region;
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
	}

//	If no customeized ROI, we will set WB ROI as the whole frame resolution
	if ((rgn.window.h_size == 0) || (rgn.window.v_size == 0)) {
		if (sif->cur_calib_item) {
			rgn.window.h_offset = 0;
			rgn.window.v_offset = 0;
			rgn.window.h_size = sif->cur_calib_item->width;
			rgn.window.v_size = sif->cur_calib_item->height;
		}
	}

	if ((rgn.window.h_size == 0) || (rgn.window.v_size == 0)) {
		ISP_PR_ERR("NO initial WB ROI\n");
	} else if (isp_fw_set_wb_region(isp, cid, rgn.window) !=
	RET_SUCCESS) {
		ISP_PR_ERR("failed to set roi\n");
	} else {
		ISP_PR_DBG("set roi suc w:h=%u:%u\n",
			rgn.window.h_offset, rgn.window.v_offset);
	}
#endif
	if ((sif->awb_mode == AWB_MODE_MANUAL) && sif->color_temperature) {
		struct _CmdAwbSetColorTemperature_t t;

		t.colorTemperature = sif->color_temperature;
		if (isp_fw_set_wb_colorT(isp, cid,
				t.colorTemperature) != RET_SUCCESS) {
			ISP_PR_ERR("failed to set color temperature[%u]\n",
				t.colorTemperature);
		} else {
			ISP_PR_DBG("set color temperature[%u] suc\n",
				t.colorTemperature);
		};
	} else
		ISP_PR_DBG("suc no need set temper\n");

	if (sif->awb_mode == AWB_MODE_MANUAL
	&& isp->drv_settings.fix_wb_lighting_index != -1) {
		if (set_wb_light_imp(isp, cid,
		isp->drv_settings.fix_wb_lighting_index) !=
		IMF_RET_SUCCESS) {
			ISP_PR_ERR("failed to set light index\n");
		} else {
			ISP_PR_DBG("set light index (%u)\n",
				isp->drv_settings.fix_wb_lighting_index);
		};
	}

	ret = RET_SUCCESS;
	isp_dbg_get_awb_mode_str(sif->awb_mode);
 quit:
	RET(ret);
	return ret;
}

int isp_setup_ctls(struct isp_context *isp, enum camera_id cid)
{
	int ret = RET_SUCCESS;
	enum fw_cmd_resp_stream_id stream_id;
	struct sensor_info *sif;

	ENTER();
	if (!is_para_legal(isp, cid)) {
		ISP_PR_ERR("bad para,isp:%p,cid:%d\n", isp, cid);
		return RET_FAILURE;
	};

	stream_id = isp_get_stream_id_from_cid(isp, cid);
	sif = &isp->sensor_info[cid];

	ISP_PR_DBG("cid[%u],stream[%u]\n", cid, stream_id);
	if (sif->para_brightness_set != BRIGHTNESS_DEF_VALUE)
		fw_if_set_brightness(isp, cid, sif->para_brightness_set);
	if (sif->para_contrast_set != CONTRAST_DEF_VALUE)
		fw_if_set_contrast(isp, cid, sif->para_contrast_set);
	if (sif->para_hue_set != HUE_DEF_VALUE)
		fw_if_set_hue(isp, cid, sif->para_hue_set);
	if (sif->para_satuaration_set != SATURATION_DEF_VALUE)
		fw_if_set_satuation(isp, cid, sif->para_satuaration_set);

	RET(ret);
	return ret;
}

int isp_kickoff_stream(struct isp_context *isp, enum camera_id cid,
		       unsigned int w, unsigned int h)
{
	enum fw_cmd_resp_stream_id stream_id;
	struct _CmdSetAcqWindow_t acqw;

	ENTER();
	if (!is_para_legal(isp, cid)) {
		ISP_PR_ERR("bad para, cid:%d\n", cid);
		return RET_FAILURE;
	}

	ISP_PR_DBG("cid[%d] w:h=%u:%u\n", cid, w, h);
	stream_id = isp_get_stream_id_from_cid(isp, cid);

	acqw.window.h_offset = 0;
	acqw.window.v_offset = 0;
	acqw.window.h_size = isp->sensor_info[cid].raw_width;
	acqw.window.v_size = isp->sensor_info[cid].raw_height;

	if (g_prop->cvip_enable == CVIP_STATUS_ENABLE &&
	    g_prop->cvip_raw_fmt == CVIP_RAW_PKT_FMT_5) {
		acqw.window.h_size -= (acqw.window.h_size % 8);
		acqw.window.v_size -= (acqw.window.v_size % 8);
	} else {
		//For raw pkt fmt 4, 10-bit data type width must
		//be multiples of 8
		acqw.window.h_size -= (acqw.window.h_size % 8);
		acqw.window.v_size -= (acqw.window.v_size % 8);
	}

	if (cid == CAMERA_ID_MEM) {
		acqw.window.h_size *= 2;
		acqw.window.v_size *= 2;
	}

	isp->sensor_info[cid].status = START_STATUS_START_FAIL;
	if (acqw.window.h_size == 0 || acqw.window.v_size == 0) {
		ISP_PR_ERR("bad acq w:h=%u:%u\n", acqw.window.h_size,
			   acqw.window.v_size);
	}

	if (isp_fw_set_acq_window(isp, cid, acqw.window) != RET_SUCCESS) {
		ISP_PR_ERR("failed to SET_ACQ_WINDOW\n");
		return RET_FAILURE;
	}
	ISP_PR_PC("cid[%d] acq w:h:x:y=%u:%u:%u:%u", cid, acqw.window.h_size,
		  acqw.window.v_size, acqw.window.h_offset,
		  acqw.window.v_offset);
	if (isp_set_calib_2_fw(isp, cid) != RET_SUCCESS) {
		ISP_PR_ERR("isp_set_calib_2_fw failed ignore\n");
		isp->sensor_info[cid].calib_enable = false;
	};

	isp_setup_aspect_ratio_wnd(isp, cid, w, h);
	isp->sensor_info[cid].status = START_STATUS_STARTED;

	RET(RET_SUCCESS);
	return RET_SUCCESS;
}

int isp_setup_3a(struct isp_context *isp, enum camera_id cam_id)
{
	int ret = RET_SUCCESS;
	struct sensor_info *snrif;

	ENTER();
	if (!is_para_legal(isp, cam_id)) {
		ISP_PR_ERR("bad para,cid:%d\n", cam_id);
		ret = RET_FAILURE;
		goto quit;
	};
	snrif = &isp->sensor_info[cam_id];
	if (isp_is_mem_camera(isp, cam_id)) {
		snrif->status = START_STATUS_START_FAIL;
		if (isp_setup_ae(isp, cam_id) != RET_SUCCESS) {
			ISP_PR_ERR("fail for set up ae\n");
			ret = RET_FAILURE;
			goto quit;
		};

		ISP_PR_DBG("cam_id[%d] isp_setup_ae suc\n", cam_id);
		snrif->status = START_STATUS_STARTING;
		goto quit;
	};

	if ((snrif->status == START_STATUS_STARTED)
	|| (snrif->status == START_STATUS_STARTING)) {
		ret = RET_SUCCESS;
		ISP_PR_WARN("already started, do none\n");
		goto quit;
	} else if (snrif->status == START_STATUS_START_FAIL) {
		ISP_PR_ERR("failed for stream status[%d]\n", snrif->status);
		ret = RET_FAILURE;
		goto quit;
	}

	snrif->status = START_STATUS_START_FAIL;
	if (isp_setup_ae(isp, cam_id) != RET_SUCCESS) {
		ISP_PR_ERR("failed to set up ae\n");
		ret = RET_FAILURE;
		goto quit;
	};

#if	defined(PER_FRAME_CTRL)
	if (isp_setup_af(isp, cam_id) != RET_SUCCESS) {
		ISP_PR_ERR("failed to set up af\n");
		ret = RET_FAILURE;
		goto quit;
	};
#endif

	if (isp_setup_awb(isp, cam_id) != RET_SUCCESS) {
		ISP_PR_ERR("failed to set up awb\n");
		ret = RET_FAILURE;
		goto quit;
	};
	snrif->status = START_STATUS_STARTING;
	ret = RET_SUCCESS;

 quit:
	RET(ret);
	return ret;
};

int isp_start_stream(struct isp_context *isp, enum camera_id cam_id,
			bool is_perframe_ctl)
{
	int ret = RET_FAILURE;
	struct _CmdConfigMmhubPrefetch_t prefetch;

	ISP_PR_DBG("enter, cid[%d]", cam_id);
	if (!is_para_legal(isp, cam_id)) {
		ISP_PR_ERR("bad para, cid:%d", cam_id);
		goto quit;
	}

	if (g_prop->prefetch_enable & 0xf00)
		prefetch.bEnStg1 = 1;
	else
		prefetch.bEnStg1 = 0;
	if (g_prop->prefetch_enable & 0xf0)
		prefetch.bEnStg2 = 1;
	else
		prefetch.bEnStg2 = 0;
	if (g_prop->prefetch_enable & 0xf)
		prefetch.bEnPerf = 1;
	else
		prefetch.bEnPerf = 0;
	prefetch.bAddGapForYuv = 1;

	if (isp_send_fw_cmd(isp, CMD_ID_CONFIG_MMHUB_PREFETCH,
		FW_CMD_RESP_STREAM_ID_GLOBAL, FW_CMD_PARA_TYPE_DIRECT,
		&prefetch, sizeof(prefetch)) != RET_SUCCESS) {
		ISP_PR_WARN("failed to config prefetch\n");
	} else {
		ISP_PR_INFO("config prefetch %d:%d:%d suc\n",
			    prefetch.bEnStg1,
			    prefetch.bEnStg2,
			    prefetch.bEnPerf);
	}

	ret = isp_setup_output(isp, cam_id);
	if (ret != RET_SUCCESS)
		ISP_PR_ERR("fail to setup output!");

quit:
	RET(ret);
	return ret;
}

int isp_set_stream_data_fmt(struct isp_context *isp_context,
					enum camera_id cam_id,
					enum stream_id stream_type,
					enum pvt_img_fmt img_fmt)
{
	struct isp_stream_info *sif;

	if (!is_para_legal(isp_context, cam_id)
	|| (stream_type > STREAM_ID_NUM)) {
		ISP_PR_ERR("bad para,isp[%p],cid[%d],sid[%d]\n",
			isp_context, cam_id, stream_type);
		return RET_FAILURE;
	};

	if ((img_fmt == PVT_IMG_FMT_INVALID) || (img_fmt >= PVT_IMG_FMT_MAX)) {
		ISP_PR_ERR("bad fmt,cid[%d],sid[%d],fmt[%d]\n",
			cam_id, stream_type, img_fmt);
		return RET_FAILURE;
	};
	sif = &isp_context->sensor_info[cam_id].str_info[stream_type];

	if (sif->start_status != START_STATUS_NOT_START) {
		if (sif->format == img_fmt) {
			ISP_PR_WARN("already started, do none\n");
			return RET_SUCCESS;
		}

		{
			ISP_PR_ERR("cid[%d] str[%d] fmt[%s] bad stat[%d]\n",
				cam_id, stream_type,
				isp_dbg_get_pvt_fmt_str(img_fmt),
				sif->start_status);
			return RET_FAILURE;
		}
	};

	sif->format = img_fmt;

	ISP_PR_DBG("suc,cid[%d] str[%d] fmt[%s]\n", cam_id, stream_type,
		isp_dbg_get_pvt_fmt_str(img_fmt));
	return RET_SUCCESS;
};

int isp_switch_profile(struct isp_context *isp, enum camera_id cid,
		       unsigned int prf_id)
{
	int ret;
	unsigned int index, o_index;
	struct sensor_info *snr_info = &isp->sensor_info[cid];
	struct _CmdSetAcqWindow_t acqw;
	struct _CmdSetSensorProp_t snr_prop;

	/* update cvip sensor clk */
	index = get_index_from_res_fps_id(isp, cid, (char)prf_id);
	o_index = get_index_from_res_fps_id(isp, cid, snr_info->cur_res_fps_id);
	if (index > o_index)
		isp_clk_change(isp, cid, index, snr_info->hdr_enable, true);

	/* config cvip sensor */
	ret = isp_config_cvip_sensor(isp, cid, TEST_PATTERN_NO);
	if (ret != RET_SUCCESS)
		goto fail;

	/* set cvip sensor prop */
	snr_prop.sensorId = isp_get_fw_sensor_id(isp, cid);
	memcpy(&snr_prop.sensorProp, &snr_info->sensor_cfg.prop,
	       sizeof(snr_prop.sensorProp));
	ret = isp_fw_set_sensor_prop(isp, cid, snr_prop);
	if (ret != RET_SUCCESS)
		goto fail;

	/* set cvip sensor acq window */
	acqw.window.h_offset = 0;
	acqw.window.v_offset = 0;
	acqw.window.h_size = isp->sensor_info[cid].raw_width;
	acqw.window.v_size = isp->sensor_info[cid].raw_height;
	if (g_prop->cvip_enable == CVIP_STATUS_ENABLE &&
	    g_prop->cvip_raw_fmt == CVIP_RAW_PKT_FMT_5) {
		acqw.window.h_size -= (acqw.window.h_size % 8);
		acqw.window.v_size -= (acqw.window.v_size % 8);
	} else {
		/* For raw pkt fmt 4, 10-bit data width must be multi of 8 */
		acqw.window.h_size -= (acqw.window.h_size % 8);
		acqw.window.v_size -= (acqw.window.v_size % 8);
	}

	if (acqw.window.h_size == 0 || acqw.window.v_size == 0) {
		ISP_PR_ERR("bad acq win w:h=%u:%u", acqw.window.h_size,
			   acqw.window.v_size);
		ret = RET_FAILURE;
		goto fail;
	}

	if (acqw.window.h_size != snr_info->raw_width &&
	    acqw.window.v_size != snr_info->raw_height) {
		ret = isp_fw_set_acq_window(isp, cid, acqw.window);
		if (ret != RET_SUCCESS)
			goto fail;

		ISP_PR_PC("acq w:h:x:y=%u:%u:%u:%u", acqw.window.h_size,
			  acqw.window.v_size, acqw.window.h_offset,
			  acqw.window.v_offset);
	}

	/* update sensor info */
	snr_info->cur_res_fps_id = (char)prf_id;

fail:
	RET(ret);
	return ret;
}

void isp_pause_stream_if_needed(struct isp_context *isp,
	enum camera_id cid, unsigned int w, unsigned int h)
{
	unsigned int running_stream_count = 0;
	struct _CmdStopStream_t stop_stream_para;
	enum fw_cmd_resp_stream_id stream_id;
	unsigned int cmd;
	unsigned int timeout;
	enum stream_id str_id;
	struct sensor_info *snr_info;
	struct _Window_t aprw;

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
	ISP_PR_DBG("cid[%d],aspr from [%u,%u,%u,%u] to [%u,%u,%u,%u]\n", cid,
		snr_info->asprw_wind.h_offset, snr_info->asprw_wind.v_offset,
		snr_info->asprw_wind.h_size, snr_info->asprw_wind.v_size,
		aprw.h_offset, aprw.v_offset, aprw.h_size, aprw.v_size);
	if (g_prop->cvip_enable == CVIP_STATUS_ENABLE)
		isp_stop_cvip_sensor(isp, cid);

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


int isp_set_str_res_fps_pitch(struct isp_context *isp_context,
					enum camera_id cam_id,
					enum stream_id stream_type,
					struct pvt_img_res_fps_pitch *value)
{
	unsigned int width;
	unsigned int height;
	unsigned int fps;
	unsigned int luma_pitch;
	unsigned int chroma_pitch;
	struct isp_stream_info *sif;
	int ret;

	if ((isp_context == NULL) || (cam_id >= CAMERA_ID_MAX)
	|| (stream_type > STREAM_ID_NUM) || (value == NULL)) {
		ISP_PR_ERR("bad para, isp[%p] cid[%d] sid[%d]\n",
			isp_context, cam_id, stream_type);
		return RET_FAILURE;
	}
	width = value->width;
	height = value->height;
	fps = value->fps;
	luma_pitch = abs(value->luma_pitch);
	chroma_pitch = abs(value->chroma_pitch);

	if ((width == 0) || (height == 0) || (luma_pitch == 0)) {
		ISP_PR_ERR("bad para, cid[%d] sid[%d] w:h:p=%d:%d:%d\n",
			cam_id, stream_type, width, height, luma_pitch);
		return RET_FAILURE;
	};

	sif = &isp_context->sensor_info[cam_id].str_info[stream_type];

	ISP_PR_DBG("cid[%d] sid[%d] lp[%d] cp[%d] w:h=%d:%d\n",
		cam_id, stream_type,
		luma_pitch, chroma_pitch, width, height);
	ISP_PR_DBG("fps[%d] streamStatus[%u] chanStatus[%u]\n", fps,
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
	} else if (sif->start_status == START_STATUS_STARTING) {
		sif->width = width;
		sif->height = height;
		sif->fps = fps;
		sif->luma_pitch_set = luma_pitch;
		sif->chroma_pitch_set = chroma_pitch;

		ret = isp_setup_output(isp_context, cam_id);
		if (ret == RET_SUCCESS) {
			ISP_PR_INFO("setup output suc\n");
			ret = RET_SUCCESS;
			goto quit;
		}

		ISP_PR_ERR("failed to setup out\n");
		ret = RET_FAILURE;
		goto quit;
	} else {
		if ((sif->width != width) || (sif->height != height)
		|| (sif->fps != fps) || (sif->luma_pitch_set != luma_pitch)
		|| (sif->chroma_pitch_set != chroma_pitch)) {
			ISP_PR_ERR("failed for non-consis\n");
			ret = RET_FAILURE;
			goto quit;
		}

		ISP_PR_WARN("suc, do none\n");
		ret = RET_SUCCESS;
		goto quit;
	}
 quit:

	return ret;
};

int is_camera_started(struct isp_context *isp_context, enum camera_id cam_id)
{
	struct isp_pwr_unit *pwr_unit;

	if ((isp_context == NULL) || (cam_id >= CAMERA_ID_MAX)) {
		ISP_PR_ERR("bad para cid[%d]\n", cam_id);
		return 0;
	};
	pwr_unit = &isp_context->isp_pu_cam[cam_id];
	if (pwr_unit->pwr_status == ISP_PWR_UNIT_STATUS_ON)
		return 1;
	else
		return 0;
}

void isp_take_back_metadata_buf(struct isp_context *isp, enum camera_id cam_id)
{
	struct sys_img_buf_handle *orig_buf = NULL;
	struct isp_mapped_buf_info *isp_general_img_buf;

	if (!is_para_legal(isp, cam_id)) {
		ISP_PR_ERR("bad para cid[%d]\n", cam_id);
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

struct isp_mapped_buf_info *isp_map_sys_2_mc(
	struct isp_context __maybe_unused *isp,
	struct sys_img_buf_handle *sys_img_buf_hdl,
	unsigned int mc_align,
	unsigned short __maybe_unused cam_id, unsigned short str_id,
	unsigned int y_len, unsigned int u_len, unsigned int v_len)
{
	struct isp_mapped_buf_info *new_buffer;
	struct sys_img_buf_handle *hdl = sys_img_buf_hdl;
	unsigned int mc_total_len;
	unsigned long long y_mc;
	unsigned long long u_mc;
	unsigned long long v_mc;
	void __maybe_unused *vaddr = NULL;

	new_buffer = isp_sys_mem_alloc(sizeof(struct isp_mapped_buf_info));
	if (new_buffer == NULL) {
		ISP_PR_ERR("failed to alloc buf\n");
		return NULL;
	};
	memset(new_buffer, 0, sizeof(struct isp_mapped_buf_info));
	new_buffer->sys_img_buf_hdl = hdl;
	new_buffer->camera_id = (unsigned char) cam_id;
	new_buffer->str_id = (unsigned char) str_id;

	mc_total_len = y_len + u_len + v_len;

	y_mc = hdl->physical_addr;
	vaddr = hdl->virtual_addr;

	u_mc = y_mc + y_len;
	v_mc = u_mc + u_len;
	new_buffer->y_map_info.len = y_len;
	new_buffer->y_map_info.mc_addr = y_mc;
	new_buffer->y_map_info.sys_addr = (unsigned long long) vaddr;
	if (u_len) {
		new_buffer->u_map_info.len = u_len;
		new_buffer->u_map_info.mc_addr = u_mc;
		new_buffer->u_map_info.sys_addr =
			(unsigned long long) vaddr + y_len;
	};
	if (v_len) {
		new_buffer->v_map_info.len = v_len;
		new_buffer->v_map_info.mc_addr = v_mc;
		new_buffer->v_map_info.sys_addr = (unsigned long long) vaddr +
			y_len + u_len;
	}

	return new_buffer;
}

void isp_unmap_sys_2_mc(struct isp_context __maybe_unused *isp,
			struct isp_mapped_buf_info *buff)
{
	ENTER();

	if (buff == NULL) {
		ISP_PR_ERR("buff is NULL\n");
		return;
	}

	if (buff->sys_img_buf_hdl == NULL) {
		ISP_PR_ERR("sys_img_buf_hdl is NULL\n");
		return;
	}

	ISP_PR_WARN("do nothing yet\n");
}


struct sys_img_buf_handle *sys_img_buf_handle_cpy(
		struct sys_img_buf_handle *hdl_in)
{
	struct sys_img_buf_handle *ret;

	if (hdl_in == NULL)
		return NULL;

	ret = isp_sys_mem_alloc(sizeof(struct sys_img_buf_handle));

	if (ret) {
		memcpy(ret, hdl_in, sizeof(struct sys_img_buf_handle));
	} else {
		ISP_PR_ERR("failed to alloc buf");
	};

	return ret;
};

void sys_img_buf_handle_free(struct sys_img_buf_handle *hdl)
{
	if (hdl)
		isp_sys_mem_free(hdl);
};

unsigned int isp_calc_total_mc_len(unsigned int y_len,
		unsigned int u_len, unsigned int v_len,
		unsigned int mc_align, unsigned int *u_offset,
		unsigned int *v_offset)
{
	unsigned int end = 0;

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

int isp_get_res_fps_id_for_str(struct isp_context *isp,
				enum camera_id cam_id, enum stream_id str_id)
{
	unsigned int width;
	unsigned int height;
	unsigned int fps;
	unsigned int i;

	if ((isp == NULL) || (cam_id >= CAMERA_ID_MAX)
	|| (str_id > STREAM_ID_NUM)) {
		ISP_PR_ERR("bad para, return\n");
		return -1;
	}
	if (!isp->snr_res_fps_info_got[cam_id]) {
		ISP_PR_ERR("snr_res_fps_info_got is null, return");
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
				ISP_PR_INFO("suc, return[%d]", i);
				return i;
			}
		}
	}

	ISP_PR_ERR("not found\n");
	return -1;
};

int isp_snr_meta_data_init(struct isp_context *isp)
{
	enum camera_id cam_id;

	if (isp == NULL) {
		ISP_PR_INFO("bad para, return failure\n");
		return RET_FAILURE;
	}

	for (cam_id = CAMERA_ID_REAR; cam_id < CAMERA_ID_MAX; cam_id++) {
		isp_list_init(&isp->sensor_info[cam_id].meta_data_free);
		isp_list_init(&isp->sensor_info[cam_id].meta_data_in_fw);
	}

	return RET_SUCCESS;
};

int isp_snr_meta_data_uninit(struct isp_context *isp)
{
	if (isp == NULL) {
		ISP_PR_INFO("bad para, return failure\n");
		return RET_FAILURE;
	}

	return RET_SUCCESS;
};

int isp_set_script_to_fw(struct isp_context *isp, enum camera_id cid)
{
	unsigned int sensor_len;
	unsigned int vcm_len;
	unsigned int flash_len;
	unsigned char *buf = NULL;
	struct _isp_fw_script_ctrl_t ctrl = {0};
	int result;
//	malloc buffer for this, because it should be sent to firmware
//	struct _instr_seq_t bin;
	struct _CmdSetDeviceScript_t *script_cmd;
//	struct isp_mapped_buf_info *script_bin_buf = NULL;

	ENTER();
	if (!is_para_legal(isp, cid)) {
		ISP_PR_ERR("cid[%d] bad para, return failure\n", cid);
		return RET_FAILURE;
	}

	if ((cid == CAMERA_ID_MEM)
	|| (isp->sensor_info[cid].cam_type == CAMERA_TYPE_MEM)) {
		ISP_PR_WARN("cid[%d] no need for mem camera, skip\n", cid);
		return RET_SUCCESS;
	}

	if ((isp->p_sensor_ops == NULL)
/*	|| (isp->p_vcm_ops == NULL)
 *	|| (isp->p_flash_ops == NULL)
 */	|| (isp->fw_parser[cid] == NULL)
	|| (isp->fw_parser[cid]->parse == NULL)) {
		ISP_PR_ERR("failed for no ops\n");
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
		ISP_PR_ERR("failed for no fw script func\n");
		return RET_FAILURE;
	}

	script_cmd = &isp->sensor_info[cid].script_cmd;
	script_cmd->sensorId = cid;
	result = isp->p_sensor_ops[cid]->get_fw_script_len(cid, &sensor_len);
	if (result != RET_SUCCESS) {
		ISP_PR_ERR("get_fw_script_len failed\n");
		goto fail;
	};
	vcm_len = 0;
	flash_len = 0;
	buf = isp_sys_mem_alloc(sensor_len + vcm_len + flash_len);
	if (buf == NULL) {
		ISP_PR_ERR("isp_sys_mem_alloc failed\n");
		goto fail;
	};


	ctrl.script_buf = buf;
	ctrl.length = sensor_len;
	result = isp->p_sensor_ops[cid]->get_fw_script_ctrl(cid, &ctrl);
	if (result != RET_SUCCESS) {
		ISP_PR_ERR("get_fw_script_ctrl failed\n");
		goto fail;
	};

	result =
		isp->fw_parser[cid]->parse(buf,
			sensor_len + vcm_len + flash_len,
			(struct _InstrSeq_t *)
			&script_cmd->deviceScript.instrSeq);
	if (result != RET_SUCCESS) {
		ISP_PR_ERR("fw_parse failed\n");
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
		ISP_PR_ERR("invalid cid[%d]\n", cid);
		goto fail;
	}
	result = isp_fw_set_script(isp, cid, script_cmd);
	if (result != RET_SUCCESS) {
		ISP_PR_ERR
			("isp_fw_set_script failed\n");
		goto fail;
	};

	if (buf)
		isp_sys_mem_free(buf);

	RET(RET_SUCCESS);
	return RET_SUCCESS;
 fail:
	if (buf)
		isp_sys_mem_free(buf);

	RET(RET_FAILURE);
	return RET_FAILURE;
}

unsigned char g_fw_log_buf[ISP_FW_TRACE_BUF_SIZE + 32];
#define MAX_ONE_TIME_LOG_INFO_LEN 510
void isp_fw_log_print(struct isp_context *isp)
{
	unsigned int cnt;

	if (isp->drv_settings.fw_log_cfg_en == 0 || ISP_DEBUG_LEVEL < TRACE_LEVEL_ERROR)
		return;

	isp_mutex_lock(&isp->command_mutex);
	cnt = isp_fw_get_fw_rb_log(isp, g_fw_log_buf);
	isp_mutex_unlock(&isp->command_mutex);

	if (cnt) {
		unsigned int len = 0;
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

					ISP_PR_INFO_FW("%s", str);
				}
				break;
			}

			{
				unsigned int tmp_len =
					MAX_ONE_TIME_LOG_INFO_LEN - 4;

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
					unsigned int len1 = tmp_len - 1;

					while ('\n' == str[len1]) {
						str[len1] = ' ';
						len1--;
					}

					ISP_PR_INFO_FW("%s", str);
				}
				str[tmp_len] = temp_ch;
				str = &str[tmp_len];
			}
		}
	}
}

void isp_fw_resp_cmd_done_extra(struct isp_context *isp,
				enum camera_id cid, struct _RespCmdDone_t *para,
				struct isp_cmd_element *ele)
{
	unsigned int major;
	unsigned int minor;
	unsigned int rev;
	unsigned int rev1;
	unsigned int *ver;
	unsigned char *payload;
	//AeIsoCapability_t *iso_cap;
	//AeEvCapability_t *ev_cap;
	struct sensor_info *snr_info;

	if ((isp == NULL) || (para == NULL) || (ele == NULL)) {
		ISP_PR_ERR("bad para, return\n");
		return;
	}

	payload = para->payload;
	if (payload == NULL) {
		ISP_PR_ERR("payload null pointer, return\n");
		return;
	}

	switch (para->cmdId) {
	case CMD_ID_GET_FW_VERSION:
		ver = (unsigned int *) payload;
		major = (*ver) >> 24;
		minor = ((*ver) >> 16) & 0xff;
		rev = ((*ver) >> 8) & 0xff;
		rev1 = (*ver) & 0xff;

		isp->isp_fw_ver = *ver;
		ISP_PR_PC("ISP FW Version[%u:%u:%u]\n", major, minor, rev);
		break;
	case CMD_ID_START_STREAM:
		ISP_PR_INFO("fw start stream rsp to host\n");
		if (g_prop && g_prop->cvip_enable == CVIP_STATUS_ENABLE) {
			ISP_PR_DBG("cid[%u] isp_snr_stream_on CVIP\n", cid);
		} else {
			if (!isp_is_mem_camera(isp, cid) &&
			    isp_snr_stream_on(isp, cid) != RET_SUCCESS) {
				ISP_PR_ERR("cid[%u] isp_snr_stream_on failed\n",
					   cid);
				}
		}

		if (isp->drv_settings.ir_illuminator_ctrl == 1)
			isp_turn_on_ir_illuminator(isp, cid);

		break;
#if	defined(PER_FRAME_CTRL)
	case CMD_ID_AE_GET_ISO_CAPABILITY:
		iso_cap = (AeIsoCapability_t *) payload;
		if (is_para_legal(isp, cid))
			isp->isp_cap.iso[cid] = *iso_cap;
		ISP_PR_INFO("iso capability cid %d,min %u, max %u\n",
			cid, iso_cap->minIso.iso, iso_cap->maxIso.iso);
		break;
#endif
#if	defined(PER_FRAME_CTRL)
	case CMD_ID_AE_GET_EV_CAPABILITY:
		ev_cap = (AeEvCapability_t *) payload;
		if (is_para_legal(isp, cid))
			isp->isp_cap.ev_compensation[cid] = *ev_cap;
		ISP_PR_INFO
		("ev capability cid%d,min %u/%u,max %u/%u,step %u/%u\n", cid,
			ev_cap->minEv.numerator, ev_cap->minEv.denominator,
			ev_cap->maxEv.numerator, ev_cap->maxEv.denominator,
			ev_cap->stepEv.numerator, ev_cap->stepEv.denominator);
		break;
#endif
	case CMD_ID_SEND_I2C_MSG:
		snr_info = &isp->sensor_info[ele->cam_id];
		if (snr_info == NULL) {
			ISP_PR_ERR("snr_info null pointer\n");
			break;
		}
		if (snr_info->iic_tmp_buf == NULL
		|| snr_info->iic_tmp_buf->sys_addr == NULL) {
			ISP_PR_ERR("iic_tmp_buf null pointer\n");
			break;
		}

		if (ele->I2CRegAddr == I2C_REGADDR_NULL) {
			ISP_PR_ERR("invalid regaddr\n");
			break;
		}

		if (ele->I2CRegAddr == snr_info->fps_tab.HighAddr
/*		|| ele->I2CRegAddr == snr_info->fps_tab.LowAddr */
		) {
			unsigned char temp[2] = { 0 };

			memcpy(&temp, snr_info->iic_tmp_buf->sys_addr, 2);
			ISP_PR_DBG("i2c read: %x, %x\n", temp[0], temp[1]);
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
				snr_info->fps_tab.HighValue = 0;
				snr_info->fps_tab.LowValue = 0;
			}
		} else {
			ISP_PR_ERR("i2c_msg invalid response\n");
		}
		break;
	default:
		break;
	};
}

void isp_fw_resp_cmd_done(struct isp_context *isp,
			  enum fw_cmd_resp_stream_id stream_id,
			  struct _RespCmdDone_t *para)
{
	struct isp_cmd_element *ele;
	enum camera_id cid;
	struct cmd_done_cb_para cb = { 0 };

	cid = isp_get_cid_from_stream_id(isp, stream_id);
	ele = isp_rm_cmd_from_cmdq(isp, para->cmdSeqNum, para->cmdId, false);

	if (ele == NULL) {
		ISP_PR_ERR("stream[%d] cmd %s(0x%08x)(%d) seq[%u], no orig\n",
			stream_id, isp_dbg_get_cmd_str(para->cmdId),
			para->cmdId, para->cmdStatus, para->cmdSeqNum);
		return;
	}

	if (ele->resp_payload && ele->resp_payload_len) {
		unsigned int tempi = *ele->resp_payload_len;
		unsigned int tempj = 36;
		*ele->resp_payload_len =
			min(tempi, tempj);
		memcpy(ele->resp_payload, para->payload,
		       *ele->resp_payload_len);
	};

	ISP_PR_DBG("stream[%d] cmd %s(0x%08x)(%d) seq[%u] cid[%u]\n",
		   stream_id, isp_dbg_get_cmd_str(para->cmdId),
		   para->cmdId, para->cmdStatus, para->cmdSeqNum, cid);

	if (para->cmdStatus == 0)
		isp_fw_resp_cmd_done_extra(isp, cid, para, ele);
	if (ele->evt) {
		ISP_PR_DBG("signal event %p\n", ele->evt);
		isp_event_signal(para->cmdStatus, ele->evt);
	}
	cb.status = para->cmdStatus;
	if (cid >= CAMERA_ID_MAX) {
		if (stream_id != FW_CMD_RESP_STREAM_ID_GLOBAL)
			ISP_PR_ERR("failed cid[%d] sid[%u]\n", cid, stream_id);
	}
#if	defined(PER_FRAME_CTRL)
	else if ((para->cmdId == CMD_ID_SEND_FRAME_INFO)
		|| (para->cmdId == CMD_ID_SEND_FRAME_INFO)) {
		struct isp_mapped_buf_info *raw = NULL;
		enum camera_id cb_cid = cid;

		cb.cam_id = cid;
		//to do
		//if (CMD_ID_SEND_FRAME_INFO == para->cmdId)
		//	cb_cid = isp_get_rgbir_cid(isp);
		if (para->cmdId == CMD_ID_SEND_FRAME_INFO) {
			cb.cmd = ISP_CMD_ID_SET_RGBIR_INPUT_BUF;
			raw = (struct isp_mapped_buf_info *)
			isp_list_get_first
				(&isp->sensor_info[cid].rgbraw_input_in_fw);
		} else {
			cb.cmd = ISP_CMD_ID_SET_MEM_CAMERA_INPUT_BUF;
			raw = (struct isp_mapped_buf_info *)isp_list_get_first
				(&isp->sensor_info[cid].irraw_input_in_fw);
		}

		if (raw) {
			cb.para.buf = *raw->sys_img_buf_hdl;
			if (cb_cid < CAMERA_ID_MAX && isp->evt_cb[cb_cid])
				isp->evt_cb[cb_cid]
					(isp->evt_cb_context[cb_cid],
					CB_EVT_ID_CMD_DONE, &cb);
			else
				ISP_PR_ERR("failed empty cb cid[%u]", cb_cid);
			isp_unmap_sys_2_mc(isp, raw);
			if (raw->sys_img_buf_hdl)
				isp_sys_mem_free(raw->sys_img_buf_hdl);
			isp_sys_mem_free(raw);
		} else if (cb_cid < CAMERA_ID_MAX) {
			//rgbir case not loopback
			ISP_PR_WARN("no raw,cid[%u]\n", cid);
		};
	}
#endif
	if (ele->mc_addr)
		isp_fw_ret_pl(isp->fw_work_buf_hdl, ele->mc_addr);

	if (ele->gpu_pkg)
		isp_gpu_mem_free(ele->gpu_pkg);

	isp_sys_mem_free(ele);
}

enum raw_image_type get_raw_img_type(enum _ImageFormat_t type)
{
	switch (type) {
	case IMAGE_FORMAT_RGBBAYER8:
		return RAW_IMAGE_TYPE_8BIT;
	case IMAGE_FORMAT_RGBBAYER10:
		return RAW_IMAGE_TYPE_10BIT;
	case IMAGE_FORMAT_RGBBAYER12:
		return RAW_IMAGE_TYPE_12BIT;
	default:
		ISP_PR_ERR("invalid type %d\n", type);
		return RAW_IMAGE_TYPE_IVALID;
	}
}

void isp_fw_resp_frame_ctrl_done(struct isp_context *isp,
				 enum fw_cmd_resp_stream_id stream_id,
				 struct _RespParamPackage_t *para)
{
	enum camera_id cid;
	unsigned long long mc;
	struct sensor_info *sif;

	cid = isp_get_cid_from_stream_id(isp, stream_id);
	sif = &isp->sensor_info[cid];
	ISP_PR_INFO("enter, cid[%d]", cid);
	if (cid == CAMERA_ID_MAX) {
		ISP_PR_ERR("fail, bad cid");
		return;
	}

	mc = isp_join_addr64(para->packageAddrLo, para->packageAddrHi);
	if (mc == 0) {
		ISP_PR_ERR("bad metainfo mc, cam[%d] mc[%llx]", cid, mc);
		return;
	}

#if	defined(RAW_DUMP)
	if (sif->dump_raw_enable == 1) {
		if (sif->dump_raw_count < MAX_DUMP_RAW_COUNT) {
			struct file *fp[MAX_DUMP_RAW_COUNT];
			char str[50] = {0};
			ulong len;
			unsigned int raw_height;
			unsigned int raw_weight;

			raw_height = sif->raw_height;
			raw_weight = sif->raw_width;

			raw_height -= (raw_height % 8);
			raw_weight -= (raw_weight % 8);

			len = raw_height * raw_weight * 2;
			sprintf(str,
				"/storage/emulated/0/DCIM/raw%d_%dx%d.raw",
				sif->dump_raw_count, raw_weight, raw_height);
			fp[sif->dump_raw_count] =
				filp_open(str, O_RDWR  | O_CREAT, 0666);
			if (IS_ERR(fp[sif->dump_raw_count])) {
				ISP_PR_ERR
					("file open failed\n");
			} else {
				isp_write_file_test(fp[sif->dump_raw_count],
			sif->stream_tmp_buf[sif->dump_raw_count % 4]->sys_addr,
					&len);
				ISP_PR_ERR("success to save file %s\n", str);
			}
			sif->dump_raw_count++;

			//comment here for save crash
			//filp_open(str);
		}
	}
#endif
	if (isp->evt_cb[cid])
		isp->evt_cb[cid](NULL, CB_EVT_ID_FRAME_CTRL_DONE, (void *)mc,
				 cid);
	else
		ISP_PR_ERR("fail empty cb for cid[%u]", cid);
}

void isp_fw_resp_frame_info(struct isp_context *isp,
			enum fw_cmd_resp_stream_id stream_id,
			struct _RespParamPackage_t *para)
{
	unsigned long long mc;
	enum camera_id cid;
	struct isp_mapped_buf_info *rgbir_output = NULL;
	struct frame_info_cb_para cb = { 0 };
	struct sensor_info *sif;

	ENTER();
	cid = isp_get_cid_from_stream_id(isp, stream_id);
	if (cid == CAMERA_ID_MAX) {
		ISP_PR_ERR("invalid cid[%d] from sid[%d]\n", cid, stream_id);
		return;
	};

	sif = &isp->sensor_info[cid];
	if (!isp_is_host_processing_raw(isp, cid)) {
		ISP_PR_ERR("failed, cid[%u] type[%u] sid[%d]\n",
			cid, sif->cam_type, stream_id);
		return;
	};
	mc = isp_join_addr64(para->packageAddrLo, para->packageAddrHi);

	rgbir_output = (struct isp_mapped_buf_info *) isp_list_get_first
		(&isp->sensor_info[cid].rgbir_raw_output_in_fw);

	if (rgbir_output == NULL) {
		ISP_PR_ERR("output null, cid[%u] sid[%d]\n", cid, stream_id);
		return;
	} else if (rgbir_output->sys_img_buf_hdl == NULL) {
		ISP_PR_ERR("buf hdl null, cid[%u] streamid[%d]\n",
			cid, stream_id);
		return;
	} else if ((rgbir_output->sys_img_buf_hdl->virtual_addr == NULL)
	|| (rgbir_output->sys_img_buf_hdl->len == 0)) {
		ISP_PR_ERR("invalid len, cid[%u] sid[%d]\n",
			cid, stream_id);
		return;
	}

	if (rgbir_output->v_map_info.mc_addr != mc) {
		ISP_PR_ERR("invalid mc, cid[%u] sid[%d]\n",
			cid, stream_id);
	};
	ISP_PR_DBG("cid[%d] poc[%u] sid[%d] raw cnt[%u]\n", cid,
	rgbir_output->v_map_info.sys_addr ?   ((struct _FrameInfo_t *)
	(unsigned long long)rgbir_output->v_map_info.sys_addr)->poc : 0,
	stream_id, --g_rgbir_raw_count_in_fw);

	ISP_PR_DBG("va[%p][%u] mc[%llx][%u:%u:%u]\n",
		rgbir_output->sys_img_buf_hdl->virtual_addr,
		rgbir_output->sys_img_buf_hdl->len,
		rgbir_output->y_map_info.mc_addr,
		rgbir_output->y_map_info.len,
		rgbir_output->u_map_info.len,
		rgbir_output->v_map_info.len);

	if (isp_is_fpga()) {
		isp_hw_mem_read_(rgbir_output->y_map_info.mc_addr,
			rgbir_output->sys_img_buf_hdl->virtual_addr,
			rgbir_output->sys_img_buf_hdl->len);
	}

	cb.cam_id = cid;
	cb.buf = *rgbir_output->sys_img_buf_hdl;
	cb.bayer_raw_buf_len = rgbir_output->y_map_info.len;
	cb.ir_raw_buf_len = rgbir_output->u_map_info.len;
	cb.frame_info_buf_len = rgbir_output->v_map_info.len;
	isp->evt_cb[cid] (isp->evt_cb_context[cid],
			CB_EVT_ID_FRAME_INFO, &cb, cid);

	if (rgbir_output) {
		isp_unmap_sys_2_mc(isp, rgbir_output);

		if (rgbir_output->sys_img_buf_hdl)
			isp_sys_mem_free(rgbir_output->sys_img_buf_hdl);
		isp_sys_mem_free(rgbir_output);
	};

	EXIT();
}

void isp_fw_resp_error(struct isp_context __maybe_unused *isp,
	enum fw_cmd_resp_stream_id __maybe_unused stream_id,
	struct _RespError_t __maybe_unused *para)
{
}

void isp_fw_resp_heart_beat(struct isp_context __maybe_unused *isp)
{
};

void isp_fw_resp_func(struct isp_context *isp,
			enum fw_cmd_resp_stream_id stream_id)
{
	int result;
	struct _Resp_t resp;

	if (ISP_GET_STATUS(isp) < ISP_STATUS_FW_RUNNING)
		return;
	isp_fw_log_print(isp);
	result = isp_get_f2h_resp(isp, stream_id, &resp);
	if (result != RET_SUCCESS)
		return;
	switch (resp.respId) {
	case RESP_ID_CMD_DONE:
		isp_fw_resp_cmd_done(isp, stream_id,
			(struct _RespCmdDone_t *) resp.respParam);
		return;
	case RESP_ID_FRAME_DONE:
		isp_fw_resp_frame_ctrl_done(isp, stream_id,
			(struct _RespParamPackage_t *) resp.respParam);
		return;
	case RESP_ID_FRAME_INFO:
		isp_fw_resp_frame_info(isp, stream_id,
			(struct _RespParamPackage_t *) resp.respParam);
		return;
	case RESP_ID_ERROR:
		isp_fw_resp_error(isp, stream_id,
			(struct _RespError_t *) resp.respParam);
		return;

	//case RESP_ID_HEART_BEAT:
		//isp_fw_resp_heart_beat(isp);
		//return;

	default:
		ISP_PR_ERR("invalid respid %s[0x%x]\n",
			isp_dbg_get_resp_str(resp.respId), resp.respId);
		break;
	}
};

int isp_fw_resp_thread_wrapper(void *context)
{
	struct isp_fw_resp_thread_para *para = context;
	struct isp_context *isp;
	enum fw_cmd_resp_stream_id stream_id;

	if (para == NULL) {
		ISP_PR_ERR("bad para\n");
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
		ISP_PR_ERR("invalid para idx[%u]\n", para->idx);
		goto quit;
	};

	isp = para->isp;
	ISP_PR_DBG("para idx[%u] started\n", para->idx);

	while (true) {
		isp_event_wait(&isp->fw_resp_thread[para->idx].wakeup_evt,
			WORK_ITEM_INTERVAL);

		if (thread_should_stop(&isp->fw_resp_thread[para->idx])) {
			ISP_PR_INFO("para idx[%u] thread quit\n", para->idx);
			break;
		};

		isp_mutex_lock(&isp->fw_resp_thread[para->idx].mutex);
		isp_fw_resp_func(isp, stream_id);
		isp_mutex_unlock(&isp->fw_resp_thread[para->idx].mutex);
	}
 quit:
	return 0;
};

unsigned int isp_get_stream_output_bits(struct isp_context *isp,
				enum camera_id cam_id, unsigned int *stream_cnt)
{
	unsigned int ret = 0;
	unsigned int cnt = 0;
	enum start_status stat;

	if (stream_cnt)
		*stream_cnt = 0;
	if (!is_para_legal(isp, cam_id)) {
		ISP_PR_ERR("failed for bad para");
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


	if (stream_cnt)
		*stream_cnt = cnt;

	return ret;
}

unsigned int isp_get_started_stream_count(struct isp_context *isp)
{
	unsigned int cnt = 0;
	enum camera_id cid;

	if (isp == NULL)
		return 0;
	for (cid = CAMERA_ID_REAR; cid < CAMERA_ID_MAX; cid++) {
		if (isp->sensor_info[cid].status == START_STATUS_STARTED)
			cnt++;
	};

	return cnt;
}

int isp_fw_work_mem_rsv(struct isp_context *isp,
			unsigned long long *sys_addr,
			unsigned long long *mc_addr)
{
	cgs_handle_t handle = 0;
	int ret;
	long long max_quota = 4;

	ENTER();
	max_quota *= 1024 * 1024 * 1024;
	if (isp == NULL) {
		ISP_PR_ERR("invalid para\n");
		return RET_INVALID_PARAM;
	};

	memset(&isp->fw_work_buf, 0, sizeof(struct isp_gpu_mem_info));
	ret = g_cgs_srv->ops->alloc_gpu_mem(g_cgs_srv,
			CGS_GPU_MEM_TYPE__GART_WRITECOMBINE,
			ISP_FW_WORK_BUF_SIZE, ISP_MC_ADDR_ALIGN, 0,
			max_quota,
			&handle);
	if (ret != 0) {
		ISP_PR_ERR("failed for alloc_gpu_mem\n");
		handle = 0;
		goto fail;
	};

	ret = g_cgs_srv->ops->gmap_gpu_mem(g_cgs_srv, handle,
				&isp->fw_work_buf.gpu_mc_addr);
	if (ret != 0) {
		ISP_PR_ERR("failed for gmap_gpu_mem\n");
		isp->fw_work_buf.gpu_mc_addr = 0;
		goto fail;
	};

	ret = g_cgs_srv->ops->kmap_gpu_mem(g_cgs_srv, handle,
					&isp->fw_work_buf.sys_addr);
	if (ret != 0) {
		ISP_PR_ERR("failed for kmap_gpu_mem\n");
		isp->fw_work_buf.sys_addr = NULL;
		goto fail;
	};

	if (sys_addr)
		*sys_addr = (unsigned long long) isp->fw_work_buf.sys_addr;
	if (mc_addr)
		*mc_addr = isp->fw_work_buf.gpu_mc_addr;
	isp->fw_work_buf.handle = handle;

	RET(RET_SUCCESS);
	return RET_SUCCESS;
fail:
	if (handle && isp->fw_work_buf.sys_addr)
		g_cgs_srv->ops->kunmap_gpu_mem(g_cgs_srv, handle);
	if (handle && isp->fw_work_buf.gpu_mc_addr)
		g_cgs_srv->ops->gunmap_gpu_mem(g_cgs_srv, handle);
	if (handle)
		g_cgs_srv->ops->free_gpu_mem(g_cgs_srv, handle);
	isp->fw_work_buf.handle = 0;

	RET(RET_FAILURE);
	return RET_FAILURE;
}


int isp_fw_work_mem_free(struct isp_context *isp)
{
	cgs_handle_t handle = 0;

	ENTER();
	if (isp == NULL) {
		ISP_PR_INFO("failed by invalid para\n");
		return RET_INVALID_PARAM;
	};

	handle = isp->fw_work_buf.handle;
	if (handle && isp->fw_work_buf.sys_addr)
		g_cgs_srv->ops->kunmap_gpu_mem(g_cgs_srv, handle);
	if (handle && isp->fw_work_buf.gpu_mc_addr)
		g_cgs_srv->ops->gunmap_gpu_mem(g_cgs_srv, handle);
	if (handle)
		g_cgs_srv->ops->free_gpu_mem(g_cgs_srv, handle);
	isp->fw_work_buf.handle = 0;

	RET(RET_SUCCESS);
	return RET_SUCCESS;
};


void init_cam_calib_item(struct isp_context *isp, enum camera_id cid)
{
	unsigned int i;

	for (i = 0; i < MAX_CALIB_ITEM_COUNT; i++)
		isp->calib_items[cid][i] = NULL;
};

void uninit_cam_calib_item(struct isp_context *isp, enum camera_id cid)
{
	unsigned int i;

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

bool calib_crc_check(struct calib_pkg_td_header __maybe_unused *pkg)
{
	return true;
}

struct calib_date_item *isp_get_calib_item(struct isp_context *isp,
					enum camera_id cid)
{
	struct calib_date_item *item;
	struct res_fps_t *res_fps;
	unsigned int i;

	ENTER();
	if (!is_para_legal(isp, cid)) {
		ISP_PR_ERR("failed by bad para, isp[%p] cid[%d]\n",
			isp, cid);
		return NULL;
	};
	res_fps =
		&isp->sensor_info[cid].res_fps.res_fps[
		(int)(isp->sensor_info[cid].cur_res_fps_id)];

	ISP_PR_DBG("cid[%d] w:h=%u:%u fps[%u] hdr[%u]\n", cid,
		res_fps->width, res_fps->height, res_fps->fps,
		isp->sensor_info[cid].hdr_enable);

	for (i = 0; i < MAX_CALIB_ITEM_COUNT; i++) {
		item = isp->calib_items[cid][i];
		if (item == NULL) {
			break;
		};

		if ((res_fps->height == item->height) &&
		(res_fps->width == item->width) &&
		(res_fps->fps == item->fps) &&
		((isp->sensor_info[cid].hdr_enable && item->hdr) ||
		(!isp->sensor_info[cid].hdr_enable && !item->hdr))) {
			ISP_PR_DBG("1th suc, return item\n");
			return item;
		};
	};

	for (i = 0; i < MAX_CALIB_ITEM_COUNT; i++) {
		item = isp->calib_items[cid][i];
		if (item == NULL) {
			break;
		};
		if ((res_fps->height > item->height) &&
		(res_fps->width > item->width) &&
		(res_fps->fps == item->fps) &&
		((isp->sensor_info[cid].hdr_enable && item->hdr) ||
		(!isp->sensor_info[cid].hdr_enable && !item->hdr))) {
			ISP_PR_DBG("2th suc, return item\n");
			return item;
		};
	};

	for (i = 0; i < MAX_CALIB_ITEM_COUNT; i++) {
		item = isp->calib_items[cid][i];
		if (item == NULL) {
			break;
		};

		if ((res_fps->height >= item->height) &&
		(res_fps->width >= item->width) &&
		((isp->sensor_info[cid].hdr_enable && item->hdr) ||
		(!isp->sensor_info[cid].hdr_enable && !item->hdr))) {
			ISP_PR_DBG("3th suc, return item\n");
			return item;
		};
	};

	for (i = 0; i < MAX_CALIB_ITEM_COUNT; i++) {
		item = isp->calib_items[cid][i];
		if (item == NULL) {
			break;
		};

		if ((res_fps->height >= item->height) &&
		(res_fps->width >= item->width) &&
		(res_fps->fps == item->fps)) {
			ISP_PR_DBG("4th suc, return item\n");
			return item;
		};
	};

	for (i = 0; i < MAX_CALIB_ITEM_COUNT; i++) {
		item = isp->calib_items[cid][i];
		if (item == NULL) {
			break;
		};

		if ((res_fps->height >= item->height) &&
		(res_fps->width >= item->width)) {
			ISP_PR_DBG("5th suc, return item\n");
			return item;
		};
	}

	item = isp->calib_items[cid][0];
	if (item == NULL) {
		ISP_PR_ERR("cid[%d] item not found\n", cid);
		return item;
	}

	ISP_PR_DBG("no match cid[%d] %p[%u] w:h=%u:%u fps[%u] hdr[%u]\n",
		cid, item->data, item->len, item->width, item->height,
		item->fps, item->hdr);

	EXIT();
	return item;
}

void add_calib_item(struct isp_context *isp, enum camera_id cid,
			struct _profile_header *profile,
			void *data, unsigned int len)
{
	unsigned int arr_size;
//	struct calib_date_item *cur, *next;
	unsigned int i;
//	unsigned int w, h;
	struct calib_date_item *item;

	ENTER();
	if (profile == NULL)
		return;

	arr_size = MAX_CALIB_ITEM_COUNT;
	for (i = 0; i < arr_size; i++) {
		item = isp->calib_items[cid][i];
		if (item == NULL) {
			item = isp_sys_mem_alloc
				(sizeof(struct calib_date_item));
			if (item == NULL) {
				ISP_PR_ERR("failed for null item\n");
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
			item->expo_limit = profile->expo_limit;
			break;
		};
	}

	ISP_PR_DBG("cid[%u] w:h=%u:%u fps:%u hdr:%u expo:%u buf[%p(%u)]\n",
		   cid, profile->width, profile->height,
		   profile->fps, profile->hdr, profile->expo_limit, data, len);

	if (1) {
		unsigned int *db = (unsigned int *) data;

		ISP_PR_DBG("%08x %08x %08x %08x %08x %08x %08x %08x\n",
		db[0], db[1], db[2], db[3], db[4], db[5], db[6], db[7]);
	}

	EXIT();
}


unsigned char parse_calib_data(struct isp_context *isp,
			enum camera_id cam_id,
			unsigned char *data, unsigned int len)
{
	uint32_t tempi;
	struct _package_td_header *ph;
	struct sensor_info *sif;
	struct _profile_header profile;
	unsigned int tempj = MAX_PROFILE_NO;

	if (!is_para_legal(isp, cam_id)) {
		ISP_PR_ERR("bad para, isp[%p] cid[%d]\n", isp, cam_id);
		return false;
	};

	if ((len == 0) || (data == NULL)) {
		ISP_PR_ERR("bad para, cid[%d] len[%d]\n", cam_id, len);
		return false;
	};

	if (isp->calib_data[cam_id])
		isp_sys_mem_free(isp->calib_data[cam_id]);
	isp->calib_data[cam_id] = isp_sys_mem_alloc(len);
	if (isp->calib_data[cam_id] == NULL) {
		ISP_PR_ERR("alloc buf failed, cid[%d] len[%d]\n",
			cam_id, len);
		return false;
	}
	sif = &isp->sensor_info[cam_id];
	memcpy(isp->calib_data[cam_id], data, len);
	data = isp->calib_data[cam_id];
	isp->calib_data_len[cam_id] = len;
	ph = (struct _package_td_header *) data;
//	sif->cproc_value = NULL;
	sif->wb_idx = NULL;
	sif->scene_tbl = NULL;

	if (ph->magic_no_high != 0x33ffffff ||
	    ph->magic_no_low != 0xffffffff) {
		ISP_PR_WARN("old format\n");
		goto old_tuning_data;
	} else {
		ISP_PR_INFO("new format,cropWnd %u:%u\n",
			ph->crop_wnd_width, ph->crop_wnd_height);
	};
	for (tempi = 0; tempi < min(ph->pro_no, tempj); tempi++) {
		uint8_t *tb;

		tb = data + ph->pro[tempi].offset;
		if ((ph->pro[tempi].offset + ph->pro[tempi].size) > len) {
			ISP_PR_DBG("data[%u][%u] should<%u\n", tempi,
				(ph->pro[tempi].offset + ph->pro[tempi].size),
				len);
			goto old_tuning_data;
		};
	};
	for (tempi = 0; tempi < min(ph->pro_no, tempj); tempi++) {
		uint8_t *tb;
		tb = data + ph->pro[tempi].offset;
		add_calib_item(isp, cam_id, &ph->pro[tempi],
			tb, ph->pro[tempi].size);
	}
	//sif->cproc_value = &ph->cproc_value;
	sif->wb_idx = &ph->wb_idx;
	sif->scene_tbl = &ph->scene_mode_table;

	RET(true);
	return true;
 old_tuning_data:
 //construct following specific profiles
 //only 2048x1536@30fps has short exposure, all others are long exposure.
	profile.width = 2048;
	profile.height = 1536;
	profile.fps = 30;
	profile.hdr = 0;
	profile.expo_limit = EXPO_LIMIT_TYPE_SHORT;
	add_calib_item(isp, cam_id, &profile, isp->calib_data[cam_id], len);
	profile.width = 2048;
	profile.height = 1536;
	profile.fps = 30;
	profile.hdr = 0;
	profile.expo_limit = EXPO_LIMIT_TYPE_LONG;
	add_calib_item(isp, cam_id, &profile, isp->calib_data[cam_id], len);
	profile.width = 2048;
	profile.height = 1536;
	profile.fps = 60;
	profile.hdr = 0;
	profile.expo_limit = EXPO_LIMIT_TYPE_LONG;
	add_calib_item(isp, cam_id, &profile, isp->calib_data[cam_id], len);
	profile.width = 4096;
	profile.height = 3072;
	profile.fps = 30;
	profile.hdr = 0;
	profile.expo_limit = EXPO_LIMIT_TYPE_LONG;
	add_calib_item(isp, cam_id, &profile, isp->calib_data[cam_id], len);
	RET(true);
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
}

void init_frame_ctl_cmd(struct sensor_info *sif,
			struct _CmdFrameCtrl_t *f,
			struct frame_ctl_info *ctl_info,
			struct isp_mapped_buf_info *zsl_buf)
{
#if defined(PER_FRAME_CTRL)
	ActionType_t default_act = ACTION_TYPE_USE_STREAM;
	ActionType_t new_act = ACTION_TYPE_USE_STREAM;
	int check_ae_mode = false;
	int check_af_mode = false;
	struct _FrameControl_t *fc;

	unsigned int dataAddrLo;
	unsigned int dataAddrHi;

	if (!f) {
		ISP_PR_ERR("bad para\n");
		return;
	}

	fc = &f->frameCtrl;
	memset(fc, 0, sizeof(struct _CmdFrameCtrl_t));
	fc->controlType = FRAME_CONTROL_TYPE_HALF;

	fc->ae.aeMode.actionType = default_act;
	fc->ae.aeFlickerType.actionType = default_act;
	fc->ae.aeRegion.actionType = default_act;
	fc->ae.aeLock.actionType = default_act;
	fc->ae.aeExposure.actionType = default_act;
	fc->ae.aeIso.actionType = default_act;
	fc->ae.aeEv.actionType = default_act;
	//fc->ae.aeFlashMode.actionType = default_act;
	//fc->ae.aeFlashPowerLevel.actionType = default_act;
	//fc->ae.aeRedEyeMode.actionType = default_act;

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
			ISP_PR_DBG("ae mode[%d]\n", fc->ae.aeMode.mode);
		} else {
			ISP_PR_DBG("ignore ae mode[%d]\n", fc->ae.aeMode.mode);
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
			ISP_PR_DBG("itime[%u]\n", ctl_info->ae_para.itime);
		} else {
			ISP_PR_DBG("ignore itime %u[%u,%u]\n",
				ctl_info->ae_para.itime,
				sif->sensor_cfg.exposure.min_integration_time,
				sif->sensor_cfg.exposure.max_integration_time);
		}
	}

	if (ctl_info->set_flash_mode) {
		if ((ctl_info->flash_mode != FLASH_MODE_INVALID)
		&& (ctl_info->flash_mode != FLASH_MODE_MAX)) {
			//todo no flash
			//fc->ae.aeFlashMode.actionType = new_act;
			//fc->ae.aeFlashMode.flashMode = ctl_info->flash_mode;
			//check_ae_mode = true;
			ISP_PR_DBG("flashMode[%u]\n", ctl_info->flash_mode);
		} else {
			ISP_PR_DBG("ignore flashMode %u\n",
				ctl_info->flash_mode);
		}
	};

	if (ctl_info->set_falsh_pwr) {
		//todo no flash
		//fc->ae.aeFlashPowerLevel.actionType = new_act;
		//fc->ae.aeFlashPowerLevel.powerLevel = ctl_info->flash_pwr;
		check_ae_mode = true;
		ISP_PR_INFO("powerLevel %u\n", ctl_info->flash_pwr);
	};

	if (ctl_info->set_ec) {
		fc->ae.aeEv.actionType = new_act;
		fc->ae.aeEv.ev = ctl_info->ae_ec;
		check_ae_mode = true;
		ISP_PR_INFO("ae_ec %d/%u\n",
			ctl_info->ae_ec.numerator,
			ctl_info->ae_ec.denominator);
	};

	if (ctl_info->set_iso) {
		fc->ae.aeIso.actionType = new_act;
		fc->ae.aeIso.iso = ctl_info->iso;
		check_ae_mode = true;
		ISP_PR_INFO("iso %u\n", ctl_info->iso.iso);
	};

	if (check_ae_mode && (fc->ae.aeMode.actionType == new_act)
	&& (fc->ae.aeMode.mode != AE_MODE_MANUAL)) {
		fc->ae.aeMode.mode = AE_MODE_MANUAL;
		ISP_PR_INFO("change aemode from auto to manual\n");
	};

	if (ctl_info->set_af_mode) {
		if ((ctl_info->af_mode == AF_MODE_INVALID)
		|| (ctl_info->af_mode == AF_MODE_MAX)) {
			ISP_PR_INFO("ignore af_mode %u\n",
				ctl_info->af_mode);
		} else if (sif->lens_range.maxLens == 0) {
			ISP_PR_DBG("ignore af_mode %u for maxlen is 0\n",
				ctl_info->af_mode);
		} else {
			fc->af.afMode.actionType = new_act;
			fc->af.afMode.mode = ctl_info->af_mode;
			ISP_PR_DBG("af_mode %u\n", ctl_info->af_mode);
		}
	};

	if (ctl_info->set_af_lens_pos) {
		if (ctl_info->af_lens_pos
		&& (ctl_info->af_lens_pos >= sif->lens_range.minLens)
		&& (ctl_info->af_lens_pos <= sif->lens_range.maxLens)) {
			fc->af.afLensPos.actionType = new_act;
			fc->af.afLensPos.lensPos = ctl_info->af_lens_pos;
			check_af_mode = true;
			ISP_PR_DBG("lensPos %u\n", ctl_info->af_lens_pos);
		} else {
			ISP_PR_DBG("ignore lens pos %u[%u,%u]\n",
				ctl_info->af_lens_pos,
				sif->lens_range.minLens,
				sif->lens_range.maxLens);
		}
	};

	if (check_af_mode && (fc->af.afMode.actionType == new_act)
	&& (fc->af.afMode.mode != AF_MODE_MANUAL)) {
		fc->af.afMode.mode = AF_MODE_MANUAL;
		ISP_PR_INFO("change afmode to manual\n");
	};

	if (zsl_buf == NULL)
		return;

	fc->zslBuf.enabled = 1;
	fc->zslBuf.buffer.vmidSpace.bit.vmid = 0;
	fc->zslBuf.buffer.vmidSpace.bit.space = ADDR_SPACE_TYPE_GPU_VA;
	isp_split_addr64(zsl_buf->y_map_info.mc_addr,
		&dataAddrLo,
		&dataAddrHi);
	fc->zslBuf.buffer.bufBaseALo = dataAddrLo;
	fc->zslBuf.buffer.bufBaseAHi = dataAddrHi;
	fc->zslBuf.buffer.bufSizeA = zsl_buf->y_map_info.len;

	isp_split_addr64(zsl_buf->u_map_info.mc_addr,
		&dataAddrLo,
		&dataAddrHi);
	fc->zslBuf.buffer.bufBaseBLo = dataAddrLo;
	fc->zslBuf.buffer.bufBaseBHi = dataAddrHi;
	fc->zslBuf.buffer.bufSizeB = zsl_buf->u_map_info.len;

	isp_split_addr64(zsl_buf->v_map_info.mc_addr,
		&dataAddrLo,
		&dataAddrHi);
	fc->zslBuf.buffer.bufBaseCLo = dataAddrLo;
	fc->zslBuf.buffer.bufBaseCHi = dataAddrHi;
	fc->zslBuf.buffer.bufSizeC = zsl_buf->v_map_info.len;
#endif
}

enum _SensorId_t isp_get_fw_sensor_id(struct isp_context *isp,
	enum camera_id cid)
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

enum _StreamId_t isp_get_fw_stream_from_drv_stream(
	enum fw_cmd_resp_stream_id stream)
{
	switch (stream) {
	case FW_CMD_RESP_STREAM_ID_1:
		return STREAM_ID_1;
	case FW_CMD_RESP_STREAM_ID_2:
		return STREAM_ID_2;
	case FW_CMD_RESP_STREAM_ID_3:
		return STREAM_ID_3;
	default:
		return (unsigned short) STREAM_ID_INVALID;
	}

}

int isp_is_mem_camera(struct isp_context *isp, enum camera_id cid)
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

int isp_hw_mem_read_(unsigned long long mem_addr, void *p_read_buffer,
			unsigned int byte_size)
{
#ifndef USING_KGD_CGS
	if (isp_is_fpga() && g_cgs_srv.cgs_gpumemcopy) {
		//g_cgs_srv
		struct cgs_gpu_mem_copy_input input;

		input.mc_dest = (unsigned long long) p_read_buffer;
		input.mc_src = mem_addr;
		input.size = byte_size;
		g_cgs_srv.cgs_gpumemcopy(g_cgs_srv.cgs_device, &input);
	}
#endif
	return RET_SUCCESS;
};

int isp_hw_mem_write_(unsigned long long mem_addr,
				void *p_write_buffer,
				unsigned int byte_size)
{
#ifndef USING_KGD_CGS
	if (isp_is_fpga() && g_cgs_srv.cgs_gpumemcopy) {
		//g_cgs_srv
		struct cgs_gpu_mem_copy_input input;

		input.mc_dest = mem_addr;
		input.mc_src = (unsigned long long) p_write_buffer;
		input.size = byte_size;
		g_cgs_srv.cgs_gpumemcopy(g_cgs_srv.cgs_device, &input);
	}
#endif
	return RET_SUCCESS;
};

long IspMapValueFromRange(long srcLow, long srcHigh,
			long srcValue, long desLow,
			long desHigh)
{
	long srcRange = srcHigh - srcLow;
	long desRange = desHigh - desLow;

	if (srcRange == 0)
		return (desLow + desHigh) / 2;

	return srcValue * desRange * 100 / srcRange / 100;
}

bool roi_window_isvalid(struct _Window_t *wnd)
{
	if (wnd->h_size != 0 && wnd->v_size != 0)
		return true;
	return false;
}

struct isp_gpu_mem_info *isp_get_work_buf(struct isp_context *isp,
					enum camera_id cid,
					enum isp_work_buf_type buf_type,
					unsigned int idx)
{
	struct isp_gpu_mem_info *ret_buf = NULL;
	static unsigned char indir_fw_cmd_pkg_idx[CAMERA_ID_MAX] = { 0 };

	if (!is_para_legal(isp, cid)) {
		ISP_PR_ERR("fail para cid %u\n", cid);
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
		if (idx < MAX_TNR_REF_BUF_CNT) {
			ret_buf =
			    isp->work_buf.sensor_work_buf[cid].tnr_tmp_buf[idx];
		}
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
		ISP_PR_ERR("fail buf type %u\n", buf_type);
		return NULL;

	};
	if (ret_buf) {
		ISP_PR_DBG("suc,cid %u,%s(%u),idx %u,ret %p(%llx)\n",
			cid, isp_dbg_get_work_buf_str(buf_type),
			buf_type, idx, ret_buf, ret_buf->mem_size);
	} else {
		ISP_PR_ERR("fail,cid %u,%s(%u),idx %u,ret %p\n",
			cid, isp_dbg_get_work_buf_str(buf_type),
			buf_type, idx, ret_buf);
	}
	return ret_buf;
};

void isp_alloc_work_buf(struct isp_context *isp)
{
	enum camera_id cid;
	unsigned int i = 0;
	unsigned int total = 0;
	unsigned int size = 0;

	if (isp == NULL) {
		ISP_PR_ERR("fail para\n");
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
		total += size;
		for (i = 0; i < MAX_TNR_REF_BUF_CNT; i++) {
			isp->work_buf.sensor_work_buf[cid].tnr_tmp_buf[i] =
				isp_gpu_mem_alloc(size, ISP_GPU_MEM_TYPE_NLFB);
		}
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
	ISP_PR_INFO("suc, %u allocated\n", total);
};

void isp_free_work_buf(struct isp_context *isp)
{
	enum camera_id cid;
	unsigned int i = 0;

	if (isp == NULL) {
		ISP_PR_ERR("fail para\n");
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
		for (i = 0; i < MAX_TNR_REF_BUF_CNT; i++) {
			isp_gpu_mem_free
			    (isp->work_buf.sensor_work_buf[cid].tnr_tmp_buf[i]);
			isp->work_buf.sensor_work_buf[cid].tnr_tmp_buf[i] =
			    NULL;
		}
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
	ISP_PR_INFO("suc\n");
};

int isp_get_rgbir_crop_window(struct isp_context *isp,
				enum camera_id cid,
				struct _Window_t *crop_window)
{
	struct _package_td_header *ph = NULL;

	memset(crop_window, 0, sizeof(struct _Window_t));

	if (isp->sensor_info[cid].cam_type != CAMERA_TYPE_MEM) {
		ISP_PR_ERR("fail, not a RGBIR camera\n");
		return RET_FAILURE;
	}

	ph = (struct _package_td_header *) isp->calib_data[cid];

	if ((ph->crop_wnd_width == 0)
	|| (ph->crop_wnd_height == 0)) {
		ISP_PR_ERR("fail, no valid ir corp wnd w:h %u:%u\n",
			ph->crop_wnd_width, ph->crop_wnd_height);
		return RET_FAILURE;
	}

	crop_window->h_size = ph->crop_wnd_width;
	crop_window->v_size = ph->crop_wnd_height;

	return RET_SUCCESS;
}

/**codes to support psp fw loading***/
char *isp_get_fw_name(unsigned int major, unsigned int minor, unsigned int rev)
{
	char *name = NULL;
	unsigned int i;

	unsigned int count = ARRAY_SIZE(isp_hwip_to_fw_name);

	for (i = 0; i < count; i++) {
		if ((major == isp_hwip_to_fw_name[i].major)
		&& (minor == isp_hwip_to_fw_name[i].minor)
		&& (rev == isp_hwip_to_fw_name[i].rev)) {
			name = (char *)isp_hwip_to_fw_name[i].fw_name;
			break;
		}
	}
	if (name) {
		ISP_PR_INFO("suc %s for %u.%u.%u\n",
			name, major, minor, rev);
	} else {
		ISP_PR_ERR("fail for %u.%u.%u\n",
			major, minor, rev);
	}
	return name;

};

void isp_get_fw_version_from_psp_header(void *fw_buf,
					unsigned int *has_psp_header,
					unsigned int *fw_version)
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
int isp_program_mmhub_ds_reg(int b_busy)
{
	/*
	 * Put ISP DS bit to busy
	 * 0x0 means that the IP is busy
	 * 0x1 means that the IP is idle
	 */
	const unsigned int PCTL_MMHUB_DEEPSLEEP = 0x00068E04;
	const unsigned char MMHUB_DS_ISP_BIT = 4;
	const unsigned char MMHUB_DS_SETCLEAR_BIT = 31;
	unsigned int reg_value;

	reg_value = 1U << MMHUB_DS_ISP_BIT;

	if (b_busy == TRUE)
		reg_value |= 1U << MMHUB_DS_SETCLEAR_BIT;
	else
		reg_value &= ~(1U << MMHUB_DS_SETCLEAR_BIT);

	isp_hw_reg_write32(PCTL_MMHUB_DEEPSLEEP, reg_value);
	return IMF_RET_SUCCESS;
}

#ifdef USING_KGD_CGS
void isp_split_addr64(unsigned long long addr,
		unsigned int *lo, unsigned int *hi)
{
	if (lo)
		*lo = (unsigned int)(addr & 0xffffffff);
	if (hi)
		*hi = (unsigned int)(addr >> 32);
};

unsigned long long isp_join_addr64(unsigned int lo, unsigned int hi)
{
	return (((unsigned long long)hi) << 32) | (unsigned long long)lo;
}

unsigned int isp_get_cmd_pl_size(void)
{
	unsigned int ret = 1024;

	ret = MAX(ret, sizeof(struct _MetaInfo_t));
	ret = MAX(ret, sizeof(struct _FrameInfo_t));
	ret = MAX(ret, sizeof(struct _CmdSetSensorProp_t));
	ret = MAX(ret, sizeof(struct _CmdSetSensorHdrProp_t));
	ret = MAX(ret, sizeof(struct _CmdSetDeviceScript_t));
	//ret = MAX(ret, sizeof(CmdAwbSetLscMatrix_t));
	//ret = MAX(ret, sizeof(CmdAwbSetLscSector_t));
	ret = MAX(ret, sizeof(struct _CmdFrameCtrl_t));
	ret = MAX(ret, sizeof(struct _CmdCaptureYuv_t));
	ret = ISP_ADDR_ALIGN_UP(ret, ISP_FW_CMD_PAY_LOAD_BUF_ALIGN);
	return ret;
};
#endif

int isp_config_cvip_sensor(struct isp_context *isp, enum camera_id cid,
			   enum _TestPattern_t pattern)
{
	int id;
	int ret = RET_FAILURE;
	struct mode_info profile;
	struct _CmdConfigCvipSensor_t param;

	ENTER();
	if (!is_para_legal(isp, cid)) {
		ISP_PR_ERR("fail, bad para isp[%p] cam[%d]", isp, cid);
		return ret;
	}

	ret = isp_get_caps_for_prf_id(isp, cid, g_profile_id[cid]);
	id = isp_get_id_for_prf(cid, g_profile_id[cid]);
	profile = g_snr_info[cid].modes[id];

	memset(&param, 0, sizeof(param));
	param.sensorId = cid;
	if (profile.bin == BINNING_MODE_2x2)
		param.param.resMode = RES_MODE_H2V2_BINNING;
	else if (profile.bin == BINNING_MODE_2_VERTICAL)
		param.param.resMode = RES_MODE_V2_BINNING;
	else if (profile.bin == BINNING_MODE_NONE)
		param.param.resMode = RES_MODE_FULL;
	else
		param.param.resMode = RES_MODE_INVALID;

	param.param.width = profile.w;
	param.param.height = profile.h;
	param.param.fps = profile.min_frame_rate * 1000;
	if (param.param.fps == 0) {
		ISP_PR_ERR("fail, bad para fps(0), change to 5\n");
		param.param.fps = 5000;
	}
	if (profile.hdr == HDR_MODE_DOL2)
		param.param.hdrMode = HDR_MODE_KERNEL_2_DOL_HDR;
	else if (profile.hdr == HDR_MODE_DOL3)
		param.param.hdrMode = HDR_MODE_KERNEL_3_DOL_HDR;
	else if (profile.hdr == HDR_MODE_INVALID)
		param.param.hdrMode = HDR_MODE_KERNEL_NO_HDR;
	else
		param.param.hdrMode = HDR_MODE_KERNEL_INVALID;
	param.param.testPattern = pattern;
	param.param.bitDepth = profile.bits_per_pixel;
	param.param.dolNum = 1;
	ISP_PR_PC("Mode:%d wxh:%dx%d fps:%d hdr:%d testPattern:%d dolNum:%d",
		  param.param.resMode, param.param.width,
		  param.param.height, param.param.fps,
		  param.param.hdrMode, param.param.testPattern,
		  param.param.dolNum);

	ret = isp_fw_config_cvip_sensor(isp, cid, param);

	RET(ret);
	return ret;
}

int isp_setup_cvip_buf(struct isp_context *isp, enum camera_id cid)
{
	u64 gpu_addr;
	void *cpu_addr;
	int ret = RET_SUCCESS;
	unsigned int stats_size;
	struct _CmdSetCvipBufLayout_t param;
	struct isp_drv_to_cvip_ctl *ctl;

	ENTER();
	if (!is_para_legal(isp, cid)) {
		ISP_PR_ERR("bad para, cam[%d]", cid);
		return RET_FAILURE;
	}

	stats_size = (unsigned int)(sizeof(struct _CvipStatsInfo_t) * 6);

	// create CVIP buf
	if (!g_cpu_addr) {
		ret = create_cvip_buf(stats_size);
		if (ret != RET_SUCCESS)
			goto quit;
	}

	cpu_addr = g_cpu_addr + cid * (CVIP_PRE_POST_BUF_SIZE + stats_size);
	gpu_addr = g_gpu_addr + cid * (CVIP_PRE_POST_BUF_SIZE + stats_size);

	//set output yuv info for CVIP
	ctl = (struct isp_drv_to_cvip_ctl *)cpu_addr;
	ctl->type = TYPE_ANDROID;
	ctl->raw_fmt = isp_trans_to_fw_raw_pkt_fmt(g_prop->cvip_raw_fmt);
	ctl->raw_slice_num = g_prop->cvip_raw_slicenum;
	ctl->output_slice_num = g_prop->cvip_output_slicenum;
	ctl->output_width = g_prop->cvip_yuv_w;
	ctl->output_height = g_prop->cvip_yuv_h;
	ctl->output_fmt = isp_trans_to_fw_img_fmt(g_prop->cvip_yuv_fmt);

	//send cvip pre-post buf
	memset(&param, 0, sizeof(param));
	param.cvipBufType = CVIP_BUF_TYPE_PREPOST_BUF;
	param.sensorId = cid;
	param.bufSize = (unsigned int)CVIP_PRE_POST_BUF_SIZE;
	isp_split_addr64(gpu_addr, &param.bufAddrLo, &param.bufAddrHi);
	ret = isp_fw_set_cvip_buf(isp, cid, param);
	if (ret != RET_SUCCESS) {
		ISP_PR_ERR("cam[%d] send cvip pre-post buf fail", cid);
		goto quit;
	}

	//send cvip stats buf
	memset(&param, 0, sizeof(param));
	param.cvipBufType = CVIP_BUF_TYPE_STATS_BUF;
	param.sensorId = cid;
	param.bufSize = stats_size;
	gpu_addr += (u64)CVIP_PRE_POST_BUF_SIZE;
	isp_split_addr64(gpu_addr, &param.bufAddrLo, &param.bufAddrHi);
	ret = isp_fw_set_cvip_buf(isp, cid, param);
	if (ret != RET_SUCCESS) {
		//add this line to trick CP
		ISP_PR_ERR("cam[%d] send cvip stats buf fail", cid);
	}

 quit:
	RET(ret);
	return ret;
}

int create_cvip_buf(unsigned int stats_size)
{
	int ret = RET_FAILURE;
	unsigned int total_size;
	unsigned long cvip_buf_size;

	ENTER();
	total_size = 2 * (CVIP_PRE_POST_BUF_SIZE + stats_size);
	if (g_swisp_svr.get_cvip_buf) {
		ret = g_swisp_svr.get_cvip_buf(&cvip_buf_size, &g_gpu_addr,
					       &g_cpu_addr);
		if (ret != 0) {
			ISP_PR_ERR("get_cvip_buf fail! ret: %d", ret);
			goto fail;
		} else {
			ISP_PR_DBG("buf size:%lu gAddr:%llx cAddr:%p",
				   cvip_buf_size, g_gpu_addr, g_cpu_addr);

			// test code
			//memset(g_cpu_addr, 0x66, 16);
			//memset(g_cpu_addr + cvip_buf_size / 2, 0x77, 16);
			//memset(g_cpu_addr + cvip_buf_size - 16, 0x88, 16);
			//ISP_PR_ERR("set 6 7 8 to cvip buf for test!");

			if (cvip_buf_size < total_size) {
				ISP_PR_ERR("buf size is small than %d",
					   total_size);
				goto fail;
			}

			if (g_swisp_svr.cvip_set_gpuva) {
				ret = g_swisp_svr.cvip_set_gpuva((u64)
								 g_gpu_addr);
				if (ret < 0) {
					ISP_PR_ERR("set gpuva fail! ret:%d",
						   ret);
					goto fail;
				}
			} else {
				ISP_PR_ERR("fail! no cvip_set_gpuva");
				goto fail;
			}
		}
	} else {
		ISP_PR_ERR("fail! no get_cvip_buf func");
		goto fail;
	}

	RET(RET_SUCCESS);
	return RET_SUCCESS;
 fail:
	RET(RET_FAILURE);
	return RET_FAILURE;
}

int isp_stop_cvip_sensor(struct isp_context *isp, enum camera_id cid)
{
	int ret = RET_FAILURE;
	struct _CmdStopCvipSensor_t param;

	ENTER();
	if (!is_para_legal(isp, cid)) {
		ISP_PR_ERR("fail, bad para isp[%p] cam[%d]", isp, cid);
		return ret;
	}

	param.sensorId = cid;
	if (isp_send_fw_cmd(isp, CMD_ID_STOP_CVIP_SENSOR,
			    FW_CMD_RESP_STREAM_ID_GLOBAL,
			    FW_CMD_PARA_TYPE_DIRECT, &param,
			    sizeof(param)) != RET_SUCCESS) {
		ISP_PR_ERR("cam[%d] fail", cid);
		ret = RET_FAILURE;
	} else {
		ISP_PR_INFO("cam[%d] suc", cid);
		ret = RET_SUCCESS;
	}

	RET(ret);
	return ret;
}

int isp_set_cvip_sensor_slice_num(struct isp_context *isp, enum camera_id cid)
{
	int ret = RET_FAILURE;
	struct _CmdSetSensorSliceNum_t param;

	ENTER();
	if (!is_para_legal(isp, cid)) {
		ISP_PR_ERR("fail, bad para isp[%p] cam[%d]", isp, cid);
		return ret;
	}

	param.sensorId = cid;
	param.sliceNum = g_prop->cvip_raw_slicenum;
	ret = isp_fw_set_cvip_sensor_slice_num(isp, cid, param);

	RET(ret);
	return ret;
}

