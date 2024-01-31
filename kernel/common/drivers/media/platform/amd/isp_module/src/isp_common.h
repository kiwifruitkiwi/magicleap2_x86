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
 *************************************************************************
 */

#ifndef ISP_COMMON_H
#define ISP_COMMON_H

#include "os_base_type.h"
#include "os_advance_type.h"
#include "isp_fw_if/isp_hw_reg.h"
#include "isp_module_if.h"
#include "sensor_wrapper.h"
#include "isp_fw_if/drv_isp_if.h"
#include "isp_gfx_if.h"
#include "isp_queue.h"
#include "isp_pwr.h"
#include "isp_hw.h"
#include "isp_work_item.h"
#include "isp_para_capability.h"
#include "tuning_data_header.h"

#define DO_SYNCHRONIZED_STOP_STREAM


#define MAX_HOST2FW_SEQ_NUM		(16 * 1024)

/*current command version is 1.0 */
#define HOST2FW_CMD_VERSION		0x00010000

//#define MAX_ISP_FW_SIZE		(2 * 1024 * 1024) /* 2M*/
#define HOST2FW_COMMAND_SIZE		(sizeof(Cmd_t))
#define FW2HOST_RESPONSE_SIZE		(sizeof(Resp_t))
//todo  give right definition
//#define MAX_HOST2FW_COMMAND_PACKAGE_SIZE 100
//#define MAX_HOST2FW_COMMAND_PACKAGE_SIZE
//	(MAX(MAX(sizeof(isp_create_params_t), sizeof(isp_config_t)),
//	sizeof(isp_image_buffer_param_t)))
#define MAX_CALIBDATA_BUFFER_SIZE	(2 * 1024 * 1024)

/*total 2MB for firmware running */
#define ISP_FW_WORK_BUF_SIZE		(2 * 1024 * 1024)
#define MAX_NUM_HOST2FW_COMMAND		(20)
#define MAX_NUM_FW2HOST_RESPONSE	(30)

#define RGBIR_FRAME_INFO_FB_SIZE	(1 * 1024 * 1024)


//#define MAX_NUM_PACKAGE		(MAX_NUM_FW2HOST_RESPONSE + 1)

#define ISP_FW_CODE_BUF_SIZE		(640 * 1024)
#define ISP_FW_STACK_BUF_SIZE		(64 * 1024)
#define ISP_FW_HEAP_BUF_SIZE		(896 * 1024)
#define ISP_FW_TRACE_BUF_SIZE	(64 * 1024)
#define ISP_FW_CMD_BUF_SIZE (MAX_NUM_HOST2FW_COMMAND * HOST2FW_COMMAND_SIZE)
#define ISP_FW_CMD_BUF_COUNT 4
//#define ISP_FW_HIGHT_CMD_BUF_SIZE (4 * 1024)
#define ISP_FW_RESP_BUF_SIZE (MAX_NUM_FW2HOST_RESPONSE * FW2HOST_RESPONSE_SIZE)
#define ISP_FW_RESP_BUF_COUNT 4

#define ISP_FW_CMD_PAY_LOAD_BUF_SIZE \
		(ISP_FW_WORK_BUF_SIZE - (ISP_FW_CODE_BUF_SIZE +\
		ISP_FW_STACK_BUF_SIZE + ISP_FW_HEAP_BUF_SIZE +\
		ISP_FW_TRACE_BUF_SIZE +\
		ISP_FW_CMD_BUF_SIZE * ISP_FW_CMD_BUF_COUNT +\
		ISP_FW_RESP_BUF_SIZE * ISP_FW_RESP_BUF_COUNT))

#define ISP_FW_CMD_PAY_LOAD_BUF_ALIGN 64
#define BUFFER_ALIGN_SIZE (0x400)
#define STREAM_TMP_BUF_COUNT 3
#define STREAM_META_BUF_COUNT 3
#define EMB_DATA_BUF_NUM 4
//#define MAX_AF_ROI_NUM 3
#define MAX_REAL_FW_RESP_STREAM_NUM 4
#define ISP_IRQ_SOURCE 1
#define CLK0_CLK1_DFS_CNTL_n0 0x0005B12C
#define CLK0_CLK3_DFS_CNTL_n1 0x0005B97C

#define FRONT_TEMP_RAW_BUF_SIZE (4148 * 1024)
#define FB_SIZE_NEEDED_FOR_13M_TNR (130 * 1024 * 1024)
#define REF_BUF_SIZE_FOR_13M_TNR (30 * 1024 * 1024)
#ifdef FRONT_CAMERA_SUPPORT_ONLY
#define ISP_PREALLOC_FB_SIZE (20 * 1024 * 1024)//(100 * 1024 * 1024)
#define REAR_TEMP_RAW_BUF_SIZE 0
#else
#define ISP_PREALLOC_FB_SIZE (100 * 1024 * 1024)	//(100 * 1024 * 1024)
#define REAR_TEMP_RAW_BUF_SIZE (26 * 1024 * 1024)
#endif

#define CALIB_DATA_SIZE (66 * 1024)

#define INDIRECT_BUF_SIZE (10 * 1024)
#define INDIRECT_BUF_CNT 10

#define HDR_STATS_BUF_SIZE (4 * 1024)
#define IIC_CONTROL_BUF_SIZE (4 * 1024)
#define EMB_DATA_BUF_SIZE (4 * 1024)
#define CMD_RESPONSE_BUF_SIZE (64 * 1024)

enum fw_cmd_resp_stream_id {
	FW_CMD_RESP_STREAM_ID_GLOBAL = 0,
	FW_CMD_RESP_STREAM_ID_1 = 1,
	FW_CMD_RESP_STREAM_ID_2 = 2,
	FW_CMD_RESP_STREAM_ID_3 = 3,
	FW_CMD_RESP_STREAM_ID_MAX = 4
};

enum fw_cmd_para_type {
	FW_CMD_PARA_TYPE_INDIRECT = 0,
	FW_CMD_PARA_TYPE_DIRECT = 1
};

enum list_type_id {
	LIST_TYPE_ID_FREE = 0,
	LIST_TYPE_ID_IN_FW = 1,
	LIST_TYPE_ID_MAX = 2
};

#include "isp_mc_addr_mgr.h"

#define FOURCC(a, b, c, d) \
	(((uint32)(a)<<0)|((uint32)(b)<<8)|((uint32)(c)<<16)|((uint32)(d)<<24))

#define MAX_PREVIEW_BUFFER_NUM		(4)	/*(6) */
#define MAX_SUBPREVIEW_BUFFER_NUM	(4)
#define MAX_VIDEO_BUFFER_NUM		(4)
#define MAX_RAW_BUFFER_NUM		(25)
#define MAX_JPEG_WIDTH			(1920)
#define MAX_JPEG_HEIGHT			(1080)
#define MAX_JPEG_BUFFER_SIZE	(MAX_JPEG_WIDTH * MAX_JPEG_HEIGHT * 2)
#define MAX_JPEG_BUFFER_NUM		(2)
#define MAX_YUV422_BUFFER_NUM		(2)
#define MAX_META_DATA_BUF_NUM_PER_SENSOR	4
#define MAX_X_Y_ZOOM_MULTIPLE			2

#define NUM_I2C_FPGA		(5) // total 5 i2c on FPGA
#define NUM_I2C_ASIC		(2) // total 2 i2c on ASIC

