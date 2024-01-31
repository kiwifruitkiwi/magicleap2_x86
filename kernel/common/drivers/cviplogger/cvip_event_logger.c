/*
 * Copyright (C) 2019-2020 Advanced Micro Devices, Inc. All rights reserved.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <stdarg.h>
#include <linux/sched/clock.h>
#include <linux/cvip_event_logger.h>
#include "cvip_event_logger_internal.h"

static struct cvip_log_database_t event_database;

int cvip_event_log_init(const char *filename, int size)
{
	struct cvip_log_t *event_log;
	int size_align;
	int ret;

	if (event_database.db_cnt < 0 || event_database.db_cnt >= LOG_SIZE) {
		/*
		 * Could perform dynamic memory to maybe store the max size
		 * internally and alloc a new array when the first is filled,
		 * updating the max size to be double each time?
		 */
		CVIP_LOGGER_ERROR("Database full.\n");
		ret = -ENOMEM;
		goto error;
	}

	if (size <= 0 || size > MAX_LOG_SIZE_2K) {
		CVIP_LOGGER_ERROR("Size larger than maximum log size.\n");
		ret = -EINVAL;
		goto error;
	}

	if (!(size & !(size & (size - 1)))) {
		size_align = 1;
		while (size_align < size)
			size_align = size_align << 1;
	} else {
		size_align = size;
	}

	event_log = kzalloc(sizeof(*event_log), GFP_KERNEL);
	if (!event_log) {
		CVIP_LOGGER_ERROR("Failed to allocated log structure.\n");
		ret = -ENOMEM;
		goto error;
	}
	event_log->file_name = filename;
	event_log->entry =
		kcalloc(size_align, sizeof(*event_log->entry), GFP_KERNEL);
	if (!event_log->entry) {
		CVIP_LOGGER_ERROR("Failed to allocated log entry array.\n");
		ret = -ENOMEM;
		goto post_log_alloc_error;
	}
	event_log->log_size = size_align;
	event_log->log_mask = size_align - 1;
	event_log->log_cnt = 0;
	event_log->log_idx = 0;
	spin_lock_init(&event_log->log_lock);

	ret = mutex_lock_interruptible(&event_database.database_lock);
	if (ret) {
		CVIP_LOGGER_ERROR("Failed to lock database.\n");
		goto post_entry_alloc_error;
	}
	event_database.log_list[event_database.db_cnt] = event_log;
	ret = event_database.db_cnt++;
	mutex_unlock(&event_database.database_lock);
	return ret;

post_entry_alloc_error:
	kfree(event_log->entry);
post_log_alloc_error:
	kfree(event_log);
error:
	return ret;
}

int cvip_event_log(int logdb_idx, const char *func, int line,
		   const char *fmt, ...)
{
	struct cvip_log_t *event_log;
	struct cvip_log_entry_t *entry;
	unsigned long flags;
	int i;
	int data;
	va_list valist;

	if (logdb_idx < 0 || logdb_idx >= LOG_SIZE) {
		CVIP_LOGGER_ERROR("Invalid log index.\n");
		return -EINVAL;
	}

	event_log = event_database.log_list[logdb_idx];
	if (!event_log) {
		CVIP_LOGGER_ERROR("Log not initialized.\n");
		mutex_unlock(&event_database.database_lock);
		return -EFAULT;
	}

	spin_lock_irqsave(&event_log->log_lock, flags);
	entry = &event_log->entry[event_log->log_idx];
	event_log->log_cnt++;
	event_log->log_idx = (event_log->log_idx + 1) & event_log->log_mask;
	spin_unlock_irqrestore(&event_log->log_lock, flags);

	entry->time = local_clock();
	entry->func_name = func;
	entry->line_num = line;
	entry->fmt_string = fmt;

	va_start(valist, fmt);
	for (i = 0; i < MAX_DATA; i++) {
		data = va_arg(valist, int);
		if (data == LOG_DATA_LIMITER)
			break;
		entry->log_data[i] = data;
	}
	va_end(valist);
	entry->entry_cnt = i;
	return 0;
}

