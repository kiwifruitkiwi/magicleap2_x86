/* SPDX-License-Identifier: GPL-2.0-only
 *
 * \file cvcore_xchannel_map.h
 *
 * \brief CVCore mero_xpc vchannel map
 *
 * Copyright (c) 2020-2022, Magic Leap, Inc. All rights reserved.
 */

#pragma once

#include <linux/types.h>

#define CVCORE_XCHANNEL_PLATFORM        (0x0U)
#define CVCORE_XCHANNEL_SHREGION        (0x1U)
#define CVCORE_XCHANNEL_C_NOTIFIER      (0x2U)
#define CVCORE_XCHANNEL_N_NOTIFIER      (0x3U)
#define CVCORE_XCHANNEL_DSPCORE_ASYNC   (0x4U)
#define CVCORE_XCHANNEL_DSPCORE_SYNCH   (0x5U)
#define CVCORE_XCHANNEL_DSPCORE_QUEUE   (0x6U)
#define CVCORE_XCHANNEL_MLNET           (0x7U)
#define CVCORE_XCHANNEL_PM              (0x8U)
#define CVCORE_XCHANNEL_DSP_CONTROL     (0x9U)
/* reserved for cvcore component validation
 * #define XPC_TEST_COMMAND_CHANNEL        (0xAU)
 */
#define CVCORE_XCHANNEL_MERO_SMMU       (0xBU)
#define CVCORE_XCHANNEL_FCLK            (0xCU)
#define CVCORE_XCHANNEL_TEST0           (0xEU)
#define CVCORE_XCHANNEL_TEST1           (0xFU)

#ifndef __KERNEL__
#error "Can't include kernel header in userspace"
#endif
