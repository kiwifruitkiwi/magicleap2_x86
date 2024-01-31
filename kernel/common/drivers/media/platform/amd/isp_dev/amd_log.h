/*
 * Copyright (C) 2019-2020 Advanced Micro Devices, Inc. All rights reserved.
 */

#ifndef __CAM_LOG_H_
#define __CAM_LOG_H_

#include "isp_common.h"

/*
 * default log switch
 * if you want enable or disable specific debug level
 * re-define it in your file
 */
#ifndef LOG_ERR
#define LOG_ERR		1
#endif

#ifndef LOG_WARN
#define LOG_WARN	0
#endif

#ifndef LOG_INFO
#define LOG_INFO	0
#endif

#ifndef LOG_DBG
#define LOG_DBG		0
#endif

extern void isp_dbg_print(uint32 trace_events_level, uint32 trace_events_flag,
	char *debug_message, ...);

#if LOG_DBG
#define logd(fmt, ...) pr_info(fmt, ##__VA_ARGS__)
#else
#define logd(fmt, ...)
#endif

#if LOG_INFO
#define logi(fmt, ...) pr_info(fmt, ##__VA_ARGS__)
#else
#define logi(fmt, ...)
#endif

#if LOG_WARN
#define logw(fmt, ...) pr_warning(fmt, ##__VA_ARGS__)
#else
#define logw(fmt, ...)
#endif

#if LOG_ERR
#define loge(fmt, ...) pr_err(fmt, ##__VA_ARGS__)
#else
#define loge(fmt, ...)
#endif

#define assert(x)		\
	do {			\
		if (!(x))	\
			loge("!!!ASSERT ERROR: " #x "in %s, %d !!!", \
				__func__, __LINE__); \
	} while (0)
/*
 *#define return_if_assert_fail(x)	\
 *	do {				\
 *		if (!(x))		\
 *			loge("!!!ASSERT ERROR:" #x "in %s, %d !!!", \
 *				__func__, __LINE__); \
 *		return;			\
 *	} while (0)
 *
 *#define return_val_if_assert_fail(x, val)	\
 *	do {					\
 *		if (!(x))			\
 *			loge("!!!ASSERT ERROR:" #x "in %s, %d !!!", \
 *				__func__, __LINE__); \
 *		return val;			\
 *	} while (0)
 */
#endif /* __CAM_LOG_H_ */
