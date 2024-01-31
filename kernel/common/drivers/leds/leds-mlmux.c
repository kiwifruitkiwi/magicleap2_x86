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

#include <linux/acpi.h>
#include <linux/jiffies.h>
#include <linux/leds.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/workqueue.h>
#include <linux/ml-mux-client.h>
#include "leds-mlmux.h"

#define MLMUX_LED_CHAN_NAME		"LED_CHAN"
#define MLMUX_LED_TIMEOUT_MS		(50)
#define MLMUX_LED_NAME_BUF_SIZE		(8)
#define MLMUX_LED_SUSPEND_RESUME_MS	(2000)
#define LED_INDEX(i, j)			((i - 1) * MLMUX_LED_COLOR_COUNT + j)

static const char rgb[MLMUX_LED_COLOR_COUNT] = {
	[MLMUX_LED_RED] = 'r',
	[MLMUX_LED_GREEN] = 'g',
	[MLMUX_LED_BLUE] = 'b'
};

struct mlmux_led {
	uint8_t led_grp;
	uint8_t rgb_idx;
	char name[MLMUX_LED_NAME_BUF_SIZE];
	struct led_classdev cdev;
	struct mlmux_led_dev_data *ldev_data;
	struct completion resp_ready;
	struct mlmux_led_rx_msg resp;
	struct mutex led_lock;
};

struct mlmux_led_dev_data {
	bool ch_open;
	bool ldev_ready;
	bool sysfs_file;
	uint8_t led_groups;
	struct device *dev;
	struct mutex lock;
	struct mlmux_led *leds;
	struct ml_mux_client client;
	struct completion sysfs_resp_ready;
	struct mlmux_led_rx_msg sysfs_resp;
	struct workqueue_struct *work_q;
	struct completion suspend;
	struct completion resume;
};

struct mlmux_led_work {
	struct work_struct work;
	struct mlmux_led_dev_data *ldev_data;
	void *data;
};

static int mlmux_led_send_msg(struct mlmux_led_dev_data *ldev_data,
			      struct mlmux_led_tx_msg *tx_msg)
{
	int ret = -1;

	if (!ldev_data->ch_open) {
		dev_err(ldev_data->dev, "[%s] channel not open\n", __func__);
		return -ENOTCONN;
	}

	ret = ml_mux_send_msg(ldev_data->client.ch, sizeof(*tx_msg), tx_msg);
	if (ret)
		dev_err(ldev_data->dev, "[%s] send err=%d\n", __func__, ret);

	return ret;
}

static int mlmux_led_brightness_set(struct led_classdev *cdev,
				     enum led_brightness brightness)
{
	int ret = -1;
	unsigned long wait_time;
	struct mlmux_led_tx_msg tx_msg;
	struct mlmux_led *led_info = container_of(cdev, struct mlmux_led, cdev);
	struct mlmux_led_dev_data *ldev_data = led_info->ldev_data;

	if (!ldev_data->ldev_ready)
		goto exit;

	mutex_lock(&ldev_data->lock);

	tx_msg.u.data.led_grp = led_info->led_grp;
	tx_msg.u.data.rgb_idx = led_info->rgb_idx;
	tx_msg.u.data.brightness = brightness;
	tx_msg.type = MLMUX_LED_TX_BRIGHTNESS_WR;

	reinit_completion(&led_info->resp_ready);
	ret = mlmux_led_send_msg(ldev_data, &tx_msg);
	if (ret) {
		dev_err(ldev_data->dev, "set: failed send, ret=%d\n", ret);
		goto exit;
	}

	wait_time = wait_for_completion_interruptible_timeout(
		&led_info->resp_ready, msecs_to_jiffies(MLMUX_LED_TIMEOUT_MS));
	if (wait_time < 0) {
		ret = wait_time;
		dev_err(ldev_data->dev, "set: wait interrupted, ret=%d\n", ret);
		goto exit;
	}
	if (wait_time == 0) {
		dev_err(ldev_data->dev, "set: timeout, type=%ld\n",
			wait_time);
		ret = -ETIMEDOUT;
		goto exit;
	}

	if (led_info->resp.type != MLMUX_LED_RX_BRIGHTNESS_WR_ACK) {
		dev_err(ldev_data->dev, "set: unexpected response, type=%d\n",
			led_info->resp.type);
		ret = -ENOMSG;
	}

exit:
	mutex_unlock(&ldev_data->lock);

