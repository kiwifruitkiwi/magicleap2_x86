/*
 * Copyright (C) 2019-2023 Advanced Micro Devices, Inc. All rights reserved.
 */

#ifndef ISP_MODULE_INTERFACE_H
#define ISP_MODULE_INTERFACE_H

#include "os_base_type.h"
#include "isp_cam_if.h"
#include <linux/cam_api.h>

#define CAMERA_DEVICE_NAME_LEN 16

/*
 *Set this flag to open camera in HDR mode,
 *otherwise camera will be opened in normal mode
 */
#define OPEN_CAMERA_FLAG_HDR 0x00000001
#define CAMERA_FLAG_NOT_ACCESS_DEV 0x00000002

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

enum _i2c_slave_addr_mode_t {
	I2C_SLAVE_ADDR_MODE_7BIT,
	I2C_SLAVE_ADDR_MODE_10BIT
};

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
	STREAM_ID_RAW,
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
	CB_EVT_ID_RAW_BUF_DONE,	//param struct buf_cb_para
	CB_EVT_ID_NOTIFICATION,	//param struct notify_cb_para
	CB_EVT_ID_FRAME_CTRL_DONE,	//param is the mc addr of _MetaInfo_t
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
	PARA_ID_COLOR_TEMPERATURE,	/*unsigned int done */
	PARA_ID_FLICKER_MODE,	/*pvt_flicker_mode done */
	PARA_ID_COLOR_ENABLE,	/*pvt_int32done */
	PARA_ID_GAMMA,		/*int*done */
	PARA_ID_ISO,		/*struct _AeIso_t **/
	PARA_ID_EV,		/*struct _AeEv_t **/
	PARA_ID_MAIN_JPEG_COMPRESS_RATE,	/*unsigned int**/
	PARA_ID_THBNAIL_JPEG_COMPRESS_RATE,	/*unsigned int**/

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

enum img_buf_type {
	IMG_BUF_TYPE_INVALID,
	IMG_BUF_TYPE_SYS,
	IMG_BUF_TYPE_PHY,//for fpga dma
	IMG_BUF_TYPE_VRAM//for capture VRAM case
};

#ifndef MAX_LOOPBACK_SUB_TUNING_CMD_PARAM_LEN
#define MAX_LOOPBACK_SUB_TUNING_CMD_PARAM_LEN 3072
#endif

struct loopback_sub_tuning_cmd {	//same as LOOPBACK_SUB_TUNING_CMD
	//fw command CMD_ID_**from cmdresp.h
	unsigned int fw_cmd;
	//if @CmdChannel in command parameter description is StreamX,
	//then set to 1 else 0
	unsigned int is_stream_channel;
	//if @ParameterType in command parameter description is Direct,
	//then set to 1 else 0
	unsigned int is_direct;
	unsigned int param_len;
	//sizeof(struct _LscMatrix_t) is 2312
	//sizeof(struct _PrepLscSector_t) is 96
	//sizeof(DegammaCurve_t) is 118
	//sizeof(WdrCurve_t) is 132
	unsigned char param[MAX_LOOPBACK_SUB_TUNING_CMD_PARAM_LEN];
};

struct camera_dev_info {
	char cam_name[CAMERA_DEVICE_NAME_LEN];
	char vcm_name[CAMERA_DEVICE_NAME_LEN];
	char flash_name[CAMERA_DEVICE_NAME_LEN];
	char storage_name[CAMERA_DEVICE_NAME_LEN];
	enum camera_type type;
	int focus_len;
};

struct photo_seq_capability {
	unsigned short max_frame_history;
	unsigned short max_fps;
};

struct para_capability {
	short min;
	short max;
	short step;
	short def;		/*default value for this parameter */
};

struct para_capability_u32 {
	unsigned int min;
	unsigned int max;
	unsigned int step;
	unsigned int def;		/*default value for this parameter */
};

struct isp_capabilities {
//	AeEvCapability_t ev_compensation[CAMERA_ID_MAX];
	struct para_capability brightness;
	struct para_capability contrast;
	struct para_capability gamma;
	struct para_capability hue;
	struct para_capability saturation;
	struct para_capability sharpness;
//	AeIsoCapability_t iso[CAMERA_ID_MAX];
	struct para_capability zoom;	/*in the unit of 0.01 */
	struct para_capability zoom_h_off;
	struct para_capability zoom_v_off;
	/*real gain value multiple 1000 */
	struct para_capability_u32 gain;
	/*real integration time multiple 1000000 */
	struct para_capability_u32 itime;
	/*struct photo_seq_capability photo_seq_cap[CAMERA_ID_MAX]; */
};

struct physical_address {
	unsigned long long phy_addr;
	unsigned int cnt;
	unsigned int align;
};

struct sys_img_buf_handle {
	void *virtual_addr;	/*virtual address of the buffer */
	unsigned long long physical_addr;	/*physical address of buffer */
	unsigned long long handle;		/*dma buffer fd for linux */
	void *private_buffer_info;	/*see below */
	/*buffer length for IMG_BUF_TYPE_SYS,
	 *phy_sg_tbl entry count for IMG_BUF_TYPE_PHY
	 */
	unsigned int len;
	enum img_buf_type buf_type;
	struct physical_address *phy_sg_tbl;
};

/*
 *struct isp_pp_jpeg_input {
 *	//only PVT_IMG_FMT_YUV422_INTERLEAVED valid now
 *	enum pvt_img_fmt input_fmt;
 *	unsigned int width;
 *	unsigned int height;
 *	unsigned int y_stride;
 *	unsigned int v_stride;
 *	struct sys_img_buf_handle *in_buf;
 * };
 */

struct take_one_pic_para {
	enum pvt_img_fmt fmt;
	unsigned int width;
	unsigned int height;
	unsigned int luma_pitch;
	unsigned int chroma_pitch;
	unsigned int use_precapture;
	unsigned int focus_first;
};

