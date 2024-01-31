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
#include <linux/module.h>
#include <linux/byteorder/generic.h>
#include <linux/platform_device.h>
#include <uapi/linux/mlt_grip.h>
#include "mlt.h"

#define GRIP_IDEV_NAME_SZ		(64)
#define GRIP_TOUCH_ABS_X_MAX		(0xfff)
#define GRIP_TOUCH_ABS_Y_MAX		(0xfff)
#define GRIP_TOUCH_PRESSURE_MAX		(0xff)
#define GRIP_ANALOG_TRIGGER_MIN_TSHLD	(0x14)
#define GRIP_ANALOG_TRIGGER_MAX		(0xff)
#define GRIP_ANALOG_TRIGGER_PULLED(x)	((x) >= GRIP_ANALOG_TRIGGER_MIN_TSHLD)

struct grip_button_map {
	u8 button;
	u16 key_code;
};

static const struct grip_button_map s_grip_buttons[] = {
	{ 0x01, BTN_1 },
	{ 0x02, BTN_2 },
};

#define GRIP_NUM_OF_BTNS	ARRAY_SIZE(s_grip_buttons)
#define GRIP_BTNS_SUPPORTED	(0x01 | 0x02)  /**< BTN_1 and BTN_2 */
#define GRIP_MAX_BATCHING	5

struct grip_data {
	struct device *dev;
	struct input_dev *idev_imu;
	struct input_dev *idev_ctrl;
	char idev_imu_name[GRIP_IDEV_NAME_SZ];
	char idev_ctrl_name[GRIP_IDEV_NAME_SZ];

	/* Current state */
	u32 tstamp;
	u8 btn_scan;
	u8 touch_state;
	u8 gesture;
	u8 trigger;
} __packed;

static void grip_send_touch_evt(struct input_dev *idev,
				const struct grip_ctrl_evt *evt,
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

static void grip_send_button_evt(struct input_dev *idev,
				 const struct grip_ctrl_evt *evt,
				 u8 *prev_state)
{
	u8 new_state = evt->buttons;
	u8 changed = (new_state ^ *prev_state) & GRIP_BTNS_SUPPORTED;
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
		input_report_key(idev, s_grip_buttons[bit_pos].key_code,
				 pressed);
		changed &= ~(1 << bit_pos);
	}
	*prev_state = new_state;
}

static void grip_send_trigger_evt(struct input_dev *idev,
				  const struct grip_ctrl_evt *evt,
				  u8 *prev_state)
{
	if (GRIP_ANALOG_TRIGGER_PULLED(evt->trigger))
		input_report_abs(idev, ABS_ANALOG_TRIGGER, evt->trigger);
	else if (GRIP_ANALOG_TRIGGER_PULLED(*prev_state))
		input_report_abs(idev, ABS_ANALOG_TRIGGER, 0);
	*prev_state = evt->trigger;
}

static int grip_parse_ctrl_pkt(const struct grip_ctrl_evt *evt,
			       struct grip_data *grip)
{
	struct input_dev *idev = grip->idev_ctrl;

	print_hex_dump_debug("ctrl: ", DUMP_PREFIX_NONE, 32, 1, evt,
			     sizeof(struct grip_ctrl_evt), 0);
	grip_send_touch_evt(idev, evt, &grip->touch_state, &grip->gesture);
	grip_send_button_evt(idev, evt, &grip->btn_scan);
	grip_send_trigger_evt(idev, evt, &grip->trigger);
	input_sync(idev);
	return 0;
}

static int grip_parse_imu_pkt(const struct grip_imu_evt *evt,
			      struct grip_data *grip)
{
	struct input_dev *idev = grip->idev_imu;

	print_hex_dump_debug("imu: ", DUMP_PREFIX_NONE, 32, 1, evt,
			     sizeof(struct grip_imu_evt), 0);
	if (grip->tstamp == evt->tstamp)
		return 0;

