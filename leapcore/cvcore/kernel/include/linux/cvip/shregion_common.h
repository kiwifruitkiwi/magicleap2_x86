/* SPDX-License-Identifier: GPL-2.0-only
 *
 * Copyright (c) 2020-2022, Magic Leap, Inc. All rights reserved.
 */

#pragma once

uint8_t ion_core_get_heap_type(uint8_t shr_id);
uint32_t ion_core_get_cache_mask(uint32_t cache_mask);
uint32_t ion_core_get_ion_flags(uint32_t flags);
int is_x86_sharing_supported(uint8_t mem_type);
bool is_shregion_slc_cached(uint32_t cache_mask);

#ifndef __KERNEL__
#error "Can't include kernel header in userspace"
#endif