struct buf_cb_para {
	struct sys_img_buf_handle *bf_hdl;
	unsigned int cam_id;
	unsigned int poc;
	unsigned int finish_status;	/*0 for success; */
	unsigned int width;		/*add for zsl */
	unsigned int height;		/*add for zsl */
	unsigned int y_stride;	/*add for zsl */
	unsigned int uv_stride;	/*add for zsl */
};	/*for CB_EVT_ID_PREV_BUF_DONE, CB_EVT_ID_REC_BUF_DONE */

struct buf_done_info {
	enum buf_done_status status;
	struct sys_img_buf_handle buf;
};

struct buf_done_info_ex {
	enum buf_done_status status;
	struct sys_img_buf_handle buf;
	unsigned int width;
	unsigned int height;
	enum raw_image_type type;
	unsigned int raw_img_format;
	enum _CFAPattern_t cfa_pattern;
};

/*call back parameter for CB_EVT_ID_FRAME_INFO*/
struct frame_info_cb_para {
	unsigned int cam_id;
	struct sys_img_buf_handle buf;
	unsigned int bayer_raw_buf_len;
	unsigned int ir_raw_buf_len;
	unsigned int frame_info_buf_len;

};

enum isp_cmd_id {
	ISP_CMD_ID_SET_RGBIR_INPUT_BUF,
	ISP_CMD_ID_SET_MEM_CAMERA_INPUT_BUF
};

/*call back parameter for CB_EVT_ID_CMD_DONE*/
struct cmd_done_cb_para {
	enum isp_cmd_id cmd;
	unsigned int cam_id;
	unsigned int status;		/*0 for success others for fail */
	union {
		/*for ISP_CMD_ID_SET_RGBIR_INPUT_BUF and
		 *ISP_CMD_ID_SET_MEM_CAMERA_INPUT_BUF
		 */
		struct sys_img_buf_handle buf;
	} para;
//struct sys_img_buf_handle *;
};

/*call back parameter for CB_EVT_ID_FRAME_DONE*/
struct frame_done_cb_para {
	unsigned int poc;
	unsigned int cam_id;
	long long timeStamp;
	struct buf_done_info preview;
	struct buf_done_info video;
	struct buf_done_info zsl;
	struct buf_done_info_ex raw;
	//struct _Window_t zoomWindow;
	//AeMetaInfo_t ae;
	//AfMetaInfo_t af;
	//AwbMetaInfo_t awb;
	//FlashMetaInfo_t flash;
	struct _MetaInfo_t metaInfo;
	unsigned int scene_mode;
	unsigned int zoom_factor;
};

struct meta_databuf_cb_para {
	struct sys_img_buf_handle *bf_hdl;
};	/*for CB_EVT_ID_METADATA_DONE */

struct take_one_pic_cb_para {
	struct sys_img_buf_handle *main_jpg_buf;
	/*for pp jpeg, main_yuv_buf will store the original input yuv buffer */
	struct sys_img_buf_handle *main_yuv_buf;
	struct sys_img_buf_handle *jpeg_thumbnail_buf;
	struct sys_img_buf_handle *yuv_thumb_buf;
	unsigned int cam_id;
	unsigned int flag;
	unsigned int finish_status;	/*0 for success; */
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
	struct sys_img_buf_handle *raw_data_buf;
	unsigned int cam_id;
	unsigned int finish_status;	/*0 for success; */
	enum raw_image_type img_type;
};	/*for CB_EVT_ID_TAKE_RAW_PIC_DONE */

struct para_set_cb_para {
	enum para_id para;
	enum camera_id cam_id;
	enum stream_id str_id;
	unsigned int set_result;	/*0 for success; */
};

struct photo_seq_cb_para {
	enum camera_id cam_id;
};

typedef int(*func_isp_module_cb) (void *context, enum cb_evt_id event,
		void *param, enum camera_id cid);

/*****************************************************************
 *description : this parameter value type is used for
 *brightness/contrast/hue/saturation
 *para id: PARA_ID_BRIGHTNESS/PARA_ID_CONTRAST
 *PARA_ID_HUE/PARA_ID_SATURATION
 ****************************************************************
 */
struct pvt_int32 {
	int value;
};


struct pvt_flicker_mode {
	enum _AeFlickerType_t value;
};

/*4208x3120//13M*/
struct pvt_img_size {
	unsigned int width;
	unsigned int height;
};

struct pvt_img_res_fps_pitch {
	unsigned int width;
	unsigned int height;
	unsigned int fps;
	int luma_pitch;
	int chroma_pitch;
};

enum isp_focus_mode {
	ISP_FOCUS_MODE_SIMU
};
struct isp_exposure_mode {
	unsigned int simu;
};
struct isp_whitebalance_mode {
	unsigned int simu;
};

struct _isp_af_sharpness_t {
	float value[3];
};

struct frame_ctl_info {
	char set_ae_mode;
	char set_itime;
	char set_flash_mode;
	char set_falsh_pwr;
	char set_ec;		/*EXPOSURE COMPENSATION */
	char set_iso;
	char set_af_mode;
	char set_af_lens_pos;

	enum _AeMode_t ae_mode;
	struct _AeExposure_t ae_para;
//	FlashMode_t flash_mode;
	unsigned int flash_pwr;
	struct _AeEv_t ae_ec;
	struct _AeIso_t iso;
	enum _AfMode_t af_mode;
	unsigned int af_lens_pos;
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
	 *0: set dgain by fw
	 *1: set dgain by sensor drv
	 */
	char set_dgain_by_sensor_drv;

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
	 *Currently, There are 33 FW modules and mod_ids aredefined in
	 *enum _ModId_t.
	 *There are 5 log levels that are defined in enum _LogLevel_t. 0x4
	 *is invalid
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

