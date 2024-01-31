/*
 * Copyright (C) 2019-2021 Advanced Micro Devices, Inc. All rights reserved.
 */

#include "sensor_if.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "[Sensor][sensor_imx577]"

#define DEV_VENDOR				S0ny
#define DEV_NAME				imx577
#define GROUP1_NAME				sensor_phy
#define GROUP0_NAME				sensor

#define GROUP1_I2C_SLAVE_ADDR			0x14
#define GROUP1_I2C_REG_ADDR_SIZE		2
#define GROUP1_I2C_REG_SIZE			1
#define GROUP1_I2C_SLAVE_ADDR_MODE		I2C_SLAVE_ADDRESS_MODE_7BIT

#define GROUP0_I2C_SLAVE_ADDR			0x1a
#define GROUP0_I2C_REG_ADDR_SIZE		2
#define GROUP0_I2C_REG_SIZE			1
#define GROUP0_I2C_SLAVE_ADDR_MODE		I2C_SLAVE_ADDRESS_MODE_7BIT

#define ENABLE_PPI
#define IMX577_EMBEDDED_DATA_EN

//#define CONFIG_imx577_HDR_ITIME_SEPARATE
#define CONFIG_IMX577_HDR_ANALOG_GAIN_SEPARATE
#define CONFIG_IMX577_HDR_DIGITAL_GAIN_EQUAL

//Tuning parameters:
#define IMX577_FPS_ACCUACY_FACTOR		1000

//2*2 binning 3MP 13FPS
#define IMX577_RES_0_MIN_FPS			13
#define IMX577_RES_0_MAX_FPS			13
#define IMX577_RES_0_SYSCLK			180900000
#define IMX577_RES_0_WIDTH			2024
#define IMX577_RES_0_HEIGHT			1520
#define IMX577_RES_0_FRAME_LENGTH_LINES		1856
#define IMX577_RES_0_LINE_LENGTH_PCLK		30000

//2*2 BINNING 3MP 15FPS normal
#define IMX577_RES_1_MIN_FPS			15
#define IMX577_RES_1_MAX_FPS			15
#define IMX577_RES_1_SYSCLK			208800000
#define IMX577_RES_1_WIDTH			2024
#define IMX577_RES_1_HEIGHT			1520
#define IMX577_RES_1_FRAME_LENGTH_LINES		1856
#define IMX577_RES_1_LINE_LENGTH_PCLK		30000

//2*2 BINNING 3MP 30FPS normal
#define IMX577_RES_2_MIN_FPS			30
#define IMX577_RES_2_MAX_FPS			30
#define IMX577_RES_2_SYSCLK			208800000
#define IMX577_RES_2_WIDTH			2024
#define IMX577_RES_2_HEIGHT			1520
#define IMX577_RES_2_FRAME_LENGTH_LINES		3440
#define IMX577_RES_2_LINE_LENGTH_PCLK		8000

//2*2 BINNING 3MP 60FPS normal
#define IMX577_RES_3_MIN_FPS			60
#define IMX577_RES_3_MAX_FPS			60
#define IMX577_RES_3_SYSCLK			208800000
#define IMX577_RES_3_WIDTH			2024
#define IMX577_RES_3_HEIGHT			1520
#define IMX577_RES_3_FRAME_LENGTH_LINES		1860
#define IMX577_RES_3_LINE_LENGTH_PCLK		7400

// v2 binning 6MP 30FPS 4LANE
#define IMX577_RES_4_MIN_FPS			30
#define IMX577_RES_4_MAX_FPS			30
#define IMX577_RES_4_SYSCLK			208800000
#define IMX577_RES_4_WIDTH			4048
#define IMX577_RES_4_HEIGHT			1520
#define IMX577_RES_4_FRAME_LENGTH_LINES		3200
#define IMX577_RES_4_LINE_LENGTH_PCLK		8700

// v2 binning 6MP 60FPS 4LANE
#define IMX577_RES_5_MIN_FPS			60
#define IMX577_RES_5_MAX_FPS			60
#define IMX577_RES_5_SYSCLK			208800000
#define IMX577_RES_5_WIDTH			4048
#define IMX577_RES_5_HEIGHT			1520
#define IMX577_RES_5_FRAME_LENGTH_LINES		1600
#define IMX577_RES_5_LINE_LENGTH_PCLK		8700

// full 12MP 30FPS 4LANE
#define IMX577_RES_6_MIN_FPS			30
#define IMX577_RES_6_MAX_FPS			30
#define IMX577_RES_6_SYSCLK			105000000
#define IMX577_RES_6_WIDTH			4056
#define IMX577_RES_6_HEIGHT			3040
#define IMX577_RES_6_FRAME_LENGTH_LINES		3102
#define IMX577_RES_6_LINE_LENGTH_PCLK		4512

// full 12MP 3.9FPS
#define IMX577_RES_7_MIN_FPS			4
#define IMX577_RES_7_MAX_FPS			4
#define IMX577_RES_7_SYSCLK			45000000
#define IMX577_RES_7_WIDTH			4056
#define IMX577_RES_7_HEIGHT			3040
#define IMX577_RES_7_FRAME_LENGTH_LINES		3102
#define IMX577_RES_7_LINE_LENGTH_PCLK		15000

//v2 BINNING 6MP 3.9FPS normal
#define IMX577_RES_8_MIN_FPS			4
#define IMX577_RES_8_MAX_FPS			4
#define IMX577_RES_8_SYSCLK			45000000
#define IMX577_RES_8_WIDTH			4056
#define IMX577_RES_8_HEIGHT			1520
#define IMX577_RES_8_FRAME_LENGTH_LINES		3102
#define IMX577_RES_8_LINE_LENGTH_PCLK		15000

//from data sheet imx577
/*Physical maximum/minimum gain setting in register*/
#define IMX577_GAIN_FACTOR             1024
#define IMX577_MIN_GAIN_SETTING        0
#define IMX577_MAX_GAIN_SETTING        978
/*Physical maximum/minimum exposure line*/
#define IMX577_MIN_EXPOSURE_LINE       1
#define IMX577_MAX_EXPOSURE_LINE       0xFFFF
/*how many ISO is equal to 1.x gain*/
#define IMX577_BASE_ISO                100

/* Sensor AF property */
#define IMX577_VCM_MIN_POS             0
#define IMX577_VCM_MAX_POS             0
#define IMX577_VCM_INIT_POS            0
#define IMX577_MIN_AGAIN	(1.0f * GAIN_ACCURACY_FACTOR)
#define IMX577_MAX_AGAIN	(22.261f * GAIN_ACCURACY_FACTOR)
#define IMX577_MIN_DGAIN	(1.0f * GAIN_ACCURACY_FACTOR)
#define IMX577_MAX_DGAIN	(16.0f * GAIN_ACCURACY_FACTOR)

#define IMX577_LINE_COUNT_OFFSET	22
#define IMX577_MAX_LINE_COUNT	(65535 - IMX577_LINE_COUNT_OFFSET)

#define IMX577_FRA_LEN_LINE_HI_ADDR		0x0340
#define IMX577_FRA_LEN_LINE_LO_ADDR		0x0341

#define IMX577_FINE_INTEG_TIME		0x790u

struct _imx577_drv_context_t {
	uint32_t frame_length_lines;
	uint32_t line_length_pck;
	uint32_t logic_clock;
	uint32_t t_line_ns;	//unit:nanosecond
	uint32_t current_frame_length_lines;
	uint32_t max_line_count;
	uint32_t i_time;	//unit:microsecond
	uint32_t a_gain;	//real A_GAIN*1000
	};

typedef struct _imx577_exp_config {
	uint32_t coarse_integ_time;	//reg 0x0202,reg 0x0203
	uint32_t a_gain_global;		//reg 0x0204,reg 0x0205
} exp_config_t;

const static exp_config_t exp_config[] = {
	{0x0F2A, 0x0},	//res_0
	{0x0F2A, 0x0},	//res_1
	{0x06A2, 0x0},	//res_2
	{0x06A2, 0x0},	//res_3
	{0x062A, 0x0},	//res_4
	{0x062A, 0x0},	//res_5
	{0x0C6A, 0x0},	//res_6
	{0x0F2A, 0x0},	//res_7
	{0x0F2A, 0x0},	//res_8
};

/* approximation of floating point operations */
#define FP_SCALE	1000
#define FP_05		(FP_SCALE / 2)
#define INT_DIV(a, b)	((a) / (b) + ((a) % (b) >= (b) / 2))
#define FP_UP(v)	((v) * FP_SCALE)
#define FP_DN(v)	INT_DIV(v, FP_SCALE)

struct _imx577_drv_context_t g_imx577_context;
static void init_imx577_context(uint32_t res)
{
	switch (res) {
	case 0:
		g_imx577_context.frame_length_lines =
			IMX577_RES_0_FRAME_LENGTH_LINES;
		g_imx577_context.line_length_pck =
			IMX577_RES_0_LINE_LENGTH_PCLK;
		g_imx577_context.logic_clock = IMX577_RES_0_SYSCLK;
		break;
	case 1:
		g_imx577_context.frame_length_lines =
			IMX577_RES_1_FRAME_LENGTH_LINES;
		g_imx577_context.line_length_pck =
			IMX577_RES_1_LINE_LENGTH_PCLK;
		g_imx577_context.logic_clock = IMX577_RES_1_SYSCLK;
		break;
	case 2:
		g_imx577_context.frame_length_lines =
			IMX577_RES_2_FRAME_LENGTH_LINES;
		g_imx577_context.line_length_pck =
			IMX577_RES_2_LINE_LENGTH_PCLK;
		g_imx577_context.logic_clock = IMX577_RES_2_SYSCLK;
		break;
	case 3:
		g_imx577_context.frame_length_lines =
			IMX577_RES_3_FRAME_LENGTH_LINES;
		g_imx577_context.line_length_pck =
			IMX577_RES_3_LINE_LENGTH_PCLK;
		g_imx577_context.logic_clock = IMX577_RES_3_SYSCLK;
		break;
	case 4:
		g_imx577_context.frame_length_lines =
			IMX577_RES_4_FRAME_LENGTH_LINES;
		g_imx577_context.line_length_pck =
			IMX577_RES_4_LINE_LENGTH_PCLK;
		g_imx577_context.logic_clock = IMX577_RES_4_SYSCLK;
		break;
	case 5:
		g_imx577_context.frame_length_lines =
			IMX577_RES_5_FRAME_LENGTH_LINES;
		g_imx577_context.line_length_pck =
			IMX577_RES_5_LINE_LENGTH_PCLK;
		g_imx577_context.logic_clock =
			IMX577_RES_5_SYSCLK;
		break;
	case 6:
		g_imx577_context.frame_length_lines =
			IMX577_RES_6_FRAME_LENGTH_LINES;
		g_imx577_context.line_length_pck =
			IMX577_RES_6_LINE_LENGTH_PCLK;
		g_imx577_context.logic_clock = IMX577_RES_6_SYSCLK;
		break;
	case 7:
		g_imx577_context.frame_length_lines =
			IMX577_RES_7_FRAME_LENGTH_LINES;
		g_imx577_context.line_length_pck =
			IMX577_RES_7_LINE_LENGTH_PCLK;
		g_imx577_context.logic_clock = IMX577_RES_7_SYSCLK;
		break;
	case 8:
		g_imx577_context.frame_length_lines =
			IMX577_RES_8_FRAME_LENGTH_LINES;
		g_imx577_context.line_length_pck =
			IMX577_RES_8_LINE_LENGTH_PCLK;
		g_imx577_context.logic_clock = IMX577_RES_8_SYSCLK;
		break;
	default:
		return;
	}

	g_imx577_context.max_line_count =
		g_imx577_context.logic_clock * 4 /
		g_imx577_context.line_length_pck;
	g_imx577_context.t_line_ns =
		FP_UP(g_imx577_context.line_length_pck) /
		(g_imx577_context.logic_clock / 1000000) / 4;
	g_imx577_context.current_frame_length_lines =
		g_imx577_context.frame_length_lines;

}

static int set_gain_imx577(struct sensor_module_t *module,
		uint32_t new_gain, uint32_t *set_gain)
{
	uint32_t gain, set_gain_tmp;
	uint32_t gain_h, gain_l;


	gain = FP_DN(INT_DIV(FP_UP(1024000), new_gain) + FP_05);
	gain = 1024 - gain;

	gain_h = (gain >> 8) & 0x03;
	gain_l = (gain & 0x00ff);

	ISP_PR_INFO("new_gain = %u\n", gain);
	SENSOR_I2C_WRITE(module->hw->sensor_i2c_index, 0x0204, gain_h);
	SENSOR_I2C_WRITE(module->hw->sensor_i2c_index, 0x0205, gain_l);
	set_gain_tmp =
		FP_DN(INT_DIV(FP_UP(1024000), 1024 - gain) + FP_05);
	ISP_PR_INFO("set_gain_tmp = %u\n", set_gain_tmp);

	if (set_gain) {
		*set_gain = set_gain_tmp;
		ISP_PR_DBG("gain[%d] Reg(0x0204)=0x%x, Reg(0x0205)=0x%x\n",
			   gain, gain_h, gain_l);
	}

	return RET_SUCCESS;
}

static int get_analog_gain_imx577(struct sensor_module_t *module,
		uint32_t *gain)
{
	uint32_t temp_gain = 0;
	uint32_t temp_gain_h, temp_gain_l = 0;

	SENSOR_I2C_READ(module->hw->sensor_i2c_index, 0x0204,
		&temp_gain_h);
	SENSOR_I2C_READ(module->hw->sensor_i2c_index, 0x0205,
		&temp_gain_l);

	*gain = ((temp_gain_h & 0x03) << 8) | (temp_gain_l & 0xff);
	ISP_PR_INFO("get_gain_tmp[%u]\n", *gain);
	temp_gain =
		FP_DN(INT_DIV(FP_UP(1024000), 1024 - *gain) + FP_05);
	ISP_PR_INFO("analog gain[%u]\n", temp_gain);
	if (gain)
		*gain = temp_gain;

	return RET_SUCCESS;
}

/*
 * new_integration_time (unit: us)
 */
static int set_itime_imx577(struct sensor_module_t *module, uint32_t res,
		uint32_t new_integration_time,
		uint32_t *set_integration_time)
{
	uint32_t line_count;
	uint32_t line_count_h, line_count_l = 0;
	uint32_t fine0, fine1, fine;
	uint32_t t_line, line_length_pck;

	init_imx577_context(res);
	t_line = FP_DN(g_imx577_context.t_line_ns);
	line_length_pck = g_imx577_context.line_length_pck;
	ISP_PR_DBG("new_integration_time[%dus] t_line[%dus]\n",
		new_integration_time, t_line);
	SENSOR_I2C_READ(module->hw->sensor_i2c_index, 0x0200, &fine0);
	SENSOR_I2C_READ(module->hw->sensor_i2c_index, 0x0201, &fine1);
	fine = ((fine1 & 0xff) | ((fine0 & 0xff) << 8));
	line_count = FP_DN((FP_UP(new_integration_time) / t_line -
			FP_UP(fine) / line_length_pck + FP_05));
	line_count_h = (line_count >> 8) & 0xff;
	line_count_l = line_count & 0xff;
	SENSOR_I2C_WRITE(module->hw->sensor_i2c_index, 0x0202, line_count_h);
	SENSOR_I2C_WRITE(module->hw->sensor_i2c_index, 0x0203, line_count_l);

	if (set_integration_time) {
		*set_integration_time = new_integration_time;
		ISP_PR_DBG("*set_integration_time[%dus] line_count[%d],",
			*set_integration_time, line_count);
		ISP_PR_DBG("Reg(0x0202)=0x%x, Reg(0x0203)=0x%x\n",
			line_count_h, line_count_l);
	}


	return RET_SUCCESS;
}

/*
 * itime (unit: us)
 */
static int get_itime_imx577(struct sensor_module_t *module,
		uint32_t *itime)
{
	uint16_t line_count = 0;
	uint8_t line_count_h, line_count_l = 0;
	uint8_t fine0, fine1;
	uint32_t fine;

	init_imx577_context(module->context.res);
	SENSOR_I2C_READ(module->hw->sensor_i2c_index, 0x0202, &line_count_h);
	SENSOR_I2C_READ(module->hw->sensor_i2c_index, 0x0203, &line_count_l);
	line_count = ((line_count_h & 0xff) << 8) | (line_count_l & 0xff);
	SENSOR_I2C_READ(module->hw->sensor_i2c_index, 0x0200, &fine0);
	SENSOR_I2C_READ(module->hw->sensor_i2c_index, 0x0201, &fine1);
	fine = ((fine1 & 0xff) | ((uint32_t)(fine0 & 0xff) << 8));

	if (itime) {
		*itime = (line_count + fine /
			g_imx577_context.line_length_pck)*
			FP_DN(g_imx577_context.t_line_ns);
		ISP_PR_INFO("*itime[%dus], line_count[%d]\n",
			*itime, line_count);
	}

	return RET_SUCCESS;
}

/*
 * new_gain : 12345 means 12.345x
 * new_integration_time (unit: us)
 */
static int exposure_control_imx577(struct sensor_module_t *module,
		uint32_t new_gain,
		uint32_t new_integration_time,
		uint32_t *set_gain,
		uint32_t *set_integration)
{
	if (new_gain)
		set_gain_imx577(module, new_gain, set_gain);
	if (new_integration_time)
		set_itime_imx577(module, module->context.res,
				new_integration_time, set_integration);

	return RET_SUCCESS;
};

static int get_current_exposure_imx577(struct sensor_module_t *module,
		uint32_t *gain,
		uint32_t *integration_time)
{
	get_analog_gain_imx577(module, gain);
	get_itime_imx577(module, integration_time);

	return RET_SUCCESS;
}

static int poweron_imx577(struct sensor_module_t __maybe_unused *module,
	int __maybe_unused on)
{
	return RET_SUCCESS;
}

static int reset_imx577(struct sensor_module_t __maybe_unused *module)
{
	return RET_SUCCESS;
}

static int config_imx577(struct sensor_module_t *module, unsigned int res,
		unsigned int __maybe_unused flag)
{
	struct drv_context *drv_context = &module->context;
	drv_context->res = res;
	drv_context->flag = flag;

	ISP_PR_INFO("res %d\n", res);
	init_imx577_context(res);
	drv_context->frame_length_lines = g_imx577_context.frame_length_lines;
	drv_context->line_length_pck = g_imx577_context.line_length_pck;
	switch (res) {
	case 0:
		drv_context->t_line_numerator =
			IMX577_RES_0_LINE_LENGTH_PCLK;
		drv_context->t_line_denominator =
			IMX577_RES_0_SYSCLK * 4 / 1000000;
		drv_context->bitrate = 300;
		break;
	case 1:
		drv_context->t_line_numerator =
			IMX577_RES_1_LINE_LENGTH_PCLK;
		drv_context->t_line_denominator =
			IMX577_RES_1_SYSCLK * 4 / 1000000;
		drv_context->bitrate = 312;
		break;
	case 2:
		drv_context->t_line_numerator =
			IMX577_RES_2_LINE_LENGTH_PCLK;
		drv_context->t_line_denominator =
			IMX577_RES_2_SYSCLK * 4 / 1000000;
		drv_context->bitrate = 600;
		break;
	case 3:
		drv_context->t_line_numerator =
			IMX577_RES_3_LINE_LENGTH_PCLK;
		drv_context->t_line_denominator =
			IMX577_RES_3_SYSCLK * 4 / 1000000;
		drv_context->bitrate = 600;
		break;
	case 4:
		drv_context->t_line_numerator =
			IMX577_RES_4_LINE_LENGTH_PCLK;
		drv_context->t_line_denominator =
			IMX577_RES_4_SYSCLK * 4 / 1000000;
		drv_context->bitrate = 1050;
		break;
	case 5:
		drv_context->t_line_numerator =
			IMX577_RES_5_LINE_LENGTH_PCLK;
		drv_context->t_line_denominator =
			IMX577_RES_5_SYSCLK * 4 / 1000000;
		drv_context->bitrate = 1050;
		break;
	case 6:
		drv_context->t_line_numerator =
			IMX577_RES_6_LINE_LENGTH_PCLK;
		drv_context->t_line_denominator =
			IMX577_RES_6_SYSCLK * 4 / 1000000;
		drv_context->bitrate = 1050;
		break;
	case 7:
		drv_context->t_line_numerator =
			IMX577_RES_7_LINE_LENGTH_PCLK;
		drv_context->t_line_denominator =
			IMX577_RES_7_SYSCLK * 4 / 1000000;
		drv_context->bitrate = 300;
		break;
	case 8:
		drv_context->t_line_numerator =
			IMX577_RES_8_LINE_LENGTH_PCLK;
		drv_context->t_line_denominator =
			IMX577_RES_8_SYSCLK * 4 / 1000000;
		drv_context->bitrate = 300;
		break;
	default:
		ISP_PR_ERR("Unsupported res:%d\n", res);
		return RET_FAILURE;
	}

	drv_context->mode = 0;	// need recheck
	drv_context->max_torch_curr = FLASH_MAX_TORCH_CURR_LEVEL0;
	drv_context->max_flash_curr = FLASH_MAX_FLASH_CURR_LEVEL0;

	g_imx577_context.i_time = (exp_config[res].coarse_integ_time +
		IMX577_FINE_INTEG_TIME / g_imx577_context.line_length_pck) *
			FP_DN(g_imx577_context.t_line_ns);
	g_imx577_context.a_gain =
		FP_DN(INT_DIV(FP_UP(1024000), 1024 -
			exp_config[res].a_gain_global) + FP_05);

	ISP_PR_DBG("res[%u] frame_length_lines[%u] line_length_pck[%u],",
		drv_context->res, drv_context->frame_length_lines,
		drv_context->line_length_pck);
	ISP_PR_DBG("t_line_numerator[%u] t_line_denominator[%u],",
		drv_context->t_line_numerator,
		drv_context->t_line_denominator);
	ISP_PR_DBG("bitrate[%u] mode[%u] max_t_curr[%u] max_f_curr[%u]\n",
		drv_context->bitrate, drv_context->mode,
		drv_context->max_torch_curr, drv_context->max_flash_curr);
	ISP_PR_DBG("initial i_time[%dus],\n", g_imx577_context.i_time);
	ISP_PR_DBG("initial analog gain[%u]\n", g_imx577_context.a_gain);

	ISP_PR_INFO("success\n");

	return RET_SUCCESS;
}

static int check_i2c_connection(struct sensor_module_t *module)
{
	uint8_t id_h, id_l = 0;
	usleep_range(600, 700);//delay 0.6~0.7ms align sensor power sequence.

	ISP_PR_INFO("id %d, bus %u slave add: 0x%x\n",
		module->id,
		module->hw->sensor_i2c_index, GROUP0_I2C_SLAVE_ADDR);
		SENSOR_I2C_READ(module->hw->sensor_i2c_index, 0x0016, &id_h);
		SENSOR_I2C_READ(module->hw->sensor_i2c_index, 0x0017, &id_l);

	if ((id_h == 0x05) && (id_l == 0x77)) {
		ISP_PR_PC("Sensor id match: 0x0577\n");
	} else {
		ISP_PR_ERR("id mismatch Reg(0x0016)=0x%x Reg(0x0017)=0x%x\n",
			id_h, id_l);
		return RET_FAILURE;
	}
	return RET_SUCCESS;
}

#define AIDT_SLEEP msleep

static int get_caps_imx577(struct sensor_module_t *module,
	struct _isp_sensor_prop_t *caps, int prf_id);

