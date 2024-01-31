// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2021, Magic Leap, Inc. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/regmap.h>
#include <linux/iio/iio.h>
#include <linux/iio/sysfs.h>
#include <linux/iio/buffer.h>
#include <linux/iio/trigger.h>
#include <linux/iio/trigger_consumer.h>
#include <linux/iio/triggered_buffer.h>
#include "icp20100.h"

enum icp20100_mode {
	ICP20100_MODE_0,
	ICP20100_MODE_1,
	ICP20100_MODE_2,
	ICP20100_MODE_3,
	ICP20100_MODE_4,
};

static const char * const mode_names[] = {
	"ICP20100_MODE_0",
	"ICP20100_MODE_1",
	"ICP20100_MODE_2",
	"ICP20100_MODE_3",
	"ICP20100_MODE_4"
};

struct icp20100_regmap {
	u32 reg_addr;
	char *reg_name;
};

#define ICP_REGMAP(x) \
	{ \
		.reg_addr = x, \
		.reg_name = #x \
	} \

static const struct icp20100_regmap icp20100_full_reg_list[] = {
	ICP_REGMAP(TRIM1_MSB),
	ICP_REGMAP(TRIM2_LSB),
	ICP_REGMAP(TRIM2_MSB),
	ICP_REGMAP(OTP_CONFIG1),
	ICP_REGMAP(OTP_MR_LSB),
	ICP_REGMAP(OTP_MR_MSB),
	ICP_REGMAP(OTP_MRA_LSB),
	ICP_REGMAP(OTP_MRA_MSB),
	ICP_REGMAP(OTP_MRB_LSB),
	ICP_REGMAP(OTP_MRB_MSB),
	ICP_REGMAP(MODE0_CONFIG1),
	ICP_REGMAP(MODE0_CONFIG2),
	ICP_REGMAP(MODE0_OSR_PRESS),
	ICP_REGMAP(MODE0_ODR_LSB),
	ICP_REGMAP(MODE0_BS_VALUE),
	ICP_REGMAP(GAIN_FACTOR_PRESS_MODE0_LSB),
	ICP_REGMAP(GAIN_FACTOR_PRESS_MODE0_MSB),
	ICP_REGMAP(GAIN_FACTOR_PRESS_MODE4_LSB),
	ICP_REGMAP(GAIN_FACTOR_PRESS_MODE4_MSB),
};

struct icp20100_config_params {
	u32 offset;
	u32 gain;
	u32 HFosc;
};

struct icp20100_mode4_config {
	u32 pres_osr;
	u32 temp_osr;
	u32 odr;
	u32 mode4_press_gain;
	bool mode4_init;
};

struct icp20100_driver_info {
	bool enable_force_meas;
	bool discard_samples;
	u8 device_id;
	struct regmap *regmap;
	struct iio_trigger *trigger;
	struct device *dev;
	int irq;
	u32 mode;
	struct completion data_ready;
	struct icp20100_config_params params;
	struct icp20100_mode4_config mode4;
	u8 device_rev;
};

#define FIFO_WMK_HIGH_INTERRUPT BIT(2)
#define FIFO_OVERFLOW_INTERRUPT BIT(0)
#define CLEAR_INTERRUPT_STATUS  0xFF
#define FIFO_MAX_SIZE (16) // 0 - 16
#define HALF_FIFO_SIZE (FIFO_MAX_SIZE >> 1)
#define NUM_FIFO_DISCARD (2)
#define NUM_SAMPLES_DISCARD (15 * NUM_FIFO_DISCARD)

/*
 * 200 ms should be enough for the longest
 * conversion time in force mode.
 */
#define ICP20100_CONVERSION_JIFFIES (HZ / 5)

static void icp20100_reset_device(struct device *parent);
static int icp20100_discard_samples(struct iio_dev *indio_dev, u32 num_discard);

static int icp_reg_read(struct iio_dev *indio_dev, u8 reg_addr, u8 *reg_value)
{
	int ret;
	u32 val;
	u8 retry_count = 3;
	struct device *parent = indio_dev->dev.parent;
	struct icp20100_driver_info *st = iio_priv(indio_dev);

	do {
		ret = regmap_read(st->regmap, reg_addr, &val);
		if (ret) {
			dev_err(st->dev, "addr: 0x%02x read failed, %d",
				reg_addr, ret);
			icp20100_reset_device(parent);
			msleep_interruptible(1000);
			retry_count--;
		}
	} while ((ret != 0) && (retry_count != 0));

	if (ret != 0) {
		dev_err(st->dev, "not able to recover, %d", reg_addr, ret);
		return ret;
	}

	*reg_value = (u8)val;
	return ret;
}

static int icp_reg_write(struct iio_dev *indio_dev,
			u8 reg_addr,
			u8 reg_value)
{
	int ret;
	struct icp20100_driver_info *st = iio_priv(indio_dev);

	ret = regmap_write(st->regmap, reg_addr, reg_value);
	if (ret)
		dev_err(st->dev, "addr: 0x%02x write failed, %d",
			reg_addr, ret);

	return ret;
}

static int icp_reg_update(struct iio_dev *indio_dev,
			  u8 mask,
			  u8 reg_addr,
			  u8 reg_value)
{
	int ret;
	struct icp20100_driver_info *st = iio_priv(indio_dev);

	ret = regmap_update_bits(st->regmap, reg_addr, mask, reg_value);
	if (ret)
		dev_err(st->dev, "addr: 0x%02x update failed, %d",
			reg_addr, ret);
	return ret;
}

static void icp20100_regdump(struct iio_dev *indio_dev)
{
	u32 i;
	u8 val;
	int ret;

	for (i = 0; i < ARRAY_SIZE(icp20100_full_reg_list); i++) {
		ret = icp_reg_read(indio_dev,
				icp20100_full_reg_list[i].reg_addr, &val);
		if (ret)
			continue;
		dev_dbg(indio_dev->dev.parent, "%s ==> 0x%02x",
			icp20100_full_reg_list[i].reg_name, val);
	}
}

enum {
	BOOT_STS_WRITE = 0,
	BOOT_STS_READ,
};

