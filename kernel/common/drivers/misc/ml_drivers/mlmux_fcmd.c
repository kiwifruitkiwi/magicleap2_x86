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

/*
 * This driver exposes basic capabilities to communicate with
 * fcmd to STM32 from userspace.
 */

#include <linux/completion.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/ioctl.h>
#include <linux/kernel.h>
#include <linux/miscdevice.h>
#include <linux/ml-mux-client.h>
#include <linux/acpi.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/uaccess.h>
#include <uapi/linux/mlmux_fcmd_ioctl.h>

#define DEVICE_NAME            "mlmux_fcmd"
#define FCMD_CHANNEL_NAME      "FCMD_CHAN"
#define RESPONSE_WAIT_MS       2000

static atomic_t s_ref_count = ATOMIC_INIT(0);

struct mlmux_fcmd_data_t {
	struct device *dev;
	struct miscdevice miscdev;
	struct ml_mux_client client;
	struct completion read_completion;
	struct mutex mutex_lock;
	struct file_data_t msg;
	bool enabled;
};

static int     cdev_open(struct inode *, struct file *);
static long    cdev_ioctl(struct file *, unsigned int, unsigned long);
static int     cdev_release(struct inode *, struct file *);

static const struct file_operations fops = {
	.owner = THIS_MODULE,
	.open = cdev_open,
	.unlocked_ioctl = cdev_ioctl,
	.release = cdev_release,
};

static void mlmux_fcmd_notify_open(struct ml_mux_client *client, bool is_open)
{
	struct mlmux_fcmd_data_t *mlmux_fcmd_data =
		container_of(client, struct mlmux_fcmd_data_t, client);

	if (is_open) {
		dev_info(mlmux_fcmd_data->dev, "mlmux_fcmd: channel open\n");
		mlmux_fcmd_data->enabled = true;
	} else {
		dev_info(mlmux_fcmd_data->dev, "mlmux_fcmd: channel closed\n");
		mlmux_fcmd_data->enabled = false;
	}
}

static void mlmux_fcmd_receive_cb(struct ml_mux_client *client, uint32_t len,
		void *msg)
{
	unsigned int length;
	struct mlmux_fcmd_data_t *mlmux_fcmd_data =
		container_of(client, struct mlmux_fcmd_data_t, client);

	if (len <= MLMUX_FCMD_BUFFER_SIZE) {
		length = len;
	} else {
		dev_warn(mlmux_fcmd_data->dev,
				"callback len %u greater than buff size %d\n",
				len, MLMUX_FCMD_BUFFER_SIZE);
		length = MLMUX_FCMD_BUFFER_SIZE;
	}

	memcpy(mlmux_fcmd_data->msg.message, msg, length);
	mlmux_fcmd_data->msg.message_size = length;
	complete(&mlmux_fcmd_data->read_completion);
}

static int mlmux_fcmd_register_cdev(struct mlmux_fcmd_data_t *mlmux_fcmd_data)
{
	int err;

	dev_info(mlmux_fcmd_data->dev, "registering mlmux_fcmd char device\n");

	mlmux_fcmd_data->miscdev.name = DEVICE_NAME;
	mlmux_fcmd_data->miscdev.fops = &fops;
	mlmux_fcmd_data->miscdev.minor = MISC_DYNAMIC_MINOR;
	err = misc_register(&mlmux_fcmd_data->miscdev);
	if (err) {
		dev_err(mlmux_fcmd_data->dev,
				"failed to register mlmux_fcmd %d\n", err);
		return err;
	}

	return 0;
}

static void mlmux_fcmd_unregister_cdev(struct mlmux_fcmd_data_t
		*mlmux_fcmd_data)
{
	misc_deregister(&mlmux_fcmd_data->miscdev);
	dev_info(mlmux_fcmd_data->dev, "mlmux_fcmd char dev unregistered\n");
}

/*
 * The device open function is called each time the device is opened.
 * Set private_data to the device data so that file operation functions
 * can access the device data structure.
 */
static int cdev_open(struct inode *inodep, struct file *filep)
{
	struct miscdevice *miscdev = filep->private_data;
	struct mlmux_fcmd_data_t *mlmux_fcmd_data;

	mlmux_fcmd_data = container_of(miscdev, struct mlmux_fcmd_data_t,
			miscdev);
	if (!mlmux_fcmd_data)
		return -ENODEV;

	if (!atomic_add_unless(&s_ref_count, 1, 1)) {
		dev_err(mlmux_fcmd_data->dev,
				"mlmux_fcmd char dev already opened\n");
		return -EBUSY;
	}

	filep->private_data = mlmux_fcmd_data;
	dev_info(mlmux_fcmd_data->dev, "mlmux_fcmd char device opened\n");
	return 0;
}

