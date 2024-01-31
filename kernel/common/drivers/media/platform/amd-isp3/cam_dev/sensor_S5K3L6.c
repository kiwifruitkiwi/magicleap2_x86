/*
 *Copyright(C) 2021 Advanced Micro Devices, Inc. All rights Reserved.
 */

#include "sensor_if.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "[Sensor][sensor_S5K3L6]"

#define DEV_VENDOR				Samsung
#define DEV_NAME				S5K3L6
#define GROUP1_NAME				sensor_phy
#define GROUP0_NAME				sensor
//phy
#define GROUP1_I2C_SLAVE_ADDR			0x14
#define GROUP1_I2C_REG_ADDR_SIZE		2
#define GROUP1_I2C_REG_SIZE			1
#define GROUP1_I2C_SLAVE_ADDR_MODE		I2C_SLAVE_ADDRESS_MODE_7BIT
//sensor
#define GROUP0_I2C_SLAVE_ADDR			0x10
#define GROUP0_I2C_REG_ADDR_SIZE		2
#define GROUP0_I2C_REG_SIZE			1
#define GROUP0_I2C_SLAVE_ADDR_MODE		I2C_SLAVE_ADDRESS_MODE_7BIT

// VCM driver IC is LC898229XI from ON Semi
#define GROUP2_I2C_SLAVE_ADDR			0x0c
#define GROUP2_I2C_REG_ADDR_SIZE		1
#define GROUP2_I2C_REG_SIZE			1
#define GROUP2_I2C_SLAVE_ADDR_MODE		I2C_SLAVE_ADDRESS_MODE_7BIT

// otp driver
#define GROUP3_I2C_SLAVE_ADDR			0x50
#define GROUP3_I2C_REG_ADDR_SIZE		2
#define GROUP3_I2C_REG_SIZE			1
#define GROUP3_I2C_SLAVE_ADDR_MODE		I2C_SLAVE_ADDRESS_MODE_7BIT

#define ENABLE_PPI
#define S5K3L6_EMBEDDED_DATA_EN

//#define CONFIG_S5K3L6_HDR_ITIME_SEPARATE
#define CONFIG_S5K3L6_HDR_ANALOG_GAIN_SEPARATE
#define CONFIG_S5K3L6_HDR_DIGITAL_GAIN_EQUAL

//Tuning parameters:
#define S5K3L6_FPS_ACCUACY_FACTOR		1000

//4096*3072@10fps normal
#define S5K3L6_RES_0_MIN_FPS			10
#define S5K3L6_RES_0_MAX_FPS			10
#define S5K3L6_RES_0_SYSCLK			120000000
#define S5K3L6_RES_0_WIDTH			4096
#define S5K3L6_RES_0_HEIGHT			3072
#define S5K3L6_RES_0_FRAME_LENGTH_LINES		5104
#define S5K3L6_RES_0_LINE_LENGTH_PCLK		9408

//V2H2  2048*1536@15fps
#define S5K3L6_RES_1_MIN_FPS			15
#define S5K3L6_RES_1_MAX_FPS			15
#define S5K3L6_RES_1_SYSCLK			120000000
#define S5K3L6_RES_1_WIDTH			2048
#define S5K3L6_RES_1_HEIGHT			1536
#define S5K3L6_RES_1_FRAME_LENGTH_LINES		6536
#define S5K3L6_RES_1_LINE_LENGTH_PCLK		4896

//V2H2  2048*1536@30fps
#define S5K3L6_RES_2_MIN_FPS			30
#define S5K3L6_RES_2_MAX_FPS			30
#define S5K3L6_RES_2_SYSCLK			120000000
#define S5K3L6_RES_2_WIDTH			2048
#define S5K3L6_RES_2_HEIGHT			1536
#define S5K3L6_RES_2_FRAME_LENGTH_LINES		3268
#define S5K3L6_RES_2_LINE_LENGTH_PCLK		4896

//V2H2  2048*1536@60fps
#define S5K3L6_RES_3_MIN_FPS			60
#define S5K3L6_RES_3_MAX_FPS			60
#define S5K3L6_RES_3_SYSCLK			120000000
#define S5K3L6_RES_3_WIDTH			2048
#define S5K3L6_RES_3_HEIGHT			1536
#define S5K3L6_RES_3_FRAME_LENGTH_LINES		1634
#define S5K3L6_RES_3_LINE_LENGTH_PCLK		4896

//do not support
#define S5K3L6_RES_4_MIN_FPS			1
#define S5K3L6_RES_4_MAX_FPS			1
#define S5K3L6_RES_4_SYSCLK			1
#define S5K3L6_RES_4_WIDTH			1
#define S5K3L6_RES_4_HEIGHT			1
#define S5K3L6_RES_4_FRAME_LENGTH_LINES		1
#define S5K3L6_RES_4_LINE_LENGTH_PCLK		1

//do not support
#define S5K3L6_RES_5_MIN_FPS			1
#define S5K3L6_RES_5_MAX_FPS			1
#define S5K3L6_RES_5_SYSCLK			1
#define S5K3L6_RES_5_WIDTH			1
#define S5K3L6_RES_5_HEIGHT			1
#define S5K3L6_RES_5_FRAME_LENGTH_LINES		1
#define S5K3L6_RES_5_LINE_LENGTH_PCLK		1

//4096x3072@30fps normal
#define S5K3L6_RES_6_MIN_FPS			30
#define S5K3L6_RES_6_MAX_FPS			30
#define S5K3L6_RES_6_SYSCLK			120000000
#define S5K3L6_RES_6_WIDTH			4096
#define S5K3L6_RES_6_HEIGHT			3072
#define S5K3L6_RES_6_FRAME_LENGTH_LINES		3268
#define S5K3L6_RES_6_LINE_LENGTH_PCLK		4896

//fullsize 4096*3072@4fps
#define S5K3L6_RES_7_MIN_FPS			4
#define S5K3L6_RES_7_MAX_FPS			4
#define S5K3L6_RES_7_SYSCLK			120000000
#define S5K3L6_RES_7_WIDTH			4096
#define S5K3L6_RES_7_HEIGHT			3072
#define S5K3L6_RES_7_FRAME_LENGTH_LINES		3136
#define S5K3L6_RES_7_LINE_LENGTH_PCLK		38240

//from data sheet
/*Physical maximum/minimum gain setting in register*/
#define S5K3L6_GAIN_FACTOR             32
#define S5K3L6_MIN_GAIN_SETTING        32
#define S5K3L6_MAX_GAIN_SETTING        512
/*Physical maximum/minimum exposure line*/
#define S5K3L6_LINE_COUNT_OFFSET       2
#define S5K3L6_MIN_EXPOSURE_LINE       1
#define S5K3L6_MAX_EXPOSURE_LINE       (0x0CC3 - S5K3L6_LINE_COUNT_OFFSET)

/*how many ISO is equal to 1.x gain*/
#define S5K3L6_BASE_ISO                100
/* Initial LV value, a 10-based fixed point */
#define S5K3L6_INIT_LV                 70
/* F-Number of lens, a 10-based fixid point */
#define S5K3L6_LENS_APERTURE           28

/* Sensor AF property */
#define S5K3L6_VCM_MIN_POS             0
#define S5K3L6_VCM_MAX_POS             1023
#define S5K3L6_VCM_INIT_POS            0

#define S5K3L6_MIN_AGAIN	(1.0f * GAIN_ACCURACY_FACTOR)
#define S5K3L6_MAX_AGAIN	(16.0f * GAIN_ACCURACY_FACTOR)
#define S5K3L6_MIN_DGAIN	(1.0f * GAIN_ACCURACY_FACTOR)
#define S5K3L6_MAX_DGAIN	(3.0f * GAIN_ACCURACY_FACTOR)

#define S5K3L6_FraLenLineHiAddr		0x0340
#define S5K3L6_FraLenLineLoAddr		0x0341

struct _S5K3L6_drv_context_t {
	uint32_t frame_length_lines;
	uint32_t line_length_pck;
	uint32_t logic_clock;
	uint32_t t_line_ns;	//unit:nanosecond
	uint32_t current_frame_length_lines;
	uint32_t max_line_count;
	uint32_t i_time;	//unit:microsecond
	uint32_t a_gain;	//real A_GAIN*1000
	};
struct _S5K3L6_drv_context_t g_S5K3L6_context;

struct exp_config_t {
	uint32_t coarse_integ_time;	//reg 0x0202,reg 0x0203
	uint32_t a_gain_global;		//reg 0x0204,reg 0x0205
};

static struct exp_config_t exp_config[] = {
	{0x0020, 0x0020},	//res_0
	{0x0020, 0x0020},	//res_1
	{0x0020, 0x0020},	//res_2
	{0x0020, 0x0020},	//res_3
	{0x0, 0x0},	//res_4, don't support case
	{0x0, 0x0},	//res_5, don't support case
	{0x0020, 0x0020},	//res_6
	{0x0020, 0x0020},	//res_7
	{0x0020, 0x0020},	//res_8
};

struct _m2m_info {
	union{
		uint8_t data[32];
		struct {
			uint8_t flag;
			uint8_t module_id;
			uint8_t year;
			uint8_t month;
			uint8_t day;
			uint8_t sensor_id;
			uint8_t lens_id;
			uint8_t vcm_id;
			uint8_t vcm_driver_ic_id;
			uint8_t app_platform;
			uint8_t reserved[18];
			uint8_t checksum[4];
		};
	} header;
	uint8_t awb[60];
	uint16_t awb_addr;
	uint8_t radialShading[836];
	uint16_t radialShading_addr;
	uint8_t meshShading[2332];
	uint16_t meshShading_addr;
	uint8_t af[22];
	uint16_t af_addr;

