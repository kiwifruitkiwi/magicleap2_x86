// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2020, Magic Leap, Inc. All rights reserved.
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

#include <linux/input.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/byteorder/generic.h>
#include <linux/platform_device.h>
#include <uapi/linux/mlt_roshi.h>
#include "mlt.h"

#define ROSHI_IDEV_NAME_SZ		(64)
#define ROSHI_TOUCH_MXT640T_ABS_X_MAX	(0xfff)
#define ROSHI_TOUCH_MXT640T_ABS_Y_MAX	(0xfff)
#define ROSHI_TOUCH_CIRQUE_ABS_X_MAX	(0x3ff)
#define ROSHI_TOUCH_CIRQUE_ABS_Y_MAX	(0x3ff)
#define ROSHI_TOUCH_PRESSURE_MAX	(0xff)
#define ROSHI_ANALOG_TRIGGER_MIN_TSHLD	(0x14)
#define ROSHI_ANALOG_TRIGGER_MAX	(0xff)
#define ROSHI_ANALOG_TRIGGER_PULLED(x)	((x) >= ROSHI_ANALOG_TRIGGER_MIN_TSHLD)

#define ROSHI_HW_REV_PEQ1A 3

struct roshi_button_map {
	u8 button;
	u16 key_code;
};

static const struct roshi_button_map s_roshi_buttons[] = {
	{ 0x01, BTN_1 },
	{ 0x02, BTN_2 },
	{ 0x04, BTN_3 },
	{ 0x08, BTN_4 },
};

#define ROSHI_NUM_OF_BTNS	ARRAY_SIZE(s_roshi_buttons)
#define ROSHI_BTNS_SUPPORTED	(0x01 | 0x02 | 0x04 | 0x08)  /**< BTN_1-4 */

struct roshi_data {
	struct device *dev;
	struct input_dev *idev;

	/* Current state */
	u8 btn_scan;
	u8 touch_state;
	u8 gesture;
	u8 trigger;
};

static void roshi_send_touch_evt(struct input_dev *idev,
				 const struct roshi_input_evt *evt,
				 u8 *prev_touch_state,
				 u8 *prev_gesture)
{
	if (evt->touch_x || evt->touch_y) {
		input_report_abs(idev, ABS_MT_POSITION_X,
				 le32_to_cpu(evt->touch_x));
		input_report_abs(idev, ABS_MT_POSITION_Y,
				 le32_to_cpu(evt->touch_y));
		input_report_abs(idev, ABS_MT_PRESSURE, evt->touch_force);
		input_mt_sync(idev);
	}

	if ((evt->gesture != *prev_gesture) && evt->gesture)
		input_event(idev, EV_MSC, MSC_GESTURE, evt->gesture);

	if (evt->touch_state != *prev_touch_state)
		input_event(idev, EV_MSC, MSC_ACTIVITY, evt->touch_state);

	*prev_touch_state = evt->touch_state;
	*prev_gesture = evt->gesture;
}

static void roshi_send_button_evt(struct input_dev *idev,
				  const struct roshi_input_evt *evt,
				  u8 *prev_state)
{
	u8 new_state = evt->buttons;
	u8 changed = (new_state ^ *prev_state) & ROSHI_BTNS_SUPPORTED;
	bool pressed;
	int  bit_pos;

	/*
	 * While not all the buttons that have changed
	 * their state were proccessed do the following:
	 * - Find the first button that has changed its state.
	 * - Check if it's pressed or not.
	 * - Report key envent.
	 * - Mask the bit associated with this button.
	 */
	while (changed) {
		bit_pos = __ffs(changed);
		pressed = new_state & BIT(bit_pos);
		input_report_key(idev, s_roshi_buttons[bit_pos].key_code,
				 pressed);
		changed &= ~(1 << bit_pos);
	}
	*prev_state = new_state;
}

static void roshi_send_trigger_evt(struct input_dev *idev,
				   const struct roshi_input_evt *evt,
				   u8 *prev_state)
{
	if (ROSHI_ANALOG_TRIGGER_PULLED(evt->trigger))
		input_report_abs(idev, ABS_ANALOG_TRIGGER, evt->trigger);
	else if (ROSHI_ANALOG_TRIGGER_PULLED(*prev_state))
		input_report_abs(idev, ABS_ANALOG_TRIGGER, 0);
	*prev_state = evt->trigger;
}

static int roshi_input_event(struct mlt_device *mlt_dev)
{
	const struct roshi_input_evt *evt = mlt_dev->shmem;
	struct roshi_data *roshi = mlt_get_drvdata(mlt_dev);

	print_hex_dump_debug("roshi: ", DUMP_PREFIX_NONE, 32, 1, evt,
			     sizeof(struct roshi_input_evt), 0);
	roshi_send_touch_evt(roshi->idev, evt, &roshi->touch_state,
			     &roshi->gesture);
	roshi_send_button_evt(roshi->idev, evt, &roshi->btn_scan);
	roshi_send_trigger_evt(roshi->idev, evt, &roshi->trigger);
	input_sync(roshi->idev);
	return 0;
}

