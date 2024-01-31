/*
 * Copyright(C) 2018-2020 Advanced Micro Devices, Inc. All rights reserved.
 */

#include "sensor_if.h"

#define DEV_VENDOR				Sony
#define DEV_NAME				imx208
#define GROUP1_NAME				sensor_phy
#define GROUP0_NAME				sensor

#define GROUP1_I2C_SLAVE_ADDR			0x14
#define GROUP1_I2C_REG_ADDR_SIZE		2
#define GROUP1_I2C_REG_SIZE			1
#define GROUP1_I2C_SLAVE_ADDR_MODE		I2C_SLAVE_ADDRESS_MODE_7BIT

#define GROUP0_I2C_SLAVE_ADDR			0x10
#define GROUP0_I2C_REG_ADDR_SIZE		2
#define GROUP0_I2C_REG_SIZE			1
#define GROUP0_I2C_SLAVE_ADDR_MODE		I2C_SLAVE_ADDRESS_MODE_7BIT

#define ENABLE_PPI
//#define SIMU_SENSOR_AS_RGBIR_SENSOR

//#define SENSOR_CLK_20M
#define SENSOR_CLK_24M

//Tuning parameters:
#define IMX208_FPS_ACCUACY_FACTOR		1000

#define IMX208_RES_0_MIN_FPS			15
#define IMX208_RES_0_MAX_FPS			15
#define IMX208_RES_0_SYSCLK			40500000
#define IMX208_RES_0_WIDTH			1920
#define IMX208_RES_0_HEIGHT			1080
#define IMX208_RES_0_FRAME_LENGTH_LINES		1200
#define IMX208_RES_0_LINE_LENGTH_PCLK		2248

#define IMX208_RES_1_MIN_FPS			30
#define IMX208_RES_1_MAX_FPS			30
#define IMX208_RES_1_SYSCLK			81000000
#define IMX208_RES_1_WIDTH			1936
#define IMX208_RES_1_HEIGHT			1096
#define IMX208_RES_1_FRAME_LENGTH_LINES		1200
#define IMX208_RES_1_LINE_LENGTH_PCLK		2248

#define IMX208_RES_2_MIN_FPS			60
#define IMX208_RES_2_MAX_FPS			60
#define IMX208_RES_2_SYSCLK			162000000
#define IMX208_RES_2_WIDTH			1920
#define IMX208_RES_2_HEIGHT			1080
#define IMX208_RES_2_FRAME_LENGTH_LINES		1200
#define IMX208_RES_2_LINE_LENGTH_PCLK		2248	// 2lane, double of reg

#define IMX208_RES_3_MIN_FPS			(120/4)
#define IMX208_RES_3_MAX_FPS			(120/4)
#define IMX208_RES_3_SYSCLK			(81000000/4)
#define IMX208_RES_3_WIDTH			640
#define IMX208_RES_3_HEIGHT			480
#define IMX208_RES_3_FRAME_LENGTH_LINES		600
// 2lane, but no double,search the timing excel
#define IMX208_RES_3_LINE_LENGTH_PCLK		1124

#define IMX208_RES_4_MIN_FPS			240
#define IMX208_RES_4_MAX_FPS			240
#define IMX208_RES_4_SYSCLK			40500000
#define IMX208_RES_4_WIDTH			320
#define IMX208_RES_4_HEIGHT			240
#define IMX208_RES_4_FRAME_LENGTH_LINES		300
// set smaller than reg, any question, can find james
#define IMX208_RES_4_LINE_LENGTH_PCLK		562

//640x480 binning mode
#define IMX208_RES_5_MIN_FPS			(120/4)
#define IMX208_RES_5_MAX_FPS			(120/4)
#define IMX208_RES_5_SYSCLK			(81000000/4)
#define IMX208_RES_5_WIDTH			640
#define IMX208_RES_5_HEIGHT			480
#define IMX208_RES_5_FRAME_LENGTH_LINES 600
// 2lane, but no double ,search the timing excel
#define IMX208_RES_5_LINE_LENGTH_PCLK		1124

/*
 *from data sheet imx214 again range: 1x ~ 8x
 *dgain range: 1x ~ 16x
 */

#define IMX208_GAIN_ACCUACY_FACTOR	1000.0
#define IMX208_MIN_AGAIN		(1.0 * IMX208_GAIN_ACCUACY_FACTOR)
#define IMX208_MAX_AGAIN		(15.9 * IMX208_GAIN_ACCUACY_FACTOR)
#define IMX208_MIN_DGAIN		(1.0 * IMX208_GAIN_ACCUACY_FACTOR)
#define IMX208_MAX_DGAIN		(15.9 * IMX208_GAIN_ACCUACY_FACTOR)

#define IMX208_LINE_COUNT_OFFSET	5
#define IMX208_MAX_LINE_COUNT		(65535 - IMX208_LINE_COUNT_OFFSET)

#define IMX208_FraLenLineHiAddr		0x0340
#define IMX208_FraLenLineLoAddr		0x0341

typedef struct _imx208_drv_context_t {
	uint32_t frame_length_lines;
	uint32_t line_length_pck;
	uint32_t logic_clock;
	float t_line;
	uint32_t current_frame_length_lines;
	uint32_t max_line_count;
} imx208_drv_context_t;

imx208_drv_context_t g_imx208_context;
static void init_imx208_context(uint32_t res)
{
	switch (res) {
	case 0:
		g_imx208_context.frame_length_lines =
			IMX208_RES_0_FRAME_LENGTH_LINES;
		g_imx208_context.line_length_pck =
			IMX208_RES_0_LINE_LENGTH_PCLK;
		g_imx208_context.logic_clock = IMX208_RES_0_SYSCLK;
		break;
	case 1:
		g_imx208_context.frame_length_lines =
			IMX208_RES_1_FRAME_LENGTH_LINES;
		g_imx208_context.line_length_pck =
			IMX208_RES_1_LINE_LENGTH_PCLK;
		g_imx208_context.logic_clock = IMX208_RES_1_SYSCLK;
		break;
	case 2:
		g_imx208_context.frame_length_lines =
			IMX208_RES_2_FRAME_LENGTH_LINES;
		g_imx208_context.line_length_pck =
			IMX208_RES_2_LINE_LENGTH_PCLK;
		g_imx208_context.logic_clock = IMX208_RES_2_SYSCLK;
		break;
	case 3:
		g_imx208_context.frame_length_lines =
			IMX208_RES_3_FRAME_LENGTH_LINES;
		g_imx208_context.line_length_pck =
			IMX208_RES_3_LINE_LENGTH_PCLK;
		g_imx208_context.logic_clock = IMX208_RES_3_SYSCLK;
		break;
	case 4:
		g_imx208_context.frame_length_lines =
			IMX208_RES_4_FRAME_LENGTH_LINES;
		g_imx208_context.line_length_pck =
			IMX208_RES_4_LINE_LENGTH_PCLK;
		g_imx208_context.logic_clock = IMX208_RES_4_SYSCLK;
		break;
	case 5:
		g_imx208_context.frame_length_lines =
			IMX208_RES_5_FRAME_LENGTH_LINES;
		g_imx208_context.line_length_pck =
			IMX208_RES_5_LINE_LENGTH_PCLK;
		g_imx208_context.logic_clock =
			IMX208_RES_5_SYSCLK;
		break;
	default:
		return;
	}

	g_imx208_context.max_line_count =
	    g_imx208_context.logic_clock / g_imx208_context.line_length_pck;


	g_imx208_context.t_line = (float)(g_imx208_context.line_length_pck /
				(g_imx208_context.logic_clock/1000000));
	g_imx208_context.current_frame_length_lines =
		g_imx208_context.frame_length_lines;

}

static result_t set_gain_imx208(struct sensor_module_t *module,
				uint32_t new_gain, uint32_t *set_gain)
{
	uint32_t gain, set_gain_tmp;


		gain = (uint32_t) (((float)256000 / (new_gain)) + 0.5);
		gain = 256 - gain;
		gain = gain & 0xff;

		SENSOR_LOG_INFO("new_gain 0x0205 = %u\n", gain);
		SENSOR_I2C_WRITE(module->hw->sensor_i2c_index, 0x0205, gain);
		set_gain_tmp = (uint32_t) ((float)256000 / (256 - gain) + 0.5);
		SENSOR_LOG_INFO("set_gain_tmp = %u\n", set_gain_tmp);

		if (set_gain) {
			*set_gain = set_gain_tmp;
			SENSOR_LOG_INFO
			("gain = %d, *set_gain = %d, Reg(0x0205)=0x%x\n",
			gain, *set_gain, set_gain_tmp);
		}

	return RET_SUCCESS;
}

static result_t get_analog_gain_imx208(struct sensor_module_t *module,
				uint32_t *gain)
{
	uint32_t temp_gain = 0;

		SENSOR_I2C_READ(module->hw->sensor_i2c_index, 0x0205, gain);
		*gain = (*gain) & 0xff;
		SENSOR_LOG_INFO("get_gain_tmp = %u\n", *gain);
		temp_gain = (uint32_t) ((float)256000 / (256 - (*gain)) + 0.5);
		SENSOR_LOG_INFO("analog gain = %u\n", temp_gain);
		if (gain)
			*gain = temp_gain;

	return RET_SUCCESS;
};

/*
 * new_integration_time (unit: us)
 */
