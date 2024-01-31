/*
 * Copyright (C) 2019 Advanced Micro Devices, Inc. All rights reserved.
 */

#include <linux/debugfs.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <stdarg.h>
#include <linux/uaccess.h>
#include <linux/cvip_event_logger.h>
#include "cvip_event_logger_internal.h"

static int cvip_debugfs_open(struct inode *inode, struct file *file);

static ssize_t cvip_debugfs_rd_eventlogs(struct file *file, char __user *buffer,
					 size_t length, loff_t *offset);

static ssize_t cvip_debugfs_wr_eventlogs(struct file *file,
					 const char __user *buffer,
					 size_t length, loff_t *offset);

static const struct file_operations dsp_debug_dspevents_fops = {
	.open = cvip_debugfs_open,
	.read = cvip_debugfs_rd_eventlogs,
	.write = cvip_debugfs_wr_eventlogs,
};

static int splice_idx;
static int cvipevents_log;
static int cvipevents_mode;
static int cvipevents_idx;

static int cvip_debugfs_open(struct inode *inode, struct file *file)
{
	file->private_data = inode->i_private;
	return 0;
}

static int cvip_user_copy(int *shift, char __user *buffer,
			  size_t length, char *fmt, ...)
{
	char output[512];
	int output_size;
	va_list argptr;

	va_start(argptr, fmt);
	output_size = vsnprintf(output, sizeof(output), fmt, argptr);
	va_end(argptr);

	if (output_size < 0)
		return -ENOMEM;

	if ((*shift + output_size - splice_idx) > length) {
		CVIP_LOGGER_DBG("Larger than buffer, splicing.\n");
		output_size = length - *shift;
		if (copy_to_user(buffer + *shift, output + splice_idx,
				 output_size)) {
			CVIP_LOGGER_ERROR("Failed to read event to user.\n");
			return -EFAULT;
		}
		splice_idx += output_size;
		*shift += output_size;
		return 1;
	}
	if (copy_to_user(buffer + *shift, output + splice_idx,
			 output_size - splice_idx)) {
		CVIP_LOGGER_ERROR("Failed to read event to user.\n");
		return -EFAULT;
	}

	*shift += (output_size - splice_idx);
	splice_idx = 0;
	return 0;
}

static int cvip_entry_send(struct cvip_log_entry_t *entry, int *shift,
			   char __user *buffer, size_t length)
{
	char output[512];
	int output_size;
	char temp[512];
	int temp_size;
	int i;

	output_size = snprintf(output, sizeof(output),
			       "[%15ld] %30s@%-5d:%-50s |", entry->time,
			       entry->func_name, entry->line_num,
			       entry->fmt_string);
	if (output_size < 0)
		return -ENOMEM;

	for (i = 0; i < entry->entry_cnt; i++) {
		temp_size = snprintf(temp, sizeof(temp), " %-9x",
				     entry->log_data[i]);
		if (temp_size < 0)
			return -ENOMEM;
		if ((output_size + temp_size + 1) >
			sizeof(output)) {
			return -ENOMEM;
		}
		strcat(output, temp);
		output_size += temp_size;
	}

	temp_size = snprintf(temp, sizeof(temp), "\n");
	if (temp_size < 0)
		return -ENOMEM;
	if ((output_size + temp_size + 1) > sizeof(output))
		return -ENOMEM;
	strcat(output, temp);
	output_size += temp_size;

	if ((*shift + output_size - splice_idx) > length) {
		CVIP_LOGGER_DBG("Larger than buffer, splicing.\n");
		output_size = length - *shift;
		if (copy_to_user(buffer + *shift, output + splice_idx,
				 output_size)) {
			CVIP_LOGGER_ERROR("Failed to read event to user.\n");
			return -EFAULT;
		}
		splice_idx += output_size;
		*shift += output_size;
		return 1;
	}
	if (copy_to_user(buffer + *shift, output + splice_idx,
			 output_size - splice_idx)) {
		CVIP_LOGGER_ERROR("Failed to read event to user.\n");
		return -EFAULT;
	}

	*shift += (output_size - splice_idx);
	splice_idx = 0;
	return 0;
}

