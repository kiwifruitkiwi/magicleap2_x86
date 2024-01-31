/* SPDX-License-Identifier: GPL-2.0-only
 *
 * Copyright (c) 2022, Magic Leap, Inc. All rights reserved.
 */

#pragma once

#ifndef __KERNEL__
    #error "Can't include kernel header in userspace"
#endif

enum {
    CDP_DESC_STAGE_0 = 0,
    CDP_DESC_STAGE_1,
    CDP_DESC_STAGE_2,
    CDP_DESC_GLOBAL,
    CDP_DESC_FULL,
    CDP_DESC_STAGE_MAX,
};

enum {
    CDP_WCAM_BPP_8 = 0,
    CDP_WCAM_BPP_10,
    CDP_WCAM_BPP_12,
    CDP_WCAM_BPP_16
};

#define CDP_IMAGE_N_ROWS (1016)
#define CDP_IMAGE_N_COLS (1016)
#define CDP_N_STAGES (3)
#define CDP_MAX_N_WINDOWS (400)
