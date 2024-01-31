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
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/acpi.h>
#include <linux/ml-mux-client.h>
#include <linux/debugfs.h>
#include <linux/kthread.h>
#include <linux/spinlock.h>
#include <linux/cpumask.h>
#include <linux/reboot.h>
#include "ml2_ec_ctrl_mlmux.h"


#define ML2_EC_CTRL_CHAN "EC_CTRL_CHAN"
#define ML2_EC_MLMUX_SUSPEND_TO (2000)
#define ML2_EC_MLMUX_RESUME_TO (2000)

struct ml_mux_client *mClient;

static ssize_t dev_sku_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ml2_ec_data *data = dev_get_drvdata(dev);
	int len;
	const char *dev_sku_name;

	switch (data->dev_sku) {
	case ML2_EC_SKU_ENT:
		dev_sku_name = "Enterprise";
		break;
	case ML2_EC_SKU_MED:
		dev_sku_name = "Medical";
		break;
	default:
		dev_sku_name = "Unknown";
		break;
	}

	len = sprintf(buf, "%s\n", dev_sku_name);
	if (len <= 0)
		dev_err(dev, "ml2_ec: Invalid sprintf len: %d\n", len);

	return len;
}

static ssize_t display_state_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ml2_ec_data *data = dev_get_drvdata(dev);
	int len;
	const char *display_state_name;

	switch (data->display_state) {
	case ML2_EC_DISPLAY_OFF:
		display_state_name = "Display Off";
		break;
	case ML2_EC_DISPLAY_ON:
		display_state_name = "Display On";
		break;
	default:
		display_state_name = "Unknown";
		break;
	}

	len = sprintf(buf, "%s\n", display_state_name);
	if (len <= 0)
		dev_err(dev, "ml2_ec: Invalid sprintf len: %d\n", len);

	return len;
}

static ssize_t display_state_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct ml2_ec_data *data = dev_get_drvdata(dev);
	uint8_t display_state_input;
	int ret;
	struct ml2_ec_mlmux_tx_msg tx_msg;

	ret = kstrtou8(buf, 10, &display_state_input);
	if (ret < 0) {
		dev_err(dev, "Failed to convert display state value: %d\n",
			ret);
		return -EINVAL;
	}

	if (display_state_input > ML2_EC_DISPLAY_UNKNOWN) {
		dev_err(dev, "Input display state value outside of valid range: %d\n",
			display_state_input);
		return -EINVAL;
	}

	data->display_state = display_state_input;

	tx_msg.type = ML2_EC_MLMUX_TX_DISPLAY_UPDATE;
	tx_msg.display_state = data->display_state;
	ret = ml_mux_send_msg(data->client.ch, sizeof(tx_msg), &tx_msg);
	if (ret)
		dev_err(data->dev, "Unable to send display update err=%d\n",
			ret);

	return count;
}

static ssize_t init_boot_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ml2_ec_data *data = dev_get_drvdata(dev);

	return scnprintf(buf, PAGE_SIZE, "%d\n", data->init_boot);
}

static ssize_t sleep_timeout_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct ml2_ec_data *data = dev_get_drvdata(dev);
	int ret;
	int32_t sleep_timeout;
	struct ml2_ec_mlmux_tx_msg tx_msg;

	ret = kstrtoint(buf, 10, &sleep_timeout);
	if (ret < 0) {
		dev_err(dev, "Failed to convert sleep_timeout value: %d\n",
			ret);
		return -EINVAL;
	}

	tx_msg.type = ML2_EC_MLMUX_TX_SLEEP_TIMEOUT;
	tx_msg.sleep_timeout = sleep_timeout;
	ret = ml_mux_send_msg(data->client.ch, sizeof(tx_msg), &tx_msg);
	if (ret)
		dev_err(data->dev, "Unable to send sleep_timeout err=%d\n",
			ret);

	return count;
}

static ssize_t is_shutdown_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ml2_ec_data *data = dev_get_drvdata(dev);
	struct ml2_ec_mlmux_tx_msg tx_msg;
	int ret;

	reinit_completion(&data->comp);
	tx_msg.type = ML2_EC_MLMUX_TX_IS_SHUTDOWN;
	ret = ml_mux_send_msg(data->client.ch, sizeof(tx_msg), &tx_msg);
	if (ret) {
		dev_err(data->dev, "Unable to send shutdown query err=%d\n",
			ret);
		return ret;
	}

	if (!wait_for_completion_timeout(&data->comp,
				msecs_to_jiffies(ML2_EC_MLMUX_RESUME_TO))) {
		dev_err(dev, "shutdown query times out\n");
		return -ETIMEDOUT;
	}
	return scnprintf(buf, PAGE_SIZE, "%d\n", data->is_shutdown);
}

