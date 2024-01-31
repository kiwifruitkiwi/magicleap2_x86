/*
 * Copyright (C) 2019-2020 Advanced Micro Devices, Inc. All rights reserved.
 */

#ifndef ISP_FW_INTERFACE_H
#define ISP_FW_INTERFACE_H

#include "isp_common.h"
#include "isp_fw_if/drv_isp_if.h"
#define aidt_sleep thread_sleep

#define g_rb_context isp->ring_log_buf_info
#define AIDT_OS_OK RET_SUCCESS
#define aidt_isp_event_signal isp_event_signal


enum isp_fw_work_buf_type {
	ISP_FW_WORK_BUF_TYPE_FW,
	ISP_FW_WORK_BUF_TYPE_PACKAGE,
	ISP_FW_WORK_BUF_TYPE_H2F_RING,
	ISP_FW_WORK_BUF_TYPE_F2H_RING
};

struct isp_config_sensor_curr_expo_param {
	enum _SensorId_t sensor_id;	/*front camera or rear camera */
	unsigned int curr_gain;	/*current gain */
	unsigned int curr_integration_time;	/*current integration time */
};

struct isp_create_param {
	unsigned int unused;
};

unsigned int isp_fw_get_work_buf_size(
		enum isp_fw_work_buf_type type);
/*unsigned int isp_fw_get_work_buf_num(enum isp_fw_work_buf_type type);*/

/*refer to  _aidt_isp_create*/
int fw_create(struct isp_context *isp);

/*refer to _aidt_isp_start_preview*/
int fw_send_start_prev_cmd(struct isp_context *isp_context);
/*refer to fw_send_stop_prev_cmd*/
int fw_send_stop_prev_cmd(struct isp_context *isp_context,
			enum camera_id camera_id);
int isp_fw_stop_stream(struct isp_context *isp_context,
			enum camera_id camera_id);
/*refer to _aidt_api_config_af*/
int fw_cfg_af(struct isp_context *isp_context, enum camera_id camera_id);
/*refer to _aidt_no_slot_for_host2fw_command*/
int no_fw_cmd_ringbuf_slot(struct isp_context *isp,
			enum fw_cmd_resp_stream_id cmd_buf_idx);

/*refer to _get_next_command_seq_num*/
unsigned int get_nxt_cmd_seq_num(struct isp_context *pcontext);
/*refer to _compute_checksum*/
unsigned char compute_check_sum(unsigned char *buffer,
		unsigned int buffer_size);

int fw_if_send_prev_buf(struct isp_context *pcontext,
			enum camera_id cam_id,
			struct isp_img_buf_info *buffer);
int fw_if_send_img_buf(struct isp_context *isp,
			struct isp_mapped_buf_info *buffer,
			enum camera_id cam_id, enum stream_id str_id);

/*refer to _insert_host_command*/
int isp_send_fw_cmd(struct isp_context *isp, unsigned int cmd_id,
			 enum fw_cmd_resp_stream_id stream,
			 enum fw_cmd_para_type directcmd, void *package,
			 unsigned int package_size);
int isp_send_fw_cmd_sync(struct isp_context *isp,
			unsigned int cmd_id,
			enum fw_cmd_resp_stream_id stream,
			enum fw_cmd_para_type directcmd, void *package,
			unsigned int package_size,
			unsigned int timeout /*in ms */,
			void *resp_pl, unsigned int *resp_pl_len);

int isp_send_fw_cmd_online_tune(struct isp_context *isp, unsigned int cmd_id,
			enum fw_cmd_resp_stream_id stream,
			enum fw_cmd_para_type directcmd, void *package,
			unsigned int package_size, unsigned int timeout,
			void *resp_pl, unsigned int *resp_pl_len);

int isp_send_fw_cmd_cid(struct isp_context *isp,
			enum camera_id cam_id,
			unsigned int cmd_id, enum fw_cmd_resp_stream_id stream,
			enum fw_cmd_para_type directcmd, void *package,
			unsigned int package_size);

int isp_send_fw_cmd_ex(struct isp_context *isp,
			enum camera_id cam_id,
			unsigned int cmd_id, enum fw_cmd_resp_stream_id stream,
			enum fw_cmd_para_type directcmd, void *package,
			unsigned int package_size, struct isp_event *evt,
			unsigned int *seq, void *resp_pl,
			unsigned int *resp_pl_len);

