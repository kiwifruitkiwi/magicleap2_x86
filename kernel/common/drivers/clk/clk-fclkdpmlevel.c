// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2022 Advanced Micro Devices, Inc. All rights reserved.
 */

#include <linux/init.h>
#include <linux/err.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <linux/module.h>
#include <linux/smu_protocol.h>

struct kobject *fclkdpmlevel_kobject;

static int smu_force_fclkdpmlevel(u32 pstate)
{
	struct smu_msg msg;

	if (pstate != 0 && pstate != 1)
		return -1;

	smu_msg(&msg, CVSMC_MSG_forcefclkdpmlevel, pstate);
	if (unlikely(!smu_send_single_msg(&msg))) {
		pr_err("Failed to send smu msg of ForceFclkDpmLevel\n");
		return -1;
	}

	return 0;
}

static int smu_unforce_fclkdpmlevel(void)
{
	struct smu_msg msg;

	smu_msg(&msg, CVSMC_MSG_unforcefclkdpmlevel, 0);
	if (unlikely(!smu_send_single_msg(&msg))) {
		pr_err("Failed to send smu msg of UnforceFclkDpmLevel\n");
		return -1;
	}

	return 0;
}

static ssize_t force_fclkdpmlevel_store(struct kobject *kobj,
					struct kobj_attribute *attr,
					const char *buf, size_t n)
{
	u32 val;
	int ret;

	ret = kstrtou32(buf, 0, &val);
	if (unlikely(ret))
		return ret;

	ret = smu_force_fclkdpmlevel(val);
	if (ret)
		return ret;

	return n;
}

static ssize_t unforce_fclkdpmlevel_store(struct kobject *kobj,
					  struct kobj_attribute *attr,
					  const char *buf, size_t n)
{
	int ret;

	ret = smu_unforce_fclkdpmlevel();
	if (ret)
		return ret;

	return n;
}

static struct kobj_attribute force_fclkdpmlevel_attr = {
	.attr	= {
		.name = __stringify(force_fclkdpmlevel),
		.mode = 0200,
	},
	.store	= force_fclkdpmlevel_store,
};

static struct kobj_attribute unforce_fclkdpmlevel_attr = {
	.attr	= {
		.name = __stringify(unforce_fclkdpmlevel),
		.mode = 0200,
	},
	.store	= unforce_fclkdpmlevel_store,
};

struct attribute *fclkdpmlevel_attrs[] = {
	&force_fclkdpmlevel_attr.attr,
	&unforce_fclkdpmlevel_attr.attr,
	NULL,
};

struct attribute_group fclkdpmlevel_attr_group = {
	.attrs = fclkdpmlevel_attrs,
};

static int __init fclkdpmlevel_module_init(void)
{
	fclkdpmlevel_kobject = kobject_create_and_add("fclkdpmlevel", NULL);
	if (!fclkdpmlevel_kobject) {
		pr_err("Cannot create fclkdpmlevel kobject\n");
		goto err_kobj;
	}

	if (sysfs_create_group(fclkdpmlevel_kobject,
			       &fclkdpmlevel_attr_group)) {
		pr_err("Cannot create sysfs file\n");
		goto err_sysfs;
	}

	return 0;

err_sysfs:
	kobject_put(fclkdpmlevel_kobject);
err_kobj:
	return -1;
}

static void __exit fclkdpmlevel_module_exit(void)
{
	if (!fclkdpmlevel_kobject)
		return;

	sysfs_remove_group(fclkdpmlevel_kobject, &fclkdpmlevel_attr_group);
	kobject_put(fclkdpmlevel_kobject);
}

module_init(fclkdpmlevel_module_init);
module_exit(fclkdpmlevel_module_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Wenyou Yang <wenyou.yang@amd.com>");
MODULE_DESCRIPTION("Force/Unfore fclkdpmlevel");
