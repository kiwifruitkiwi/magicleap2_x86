/* SPDX-License-Identifier: GPL-2.0-only
 *
 * Copyright (c) 2020-2023, Magic Leap, Inc. All rights reserved.
 */

#pragma once

#include <linux/types.h>

// cvcore_name header is not supported on kernel.
// So, using CVCORE_NAME_STATIC which works when the name
// is statically hard-coded to a constant.
#define CVCORE_NAME_STATIC(x) CVCORE_NAME_ ## x

// Used in cvcore_trace
#define CVCORE_NAME_CVCORE_EASY_TRACER (0x590c5336c75600)

// Used in hydra-vcam
#define CVCORE_NAME_HSI_VCAM_INBUF0  (0x1995c000101995)
#define CVCORE_NAME_HSI_VCAM_INBUF1  (0x1995c000201995)
#define CVCORE_NAME_HSI_VCAM_INBUF2  (0x1995c000301995)
#define CVCORE_NAME_HSI_VCAM_INBUF3  (0x1995c000401995)
#define CVCORE_NAME_HSI_VCAM_INBUF4  (0x1995c000501995)
#define CVCORE_NAME_HSI_VCAM_INBUF5  (0x1995c000601995)
#define CVCORE_NAME_HSI_VCAM_INBUF6  (0x1995c000701995)
#define CVCORE_NAME_HSI_VCAM_INBUF7  (0x1995c000801995)

// Used in cdp
#define CVCORE_NAME_CDP_DRV_SHREGION_INPUT  (0x1995c74d551995)
#define CVCORE_NAME_CDP_DRV_SHREGION_OUTPUT (0x19983c74d551998)

// Used in shregin
#define CVCORE_NAME_CDP_DCPRS_KERNEL_TEST (0x5ef3c74d55e111)

#ifndef __KERNEL__
#error "Can't include kernel header in userspace"
#endif
