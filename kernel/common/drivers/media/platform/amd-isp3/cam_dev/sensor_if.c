/*
 * Copyright (C) 2019-2021 Advanced Micro Devices, Inc. All rights reserved.
 */

#include "sensor_if.h"
#include "isp_hw_reg.h"
#include "isp_common.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "[Sensor][sensor_if]"

#define mmISP_SENSOR_FLASH_CTRL		0x1B15C

struct g_sensor_t {
	struct sensor_hw_config hw;
	enum sensor_driver_mode mode;
	struct amdisp_sensor_prog *pprog[SENSOR_IOCTL_MAX_NUM];
	unsigned int *pgv;
	struct amdisp_sensor_init *init;
	struct dce_module *module;
	struct sensor_module_t c_module;
	struct sensor_res_fps_t sensor_res_fps;
	char xml_path[FILE_NAME_MAX_LEN];
	char fw_path[FILE_NAME_MAX_LEN];
	unsigned int config_id;
};

static struct mutex g_mt[MAX_SENSOR_COUNT];

static struct g_sensor_t g_sensor[MAX_SENSOR_COUNT];

static struct sensor_module_t *g_sensor_module[] = {
	&g_imx577_driver,
	&g_S5K3L6_driver,
};

struct isp_isp_module_if g_ifl_interface = { 0 };

struct isp_isp_module_if *get_ifl_interface(void)
{
	return &g_ifl_interface;
};

int fw_flen(char *file_path, unsigned int *length)
{
	int status = -1;
	const struct firmware *fw = NULL;

	if (request_firmware(&fw, file_path, &platform_bus)) {
		ISP_PR_ERR("path %s, fail\n", file_path);
		return status;
	}

	*length = fw->size;
	status = 0;

	if (fw)
		release_firmware(fw);

	return status;
}

int fw_fread(char *file_path, char *buffer, unsigned int *length)
{
	int status = -1;
	const struct firmware *fw = NULL;

	if (request_firmware(&fw, file_path, &platform_bus))
		return status;

	memcpy(buffer, fw->data, *length);
	status = 0;

	if (fw)
		release_firmware(fw);

	return status;
}

static int poweron_sensor(unsigned int id, int on)
{
	struct sensor_module_t *module;
	int status = 0;

	ISP_PR_INFO("id = %d\n", id);

	if (id >= MAX_SENSOR_COUNT)
		return -1;

	module = &g_sensor[id].c_module;

	mutex_lock(&g_mt[id]);

	if (g_sensor[id].mode == SENSOR_DRIVER_MODE_STATIC
	&& module->poweron_sensor)
		status = module->poweron_sensor(module, on);
	else {
		ISP_PR_ERR("unknown mode");
		status = -1;
	}

	mutex_unlock(&g_mt[id]);

	ISP_PR_INFO("done\n");
	return status;
}

static int reset_sensor(unsigned int id)
{
	struct sensor_module_t *module;
	int status = 0;

	ISP_PR_INFO("id = %d\n", id);

	if (id >= MAX_SENSOR_COUNT)
		return -1;

	module = &g_sensor[id].c_module;

	mutex_lock(&g_mt[id]);

	if (g_sensor[id].mode == SENSOR_DRIVER_MODE_STATIC
	&& module->reset_sensor)
		status = module->reset_sensor(module);
	else {
		ISP_PR_ERR("unknown mode");
		status = -1;
	}

	mutex_unlock(&g_mt[id]);

	ISP_PR_INFO("done\n");
	return status;
}

static int config_sensor(unsigned int id, unsigned int res, unsigned int flag)
{
	struct sensor_module_t *module;
	int status = 0;

	ISP_PR_INFO("id = %d\n", id);

	if (id >= MAX_SENSOR_COUNT)
		return -1;

	module = &g_sensor[id].c_module;
	g_sensor[id].config_id = res;

	mutex_lock(&g_mt[id]);

	if (g_sensor[id].mode == SENSOR_DRIVER_MODE_STATIC
	&& module->config_sensor)
		status = module->config_sensor(module, res, flag);
	else {
		ISP_PR_ERR("unknown mode");
		status = -1;
	}

	mutex_unlock(&g_mt[id]);

	ISP_PR_INFO("done\n");
	return status;
}

