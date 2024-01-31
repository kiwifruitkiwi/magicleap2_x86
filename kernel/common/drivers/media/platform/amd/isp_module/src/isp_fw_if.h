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
 *FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 *THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 *OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 *ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 *OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

#ifndef ISP_FW_INTERFACE_H
#define ISP_FW_INTERFACE_H

#include "isp_common.h"
#include "isp_fw_if/drv_isp_if.h"

#define g_rb_context isp->ring_log_buf_info
#define AIDT_OS_OK RET_SUCCESS
#define aidt_isp_event_signal isp_event_signal

typedef isp_ret_status_t aidt_os_status_t;

enum isp_fw_work_buf_type {
	ISP_FW_WORK_BUF_TYPE_FW,
	ISP_FW_WORK_BUF_TYPE_PACKAGE,
	ISP_FW_WORK_BUF_TYPE_H2F_RING,
	ISP_FW_WORK_BUF_TYPE_F2H_RING
};

struct isp_config_sensor_curr_expo_param {
	SensorId_t sensor_id;	/*front camera or rear camera */
	uint32 curr_gain;	/*current gain */
	uint32 curr_integration_time;	/*current integration time */
};

struct isp_create_param {
	uint32 unused;
};

uint32 isp_fw_get_work_buf_size(enum isp_fw_work_buf_type type);
/*uint32 isp_fw_get_work_buf_num(enum isp_fw_work_buf_type type);*/

/*refer to  _aidt_isp_create*/
result_t fw_create(struct isp_context *isp);

/*refer to _aidt_isp_start_preview*/
result_t fw_send_start_prev_cmd(struct isp_context *isp_context);
/*refer to fw_send_stop_prev_cmd*/
result_t fw_send_stop_prev_cmd(struct isp_context *isp_context,
			enum camera_id camera_id);
result_t isp_fw_stop_stream(struct isp_context *isp_context,
			enum camera_id camera_id);
/*refer to _aidt_api_config_af*/
result_t fw_cfg_af(struct isp_context *isp_context, enum camera_id camera_id);
/*refer to _aidt_no_slot_for_host2fw_command*/
int32 no_fw_cmd_ringbuf_slot(struct isp_context *isp,
			enum fw_cmd_resp_stream_id cmd_buf_idx);

/*refer to _get_next_command_seq_num*/
uint32 get_nxt_cmd_seq_num(struct isp_context *pcontext);
/*refer to _compute_checksum*/
uint8 compute_check_sum(uint8 *buffer, uint32 buffer_size);

result_t fw_if_send_prev_buf(struct isp_context *pcontext,
			enum camera_id cam_id,
			struct isp_img_buf_info *buffer);
result_t fw_if_send_img_buf(struct isp_context *isp,
			struct isp_mapped_buf_info *buffer,
			enum camera_id cam_id, enum stream_id str_id);

/*refer to _insert_host_command*/
result_t isp_send_fw_cmd(struct isp_context *isp, uint32 cmd_id,
			 enum fw_cmd_resp_stream_id stream,
			 enum fw_cmd_para_type directcmd, void *package,
			 uint32 package_size);
result_t isp_send_fw_cmd_sync(struct isp_context *isp, uint32 cmd_id,
			enum fw_cmd_resp_stream_id stream,
			enum fw_cmd_para_type directcmd, void *package,
			uint32 package_size, uint32 timeout /*in ms */,
			void *resp_pl, uint32 *resp_pl_len);

result_t isp_send_fw_cmd_cid(struct isp_context *isp, enum camera_id cam_id,
			uint32 cmd_id, enum fw_cmd_resp_stream_id stream,
			enum fw_cmd_para_type directcmd, void *package,
			uint32 package_size);

result_t isp_send_fw_cmd_ex(struct isp_context *isp, enum camera_id cam_id,
			uint32 cmd_id, enum fw_cmd_resp_stream_id stream,
			enum fw_cmd_para_type directcmd, void *package,
			uint32 package_size, struct isp_event *evt,
			uint32 *seq, void *resp_pl, uint32 *resp_pl_len);

/*_aidt_queue_command_insert_tail*/
struct isp_cmd_element *isp_append_cmd_2_cmdq(struct isp_context *isp,
					struct isp_cmd_element *command);
struct isp_cmd_element *isp_rm_cmd_from_cmdq(struct isp_context *isp,
					uint32 seq_num, uint32 cmd_id,
					bool_t signal_evt);
struct isp_cmd_element *isp_rm_cmd_from_cmdq_by_stream(struct isp_context *isp,
						enum fw_cmd_resp_stream_id
						stream,
						bool_t signal_evt);

/*refer to _aidt_insert_host2fw_command*/
result_t insert_isp_fw_cmd(struct isp_context *isp,
			enum fw_cmd_resp_stream_id cmd_buf_idx, Cmd_t *cmd);

result_t fw_if_set_brightness(struct isp_context *pcontext, uint32 camera_id,
				int32 brightness);
result_t fw_if_set_contrast(struct isp_context *pcontext, uint32 camera_id,
				int32 contrast);
result_t fw_if_set_hue(struct isp_context *pcontext, uint32 camera_id,
				int32 hue);
