/*
 * Copyright (C) 2019-2021 Advanced Micro Devices, Inc. All rights reserved.
 */

#ifndef _SENSOR_IF_H
#define _SENSOR_IF_H

#include "isp_fw_if/drv_isp_if.h"
#include "isp_cam_if.h"
#include "isp_module_if.h"
#include "cam_userspace.h"
#include "os_base_type.h"
#include "cam_common.h"
#include "isp_module/src/log.h"

#define SENSOR_SCRIPT_PATH "isp/"
#define FILE_NAME_MAX_LEN 100
#define FW_SCRIPT_FILE_NAME_SUFFIX ".asm"
#define FW_SCRIPT_FILE_NAME_PREFIX "asm_"

struct sensor_hw_config {
	unsigned char sensor_i2c_index;
	unsigned char sensor_phy_i2c_index;
	unsigned short reverse;
};

struct rational_t {
	unsigned int numerator;
	unsigned int denominator;
};
struct sensor_res_fps_info_ops {
	struct res_fps_t res_fps;
};

struct sensor_module_t {
	unsigned int id;
	struct sensor_hw_config *hw;
	char *name;

	int (*poweron_sensor)(struct sensor_module_t *module,
				 int on);
	int (*reset_sensor)(struct sensor_module_t *module);
	int (*config_sensor)(struct sensor_module_t *module,
				unsigned int res, unsigned int flag);
	int (*start_sensor)(struct sensor_module_t *module,
			int config_phy);
	int (*stop_sensor)(struct sensor_module_t *module);
	int (*stream_on_sensor)(struct sensor_module_t *module);
	int (*get_caps)(struct sensor_module_t *module,
			struct _isp_sensor_prop_t *caps, int prf_id);
	int (*get_gain_limit)(struct sensor_module_t *module,
		 unsigned int *min_gain, unsigned int *max_gain);
	int (*get_integration_time_limit)(struct sensor_module_t *
					module,
					unsigned int *
					min_integration_time,
					unsigned int *
					max_integration_time);
	int (*exposure_control)(struct sensor_module_t *module,
					unsigned int new_gain,
					unsigned int new_integration_time,
					unsigned int *set_gain,
					unsigned int *set_integration);
	int (*get_current_exposure)(struct sensor_module_t *module,
					uint32_t *gain,
					uint32_t *integration_time);
	int (*support_af)(struct sensor_module_t *module,
				unsigned int *support);
	int (*hdr_support)(struct sensor_module_t *module,
				unsigned int *support);
	int (*hdr_get_expo_ratio)(struct sensor_module_t *module);
	int (*hdr_set_expo_ratio)(struct sensor_module_t *module,
				unsigned int ratio);
	int (*hdr_set_gain_mode)(struct sensor_module_t *module,
				enum sensor_hdr_gain_mode mode);
	int (*hdr_set_gain)(struct sensor_module_t *module,
				unsigned int new_gain, unsigned int *set_gain);
	int (*hdr_enable)(struct sensor_module_t *module);
	int (*hdr_disable)(struct sensor_module_t *module);
	int (*get_emb_prop)(struct sensor_module_t *module,
				bool *supported,
				struct _SensorEmbProp_t *embProp);
	int (*get_res_fps)(struct sensor_module_t *module,
				unsigned int res_fps_id,
				struct res_fps_t *res_fps);
	int (*sned_get_fralenline_cmd)(struct sensor_module_t *
				module);
	int (*get_fralenline_regaddr)(struct sensor_module_t *
				module, unsigned short *hi_addr,
				unsigned short *lo_addr);
	int (*get_i2c_config)(struct sensor_module_t *module,
				unsigned int *i2c_addr, unsigned int *mode,
				unsigned int *reg_addr_width);
	int (*get_fw_script_ctrl)(struct sensor_module_t *module,
				struct _isp_fw_script_ctrl_t *ctrl);
	int (*get_analog_gain)(struct sensor_module_t *module,
				uint32_t *gain);
	int (*get_itime)(struct sensor_module_t *module,
				uint32_t *itime);
	int (*get_sensor_hw_parameter)(struct sensor_module_t *
				module,
				struct sensor_hw_parameter *para);
	int (*set_test_pattern)(struct sensor_module_t *module,
				enum sensor_test_pattern mode);
	int (*get_m2m_data)(struct sensor_module_t *module,
				struct _M2MTdb_t *pM2MTdb);
	struct drv_context context;
};

extern struct sensor_module_t g_imx577_driver;
extern struct sensor_module_t g_S5K3L6_driver;


void update_fw_gv(struct sensor_module_t *module, unsigned int id,
		unsigned int count, ...);

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

#endif