static int start_sensor(unsigned int id, int config_phy)
{
	struct sensor_module_t *module;
	int status = 0;

	ISP_PR_INFO("id = %d\n", id);

	if (id >= MAX_SENSOR_COUNT)
		return -1;

	module = &g_sensor[id].c_module;

	mutex_lock(&g_mt[id]);

	if (g_sensor[id].mode == SENSOR_DRIVER_MODE_STATIC
	&& module->start_sensor)
		status = module->start_sensor(module, config_phy);
	else {
		ISP_PR_ERR("unknown mode");
		status = -1;
	}

	mutex_unlock(&g_mt[id]);

	ISP_PR_INFO("done\n");
	return status;
}

static int stop_sensor(unsigned int id)
{
	struct sensor_module_t *module;
	int status = 0;

	ISP_PR_INFO("id = %d\n", id);

	if (id >= MAX_SENSOR_COUNT)
		return -1;

	module = &g_sensor[id].c_module;

	mutex_lock(&g_mt[id]);

	if (g_sensor[id].mode == SENSOR_DRIVER_MODE_STATIC
	&& module->start_sensor) {
		ISP_PR_INFO("%p\n", (void *)(size_t) module->stop_sensor);
		status = module->stop_sensor(module);
	} else {
		ISP_PR_ERR("unknown mode");
		status = -1;
	}

	mutex_unlock(&g_mt[id]);

	ISP_PR_INFO("done\n");
	return status;
}

static int stream_on_sensor(unsigned int id)
{
	struct sensor_module_t *module;
	int status = 0;

	ISP_PR_INFO("id = %d\n", id);

	if (id >= MAX_SENSOR_COUNT)
		return -1;

	module = &g_sensor[id].c_module;

	mutex_lock(&g_mt[id]);

	if (g_sensor[id].mode == SENSOR_DRIVER_MODE_STATIC) {
		ISP_PR_INFO("SENSOR_IF stop_sensor %p\n",
				(void *)(size_t) module->stop_sensor);
		if (module->stream_on_sensor)
			status = module->stream_on_sensor(module);
		else
			ISP_PR_INFO("sensor doesn't implement streamon\n");
	} else {
		ISP_PR_ERR("fail unknown mode, %d\n",
			g_sensor[id].mode);
		status = -1;
	}

	mutex_unlock(&g_mt[id]);

	ISP_PR_INFO("done\n");
	return status;
}

static int get_gain_limit(unsigned int id, unsigned int *min_gain,
		unsigned int *max_gain)
{
	struct sensor_module_t *module;
	int status = 0;

	ISP_PR_INFO("id = %d\n", id);

	if (id >= MAX_SENSOR_COUNT)
		return -1;

	module = &g_sensor[id].c_module;

	mutex_lock(&g_mt[id]);

	if (g_sensor[id].mode == SENSOR_DRIVER_MODE_STATIC
	&& module->get_gain_limit) {
		status = module->get_gain_limit(module,
				min_gain, max_gain);
	} else {
		ISP_PR_ERR("unknown mode");
		status = -1;
	}

	mutex_unlock(&g_mt[id]);

	ISP_PR_INFO("done\n");
	return status;
}

static int get_integration_time_limit(unsigned int id,
			unsigned int *min_integration_time,
			unsigned int *max_integration_time)
{
	struct sensor_module_t *module;
	int status = 0;

	ISP_PR_INFO("device_id = %d\n", id);

	if (id >= MAX_SENSOR_COUNT)
		return -1;

	module = &g_sensor[id].c_module;

	mutex_lock(&g_mt[id]);

	if (g_sensor[id].mode == SENSOR_DRIVER_MODE_STATIC
	&& module->get_integration_time_limit) {
		status = module->get_integration_time_limit(module,
					min_integration_time,
					max_integration_time);
	} else {
		ISP_PR_ERR("error unknown mode");
		status = -1;
	}

	mutex_unlock(&g_mt[id]);

	ISP_PR_INFO("done\n");
	return status;
}

static int get_caps(unsigned int id, int prf_id, void *caps)
{
	struct sensor_module_t *module;
	int status = 0;

	if (id >= MAX_SENSOR_COUNT)
		return -1;

	module = &g_sensor[id].c_module;
	mutex_lock(&g_mt[id]);

	if (g_sensor[id].mode == SENSOR_DRIVER_MODE_STATIC &&
	    module->get_caps) {
		status = module->get_caps(module, caps, prf_id);
	} else {
		ISP_PR_ERR("unknown mode");
		status = -1;
	}

	mutex_unlock(&g_mt[id]);

	return status;
}

