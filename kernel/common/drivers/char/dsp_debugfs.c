/*
 * Copyright (C) 2019-2020 Advanced Micro Devices, Inc. All rights reserved.
 */

#include <linux/debugfs.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/dsp_manager.h>
#include <linux/clk.h>
#include "dsp_manager_internal.h"

static int dsp_debugfs_open(struct inode *inode, struct file *file);

static ssize_t dsp_debugfs_rd_dspalloc(struct file *file, char __user *buffer,
				       size_t length, loff_t *offset);
static ssize_t dsp_debugfs_wr_dspalloc(struct file *file,
				       const char __user *buffer,
				       size_t length, loff_t *offset);

static const struct file_operations dsp_debug_dspalloc_fops = {
	.open = dsp_debugfs_open,
	.read = dsp_debugfs_rd_dspalloc,
	.write = dsp_debugfs_wr_dspalloc,
};

static ssize_t dsp_debugfs_rd_dspclient(struct file *file, char __user *buffer,
					size_t length, loff_t *offset);

static const struct file_operations dsp_debug_dspclient_fops = {
	.open = dsp_debugfs_open,
	.read = dsp_debugfs_rd_dspclient,
	.write = NULL,
};

static ssize_t dsp_debugfs_rd_dspstate(struct file *file, char __user *buffer,
				       size_t length, loff_t *offset);
static ssize_t dsp_debugfs_wr_dspstate(struct file *file,
				       const char __user *buffer,
				       size_t length, loff_t *offset);

static const struct file_operations dsp_debug_dspstate_fops = {
	.open = dsp_debugfs_open,
	.read = dsp_debugfs_rd_dspstate,
	.write = dsp_debugfs_wr_dspstate,
};

static ssize_t dsp_debugfs_rd_dspclkgate(struct file *file,
					 char __user *buffer,
					 size_t length,
					 loff_t *offset);
static ssize_t dsp_debugfs_wr_dspclkgate(struct file *file,
					 const char __user *buffer,
					 size_t length,
					 loff_t *offset);

static const struct file_operations dsp_debug_dspclkgate_fops = {
	.open = dsp_debugfs_open,
	.read = dsp_debugfs_rd_dspclkgate,
	.write = dsp_debugfs_wr_dspclkgate,
};

static ssize_t dsp_debugfs_rd_stat_vector_sel(struct file *file,
					      char __user *buffer,
					      size_t length,
					      loff_t *offset);
static ssize_t dsp_debugfs_wr_stat_vector_sel(struct file *file,
					      const char __user *buffer,
					      size_t length,
					      loff_t *offset);

static const struct file_operations dsp_debug_stat_vector_sel_fops = {
	.open = dsp_debugfs_open,
	.read = dsp_debugfs_rd_stat_vector_sel,
	.write = dsp_debugfs_wr_stat_vector_sel,
};

static ssize_t dsp_debugfs_rd_dsp_pwrgate(struct file *file,
					  char __user *buffer,
					  size_t length,
					  loff_t *offset);
static ssize_t dsp_debugfs_wr_dsp_pwrgate(struct file *file,
					  const char __user *buffer,
					  size_t length,
					  loff_t *offset);

static const struct file_operations dsp_debug_dsp_pwrgate_fops = {
	.open = dsp_debugfs_open,
	.read = dsp_debugfs_rd_dsp_pwrgate,
	.write = dsp_debugfs_wr_dsp_pwrgate,
};

static ssize_t dsp_debugfs_wr_dspctrl(struct file *file,
				      const char __user *buffer,
				      size_t length, loff_t *offset);

static const struct file_operations dsp_debug_dspctrl_fops = {
	.open = dsp_debugfs_open,
	.read = NULL,
	.write = dsp_debugfs_wr_dspctrl,
};

static ssize_t dsp_debugfs_wr_dspdpm(struct file *file,
				     const char __user *buffer,
				     size_t length,
				     loff_t *offset);

static const struct file_operations dsp_debug_dspdpm_fops = {
	.open = dsp_debugfs_open,
	.read = NULL,
	.write = dsp_debugfs_wr_dspdpm,
};

static int dsp_splice_idx;
static int dsp_debugfs_idx;
static struct list_head *dsp_debugfs_pos;
static int dspalloc_dsp_id;
static int dspstate_dsp_id;

static int dsp_debugfs_open(struct inode *inode, struct file *file)
{
	file->private_data = inode->i_private;
	return 0;
}

static int dsp_user_copy(int *shift, char __user *buffer,
			 size_t length, int *splice_idx, char *fmt, ...)
{
	char output[512];
	int output_size;
	va_list argptr;

	va_start(argptr, fmt);
	output_size = vsnprintf(output, sizeof(output), fmt, argptr);
	va_end(argptr);

	if (output_size < 0)
		return -ENOMEM;

	if ((*shift + output_size - *splice_idx) > length) {
		DSP_DBG("Larger than buffer, splicing.\n");
		output_size = length - *shift;
		if (copy_to_user(buffer + *shift, output + *splice_idx,
				 output_size)) {
			DSP_ERROR("Failed to read event to user.\n");
			return -EFAULT;
		}
		*splice_idx += output_size;
		*shift += output_size;
		return 1;
	}
	if (copy_to_user(buffer + *shift, output + *splice_idx,
			 output_size - *splice_idx)) {
		DSP_ERROR("Failed to read event to user.\n");
		return -EFAULT;
	}

	*shift += (output_size - *splice_idx);
	*splice_idx = 0;
	return 0;
}

