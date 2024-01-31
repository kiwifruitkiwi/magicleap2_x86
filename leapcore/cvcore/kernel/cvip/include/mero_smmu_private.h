/* SPDX-License-Identifier: GPL-2.0-only
 *
 * Copyright (c) 2023, Magic Leap, Inc. All rights reserved.
 */

#ifndef MERO_SMMU_PRIV_H
#define MERO_SMMU_PRIV_H

#include "cvcore_processor_common.h"

typedef uint16_t sid_t;

#define TOTAL_NR_OF_STREAM_IDS (24)

static const uint16_t cvcore_stream_ids[TOTAL_NR_OF_STREAM_IDS] = {
    /* Default Stream ID */
    0x0,
    /* Stream IDs for CVCORE DSP Tests */
    0x100,
    0x101,
    0x102,
    0x103,
    0x104,
    0x105,
    0x106,
    0x107,
    0x108,
    0x109,
    0x7, /* used in shregion gtest */
    /* Stream IDs for Algo Tests / Packages */
    0x8000,
    0x8001,
    0x8002,
    0x8003,
    0x8004,
    0x8005,
    0x8006,
    0x8007,
    0x8008,
    /* Stream IDs used for shregion duplication */
    0x800C,
    0x800D,
    0x800E,
};

// GSM Stream ID meta
// The follwowing 2 macro defs are dup
// from interfaces/core/kernel/ml_drivers/gsm/include/gsm.h
// since that file can't be pulled into the kernel space.
#define GSM_RESERVED_STREAM_ID_GLOBAL                   (0x0C448U)
#define GSM_RESERVED_STREAM_ID_GLOBAL_SIZE              (0x200) // 512B

#define SMMU_MAINT_TYPE_UNMAP  (1)

struct __attribute__((__packed__)) smmu_maint_req {
    uint8_t type;
    sid_t stream_id;
};

typedef union {
	struct {
		sid_t stream_id;
		uint32_t is_orphaned : 1;
		uint32_t is_active : 1;
		uint32_t ref_count : 8;
		uint32_t reserved : 6;
		uint8_t dsp_ref[CVCORE_NUM_DSPS];
	} __attribute__ ((__packed__)) sid_ctx;
    /*
     * total size is 4 bytes plus 1 byte per dsp rounded up
     * to multiply of 4.
     */
	uint32_t ctx_val[((CVCORE_NUM_DSPS * sizeof(uint8_t)) / sizeof(uint32_t)) + 1];
} gsm_stream_id_ctx_t;

#endif
