/*
 * Copyright (C) 2019-2023 Advanced Micro Devices, Inc. All rights reserved.
 */

#include "os_base_type.h"
#include "isp_module_if.h"

#define ISP_MODULE_IF_VERSION 1
#define ISP_NEED_CLKSWITCH(frm_ctrl) \
	((frm_ctrl)->frameCtrl.privateData.data[SWITCH_LOW_CLK_IDX] == \
	 CLOCK_SWITCH_ENABLE)

void isp_module_if_init(struct isp_isp_module_if *p_interface);
void isp_module_if_init_ext(struct isp_isp_module_if *p_interface);

int set_fw_bin_imp(void *context, void *fw_data, unsigned int fw_len);
int set_calib_bin_imp(void *context, enum camera_id cam_id,
			void *calib_data, unsigned int len);

void de_init(void *context);
int open_camera_imp(void *context, enum camera_id camera_id,
		unsigned int res_fps_id, uint32_t flag);
int open_camera_fps_imp(void *context, enum camera_id camera_id,
		unsigned int fps, uint32_t flag);
int close_camera_imp(void *context, enum camera_id cid);

int start_stream_imp(void *context, enum camera_id cam_id);
int set_test_pattern_imp(void *context, enum camera_id cam_id, void *param);
int switch_profile_imp(void *context, enum camera_id cid, unsigned int prf_id);
int start_preview_imp(void *context, enum camera_id camera_id);
int stop_stream_imp(void *context, enum camera_id camera_id,
		enum stream_id str_id, bool pause);
int stop_preview_imp(void *context,
	enum camera_id camera_id, bool pause);
int set_preview_buf_imp(void *context, enum camera_id camera_id,
			struct sys_img_buf_handle *buf_hdl);
int set_stream_buf_imp(void *context, enum camera_id camera_id,
			 struct sys_img_buf_handle *buf_hdl,
			 enum stream_id str_id);
int set_stream_para_imp(void *context, enum camera_id cam_id,
			enum stream_id str_id, enum para_id para_type,
			void *para_value);

void reg_notify_cb_imp(void *context, enum camera_id cam_id,
		 func_isp_module_cb cb, void *cb_context);
void unreg_notify_cb_imp(void *context, enum camera_id cam_id);
int i2c_read_mem_imp(void *context, unsigned char bus_num,
		 unsigned short slave_addr,
		 enum _i2c_slave_addr_mode_t slave_addr_mode,
		 int enable_restart, unsigned int reg_addr,
		 unsigned char reg_addr_size, unsigned char *p_read_buffer,
		 unsigned int byte_size);
int i2c_write_mem_imp(void *context, unsigned char bus_num,
			unsigned short slave_addr,
			enum _i2c_slave_addr_mode_t slave_addr_mode,
			unsigned int reg_addr, unsigned int reg_addr_size,
			unsigned char *p_write_buffer, unsigned int byte_size);
int i2c_read_reg_imp(void *context, unsigned char bus_num,
		 unsigned short slave_addr,
		 enum _i2c_slave_addr_mode_t slave_addr_mode,
		 int enable_restart, unsigned int reg_addr,
		 unsigned char reg_addr_size, void *preg_value,
		 unsigned char reg_size);
int i2c_write_reg_imp(void *context, unsigned char bus_num,
			unsigned short slave_addr,
			enum _i2c_slave_addr_mode_t slave_addr_mode,
			unsigned int reg_addr, unsigned char reg_addr_size,
			unsigned int reg_value, unsigned char reg_size);

int i2c_write_reg_imp_group(void *context, unsigned char bus_num,
			unsigned short slave_addr,
			enum _i2c_slave_addr_mode_t slave_addr_mode,
			unsigned int reg_addr, unsigned char reg_addr_size,
			unsigned int *reg_value, unsigned char reg_size);

void reg_snr_op_imp(void *context, enum camera_id cam_id,
			  struct sensor_operation_t *ops);
void unreg_snr_op_imp(void *context, enum camera_id cam_id);
void reg_vcm_op_imp(void *context, enum camera_id cam_id,
			  struct vcm_operation_t *ops);
void unreg_vcm_op_imp(void *context, enum camera_id cam_id);
void reg_flash_op_imp(void *context, enum camera_id cam_id,
		struct flash_operation_t *ops);
void unreg_flash_op_imp(void *context, enum camera_id cam_id);
int take_one_pic_imp(void *context, enum camera_id cam_id,
		 struct take_one_pic_para *para,
		 struct sys_img_buf_handle *buf);