static ssize_t dsp_debugfs_rd_dspalloc(struct file *file, char __user *buffer,
				       size_t length, loff_t *offset)
{
	struct dsp_device_t *dsp_device;
	struct dsp_resources_t *dsp_resource = NULL;
	struct dsp_memory_t *dsp_memory;
	struct list_head *pos;
	int shift = 0;
	int ret;

	if (dsp_debugfs_idx < 0 && *offset > 0)
		return 0;

	if (dspalloc_dsp_id < DSP_Q6_0 || dspalloc_dsp_id >= DSP_MAX) {
		DSP_ERROR("DSP ID %d not in range of available DSPs.\n",
			  dspalloc_dsp_id);
		ret = -EINVAL;
		goto error;
	}

	if (!file->private_data) {
		DSP_ERROR("Invalid pointer to device structure.\n");
		ret = -EFAULT;
		goto error;
	}
	dsp_device = (struct dsp_device_t *)file->private_data;

	ret = mutex_lock_interruptible(&dsp_device->dsp_device_lock);
	if (ret != 0) {
		DSP_ERROR("Failed to lock DSP Loader device.\n");
		goto error;
	}

	if (list_empty(&dsp_device->dsp_list)) {
		DSP_ERROR("DSP_Loader DSP list is empty.\n");
		mutex_unlock(&dsp_device->dsp_device_lock);
		ret = -EFAULT;
		goto error;
	}

	list_for_each(pos, &dsp_device->dsp_list) {
		dsp_resource = list_entry(pos, struct dsp_resources_t, list);
		if (dsp_resource->dsp_id == dspalloc_dsp_id)
			break;
		if (list_is_last(pos, &dsp_device->dsp_list)) {
			DSP_ERROR("DSP_Loader DSP not registered.\n");
			mutex_unlock(&dsp_device->dsp_device_lock);
			ret = -EINVAL;
			goto error;
		}
	}
	mutex_unlock(&dsp_device->dsp_device_lock);

	ret = mutex_lock_interruptible(&dsp_resource->dsp_resource_lock);
	if (ret != 0) {
		DSP_ERROR("Failed to lock DSP.\n");
		goto error;
	}

	if (list_empty(&dsp_resource->memory_list)) {
		ret = dsp_user_copy(&shift, buffer, length, &dsp_splice_idx,
				    "No memory has been allocated for DSP %d\n",
				    dspalloc_dsp_id);
		if (ret < 0)
			goto post_lock_error;
		dsp_debugfs_idx = -1;
		goto exit;
	}
	if (!*offset) {
		dsp_debugfs_pos = &dsp_resource->memory_list;
		dsp_debugfs_idx = 0;
	}
	dsp_memory = list_entry(dsp_debugfs_pos, struct dsp_memory_t, list);
	list_for_each_entry_continue(dsp_memory, &dsp_resource->memory_list,
				     list) {
		ret = dsp_user_copy(&shift, buffer, length, &dsp_splice_idx,
				    "DSP ID: %d\n"
				    "File Descriptor: %d\n"
				    "Memory Pool ID: %d\n"
				    "Cache Attribute: %x\n"
				    "Address: %lx\n"
				    "Size:%x\n"
				    "Access Attribute %d\n\n",
				    dspalloc_dsp_id,
				    dsp_memory->fd,
				    dsp_memory->mem_pool_id,
				    dsp_memory->cache_attr,
				    (uintptr_t)dsp_memory->addr,
				    dsp_memory->size,
				    dsp_memory->access_attr);
		if (ret < 0)
			goto post_lock_error;
		if (ret > 0)
			goto exit;
		dsp_debugfs_pos = &dsp_memory->list;
	}
	dsp_debugfs_idx = -1;
exit:
	mutex_unlock(&dsp_resource->dsp_resource_lock);
	*offset += shift;
	return shift;

post_lock_error:
	mutex_unlock(&dsp_resource->dsp_resource_lock);
error:
	return ret;
}

static ssize_t dsp_debugfs_wr_dspalloc(struct file *file,
				       const char __user *buffer,
				       size_t length, loff_t *offset)
{
	char input[16];
	int dsp_id;
	int ret;

	if (length >= sizeof(input)) {
		DSP_ERROR("Invalid write input.\n");
		ret = -EINVAL;
		goto error;
	}

	ret = copy_from_user(input, buffer, length);
	if (ret < 0) {
		DSP_ERROR("Unable to copy from user.\n");
		goto error;
	}
	input[length] = '\0';

	ret = kstrtoint(input, 10, &dsp_id);
	if (ret) {
		DSP_ERROR("Invalid write input.\n");
		ret = -EINVAL;
		goto error;
	}

	if (dsp_id < DSP_Q6_0 || dsp_id >= DSP_MAX) {
		DSP_ERROR("DSP ID %d not in range of available DSPs.\n",
			  dsp_id);
		ret = -EINVAL;
		goto error;
	}
	dspalloc_dsp_id = dsp_id;
	DSP_DBG("DSP debugfs dsp alloc ID set to %d.\n", dspalloc_dsp_id);
	ret = length;
error:
	return ret;
}