/*
 * rw - 1 (reads the boot status bit)
 * rw - 0 (writes to the boot status bit)
 */
static int icp20100_boot_sts(struct iio_dev *indio_dev,
			     bool rw,
			     bool *val)
{
	int ret;
	u8 reg_val;

	if (rw) {
		ret = icp_reg_read(indio_dev, OTP_STATUS2, &reg_val);
		if (!ret)
			*val = !!(reg_val & ICP20100_BOOT_STATUS);

	} else {
		reg_val = (u8)(*val);
		ret = icp_reg_update(indio_dev, ICP20100_BOOT_STATUS,
				     OTP_STATUS2, reg_val);
	}

	return ret;
}

static int icp20100_clear_fifo(struct iio_dev *indio_dev)
{
	int ret;

	ret = icp_reg_write(indio_dev, FIFO_FILL, ICP20100_FIFO_FLUSH_MASK);
	if (ret)
		dev_err(indio_dev->dev.parent, "clearing fifo, %d", ret);

	return ret;
}

static int icp20100_clear_int_status(struct iio_dev *indio_dev)
{
	int ret;

	ret = icp_reg_write(indio_dev, INTERRUPT_STATUS,
			   CLEAR_INTERRUPT_STATUS);
	if (ret)
		dev_err(indio_dev->dev.parent, "clearing int status, %d", ret);

	return ret;
}

static int icp20100_device_status(struct iio_dev *indio_dev)
{
	int ret;
	u8 val;
	u8 retry_count = 3;

	ret = icp_reg_read(indio_dev, DEVICE_STATUS, &val);
	if (ret)
		return ret;

	while (!(val & 0x1) && retry_count--) {
		msleep_interruptible(1000);
		ret = icp_reg_read(indio_dev, DEVICE_STATUS, &val);
		if (ret)
			dev_err(indio_dev->dev.parent, "device status, %d",
				ret);
	}

	return ret;
}

static int icp20100_set_force_mode(struct iio_dev *indio_dev)
{
	int ret;
	u8  val;

	ret = icp20100_device_status(indio_dev);
	if (ret)
		return ret;

	val = (ICP20100_MODE_4 << ICP20100_MODE_SELECT_MEAS_CONFIG_SHIFT) |
	      (ICP20100_FORCE_MEAS_TRIGGER);
	ret = icp_reg_write(indio_dev, MODE_SELECT, val);
	if (ret)
		dev_err(indio_dev->dev.parent, "set force mode, %d", ret);

	return ret;
}

static int icp20100_standby(struct iio_dev *indio_dev)
{
	int ret;

	ret = icp20100_device_status(indio_dev);
	if (ret)
		return ret;

	ret = icp_reg_write(indio_dev, MODE_SELECT, 0);
	if (ret)
		dev_err(indio_dev->dev.parent, "set standby, %d", ret);

	return ret;
}

static int icp20100_set_cont_mode(struct iio_dev *indio_dev)
{
	int ret;
	struct icp20100_driver_info *st = iio_priv(indio_dev);
	u8 val;

	ret = icp20100_device_status(indio_dev);
	if (ret)
		return ret;

	val = st->mode << ICP20100_MODE_SELECT_MEAS_CONFIG_SHIFT;
	val |= ICP20100_MEAS_MODE_MASK;

	ret = icp_reg_write(indio_dev, MODE_SELECT, val);
	if (ret)
		dev_err(st->dev, "set cont. mode, %d", ret);

	return ret;
}

/* ------------------ BUFFER TRIGGER SETUP ------------------------- */

#define FULL_FIFO_LEVEL               (0x10)
#define FIFO_FILL_LEVEL_MASK          (0x1F)

static int icp20100_fill_buffer(struct iio_dev *indio_dev,
				struct icp20100_driver_info *st)
{
	int ret;
	/* 3 bytes of pressure
	 * 3 bytes of temperature
	 */
	struct sample {
		u8 press[3];
		u8 temp[3];
	} __packed;
	/* sample pushed to iio buffer
	 * needs 8 bytes for timestamp
	 */
	struct iio_sample {
		struct sample one_sample;
		u64 timestamp;
	} __packed;

	struct sample sample_arr[HALF_FIFO_SIZE];
	struct iio_sample i_sample;
	u32 i;

	dev_dbg(st->dev, "collecting samples now..");

	/* copy half the fifo elements */
	ret = regmap_bulk_read(st->regmap, PRESS_DATA_0,
			(u8 *)&sample_arr[0],
			(HALF_FIFO_SIZE*sizeof(struct sample)));
	if (ret) {
		dev_err(st->dev, "bulk read failed, %d", ret);
		return ret;
	}

	for (i = 0; i < HALF_FIFO_SIZE; i++) {
		memcpy(&i_sample.one_sample,
			(void *)&sample_arr[i], sizeof(struct sample));
		iio_push_to_buffers_with_timestamp(indio_dev,
			(void *)(&i_sample),
			iio_get_time_ns(indio_dev));
	}

	dev_dbg(st->dev, "samples pushed to iio buffers!");
	return 0;
}

#define MODE4_OSR_PRESS         (0x2C)
#define MODE4_CONFIG1           (0x2D)
#define MODE4_ODR_LSB           (0x2E)
#define MODE4_CONFIG2           (0x2F)
#define MODE4_BS_VALUE          (0x30)
#define IIR_K_FACTOR_LSB        (0x78)
#define IIR_K_FACTOR_MSB        (0x79)

