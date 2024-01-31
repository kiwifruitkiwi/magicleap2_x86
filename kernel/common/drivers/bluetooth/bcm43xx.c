// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019, Magic Leap, Inc. All rights reserved.
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
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/proc_fs.h>
#include <linux/rfkill.h>
#include <linux/timer.h>
#include <linux/uaccess.h>

#define BT_WAKE			BIT(0)
#define BT_AWAKE_INTERVAL_MS	5000

struct bcm43xx_data {
	struct device		*dev;
	struct rfkill		*rfkill_dev;
	struct gpio_desc	*gpio_reset;
	struct gpio_desc	*gpio_bt_wake;
	struct gpio_desc	*gpio_host_wake;
	int			host_wake_irq;
	struct proc_dir_entry	*proc_bluetooth;
	struct proc_dir_entry	*proc_sleep;
	struct proc_dir_entry	*proc_lpm;
	bool			is_blocked;
	unsigned long		pm_flags;
	struct timer_list	bt_awake_timer;
};

static int bcm43xx_gpio_get_value(struct gpio_desc *gpio)
{
	if (gpiod_cansleep(gpio))
		return gpiod_get_value_cansleep(gpio);
	else
		return gpiod_get_value(gpio);
}

static void bcm43xx_gpio_set_value(struct gpio_desc *gpio, int value)
{
	if (gpiod_cansleep(gpio))
		gpiod_set_value_cansleep(gpio, value);
	else
		gpiod_set_value(gpio, value);
}

static void bcm43xx_set_awake_signal(struct bcm43xx_data *bcm43xx, bool on)
{
	if (on) {
		dev_dbg(bcm43xx->dev, "Assert bt_wake gpio\n");
		bcm43xx_gpio_set_value(bcm43xx->gpio_bt_wake, 1);
		set_bit(BT_WAKE, &bcm43xx->pm_flags);
	} else {
		/* Check if BT Host Wake GPIO signal is asserted */
		if (bcm43xx_gpio_get_value(bcm43xx->gpio_host_wake)) {
			/* BT is sending data to the host, restart the timer */
			dev_dbg(bcm43xx->dev,
				"host_wake is asserted, restart timer\n");
			mod_timer(&bcm43xx->bt_awake_timer, jiffies +
				  msecs_to_jiffies(BT_AWAKE_INTERVAL_MS));
		} else {
			/* Bluetooth IC can sleep */
			dev_dbg(bcm43xx->dev, "De-assert bt_wake gpio\n");
			bcm43xx_gpio_set_value(bcm43xx->gpio_bt_wake, 0);
		}
		clear_bit(BT_WAKE, &bcm43xx->pm_flags);
	}
}

void bcm43xx_bt_awake_timer_cb(struct timer_list *t)
{
	struct bcm43xx_data *bcm43xx = from_timer(bcm43xx, t, bt_awake_timer);

	dev_dbg(bcm43xx->dev, "%s\n", __func__);
	bcm43xx_set_awake_signal(bcm43xx, false);
}

static int bcm43xx_bt_rfkill_set_power(void *data, bool blocked)
{
	struct bcm43xx_data *bcm43xx = data;

	dev_info(bcm43xx->dev, "%s: %s\n", __func__, blocked ? "off" : "on");
	if (blocked) {
		bcm43xx_gpio_set_value(bcm43xx->gpio_reset, 0);
		bcm43xx_gpio_set_value(bcm43xx->gpio_bt_wake, 0);
		clear_bit(BT_WAKE, &bcm43xx->pm_flags);
	} else {
		bcm43xx_gpio_set_value(bcm43xx->gpio_reset, 1);
		msleep(100);
	}
	bcm43xx->is_blocked = blocked;
	return 0;
}

static const struct rfkill_ops bcm43xx_bt_rfkill_ops = {
	.set_block = bcm43xx_bt_rfkill_set_power,
};

static irqreturn_t bcm43xx_hostwake_isr(int irq, void *data)
{
	return IRQ_HANDLED;
}

static ssize_t bcm43xx_lpm_read_proc(struct file *file, char __user *buf,
				     size_t size, loff_t *ppos)
{
	struct bcm43xx_data *bcm43xx = PDE_DATA(file_inode(file));
	char msg[32];

	sprintf(msg, "BT LPM Status: TX %x, RX %x\n",
		bcm43xx_gpio_get_value(bcm43xx->gpio_bt_wake),
		bcm43xx_gpio_get_value(bcm43xx->gpio_host_wake));
	return simple_read_from_buffer(buf, size, ppos, msg, strlen(msg));
}