	grip->tstamp = evt->tstamp;
	input_report_abs(idev, ABS_ACCEL_X, le32_to_cpu(evt->accel[0]));
	input_report_abs(idev, ABS_ACCEL_Y, le32_to_cpu(evt->accel[1]));
	input_report_abs(idev, ABS_ACCEL_Z, le32_to_cpu(evt->accel[2]));

	input_report_abs(idev, ABS_GYRO_X, le32_to_cpu(evt->gyro[0]));
	input_report_abs(idev, ABS_GYRO_Y, le32_to_cpu(evt->gyro[1]));
	input_report_abs(idev, ABS_GYRO_Z, le32_to_cpu(evt->gyro[2]));

	input_report_abs(idev, ABS_MGNT_X, le32_to_cpu(evt->mgnt[0]));
	input_report_abs(idev, ABS_MGNT_Y, le32_to_cpu(evt->mgnt[1]));
	input_report_abs(idev, ABS_MGNT_Z, le32_to_cpu(evt->mgnt[2]));

	input_event(idev, EV_MSC, MSC_TIMESTAMP, le32_to_cpu(evt->tstamp));
	input_sync(idev);
	return 0;
}

static int grip_input_event(struct mlt_device *mlt_dev)
{
	const struct grip_input_evt *input_event = mlt_dev->shmem;
	struct grip_data *grip = mlt_get_drvdata(mlt_dev);
	int rc = -EINVAL;

	if (input_event->type == GRIP_INPUT_IMU)
		rc = grip_parse_imu_pkt(&input_event->u.imu, grip);
	else if (input_event->type == GRIP_INPUT_CTRL)
		rc = grip_parse_ctrl_pkt(&input_event->u.ctrl, grip);
	return rc;
}

static int grip_input_dev_register(struct mlt_device *mlt_dev,
				   struct grip_data *grip)
{
	int rc;
	int i;

	/* Register input device for IMU events */
	grip->idev_imu = devm_input_allocate_device(&mlt_dev->dev);
	if (!grip->idev_imu)
		return -ENOMEM;

	snprintf(grip->idev_imu_name, GRIP_IDEV_NAME_SZ, "imu-%s",
		 dev_name(&mlt_dev->dev));
	grip->idev_imu->name = grip->idev_imu_name;
	grip->idev_imu->id.bustype = mlt_dev->bus;
	grip->idev_imu->id.vendor= mlt_dev->vendor;
	grip->idev_imu->id.product = mlt_dev->product;
	grip->idev_imu->id.version = mlt_dev->version;
	grip->idev_imu->dev.parent = grip->dev;

	__set_bit(EV_ABS, grip->idev_imu->evbit);
	__set_bit(EV_MSC, grip->idev_imu->evbit);
	__set_bit(MSC_TIMESTAMP, grip->idev_imu->mscbit);

	input_set_abs_params(grip->idev_imu, ABS_ACCEL_X, 0, U16_MAX, 0, 0);
	input_set_abs_params(grip->idev_imu, ABS_ACCEL_Y, 0, U16_MAX, 0, 0);
	input_set_abs_params(grip->idev_imu, ABS_ACCEL_Z, 0, U16_MAX, 0, 0);

	input_set_abs_params(grip->idev_imu, ABS_GYRO_X, 0, U16_MAX, 0, 0);
	input_set_abs_params(grip->idev_imu, ABS_GYRO_Y, 0, U16_MAX, 0, 0);
	input_set_abs_params(grip->idev_imu, ABS_GYRO_Z, 0, U16_MAX, 0, 0);

	input_set_abs_params(grip->idev_imu, ABS_MGNT_X, 0, U16_MAX, 0, 0);
	input_set_abs_params(grip->idev_imu, ABS_MGNT_Y, 0, U16_MAX, 0, 0);
	input_set_abs_params(grip->idev_imu, ABS_MGNT_Z, 0, U16_MAX, 0, 0);

	rc = input_register_device(grip->idev_imu);
	if (rc) {
		dev_err(grip->dev, "input_register_device(%s) failed: %d\n",
			grip->idev_imu->name, rc);
		return rc;
	}