DEVICE_ATTR_RO(dev_sku);
DEVICE_ATTR_RW(display_state);
DEVICE_ATTR_RO(init_boot);
DEVICE_ATTR_WO(sleep_timeout);
DEVICE_ATTR_RO(is_shutdown);

static ssize_t ec_ver_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ml2_ec_data *data = dev_get_drvdata(dev);

	return scnprintf(buf, ML2_EC_VER_SIZE, "%s\n", data->ec_ver);
}

/* sys/devices/platform/MXEC0000/ml2_ec/ec_ver */
static DEVICE_ATTR_RO(ec_ver);

static struct attribute *ml2_ec_attrs[] = {
	&dev_attr_dev_sku.attr,
	&dev_attr_ec_ver.attr,
	&dev_attr_init_boot.attr,
	&dev_attr_sleep_timeout.attr,
	&dev_attr_is_shutdown.attr,
	NULL
};

static struct attribute_group ml2_ec_group = {
	.name = "ml2_ec",
	.attrs = ml2_ec_attrs,
};

static int lockup_thread(void *data)
{
	struct device *dev = data;
	static DEFINE_SPINLOCK(lock);
	unsigned long flags;

	dev_info(dev, "locking a CPU now\n");
	spin_lock_irqsave(&lock, flags);
	while (1)
		;

	/* unreachable */
	return 0;
}

static int debugfs_lockup_cpu(void *data, u64 val)
{
	struct device *dev = data;
	int i;

	for (i = 0; i < num_online_cpus(); i++)
		kthread_run(lockup_thread, dev, "lockup_thread");

	return 0;
}

DEFINE_DEBUGFS_ATTRIBUTE(lockup_cpu_fops, NULL, debugfs_lockup_cpu, "%llu\n");

static void ml2_ec_create_debugfs(struct ml2_ec_data *data)
{
	data->dentry = debugfs_create_dir("ml2_ec_ctrl_mlmux", NULL);
	if (IS_ERR_OR_NULL(data->dentry))
		return;
	debugfs_create_file("lockup_cpu", 0200, data->dentry, data->dev,
			&lockup_cpu_fops);
}

static int ml2_ec_mlmux_queue_msg(struct ml2_ec_data *dev_data,
				  const void *msg_data,
				  uint16_t size,
				  void (*work_func)(struct work_struct *))
{
	int ret = 0;
	struct ml2_ec_mlmux_work *ec_work = NULL;

	ec_work = devm_kzalloc(dev_data->dev, sizeof(*ec_work), GFP_KERNEL);
	if (!ec_work) {
		ret = -ENOMEM;
		dev_err(dev_data->dev, "Failed to alloc memory for ec_work\n");
		goto exit;
	}

	ec_work->msg_data = devm_kzalloc(dev_data->dev, size, GFP_KERNEL);
	if (!ec_work->msg_data) {
		devm_kfree(dev_data->dev, ec_work);
		ret = -ENOMEM;
		dev_err(dev_data->dev, "Failed to alloc memory for ec_work->msg_data\n");
		goto exit;
	}
	memcpy(ec_work->msg_data, msg_data, size);

	ec_work->dev_data = dev_data;

	INIT_WORK(&ec_work->work, work_func);
	queue_work(dev_data->work_q, &ec_work->work);
exit:
	return ret;
}

static void ml2_ec_mlmux_send_display_state(struct work_struct *work)
{
	struct ml2_ec_mlmux_work *ec_work;
	struct ml2_ec_data *data;
	struct ml2_ec_mlmux_tx_msg tx_msg;
	int ret;

	ec_work = container_of(work, struct ml2_ec_mlmux_work, work);
	data = ec_work->dev_data;

	tx_msg.type = ML2_EC_MLMUX_TX_DISPLAY_UPDATE;
	tx_msg.display_state = data->display_state;

	ret = ml_mux_send_msg(data->client.ch, sizeof(tx_msg), &tx_msg);
	if (ret)
		dev_err(data->dev, "Send err=%d\n", ret);

	devm_kfree(data->dev, ec_work->msg_data);
	devm_kfree(data->dev, ec_work);
}

static void ml2_ec_mlmux_open(struct ml_mux_client *cli, bool is_open)
{
	struct ml2_ec_data *data = container_of(cli, struct ml2_ec_data,
		client);

	if (is_open)
		ml2_ec_mlmux_queue_msg(data, NULL, 0,
			ml2_ec_mlmux_send_display_state);
}