static int support_af(unsigned int id)
{
	struct sensor_module_t *module;
	int status = 0;
	unsigned int ret;

	if (id >= MAX_SENSOR_COUNT)
		return -1;

	module = &g_sensor[id].c_module;

	mutex_lock(&g_mt[id]);

	if (g_sensor[id].mode == SENSOR_DRIVER_MODE_STATIC
	&& module->support_af) {
		module->support_af(module, &ret);
		status = (int) ret;
	} else {
		ISP_PR_ERR("unknown mode");
		status = -1;
	}

	mutex_unlock(&g_mt[id]);

	return (int) status;
}

int get_emb_prop(unsigned int id, bool *supported,
		struct _SensorEmbProp_t *embProp)
{
	struct sensor_module_t *module;
	int status = 0;

	ISP_PR_INFO("id=%d\n", id);

	if (id >= MAX_SENSOR_COUNT)
		return -1;

	module = &g_sensor[id].c_module;

	mutex_lock(&g_mt[id]);

	if (g_sensor[id].mode == SENSOR_DRIVER_MODE_STATIC
	&& module->get_emb_prop)
		status = module->get_emb_prop(module, supported, embProp);
	else if (g_sensor[id].mode != SENSOR_DRIVER_MODE_STATIC) {
		ISP_PR_ERR("fail bad mode %u\n", g_sensor[id].mode);
		status = -1;
	} else {//module->get_emb_prop is NULL
		if (supported)
			*supported = FALSE;
		ISP_PR_INFO("sensor doesn't support\n");
		status = 0;
	}

	mutex_unlock(&g_mt[id]);

	ISP_PR_INFO("done, ret %i\n", status);
	return status;
}

static struct sensor_res_fps_t *get_res_fps(unsigned int id)
{
	struct sensor_module_t *module;
	int status = 0;
	unsigned int i = 0;
	unsigned int max_count = RES_FPS_MAX;
	struct sensor_res_fps_t *sensor_res_fps;

	if (id >= MAX_SENSOR_COUNT)
		return NULL;

	module = &g_sensor[id].c_module;

	mutex_lock(&g_mt[id]);

	sensor_res_fps = &g_sensor[id].sensor_res_fps;
	memset(sensor_res_fps, 0, sizeof(*sensor_res_fps));

	ISP_PR_INFO("mode=%d, res_fps= %p\n",
		g_sensor[id].mode, module->get_res_fps);

	while (max_count--) {
		if (g_sensor[id].mode == SENSOR_DRIVER_MODE_STATIC
		&& module->get_res_fps) {
			status = module->get_res_fps(module,
					RES_FPS_MAX - (max_count + 1),
					&sensor_res_fps->res_fps[i]);

			if (status == RET_SUCCESS) {
				if (sensor_res_fps->res_fps[i].valid == FALSE)
					continue;
				else
					i++;
			} else {
				sensor_res_fps->id = id;
				sensor_res_fps->count = i;
				break;
			}
		} else {
			ISP_PR_INFO("no valid resolution\n");
		}
	}

	mutex_unlock(&g_mt[id]);

	return sensor_res_fps;
}

static int get_sensor_hw_parameter(unsigned int id,
			struct sensor_hw_parameter *para)
{
	struct sensor_module_t *module;
	int status = 0;

	ISP_PR_INFO("id=%d\n", id);

	if (id >= MAX_SENSOR_COUNT)
		return -1;

	module = &g_sensor[id].c_module;

	//mutex_lock(&g_mt[id]);

	if (g_sensor[id].mode == SENSOR_DRIVER_MODE_STATIC
	&& module->get_sensor_hw_parameter)
		status = module->get_sensor_hw_parameter(module, para);
	else {
		ISP_PR_ERR("fail bad mode\n");
		status = -1;
	}

	//mutex_unlock(&g_mt[id]);

	ISP_PR_INFO("done, ret %u\n", status);
	return status;
}

static int get_i2c_config(unsigned int id,
		struct _ScriptI2cArg_t __maybe_unused *i2c_config)
{
	struct sensor_module_t *module;
	int status = 0;

	if (id >= MAX_SENSOR_COUNT)
		return -1;

	module = &g_sensor[id].c_module;

	mutex_lock(&g_mt[id]);

