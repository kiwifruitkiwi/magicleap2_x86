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

#include "os_base_type.h"
#include "isp_common.h"
#include "log.h"
#include "buffer_mgr.h"
#include "isp_fw_if/drv_isp_if.h"

uint32 g_log_level_ispm = TRACE_LEVEL_VERBOSE;

uint32 dbg_flag;

char *isp_dbg_get_isp_status_str(uint32 status)
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

char *dbg_get_snr_intf_type_str(SensorIntfType_t type)
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
	SensorProp_t *prop;
	CmdSetSensorProp_t *cfg;

	if (sensor_cfg == NULL)
		return;
	cfg = (CmdSetSensorProp_t *) sensor_cfg;
	prop = &cfg->sensorProp;

	isp_dbg_print_info("cid %d, intfType %s\n", cfg->sensorId,
			dbg_get_snr_intf_type_str(prop->intfType));

	if (prop->intfType == SENSOR_INTF_TYPE_MIPI) {
		isp_dbg_print_info("mipi.num_lanes %d\n",
				prop->intfProp.mipi.numLanes);
		isp_dbg_print_info("mipi.MIPI_VIRTUAL_CHANNEL_%d\n",
				prop->intfProp.mipi.virtChannel);
		isp_dbg_print_info("mipi.data_type %d\n",
				prop->intfProp.mipi.dataType);
		isp_dbg_print_info("mipi.comp_scheme %d\n",
				prop->intfProp.mipi.compScheme);
		isp_dbg_print_info("mipi.pred_block %d\n",
				prop->intfProp.mipi.predBlock);
		isp_dbg_print_info("cfaPattern %d\n", prop->cfaPattern);

	} else if (prop->intfType == SENSOR_INTF_TYPE_PARALLEL) {
		isp_dbg_print_info("parallel.dataType %d\n",
				prop->intfProp.parallel.dataType);
		isp_dbg_print_info("parallel.hPol %d\n",
				prop->intfProp.parallel.hPol);
		isp_dbg_print_info("parallel.vPol %d\n",
				prop->intfProp.parallel.vPol);
		isp_dbg_print_info("parallel.edge %d\n",
				prop->intfProp.parallel.edge);
		isp_dbg_print_info("parallel.ycSeq %d\n",
				prop->intfProp.parallel.ycSeq);
		isp_dbg_print_info("parallel.subSampling %d\n",
				prop->intfProp.parallel.subSampling);

		isp_dbg_print_info("cfaPattern %d\n", prop->cfaPattern);
	};

}

void dbg_show_sensor_caps(void *sensor_cap /*isp_sensor_prop_t */)
{
	isp_sensor_prop_t *cap;

	if (sensor_cap == NULL)
		return;
	cap = (isp_sensor_prop_t *) sensor_cap;

	isp_dbg_print_info("dgain range [%u,%u]\n",
			cap->exposure.min_digi_gain,
			cap->exposure.max_digi_gain);
	isp_dbg_print_info("again range [%u,%u]\n",
			cap->exposure.min_gain, cap->exposure.max_gain);

	isp_dbg_print_info("itime range [%u,%u]\n",
			cap->exposure.min_integration_time,
			cap->exposure.max_integration_time);
	isp_dbg_print_info("lens range [%u,%u]\n",
			cap->lens_pos.min_pos, cap->lens_pos.max_pos);

	isp_dbg_print_info("windows [%u,%u,%u,%u]\n",
			cap->window.h_offset, cap->window.v_offset,
			cap->window.h_size, cap->window.v_size);

	isp_dbg_print_info("framerate %u\n", cap->frame_rate);

	isp_dbg_print_info("hdr_valid %u\n", cap->hdr_valid);
};

void isp_dbg_show_map_info(void *in /*struct isp_mapped_buf_info */)
{
	struct isp_mapped_buf_info *p = in;

	if (p == NULL)
		return;
	isp_dbg_print_info("y sys:mc:len %llx:%llx:%u\n",
			p->y_map_info.sys_addr, p->y_map_info.mc_addr,
			p->y_map_info.len);
	isp_dbg_print_info("u sys:mc:len %llx:%llx:%u\n",
			p->u_map_info.sys_addr, p->u_map_info.mc_addr,
			p->u_map_info.len);
	isp_dbg_print_info("v sys:mc:len %llx:%llx:%u\n",
			p->v_map_info.sys_addr, p->v_map_info.mc_addr,
			p->v_map_info.len);
};

char *isp_dbg_get_buf_src_str(uint32 src)
{
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
}

char *isp_dbg_get_buf_done_str(uint32 status)
{
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
};

