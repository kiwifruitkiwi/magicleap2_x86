/*
 * Copyright (C) 2020 Advanced Micro Devices.
 */

#ifndef ISP_MODULE_INTERFACE_H
#define ISP_MODULE_INTERFACE_H

#include "os_base_type.h"
#include "isp_cam_if.h"
#include <linux/cam_api_simulation.h>

#define CAMERA_DEVICE_NAME_LEN 16

/*
 *Set this flag to open camera in HDR mode,
 *otherwise camera will be opened in normal mode
 */
#define OPEN_CAMERA_FLAG_HDR 0x00000001
/*
 *	enum pvt_img_fmt {
 *	PVT_IMG_FMT_INVALID = -1,
 *	PVT_IMG_FMT_YV12,
 *	PVT_IMG_FMT_I420,
 *	PVT_IMG_FMT_NV21,
 *	PVT_IMG_FMT_NV12,
 *	PVT_IMG_FMT_YUV422P,
 *	PVT_IMG_FMT_YUV422_SEMIPLANAR,
 *	PVT_IMG_FMT_YUV422_INTERLEAVED,
 *	PVT_IMG_FMT_L8,
 *	PVT_IMG_FMT_MAX
 *	};
 */
/*
 *enum pvt_iso {
 *	PVT_ISO_AUTO,
 *	PVT_ISO_100,
 *	PVT_ISO_200,
 *	PVT_ISO_400,
 *	PVT_ISO_800,
 *	PVT_ISO_1600,
 *	PVT_ISO_3200,
 *	PVT_ISO_6400,
 *	PVT_ISO_12800,
 *	PVT_ISO_25600
 *};
 */

enum pvt_scene_mode {
	PVT_SCENE_MODE_AUTO,
	PVT_SCENE_MODE_MANUAL,
	PVT_SCENE_MODE_MACRO,
	PVT_SCENE_MODE_PORTRAIT,
	PVT_SCENE_MODE_SPORT,
	PVT_SCENE_MODE_SNOW,
	PVT_SCENE_MODE_NIGHT,
	PVT_SCENE_MODE_BEACH,
	PVT_SCENE_MODE_SUNSET,
	PVT_SCENE_MODE_CANDLELIGHT,
	PVT_SCENE_MODE_LANDSCAPE,
	PVT_SCENE_MODE_NIGHTPORTRAIT,
	PVT_SCENE_MODE_BACKLIT
};

/*return value of isp module functions*/
enum imf_ret_value {
	IMF_RET_SUCCESS = 0,
	IMF_RET_FAIL = -1,
	IMF_RET_INVALID_PARAMETER = -2
};

enum pic_task_flag {
	PIC_TASK_FLAG_MAIN_JPEG = 0x01,
	PIC_TASK_FLAG_THUMBNAIL_JPEG = 0x02,
	PIC_TASK_FLAG_JPEG_SIZE_NV12 = 0x04,
	PIC_TASK_FLAG_THUMBNAIL_NV12 = 0x08,
};

typedef enum {
	I2C_SLAVE_ADDR_MODE_7BIT,
	I2C_SLAVE_ADDR_MODE_10BIT
} i2c_slave_addr_mode_t;

enum camera_id {
	CAMERA_ID_REAR,		/*rear camera for both integrate and
				 *discrete ISP
				 */
	CAMERA_ID_FRONT_LEFT,	/*front left camera for both integrate and
				 *discrete ISP, it means front camera for
				 *discrete ISP
				 */
	CAMERA_ID_FRONT_RIGHT,	/*front right camera only for integrate ISP */
	CAMERA_ID_MEM,		/*a simu camera whose raw data comes from user
				 *input, we use this camera to do post
				 *processing to raw data to
				 *generate preview/video/zsl output
				 */
	CAMERA_ID_MAX
};

enum stream_id {
	STREAM_ID_PREVIEW,
	STREAM_ID_VIDEO,
	STREAM_ID_ZSL,
	STREAM_ID_RGBIR_IR,
	STREAM_ID_RGBIR,
	STREAM_ID_NUM = STREAM_ID_RGBIR,
};

enum isp_3a_type {
	ISP_3A_TYPE_AF,
	ISP_3A_TYPE_AE,
	ISP_3A_TYPE_AWB,
};

enum cb_evt_id {
	CB_EVT_ID_AF_STATUS,
	CB_EVT_ID_FRAME_DONE,	/*parameter is frame_done_cb_para */
	CB_EVT_ID_FRAME_INFO,	/*parameter is frame_info_cb_para */
	CB_EVT_ID_CMD_DONE,	/*parameter is cmd_done_cb_para */
	/*the following param already removed for windows*/
	CB_EVT_ID_PREV_BUF_DONE,	//param struct buf_cb_para
	CB_EVT_ID_REC_BUF_DONE,	 //param struct buf_cb_para
	CB_EVT_ID_STILL_BUF_DONE,
	CB_EVT_ID_TAKE_ONE_PIC_DONE,	//param struct take_one_pic_cb_para
	CB_EVT_ID_TAKE_RAW_PIC_DONE,	//param struct take_raw_pic_cb_para
	CB_EVT_ID_PARA_SET_DONE,	//param para_set_cb_para
	CB_EVT_ID_PHOTO_SEQ_STARTED,	//param photo_seq_cb_para
	CB_EVT_ID_PHOTO_SEQ_STOPPED,	//param photo_seq_cb_para
	CB_EVT_ID_PHOTO_SEQ_PIC_DONE,	//param struct take_one_pic_cb_para
	CB_EVT_ID_ZSL_BUF_DONE,	//param struct buf_cb_para
	CB_EVT_ID_NOTIFICATION,	//param struct notify_cb_para
	CB_EVT_ID_METADATA_DONE	//param struct meta_databuf_cb_para
};

enum img_data_layout {
	IMG_DATA_LAYOUT_LINEAR,
	IMG_DATA_LAYOUT_TILING
};

enum para_id {
	PARA_ID_DATA_FORMAT,	/*para value type is pvt_img_fmtdone */
	/*para value type is pvt_img_res_fps_pitchdone */
	PARA_ID_DATA_RES_FPS_PITCH,
	PARA_ID_IMAGE_SIZE,	/*para value type is pvt_img_sizedone */
	PARA_ID_JPEG_THUMBNAIL_SIZE,	/*para value type is pvt_img_sizedone*/
	PARA_ID_YUV_THUMBNAIL_SIZE,	/*para value type is pvt_img_sizedone*/
	PARA_ID_YUV_THUMBNAIL_PITCH,	/*para value type is pvt_img_sizedone*/
	PARA_ID_BRIGHTNESS,	/*pvt_int32done */
	PARA_ID_CONTRAST,	/*pvt_int32done */
	PARA_ID_HUE,		/*pvt_int32done */
	PARA_ID_SATURATION,	/*pvt_int32 done */
	PARA_ID_COLOR_TEMPERATURE,	/*uint32 done */
	PARA_ID_ANTI_BANDING,	/*pdata_anti_banding_t done */
	PARA_ID_COLOR_ENABLE,	/*pvt_int32done */
	PARA_ID_GAMMA,		/*int32*done */
	PARA_ID_ISO,		/*AeIso_t **/
	PARA_ID_EV,		/*AeEv_t **/
	PARA_ID_MAIN_JPEG_COMPRESS_RATE,	/*uint32**/
	PARA_ID_THBNAIL_JPEG_COMPRESS_RATE,	/*uint32**/

	PARA_ID_WB_MODE,	/*pdata_wb_t, redefine */
	PARA_ID_EXPOSURE,	/*pdata_exposure_t, */
	PARA_ID_FOCUS,		/*pdata_focus_t, */
	PARA_ID_ROI,		/*pdata_roi_t */

	PARA_ID_PHOTO_MODE,	/*pdata_photo_mode_t */
	PARA_ID_FLASH,		/*pdata_flash_t */
	PARA_ID_SCENE,		/*pvt_scene_mode done */
	PARA_ID_PHOTO_THUMBNAIL,	/*pdata_photo_thumbnail_t */

	/*PARA_ID_IMG_LAYOUT,struct isp_layout_para**/

	PARA_ID_COLOR_CORRECT_TRANSFORM,
	PARA_ID_COLOR_CORRECT_GAINS,
	PARA_ID_AE_MODE,
	PARA_ID_AE_REGIONS,
	PARA_ID_AE_STATE,
	PARA_ID_AF_MODE,
	PARA_ID_AF_REGIONS,
	PARA_ID_AF_STATE,
	PARA_ID_AWB_MODE,
	PARA_ID_AWB_REGIONS,
	PARA_ID_AWB_STATE,
	PARA_ID_CTL_MODE,
	PARA_ID_EDGE_MODE,
	PARA_ID_FIRING_POWER,
	PARA_ID_FIRING_TIME,
	PARA_ID_FLASH_MODE,
	PARA_ID_FLASH_STATE,
	PARA_ID_HOT_PIXEL_MODE,
	PARA_ID_JPEG_QUALITY,
	PARA_ID_JPEG_SIZE,
	PARA_ID_JPEG_THUMBNAIL_QUALITY,
	PARA_ID_FOCUS_DISTANCE,
	PARA_ID_LENS_STATE,
	PARA_ID_NOISE_REDUCTION_MODE,
	PARA_ID_SCALER_CROP_REGION,
	PARA_ID_SENSOR_EXPOSURE_TIME,
	PARA_ID_SENSOR_FRAME_DURATION,
	PARA_ID_SENSOR_SENSITIVITY,
	PARA_ID_SENSOR_TIMESTAMP,
	PARA_ID_SENSOR_TEMPERATURE,
	PARA_ID_SHADING_MODE,
	PARA_ID_STATISTICS_HISTOGRAM,
	PARA_ID_STATISTICS_HISTOGRAM_MODE,
	PARA_ID_STATISTICS_SHAPR_MAP,
	PARA_ID_STATISTICS_SHARP_MAP_MODE,
	PARA_ID_STATISTICS_LENS_SHADING_MAP,
	PARA_ID_STATISTICS_PREDICTED_COLOR_GRAINS,
	PARA_ID_STATISTICS_PREDICTED_COLOR_TRANSFORM,
	PARA_ID_TONE_MAP_CURVE_BLUE,
	PARA_ID_TONE_MAP_CURVE_GREEN,
	PARA_ID_TONE_MAP_CURVE_RED,
	PARA_ID_TONE_MAP_MODE,
	PARA_ID_BLACK_LEVEL_LOCK,

	//following p_a_r_as only support query, in fact they are capabilities
	PARA_ID_AE_AVAILABLE_ANTIBANDING_MODE,
	PARA_ID_AE_AVAILABLE_MODE,
	PARA_ID_AE_AVAILABLE_TARGET_FPS_RANGE,
	PARA_ID_AE_COMPENSATION_RANGE,
	PARA_ID_AE_COMPENSATION_STEP,
	PARA_ID_AF_AVAILABLE_MODE,
	PARA_ID_AVAILABLE_EFFECTS,
	PARA_ID_AVAILABLE_SCENCE_MODE,
	PARA_ID_AVAILABLE_VIDEO_STABIL_MODE,
	PARA_ID_AWB_AVAILABLE_MODE,
	PARA_ID_MAX_REGIONS,
	PARA_ID_SCENE_MODE_OVERRIDE,
	PARA_ID_HOT_PIXEL_INFO_MAP,
	PARA_ID_SCALER_AVAILABLE_FORMAT,
	PARA_ID_SCALER_AVAILABLE_MAX_DIGITAL_ZOOM,
	PARA_ID_HISTOGRAM_BUCKET_COUNT,
	PARA_ID_MAX_HISTOGRAM_COUNT,
	PARA_ID_MAX_SHARPNESS_MAP_VALUE,
	PARA_ID_SHARPNESS_MAP_SIZE,
	PARA_ID_TONE_MAP_MAX_CURVE_POINT,
	PARA_ID_AE_STEP,
	PARA_ID_AF_STEP,
	PARA_ID_AWB_STEP,