	/*level_bits_E: Bit map E for log level of FW modules.
	 *4 bits represent one FW modules.
	 *Bit[4*(mod_id - 35) ~ (4*(mod_id - 41)+3)]
	 *represents FW module refers to mod_id, When 33 <= mod_id <= 40.
	 */
	ulong fw_log_cfg_F;

	/*level_bits_E: Bit map E for log level of FW modules.
	 *4 bits represent one FW modules.
	 *Bit[4*(mod_id - 42) ~ (4*(mod_id - 49)+3)]
	 *represents FW module refers to mod_id, When 33 <= mod_id <= 40.
	 */
	ulong fw_log_cfg_G;

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
	unsigned int dump_pin_output_to_file;

	/*
	 *0: disable high fps video
	 *1: enable high fps video
	 */
	char enable_high_video_capabilities;

	/*
	 *ROI settings
	 */
	struct _Window_t rear_roi_af_wnd[3];
	struct _Window_t rear_roi_ae_wnd;
	struct _Window_t rear_roi_awb_wnd;

	struct _Window_t frontl_roi_af_wnd[3];
	struct _Window_t frontl_roi_ae_wnd;
	struct _Window_t frontl_roi_awb_wnd;

	struct _Window_t frontr_roi_af_wnd[3];
	struct _Window_t frontr_roi_ae_wnd;
	struct _Window_t frontr_roi_awb_wnd;

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
	unsigned int output_metadata_in_log;

	unsigned int demosaic_thresh;

	/*
	 *0: disable the frame drop for frame pair
	 *1: enable the frame drop for frame pair
	 */
	unsigned int enable_frame_drop_solution_for_frame_pair;

	/*
	 *0: enable IR illuminator in Hello case
	 *1: disable IR illuminator in Hello case
	 */
	unsigned int disable_ir_illuminator;

	/*
	 *0: always off;
	 *1: always on;
	 *2: normal, on/off pair
	 */
	unsigned int ir_illuminator_ctrl;

	/*
	 *0: no filter on minimum AE ROI size;
	 *N: width x height;
	 */
	unsigned int min_ae_roi_size_filter;

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
	unsigned short size;
	/**
	 *the interface version,
	 *its value will be (version_high<<16) | version_low,
	 *so the current version 1.0 will be (1<<16)|0
	 */
	unsigned short version;

	/**
	 *the context of function call, it should be the first parameter
	 *of all function call in this interface.
	 */
	void *context;

	/*
	 *add following two member to comply with windows' interface definition
	 */
	void (*if_reference)(void *context);
	void (*if_dereference)(void *context);
	/**
	 *set fw binary.
	 */
	int (*set_fw_bin)(void *context, void *fw_data, unsigned int fw_len);

	/**
	 *set calibration data binary.
	 */
	int (*set_calib_bin)(void *context, enum camera_id cam_id,
				void *calib_data, unsigned int len);
	/**
	 *de-init ISP which includes stopping FW running, freeing temp buffer,
	 *powering down ISP and other
	 *possible de-initialization work.
	 */
	void (*de_init)(void *context);

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
	int (*open_camera)(void *context, enum camera_id cam_id,
			unsigned int res_fps_id, uint32_t flag);


	int (*open_camera_fps)(void *context, enum camera_id cam_id,
			unsigned int fps, uint32_t flag);

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
	int (*close_camera)(void *context, enum camera_id cam_id);