/*
 * This function acks just like `isp_send_fw_cmd_ex` but accepts a void*
 * argument |dma_buf| to represent an allocated DMA buffer. Caller has
 * responsibility to manage the lifecycle of the given DMA buffer handle.
 * @dma_buf: pointer of a DMA buffer, currently give it a pointer of
 * `struct isp_gpu_mem_info` and which must be allocated and stored data.
 * @return
 * 0: ok.
 */
int isp_send_fw_cmd_by_dma_buf(struct isp_context *isp,
			enum camera_id cam_id,
			unsigned int cmd_id, enum fw_cmd_resp_stream_id stream,
			enum fw_cmd_para_type directcmd, void *dma_buf,
			unsigned int dma_buf_size, struct isp_event *evt,
			unsigned int *seq,
			void *resp_pl, unsigned int *resp_pl_len);


/*_aidt_queue_command_insert_tail*/
struct isp_cmd_element *isp_append_cmd_2_cmdq(
	struct isp_context *isp,
	struct isp_cmd_element *command);
struct isp_cmd_element *isp_rm_cmd_from_cmdq(
	struct isp_context *isp,
	unsigned int seq_num, unsigned int cmd_id,
	int signal_evt);
struct isp_cmd_element *isp_rm_cmd_from_cmdq_by_stream(
	struct isp_context *isp,
	enum fw_cmd_resp_stream_id
	stream,
	int signal_evt);

/*refer to _aidt_insert_host2fw_command*/
int insert_isp_fw_cmd(struct isp_context *isp,
			enum fw_cmd_resp_stream_id cmd_buf_idx,
			struct _Cmd_t *cmd);

int fw_if_set_brightness(struct isp_context *pcontext,
	unsigned int camera_id,
				int brightness);
int fw_if_set_contrast(struct isp_context *pcontext,
	unsigned int camera_id,
				int contrast);
int fw_if_set_hue(struct isp_context *pcontext, unsigned int camera_id,
				int hue);
int fw_if_set_satuation(struct isp_context *pcontext,
	unsigned int camera_id,
				int stauation);
int fw_if_set_color_enable(struct isp_context *pcontext,
				unsigned int camera_id, int color_enable);
int fw_if_set_flicker_mode(struct isp_context *pcontext,
				unsigned int camera_id,
				enum _AeFlickerType_t mode);
int fw_if_set_cproc_enable(struct isp_context *pcontext,
				unsigned int camera_id, int cproc_enable);
int fw_if_set_iso(struct isp_context *pcontext, unsigned int camera_id,
				struct _AeIso_t iso);
int fw_if_set_ev_compensation(struct isp_context *pcontext,
				unsigned int camera_id, struct _AeEv_t ev);
int fw_if_set_scene_mode(struct isp_context *pcontext,
				unsigned int camera_id,
				enum pvt_scene_mode mode);

int fw_if_get_curr_cproc(struct isp_context *pcontext,
				unsigned int camera_id);
int fw_if_cproc_config(struct isp_context *pcontext,
				unsigned int camera_id,
				int contrast, int brightness,
				int saturation, int hue);

int isp_config_video2d(struct isp_context *pcontext,
				enum camera_id cam_id);
int fw_if_send_record_img_buf(struct isp_context *pcontext,
				enum camera_id cam_id,
				struct isp_img_buf_info *buffer);
int isp_get_f2h_resp(struct isp_context *isp,
			enum fw_cmd_resp_stream_id stream,
			struct _Resp_t *response);
int isp_fw_start(struct isp_context *pcontext);
int isp_fw_boot(struct isp_context *pcontext);
int isp_fw_config_stream(struct isp_context *isp_context,
				enum camera_id cam_id, int pipe_id,
				enum stream_id str_id);
int isp_fw_start_streaming(struct isp_context *isp_context,
				enum camera_id cam_id, int pipe_id);
void isp_fw_stop_all_stream(struct isp_context *isp);

void isp_response_process_raw_return(struct isp_context *isp,
				enum camera_id cam_id, unsigned int status);

int isp_fw_set_focus_mode(struct isp_context *isp,
				enum camera_id cam_id, enum _AfMode_t mode);
int isp_fw_start_af(struct isp_context *isp, enum camera_id cam_id);

int isp_fw_3a_lock(struct isp_context *isp, enum camera_id cam_id,
			enum isp_3a_type type, enum _LockType_t lock_mode);
int isp_fw_3a_unlock(struct isp_context *isp,
				enum camera_id cam_id,
				enum isp_3a_type type);

