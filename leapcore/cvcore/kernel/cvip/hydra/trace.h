/* SPDX-License-Identifier: GPL-2.0-only
 *
 * Copyright (c) 2022, Magic Leap, Inc. All rights reserved.
 */

#pragma once

#ifndef __KERNEL__
    #error "Can't include kernel header in userspace"
#endif

struct tr_args_intr {
    uint32_t vix, dix, ssix, bufix, tail, dlen, slice, xstat, subctr, uhand, fcnt, pass;
    uint16_t uid, uidr;
    unsigned long cpu, count;
};
