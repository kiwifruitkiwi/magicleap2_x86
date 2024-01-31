/* SPDX-License-Identifier: GPL-2.0-only
 *
 * Copyright (c) 2022, Magic Leap, Inc. All rights reserved.
 */

#pragma once

#ifndef __KERNEL__
    #error "Can't include kernel header in userspace"
#endif

//
// This file describes the root structure that's shared between the Hydra
// kernel driver and the Hydra bootloader/firmware, and resident in Hydra SRAM.
//

#include <cv_version.h>
#include <hsi.h>

#define CLOG_BOOT_SIZE	        (1024 * 4)
#define CLOG_APP_SIZE	        (1024 * 4)
#define MAX_LOG_COUNT           (42)
#define MAX_LOG_OFFSET          (96 * MAX_LOG_COUNT)

#define SRAM_LOG_MLDMA_FAST     (0)
#define SRAM_LOG_MLDMA_SLOW     (1)
#define SRAM_LOG_MLDMA_PCIE     (2)
#define SRAM_LOG_MLDMA_AUDIO    (3)
#define MLDMA_COUNT             (4)

// These bits are exported in byte #1 of the hydra device's PCIe config serial number.
#define PCI_CFG_BITS_OTP         (0x08)  // Indicates that the bootloader is executing in OTP space, vs sram.
#define PCI_CFG_BITS_SEC_BOOT    (0x04)  // Indicates that secure booting is enforced.
#define PCI_CFG_BITS_ORION_BOOT  (0x02)  // Indicates orion is being booted from Hydra bootloader.

struct sram_map {
    uint32_t        magic;
#define SRAM_MAGIC    (0x83825740)

    CvVersion       vers;
#define SRAM_VERS     (CV_VERSION(1,3))

    // Console logs for bootloader and firmware.
    uint8_t         clog_boot[CLOG_BOOT_SIZE];
    uint8_t         clog_app[CLOG_APP_SIZE];
    uint32_t        clog_boot_off;
    uint32_t        clog_app_off;

    // If non-zero, the address of the shmem_map.
    uint64_t        shmem_addr;

    // If non-zero, the offset to (from SRAM base) and size of HSI table.
    uint32_t        hsi_tab_off;
    uint32_t        hsi_tab_size;

    uint32_t        boot_status_bl;
        #define SRAM_BL_STATUS_SIG_BAD    (1 << 0)    // bad signature verification
        #define SRAM_BL_STATUS_SIG_NONE   (1 << 1)    // no signature on archive
    uint32_t        boot_status_fw;
    uint32_t        boot_status_cmd;
#define BOOT_STATUS_CMD_NORMAL     (0xcafedead)
#define BOOT_STATUS_CMD_PCIE_ONLY  (0xcafedeaf)

    // MLDMA log offsets (from SRAM_BASE) and sizes.
    // The logs must all be below 0x60600000.
    struct {
        uint32_t offset;
        uint32_t size;
    }               mldma_log[MLDMA_COUNT];
    uint32_t clog_last_write_count;
};
