/*
 * Copyright 2019-2021 (C) Advanced Micro Devices, Inc. All rights reserved.
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

#ifndef _DMUB_DCN301_H_
#define _DMUB_DCN301_H_

#include "dmub_dcn20.h"

/* Registers. */

extern const struct dmub_srv_common_regs dmub_srv_dcn301_regs;

/* Hardware functions. */
uint32_t dmub_dcn301_get_scratch9(struct dmub_srv *dmub);
void dmub_dcn301_clear_scratch9(struct dmub_srv *dmub);
uint32_t dmub_dcn301_get_scratch11(struct dmub_srv *dmub);
void dmub_dcn301_clear_scratch11(struct dmub_srv *dmub);

#endif /* _DMUB_DCN301_H_ */
