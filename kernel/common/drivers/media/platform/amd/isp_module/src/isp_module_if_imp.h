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

#include "os_base_type.h"
#include "isp_module_if.h"

#define ISP_MODULE_IF_VERSION 1

void isp_module_if_init(struct isp_isp_module_if *p_interface);
void isp_module_if_init_ext(struct isp_isp_module_if *p_interface);

int32 set_fw_bin_imp(pvoid context, pvoid fw_data, uint32 fw_len);
int32 set_calib_bin_imp(pvoid context, enum camera_id cam_id,
			pvoid calib_data, uint32 len);

void de_init(pvoid context);
int32 open_camera_imp(pvoid context, enum camera_id camera_id,
		uint32 res_fps_id, uint32_t flag);
int32 close_camera_imp(pvoid context, enum camera_id cid);

int32 start_preview_imp(pvoid context, enum camera_id camera_id);
int32 stop_stream_imp(pvoid context, enum camera_id camera_id,
		enum stream_id str_id, bool pause);
int32 stop_preview_imp(pvoid context, enum camera_id camera_id, bool pause);
int32 set_preview_buf_imp(pvoid context, enum camera_id camera_id,
			sys_img_buf_handle_t buf_hdl);
int32 set_stream_buf_imp(pvoid context, enum camera_id camera_id,
			 sys_img_buf_handle_t buf_hdl,
			 enum stream_id str_id);
int32 set_stream_para_imp(pvoid context, enum camera_id cam_id,
			enum stream_id str_id, enum para_id para_type,
			pvoid para_value);

void reg_notify_cb_imp(pvoid context, enum camera_id cam_id,
		 func_isp_module_cb cb, pvoid cb_context);
void unreg_notify_cb_imp(pvoid context, enum camera_id cam_id);
int32 i2c_read_mem_imp(pvoid context, uint8 bus_num, uint16 slave_addr,
		 i2c_slave_addr_mode_t slave_addr_mode,
		 bool_t enable_restart, uint32 reg_addr,
		 uint8 reg_addr_size, uint8 *p_read_buffer,
		 uint32 byte_size);
int32 i2c_write_mem_imp(pvoid context, uint8 bus_num, uint16 slave_addr,
			i2c_slave_addr_mode_t slave_addr_mode,
			uint32 reg_addr, uint32 reg_addr_size,
			uint8 *p_write_buffer, uint32 byte_size);
int32 i2c_read_reg_imp(pvoid context, uint8 bus_num, uint16 slave_addr,
		 i2c_slave_addr_mode_t slave_addr_mode,
		 bool_t enable_restart, uint32 reg_addr,
		 uint8 reg_addr_size, void *preg_value,
		 uint8 reg_size);
int32 i2c_read_reg_fw_imp(pvoid context, uint32 id,
			uint8 bus_num, uint16 slave_addr,
			i2c_slave_addr_mode_t slave_addr_mode,
			bool_t enable_restart, uint32 reg_addr,
			uint8 reg_addr_size, void *preg_value,
			uint8 reg_size);
int32 i2c_write_reg_imp(pvoid context, uint8 bus_num, uint16 slave_addr,
			i2c_slave_addr_mode_t slave_addr_mode,
			uint32 reg_addr, uint8 reg_addr_size,
			uint32 reg_value, uint8 reg_size);
int32 i2c_write_reg_fw_imp(pvoid context, uint32 id,
			 uint8 bus_num, uint16 slave_addr,
			 i2c_slave_addr_mode_t slave_addr_mode,
			 uint32 reg_addr, uint8 reg_addr_size,
			 uint32 reg_value, uint8 reg_size);
void reg_snr_op_imp(pvoid context, enum camera_id cam_id,
		struct sensor_operation_t *ops);
void unreg_snr_op_imp(pvoid context, enum camera_id cam_id);
void reg_vcm_op_imp(pvoid context, enum camera_id cam_id,
		struct vcm_operation_t *ops);
void unreg_vcm_op_imp(pvoid context, enum camera_id cam_id);
void reg_flash_op_imp(pvoid context, enum camera_id cam_id,
		struct flash_operation_t *ops);
void unreg_flash_op_imp(pvoid context, enum camera_id cam_id);
int32 take_one_pic_imp(pvoid context, enum camera_id cam_id,
		 struct take_one_pic_para *para,
		 sys_img_buf_handle_t buf);
