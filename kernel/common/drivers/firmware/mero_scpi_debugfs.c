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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include <linux/compiler.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/debugfs.h>
#include <linux/uaccess.h>
#include <linux/err.h>
#include <linux/scpi_protocol.h>
#include <linux/of_address.h>
#include <linux/of_platform.h>
#include <linux/clk.h>
#include "mero_scpi.h"
#include "mero_scpi_debugfs.h"
#include "arm_scpi.h"

#define SIZE            31
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

static int mero_scpi_open(struct inode *inode, struct file *file)
{
	file->private_data = inode->i_private;
	return 0;
}

static ssize_t mero_scpi_coresight_write(struct file *file,
					 const char __user *ptr, size_t len,
					 loff_t *off)
{
	int ret;
	char input[SIZE + 1];
	unsigned long inputVal;
	struct scpi_drvinfo *scpi_info;
	char *p;
	int slen;

	if (!file->private_data) {
		pr_err("Invalid pointer to device structure.\n");
		return len;
	}
	scpi_info = (struct scpi_drvinfo *)file->private_data;
	if (len > SIZE) {
		pr_err("Length of the buffer exceeds size\n");
		return len;
	}
	ret = copy_from_user(input, ptr, len);
	if (ret < 0) {
		pr_err("Unable to copy from user.\n");
		return len;
	}

	p = memchr(input, '\n', len);
	slen = p ? p - input : len;

	if (slen == strlen(scpi_clock_on) &&
	    !strncmp(input, scpi_clock_on, slen))
		inputVal = SCPI_1MHZ;
	else
		if (slen == strlen(scpi_clock_off) &&
		    !strncmp(input, scpi_clock_off, slen))
			inputVal = 0;
		else
			return len;

	mero_scpi_set_clk(scpi_info->np, "coresightclk", inputVal);

	return len;
}

static ssize_t mero_scpi_coresight_read(struct file *file, char __user *ptr,
					size_t len, loff_t *off)
{
	char buff[100];
	unsigned long inputVal;
	struct scpi_drvinfo *scpi_info;

	if (!file->private_data) {
		pr_err("Invalid pointer to device structure.\n");
		return len;
	}
	scpi_info = (struct scpi_drvinfo *)file->private_data;

	inputVal = scpi_info->scpi_ops->clk_get_val(CORESIGHTCLK) * SCPI_1MHZ;
	snprintf(buff, sizeof(buff), "Coresight clock rate: %lu\n", inputVal);

	return simple_read_from_buffer(ptr, len, off, buff,
				       strlen(buff));
}

static ssize_t mero_scpi_C5_write(struct file *file,
				  const char __user *ptr, size_t len,
				  loff_t *off)
{
	int ret;
	char input[SIZE + 1];
	unsigned long inputVal;
	struct scpi_drvinfo *scpi_info;

	if (!file->private_data) {
		pr_err("Invalid pointer to device structure.\n");
		return len;
	}
	scpi_info = (struct scpi_drvinfo *)file->private_data;
	if (len > SIZE) {
		pr_err("Length of the buffer exceeds size\n");
		return len;
	}
	ret = copy_from_user(input, ptr, len);
	if (ret < 0) {
		pr_err("Unable to copy from user.\n");
		return len;
	}
	input[len] = '\0';
	sscanf(input, "%lu", &inputVal);

	mero_scpi_set_clk(scpi_info->np, "c5clk", inputVal);

	return len;
}

static ssize_t mero_scpi_C5_read(struct file *file, char __user *ptr,
				 size_t len, loff_t *off)
{
	char buff[100];
	unsigned long inputVal;
	struct scpi_drvinfo *scpi_info;

	if (!file->private_data) {
		pr_err("Invalid pointer to device structure.\n");
		return len;
	}
	scpi_info = (struct scpi_drvinfo *)file->private_data;

	inputVal = scpi_info->scpi_ops->clk_get_val(C5CLK) * SCPI_1MHZ;
	snprintf(buff, sizeof(buff), "C5 clock rate: %lu\n", inputVal);

	return simple_read_from_buffer(ptr, len, off, buff,
				       strlen(buff));
}