	return ret;
}

static enum led_brightness mlmux_led_brightness_get(struct led_classdev *cdev)
{
	int ret = -1;
	unsigned long wait_time;
	uint8_t ret_val;
	struct mlmux_led_tx_msg tx_msg;
	struct mlmux_led *led_info = container_of(cdev, struct mlmux_led, cdev);
	struct mlmux_led_dev_data *ldev_data = led_info->ldev_data;

	if (!ldev_data->ldev_ready)
		goto exit;

	mutex_lock(&ldev_data->lock);

	tx_msg.u.data.led_grp = led_info->led_grp;
	tx_msg.u.data.rgb_idx = led_info->rgb_idx;
	tx_msg.type = MLMUX_LED_TX_BRIGHTNESS_RD;

	reinit_completion(&led_info->resp_ready);
	ret = mlmux_led_send_msg(ldev_data, &tx_msg);
	if (ret) {
		dev_err(ldev_data->dev, "get: failed send, ret=%d\n", ret);
		goto exit;
	}

	wait_time = wait_for_completion_interruptible_timeout(
		&led_info->resp_ready, msecs_to_jiffies(MLMUX_LED_TIMEOUT_MS));
	if (wait_time < 0) {
		ret = wait_time;
		dev_err(ldev_data->dev, "get: wait interrupted, ret=%d\n", ret);
		goto exit;
	}
	if (wait_time == 0) {
		dev_err(ldev_data->dev, "get: timeout, type=%ld\n",
			wait_time);
		ret = -ETIMEDOUT;
		goto exit;
	}

	if (led_info->resp.type == MLMUX_LED_RX_BRIGHTNESS_RD_ACK) {
		ret_val = led_info->resp.content.data.brightness;
	} else {
		dev_err(ldev_data->dev, "get: unexpected response, type=%d\n",
			led_info->resp.type);
		ret = -ENOMSG;
	}
exit:
	mutex_unlock(&ldev_data->lock);

	return (ret == 0) ? ret_val : LED_OFF;
}

static ssize_t pattern_store(struct device *dev,
			     struct device_attribute *attr,
			     const char *buf, size_t len)
{
	int ret = -1;
	unsigned long wait_time;
	uint8_t pattern;
	struct mlmux_led_tx_msg tx_msg;
	struct mlmux_led_dev_data *ldev_data = dev_get_drvdata(dev);

	if (kstrtou8(buf, 10, &pattern) == -EINVAL)
		return len;

	tx_msg.u.e_data.pattern_type = pattern;
	tx_msg.type = MLMUX_LED_TX_PATTERN_WR;

	reinit_completion(&ldev_data->sysfs_resp_ready);
	ret = mlmux_led_send_msg(ldev_data, &tx_msg);
	if (ret) {
		dev_err(ldev_data->dev, "[%s] , msg send failure ret=%d\n",
			__func__, ret);
		goto exit;
	}

	wait_time = wait_for_completion_timeout(&ldev_data->sysfs_resp_ready,
					msecs_to_jiffies(MLMUX_LED_TIMEOUT_MS));
	if (!wait_time) {
		dev_err(ldev_data->dev, "pattern: timeout, type=%ld\n",
			wait_time);
		ret = -ETIMEDOUT;
		goto exit;
	}

	if (ldev_data->sysfs_resp.type != MLMUX_LED_RX_PATTERN_WR_ACK) {
		dev_err(ldev_data->dev, "get: unexpected response, type=%d\n",
			ldev_data->sysfs_resp.type);
		ret = -ENOMSG;
	}

exit:
	return ret == 0 ? len : ret;
}

static DEVICE_ATTR_WO(pattern);