static ssize_t dsp_debugfs_rd_dspclient(struct file *file, char __user *buffer,
					size_t length, loff_t *offset)
{
	struct dsp_device_t *dsp_device;
	struct dsp_client_t *dsp_client;
	int shift = 0;
	int ret;

	if (dsp_debugfs_idx < 0 && *offset > 0)
		return 0;

	if (!file->private_data) {
		DSP_ERROR("Invalid pointer to device structure.\n");
		ret = -EFAULT;
		goto error;
	}
	dsp_device = (struct dsp_device_t *)file->private_data;

	ret = mutex_lock_interruptible(&dsp_device->dsp_device_lock);
	if (ret != 0) {
		DSP_ERROR("Failed to lock DSP Loader device.\n");
		goto error;
	}

	if (list_empty(&dsp_device->client_list)) {
		ret = dsp_user_copy(&shift, buffer, length, &dsp_splice_idx,
				    "No clients registered\n");
		if (ret < 0)
			goto post_lock_error;
		dsp_debugfs_idx = -1;
		goto exit;
	}
	if (!*offset) {
		dsp_debugfs_pos = &dsp_device->client_list;
		dsp_debugfs_idx = 0;
	}
	dsp_client = list_entry(dsp_debugfs_pos, struct dsp_client_t, list);
	list_for_each_entry_continue(dsp_client, &dsp_device->client_list,
				     list) {
		ret = dsp_user_copy(&shift, buffer, length, &dsp_splice_idx,
				    "Client %lu\n", (uintptr_t)dsp_client);
		if (ret < 0)
			goto post_lock_error;
		if (ret > 0)
			goto exit;
		dsp_debugfs_pos = &dsp_client->list;
	}
	dsp_debugfs_idx = -1;
exit:
	mutex_unlock(&dsp_device->dsp_device_lock);
	*offset += shift;
	return shift;

post_lock_error:
	mutex_unlock(&dsp_device->dsp_device_lock);
error:
	return ret;
}

static ssize_t dsp_debugfs_rd_dspstate(struct file *file, char __user *buffer,
				       size_t length, loff_t *offset)
{
	struct dsp_device_t *dsp_device;
	struct dsp_resources_t *dsp_resource = NULL;
	struct dsp_state_t *dsp_states;
	struct list_head *pos;
	static char *mode_string;
	int shift = 0;
	int dsp_mode;
	int ret;

	if (dsp_debugfs_idx < 0 && *offset > 0)
		return 0;

	if (!file->private_data) {
		DSP_ERROR("Invalid pointer to device structure.\n");
		ret = -EFAULT;
		goto error;
	}
	dsp_device = (struct dsp_device_t *)file->private_data;

	if (dspstate_dsp_id < DSP_Q6_0 || dspstate_dsp_id >= DSP_MAX) {
		DSP_ERROR("DSP ID %d not in range of available DSPs.\n",
			  dspstate_dsp_id);
		ret = -EINVAL;
		goto error;
	}

	ret = mutex_lock_interruptible(&dsp_device->dsp_device_lock);
	if (ret != 0) {
		DSP_ERROR("Failed to lock DSP Loader device.\n");
		goto error;
	}

	if (list_empty(&dsp_device->dsp_list)) {
		DSP_ERROR("DSP_Loader DSP list is empty.\n");
		mutex_unlock(&dsp_device->dsp_device_lock);
		ret = -EFAULT;
		goto error;
	}

	list_for_each(pos, &dsp_device->dsp_list) {
		dsp_resource = list_entry(pos, struct dsp_resources_t, list);
		if (dsp_resource->dsp_id == dspstate_dsp_id)
			break;
		if (list_is_last(pos, &dsp_device->dsp_list)) {
			DSP_ERROR("DSP_Loader DSP not registered.\n");
			mutex_unlock(&dsp_device->dsp_device_lock);
			ret = -EINVAL;
			goto error;
		}
	}
	mutex_unlock(&dsp_device->dsp_device_lock);

	ret = mutex_lock_interruptible(&dsp_resource->dsp_resource_lock);
	if (ret != 0) {
		DSP_ERROR("Failed to lock DSP.\n");
		goto error;
	}

	if (!dsp_resource->states) {
		DSP_ERROR("Invalid pointer to state structure.\n");
		ret = -EFAULT;
		goto post_lock_error;
	}
	dsp_states = dsp_resource->states;

	dsp_mode = dsp_helper_read(dsp_states);
	switch (dsp_mode) {
	case DSP_OFF:
		mode_string = "OFF";
		break;
	case DSP_STOP:
		mode_string = "STOP";
		break;
	case DSP_PAUSE:
		mode_string = "PAUSE";
		break;
	case DSP_RUN:
		mode_string = "RUN";
		break;
	default:
		DSP_ERROR("Unknown DSP state encounterd, update states.\n");
		ret = -EINVAL;
		goto post_lock_error;
	}

	ret = dsp_user_copy(&shift, buffer, length, &dsp_splice_idx,
			    "DSP ID: %d\nDSP state: %s\n",
			    dspstate_dsp_id, mode_string);
	if (ret < 0)
		goto post_lock_error;
	dsp_debugfs_idx = -1;
	mutex_unlock(&dsp_resource->dsp_resource_lock);
	*offset += shift;
	return shift;

post_lock_error:
	mutex_unlock(&dsp_resource->dsp_resource_lock);
error:
	return ret;
}

static ssize_t dsp_debugfs_wr_dspstate(struct file *file,
				       const char __user *buffer,
				       size_t length, loff_t *offset)
{
	char input[16];
	int dsp_id;
	int ret;

	if (length >= sizeof(input)) {
		DSP_ERROR("Invalid write input.\n");
		ret = -EINVAL;
		goto error;
	}

	ret = copy_from_user(input, buffer, length);
	if (ret < 0) {
		DSP_ERROR("Unable to copy from user.\n");
		goto error;
	}
	input[length] = '\0';

	ret = kstrtoint(input, 10, &dsp_id);
	if (ret) {
		DSP_ERROR("Invalid write input.\n");
		ret = -EINVAL;
		goto error;
	}

	if (dsp_id < DSP_Q6_0 || dsp_id >= DSP_MAX) {
		DSP_ERROR("DSP ID %d not in range of available DSPs.\n",
			  dsp_id);
		ret = -EINVAL;
		goto error;
	}
	dspstate_dsp_id = dsp_id;
	DSP_DBG("DSP debugfs dsp state ID set to %d.\n", dspstate_dsp_id);
	ret = length;
error:
	return ret;
}

