/*
 * Copyright (C) 2019-2021 Advanced Micro Devices, Inc. All rights reserved.
 */

#ifndef ISP_CAMERA_H
#define ISP_CAMERA_H

#include "os_base_type.h"
#include "isp_fw_if/drv_isp_if.h"
#include "fsl_common.h"

#define RET_SUCCESS 0
#define RET_FAILURE 1		/*!< general failure */
#define STATUS_FLASH_INTERRUPT_TORCH -1
#define STATUS_FLASH_HOLD_BY_FLASH -2
#define STATUS_FLASH_UNSUPPORT -3

#define RES_FPS_MAX 10

/*same as aidt_sensor_ctrl_script_t*/
struct _isp_fw_script_ctrl_t {
	unsigned char *script_buf;	// The ctrl script buffer
	// The length of the script in bytes
	uint32_t length;
	struct _ScriptFuncArg_t argAf;	//Args for af control
	struct _ScriptFuncArg_t argAe;	//Args for ae control
	struct _ScriptFuncArg_t argFl;	//Args for flash control
	struct _ScriptI2cArg_t i2cAf;	//I2C Parameters for af
	struct _ScriptI2cArg_t i2cAe;	//I2C Parameters for ae
	struct _ScriptI2cArg_t i2cFl;	//I2C Parameters for flash light
};

struct res_fps_t {
	unsigned int width;	// Width of this sensor profile
	unsigned int height;	// Height of this sensor profile
	unsigned int fps;	// FPS, real value like 15, 30, 60
	/*Whether can be opened as non-HDR mode
	 *(always TRUE for now, can be removed?)
	 */
	unsigned char none_hdr_support;
	// Whether can be opened as HDR mode
	unsigned char hdr_support;
	/*Whether this profile is valid for current
	 *platform (Silicon, FPGA and test purpose)
	 */
	bool valid;
	/*The index defined in camera sensor driver
	 *that mapped to specific width/height/FPS/HDR..
	 */
	unsigned int index;
	struct _Window_t crop_wnd;
};

struct sensor_res_fps_t {
	unsigned int id;
	unsigned int count;
	struct res_fps_t res_fps[RES_FPS_MAX];
};

enum sensor_hdr_gain_mode {
	SENSOR_HDR_GAIN_COMBINED = 0,
	SENSOR_HDR_GAIN_SEPARATE = 1,
};

enum sensor_test_pattern {
	TP_MODE_OFF = 0,
	TP_MODE_SOLID_COLOR = 1,
	TP_MODE_COLOR_BARS = 2,
	TP_MODE_COLOR_BARS_FADE_TO_GRAY = 3,
	TP_MODE_PN9 = 4,
	TP_MODE_CUSTOM1 = 256,
};

enum storage_data_id {
	STORAGE_DATA_ID_CALIBRATE_REFERENCE_TUNE_DATA = 0,
	STORAGE_DATA_ID_CALIBRATE_RESULT,
};

enum fw_gv_type {
	FW_GV_AF,
	FW_GV_AE_ANA_GAIN,
	FW_GV_AE_DIG_GAIN,
	FW_GV_AE_ITIME,
	FW_GV_FL,
	FW_GV_HDR,
};

enum camera_type {
	CAMERA_TYPE_RGB_BAYER = 0,
	CAMERA_TYPE_RGBIR = 1,
	CAMERA_TYPE_IR = 2,
	CAMERA_TYPE_RGBIR_HWDIR = 3,
	CAMERA_TYPE_MEM
};

struct sensor_hw_parameter {
	enum camera_type type;
	int focus_len;
	unsigned int normalized_focal_len_x;
	unsigned int normalized_focal_len_y;
	char *module_name;
	char *sensor_name;
	char *vcm_name;

	unsigned char support_hdr;

	/*following info should better be put into tuning data */
	unsigned int min_lens_pos;	/*min lens position */
	unsigned int max_lens_pos;	/*max lens position */
	unsigned int init_lens_pos;	/*initial lens position */
	unsigned int lens_step;
	unsigned int hyperfocal_pos;
	struct _Window_t ae_roi_wnd;
	unsigned int IRControl_drop_daley;
	unsigned int IRControl_drop_durarion;
	unsigned int IRControl_delay;
	unsigned int IRControl_duration;
	enum _SensorShutterType_t sensorShutterType;
};