static int config_common1_imx577(struct sensor_module_t *module)
{
	unsigned char bus = module->hw->sensor_i2c_index;

	//Image Quality adjustment setting
	SENSOR_I2C_WRITE(bus, 0x9827, 0x20);
	SENSOR_I2C_WRITE(bus, 0x9830, 0x0A);
	SENSOR_I2C_WRITE(bus, 0x9833, 0x0A);
	SENSOR_I2C_WRITE(bus, 0x9834, 0x32);
	SENSOR_I2C_WRITE(bus, 0x9837, 0x22);
	SENSOR_I2C_WRITE(bus, 0x983C, 0x04);
	SENSOR_I2C_WRITE(bus, 0x983F, 0x0A);
	SENSOR_I2C_WRITE(bus, 0x994F, 0x00);
	SENSOR_I2C_WRITE(bus, 0x9A48, 0x06);
	SENSOR_I2C_WRITE(bus, 0x9A49, 0x06);
	SENSOR_I2C_WRITE(bus, 0x9A4A, 0x06);
	SENSOR_I2C_WRITE(bus, 0x9A4B, 0x06);
	SENSOR_I2C_WRITE(bus, 0x9A4E, 0x03);
	SENSOR_I2C_WRITE(bus, 0x9A4F, 0x03);
	SENSOR_I2C_WRITE(bus, 0x9A54, 0x03);
	SENSOR_I2C_WRITE(bus, 0x9A66, 0x03);
	SENSOR_I2C_WRITE(bus, 0x9A67, 0x03);
	SENSOR_I2C_WRITE(bus, 0xA2C9, 0x02);
	SENSOR_I2C_WRITE(bus, 0xA2CB, 0x02);
	SENSOR_I2C_WRITE(bus, 0xA2CD, 0x02);
	SENSOR_I2C_WRITE(bus, 0xB249, 0x3F);
	SENSOR_I2C_WRITE(bus, 0xB24F, 0x3F);
	SENSOR_I2C_WRITE(bus, 0xB290, 0x3F);
	SENSOR_I2C_WRITE(bus, 0xB293, 0x3F);
	SENSOR_I2C_WRITE(bus, 0xB296, 0x3F);
	SENSOR_I2C_WRITE(bus, 0xB299, 0x3F);
	SENSOR_I2C_WRITE(bus, 0xB2A2, 0x3F);
	SENSOR_I2C_WRITE(bus, 0xB2A8, 0x3F);
	SENSOR_I2C_WRITE(bus, 0xB2A9, 0x0D);
	SENSOR_I2C_WRITE(bus, 0xB2AA, 0x0D);
	SENSOR_I2C_WRITE(bus, 0xB2AB, 0x3F);
	SENSOR_I2C_WRITE(bus, 0xB2BA, 0x2F);
	SENSOR_I2C_WRITE(bus, 0xB2BB, 0x2F);
	SENSOR_I2C_WRITE(bus, 0xB2BC, 0x2F);
	SENSOR_I2C_WRITE(bus, 0xB2BD, 0x10);
	SENSOR_I2C_WRITE(bus, 0xB2C0, 0x3F);
	SENSOR_I2C_WRITE(bus, 0xB2C3, 0x3F);
	SENSOR_I2C_WRITE(bus, 0xB2D2, 0x3F);
	SENSOR_I2C_WRITE(bus, 0xB2DE, 0x20);
	SENSOR_I2C_WRITE(bus, 0xB2DF, 0x20);
	SENSOR_I2C_WRITE(bus, 0xB2E0, 0x20);
	SENSOR_I2C_WRITE(bus, 0xB2EA, 0x3F);
	SENSOR_I2C_WRITE(bus, 0xB2ED, 0x3F);
	SENSOR_I2C_WRITE(bus, 0xB2EE, 0x3F);
	SENSOR_I2C_WRITE(bus, 0xB2EF, 0x3F);
	SENSOR_I2C_WRITE(bus, 0xB2F0, 0x2F);
	SENSOR_I2C_WRITE(bus, 0xB2F1, 0x2F);
	SENSOR_I2C_WRITE(bus, 0xB2F2, 0x2F);
	SENSOR_I2C_WRITE(bus, 0xB2F9, 0x0E);
	SENSOR_I2C_WRITE(bus, 0xB2FA, 0x0E);
	SENSOR_I2C_WRITE(bus, 0xB2FB, 0x0E);
	SENSOR_I2C_WRITE(bus, 0xB759, 0x01);
	SENSOR_I2C_WRITE(bus, 0xB765, 0x3F);
	SENSOR_I2C_WRITE(bus, 0xB76B, 0x3F);
	SENSOR_I2C_WRITE(bus, 0xB7B3, 0x03);
	SENSOR_I2C_WRITE(bus, 0xB7B5, 0x03);
	SENSOR_I2C_WRITE(bus, 0xB7B7, 0x03);
	SENSOR_I2C_WRITE(bus, 0xB7BF, 0x03);
	SENSOR_I2C_WRITE(bus, 0xB7C1, 0x03);
	SENSOR_I2C_WRITE(bus, 0xB7C3, 0x03);
	SENSOR_I2C_WRITE(bus, 0xB7EF, 0x02);
	SENSOR_I2C_WRITE(bus, 0xB7F5, 0x1F);
	SENSOR_I2C_WRITE(bus, 0xB7F7, 0x1F);
	SENSOR_I2C_WRITE(bus, 0xB7F9, 0x1F);

	//External Clock Setting
	SENSOR_I2C_WRITE(bus, 0x0136, 0x18);
	SENSOR_I2C_WRITE(bus, 0x0137, 0x00);

	//Register version
	SENSOR_I2C_WRITE(bus, 0x3C7E, 0x02);
	SENSOR_I2C_WRITE(bus, 0x3C7F, 0x02);

	//Global Setting
	//Basic Config
	SENSOR_I2C_WRITE(bus, 0x38A8, 0x1F);
	SENSOR_I2C_WRITE(bus, 0x38A9, 0xFF);
	SENSOR_I2C_WRITE(bus, 0x38AA, 0x1F);
	SENSOR_I2C_WRITE(bus, 0x38AB, 0xFF);
	SENSOR_I2C_WRITE(bus, 0x55D4, 0x00);
	SENSOR_I2C_WRITE(bus, 0x55D5, 0x00);
	SENSOR_I2C_WRITE(bus, 0x55D6, 0x07);
	SENSOR_I2C_WRITE(bus, 0x55D7, 0xFF);
	SENSOR_I2C_WRITE(bus, 0x55E8, 0x07);
	SENSOR_I2C_WRITE(bus, 0x55E9, 0xFF);
	SENSOR_I2C_WRITE(bus, 0x55EA, 0x00);
	SENSOR_I2C_WRITE(bus, 0x55EB, 0x00);
	SENSOR_I2C_WRITE(bus, 0x575C, 0x07);
	SENSOR_I2C_WRITE(bus, 0x575D, 0xFF);
	SENSOR_I2C_WRITE(bus, 0x575E, 0x00);
	SENSOR_I2C_WRITE(bus, 0x575F, 0x00);
	SENSOR_I2C_WRITE(bus, 0x5764, 0x00);
	SENSOR_I2C_WRITE(bus, 0x5765, 0x00);
	SENSOR_I2C_WRITE(bus, 0x5766, 0x07);
	SENSOR_I2C_WRITE(bus, 0x5767, 0xFF);
	SENSOR_I2C_WRITE(bus, 0x5974, 0x04);
	SENSOR_I2C_WRITE(bus, 0x5975, 0x01);
	SENSOR_I2C_WRITE(bus, 0x5F10, 0x09);
	SENSOR_I2C_WRITE(bus, 0x5F11, 0x92);
	SENSOR_I2C_WRITE(bus, 0x5F12, 0x32);
	SENSOR_I2C_WRITE(bus, 0x5F13, 0x72);
	SENSOR_I2C_WRITE(bus, 0x5F14, 0x16);
	SENSOR_I2C_WRITE(bus, 0x5F15, 0xBA);
	SENSOR_I2C_WRITE(bus, 0x5F17, 0x13);
	SENSOR_I2C_WRITE(bus, 0x5F18, 0x24);
	SENSOR_I2C_WRITE(bus, 0x5F19, 0x60);
	SENSOR_I2C_WRITE(bus, 0x5F1A, 0xE3);
	SENSOR_I2C_WRITE(bus, 0x5F1B, 0xAD);
	SENSOR_I2C_WRITE(bus, 0x5F1C, 0x74);
	SENSOR_I2C_WRITE(bus, 0x5F2D, 0x25);
	SENSOR_I2C_WRITE(bus, 0x5F5C, 0xD0);
	SENSOR_I2C_WRITE(bus, 0x6A22, 0x00);
	SENSOR_I2C_WRITE(bus, 0x6A23, 0x1D);
	SENSOR_I2C_WRITE(bus, 0x7BA8, 0x00);
	SENSOR_I2C_WRITE(bus, 0x7BA9, 0x00);
	SENSOR_I2C_WRITE(bus, 0x886B, 0x00);
	SENSOR_I2C_WRITE(bus, 0x9002, 0x0A);
	SENSOR_I2C_WRITE(bus, 0x9004, 0x1A);
	SENSOR_I2C_WRITE(bus, 0x9214, 0x93);
	SENSOR_I2C_WRITE(bus, 0x9215, 0x69);
	SENSOR_I2C_WRITE(bus, 0x9216, 0x93);
	SENSOR_I2C_WRITE(bus, 0x9217, 0x6B);
	SENSOR_I2C_WRITE(bus, 0x9218, 0x93);
	SENSOR_I2C_WRITE(bus, 0x9219, 0x6D);
	SENSOR_I2C_WRITE(bus, 0x921A, 0x57);
	SENSOR_I2C_WRITE(bus, 0x921B, 0x58);
	SENSOR_I2C_WRITE(bus, 0x921C, 0x57);
	SENSOR_I2C_WRITE(bus, 0x921D, 0x59);
	SENSOR_I2C_WRITE(bus, 0x921E, 0x57);
	SENSOR_I2C_WRITE(bus, 0x921F, 0x5A);
	SENSOR_I2C_WRITE(bus, 0x9220, 0x57);
	SENSOR_I2C_WRITE(bus, 0x9221, 0x5B);
	SENSOR_I2C_WRITE(bus, 0x9222, 0x93);
	SENSOR_I2C_WRITE(bus, 0x9223, 0x02);
	SENSOR_I2C_WRITE(bus, 0x9224, 0x93);
	SENSOR_I2C_WRITE(bus, 0x9225, 0x03);
	SENSOR_I2C_WRITE(bus, 0x9226, 0x93);
	SENSOR_I2C_WRITE(bus, 0x9227, 0x04);
	SENSOR_I2C_WRITE(bus, 0x9228, 0x93);
	SENSOR_I2C_WRITE(bus, 0x9229, 0x05);
	SENSOR_I2C_WRITE(bus, 0x922A, 0x98);
	SENSOR_I2C_WRITE(bus, 0x922B, 0x21);
	SENSOR_I2C_WRITE(bus, 0x922C, 0xB2);
	SENSOR_I2C_WRITE(bus, 0x922D, 0xDB);
	SENSOR_I2C_WRITE(bus, 0x922E, 0xB2);
	SENSOR_I2C_WRITE(bus, 0x922F, 0xDC);
	SENSOR_I2C_WRITE(bus, 0x9230, 0xB2);
	SENSOR_I2C_WRITE(bus, 0x9231, 0xDD);
	SENSOR_I2C_WRITE(bus, 0x9232, 0xB2);
	SENSOR_I2C_WRITE(bus, 0x9233, 0xE1);
	SENSOR_I2C_WRITE(bus, 0x9234, 0xB2);
	SENSOR_I2C_WRITE(bus, 0x9235, 0xE2);
	SENSOR_I2C_WRITE(bus, 0x9236, 0xB2);
	SENSOR_I2C_WRITE(bus, 0x9237, 0xE3);
	SENSOR_I2C_WRITE(bus, 0x9238, 0xB7);
	SENSOR_I2C_WRITE(bus, 0x9239, 0xB9);
	SENSOR_I2C_WRITE(bus, 0x923A, 0xB7);
	SENSOR_I2C_WRITE(bus, 0x923B, 0xBB);
	SENSOR_I2C_WRITE(bus, 0x923C, 0xB7);
	SENSOR_I2C_WRITE(bus, 0x923D, 0xBC);
	SENSOR_I2C_WRITE(bus, 0x923E, 0xB7);
	SENSOR_I2C_WRITE(bus, 0x923F, 0xC5);
	SENSOR_I2C_WRITE(bus, 0x9240, 0xB7);
	SENSOR_I2C_WRITE(bus, 0x9241, 0xC7);
	SENSOR_I2C_WRITE(bus, 0x9242, 0xB7);
	SENSOR_I2C_WRITE(bus, 0x9243, 0xC9);
	SENSOR_I2C_WRITE(bus, 0x9244, 0x98);
	SENSOR_I2C_WRITE(bus, 0x9245, 0x56);
	SENSOR_I2C_WRITE(bus, 0x9246, 0x98);
	SENSOR_I2C_WRITE(bus, 0x9247, 0x55);
	SENSOR_I2C_WRITE(bus, 0x9380, 0x00);
	SENSOR_I2C_WRITE(bus, 0x9381, 0x62);
	SENSOR_I2C_WRITE(bus, 0x9382, 0x00);
	SENSOR_I2C_WRITE(bus, 0x9383, 0x56);
	SENSOR_I2C_WRITE(bus, 0x9384, 0x00);
	SENSOR_I2C_WRITE(bus, 0x9385, 0x52);
	SENSOR_I2C_WRITE(bus, 0x9388, 0x00);
	SENSOR_I2C_WRITE(bus, 0x9389, 0x55);
	SENSOR_I2C_WRITE(bus, 0x938A, 0x00);
	SENSOR_I2C_WRITE(bus, 0x938B, 0x55);
	SENSOR_I2C_WRITE(bus, 0x938C, 0x00);
	SENSOR_I2C_WRITE(bus, 0x938D, 0x41);
	SENSOR_I2C_WRITE(bus, 0x5078, 0x01);

	return RET_SUCCESS;
}

//2024x1520(VH2x2binning) RAW10 13fps 2lanes 300Mbps/lane
static int res_0_config_imx577(struct sensor_module_t *module)
{
	unsigned char bus = module->hw->sensor_i2c_index;

	ISP_PR_INFO("device id[%d]\n", module->id);

	config_common1_imx577(module);

	//MIPI setting
	SENSOR_I2C_WRITE(bus, 0x0112, 0x0A);    //CSI_DT_FMT_H
	SENSOR_I2C_WRITE(bus, 0x0113, 0x0A);    //CSI_DT_FMT_L
	SENSOR_I2C_WRITE(bus, 0x0114, 0x01);    // CSI_LANE_MODE

	//Frame Horizontal Clock Count
	SENSOR_I2C_WRITE(bus, 0x0342, 0x75);    // LINE_LENGTH_PCK[15:8]
	SENSOR_I2C_WRITE(bus, 0x0343, 0x30);    // LINE_LENGTH_PCK[7:0]

	//Frame Vertical Clock Count
	SENSOR_I2C_WRITE(bus, 0x0340, 0x07);    //FRM_LENGTH_LINES[15:8]
	SENSOR_I2C_WRITE(bus, 0x0341, 0x40);    //FRM_LENGTH_LINES[7:0]
	SENSOR_I2C_WRITE(bus, 0x3210, 0x00);    //FLL_LSHIFT

	//Visible Size
	SENSOR_I2C_WRITE(bus, 0x0344, 0x00);    //X_ADD_STA[12:8]
	SENSOR_I2C_WRITE(bus, 0x0345, 0x00);    //X_ADD_STA[7:0]
	SENSOR_I2C_WRITE(bus, 0x0346, 0x00);    //Y_ADD_STA[12:8]
	SENSOR_I2C_WRITE(bus, 0x0347, 0x00);    //Y_ADD_STA[7:0]
	SENSOR_I2C_WRITE(bus, 0x0348, 0x0F);    //X_ADD_END[12:8]
	SENSOR_I2C_WRITE(bus, 0x0349, 0xD7);    //X_ADD_END[7:0]
	SENSOR_I2C_WRITE(bus, 0x034A, 0x0B);    //Y_ADD_END[12:8]
	SENSOR_I2C_WRITE(bus, 0x034B, 0xDF);    //Y_ADD_END[7:0]

	//Mode Setting
	SENSOR_I2C_WRITE(bus, 0x00E3, 0x00);    //DOL_EN --> Disable DOL-HDR
	SENSOR_I2C_WRITE(bus, 0x00E4, 0x00);    //DOL_NUM
	SENSOR_I2C_WRITE(bus, 0x00E5, 0x01);    //DOL_OUTPUT_FMT
	SENSOR_I2C_WRITE(bus, 0x00FC, 0x0A);    //DOL_CSI_DT_FMT_H_2ND
	SENSOR_I2C_WRITE(bus, 0x00FD, 0x0A);    //DOL_CSI_DT_FMT_L_2ND
	SENSOR_I2C_WRITE(bus, 0x00FE, 0x0A);    //DOL_CSI_DT_FMT_H_3RD
	SENSOR_I2C_WRITE(bus, 0x00FF, 0x0A);    //DOL_CSI_DT_FMT_L_3RD
	SENSOR_I2C_WRITE(bus, 0xE013, 0x00);
	// HDR_MODE --> Disable SME-HDR_MODE
	SENSOR_I2C_WRITE(bus, 0x0220, 0x00);
	SENSOR_I2C_WRITE(bus, 0x0221, 0x11);
	SENSOR_I2C_WRITE(bus, 0x0381, 0x01);    //X_EVN_INC
	SENSOR_I2C_WRITE(bus, 0x0383, 0x01);    //X_ODD_INC
	SENSOR_I2C_WRITE(bus, 0x0385, 0x01);    //Y_EVN_INC
	SENSOR_I2C_WRITE(bus, 0x0387, 0x01);    //Y_ODD_INC
	SENSOR_I2C_WRITE(bus, 0x0900, 0x01);    //BINNING_MODE
	//[7:4] BINNING_TYPE_H, [3:0] BINNING_TYPE_V
	SENSOR_I2C_WRITE(bus, 0x0901, 0x22);
	SENSOR_I2C_WRITE(bus, 0x0902, 0x02);    //BINNING_WEIGHTING
	SENSOR_I2C_WRITE(bus, 0x3140, 0x02);    //HDR_FNCSEL
	SENSOR_I2C_WRITE(bus, 0x3241, 0x11);
	SENSOR_I2C_WRITE(bus, 0x3250, 0x03);    //SUB_WEIGHTING
	SENSOR_I2C_WRITE(bus, 0x3E10, 0x00);    //need to confirm
	SENSOR_I2C_WRITE(bus, 0x3E11, 0x00);    //need to confirm
	SENSOR_I2C_WRITE(bus, 0x3F0D, 0x00);    //ADBIT_MODE
	SENSOR_I2C_WRITE(bus, 0x3F42, 0x00);    //BINNING_TYPE_EXT_EN
	SENSOR_I2C_WRITE(bus, 0x3F43, 0x00);    //BINNING_TYPE_H_EXT

	//Digital Crop & Scaling
	SENSOR_I2C_WRITE(bus, 0x0401, 0x00);    //SCALE_MODE
	SENSOR_I2C_WRITE(bus, 0x0404, 0x00);    //SCALE_M[8]
	SENSOR_I2C_WRITE(bus, 0x0405, 0x10);    //SCALE_M[7:0]
	SENSOR_I2C_WRITE(bus, 0x0408, 0x00);    //DIG_CROP_X_OFFSET[12:8]
	SENSOR_I2C_WRITE(bus, 0x0409, 0x02);    //DIG_CROP_X_OFFSET[7:0]
	SENSOR_I2C_WRITE(bus, 0x040A, 0x00);    //DIG_CROP_Y_OFFSET[12:8]
	SENSOR_I2C_WRITE(bus, 0x040B, 0x00);    //DIG_CROP_Y_OFFSET[7:0]
	SENSOR_I2C_WRITE(bus, 0x040C, 0x07);    //DIG_CROP_IMAGE_WIDTH[12:8]
	SENSOR_I2C_WRITE(bus, 0x040D, 0xE8);    //DIG_CROP_IMAGE_WIDTH[7:0]
	SENSOR_I2C_WRITE(bus, 0x040E, 0x05);    //DIG_CROP_IMAGE_HEIGHT[12:8]
	SENSOR_I2C_WRITE(bus, 0x040F, 0xF0);    //DIG_CROP_IMAGE_HEIGHT[7:0]

	//Output Crop
	SENSOR_I2C_WRITE(bus, 0x034C, 0x07);    //X_OUT_SIZE[12:8]
	SENSOR_I2C_WRITE(bus, 0x034D, 0xE8);    //X_OUT_SIZE[7:0]
	SENSOR_I2C_WRITE(bus, 0x034E, 0x05);    //Y_OUT_SIZE[15:8]
	SENSOR_I2C_WRITE(bus, 0x034F, 0xF0);    //Y_OUT_SIZE[7:0]

	//Clock Setting
	SENSOR_I2C_WRITE(bus, 0x0301, 0x05);    //IVT_PXCK_DIV
	SENSOR_I2C_WRITE(bus, 0x0303, 0x02);    //IVT_SYCK_DIV
	SENSOR_I2C_WRITE(bus, 0x0305, 0x04);    //IVT_PREPLLCK_DIV
	SENSOR_I2C_WRITE(bus, 0x0306, 0x01);    //IVT_PLL_MPY[11:8]
	SENSOR_I2C_WRITE(bus, 0x0307, 0x2C);    //IVT_PLL_MPY[7:0]
	SENSOR_I2C_WRITE(bus, 0x0309, 0x0A);    //IOP_PXCK_DIV
	SENSOR_I2C_WRITE(bus, 0x030B, 0x04);    //IOP_SYCK_DIV
	SENSOR_I2C_WRITE(bus, 0x030D, 0x04);    //IOP_PREPLLCK_DIV
	SENSOR_I2C_WRITE(bus, 0x030E, 0x00);    //IOP_PLL_MPY[11:8]
	SENSOR_I2C_WRITE(bus, 0x030F, 0xC8);    //IOP_PLL_MPY[7:0]
	SENSOR_I2C_WRITE(bus, 0x0310, 0x01);    //PLL_MULT_DRIV
	SENSOR_I2C_WRITE(bus, 0x0820, 0x02);    //REQ_LINK_BIT_RATE_MBPS[31:24]
	SENSOR_I2C_WRITE(bus, 0x0821, 0x58);    //REQ_LINK_BIT_RATE_MBPS[23:16]
	SENSOR_I2C_WRITE(bus, 0x0822, 0x00);    //REQ_LINK_BIT_RATE_MBPS[15:8]
	SENSOR_I2C_WRITE(bus, 0x0823, 0x00);    //REQ_LINK_BIT_RATE_MBPS[7:0]

	//Output Data Select Setting
	SENSOR_I2C_WRITE(bus, 0x3E20, 0x01);    // PDAF_TYPE_SEL
	SENSOR_I2C_WRITE(bus, 0x3E37, 0x00);    // PDAF_CTRL1[7:0]

	//PowerSave Setting
	SENSOR_I2C_WRITE(bus, 0x3F50, 0x00);    //POWER_SAVE_ENABLE
	SENSOR_I2C_WRITE(bus, 0x3F56, 0x03);    //LINE_LENGTH_INCK[15:8]
	SENSOR_I2C_WRITE(bus, 0x3F57, 0x5F);    //LINE_LENGTH_INCK[7:0]
#ifdef IMX577_EMBEDDED_DATA_EN
	    //Embedded data
	    SENSOR_I2C_WRITE(bus, 0xBCF1, 0x02);
#else
	    SENSOR_I2C_WRITE(bus, 0xBCF1, 0x00);
#endif

	//Other Setting
	SENSOR_I2C_WRITE(bus, 0x3C0A, 0x73);
	SENSOR_I2C_WRITE(bus, 0x3C0B, 0x64);
	SENSOR_I2C_WRITE(bus, 0x3C0C, 0x5F);
	SENSOR_I2C_WRITE(bus, 0x3C0D, 0x00);
	SENSOR_I2C_WRITE(bus, 0x3C0E, 0x00);
	SENSOR_I2C_WRITE(bus, 0x3C0F, 0x00);
	SENSOR_I2C_WRITE(bus, 0x3C10, 0xA4);
	SENSOR_I2C_WRITE(bus, 0x3C11, 0x02);
	SENSOR_I2C_WRITE(bus, 0x3C12, 0x00);
	SENSOR_I2C_WRITE(bus, 0x3C13, 0x03);
	SENSOR_I2C_WRITE(bus, 0x3C14, 0x80);
	SENSOR_I2C_WRITE(bus, 0x3C15, 0x04);
	SENSOR_I2C_WRITE(bus, 0x3C16, 0x15);
	SENSOR_I2C_WRITE(bus, 0x3C17, 0x15);
	SENSOR_I2C_WRITE(bus, 0x3C18, 0x15);
	SENSOR_I2C_WRITE(bus, 0x3C19, 0x15);
	SENSOR_I2C_WRITE(bus, 0x3C1A, 0x15);
	SENSOR_I2C_WRITE(bus, 0x3C1B, 0x15);
	SENSOR_I2C_WRITE(bus, 0x3C1C, 0x06);
	SENSOR_I2C_WRITE(bus, 0x3C1D, 0x06);
	SENSOR_I2C_WRITE(bus, 0x3C1E, 0x06);
	SENSOR_I2C_WRITE(bus, 0x3C1F, 0x06);
	SENSOR_I2C_WRITE(bus, 0x3C20, 0x06);
	SENSOR_I2C_WRITE(bus, 0x3C21, 0x06);
	SENSOR_I2C_WRITE(bus, 0x3C22, 0x3F);
	SENSOR_I2C_WRITE(bus, 0x3C23, 0x0A);
	SENSOR_I2C_WRITE(bus, 0x3E35, 0x01);    //SPC control
	SENSOR_I2C_WRITE(bus, 0x3F4A, 0x01);
	SENSOR_I2C_WRITE(bus, 0x3F4B, 0x7F);
	SENSOR_I2C_WRITE(bus, 0x3F26, 0x00);

	//Integration Time Setting
	SENSOR_I2C_WRITE(bus, 0x0202, 0x0F);    // COARSE_INTEG_TIME[15:8]
	SENSOR_I2C_WRITE(bus, 0x0203, 0x2A);    // COARSE_INTEG_TIME[7:0]

	//Gain Setting
	SENSOR_I2C_WRITE(bus, 0x0204, 0x00);    //ANA_GAIN_GLOBAL[9:8]
	SENSOR_I2C_WRITE(bus, 0x0205, 0x00);    //ANA_GAIN_GLOBAL[7:0]
	SENSOR_I2C_WRITE(bus, 0x020E, 0x01);    //DIG_GAIN_GR[15:8]
	SENSOR_I2C_WRITE(bus, 0x020F, 0x00);    //DIG_GAIN_GR[7:0]
	SENSOR_I2C_WRITE(bus, 0x0210, 0x01);    //DIG_GAIN_R[15:8]
	SENSOR_I2C_WRITE(bus, 0x0211, 0x00);    //DIG_GAIN_R[7:0]
	SENSOR_I2C_WRITE(bus, 0x0212, 0x01);    //DIG_GAIN_B[15:8]
	SENSOR_I2C_WRITE(bus, 0x0213, 0x00);    //DIG_GAIN_B[7:0]
	SENSOR_I2C_WRITE(bus, 0x0214, 0x01);    //DIG_GAIN_GB[15:8]
	SENSOR_I2C_WRITE(bus, 0x0215, 0x00);    //DIG_GAIN_GB[7:0]
	//Streaming Setting
	SENSOR_I2C_WRITE(bus, 0x0100, 0x01);
	ISP_PR_INFO("success\n");

	return RET_SUCCESS;
}

//2024x1520(VH2x2binning) normal RAW10 15fps 4lanes 312Mbps / lane
static int res_1_config_imx577(struct sensor_module_t *module)
{
	unsigned char bus = module->hw->sensor_i2c_index;

	ISP_PR_INFO("device id[%d]\n", module->id);
	config_common1_imx577(module);
	//MIPI setting
	SENSOR_I2C_WRITE(bus, 0x0112, 0x0A);
	SENSOR_I2C_WRITE(bus, 0x0113, 0x0A);
	SENSOR_I2C_WRITE(bus, 0x0114, 0x01);

    //Frame Horizontal Clock Count
	SENSOR_I2C_WRITE(bus, 0x0342, 0x75);
	SENSOR_I2C_WRITE(bus, 0x0343, 0x30);

    //Frame Vertical Clock Count
	SENSOR_I2C_WRITE(bus, 0x0340, 0x07);
	SENSOR_I2C_WRITE(bus, 0x0341, 0x40);
	SENSOR_I2C_WRITE(bus, 0x3210, 0x00);

    //Visible Size
	SENSOR_I2C_WRITE(bus, 0x0344, 0x00);
	SENSOR_I2C_WRITE(bus, 0x0345, 0x00);
	SENSOR_I2C_WRITE(bus, 0x0346, 0x00);
	SENSOR_I2C_WRITE(bus, 0x0347, 0x00);
	SENSOR_I2C_WRITE(bus, 0x0348, 0x0F);
	SENSOR_I2C_WRITE(bus, 0x0349, 0xD7);
	SENSOR_I2C_WRITE(bus, 0x034A, 0x0B);
	SENSOR_I2C_WRITE(bus, 0x034B, 0xDF);

    //Mode Setting
	SENSOR_I2C_WRITE(bus, 0x00E3, 0x00);
	SENSOR_I2C_WRITE(bus, 0x00E4, 0x00);
	SENSOR_I2C_WRITE(bus, 0x00E5, 0x01);
	SENSOR_I2C_WRITE(bus, 0x00FC, 0x0A);
	SENSOR_I2C_WRITE(bus, 0x00FD, 0x0A);
	SENSOR_I2C_WRITE(bus, 0x00FE, 0x0A);
	SENSOR_I2C_WRITE(bus, 0x00FF, 0x0A);
	SENSOR_I2C_WRITE(bus, 0xE013, 0x00);

	SENSOR_I2C_WRITE(bus, 0x0220, 0x00);
	SENSOR_I2C_WRITE(bus, 0x0221, 0x11);
	SENSOR_I2C_WRITE(bus, 0x0381, 0x01);
	SENSOR_I2C_WRITE(bus, 0x0383, 0x01);
	SENSOR_I2C_WRITE(bus, 0x0385, 0x01);
	SENSOR_I2C_WRITE(bus, 0x0387, 0x01);
	SENSOR_I2C_WRITE(bus, 0x0900, 0x01);
	SENSOR_I2C_WRITE(bus, 0x0901, 0x22);
	SENSOR_I2C_WRITE(bus, 0x0902, 0x02);
	SENSOR_I2C_WRITE(bus, 0x3140, 0x02);
	SENSOR_I2C_WRITE(bus, 0x3241, 0x11);
	SENSOR_I2C_WRITE(bus, 0x3250, 0x03);
	SENSOR_I2C_WRITE(bus, 0x3E10, 0x00);
	SENSOR_I2C_WRITE(bus, 0x3E11, 0x00);
	SENSOR_I2C_WRITE(bus, 0x3F0D, 0x00);
	SENSOR_I2C_WRITE(bus, 0x3F42, 0x00);
	SENSOR_I2C_WRITE(bus, 0x3F43, 0x00);

    //Digital Crop & Scaling
	SENSOR_I2C_WRITE(bus, 0x0401, 0x00);
	SENSOR_I2C_WRITE(bus, 0x0404, 0x00);
	SENSOR_I2C_WRITE(bus, 0x0405, 0x10);
	SENSOR_I2C_WRITE(bus, 0x0408, 0x00);
	SENSOR_I2C_WRITE(bus, 0x0409, 0x02);
	SENSOR_I2C_WRITE(bus, 0x040A, 0x00);
	SENSOR_I2C_WRITE(bus, 0x040B, 0x00);
	SENSOR_I2C_WRITE(bus, 0x040C, 0x07);
	SENSOR_I2C_WRITE(bus, 0x040D, 0xE8);
	SENSOR_I2C_WRITE(bus, 0x040E, 0x05);
	SENSOR_I2C_WRITE(bus, 0x040F, 0xF0);

    //Output Crop
	SENSOR_I2C_WRITE(bus, 0x034C, 0x07);
	SENSOR_I2C_WRITE(bus, 0x034D, 0xE8);
	SENSOR_I2C_WRITE(bus, 0x034E, 0x05);
	SENSOR_I2C_WRITE(bus, 0x034F, 0xF0);

    //Clock Setting
	SENSOR_I2C_WRITE(bus, 0x0301, 0x05);
	SENSOR_I2C_WRITE(bus, 0x0303, 0x02);
	SENSOR_I2C_WRITE(bus, 0x0305, 0x04);
	SENSOR_I2C_WRITE(bus, 0x0306, 0x01);
	SENSOR_I2C_WRITE(bus, 0x0307, 0x5C);
	SENSOR_I2C_WRITE(bus, 0x0309, 0x0A);
	SENSOR_I2C_WRITE(bus, 0x030B, 0x04);
	SENSOR_I2C_WRITE(bus, 0x030D, 0x04);
	SENSOR_I2C_WRITE(bus, 0x030E, 0x00);
	SENSOR_I2C_WRITE(bus, 0x030F, 0xD0);
	SENSOR_I2C_WRITE(bus, 0x0310, 0x01);
	SENSOR_I2C_WRITE(bus, 0x0820, 0x02);
	SENSOR_I2C_WRITE(bus, 0x0821, 0x70);
	SENSOR_I2C_WRITE(bus, 0x0822, 0x00);
	SENSOR_I2C_WRITE(bus, 0x0823, 0x00);

    //Output Data Select Setting
	SENSOR_I2C_WRITE(bus, 0x3E20, 0x01);
	SENSOR_I2C_WRITE(bus, 0x3E37, 0x00);

    //PowerSave Setting
	SENSOR_I2C_WRITE(bus, 0x3F50, 0x00);
	SENSOR_I2C_WRITE(bus, 0x3F56, 0x03);
	SENSOR_I2C_WRITE(bus, 0x3F57, 0x5F);

#ifdef IMX577_EMBEDDED_DATA_EN
    //Embedded data
	SENSOR_I2C_WRITE(bus, 0xBCF1, 0x02);
#else
	SENSOR_I2C_WRITE(bus, 0xBCF1, 0x00);
#endif

    //Other Setting
	SENSOR_I2C_WRITE(bus, 0x3C0A, 0x73);
	SENSOR_I2C_WRITE(bus, 0x3C0B, 0x64);
	SENSOR_I2C_WRITE(bus, 0x3C0C, 0x5F);
	SENSOR_I2C_WRITE(bus, 0x3C0D, 0x00);
	SENSOR_I2C_WRITE(bus, 0x3C0E, 0x00);
	SENSOR_I2C_WRITE(bus, 0x3C0F, 0x00);
	SENSOR_I2C_WRITE(bus, 0x3C10, 0xA4);
	SENSOR_I2C_WRITE(bus, 0x3C11, 0x02);
	SENSOR_I2C_WRITE(bus, 0x3C12, 0x00);
	SENSOR_I2C_WRITE(bus, 0x3C13, 0x03);
	SENSOR_I2C_WRITE(bus, 0x3C14, 0x80);
	SENSOR_I2C_WRITE(bus, 0x3C15, 0x04);
	SENSOR_I2C_WRITE(bus, 0x3C16, 0x15);
	SENSOR_I2C_WRITE(bus, 0x3C17, 0x15);
	SENSOR_I2C_WRITE(bus, 0x3C18, 0x15);
	SENSOR_I2C_WRITE(bus, 0x3C19, 0x15);
	SENSOR_I2C_WRITE(bus, 0x3C1A, 0x15);
	SENSOR_I2C_WRITE(bus, 0x3C1B, 0x15);
	SENSOR_I2C_WRITE(bus, 0x3C1C, 0x06);
	SENSOR_I2C_WRITE(bus, 0x3C1D, 0x06);
	SENSOR_I2C_WRITE(bus, 0x3C1E, 0x06);
	SENSOR_I2C_WRITE(bus, 0x3C1F, 0x06);
	SENSOR_I2C_WRITE(bus, 0x3C20, 0x06);
	SENSOR_I2C_WRITE(bus, 0x3C21, 0x06);
	SENSOR_I2C_WRITE(bus, 0x3C22, 0x3F);
	SENSOR_I2C_WRITE(bus, 0x3C23, 0x0A);
	SENSOR_I2C_WRITE(bus, 0x3E35, 0x01);
	SENSOR_I2C_WRITE(bus, 0x3F4A, 0x01);
	SENSOR_I2C_WRITE(bus, 0x3F4B, 0x7F);
	SENSOR_I2C_WRITE(bus, 0x3F26, 0x00);

    //Integration Time Setting
	SENSOR_I2C_WRITE(bus, 0x0202, 0x0F);
	SENSOR_I2C_WRITE(bus, 0x0203, 0x2A);

    //Gain Setting
	SENSOR_I2C_WRITE(bus, 0x0204, 0x00);
	SENSOR_I2C_WRITE(bus, 0x0205, 0x00);
	SENSOR_I2C_WRITE(bus, 0x020E, 0x01);
	SENSOR_I2C_WRITE(bus, 0x020F, 0x00);
	SENSOR_I2C_WRITE(bus, 0x0210, 0x01);
	SENSOR_I2C_WRITE(bus, 0x0211, 0x00);
	SENSOR_I2C_WRITE(bus, 0x0212, 0x01);
	SENSOR_I2C_WRITE(bus, 0x0213, 0x00);
	SENSOR_I2C_WRITE(bus, 0x0214, 0x01);
	SENSOR_I2C_WRITE(bus, 0x0215, 0x00);
	//Streaming
	SENSOR_I2C_WRITE(bus, 0x0100, 0x01);
	ISP_PR_INFO("success, 2024*1520 15fps\n");
	return RET_SUCCESS;
}