static ssize_t dsp_debugfs_rd_dspclkgate(struct file *file,
					 char __user *buffer,
					 size_t length,
					 loff_t *offset)
{
	struct dsp_device_t *dsp_device;
	struct dsp_resources_t *dsp_resource = NULL;
	struct dsp_state_t *dsp_states;
	struct list_head *pos;
	int shift = 0;
	int clkgate_Val;
	int ret;

	if (dsp_debugfs_idx < 0 && *offset > 0)
		return 0;

	if (!file->private_data) {
		DSP_ERROR("Invalid pointer to device structure.\n");
		ret = -EFAULT;
		goto error;
	}
	dsp_device = (struct dsp_device_t *)file->private_data;

	if (dspstate_dsp_id < DSP_Q6_0 || dspstate_dsp_id >= DSP_MAX) {
		DSP_ERROR("DSP ID %d not in range of available DSPs.\n",
			  dspstate_dsp_id);
		ret = -EINVAL;
		goto error;
	}

	ret = mutex_lock_interruptible(&dsp_device->dsp_device_lock);
	if (ret != 0) {
		DSP_ERROR("Failed to lock DSP Loader device.\n");
		goto error;
	}

	if (list_empty(&dsp_device->dsp_list)) {
		DSP_ERROR("DSP_Loader DSP list is empty.\n");
		mutex_unlock(&dsp_device->dsp_device_lock);
		ret = -EFAULT;
		goto error;
	}

	list_for_each(pos, &dsp_device->dsp_list) {
		dsp_resource = list_entry(pos, struct dsp_resources_t, list);
		if (dsp_resource->dsp_id == dspstate_dsp_id)
			break;
		if (list_is_last(pos, &dsp_device->dsp_list)) {
			DSP_ERROR("DSP_Loader DSP not registered.\n");
			mutex_unlock(&dsp_device->dsp_device_lock);
			ret = -EINVAL;
			goto error;
		}
	}
	mutex_unlock(&dsp_device->dsp_device_lock);

	ret = mutex_lock_interruptible(&dsp_resource->dsp_resource_lock);
	if (ret != 0) {
		DSP_ERROR("Failed to lock DSP.\n");
		goto error;
	}

	if (!dsp_resource->states) {
		DSP_ERROR("Invalid pointer to state structure.\n");
		ret = -EFAULT;
		goto post_lock_error;
	}
	dsp_states = dsp_resource->states;

	clkgate_Val = dsp_helper_read_clkgate_setting(dsp_states);

	ret = dsp_user_copy(&shift, buffer, length, &dsp_splice_idx,
			    "DSP ID: %d DSP clkgate setting: %d\n",
			    dspstate_dsp_id, clkgate_Val);
	if (ret < 0)
		goto post_lock_error;
	dsp_debugfs_idx = -1;
	mutex_unlock(&dsp_resource->dsp_resource_lock);
	*offset += shift;
	return shift;

post_lock_error:
	mutex_unlock(&dsp_resource->dsp_resource_lock);
error:
	return ret;
}

static ssize_t dsp_debugfs_wr_dspclkgate(struct file *file,
					 const char __user *buffer,
					 size_t length, loff_t *offset)
{
	struct dsp_device_t *dsp_device;
	struct dsp_resources_t *dsp_resource = NULL;
	struct dsp_state_t *dsp_states;
	struct list_head *pos;
	char input[32];
	int clkgate_val;
	int ret;

	if (length >= sizeof(input)) {
		DSP_ERROR("Invalid write input.\n");
		ret = -EINVAL;
		goto error;
	}

	ret = copy_from_user(input, buffer, length);
	if (ret < 0) {
		DSP_ERROR("Unable to copy from user.\n");
		goto error;
	}
	input[length] = '\0';

	ret = kstrtoint(input, 10, &clkgate_val);
	if (ret) {
		DSP_ERROR("Invalid write input.\n");
		ret = -EINVAL;
		goto error;
	}

	if (clkgate_val > 1) {
		DSP_ERROR("Invalid clkgate input > 1.\n");
		ret = -EINVAL;
		goto error;
	}

	if (!file->private_data) {
		DSP_ERROR("Invalid pointer to device structure.\n");
		ret = -EFAULT;
		goto error;
	}
	dsp_device = (struct dsp_device_t *)file->private_data;

	if (dspstate_dsp_id < DSP_Q6_0 || dspstate_dsp_id >= DSP_MAX) {
		DSP_ERROR("DSP ID %d not in range of available DSPs.\n",
			  dspstate_dsp_id);
		ret = -EINVAL;
		goto error;
	}

	ret = mutex_lock_interruptible(&dsp_device->dsp_device_lock);
	if (ret != 0) {
		DSP_ERROR("Failed to lock DSP Loader device.\n");
		goto error;
	}

	if (list_empty(&dsp_device->dsp_list)) {
		DSP_ERROR("DSP_Loader DSP list is empty.\n");
		mutex_unlock(&dsp_device->dsp_device_lock);
		ret = -EFAULT;
		goto error;
	}

	list_for_each(pos, &dsp_device->dsp_list) {
		dsp_resource = list_entry(pos, struct dsp_resources_t, list);
		if (dsp_resource->dsp_id == dspstate_dsp_id)
			break;
		if (list_is_last(pos, &dsp_device->dsp_list)) {
			DSP_ERROR("DSP_Loader DSP not registered.\n");
			mutex_unlock(&dsp_device->dsp_device_lock);
			ret = -EINVAL;
			goto error;
		}
	}
	mutex_unlock(&dsp_device->dsp_device_lock);

	ret = mutex_lock_interruptible(&dsp_resource->dsp_resource_lock);
	if (ret != 0) {
		DSP_ERROR("Failed to lock DSP.\n");
		goto error;
	}

	if (!dsp_resource->states) {
		DSP_ERROR("Invalid pointer to state structure.\n");
		ret = -EFAULT;
		goto post_lock_error;
	}
	dsp_states = dsp_resource->states;
	ret = dsp_helper_write_clkgate_setting(dsp_states,
					       clkgate_val);
	DSP_DBG("DSP debugfs dsp clkgate set to %d.\n", clkgate_val);
	ret = length;

post_lock_error:
	mutex_unlock(&dsp_resource->dsp_resource_lock);
error:
	return ret;
}

