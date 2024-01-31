/* SPDX-License-Identifier: GPL-2.0-only
 *
 * Copyright (c) 2020-2022, Magic Leap, Inc. All rights reserved.
 */

#define SHREGION_STATUS_BASE                (0x4100)

// Status codes returned by the shregion misc driver.

// shregion already exists.
#define SHREGION_MEM_PREEXISTENT            (0x4101)

// shregion doesn't exist.
#define SHREGION_MEM_NONEXISTENT            (0x4102)

// shregion is already registered.
#define SHREGION_MEM_PREREGISTERED          (0x4103)

// shregion created successfully.
#define SHREGION_MEM_CREATED                (0x4104)

// shregion is shareable.
#define SHREGION_MEM_SHAREABLE              (0x4105)

// shregion is not shareable.
#define SHREGION_MEM_NON_SHAREABLE          (0x4106)

// shregion size is <= 0.
#define SHREGION_MEM_INVALID_SIZE           (0x4107)

// shregion was destroyed.
#define SHREGION_MEM_FREE_SUCCESS           (0x4108)

// shregion metadata found.
#define SHREGION_MEM_META_SUCCESS           (0x4109)

// shregion metadata not found.
#define SHREGION_MEM_META_FAILED            (0x410A)

// Failed to get shregions' physical address.
#define SHREGION_MEM_GET_PHYS_FAILED        (0x410B)

// Got shregions' physical address successfully.
#define SHREGION_MEM_GET_PHYS_SUCCESS       (0x410C)

// Reached max pid references to a shregion.
#define SHREGION_MEM_MAX_REF_ERROR          (0x410D)

// Failed to get buffer fd in QUERY IOCTL.
#define SHREGION_MEM_GET_BUFFER_FD_FAILED   (0x410E)

// This shregion type is not supported.
#define SHREGION_MEM_TYPE_NOT_SUPPORTED     (0x410F)

#ifndef __KERNEL__
#error "Can't include kernel header in userspace"
#endif