	/* Register input device for touch, button and trigger events */
	grip->idev_ctrl = devm_input_allocate_device(&mlt_dev->dev);
	if (!grip->idev_ctrl)
		return -ENOMEM;

	snprintf(grip->idev_ctrl_name, GRIP_IDEV_NAME_SZ, "ctrl-%s",
		 dev_name(&mlt_dev->dev));
	grip->idev_ctrl->name = grip->idev_ctrl_name;

	grip->idev_ctrl->id.bustype = mlt_dev->bus;
	grip->idev_ctrl->id.vendor= mlt_dev->vendor;
	grip->idev_ctrl->id.product = mlt_dev->product;
	grip->idev_ctrl->id.version = mlt_dev->version;
	grip->idev_ctrl->dev.parent = grip->dev;

	__set_bit(BTN_TOUCH, grip->idev_ctrl->keybit);
	__set_bit(EV_KEY, grip->idev_ctrl->evbit);
	__set_bit(EV_ABS, grip->idev_ctrl->evbit);
	input_set_abs_params(grip->idev_ctrl, ABS_MT_POSITION_X,
			     0, GRIP_TOUCH_ABS_X_MAX, 0, 0);
	input_set_abs_params(grip->idev_ctrl, ABS_MT_POSITION_Y,
			     0, GRIP_TOUCH_ABS_Y_MAX, 0, 0);
	input_set_abs_params(grip->idev_ctrl, ABS_MT_PRESSURE, 0,
			     GRIP_TOUCH_PRESSURE_MAX, 0, 0);
	input_set_capability(grip->idev_ctrl, EV_MSC, MSC_ACTIVITY);
	input_set_capability(grip->idev_ctrl, EV_MSC, MSC_GESTURE);

	__set_bit(EV_KEY, grip->idev_ctrl->evbit);
	for (i = 0; i < GRIP_NUM_OF_BTNS; i++) {
		if (s_grip_buttons[i].key_code != KEY_RESERVED)
			__set_bit(s_grip_buttons[i].key_code,
				  grip->idev_ctrl->keybit);
	}

	__set_bit(EV_ABS, grip->idev_ctrl->evbit);
	input_set_abs_params(grip->idev_ctrl, ABS_ANALOG_TRIGGER, 0,
			     GRIP_ANALOG_TRIGGER_MAX, 0, 0);

	rc = input_register_device(grip->idev_ctrl);
	if (rc)
		dev_err(grip->dev, "input_register_device(%s) failed: %d\n",
			grip->idev_ctrl->name, rc);

	return rc;
}

static int grip_probe(struct mlt_device *mlt_dev)
{
	struct device *dev = &mlt_dev->dev;
	struct grip_data *grip;
	int rc;

	dev_info(dev, "%s\n", __func__);
	grip = devm_kzalloc(dev, sizeof(struct grip_data), GFP_KERNEL);
	if (!grip)
		return -ENOMEM;

	grip->dev = dev;
	mlt_set_drvdata(mlt_dev, grip);
	rc = grip_input_dev_register(mlt_dev, grip);
	if (rc)
		dev_err(dev, "%s failed, %d\n", __func__, rc);

	return rc;
}

static int grip_remove(struct mlt_device *mlt_dev)
{
	dev_info(&mlt_dev->dev, "%s\n", __func__);
	return 0;
}

static const struct mlt_device_id grip_devices[] = {
	{ MLT_DEVICE_ID(MAGICLEAP_VENDOR_ID, GRIP_ID, BUS_BLUETOOTH) },
	{ }
};
MODULE_DEVICE_TABLE(mlt, grip_devices);

static struct mlt_driver grip_driver = {
	.name = "grip",
	.id_table = grip_devices,
	.probe = grip_probe,
	.remove = grip_remove,
	.input_event = grip_input_event,
};
module_mlt_driver(grip_driver);

MODULE_AUTHOR("Magic Leap, Inc.");
MODULE_DESCRIPTION("Magic Leap Grip Control Driver");
MODULE_LICENSE("GPL v2");