	struct _M2MAwb_t M2MAwb_t;
	struct _M2MRadialShading_t M2MRadialShading_t;
	struct _M2MMeshShading_t M2MMeshShading_t;
	struct _M2MAF_t M2MAF_t;
	};

//+---------------+--------+-------+
//|Section        |  Start |  Bytes|
//+--------------------------------+
//|Header         |  0x00  |  32   |
//|WB             |  0x20  |  60   |
//|radial shading |  0x5C  |  836  |
//|mesh shading   |  0x3A0 |  2332 |
//|AF             |  0xCBC |  22   |
//+---------------+--------+-------+
struct _m2m_info S5K3L6_m2m_info = {
	{{0}},		//header
	{0},		//awb
	0X0020,		//awb_addr
	{0},		//shading
	0X005C,		//shading_addr
	{0},		//mesh
	0X03A0,		//mesh_addr
	{0},		//af
	0X0CBC,		//af_addr
};

/* approximation of floating point operations */
#define FP_SCALE	1000
#define FP_05		(FP_SCALE / 2)
#define INT_DIV(a, b)	((a) / (b) + ((a) % (b) >= (b) / 2))
#define FP_UP(v)	((v) * FP_SCALE)
#define FP_DN(v)	INT_DIV(v, FP_SCALE)

static void init_S5K3L6_context(uint32_t res)
{
	switch (res) {
	case 0:
		g_S5K3L6_context.frame_length_lines =
			S5K3L6_RES_0_FRAME_LENGTH_LINES;
		g_S5K3L6_context.line_length_pck =
			S5K3L6_RES_0_LINE_LENGTH_PCLK;
		g_S5K3L6_context.logic_clock = S5K3L6_RES_0_SYSCLK;
		break;
	case 1:
		g_S5K3L6_context.frame_length_lines =
			S5K3L6_RES_1_FRAME_LENGTH_LINES;
		g_S5K3L6_context.line_length_pck =
			S5K3L6_RES_1_LINE_LENGTH_PCLK;
		g_S5K3L6_context.logic_clock = S5K3L6_RES_1_SYSCLK;
		break;
	case 2:
		g_S5K3L6_context.frame_length_lines =
			S5K3L6_RES_2_FRAME_LENGTH_LINES;
		g_S5K3L6_context.line_length_pck =
			S5K3L6_RES_2_LINE_LENGTH_PCLK;
		g_S5K3L6_context.logic_clock = S5K3L6_RES_2_SYSCLK;
		break;
	case 3:
		g_S5K3L6_context.frame_length_lines =
			S5K3L6_RES_3_FRAME_LENGTH_LINES;
		g_S5K3L6_context.line_length_pck =
			S5K3L6_RES_3_LINE_LENGTH_PCLK;
		g_S5K3L6_context.logic_clock = S5K3L6_RES_3_SYSCLK;
		break;
	case 4:
		g_S5K3L6_context.frame_length_lines =
			S5K3L6_RES_4_FRAME_LENGTH_LINES;
		g_S5K3L6_context.line_length_pck =
			S5K3L6_RES_4_LINE_LENGTH_PCLK;
		g_S5K3L6_context.logic_clock = S5K3L6_RES_4_SYSCLK;
		break;
	case 5:
		g_S5K3L6_context.frame_length_lines =
			S5K3L6_RES_5_FRAME_LENGTH_LINES;
		g_S5K3L6_context.line_length_pck =
			S5K3L6_RES_5_LINE_LENGTH_PCLK;
		g_S5K3L6_context.logic_clock =
			S5K3L6_RES_5_SYSCLK;
		break;
	case 6:
		g_S5K3L6_context.frame_length_lines =
			S5K3L6_RES_6_FRAME_LENGTH_LINES;
		g_S5K3L6_context.line_length_pck =
			S5K3L6_RES_6_LINE_LENGTH_PCLK;
		g_S5K3L6_context.logic_clock = S5K3L6_RES_6_SYSCLK;
		break;
	case 7:
		g_S5K3L6_context.frame_length_lines =
			S5K3L6_RES_7_FRAME_LENGTH_LINES;
		g_S5K3L6_context.line_length_pck =
			S5K3L6_RES_7_LINE_LENGTH_PCLK;
		g_S5K3L6_context.logic_clock = S5K3L6_RES_7_SYSCLK;
		break;
	default:
		return;
	}

	g_S5K3L6_context.max_line_count =
		g_S5K3L6_context.logic_clock*4 /
		g_S5K3L6_context.line_length_pck;
	g_S5K3L6_context.t_line_ns = FP_UP(g_S5K3L6_context.line_length_pck) /
			(g_S5K3L6_context.logic_clock / 1000000) / 4;
	g_S5K3L6_context.current_frame_length_lines =
		g_S5K3L6_context.frame_length_lines;

}

static int set_gain_S5K3L6(struct sensor_module_t *module,
		uint32_t new_gain, uint32_t *set_gain)
{
	uint32_t gain;
	uint32_t gain_h, gain_l;

	gain = FP_DN(INT_DIV(FP_UP(new_gain * 32),
			     (uint32_t)GAIN_ACCURACY_FACTOR) + FP_05);
	gain_h = (gain & 0xff00) >> 8;
	gain_l = (gain & 0x00ff);

	ISP_PR_INFO("new_gain = %u\n", gain);
	SENSOR_I2C_WRITE(module->hw->sensor_i2c_index, 0x0204, gain_h);
	SENSOR_I2C_WRITE(module->hw->sensor_i2c_index, 0x0205, gain_l);
	if (set_gain) {
		*set_gain = new_gain;
		ISP_PR_DBG("gain[%d] set gain[%d]\n", gain, *set_gain);
	}

	return RET_SUCCESS;
}

static int get_analog_gain_S5K3L6(struct sensor_module_t *module,
		uint32_t *gain)
{
	uint32_t temp_gain = 0;
	uint32_t temp_gain_h, temp_gain_l = 0;

	SENSOR_I2C_READ(module->hw->sensor_i2c_index, 0x0204,
		&temp_gain_h);
	SENSOR_I2C_READ(module->hw->sensor_i2c_index, 0x0205,
		&temp_gain_l);

	temp_gain = ((temp_gain_h & 0xff) << 8) | (temp_gain_l & 0xff);
	ISP_PR_INFO("get_gain_tmp[%u]\n", temp_gain);
	*gain = FP_DN(INT_DIV(FP_UP(temp_gain), 32) *
		      (uint32_t)GAIN_ACCURACY_FACTOR + FP_05);
	ISP_PR_INFO("analog gain[%u]\n", *gain);
	if (gain)
		*gain = temp_gain;

	return RET_SUCCESS;
};

/*
 * new_integration_time (unit: us)
 */
static int set_itime_S5K3L6(struct sensor_module_t *module, uint32_t res,
		uint32_t new_integration_time,
		uint32_t *set_integration_time)
{
	uint32_t line_count;
	uint32_t line_count_h, line_count_l = 0;
	uint32_t line_length_pck;
	uint32_t tline;

	init_S5K3L6_context(res);
	line_length_pck = g_S5K3L6_context.line_length_pck;
	tline = g_S5K3L6_context.t_line_ns;
	line_count = FP_DN(INT_DIV(FP_UP(new_integration_time), FP_DN(tline)) +
			FP_05);
	line_count_h = (line_count >> 8) & 0xff;
	line_count_l = line_count & 0xff;
	SENSOR_I2C_WRITE(module->hw->sensor_i2c_index, 0x0202, line_count_h);
	SENSOR_I2C_WRITE(module->hw->sensor_i2c_index, 0x0203, line_count_l);
	*set_integration_time = FP_DN(line_count * tline + FP_05);
	ISP_PR_DBG("set integration time=%uus\n",
		*set_integration_time);

	return RET_SUCCESS;
}

/*
 * itime (unit: us)
 */
static int get_itime_S5K3L6(struct sensor_module_t *module,
		uint32_t *itime)
{
	uint16_t line_count = 0;
	uint8_t line_count_h, line_count_l = 0;

	init_S5K3L6_context(module->context.res);
	SENSOR_I2C_READ(module->hw->sensor_i2c_index, 0x0202, &line_count_h);
	SENSOR_I2C_READ(module->hw->sensor_i2c_index, 0x0203, &line_count_l);
	line_count = ((line_count_h & 0xff) << 8) | (line_count_l & 0xff);
	if (itime) {
		*itime =
			line_count * FP_DN(g_S5K3L6_context.t_line_ns);
		ISP_PR_INFO("*itime[%dus], line_count[%d]\n",
			*itime, line_count);
	}

	return RET_SUCCESS;
}

/*
 * new_gain : 12345 means 12.345x
 * new_integration_time (unit: us)
 */
static int exposure_control_S5K3L6(struct sensor_module_t *module,
		uint32_t new_gain,
		uint32_t new_integration_time,
		uint32_t *set_gain,
		uint32_t *set_integration)
{
	if (new_gain)
		set_gain_S5K3L6(module, new_gain, set_gain);
	if (new_integration_time)
		set_itime_S5K3L6(module, module->context.res,
				new_integration_time, set_integration);

	return RET_SUCCESS;
};

static int get_current_exposure_S5K3L6(struct sensor_module_t *module,
		uint32_t *gain,
		uint32_t *integration_time)
{
	get_analog_gain_S5K3L6(module, gain);
	get_itime_S5K3L6(module, integration_time);

	return RET_SUCCESS;
}

static int poweron_S5K3L6(struct sensor_module_t __maybe_unused *module,
	int __maybe_unused on)
{
	return RET_SUCCESS;
}

static int reset_S5K3L6(struct sensor_module_t __maybe_unused *module)
{
	return RET_SUCCESS;
}

