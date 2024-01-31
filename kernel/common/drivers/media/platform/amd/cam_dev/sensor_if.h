/*
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
 */

#ifndef _SENSOR_IF_H
#define _SENSOR_IF_H

#include "isp_fw_if/drv_isp_if.h"
#include "isp_cam_if.h"
#include "isp_module_if.h"
#include "cam_userspace.h"
#include "os_base_type.h"
#include "cam_common.h"

#define SENSOR_SCRIPT_PATH "isp/"
#define FILE_NAME_MAX_LEN 100
#define FW_SCRIPT_FILE_NAME_SUFFIX ".asm"
#define FW_SCRIPT_FILE_NAME_PREFIX "asm_"

#define SENSOR_LOG_ERROR(fmt, args...) pr_err("SENSOR: " fmt, ## args)
#define SENSOR_LOG_INFO(fmt, args...) pr_info("SENSOR: " fmt, ## args)

struct sensor_hw_config {
	uint8 sensor_i2c_index;
	uint8 sensor_phy_i2c_index;
	uint16 reverse;
};

struct rational_t {
	uint32 numerator;
	uint32 denominator;
};
struct sensor_res_fps_info_ops {
	struct res_fps_t res_fps;
};

struct sensor_module_t {
	uint32 id;
	struct sensor_hw_config *hw;
	char *name;

	int32 (*poweron_sensor)(struct sensor_module_t *module,
				 bool_t on);
	int32 (*reset_sensor)(struct sensor_module_t *module);
	int32 (*config_sensor)(struct sensor_module_t *module,
				uint32 res, uint32 flag);
	int32 (*start_sensor)(struct sensor_module_t *module,
			bool_t config_phy);
	int32 (*stop_sensor)(struct sensor_module_t *module);
	int32 (*stream_on_sensor)(struct sensor_module_t *module);
	int32 (*get_caps)(struct sensor_module_t *module,
			isp_sensor_prop_t *caps);
	int32 (*get_gain_limit)(struct sensor_module_t *module,
				 uint32 *min_gain, uint32 *max_gain);
	int32 (*get_integration_time_limit)(struct sensor_module_t *
					module,
					uint32 *
					min_integration_time,
					uint32 *
					max_integration_time);
	int32 (*exposure_control)(struct sensor_module_t *module,
					uint32 new_gain,
					uint32 new_integration_time,
					uint32 *set_gain,
					uint32 *set_integration);
	int32 (*get_current_exposure)(struct sensor_module_t *module,
					uint32_t *gain,
					uint32_t *integration_time);
	int32 (*support_af)(struct sensor_module_t *module,
				uint32 *support);
	int32 (*hdr_support)(struct sensor_module_t *module,
				uint32 *support);
	int32 (*hdr_get_expo_ratio)(struct sensor_module_t *module);
	int32 (*hdr_set_expo_ratio)(struct sensor_module_t *module,
				uint32 ratio);
	int32 (*hdr_set_gain_mode)(struct sensor_module_t *module,
				enum sensor_hdr_gain_mode mode);
	int32 (*hdr_set_gain)(struct sensor_module_t *module,
				uint32 new_gain, uint32 *set_gain);
	int32 (*hdr_enable)(struct sensor_module_t *module);
	int32 (*hdr_disable)(struct sensor_module_t *module);
	int32 (*get_emb_prop)(struct sensor_module_t *module,
				bool *supported,
				SensorEmbProp_t *embProp);
	int32 (*get_res_fps)(struct sensor_module_t *module,
				uint32 res_fps_id,
				struct res_fps_t *res_fps);
	int32 (*get_runtime_fps)(struct sensor_module_t *module,
				uint16 hi_value, uint16 lo_value,
				float *fps);
	int32 (*sned_get_fralenline_cmd)(struct sensor_module_t *
				module);
	int32 (*get_fralenline_regaddr)(struct sensor_module_t *
				module, uint16 *hi_addr,
				uint16 *lo_addr);
	int32 (*get_i2c_config)(struct sensor_module_t *module,
				uint32 *i2c_addr, uint32 *mode,
				uint32 *reg_addr_width);
	int32 (*get_fw_script_ctrl)(struct sensor_module_t *module,
				isp_fw_script_ctrl_t *ctrl);
	int32 (*get_analog_gain)(struct sensor_module_t *module,
				uint32_t *gain);
	int32 (*get_itime)(struct sensor_module_t *module,
				uint32_t *itime);
	int32 (*get_sensor_hw_parameter)(struct sensor_module_t *
				module,
				struct sensor_hw_parameter *para);
	struct drv_context context;
};

extern struct sensor_module_t g_imx208_driver;

void update_fw_gv(struct sensor_module_t *module, uint32 id,
		uint32 count, ...);

struct sensor_operation_t *get_sensor_interface(void);
//commented out since not necessary for mero.
//struct vcm_operation_t *get_vcm_interface(void);
//struct flash_operation_t *get_flash_interface(void);
//struct storage_operation_t *get_storage_interface(void);
struct fw_parser_operation_t *get_fw_parser_interface(void);

void init_camera_devices(
		void *p,
		char using_xml);

struct isp_isp_module_if *get_ifl_interface(void);
void set_ifl_interface(struct isp_isp_module_if *intface);

#endif