//2024x1520(VH2x2binning) normal RAW10 30fps 4lanes 600Mbps / lane
static int res_2_config_imx577(struct sensor_module_t *module)
{
	unsigned char bus = module->hw->sensor_i2c_index;

	ISP_PR_INFO("device id[%d]\n", module->id);
	config_common1_imx577(module);
	//MIPI setting
	SENSOR_I2C_WRITE(bus, 0x0112, 0x0A);
	SENSOR_I2C_WRITE(bus, 0x0113, 0x0A);
	SENSOR_I2C_WRITE(bus, 0x0114, 0x03);

    //Frame Horizontal Clock Count
	SENSOR_I2C_WRITE(bus, 0x0342, 0x1F);
	SENSOR_I2C_WRITE(bus, 0x0343, 0x40);

    //Frame Vertical Clock Count
	SENSOR_I2C_WRITE(bus, 0x0340, 0x0D);
	SENSOR_I2C_WRITE(bus, 0x0341, 0x70);
	SENSOR_I2C_WRITE(bus, 0x3210, 0x00);

    //Visible Size
	SENSOR_I2C_WRITE(bus, 0x0344, 0x00);
	SENSOR_I2C_WRITE(bus, 0x0345, 0x00);
	SENSOR_I2C_WRITE(bus, 0x0346, 0x00);
	SENSOR_I2C_WRITE(bus, 0x0347, 0x00);
	SENSOR_I2C_WRITE(bus, 0x0348, 0x0F);
	SENSOR_I2C_WRITE(bus, 0x0349, 0xD7);
	SENSOR_I2C_WRITE(bus, 0x034A, 0x0B);
	SENSOR_I2C_WRITE(bus, 0x034B, 0xDF);

    //Mode Setting
	SENSOR_I2C_WRITE(bus, 0x00E3, 0x00);
	SENSOR_I2C_WRITE(bus, 0x00E4, 0x00);
	SENSOR_I2C_WRITE(bus, 0x00E5, 0x01);
	SENSOR_I2C_WRITE(bus, 0x00FC, 0x0A);
	SENSOR_I2C_WRITE(bus, 0x00FD, 0x0A);
	SENSOR_I2C_WRITE(bus, 0x00FE, 0x0A);
	SENSOR_I2C_WRITE(bus, 0x00FF, 0x0A);
	SENSOR_I2C_WRITE(bus, 0xE013, 0x00);

	SENSOR_I2C_WRITE(bus, 0x0220, 0x00);
	SENSOR_I2C_WRITE(bus, 0x0221, 0x11);
	SENSOR_I2C_WRITE(bus, 0x0381, 0x01);
	SENSOR_I2C_WRITE(bus, 0x0383, 0x01);
	SENSOR_I2C_WRITE(bus, 0x0385, 0x01);
	SENSOR_I2C_WRITE(bus, 0x0387, 0x01);
	SENSOR_I2C_WRITE(bus, 0x0900, 0x01);
	SENSOR_I2C_WRITE(bus, 0x0901, 0x22);
	SENSOR_I2C_WRITE(bus, 0x0902, 0x02);
	SENSOR_I2C_WRITE(bus, 0x3140, 0x02);
	SENSOR_I2C_WRITE(bus, 0x3241, 0x11);
	SENSOR_I2C_WRITE(bus, 0x3250, 0x03);
	SENSOR_I2C_WRITE(bus, 0x3E10, 0x00);
	SENSOR_I2C_WRITE(bus, 0x3E11, 0x00);
	SENSOR_I2C_WRITE(bus, 0x3F0D, 0x00);
	SENSOR_I2C_WRITE(bus, 0x3F42, 0x00);
	SENSOR_I2C_WRITE(bus, 0x3F43, 0x00);

    //Digital Crop & Scaling
	SENSOR_I2C_WRITE(bus, 0x0401, 0x00);
	SENSOR_I2C_WRITE(bus, 0x0404, 0x00);
	SENSOR_I2C_WRITE(bus, 0x0405, 0x10);
	SENSOR_I2C_WRITE(bus, 0x0408, 0x00);
	SENSOR_I2C_WRITE(bus, 0x0409, 0x02);
	SENSOR_I2C_WRITE(bus, 0x040A, 0x00);
	SENSOR_I2C_WRITE(bus, 0x040B, 0x00);
	SENSOR_I2C_WRITE(bus, 0x040C, 0x07);
	SENSOR_I2C_WRITE(bus, 0x040D, 0xE8);
	SENSOR_I2C_WRITE(bus, 0x040E, 0x05);
	SENSOR_I2C_WRITE(bus, 0x040F, 0xF0);

    //Output Crop
	SENSOR_I2C_WRITE(bus, 0x034C, 0x07);
	SENSOR_I2C_WRITE(bus, 0x034D, 0xE8);
	SENSOR_I2C_WRITE(bus, 0x034E, 0x05);
	SENSOR_I2C_WRITE(bus, 0x034F, 0xF0);

    //Clock Setting
	SENSOR_I2C_WRITE(bus, 0x0301, 0x05);
	SENSOR_I2C_WRITE(bus, 0x0303, 0x02);
	SENSOR_I2C_WRITE(bus, 0x0305, 0x04);
	SENSOR_I2C_WRITE(bus, 0x0306, 0x01);
	SENSOR_I2C_WRITE(bus, 0x0307, 0x58);
	SENSOR_I2C_WRITE(bus, 0x0309, 0x0A);
	SENSOR_I2C_WRITE(bus, 0x030B, 0x02);
	SENSOR_I2C_WRITE(bus, 0x030D, 0x03);
	SENSOR_I2C_WRITE(bus, 0x030E, 0x00);
	SENSOR_I2C_WRITE(bus, 0x030F, 0x96);
	SENSOR_I2C_WRITE(bus, 0x0310, 0x01);
	SENSOR_I2C_WRITE(bus, 0x0820, 0x09);
	SENSOR_I2C_WRITE(bus, 0x0821, 0x60);
	SENSOR_I2C_WRITE(bus, 0x0822, 0x00);
	SENSOR_I2C_WRITE(bus, 0x0823, 0x00);

    //Output Data Select Setting
	SENSOR_I2C_WRITE(bus, 0x3E20, 0x01);
	SENSOR_I2C_WRITE(bus, 0x3E37, 0x00);

    //PowerSave Setting
	SENSOR_I2C_WRITE(bus, 0x3F50, 0x00);
	SENSOR_I2C_WRITE(bus, 0x3F56, 0x00);
	SENSOR_I2C_WRITE(bus, 0x3F57, 0xE9);

#ifdef IMX577_EMBEDDED_DATA_EN
    //Embedded data
	SENSOR_I2C_WRITE(bus, 0xBCF1, 0x02);
#else
	SENSOR_I2C_WRITE(bus, 0xBCF1, 0x00);
#endif

    //Other Setting
	SENSOR_I2C_WRITE(bus, 0x3C0A, 0x73);
	SENSOR_I2C_WRITE(bus, 0x3C0B, 0x64);
	SENSOR_I2C_WRITE(bus, 0x3C0C, 0x5F);
	SENSOR_I2C_WRITE(bus, 0x3C0D, 0x00);
	SENSOR_I2C_WRITE(bus, 0x3C0E, 0x00);
	SENSOR_I2C_WRITE(bus, 0x3C0F, 0x00);
	SENSOR_I2C_WRITE(bus, 0x3C10, 0xA4);
	SENSOR_I2C_WRITE(bus, 0x3C11, 0x02);
	SENSOR_I2C_WRITE(bus, 0x3C12, 0x00);
	SENSOR_I2C_WRITE(bus, 0x3C13, 0x03);
	SENSOR_I2C_WRITE(bus, 0x3C14, 0x80);
	SENSOR_I2C_WRITE(bus, 0x3C15, 0x04);
	SENSOR_I2C_WRITE(bus, 0x3C16, 0x15);
	SENSOR_I2C_WRITE(bus, 0x3C17, 0x15);
	SENSOR_I2C_WRITE(bus, 0x3C18, 0x15);
	SENSOR_I2C_WRITE(bus, 0x3C19, 0x15);
	SENSOR_I2C_WRITE(bus, 0x3C1A, 0x15);
	SENSOR_I2C_WRITE(bus, 0x3C1B, 0x15);
	SENSOR_I2C_WRITE(bus, 0x3C1C, 0x06);
	SENSOR_I2C_WRITE(bus, 0x3C1D, 0x06);
	SENSOR_I2C_WRITE(bus, 0x3C1E, 0x06);
	SENSOR_I2C_WRITE(bus, 0x3C1F, 0x06);
	SENSOR_I2C_WRITE(bus, 0x3C20, 0x06);
	SENSOR_I2C_WRITE(bus, 0x3C21, 0x06);
	SENSOR_I2C_WRITE(bus, 0x3C22, 0x3F);
	SENSOR_I2C_WRITE(bus, 0x3C23, 0x0A);
	SENSOR_I2C_WRITE(bus, 0x3E35, 0x01);
	SENSOR_I2C_WRITE(bus, 0x3F4A, 0x01);
	SENSOR_I2C_WRITE(bus, 0x3F4B, 0x7F);
	SENSOR_I2C_WRITE(bus, 0x3F26, 0x00);

    //Integration Time Setting
	SENSOR_I2C_WRITE(bus, 0x0202, 0x06);
	SENSOR_I2C_WRITE(bus, 0x0203, 0xA2);

    //Gain Setting
	SENSOR_I2C_WRITE(bus, 0x0204, 0x00);
	SENSOR_I2C_WRITE(bus, 0x0205, 0x00);
	SENSOR_I2C_WRITE(bus, 0x020E, 0x01);
	SENSOR_I2C_WRITE(bus, 0x020F, 0x00);
	SENSOR_I2C_WRITE(bus, 0x0210, 0x01);
	SENSOR_I2C_WRITE(bus, 0x0211, 0x00);
	SENSOR_I2C_WRITE(bus, 0x0212, 0x01);
	SENSOR_I2C_WRITE(bus, 0x0213, 0x00);
	SENSOR_I2C_WRITE(bus, 0x0214, 0x01);
	SENSOR_I2C_WRITE(bus, 0x0215, 0x00);
	//Streaming
	SENSOR_I2C_WRITE(bus, 0x0100, 0x01);
	ISP_PR_INFO("success\n");
	return RET_SUCCESS;
}

//2024x1520(VH2x2binning) normal RAW10 60fps 4lanes 600Mbps / lane
static int res_3_config_imx577(struct sensor_module_t *module)
{
	unsigned char bus = module->hw->sensor_i2c_index;

	ISP_PR_INFO("device id[%d]\n", module->id);
	config_common1_imx577(module);
	//MIPI setting
	SENSOR_I2C_WRITE(bus, 0x0112, 0x0A);
	SENSOR_I2C_WRITE(bus, 0x0113, 0x0A);
	SENSOR_I2C_WRITE(bus, 0x0114, 0x03);

    //Frame Horizontal Clock Count
	SENSOR_I2C_WRITE(bus, 0x0342, 0x1C);
	SENSOR_I2C_WRITE(bus, 0x0343, 0xE8);

    //Frame Vertical Clock Count
	SENSOR_I2C_WRITE(bus, 0x0340, 0x07);
	SENSOR_I2C_WRITE(bus, 0x0341, 0x44);
	SENSOR_I2C_WRITE(bus, 0x3210, 0x00);

    //Visible Size
	SENSOR_I2C_WRITE(bus, 0x0344, 0x00);
	SENSOR_I2C_WRITE(bus, 0x0345, 0x00);
	SENSOR_I2C_WRITE(bus, 0x0346, 0x00);
	SENSOR_I2C_WRITE(bus, 0x0347, 0x00);
	SENSOR_I2C_WRITE(bus, 0x0348, 0x0F);
	SENSOR_I2C_WRITE(bus, 0x0349, 0xD7);
	SENSOR_I2C_WRITE(bus, 0x034A, 0x0B);
	SENSOR_I2C_WRITE(bus, 0x034B, 0xDF);

    //Mode Setting
	SENSOR_I2C_WRITE(bus, 0x00E3, 0x00);
	SENSOR_I2C_WRITE(bus, 0x00E4, 0x00);
	SENSOR_I2C_WRITE(bus, 0x00E5, 0x01);
	SENSOR_I2C_WRITE(bus, 0x00FC, 0x0A);
	SENSOR_I2C_WRITE(bus, 0x00FD, 0x0A);
	SENSOR_I2C_WRITE(bus, 0x00FE, 0x0A);
	SENSOR_I2C_WRITE(bus, 0x00FF, 0x0A);
	SENSOR_I2C_WRITE(bus, 0xE013, 0x00);

	SENSOR_I2C_WRITE(bus, 0x0220, 0x00);
	SENSOR_I2C_WRITE(bus, 0x0221, 0x11);
	SENSOR_I2C_WRITE(bus, 0x0381, 0x01);
	SENSOR_I2C_WRITE(bus, 0x0383, 0x01);
	SENSOR_I2C_WRITE(bus, 0x0385, 0x01);
	SENSOR_I2C_WRITE(bus, 0x0387, 0x01);
	SENSOR_I2C_WRITE(bus, 0x0900, 0x01);
	SENSOR_I2C_WRITE(bus, 0x0901, 0x22);
	SENSOR_I2C_WRITE(bus, 0x0902, 0x02);
	SENSOR_I2C_WRITE(bus, 0x3140, 0x02);
	SENSOR_I2C_WRITE(bus, 0x3241, 0x11);
	SENSOR_I2C_WRITE(bus, 0x3250, 0x03);
	SENSOR_I2C_WRITE(bus, 0x3E10, 0x00);
	SENSOR_I2C_WRITE(bus, 0x3E11, 0x00);
	SENSOR_I2C_WRITE(bus, 0x3F0D, 0x00);
	SENSOR_I2C_WRITE(bus, 0x3F42, 0x00);
	SENSOR_I2C_WRITE(bus, 0x3F43, 0x00);

    //Digital Crop & Scaling
	SENSOR_I2C_WRITE(bus, 0x0401, 0x00);
	SENSOR_I2C_WRITE(bus, 0x0404, 0x00);
	SENSOR_I2C_WRITE(bus, 0x0405, 0x10);
	SENSOR_I2C_WRITE(bus, 0x0408, 0x00);
	SENSOR_I2C_WRITE(bus, 0x0409, 0x02);
	SENSOR_I2C_WRITE(bus, 0x040A, 0x00);
	SENSOR_I2C_WRITE(bus, 0x040B, 0x00);
	SENSOR_I2C_WRITE(bus, 0x040C, 0x07);
	SENSOR_I2C_WRITE(bus, 0x040D, 0xE8);
	SENSOR_I2C_WRITE(bus, 0x040E, 0x05);
	SENSOR_I2C_WRITE(bus, 0x040F, 0xF0);

    //Output Crop
	SENSOR_I2C_WRITE(bus, 0x034C, 0x07);
	SENSOR_I2C_WRITE(bus, 0x034D, 0xE8);
	SENSOR_I2C_WRITE(bus, 0x034E, 0x05);
	SENSOR_I2C_WRITE(bus, 0x034F, 0xF0);

    //Clock Setting
	SENSOR_I2C_WRITE(bus, 0x0301, 0x05);
	SENSOR_I2C_WRITE(bus, 0x0303, 0x02);
	SENSOR_I2C_WRITE(bus, 0x0305, 0x04);
	SENSOR_I2C_WRITE(bus, 0x0306, 0x01);
	SENSOR_I2C_WRITE(bus, 0x0307, 0x58);
	SENSOR_I2C_WRITE(bus, 0x0309, 0x0A);
	SENSOR_I2C_WRITE(bus, 0x030B, 0x02);
	SENSOR_I2C_WRITE(bus, 0x030D, 0x03);
	SENSOR_I2C_WRITE(bus, 0x030E, 0x00);
	SENSOR_I2C_WRITE(bus, 0x030F, 0x96);
	SENSOR_I2C_WRITE(bus, 0x0310, 0x01);
	SENSOR_I2C_WRITE(bus, 0x0820, 0x09);
	SENSOR_I2C_WRITE(bus, 0x0821, 0x60);
	SENSOR_I2C_WRITE(bus, 0x0822, 0x00);
	SENSOR_I2C_WRITE(bus, 0x0823, 0x00);

    //Output Data Select Setting
	SENSOR_I2C_WRITE(bus, 0x3E20, 0x01);
	SENSOR_I2C_WRITE(bus, 0x3E37, 0x00);

    //PowerSave Setting
	SENSOR_I2C_WRITE(bus, 0x3F50, 0x00);
	SENSOR_I2C_WRITE(bus, 0x3F56, 0x00);
	SENSOR_I2C_WRITE(bus, 0x3F57, 0xE9);

#ifdef IMX577_EMBEDDED_DATA_EN
    //Embedded data
	SENSOR_I2C_WRITE(bus, 0xBCF1, 0x02);
#else
	SENSOR_I2C_WRITE(bus, 0xBCF1, 0x00);
#endif

    //Other Setting
	SENSOR_I2C_WRITE(bus, 0x3C0A, 0x73);
	SENSOR_I2C_WRITE(bus, 0x3C0B, 0x64);
	SENSOR_I2C_WRITE(bus, 0x3C0C, 0x5F);
	SENSOR_I2C_WRITE(bus, 0x3C0D, 0x00);
	SENSOR_I2C_WRITE(bus, 0x3C0E, 0x00);
	SENSOR_I2C_WRITE(bus, 0x3C0F, 0x00);
	SENSOR_I2C_WRITE(bus, 0x3C10, 0xA4);
	SENSOR_I2C_WRITE(bus, 0x3C11, 0x02);
	SENSOR_I2C_WRITE(bus, 0x3C12, 0x00);
	SENSOR_I2C_WRITE(bus, 0x3C13, 0x03);
	SENSOR_I2C_WRITE(bus, 0x3C14, 0x80);
	SENSOR_I2C_WRITE(bus, 0x3C15, 0x04);
	SENSOR_I2C_WRITE(bus, 0x3C16, 0x15);
	SENSOR_I2C_WRITE(bus, 0x3C17, 0x15);
	SENSOR_I2C_WRITE(bus, 0x3C18, 0x15);
	SENSOR_I2C_WRITE(bus, 0x3C19, 0x15);
	SENSOR_I2C_WRITE(bus, 0x3C1A, 0x15);
	SENSOR_I2C_WRITE(bus, 0x3C1B, 0x15);
	SENSOR_I2C_WRITE(bus, 0x3C1C, 0x06);
	SENSOR_I2C_WRITE(bus, 0x3C1D, 0x06);
	SENSOR_I2C_WRITE(bus, 0x3C1E, 0x06);
	SENSOR_I2C_WRITE(bus, 0x3C1F, 0x06);
	SENSOR_I2C_WRITE(bus, 0x3C20, 0x06);
	SENSOR_I2C_WRITE(bus, 0x3C21, 0x06);
	SENSOR_I2C_WRITE(bus, 0x3C22, 0x3F);
	SENSOR_I2C_WRITE(bus, 0x3C23, 0x0A);
	SENSOR_I2C_WRITE(bus, 0x3E35, 0x01);
	SENSOR_I2C_WRITE(bus, 0x3F4A, 0x01);
	SENSOR_I2C_WRITE(bus, 0x3F4B, 0x7F);
	SENSOR_I2C_WRITE(bus, 0x3F26, 0x00);

    //Integration Time Setting
	SENSOR_I2C_WRITE(bus, 0x0202, 0x06);
	SENSOR_I2C_WRITE(bus, 0x0203, 0xA2);

    //Gain Setting
	SENSOR_I2C_WRITE(bus, 0x0204, 0x00);
	SENSOR_I2C_WRITE(bus, 0x0205, 0x00);
	SENSOR_I2C_WRITE(bus, 0x020E, 0x01);
	SENSOR_I2C_WRITE(bus, 0x020F, 0x00);
	SENSOR_I2C_WRITE(bus, 0x0210, 0x01);
	SENSOR_I2C_WRITE(bus, 0x0211, 0x00);
	SENSOR_I2C_WRITE(bus, 0x0212, 0x01);
	SENSOR_I2C_WRITE(bus, 0x0213, 0x00);
	SENSOR_I2C_WRITE(bus, 0x0214, 0x01);
	SENSOR_I2C_WRITE(bus, 0x0215, 0x00);
	SENSOR_I2C_WRITE(bus, 0x0100, 0x01);
	ISP_PR_INFO("success\n");
	return RET_SUCCESS;
}

//4048x1520(V2binning) RAW10 30fps 4lanes 1050Mbps/lane
static int res_4_config_imx577(struct sensor_module_t *module)
{
	unsigned char bus = module->hw->sensor_i2c_index;

	ISP_PR_INFO("device id[%d]\n", module->id);
	config_common1_imx577(module);

	//MIPI setting
	SENSOR_I2C_WRITE(bus, 0x0112, 0x0A);
	SENSOR_I2C_WRITE(bus, 0x0113, 0x0A);
	SENSOR_I2C_WRITE(bus, 0x0114, 0x03);

    //Frame Horizontal Clock Count
	SENSOR_I2C_WRITE(bus, 0x0342, 0x21);
	SENSOR_I2C_WRITE(bus, 0x0343, 0xFC);

    //Frame Vertical Clock Count
	SENSOR_I2C_WRITE(bus, 0x0340, 0x0C);
	SENSOR_I2C_WRITE(bus, 0x0341, 0x80);
	SENSOR_I2C_WRITE(bus, 0x3210, 0x00);

    //Visible Size
	SENSOR_I2C_WRITE(bus, 0x0344, 0x00);
	SENSOR_I2C_WRITE(bus, 0x0345, 0x00);
	SENSOR_I2C_WRITE(bus, 0x0346, 0x00);
	SENSOR_I2C_WRITE(bus, 0x0347, 0x00);
	SENSOR_I2C_WRITE(bus, 0x0348, 0x0F);
	SENSOR_I2C_WRITE(bus, 0x0349, 0xD7);
	SENSOR_I2C_WRITE(bus, 0x034A, 0x0B);
	SENSOR_I2C_WRITE(bus, 0x034B, 0xDF);

    //Mode Setting
	SENSOR_I2C_WRITE(bus, 0x00E3, 0x00);
	SENSOR_I2C_WRITE(bus, 0x00E4, 0x00);
	SENSOR_I2C_WRITE(bus, 0x00E5, 0x01);
	SENSOR_I2C_WRITE(bus, 0x00FC, 0x0A);
	SENSOR_I2C_WRITE(bus, 0x00FD, 0x0A);
	SENSOR_I2C_WRITE(bus, 0x00FE, 0x0A);
	SENSOR_I2C_WRITE(bus, 0x00FF, 0x0A);
	SENSOR_I2C_WRITE(bus, 0xE013, 0x00);

	SENSOR_I2C_WRITE(bus, 0x0220, 0x00);
	SENSOR_I2C_WRITE(bus, 0x0221, 0x11);
	SENSOR_I2C_WRITE(bus, 0x0381, 0x01);
	SENSOR_I2C_WRITE(bus, 0x0383, 0x01);
	SENSOR_I2C_WRITE(bus, 0x0385, 0x01);
	SENSOR_I2C_WRITE(bus, 0x0387, 0x01);
	SENSOR_I2C_WRITE(bus, 0x0900, 0x01);
	SENSOR_I2C_WRITE(bus, 0x0901, 0x12);
	SENSOR_I2C_WRITE(bus, 0x0902, 0x00);
	SENSOR_I2C_WRITE(bus, 0x3140, 0x02);
	SENSOR_I2C_WRITE(bus, 0x3241, 0x11);
    //SENSOR_I2C_WRITE(bus, 0x3242, 0x01
	SENSOR_I2C_WRITE(bus, 0x3250, 0x00);
	SENSOR_I2C_WRITE(bus, 0x3E10, 0x00);
	SENSOR_I2C_WRITE(bus, 0x3E11, 0x00);
	SENSOR_I2C_WRITE(bus, 0x3F0D, 0x00);
	SENSOR_I2C_WRITE(bus, 0x3F42, 0x00);
	SENSOR_I2C_WRITE(bus, 0x3F43, 0x00);

    //Digital Crop & Scaling
	SENSOR_I2C_WRITE(bus, 0x0401, 0x00);
	SENSOR_I2C_WRITE(bus, 0x0404, 0x00);
	SENSOR_I2C_WRITE(bus, 0x0405, 0x10);
	SENSOR_I2C_WRITE(bus, 0x0408, 0x00);
	SENSOR_I2C_WRITE(bus, 0x0409, 0x04);
	SENSOR_I2C_WRITE(bus, 0x040A, 0x00);
	SENSOR_I2C_WRITE(bus, 0x040B, 0x00);
	SENSOR_I2C_WRITE(bus, 0x040C, 0x0F);
	SENSOR_I2C_WRITE(bus, 0x040D, 0xD0);
	SENSOR_I2C_WRITE(bus, 0x040E, 0x05);
	SENSOR_I2C_WRITE(bus, 0x040F, 0xF0);

    //Output Crop
	SENSOR_I2C_WRITE(bus, 0x034C, 0x0F);
	SENSOR_I2C_WRITE(bus, 0x034D, 0xD0);
	SENSOR_I2C_WRITE(bus, 0x034E, 0x05);
	SENSOR_I2C_WRITE(bus, 0x034F, 0xF0);

    //Clock Setting
	SENSOR_I2C_WRITE(bus, 0x0301, 0x05);
	SENSOR_I2C_WRITE(bus, 0x0303, 0x02);
	SENSOR_I2C_WRITE(bus, 0x0305, 0x04);
	SENSOR_I2C_WRITE(bus, 0x0306, 0x01);
	SENSOR_I2C_WRITE(bus, 0x0307, 0x5C);
	SENSOR_I2C_WRITE(bus, 0x0309, 0x0A);
	SENSOR_I2C_WRITE(bus, 0x030B, 0x01);
	SENSOR_I2C_WRITE(bus, 0x030D, 0x04);
	SENSOR_I2C_WRITE(bus, 0x030E, 0x00);
	SENSOR_I2C_WRITE(bus, 0x030F, 0xAF);
	SENSOR_I2C_WRITE(bus, 0x0310, 0x01);
	SENSOR_I2C_WRITE(bus, 0x0820, 0x10);
	SENSOR_I2C_WRITE(bus, 0x0821, 0x68);
	SENSOR_I2C_WRITE(bus, 0x0822, 0x00);
	SENSOR_I2C_WRITE(bus, 0x0823, 0x00);

    //Output Data Select Setting
	SENSOR_I2C_WRITE(bus, 0x3E20, 0x01);
	SENSOR_I2C_WRITE(bus, 0x3E37, 0x00);

    //PowerSave Setting
	SENSOR_I2C_WRITE(bus, 0x3F50, 0x00);
	SENSOR_I2C_WRITE(bus, 0x3F56, 0x00);
	SENSOR_I2C_WRITE(bus, 0x3F57, 0xFB);

#ifdef IMX577_EMBEDDED_DATA_EN
    //Embedded data
	SENSOR_I2C_WRITE(bus, 0xBCF1, 0x02);
#else
	SENSOR_I2C_WRITE(bus, 0xBCF1, 0x00);
#endif

    //Other Setting
	SENSOR_I2C_WRITE(bus, 0x3C0A, 0x73);
	SENSOR_I2C_WRITE(bus, 0x3C0B, 0x64);
	SENSOR_I2C_WRITE(bus, 0x3C0C, 0x5F);
	SENSOR_I2C_WRITE(bus, 0x3C0D, 0x07);
	SENSOR_I2C_WRITE(bus, 0x3C0E, 0xFF);
	SENSOR_I2C_WRITE(bus, 0x3C0F, 0x00);
	SENSOR_I2C_WRITE(bus, 0x3C10, 0x00);
	SENSOR_I2C_WRITE(bus, 0x3C11, 0x02);
	SENSOR_I2C_WRITE(bus, 0x3C12, 0x00);
	SENSOR_I2C_WRITE(bus, 0x3C13, 0x03);
	SENSOR_I2C_WRITE(bus, 0x3C14, 0x00);
	SENSOR_I2C_WRITE(bus, 0x3C15, 0x04);
	SENSOR_I2C_WRITE(bus, 0x3C16, 0x15);
	SENSOR_I2C_WRITE(bus, 0x3C17, 0x15);
	SENSOR_I2C_WRITE(bus, 0x3C18, 0x15);
	SENSOR_I2C_WRITE(bus, 0x3C19, 0x15);
	SENSOR_I2C_WRITE(bus, 0x3C1A, 0x15);
	SENSOR_I2C_WRITE(bus, 0x3C1B, 0x15);
	SENSOR_I2C_WRITE(bus, 0x3C1C, 0x06);
	SENSOR_I2C_WRITE(bus, 0x3C1D, 0x06);
	SENSOR_I2C_WRITE(bus, 0x3C1E, 0x06);
	SENSOR_I2C_WRITE(bus, 0x3C1F, 0x06);
	SENSOR_I2C_WRITE(bus, 0x3C20, 0x06);
	SENSOR_I2C_WRITE(bus, 0x3C21, 0x06);
	SENSOR_I2C_WRITE(bus, 0x3C22, 0x3F);
	SENSOR_I2C_WRITE(bus, 0x3C23, 0x0A);
	SENSOR_I2C_WRITE(bus, 0x3E35, 0x01);
	SENSOR_I2C_WRITE(bus, 0x3F4A, 0x03);
	SENSOR_I2C_WRITE(bus, 0x3F4B, 0xBF);
	SENSOR_I2C_WRITE(bus, 0x3F26, 0x01);

    //Integration Time Setting
	SENSOR_I2C_WRITE(bus, 0x0202, 0x06);
	SENSOR_I2C_WRITE(bus, 0x0203, 0x2A);

    //Gain Setting
	SENSOR_I2C_WRITE(bus, 0x0204, 0x00);
	SENSOR_I2C_WRITE(bus, 0x0205, 0x00);
	SENSOR_I2C_WRITE(bus, 0x020E, 0x01);
	SENSOR_I2C_WRITE(bus, 0x020F, 0x00);
	SENSOR_I2C_WRITE(bus, 0x0210, 0x01);
	SENSOR_I2C_WRITE(bus, 0x0211, 0x00);
	SENSOR_I2C_WRITE(bus, 0x0212, 0x01);
	SENSOR_I2C_WRITE(bus, 0x0213, 0x00);
	SENSOR_I2C_WRITE(bus, 0x0214, 0x01);
	SENSOR_I2C_WRITE(bus, 0x0215, 0x00);
	//Streaming
	SENSOR_I2C_WRITE(bus, 0x0100, 0x01);
	ISP_PR_INFO("success\n");
	return RET_SUCCESS;
}