static int config_S5K3L6(struct sensor_module_t __maybe_unused *module,
			 unsigned int res, unsigned int flag)
{
	struct drv_context *drv_context = &module->context;
	drv_context->res = res;
	drv_context->flag = flag;

	ISP_PR_INFO("res %d\n", res);
	init_S5K3L6_context(res);
	drv_context->frame_length_lines = g_S5K3L6_context.frame_length_lines;
	drv_context->line_length_pck = g_S5K3L6_context.line_length_pck;
	switch (res) {
	case 0:
		drv_context->t_line_numerator =
			S5K3L6_RES_0_LINE_LENGTH_PCLK;
		drv_context->t_line_denominator =
			S5K3L6_RES_0_SYSCLK * 4 / 1000000;
		drv_context->bitrate = 600;
		break;
	case 1:
		drv_context->t_line_numerator =
			S5K3L6_RES_1_LINE_LENGTH_PCLK;
		drv_context->t_line_denominator =
			S5K3L6_RES_1_SYSCLK * 4 / 1000000;
		drv_context->bitrate = 600;
		break;
	case 2:
		drv_context->t_line_numerator =
			S5K3L6_RES_2_LINE_LENGTH_PCLK;
		drv_context->t_line_denominator =
			S5K3L6_RES_2_SYSCLK * 4 / 1000000;
		drv_context->bitrate = 600;
		break;
	case 3:
		drv_context->t_line_numerator =
			S5K3L6_RES_3_LINE_LENGTH_PCLK;
		drv_context->t_line_denominator =
			S5K3L6_RES_3_SYSCLK * 4 / 1000000;
		drv_context->bitrate = 568;
		break;
	case 6:
		drv_context->t_line_numerator =
			S5K3L6_RES_6_LINE_LENGTH_PCLK;
		drv_context->t_line_denominator =
			S5K3L6_RES_6_SYSCLK * 4 / 1000000;
		drv_context->bitrate = 1104;
		break;
	case 7:
		drv_context->t_line_numerator =
			S5K3L6_RES_7_LINE_LENGTH_PCLK;
		drv_context->t_line_denominator =
			S5K3L6_RES_7_SYSCLK * 4 / 1000000;
		drv_context->bitrate = 300;
		break;
	default:
		ISP_PR_ERR("Unsupported res:%d\n", res);
		return RET_FAILURE;
	}

	drv_context->mode = 0;
	drv_context->max_torch_curr = FLASH_MAX_TORCH_CURR_LEVEL0;
	drv_context->max_flash_curr = FLASH_MAX_FLASH_CURR_LEVEL0;

	g_S5K3L6_context.i_time = exp_config[res].coarse_integ_time *
			FP_DN(g_S5K3L6_context.t_line_ns);
	g_S5K3L6_context.a_gain = FP_DN(INT_DIV(FP_UP
		(exp_config[res].a_gain_global), 32) *
		(uint32_t)GAIN_ACCURACY_FACTOR + FP_05);

	ISP_PR_DBG("res[%u] frame_length_lines[%u] line_length_pck[%u],",
		drv_context->res, drv_context->frame_length_lines,
		drv_context->line_length_pck);
	ISP_PR_DBG("t_line_numerator[%u] t_line_denominator[%u],",
		drv_context->t_line_numerator,
		drv_context->t_line_denominator);
	ISP_PR_DBG("bitrate[%u] mode[%u] max_t_curr[%u] max_f_curr[%u]\n",
		drv_context->bitrate, drv_context->mode,
		drv_context->max_torch_curr, drv_context->max_flash_curr);
	ISP_PR_DBG("initial i_time[%dus],\n", g_S5K3L6_context.i_time);
	ISP_PR_DBG("initial analog gain[%u]\n", g_S5K3L6_context.a_gain);
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
		SENSOR_I2C_READ(module->hw->sensor_i2c_index, 0x0000, &id_h);
		SENSOR_I2C_READ(module->hw->sensor_i2c_index, 0x0001, &id_l);

	if ((id_h == 0x30) && (id_l == 0xc6)) {
		ISP_PR_PC("Sensor id match: S5K3L6\n");
	} else {
		ISP_PR_ERR("id mismatch Reg(0x0016)=0x%x Reg(0x0017)=0x%x\n",
			id_h, id_l);
		return RET_FAILURE;
	}
	return RET_SUCCESS;
}

#define aidt_sleep msleep

static int get_caps_S5K3L6(struct sensor_module_t *module,
	struct _isp_sensor_prop_t *caps, int prf_id);

uint16_t s5k3l6_init[] = {
	0x0100, 0x0000,
	0x0A02, 0x3400,
	0x3084, 0x1314,
	0x3266, 0x0001,
	0x3242, 0x2020,
	0x306A, 0x2F4C,
	0x306C, 0xCA01,
	0x307A, 0x0D20,
	0x309E, 0x002D,
	0x3072, 0x0013,
	0x3074, 0x0977,
	0x3076, 0x9411,
	0x3024, 0x0016,
	0x3070, 0x3D00,
	0x3002, 0x0E00,
	0x3006, 0x1000,
	0x300A, 0x0C00,
	0x3010, 0x0400,
	0x3018, 0xC500,
	0x303A, 0x0204,
	0x3452, 0x0001,
	0x3454, 0x0001,
	0x3456, 0x0001,
	0x3458, 0x0001,
	0x345a, 0x0002,
	0x345C, 0x0014,
	0x345E, 0x0002,
	0x3460, 0x0014,
	0x3464, 0x0006,
	0x3466, 0x0012,
	0x3468, 0x0012,
	0x346A, 0x0012,
	0x346C, 0x0012,
	0x346E, 0x0012,
	0x3470, 0x0012,
	0x3472, 0x0008,
	0x3474, 0x0004,
	0x3476, 0x0044,
	0x3478, 0x0004,
	0x347A, 0x0044,
	0x347E, 0x0006,
	0x3480, 0x0010,
	0x3482, 0x0010,
	0x3484, 0x0010,
	0x3486, 0x0010,
	0x3488, 0x0010,
	0x348A, 0x0010,
	0x348E, 0x000C,
	0x3490, 0x004C,
	0x3492, 0x000C,
	0x3494, 0x004C,
	0x3496, 0x0020,
	0x3498, 0x0006,
	0x349A, 0x0008,
	0x349C, 0x0008,
	0x349E, 0x0008,
	0x34A0, 0x0008,
	0x34A2, 0x0008,
	0x34A4, 0x0008,
	0x34A8, 0x001A,
	0x34AA, 0x002A,
	0x34AC, 0x001A,
	0x34AE, 0x002A,
	0x34B0, 0x0080,
	0x34B2, 0x0006,
	0x32A2, 0x0000,
	0x32A4, 0x0000,
	0x32A6, 0x0000,
	0x32A8, 0x0000,
	0x3066, 0x7E00,
	0x3004, 0x0800,
	0x3C08, 0xFFFF,
};

//case 0:
uint16_t s5k3l6_4096x3072_10fps_4lane[] = {
	0x0344, 0x0040, //x_addr_start    [15:8]    [7:0]
	0x0346, 0x0020, //y_addr_start    [15:8]    [7:0]
	0x0348, 0x103F, //x_addr_end      [15:8]    [7:0]
	0x034A, 0x0C1F, //y_addr_end      [15:8]    [7:0]
	0x034C, 0x1000, //X_output_size   [15:8]    [7:0]
	0x034E, 0x0C00, //y_output_size   [15:8]    [7:0]
	0x0900, 0x0000, //binning_mode binning_type    [7:0]    [7:0]
	0x0380, 0x0001, //x_even_inc      [4:0]
	0x0382, 0x0001, //x_odd_inc       [4:0]
	0x0384, 0x0001, //y_even_inc      [4:0]
	0x0386, 0x0001, //y_odd_inc       [4:0]
	0x0114, 0x0330, //csi_lane_mode   0-3h
	0x0110, 0x0002, //csi_ch_id       0-3h
	0x0136, 0x1800, //extern clock Frequency    [15:8]    [7:0]
	//0x0304, 0x0004, //pre_pll_clk_div           [5:0]
	0x0304, 0x0004, //pre_pll_clk_div           [5:0]
	//0x0306, 0x0078, //pll_multiplier            [9:8]     [7:0]
	0x0306, 120, //pll_multiplier            [9:8]     [7:0]
	0x3C1E, 0x0000, //pll_post_scaler           [2:0]
	0x030C, 0x0004, //MIPI_pre_pll_clk_div
	0x030E, 0x0064, //MIPI_pll_multiplier
	0x3C16, 0x0001, //MIPI_post_scaler          [2:0]
	0x0300, 0x0006, //vt_pix_clk_div            [3:0]
	0x0342, 9408, //line_length_pck           [15:8]    [7:0]
	0x0340, 5104, //frame_length_lines        [15:8]    [7:0]
	0x38C4, 0x0004,
	0x38D8, 0x0012,
	0x38DA, 0x0005,
	0x38DC, 0x0006,
	0x38C2, 0x0005,
	0x38C0, 0x0005,
	0x38D6, 0x0005,
	0x38D4, 0x0004,
	0x38B0, 0x0007,
	0x3932, 0x1800,
	0x3938, 0x000C,
	0x0820, 600, //MIPI_bit_rate
	//0x0820, 0x0258, //MIPI_bit_rate
	0x380C, 0x0090,
	0x3064, 0xEFCF,
	0x309C, 0x0640,
	0x3090, 0x8800,
	0x3238, 0x000C,
	0x314A, 0x5F00,
	0x3300, 0x0000,
	0x3400, 0x0000,
	0x3402, 0x4E46, //0x3403 bit[0]=0/1:dynamic BPC OFF/ON
	0x32B2, 0x0000,
	0x32B4, 0x0000,
	0x32B6, 0x0000,
	0x32B8, 0x0000,
	0x32B2, 0x0006,
	0x32B4, 0x0006,
	0x32B6, 0x0006,
	0x32B8, 0x0006,
	0x3C34, 0x0008,
	0x3C36, 0x0000,
	0x3C38, 0x0000,
	0x393E, 0x4000,
	0x0202, 0x0020, //Expo
	0x0204, 0x0020, //Gain
//	0x3C30, 0x0F2E, //Embedded Data [1] 0:on 1:off
	0x3C1E, 0x0100,
	0x0100, 0x0100,
	0x3C1E, 0x0000,
};