static ssize_t bcm43xx_lpm_write_proc(struct file *file,
				      const char __user *buffer,
				      size_t count, loff_t *ppos)
{
	struct bcm43xx_data *bcm43xx = PDE_DATA(file_inode(file));
	char buf[2];
	int rc;

	if (count < 1 || count > 2)
		return -EINVAL;


	rc = copy_from_user(buf, buffer, count);
	if (rc)
		return -EFAULT;

	/* If Bluetooth is off */
	if (bcm43xx->is_blocked)
		return -EFAULT;

	if (buf[0] == '0')
		bcm43xx_set_awake_signal(bcm43xx, false);
	else if (buf[0] == '1')
		bcm43xx_set_awake_signal(bcm43xx, true);
	else
		return -EINVAL;
	return count;
}

static const struct file_operations bcm43xx_lpm_fops = {
	.read		= bcm43xx_lpm_read_proc,
	.write		= bcm43xx_lpm_write_proc,
	.llseek		= default_llseek,
};

static int bcm43xx_create_procfs(struct bcm43xx_data *bcm43xx)
{
	bcm43xx->proc_bluetooth = proc_mkdir("bluetooth", NULL);
	if (!bcm43xx->proc_bluetooth) {
		dev_err(bcm43xx->dev, "Failed to create bt procfs entry\n");
		return -ENOTDIR;
	}

	bcm43xx->proc_sleep = proc_mkdir("sleep", bcm43xx->proc_bluetooth);
	if (!bcm43xx->proc_sleep) {
		dev_err(bcm43xx->dev, "Failed to create sleep procfs entry\n");
		goto free_bt;
	}

	bcm43xx->proc_lpm = proc_create_data("lpm", 0622, bcm43xx->proc_sleep,
					     &bcm43xx_lpm_fops, bcm43xx);
	if (bcm43xx->proc_lpm == NULL) {
		dev_err(bcm43xx->dev, "Failed to create lpm procfs entry\n");
		goto free_sleep;
	}
	return 0;

free_sleep:
	remove_proc_entry("sleep", bcm43xx->proc_bluetooth);
	bcm43xx->proc_sleep = NULL;
free_bt:
	remove_proc_entry("bluetooth", NULL);
	bcm43xx->proc_bluetooth = NULL;
	return -ENOTDIR;
}

static void bcm43xx_remove_procfs(struct bcm43xx_data *bcm43xx)
{
	remove_proc_subtree("bluetooth", NULL);
	bcm43xx->proc_bluetooth = NULL;
	bcm43xx->proc_sleep = NULL;
	bcm43xx->proc_lpm = NULL;
}

#ifdef CONFIG_PM
static int bcm43xx_pm_suspend(struct device *dev)
{
	struct bcm43xx_data *bcm43xx = dev_get_drvdata(dev);

	enable_irq_wake(bcm43xx->host_wake_irq);
	return 0;
}

static int bcm43xx_pm_resume(struct device *dev)
{
	struct bcm43xx_data *bcm43xx = dev_get_drvdata(dev);

	disable_irq_wake(bcm43xx->host_wake_irq);
	return 0;
}

static const struct dev_pm_ops bcm43xx_pm_ops = {
	.suspend = bcm43xx_pm_suspend,
	.resume = bcm43xx_pm_resume,
};
#endif

