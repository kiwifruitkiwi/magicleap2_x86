/*
 * Copyright (C) 2019-2023 Advanced Micro Devices, Inc. All rights reserved.
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
#include "amd_common.h"

#define DO_SYNCHRONIZED_STOP_STREAM


#define MAX_HOST2FW_SEQ_NUM		(16 * 1024)
#define SENSOR_FPS_ACCUACY_FACTOR       1000

/*current command version is 1.0 */
#define HOST2FW_CMD_VERSION		0x00010000

//#define MAX_ISP_FW_SIZE		(2 * 1024 * 1024) /* 2M*/
#define HOST2FW_COMMAND_SIZE		(sizeof(struct _Cmd_t))
#define FW2HOST_RESPONSE_SIZE		(sizeof(struct _Resp_t))
//todo  give right definition
//#define MAX_HOST2FW_COMMAND_PACKAGE_SIZE 100
//#define MAX_HOST2FW_COMMAND_PACKAGE_SIZE
//	(MAX(MAX(sizeof(isp_create_params_t), sizeof(isp_config_t)),
//	sizeof(isp_image_buffer_param_t)))
#define MAX_CALIBDATA_BUFFER_SIZE	(2 * 1024 * 1024)

/*total 2MB for firmware running */
#define ISP_FW_WORK_BUF_SIZE		(11 * 1024 * 1024)
#define MAX_NUM_HOST2FW_COMMAND		(100)
#define MAX_NUM_FW2HOST_RESPONSE	(200)

#define RGBIR_FRAME_INFO_FB_SIZE	(1 * 1024 * 1024)


//#define MAX_NUM_PACKAGE		(MAX_NUM_FW2HOST_RESPONSE + 1)

#define ISP_FW_CODE_BUF_SIZE		(2 * 1024 * 1024)
#define ISP_FW_STACK_BUF_SIZE		(8 * 64 * 1024)
//to do check, aidt use 2M heap
// 2M heap (896 * 1024)	/*(1 * 1024 * 1024)  //(896 * 1024) */
#define ISP_FW_HEAP_BUF_SIZE  (11 * 1024 * 1024 / 2)
#define ISP_FW_TRACE_BUF_SIZE (2*1024*1024)// (64 * 1024)
#define ISP_FW_CMD_BUF_SIZE (MAX_NUM_HOST2FW_COMMAND * HOST2FW_COMMAND_SIZE)
#define ISP_FW_CMD_BUF_COUNT 4
//#define ISP_FW_HIGHT_CMD_BUF_SIZE (4 * 1024)
#define ISP_FW_RESP_BUF_SIZE (MAX_NUM_FW2HOST_RESPONSE * FW2HOST_RESPONSE_SIZE)
#define ISP_FW_RESP_BUF_COUNT 4
#define MAX_DUMP_RAW_COUNT 200

#define ISP_FW_CMD_PAY_LOAD_BUF_SIZE \
		(ISP_FW_WORK_BUF_SIZE - (ISP_FW_CODE_BUF_SIZE +\
		ISP_FW_STACK_BUF_SIZE + ISP_FW_HEAP_BUF_SIZE +\
		ISP_FW_TRACE_BUF_SIZE +\
		ISP_FW_CMD_BUF_SIZE * ISP_FW_CMD_BUF_COUNT +\
		ISP_FW_RESP_BUF_SIZE * ISP_FW_RESP_BUF_COUNT))

#define ISP_FW_CMD_PAY_LOAD_BUF_ALIGN 64
#define BUFFER_ALIGN_SIZE (0x400)
#define STREAM_TMP_BUF_COUNT 4
#define STREAM_META_BUF_COUNT 3
#define EMB_DATA_BUF_NUM 4
//#define MAX_AF_ROI_NUM 3
#define MAX_REAL_FW_RESP_STREAM_NUM 4
#define ISP_IRQ_SOURCE 1
#define CLK0_CLK1_DFS_CNTL_n0 0x0005B12C
#define CLK0_CLK3_DFS_CNTL_n1 0x0005B97C
#define CLK0_CLK2_DFS_CNTL_n0 0x0005B154
#define CLK0_CLK3_DFS_CNTL_n0 0x0005B17C
#define CLK0_CLK2_BYPASS_CNTL_n0 0x0005B174
#define CLK0_CLK2_SLICE_CNTL_n0 0x0005B170
#define CLK0_CLK_DFSBYPASS_CONTROL_n0 0x0005B1f8
#define CLK0_CLK3_BYPASS_CNTL_n0 0x0005B19C
#define CLK0_CLK3_SLICE_CNTL_n0 0x0005B198
#define CLK0_CLK1_CURRENT_CNT_n0 0x0005B274
#define CLK0_CLK2_CURRENT_CNT_n0 0x0005B278
#define CLK0_CLK3_CURRENT_CNT_n0 0x0005B27C

#define HDP_HOST_PATH_CNTL 0x00003FB0
#define HDP_XDP_CHKN 0x00004180

#define RSMU_PGFSM_STATUS_ISP_rsmuISP 0x902C790
#define RSMU_PGFSM_CONTROL_ISP_rsmuISP 0x902C784
#define RSMU_HARD_RESETB_ISP_rsmuISP 0x902C008
#define RSMU_COLD_RESETB_ISP_rsmuISP 0x902C004
#define RSMU_SMS_FUSE_CFG_ISP_rsmuISP 0x902C028
#define NBIF_GPU_PCIE_INDEX 0x30
#define NBIF_GPU_PCIE_DATA 0x34

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
#define EMB_DATA_BUF_SIZE (32 * 1024)
#define CMD_RESPONSE_BUF_SIZE (64 * 1024)
#define PFC_CNT_BEFORE_START_STREAM 1

#define CVIP_PRE_POST_BUF_SIZE 0x12000

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
	(((unsigned int)(a)<<0) | ((unsigned int)(b)<<8) |\
	((unsigned int)(c)<<16) | ((unsigned int)(d)<<24))