	PARA_ID_MAX_PARA_COUNT
};

enum raw_image_type {
	RAW_IMAGE_TYPE_IVALID = -1,
	RAW_IMAGE_TYPE_8BIT,
	RAW_IMAGE_TYPE_10BIT,
	RAW_IMAGE_TYPE_12BIT
};

enum isp_scene_mode {
	ISP_SCENE_MODE_AUTO,
	ISP_SCENE_MODE_MACRO,
	ISP_SCENE_MODE_PORTRAIT,
	ISP_SCENE_MODE_SPORT,
	ISP_SCENE_MODE_SNOW,
	ISP_SCENE_MODE_NIGHT,
	ISP_SCENE_MODE_BEACH,
	ISP_SCENE_MODE_SUNSET,
	ISP_SCENE_MODE_CANDLELIGHT,
	ISP_SCENE_MODE_LANDSCAPE,
	ISP_SCENE_MODE_NIGHTPORTRAIT,
	ISP_SCENE_MODE_BACKLIT,
	ISP_SCENE_MODE_BARCODE,
	ISP_SCENE_MODE_MAX
};

enum buf_done_status {
	//It means no corresponding image buf in callback
	BUF_DONE_STATUS_ABSENT,
	BUF_DONE_STATUS_SUCCESS,
	BUF_DONE_STATUS_FAILED
};

#ifndef MAX_LOOPBACK_SUB_TUNING_CMD_PARAM_LEN
#define MAX_LOOPBACK_SUB_TUNING_CMD_PARAM_LEN 3072
#endif

struct loopback_sub_tuning_cmd {	//same as LOOPBACK_SUB_TUNING_CMD
	//fw command CMD_ID_**from cmdresp.h
	uint32 fw_cmd;
	//if @CmdChannel in command parameter description is StreamX,
	//then set to 1 else 0
	uint32 is_stream_channel;
	//if @ParameterType in command parameter description is Direct,
	//then set to 1 else 0
	uint32 is_direct;
	uint32 param_len;
	//sizeof(LscMatrix_t) is 2312
	//sizeof(PrepLscSector_t) is 96
	//sizeof(DegammaCurve_t) is 118
	//sizeof(WdrCurve_t) is 132
	uint8 param[MAX_LOOPBACK_SUB_TUNING_CMD_PARAM_LEN];
};

struct camera_dev_info {
	char cam_name[CAMERA_DEVICE_NAME_LEN];
	char vcm_name[CAMERA_DEVICE_NAME_LEN];
	char flash_name[CAMERA_DEVICE_NAME_LEN];
	char storage_name[CAMERA_DEVICE_NAME_LEN];
	enum camera_type type;
	int32 focus_len;
};

struct photo_seq_capability {
	uint16 max_frame_history;
	uint16 max_fps;
};

struct para_capability {
	int16 min;
	int16 max;
	int16 step;
	int16 def;		/*default value for this parameter */
};

struct para_capability_u32 {
	uint32 min;
	uint32 max;
	uint32 step;
	uint32 def;		/*default value for this parameter */
};

struct isp_capabilities {
	AeEvCapability_t ev_compensation[CAMERA_ID_MAX];
	struct para_capability brightness;
	struct para_capability contrast;
	struct para_capability gamma;
	struct para_capability hue;
	struct para_capability saturation;
	struct para_capability sharpness;
	AeIsoCapability_t iso[CAMERA_ID_MAX];
	struct para_capability zoom;	/*in the unit of 0.01 */
	struct para_capability zoom_h_off;
	struct para_capability zoom_v_off;
	/*real gain value multiple 1000 */
	struct para_capability_u32 gain;
	/*real integration time multiple 1000000 */
	struct para_capability_u32 itime;
	/*struct photo_seq_capability photo_seq_cap[CAMERA_ID_MAX]; */
};

typedef struct sys_img_buf_handle {
	pvoid virtual_addr;	/*virtual address of the buffer */
	uint64 physical_addr;	/*physical address of buffer */
	uint64 handle;		/*dma buffer fd for linux */
	pvoid private_buffer_info;	/*see below */
	uint32 len;		/*buffer length */
} *sys_img_buf_handle_t;

/*
 *struct isp_pp_jpeg_input {
 *	//only PVT_IMG_FMT_YUV422_INTERLEAVED valid now
 *	enum pvt_img_fmt input_fmt;
 *	uint32 width;
 *	uint32 height;
 *	uint32 y_stride;
 *	uint32 v_stride;
 *	sys_img_buf_handle_t in_buf;
 *};
 */

struct take_one_pic_para {
	enum pvt_img_fmt fmt;
	uint32 width;
	uint32 height;
	uint32 luma_pitch;
	uint32 chroma_pitch;
	uint32 use_precapture;
	uint32 focus_first;
};

struct buf_cb_para {
	sys_img_buf_handle_t bf_hdl;
	uint32 cam_id;
	uint32 poc;
	uint32 finish_status;	/*0 for success; */
	uint32 width;		/*add for zsl */
	uint32 height;		/*add for zsl */
	uint32 y_stride;	/*add for zsl */
	uint32 uv_stride;	/*add for zsl */
};	/*for CB_EVT_ID_PREV_BUF_DONE, CB_EVT_ID_REC_BUF_DONE */

struct buf_done_info {
	enum buf_done_status status;
	struct sys_img_buf_handle buf;
};

struct buf_done_info_ex {
	enum buf_done_status status;
	struct sys_img_buf_handle buf;
	uint32 width;
	uint32 height;
	enum raw_image_type type;
	uint32 raw_img_format;
	CFAPattern_t cfa_pattern;
};

/*call back parameter for CB_EVT_ID_FRAME_INFO*/
struct frame_info_cb_para {
	uint32 cam_id;
	struct sys_img_buf_handle buf;
	uint32 bayer_raw_buf_len;
	uint32 ir_raw_buf_len;
	uint32 frame_info_buf_len;

};

enum isp_cmd_id {
	ISP_CMD_ID_SET_RGBIR_INPUT_BUF,
	ISP_CMD_ID_SET_MEM_CAMERA_INPUT_BUF
};

/*call back parameter for CB_EVT_ID_CMD_DONE*/
struct cmd_done_cb_para {
	enum isp_cmd_id cmd;
	uint32 cam_id;
	uint32 status;		/*0 for success others for fail */
	union {
		/*for ISP_CMD_ID_SET_RGBIR_INPUT_BUF and
		 *ISP_CMD_ID_SET_MEM_CAMERA_INPUT_BUF
		 */
		struct sys_img_buf_handle buf;
	} para;
//sys_img_buf_handle_t;
};

/*call back parameter for CB_EVT_ID_FRAME_DONE*/
struct frame_done_cb_para {
	uint32 poc;
	uint32 cam_id;
	int64 timeStamp;
	struct buf_done_info preview;
	struct buf_done_info video;
	struct buf_done_info zsl;
	struct buf_done_info_ex raw;
	//Window_t zoomWindow;
	//AeMetaInfo_t ae;
	//AfMetaInfo_t af;
	//AwbMetaInfo_t awb;
	//FlashMetaInfo_t flash;
	MetaInfo_t metaInfo;
	uint32 scene_mode;
	uint32 zoom_factor;
};

struct meta_databuf_cb_para {
	sys_img_buf_handle_t bf_hdl;
};	/*for CB_EVT_ID_METADATA_DONE */

struct take_one_pic_cb_para {
	sys_img_buf_handle_t main_jpg_buf;
	/*for pp jpeg, main_yuv_buf will store the original input yuv buffer */
	sys_img_buf_handle_t main_yuv_buf;
	sys_img_buf_handle_t jpeg_thumbnail_buf;
	sys_img_buf_handle_t yuv_thumb_buf;
	uint32 cam_id;
	uint32 flag;
	uint32 finish_status;	/*0 for success; */
};	/*for CB_EVT_ID_TAKE_ONE_PIC_DONE */

enum notify_id {
	NOTIFY_ID_ONE_JPEG_START,
	NOTIFY_ID_PS_START	/*photo sequence */
};

struct notify_cb_para {
	enum notify_id id;
	enum camera_id cam_id;
};

struct take_raw_pic_cb_para {
	sys_img_buf_handle_t raw_data_buf;
	uint32 cam_id;
	uint32 finish_status;	/*0 for success; */
	enum raw_image_type img_type;
};	/*for CB_EVT_ID_TAKE_RAW_PIC_DONE */

struct para_set_cb_para {
	enum para_id para;
	enum camera_id cam_id;
	enum stream_id str_id;
	uint32 set_result;	/*0 for success; */
};

struct photo_seq_cb_para {
	enum camera_id cam_id;
};

typedef int32(*func_isp_module_cb) (pvoid context, enum cb_evt_id event_id,
				pvoid even_para);

/*****************************************************************
 *description : this parameter value type is used for
 *brightness/contrast/hue/saturation
 *para id: PARA_ID_BRIGHTNESS/PARA_ID_CONTRAST
 *PARA_ID_HUE/PARA_ID_SATURATION
 ****************************************************************
 */
struct pvt_int32 {
	int32 value;
};

/*****************************************************************
 *description : this structure is used for anti-banding
 *command id: PID_ANTI_BANDING
 *parameter : pdata_anti_banding_t
 *ISP module: AUTO is not aviable in ISP1.1
 ****************************************************************
 */
enum anti_banding_mode {
	ANTI_BANDING_MODE_DISABLE = 1,
	ANTI_BANDING_MODE_50HZ = 2,
	ANTI_BANDING_MODE_60HZ = 3,
};

struct pvt_anti_banding {
	enum anti_banding_mode value;
};

/*4208x3120//13M*/
struct pvt_img_size {
	uint32 width;
	uint32 height;
};

struct pvt_img_res_fps_pitch {
	uint32 width;
	uint32 height;
	uint32 fps;
	int32 luma_pitch;
	int32 chroma_pitch;
};

enum isp_focus_mode {
	ISP_FOCUS_MODE_SIMU
};
struct isp_exposure_mode {
	uint32 simu;
};
struct isp_whitebalance_mode {
	uint32 simu;
};

typedef struct _isp_af_sharpness_t {
	float value[3];
} isp_af_sharpness_t;

struct frame_ctl_info {
	int8 set_ae_mode;
	int8 set_itime;
	int8 set_flash_mode;
	int8 set_falsh_pwr;
	int8 set_ec;		/*EXPOSURE COMPENSATION */
	int8 set_iso;
	int8 set_af_mode;
	int8 set_af_lens_pos;

	AeMode_t ae_mode;
	AeExposure_t ae_para;
	FlashMode_t flash_mode;
	uint32 flash_pwr;
	AeEv_t ae_ec;
	AeIso_t iso;
	AfMode_t af_mode;
	uint32 af_lens_pos;
};

struct driver_settings {
	char rear_sensor[CAMERA_DEVICE_NAME_LEN];
	char rear_vcm[CAMERA_DEVICE_NAME_LEN];
	char rear_flash[CAMERA_DEVICE_NAME_LEN];
	char frontl_sensor[CAMERA_DEVICE_NAME_LEN];
	char frontr_sensor[CAMERA_DEVICE_NAME_LEN];

	/*Used for tuning, if it is bigger
	 *than actual sensor frame, then use the real one
	 */
	unsigned char sensor_frame_rate;

