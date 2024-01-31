/*
 * Copyright (C) 2019-2021 Advanced Micro Devices, Inc. All rights reserved.
 */

#include "os_base_type.h"
#include "isp_common.h"
#include "log.h"
#include "buffer_mgr.h"
#include "isp_fw_if/drv_isp_if.h"
#include <media/v4l2-common.h>

#ifdef OUTPUT_FW_LOG_TO_FILE
#define FW_LOG_FILE_PATH "/data/misc/camera_dump/fw.log"
struct file *g_fwlog_fp;
char g_fw_log[512 * 1024];

void open_fw_log_file(void)
{
	if (ISP_DEBUG_LEVEL < TRACE_LEVEL_ERROR)
		return;
	if (!g_fwlog_fp) {
		g_fwlog_fp = filp_open(FW_LOG_FILE_PATH, O_WRONLY
			| O_TRUNC | O_APPEND | O_CREAT, 0600);
		if (IS_ERR(g_fwlog_fp)) {
			ISP_PR_ERR("Open FW log file %s fail 0x%llx",
				   FW_LOG_FILE_PATH,
				   (unsigned long long)g_fwlog_fp);
			g_fwlog_fp = NULL;
			return;
		}
		ISP_PR_INFO("Open FW log file %s ret 0x%llx, succ\n",
			    FW_LOG_FILE_PATH,
			    (unsigned long long)g_fwlog_fp);
	} else {
		ISP_PR_DBG("FW log file %s opened already\n",
			    FW_LOG_FILE_PATH);
	}
}

void close_fw_log_file(void)
{
	if (g_fwlog_fp) {
		filp_close(g_fwlog_fp, NULL);
		g_fwlog_fp = NULL;
		ISP_PR_INFO("close FW log file");
	} else {
		ISP_PR_INFO("no need to close FW log for not opened");
	}
}

void ISP_PR_INFO_FW(const char *fmt, ...)
{
	va_list ap;
	ulong len;
	if (ISP_DEBUG_LEVEL < TRACE_LEVEL_ERROR)
		return;
	if (!g_fwlog_fp)
		return;
	va_start(ap, fmt);
	vsprintf(g_fw_log, fmt, ap);
	len = strlen(g_fw_log);
	isp_write_file_test(g_fwlog_fp, g_fw_log, &len);
	va_end(ap);
}
#endif

char *isp_dbg_get_isp_status_str(unsigned int status)
{
	switch (status) {
	case ISP_STATUS_PWR_OFF:
		return "ISP_STATUS_PWR_OFF";
	case ISP_STATUS_FW_RUNNING:
		return "ISP_STATUS_FW_RUNNING";
		/*case ISP_FSM_STATUS_MAX: return ""; */
	default:
		return "unknown ISP status";
	}
}

char *dbg_get_snr_intf_type_str(enum _SensorIntfType_t type)
{
	switch (type) {
	case SENSOR_INTF_TYPE_MIPI:
		return "SENSOR_INTF_TYPE_MIPI";
	case SENSOR_INTF_TYPE_PARALLEL:
		return "SENSOR_INTF_TYPE_PARALLEL";
	case SENSOR_INTF_TYPE_RDMA:
		return "SENSOR_INTF_TYPE_RDMA";
	case SENSOR_INTF_TYPE_MAX:
		return "SENSOR_INTF_TYPE_MAX";
	default:
		return "Unknown type";
	};
}

void dbg_show_sensor_prop(void *sensor_cfg)
{
	struct _SensorProp_t *prop;
	struct _CmdSetSensorProp_t *cfg;

	if (sensor_cfg == NULL)
		return;
	cfg = (struct _CmdSetSensorProp_t *) sensor_cfg;
	prop = &cfg->sensorProp;

	ISP_PR_INFO("cid %d, intfType %s\n", cfg->sensorId,
			dbg_get_snr_intf_type_str(prop->intfType));

	if (prop->intfType == SENSOR_INTF_TYPE_MIPI) {
		ISP_PR_INFO("mipi.num_lanes %d\n",
				prop->intfProp.mipi.numLanes);
		ISP_PR_INFO("mipi.MIPI_VIRTUAL_CHANNEL_%d\n",
				prop->intfProp.mipi.virtChannel);
		ISP_PR_INFO("mipi.data_type %d\n",
				prop->intfProp.mipi.dataType);
		ISP_PR_INFO("mipi.comp_scheme %d\n",
				prop->intfProp.mipi.compScheme);
		ISP_PR_INFO("mipi.pred_block %d\n",
				prop->intfProp.mipi.predBlock);
		ISP_PR_INFO("cfaPattern %d\n", prop->cfaPattern);

	} else if (prop->intfType == SENSOR_INTF_TYPE_PARALLEL) {
		ISP_PR_INFO("parallel.dataType %d\n",
				prop->intfProp.parallel.dataType);
		ISP_PR_INFO("parallel.hPol %d\n",
				prop->intfProp.parallel.hPol);
		ISP_PR_INFO("parallel.vPol %d\n",
				prop->intfProp.parallel.vPol);
		ISP_PR_INFO("parallel.edge %d\n",
				prop->intfProp.parallel.edge);
		ISP_PR_INFO("parallel.ycSeq %d\n",
				prop->intfProp.parallel.ycSeq);
		ISP_PR_INFO("parallel.subSampling %d\n",
				prop->intfProp.parallel.subSampling);

		ISP_PR_INFO("cfaPattern %d\n", prop->cfaPattern);
	};

}

void dbg_show_sensor_caps(void *sensor_cap /*struct _isp_sensor_prop_t */)
{
	struct _isp_sensor_prop_t *cap;

	if (sensor_cap == NULL)
		return;
	cap = (struct _isp_sensor_prop_t *) sensor_cap;

	ISP_PR_INFO("dgain range [%u,%u]\n",
			cap->exposure.min_digi_gain,
			cap->exposure.max_digi_gain);
	ISP_PR_INFO("again range [%u,%u]\n",
			cap->exposure.min_gain, cap->exposure.max_gain);

	ISP_PR_INFO("itime range [%u,%u]\n",
			cap->exposure.min_integration_time,
			cap->exposure.max_integration_time);
	ISP_PR_INFO("lens range [%u,%u]\n",
			cap->lens_pos.min_pos, cap->lens_pos.max_pos);

	ISP_PR_INFO("windows [%u,%u,%u,%u]\n",
			cap->window.h_offset, cap->window.v_offset,
			cap->window.h_size, cap->window.v_size);

	ISP_PR_INFO("framerate %u\n", cap->frame_rate);

	ISP_PR_INFO("hdr_valid %u\n", cap->hdr_valid);
};

void isp_dbg_show_map_info(void *in /*struct isp_mapped_buf_info */)
{
	struct isp_mapped_buf_info *p = in;

	if (p == NULL)
		return;
	ISP_PR_INFO("y sys:mc:len %llx:%llx:%u\n",
			p->y_map_info.sys_addr, p->y_map_info.mc_addr,
			p->y_map_info.len);
	ISP_PR_INFO("u sys:mc:len %llx:%llx:%u\n",
			p->u_map_info.sys_addr, p->u_map_info.mc_addr,
			p->u_map_info.len);
	ISP_PR_INFO("v sys:mc:len %llx:%llx:%u\n",
			p->v_map_info.sys_addr, p->v_map_info.mc_addr,
			p->v_map_info.len);
};

char *isp_dbg_get_buf_src_str(unsigned int src)
{
	char *ret;

	switch (src) {
		CASE_MACRO_2_MACRO_STR(BUFFER_SOURCE_INVALID);
		CASE_MACRO_2_MACRO_STR(BUFFER_SOURCE_CMD_CAPTURE);
		CASE_MACRO_2_MACRO_STR(BUFFER_SOURCE_FRAME_CONTROL);
		CASE_MACRO_2_MACRO_STR(BUFFER_SOURCE_STREAM);
		CASE_MACRO_2_MACRO_STR(BUFFER_SOURCE_HOST_POST);
		CASE_MACRO_2_MACRO_STR(BUFFER_SOURCE_TEMP);
		CASE_MACRO_2_MACRO_STR(BUFFER_SOURCE_MAX);
			break;
	default:
		return "Unknown output fmt";
	}
	return ret;
}

char *isp_dbg_get_buf_done_str(unsigned int status)
{
	char *ret;

	switch (status) {
		CASE_MACRO_2_MACRO_STR(BUFFER_STATUS_INVALID);
		CASE_MACRO_2_MACRO_STR(BUFFER_STATUS_SKIPPED);
		CASE_MACRO_2_MACRO_STR(BUFFER_STATUS_EXIST);
		CASE_MACRO_2_MACRO_STR(BUFFER_STATUS_DONE);
		CASE_MACRO_2_MACRO_STR(BUFFER_STATUS_LACK);
		CASE_MACRO_2_MACRO_STR(BUFFER_STATUS_DIRTY);
		CASE_MACRO_2_MACRO_STR(BUFFER_STATUS_MAX);
			break;
	default:
		return "Unknown Buf Done Status";
	}
	return ret;
};

