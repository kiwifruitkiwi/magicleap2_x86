// SPDX-License-Identifier: GPL-2.0-only
/* Copyright (c) 202x, Magic Leap, Inc. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 */

#include <linux/acpi.h>
#include <linux/cpu.h>
#include <linux/cpumask.h>
#include <linux/gpio.h>
#include <linux/gpio/consumer.h>
#include <linux/idr.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/kfifo.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/pm_qos.h>
#include <linux/poll.h>
#include <linux/timekeeping.h>
#include <linux/wait.h>
#include <linux/cvip/gsm_jrtc.h>
#include <uapi/linux/ml_tsf.h>
#include <linux/debugfs.h>

#define TSF_PM_QOS_HIGH_PRECISION 17
#define TSF_DEV_NAME_LEN 32
static DEFINE_IDA(dev_nrs);

struct ml_tsf {
	struct device *dev;
	int dev_id;
	struct miscdevice mdev;
	atomic_t ref_count;
	struct gpio_desc *gpio;
	int irq;
	struct cpumask qos_mask;
	struct dev_pm_qos_request pm_qos_req[NR_CPUS];
	DECLARE_KFIFO(events, struct ml_tsf_event, 4);
	wait_queue_head_t wait;
	struct mutex lock;
	u64 ts_monotonic;
	u64 ts_gsm;

	/* the following fields are for performance benchmark only */
	struct dentry *dir;
	u32 gpio_toggle;
	u32 gpio_number;
	u32 gpio_state;
};

static irqreturn_t ml_tsf_irq_threaded(int irq, void *data)
{
	struct ml_tsf *ts = data;
	struct ml_tsf_event evt;

	memset(&evt, 0, sizeof(evt));
	evt.ts_monotonic = ts->ts_monotonic;
	evt.ts_gsm = ts->ts_gsm;
	if (kfifo_put(&ts->events, evt))
		wake_up_poll(&ts->wait, EPOLLIN);

	/* for measurement by scope */
	if (ts->gpio_toggle) {
		ts->gpio_state = ts->gpio_state == 0 ? 1 : 0;
		gpio_set_value(ts->gpio_number, ts->gpio_state);
	}

	return IRQ_HANDLED;
}

static irqreturn_t ml_tsf_irq(int irq, void *data)
{
	struct ml_tsf *ts = data;

	ts->ts_monotonic = ktime_get_ns();
	ts->ts_gsm = gsm_jrtc_get_time_ns();
	return IRQ_WAKE_THREAD;
}

static int ml_tsf_pm_qos_add_request(struct ml_tsf *ts)
{
	struct device *cpu_dev;
	int cpu, rc;

	for_each_cpu(cpu, &ts->qos_mask) {
		cpu_dev = get_cpu_device(cpu);
		if (!cpu_dev) {
			dev_err(ts->dev,
				"Failed to get cpu %d, pq_qos mask %*pb\n",
				cpu, cpumask_pr_args(&ts->qos_mask));
			return -EINVAL;
		}

		rc = dev_pm_qos_add_request(cpu_dev, &ts->pm_qos_req[cpu],
					    DEV_PM_QOS_RESUME_LATENCY,
					    PM_QOS_CPU_DMA_LAT_DEFAULT_VALUE);
		if (rc < 0) {
			dev_err(ts->dev,
				"pq_qos mask %*pb failed for cpu %d, %d\n",
				cpumask_pr_args(&ts->qos_mask), cpu, rc);
			return rc;
		}
	}
	return 0;
}

static void ml_tsf_pm_qos_remove_request(struct ml_tsf *ts)
{
	struct device *cpu_dev;
	int cpu;

	for_each_cpu(cpu, &ts->qos_mask) {
		cpu_dev = get_cpu_device(cpu);
		if (cpu_dev)
			dev_pm_qos_remove_request(&ts->pm_qos_req[cpu]);
	}
}

