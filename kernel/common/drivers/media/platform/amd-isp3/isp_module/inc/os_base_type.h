/*
 * Copyright (C) 2019-2020 Advanced Micro Devices, Inc. All rights reserved.
 */

#ifndef OS_BASE_TYPE_H
#define OS_BASE_TYPE_H

#include <linux/module.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/font.h>
#include <linux/mutex.h>
#include <linux/videodev2.h>
#include <linux/kthread.h>
#include <linux/freezer.h>
#include <linux/delay.h>
#include <linux/firmware.h>
#include <linux/platform_device.h>
#include <linux/string.h>
#include <linux/uaccess.h>
#include <linux/types.h>
#include <stddef.h>
#include <stdarg.h>
#include "isp_module_cfg.h"


#ifndef FALSE
#define FALSE	(0)
#endif

#ifndef TRUE
#define TRUE	(1)
#endif

#define IDMA_COPY_START_ADDRESS_ALIGNED_WITH   256
#define IDMA_COPY_SIZE_ALIGNED_WITH            64

#ifndef IDMA_COPY_ADDR_ALIGN
//Use these macros when allocating copy buffers to avoid sharing cache lines:
#define IDMA_COPY_ADDR_ALIGN \
	 __aligned(IDMA_COPY_START_ADDRESS_ALIGNED_WITH)
#endif
#ifndef IDMA_COPY_SIZE_ALIGN
//Buffer's size will be rounded to the cache line size so the buffer
//occupies the whole cache line
#define IDMA_COPY_SIZE_ALIGN(N) \
(((N) + IDMA_COPY_SIZE_ALIGNED_WITH - 1) & -IDMA_COPY_SIZE_ALIGNED_WITH)
#endif
#endif