static int icp20100_mode4_config(struct iio_dev *indio_dev)
{
	u32 f_curgain;
	u32 temp_bs = 0x06;
	u32 pres_bs = 0;
	u32 odr_value;
	u8 pres_osr;
	u8 temp_osr;
	u16 final_press_gain = 0;
	u8 gain_lsb;
	u8 gain_msb;
	u32 m4_init_press_gain;
	u8 val;
	int ret;

	struct icp20100_driver_info *st = iio_priv(indio_dev);

	if (st->mode4.mode4_init)
		return 0;

	ret = icp_reg_read(indio_dev, GAIN_FACTOR_PRESS_MODE4_LSB, &gain_lsb);
	if (ret)
		return ret;

	ret = icp_reg_read(indio_dev, GAIN_FACTOR_PRESS_MODE4_MSB, &gain_msb);
	if (ret)
		return ret;

	m4_init_press_gain = (u32)(gain_msb << 8) | gain_lsb;

	odr_value = st->mode4.odr;
	temp_osr = st->mode4.temp_osr & 0x1F;
	pres_osr = st->mode4.pres_osr & 0xFF;

	/* calculation obtained from invensence presentation */
	do {
		f_curgain = ((m4_init_press_gain * int_pow(256, 2)) /
			(int_pow((pres_osr+1), 2) * int_pow(2, pres_bs)));
		if (f_curgain < (0x02 << 15))
			break;
		pres_bs++;
	} while (1);

	final_press_gain = (u16)f_curgain;

	do {

		/* pressure OSR */
		ret = icp_reg_write(indio_dev, MODE4_OSR_PRESS, pres_osr);
		if (ret)
			break;

		/* temp OSR, FIR: enable IIR: enable */
		val = temp_osr | ICP20100_FIR_ENABLE | ICP20100_IIR_ENABLE;
		ret = icp_reg_write(indio_dev, MODE4_CONFIG1, val);
		if (ret)
			break;

		/* odr */
		ret = icp_reg_write(indio_dev, MODE4_ODR_LSB,
				   (u8)(0xFF & odr_value));
		if (ret)
			break;

		/* HFSOC & DVDD disabled */
		ret = icp_reg_write(indio_dev, MODE4_CONFIG2,
				   (u8)(0x1F & (odr_value >> 8)));
		if (ret)
			break;

		/* temp bs: 0x06, press_bs derived from osr and gain factor */
		ret = icp_reg_write(indio_dev, MODE4_BS_VALUE,
				   (u8)((temp_bs << 4) | (pres_bs & 0x0F)));
		if (ret)
			break;

		/* gain factor */
		ret = icp_reg_write(indio_dev, GAIN_FACTOR_PRESS_MODE4_LSB,
				   (u8)(final_press_gain & 0xFF));
		if (ret)
			break;

		ret = icp_reg_write(indio_dev, GAIN_FACTOR_PRESS_MODE4_MSB,
				   (u8)((final_press_gain >> 8) & 0xFF));
		if (ret)
			break;

		// iir filter k factor
		ret = icp_reg_write(indio_dev, IIR_K_FACTOR_LSB, 0);
		if (ret)
			break;

		ret = icp_reg_write(indio_dev, IIR_K_FACTOR_MSB, 0);
		if (ret)
			break;

		st->mode4.mode4_init = true;

	} while (0);

	if (ret)
		dev_err(st->dev, "set mode4 config failed, %d", ret);

	return ret;
}

#define ICP20100_FIFO_HIGH_WMK_MASK ~BIT(2)
#define ICP20100_FIFO_OVERFLOW_MASK ~BIT(0)

static int icp20100_enable_cont_mode(struct iio_dev *indio_dev)
{
	struct icp20100_driver_info *st = iio_priv(indio_dev);
	int ret;
	u32 count = 3;

	ret = icp20100_standby(indio_dev);
	if (ret)
		return ret;

	if (st->mode == ICP20100_MODE_4) {
		ret = icp20100_mode4_config(indio_dev);
		if (ret)
			return ret;
	} else
		st->mode4.mode4_init = false;

	/* flush fifo before starting continuous measurements */
	ret = icp20100_clear_fifo(indio_dev);
	if (ret)
		return ret;

	/* fifo high watermark set to half the fifo size */
	ret = icp_reg_write(indio_dev, FIFO_CONFIG, 0x80);
	if (ret)
		return ret;

	/* fifo high wm interrupt enabled */
	ret = icp_reg_write(indio_dev, INTERRUPT_MASK,
			   (u8)(ICP20100_FIFO_HIGH_WMK_MASK &
			   ICP20100_FIFO_OVERFLOW_MASK));
	if (ret)
		return ret;

	do {
		ret = icp20100_set_cont_mode(indio_dev);
		if (ret)
			msleep_interruptible(10);
	} while (ret && count--);

	if (ret)
		return ret;

	st->enable_force_meas = false;
	dev_dbg(st->dev, "cont. mode set successfully. wait for irq..");
	return 0;
}

static int icp20100_enable_force_mode(struct iio_dev *indio_dev)
{
	int ret;
	struct icp20100_driver_info *st = iio_priv(indio_dev);

	do {
		ret = icp20100_standby(indio_dev);
		if (ret)
			break;

		ret = icp20100_mode4_config(indio_dev);
		if (ret)
			break;

		// discard samples before starting
		if (!st->discard_samples) {
			ret = icp20100_discard_samples(indio_dev,
						       NUM_FIFO_DISCARD);
			if (ret)
				return ret;
			dev_dbg(st->dev, "%d samples discarded.",
				NUM_SAMPLES_DISCARD);
		}

		/* fifo high watermark set to 1 sample */
		ret = icp_reg_write(indio_dev, FIFO_CONFIG, (u8)(1 << 4));
		if (ret)
			break;

		/* fifo high wm interrupt enabled */
		ret = icp_reg_write(indio_dev, INTERRUPT_MASK, (u8)(~(0x4)));
		if (ret)
			break;

	} while (0);

	if (ret)
		dev_err(indio_dev->dev.parent, "failed force mode, %d", ret);

	return ret;
}

#define ICP20100_FIFO_EMPTY   (BIT(6))

static int icp20100_disable_trigger_state(struct iio_dev *indio_dev)
{
	int ret;
	struct icp20100_driver_info *st = iio_priv(indio_dev);

	do {
		// put device in standby mode
		ret = icp20100_standby(indio_dev);
		if (ret)
			break;

		//disable interrupt line
		disable_irq(st->irq);

		// clear fifo
		ret = icp20100_clear_fifo(indio_dev);
		if (ret)
			break;

		// clear interrupt status
		ret = icp20100_clear_int_status(indio_dev);
		if (ret)
			break;

		ret = iio_triggered_buffer_predisable(indio_dev);
		if (ret) {
			dev_err(st->dev, "fail. buffer disable, %d", ret);
			break;
		}

		dev_dbg(st->dev, "predisable buffer success!");
	} while (0);

	enable_irq(st->irq);

	if (ret)
		dev_err(st->dev, "disable trig. failed, %d", ret);

	return ret;
}