static result_t set_itime_imx208(struct sensor_module_t *module, uint32_t res,
			uint32_t new_integration_time,
			uint32_t *set_integration_time)
{
	uint32_t line_count, temp_frame_length_lines;
	uint32_t line_count_h, line_count_l = 0;
	uint32_t temp_frame_length_lines_h, temp_frame_length_lines_l = 0;

	init_imx208_context(res);

		SENSOR_LOG_INFO
			("new_integration_time = %d(us), t_line = %d(us)\n",
			new_integration_time,
			(uint32_t) (g_imx208_context.t_line));

		line_count = (uint32_t) ((float)new_integration_time /
				(g_imx208_context.t_line) + 0.5);

		if (line_count > IMX208_MAX_LINE_COUNT)
			line_count = IMX208_MAX_LINE_COUNT;

		if ((line_count + IMX208_LINE_COUNT_OFFSET) >
			g_imx208_context.frame_length_lines) {
			temp_frame_length_lines =
				line_count + IMX208_LINE_COUNT_OFFSET;
		} else {
			temp_frame_length_lines =
				g_imx208_context.frame_length_lines;
		}

		if (line_count >
			g_imx208_context.current_frame_length_lines -
			IMX208_LINE_COUNT_OFFSET) {
			// decrease frame rate
			// update frame length lines
			temp_frame_length_lines_h =
				(temp_frame_length_lines >> 8) & 0xff;
			temp_frame_length_lines_l =
				temp_frame_length_lines & 0xff;
			SENSOR_I2C_WRITE(module->hw->sensor_i2c_index, 0x0340,
					temp_frame_length_lines_h);
			SENSOR_I2C_WRITE(module->hw->sensor_i2c_index, 0x0341,
					temp_frame_length_lines_l);
			// update new line count
			line_count_h = (line_count >> 8) & 0xff;
			line_count_l = line_count & 0xff;
			SENSOR_I2C_WRITE(module->hw->sensor_i2c_index, 0x0202,
					line_count_h);
			SENSOR_I2C_WRITE(module->hw->sensor_i2c_index, 0x0203,
					line_count_l);
		} else {
			// increase frame rate
			// update new line count
			line_count_h = (line_count >> 8) & 0xff;
			line_count_l = line_count & 0xff;
			SENSOR_I2C_WRITE(module->hw->sensor_i2c_index, 0x0202,
					line_count_h);
			SENSOR_I2C_WRITE(module->hw->sensor_i2c_index, 0x0203,
					line_count_l);
			// update frame length lines
			temp_frame_length_lines_h =
				(temp_frame_length_lines >> 8) & 0xff;
			temp_frame_length_lines_l =
				temp_frame_length_lines & 0xff;
			SENSOR_I2C_WRITE(module->hw->sensor_i2c_index, 0x0340,
					 temp_frame_length_lines_h);
			SENSOR_I2C_WRITE(module->hw->sensor_i2c_index, 0x0341,
					 temp_frame_length_lines_l);
		}
		g_imx208_context.current_frame_length_lines =
							temp_frame_length_lines;
		if (set_integration_time) {
			*set_integration_time = new_integration_time;
			SENSOR_LOG_INFO
			("*set_integration_time = %d(us), line_count = %d,",
				*set_integration_time, line_count);
			SENSOR_LOG_INFO
			("Reg(0x0340)=0x%x, Reg(0x0341)=0x%x,",
				temp_frame_length_lines_h,
				temp_frame_length_lines_l);
			SENSOR_LOG_INFO
			("Reg(0x0202)=0x%x, Reg(0x0203)=0x%x\n",
				line_count_h, line_count_l);
		}


	return RET_SUCCESS;
}

/*
 * itime (unit: us)
 */
static result_t get_itime_imx208(struct sensor_module_t *module,
			uint32_t *itime)
{
	uint16_t line_count = 0;
	uint8_t line_count_h, line_count_l = 0;

	init_imx208_context(module->context.res);
	SENSOR_I2C_READ(module->hw->sensor_i2c_index, 0x0202, &line_count_h);
	SENSOR_I2C_READ(module->hw->sensor_i2c_index, 0x0203, &line_count_l);
	line_count = ((line_count_h & 0xff) << 8) | (line_count_l & 0xff);
	if (itime) {
		*itime =
			(uint32_t) (line_count * g_imx208_context.t_line);

		SENSOR_LOG_INFO
("*itime = %d(us), line_count = %d,line_count_h = 0x%x, line_count_l = 0x%x\n",
				*itime, line_count, line_count_h,
				line_count_l);

	}

	return RET_SUCCESS;
}

/*
 * new_gain : 12345 means 12.345x
 * new_integration_time (unit: us)
 */
static result_t exposure_control_imx208(struct sensor_module_t *module,
					uint32_t new_gain,
					uint32_t new_integration_time,
					uint32_t *set_gain,
					uint32_t *set_integration)
{
	if (new_gain)
		set_gain_imx208(module, new_gain, set_gain);
	if (new_integration_time)
		set_itime_imx208(module, module->context.res,
				new_integration_time, set_integration);

	return RET_SUCCESS;
};

static result_t get_current_exposure_imx208(struct sensor_module_t *module,
				uint32_t *gain,
				uint32_t *integration_time)
{
	get_analog_gain_imx208(module, gain);
	get_itime_imx208(module, integration_time);

	return RET_SUCCESS;
}

static result_t poweron_imx208(struct sensor_module_t *module, bool_t on)
{
	on = on;
	module = module;
	SENSOR_LOG_INFO("-> %s: on = %d\n", __func__, on);
	return RET_SUCCESS;
}

static result_t reset_imx208(struct sensor_module_t *module)
{
	module = module;
	SENSOR_LOG_INFO("-> %s\n",  __func__);
	return RET_SUCCESS;
}

static int32 config_imx208(struct sensor_module_t *module, uint32 res,
			uint32 flag)
{
	struct drv_context *drv_context = &module->context;

	// TODO
	//module = module;

	drv_context->res = res;
	drv_context->flag = flag;

	SENSOR_LOG_INFO("-> %s, res %d\n", __func__, res);
	init_imx208_context(res);
	drv_context->frame_length_lines = g_imx208_context.frame_length_lines;
	drv_context->line_length_pck = g_imx208_context.line_length_pck;
	switch (res) {
	// avoid float value in t_line_denominator and
	// t_line_numerator to keep accuracy
	// avoid t_line_denominator * maxmun line count overflow
	//to the sign bit(bit31)
	case 0:
		drv_context->t_line_numerator =
			2 * IMX208_RES_0_LINE_LENGTH_PCLK;
		drv_context->t_line_denominator =
			2 * IMX208_RES_0_SYSCLK / 1000000;
		drv_context->bitrate = IMX208_RES_0_SYSCLK * 10 / 1000000;
		break;
	case 1:
		drv_context->t_line_numerator = IMX208_RES_1_LINE_LENGTH_PCLK;
		drv_context->t_line_denominator =
			IMX208_RES_1_SYSCLK / 1000000;
		drv_context->bitrate = IMX208_RES_1_SYSCLK * 10 / 1000000;
		break;
	case 2:
		drv_context->t_line_numerator = IMX208_RES_2_LINE_LENGTH_PCLK;
		drv_context->t_line_denominator =
			IMX208_RES_2_SYSCLK / 1000000;
		drv_context->bitrate = IMX208_RES_2_SYSCLK / 2 * 10 / 1000000;
		break;
	case 3:
		drv_context->t_line_numerator = IMX208_RES_3_LINE_LENGTH_PCLK;
		drv_context->t_line_denominator =
			IMX208_RES_3_SYSCLK / 1000000;
		drv_context->bitrate = IMX208_RES_3_SYSCLK / 2 * 10 / 1000000;
		break;
	case 4:
		drv_context->t_line_numerator = IMX208_RES_4_LINE_LENGTH_PCLK;
		drv_context->t_line_denominator =
			IMX208_RES_4_SYSCLK / 1000000;
		drv_context->bitrate = IMX208_RES_4_SYSCLK / 2 * 10 / 1000000;
		break;
	case 5:
		drv_context->t_line_numerator = IMX208_RES_5_LINE_LENGTH_PCLK;
		drv_context->t_line_denominator =
			IMX208_RES_5_SYSCLK / 1000000;
		drv_context->bitrate = IMX208_RES_5_SYSCLK/2 * 10 / 1000000;
		break;
	default:
		SENSOR_LOG_ERROR("Unsupported res:%d\n", res);
		return RET_FAILURE;
	}

	drv_context->mode = 0;	// need recheck
	drv_context->max_torch_curr = FLASH_MAX_TORCH_CURR_LEVEL0;
	drv_context->max_flash_curr = FLASH_MAX_FLASH_CURR_LEVEL0;

	SENSOR_LOG_INFO
		("res = %u, frame_length_lines = %u, line_length_pck = %u,",
		drv_context->res, drv_context->frame_length_lines,
		drv_context->line_length_pck);
	SENSOR_LOG_INFO
		("t_line_numerator = %u, t_line_denominator = %u,",
		drv_context->t_line_numerator,
		drv_context->t_line_denominator);
	SENSOR_LOG_INFO
	("bitrate = %u, mode = %u, max_torch_curr = %u, max_flash_curr = %u\n",
		drv_context->bitrate, drv_context->mode,
		drv_context->max_torch_curr, drv_context->max_flash_curr);

	SENSOR_LOG_INFO("<- %s, success\n", __func__);

	return RET_SUCCESS;
}