#define MAX_PREVIEW_BUFFER_NUM		(4)	/*(6) */
#define MAX_SUBPREVIEW_BUFFER_NUM	(4)
#define MAX_VIDEO_BUFFER_NUM		(4)
#define MAX_RAW_BUFFER_NUM		(25)
#define MAX_JPEG_WIDTH			(1920)
#define MAX_JPEG_HEIGHT			(1080)
#define MAX_JPEG_BUFFER_SIZE		(MAX_JPEG_WIDTH * MAX_JPEG_HEIGHT * 2)
#define MAX_JPEG_BUFFER_NUM		(2)
#define MAX_YUV422_BUFFER_NUM		(2)
#define MAX_META_DATA_BUF_NUM_PER_SENSOR	4
#define MAX_X_Y_ZOOM_MULTIPLE			2

#define NUM_I2C_FPGA		(4) // total 5 i2c on FPGA
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
#define MAX_SLEEP_TIME		(20)

#define ISP_ADDR_ALIGN_UP(addr, align) \
	((((unsigned long long)(addr)) + \
	(((unsigned long long)(align))-1)) &\
	~(((unsigned long long)(align))-1))
#define ISP_ALIGN_SIZE_1K	(0x400)
#define ISP_ADDR_ALIGN_UP_1K(addr) \
	(ISP_ADDR_ALIGN_UP(addr, ISP_ALIGN_SIZE_1K))
#define GEN_64BIT_ADDR(ADDRHI, ADDRLO) \
	((((unsigned long long)(ADDRHI)) << 32) |\
	((unsigned long long)(ADDRLO)))

#define SKIP_FRAME_COUNT_AT_START	0

#define FPGA_PHOTE_SEQUENCE_DEPTH	2
#define MAX_CALIB_ITEM_COUNT		MAX_PROFILE_NO

//fw support 4 calib tuning data for each sensor which are
//ConfigA: 4096x3072@30fps
//ConfigC: 2048x1536@30fps short exposure
//ConfigD: 2048x1536@30fps long exposure
//ConfigF is for 2048x1536@60fps
#define MAX_CALIB_NUM_IN_FW		4

#define ISP_MC_ADDR_ALIGN		(1024 * 32)
#define ISP_PREFETCH_GAP		(1024 * 32)
#define ISP_MC_PREFETCH_GAP		(1024 * 32)

#define BUF_NUM_BEFORE_START_CMD 2

#ifndef true
#define true 1
#endif

#ifndef false
#define false 0
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

typedef int (*func_get_cvip_buf)(unsigned long *size, uint64_t *gpu_addr,
				 void **cpu_addr);
typedef int (*func_cvip_set_gpuva)(uint64_t gpuva);
// need separate TILE per power type.
//typedef int (*func_set_isp_power)(uint8_t enable, uint8_t selection);
typedef int (*func_set_isp_power)(uint8_t enable);
typedef int (*func_set_isp_clock)(unsigned int sclk, unsigned int iclk,
		unsigned int xclk);

struct swisp_services {
	func_get_cvip_buf get_cvip_buf;
	func_cvip_set_gpuva cvip_set_gpuva;
	func_set_isp_power set_isp_power;
	func_set_isp_clock set_isp_clock;
};

struct isp_mc_addr_node {
	unsigned long long start_addr;
	unsigned long long align_addr;
	unsigned long long end_addr;
	unsigned int size;
	struct isp_mc_addr_node *next;
	struct isp_mc_addr_node *prev;
};

struct isp_mc_addr_mgr {
	struct isp_mc_addr_node head;
	struct mutex mutext;
	unsigned long long start;
	unsigned long long len;
};

struct sys_to_mc_map_info {
	unsigned long long sys_addr;
	unsigned long long mc_addr;
	unsigned int len;
};

struct isp_mapped_buf_info {
	struct list_node node;
	unsigned char camera_id;
	unsigned char str_id;
	/*unsigned char map_count;
	 *enum isp_buffer_status buf_status;
	 */
	struct sys_img_buf_handle *sys_img_buf_hdl;
	unsigned long long multi_map_start_mc;
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
	unsigned short camera_id;
	unsigned short enable_bit;
	struct isp_mapped_buf_info *yuv_in;
	struct isp_mapped_buf_info *main_jepg;
	struct isp_mapped_buf_info *embed_thumb_jepg;
	struct isp_mapped_buf_info *standalone_thumb_jepg;
	struct isp_mapped_buf_info *yuv_out;
};

struct isp_meta_data_info {
	struct list_node node;
	struct _MetaInfo_t meta_data;
	struct sys_to_mc_map_info map_info;
};

struct isp_stream_info {
	enum pvt_img_fmt format;
	unsigned int width;
	unsigned int height;
	unsigned int fps;
	unsigned int luma_pitch_set;
	unsigned int chroma_pitch_set;
	unsigned int max_fps_numerator;
	unsigned int max_fps_denominator;
	struct isp_list buf_free;
	struct isp_list buf_in_fw;
	struct isp_gpu_mem_info *uv_tmp_buf;
	/*
	 *struct isp_list linear_tmpbuf_free;
	 *struct isp_list linear_tmpbuf_in_fw;
	 */
	enum start_status start_status;
	unsigned char running;
	unsigned char buf_num_sent;
	unsigned char tnr_on;
	unsigned char vs_on;
	bool is_perframe_ctl;
	/*
	 *struct isp_mapped_buf_info *tnr_ref_buf;
	 */
};

struct zoom_information {
	unsigned int ratio;
	short v_offset;
	short h_offset;

	struct _Window_t window;
};

struct fps_table {
	unsigned short HighAddr;
	unsigned short LowAddr;
	unsigned short HighValue;
	unsigned short LowValue;
	float fps;
};

struct isp_cos_sys_mem_info {
	unsigned long long mem_size;
	void *sys_addr;
	void *mem_handle;
};