result_t fw_if_set_satuation(struct isp_context *pcontext, uint32 camera_id,
				int32 stauation);
result_t fw_if_set_color_enable(struct isp_context *pcontext,
				uint32 camera_id, int32 color_enable);
result_t fw_if_set_anti_banding_mode(struct isp_context *pcontext,
				uint32 camera_id,
				enum anti_banding_mode mode);
result_t fw_if_set_cproc_enable(struct isp_context *pcontext,
				uint32 camera_id, int32 cproc_enable);
result_t fw_if_set_iso(struct isp_context *pcontext, uint32 camera_id,
				AeIso_t iso);
result_t fw_if_set_ev_compensation(struct isp_context *pcontext,
				uint32 camera_id, AeEv_t *ev);
result_t fw_if_set_scene_mode(struct isp_context *pcontext, uint32 camera_id,
				enum pvt_scene_mode mode);

result_t fw_if_get_curr_cproc(struct isp_context *pcontext, uint32 camera_id);
result_t fw_if_cproc_config(struct isp_context *pcontext, uint32 camera_id,
				int32 contrast, int32 brightness,
				int32 saturation, int32 hue);

result_t isp_config_video2d(struct isp_context *pcontext,
				enum camera_id cam_id);
result_t fw_if_send_record_img_buf(struct isp_context *pcontext,
				enum camera_id cam_id,
				struct isp_img_buf_info *buffer);
result_t isp_get_f2h_resp(struct isp_context *isp,
			enum fw_cmd_resp_stream_id stream, Resp_t *response);
result_t isp_fw_start(struct isp_context *pcontext);
result_t isp_fw_boot(struct isp_context *pcontext);
result_t isp_fw_config_stream(struct isp_context *isp_context,
				enum camera_id cam_id, int32 pipe_id,
				enum stream_id str_id);
result_t isp_fw_start_streaming(struct isp_context *isp_context,
				enum camera_id cam_id, int32 pipe_id);
void isp_fw_stop_all_stream(struct isp_context *isp);

void isp_response_process_raw_return(struct isp_context *isp,
				enum camera_id cam_id, uint32 status);

result_t isp_fw_set_focus_mode(struct isp_context *isp,
				enum camera_id cam_id, AfMode_t mode);
result_t isp_fw_start_af(struct isp_context *isp, enum camera_id cam_id);

result_t isp_fw_3a_lock(struct isp_context *isp, enum camera_id cam_id,
			enum isp_3a_type type, LockType_t lock_mode);
result_t isp_fw_3a_unlock(struct isp_context *isp, enum camera_id cam_id,
				enum isp_3a_type type);

result_t isp_fw_set_lens_pos(struct isp_context *isp, enum camera_id cam_id,
				uint32 pos);

result_t isp_fw_set_lens_range(struct isp_context *isp, enum camera_id cid,
				uint32 near_value, uint32 far_value);

result_t isp_fw_set_iso(struct isp_context *isp, enum camera_id cam_id,
			AeIso_t iso);

result_t isp_fw_set_exposure_mode(struct isp_context *isp,
				enum camera_id cam_id, AeMode_t mode);

result_t isp_fw_set_wb_mode(struct isp_context *isp,
				enum camera_id cam_id, AwbMode_t mode);
result_t isp_fw_set_snr_ana_gain(struct isp_context *isp,
				enum camera_id cam_id, uint32 ana_gain);
result_t isp_fw_set_snr_dig_gain(struct isp_context *isp,
				enum camera_id cam_id, uint32 dig_gain);
result_t isp_fw_set_itime(struct isp_context *isp,
				enum camera_id cam_id, uint32 itime);
result_t isp_fw_send_meta_data_buf(struct isp_context *isp,
				enum camera_id cam_id,
				struct isp_meta_data_info *info);
result_t isp_fw_send_all_meta_data(struct isp_context *isp);
result_t isp_fw_set_script(struct isp_context *isp,
				CmdSetDeviceScript_t *script_cmd);
uint32 isp_fw_get_fw_rb_log(struct isp_context *isp, uint8 *temp_buf);
result_t isp_fw_set_gamma(struct isp_context *isp, uint32 camera_id,
				int32 gamma);
result_t isp_fw_set_jpeg_qtable(struct isp_context *isp, enum camera_id cam_id,
				uint32 comp_rate, bool_t main_jpeg);
result_t isp_fw_send_metadata_buf(struct isp_context *pcontext,
				enum camera_id cam_id,
				struct isp_mapped_buf_info *buffer);

result_t isp_set_clk_info_2_fw(struct isp_context *isp);
result_t isp_fw_log_module_set(struct isp_context *isp, ModId_t id,
				bool enable);
result_t isp_fw_log_level_set(struct isp_context *isp, LogLevel_t level);
result_t isp_fw_get_ver(struct isp_context *isp, uint32 *ver);
result_t isp_fw_get_iso_cap(struct isp_context *isp, enum camera_id cid);
result_t isp_fw_get_ev_cap(struct isp_context *isp, enum camera_id cid);
result_t isp_fw_get_cproc_status(struct isp_context *isp,
				enum camera_id cam_id, CprocStatus_t *cproc);

#endif
