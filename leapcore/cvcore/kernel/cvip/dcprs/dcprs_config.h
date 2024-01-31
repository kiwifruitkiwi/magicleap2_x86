/* SPDX-License-Identifier: GPL-2.0-only
 *
 * Copyright (c) 2022, Magic Leap, Inc. All rights reserved.
 */

#pragma once

#ifndef __KERNEL__
    #error "Can't include kernel header in userspace"
#endif

#include "dcprs_top.h"
#include "dcprs_desc.h"

#define DCPRS_MERGEMOD_RGB (0)
#define DCPRS_MERGEMOD_GRAYSCALE_SPLIT (1)
#define DCPRS_MERGEMOD_GRAYSCALE_NO_SPLIT (2)

#define DCPRS_BPP_RAW8  (0)
#define DCPRS_BPP_RAW10 (1)
#define DCPRS_BPP_RAW12 (2)
#define DCPRS_BPP_RAW16 (3)

#define DCPRS_CDP_DISABLED (0)
#define DCPRS_CDP_ENABLED (1)

#define DCPRS_FRAME_SEC_MODE_DISABLED (0)
#define DCPRS_FRAME_SEC_MODE_ENABLED (1)
#define DCPRS_FRAME_SEC_MODE_DATASIZE (0x3fffffd)

#define DCPRS_ENGINE0 (0)
#define DCPRS_ENGINE1 (1)

#define DCPRS_MP0 (0)
#define DCPRS_MP1 (1)

#define DCPRS_EOF_RELEASE (0)
#define DCPRS_EOF_FORCE (1)

#define DCPRS_DESC_MEM_BASE_ADDRESS DCPRS_BASE_ADDRESS

#define DCPRS_BAYER_FIRST_PIXEL_BLUE_RED (0)
#define DCPRS_BAYER_FIRST_PIXEL_GREEN (1)

#define DCPRS_MAX_DESCRIPTOR_INDEX (10)

int32_t dcprs_write_descriptor(dcprs_desc_t *desc, uint8_t mp, uint8_t eng, uint8_t desc_num);
int32_t dcprs_write_block(uint32_t *src, uint32_t *dest, uint16_t size_bytes);
int32_t dcprs_init_descriptor(dcprs_desc_t *desc);
void    dcprs_dump_descriptor(dcprs_desc_t *desc);
dcprs_mp_rgf_dcprs_desc_mem_t *dcprs_get_descriptor_base_address(uint8_t mp_num, uint8_t engine_num, uint8_t desc_num);