static void ml2_ec_mlmux_send_ping_ack(struct work_struct *work)
{
	struct ml2_ec_mlmux_work *ec_work;
	struct ml2_ec_data *data;
	struct ml2_ec_mlmux_tx_msg tx_msg;
	int ret;

	ec_work = container_of(work, struct ml2_ec_mlmux_work, work);
	data = ec_work->dev_data;

	tx_msg.type = ML2_EC_MLMUX_TX_PING_ACK;

	ret = ml_mux_send_msg(data->client.ch, sizeof(tx_msg), &tx_msg);
	if (ret)
		dev_err(data->dev, "Send err=%d\n", ret);

	devm_kfree(data->dev, ec_work->msg_data);
	devm_kfree(data->dev, ec_work);
}

static void auto_forced_shutdown_func(struct work_struct *work)
{
	WARN(1, "auto_forced_shutdown start");
	kernel_power_off();
}

static DECLARE_DELAYED_WORK(auto_forced_shutdown_work,
			    auto_forced_shutdown_func);

static void ml2_ec_mlmux_recv(struct ml_mux_client *cli, uint32_t len,
			      void *pkt)
{
	struct ml2_ec_data *data =
		container_of(cli, struct ml2_ec_data, client);
	struct ml2_ec_mlmux_rx_msg *rx_msg = pkt;

	if (!rx_msg || !len) {
		dev_err(data->dev, "Discarding invalid MLMUX message\n");
		return;
	}

	dev_dbg(data->dev, "rx_msg->type=%d\n", rx_msg->type);

	switch (rx_msg->type) {
	case ML2_EC_MLMUX_RX_PING:
		dev_dbg(data->dev, "ping_seq=%d\n", rx_msg->data);
		ml2_ec_mlmux_queue_msg(data, NULL, 0,
				       ml2_ec_mlmux_send_ping_ack);
		break;
	case ML2_EC_MLMUX_RX_EC_VER:
		strlcpy(data->ec_ver, rx_msg->ec_ver, ML2_EC_VER_SIZE);
		break;
	case ML2_EC_MLMUX_RX_DEV_SKU:
		data->dev_sku = rx_msg->data;
		break;
	case ML2_EC_MLMUX_RX_QUIET_ACK:
	case ML2_EC_MLMUX_RX_RESUME_ACK:
		complete(&data->comp);
		break;
	case ML2_EC_MLMUX_RX_EC_BOOT:
		data->init_boot = rx_msg->data;
		break;
	case ML2_EC_MLMUX_RX_SHUTDOWN:
		dev_info(data->dev, "Auto shutdown requested");
		schedule_delayed_work(&auto_forced_shutdown_work,
				      msecs_to_jiffies(10000));
		orderly_poweroff(true);
		break;
	case ML2_EC_MLMUX_RX_IS_SHUTDOWN_ACK:
		dev_dbg(data->dev, "shutdown=%d", rx_msg->data);
		data->is_shutdown = rx_msg->data;
		complete(&data->comp);
		break;
	default:
		dev_err(data->dev, "Unknown msg type: %d\n", rx_msg->type);
		break;
	}
}

static int ml2_ec_mlmux_reboot_notifier_call(struct notifier_block *notifier,
					     unsigned long event, void *unused)
{
	struct ml2_ec_mlmux_tx_msg tx_msg;

	if (event == SYS_POWER_OFF) {
		tx_msg.type = ML2_EC_MLMUX_TX_SHUTDOWN;
		ml_mux_send_msg(mClient->ch, sizeof(tx_msg), &tx_msg);
	}

	return NOTIFY_DONE;
}

static struct notifier_block ml2_ec_reboot_notifier = {
	.notifier_call = ml2_ec_mlmux_reboot_notifier_call,
};