struct sensor_operation_t {
	int (*ioctl)(unsigned int cmd, void *arg);
	int (*get_i2c_config)(unsigned int id,
			struct _ScriptI2cArg_t *i2c_config);
	int (*get_fw_script_len)(unsigned int id, unsigned int *len);
	int (*get_fw_script_ctrl)(unsigned int id,
				struct _isp_fw_script_ctrl_t *buf);

	int (*poweron_sensor)(unsigned int id, int on);
	int (*reset_sensor)(unsigned int id);
	int (*config_sensor)(unsigned int id, unsigned int res_fps_id,
			unsigned int flag);
	int (*start_sensor)(unsigned int id, int config_phy);
	int (*stop_sensor)(unsigned int id);
	int (*stream_on_sensor)(unsigned int id);
	int (*get_caps)(unsigned int id, int prf_id,
			void *caps/*struct _isp_sensor_prop_t */);
	int (*get_gain_limit)(unsigned int id, unsigned int *min_gain,
				unsigned int *max_gain);
	int (*get_integration_time_limit)(unsigned int id,
					unsigned int *
					min_integration_time,
					unsigned int *
					max_integration_time);
	int (*exposure_control)(unsigned int id, unsigned int new_gain,
				unsigned int new_integration_time,
				unsigned int *set_gain,
				unsigned int *set_integration);
	int (*get_current_exposure)(unsigned int id, uint32_t *gain,
				uint32_t *integration_time);
	int (*get_analog_gain)(unsigned int id, uint32_t *gain);
	int (*get_itime)(unsigned int id, uint32_t *itime);
	int (*support_af)(unsigned int id);
	int (*hdr_support)(unsigned int id);
	int (*hdr_get_expo_ratio)(unsigned int id);
	int (*hdr_set_expo_ratio)(unsigned int id, unsigned int ratio);
	int (*hdr_set_gain_mode)(unsigned int id,
				enum sensor_hdr_gain_mode mode);
	int (*hdr_set_gain)(unsigned int id, unsigned int new_gain,
				unsigned int *set_gain);
	int (*hdr_enable)(unsigned int id);
	int (*hdr_disable)(unsigned int id);
	int (*get_emb_prop)(unsigned int id, bool *supported,
			struct _SensorEmbProp_t *embProp);
	struct sensor_res_fps_t *(*get_res_fps)(unsigned int id);
	 int (*sned_get_fralenline_cmd)(unsigned int id);
	 int (*get_fralenline_regaddr)(unsigned int id,
					unsigned short *hi_addr,
					unsigned short *lo_addr);
	 int (*get_sensor_hw_parameter)(unsigned int id,
					struct sensor_hw_parameter *
					para);
	int (*set_test_pattern)(unsigned int id, enum sensor_test_pattern mode);
	int (*get_m2m_data)(unsigned int id, struct _M2MTdb_t *pM2MTdb);
};

struct vcm_operation_t {
	int (*ioctl)(unsigned int cmd, void *arg);
	int (*get_i2c_config)(unsigned int id,
			struct _ScriptI2cArg_t *i2c_config);
	int (*get_fw_script_len)(unsigned int id, unsigned int *len);
	int (*get_fw_script_ctrl)(unsigned int id, unsigned char *buf,
				unsigned int *len);

	int (*setup_moto_drive)(unsigned int id, unsigned int *max_step);
	int (*set_focus)(unsigned int id, unsigned int abs_step);
	int (*get_focus)(unsigned int id, unsigned int *abs_step);
};

struct flash_operation_t {
	int (*ioctl)(unsigned int cmd, void *arg);
	int (*get_i2c_config)(unsigned int id,
			struct _ScriptI2cArg_t *i2c_config);
	int (*get_fw_script_len)(unsigned int id, unsigned int *len);
	int (*get_fw_script_ctrl)(unsigned int id, unsigned char *buf,
				unsigned int *len);

	int (*config_flashlight)(unsigned int id, unsigned int mode,
				unsigned int intensity,
				unsigned int flash_timeout);
	int (*trigger_flashlight)(unsigned int id);
	int (*switch_off_torch)(unsigned int id);
};

struct storage_operation_t {
	int (*ioctl)(unsigned int cmd, void *arg);

	int (*read_crc)(unsigned int id, unsigned int *crc);
	int (*read)(unsigned int id, unsigned char *buf, unsigned int *len);
};

struct fw_parser_operation_t {
	int (*parse)(unsigned char *script, unsigned int len,
			struct _InstrSeq_t *bin);
};

unsigned int util_crc32(unsigned int crc, unsigned char const *buffer,
	size_t len);

#endif
