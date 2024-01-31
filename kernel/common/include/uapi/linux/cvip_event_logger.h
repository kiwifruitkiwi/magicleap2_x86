/*
 * Copyright (C) 2019-2020 Advanced Micro Devices, Inc. All rights reserved.
 */

#ifndef CVIP_EVENT_LOGGER
#define CVIP_EVENT_LOGGER

#define LOG_DATA_LIMITER -1
#define __FILENAME__ (__builtin_strrchr(__FILE__, '/') ? \
	__builtin_strrchr(__FILE__, '/') + 1 : __FILE__)

#ifdef CONFIG_CVIP_LOGGER

int cvip_event_log_init(const char *filename, int size);
int cvip_event_log(int log_idx, const char *func, int line,
		   const char *fmt, ...);
void cvip_event_log_dump(int log_idx);

#else

static inline int cvip_event_log_init(const char *filename, int size)
{
	return -1;
}

static inline int cvip_event_log(int log_idx, const char *func, int line,
				 const char *fmt, ...)
{
	return -1;
}

static inline void cvip_event_log_dump(int log_idx) {}

#endif

#define CVIP_EVTLOGINIT(size) \
	cvip_event_log_init(__FILENAME__, size)
#define CVIP_EVTLOG(log_idx, fmt, ...) \
	cvip_event_log(log_idx, __func__, __LINE__, fmt, \
		       ##__VA_ARGS__, LOG_DATA_LIMITER)

#endif