	/**
	 *get ISP HW capability, 0 for success others for fail.
	 *@param cap
	 *the returned ISP HW capability, it is defined as following
	 *struct AMD_ISP_ CAPABILITY {
	 *
	 *}
	 */
	int (*get_capabilities)(void *context,
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
	int (*set_preview_buf)(void *context, enum camera_id cam_id,
				struct sys_img_buf_handle *buf_hdl);

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
	int (*set_video_buf)(void *context, enum camera_id cam_id,
				struct sys_img_buf_handle *buf_hdl);

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
	int (*set_still_buf)(void *context, enum camera_id cam_id,
				struct sys_img_buf_handle *buf_hdl);

	/**
	 *set frame control from OS to ISP FW/HW. return 0 for success
	 *others for fail.
	 *@param cam_id
	 *indicate which camera this buffer is for, its available values are:
	 *0: rear camera for both integrate and discrete ISP
	 *1: front left camera for both integrate and discrete ISP,
	 *   it means front camera for discrete ISP
	 *2: front right camera only for integrate ISP
	 *@param frm_ctrl
	 *the frame control info
	 */
	int (*set_frame_ctrl)(void *context, enum camera_id cam_id,
			      struct _CmdFrameCtrl_t *frm_ctrl);

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
	int (*set_common_para)(void *context, enum camera_id cam_id,
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
	int (*get_common_para)(void *context, enum camera_id cam_id,
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
	int (*set_preview_para)(void *context, enum camera_id cam_id,
				enum para_id para_type, void *para_value);

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
	int (*get_prev_para)(void *context, enum camera_id cam_id,
				enum para_id para_type, void *para_value);

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
	int (*set_video_para)(void *context, enum camera_id cam_id,
				 enum para_id para_type, void *para_value);

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
	int (*get_video_para)(void *context, enum camera_id cam_id,
				 enum para_id para_type, void *para_value);


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
	int (*set_zsl_para)(void *context, enum camera_id cam_id,
			enum para_id para_type, void *para_value);

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
	int (*get_zsl_para)(void *context, enum camera_id cam_id,
			 enum para_id para_type, void *para_value);

	/**
	 *start preview for cam_id, return 0 for success others for fail
	 */
	int (*start_preview)(void *context, enum camera_id cam_id);

	/**
	 *start record for cam_id, return 0 for success others for fail
	 */
	int (*start_video)(void *context, enum camera_id cam_id);

	/**
	 *set test pattern for cam_id, return 0 for success others for fail
	 */
	int (*set_test_pattern)(void *context, enum camera_id cam_id,
				void *param);

	/**
	 *switch sensor profile for cam_id, return 0 for success others for fail
	 */
	int (*switch_profile)(void *context, enum camera_id cam_id,
			      unsigned int prf_id);

	/**
	 *start for cam_id, return 0 for success others for fail
	 */
	int (*start_stream)(void *context, enum camera_id cam_id);

	/**
	 *start capture still image for cam_id, return 0 for success others
	 *for fail
	 */
	int (*start_still)(void *context, enum camera_id cam_id);

	/**
	 *stop preview for cam_id, return 0 for success others for fail
	 */
	int (*stop_preview)(void *context, enum camera_id cam_id, bool pause);

	/**
	 *stop record for cam_id, return 0 for success others for fail
	 */
	int (*stop_video)(void *context, enum camera_id cam_id, bool pause);

	/**
	 *stop capture still image for cam_id, return 0 for
	 *success others for fail
	 */
	int (*stop_still)(void *context, enum camera_id cam_id);

	/**
	 *register callback functions for different events, function library
	 *will call these call back at appropriate time,
	 *these callback should return ASAP. no sleep or block wait should
	 *happen in these callbacks. the callback function
	 *prototype is defined as following:
	 *typedef int (*func_isp_module_cb)
	 *(void *context,enum cb_evt_id event_id, pvoideven_para);
	 */
	void (*reg_notify_cb)(void *context, enum camera_id cam_id,
			 func_isp_module_cb cb, void *cb_context);

	/**
	 *unregister callback functions for different events, *event_id is same
	 *as that in reg_notify_cb_imp,
	 *more details please refer to section 5.
	 *
	 */
	void (*unreg_notify_cb)(void *context, enum camera_id cam_id);

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
	int (*i2c_read_mem)(void *context, unsigned char bus_num,
			unsigned short slave_addr,
			enum _i2c_slave_addr_mode_t slave_addr_mode,
			int enable_restart, unsigned int reg_addr,
			unsigned char reg_addr_size,
			unsigned char *p_read_buffer,
			unsigned int byte_size);

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
	int (*i2c_write_mem)(void *context, unsigned char bus_num,
				unsigned short slave_addr,
				enum _i2c_slave_addr_mode_t slave_addr_mode,
				unsigned int reg_addr,
				unsigned int reg_addr_size,
				unsigned char *p_write_buffer,
				unsigned int byte_size);

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
	int (*i2c_read_reg)(void *context, unsigned char bus_num,
			unsigned short slave_addr,
			enum _i2c_slave_addr_mode_t slave_addr_mode,
			int enable_restart, unsigned int reg_addr,
			unsigned char reg_addr_size, void *preg_value,
			unsigned char reg_size);

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

	int (*i2c_write_reg)(void *context, unsigned char bus_num,
				unsigned short slave_addr,
				enum _i2c_slave_addr_mode_t slave_addr_mode,
				unsigned int reg_addr,
				unsigned char reg_addr_size,
				unsigned int reg_value,
				unsigned char reg_size);

	/**
	 *register sensor operation function
	 */
	void (*reg_snr_op)(void *context, enum camera_id cam_id,
			struct sensor_operation_t *ops);

	/**
	 *register vcm operation function
	 */

	//void (*reg_vcm_op)(void *context, enum camera_id cam_id,
		//	struct vcm_operation_t *ops);

	/**
	 *register flash operation function
	 */
	//void (*reg_flash_op)(void *context, enum camera_id cam_id,
			//struct flash_operation_t *ops);

	/**
	 *unregister sensor operation function
	 */
	void (*unreg_snr_op)(void *context, enum camera_id cam_id);

	/**
	 *unregister vcm operation function
	 */
	//void (*unreg_vcm_op)(void *context, enum camera_id cam_id);

	/**
	 *unregister flash operation function
	 */

	/**
	 *take one picture. return 0 for success others for fail.
	 *@param cam_id
	 *indicate which camera picture is taken from
	 *@param para
	 *it contains the parameters of the pic to be taken.
	 *@param buf
	 *it contains the buffer to fill the pic.
	 */
	int (*take_one_pic)(void *context, enum camera_id cam_id,
			struct take_one_pic_para *para,
			struct sys_img_buf_handle *buf);

	/**
	 *stop take one picture. return 0 for success others for fail.
	 */
	int (*stop_take_one_pic)(void *context, enum camera_id cam_id);

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
	int (*set_one_photo_seq_buf)(void *context, enum camera_id cam_id,
			struct sys_img_buf_handle *main_jpg_buf,
			struct sys_img_buf_handle *main_yuv_buf,
			struct sys_img_buf_handle *jpg_thumb_buf,
			struct sys_img_buf_handle *yuv_thumb_buf);

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
	int (*start_photo_seq)(void *context, enum camera_id cam_id,
				unsigned int fps, unsigned int task_bits);

	/**
	 *stop photo sequence. return 0 for success others for fail.
	 *@param cam_id
	 *indicate which camera to stop
	 */
	int (*stop_photo_seq)(void *context, enum camera_id cam_id);

	/**
	 *write isp register
	 */
	void (*isp_reg_write32)(void *context, unsigned long long reg_addr,
				unsigned int value);

	/**
	 *read isp register
	 */
	unsigned int (*isp_reg_read32)(void *context,
				unsigned long long reg_addr);

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
	 *int (*snr_pwr_set)(void *context, enum camera_id cam_id,
	.*		unsigned short *ana_core, unsigned short *dig_core,
	.*		unsigned short *dig_sel);
	 */

	/**
	 *set clock of sensor. return 0 for success others for fail.
	 *@param cam_id
	 *indicate which sensor to set clk
	 *@param clk in k_h_z
	 *clk, 0 to off others to set
	 */
	int (*snr_clk_set)(void *context, enum camera_id cam_id,
			unsigned int clk /*in k_h_z */);

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
	struct sensor_res_fps_t *(*get_camera_res_fps)(void *context,
					enum camera_id cam_id);

	/**
	 *enable VS.
	 *@param cam_id
	 *indicate which sensor
	 *@param str_id
	 *indicate which stream
	 *
	 *int (*enable_vs)(void *context, enum camera_id cam_id,
	 *		enum stream_id str_id);
	 */
	/**
	 *disable VS.
	 *@param cam_id
	 *indicate which sensor
	 *@param str_id
	 *indicate which stream
	 *
	 *int (*disable_vs)(void *context, enum camera_id cam_id,
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
	int (*take_raw_pic)(void *context, enum camera_id cam_id,
			struct sys_img_buf_handle *raw_pic_buf,
			unsigned int use_precapture);

	/**
	 *stop take one raw picture. return 0 for success others for fail.
	 */
	int (*stop_take_raw_pic)(void *context, enum camera_id cam_id);

	/**
	 *auto exposure lock.
	 *@param cam_id
	 *indicate which camera
	 *@param lock_mode
	 *the lock mode
	 */
	int (*auto_exposure_lock)(void *context, enum camera_id cam_id,
				enum _LockType_t lock_mode);

	/**
	 *auto exposure unlock.
	 *@param cam_id
	 *indicate which camera
	 */
	int (*auto_exposure_unlock)(void *context, enum camera_id cam_id);

	/**
	 *auto white balance lock.
	 *@param cam_id
	 *indicate which camera
	 *@param lock_mode
	 *the lock mode
	 */
	int (*auto_wb_lock)(void *context, enum camera_id cam_id,
			enum _LockType_t lock_mode);

	/**
	 *auto white balance unlock.
	 *@param cam_id
	 *indicate which camera
	 */
	int (*auto_wb_unlock)(void *context, enum camera_id cam_id);

	/**
	 *set exposure mode.
	 *@param cam_id
	 *indicate which camera
	 *@param mode
	 *it contains the mode to set.
	 */
	int (*set_exposure_mode)(void *context, enum camera_id cam_id,
				enum _AeMode_t mode);

	/**
	 *start auto exposure for cam_id, return 0 for success others for fail
	 */
	int (*start_ae)(void *context, enum camera_id cam_id);

	/**
	 *stop auto exposure for cam_id, return 0 for success others for fail
	 */
	int (*stop_ae)(void *context, enum camera_id cam_id);

	/**
	 *set white balance mode.
	 *@param cam_id
	 *indicate which camera
	 *@param mode
	 *it contains the mode to set.
	 */
	int (*set_wb_mode)(void *context, enum camera_id cam_id,
			enum _AwbMode_t mode);

	/**
	 *start white balance for cam_id, return 0 for success others for fail
	 */
	int (*start_wb)(void *context, enum camera_id cam_id);

	/**
	 *stop white balance for cam_id, return 0 for success others for fail
	 */
	int (*stop_wb)(void *context, enum camera_id cam_id);

	/**
	 *set white balance light.
	 *@param cam_id
	 *indicate which camera
	 *@param light
	 *the light, should be light index
	 */
	int (*set_wb_light)(void *context, enum camera_id cam_id,
			unsigned int light_idx);
	/**
	 *set white balance color temperature.
	 */
	int (*set_wb_colorT)(void *context, enum camera_id cam_id,
			unsigned int colorT);
	/**
	 *set white balance gain.
	 */
	int (*set_wb_gain)(void *context, enum camera_id cam_id,
			struct _WbGain_t wb_gain);
	/**
	 *set sensor analog gain.
	 *@param cam_id
	 *indicate which camera
	 *@param ana_gain
	 *it contains the analog gain value multiplied by 1000
	 */
	int (*set_snr_ana_gain)(void *context, enum camera_id cam_id,
				unsigned int ana_gain);

	/**
	 *set sensor digital gain.
	 *@param cam_id
	 *indicate which camera
	 *@param ana_gain
	 *it contains the digital gain
	 */
	int (*set_snr_dig_gain)(void *context, enum camera_id cam_id,
				unsigned int dig_gain);

	/**
	 *get sensor analog gain.
	 *@param cam_id
	 *indicate which camera
	 *@param ana_gain
	 *it stores the analog gain value multiplied by 1000
	 *@param dig_gain
	 *it stores the digital gain value multiplied by 1000
	 */
	int (*get_snr_gain)(void *context, enum camera_id cam_id,
			unsigned int *ana_gain, unsigned int *dig_gain);

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
	int (*get_snr_gain_cap)(void *context, enum camera_id cam_id,
				struct para_capability_u32 *ana,
				struct para_capability_u32 *dig);

	/**
	 *set sensor integreation time.
	 *@param cam_id
	 *indicate which camera
	 *@param itime
	 *the integration time in microsecond to set
	 */
	int (*set_snr_itime)(void *context, enum camera_id cam_id,
				unsigned int itime);

	/**
	 *get sensor integreation time.
	 *@param cam_id
	 *indicate which camera
	 *@param itime
	 *it stores the integration time in microsecond to set
	 */
	int (*get_snr_itime)(void *context, enum camera_id cam_id,
				unsigned int *itime);

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
	int (*get_snr_itime_cap)(void *context, enum camera_id cam_id,
				unsigned int *min, unsigned int *max,
				unsigned int *step);

	/**
	 *register fw parser.
	 *@param parser
	 *parser
	 */
	void (*reg_fw_parser)(void *context, enum camera_id cam_id,
			struct fw_parser_operation_t *parser);

	/**
	 *unregister fw parser.
	 */
	void (*unreg_fw_parser)(void *context, enum camera_id cam_id);

	/**
	 *set the global value in script
	 *@param cam_id
	 *indicate which camera
	 *@param type
	 *indicate what kind of value to set
	 *@param gv
	 *value to be set
	 */
	int (*set_fw_gv)(void *context, enum camera_id id,
			enum fw_gv_type type, struct _ScriptFuncArg_t *gv);

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
	int (*set_digital_zoom)(void *context, enum camera_id id,
				unsigned int h_off, unsigned int v_off,
				unsigned int width, unsigned int height);

	/**
	 *set the digital zoom ratio multi by 100, so 100 means 1
	 *@param cam_id
	 *indicate which camera
	 *@param zoom
	 *numerator of ratio
	 */
	int (*set_digital_zoom_ratio)(void *context, enum camera_id id,
					unsigned int zoom);

	/**
	 *set the digital zoom pos
	 *@param cam_id
	 *indicate which camera
	 *@param h_off
	 *horizontal offset of the region to be zoomed
	 *@param v_off
	 *virtical offset of the region to be zoomed
	 */
	int (*set_digital_zoom_pos)(void *context, enum camera_id id,
				short h_off, short v_off);

	/**
	 *set the iso
	 *@param cam_id
	 *indicate which camera
	 *@param value
	 *the iso value
	 */
	int (*set_iso_value)(void *context, enum camera_id id,
				struct _AeIso_t value);

	/**
	 *set the EV compensation
	 *@param cam_id
	 *indicate which camera
	 *@param value
	 *the ev value
	 */
	int (*set_ev_value)(void *context, enum camera_id id,
				struct _AeEv_t value);

	/**
	 *get the sharpness
	 *@param cam_id
	 *indicate which camera
	 *@param sharp
	 *the sharpness value
	 *
	 *int (*get_sharpness)(void *context, enum camera_id id,
	 *			struct _isp_af_sharpness_t *sharp);
	 */
	/**
	 *set the color effect
	 *@param cam_id
	 *indicate which camera
	 *@param ce
	 *the color effect value
	 *
	 *int (*set_color_effect)(void *context, enum camera_id id,
	 *			isp_ie_config_t *ie);
	 */
	/**
	 *disable the color effect
	 *@param cam_id
	 *indicate which camera
	 */
	int (*disable_color_effect)(void *context, enum camera_id cam_id);

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
	int (*set_zsl_buf)(void *context, enum camera_id cam_id,
			struct sys_img_buf_handle *buf_hdl);
	/**
	 *start zsl for cam_id, return 0 for success others for fail
	 */
	int (*start_zsl)(void *context, enum camera_id cam_id,
			bool is_perframe_ctl);

	/**
	 *stop zsl for cam_id, return 0 for success others for fail
	 */
	int (*stop_zsl)(void *context, enum camera_id cam_id, bool pause);

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
	int (*take_one_pp_jpeg)(void *context, enum camera_id cam_id,
/*			struct isp_pp_jpeg_input *pp_input,*/
			struct sys_img_buf_handle *in_yuy2_buf,
			struct sys_img_buf_handle *main_jpg_buf,
			struct sys_img_buf_handle *jpg_thumb_buf,
			struct sys_img_buf_handle *yuv_thumb_buf);

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
	int (*set_metadata_buf)(void *context, enum camera_id cam_id,
				struct sys_img_buf_handle *buf_hdl);

	/**
	 *start metadata for cam_id, return 0 for success others for fail
	 */
	int (*start_metadata)(void *context, enum camera_id cam_id);

	/**
	 *stop metadata for cam_id, return 0 for success others for fail
	 */
	int (*stop_metadata)(void *context, enum camera_id cam_id);

	/**
	 *get camera devices info, return 0 for success others for fail
	 */
	int (*get_camera_dev_info)(void *context, enum camera_id cam_id,
				struct camera_dev_info *cam_dev_info);
	/**
	 *get ISP color format list, return 0 for support others for fail
	 */
	int (*get_caps_data_format)(void *context, enum camera_id cam_id,
				enum pvt_img_fmt fmt);
	/**
	 *get capability, return 0 for support others for fail
	 */
	int (*get_caps)(void *context, enum camera_id cam_id);
	/**
	 *start CVIP sensor, return 0 for success, others for fail
	 */
	int (*start_cvip_sensor)(void *context, enum camera_id cam_id);
	/**
	 *set awb roi. return 0 for success others for fail.
	 *@param cam_id
	 *indicate which sensor
	 *@param window
	 *indicate the window
	 */
	int (*set_awb_roi)(void *context, enum camera_id cam_id,
			struct _Window_t *window);
	/**
	 *set ae roi. return 0 for success others for fail.
	 *@param cam_id
	 *indicate which sensor
	 *@param window
	 *indicate the window
	 */
	int (*set_ae_roi)(void *context, enum camera_id cam_id,
				struct _Window_t *window);
	/**
	 *set af roi. return 0 for success others for fail.
	 *@param cam_id
	 *indicate which sensor
	 *@param window
	 *indicate the window, max 3 windows to be set,
	 *window[i] == NULL means no ith windows
	 */
	//int (*set_af_roi)(void *context, enum camera_id cam_id,
			//AfWindowId_t id, struct _Window_t *window);

	/**
	 *set scene mode. return 0 for success others for fail.
	 *@param cam_id
	 *indicate which sensor
	 *@param mode
	 *indicate the the scene
	 */
	int (*set_scene_mode)(void *context, enum camera_id cam_id,
				enum isp_scene_mode mode);
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
	int (*set_snr_calib_data)(void *context, enum camera_id cam_id,
				void *data, unsigned int len);
	/**
	 *Map a vram surface handle to vram address. return 0 for success
	 *others for fail.
	 *@param handle
	 *the vram handle
	 *@param vram_addr
	 *store the mapped address
	 */
	int (*map_handle_to_vram_addr)(void *context, void *handle,
					unsigned long long *vram_addr);
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
	 *together with struct _FrameInfo_t in callback(CB_EVT_ID_FRAME_INFO),
	 *then upper layer can call
	 *set_rgb_bayer_frame_info to send the struct _FrameInfo_t and
	 *filled bayer raw down again to do post process to generate
	 *preview/video/zsl output. It is up to upperlayer to deciede
	 *if it will do any modifications to struct _FrameInfo_t and
	 *bayer raw before sending them down. so cam_id in this call must be
	 *the same as cam_id in set_rgbir_raw_buf and the camera must be rg
	 */
	int (*set_rgb_bayer_frame_info)(void *context, enum camera_id cam_id,
					struct _FrameInfo_t *frame_info,
					struct sys_img_buf_handle *bayer_raw);
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
	 *together with struct _FrameInfo_t in callback(CB_EVT_ID_FRAME_INFO),
	 *then upper layer can call
	 *set_ir_frame_info to send a Mode4FrameInfo_t (which is derived from
	 *struct _FrameInfo_t) and filled ir raw down to do post process to
	 *generate preview/video/zsl output. It is up to
	 *upperlayer to deciede if it will do any modifications to ir raw
	 *before sending it down. Before any operations to this camera,
	 *it must be opend first by
	 *calling open_camera with cam_id set to CAMERA_ID_MEM
	 */
	int (*set_ir_frame_info)(void *context, enum camera_id cam_id,
				struct _FrameInfo_t *frame_info,
				struct sys_img_buf_handle *ir_raw);
	/**
	 *Set driver setting. return 0 for success
	 *others for fail.
	 */
	int (*set_drv_settings)(void *context,
				struct driver_settings *setting);
	/**
	 *Enable dynamic frame rate. return 0 for success
	 *others for fail.
	 *@param enable
	 *1 to enable and 0 to disable
	 */
	int (*enable_dynamic_frame_rate)(void *context,
					enum camera_id cam_id, bool enable);
	/**
	 *Set the max frame rate of a stream. return 0 for success
	 *others for fail.
	 *@comment
	 *this call must be called before calling start_preview,
	 *start_video and start_still
	 */
	int (*set_max_frame_rate)(void *context, enum camera_id cam_id,
				enum stream_id sid,
				unsigned int frame_rate_numerator,
				unsigned int frame_rate_denominator);
	/**
	 *Set frame rate info of a stream. return 0 for success
	 *others for fail.
	 *@comment
	 *frame_rate = Actual framerate multiplied by 1000
	 */
	int (*set_frame_rate_info)(void *context, enum camera_id cam_id,
				enum stream_id sid, unsigned int frame_rate);
	/**
	 *Set flicker. return 0 for success
	 *others for fail.
	 */
	int (*set_flicker)(void *context, enum camera_id cam_id,
			enum stream_id sid,
			enum _AeFlickerType_t mode);
	/**
	 *Set iso priority. return 0 for success
	 *others for fail.
	 */
	int (*set_iso_priority)(void *context, enum camera_id cam_id,
			enum stream_id sid, struct _AeIso_t *iso);

	/**
	 *Set ev compensation. return 0 for success
	 *others for fail.
	 */
	int (*set_ev_compensation)(void *context, enum camera_id cam_id,
			enum stream_id sid, struct _AeEv_t *ev);
	/**
	 *Set tnr config. return 0 for success
	 *others for fail.
	 */
	int (*tnr_config)(void *context, enum camera_id cam_id,
			enum stream_id sid, struct _TemperParams_t *param);
	/**
	 *Set snr config. return 0 for success
	 *others for fail.
	 */
	int (*snr_config)(void *context, enum camera_id cam_id,
			enum stream_id sid, struct _SinterParams_t *param);
	/**
	 *sherpen enable. return 0 for success
	 *others for fail.
	 */
	int (*sharpen_enable)(void *context, enum camera_id cam_id,
			enum stream_id sid, enum _SharpenId_t *sharpen_id);
	/**
	 *sherpen disable. return 0 for success
	 *others for fail.
	 */
	int (*sharpen_disable)(void *context, enum camera_id cam_id,
				enum stream_id sid,
				enum _SharpenId_t *sharpen_id);
	/**
	 *sherpen config. return 0 for success
	 *others for fail.
	 */
	int (*sharpen_config)(void *context, enum camera_id cam_id,
				enum stream_id sid,
				enum _SharpenId_t sharpen_id,
				struct _TdbSharpRegByChannel_t param);
	/**
	 *clock gating enable. return 0 for success
	 *others for fail.
	 */
	int (*clk_gating_enable)(void *context, enum camera_id cam_id,
				unsigned int enable);
	/**
	 *power gating enable. return 0 for success
	 *others for fail.
	 */
	int (*power_gating_enable)(void *context, enum camera_id cam_id,
				unsigned int enable);

	/**
	 *enable dump raw. return 0 for success
	 *others for fail.
	 */
	int (*dump_raw_enable)(void *context, enum camera_id cam_id,
				unsigned int enable);

	/**
	 *image effect enable. return 0 for success
	 *others for fail.
	 */
	int (*image_effect_enable)(void *context, enum camera_id cam_id,
				enum stream_id sid, unsigned int enable);

	/**
	 *image effect config. return 0 for success
	 *others for fail.
	 */
	int (*image_effect_config)(void *context, enum camera_id cam_id,
				enum stream_id sid, struct _IeConfig_t *param);

	/**
	 *set video hdr .
	 *@param cam_id
	 *indicate which camera
	 */
	int (*set_video_hdr)(void *context, enum camera_id cam_id,
				unsigned int enable);

	/**
	 *set video hdr .
	 *@param cam_id
	 *indicate which camera
	 */
	int (*set_photo_hdr)(void *context, enum camera_id cam_id,
				unsigned int enable);
	/**
	 *set lens shading matrix .
	 *@param cam_id
	 *indicate which camera
	 */
	int (*set_lens_shading_matrix)(void *context, enum camera_id cam_id,
				struct _LscMatrix_t *matrix);

	/**
	 *set lens shading sector .
	 *@param cam_id
	 *indicate which camera
	 */
	int (*set_lens_shading_sector)(void *context, enum camera_id cam_id,
				struct _PrepLscSector_t *sectors);
	/**
	 *set AWB cc matrix command.
	 *@param cam_id
	 *indicate which camera
	 */
	int (*set_awb_cc_matrix)(void *context, enum camera_id cam_id,
				struct _CcMatrix_t *matrix);

	/**
	 *set AWB cc matrix offset.
	 *@param cam_id
	 *indicate which camera
	 */
	int (*set_awb_cc_offset)(void *context, enum camera_id cam_id,
				struct _CcOffset_t *ccOffset);
	/**
	 *gamma outenable/disable .
	 *@param cam_id
	 *indicate which camera
	 *@param enable
	 *1 to enable 0 to disable
	 */
	int (*gamma_enable)(void *context, enum camera_id cam_id,
				unsigned int enable);

	/**
	 *TNR enable/disable .
	 *@param cam_id
	 *indicate which camera
	 *@param enable
	 *1 to enable 0 to disable
	 */
	int (*tnr_enable)(void *context, enum camera_id cam_id,
				unsigned int enable);

	/**
	 *SNR enable/disable .
	 *@param cam_id
	 *indicate which camera
	 *@param enable
	 *1 to enable 0 to disable
	 */
	int (*snr_enable)(void *context, enum camera_id cam_id,
				unsigned int enable);

	/**
	 *set the color process enablement of camera cam_id
	 *@param enable
	 *1 is enable, 0 is disable
	 */
	int (*cproc_enable)(void *context, enum camera_id cam_id,
			unsigned int enable);

	/**
	 *set contrast of camera cam_id
	 */
	int (*cproc_set_contrast)(void *context, enum camera_id cam_id,
			unsigned int contrast);
	/**
	 *set saturation of camera cam_id
	 */
	int (*cproc_set_saturation)(void *context, enum camera_id cam_id,
			unsigned int saturation);

	/**
	 *set hue of camera cam_id
	 */
	int (*cproc_set_hue)(void *context, enum camera_id cam_id,
			unsigned int hue);

	/**
	 *set brightness of camera cam_id
	 */
	int (*cproc_set_brightness)(void *context, enum camera_id cam_id,
			unsigned int brightness);

	/**
	 *get the color process status of camera cam_id
	 *@param cproc
	 *to store the result of color process
	 */
	int (*get_cproc_status)(void *context, enum camera_id cam_id,
				struct _CprocStatus_t *cproc);

	/**
	 *get isp moudle init parameter called by KMD
	 *@param sw_init
	 *struct isp_sw_init_input *typed, to store the sw_init para
	 *@param hw_init
	 *struct isp_hw_init_input *typed, to store the hw_init para
	 *@param isp_env
	 *enum isp_environment_setting typed, to stroe the actual environment
	 */
	int (*get_init_para)(void *context,
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
	int (*lfb_resv)(void *context, unsigned int size,
		unsigned long long *sys, unsigned long long *mc);

	/**
	 *free the reserved local frame buffer
	 */
	int (*lfb_free)(void *context);


	/**
	 *send fw command
	 */
	int (*fw_cmd_send)(void *context, enum camera_id cam_id,
				unsigned int cmd, unsigned int is_set_cmd,
				unsigned short is_stream_cmd,
				unsigned short to_ir,
				unsigned int is_direct_cmd,
				void *para, unsigned int *para_len);

	/**
	 * Interface to exchange the online tuning buffer.
	 * @param context
	 * pointer of struct isp_isp_module_if.context.
	 * @param camera_id
	 * the running camera id.
	 * @param tuning_cmd
	 * ISP online tuning command ID.
	 * @param is_set_cmd
	 * indicate to the given command is a set commnad or not. If the given
	 * command is NOT a set command, the given buffer |tuning_data|
	 * represents an in/out buffer and will store data updated by ISP
	 * FW.
	 * 0: indicate this command is set only (no need to read data back).
	 * 1: indicate this command carries out the data from ISP FW.
	 * @param is_stream_cmd
	 * Need to see the doument and set the value by the given command.
	 * 0: indicate this command is not a stream specific command.
	 * 1: indicate this command is stream specific command, and the
	 * stream id will be retrieved by camera_id automatically.
	 * @param is_direct_cmd
	 * 0: indicate this command is not a direct command (see document for
	 *    knowning which commands are direct or indirect).
	 * 1: indicate this command is the direct command.
	 * @param tuning_data memory stored data struct of the given command.
	 * This buffer should be accessible by kernel, e.g.: allocated by
	 * vmalloc, kmalloc or kzalloc.
	 * @param data_size should be the size of |tuning_data|, can be 0 if
	 * |tuning_data| is also NULL.
	 * @param resp_payload
	 * Buffer to store the response payload returned by ISP firmware, this
	 * buffer is valid if this function returns OK, and this buffer must
	 * be the one which can be accssed by kernel.
	 * @param payload_size
	 * The payload size usually fixed to 36 since it's predefined buffer.
	 * @return
	 * 0: success.
	 * -ENOMEM: wrong mem
	 * otherwise: check the error code.
	 */
	int (*online_isp_tune)(void *context, enum camera_id cam_id,
			unsigned int tuning_cmd, unsigned int is_set_cmd,
			unsigned short is_stream_cmd,
			unsigned int is_direct_cmd,
			void *tuning_data, unsigned int *data_size,
			void *resp_payload, unsigned int payload_size);

	/**
	 *get the work buf info which is pre allocated in swISP
	 *@param work_buf_info
	 *store the buffer info actuall it's struct isp_work_buf *
	 */
	int (*get_isp_work_buf)(void *context, void *work_buf_info);

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
	int (*i2c_write_reg_group)(void *context, unsigned char bus_num,
				unsigned short slave_addr,
				enum _i2c_slave_addr_mode_t slave_addr_mode,
				unsigned int reg_addr,
				unsigned char reg_addr_size,
				unsigned int *reg_value,
				unsigned char reg_size);
};

#endif
