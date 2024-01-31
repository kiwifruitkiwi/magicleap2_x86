// SPDX-License-Identifier: GPL-2.0
/* Copyright (c) 2019, Magic Leap, Inc. All rights reserved.
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
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include "mlmux_test_cli.h"

#define MLMUX_TEST_TIMEOUT_MS (500)

struct mlmux_test_dev_data {
	struct device *dev;
	struct ml_mux_client client;
	const char *chan_name;
	struct workqueue_struct *work_q;
	struct completion suspend;
	struct completion resume;
	struct mutex lock;
	bool ch_open;

	/* test related variables */
	bool auto_test;
	uint32_t auto_test_cnt;
	struct completion test_cmplt;
	char msg_buf[MLMUX_TEST_DATA_STR_LEN];
};

struct mlmux_test_work {
	struct work_struct work;
	struct mlmux_test_dev_data *tdev;
	void *data;
};

static int mlmux_test_send_test_msg(struct mlmux_test_dev_data *tdev,
				    __le32 data,
				    const char *str)
{
	static struct mlmux_test_tx_msg msg = {.type = MLMUX_TEST_TX_TEST};
	int ret = 0;

	mutex_lock(&tdev->lock);
	if (!tdev->ch_open) {
		dev_err(tdev->dev, "test channel not open\n");
		goto exit;
	}
	msg.u.test_msg.data = data;
	snprintf(msg.u.test_msg.str, sizeof(msg.u.test_msg.str), "%s", str);
	ret = ml_mux_send_msg(tdev->client.ch, sizeof(msg), &msg);
	if (ret)
		dev_err(tdev->dev, "mlmux send err=%d\n", ret);

exit:
	mutex_unlock(&tdev->lock);
	return ret;
}

static ssize_t auto_test_store(struct device *dev,
			    struct device_attribute *attr,
			    const char *buf,
			    size_t count)
{
	struct platform_device *pdev =
			container_of(dev, struct platform_device, dev);
	struct mlmux_test_dev_data *tdev = platform_get_drvdata(pdev);
	const char *str = "x86 AUTOTEST MLMUX CHANNEL";
	int ret;
	uint32_t data;

	ret = kstrtou32(buf, 10, &data);
	if (ret) {
		dev_err(dev, "%s: failed to convert data value", __func__);
		goto exit;
	}
	tdev->auto_test = !!data;
	tdev->auto_test_cnt = 0;
	mlmux_test_send_test_msg(tdev, cpu_to_le32(tdev->auto_test_cnt++),
				 str);

exit:
	return ret ? ret : count;
}

static ssize_t auto_test_show(struct device *dev,
			 struct device_attribute *attr,
			 char *buf)
{
	struct platform_device *pdev =
			container_of(dev, struct platform_device, dev);
	struct mlmux_test_dev_data *tdev = platform_get_drvdata(pdev);

	return scnprintf(buf, PAGE_SIZE, "auto_test=%d\n", tdev->auto_test);
}
static DEVICE_ATTR_RW(auto_test);

static ssize_t test_store(struct device *dev,
			    struct device_attribute *attr,
			    const char *buf,
			    size_t count)
{
	struct platform_device *pdev =
			container_of(dev, struct platform_device, dev);
	struct mlmux_test_dev_data *tdev = platform_get_drvdata(pdev);
	const char *str = "x86 TEST MLMUX CHANNEL";
	int ret;
	uint32_t data;

	ret = kstrtou32(buf, 10, &data);
	if (ret) {
		dev_err(dev, "%s: failed to convert data value", __func__);
		goto exit;
	}
	reinit_completion(&tdev->test_cmplt);

	ret = mlmux_test_send_test_msg(tdev, cpu_to_le32(data), str);
	if (ret)
		complete(&tdev->test_cmplt);

exit:
	return ret ? ret : count;
}

static ssize_t test_show(struct device *dev,
			 struct device_attribute *attr,
			 char *buf)
{
	struct platform_device *pdev =
			container_of(dev, struct platform_device, dev);
	struct mlmux_test_dev_data *tdev = platform_get_drvdata(pdev);
	int t = msecs_to_jiffies(MLMUX_TEST_TIMEOUT_MS);
	long ret;

	ret = wait_for_completion_interruptible_timeout(&tdev->test_cmplt, t);
	if (ret == 0)
		return scnprintf(buf, PAGE_SIZE, "test error: timeout\n");
	else if (ret < 0)
		return scnprintf(buf, PAGE_SIZE, "test error: %lu\n", ret);

	return scnprintf(buf, PAGE_SIZE, "%s\n", tdev->msg_buf);
}
static DEVICE_ATTR_RW(test);