void cvip_event_log_dump(int logdb_idx)
{
	struct cvip_log_t *event_log;
	struct cvip_log_entry_t *entry;
	unsigned long flags;
	char output[512] = {0,};
	char temp[512] = {0,};
	int ret;
	int i;
	int event_idx;

	if (logdb_idx < 0 || logdb_idx >= LOG_SIZE) {
		CVIP_LOGGER_ERROR("invalid log index.\n");
		return;
	}

	ret = mutex_lock_interruptible(&event_database.database_lock);
	if (ret) {
		CVIP_LOGGER_ERROR("failed to lock database.\n");
		return;
	}

	if (logdb_idx >= event_database.db_cnt) {
		CVIP_LOGGER_ERROR("Invalid log index.\n");
		mutex_unlock(&event_database.database_lock);
		return;
	}

	event_log = event_database.log_list[logdb_idx];
	if (!event_log) {
		CVIP_LOGGER_ERROR("Invalid Log.\n");
		mutex_unlock(&event_database.database_lock);
		return;
	}

	mutex_unlock(&event_database.database_lock);

	spin_lock_irqsave(&event_log->log_lock, flags);
	if (event_log->log_cnt == 0)
		goto exit;

	CVIP_LOGGER_DBG("[%15s] %30s@%-5s:%-50s | %s\n", "Time", "Function",
			"Line", "Debug Text", "Logged data");

	if (event_log->log_cnt > event_log->log_size)
		CVIP_LOGGER_DBG("Buffer wrapped: %d %s\n",
				(event_log->log_cnt - event_log->log_size),
				"entries lost since dump");

	event_idx = event_log->log_cnt < event_log->log_size ?
		0 : event_log->log_idx;

	spin_unlock_irqrestore(&event_log->log_lock, flags);
	do {
		entry = &event_log->entry[event_idx];
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

		event_idx = (event_idx + 1) & event_log->log_mask;
	} while (event_idx != event_log->log_idx);

	spin_lock_irqsave(&event_log->log_lock, flags);
	event_log->log_cnt = 0;
	event_log->log_idx = 0;
exit:
	spin_unlock_irqrestore(&event_log->log_lock, flags);
}

void cvip_event_log_dump_all(void)
{
	struct cvip_log_t *event_log;
	struct cvip_log_entry_t *entry;
	unsigned long flags;
	char output[512] = {0,};
	char temp[512] = {0,};
	int ret;
	int i;
	int event_idx;

	ret = mutex_lock_interruptible(&event_database.database_lock);
	if (ret) {
		CVIP_LOGGER_ERROR("Failed to lock database.\n");
		return;
	}

	for (i = 0; i < event_database.db_cnt; i++) {
		event_log = event_database.log_list[i];
		if (!event_log)
			continue;

		spin_lock_irqsave(&event_log->log_lock, flags);

		if (event_log->log_cnt == 0)
			goto post_log_lock_error;

		CVIP_LOGGER_DBG("[%15s] %30s@%-5s:%-50s | %s\n", "Time",
				"Function", "Line", "Debug Text",
				"Logged data");

		if (event_log->log_cnt > event_log->log_size)
			CVIP_LOGGER_DBG("Buffer wrapped: %d %s\n",
					(event_log->log_cnt -
					 event_log->log_size),
					"entries lost since dump");

		event_idx = event_log->log_cnt < event_log->log_size ?
			0 : event_log->log_idx;
		spin_unlock_irqrestore(&event_log->log_lock, flags);

		do {
			entry = &event_log->entry[event_idx];
			ret = snprintf(output, sizeof(output),
				       "[%15ld] %30s@%-5d:%-50s |", entry->time,
				       entry->func_name, entry->line_num,
				       entry->fmt_string);
			if (ret < 0) {
				CVIP_LOGGER_ERROR("Failed to format string.\n");
				goto post_database_lock_error;
			}
			for (i = 0; i < entry->entry_cnt; i++) {
				ret = snprintf(temp, sizeof(temp), " %-9x",
					       entry->log_data[i]);
				if (ret < 0)
					goto post_database_lock_error;
				if ((strlen(output) + ret + 1)
				    > sizeof(output))
					goto post_database_lock_error;
				strcat(output, temp);
			}
			CVIP_LOGGER_DBG("%s\n", output);

			event_idx = (event_idx + 1) & event_log->log_mask;
		} while (event_idx != event_log->log_idx);
	}
	mutex_unlock(&event_database.database_lock);
	return;

post_log_lock_error:
	spin_unlock_irqrestore(&event_log->log_lock, flags);
post_database_lock_error:
	mutex_unlock(&event_database.database_lock);
}

static int __init cvip_logger_init(void)
{
	event_database.db_cnt = 0;
	mutex_init(&event_database.database_lock);
	cvip_logger_debugfs_add(&event_database);
	return 0;
}

static void __exit cvip_logger_exit(void)
{
	struct cvip_log_t *event_log;
	int i;
	int ret;

	cvip_logger_debugfs_rm(&event_database);

	ret = mutex_lock_interruptible(&event_database.database_lock);
	if (ret) {
		CVIP_LOGGER_ERROR("Failed to lock database.\n");
		return;
	}

	for (i = 0; i < event_database.db_cnt; i++) {
		event_log = event_database.log_list[i];
		if (!event_log)
			continue;
		if (ret) {
			CVIP_LOGGER_ERROR("Failed to lock log.\n");
			goto post_database_lock_error;
		}
		event_database.log_list[i] = NULL;
		kfree(event_log->entry);
		kfree(event_log);
	}
	mutex_destroy(&event_database.database_lock);

post_database_lock_error:
	mutex_unlock(&event_database.database_lock);
}

MODULE_DESCRIPTION("CVIP Event Logger");

subsys_initcall(cvip_logger_init);
module_exit(cvip_logger_exit);