char *isp_dbg_get_img_fmt_str(void *in /*enum _ImageFormat_t **/)
{
	enum _ImageFormat_t *t;

	t = (enum _ImageFormat_t *) in;

	switch (*t) {
	case IMAGE_FORMAT_INVALID:
		return "INVALID";
	case IMAGE_FORMAT_NV12:
		return "NV12";
	case IMAGE_FORMAT_NV21:
		return "NV21";
	case IMAGE_FORMAT_I420:
		return "I420";
	case IMAGE_FORMAT_YV12:
		return "YV12";
	case IMAGE_FORMAT_YUV422PLANAR:
		return "YUV422P";
	case IMAGE_FORMAT_YUV422SEMIPLANAR:
		return "YUV422SEMIPLANAR";
	case IMAGE_FORMAT_YUV422INTERLEAVED:
		return "YUV422INTERLEAVED";
	case IMAGE_FORMAT_RGBBAYER8:
		return "RGBBAYER8";
	case IMAGE_FORMAT_RGBBAYER10:
		return "RGBBAYER10";
	case IMAGE_FORMAT_RGBBAYER12:
		return "RGBBAYER12";
	case IMAGE_FORMAT_RGBIR8:
		return "RGBIR8";
	case IMAGE_FORMAT_RGBIR10:
		return "RGBIR10";
	case IMAGE_FORMAT_RGBIR12:
		return "RGBIR12";
	default:
		return "Unknown";
	}
}

void isp_dbg_show_bufmeta_info(char *pre, unsigned int cid,
			 void *in, void *origBuf)
{
	struct _BufferMetaInfo_t *p;
	struct sys_img_buf_handle *orig;

	if (in == NULL)
		return;
	if (pre == NULL)
		pre = "";
	p = (struct _BufferMetaInfo_t *) in;
	orig = (struct sys_img_buf_handle *)origBuf;

	ISP_PR_INFO("%s(%s)%u %p(%u), en:%d,stat:%s(%u),src:%s\n",
			   pre,
			   isp_dbg_get_img_fmt_str(&p->imageProp.imageFormat),
			   cid, orig->virtual_addr, orig->len, p->enabled,
			   isp_dbg_get_buf_done_str(p->status), p->status,
			   isp_dbg_get_buf_src_str(p->source));
}

void isp_dbg_show_img_prop(char *pre, void *in /*struct _ImageProp_t **/)
{
	struct _ImageProp_t *p = (struct _ImageProp_t *) in;

	ISP_PR_INFO("%s fmt:%s(%d),w:h(%d:%d),lp:cp(%d:%d)\n",
			 pre, isp_dbg_get_out_fmt_str(p->imageFormat),
			p->imageFormat, p->width, p->height, p->lumaPitch,
			 p->chromaPitch);
};

void dbg_show_drv_settings(void *setting /*struct driver_settings */)
{
	struct driver_settings *p = (struct driver_settings *)setting;

	ISP_PR_INFO("rear_sensor %s\n", p->rear_sensor);
	ISP_PR_INFO("rear_vcm %s\n", p->rear_vcm);
	ISP_PR_INFO("rear_flash %s\n", p->rear_flash);
	ISP_PR_INFO("frontl_sensor %s\n", p->frontl_sensor);
	ISP_PR_INFO("rear_sensor %s\n", p->frontr_sensor);
	ISP_PR_INFO("sensor_frame_rate %u\n", p->sensor_frame_rate);
	ISP_PR_INFO("af_default_auto %i\n", p->af_default_auto);
	ISP_PR_INFO("rear_sensor_type %i\n", p->rear_sensor_type);
	ISP_PR_INFO("frontl_sensor_type %i\n", p->frontl_sensor_type);
	ISP_PR_INFO("frontr_sensor_type %i\n", p->frontr_sensor_type);
	ISP_PR_INFO("ae_default_auto %i\n", p->ae_default_auto);
	ISP_PR_INFO("awb_default_auto %i\n", p->awb_default_auto);
	ISP_PR_INFO("fw_ctrl_3a %i\n", p->fw_ctrl_3a);
	ISP_PR_INFO("set_again_by_sensor_drv %i\n",
			p->set_again_by_sensor_drv);
	ISP_PR_INFO("set_itime_by_sensor_drv %i\n",
			p->set_itime_by_sensor_drv);
	ISP_PR_INFO("set_dgain_by_sensor_drv %i\n",
			p->set_dgain_by_sensor_drv);
	ISP_PR_INFO("set_lens_pos_by_sensor_drv %i\n",
			p->set_lens_pos_by_sensor_drv);
	ISP_PR_INFO("do_fpga_to_sys_mem_cpy %i\n",
			p->do_fpga_to_sys_mem_cpy);
	ISP_PR_INFO("hdmi_support_enable %i\n",
		p->hdmi_support_enable);

	ISP_PR_INFO("fw_log_cfg_en %lu\n", p->fw_log_cfg_en);
	ISP_PR_INFO("fw_log_cfg_A %lu\n", p->fw_log_cfg_A);
	ISP_PR_INFO("fw_log_cfg_B %lu\n", p->fw_log_cfg_B);
	ISP_PR_INFO("fw_log_cfg_C %lu\n", p->fw_log_cfg_C);
	ISP_PR_INFO("fw_log_cfg_D %lu\n", p->fw_log_cfg_D);
	ISP_PR_INFO("fw_log_cfg_E %lu\n", p->fw_log_cfg_E);
	ISP_PR_INFO("fw_log_cfg_F %lu\n", p->fw_log_cfg_F);
	ISP_PR_INFO("fw_log_cfg_G %lu\n", p->fw_log_cfg_G);
	ISP_PR_INFO("fix_wb_lighting_index %i\n",
			p->fix_wb_lighting_index);
	ISP_PR_INFO("Stage2TempBufEnable %i\n",
			p->enable_stage2_temp_buf);
	ISP_PR_INFO("IgnoreStage2Meta %i\n", p->ignore_meta_at_stage2);
	ISP_PR_INFO("DriverLoadFw %i\n", p->driver_load_fw);
	ISP_PR_INFO("DisableDynAspRatioWnd %i\n",
			p->disable_dynamic_aspect_wnd);
	ISP_PR_INFO("tnr_enable %i\n", p->tnr_enable);
	ISP_PR_INFO("snr_enable %i\n", p->snr_enable);

	ISP_PR_INFO("PinOutputDumpEnable %i\n",
			p->dump_pin_output_to_file);
	ISP_PR_INFO("HighVideoCapEnable %i\n",
			p->enable_high_video_capabilities);
	ISP_PR_INFO("dpf_enable %i\n", p->dpf_enable);
	ISP_PR_INFO("output_metadata_in_log %i\n",
			p->output_metadata_in_log);
	ISP_PR_INFO("demosaic_thresh %u\n", p->demosaic_thresh);
	ISP_PR_INFO("enable_frame_drop_solution_for_frame_pair %u\n",
			p->enable_frame_drop_solution_for_frame_pair);
	ISP_PR_INFO("disable_ir_illuminator %u\n",
			p->disable_ir_illuminator);
	ISP_PR_INFO("ir_illuminator_ctrl %u\n", p->ir_illuminator_ctrl);
	ISP_PR_INFO("min_ae_roi_size_filter %u\n",
			p->min_ae_roi_size_filter);
	ISP_PR_INFO("reset_small_ae_roi %u\n", p->reset_small_ae_roi);
	ISP_PR_INFO("process_3a_by_host %u\n", p->process_3a_by_host);
	ISP_PR_INFO("low_light_fps %u\n", p->low_light_fps);
	ISP_PR_INFO("drv_log_level %u\n", p->drv_log_level);
	ISP_PR_INFO("scaler_param_tbl_idx %d\n",
					p->scaler_param_tbl_idx);
	ISP_PR_INFO("enable_using_fb_for_taking_raw %d\n",
					p->enable_using_fb_for_taking_raw);
};

