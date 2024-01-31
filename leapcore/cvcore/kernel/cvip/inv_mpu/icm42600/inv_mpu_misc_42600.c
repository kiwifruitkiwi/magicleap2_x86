/*
 * Copyright (C) 2018-2020 InvenSense, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#define pr_fmt(fmt) "inv_mpu: " fmt

#include "../inv_mpu_iio.h"

/**
 * inv_get_apex_enabled() - Check if any APEX feature is enabled
 * @st: struct inv_mpu_state.
 *
 * Return: true when any is enabled, otherwise false.
 */
bool inv_get_apex_enabled(struct inv_mpu_state *st)
{
	if (!(st->apex_supported))
		return false;

	if (st->step_detector_l_on ||
		st->step_detector_wake_l_on ||
		st->step_counter_l_on ||
		st->step_counter_wake_l_on)
		return true;
	if (st->chip_config.tilt_enable)
		return true;
	if (st->chip_config.tap_enable)
		return true;
	if (st->chip_config.pick_up_enable)
		return true;
	if (st->smd.on)
		return true;

	return false;
}

/**
 * inv_get_apex_odr() - Get min accel ODR according to enabled APEX feature
 * @st: struct inv_mpu_state.
 *
 * Return: min accel ODR in Hz
 */
int inv_get_apex_odr(struct inv_mpu_state *st)
{
	int odr_hz;

#ifdef SUPPORT_ACCEL_LPM
	odr_hz = MPU_INIT_SENSOR_RATE_LPM;
#else
	odr_hz = MPU_INIT_SENSOR_RATE_LNM;
#endif
	if (st->apex_supported) {
		/* returns min accel rate for each algorithm */
		if (st->chip_config.tap_enable)
			odr_hz = 200;
		else if (st->step_detector_l_on ||
			st->step_detector_wake_l_on ||
			st->step_counter_l_on ||
			st->step_counter_wake_l_on ||
			st->chip_config.tilt_enable ||
			st->chip_config.pick_up_enable ||
			st->smd.on)
			odr_hz = 50;
	}

	return odr_hz;
}

/**
 * inv_run_auth() - Run authentication
 * @st: struct inv_mpu_state.
 *
 * Return: 0 when no error, otherwise errno
 */
int inv_run_auth(struct inv_mpu_state *st)
{
#define REG_SCAN0		0x71	/* bank 0 */
#define REG_SEC_AUTH0		0x6F	/* bank 1 */
#define REG_OTP_SEC_STATUS	0x70	/* bank 1 */

	int i;
	u8 data;
	u8 reg_pwr_mgmt_0, reg_scan0, reg_sec_auth0;
	int ret = 0;

	pr_info("auth: start\n");

	/* check variables */
	if (st->auth_i[8] > (int)sizeof(st->auth_o) ||
			st->auth_i[8] > (int)sizeof(st->auth_i) - 9) {
		pr_err("auth: invalid input data\n");
		return -EINVAL;
	}

	/* save registers */
	ret |= inv_plat_read(st, REG_PWR_MGMT_0, 1, &reg_pwr_mgmt_0);
	ret |= inv_plat_read(st, REG_SCAN0, 1, &reg_scan0);
	ret |= inv_plat_single_write(st, REG_REG_BANK_SEL, BIT_BANK_SEL_1);
	ret |= inv_plat_read(st, REG_SEC_AUTH0, 1, &reg_sec_auth0);
	ret |= inv_plat_single_write(st, REG_REG_BANK_SEL, BIT_BANK_SEL_0);

	/* disable all ensors and set idle mode */
	ret |= inv_plat_single_write(st, REG_PWR_MGMT_0, BIT_IDLE);
	usleep_range(1000, 1200);

	/* enable write access to memory from serial if */
	ret |= inv_plat_single_write(st, REG_SCAN0,
			0x04 /* mem_otp_access_en */ |
			0x01 /* fifo_mem_wr_ser */);

	/* enable auth mode */
	ret |= inv_plat_single_write(st, REG_REG_BANK_SEL, BIT_BANK_SEL_1);
	ret |= inv_plat_single_write(st, REG_SEC_AUTH0,
			0x02 /* sec_auth_mode=1 */);
	ret |= inv_plat_single_write(st, REG_REG_BANK_SEL, BIT_BANK_SEL_0);

	/* write data to memory 1 */
	ret |= inv_plat_single_write(st, REG_MEM_BANK_SEL, 0);
	ret |= inv_plat_single_write(st, REG_MEM_START_ADDR, 0);
	for (i = 0; i < 9; i++)
		ret |= inv_plat_single_write(st, REG_MEM_R_W, st->auth_i[i]);

	/* write data to memory 2 */
	for (i = 0; i < st->auth_i[8]; i++)
		ret |= inv_plat_single_write(st, REG_MEM_R_W, st->auth_i[9 + i]);

	/* wait for a while after writing to memory */
	usleep_range(100, 120);

	/* enable read access to memory from system */
	ret |= inv_plat_single_write(st, REG_SCAN0,
			0x04 /* mem_otp_access_en */ |
			0x02 /* fifo_mem_rd_sys */);

	/* trigger authentication */
	ret |= inv_plat_single_write(st, REG_REG_BANK_SEL, BIT_BANK_SEL_1);
	ret |= inv_plat_single_write(st, REG_SEC_AUTH0,
			0x02 /* sec_auth_mode=1 */ |
			0x01 /* sec_start=1 */);

	/* manually reset start bit */
	ret |= inv_plat_single_write(st, REG_SEC_AUTH0,
			0x02 /* sec_auth_mode=1, sec_start=0 */);

	/* wait for a while, 30ms should be good enough */
	usleep_range(30 * 1000, 50 * 1000);

	/* check status */
	ret |= inv_plat_read(st, REG_OTP_SEC_STATUS, 1, &data);
	if (!(data & 0x02 /* sec_done */)) {
		pr_info("auth: timeout 0x%02x\n", data);
		ret = -ETIME;
		goto auth_done;
	}
	pr_info("auth: done\n");

	/* enable read access to memory from system */
	ret |= inv_plat_single_write(st, REG_REG_BANK_SEL, BIT_BANK_SEL_0);
	ret |= inv_plat_single_write(st, REG_SCAN0,
			0x04 /* mem_otp_access_en */);

	/* read response */
	ret |= inv_plat_single_write(st, REG_MEM_BANK_SEL, 0);
	for (i = 0; i < st->auth_i[8]; i++) {
		ret |= inv_plat_single_write(st, REG_MEM_START_ADDR, i);
		ret |= inv_plat_read(st, REG_MEM_R_W, 1, &st->auth_o[i]);
	}

auth_done:
	ret |= inv_plat_single_write(st, REG_REG_BANK_SEL, BIT_BANK_SEL_0);

	/* restore settings */
	ret |= inv_plat_single_write(st, REG_SCAN0, reg_scan0);
	ret |= inv_plat_single_write(st, REG_REG_BANK_SEL, BIT_BANK_SEL_1);
	ret |= inv_plat_single_write(st, REG_SEC_AUTH0, reg_sec_auth0);
	ret |= inv_plat_single_write(st, REG_REG_BANK_SEL, BIT_BANK_SEL_0);
	ret |= inv_plat_single_write(st, REG_PWR_MGMT_0, reg_pwr_mgmt_0);

	pr_info("auth: end with %d\n", ret);

	return ret;
}