char *isp_dbg_get_img_fmt_str(void *in /*ImageFormat_t **/)
{
	ImageFormat_t *t;

	t = (ImageFormat_t *) in;

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

void isp_dbg_show_bufmeta_info(char *pre, uint32 cid,
			 void *in, void *origBuf)
{
	BufferMetaInfo_t *p;
	struct sys_img_buf_handle *orig;

	if (in == NULL)
		return;
	if (pre == NULL)
		pre = "";
	p = (BufferMetaInfo_t *) in;
	orig = (struct sys_img_buf_handle *)origBuf;

	isp_dbg_print_info("%s(%s)%u %p(%u), en:%d,stat:%s(%u),src:%s\n",
			pre,
			isp_dbg_get_img_fmt_str(&p->imageProp.imageFormat),
			cid, orig->virtual_addr, orig->len, p->enabled,
			isp_dbg_get_buf_done_str(p->statusEx.status),
			p->statusEx.status,
			isp_dbg_get_buf_src_str(p->source));
}

void isp_dbg_show_img_prop(char *pre, void *in /*ImageProp_t **/)
{
	ImageProp_t *p = (ImageProp_t *) in;

	isp_dbg_print_info("%s fmt:%s(%d),w:h(%d:%d),lp:cp(%d:%d)\n",
			 pre, isp_dbg_get_out_fmt_str(p->imageFormat),
			p->imageFormat, p->width, p->height, p->lumaPitch,
			 p->chromaPitch);
};

void dbg_show_drv_settings(void *setting /*struct driver_settings */)
{
	struct driver_settings *p = (struct driver_settings *)setting;

	isp_dbg_print_info("rear_sensor %s\n", p->rear_sensor);
	isp_dbg_print_info("rear_vcm %s\n", p->rear_vcm);
	isp_dbg_print_info("rear_flash %s\n", p->rear_flash);
	isp_dbg_print_info("frontl_sensor %s\n", p->frontl_sensor);
	isp_dbg_print_info("rear_sensor %s\n", p->frontr_sensor);
	isp_dbg_print_info("sensor_frame_rate %u\n", p->sensor_frame_rate);
	isp_dbg_print_info("af_default_auto %i\n", p->af_default_auto);
	isp_dbg_print_info("rear_sensor_type %i\n", p->rear_sensor_type);
	isp_dbg_print_info("frontl_sensor_type %i\n", p->frontl_sensor_type);
	isp_dbg_print_info("frontr_sensor_type %i\n", p->frontr_sensor_type);
	isp_dbg_print_info("ae_default_auto %i\n", p->ae_default_auto);
	isp_dbg_print_info("awb_default_auto %i\n", p->awb_default_auto);
	isp_dbg_print_info("fw_ctrl_3a %i\n", p->fw_ctrl_3a);
	isp_dbg_print_info("set_again_by_sensor_drv %i\n",
			p->set_again_by_sensor_drv);
	isp_dbg_print_info("set_itime_by_sensor_drv %i\n",
			p->set_itime_by_sensor_drv);
	isp_dbg_print_info("set_lens_pos_by_sensor_drv %i\n",
			p->set_lens_pos_by_sensor_drv);
	isp_dbg_print_info("do_fpga_to_sys_mem_cpy %i\n",
			p->do_fpga_to_sys_mem_cpy);
	isp_dbg_print_info("hdmi_support_enable %i\n", p->hdmi_support_enable);

	isp_dbg_print_info("fw_log_cfg_en %lu\n", p->fw_log_cfg_en);
	isp_dbg_print_info("fw_log_cfg_A %lu\n", p->fw_log_cfg_A);
	isp_dbg_print_info("fw_log_cfg_B %lu\n", p->fw_log_cfg_B);
	isp_dbg_print_info("fw_log_cfg_C %lu\n", p->fw_log_cfg_C);
	isp_dbg_print_info("fw_log_cfg_D %lu\n", p->fw_log_cfg_D);
	isp_dbg_print_info("fw_log_cfg_E %lu\n", p->fw_log_cfg_E);
	isp_dbg_print_info("fix_wb_lighting_index %i\n",
			p->fix_wb_lighting_index);
	isp_dbg_print_info("Stage2TempBufEnable %i\n",
			p->enable_stage2_temp_buf);
	isp_dbg_print_info("IgnoreStage2Meta %i\n", p->ignore_meta_at_stage2);
	isp_dbg_print_info("DriverLoadFw %i\n", p->driver_load_fw);
	isp_dbg_print_info("DisableDynAspRatioWnd %i\n",
			p->disable_dynamic_aspect_wnd);
	isp_dbg_print_info("tnr_enable %i\n", p->tnr_enable);
	isp_dbg_print_info("snr_enable %i\n", p->snr_enable);

	isp_dbg_print_info("PinOutputDumpEnable %i\n",
			p->dump_pin_output_to_file);
	isp_dbg_print_info("HighVideoCapEnable %i\n",
			p->enable_high_video_capabilities);
	isp_dbg_print_info("dpf_enable %i\n", p->dpf_enable);
	isp_dbg_print_info("output_metadata_in_log %i\n",
			p->output_metadata_in_log);
	isp_dbg_print_info("demosaic_thresh %u\n", p->demosaic_thresh);
	isp_dbg_print_info("enable_frame_drop_solution_for_frame_pair %u\n",
			p->enable_frame_drop_solution_for_frame_pair);
	isp_dbg_print_info("disable_ir_illuminator %u\n",
			p->disable_ir_illuminator);
	isp_dbg_print_info("ir_illuminator_ctrl %u\n", p->ir_illuminator_ctrl);
	isp_dbg_print_info("min_ae_roi_size_filter %u\n",
			p->min_ae_roi_size_filter);
	isp_dbg_print_info("reset_small_ae_roi %u\n", p->reset_small_ae_roi);
	isp_dbg_print_info("process_3a_by_host %u\n", p->process_3a_by_host);
	isp_dbg_print_info("low_light_fps %u\n", p->low_light_fps);
	isp_dbg_print_info("drv_log_level %u\n", p->drv_log_level);
	isp_dbg_print_info("scaler_param_tbl_idx %d\n",
					p->scaler_param_tbl_idx);
	isp_dbg_print_info("enable_using_fb_for_taking_raw %d\n",
					p->enable_using_fb_for_taking_raw);
};

char *isp_dbg_get_out_fmt_str(int32 fmt /*enum ImageFormat_t */)
{
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
}

char *isp_dbg_get_buf_type(uint32 type)
{				/*BufferType_t */
	switch (type) {
		CASE_MACRO_2_MACRO_STR(BUFFER_TYPE_INVALID);
		CASE_MACRO_2_MACRO_STR(BUFFER_TYPE_PREVIEW);
		CASE_MACRO_2_MACRO_STR(BUFFER_TYPE_VIDEO);
		CASE_MACRO_2_MACRO_STR(BUFFER_TYPE_ZSL);
		CASE_MACRO_2_MACRO_STR(BUFFER_TYPE_RAW_ZSL);
		CASE_MACRO_2_MACRO_STR(BUFFER_TYPE_RAW_TEMP);
		CASE_MACRO_2_MACRO_STR(BUFFER_TYPE_PREVIEW_TEMP);
		CASE_MACRO_2_MACRO_STR(BUFFER_TYPE_VIDEO_TEMP);
		CASE_MACRO_2_MACRO_STR(BUFFER_TYPE_ZSL_TEMP);
		CASE_MACRO_2_MACRO_STR(BUFFER_TYPE_HDR_STATS_DATA);
		CASE_MACRO_2_MACRO_STR(BUFFER_TYPE_EMB_DATA);
		CASE_MACRO_2_MACRO_STR(BUFFER_TYPE_META_INFO);
		CASE_MACRO_2_MACRO_STR(BUFFER_TYPE_FRAME_INFO);
		CASE_MACRO_2_MACRO_STR(BUFFER_TYPE_TNR_REF);
		CASE_MACRO_2_MACRO_STR(BUFFER_TYPE_STAGE2_RECYCLE);
		CASE_MACRO_2_MACRO_STR(BUFFER_TYPE_MAX);
			break;
	default:
		return "Unknown type";
	}

}

char *isp_dbg_get_cmd_str(uint32 cmd)
{
	switch (cmd) {
		CASE_MACRO_2_MACRO_STR(CMD_ID_BIND_STREAM);
		CASE_MACRO_2_MACRO_STR(CMD_ID_UNBIND_STREAM);
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
		CASE_MACRO_2_MACRO_STR(CMD_ID_SET_SENSOR_M2M_CALIBDB);
		CASE_MACRO_2_MACRO_STR(CMD_ID_ENABLE_DEVICE_SCRIPT_CONTROL);
		CASE_MACRO_2_MACRO_STR(CMD_ID_DISABLE_DEVICE_SCRIPT_CONTROL);
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
		CASE_MACRO_2_MACRO_STR(CMD_ID_SET_STREAM_MODE);
		CASE_MACRO_2_MACRO_STR(CMD_ID_SET_INPUT_SENSOR);
		CASE_MACRO_2_MACRO_STR(CMD_ID_SET_ACQ_WINDOW);
		CASE_MACRO_2_MACRO_STR(CMD_ID_ENABLE_CALIB);
		CASE_MACRO_2_MACRO_STR(CMD_ID_DISABLE_CALIB);
		CASE_MACRO_2_MACRO_STR(CMD_ID_SET_ASPECT_RATIO_WINDOW);
		CASE_MACRO_2_MACRO_STR(CMD_ID_SET_ZOOM_WINDOW);
		CASE_MACRO_2_MACRO_STR(CMD_ID_SET_IR_ILLU_CONFIG);
		CASE_MACRO_2_MACRO_STR(CMD_ID_SET_PREVIEW_OUT_PROP);
		CASE_MACRO_2_MACRO_STR(CMD_ID_SET_PREVIEW_FRAME_RATE_RATIO);
		CASE_MACRO_2_MACRO_STR(CMD_ID_ENABLE_PREVIEW);
		CASE_MACRO_2_MACRO_STR(CMD_ID_DISABLE_PREVIEW);
		CASE_MACRO_2_MACRO_STR(CMD_ID_SET_VIDEO_OUT_PROP);
		CASE_MACRO_2_MACRO_STR(CMD_ID_SET_VIDEO_FRAME_RATE_RATIO);
		CASE_MACRO_2_MACRO_STR(CMD_ID_ENABLE_VIDEO);
		CASE_MACRO_2_MACRO_STR(CMD_ID_DISABLE_VIDEO);
		CASE_MACRO_2_MACRO_STR(CMD_ID_SET_ZSL_OUT_PROP);
		CASE_MACRO_2_MACRO_STR(CMD_ID_SET_ZSL_FRAME_RATE_RATIO);
		CASE_MACRO_2_MACRO_STR(CMD_ID_ENABLE_ZSL);
		CASE_MACRO_2_MACRO_STR(CMD_ID_DISABLE_ZSL);
		CASE_MACRO_2_MACRO_STR(CMD_ID_CAPTURE_YUV);
		CASE_MACRO_2_MACRO_STR(CMD_ID_SET_RAW_ZSL_FRAME_RATE_RATIO);
		CASE_MACRO_2_MACRO_STR(CMD_ID_ENABLE_RAW_ZSL);
		CASE_MACRO_2_MACRO_STR(CMD_ID_DISABLE_RAW_ZSL);
		CASE_MACRO_2_MACRO_STR(CMD_ID_CAPTURE_RAW);
		CASE_MACRO_2_MACRO_STR(CMD_ID_START_STREAM);
		CASE_MACRO_2_MACRO_STR(CMD_ID_STOP_STREAM);
		CASE_MACRO_2_MACRO_STR(CMD_ID_RESET_STREAM);
		CASE_MACRO_2_MACRO_STR(CMD_ID_SET_FRAME_RATE_INFO);
		CASE_MACRO_2_MACRO_STR(CMD_ID_SET_FRAME_CONTROL_RATIO);
		CASE_MACRO_2_MACRO_STR(CMD_ID_AE_SET_MODE);
		CASE_MACRO_2_MACRO_STR(CMD_ID_AE_SET_FLICKER);
		CASE_MACRO_2_MACRO_STR(CMD_ID_AE_SET_REGION);
		CASE_MACRO_2_MACRO_STR(CMD_ID_AE_LOCK);
		CASE_MACRO_2_MACRO_STR(CMD_ID_AE_UNLOCK);
		CASE_MACRO_2_MACRO_STR(CMD_ID_AE_SET_ANALOG_GAIN_RANGE);
		CASE_MACRO_2_MACRO_STR(CMD_ID_AE_SET_DIGITAL_GAIN_RANGE);
		CASE_MACRO_2_MACRO_STR(CMD_ID_AE_SET_ITIME_RANGE);
		CASE_MACRO_2_MACRO_STR(CMD_ID_AE_SET_ANALOG_GAIN);
		CASE_MACRO_2_MACRO_STR(CMD_ID_AE_SET_DIGITAL_GAIN);
		CASE_MACRO_2_MACRO_STR(CMD_ID_AE_SET_ITIME);
		CASE_MACRO_2_MACRO_STR(CMD_ID_AE_SET_ISO);
		CASE_MACRO_2_MACRO_STR(CMD_ID_AE_GET_ISO_CAPABILITY);
		CASE_MACRO_2_MACRO_STR(CMD_ID_AE_SET_EV);
		CASE_MACRO_2_MACRO_STR(CMD_ID_AE_GET_EV_CAPABILITY);
		CASE_MACRO_2_MACRO_STR(CMD_ID_AE_PRECAPTURE);
		CASE_MACRO_2_MACRO_STR(CMD_ID_AE_ENABLE_HDR);
		CASE_MACRO_2_MACRO_STR(CMD_ID_AE_DISABLE_HDR);
		CASE_MACRO_2_MACRO_STR(CMD_ID_AE_SET_FLASH_MODE);
		CASE_MACRO_2_MACRO_STR(CMD_ID_AE_SET_FLASH_POWER_LEVEL);
		CASE_MACRO_2_MACRO_STR(CMD_ID_AE_SET_RED_EYE_MODE);
		CASE_MACRO_2_MACRO_STR(CMD_ID_AE_GET_STATUS);
		CASE_MACRO_2_MACRO_STR(CMD_ID_AE_SET_START_EXPOSURE);
		CASE_MACRO_2_MACRO_STR(CMD_ID_AWB_SET_MODE);
		CASE_MACRO_2_MACRO_STR(CMD_ID_AWB_SET_REGION);
		CASE_MACRO_2_MACRO_STR(CMD_ID_AWB_SET_LIGHT);
		CASE_MACRO_2_MACRO_STR(CMD_ID_AWB_SET_COLOR_TEMPERATURE);
		CASE_MACRO_2_MACRO_STR(CMD_ID_AWB_SET_WB_GAIN);
		CASE_MACRO_2_MACRO_STR(CMD_ID_AWB_GET_WB_GAIN);
		CASE_MACRO_2_MACRO_STR(CMD_ID_AWB_SET_CC_MATRIX);
		CASE_MACRO_2_MACRO_STR(CMD_ID_AWB_GET_CC_MATRIX);
		CASE_MACRO_2_MACRO_STR(CMD_ID_AWB_SET_CC_OFFSET);
		CASE_MACRO_2_MACRO_STR(CMD_ID_AWB_GET_CC_OFFSET);
		CASE_MACRO_2_MACRO_STR(CMD_ID_AWB_SET_LSC_MATRIX);
		CASE_MACRO_2_MACRO_STR(CMD_ID_AWB_GET_LSC_MATRIX);
		CASE_MACRO_2_MACRO_STR(CMD_ID_AWB_SET_LSC_SECTOR);
		CASE_MACRO_2_MACRO_STR(CMD_ID_AWB_GET_LSC_SECTOR);
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
		CASE_MACRO_2_MACRO_STR(CMD_ID_CPROC_SET_PROP);
		CASE_MACRO_2_MACRO_STR(CMD_ID_CPROC_SET_CONTRAST);
		CASE_MACRO_2_MACRO_STR(CMD_ID_CPROC_SET_BRIGHTNESS);
		CASE_MACRO_2_MACRO_STR(CMD_ID_CPROC_SET_SATURATION);
		CASE_MACRO_2_MACRO_STR(CMD_ID_CPROC_SET_HUE);
		CASE_MACRO_2_MACRO_STR(CMD_ID_CPROC_GET_STATUS);
		CASE_MACRO_2_MACRO_STR(CMD_ID_GAMMA_ENABLE);
		CASE_MACRO_2_MACRO_STR(CMD_ID_GAMMA_DISABLE);
		CASE_MACRO_2_MACRO_STR(CMD_ID_GAMMA_SET_CURVE);
		CASE_MACRO_2_MACRO_STR(CMD_ID_SNR_ENABLE);
		CASE_MACRO_2_MACRO_STR(CMD_ID_SNR_DISABLE);
		CASE_MACRO_2_MACRO_STR(CMD_ID_TNR_ENABLE);
		CASE_MACRO_2_MACRO_STR(CMD_ID_TNR_DISABLE);
		CASE_MACRO_2_MACRO_STR(CMD_ID_BLS_ENABLE);
		CASE_MACRO_2_MACRO_STR(CMD_ID_BLS_DISABLE);
		CASE_MACRO_2_MACRO_STR(CMD_ID_BLS_SET_BLACK_LEVEL);
		CASE_MACRO_2_MACRO_STR(CMD_ID_BLS_GET_STATUS);
		CASE_MACRO_2_MACRO_STR(CMD_ID_DEGAMMA_ENABLE);
		CASE_MACRO_2_MACRO_STR(CMD_ID_DEGAMMA_DISABLE);
		CASE_MACRO_2_MACRO_STR(CMD_ID_DEGAMMA_SET_CURVE);
		CASE_MACRO_2_MACRO_STR(CMD_ID_DEGAMMA_GET_STATUS);
		CASE_MACRO_2_MACRO_STR(CMD_ID_DPF_ENABLE);
		CASE_MACRO_2_MACRO_STR(CMD_ID_SET_RAW_PKT_FMT);
		CASE_MACRO_2_MACRO_STR(CMD_ID_DPF_DISABLE);
		CASE_MACRO_2_MACRO_STR(CMD_ID_DPF_CONFIG);
		CASE_MACRO_2_MACRO_STR(CMD_ID_DPF_GET_STATUS);
		CASE_MACRO_2_MACRO_STR(CMD_ID_DPCC_ENABLE);
		CASE_MACRO_2_MACRO_STR(CMD_ID_DPCC_DISABLE);
		CASE_MACRO_2_MACRO_STR(CMD_ID_DPCC_CONFIG);
		CASE_MACRO_2_MACRO_STR(CMD_ID_DPCC_GET_STATUS);
		CASE_MACRO_2_MACRO_STR(CMD_ID_CAC_ENABLE);
		CASE_MACRO_2_MACRO_STR(CMD_ID_CAC_DISABLE);
		CASE_MACRO_2_MACRO_STR(CMD_ID_CAC_CONFIG);
		CASE_MACRO_2_MACRO_STR(CMD_ID_CAC_GET_STATUS);
		CASE_MACRO_2_MACRO_STR(CMD_ID_WDR_ENABLE);
		CASE_MACRO_2_MACRO_STR(CMD_ID_WDR_DISABLE);
		CASE_MACRO_2_MACRO_STR(CMD_ID_WDR_SET_CURVE);
		CASE_MACRO_2_MACRO_STR(CMD_ID_WDR_GET_STATUS);
		CASE_MACRO_2_MACRO_STR(CMD_ID_CNR_ENABLE);
		CASE_MACRO_2_MACRO_STR(CMD_ID_CNR_DISABLE);
		CASE_MACRO_2_MACRO_STR(CMD_ID_CNR_SET_PARAM);
		CASE_MACRO_2_MACRO_STR(CMD_ID_CNR_GET_STATUS);
		CASE_MACRO_2_MACRO_STR(CMD_ID_DEMOSAIC_SET_THRESH);
		CASE_MACRO_2_MACRO_STR(CMD_ID_DEMOSAIC_GET_THRESH);
		CASE_MACRO_2_MACRO_STR(CMD_ID_FILTER_ENABLE);
		CASE_MACRO_2_MACRO_STR(CMD_ID_FILTER_DISABLE);
		CASE_MACRO_2_MACRO_STR(CMD_ID_FILTER_SET_CURVE);
		CASE_MACRO_2_MACRO_STR(CMD_ID_FILTER_GET_STATUS);
		CASE_MACRO_2_MACRO_STR(CMD_ID_SEND_FRAME_CONTROL);
		CASE_MACRO_2_MACRO_STR(CMD_ID_SEND_MODE3_FRAME_INFO);
		CASE_MACRO_2_MACRO_STR(CMD_ID_SEND_MODE4_FRAME_INFO);
		CASE_MACRO_2_MACRO_STR(CMD_ID_SEND_BUFFER);
		CASE_MACRO_2_MACRO_STR(CMD_ID_AE_SET_SETPOINT);
		CASE_MACRO_2_MACRO_STR(CMD_ID_CONFIG_GMC_PREFETCH);
		CASE_MACRO_2_MACRO_STR(CMD_ID_STNR_GET_STATUS);
		CASE_MACRO_2_MACRO_STR(CMD_ID_GAMMA_GET_STATUS);
		CASE_MACRO_2_MACRO_STR(CMD_ID_SET_RESIZE_TABLE);
			break;
	default:
		return "Unknown cmd";
	};
}

char *isp_dbg_get_resp_str(uint32 cmd)
{
	switch (cmd) {
		CASE_MACRO_2_MACRO_STR(RESP_ID_CMD_DONE);
		CASE_MACRO_2_MACRO_STR(RESP_ID_FRAME_DONE);
		CASE_MACRO_2_MACRO_STR(RESP_ID_FRAME_INFO);
		CASE_MACRO_2_MACRO_STR(RESP_ID_ERROR);
		CASE_MACRO_2_MACRO_STR(RESP_ID_HEART_BEAT);

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
		//CASE_MACRO_2_MACRO_STR
		//		(RESP_ID_SENSOR_SET_HDR_LOW_DGAIN_RATIO);
		//CASE_MACRO_2_MACRO_STR(RESP_ID_SENSOR_SET_HDR_EQUAL_DGAIN);

		CASE_MACRO_2_MACRO_STR(RESP_ID_LENS_SET_POS);

		CASE_MACRO_2_MACRO_STR(RESP_ID_FLASH_SET_POWER);
		CASE_MACRO_2_MACRO_STR(RESP_ID_FLASH_SET_ON);
		CASE_MACRO_2_MACRO_STR(RESP_ID_FLASH_SET_OFF);
			break;
	default:
		return "Unknown respid";
	};
}

char *isp_dbg_get_pvt_fmt_str(int32 fmt /*enum pvt_img_fmt */)
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

char *isp_dbg_get_af_mode_str(uint32 mode /*AfMode_t */)
{
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
}

char *isp_dbg_get_ae_mode_str(uint32 mode /*AeMode_t */)
{
	switch (mode) {
		CASE_MACRO_2_MACRO_STR(AE_MODE_INVALID);
		CASE_MACRO_2_MACRO_STR(AE_MODE_MANUAL);
		CASE_MACRO_2_MACRO_STR(AE_MODE_AUTO);
		CASE_MACRO_2_MACRO_STR(AE_MODE_MAX);
			break;
	default:
		return "Unknown ae mode";
	};
};

char *isp_dbg_get_awb_mode_str(uint32 mode /*AwbMode_t */)
{
	switch (mode) {
		CASE_MACRO_2_MACRO_STR(AWB_MODE_INVALID);
		CASE_MACRO_2_MACRO_STR(AWB_MODE_MANUAL);
		CASE_MACRO_2_MACRO_STR(AWB_MODE_AUTO);
		CASE_MACRO_2_MACRO_STR(AWB_MODE_MAX);
			break;
	default:
		return "Unknown awb mode";
	};
}

char *isp_dbg_get_scene_mode_str(uint32 mode /*enum isp_scene_mode */)
{
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
};

char *isp_dbg_get_stream_str(uint32 stream /*enum fw_cmd_resp_stream_id */)
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

char *isp_dbg_get_para_str(uint32 para /*enum para_id */)
{
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
		/*uint32 done */
		CASE_MACRO_2_MACRO_STR(PARA_ID_COLOR_TEMPERATURE);
		/*pdata_anti_banding_t done */
		CASE_MACRO_2_MACRO_STR(PARA_ID_ANTI_BANDING);
		/*pvt_int32done */
		CASE_MACRO_2_MACRO_STR(PARA_ID_COLOR_ENABLE);
		CASE_MACRO_2_MACRO_STR(PARA_ID_GAMMA);	/*int32*done */
		CASE_MACRO_2_MACRO_STR(PARA_ID_ISO);	/*AeIso_t **/
		CASE_MACRO_2_MACRO_STR(PARA_ID_EV);	/*AeEv_t **/
		/*uint32**/
		CASE_MACRO_2_MACRO_STR(PARA_ID_MAIN_JPEG_COMPRESS_RATE);
		/*uint32**/
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
}

#ifdef TBD_LATER //#if 0
char *isp_dbg_get_iso_str(uint32 para /*AeIso_t */)
{
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
};
#endif
char *isp_dbg_get_flash_mode_str(uint32 mode /*FlashMode_t */)
{
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
}

char *isp_dbg_get_flash_assist_mode_str(uint32 mode /*FocusAssistMode_t */)
{
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
};

char *isp_dbg_get_red_eye_mode_str(uint32 mode /*RedEyeMode_t */)
{
	switch (mode) {
		CASE_MACRO_2_MACRO_STR(RED_EYE_MODE_INVALID);
		CASE_MACRO_2_MACRO_STR(RED_EYE_MODE_OFF);
		CASE_MACRO_2_MACRO_STR(RED_EYE_MODE_ON);
		CASE_MACRO_2_MACRO_STR(RED_EYE_MODE_MAX);
			break;
	default:
		return "bad red eye mode";
	};
};

char *isp_dbg_get_anti_flick_str(uint32 type /*AeFlickerType_t */)
{
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
};

void isp_dbg_show_af_ctrl_info(void *p_in /*AfCtrlInfo_t **/)
{
	AfCtrlInfo_t *p = p_in;

	isp_dbg_print_info("AfCtrlInfo_t info:\n");
	isp_dbg_print_info("lockState %u,bSupportFocus %u, lenPos %u\n",
			p->lockState, p->bSupportFocus, p->lensPos);
	isp_dbg_print_info("focusAssistOn %u,focusAssistPowerLevel %u\n",
			p->focusAssistOn, p->focusAssistPowerLevel);
	isp_dbg_print_info("mode %u,focusAssistMode %u,searchState%u\n",
			p->mode, p->focusAssistMode, p->searchState);
}

void isp_dbg_show_ae_ctrl_info(void *p_in /*AeCtrlInfo_t **/)
{
	AeCtrlInfo_t *p = p_in;

	isp_dbg_print_info("AeCtrlInfo_t info:\n");
	isp_dbg_print_info("lockState %u,itime %u,aGain %u,dGain %u\n",
			p->lockState, p->itime, p->aGain, p->dGain);
	isp_dbg_print_info("hdrEnabled %u,hdrParamValid %u\n",
			p->hdrEnabled, p->hdrParamValid);
	isp_dbg_print_info("flashOn %u,flashPowerLevel %u,preCapMeasure %u\n",
			p->flashOn, p->flashPowerLevel, p->preCapMeasure);
	isp_dbg_print_info("Aemode %u,flickerType %u,searchState %u\n",
			p->mode, p->flickerType, p->searchState);
	isp_dbg_print_info("iso %u,ev %u/%u\n",
			p->iso.iso, p->ev.numerator, p->ev.denominator);
	isp_dbg_print_info("flashMode %u,redEyeMode %u\n",
			p->flashMode, p->redEyeMode);
};

void isp_dbg_show_frame3_info(void *p_in	/*Mode3FrameInfo_t **/
			, char *prefix)
{
	Mode3FrameInfo_t *p = p_in;

	isp_dbg_print_info("rgbir:Mode3Frame info(%p):\n", p_in);
	isp_dbg_print_info("poc %u, timeStampLo %u, timeStampHi %u\n",
			p->poc, p->timeStampLo, p->timeStampHi);
	isp_dbg_print_info("enableFrameControl %u, ctlCmd 0x%x\n",
			p->enableFrameControl, p->frameControlCmd.cmd.cmdId);
	isp_dbg_print_info("%s enableYuvCap %u, yuvCapCmd 0x%x, %llx(%u)\n",
			prefix, p->enableYuvCap, p->yuvCapCmd.cmd.cmdId,
			(((uint64) p->yuvCapCmd.buffer.bufBaseAHi) << 32) |
			((uint64) p->yuvCapCmd.buffer.bufBaseALo),
			p->yuvCapCmd.buffer.bufSizeA);
	isp_dbg_print_info("enableRawCap %u, rawCapCmd 0x%x\n",
			p->enableRawCap, p->rawCapCmd.cmd.cmdId);
	isp_dbg_print_info("flashPowerLevel %u, flashOn 0x%x, frameRate %u\n",
			p->flashPowerLevel, p->flashOn, p->frameRate);
	isp_dbg_print_info
	("hasZoomCmd %u, zoomCmd 0x%x, zoomWnd(%u,%u,%u,%u)\n",
	 p->hasZoomCmd, p->zoomCmd.cmdId, p->zoomWindow.h_offset,
	 p->zoomWindow.v_offset, p->zoomWindow.h_size,
	 p->zoomWindow.v_size);

	isp_dbg_print_info("RAW_PKT_FMT_%u,rawWidthInByte %u,cfaPattern %u\n",
			p->rawPktFmt, p->rawWidthInByte, p->cfaPattern);

	isp_dbg_show_af_ctrl_info(&p->afCtrlInfo);
	isp_dbg_show_ae_ctrl_info(&p->aeCtrlInfo);
}

char *isp_dbg_get_focus_state_str(uint32 state /*AfSearchState_t */)
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

char *isp_dbg_get_work_buf_str(uint32 type /*isp_work_buf_type */)
{
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
}

;

char *isp_dbg_get_reg_name(uint32 reg)
{
	switch (reg) {
		CASE_MACRO_2_MACRO_STR(mmRSMU_SW_MMIO_PUB_IND_ADDR_0_ISP);
		CASE_MACRO_2_MACRO_STR(mmRSMU_SW_MMIO_PUB_IND_DATA_0_ISP);
		CASE_MACRO_2_MACRO_STR(mmRSMU_PGFSM_CONTROL_ISP);
		CASE_MACRO_2_MACRO_STR(mmISP_POWER_STATUS);
		CASE_MACRO_2_MACRO_STR(mmISP_CORE_ISP_CTRL);
		CASE_MACRO_2_MACRO_STR(mmISP_CCPU_CNTL);
		CASE_MACRO_2_MACRO_STR(mmISP_SOFT_RESET);
		CASE_MACRO_2_MACRO_STR(mmISP_CORE_VI_IRCL);
		CASE_MACRO_2_MACRO_STR(mmISP_MIPI_VI_IRCL);
		CASE_MACRO_2_MACRO_STR(mmISP_CORE_MI_MP_STALL);
		CASE_MACRO_2_MACRO_STR(mmISP_CORE_MI_VP_STALL);
		CASE_MACRO_2_MACRO_STR(mmISP_CORE_MI_PP_STALL);
		CASE_MACRO_2_MACRO_STR(mmISP_CORE_MI_RD_DMA_STALL);
		CASE_MACRO_2_MACRO_STR(mmISP_MIPI_MI_MIPI0_STALL);
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
		CASE_MACRO_2_MACRO_STR(mmISP_RB_BASE_LO5);
		CASE_MACRO_2_MACRO_STR(mmISP_RB_BASE_HI5);
		CASE_MACRO_2_MACRO_STR(mmISP_RB_SIZE5);
		CASE_MACRO_2_MACRO_STR(mmISP_RB_RPTR5);
		CASE_MACRO_2_MACRO_STR(mmISP_RB_WPTR5);
		CASE_MACRO_2_MACRO_STR(mmISP_RB_BASE_LO2);
		CASE_MACRO_2_MACRO_STR(mmISP_RB_BASE_HI2);
		CASE_MACRO_2_MACRO_STR(mmISP_RB_SIZE2);
		CASE_MACRO_2_MACRO_STR(mmISP_RB_RPTR2);
		CASE_MACRO_2_MACRO_STR(mmISP_RB_WPTR2);
		CASE_MACRO_2_MACRO_STR(mmISP_RB_BASE_LO6);
		CASE_MACRO_2_MACRO_STR(mmISP_RB_BASE_HI6);
		CASE_MACRO_2_MACRO_STR(mmISP_RB_SIZE6);
		CASE_MACRO_2_MACRO_STR(mmISP_RB_RPTR6);
		CASE_MACRO_2_MACRO_STR(mmISP_RB_WPTR6);
		CASE_MACRO_2_MACRO_STR(mmISP_RB_BASE_LO3);
		CASE_MACRO_2_MACRO_STR(mmISP_RB_BASE_HI3);
		CASE_MACRO_2_MACRO_STR(mmISP_RB_SIZE3);
		CASE_MACRO_2_MACRO_STR(mmISP_RB_RPTR3);
		CASE_MACRO_2_MACRO_STR(mmISP_RB_WPTR3);
		CASE_MACRO_2_MACRO_STR(mmISP_RB_BASE_LO7);
		CASE_MACRO_2_MACRO_STR(mmISP_RB_BASE_HI7);
		CASE_MACRO_2_MACRO_STR(mmISP_RB_SIZE7);
		CASE_MACRO_2_MACRO_STR(mmISP_RB_RPTR7);
		CASE_MACRO_2_MACRO_STR(mmISP_RB_WPTR7);
		CASE_MACRO_2_MACRO_STR(mmISP_RB_BASE_LO4);
		CASE_MACRO_2_MACRO_STR(mmISP_RB_BASE_HI4);
		CASE_MACRO_2_MACRO_STR(mmISP_RB_SIZE4);
		CASE_MACRO_2_MACRO_STR(mmISP_RB_RPTR4);
		CASE_MACRO_2_MACRO_STR(mmISP_RB_WPTR4);
		CASE_MACRO_2_MACRO_STR(mmISP_RB_BASE_LO8);
		CASE_MACRO_2_MACRO_STR(mmISP_RB_BASE_HI8);
		CASE_MACRO_2_MACRO_STR(mmISP_RB_SIZE8);
		CASE_MACRO_2_MACRO_STR(mmISP_RB_RPTR8);
		CASE_MACRO_2_MACRO_STR(mmISP_RB_WPTR8);
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
		return "mmHDP_MEM_COHERENCY_FLUSH_CNTL";
	default:
		return "unknown reg";

	}

};

void isp_dbg_assert_func(int32 reason)
{
	reason = reason;
};

void isp_dbg_dump_ae_status(void *p /*AeStatus_t **/)
{
	AeStatus_t *v = (AeStatus_t *)p;

	isp_dbg_print_info(
	"AeStatus_t mode %u, flickerType %u, lochstate %u, searchState %u\n",
		v->mode, v->flickerType, v->lockState, v->searchState);
	isp_dbg_print_info(
		"isoCap [%u,%u], iso %u\n",
		v->isoCap.minIso.iso, v->isoCap.maxIso.iso, v->iso.iso);
	isp_dbg_print_info(
		"evCap [%u/%u,%u/%u,%u/%u], ev %u/%u\n",
		v->evCap.minEv.numerator, v->evCap.minEv.denominator,
		v->evCap.maxEv.numerator, v->evCap.maxEv.denominator,
		v->evCap.stepEv.numerator, v->evCap.stepEv.denominator,
		v->ev.numerator, v->ev.denominator);
	isp_dbg_print_info(
		"region[%u,%u,%u,%u]\n",
		v->region.h_offset, v->region.v_offset,
		v->region.h_size, v->region.v_size);

	isp_dbg_print_info(
		"itime %u, aGain %u, dGain %u, minItime %u, maxItime %u\n",
		v->itime, v->aGain, v->dGain, v->minItime, v->maxItime);
	isp_dbg_print_info(
		"minAGain %u, maxAGain %u, minDGain %u, maxDgain %u\n",
		v->minAGain, v->maxAGain, v->minDGain, v->maxDgain);
	isp_dbg_print_info(
	"hdrEnabled %u, hdrItimeType %u, hdrAGainType %u, hdrDGainType %u\n",
		v->hdrEnabled, v->hdrItimeType, v->hdrAGainType,
		v->hdrDGainType);
}