	/*-1: not set and use driver default setting
	 *0: RGB Bayer sensor
	 *1: RGB IR sensor
	 *2: IR senor
	 *3: mem sensor for loopback
	 *In most case, when none -1 value is set,
	 *it should be set together with rear_sensor
	 */
	char rear_sensor_type;

	/*-1: not set and use driver default setting
	 *0: RGB Bayer sensor
	 *1: RGB IR sensor
	 *2: IR senor
	 *3: mem sensor for loopback
	 *In most case, when none -1 value is set,
	 *it should be set together with frontl_sensor
	 */
	char frontl_sensor_type;

	/*-1: not set and use driver default setting
	 *0: RGB Bayer sensor
	 *1: RGB IR sensor
	 *2: IR senor
	 *3: mem sensor for loopback
	 *In most case, when none -1 value is set,
	 *it should be set together with frontr_sensor
	 */
	char frontr_sensor_type;

	/*-1: not set and use driver default setting
	 *0: set to manual mode
	 *1: set to auto mode
	 */
	char af_default_auto;

	/*-1: not set and use driver default setting
	 *0: set to manual mode
	 *1: set to auto mode
	 */
	char ae_default_auto;

	/*-1: not set and use driver default setting
	 *0: set to manual mode
	 *1: set to auto mode
	 */
	char awb_default_auto;

	/*-1: not set and use driver default setting
	 *0: driver control 3a
	 *1: fw control 3a
	 */
	char fw_ctrl_3a;

	/*-1: not set and use driver default setting
	 *0: set again by fw
	 *1: set again by sensor drv
	 */
	char set_again_by_sensor_drv;

	/*-1: not set and use driver default setting
	 *0: set itime by fw
	 *1: set itime by sensor drv
	 */
	char set_itime_by_sensor_drv;

	/*-1: not set and use driver default setting
	 *0: set lens position by fw
	 *1: set lens position by sensor drv
	 */
	char set_lens_pos_by_sensor_drv;

	/*-1: not set and use driver default setting
	 *0: not do the copy
	 *1: do the copy
	 */
	char do_fpga_to_sys_mem_cpy;

	/*0: not support
	 *others: support
	 */
	char hdmi_support_enable;

	/*These values used to set specific modules log dump
	 *at specific log levels(0~5).
	 *It can be sent at any time after the firmware is boot up.
	 *This extend log command can't be used with old log commands together
	 *for avoiding log level setting conflict.
	 *Currently, There are 33 FW modules and mod_ids aredefined in ModId_t.
	 *There are 5 log levels that are defined in LogLevel_t. 0x4 is invalid
	 *log level. it means off
	 */
	ulong fw_log_cfg_en;

	/*level_bits_A: Bit map A for log level of FW modules.
	 *4 bits represent one FW modules.
	 *Bit[4*(mod_id - 1) ~ (4*mod_id-1)] represents FW module
	 *refers to mod_id, When 1 <= mod_id <= 8.
	 */
	ulong fw_log_cfg_A;

	/*level_bits_B: Bit map B for log level of FW modules.
	 *4 bits represent one FW modules.
	 *Bit[4*(mod_id - 9) ~ (4*(mod_id - 9)+3)] represents FW module
	 *refers to mod_id, When 9 <= mod_id <= 16.
	 */
	ulong fw_log_cfg_B;

	/*level_bits_C: Bit map C for log level of FW modules.
	 *4 bits represent one FW modules.
	 *Bit[4*(mod_id - 17) ~ (4*(mod_id - 17)+3)] represents FW module
	 *refers to mod_id, When 17 <= mod_id <= 24.
	 */
	ulong fw_log_cfg_C;

	/*level_bits_D: Bit map D for log level of FW modules.
	 *4 bits represent one FW modules.
	 *Bit[4*(mod_id - 25) ~ (4*(mod_id - 25) +3)] represents FW module
	 *refers to mod_id, When 25 < mod_id <= 32.
	 */
	ulong fw_log_cfg_D;

	/*level_bits_E: Bit map E for log level of FW modules.
	 *4 bits represent one FW modules.
	 *Bit[4*(mod_id - 33) ~ (4*(mod_id - 33)+3)]
	 *represents FW module refers to mod_id, When 33 <= mod_id <= 40.
	 */
	ulong fw_log_cfg_E;

	/*-1: Do not use manual WB and fix the WB loghting index
	 *0-8: enable manual WB and fix to the WB loghting index
	 */
	char fix_wb_lighting_index;

	/*0: do not send the stage 2 recycling buffer
	 *1: send the stage 2 recycling buffer
	 */
	char enable_stage2_temp_buf;

	/*
	 *1: ignore meta at stage2 which means continue processing even
	 *without meta buf at
	 *stage 2.
	 *0: not ignore
	 */
	char ignore_meta_at_stage2;

	/*
	 *0: use psp to load fw.
	 *1: use driver to load fw even if it has been loaded by psp
	 *if psp loads fw fail, driver will load fw, this flag will be ingored.
	 */
	char driver_load_fw;

	/*
	 *0: enable dynamic aspect ratio wnd.
	 *1: disable dynamic aspect ratio wnd,
	 *   it will be equal to the sensor raw output
	 */
	char disable_dynamic_aspect_wnd;

	/*
	 *0: disable TNR
	 *1: enable TNR
	 */
	char tnr_enable;

	/*
	 *0: disable SNR
	 *1: enable SNR
	 */
	char snr_enable;

	/*
	 *0: disable variable frame rate for video pin
	 *1: enable variable frame rate for video pin
	 */
	char vfr_enable;

	/*
	 *0: disable the dumping
	 *bit0: 1 means to dump preview of rear camera;
	 *bit1: 1 means to dump video of rear camera;
	 *bit2: 1 means to dump zsl of rear camera;
	 *bit3: 1 means to dump raw of rear camera;
	 *bit8: 1 means to dump preview of front camera;
	 *bit9: 1 means to dump video of front camera;
	 *bit10: 1 means to dump zsl of front camera;
	 *bit11: 1 means to dump raw of front camera;
	 *bit16: 1 means to dump the metadata;
	 *The files will be located
	 *in c:\temp, please make sure to create this dir first
	 *before enable this flag
	 */
	uint32 dump_pin_output_to_file;

	/*
	 *0: disable high fps video
	 *1: enable high fps video
	 */
	char enable_high_video_capabilities;

	/*
	 *ROI settings
	 */
	Window_t rear_roi_af_wnd[3];
	Window_t rear_roi_ae_wnd;
	Window_t rear_roi_awb_wnd;

	Window_t frontl_roi_af_wnd[3];
	Window_t frontl_roi_ae_wnd;
	Window_t frontl_roi_awb_wnd;

	Window_t frontr_roi_af_wnd[3];
	Window_t frontr_roi_ae_wnd;
	Window_t frontr_roi_awb_wnd;

	char dpf_enable;

	/*
	 *0: disable the dumping
	 *bit0-1: reserved (as 0)
	 *bit2: 1 means to dump all other metadata;
	 *bit3: 1 means to dump Zoom metadata;
	 *bit4: 1 means to dump IR illuminator metadata;
	 *bit5: 1 means to dump White Balance related metadata;
	 *bit6: 1 means to dump Exposure related metadata;
	 *bit7: 1 means to dump Focus related metadata;
	 */
	uint32 output_metadata_in_log;

	uint32 demosaic_thresh;

	/*
	 *0: disable the frame drop for frame pair
	 *1: enable the frame drop for frame pair
	 */
	uint32 enable_frame_drop_solution_for_frame_pair;

	/*
	 *0: enable IR illuminator in Hello case
	 *1: disable IR illuminator in Hello case
	 */
	uint32 disable_ir_illuminator;

	/*
	 *0: always off;
	 *1: always on;
	 *2: normal, on/off pair
	 */
	uint32 ir_illuminator_ctrl;

	/*
	 *0: no filter on minimum AE ROI size;
	 *N: width x height;
	 */
	uint32 min_ae_roi_size_filter;

	/*
	 *0: ignore;
	 *1: reset AE ROI to default;
	 */
	char reset_small_ae_roi;

	/*
	 *0: process 3a by ISP fw, default;
	 *1: process 3a by host;
	 */
	char process_3a_by_host;

	/*fps at low light condition, default value is 15 */
	char low_light_fps;

	/*
	 *1: fw log only
	 *2: err
	 *3: warning
	 *4: information
	 *5: verbose
	 */
	char drv_log_level;

	/*
	 *index of scaler coefficients table idx from 0 - 9
	 */
	char scaler_param_tbl_idx;

	/*
	 *there are total 3 temp buffers,
	 *when enable enable_using_fb_for_taking_raw, driver will use the 3rd
	 *temp buffer as taking raw buffer and the first two
	 *as real temp buffer, because FW requires at least 3 temp
	 *buffers to run, so driver will send the 2 tmp buffers in following
	 *sequence 0,1,0,1 to make them seem to like 4 temp buffers.
	 *And the registry control is only valid
	 *when USING_PREALLOCATED_FB is defined.
	 */
	/*
	 *0: not enable
	 *others: enable
	 */
	char enable_using_fb_for_taking_raw;
};

struct isp_isp_module_if {

	/**
	 *the interface size;
	 */
	uint16 size;
	/**
	 *the interface version,
	 *its value will be (version_high<<16) | version_low,
	 *so the current version 1.0 will be (1<<16)|0
	 */
	uint16 version;

	/**
	 *the context of function call, it should be the first parameter
	 *of all function call in this interface.
	 */
	pvoid context;

	/*
	 *add following two member to comply with windows' interface definition
	 */
	void (*if_reference)(pvoid context);
	void (*if_dereference)(pvoid context);
	/**
	 *set fw binary.
	 */
	int32 (*set_fw_bin)(pvoid context, pvoid fw_data, uint32 fw_len);

	/**
	 *set calibration data binary.
	 */
	int32 (*set_calib_bin)(pvoid context, enum camera_id cam_id,
				pvoid calib_data, uint32 len);
	/**
	 *de-init ISP which includes stopping FW running, freeing temp buffer,
	 *powering down ISP and other
	 *possible de-initialization work.
	 */
	void (*de_init)(pvoid context);

	/**
	 *open a camera including sensor, VCM and flashlight as whole.
	 *@param cam_id
	 *indicate which camera to open:
	 *CAMERA_ID_REAR: rear camera for both integrate and discrete ISP
	 *CAMERA_ID_FRONT_LEFT: front left camera for both integrate and
	 *discrete ISP, it means front camera for discrete ISP
	 *CAMERA_ID_FRONT_RIGHT: front right camera only for integrate ISP
	 *CAMERA_ID_MEM: virtual mem camera whose raw data come from memory
	 *which loads from file
	 *@param res_fps_id
	 *index got from get_camera_res_fps,please refer to get_camera_res_fps,
	 *valid for CAMERA_ID_REAR,
	 *CAMERA_ID_FRONT_LEFT and CAMERA_ID_FRONT_RIGHT,
	 *for CAMERA_ID_MEM it means the width of the raw image
	 *@param flag
	 *Ored OPEN_CAMERA_FLAG_*, to indicate the open options,
	 *valid for CAMERA_ID_REAR,
	 *CAMERA_ID_FRONT_LEFT and CAMERA_ID_FRONT_RIGHT,
	 *for CAMERA_ID_MEM it means the height
	 *of the raw image
	 */
	int32 (*open_camera)(pvoid context, enum camera_id cam_id,
			uint32 res_fps_id, uint32_t flag);