char *isp_dbg_get_out_fmt_str(int fmt /*enum enum _ImageFormat_t */)
{
	char *ret;

	switch (fmt) {
		CASE_MACRO_2_MACRO_STR(IMAGE_FORMAT_INVALID);
		CASE_MACRO_2_MACRO_STR(IMAGE_FORMAT_NV12);
		CASE_MACRO_2_MACRO_STR(IMAGE_FORMAT_NV21);
		CASE_MACRO_2_MACRO_STR(IMAGE_FORMAT_I420);
		CASE_MACRO_2_MACRO_STR(IMAGE_FORMAT_YV12);
		CASE_MACRO_2_MACRO_STR(IMAGE_FORMAT_YUV422PLANAR);
		CASE_MACRO_2_MACRO_STR(IMAGE_FORMAT_YUV422SEMIPLANAR);
		CASE_MACRO_2_MACRO_STR(IMAGE_FORMAT_YUV422INTERLEAVED);
		CASE_MACRO_2_MACRO_STR(IMAGE_FORMAT_RGBBAYER8);
		CASE_MACRO_2_MACRO_STR(IMAGE_FORMAT_RGBBAYER10);
		CASE_MACRO_2_MACRO_STR(IMAGE_FORMAT_RGBBAYER12);
		CASE_MACRO_2_MACRO_STR(IMAGE_FORMAT_RGBIR8);
		CASE_MACRO_2_MACRO_STR(IMAGE_FORMAT_RGBIR10);
		CASE_MACRO_2_MACRO_STR(IMAGE_FORMAT_RGBIR12);
			break;
	default:
		return "Unknown output fmt";
	};
	return ret;
}

char *isp_dbg_get_buf_type(unsigned int type)
{				/*enum _BufferType_t */
	char *ret;

	switch (type) {
		CASE_MACRO_2_MACRO_STR(BUFFER_TYPE_RAW);
		CASE_MACRO_2_MACRO_STR(BUFFER_TYPE_RAW_TEMP);
		CASE_MACRO_2_MACRO_STR(BUFFER_TYPE_FULL_RAW_TEMP);
		CASE_MACRO_2_MACRO_STR(BUFFER_TYPE_EMB_DATA);
		CASE_MACRO_2_MACRO_STR(BUFFER_TYPE_HDR_STATS_DATA);
		CASE_MACRO_2_MACRO_STR(BUFFER_TYPE_PD_DATA);
		CASE_MACRO_2_MACRO_STR(BUFFER_TYPE_STILL);
		CASE_MACRO_2_MACRO_STR(BUFFER_TYPE_PREVIEW);
		CASE_MACRO_2_MACRO_STR(BUFFER_TYPE_VIDEO);
		CASE_MACRO_2_MACRO_STR(BUFFER_TYPE_X86_CV);
		CASE_MACRO_2_MACRO_STR(BUFFER_TYPE_YUV_TEMP);
		CASE_MACRO_2_MACRO_STR(BUFFER_TYPE_RGBIR_IR);
		CASE_MACRO_2_MACRO_STR(BUFFER_TYPE_TNR_REF);
		CASE_MACRO_2_MACRO_STR(BUFFER_TYPE_META_INFO);
		CASE_MACRO_2_MACRO_STR(BUFFER_TYPE_FRAME_INFO);
			break;
	default:
		return "Unknown type";
	}
	return ret;

}