#define RET_SUCCESS		0
#define RET_FAILURE		1 /*!< general failure */
#define RET_NOTSUPP		2 /*!< feature not supported */
#define RET_BUSY		3 /*!< there's already something going on*/
#define RET_CANCELED		4 /*!< operation canceled */
#define RET_OUTOFMEM		5 /*!< out of memory */
#define RET_OUTOFRANGE		6 /*!< parameter/value out of range */
#define RET_IDLE		7 /*!< feature/subsystem is in idle state */
#define RET_WRONG_HANDLE	8 /*!< handle is wrong */
/*!< the/one/all parameter(s) is a(are) NULL pointer(s) */
#define RET_NULL_POINTER	9
#define RET_NOTAVAILABLE	10 /*!< profile not available */
#define RET_DIVISION_BY_ZERO	11 /*!< a divisor equals ZERO */
#define RET_WRONG_STATE		12 /*!< state machine in wrong state */
#define RET_INVALID_PARM		13 /*!< invalid parameter */
#define RET_PENDING		14 /*!< command pending */
#define RET_WRONG_CONFIG	15 /*!< givin configuration is invalid */
#define RET_TIMEOUT		16 /*!< time out */
#define RET_INVALID_PARAM	17

#define MAX_MODE_TYPE_STR_LEN	16

#define MAX_SLEEP_COUNT		(10)
#define MAX_SLEEP_TIME		(100)

#define ISP_ADDR_ALIGN_UP(addr, align) \
	((((uint64)(addr)) + (((uint64)(align))-1)) & ~(((uint64)(align))-1))
#define ISP_ALIGN_SIZE_1K	(0x400)
#define ISP_ADDR_ALIGN_UP_1K(addr) \
	(ISP_ADDR_ALIGN_UP(addr, ISP_ALIGN_SIZE_1K))
#define GEN_64BIT_ADDR(ADDRHI, ADDRLO) \
	((((uint64)(ADDRHI)) << 32) | ((uint64)(ADDRLO)))

#define SKIP_FRAME_COUNT_AT_START	0

#define FPGA_PHOTE_SEQUENCE_DEPTH	2
#define MAX_CALIB_ITEM_COUNT		MAX_PROFILE_NO

#define ISP_MC_ADDR_ALIGN		(1024 * 32)
#define ISP_MC_PREFETCH_GAP		(1024 * 32)

#define BUF_NUM_BEFORE_START_CMD 2

#ifndef true
#define true (1 == 1)
#endif

#ifndef false
#define false (!true)
#endif

#ifndef MIN
#define MIN(a, b)	(((a) < (b)) ? (a) : (b))
#endif
#ifndef MAX
#define MAX(a, b)	(((a) > (b)) ? (a) : (b))
#endif

enum isp_gpu_mem_type {
	/*ISP_MEM_SRC_SYS, */
	ISP_GPU_MEM_TYPE_LFB,
	ISP_GPU_MEM_TYPE_NLFB
};

#define ISP_GET_STATUS(isp) isp->isp_status
#define ISP_SET_STATUS(isp, state) \
{ \
isp->isp_status = state; \
if (state == ISP_STATUS_FW_RUNNING) \
	isp_get_cur_time_tick(&isp->isp_pu_isp.idle_start_time); \
else \
	isp->isp_pu_isp.idle_start_time = MAX_ISP_TIME_TICK; \
}

enum isp_status {
	ISP_STATUS_UNINITED,
	ISP_STATUS_INITED,
	ISP_STATUS_PWR_OFF = ISP_STATUS_INITED,
	ISP_STATUS_PWR_ON,
	ISP_STATUS_FW_RUNNING,
	ISP_FSM_STATUS_MAX
};

enum isp_af_type {
	ISP_AF_TYPE_INVALID,
	ISP_AF_TYPE_CONTINUOUS,
	ISP_AF_TYPE_ONE_SHOT
};

enum start_status {
	START_STATUS_NOT_START,
	START_STATUS_STARTING,
	START_STATUS_STARTED,
	START_STATUS_START_FAIL,
	START_STATUS_START_STOPPING
};

#define STREAM__PREVIEW_OUTPUT_BIT		(1<<STREAM_ID_PREVIEW)
#define STREAM__VIDEO_OUTPUT_BIT		(1<<STREAM_ID_VIDEO)
#define STREAM__ZSL_OUTPUT_BIT			(1<<STREAM_ID_ZSL)
#define STREAM__RGBIR_OUTPUT_BIT		(1<<STREAM_ID_RGBIR)
#define STREAM__TAKE_ONE_YUV_OUTPUT_BIT		(1<<(STREAM_ID_ZSL+1))
#define STREAM__TAKE_ONE_RAW_OUTPUT_BIT		(1<<(STREAM_ID_ZSL+2))

typedef void *isp_mc_adr_mgr_handle;

struct isp_mc_addr_node {
	uint64 start_addr;
	uint64 align_addr;
	uint64 end_addr;
	uint32 size;
	struct isp_mc_addr_node *next;
	struct isp_mc_addr_node *prev;
};

struct isp_mc_addr_mgr {
	struct isp_mc_addr_node head;
	isp_mutex_t mutext;
	uint64 start;
	uint64 len;
};

struct sys_to_mc_map_info {
	uint64 sys_addr;
	uint64 mc_addr;
	uint32 len;
};

struct isp_mapped_buf_info {
	struct list_node node;
	uint8 camera_id;
	uint8 str_id;
	/*uint8 map_count;
	 *enum isp_buffer_status buf_status;
	 */
	sys_img_buf_handle_t sys_img_buf_hdl;
	uint64 multi_map_start_mc;
	struct sys_to_mc_map_info y_map_info;
	struct sys_to_mc_map_info u_map_info;
	struct sys_to_mc_map_info v_map_info;
#ifdef USING_KGD_CGS
	cgs_handle_t cgs_hdl;
#else
	void *map_hdl;
	void *cos_mem_handle;
	void *mdl_for_map;
#endif
};

struct isp_general_still_img_buf_info {
	struct list_node node;
	uint16 camera_id;
	uint16 enable_bit;
	struct isp_mapped_buf_info *yuv_in;
	struct isp_mapped_buf_info *main_jepg;
	struct isp_mapped_buf_info *embed_thumb_jepg;
	struct isp_mapped_buf_info *standalone_thumb_jepg;
	struct isp_mapped_buf_info *yuv_out;
};

struct isp_meta_data_info {
	struct list_node node;
	MetaInfo_t meta_data;
	struct sys_to_mc_map_info map_info;
};

struct isp_stream_info {
	enum pvt_img_fmt format;
	uint32 width;
	uint32 height;
	uint32 fps;
	uint32 luma_pitch_set;
	uint32 chroma_pitch_set;
	uint32 max_fps_numerator;
	uint32 max_fps_denominator;
	struct isp_list buf_free;
	struct isp_list buf_in_fw;
	struct isp_gpu_mem_info *uv_tmp_buf;
	/*
	 *struct isp_list linear_tmpbuf_free;
	 *struct isp_list linear_tmpbuf_in_fw;
	 */
	enum start_status start_status;
	uint8 running;
	uint8 buf_num_sent;
	uint8 tnr_on;
	uint8 vs_on;
	bool is_perframe_ctl;
	/*
	 *struct isp_mapped_buf_info *tnr_ref_buf;
	 */
};