//case 1:
uint16_t s5k3l6_2048x1536_15fps_4lane[] = {
	0x0344, 0x0040,	//x_addr_start    [15:8]    [7:0]
	0x0346, 0x0020,	//y_addr_start    [15:8]    [7:0]
	0x0348, 0x103F,	//x_addr_end      [15:8]    [7:0]
	0x034A, 0x0C1F,	//y_addr_end      [15:8]    [7:0]
	0x034C, 0x0800,	//X_output_size   [15:8]    [7:0]
	0x034E, 0x0600,	//y_output_size   [15:8]    [7:0]
	0x0900, 0x0122,	//binning_mode binning_type    [7:0]    [7:0]
	0x0380, 0x0001,	//x_even_inc      [4:0]
	0x0382, 0x0001,	//x_odd_inc       [4:0]
	0x0384, 0x0001,	//y_even_inc      [4:0]
	0x0386, 0x0003,	//y_odd_inc       [4:0]
	0x0114, 0x0330,	//csi_lane_mode   0-3h
	0x0110, 0x0002,	//csi_ch_id       0-3h
	0x0136, 0x1800,	//extern clock Frequency    [15:8]    [7:0]
	0x0304, 0x0004,	//pre_pll_clk_div           [5:0]
	0x0306, 0x0078,	//pll_multiplier            [9:8]     [7:0]
	0x3C1E, 0x0000, //pll_post_scaler           [2:0]
	0x030C, 0x0003, //MIPI_pre_pll_clk_div
	0x030E, 0x004B, //MIPI_pll_multiplier
	0x3C16, 0x0001, //MIPI_post_scaler          [2:0]
	0x0300, 0x0006, //vt_pix_clk_div            [3:0]
	0x0342, 0x1320, //line_length_pck           [15:8]    [7:0]
	0x0340, 0x1988, //frame_length_lines        [15:8]    [7:0]
	0x38C4, 0x0004,
	0x38D8, 0x0012,
	0x38DA, 0x0005,
	0x38DC, 0x0006,
	0x38C2, 0x0005,
	0x38C0, 0x0005,
	0x38D6, 0x0005,
	0x38D4, 0x0004,
	0x38B0, 0x0007,
	0x3932, 0x1800,
	0x3938, 0x000C,
	0x0820, 0x0258, //MIPI_bit_rate
	0x380C, 0x0049,
	0x3064, 0xFFCF,
	0x309C, 0x0640,
	0x3090, 0x8000,
	0x3238, 0x000B,
	0x314A, 0x5F02,
	0x3300, 0x0000,
	0x3400, 0x0000,
	0x3402, 0x4E46, //0x3403 bit[0]=0/1:dynamic BPC OFF/ON
	0x32B2, 0x0008,
	0x32B4, 0x0008,
	0x32B6, 0x0008,
	0x32B8, 0x0008,
	0x3C34, 0x0048,
	0x3C36, 0x3000,
	0x3C38, 0x0020,
	0x393E, 0x4000,
	0x0202, 0x0020, //Expo
	0x0204, 0x0020, //Gain
//	0x3C30, 0x0F2E, //Embedded Data [1] 0:on 1:off
	0x3C1E, 0x0100,
	0x0100, 0x0100,
	0x3C1E, 0x0000,
};

//case 2:
uint16_t s5k3l6_2048x1536_30fps_4lane[] = {
	0x0344, 0x0040,	//x_addr_start    [15:8]    [7:0]
	0x0346, 0x0020,	//y_addr_start    [15:8]    [7:0]
	0x0348, 0x103F,	//x_addr_end      [15:8]    [7:0]
	0x034A, 0x0C1F,	//y_addr_end      [15:8]    [7:0]
	0x034C, 0x0800,	//X_output_size   [15:8]    [7:0]
	0x034E, 0x0600,	//y_output_size   [15:8]    [7:0]
	0x0900, 0x0122,	//binning_mode binning_type    [7:0]    [7:0]
	0x0380, 0x0001,	//x_even_inc      [4:0]
	0x0382, 0x0001,	//x_odd_inc       [4:0]
	0x0384, 0x0001,	//y_even_inc      [4:0]
	0x0386, 0x0003,	//y_odd_inc       [4:0]
	0x0114, 0x0330,	//csi_lane_mode   0-3h
	0x0110, 0x0002,	//csi_ch_id       0-3h
	0x0136, 0x1800,	//extern clock Frequency    [15:8]    [7:0]
	0x0304, 0x0004,	//pre_pll_clk_div           [5:0]
	0x0306, 0x0078,	//pll_multiplier            [9:8]     [7:0]
	0x3C1E, 0x0000, //pll_post_scaler           [2:0]
	0x030C, 0x0003, //MIPI_pre_pll_clk_div
	0x030E, 0x004B, //MIPI_pll_multiplier
	0x3C16, 0x0001, //MIPI_post_scaler          [2:0]
	0x0300, 0x0006, //vt_pix_clk_div            [3:0]
	0x0342, 0x1320, //line_length_pck           [15:8]    [7:0]
	0x0340, 0x0CC4, //frame_length_lines        [15:8]    [7:0]
	0x38C4, 0x0004,
	0x38D8, 0x0012,
	0x38DA, 0x0005,
	0x38DC, 0x0006,
	0x38C2, 0x0005,
	0x38C0, 0x0005,
	0x38D6, 0x0005,
	0x38D4, 0x0004,
	0x38B0, 0x0007,
	0x3932, 0x1800,
	0x3938, 0x000C,
	0x0820, 0x0258, //MIPI_bit_rate
	0x380C, 0x0049,
	0x3064, 0xFFCF,
	0x309C, 0x0640,
	0x3090, 0x8000,
	0x3238, 0x000B,
	0x314A, 0x5F02,
	0x3300, 0x0000,
	0x3400, 0x0000,
	0x3402, 0x4E46, //0x3403 bit[0]=0/1:dynamic BPC OFF/ON
	0x32B2, 0x0008,
	0x32B4, 0x0008,
	0x32B6, 0x0008,
	0x32B8, 0x0008,
	0x3C34, 0x0048,
	0x3C36, 0x3000,
	0x3C38, 0x0020,
	0x393E, 0x4000,
	0x0202, 0x0020, //Expo
	0x0204, 0x0020, //Gain
//	0x3C30, 0x0F2E, //Embedded Data [1] 0:on 1:off
	0x3C1E, 0x0100,
	0x0100, 0x0100,
	0x3C1E, 0x0000,
};

// case 3
uint16_t s5k3l6_2048x1536_60fps_4lane[] = {
	0x0344, 0x0040,	//x_addr_start    [15:8]    [7:0]
	0x0346, 0x0020,	//y_addr_start    [15:8]    [7:0]
	0x0348, 0x103F,	//x_addr_end      [15:8]    [7:0]
	0x034A, 0x0C1F,	//y_addr_end      [15:8]    [7:0]
	0x034C, 0x0800,	//X_output_size   [15:8]    [7:0]
	0x034E, 0x0600,	//y_output_size   [15:8]    [7:0]
	0x0900, 0x0122,
	0x0380, 0x0001,
	0x0382, 0x0001,
	0x0384, 0x0001,
	0x0386, 0x0003,
	0x0114, 0x0330,
	0x0110, 0x0002,
	0x0136, 0x1800,
	0x0304, 0x0004,
	0x0306, 0x0078,
	0x3C1E, 0x0000,
	0x030C, 0x0003,
	0x030E, 0x0047,
	0x3C16, 0x0001,
	0x0300, 0x0006,
	0x0342, 0x1320, //line_length_pck           [15:8]    [7:0]
	0x0340, 0x0662, //frame_length_lines        [15:8]    [7:0]
	0x38C4, 0x0004,
	0x38D8, 0x0011,
	0x38DA, 0x0005,
	0x38DC, 0x0005,
	0x38C2, 0x0005,
	0x38C0, 0x0004,
	0x38D6, 0x0004,
	0x38D4, 0x0004,
	0x38B0, 0x0007,
	0x3932, 0x1000,
	0x3938, 0x000C,
	0x0820, 0x0238,
	0x380C, 0x0049,
	0x3064, 0xFFCF,
	0x309C, 0x0640,
	0x3090, 0x8000,
	0x3238, 0x000B,
	0x314A, 0x5F02,
	0x3300, 0x0000,
	0x3400, 0x0000,
	0x3402, 0x4E46, //0x3403 bit[0]=0/1:dynamic BPC OFF/ON
	0x32B2, 0x0008,
	0x32B4, 0x0008,
	0x32B6, 0x0008,
	0x32B8, 0x0008,
	0x3C34, 0x0048,
	0x3C36, 0x3000,
	0x3C38, 0x0020,
	0x393E, 0x4000,
	0x0202, 0x0020, //Expo
	0x0204, 0x0020, //Gain
//	0x3C30, 0x0F2E, //Embedded Data [1] 0:on 1:off
	0x3C1E, 0x0100,
	0x0100, 0x0100,
	0x3C1E, 0x0000,
};

