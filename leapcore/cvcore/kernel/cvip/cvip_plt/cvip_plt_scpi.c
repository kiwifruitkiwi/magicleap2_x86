// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2010 ARM Ltd.
 * Modification Copyright (C) 2021, Advanced Micro Devices, Inc.
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/compiler.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/err.h>
#include <linux/device.h>
#include <linux/scpi_protocol.h>
#include <linux/of_address.h>
#include <linux/of_platform.h>
#include <linux/clk.h>
#include <linux/sysfs.h>
#include <mero_scpi.h>

static const char * const scpi_clock_on = "on";
static const char * const scpi_clock_off = "off";

struct dentry *firmware_debugfs_root, *check;

static void mero_scpi_set_clk(struct device_node *np, char *name, u32 value)
{
	struct clk *clk = NULL;
	int ret;

	clk = of_clk_get_by_name(np, name);
	if (IS_ERR(clk)) {
		pr_err("Failed to found clock node %s\n", name);
		return;
	}

	ret = clk_set_rate(clk, value);
	if (ret)
		pr_err("Fail setting clk[%s], err=%d\n", name, ret);
}

static ssize_t coresightclk_store(struct device *dev,
		struct device_attribute *attr, const char *ptr, size_t len)
{
	unsigned long inputVal;
	struct scpi_drvinfo *scpi_info;
	char *p;
	int slen;

	scpi_info = get_scpi_drvinfo();
	if (!scpi_info)
		return -ENODATA;

	p = memchr(ptr, '\n', len);
	slen = p ? p - ptr : len;

	if (slen == strlen(scpi_clock_on) &&
	    !strncmp(ptr, scpi_clock_on, slen)) {
		inputVal = SCPI_1MHZ;
	} else {
		if (slen == strlen(scpi_clock_off) &&
		    !strncmp(ptr, scpi_clock_off, slen))
			inputVal = 0;
		else
			return len;
	}
	mero_scpi_set_clk(scpi_info->np, "coresightclk", inputVal);

	return len;
}

static ssize_t coresightclk_show(struct device *dev,
		struct device_attribute *attr, char *ptr)
{
	unsigned long inputVal;
	struct scpi_drvinfo *scpi_info;

	scpi_info = get_scpi_drvinfo();
	if (!scpi_info)
		return -ENODATA;

	inputVal = scpi_info->scpi_ops->clk_get_val(CORESIGHTCLK) * SCPI_1MHZ;

	return scnprintf(ptr, PAGE_SIZE,
			"Coresight clock rate: %lu\n", inputVal);
}

static ssize_t c5clk_store(struct device *dev,
		struct device_attribute *attr, const char *ptr, size_t len)
{
	unsigned long inputVal;
	struct scpi_drvinfo *scpi_info;
	int ret;

	scpi_info = get_scpi_drvinfo();
	if (!scpi_info)
		return -ENODATA;

	ret = kstrtoul(ptr, 0, &inputVal);
	if (ret)
		return ret;

	mero_scpi_set_clk(scpi_info->np, "c5clk", inputVal);

	return len;
}

static ssize_t c5clk_show(struct device *dev,
		struct device_attribute *attr, char *ptr)
{
	unsigned long inputVal;
	struct scpi_drvinfo *scpi_info;

	scpi_info = get_scpi_drvinfo();
	if (!scpi_info)
		return -ENODATA;

	inputVal = scpi_info->scpi_ops->clk_get_val(C5CLK) * SCPI_1MHZ;

	return scnprintf(ptr, PAGE_SIZE, "C5 clock rate: %lu\n", inputVal);
}

static ssize_t q6clk_store(struct device *dev,
		struct device_attribute *attr, const char *ptr, size_t len)
{
	unsigned long inputVal;
	struct scpi_drvinfo *scpi_info;
	int ret;

	scpi_info = get_scpi_drvinfo();
	if (!scpi_info)
		return -ENODATA;