struct zoom_information {
	uint32 ratio;
	int16 v_offset;
	int16 h_offset;

	window_t window;
};

struct fps_table {
	uint16 HighAddr;
	uint16 LowAddr;
	uint16 HighValue;
	uint16 LowValue;
	float fps;
};

struct isp_cos_sys_mem_info {
	uint64 mem_size;
	void *sys_addr;
	void *mem_handle;
};

struct sensor_info {
	enum camera_id cid;
	struct calib_date_item *cur_calib_item;
	struct isp_gpu_mem_info *calib_gpu_mem;
	struct isp_gpu_mem_info *stream_tmp_buf[STREAM_TMP_BUF_COUNT];
	struct isp_gpu_mem_info *cmd_resp_buf;
	struct isp_gpu_mem_info *loopback_raw_buf;
	struct isp_gpu_mem_info *loopback_raw_info;
	uint64 meta_mc[STREAM_META_BUF_COUNT];
	struct isp_gpu_mem_info *tnr_tmp_buf;
	struct isp_gpu_mem_info *hdr_stats_buf;
	struct isp_gpu_mem_info *iic_tmp_buf;
	struct isp_gpu_mem_info *emb_data_buf[EMB_DATA_BUF_NUM];
	struct isp_gpu_mem_info *stage2_tmp_buf;
	struct isp_list take_one_pic_buf_list;
	struct isp_list rgbir_raw_output_in_fw;
	struct isp_list rgbraw_input_in_fw;
	struct isp_list irraw_input_in_fw;
	struct isp_list rgbir_frameinfo_input_in_fw;
	uint32 take_one_pic_left_cnt;
	struct isp_list take_one_raw_buf_list;
	uint32 take_one_raw_left_cnt;
	isp_sensor_prop_t sensor_cfg;
	enum start_status status;
	AeMode_t aec_mode;
	AfMode_t af_mode;
	FocusAssistMode_t flash_assist_mode;
	uint32 flash_assist_pwr;
	LensRange_t lens_range;
	struct fps_table fps_tab;
	AwbMode_t awb_mode;
	Window_t ae_roi;
	Window_t af_roi[MAX_AF_ROI_NUM];
	Window_t awb_region;
	FlashMode_t flash_mode;
	uint32 flash_pwr;
	RedEyeMode_t redeye_mode;
	uint32 color_temperature;	/*in K */
/*previous*/
	uint32 raw_width;
	uint32 raw_height;
	Window_t asprw_wind;
	struct sensor_res_fps_t res_fps;
	struct isp_stream_info str_info[STREAM_ID_NUM + 1];
	/*int32 cproc_enable; */
	int32 para_brightness_cur;
	int32 para_brightness_set;
	int32 para_contrast_cur;
	int32 para_contrast_set;
	int32 para_hue_cur;
	int32 para_hue_set;
	int32 para_satuaration_cur;
	int32 para_satuaration_set;
	AeFlickerType_t flick_type_cur;
	AeFlickerType_t flick_type_set;
	AeIso_t para_iso_cur;
	AeIso_t para_iso_set;
	uint32 para_color_temperature_cur;
	uint32 para_color_temperature_set;

	int32 para_color_enable_cur;
	int32 para_color_enable_set;
	AeEv_t para_ev_compensate_cur;
	AeEv_t para_ev_compensate_set;

	int32 para_gamma_cur;
	int32 para_gamma_set;
	enum pvt_scene_mode para_scene_mode_cur;
	enum pvt_scene_mode para_scene_mode_set;

	/*struct isp_still_img_info still_info; */
	MetaInfo_t meta_data;
	struct isp_list meta_data_free;
	struct isp_list meta_data_in_fw;
	/*
	 *isp_mutex_t snr_mutex;
	 *isp_mutex_t stream_op_mutex;
	 *
	 *struct isp_mapped_buf_info *script_bin_buf;
	 */
	CmdSetDeviceScript_t script_cmd;
	uint32 resume_cmd;
	struct isp_event resume_success_evt;
	float sharpness[3];
	struct zoom_information zoom_info;
	uint32 zsl_ret_width;
	uint32 zsl_ret_height;
	uint32 zsl_ret_stride;
	uint32 open_flag;
	enum camera_type cam_type;
	enum fw_cmd_resp_stream_id stream_id;
	enum isp_scene_mode scene_mode;
	RawPktFmt_t raw_pkt_fmt;
	uint8 zsl_enable;
	uint8 resend_zsl_enable;
	char cur_res_fps_id;
	char stream_inited;
	char hdr_enable;
	char hdr_prop_set_suc;
	char hdr_enabled_suc;
	char calib_enable;
	char calib_set_suc;
	char tnr_tmp_buf_set;
	char tnr_enable;
	char start_str_cmd_sent;
	//cproc_header *cproc_value;
	wb_light_source_index *wb_idx;
	scene_mode_table_header *scene_tbl;
	bool enable_dynamic_frame_rate;

	// Face Authtication Mode
	bool_t face_auth_mode;
};

#define I2C_REGADDR_NULL 0xffff

struct isp_cmd_element {
	uint32 seq_num;
	uint32 cmd_id;
	enum fw_cmd_resp_stream_id stream;
	uint64 mc_addr;
	isp_time_tick_t send_time;
	struct isp_event *evt;
	struct isp_gpu_mem_info *gpu_pkg;
	void *resp_payload;
	uint32 *resp_payload_len;
	uint16 I2CRegAddr;
	enum camera_id cam_id;
	struct isp_cmd_element *next;
};

enum isp_pipe_used_status {
	ISP_PIPE_STATUS_USED_BY_NONE,
	/*used by rear camera */
	ISP_PIPE_STATUS_USED_BY_CAM_R = (CAMERA_ID_REAR + 1),
	/*used by front left camera */
	ISP_PIPE_STATUS_USED_BY_CAM_FL = (CAMERA_ID_FRONT_LEFT + 1),
	/*used by front right camera */
	ISP_PIPE_STATUS_USED_BY_CAM_FR = (CAMERA_ID_FRONT_RIGHT + 1)
};

enum isp_config_mode {
	ISP_CONFIG_MODE_INVALID,
	ISP_CONFIG_MODE_PREVIEW,
	ISP_CONFIG_MODE_RAW,
	ISP_CONFIG_MODE_VIDEO2D,
	ISP_CONFIG_MODE_VIDEO3D,
	ISP_CONFIG_MODE_VIDEOSIMU,
	ISP_CONFIG_MODE_DATAT_TRANSFER,
	ISP_CONFIG_MODE_MAX
};

struct isp_picture_buffer_param {
	enum pvt_img_fmt color_format;

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
};

#define MAX_CAMERA_DEVICE_TYPE_NAME_LEN 10

struct dphy_config_para {
	uint32 value;		/*to be added later */
};

struct acpi_isp_info {
	char rear_cam_type[MAX_CAMERA_DEVICE_TYPE_NAME_LEN];
	char frontl_cam_type[MAX_CAMERA_DEVICE_TYPE_NAME_LEN];
	char frontr_cam_type[MAX_CAMERA_DEVICE_TYPE_NAME_LEN];