	/**
	 *Close a camera including sensor, VCM and flashlight as whole.
	 *@param cam_id
	 *indicate which camera to open:
	 *0: rear camera for both integrate and discrete ISP
	 *1: front left camera for both integrate and discrete ISP,
		it means front camera for discrete ISP
	 *2: front right camera only for integrate ISP
	 *param res_fps_id
	 *index got from get_camera_res_fps,please refer to get_camera_res_fps
	 */
	int32 (*close_camera)(pvoid context, enum camera_id cam_id);

	/**
	 *get ISP HW capability, 0 for success others for fail.
	 *@param cap
	 *the returned ISP HW capability, it is defined as following
	 *struct AMD_ISP_ CAPABILITY {
	 *
	 *}
	 */
	int32 (*get_capabilities)(pvoid context,
				 struct isp_capabilities *cap);

	/**
	 *set preview buffer from OS to ISP FW/HW. return 0 for
	 *success others for fail.
	 *@param cam_id
	 *indicate which camera this buffer is for,
	 *its available values are:
	 *0: rear camera for both integrate and discrete ISP
	 *1: front left camera for both integrate and discrete ISP,
	 *   it means front camera for discrete ISP
	 *2: front right camera only for integrate ISP
	 *@param buf_hdl
	 *it contains the buffer information, please refer to section 6 for
	 *the detailed structure definition.
	 */
	int32 (*set_preview_buf)(pvoid context, enum camera_id cam_id,
				sys_img_buf_handle_t buf_hdl);

	/**
	 *set record buffer from OS to ISP FW/HW.
	 *return 0 for success others for fail.
	 *@param cam_id
	 *indicate which camera this buffer is for, its available values are:
	 *0: rear camera for both integrate and discrete ISP
	 *1: front left camera for both integrate and discrete ISP,
	 *   it means front camera for discrete ISP
	 *2: front right camera only for integrate ISP
	 *@param buf_hdl
	 *it contains the buffer information, please refer to section 6 for
	 *the detailed structure definition.
	 */
	int32 (*set_video_buf)(pvoid context, enum camera_id cam_id,
				sys_img_buf_handle_t buf_hdl);

	/**
	 *set still image buffer from OS to ISP FW/HW. return 0 for success
	 *others for fail.
	 *@param cam_id
	 *indicate which camera this buffer is for, its available values are:
	 *0: rear camera for both integrate and discrete ISP
	 *1: front left camera for both integrate and discrete ISP,
	 *   it means front camera for discrete ISP
	 *2: front right camera only for integrate ISP
	 *@param buf_hdl
	 *it contains the buffer information, please refer to section 6 for
	 *the detailed structure definition.
	 */
	int32 (*set_still_buf)(pvoid context, enum camera_id cam_id,
				sys_img_buf_handle_t buf_hdl);

	/**
	 *set common parameters for preview, still and record.
	 *return 0 for success others for fail.
	 *@param cam_id
	 *indicate which camera this parameter is for,
	 *its available values are:
	 *0: rear camera for both integrate and discrete ISP
	 *1: front left camera for both integrate and discrete ISP,
	 *it means front camera for discrete ISP
	 *2: front right camera only for integrate ISP
	 *@param para_id
	 *it indicates what parameter to set.
	 *@param para_value
	 *it indicates the parameter value to set. different parameter type
	 *will have different parameter value definition.
	 */
	int32 (*set_common_para)(void *context, enum camera_id cam_id,
				enum para_id para_id, void *para_value);

	/**
	 *get common parameters for preview, still and record.
	 *return 0 for success others for fail.
	 *@param cam_id
	 *indicate which camera this parameter is for,
	 *its available values are:
	 *0: rear camera for both integrate and discrete ISP
	 *1: front left camera for both integrate and discrete ISP,
	 *   it means front camera for discrete ISP
	 *2: front right camera only for integrate ISP
	 *@param para_id
	 *it indicates what parameter to gset.
	 *@param para_value
	 *it indicates the parameter value to get. different parameter type
	 *will have different parameter value definition.
	 */
	int32 (*get_common_para)(void *context, enum camera_id cam_id,
				enum para_id para_id, void *para_value);

	/**
	 *set parameter for preview. return 0 for success others for fail.
	 *refer to section 4 for all available parameters.
	 *@param cam_id
	 *indicate which camera this parameter is for,
	 *its available values are:
	 *0: rear camera for both integrate and discrete ISP
	 *1: front left camera for both integrate and discrete ISP,
	 *   it means front camera for discrete ISP
	 *2: front right camera only for integrate ISP
	 *@param para_type
	 *it indicates what parameter to set.
	 *@param para_value
	 *it indicates the parameter value to set. different parameter type
	 *will have different parameter value definition.
	 */
	int32 (*set_preview_para)(pvoid context, enum camera_id cam_id,
				enum para_id para_type, pvoid para_value);

	/**
	 *get parameter for preview. return 0 for success others negative value
	 *for fail. if para_value is NULL the function will return the buffer
	 *size needed to store the parameter value. refer to section 4 for all
	 *available parameters.
	 *@param cam_id
	 *indicate which camera this parameter is for,
	 *its available values are:
	 *0: rear camera for both integrate and discrete ISP
	 *1: front left camera for both integrate and discrete ISP,
	 *   it means front camera for discrete ISP
	 *2: front right camera only for integrate ISP
	 *@param para_type
	 *it indicates what parameter to get.
	 *@param para_value
	 *it indicates the parameter value to get. different parameter type
	 *will have different parameter value definition.
	 *@return 0: success others fail
	 */
	int32 (*get_prev_para)(pvoid context, enum camera_id cam_id,
				enum para_id para_type, pvoid para_value);

	/**
	 *set parameter for record. return 0 for success others for fail.
	 *refer to section 4 for all available parameters.
	 *@param cam_id
	 *indicate which camera this parameter is for,
	 *its available values are:
	 *0: rear camera for both integrate and discrete ISP
	 *1: front left camera for both integrate and discrete ISP,
	 *it means front camera for discrete ISP
	 *2: front right camera only for integrate ISP
	 *@param para_type
	 *it indicates what parameter to set.
	 *@param para_value
	 *it indicates the parameter value to set. different parameter type
	 *will have different parameter value definition.
	 */
	int32 (*set_video_para)(pvoid context, enum camera_id cam_id,
				 enum para_id para_type, pvoid para_value);

	/**
	 *get parameter for record. return 0 for success others negative value
	 *for fail. if para_value is NULL the function will return the buffer
	 *size needed to store the parameter value. refer to section 4 for
	 *all available parameters.
	 *@param cam_id
	 *indicate which camera this parameter is for,
	 *its available values are:
	 *0: rear camera for both integrate and discrete ISP
	 *1: front left camera for both integrate and discrete ISP,
	 *   it means front camera for discrete ISP
	 *2: front right camera only for integrate ISP
	 *@param para_type
	 *it indicates what parameter to get.
	 *@param para_value
	 *it indicates the parameter value to get. different parameter type
	 *will have different parameter value definition.
	 *@return 0: success others fail
	 */
	int32 (*get_video_para)(pvoid context, enum camera_id cam_id,
				 enum para_id para_type, pvoid para_value);

	/**
	 *set parameter for ir in rgbir. return 0 for success others for fail.
	 *refer to section 4 for all available parameters.
	 *@param cam_id
	 *indicate which camera this parameter is for,
	 *its available values are:
	 *0: rear camera for both integrate and discrete ISP
	 *1: front left camera for both integrate and discrete ISP,
	 *   it means front camera for discrete ISP
	 *2: front right camera only for integrate ISP
	 *@param para_type
	 *it indicates what parameter to set.
	 *@param para_value
	 *it indicates the parameter value to set. different parameter type
	 *will have different parameter value definition.
	 */
	int32 (*set_rgbir_ir_para)(pvoid context, enum camera_id cam_id,
				enum para_id para_type, pvoid para_value);

	/**
	 *get parameter for ir in rgbir. return 0 for success others negative
	 *value for fail. if para_value is NULL the function will return the
	 *buffer size needed to store the parameter value. refer to section 4
	 *for all available parameters.
	 *@param cam_id
	 *indicate which camera this parameter is for,
	 *its available values are:
	 *0: rear camera for both integrate and discrete ISP
	 *1: front left camera for both integrate and discrete ISP,
	 *   it means front camera for discrete ISP
	 *2: front right camera only for integrate ISP
	 *@param para_type
	 *it indicates what parameter to get.
	 *@param para_value
	 *it indicates the parameter value to get. different parameter type
	 *will have different parameter value definition.
	 *@return 0: success others fail
	 */
	int32 (*get_rgbir_ir_para)(pvoid context, enum camera_id cam_id,
				enum para_id para_type, pvoid para_value);

	/**
	 *set parameter for zsl. return 0 for success others for fail.
	 *refer to section 4 for all available parameters.
	 *@param cam_id
	 *indicate which camera this parameter is for,
	 *its available values are:
	 *0: rear camera for both integrate and discrete ISP
	 *1: front left camera for both integrate and discrete ISP,
	 *it means front camera for discrete ISP
	 *2: front right camera only for integrate ISP
	 *@param para_type
	 *it indicates what parameter to set.
	 *@param para_value
	 *it indicates the parameter value to set. different parameter type
	 *will have different parameter value definition.
	 */
	int32 (*set_zsl_para)(pvoid context, enum camera_id cam_id,
			enum para_id para_type, pvoid para_value);

	/**
	 *get parameter for zsl. return 0 for success others negative value for
	 *fail. if para_value is NULL the function will return the buffer size
	 *needed to store the parameter value. refer to section 4 for all
	 *available parameters.
	 *@param cam_id
	 *indicate which camera this parameter is for,
	 *its available values are:
	 *0: rear camera for both integrate and discrete ISP
	 *1: front left camera for both integrate and discrete ISP, it means
	 *   front camera for discrete ISP
	 *2: front right camera only for integrate ISP
	 *@param para_type
	 *it indicates what parameter to get.
	 *@param para_value
	 *it indicates the parameter value to set. different parameter type
	 *will have different parameter value definition.
	 *@param buf_len
	 *buffer size pointed by para_value
	 *@return 0: success others fail
	 */
	int32 (*get_zsl_para)(pvoid context, enum camera_id cam_id,
			 enum para_id para_type, pvoid para_value);

	/**
	 *start preview for cam_id, return 0 for success others for fail
	 */
	int32 (*start_preview)(pvoid context, enum camera_id cam_id);

	/**
	 *start record for cam_id, return 0 for success others for fail
	 */
	int32 (*start_video)(pvoid context, enum camera_id cam_id);

	/**
	 *start record for cam_id, return 0 for success others for fail
	 */
	int32 (*start_rgbir_ir)(pvoid context, enum camera_id cam_id);

	/**
	 *start capture still image for cam_id, return 0 for success others
	 *for fail
	 */
	int32 (*start_still)(pvoid context, enum camera_id cam_id);

	/**
	 *stop preview for cam_id, return 0 for success others for fail
	 */
	int32 (*stop_preview)(pvoid context, enum camera_id cam_id, bool pause);

	/**
	 *stop record for cam_id, return 0 for success others for fail
	 */
	int32 (*stop_video)(pvoid context, enum camera_id cam_id, bool pause);

	/**
	 *stop ir in rgbir, return 0 for success others for fail
	 */
	int32 (*stop_rgbir_ir)(pvoid context, enum camera_id cam_id);
	/**
	 *stop capture still image for cam_id, return 0 for
	 *success others for fail
	 */
	int32 (*stop_still)(pvoid context, enum camera_id cam_id);

	/**
	 *start auto focus for cam_id, return 0 for success others for fail
	 */
	int32 (*start_af)(pvoid context, enum camera_id cam_id);