static void cvip_debugfs_dump_log(struct cvip_log_t *event_log)
{
	struct cvip_log_entry_t *entry;
	char output[512];
	char temp[512];
	int i;
	int ret;
	int idx;

	if (event_log->log_cnt == 0)
		return;

	CVIP_LOGGER_DBG("[%15s] %30s@%-5s:%-50s | %s\n", "Time", "Function",
			"Line", "Debug Text", "Logged data");

	if (event_log->log_cnt > event_log->log_size)
		CVIP_LOGGER_DBG("Buffer wrapped: %d %s\n",
				(event_log->log_cnt - event_log->log_size),
				"entries lost since dump");

	idx = event_log->log_cnt < event_log->log_size ? 0 : event_log->log_idx;

	do {
		entry = &event_log->entry[idx];
		ret = snprintf(output, sizeof(output),
			       "[%15ld] %30s@%-5d:%-50s |", entry->time,
			       entry->func_name, entry->line_num,
			       entry->fmt_string);
		if (ret < 0) {
			CVIP_LOGGER_ERROR("Failed to format string.\n");
			return;
		}
		for (i = 0; i < entry->entry_cnt; i++) {
			ret = snprintf(temp, sizeof(temp), " %-9x",
				       entry->log_data[i]);
			if (ret < 0) {
				CVIP_LOGGER_ERROR("Failed to format string.\n");
				return;
			}
			if ((strlen(output) + ret + 1) > sizeof(output)) {
				CVIP_LOGGER_ERROR("Output buffer too small.\n");
				return;
			}
			strcat(output, temp);
		}
		CVIP_LOGGER_DBG("%s\n", output);

		idx = (idx + 1) & event_log->log_mask;
	} while (idx != event_log->log_idx);
}

static ssize_t cvip_debugfs_rd_eventlogs(struct file *file, char __user *buffer,
					 size_t length, loff_t *offset)
{
	struct cvip_log_t *event_log;
	struct cvip_log_entry_t *entry;
	struct cvip_log_database_t *event_database;
	int shift = 0;
	int ret;

	if (!file->private_data) {
		CVIP_LOGGER_ERROR("Invalid pointer to device structure.\n");
		ret = -EFAULT;
		goto error;
	}
	event_database = (struct cvip_log_database_t *)file->private_data;

	if (cvipevents_idx < 0 && *offset > 0)
		return 0;

	ret = mutex_lock_interruptible(&event_database->database_lock);
	if (ret) {
		CVIP_LOGGER_ERROR("Failed to lock database.\n");
		goto error;
	}

	if (!event_database->db_cnt) {
		ret = cvip_user_copy(&shift, buffer, length,
				     "No logs in database.\n");
		if (ret < 0)
			goto post_lock_error;
		cvipevents_idx = -1;
		goto exit;
	}

	if (!cvipevents_log) {
		if (!*offset) {
			ret = cvip_user_copy(&shift, buffer, length,
					     "Logs to choose from:\n");
			if (ret < 0)
				goto post_lock_error;
			if (ret > 0)
				goto exit;
			cvipevents_idx = 0;
		}

		while (cvipevents_idx < event_database->db_cnt) {
			event_log = event_database->log_list[cvipevents_idx];
			if (!event_log) {
				cvipevents_idx += 1;
				continue;
			}
			ret = cvip_user_copy(&shift, buffer, length,
					     "[%d] %s\n", cvipevents_idx + 1,
					     event_log->file_name);
			if (ret < 0)
				goto post_lock_error;
			if (ret > 0)
				goto exit;
			cvipevents_idx += 1;
		}
		cvipevents_idx = -1;
		goto exit;
	}

	if (cvipevents_log > event_database->db_cnt) {
		CVIP_LOGGER_ERROR("Log not in database.\n");
		ret = -EINVAL;
		goto post_lock_error;
	}

	event_log = event_database->log_list[cvipevents_log - 1];
	if (!event_log) {
		CVIP_LOGGER_ERROR("Log not in database.\n");
		ret = -EINVAL;
		goto post_lock_error;
	}

	if (!cvipevents_mode) {
		if (!event_log->log_cnt) {
			ret = cvip_user_copy(&shift, buffer, length,
					     "Log Empty");
			if (ret < 0)
				goto post_lock_error;
			cvipevents_idx = -1;
			goto exit;
		}
		if (!*offset) {
			cvipevents_idx =
				event_log->log_cnt < event_log->log_size ?
				0 : event_log->log_idx;

			ret = cvip_user_copy(&shift, buffer, length,
					     "[%15s] %30s@%-5s:%-50s | %s\n",
					     "Time", "Function", "Line",
					     "Debug Text", "Logged data");
			if (ret < 0)
				goto post_lock_error;
			if (ret > 0)
				goto exit;

			if (event_log->log_cnt > event_log->log_size) {
				ret = cvip_user_copy(&shift, buffer, length,
						     "Buffer wrapped: %d %s\n",
						     (event_log->log_cnt -
						      event_log->log_size),
						     "entries lost since dump");
				if (ret < 0)
					goto post_lock_error;
				if (ret > 0)
					goto exit;
			}
		}
		do {
			entry = &event_log->entry[cvipevents_idx];
			ret = cvip_entry_send(entry, &shift, buffer, length);
			if (ret < 0)
				goto post_lock_error;
			if (ret > 0)
				goto exit;
			cvipevents_idx =
				(cvipevents_idx + 1) & event_log->log_mask;
		} while (cvipevents_idx != event_log->log_idx);
		cvipevents_idx = -1;
	} else {
		cvip_debugfs_dump_log(event_log);
	}
exit:
	mutex_unlock(&event_database->database_lock);
	*offset += shift;
	return shift;

post_lock_error:
	mutex_unlock(&event_database->database_lock);
error:
	return ret;
}