	char rear_vcm_type[MAX_CAMERA_DEVICE_TYPE_NAME_LEN];
	char frontl_vcm_type[MAX_CAMERA_DEVICE_TYPE_NAME_LEN];
	char frontr_vcm_type[MAX_CAMERA_DEVICE_TYPE_NAME_LEN];

	char rear_flash_type[MAX_CAMERA_DEVICE_TYPE_NAME_LEN];
	char frontl_flash_type[MAX_CAMERA_DEVICE_TYPE_NAME_LEN];
	char frontr_flash_type[MAX_CAMERA_DEVICE_TYPE_NAME_LEN];

	uint8 isp_present;
	uint8 pack;		/*to make it aligh 64 */
	uint8 isp_mem_size_b0;	/*the 0 byte of the value */
	uint8 isp_mem_size_b1;	/*the 0 byte of the value */
	uint8 isp_mem_size_b2;	/*the 0 byte of the value */
	uint8 isp_mem_size_b3;	/*the 0 byte of the value */

	struct dphy_config_para dphy_para;
};

enum isp_pipe_mode {
	ISP_PIPE_MODE_INVALID,
	ISP_PIPE_MODE_DATA_TRANSFER,
	ISP_PIPE_MODE_RAW_CAPTURE,
	ISP_PIPE_MODE_MULTI_STREAM,
	ISP_PIPE_MODE_3D_VIDEO_CAPTURE,
	ISP_PIPE_MODE_LOOPBACK_PREVIEW,
	ISP_PIPE_MODE_MAX
};

enum isp_bayer_pattern {
	ISP_BAYER_PATTERN_INVALID,
	ISP_BAYER_PATTERN_RGRGGBGB,
	ISP_BAYER_PATTERN_GRGRBGBG,
	ISP_BAYER_PATTERN_GBGBRGRG,
	ISP_BAYER_PATTERN_BGBGGRGR,
	ISP_BAYER_PATTERN_MAX
};

struct isp_config_mode_param {
	enum isp_config_mode mode;
	bool_t disable_calib;
	union mode_param_t {
		struct _loopback_preview_param_t_unused {
			bool_t en_continue;
			enum isp_bayer_pattern bayer_pattern;
			char *raw_file;
		} loopback_preview;
	} mode_param;
};

struct isp_config_preview_param {
	uint32 preview_width;
	uint32 preview_height;
	uint32 preview_luma_picth;
	uint32 preview_chroma_pitch;
	bool_t disable_calib;
};

struct photo_seque_info {
	uint16 max_fps;
	uint16 cur_fps;
	uint16 max_history_frame_cnt;
	uint16 cur_history_frame_cnt;
	uint32 task_bits_by_buf;
	uint32 task_bits_by_cmd;
	uint32 buf_sent;
	enum start_status stream_status;
	bool_t running;
};

struct isp_gpu_mem_info {
	enum isp_gpu_mem_type mem_src;
	cgs_handle_t handle;
	uint64 mem_size;
	uint64 gpu_mc_addr;
	void *sys_addr;
	void *mem_handle;
	void *acc_handle;
};

struct isp_mode_cfg_para {
	enum isp_pipe_mode mode;
	bool_t disable_calib;
	union _mode_param_t {
		struct _loopback_preview_param_t {
			bool_t en_continue;
			enum isp_bayer_pattern bayer_pattern;
			char *raw_file;
		} loopback_preview;

		struct _multistream_param_t {
			bool_t en_preview;
			bool_t en_video;
			bool_t en_zsl;
		} multistream;
	} mode_param;
};

struct isp_pipe_info {
	enum isp_pipe_used_status pipe_used_status;
	/*struct isp_mode_cfg_para next_mode_param; */
	/*struct isp_mode_cfg_para mode_param; */
	enum isp_pipe_mode mode;
	/*uint64 loopback_raw_base; */
	/*uint32 loopback_raw_size; */
};
struct calib_date_item {
	uint32 width;
	uint32 height;
	uint32 fps;
	uint32 hdr;
	void *data;
	uint32 len;
	struct calib_date_item *next;
};

enum isp_work_buf_type {
	ISP_WORK_BUF_TYPE_CALIB,
	ISP_WORK_BUF_TYPE_STREAM_TMP,
	ISP_WORK_BUF_TYPE_CMD_RESP,
	ISP_WORK_BUF_TYPE_LB_RAW_DATA,
	ISP_WORK_BUF_TYPE_LB_RAW_INFO,
	ISP_WORK_BUF_TYPE_TNR_TMP,
	ISP_WORK_BUF_TYPE_HDR_STATUS,
	ISP_WORK_BUF_TYPE_IIC_TMP,	//IIC_CONTROL_BUF_SIZE
	ISP_WORK_BUF_TYPE_STAGE2_TMP,
	ISP_WORK_BUF_TYPE_UV_TMP,
	ISP_WORK_BUF_TYPE_INDIR_FW_CMD_PKG,
	ISP_WORK_BUF_TYPE_EMB_DATA
};

struct isp_sensor_work_buf {
	struct isp_gpu_mem_info *calib_buf;
	struct isp_gpu_mem_info *stream_tmp_buf[STREAM_TMP_BUF_COUNT];
	struct isp_gpu_mem_info *cmd_resp_buf;
	struct isp_gpu_mem_info *loopback_raw_buf;
	struct isp_gpu_mem_info *loopback_raw_info;
	struct isp_gpu_mem_info *tnr_tmp_buf;
	struct isp_gpu_mem_info *hdr_stats_buf;
	struct isp_gpu_mem_info *iic_tmp_buf;
	struct isp_gpu_mem_info *stage2_tmp_buf;
	struct isp_gpu_mem_info *indir_fw_cmd_pkg[INDIRECT_BUF_CNT];
	struct isp_gpu_mem_info *emb_data_buf[EMB_DATA_BUF_NUM];
};

struct isp_work_buf {
	struct isp_sensor_work_buf sensor_work_buf[CAMERA_ID_MAX];
	struct isp_gpu_mem_info *uv_tmp_buf;
};

struct psp_fw_header {
	uint32 image_id[4];
	uint32 header_ver;
	uint32 fw_size_in_sig;
	uint32 encrypt_opt;
	uint32 encrypt_id;
	uint32 encrypt_param[4];
	uint32 sig_opt;
	uint32 sig_id;
	uint32 sig_param[4];
	uint32 compress_opt;
	uint32 compress_id;
	uint32 uncompress_size;
	uint32 compress_size;
	uint32 compress_param[2];
	uint32 fw_version;
	uint32 family_id;
	uint32 load_addr;
	uint32 signed_img_size;
	uint32 unsigned_size;
	uint32 offset_nwd;
	uint32 fw_version_sign_opt;
	uint32 reserved;
	uint32 encrypt_key[4];
	uint32 tool_info[4];
	uint32 fw_data[4];
	uint32 reserved_zero[4];
};

struct isp_context {
	void *sw_isp_context;
	enum isp_status isp_status;
	isp_mutex_t ops_mutex;

	uint64 fw_mc_loaded_by_psp;
	/*enum isp_pwr_unit_status1 isp_pwr_status; */
	/*enum isp_dphy_pwr_statusdfsdfs1 dphy_pwr_status; */
	/*enum sdf a; */
	/*struct isp_spin_lock isp_pwr_spin_lock; */
	/*struct isp_event isp_pwr_on_evt; */
	/*struct isp_event isp_pwr_off_evt; */