static ssize_t dsp_debugfs_wr_dspdpm(struct file *file,
				     const char __user *buffer,
				     size_t length,
				     loff_t *offset)
{
	char input[32];
	int clk_id;
	int dpm_level;
	int ret;

	if (length >= sizeof(input)) {
		DSP_ERROR("Invalid write input.\n");
		ret = -EINVAL;
		goto error;
	}

	ret = copy_from_user(input, buffer, length);
	if (ret < 0) {
		DSP_ERROR("Unable to copy from user.\n");
		goto error;
	}
	input[length] = '\0';

	ret = sscanf(input, "%d %d", &clk_id, &dpm_level);
	if (ret != 2) {
		DSP_ERROR("Invalid write input.\n");
		ret = -EINVAL;
		goto error;
	}

	ret = dsp_clk_dpm_cntrl(clk_id, dpm_level);
	if (ret < 0) {
		DSP_ERROR("Unable to set the dpmlevel.\n");
		goto error;
	}

	ret = length;
error:
	return ret;
}

static ssize_t dsp_debugfs_rd_stat_vector_sel(struct file *file,
					      char __user *buffer,
					      size_t length,
					      loff_t *offset)
{
	struct dsp_device_t *dsp_device;
	struct dsp_resources_t *dsp_resource = NULL;
	struct dsp_state_t *dsp_states;
	struct list_head *pos;
	int shift = 0;
	int override_val;
	int ret;

	if (dsp_debugfs_idx < 0 && *offset > 0)
		return 0;

	if (!file->private_data) {
		DSP_ERROR("Invalid pointer to device structure.\n");
		ret = -EFAULT;
		goto error;
	}
	dsp_device = (struct dsp_device_t *)file->private_data;

	if (dspstate_dsp_id < DSP_Q6_0 || dspstate_dsp_id >= DSP_MAX) {
		DSP_ERROR("DSP ID %d not in range of available DSPs.\n",
			  dspstate_dsp_id);
		ret = -EINVAL;
		goto error;
	}

	ret = mutex_lock_interruptible(&dsp_device->dsp_device_lock);
	if (ret != 0) {
		DSP_ERROR("Failed to lock DSP Loader device.\n");
		goto error;
	}

	if (list_empty(&dsp_device->dsp_list)) {
		DSP_ERROR("DSP_Loader DSP list is empty.\n");
		mutex_unlock(&dsp_device->dsp_device_lock);
		ret = -EFAULT;
		goto error;
	}

	list_for_each(pos, &dsp_device->dsp_list) {
		dsp_resource = list_entry(pos, struct dsp_resources_t, list);
		if (dsp_resource->dsp_id == dspstate_dsp_id)
			break;
		if (list_is_last(pos, &dsp_device->dsp_list)) {
			DSP_ERROR("DSP_Loader DSP not registered.\n");
			mutex_unlock(&dsp_device->dsp_device_lock);
			ret = -EINVAL;
			goto error;
		}
	}
	mutex_unlock(&dsp_device->dsp_device_lock);

	ret = mutex_lock_interruptible(&dsp_resource->dsp_resource_lock);
	if (ret != 0) {
		DSP_ERROR("Failed to lock DSP.\n");
		goto error;
	}

	if (!dsp_resource->states) {
		DSP_ERROR("Invalid pointer to state structure.\n");
		ret = -EFAULT;
		goto post_lock_error;
	}
	dsp_states = dsp_resource->states;

	override_val = dsp_helper_read_stat_vector_sel(dsp_states);

	ret = dsp_user_copy(&shift, buffer, length, &dsp_splice_idx,
			    "DSP ID: %d alternative vector selection: %d\n",
			    dspstate_dsp_id, override_val);
	if (ret < 0)
		goto post_lock_error;
	dsp_debugfs_idx = -1;
	mutex_unlock(&dsp_resource->dsp_resource_lock);
	*offset += shift;
	return shift;

post_lock_error:
	mutex_unlock(&dsp_resource->dsp_resource_lock);
error:
	return ret;
}

