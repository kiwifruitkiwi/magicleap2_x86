/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2012 ARM Limited
 *
 * Author: Will Deacon <will.deacon@arm.com>
 */

#include <linux/init.h>
#include <linux/linkage.h>
#include <linux/const.h>
#include <asm/page.h>

	.globl vdso_lp64_start, vdso_lp64_end
	.section .rodata
	.balign PAGE_SIZE
vdso_lp64_start:
	.incbin "arch/arm64/kernel/vdso/vdso.so"
	.balign PAGE_SIZE
vdso_lp64_end:

	.previous