//case 6:
uint16_t s5k3l6_4096x3072_30fps_4lane[] = {
	0x0344, 0x0040,	//x_addr_start    [15:8]    [7:0]
	0x0346, 0x0020,	//y_addr_start    [15:8]    [7:0]
	0x0348, 0x103F,	//x_addr_end      [15:8]    [7:0]
	0x034A, 0x0C1F,	//y_addr_end      [15:8]    [7:0]
	0x034C, 0x1000,	//X_output_size   [15:8]    [7:0]
	0x034E, 0x0C00,	//y_output_size   [15:8]    [7:0]
	0x0900, 0x0000,	//binning_mode binning_type    [7:0]    [7:0]
	0x0380, 0x0001,	//x_even_inc      [4:0]
	0x0382, 0x0001,	//x_odd_inc       [4:0]
	0x0384, 0x0001,	//y_even_inc      [4:0]
	0x0386, 0x0001,	//y_odd_inc       [4:0]
	0x0114, 0x0330,	//csi_lane_mode   0-3h
	0x0110, 0x0002,	//csi_ch_id       0-3h
	0x0136, 0x1800,	//extern clock Frequency    [15:8]    [7:0]
	0x0304, 0x0004,	//pre_pll_clk_div           [5:0]
	0x0306, 0x0078,	//pll_multiplier            [9:8]     [7:0]
	0x3C1E, 0x0000, //pll_post_scaler           [2:0]
	0x030C, 0x0003, //MIPI_pre_pll_clk_div
	0x030E, 0x0045, //MIPI_pll_multiplier
	0x3C16, 0x0000, //MIPI_post_scaler          [2:0]
	0x0300, 0x0006, //vt_pix_clk_div            [3:0]
	0x0342, 0x1320, //line_length_pck           [15:8]    [7:0]
	0x0340, 0x0CC4, //frame_length_lines        [15:8]    [7:0]
	0x38C4, 0x0009,
	0x38D8, 0x0027,
	0x38DA, 0x0009,
	0x38DC, 0x000A,
	0x38C2, 0x0009,
	0x38C0, 0x000D,
	0x38D6, 0x0009,
	0x38D4, 0x0008,
	0x38B0, 0x000D,
	0x3932, 0x1800,
	0x3938, 0x000C,
	0x0820, 0x0450, //MIPI_bit_rate
	0x380C, 0x0090,
	0x3064, 0xFFCF,
	0x309C, 0x0640,
	0x3090, 0x8800,
	0x3238, 0x000C,
	0x314A, 0x5F00,
	0x3300, 0x0000,
	0x3400, 0x0000,
	0x3402, 0x4E46, //0x3403 bit[0]=0/1:dynamic BPC OFF/ON
	0x32B2, 0x0006,
	0x32B4, 0x0006,
	0x32B6, 0x0006,
	0x32B8, 0x0006,
	0x3C34, 0x0048,
	0x3C36, 0x3000,
	0x3C38, 0x0020,
	0x393E, 0x4000,
	0x0202, 0x0020, //Expo
	0x0204, 0x0020, //Gain
//	0x3C30, 0x0F2E, //Embedded Data [1] 0:on 1:off
	0x3C1E, 0x0100,
	0x0100, 0x0100,
	0x3C1E, 0x0000,
};

//case 7:
uint16_t s5k3l6_4096x3072_4fps_2lane[] = {
	0x0344, 0x0040,	//x_addr_start    [15:8]    [7:0]
	0x0346, 0x0020,	//y_addr_start    [15:8]    [7:0]
	0x0348, 0x103F,	//x_addr_end      [15:8]    [7:0]
	0x034A, 0x0C1F,	//y_addr_end      [15:8]    [7:0]
	0x034C, 0x1000,	//X_output_size   [15:8]    [7:0]
	0x034E, 0x0C00,	//y_output_size   [15:8]    [7:0]
	0x0900, 0x0000,	//binning_mode binning_type    [7:0]    [7:0]
	0x0380, 0x0001,	//x_even_inc      [4:0]
	0x0382, 0x0001,	//x_odd_inc       [4:0]
	0x0384, 0x0001,	//y_even_inc      [4:0]
	0x0386, 0x0001,	//y_odd_inc       [4:0]
	0x0114, 0x0130,	//csi_lane_mode   0-3h
	0x0110, 0x0002,	//csi_ch_id       0-3h
	0x0136, 0x1800,	//extern clock Frequency    [15:8]    [7:0]
	0x0304, 0x0004,	//pre_pll_clk_div           [5:0]
	0x0306, 0x0078,	//pll_multiplier            [9:8]     [7:0]
	0x3C1E, 0x0000, //pll_post_scaler           [2:0]
	0x030C, 0x0003, //MIPI_pre_pll_clk_div
	0x030E, 0x004B, //MIPI_pll_multiplier
	0x3C16, 0x0002, //MIPI_post_scaler          [2:0]
	0x0300, 0x0006, //vt_pix_clk_div            [3:0]
	0x0342, 0x9560, //line_length_pck           [15:8]    [7:0]
	0x0340, 0x0C40, //frame_length_lines        [15:8]    [7:0]
	0x38C4, 0x0002,
	0x38D8, 0x0006,
	0x38DA, 0x0003,
	0x38DC, 0x0003,
	0x38C2, 0x0002,
	0x38C0, 0x0000,
	0x38D6, 0x0002,
	0x38D4, 0x0002,
	0x38B0, 0x0003,
	0x3932, 0x1800,
	0x3938, 0x000C,
	0x0820, 0x012C, //MIPI_bit_rate
	0x380C, 0x0090,
	0x3064, 0xFFCF,
	0x309C, 0x0640,
	0x3090, 0x8800,
	0x3238, 0x000C,
	0x314A, 0x5F00,
	0x3300, 0x0000,
	0x3400, 0x0000,
	0x3402, 0x4E46, //0x3403 bit[0]=0/1:dynamic BPC OFF/ON
	0x32B2, 0x0006,
	0x32B4, 0x0006,
	0x32B6, 0x0006,
	0x32B8, 0x0006,
	0x3C34, 0x0048,
	0x3C36, 0x3000,
	0x3C38, 0x0020,
	0x393E, 0x4000,
	0x0202, 0x0020, //Expo
	0x0204, 0x0020, //Gain
//	0x3C30, 0x0F2E, //Embedded Data [1] 0:on 1:off
	0x3C1E, 0x0100,
	0x0100, 0x0100,
	0x3C1E, 0x0000,
};

static uint32_t  high_low_switch(uint32_t data)
{
	return ((data << 8) & 0xff00) | ((data >> 8) & 0x00ff);
}

static int start_sensor_mode_setting(unsigned char bus,
				uint16_t (*psetting)[2], uint32_t size)
{
	uint16_t value = 0;
	uint32_t index;

	for (index = 0 ; index < size; index++) {
		value = high_low_switch(psetting[index][1]);
		SENSOR_I2C_WRITE_GROUP(bus, psetting[index][0],
					(int *)&value, 2);
	}

	ISP_PR_INFO("s5k3l6 mode setting, success\n");

	return RET_SUCCESS;
}

static int start_sensor_init_setting(unsigned char bus,
				uint16_t (*psetting)[2], uint32_t size)
{
	uint16_t value = 0;
	uint32_t index;
	uint32_t regVal = 0;

	for (index = 0 ; index < size; index++) {
		value = high_low_switch(psetting[index][1]);
		SENSOR_I2C_WRITE_GROUP(bus, psetting[index][0],
					(int *)&value, 2);
		if (index == 2)
			aidt_sleep(10);
	}

#ifdef S5K3L6_EMBEDDED_DATA_EN
	regVal = 0;
	SENSOR_I2C_READ(bus, 0x3C30, &regVal);
	regVal |= 0x0F;
	SENSOR_I2C_WRITE(bus, 0x3C30, regVal);
#endif
	ISP_PR_INFO("s5k3l6 init setting, success\n");

	return RET_SUCCESS;
}

static int start_sensor_profile_setting(unsigned char bus,
					uint16_t (*psetting)[2], uint32_t size)
{
	int ret = RET_SUCCESS;

	ret = start_sensor_init_setting(bus, (uint16_t (*)[2])s5k3l6_init,
			ARRAY_SIZE(s5k3l6_init) / 2);
	ret = start_sensor_mode_setting(bus, psetting, size);
	return ret;
}

static int res_0_config_S5K3L6(struct sensor_module_t *module)
{
	unsigned char bus = module->hw->sensor_i2c_index;
	uint16_t (*psetting)[2];
	uint32_t size;

	ISP_PR_INFO("device id[%d]\n", module->id);
	psetting = (uint16_t (*)[2])s5k3l6_4096x3072_10fps_4lane;
	size = ARRAY_SIZE(s5k3l6_4096x3072_10fps_4lane) / 2;
	start_sensor_profile_setting(bus, psetting, size);

	return RET_SUCCESS;
}

static int res_1_config_S5K3L6(struct sensor_module_t *module)
{
	unsigned char bus = module->hw->sensor_i2c_index;
	uint16_t (*psetting)[2];
	uint32_t size;

	ISP_PR_PC("device id[%d]\n", module->id);
	psetting = (uint16_t (*)[2])s5k3l6_2048x1536_15fps_4lane;
	size = ARRAY_SIZE(s5k3l6_2048x1536_15fps_4lane) / 2;
	start_sensor_profile_setting(bus, psetting, size);

	return RET_SUCCESS;
}

static int res_2_config_S5K3L6(struct sensor_module_t *module)
{


	unsigned char bus = module->hw->sensor_i2c_index;
	uint16_t (*psetting)[2];
	uint32_t size;

	ISP_PR_PC("device id[%d]\n", module->id);
	psetting = (uint16_t (*)[2])s5k3l6_2048x1536_30fps_4lane;
	size = ARRAY_SIZE(s5k3l6_2048x1536_30fps_4lane) / 2;
	start_sensor_profile_setting(bus, psetting, size);

	return RET_SUCCESS;
}