struct sensor_info {
	enum camera_id cid;
	enum camera_id actual_cid;
	struct calib_date_item *cur_calib_item;
	struct isp_gpu_mem_info *calib_gpu_mem[MAX_CALIB_NUM_IN_FW];
	struct isp_gpu_mem_info *m2m_calib_gpu_mem;
	struct isp_gpu_mem_info *stream_tmp_buf[STREAM_TMP_BUF_COUNT];
	struct isp_gpu_mem_info *cmd_resp_buf;
	struct isp_gpu_mem_info *cvip_pre_post_buf;
	struct isp_gpu_mem_info *cvip_stats_buf;
	struct isp_gpu_mem_info *loopback_raw_buf;
	struct isp_gpu_mem_info *loopback_raw_info;
	unsigned long long meta_mc[STREAM_META_BUF_COUNT];
	struct isp_gpu_mem_info *tnr_tmp_buf[MAX_TNR_REF_BUF_CNT];
	struct isp_gpu_mem_info *hdr_stats_buf;
	struct isp_gpu_mem_info *iic_tmp_buf;
	struct isp_gpu_mem_info *emb_data_buf[EMB_DATA_BUF_NUM];
	struct isp_gpu_mem_info *stage2_tmp_buf;
	struct isp_list take_one_pic_buf_list;
	struct isp_list rgbir_raw_output_in_fw;
	struct isp_list rgbraw_input_in_fw;
	struct isp_list irraw_input_in_fw;
	struct isp_list rgbir_frameinfo_input_in_fw;
	unsigned int take_one_pic_left_cnt;
	struct _isp_sensor_prop_t sensor_cfg;
	enum start_status status;
	enum _AeMode_t aec_mode;
	enum _AfMode_t af_mode;
	enum _FocusAssistMode_t flash_assist_mode;
	unsigned int flash_assist_pwr;
	struct _LensRange_t lens_range;
	struct fps_table fps_tab;
	enum _AwbMode_t awb_mode;
	struct _Window_t ae_roi;
	struct _Window_t af_roi[MAX_AF_ROI_NUM];
	struct _Window_t awb_region;
//	FlashMode_t flash_mode;
	unsigned int flash_pwr;
//	RedEyeMode_t redeye_mode;
	unsigned int color_temperature;	/*in K */
/*previous*/
	unsigned int raw_width;
	unsigned int raw_height;
	struct _Window_t asprw_wind;
	struct sensor_res_fps_t res_fps;
	struct isp_stream_info str_info[STREAM_ID_NUM + 1];
	/*int cproc_enable; */
	int para_brightness_cur;
	int para_brightness_set;
	int para_contrast_cur;
	int para_contrast_set;
	int para_hue_cur;
	int para_hue_set;
	int para_satuaration_cur;
	int para_satuaration_set;
	enum _AeFlickerType_t flick_type_cur;
	enum _AeFlickerType_t flick_type_set;
	struct _AeIso_t para_iso_cur;
	struct _AeIso_t para_iso_set;
	unsigned int para_color_temperature_cur;
	unsigned int para_color_temperature_set;

	int para_color_enable_cur;
	int para_color_enable_set;
	struct _AeEv_t para_ev_compensate_cur;
	struct _AeEv_t para_ev_compensate_set;

	int para_gamma_cur;
	int para_gamma_set;
	enum pvt_scene_mode para_scene_mode_cur;
	enum pvt_scene_mode para_scene_mode_set;

	/*struct isp_still_img_info still_info; */
	struct _MetaInfo_t meta_data;
	struct isp_list meta_data_free;
	struct isp_list meta_data_in_fw;
	/*
	 *struct mutex snr_mutex;
	 *struct mutex stream_op_mutex;
	 *
	 *struct isp_mapped_buf_info *script_bin_buf;
	 */
	struct _CmdSetDeviceScript_t script_cmd;
	unsigned int resume_cmd;
	struct isp_event resume_success_evt;
	float sharpness[3];
	struct zoom_information zoom_info;
	unsigned int zsl_ret_width;
	unsigned int zsl_ret_height;
	unsigned int zsl_ret_stride;
	unsigned int open_flag;
	enum camera_type cam_type;
	enum fw_cmd_resp_stream_id stream_id;
	enum isp_scene_mode scene_mode;
	enum _RawPktFmt_t raw_pkt_fmt;
	unsigned char zsl_enable;
	unsigned char resend_zsl_enable;
	char cur_res_fps_id;
	char stream_inited;
	char sensor_opened;
	char hdr_enable;
	char hdr_prop_set_suc;
	char hdr_enabled_suc;
	char calib_enable;
	char calib_set_suc;
	char tnr_tmp_buf_set;
	char tnr_enable;
	char start_str_cmd_sent;
	//struct _cproc_header *cproc_value;
	struct _wb_light_source_index *wb_idx;
	struct _scene_mode_table *scene_tbl;
	bool enable_dynamic_frame_rate;

	// Face Authtication Mode
	int face_auth_mode;
	int dump_raw_enable;
	int dump_raw_count;
	unsigned int per_frame_ctrl_cnt;
};

struct profile_info {
	enum mode_profile_name id;
	enum _CFAPattern_t cfa_fmt;
	enum _MipiDataType_t raw_fmt;
	unsigned char num_lanes;
	unsigned char hdr_valid;
	unsigned int h_size;
	unsigned int v_size;
	unsigned int frame_rate;
	unsigned int t_line;
	unsigned int frame_length_lines;
	unsigned int mipi_bitrate;
	unsigned int init_itime;
};

#define I2C_REGADDR_NULL 0xffff

struct isp_cmd_element {
	unsigned int seq_num;
	unsigned int cmd_id;
	enum fw_cmd_resp_stream_id stream;
	unsigned long long mc_addr;
	long long send_time;
	struct isp_event *evt;
	struct isp_gpu_mem_info *gpu_pkg;
	void *resp_payload;
	unsigned int *resp_payload_len;
	unsigned short I2CRegAddr;
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
};

#define MAX_CAMERA_DEVICE_TYPE_NAME_LEN 10

struct dphy_config_para {
	unsigned int value;		/*to be added later */
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

	unsigned char isp_present;
	unsigned char pack;		/*to make it aligh 64 */
	unsigned char isp_mem_size_b0;	/*the 0 byte of the value */
	unsigned char isp_mem_size_b1;	/*the 0 byte of the value */
	unsigned char isp_mem_size_b2;	/*the 0 byte of the value */
	unsigned char isp_mem_size_b3;	/*the 0 byte of the value */

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
	int disable_calib;
	union mode_param_t {
		struct _loopback_preview_param_t_unused {
			int en_continue;
			enum isp_bayer_pattern bayer_pattern;
			char *raw_file;
		} loopback_preview;
	} mode_param;
};