//4048x1520(V2binning) RAW10 30fps 4lanes 1050Mbps/lane
static int res_4_config_imx577_hdr(struct sensor_module_t *module)
{
	unsigned char bus = module->hw->sensor_i2c_index;

	ISP_PR_INFO("device id[%d]\n", module->id);
	config_common1_imx577(module);
	//MIPI setting
	SENSOR_I2C_WRITE(bus, 0x0112, 0x0A);
	SENSOR_I2C_WRITE(bus, 0x0113, 0x0A);
	SENSOR_I2C_WRITE(bus, 0x0114, 0x03);

    //Frame Horizontal Clock Count
	SENSOR_I2C_WRITE(bus, 0x0342, 0x21);
	SENSOR_I2C_WRITE(bus, 0x0343, 0xFC);

    //Frame Vertical Clock Count
	SENSOR_I2C_WRITE(bus, 0x0340, 0x0C);
	SENSOR_I2C_WRITE(bus, 0x0341, 0x80);
	SENSOR_I2C_WRITE(bus, 0x3210, 0x00);

    //Visible Size
	SENSOR_I2C_WRITE(bus, 0x0344, 0x00);
	SENSOR_I2C_WRITE(bus, 0x0345, 0x00);
	SENSOR_I2C_WRITE(bus, 0x0346, 0x00);
	SENSOR_I2C_WRITE(bus, 0x0347, 0x00);
	SENSOR_I2C_WRITE(bus, 0x0348, 0x0F);
	SENSOR_I2C_WRITE(bus, 0x0349, 0xD7);
	SENSOR_I2C_WRITE(bus, 0x034A, 0x0B);
	SENSOR_I2C_WRITE(bus, 0x034B, 0xDF);

    //Mode Setting
	SENSOR_I2C_WRITE(bus, 0x00E3, 0x00);
	SENSOR_I2C_WRITE(bus, 0x00E4, 0x00);
	SENSOR_I2C_WRITE(bus, 0x00E5, 0x01);
	SENSOR_I2C_WRITE(bus, 0x00FC, 0x0A);
	SENSOR_I2C_WRITE(bus, 0x00FD, 0x0A);
	SENSOR_I2C_WRITE(bus, 0x00FE, 0x0A);
	SENSOR_I2C_WRITE(bus, 0x00FF, 0x0A);
	SENSOR_I2C_WRITE(bus, 0xE013, 0x00);

	SENSOR_I2C_WRITE(bus, 0x0220, 0x01);
	SENSOR_I2C_WRITE(bus, 0x0221, 0x12);
	SENSOR_I2C_WRITE(bus, 0x0381, 0x01);
	SENSOR_I2C_WRITE(bus, 0x0383, 0x01);
	SENSOR_I2C_WRITE(bus, 0x0385, 0x01);
	SENSOR_I2C_WRITE(bus, 0x0387, 0x01);
	SENSOR_I2C_WRITE(bus, 0x0900, 0x00);
	SENSOR_I2C_WRITE(bus, 0x0901, 0x11);
	SENSOR_I2C_WRITE(bus, 0x0902, 0x00);
	SENSOR_I2C_WRITE(bus, 0x3140, 0x02);
	SENSOR_I2C_WRITE(bus, 0x3241, 0x11);
    //SENSOR_I2C_WRITE(bus, 0x3242, 0x01
	SENSOR_I2C_WRITE(bus, 0x3250, 0x00);
	SENSOR_I2C_WRITE(bus, 0x3E10, 0x00);
	SENSOR_I2C_WRITE(bus, 0x3E11, 0x00);
	SENSOR_I2C_WRITE(bus, 0x3F0D, 0x00);
	SENSOR_I2C_WRITE(bus, 0x3F42, 0x00);
	SENSOR_I2C_WRITE(bus, 0x3F43, 0x00);

    //Digital Crop & Scaling
	SENSOR_I2C_WRITE(bus, 0x0401, 0x00);
	SENSOR_I2C_WRITE(bus, 0x0404, 0x00);
	SENSOR_I2C_WRITE(bus, 0x0405, 0x10);
	SENSOR_I2C_WRITE(bus, 0x0408, 0x00);
	SENSOR_I2C_WRITE(bus, 0x0409, 0x04);
	SENSOR_I2C_WRITE(bus, 0x040A, 0x00);
	SENSOR_I2C_WRITE(bus, 0x040B, 0x00);
	SENSOR_I2C_WRITE(bus, 0x040C, 0x0F);
	SENSOR_I2C_WRITE(bus, 0x040D, 0xD0);
	SENSOR_I2C_WRITE(bus, 0x040E, 0x05);
	SENSOR_I2C_WRITE(bus, 0x040F, 0xF0);

    //Output Crop
	SENSOR_I2C_WRITE(bus, 0x034C, 0x0F);
	SENSOR_I2C_WRITE(bus, 0x034D, 0xD0);
	SENSOR_I2C_WRITE(bus, 0x034E, 0x05);
	SENSOR_I2C_WRITE(bus, 0x034F, 0xF0);

    //Clock Setting
	SENSOR_I2C_WRITE(bus, 0x0301, 0x05);
	SENSOR_I2C_WRITE(bus, 0x0303, 0x02);
	SENSOR_I2C_WRITE(bus, 0x0305, 0x04);
	SENSOR_I2C_WRITE(bus, 0x0306, 0x01);
	SENSOR_I2C_WRITE(bus, 0x0307, 0x5C);
	SENSOR_I2C_WRITE(bus, 0x0309, 0x0A);
	SENSOR_I2C_WRITE(bus, 0x030B, 0x01);
	SENSOR_I2C_WRITE(bus, 0x030D, 0x04);
	SENSOR_I2C_WRITE(bus, 0x030E, 0x00);
	SENSOR_I2C_WRITE(bus, 0x030F, 0xAF);
	SENSOR_I2C_WRITE(bus, 0x0310, 0x01);
	SENSOR_I2C_WRITE(bus, 0x0820, 0x10);
	SENSOR_I2C_WRITE(bus, 0x0821, 0x68);
	SENSOR_I2C_WRITE(bus, 0x0822, 0x00);
	SENSOR_I2C_WRITE(bus, 0x0823, 0x00);

    //Output Data Select Setting
	SENSOR_I2C_WRITE(bus, 0x3E20, 0x01);
	SENSOR_I2C_WRITE(bus, 0x3E37, 0x00);

    //PowerSave Setting
	SENSOR_I2C_WRITE(bus, 0x3F50, 0x00);
	SENSOR_I2C_WRITE(bus, 0x3F56, 0x00);
	SENSOR_I2C_WRITE(bus, 0x3F57, 0xFB);

#ifdef IMX577_EMBEDDED_DATA_EN
    //Embedded data
	SENSOR_I2C_WRITE(bus, 0xBCF1, 0x02);
#else
	SENSOR_I2C_WRITE(bus, 0xBCF1, 0x00);
#endif

    //Other Setting
	SENSOR_I2C_WRITE(bus, 0x3C0A, 0x5A);
	SENSOR_I2C_WRITE(bus, 0x3C0B, 0x55);
	SENSOR_I2C_WRITE(bus, 0x3C0C, 0x28);
	SENSOR_I2C_WRITE(bus, 0x3C0D, 0x07);
	SENSOR_I2C_WRITE(bus, 0x3C0E, 0xFF);
	SENSOR_I2C_WRITE(bus, 0x3C0F, 0x00);
	SENSOR_I2C_WRITE(bus, 0x3C10, 0x00);
	SENSOR_I2C_WRITE(bus, 0x3C11, 0x02);
	SENSOR_I2C_WRITE(bus, 0x3C12, 0x00);
	SENSOR_I2C_WRITE(bus, 0x3C13, 0x03);
	SENSOR_I2C_WRITE(bus, 0x3C14, 0x00);
	SENSOR_I2C_WRITE(bus, 0x3C15, 0x04);
	SENSOR_I2C_WRITE(bus, 0x3C16, 0x15);
	SENSOR_I2C_WRITE(bus, 0x3C17, 0x15);
	SENSOR_I2C_WRITE(bus, 0x3C18, 0x15);
	SENSOR_I2C_WRITE(bus, 0x3C19, 0x15);
	SENSOR_I2C_WRITE(bus, 0x3C1A, 0x15);
	SENSOR_I2C_WRITE(bus, 0x3C1B, 0x15);
	SENSOR_I2C_WRITE(bus, 0x3C1C, 0x06);
	SENSOR_I2C_WRITE(bus, 0x3C1D, 0x06);
	SENSOR_I2C_WRITE(bus, 0x3C1E, 0x06);
	SENSOR_I2C_WRITE(bus, 0x3C1F, 0x06);
	SENSOR_I2C_WRITE(bus, 0x3C20, 0x06);
	SENSOR_I2C_WRITE(bus, 0x3C21, 0x06);
	SENSOR_I2C_WRITE(bus, 0x3C22, 0x3F);
	SENSOR_I2C_WRITE(bus, 0x3C23, 0x0A);
	SENSOR_I2C_WRITE(bus, 0x3E35, 0x01);
	SENSOR_I2C_WRITE(bus, 0x3F4A, 0x03);
	SENSOR_I2C_WRITE(bus, 0x3F4B, 0xBF);
	SENSOR_I2C_WRITE(bus, 0x3F26, 0x01);

    //Integration Time Setting
	SENSOR_I2C_WRITE(bus, 0x0202, 0x06);
	SENSOR_I2C_WRITE(bus, 0x0203, 0x2A);

    //Gain Setting
	SENSOR_I2C_WRITE(bus, 0x0204, 0x00);
	SENSOR_I2C_WRITE(bus, 0x0205, 0x00);
	SENSOR_I2C_WRITE(bus, 0x020E, 0x01);
	SENSOR_I2C_WRITE(bus, 0x020F, 0x00);
	SENSOR_I2C_WRITE(bus, 0x0210, 0x01);
	SENSOR_I2C_WRITE(bus, 0x0211, 0x00);
	SENSOR_I2C_WRITE(bus, 0x0212, 0x01);
	SENSOR_I2C_WRITE(bus, 0x0213, 0x00);
	SENSOR_I2C_WRITE(bus, 0x0214, 0x01);
	SENSOR_I2C_WRITE(bus, 0x0215, 0x00);
	//Streaming
	SENSOR_I2C_WRITE(bus, 0x0100, 0x01);
	ISP_PR_INFO("success\n");
	return RET_SUCCESS;
}

//4048x1520(V2binning) RAW10 60fps 4lanes 1050Mbps/lane
static int res_5_config_imx577(struct sensor_module_t *module)
{
	unsigned char bus = module->hw->sensor_i2c_index;

	ISP_PR_INFO("device id[%d]\n", module->id);
	config_common1_imx577(module);
	//MIPI setting
	SENSOR_I2C_WRITE(bus, 0x0112, 0x0A);
	SENSOR_I2C_WRITE(bus, 0x0113, 0x0A);
	SENSOR_I2C_WRITE(bus, 0x0114, 0x03);

    //Frame Horizontal Clock Count
	SENSOR_I2C_WRITE(bus, 0x0342, 0x21);
	SENSOR_I2C_WRITE(bus, 0x0343, 0xFC);

    //Frame Vertical Clock Count
	SENSOR_I2C_WRITE(bus, 0x0340, 0x06);
	SENSOR_I2C_WRITE(bus, 0x0341, 0x40);
	SENSOR_I2C_WRITE(bus, 0x3210, 0x00);

    //Visible Size
	SENSOR_I2C_WRITE(bus, 0x0344, 0x00);
	SENSOR_I2C_WRITE(bus, 0x0345, 0x00);
	SENSOR_I2C_WRITE(bus, 0x0346, 0x00);
	SENSOR_I2C_WRITE(bus, 0x0347, 0x00);
	SENSOR_I2C_WRITE(bus, 0x0348, 0x0F);
	SENSOR_I2C_WRITE(bus, 0x0349, 0xD7);
	SENSOR_I2C_WRITE(bus, 0x034A, 0x0B);
	SENSOR_I2C_WRITE(bus, 0x034B, 0xDF);

    //Mode Setting
	SENSOR_I2C_WRITE(bus, 0x00E3, 0x00);
	SENSOR_I2C_WRITE(bus, 0x00E4, 0x00);
	SENSOR_I2C_WRITE(bus, 0x00E5, 0x01);
	SENSOR_I2C_WRITE(bus, 0x00FC, 0x0A);
	SENSOR_I2C_WRITE(bus, 0x00FD, 0x0A);
	SENSOR_I2C_WRITE(bus, 0x00FE, 0x0A);
	SENSOR_I2C_WRITE(bus, 0x00FF, 0x0A);
	SENSOR_I2C_WRITE(bus, 0xE013, 0x00);

	SENSOR_I2C_WRITE(bus, 0x0220, 0x00);
	SENSOR_I2C_WRITE(bus, 0x0221, 0x11);
	SENSOR_I2C_WRITE(bus, 0x0381, 0x01);
	SENSOR_I2C_WRITE(bus, 0x0383, 0x01);
	SENSOR_I2C_WRITE(bus, 0x0385, 0x01);
	SENSOR_I2C_WRITE(bus, 0x0387, 0x01);
	SENSOR_I2C_WRITE(bus, 0x0900, 0x01);
	SENSOR_I2C_WRITE(bus, 0x0901, 0x12);
	SENSOR_I2C_WRITE(bus, 0x0902, 0x00);
	SENSOR_I2C_WRITE(bus, 0x3140, 0x02);
	SENSOR_I2C_WRITE(bus, 0x3241, 0x11);
    //SENSOR_I2C_WRITE(bus, 0x3242, 0x01
	SENSOR_I2C_WRITE(bus, 0x3250, 0x00);
	SENSOR_I2C_WRITE(bus, 0x3E10, 0x00);
	SENSOR_I2C_WRITE(bus, 0x3E11, 0x00);
	SENSOR_I2C_WRITE(bus, 0x3F0D, 0x00);
	SENSOR_I2C_WRITE(bus, 0x3F42, 0x00);
	SENSOR_I2C_WRITE(bus, 0x3F43, 0x00);

    //Digital Crop & Scaling
	SENSOR_I2C_WRITE(bus, 0x0401, 0x00);
	SENSOR_I2C_WRITE(bus, 0x0404, 0x00);
	SENSOR_I2C_WRITE(bus, 0x0405, 0x10);
	SENSOR_I2C_WRITE(bus, 0x0408, 0x00);
	SENSOR_I2C_WRITE(bus, 0x0409, 0x04);
	SENSOR_I2C_WRITE(bus, 0x040A, 0x00);
	SENSOR_I2C_WRITE(bus, 0x040B, 0x00);
	SENSOR_I2C_WRITE(bus, 0x040C, 0x0F);
	SENSOR_I2C_WRITE(bus, 0x040D, 0xD0);
	SENSOR_I2C_WRITE(bus, 0x040E, 0x05);
	SENSOR_I2C_WRITE(bus, 0x040F, 0xF0);

    //Output Crop
	SENSOR_I2C_WRITE(bus, 0x034C, 0x0F);
	SENSOR_I2C_WRITE(bus, 0x034D, 0xD0);
	SENSOR_I2C_WRITE(bus, 0x034E, 0x05);
	SENSOR_I2C_WRITE(bus, 0x034F, 0xF0);

    //Clock Setting
	SENSOR_I2C_WRITE(bus, 0x0301, 0x05);
	SENSOR_I2C_WRITE(bus, 0x0303, 0x02);
	SENSOR_I2C_WRITE(bus, 0x0305, 0x04);
	SENSOR_I2C_WRITE(bus, 0x0306, 0x01);
	SENSOR_I2C_WRITE(bus, 0x0307, 0x5C);
	SENSOR_I2C_WRITE(bus, 0x0309, 0x0A);
	SENSOR_I2C_WRITE(bus, 0x030B, 0x01);
	SENSOR_I2C_WRITE(bus, 0x030D, 0x04);
	SENSOR_I2C_WRITE(bus, 0x030E, 0x00);
	SENSOR_I2C_WRITE(bus, 0x030F, 0xAF);
	SENSOR_I2C_WRITE(bus, 0x0310, 0x01);
	SENSOR_I2C_WRITE(bus, 0x0820, 0x10);
	SENSOR_I2C_WRITE(bus, 0x0821, 0x68);
	SENSOR_I2C_WRITE(bus, 0x0822, 0x00);
	SENSOR_I2C_WRITE(bus, 0x0823, 0x00);

    //Output Data Select Setting
	SENSOR_I2C_WRITE(bus, 0x3E20, 0x01);
	SENSOR_I2C_WRITE(bus, 0x3E37, 0x00);

    //PowerSave Setting
	SENSOR_I2C_WRITE(bus, 0x3F50, 0x00);
	SENSOR_I2C_WRITE(bus, 0x3F56, 0x00);
	SENSOR_I2C_WRITE(bus, 0x3F57, 0xFB);

#ifdef IMX577_EMBEDDED_DATA_EN
    //Embedded data
	SENSOR_I2C_WRITE(bus, 0xBCF1, 0x02);
#else
	SENSOR_I2C_WRITE(bus, 0xBCF1, 0x00);
#endif

    //Other Setting
	SENSOR_I2C_WRITE(bus, 0x3C0A, 0x73);
	SENSOR_I2C_WRITE(bus, 0x3C0B, 0x64);
	SENSOR_I2C_WRITE(bus, 0x3C0C, 0x5F);
	SENSOR_I2C_WRITE(bus, 0x3C0D, 0x07);
	SENSOR_I2C_WRITE(bus, 0x3C0E, 0xFF);
	SENSOR_I2C_WRITE(bus, 0x3C0F, 0x00);
	SENSOR_I2C_WRITE(bus, 0x3C10, 0x00);
	SENSOR_I2C_WRITE(bus, 0x3C11, 0x02);
	SENSOR_I2C_WRITE(bus, 0x3C12, 0x00);
	SENSOR_I2C_WRITE(bus, 0x3C13, 0x03);
	SENSOR_I2C_WRITE(bus, 0x3C14, 0x00);
	SENSOR_I2C_WRITE(bus, 0x3C15, 0x04);
	SENSOR_I2C_WRITE(bus, 0x3C16, 0x15);
	SENSOR_I2C_WRITE(bus, 0x3C17, 0x15);
	SENSOR_I2C_WRITE(bus, 0x3C18, 0x15);
	SENSOR_I2C_WRITE(bus, 0x3C19, 0x15);
	SENSOR_I2C_WRITE(bus, 0x3C1A, 0x15);
	SENSOR_I2C_WRITE(bus, 0x3C1B, 0x15);
	SENSOR_I2C_WRITE(bus, 0x3C1C, 0x06);
	SENSOR_I2C_WRITE(bus, 0x3C1D, 0x06);
	SENSOR_I2C_WRITE(bus, 0x3C1E, 0x06);
	SENSOR_I2C_WRITE(bus, 0x3C1F, 0x06);
	SENSOR_I2C_WRITE(bus, 0x3C20, 0x06);
	SENSOR_I2C_WRITE(bus, 0x3C21, 0x06);
	SENSOR_I2C_WRITE(bus, 0x3C22, 0x3F);
	SENSOR_I2C_WRITE(bus, 0x3C23, 0x0A);
	SENSOR_I2C_WRITE(bus, 0x3E35, 0x01);
	SENSOR_I2C_WRITE(bus, 0x3F4A, 0x03);
	SENSOR_I2C_WRITE(bus, 0x3F4B, 0xBF);
	SENSOR_I2C_WRITE(bus, 0x3F26, 0x01);

    //Integration Time Setting
	SENSOR_I2C_WRITE(bus, 0x0202, 0x06);
	SENSOR_I2C_WRITE(bus, 0x0203, 0x2A);

    //Gain Setting
	SENSOR_I2C_WRITE(bus, 0x0204, 0x00);
	SENSOR_I2C_WRITE(bus, 0x0205, 0x00);
	SENSOR_I2C_WRITE(bus, 0x020E, 0x01);
	SENSOR_I2C_WRITE(bus, 0x020F, 0x00);
	SENSOR_I2C_WRITE(bus, 0x0210, 0x01);
	SENSOR_I2C_WRITE(bus, 0x0211, 0x00);
	SENSOR_I2C_WRITE(bus, 0x0212, 0x01);
	SENSOR_I2C_WRITE(bus, 0x0213, 0x00);
	SENSOR_I2C_WRITE(bus, 0x0214, 0x01);
	SENSOR_I2C_WRITE(bus, 0x0215, 0x00);
	//Streaming
	SENSOR_I2C_WRITE(bus, 0x0100, 0x01);
	ISP_PR_INFO("success\n");
	return RET_SUCCESS;
}

//4048x1520(V2binning) RAW10 60fps 4lanes 1050Mbps/lane
static int res_5_config_imx577_hdr(struct sensor_module_t *module)
{
	unsigned char bus = module->hw->sensor_i2c_index;

	ISP_PR_INFO("device id[%d]\n", module->id);
	config_common1_imx577(module);
    //MIPI setting
	SENSOR_I2C_WRITE(bus, 0x0112, 0x0A);
	SENSOR_I2C_WRITE(bus, 0x0113, 0x0A);
	SENSOR_I2C_WRITE(bus, 0x0114, 0x03);

    //Frame Horizontal Clock Count
	SENSOR_I2C_WRITE(bus, 0x0342, 0x21);
	SENSOR_I2C_WRITE(bus, 0x0343, 0xFC);

    //Frame Vertical Clock Count
	SENSOR_I2C_WRITE(bus, 0x0340, 0x06);
	SENSOR_I2C_WRITE(bus, 0x0341, 0x40);
	SENSOR_I2C_WRITE(bus, 0x3210, 0x00);

    //Visible Size
	SENSOR_I2C_WRITE(bus, 0x0344, 0x00);
	SENSOR_I2C_WRITE(bus, 0x0345, 0x00);
	SENSOR_I2C_WRITE(bus, 0x0346, 0x00);
	SENSOR_I2C_WRITE(bus, 0x0347, 0x00);
	SENSOR_I2C_WRITE(bus, 0x0348, 0x0F);
	SENSOR_I2C_WRITE(bus, 0x0349, 0xD7);
	SENSOR_I2C_WRITE(bus, 0x034A, 0x0B);
	SENSOR_I2C_WRITE(bus, 0x034B, 0xDF);

    //Mode Setting
	SENSOR_I2C_WRITE(bus, 0x00E3, 0x00);
	SENSOR_I2C_WRITE(bus, 0x00E4, 0x00);
	SENSOR_I2C_WRITE(bus, 0x00E5, 0x01);
	SENSOR_I2C_WRITE(bus, 0x00FC, 0x0A);
	SENSOR_I2C_WRITE(bus, 0x00FD, 0x0A);
	SENSOR_I2C_WRITE(bus, 0x00FE, 0x0A);
	SENSOR_I2C_WRITE(bus, 0x00FF, 0x0A);
	SENSOR_I2C_WRITE(bus, 0xE013, 0x00);

	SENSOR_I2C_WRITE(bus, 0x0220, 0x01);
	SENSOR_I2C_WRITE(bus, 0x0221, 0x12);
	SENSOR_I2C_WRITE(bus, 0x0381, 0x01);
	SENSOR_I2C_WRITE(bus, 0x0383, 0x01);
	SENSOR_I2C_WRITE(bus, 0x0385, 0x01);
	SENSOR_I2C_WRITE(bus, 0x0387, 0x01);
	SENSOR_I2C_WRITE(bus, 0x0900, 0x00);
	SENSOR_I2C_WRITE(bus, 0x0901, 0x11);
	SENSOR_I2C_WRITE(bus, 0x0902, 0x00);
	SENSOR_I2C_WRITE(bus, 0x3140, 0x02);
	SENSOR_I2C_WRITE(bus, 0x3241, 0x11);
    //SENSOR_I2C_WRITE(bus, 0x3242, 0x01
	SENSOR_I2C_WRITE(bus, 0x3250, 0x00);
	SENSOR_I2C_WRITE(bus, 0x3E10, 0x00);
	SENSOR_I2C_WRITE(bus, 0x3E11, 0x00);
	SENSOR_I2C_WRITE(bus, 0x3F0D, 0x00);
	SENSOR_I2C_WRITE(bus, 0x3F42, 0x00);
	SENSOR_I2C_WRITE(bus, 0x3F43, 0x00);

    //Digital Crop & Scaling
	SENSOR_I2C_WRITE(bus, 0x0401, 0x00);
	SENSOR_I2C_WRITE(bus, 0x0404, 0x00);
	SENSOR_I2C_WRITE(bus, 0x0405, 0x10);
	SENSOR_I2C_WRITE(bus, 0x0408, 0x00);
	SENSOR_I2C_WRITE(bus, 0x0409, 0x04);
	SENSOR_I2C_WRITE(bus, 0x040A, 0x00);
	SENSOR_I2C_WRITE(bus, 0x040B, 0x00);
	SENSOR_I2C_WRITE(bus, 0x040C, 0x0F);
	SENSOR_I2C_WRITE(bus, 0x040D, 0xD0);
	SENSOR_I2C_WRITE(bus, 0x040E, 0x05);
	SENSOR_I2C_WRITE(bus, 0x040F, 0xF0);

    //Output Crop
	SENSOR_I2C_WRITE(bus, 0x034C, 0x0F);
	SENSOR_I2C_WRITE(bus, 0x034D, 0xD0);
	SENSOR_I2C_WRITE(bus, 0x034E, 0x05);
	SENSOR_I2C_WRITE(bus, 0x034F, 0xF0);

    //Clock Setting
	SENSOR_I2C_WRITE(bus, 0x0301, 0x05);
	SENSOR_I2C_WRITE(bus, 0x0303, 0x02);
	SENSOR_I2C_WRITE(bus, 0x0305, 0x04);
	SENSOR_I2C_WRITE(bus, 0x0306, 0x01);
	SENSOR_I2C_WRITE(bus, 0x0307, 0x5C);
	SENSOR_I2C_WRITE(bus, 0x0309, 0x0A);
	SENSOR_I2C_WRITE(bus, 0x030B, 0x01);
	SENSOR_I2C_WRITE(bus, 0x030D, 0x04);
	SENSOR_I2C_WRITE(bus, 0x030E, 0x00);
	SENSOR_I2C_WRITE(bus, 0x030F, 0xAF);
	SENSOR_I2C_WRITE(bus, 0x0310, 0x01);
	SENSOR_I2C_WRITE(bus, 0x0820, 0x10);
	SENSOR_I2C_WRITE(bus, 0x0821, 0x68);
	SENSOR_I2C_WRITE(bus, 0x0822, 0x00);
	SENSOR_I2C_WRITE(bus, 0x0823, 0x00);

    //Output Data Select Setting
	SENSOR_I2C_WRITE(bus, 0x3E20, 0x01);
	SENSOR_I2C_WRITE(bus, 0x3E37, 0x00);

    //PowerSave Setting
	SENSOR_I2C_WRITE(bus, 0x3F50, 0x00);
	SENSOR_I2C_WRITE(bus, 0x3F56, 0x00);
	SENSOR_I2C_WRITE(bus, 0x3F57, 0xFB);

#ifdef IMX577_EMBEDDED_DATA_EN
    //Embedded data
	SENSOR_I2C_WRITE(bus, 0xBCF1, 0x02);
#else
	SENSOR_I2C_WRITE(bus, 0xBCF1, 0x00);
#endif

    //Other Setting
	SENSOR_I2C_WRITE(bus, 0x3C0A, 0x5A);
	SENSOR_I2C_WRITE(bus, 0x3C0B, 0x55);
	SENSOR_I2C_WRITE(bus, 0x3C0C, 0x28);
	SENSOR_I2C_WRITE(bus, 0x3C0D, 0x07);
	SENSOR_I2C_WRITE(bus, 0x3C0E, 0xFF);
	SENSOR_I2C_WRITE(bus, 0x3C0F, 0x00);
	SENSOR_I2C_WRITE(bus, 0x3C10, 0x00);
	SENSOR_I2C_WRITE(bus, 0x3C11, 0x02);
	SENSOR_I2C_WRITE(bus, 0x3C12, 0x00);
	SENSOR_I2C_WRITE(bus, 0x3C13, 0x03);
	SENSOR_I2C_WRITE(bus, 0x3C14, 0x00);
	SENSOR_I2C_WRITE(bus, 0x3C15, 0x04);
	SENSOR_I2C_WRITE(bus, 0x3C16, 0x15);
	SENSOR_I2C_WRITE(bus, 0x3C17, 0x15);
	SENSOR_I2C_WRITE(bus, 0x3C18, 0x15);
	SENSOR_I2C_WRITE(bus, 0x3C19, 0x15);
	SENSOR_I2C_WRITE(bus, 0x3C1A, 0x15);
	SENSOR_I2C_WRITE(bus, 0x3C1B, 0x15);
	SENSOR_I2C_WRITE(bus, 0x3C1C, 0x06);
	SENSOR_I2C_WRITE(bus, 0x3C1D, 0x06);
	SENSOR_I2C_WRITE(bus, 0x3C1E, 0x06);
	SENSOR_I2C_WRITE(bus, 0x3C1F, 0x06);
	SENSOR_I2C_WRITE(bus, 0x3C20, 0x06);
	SENSOR_I2C_WRITE(bus, 0x3C21, 0x06);
	SENSOR_I2C_WRITE(bus, 0x3C22, 0x3F);
	SENSOR_I2C_WRITE(bus, 0x3C23, 0x0A);
	SENSOR_I2C_WRITE(bus, 0x3E35, 0x01);
	SENSOR_I2C_WRITE(bus, 0x3F4A, 0x03);
	SENSOR_I2C_WRITE(bus, 0x3F4B, 0xBF);
	SENSOR_I2C_WRITE(bus, 0x3F26, 0x01);

    //Integration Time Setting
	SENSOR_I2C_WRITE(bus, 0x0202, 0x06);
	SENSOR_I2C_WRITE(bus, 0x0203, 0x2A);

    //Gain Setting
	SENSOR_I2C_WRITE(bus, 0x0204, 0x00);
	SENSOR_I2C_WRITE(bus, 0x0205, 0x00);
	SENSOR_I2C_WRITE(bus, 0x020E, 0x01);
	SENSOR_I2C_WRITE(bus, 0x020F, 0x00);
	SENSOR_I2C_WRITE(bus, 0x0210, 0x01);
	SENSOR_I2C_WRITE(bus, 0x0211, 0x00);
	SENSOR_I2C_WRITE(bus, 0x0212, 0x01);
	SENSOR_I2C_WRITE(bus, 0x0213, 0x00);
	SENSOR_I2C_WRITE(bus, 0x0214, 0x01);
		SENSOR_I2C_WRITE(bus, 0x0215, 0x00);
	//Streaming
	SENSOR_I2C_WRITE(bus, 0x0100, 0x01);
	ISP_PR_INFO("success\n");
	return RET_SUCCESS;
}