static int res_3_config_S5K3L6(struct sensor_module_t *module)
{
	unsigned char bus = module->hw->sensor_i2c_index;
	uint16_t (*psetting)[2];
	uint32_t size;

	ISP_PR_PC("device id[%d]\n", module->id);
	psetting = (uint16_t (*)[2])s5k3l6_2048x1536_60fps_4lane;
	size = ARRAY_SIZE(s5k3l6_2048x1536_60fps_4lane) / 2;
	start_sensor_profile_setting(bus, psetting, size);
	return RET_SUCCESS;
}

static int res_4_config_S5K3L6(struct sensor_module_t *module)
{
	ISP_PR_INFO("device id[%d]\n", module->id);

	return RET_SUCCESS;
}

static int res_5_config_S5K3L6(struct sensor_module_t *module)
{
	ISP_PR_INFO("device id[%d]\n", module->id);

	return RET_SUCCESS;
}

static int res_6_config_S5K3L6(struct sensor_module_t *module)
{
	unsigned char bus = module->hw->sensor_i2c_index;
	uint16_t (*psetting)[2];
	uint32_t size;

	ISP_PR_PC("device id[%d] ", module->id);

	psetting = (uint16_t (*)[2])s5k3l6_4096x3072_30fps_4lane;
	size = ARRAY_SIZE(s5k3l6_4096x3072_30fps_4lane) / 2;
	start_sensor_profile_setting(bus, psetting, size);

	return RET_SUCCESS;
}

static int res_7_config_S5K3L6(struct sensor_module_t *module)
{
	unsigned char bus = module->hw->sensor_i2c_index;
	uint16_t (*psetting)[2];
	uint32_t size;

	ISP_PR_PC("device id[%d]\n", module->id);

	psetting = (uint16_t (*)[2])s5k3l6_4096x3072_4fps_2lane;
	size = ARRAY_SIZE(s5k3l6_4096x3072_4fps_2lane) / 2;
	start_sensor_profile_setting(bus, psetting, size);

	return RET_SUCCESS;
}

static int vcm_PD50_initial(struct sensor_module_t *module)
{
	uint8_t bus = module->hw->sensor_i2c_index;
	uint8_t reg = 0;

	ISP_PR_INFO("PD50_initialize, device id %d\n", module->id);

	SENSOR_VCM_I2C_READ(bus, 0x07, &reg);
	ISP_PR_INFO("PD50 chip id %x\n", reg);
	ISP_PR_INFO("PD50 standby\n");
	SENSOR_VCM_I2C_WRITE(bus, 0x02, 00);
	SENSOR_VCM_I2C_WRITE(bus, 0x03, 00);
	SENSOR_VCM_I2C_WRITE(bus, 0x00, 00);
	usleep_range(9000, 10000);
	SENSOR_VCM_I2C_READ(bus, 0x02, &reg);
	ISP_PR_INFO("PD50 ADC_MSB %x\n", reg);
	SENSOR_VCM_I2C_READ(bus, 0x03, &reg);
	ISP_PR_INFO("PD50 ADC_LSB %x\n", reg);
	SENSOR_VCM_I2C_READ(bus, 0x00, &reg);
	ISP_PR_INFO("PD50 enable: %02X\n", reg);
	SENSOR_VCM_I2C_WRITE(bus, 0x00, 01);
	SENSOR_VCM_I2C_READ(bus, 0x00, &reg);
	ISP_PR_INFO("PD50 enable: %02X\n", reg);

	return RET_SUCCESS;
}

static int get_m2m_data_S5K3L6(struct sensor_module_t *module,
				struct _M2MTdb_t *pM2MTdb)
{
	uint8_t bus = module->hw->sensor_i2c_index;
	//uint8_t i = 0;
	ISP_PR_INFO(" device id %d\n", module->id);
	memset(S5K3L6_m2m_info.header.data, 0,
		sizeof(S5K3L6_m2m_info.header.data));
	SENSOR_OTP_I2C_READ_GROUP(bus, 0x00, S5K3L6_m2m_info.header.data,
				sizeof(S5K3L6_m2m_info.header.data));
	//for test
	//for (i= 0; i<32; i++){
	//	ISP_PR_PC(" i =%d, is %02X ", i,
	//		S5K3L6_m2m_info.header.data[i]);
	//}
	if ((S5K3L6_m2m_info.header.flag != 1) ||
	(S5K3L6_m2m_info.header.module_id != 0x07)) {
		ISP_PR_ERR("Module flag [%2x] or module id [%2x] is error\n",
			S5K3L6_m2m_info.header.flag,
			S5K3L6_m2m_info.header.module_id);
		return RET_FAILURE;
	}

	SENSOR_OTP_I2C_READ_GROUP(bus, S5K3L6_m2m_info.awb_addr,
		S5K3L6_m2m_info.awb, sizeof(S5K3L6_m2m_info.awb));
	SENSOR_OTP_I2C_READ_GROUP(bus, S5K3L6_m2m_info.radialShading_addr,
				S5K3L6_m2m_info.radialShading,
				sizeof(S5K3L6_m2m_info.radialShading));
	SENSOR_OTP_I2C_READ_GROUP(bus, S5K3L6_m2m_info.meshShading_addr,
				S5K3L6_m2m_info.meshShading,
				sizeof(S5K3L6_m2m_info.meshShading));
	SENSOR_OTP_I2C_READ_GROUP(bus, S5K3L6_m2m_info.af_addr,
		S5K3L6_m2m_info.af, sizeof(S5K3L6_m2m_info.af));

	memcpy((uint8_t *)&pM2MTdb->awb, S5K3L6_m2m_info.awb,
			sizeof(S5K3L6_m2m_info.M2MAwb_t));
	memcpy((uint8_t *)&pM2MTdb->radialShading,
			S5K3L6_m2m_info.radialShading,
			sizeof(S5K3L6_m2m_info.M2MRadialShading_t));
	memcpy((uint8_t *)&pM2MTdb->meshShading, S5K3L6_m2m_info.meshShading,
			sizeof(S5K3L6_m2m_info.M2MMeshShading_t));
	memcpy((uint8_t *)&pM2MTdb->af, S5K3L6_m2m_info.af,
			sizeof(S5K3L6_m2m_info.M2MAF_t));

	return RET_SUCCESS;
}

static int start_S5K3L6(struct sensor_module_t *module, int config_phy)
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
		res_0_config_S5K3L6(module);
		break;
	case 1:
		res_1_config_S5K3L6(module);
		break;
	case 2:
		res_2_config_S5K3L6(module);
		break;
	case 3:
		res_3_config_S5K3L6(module);
		break;
	case 4:
		res_4_config_S5K3L6(module);
		break;
	case 5:
		res_5_config_S5K3L6(module);
		break;
	case 6:
		res_6_config_S5K3L6(module);
		break;
	case 7:
		res_7_config_S5K3L6(module);
		break;
	default:
		ISP_PR_ERR("unsupported res\n");
		return RET_FAILURE;
	}
	vcm_PD50_initial(module);
	ISP_PR_INFO("success\n");
	return RET_SUCCESS;
}

static int stop_S5K3L6(struct sensor_module_t *module)
{
	ISP_PR_INFO("device id[%d]\n", module->id);
//	SENSOR_I2C_WRITE(module->hw->sensor_i2c_index, 0x0100, 0x00);
	ISP_PR_INFO("success\n");

	return RET_SUCCESS;
}

static int stream_on_S5K3L6(struct sensor_module_t __maybe_unused *module)
{
	ISP_PR_INFO("device id[%d]\n", module->id);
//	SENSOR_I2C_WRITE(module->hw->sensor_i2c_index, 0x0100, 0x01);
	ISP_PR_INFO("success\n");
	return RET_SUCCESS;
}

static int get_res_fps_S5K3L6(struct sensor_module_t __maybe_unused *module,
				uint32_t res_fps_id, struct res_fps_t *res_fps)
{
	ISP_PR_INFO("res[%d]\n", res_fps_id);

	switch (res_fps_id) {
	case 0:
		res_fps->width = S5K3L6_RES_0_WIDTH;
		res_fps->height = S5K3L6_RES_0_HEIGHT;
		res_fps->fps = S5K3L6_RES_0_MAX_FPS;
		res_fps->none_hdr_support = 1;
		res_fps->hdr_support = 0;
		res_fps->index = res_fps_id;
		res_fps->valid = TRUE;
		break;
	case 1:
		res_fps->width = S5K3L6_RES_1_WIDTH;
		res_fps->height = S5K3L6_RES_1_HEIGHT;
		res_fps->fps = S5K3L6_RES_1_MAX_FPS;
		res_fps->none_hdr_support = 1;
		res_fps->hdr_support = 0;
		res_fps->index = res_fps_id;
		res_fps->valid = TRUE;
		break;
	case 2:
		res_fps->width = S5K3L6_RES_2_WIDTH;
		res_fps->height = S5K3L6_RES_2_HEIGHT;
		res_fps->fps = S5K3L6_RES_2_MAX_FPS;
		res_fps->none_hdr_support = 1;
		res_fps->hdr_support = 0;
		res_fps->index = res_fps_id;
		res_fps->valid = TRUE;
		break;
	case 3:
		res_fps->width = S5K3L6_RES_3_WIDTH;
		res_fps->height = S5K3L6_RES_3_HEIGHT;
		res_fps->fps = S5K3L6_RES_3_MAX_FPS;
		res_fps->none_hdr_support = 1;
		res_fps->hdr_support = 0;
		res_fps->index = res_fps_id;
		res_fps->valid = TRUE;
		break;
	case 4:
		res_fps->width = S5K3L6_RES_4_WIDTH;
		res_fps->height = S5K3L6_RES_4_HEIGHT;
		res_fps->fps = S5K3L6_RES_4_MAX_FPS;
		res_fps->none_hdr_support = 1;
		res_fps->hdr_support = 0;
		res_fps->index = res_fps_id;
		res_fps->valid = FALSE;
		break;
	case 5:
		res_fps->width = S5K3L6_RES_5_WIDTH;
		res_fps->height = S5K3L6_RES_5_HEIGHT;
		res_fps->fps = S5K3L6_RES_5_MAX_FPS;
		res_fps->none_hdr_support = 1;
		res_fps->hdr_support = 0;
		res_fps->index = res_fps_id;
		res_fps->valid = FALSE;
		break;
	case 6:
		res_fps->width = S5K3L6_RES_6_WIDTH;
		res_fps->height = S5K3L6_RES_6_HEIGHT;
		res_fps->fps = S5K3L6_RES_6_MAX_FPS;
		res_fps->none_hdr_support = 1;
		res_fps->hdr_support = 0;
		res_fps->index = res_fps_id;
		res_fps->valid = TRUE;
		break;
	case 7:
		res_fps->width = S5K3L6_RES_7_WIDTH;
		res_fps->height = S5K3L6_RES_7_HEIGHT;
		res_fps->fps = S5K3L6_RES_7_MAX_FPS;
		res_fps->none_hdr_support = 1;
		res_fps->hdr_support = 0;
		res_fps->index = res_fps_id;
		res_fps->valid = TRUE;
		break;
	default:
		ISP_PR_INFO("unsupported res:%d\n", res_fps_id);
		return RET_FAILURE;
	}

	ISP_PR_INFO("success\n");

	return RET_SUCCESS;
}