static void mlmux_led_unregister_sysfs(struct mlmux_led_dev_data *ldev_data)
{
	int i;
	int led_count;

	mutex_lock(&ldev_data->lock);

	if (!ldev_data->leds) {
		mutex_unlock(&ldev_data->lock);
		return;
	}

	if (ldev_data->sysfs_file)
		sysfs_remove_file(&ldev_data->dev->kobj, &dev_attr_pattern.attr);

	led_count = ldev_data->led_groups * MLMUX_LED_COLOR_COUNT;

	for (i = 0; i < led_count; i++) {
		devm_led_classdev_unregister(ldev_data->dev,
					     &ldev_data->leds[i].cdev);
		mutex_destroy(&ldev_data->leds[i].led_lock);
	}

	devm_kfree(ldev_data->dev, ldev_data->leds);
	ldev_data->leds = NULL;

	mutex_unlock(&ldev_data->lock);
}

static void mlmux_led_register_sysfs(struct work_struct *work)
{
	int ret = -1;
	int index;
	int i, j;
	int led_count;
	struct mlmux_led *led;
	struct mlmux_led_work *led_work;
	struct mlmux_led_rx_msg *rx_msg;
	struct mlmux_led_dev_data *ldev_data;

	led_work = container_of(work, struct mlmux_led_work, work);
	ldev_data = led_work->ldev_data;
	rx_msg = (struct mlmux_led_rx_msg *)led_work->data;

	if (ldev_data->leds) {
		dev_warn(ldev_data->dev, "already registered\n");
		return;
	}

	ldev_data->led_groups = rx_msg->content.led_grp_count;
	led_count = ldev_data->led_groups * MLMUX_LED_COLOR_COUNT;

	ldev_data->leds = devm_kcalloc(ldev_data->dev, led_count,
				       sizeof(*ldev_data->leds), GFP_KERNEL);
	if (!ldev_data->leds) {
		dev_err(ldev_data->dev, "failed to allocate memory\n");
		return;
	}

	for (i = 0; i < ldev_data->led_groups; i++) {
		for (j = 0; j < MLMUX_LED_COLOR_COUNT; j++) {
			index = i * MLMUX_LED_COLOR_COUNT + j;
			led = &ldev_data->leds[index];
			/* index is 0 based but led group number is 1 based */
			led->led_grp = i + 1;
			led->rgb_idx = j;

			snprintf(led->name, MLMUX_LED_NAME_BUF_SIZE, "d%d-%c",
				 i + 1, rgb[j]);
			led->cdev.name = led->name;
			led->cdev.brightness_get = mlmux_led_brightness_get;
			led->cdev.brightness_set_blocking =
					mlmux_led_brightness_set;
			led->ldev_data = ldev_data;

			mutex_init(&led->led_lock);
			init_completion(&led->resp_ready);
			ret = devm_led_classdev_register(ldev_data->dev,
							 &led->cdev);
			if (ret) {
				dev_err(ldev_data->dev,
					"failed to register led %d\n", index);
				goto exit_sysfs;
			}
		}
	}
	ldev_data->ldev_ready = true;

	ret = sysfs_create_file(&ldev_data->dev->kobj, &dev_attr_pattern.attr);
	if (ret < 0) {
		dev_err(ldev_data->dev, "couldn't register mlmux sysfs file\n");
	} else {
		ldev_data->sysfs_file = true;
		init_completion(&ldev_data->sysfs_resp_ready);
		kobject_uevent(&ldev_data->dev->kobj, KOBJ_CHANGE);
		return;
	}

exit_sysfs:
	mlmux_led_unregister_sysfs(ldev_data);
}

static void mlmux_led_open_work(struct work_struct *work)
{
	struct mlmux_led_dev_data *ldev_data;
	struct mlmux_led_work *led_work;
	bool chan_open;

	led_work = container_of(work, struct mlmux_led_work, work);
	ldev_data = led_work->ldev_data;
	chan_open = *(bool *)led_work->data;

	if (ldev_data->ch_open == chan_open) {
		dev_warn(ldev_data->dev, "led chan already %s\n",
			 chan_open ? "opened" : "closed");
		goto exit;
	}
	ldev_data->ch_open = chan_open;

	dev_info(ldev_data->dev, "led channel is %s\n",
		 ldev_data->ch_open ? "opened" : "closed");

	if (!ldev_data->ch_open)
		mlmux_led_unregister_sysfs(ldev_data);

exit:
	devm_kfree(ldev_data->dev, led_work->data);
	devm_kfree(ldev_data->dev, led_work);
}