//4056x3040 RAW10 30fps  4lanes 1050 Mbps/lane
static int res_6_config_imx577(struct sensor_module_t *module)
{
	unsigned char bus = module->hw->sensor_i2c_index;

	ISP_PR_INFO("device id[%d]\n", module->id);
	config_common1_imx577(module);

	// MIPI setting
	SENSOR_I2C_WRITE(bus, 0x0112, 0x0A);    //CSI_DT_FMT_H
	SENSOR_I2C_WRITE(bus, 0x0113, 0x0A);    //CSI_DT_FMT_L
	SENSOR_I2C_WRITE(bus, 0x0114, 0x03);    //CSI_LANE_MODE

	//Frame Horizontal Clock Count
	SENSOR_I2C_WRITE(bus, 0x0342, 0x11);    // LINE_LENGTH_PCK[15:8]
	SENSOR_I2C_WRITE(bus, 0x0343, 0xA0);    // LINE_LENGTH_PCK[7:0]
	//Frame Vertical Clock Count
	SENSOR_I2C_WRITE(bus, 0x0340, 0x0C);    // FRM_LENGTH_LINES[15:8]
	SENSOR_I2C_WRITE(bus, 0x0341, 0x1E);    // FRM_LENGTH_LINES[7:0]
	SENSOR_I2C_WRITE(bus, 0x3210, 0x00);    // FLL_LSHIFT
	//Visible Size
	SENSOR_I2C_WRITE(bus, 0x0344, 0x00);    //X_ADD_STA[12:8]
	SENSOR_I2C_WRITE(bus, 0x0345, 0x00);    //X_ADD_STA[7:0]
	SENSOR_I2C_WRITE(bus, 0x0346, 0x00);    //Y_ADD_STA[12:8]
	SENSOR_I2C_WRITE(bus, 0x0347, 0x00);    //Y_ADD_STA[7:0]
	SENSOR_I2C_WRITE(bus, 0x0348, 0x0F);    //X_ADD_END[12:8]
	SENSOR_I2C_WRITE(bus, 0x0349, 0xD7);    //X_ADD_END[7:0]
	SENSOR_I2C_WRITE(bus, 0x034A, 0x0B);    //Y_ADD_END[12:8]
	SENSOR_I2C_WRITE(bus, 0x034B, 0xDF);    //Y_ADD_END[7:0]

	//Mode Setting
	SENSOR_I2C_WRITE(bus, 0x00E3, 0x00);    //DOL_EN --> Disable DOL-HDR
	SENSOR_I2C_WRITE(bus, 0x00E4, 0x00);    //DOL_NUM
	SENSOR_I2C_WRITE(bus, 0x00E5, 0x01);    //DOL_OUTPUT_FMT
	SENSOR_I2C_WRITE(bus, 0x00FC, 0x0A);    //DOL_CSI_DT_FMT_H_2ND
	SENSOR_I2C_WRITE(bus, 0x00FD, 0x0A);    //DOL_CSI_DT_FMT_L_2ND
	SENSOR_I2C_WRITE(bus, 0x00FE, 0x0A);    //DOL_CSI_DT_FMT_H_3RD
	SENSOR_I2C_WRITE(bus, 0x00FF, 0x0A);    //DOL_CSI_DT_FMT_L_3RD
	SENSOR_I2C_WRITE(bus, 0xE013, 0x00);
	// HDR_MODE --> Disable SME-HDR_MODE
	SENSOR_I2C_WRITE(bus, 0x0220, 0x00);
	SENSOR_I2C_WRITE(bus, 0x0221, 0x11);
	SENSOR_I2C_WRITE(bus, 0x0381, 0x01);    //X_EVN_INC
	SENSOR_I2C_WRITE(bus, 0x0383, 0x01);    //X_ODD_INC
	SENSOR_I2C_WRITE(bus, 0x0385, 0x01);    //Y_EVN_INC
	SENSOR_I2C_WRITE(bus, 0x0387, 0x01);    //Y_ODD_INC
	SENSOR_I2C_WRITE(bus, 0x0900, 0x00);    //BINNING_MODE
	//[7:4] BINNING_TYPE_H, [3:0] BINNING_TYPE_V
	SENSOR_I2C_WRITE(bus, 0x0901, 0x11);
	SENSOR_I2C_WRITE(bus, 0x0902, 0x00);    //BINNING_WEIGHTING
	SENSOR_I2C_WRITE(bus, 0x3140, 0x02);    //HDR_FNCSEL
	SENSOR_I2C_WRITE(bus, 0x3241, 0x11);
	SENSOR_I2C_WRITE(bus, 0x3250, 0x03);    //SUB_WEIGHTING
	SENSOR_I2C_WRITE(bus, 0x3E10, 0x00);    //need to confirm
	SENSOR_I2C_WRITE(bus, 0x3E11, 0x00);    //need to confirm
	SENSOR_I2C_WRITE(bus, 0x3F0D, 0x00);    //ADBIT_MODE
	SENSOR_I2C_WRITE(bus, 0x3F42, 0x00);    //BINNING_TYPE_EXT_EN
	SENSOR_I2C_WRITE(bus, 0x3F43, 0x00);    //BINNING_TYPE_H_EXT

	//Digital Crop & Scaling
	SENSOR_I2C_WRITE(bus, 0x0401, 0x00);    //SCALE_MODE
	SENSOR_I2C_WRITE(bus, 0x0404, 0x00);    //SCALE_M[8]
	SENSOR_I2C_WRITE(bus, 0x0405, 0x10);    //SCALE_M[7:0]
	SENSOR_I2C_WRITE(bus, 0x0408, 0x00);    //DIG_CROP_X_OFFSET[12:8]
	SENSOR_I2C_WRITE(bus, 0x0409, 0x00);    //DIG_CROP_X_OFFSET[7:0]
	SENSOR_I2C_WRITE(bus, 0x040A, 0x00);    //DIG_CROP_Y_OFFSET[12:8]
	SENSOR_I2C_WRITE(bus, 0x040B, 0x00);    //DIG_CROP_Y_OFFSET[7:0]
	SENSOR_I2C_WRITE(bus, 0x040C, 0x0F);    //DIG_CROP_IMAGE_WIDTH[12:8]
	SENSOR_I2C_WRITE(bus, 0x040D, 0xD8);    //DIG_CROP_IMAGE_WIDTH[7:0]
	SENSOR_I2C_WRITE(bus, 0x040E, 0x0B);    //DIG_CROP_IMAGE_HEIGHT[12:8]
	SENSOR_I2C_WRITE(bus, 0x040F, 0xE0);    //DIG_CROP_IMAGE_HEIGHT[7:0]

	//Output Crop
	SENSOR_I2C_WRITE(bus, 0x034C, 0x0F);    //X_OUT_SIZE[12:8]
	SENSOR_I2C_WRITE(bus, 0x034D, 0xD8);    //X_OUT_SIZE[7:0]
	SENSOR_I2C_WRITE(bus, 0x034E, 0x0B);    //Y_OUT_SIZE[15:8]
	SENSOR_I2C_WRITE(bus, 0x034F, 0xE0);    //Y_OUT_SIZE[7:0]

	//Clock Setting
	SENSOR_I2C_WRITE(bus, 0x0301, 0x05);    // IVT_PXCK_DIV
	SENSOR_I2C_WRITE(bus, 0x0303, 0x02);    // IVT_SYCK_DIV
	SENSOR_I2C_WRITE(bus, 0x0305, 0x04);    // IVT_PREPLLCK_DIV
	SENSOR_I2C_WRITE(bus, 0x0306, 0x00);    // IVT_PLL_MPY[11:8]
	SENSOR_I2C_WRITE(bus, 0x0307, 0xAF);    // IVT_PLL_MPY[7:0]
	SENSOR_I2C_WRITE(bus, 0x0309, 0x0A);    // IOP_PXCK_DIV
	SENSOR_I2C_WRITE(bus, 0x030B, 0x01);    // IOP_SYCK_DIV
	SENSOR_I2C_WRITE(bus, 0x030D, 0x02);    // IOP_PREPLLCK_DIV
	SENSOR_I2C_WRITE(bus, 0x030E, 0x01);    // IOP_PLL_MPY[11:8]
	SENSOR_I2C_WRITE(bus, 0x030F, 0x5E);    // IOP_PLL_MPY[7:0]
	SENSOR_I2C_WRITE(bus, 0x0310, 0x00);    // PLL_MULT_DRIV
	SENSOR_I2C_WRITE(bus, 0x0820, 0x10);    // REQ_LINK_BIT_RATE_MBPS[31:24]
	SENSOR_I2C_WRITE(bus, 0x0821, 0x68);    // REQ_LINK_BIT_RATE_MBPS[23:16]
	SENSOR_I2C_WRITE(bus, 0x0822, 0x00);    // REQ_LINK_BIT_RATE_MBPS[15:8]
	SENSOR_I2C_WRITE(bus, 0x0823, 0x00);    // REQ_LINK_BIT_RATE_MBPS[7:0]

	//Output Data Select Setting
	SENSOR_I2C_WRITE(bus, 0x3E20, 0x01);    // PDAF_TYPE_SEL
	SENSOR_I2C_WRITE(bus, 0x3E37, 0x00);    // PDAF_CTRL1[7:0]

	//PowerSave Setting
	SENSOR_I2C_WRITE(bus, 0x3F50, 0x00);    //POWER_SAVE_ENABLE
	SENSOR_I2C_WRITE(bus, 0x3F56, 0x00);    //LINE_LENGTH_INCK[15:8]
	SENSOR_I2C_WRITE(bus, 0x3F57, 0xFB);    //LINE_LENGTH_INCK[7:0]

#ifdef IMX577_EMBEDDED_DATA_EN
	//Embedded data
	SENSOR_I2C_WRITE(bus, 0xBCF1, 0x02);
#else
	SENSOR_I2C_WRITE(bus, 0xBCF1, 0x00);
#endif

	//Other Setting
	SENSOR_I2C_WRITE(bus, 0x3C0A, 0x73);
	SENSOR_I2C_WRITE(bus, 0x3C0B, 0x64);
	SENSOR_I2C_WRITE(bus, 0x3C0C, 0x5F);
	SENSOR_I2C_WRITE(bus, 0x3C0D, 0x07);
	SENSOR_I2C_WRITE(bus, 0x3C0E, 0xFF);
	SENSOR_I2C_WRITE(bus, 0x3C0F, 0x00);
	SENSOR_I2C_WRITE(bus, 0x3C10, 0x00);
	SENSOR_I2C_WRITE(bus, 0x3C11, 0x02);
	SENSOR_I2C_WRITE(bus, 0x3C12, 0x00);
	SENSOR_I2C_WRITE(bus, 0x3C13, 0x03);
	SENSOR_I2C_WRITE(bus, 0x3C14, 0x00);
	SENSOR_I2C_WRITE(bus, 0x3C15, 0x00);
	SENSOR_I2C_WRITE(bus, 0x3C16, 0x0C);
	SENSOR_I2C_WRITE(bus, 0x3C17, 0x0C);
	SENSOR_I2C_WRITE(bus, 0x3C18, 0x0C);
	SENSOR_I2C_WRITE(bus, 0x3C19, 0x0A);
	SENSOR_I2C_WRITE(bus, 0x3C1A, 0x0A);
	SENSOR_I2C_WRITE(bus, 0x3C1B, 0x0A);
	SENSOR_I2C_WRITE(bus, 0x3C1C, 0x00);
	SENSOR_I2C_WRITE(bus, 0x3C1D, 0x00);
	SENSOR_I2C_WRITE(bus, 0x3C1E, 0x00);
	SENSOR_I2C_WRITE(bus, 0x3C1F, 0x00);
	SENSOR_I2C_WRITE(bus, 0x3C20, 0x00);
	SENSOR_I2C_WRITE(bus, 0x3C21, 0x00);
	SENSOR_I2C_WRITE(bus, 0x3C22, 0x3F);
	SENSOR_I2C_WRITE(bus, 0x3C23, 0x0A);
	SENSOR_I2C_WRITE(bus, 0x3E35, 0x01);    //SPC control
	SENSOR_I2C_WRITE(bus, 0x3F4A, 0x03);
	SENSOR_I2C_WRITE(bus, 0x3F4B, 0xBF);
	SENSOR_I2C_WRITE(bus, 0x3F26, 0x00);

	//Integration Time Setting
	SENSOR_I2C_WRITE(bus, 0x0202, 0x0C);    //COARSE_INTEG_TIME[15:8]
	SENSOR_I2C_WRITE(bus, 0x0203, 0x6A);    //COARSE_INTEG_TIME[7:0]

	//Gain Setting
	SENSOR_I2C_WRITE(bus, 0x0204, 0x00);    //ANA_GAIN_GLOBAL[9:8]
	SENSOR_I2C_WRITE(bus, 0x0205, 0x00);    //ANA_GAIN_GLOBAL[7:0]
	SENSOR_I2C_WRITE(bus, 0x020E, 0x01);    //DIG_GAIN_GR[15:8]
	SENSOR_I2C_WRITE(bus, 0x020F, 0x00);    //DIG_GAIN_GR[7:0]
	SENSOR_I2C_WRITE(bus, 0x0210, 0x01);    //DIG_GAIN_R[15:8]
	SENSOR_I2C_WRITE(bus, 0x0211, 0x00);    //DIG_GAIN_R[7:0]
	SENSOR_I2C_WRITE(bus, 0x0212, 0x01);    //DIG_GAIN_B[15:8]
	SENSOR_I2C_WRITE(bus, 0x0213, 0x00);    //DIG_GAIN_B[7:0]
	SENSOR_I2C_WRITE(bus, 0x0214, 0x01);    //DIG_GAIN_GB[15:8]
	SENSOR_I2C_WRITE(bus, 0x0215, 0x00);    //DIG_GAIN_GB[7:0]
	//Streaming
	SENSOR_I2C_WRITE(bus, 0x0100, 0x01);
	ISP_PR_INFO("success\n");
	return RET_SUCCESS;
}

//4056x3040 RAW10 30fps  4lanes 1050 Mbps/lane
static int res_6_config_imx577_hdr(struct sensor_module_t *module)
{
	unsigned char bus = module->hw->sensor_i2c_index;

	ISP_PR_INFO("device id[%d]\n", module->id);
	config_common1_imx577(module);
	// MIPI setting
	SENSOR_I2C_WRITE(bus, 0x0112, 0x0A);    //CSI_DT_FMT_H
	SENSOR_I2C_WRITE(bus, 0x0113, 0x0A);    //CSI_DT_FMT_L
	SENSOR_I2C_WRITE(bus, 0x0114, 0x03);    //CSI_LANE_MODE

	//Frame Horizontal Clock Count
	SENSOR_I2C_WRITE(bus, 0x0342, 0x11);    // LINE_LENGTH_PCK[15:8]
	SENSOR_I2C_WRITE(bus, 0x0343, 0xA0);    // LINE_LENGTH_PCK[7:0]
	//Frame Vertical Clock Count
	SENSOR_I2C_WRITE(bus, 0x0340, 0x0C);    // FRM_LENGTH_LINES[15:8]
	SENSOR_I2C_WRITE(bus, 0x0341, 0x1E);    // FRM_LENGTH_LINES[7:0]
	SENSOR_I2C_WRITE(bus, 0x3210, 0x00);    // FLL_LSHIFT
	//Visible Size
	SENSOR_I2C_WRITE(bus, 0x0344, 0x00);    //X_ADD_STA[12:8]
	SENSOR_I2C_WRITE(bus, 0x0345, 0x00);    //X_ADD_STA[7:0]
	SENSOR_I2C_WRITE(bus, 0x0346, 0x00);    //Y_ADD_STA[12:8]
	SENSOR_I2C_WRITE(bus, 0x0347, 0x00);    //Y_ADD_STA[7:0]
	SENSOR_I2C_WRITE(bus, 0x0348, 0x0F);    //X_ADD_END[12:8]
	SENSOR_I2C_WRITE(bus, 0x0349, 0xD7);    //X_ADD_END[7:0]
	SENSOR_I2C_WRITE(bus, 0x034A, 0x0B);    //Y_ADD_END[12:8]
	SENSOR_I2C_WRITE(bus, 0x034B, 0xDF);    //Y_ADD_END[7:0]

	//Mode Setting
	SENSOR_I2C_WRITE(bus, 0x00E3, 0x00);    //DOL_EN --> Disable DOL-HDR
	SENSOR_I2C_WRITE(bus, 0x00E4, 0x00);    //DOL_NUM
	SENSOR_I2C_WRITE(bus, 0x00E5, 0x01);    //DOL_OUTPUT_FMT
	SENSOR_I2C_WRITE(bus, 0x00FC, 0x0A);    //DOL_CSI_DT_FMT_H_2ND
	SENSOR_I2C_WRITE(bus, 0x00FD, 0x0A);    //DOL_CSI_DT_FMT_L_2ND
	SENSOR_I2C_WRITE(bus, 0x00FE, 0x0A);    //DOL_CSI_DT_FMT_H_3RD
	SENSOR_I2C_WRITE(bus, 0x00FF, 0x0A);    //DOL_CSI_DT_FMT_L_3RD
	SENSOR_I2C_WRITE(bus, 0xE013, 0x00);

	SENSOR_I2C_WRITE(bus, 0x0220, 0x01);    // HDR_MODE
	SENSOR_I2C_WRITE(bus, 0x0221, 0x11);    // HDR_RESO_REDU_V
//	SENSOR_I2C_WRITE(bus, 0x0222, 0x08);     //set expo ratio

	SENSOR_I2C_WRITE(bus, 0x0381, 0x01);    //X_EVN_INC
	SENSOR_I2C_WRITE(bus, 0x0383, 0x01);    //X_ODD_INC
	SENSOR_I2C_WRITE(bus, 0x0385, 0x01);    //Y_EVN_INC
	SENSOR_I2C_WRITE(bus, 0x0387, 0x01);    //Y_ODD_INC
	SENSOR_I2C_WRITE(bus, 0x0900, 0x00);    //BINNING_MODE
	//[7:4] BINNING_TYPE_H, [3:0] BINNING_TYPE_V
	SENSOR_I2C_WRITE(bus, 0x0901, 0x11);
	SENSOR_I2C_WRITE(bus, 0x0902, 0x00);    //BINNING_WEIGHTING
	SENSOR_I2C_WRITE(bus, 0x3140, 0x02);    //HDR_FNCSEL
	SENSOR_I2C_WRITE(bus, 0x3241, 0x11);
	SENSOR_I2C_WRITE(bus, 0x3250, 0x03);    //SUB_WEIGHTING
	SENSOR_I2C_WRITE(bus, 0x3E10, 0x00);    //need to confirm
	SENSOR_I2C_WRITE(bus, 0x3E11, 0x00);    //need to confirm
	SENSOR_I2C_WRITE(bus, 0x3F0D, 0x00);    //ADBIT_MODE
	SENSOR_I2C_WRITE(bus, 0x3F42, 0x00);    //BINNING_TYPE_EXT_EN
	SENSOR_I2C_WRITE(bus, 0x3F43, 0x00);    //BINNING_TYPE_H_EXT

	//Digital Crop & Scaling
	SENSOR_I2C_WRITE(bus, 0x0401, 0x00);    //SCALE_MODE
	SENSOR_I2C_WRITE(bus, 0x0404, 0x00);    //SCALE_M[8]
	SENSOR_I2C_WRITE(bus, 0x0405, 0x10);    //SCALE_M[7:0]
	SENSOR_I2C_WRITE(bus, 0x0408, 0x00);    //DIG_CROP_X_OFFSET[12:8]
	SENSOR_I2C_WRITE(bus, 0x0409, 0x00);    //DIG_CROP_X_OFFSET[7:0]
	SENSOR_I2C_WRITE(bus, 0x040A, 0x00);    //DIG_CROP_Y_OFFSET[12:8]
	SENSOR_I2C_WRITE(bus, 0x040B, 0x00);    //DIG_CROP_Y_OFFSET[7:0]
	SENSOR_I2C_WRITE(bus, 0x040C, 0x0F);    //DIG_CROP_IMAGE_WIDTH[12:8]
	SENSOR_I2C_WRITE(bus, 0x040D, 0xD8);    //DIG_CROP_IMAGE_WIDTH[7:0]
	SENSOR_I2C_WRITE(bus, 0x040E, 0x0B);    //DIG_CROP_IMAGE_HEIGHT[12:8]
	SENSOR_I2C_WRITE(bus, 0x040F, 0xE0);    //DIG_CROP_IMAGE_HEIGHT[7:0]

	//Output Crop
	SENSOR_I2C_WRITE(bus, 0x034C, 0x0F);    //X_OUT_SIZE[12:8]
	SENSOR_I2C_WRITE(bus, 0x034D, 0xD8);    //X_OUT_SIZE[7:0]
	SENSOR_I2C_WRITE(bus, 0x034E, 0x0B);    //Y_OUT_SIZE[15:8]
	SENSOR_I2C_WRITE(bus, 0x034F, 0xE0);    //Y_OUT_SIZE[7:0]

	//Clock Setting
	SENSOR_I2C_WRITE(bus, 0x0301, 0x05);    // IVT_PXCK_DIV
	SENSOR_I2C_WRITE(bus, 0x0303, 0x02);    // IVT_SYCK_DIV
	SENSOR_I2C_WRITE(bus, 0x0305, 0x04);    // IVT_PREPLLCK_DIV
	SENSOR_I2C_WRITE(bus, 0x0306, 0x00);    // IVT_PLL_MPY[11:8]
	SENSOR_I2C_WRITE(bus, 0x0307, 0xAF);    // IVT_PLL_MPY[7:0]
	SENSOR_I2C_WRITE(bus, 0x0309, 0x0A);    // IOP_PXCK_DIV
	SENSOR_I2C_WRITE(bus, 0x030B, 0x01);    // IOP_SYCK_DIV
	SENSOR_I2C_WRITE(bus, 0x030D, 0x02);    // IOP_PREPLLCK_DIV
	SENSOR_I2C_WRITE(bus, 0x030E, 0x01);    // IOP_PLL_MPY[11:8]
	SENSOR_I2C_WRITE(bus, 0x030F, 0x5E);    // IOP_PLL_MPY[7:0]
	SENSOR_I2C_WRITE(bus, 0x0310, 0x00);    // PLL_MULT_DRIV
	SENSOR_I2C_WRITE(bus, 0x0820, 0x10);    // REQ_LINK_BIT_RATE_MBPS[31:24]
	SENSOR_I2C_WRITE(bus, 0x0821, 0x68);    // REQ_LINK_BIT_RATE_MBPS[23:16]
	SENSOR_I2C_WRITE(bus, 0x0822, 0x00);    // REQ_LINK_BIT_RATE_MBPS[15:8]
	SENSOR_I2C_WRITE(bus, 0x0823, 0x00);    // REQ_LINK_BIT_RATE_MBPS[7:0]

	//Output Data Select Setting
	SENSOR_I2C_WRITE(bus, 0x3E20, 0x01);    // PDAF_TYPE_SEL
	SENSOR_I2C_WRITE(bus, 0x3E37, 0x00);    // PDAF_CTRL1[7:0]

	//PowerSave Setting
	SENSOR_I2C_WRITE(bus, 0x3F50, 0x00);    //POWER_SAVE_ENABLE
	SENSOR_I2C_WRITE(bus, 0x3F56, 0x00);    //LINE_LENGTH_INCK[15:8]
	SENSOR_I2C_WRITE(bus, 0x3F57, 0xFB);    //LINE_LENGTH_INCK[7:0]
#ifdef IMX577_EMBEDDED_DATA_EN
	//Embedded data
	SENSOR_I2C_WRITE(bus, 0xBCF1, 0x02);
#else
	SENSOR_I2C_WRITE(bus, 0xBCF1, 0x00);
#endif

	//Other Setting
	SENSOR_I2C_WRITE(bus, 0x3C0A, 0x5A);
	SENSOR_I2C_WRITE(bus, 0x3C0B, 0x55);
	SENSOR_I2C_WRITE(bus, 0x3C0C, 0x28);
	SENSOR_I2C_WRITE(bus, 0x3C0D, 0x07);
	SENSOR_I2C_WRITE(bus, 0x3C0E, 0xFF);
	SENSOR_I2C_WRITE(bus, 0x3C0F, 0x00);
	SENSOR_I2C_WRITE(bus, 0x3C10, 0x00);
	SENSOR_I2C_WRITE(bus, 0x3C11, 0x02);
	SENSOR_I2C_WRITE(bus, 0x3C12, 0x00);
	SENSOR_I2C_WRITE(bus, 0x3C13, 0x03);
	SENSOR_I2C_WRITE(bus, 0x3C14, 0x00);
	SENSOR_I2C_WRITE(bus, 0x3C15, 0x00);
	SENSOR_I2C_WRITE(bus, 0x3C16, 0x0C);
	SENSOR_I2C_WRITE(bus, 0x3C17, 0x0C);
	SENSOR_I2C_WRITE(bus, 0x3C18, 0x0C);
	SENSOR_I2C_WRITE(bus, 0x3C19, 0x0A);
	SENSOR_I2C_WRITE(bus, 0x3C1A, 0x0A);
	SENSOR_I2C_WRITE(bus, 0x3C1B, 0x0A);
	SENSOR_I2C_WRITE(bus, 0x3C1C, 0x00);
	SENSOR_I2C_WRITE(bus, 0x3C1D, 0x00);
	SENSOR_I2C_WRITE(bus, 0x3C1E, 0x00);
	SENSOR_I2C_WRITE(bus, 0x3C1F, 0x00);
	SENSOR_I2C_WRITE(bus, 0x3C20, 0x00);
	SENSOR_I2C_WRITE(bus, 0x3C21, 0x00);
	SENSOR_I2C_WRITE(bus, 0x3C22, 0x3F);
	SENSOR_I2C_WRITE(bus, 0x3C23, 0x0A);
	SENSOR_I2C_WRITE(bus, 0x3E35, 0x01);    //SPC control
	SENSOR_I2C_WRITE(bus, 0x3F4A, 0x03);
	SENSOR_I2C_WRITE(bus, 0x3F4B, 0xBF);
	SENSOR_I2C_WRITE(bus, 0x3F26, 0x00);

	//Integration Time Setting
	SENSOR_I2C_WRITE(bus, 0x0202, 0x0C);    //COARSE_INTEG_TIME[15:8]
	SENSOR_I2C_WRITE(bus, 0x0203, 0x6A);    //COARSE_INTEG_TIME[7:0]

	//Gain Setting
	SENSOR_I2C_WRITE(bus, 0x0204, 0x00);    //ANA_GAIN_GLOBAL[9:8]
	SENSOR_I2C_WRITE(bus, 0x0205, 0x00);    //ANA_GAIN_GLOBAL[7:0]
	SENSOR_I2C_WRITE(bus, 0x020E, 0x01);    //DIG_GAIN_GR[15:8]
	SENSOR_I2C_WRITE(bus, 0x020F, 0x00);    //DIG_GAIN_GR[7:0]
	SENSOR_I2C_WRITE(bus, 0x0210, 0x01);    //DIG_GAIN_R[15:8]
	SENSOR_I2C_WRITE(bus, 0x0211, 0x00);    //DIG_GAIN_R[7:0]
	SENSOR_I2C_WRITE(bus, 0x0212, 0x01);    //DIG_GAIN_B[15:8]
	SENSOR_I2C_WRITE(bus, 0x0213, 0x00);    //DIG_GAIN_B[7:0]
	SENSOR_I2C_WRITE(bus, 0x0214, 0x01);    //DIG_GAIN_GB[15:8]
	SENSOR_I2C_WRITE(bus, 0x0215, 0x00);    //DIG_GAIN_GB[7:0]
	//Streaming
	SENSOR_I2C_WRITE(bus, 0x0100, 0x01);
	ISP_PR_INFO("success\n");
	return RET_SUCCESS;
}

