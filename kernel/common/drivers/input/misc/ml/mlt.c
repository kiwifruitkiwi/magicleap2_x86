// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2020, Magic Leap, Inc. All rights reserved.
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

#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/miscdevice.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/sysfs.h>
#include <linux/uaccess.h>
#include <linux/vmalloc.h>
#include <uapi/linux/mlt_ioctl.h>
#include "mlt.h"

#define MLT_NAME	"mltotem"

static struct miscdevice mlt_misc;

static ssize_t name_show(struct device *dev, struct device_attribute *attr,
			 char *buf)
{
	return sprintf(buf, "%s\n", to_mlt_device(dev)->name);
}
static DEVICE_ATTR_RO(name);

static ssize_t bus_show(struct device *dev, struct device_attribute *attr,
			char *buf)
{
	return sprintf(buf, "%04X\n", to_mlt_device(dev)->bus);
}
static DEVICE_ATTR_RO(bus);

static ssize_t vendor_show(struct device *dev, struct device_attribute *attr,
			   char *buf)
{
	return sprintf(buf, "%04X\n", to_mlt_device(dev)->vendor);
}
static DEVICE_ATTR_RO(vendor);

static ssize_t pid_show(struct device *dev, struct device_attribute *attr,
			char *buf)
{
	return sprintf(buf, "%04X\n", to_mlt_device(dev)->product);
}
static DEVICE_ATTR_RO(pid);

static struct attribute *mlt_dev_attrs[] = {
	&dev_attr_name.attr,
	&dev_attr_bus.attr,
	&dev_attr_vendor.attr,
	&dev_attr_pid.attr,
	NULL
};
ATTRIBUTE_GROUPS(mlt_dev);

static int mlt_dev_uevent(struct device *dev, struct kobj_uevent_env *env)
{
	struct mlt_device *mlt_dev = to_mlt_device(dev);

	if (add_uevent_var(env, "MLT=%s:%04x:%04x:%04x", dev_name(dev),
			   mlt_dev->bus, mlt_dev->vendor, mlt_dev->product))
		return -ENOMEM;

	return 0;
}

static void mlt_dev_release(struct device *dev)
{
	struct mlt_device *mlt_dev = to_mlt_device(dev);

	mlt_dev->running = false;
}

static struct device_type mlt_type = {
	.groups		= mlt_dev_groups,
	.uevent		= mlt_dev_uevent,
	.release	= mlt_dev_release,
};

static bool mlt_match_one_id(struct mlt_device *mlt_dev,
			     const struct mlt_device_id *id)
{
	return (id->bus == MLT_BUS_ANY || id->bus == mlt_dev->bus) &&
		id->vendor == mlt_dev->vendor &&
		id->product == mlt_dev->product;
}

static const struct mlt_device_id *mlt_match_device(struct mlt_device *mlt_dev,
						const struct mlt_device_id *id)
{
	for (; id->bus; id++)
		if (mlt_match_one_id(mlt_dev, id))
			return id;

	return NULL;
}

static int mlt_device_match(struct device *dev, struct device_driver *drv)
{
	struct mlt_device *mlt_dev = to_mlt_device(dev);
	struct mlt_driver *mlt_drv = to_mlt_driver(drv);

	if (!mlt_drv->id_table)
		return 0;

	return (mlt_match_device(mlt_dev, mlt_drv->id_table) != NULL);
}

static int mlt_device_probe(struct device *dev)
{
	struct mlt_device *mlt_dev = to_mlt_device(dev);
	struct mlt_driver *driver = to_mlt_driver(dev->driver);

	return driver->probe(mlt_dev);
}

static int mlt_device_remove(struct device *dev)
{
	struct mlt_device *mlt_dev = to_mlt_device(dev);
	struct mlt_driver *driver;

	if (!dev->driver)
		return 0;

	driver = to_mlt_driver(dev->driver);
	if (driver->remove)
		return driver->remove(mlt_dev);

	return 0;
}

struct bus_type mlt_bus_type = {
	.name		= "mlt",
	.match		= mlt_device_match,
	.probe		= mlt_device_probe,
	.remove		= mlt_device_remove,
};

int __mlt_register_driver(struct mlt_driver *mlt_drv, struct module *owner,
			  const char *mod_name)
{
	mlt_drv->driver.name = mlt_drv->name;
	mlt_drv->driver.bus = &mlt_bus_type;
	mlt_drv->driver.owner = owner;
	mlt_drv->driver.mod_name = mod_name;
	return driver_register(&mlt_drv->driver);
}
EXPORT_SYMBOL_GPL(__mlt_register_driver);

void mlt_unregister_driver(struct mlt_driver *mlt_drv)
{
	driver_unregister(&mlt_drv->driver);
}
EXPORT_SYMBOL_GPL(mlt_unregister_driver);

static void mlt_unregister_device(struct mlt_device *mlt_dev)
{
	dev_info(&mlt_dev->dev, "Unregister\n");
	device_unregister(&mlt_dev->dev);
}

