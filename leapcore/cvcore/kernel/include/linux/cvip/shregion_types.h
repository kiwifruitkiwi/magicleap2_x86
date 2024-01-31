/* SPDX-License-Identifier: GPL-2.0-only
 *
 * Copyright (c) 2020-2023, Magic Leap, Inc. All rights reserved.
 */

#pragma once

#include <linux/types.h>

#define SHREGION_FLAG_BIT(bit)         (1UL << (bit))
#define SHREGION_CACHE_BIT(bit)        (1UL << (bit))

// This flag must be used for shregions
// (with its cache_mask) that are not
// shared outside of arms or if it's
// not a device type shregion.
#define SHREGION_ARM_CACHED            SHREGION_CACHE_BIT(0)

// This flag currently maps to SLC_0
// behind the scenes.
#define SHREGION_CVIP_SLC_CACHED       SHREGION_CACHE_BIT(1)

// Use this flag if x86 shared shregion
// needs to be x86 cached
#define SHREGION_X86_CACHED            SHREGION_CACHE_BIT(2)

// Use this flag with cache_mask
// for shregions needed to be
// DSP data cached (write back mode).
#define SHREGION_DSP_DATA_WB_CACHED    SHREGION_CACHE_BIT(3)

#define SHREGION_CVIP_SLC_0_CACHED     SHREGION_CACHE_BIT(1)

#define SHREGION_CVIP_SLC_1_CACHED     SHREGION_CACHE_BIT(4)

#define SHREGION_CVIP_SLC_2_CACHED     SHREGION_CACHE_BIT(5)

#define SHREGION_CVIP_SLC_3_CACHED     SHREGION_CACHE_BIT(6)

/* Types of shregions and type member.
 * These are to be used for type member of struct shregion_metadata.
 */
enum shregion_mem_types {
    SHREGION_DDR_SHARED = 0,
    SHREGION_DDR_PRIVATE,
    SHREGION_DDR_PRIVATE_CONTIG,
    // DSP Q6 and C5 SRAMs.
    SHREGION_Q6_SCM_0,
    SHREGION_C5_NCM_0,
    SHREGION_C5_NCM_1,
    // GSM space (8M in total).
    SHREGION_GSM,
    // FMRs
    SHREGION_HYDRA_FMR2,
    SHREGION_X86_SPCIE_FMR2_10,
    SHREGION_ISPIN_FMR5,
    SHREGION_ISPOUT_FMR8,
    SHREGION_X86_RO_FMR10,
    SHREGION_MAX_ENTRIES,
};

// TODO(sjain): we might have one more flag that would indicate if the
// non-allocator process wants to MAP_WRITE or MAP_READ the mmapped
// shregion.

/*
 * By default, every shregion will be non-shareable.
 * Use this with flag member of struct shregion_metadata depending on if you
 * want the memory allocated to be shared with the other arm processes. Also,
 * this flag must be used (logical OR'd) with SHREGION_PERSISTENT. In case
 * a process creates a persistent shregion, dies and comes back up,
 * it should be able to map the same shregion again.
 */
#define SHREGION_SHAREABLE                    SHREGION_FLAG_BIT(0)

/*
 * By default, every shregion will be non-presistent i.e shregion
 * will be freed as soon as all the processes referencing it
 * dies or frees it.
 * If this flag is not set, shregion will be destroyed when the
 * reference count to it reaches 0.
 * This flag must be used for shregions shared with the dsps or
 * the x86.
 */
#define SHREGION_PERSISTENT                   SHREGION_FLAG_BIT(1)

/*
 * Set this flag if the shregion needs to be shared with the dsps.
 * Note: This flag cannot be used along with SHREGION_DSP_CODE
 * and SHREGION_DSP_MAP_ALL_STREAM_IDS.
 */
#define SHREGION_DSP_SHAREABLE                SHREGION_FLAG_BIT(2)

/*
 * Set this flag if you need device type (truely non-arm-cached,
 * ordered) access to a shregion buffer.
 * NOTE: Use it only when necessary as it may affect performance.
 */
#define SHREGION_DEVICE_TYPE                  SHREGION_FLAG_BIT(3)

/*
 * This flag is used for creating dsp code region
 * mappings in smmu for all dsp stream ids
 */