	ret = kstrtoul(ptr, 0, &inputVal);
	if (ret)
		return ret;

	mero_scpi_set_clk(scpi_info->np, "q6clk", inputVal);

	return len;
}

static ssize_t q6clk_show(struct device *dev,
		struct device_attribute *attr, char *ptr)
{
	unsigned long inputVal;
	struct scpi_drvinfo *scpi_info;

	scpi_info = get_scpi_drvinfo();
	if (!scpi_info)
		return -ENODATA;

	inputVal = scpi_info->scpi_ops->clk_get_val(Q6CLK) * SCPI_1MHZ;

	return scnprintf(ptr, PAGE_SIZE, "Q6 clock rate: %lu\n", inputVal);
}

static ssize_t nic400clk_store(struct device *dev,
		struct device_attribute *attr, const char *ptr, size_t len)
{
	unsigned long inputVal;
	struct scpi_drvinfo *scpi_info;
	int ret;

	scpi_info = get_scpi_drvinfo();
	if (!scpi_info)
		return -ENODATA;

	ret = kstrtoul(ptr, 0, &inputVal);
	if (ret)
		return ret;

	mero_scpi_set_clk(scpi_info->np, "nicclk", inputVal);

	return len;
}

static ssize_t nic400clk_show(struct device *dev,
		struct device_attribute *attr, char *ptr)
{
	unsigned long inputVal;
	struct scpi_drvinfo *scpi_info;

	scpi_info = get_scpi_drvinfo();
	if (!scpi_info)
		return -ENODATA;

	inputVal = scpi_info->scpi_ops->clk_get_val(NIC400CLK) * SCPI_1MHZ;

	return scnprintf(ptr, PAGE_SIZE, "Nic400 clock rate: %lu\n", inputVal);
}

static ssize_t decompressclk_store(struct device *dev,
		struct device_attribute *attr, const char *ptr, size_t len)
{
	int err;
	unsigned long inputVal;
	struct scpi_drvinfo *scpi_info;

	scpi_info = get_scpi_drvinfo();
	if (!scpi_info)
		return -ENODATA;

	err = kstrtoul(ptr, 10, &inputVal);
	if (err) {
		pr_err("Fail to obtain input value\n");
		return len;
	}

	mero_scpi_set_clk(scpi_info->np, "decompressclk", inputVal);

	return len;
}

static ssize_t decompressclk_show(struct device *dev,
		struct device_attribute *attr, char *ptr)
{
	unsigned long inputVal;
	struct scpi_drvinfo *scpi_info;

	scpi_info = get_scpi_drvinfo();
	if (!scpi_info)
		return -ENODATA;

	inputVal = scpi_info->scpi_ops->clk_get_val(DECOMPRESSCLK) * SCPI_1MHZ;

	return scnprintf(ptr, PAGE_SIZE,
			"Decompress clock rate: %lu\n", inputVal);
}

static ssize_t a55dsuclk_store(struct device *dev,
		struct device_attribute *attr, const char *ptr, size_t len)
{
	int err;
	unsigned long inputVal;
	struct scpi_drvinfo *scpi_info;

	scpi_info = get_scpi_drvinfo();
	if (!scpi_info)
		return -ENODATA;

	err = kstrtoul(ptr, 10, &inputVal);
	if (err)
		return err;

	mero_scpi_set_clk(scpi_info->np, "a55dsuclk", inputVal);

	return len;
}

static ssize_t a55dsuclk_show(struct device *dev,
		struct device_attribute *attr, char *ptr)
{
	unsigned long inputVal;
	struct scpi_drvinfo *scpi_info;

	scpi_info = get_scpi_drvinfo();
	if (!scpi_info)
		return -ENODATA;

	inputVal = scpi_info->scpi_ops->clk_get_val(A55DSUCLK) * SCPI_1MHZ;

	return scnprintf(ptr, PAGE_SIZE, "A55DSU clock rate: %lu\n", inputVal);
}