char *isp_dbg_get_cmd_str(unsigned int cmd)
{
	char *ret;

	switch (cmd) {
		CASE_MACRO_2_MACRO_STR(CMD_ID_GET_FW_VERSION);
		CASE_MACRO_2_MACRO_STR(CMD_ID_SET_CLK_INFO);
		CASE_MACRO_2_MACRO_STR(CMD_ID_READ_REG);
		CASE_MACRO_2_MACRO_STR(CMD_ID_WRITE_REG);
		CASE_MACRO_2_MACRO_STR(CMD_ID_LOG_SET_LEVEL);
		CASE_MACRO_2_MACRO_STR(CMD_ID_LOG_ENABLE_MOD);
		CASE_MACRO_2_MACRO_STR(CMD_ID_LOG_DISABLE_MOD);
		CASE_MACRO_2_MACRO_STR(CMD_ID_PROFILER_GET_RESULT);
		CASE_MACRO_2_MACRO_STR(CMD_ID_SET_LOG_MOD_EXT);
		CASE_MACRO_2_MACRO_STR(CMD_ID_SET_SENSOR_PROP);
		CASE_MACRO_2_MACRO_STR(CMD_ID_SET_SENSOR_HDR_PROP);
		CASE_MACRO_2_MACRO_STR(CMD_ID_SET_SENSOR_EMB_PROP);
		CASE_MACRO_2_MACRO_STR(CMD_ID_SET_SENSOR_CALIBDB);
		CASE_MACRO_2_MACRO_STR(CMD_ID_SET_DEVICE_CTRL_MODE);
		CASE_MACRO_2_MACRO_STR(CMD_ID_SET_DEVICE_SCRIPT);
		CASE_MACRO_2_MACRO_STR(CMD_ID_ACK_SENSOR_GROUP_HOLD);
		CASE_MACRO_2_MACRO_STR(CMD_ID_ACK_SENSOR_GROUP_RELEASE);
		CASE_MACRO_2_MACRO_STR(CMD_ID_ACK_SENSOR_SET_AGAIN);
		CASE_MACRO_2_MACRO_STR(CMD_ID_ACK_SENSOR_SET_DGAIN);
		CASE_MACRO_2_MACRO_STR(CMD_ID_ACK_SENSOR_SET_ITIME);
		CASE_MACRO_2_MACRO_STR(CMD_ID_ACK_SENSOR_SET_HDR_HIGH_ITIME);
		CASE_MACRO_2_MACRO_STR(CMD_ID_ACK_SENSOR_SET_HDR_LOW_ITIME);
		CASE_MACRO_2_MACRO_STR
		    (CMD_ID_ACK_SENSOR_SET_HDR_LOW_ITIME_RATIO);
		CASE_MACRO_2_MACRO_STR(CMD_ID_ACK_SENSOR_SET_HDR_EQUAL_ITIME);
		CASE_MACRO_2_MACRO_STR(CMD_ID_ACK_SENSOR_SET_HDR_HIGH_AGAIN);
		CASE_MACRO_2_MACRO_STR(CMD_ID_ACK_SENSOR_SET_HDR_LOW_AGAIN);
		CASE_MACRO_2_MACRO_STR
		    (CMD_ID_ACK_SENSOR_SET_HDR_LOW_AGAIN_RATIO);
		CASE_MACRO_2_MACRO_STR(CMD_ID_ACK_SENSOR_SET_HDR_EQUAL_AGAIN);
		CASE_MACRO_2_MACRO_STR(CMD_ID_ACK_SENSOR_SET_HDR_HIGH_DGAIN);
		CASE_MACRO_2_MACRO_STR(CMD_ID_ACK_SENSOR_SET_HDR_LOW_DGAIN);
		CASE_MACRO_2_MACRO_STR
		    (CMD_ID_ACK_SENSOR_SET_HDR_LOW_DGAIN_RATIO);
		CASE_MACRO_2_MACRO_STR(CMD_ID_ACK_SENSOR_SET_HDR_EQUAL_DGAIN);
		CASE_MACRO_2_MACRO_STR(CMD_ID_ACK_LENS_SET_POS);
		CASE_MACRO_2_MACRO_STR(CMD_ID_ACK_FLASH_SET_POWER);
		CASE_MACRO_2_MACRO_STR(CMD_ID_ACK_FLASH_SET_ON);
		CASE_MACRO_2_MACRO_STR(CMD_ID_ACK_FLASH_SET_OFF);
		CASE_MACRO_2_MACRO_STR(CMD_ID_SEND_I2C_MSG);
		CASE_MACRO_2_MACRO_STR(CMD_ID_SET_ACQ_WINDOW);
		CASE_MACRO_2_MACRO_STR(CMD_ID_ENABLE_CALIB);
		CASE_MACRO_2_MACRO_STR(CMD_ID_DISABLE_CALIB);
		CASE_MACRO_2_MACRO_STR(CMD_ID_SET_ASPECT_RATIO_WINDOW);
		CASE_MACRO_2_MACRO_STR(CMD_ID_SET_ZOOM_WINDOW);
		CASE_MACRO_2_MACRO_STR(CMD_ID_SET_IR_ILLU_CONFIG);
		CASE_MACRO_2_MACRO_STR(CMD_ID_SET_DIR_OUTPUT_FRAME_TYPE);
		CASE_MACRO_2_MACRO_STR(CMD_ID_CAPTURE_YUV);
		CASE_MACRO_2_MACRO_STR(CMD_ID_CAPTURE_RAW);
		CASE_MACRO_2_MACRO_STR(CMD_ID_START_STREAM);
		CASE_MACRO_2_MACRO_STR(CMD_ID_STOP_STREAM);
		CASE_MACRO_2_MACRO_STR(CMD_ID_RESET_STREAM);
		CASE_MACRO_2_MACRO_STR(CMD_ID_SET_FRAME_RATE_INFO);
		CASE_MACRO_2_MACRO_STR(CMD_ID_SET_FRAME_CTRL_RATIO);
		CASE_MACRO_2_MACRO_STR(CMD_ID_AE_SET_MODE);
		CASE_MACRO_2_MACRO_STR(CMD_ID_AE_SET_FLICKER);
		CASE_MACRO_2_MACRO_STR(CMD_ID_AE_SET_REGION);
		CASE_MACRO_2_MACRO_STR(CMD_ID_AE_LOCK);
		CASE_MACRO_2_MACRO_STR(CMD_ID_AE_UNLOCK);
		CASE_MACRO_2_MACRO_STR(CMD_ID_AE_SET_ANALOG_GAIN);
		CASE_MACRO_2_MACRO_STR(CMD_ID_AE_SET_DIGITAL_GAIN);
		CASE_MACRO_2_MACRO_STR(CMD_ID_AE_SET_ITIME);
		CASE_MACRO_2_MACRO_STR(CMD_ID_AE_SET_ISO_PRIORITY);
		CASE_MACRO_2_MACRO_STR(CMD_ID_AE_SET_EV_COMPENSATION);
		CASE_MACRO_2_MACRO_STR(CMD_ID_AE_PRECAPTURE);
		CASE_MACRO_2_MACRO_STR(CMD_ID_AE_ENABLE_HDR);
		CASE_MACRO_2_MACRO_STR(CMD_ID_AE_DISABLE_HDR);
		CASE_MACRO_2_MACRO_STR(CMD_ID_AE_GET_STATUS);
		CASE_MACRO_2_MACRO_STR(CMD_ID_AWB_SET_MODE);
		//CASE_MACRO_2_MACRO_STR(CMD_ID_AWB_SET_REGION);
		CASE_MACRO_2_MACRO_STR(CMD_ID_AWB_SET_LIGHT_SOURCE);
		CASE_MACRO_2_MACRO_STR(CMD_ID_AWB_SET_COLOR_TEMPERATURE);
		CASE_MACRO_2_MACRO_STR(CMD_ID_AWB_SET_WB_GAIN);
		CASE_MACRO_2_MACRO_STR(CMD_ID_AWB_GET_WB_GAIN);
		CASE_MACRO_2_MACRO_STR(CMD_ID_AWB_SET_CC_MATRIX);
		CASE_MACRO_2_MACRO_STR(CMD_ID_AWB_GET_CC_MATRIX);
		CASE_MACRO_2_MACRO_STR(CMD_ID_AWB_LOCK);
		CASE_MACRO_2_MACRO_STR(CMD_ID_AWB_UNLOCK);
		CASE_MACRO_2_MACRO_STR(CMD_ID_AWB_GET_STATUS);
		CASE_MACRO_2_MACRO_STR(CMD_ID_AF_SET_MODE);
		CASE_MACRO_2_MACRO_STR(CMD_ID_AF_SET_LENS_RANGE);
		CASE_MACRO_2_MACRO_STR(CMD_ID_AF_SET_LENS_POS);
		CASE_MACRO_2_MACRO_STR(CMD_ID_AF_SET_REGION);
		CASE_MACRO_2_MACRO_STR(CMD_ID_AF_TRIG);
		CASE_MACRO_2_MACRO_STR(CMD_ID_AF_CANCEL);
		CASE_MACRO_2_MACRO_STR(CMD_ID_AF_LOCK);
		CASE_MACRO_2_MACRO_STR(CMD_ID_AF_UNLOCK);
		CASE_MACRO_2_MACRO_STR(CMD_ID_AF_SET_FOCUS_ASSIST_MODE);
		CASE_MACRO_2_MACRO_STR(CMD_ID_AF_SET_FOCUS_ASSIST_POWER_LEVEL);
		CASE_MACRO_2_MACRO_STR(CMD_ID_AF_GET_STATUS);
		CASE_MACRO_2_MACRO_STR(CMD_ID_CPROC_ENABLE);
		CASE_MACRO_2_MACRO_STR(CMD_ID_CPROC_DISABLE);
		CASE_MACRO_2_MACRO_STR(CMD_ID_CPROC_SET_CONTRAST);
		CASE_MACRO_2_MACRO_STR(CMD_ID_CPROC_SET_BRIGHTNESS);
		CASE_MACRO_2_MACRO_STR(CMD_ID_CPROC_SET_SATURATION);
		CASE_MACRO_2_MACRO_STR(CMD_ID_CPROC_SET_HUE);
		CASE_MACRO_2_MACRO_STR(CMD_ID_CPROC_GET_STATUS);
		CASE_MACRO_2_MACRO_STR(CMD_ID_GAMMA_ENABLE);
		CASE_MACRO_2_MACRO_STR(CMD_ID_GAMMA_DISABLE);
		CASE_MACRO_2_MACRO_STR(CMD_ID_SNR_ENABLE);
		CASE_MACRO_2_MACRO_STR(CMD_ID_SNR_DISABLE);
		CASE_MACRO_2_MACRO_STR(CMD_ID_TNR_ENABLE);
		CASE_MACRO_2_MACRO_STR(CMD_ID_TNR_DISABLE);
		CASE_MACRO_2_MACRO_STR(CMD_ID_TNR_CONFIG);
		CASE_MACRO_2_MACRO_STR(CMD_ID_SNR_CONFIG);
		CASE_MACRO_2_MACRO_STR(CMD_ID_BLS_SET_BLACK_LEVEL);
		CASE_MACRO_2_MACRO_STR(CMD_ID_BLS_GET_STATUS);
		CASE_MACRO_2_MACRO_STR(CMD_ID_DEGAMMA_ENABLE);
		CASE_MACRO_2_MACRO_STR(CMD_ID_DEGAMMA_DISABLE);
		CASE_MACRO_2_MACRO_STR(CMD_ID_DEGAMMA_SET_CURVE);
		CASE_MACRO_2_MACRO_STR(CMD_ID_DEGAMMA_GET_STATUS);
		CASE_MACRO_2_MACRO_STR(CMD_ID_SET_RAW_PKT_FMT);
		CASE_MACRO_2_MACRO_STR(CMD_ID_CAC_ENABLE);
		CASE_MACRO_2_MACRO_STR(CMD_ID_CAC_DISABLE);
		CASE_MACRO_2_MACRO_STR(CMD_ID_CAC_CONFIG);
		CASE_MACRO_2_MACRO_STR(CMD_ID_CAC_GET_STATUS);
		CASE_MACRO_2_MACRO_STR(CMD_ID_CNR_ENABLE);
		CASE_MACRO_2_MACRO_STR(CMD_ID_CNR_DISABLE);
		CASE_MACRO_2_MACRO_STR(CMD_ID_CNR_GET_STATUS);
		CASE_MACRO_2_MACRO_STR(CMD_ID_SEND_FRAME_CTRL);
		CASE_MACRO_2_MACRO_STR(CMD_ID_SEND_FRAME_INFO);
		CASE_MACRO_2_MACRO_STR(CMD_ID_SEND_BUFFER);
		CASE_MACRO_2_MACRO_STR(CMD_ID_SET_STREAM_PATH);
		CASE_MACRO_2_MACRO_STR(CMD_ID_SET_OUT_CH_PROP);
		CASE_MACRO_2_MACRO_STR(CMD_ID_SET_OUT_CH_FRAME_RATE_RATIO);
		CASE_MACRO_2_MACRO_STR(CMD_ID_ENABLE_OUT_CH);
		CASE_MACRO_2_MACRO_STR(CMD_ID_DISABLE_OUT_CH);
#ifdef CONFIG_ENABLE_WINSTON_NEW_TUNING_DATA
		CASE_MACRO_2_MACRO_STR(CMD_ID_GAMMA_GET_STATUS);
#else
		CASE_MACRO_2_MACRO_STR(CMD_ID_GAMMA_GET_STATUS);
#endif
		CASE_MACRO_2_MACRO_STR(CMD_ID_SHARPEN_ENABLE);
		CASE_MACRO_2_MACRO_STR(CMD_ID_SHARPEN_DISABLE);
		CASE_MACRO_2_MACRO_STR(CMD_ID_SHARPEN_CONFIG);
		CASE_MACRO_2_MACRO_STR(CMD_ID_IE_CONFIG);
		CASE_MACRO_2_MACRO_STR(CMD_ID_IE_ENABLE);
		CASE_MACRO_2_MACRO_STR(CMD_ID_IE_DISABLE);
		CASE_MACRO_2_MACRO_STR(CMD_ID_ENABLE_CLOCK_GATING);
		CASE_MACRO_2_MACRO_STR(CMD_ID_DISABLE_CLOCK_GATING);
		CASE_MACRO_2_MACRO_STR(CMD_ID_ENABLE_POWER_GATING);
		CASE_MACRO_2_MACRO_STR(CMD_ID_DISABLE_POWER_GATING);
		CASE_MACRO_2_MACRO_STR(CMD_ID_CONFIG_CVIP_SENSOR);
		CASE_MACRO_2_MACRO_STR(CMD_ID_SET_CVIP_BUF_LAYOUT);
		CASE_MACRO_2_MACRO_STR(CMD_ID_START_CVIP_SENSOR);
		CASE_MACRO_2_MACRO_STR(CMD_ID_SET_SENSOR_SLICE_NUM);
		CASE_MACRO_2_MACRO_STR(CMD_ID_M2M_CONFIG);
		CASE_MACRO_2_MACRO_STR(CMD_ID_CONFIG_MMHUB_PREFETCH);
		break;
	default:
		return "Unknown cmd";
	};
	return ret;
}