	/**
	 *stop auto focus for cam_id, return 0 for success others for fail
	 */
	int32 (*stop_af)(pvoid context, enum camera_id cam_id);

	/**
	 *cancel auto focus for cam_id, return 0 for success others for fail
	 */
	int32 (*cancel_af)(pvoid context, enum camera_id cam_id);

	/**
	 *register callback functions for different events, function library
	 *will call these call back at appropriate time,
	 *these callback should return ASAP. no sleep or block wait should
	 *happen in these callbacks. the callback function
	 *prototype is defined as following:
	 *typedef int32 (*func_isp_module_cb)
	 *(pvoid context,enum cb_evt_id event_id, pvoideven_para);
	 */
	void (*reg_notify_cb)(pvoid context, enum camera_id cam_id,
			 func_isp_module_cb cb, pvoid cb_context);

	/**
	 *unregister callback functions for different events, *event_id is same
	 *as that in reg_notify_cb_imp,
	 *more details please refer to section 5.
	 *
	 */
	void (*unreg_notify_cb)(pvoid context, enum camera_id cam_id);

	/**
	 *read bytes from i2c device memory
	 *@param bus_num number of the i2c bus which to be used
	 *@param slave_addr address of slave to be accessed
	 *@param slave_addr_mode address mode of slave adderss,
	 *SLAVE_ADDR_MODE_7BIT or SLAVE_ADDR_MODE_10BIT
	 *@param reg_addr start address of the device memory to read
	 *@param reg_addr_size size of @ref reg_addr in bytes,
	 *valid range: 0..4 bytes.
	 *@param p_read_buffer pointer to local memory for
	 *holding the data being read
	 *@param byte_size amount of data to read
	 *@return result of operation
	 */
	int32 (*i2c_read_mem)(pvoid context, uint8 bus_num,
			uint16 slave_addr,
			i2c_slave_addr_mode_t slave_addr_mode,
			bool_t enable_restart, uint32 reg_addr,
			uint8 reg_addr_size, uint8 *p_read_buffer,
			uint32 byte_size);

	/**
	 *write bytes to i2c device memory
	 *@param bus_num number of the i2c bus which to be used
	 *@param slave_addr address of slave to be accessed
	 *@param slave_addr_mode address mode of slave adderss,
	 *SLAVE_ADDR_MODE_7BIT or SLAVE_ADDR_MODE_10BIT
	 *@param reg_addr start address of the device memory to read
	 *@param reg_addr_size size of @ref reg_addr in bytes,
	 *valid range: 0..4 bytes.
	 *@param p_read_buffer pointer to local memory holding the data
	 *to be written
	 *@param byte_size amount of data to be written
	 *@return result of operation
	 */
	int32 (*i2c_write_mem)(pvoid context, uint8 bus_num,
				uint16 slave_addr,
				i2c_slave_addr_mode_t slave_addr_mode,
				uint32 reg_addr, uint32 reg_addr_size,
				uint8 *p_write_buffer, uint32 byte_size);

	/**
	 *read bytes from i2c device register
	 *@param bus_num number of the i2c bus which to be used
	 *@param slave_addr address of slave to be accessed
	 *@param slave_addr_mode address mode of slave adderss,
	 *SLAVE_ADDR_MODE_7BIT or SLAVE_ADDR_MODE_10BIT
	 *@param reg_addr register address of the device memory to read
	 *@param reg_addr_size size of @ref reg_addr in bytes,
	 *valid range: 0..4 bytes.
	 *@param preg_value pointer to local memory for
	 *holding the register being read
	 *@param reg_size size of register in bytes, valid range: 1..4
	 *@return result of operation
	 */
	int32 (*i2c_read_reg)(pvoid context, uint8 bus_num,
			uint16 slave_addr,
			i2c_slave_addr_mode_t slave_addr_mode,
			bool_t enable_restart, uint32 reg_addr,
			uint8 reg_addr_size, void *preg_value,
			uint8 reg_size);

	/**
	 *write bytes to i2c device register
	 *@param bus_num number of the i2c bus which to be used
	 *@param slave_addr address of slave to be accessed
	 *@param slave_addr_mode address mode of slave adderss,
	 *SLAVE_ADDR_MODE_7BIT or SLAVE_ADDR_MODE_10BIT
	 *@param reg_addr register address of the device memory to read
	 *@param reg_addr_size size of @ref reg_addr in bytes,
	 *valid range: 0..4 bytes.
	 *@param reg_value register data to be written.
	 *@param reg_size size of register in bytes, valid range: 1..4
	 *@return result of operation
	 */

	int32 (*i2c_write_reg)(pvoid context, uint8 bus_num,
				uint16 slave_addr,
				i2c_slave_addr_mode_t slave_addr_mode,
				uint32 reg_addr, uint8 reg_addr_size,
				uint32 reg_value, uint8 reg_size);

	/**
	 *register sensor operation function
	 */
	void (*reg_snr_op)(pvoid context, enum camera_id cam_id,
			struct sensor_operation_t *ops);

	/**
	 *register vcm operation function
	 */
	void (*reg_vcm_op)(pvoid context, enum camera_id cam_id,
			struct vcm_operation_t *ops);

	/**
	 *register flash operation function
	 */
	void (*reg_flash_op)(pvoid context, enum camera_id cam_id,
			struct flash_operation_t *ops);

	/**
	 *unregister sensor operation function
	 */
	void (*unreg_snr_op)(pvoid context, enum camera_id cam_id);

	/**
	 *unregister vcm operation function
	 */
	void (*unreg_vcm_op)(pvoid context, enum camera_id cam_id);

	/**
	 *unregister flash operation function
	 */
	void (*unreg_flash_op)(pvoid context, enum camera_id cam_id);

	int32 (*set_flash_mode)(pvoid context, enum camera_id cam_id,
				 FlashMode_t mode);

	int32 (*set_flash_power)(pvoid context, enum camera_id cam_id,
				uint32 pwr);

	int32 (*set_red_eye_mode)(pvoid context, enum camera_id cam_id,
				RedEyeMode_t mode);

	int32 (*set_flash_assist_mode)(pvoid context, enum camera_id cam_id,
					FocusAssistMode_t mode);

	int32 (*set_flash_assist_power)(pvoid context, enum camera_id cam_id,
					uint32 pwr);

	/**
	 *take one picture. return 0 for success others for fail.
	 *@param cam_id
	 *indicate which camera picture is taken from
	 *@param para
	 *it contains the parameters of the pic to be taken.
	 *@param buf
	 *it contains the buffer to fill the pic.
	 */
	int32 (*take_one_pic)(pvoid context, enum camera_id cam_id,
			struct take_one_pic_para *para,
			sys_img_buf_handle_t buf);

	/**
	 *stop take one picture. return 0 for success others for fail.
	 */
	int32 (*stop_take_one_pic)(pvoid context, enum camera_id cam_id);

	/**
	 *set one photo sequence. return 0 for success others for fail.
	 *@param cam_id
	 *indicate which camera picture is taken from
	 *@param main_jpg_buf
	 *it contains the buffer to fill main jpeg data.
	 *@param main_yuv_buf
	 *it contains the buffer to fill nv12 data corresponding
	 *to main jpeg data.
	 *@param jpg_thumb_buf
	 *it contains the buffer to fill jpeg thumbnail data.
	 *@param yuv_thumb_buf
	 *it contains the buffer to fill nv12 thumbnail data.
	 */
	int32 (*set_one_photo_seq_buf)(pvoid context, enum camera_id cam_id,
					sys_img_buf_handle_t main_jpg_buf,
					sys_img_buf_handle_t main_yuv_buf,
					sys_img_buf_handle_t jpg_thumb_buf,
					sys_img_buf_handle_t yuv_thumb_buf);

	/**
	 *start photo sequence. return 0 for success others for fail.
	 *@param cam_id
	 *indicate which camera to start
	 *@param fps
	 *fps of photo sequence.
	 *@param task_bits
	 *ored value of enum pic_task_flag to indicate what output is needed
	 *it should match the buffer set by set_one_photo_seq_buf
	 */
	int32 (*start_photo_seq)(pvoid context, enum camera_id cam_id,
				uint32 fps, uint32 task_bits);

	/**
	 *stop photo sequence. return 0 for success others for fail.
	 *@param cam_id
	 *indicate which camera to stop
	 */
	int32 (*stop_photo_seq)(pvoid context, enum camera_id cam_id);

	/**
	 *write isp register
	 */
	void (*isp_reg_write32)(pvoid context, uint64 reg_addr, uint32 value);

	/**
	 *read isp register
	 */
	 uint32 (*isp_reg_read32)(pvoid context, uint64 reg_addr);

	/**
	 *set power of sensor. return 0 for success others for fail.
	 *@param cam_id
	 *indicate which sensor to set power
	 *@param ana_core
	 *analog core power in m_v,
	 *if the pointer is NULL then no need to set it
	 *@param dig_core
	 *digital core power m_v, if the pointer is NULL then no need to set it
	 *@param dig_sel
	 *digital selectable power m_v, if the pointer is NULL then no need to
	 *set it
	 *
	 *int32 (*snr_pwr_set)(pvoid context, enum camera_id cam_id,
	 *			uint16 *ana_core, uint16 *dig_core,
	 *			uint16 *dig_sel);
	 */

	/**
	 *set clock of sensor. return 0 for success others for fail.
	 *@param cam_id
	 *indicate which sensor to set clk
	 *@param clk in k_h_z
	 *clk, 0 to off others to set
	 */
	int32 (*snr_clk_set)(pvoid context, enum camera_id cam_id,
			uint32 clk /*in k_h_z */);

	/**
	 *get all resolution and fps a sensor support.
	 *@param cam_id
	 *indicate which sensor get
	 *@return all resolution and fps a sensor support or NULL for fail.
	 *@remark this function must be called after reg_snr_op
	 *when calling open_camera, caller should provide an index of
	 *res_fps@sensor_res_fps_t to
	 *indicate which resolution and fps sensor should run at
	 */
	struct sensor_res_fps_t *(*get_camera_res_fps)(pvoid context,
							enum camera_id cam_id);

	/**
	 *enable VS.
	 *@param cam_id
	 *indicate which sensor
	 *@param str_id
	 *indicate which stream
	 *
	 *bool_t (*enable_vs)(pvoid context, enum camera_id cam_id,
	 *		enum stream_id str_id);
	 */
	/**
	 *disable VS.
	 *@param cam_id
	 *indicate which sensor
	 *@param str_id
	 *indicate which stream
	 *
	 *bool_t (*disable_vs)(pvoid context, enum camera_id cam_id,
	 *		 enum stream_id str_id);
	 */
	/**
	 *take one picture. return 0 for success others for fail.
	 *@param cam_id
	 *indicate which camera picture is taken from
	 *@param raw_pic_buf
	 *it contains the buffer to fill the raw data.
	 *@param use_precapture
	 *if use precapture.
	 */
	int32 (*take_raw_pic)(pvoid context, enum camera_id cam_id,
			sys_img_buf_handle_t raw_pic_buf,
			uint32 use_precapture);

	/**
	 *stop take one raw picture. return 0 for success others for fail.
	 */
	int32 (*stop_take_raw_pic)(pvoid context, enum camera_id cam_id);

	/**
	 *set focus mode.
	 *@param cam_id
	 *indicate which camera
	 *@param mode
	 *it contains the mode to set.
	 */
	int32 (*set_focus_mode)(pvoid context, enum camera_id cam_id,
				 AfMode_t mode);