	struct isp_pwr_unit isp_pu_isp;
	struct isp_pwr_unit isp_pu_dphy;
	struct isp_pwr_unit isp_pu_cam[CAMERA_ID_MAX];
	uint32 isp_fw_ver;
	bool_t isp_flash_torch_on;

	struct isp_capabilities isp_cap;
	struct isp_config_mode_param mode_param;
	struct isp_gpu_mem_info fw_work_buf;
	isp_fw_work_buf_handle fw_work_buf_hdl;
	struct isp_gpu_mem_info fb_buf;

	isp_fw_work_buf_handle fb_hdl;

	uint64 fw_cmd_buf_sys[ISP_FW_CMD_BUF_COUNT];
	uint64 fw_cmd_buf_mc[ISP_FW_CMD_BUF_COUNT];
	uint32 fw_cmd_buf_size[ISP_FW_CMD_BUF_COUNT];
	uint64 fw_resp_buf_sys[ISP_FW_RESP_BUF_COUNT];
	uint64 fw_resp_buf_mc[ISP_FW_RESP_BUF_COUNT];
	uint32 fw_resp_buf_size[ISP_FW_RESP_BUF_COUNT];
	uint64 fw_log_sys;
	uint64 fw_log_mc;
	uint32 fw_log_size;

	struct isp_cmd_element *cmd_q;
	isp_mutex_t cmd_q_mtx;

	uint32 sensor_count;
	struct camera_dev_info cam_dev_info[CAMERA_ID_MAX];
	struct thread_handler fw_resp_thread[MAX_REAL_FW_RESP_STREAM_NUM];
	uint64 irq_enable_id[MAX_REAL_FW_RESP_STREAM_NUM];
	/*struct thread_handler poll_thread_hdl; */
	/*struct thread_handler idle_detect_thread; */
	struct thread_handler work_item_thread;
	//struct isp_stream_info stream_info[CAMERA_ID_MAX][STREAM_ID_NUM + 1];

	isp_mutex_t take_pic_mutex;
	struct isp_list take_photo_seq_buf_list;
	struct isp_list work_item_list;
	struct photo_seque_info photo_seq_info[CAMERA_ID_MAX];
	enum camera_id cur_photo__seq_cam_id;
	/*sensor_operation_t sensor_ops[CAMERA_ID_MAX]; */
	struct sensor_operation_t *p_sensor_ops[CAMERA_ID_MAX];
	struct vcm_operation_t *p_vcm_ops[CAMERA_ID_MAX];
	struct flash_operation_t *p_flash_ops[CAMERA_ID_MAX];
	struct fw_parser_operation_t *fw_parser[CAMERA_ID_MAX];
	isp_mutex_t command_mutex;	/*mutext to command */
	isp_mutex_t response_mutex;	/*mutext to retrieve response */

	uint32 host2fw_seq_num;

	uint32 reg_value;
	uint32 fw2host_response_result;
	uint32 fw2host_sync_response_payload[40];

	/*struct isp_ring_log_buf_context ring_log_buf_info; */

	func_isp_module_cb evt_cb[CAMERA_ID_MAX];
	pvoid evt_cb_context[CAMERA_ID_MAX];
	void *fw_data;
	uint32 fw_len;
	void *fw_mem_hdl;
	struct calib_date_item
	*calib_items[CAMERA_ID_MAX][MAX_CALIB_ITEM_COUNT];
	void *calib_data[CAMERA_ID_MAX];
	uint32 calib_data_len[CAMERA_ID_MAX];
	uint32 sclk;		//In MHZ
	uint32 iclk;		//In MHZ
	uint32 refclk;		//In MHZ
	struct driver_settings drv_settings;
	bool snr_res_fps_info_got[CAMERA_ID_MAX];
	bool fw_ctrl_3a;
	bool flashlight_cfg_to_be_set;
	bool clk_info_set_2_fw;
	bool req_fw_load_suc;
	isp_mutex_t map_unmap_mutex;
	struct isp_work_buf work_buf;
	struct hwip_discovery_info hw_ip_info;
	struct isp_cos_sys_mem_info prefetch_buf;
	struct sensor_info sensor_info[CAMERA_ID_MAX];
};

struct isp_fw_resp_thread_para {
	uint32 idx;
	struct isp_context *isp;
};

enum isp_environment_setting {
	ISP_ENV_INVALID = -1,
	ISP_ENV_SILICON = 0,
	ISP_ENV_ALTERA = 1,
	ISP_ENV_MAXIMUS = 2,
	ISP_ENV_MAX = 3
};

	extern enum isp_environment_setting g_isp_env_setting;

	extern struct isp_context g_isp_context;
	extern struct acpi_isp_info g_acpi_isp_info;
	extern uint32 g_num_i2c;


	extern struct cgs_device *g_cgs_srv;
	extern enum irq_source_isp g_irq_src[];

#ifdef USING_WIRTECOMBINE_BUFFER_FOR_STAGE2
#define DBG_STAGE2_WB_BUF_CNT 4
	extern struct isp_gpu_mem_info *g_dbg_stage2_buf[DBG_STAGE2_WB_BUF_CNT];

	extern long g_dbg_stage2_wb_idx;
#endif

void isp_sw_init_capabilities(struct isp_context *pisp_context);
void isp_reset_str_info(struct isp_context *isp, enum camera_id cid,
			enum stream_id sid, bool pause);
void isp_reset_camera_info(struct isp_context *isp, enum camera_id cid);
void init_all_camera_info(struct isp_context *pisp_context);
void init_all_camera_dev_info(struct isp_context *pisp_context);
void isp_init_drv_setting(struct isp_context *isp);

result_t ispm_sw_init(struct isp_context *isp_context,
		uint32 init_falg, void *gart_range_hdl);
result_t ispm_sw_uninit(struct isp_context *isp_context);
void unit_isp(void);

void isp_idle_detect(struct isp_context *isp_context);

void isp_hw_ccpu_disable(struct isp_context *isp_context);

void isp_hw_ccpu_rst(struct isp_context *isp_context);
void isp_hw_ccpu_stall(struct isp_context *isp_context);
void isp_hw_reset_all(struct isp_context *isp);

void isp_hw_ccpu_enable(struct isp_context *isp_context);
int32 isp_load_fw_fpga(struct isp_context *isp_context, pvoid fw_data,
			uint32 fw_len);

void isp_init_fw_rb_log_buffer(struct isp_context *isp_context,
		uint32 fw_rb_log_base_lo,
		uint32 fw_rb_log_base_hi,
		uint32 fw_rb_log_size);

result_t isp_snr_open(struct isp_context *isp_context,
		enum camera_id camera_id, int32 res_fps_id,
		uint32 flag);

result_t isp_snr_close(struct isp_context *isp, enum camera_id cid);

result_t isp_snr_pwr_toggle(struct isp_context *isp,
			   enum camera_id cid, bool on);
result_t isp_snr_clk_toggle(struct isp_context *isp,
			   enum camera_id cid, bool on);

bool_t isp_get_pic_buf_param(struct isp_stream_info *str_info,
		struct isp_picture_buffer_param *buffer_param);

bool isp_get_str_out_prop(struct isp_stream_info *str_info,
			ImageProp_t *out_prop);