static int sned_get_fralenline_cmd_S5K3L6(
				struct sensor_module_t *module)
{
	//uint32_t temp_h = 0 , temp_l = 0;

	init_S5K3L6_context(module->context.res);

	//SENSOR_I2C_READ_FW(module->id, module->hw->sensor_i2c_index, 0x0340,
			//&temp_h, 2);
	//SENSOR_I2C_READ_FW(module->id, module->hw->sensor_i2c_index,
	//		0x0341, &temp_l);

	return RET_SUCCESS;
}

static int get_fralenline_regaddr_S5K3L6(
	struct sensor_module_t __maybe_unused *module,
	unsigned short *hi_addr, unsigned short *lo_addr)
{

	*hi_addr = S5K3L6_FraLenLineHiAddr;
	*lo_addr = S5K3L6_FraLenLineLoAddr;

	return RET_SUCCESS;
}

static int get_caps_S5K3L6(struct sensor_module_t *module,
			   struct _isp_sensor_prop_t *caps, int prf_id)
{
	struct drv_context *drv_context = &module->context;
	u32 max_fps = 30;

	caps->window.h_offset = 0;
	caps->window.v_offset = 0;
	caps->frame_rate = 0;

	if (prf_id == -1)
		prf_id = drv_context->res;

	ISP_PR_DBG("res[%d]\n", prf_id);
	switch (prf_id) {
	case 0:
		caps->window.h_size = S5K3L6_RES_0_WIDTH;
		caps->window.v_size = S5K3L6_RES_0_HEIGHT;
		caps->frame_rate =
			S5K3L6_RES_0_MAX_FPS * S5K3L6_FPS_ACCUACY_FACTOR;
		caps->prop.intfProp.mipi.numLanes = 4;
		caps->prop.ae.frameLengthLines =
			S5K3L6_RES_0_FRAME_LENGTH_LINES;
		caps->prop.ae.timeOfLine =
			FP_UP(S5K3L6_RES_0_LINE_LENGTH_PCLK) /
			(S5K3L6_RES_0_SYSCLK / 1000000) / 4;
		caps->mipi_bitrate = 600;
		break;
	case 1:
		caps->window.h_size = S5K3L6_RES_1_WIDTH;
		caps->window.v_size = S5K3L6_RES_1_HEIGHT;
		caps->frame_rate =
			S5K3L6_RES_1_MAX_FPS * S5K3L6_FPS_ACCUACY_FACTOR;
		caps->prop.intfProp.mipi.numLanes = 4;
		caps->prop.ae.frameLengthLines =
			S5K3L6_RES_1_FRAME_LENGTH_LINES;
		caps->prop.ae.timeOfLine =
			FP_UP(S5K3L6_RES_1_LINE_LENGTH_PCLK) /
			(S5K3L6_RES_1_SYSCLK / 1000000) / 4;
		caps->mipi_bitrate = 600;
		break;
	case 2:
		caps->window.h_size = S5K3L6_RES_2_WIDTH;
		caps->window.v_size = S5K3L6_RES_2_HEIGHT;
		caps->frame_rate =
			S5K3L6_RES_2_MAX_FPS * S5K3L6_FPS_ACCUACY_FACTOR;
		caps->prop.intfProp.mipi.numLanes = 4;
		caps->prop.ae.frameLengthLines =
			S5K3L6_RES_2_FRAME_LENGTH_LINES;
		caps->prop.ae.timeOfLine =
			FP_UP(S5K3L6_RES_2_LINE_LENGTH_PCLK) /
			(S5K3L6_RES_2_SYSCLK / 1000000) / 4;
		caps->mipi_bitrate = 600;
		break;
	case 3:
		caps->window.h_size = S5K3L6_RES_3_WIDTH;
		caps->window.v_size = S5K3L6_RES_3_HEIGHT;
		caps->frame_rate =
			S5K3L6_RES_3_MAX_FPS * S5K3L6_FPS_ACCUACY_FACTOR;
		caps->prop.intfProp.mipi.numLanes = 4;
		caps->prop.ae.frameLengthLines =
			S5K3L6_RES_3_FRAME_LENGTH_LINES;
		caps->prop.ae.timeOfLine =
			FP_UP(S5K3L6_RES_3_LINE_LENGTH_PCLK) /
			(S5K3L6_RES_3_SYSCLK / 1000000) / 4;
		caps->mipi_bitrate = 568;
		break;
	case 4:
		caps->window.h_size = S5K3L6_RES_4_WIDTH;
		caps->window.v_size = S5K3L6_RES_4_HEIGHT;
		caps->frame_rate =
			S5K3L6_RES_4_MAX_FPS * S5K3L6_FPS_ACCUACY_FACTOR;
		caps->prop.intfProp.mipi.numLanes = 4;
		caps->prop.ae.frameLengthLines =
			S5K3L6_RES_4_FRAME_LENGTH_LINES;
		caps->prop.ae.timeOfLine = g_S5K3L6_context.t_line_ns;
		caps->mipi_bitrate = 600;
		break;
	case 5:
		caps->window.h_size = S5K3L6_RES_5_WIDTH;
		caps->window.v_size = S5K3L6_RES_5_HEIGHT;
		caps->frame_rate = S5K3L6_RES_5_MAX_FPS *
					S5K3L6_FPS_ACCUACY_FACTOR;
		caps->prop.intfProp.mipi.numLanes = 4;
		caps->prop.ae.frameLengthLines =
			S5K3L6_RES_5_FRAME_LENGTH_LINES;
		caps->prop.ae.timeOfLine = g_S5K3L6_context.t_line_ns;
		caps->mipi_bitrate = 600;
		break;
	case 6:
		caps->window.h_size = S5K3L6_RES_6_WIDTH;
		caps->window.v_size = S5K3L6_RES_6_HEIGHT;
		caps->frame_rate = S5K3L6_RES_6_MAX_FPS *
					S5K3L6_FPS_ACCUACY_FACTOR;
		caps->prop.intfProp.mipi.numLanes = 4;
		caps->prop.ae.frameLengthLines =
			S5K3L6_RES_6_FRAME_LENGTH_LINES;
		caps->prop.ae.timeOfLine =
			FP_UP(S5K3L6_RES_6_LINE_LENGTH_PCLK) /
			(S5K3L6_RES_6_SYSCLK / 1000000) / 4;
		caps->mipi_bitrate = 1104;
		break;
	case 7:
		caps->window.h_size = S5K3L6_RES_7_WIDTH;
		caps->window.v_size = S5K3L6_RES_7_HEIGHT;
		caps->frame_rate =
			S5K3L6_RES_7_MAX_FPS * S5K3L6_FPS_ACCUACY_FACTOR;
		caps->prop.intfProp.mipi.numLanes = 2;
		caps->prop.ae.frameLengthLines =
			S5K3L6_RES_7_FRAME_LENGTH_LINES;
		caps->prop.ae.timeOfLine =
			FP_UP(S5K3L6_RES_7_LINE_LENGTH_PCLK) /
			(S5K3L6_RES_7_SYSCLK / 1000000) / 4;
		caps->mipi_bitrate = 300;
		break;
	default:
		ISP_PR_ERR("unsupported res\n");
		return RET_FAILURE;
	}

#ifdef ENABLE_PPI
	caps->prop.intfType = SENSOR_INTF_TYPE_MIPI;
	caps->prop.intfProp.mipi.virtChannel = 0;
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
	caps->prop.cfaPattern = CFA_PATTERN_GRBG;
	caps->prop.sensorShutterType = SENSOR_SHUTTER_TYPE_ROLLING;

