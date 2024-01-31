/*
 * Copyright (C) 2019-2021 Advanced Micro Devices, Inc. All rights reserved.
 */

#ifndef __CVIP_SYSTEM_CACHE_H
#define __CVIP_SYSTEM_CACHE_H

#define SYSCACHE_STATE_EN		1
#define SYSCACHE_STATE_DIS		0

#define NONCA_NONBUFF			0x0
#define BUFFERABLE_ONLY			0x1
#define CACHEABLE			0x2
#define CACHEABLE_BUFF			0x3
#define WRTHR_RDALC_CACHEABLE		0x6
#define WRBK_RDALC_CACHEABLE		0x7
#define WRTHR_WRALC_CACHEABLE		0xA
#define WRBK_WRALC_CACHEABLE		0xB
#define WRTHR_RDWRALC_CACHEABLE		0xE
#define WRBK_RDWRALC_CACHEABLE		0xF

#define ARCACHE(x)			((x) << 0)
#define AWCACHE(x)			((x) << 4)

struct pl310_regs {
	unsigned long aux_ctrl;
	unsigned long tag_latency;
	unsigned long data_latency;
	unsigned long filter_start;
	unsigned long filter_end;
	unsigned long prefetch_ctrl;
	unsigned long pwr_ctrl;
	unsigned long ctrl;
	unsigned long dbg_ctrl;
};

enum ctrler_idx {
	PL310_CTRLER0,
	PL310_CTRLER1,
	PL310_CTRLER2,
	PL310_CTRLER3
};

int syscache_enable(unsigned int en);
int syscache_flush_all(void);
int syscache_flush_range(unsigned long start, unsigned long end);
int syscache_inv_range(unsigned long start, unsigned long end);
int syscache_inv_all(void);
int syscache_clean_range(unsigned long start, unsigned long end);
int syscache_sync(void);
int setup_syscache_axcache(unsigned int value);
int syscache_write_reg(u32 mask, u32 val, enum ctrler_idx idx, u32 reg);
unsigned int syscache_read_reg(enum ctrler_idx idx, u32 reg);
struct pl310_regs *syscache_save(enum ctrler_idx idx);

#endif