result_t send_prev_buf_and_put_to_inuse_list
				(struct isp_context *isp_context,
				enum camera_id camera_id,
				struct isp_img_buf_info *buffer);

void rst_snr_str_img_buf_type(struct isp_context *isp_context,
			enum camera_id camera_id,
			enum stream_id stream_type);

result_t send_record_buf_and_put_to_inuse_queue
				(struct isp_context *isp_context,
				enum camera_id cam_id,
				struct isp_img_buf_info *buffer);
result_t isp_send_switch_preview_to_video_cmd(struct isp_context
				*isp_context);
result_t fw_send_switch_video2d_to_preview_cmd(struct isp_context
				*isp_context);

result_t isp_set_stream_data_fmt(struct isp_context *isp_context,
				enum camera_id camera_id,
				enum stream_id stream_type,
				enum pvt_img_fmt img_fmt);
result_t isp_set_str_res_fps_pitch(struct isp_context *isp_context,
				enum camera_id camera_id,
				enum stream_id stream_type,
				struct pvt_img_res_fps_pitch *value);

result_t isp_switch_from_preview_to_video2d(struct isp_context
				*isp_context,
				enum camera_id cam_id);

result_t isp_send_stop_record_cmd(struct isp_context *isp_context,
				enum camera_id camera_id);
bool_t is_camera_started(struct isp_context *isp_context,
				enum camera_id cam_id);

result_t isp_take_oneshot_pic(struct isp_context *isp,
				enum camera_id cam_id,
				sys_img_buf_handle_t main_jpg_buf,
				sys_img_buf_handle_t main_yuv_buf,
				sys_img_buf_handle_t jpg_thumb_buf,
				sys_img_buf_handle_t yuv_thumb_buf);
result_t isp_take_oneshot_pic_fpga(struct isp_context *context,
				enum camera_id cam_id,
				sys_img_buf_handle_t main_jpg_buf,
				sys_img_buf_handle_t main_yuv_buf,
				sys_img_buf_handle_t jpg_thumb_buf,
				sys_img_buf_handle_t yuv_thumb_buf);
result_t isp_send_one_photo_seq_buf(struct isp_context *context,
				enum camera_id cam_id,
				sys_img_buf_handle_t main_jpg_buf,
				sys_img_buf_handle_t main_yuv_buf,
				sys_img_buf_handle_t jpg_thumb_buf,
				sys_img_buf_handle_t yuv_thumb_buf);

result_t isp_start_photo_seq(struct isp_context *context,
				enum camera_id cam_id, uint32 fps,
				uint32 task_bits);
result_t isp_stop_photo_seq(struct isp_context *context,
				enum camera_id cam_id);

result_t create_video_queue_and_send_buf
				(struct isp_context *isp_context,
				enum camera_id cam_id);
result_t fw_config_video2d(struct isp_context *isp_context,
				enum camera_id cam_id);
result_t fw_switch_from_preview_to_video
				(struct isp_context *isp_context,
				enum camera_id cam_id);
result_t fw_send_preview_to_video2d_cmd
				(struct isp_context *isp_context);
result_t isp_switch_video2d_to_preview(struct isp_context *isp_context,
				enum camera_id cam_id);
result_t isp_switch_video2d_to_idle(struct isp_context *isp_context,
				enum camera_id cam_id);

void isp_take_back_str_buf(struct isp_context *isp,
			struct isp_stream_info *str,
			enum camera_id cid, enum stream_id sid);
void isp_take_back_metadata_buf(struct isp_context *isp,
				enum camera_id cam_id);
result_t fw_send_stop_stream_cmd(struct isp_context *isp_context,
				enum camera_id cam_id);

struct take_pic_orig_req *isp_create_orig_pic_request
				(struct isp_context *isp,
				enum camera_id cam_id,
				sys_img_buf_handle_t main_jpg_buf,
				sys_img_buf_handle_t main_yuv_buf,
				sys_img_buf_handle_t jpg_thumb_buf,
				sys_img_buf_handle_t yuv_thumb_buf);
void isp_free_orig_pic_request(struct isp_context *isp,
				struct take_pic_orig_req *orig_req);
result_t isp_alloc_sys_mem_hdl(struct isp_context *isp, void *addr,
				uint64 len, void **mem_hdl);
result_t isp_release_sys_mem_hdl(struct isp_context *isp,
				 void *mem_hdl);

struct isp_mapped_buf_info *isp_map_sys_2_mc
				(struct isp_context *isp,
				sys_img_buf_handle_t sys_img_buf_hdl,
				uint32 mc_align, uint16 cam_id,
				uint16 str_id, uint32 y_len,
				uint32 u_len, uint32 v_len);

void isp_unmap_sys_2_mc(struct isp_context *isp,
			struct isp_mapped_buf_info *buff);
struct isp_mapped_buf_info *isp_map_sys_2_mc_single
			(struct isp_context *isp,
			sys_img_buf_handle_t sys_img_buf_hdl,
			uint32 mc_align, uint16 cam_id, uint16 str_id,
			uint32 y_len, uint32 u_len, uint32 v_len);
void isp_unmap_sys_2_mc_single(struct isp_context *isp,
			struct isp_mapped_buf_info *buff);
struct isp_mapped_buf_info *isp_map_sys_2_mc_multi(
				struct isp_context *isp,
				sys_img_buf_handle_t
				sys_img_buf_hdl,
				uint32 mc_align,
				uint16 cam_id,
				uint16 str_id,
				uint32 y_len,
				uint32 u_len,
				uint32 v_len);
void isp_unmap_sys_2_mc_multi(struct isp_context *isp,
				struct isp_mapped_buf_info *buff);

struct isp_general_still_img_buf_info *isp_gen_gerneral_still_img
			(struct isp_context *isp,
			sys_img_buf_handle_t main_jpg_hdl,
			sys_img_buf_handle_t emb_thumb_hdl,
			sys_img_buf_handle_t standalone_thumb_hdl,
			sys_img_buf_handle_t yuv_out_hdl,
			uint32 mc_align, uint16 cam_id);

struct isp_general_still_img_buf_info *isp_gen_gerneral_pp_still_img
			(struct isp_context *isp,
			sys_img_buf_handle_t in_yuy2_buf,
			sys_img_buf_handle_t main_jpg_buf,
			sys_img_buf_handle_t emb_thumb_hdl,
			sys_img_buf_handle_t
			standalone_thumb_hdl,
			uint32 mc_align, uint16 cam_id);

void isp_sys_mc_unmap_and_free(struct isp_context *isp,
			struct isp_mapped_buf_info *info);

result_t isp_do_sys_2_mc_map(struct isp_context *isp, uint64 sys,
			uint64 mc, uint32 len, void **mem_desc);
result_t isp_do_sys_2_mc_unmap(struct isp_context *isp, uint64 sys,
			uint64 mc, uint32 len, void *mem_desc);

result_t isp_sw_unmap_sys_to_mc_soc(struct isp_context *isp,
			struct sys_to_mc_map_info *info);

result_t isp_sw_map_sys_to_mc_fpga(struct isp_context *isp, uint64 sys,
			uint64 mc, uint32 len,
			struct sys_to_mc_map_info *info);
result_t isp_sw_unmap_sys_to_mc_fpga(struct isp_context *isp,
			struct sys_to_mc_map_info *info);

int32 get_free_pipe_for_cam(struct isp_context *isp,
			enum camera_id cam_id);