struct isp_config_preview_param {
	unsigned int preview_width;
	unsigned int preview_height;
	unsigned int preview_luma_picth;
	unsigned int preview_chroma_pitch;
	int disable_calib;
};

struct photo_seque_info {
	unsigned short max_fps;
	unsigned short cur_fps;
	unsigned short max_history_frame_cnt;
	unsigned short cur_history_frame_cnt;
	unsigned int task_bits_by_buf;
	unsigned int task_bits_by_cmd;
	unsigned int buf_sent;
	enum start_status stream_status;
	int running;
};

struct isp_gpu_mem_info {
	enum isp_gpu_mem_type mem_src;
	cgs_handle_t handle;
	unsigned long long mem_size;
	unsigned long long gpu_mc_addr;
	void *sys_addr;
	void *mem_handle;
	void *acc_handle;
};

struct isp_mode_cfg_para {
	enum isp_pipe_mode mode;
	int disable_calib;
	union _mode_param_t {
		struct _loopback_preview_param_t {
			int en_continue;
			enum isp_bayer_pattern bayer_pattern;
			char *raw_file;
		} loopback_preview;

		struct _multistream_param_t {
			int en_preview;
			int en_video;
			int en_zsl;
		} multistream;
	} mode_param;
};

struct isp_pipe_info {
	enum isp_pipe_used_status pipe_used_status;
	/*struct isp_mode_cfg_para next_mode_param; */
	/*struct isp_mode_cfg_para mode_param; */
	enum isp_pipe_mode mode;
	/*unsigned long long loopback_raw_base; */
	/*unsigned int loopback_raw_size; */
};
struct calib_date_item {
	unsigned int width;
	unsigned int height;
	unsigned int fps;
	unsigned int hdr;
	enum expo_limit_type expo_limit;
	void *data;
	unsigned int len;
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
	struct isp_gpu_mem_info *tnr_tmp_buf[MAX_TNR_REF_BUF_CNT];
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
	unsigned int image_id[4];
	unsigned int header_ver;
	unsigned int fw_size_in_sig;
	unsigned int encrypt_opt;
	unsigned int encrypt_id;
	unsigned int encrypt_param[4];
	unsigned int sig_opt;
	unsigned int sig_id;
	unsigned int sig_param[4];
	unsigned int compress_opt;
	unsigned int compress_id;
	unsigned int uncompress_size;
	unsigned int compress_size;
	unsigned int compress_param[2];
	unsigned int fw_version;
	unsigned int family_id;
	unsigned int load_addr;
	unsigned int signed_img_size;
	unsigned int unsigned_size;
	unsigned int offset_nwd;
	unsigned int fw_version_sign_opt;
	unsigned int reserved;
	unsigned int encrypt_key[4];
	unsigned int tool_info[4];
	unsigned int fw_data[4];
	unsigned int reserved_zero[4];
};

struct isp_context {
	void *sw_isp_context;
	enum isp_status isp_status;
	struct mutex ops_mutex;

	unsigned long long fw_mc_loaded_by_psp;
	/*enum isp_pwr_unit_status1 isp_pwr_status; */
	/*enum isp_dphy_pwr_statusdfsdfs1 dphy_pwr_status; */
	/*enum sdf a; */
	/*struct isp_spin_lock isp_pwr_spin_lock; */
	/*struct isp_event isp_pwr_on_evt; */
	/*struct isp_event isp_pwr_off_evt; */

	struct isp_pwr_unit isp_pu_isp;
	struct isp_pwr_unit isp_pu_dphy;
	struct isp_pwr_unit isp_pu_cam[CAMERA_ID_MAX];
	unsigned int isp_fw_ver;
	int isp_flash_torch_on;

	struct isp_capabilities isp_cap;
	struct isp_config_mode_param mode_param;
	struct isp_gpu_mem_info fw_work_buf;
	void *fw_work_buf_hdl;
	struct isp_gpu_mem_info *fb_buf;

	void *fb_hdl;

	unsigned long long fw_cmd_buf_sys[ISP_FW_CMD_BUF_COUNT];
	unsigned long long fw_cmd_buf_mc[ISP_FW_CMD_BUF_COUNT];
	unsigned int fw_cmd_buf_size[ISP_FW_CMD_BUF_COUNT];
	unsigned long long fw_resp_buf_sys[ISP_FW_RESP_BUF_COUNT];
	unsigned long long fw_resp_buf_mc[ISP_FW_RESP_BUF_COUNT];
	unsigned int fw_resp_buf_size[ISP_FW_RESP_BUF_COUNT];
	unsigned long long fw_log_sys;
	unsigned long long fw_log_mc;
	unsigned int fw_log_size;

	struct isp_cmd_element *cmd_q;
	struct mutex cmd_q_mtx;

	unsigned int sensor_count;
	struct camera_dev_info cam_dev_info[CAMERA_ID_MAX];
	struct thread_handler fw_resp_thread[MAX_REAL_FW_RESP_STREAM_NUM];
	unsigned long long irq_enable_id[MAX_REAL_FW_RESP_STREAM_NUM];
	/*struct thread_handler poll_thread_hdl; */
	/*struct thread_handler idle_detect_thread; */

//struct isp_stream_info stream_info[CAMERA_ID_MAX][STREAM_ID_NUM + 1];

	struct mutex take_pic_mutex;
	struct isp_list take_photo_seq_buf_list;
	struct isp_list work_item_list;
	struct photo_seque_info photo_seq_info[CAMERA_ID_MAX];
	enum camera_id cur_photo__seq_cam_id;
	/*sensor_operation_t sensor_ops[CAMERA_ID_MAX]; */
	struct sensor_operation_t *p_sensor_ops[CAMERA_ID_MAX];
	struct vcm_operation_t *p_vcm_ops[CAMERA_ID_MAX];
	struct flash_operation_t *p_flash_ops[CAMERA_ID_MAX];
	struct fw_parser_operation_t *fw_parser[CAMERA_ID_MAX];
	struct mutex command_mutex;	/*mutext to command */
	struct mutex response_mutex;	/*mutext to retrieve response */

	unsigned int host2fw_seq_num;

	unsigned int reg_value;
	unsigned int fw2host_response_result;
	unsigned int fw2host_sync_response_payload[40];

