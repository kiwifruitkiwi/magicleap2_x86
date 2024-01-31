/* SPDX-License-Identifier: GPL-2.0-only
 *
 * \file cvcore_processor_common.h
 *
 * \brief CVCore DSP and ARM Common Headers and defines
 *
 * Copyright (c) 2019-2023, Magic Leap, Inc. All rights reserved.
 */

#pragma once

#include "cvcore_typedefs.h"

#define CVCORE_DSP_RENUMBER_C5Q6_TO_Q6C5(c) \
    ((c >= 8) ? c : (((c) - 0x2) & 0x7))

#define CVCORE_DSP_RENUMBER_Q6C5_TO_C5Q6(c) \
    ((c >= 8) ? c : (((c) + 0x2) & 0x7))

#define CVCORE_NUM_DSPS (8)

/**
 * CORE/Masters IDs CVIP masters
 */
#define CVCORE_MASTER_ID(x) \
  x(CVCORE_DSP_C5_0,    0, "C5_0") \
  x(CVCORE_DSP_C5_1,    1, "C5_1") \
  x(CVCORE_DSP_Q6_0,    2, "Q6_0") \
  x(CVCORE_DSP_Q6_1,    3, "Q6_1") \
  x(CVCORE_DSP_Q6_2,    4, "Q6_2") \
  x(CVCORE_DSP_Q6_3,    5, "Q6_3") \
  x(CVCORE_DSP_Q6_4,    6, "Q6_4") \
  x(CVCORE_DSP_Q6_5,    7, "Q6_5") \
  x(CVCORE_ARM,         8, "ARM") \
  x(CVCORE_X86,         9, "X86") \

enum cvcore_master_id {
    CVCORE_MASTER_ID(CVCORE_ENUM_DEF_EXT)
};

#define CVCORE_AXI_ID(x) \
  x(CVCORE_AXI_Q6_P0,    1, "Q6_0") \
  x(CVCORE_AXI_Q6_P1,    2, "Q6_1") \
  x(CVCORE_AXI_Q6_P2,    3, "Q6_2") \
  x(CVCORE_AXI_Q6_P3,    4, "Q6_3") \
  x(CVCORE_AXI_Q6_P4,    5, "Q6_4") \
  x(CVCORE_AXI_Q6_P5,    6, "Q6_5") \
  x(CVCORE_AXI_C5_P0,    7, "C5_0") \
  x(CVCORE_AXI_C5_P1,    8, "C5_1") \
  x(CVCORE_AXI_A55,      9, "ARM") \
  x(CVCORE_AXI_X86,     10, "X86") \

enum cvcore_axi_id {
    CVCORE_AXI_ID(CVCORE_ENUM_DEF_EXT)
};

#define CVCORE_DSP_ID_MIN CVCORE_DSP_C5_0
#define CVCORE_DSP_ID_MAX CVCORE_DSP_Q6_5
#define CVCORE_CORE_ID_MIN CVCORE_DSP_C5_0
#define CVCORE_CORE_ID_MAX CVCORE_X86

#ifndef __KERNEL__
#error "Can't include kernel header in userspace"
#endif