static int res_7_config_imx577(struct sensor_module_t *module)
{
	unsigned char bus = module->hw->sensor_i2c_index;

	ISP_PR_INFO("device id[%d]\n", module->id);
	config_common1_imx577(module);
	//MIPI setting
	SENSOR_I2C_WRITE(bus, 0x0112, 0x0A);    //CSI_DT_FMT_H
	SENSOR_I2C_WRITE(bus, 0x0113, 0x0A);    //CSI_DT_FMT_L
	SENSOR_I2C_WRITE(bus, 0x0114, 0x01);    //CSI_LANE_MODE

	//Frame Horizontal Clock Count
	SENSOR_I2C_WRITE(bus, 0x0342, 0x3A);    //LINE_LENGTH_PCK[15:8]
	SENSOR_I2C_WRITE(bus, 0x0343, 0x98);    //LINE_LENGTH_PCK[7:0]

	//Frame Vertical Clock Count
	SENSOR_I2C_WRITE(bus, 0x0340, 0x0C);    //FRM_LENGTH_LINES[15:8]
	SENSOR_I2C_WRITE(bus, 0x0341, 0x1E);    //FRM_LENGTH_LINES[7:0]
	SENSOR_I2C_WRITE(bus, 0x3210, 0x00);    //FLL_LSHIFT

	//Visible Size
	SENSOR_I2C_WRITE(bus, 0x0344, 0x00);    //X_ADD_STA[12:8]
	SENSOR_I2C_WRITE(bus, 0x0345, 0x00);    //X_ADD_STA[7:0]
	SENSOR_I2C_WRITE(bus, 0x0346, 0x00);    //Y_ADD_STA[12:8]
	SENSOR_I2C_WRITE(bus, 0x0347, 0x00);    //Y_ADD_STA[7:0]
	SENSOR_I2C_WRITE(bus, 0x0348, 0x0F);    //X_ADD_END[12:8]
	SENSOR_I2C_WRITE(bus, 0x0349, 0xD7);    //X_ADD_END[7:0]
	SENSOR_I2C_WRITE(bus, 0x034A, 0x0B);    //Y_ADD_END[12:8]
	SENSOR_I2C_WRITE(bus, 0x034B, 0xDF);    //Y_ADD_END[7:0]

	//Mode Setting
	SENSOR_I2C_WRITE(bus, 0x00E3, 0x00);    //DOL_EN --> Disable DOL-HDR
	SENSOR_I2C_WRITE(bus, 0x00E4, 0x00);    //DOL_NUM
	SENSOR_I2C_WRITE(bus, 0x00E5, 0x01);    //DOL_OUTPUT_FMT
	SENSOR_I2C_WRITE(bus, 0x00FC, 0x0A);    //DOL_CSI_DT_FMT_H_2ND
	SENSOR_I2C_WRITE(bus, 0x00FD, 0x0A);    //DOL_CSI_DT_FMT_L_2ND
	SENSOR_I2C_WRITE(bus, 0x00FE, 0x0A);    //DOL_CSI_DT_FMT_H_3RD
	SENSOR_I2C_WRITE(bus, 0x00FF, 0x0A);    //DOL_CSI_DT_FMT_L_3RD
	SENSOR_I2C_WRITE(bus, 0xE013, 0x00);

	// HDR_MODE --> Disable SME-HDR_MODE
	SENSOR_I2C_WRITE(bus, 0x0220, 0x00);
	SENSOR_I2C_WRITE(bus, 0x0221, 0x11);
	SENSOR_I2C_WRITE(bus, 0x0381, 0x01);    //X_EVN_INC
	SENSOR_I2C_WRITE(bus, 0x0383, 0x01);    //X_ODD_INC
	SENSOR_I2C_WRITE(bus, 0x0385, 0x01);    //Y_EVN_INC
	SENSOR_I2C_WRITE(bus, 0x0387, 0x01);    //Y_ODD_INC
	SENSOR_I2C_WRITE(bus, 0x0900, 0x00);    //BINNING_MODE
	//[7:4] BINNING_TYPE_H, [3:0] BINNING_TYPE_V
	SENSOR_I2C_WRITE(bus, 0x0901, 0x11);
	SENSOR_I2C_WRITE(bus, 0x0902, 0x00);    //BINNING_WEIGHTING
	SENSOR_I2C_WRITE(bus, 0x3140, 0x02);    //HDR_FNCSEL
	SENSOR_I2C_WRITE(bus, 0x3241, 0x11);
	SENSOR_I2C_WRITE(bus, 0x3250, 0x03);    //SUB_WEIGHTING
	SENSOR_I2C_WRITE(bus, 0x3E10, 0x00);    //need to confirm
	SENSOR_I2C_WRITE(bus, 0x3E11, 0x00);    //need to confirm
	SENSOR_I2C_WRITE(bus, 0x3F0D, 0x00);    //ADBIT_MODE
	SENSOR_I2C_WRITE(bus, 0x3F42, 0x00);    //BINNING_TYPE_EXT_EN
	SENSOR_I2C_WRITE(bus, 0x3F43, 0x00);    //BINNING_TYPE_H_EXT

	//Digital Crop & Scaling
	SENSOR_I2C_WRITE(bus, 0x0401, 0x00);    //SCALE_MODE
	SENSOR_I2C_WRITE(bus, 0x0404, 0x00);    //SCALE_M[8]
	SENSOR_I2C_WRITE(bus, 0x0405, 0x10);    //SCALE_M[7:0]
	SENSOR_I2C_WRITE(bus, 0x0408, 0x00);    //DIG_CROP_X_OFFSET[12:8]
	SENSOR_I2C_WRITE(bus, 0x0409, 0x00);    //DIG_CROP_X_OFFSET[7:0]
	SENSOR_I2C_WRITE(bus, 0x040A, 0x00);    //DIG_CROP_Y_OFFSET[12:8]
	SENSOR_I2C_WRITE(bus, 0x040B, 0x00);    //DIG_CROP_Y_OFFSET[7:0]
	SENSOR_I2C_WRITE(bus, 0x040C, 0x0F);    //DIG_CROP_IMAGE_WIDTH[12:8]
	SENSOR_I2C_WRITE(bus, 0x040D, 0xD8);    //DIG_CROP_IMAGE_WIDTH[7:0]
	SENSOR_I2C_WRITE(bus, 0x040E, 0x0B);    //DIG_CROP_IMAGE_HEIGHT[12:8]
	SENSOR_I2C_WRITE(bus, 0x040F, 0xE0);    //DIG_CROP_IMAGE_HEIGHT[7:0]

	//Output Crop
	SENSOR_I2C_WRITE(bus, 0x034C, 0x0F);    //X_OUT_SIZE[12:8]
	SENSOR_I2C_WRITE(bus, 0x034D, 0xD8);    //X_OUT_SIZE[7:0]
	SENSOR_I2C_WRITE(bus, 0x034E, 0x0B);    //Y_OUT_SIZE[15:8]
	SENSOR_I2C_WRITE(bus, 0x034F, 0xE0);    //Y_OUT_SIZE[7:0]

	//Clock Setting
	SENSOR_I2C_WRITE(bus, 0x0301, 0x05);    //IVT_PXCK_DIV
	SENSOR_I2C_WRITE(bus, 0x0303, 0x04);    //IVT_SYCK_DIV
	SENSOR_I2C_WRITE(bus, 0x0305, 0x04);    //IVT_PREPLLCK_DIV
	SENSOR_I2C_WRITE(bus, 0x0306, 0x00);    //IVT_PLL_MPY[11:8]
	SENSOR_I2C_WRITE(bus, 0x0307, 0x96);    //IVT_PLL_MPY[7:0]
	SENSOR_I2C_WRITE(bus, 0x0309, 0x0C);    //IOP_PXCK_DIV
	SENSOR_I2C_WRITE(bus, 0x030B, 0x02);    //IOP_SYCK_DIV
	SENSOR_I2C_WRITE(bus, 0x030D, 0x04);    //IOP_PREPLLCK_DIV
	SENSOR_I2C_WRITE(bus, 0x030E, 0x00);    //IOP_PLL_MPY[11:8]
	SENSOR_I2C_WRITE(bus, 0x030F, 0x64);    //IOP_PLL_MPY[7:0]
	SENSOR_I2C_WRITE(bus, 0x0310, 0x01);    //PLL_MULT_DRIV
	SENSOR_I2C_WRITE(bus, 0x0820, 0x02);    //REQ_LINK_BIT_RATE_MBPS[31:24]
	SENSOR_I2C_WRITE(bus, 0x0821, 0x58);    //REQ_LINK_BIT_RATE_MBPS[23:16]
	SENSOR_I2C_WRITE(bus, 0x0822, 0x00);    //REQ_LINK_BIT_RATE_MBPS[15:8]
	SENSOR_I2C_WRITE(bus, 0x0823, 0x00);    //REQ_LINK_BIT_RATE_MBPS[7:0]

	//Output Data Select Setting
	SENSOR_I2C_WRITE(bus, 0x3E20, 0x01);    // PDAF_TYPE_SEL
	SENSOR_I2C_WRITE(bus, 0x3E37, 0x00);    // PDAF_CTRL1[7:0]

	//PowerSave Setting
	SENSOR_I2C_WRITE(bus, 0x3F50, 0x00);    //POWER_SAVE_ENABLE
	SENSOR_I2C_WRITE(bus, 0x3F56, 0x00);    //LINE_LENGTH_INCK[15:8]
	SENSOR_I2C_WRITE(bus, 0x3F57, 0x41);    //LINE_LENGTH_INCK[7:0]
#ifdef IMX577_EMBEDDED_DATA_EN
	//Embedded data
	SENSOR_I2C_WRITE(bus, 0xBCF1, 0x02);
#else
	SENSOR_I2C_WRITE(bus, 0xBCF1, 0x00);
#endif

	//Other Setting
	SENSOR_I2C_WRITE(bus, 0x3C0A, 0x73);
	SENSOR_I2C_WRITE(bus, 0x3C0B, 0x64);
	SENSOR_I2C_WRITE(bus, 0x3C0C, 0x5F);
	SENSOR_I2C_WRITE(bus, 0x3C0D, 0x07);
	SENSOR_I2C_WRITE(bus, 0x3C0E, 0xFF);
	SENSOR_I2C_WRITE(bus, 0x3C0F, 0x00);
	SENSOR_I2C_WRITE(bus, 0x3C10, 0x00);
	SENSOR_I2C_WRITE(bus, 0x3C11, 0x02);
	SENSOR_I2C_WRITE(bus, 0x3C12, 0x00);
	SENSOR_I2C_WRITE(bus, 0x3C13, 0x03);
	SENSOR_I2C_WRITE(bus, 0x3C14, 0x00);
	SENSOR_I2C_WRITE(bus, 0x3C15, 0x00);
	SENSOR_I2C_WRITE(bus, 0x3C16, 0x0C);
	SENSOR_I2C_WRITE(bus, 0x3C17, 0x0C);
	SENSOR_I2C_WRITE(bus, 0x3C18, 0x0C);
	SENSOR_I2C_WRITE(bus, 0x3C19, 0x0A);
	SENSOR_I2C_WRITE(bus, 0x3C1A, 0x0A);
	SENSOR_I2C_WRITE(bus, 0x3C1B, 0x0A);
	SENSOR_I2C_WRITE(bus, 0x3C1C, 0x00);
	SENSOR_I2C_WRITE(bus, 0x3C1D, 0x00);
	SENSOR_I2C_WRITE(bus, 0x3C1E, 0x00);
	SENSOR_I2C_WRITE(bus, 0x3C1F, 0x00);
	SENSOR_I2C_WRITE(bus, 0x3C20, 0x00);
	SENSOR_I2C_WRITE(bus, 0x3C21, 0x00);
	SENSOR_I2C_WRITE(bus, 0x3C22, 0x3F);
	SENSOR_I2C_WRITE(bus, 0x3C23, 0x0A);
	SENSOR_I2C_WRITE(bus, 0x3E35, 0x01);    //SPC control
	SENSOR_I2C_WRITE(bus, 0x3F4A, 0x03);
	SENSOR_I2C_WRITE(bus, 0x3F4B, 0xBF);
	SENSOR_I2C_WRITE(bus, 0x3F26, 0x00);
		//Integration Time Setting.
	SENSOR_I2C_WRITE(bus, 0x0202, 0x07);
	SENSOR_I2C_WRITE(bus, 0x0203, 0x2A);
	//Gain Setting
	SENSOR_I2C_WRITE(bus, 0x0204, 0x00);    //ANA_GAIN_GLOBAL[9:8]
	SENSOR_I2C_WRITE(bus, 0x0205, 0x00);    //ANA_GAIN_GLOBAL[7:0]
	SENSOR_I2C_WRITE(bus, 0x020E, 0x01);    //DIG_GAIN_GR[15:8]
	SENSOR_I2C_WRITE(bus, 0x020F, 0x00);    //DIG_GAIN_GR[7:0]
	SENSOR_I2C_WRITE(bus, 0x0210, 0x01);    //DIG_GAIN_R[15:8]
	SENSOR_I2C_WRITE(bus, 0x0211, 0x00);    //DIG_GAIN_R[7:0]
	SENSOR_I2C_WRITE(bus, 0x0212, 0x01);    //DIG_GAIN_B[15:8]
	SENSOR_I2C_WRITE(bus, 0x0213, 0x00);    //DIG_GAIN_B[7:0]
	SENSOR_I2C_WRITE(bus, 0x0214, 0x01);    //DIG_GAIN_GB[15:8]
	SENSOR_I2C_WRITE(bus, 0x0215, 0x00);    //DIG_GAIN_GB[7:0]

	//Streaming Setting
	SENSOR_I2C_WRITE(bus, 0x0100, 0x01);
	ISP_PR_INFO("success\n");
	return RET_SUCCESS;
}

static int res_7_config_imx577_hdr(struct sensor_module_t *module)
{
	unsigned char bus = module->hw->sensor_i2c_index;

	ISP_PR_INFO("device id[%d]\n", module->id);
	config_common1_imx577(module);

	//MIPI setting
	SENSOR_I2C_WRITE(bus, 0x0112, 0x0A);    //CSI_DT_FMT_H
	SENSOR_I2C_WRITE(bus, 0x0113, 0x0A);    //CSI_DT_FMT_L
	SENSOR_I2C_WRITE(bus, 0x0114, 0x01);    //CSI_LANE_MODE

	//Frame Horizontal Clock Count
	SENSOR_I2C_WRITE(bus, 0x0342, 0x3A);    //LINE_LENGTH_PCK[15:8]
	SENSOR_I2C_WRITE(bus, 0x0343, 0x98);    //LINE_LENGTH_PCK[7:0]

	//Frame Vertical Clock Count
	SENSOR_I2C_WRITE(bus, 0x0340, 0x0C);    //FRM_LENGTH_LINES[15:8]
	SENSOR_I2C_WRITE(bus, 0x0341, 0x1E);    //FRM_LENGTH_LINES[7:0]
	SENSOR_I2C_WRITE(bus, 0x3210, 0x00);    //FLL_LSHIFT

	//Visible Size
	SENSOR_I2C_WRITE(bus, 0x0344, 0x00);    //X_ADD_STA[12:8]
	SENSOR_I2C_WRITE(bus, 0x0345, 0x00);    //X_ADD_STA[7:0]
	SENSOR_I2C_WRITE(bus, 0x0346, 0x00);    //Y_ADD_STA[12:8]
	SENSOR_I2C_WRITE(bus, 0x0347, 0x00);    //Y_ADD_STA[7:0]
	SENSOR_I2C_WRITE(bus, 0x0348, 0x0F);    //X_ADD_END[12:8]
	SENSOR_I2C_WRITE(bus, 0x0349, 0xD7);    //X_ADD_END[7:0]
	SENSOR_I2C_WRITE(bus, 0x034A, 0x0B);    //Y_ADD_END[12:8]
	SENSOR_I2C_WRITE(bus, 0x034B, 0xDF);    //Y_ADD_END[7:0]

	//Mode Setting
	SENSOR_I2C_WRITE(bus, 0x00E3, 0x00);    //DOL_EN --> Disable DOL-HDR
	SENSOR_I2C_WRITE(bus, 0x00E4, 0x00);    //DOL_NUM
	SENSOR_I2C_WRITE(bus, 0x00E5, 0x01);    //DOL_OUTPUT_FMT
	SENSOR_I2C_WRITE(bus, 0x00FC, 0x0A);    //DOL_CSI_DT_FMT_H_2ND
	SENSOR_I2C_WRITE(bus, 0x00FD, 0x0A);    //DOL_CSI_DT_FMT_L_2ND
	SENSOR_I2C_WRITE(bus, 0x00FE, 0x0A);    //DOL_CSI_DT_FMT_H_3RD
	SENSOR_I2C_WRITE(bus, 0x00FF, 0x0A);    //DOL_CSI_DT_FMT_L_3RD
	SENSOR_I2C_WRITE(bus, 0xE013, 0x00);

	// HDR_MODE --> Disable SME-HDR_MODE
	SENSOR_I2C_WRITE(bus, 0x0220, 0x01);
	SENSOR_I2C_WRITE(bus, 0x0221, 0x11);
	SENSOR_I2C_WRITE(bus, 0x0381, 0x01);    //X_EVN_INC
	SENSOR_I2C_WRITE(bus, 0x0383, 0x01);    //X_ODD_INC
	SENSOR_I2C_WRITE(bus, 0x0385, 0x01);    //Y_EVN_INC
	SENSOR_I2C_WRITE(bus, 0x0387, 0x01);    //Y_ODD_INC
	SENSOR_I2C_WRITE(bus, 0x0900, 0x00);    //BINNING_MODE
	//[7:4] BINNING_TYPE_H, [3:0] BINNING_TYPE_V
	SENSOR_I2C_WRITE(bus, 0x0901, 0x11);
	SENSOR_I2C_WRITE(bus, 0x0902, 0x00);    //BINNING_WEIGHTING
	SENSOR_I2C_WRITE(bus, 0x3140, 0x02);    //HDR_FNCSEL
	SENSOR_I2C_WRITE(bus, 0x3241, 0x11);
	SENSOR_I2C_WRITE(bus, 0x3250, 0x03);    //SUB_WEIGHTING
	SENSOR_I2C_WRITE(bus, 0x3E10, 0x00);    //need to confirm
	SENSOR_I2C_WRITE(bus, 0x3E11, 0x00);    //need to confirm
	SENSOR_I2C_WRITE(bus, 0x3F0D, 0x00);    //ADBIT_MODE
	SENSOR_I2C_WRITE(bus, 0x3F42, 0x00);    //BINNING_TYPE_EXT_EN
	SENSOR_I2C_WRITE(bus, 0x3F43, 0x00);    //BINNING_TYPE_H_EXT

	//Digital Crop & Scaling
	SENSOR_I2C_WRITE(bus, 0x0401, 0x00);    //SCALE_MODE
	SENSOR_I2C_WRITE(bus, 0x0404, 0x00);    //SCALE_M[8]
	SENSOR_I2C_WRITE(bus, 0x0405, 0x10);    //SCALE_M[7:0]
	SENSOR_I2C_WRITE(bus, 0x0408, 0x00);    //DIG_CROP_X_OFFSET[12:8]
	SENSOR_I2C_WRITE(bus, 0x0409, 0x00);    //DIG_CROP_X_OFFSET[7:0]
	SENSOR_I2C_WRITE(bus, 0x040A, 0x00);    //DIG_CROP_Y_OFFSET[12:8]
	SENSOR_I2C_WRITE(bus, 0x040B, 0x00);    //DIG_CROP_Y_OFFSET[7:0]
	SENSOR_I2C_WRITE(bus, 0x040C, 0x0F);    //DIG_CROP_IMAGE_WIDTH[12:8]
	SENSOR_I2C_WRITE(bus, 0x040D, 0xD8);    //DIG_CROP_IMAGE_WIDTH[7:0]
	SENSOR_I2C_WRITE(bus, 0x040E, 0x0B);    //DIG_CROP_IMAGE_HEIGHT[12:8]
	SENSOR_I2C_WRITE(bus, 0x040F, 0xE0);    //DIG_CROP_IMAGE_HEIGHT[7:0]

	//Output Crop
	SENSOR_I2C_WRITE(bus, 0x034C, 0x0F);    //X_OUT_SIZE[12:8]
	SENSOR_I2C_WRITE(bus, 0x034D, 0xD8);    //X_OUT_SIZE[7:0]
	SENSOR_I2C_WRITE(bus, 0x034E, 0x0B);    //Y_OUT_SIZE[15:8]
	SENSOR_I2C_WRITE(bus, 0x034F, 0xE0);    //Y_OUT_SIZE[7:0]

	//Clock Setting
	SENSOR_I2C_WRITE(bus, 0x0301, 0x05);    //IVT_PXCK_DIV
	SENSOR_I2C_WRITE(bus, 0x0303, 0x04);    //IVT_SYCK_DIV
	SENSOR_I2C_WRITE(bus, 0x0305, 0x04);    //IVT_PREPLLCK_DIV
	SENSOR_I2C_WRITE(bus, 0x0306, 0x00);    //IVT_PLL_MPY[11:8]
	SENSOR_I2C_WRITE(bus, 0x0307, 0x96);    //IVT_PLL_MPY[7:0]
	SENSOR_I2C_WRITE(bus, 0x0309, 0x0C);    //IOP_PXCK_DIV
	SENSOR_I2C_WRITE(bus, 0x030B, 0x02);    //IOP_SYCK_DIV
	SENSOR_I2C_WRITE(bus, 0x030D, 0x04);    //IOP_PREPLLCK_DIV
	SENSOR_I2C_WRITE(bus, 0x030E, 0x00);    //IOP_PLL_MPY[11:8]
	SENSOR_I2C_WRITE(bus, 0x030F, 0x64);    //IOP_PLL_MPY[7:0]
	SENSOR_I2C_WRITE(bus, 0x0310, 0x01);    //PLL_MULT_DRIV
	SENSOR_I2C_WRITE(bus, 0x0820, 0x02);    //REQ_LINK_BIT_RATE_MBPS[31:24]
	SENSOR_I2C_WRITE(bus, 0x0821, 0x58);    //REQ_LINK_BIT_RATE_MBPS[23:16]
	SENSOR_I2C_WRITE(bus, 0x0822, 0x00);    //REQ_LINK_BIT_RATE_MBPS[15:8]
	SENSOR_I2C_WRITE(bus, 0x0823, 0x00);    //REQ_LINK_BIT_RATE_MBPS[7:0]

	//Output Data Select Setting
	SENSOR_I2C_WRITE(bus, 0x3E20, 0x01);    // PDAF_TYPE_SEL
	SENSOR_I2C_WRITE(bus, 0x3E37, 0x00);    // PDAF_CTRL1[7:0]

	//PowerSave Setting
	SENSOR_I2C_WRITE(bus, 0x3F50, 0x00);    //POWER_SAVE_ENABLE
	SENSOR_I2C_WRITE(bus, 0x3F56, 0x00);    //LINE_LENGTH_INCK[15:8]
	SENSOR_I2C_WRITE(bus, 0x3F57, 0x41);    //LINE_LENGTH_INCK[7:0]

#ifdef IMX577_EMBEDDED_DATA_EN
	//Embedded data
	SENSOR_I2C_WRITE(bus, 0xBCF1, 0x02);
#else
	SENSOR_I2C_WRITE(bus, 0xBCF1, 0x00);
#endif

	//Other Setting
	SENSOR_I2C_WRITE(bus, 0x3C0A, 0x5A);
	SENSOR_I2C_WRITE(bus, 0x3C0B, 0x55);
	SENSOR_I2C_WRITE(bus, 0x3C0C, 0x28);
	SENSOR_I2C_WRITE(bus, 0x3C0D, 0x07);
	SENSOR_I2C_WRITE(bus, 0x3C0E, 0xFF);
	SENSOR_I2C_WRITE(bus, 0x3C0F, 0x00);
	SENSOR_I2C_WRITE(bus, 0x3C10, 0x00);
	SENSOR_I2C_WRITE(bus, 0x3C11, 0x02);
	SENSOR_I2C_WRITE(bus, 0x3C12, 0x00);
	SENSOR_I2C_WRITE(bus, 0x3C13, 0x03);
	SENSOR_I2C_WRITE(bus, 0x3C14, 0x00);
	SENSOR_I2C_WRITE(bus, 0x3C15, 0x00);
	SENSOR_I2C_WRITE(bus, 0x3C16, 0x0C);
	SENSOR_I2C_WRITE(bus, 0x3C17, 0x0C);
	SENSOR_I2C_WRITE(bus, 0x3C18, 0x0C);
	SENSOR_I2C_WRITE(bus, 0x3C19, 0x0A);
	SENSOR_I2C_WRITE(bus, 0x3C1A, 0x0A);
	SENSOR_I2C_WRITE(bus, 0x3C1B, 0x0A);
	SENSOR_I2C_WRITE(bus, 0x3C1C, 0x00);
	SENSOR_I2C_WRITE(bus, 0x3C1D, 0x00);
	SENSOR_I2C_WRITE(bus, 0x3C1E, 0x00);
	SENSOR_I2C_WRITE(bus, 0x3C1F, 0x00);
	SENSOR_I2C_WRITE(bus, 0x3C20, 0x00);
	SENSOR_I2C_WRITE(bus, 0x3C21, 0x00);
	SENSOR_I2C_WRITE(bus, 0x3C22, 0x3F);
	SENSOR_I2C_WRITE(bus, 0x3C23, 0x0A);
	SENSOR_I2C_WRITE(bus, 0x3E35, 0x01);    //SPC control
	SENSOR_I2C_WRITE(bus, 0x3F4A, 0x03);
	SENSOR_I2C_WRITE(bus, 0x3F4B, 0xBF);
	SENSOR_I2C_WRITE(bus, 0x3F26, 0x00);

	//Integration Time Setting
	SENSOR_I2C_WRITE(bus, 0x0224, 0x0C); // ST_COARSE_INTEG_TIME[15:8]
	SENSOR_I2C_WRITE(bus, 0x0225, 0xF4); // ST_COARSE_INTEG_TIME[7:0]

	//Gain Setting
	SENSOR_I2C_WRITE(bus, 0x0204, 0x00);    //ANA_GAIN_GLOBAL[9:8]
	SENSOR_I2C_WRITE(bus, 0x0205, 0x00);    //ANA_GAIN_GLOBAL[7:0]
	SENSOR_I2C_WRITE(bus, 0x020E, 0x01);    //DIG_GAIN_GR[15:8]
	SENSOR_I2C_WRITE(bus, 0x020F, 0x00);    //DIG_GAIN_GR[7:0]
	SENSOR_I2C_WRITE(bus, 0x0210, 0x01);    //DIG_GAIN_R[15:8]
	SENSOR_I2C_WRITE(bus, 0x0211, 0x00);    //DIG_GAIN_R[7:0]
	SENSOR_I2C_WRITE(bus, 0x0212, 0x01);    //DIG_GAIN_B[15:8]
	SENSOR_I2C_WRITE(bus, 0x0213, 0x00);    //DIG_GAIN_B[7:0]
	SENSOR_I2C_WRITE(bus, 0x0214, 0x01);    //DIG_GAIN_GB[15:8]
	SENSOR_I2C_WRITE(bus, 0x0215, 0x00);    //DIG_GAIN_GB[7:0]
	//Streaming Setting
	SENSOR_I2C_WRITE(bus, 0x0100, 0x01);

	ISP_PR_INFO("res_7_config_imx577_HDR, success\n");
	return RET_SUCCESS;
}