enum isp_pipe_used_status calc_used_pipe_status_for_cam(enum camera_id
							cam_id);

result_t isp_start_stream_from_idle(struct isp_context *isp_context,
				enum camera_id cam_id,
				int32 pipe_id,
				enum stream_id str_id);
result_t isp_start_stream_from_busy(struct isp_context *isp_context,
				enum camera_id cam_id,
				int32 pipe_id,
				enum stream_id str_id);
result_t isp_stop_video2d_to_video_only(struct isp_context *isp_context,
				enum camera_id cam_id,
				int32 pipe_id);
result_t isp_stop_video2d_to_preview(struct isp_context *isp_context,
				enum camera_id cam_id,
				int32 pipe_id);
result_t isp_start_stream(struct isp_context *isp_context,
			enum camera_id cam_id, enum stream_id str_id,
			bool is_perframe_ctl);
result_t isp_setup_output(struct isp_context *isp, enum camera_id cid,
			enum stream_id str_id);
result_t isp_setup_ae(struct isp_context *isp, enum camera_id cid);
result_t isp_setup_ae_range(struct isp_context *isp,
			enum camera_id cid);

result_t isp_setup_af(struct isp_context *isp, enum camera_id cid);
result_t isp_setup_awb(struct isp_context *isp, enum camera_id cid);
result_t isp_setup_ctls(struct isp_context *isp, enum camera_id cid);

result_t isp_init_stream(struct isp_context *isp,
			 enum camera_id cam_id);
result_t isp_setup_3a(struct isp_context *isp, enum camera_id cam_id);
result_t isp_kickoff_stream(struct isp_context *isp,
			enum camera_id cam_id, uint32 w, uint32 h);

result_t isp_uninit_stream(struct isp_context *isp,
			enum camera_id cam_id);

result_t isp_start_zsl_stream(struct isp_context *isp_context,
			enum camera_id cam_id);
result_t isp_stop_zsl_stream(struct isp_context *isp_context,
			enum camera_id cam_id);

sys_img_buf_handle_t sys_img_buf_handle_cpy(sys_img_buf_handle_t
					hdl_in);
void sys_img_buf_handle_free(sys_img_buf_handle_t hdle);

uint32 isp_calc_total_mc_len(uint32 y_len, uint32 u_len, uint32 v_len,
			uint32 mc_align, uint32 *u_offset,
			uint32 *v_offset);
int32 isp_get_res_fps_id_for_str(struct isp_context *isp,
				enum camera_id cam_id,
				enum stream_id str_id);

result_t isp_snr_meta_data_init(struct isp_context *isp);
result_t isp_snr_meta_data_uninit(struct isp_context *isp);
result_t isp_set_script_to_fw(struct isp_context *isp,
				enum camera_id cid);
void isp_fw_log_print(struct isp_context *isp);
result_t isp_take_oneshot_pic(struct isp_context *isp,
			enum camera_id cam_id,
			sys_img_buf_handle_t main_yuv_buf,
			sys_img_buf_handle_t main_jpg_buf,
			sys_img_buf_handle_t jpg_thumb_buf,
			sys_img_buf_handle_t yuv_thumb_buf);
result_t isp_take_pp_pic(struct isp_context *isp,
			enum camera_id cam_id,
			sys_img_buf_handle_t in_yuy2_buf,
			sys_img_buf_handle_t main_jpg_buf,
			sys_img_buf_handle_t jpg_thumb_buf,
			sys_img_buf_handle_t yuv_thumb_buf);

uint32 isp_get_len_from_zoom(uint32 max_len, uint32 min_len,
			uint32 max_zoom, uint32 min_zoom,
			uint32 cur_zoom);
result_t isp_af_set_mode(struct isp_context *isp,
			enum camera_id cam_id,
			AfMode_t *af_mode);
result_t isp_af_start(struct isp_context *isp, enum camera_id cam_id);

uint32 isp_get_stream_output_bits(struct isp_context *isp,
			enum camera_id, uint32 *stream_cnt);
uint32 isp_get_started_stream_count(struct isp_context *isp);

result_t isp_alloc_tmp_linear_buf(struct isp_context *isp,
			enum camera_id cam_id,
			enum stream_id str_id);
result_t isp_send_tmp_linear_buf(struct isp_context *isp,
			enum camera_id cam_id, int32 pipe_id,
			enum stream_id str_id);
result_t isp_config_vep_for_tiling(struct isp_context *isp,
			enum camera_id cam_id, int32 pipe_id,
			enum stream_id str_id);
result_t isp_config_vep_for_tnr(struct isp_context *isp,
			enum camera_id cam_id,
			enum stream_id str_id);

result_t isp_config_vep_for_vs(struct isp_context *isp,
			enum camera_id cam_id,
			enum stream_id str_id);
void isp_free_tmp_vs_buf(struct isp_context *isp, enum camera_id cam_id,
			 enum stream_id str_id);
result_t isp_alloc_tmp_vs_buf(struct isp_context *isp,
			enum camera_id cam_id,
			enum stream_id str_id);

result_t isp_fw_work_mem_rsv(struct isp_context *isp, uint64 *sys_addr,
			uint64 *mc_addr);
result_t isp_fb_alloc(struct isp_context *isp);
result_t isp_fb_free(struct isp_context *isp);

unsigned char parse_calib_data(struct isp_context *isp,
			enum camera_id cam_id,
			uint8 *data, uint32 len);

void init_calib_items(struct isp_context *isp);
void uninit_calib_items(struct isp_context *isp);

void init_cam_calib_item(struct isp_context *isp, enum camera_id cid);
void uninit_cam_calib_item(struct isp_context *isp,
			  enum camera_id cid);

void add_calib_item(struct isp_context *isp, enum camera_id cam_id,
		   profile_header *profile, void *data,
		   uint32 len);
struct calib_date_item *isp_get_calib_item(struct isp_context *isp,
			enum camera_id cid);

void isp_init_fw_ring_buf(struct isp_context *isp,
			enum fw_cmd_resp_stream_id idx, uint32 cmd);
void isp_get_cmd_buf_regs(enum fw_cmd_resp_stream_id idx,
			uint32 *rreg, uint32 *wreg,
			uint32 *baselo_reg, uint32 *basehi_reg,
			uint32 *size_reg);
void isp_get_resp_buf_regs(enum fw_cmd_resp_stream_id idx,
			uint32 *rreg, uint32 *wreg,
			uint32 *baselo_reg, uint32 *basehi_reg,
			uint32 *size_reg);

void isp_split_addr64(uint64 addr, uint32 *lo, uint32 *hi);

uint64 isp_join_addr64(uint32 lo, uint32 hi);

uint32 isp_get_cmd_pl_size(void);

result_t isp_set_calib_2_fw(struct isp_context *isp,
			enum camera_id cid);

int32 isp_fw_resp_thread_wrapper(pvoid context);

result_t isp_set_snr_info_2_fw(struct isp_context *isp,
			enum camera_id cid);
result_t isp_setup_stream(struct isp_context *isp, enum camera_id cid);
result_t isp_setup_stream_tmp_buf(struct isp_context *isp,
			enum camera_id cid);
result_t isp_setup_metadata_buf(struct isp_context *isp,
				enum camera_id cid);