static void mlmux_led_proc_ack_work(struct work_struct *work)
{
	struct mlmux_led_dev_data *ldev_data;
	struct mlmux_led_work *led_work;
	struct mlmux_led_rx_msg *rx_msg;
	int led_idx;
	uint8_t grp;
	uint8_t color;

	led_work = container_of(work, struct mlmux_led_work, work);
	ldev_data = led_work->ldev_data;
	rx_msg = (struct mlmux_led_rx_msg *)led_work->data;

	grp = rx_msg->content.data.led_grp;
	color = rx_msg->content.data.rgb_idx;

	if (grp == 0 || grp > ldev_data->led_groups ||
		color >= MLMUX_LED_COLOR_COUNT) {
		dev_err(ldev_data->dev, "recv bad led info, grp=%u, rgb=%u\n",
			grp, color);
		return;
	}

	led_idx = LED_INDEX(grp, color);

	if (!ldev_data->leds) {
		dev_warn(ldev_data->dev, "recv ack but not registered\n");
		goto exit;
	}

	/* let waiting function handle resp, success or error type */
	mutex_lock(&ldev_data->lock);
	memcpy(&ldev_data->leds[led_idx].resp, rx_msg,
		MLMUX_LED_RX_SIZE(data));
	mutex_unlock(&ldev_data->lock);
	complete(&ldev_data->leds[led_idx].resp_ready);

exit:
	devm_kfree(ldev_data->dev, led_work->data);
	devm_kfree(ldev_data->dev, led_work);
}

static void mlmux_led_proc_pattern_work(struct work_struct *work)
{
	struct mlmux_led_dev_data *ldev_data;
	struct mlmux_led_work *led_work;
	struct mlmux_led_rx_msg *rx_msg;

	led_work = container_of(work, struct mlmux_led_work, work);
	ldev_data = led_work->ldev_data;
	rx_msg = (struct mlmux_led_rx_msg *)led_work->data;

	/* let waiting function handle resp, success or error type */
	mutex_lock(&ldev_data->lock);
	memcpy(&ldev_data->sysfs_resp, rx_msg,
		MLMUX_LED_RX_SIZE(data));
	mutex_unlock(&ldev_data->lock);
	complete(&ldev_data->sysfs_resp_ready);

	devm_kfree(ldev_data->dev, led_work->data);
	devm_kfree(ldev_data->dev, led_work);
}

static int mlmux_led_queue_msg(struct mlmux_led_dev_data *ldev_data,
				const void *data, size_t size,
				void (*work_func)(struct work_struct *))
{
	int ret = 0;
	struct mlmux_led_work *led_work = NULL;

	if (!size || !data) {
		dev_err(ldev_data->dev, "queue data is corrupted\n");
		ret = -EINVAL;
		goto exit;
	}

	led_work = devm_kzalloc(ldev_data->dev, sizeof(*led_work), GFP_KERNEL);
	if (!led_work) {
		ret = -ENOMEM;
		goto exit;
	}

	led_work->data = devm_kzalloc(ldev_data->dev, size, GFP_KERNEL);
	if (!led_work->data) {
		ret = -ENOMEM;
		devm_kfree(ldev_data->dev, led_work);
		goto exit;
	}

	memcpy(led_work->data, data, size);
	led_work->ldev_data = ldev_data;

	INIT_WORK(&led_work->work, work_func);
	queue_work(ldev_data->work_q, &led_work->work);

exit:
	return ret;
}

static void mlmux_led_open(struct ml_mux_client *cli, bool is_open)
{
	struct mlmux_led_dev_data *ldev_data;
	int ret = -1;

	ldev_data = container_of(cli, struct mlmux_led_dev_data, client);

	ret = mlmux_led_queue_msg(ldev_data, &is_open, sizeof(is_open),
				   mlmux_led_open_work);
	if (ret)
		dev_err(ldev_data->dev, "fail queue open msg err=%d\n", ret);
}

