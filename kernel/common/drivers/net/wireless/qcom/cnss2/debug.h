/* SPDX-License-Identifier: GPL-2.0-only */
/* Copyright (c) 2016-2019, The Linux Foundation. All rights reserved. */

#ifndef _CNSS_DEBUG_H
#define _CNSS_DEBUG_H

#ifndef CONFIG_X86
#include <linux/ipc_logging.h>
#endif
#include <linux/printk.h>

#define CNSS_IPC_LOG_PAGES		32

extern void *cnss_ipc_log_context;
extern void *cnss_ipc_log_long_context;

#define pr_cnss_fmt(_fmt)  "cnss: " _fmt
#ifndef CONFIG_X86
#define cnss_ipc_log_string(_x...)					\
	ipc_log_string(cnss_ipc_log_context, _x)

#define cnss_ipc_log_long_string(_x...)					\
	ipc_log_string(cnss_ipc_log_long_context, _x)
#else
#define cnss_ipc_log_string(_x...) do {					\
	} while (0)
#define cnss_ipc_log_long_string(_x...) do {					\
	} while (0)
#endif

#define cnss_pr_err(_fmt, ...) do {					\
		pr_err(pr_cnss_fmt(_fmt), ##__VA_ARGS__);		\
		cnss_ipc_log_string("%scnss: " _fmt, "", ##__VA_ARGS__);\
	} while (0)

#define cnss_pr_warn(_fmt, ...) do {					\
		pr_warn(pr_cnss_fmt(_fmt), ##__VA_ARGS__);		\
		cnss_ipc_log_string("%scnss: " _fmt, "", ##__VA_ARGS__);\
	} while (0)

#define cnss_pr_info(_fmt, ...) do {					\
		pr_info(pr_cnss_fmt(_fmt), ##__VA_ARGS__);		\
		cnss_ipc_log_string("%scnss: " _fmt, "", ##__VA_ARGS__);\
	} while (0)

#define cnss_pr_dbg(_fmt, ...) do {					\
		pr_debug(pr_cnss_fmt(_fmt), ##__VA_ARGS__);		\
		cnss_ipc_log_string("%scnss: " _fmt, "", ##__VA_ARGS__);\
	} while (0)

#define cnss_pr_vdbg(_fmt, ...) do {					\
		pr_debug(pr_cnss_fmt(_fmt), ##__VA_ARGS__);		\
		cnss_ipc_log_long_string("%scnss: " _fmt, "",		\
					 ##__VA_ARGS__);		\
	} while (0)

#ifdef CONFIG_CNSS2_DEBUG
#define CNSS_ASSERT(_condition) do {					\
		if (!(_condition)) {					\
			cnss_pr_err("ASSERT at line %d\n",		\
				    __LINE__);				\
			BUG();						\
		}							\
	} while (0)
#else
#define CNSS_ASSERT(_condition) do {					\
		if (!(_condition)) {					\
			cnss_pr_err("ASSERT at line %d\n",		\
				    __LINE__);				\
			WARN_ON(1);					\
		}							\
	} while (0)
#endif

#define cnss_fatal_err(_fmt, ...)					\
	cnss_pr_err("fatal: " _fmt, ##__VA_ARGS__)

int cnss_debug_init(void);
void cnss_debug_deinit(void);
int cnss_debugfs_create(struct cnss_plat_data *plat_priv);
void cnss_debugfs_destroy(struct cnss_plat_data *plat_priv);

#endif /* _CNSS_DEBUG_H */