static ssize_t smuready_store(struct device *dev,
		struct device_attribute *attr, const char *ptr, size_t len)
{
	int ret;
	unsigned long inputval;

	ret = kstrtoul(ptr, 10, &inputval);
	if (ret)
		pr_err("Fail to obtain input value\n");
	else
		set_smu_dpm_ready(inputval);

	return len;
}

static ssize_t smuready_show(struct device *dev,
		struct device_attribute *attr, char *ptr)
{
	int dpm_ready;

	dpm_ready = is_smu_dpm_ready();

	return scnprintf(ptr, PAGE_SIZE, "%u\n", dpm_ready);
}

static ssize_t a55twoclk_store(struct device *dev,
		struct device_attribute *attr, const char *ptr, size_t len)
{
	int err;
	unsigned long inputVal;
	struct scpi_drvinfo *scpi_info;

	scpi_info = get_scpi_drvinfo();
	if (!scpi_info)
		return -ENODATA;

	err = kstrtoul(ptr, 10, &inputVal);
	if (err)
		return err;

	mero_scpi_set_clk(scpi_info->np, "a55twoclk", inputVal);

	return len;
}

static ssize_t a55twoclk_show(struct device *dev,
		struct device_attribute *attr, char *ptr)
{
	unsigned long inputVal;
	struct scpi_drvinfo *scpi_info;

	scpi_info = get_scpi_drvinfo();
	if (!scpi_info)
		return -ENODATA;

	inputVal = scpi_info->scpi_ops->clk_get_val(A55TWOCLK) * SCPI_1MHZ;

	return scnprintf(ptr, PAGE_SIZE,
			"a55twoclk clock rate: %lu\n", inputVal);
}

int cvip_plt_scpi_set_a55two(unsigned long freq)
{
	struct scpi_drvinfo *scpi_info;

	scpi_info = get_scpi_drvinfo();
	if (!scpi_info)
		return -ENODATA;
	mero_scpi_set_clk(scpi_info->np, "a55twoclk", freq);
	return 0;
}

int cvip_plt_scpi_set_nic(unsigned long freq)
{
	struct scpi_drvinfo *scpi_info;

	scpi_info = get_scpi_drvinfo();
	if (!scpi_info)
		return -ENODATA;
	mero_scpi_set_clk(scpi_info->np, "nicclk", freq);
	return 0;
}

int cvip_plt_scpi_set_decompress(unsigned long freq)
{
	struct scpi_drvinfo *scpi_info;

	scpi_info = get_scpi_drvinfo();
	if (!scpi_info)
		return -ENODATA;
	mero_scpi_set_clk(scpi_info->np, "decompressclk", freq);
	return 0;
}

int cvip_plt_scpi_set_a55dsu(unsigned long freq)
{
	struct scpi_drvinfo *scpi_info;

	scpi_info = get_scpi_drvinfo();
	if (!scpi_info)
		return -ENODATA;

	mero_scpi_set_clk(scpi_info->np, "a55dsuclk", freq);
	return 0;
}

static const DEVICE_ATTR_RW(smuready);
static const DEVICE_ATTR_RW(coresightclk);
static const DEVICE_ATTR_RW(c5clk);
static const DEVICE_ATTR_RW(q6clk);
static const DEVICE_ATTR_RW(decompressclk);
static const DEVICE_ATTR_RW(nic400clk);
static const DEVICE_ATTR_RW(a55dsuclk);
static const DEVICE_ATTR_RW(a55twoclk);

const struct attribute *scpi_attrs[] = {
	&dev_attr_smuready.attr,
	&dev_attr_coresightclk.attr,
	&dev_attr_decompressclk.attr,
	&dev_attr_c5clk.attr,
	&dev_attr_q6clk.attr,
	&dev_attr_nic400clk.attr,
	&dev_attr_a55dsuclk.attr,
	&dev_attr_a55twoclk.attr,
	NULL
};

