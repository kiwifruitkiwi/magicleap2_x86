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
#include "cdp.h"

int32_t cdp_lib_init(void *base);
int32_t cdp_exec_descriptor(void);
int32_t cdp_config_global_descriptor(uint64_t input_addr, uint64_t summary_addr);
int32_t cdp_config_stage_descriptor(uint8_t stage, uint64_t axi_output_buf, uint64_t fast_output_buf);
int32_t cdp_config_thresholds(uint8_t stage, struct cdp_windows* threshold_buf);
int32_t cdp_section_received(uint32_t size);
int32_t cdp_wait_for_finish(void);

#ifdef __va
#undef __va
#define __va(x)  (x)
#endif

void writelX(uint32_t value, volatile void *addr);
#define write_reg(reg_name, value) writelX(value, __va(&(reg_name)))
#define read_reg(reg_name) readl(__va(&(reg_name)))

