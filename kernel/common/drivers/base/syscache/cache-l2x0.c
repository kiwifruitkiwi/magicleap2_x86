/*
 * arch/arm/mm/cache-l2x0.c - L210/L220/L310 cache controller support
 *
 * Copyright (C) 2007 ARM Limited
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
 * along with this program.
 */
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/cpu.h>
#include <linux/err.h>
#include <linux/smp.h>
#include <linux/init.h>
#include <linux/log2.h>
#include <linux/module.h>
#include <linux/spinlock.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>
#include <asm/cputype.h>
#include <asm/cacheflush.h>

#include "outercache.h"
#include "cache-l2x0.h"

/**
 * struct l2c_init_data - information for one single l2c block instance
 * @type: l2c hardware type
 * @way_size_0: used to calculate cache size
 * @num_lock: number of locking
 * @of_parse: functor for parsing OF
 * @enable: functor for enabling cache
 * @fixup: functor used during initialization
 * @save: functor for saving registers
 * @configure: functor for configuration
 * @unlock: functor for unlocking
 * @outer_cache: outer_cache interface
 * @l2x0_base: base register address of l2c instance
 * @l2x0_lock: lock for atomic operation
 * @l2x0_way_mask: used to calculate cache size
 * @l2x0_size: used to store cache size
 * @sync_reg_offset: offset of sync register
 * @l2x0_saved_regs: struct to save registers
 * @pmu: pmu pointer for perf_event support
 */
struct l2c_init_data {
	const char *type;
	unsigned way_size_0;
	unsigned num_lock;
	void (*of_parse)(struct l2c_init_data *data,
			 const struct device_node *np,
			 u32 *aux_val, u32 *aux_mask);
	void (*enable)(struct l2c_init_data *data, unsigned int num_lock);
	void (*fixup)(struct l2c_init_data *data,
		      u32 cache_id, struct outer_cache_fns *fns);
	void (*save)(struct l2c_init_data *data);
	void (*configure)(struct l2c_init_data *data);
	void (*unlock)(struct l2c_init_data *data, unsigned int num);
	struct outer_cache_fns outer_cache;
	void __iomem *l2x0_base;
	raw_spinlock_t l2x0_lock;
	u32 l2x0_way_mask;	/* Bitmask of active ways */
	u32 l2x0_size;
	unsigned long sync_reg_offset;
	struct l2x0_regs l2x0_saved_regs;
	struct pmu *pmu;
};

#define CACHE_LINE_SIZE		32
#define CACHE_AUX_VALUE		0x02020000
#define CACHE_AUX_MASK		0x7FFF3C01

/*
 * Common code for all cache controllers.
 */
static inline void l2c_wait_mask(void __iomem *reg, unsigned long mask)
{
	/* wait for cache operation by line or way to complete */
	while (readl_relaxed(reg) & mask)
		cpu_relax();
}

/*
 * By default, we write directly to secure registers.  Platforms must
 * override this if they are running non-secure.
 */
static void l2c_write_sec(struct l2c_init_data *data,
			  unsigned long val, unsigned int reg)
{
	/*
	 * For Mero, currently all the secure L2c310 registers are written
	 * by bootloader during bootup.
	 * Linux kernel doesn't have the privilege to write them after bootup.
	 * So do nothing here.
	 */
	/* TODO: implement SiP SMC based secure register writing routine */
}

/*
 * This should only be called when we have a requirement that the
 * register be written due to a work-around, as platforms running
 * in non-secure mode may not be able to access this register.
 */
static inline void l2c_set_debug(struct l2c_init_data *data, unsigned long val)
{
	l2c_write_sec(data, val, L2X0_DEBUG_CTRL);
}

static void __l2c_op_way(void __iomem *reg, u32 way_mask)
{
	writel_relaxed(way_mask, reg);
	l2c_wait_mask(reg, way_mask);
}

static inline void l2c_unlock(struct l2c_init_data *data, unsigned int num)
{
	unsigned i;

	for (i = 0; i < num; i++) {
		writel_relaxed(0, data->l2x0_base + L2X0_LOCKDOWN_WAY_D_BASE +
			       i * L2X0_LOCKDOWN_STRIDE);
		writel_relaxed(0, data->l2x0_base + L2X0_LOCKDOWN_WAY_I_BASE +
			       i * L2X0_LOCKDOWN_STRIDE);
	}
}

static void l2c_configure(struct l2c_init_data *data)
{
	l2c_write_sec(data, data->l2x0_saved_regs.aux_ctrl, L2X0_AUX_CTRL);
}

/*
 * Enable the L2 cache controller.  This function must only be
 * called when the cache controller is known to be disabled.
 */
static void l2c_enable(struct l2c_init_data *data, unsigned int num_lock)
{
	unsigned long flags;

	if (data->outer_cache.configure)
		data->outer_cache.configure(&data->outer_cache,
		&data->l2x0_saved_regs);
	else
		data->configure(data);

	data->unlock(data, num_lock);

	local_irq_save(flags);
	__l2c_op_way(data->l2x0_base + L2X0_INV_WAY, data->l2x0_way_mask);
	writel_relaxed(0, data->l2x0_base + data->sync_reg_offset);
	l2c_wait_mask(data->l2x0_base + data->sync_reg_offset, 1);
	local_irq_restore(flags);

	l2c_write_sec(data, L2X0_CTRL_EN, L2X0_CTRL);
}