char *isp_dbg_get_resp_str(unsigned int cmd)
{
	char *ret;

	switch (cmd) {
		CASE_MACRO_2_MACRO_STR(RESP_ID_CMD_DONE);
		CASE_MACRO_2_MACRO_STR(RESP_ID_FRAME_DONE);
		CASE_MACRO_2_MACRO_STR(RESP_ID_FRAME_INFO);
		CASE_MACRO_2_MACRO_STR(RESP_ID_ERROR);
		//CASE_MACRO_2_MACRO_STR(RESP_ID_HEART_BEAT);

		CASE_MACRO_2_MACRO_STR(RESP_ID_SENSOR_GROUP_HOLD);
		CASE_MACRO_2_MACRO_STR(RESP_ID_SENSOR_GROUP_RELEASE);
		CASE_MACRO_2_MACRO_STR(RESP_ID_SENSOR_SET_AGAIN);
		CASE_MACRO_2_MACRO_STR(RESP_ID_SENSOR_SET_DGAIN);
		CASE_MACRO_2_MACRO_STR(RESP_ID_SENSOR_SET_ITIME);
		CASE_MACRO_2_MACRO_STR(RESP_ID_SENSOR_SET_HDR_HIGH_ITIME);
		CASE_MACRO_2_MACRO_STR(RESP_ID_SENSOR_SET_HDR_LOW_ITIME);
		CASE_MACRO_2_MACRO_STR(RESP_ID_SENSOR_SET_HDR_LOW_ITIME_RATIO);
		CASE_MACRO_2_MACRO_STR(RESP_ID_SENSOR_SET_HDR_EQUAL_ITIME);
		CASE_MACRO_2_MACRO_STR(RESP_ID_SENSOR_SET_HDR_HIGH_AGAIN);
		CASE_MACRO_2_MACRO_STR(RESP_ID_SENSOR_SET_HDR_LOW_AGAIN);
		CASE_MACRO_2_MACRO_STR(RESP_ID_SENSOR_SET_HDR_LOW_AGAIN_RATIO);
		CASE_MACRO_2_MACRO_STR(RESP_ID_SENSOR_SET_HDR_EQUAL_AGAIN);
//CASE_MACRO_2_MACRO_STR(RESP_ID_SENSOR_SET_HDR_HIGH_DGAIN);
//CASE_MACRO_2_MACRO_STR(RESP_ID_SENSOR_SET_HDR_LOW_DGAIN);
//CASE_MACRO_2_MACRO_STR(RESP_ID_SENSOR_SET_HDR_LOW_DGAIN_RATIO);
//CASE_MACRO_2_MACRO_STR(RESP_ID_SENSOR_SET_HDR_EQUAL_DGAIN);

		CASE_MACRO_2_MACRO_STR(RESP_ID_LENS_SET_POS);
		CASE_MACRO_2_MACRO_STR(RESP_ID_IMC_CAM_RESET_REQ);
		CASE_MACRO_2_MACRO_STR(RESP_ID_IMC_CAM_RESET_RECOVERY);
		CASE_MACRO_2_MACRO_STR(RESP_ID_IMC_WAIT_CVIP_ACK_OVERTIME);
		CASE_MACRO_2_MACRO_STR(RESP_ID_IMC_WAIT_CVIP_IN_IMAGE_OVERTIME);
		//CASE_MACRO_2_MACRO_STR(RESP_ID_FLASH_SET_POWER);
		//CASE_MACRO_2_MACRO_STR(RESP_ID_FLASH_SET_ON);
		//CASE_MACRO_2_MACRO_STR(RESP_ID_FLASH_SET_OFF);
		break;
	default:
		return "Unknown respid";
	};
	return ret;
}


char *isp_dbg_get_pvt_fmt_str(int fmt /*enum pvt_img_fmt */)
{
	switch (fmt) {
	case PVT_IMG_FMT_INVALID:
		return "PVT_IMG_FMT_INVALID";
	case PVT_IMG_FMT_YV12:
		return "PVT_IMG_FMT_YV12";
	case PVT_IMG_FMT_I420:
		return "PVT_IMG_FMT_I420";
	case PVT_IMG_FMT_NV21:
		return "PVT_IMG_FMT_NV21";
	case PVT_IMG_FMT_NV12:
		return "PVT_IMG_FMT_NV12";
	case PVT_IMG_FMT_YUV422P:
		return "PVT_IMG_FMT_YUV422P";
	case PVT_IMG_FMT_YUV422_SEMIPLANAR:
		return "PVT_IMG_FMT_YUV422_SEMIPLANAR";
	case PVT_IMG_FMT_YUV422_INTERLEAVED:
		return "PVT_IMG_FMT_YUV422_INTERLEAVED";
	case PVT_IMG_FMT_L8:
		return "PVT_IMG_FMT_L8";
	default:
		return "Unknown PVT fmt";
	};
}

char *isp_dbg_get_af_mode_str(unsigned int mode /*enum _AfMode_t */)
{
	char *ret;

	switch (mode) {
		CASE_MACRO_2_MACRO_STR(AF_MODE_INVALID);
		CASE_MACRO_2_MACRO_STR(AF_MODE_MANUAL);
		CASE_MACRO_2_MACRO_STR(AF_MODE_ONE_SHOT);
		CASE_MACRO_2_MACRO_STR(AF_MODE_CONTINUOUS_PICTURE);
		CASE_MACRO_2_MACRO_STR(AF_MODE_CONTINUOUS_VIDEO);
		CASE_MACRO_2_MACRO_STR(AF_MODE_MAX);
			break;
	default:
		return "Unknown af mode";
	};
	return ret;
}

char *isp_dbg_get_ae_mode_str(unsigned int mode /*enum _AeMode_t */)
{
	char *ret;

	switch (mode) {
		CASE_MACRO_2_MACRO_STR(AE_MODE_INVALID);
		CASE_MACRO_2_MACRO_STR(AE_MODE_MANUAL);
		CASE_MACRO_2_MACRO_STR(AE_MODE_AUTO);
		CASE_MACRO_2_MACRO_STR(AE_MODE_MAX);
			break;
	default:
		return "Unknown ae mode";
	};
	return ret;
};

char *isp_dbg_get_awb_mode_str(unsigned int mode /*enum _AwbMode_t */)
{
	char *ret;

	switch (mode) {
		CASE_MACRO_2_MACRO_STR(AWB_MODE_INVALID);
		CASE_MACRO_2_MACRO_STR(AWB_MODE_MANUAL);
		CASE_MACRO_2_MACRO_STR(AWB_MODE_AUTO);
		CASE_MACRO_2_MACRO_STR(AWB_MODE_MAX);
			break;
	default:
		return "Unknown awb mode";
	};
	return ret;
}

char *isp_dbg_get_scene_mode_str(unsigned int mode /*enum isp_scene_mode */)
{
	char *ret;

	switch (mode) {
		CASE_MACRO_2_MACRO_STR(ISP_SCENE_MODE_AUTO);
		CASE_MACRO_2_MACRO_STR(ISP_SCENE_MODE_MACRO);
		CASE_MACRO_2_MACRO_STR(ISP_SCENE_MODE_PORTRAIT);
		CASE_MACRO_2_MACRO_STR(ISP_SCENE_MODE_SPORT);
		CASE_MACRO_2_MACRO_STR(ISP_SCENE_MODE_SNOW);
		CASE_MACRO_2_MACRO_STR(ISP_SCENE_MODE_NIGHT);
		CASE_MACRO_2_MACRO_STR(ISP_SCENE_MODE_BEACH);
		CASE_MACRO_2_MACRO_STR(ISP_SCENE_MODE_SUNSET);
		CASE_MACRO_2_MACRO_STR(ISP_SCENE_MODE_CANDLELIGHT);
		CASE_MACRO_2_MACRO_STR(ISP_SCENE_MODE_LANDSCAPE);
		CASE_MACRO_2_MACRO_STR(ISP_SCENE_MODE_NIGHTPORTRAIT);
		CASE_MACRO_2_MACRO_STR(ISP_SCENE_MODE_BACKLIT);
		CASE_MACRO_2_MACRO_STR(ISP_SCENE_MODE_BARCODE);
			break;
	default:
		return "Unknown scene mode";
	};
	return ret;
};