/* Each call to this function will discard 15 samples.
 * num_discard means the number of discards to perform
 */
static int icp20100_discard_samples(struct iio_dev *indio_dev,
				    u32 num_discard)
{
	int ret;
	struct icp20100_driver_info *st = iio_priv(indio_dev);
	s64 timeout;
	u32 count = 3;

	while (num_discard) {
		// put device in standby mode
		ret = icp20100_standby(indio_dev);
		if (ret)
			break;

		ret = icp_reg_write(indio_dev, FIFO_CONFIG, 0xF0);
		if (ret)
			break;

		ret = icp_reg_write(indio_dev, INTERRUPT_MASK, 0xFB);
		if (ret)
			break;

		do {
			ret = icp20100_set_cont_mode(indio_dev);
			if (ret)
				msleep_interruptible(10);
		} while (ret && count--);

		if (ret)
			break;

		/* sleep before probing as sometimes first few sample
		 * dont arrive quickly
		 */
		msleep_interruptible(100);

		timeout = wait_for_completion_interruptible_timeout(
			&st->data_ready, HZ);
		if (timeout <= 0) {
			dev_err(st->dev, "samples discard timeout");
			ret = -ETIME;
			break;
		}

		ret = icp20100_standby(indio_dev);
		if (ret)
			break;

		msleep_interruptible(10);

		ret = icp20100_clear_fifo(indio_dev);
		if (ret)
			break;

		num_discard--;
	}

	if (ret || !!num_discard)
		dev_err(st->dev, "discarding %d samples failed, %d",
			num_discard, ret);
	else
		st->discard_samples = true;

	return ret;
}

static int icp20100_set_trigger_state(struct iio_trigger *trig, bool state)
{
	struct iio_dev *indio_dev = dev_get_drvdata(trig->dev.parent);
	struct icp20100_driver_info *st = iio_priv(indio_dev);
	int ret = 0;

	dev_dbg(st->dev, "continuous meas. %s", state ? "start" : "stopped");
	if (state) {
		/* This only runs once at powerup. As per
		 * datasheet, when FIR filter is enabled
		 * the first 15 samples need to be discarded.
		 * We are discarding the necessary needed
		 * based on our test results.
		 */
		if (!st->discard_samples) {
			ret = icp20100_discard_samples(indio_dev,
						       NUM_FIFO_DISCARD);
			if (ret)
				return ret;
			dev_dbg(st->dev, "%d samples discarded.",
				NUM_SAMPLES_DISCARD);
		}
		ret = icp20100_enable_cont_mode(indio_dev);
	}

	if (ret)
		icp20100_regdump(indio_dev);

	return ret;
}

static const struct iio_trigger_ops icp20100_trigger_ops = {
	.set_trigger_state = icp20100_set_trigger_state,
};

static irqreturn_t icp20100_trigger_handler(int irq, void *data)
{
	struct iio_dev *indio_dev = ((struct iio_poll_func *)data)->indio_dev;
	struct icp20100_driver_info *st = iio_priv(indio_dev);
	int ret;
	u8 dummy_reg;

	dev_dbg(st->dev, "trigger handled invoked");
	ret = icp20100_fill_buffer(indio_dev, st);
	if (ret)
		dev_err(st->dev, "fill buffer failed, %d", ret);

	/*
	 * Perform dummy read to 0x00 register as last transaction
	 * after FIFO read for I2C interface. Recommended by Invensense.
	 */
	ret = icp_reg_read(indio_dev, 0x00, &dummy_reg);
	if (ret)
		dev_err(st->dev, "dummy reg read fail, %d", ret);

	/* Inform attached trigger we are done. */
	iio_trigger_notify_done(indio_dev->trig);
	return IRQ_HANDLED;
}

static int icp20100_init_trigger_setup(struct device *dev,
				       struct iio_dev *indio_dev,
				       struct icp20100_driver_info *st,
				       int irq)
{
	struct iio_trigger *trigger;
	int ret;

	trigger = devm_iio_trigger_alloc(dev, "%s-dev%d",
					 indio_dev->name, indio_dev->id);
	if (!trigger)
		return -ENOMEM;

	trigger->dev.parent = dev;
	trigger->ops = &icp20100_trigger_ops;
	st->trigger = trigger;

	/* Register to triggers space. */
	ret = devm_iio_trigger_register(dev, trigger);
	if (ret)
		dev_err(dev, "failed to register hardware trigger (%d)", ret);

	return ret;
}

const struct iio_buffer_setup_ops icp20100_buffer_setup_ops = {
	.postenable  = iio_triggered_buffer_postenable,
	.predisable  = icp20100_disable_trigger_state,
};

/* ------------------ BUFFER TRIGGER SETUP ------------------------- */

/* ----------------------INTERRUPT SETUP --------------------------- */

static irqreturn_t icp20100_handle_threaded_irq(int irq, void *data)
{
	struct iio_dev *indio_dev = data;
	struct icp20100_driver_info *st = iio_priv(indio_dev);
	u8 val;
	int ret;

	ret = icp_reg_read(indio_dev, INTERRUPT_STATUS, &val);
	if (ret)
		return IRQ_NONE;

	icp20100_clear_int_status(indio_dev);

	if (!(val & (FIFO_WMK_HIGH_INTERRUPT | FIFO_OVERFLOW_INTERRUPT))) {
		dev_warn(st->dev, "unknown interrupt fired %08x", val);
		return IRQ_NONE;
	}

	if (st->enable_force_meas || !st->discard_samples) {
		complete(&st->data_ready);
		return IRQ_HANDLED;
	}

	iio_trigger_poll_chained(st->trigger);
	return IRQ_HANDLED;
}