static void l2c_disable(struct l2c_init_data *data)
{
	l2x0_pmu_suspend(data->pmu);

	data->outer_cache.flush_all(&data->outer_cache);
	l2c_write_sec(data, 0, L2X0_CTRL);
	dsb(st);
}

static void l2c_save(struct l2c_init_data *data)
{
	data->l2x0_saved_regs.aux_ctrl =
		readl_relaxed(data->l2x0_base + L2X0_AUX_CTRL);
}

static void l2c_resume(struct l2c_init_data *data)
{
	void __iomem *base = data->l2x0_base;

	/* Do not touch the controller if already enabled. */
	if (!(readl_relaxed(base + L2X0_CTRL) & L2X0_CTRL_EN))
		l2c_enable(data, data->num_lock);

	l2x0_pmu_resume(data->pmu);
}

/*
 * L2C-210 specific code.
 *
 * The L2C-2x0 PA, set/way and sync operations are atomic, but we must
 * ensure that no background operation is running.  The way operations
 * are all background tasks.
 *
 * While a background operation is in progress, any new operation is
 * ignored (unspecified whether this causes an error.)  Thankfully, not
 * used on SMP.
 *
 * Never has a different sync register other than L2X0_CACHE_SYNC, but
 * we use sync_reg_offset here so we can share some of this with L2C-310.
 */
static void __l2c210_cache_sync(struct l2c_init_data *data)
{
	writel_relaxed(0, data->l2x0_base + data->sync_reg_offset);
}

static void __l2c210_op_pa_range(void __iomem *reg, unsigned long start,
	unsigned long end)
{
	while (start < end) {
		writel_relaxed(start, reg);
		start += CACHE_LINE_SIZE;
	}
}

static void l2c210_inv_range(struct outer_cache_fns *fns,
			     unsigned long start, unsigned long end)
{
	struct l2c_init_data *data = container_of(fns,
		struct l2c_init_data, outer_cache);
	void __iomem *base = data->l2x0_base;

	if (start & (CACHE_LINE_SIZE - 1)) {
		start &= ~(CACHE_LINE_SIZE - 1);
		writel_relaxed(start, base + L2X0_CLEAN_INV_LINE_PA);
		start += CACHE_LINE_SIZE;
	}

	if (end & (CACHE_LINE_SIZE - 1)) {
		end &= ~(CACHE_LINE_SIZE - 1);
		writel_relaxed(end, base + L2X0_CLEAN_INV_LINE_PA);
	}

	__l2c210_op_pa_range(base + L2X0_INV_LINE_PA, start, end);
	__l2c210_cache_sync(base);
}

static void l2c210_clean_range(struct outer_cache_fns *fns,
			       unsigned long start, unsigned long end)
{
	struct l2c_init_data *data = container_of(fns,
		struct l2c_init_data, outer_cache);
	void __iomem *base = data->l2x0_base;

	start &= ~(CACHE_LINE_SIZE - 1);
	__l2c210_op_pa_range(base + L2X0_CLEAN_LINE_PA, start, end);
	__l2c210_cache_sync(base);
}

static void l2c210_flush_range(struct outer_cache_fns *fns,
			       unsigned long start, unsigned long end)
{
	struct l2c_init_data *data = container_of(fns,
		struct l2c_init_data, outer_cache);
	void __iomem *base = data->l2x0_base;

	start &= ~(CACHE_LINE_SIZE - 1);
	__l2c210_op_pa_range(base + L2X0_CLEAN_INV_LINE_PA, start, end);
	__l2c210_cache_sync(base);
}

static void l2c210_flush_all(struct outer_cache_fns *fns)
{
	struct l2c_init_data *data = container_of(fns,
		struct l2c_init_data, outer_cache);
	void __iomem *base = data->l2x0_base;

	BUG_ON(!irqs_disabled());

	__l2c_op_way(base + L2X0_CLEAN_INV_WAY, data->l2x0_way_mask);
	__l2c210_cache_sync(data);
}

static void l2c210_sync(struct outer_cache_fns *fns)
{
	struct l2c_init_data *data = container_of(fns,
		struct l2c_init_data, outer_cache);
	__l2c210_cache_sync(data);
}

/*
 * L2C-310 specific code.
 *
 * Very similar to L2C-210, the PA, set/way and sync operations are atomic,
 * and the way operations are all background tasks.  However, issuing an
 * operation while a background operation is in progress results in a
 * SLVERR response.  We can reuse:
 *
 *  __l2c210_cache_sync (using sync_reg_offset)
 *  l2c210_sync
 *  l2c210_inv_range (if 588369 is not applicable)
 *  l2c210_clean_range
 *  l2c210_flush_range (if 588369 is not applicable)
 *  l2c210_flush_all (if 727915 is not applicable)
 *
 * Errata:
 * 588369: PL310 R0P0->R1P0, fixed R2P0.
 *	Affects: all clean+invalidate operations
 *	clean and invalidate skips the invalidate step, so we need to issue
 *	separate operations.  We also require the above debug workaround
 *	enclosing this code fragment on affected parts.  On unaffected parts,
 *	we must not use this workaround without the debug register writes
 *	to avoid exposing a problem similar to 727915.
 *
 * 727915: PL310 R2P0->R3P0, fixed R3P1.
 *	Affects: clean+invalidate by way
 *	clean and invalidate by way runs in the background, and a store can
 *	hit the line between the clean operation and invalidate operation,
 *	resulting in the store being lost.
 *
 * 752271: PL310 R3P0->R3P1-50REL0, fixed R3P2.
 *	Affects: 8x64-bit (double fill) line fetches
 *	double fill line fetches can fail to cause dirty data to be evicted
 *	from the cache before the new data overwrites the second line.
 *
 * 753970: PL310 R3P0, fixed R3P1.
 *	Affects: sync
 *	prevents merging writes after the sync operation, until another L2C
 *	operation is performed (or a number of other conditions.)
 *
 * 769419: PL310 R0P0->R3P1, fixed R3P2.
 *	Affects: store buffer
 *	store buffer is not automatically drained.
 */