static int cdev_ioctl_write_read(struct mlmux_fcmd_data_t *mlmux_fcmd_data,
		void __user *user_data)
{
	int ret;
	int error_count = 0;
	long timeleft;

	if (!mlmux_fcmd_data->enabled)
		return -EIO;

	ret = copy_from_user(&mlmux_fcmd_data->msg, user_data,
			sizeof(struct file_data_t));

	if (ret) {
		ret = -EACCES;
		goto exit;
	}

	if (mlmux_fcmd_data->msg.message_size > MLMUX_FCMD_BUFFER_SIZE) {
		dev_err(mlmux_fcmd_data->dev,
				"Error: cmd len %u exceeds buf size %d\n",
				mlmux_fcmd_data->msg.message_size,
				MLMUX_FCMD_BUFFER_SIZE);
		ret = -EMSGSIZE;
		goto exit;
	}

	ret = ml_mux_send_msg(mlmux_fcmd_data->client.ch,
			mlmux_fcmd_data->msg.message_size,
			mlmux_fcmd_data->msg.message);

	if (ret) {
		dev_err(mlmux_fcmd_data->dev,
				"ml_mux_send_msg failed with error %d\n", ret);
		goto exit;
	}

	/* block until the response comes back from mlmux */
	reinit_completion(&mlmux_fcmd_data->read_completion);
	timeleft = wait_for_completion_interruptible_timeout(
			&mlmux_fcmd_data->read_completion,
			msecs_to_jiffies(RESPONSE_WAIT_MS));
	if (timeleft <= 0) {
		dev_err(mlmux_fcmd_data->dev,
				"mlmux_fcmd timed out waiting for resp\n");
		ret = -ETIMEDOUT;
		goto exit;
	}

	/* send message to user space */
	error_count = copy_to_user(user_data, &mlmux_fcmd_data->msg,
			sizeof(struct file_data_t));

	if (error_count != 0) {
		dev_err(mlmux_fcmd_data->dev,
				"failed to send %d bytes to user space\n",
				error_count);
		ret = -EFAULT;
	}

exit:
	return ret;
}

static long cdev_ioctl(struct file *filep, unsigned int cmd, unsigned long arg)
{
	struct mlmux_fcmd_data_t *mlmux_fcmd_data = filep->private_data;
	int ret;
	void __user *argp = (void __user *) arg;

	switch (cmd) {
	case MLMUX_FCMD_IOCTL_MSG:
		mutex_lock(&mlmux_fcmd_data->mutex_lock);
		ret = cdev_ioctl_write_read(mlmux_fcmd_data, argp);
		mutex_unlock(&mlmux_fcmd_data->mutex_lock);
		break;
	default:
		ret = -EINVAL;
	}
	return ret;
}

/*
 * The device release function that is called whenever the device is
 * closed by the userspace program
 */
static int cdev_release(struct inode *inodep, struct file *filep)
{
	struct mlmux_fcmd_data_t *mlmux_fcmd_data = filep->private_data;

	filep->private_data = NULL;
	atomic_dec(&s_ref_count);
	dev_info(mlmux_fcmd_data->dev, "mlmux_fcmd char device closed\n");
	return 0;
}

static int mlmux_fcmd_probe(struct platform_device *pdev)
{
	struct mlmux_fcmd_data_t *mlmux_fcmd_data;
	int ret = 0;

	dev_info(&pdev->dev, "mlmux_fcmd probe enter\n");

	mlmux_fcmd_data = devm_kzalloc(&pdev->dev, sizeof(*mlmux_fcmd_data),
			GFP_KERNEL);
	if (!mlmux_fcmd_data) {
		ret = -ENOMEM;
		dev_err(&pdev->dev, "unable to allocate memory\n");
		goto err2;
	}
	platform_set_drvdata(pdev, mlmux_fcmd_data);
	mlmux_fcmd_data->dev = &pdev->dev;

	init_completion(&mlmux_fcmd_data->read_completion);
	mutex_init(&mlmux_fcmd_data->mutex_lock);

	mlmux_fcmd_data->client.dev = mlmux_fcmd_data->dev;
	mlmux_fcmd_data->client.notify_open = mlmux_fcmd_notify_open;
	mlmux_fcmd_data->client.receive_cb = mlmux_fcmd_receive_cb;

	ret = ml_mux_request_channel(&mlmux_fcmd_data->client,
			FCMD_CHANNEL_NAME);
	if (ret < 0) {
		dev_err(&pdev->dev, "failed to request channel %s, error %d\n",
				FCMD_CHANNEL_NAME, ret);
		goto err1;
	} else if (ret == ML_MUX_CH_REQ_OPENED) {
		mlmux_fcmd_data->enabled = true;
	}

	ret = mlmux_fcmd_register_cdev(mlmux_fcmd_data);
	if (ret)
		goto err1;

	dev_info(&pdev->dev, "mlmux_fcmd probe success\n");
	return 0;

err1:
	mutex_destroy(&mlmux_fcmd_data->mutex_lock);
err2:
	dev_err(&pdev->dev, "mlmux_fcmd probe failed: %d\n", ret);
	return ret;
}

static int mlmux_fcmd_remove(struct platform_device *pdev)
{
	struct mlmux_fcmd_data_t *mlmux_fcmd_data;

	mlmux_fcmd_data = platform_get_drvdata(pdev);
	mlmux_fcmd_unregister_cdev(mlmux_fcmd_data);
	ml_mux_release_channel(mlmux_fcmd_data->client.ch);
	mutex_destroy(&mlmux_fcmd_data->mutex_lock);
	return 0;
}

static const struct of_device_id mlmux_fcmd_of_match[] = {
	{ .compatible = "ml,mlmux_fcmd", },
	{ },
};
MODULE_DEVICE_TABLE(of, mlmux_fcmd_of_match);

static const struct acpi_device_id mlmux_fcmd_acpi_match[] = {
	{"MXFC1111", 0},
	{ },
};
MODULE_DEVICE_TABLE(acpi, mlmux_fcmd_acpi_match);

static struct platform_driver mlmux_fcmd_driver = {
	.driver = {
		.name = "mlmux_fcmd",
		.of_match_table = mlmux_fcmd_of_match,
		.acpi_match_table = mlmux_fcmd_acpi_match,
	},
	.probe = mlmux_fcmd_probe,
	.remove = mlmux_fcmd_remove,
};

module_platform_driver(mlmux_fcmd_driver);

MODULE_AUTHOR("Magic Leap LLC");
MODULE_DESCRIPTION("Linux char driver for fcmd mlmux channel");
MODULE_LICENSE("GPL v2");
