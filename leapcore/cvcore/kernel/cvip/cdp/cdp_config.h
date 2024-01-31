/* SPDX-License-Identifier: GPL-2.0-only
 *
 * Copyright (c) 2022, Magic Leap, Inc. All rights reserved.
 */

#pragma once

#ifndef __KERNEL__
    #error "Can't include kernel header in userspace"
#endif

#include "cdp_top.h"
#include "cdp_desc.h"

extern volatile cdp_t *cdp;

#define CDP_DESC_MEM_BASE_ADDRESS (cdp)
#define CDP_DESCRIPTOR_BASE_ADDRESS(descriptor_number) (uint64_t)(CDP_DESC_MEM_BASE_ADDRESS + descriptor_number * (sizeof(cdp_desc_t)))
#define CDP_MAX_DESCRIPTOR_NUM (20)

#define CDP_DEFAULT_DESCRIPTOR_INDEX (0)
#define CDP_BYTES_PER_THRESHOLD_VALUE (2)

#define CDP_STAGE0_N_WINDOWS (64)
#define CDP_STAGE1_N_WINDOWS (16)
#define CDP_STAGE2_N_WINDOWS (4)

int32_t cdp_write_descriptor(uint8_t desc_num, uint8_t desc_stage, void *desc);
int32_t cdp_write_block(uint32_t *src, uint32_t *dest, uint16_t size_bytes);
int32_t cdp_init_default_global_desc(cdp_global_desc_t *global_desc);
int32_t cdp_init_default_stage_desc(cdp_stage_desc_t *stage_desc);
int32_t cdp_init_default_stage0_desc(cdp_stage_desc_t *stage_desc);
int32_t cdp_init_default_stage1_desc(cdp_stage_desc_t *stage_desc);
int32_t cdp_init_default_stage2_desc(cdp_stage_desc_t *stage_desc);
int32_t cdp_init_descriptor(cdp_desc_t *desc);
int32_t cdp_write_thresholds(cdp_rgf_fast_thold_mem_t* thresholds, volatile cdp_rgf_fast_thold_mem_t *threshold_memory, uint16_t n_windows);

extern cdp_desc_t cdp_default_desc;
