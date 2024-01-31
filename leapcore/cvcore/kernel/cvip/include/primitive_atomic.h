/* SPDX-License-Identifier: GPL-2.0-only
 *
 * \file primitive_atomic.h
 *
 * \brief  Definition of the GSMi C API.
 *
 * Copyright (c) 2020-2022, Magic Leap, Inc. All rights reserved.
 */

#ifndef GSM_PRIMITIVE_ATOMIC_H
#define GSM_PRIMITIVE_ATOMIC_H

#include "gsm.h"

typedef uint32_t primitive_atomic_t;

// One of the few calls that can fail is find and set first.
// Both the 32 bit and 128 bit versions return this value if there
// are no 0 bits found.  Otherwise, they return the index of the bit
// set.
#define PRIMITIVE_ATOMIC_FIND_AND_SET_ERROR                          (0xffffffff)

#include "primitive_atomic_cvip.h"

#ifndef __KERNEL__
#error "Can't include kernel header in userspace"
#endif

#endif /* GSM_PRIMITIVE_ATOMIC_H */