int stop_take_one_pic_imp(void *context, enum camera_id cam_id);

/*
 * For online tuning used. Exchange ISP firmware commands.
 * This function was binded to function pointer `online_isp_tune` in file
 * 'isp_module_if.h'. Refer to the file for more information.
 */
int online_tune_imp(void *context, enum camera_id cam_id,
		unsigned int cmd, unsigned int is_set_cmd,
		unsigned short is_stream_cmd, unsigned int is_direct_cmd,
		void *para, unsigned int *para_len,
		void *resp, unsigned int resp_len);

void write_isp_reg32_imp(void *context, unsigned long long reg_addr,
	unsigned int value);
unsigned int read_isp_reg32_imp(void *context,
	unsigned long long reg_addr);

int set_common_para_imp(void *context, enum camera_id camera_id,
			enum para_id para_id /*enum para_id */,
			void *para_value);
int get_common_para_imp(void *context, enum camera_id camera_id,
			enum para_id para_id /*enum para_id */,
			void *para_value);
int set_preview_para_imp(void *context, enum camera_id camera_id,
			 enum para_id para_type, void *para_value);
int get_preview_para_imp(void *context, enum camera_id camera_id,
			 enum para_id para_type, void *para_value);
int set_zsl_para_imp(void *context, enum camera_id camera_id,
		 enum para_id para_type, void *para_value);
int get_zsl_para_imp(void *context, enum camera_id camera_id,
		 enum para_id para_type, void *para_value);
int set_video_para_imp(void *context, enum camera_id camera_id,
			 enum para_id para_type, void *para_value);
int get_video_para_imp(void *context, enum camera_id camera_id,
			 enum para_id para_type, void *para_value);
int set_video_buf_imp(void *context, enum camera_id cam_id,
			struct sys_img_buf_handle *buffer_handle);
int start_video_imp(void *context, enum camera_id camera_id);
int stop_video_imp(void *context, enum camera_id camera_id, bool pause);
int set_one_photo_seq_buf_imp(void *context,
				enum camera_id cam_id,
				struct sys_img_buf_handle *main_jpg_buf,
				struct sys_img_buf_handle *main_yuv_buf,
				struct sys_img_buf_handle *jpg_thumb_buf,
				struct sys_img_buf_handle *yuv_thumb_buf);
int start_photo_seq_imp(void *context, enum camera_id cam_id,
			unsigned int fps, unsigned int task_bits);
int stop_photo_seq_imp(void *context, enum camera_id cam_id);
int get_capabilities_imp(void *context, struct isp_capabilities *cap);
int snr_pwr_set_imp(void *context, enum camera_id cam_id,
		unsigned short *ana_core, unsigned short *dig_core,
		unsigned short *dig_sel);
int snr_clk_set_imp(void *context, enum camera_id cam_id,
		unsigned int clk /*in k_h_z */);
struct sensor_res_fps_t *get_camera_res_fps_imp(void *context,
						enum camera_id cam_id);

int enable_tnr_imp(void *context, enum camera_id cam_id,
		enum stream_id str_id,
		unsigned int strength /*[0,100] */);
int disable_tnr_imp(void *context, enum camera_id cam_id,
		 enum stream_id str_id);

int enable_vs_imp(void *context, enum camera_id cam_id,
		 enum stream_id str_id);
int disable_vs_imp(void *context, enum camera_id cam_id,
		enum stream_id str_id);

int take_raw_pic_imp(void *context, enum camera_id cam_id,
		 struct sys_img_buf_handle *raw_pic_buf,
		 unsigned int use_precapture);
int stop_take_raw_pic_imp(void *context, enum camera_id cam_id);
int auto_exposure_lock_imp(void *context, enum camera_id cam_id,
			 enum _LockType_t lock_mode);
int auto_exposure_unlock_imp(void *context, enum camera_id cam_id);
int cproc_enable_imp(void *context, enum camera_id cam_id,
			unsigned int enable);
int cproc_set_contrast_imp(void *context, enum camera_id cam_id,
			unsigned int contrast);
int cproc_set_brightness_imp(void *context, enum camera_id cam_id,
			unsigned int brightness);
int cproc_set_saturation_imp(void *context, enum camera_id cam_id,
			unsigned int saturation);
int cproc_set_hue_imp(void *context, enum camera_id cam_id,
			unsigned int hue);
