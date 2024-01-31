/* SPDX-License-Identifier: GPL-2.0-only
 *
 * \file cvcore_typedefs.h
 *
 * \brief CVCore common type definitions
 *
 * Copyright (c) 2019-2022, Magic Leap, Inc. All rights reserved.
 */

#pragma once

#include <linux/types.h>

typedef uint64_t CvcoreTimeStamp;       //!< CV Core timestamp data type
typedef uint64_t CvcoreEventId;         //!< CV Core event ID data type

/* Now for gcc (C) (and C++, given the define above): */
#define STATIC_ASSERT(test_for_true) _Static_assert((test_for_true), "(" #test_for_true ") failed");

#ifndef bool
#define bool        _Bool
#define false       0
#define true        1
#endif

// enum-macro definitions that are used to help manage enum 'lists' that
// have human-readable strings associated with them.
// the macros are seperated into two categories, depending on how the original list was created

/* the basic vs. extended list definition:

#define MY_ENUM_BASIC_LIST(x) \
    x(THE_ENUM, "The String")

#define MY_ENUM_EXTENDED_LIST(x) \
    x(THE_ENUM, THE_VAL, "The String")
*/

// basic definitions, for lists with two entries
#define CVCORE_ENUM_DEF(e_def, e_str)                       e_def,
#define CVCORE_ENUM_DEF_STR(e_def, e_str)                   case e_def: return e_str;
#define CVCORE_ENUM_DEF_STR_EXACT(e_def, e_str)             case e_def: return #e_def;

// extended definitions, for list with 3 entries
#define CVCORE_ENUM_DEF_EXT(e_def, e_val, e_str)            e_def = e_val,
#define CVCORE_ENUM_DEF_EXT_STR(e_def, e_val, e_str)        case e_def: return e_str;
#define CVCORE_ENUM_DEF_EXT_STR_EXACT(e_def, e_val, e_str)  case e_def: return #e_def;

// Weak Symbol
#define CVCORE_SYMBOL_WEAK  __attribute__((weak))

/******* AlIGNMENT DEFINITIONS **********/

/*!
 * IS_CVCORE_ALIGNED checks for buffer base addresses.
 * Check if buffer is aligned to selected power of 2 alignment in bytes.
 */
#define IS_CVCORE_ALIGNED_XB(_addr, _align_size) \
    ((((uintptr_t) (_addr)) & ((_align_size) - 1)) == 0)

// 32bit Word (or Scalar aligned for DSPs) aligned
#define IS_CVCORE_ALIGNED_4B(_addr)        IS_CVCORE_ALIGNED_XB(_addr,  4)
// 128bit AXI aligned
#define IS_CVCORE_ALIGNED_16B(_addr)       IS_CVCORE_ALIGNED_XB(_addr, 16)

// DSP or ARM L1 Cache Line size in bytes
#define CVCORE_CACHELINE_SIZE_BYTES        (64)
// DSP Vector width in bytes
#define CVCORE_DSP_VECTOR_WIDTH_BYTES      (64)

// DSP Vector or ARM/DSP Cache Line Aligned
// Vector aligned (mainly useful for DSPs and data shared between DSPs and A55)
#define IS_CVCORE_ALIGNED_VECTOR(_addr)       IS_CVCORE_ALIGNED_XB(_addr, 64)
// 64byte L1 Cache line aligned (For CVIP, L1 cache line size is same for A55s and DSPs)
#define IS_CVCORE_ALIGNED_L1_CACHELINE(_addr) IS_CVCORE_ALIGNED_XB(_addr, 64)

/*!
 * Force align base address of a memory (updates the base address
 * to nearest aligned value)
 */
// Get next aligned address, specificed by align size in Power of 2 Bytes.
#define CVCORE_ALIGN_XB(_addr, _align_size) \
    ((((uintptr_t) (_addr)) + (_align_size) - 1) & (~ (uintptr_t)((_align_size) - 1)))

// DSP Vector or ARM/DSP Cache Line Aligned
#define CVCORE_ALIGN_L1_CACHELINE(_addr)      CVCORE_ALIGN_XB(_addr, 64)

/*!
 * Variable attributes (Packed, data placements, alignment)
 */
#define CVCORE_ALIGNED_XB(_align_size) __attribute__ ((aligned(_align_size)))
#define CVCORE_ALIGNED_L1_CACHELINE    CVCORE_ALIGNED_XB(64)
#define CVCORE_DATA_PACKED             __attribute__ ((packed))

#ifndef __KERNEL__
#error "Can't include kernel header in userspace"
#endif