static result_t check_i2c_connection(struct sensor_module_t *module)
{
	uint8_t id_h, id_l = 0;

	SENSOR_LOG_INFO
		("-> %s, id %d, bus %u slave add: 0x%x\n",
		__func__, module->id, module->hw->sensor_i2c_index,
		GROUP0_I2C_SLAVE_ADDR);
	SENSOR_I2C_READ(module->hw->sensor_i2c_index, 0x0000, &id_h);
	SENSOR_I2C_READ(module->hw->sensor_i2c_index, 0x0001, &id_l);
	if ((id_h == 0x02) && (id_l == 0x08)) {
		SENSOR_LOG_INFO("id match 0x0208\n");
	} else {
		SENSOR_LOG_INFO
			("id mismatch, Reg(0x0000)=0x%x, Reg(0x0001)=0x%x\n",
			id_h, id_l);
		return RET_FAILURE;
	}
	return RET_SUCCESS;
}

static result_t start_phy_imx208(struct sensor_module_t *module,
				 uint32_t bitrate)
{
	uint32 reg_val = 0;
	uint32 temp_reg_val = 0;
	uint32 ui_4i = 0;
	uint8 bus = module->hw->sensor_phy_i2c_index;

	SENSOR_LOG_INFO("-> %s, id %d, bus %u, bitrate %u\n",  __func__,
			module->id, bus, bitrate);

	SENSOR_PHY_I2C_READ(bus, 0x02, &reg_val);

//#if 1
	// one lane setting
//#if 1
	// enable pixel decomparess
	SENSOR_PHY_I2C_WRITE(bus, 0x14, 0x00);
//#else
	// bypass pixel generation and decomparession
	//SENSOR_PHY_I2C_WRITE(bus, 0x14, 0x40);
//#endif
	SENSOR_PHY_I2C_WRITE(bus, 0x15, 0x00);
	SENSOR_PHY_I2C_WRITE(bus, 0x36, 0x20);
	SENSOR_PHY_I2C_WRITE(bus, 0x06, 0x01);
	SENSOR_PHY_I2C_WRITE(bus, 0x05, 0x03);
	SENSOR_PHY_I2C_WRITE(bus, 0x04, 0x02);
//#else
	// two lanes setting
/*
 *	SENSOR_PHY_I2C_WRITE(bus, 0x14, 0x42);
 *	SENSOR_PHY_I2C_WRITE(bus, 0x15, 0x00);
 *	SENSOR_PHY_I2C_WRITE(bus, 0x36, 0x20);
 *	SENSOR_PHY_I2C_WRITE(bus, 0x06, 0x01);
 *	SENSOR_PHY_I2C_WRITE(bus, 0x05, 0x03);
 *	SENSOR_PHY_I2C_WRITE(bus, 0x0A, 0x01);
 *	SENSOR_PHY_I2C_WRITE(bus, 0x09, 0x01);
 *	SENSOR_PHY_I2C_WRITE(bus, 0x04, 0x02);
 */
//#endif

	//The timing setting
	ui_4i = 1 * 2 * 1000 * 1000 * 4;
	ui_4i /= (uint32) (bitrate * 1000);
	ui_4i &= 0x3f;
	reg_val = (ui_4i << 2) | 0x1;
	SENSOR_LOG_INFO("%s timing setting = 0x%x\n", __func__, reg_val);
	//reg_val = 0x29;//400Mbpps
	//reg_val = 0x21;//480Mbpps
	//reg_val = 0x19;//600Mbpps
	//reg_val = 0x15;//800Mbpps

	SENSOR_PHY_I2C_WRITE(bus, 0x02, reg_val);
	SENSOR_PHY_I2C_READ(bus, 0x02, &temp_reg_val);
	SENSOR_LOG_INFO("%s temp_reg_val = 0x%x\n", __func__,
		temp_reg_val);

	if (temp_reg_val != reg_val) {
		SENSOR_LOG_ERROR("STD phy programming fail\n");
		return RET_FAILURE;
	}

	SENSOR_PHY_I2C_READ(bus, 0x02, &temp_reg_val);
	SENSOR_LOG_ERROR("Phy Reg_0x02 = 0x%x\n", temp_reg_val);
	SENSOR_PHY_I2C_READ(bus, 0x14, &temp_reg_val);
	SENSOR_LOG_ERROR("Phy Reg_0x14 = 0x%x\n", temp_reg_val);
	SENSOR_PHY_I2C_READ(bus, 0x15, &temp_reg_val);
	SENSOR_LOG_ERROR("Phy Reg_0x15 = 0x%x\n", temp_reg_val);
	SENSOR_PHY_I2C_READ(bus, 0x36, &temp_reg_val);
	SENSOR_LOG_ERROR("Phy Reg_0x36 = 0x%x\n", temp_reg_val);
	SENSOR_PHY_I2C_READ(bus, 0x06, &temp_reg_val);
	SENSOR_LOG_ERROR("Phy Reg_0x06 = 0x%x\n", temp_reg_val);
	SENSOR_PHY_I2C_READ(bus, 0x05, &temp_reg_val);
	SENSOR_LOG_ERROR("Phy Reg_0x05 = 0x%x\n", temp_reg_val);
	SENSOR_PHY_I2C_READ(bus, 0x0a, &temp_reg_val);
	SENSOR_LOG_ERROR("Phy Reg_0x0a = 0x%x\n", temp_reg_val);
	SENSOR_PHY_I2C_READ(bus, 0x09, &temp_reg_val);
	SENSOR_LOG_ERROR("Phy Reg_0x09 = 0x%x\n", temp_reg_val);
	SENSOR_PHY_I2C_READ(bus, 0x04, &temp_reg_val);
	SENSOR_LOG_ERROR("Phy Reg_0x04 = 0x%x\n", temp_reg_val);
	SENSOR_PHY_I2C_READ(bus, 0x02, &temp_reg_val);
	SENSOR_LOG_ERROR("Phy Reg_0x02 = 0x%x\n", temp_reg_val);

	SENSOR_LOG_ERROR("============status============\n");
	SENSOR_PHY_I2C_READ(bus, 0x01, &temp_reg_val);
	SENSOR_LOG_ERROR("Phy Reg_0x01 = 0x%x\n", temp_reg_val);
	SENSOR_PHY_I2C_READ(bus, 0x07, &temp_reg_val);
	SENSOR_LOG_ERROR("Phy Reg_0x07 = 0x%x\n", temp_reg_val);
	SENSOR_PHY_I2C_READ(bus, 0x0c, &temp_reg_val);
	SENSOR_LOG_ERROR("Phy Reg_0x0c = 0x%x\n", temp_reg_val);
	SENSOR_PHY_I2C_READ(bus, 0x08, &temp_reg_val);
	SENSOR_LOG_ERROR("Phy Reg_0x08 = 0x%x\n", temp_reg_val);
	SENSOR_PHY_I2C_READ(bus, 0x0b, &temp_reg_val);
	SENSOR_LOG_ERROR("Phy Reg_0x0b = 0x%x\n", temp_reg_val);
	SENSOR_PHY_I2C_READ(bus, 0x3a, &temp_reg_val);
	SENSOR_LOG_ERROR("Phy Reg_0x3a = 0x%x\n", temp_reg_val);
	SENSOR_PHY_I2C_READ(bus, 0x3b, &temp_reg_val);
	SENSOR_LOG_ERROR("Phy Reg_0x3b = 0x%x\n", temp_reg_val);
	SENSOR_PHY_I2C_READ(bus, 0x0f, &temp_reg_val);
	SENSOR_LOG_ERROR("Phy Reg_0x0f = 0x%x\n", temp_reg_val);
	SENSOR_PHY_I2C_READ(bus, 0x10, &temp_reg_val);
	SENSOR_LOG_ERROR("Phy Reg_0x10 = 0x%x\n", temp_reg_val);

	SENSOR_LOG_INFO("<- %s, success\n", __func__);

	return RET_SUCCESS;
}

/*****************************************************************************
 *	1920 x 1080 @ 15fps (Max) 1 lanes
 *	frame_length_lines = 1200(unit: lines)
 *	line_length_pixel_clocks = 2248(unit: pixel_clocks)
 *	pixel_clock_per_frame: 40500000(unit: pixel_clocks)
 *	Max fps = 40500000/(1200*2248) = 15
 *	time_per_line(unit: seconds) = 2248 / 40500000(second)
 ****************************************************************************/