static int icp20100_init_interrupt_handler(struct device *dev,
					struct iio_dev *indio_dev,
					struct icp20100_driver_info *st,
					int irq)
{
	int ret;

	if (irq <= 0) {
		dev_err(dev, "invalid interrupt, %d", irq);
		return -EINVAL;
	}

	init_completion(&st->data_ready);

	// Request handler to be scheduled into threaded interrupt context.
	ret = devm_request_threaded_irq(dev, irq, NULL,
					icp20100_handle_threaded_irq,
					IRQF_TRIGGER_LOW | IRQF_ONESHOT,
					dev_name(dev), indio_dev);
	if (ret) {
		dev_err(dev, "failed to request interrupt %d (%d)", irq, ret);
		return ret;
	}

	dev_dbg(dev, "using interrupt %d", irq);

	return 0;
}

/* ----------------------INTERRUPT SETUP --------------------------- */

static int icp20100_get_device_id(struct iio_dev *indio_dev, u8 *id)
{
	int ret;
	u8 val;

	ret = icp_reg_read(indio_dev, DEVICE_ID, &val);
	if (ret)
		return ret;

	dev_dbg(indio_dev->dev.parent, "Device ID: %02x\n", val);
	*id = val;
	return ret;
}

static int icp20100_read_otp_data(struct iio_dev *indio_dev,
				  u16 otp_address,
				  u8 otp_cmd,
				  u32 *value)
{
	int ret;
	u8 val;
	u8 otp_address_lsb;
	u8 otp_address_msb;
	u8 otp_read_value;
	int retry_count = 10;

	otp_address_lsb = otp_address & 0xFF;
	ret = icp_reg_write(indio_dev, OTP_ADDRESS_REG, otp_address_lsb);
	if (ret)
		return ret;

	otp_address_msb = (otp_address >> 8) & 0xF;
	val = (otp_cmd << ICP20100_COMMAND) | otp_address_msb;
	ret = icp_reg_write(indio_dev, OTP_COMMAND_REG, val);
	if (ret)
		return ret;

	do {
		usleep_range(10, 12);
		ret = icp_reg_read(indio_dev, OTP_STATUS, &val);
		if (ret)
			return ret;
	} while ((val & 0x1) && retry_count--);

	// read OTP_RDATA
	ret = icp_reg_read(indio_dev, OTP_RDATA, &otp_read_value);
	if (ret)
		return ret;

	*value = otp_read_value;

	dev_dbg(indio_dev->dev.parent,
		"otp addr: %d, otp cmd: %d, otp val: %d",
		otp_address, otp_cmd, otp_read_value);

	return 0;
}

#define OTP_OFFSET  (0xF8)
#define OTP_GAIN    (0xF9)
#define OTP_HFOSC   (0xFA)

static int icp20100_config(struct iio_dev *indio_dev)
{
	int ret = 0;
	u8 val;
	struct icp20100_driver_info *st = iio_priv(indio_dev);

	do {
		val = (ICP20100_OTP_EN | ICP20100_OTP_WR);
		ret = icp_reg_write(indio_dev, OTP_CONFIG1, val);
		if (ret)
			break;
		usleep_range(10, 12);

		/* OTP reset HIGH */
		ret = icp_reg_write(indio_dev, OTP_DBG2, ICP20100_OTP_RESET);
		if (ret)
			break;
		usleep_range(10, 12);

		/* OTP reset LOW */
		ret = icp_reg_write(indio_dev, OTP_DBG2, 0);
		if (ret)
			break;
		usleep_range(10, 12);

		//program redundant read
		//as per datasheet for initial device config
		ret = icp_reg_write(indio_dev, OTP_MRA_LSB, 0x04);
		if (ret)
			break;

		ret = icp_reg_write(indio_dev, OTP_MRA_MSB, 0x04);
		if (ret)
			break;

		ret = icp_reg_write(indio_dev, OTP_MRB_LSB, 0x21);
		if (ret)
			break;

		ret = icp_reg_write(indio_dev, OTP_MRB_MSB, 0x20);
		if (ret)
			break;

		ret = icp_reg_write(indio_dev, OTP_MR_LSB, 0x10);
		if (ret)
			break;

		ret = icp_reg_write(indio_dev, OTP_MR_MSB, 0x80);
		if (ret)
			break;

		ret = icp20100_read_otp_data(indio_dev,
			OTP_OFFSET, 1, &st->params.offset);
		if (ret) {
			dev_err(st->dev, "read otp data 0x%02x fail, %d",
				OTP_OFFSET, ret);
			break;
		}

		ret = icp20100_read_otp_data(indio_dev,
			OTP_GAIN, 1, &st->params.gain);
		if (ret) {
			dev_err(st->dev, "read otp data 0x%02x fail, %d",
				OTP_GAIN, ret);
			break;
		}

		ret = icp20100_read_otp_data(indio_dev,
			OTP_HFOSC, 1, &st->params.HFosc);
		if (ret) {
			dev_err(st->dev, "read otp data 0x%02x fail, %d",
				OTP_HFOSC, ret);
			break;
		}

		//disable OTP and the write switch
		ret = icp_reg_write(indio_dev, OTP_CONFIG1, 0);
		if (ret)
			break;
		usleep_range(10, 12);

		ret = icp_reg_write(indio_dev, TRIM1_MSB,
			(st->params.offset & ICP20100_TRIM1_MSB_MASK));
		if (ret)
			break;

		ret = icp_reg_read(indio_dev, TRIM2_MSB, &val);
		if (ret)
			break;

		val &= ~(ICP20100_PEFE_GAIN_TRIM_MASK);
		/* update gain without touching BG_PTAT_TRIM */
		val |= (st->params.gain & 0x7) << 4;
		ret = icp_reg_write(indio_dev, TRIM2_MSB, val);
		if (ret)
			break;

		ret = icp_reg_write(indio_dev, TRIM2_LSB,
			(st->params.HFosc & ICP20100_TRIM2_LSB_MASK));
		if (ret)
			break;
	} while (0);

	return ret;
}