	/**
	 *auto focus lock.
	 *@param cam_id
	 *indicate which camera
	 *@param lock_mode
	 *the lock mode
	 */
	int32 (*auto_focus_lock)(pvoid context, enum camera_id cam_id,
				LockType_t lock_mode);

	/**
	 *auto focus unlock.
	 *@param cam_id
	 *indicate which camera
	 */
	int32 (*auto_focus_unlock)(pvoid context, enum camera_id cam_id);

	/**
	 *auto exposure lock.
	 *@param cam_id
	 *indicate which camera
	 *@param lock_mode
	 *the lock mode
	 */
	int32 (*auto_exposure_lock)(pvoid context, enum camera_id cam_id,
				LockType_t lock_mode);

	/**
	 *auto exposure unlock.
	 *@param cam_id
	 *indicate which camera
	 */
	int32 (*auto_exposure_unlock)(pvoid context, enum camera_id cam_id);

	/**
	 *auto white balance lock.
	 *@param cam_id
	 *indicate which camera
	 *@param lock_mode
	 *the lock mode
	 */
	int32 (*auto_wb_lock)(pvoid context, enum camera_id cam_id,
			LockType_t lock_mode);

	/**
	 *auto white balance unlock.
	 *@param cam_id
	 *indicate which camera
	 */
	int32 (*auto_wb_unlock)(pvoid context, enum camera_id cam_id);

	/**
	 *set lens position.
	 *@param cam_id
	 *indicate which camera
	 *@param pos
	 *pos to set [0,1024]
	 */
	int32 (*set_lens_position)(pvoid context, enum camera_id cam_id,
				uint32 pos);

	/**
	 *get focus position.
	 *@param cam_id
	 *indicate which camera
	 *@param pos
	 *store the returned pos
	 */
	int32 (*get_focus_position)(pvoid context, enum camera_id cam_id,
				uint32 *pos);

	/**
	 *set exposure mode.
	 *@param cam_id
	 *indicate which camera
	 *@param mode
	 *it contains the mode to set.
	 */
	int32 (*set_exposure_mode)(pvoid context, enum camera_id cam_id,
				AeMode_t mode);

	/**
	 *start auto exposure for cam_id, return 0 for success others for fail
	 */
	int32 (*start_ae)(pvoid context, enum camera_id cam_id);

	/**
	 *stop auto exposure for cam_id, return 0 for success others for fail
	 */
	int32 (*stop_ae)(pvoid context, enum camera_id cam_id);

	/**
	 *set white balance mode.
	 *@param cam_id
	 *indicate which camera
	 *@param mode
	 *it contains the mode to set.
	 */
	int32 (*set_wb_mode)(pvoid context, enum camera_id cam_id,
			AwbMode_t mode);

	/**
	 *start white balance for cam_id, return 0 for success others for fail
	 */
	int32 (*start_wb)(pvoid context, enum camera_id cam_id);

	/**
	 *stop white balance for cam_id, return 0 for success others for fail
	 */
	int32 (*stop_wb)(pvoid context, enum camera_id cam_id);

	/**
	 *set white balance light.
	 *@param cam_id
	 *indicate which camera
	 *@param light
	 *the light, should be light index
	 */
	int32 (*set_wb_light)(pvoid context, enum camera_id cam_id,
			uint32 light_idx);

	/**
	 *set sensor analog gain.
	 *@param cam_id
	 *indicate which camera
	 *@param ana_gain
	 *it contains the analog gain value multiplied by 1000
	 */
	int32 (*set_snr_ana_gain)(pvoid context, enum camera_id cam_id,
				uint32 ana_gain);

	/**
	 *set sensor digital gain.
	 *@param cam_id
	 *indicate which camera
	 *@param ana_gain
	 *it contains the digital gain
	 */
	int32 (*set_snr_dig_gain)(pvoid context, enum camera_id cam_id,
				uint32 dig_gain);

	/**
	 *get sensor analog gain.
	 *@param cam_id
	 *indicate which camera
	 *@param ana_gain
	 *it stores the analog gain value multiplied by 1000
	 *@param dig_gain
	 *it stores the digital gain value multiplied by 1000
	 */
	int32 (*get_snr_gain)(pvoid context, enum camera_id cam_id,
			uint32 *ana_gain, uint32 *dig_gain);

	/**
	 *get sensor analog gain capability.
	 *@param cam_id
	 *indicate which camera
	 *@param min
	 *it stores the mininal gain value multiplied by 1000
	 *@param max
	 *it stores the maximum gain value multiplied by 1000
	 *@param step
	 *it stores the step gain value multiplied by 1000
	 */
	int32 (*get_snr_gain_cap)(pvoid context, enum camera_id cam_id,
				struct para_capability_u32 *ana,
				struct para_capability_u32 *dig);

	/**
	 *set sensor integreation time.
	 *@param cam_id
	 *indicate which camera
	 *@param itime
	 *the integration time in microsecond to set
	 */
	int32 (*set_snr_itime)(pvoid context, enum camera_id cam_id,
				uint32 itime);

	/**
	 *get sensor integreation time.
	 *@param cam_id
	 *indicate which camera
	 *@param itime
	 *it stores the integration time in microsecond to set
	 */
	int32 (*get_snr_itime)(pvoid context, enum camera_id cam_id,
				uint32 *itime);

	/**
	 *get sensor integration time capability.
	 *@param cam_id
	 *indicate which camera
	 *@param min
	 *it stores the mininal itime in microsecond
	 *@param max
	 *it stores the maximum itime in microsecond
	 *@param step
	 *it stores the step itime in microsecond
	 */
	int32 (*get_snr_itime_cap)(pvoid context, enum camera_id cam_id,
				uint32 *min, uint32 *max, uint32 *step);

	/**
	 *register fw parser.
	 *@param parser
	 *parser
	 */
	void (*reg_fw_parser)(pvoid context, enum camera_id cam_id,
			struct fw_parser_operation_t *parser);

	/**
	 *unregister fw parser.
	 */
	void (*unreg_fw_parser)(pvoid context, enum camera_id cam_id);

	/**
	 *set the global value in script
	 *@param cam_id
	 *indicate which camera
	 *@param type
	 *indicate what kind of value to set
	 *@param gv
	 *value to be set
	 */
	int32 (*set_fw_gv)(pvoid context, enum camera_id id,
			enum fw_gv_type type, ScriptFuncArg_t *gv);

	/**
	 *set the digital zoom
	 *@param cam_id
	 *indicate which camera
	 *@param h_off
	 *horizontal offset of the region to be zoomed
	 *@param v_off
	 *virtical offset of the region to be zoomed
	 *@param width
	 *width of the region to be zoomed
	 *@param height
	 *height of the region to be zoomed
	 */
	int32 (*set_digital_zoom)(pvoid context, enum camera_id id,
				uint32 h_off, uint32 v_off,
				uint32 width, uint32 height);

	/**
	 *set the digital zoom ratio multi by 100, so 100 means 1
	 *@param cam_id
	 *indicate which camera
	 *@param zoom
	 *numerator of ratio
	 */
	int32 (*set_digital_zoom_ratio)(pvoid context, enum camera_id id,
					uint32 zoom);

	/**
	 *set the digital zoom pos
	 *@param cam_id
	 *indicate which camera
	 *@param h_off
	 *horizontal offset of the region to be zoomed
	 *@param v_off
	 *virtical offset of the region to be zoomed
	 */
	int32 (*set_digital_zoom_pos)(pvoid context, enum camera_id id,
				int16 h_off, int16 v_off);

	/**
	 *get the raw image type
	 *@param cam_id
	 *indicate which camera
	 *@param type
	 *store the raw image type
	 */
	int32 (*get_snr_raw_img_type)(pvoid context, enum camera_id id,
				enum raw_image_type *type);

	/**
	 *set the iso
	 *@param cam_id
	 *indicate which camera
	 *@param value
	 *the iso value
	 */
	int32 (*set_iso_value)(pvoid context, enum camera_id id,
				AeIso_t value);

	/**
	 *set the EV compensation
	 *@param cam_id
	 *indicate which camera
	 *@param value
	 *the ev value
	 */
	int32 (*set_ev_value)(pvoid context, enum camera_id id, AeEv_t value);

	/**
	 *get the sharpness
	 *@param cam_id
	 *indicate which camera
	 *@param sharp
	 *the sharpness value
	 *
	 *int32 (*get_sharpness)(pvoid context, enum camera_id id,
	 *			isp_af_sharpness_t *sharp);
	 */
	/**
	 *set the color effect
	 *@param cam_id
	 *indicate which camera
	 *@param ce
	 *the color effect value
	 *
	 *int32 (*set_color_effect)(pvoid context, enum camera_id id,
	 *			isp_ie_config_t *ie);
	 */
	/**
	 *disable the color effect
	 *@param cam_id
	 *indicate which camera
	 */
	int32 (*disable_color_effect)(pvoid context, enum camera_id cam_id);

	/**
	 *set zsl buffer from OS to ISP FW/HW. return 0 for success others
	 *for fail.
	 *@param cam_id
	 *indicate which camera this buffer is for, its available values are:
	 *0: rear camera for both integrate and discrete ISP
	 *1: front left camera for both integrate and discrete ISP,
	 *it means front camera for discrete ISP
	 *2: front right camera only for integrate ISP
	 *@param buf_hdl
	 *it contains the buffer information, please refer to section 6 for
	 *the detailed structure definition.
	 */
	int32 (*set_zsl_buf)(pvoid context, enum camera_id cam_id,
			sys_img_buf_handle_t buf_hdl);
	/**
	 *start zsl for cam_id, return 0 for success others for fail
	 */
	int32 (*start_zsl)(pvoid context, enum camera_id cam_id,
			bool is_perframe_ctl);

	/**
	 *stop zsl for cam_id, return 0 for success others for fail
	 */
	int32 (*stop_zsl)(pvoid context, enum camera_id cam_id, bool pause);

	/**
	 *take one picture. return 0 for success others for fail.
	 *@param cam_id
	 *indicate which camera picture is taken from, not used
	 *@param in_yuy2_buf
	 *conotains the input parameter and yuv data used to do post jpeg
	 *@param main_jpg_buf
	 *it contains the buffer to fill main jpeg data.
	 *@param jpg_thumb_buf
	 *it contains the buffer to fill jpeg thumbnail data.
	 *@param yuv_thumb_buf
	 *it contains the buffer to fill nv12 thumbnail data.
	 */
	int32 (*take_one_pp_jpeg)(pvoid context, enum camera_id cam_id,
/*			struct isp_pp_jpeg_input *pp_input,*/
			sys_img_buf_handle_t in_yuy2_buf,
			sys_img_buf_handle_t main_jpg_buf,
			sys_img_buf_handle_t jpg_thumb_buf,
			sys_img_buf_handle_t yuv_thumb_buf);

	void (*test_gfx_interface)(pvoid context);

	/**
	 *set meta buffer from OS to ISP FW/HW. return 0 for success others
	 *for fail.
	 *@param cam_id
	 *indicate which camera this buffer is for, its available values are:
	 *0: rear camera for both integrate and discrete ISP
	 *1: front left camera for both integrate and discrete ISP,
	 *   it means front camera for discrete ISP
	 *2: front right camera only for integrate ISP
	 *@param buf_hdl
	 *it contains the buffer information, please refer to section 6 for
	 *the detailed structure definition.
	 */
	int32 (*set_metadata_buf)(pvoid context, enum camera_id cam_id,
				sys_img_buf_handle_t buf_hdl);

	/**
	 *start metadata for cam_id, return 0 for success others for fail
	 */
	int32 (*start_metadata)(pvoid context, enum camera_id cam_id);

