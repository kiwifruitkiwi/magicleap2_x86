/* SPDX-License-Identifier: GPL-2.0-only
 *
 * Copyright (c) 2020-2022, Magic Leap, Inc. All rights reserved.
 */

#pragma once

#ifndef __KERNEL__
    #error "Can't include kernel header in userspace"
#endif

typedef uint32_t CvVersion;

#define CV_VERSION(maj, min)    ((((maj) & 0x7fff) << 16) | (((min) & 0x7fff)))