static int icp20100_init(struct iio_dev *indio_dev, bool device_config)
{
	int ret;
	struct icp20100_driver_info *st = iio_priv(indio_dev);
	u8 device_id;
	u32 i;
	u8 val;
	bool boot_sts;

	//sending dummy i2c transaction twice to initialize i2c interface
	for (i = 0; i < 4; i++) {
		ret = icp_reg_write(indio_dev, MASTER_LOCK, 0);
		if (!ret)
			break;
		msleep_interruptible(500);
	}

	if (ret) {
		dev_err(st->dev, "failed to write MASTER_LOCK, %d", ret);
		return ret;
	}

	ret = icp_reg_read(indio_dev, VERSION, &val);
	if (ret) {
		dev_err(st->dev, "failed to read VERSION, %d", ret);
		return ret;
	}
	// store revision value
	st->device_rev = val;

	// read boot status bit
	ret = icp20100_boot_sts(indio_dev, BOOT_STS_READ, &boot_sts);
	if (ret) {
		dev_err(st->dev, "failed to read BOOT STATUS, %d", ret);
		return ret;
	}

	/*
	 * REV b part will do it's init automatically
	 * if boot sts == 1, then no need to init again
	 */
	if ((val == ICP20100_REV_B) || (boot_sts == 1))
		return ret;

	do {

		ret = icp_reg_read(indio_dev, MODE_SELECT, &val);
		if (ret)
			break;

		val &= ~(1 << ICP20100_POWER_MODE);
		val |= 1 << ICP20100_POWER_MODE;
		//setting power mode to ACTIVE
		ret = icp_reg_write(indio_dev, MODE_SELECT, val);
		if (ret)
			break;
		usleep_range(4000, 4200);

		//unlock the main registers
		ret = icp_reg_write(indio_dev, MASTER_LOCK, 0x1F);
		if (ret)
			break;

		ret = icp20100_get_device_id(indio_dev, &device_id);
		if (ret)
			break;

		if (st->device_id != device_id) {
			dev_err(st->dev,
				"device ID read %02x does not match %02x",
				device_id, st->device_id);
			ret = -EINVAL;
			break;
		}

		dev_dbg(st->dev,
			"device id %02x", st->device_id);

		if (device_config) {
			ret = icp20100_config(indio_dev);
			if (ret) {
				dev_err(st->dev,
				"Fail to config. device for meas. %d", ret);
			}
		}

		ret = icp_reg_write(indio_dev, MASTER_LOCK, 0);
		if (ret)
			break;

		ret = icp20100_standby(indio_dev);
		if (ret)
			break;

		boot_sts = 1;
		ret = icp20100_boot_sts(indio_dev, BOOT_STS_WRITE, &boot_sts);
		if (ret)
			break;

	} while (0);

	if (ret)
		icp20100_regdump(indio_dev);

	return ret;
}

static int icp20100_forced_meas(struct iio_dev *indio_dev,
				enum iio_chan_type type,
				int *value)
{
	int ret;
	u8 dummy_reg;
	struct icp20100_driver_info *st = iio_priv(indio_dev);
	/* 3 bytes of pressure
	 * 3 bytes of temperature
	 */
	struct sample {
		u8 press[3];
		u8 temp[3];
	} __packed;
	s64 timeout;
	struct sample one_sample;
	u8 retry_count = 3;
	struct device *parent = indio_dev->dev.parent;

	do {
		ret = iio_device_claim_direct_mode(indio_dev);
		if (ret)
			break;

		if (!st->enable_force_meas) {
			dev_info(st->dev, "enable force meas first..");
			icp20100_enable_force_mode(indio_dev);
			st->enable_force_meas = true;
		}

		ret = icp20100_set_force_mode(indio_dev);
		if (ret) {
			dev_err(st->dev, "set force mode failed, %d", ret);
			break;
		}

		do {
			timeout = wait_for_completion_interruptible_timeout(
				&st->data_ready, ICP20100_CONVERSION_JIFFIES);
			if (timeout <= 0) {
				dev_err(st->dev, "force mode int. timeout");
				retry_count--;
				msleep_interruptible(500);
			}
		} while ((retry_count > 0) && (timeout <= 0));

		if (timeout > 0) {
			ret = regmap_bulk_read(st->regmap, PRESS_DATA_0,
					       (void *)&one_sample,
					       sizeof(struct sample));
			if (ret) {
				dev_err(st->dev, "Failed to bulk read");
				break;
			}

			/*
			 * Perform dummy read to 0x00 register as last
			 * i2c transaction. Recommended by Invensense.
			 */
			ret = icp_reg_read(indio_dev, 0x00, &dummy_reg);
			if (ret) {
				dev_err(st->dev, "dummy reg read fail, %d",
					ret);
				break;
			}
		} else {
			type = -1;
		}

		ret = icp20100_standby(indio_dev);
		if (ret)
			break;

		iio_device_release_direct_mode(indio_dev);

		switch (type) {
		case IIO_PRESSURE:
			dev_dbg(st->dev, "fetching raw pressure values");
			*value = (one_sample.press[2] << 16) |
				 (one_sample.press[1] << 8) |
				 (one_sample.press[0]);
			/* pressure is a 20 bit 2's complement value,
			 * preserve negative sign.
			 */
			if (*value & 1 << 19)
				*value -= 1 << 20;
			ret = IIO_VAL_INT;
			/* Min and Max pressure values in Pout
			 * as per the Pout to Kpa pressure formula in
			 * icp20100 datasheet
			 */
			if (*value < -131072 || *value > 131071)
				ret = -ERANGE;
			break;

		case IIO_TEMP:
			dev_dbg(st->dev, "fetching raw temperature values");
			*value = (one_sample.temp[2] << 16) |
				 (one_sample.temp[1] << 8) |
				 (one_sample.temp[0]);
			/* temp is a 20 bit 2's complement value,
			 * preserve negative sign.
			 */
			if (*value & 1 << 19)
				*value -= 1 << 20;
			ret = IIO_VAL_INT;
			/* Min and Max temp. value in Tout
			 * as per the Tout to Celcius temp. formula in
			 * icp20100 datasheet
			 */
			if (*value < -262144 || *value > 262143)
				ret = -ERANGE;
			break;

		default:
			ret = -EINVAL;
		}

	} while (0);

	if (ret == -ERANGE) {
		dev_dbg(st->dev, "raw val. out of range. resetting dev.");
		icp20100_reset_device(parent);
	}

	if (ret)
		icp20100_regdump(indio_dev);

	return ret;
}

