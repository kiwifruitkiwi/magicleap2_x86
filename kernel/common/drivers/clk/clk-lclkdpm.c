// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2023 Advanced Micro Devices, Inc. All rights reserved.
 */

#include <linux/init.h>
#include <linux/err.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <linux/module.h>
#include <linux/smu_protocol.h>

struct kobject *lclkdpm_kobject;

static int smu_calculatelclkbusy(u32 value)
{
	struct smu_msg msg;

	if (value != 0 && value != 1)
		return -1;

	smu_msg(&msg, CVSMC_MSG_calculatelclkbusyfromiohconly, value);
	if (unlikely(!smu_send_single_msg(&msg))) {
		pr_err("Failed to send smu msg of CalculateLclkBusyfromIohcOnly\n");
		return -1;
	}

	return 0;
}

static int smu_updatelclkdpmconstants(u32 value)
{
	struct smu_msg msg;

	smu_msg(&msg, CVSMC_MSG_updatelclkdpmconstants, value);
	if (unlikely(!smu_send_single_msg(&msg))) {
		pr_err("Failed to send smu msg of UpdateLclkDpmConstants\n");
		return -1;
	}

	return 0;
}

static ssize_t calculatelclkbusy_store(struct kobject *kobj,
				       struct kobj_attribute *attr,
				       const char *buf, size_t n)
{
	u32 val;
	int ret;

	ret = kstrtou32(buf, 0, &val);
	if (unlikely(ret))
		return ret;

	ret = smu_calculatelclkbusy(val);
	if (ret)
		return ret;

	return n;
}

static ssize_t updatelclkdpmconstants_store(struct kobject *kobj,
					    struct kobj_attribute *attr,
					    const char *buf, size_t n)
{
	u32 val;
	u32 update, threshold;
	int ret;

	if (sscanf(buf, "%x %x", &update, &threshold) != 2)
		return -EINVAL;

	if (update != 1 && update != 2 && update != 3)
		return -EINVAL;

	val = (update << 30) | threshold;

	ret = smu_updatelclkdpmconstants(val);
	if (ret)
		return ret;

	return n;
}

static struct kobj_attribute calculatelclkbusy_attr = {
	.attr	= {
		.name = __stringify(calculatelclkbusy),
		.mode = 0200,
	},
	.store	= calculatelclkbusy_store,
};

static struct kobj_attribute updatelclkdpmconstants_attr = {
	.attr	= {
		.name = __stringify(updatelclkdpmconstants),
		.mode = 0200,
	},
	.store	= updatelclkdpmconstants_store,
};

struct attribute *lclkdpm_attrs[] = {
	&calculatelclkbusy_attr.attr,
	&updatelclkdpmconstants_attr.attr,
	NULL,
};

struct attribute_group lclkdpm_attr_group = {
	.attrs = lclkdpm_attrs,
};

static int __init lclkdpm_module_init(void)
{
	lclkdpm_kobject = kobject_create_and_add("lclkdpm", NULL);
	if (!lclkdpm_kobject) {
		pr_err("Cannot create lclkdpm kobject\n");
		goto err_kobj;
	}

	if (sysfs_create_group(lclkdpm_kobject,
			       &lclkdpm_attr_group)) {
		pr_err("Cannot create sysfs file\n");
		goto err_sysfs;
	}

	return 0;

err_sysfs:
	kobject_put(lclkdpm_kobject);
err_kobj:
	return -1;
}

static void __exit lclkdpm_module_exit(void)
{
	if (!lclkdpm_kobject)
		return;

	sysfs_remove_group(lclkdpm_kobject, &lclkdpm_attr_group);
	kobject_put(lclkdpm_kobject);
}

module_init(lclkdpm_module_init);
module_exit(lclkdpm_module_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Wenyou Yang <wenyou.yang@amd.com>");
MODULE_DESCRIPTION("Allow enabling/disabling the LCLK switching");
