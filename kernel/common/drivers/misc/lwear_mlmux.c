// SPDX-License-Identifier: GPL-2.0
/* Copyright (c) 2022, Magic Leap, Inc. All rights reserved.
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

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/acpi.h>
#include <linux/delay.h>
#include <linux/ml-mux-client.h>
#include <linux/ktime.h>
#include "lwear_mlmux.h"

#define LWEAR_MUX_CHAN "LWEAR_MUX_CHAN"
#define MLMUX_LWEAR_TIMEOUT_MS (2000)
#define LWEAR_POLL_LIMIT_MS (5000)

struct lwear_drv_data {
	struct device *dev;
	struct ml_mux_client mlmux_client;
	struct completion complete;
	struct mutex tx_lock;
	enum lwear_state_report state;
};

/* Callback function for channel open. Not used at the moment */
static void lwear_mux_open(struct ml_mux_client *cli, bool is_open)
{
	pr_info("lwear mux channel open: %d\n", is_open);
}

static void lwear_mux_rx(struct ml_mux_client *cli, uint32_t len, void *data)
{
	struct lwear_drv_data *drv_data = container_of(cli,
			struct lwear_drv_data, mlmux_client);
	struct lwear_rx_msg *msg = (struct lwear_rx_msg *)data;

	switch (msg->id) {
	case LWEAR_STATE_REPORT_ACK:
		drv_data->state = msg->state;
		if (!completion_done(&drv_data->complete))
			complete(&drv_data->complete);
		break;

	default:
		dev_err(drv_data->dev, "invalid lwear msg id: %d\n", msg->id);
	}
}

/* string representation of "enum lwear_state_report" */
static const char * const lwear_state_text[] = {
	"On", "Off", "Not connected", "Fault"
};

static ktime_t last_read_time;

static ssize_t lwear_state_show(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	struct lwear_drv_data *drv_data = dev_get_drvdata(dev);
	struct lwear_tx_msg tx_msg;
	unsigned long timeout;
	enum lwear_state_report state;
	int ret = 0;
	ktime_t now;
	long delta_ms;

	if (!drv_data)
		return -ENODEV;

	mutex_lock(&drv_data->tx_lock);

	now = ktime_get();
	delta_ms = ktime_to_ms(ktime_sub(now, last_read_time));
	if (delta_ms < LWEAR_POLL_LIMIT_MS) {
		ret = -EAGAIN;
		goto err;
	}
	last_read_time = now;

	tx_msg.id = LWEAR_STATE_REPORT_REQ;
	reinit_completion(&drv_data->complete);
	ret = ml_mux_send_msg(drv_data->mlmux_client.ch,
		    sizeof(tx_msg), &tx_msg);
	if (ret) {
		dev_err(drv_data->dev, "mlmux send err=%d\n", ret);
		ret = -EIO;
		goto err;
	}

	timeout = wait_for_completion_interruptible_timeout(&drv_data->complete,
				msecs_to_jiffies(MLMUX_LWEAR_TIMEOUT_MS));
	if (timeout == 0) {
		dev_err(drv_data->dev, "LWEAR_STATE_REPORT_REQ timed out");
		ret = -ETIMEDOUT;
		goto err;
	} else if (timeout < 0) {
		dev_warn(drv_data->dev, "LWEAR_STATE_REPORT_REQ interrupted");
		ret = -EINTR;
		goto err;
	}

	state = drv_data->state;
	if (state >= ARRAY_SIZE(lwear_state_text)) {
		dev_err(drv_data->dev, "invalid lwear state received");
		ret = -EBADMSG;
		goto err;
	}
	mutex_unlock(&drv_data->tx_lock);

	return sprintf(buf, "%s\n", lwear_state_text[state]);

err:
	mutex_unlock(&drv_data->tx_lock);
	return ret;
}

DEVICE_ATTR_RO(lwear_state);

static int ec_lwear_comp_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct lwear_drv_data *drv_data;

	dev_info(&pdev->dev, "probe enter\n");

	drv_data = devm_kzalloc(&pdev->dev, sizeof(*drv_data), GFP_KERNEL);
	if (!drv_data)
		return -ENOMEM;

	drv_data->dev = &pdev->dev;
	platform_set_drvdata(pdev, drv_data);

	drv_data->mlmux_client.dev = &pdev->dev;
	drv_data->mlmux_client.notify_open = lwear_mux_open;
	drv_data->mlmux_client.receive_cb = lwear_mux_rx;
	drv_data->state = LWEAR_STATE_REPORT_OFF;

	ret = ml_mux_request_channel(&drv_data->mlmux_client, LWEAR_MUX_CHAN);
	if (ret < 0) {
		dev_err(drv_data->dev,
			"failed to create lwear channel: %d\n", ret);
		devm_kfree(&pdev->dev, drv_data);
		return ret;
	}

	if (ret == ML_MUX_CH_REQ_OPENED)
		dev_info(drv_data->dev, "LWEAR_MUX_CHAN is already opened\n");

	ret = sysfs_create_file(&drv_data->dev->kobj,
			&dev_attr_lwear_state.attr);
	if (ret < 0) {
		dev_err(drv_data->dev,
			"failed to create sysfs entry. %d\n", ret);
		ret = -EIO;
		goto chnl_remove;
	}

	init_completion(&drv_data->complete);
	mutex_init(&drv_data->tx_lock);

	return 0;

chnl_remove:
	devm_kfree(&pdev->dev, drv_data);
	ml_mux_release_channel(drv_data->mlmux_client.ch);

	return ret;
}

static int ec_lwear_comp_remove(struct platform_device *pdev)
{
	struct lwear_drv_data *drv_data = platform_get_drvdata(pdev);

	sysfs_remove_file(&drv_data->dev->kobj, &dev_attr_lwear_state.attr);
	ml_mux_release_channel(drv_data->mlmux_client.ch);

	return 0;
}

#ifdef CONFIG_ACPI
static const struct acpi_device_id ec_lwear_comp_acpi_match[] = {
	{ "MXLW0000", 0 },
	{ },
};
MODULE_DEVICE_TABLE(acpi, ec_lwear_comp_acpi_match);
#endif

static struct platform_driver ec_lwear_comp_drv = {
	.probe = ec_lwear_comp_probe,
	.remove = ec_lwear_comp_remove,
	.driver = {
		.name = "ec-lwear-ch",
		.acpi_match_table = ACPI_PTR(ec_lwear_comp_acpi_match),
	}
};
module_platform_driver(ec_lwear_comp_drv);

MODULE_AUTHOR("Magic Leap, Inc.");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("ML2 lwear mlmux driver");