static result_t res_0_config_imx208(struct sensor_module_t *module)
{
	uint8 bus = module->hw->sensor_i2c_index;

	SENSOR_LOG_INFO("-> %s, device id %d\n", __func__, module->id);

	//PLL Setting
#ifdef SENSOR_CLK_20M
	//20M 15fps20 / 4 * 162 / 2 = 405 M
	SENSOR_I2C_WRITE(bus, 0x0305, 0x04);	//prepllck_div		4
	SENSOR_I2C_WRITE(bus, 0x0307, 0xA2);	//pll_mpy		162
	SENSOR_I2C_WRITE(bus, 0x303C, 0x3E);	//plstatim
	//62.33 = ((75-56)/(24-18))*(20-18)+56(20M)
	SENSOR_I2C_WRITE(bus, 0x30A4, 0x00);	//rgpltd		1/2
#else
	//24M 15fps24 / 4 * 135 / 2 = 405 M
	SENSOR_I2C_WRITE(bus, 0x0305, 0x04);	//prepllck_div		4
	SENSOR_I2C_WRITE(bus, 0x0307, 0x87);	//pll_mpy		162
	SENSOR_I2C_WRITE(bus, 0x303C, 0x4b);	//plstatim		75(24M)
	SENSOR_I2C_WRITE(bus, 0x30A4, 0x00);	//rgpltd		1/2
#endif
	//Mode Setting
	SENSOR_I2C_WRITE(bus, 0x0112, 0x0A);	//CCP_DT_FMT[15:8]
	SENSOR_I2C_WRITE(bus, 0x0113, 0x0A);	//CCP_DT_FMT[7:0]
	SENSOR_I2C_WRITE(bus, 0x0340, 0x04);	//frame_length_lines[15:8]
	SENSOR_I2C_WRITE(bus, 0x0341, 0xB0);	//frame_length_lines[7:0]
	SENSOR_I2C_WRITE(bus, 0x0342, 0x08);	//line_length_pck[15:8]
	SENSOR_I2C_WRITE(bus, 0x0343, 0xC8);	//line_length_pck[7:0]
	SENSOR_I2C_WRITE(bus, 0x0344, 0x00);	//x_addr_start[15:8]
	SENSOR_I2C_WRITE(bus, 0x0345, 0x08);	//x_addr_start[7:0]
	SENSOR_I2C_WRITE(bus, 0x0346, 0x00);	//y_addr_start[15:8]
	SENSOR_I2C_WRITE(bus, 0x0347, 0x08);	//y_addr_start[7:0]
	SENSOR_I2C_WRITE(bus, 0x0348, 0x07);	//x_addr_end[15:8]
	SENSOR_I2C_WRITE(bus, 0x0349, 0x87);	//x_addr_end[7:0]
	SENSOR_I2C_WRITE(bus, 0x034A, 0x04);	//y_addr_end[15:8]
	SENSOR_I2C_WRITE(bus, 0x034B, 0x3f);	//y_addr_end[7:0]
	SENSOR_I2C_WRITE(bus, 0x034C, 0x07);	//x_output_size[15:8]
	SENSOR_I2C_WRITE(bus, 0x034D, 0x80);	//x_output_size[7:0]
	SENSOR_I2C_WRITE(bus, 0x034E, 0x04);	//y_output_size[15:8]
	SENSOR_I2C_WRITE(bus, 0x034F, 0x38);	//y_output_size[7:0]
	SENSOR_I2C_WRITE(bus, 0x0381, 0x01);	//x_even_inc[3:0]
	SENSOR_I2C_WRITE(bus, 0x0383, 0x01);	//x_odd_inc[3:0]
	SENSOR_I2C_WRITE(bus, 0x0385, 0x01);	//y_even_inc[3:0]
	SENSOR_I2C_WRITE(bus, 0x0387, 0x01);	//y_odd_inc[3:0]
	SENSOR_I2C_WRITE(bus, 0x3048, 0x00);	//VMODEFDS
	SENSOR_I2C_WRITE(bus, 0x304E, 0x0A);	//VTPXCK_DIV
	SENSOR_I2C_WRITE(bus, 0x3050, 0x02);	//OPSYCK_DIV
	SENSOR_I2C_WRITE(bus, 0x309B, 0x00);	//RGDAFDSUMEN
	SENSOR_I2C_WRITE(bus, 0x30D5, 0x00);	//HADDEN,HADDMODE
	SENSOR_I2C_WRITE(bus, 0x3301, 0x01);	//RGLANESEL
	SENSOR_I2C_WRITE(bus, 0x3318, 0x66);	//MIPI Global Timing Table

	//Shutter Gain Setting
	SENSOR_I2C_WRITE(bus, 0x0202, 0x01);	//coarse_integration_time[15:8]
	SENSOR_I2C_WRITE(bus, 0x0203, 0x90);	//coarse_integration_time[7:0]
	SENSOR_I2C_WRITE(bus, 0x0205, 0x00);	//ana_gain_global[7:0]

	//TEST PARTTEN
/*
 *	SENSOR_I2C_WRITE(bus, 0x3032, 0x40);
 *	SENSOR_I2C_WRITE(bus, 0x3282, 0x01);
 *	SENSOR_I2C_WRITE(bus, 0x0600, 0x00);
 *	SENSOR_I2C_WRITE(bus, 0x0601, 0x02);
 *
 *	SENSOR_I2C_WRITE(bus, 0x0602, 0x00);
 *	SENSOR_I2C_WRITE(bus, 0x0603, 0x00);
 *	SENSOR_I2C_WRITE(bus, 0x0604, 0x00);
 *	SENSOR_I2C_WRITE(bus, 0x0605, 0x40);
 *	SENSOR_I2C_WRITE(bus, 0x0606, 0x00);
 *	SENSOR_I2C_WRITE(bus, 0x0607, 0x00);
 *	SENSOR_I2C_WRITE(bus, 0x0608, 0x00);
 *	SENSOR_I2C_WRITE(bus, 0x0609, 0x40);
 */
	//Streaming
	SENSOR_I2C_WRITE(bus, 0x0100, 0x01);

	SENSOR_LOG_INFO("<- %s, success\n", __func__);
	return RET_SUCCESS;
}

/*****************************************************************************
 *	1936 x 1096 @ 30fps (Max) 1 lanes
 *	frame_length_lines = 1200(unit: lines)
 *	line_length_pixel_clocks = 2248(unit: pixel_clocks)
 *	pixel_clock_per_frame: 8100000(unit: pixel_clocks)
 *	Max fps = 8100000/(1200*2248) = 30
 *	time_per_line(unit: seconds) = 2248 / 8100000(second)
 ****************************************************************************/
static result_t res_1_config_imx208(struct sensor_module_t *module)
{
	uint8 bus = module->hw->sensor_i2c_index;

	SENSOR_LOG_INFO("-> %s, device id %d\n", __func__, module->id);

	//PLL Setting
	//24M 30fps24 / 4 * 135 / 1 = 810 M
	SENSOR_I2C_WRITE(bus, 0x0305, 0x04);	//prepllck_div		4
	SENSOR_I2C_WRITE(bus, 0x0307, 0x87);	//pll_mpy		162
	SENSOR_I2C_WRITE(bus, 0x303C, 0x4b);	//plstatim		75(24M)
	SENSOR_I2C_WRITE(bus, 0x30A4, 0x02);	//rgpltd		1

	//Mode Setting
	SENSOR_I2C_WRITE(bus, 0x0112, 0x0A);	//CCP_DT_FMT[15:8]
	SENSOR_I2C_WRITE(bus, 0x0113, 0x0A);	//CCP_DT_FMT[7:0]
	SENSOR_I2C_WRITE(bus, 0x0340, 0x04);	//frame_length_lines[15:8]
	SENSOR_I2C_WRITE(bus, 0x0341, 0xB0);	//frame_length_lines[7:0]
	SENSOR_I2C_WRITE(bus, 0x0342, 0x08);	//line_length_pck[15:8]
	SENSOR_I2C_WRITE(bus, 0x0343, 0xC8);	//line_length_pck[7:0]
	SENSOR_I2C_WRITE(bus, 0x0344, 0x00);	//x_addr_start[15:8]
	SENSOR_I2C_WRITE(bus, 0x0345, 0x00);	//x_addr_start[7:0]
	SENSOR_I2C_WRITE(bus, 0x0346, 0x00);	//y_addr_start[15:8]
	SENSOR_I2C_WRITE(bus, 0x0347, 0x00);	//y_addr_start[7:0]
	SENSOR_I2C_WRITE(bus, 0x0348, 0x07);	//x_addr_end[15:8]
	SENSOR_I2C_WRITE(bus, 0x0349, 0x8F);	//x_addr_end[7:0]
	SENSOR_I2C_WRITE(bus, 0x034A, 0x04);	//y_addr_end[15:8]
	SENSOR_I2C_WRITE(bus, 0x034B, 0x47);	//y_addr_end[7:0]
	SENSOR_I2C_WRITE(bus, 0x034C, 0x07);	//x_output_size[15:8]
	SENSOR_I2C_WRITE(bus, 0x034D, 0x90);	//x_output_size[7:0]
	SENSOR_I2C_WRITE(bus, 0x034E, 0x04);	//y_output_size[15:8]
	SENSOR_I2C_WRITE(bus, 0x034F, 0x48);	//y_output_size[7:0]
	SENSOR_I2C_WRITE(bus, 0x0381, 0x01);	//x_even_inc[3:0]
	SENSOR_I2C_WRITE(bus, 0x0383, 0x01);	//x_odd_inc[3:0]
	SENSOR_I2C_WRITE(bus, 0x0385, 0x01);	//y_even_inc[3:0]
	SENSOR_I2C_WRITE(bus, 0x0387, 0x01);	//y_odd_inc[3:0]
	SENSOR_I2C_WRITE(bus, 0x3048, 0x00);	//VMODEFDS
	SENSOR_I2C_WRITE(bus, 0x304E, 0x0A);	//VTPXCK_DIV
	SENSOR_I2C_WRITE(bus, 0x3050, 0x02);	//OPSYCK_DIV
	SENSOR_I2C_WRITE(bus, 0x309B, 0x00);	//RGDAFDSUMEN
	SENSOR_I2C_WRITE(bus, 0x30D5, 0x00);	//HADDEN,HADDMODE
	SENSOR_I2C_WRITE(bus, 0x3301, 0x01);	//RGLANESEL
	SENSOR_I2C_WRITE(bus, 0x3318, 0x61);	//MIPI Global Timing Table

	//Shutter Gain Setting
	SENSOR_I2C_WRITE(bus, 0x0202, 0x01);	//coarse_integration_time[15:8]
	SENSOR_I2C_WRITE(bus, 0x0203, 0x90);	//coarse_integration_time[7:0]
	SENSOR_I2C_WRITE(bus, 0x0205, 0x00);	//ana_gain_global[7:0]

	//Streaming
	SENSOR_I2C_WRITE(bus, 0x0100, 0x01);
	SENSOR_LOG_INFO("<- %s, success\n", __func__);
	return RET_SUCCESS;
}