static ssize_t dsp_debugfs_wr_stat_vector_sel(struct file *file,
					      const char __user *buffer,
					      size_t length, loff_t *offset)
{
	struct dsp_device_t *dsp_device;
	struct dsp_resources_t *dsp_resource = NULL;
	struct dsp_state_t *dsp_states;
	struct list_head *pos;
	char input[32];
	int user_val;
	int ret;

	if (length >= sizeof(input)) {
		DSP_ERROR("Invalid write input.\n");
		ret = -EINVAL;
		goto error;
	}

	ret = copy_from_user(input, buffer, length);
	if (ret < 0) {
		DSP_ERROR("Unable to copy from user.\n");
		goto error;
	}
	input[length] = '\0';

	ret = kstrtoint(input, 10, &user_val);
	if (ret) {
		DSP_ERROR("Invalid write input.\n");
		ret = -EINVAL;
		goto error;
	}

	if (user_val > 1) {
		DSP_ERROR("Invalid alternative vector selection value > 1.\n");
		ret = -EINVAL;
		goto error;
	}

	if (!file->private_data) {
		DSP_ERROR("Invalid pointer to device structure.\n");
		ret = -EFAULT;
		goto error;
	}
	dsp_device = (struct dsp_device_t *)file->private_data;

	if (dspstate_dsp_id < DSP_Q6_0 || dspstate_dsp_id >= DSP_MAX) {
		DSP_ERROR("DSP ID %d not in range of available DSPs.\n",
			  dspstate_dsp_id);
		ret = -EINVAL;
		goto error;
	}

	ret = mutex_lock_interruptible(&dsp_device->dsp_device_lock);
	if (ret != 0) {
		DSP_ERROR("Failed to lock DSP Loader device.\n");
		goto error;
	}

	if (list_empty(&dsp_device->dsp_list)) {
		DSP_ERROR("DSP_Loader DSP list is empty.\n");
		mutex_unlock(&dsp_device->dsp_device_lock);
		ret = -EFAULT;
		goto error;
	}

	list_for_each(pos, &dsp_device->dsp_list) {
		dsp_resource = list_entry(pos, struct dsp_resources_t, list);
		if (dsp_resource->dsp_id == dspstate_dsp_id)
			break;
		if (list_is_last(pos, &dsp_device->dsp_list)) {
			DSP_ERROR("DSP_Loader DSP not registered.\n");
			mutex_unlock(&dsp_device->dsp_device_lock);
			ret = -EINVAL;
			goto error;
		}
	}
	mutex_unlock(&dsp_device->dsp_device_lock);

	ret = mutex_lock_interruptible(&dsp_resource->dsp_resource_lock);
	if (ret != 0) {
		DSP_ERROR("Failed to lock DSP.\n");
		goto error;
	}

	if (!dsp_resource->states) {
		DSP_ERROR("Invalid pointer to state structure.\n");
		ret = -EFAULT;
		goto post_lock_error;
	}
	dsp_states = dsp_resource->states;
	ret = dsp_helper_write_stat_vector_sel(dsp_states,
					       user_val);
	DSP_DBG("DSP debugfs alternative vector selection set to %d.\n",
		user_val);
	ret = length;

post_lock_error:
	mutex_unlock(&dsp_resource->dsp_resource_lock);
error:
	return ret;
}

static ssize_t dsp_debugfs_rd_dsp_pwrgate(struct file *file,
					  char __user *buffer,
					  size_t length,
					  loff_t *offset)
{
	struct dsp_device_t *dsp_device;
	struct dsp_resources_t *dsp_resource = NULL;
	struct dsp_state_t *dsp_states;
	struct list_head *pos;
	int shift = 0;
	int ret;
	int status;

	if (dsp_debugfs_idx < 0 && *offset > 0)
		return 0;

	if (!file->private_data) {
		DSP_ERROR("Invalid pointer to device structure.\n");
		ret = -EFAULT;
		goto error;
	}
	dsp_device = (struct dsp_device_t *)file->private_data;

	if (dspstate_dsp_id < DSP_Q6_0 || dspstate_dsp_id >= DSP_MAX) {
		DSP_ERROR("DSP ID %d not in range of available DSPs.\n",
			  dspstate_dsp_id);
		ret = -EINVAL;
		goto error;
	}

	ret = mutex_lock_interruptible(&dsp_device->dsp_device_lock);
	if (ret != 0) {
		DSP_ERROR("Failed to lock DSP Loader device.\n");
		goto error;
	}

	if (list_empty(&dsp_device->dsp_list)) {
		DSP_ERROR("DSP_Loader DSP list is empty.\n");
		mutex_unlock(&dsp_device->dsp_device_lock);
		ret = -EFAULT;
		goto error;
	}

	list_for_each(pos, &dsp_device->dsp_list) {
		dsp_resource = list_entry(pos, struct dsp_resources_t, list);
		if (dsp_resource->dsp_id == dspstate_dsp_id)
			break;
		if (list_is_last(pos, &dsp_device->dsp_list)) {
			DSP_ERROR("DSP_Loader DSP not registered.\n");
			mutex_unlock(&dsp_device->dsp_device_lock);
			ret = -EINVAL;
			goto error;
		}
	}
	mutex_unlock(&dsp_device->dsp_device_lock);

	ret = mutex_lock_interruptible(&dsp_resource->dsp_resource_lock);
	if (ret != 0) {
		DSP_ERROR("Failed to lock DSP.\n");
		goto error;
	}

	if (!dsp_resource->states) {
		DSP_ERROR("Invalid pointer to state structure.\n");
		ret = -EFAULT;
		goto post_lock_error;
	}
	dsp_states = dsp_resource->states;

	status = pgfsm_helper_get_status(dsp_states->pgfsm_register,
					 dspstate_dsp_id);

	ret = dsp_user_copy(&shift, buffer, length, &dsp_splice_idx,
			    "DSP ID: %d with PGFSM status %d\n",
			    dspstate_dsp_id, status);
	if (ret < 0)
		goto post_lock_error;
	dsp_debugfs_idx = -1;
	mutex_unlock(&dsp_resource->dsp_resource_lock);
	*offset += shift;
	return shift;

post_lock_error:
	mutex_unlock(&dsp_resource->dsp_resource_lock);
error:
	return ret;
}