static void l2c310_inv_range_erratum(struct outer_cache_fns *fns,
				     unsigned long start, unsigned long end)
{
	struct l2c_init_data *data = container_of(fns,
		struct l2c_init_data, outer_cache);
	void __iomem *base = data->l2x0_base;

	if ((start | end) & (CACHE_LINE_SIZE - 1)) {
		unsigned long flags;

		/* Erratum 588369 for both clean+invalidate operations */
		raw_spin_lock_irqsave(&data->l2x0_lock, flags);
		l2c_set_debug(base, 0x03);

		if (start & (CACHE_LINE_SIZE - 1)) {
			start &= ~(CACHE_LINE_SIZE - 1);
			writel_relaxed(start, base + L2X0_CLEAN_LINE_PA);
			writel_relaxed(start, base + L2X0_INV_LINE_PA);
			start += CACHE_LINE_SIZE;
		}

		if (end & (CACHE_LINE_SIZE - 1)) {
			end &= ~(CACHE_LINE_SIZE - 1);
			writel_relaxed(end, base + L2X0_CLEAN_LINE_PA);
			writel_relaxed(end, base + L2X0_INV_LINE_PA);
		}

		l2c_set_debug(base, 0x00);
		raw_spin_unlock_irqrestore(&data->l2x0_lock, flags);
	}

	__l2c210_op_pa_range(base + L2X0_INV_LINE_PA, start, end);
	__l2c210_cache_sync(base);
}

static void l2c310_flush_range_erratum(struct outer_cache_fns *fns,
				       unsigned long start, unsigned long end)
{
	struct l2c_init_data *data = container_of(fns,
		struct l2c_init_data, outer_cache);
	void __iomem *base = data->l2x0_base;
	raw_spinlock_t *lock = &data->l2x0_lock;
	unsigned long flags;

	raw_spin_lock_irqsave(lock, flags);
	while (start < end) {
		unsigned long blk_end = start + min(end - start, 4096UL);

		l2c_set_debug(base, 0x03);
		while (start < blk_end) {
			writel_relaxed(start, base + L2X0_CLEAN_LINE_PA);
			writel_relaxed(start, base + L2X0_INV_LINE_PA);
			start += CACHE_LINE_SIZE;
		}
		l2c_set_debug(base, 0x00);

		if (blk_end < end) {
			raw_spin_unlock_irqrestore(lock, flags);
			raw_spin_lock_irqsave(lock, flags);
		}
	}
	raw_spin_unlock_irqrestore(lock, flags);
	__l2c210_cache_sync(base);
}

static void l2c310_flush_all_erratum(struct outer_cache_fns *fns)
{
	struct l2c_init_data *data = container_of(fns,
		struct l2c_init_data, outer_cache);
	void __iomem *base = data->l2x0_base;
	unsigned long flags;

	raw_spin_lock_irqsave(&data->l2x0_lock, flags);
	l2c_set_debug(base, 0x03);
	__l2c_op_way(base + L2X0_CLEAN_INV_WAY, data->l2x0_way_mask);
	l2c_set_debug(base, 0x00);
	__l2c210_cache_sync(base);
	raw_spin_unlock_irqrestore(&data->l2x0_lock, flags);
}

static void __init l2c310_save(struct l2c_init_data *data)
{
	unsigned revision;
	void __iomem *base = data->l2x0_base;

	l2c_save(data);

	data->l2x0_saved_regs.tag_latency = readl_relaxed(base +
		L310_TAG_LATENCY_CTRL);
	data->l2x0_saved_regs.data_latency = readl_relaxed(base +
		L310_DATA_LATENCY_CTRL);
	data->l2x0_saved_regs.filter_end = readl_relaxed(base +
		L310_ADDR_FILTER_END);
	data->l2x0_saved_regs.filter_start = readl_relaxed(base +
		L310_ADDR_FILTER_START);

	revision = readl_relaxed(base + L2X0_CACHE_ID) &
			L2X0_CACHE_ID_RTL_MASK;

	/* From r2p0, there is Prefetch offset/control register */
	if (revision >= L310_CACHE_ID_RTL_R2P0)
		data->l2x0_saved_regs.prefetch_ctrl = readl_relaxed(base +
							L310_PREFETCH_CTRL);

	/* From r3p0, there is Power control register */
	if (revision >= L310_CACHE_ID_RTL_R3P0)
		data->l2x0_saved_regs.pwr_ctrl = readl_relaxed(base +
							L310_POWER_CTRL);
}

