/* SPDX-License-Identifier: GPL-2.0 */
/*
 * DRM fornt-end of pstore:
 * APIs used by pstore and gpu driver for
 * dumping gpu status upon gpu hang
 */

#include <stddef.h>
#include <linux/module.h>
#include <drm/drm_dump.h>
#include <drm/drm_device.h>
#include <drm/drm_drv.h>
#include <linux/slab.h>
#include <linux/spinlock.h>

/**
 * struct drm_dev_list - Linked list of DRM device structures
 *
 * This structure contains all references to registered DRM
 * devices, drm_device structure pointers.
 */
struct drm_dev_list {
	struct list_head list;
	struct drm_device *dev;
};

static DEFINE_SPINLOCK(drm_dev_list_lock);
static LIST_HEAD(drm_device_list);

static struct drm_dumper drm_dumper;

void drm_dump_register(drm_dump_cb dump)
{
	drm_dumper.dump = dump;
}
EXPORT_SYMBOL_GPL(drm_dump_register);

void drm_dump_unregister(void)
{
	drm_dumper.dump = NULL;
}
EXPORT_SYMBOL_GPL(drm_dump_unregister);

void drm_dump_add_device(void *data)
{
	struct drm_dev_list *dev_list;

	dev_list = kzalloc(sizeof(*dev_list), GFP_KERNEL);
	if (dev_list) {
		unsigned long flags;

		dev_list->dev = (struct drm_device *)data;

		spin_lock_irqsave(&drm_dev_list_lock, flags);
		list_add_tail(&dev_list->list, &drm_device_list);
		spin_unlock_irqrestore(&drm_dev_list_lock, flags);
	}
}
EXPORT_SYMBOL_GPL(drm_dump_add_device);

void drm_dump_remove_device(void *data)
{
	struct drm_dev_list *dev_list;
	unsigned long flags;

	spin_lock_irqsave(&drm_dev_list_lock, flags);
	list_for_each_entry(dev_list, &drm_device_list, list) {
		if (dev_list->dev == (struct drm_device *)data) {
			list_del(&dev_list->list);
			kfree(dev_list);
			break;
		}
	}
	spin_unlock_irqrestore(&drm_dev_list_lock, flags);
}
EXPORT_SYMBOL_GPL(drm_dump_remove_device);

void drm_dump_dev(void *data, enum drm_dump_reason reason)
{
	if (drm_dumper.dump) {
		drm_dumper.data = data;
		drm_dumper.dump(&drm_dumper, reason);
	}
}
EXPORT_SYMBOL_GPL(drm_dump_dev);

void drm_dump(enum drm_dump_reason reason)
{
	struct drm_dev_list *dev_list;
	unsigned long flags;

	spin_lock_irqsave(&drm_dev_list_lock, flags);
	list_for_each_entry(dev_list, &drm_device_list, list)
		drm_dump_dev((void *)(dev_list->dev), reason);
	spin_unlock_irqrestore(&drm_dev_list_lock, flags);
}
EXPORT_SYMBOL_GPL(drm_dump);

size_t drm_dump_get_buffer(struct drm_dumper *dumper, char *buf, size_t len)
{
	size_t size = 0;
	struct drm_device *dev = (struct drm_device *)(dumper->data);

	if (dev->driver->dump_status)
		size = dev->driver->dump_status(dev, buf, len);

	return size;
}
EXPORT_SYMBOL_GPL(drm_dump_get_buffer);