	if (g_sensor[id].mode == SENSOR_DRIVER_MODE_STATIC
	&& module->get_i2c_config) {
		i2c_config->deviceId = module->hw->sensor_i2c_index;
		i2c_config->enRestart = 1;
		status = module->get_i2c_config(module,
				(unsigned int *) &i2c_config->deviceAddr,
				(unsigned int *) &i2c_config->deviceAddrType,
				(unsigned int *) &i2c_config->regAddrType);
	} else {
		ISP_PR_ERR("error unknown mode");
		status = -1;
	}

	mutex_unlock(&g_mt[id]);

	return status;
}

static int get_fw_script_len(unsigned int id, unsigned int *len)
{
	return fw_flen(g_sensor[id].fw_path, len);
}

static int get_fw_script_ctrl(unsigned int id,
	struct _isp_fw_script_ctrl_t *ctrl)
{
	struct sensor_module_t *module;
	int status = 0;

	fw_fread(g_sensor[id].fw_path, (char *)ctrl->script_buf, &ctrl->length);
	module = &g_sensor[id].c_module;

	mutex_lock(&g_mt[id]);

	if (g_sensor[id].mode == SENSOR_DRIVER_MODE_STATIC
	&& module->start_sensor)
		status = module->get_fw_script_ctrl(module, ctrl);
	else {
		ISP_PR_ERR("fail unknown mode %d\n", g_sensor[id].mode);
		status = -1;
	}

	mutex_unlock(&g_mt[id]);

	return status;
}

static int sensor_ioctl_static(unsigned int cmd, void *arg)
{
	int status = 0;
	unsigned int id = 0;

	if (cmd == SENSOR_IOCTL_INIT) {
		struct amdisp_sensor_init *pinit = arg;
		unsigned int i;

		if (pinit->sensor_info.name_size == 0
			|| pinit->sensor_info.name_size >
			sizeof(pinit->sensor_info.name))
			return -1;

		/*find the sensor index of the g_sensor_module */
		for (i = 0;
			i < ARRAY_SIZE(g_sensor_module); i++) {
			if (0 ==
				memcmp(pinit->sensor_info.name,
					g_sensor_module[i]->name,
					pinit->sensor_info.name_size)) {
				id = pinit->sensor_info.sensor_id;
				memcpy(&g_sensor[id].c_module,
					g_sensor_module[i],
					sizeof(struct sensor_module_t));
				g_sensor[id].c_module.hw = &g_sensor[id].hw;

				g_sensor[id].c_module.id = id;

				memset(&g_sensor[id].c_module.context, 0,
					sizeof(g_sensor[id].c_module.context));

				ISP_PR_INFO("Get sensor:%s\n",
					g_sensor_module[i]->name);
				return 0;
			}
		}

		return -1;
	}

	return status;
}

static int sensor_ioctl(unsigned int cmd, void *arg)
{
	unsigned int *pmode;
	unsigned int id = MAX_SENSOR_COUNT;
	int status = -1;
	struct amdisp_sensor_init *pinit = (struct amdisp_sensor_init *)arg;

	ISP_PR_INFO("cmd=%d\n", cmd);

	if (cmd == SENSOR_IOCTL_INIT) {
		pmode = (unsigned int *) arg;

		id = pinit->sensor_info.sensor_id;

		if (id >= MAX_SENSOR_COUNT) {
			ISP_PR_ERR("bad cid %u\n", id);
			return -1;
		}

		memset(g_sensor[id].fw_path, 0, sizeof(g_sensor[id].fw_path));
		snprintf(g_sensor[id].fw_path, sizeof(g_sensor[id].fw_path),
			"%s%s%s%s", SENSOR_SCRIPT_PATH,
			FW_SCRIPT_FILE_NAME_PREFIX, pinit->sensor_info.name,
			FW_SCRIPT_FILE_NAME_SUFFIX);
		ISP_PR_INFO("fw path %s\n", g_sensor[id].fw_path);

		if (*pmode == SENSOR_DRIVER_MODE_STATIC) {
			g_sensor[0].mode = SENSOR_DRIVER_MODE_STATIC;
			g_sensor[0].hw.sensor_i2c_index = SENSOR_ID0_GROUP0_BUS;
			g_sensor[0].hw.sensor_phy_i2c_index =
				SENSOR_ID0_GROUP1_BUS;
			g_sensor[1].mode = SENSOR_DRIVER_MODE_STATIC;
			g_sensor[1].hw.sensor_i2c_index = SENSOR_ID1_GROUP0_BUS;
			g_sensor[1].hw.sensor_phy_i2c_index =
				SENSOR_ID1_GROUP1_BUS;
			g_sensor[2].mode = SENSOR_DRIVER_MODE_STATIC;
			g_sensor[2].hw.sensor_i2c_index = SENSOR_ID2_GROUP0_BUS;
			g_sensor[2].hw.sensor_phy_i2c_index =
				SENSOR_ID2_GROUP1_BUS;

			id = pinit->sensor_info.sensor_id;
			g_sensor[id].mode = SENSOR_DRIVER_MODE_STATIC;
			status = sensor_ioctl_static(cmd, arg);
		}

		if (id < MAX_SENSOR_COUNT) {
			memset(&g_mt[id], 0, sizeof(struct mutex));
			mutex_init(&g_mt[id]);
		}
	} else if (cmd == SENSOR_IOCTL_RELEASE) {
		id = pinit->sensor_info.sensor_id;
		if (id >= MAX_SENSOR_COUNT) {
			ISP_PR_ERR("bad cid %u\n", id);
			return -1;
		};
		kfree(g_sensor[id].module);

		memset(&g_sensor, 0, sizeof(g_sensor));
		status = 0;
	}

	return status;
}

