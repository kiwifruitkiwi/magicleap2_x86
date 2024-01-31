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
 **************************************************************************/

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
typedef struct _isp_fw_script_ctrl_t {
	uint8 *script_buf;	// The ctrl script buffer
	// The length of the script in bytes
	uint32_t length;
	ScriptFuncArg_t argAf;	//Args for af control
	ScriptFuncArg_t argAe;	//Args for ae control
	ScriptFuncArg_t argFl;	//Args for flash control
	ScriptI2cArg_t i2cAf;	//I2C Parameters for af
	ScriptI2cArg_t i2cAe;	//I2C Parameters for ae
	ScriptI2cArg_t i2cFl;	//I2C Parameters for flash light
} isp_fw_script_ctrl_t;

struct res_fps_t {
	uint32 width;	// Width of this sensor profile
	uint32 height;	// Height of this sensor profile
	uint32 fps;	// FPS, real value like 15, 30, 60
	/*Whether can be opened as non-HDR mode
	 *(always TRUE for now, can be removed?)
	 */
	uint8 none_hdr_support;
	// Whether can be opened as HDR mode
	uint8 hdr_support;
	/*Whether this profile is valid for current
	 *platform (Silicon, FPGA and test purpose)
	 */
	bool valid;
	/*The index defined in camera sensor driver
	 *that mapped to specific width/height/FPS/HDR..
	 */
	uint32 index;
	window_t crop_wnd;
};

struct sensor_res_fps_t {
	uint32 id;
	uint32 count;
	struct res_fps_t res_fps[RES_FPS_MAX];
};

enum sensor_hdr_gain_mode {
	SENSOR_HDR_GAIN_COMBINED = 0,
	SENSOR_HDR_GAIN_SEPARATE = 1,
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
	CAMERA_TYPE_MEM = 3
};

struct sensor_hw_parameter {
	enum camera_type type;
	int32 focus_len;
	uint32 normalized_focal_len_x;
	uint32 normalized_focal_len_y;
	int8 *module_name;
	int8 *sensor_name;
	int8 *vcm_name;

	uint8 support_hdr;

	/*following info should better be put into tuning data */
	uint32 min_lens_pos;	/*min lens position */
	uint32 max_lens_pos;	/*max lens position */
	uint32 init_lens_pos;	/*initial lens position */
	uint32 lens_step;
	uint32 hyperfocal_pos;
	window_t ae_roi_wnd;
	uint32 IRControl_drop_daley;
	uint32 IRControl_drop_durarion;
	uint32 IRControl_delay;
	uint32 IRControl_duration;
	SensorShutterType_t sensorShutterType;
};

struct sensor_operation_t {
	int32 (*ioctl)(uint32 cmd, void *arg);
	int32 (*get_i2c_config)(uint32 id, ScriptI2cArg_t *i2c_config);
	int32 (*get_fw_script_len)(uint32 id, uint32 *len);
	int32 (*get_fw_script_ctrl)(uint32 id,
				isp_fw_script_ctrl_t *buf);

	int32 (*poweron_sensor)(uint32 id, bool_t on);
	int32 (*reset_sensor)(uint32 id);
	int32 (*config_sensor)(uint32 id, uint32 res_fps_id,
			uint32 flag);
	int32 (*start_sensor)(uint32 id, bool_t config_phy);
	int32 (*stop_sensor)(uint32 id);
	int32 (*stream_on_sensor)(uint32 id);
	int32 (*get_caps)(uint32 id,
			void *caps/*isp_sensor_prop_t */);
	int32 (*get_gain_limit)(uint32 id, uint32 *min_gain,
				uint32 *max_gain);
	int32 (*get_integration_time_limit)(uint32 id,
					uint32 *
					min_integration_time,
					uint32 *
					max_integration_time);
	int32 (*exposure_control)(uint32 id, uint32 new_gain,
				uint32 new_integration_time,
				uint32 *set_gain,
				uint32 *set_integration);
	int32 (*get_current_exposure)(uint32 id, uint32_t *gain,
				uint32_t *integration_time);
	int32 (*get_analog_gain)(uint32 id, uint32_t *gain);
	int32 (*get_itime)(uint32 id, uint32_t *itime);
	int32 (*support_af)(uint32 id);
	int32 (*hdr_support)(uint32 id);
	int32 (*hdr_get_expo_ratio)(uint32 id);
	int32 (*hdr_set_expo_ratio)(uint32 id, uint32 ratio);
	int32 (*hdr_set_gain_mode)(uint32 id,
				enum sensor_hdr_gain_mode mode);
	int32 (*hdr_set_gain)(uint32 id, uint32 new_gain,
				uint32 *set_gain);
	int32 (*hdr_enable)(uint32 id);
	int32 (*hdr_disable)(uint32 id);
	int32 (*get_emb_prop)(uint32 id, bool *supported,
			SensorEmbProp_t *embProp);
	struct sensor_res_fps_t *(*get_res_fps)(uint32 id);
	 int32 (*get_runtime_fps)(uint32 id, uint16 hi_value,
				uint16 lo_value, float *fps);
	 int32 (*sned_get_fralenline_cmd)(uint32 id);
	 int32 (*get_fralenline_regaddr)(uint32 id,
					uint16 *hi_addr,
					uint16 *lo_addr);
	 int32 (*get_sensor_hw_parameter)(uint32 id,
					struct sensor_hw_parameter *
					para);
};

struct vcm_operation_t {
	int32 (*ioctl)(uint32 cmd, void *arg);
	int32 (*get_i2c_config)(uint32 id, ScriptI2cArg_t *i2c_config);
	int32 (*get_fw_script_len)(uint32 id, uint32 *len);
	int32 (*get_fw_script_ctrl)(uint32 id, uint8 *buf,
				uint32 *len);

	int32 (*setup_moto_drive)(uint32 id, uint32 *max_step);
	int32 (*set_focus)(uint32 id, uint32 abs_step);
	int32 (*get_focus)(uint32 id, uint32 *abs_step);
};

struct flash_operation_t {
	int32 (*ioctl)(uint32 cmd, void *arg);
	int32 (*get_i2c_config)(uint32 id, ScriptI2cArg_t *i2c_config);
	int32 (*get_fw_script_len)(uint32 id, uint32 *len);
	int32 (*get_fw_script_ctrl)(uint32 id, uint8 *buf,
				uint32 *len);

	int32 (*config_flashlight)(uint32 id, uint32 mode,
				uint32 intensity,
				uint32 flash_timeout);
	int32 (*trigger_flashlight)(uint32 id);
	int32 (*switch_off_torch)(uint32 id);
};

struct storage_operation_t {
	int32 (*ioctl)(uint32 cmd, void *arg);

	int32 (*read_crc)(uint32 id, uint32 *crc);
	int32 (*read)(uint32 id, uint8 *buf, uint32 *len);
};

struct fw_parser_operation_t {
	int32 (*parse)(uint8 *script, uint32 len, InstrSeq_t *bin);
};

uint32 util_crc32(uint32 crc, uint8 const *buffer, size_t len);

#endif