/*****************************************************************************
 *	1920 x 1080 @ 60fps (Max) 2 lanes
 *	frame_length_lines = 1200(unit: lines)
 *	line_length_pixel_clocks = 2248(unit: pixel_clocks)
 *	pixel_clock_per_frame: 16200000(unit: pixel_clocks)
 *	Max fps = 16200000/(1200*2248) = 60
 *	time_per_line(unit: seconds) = 2248 / 16200000(second)
 ****************************************************************************/
static result_t res_2_config_imx208(struct sensor_module_t *module)
{
	uint8 bus = module->hw->sensor_i2c_index;

	SENSOR_LOG_INFO("-> %s, device id %d\n", __func__, module->id);

	//PLL Setting
	//24M 30fps24 / 4 * 135 / 1 = 810 M
	SENSOR_I2C_WRITE(bus, 0x0305, 0x04);	//prepllck_div		4
	SENSOR_I2C_WRITE(bus, 0x0307, 0x87);	//pll_mpy		162
	SENSOR_I2C_WRITE(bus, 0x303C, 0x4b);	//plstatim		75(24M)
	SENSOR_I2C_WRITE(bus, 0x30A4, 0x02);	//rgpltd		1

	//Mode Setting
	SENSOR_I2C_WRITE(bus, 0x0112, 0x0A);	//CCP_DT_FMT[15:8]
	SENSOR_I2C_WRITE(bus, 0x0113, 0x0A);	//CCP_DT_FMT[7:0]
	SENSOR_I2C_WRITE(bus, 0x0340, 0x04);	//frame_length_lines[15:8]
	SENSOR_I2C_WRITE(bus, 0x0341, 0xB0);	//frame_length_lines[7:0]
	SENSOR_I2C_WRITE(bus, 0x0342, 0x04);	//line_length_pck[15:8]
	SENSOR_I2C_WRITE(bus, 0x0343, 0x64);	//line_length_pck[7:0]
	SENSOR_I2C_WRITE(bus, 0x0344, 0x00);	//x_addr_start[15:8]
	SENSOR_I2C_WRITE(bus, 0x0345, 0x08);	//x_addr_start[7:0]
	SENSOR_I2C_WRITE(bus, 0x0346, 0x00);	//y_addr_start[15:8]
	SENSOR_I2C_WRITE(bus, 0x0347, 0x08);	//y_addr_start[7:0]
	SENSOR_I2C_WRITE(bus, 0x0348, 0x07);	//x_addr_end[15:8]
	SENSOR_I2C_WRITE(bus, 0x0349, 0x87);	//x_addr_end[7:0]
	SENSOR_I2C_WRITE(bus, 0x034A, 0x04);	//y_addr_end[15:8]
	SENSOR_I2C_WRITE(bus, 0x034B, 0x3f);	//y_addr_end[7:0]
	SENSOR_I2C_WRITE(bus, 0x034C, 0x07);	//x_output_size[15:8]
	SENSOR_I2C_WRITE(bus, 0x034D, 0x80);	//x_output_size[7:0]
	SENSOR_I2C_WRITE(bus, 0x034E, 0x04);	//y_output_size[15:8]
	SENSOR_I2C_WRITE(bus, 0x034F, 0x38);	//y_output_size[7:0]
	SENSOR_I2C_WRITE(bus, 0x0381, 0x01);	//x_even_inc[3:0]
	SENSOR_I2C_WRITE(bus, 0x0383, 0x01);	//x_odd_inc[3:0]
	SENSOR_I2C_WRITE(bus, 0x0385, 0x01);	//y_even_inc[3:0]
	SENSOR_I2C_WRITE(bus, 0x0387, 0x01);	//y_odd_inc[3:0]
	SENSOR_I2C_WRITE(bus, 0x3048, 0x00);	//VMODEFDS
	SENSOR_I2C_WRITE(bus, 0x304E, 0x0A);	//VTPXCK_DIV
	SENSOR_I2C_WRITE(bus, 0x3050, 0x01);	//OPSYCK_DIV
	SENSOR_I2C_WRITE(bus, 0x309B, 0x00);	//RGDAFDSUMEN
	SENSOR_I2C_WRITE(bus, 0x30D5, 0x00);	//HADDEN,HADDMODE
	SENSOR_I2C_WRITE(bus, 0x3301, 0x00);	//RGLANESEL
	SENSOR_I2C_WRITE(bus, 0x3318, 0x61);	//MIPI Global Timing Table

	//Shutter Gain Setting
	SENSOR_I2C_WRITE(bus, 0x0202, 0x01);	//coarse_integration_time[15:8]
	SENSOR_I2C_WRITE(bus, 0x0203, 0x90);	//coarse_integration_time[7:0]
	SENSOR_I2C_WRITE(bus, 0x0205, 0x00);	//ana_gain_global[7:0]

	//Streaming
	SENSOR_I2C_WRITE(bus, 0x0100, 0x01);
	SENSOR_LOG_INFO("<- %s, success\n",  __func__);
	return RET_SUCCESS;
}

/*****************************************************************************
 *	640 x 480 @ 120fps (Max) 2 lanes
 *	frame_length_lines = 600(unit: lines)
 *	line_length_pixel_clocks = 1124(unit: pixel_clocks)
 *	pixel_clock_per_frame: 8100000(unit: pixel_clocks)
 *	Max fps = 8100000/(600*1124) = 120
 *	time_per_line(unit: seconds) = 1124 / 8100000(second)
 ****************************************************************************/
static result_t res_3_config_imx208(struct sensor_module_t *module)
{
	uint8 bus = module->hw->sensor_i2c_index;

	SENSOR_LOG_INFO("-> %s, device id %d\n", __func__, module->id);

	//PLL Setting
	//24M 30fps24 / 4 * 135 / 1 = 810 M
	SENSOR_I2C_WRITE(bus, 0x0305, 0x04);	//prepllck_div		4
	SENSOR_I2C_WRITE(bus, 0x0307, 0x87);	//pll_mpy		162
	SENSOR_I2C_WRITE(bus, 0x303C, 0x4b);	//plstatim		75(24M)
	SENSOR_I2C_WRITE(bus, 0x30A4, 0x01);	//rgpltd		1

	//Mode Setting
	SENSOR_I2C_WRITE(bus, 0x0112, 0x0A);	//CCP_DT_FMT[15:8]
	SENSOR_I2C_WRITE(bus, 0x0113, 0x0A);	//CCP_DT_FMT[7:0]
	SENSOR_I2C_WRITE(bus, 0x0340, 0x02);	//frame_length_lines[15:8]
	SENSOR_I2C_WRITE(bus, 0x0341, 0x58);	//frame_length_lines[7:0]
	SENSOR_I2C_WRITE(bus, 0x0342, 0x04);	//line_length_pck[15:8]
	SENSOR_I2C_WRITE(bus, 0x0343, 0x64);	//line_length_pck[7:0]
	SENSOR_I2C_WRITE(bus, 0x0344, 0x01);	//x_addr_start[15:8]
	SENSOR_I2C_WRITE(bus, 0x0345, 0x48);	//x_addr_start[7:0]
	SENSOR_I2C_WRITE(bus, 0x0346, 0x00);	//y_addr_start[15:8]
	SENSOR_I2C_WRITE(bus, 0x0347, 0x44);	//y_addr_start[7:0]
	SENSOR_I2C_WRITE(bus, 0x0348, 0x03);	//x_addr_end[15:8]
	SENSOR_I2C_WRITE(bus, 0x0349, 0xc7);	//x_addr_end[7:0]
	SENSOR_I2C_WRITE(bus, 0x034A, 0x02);	//y_addr_end[15:8]
	SENSOR_I2C_WRITE(bus, 0x034B, 0x23);	//y_addr_end[7:0]
	SENSOR_I2C_WRITE(bus, 0x034C, 0x02);	//x_output_size[15:8]
	SENSOR_I2C_WRITE(bus, 0x034D, 0x80);	//x_output_size[7:0]
	SENSOR_I2C_WRITE(bus, 0x034E, 0x01);	//y_output_size[15:8]
	SENSOR_I2C_WRITE(bus, 0x034F, 0xe0);	//y_output_size[7:0]
	SENSOR_I2C_WRITE(bus, 0x0381, 0x01);	//x_even_inc[3:0]
	SENSOR_I2C_WRITE(bus, 0x0383, 0x01);	//x_odd_inc[3:0]
	SENSOR_I2C_WRITE(bus, 0x0385, 0x01);	//y_even_inc[3:0]
	SENSOR_I2C_WRITE(bus, 0x0387, 0x01);	//y_odd_inc[3:0]
	SENSOR_I2C_WRITE(bus, 0x3048, 0x00);	//VMODEFDS
	SENSOR_I2C_WRITE(bus, 0x304E, 0x0A);	//VTPXCK_DIV
	SENSOR_I2C_WRITE(bus, 0x3050, 0x01);	//OPSYCK_DIV
	SENSOR_I2C_WRITE(bus, 0x309B, 0x00);	//RGDAFDSUMEN
	SENSOR_I2C_WRITE(bus, 0x30D5, 0x00);	//HADDEN,HADDMODE
	SENSOR_I2C_WRITE(bus, 0x3301, 0x00);	//RGLANESEL
	SENSOR_I2C_WRITE(bus, 0x3318, 0x6a);	//MIPI Global Timing Table

	//Shutter Gain Setting
	SENSOR_I2C_WRITE(bus, 0x0202, 0x01);	//coarse_integration_time[15:8]
	SENSOR_I2C_WRITE(bus, 0x0203, 0x90);	//coarse_integration_time[7:0]
	SENSOR_I2C_WRITE(bus, 0x0205, 0x00);	//ana_gain_global[7:0]

	//Streaming
	SENSOR_I2C_WRITE(bus, 0x0100, 0x01);
	SENSOR_LOG_INFO("<- %s, success\n", __func__);
	return RET_SUCCESS;
}