int auto_wb_lock_imp(void *context, enum camera_id cam_id,
		 enum _LockType_t lock_mode);
int auto_wb_unlock_imp(void *context, enum camera_id cam_id);
int set_exposure_mode_imp(void *context, enum camera_id cam_id,
			enum _AeMode_t mode);
int set_wb_mode_imp(void *context, enum camera_id cam_id,
		enum _AwbMode_t mode);
int set_snr_ana_gain_imp(void *context, enum camera_id cam_id,
			 unsigned int ana_gain);
int set_snr_dig_gain_imp(void *context, enum camera_id cam_id,
			 unsigned int dig_gain);
int get_snr_gain_imp(void *context, enum camera_id cam_id,
		 unsigned int *ana_gain, unsigned int *dig_gain);
int get_snr_gain_cap_imp(void *context, enum camera_id cam_id,
			 struct para_capability_u32 *ana,
			 struct para_capability_u32 *dig);
int set_snr_itime_imp(void *context, enum camera_id cam_id,
			unsigned int itime);
int get_snr_itime_imp(void *context, enum camera_id cam_id,
			unsigned int *itime);
int get_snr_itime_cap_imp(void *context, enum camera_id cam_id,
			unsigned int *min, unsigned int *max,
			unsigned int *step);
void reg_fw_parser_imp(void *context, enum camera_id cam_id,
		 struct fw_parser_operation_t *parser);
void unreg_fw_parser_imp(void *context, enum camera_id cam_id);
int set_fw_gv_imp(void *context, enum camera_id id,
		enum fw_gv_type type, struct _ScriptFuncArg_t *gv);
int set_digital_zoom_imp(void *context, enum camera_id cam_id,
			 unsigned int h_off, unsigned int v_off,
			 unsigned int width,
			 unsigned int height);
int get_sharpness_imp(void *context, enum camera_id id,
			struct _isp_af_sharpness_t *sharp);

int set_zsl_buf_imp(void *context, enum camera_id cam_id,
		struct sys_img_buf_handle *buf_hdl);
int start_zsl_imp(void *context, enum camera_id cid,
		bool is_perframe_ctl);
int stop_zsl_imp(void *context, enum camera_id cam_id, bool pause);
int take_one_pp_jpeg_imp(void *context, enum camera_id cam_id,
			 /*struct isp_pp_jpeg_input *pp_input, */
			 struct sys_img_buf_handle *in_yuy2_buf,
			 struct sys_img_buf_handle *main_jpg_buf,
			 struct sys_img_buf_handle *jpg_thumb_buf,
			 struct sys_img_buf_handle *yuv_thumb_buf);

int set_digital_zoom_ratio_imp(void *context, enum camera_id id,
				 unsigned int zoom);

int set_digital_zoom_pos_imp(void *context, enum camera_id id,
		short h_off, short v_off);

int set_metadata_buf_imp(void *context, enum camera_id cam_id,
			 struct sys_img_buf_handle *buf_hdl);
int get_camera_dev_info_imp(void *context, enum camera_id cam_id,
			struct camera_dev_info *cam_dev_info);
int get_caps_data_format_imp(void *context, enum camera_id cam_id,
			enum pvt_img_fmt fmt);
int get_caps_imp(void *context, enum camera_id cid);
int set_wb_light_imp(void *context, enum camera_id cam_id,
		 unsigned int light);
int set_wb_gain_imp(void *context, enum camera_id cam_id,
	struct _WbGain_t wb_gain);
int set_wb_colorT_imp(void *context, enum camera_id cam_id,
	unsigned int colorT);
int disable_color_effect_imp(void *context, enum camera_id id);
int set_awb_roi_imp(void *context, enum camera_id cam_id,
		struct _Window_t *window);
int set_ae_roi_imp(void *context, enum camera_id cam_id,
		 struct _Window_t *window);
int start_wb_imp(void *context, enum camera_id cam_id);
int stop_wb_imp(void *context, enum camera_id cam_id);
int start_ae_imp(void *context, enum camera_id cam_id);
int stop_ae_imp(void *context, enum camera_id cam_id);
int set_scene_mode_imp(void *context, enum camera_id cam_id,
			 enum isp_scene_mode mode);
int set_snr_calib_data_imp(void *context, enum camera_id cam_id,
			 void *data, unsigned int len);

int map_handle_to_vram_addr_imp(void *context, void *handle,
				unsigned long long *vram_addr);

int set_drv_settings_imp(void *context,
			 struct driver_settings *setting);

