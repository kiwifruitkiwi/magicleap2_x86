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

#include <linux/acpi.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/gpio.h>
#include <linux/gpio/consumer.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/rfkill.h>
#include <linux/uaccess.h>

#define BT_CMD_PWR_CTRL			0xbfad
#define BT_CMD_GETVAL_RESET_GPIO	0xbfaf

struct btqca_data {
	struct device		*dev;
	struct miscdevice	mdev;
	struct rfkill		*rfkill_dev;
	struct gpio_desc	*gpio_reset;
	bool			is_enabled;
};

static int btqca_gpio_get_value(struct gpio_desc *gpio)
{
	if (gpiod_cansleep(gpio))
		return gpiod_get_value_cansleep(gpio);
	else
		return gpiod_get_value(gpio);
}

static void btqca_gpio_set_value(struct gpio_desc *gpio, int val)
{
	if (gpiod_cansleep(gpio))
		gpiod_set_value_cansleep(gpio, val);
	else
		gpiod_set_value(gpio, val);
}

static void btqca_set_power(struct btqca_data *btqca, bool on)
{
	if (btqca->is_enabled != on) {
		dev_info(btqca->dev, "set BT_EN to %d\n", on);
		btqca_gpio_set_value(btqca->gpio_reset, on);
		btqca->is_enabled = on;
		if (on)
			msleep(50);
	}
}

static int btqca_rfkill_set_power(void *data, bool blocked)
{
	struct btqca_data *btqca = data;

	dev_dbg(btqca->dev, "%s: %s\n", __func__, blocked ? "off" : "on");
	btqca_set_power(btqca, !blocked);
	return 0;
}

static const struct rfkill_ops btqca_rfkill_ops = {
	.set_block = btqca_rfkill_set_power,
};

static long btqca_pwr_ioctl(struct file *filep, unsigned int cmd,
			    unsigned long arg)
{
	struct btqca_data *btqca = container_of(filep->private_data,
						struct btqca_data, mdev);
	struct device *dev = btqca->dev;
	bool pwr_on;
	int rc = 0;

	switch (cmd) {
	case BT_CMD_PWR_CTRL:
		pwr_on = (bool)arg;
		dev_dbg(dev, "BT_CMD_PWR_CTRL: %d\n", pwr_on);
		btqca_set_power(btqca, pwr_on);
		rfkill_set_sw_state(btqca->rfkill_dev, !pwr_on);
		break;
	case BT_CMD_GETVAL_RESET_GPIO:
		rc = btqca_gpio_get_value(btqca->gpio_reset);
		dev_dbg(dev, "GET_RESET_GPIO: %d\n", rc);
		break;
	default:
		rc = -EINVAL;
		break;
	}
	return rc;
}

static const struct file_operations btqca_dev_fops = {
	.unlocked_ioctl = btqca_pwr_ioctl,
	.compat_ioctl = btqca_pwr_ioctl,
};

static int btqca_pwr_probe(struct platform_device *pdev)
{
	struct btqca_data *btqca;
	struct device *dev = &pdev->dev;
	int rc;

	dev_info(dev, "%s\n", __func__);
	btqca = devm_kzalloc(dev, sizeof(*btqca), GFP_KERNEL);
	if (!btqca)
		return -ENOMEM;

	btqca->dev = dev;
	platform_set_drvdata(pdev, btqca);

	btqca->gpio_reset = devm_gpiod_get(dev, "reset", GPIOD_OUT_LOW);
	if (IS_ERR(btqca->gpio_reset)) {
		dev_err(dev, "Failed to obtain bt_reset gpio\n");
		return PTR_ERR(btqca->gpio_reset);
	}

	btqca->mdev.minor = MISC_DYNAMIC_MINOR;
	btqca->mdev.parent = &pdev->dev;
	btqca->mdev.fops = &btqca_dev_fops;
	btqca->mdev.name = "btpower";
	rc = misc_register(&btqca->mdev);
	if (rc) {
		dev_err(dev, "failed to register miscdev, %d\n", rc);
		return rc;
	}

	/* Allocate Bluetooth rfkill device */
	btqca->rfkill_dev = rfkill_alloc("bt_power", dev,
					   RFKILL_TYPE_BLUETOOTH,
					   &btqca_rfkill_ops, btqca);
	if (!btqca->rfkill_dev) {
		rc = -ENOMEM;
		goto err_misc_unreg;
	}

	/* The default state of Bluetooth is off */
	btqca->is_enabled = false;
	rfkill_init_sw_state(btqca->rfkill_dev, true);

	/* Register rfkill */
	rc = rfkill_register(btqca->rfkill_dev);
	if (rc) {
		dev_err(dev, "Failed to register rfkill, %d\n", rc);
		goto err_rfkill_destroy;
	}
	return 0;

err_rfkill_destroy:
	rfkill_destroy(btqca->rfkill_dev);
err_misc_unreg:
	misc_deregister(&btqca->mdev);
	return rc;
}

static int btqca_pwr_remove(struct platform_device *pdev)
{
	struct btqca_data *btqca = platform_get_drvdata(pdev);

	dev_info(&pdev->dev, "%s\n", __func__);
	rfkill_unregister(btqca->rfkill_dev);
	rfkill_destroy(btqca->rfkill_dev);
	misc_deregister(&btqca->mdev);
	return 0;
}

#if defined(CONFIG_ACPI)
static const struct acpi_device_id btqca_power_acpi_match[] = {
	{"BTQC6850", 0},
	{"", 0},
};
MODULE_DEVICE_TABLE(acpi, btqca_power_acpi_match);
#endif

static struct platform_driver btqca_power_platform_driver = {
	.probe = btqca_pwr_probe,
	.remove = btqca_pwr_remove,
	.driver = {
		.name = "bt_power",
		.owner = THIS_MODULE,
#if defined(CONFIG_ACPI)
		.acpi_match_table = ACPI_PTR(btqca_power_acpi_match),
#endif
	},
};

module_platform_driver(btqca_power_platform_driver);

MODULE_AUTHOR("Magic Leap, Inc.");
MODULE_DESCRIPTION("QCA Bluetooth Power Driver");
MODULE_LICENSE("GPL v2");

