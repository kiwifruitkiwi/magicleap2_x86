/*
 * Copyright 2021 Advanced Micro Devices, Inc.
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

#ifndef __AMDGPU_FWLOG_PROC_H__
#define __AMDGPU_FWLOG_PROC_H__

#include <linux/kernel.h>

#define FW_SHARED_FLAG_0_FW_LOGGING         (1 << 10)

extern int amdgpu_vcn3_fwlog;
extern void *fwlog_cpu_addr;
extern uint64_t fwlog_gpu_addr;
extern struct amdgpu_bo *fwlog_bo;

extern const char program_name[];

/// if defined debug is enabled
#define DEBUG

#ifdef DEBUG
/// Macro for debug print
#define dbgprint(format, args...) \
    do {            \
        printk(KERN_DEBUG "%s %s %d: "format"\n", program_name, __FUNCTION__, __LINE__, ## args);\
    } while ( 0 )
#else
#define dbgprint(format, args...) do {} while( 0 );
#endif

/* Just the function prototypes for now */
int __init vcn3_fwlog_proc_init(void);
void __exit vcn3_fwlog_proc_cleanup(void);

#endif