enum fw_cmd_resp_stream_id isp_get_stream_id_from_cid(
				struct isp_context *isp,
				enum camera_id cid);
enum camera_id isp_get_cid_from_stream_id(struct isp_context *isp,
				enum fw_cmd_resp_stream_id stream_id);
enum camera_id isp_get_rgbir_cid(struct isp_context *isp);

bool_t is_para_legal(pvoid context, enum camera_id cam_id);
result_t isp_start_resp_proc_threads(struct isp_context *isp);
result_t isp_stop_resp_proc_threads(struct isp_context *isp);
void isp_fw_resp_func(struct isp_context *isp,
			enum fw_cmd_resp_stream_id stream_id);
void isp_fw_resp_cmd_done(struct isp_context *isp,
			enum fw_cmd_resp_stream_id stream_id,
			RespCmdDone_t *para);
void isp_fw_resp_cmd_done_extra(struct isp_context *isp,
				enum camera_id cid,
				RespCmdDone_t *para,
				struct isp_cmd_element *ele);
void isp_fw_resp_frame_done(struct isp_context *isp,
			enum fw_cmd_resp_stream_id stream_id,
			RespParamPackage_t *para);
void isp_fw_resp_frame_info(struct isp_context *isp,
			enum fw_cmd_resp_stream_id stream_id,
			RespParamPackage_t *para);
void isp_fw_resp_error(struct isp_context *isp,
			enum fw_cmd_resp_stream_id stream_id,
			RespError_t *para);
void isp_fw_resp_heart_beat(struct isp_context *isp);

int isp_isr(void *private_data, unsigned int src_id,
		const uint32_t *iv_entry);

void isp_clear_cmdq(struct isp_context *isp);
void isp_post_proc_str_buf(struct isp_context *isp,
			struct isp_mapped_buf_info *buf);

void isp_test_reg_rw(struct isp_context *isp);
uint32 isp_cal_buf_len_by_para(enum pvt_img_fmt fmt, uint32 width,
			uint32 height, uint32 l_pitch,
			uint32 c_pitch, uint32 *y_len,
			uint32 *u_len, uint32 *v_len);
ImageFormat_t isp_trans_to_fw_img_fmt(enum pvt_img_fmt format);
enum raw_image_type get_raw_img_type(ImageFormat_t type);
void init_frame_ctl_cmd(struct sensor_info *sif, FrameControl_t *fc,
			struct frame_ctl_info *ctl_info,
			struct isp_mapped_buf_info *zsl_buf);
result_t isp_setup_aspect_ratio_wnd(struct isp_context *isp,
			enum camera_id cid, uint32 w,
			uint32 h);

result_t isp_send_rgbir_raw_info_buf(struct isp_context *isp,
			struct isp_mapped_buf_info *buffer,
			enum camera_id cam_id);
SensorId_t isp_get_fw_sensor_id(struct isp_context *isp,
				enum camera_id cid);
StreamId_t isp_get_fw_stream_from_drv_stream
	(enum fw_cmd_resp_stream_id stream);

bool_t isp_is_mem_camera(struct isp_context *isp, enum camera_id cid);
AeIso_t isp_convert_iso_to_fw(ulong value);

bool isp_is_fpga(void);
result_t isp_register_isr(struct isp_context *isp);
result_t isp_unregister_isr(struct isp_context *isp);
void isp_fb_get_nxt_indirect_buf(struct isp_context *isp,
				 uint64 *sys, uint64 *mc);

long IspMapValueFromRange(long srcLow, long srcHigh,
			long srcValue, long desLow, long desHigh);

uint32 isp_fb_get_emb_data_size(void);

void dump_pin_outputs_to_file(struct isp_context *isp,
				enum camera_id cid,
				struct frame_done_cb_para *cb,
				MetaInfo_t *meta, uint32 dump_flag);

bool roi_window_isvalid(Window_t *wnd);
void isp_init_loopback_camera_info(struct isp_context *isp, enum camera_id cid);
void isp_uninit_loopback_camera_info(struct isp_context *isp,
				enum camera_id cid);

void isp_alloc_loopback_buf(struct isp_context *isp,
				enum camera_id cid, uint32 raw_size);
void isp_free_loopback_buf(struct isp_context *isp, enum camera_id cid);
result_t loopback_set_raw_info_2_fw(struct isp_context *isp,
				enum camera_id cid,
				struct isp_gpu_mem_info *raw,
				struct isp_gpu_mem_info *raw_info);
struct isp_gpu_mem_info *isp_get_work_buf(struct isp_context *isp,
				enum camera_id cid,
				enum isp_work_buf_type buf_type,
				uint32 idx);
void isp_alloc_work_buf(struct isp_context *isp);
void isp_free_work_buf(struct isp_context *isp);
result_t isp_get_rgbir_crop_window(struct isp_context *isp,
				enum camera_id cid, Window_t *crop_window);
result_t isp_read_and_set_fw_info(struct isp_context *isp);
void isp_query_fw_load_status(struct isp_context *isp);

/**codes to support psp fw loading***/
#define FW_IMAGE_ID_0 0x45574F50
#define FW_IMAGE_ID_1 0x20444552
#define FW_IMAGE_ID_2 0x41205942
#define FW_IMAGE_ID_3 0x0A0D444D

#define PSP_HEAD_SIZE 256

struct hwip_to_fw_name_mapping {
	uint32 major;
	uint32 minor;
	uint32 rev;
	const char *fw_name;
};

enum isp_registry_para_idx {
	ENABLE_SWISP_FWLOADING,
	MAX_ISP_REGISTRY_PARA_NUM,
};

struct isp_registry_name_mapping {
	enum isp_registry_para_idx index;
	const char name[256];
	int32_t default_value;
};

extern struct hwip_to_fw_name_mapping isp_hwip_to_fw_name[];
extern struct isp_registry_name_mapping
	g_isp_registry_name_mapping[MAX_ISP_REGISTRY_PARA_NUM];

/*
 * reserve local frame buffer
 *
 * @param size
 * size the length the frame buffer
 *
 * @param sys
 * store the sys address
 *
 * @param mc
 * store the mc address
 */
int32 isp_lfb_resv(pvoid context, uint32 size);

/*
 *free the reserved local frame buffer
 */
int32 isp_lfb_free(pvoid context);

/*
 *Acquire the CPU access to LFB reserved in ispm_sw_init
 */
int32 isp_init_lfb_access(struct isp_context *isp);

/*
 *Release the CPU access to LFB reserved in ispm_sw_uninit
 */
int32 isp_uninit_lfb_access(struct isp_context *isp);

/*
 *Program DS4 bit for ISP in PCTL_MMHUB_DEEPSLEEP
 *Set it to busy (0x0) when FW is running
 *Set it to dile (0x1) in other cases
 */
int32 isp_program_mmhub_ds_reg(bool_t b_busy);

uint8 isp_multiple_map_enabled(struct isp_context *isp);
bool_t isp_is_host_processing_raw(struct isp_context *isp,
					enum camera_id cid);
void isp_pause_stream_if_needed(struct isp_context *isp,
	enum camera_id cid, uint32 w, uint32 h);
void isp_calc_aspect_ratio_wnd(struct isp_context *isp,
				enum camera_id cid, uint32 w, uint32 h,
				Window_t *aprw);
extern uint32 g_rgbir_raw_count_in_fw;
#endif