int32 stop_take_one_pic_imp(pvoid context, enum camera_id cam_id);

void write_isp_reg32_imp(pvoid context, uint64 reg_addr, uint32 value);
uint32 read_isp_reg32_imp(pvoid context, uint64 reg_addr);

int32 set_common_para_imp(void *context, enum camera_id camera_id,
			enum para_id para_id /*enum para_id */,
			void *para_value);
int32 get_common_para_imp(void *context, enum camera_id camera_id,
			enum para_id para_id /*enum para_id */,
			void *para_value);
int32 set_preview_para_imp(pvoid context, enum camera_id camera_id,
			 enum para_id para_type, pvoid para_value);
int32 get_preview_para_imp(pvoid context, enum camera_id camera_id,
			 enum para_id para_type, pvoid para_value);
int32 set_zsl_para_imp(pvoid context, enum camera_id camera_id,
		 enum para_id para_type, pvoid para_value);
int32 get_zsl_para_imp(pvoid context, enum camera_id camera_id,
		 enum para_id para_type, pvoid para_value);
int32 set_rgbir_raw_para_imp(pvoid context, enum camera_id camera_id,
			 enum para_id para_type, pvoid para_value);
int32 get_rgbir_raw_para_imp(pvoid context, enum camera_id camera_id,
			 enum para_id para_type, pvoid para_value);

int32 set_video_para_imp(pvoid context, enum camera_id camera_id,
			 enum para_id para_type, pvoid para_value);
int32 get_video_para_imp(pvoid context, enum camera_id camera_id,
			 enum para_id para_type, pvoid para_value);
int32 set_rgbir_ir_para_imp(pvoid context, enum camera_id camera_id,
			enum para_id para_type, pvoid para_value);
int32 get_rgbir_ir_para_imp(pvoid context, enum camera_id camera_id,
			enum para_id para_type, pvoid para_value);

int32 set_video_buf_imp(pvoid context, enum camera_id cam_id,
			sys_img_buf_handle_t buffer_handle);
int32 start_video_imp(pvoid context, enum camera_id camera_id);
int32 start_rgbir_ir_imp(pvoid context, enum camera_id camera_id);

int32 stop_video_imp(pvoid context, enum camera_id camera_id, bool pause);
int32 stop_rgbir_ir_imp(pvoid context, enum camera_id camera_id);

int32 set_one_photo_seq_buf_imp(pvoid context, enum camera_id cam_id,
				sys_img_buf_handle_t main_jpg_buf,
				sys_img_buf_handle_t main_yuv_buf,
				sys_img_buf_handle_t jpg_thumb_buf,
				sys_img_buf_handle_t yuv_thumb_buf);
int32 start_photo_seq_imp(pvoid context, enum camera_id cam_id,
			uint32 fps, uint32 task_bits);
int32 stop_photo_seq_imp(pvoid context, enum camera_id cam_id);
int32 get_capabilities_imp(pvoid context, struct isp_capabilities *cap);
int32 snr_pwr_set_imp(pvoid context, enum camera_id cam_id,
		uint16 *ana_core, uint16 *dig_core,
		uint16 *dig_sel);
int32 snr_clk_set_imp(pvoid context, enum camera_id cam_id,
		uint32 clk /*in k_h_z */);
struct sensor_res_fps_t *get_camera_res_fps_imp(pvoid context,
						enum camera_id cam_id);

bool_t enable_tnr_imp(pvoid context, enum camera_id cam_id,
		enum stream_id str_id,
		uint32 strength /*[0,100] */);
bool_t disable_tnr_imp(pvoid context, enum camera_id cam_id,
		 enum stream_id str_id);

bool_t enable_vs_imp(pvoid context, enum camera_id cam_id,
		 enum stream_id str_id);
bool_t disable_vs_imp(pvoid context, enum camera_id cam_id,
		enum stream_id str_id);

int32 take_raw_pic_imp(pvoid context, enum camera_id cam_id,
		 sys_img_buf_handle_t raw_pic_buf,
		 uint32 use_precapture);
int32 stop_take_raw_pic_imp(pvoid context, enum camera_id cam_id);

int32 set_focus_mode_imp(pvoid context, enum camera_id cam_id,
			 AfMode_t mode);
