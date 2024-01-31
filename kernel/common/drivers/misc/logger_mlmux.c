// SPDX-License-Identifier: GPL-2.0
/* Copyright (c) 2020, Magic Leap, Inc. All rights reserved.
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
#include "logger_mlmux.h"

#define EC_LOGGER_CHAN_NAME "LOGGER_EC_CHAN"
#define MLMUX_EC_LOGGER_TIMEOUT_MS (50)

struct lgr_drv_data {
	struct device *dev;
	struct ml_mux_client mlmux_client;
	struct completion s_cmpl;
	struct mutex tx_lock;
	struct workqueue_struct *work_q;
	bool lgr_ready;
};

struct ec_lgr_work {
	struct work_struct work;
	struct lgr_drv_data *drv_data;
	void *data;
};

/* Callback function for channel open. Not used at the moment */
static void ec_logger_open(struct ml_mux_client *cli, bool is_open)
{

}

static void ec_logger_recv(struct ml_mux_client *cli, uint32_t len, void *msg)
{
	uint8_t recv_code;
	struct lgr_drv_data *drv_data = container_of(cli, struct lgr_drv_data,
						     mlmux_client);

	recv_code = ((struct logger_rx_msg *)msg)->type;

	switch (recv_code) {
	case LOGGER_RX_FLUSH_ACK:
	case LOGGER_RX_LGR_DUMP_ACK:
		complete(&drv_data->s_cmpl);
		break;

	default:
		dev_err(drv_data->dev, "unknw. recv code %d\n", recv_code);
	}
}

static ssize_t logger_store(struct device *dev, struct device_attribute *attr,
			    const char *buf, size_t len)
{
	int ret;
	uint8_t cmd;
	struct logger_tx_msg tx_cmd;
	unsigned long timeout;
	struct lgr_drv_data *drv_data = dev->driver_data;

	mutex_lock(&drv_data->tx_lock);

	if (kstrtou8(buf, 10, &cmd) == -EINVAL) {
		ret = 0;
		goto exit;
	}

	if (cmd >= MAX_CMDS) {
		dev_err(drv_data->dev, "invalid logger cmd =%d\n", cmd);
		ret = 0;
		goto exit;
	}

	tx_cmd.type = cmd;

	reinit_completion(&drv_data->s_cmpl);
	ret = ml_mux_send_msg(drv_data->mlmux_client.ch, sizeof(tx_cmd),
			      &tx_cmd);
	if (ret) {
		dev_err(drv_data->dev, "mlmux send err=%d\n", ret);
		goto exit;
	}

	timeout = wait_for_completion_interruptible_timeout(&drv_data->s_cmpl,
				msecs_to_jiffies(MLMUX_EC_LOGGER_TIMEOUT_MS));
	if (timeout <= 0) {
		dev_err(drv_data->dev, "ack timeout =%d, cmd=%d\n", ret, cmd);
		ret = -ETIMEDOUT;
		goto exit;
	}

exit:
	mutex_unlock(&drv_data->tx_lock);
	return (ret == 0) ? len : ret;
}

DEVICE_ATTR_WO(logger);

static int ec_logger_comp_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct lgr_drv_data *drv_data;
	int mlmux_sts = -1;

	dev_info(&pdev->dev, "probe enter\n");

	drv_data = devm_kzalloc(&pdev->dev, sizeof(*drv_data), GFP_KERNEL);
	if (!drv_data)
		return -ENOMEM;

	drv_data->dev = &pdev->dev;
	platform_set_drvdata(pdev, drv_data);

	drv_data->mlmux_client.dev = &pdev->dev;
	drv_data->mlmux_client.notify_open = ec_logger_open;
	drv_data->mlmux_client.receive_cb = ec_logger_recv;

	mlmux_sts = ml_mux_request_channel(&drv_data->mlmux_client,
					   EC_LOGGER_CHAN_NAME);
	if (mlmux_sts == ML_MUX_CH_REQ_OPENED)
		dev_info(drv_data->dev, "lgr chan is open\n");

	if (mlmux_sts < 0) {
		if (mlmux_sts == -EPROBE_DEFER)
			dev_err(drv_data->dev,
				"req. driver probe re-entry %d\n", mlmux_sts);
		else
			dev_err(drv_data->dev,
				"failed to req. mlmux chnl. %d\n", mlmux_sts);
		ret = mlmux_sts;
		goto chnl_remove;
	} else if (mlmux_sts == ML_MUX_CH_REQ_SUCCESS) {
		dev_info(drv_data->dev, "lgr chnl req. success\n");
	}

	ret = sysfs_create_file(&drv_data->dev->kobj, &dev_attr_logger.attr);
	if (ret < 0) {
		dev_err(drv_data->dev,
			"failed to create sysfs entry. %d\n", ret);
		ret = -EIO;
		goto chnl_remove;
	}

	init_completion(&drv_data->s_cmpl);
	mutex_init(&drv_data->tx_lock);

	return 0;

chnl_remove:
	ml_mux_release_channel(drv_data->mlmux_client.ch);

	return ret;
}

static int ec_logger_comp_remove(struct platform_device *pdev)
{
	struct lgr_drv_data *drv_data = platform_get_drvdata(pdev);

	sysfs_remove_file(&drv_data->dev->kobj, &dev_attr_logger.attr);
	ml_mux_release_channel(drv_data->mlmux_client.ch);

	return 0;
}

#ifdef CONFIG_ACPI
static const struct acpi_device_id ec_logger_comp_acpi_match[] = {
	{ "MXLG0000", 0 },
	{ },
};
MODULE_DEVICE_TABLE(acpi, ec_logger_comp_acpi_match);
#endif

static struct platform_driver ec_logger_comp_drv = {
	.probe = ec_logger_comp_probe,
	.remove = ec_logger_comp_remove,
	.driver = {
		.name = "ec-logger-ch",
		.acpi_match_table = ACPI_PTR(ec_logger_comp_acpi_match),
	}
};
module_platform_driver(ec_logger_comp_drv);

MODULE_AUTHOR("Magic Leap, Inc.");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("ML2 EC logger MLMUX channel");
