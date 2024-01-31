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
#include <linux/interrupt.h>
#include <linux/kfifo.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/cvip/gsm_jrtc.h>

#define ML_VICON_QUEUE_SIZE	(64)
#define DEFAULT_VSYNC_EDGE	(VSYNC_EDGE_BOTH)

struct ml_vicon_qdata {
	u64 ts;
	int value;
};

enum vsync_edge {
	VSYNC_EDGE_NONE    = 0,
	VSYNC_EDGE_RISING  = 1 << 0,
	VSYNC_EDGE_FALLING = 1 << 1,
	VSYNC_EDGE_BOTH    = VSYNC_EDGE_FALLING | VSYNC_EDGE_RISING
};

struct ml_vicon_mero_data {
	struct device	*dev;
	struct gpio_desc	*vsync_pin;
	int	vsync_irq;
	u8	vsync_edges;
	int8_t	gpio_val;
	u64	ts;
};

static DEFINE_KFIFO(vicon_queue, struct ml_vicon_qdata, ML_VICON_QUEUE_SIZE);

static irqreturn_t ml_vicon_mero_isr(int irq, void *data)
{
	struct ml_vicon_mero_data *vdata = (struct ml_vicon_mero_data *) data;

	vdata->ts = gsm_jrtc_get_time_ns();

	return IRQ_WAKE_THREAD;
}

static irqreturn_t ml_vicon_mero_threaded_isr(int irq, void *data)
{
	struct ml_vicon_mero_data *vdata = (struct ml_vicon_mero_data *) data;
	struct ml_vicon_qdata qdata;
	struct ml_vicon_qdata temp;

	qdata.ts = vdata->ts;
	if (gpiod_cansleep(vdata->vsync_pin))
		qdata.value = gpiod_get_value_cansleep(vdata->vsync_pin);
	else
		qdata.value = gpiod_get_value(vdata->vsync_pin);
	while (!kfifo_put(&vicon_queue, qdata)) {
		/* If queue is full, throw out the oldest content */
		if (!kfifo_get(&vicon_queue, &temp))
			goto exit;
	}
	sysfs_notify(&vdata->dev->kobj, NULL, "vsync_time");
exit:
	return IRQ_HANDLED;
}

static int setup_vsync_irq(struct ml_vicon_mero_data *vdata, u8 edges)
{
	int rc;
	int irq_flags = 0;

	if (edges & VSYNC_EDGE_RISING)
		irq_flags |= IRQF_TRIGGER_RISING;
	if (edges & VSYNC_EDGE_FALLING)
		irq_flags |= IRQF_TRIGGER_FALLING;

	rc =  devm_request_threaded_irq(vdata->dev, vdata->vsync_irq,
					ml_vicon_mero_isr,
					ml_vicon_mero_threaded_isr,
					irq_flags | IRQF_ONESHOT,
					"ml_vicon_mero_vsync", vdata);
	if (rc) {
		dev_err(vdata->dev, "Failed to enable vicon mero irq, %d\n",
			rc);
		return rc;
	}

	vdata->vsync_edges = edges;
	return 0;
}

static ssize_t vsync_time_show(struct device *dev,
			       struct device_attribute *attr,
			       char *buf)
{
	struct ml_vicon_qdata qval = {0,};
	struct ml_vicon_mero_data *vdata = dev_get_platdata(dev);

	if (!kfifo_get(&vicon_queue, &qval))
		dev_warn(vdata->dev, "Queue is empty!\n");
	else
		vdata->gpio_val = qval.value;
	return scnprintf(buf, PAGE_SIZE, "%llu\n", qval.ts);
}

static ssize_t gpio_val_show(struct device *dev,
			     struct device_attribute *attr,
			     char *buf)
{
	struct ml_vicon_mero_data *vdata = dev_get_platdata(dev);

	return scnprintf(buf, PAGE_SIZE, "%d\n", vdata->gpio_val);
}

static const char * const vsync_edge_str[] = {
	[VSYNC_EDGE_NONE]    = "none",
	[VSYNC_EDGE_RISING]  = "rising",
	[VSYNC_EDGE_FALLING] = "falling",
	[VSYNC_EDGE_BOTH]    = "both",
};

static ssize_t vsync_edge_show(struct device *dev,
			       struct device_attribute *attr,
			       char *buf)
{
	struct ml_vicon_mero_data *vdata = dev_get_platdata(dev);

	return scnprintf(buf, PAGE_SIZE, "%s\n",
			vsync_edge_str[vdata->vsync_edges]);
}