int32 auto_focus_lock_imp(pvoid context, enum camera_id cam_id,
			LockType_t lock_mode);
int32 auto_focus_unlock_imp(pvoid context, enum camera_id cam_id);
int32 auto_exposure_lock_imp(pvoid context, enum camera_id cam_id,
			 LockType_t lock_mode);
int32 auto_exposure_unlock_imp(pvoid context, enum camera_id cam_id);
int32 auto_wb_lock_imp(pvoid context, enum camera_id cam_id,
		 LockType_t lock_mode);
int32 auto_wb_unlock_imp(pvoid context, enum camera_id cam_id);
int32 set_lens_position_imp(pvoid context, enum camera_id cam_id,
			uint32 pos);
int32 get_focus_position_imp(pvoid context, enum camera_id cam_id,
			 uint32 *pos);
int32 set_exposure_mode_imp(pvoid context, enum camera_id cam_id,
			AeMode_t mode);
int32 set_wb_mode_imp(pvoid context, enum camera_id cam_id,
		AwbMode_t mode);
int32 set_snr_ana_gain_imp(pvoid context, enum camera_id cam_id,
			 uint32 ana_gain);
int32 set_snr_dig_gain_imp(pvoid context, enum camera_id cam_id,
			 uint32 dig_gain);
int32 get_snr_gain_imp(pvoid context, enum camera_id cam_id,
		 uint32 *ana_gain, uint32 *dig_gain);
int32 get_snr_gain_cap_imp(pvoid context, enum camera_id cam_id,
			 struct para_capability_u32 *ana,
			 struct para_capability_u32 *dig);
int32 set_snr_itime_imp(pvoid context, enum camera_id cam_id,
			uint32 itime);
int32 get_snr_itime_imp(pvoid context, enum camera_id cam_id,
			uint32 *itime);
int32 get_snr_itime_cap_imp(pvoid context, enum camera_id cam_id,
			uint32 *min, uint32 *max, uint32 *step);
void reg_fw_parser_imp(pvoid context, enum camera_id cam_id,
		 struct fw_parser_operation_t *parser);
void unreg_fw_parser_imp(pvoid context, enum camera_id cam_id);
int32 set_fw_gv_imp(pvoid context, enum camera_id id,
		enum fw_gv_type type, ScriptFuncArg_t *gv);
int32 set_digital_zoom_imp(pvoid context, enum camera_id cam_id,
			 uint32 h_off, uint32 v_off, uint32 width,
			 uint32 height);

int32 get_snr_raw_img_type_imp(pvoid context, enum camera_id id,
			 enum raw_image_type *type);

int32 set_iso_value_imp(pvoid context, enum camera_id id,
			AeIso_t value);
int32 set_ev_value_imp(pvoid context, enum camera_id id, AeEv_t value);
int32 get_sharpness_imp(pvoid context, enum camera_id id,
			isp_af_sharpness_t *sharp);

int32 set_zsl_buf_imp(pvoid context, enum camera_id cam_id,
		sys_img_buf_handle_t buf_hdl);
int32 start_zsl_imp(pvoid context, enum camera_id cid,
		bool is_perframe_ctl);
int32 stop_zsl_imp(pvoid context, enum camera_id cam_id, bool pause);
int32 take_one_pp_jpeg_imp(pvoid context, enum camera_id cam_id,
			 /*struct isp_pp_jpeg_input *pp_input, */
			 sys_img_buf_handle_t in_yuy2_buf,
			 sys_img_buf_handle_t main_jpg_buf,
			 sys_img_buf_handle_t jpg_thumb_buf,
			 sys_img_buf_handle_t yuv_thumb_buf);

int32 set_digital_zoom_ratio_imp(pvoid context, enum camera_id id,
				 uint32 zoom);

int32 set_digital_zoom_pos_imp(pvoid context, enum camera_id id,
			 int16 h_off, int16 v_off);

void test_gfx_interface_imp(pvoid context);
int32 set_metadata_buf_imp(pvoid context, enum camera_id cam_id,
			 sys_img_buf_handle_t buf_hdl);

int32 get_camera_raw_type_imp(pvoid context, enum camera_id cam_id,
			enum raw_image_type *raw_type,
			uint32 *raw_bayer_pattern);