static ssize_t dsp_debugfs_wr_dsp_pwrgate(struct file *file,
					  const char __user *buffer,
					  size_t length, loff_t *offset)
{
	struct dsp_device_t *dsp_device;
	struct dsp_resources_t *dsp_resource = NULL;
	struct dsp_state_t *dsp_states;
	struct list_head *pos;
	char input[32];
	int dsp_pwr_val;
	int dsp_mode;
	int ret;

	if (length >= sizeof(input)) {
		DSP_ERROR("Invalid write input.\n");
		ret = -EINVAL;
		goto error;
	}

	ret = copy_from_user(input, buffer, length);
	if (ret < 0) {
		DSP_ERROR("Unable to copy from user.\n");
		goto error;
	}
	input[length] = '\0';

	ret = kstrtoint(input, 10, &dsp_pwr_val);
	if (ret) {
		DSP_ERROR("Invalid write input.\n");
		ret = -EINVAL;
		goto error;
	}

	if (dsp_pwr_val > 1) {
		DSP_ERROR("Invalid DSP pwr input > 1.\n");
		ret = -EINVAL;
		goto error;
	}

	if (!file->private_data) {
		DSP_ERROR("Invalid pointer to device structure.\n");
		ret = -EFAULT;
		goto error;
	}
	dsp_device = (struct dsp_device_t *)file->private_data;

	if (dspstate_dsp_id < DSP_Q6_0 || dspstate_dsp_id >= DSP_MAX) {
		DSP_ERROR("DSP ID %d not in range of available DSPs.\n",
			  dspstate_dsp_id);
		ret = -EINVAL;
		goto error;
	}

	ret = mutex_lock_interruptible(&dsp_device->dsp_device_lock);
	if (ret != 0) {
		DSP_ERROR("Failed to lock DSP Loader device.\n");
		goto error;
	}

	if (list_empty(&dsp_device->dsp_list)) {
		DSP_ERROR("DSP_Loader DSP list is empty.\n");
		mutex_unlock(&dsp_device->dsp_device_lock);
		ret = -EFAULT;
		goto error;
	}

	list_for_each(pos, &dsp_device->dsp_list) {
		dsp_resource = list_entry(pos, struct dsp_resources_t, list);
		if (dsp_resource->dsp_id == dspstate_dsp_id)
			break;
		if (list_is_last(pos, &dsp_device->dsp_list)) {
			DSP_ERROR("DSP_Loader DSP not registered.\n");
			mutex_unlock(&dsp_device->dsp_device_lock);
			ret = -EINVAL;
			goto error;
		}
	}
	mutex_unlock(&dsp_device->dsp_device_lock);

	ret = mutex_lock_interruptible(&dsp_resource->dsp_resource_lock);
	if (ret != 0) {
		DSP_ERROR("Failed to lock DSP.\n");
		goto error;
	}

	if (!dsp_resource->states) {
		DSP_ERROR("Invalid pointer to state structure.\n");
		ret = -EFAULT;
		goto post_lock_error;
	}

	dsp_states = dsp_resource->states;
	dsp_mode = dsp_helper_read(dsp_states);

	if (dsp_mode != DSP_OFF) {
		DSP_ERROR("DSP %d is not in OFF state\n", dspstate_dsp_id);
		ret = -EINVAL;
		goto post_lock_error;
	}

	pgfsm_helper_power(dsp_states->pgfsm_register, dspstate_dsp_id,
			   dsp_pwr_val);

	ret = length;

post_lock_error:
	mutex_unlock(&dsp_resource->dsp_resource_lock);
error:
	return ret;
}