	/*struct isp_ring_log_buf_context ring_log_buf_info; */

	func_isp_module_cb evt_cb[CAMERA_ID_MAX];
	void *evt_cb_context[CAMERA_ID_MAX];
	void *fw_data;
	unsigned int fw_len;
	void *fw_mem_hdl;
	struct calib_date_item
	*calib_items[CAMERA_ID_MAX][MAX_CALIB_ITEM_COUNT];
	void *calib_data[CAMERA_ID_MAX];
	unsigned int calib_data_len[CAMERA_ID_MAX];
	unsigned int sclk;		//In MHZ
	unsigned int iclk;		//In MHZ
	unsigned int xclk;		//In MHZ
	unsigned int refclk;		//In MHZ
	struct driver_settings drv_settings;
	bool snr_res_fps_info_got[CAMERA_ID_MAX];
	bool fw_ctrl_3a;
	bool flashlight_cfg_to_be_set;
	bool clk_info_set_2_fw;
	bool snr_info_set_2_fw;
	bool req_fw_load_suc;
	struct mutex map_unmap_mutex;
	struct isp_work_buf work_buf;
	struct hwip_discovery_info hw_ip_info;
	struct isp_cos_sys_mem_info prefetch_buf;
	struct sensor_info sensor_info[CAMERA_ID_MAX];
	struct profile_info prf_info[CAMERA_ID_MAX][PROFILE_MAX];
};