int32 get_camera_dev_info_imp(pvoid context, enum camera_id cam_id,
			struct camera_dev_info *cam_dev_info);

int32 set_wb_light_imp(pvoid context, enum camera_id cam_id,
		 uint32 light);
int32 disable_color_effect_imp(pvoid context, enum camera_id id);
int32 set_awb_roi_imp(pvoid context, enum camera_id cam_id,
		window_t *window);
int32 set_ae_roi_imp(pvoid context, enum camera_id cam_id,
		 window_t *window);
int32 set_af_roi_imp(pvoid context, enum camera_id cam_id,
		 AfWindowId_t id, window_t *window);

int32 start_af_imp(pvoid context, enum camera_id cam_id);
int32 stop_af_imp(pvoid context, enum camera_id cam_id);
int32 cancel_af_imp(pvoid context, enum camera_id cam_id);
int32 start_wb_imp(pvoid context, enum camera_id cam_id);
int32 stop_wb_imp(pvoid context, enum camera_id cam_id);
int32 start_ae_imp(pvoid context, enum camera_id cam_id);
int32 stop_ae_imp(pvoid context, enum camera_id cam_id);
int32 set_scene_mode_imp(pvoid context, enum camera_id cam_id,
			 enum isp_scene_mode mode);
int32 set_gamma_imp(pvoid context, enum camera_id cam_id, int32 value);
int32 set_color_temperature_imp(pvoid context, enum camera_id cam_id,
			uint32 value);
int32 set_snr_calib_data_imp(pvoid context, enum camera_id cam_id,
			 pvoid data, uint32 len);

int32 map_handle_to_vram_addr_imp(pvoid context, void *handle,
				uint64 *vram_addr);
int32 set_per_frame_ctl_info_imp(pvoid context, enum camera_id cam_id,
				 struct frame_ctl_info *ctl_info,
				 sys_img_buf_handle_t zsl_buf);

int32 set_drv_settings_imp(pvoid context,
			 struct driver_settings *setting);

int32 enable_dynamic_frame_rate_imp(pvoid context,
				enum camera_id cam_id, bool enable);

int32 set_max_frame_rate_imp(pvoid context, enum camera_id cam_id,
			 enum stream_id sid,
			 uint32 frame_rate_numerator,
			 uint32 frame_rate_denominator);

int32 set_flicker_imp(pvoid context, enum camera_id cam_id,
		enum anti_banding_mode anti_banding);
int32 set_iso_priority_imp(pvoid context, enum camera_id cam_id,
		enum stream_id sid, AeIso_t *iso);
int32 set_ev_compensation_imp(pvoid context, enum camera_id cam_id,
		AeEv_t *ev);
int32 set_lens_range_imp(pvoid context, enum camera_id cam_id,
			 uint32 near_value, uint32 far_value);

int32 set_flash_mode_imp(pvoid context, enum camera_id cam_id,
			 FlashMode_t mode);

int32 set_flash_power_imp(pvoid context, enum camera_id cam_id,
			uint32 pwr);

int32 set_red_eye_mode_imp(pvoid context, enum camera_id cam_id,
			 RedEyeMode_t mode);

int32 set_flash_assist_mode_imp(pvoid context, enum camera_id cam_id,
				FocusAssistMode_t mode);

int32 set_flash_assist_power_imp(pvoid context, enum camera_id cam_id,
				 uint32 pwr);

int32 set_video_hdr_imp(pvoid context, enum camera_id cam_id,
			uint32 enable);
int32 set_lens_shading_matrix_imp(pvoid context, enum camera_id cam_id,
				LscMatrix_t *matrix);
int32 set_lens_shading_sector_imp(pvoid context, enum camera_id cam_id,
				PrepLscSector_t *sectors);
int32 set_awb_cc_matrix_imp(pvoid context, enum camera_id cam_id,
			CcMatrix_t *matrix);
int32 set_awb_cc_offset_imp(pvoid context, enum camera_id cam_id,
			CcOffset_t *ccOffset);

int32 gamma_enable_imp(pvoid context, enum camera_id cam_id,
		 uint32 enable);
int32 wdr_enable_imp(pvoid context, enum camera_id cam_id,
		 uint32 enable);
int32 tnr_enable_imp(pvoid context, enum camera_id cam_id,
		 uint32 enable);
