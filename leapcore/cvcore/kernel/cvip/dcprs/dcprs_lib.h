/* SPDX-License-Identifier: GPL-2.0-only
 *
 * Copyright (c) 2022, Magic Leap, Inc. All rights reserved.
 */

#pragma once

#ifndef __KERNEL__
    #error "Can't include kernel header in userspace"
#endif

#include "dcprs_config.h"
#include "dcprs_top.h"

#define  SUCCESS                         (0)

#define BPP_RAW8  (0)
#define BPP_RAW10 (1)
#define BPP_RAW12 (2)

#define MERGEMOD_RGB (0)
#define MERGEMOD_GRAYSCALE_SPLIT (1)
#define MERGEMOD_GRAYSCALE_NO_SPLIT (2)

#define CDP_DISABLED (0)
#define CDP_ENABLED (1)

#define FRAME_SEC_MODE_DISABLED (0)
#define FRAME_SEC_MODE_ENABLED (1)
#define FRAME_SEC_MODE_DATASIZE (0x3fffffff)

#define DCPRS_ENGINE0 (0)
#define DCPRS_ENGINE1 (1)

#define DCPRS_MP0 (0)
#define DCPRS_MP1 (1)

#define DCPRS_DEFAULT_DESCRIPTOR_INDEX (0)
#define DCPRS_DEFAULT_MP_INDEX (0)

#define DCPRS_ENGINE_RB DCPRS_ENGINE1
#define DCPRS_ENGINE_G  DCPRS_ENGINE0

#define DCPRS_EOF_FORCE (1)
#define DCPRS_EOF_RELEASE (0)

int32_t dcprs_lib_init(void *base, uint32_t bpp);
int32_t dcprs_exec_descriptor(int chain_engines, int enable_int);
int32_t dcprs_config_descriptor(uint64_t input_addr,
                                uint64_t output_addr,
                                uint8_t engine);
void dcprs_config_sec_mode(uint8_t sec_mode_enabled, uint8_t which_engine, uint8_t which_mp);
void dcprs_force_eof(uint8_t value, uint8_t which_engine, uint8_t which_mp);
int32_t dcprs_section_received(uint8_t engine, uint32_t size);
void dcprs_sec_mode_wait_for_finish(void);
int32_t dcprs_eof_received(uint8_t engine);
void dcprs_engine_flush(void);
void dump_jpegls(void);

extern volatile dcprs_mp_rmap_t *default_mp;
extern dcprs_desc_t dcprs_default_mp0_eng0_desc;
extern dcprs_desc_t dcprs_default_mp0_eng1_desc;

#ifdef __va
#undef __va
#define __va(x)  (x)
#endif
#define write_reg(reg_name, value) writel(value, __va(&(reg_name)))
#define read_reg(reg_name) readl(__va(&(reg_name)))