static int res_8_config_imx577(struct sensor_module_t *module)
{
	unsigned char bus = module->hw->sensor_i2c_index;

	ISP_PR_INFO("device id[%d]\n", module->id);
	config_common1_imx577(module);
	//MIPI setting
	SENSOR_I2C_WRITE(bus, 0x0112, 0x0A);    //CSI_DT_FMT_H
	SENSOR_I2C_WRITE(bus, 0x0113, 0x0A);    //CSI_DT_FMT_L
	SENSOR_I2C_WRITE(bus, 0x0114, 0x01);    //CSI_LANE_MODE

	//Frame Horizontal Clock Count
	SENSOR_I2C_WRITE(bus, 0x0342, 0x3A);    //LINE_LENGTH_PCK[15:8]
	SENSOR_I2C_WRITE(bus, 0x0343, 0x98);    //LINE_LENGTH_PCK[7:0]

	//Frame Vertical Clock Count
	SENSOR_I2C_WRITE(bus, 0x0340, 0x0C);    //FRM_LENGTH_LINES[15:8]
	SENSOR_I2C_WRITE(bus, 0x0341, 0x1E);    //FRM_LENGTH_LINES[7:0]
	SENSOR_I2C_WRITE(bus, 0x3210, 0x00);    //FLL_LSHIFT

	//Visible Size
	SENSOR_I2C_WRITE(bus, 0x0344, 0x00);    //X_ADD_STA[12:8]
	SENSOR_I2C_WRITE(bus, 0x0345, 0x00);    //X_ADD_STA[7:0]
	SENSOR_I2C_WRITE(bus, 0x0346, 0x00);    //Y_ADD_STA[12:8]
	SENSOR_I2C_WRITE(bus, 0x0347, 0x00);    //Y_ADD_STA[7:0]
	SENSOR_I2C_WRITE(bus, 0x0348, 0x0F);    //X_ADD_END[12:8]
	SENSOR_I2C_WRITE(bus, 0x0349, 0xD7);    //X_ADD_END[7:0]
	SENSOR_I2C_WRITE(bus, 0x034A, 0x0B);    //Y_ADD_END[12:8]
	SENSOR_I2C_WRITE(bus, 0x034B, 0xDF);    //Y_ADD_END[7:0]

	//Mode Setting
	SENSOR_I2C_WRITE(bus, 0x00E3, 0x00);    //DOL_EN --> Disable DOL-HDR
	SENSOR_I2C_WRITE(bus, 0x00E4, 0x00);    //DOL_NUM
	SENSOR_I2C_WRITE(bus, 0x00E5, 0x01);    //DOL_OUTPUT_FMT
	SENSOR_I2C_WRITE(bus, 0x00FC, 0x0A);    //DOL_CSI_DT_FMT_H_2ND
	SENSOR_I2C_WRITE(bus, 0x00FD, 0x0A);    //DOL_CSI_DT_FMT_L_2ND
	SENSOR_I2C_WRITE(bus, 0x00FE, 0x0A);    //DOL_CSI_DT_FMT_H_3RD
	SENSOR_I2C_WRITE(bus, 0x00FF, 0x0A);    //DOL_CSI_DT_FMT_L_3RD
	SENSOR_I2C_WRITE(bus, 0xE013, 0x00);
	// HDR_MODE --> Disable SME-HDR_MODE
	SENSOR_I2C_WRITE(bus, 0x0220, 0x00);
	SENSOR_I2C_WRITE(bus, 0x0221, 0x11);
	SENSOR_I2C_WRITE(bus, 0x0381, 0x01);    //X_EVN_INC
	SENSOR_I2C_WRITE(bus, 0x0383, 0x01);    //X_ODD_INC
	SENSOR_I2C_WRITE(bus, 0x0385, 0x01);    //Y_EVN_INC
	SENSOR_I2C_WRITE(bus, 0x0387, 0x01);    //Y_ODD_INC
	SENSOR_I2C_WRITE(bus, 0x0900, 0x01);    //BINNING_MODE
	//[7:4] BINNING_TYPE_H, [3:0] BINNING_TYPE_V
	SENSOR_I2C_WRITE(bus, 0x0901, 0x12);
	SENSOR_I2C_WRITE(bus, 0x0902, 0x00);    //BINNING_WEIGHTING
	SENSOR_I2C_WRITE(bus, 0x3140, 0x02);    //HDR_FNCSEL
	SENSOR_I2C_WRITE(bus, 0x3241, 0x11);
	//SENSOR_I2C_WRITE(bus, 0x3242, 0x01);
	// DUBE_SUM_WEIGHT_MODE  need to confirm
	SENSOR_I2C_WRITE(bus, 0x3250, 0x00);    //SUB_WEIGHTING
	SENSOR_I2C_WRITE(bus, 0x3E10, 0x00);    //need to confirm
	SENSOR_I2C_WRITE(bus, 0x3E11, 0x00);    //need to confirm
	SENSOR_I2C_WRITE(bus, 0x3F0D, 0x00);    //ADBIT_MODE
	//SENSOR_I2C_WRITE(bus, 0x3F40, 0x00);    // BINNING_PRIORITY_V
	//SENSOR_I2C_WRITE(bus, 0x3F41, 0x00);    // BINNING_PRIORITY_H
	SENSOR_I2C_WRITE(bus, 0x3F42, 0x00);    //BINNING_TYPE_EXT_EN
	SENSOR_I2C_WRITE(bus, 0x3F43, 0x00);    //BINNING_TYPE_H_EXT

	//Digital Crop & Scaling
	SENSOR_I2C_WRITE(bus, 0x0401, 0x00);    //SCALE_MODE
	SENSOR_I2C_WRITE(bus, 0x0404, 0x00);    //SCALE_M[8]
	SENSOR_I2C_WRITE(bus, 0x0405, 0x10);    //SCALE_M[7:0]
	SENSOR_I2C_WRITE(bus, 0x0408, 0x00);    //DIG_CROP_X_OFFSET[12:8]
	SENSOR_I2C_WRITE(bus, 0x0409, 0x00);    //DIG_CROP_X_OFFSET[7:0]
	SENSOR_I2C_WRITE(bus, 0x040A, 0x00);    //DIG_CROP_Y_OFFSET[12:8]
	SENSOR_I2C_WRITE(bus, 0x040B, 0x00);    //DIG_CROP_Y_OFFSET[7:0]
	SENSOR_I2C_WRITE(bus, 0x040C, 0x0F);    //DIG_CROP_IMAGE_WIDTH[12:8]
	SENSOR_I2C_WRITE(bus, 0x040D, 0xD8);    //DIG_CROP_IMAGE_WIDTH[7:0]
	SENSOR_I2C_WRITE(bus, 0x040E, 0x05);    //DIG_CROP_IMAGE_HEIGHT[12:8]
	SENSOR_I2C_WRITE(bus, 0x040F, 0xF0);    //DIG_CROP_IMAGE_HEIGHT[7:0]

	//Output Crop
	SENSOR_I2C_WRITE(bus, 0x034C, 0x0F);    //X_OUT_SIZE[12:8]
	SENSOR_I2C_WRITE(bus, 0x034D, 0xD8);    //X_OUT_SIZE[7:0]
	SENSOR_I2C_WRITE(bus, 0x034E, 0x05);    //Y_OUT_SIZE[15:8]
	SENSOR_I2C_WRITE(bus, 0x034F, 0xF0);    //Y_OUT_SIZE[7:0]

	//Clock Setting
	SENSOR_I2C_WRITE(bus, 0x0301, 0x05);    //IVT_PXCK_DIV
	SENSOR_I2C_WRITE(bus, 0x0303, 0x04);    //IVT_SYCK_DIV
	SENSOR_I2C_WRITE(bus, 0x0305, 0x04);    //IVT_PREPLLCK_DIV
	SENSOR_I2C_WRITE(bus, 0x0306, 0x00);    //IVT_PLL_MPY[11:8]
	SENSOR_I2C_WRITE(bus, 0x0307, 0x96);    //IVT_PLL_MPY[7:0]
	SENSOR_I2C_WRITE(bus, 0x0309, 0x0C);    //IOP_PXCK_DIV
	SENSOR_I2C_WRITE(bus, 0x030B, 0x02);    //IOP_SYCK_DIV
	SENSOR_I2C_WRITE(bus, 0x030D, 0x04);    //IOP_PREPLLCK_DIV
	SENSOR_I2C_WRITE(bus, 0x030E, 0x00);    //IOP_PLL_MPY[11:8]
	SENSOR_I2C_WRITE(bus, 0x030F, 0x64);    //IOP_PLL_MPY[7:0]
	SENSOR_I2C_WRITE(bus, 0x0310, 0x01);    //PLL_MULT_DRIV
	SENSOR_I2C_WRITE(bus, 0x0820, 0x02);    //REQ_LINK_BIT_RATE_MBPS[31:24]
	SENSOR_I2C_WRITE(bus, 0x0821, 0x58);    //REQ_LINK_BIT_RATE_MBPS[23:16]
	SENSOR_I2C_WRITE(bus, 0x0822, 0x00);    //REQ_LINK_BIT_RATE_MBPS[15:8]
	SENSOR_I2C_WRITE(bus, 0x0823, 0x00);    //REQ_LINK_BIT_RATE_MBPS[7:0]

	//Output Data Select Setting
	SENSOR_I2C_WRITE(bus, 0x3E20, 0x01);    // PDAF_TYPE_SEL
	SENSOR_I2C_WRITE(bus, 0x3E37, 0x00);    // PDAF_CTRL1[7:0]

	//PowerSave Setting
	SENSOR_I2C_WRITE(bus, 0x3F50, 0x00);    //POWER_SAVE_ENABLE
	SENSOR_I2C_WRITE(bus, 0x3F56, 0x00);    //LINE_LENGTH_INCK[15:8]
	SENSOR_I2C_WRITE(bus, 0x3F57, 0x41);    //LINE_LENGTH_INCK[7:0]
#ifdef IMX577_EMBEDDED_DATA_EN
	//Embedded data
	SENSOR_I2C_WRITE(bus, 0xBCF1, 0x02);
#else
	SENSOR_I2C_WRITE(bus, 0xBCF1, 0x00);
#endif

	//Other Setting
	SENSOR_I2C_WRITE(bus, 0x3C0A, 0x73);
	SENSOR_I2C_WRITE(bus, 0x3C0B, 0x64);
	SENSOR_I2C_WRITE(bus, 0x3C0C, 0x5F);
	SENSOR_I2C_WRITE(bus, 0x3C0D, 0x07);
	SENSOR_I2C_WRITE(bus, 0x3C0E, 0xFF);
	SENSOR_I2C_WRITE(bus, 0x3C0F, 0x00);
	SENSOR_I2C_WRITE(bus, 0x3C10, 0x00);
	SENSOR_I2C_WRITE(bus, 0x3C11, 0x02);
	SENSOR_I2C_WRITE(bus, 0x3C12, 0x00);
	SENSOR_I2C_WRITE(bus, 0x3C13, 0x03);
	SENSOR_I2C_WRITE(bus, 0x3C14, 0x00);
	SENSOR_I2C_WRITE(bus, 0x3C15, 0x04);
	SENSOR_I2C_WRITE(bus, 0x3C16, 0x15);
	SENSOR_I2C_WRITE(bus, 0x3C17, 0x15);
	SENSOR_I2C_WRITE(bus, 0x3C18, 0x15);
	SENSOR_I2C_WRITE(bus, 0x3C19, 0x15);
	SENSOR_I2C_WRITE(bus, 0x3C1A, 0x15);
	SENSOR_I2C_WRITE(bus, 0x3C1B, 0x15);
	SENSOR_I2C_WRITE(bus, 0x3C1C, 0x06);
	SENSOR_I2C_WRITE(bus, 0x3C1D, 0x06);
	SENSOR_I2C_WRITE(bus, 0x3C1E, 0x06);
	SENSOR_I2C_WRITE(bus, 0x3C1F, 0x06);
	SENSOR_I2C_WRITE(bus, 0x3C20, 0x06);
	SENSOR_I2C_WRITE(bus, 0x3C21, 0x06);
	SENSOR_I2C_WRITE(bus, 0x3C22, 0x3F);
	SENSOR_I2C_WRITE(bus, 0x3C23, 0x0A);
	SENSOR_I2C_WRITE(bus, 0x3E35, 0x01);    //SPC control
	//Additional defect correction control  need to confirm
	//SENSOR_I2C_WRITE(bus, 0x3E36, 0x01);
	SENSOR_I2C_WRITE(bus, 0x3F4A, 0x03);
	SENSOR_I2C_WRITE(bus, 0x3F4B, 0xBF);
	SENSOR_I2C_WRITE(bus, 0x3F26, 0x01);
	//Integration Time Setting.
	SENSOR_I2C_WRITE(bus, 0x0202, 0x07);
	SENSOR_I2C_WRITE(bus, 0x0203, 0x2A);

	//Gain Setting
	SENSOR_I2C_WRITE(bus, 0x0204, 0x00);    //ANA_GAIN_GLOBAL[9:8]
	SENSOR_I2C_WRITE(bus, 0x0205, 0x00);    //ANA_GAIN_GLOBAL[7:0]

	SENSOR_I2C_WRITE(bus, 0x020E, 0x01);    //DIG_GAIN_GR[15:8]
	SENSOR_I2C_WRITE(bus, 0x020F, 0x00);    //DIG_GAIN_GR[7:0]
	SENSOR_I2C_WRITE(bus, 0x0210, 0x01);    //DIG_GAIN_R[15:8]
	SENSOR_I2C_WRITE(bus, 0x0211, 0x00);    //DIG_GAIN_R[7:0]
	SENSOR_I2C_WRITE(bus, 0x0212, 0x01);    //DIG_GAIN_B[15:8]
	SENSOR_I2C_WRITE(bus, 0x0213, 0x00);    //DIG_GAIN_B[7:0]
	SENSOR_I2C_WRITE(bus, 0x0214, 0x01);    //DIG_GAIN_GB[15:8]
	SENSOR_I2C_WRITE(bus, 0x0215, 0x00);    //DIG_GAIN_GB[7:0]
	//Streaming Setting
	SENSOR_I2C_WRITE(bus, 0x0100, 0x01);

	ISP_PR_INFO("success\n");
	return RET_SUCCESS;
}

static int res_8_config_imx577_hdr(struct sensor_module_t *module)
{
	unsigned char bus = module->hw->sensor_i2c_index;

	ISP_PR_INFO("device id[%d]\n", module->id);
	config_common1_imx577(module);

	//MIPI setting
	SENSOR_I2C_WRITE(bus, 0x0112, 0x0A);    //CSI_DT_FMT_H
	SENSOR_I2C_WRITE(bus, 0x0113, 0x0A);    //CSI_DT_FMT_L
	SENSOR_I2C_WRITE(bus, 0x0114, 0x01);    //CSI_LANE_MODE

	//Frame Horizontal Clock Count
	SENSOR_I2C_WRITE(bus, 0x0342, 0x3A);    //LINE_LENGTH_PCK[15:8]
	SENSOR_I2C_WRITE(bus, 0x0343, 0x98);    //LINE_LENGTH_PCK[7:0]

	//Frame Vertical Clock Count
	SENSOR_I2C_WRITE(bus, 0x0340, 0x0C);    //FRM_LENGTH_LINES[15:8]
	SENSOR_I2C_WRITE(bus, 0x0341, 0x1E);    //FRM_LENGTH_LINES[7:0]
	SENSOR_I2C_WRITE(bus, 0x3210, 0x00);    //FLL_LSHIFT

	//Visible Size
	SENSOR_I2C_WRITE(bus, 0x0344, 0x00);    //X_ADD_STA[12:8]
	SENSOR_I2C_WRITE(bus, 0x0345, 0x00);    //X_ADD_STA[7:0]
	SENSOR_I2C_WRITE(bus, 0x0346, 0x00);    //Y_ADD_STA[12:8]
	SENSOR_I2C_WRITE(bus, 0x0347, 0x00);    //Y_ADD_STA[7:0]
	SENSOR_I2C_WRITE(bus, 0x0348, 0x0F);    //X_ADD_END[12:8]
	SENSOR_I2C_WRITE(bus, 0x0349, 0xD7);    //X_ADD_END[7:0]
	SENSOR_I2C_WRITE(bus, 0x034A, 0x0B);    //Y_ADD_END[12:8]
	SENSOR_I2C_WRITE(bus, 0x034B, 0xDF);    //Y_ADD_END[7:0]

	//Mode Setting
	SENSOR_I2C_WRITE(bus, 0x00E3, 0x00);    //DOL_EN --> Disable DOL-HDR
	SENSOR_I2C_WRITE(bus, 0x00E4, 0x00);    //DOL_NUM
	SENSOR_I2C_WRITE(bus, 0x00E5, 0x01);    //DOL_OUTPUT_FMT
	SENSOR_I2C_WRITE(bus, 0x00FC, 0x0A);    //DOL_CSI_DT_FMT_H_2ND
	SENSOR_I2C_WRITE(bus, 0x00FD, 0x0A);    //DOL_CSI_DT_FMT_L_2ND
	SENSOR_I2C_WRITE(bus, 0x00FE, 0x0A);    //DOL_CSI_DT_FMT_H_3RD
	SENSOR_I2C_WRITE(bus, 0x00FF, 0x0A);    //DOL_CSI_DT_FMT_L_3RD
	SENSOR_I2C_WRITE(bus, 0xE013, 0x00);
	// HDR_MODE --> Disable SME-HDR_MODE
	SENSOR_I2C_WRITE(bus, 0x0220, 0x03);
	SENSOR_I2C_WRITE(bus, 0x0221, 0x12);
	SENSOR_I2C_WRITE(bus, 0x0381, 0x01);    //X_EVN_INC
	SENSOR_I2C_WRITE(bus, 0x0383, 0x01);    //X_ODD_INC
	SENSOR_I2C_WRITE(bus, 0x0385, 0x01);    //Y_EVN_INC
	SENSOR_I2C_WRITE(bus, 0x0387, 0x01);    //Y_ODD_INC
	SENSOR_I2C_WRITE(bus, 0x0900, 0x01);    //BINNING_MODE
	//[7:4] BINNING_TYPE_H, [3:0] BINNING_TYPE_V
	SENSOR_I2C_WRITE(bus, 0x0901, 0x12);
	SENSOR_I2C_WRITE(bus, 0x0902, 0x00);    //BINNING_WEIGHTING
	SENSOR_I2C_WRITE(bus, 0x3140, 0x02);    //HDR_FNCSEL
	SENSOR_I2C_WRITE(bus, 0x3241, 0x11);
	// DUBE_SUM_WEIGHT_MODE  need to confirm
	//SENSOR_I2C_WRITE(bus, 0x3242, 0x01);
	SENSOR_I2C_WRITE(bus, 0x3250, 0x00);    //SUB_WEIGHTING
	SENSOR_I2C_WRITE(bus, 0x3E10, 0x00);    //need to confirm
	SENSOR_I2C_WRITE(bus, 0x3E11, 0x00);    //need to confirm
	SENSOR_I2C_WRITE(bus, 0x3F0D, 0x00);    //ADBIT_MODE
	//SENSOR_I2C_WRITE(bus, 0x3F40, 0x00);    // BINNING_PRIORITY_V
	//SENSOR_I2C_WRITE(bus, 0x3F41, 0x00);    // BINNING_PRIORITY_H
	SENSOR_I2C_WRITE(bus, 0x3F42, 0x00);    //BINNING_TYPE_EXT_EN
	SENSOR_I2C_WRITE(bus, 0x3F43, 0x00);    //BINNING_TYPE_H_EXT

	//Digital Crop & Scaling
	SENSOR_I2C_WRITE(bus, 0x0401, 0x00);    //SCALE_MODE
	SENSOR_I2C_WRITE(bus, 0x0404, 0x00);    //SCALE_M[8]
	SENSOR_I2C_WRITE(bus, 0x0405, 0x10);    //SCALE_M[7:0]
	SENSOR_I2C_WRITE(bus, 0x0408, 0x00);    //DIG_CROP_X_OFFSET[12:8]
	SENSOR_I2C_WRITE(bus, 0x0409, 0x00);    //DIG_CROP_X_OFFSET[7:0]
	SENSOR_I2C_WRITE(bus, 0x040A, 0x00);    //DIG_CROP_Y_OFFSET[12:8]
	SENSOR_I2C_WRITE(bus, 0x040B, 0x00);    //DIG_CROP_Y_OFFSET[7:0]
	SENSOR_I2C_WRITE(bus, 0x040C, 0x0F);    //DIG_CROP_IMAGE_WIDTH[12:8]
	SENSOR_I2C_WRITE(bus, 0x040D, 0xD8);    //DIG_CROP_IMAGE_WIDTH[7:0]
	SENSOR_I2C_WRITE(bus, 0x040E, 0x05);    //DIG_CROP_IMAGE_HEIGHT[12:8]
	SENSOR_I2C_WRITE(bus, 0x040F, 0xF0);    //DIG_CROP_IMAGE_HEIGHT[7:0]

	//Output Crop
	SENSOR_I2C_WRITE(bus, 0x034C, 0x0F);    //X_OUT_SIZE[12:8]
	SENSOR_I2C_WRITE(bus, 0x034D, 0xD8);    //X_OUT_SIZE[7:0]
	SENSOR_I2C_WRITE(bus, 0x034E, 0x05);    //Y_OUT_SIZE[15:8]
	SENSOR_I2C_WRITE(bus, 0x034F, 0xF0);    //Y_OUT_SIZE[7:0]

	//Clock Setting
	SENSOR_I2C_WRITE(bus, 0x0301, 0x05);    //IVT_PXCK_DIV
	SENSOR_I2C_WRITE(bus, 0x0303, 0x04);    //IVT_SYCK_DIV
	SENSOR_I2C_WRITE(bus, 0x0305, 0x04);    //IVT_PREPLLCK_DIV
	SENSOR_I2C_WRITE(bus, 0x0306, 0x00);    //IVT_PLL_MPY[11:8]
	SENSOR_I2C_WRITE(bus, 0x0307, 0x96);    //IVT_PLL_MPY[7:0]
	SENSOR_I2C_WRITE(bus, 0x0309, 0x0C);    //IOP_PXCK_DIV
	SENSOR_I2C_WRITE(bus, 0x030B, 0x02);    //IOP_SYCK_DIV
	SENSOR_I2C_WRITE(bus, 0x030D, 0x04);    //IOP_PREPLLCK_DIV
	SENSOR_I2C_WRITE(bus, 0x030E, 0x00);    //IOP_PLL_MPY[11:8]
	SENSOR_I2C_WRITE(bus, 0x030F, 0x64);    //IOP_PLL_MPY[7:0]
	SENSOR_I2C_WRITE(bus, 0x0310, 0x01);    //PLL_MULT_DRIV
	SENSOR_I2C_WRITE(bus, 0x0820, 0x02);    //REQ_LINK_BIT_RATE_MBPS[31:24]
	SENSOR_I2C_WRITE(bus, 0x0821, 0x58);    //REQ_LINK_BIT_RATE_MBPS[23:16]
	SENSOR_I2C_WRITE(bus, 0x0822, 0x00);    //REQ_LINK_BIT_RATE_MBPS[15:8]
	SENSOR_I2C_WRITE(bus, 0x0823, 0x00);    //REQ_LINK_BIT_RATE_MBPS[7:0]

	//Output Data Select Setting
	SENSOR_I2C_WRITE(bus, 0x3E20, 0x01);    // PDAF_TYPE_SEL
	SENSOR_I2C_WRITE(bus, 0x3E37, 0x00);    // PDAF_CTRL1[7:0]

	//PowerSave Setting
	SENSOR_I2C_WRITE(bus, 0x3F50, 0x00);    //POWER_SAVE_ENABLE
	SENSOR_I2C_WRITE(bus, 0x3F56, 0x00);    //LINE_LENGTH_INCK[15:8]
	SENSOR_I2C_WRITE(bus, 0x3F57, 0x41);    //LINE_LENGTH_INCK[7:0]
#ifdef IMX577_EMBEDDED_DATA_EN
	//Embedded data
	SENSOR_I2C_WRITE(bus, 0xBCF1, 0x02);
#else
	SENSOR_I2C_WRITE(bus, 0xBCF1, 0x00);
#endif
//Other Setting
	SENSOR_I2C_WRITE(bus, 0x3C0A, 0x5A);
	SENSOR_I2C_WRITE(bus, 0x3C0B, 0x55);
	SENSOR_I2C_WRITE(bus, 0x3C0C, 0x28);
	SENSOR_I2C_WRITE(bus, 0x3C0D, 0x07);
	SENSOR_I2C_WRITE(bus, 0x3C0E, 0xFF);
	SENSOR_I2C_WRITE(bus, 0x3C0F, 0x00);
	SENSOR_I2C_WRITE(bus, 0x3C10, 0x00);
	SENSOR_I2C_WRITE(bus, 0x3C11, 0x02);
	SENSOR_I2C_WRITE(bus, 0x3C12, 0x00);
	SENSOR_I2C_WRITE(bus, 0x3C13, 0x03);
	SENSOR_I2C_WRITE(bus, 0x3C14, 0x00);
	SENSOR_I2C_WRITE(bus, 0x3C15, 0x04);
	SENSOR_I2C_WRITE(bus, 0x3C16, 0x15);
	SENSOR_I2C_WRITE(bus, 0x3C17, 0x15);
	SENSOR_I2C_WRITE(bus, 0x3C18, 0x15);
	SENSOR_I2C_WRITE(bus, 0x3C19, 0x15);
	SENSOR_I2C_WRITE(bus, 0x3C1A, 0x15);
	SENSOR_I2C_WRITE(bus, 0x3C1B, 0x15);
	SENSOR_I2C_WRITE(bus, 0x3C1C, 0x06);
	SENSOR_I2C_WRITE(bus, 0x3C1D, 0x06);
	SENSOR_I2C_WRITE(bus, 0x3C1E, 0x06);
	SENSOR_I2C_WRITE(bus, 0x3C1F, 0x06);
	SENSOR_I2C_WRITE(bus, 0x3C20, 0x06);
	SENSOR_I2C_WRITE(bus, 0x3C21, 0x06);
	SENSOR_I2C_WRITE(bus, 0x3C22, 0x3F);
	SENSOR_I2C_WRITE(bus, 0x3C23, 0x0A);
	SENSOR_I2C_WRITE(bus, 0x3E35, 0x01);//SPC control
	//Additional defect correction control  need to confirm
	//SENSOR_I2C_WRITE(bus, 0x3E36, 0x01);
	SENSOR_I2C_WRITE(bus, 0x3F4A, 0x03);
	SENSOR_I2C_WRITE(bus, 0x3F4B, 0xBF);
	SENSOR_I2C_WRITE(bus, 0x3F26, 0x01);
	//Integration Time Setting.
	SENSOR_I2C_WRITE(bus, 0x0202, 0x07);
	SENSOR_I2C_WRITE(bus, 0x0203, 0x2A);

	//Gain Setting
	SENSOR_I2C_WRITE(bus, 0x0204, 0x00);    //ANA_GAIN_GLOBAL[9:8]
	SENSOR_I2C_WRITE(bus, 0x0205, 0x00);    //ANA_GAIN_GLOBAL[7:0]

	SENSOR_I2C_WRITE(bus, 0x020E, 0x01);    //DIG_GAIN_GR[15:8]
	SENSOR_I2C_WRITE(bus, 0x020F, 0x00);    //DIG_GAIN_GR[7:0]
	SENSOR_I2C_WRITE(bus, 0x0210, 0x01);    //DIG_GAIN_R[15:8]
	SENSOR_I2C_WRITE(bus, 0x0211, 0x00);    //DIG_GAIN_R[7:0]
	SENSOR_I2C_WRITE(bus, 0x0212, 0x01);    //DIG_GAIN_B[15:8]
	SENSOR_I2C_WRITE(bus, 0x0213, 0x00);    //DIG_GAIN_B[7:0]
	SENSOR_I2C_WRITE(bus, 0x0214, 0x01);    //DIG_GAIN_GB[15:8]
	SENSOR_I2C_WRITE(bus, 0x0215, 0x00);    //DIG_GAIN_GB[7:0]
	//Streaming Setting
	SENSOR_I2C_WRITE(bus, 0x0100, 0x01);

	ISP_PR_INFO("res_8_config_imx577_HDR, success\n");
	return RET_SUCCESS;
}
static int start_imx577(struct sensor_module_t *module, int __maybe_unused
			config_phy)
{
	struct drv_context *drv_context = &module->context;

	ISP_PR_DBG("dev_id[%d], res[%d], bitrate[%dMbps]\n",
		module->id, drv_context->res, drv_context->bitrate);

	// check sensor i2c connection
	if (check_i2c_connection(module) != RET_SUCCESS)
		return RET_FAILURE;
	// write new settings
	switch (drv_context->res) {
	case 0:
		res_0_config_imx577(module);
		break;
	case 1:
		res_1_config_imx577(module);
		break;
	case 2:
		res_2_config_imx577(module);
		break;
	case 3:
		res_3_config_imx577(module);
		break;
	case 4:
		if (drv_context->flag & OPEN_CAMERA_FLAG_HDR)
			res_4_config_imx577_hdr(module);
		else
			res_4_config_imx577(module);
		break;
	case 5:
		if (drv_context->flag & OPEN_CAMERA_FLAG_HDR)
			res_5_config_imx577_hdr(module);
		else
			res_5_config_imx577(module);
		break;

	case 6:
		if (drv_context->flag & OPEN_CAMERA_FLAG_HDR)
			res_6_config_imx577_hdr(module);
		else
			res_6_config_imx577(module);
		break;
	case 7:
		if (drv_context->flag & OPEN_CAMERA_FLAG_HDR)
			res_7_config_imx577_hdr(module);
		else
			res_7_config_imx577(module);
		break;
	case 8:
		if (drv_context->flag & OPEN_CAMERA_FLAG_HDR)
			res_8_config_imx577_hdr(module);
		else
			res_8_config_imx577(module);
		break;
	default:
		ISP_PR_ERR("unsupported res\n");
		return RET_FAILURE;
	}

	ISP_PR_INFO("success\n");

	return RET_SUCCESS;
}

