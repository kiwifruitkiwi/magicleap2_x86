/* SPDX-License-Identifier: GPL-2.0-only
 *
 * Copyright (c) 2022, Magic Leap, Inc. All rights reserved.
 */

#pragma once

#ifndef __KERNEL__
    #error "Can't include kernel header in userspace"
#endif

#include <cv_version.h>

//
// This file describes the root structure that's shared between the Hydra
// kernel driver and the Hydra bootloader/firmware, resident in CVIP DDR,
// and used to convey low-level operating parameters.
//

#define HYDRA_COUNT    (2)

#pragma pack(push, 1)
struct __attribute__ ((aligned (4))) shmem_map {
    uint32_t    magic;
#define SHMEM_MAP_MAGIC	(0x19951998)

    CvVersion   vers;
#define SHMEM_VERS     (CV_VERSION(1,2))

    // Bus address and length of the block containing this map structure.
    uint64_t    shmem_io_addr;
    uint32_t    shmem_length;

    // To communicate to Hydra the bus address of the base of GSM, and its length.
    uint64_t	gsm_io_addr;
    uint64_t	gsm_length;

    // spares, what used to be xcmp
    uint64_t    spare_0;
    uint64_t    spare_1;

    struct {
        uint64_t bar0;
        uint64_t bar2;
        uint64_t bar4;

        // Bus address and length of Hydra's application code archive.
        uint64_t fw_io_addr;
        uint64_t fw_length;

        // Hydra critical event notification. Written to by hydra endpoint.
        uint32_t hydra_ce_arg0;
        uint32_t hydra_ce_arg1; // Used for hydra watchdog

        // Hydra watchdog enabling. Written to by cvip endpoint.
        // Bitfield - each bit enables watchdog's feature
        uint32_t hydra_wd_enable;
    } hydra[HYDRA_COUNT];

    // To communicate to Hydra the bus address of the base of ACP sram, and its length.
    uint64_t acpmem_io_addr;
    uint64_t acpmem_length;

    // To communicate to Hydra the bus address of the base of dimmer memory, and its length.
    uint64_t dim_mem_io_addr;
    uint64_t dim_mem_length;

    // To communicate to Hydra the bus address of the base of SCM (cvip sram), and its length.
    uint64_t scm_mem_io_addr;
    uint64_t scm_mem_length;

    // NOTE!! This struct is also accessed by the bootloader!
    // Do NOT change any fields above this point,
    // New words can be only appended at the end.
    uint32_t hydra_p_guc_version;
    uint32_t hydra_s_guc_version;
};
#pragma pack(pop)