/*****************************************************************************
 *	320 x 240 @ 240fps (Max) 2 lanes
 *	frame_length_lines = 300(unit: lines)
 *	line_length_pixel_clocks = 1124(unit: pixel_clocks)
 *	pixel_clock_per_frame: 8100000(unit: pixel_clocks)
 *	Max fps = 8100000/(300*1124) = 240
 *	time_per_line(unit: seconds) = 1124 / 8100000(second)
 ****************************************************************************/
static result_t res_4_config_imx208(struct sensor_module_t *module)
{
	uint8 bus = module->hw->sensor_i2c_index;

	SENSOR_LOG_INFO("-> %s, device id %d\n", __func__, module->id);

	//PLL Setting
	//24M 30fps24 / 4 * 135 / 1 = 810 M
	SENSOR_I2C_WRITE(bus, 0x0305, 0x04);	//prepllck_div		4
	SENSOR_I2C_WRITE(bus, 0x0307, 0x87);	//pll_mpy		162
	SENSOR_I2C_WRITE(bus, 0x303C, 0x4b);	//plstatim		75(24M)
	SENSOR_I2C_WRITE(bus, 0x30A4, 0x02);	//rgpltd		1

	//Mode Setting
	SENSOR_I2C_WRITE(bus, 0x0112, 0x0A);	//CCP_DT_FMT[15:8]
	SENSOR_I2C_WRITE(bus, 0x0113, 0x0A);	//CCP_DT_FMT[7:0]
	SENSOR_I2C_WRITE(bus, 0x0340, 0x01);	//frame_length_lines[15:8]
	SENSOR_I2C_WRITE(bus, 0x0341, 0x2c);	//frame_length_lines[7:0]
	SENSOR_I2C_WRITE(bus, 0x0342, 0x04);	//line_length_pck[15:8]
	SENSOR_I2C_WRITE(bus, 0x0343, 0x64);	//line_length_pck[7:0]
	SENSOR_I2C_WRITE(bus, 0x0344, 0x01);	//x_addr_start[15:8]
	SENSOR_I2C_WRITE(bus, 0x0345, 0x48);	//x_addr_start[7:0]
	SENSOR_I2C_WRITE(bus, 0x0346, 0x00);	//y_addr_start[15:8]
	SENSOR_I2C_WRITE(bus, 0x0347, 0x44);	//y_addr_start[7:0]
	SENSOR_I2C_WRITE(bus, 0x0348, 0x06);	//x_addr_end[15:8]
	SENSOR_I2C_WRITE(bus, 0x0349, 0x47);	//x_addr_end[7:0]
	SENSOR_I2C_WRITE(bus, 0x034A, 0x04);	//y_addr_end[15:8]
	SENSOR_I2C_WRITE(bus, 0x034B, 0x03);	//y_addr_end[7:0]
	SENSOR_I2C_WRITE(bus, 0x034C, 0x01);	//x_output_size[15:8]
	SENSOR_I2C_WRITE(bus, 0x034D, 0x40);	//x_output_size[7:0]
	SENSOR_I2C_WRITE(bus, 0x034E, 0x00);	//y_output_size[15:8]
	SENSOR_I2C_WRITE(bus, 0x034F, 0xf0);	//y_output_size[7:0]
	SENSOR_I2C_WRITE(bus, 0x0381, 0x03);	//x_even_inc[3:0]
	SENSOR_I2C_WRITE(bus, 0x0383, 0x05);	//x_odd_inc[3:0]
	SENSOR_I2C_WRITE(bus, 0x0385, 0x03);	//y_even_inc[3:0]
	SENSOR_I2C_WRITE(bus, 0x0387, 0x05);	//y_odd_inc[3:0]
	SENSOR_I2C_WRITE(bus, 0x3048, 0x01);	//VMODEFDS
	SENSOR_I2C_WRITE(bus, 0x304E, 0x0A);	//VTPXCK_DIV
	SENSOR_I2C_WRITE(bus, 0x3050, 0x01);	//OPSYCK_DIV
	SENSOR_I2C_WRITE(bus, 0x309B, 0x00);	//RGDAFDSUMEN
	SENSOR_I2C_WRITE(bus, 0x30D5, 0x03);	//HADDEN,HADDMODE
	SENSOR_I2C_WRITE(bus, 0x3301, 0x00);	//RGLANESEL
	SENSOR_I2C_WRITE(bus, 0x3318, 0x74);	//MIPI Global Timing Table

	//Shutter Gain Setting
	SENSOR_I2C_WRITE(bus, 0x0202, 0x01);	//coarse_integration_time[15:8]
	SENSOR_I2C_WRITE(bus, 0x0203, 0x90);	//coarse_integration_time[7:0]
	SENSOR_I2C_WRITE(bus, 0x0205, 0x00);	//ana_gain_global[7:0]

	//Streaming
	SENSOR_I2C_WRITE(bus, 0x0100, 0x01);
	SENSOR_LOG_INFO("<- %s, success\n", __func__);
	return RET_SUCCESS;
}

/*****************************************************************************
 *	640 x 480 @ 120fps (Max) 2 lanes  binning mode
 *	frame_length_lines = 600(unit: lines)
 *	line_length_pixel_clocks = 1124(unit: pixel_clocks)
 *	pixel_clock_per_frame: 8100000(unit: pixel_clocks)
 *	Max fps = 8100000/(600*1124) = 120
 *	time_per_line(unit: seconds) = 1124 / 8100000(second)
 ****************************************************************************/
static result_t res_5_config_imx208(struct sensor_module_t *module)
{
	uint8 bus = module->hw->sensor_i2c_index;

	SENSOR_LOG_ERROR("-> %s, device id %d\n",
		__func__, module->id);


	//PLL Setting
	//24M 30fps  24 / 4 * 135 / 1 = 810 M
	SENSOR_I2C_WRITE(bus, 0x0305, 0x04);//prepllck_div	4
	SENSOR_I2C_WRITE(bus, 0x0307, 0x87);//pll_mpy		162
	SENSOR_I2C_WRITE(bus, 0x303C, 0x4b);//plstatim		75	(24M)
	SENSOR_I2C_WRITE(bus, 0x30A4, 0x01);//rgpltd			1

	//Mode Setting
	SENSOR_I2C_WRITE(bus, 0x0112, 0x0A);//CCP_DT_FMT[15:8]
	SENSOR_I2C_WRITE(bus, 0x0113, 0x0A);//CCP_DT_FMT[7:0]
	SENSOR_I2C_WRITE(bus, 0x0340, 0x02);//frame_length_lines[15:8]
	SENSOR_I2C_WRITE(bus, 0x0341, 0x58);//frame_length_lines[7:0]
	SENSOR_I2C_WRITE(bus, 0x0342, 0x04);//line_length_pck[15:8]
	SENSOR_I2C_WRITE(bus, 0x0343, 0x64);//line_length_pck[7:0]
	SENSOR_I2C_WRITE(bus, 0x0344, 0x01);//x_addr_start[15:8]
	SENSOR_I2C_WRITE(bus, 0x0345, 0x48);//x_addr_start[7:0]
	SENSOR_I2C_WRITE(bus, 0x0346, 0x00);//y_addr_start[15:8]
	SENSOR_I2C_WRITE(bus, 0x0347, 0x44);//y_addr_start[7:0]
	SENSOR_I2C_WRITE(bus, 0x0348, 0x06);//x_addr_end[15:8]
	SENSOR_I2C_WRITE(bus, 0x0349, 0x47);//x_addr_end[7:0]
	SENSOR_I2C_WRITE(bus, 0x034A, 0x04);//y_addr_end[15:8]
	SENSOR_I2C_WRITE(bus, 0x034B, 0x03);//y_addr_end[7:0]
	SENSOR_I2C_WRITE(bus, 0x034C, 0x02);//x_output_size[15:8]
	SENSOR_I2C_WRITE(bus, 0x034D, 0x80);//x_output_size[7:0]
	SENSOR_I2C_WRITE(bus, 0x034E, 0x01);//y_output_size[15:8]
	SENSOR_I2C_WRITE(bus, 0x034F, 0xe0);//y_output_size[7:0]
	SENSOR_I2C_WRITE(bus, 0x0381, 0x01);//x_even_inc[3:0]
	SENSOR_I2C_WRITE(bus, 0x0383, 0x03);//x_odd_inc[3:0]
	SENSOR_I2C_WRITE(bus, 0x0385, 0x01);//y_even_inc[3:0]
	SENSOR_I2C_WRITE(bus, 0x0387, 0x03);//y_odd_inc[3:0]
	SENSOR_I2C_WRITE(bus, 0x3048, 0x01);//VMODEFDS
	SENSOR_I2C_WRITE(bus, 0x304E, 0x0A);//VTPXCK_DIV
	SENSOR_I2C_WRITE(bus, 0x3050, 0x01);//OPSYCK_DIV
	SENSOR_I2C_WRITE(bus, 0x309B, 0x00);//RGDAFDSUMEN
	SENSOR_I2C_WRITE(bus, 0x30D5, 0x03);//HADDEN,HADDMODE
	SENSOR_I2C_WRITE(bus, 0x3301, 0x00);//RGLANESEL
	SENSOR_I2C_WRITE(bus, 0x3318, 0x6a);//MIPI Global Timing Table

	//Shutter Gain Setting				//
	SENSOR_I2C_WRITE(bus, 0x0202, 0x01);//coarse_integration_time[15:8]
	SENSOR_I2C_WRITE(bus, 0x0203, 0x90);//coarse_integration_time[7:0]
	SENSOR_I2C_WRITE(bus, 0x0205, 0x00);//ana_gain_global[7:0]

	//Streaming
	SENSOR_I2C_WRITE(bus, 0x0100, 0x01);
	SENSOR_LOG_INFO("<- %s, success\n", __func__);
	return RET_SUCCESS;
}