static int icp20100_read_raw(struct iio_dev *indio_dev,
			     struct iio_chan_spec const *chan,
			     int *val, int *val2, long mask)
{
	struct icp20100_driver_info *st = iio_priv(indio_dev);

	switch (mask) {
	case IIO_CHAN_INFO_RAW:
	case IIO_CHAN_INFO_PROCESSED:
		return icp20100_forced_meas(indio_dev, chan->type, val);
	case IIO_CHAN_INFO_SCALE:
		switch (chan->type) {
		case IIO_TEMP:
			/*
			 * IIO ABI expects temperature to be reported as
			 *  Temp[milli degC] = (Traw + Offset) * Scale
			 *  Formula as per datasheet:
			 *      Temp [degC] = (Traw/2^18)*65C + 25C
			 *  IIO ABI conversion formula
			 *      Temp [milli degC] =
			 *             (Traw + (25C*2^18)/65) * (65/2^18) * 1000
			 *  For ML purpose, showing temperature in Celcius
			 */
			*val = 65;
			*val2 = 262144;
			return IIO_VAL_FRACTIONAL;

		case IIO_PRESSURE:
			/*
			 * IIO ABI expects pressure to be reported as
			 *  P[KPa] = (Praw + Offset) * Scale
			 *  Formula as per datasheet:
			 *      P[KPa] = (Praw/2^17)*40Kpa + 70Kpa
			 *  IIO ABI conversion formula
			 *      P[Kpa] = (Praw + (7*2^17)/4) * (40/2^17)
			 *  For ML purpose, showing pressure in pascals
			 */
			*val = 40000;
			*val2 = 131072;
			return IIO_VAL_FRACTIONAL;

		default:
			return -EINVAL;
		}

	case IIO_CHAN_INFO_OFFSET:
		switch (chan->type) {
		case IIO_TEMP:
			*val = 6553600;
			*val2 = 65;
			return IIO_VAL_FRACTIONAL;

		case IIO_PRESSURE:
			*val = 917504;
			*val2 = 4;
			return IIO_VAL_FRACTIONAL;

		default:
			return -EINVAL;
		}

	case IIO_CHAN_INFO_OVERSAMPLING_RATIO:
		switch (chan->type) {
		case IIO_TEMP:
			*val = st->mode4.temp_osr;
			return IIO_VAL_INT;

		case IIO_PRESSURE:
			*val = st->mode4.pres_osr;
			return IIO_VAL_INT;

		default:
			return -EINVAL;
		}

	default:
		return -EINVAL;
	}
}

static int icp20100_write_raw(struct iio_dev *indio_dev,
			      const struct iio_chan_spec *chan,
			      int val,
			      int val2,
			      long mask)
{
	struct icp20100_driver_info *st = iio_priv(indio_dev);
	int ret = -EINVAL;

	iio_device_claim_direct_mode(indio_dev);

	switch (mask) {
	case IIO_CHAN_INFO_OVERSAMPLING_RATIO:
		switch (chan->type) {
		case IIO_PRESSURE:
			st->mode4.pres_osr = val;
			break;

		case IIO_TEMP:
			st->mode4.temp_osr = val;
			break;

		default:
			ret = -EINVAL;
			break;
		}
		break;

	default:
		break;
	}

	iio_device_release_direct_mode(indio_dev);
	return ret;
}

static const struct iio_chan_spec icp20100_channels[] = {
	[0] = {
		.type               = IIO_PRESSURE,
		.scan_index         = 0,
		.scan_type          = {
			.sign            = 's',
			.realbits        = 20,
			.storagebits     = 24,
			.endianness      = IIO_LE,
		},
		.info_mask_separate = BIT(IIO_CHAN_INFO_RAW) |
				      BIT(IIO_CHAN_INFO_SCALE) |
				      BIT(IIO_CHAN_INFO_OFFSET) |
				      BIT(IIO_CHAN_INFO_OVERSAMPLING_RATIO),
	},
	[1] = {
		.type               = IIO_TEMP,
		.scan_index         = 1,
		.scan_type          = {
			.sign             = 's',
			.realbits         = 20,
			.storagebits      = 24,
			.endianness       = IIO_LE,
		},
		.info_mask_separate = BIT(IIO_CHAN_INFO_RAW) |
				      BIT(IIO_CHAN_INFO_SCALE) |
				      BIT(IIO_CHAN_INFO_OFFSET) |
				      BIT(IIO_CHAN_INFO_OVERSAMPLING_RATIO),
	},
	[2] = IIO_CHAN_SOFT_TIMESTAMP(2),
};

static ssize_t icp20100_set_meas_mode(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct icp20100_driver_info *st = iio_priv(dev_to_iio_dev(dev));
	u32 mode;

	if (kstrtou32(buf, 10, &mode))
		return -EINVAL;

	if (mode < ICP20100_MODE_0 || mode > ICP20100_MODE_4)
		return -EINVAL;

	if (st->mode == mode)
		dev_warn(st->dev,
			 "Mode already at %s", (char *)mode_names[mode]);
	else {
		st->mode = mode;
		dev_info(st->dev,
			 "Mode changed to %s", (char *)mode_names[mode]);
	}

	return count;
}

#define MAX_ODR_VALUE     (8191)

static ssize_t icp20100_set_mode4_odr(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct iio_dev *indio_dev = dev_to_iio_dev(dev);
	struct icp20100_driver_info *st = iio_priv(indio_dev);
	u32 odr;

	if (kstrtou32(buf, 10, &odr))
		return -EINVAL;

	if (odr <= MAX_ODR_VALUE)
		st->mode4.odr = odr;
	else
		return -EINVAL;

	return count;
}

static ssize_t icp20100_show_device_id(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct icp20100_driver_info *st = iio_priv(dev_to_iio_dev(dev));

	return sprintf(buf, "0x%02x\n", st->device_id);
}