static void l2c310_configure(struct l2c_init_data *data)
{
	unsigned revision;
	void __iomem *base = data->l2x0_base;

	l2c_configure(data);

	/* restore pl310 setup */
	l2c_write_sec(data, data->l2x0_saved_regs.tag_latency,
		      L310_TAG_LATENCY_CTRL);
	l2c_write_sec(data, data->l2x0_saved_regs.data_latency,
		      L310_DATA_LATENCY_CTRL);
	l2c_write_sec(data, data->l2x0_saved_regs.filter_end,
		      L310_ADDR_FILTER_END);
	l2c_write_sec(data, data->l2x0_saved_regs.filter_start,
		      L310_ADDR_FILTER_START);

	revision = readl_relaxed(base + L2X0_CACHE_ID) &
				 L2X0_CACHE_ID_RTL_MASK;

	if (revision >= L310_CACHE_ID_RTL_R2P0)
		l2c_write_sec(data, data->l2x0_saved_regs.prefetch_ctrl,
			      L310_PREFETCH_CTRL);
	if (revision >= L310_CACHE_ID_RTL_R3P0)
		l2c_write_sec(data, data->l2x0_saved_regs.pwr_ctrl,
			      L310_POWER_CTRL);
}

static void __init l2c310_enable(struct l2c_init_data *data,
				 unsigned int num_lock)
{
	void __iomem *base = data->l2x0_base;
	unsigned rev = readl_relaxed(base + L2X0_CACHE_ID) & L2X0_CACHE_ID_RTL_MASK;
	u32 aux = data->l2x0_saved_regs.aux_ctrl;

	if (rev >= L310_CACHE_ID_RTL_R2P0) {
		pr_warn("L2C-310 early BRESP only supported with Cortex-A9\n");
		aux &= ~L310_AUX_CTRL_EARLY_BRESP;
	} else if (aux & (L310_AUX_CTRL_FULL_LINE_ZERO | L310_AUX_CTRL_EARLY_BRESP)) {
		pr_err("L2C-310: disabling Cortex-A9 specific feature bits\n");
		aux &= ~(L310_AUX_CTRL_FULL_LINE_ZERO | L310_AUX_CTRL_EARLY_BRESP);
	}

	/*
	 * Always enable non-secure access to the lockdown registers -
	 * we write to them as part of the L2C enable sequence so they
	 * need to be accessible.
	 */
	data->l2x0_saved_regs.aux_ctrl = aux | L310_AUX_CTRL_NS_LOCKDOWN;

	l2c_enable(data, num_lock);

	/* Read back resulting AUX_CTRL value as it could have been altered. */
	aux = readl_relaxed(base + L2X0_AUX_CTRL);

	if (aux & (L310_AUX_CTRL_DATA_PREFETCH | L310_AUX_CTRL_INSTR_PREFETCH)) {
		u32 prefetch = readl_relaxed(base + L310_PREFETCH_CTRL);

		pr_info("L2C-310 %s%s prefetch enabled, offset %u lines\n",
			aux & L310_AUX_CTRL_INSTR_PREFETCH ? "I" : "",
			aux & L310_AUX_CTRL_DATA_PREFETCH ? "D" : "",
			1 + (prefetch & L310_PREFETCH_CTRL_OFFSET_MASK));
	}

	/* r3p0 or later has power control register */
	if (rev >= L310_CACHE_ID_RTL_R3P0) {
		u32 power_ctrl;

		power_ctrl = readl_relaxed(base + L310_POWER_CTRL);
		pr_info("L2C-310 dynamic clock gating %sabled, standby mode %sabled\n",
			power_ctrl & L310_DYNAMIC_CLK_GATING_EN ? "en" : "dis",
			power_ctrl & L310_STNDBY_MODE_EN ? "en" : "dis");
	}
}

static void __init l2c310_fixup(struct l2c_init_data *data, u32 cache_id,
				struct outer_cache_fns *fns)
{
	unsigned revision = cache_id & L2X0_CACHE_ID_RTL_MASK;
	const char *errata[8];
	unsigned n = 0;

	if (IS_ENABLED(CONFIG_PL310_ERRATA_588369) &&
	    revision < L310_CACHE_ID_RTL_R2P0 &&
	    /* For bcm compatibility */
	    fns->inv_range == l2c210_inv_range) {
		fns->inv_range = l2c310_inv_range_erratum;
		fns->flush_range = l2c310_flush_range_erratum;
		errata[n++] = "588369";
	}

	if (IS_ENABLED(CONFIG_PL310_ERRATA_727915) &&
	    revision >= L310_CACHE_ID_RTL_R2P0 &&
	    revision < L310_CACHE_ID_RTL_R3P1) {
		fns->flush_all = l2c310_flush_all_erratum;
		errata[n++] = "727915";
	}

	if (revision >= L310_CACHE_ID_RTL_R3P0 &&
	    revision < L310_CACHE_ID_RTL_R3P2) {
		u32 val = data->l2x0_saved_regs.prefetch_ctrl;
		if (val & L310_PREFETCH_CTRL_DBL_LINEFILL) {
			val &= ~L310_PREFETCH_CTRL_DBL_LINEFILL;
			data->l2x0_saved_regs.prefetch_ctrl = val;
			errata[n++] = "752271";
		}
	}

	if (IS_ENABLED(CONFIG_PL310_ERRATA_753970) &&
	    revision == L310_CACHE_ID_RTL_R3P0) {
		data->sync_reg_offset = L2X0_DUMMY_REG;
		errata[n++] = "753970";
	}

	if (IS_ENABLED(CONFIG_PL310_ERRATA_769419))
		errata[n++] = "769419";

	if (n) {
		unsigned i;

		pr_info("L2C-310 errat%s", n > 1 ? "a" : "um");
		for (i = 0; i < n; i++)
			pr_cont(" %s", errata[i]);
		pr_cont(" enabled\n");
	}
}