static struct attribute *mlm_test_ch_attrs[] = {
	&dev_attr_auto_test.attr,
	&dev_attr_test.attr,
	NULL
};

static const struct attribute_group mlm_test_ch_attr_group = {
	.attrs = mlm_test_ch_attrs,
};

static void mlmux_test_proc_test_msg(struct work_struct *work)
{
	struct mlmux_test_dev_data *tdev;
	struct mlmux_test_work *test_work;
	struct mlmux_test_rx_test *rx_tst;
	const char *str = "x86 AUTOTEST MLMUX CHANNEL";
	static int count = 100;

	test_work = container_of(work, struct mlmux_test_work, work);
	tdev = test_work->tdev;

	rx_tst = (struct mlmux_test_rx_test *)test_work->data;
	/* If auto test is ON resend the message to create ping pong msging */
	if (tdev->auto_test) {
		if (rx_tst && count == 100) {
			dev_info(tdev->dev, "%s - %d\n",
				 rx_tst->str, le32_to_cpu(rx_tst->data));
			count = 0;
		}
		count++;
		mlmux_test_send_test_msg(tdev,
					 cpu_to_le32(tdev->auto_test_cnt++),
					 str);
	}

	if (rx_tst) {
		snprintf(tdev->msg_buf, sizeof(tdev->msg_buf), "%s - %u",
			 rx_tst->str, le32_to_cpu(rx_tst->data));
		complete(&tdev->test_cmplt);
		devm_kfree(tdev->dev, rx_tst);
	}

	devm_kfree(tdev->dev, test_work);
}

static inline int mlmux_test_create_sysfs(struct mlmux_test_dev_data *tdev)
{
	int ret;

	ret = sysfs_create_group(&tdev->dev->kobj, &mlm_test_ch_attr_group);
	if (ret < 0)
		dev_err(tdev->dev, "failed to create sysfs node %d\n", ret);

	return ret;
}

static inline void mlmux_test_remove_sysfs(struct mlmux_test_dev_data *tdev)
{
	sysfs_remove_group(&tdev->dev->kobj, &mlm_test_ch_attr_group);
}

static void mlmux_test_proc_open_msg(struct work_struct *work)
{
	struct mlmux_test_dev_data *tdev;
	struct mlmux_test_work *test_work;
	bool chan_open;

	test_work = container_of(work, struct mlmux_test_work, work);
	tdev = test_work->tdev;
	chan_open = *(bool *)test_work->data;

	mutex_lock(&tdev->lock);
	if (tdev->ch_open == chan_open) {
		dev_warn(tdev->dev, "test chan already %s\n",
			 chan_open ? "opened" : "closed");
		goto exit;
	}
	tdev->ch_open = chan_open;

	if (tdev->ch_open)
		mlmux_test_create_sysfs(tdev);
	else
		mlmux_test_remove_sysfs(tdev);

	dev_info(tdev->dev, "mlmux channel is %s\n",
		 tdev->ch_open ? "opened" : "closed");

exit:
	mutex_unlock(&tdev->lock);
}

static int mlmux_test_queue_msg(struct mlmux_test_dev_data *tdev,
				const void *data, size_t size,
				void (*work_func)(struct work_struct *))
{
	int ret = 0;
	struct mlmux_test_work *test_work = NULL;

	if (!size || !data) {
		dev_err(tdev->dev, "test msg is corrupted\n");
		ret = -EINVAL;
		goto exit;
	}

	test_work = devm_kzalloc(tdev->dev, sizeof(*test_work), GFP_KERNEL);
	if (!test_work) {
		ret = -ENOMEM;
		goto exit;
	}

	test_work->data = devm_kzalloc(tdev->dev, size, GFP_KERNEL);
	if (!test_work->data) {
		ret = -ENOMEM;
		devm_kfree(tdev->dev, test_work);
		goto exit;
	}
	memcpy(test_work->data, data, size);

	test_work->tdev = tdev;

	INIT_WORK(&test_work->work, work_func);
	queue_work(tdev->work_q, &test_work->work);

exit:
	return ret;
}

static void mlmux_test_open(struct ml_mux_client *cli, bool is_open)
{
	struct mlmux_test_dev_data *tdev;
	int ret;

	tdev = container_of(cli, struct mlmux_test_dev_data, client);

	ret = mlmux_test_queue_msg(tdev, &is_open, sizeof(is_open),
				   mlmux_test_proc_open_msg);
	if (ret)
		dev_err(tdev->dev, "fail to q open msg err=%d\n", ret);
}

