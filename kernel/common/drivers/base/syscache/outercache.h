/*
 * arch/arm/include/asm/outercache.h
 *
 * Copyright (C) 2010 ARM Ltd.
 * Written by Catalin Marinas <catalin.marinas@arm.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#ifndef __ASM_OUTERCACHE_H
#define __ASM_OUTERCACHE_H

#include <linux/types.h>

struct l2x0_regs;

struct outer_cache_fns {
	void (*inv_range)(struct outer_cache_fns *fns,
			  unsigned long start, unsigned long end);
	void (*clean_range)(struct outer_cache_fns *fns,
			    unsigned long start, unsigned long end);
	void (*flush_range)(struct outer_cache_fns *fns,
			    unsigned long start, unsigned long end);
	void (*flush_all)(struct outer_cache_fns *fns);
	void (*disable)(struct outer_cache_fns *fns);
#ifdef CONFIG_OUTER_CACHE_SYNC
	void (*sync)(struct outer_cache_fns *fns);
#endif
	void (*resume)(struct outer_cache_fns *fns);

	/* This is an ARM L2C thing */
	void (*write_sec)(struct outer_cache_fns *fns,
			  unsigned long val, unsigned int reg);
	void (*configure)(struct outer_cache_fns *fns,
			  const struct l2x0_regs *regs);
};


#ifdef CONFIG_OUTER_CACHE
/**
 * outer_inv_range - invalidate range of outer cache lines
 * @fns: address of the callback object
 * @start: starting physical address, inclusive
 * @end: end physical address, exclusive
 */
static inline void outer_inv_range(struct outer_cache_fns *fns,
				   phys_addr_t start, phys_addr_t end)
{
	if (fns->inv_range)
		fns->inv_range(fns, start, end);
}

/**
 * outer_clean_range - clean dirty outer cache lines
 * @fns: address of the callback object
 * @start: starting physical address, inclusive
 * @end: end physical address, exclusive
 */
static inline void outer_clean_range(struct outer_cache_fns *fns,
				     phys_addr_t start, phys_addr_t end)
{
	if (fns->clean_range)
		fns->clean_range(fns, start, end);
}

/**
 * outer_flush_range - clean and invalidate outer cache lines
 * @fns: address of the callback object
 * @start: starting physical address, inclusive
 * @end: end physical address, exclusive
 */
static inline void outer_flush_range(struct outer_cache_fns *fns,
				     phys_addr_t start, phys_addr_t end)
{
	if (fns->flush_range)
		fns->flush_range(fns, start, end);
}

/**
 * outer_flush_all - clean and invalidate all cache lines in the outer cache
 * @fns: address of the callback object
 *
 * Note: depending on implementation, this may not be atomic - it must
 * only be called with interrupts disabled and no other active outer
 * cache masters.
 *
 * It is intended that this function is only used by implementations
 * needing to override the outer_cache.disable() method due to security.
 * (Some implementations perform this as a clean followed by an invalidate.)
 */
static inline void outer_flush_all(struct outer_cache_fns *fns)
{
	if (fns->flush_all)
		fns->flush_all(fns);
}

/**
 * outer_resume - restore the cache configuration and re-enable outer cache
 * @fns: address of the callback object
 *
 * Restore any configuration that the cache had when previously enabled,
 * and re-enable the outer cache.
 */
static inline void outer_resume(struct outer_cache_fns *fns)
{
	if (fns->resume)
		fns->resume(fns);
}

#else

static inline void outer_inv_range(struct outer_cache_fns *fns,
				   phys_addr_t start, phys_addr_t end)
{ }
static inline void outer_clean_range(struct outer_cache_fns *fns,
				     phys_addr_t start, phys_addr_t end)
{ }
static inline void outer_flush_range(struct outer_cache_fns *fns,
				     phys_addr_t start, phys_addr_t end)
{ }
static inline void outer_flush_all(struct outer_cache_fns *fns) { }
static inline void outer_resume(struct outer_cache_fns *fns) { }

#endif

#endif	/* __ASM_OUTERCACHE_H */