static ssize_t mero_scpi_Q6_write(struct file *file,
				  const char __user *ptr, size_t len,
				  loff_t *off)
{
	int ret;
	char input[SIZE + 1];
	unsigned long inputVal;
	struct scpi_drvinfo *scpi_info;

	if (!file->private_data) {
		pr_err("Invalid pointer to device structure.\n");
		return len;
	}
	scpi_info = (struct scpi_drvinfo *)file->private_data;
	if (len > SIZE) {
		pr_err("Length of the buffer exceeds size\n");
		return len;
	}
	ret = copy_from_user(input, ptr, len);
	if (ret < 0) {
		pr_err("Unable to copy from user.\n");
		return len;
	}
	input[len] = '\0';
	sscanf(input, "%lu", &inputVal);

	mero_scpi_set_clk(scpi_info->np, "q6clk", inputVal);

	return len;
}

static ssize_t mero_scpi_Q6_read(struct file *file, char __user *ptr,
				 size_t len, loff_t *off)
{
	char buff[100];
	unsigned long inputVal;
	struct scpi_drvinfo *scpi_info;

	if (!file->private_data) {
		pr_err("Invalid pointer to device structure.\n");
		return len;
	}
	scpi_info = (struct scpi_drvinfo *)file->private_data;
	inputVal = scpi_info->scpi_ops->clk_get_val(Q6CLK) * SCPI_1MHZ;
	snprintf(buff, sizeof(buff), "Q6 clock rate: %lu\n", inputVal);

	return simple_read_from_buffer(ptr, len, off, buff,
				       strlen(buff));
}

static ssize_t mero_scpi_nic400_write(struct file *file,
				      const char __user *ptr, size_t len,
				      loff_t *off)
{
	int ret;
	char input[SIZE + 1];
	unsigned long inputVal;
	struct scpi_drvinfo *scpi_info;

	if (!file->private_data) {
		pr_err("Invalid pointer to device structure.\n");
		return len;
	}
	scpi_info = (struct scpi_drvinfo *)file->private_data;
	if (len > SIZE) {
		pr_err("Length of the buffer exceeds size\n");
		return len;
	}
	ret = copy_from_user(input, ptr, len);
	if (ret < 0) {
		pr_err("Unable to copy from user.\n");
		return len;
	}
	input[len] = '\0';
	sscanf(input, "%lu", &inputVal);

	mero_scpi_set_clk(scpi_info->np, "nicclk", inputVal);

	return len;
}

static ssize_t mero_scpi_nic400_read(struct file *file, char __user *ptr,
				     size_t len, loff_t *off)
{
	char buff[100];
	unsigned long inputVal;
	struct scpi_drvinfo *scpi_info;

	if (!file->private_data) {
		pr_err("Invalid pointer to device structure.\n");
		return len;
	}
	scpi_info = (struct scpi_drvinfo *)file->private_data;
	inputVal = scpi_info->scpi_ops->clk_get_val(NIC400CLK) * SCPI_1MHZ;
	snprintf(buff, sizeof(buff), "Nic400 clock rate: %lu\n", inputVal);

	return simple_read_from_buffer(ptr, len, off, buff,
				       strlen(buff));
}

static ssize_t mero_scpi_decompress_write(struct file *file,
					  const char __user *ptr, size_t len,
					  loff_t *off)
{
	int ret;
	int err;
	char input[SIZE + 1];
	unsigned long inputVal;
	struct scpi_drvinfo *scpi_info;

	if (!file->private_data) {
		pr_err("Invalid pointer to device structure.\n");
		return len;
	}
	scpi_info = (struct scpi_drvinfo *)file->private_data;
	if (len > SIZE) {
		pr_err("Length of the buffer exceeds size\n");
		return len;
	}
	ret = copy_from_user(input, ptr, len);
	if (ret < 0) {
		pr_err("Unable to copy from user.\n");
		return len;
	}
	input[len] = '\0';
	err = kstrtoul(input, 10, &inputVal);
	if (err) {
		pr_err("Fail to obtain input value\n");
		return len;
	}

	mero_scpi_set_clk(scpi_info->np, "decompressclk", inputVal);

	return len;
}

static ssize_t mero_scpi_decompress_read(struct file *file, char __user *ptr,
					 size_t len, loff_t *off)
{
	char buff[100];
	unsigned long inputVal;
	struct scpi_drvinfo *scpi_info;

	if (!file->private_data) {
		pr_err("Invalid pointer to device structure.\n");
		return len;
	}
	scpi_info = (struct scpi_drvinfo *)file->private_data;
	inputVal = scpi_info->scpi_ops->clk_get_val(DECOMPRESSCLK) * SCPI_1MHZ;
	snprintf(buff, sizeof(buff), "Decompress clock rate: %lu\n", inputVal);

	return simple_read_from_buffer(ptr, len, off, buff,
				       strlen(buff));
}

