/*
 * Copyright (C) 2019-2021 Advanced Micro Devices, Inc. All rights reserved.
 */

#ifndef __LOG_H__
#define __LOG_H__
#if defined(__cplusplus)
extern "C" {
#endif

#define CASE_MACRO_2_MACRO_STR(X) {case X: ret = #X; break; }

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "ISP"

extern struct s_properties *g_prop;

#define ISP_DEBUG_LEVEL (g_prop ? g_prop->drv_log_level : TRACE_LEVEL_ERROR)

#define TRACE_LEVEL_NONE	0
#define TRACE_LEVEL_ERROR	1
#define TRACE_LEVEL_WARNING	2
#define TRACE_LEVEL_INFO	3
#define TRACE_LEVEL_DEBUG	4
#define TRACE_LEVEL_VERBOSE	5

#define ENTER() {if (ISP_DEBUG_LEVEL >= TRACE_LEVEL_DEBUG) { \
	pr_info("[D][Cam]%s[%s][%d]: Entry!\n", \
	LOG_TAG, __func__, __LINE__); } }
#define EXIT() {if (ISP_DEBUG_LEVEL >= TRACE_LEVEL_DEBUG) { \
	pr_info("[D][Cam]%s[%s][%d]: Exit!\n", \
	LOG_TAG, __func__, __LINE__); } }
#define RET(X) {if (ISP_DEBUG_LEVEL >= TRACE_LEVEL_DEBUG) { \
	pr_info("[D][Cam]%s[%s][%d]: Exit with %d!\n", \
	LOG_TAG, __func__, __LINE__, X); } }