void init_camera_devices(void *p, char using_xml)
{
	struct isp_isp_module_if *ispm = p;
	struct amdisp_sensor_init init;

	if (ispm == NULL) {
		ISP_PR_ERR("failed to get isp module\n");
		return;
	}

	if (g_prop->sensor_id == SENSOR_IMX577) {
		memset(&init, 0, sizeof(init));
		init.mode = SENSOR_DRIVER_MODE_STATIC;
		init.b.user_buffer = NULL;
		init.sensor_info.sensor_id = CAMERA_ID_REAR;
		init.sensor_info.name_size = (unsigned int)strlen("imx577");
		memcpy(init.sensor_info.name, "imx577",
			init.sensor_info.name_size);
		get_sensor_interface()->ioctl(SENSOR_IOCTL_INIT, &init);

		memset(&init, 0, sizeof(init));
		init.mode = SENSOR_DRIVER_MODE_STATIC;
		init.b.user_buffer = NULL;
		init.sensor_info.sensor_id = CAMERA_ID_FRONT_LEFT;
		init.sensor_info.name_size = (unsigned int)strlen("imx577");
		memcpy(init.sensor_info.name, "imx577",
			init.sensor_info.name_size);
		get_sensor_interface()->ioctl(SENSOR_IOCTL_INIT, &init);

	} else if (g_prop->sensor_id == SENSOR_S5K3L6) {
		memset(&init, 0, sizeof(init));
		init.mode = SENSOR_DRIVER_MODE_STATIC;
		init.b.user_buffer = NULL;
		init.sensor_info.sensor_id = CAMERA_ID_REAR;
		init.sensor_info.name_size = (unsigned int)strlen("S5K3L6");
		memcpy(init.sensor_info.name, "S5K3L6",
			init.sensor_info.name_size);
		get_sensor_interface()->ioctl(SENSOR_IOCTL_INIT, &init);

		memset(&init, 0, sizeof(init));
		init.mode = SENSOR_DRIVER_MODE_STATIC;
		init.b.user_buffer = NULL;
		init.sensor_info.sensor_id = CAMERA_ID_FRONT_LEFT;
		init.sensor_info.name_size = (unsigned int)strlen("S5K3L6");
		memcpy(init.sensor_info.name, "S5K3L6",
			init.sensor_info.name_size);
		get_sensor_interface()->ioctl(SENSOR_IOCTL_INIT, &init);

	} else {
		ISP_PR_ERR("sensor not supported!");
	}
/*comment out since not necessary for mero.*/
//		memset(&init, 0, sizeof(init));
//		init.mode = mode;
//		init.b.user_buffer = NULL;
//		init.sensor_info.sensor_id = cid;
//		init.sensor_info.name_size =
//			(unsigned int) strlen(cinfo.vcm_name);
//		if (init.sensor_info.name_size) {
//			memcpy(init.sensor_info.name, cinfo.vcm_name,
//				init.sensor_info.name_size);
//			get_vcm_interface()->ioctl(VCM_IOCTL_INIT, &init);
//		}
//
//		memset(&init, 0, sizeof(init));
//		init.mode = mode;
//		init.b.user_buffer = NULL;
//		init.sensor_info.sensor_id = cid;
//		init.sensor_info.name_size =
//			(unsigned int) strlen(cinfo.flash_name);
//		if (init.sensor_info.name_size) {
//			memcpy(init.sensor_info.name, cinfo.flash_name,
//				init.sensor_info.name_size);
//			get_flash_interface()->ioctl(FLASH_IOCTL_INIT, &init);
//		}
//
//		memset(&init, 0, sizeof(init));
//		init.mode = mode;
//		init.b.user_buffer = NULL;
//		init.sensor_info.sensor_id = cid;
//		init.sensor_info.name_size =
//			(unsigned int) strlen(cinfo.storage_name);
//		if (init.sensor_info.name_size) {
//			memcpy(init.sensor_info.name, cinfo.storage_name,
//				init.sensor_info.name_size);
//			get_storage_interface()->ioctl(STORAGE_IOCTL_INIT,
//						&init);
//		}

	ISP_PR_INFO("done\n");
}