static int stop_imx577(struct sensor_module_t *module)
{
	ISP_PR_INFO("device id[%d]\n", module->id);
	SENSOR_I2C_WRITE(module->hw->sensor_i2c_index, 0x0100, 0x00);
	ISP_PR_INFO("success\n");

	return RET_SUCCESS;
}

static int stream_on_imx577(struct sensor_module_t __maybe_unused *module)
{
	ISP_PR_INFO("device id[%d]\n", module->id);
	//placeholder to set stream on separately, now the streaming on is
	//handled in the end of function res_x_config_imx577.
	//SENSOR_I2C_WRITE(module->hw->sensor_i2c_index, 0x0100, 0x01);
	ISP_PR_INFO("success\n");
	return RET_SUCCESS;
}

static int get_res_fps_imx577(struct sensor_module_t __maybe_unused *module,
				uint32_t res_fps_id, struct res_fps_t *res_fps)
{
	ISP_PR_INFO("res[%d]\n", res_fps_id);

	switch (res_fps_id) {
	case 0:
		res_fps->width = IMX577_RES_0_WIDTH;
		res_fps->height = IMX577_RES_0_HEIGHT;
		res_fps->fps = IMX577_RES_0_MAX_FPS;
		res_fps->none_hdr_support = 1;
		res_fps->hdr_support = 0;
		res_fps->index = res_fps_id;
		res_fps->valid = TRUE;
		break;
	case 1:
		res_fps->width = IMX577_RES_1_WIDTH;
		res_fps->height = IMX577_RES_1_HEIGHT;
		res_fps->fps = IMX577_RES_1_MAX_FPS;
		res_fps->none_hdr_support = 1;
		res_fps->hdr_support = 0;
		res_fps->index = res_fps_id;
		res_fps->valid = TRUE;
		break;
	case 2:
		res_fps->width = IMX577_RES_2_WIDTH;
		res_fps->height = IMX577_RES_2_HEIGHT;
		res_fps->fps = IMX577_RES_2_MAX_FPS;
		res_fps->none_hdr_support = 1;
		res_fps->hdr_support = 0;
		res_fps->index = res_fps_id;
		res_fps->valid = TRUE;
		break;
	case 3:
		res_fps->width = IMX577_RES_3_WIDTH;
		res_fps->height = IMX577_RES_3_HEIGHT;
		res_fps->fps = IMX577_RES_3_MAX_FPS;
		res_fps->none_hdr_support = 1;
		res_fps->hdr_support = 0;
		res_fps->index = res_fps_id;
		res_fps->valid = TRUE;
		break;
	case 4:
		res_fps->width = IMX577_RES_4_WIDTH;
		res_fps->height = IMX577_RES_4_HEIGHT;
		res_fps->fps = IMX577_RES_4_MAX_FPS;
		res_fps->none_hdr_support = 1;
		res_fps->hdr_support = 0;
		res_fps->index = res_fps_id;
		res_fps->valid = TRUE;
		break;
	case 5:
		res_fps->width = IMX577_RES_5_WIDTH;
		res_fps->height = IMX577_RES_5_HEIGHT;
		res_fps->fps = IMX577_RES_5_MAX_FPS;
		res_fps->none_hdr_support = 1;
		res_fps->hdr_support = 0;
		res_fps->index = res_fps_id;
		res_fps->valid = TRUE;
		break;
	case 6:
		res_fps->width = IMX577_RES_6_WIDTH;
		res_fps->height = IMX577_RES_6_HEIGHT;
		res_fps->fps = IMX577_RES_6_MAX_FPS;
		res_fps->none_hdr_support = 1;
		res_fps->hdr_support = 0;
		res_fps->index = res_fps_id;
		res_fps->valid = TRUE;
		break;
	case 7:
		res_fps->width = IMX577_RES_7_WIDTH;
		res_fps->height = IMX577_RES_7_HEIGHT;
		res_fps->fps = IMX577_RES_7_MAX_FPS;
		res_fps->none_hdr_support = 1;
		res_fps->hdr_support = 0;
		res_fps->index = res_fps_id;
		res_fps->valid = TRUE;
		break;
	case 8:
		res_fps->width = IMX577_RES_8_WIDTH;
		res_fps->height = IMX577_RES_8_HEIGHT;
		res_fps->fps = IMX577_RES_8_MAX_FPS;
		res_fps->none_hdr_support = 1;
		res_fps->hdr_support = 0;
		res_fps->index = res_fps_id;
		res_fps->valid = TRUE;
		break;
	default:
		ISP_PR_WARN("unsupported res:%d\n", res_fps_id);
		return RET_FAILURE;
	}

	ISP_PR_INFO("success\n");

	return RET_SUCCESS;
}

static int sned_get_fralenline_cmd_imx577(
				struct sensor_module_t *module)
{
	//uint32_t temp_h = 0 , temp_l = 0;

	init_imx577_context(module->context.res);

	//SENSOR_I2C_READ_FW(module->id, module->hw->sensor_i2c_index, 0x0340,
			//&temp_h, 2);
	//SENSOR_I2C_READ_FW(module->id, module->hw->sensor_i2c_index,
	//		0x0341, &temp_l);

	return RET_SUCCESS;
}

static int get_fralenline_regaddr_imx577(
	struct sensor_module_t __maybe_unused *module,
	unsigned short *hi_addr, unsigned short *lo_addr)
{
	*hi_addr = IMX577_FRA_LEN_LINE_HI_ADDR;
	*lo_addr = IMX577_FRA_LEN_LINE_LO_ADDR;

	return RET_SUCCESS;
}

static int get_caps_imx577(struct sensor_module_t *module,
			   struct _isp_sensor_prop_t *caps, int prf_id)
{
	struct drv_context *drv_context = &module->context;
	u32 max_fps = 30;
	u32 pclk;
	u32 alpha;

	caps->window.h_offset = 0;
	caps->window.v_offset = 0;
	caps->frame_rate = 0;

	if (prf_id == -1)
		prf_id = drv_context->res;

	ISP_PR_DBG("res[%d]\n", prf_id);
	switch (prf_id) {
	case 0:
		caps->window.h_size = IMX577_RES_0_WIDTH;
		caps->window.v_size = IMX577_RES_0_HEIGHT;
		pclk = IMX577_RES_0_LINE_LENGTH_PCLK;
		caps->frame_rate =
			IMX577_RES_0_MAX_FPS * IMX577_FPS_ACCUACY_FACTOR;
		caps->prop.intfProp.mipi.numLanes = 2;
		caps->prop.ae.frameLengthLines =
			IMX577_RES_0_FRAME_LENGTH_LINES;
		caps->prop.ae.timeOfLine =
			FP_UP(pclk) / (IMX577_RES_0_SYSCLK / 1000000) / 4;
		caps->mipi_bitrate = 300;
		break;
	case 1:
		caps->window.h_size = IMX577_RES_1_WIDTH;
		caps->window.v_size = IMX577_RES_1_HEIGHT;
		pclk = IMX577_RES_1_LINE_LENGTH_PCLK;
		caps->frame_rate =
			IMX577_RES_1_MAX_FPS * IMX577_FPS_ACCUACY_FACTOR;
		caps->prop.intfProp.mipi.numLanes = 2;
		caps->prop.ae.frameLengthLines =
			IMX577_RES_1_FRAME_LENGTH_LINES;
		caps->prop.ae.timeOfLine =
			FP_UP(pclk) / (IMX577_RES_1_SYSCLK / 1000000) / 4;
		caps->mipi_bitrate = 312;
		break;
	case 2:
		caps->window.h_size = IMX577_RES_2_WIDTH;
		caps->window.v_size = IMX577_RES_2_HEIGHT;
		pclk = IMX577_RES_2_LINE_LENGTH_PCLK;
		caps->frame_rate =
			IMX577_RES_2_MAX_FPS * IMX577_FPS_ACCUACY_FACTOR;
		caps->prop.intfProp.mipi.numLanes = 4;
		caps->prop.ae.frameLengthLines =
			IMX577_RES_2_FRAME_LENGTH_LINES;
		caps->prop.ae.timeOfLine =
			FP_UP(pclk) / (IMX577_RES_2_SYSCLK / 1000000) / 4;
		caps->mipi_bitrate = 600;
		break;
	case 3:
		caps->window.h_size = IMX577_RES_3_WIDTH;
		caps->window.v_size = IMX577_RES_3_HEIGHT;
		pclk = IMX577_RES_3_LINE_LENGTH_PCLK;
		caps->frame_rate =
			IMX577_RES_3_MAX_FPS * IMX577_FPS_ACCUACY_FACTOR;
		caps->prop.intfProp.mipi.numLanes = 4;
		caps->prop.ae.frameLengthLines =
			IMX577_RES_3_FRAME_LENGTH_LINES;
		caps->prop.ae.timeOfLine =
			FP_UP(pclk) / (IMX577_RES_3_SYSCLK / 1000000) / 4;
		caps->mipi_bitrate = 600;
		break;
	case 4:
		caps->window.h_size = IMX577_RES_4_WIDTH;
		caps->window.v_size = IMX577_RES_4_HEIGHT;
		pclk = IMX577_RES_4_LINE_LENGTH_PCLK;
		caps->frame_rate =
			IMX577_RES_4_MAX_FPS * IMX577_FPS_ACCUACY_FACTOR;
		caps->prop.intfProp.mipi.numLanes = 4;
		caps->prop.ae.frameLengthLines =
			IMX577_RES_4_FRAME_LENGTH_LINES;
		caps->prop.ae.timeOfLine =
			FP_UP(pclk) / (IMX577_RES_4_SYSCLK / 1000000) / 4;
		caps->mipi_bitrate = 1050;
		break;
	case 5:
		caps->window.h_size = IMX577_RES_5_WIDTH;
		caps->window.v_size = IMX577_RES_5_HEIGHT;
		pclk = IMX577_RES_5_LINE_LENGTH_PCLK;
		caps->frame_rate = IMX577_RES_5_MAX_FPS *
					IMX577_FPS_ACCUACY_FACTOR;
		caps->prop.intfProp.mipi.numLanes = 4;
		caps->prop.ae.frameLengthLines =
			IMX577_RES_5_FRAME_LENGTH_LINES;
		caps->prop.ae.timeOfLine =
			FP_UP(pclk) / (IMX577_RES_5_SYSCLK / 1000000) / 4;
		caps->mipi_bitrate = 1050;
		break;
	case 6:
		caps->window.h_size = IMX577_RES_6_WIDTH;
		caps->window.v_size = IMX577_RES_6_HEIGHT;
		pclk = IMX577_RES_6_LINE_LENGTH_PCLK;
		caps->frame_rate = IMX577_RES_6_MAX_FPS *
					IMX577_FPS_ACCUACY_FACTOR;
		caps->prop.intfProp.mipi.numLanes = 4;
		caps->prop.ae.frameLengthLines =
			IMX577_RES_6_FRAME_LENGTH_LINES;
		caps->prop.ae.timeOfLine =
			FP_UP(pclk) / (IMX577_RES_6_SYSCLK / 1000000) / 4;
		caps->mipi_bitrate = 1050;
		break;
	case 7:
		caps->window.h_size = IMX577_RES_7_WIDTH;
		caps->window.v_size = IMX577_RES_7_HEIGHT;
		pclk = IMX577_RES_7_LINE_LENGTH_PCLK;
		caps->frame_rate =
			IMX577_RES_7_MAX_FPS * IMX577_FPS_ACCUACY_FACTOR;
		caps->prop.intfProp.mipi.numLanes = 2;
		caps->prop.ae.frameLengthLines =
			IMX577_RES_7_FRAME_LENGTH_LINES;
		caps->prop.ae.timeOfLine =
			FP_UP(pclk) / (IMX577_RES_7_SYSCLK / 1000000) / 4;
		caps->mipi_bitrate = 300;
		break;
	case 8:
		caps->window.h_size = IMX577_RES_8_WIDTH;
		caps->window.v_size = IMX577_RES_8_HEIGHT;
		pclk = IMX577_RES_8_LINE_LENGTH_PCLK;
		caps->frame_rate =
			IMX577_RES_8_MAX_FPS * IMX577_FPS_ACCUACY_FACTOR;
		caps->prop.intfProp.mipi.numLanes = 2;
		caps->prop.ae.frameLengthLines =
			IMX577_RES_8_FRAME_LENGTH_LINES;
		caps->prop.ae.timeOfLine =
			FP_UP(pclk) / (IMX577_RES_8_SYSCLK / 1000000) / 4;
		caps->mipi_bitrate = 300;
		break;
	default:
		ISP_PR_ERR("unsupported res\n");
		return RET_FAILURE;
	}

#ifdef ENABLE_PPI
	caps->prop.intfType = SENSOR_INTF_TYPE_MIPI;
	caps->prop.intfProp.mipi.dataType = MIPI_DATA_TYPE_RAW_10;
	caps->prop.intfProp.mipi.compScheme = MIPI_COMP_SCHEME_NONE;
	caps->prop.intfProp.mipi.predBlock = MIPI_PRED_BLOCK_INVALID;
#else
	caps->prop.intfType = SENSOR_INTF_TYPE_PARALLEL;
	caps->prop.intfProp.parallel.dataType =
		PARALLEL_DATA_TYPE_RAW10;
	caps->prop.intfProp.parallel.hPol = POLARITY_HIGH;
	caps->prop.intfProp.parallel.vPol = POLARITY_LOW;
	caps->prop.intfProp.parallel.edge = SAMPLE_EDGE_NEG;
	caps->prop.intfProp.parallel.ycSeq = YCSEQUENCE_YCBYCR;
	caps->prop.intfProp.parallel.subSampling =
		COLOR_SUB_SAMPLING_CONV422_COSITED;
#endif
	caps->prop.cfaPattern = CFA_PATTERN_RGGB;
	caps->prop.sensorShutterType = SENSOR_SHUTTER_TYPE_ROLLING;

	caps->prop.itimeDelayFrames = 2;
	caps->prop.gainDelayFrames = 2;
#ifdef IMX577_EMBEDDED_DATA_EN
		caps->prop.hasEmbeddedData	= TRUE;
#else
		caps->prop.hasEmbeddedData	= FALSE;
#endif
	caps->prop.ae.type = SENSOR_AE_PROP_TYPE_S0NY;
	caps->prop.ae.baseIso = IMX577_BASE_ISO;
	caps->prop.ae.minExpoLine = IMX577_MIN_EXPOSURE_LINE;
	caps->prop.ae.maxExpoLine = IMX577_MAX_EXPOSURE_LINE;
	caps->prop.ae.expoLineAlpha = IMX577_LINE_COUNT_OFFSET;
	caps->prop.ae.minAnalogGain = (uint32_t)IMX577_MIN_AGAIN;
	caps->prop.ae.maxAnalogGain = (uint32_t)IMX577_MAX_AGAIN;
	caps->prop.ae.initItime = (exp_config[prf_id].coarse_integ_time +
				   IMX577_FINE_INTEG_TIME / pclk) *
				  FP_DN(caps->prop.ae.timeOfLine);
	caps->prop.ae.initAgain =
		FP_DN(INT_DIV(FP_UP(1024000),
			      1024 - exp_config[prf_id].a_gain_global) + FP_05);
	caps->prop.ae.expoOffset = (uint32_t)(IMX577_FINE_INTEG_TIME / pclk);
	caps->prop.ae.formula.weight1 = IMX577_GAIN_FACTOR;
	caps->prop.ae.formula.weight2 = IMX577_GAIN_FACTOR;
	caps->prop.ae.formula.minShift = 0;
	caps->prop.ae.formula.maxShift = 0;
	caps->prop.ae.formula.minParam = IMX577_MIN_GAIN_SETTING;
	caps->prop.ae.formula.maxParam = IMX577_MAX_GAIN_SETTING;

	caps->prop.af.min_pos = IMX577_VCM_MIN_POS;
	caps->prop.af.max_pos = IMX577_VCM_MAX_POS;
	caps->prop.af.init_pos = IMX577_VCM_INIT_POS;
	caps->exposure.min_digi_gain = (uint32_t)IMX577_MIN_DGAIN;
	caps->exposure.max_digi_gain = (uint32_t)IMX577_MAX_DGAIN;
	caps->hdr_valid = 0;
	caps->exposure.min_integration_time =
		FP_DN(2 * caps->prop.ae.timeOfLine);
	alpha = FP_DN(INT_DIV(FP_UP(IMX577_FINE_INTEG_TIME), pclk) + FP_05);
	caps->exposure.max_integration_time =
		min(FP_DN(INT_DIV(FP_UP(1000000), max_fps)),
		    (uint32_t)((caps->prop.ae.frameLengthLines - 22 + alpha) *
			       FP_DN(caps->prop.ae.timeOfLine)));
	caps->exposure_orig = caps->exposure;
	caps->exposure.min_gain = (uint32_t)IMX577_MIN_AGAIN;
	caps->exposure.max_gain = (uint32_t)IMX577_MAX_AGAIN;

	if (module->context.flag & OPEN_CAMERA_FLAG_HDR) {
		struct _SensorHdrProp_t *hdrProp = &caps->hdrProp;
		int i;

		caps->hdr_valid = 1;
		hdrProp->virtChannel = 0x0;
		hdrProp->dataType = 0x2B;
		hdrProp->matrixId = HDR_STAT_DATA_MATRIX_ID_16X16;

		switch (module->context.res) {
		case 0:
		case 1:
		case 2:
		case 3:
		case 4:
			hdrProp->hdrStatWindow.h_size = 256;
			hdrProp->hdrStatWindow.v_size = 2;
			break;
		case 5:
			hdrProp->hdrStatWindow.h_size = 256;
			hdrProp->hdrStatWindow.v_size = 2;
			break;
		case 6:
			hdrProp->hdrStatWindow.h_size = 256;
			hdrProp->hdrStatWindow.v_size = 2;
			break;
		case 7:
			hdrProp->hdrStatWindow.h_size = 256;
			hdrProp->hdrStatWindow.v_size = 2;
			break;
		case 8:
			hdrProp->hdrStatWindow.h_size = 256;
			hdrProp->hdrStatWindow.v_size = 2;
			break;
		default:
			ISP_PR_WARN("unsupported res\n");
			return RET_FAILURE;
		}

#ifdef CONFIG_IMX577_HDR_ITIME_SEPARATE
		hdrProp->itimeType = HDR_ITIME_TYPE_SEPARATE;
#else
		hdrProp->itimeType = HDR_ITIME_TYPE_RATIO;
		for (i = 0; i < MAX_HDR_RATIO_NUM; i++)
			hdrProp->itimeRatioList[i] = 0;

		/* Add the supported ratio list */
		hdrProp->itimeRatioList[0] = 1;
		hdrProp->itimeRatioList[1] = 2;
		hdrProp->itimeRatioList[2] = 4;
		hdrProp->itimeRatioList[3] = 8;
		hdrProp->itimeRatioList[4] = 16;
#endif

#ifdef CONFIG_IMX577_HDR_ANALOG_GAIN_SEPARATE
		hdrProp->aGainType = HDR_AGAIN_TYPE_SEPARATE;
#else
#error "The hdr analog gain type is not defined"
#endif
#ifdef CONFIG_IMX577_HDR_DIGITAL_GAIN_EQUAL
		hdrProp->dGainType = HDR_DGAIN_TYPE_EQUAL;
#else
#error "The hdr digital gain type is not defined"
#endif
	}

	return RET_SUCCESS;
}

static int get_gain_limit_imx577(struct sensor_module_t __maybe_unused *module,
			uint32_t *min_gain, uint32_t *max_gain)
{
	*min_gain = (uint32_t)IMX577_MIN_AGAIN;
	*max_gain = (uint32_t)IMX577_MAX_AGAIN;

	return RET_SUCCESS;
}

static int get_emb_prop_imx577(struct sensor_module_t __maybe_unused *module,
				bool *supported,
				struct _SensorEmbProp_t *embProp)
{
	*supported = TRUE;
	embProp->virtChannel = 0x0;
	embProp->dataType = 0x12;
	embProp->embDataWindow.h_offset = 0;
	embProp->embDataWindow.v_offset= 0;
	embProp->embDataWindow.h_size= 100;
	embProp->embDataWindow.v_size = 1;
	embProp->expoStartByteOffset = 47;
	embProp->expoNeededBytes = 9;
	return RET_SUCCESS;
}
static int get_integration_time_limit_imx577(
					struct sensor_module_t *module,
					uint32_t *
					min_integration_time,
					uint32_t *
					max_integration_time)
{
	//struct drv_context *drv_context = &module->context;
	uint32_t max_fps = 30;
	uint32_t alpha;
	ISP_PR_INFO("get_itime_limit_imx577\n");
	ISP_PR_INFO("res = %d\n", module->context.res);
	//ISP_PR_INFO("t_line = %dns\n", g_imx577_context.t_line_ns);

	init_imx577_context(module->context.res);
	*min_integration_time = FP_DN(2 * g_imx577_context.t_line_ns);

	ISP_PR_INFO("max_fps = %d\n", max_fps);

	alpha = FP_DN(INT_DIV(FP_UP(IMX577_FINE_INTEG_TIME),
		g_imx577_context.line_length_pck) + FP_05);

	*max_integration_time =
		min(FP_DN(INT_DIV(FP_UP(1000000), max_fps)), (uint32_t)
		((g_imx577_context.frame_length_lines - 22 +
		alpha) * FP_DN(g_imx577_context.t_line_ns)));

	ISP_PR_INFO("min integration time limit = %d(us)\n",
			*min_integration_time);

	ISP_PR_INFO("max integration time limit = %d(us)\n",
			*max_integration_time);
	ISP_PR_INFO("<- get_itime_limit_imx577, success\n");
	return RET_SUCCESS;
}

static int support_af_imx577(struct sensor_module_t __maybe_unused *module,
				uint32_t *support)
{
	*support = 0;

	return RET_SUCCESS;
}

static int get_i2c_config_imx577(struct sensor_module_t __maybe_unused *module,
		uint32_t *i2c_addr, uint32_t __maybe_unused *mode,
		uint32_t *reg_addr_width)
{
	*i2c_addr = GROUP0_I2C_SLAVE_ADDR;
	*mode = I2C_DEVICE_ADDR_TYPE_7BIT;
	*reg_addr_width = I2C_REG_ADDR_TYPE_16BIT;

	return RET_SUCCESS;
}

static int get_fw_script_ctrl_imx577(struct sensor_module_t *module,
				struct _isp_fw_script_ctrl_t *script)
{
	ISP_PR_INFO("device id %d\n", module->id);

	// get I2C prop of the sensor
	script->i2cAe.deviceId = I2C_DEVICE_ID_INVALID;
	script->i2cAe.deviceAddr = GROUP0_I2C_SLAVE_ADDR;
	script->i2cAe.deviceAddrType = I2C_DEVICE_ADDR_TYPE_7BIT;
	script->i2cAe.regAddrType = I2C_REG_ADDR_TYPE_16BIT;
	script->i2cAe.enRestart = 1;

	script->i2cAf.deviceId = I2C_DEVICE_ID_INVALID;
	script->i2cAf.deviceAddr = 0x72;
	script->i2cAf.deviceAddrType = I2C_DEVICE_ADDR_TYPE_7BIT;
	script->i2cAf.regAddrType = I2C_REG_ADDR_TYPE_8BIT;
	script->i2cAf.enRestart =  1;
	script->argAe.args[0] =
		module->context.t_line_numerator /
		module->context.t_line_denominator;
	script->argAe.args[1] =
		module->context.line_length_pck;
	script->argAe.args[2] = MIPI_DATA_TYPE_RAW_10;
	script->argAe.numArgs = 3;

	//AF
	script->argAf.numArgs = 0;

	//Flash
	script->argFl.numArgs = 0;

	ISP_PR_INFO("success\n");

	return RET_SUCCESS;
}

static int get_sensor_hw_parameter_imx577(
	struct sensor_module_t __maybe_unused *module,
	struct sensor_hw_parameter *para)
{
	ISP_PR_INFO("device id[%d]\n", module->id);
	if (para) {
		para->type = CAMERA_TYPE_RGB_BAYER;
		para->focus_len = 1;
		para->support_hdr = 1;
		para->min_lens_pos = IMX577_VCM_MIN_POS;
		para->max_lens_pos = IMX577_VCM_MAX_POS;
		para->init_lens_pos = IMX577_VCM_INIT_POS;
		para->lens_step = 0;
		para->normalized_focal_len_x = 28;
		para->normalized_focal_len_y = 25;
	} else {
		ISP_PR_ERR("failed for NULL para\n");
		return RET_FAILURE;
	}

	ISP_PR_INFO("suc\n");
	return RET_SUCCESS;
}

static int set_test_pattern_imx577(
	struct sensor_module_t __maybe_unused *module,
	enum sensor_test_pattern mode)
{
	ISP_PR_INFO("test pattern mode [%d]\n", mode);

	switch (mode) {
	case 0:
		SENSOR_I2C_WRITE(module->hw->sensor_i2c_index, 0x0600, 0x00);
		SENSOR_I2C_WRITE(module->hw->sensor_i2c_index, 0x0601, 0x00);
		break;
	case 1:
		SENSOR_I2C_WRITE(module->hw->sensor_i2c_index, 0x0600, 0x00);
		SENSOR_I2C_WRITE(module->hw->sensor_i2c_index, 0x0601, 0x01);
		break;
	case 2:
		SENSOR_I2C_WRITE(module->hw->sensor_i2c_index, 0x0600, 0x00);
		SENSOR_I2C_WRITE(module->hw->sensor_i2c_index, 0x0601, 0x02);
		break;
	case 3:
		SENSOR_I2C_WRITE(module->hw->sensor_i2c_index, 0x0600, 0x00);
		SENSOR_I2C_WRITE(module->hw->sensor_i2c_index, 0x0601, 0x03);
		break;
	case 4:
		SENSOR_I2C_WRITE(module->hw->sensor_i2c_index, 0x0600, 0x00);
		SENSOR_I2C_WRITE(module->hw->sensor_i2c_index, 0x0601, 0x04);
		break;
	default:
		ISP_PR_ERR("Unsupported test pattern mode :%d\n", mode);
		return RET_FAILURE;
	}

	return RET_SUCCESS;
}

struct sensor_module_t g_imx577_driver = {
	.id = 0,
	.hw = NULL,
	.name = "imx577",
	.poweron_sensor = poweron_imx577,
	.reset_sensor = reset_imx577,
	.config_sensor = config_imx577,
	.start_sensor = start_imx577,
	.stop_sensor = stop_imx577,
	.stream_on_sensor = stream_on_imx577,
	.get_caps = get_caps_imx577,
	.get_gain_limit = get_gain_limit_imx577,
	.get_integration_time_limit = get_integration_time_limit_imx577,
	.exposure_control = exposure_control_imx577,
	.get_current_exposure = get_current_exposure_imx577,
	.support_af = support_af_imx577,
	.hdr_support = NULL,
	.hdr_get_expo_ratio = NULL,
	.hdr_set_expo_ratio = NULL,
	.hdr_set_gain_mode = NULL,
	.hdr_set_gain = NULL,
	.hdr_enable = NULL,
	.hdr_disable = NULL,
	.get_emb_prop = get_emb_prop_imx577,
	.get_res_fps = get_res_fps_imx577,
	.sned_get_fralenline_cmd = sned_get_fralenline_cmd_imx577,
	.get_fralenline_regaddr = get_fralenline_regaddr_imx577,
	.get_i2c_config = get_i2c_config_imx577,
	.get_fw_script_ctrl = get_fw_script_ctrl_imx577,
	.get_analog_gain = get_analog_gain_imx577,
	.get_itime = get_itime_imx577,
	.get_sensor_hw_parameter = get_sensor_hw_parameter_imx577,
	.set_test_pattern = set_test_pattern_imx577,
	.get_m2m_data = NULL,
};
