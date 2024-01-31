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

#include <linux/debugfs.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/ml_sec/mlsec_debugfs.h>
#include <linux/ml_sec/jail_debugfs.h>

static struct dentry *g_jail_file;
static bool g_jail_is_permissive;

static void __exit jail_debugfs_exit(void)
{
	debugfs_remove(g_jail_file);
}

static int __init jail_debugfs_init(void)
{

	g_jail_file = debugfs_create_bool("jail_permissive",
					  0644,
					  g_mlsec_debugfs_dir,
					  &g_jail_is_permissive);
	if (g_jail_file == NULL)
		return -ENOMEM;

	return 0;
}

bool jail_is_permissive(void)
{
	return g_jail_is_permissive;
}
EXPORT_SYMBOL(jail_is_permissive);

module_init(jail_debugfs_init);
module_exit(jail_debugfs_exit);
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Magic Leap, Inc.");
MODULE_DESCRIPTION("Creates jail_permissive file in ml_sec debugfs to allow disabling jail.");