static ssize_t mero_scpi_a55dsuclk_write(struct file *file,
					 const char __user *ptr, size_t len,
					 loff_t *off)
{
	int ret;
	char input[SIZE + 1];
	int err;
	unsigned long inputVal;
	struct scpi_drvinfo *scpi_info;

	if (!file->private_data) {
		pr_err("Invalid pointer to device structure.\n");
		return len;
	}
	scpi_info = (struct scpi_drvinfo *)file->private_data;
	if (len > SIZE) {
		pr_err("Length of the buffer exceeds size\n");
		return len;
	}
	ret = copy_from_user(input, ptr, len);
	if (ret < 0) {
		pr_err("Unable to copy from user.\n");
		return len;
	}
	input[len] = '\0';
	err = kstrtoul(input, 10, &inputVal);
	if (err)
		return err;

	mero_scpi_set_clk(scpi_info->np, "a55dsuclk", inputVal);

	return len;
}

static ssize_t mero_scpi_a55dsuclk_read(struct file *file, char __user *ptr,
					size_t len, loff_t *off)
{
	char buff[100];
	unsigned long inputVal;
	struct scpi_drvinfo *scpi_info;

	if (!file->private_data) {
		pr_err("Invalid pointer to device structure.\n");
		return len;
	}
	scpi_info = (struct scpi_drvinfo *)file->private_data;
	inputVal = scpi_info->scpi_ops->clk_get_val(A55DSUCLK) * SCPI_1MHZ;
	snprintf(buff, sizeof(buff), "A55DSU clock rate: %lu\n", inputVal);

	return simple_read_from_buffer(ptr, len, off, buff,
				       strlen(buff));
}

static ssize_t mero_scpi_smu_ready_write(struct file *file,
					 const char __user *ptr, size_t len,
					 loff_t *off)
{
	int ret;
	char input[SIZE + 1];
	unsigned long inputval;

	if (!file->private_data) {
		pr_err("Invalid pointer to device structure.\n");
		return len;
	}
	if (len > SIZE) {
		pr_err("Length of the buffer exceeds size\n");
		return len;
	}
	ret = copy_from_user(input, ptr, len);
	if (ret < 0) {
		pr_err("Unable to copy from user.\n");
		return len;
	}
	input[len] = '\0';
	ret = kstrtoul(input, 10, &inputval);
	if (ret)
		pr_err("Fail to obtain input value\n");
	else
		set_smu_dpm_ready(inputval);

	return len;
}

static ssize_t mero_scpi_smu_ready_read(struct file *file, char __user *ptr,
					size_t len, loff_t *off)
{
	char buff[100];
	int dpm_ready;

	if (!file->private_data) {
		pr_err("Invalid pointer to device structure.\n");
		return len;
	}

	dpm_ready = is_smu_dpm_ready();
	snprintf(buff, sizeof(buff), "%u\n", dpm_ready);

	return simple_read_from_buffer(ptr, len, off, buff,
				       strlen(buff));
}

static ssize_t mero_scpi_a55twoclk_write(struct file *file,
					 const char __user *ptr, size_t len,
					 loff_t *off)
{
	int ret;
	char input[SIZE + 1];
	int err;
	unsigned long inputVal;
	struct scpi_drvinfo *scpi_info;

	if (!file->private_data) {
		pr_err("Invalid pointer to device structure.\n");
		return len;
	}
	scpi_info = (struct scpi_drvinfo *)file->private_data;
	if (len > SIZE) {
		pr_err("Length of the buffer exceeds size\n");
		return len;
	}
	ret = copy_from_user(input, ptr, len);
	if (ret < 0) {
		pr_err("Unable to copy from user.\n");
		return len;
	}
	input[len] = '\0';
	err = kstrtoul(input, 10, &inputVal);
	if (err)
		return err;

	mero_scpi_set_clk(scpi_info->np, "a55twoclk", inputVal);

	return len;
}

static ssize_t mero_scpi_a55twoclk_read(struct file *file, char __user *ptr,
					size_t len, loff_t *off)
{
	char buff[100];
	unsigned long inputVal;
	struct scpi_drvinfo *scpi_info;

	if (!file->private_data) {
		pr_err("Invalid pointer to device structure.\n");
		return len;
	}
	scpi_info = (struct scpi_drvinfo *)file->private_data;

	inputVal = scpi_info->scpi_ops->clk_get_val(A55TWOCLK) * SCPI_1MHZ;
	snprintf(buff, sizeof(buff), "a55twoclk clock rate: %lu\n", inputVal);

	return simple_read_from_buffer(ptr, len, off, buff,
				       strlen(buff));
}