static int ml_tsf_pm_qos_update_request(struct ml_tsf *ts, s32 new_value)
{
	int cpu, rc;

	for_each_cpu(cpu, &ts->qos_mask) {
		rc = dev_pm_qos_update_request(&ts->pm_qos_req[cpu], new_value);
		if (rc < 0) {
			dev_err(ts->dev,
				"pq_qos_update for cpu %d failed, %d\n",
				cpu, rc);
			return rc;
		}
	}
	return 0;
}

static int ml_tsf_cdev_open(struct inode *inode, struct file *filp)
{
	struct ml_tsf *ts = container_of(filp->private_data,
					 struct ml_tsf, mdev);

	dev_dbg(ts->dev, "%s\n", __func__);
	/* To avoid race condition do not allow more than 1 consumer */
	if (!atomic_add_unless(&ts->ref_count, 1, 1)) {
		dev_err(ts->dev, "char device is busy\n");
		return -EBUSY;
	}
	get_device(ts->dev);
	kfifo_reset(&ts->events);
	ml_tsf_pm_qos_update_request(ts, TSF_PM_QOS_HIGH_PRECISION);
	enable_irq(ts->irq);
	return nonseekable_open(inode, filp);
}

static int ml_tsf_cdev_release(struct inode *inode, struct file *filp)
{
	struct ml_tsf *ts = container_of(filp->private_data,
					 struct ml_tsf, mdev);

	dev_dbg(ts->dev, "%s\n", __func__);
	disable_irq(ts->irq);
	put_device(ts->dev);
	ml_tsf_pm_qos_update_request(ts, PM_QOS_CPU_DMA_LAT_DEFAULT_VALUE);
	atomic_dec(&ts->ref_count);
	return 0;
}

static __poll_t ml_tsf_cdev_poll(struct file *filp, poll_table *wait)
{
	struct ml_tsf *ts = container_of(filp->private_data,
					 struct ml_tsf, mdev);
	__poll_t mask = 0;

	poll_wait(filp, &ts->wait, wait);
	if (!kfifo_is_empty(&ts->events))
		mask = EPOLLIN | EPOLLRDNORM;

	return mask;
}

static ssize_t ml_tsf_cdev_read(struct file *filp, char __user *buf,
				 size_t count, loff_t *ppos)
{
	struct ml_tsf *ts = container_of(filp->private_data,
					  struct ml_tsf, mdev);
	unsigned int copied;
	int rc;

	if (count < sizeof(struct ml_tsf_event))
		return -EINVAL;

	do {
		if (kfifo_is_empty(&ts->events)) {
			if (filp->f_flags & O_NONBLOCK)
				return -EAGAIN;

			rc = wait_event_interruptible(ts->wait,
					!kfifo_is_empty(&ts->events));
			if (rc)
				return rc;
		}

		if (mutex_lock_interruptible(&ts->lock))
			return -ERESTARTSYS;

		rc = kfifo_to_user(&ts->events, buf, count, &copied);
		mutex_unlock(&ts->lock);
		if (rc)
			return rc;

		if (copied == 0 && (filp->f_flags & O_NONBLOCK))
			return -EAGAIN;
	} while (copied == 0);

	return copied;
}

static const struct file_operations ml_tsf_fops = {
	.owner		= THIS_MODULE,
	.open		= ml_tsf_cdev_open,
	.release	= ml_tsf_cdev_release,
	.poll		= ml_tsf_cdev_poll,
	.read		= ml_tsf_cdev_read,
	.llseek		= no_llseek,
};