static result_t start_imx208(struct sensor_module_t *module, bool_t config_phy)
{
	struct drv_context *drv_context = &module->context;

	SENSOR_LOG_INFO("-> %s, dev_id %d, res %d, bitrate = %dMbps\n",
		__func__, module->id, drv_context->res, drv_context->bitrate);

	// Set MIPI receiver related parameters
	if (config_phy == TRUE)
		start_phy_imx208(module, drv_context->bitrate);

	// check sensor i2c connection
	if (check_i2c_connection(module) != RET_SUCCESS)
		return RET_FAILURE;
	// write new settings
	switch (drv_context->res) {
	case 0:
		res_0_config_imx208(module);
		break;
	case 1:
		res_1_config_imx208(module);
		break;
	case 2:
		res_2_config_imx208(module);
		break;
	case 3:
		res_3_config_imx208(module);
		break;
	case 4:
		res_4_config_imx208(module);
		break;
	case 5:
		res_5_config_imx208(module);
		break;
	default:
		SENSOR_LOG_ERROR("<- %s, unsupported res\n", __func__);
		return RET_FAILURE;
	}

	SENSOR_LOG_INFO("<- %s, success\n", __func__);

	return RET_SUCCESS;
}

static result_t stop_imx208(struct sensor_module_t *module)
{
	SENSOR_LOG_INFO("-> %s, device id %d\n", __func__, module->id);
	SENSOR_I2C_WRITE(module->hw->sensor_i2c_index, 0x0100, 0x00);
	SENSOR_LOG_INFO("<- %s, success\n",  __func__);

	return RET_SUCCESS;
}

static result_t stream_on_imx208(struct sensor_module_t *module)
{
	module = module;
	SENSOR_LOG_INFO("-> %s, device id %d\n", __func__, module->id);
	//SENSOR_I2C_WRITE(module->hw->sensor_i2c_index, 0x0100, 0x01);
	SENSOR_LOG_INFO("<- %s, success\n", __func__);
	return RET_SUCCESS;
}

static int32 get_res_fps_imx208(struct sensor_module_t *module,
				uint32_t res_fps_id, struct res_fps_t *res_fps)
{
	module = module;
	SENSOR_LOG_INFO("-> %s, res %d\n", __func__, res_fps_id);

	switch (res_fps_id) {
	case 0:
		res_fps->width = IMX208_RES_0_WIDTH;
		res_fps->height = IMX208_RES_0_HEIGHT;
		res_fps->fps = IMX208_RES_0_MAX_FPS;
		res_fps->none_hdr_support = 1;
		res_fps->hdr_support = 0;
		res_fps->index = res_fps_id;
		res_fps->valid = TRUE;
		break;

	case 1:
		res_fps->width = IMX208_RES_1_WIDTH;
		res_fps->height = IMX208_RES_1_HEIGHT;
		res_fps->fps = IMX208_RES_1_MAX_FPS;
		res_fps->none_hdr_support = 1;
		res_fps->hdr_support = 0;
		res_fps->index = res_fps_id;
		res_fps->valid = TRUE;
		break;
	case 2:
		res_fps->width = IMX208_RES_2_WIDTH;
		res_fps->height = IMX208_RES_2_HEIGHT;
		res_fps->fps = IMX208_RES_2_MAX_FPS;
		res_fps->none_hdr_support = 1;
		res_fps->hdr_support = 0;
		res_fps->index = res_fps_id;
		res_fps->valid = TRUE;
		break;
	case 3:
		res_fps->width = IMX208_RES_3_WIDTH;
		res_fps->height = IMX208_RES_3_HEIGHT;
		res_fps->fps = IMX208_RES_3_MAX_FPS;
		res_fps->none_hdr_support = 1;
		res_fps->hdr_support = 0;
		res_fps->index = res_fps_id;
		res_fps->valid = TRUE;
		break;
	case 4:
		res_fps->width = IMX208_RES_4_WIDTH;
		res_fps->height = IMX208_RES_4_HEIGHT;
		res_fps->fps = IMX208_RES_4_MAX_FPS;
		res_fps->none_hdr_support = 1;
		res_fps->hdr_support = 0;
		res_fps->index = res_fps_id;
		res_fps->valid = TRUE;
		break;
	case 5:
		res_fps->width = IMX208_RES_5_WIDTH;
		res_fps->height = IMX208_RES_5_HEIGHT;
		res_fps->fps = IMX208_RES_5_MAX_FPS;
		res_fps->none_hdr_support = 1;
		res_fps->hdr_support = 0;
		res_fps->index = res_fps_id;
		res_fps->valid = TRUE;
		break;
	default:
		SENSOR_LOG_ERROR("<- %s unsupported res:%d\n",
		__func__, res_fps_id);
		return RET_FAILURE;
	}

	SENSOR_LOG_INFO("<- %s, success\n", __func__);

	return RET_SUCCESS;
}

static result_t get_runtime_fps_imx208(struct sensor_module_t *module,
			uint16 hi_value, uint16 lo_value,
			float *fps)
{
	uint32_t frame_length_line;

	init_imx208_context(module->context.res);

		frame_length_line =
			((hi_value & 0xff) << 8) | (lo_value & 0xff);
		*fps = (float)((float)g_imx208_context.logic_clock /
			frame_length_line / g_imx208_context.line_length_pck);

	SENSOR_LOG_INFO
	("imx208 runtime fps = %u, Reg(0x0340)=0x%x, Reg(0x0341)=0x%x\n",
		(uint32) (*fps), hi_value, lo_value);

	return RET_SUCCESS;
}

static result_t sned_get_fralenline_cmd_imx208(struct sensor_module_t *module)
{
	uint32_t temp_h = 0;

	SENSOR_LOG_INFO("-> %s\n", __func__);
	init_imx208_context(module->context.res);

	SENSOR_I2C_READ_FW(module->id, module->hw->sensor_i2c_index, 0x0340,
			&temp_h, 2);
	//SENSOR_I2C_READ_FW(module->id, module->hw->sensor_i2c_index,
	//		0x0341, &temp_l);

	SENSOR_LOG_INFO("<- %s,suc\n", __func__);

	return RET_SUCCESS;
}

static result_t get_fralenline_regaddr_imx208(struct sensor_module_t *module,
				uint16 *hi_addr,
				uint16 *lo_addr)
{
	SENSOR_LOG_INFO("-> %s\n", __func__);
	module = module;

	*hi_addr = IMX208_FraLenLineHiAddr;
	*lo_addr = IMX208_FraLenLineLoAddr;

	SENSOR_LOG_INFO("<- %s,suc\n", __func__);
	return RET_SUCCESS;
}