	/**
	 *stop metadata for cam_id, return 0 for success others for fail
	 */
	int32 (*stop_metadata)(pvoid context, enum camera_id cam_id);

	/**
	 *get camera raw type, return 0 for success others for fail
	 *@param raw_bayer_pattern
	 *indicate raw bayer pattern
	 *1:RGGB, 2:GRBG, 3:GBRG, BGGR:4,
	 */
	int32 (*get_camera_raw_type)(pvoid context, enum camera_id cam_id,
				enum raw_image_type *raw_type,
				uint32 *raw_bayer_pattern);
	/**
	 *get camera devices info, return 0 for success others for fail
	 */
	int32 (*get_camera_dev_info)(pvoid context, enum camera_id cam_id,
				struct camera_dev_info *cam_dev_info);

	/**
	 *set awb roi. return 0 for success others for fail.
	 *@param cam_id
	 *indicate which sensor
	 *@param window
	 *indicate the window
	 */
	int32 (*set_awb_roi)(pvoid context, enum camera_id cam_id,
			window_t *window);

	/**
	 *set ae roi. return 0 for success others for fail.
	 *@param cam_id
	 *indicate which sensor
	 *@param window
	 *indicate the window
	 */
	int32 (*set_ae_roi)(pvoid context, enum camera_id cam_id,
				window_t *window);

	/**
	 *set af roi. return 0 for success others for fail.
	 *@param cam_id
	 *indicate which sensor
	 *@param window
	 *indicate the window, max 3 windows to be set,
	 *window[i] == NULL means no ith windows
	 */
	int32 (*set_af_roi)(pvoid context, enum camera_id cam_id,
			AfWindowId_t id, window_t *window);

	/**
	 *set scene mode. return 0 for success others for fail.
	 *@param cam_id
	 *indicate which sensor
	 *@param mode
	 *indicate the the scene
	 */
	int32 (*set_scene_mode)(pvoid context, enum camera_id cam_id,
				enum isp_scene_mode mode);

	/**
	 *set gamma. return 0 for success others for fail.
	 *@param cam_id
	 *indicate which sensor
	 */
	int32 (*set_gamma)(pvoid context, enum camera_id cam_id,
				int32 value);

	/**
	 *set color temperature. return 0 for success others for fail.
	 *@param cam_id
	 *indicate which sensor
	 */
	int32 (*set_color_temperature)(pvoid context,
			enum camera_id cam_id, uint32 value);

	/**
	 *set per sensor calibration data. return 0 for success others
	 *for fail.
	 *@param cam_id
	 *indicate which sensor
	 *@param data
	 *calibration data
	 *@param len
	 *calibration data len
	 */
	int32 (*set_snr_calib_data)(pvoid context, enum camera_id cam_id,
				pvoid data, uint32 len);

	/**
	 *Map a vram surface handle to vram address. return 0 for success
	 *others for fail.
	 *@param handle
	 *the vram handle
	 *@param vram_addr
	 *store the mapped address
	 */
	int32 (*map_handle_to_vram_addr)(pvoid context, void *handle,
					uint64 *vram_addr);
	/**
	 *Set per frame control info together with zsl buffer. return 0
	 *for success others for fail.
	 *@param ctl_info
	 *the per frame control info
	 *@param zsl_buf
	 *the zsl buffer
	 */
	int32 (*set_per_frame_ctl_info)(pvoid context, enum camera_id cam_id,
					 struct frame_ctl_info *ctl_info,
					 sys_img_buf_handle_t zsl_buf);

	/**
	 *Set rgb and ir raw buffer. return 0 for success
	 *others for fail.
	 *@param bayer_raw
	 *the rgb bayer raw buffer
	 *@param ir_raw
	 *the ir raw buffer
	 */
	int32 (*set_rgbir_raw_buf)(pvoid context, enum camera_id cam_id,
				sys_img_buf_handle_t bayer_raw,
				sys_img_buf_handle_t ir_raw);

	/**
	 *Set frame info and rgb bayer raw input, 0 for success
	 *others for fail.
	 *@param frame info
	 *the frame_info
	 *@param bayer_raw
	 *the input bayer raw data.
	 *@remark upper layer will first call set_rgbir_raw_buf to set bayer
	 *and ir raw buffer to isp module, after filled with raw data,
	 *isp module will return them to upper layer
	 *together with Mode3FrameInfo_t in callback(CB_EVT_ID_FRAME_INFO),
	 *then upper layer can call
	 *set_rgb_bayer_frame_info to send the Mode3FrameInfo_t and
	 *filled bayer raw down again to do post process to generate
	 *preview/video/zsl output. It is up to upperlayer to deciede
	 *if it will do any modifications to Mode3FrameInfo_t and
	 *bayer raw before sending them down. so cam_id in this call must be
	 *the same as cam_id in set_rgbir_raw_buf and the camera must be rg
	 */
	int32 (*set_rgb_bayer_frame_info)(pvoid context, enum camera_id cam_id,
					Mode3FrameInfo_t *frame_info,
					sys_img_buf_handle_t bayer_raw);

	/**
	 *Set frame info and ir raw input, 0 for success
	 *others for fail.
	 *@param cam_id
	 *it must be CAMERA_ID_MEM
	 *@param frame_info
	 *the frame info
	 *@param ir_raw
	 *the input ir raw data.
	 *@remark the most common scenario is upper layer first calls
	 *set_rgbir_raw_buf to set bayer and ir raw buffer to isp module, after
	 *filled with raw data, isp module will return them to upper layer
	 *together with Mode3FrameInfo_t in callback(CB_EVT_ID_FRAME_INFO),
	 *then upper layer can call
	 *set_ir_frame_info to send a Mode4FrameInfo_t (which is derived from
	 *Mode3FrameInfo_t) and filled ir raw down to do post process to
	 *generate preview/video/zsl output. It is up to
	 *upperlayer to deciede if it will do any modifications to ir raw
	 *before sending it down. Before any operations to this camera,
	 *it must be opend first by
	 *calling open_camera with cam_id set to CAMERA_ID_MEM
	 */
	int32 (*set_ir_frame_info)(pvoid context, enum camera_id cam_id,
				Mode4FrameInfo_t *frame_info,
				sys_img_buf_handle_t ir_raw);

	/**
	 *Set driver setting. return 0 for success
	 *others for fail.
	 */
	int32 (*set_drv_settings)(pvoid context,
				struct driver_settings *setting);

	/**
	 *Enable dynamic frame rate. return 0 for success
	 *others for fail.
	 *@param enable
	 *1 to enable and 0 to disable
	 */
	int32 (*enable_dynamic_frame_rate)(pvoid context,
					enum camera_id cam_id, bool enable);

	/**
	 *Set the max frame rate of a stream. return 0 for success
	 *others for fail.
	 *@comment
	 *this call must be called before calling start_preview,
	 *start_video and start_still
	 */
	int32 (*set_max_frame_rate)(pvoid context, enum camera_id cam_id,
				enum stream_id sid,
				uint32 frame_rate_numerator,
				uint32 frame_rate_denominator);

	/**
	 *Set flicker. return 0 for success
	 *others for fail.
	 */
	int32 (*set_flicker)(pvoid context, enum camera_id cam_id,
			enum anti_banding_mode anti_banding);
	/**
	 *Set iso priority. return 0 for success
	 *others for fail.
	 */
	int32 (*set_iso_priority)(pvoid context, enum camera_id cam_id,
			enum stream_id sid, AeIso_t *iso);

	/**
	 *Set ev compensation. return 0 for success
	 *others for fail.
	 */
	int32 (*set_ev_compensation)(pvoid context, enum camera_id cam_id,
			AeEv_t *ev);

	/**
	 *set lens range.
	 *@param cam_id
	 *indicate which camera
	 *@comment
	 *[near_value, far_value] should be in the range of [0,1024]
	 */
	int32 (*set_lens_range)(pvoid context, enum camera_id cam_id,
				 uint32 near_value, uint32 far_value);

	/**
	 *set video hdr .
	 *@param cam_id
	 *indicate which camera
	 */
	int32 (*set_video_hdr)(pvoid context, enum camera_id cam_id,
				uint32 enable);

	/**
	 *set video hdr .
	 *@param cam_id
	 *indicate which camera
	 */
	int32 (*set_photo_hdr)(pvoid context, enum camera_id cam_id,
				uint32 enable);
	/**
	 *set lens shading matrix .
	 *@param cam_id
	 *indicate which camera
	 */
	int32 (*set_lens_shading_matrix)(pvoid context, enum camera_id cam_id,
				LscMatrix_t *matrix);

	/**
	 *set lens shading sector .
	 *@param cam_id
	 *indicate which camera
	 */
	int32 (*set_lens_shading_sector)(pvoid context, enum camera_id cam_id,
				PrepLscSector_t *sectors);
	/**
	 *set AWB cc matrix command.
	 *@param cam_id
	 *indicate which camera
	 */
	int32 (*set_awb_cc_matrix)(pvoid context, enum camera_id cam_id,
				CcMatrix_t *matrix);

	/**
	 *set AWB cc matrix offset.
	 *@param cam_id
	 *indicate which camera
	 */
	int32 (*set_awb_cc_offset)(pvoid context, enum camera_id cam_id,
				CcOffset_t *ccOffset);
	/**
	 *gamma outenable/disable .
	 *@param cam_id
	 *indicate which camera
	 *@param enable
	 *1 to enable 0 to disable
	 */
	int32 (*gamma_enable)(pvoid context, enum camera_id cam_id,
				uint32 enable);

	/**
	 *WDR enable/disable .
	 *@param cam_id
	 *indicate which camera
	 *@param enable
	 *1 to enable 0 to disable
	 */
	int32 (*wdr_enable)(pvoid context, enum camera_id cam_id,
				uint32 enable);

	/**
	 *TNR enable/disable .
	 *@param cam_id
	 *indicate which camera
	 *@param enable
	 *1 to enable 0 to disable
	 */
	int32 (*tnr_enable)(pvoid context, enum camera_id cam_id,
				uint32 enable);

	/**
	 *SNR enable/disable .
	 *@param cam_id
	 *indicate which camera
	 *@param enable
	 *1 to enable 0 to disable
	 */
	int32 (*snr_enable)(pvoid context, enum camera_id cam_id,
				uint32 enable);

	/**
	 *DPF enable/disable .
	 *@param cam_id
	 *indicate which camera
	 *@param enable
	 *1 to enable 0 to disable
	 */
	int32 (*dpf_enable)(pvoid context, enum camera_id cam_id,
				uint32 enable);

	/**
	 *start RGBIR .
	 */
	int32 (*start_rgbir)(pvoid context, enum camera_id cam_id);

	/**
	 *stop RGBIR .
	 */
	int32 (*stop_rgbir)(pvoid context, enum camera_id cam_id);

	/**
	 *Set RGBIR output buff, which will be used to store split bayer raw,
	 *ir raw and frame info
	 */
	int32 (*set_rgbir_output_buf)(pvoid context, enum camera_id cam_id,
				sys_img_buf_handle_t buf_hdl,
				uint32 bayer_raw_len, uint32 ir_raw_len,
				uint32 frame_info_len);

	/**
	 *Set RGBIR input buff
	 *@param bayer_raw_buf
	 *the rgbbayer raw as input
	 *@param frame_info
	 *the Mode3FrameInfo_t typed frame info as input
	 */
	int32 (*set_rgbir_input_buf)(pvoid context, enum camera_id cam_id,
				sys_img_buf_handle_t bayer_raw_buf,
				sys_img_buf_handle_t frame_info);