int isp_fw_set_lens_pos(struct isp_context *isp,
				enum camera_id cam_id,
				unsigned int pos);

int isp_fw_set_lens_range(struct isp_context *isp,
			enum camera_id cid,
			struct _LensRange_t lens_range);
int isp_fw_set_iso(struct isp_context *isp, enum camera_id cam_id,
			unsigned int iso);
int isp_fw_set_exposure_mode(struct isp_context *isp,
				enum camera_id cam_id, enum _AeMode_t mode);

int isp_fw_set_wb_mode(struct isp_context *isp,
				enum camera_id cam_id, enum _AwbMode_t mode);
int isp_fw_set_snr_ana_gain(struct isp_context *isp,
				enum camera_id cam_id, unsigned int ana_gain);
int isp_fw_set_snr_dig_gain(struct isp_context *isp,
				enum camera_id cam_id, unsigned int dig_gain);
int isp_fw_set_itime(struct isp_context *isp,
				enum camera_id cam_id, unsigned int itime);
int isp_fw_send_meta_data_buf(struct isp_context *isp,
				enum camera_id cam_id,
				struct isp_meta_data_info *info);
int isp_fw_send_all_meta_data(struct isp_context *isp);
int isp_fw_set_script(struct isp_context *isp, enum camera_id cam_id,
			   struct _CmdSetDeviceScript_t *script_cmd);
unsigned int isp_fw_get_fw_rb_log(struct isp_context *isp,
	unsigned char *temp_buf);
int isp_fw_set_gamma(struct isp_context *isp, unsigned int camera_id,
				int gamma);
int isp_fw_set_jpeg_qtable(struct isp_context *isp,
	enum camera_id cam_id,
				unsigned int comp_rate, int main_jpeg);
int isp_fw_send_metadata_buf(struct isp_context *pcontext,
				enum camera_id cam_id,
				struct isp_mapped_buf_info *buffer);

int isp_set_clk_info_2_fw(struct isp_context *isp);
int isp_fw_log_module_set(struct isp_context *isp,
				enum _ModId_t id,
				bool enable);
int isp_fw_log_level_set(struct isp_context *isp,
				enum _LogLevel_t level);
int isp_fw_get_ver(struct isp_context *isp, unsigned int *ver);
int isp_fw_get_iso_cap(struct isp_context *isp, enum camera_id cid);
int isp_fw_get_ev_cap(struct isp_context *isp, enum camera_id cid);
int isp_fw_get_cproc_status(struct isp_context *isp,
				enum camera_id cam_id,
				struct _CprocStatus_t *cproc);
int isp_fw_set_sensor_prop(struct isp_context *isp,
				 enum camera_id cam_id,
				 struct _CmdSetSensorProp_t snr_prop);
int isp_fw_set_frame_rate_info(struct isp_context *isp,
				 enum camera_id cam_id,
				 unsigned int frame_rate);
int isp_fw_set_hdr_prop(struct isp_context *isp,
			enum camera_id cam_id,
			struct _CmdSetSensorHdrProp_t hdr_prop);
int isp_fw_set_calib_data(struct isp_context *isp,
			enum camera_id cam_id,
			struct _CmdSetSensorCalibdb_t calib);
int isp_fw_set_emb_prop(struct isp_context *isp,
			enum camera_id cam_id,
			struct _CmdSetSensorEmbProp_t snr_embprop);
int isp_fw_set_stream_path(struct isp_context *isp,
			enum camera_id cam_id,
			struct _CmdSetStreamPath_t stream_path_cmd);
int isp_fw_set_zoom_window(struct isp_context *isp,
			enum camera_id cam_id, struct _Window_t ZoomWindow);
int isp_fw_set_snr_enable(struct isp_context *isp,
			enum camera_id cam_id, unsigned int enable);
int isp_fw_set_tnr_enable(struct isp_context *isp,
			enum camera_id cam_id, unsigned int enable);
int isp_fw_set_calib_enable(struct isp_context *isp,
				 enum camera_id cam_id,
				 unsigned char enable_mask);
int isp_fw_set_calib_disable(struct isp_context *isp,
			enum camera_id cam_id, unsigned int disable_mask);
int isp_fw_set_send_buffer(struct isp_context *isp,
			enum camera_id cam_id, enum _BufferType_t bufferType,
			struct _Buffer_t buffer);
int isp_fw_set_raw_fmt(struct isp_context *isp,
			enum camera_id cam_id,
			struct _CmdSetRawPktFmt_t raw_fmt);