static result_t get_caps_imx208(struct sensor_module_t *module,
				isp_sensor_prop_t *caps)
{
	struct drv_context *drv_context = &module->context;

	caps->window.h_offset = 0;
	caps->window.v_offset = 0;
	caps->frame_rate = 0;
	caps->mipi_bitrate = 0;

	SENSOR_LOG_INFO("-> %s, res %d\n", __func__, module->context.res);
	switch (module->context.res) {
	case 0:
		caps->window.h_size = IMX208_RES_0_WIDTH;
		caps->window.v_size = IMX208_RES_0_HEIGHT;
		caps->frame_rate =
			IMX208_RES_0_MAX_FPS * IMX208_FPS_ACCUACY_FACTOR;
		caps->prop.intfProp.mipi.numLanes = 1;
		break;
	case 1:
		caps->window.h_size = IMX208_RES_1_WIDTH;
		caps->window.v_size = IMX208_RES_1_HEIGHT;
		caps->frame_rate =
			IMX208_RES_1_MAX_FPS * IMX208_FPS_ACCUACY_FACTOR;
		caps->prop.intfProp.mipi.numLanes = 1;
		break;
	case 2:
		caps->window.h_size = IMX208_RES_2_WIDTH;
		caps->window.v_size = IMX208_RES_2_HEIGHT;
		caps->frame_rate =
			IMX208_RES_2_MAX_FPS * IMX208_FPS_ACCUACY_FACTOR;
		caps->prop.intfProp.mipi.numLanes = 2;
		break;
	case 3:
		caps->window.h_size = IMX208_RES_3_WIDTH;
		caps->window.v_size = IMX208_RES_3_HEIGHT;
		caps->frame_rate =
			IMX208_RES_3_MAX_FPS * IMX208_FPS_ACCUACY_FACTOR;
		caps->prop.intfProp.mipi.numLanes = 2;
		break;
	case 4:
		caps->window.h_size = IMX208_RES_4_WIDTH;
		caps->window.v_size = IMX208_RES_4_HEIGHT;
		caps->frame_rate =
			IMX208_RES_4_MAX_FPS * IMX208_FPS_ACCUACY_FACTOR;
		caps->prop.intfProp.mipi.numLanes = 2;
		break;
	case 5:
		caps->window.h_size = IMX208_RES_5_WIDTH;
		caps->window.v_size = IMX208_RES_5_HEIGHT;
		caps->frame_rate = IMX208_RES_5_MAX_FPS *
					IMX208_FPS_ACCUACY_FACTOR;
		caps->prop.intfProp.mipi.numLanes = 2;
		break;
	default:
		SENSOR_LOG_ERROR("<- %s unsupported res\n", __func__);
		return RET_FAILURE;
	}

#ifdef ENABLE_PPI
	caps->prop.intfType = SENSOR_INTF_TYPE_MIPI;
	caps->prop.intfProp.mipi.virtChannel = 0;
	caps->prop.intfProp.mipi.dataType = MIPI_DATA_TYPE_RAW_10;
	caps->prop.intfProp.mipi.compScheme = MIPI_COMP_SCHEME_NONE;
	caps->prop.intfProp.mipi.predBlock = MIPI_PRED_BLOCK_INVALID;
	caps->prop.cfaPattern = CFA_PATTERN_RGGB;
	caps->mipi_bitrate = drv_context->bitrate;
#else
	caps->prop.intfType = SENSOR_INTF_TYPE_PARALLEL;
	caps->prop.intfProp.parallel.dataType = PARALLEL_DATA_TYPE_RAW10;
	caps->prop.intfProp.parallel.hPol = POLARITY_HIGH;
	caps->prop.intfProp.parallel.vPol = POLARITY_LOW;
	caps->prop.intfProp.parallel.edge = SAMPLE_EDGE_NEG;
	caps->prop.intfProp.parallel.ycSeq = YCSEQUENCE_YCBYCR;
	caps->prop.cfaPattern = CFA_PATTERN_RGGB;
#endif

#ifdef SIMU_SENSOR_AS_RGBIR_SENSOR
	caps->prop.cfaPattern = CFA_PATTERN_RIGB;
#endif

	caps->prop.sensorShutterType = SENSOR_SHUTTER_TYPE_ROLLING;
	caps->prop.hasEmbeddedData = FALSE;	//Currently set to false
	caps->prop.itimeDelayFrames = 2;
	caps->prop.gainDelayFrames = 2;

	caps->exposure.min_digi_gain = (uint32_t) IMX208_MIN_DGAIN;
	caps->exposure.max_digi_gain = (uint32_t) IMX208_MAX_DGAIN;
	SENSOR_LOG_INFO("<- %s,suc\n", __func__);
	return RET_SUCCESS;
}

static result_t get_gain_limit_imx208(struct sensor_module_t *module,
			uint32_t *min_gain, uint32_t *max_gain)
{
	module = module;
	*min_gain = (uint32_t) IMX208_MIN_AGAIN;
	*max_gain = (uint32_t) IMX208_MAX_AGAIN;

	return RET_SUCCESS;
}

static result_t get_integration_time_limit_imx208(struct sensor_module_t
					*module,
					uint32_t *
					min_integration_time,
					uint32_t *
					max_integration_time)
{
	SENSOR_LOG_INFO("-> %s entry\n", __func__);

	init_imx208_context(module->context.res);
	*min_integration_time = (uint32_t) (2 * g_imx208_context.t_line);

	switch (module->context.res) {
	case 0:
		*max_integration_time = 1000000 / IMX208_RES_0_MIN_FPS;
		break;
	case 1:
		*max_integration_time = 1000000 / IMX208_RES_1_MIN_FPS;
		break;
	case 2:
		*max_integration_time = 1000000 / IMX208_RES_2_MIN_FPS;
		break;
	case 3:
		*max_integration_time = 1000000 / IMX208_RES_3_MIN_FPS;
		break;
	case 4:
		*max_integration_time = 1000000 / IMX208_RES_4_MIN_FPS;
		break;
	case 5:
		*max_integration_time = 1000000 / IMX208_RES_5_MIN_FPS;
		break;
	default:
		SENSOR_LOG_ERROR("Unsupported res:%d\n", module->context.res);
		return RET_FAILURE;
	}

	SENSOR_LOG_INFO("max integration time limit = %d(us)\n",
			*max_integration_time);
	SENSOR_LOG_INFO("<- get_itime_limit_imx208, success\n");
	return RET_SUCCESS;
}

static result_t support_af_imx208(struct sensor_module_t *module,
				uint32_t *support)
{
	module = module;
	*support = 0;

	return RET_SUCCESS;
}

static result_t get_i2c_config_imx208(struct sensor_module_t *module,
				uint32_t *i2c_addr, uint32_t *mode,
				uint32_t *reg_addr_width)
{
	module = module;
	mode = mode;

	*i2c_addr = GROUP0_I2C_SLAVE_ADDR;
	*mode = I2C_DEVICE_ADDR_TYPE_7BIT;
	*reg_addr_width = I2C_REG_ADDR_TYPE_16BIT;

	return RET_SUCCESS;
}

static result_t get_fw_script_ctrl_imx208(struct sensor_module_t *module,
				isp_fw_script_ctrl_t *script)
{
	uint32_t i = 0;

	SENSOR_LOG_INFO("-> %s, device id %d\n", __func__,
			module->id);

	// get I2C prop of the sensor
	script->i2cAe.deviceId = I2C_DEVICE_ID_INVALID;
	script->i2cAe.deviceAddr = GROUP0_I2C_SLAVE_ADDR;
	script->i2cAe.deviceAddrType = I2C_DEVICE_ADDR_TYPE_7BIT;
	script->i2cAe.regAddrType = I2C_REG_ADDR_TYPE_16BIT;
	script->i2cAe.enRestart = 1;

	//AE
	script->argAe.args[0] = module->context.t_line_numerator;
	script->argAe.args[1] = module->context.t_line_denominator;
	script->argAe.args[2] = module->context.line_length_pck;
	script->argAe.args[3] = module->context.frame_length_lines;
	script->argAe.args[4] = g_imx208_context.current_frame_length_lines;
	script->argAe.args[5] =
		g_imx208_context.max_line_count > IMX208_MAX_LINE_COUNT ?
		IMX208_MAX_LINE_COUNT : g_imx208_context.max_line_count;
	for (i = 6; i < 8; i++)
		script->argAe.args[i] = 0;
	script->argAe.numArgs = 8;
	//AF
	script->argAf.numArgs = 0;

	//Flash
	script->argFl.numArgs = 0;

	SENSOR_LOG_INFO("<- %s, success\n", __func__);

	return RET_SUCCESS;
}

static result_t get_sensor_hw_parameter_imx208(struct sensor_module_t *module,
				struct sensor_hw_parameter *para)
{
	module = module;
	SENSOR_LOG_INFO("-> %s, device id %d\n",
		__func__, module->id);
	if (para) {
		para->type = CAMERA_TYPE_RGB_BAYER;
#ifdef SIMU_SENSOR_AS_RGBIR_SENSOR
		para->type = CAMERA_TYPE_RGBIR;
#endif

		para->focus_len = 1;
		para->support_hdr = 0;
		para->min_lens_pos = 0;
		para->max_lens_pos = 0;
		para->init_lens_pos = 0;
		para->lens_step = 0;
		para->normalized_focal_len_x = 28;
		para->normalized_focal_len_y = 25;
	} else {
		SENSOR_LOG_ERROR
		("In %s, fail NULL para\n", __func__);
		return RET_FAILURE;
	}

	SENSOR_LOG_INFO("<- %s, suc\n", __func__);
	return RET_SUCCESS;
}

struct sensor_module_t g_imx208_driver = {
	.id = 0,
	.hw = NULL,
	.name = "imx208",
	.poweron_sensor = poweron_imx208,
	.reset_sensor = reset_imx208,
	.config_sensor = config_imx208,
	.start_sensor = start_imx208,
	.stop_sensor = stop_imx208,
	.stream_on_sensor = stream_on_imx208,
	.get_caps = get_caps_imx208,
	.get_gain_limit = get_gain_limit_imx208,
	.get_integration_time_limit = get_integration_time_limit_imx208,
	.exposure_control = exposure_control_imx208,
	.get_current_exposure = get_current_exposure_imx208,
	.support_af = support_af_imx208,
	.hdr_support = NULL,
	.hdr_get_expo_ratio = NULL,
	.hdr_set_expo_ratio = NULL,
	.hdr_set_gain_mode = NULL,
	.hdr_set_gain = NULL,
	.hdr_enable = NULL,
	.hdr_disable = NULL,
	.get_emb_prop = NULL,
	.get_res_fps = get_res_fps_imx208,
	.get_runtime_fps = get_runtime_fps_imx208,
	.sned_get_fralenline_cmd = sned_get_fralenline_cmd_imx208,
	.get_fralenline_regaddr = get_fralenline_regaddr_imx208,
	.get_i2c_config = get_i2c_config_imx208,
	.get_fw_script_ctrl = get_fw_script_ctrl_imx208,
	.get_analog_gain = get_analog_gain_imx208,
	.get_itime = get_itime_imx208,
	.get_sensor_hw_parameter = get_sensor_hw_parameter_imx208,
};