static int bcm43xx_bluetooth_probe(struct platform_device *pdev)
{
	struct bcm43xx_data *bcm43xx;
	struct device *dev = &pdev->dev;
	int rc;

	dev_info(dev, "%s\n", __func__);
	bcm43xx = devm_kzalloc(dev, sizeof(*bcm43xx), GFP_KERNEL);
	if (!bcm43xx)
		return -ENOMEM;

	bcm43xx->dev = dev;
	platform_set_drvdata(pdev, bcm43xx);

	bcm43xx->gpio_reset = devm_gpiod_get(dev, "reset", GPIOD_OUT_LOW);
	if (IS_ERR(bcm43xx->gpio_reset)) {
		dev_err(dev, "Failed to obtain bt_reset gpio\n");
		return PTR_ERR(bcm43xx->gpio_reset);
	}

	bcm43xx->gpio_bt_wake = devm_gpiod_get(dev, "bt-wakeup", GPIOD_OUT_LOW);
	if (IS_ERR(bcm43xx->gpio_bt_wake)) {
		dev_err(dev, "Failed to obtain bt_wake gpio\n");
		return PTR_ERR(bcm43xx->gpio_bt_wake);
	}

	/* GPIO interrupt signal from Bluetooth IC to the host */
	bcm43xx->gpio_host_wake = devm_gpiod_get(dev, "host-wakeup", GPIOD_IN);
	if (IS_ERR(bcm43xx->gpio_host_wake)) {
		dev_err(dev, "Failed to obtain host_wake gpio\n");
		return PTR_ERR(bcm43xx->gpio_host_wake);
	}

	bcm43xx->host_wake_irq = gpiod_to_irq(bcm43xx->gpio_host_wake);
	if (bcm43xx->host_wake_irq < 0) {
		dev_err(dev, "No corresponding irq for host-wakeup gpio, %d\n",
			bcm43xx->host_wake_irq);
		return bcm43xx->host_wake_irq;
	}

	/* Register interrupt */
	rc = devm_request_threaded_irq(dev, bcm43xx->host_wake_irq,
			NULL, bcm43xx_hostwake_isr,
			IRQF_TRIGGER_RISING | IRQF_ONESHOT,
			"bt_host_wake", bcm43xx);
	if (rc) {
		dev_err(dev, "Failed to enable host_wake irq, %d\n", rc);
		return rc;
	}

	timer_setup(&bcm43xx->bt_awake_timer, bcm43xx_bt_awake_timer_cb,
		    TIMER_DEFERRABLE);

	/* Allocate Bluetooth rfkill device */
	bcm43xx->rfkill_dev = rfkill_alloc("BCM43xx Bluetooth", dev,
					   RFKILL_TYPE_BLUETOOTH,
					   &bcm43xx_bt_rfkill_ops, bcm43xx);
	if (!bcm43xx->rfkill_dev)
		return -ENOMEM;

	/* The default state of Bluetooth is off */
	bcm43xx->is_blocked = true;
	rfkill_set_states(bcm43xx->rfkill_dev, bcm43xx->is_blocked, false);

	/* Register rfkill */
	rc = rfkill_register(bcm43xx->rfkill_dev);
	if (rc) {
		dev_err(dev, "Failed to register rfkill, %d\n", rc);
		goto err_rfkill_destroy;
	}

	/* Create procfs entries */
	rc = bcm43xx_create_procfs(bcm43xx);
	if (rc)
		goto err_rfkill_dereg;

	return 0;

err_rfkill_dereg:
	rfkill_unregister(bcm43xx->rfkill_dev);

err_rfkill_destroy:
	rfkill_destroy(bcm43xx->rfkill_dev);
	bcm43xx->rfkill_dev = NULL;
	del_timer_sync(&bcm43xx->bt_awake_timer);
	return rc;
}

static int bcm43xx_bluetooth_remove(struct platform_device *pdev)
{
	struct bcm43xx_data *bcm43xx = platform_get_drvdata(pdev);

	dev_info(&pdev->dev, "%s\n", __func__);
	disable_irq(bcm43xx->host_wake_irq);
	del_timer_sync(&bcm43xx->bt_awake_timer);
	bcm43xx_remove_procfs(bcm43xx);
	rfkill_unregister(bcm43xx->rfkill_dev);
	rfkill_destroy(bcm43xx->rfkill_dev);
	return 0;
}

static void bcm43xx_bluetooth_shutdown(struct platform_device *pdev)
{
	struct bcm43xx_data *bcm43xx = platform_get_drvdata(pdev);

	dev_info(&pdev->dev, "%s\n", __func__);
	disable_irq(bcm43xx->host_wake_irq);
}

#if defined(CONFIG_ACPI)
static const struct acpi_device_id bcm43xx_bluetooth_acpi_match[] = {
	{"BCM4375", 0},
	{"", 0},
};
MODULE_DEVICE_TABLE(acpi, bcm43xx_bluetooth_acpi_match);
#endif

static struct platform_driver bcm43xx_bluetooth_platform_driver = {
	.probe = bcm43xx_bluetooth_probe,
	.remove = bcm43xx_bluetooth_remove,
	.shutdown = bcm43xx_bluetooth_shutdown,
	.driver = {
		.name = "bcm43xx_bluetooth",
		.owner = THIS_MODULE,
#if defined(CONFIG_PM)
		.pm = &bcm43xx_pm_ops,
#endif
#if defined(CONFIG_ACPI)
		.acpi_match_table = ACPI_PTR(bcm43xx_bluetooth_acpi_match),
#endif
	},
};

module_platform_driver(bcm43xx_bluetooth_platform_driver);

MODULE_ALIAS("platform:bcm43xx");
MODULE_DESCRIPTION("bcm43xx_bluetooth");
MODULE_LICENSE("GPL");
