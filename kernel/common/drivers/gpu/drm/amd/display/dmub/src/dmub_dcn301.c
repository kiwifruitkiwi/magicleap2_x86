/*
 * Copyright (C) 2019-2021 Advanced Micro Devices, Inc. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors: AMD
 *
 */

#include "../dmub_srv.h"
#include "dmub_reg.h"
#include "dmub_dcn301.h"

// TODO: How to get these files LNX?
#include "dcn/dcn_3_0_1_offset.h"
#include "dcn/dcn_3_0_1_sh_mask.h"
#include "mero_ip_offset.h"

#define BASE_INNER(seg) DCN_BASE__INST0_SEG##seg
#define CTX dmub
#define REGS dmub->regs

/* Registers. */

#undef REG_OFFSET
#define REG_OFFSET(reg_name) (BASE(mm##reg_name##_BASE_IDX) + mm##reg_name)

const struct dmub_srv_common_regs dmub_srv_dcn301_regs = {
#define DMUB_SR(reg) REG_OFFSET(reg),
	{ DMUB_COMMON_REGS() },
#undef DMUB_SR

#define DMUB_SF(reg, field) FD_MASK(reg, field),
	{ DMUB_COMMON_FIELDS() },
#undef DMUB_SF

#define DMUB_SF(reg, field) FD_SHIFT(reg, field),
	{ DMUB_COMMON_FIELDS() },
#undef DMUB_SF
};

/* Shared functions. */

uint32_t dmub_dcn301_get_scratch9(struct dmub_srv *dmub)
{
	/* multiplexed bits in SCRATCH9
	 * bit 0-1: current mode
	 * bit 2-3: request mode
	 * bit 4-7: dark mode status
	 */
	return REG_READ(DMCUB_SCRATCH9);
}

void dmub_dcn301_clear_scratch9(struct dmub_srv *dmub)
{
	// here we reset the scrach9 reg to an invalid value, i.e.
	// invalid dark mode status and invalid mode.
	REG_WRITE(DMCUB_SCRATCH9, 0xFFFFFFFF);
}

uint32_t dmub_dcn301_get_scratch11(struct dmub_srv *dmub)
{
	/* multiplexed bits in SCRATCH11
	 * bits 16-31: PHY status toggle UP (link wakeup) count
	 * bits 0-15:  PHY status toggle DOWN (link shutdown) count
	 */
	return REG_READ(DMCUB_SCRATCH11);
}

void dmub_dcn301_clear_scratch11(struct dmub_srv *dmub)
{
	REG_WRITE(DMCUB_SCRATCH11, 0);
}