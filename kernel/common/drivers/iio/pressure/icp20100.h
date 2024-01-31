/* SPDX-License-Identifier: GPL-2.0-only */
/* Copyright (c) 2021, Magic Leap, Inc. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef ICP20100_H
#define ICP20100_H

#define ICP20100_DEVICE_ID (0x63)
#define ICP20100_OTP_EN BIT(1)
#define ICP20100_OTP_WR BIT(0)
#define ICP20100_OTP_RESET BIT(7)
#define ICP20100_OTP_DBG2_MASK BIT(7)
#define ICP20100_TRIM1_MSB_MASK 0x3F
#define ICP20100_PEFE_GAIN_TRIM_MASK 0x70
#define ICP20100_TRIM2_LSB_MASK 0x7F
#define ICP20100_POWER_MODE 2
#define ICP20100_COMMAND 4
#define ICP20100_FIFO_FLUSH_MASK BIT(7)
#define ICP20100_FIFO_EMPTY_MASK BIT(6)
#define ICP20100_MODE_SELECT_MEAS_CONFIG_SHIFT 5
#define ICP20100_MEAS_MODE_MASK BIT(3)
#define ICP20100_FIR_ENABLE BIT(5)
#define ICP20100_IIR_ENABLE BIT(6)
#define ICP20100_FORCE_MEAS_TRIGGER BIT(4)
#define ICP20100_BOOT_STATUS BIT(0)

#define ICP20100_REV_A   (0x00)
#define ICP20100_REV_B   (0xB2)

enum ICP20100_REGISTERS {
	TRIM1_MSB = 0x5,
	TRIM2_LSB,
	TRIM2_MSB,
	DEVICE_ID = 0xC,
	IO_DRIVE_STRENGTH = 0xD,
	MODE0_OSR_PRESS = 0x14,
	MODE0_CONFIG1 = 0x15,
	MODE0_ODR_LSB = 0x16,
	MODE0_CONFIG2 = 0x17,
	MODE0_BS_VALUE = 0x18,
	GAIN_FACTOR_PRESS_MODE0_LSB = 0x7A,
	GAIN_FACTOR_PRESS_MODE0_MSB = 0x7B,
	GAIN_FACTOR_PRESS_MODE4_LSB = 0x82,
	GAIN_FACTOR_PRESS_MODE4_MSB,
	OTP_CONFIG1 = 0xAC,
	OTP_MR_LSB,
	OTP_MR_MSB,
	OTP_MRA_LSB,
	OTP_MRA_MSB,
	OTP_MRB_LSB,
	OTP_MRB_MSB,
	OTP_ADDRESS_REG = 0xB5,
	OTP_COMMAND_REG,
	OTP_RDATA = 0xB8,
	OTP_STATUS,
	OTP_DBG2 = 0xBC,
	MASTER_LOCK = 0xBE,
	OTP_STATUS2 = 0xBF,
	MODE_SELECT = 0xC0,
	INTERRUPT_STATUS = 0xC1,
	INTERRUPT_MASK,
	FIFO_CONFIG = 0xC3,
	FIFO_FILL = 0xC4,
	SPI_MODE = 0xC5,
	PRESS_ABS_LSB = 0xC7,
	PRESS_ABS_MSB,
	PRESS_DELTA_LSB,
	PRESS_DELTA_MSB,
	DEVICE_STATUS = 0xCD,
	VERSION = 0xD3,
	PRESS_DATA_0 = 0xFA,
	PRESS_DATA_1,
	PRESS_DATA_2,
	TEMP_DATA_0,
	TEMP_DATA_1,
	TEMP_DATA_2,
};

int icp20100_probe(struct device        *parent,
		  const char           *name,
		  int                   irq,
		  unsigned int          hwid,
		  struct regmap        *regmap);

void icp20100_remove(struct device *parent);
void icp20100_shutdown(struct device *parent);

#endif /* ICP20100_H */
