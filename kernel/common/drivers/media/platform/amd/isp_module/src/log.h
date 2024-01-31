/*
 * Copyright (C) 2019-2020 Advanced Micro Devices, Inc. All rights reserved.
 */

#ifndef __LOG_H__
#define __LOG_H__
#if defined(__cplusplus)
extern "C" {
#endif
extern uint32 g_log_level_ispm;
extern uint32 dbg_flag;

#define _DRIVER_NAME_ "AIP:"
#define _DRIVER_NAME_LEN 4

#define CASE_MACRO_2_MACRO_STR(X) {case X: return #X; }
/*#define DBG_MSG_BUFFER_LEN	(64 *1024+24)*/
#define DBG_MSG_BUFFER_LEN	(4 * 1024)

/*for performance test*/
#define PT_EVT_RC	"RC"	/*receive cmd from upper layer */
#define PT_EVT_SC	"SC"	/*send cmd to low layer */
#define PT_EVT_RCD	"RCD"	/*receive cmd done from low laye */
#define PT_EVT_SCD	"SCD"	/*send cmd done to up layer */
#define PT_CMD_OC	"OC"	/*open camera */
#define PT_CMD_CC	"CC"	/*close camera */
#define PT_CMD_SS	"SS"	/*start stream */
#define PT_CMD_ES	"ES"	/*end stream */
#define PT_CMD_SP	"SP"	/*start preview */
#define PT_CMD_EP	"EP"	/*end preview */
#define PT_CMD_SV	"SV"	/*start video */
#define PT_CMD_EV	"EV"	/*end video */
#define PT_CMD_EZ	"EZ"	/*end zsl */
#define PT_CMD_SZ	"SZ"	/*start zsl */
#define PT_CMD_ER	"ER"	/*end zsl */
#define PT_CMD_SCY	"SCY"	/*start capturing YUV */
#define PT_CMD_ECY	"ECY"	/*end capturing YUV */
#define PT_CMD_SCR	"SCR"	/*start capturing RAW */
#define PT_CMD_ECR	"ECR"	/*end capturing RAW */
#define PT_CMD_IPON	"IPON"	/*isp isp power on */
#define PT_CMD_IPOFF	"IPOFF"	/*isp isp power off */
#define PT_CMD_PHYON	"PHYON"	/*phy power on */
#define PT_CMD_PHYOFF	"PHYOFF"	/*phy power off */
#define PT_CMD_SNRO	"SNRO"	/*sensor open */
#define PT_CMD_SNRC	"SNRC"	/*sensor close */
#define PT_CMD_SF	"SF"	/*start FW */
#define PT_CMD_PB	"PB"	/*preview buffer */
#define PT_CMD_VB	"VB"	/*video buffer */
#define PT_CMD_ZB	"ZB"	/*zsl buffer */
#define PT_CMD_RB	"RB"	/*rgb ir buffer */
#define PT_CMD_FC	"FC"	/*preview buffer */
#define PT_STATUS_SUCC	"S"	/*success */
#define PT_STATUS_FAIL	"F"	/*fail */

#define TRACE_LEVEL_NONE	0
#define TRACE_LEVEL_CRITICAL	1
#define TRACE_LEVEL_FATAL	1
#define TRACE_LEVEL_ERROR	2
#define TRACE_LEVEL_WARNING	3
#define TRACE_LEVEL_INFORMATION	4
#define TRACE_LEVEL_VERBOSE	5
#define TRACE_LEVEL_RESERVED6	6
#define TRACE_LEVEL_RESERVED7	7
#define TRACE_LEVEL_RESERVED8	8
#define TRACE_LEVEL_RESERVED9	9

#define ISP_DBG_LEVEL 2

#define ISP_A 0x1
#define ISP_M 0x2
#define ISP_D 0x4
#define CAM_D 0x8

#ifdef ENABLE_PERFORMANCE_PROFILE_LOG
#define isp_dbg_print_pt(evt, cmd, status) \
	{isp_dbg_print_info("[PT][Cam][IM][%s][%s][%s]\n", evt, cmd, status); }
#define isp_dbg_print_pt_ex(evt, cmd, status, format, ...) \
	{isp_dbg_print_info("[PT][Cam][IM][%s][%s][%s]" \
	format, evt, cmd, status, ##__VA_ARGS__); }
#else
#define isp_dbg_print_pt(fmt, evt, cmd, status)
#define isp_dbg_print_pt_ex(evt, cmd, status, format, ...)
#endif

#define isp_dbg_print_info(fmt, ...) { if (ISP_DBG_LEVEL >= \
	TRACE_LEVEL_INFORMATION) { pr_info(fmt, ##__VA_ARGS__); } }
#define isp_dbg_print_warn(fmt, ...) { if (ISP_DBG_LEVEL >= \
	TRACE_LEVEL_WARNING) { pr_warn(fmt, ##__VA_ARGS__); } }
#define isp_dbg_print_err(fmt, ...) { if (ISP_DBG_LEVEL >= \
	TRACE_LEVEL_ERROR) { pr_err(fmt, ##__VA_ARGS__); } }
#define isp_dbg_print_verb(fmt, ...) { if (ISP_DBG_LEVEL >= \
	TRACE_LEVEL_VERBOSE) { pr_notice(fmt, ##__VA_ARGS__); } }
#define isp_dbg_print_info_fw(fmt, ...) { if (ISP_DBG_LEVEL >= \
	TRACE_LEVEL_ERROR) { pr_info(fmt, ##__VA_ARGS__); } }

	char *get_host_2_fw_cmd_str(uint32 cmd_id);
	char *isp_dbg_get_isp_status_str(uint32 status);
	void dbg_show_sensor_prop(void *sensor_cfg);
	void dbg_show_sensor_caps(void *sensor_cap);
	void isp_dbg_show_map_info(void *p);
	void isp_dbg_show_bufmeta_info(char *pre, uint32 cid,
				void *p,
				void *origBuf
				/*struct sys_img_buf_handle**/);
	char *isp_dbg_get_img_fmt_str(void *in /*ImageFormat_t **/);
	char *isp_dbg_get_pvt_fmt_str(int32 fmt /*enum pvt_img_fmt */);
	char *isp_dbg_get_out_fmt_str(int32 fmt /*enum ImageFormat_t */);
	char *isp_dbg_get_cmd_str(uint32 cmd);
	char *isp_dbg_get_buf_type(uint32 type);	/*BufferType_t */
	char *isp_dbg_get_resp_str(uint32 resp);
	char *isp_dbg_get_buf_src_str(uint32 src);
	char *isp_dbg_get_buf_done_str(uint32 status);
	char *isp_dbg_get_iso_str(uint32 iso);
	char *isp_dbg_get_af_mode_str(uint32 mode /*AfMode_t */);
	char *isp_dbg_get_ae_mode_str(uint32 mode /*AeMode_t */);
	char *isp_dbg_get_awb_mode_str(uint32 mode /*AwbMode_t */);
	void isp_dbg_show_img_prop(char *pre, void *p /*ImageProp_t * */);
	void dbg_show_drv_settings(void *setting);
	char *isp_dbg_get_scene_mode_str(uint32 mode);
	char *isp_dbg_get_stream_str(uint32 stream);
	char *isp_dbg_get_para_str(uint32 para /*enum para_id */);
	char *isp_dbg_get_iso_str(uint32 para /*AeIso_t */);
	char *isp_dbg_get_flash_mode_str(uint32 mode /*FlashMode_t */);
	char *isp_dbg_get_flash_assist_mode_str(uint32 mode
						/*FocusAssistMode_t */);
	char *isp_dbg_get_red_eye_mode_str(uint32 mode);
	char *isp_dbg_get_anti_flick_str(uint32 type);
	void isp_dbg_show_frame3_info(void *p /*Mode3FrameInfo_t * */,
				char *prefix);
	void isp_dbg_show_af_ctrl_info(void *p /*AfCtrlInfo_t * */);
	void isp_dbg_show_ae_ctrl_info(void *p /*AeCtrlInfo_t * */);
	char *isp_dbg_get_focus_state_str(uint32 state);
	char *isp_dbg_get_reg_name(uint32 reg);
	char *isp_dbg_get_work_buf_str(uint32 type);

	void isp_dbg_assert_func(int32 reason);

	void isp_dbg_dump_ae_status(void *p /*AeStatus_t * */);

#if defined(__cplusplus)
};
#endif

#endif /*__LOG_H__*/