	caps->prop.itimeDelayFrames = 2;
	caps->prop.gainDelayFrames = 2;
#ifdef S5K3L6_EMBEDDED_DATA_EN
		caps->prop.hasEmbeddedData	= TRUE;
#else
		caps->prop.hasEmbeddedData	= FALSE;
#endif
	caps->prop.ae.type = SENSOR_AE_PROP_TYPE_OV;
	caps->prop.ae.baseIso = S5K3L6_BASE_ISO;
	caps->prop.ae.minExpoLine = S5K3L6_MIN_EXPOSURE_LINE;
	caps->prop.ae.maxExpoLine = S5K3L6_MAX_EXPOSURE_LINE;
	caps->prop.ae.expoLineAlpha = S5K3L6_LINE_COUNT_OFFSET;
	caps->prop.ae.minAnalogGain = (uint32_t)S5K3L6_MIN_AGAIN;
	caps->prop.ae.maxAnalogGain = (uint32_t)S5K3L6_MAX_AGAIN;
	caps->prop.ae.initItime = exp_config[prf_id].coarse_integ_time *
					FP_DN(caps->prop.ae.timeOfLine);
	caps->prop.ae.initAgain =
		FP_DN(INT_DIV(FP_UP(exp_config[prf_id].a_gain_global), 32) *
		      (uint32_t)GAIN_ACCURACY_FACTOR + FP_05);

	caps->prop.ae.expoOffset = 0;
	caps->prop.ae.formula.weight1 = S5K3L6_GAIN_FACTOR;
	caps->prop.ae.formula.weight2 = 0;
	caps->prop.ae.formula.minShift = 0;
	caps->prop.ae.formula.maxShift = 0;
	caps->prop.ae.formula.minParam = S5K3L6_MIN_GAIN_SETTING;
	caps->prop.ae.formula.maxParam = S5K3L6_MAX_GAIN_SETTING;

	caps->prop.af.min_pos = S5K3L6_VCM_MIN_POS;
	caps->prop.af.max_pos = S5K3L6_VCM_MAX_POS;
	caps->prop.af.init_pos = S5K3L6_VCM_INIT_POS;

	caps->exposure.min_digi_gain = (uint32_t) S5K3L6_MIN_DGAIN;
	caps->exposure.max_digi_gain = (uint32_t) S5K3L6_MAX_DGAIN;
	caps->hdr_valid = 0;
	caps->exposure.min_integration_time =
			FP_DN(2 * caps->prop.ae.timeOfLine);
	caps->exposure.max_integration_time =
		min((uint32_t)(1000000 / max_fps),
		    (uint32_t)(caps->prop.ae.frameLengthLines *
			       FP_DN(caps->prop.ae.timeOfLine)));
	caps->exposure_orig = caps->exposure;
	caps->exposure.min_gain = (uint32_t) S5K3L6_MIN_AGAIN;
	caps->exposure.max_gain = (uint32_t) S5K3L6_MAX_AGAIN;
	return RET_SUCCESS;
}

static int get_gain_limit_S5K3L6(struct sensor_module_t __maybe_unused *module,
			uint32_t *min_gain, uint32_t *max_gain)
{
	*min_gain = (uint32_t) S5K3L6_MIN_AGAIN;
	*max_gain = (uint32_t) S5K3L6_MAX_AGAIN;

	return RET_SUCCESS;
}

static int get_emb_prop_S5K3L6(struct sensor_module_t __maybe_unused *module,
				bool *supported,
				struct _SensorEmbProp_t *embProp)
{
	*supported = TRUE;
	embProp->virtChannel = 0x0;
	embProp->dataType = 0x12;
	embProp->embDataWindow.h_offset = 0;
	embProp->embDataWindow.v_offset = 0;
	embProp->embDataWindow.h_size = 100;
	embProp->embDataWindow.v_size = 2;
	embProp->expoStartByteOffset = 167;
	embProp->expoNeededBytes = 9;
	return RET_SUCCESS;
}

static int get_integration_time_limit_S5K3L6(
					struct sensor_module_t *module,
					uint32_t *
					min_integration_time,
					uint32_t *
					max_integration_time)
{
	//struct drv_context *drv_context = &module->context;
	uint32_t max_fps = 30;

	ISP_PR_INFO("get_itime_limit_S5K3L6\n");
	ISP_PR_INFO("res = %d\n", module->context.res);
	//ISP_PR_INFO("t_line = %d\n", g_S5K3L6_context.t_line);

	init_S5K3L6_context(module->context.res);
	*min_integration_time = FP_DN(2 * g_S5K3L6_context.t_line_ns);
	*max_integration_time =
		min((uint32_t)(1000000 / max_fps), (uint32_t)
		(g_S5K3L6_context.frame_length_lines *
		FP_DN(g_S5K3L6_context.t_line_ns)));

	ISP_PR_INFO("min integration time limit = %d(us)\n",
			*min_integration_time);

	ISP_PR_INFO("max integration time limit = %d(us)\n",
			*max_integration_time);
	ISP_PR_INFO("<- get_itime_limit_S5K3L6, success\n");
	return RET_SUCCESS;
}

static int support_af_S5K3L6(struct sensor_module_t __maybe_unused *module,
				uint32_t *support)
{
	*support = 1;

	return RET_SUCCESS;
}

static int get_i2c_config_S5K3L6(struct sensor_module_t __maybe_unused *module,
		uint32_t *i2c_addr, uint32_t __maybe_unused *mode,
				uint32_t *reg_addr_width)
{

	*i2c_addr = GROUP0_I2C_SLAVE_ADDR;
	*mode = I2C_DEVICE_ADDR_TYPE_7BIT;
	*reg_addr_width = I2C_REG_ADDR_TYPE_16BIT;

	return RET_SUCCESS;
}

static int get_fw_script_ctrl_S5K3L6(struct sensor_module_t *module,
				struct _isp_fw_script_ctrl_t *script)
{
	ISP_PR_INFO("device id %d\n", module->id);

	// get I2C prop of the sensor
	script->i2cAe.deviceId = I2C_DEVICE_ID_INVALID;
	script->i2cAe.deviceAddr = GROUP0_I2C_SLAVE_ADDR;
	script->i2cAe.deviceAddrType = I2C_DEVICE_ADDR_TYPE_7BIT;
	script->i2cAe.regAddrType = I2C_REG_ADDR_TYPE_16BIT;
	script->i2cAe.enRestart = 1;
	//AE
	script->argAe.args[0] = MIPI_DATA_TYPE_RAW_10;
	script->argAe.numArgs = 1;

	// VCM driver IC uses the same i2c bus as the sensor
	script->i2cAf.deviceId = I2C_DEVICE_ID_INVALID;
	script->i2cAf.deviceAddr = GROUP2_I2C_SLAVE_ADDR;
	script->i2cAf.deviceAddrType = I2C_DEVICE_ADDR_TYPE_7BIT;
	script->i2cAf.regAddrType = I2C_REG_ADDR_TYPE_8BIT;
	script->i2cAf.enRestart =  1;

	//AF
	script->argAf.numArgs = 0;

	//Flash
	script->argFl.numArgs = 0;

	ISP_PR_INFO("success\n");

	return RET_SUCCESS;
}

static int get_sensor_hw_parameter_S5K3L6(
	struct sensor_module_t __maybe_unused *module,
				struct sensor_hw_parameter *para)
{

	ISP_PR_INFO("device id[%d]\n", module->id);
	if (para) {
		para->type = CAMERA_TYPE_RGB_BAYER;
		para->focus_len = 1;
		para->support_hdr = 0;
		para->min_lens_pos = S5K3L6_VCM_MIN_POS;
		para->max_lens_pos = S5K3L6_VCM_MAX_POS;
		para->init_lens_pos = S5K3L6_VCM_INIT_POS;
		para->lens_step = 1;
		para->normalized_focal_len_x = 28;
		para->normalized_focal_len_y = 25;
	} else {
		ISP_PR_ERR("failed for NULL para\n");
		return RET_FAILURE;
	}

	ISP_PR_INFO("suc\n");
	return RET_SUCCESS;
}

static int set_test_pattern_S5K3L6(
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
	default:
		ISP_PR_ERR("Unsupported test pattern mode :%d\n", mode);
		return RET_FAILURE;
	}

	return RET_SUCCESS;
}

struct sensor_module_t g_S5K3L6_driver = {
	.id = 0,
	.hw = NULL,
	.name = "S5K3L6",
	.poweron_sensor = poweron_S5K3L6,
	.reset_sensor = reset_S5K3L6,
	.config_sensor = config_S5K3L6,
	.start_sensor = start_S5K3L6,
	.stop_sensor = stop_S5K3L6,
	.stream_on_sensor = stream_on_S5K3L6,
	.get_caps = get_caps_S5K3L6,
	.get_gain_limit = get_gain_limit_S5K3L6,
	.get_integration_time_limit = get_integration_time_limit_S5K3L6,
	.exposure_control = exposure_control_S5K3L6,
	.get_current_exposure = get_current_exposure_S5K3L6,
	.support_af = support_af_S5K3L6,
	.hdr_support = NULL,
	.hdr_get_expo_ratio = NULL,
	.hdr_set_expo_ratio = NULL,
	.hdr_set_gain_mode = NULL,
	.hdr_set_gain = NULL,
	.hdr_enable = NULL,
	.hdr_disable = NULL,
	.get_emb_prop = get_emb_prop_S5K3L6,
	.get_res_fps = get_res_fps_S5K3L6,
	.sned_get_fralenline_cmd = sned_get_fralenline_cmd_S5K3L6,
	.get_fralenline_regaddr = get_fralenline_regaddr_S5K3L6,
	.get_i2c_config = get_i2c_config_S5K3L6,
	.get_fw_script_ctrl = get_fw_script_ctrl_S5K3L6,
	.get_analog_gain = get_analog_gain_S5K3L6,
	.get_itime = get_itime_S5K3L6,
	.get_sensor_hw_parameter = get_sensor_hw_parameter_S5K3L6,
	.set_test_pattern = set_test_pattern_S5K3L6,
	.get_m2m_data = get_m2m_data_S5K3L6,
};