static void l2c310_disable(struct outer_cache_fns *fns)
{
	struct l2c_init_data *data = container_of(fns,
		struct l2c_init_data, outer_cache);
	l2c_disable(data);
}

static void l2c310_resume(struct outer_cache_fns *fns)
{
	struct l2c_init_data *data = container_of(fns,
		struct l2c_init_data, outer_cache);
	l2c_resume(data);
}

static void l2c310_unlock(struct l2c_init_data *data,
			  unsigned int num_lock)
{
	if (readl_relaxed(data->l2x0_base + L2X0_AUX_CTRL) &
		L310_AUX_CTRL_NS_LOCKDOWN)
		l2c_unlock(data, num_lock);
}

static int __init __l2c_init(struct l2c_init_data *data,
			     u32 aux_val, u32 aux_mask, u32 cache_id, bool nosync)
{
	unsigned way_size_bits, ways;
	u32 aux, old_aux;


	/*
	 * Sanity check the aux values.  aux_mask is the bits we preserve
	 * from reading the hardware register, and aux_val is the bits we
	 * set.
	 */
	if (aux_val & aux_mask)
		pr_alert("L2C: platform provided aux values permit register corruption.\n");

	aux = readl_relaxed(data->l2x0_base + L2X0_AUX_CTRL);
	old_aux = aux;
	aux &= aux_mask;
	aux |= aux_val;

	if (old_aux != aux)
		pr_warn("L2C: DT/platform modifies aux control register: 0x%08x -> 0x%08x\n",
		        old_aux, aux);

	/* Determine the number of ways */
	switch (cache_id & L2X0_CACHE_ID_PART_MASK) {
	case L2X0_CACHE_ID_PART_L310:
		if ((aux_val | ~aux_mask) & (L2C_AUX_CTRL_WAY_SIZE_MASK | L310_AUX_CTRL_ASSOCIATIVITY_16))
			pr_warn("L2C: DT/platform tries to modify or specify cache size\n");
		if (aux & (1 << 16))
			ways = 16;
		else
			ways = 8;
		break;

	case L2X0_CACHE_ID_PART_L210:
	case L2X0_CACHE_ID_PART_L220:
		ways = (aux >> 13) & 0xf;
		break;

	default:
		/* Assume unknown chips have 8 ways */
		ways = 8;
		break;
	}

	data->l2x0_way_mask = (1 << ways) - 1;

	/*
	 * way_size_0 is the size that a way_size value of zero would be
	 * given the calculation: way_size = way_size_0 << way_size_bits.
	 * So, if way_size_bits=0 is reserved, but way_size_bits=1 is 16k,
	 * then way_size_0 would be 8k.
	 *
	 * L2 cache size = number of ways * way size.
	 */
	way_size_bits = (aux & L2C_AUX_CTRL_WAY_SIZE_MASK) >>
			L2C_AUX_CTRL_WAY_SIZE_SHIFT;
	data->l2x0_size = ways * (data->way_size_0 << way_size_bits);

	if (data->fixup)
		data->fixup(data, cache_id, &data->outer_cache);
	if (nosync) {
		pr_info("L2C: disabling outer sync\n");
		data->outer_cache.sync = NULL;
	}

	/*
	 * Check if l2x0 controller is already enabled.  If we are booting
	 * in non-secure mode accessing the below registers will fault.
	 */
	if (!(readl_relaxed(data->l2x0_base + L2X0_CTRL) & L2X0_CTRL_EN)) {
		data->l2x0_saved_regs.aux_ctrl = aux;

		data->enable(data, data->num_lock);
	}


	/*
	 * It is strange to save the register state before initialisation,
	 * but hey, this is what the DT implementations decided to do.
	 */
	if (data->save)
		data->save(data);

	/* Re-read it in case some bits are reserved. */
	aux = readl_relaxed(data->l2x0_base + L2X0_AUX_CTRL);

	pr_info("%s cache controller enabled, %d ways, %d kB\n",
		data->type, ways, data->l2x0_size >> 10);
	pr_info("%s: CACHE_ID 0x%08x, AUX_CTRL 0x%08x\n",
		data->type, cache_id, aux);

	data->pmu = l2x0_pmu_register(data->l2x0_base, cache_id);

	if (IS_ERR(data->pmu))
		data->pmu = NULL;

	return 0;
}

static int __init __l2c_exit(struct l2c_init_data *data)
{
	l2x0_pmu_unregister(data->pmu);

	return 0;
}

#ifdef CONFIG_OF

/**
 * l2x0_cache_size_of_parse() - read cache size parameters from DT
 * @np: the device tree node for the l2 cache
 * @aux_val: pointer to machine-supplied auxilary register value, to
 * be augmented by the call (bits to be set to 1)
 * @aux_mask: pointer to machine-supplied auxilary register mask, to
 * be augmented by the call (bits to be set to 0)
 * @associativity: variable to return the calculated associativity in
 * @max_way_size: the maximum size in bytes for the cache ways
 */
