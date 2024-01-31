/*
 * copyright 2014~2015 advanced micro devices, inc.
 *
 * permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "software"),
 * to deal in the software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the software, and to permit persons to whom the
 * software is furnished to do so, subject to the following conditions:
 *
 * the above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include "sensor_if.h"
#include "isp_hw_reg.h"

#define mmISP_SENSOR_FLASH_CTRL		0x1B15C

struct g_sensor_t {
	struct sensor_hw_config hw;
	enum sensor_driver_mode mode;
	struct amdisp_sensor_prog *pprog[SENSOR_IOCTL_MAX_NUM];
	uint32 *pgv;
	struct amdisp_sensor_init *init;
	struct dce_module *module;
	struct sensor_module_t c_module;
	struct sensor_res_fps_t sensor_res_fps;
	char xml_path[FILE_NAME_MAX_LEN];
	char fw_path[FILE_NAME_MAX_LEN];
	uint32 config_id;
};

static struct mutex g_mt[MAX_SENSOR_COUNT];

static struct g_sensor_t g_sensor[MAX_SENSOR_COUNT];

static struct sensor_module_t *g_sensor_module[] = {
	&g_imx208_driver,
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
		SENSOR_LOG_INFO("%s, path %s, fail\n", __func__, file_path);
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

static int32 poweron_sensor(uint32 id, bool_t on)
{
	struct sensor_module_t *module;
	int32 status = 0;

	SENSOR_LOG_INFO("-> SENSOR_IF %s, id = %d\n", __func__, id);

	if (id >= MAX_SENSOR_COUNT)
		return -1;

	module = &g_sensor[id].c_module;

	mutex_lock(&g_mt[id]);

	if (g_sensor[id].mode == SENSOR_DRIVER_MODE_STATIC
	&& module->poweron_sensor)
		status = module->poweron_sensor(module, on);
	else {
		SENSOR_LOG_ERROR("error %s unknown mode", __func__);
		status = -1;
	}

	mutex_unlock(&g_mt[id]);

	SENSOR_LOG_INFO("<- SENSOR_IF %s done\n", __func__);
	return status;
}

static int32 reset_sensor(uint32 id)
{
	struct sensor_module_t *module;
	int32 status = 0;

	SENSOR_LOG_INFO("-> SENSOR_IF %s, id = %d\n", __func__, id);

	if (id >= MAX_SENSOR_COUNT)
		return -1;

	module = &g_sensor[id].c_module;

	mutex_lock(&g_mt[id]);

	if (g_sensor[id].mode == SENSOR_DRIVER_MODE_STATIC
	&& module->reset_sensor)
		status = module->reset_sensor(module);
	else {
		SENSOR_LOG_ERROR("error %s unknown mode", __func__);
		status = -1;
	}

	mutex_unlock(&g_mt[id]);

	SENSOR_LOG_INFO("<- SENSOR_IF %s done\n", __func__);
	return status;
}

static int32 config_sensor(uint32 id, uint32 res, uint32 flag)
{
	struct sensor_module_t *module;
	int32 status = 0;

	SENSOR_LOG_INFO("-> SENSOR_IF %s, id = %d\n", __func__, id);

	if (id >= MAX_SENSOR_COUNT)
		return -1;

	module = &g_sensor[id].c_module;
	g_sensor[id].config_id = res;

	mutex_lock(&g_mt[id]);

	if (g_sensor[id].mode == SENSOR_DRIVER_MODE_STATIC
	&& module->config_sensor)
		status = module->config_sensor(module, res, flag);
	else {
		SENSOR_LOG_ERROR("error %s unknown mode", __func__);
		status = -1;
	}

	mutex_unlock(&g_mt[id]);

	SENSOR_LOG_INFO("<- SENSOR_IF %s done\n", __func__);
	return status;
}

static int32 start_sensor(uint32 id, bool_t config_phy)
{
	struct sensor_module_t *module;
	int32 status = 0;

	SENSOR_LOG_INFO("-> SENSOR_IF %s, id = %d\n", __func__, id);

	if (id >= MAX_SENSOR_COUNT)
		return -1;

	module = &g_sensor[id].c_module;

	mutex_lock(&g_mt[id]);

	if (g_sensor[id].mode == SENSOR_DRIVER_MODE_STATIC
	&& module->start_sensor)
		status = module->start_sensor(module, config_phy);
	else {
		SENSOR_LOG_ERROR("error %s unknown mode", __func__);
		status = -1;
	}

	mutex_unlock(&g_mt[id]);

	SENSOR_LOG_INFO("<- SENSOR_IF %s done\n", __func__);
	return status;
}

static int32 stop_sensor(uint32 id)
{
	struct sensor_module_t *module;
	int32 status = 0;

	SENSOR_LOG_INFO("-> SENSOR_IF %s, id = %d\n", __func__, id);

	if (id >= MAX_SENSOR_COUNT)
		return -1;

	module = &g_sensor[id].c_module;

	mutex_lock(&g_mt[id]);

	if (g_sensor[id].mode == SENSOR_DRIVER_MODE_STATIC
	&& module->start_sensor) {
		SENSOR_LOG_INFO("SENSOR_IF %s %p\n", __func__,
			(void *)(size_t) module->stop_sensor);
		status = module->stop_sensor(module);
	} else {
		SENSOR_LOG_ERROR("error %s unknown mode", __func__);
		status = -1;
	}

	mutex_unlock(&g_mt[id]);

	SENSOR_LOG_INFO("<- SENSOR_IF %s done\n", __func__);
	return status;
}

static int32 stream_on_sensor(uint32 id)
{
	struct sensor_module_t *module;
	int32 status = 0;

	SENSOR_LOG_INFO("-> SENSOR_IF %s, id = %d\n", __func__, id);

	if (id >= MAX_SENSOR_COUNT)
		return -1;

	module = &g_sensor[id].c_module;

	mutex_lock(&g_mt[id]);

	if (g_sensor[id].mode == SENSOR_DRIVER_MODE_STATIC) {
		SENSOR_LOG_INFO("SENSOR_IF stop_sensor %p\n",
				(void *)(size_t) module->stop_sensor);
		if (module->stream_on_sensor)
			status = module->stream_on_sensor(module);
		else
			SENSOR_LOG_INFO("sensor doesn't implement streamon\n");
	} else {
		SENSOR_LOG_ERROR("fail %s unknown mode, %d\n",  __func__,
			g_sensor[id].mode);
		status = -1;
	}

	mutex_unlock(&g_mt[id]);

	SENSOR_LOG_INFO("<- SENSOR_IF %s done\n", __func__);
	return status;
}

static int32 get_gain_limit(uint32 id, uint32 *min_gain, uint32 *max_gain)
{
	struct sensor_module_t *module;
	int32 status = 0;

	SENSOR_LOG_INFO("-> SENSOR_IF %s, id = %d\n", __func__, id);

	if (id >= MAX_SENSOR_COUNT)
		return -1;

	module = &g_sensor[id].c_module;

	mutex_lock(&g_mt[id]);

	if (g_sensor[id].mode == SENSOR_DRIVER_MODE_STATIC
	&& module->get_gain_limit) {
		status = module->get_gain_limit(module,
				min_gain, max_gain);
	} else {
		SENSOR_LOG_ERROR("error %s unknown mode",  __func__);
		status = -1;
	}

	mutex_unlock(&g_mt[id]);

	SENSOR_LOG_INFO("<- SENSOR_IF %s done\n", __func__);
	return status;
}

static int32 get_integration_time_limit(uint32 id,
			uint32 *min_integration_time,
			uint32 *max_integration_time)
{
	struct sensor_module_t *module;
	int32 status = 0;

	SENSOR_LOG_INFO("-> %s, device_id = %d\n", __func__, id);

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
		SENSOR_LOG_ERROR
			("error %s unknown mode",  __func__);
		status = -1;
	}

	mutex_unlock(&g_mt[id]);

	SENSOR_LOG_INFO("<- SENSOR_IF %s done\n",  __func__);
	return status;
}

static int32 get_caps(uint32 id, void *caps)
{
	isp_sensor_prop_t *sensor_caps;
	struct sensor_module_t *module;
	int32 status = 0;

	if (id >= MAX_SENSOR_COUNT)
		return -1;

	module = &g_sensor[id].c_module;

	mutex_lock(&g_mt[id]);

	if (g_sensor[id].mode == SENSOR_DRIVER_MODE_STATIC
	&& module->get_caps) {
		//struct res_fps_t res_fps;
		sensor_caps = (isp_sensor_prop_t *) caps;
		status = module->get_caps(module, caps);
		module->get_gain_limit(module, &sensor_caps->exposure.min_gain,
				&sensor_caps->exposure.max_gain);
		//todo add dig gain later
		sensor_caps->exposure.min_digi_gain =
				sensor_caps->exposure.min_gain;
		sensor_caps->exposure.max_digi_gain =
				sensor_caps->exposure.max_gain;
		module->get_integration_time_limit(module,
				&sensor_caps->exposure.min_integration_time,
				&sensor_caps->exposure.max_integration_time);
		sensor_caps->exposure_orig = sensor_caps->exposure;
		//module->get_res_fps(module, g_sensor[id].config_id, &res_fps);
		//sensor_caps->window.h_size = res_fps.width;
		//sensor_caps->window.v_size = res_fps.height;
		//fps = res_fps.fps;
		//hdr = res_fps.hdr_support;
		//module->get_res_fps(module, g_sensor[id].config_id,
		//		&sensor_caps->window.h_size,
		//		&sensor_caps->window.v_size, &fps, &hdr);
		//sensor_caps->frame_rate = fps * 1000;
	} else {
		SENSOR_LOG_ERROR("error %s unknown mode", __func__);
		status = -1;
	}

	mutex_unlock(&g_mt[id]);

	return status;
}

static int32 support_af(uint32 id)
{
	struct sensor_module_t *module;
	int32 status = 0;
	uint32 ret;

	if (id >= MAX_SENSOR_COUNT)
		return -1;

	module = &g_sensor[id].c_module;

	mutex_lock(&g_mt[id]);

	if (g_sensor[id].mode == SENSOR_DRIVER_MODE_STATIC
	&& module->support_af) {
		module->support_af(module, &ret);
		status = (int32) ret;
	} else {
		SENSOR_LOG_ERROR("error %s unknown mode", __func__);
		status = -1;
	}

	mutex_unlock(&g_mt[id]);

	return (int32) status;
}

int32 get_emb_prop(uint32 id, bool *supported, SensorEmbProp_t *embProp)
{
	struct sensor_module_t *module;
	int32 status = 0;

	SENSOR_LOG_INFO("-> %s, id=%d\n", __func__, id);

	if (id >= MAX_SENSOR_COUNT)
		return -1;

	module = &g_sensor[id].c_module;

	mutex_lock(&g_mt[id]);

	if (g_sensor[id].mode == SENSOR_DRIVER_MODE_STATIC
	&& module->get_emb_prop)
		status = module->get_emb_prop(module, supported, embProp);
	else if (g_sensor[id].mode != SENSOR_DRIVER_MODE_STATIC) {
		SENSOR_LOG_ERROR("%s fail bad mode %u\n", __func__,
				g_sensor[id].mode);
		status = -1;
	} else {//module->get_emb_prop is NULL
		if (supported)
			*supported = FALSE;
		SENSOR_LOG_INFO("in %s,sensor doesn't support\n", __func__);
		status = 0;
	}

	mutex_unlock(&g_mt[id]);

	SENSOR_LOG_INFO("<- %s done, ret %i\n", __func__, status);
	return status;
}

static struct sensor_res_fps_t *get_res_fps(uint32 id)
{
	struct sensor_module_t *module;
	int32 status = 0;
	uint32 i = 0;
	uint32 max_count = RES_FPS_MAX;
	struct sensor_res_fps_t *sensor_res_fps;

	if (id >= MAX_SENSOR_COUNT)
		return NULL;

	module = &g_sensor[id].c_module;

	mutex_lock(&g_mt[id]);

	sensor_res_fps = &g_sensor[id].sensor_res_fps;
	memset(sensor_res_fps, 0, sizeof(*sensor_res_fps));

	SENSOR_LOG_INFO("<- %s mode=%d, res_fps= %p\n",
		__func__, g_sensor[id].mode, module->get_res_fps);

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
			SENSOR_LOG_ERROR("error %s\n", __func__);
		}
	}

	mutex_unlock(&g_mt[id]);

	return sensor_res_fps;
}

static int32 get_sensor_hw_parameter(uint32 id,
			struct sensor_hw_parameter *para)
{
	struct sensor_module_t *module;
	int32 status = 0;

	SENSOR_LOG_INFO("-> %s,id=%d\n", __func__, id);

	if (id >= MAX_SENSOR_COUNT)
		return -1;

	module = &g_sensor[id].c_module;

	//mutex_lock(&g_mt[id]);

	if (g_sensor[id].mode == SENSOR_DRIVER_MODE_STATIC
	&& module->get_sensor_hw_parameter)
		status = module->get_sensor_hw_parameter(module, para);
	else {
		SENSOR_LOG_ERROR("%s fail bad mode\n", __func__);
		status = -1;
	}

	//mutex_unlock(&g_mt[id]);

	SENSOR_LOG_INFO("<- %s done, ret %u\n", __func__, status);
	return status;
}

static int32 get_i2c_config(uint32 id, ScriptI2cArg_t *i2c_config)
{
	struct sensor_module_t *module;
	struct dce_module *dce;
	int32 status = 0;

	if (id >= MAX_SENSOR_COUNT)
		return -1;

	module = &g_sensor[id].c_module;

	mutex_lock(&g_mt[id]);
	i2c_config = i2c_config;
	dce = NULL;

	if (g_sensor[id].mode == SENSOR_DRIVER_MODE_STATIC
	&& module->get_i2c_config) {
		i2c_config->deviceId = module->hw->sensor_i2c_index;
		i2c_config->enRestart = 1;
		status = module->get_i2c_config(module,
				(uint32 *) &i2c_config->deviceAddr,
				(uint32 *) &i2c_config->deviceAddrType,
				(uint32 *) &i2c_config->regAddrType);
	} else {
		SENSOR_LOG_ERROR("error %s unknown mode", __func__);
		status = -1;
	}

	mutex_unlock(&g_mt[id]);

	return status;
}

static int32 get_fw_script_len(uint32 id, uint32 *len)
{
	return fw_flen(g_sensor[id].fw_path, len);
}

static int32 get_fw_script_ctrl(uint32 id, isp_fw_script_ctrl_t *ctrl)
{
	struct sensor_module_t *module;
	int32 status = 0;

	fw_fread(g_sensor[id].fw_path, (char *)ctrl->script_buf, &ctrl->length);
	module = &g_sensor[id].c_module;

	mutex_lock(&g_mt[id]);

	if (g_sensor[id].mode == SENSOR_DRIVER_MODE_STATIC
	&& module->start_sensor)
		status = module->get_fw_script_ctrl(module, ctrl);
	else {
		SENSOR_LOG_ERROR("%s fail unknown mode %d\n",
				__func__, g_sensor[id].mode);
		status = -1;
	}

	mutex_unlock(&g_mt[id]);

	return status;
}

static int32 sensor_ioctl_static(unsigned int cmd, void *arg)
{
	int32 status = 0;
	uint32 id = 0;

	if (cmd == SENSOR_IOCTL_INIT) {
		struct amdisp_sensor_init *pinit = arg;
		uint32 i;

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

				SENSOR_LOG_INFO
				("%s find sensor:%s\n",  __func__,
				g_sensor_module[i]->name);
				return 0;
			}
		}

		return -1;
	}

	return status;
}

static int32 sensor_ioctl(uint32 cmd, void *arg)
{
	uint32 *pmode;
	uint32 id = MAX_SENSOR_COUNT;
	int32 status = -1;
	struct amdisp_sensor_init *pinit = (struct amdisp_sensor_init *)arg;

	SENSOR_LOG_INFO("%s cmd=%d\n", __func__, cmd);

	if (cmd == SENSOR_IOCTL_INIT) {
		pmode = (uint32 *) arg;

		id = pinit->sensor_info.sensor_id;

		if (id >= MAX_SENSOR_COUNT) {
			SENSOR_LOG_ERROR("in %s bad cid %u\n", __func__, id);
			return -1;
		}

		memset(g_sensor[id].fw_path, 0, sizeof(g_sensor[id].fw_path));
		snprintf(g_sensor[id].fw_path, sizeof(g_sensor[id].fw_path),
			"%s%s%s%s", SENSOR_SCRIPT_PATH,
			FW_SCRIPT_FILE_NAME_PREFIX, pinit->sensor_info.name,
			FW_SCRIPT_FILE_NAME_SUFFIX);
		SENSOR_LOG_INFO("fw path %s\n", g_sensor[id].fw_path);

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
			SENSOR_LOG_ERROR("in %s bad cid %u\n", __func__, id);
			return -1;
		};
		if (g_sensor[id].module)
			kfree(g_sensor[id].module);

		memset(&g_sensor, 0, sizeof(g_sensor));
		status = 0;
	}

	return status;
}

void init_camera_devices(void *p, char using_xml)
{
	struct isp_isp_module_if *ispm = p;
	enum camera_id cid;
	struct camera_dev_info cinfo;
	uint32 mode = SENSOR_DRIVER_MODE_STATIC;

	if (ispm == NULL) {
		SENSOR_LOG_ERROR("-><- %s fail for p\n", __func__);
		return;
	}

//	if (using_xml)
//		mode = SENSOR_DRIVER_MODE_XML_DCE;

	SENSOR_LOG_INFO("-> %s\n", __func__);

//	cid = CAMERA_ID_FRONT_LEFT;//CAMERA_ID_REAR;
	for (cid = CAMERA_ID_REAR; cid < CAMERA_ID_MAX; cid++) {
		struct amdisp_sensor_init init;

		if (ispm->get_camera_dev_info(ispm->context, cid, &cinfo)
			!= 0) {
			SENSOR_LOG_ERROR("get cid (%d)info fail\n", cid);
//			continue;
		};

		memset(&init, 0, sizeof(init));
		init.mode = mode;
		init.b.user_buffer = NULL;
		init.sensor_info.sensor_id = cid;
		init.sensor_info.name_size = (uint32) strlen("imx208");
		if (init.sensor_info.name_size) {
			memcpy(init.sensor_info.name, "imx208",
				init.sensor_info.name_size);
			get_sensor_interface()->ioctl(SENSOR_IOCTL_INIT, &init);
		}
	}
	SENSOR_LOG_INFO("<- %s\n", __func__);
}

void update_fw_gv(struct sensor_module_t *module, uint32 id, uint32 count, ...)
{
	unsigned int i;
	ScriptFuncArg_t fw_gv;
	va_list args;
	uint32 in;
	enum fw_gv_type type;

	va_start(args, count);

	memset(&fw_gv, 0, sizeof(fw_gv));

	for (i = 0; i < count; i++) {
		in = va_arg(args, uint32);
		fw_gv.args[i] = in;
	}

	fw_gv.numArgs = count;

	va_end(args);

	if (id == 0)
		type = FW_GV_AE_ITIME;

	get_ifl_interface()->set_fw_gv(get_ifl_interface()->context,
				module->id, FW_GV_AE_ITIME, &fw_gv);
}

int32 exposure_control(uint32 id, uint32 new_gain,
		uint32 new_integration_time,
		uint32 *set_gain, uint32 *set_integration)
{
	struct sensor_module_t *module;
	int32 status = 0;

	SENSOR_LOG_INFO("-> %s,id=%d, gain %u, itime %u\n",
			 __func__, id, new_gain, new_integration_time);

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
		SENSOR_LOG_ERROR("%s fail bad mode\n", __func__);
		status = -1;
	}

	mutex_unlock(&g_mt[id]);

	SENSOR_LOG_INFO("<- %s done, ret %u\n", __func__, status);
	return status;
};

int32 get_current_exposure(uint32 id, uint32_t *gain,
			uint32_t *integration_time)
{
	struct sensor_module_t *module;
	int32 status = 0;

	SENSOR_LOG_INFO("-> %s, id=%d\n", __func__, id);

	if (id >= MAX_SENSOR_COUNT)
		return -1;

	module = &g_sensor[id].c_module;

	mutex_lock(&g_mt[id]);

	if (g_sensor[id].mode == SENSOR_DRIVER_MODE_STATIC
	&& module->get_current_exposure) {
		status = module->get_current_exposure(module, gain,
						integration_time);
	} else {
		SENSOR_LOG_ERROR("%s fail bad mode\n", __func__);
		status = -1;
	}

	mutex_unlock(&g_mt[id]);

	SENSOR_LOG_INFO("<- %s done, ret %u\n", __func__, status);
	return status;
}

int32 get_analog_gain(uint32 id, uint32 *gain)
{
	struct sensor_module_t *module;
	int32 status = 0;

	SENSOR_LOG_INFO("-> %s,id=%d\n", __func__, id);

	if (id >= MAX_SENSOR_COUNT)
		return -1;

	module = &g_sensor[id].c_module;

	mutex_lock(&g_mt[id]);

	if (g_sensor[id].mode == SENSOR_DRIVER_MODE_STATIC
	&& module->get_analog_gain)
		status = module->get_analog_gain(module, gain);
	else {
		SENSOR_LOG_ERROR("%s fail bad mode\n", __func__);
		status = -1;
	}

	mutex_unlock(&g_mt[id]);

	SENSOR_LOG_INFO("<- %s done, ret %u\n",  __func__, status);
	return status;
};

int32 get_itime(uint32 id, uint32 *itime)
{
	struct sensor_module_t *module;
	int32 status = 0;

	SENSOR_LOG_INFO("-> %s,id=%d\n", __func__, id);

	if (id >= MAX_SENSOR_COUNT)
		return -1;

	module = &g_sensor[id].c_module;

	mutex_lock(&g_mt[id]);

	if (g_sensor[id].mode == SENSOR_DRIVER_MODE_STATIC
	&& module->get_itime)
		status = module->get_itime(module, itime);
	else {
		SENSOR_LOG_ERROR("%s fail bad mode\n",  __func__);
		status = -1;
	}

	mutex_unlock(&g_mt[id]);

	SENSOR_LOG_INFO("<- %s done, ret %u\n",  __func__, status);
	return status;
}

int32 get_runtime_fps(uint32 id, uint16 hi_value, uint16 lo_value, float *fps)
{
	struct sensor_module_t *module;
	int32 status = 0;

	SENSOR_LOG_INFO("-> %s, id=%d\n", __func__, id);

	if (id >= MAX_SENSOR_COUNT)
		return -1;

	module = &g_sensor[id].c_module;

	mutex_lock(&g_mt[id]);

	if (g_sensor[id].mode == SENSOR_DRIVER_MODE_STATIC
	&& module->get_runtime_fps) {
		status = module->get_runtime_fps(module,
					hi_value, lo_value, fps);
	} else {
		SENSOR_LOG_ERROR("%s fail bad mode\n", __func__);
		status = -1;
	}

	mutex_unlock(&g_mt[id]);

	SENSOR_LOG_INFO("<- %s done, ret %u\n", __func__, status);
	return status;
}

int32 sned_get_fralenline_cmd(uint32 id)
{
	struct sensor_module_t *module;
	int32 status = 0;

	SENSOR_LOG_INFO("-> %s, id=%d\n", __func__, id);

	if (id >= MAX_SENSOR_COUNT)
		return -1;

	module = &g_sensor[id].c_module;

	mutex_lock(&g_mt[id]);

	if (g_sensor[id].mode == SENSOR_DRIVER_MODE_STATIC
	&& module->sned_get_fralenline_cmd)
		status = module->sned_get_fralenline_cmd(module);
	else {
		SENSOR_LOG_ERROR("%s fail bad mode\n",  __func__);
		status = -1;
	}

	mutex_unlock(&g_mt[id]);

	SENSOR_LOG_INFO("<- %s done, ret %u\n", __func__, status);
	return status;
}

int32 get_fralenline_regaddr(uint32 id, uint16 *hi_addr, uint16 *lo_addr)
{
	struct sensor_module_t *module;
	int32 status = 0;

	SENSOR_LOG_INFO("-> %s, id=%d\n", __func__, id);

	if (id >= MAX_SENSOR_COUNT)
		return -1;

	module = &g_sensor[id].c_module;

	mutex_lock(&g_mt[id]);

	if (g_sensor[id].mode == SENSOR_DRIVER_MODE_STATIC
	&& module->get_fralenline_regaddr) {
		status = module->get_fralenline_regaddr(module,
						hi_addr, lo_addr);
	} else {
		SENSOR_LOG_ERROR("%s fail bad mode\n", __func__);
		status = -1;
	}

	mutex_unlock(&g_mt[id]);

	SENSOR_LOG_INFO("<- %s done, ret %u\n", __func__, status);
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
	get_runtime_fps,
	sned_get_fralenline_cmd,
	get_fralenline_regaddr,
	get_sensor_hw_parameter,
};

struct sensor_operation_t *get_sensor_interface(void)
{
	return &g_sensor_interface;
}
EXPORT_SYMBOL(get_sensor_interface);

//static int __init amd_sensor_init(void)
//{
//	return 0;
//}

//static void __exit amd_sensor_exit(void)
//{
//}

//module_init(amd_sensor_init);
//module_exit(amd_sensor_exit);

//MODULE_DESCRIPTION("AMD sensor device driver");
//MODULE_LICENSE("GPL");
