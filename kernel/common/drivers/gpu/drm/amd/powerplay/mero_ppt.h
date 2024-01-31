/*
 * Copyright 2020-2021 Advanced Micro Devices, Inc.
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
 */

#ifndef __MERO_PPT_H__
#define __MERO_PPT_H__

extern void mero_set_ppt_funcs(struct smu_context *smu);

/* UMD PState Msg Parameters in MHz */
#define MERO_UMD_PSTATE_GFXCLK       800
#define MERO_UMD_PSTATE_SOCCLK       678
#define MERO_UMD_PSTATE_FCLK         800


/*
 * These are the frequencies (in MHz) being set for profile_standard
 * and profile_peak performance mode. Not all of them are being used
 * in code but they are kept here for documentation purpose to know
 * what frequencies are being set.
 */

#define MERO_UMD_PSTATE_STANDARD_GFXCLK		600
#define MERO_UMD_PSTATE_STANDARD_SOCCLK		600
#define MERO_UMD_PSTATE_STANDARD_FCLK		800
#define MERO_UMD_PSTATE_STANDARD_VCLK		705
#define MERO_UMD_PSTATE_STANDARD_DCLK		600

#define MERO_UMD_PSTATE_PEAK_GFXCLK		1100
#define MERO_UMD_PSTATE_PEAK_SOCCLK		1000
#define MERO_UMD_PSTATE_PEAK_FCLK		800
#define MERO_UMD_PSTATE_PEAK_VCLK		1000
#define MERO_UMD_PSTATE_PEAK_DCLK		800

/* RLC Power Status */
#define RLC_STATUS_OFF          0
#define RLC_STATUS_NORMAL       1

#endif