static int ml2_ec_probe(struct platform_device *pdev)
{
	int ret;
	struct ml2_ec_data *data;

	data = devm_kzalloc(&pdev->dev, sizeof(*data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	data->dev = &pdev->dev;
	data->client.dev = &pdev->dev;
	data->client.notify_open = ml2_ec_mlmux_open;
	data->client.receive_cb = ml2_ec_mlmux_recv;
	data->display_state = ML2_EC_DISPLAY_ON;
	mClient = &data->client;

	platform_set_drvdata(pdev, data);

	init_completion(&data->comp);

	data->work_q = alloc_workqueue(ML2_EC_CTRL_CHAN, WQ_UNBOUND, 1);
	if (!data->work_q) {
		dev_info(data->dev, "Failed to create workqueue\n");
		ret = -ENOMEM;
		goto exit_probe;
	}

	ret = register_reboot_notifier(&ml2_ec_reboot_notifier);
	if (ret) {
		dev_err(&pdev->dev, "unable to register reboot notifier\n");
		goto exit_remove_workq;
	}

	ret = ml_mux_request_channel(&data->client, ML2_EC_CTRL_CHAN);
	if (ret < 0) {
		dev_err(data->dev, "MLMUX failed: %d\n", ret);
		goto exit_remove_workq;
	}

	ret = sysfs_create_group(&pdev->dev.kobj, &ml2_ec_group);
	if (ret) {
		dev_err(&pdev->dev, "ec group sysfs creation failed\n");
		goto exit_remove_workq;
	}

	ret = sysfs_create_file(&pdev->dev.kobj, &dev_attr_display_state.attr);
	if (ret < 0) {
		dev_err(&pdev->dev, "display state sysfs creation failed\n",
			ret);
		goto exit_remove_group;
	}

	ml2_ec_create_debugfs(data);

	return 0;

exit_remove_group:
	sysfs_remove_group(&pdev->dev.kobj, &ml2_ec_group);
exit_remove_workq:
	destroy_workqueue(data->work_q);
exit_probe:
	return ret;
}

static int ml2_ec_remove(struct platform_device *pdev)
{
	struct ml2_ec_data *data = platform_get_drvdata(pdev);

	ml_mux_release_channel(data->client.ch);
	sysfs_remove_file(&pdev->dev.kobj, &dev_attr_sleep_timeout.attr);
	sysfs_remove_file(&pdev->dev.kobj, &dev_attr_display_state.attr);
	sysfs_remove_file(&pdev->dev.kobj, &dev_attr_is_shutdown.attr);
	sysfs_remove_group(&pdev->dev.kobj, &ml2_ec_group);
	destroy_workqueue(data->work_q);
	debugfs_remove_recursive(data->dentry);

	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int ml2_ec_suspend(struct device *dev)
{
	struct ml2_ec_data *data = dev_get_drvdata(dev);
	struct ml2_ec_mlmux_tx_msg msg;
	int ret;

	reinit_completion(&data->comp);
	msg.type = ML2_EC_MLMUX_TX_QUIET;

	ret = ml_mux_send_msg(data->client.ch, sizeof(msg), &msg);
	if (ret)
		return ret;

	if (!wait_for_completion_timeout(&data->comp,
				msecs_to_jiffies(ML2_EC_MLMUX_SUSPEND_TO))) {
		dev_err(dev, "suspend times out on notifying EC\n");
		return -ETIMEDOUT;
	}

	dev_dbg(dev, "suspend\n");
	return 0;
}

static int ml2_ec_resume(struct device *dev)
{
	struct ml2_ec_data *data = dev_get_drvdata(dev);
	struct ml2_ec_mlmux_tx_msg msg;
	int ret;

	reinit_completion(&data->comp);
	msg.type = ML2_EC_MLMUX_TX_RESUME;

	ret = ml_mux_send_msg(data->client.ch, sizeof(msg), &msg);
	if (ret)
		return ret;

	if (!wait_for_completion_timeout(&data->comp,
				msecs_to_jiffies(ML2_EC_MLMUX_RESUME_TO))) {
		dev_err(dev, "resume times out on notifying EC\n");
		return -ETIMEDOUT;
	}

	dev_dbg(dev, "resume\n");
	return 0;
}
#endif

#ifdef CONFIG_ACPI
static const struct acpi_device_id ml2_ec_mlmux_acpi_match[] = {
	{ "MXEC0000", 0 },
	{ },
};
MODULE_DEVICE_TABLE(acpi, ml2_ec_mlmux_acpi_match);
#endif

#ifdef CONFIG_PM_SLEEP
static const struct dev_pm_ops ml2_ec_pm = {
	.suspend = ml2_ec_suspend,
	.resume = ml2_ec_resume,
};
#endif

static struct platform_driver ml2_ec_driver = {
	.probe = ml2_ec_probe,
	.remove = ml2_ec_remove,
	.driver = {
		.name = "ec-ctrl-mlmux",
#ifdef CONFIG_PM_SLEEP
		.pm = &ml2_ec_pm,
#endif
		.acpi_match_table = ACPI_PTR(ml2_ec_mlmux_acpi_match),
	}
};
module_platform_driver(ml2_ec_driver);


MODULE_AUTHOR("Magic Leap, Inc.");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("ML2 EC Ctrl driver for Magic Leap MUX");