static ssize_t cvip_debugfs_wr_eventlogs(struct file *file,
					 const char __user *buffer,
					 size_t length, loff_t *offset)
{
	struct cvip_log_database_t *event_database;
	char input[32];
	int log_flag;
	int mode_flag;
	int ret;

	if (!file->private_data) {
		CVIP_LOGGER_ERROR("Invalid pointer to device structure.\n");
		ret = -EFAULT;
		goto error;
	}
	event_database = (struct cvip_log_database_t *)file->private_data;

	if (length >= sizeof(input)) {
		CVIP_LOGGER_ERROR("Invalid write input.\n");
		ret = -EINVAL;
		goto error;
	}

	ret = copy_from_user(input, buffer, length);
	if (ret < 0) {
		CVIP_LOGGER_ERROR("Unable to copy from user.\n");
		goto error;
	}
	input[length] = '\0';

	ret = sscanf(input, "%d %d", &log_flag, &mode_flag);
	if (ret < 1 || ret > 2) {
		CVIP_LOGGER_ERROR("Invalid write input.\n");
		ret = -EINVAL;
		goto error;
	}

	if (log_flag < 0 || log_flag > event_database->db_cnt) {
		CVIP_LOGGER_ERROR("Incorrect flag for cvip event log %d.\n",
				  log_flag);
		ret = -EINVAL;
		goto error;
	}
	cvipevents_log = log_flag;
	CVIP_LOGGER_DBG("CVIP event log flag set to %d.\n",
			cvipevents_log);

	if (ret == 2) {
		if (mode_flag < 0 || mode_flag > 1) {
			CVIP_LOGGER_ERROR("Incorrect flag for cvip mode %d.\n",
					  log_flag);
			ret = -EINVAL;
			goto error;
		}
		cvipevents_mode = mode_flag;
		CVIP_LOGGER_DBG("CVIP event mode flag set to %d.\n",
				cvipevents_mode);
	} else {
		cvipevents_mode = 0;
	}

	return length;
error:
	return ret;
}

int cvip_logger_debugfs_add(struct cvip_log_database_t *event_database)
{
	struct dentry *check;
	int ret;

	check = debugfs_create_dir("cvip_event_logger", NULL);
	if (!check) {
		CVIP_LOGGER_ERROR("Failed to create debug directory.\n");
		ret = PTR_ERR(check);
		goto error;
	}
	event_database->debug_dir = check;

	splice_idx = 0;
	cvipevents_log = 0;
	cvipevents_mode = 0;
	cvipevents_idx = -1;
	check = debugfs_create_file("cvipeventlog", 0600,
				    event_database->debug_dir,
				    event_database,
				    &dsp_debug_dspevents_fops);
	if (!check) {
		CVIP_LOGGER_ERROR("Failed to create cvip logger debug file.\n");
		ret = PTR_ERR(check);
		goto error;
	}

	return 0;
error:
	debugfs_remove_recursive(event_database->debug_dir);
	return ret;
}

void cvip_logger_debugfs_rm(struct cvip_log_database_t *event_database)
{
	debugfs_remove_recursive(event_database->debug_dir);
}