void update_fw_gv(struct sensor_module_t *module,
		unsigned int id, unsigned int count, ...)
{
	unsigned int i;
	struct _ScriptFuncArg_t fw_gv;
	va_list args;
	unsigned int in;
	enum fw_gv_type type;

	va_start(args, count);

	memset(&fw_gv, 0, sizeof(fw_gv));

	for (i = 0; i < count; i++) {
		in = va_arg(args, unsigned int);
		fw_gv.args[i] = in;
	}

	fw_gv.numArgs = count;

	va_end(args);

	if (id == 0)
		type = FW_GV_AE_ITIME;

	get_ifl_interface()->set_fw_gv(get_ifl_interface()->context,
				module->id, FW_GV_AE_ITIME, &fw_gv);
}

int exposure_control(unsigned int id, unsigned int new_gain,
		unsigned int new_integration_time,
		unsigned int *set_gain, unsigned int *set_integration)
{
	struct sensor_module_t *module;
	int status = 0;

	ISP_PR_INFO("id=%d, gain %u, itime %u\n",
			 id, new_gain, new_integration_time);

	if (id >= MAX_SENSOR_COUNT)
		return -1;

	module = &g_sensor[id].c_module;

	mutex_lock(&g_mt[id]);

	if (g_sensor[id].mode == SENSOR_DRIVER_MODE_STATIC
	&& module->exposure_control) {
		status = module->exposure_control(module, new_gain,
					new_integration_time,
					set_gain, set_integration);
	} else {
		ISP_PR_ERR("bad mode\n");
		status = -1;
	}

	mutex_unlock(&g_mt[id]);

	ISP_PR_INFO("done, ret %u\n", status);
	return status;
};

int get_current_exposure(unsigned int id, uint32_t *gain,
			uint32_t *integration_time)
{
	struct sensor_module_t *module;
	int status = 0;

	ISP_PR_INFO("id=%d\n", id);

	if (id >= MAX_SENSOR_COUNT)
		return -1;

	module = &g_sensor[id].c_module;

	mutex_lock(&g_mt[id]);

	if (g_sensor[id].mode == SENSOR_DRIVER_MODE_STATIC
	&& module->get_current_exposure) {
		status = module->get_current_exposure(module, gain,
						integration_time);
	} else {
		ISP_PR_ERR("bad mode\n");
		status = -1;
	}

	mutex_unlock(&g_mt[id]);

	ISP_PR_INFO("done, ret %u\n", status);
	return status;
}

int get_analog_gain(unsigned int id, unsigned int *gain)
{
	struct sensor_module_t *module;
	int status = 0;

	ISP_PR_INFO("id=%d\n", id);

	if (id >= MAX_SENSOR_COUNT)
		return -1;

	module = &g_sensor[id].c_module;

	mutex_lock(&g_mt[id]);

	if (g_sensor[id].mode == SENSOR_DRIVER_MODE_STATIC
	&& module->get_analog_gain)
		status = module->get_analog_gain(module, gain);
	else {
		ISP_PR_ERR("bad mode\n");
		status = -1;
	}

	mutex_unlock(&g_mt[id]);

	ISP_PR_INFO("done, ret %u\n", status);
	return status;
};

