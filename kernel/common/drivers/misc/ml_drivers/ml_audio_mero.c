// SPDX-License-Identifier: GPL-2.0
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

#include <linux/acpi.h>
#include <linux/gpio/consumer.h>
#include <linux/module.h>
#include <linux/platform_device.h>

struct ml_audio_mero_data {
	struct device	*dev;
	struct gpio_desc	*trigger_pin;
};

static ssize_t latency_test_pin_show(struct device *dev,
				     struct device_attribute *attr,
				     char *buf)
{
	struct ml_audio_mero_data *adata = dev_get_platdata(dev);
	u8 latency_test_pin;

	if (gpiod_cansleep(adata->trigger_pin))
		latency_test_pin = gpiod_get_value_cansleep(adata->trigger_pin);
	else
		latency_test_pin = gpiod_get_value(adata->trigger_pin);

	return scnprintf(buf, PAGE_SIZE, "%d\n", latency_test_pin);
}

static ssize_t latency_test_pin_store(struct device *dev,
				      struct device_attribute *attr,
				      const char *buf,
				      size_t count)
{
	int rc;
	struct ml_audio_mero_data *adata = dev_get_platdata(dev);
	u8 val;

	rc = kstrtou8(buf, 0, &val);
	if (rc)
		return rc;

	val = !!val;
	if (gpiod_cansleep(adata->trigger_pin))
		gpiod_set_value_cansleep(adata->trigger_pin, val);
	else
		gpiod_set_value(adata->trigger_pin, val);

	return count;
}

static DEVICE_ATTR_RW(latency_test_pin);

static struct attribute *audio_attributes[] = {
	&dev_attr_latency_test_pin.attr,
	NULL
};

static const struct attribute_group audio_attr_group = {
	.attrs = audio_attributes,
};

static int ml_audio_mero_probe(struct platform_device *pdev)
{
	struct ml_audio_mero_data *adata;
	struct device *dev = &pdev->dev;
	int rc;

	dev_info(dev, "%s enter", __func__);

	adata = devm_kzalloc(dev, sizeof(*adata), GFP_KERNEL);
	if (!adata)
		return -ENOMEM;

	adata->dev = dev;
	pdev->dev.platform_data = adata;
	platform_set_drvdata(pdev, adata);

	/* GPIO for audio benchmark */
	adata->trigger_pin = devm_gpiod_get(dev, "trigger", GPIOD_ASIS);
	if (IS_ERR(adata->trigger_pin)) {
		dev_err(dev, "Failed to get trigger pin\n");
		return PTR_ERR(adata->trigger_pin);
	}
	gpiod_direction_output(adata->trigger_pin, 1);
	rc = sysfs_create_group(&pdev->dev.kobj, &audio_attr_group);
	if (rc) {
		dev_err(dev, "unable to create sysfs node %d\n", rc);
		return rc;
	}

	return rc;
}

static int ml_audio_mero_remove(struct platform_device *pdev)
{
	sysfs_remove_group(&pdev->dev.kobj, &audio_attr_group);

	return 0;
}

#ifdef CONFIG_ACPI
static const struct acpi_device_id ml_audio_mero_acpi_match[] = {
	{"AUD0000", 0},
	{"", 0},
};
MODULE_DEVICE_TABLE(acpi, ml_audio_mero_acpi_match);
#endif

static struct platform_driver ml_audio_mero_driver = {
	.driver = {
		.name = "ml_audio_mero",
		.owner = THIS_MODULE,
		.acpi_match_table = ACPI_PTR(ml_audio_mero_acpi_match),
	},
	.probe = ml_audio_mero_probe,
	.remove = ml_audio_mero_remove,
};
module_platform_driver(ml_audio_mero_driver);

MODULE_ALIAS("Magic Leap, Inc.");
MODULE_DESCRIPTION("Audio benchmark driver for Mero");
MODULE_LICENSE("GPL");