#define SHREGION_DSP_CODE                     SHREGION_FLAG_BIT(4)

/* This flag forces dsp shared meta registration.
 * This flag must only be used for testing and
 * debug purposes and must not be used in user
 * build (production code).
 */
#define FORCE_DSP_META_REGISTRATION           SHREGION_FLAG_BIT(5)

/*
 * This flag has meaning only when SCM buffers
 * are needed to be shared with the x86.
 */
#define X86_SHARED_SHREGION_SRAM              SHREGION_FLAG_BIT(6)

/*
 * Use this flag for shregion accessibility
 * from CDP and decompression engines.
 */
#define SHREGION_CDP_AND_DCPRS_SHAREABLE      SHREGION_FLAG_BIT(7)

/*
 * This shregion's iommu mapping will never be
 * removed unless cvip reboots. The mapping will
 * persist on the death of the process that created
 * this mapping.
 */
#define CDP_AND_DCPRS_MMU_MAP_PERSIST         SHREGION_FLAG_BIT(8)

/*
 * This flag is used to create a shregion with accessibility
 * from the DMAC engine.
 */
#define SHREGION_DMAC_SHAREABLE               SHREGION_FLAG_BIT(9)

/*
 * Use this flag for SRAM shregion accessibility
 * from Hydra.
 */
#define SHREGION_CVIP_SRAM_SPCIE_SHAREABLE    SHREGION_FLAG_BIT(10)

/* Flags to select system direct dram alt_0/alt_1
 * path in cvip
 */
#define SHREGION_DRAM_ALT_0                   SHREGION_FLAG_BIT(11)
#define SHREGION_DRAM_ALT_1                   SHREGION_FLAG_BIT(12)

/*
 * This flag is used for creating dsp static region
 * mappings in smmu for all dsp stream ids
 */
#define SHREGION_DSP_MAP_ALL_STREAM_IDS       SHREGION_FLAG_BIT(13)

/*
 * This flag is used to create a DMAC shareable shregion
 * as having a requested fixed io virtual address.
 */
#define SHREGION_FIXED_DMAC_IO_VADDR          SHREGION_FLAG_BIT(14)

/*
 * This flag is used to force disable guard bands for
 * given shregion when the feature is enabled. This
 * flag has to be passed when shregion is created.
 */
#define SHREGION_FORCE_DISABLE_GUARD_BANDS    SHREGION_FLAG_BIT(15)

/*
 * This flag marks the shregion as available to be included in a core dump.
 */
#define SHREGION_INCLUDE_IN_CORE              SHREGION_FLAG_BIT(16)

#define SHREGION_SLC_SYNC_FLAG_BITS(bit)      (1UL << (bit))

// To be used with struct shr_slc_sync_meta flags

// Clean causes the contents of the cache line to
// be written back to memory (or the next level of cache),
// but only if the cache line is "dirty".
#define SHREGION_SLC_SYNC_CLEAN               SHREGION_SLC_SYNC_FLAG_BITS(0)
// Flush causes clean and then invalidate.
#define SHREGION_SLC_SYNC_FLUSH               SHREGION_SLC_SYNC_FLAG_BITS(1)
#define SHREGION_SLC_SYNC_INVAL               SHREGION_SLC_SYNC_FLAG_BITS(2)

typedef void *ShregionMem;          /* shregion memory type */
typedef void *ShregionHandle;       /* shregion instance handle */
typedef uint64_t ShregionPhysAddr;  /* shregion physical mem address */

typedef uint32_t ShregionDspVaddr;
typedef uint32_t ShregionCdpDcprsIOaddr;
typedef uint32_t ShregionDmacIOaddr;

struct shregion_metadata {
    uint64_t name; // CvcoreName.
    enum shregion_mem_types type;
    uint32_t size_bytes;
    uint32_t cache_mask;
    uint32_t flags;
    uint32_t fixed_dsp_v_addr;
    uint32_t fixed_dmac_iova;
};

struct shr_slc_sync_meta {
    uint64_t name;
    uint64_t start_addr;
    uint32_t size_bytes;
    uint32_t flags;
};

#ifndef __KERNEL__
#error "Can't include kernel header in userspace"
#endif
