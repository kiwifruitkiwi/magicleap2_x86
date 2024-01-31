/* SPDX-License-Identifier: GPL-2.0-only
 *
 * Copyright (c) 2022, Magic Leap, Inc. All rights reserved.
 */

#pragma once

#ifndef __KERNEL__
    #error "Can't include kernel header in userspace"
#endif

#include "hsi.h"
#include "sram_map.h"

#define BCOR_BASE       (0x80000000)
#define BCOR_AXI(a)     ((a)&0x0fffffff)
#define APB_BASE        (0x00140000)
#define APB_ADDR(a)     ((a))
#define PCI_TO_X11(a)   ((a) | BCOR_BASE)

#define FW_LOAD_ADDR    (0x60700000)

#define OTP_BASE_ADDR   (0x40020000)

#define PCIE_BASE       (0x00000000)
#define NWL_DMA_BASE    (PCIE_BASE + 0x00000000)
#define NWL_BRCORE_BASE (PCIE_BASE + 0x00001000)
#define HOST_DDR_BASE   (PCIE_BASE + (1024 * 1024 * 4))
#define HOST_FW_BASE    (PCIE_BASE + (1024 * 1024 * 8))
#define ACP_SRAM_BASE   (PCIE_BASE + (1024 * 1024 * 12))
#define DIMMER_DDR_BASE (PCIE_BASE + (1024 * 1024 * 14))

#define HCP_DATA_LEN    (50204UL)

// Calculate the location of interesting things in hydra sram.
#define SRAM_ALIGN (1024 * 4)
#define OFF_HSI_TABLE ((sizeof(struct sram_map) + SRAM_ALIGN) & ~(SRAM_ALIGN - 1))
#define OFF_HCP_HYDRA ((OFF_HSI_TABLE + sizeof(struct hsi_root) + SRAM_ALIGN) & ~(SRAM_ALIGN - 1))
#define OFF_IMU_SHMEM ((OFF_HCP_HYDRA + HCP_DATA_LEN + SRAM_ALIGN) & ~(SRAM_ALIGN - 1))
#define IMU_SHMEM_SIZE  (1024 * 4)
#define OFF_IMU_EXPORT_AND_SHARED (OFF_IMU_SHMEM + IMU_SHMEM_SIZE)
#define IMU_EXPORT_AND_SHARED_SIZE (1024 * 8)
#define OFF_SPI_LOCK (OFF_IMU_EXPORT_AND_SHARED + IMU_EXPORT_AND_SHARED_SIZE)

#define MEA_ACP_MEM_BASE       0x80c00000
#define MEA_ACP_MEM_OUT_OFF    0x35400
#define MEA_ACP_MEM_IN_OFF     0x25400
#define MEA_ACP_MEM_SIZE       (0x10000)
#define MEA_ACP_MEM_BASE_OR    (MEA_ACP_MEM_BASE + MEA_ACP_MEM_OUT_OFF)
#define MEA_ACP_MEM_BASE_IR    (MEA_ACP_MEM_BASE + MEA_ACP_MEM_IN_OFF)
#define MEA_MIC_STATUS_OFFSET  (5 * sizeof(uint32_t))

#define MIC_STATUS_ADDR (MEA_ACP_MEM_BASE_IR + (MEA_ACP_MEM_SIZE/2) + MEA_MIC_STATUS_OFFSET)

#define MEA_ACP_RECORDING_STATUS_SHIFT (1)
#define MEA_ACP_RECORDING_STATUS_MASK  (0x1 << MEA_ACP_RECORDING_STATUS_SHIFT)