char *isp_dbg_get_stream_str(
	unsigned int stream /*enum fw_cmd_resp_stream_id */)
{
	switch (stream) {
	case FW_CMD_RESP_STREAM_ID_GLOBAL:
		return "STREAM_GLOBAL";
	case FW_CMD_RESP_STREAM_ID_1:
		return "STREAM1";
	case FW_CMD_RESP_STREAM_ID_2:
		return "STREAM2";
	case FW_CMD_RESP_STREAM_ID_3:
		return "STREAM3";
	default:
		return "Unkonow streamID";
	}
}

char *isp_dbg_get_para_str(unsigned int para /*enum para_id */)
{
	char *ret;

	switch (para) {
		/*para value type is pvt_img_fmtdone */
		CASE_MACRO_2_MACRO_STR(PARA_ID_DATA_FORMAT);
		/*para value type is pvt_img_res_fps_pitchdone */
		CASE_MACRO_2_MACRO_STR(PARA_ID_DATA_RES_FPS_PITCH);
		/*para value type is pvt_img_sizedone */
		CASE_MACRO_2_MACRO_STR(PARA_ID_IMAGE_SIZE);
		/*para value type is pvt_img_sizedone */
		CASE_MACRO_2_MACRO_STR(PARA_ID_JPEG_THUMBNAIL_SIZE);
		/*para value type is pvt_img_sizedone */
		CASE_MACRO_2_MACRO_STR(PARA_ID_YUV_THUMBNAIL_SIZE);
		/*para value type is pvt_img_sizedone */
		CASE_MACRO_2_MACRO_STR(PARA_ID_YUV_THUMBNAIL_PITCH);
		CASE_MACRO_2_MACRO_STR(PARA_ID_BRIGHTNESS);/*pvt_int32done */
		CASE_MACRO_2_MACRO_STR(PARA_ID_CONTRAST);/*pvt_int32done */
		CASE_MACRO_2_MACRO_STR(PARA_ID_HUE);	/*pvt_int32done */
		CASE_MACRO_2_MACRO_STR(PARA_ID_SATURATION);/*pvt_int32 done */
		/*unsigned int done */
		CASE_MACRO_2_MACRO_STR(PARA_ID_COLOR_TEMPERATURE);
		/*pvt_flicker_mode done */
		CASE_MACRO_2_MACRO_STR(PARA_ID_FLICKER_MODE);
		/*pvt_int32done */
		CASE_MACRO_2_MACRO_STR(PARA_ID_COLOR_ENABLE);
		CASE_MACRO_2_MACRO_STR(PARA_ID_GAMMA);	/*int*done */
		CASE_MACRO_2_MACRO_STR(PARA_ID_ISO);	/*struct _AeIso_t **/
		CASE_MACRO_2_MACRO_STR(PARA_ID_EV);	/*struct _AeEv_t **/
		/*unsigned int**/
		CASE_MACRO_2_MACRO_STR(PARA_ID_MAIN_JPEG_COMPRESS_RATE);
		/*unsigned int**/
		CASE_MACRO_2_MACRO_STR(PARA_ID_THBNAIL_JPEG_COMPRESS_RATE);
		/*pdata_wb_t); redefine */
		CASE_MACRO_2_MACRO_STR(PARA_ID_WB_MODE);
		/*pdata_exposure_t); */
		CASE_MACRO_2_MACRO_STR(PARA_ID_EXPOSURE);
		CASE_MACRO_2_MACRO_STR(PARA_ID_FOCUS);	/*pdata_focus_t); */
		CASE_MACRO_2_MACRO_STR(PARA_ID_ROI);	/*pdata_roi_t */
		/*pdata_photo_mode_t */
		CASE_MACRO_2_MACRO_STR(PARA_ID_PHOTO_MODE);
		CASE_MACRO_2_MACRO_STR(PARA_ID_FLASH);	/*pdata_flash_t */
		CASE_MACRO_2_MACRO_STR(PARA_ID_SCENE);/*pvt_scene_mode done */
		/*pdata_photo_thumbnail_t */
		CASE_MACRO_2_MACRO_STR(PARA_ID_PHOTO_THUMBNAIL);
			break;
	default:
		return "Unknown paraId";
	};
	return ret;
}

#ifdef TBD_LATER //#if 0
char *isp_dbg_get_iso_str(unsigned int para /*struct _AeIso_t */)
{
	char *ret;

	switch (para) {
		CASE_MACRO_2_MACRO_STR(AE_ISO_INVALID);
		CASE_MACRO_2_MACRO_STR(AE_ISO_AUTO);
		CASE_MACRO_2_MACRO_STR(AE_ISO_100);
		CASE_MACRO_2_MACRO_STR(AE_ISO_200);
		CASE_MACRO_2_MACRO_STR(AE_ISO_400);
		CASE_MACRO_2_MACRO_STR(AE_ISO_800);
		CASE_MACRO_2_MACRO_STR(AE_ISO_1600);
		CASE_MACRO_2_MACRO_STR(AE_ISO_3200);
		CASE_MACRO_2_MACRO_STR(AE_ISO_6400);
		CASE_MACRO_2_MACRO_STR(AE_ISO_12800);
		CASE_MACRO_2_MACRO_STR(AE_ISO_25600);
		CASE_MACRO_2_MACRO_STR(AE_ISO_MAX);
			break;
	default:
		return "bad iso value";
	};
	return ret;
};
#endif
#if	defined(PER_FRAME_CTRL)
char *isp_dbg_get_flash_mode_str(
	unsigned int mode /*FlashMode_t */)
{
	char *ret;

	switch (mode) {
		CASE_MACRO_2_MACRO_STR(FLASH_MODE_INVALID);
		CASE_MACRO_2_MACRO_STR(FLASH_MODE_OFF);
		CASE_MACRO_2_MACRO_STR(FLASH_MODE_ON);
		CASE_MACRO_2_MACRO_STR(FLASH_MODE_AUTO);
		CASE_MACRO_2_MACRO_STR(FLASH_MODE_MAX);
			break;
	default:
		return "bad flash mode";
	};
	return ret;
}

char *isp_dbg_get_flash_assist_mode_str(
	unsigned int mode /*FocusAssistMode_t */)
{
	char *ret;

	switch (mode) {
		CASE_MACRO_2_MACRO_STR(FOCUS_ASSIST_MODE_INVALID);
		CASE_MACRO_2_MACRO_STR(FOCUS_ASSIST_MODE_OFF);
		CASE_MACRO_2_MACRO_STR(FOCUS_ASSIST_MODE_ON);
		CASE_MACRO_2_MACRO_STR(FOCUS_ASSIST_MODE_AUTO);
		CASE_MACRO_2_MACRO_STR(FOCUS_ASSIST_MODE_MAX);
			break;
	default:
		return "bad flash assist mode";
	};
	return ret;
};

char *isp_dbg_get_red_eye_mode_str(
	unsigned int mode /*RedEyeMode_t */)
{
	char *ret;

	switch (mode) {
		CASE_MACRO_2_MACRO_STR(RED_EYE_MODE_INVALID);
		CASE_MACRO_2_MACRO_STR(RED_EYE_MODE_OFF);
		CASE_MACRO_2_MACRO_STR(RED_EYE_MODE_ON);
		CASE_MACRO_2_MACRO_STR(RED_EYE_MODE_MAX);
			break;
	default:
		return "bad red eye mode";
	};
	return ret;
};
#endif
char *isp_dbg_get_anti_flick_str(
	unsigned int type /*enum _AeFlickerType_t */)
{
	char *ret;

	switch (type) {
		CASE_MACRO_2_MACRO_STR(AE_FLICKER_TYPE_INVALID);
		CASE_MACRO_2_MACRO_STR(AE_FLICKER_TYPE_OFF);
		CASE_MACRO_2_MACRO_STR(AE_FLICKER_TYPE_50HZ);
		CASE_MACRO_2_MACRO_STR(AE_FLICKER_TYPE_60HZ);
		CASE_MACRO_2_MACRO_STR(AE_FLICKER_TYPE_MAX);
			break;
	default:
		return "bad anti flick type";
	};
	return ret;
};

void isp_dbg_show_af_ctrl_info(
	void *p_in /*struct _AfCtrlInfo_t **/)
{
	struct _AfCtrlInfo_t *p = p_in;

	ISP_PR_INFO("struct _AfCtrlInfo_t info:\n");
	ISP_PR_INFO("lockState %u,bSupportFocus %u, lenPos %u\n",
			p->lockState, p->bSupportFocus, p->lensPos);
	ISP_PR_INFO("focusAssistOn %u,focusAssistPowerLevel %u\n",
			p->focusAssistOn, p->focusAssistPowerLevel);
	ISP_PR_INFO("mode %u,focusAssistMode %u,searchState%u\n",
			p->mode, p->focusAssistMode, p->searchState);
}