static int roshi_input_dev_register(struct mlt_device *mlt_dev,
				    struct roshi_data *roshi)
{
	int rc;
	int i;

	roshi->idev = devm_input_allocate_device(&mlt_dev->dev);
	if (!roshi->idev)
		return -ENOMEM;

	roshi->idev->name = dev_name(&mlt_dev->dev);
	roshi->idev->id.bustype = mlt_dev->bus;
	roshi->idev->id.vendor = mlt_dev->vendor;
	roshi->idev->id.product = mlt_dev->product;
	roshi->idev->id.version = mlt_dev->version;
	roshi->idev->dev.parent = roshi->dev;

	__set_bit(BTN_TOUCH, roshi->idev->keybit);
	__set_bit(EV_KEY, roshi->idev->evbit);
	__set_bit(EV_ABS, roshi->idev->evbit);
	/* PEQ1A and newer revs use a Cirque touch and AF3X68 force sensor */
	if (mlt_dev->version < ROSHI_HW_REV_PEQ1A) {
		input_set_abs_params(roshi->idev, ABS_MT_POSITION_X, 0,
				     ROSHI_TOUCH_MXT640T_ABS_X_MAX, 0, 0);
		input_set_abs_params(roshi->idev, ABS_MT_POSITION_Y, 0,
				     ROSHI_TOUCH_MXT640T_ABS_Y_MAX, 0, 0);
	} else {
		input_set_abs_params(roshi->idev, ABS_MT_POSITION_X, 0,
				     ROSHI_TOUCH_CIRQUE_ABS_X_MAX, 0, 0);
		input_set_abs_params(roshi->idev, ABS_MT_POSITION_Y, 0,
				     ROSHI_TOUCH_CIRQUE_ABS_Y_MAX, 0, 0);
	}
	input_set_abs_params(roshi->idev, ABS_MT_PRESSURE, 0,
			     ROSHI_TOUCH_PRESSURE_MAX, 0, 0);
	input_set_capability(roshi->idev, EV_MSC, MSC_ACTIVITY);
	input_set_capability(roshi->idev, EV_MSC, MSC_GESTURE);

	__set_bit(EV_KEY, roshi->idev->evbit);
	for (i = 0; i < ROSHI_NUM_OF_BTNS; i++) {
		if (s_roshi_buttons[i].key_code != KEY_RESERVED)
			__set_bit(s_roshi_buttons[i].key_code,
				  roshi->idev->keybit);
	}

	__set_bit(EV_ABS, roshi->idev->evbit);
	input_set_abs_params(roshi->idev, ABS_ANALOG_TRIGGER, 0,
			     ROSHI_ANALOG_TRIGGER_MAX, 0, 0);

	rc = input_register_device(roshi->idev);
	if (rc)
		dev_err(roshi->dev, "input_register_device(%s) failed: %d\n",
			roshi->idev->name, rc);

	return rc;
}

static int roshi_probe(struct mlt_device *mlt_dev)
{
	struct device *dev = &mlt_dev->dev;
	struct roshi_data *roshi;
	int rc;

	dev_info(dev, "%s: hw rev %04x\n", __func__, mlt_dev->version);
	roshi = devm_kzalloc(dev, sizeof(struct roshi_data), GFP_KERNEL);
	if (!roshi)
		return -ENOMEM;

	roshi->dev = dev;
	mlt_set_drvdata(mlt_dev, roshi);
	rc = roshi_input_dev_register(mlt_dev, roshi);
	if (rc)
		dev_err(dev, "%s failed, %d\n", __func__, rc);

	return rc;
}

static int roshi_remove(struct mlt_device *mlt_dev)
{
	dev_info(&mlt_dev->dev, "%s\n", __func__);
	return 0;
}

static const struct mlt_device_id roshi_devices[] = {
	{ MLT_DEVICE_ID(MAGICLEAP_VENDOR_ID, ROSHI_ID, BUS_BLUETOOTH) },
	{ }
};
MODULE_DEVICE_TABLE(mlt, roshi_devices);

static struct mlt_driver roshi_driver = {
	.name = "roshi",
	.id_table = roshi_devices,
	.probe = roshi_probe,
	.remove = roshi_remove,
	.input_event = roshi_input_event,
};
module_mlt_driver(roshi_driver);

MODULE_AUTHOR("Magic Leap, Inc.");
MODULE_DESCRIPTION("Magic Leap Roshi Control Driver");
MODULE_LICENSE("GPL v2");