//PC: performance check
#define ISP_PR_PC(format, ...) { if (ISP_DEBUG_LEVEL >= \
	TRACE_LEVEL_WARNING) { pr_info("[P][Cam]%s[%s][%d]:" \
	format, LOG_TAG, __func__, __LINE__, ##__VA_ARGS__); } }

#define ISP_PR_ERR(format, ...) { if (ISP_DEBUG_LEVEL >= \
	TRACE_LEVEL_ERROR) { pr_err("[E][Cam]%s[%s][%d]:" \
	format, LOG_TAG, __func__, __LINE__, ##__VA_ARGS__); } }

#define ISP_PR_WARN(format, ...) { if (ISP_DEBUG_LEVEL >= \
	TRACE_LEVEL_WARNING) { pr_warn("[W][Cam]%s[%s][%d]:" \
	format, LOG_TAG, __func__, __LINE__, ##__VA_ARGS__); } }

#define ISP_PR_INFO(format, ...) { if (ISP_DEBUG_LEVEL >= \
	TRACE_LEVEL_INFO) { pr_info("[I][Cam]%s[%s][%d]:" \
	format, LOG_TAG, __func__, __LINE__, ##__VA_ARGS__); } }

#define ISP_PR_DBG(format, ...) { if (ISP_DEBUG_LEVEL >= \
	TRACE_LEVEL_DEBUG) { pr_info("[D][Cam]%s[%s][%d]:" \
	format, LOG_TAG, __func__, __LINE__, ##__VA_ARGS__); } }

#define ISP_PR_VERB(format, ...) { if (ISP_DEBUG_LEVEL >= \
	TRACE_LEVEL_VERBOSE) { pr_notice("[V][Cam]%s[%s][%d]:" \
	format, LOG_TAG, __func__, __LINE__, ##__VA_ARGS__); } }

#ifdef OUTPUT_FW_LOG_TO_FILE
void open_fw_log_file(void);
void close_fw_log_file(void);
void ISP_PR_INFO_FW(const char *fmt, ...);
#else
#define ISP_PR_INFO_FW(format, ...) { if (ISP_DEBUG_LEVEL >= \
	TRACE_LEVEL_ERROR) { pr_info("[Cam][FW]:" \
	format,  ##__VA_ARGS__); } }
#endif


#define assert(x)		\
	do {			\
		if (!(x))	\
			ISP_PR_ERR("!!!ASSERT ERROR: " #x " !!!"); \
	} while (0)
/*
 *#define return_if_assert_fail(x)	\
 *	do {				\
 *		if (!(x))		\
 *			ISP_PR_ERR("!!!ASSERT ERROR:" #x " !!!"); \
 *		return;			\
 *	} while (0)
 *
 *#define return_val_if_assert_fail(x, val)	\
 *	do {					\
 *		if (!(x))			\
 *			ISP_PR_ERR("!!!ASSERT ERROR:" #x " !!!"); \
 *		return val;			\
 *	} while (0)
 */

	char *get_host_2_fw_cmd_str(unsigned int cmd_id);
	char *isp_dbg_get_isp_status_str(unsigned int status);
	void dbg_show_sensor_prop(void *sensor_cfg);
	void dbg_show_sensor_caps(void *sensor_cap);
	void isp_dbg_show_map_info(void *p);
	void isp_dbg_show_bufmeta_info(char *pre,
				unsigned int cid,
				void *p,
				void *origBuf
				/*struct sys_img_buf_handle**/);
	char *isp_dbg_get_img_fmt_str(
			void *in /*enum _ImageFormat_t **/);
	char *isp_dbg_get_pvt_fmt_str(
			int fmt /*enum pvt_img_fmt */);
	char *isp_dbg_get_out_ch_str(
			int ch /*enum _IspPipeOutCh_t*/);
	char *isp_dbg_get_out_fmt_str(
			int fmt /*enum enum _ImageFormat_t */);
	char *isp_dbg_get_cmd_str(unsigned int cmd);
	char *isp_dbg_get_buf_type(
			unsigned int type);	/*enum _BufferType_t */
	char *isp_dbg_get_resp_str(unsigned int resp);
	char *isp_dbg_get_buf_src_str(unsigned int src);
	char *isp_dbg_get_buf_done_str(unsigned int status);
	char *isp_dbg_get_iso_str(unsigned int iso);
	char *isp_dbg_get_af_mode_str(
			unsigned int mode /*enum _AfMode_t */);
	char *isp_dbg_get_ae_mode_str(
			unsigned int mode /*enum _AeMode_t */);
	char *isp_dbg_get_awb_mode_str(
			unsigned int mode /*enum _AwbMode_t */);
	void isp_dbg_show_img_prop(char *pre,
			void *p /*struct _ImageProp_t * */);
	void dbg_show_drv_settings(void *setting);
	char *isp_dbg_get_scene_mode_str(unsigned int mode);
	char *isp_dbg_get_stream_str(unsigned int stream);
	char *isp_dbg_get_para_str(
			unsigned int para /*enum para_id */);
	char *isp_dbg_get_iso_str(
			unsigned int para /*struct _AeIso_t */);
	char *isp_dbg_get_flash_mode_str(
			unsigned int mode /*FlashMode_t */);
	char *isp_dbg_get_flash_assist_mode_str(
		unsigned int mode/*FocusAssistMode_t */);
	char *isp_dbg_get_red_eye_mode_str(unsigned int mode);
	char *isp_dbg_get_anti_flick_str(unsigned int type);
	void isp_dbg_show_frame3_info(
			void *p /*Mode3FrameInfo_t * */,
			char *prefix);
	void isp_dbg_show_af_ctrl_info(
			void *p /*struct _AfCtrlInfo_t * */);
	void isp_dbg_show_ae_ctrl_info(
			void *p /*struct _AeCtrlInfo_t * */);
	char *isp_dbg_get_focus_state_str(unsigned int state);
	char *isp_dbg_get_reg_name(unsigned int reg);
	char *isp_dbg_get_work_buf_str(unsigned int type);
	void isp_dbg_dump_ae_status(
		void *p /*struct _AeStatus_t * */);
	char *isp_dbg_get_mipi_out_path_str(
		int test/*enum _MipiPathOutType_t*/);

#if defined(__cplusplus)
};
#endif

#endif /*__LOG_H__*/
