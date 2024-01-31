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

#ifndef __MLT_H__
#define __MLT_H__

#include <linux/device.h>

#define MAGICLEAP_VENDOR_ID	0x2c55
#define MLT_NAME_SZ		64

struct mlt_device_id {
	__u32	vendor;
	__u32	product;
	__u16	bus;
};

#define MLT_DEVICE_ID(ven, prod, bus_id)				\
	.vendor = (ven), .product = (prod), .bus = (bus_id)

struct mlt_device {
	struct device	dev;
	char		name[MLT_NAME_SZ];
	__u32		vendor;
	__u32		product;
	__u32		version;
	__u16		bus;
	void		*shmem;
	bool		running;
};
#define to_mlt_device(d) container_of(d, struct mlt_device, dev)

struct mlt_driver {
	char *name;
	const struct mlt_device_id *id_table;

	int (*probe)(struct mlt_device *mlt_dev);
	int (*remove)(struct mlt_device *mlt_dev);
	int (*input_event)(struct mlt_device *mlt_dev);

	struct device_driver driver;
};
#define to_mlt_driver(d) container_of(d, struct mlt_driver, driver)

static inline void *mlt_get_drvdata(const struct mlt_device *mlt_dev)
{
	return dev_get_drvdata(&mlt_dev->dev);
}

static inline void mlt_set_drvdata(struct mlt_device *mlt_dev, void *data)
{
	dev_set_drvdata(&mlt_dev->dev, data);
}

extern int __must_check __mlt_register_driver(struct mlt_driver *,
		struct module *, const char *mod_name);
extern void mlt_unregister_driver(struct mlt_driver *mlt_drv);

#define mlt_register_driver(driver) \
	__mlt_register_driver(driver, THIS_MODULE, KBUILD_MODNAME)

#define module_mlt_driver(__mlt_driver) \
	module_driver(__mlt_driver, mlt_register_driver, \
		      mlt_unregister_driver)

#endif /* __MLT_H__ */