int get_itime(unsigned int id, unsigned int *itime)
{
	struct sensor_module_t *module;
	int status = 0;

	ISP_PR_INFO("id=%d\n", id);

	if (id >= MAX_SENSOR_COUNT)
		return -1;

	module = &g_sensor[id].c_module;

	mutex_lock(&g_mt[id]);

	if (g_sensor[id].mode == SENSOR_DRIVER_MODE_STATIC
	&& module->get_itime)
		status = module->get_itime(module, itime);
	else {
		ISP_PR_ERR("bad mode\n");
		status = -1;
	}

	mutex_unlock(&g_mt[id]);

	ISP_PR_INFO("done, ret %u\n", status);
	return status;
}

int sned_get_fralenline_cmd(unsigned int id)
{
	struct sensor_module_t *module;
	int status = 0;

	ISP_PR_INFO("id=%d\n", id);

	if (id >= MAX_SENSOR_COUNT)
		return -1;

	module = &g_sensor[id].c_module;

	mutex_lock(&g_mt[id]);

	if (g_sensor[id].mode == SENSOR_DRIVER_MODE_STATIC
	&& module->sned_get_fralenline_cmd)
		status = module->sned_get_fralenline_cmd(module);
	else {
		ISP_PR_ERR("bad mode\n");
		status = -1;
	}

	mutex_unlock(&g_mt[id]);

	ISP_PR_INFO("done, ret %u\n", status);
	return status;
}

int get_fralenline_regaddr(unsigned int id, unsigned short *hi_addr,
		unsigned short *lo_addr)
{
	struct sensor_module_t *module;
	int status = 0;

	ISP_PR_INFO("id=%d\n", id);

	if (id >= MAX_SENSOR_COUNT)
		return -1;

	module = &g_sensor[id].c_module;

	mutex_lock(&g_mt[id]);

	if (g_sensor[id].mode == SENSOR_DRIVER_MODE_STATIC
	&& module->get_fralenline_regaddr) {
		status = module->get_fralenline_regaddr(module,
						hi_addr, lo_addr);
	} else {
		ISP_PR_ERR("bad mode\n");
		status = -1;
	}

	mutex_unlock(&g_mt[id]);

	ISP_PR_INFO("done, ret %u\n", status);
	return status;
}

int set_test_pattern(unsigned int id, enum sensor_test_pattern mode)
{
	struct sensor_module_t *module;
	int status = 0;

	ISP_PR_INFO("id=%d\n", id);

	if (id >= MAX_SENSOR_COUNT)
		return -1;

	module = &g_sensor[id].c_module;

	mutex_lock(&g_mt[id]);
	if (g_sensor[id].mode == SENSOR_DRIVER_MODE_STATIC &&
	    module->set_test_pattern) {
		status = module->set_test_pattern(module, mode);
	} else {
		ISP_PR_ERR("bad mode\n");
		status = -1;
	}

	mutex_unlock(&g_mt[id]);

	ISP_PR_INFO("done, ret %i\n", status);
	return status;
}

int get_m2m_data(unsigned int id, struct _M2MTdb_t *pM2MTdb)
{
	struct sensor_module_t *module;
	int status = 0;

	ISP_PR_INFO("id=%d\n", id);
	if (id >= MAX_SENSOR_COUNT)
		return -1;

	module = &g_sensor[id].c_module;
	mutex_lock(&g_mt[id]);
	if (g_sensor[id].mode != SENSOR_DRIVER_MODE_STATIC) {
		ISP_PR_ERR("bad mode\n");
		status = RET_FAILURE;
	} else if (module->get_m2m_data == NULL) {
		ISP_PR_INFO("sensor doesn't support\n");
		status = RET_NOTSUPP;
	} else {
		status = module->get_m2m_data(module, pM2MTdb);
	}
	mutex_unlock(&g_mt[id]);

	ISP_PR_INFO("done, ret %i\n", status);
	return status;
}

static struct sensor_operation_t g_sensor_interface = {
	sensor_ioctl,
	get_i2c_config,
	get_fw_script_len,
	get_fw_script_ctrl,
	poweron_sensor,
	reset_sensor,
	config_sensor,
	start_sensor,
	stop_sensor,
	stream_on_sensor,
	get_caps,
	get_gain_limit,
	get_integration_time_limit,
	exposure_control,
	get_current_exposure,
	get_analog_gain,
	get_itime,
	support_af,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	get_emb_prop,
	get_res_fps,
	sned_get_fralenline_cmd,
	get_fralenline_regaddr,
	get_sensor_hw_parameter,
	set_test_pattern,
	get_m2m_data,
};

struct sensor_operation_t *get_sensor_interface(void)
{
	return &g_sensor_interface;
}