static void mlmux_test_recv(struct ml_mux_client *cli, uint32_t len, void *msg)
{
	struct mlmux_test_dev_data *tdev;
	struct mlmux_test_rx_msg *msg_rx = (struct mlmux_test_rx_msg *)msg;
	int ret;

	tdev = container_of(cli, struct mlmux_test_dev_data, client);

	switch (msg_rx->type) {
	case MLMUX_TEST_RX_TEST:
		ret = mlmux_test_queue_msg(tdev, &msg_rx->u.test_msg,
					   sizeof(msg_rx->u.test_msg),
					   mlmux_test_proc_test_msg);
		if (ret)
			dev_err(tdev->dev, "fail to q test msg err=%d\n", ret);
	break;

	default:
		dev_err(tdev->dev, "Unknown cmd type %d\n", msg_rx->type);
		return;
	}
}

static int mlmux_test_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct mlmux_test_dev_data *tdev = NULL;
	bool ch_open = true;

	tdev = devm_kzalloc(&pdev->dev, sizeof(*tdev), GFP_KERNEL);
	if (!tdev) {
		ret = -ENOMEM;
		goto exit;
	}
	tdev->dev = &pdev->dev;
	platform_set_drvdata(pdev, tdev);

	tdev->chan_name = "TEST_CHAN";
	tdev->client.dev = tdev->dev;
	tdev->client.notify_open = mlmux_test_open;
	tdev->client.receive_cb = mlmux_test_recv;

	init_completion(&tdev->suspend);
	init_completion(&tdev->resume);
	init_completion(&tdev->test_cmplt);
	mutex_init(&tdev->lock);

	tdev->work_q = alloc_workqueue(tdev->chan_name, WQ_UNBOUND, 1);
	if (!tdev->work_q) {
		dev_info(tdev->dev, "Failed to create workqueue\n");
		ret = -ENOMEM;
		goto exit_mtx_destroy;
	}

	ret = ml_mux_request_channel(&tdev->client, (char *)tdev->chan_name);
	if (ret == ML_MUX_CH_REQ_OPENED) {
		if (mlmux_test_queue_msg(tdev, &ch_open, sizeof(ch_open),
					 mlmux_test_proc_open_msg))
			dev_err(tdev->dev, "fail to q open msg err=%d\n", ret);
	} else if (ret < 0) {
		goto exit_wq_destroy;
	}
	dev_info(&pdev->dev, "Success!\n");

	return 0;

exit_wq_destroy:
	destroy_workqueue(tdev->work_q);
exit_mtx_destroy:
	mutex_destroy(&tdev->lock);
exit:
	return ret;
}

static int mlmux_test_remove(struct platform_device *pdev)
{
	struct mlmux_test_dev_data *tdev = platform_get_drvdata(pdev);

	dev_dbg(tdev->dev, "remove\n");
	mlmux_test_remove_sysfs(tdev);
	ml_mux_release_channel(tdev->client.ch);
	destroy_workqueue(tdev->work_q);
	mutex_destroy(&tdev->lock);

	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int mlmux_test_suspend(struct device *dev)
{
	dev_dbg(dev, "suspend\n");
	/* Place holder */

	return 0;
}

static int mlmux_test_resume(struct device *dev)
{
	dev_dbg(dev, "resume\n");
	/* Place holder */

	return 0;
}

static const struct dev_pm_ops mlmux_test_pm = {
	.suspend = mlmux_test_suspend,
	.resume = mlmux_test_resume,
};
#define MLMUX_TEST_PM_OPS	(&mlmux_test_pm)
#else
#define MLMUX_TEST_PM_OPS	NULL
#endif


static const struct of_device_id ml_test_match_table[] = {
	{ .compatible = "ml,test_mlmux", },
	{ },
};
MODULE_DEVICE_TABLE(of, ml_test_match_table);


#ifdef CONFIG_ACPI
static const struct acpi_device_id mlm_test_acpi_match[] = {
	{"MXDMEEEE", 0},
	{ },
};
MODULE_DEVICE_TABLE(acpi, mlm_test_acpi_match);
#endif

static struct platform_driver ml_test_driver = {
	.driver = {
		.name = "test_mlmux",
		.of_match_table = ml_test_match_table,
		.pm = MLMUX_TEST_PM_OPS,
		.acpi_match_table = ACPI_PTR(mlm_test_acpi_match),
	},
	.probe = mlmux_test_probe,
	.remove = mlmux_test_remove,
};

module_platform_driver(ml_test_driver);

MODULE_AUTHOR("Magic Leap, Inc.");
MODULE_DESCRIPTION("ML MUX client driver for test drv");
MODULE_LICENSE("GPL v2");