static void mlmux_led_recv(struct ml_mux_client *cli, uint32_t len, void *msg)
{
	int ret = -1;
	size_t expected_len;
	void (*fn)(struct work_struct *work) = NULL;
	struct mlmux_led_dev_data *ldev_data;
	struct mlmux_led_rx_msg *rx_msg = (struct mlmux_led_rx_msg *)msg;

	ldev_data = container_of(cli, struct mlmux_led_dev_data, client);

	switch (rx_msg->type) {
	case MLMUX_LED_RX_REG_CMD:
		expected_len = MLMUX_LED_RX_SIZE(led_grp_count);
		if (len != expected_len) {
			dev_err(ldev_data->dev,
				"Unexpected len %u vs %zu for msg type %u\n",
				len, expected_len, rx_msg->type);
			return;
		}
		fn = mlmux_led_register_sysfs;
		break;
	case MLMUX_LED_RX_BRIGHTNESS_WR_ACK:
	case MLMUX_LED_RX_BRIGHTNESS_RD_ACK:
	case MLMUX_LED_RX_ERROR:
		expected_len = MLMUX_LED_RX_SIZE(data);
		if (len != expected_len) {
			dev_err(ldev_data->dev,
				"Unexpected len %u vs %zu for msg type %u\n",
				len, expected_len, rx_msg->type);
			return;
		}
		fn = mlmux_led_proc_ack_work;
		break;
	case MLMUX_LED_RX_PATTERN_WR_ACK:
		expected_len = MLMUX_LED_RX_SIZE(data);
		if (len != expected_len) {
			dev_err(ldev_data->dev,
				"Unexpected len %u vs %zu for msg type %u\n",
				len, expected_len, rx_msg->type);
			return;
		}
		fn = mlmux_led_proc_pattern_work;
		break;
	case MLMUX_LED_RX_SUSPEND_ACK:
		expected_len = MLMUX_LED_RX_SIZE(data);
		if (len != expected_len) {
			dev_err(ldev_data->dev,
				"Unexpected len %u vs %zu for msg type %u\n",
				len, expected_len, rx_msg->type);
			return;
		}
		complete(&ldev_data->suspend);
		return;
	case MLMUX_LED_RX_RESUME_ACK:
		expected_len = MLMUX_LED_RX_SIZE(data);
		if (len != expected_len) {
			dev_err(ldev_data->dev,
				"Unexpected len %u vs %zu for msg type %u\n",
				len, expected_len, rx_msg->type);
			return;
		}
		complete(&ldev_data->resume);
		return;
	default:
		dev_err(ldev_data->dev, "recv unknown msg type = %d\n",
			rx_msg->type);
		return;
	}

	ret = mlmux_led_queue_msg(ldev_data, rx_msg, len, fn);
	if (ret)
		dev_err(ldev_data->dev, "fail queue proc_ack, err=%d\n", ret);
}

static int mlmux_led_probe(struct platform_device *pdev)
{
	int ret = 0;
	int status = 0;
	struct mlmux_led_dev_data *ldev_data = NULL;
	bool ch_open = true;

	dev_info(&pdev->dev, "Enter\n");

	ldev_data = devm_kzalloc(&pdev->dev, sizeof(*ldev_data), GFP_KERNEL);
	if (!ldev_data) {
		ret = -ENOMEM;
		goto exit;
	}
	ldev_data->dev = &pdev->dev;
	platform_set_drvdata(pdev, ldev_data);

	ldev_data->ldev_ready = false;
	ldev_data->ch_open = false;
	ldev_data->client.dev = ldev_data->dev;
	ldev_data->client.notify_open = mlmux_led_open;
	ldev_data->client.receive_cb = mlmux_led_recv;

	init_completion(&ldev_data->suspend);
	init_completion(&ldev_data->resume);

	mutex_init(&ldev_data->lock);

	ldev_data->work_q = alloc_workqueue(MLMUX_LED_CHAN_NAME,
					    WQ_UNBOUND, 1);
	if (!ldev_data->work_q) {
		dev_err(ldev_data->dev, "Failed to create workqueue\n");
		ret = -ENOMEM;
		goto exit_mtx_destroy;
	}

	status = ml_mux_request_channel(&ldev_data->client,
					MLMUX_LED_CHAN_NAME);

	/* If ML_MUX_CH_REQ_SUCCESS, will not queue msg since remote side
	 * is not open yet
	 */
	if (status == ML_MUX_CH_REQ_OPENED) {
		ret = mlmux_led_queue_msg(ldev_data, &ch_open,
					   sizeof(ch_open),
					   mlmux_led_open_work);
		if (ret) {
			dev_err(ldev_data->dev, "fail queue open msg err=%d\n",
				ret);
			goto exit_wq_destroy;
		}
	} else if (status < 0) {
		ret = status;
		goto exit_wq_destroy;
	}

	dev_info(&pdev->dev, "Success!\n");

	return 0;

exit_wq_destroy:
	destroy_workqueue(ldev_data->work_q);
exit_mtx_destroy:
	mutex_destroy(&ldev_data->lock);
exit:
	return ret;
}