static ssize_t icp20100_show_device_rev(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct icp20100_driver_info *st = iio_priv(dev_to_iio_dev(dev));

	char revision = 'A';

	if (st->device_rev == ICP20100_REV_B)
		revision = 'B';

	return sprintf(buf, "rev %c, val: 0x%02x\n", revision, st->device_rev);
}

static ssize_t icp20100_get_meas_mode(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct icp20100_driver_info *st = iio_priv(dev_to_iio_dev(dev));

	return sprintf(buf, "%d\n", st->mode);
}

static ssize_t icp20100_get_mode4_odr(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct icp20100_driver_info *st = iio_priv(dev_to_iio_dev(dev));

	return sprintf(buf, "%d\n", st->mode4.odr);
}

static IIO_DEVICE_ATTR(get_meas_mode, 0444, icp20100_get_meas_mode, NULL, 0);
static IIO_DEVICE_ATTR(device_id, 0444, icp20100_show_device_id, NULL, 0);
static IIO_DEVICE_ATTR(device_rev, 0444, icp20100_show_device_rev, NULL, 0);
static IIO_DEVICE_ATTR(get_mode4_odr, 0444, icp20100_get_mode4_odr, NULL, 0);
static IIO_DEVICE_ATTR(set_meas_mode, 0644, NULL, icp20100_set_meas_mode, 0);
static IIO_DEVICE_ATTR(set_mode4_odr, 0644, NULL, icp20100_set_mode4_odr, 0);

#define ICP20100_DEV_ATTR(name) (&iio_dev_attr_##name.dev_attr.attr)

static struct attribute *icp20100_attributes[] = {
	ICP20100_DEV_ATTR(get_meas_mode),
	ICP20100_DEV_ATTR(set_meas_mode),
	ICP20100_DEV_ATTR(set_mode4_odr),
	ICP20100_DEV_ATTR(get_mode4_odr),
	ICP20100_DEV_ATTR(device_id),
	ICP20100_DEV_ATTR(device_rev),
	NULL,
};

static const struct attribute_group icp20100_attribute_group = {
	.attrs = icp20100_attributes,
};

static const struct iio_info icp20100_info = {
	.read_raw = icp20100_read_raw,
	.write_raw = icp20100_write_raw,
	.attrs = &icp20100_attribute_group,
};

#define DEFAULT_PRESS_OSR_VALUE       (255)
#define DEFAULT_TEMP_OSR_VALUE         (31)

int icp20100_probe(struct device *parent,
		   const char *name,
		   int irq,
		   u32 hwid,
		   struct regmap *regmap)
{
	int ret;
	struct iio_dev *indio_dev;
	struct icp20100_driver_info *st;

	indio_dev = devm_iio_device_alloc(parent,
					  sizeof(struct icp20100_driver_info));
	if (!indio_dev)
		return -ENOMEM;

	st = iio_priv(indio_dev);
	st->dev = parent;
	indio_dev->dev.parent = parent;
	indio_dev->name = name;
	indio_dev->modes = INDIO_DIRECT_MODE;
	indio_dev->channels = icp20100_channels;
	indio_dev->num_channels = ARRAY_SIZE(icp20100_channels);
	indio_dev->info = &icp20100_info;

	st->regmap = regmap;
	st->device_id = hwid;
	/* set MODE4 as default mode */
	st->mode = ICP20100_MODE_4;
	st->mode4.pres_osr = DEFAULT_PRESS_OSR_VALUE;
	st->mode4.temp_osr = DEFAULT_TEMP_OSR_VALUE;

	dev_set_drvdata(parent, indio_dev);

	ret = icp20100_init(indio_dev, true);
	if (ret) {
		dev_err(parent, "Failed to init icp20100, %d", ret);
		return ret;
	}

	ret = devm_iio_triggered_buffer_setup(parent, indio_dev, NULL,
					      icp20100_trigger_handler,
					      &icp20100_buffer_setup_ops);
	if (ret) {
		dev_err(parent, "Failed to setup triggered buffer, %d", ret);
		return ret;
	}

	ret = icp20100_init_trigger_setup(parent, indio_dev, st, irq);
	if (ret) {
		dev_err(parent, "Failed to init trigger setup, %d", ret);
		return ret;
	}

	ret = icp20100_init_interrupt_handler(parent, indio_dev, st, irq);
	if (ret) {
		dev_err(parent, "Failed to init interrupt handler, %d", ret);
		return ret;
	}

	ret = devm_iio_device_register(parent, indio_dev);
	if (ret) {
		dev_err(parent, "Failed to register device icp20100, %d", ret);
		return ret;
	}

	dev_info(parent, "ICP20100 successfully probed!");

	return 0;
}
EXPORT_SYMBOL_GPL(icp20100_probe);

static void icp20100_device_shutdown(struct device *parent)
{
	struct iio_dev *indio_dev = dev_get_drvdata(parent);
	struct icp20100_driver_info *st = iio_priv(indio_dev);

	icp20100_standby(indio_dev);
	msleep_interruptible(50);
	disable_irq(st->irq);
	icp20100_clear_fifo(indio_dev);
	icp20100_clear_int_status(indio_dev);
}

static void icp20100_reset_device(struct device *parent)
{
	struct iio_dev *indio_dev = dev_get_drvdata(parent);
	struct icp20100_driver_info *st = iio_priv(indio_dev);
	int ret;

	icp20100_device_shutdown(parent);
	msleep_interruptible(50);
	ret = icp20100_init(indio_dev, false);
	if (ret)
		dev_err(parent, "Failed to re-init icp20100, %d", ret);

	enable_irq(st->irq);
}

void icp20100_remove(struct device *parent)
{
	icp20100_device_shutdown(parent);
}
EXPORT_SYMBOL_GPL(icp20100_remove);

void icp20100_shutdown(struct device *parent)
{
	icp20100_device_shutdown(parent);
}
EXPORT_SYMBOL_GPL(icp20100_shutdown);

MODULE_AUTHOR("Siddarth Kare <skare@magicleap.com>");
MODULE_DESCRIPTION("Invensense ICP201xx I2C Altimeter driver");
MODULE_LICENSE("GPL v2");