static int __init l2x0_cache_size_of_parse(const struct device_node *np,
					    u32 *aux_val, u32 *aux_mask,
					    u32 *associativity,
					    u32 max_way_size)
{
	u32 mask = 0, val = 0;
	u32 cache_size = 0, sets = 0;
	u32 way_size_bits = 1;
	u32 way_size = 0;
	u32 block_size = 0;
	u32 line_size = 0;

	of_property_read_u32(np, "cache-size", &cache_size);
	of_property_read_u32(np, "cache-sets", &sets);
	of_property_read_u32(np, "cache-block-size", &block_size);
	of_property_read_u32(np, "cache-line-size", &line_size);

	if (!cache_size || !sets)
		return -ENODEV;

	/* All these l2 caches have the same line = block size actually */
	if (!line_size) {
		if (block_size) {
			/* If linesize is not given, it is equal to blocksize */
			line_size = block_size;
		} else {
			/* Fall back to known size */
			pr_warn("L2C OF: no cache block/line size given: "
				"falling back to default size %d bytes\n",
				CACHE_LINE_SIZE);
			line_size = CACHE_LINE_SIZE;
		}
	}

	if (line_size != CACHE_LINE_SIZE)
		pr_warn("L2C OF: DT supplied line size %d bytes does "
			"not match hardware line size of %d bytes\n",
			line_size,
			CACHE_LINE_SIZE);

	/*
	 * Since:
	 * set size = cache size / sets
	 * ways = cache size / (sets * line size)
	 * way size = cache size / (cache size / (sets * line size))
	 * way size = sets * line size
	 * associativity = ways = cache size / way size
	 */
	way_size = sets * line_size;
	*associativity = cache_size / way_size;

	if (way_size > max_way_size) {
		pr_err("L2C OF: set size %dKB is too large\n", way_size);
		return -EINVAL;
	}

	pr_info("L2C OF: override cache size: %d bytes (%dKB)\n",
		cache_size, cache_size >> 10);
	pr_info("L2C OF: override line size: %d bytes\n", line_size);
	pr_info("L2C OF: override way size: %d bytes (%dKB)\n",
		way_size, way_size >> 10);
	pr_info("L2C OF: override associativity: %d\n", *associativity);

	/*
	 * Calculates the bits 17:19 to set for way size:
	 * 512KB -> 6, 256KB -> 5, ... 16KB -> 1
	 */
	way_size_bits = ilog2(way_size >> 10) - 3;
	if (way_size_bits < 1 || way_size_bits > 6) {
		pr_err("L2C OF: cache way size illegal: %dKB is not mapped\n",
		       way_size);
		return -EINVAL;
	}

	mask |= L2C_AUX_CTRL_WAY_SIZE_MASK;
	val |= (way_size_bits << L2C_AUX_CTRL_WAY_SIZE_SHIFT);

	*aux_val &= ~mask;
	*aux_val |= val;
	*aux_mask &= ~mask;

	return 0;
}

