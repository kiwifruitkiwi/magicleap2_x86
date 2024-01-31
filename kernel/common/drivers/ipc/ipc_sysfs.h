/*
 * Header file for IPC library sysfs definition
 *
 * Copyright (C) 2019-2020 Advanced Micro Devices, Inc. All rights reserved.
 */

#ifndef __IPC_SYSFS_H__
#define __IPC_SYSFS_H__

#include <linux/device.h>

#define ipc_attr(_name)				\
static struct kobj_attribute _name##_attr = {	\
	.attr	= {				\
		.name = __stringify(_name),	\
		.mode = 0644,			\
	},					\
	.show	= _name##_show,			\
	.store	= _name##_store,		\
}

#define ipc_attr_ro(_name)			\
static struct kobj_attribute _name##_attr = {	\
	.attr	= {				\
		.name = __stringify(_name),	\
		.mode = 0444,			\
	},					\
	.show	= _name##_show,			\
}

#define ipc_attr_wo(_name)			\
static struct kobj_attribute _name##_attr = {	\
	.attr	= {				\
		.name = __stringify(_name),	\
		.mode = 0200,			\
	},					\
	.store	= _name##_store,		\
}

struct isp_device_attribute {
	struct device_attribute dev_attr;
	int idx;
};

#define to_isp_dev_attr(_dev_attr)					\
	container_of(_dev_attr, struct isp_device_attribute, dev_attr)

#define ISP_ATTR(_name, _mode, _show, _store, _idx)			\
	{.dev_attr = __ATTR(_name, _mode, _show, _store),		\
	.idx = _idx }

#define ISP_ATTR_RO(_name, _func, _idx)				\
	ISP_ATTR(_name, 0444, _func##_show, NULL, _index)

#define ISP_ATTR_RW(_name, _func, _idx)				\
	ISP_ATTR(_name, 0644, _func##_show, _func##_store, _index)

#define ISP_DEVICE_ATTR(_name, _mode, _show, _store, _index)		\
struct isp_device_attribute isp_attr_##_name = ISP_ATTR			\
	(_name, _mode, _show, _store, _index)

#define ISP_DEVICE_ATTR_WO(_name, _func, _idx)				\
	ISP_DEVICE_ATTR(_name, 0200, NULL, _func##_store, _idx)

#define ISP_DEVICE_ATTR_RO(_name, _func, _idx)				\
	ISP_DEVICE_ATTR(_name, 0444, _func##_show, NULL, _idx)

#define ISP_DEVICE_ATTR_RW(_name, _func, _idx)				\
	ISP_DEVICE_ATTR(_name, 0644, _func##_show, _func##_store, _idx)

#define ISP_SYSFS_RW(_name, _idx)					\
	ISP_DEVICE_ATTR_RW(_name##_idx, _name, _idx)

#define ISP_SYSFS_RO(_name, _idx)					\
	ISP_DEVICE_ATTR_RO(_name##_idx, _name, _idx)

#define ISP_SYSFS_WO(_name, _idx)					\
	ISP_DEVICE_ATTR_WO(_name##_idx, _name, _idx)

#endif /* __IPC_SYSFS_H__ */
