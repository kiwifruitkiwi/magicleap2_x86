/* SPDX-License-Identifier: GPL-2.0-only
 *
 * Copyright (c) 2022, Magic Leap, Inc. All rights reserved.
 */

#pragma once

#ifndef __KERNEL__
    #error "Can't include kernel header in userspace"
#endif

//
// Vender IDs
//
#define PCIE_NWL_VID                    0x19aa
#define PCIE_ML_VID                     0x1e65
#define PCIE_AMD_VID                    0X1022

//
// Device IDs
//
#define PCIE_DSE_DID                    0x5002
#define PCIE_EP_HYDRA_BOOTLOADER_DID    0xb0bc
#define PCIE_EP_HYDRA_MAIN_FW_DID       0xb0bb