static ssize_t vsync_edge_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf,
				size_t count)
{
	u8 edges;
	int rc = 0;
	struct ml_vicon_mero_data *vdata = dev_get_platdata(dev);

	edges = sysfs_match_string(vsync_edge_str, buf);
	if ((edges < 0) || (edges == VSYNC_EDGE_NONE))
		return -EINVAL;

	devm_free_irq(dev, vdata->vsync_irq, vdata);
	rc = setup_vsync_irq(vdata, edges);
	if (rc)
		return -EIO;

	return count;
}

static DEVICE_ATTR_RO(vsync_time);
static DEVICE_ATTR_RO(gpio_val);
static DEVICE_ATTR_RW(vsync_edge);

static struct attribute *vicon_attributes[] = {
	&dev_attr_vsync_time.attr,
	&dev_attr_gpio_val.attr,
	&dev_attr_vsync_edge.attr,
	NULL
};

static const struct attribute_group vicon_attr_group = {
	.attrs = vicon_attributes,
};

#ifdef CONFIG_PM
static int ml_vicon_mero_suspend(struct device *dev)
{
	struct ml_vicon_mero_data *vdata = dev_get_drvdata(dev);

	disable_irq(vdata->vsync_irq);
	kfifo_reset(&vicon_queue);
	return 0;
}

static int ml_vicon_mero_resume(struct device *dev)
{
	struct ml_vicon_mero_data *vdata = dev_get_drvdata(dev);

	enable_irq(vdata->vsync_irq);
	return 0;
}

static const struct dev_pm_ops ml_vicon_mero_ops = {
	.suspend = ml_vicon_mero_suspend,
	.resume = ml_vicon_mero_resume,
};
#endif

static int ml_vicon_mero_probe(struct platform_device *pdev)
{
	struct ml_vicon_mero_data *vdata;
	struct device *dev = &pdev->dev;
	int rc;

	dev_info(dev, "%s enter", __func__);

	vdata = devm_kzalloc(dev, sizeof(*vdata), GFP_KERNEL);
	if (!vdata)
		return -ENOMEM;

	vdata->dev = dev;
	pdev->dev.platform_data = vdata;
	vdata->gpio_val = -1;
	platform_set_drvdata(pdev, vdata);

	/* GPIO IRQ signal to trigger vsync data */
	vdata->vsync_pin = devm_gpiod_get(dev, "vsync", GPIOD_IN);
	if (IS_ERR(vdata->vsync_pin)) {
		dev_err(dev, "Failed to get vsync pin\n");
		return PTR_ERR(vdata->vsync_pin);
	}

	vdata->vsync_irq = gpiod_to_irq(vdata->vsync_pin);
	if (vdata->vsync_irq < 0) {
		dev_err(dev, "Failed to get irq\n");
		return vdata->vsync_irq;
	}

	rc = sysfs_create_group(&pdev->dev.kobj, &vicon_attr_group);
	if (rc) {
		dev_err(dev, "unable to create sysfs node %d\n", rc);
		return rc;
	}

	rc = setup_vsync_irq(vdata, DEFAULT_VSYNC_EDGE);
	if (rc)
		sysfs_remove_group(&pdev->dev.kobj, &vicon_attr_group);

	return rc;
}

static int ml_vicon_mero_remove(struct platform_device *pdev)
{
	struct ml_vicon_mero_data *vdata = dev_get_drvdata(&pdev->dev);

	disable_irq(vdata->vsync_irq);
	sysfs_remove_group(&pdev->dev.kobj, &vicon_attr_group);

	return 0;
}

#ifdef CONFIG_ACPI
static const struct acpi_device_id ml_vicon_mero_acpi_match[] = {
	{"MDA0000", 0},
	{"", 0},
};
MODULE_DEVICE_TABLE(acpi, ml_vicon_mero_acpi_match);
#endif

static struct platform_driver ml_vicon_mero_driver = {
	.driver = {
		.name = "ml_vicon_mero",
		.owner = THIS_MODULE,
#ifdef CONFIG_PM
		.pm = &ml_vicon_mero_ops,
#endif
		.acpi_match_table = ACPI_PTR(ml_vicon_mero_acpi_match),
	},
	.probe = ml_vicon_mero_probe,
	.remove = ml_vicon_mero_remove,
};
module_platform_driver(ml_vicon_mero_driver);

MODULE_ALIAS("Magic Leap, Inc.");
MODULE_DESCRIPTION("Motion capture for Vicon on Mero");
MODULE_LICENSE("GPL");