	/**
	 *Set mem camera input buff
	 *@param bayer_raw_buf
	 *the rgbbayer raw as input
	 *@param frame_info
	 *the Mode4FrameInfo_t typed frame info as input
	 */
	int32 (*set_mem_cam_input_buf)(pvoid context, enum camera_id cam_id,
				sys_img_buf_handle_t bayer_raw_buf,
				sys_img_buf_handle_t frame_info);

	/**
	 *get the original windows size of camera cam_id
	 *@param width
	 *to store the width of the windows
	 *@param height
	 *to store the height of the windows
	 */
	int32 (*get_original_wnd_size)(pvoid context, enum camera_id cam_id,
				uint32 *width, uint32 *height);

	/**
	 *get the color process status of camera cam_id
	 *@param cproc
	 *to store the result of color process
	 */
	int32 (*get_cproc_status)(pvoid context, enum camera_id cam_id,
				CprocStatus_t *cproc);

	/**
	 *set the color process enablement of camera cam_id
	 *@param enable
	 *1 is enable, 0 is disable
	 */
	int32 (*cproc_enable)(pvoid context, enum camera_id cam_id,
			uint32 enable);

	/**
	 *set contrast of camera cam_id
	 */
	int32 (*cproc_set_contrast)(pvoid context, enum camera_id cam_id,
			uint32 contrast);
	/**
	 *set saturation of camera cam_id
	 */
	int32 (*cproc_set_saturation)(pvoid context, enum camera_id cam_id,
			uint32 saturation);

	/**
	 *set hue of camera cam_id
	 */
	int32 (*cproc_set_hue)(pvoid context, enum camera_id cam_id,
			uint32 hue);

	/**
	 *set brightness of camera cam_id
	 */
	int32 (*cproc_set_brightness)(pvoid context, enum camera_id cam_id,
			uint32 brightness);


	/**
	 *get isp moudle init parameter called by KMD
	 *@param sw_init
	 *struct isp_sw_init_input *typed, to store the sw_init para
	 *@param hw_init
	 *struct isp_hw_init_input *typed, to store the hw_init para
	 *@param isp_env
	 *enum isp_environment_setting typed, to stroe the actual environment
	 */
	int32 (*get_init_para)(pvoid context,
				void *sw_init,
				void *hw_init,
				void *isp_env
	);
	/**
	 *reserve local frame buffer
	 *@param size
	 *size the length the frame buffer
	 *@param sys
	 *store the sys address
	 *@param mc
	 *store the mc address
	 */
	int32 (*lfb_resv)(pvoid context, uint32 size,
				uint64 *sys, uint64 *mc);

	/**
	 *free the reserved local frame buffer
	 */
	int32 (*lfb_free)(pvoid context);

	/**
	 *set parameter for rgbir raw. return 0 for success others for fail.
	 *refer to section 4 for all available parameters.
	 *@param cam_id
	 *indicate which camera this parameter is for,
	 *its available values are:
	 *0: rear camera for both integrate and discrete ISP
	 *1: front left camera for both integrate and discrete ISP,
	 *   it means front camera for discrete ISP
	 *2: front right camera only for integrate ISP
	 *@param para_type
	 *it indicates what parameter to set.
	 *@param para_value
	 *it indicates the parameter value to set. different parameter type
	 *will have different parameter value definition.
	 */
	int32 (*set_rgbir_raw_para)(pvoid context, enum camera_id cam_id,
				enum para_id para_type, pvoid para_value);

	/**
	 *get parameter for rgbir raw. return 0 for success others negative
	 *value for fail. if para_value is NULL the function will return the
	 *buffer size needed to store the parameter value.
	 *refer to section 4 for all available parameters.
	 *@param cam_id
	 *indicate which camera this parameter is for,
	 *its available values are:
	 *0: rear camera for both integrate and discrete ISP
	 *1: front left camera for both integrate and discrete ISP,
	 *   it means front camera for discrete ISP
	 *2: front right camera only for integrate ISP
	 *@param para_type
	 *it indicates what parameter to get.
	 *@param para_value
	 *it indicates the parameter value to set. different parameter type
	 *will have different parameter value definition.
	 *@param buf_len
	 *buffer size pointed by para_value
	 *@return 0: success others fail
	 */
	int32 (*get_rgbir_raw_para)(pvoid context, enum camera_id cam_id,
				enum para_id para_type, pvoid para_value);

	/**
	 *get raw image info.
	 *@param cam_id
	 *indicate which camera this parameter is for,
	 *its available values are:
	 *0: rear camera for both integrate and discrete ISP
	 *1: front left camera for both integrate and discrete ISP,
	 *   it means front camera for discrete ISP
	 *2: front right camera only for integrate ISP
	 *@param w
	 *used to store the width of raw image.
	 *@param h
	 *used to store the height of raw image.
	 *@param raw_type
	 *used to store the raw type of the raw image,
	 *0: 8bit, 1: 10bit, 2: 12bit
	 *@param raw_pkt_fmt
	 *used to store the raw package format of the raw image,
	 *its value is from RawPktFmt_t
	 *@return 0: success others fail
	 */
	int32 (*get_raw_img_info)(pvoid context, enum camera_id cam_id,
				uint32 *w, uint32 *h, uint32 *raw_type,
				uint32 *raw_pkt_fmt);

	/**
	 *send fw command
	 */
	int32 (*fw_cmd_send)(pvoid context, enum camera_id cam_id,
				uint32 cmd, uint32 is_set_cmd,
				uint16 is_stream_cmd, uint16 to_ir,
				uint32 is_direct_cmd,
				pvoid para, uint32 *para_len);

	/**
	 *get tuning data info.
	 *@param cam_id
	 *indicate which camera this parameter is for,
	 *its available values are:
	 *0: rear camera for both integrate and discrete ISP
	 *1: front left camera for both integrate and discrete ISP,
	 *   it means front camera for discrete ISP
	 *2: front right camera only for integrate ISP
	 *@param struct_ver
	 *used to store the struct version of the tuning data.
	 *@param date
	 *used to store the date of the tuning data.
	 *@param author
	 *used to store the author name of the tuning data,
	 *should at least 32 bytes
	 *@param cam_name
	 *used to store the camera name of the tuning data,
	 *should at least 32 bytes
	 *@return 0: success others fail
	 */
	int32 (*get_tuning_data_info)(pvoid context, enum camera_id cam_id,
					uint32 *struct_ver, uint32 *date,
					char *author, char *cam_name);

	/**
	 *read bytes from i2c device register by fw
	 *@param bus_num number of the i2c bus which to be used
	 *@param slave_addr address of slave to be accessed
	 *@param slave_addr_mode address mode of slave adderss,
	 *SLAVE_ADDR_MODE_7BIT or SLAVE_ADDR_MODE_10BIT
	 *@param reg_addr register address of the device memory to read
	 *@param reg_addr_size size of @ref reg_addr in bytes,
	 *valid range: 0..4 bytes.
	 *@param preg_value pointer to local memory for
	 *holding the register being read
	 *@param reg_size size of register in bytes, valid range: 1..4
	 *@return result of operation
	 **/
	int32 (*i2c_read_reg_fw)(pvoid context, uint32 id,
				uint8 bus_num, uint16 slave_addr,
				i2c_slave_addr_mode_t slave_addr_mode,
				bool_t enable_restart, uint32 reg_addr,
				uint8 reg_addr_size, void *preg_value,
				uint8 reg_size);

	/**
	 *write bytes to i2c device register by firaware
	 *@param bus_num number of the i2c bus which to be used
	 *@param slave_addr address of slave to be accessed
	 *@param slave_addr_mode address mode of slave adderss,
	 *SLAVE_ADDR_MODE_7BIT or SLAVE_ADDR_MODE_10BIT
	 *@param reg_addr register address of the device memory to read
	 *@param reg_addr_size size of @ref reg_addr in bytes,
	 *valid range: 0..4 bytes.
	 *@param reg_value register data to be written.
	 *@param reg_size size of register in bytes, valid range: 1..4
	 *@return result of operation
	 **/
	int32 (*i2c_write_reg_fw)(pvoid context, uint32 id,
				uint8 bus_num, uint16 slave_addr,
				i2c_slave_addr_mode_t slave_addr_mode,
				uint32 reg_addr, uint8 reg_addr_size,
				uint32 reg_value, uint8 reg_size);
	/**
	 *start loopback.
	 *@param cam_id
	 *indicate which camera this loopback will use
	 *0: rear camera for both integrate and discrete ISP
	 *1: front left camera for both integrate and discrete ISP,
	 *   it means front camera for discrete ISP
	 *@param yuv_format
	 *the format of output yuv image.
	 *@param yuv_width
	 *width of output yuv image.
	 *@param yuv_height
	 *height of output yuv image
	 *@param yuv_ypitch
	 *y pitch of output yuv image
	 *@param yuv_uvpitch
	 *uv pitch of output yuv image
	 *@return 0: success others fail
	 */
	int32 (*loopback_start)(pvoid context, enum camera_id cam_id,
				uint32 yuv_format, uint32 yuv_width,
				uint32 yuv_height, uint32 yuv_ypitch,
				uint32 yuv_uvpitch);
	/**
	 *stop loopback.
	 *@param cam_id
	 *indicate which camera this loopback will use
	 *0: rear camera for both integrate and discrete ISP
	 *1: front left camera for both integrate and discrete ISP,
	 *   it means front camera for discrete ISP
	 *@return 0: success others fail
	 */
	int32 (*loopback_stop)(pvoid context, enum camera_id cam_id);

	/**
	 *set raw image together with frame info in loopback case.
	 *@param cam_id
	 *indicate which camera this loopback will use
	 *0: rear camera for both integrate and discrete ISP
	 *1: front left camera for both integrate and discrete ISP,
	 *   it means front camera for discrete ISP
	 *@param raw_buf
	 *the raw as input
	 *@param frame_info
	 *the Mode4FrameInfo_t typed frame info as input
	 *@return 0: success others fail
	 */
	int32 (*loopback_set_raw)(pvoid context, enum camera_id cam_id,
				sys_img_buf_handle_t raw_buf,
				sys_img_buf_handle_t frame_info);

	/**
	 *Process raw image set by loopback_set_raw to generate
	 *return yuv image and meta data
	 *@param cam_id
	 *indicate which camera this loopback will use
	 *0: rear camera for both integrate and discrete ISP
	 *1: front left camera for both integrate and discrete ISP,
	 *   it means front camera for discrete ISP
	 *@param sub_tuning_cmd_count
	 *subjective tuning cmd count pointed by sub_tuning_cmd
	 *@param sub_tuning_cmd
	 *the subjective tuning command
	 *@param buf
	 *the buffer to store yuv image and metadata, the layout is yuv+meta
	 *@param yuv_buf_len
	 *the yuv image buffer len in buf
	 *@param meta_buf_len
	 *the meta buffer len in buf, should >= sizeof(MetaInfo_t)
	 *@return 0: success others fail
	 */
	int32 (*loopback_process_raw)(pvoid context, enum camera_id cam_id,
				uint32 sub_tuning_cmd_count,
				struct loopback_sub_tuning_cmd *
				sub_tuning_cmd, sys_img_buf_handle_t buf,
				uint32 yuv_buf_len, uint32 meta_buf_len);

	/**
	 *set the face authtication mode
	 *@param cam_id
	 *indicate which camera
	 *@param value
	 *the face authtication mode
	 */
	int32 (*set_face_authtication_mode)(pvoid context, enum camera_id id,
					bool_t mode);
	/**
	 *get the work buf info which is pre allocated in swISP
	 *@param work_buf_info
	 *store the buffer info actuall it's struct isp_work_buf *
	 */
	int32 (*get_isp_work_buf)(pvoid context, pvoid work_buf_info);
};

#endif