static int mlmux_led_remove(struct platform_device *pdev)
{
	struct mlmux_led_dev_data *ldev_data = platform_get_drvdata(pdev);

	dev_dbg(ldev_data->dev, "remove\n");
	mlmux_led_unregister_sysfs(ldev_data);
	ml_mux_release_channel(ldev_data->client.ch);
	destroy_workqueue(ldev_data->work_q);
	mutex_destroy(&ldev_data->lock);

	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int mlmux_led_suspend(struct device *dev)
{
	int ret;
	unsigned long wait_time;
	struct mlmux_led_dev_data *ldev_data = dev_get_drvdata(dev);
	struct mlmux_led_tx_msg tx_msg;

	reinit_completion(&ldev_data->suspend);
	tx_msg.type = MLMUX_LED_TX_SUSPEND;

	ret = mlmux_led_send_msg(ldev_data, &tx_msg);
	if (ret) {
		dev_err(ldev_data->dev, "[%s] , msg send failure ret=%d\n",
			__func__, ret);
		return ret;
	}

	wait_time = wait_for_completion_timeout(&ldev_data->suspend,
		msecs_to_jiffies(MLMUX_LED_SUSPEND_RESUME_MS));
	if (!wait_time) {
		dev_err(ldev_data->dev, "suspend: timeout, type=%ld\n",
			wait_time);
		return -ETIMEDOUT;
	}
	dev_dbg(dev, "suspend\n");

	return 0;
}

static int mlmux_led_resume(struct device *dev)
{
	int ret;
	unsigned long wait_time;
	struct mlmux_led_dev_data *ldev_data = dev_get_drvdata(dev);
	struct mlmux_led_tx_msg tx_msg;

	reinit_completion(&ldev_data->resume);
	tx_msg.type = MLMUX_LED_TX_RESUME;

	ret = mlmux_led_send_msg(ldev_data, &tx_msg);
	if (ret) {
		dev_err(ldev_data->dev, "[%s] , msg send failure ret=%d\n",
			__func__, ret);
		return ret;
	}

	wait_time = wait_for_completion_timeout(&ldev_data->resume,
		msecs_to_jiffies(MLMUX_LED_SUSPEND_RESUME_MS));
	if (!wait_time) {
		dev_err(ldev_data->dev, "resume: timeout, type=%ld\n",
			wait_time);
		return -ETIMEDOUT;
	}
	dev_dbg(dev, "resume\n");

	return 0;
}

static const struct dev_pm_ops mlmux_led_pm = {
	.suspend = mlmux_led_suspend,
	.resume = mlmux_led_resume,
};
#define MLMUX_LED_PM_OPS	(&mlmux_led_pm)
#else
#define MLMUX_LED_PM_OPS	NULL
#endif

#ifdef CONFIG_OF
static const struct of_device_id mlmux_led_match_table[] = {
	{ .compatible = "ml,leds_mlmux", },
	{ },
};
MODULE_DEVICE_TABLE(of, mlmux_led_match_table);
#endif

#ifdef CONFIG_ACPI
static const struct acpi_device_id mlmux_led_acpi_match[] = {
	{"MXLD0000", 0},
	{ },
};
MODULE_DEVICE_TABLE(acpi, mlmux_led_acpi_match);
#endif

static struct platform_driver mlmux_led_driver = {
	.driver = {
		.name = "leds_mlmux",
		.of_match_table = of_match_ptr(mlmux_led_match_table),
		.pm = MLMUX_LED_PM_OPS,
		.acpi_match_table = ACPI_PTR(mlmux_led_acpi_match),
	},
	.probe = mlmux_led_probe,
	.remove = mlmux_led_remove,
};

module_platform_driver(mlmux_led_driver);

MODULE_AUTHOR("Magic Leap, Inc.");
MODULE_DESCRIPTION("led driver over mlmux");
MODULE_LICENSE("GPL v2");
