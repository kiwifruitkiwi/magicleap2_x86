/* SPDX-License-Identifier: GPL-2.0-only
 *
 * Copyright (c) 2022, Magic Leap, Inc. All rights reserved.
 */

#pragma once

#ifndef __KERNEL__
    #error "Can't include kernel header in userspace"
#endif

//
// Low-level HBCP protocol definitions, used between Hydra bootloader
// and Hydra kernel driver.
//
//
// Define fields in vendor-defined register for HBCP use.
//
#define HBCP_H_CMD_A	    (0x80000000)
#define HBCP_H_CMD_B	    (0x40000000)
#define HBCP_E_RDY_A	    (0x00000080)
#define HBCP_E_RDY_B	    (0x00000040)
#define HBCP_E_ERR_A	    (0x00000020)
#define HBCP_E_ERR_B	    (0x00000010)
#define HBCP_E_MODE_FW_NONE (0x00000001)
#define HBCP_E_MODE_FW_BOOT (0x00000002)
#define HBCP_E_MODE_BUSY    (0x00000004)
#define HBCP_E_MODE_PANIC   (0x00000008)
#define HBCP_E_MODE_MASK    (0x0f)
#define HBCP_GET_CMD(w)	    (((w) >> 24) & 0x3f)
#define HBCP_SET_CMD(w)	    ((w) << 24)
#define HBCP_GET_ARG(w)	    (((w) >> 8) & 0xffff)
#define HBCP_SET_ARG(w)	    ((w) << 8)


//
// HBCP commands
//
#define HBCP_CMD_SHMEM_MAP_0	(0)
#define HBCP_CMD_SHMEM_MAP_1	(1)
#define HBCP_CMD_SHMEM_MAP_2	(2)
#define HBCP_CMD_SHMEM_MAP_3	(3)
#define HBCP_CMD_NOOP	        (4)
#define HBCP_CMD_UNLOCK	        (5)
#define HBCP_CMD_RESET	        (6)
#define HBCP_CMD_BOOT	        (7)
#define HBCP_CMD_IDENT_SET      (8)
#define HBCP_CMD_OTP_LOAD       (9)
#define HBCP_CMD_OTP_STORE      (10)
#define HBCP_CMD_TLV_STORE      (11)

// For legacy compatability
#define HBCP_IDENT_SET      (HBCP_CMD_IDENT_SET)

void hbcp_init(void);
void hbcp_sysmode_set(uint32_t mode);
void hbcp_sysmode_get(uint32_t *mode);