struct isp_fw_resp_thread_para {
	unsigned int idx;
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
	extern unsigned int g_num_i2c;


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
void isp_reset_camera_info(struct isp_context *isp,
			enum camera_id cid);
void init_all_camera_info(struct isp_context *pisp_context);
void init_all_camera_dev_info(struct isp_context *pisp_context);
void isp_init_drv_setting(struct isp_context *isp);

int ispm_sw_init(struct isp_context *isp_context,
		unsigned int init_falg, void *gart_range_hdl);
int ispm_sw_uninit(struct isp_context *isp_context);
void unit_isp(void);

void isp_idle_detect(struct isp_context *isp_context);
void isp_hw_ccpu_stall(struct isp_context *isp_context);
void isp_hw_ccpu_disable(struct isp_context *isp_context);

void isp_hw_ccpu_rst(struct isp_context *isp_context);
void isp_hw_ccpu_stall(struct isp_context *isp_context);
void isp_hw_reset_all(struct isp_context *isp);

void isp_hw_ccpu_enable(struct isp_context *isp_context);
int isp_load_fw_fpga(struct isp_context *isp_context, void *fw_data,
			unsigned int fw_len);

void isp_init_fw_rb_log_buffer(struct isp_context *isp_context,
		unsigned int fw_rb_log_base_lo,
		unsigned int fw_rb_log_base_hi,
		unsigned int fw_rb_log_size);

int isp_snr_open(struct isp_context *isp_context,
		enum camera_id camera_id, int res_fps_id,
		unsigned int flag);

int isp_snr_close(struct isp_context *isp, enum camera_id cid);

int isp_snr_pwr_toggle(struct isp_context *isp,
			   enum camera_id cid, bool on);
int isp_snr_clk_toggle(struct isp_context *isp,
			   enum camera_id cid, bool on);

int isp_get_pic_buf_param(struct isp_stream_info *str_info,
		struct isp_picture_buffer_param *buffer_param);

bool isp_get_str_out_prop(struct isp_stream_info *str_info,
			struct _ImageProp_t *out_prop);

int send_prev_buf_and_put_to_inuse_list
				(struct isp_context *isp_context,
				enum camera_id camera_id,
				struct isp_img_buf_info *buffer);

void rst_snr_str_img_buf_type(struct isp_context *isp_context,
			enum camera_id camera_id,
			enum stream_id stream_type);

int send_record_buf_and_put_to_inuse_queue
				(struct isp_context *isp_context,
				enum camera_id cam_id,
				struct isp_img_buf_info *buffer);

int isp_set_stream_data_fmt(struct isp_context *isp_context,
				enum camera_id camera_id,
				enum stream_id stream_type,
				enum pvt_img_fmt img_fmt);
int isp_set_str_res_fps_pitch(struct isp_context *isp_context,
				enum camera_id camera_id,
				enum stream_id stream_type,
				struct pvt_img_res_fps_pitch *value);

int isp_switch_from_preview_to_video2d(struct isp_context
				*isp_context,
				enum camera_id cam_id);

int isp_send_stop_record_cmd(struct isp_context *isp_context,
				enum camera_id camera_id);
int is_camera_started(struct isp_context *isp_context,
				enum camera_id cam_id);

int isp_take_oneshot_pic(struct isp_context *isp,
				enum camera_id cam_id,
				struct sys_img_buf_handle *main_jpg_buf,
				struct sys_img_buf_handle *main_yuv_buf,
				struct sys_img_buf_handle *jpg_thumb_buf,
				struct sys_img_buf_handle *yuv_thumb_buf);
int isp_take_oneshot_pic_fpga(struct isp_context *context,
				enum camera_id cam_id,
				struct sys_img_buf_handle *main_jpg_buf,
				struct sys_img_buf_handle *main_yuv_buf,
				struct sys_img_buf_handle *jpg_thumb_buf,
				struct sys_img_buf_handle *yuv_thumb_buf);
int isp_send_one_photo_seq_buf(struct isp_context *context,
				enum camera_id cam_id,
				struct sys_img_buf_handle *main_jpg_buf,
				struct sys_img_buf_handle *main_yuv_buf,
				struct sys_img_buf_handle *jpg_thumb_buf,
				struct sys_img_buf_handle *yuv_thumb_buf);

int isp_start_photo_seq(struct isp_context *context,
				enum camera_id cam_id, unsigned int fps,
				unsigned int task_bits);
int isp_stop_photo_seq(struct isp_context *context,
				enum camera_id cam_id);

int create_video_queue_and_send_buf
				(struct isp_context *isp_context,
				enum camera_id cam_id);
int fw_config_video2d(struct isp_context *isp_context,
				enum camera_id cam_id);
int fw_switch_from_preview_to_video
				(struct isp_context *isp_context,
				enum camera_id cam_id);
int fw_send_preview_to_video2d_cmd
				(struct isp_context *isp_context);
int isp_switch_video2d_to_preview(struct isp_context *isp_context,
				enum camera_id cam_id);
int isp_switch_video2d_to_idle(struct isp_context *isp_context,
				enum camera_id cam_id);
void isp_take_back_metadata_buf(struct isp_context *isp,
				enum camera_id cam_id);
int fw_send_stop_stream_cmd(struct isp_context *isp_context,
				enum camera_id cam_id);

struct take_pic_orig_req *isp_create_orig_pic_request
				(struct isp_context *isp,
				enum camera_id cam_id,
				struct sys_img_buf_handle *main_jpg_buf,
				struct sys_img_buf_handle *main_yuv_buf,
				struct sys_img_buf_handle *jpg_thumb_buf,
				struct sys_img_buf_handle *yuv_thumb_buf);
void isp_free_orig_pic_request(struct isp_context *isp,
				struct take_pic_orig_req *orig_req);
int isp_alloc_sys_mem_hdl(struct isp_context *isp, void *addr,
				unsigned long long len, void **mem_hdl);
int isp_release_sys_mem_hdl(struct isp_context *isp,
				 void *mem_hdl);

struct isp_mapped_buf_info *isp_map_sys_2_mc
				(struct isp_context *isp,
				struct sys_img_buf_handle *sys_img_buf_hdl,
				unsigned int mc_align, unsigned short cam_id,
				unsigned short str_id, unsigned int y_len,
				unsigned int u_len, unsigned int v_len);

void isp_unmap_sys_2_mc(struct isp_context *isp,
			struct isp_mapped_buf_info *buff);
struct isp_mapped_buf_info *isp_map_sys_2_mc_single
			(struct isp_context *isp,
			struct sys_img_buf_handle *sys_img_buf_hdl,
			unsigned int mc_align, unsigned short cam_id,
			unsigned short str_id,
			unsigned int y_len, unsigned int u_len,
			unsigned int v_len);
void isp_unmap_sys_2_mc_single(struct isp_context *isp,
			struct isp_mapped_buf_info *buff);
struct isp_mapped_buf_info *isp_map_sys_2_mc_multi(
				struct isp_context *isp,
				struct sys_img_buf_handle *
				sys_img_buf_hdl,
				unsigned int mc_align,
				unsigned short cam_id,
				unsigned short str_id,
				unsigned int y_len,
				unsigned int u_len,
				unsigned int v_len);
void isp_unmap_sys_2_mc_multi(struct isp_context *isp,
				struct isp_mapped_buf_info *buff);

struct isp_general_still_img_buf_info *isp_gen_gerneral_still_img
			(struct isp_context *isp,
			struct sys_img_buf_handle *main_jpg_hdl,
			struct sys_img_buf_handle *emb_thumb_hdl,
			struct sys_img_buf_handle *standalone_thumb_hdl,
			struct sys_img_buf_handle *yuv_out_hdl,
			unsigned int mc_align, unsigned short cam_id);

struct isp_general_still_img_buf_info *isp_gen_gerneral_pp_still_img
			(struct isp_context *isp,
			struct sys_img_buf_handle *in_yuy2_buf,
			struct sys_img_buf_handle *main_jpg_buf,
			struct sys_img_buf_handle *emb_thumb_hdl,
			struct sys_img_buf_handle *
			standalone_thumb_hdl,
			unsigned int mc_align, unsigned short cam_id);

void isp_sys_mc_unmap_and_free(struct isp_context *isp,
			struct isp_mapped_buf_info *info);

int isp_do_sys_2_mc_map(struct isp_context *isp,
			unsigned long long sys,
			unsigned long long mc, unsigned int len,
			void **mem_desc);
int isp_do_sys_2_mc_unmap(struct isp_context *isp,
		unsigned long long sys,
		unsigned long long mc, unsigned int len, void *mem_desc);

int isp_sw_unmap_sys_to_mc_soc(struct isp_context *isp,
			struct sys_to_mc_map_info *info);

int isp_sw_map_sys_to_mc_fpga(struct isp_context *isp,
			unsigned long long sys,
			unsigned long long mc, unsigned int len,
			struct sys_to_mc_map_info *info);
int isp_sw_unmap_sys_to_mc_fpga(struct isp_context *isp,
			struct sys_to_mc_map_info *info);

int get_free_pipe_for_cam(struct isp_context *isp,
			enum camera_id cam_id);
int isp_stop_video2d_to_video_only(struct isp_context *isp_context,
				enum camera_id cam_id,
				int pipe_id);
int isp_stop_video2d_to_preview(struct isp_context *isp_context,
				enum camera_id cam_id,
				int pipe_id);
int isp_start_stream(struct isp_context *isp_context,
			enum camera_id cam_id, bool is_perframe_ctl);
int isp_setup_output(struct isp_context *isp, enum camera_id cid);
int isp_setup_ae(struct isp_context *isp, enum camera_id cid);
int isp_setup_ae_range(struct isp_context *isp,
			enum camera_id cid);

int isp_setup_af(struct isp_context *isp, enum camera_id cid);
int isp_setup_awb(struct isp_context *isp, enum camera_id cid);
int isp_setup_ctls(struct isp_context *isp, enum camera_id cid);

int isp_init_stream(struct isp_context *isp,
			 enum camera_id cam_id);
int isp_setup_3a(struct isp_context *isp, enum camera_id cam_id);
int isp_kickoff_stream(struct isp_context *isp,
			enum camera_id cam_id, unsigned int w, unsigned int h);

int isp_uninit_stream(struct isp_context *isp,
			enum camera_id cam_id);

int isp_start_zsl_stream(struct isp_context *isp_context,
			enum camera_id cam_id);
int isp_stop_zsl_stream(struct isp_context *isp_context,
			enum camera_id cam_id);

struct sys_img_buf_handle *sys_img_buf_handle_cpy(
				struct sys_img_buf_handle *hdl_in);
void sys_img_buf_handle_free(struct sys_img_buf_handle *hdle);

unsigned int isp_calc_total_mc_len(unsigned int y_len,
			unsigned int u_len, unsigned int v_len,
			unsigned int mc_align, unsigned int *u_offset,
			unsigned int *v_offset);
int isp_get_res_fps_id_for_str(struct isp_context *isp,
				enum camera_id cam_id,
				enum stream_id str_id);

int isp_snr_meta_data_init(struct isp_context *isp);
int isp_snr_meta_data_uninit(struct isp_context *isp);
int isp_set_script_to_fw(struct isp_context *isp,
				enum camera_id cid);
void isp_fw_log_print(struct isp_context *isp);
int isp_take_oneshot_pic(struct isp_context *isp,
			enum camera_id cam_id,
			struct sys_img_buf_handle *main_yuv_buf,
			struct sys_img_buf_handle *main_jpg_buf,
			struct sys_img_buf_handle *jpg_thumb_buf,
			struct sys_img_buf_handle *yuv_thumb_buf);
int isp_take_pp_pic(struct isp_context *isp,
			enum camera_id cam_id,
			struct sys_img_buf_handle *in_yuy2_buf,
			struct sys_img_buf_handle *main_jpg_buf,
			struct sys_img_buf_handle *jpg_thumb_buf,
			struct sys_img_buf_handle *yuv_thumb_buf);

unsigned int isp_get_stream_output_bits(struct isp_context *isp,
			enum camera_id, unsigned int *stream_cnt);
unsigned int isp_get_started_stream_count(struct isp_context *isp);

int isp_alloc_tmp_linear_buf(struct isp_context *isp,
			enum camera_id cam_id,
			enum stream_id str_id);
int isp_send_tmp_linear_buf(struct isp_context *isp,
			enum camera_id cam_id, int pipe_id,
			enum stream_id str_id);
int isp_config_vep_for_tiling(struct isp_context *isp,
			enum camera_id cam_id, int pipe_id,
			enum stream_id str_id);
int isp_config_vep_for_tnr(struct isp_context *isp,
			enum camera_id cam_id,
			enum stream_id str_id);

int isp_config_vep_for_vs(struct isp_context *isp,
			enum camera_id cam_id,
			enum stream_id str_id);
void isp_free_tmp_vs_buf(struct isp_context *isp,
			 enum camera_id cam_id,
			 enum stream_id str_id);
int isp_alloc_tmp_vs_buf(struct isp_context *isp,
			enum camera_id cam_id,
			enum stream_id str_id);

int isp_fw_work_mem_rsv(struct isp_context *isp,
			unsigned long long *sys_addr,
			unsigned long long *mc_addr);
int isp_fb_alloc(struct isp_context *isp);
int isp_fb_free(struct isp_context *isp);

unsigned char parse_calib_data(struct isp_context *isp,
			enum camera_id cam_id,
			unsigned char *data, unsigned int len);

void init_calib_items(struct isp_context *isp);
void uninit_calib_items(struct isp_context *isp);

void init_cam_calib_item(struct isp_context *isp, enum camera_id cid);
void uninit_cam_calib_item(struct isp_context *isp,
			  enum camera_id cid);

void add_calib_item(struct isp_context *isp, enum camera_id cam_id,
		   struct _profile_header *profile, void *data,
		   unsigned int len);
struct calib_date_item *isp_get_calib_item(struct isp_context *isp,
			enum camera_id cid);

void isp_init_fw_ring_buf(struct isp_context *isp,
			enum fw_cmd_resp_stream_id idx, unsigned int cmd);
void isp_get_cmd_buf_regs(enum fw_cmd_resp_stream_id idx,
			unsigned int *rreg, unsigned int *wreg,
			unsigned int *baselo_reg, unsigned int *basehi_reg,
			unsigned int *size_reg);
void isp_get_resp_buf_regs(enum fw_cmd_resp_stream_id idx,
			unsigned int *rreg, unsigned int *wreg,
			unsigned int *baselo_reg, unsigned int *basehi_reg,
			unsigned int *size_reg);

void isp_split_addr64(unsigned long long addr, unsigned int *lo,
		unsigned int *hi);

unsigned long long isp_join_addr64(unsigned int lo, unsigned int hi);

unsigned int isp_get_cmd_pl_size(void);

int isp_set_calib_2_fw(struct isp_context *isp, enum camera_id cid);

int isp_set_m2m_calib_2_fw(struct isp_context *isp, enum camera_id cid);
int isp_fw_resp_thread_wrapper(void *context);

int isp_get_caps(struct isp_context *isp, enum camera_id cid);
int isp_set_snr_info_2_fw(struct isp_context *isp, enum camera_id cid);
int isp_setup_stream(struct isp_context *isp, enum camera_id cid);
int isp_setup_stream_tmp_buf(struct isp_context *isp, enum camera_id cid);
int isp_setup_metadata_buf(struct isp_context *isp, enum camera_id cid);

enum fw_cmd_resp_stream_id isp_get_stream_id_from_cid(
	struct isp_context __maybe_unused *isp,
	enum camera_id cid);
enum camera_id isp_get_cid_from_stream_id(struct isp_context *isp,
				enum fw_cmd_resp_stream_id stream_id);
enum camera_id isp_get_rgbir_cid(struct isp_context *isp);

int is_para_legal(void *context, enum camera_id cam_id);
int isp_start_resp_proc_threads(struct isp_context *isp);
int isp_stop_resp_proc_threads(struct isp_context *isp);
void isp_fw_resp_func(struct isp_context *isp,
			enum fw_cmd_resp_stream_id stream_id);
void isp_fw_resp_cmd_done(struct isp_context *isp,
			enum fw_cmd_resp_stream_id stream_id,
			struct _RespCmdDone_t *para);
void isp_fw_resp_cmd_done_extra(struct isp_context *isp,
				enum camera_id cid,
				struct _RespCmdDone_t *para,
				struct isp_cmd_element *ele);
void isp_fw_resp_frame_ctrl_done(struct isp_context *isp,
				 enum fw_cmd_resp_stream_id stream_id,
				 struct _RespParamPackage_t *para);
void isp_fw_resp_frame_info(struct isp_context *isp,
			enum fw_cmd_resp_stream_id stream_id,
			struct _RespParamPackage_t *para);
void isp_fw_resp_error(struct isp_context *isp,
			enum fw_cmd_resp_stream_id stream_id,
			struct _RespError_t *para);
void isp_fw_resp_heart_beat(struct isp_context *isp);

int isp_isr(void *private_data, unsigned int src_id,
		const uint32_t *iv_entry);

void isp_clear_cmdq(struct isp_context *isp);
unsigned int isp_cal_buf_len_by_para(enum pvt_img_fmt fmt,
			unsigned int width,
			unsigned int height, unsigned int l_pitch,
			unsigned int c_pitch, unsigned int *y_len,
			unsigned int *u_len, unsigned int *v_len);
enum _ImageFormat_t isp_trans_to_fw_img_fmt(
			enum pvt_img_fmt format);
enum raw_image_type get_raw_img_type(enum _ImageFormat_t type);
void init_frame_ctl_cmd(struct sensor_info *sif,
			struct _CmdFrameCtrl_t *fc,
			struct frame_ctl_info *ctl_info,
			struct isp_mapped_buf_info *zsl_buf);
int isp_setup_aspect_ratio_wnd(struct isp_context *isp,
			enum camera_id cid, unsigned int w,
			unsigned int h);
enum _SensorId_t isp_get_fw_sensor_id(struct isp_context *isp,
				enum camera_id cid);
enum _StreamId_t isp_get_fw_stream_from_drv_stream
	(enum fw_cmd_resp_stream_id stream);

int isp_is_mem_camera(struct isp_context *isp, enum camera_id cid);
struct _AeIso_t isp_convert_iso_to_fw(ulong value);

bool isp_is_fpga(void);
int isp_register_isr(struct isp_context *isp);
int isp_unregister_isr(struct isp_context *isp);
void isp_fb_get_nxt_indirect_buf(struct isp_context *isp,
				 unsigned long long *sys,
				 unsigned long long *mc);

long IspMapValueFromRange(long srcLow, long srcHigh,
			long srcValue, long desLow, long desHigh);

unsigned int isp_fb_get_emb_data_size(void);

bool roi_window_isvalid(struct _Window_t *wnd);
struct isp_gpu_mem_info *isp_get_work_buf(struct isp_context *isp,
				enum camera_id cid,
				enum isp_work_buf_type buf_type,
				unsigned int idx);
void isp_alloc_work_buf(struct isp_context *isp);
void isp_free_work_buf(struct isp_context *isp);
int isp_get_rgbir_crop_window(struct isp_context *isp,
	enum camera_id cid, struct _Window_t *crop_window);
int isp_read_and_set_fw_info(struct isp_context *isp);
void isp_query_fw_load_status(struct isp_context *isp);
int isp_config_cvip_sensor(struct isp_context *isp, enum camera_id cid,
			   enum _TestPattern_t pattern);
int isp_setup_cvip_buf(struct isp_context *isp, enum camera_id cid);
int create_cvip_buf(unsigned int stats_size);
int isp_stop_cvip_sensor(struct isp_context *isp, enum camera_id cid);
int isp_set_cvip_sensor_slice_num(struct isp_context *isp, enum camera_id cid);
int isp_switch_profile(struct isp_context *isp, enum camera_id cid,
		       unsigned int prf_id);

/**codes to support psp fw loading***/
#define FW_IMAGE_ID_0 0x45574F50
#define FW_IMAGE_ID_1 0x20444552
#define FW_IMAGE_ID_2 0x41205942
#define FW_IMAGE_ID_3 0x0A0D444D

#define PSP_HEAD_SIZE 256

struct hwip_to_fw_name_mapping {
	unsigned int major;
	unsigned int minor;
	unsigned int rev;
	const char *fw_name;
};

enum _StreamMode_t {
	STREAM_MODE_INVALID,
	STREAM_MODE_0,
	STREAM_MODE_1,
	STREAM_MODE_2,
	STREAM_MODE_3,
	STREAM_MODE_4,
	STREAM_MODE_MAX
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

#define TYPE_ANDROID    0x76895321
struct isp_drv_to_cvip_ctl {
	/*
	 * when type is 0x76895321, config will have valid config value
	 * otherwise, it should be ignored
	 */
	u32 type;
	enum _RawPktFmt_t raw_fmt;
	u32 raw_slice_num;
	u32 output_slice_num;
	u32 output_width;
	u32 output_height;
	enum _ImageFormat_t output_fmt;
};

extern int isp_init(struct cgs_device *cgs_dev, void *svr_in);
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
int isp_lfb_resv(void *context, unsigned int size);

/*
 *free the reserved local frame buffer
 */
int isp_lfb_free(void *context);

/*
 *Acquire the CPU access to LFB reserved in ispm_sw_init
 */
int isp_init_lfb_access(struct isp_context *isp);

/*
 *Release the CPU access to LFB reserved in ispm_sw_uninit
 */
int isp_uninit_lfb_access(struct isp_context *isp);

/*
 *Program DS4 bit for ISP in PCTL_MMHUB_DEEPSLEEP
 *Set it to busy (0x0) when FW is running
 *Set it to dile (0x1) in other cases
 */
int isp_program_mmhub_ds_reg(int b_busy);

unsigned char isp_multiple_map_enabled(struct isp_context *isp);
int isp_is_host_processing_raw(struct isp_context *isp,
					enum camera_id cid);
int isp_donot_power_off_fchclk_sensor_pwr(
		struct isp_context *isp);
void isp_pause_stream_if_needed(struct isp_context *isp,
	enum camera_id cid, unsigned int w, unsigned int h);
void isp_calc_aspect_ratio_wnd(struct isp_context *isp,
				enum camera_id cid, unsigned int w,
				unsigned int h,
				struct _Window_t *aprw);
extern void reset_sensor_phy(enum camera_id device_id, bool on,
		bool fpga_env, uint32_t lane_num);
extern unsigned int g_rgbir_raw_count_in_fw;
extern struct isp_context g_isp_context;
extern struct cgs_device *g_cgs_srv;
extern void *mem_copy_virt_addr;
extern dma_addr_t mem_copy_phys_addr;
extern struct swisp_services g_swisp_svr;
extern struct s_properties *g_prop;
extern enum mode_profile_name g_profile_id[CAM_IDX_MAX];
extern struct s_info g_snr_info[CAM_IDX_MAX];
extern struct _M2MTdb_t g_otp[CAM_IDX_MAX];
#endif