static int mlt_create_ioctl(struct mlt_device *mlt_dev,
			    const struct mlt_req_create __user *req)
{
	static atomic_t id = ATOMIC_INIT(0);
	struct mlt_req_create create_req;
	int rc;

	if (mlt_dev->running)
		return -EALREADY;

	rc = copy_from_user(&create_req, req, sizeof(struct mlt_req_create));
	if (rc)
		return -EFAULT;

	mlt_dev->vendor = create_req.vendor;
	mlt_dev->product = create_req.product;
	mlt_dev->version = create_req.version;
	mlt_dev->bus = create_req.bus;
	strncpy(mlt_dev->name, create_req.name, MLT_NAME_SZ);
	mlt_dev->dev.platform_data = NULL;
	mlt_dev->dev.parent = mlt_misc.this_device;
	mlt_dev->dev.bus = &mlt_bus_type;
	mlt_dev->dev.type = &mlt_type;
	dev_set_name(&mlt_dev->dev, "%s:%04X:%04X:%04X.%04X",
		     mlt_dev->name, mlt_dev->bus, mlt_dev->vendor,
		     mlt_dev->product, atomic_inc_return(&id));
	dev_info(&mlt_dev->dev, "Register device\n");
	rc = device_register(&mlt_dev->dev);
	if (rc) {
		dev_err(&mlt_dev->dev, "Register device failed, %d\n", rc);
		return rc;
	}
	mlt_dev->running = true;
	return 0;
}

static int mlt_destroy_ioctl(struct mlt_device *mlt_dev)
{
	if (!mlt_dev->running)
		return -EINVAL;

	mlt_unregister_device(mlt_dev);
	return 0;
}

static int mlt_input_evt_ioctl(struct mlt_device *mlt_dev)
{
	struct mlt_driver *driver;

	driver = to_mlt_driver(mlt_dev->dev.driver);
	if (!mlt_dev->running || !driver->input_event)
		return -EFAULT;

	return driver->input_event(mlt_dev);
}

static long mlt_char_ioctl(struct file *filep, unsigned int cmd,
			   unsigned long arg)
{
	struct mlt_device *mlt_dev = filep->private_data;
	long rc;

	switch (cmd) {
	case MLT_IOCTL_CREATE:
		rc = mlt_create_ioctl(mlt_dev, (const void __user *)arg);
		break;
	case MLT_IOCTL_DESTROY:
		rc = mlt_destroy_ioctl(mlt_dev);
		break;
	case MLT_IOCTL_INPUT:
		if (mlt_dev->running && mlt_dev->shmem)
			rc = mlt_input_evt_ioctl(mlt_dev);
		else
			rc = -EFAULT;
		break;
	default:
		rc = -EINVAL;
		break;
	}
	return rc;
}

static int mlt_char_mmap(struct file *filep, struct vm_area_struct *vma)
{
	struct mlt_device *mlt_dev = filep->private_data;
	unsigned long length = vma->vm_end - vma->vm_start;
	unsigned long pfn;
	int rc;

	if (!mlt_dev->running) {
		pr_err("%s: Device is not probed yet\n", __func__);
		return -ENODEV;
	}

	if (mlt_dev->shmem) {
		dev_err(&mlt_dev->dev, "Memory map already exists\n");
		return -EFAULT;
	}

	if (length != PAGE_SIZE) {
		dev_err(&mlt_dev->dev, "Invalid mmap len = %lu\n", length);
		return -EINVAL;
	}

	mlt_dev->shmem = vzalloc(PAGE_SIZE);
	if (!mlt_dev->shmem)
		return -ENOMEM;

	pfn = vmalloc_to_pfn(mlt_dev->shmem);
	rc = remap_pfn_range(vma, vma->vm_start, pfn, PAGE_SIZE, PAGE_SHARED);
	if (rc) {
		dev_err(&mlt_dev->dev, "remap_pfn_range failed, %d\n", rc);
		vfree(mlt_dev->shmem);
		mlt_dev->shmem = NULL;
	}
	return rc;
}

static int mlt_char_open(struct inode *inode, struct file *filep)
{
	struct mlt_device *mlt_dev;

	mlt_dev = kzalloc(sizeof(struct mlt_device), GFP_KERNEL);
	if (!mlt_dev)
		return -ENOMEM;

	filep->private_data = mlt_dev;
	nonseekable_open(inode, filep);
	return 0;
}

static int mlt_char_release(struct inode *inode, struct file *filep)
{
	struct mlt_device *mlt_dev = filep->private_data;

	if (mlt_dev->running)
		mlt_unregister_device(mlt_dev);

	vfree(mlt_dev->shmem);
	kfree(mlt_dev);
	return 0;
}

static const struct file_operations mlt_fops = {
	.owner		= THIS_MODULE,
	.unlocked_ioctl	= mlt_char_ioctl,
	.mmap		= mlt_char_mmap,
	.open		= mlt_char_open,
	.release	= mlt_char_release,
	.llseek		= no_llseek,
};

static struct miscdevice mlt_misc = {
	.minor		= MISC_DYNAMIC_MINOR,
	.name		= MLT_NAME,
	.fops		= &mlt_fops,
};

static int __init mlt_init(void)
{
	int rc;

	pr_info("%s\n", __func__);
	rc = bus_register(&mlt_bus_type);
	if (rc) {
		pr_err("Failed to register bus %s, %d\n",
			mlt_bus_type.name, rc);
		return rc;
	}

	rc = misc_register(&mlt_misc);
	if (rc) {
		pr_err("Failed to register misc device %s, %d\n",
			mlt_misc.name, rc);
		goto misc_dev_err;
	}
	return 0;

misc_dev_err:
	bus_unregister(&mlt_bus_type);
	return rc;
}

static void __exit mlt_exit(void)
{
	pr_info("%s\n", __func__);
	misc_deregister(&mlt_misc);
	bus_unregister(&mlt_bus_type);
}

subsys_initcall(mlt_init);
module_exit(mlt_exit);

MODULE_AUTHOR("Magic Leap, Inc.");
MODULE_DESCRIPTION("Magic Leap Totem Bus Driver");
MODULE_LICENSE("GPL v2");