static ssize_t dsp_debugfs_wr_dspctrl(struct file *file,
				      const char __user *buffer,
				      size_t length, loff_t *offset)
{
	struct dsp_device_t *dsp_device;
	struct dsp_resources_t *dsp_resource = NULL;
	struct dsp_state_t *dsp_states;
	struct list_head *pos;
	char input[32];
	int dsp_id;
	int dsp_mode;
	int ret;

	if (length >= sizeof(input)) {
		DSP_ERROR("Invalid write input.\n");
		ret = -EINVAL;
		goto error;
	}

	ret = copy_from_user(input, buffer, length);
	if (ret < 0) {
		DSP_ERROR("Unable to copy from user.\n");
		goto error;
	}
	input[length] = '\0';

	ret = sscanf(input, "%d %d", &dsp_id, &dsp_mode);
	if (ret != 2) {
		DSP_ERROR("Invalid write input.\n");
		ret = -EINVAL;
		goto error;
	}

	if (!file->private_data) {
		DSP_ERROR("Invalid pointer to device structure.\n");
		ret = -EFAULT;
		goto error;
	}
	dsp_device = (struct dsp_device_t *)file->private_data;

	if (dsp_id < DSP_Q6_0 || dsp_id >= DSP_MAX) {
		DSP_ERROR("DSP ID %d not in range of available DSPs.\n",
			  dsp_id);
		ret = -EINVAL;
		goto error;
	}

	ret = mutex_lock_interruptible(&dsp_device->dsp_device_lock);
	if (ret != 0) {
		DSP_ERROR("Failed to lock DSP Loader device.\n");
		goto error;
	}

	if (list_empty(&dsp_device->dsp_list)) {
		DSP_ERROR("DSP_Loader DSP list is empty.\n");
		mutex_unlock(&dsp_device->dsp_device_lock);
		ret = -EFAULT;
		goto error;
	}

	list_for_each(pos, &dsp_device->dsp_list) {
		dsp_resource = list_entry(pos, struct dsp_resources_t, list);
		if (dsp_resource->dsp_id == dsp_id)
			break;
		if (list_is_last(pos, &dsp_device->dsp_list)) {
			DSP_ERROR("DSP_Loader DSP not registered.\n");
			mutex_unlock(&dsp_device->dsp_device_lock);
			ret = -EINVAL;
			goto error;
		}
	}
	mutex_unlock(&dsp_device->dsp_device_lock);

	ret = mutex_lock_interruptible(&dsp_resource->dsp_resource_lock);
	if (ret != 0) {
		DSP_ERROR("Failed to lock DSP.\n");
		goto error;
	}

	if (!dsp_resource->states) {
		DSP_ERROR("Invalid pointer to state structure.\n");
		ret = -EFAULT;
		goto post_lock_error;
	}
	dsp_states = dsp_resource->states;
	dsp_states->submit_mode = dsp_mode;
	if (!dsp_states->clk) {
		dsp_states->clk = helper_get_clk(dsp_resource->dsp_id);
		if (IS_ERR_OR_NULL(dsp_states->clk)) {
			DSP_ERROR("Failed to get dsp%d clock.\n", dsp_resource->dsp_id);
			dsp_states->clk = NULL;
		}
	}
	dsp_resource->states->bypass_pd = helper_get_pdbypass(dsp_resource->dsp_id);

	switch (dsp_mode) {
	case DSP_OFF:
		ret = dsp_helper_off(dsp_device, dsp_resource);
		break;
	case DSP_STOP:
		ret = dsp_helper_stop(dsp_device, dsp_states, dsp_id);
		break;
	case DSP_PAUSE:
		ret = dsp_helper_pause(dsp_device, dsp_states, dsp_id);
		break;
	case DSP_RUN:
		ret = dsp_helper_run(dsp_device, dsp_states, dsp_id);
		break;
	default:
		DSP_ERROR("Invalid DSP mode.\n");
		ret = -EINVAL;
		goto post_lock_error;
	}
	DSP_DBG("DSP debugfs dsp state set to %d.\n", dsp_mode);
	ret = length;

post_lock_error:
	mutex_unlock(&dsp_resource->dsp_resource_lock);
error:
	return ret;
}

int dsp_debugfs_add(struct dsp_device_t *dsp_device)
{
	struct dentry *check;
	int ret;

	check = debugfs_create_dir("dsp_loader", NULL);
	if (!check) {
		DSP_ERROR("Failed to create debug directory.\n");
		ret = PTR_ERR(check);
		goto error;
	}
	dsp_device->debug_dir = check;

	dspalloc_dsp_id = -1;
	check = debugfs_create_file("dspalloc", 0600,
				    dsp_device->debug_dir,
				    dsp_device,
				    &dsp_debug_dspalloc_fops);
	if (!check) {
		DSP_ERROR("Failed to create dsp memory debug file.\n");
		ret = PTR_ERR(check);
		goto error;
	}

	check = debugfs_create_file("dspclient", 0400,
				    dsp_device->debug_dir,
				    dsp_device,
				    &dsp_debug_dspclient_fops);
	if (!check) {
		DSP_ERROR("Failed to create clients debug file.\n");
		ret = PTR_ERR(check);
		goto error;
	}

	dspstate_dsp_id = -1;
	check = debugfs_create_file("dspstate", 0600,
				    dsp_device->debug_dir,
				    dsp_device,
				    &dsp_debug_dspstate_fops);
	if (!check) {
		DSP_ERROR("Failed to create dsp state debug file.\n");
		ret = PTR_ERR(check);
		goto error;
	}

	check = debugfs_create_file("dspclkgate", 0600,
				    dsp_device->debug_dir,
				    dsp_device,
				    &dsp_debug_dspclkgate_fops);
	if (!check) {
		DSP_ERROR("Failed to create dsp clkgate debug file.\n");
		ret = PTR_ERR(check);
		goto error;
	}

	check = debugfs_create_file("stat_vector_sel", 0600,
				    dsp_device->debug_dir,
				    dsp_device,
				    &dsp_debug_stat_vector_sel_fops);
	if (!check) {
		DSP_ERROR("Failed to create dsp stat_vector_sel file.\n");
		ret = PTR_ERR(check);
		goto error;
	}

	check = debugfs_create_file("dsp_power", 0600,
				    dsp_device->debug_dir,
				    dsp_device,
				    &dsp_debug_dsp_pwrgate_fops);
	if (!check) {
		DSP_ERROR("Failed to create dsp clkgate debug file.\n");
		ret = PTR_ERR(check);
		goto error;
	}

	check = debugfs_create_file("dspctrl", 0200,
				    dsp_device->debug_dir,
				    dsp_device,
				    &dsp_debug_dspctrl_fops);
	if (!check) {
		DSP_ERROR("Failed to create dsp control debug file.\n");
		ret = PTR_ERR(check);
		goto error;
	}

	check = debugfs_create_file("dspdpm", 0200,
				    dsp_device->debug_dir,
				    dsp_device,
				    &dsp_debug_dspdpm_fops);
	if (!check) {
		DSP_ERROR("Failed to create dsp clk file.\n");
		ret = PTR_ERR(check);
		goto error;
	}
	return 0;
error:
	debugfs_remove_recursive(dsp_device->debug_dir);
	return ret;
}

void dsp_debugfs_rm(struct dsp_device_t *dsp_device)
{
	debugfs_remove_recursive(dsp_device->debug_dir);
}