static const struct file_operations smu_ready_debugfs_ops = {
	.owner = THIS_MODULE,
	.open = mero_scpi_open,
	.write = mero_scpi_smu_ready_write,
	.read = mero_scpi_smu_ready_read,
};

static const struct file_operations coresight_debugfs_ops = {
	.owner = THIS_MODULE,
	.open = mero_scpi_open,
	.write = mero_scpi_coresight_write,
	.read = mero_scpi_coresight_read,
};

static const struct file_operations C5_debugfs_ops = {
	.owner = THIS_MODULE,
	.open = mero_scpi_open,
	.write = mero_scpi_C5_write,
	.read = mero_scpi_C5_read,
};

static const struct file_operations Q6_debugfs_ops = {
	.owner = THIS_MODULE,
	.open = mero_scpi_open,
	.write = mero_scpi_Q6_write,
	.read = mero_scpi_Q6_read,
};

static const struct file_operations nic400_debugfs_ops = {
	.owner = THIS_MODULE,
	.open = mero_scpi_open,
	.write = mero_scpi_nic400_write,
	.read = mero_scpi_nic400_read,
};

static const struct file_operations decompress_debugfs_ops = {
	.owner = THIS_MODULE,
	.open = mero_scpi_open,
	.write = mero_scpi_decompress_write,
	.read = mero_scpi_decompress_read,
};

static const struct file_operations a55dsuclk_debugfs_ops = {
	.owner = THIS_MODULE,
	.open = mero_scpi_open,
	.write = mero_scpi_a55dsuclk_write,
	.read = mero_scpi_a55dsuclk_read,
};

static const struct file_operations a55twoclk_debugfs_ops = {
	.owner = THIS_MODULE,
	.open = mero_scpi_open,
	.write = mero_scpi_a55twoclk_write,
	.read = mero_scpi_a55twoclk_read,
};

int mero_scpi_debugfs_init(struct scpi_drvinfo *scpi_info)
{
	/* Initialize debugfs interface */
	firmware_debugfs_root = debugfs_create_dir("mero_scpi", NULL);
	if (!firmware_debugfs_root) {
		pr_err("Failed to create mero_scpi directory\n");
		goto error;
	}

	if (!debugfs_create_file("smuready", 0600,
				 firmware_debugfs_root,
				 scpi_info, &smu_ready_debugfs_ops)) {
		pr_err("Failed to create smu ready file\n");
		goto error;
	}
	if (!debugfs_create_file("coresightclk", 0600,
				 firmware_debugfs_root,
				 scpi_info, &coresight_debugfs_ops)) {
		pr_err("Failed to create Coresight clk file\n");
		goto error;
	}
	if (!debugfs_create_file("c5clk", 0600, firmware_debugfs_root,
				 scpi_info,  &C5_debugfs_ops)) {
		pr_err("Failed to create C5clk file\n");
		goto error;
	}
	if (!debugfs_create_file("q6clk", 0600, firmware_debugfs_root,
				 scpi_info,  &Q6_debugfs_ops)) {
		pr_err("Failed to create Q6clk file\n");
		goto error;
	}
	if (!debugfs_create_file("decompressclk", 0600,
				 firmware_debugfs_root,
				 scpi_info,  &decompress_debugfs_ops)) {
		pr_err("Failed to create Decompressclk file\n");
		goto error;
	}
	if (!debugfs_create_file("nic400clk", 0600, firmware_debugfs_root,
				 scpi_info,  &nic400_debugfs_ops)) {
		pr_err("Failed to create Nic400clk file\n");
		goto error;
	}
	if (!debugfs_create_file("a55dsuclk", 0600, firmware_debugfs_root,
				 scpi_info,  &a55dsuclk_debugfs_ops)) {
		pr_err("Failed to create a55dsuclk file\n");
		goto error;
	}
	if (!debugfs_create_file("a55twoclk", 0600, firmware_debugfs_root,
				 scpi_info,  &a55twoclk_debugfs_ops)) {
		pr_err("Failed to create a55twoclk file\n");
		goto error;
	}

	return 0;
error:
	if (!firmware_debugfs_root)
		debugfs_remove_recursive(firmware_debugfs_root);
	return -1;
}
EXPORT_SYMBOL(mero_scpi_debugfs_init);

void mero_scpi_debugfs_exit(void)
{
	debugfs_remove_recursive(firmware_debugfs_root);
}
EXPORT_SYMBOL(mero_scpi_debugfs_exit);