int32 snr_enable_imp(pvoid context, enum camera_id cam_id,
		 uint32 enable);
int32 dpf_enable_imp(pvoid context, enum camera_id cam_id,
		 uint32 enable);
int32 start_rgbir_imp(pvoid context, enum camera_id cam_id);

/**
 *stop RGBIR .
 */
int32 stop_rgbir_imp(pvoid context, enum camera_id cam_id);

int32 set_rgbir_output_buf_imp(pvoid context, enum camera_id cam_id,
			 sys_img_buf_handle_t buf_hdl,
			 uint32 bayer_raw_len, uint32 ir_raw_len,
			 uint32 frame_info_len);
int32 set_rgbir_input_buf_imp(pvoid context, enum camera_id cam_id,
			sys_img_buf_handle_t bayer_raw_buf,
			sys_img_buf_handle_t frame_info);
int32 set_mem_cam_input_buf_imp(pvoid context, enum camera_id cam_id,
				sys_img_buf_handle_t bayer_raw_buf,
				sys_img_buf_handle_t frame_info);
int32 get_original_wnd_size_imp(pvoid context, enum camera_id cam_id,
				uint32 *width, uint32 *height);

int32 get_cproc_status_imp(pvoid context, enum camera_id cam_id,
			   CprocStatus_t *cproc);
int32 cproc_enable_imp(pvoid context, enum camera_id cam_id,
			uint32 enable);
int32 cproc_set_contrast_imp(pvoid context, enum camera_id cam_id,
			uint32 contrast);
int32 cproc_set_brightness_imp(pvoid context, enum camera_id cam_id,
			uint32 brightness);
int32 cproc_set_saturation_imp(pvoid context, enum camera_id cam_id,
			uint32 saturation);
int32 cproc_set_hue_imp(pvoid context, enum camera_id cam_id,
			uint32 hue);

int32 get_init_para_imp(pvoid context, void *sw_init,
			void *hw_init, void *isp_env
);

int32 lfb_resv_imp(pvoid context, uint32 size,
		 uint64 *sys, uint64 *mc);

int32 lfb_free_imp(pvoid context);

uint32 get_index_from_res_fps_id(pvoid context, enum camera_id cid,
				 uint32 res_fps_id);

int32 get_raw_img_info_imp(pvoid context, enum camera_id cam_id,
			 uint32 *w, uint32 *h, uint32 *raw_type,
			 uint32 *raw_pkt_fmt);
int32 fw_cmd_send_imp(pvoid context, enum camera_id cam_id,
		uint32 cmd, uint32 is_set_cmd,
		uint16 is_stream_cmd, uint16 to_ir,
		uint32 is_direct_cmd,
		pvoid para, uint32 *para_len);
int32 get_tuning_data_info_imp(pvoid context, enum camera_id cam_id,
			 uint32 *struct_ver, uint32 *date,
			 char *author, char *cam_name);
int32 loopback_start_imp(pvoid context, enum camera_id cam_id,
			 uint32 yuv_format, uint32 yuv_width,
			 uint32 yuv_height, uint32 yuv_ypitch,
			 uint32 yuv_uvpitch);
int32 loopback_stop_imp(pvoid context, enum camera_id cam_id);
int32 loopback_set_raw_imp(pvoid context, enum camera_id cam_id,
			 sys_img_buf_handle_t raw_buf,
			 sys_img_buf_handle_t frame_info);
int32 loopback_process_raw_imp(pvoid context, enum camera_id cam_id,
			 uint32 sub_tuning_cmd_count,
			 struct loopback_sub_tuning_cmd
			 *sub_tuning_cmd,
			 sys_img_buf_handle_t buf,
			 uint32 yuv_buf_len, uint32 meta_buf_len);

int32 set_face_authtication_mode_imp(pvoid context, enum camera_id cid,
				 bool_t mode);
int32 get_isp_work_buf_imp(pvoid context, pvoid work_buf_info);


uint32 isp_fb_get_temp_raw_offset(enum camera_id cid);
uint32 isp_fb_get_temp_raw_size(enum camera_id cid);

void isp_acpi_set_sensor_pwr(struct isp_context *isp,
			enum camera_id cid, bool on);
uint32 isp_fb_get_calib_data_offset(enum camera_id cid);