static int ml_tsf_probe(struct platform_device *pdev)
{
	struct ml_tsf *ts = NULL;
	struct device *dev = &pdev->dev;
	char name[TSF_DEV_NAME_LEN];
	const char *cpu_aff;
	int rc;

	dev_info(dev, "%s\n", __func__);
	ts = devm_kzalloc(dev, sizeof(*ts), GFP_KERNEL);
	if (!ts)
		return -ENOMEM;

	ts->dev = dev;
	platform_set_drvdata(pdev, ts);

	ts->gpio = devm_gpiod_get(dev, "ts", GPIOD_IN);
	if (IS_ERR(ts->gpio)) {
		rc = PTR_ERR(ts->gpio);
		dev_err(dev, "failed to obtain ts gpio, %d\n", rc);
		return PTR_ERR(ts->gpio);
	}

	ts->irq = gpiod_to_irq(ts->gpio);
	if (ts->irq < 0) {
		dev_err(dev, "no corresponding irq for ts gpio, %d\n", ts->irq);
		return ts->irq;
	}


	rc = device_property_read_string(dev, "cpu-affinity", &cpu_aff);
	if (rc) {
		dev_err(dev, "failed to read cpu affinity, %d\n", rc);
		return rc;
	}

	rc = cpumask_parse(cpu_aff, &ts->qos_mask);
	if (rc) {
		dev_err(dev, "failed to parse affinity %s, %d\n", cpu_aff, rc);
		return rc;
	}

	INIT_KFIFO(ts->events);
	init_waitqueue_head(&ts->wait);
	mutex_init(&ts->lock);

	rc = ml_tsf_pm_qos_add_request(ts);
	if (rc < 0)
		goto err_mutex;

	memset(name, 0, sizeof(name));
	rc = ida_alloc(&dev_nrs, GFP_KERNEL);
	if (rc < 0)
		goto err_pm_qos;

	ts->dev_id = rc;
	snprintf(name, TSF_DEV_NAME_LEN, "tsf%d", ts->dev_id);

	/* misc device */
	ts->mdev.minor = MISC_DYNAMIC_MINOR;
	ts->mdev.fops = &ml_tsf_fops;
	ts->mdev.parent = dev;
	ts->mdev.name = name;
	rc = misc_register(&ts->mdev);
	if (rc) {
		dev_err(dev, "failed to register misc device %s, %d\n",
			name, rc);
		goto err_ida;
	}

	rc = devm_request_threaded_irq(dev, ts->irq, ml_tsf_irq,
				       ml_tsf_irq_threaded,
				       IRQF_TRIGGER_RISING | IRQF_ONESHOT,
				       "ts_irq", ts);
	if (rc) {
		dev_err(&pdev->dev, "failed to request irq, %d\n", rc);
		goto err_misc;
	}
	disable_irq(ts->irq);

	ts->dir = debugfs_create_dir("ml_tsf", 0);
	if (ts->dir) {
		ts->gpio_number = 324;
		debugfs_create_u32("gpio_number", 0660, ts->dir,
							&ts->gpio_number);
		debugfs_create_u32("gpio_toggle", 0660, ts->dir,
							&ts->gpio_toggle);
	} else
		dev_warn(&pdev->dev, "failed to register debugfs");

	return 0;

err_misc:
	misc_deregister(&ts->mdev);
err_ida:
	ida_free(&dev_nrs, ts->dev_id);
err_pm_qos:
	ml_tsf_pm_qos_remove_request(ts);
err_mutex:
	mutex_destroy(&ts->lock);
	return rc;
}

static int ml_tsf_remove(struct platform_device *pdev)
{
	struct ml_tsf *ts = platform_get_drvdata(pdev);

	dev_info(&pdev->dev, "%s\n", __func__);
	misc_deregister(&ts->mdev);
	ida_free(&dev_nrs, ts->dev_id);
	ml_tsf_pm_qos_remove_request(ts);
	debugfs_remove_recursive(ts->dir);
	mutex_destroy(&ts->lock);
	return 0;
}

#ifdef CONFIG_ACPI
static const struct acpi_device_id ml_tsf_acpi_match[] = {
	{ "TSF0001", 0 },
	{ },
};
MODULE_DEVICE_TABLE(acpi, ml_tsf_acpi_match);
#endif

static struct platform_driver ml_tsf_driver = {
	.probe = ml_tsf_probe,
	.remove = ml_tsf_remove,
	.driver = {
		.name = "gpio-ts",
		.owner = THIS_MODULE,
		.acpi_match_table = ACPI_PTR(ml_tsf_acpi_match),
	},
};

module_platform_driver(ml_tsf_driver);

MODULE_AUTHOR("Andrey Gostev <agostev@magicleap.com>");
MODULE_DESCRIPTION("MagicLeap time synchronization driver");
MODULE_LICENSE("GPL v2");