void isp_dbg_show_ae_ctrl_info(
	void *p_in /*struct _AeCtrlInfo_t * */)
{
	struct _AeCtrlInfo_t *p = p_in;

	ISP_PR_INFO("struct _AeCtrlInfo_t info:\n");
	ISP_PR_INFO("lockState %u,itime %u,aGain %u,dGain %u\n",
			   p->lockState, p->itime[0], p->aGain[0], p->dGain[0]);
	//ISP_PR_INFO("hdrEnabled %u,hdrParamValid %u\n",
			   //p->hdrEnabled, p->hdrParamValid);
//	ISP_PR_INFO("flashOn %u,flashPowerLevel %u\n",
//			   p->flashOn, p->flashPowerLevel);
	ISP_PR_INFO("Aemode %u,flickerType %u,searchState %u\n",
			   p->mode, p->flickerType, p->searchState);
//	ISP_PR_INFO("iso %u,ev %u/%u\n",
//			   p->iso.iso, p->ev.numerator, p->ev.denominator);
	//ISP_PR_INFO("flashMode %u,redEyeMode %u\n",
	//		   p->flashMode, p->redEyeMode);
};


void isp_dbg_show_frame3_info(void *p_in	/*struct _FrameInfo_t * */
			      , char *prefix)
{
	struct _FrameInfo_t *p = p_in;

	ISP_PR_INFO("rgbir:Mode3Frame info(%p):\n", p_in);
	ISP_PR_INFO("poc %u, timeStampLo %u, timeStampHi %u\n",
			   p->poc, p->timeStampLo, p->timeStampHi);
	ISP_PR_INFO("ctlCmd 0x%x\n", p->frameControlCmd.cmd.cmdId);
	ISP_PR_INFO("%s yuvCapCmd 0x%x, %llx(%u)\n",
			prefix, p->yuvCapCmd.cmd.cmdId,
			(((unsigned long long) p->yuvCapCmd.buffer.bufBaseAHi)
				<< 32) |
			((unsigned long long) p->yuvCapCmd.buffer.bufBaseALo),
			p->yuvCapCmd.buffer.bufSizeA);
	ISP_PR_INFO("rawCapCmd 0x%x\n", p->rawCapCmd.cmd.cmdId);
	ISP_PR_INFO("frameRate %u\n", p->frameRate);
	ISP_PR_INFO
	    ("zoomCmd 0x%x, zoomWnd(%u,%u,%u,%u)\n",
	     p->zoomCmd.cmdId, p->zoomWindow.h_offset,
	     p->zoomWindow.v_offset, p->zoomWindow.h_size,
	     p->zoomWindow.v_size);

	ISP_PR_INFO("RAW_PKT_FMT_%u,rawWidthInByte %u,cfaPattern %u\n",
			   p->rawPktFmt, p->rawWidthInByte, p->cfaPattern);

	isp_dbg_show_af_ctrl_info(&p->afCtrlInfo);
//	isp_dbg_show_ae_ctrl_info(&p->aeCtrlInfo);
}

char *isp_dbg_get_focus_state_str(
	unsigned int state /*AfSearchState_t */)
{
	switch (state) {
	case AF_SEARCH_STATE_INVALID:
		return "FOCUS_INVALID";
	case AF_SEARCH_STATE_INIT:
		return "FOCUS_INIT";
	case AF_SEARCH_STATE_SEARCHING:
		return "FOCUS_SEARCHING";
	case AF_SEARCH_STATE_CONVERGED:
		return "FOCUS_CONVERGED";
	case AF_SEARCH_STATE_FAILED:
		return "FOCUS_FAILED";
	default:
		return "FOCUS_UNKNOWN";
	}
}

char *isp_dbg_get_work_buf_str(
	unsigned int type /*isp_work_buf_type */)
{
	char *ret;

	switch (type) {
		CASE_MACRO_2_MACRO_STR(ISP_WORK_BUF_TYPE_CALIB);
		CASE_MACRO_2_MACRO_STR(ISP_WORK_BUF_TYPE_STREAM_TMP);
		CASE_MACRO_2_MACRO_STR(ISP_WORK_BUF_TYPE_CMD_RESP);
		CASE_MACRO_2_MACRO_STR(ISP_WORK_BUF_TYPE_LB_RAW_DATA);
		CASE_MACRO_2_MACRO_STR(ISP_WORK_BUF_TYPE_LB_RAW_INFO);
		CASE_MACRO_2_MACRO_STR(ISP_WORK_BUF_TYPE_TNR_TMP);
		CASE_MACRO_2_MACRO_STR(ISP_WORK_BUF_TYPE_HDR_STATUS);
		CASE_MACRO_2_MACRO_STR(ISP_WORK_BUF_TYPE_IIC_TMP);
		CASE_MACRO_2_MACRO_STR(ISP_WORK_BUF_TYPE_STAGE2_TMP);
		CASE_MACRO_2_MACRO_STR(ISP_WORK_BUF_TYPE_UV_TMP);
		CASE_MACRO_2_MACRO_STR(ISP_WORK_BUF_TYPE_INDIR_FW_CMD_PKG);
		CASE_MACRO_2_MACRO_STR(ISP_WORK_BUF_TYPE_EMB_DATA);
			break;
	default:
		return "unknown work buf type";
	}
	return ret;
}

;