int isp_fw_set_aspect_ratio_window(struct isp_context *isp,
			enum camera_id cam_id,
			struct _CmdSetAspectRatioWindow_t aprw);
int isp_fw_set_out_ch_prop(struct isp_context *isp,
			enum camera_id cam_id,
			struct _CmdSetOutChProp_t cmdChProp);
int isp_fw_set_out_ch_fps_ratio(struct isp_context *isp,
			enum camera_id cam_id,
			struct _CmdSetOutChFrameRateRatio_t cmdChRatio);
int isp_fw_set_out_ch_enable(struct isp_context *isp,
			enum camera_id cam_id,
			struct _CmdEnableOutCh_t cmdChEn);
int isp_fw_set_ae_flicker(struct isp_context *isp,
			enum camera_id cam_id,
			enum _AeFlickerType_t flickerType);
int isp_fw_set_hdr_enable(struct isp_context *isp,
			enum camera_id cam_id, unsigned int enable);
int isp_fw_set_wb_colorT(struct isp_context *isp,
			enum camera_id cam_id, unsigned int colorT);
int isp_fw_set_acq_window(struct isp_context *isp,
			enum camera_id cam_id, struct _Window_t acq_window);
int isp_fw_set_start_stream(struct isp_context *isp,
			enum camera_id cam_id);
int isp_fw_send_frame_ctrl(struct isp_context *isp, enum camera_id cam_id,
			   struct _CmdFrameCtrl_t *frame_ctrl);
int isp_fw_set_ae_precapture(struct isp_context *isp,
			enum camera_id cam_id);
int isp_fw_set_capture_yuv(struct isp_context *isp,
			enum camera_id cam_id, struct _ImageProp_t imageProp,
			struct _Buffer_t buffer);
int isp_fw_set_capture_raw(struct isp_context *isp,
			enum camera_id cam_id, struct _Buffer_t buffer);
int isp_fw_set_wb_light(struct isp_context *isp,
			enum camera_id cam_id, unsigned int light);
int isp_fw_set_wb_gain(struct isp_context *isp,
			enum camera_id cam_id, struct _WbGain_t wb_gain);
int isp_fw_set_ae_region(struct isp_context *isp,
			enum camera_id cam_id, struct _Window_t ae_window);
int isp_fw_set_sharpen_enable(struct isp_context *isp,
			enum camera_id cam_id, enum _SharpenId_t *sharpen_id);
int isp_fw_set_sharpen_disable(struct isp_context *isp,
			enum camera_id cam_id, enum _SharpenId_t *sharpen_id);
int isp_fw_set_sharpen_config(struct isp_context *isp,
			enum camera_id cam_id, enum _SharpenId_t sharpen_id,
			struct _TdbSharpRegByChannel_t param);
int isp_fw_set_clk_gating_enable(struct isp_context *isp,
			enum camera_id cam_id, unsigned int enable);
int isp_fw_set_power_gating_enable(struct isp_context *isp,
			enum camera_id cam_id, unsigned int enable);
int isp_fw_set_image_effect_config(struct isp_context *isp,
		enum camera_id cam_id, struct _IeConfig_t *param,
		int enable);
int isp_fw_set_image_effect_enable(struct isp_context *isp,
			enum camera_id cam_id, unsigned int enable);
int isp_fw_set_wb_cc_matrix(struct isp_context *isp,
			enum camera_id cam_id, struct _CcMatrix_t *matrix);
int isp_fw_set_gamma_enable(struct isp_context *isp,
			enum camera_id cam_id, unsigned int enable);
int isp_fw_set_snr_config(struct isp_context *isp,
			enum camera_id cam_id, struct _SinterParams_t *param);
int isp_fw_set_tnr_config(struct isp_context *isp,
			enum camera_id cam_id, struct _TemperParams_t *param);
int isp_fw_config_cvip_sensor(struct isp_context *isp, enum camera_id cam_id,
			      struct _CmdConfigCvipSensor_t param);
int isp_fw_set_cvip_buf(struct isp_context *isp, enum camera_id cam_id,
			struct _CmdSetCvipBufLayout_t param);
int isp_fw_start_cvip_sensor(struct isp_context *isp, enum camera_id cam_id,
			     struct _CmdStartCvipSensor_t param);
int isp_fw_set_cvip_sensor_slice_num(struct isp_context *isp,
				     enum camera_id cam_id,
				     struct _CmdSetSensorSliceNum_t param);
#endif