static void __init l2c310_of_parse(struct l2c_init_data *d,
				   const struct device_node *np,
	u32 *aux_val, u32 *aux_mask)
{
	u32 data[3] = { 0, 0, 0 };
	u32 tag[3] = { 0, 0, 0 };
	u32 filter[2] = { 0, 0 };
	u32 assoc;
	u32 prefetch;
	u32 power;
	u32 val;
	int ret;

	of_property_read_u32_array(np, "arm,tag-latency", tag, ARRAY_SIZE(tag));
	if (tag[0] && tag[1] && tag[2])
		d->l2x0_saved_regs.tag_latency =
			L310_LATENCY_CTRL_RD(tag[0] - 1) |
			L310_LATENCY_CTRL_WR(tag[1] - 1) |
			L310_LATENCY_CTRL_SETUP(tag[2] - 1);

	of_property_read_u32_array(np, "arm,data-latency",
				   data, ARRAY_SIZE(data));
	if (data[0] && data[1] && data[2])
		d->l2x0_saved_regs.data_latency =
			L310_LATENCY_CTRL_RD(data[0] - 1) |
			L310_LATENCY_CTRL_WR(data[1] - 1) |
			L310_LATENCY_CTRL_SETUP(data[2] - 1);

	of_property_read_u32_array(np, "arm,filter-ranges",
				   filter, ARRAY_SIZE(filter));
	if (filter[1]) {
		d->l2x0_saved_regs.filter_end =
					ALIGN(filter[0] + filter[1], SZ_1M);
		d->l2x0_saved_regs.filter_start = (filter[0] & ~(SZ_1M - 1))
					| L310_ADDR_FILTER_EN;
	}

	ret = l2x0_cache_size_of_parse(np, aux_val, aux_mask, &assoc, SZ_512K);
	if (!ret) {
		switch (assoc) {
		case 16:
			*aux_val &= ~L2X0_AUX_CTRL_ASSOC_MASK;
			*aux_val |= L310_AUX_CTRL_ASSOCIATIVITY_16;
			*aux_mask &= ~L2X0_AUX_CTRL_ASSOC_MASK;
			break;
		case 8:
			*aux_val &= ~L2X0_AUX_CTRL_ASSOC_MASK;
			*aux_mask &= ~L2X0_AUX_CTRL_ASSOC_MASK;
			break;
		default:
			pr_err("L2C-310 OF cache associativity %d invalid, only 8 or 16 permitted\n",
			       assoc);
			break;
		}
	}

	if (of_property_read_bool(np, "arm,shared-override")) {
		*aux_val |= L2C_AUX_CTRL_SHARED_OVERRIDE;
		*aux_mask &= ~L2C_AUX_CTRL_SHARED_OVERRIDE;
	}

	if (of_property_read_bool(np, "arm,parity-enable")) {
		*aux_val |= L2C_AUX_CTRL_PARITY_ENABLE;
		*aux_mask &= ~L2C_AUX_CTRL_PARITY_ENABLE;
	} else if (of_property_read_bool(np, "arm,parity-disable")) {
		*aux_val &= ~L2C_AUX_CTRL_PARITY_ENABLE;
		*aux_mask &= ~L2C_AUX_CTRL_PARITY_ENABLE;
	}

	prefetch = d->l2x0_saved_regs.prefetch_ctrl;

	ret = of_property_read_u32(np, "arm,double-linefill", &val);
	if (ret == 0) {
		if (val)
			prefetch |= L310_PREFETCH_CTRL_DBL_LINEFILL;
		else
			prefetch &= ~L310_PREFETCH_CTRL_DBL_LINEFILL;
	} else if (ret != -EINVAL) {
		pr_err("L2C-310 OF arm,double-linefill property value is missing\n");
	}

	ret = of_property_read_u32(np, "arm,double-linefill-incr", &val);
	if (ret == 0) {
		if (val)
			prefetch |= L310_PREFETCH_CTRL_DBL_LINEFILL_INCR;
		else
			prefetch &= ~L310_PREFETCH_CTRL_DBL_LINEFILL_INCR;
	} else if (ret != -EINVAL) {
		pr_err("L2C-310 OF arm,double-linefill-incr property value is missing\n");
	}

	ret = of_property_read_u32(np, "arm,double-linefill-wrap", &val);
	if (ret == 0) {
		if (!val)
			prefetch |= L310_PREFETCH_CTRL_DBL_LINEFILL_WRAP;
		else
			prefetch &= ~L310_PREFETCH_CTRL_DBL_LINEFILL_WRAP;
	} else if (ret != -EINVAL) {
		pr_err("L2C-310 OF arm,double-linefill-wrap property value is missing\n");
	}

	ret = of_property_read_u32(np, "arm,prefetch-drop", &val);
	if (ret == 0) {
		if (val)
			prefetch |= L310_PREFETCH_CTRL_PREFETCH_DROP;
		else
			prefetch &= ~L310_PREFETCH_CTRL_PREFETCH_DROP;
	} else if (ret != -EINVAL) {
		pr_err("L2C-310 OF arm,prefetch-drop property value is missing\n");
	}

	ret = of_property_read_u32(np, "arm,prefetch-offset", &val);
	if (ret == 0) {
		prefetch &= ~L310_PREFETCH_CTRL_OFFSET_MASK;
		prefetch |= val & L310_PREFETCH_CTRL_OFFSET_MASK;
	} else if (ret != -EINVAL) {
		pr_err("L2C-310 OF arm,prefetch-offset property value is missing\n");
	}

	ret = of_property_read_u32(np, "prefetch-data", &val);
	if (ret == 0) {
		if (val)
			prefetch |= L310_PREFETCH_CTRL_DATA_PREFETCH;
		else
			prefetch &= ~L310_PREFETCH_CTRL_DATA_PREFETCH;
	} else if (ret != -EINVAL) {
		pr_err("L2C-310 OF prefetch-data property value is missing\n");
	}

	ret = of_property_read_u32(np, "prefetch-instr", &val);
	if (ret == 0) {
		if (val)
			prefetch |= L310_PREFETCH_CTRL_INSTR_PREFETCH;
		else
			prefetch &= ~L310_PREFETCH_CTRL_INSTR_PREFETCH;
	} else if (ret != -EINVAL) {
		pr_err("L2C-310 OF prefetch-instr property value is missing\n");
	}

	d->l2x0_saved_regs.prefetch_ctrl = prefetch;

	power = d->l2x0_saved_regs.pwr_ctrl |
		L310_DYNAMIC_CLK_GATING_EN | L310_STNDBY_MODE_EN;

	ret = of_property_read_u32(np, "arm,dynamic-clock-gating", &val);
	if (!ret) {
		if (!val)
			power &= ~L310_DYNAMIC_CLK_GATING_EN;
	} else if (ret != -EINVAL) {
		pr_err("L2C-310 OF dynamic-clock-gating property value is missing or invalid\n");
	}
	ret = of_property_read_u32(np, "arm,standby-mode", &val);
	if (!ret) {
		if (!val)
			power &= ~L310_STNDBY_MODE_EN;
	} else if (ret != -EINVAL) {
		pr_err("L2C-310 OF standby-mode property value is missing or invalid\n");
	}

	d->l2x0_saved_regs.pwr_ctrl = power;
}

static const struct l2c_init_data of_l2c310_data __initconst = {
	.type = "L2C-310",
	.way_size_0 = SZ_8K,
	.num_lock = 8,
	.of_parse = l2c310_of_parse,
	.enable = l2c310_enable,
	.fixup = l2c310_fixup,
	.save  = l2c310_save,
	.configure = l2c310_configure,
	.unlock = l2c310_unlock,
	.outer_cache = {
		.inv_range   = l2c210_inv_range,
		.clean_range = l2c210_clean_range,
		.flush_range = l2c210_flush_range,
		.flush_all   = l2c210_flush_all,
		.disable     = l2c310_disable,
		.sync        = l2c210_sync,
		.resume      = l2c310_resume,
	},
	.sync_reg_offset = L2X0_CACHE_SYNC,
};