char *isp_dbg_get_reg_name(unsigned int reg)
{
	char *ret;

	switch (reg) {
//CASE_MACRO_2_MACRO_STR(mmRSMU_SW_MMIO_PUB_IND_ADDR_0_ISP);
//CASE_MACRO_2_MACRO_STR(mmRSMU_SW_MMIO_PUB_IND_DATA_0_ISP);
		CASE_MACRO_2_MACRO_STR(mmRSMU_PGFSM_CONTROL_ISP);
		CASE_MACRO_2_MACRO_STR(mmISP_POWER_STATUS);
//		CASE_MACRO_2_MACRO_STR(mmISP_CORE_ISP_CTRL);
		CASE_MACRO_2_MACRO_STR(mmISP_CCPU_CNTL);
		CASE_MACRO_2_MACRO_STR(mmISP_SOFT_RESET);
//		CASE_MACRO_2_MACRO_STR(mmISP_CORE_VI_IRCL);
//		CASE_MACRO_2_MACRO_STR(mmISP_MIPI_VI_IRCL);
//		CASE_MACRO_2_MACRO_STR(mmISP_CORE_MI_MP_STALL);
//		CASE_MACRO_2_MACRO_STR(mmISP_CORE_MI_VP_STALL);
//		CASE_MACRO_2_MACRO_STR(mmISP_CORE_MI_PP_STALL);
//		CASE_MACRO_2_MACRO_STR(mmISP_CORE_MI_RD_DMA_STALL);
//		CASE_MACRO_2_MACRO_STR(mmISP_MIPI_MI_MIPI0_STALL);
		CASE_MACRO_2_MACRO_STR(mmI2C2_IC_ENABLE);
		CASE_MACRO_2_MACRO_STR(mmI2C2_IC_INTR_MASK);
		CASE_MACRO_2_MACRO_STR(mmI2C2_IC_SS_SCL_HCNT);
		CASE_MACRO_2_MACRO_STR(mmI2C2_IC_SS_SCL_LCNT);
		CASE_MACRO_2_MACRO_STR(mmI2C2_IC_FS_SCL_HCNT);
		CASE_MACRO_2_MACRO_STR(mmI2C2_IC_FS_SCL_LCNT);
		CASE_MACRO_2_MACRO_STR(mmI2C2_IC_CON);
		CASE_MACRO_2_MACRO_STR(mmI2C2_IC_TAR);
		CASE_MACRO_2_MACRO_STR(mmI2C2_IC_TX_TL);
		CASE_MACRO_2_MACRO_STR(mmI2C2_IC_RX_TL);
		CASE_MACRO_2_MACRO_STR(mmI2C1_IC_ENABLE);
		CASE_MACRO_2_MACRO_STR(mmI2C1_IC_INTR_MASK);
		CASE_MACRO_2_MACRO_STR(mmI2C1_IC_SS_SCL_HCNT);
		CASE_MACRO_2_MACRO_STR(mmI2C1_IC_SS_SCL_LCNT);
		CASE_MACRO_2_MACRO_STR(mmI2C1_IC_FS_SCL_HCNT);
		CASE_MACRO_2_MACRO_STR(mmI2C1_IC_FS_SCL_LCNT);
		CASE_MACRO_2_MACRO_STR(mmI2C1_IC_CON);
		CASE_MACRO_2_MACRO_STR(mmI2C1_IC_TAR);
		CASE_MACRO_2_MACRO_STR(mmI2C1_IC_TX_TL);
		CASE_MACRO_2_MACRO_STR(mmI2C1_IC_RX_TL);
		CASE_MACRO_2_MACRO_STR(mmISP_RB_BASE_LO1);
		CASE_MACRO_2_MACRO_STR(mmISP_RB_BASE_HI1);
		CASE_MACRO_2_MACRO_STR(mmISP_RB_SIZE1);
		CASE_MACRO_2_MACRO_STR(mmISP_RB_RPTR1);
		CASE_MACRO_2_MACRO_STR(mmISP_RB_WPTR1);
		CASE_MACRO_2_MACRO_STR(mmISP_RB_BASE_LO9);
		CASE_MACRO_2_MACRO_STR(mmISP_RB_BASE_HI9);
		CASE_MACRO_2_MACRO_STR(mmISP_RB_SIZE9);
		CASE_MACRO_2_MACRO_STR(mmISP_RB_RPTR9);
		CASE_MACRO_2_MACRO_STR(mmISP_RB_WPTR9);
		CASE_MACRO_2_MACRO_STR(mmISP_RB_BASE_LO2);
		CASE_MACRO_2_MACRO_STR(mmISP_RB_BASE_HI2);
		CASE_MACRO_2_MACRO_STR(mmISP_RB_SIZE2);
		CASE_MACRO_2_MACRO_STR(mmISP_RB_RPTR2);
		CASE_MACRO_2_MACRO_STR(mmISP_RB_WPTR2);
		CASE_MACRO_2_MACRO_STR(mmISP_RB_BASE_LO10);
		CASE_MACRO_2_MACRO_STR(mmISP_RB_BASE_HI10);
		CASE_MACRO_2_MACRO_STR(mmISP_RB_SIZE10);
		CASE_MACRO_2_MACRO_STR(mmISP_RB_RPTR10);
		CASE_MACRO_2_MACRO_STR(mmISP_RB_WPTR10);
		CASE_MACRO_2_MACRO_STR(mmISP_RB_BASE_LO3);
		CASE_MACRO_2_MACRO_STR(mmISP_RB_BASE_HI3);
		CASE_MACRO_2_MACRO_STR(mmISP_RB_SIZE3);
		CASE_MACRO_2_MACRO_STR(mmISP_RB_RPTR3);
		CASE_MACRO_2_MACRO_STR(mmISP_RB_WPTR3);
		CASE_MACRO_2_MACRO_STR(mmISP_RB_BASE_LO11);
		CASE_MACRO_2_MACRO_STR(mmISP_RB_BASE_HI11);
		CASE_MACRO_2_MACRO_STR(mmISP_RB_SIZE11);
		CASE_MACRO_2_MACRO_STR(mmISP_RB_RPTR11);
		CASE_MACRO_2_MACRO_STR(mmISP_RB_WPTR11);
		CASE_MACRO_2_MACRO_STR(mmISP_RB_BASE_LO4);
		CASE_MACRO_2_MACRO_STR(mmISP_RB_BASE_HI4);
		CASE_MACRO_2_MACRO_STR(mmISP_RB_SIZE4);
		CASE_MACRO_2_MACRO_STR(mmISP_RB_RPTR4);
		CASE_MACRO_2_MACRO_STR(mmISP_RB_WPTR4);
		CASE_MACRO_2_MACRO_STR(mmISP_RB_BASE_LO12);
		CASE_MACRO_2_MACRO_STR(mmISP_RB_BASE_HI12);
		CASE_MACRO_2_MACRO_STR(mmISP_RB_SIZE12);
		CASE_MACRO_2_MACRO_STR(mmISP_RB_RPTR12);
		CASE_MACRO_2_MACRO_STR(mmISP_RB_WPTR12);
		CASE_MACRO_2_MACRO_STR(mmISP_LOG_RB_BASE_LO);
		CASE_MACRO_2_MACRO_STR(mmISP_LOG_RB_BASE_HI);
		CASE_MACRO_2_MACRO_STR(mmISP_LOG_RB_SIZE);
		CASE_MACRO_2_MACRO_STR(mmISP_LOG_RB_WPTR);
		CASE_MACRO_2_MACRO_STR(mmISP_LOG_RB_RPTR);
		CASE_MACRO_2_MACRO_STR(mmISP_CCPU_CACHE_OFFSET0_HI);
		CASE_MACRO_2_MACRO_STR(mmISP_CCPU_CACHE_OFFSET0_LO);
		CASE_MACRO_2_MACRO_STR(mmISP_CCPU_CACHE_SIZE0);
		CASE_MACRO_2_MACRO_STR(mmISP_CCPU_CACHE_OFFSET1_HI);
		CASE_MACRO_2_MACRO_STR(mmISP_CCPU_CACHE_OFFSET1_LO);
		CASE_MACRO_2_MACRO_STR(mmISP_CCPU_CACHE_SIZE1);
		CASE_MACRO_2_MACRO_STR(mmISP_CCPU_CACHE_OFFSET2_HI);
		CASE_MACRO_2_MACRO_STR(mmISP_CCPU_CACHE_OFFSET2_LO);
		CASE_MACRO_2_MACRO_STR(mmISP_CCPU_CACHE_SIZE2);
		CASE_MACRO_2_MACRO_STR(mmISP_STATUS);
		break;
	case 0x385c:
		ret = "mmHDP_MEM_COHERENCY_FLUSH_CNTL";
		break;
	default:
		ret = "unknown reg";
		break;
	}
	return ret;

};


void isp_dbg_dump_ae_status(void *p /*struct _AeStatus_t **/)
{
	struct _AeStatus_t *v = (struct _AeStatus_t *)p;

	ISP_PR_INFO(
"struct _AeStatus_t mode %u, flickerType %u, lochstate %u, searchState %u\n",
		v->aeCtrlInfo.mode, v->aeCtrlInfo.flickerType,
		v->aeCtrlInfo.lockState, v->aeCtrlInfo.searchState);
	//ISP_PR_INFO(
		//"isoCap [%u,%u], iso %u\n",
		//v->isoCap.minIso.iso, v->isoCap.maxIso.iso, v->iso.iso);
	//ISP_PR_INFO(
		//"evCap [%u/%u,%u/%u,%u/%u], ev %u/%u\n",
		//v->evCap.minEv.numerator, v->evCap.minEv.denominator,
		//v->evCap.maxEv.numerator, v->evCap.maxEv.denominator,
		//v->evCap.stepEv.numerator, v->evCap.stepEv.denominator,
		//v->ev.numerator, v->ev.denominator);
//	ISP_PR_INFO(
//		"region[%u,%u,%u,%u]\n",
//		v->aeCtrlInfo.region.h_offset, v->aeCtrlInfo.region.v_offset,
//		v->aeCtrlInfo.region.h_size, v->aeCtrlInfo.region.v_size);

//	ISP_PR_INFO(
//		"itime %u, aGain %u, dGain %u, minItime %u, maxItime %u\n",
//		v->aeCtrlInfo.itime, v->aGain, v->dGain,
//		v->minItime, v->maxItime);
//	ISP_PR_INFO(
//		"minAGain %u, maxAGain %u, minDGain %u, maxDgain %u\n",
//		v->minAGain, v->maxAGain, v->minDGain, v->maxDgain);
//	ISP_PR_INFO(
//	"hdrEnabled %u, hdrItimeType %u, hdrAGainType %u, hdrDGainType %u\n",
//		v->hdrEnabled, v->hdrItimeType, v->hdrAGainType,
//		v->hdrDGainType);
}
char *isp_dbg_get_mipi_out_path_str(
	int path/*enum _MipiPathOutType_t*/)
{
	switch ((enum _MipiPathOutType_t)path) {
	case MIPI_PATH_OUT_TYPE_INVALID:
		return "invalid";
	case MIPI_PATH_OUT_TYPE_DMABUF_TO_ISP:
		return "DMABUF_TO_ISP";
	case MIPI_PATH_OUT_TYPE_DMABUF_TO_HOST:
		return "DMABUF_TO_HOST";
	case MIPI_PATH_OUT_TYPE_DIRECT_TO_ISP:
		return "DIRECT_TO_ISP";
	default:
		return "fail unknown path";
	}
}

char *isp_dbg_get_out_ch_str(int ch /*enum _IspPipeOutCh_t*/)
{
	switch ((enum _IspPipeOutCh_t)ch) {
	case ISP_PIPE_OUT_CH_PREVIEW:
		return "prev";
	case ISP_PIPE_OUT_CH_VIDEO:
		return "video";
	case ISP_PIPE_OUT_CH_STILL:
		return "still";
	case ISP_PIPE_OUT_CH_CVP_CV:
		return "cvp_cv";
	case ISP_PIPE_OUT_CH_X86_CV:
		return "x86_cv";
	case ISP_PIPE_OUT_CH_IR:
		return "ir";
	case ISP_PIPE_OUT_CH_RAW:
		return "raw";
//	case ISP_PIPE_OUT_CH_CVP_CV2:
//		return "cvp_cv2";
	default:
		return "fail unknown channel";
	}
};



