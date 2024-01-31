/*
 * Copyright (C) 2019-2020 Advanced Micro Devices, Inc. All rights reserved.
 */

#ifndef CVIP_EVENT_LOGGER_INTERNAL
#define CVIP_EVENT_LOGGER_INTERNAL

#include <linux/mutex.h>
#include <linux/list.h>
#include <linux/spinlock.h>

#define CVIP_LOGGER_DBG(fmt, ...) \
	pr_debug("cvip_event_logger: %s: " fmt, __func__, ##__VA_ARGS__)
#define CVIP_LOGGER_ERROR(fmt, ...) \
	pr_err("cvip_event_logger: %s: " fmt, __func__, ##__VA_ARGS__)

#define MAX_LOG_SIZE_2K BIT(11)
#define MAX_DATA 16
#define LOG_SIZE 64

/*
 * struct cvip_log_entry_t
 *
 * A structure used to store a single logging entry in the event buffer.
 *
 * @time: A long containing the current time the log entry was made
 * @func_name: A string of the function name function which made the event
 * @line_num: An integer containing the line number
 * @fmt_string: A string with the event message
 * @log_data: An array of integers containing any recorded data
 * @entry_cnt: An integer to record up much data was logged in log_data
 */
struct cvip_log_entry_t {
	long time;
	const char *func_name;
	int line_num;
	const char *fmt_string;
	int log_data[MAX_DATA];
	int entry_cnt;
};

/*
 * struct cvip_log_t
 *
 * A structure used to store the information about a single event log
 *
 * @log_lock: A lock to prevent multiple access to one log structure
 * @file_name: A character pointer to the name of file that made the log
 * @entry: A pointer to an array of log event entries
 * @log_size: The amount of entries the log can contain
 * @log_mask: A mask for size of the log
 * @log_cnt: The current amount of entries in the log
 * @log_idx: The current write index for the log
 */
struct cvip_log_t {
	/* spinlock to protect access to the log_idx and log_cnt */
	spinlock_t log_lock;
	const char *file_name;
	struct cvip_log_entry_t *entry;
	int log_size;
	int log_mask;
	int log_cnt;
	int log_idx;
};

/*
 * struct cvip_log_database_t
 *
 * A structure used to store the database information for all of the event logs
 *
 * @database_lock: A lock to prevent multiple access to the database at a time
 * @log_list: An array of pointers to log structures.
 * @db_cnt: The amount of logs currently stored in the database
 * @debug_dir: An entry point to the debugfs directory
 */
struct cvip_log_database_t {
	/* Mutex lock to protect access to the database */
	struct mutex database_lock;
	struct cvip_log_t *log_list[LOG_SIZE];
	int db_cnt;
	struct dentry *debug_dir;
};

#ifdef CONFIG_DEBUG_FS
int cvip_logger_debugfs_add(struct cvip_log_database_t *event_database);
void cvip_logger_debugfs_rm(struct cvip_log_database_t *event_database);
#else
static inline int
	cvip_logger_debugfs_add(struct cvip_log_database_t *event_database)
{
	return 0;
}

static inline void
	cvip_logger_debugfs_rm(struct cvip_log_database_t *event_database) {}
#endif

#endif