int enable_dynamic_frame_rate_imp(void *context,
				enum camera_id cam_id, bool enable);

int set_max_frame_rate_imp(void *context, enum camera_id cam_id,
			 enum stream_id sid,
			 unsigned int frame_rate_numerator,
			 unsigned int frame_rate_denominator);

int set_frame_rate_info_imp(void *context, enum camera_id cam_id,
			enum stream_id sid, unsigned int frame_rate);
int set_flicker_imp(void *context, enum camera_id cam_id,
			enum stream_id sid,
			enum _AeFlickerType_t mode);
int set_iso_priority_imp(void *context, enum camera_id cam_id,
		enum stream_id sid, struct _AeIso_t *iso);
int set_ev_compensation_imp(void *context, enum camera_id cam_id,
		enum stream_id sid, struct _AeEv_t *ev);
int sharpen_enable_imp(void *context, enum camera_id cam_id,
			enum stream_id sid, enum _SharpenId_t *sharpen_id);
int sharpen_disable_imp(void *context, enum camera_id cam_id,
			enum stream_id sid, enum _SharpenId_t *sharpen_id);
int sharpen_config_imp(void *context, enum camera_id cam_id,
			enum stream_id sid, enum _SharpenId_t sharpen_id,
			struct _TdbSharpRegByChannel_t param);
int tnr_config_imp(void *context, enum camera_id cam_id,
			enum stream_id sid, struct _TemperParams_t *param);
int snr_config_imp(void *context, enum camera_id cam_id,
			enum stream_id sid, struct _SinterParams_t *param);
int clk_gating_enable_imp(void *context, enum camera_id cam_id,
			unsigned int enable);
int power_gating_enable_imp(void *context, enum camera_id cam_id,
			unsigned int enable);
int dump_raw_enable_imp(void *context, enum camera_id cam_id,
			unsigned int enable);
int image_effect_enable_imp(void *context, enum camera_id cam_id,
			enum stream_id sid, unsigned int enable);
int image_effect_config_imp(void *context, enum camera_id cam_id,
	enum stream_id sid, struct _IeConfig_t *param, int enable);

int set_video_hdr_imp(void *context, enum camera_id cam_id,
			unsigned int enable);
int set_lens_shading_matrix_imp(void *context,
				enum camera_id cam_id,
				struct _LscMatrix_t *matrix);
int set_lens_shading_sector_imp(void *context,
				enum camera_id cam_id,
				struct _PrepLscSector_t *sectors);
int set_awb_cc_matrix_imp(void *context, enum camera_id cam_id,
			struct _CcMatrix_t *matrix);
int set_awb_cc_offset_imp(void *context, enum camera_id cam_id,
			struct _CcOffset_t *ccOffset);

int gamma_enable_imp(void *context, enum camera_id cam_id,
		 unsigned int enable);
int tnr_enable_imp(void *context, enum camera_id cam_id,
		 unsigned int enable);
int snr_enable_imp(void *context, enum camera_id cam_id,
		 unsigned int enable);

int get_cproc_status_imp(void *context, enum camera_id cam_id,
			struct _CprocStatus_t *cproc);

int get_init_para_imp(void *context, void *sw_init,
			void *hw_init, void *isp_env
);

int lfb_resv_imp(void *context, unsigned int size,
		 unsigned long long *sys, unsigned long long *mc);

int lfb_free_imp(void *context);

unsigned int get_index_from_res_fps_id(void *context,
			enum camera_id cid,
			unsigned int res_fps_id);
int fw_cmd_send_imp(void *context, enum camera_id cam_id,
		unsigned int cmd, unsigned int is_set_cmd,
		unsigned short is_stream_cmd, unsigned short to_ir,
		unsigned int is_direct_cmd,
		void *para, unsigned int *para_len);
int get_isp_work_buf_imp(void *context, void *work_buf_info);


unsigned int isp_fb_get_temp_raw_offset(enum camera_id cid);
unsigned int isp_fb_get_temp_raw_size(enum camera_id cid);

void isp_acpi_set_sensor_pwr(struct isp_context *isp,
			enum camera_id cid, bool on);
unsigned int isp_fb_get_calib_data_offset(enum camera_id cid);

int set_frame_ctrl_imp(void *context, enum camera_id camera_id,
		       struct _CmdFrameCtrl_t *frm_ctrl);
int start_cvip_sensor_imp(void *context, enum camera_id cam_id);