/*
 * This is a variant of the of_l2c310_data with .sync set to
 * NULL. Outer sync operations are not needed when the system is I/O
 * coherent, and potentially harmful in certain situations (PCIe/PL310
 * deadlock on Armada 375/38x due to hardware I/O coherency). The
 * other operations are kept because they are infrequent (therefore do
 * not cause the deadlock in practice) and needed for secondary CPU
 * boot and other power management activities.
 */
static const struct l2c_init_data of_l2c310_coherent_data __initconst = {
	.type = "L2C-310 Coherent",
	.way_size_0 = SZ_8K,
	.num_lock = 8,
	.of_parse = l2c310_of_parse,
	.enable = l2c310_enable,
	.fixup = l2c310_fixup,
	.save  = l2c310_save,
	.configure = l2c310_configure,
	.unlock = l2c310_unlock,
	.outer_cache = {
		.inv_range   = l2c210_inv_range,
		.clean_range = l2c210_clean_range,
		.flush_range = l2c210_flush_range,
		.flush_all   = l2c210_flush_all,
		.disable     = l2c310_disable,
		.resume      = l2c310_resume,
	},
	.sync_reg_offset = L2X0_CACHE_SYNC,
};

#define L2C_ID(name, fns) { .compatible = (name), .data = (void *)&(fns) }
static const struct of_device_id l2x0_ids[] __initconst = {
	L2C_ID("arm,pl310-cache", of_l2c310_data),
	{}
};

static int l2c_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct l2c_init_data *data;
	struct device_node *np = pdev->dev.of_node;
	struct resource res;
	u32 cache_id, old_aux;
	u32 cache_level = 2;
	bool nosync = false;

	u32 aux_val = CACHE_AUX_VALUE;
	u32 aux_mask = CACHE_AUX_MASK;

	data = kzalloc(sizeof(*data), GFP_KERNEL);
	if (!data) {
		ret = -ENOMEM;
		goto fail;
	}

	raw_spin_lock_init(&data->l2x0_lock);

	if (of_device_is_compatible(np, "arm,pl310-cache") &&
	    of_property_read_bool(np, "arm,io-coherent"))
		memcpy(data, &of_l2c310_coherent_data, sizeof(*data));
	else
		memcpy(data, &of_l2c310_data, sizeof(*data));

	if (of_address_to_resource(np, 0, &res)) {
		pr_err("L2C: Failed to acquire register space\n");
		ret = -ENODEV;
		goto fail;
	}

	data->l2x0_base = ioremap(res.start, resource_size(&res));
	if (!data->l2x0_base) {
		pr_err("L2C: Failed to ioremap register space\n");
		ret = -ENOMEM;
		goto fail;
	}

	pr_info("L2C: controller address: 0x%px\n", data->l2x0_base);

	data->l2x0_saved_regs.phy_base = res.start;

	old_aux = readl_relaxed(data->l2x0_base + L2X0_AUX_CTRL);
	if (old_aux != ((old_aux & aux_mask) | aux_val)) {
		pr_warn("L2C: platform modifies aux control register: 0x%08x -> 0x%08x\n",
			old_aux, (old_aux & aux_mask) | aux_val);
	} else if (aux_mask != ~0U && aux_val != 0) {
		pr_alert("L2C: platform provided aux values match the hardware, so have no effect.  Please remove them.\n");
	}

	/* All L2 caches are unified, so this property should be specified */
	if (!of_property_read_bool(np, "cache-unified"))
		pr_err("L2C: device tree omits to specify unified cache\n");

	if (of_property_read_u32(np, "cache-level", &cache_level))
		pr_err("L2C: device tree omits to specify cache-level\n");

	if (cache_level != 2)
		pr_err("L2C: device tree specifies invalid cache level\n");

	nosync = of_property_read_bool(np, "arm,outer-sync-disable");

	/* Read back current (default) hardware configuration */
	if (data->save)
		data->save(data);

	/* L2 configuration can only be changed if the cache is disabled */
	if (!(readl_relaxed(data->l2x0_base + L2X0_CTRL) & L2X0_CTRL_EN))
		if (data->of_parse)
			data->of_parse(data, np, &aux_val, &aux_mask);

	cache_id = readl_relaxed(data->l2x0_base + L2X0_CACHE_ID);

	ret = __l2c_init(data, aux_val, aux_mask, cache_id, nosync);

	if (ret) {
		pr_err("L2C: Failed to initialize L2C\n");
		goto fail;
	}

	platform_set_drvdata(pdev, data);

	pr_info("Mero SysCache detected: %s\n", np->full_name);

	return 0;

fail:
	if (data->l2x0_base)
		iounmap(data->l2x0_base);

	kfree(data);

	return ret;
}

static int l2c_remove(struct platform_device *pdev)
{
	struct l2c_init_data *data = platform_get_drvdata(pdev);

	if (data->l2x0_base)
		iounmap(data->l2x0_base);

	__l2c_exit(data);

	kfree(data);

	return 0;
}
#endif

struct platform_driver mero_syscache_drv = {
	.driver = {
		.owner	= THIS_MODULE,
		.name	= "mero-syscache",
		.of_match_table = of_match_ptr(l2x0_ids),
	},
	.probe		= l2c_probe,
	.remove		= l2c_remove,
};

static void __exit
l2c_exit(void)
{
	platform_driver_unregister(&mero_syscache_drv);
}

static int __init
l2c_init(void)
{
	return platform_driver_register(&mero_syscache_drv);
}

module_init(l2c_init);
module_exit(l2c_exit);
MODULE_DESCRIPTION("MERO System Cache Driver");
