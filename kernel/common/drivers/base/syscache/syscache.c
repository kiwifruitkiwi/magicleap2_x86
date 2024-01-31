/*
 * Copyright (C) 2010 ARM Ltd.
 *
 * Modification Copyright (c) 2021, Advanced Micro Devices, Inc.
 * All rights Reserved.
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

#include <linux/of.h>
#include <linux/io.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/resource.h>
#include <linux/interrupt.h>
#include <linux/of_irq.h>
#include <linux/platform_device.h>
#include <linux/arm-smccc.h>
#include <linux/syscache.h>
#include <asm/barrier.h>
#include <linux/reboot.h>

#include "cache-l310.h"
#include "syscache_debugfs.h"
#include "syscache_dev.h"

#define CVIP_SIP_PL310_ENABLE			0x82000101
#define CVIP_SIP_PL310_DISABLE			0x82000102
#define CVIP_SIP_PL310_FLUSH_ALL		0x82000103
#define CVIP_SIP_PL310_FLUSH_RANGE		0x82000104
#define CVIP_SIP_PL310_INV_RANGE		0x82000105
#define CVIP_SIP_PL310_CLEAN_RANGE		0x82000106
#define CVIP_SIP_PL310_SYNC			0x82000107
#define CVIP_SIP_PL310_WR_REG			0x82000108
#define CVIP_SIP_PL310_INV_ALL			0x82000109

#define CVIP_SYSCACHE_CTRL			0x848
#define PL310_NR				0x4

#define CACHE_LINE_SIZE				32
#define CACHE_AUX_VALUE				0x02020000
#define CACHE_AUX_MASK				0x7FFF3C01

/**
 * struct syscache_data - information for system cache init data
 * @cvip_base: base register address of CVIP module
 * @pl310_base: array of base register addresses of pl310 controllers
 * @pl310_res: resoure array of pl310 controllers
 * @way_size_0: size of a cache way
 * @pl310_way_mask: bitmask of active ways
 * @pl310_size: size of each system cache
 * @irqmap: array of system cache irq num
 * @pl310_regs: struct to save registers
 */
struct syscache_data {
	void __iomem *cvip_base;
	void __iomem *pl310_base[PL310_NR];
	struct resource *pl310_res[PL310_NR];
	unsigned int way_size_0;
	u32 pl310_way_mask;
	u32 pl310_size;
	u32 aux_mask;
	int irqmap[PL310_NR];
	struct pl310_regs pl310_regs;
};

static const char * const thread[] = {
	"syscache0", "syscache1", "syscache2", "syscache3"
};

static const char * const intr_err[] = {
	"ECNTR", "PARRT", "PARRD",
	"ERRWT", "ERRWD", "ERRRT",
	"ERRRD", "SLVERR", "DECERR"
};
static int syscache_state = SYSCACHE_STATE_DIS;
static struct syscache_data *scdata;

static int syscache_disable(struct notifier_block *nb,
			    unsigned long event, void *ptr)
{
	syscache_flush_all();
	syscache_enable(SYSCACHE_STATE_DIS);

	return NOTIFY_DONE;
}

static struct notifier_block syscache_panic_notifier = {
	.notifier_call = syscache_disable,
};

static struct notifier_block syscache_reboot_notifier = {
	.notifier_call = syscache_disable,
};

int syscache_enable(unsigned int en)
{
	struct arm_smccc_res res;
	unsigned long cmd =
		en ? CVIP_SIP_PL310_ENABLE : CVIP_SIP_PL310_DISABLE;

	arm_smccc_smc(cmd, 0, 0, 0, 0, 0, 0, 0, &res);
	if (res.a0)
		pr_err("SysCache: failed to %sable system cache!\n",
		       en ? "en" : "dis");
	else
		syscache_state = en;

	return res.a0;
}
EXPORT_SYMBOL(syscache_enable);

static ssize_t
syscache_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", syscache_state);
}

static ssize_t
syscache_store(struct device *dev, struct device_attribute *attr,
	       const char *buf, size_t count)
{
	int ret_val;
	unsigned long val;

	ret_val = kstrtoul(buf, 10, &val);
	if (ret_val)
		return ret_val;

	if (val == 0) {
		syscache_enable(SYSCACHE_STATE_DIS);
	} else if (val == 1) {
		syscache_enable(SYSCACHE_STATE_EN);
	} else {
		pr_err("SysCache: unsupport cmd!\n");
		return -ERANGE;
	}

	return count;
}

static DEVICE_ATTR_RW(syscache);

int syscache_flush_all(void)
{
	struct arm_smccc_res res;
	unsigned long cmd = CVIP_SIP_PL310_FLUSH_ALL;

	arm_smccc_smc(cmd, 0, 0, 0, 0, 0, 0, 0, &res);

	return res.a0;
}
EXPORT_SYMBOL(syscache_flush_all);

static int pl310_flush_range(unsigned long start, unsigned long end)
{
	struct arm_smccc_res res;
	unsigned long cmd = CVIP_SIP_PL310_FLUSH_RANGE;

	arm_smccc_smc(cmd, start, end, 0, 0, 0, 0, 0, &res);

	return res.a0;
}

int syscache_flush_range(unsigned long start, unsigned long end)
{
	return pl310_flush_range(start, end);
}
EXPORT_SYMBOL(syscache_flush_range);

static int pl310_inv_range(unsigned long start, unsigned long end)
{
	struct arm_smccc_res res;
	unsigned long cmd = CVIP_SIP_PL310_INV_RANGE;

	arm_smccc_smc(cmd, start, end, 0, 0, 0, 0, 0, &res);

	return res.a0;
}

int syscache_inv_range(unsigned long start, unsigned long end)
{
	int ret;

	if (start & (CACHE_LINE_SIZE - 1)) {
		start &= ~(CACHE_LINE_SIZE - 1);
		ret = pl310_flush_range(start, start + CACHE_LINE_SIZE);
		if (ret)
			return ret;
		start += CACHE_LINE_SIZE;
	}

	if (end & (CACHE_LINE_SIZE - 1)) {
		end &= ~(CACHE_LINE_SIZE - 1);
		ret = pl310_flush_range(end, end + CACHE_LINE_SIZE);
		if (ret)
			return ret;
	}

	return pl310_inv_range(start, end);
}
EXPORT_SYMBOL(syscache_inv_range);

int syscache_inv_all(void)
{
	struct arm_smccc_res res;
	unsigned long cmd = CVIP_SIP_PL310_INV_ALL;

	arm_smccc_smc(cmd, 0, 0, 0, 0, 0, 0, 0, &res);

	return res.a0;
}
EXPORT_SYMBOL(syscache_inv_all);

int syscache_clean_range(unsigned long start, unsigned long end)
{
	struct arm_smccc_res res;
	unsigned long cmd = CVIP_SIP_PL310_CLEAN_RANGE;

	arm_smccc_smc(cmd, start, end, 0, 0, 0, 0, 0, &res);

	return res.a0;
}
EXPORT_SYMBOL(syscache_clean_range);

int syscache_sync(void)
{
	struct arm_smccc_res res;
	unsigned long cmd = CVIP_SIP_PL310_SYNC;

	arm_smccc_smc(cmd, 0, 0, 0, 0, 0, 0, 0, &res);

	return res.a0;
}
EXPORT_SYMBOL(syscache_sync);

static int pl310_write_sec(u32 mask, u32 val, enum ctrler_idx idx, u32 reg)
{
	struct arm_smccc_res res;
	unsigned long base;
	unsigned long cmd = CVIP_SIP_PL310_WR_REG;

	if (idx < PL310_CTRLER0 || idx > PL310_CTRLER3) {
		pr_warn("Syscache: invalid controller index %d\n", idx);
		return -ERANGE;
	}

	if (!scdata || !scdata->pl310_res[idx])
		return -ENODEV;

	base = scdata->pl310_res[idx]->start;

	arm_smccc_smc(cmd, mask, val, base, reg, 0, 0, 0, &res);

	return res.a0;
}

/*
 * Modify the bits specified by mask to a register.
 */
int syscache_write_reg(u32 mask, u32 val, enum ctrler_idx idx, u32 reg)
{
	return pl310_write_sec(mask, val, idx, reg);
}
EXPORT_SYMBOL(syscache_write_reg);

unsigned int syscache_read_reg(enum ctrler_idx idx, u32 reg)
{
	if (idx < PL310_CTRLER0 || idx > PL310_CTRLER3) {
		pr_warn("Syscache: invalid controller index %d\n", idx);
		return -ERANGE;
	}

	if (scdata && scdata->pl310_base[idx])
		return readl_relaxed(scdata->pl310_base[idx] + reg);
	else
		return -ENODEV;
}
EXPORT_SYMBOL(syscache_read_reg);

static void pl310_save(void __iomem *base, struct pl310_regs *regs)
{
	regs->ctrl = readl_relaxed(base + L310_CTRL);
	regs->aux_ctrl = readl_relaxed(base + L310_AUX_CTRL);
	regs->tag_latency = readl_relaxed(base + L310_TAG_LATENCY_CTRL);
	regs->data_latency = readl_relaxed(base + L310_DATA_LATENCY_CTRL);
	regs->filter_end = readl_relaxed(base + L310_ADDR_FILTER_END);
	regs->filter_start = readl_relaxed(base + L310_ADDR_FILTER_START);
	regs->dbg_ctrl = readl_relaxed(base + L310_DEBUG_CTRL);
	regs->prefetch_ctrl = readl_relaxed(base + L310_PREFETCH_CTRL);
	regs->pwr_ctrl = readl_relaxed(base + L310_POWER_CTRL);
}

struct pl310_regs *syscache_save(enum ctrler_idx idx)
{
	if (idx < PL310_CTRLER0 || idx > PL310_CTRLER3) {
		pr_warn("Syscache: invalid controller index %d\n", idx);
		return ERR_PTR(-ERANGE);
	}

	if (scdata && scdata->pl310_base[idx]) {
		pl310_save(scdata->pl310_base[idx], &scdata->pl310_regs);
		return &scdata->pl310_regs;
	}

	return ERR_PTR(-ENODEV);
}
EXPORT_SYMBOL(syscache_save);

int setup_syscache_axcache(unsigned int value)
{
	if (scdata && scdata->cvip_base) {
		writel_relaxed(value, scdata->cvip_base + CVIP_SYSCACHE_CTRL);
		mb();
		return 0;
	}

	pr_err("Syscache: failed to setup axcache\n");

	return -ENODEV;
}
EXPORT_SYMBOL(setup_syscache_axcache);

static inline int find_cacheid_by_irq(struct syscache_data *pd, int irq)
{
	int i;

	for (i = 0; i < PL310_NR; i++)
		if (pd->irqmap[i] == irq)
			return i;

	return -EINVAL;
}

static irqreturn_t syscache_thread_fn(int irq, void *p)
{
	struct syscache_data *pd = p;
	unsigned long err, val;
	int idx;

	idx = find_cacheid_by_irq(pd, irq);
	if (idx < 0) {
		pr_warn("Syscache: cannot find valid idx\n");
		return IRQ_NONE;
	}

	val = readl_relaxed(pd->pl310_base[idx] + L310_MASKED_INTR_STAT);
	if (val) {
		err = find_first_bit(&val, L310_INTR_NUM);
		pr_debug("Syscache: %s occurred\n", intr_err[err]);
		writel_relaxed(val, pd->pl310_base[idx] + L310_INTR_CLEAR);
	}

	return IRQ_HANDLED;
}

static bool all_cache_disabled(struct syscache_data *d)
{
	return !((readl_relaxed(d->pl310_base[0] + L310_CTRL) & L310_CTRL_EN) ||
		 (readl_relaxed(d->pl310_base[1] + L310_CTRL) & L310_CTRL_EN) ||
		 (readl_relaxed(d->pl310_base[2] + L310_CTRL) & L310_CTRL_EN) ||
		 (readl_relaxed(d->pl310_base[3] + L310_CTRL) & L310_CTRL_EN));
}

static void pl310_configure(struct syscache_data *d)
{
	u32 idx;
	u32 pwr_mask = L310_DYNAMIC_CLK_GATING_EN | L310_STNDBY_MODE_EN;

	for (idx = 0; idx < PL310_NR; idx++) {
		if (pl310_write_sec(d->aux_mask, d->pl310_regs.aux_ctrl,
				    idx, L310_AUX_CTRL))
			pr_err("%s: failed to write 0x%lx to 0x%x reg of #%d\n",
			       __func__, d->pl310_regs.aux_ctrl,
			       L310_AUX_CTRL, idx);

		if (pl310_write_sec(pwr_mask, d->pl310_regs.pwr_ctrl,
				    idx, L310_POWER_CTRL))
			pr_err("%s: failed to write 0x%lx to 0x%x reg of #%d\n",
			       __func__, d->pl310_regs.pwr_ctrl,
			       L310_POWER_CTRL, idx);

		/* unmask all of the interrupts */
		if (pl310_write_sec(L310_INTR_STAT_MASK, L310_INTR_STAT_MASK,
				    idx, L310_INTR_MASK))
			pr_err("%s: failed to write 0x%x to 0x%x reg of #%d\n",
			       __func__, L310_INTR_STAT_MASK,
			       L310_INTR_MASK, idx);
	}
}

static void __pl310_init(struct syscache_data *data)
{
	unsigned int way_size_bits, ways;
	u32 aux = data->pl310_regs.aux_ctrl;

	if (aux & L310_AUX_CTRL_ASSOC_MASK)
		ways = 16;
	else
		ways = 8;

	data->pl310_way_mask = (1 << ways) - 1;

	/*
	 * way_size_0 is the size that a way_size value of zero would be
	 * given the calculation: way_size = way_size_0 << way_size_bits.
	 * So, if way_size_bits=0 is reserved, but way_size_bits=1 is 16k,
	 * then way_size_0 would be 8k.
	 *
	 * syscache size = number of ways * way size.
	 */
	data->way_size_0 = SZ_8K;
	way_size_bits = (aux & L310_AUX_CTRL_WAY_SIZE_MASK) >>
			 L310_AUX_CTRL_WAY_SIZE_SHIFT;
	data->pl310_size = ways * (data->way_size_0 << way_size_bits);

	pl310_configure(data);

	/*
	 * Check if pl310 controller is already enabled.  If we are booting
	 * in non-secure mode accessing the below registers will fault.
	 */
	if (all_cache_disabled(data))
		syscache_enable(SYSCACHE_STATE_EN);

	/*
	 * It is strange to save the register state before initialisation,
	 * but hey, this is what the DT implementations decided to do.
	 */
	syscache_save(PL310_CTRLER0);
}

/**
 * pl310_cache_size_of_parse() - read cache size parameters from DT
 * @np: the device tree node for the system cache
 * @aux_val: pointer to machine-supplied auxiliary register value, to
 * be augmented by the call (bits to be set to 1)
 * @aux_mask: pointer to machine-supplied auxiliary register mask, to
 * be augmented by the call (bits to be set to 0)
 * @associativity: variable to return the calculated associativity in
 * @max_way_size: the maximum size in bytes for the cache ways
 */
static int pl310_cache_size_of_parse(const struct device_node *np,
				     u32 *aux_val, u32 *aux_mask,
				     u32 *associativity, u32 max_way_size)
{
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

	/* All these system caches have the same line = block size actually */
	if (!line_size) {
		if (block_size) {
			/* If linesize is not given, it is equal to blocksize */
			line_size = block_size;
		} else {
			/* Fall back to known size */
			pr_warn("Syscache: no cache block/line size given: "
				"falling back to default size %d bytes\n",
				CACHE_LINE_SIZE);
			line_size = CACHE_LINE_SIZE;
		}
	}

	if (line_size != CACHE_LINE_SIZE)
		pr_warn("Syscache: DT supplied line size %d bytes does "
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
		pr_err("Syscache: set size %dKB is too large\n", way_size);
		return -EINVAL;
	}

	pr_info("Syscache: override cache size: %d bytes (%dKB)\n",
		cache_size, cache_size >> 10);
	pr_info("Syscache: override line size: %d bytes\n", line_size);
	pr_info("Syscache: override way size: %d bytes (%dKB)\n",
		way_size, way_size >> 10);
	pr_info("Syscache: override associativity: %d\n", *associativity);

	/*
	 * Calculates the bits 17:19 to set for way size:
	 * 512KB -> 6, 256KB -> 5, ... 16KB -> 1
	 */
	way_size_bits = ilog2(way_size >> 10) - 3;
	if (way_size_bits < 1 || way_size_bits > 6) {
		pr_err("Syscache: cache way size illegal: %dKB is not mapped\n",
		       way_size);
		return -EINVAL;
	}

	*aux_val &= ~L310_AUX_CTRL_WAY_SIZE_MASK;
	*aux_val |= L310_AUX_CTRL_WAY_SIZE(way_size_bits);
	*aux_mask |= L310_AUX_CTRL_WAY_SIZE_MASK;

	return 0;
}

static void pl310_of_parse(struct syscache_data *d,
			   const struct device_node *np)
{
	u32 data[3] = { 0, 0, 0 };
	u32 tag[3] = { 0, 0, 0 };
	u32 filter[2] = { 0, 0 };
	u32 *aux_mask = &d->aux_mask;
	u32 assoc, prefetch, power, val, aux_val;
	struct pl310_regs *regs = &d->pl310_regs;
	int ret;

	ret = of_property_read_u32_array(np, "arm,tag-latency", tag,
					 ARRAY_SIZE(tag));
	if (ret)
		pr_err("Syscache: cannot find arm,tag-latency property value\n");
	else
		if (tag[0] && tag[1] && tag[2])
			regs->tag_latency =
				L310_LATENCY_CTRL_RD(tag[0] - 1) |
				L310_LATENCY_CTRL_WR(tag[1] - 1) |
				L310_LATENCY_CTRL_SETUP(tag[2] - 1);

	ret = of_property_read_u32_array(np, "arm,data-latency", data,
					 ARRAY_SIZE(data));
	if (ret)
		pr_err("Syscache: cannot find arm,data-latency property value\n");
	else
		if (data[0] && data[1] && data[2])
			regs->data_latency =
				L310_LATENCY_CTRL_RD(data[0] - 1) |
				L310_LATENCY_CTRL_WR(data[1] - 1) |
				L310_LATENCY_CTRL_SETUP(data[2] - 1);

	ret = of_property_read_u32_array(np, "arm,filter-ranges", filter,
					 ARRAY_SIZE(filter));
	if (ret)
		pr_err("Syscache: cannot find arm,filter-ranges property value\n");
	else
		if (filter[1]) {
			regs->filter_end = ALIGN(filter[0] + filter[1], SZ_1M);
			d->pl310_regs.filter_start = (filter[0] & ~(SZ_1M - 1))
						| L310_ADDR_FILTER_EN;
		}

	aux_val = regs->aux_ctrl;

	ret = pl310_cache_size_of_parse(np, &aux_val, aux_mask,
					&assoc, SZ_512K);
	if (!ret) {
		switch (assoc) {
		case 16:
			aux_val |= L310_AUX_CTRL_ASSOCIATIVITY;
			*aux_mask |= L310_AUX_CTRL_ASSOC_MASK;
			break;
		case 8:
			aux_val &= ~L310_AUX_CTRL_ASSOCIATIVITY;
			*aux_mask |= L310_AUX_CTRL_ASSOC_MASK;
			break;
		default:
			pr_err("System cache associativity %d invalid,"
			       "only 8 or 16 permitted\n", assoc);
			break;
		}
	}

	if (of_property_read_bool(np, "arm,event-monitor")) {
		aux_val |= L310_AUX_CTRL_EVTMON_ENABLE;
		*aux_mask |= L310_AUX_CTRL_EVTMON_ENABLE;
	}

	if (of_property_read_bool(np, "arm,shared-override")) {
		aux_val |= L310_AUX_CTRL_SHARED_OVERRIDE;
		*aux_mask |= L310_AUX_CTRL_SHARED_OVERRIDE;
	}

	if (of_property_read_bool(np, "arm,parity-enable")) {
		aux_val |= L310_AUX_CTRL_PARITY_ENABLE;
		*aux_mask |= L310_AUX_CTRL_PARITY_ENABLE;
	} else if (of_property_read_bool(np, "arm,parity-disable")) {
		aux_val &= ~L310_AUX_CTRL_PARITY_ENABLE;
		*aux_mask |= L310_AUX_CTRL_PARITY_ENABLE;
	}

	if (of_property_read_bool(np, "arm,ns-int-ctrl")) {
		aux_val |= L310_AUX_CTRL_NS_INT_CTRL;
		*aux_mask |= L310_AUX_CTRL_NS_INT_CTRL;
	}

	regs->aux_ctrl = aux_val;

	prefetch = regs->prefetch_ctrl;

	ret = of_property_read_u32(np, "arm,double-linefill", &val);
	if (ret) {
		pr_err("Syscache: cannot find arm,double-linefill property value\n");
	} else {
		if (val)
			prefetch |= L310_PREFETCH_CTRL_DBL_LINEFILL;
		else
			prefetch &= ~L310_PREFETCH_CTRL_DBL_LINEFILL;
	}

	ret = of_property_read_u32(np, "arm,double-linefill-incr", &val);
	if (ret) {
		pr_err("Syscache: cannot find arm,double-linefill-incr property value\n");
	} else {
		if (val)
			prefetch |= L310_PREFETCH_CTRL_DBL_LINEFILL_INCR;
		else
			prefetch &= ~L310_PREFETCH_CTRL_DBL_LINEFILL_INCR;
	}

	ret = of_property_read_u32(np, "arm,double-linefill-wrap", &val);
	if (ret) {
		pr_err("Syscache: cannot find arm,double-linefill-wrap property value\n");
	} else {
		if (!val)
			prefetch |= L310_PREFETCH_CTRL_DBL_LINEFILL_WRAP;
		else
			prefetch &= ~L310_PREFETCH_CTRL_DBL_LINEFILL_WRAP;
	}

	ret = of_property_read_u32(np, "arm,prefetch-drop", &val);
	if (ret) {
		pr_err("Syscache: cannot find arm,prefetch-drop property value\n");
	} else {
		if (val)
			prefetch |= L310_PREFETCH_CTRL_PREFETCH_DROP;
		else
			prefetch &= ~L310_PREFETCH_CTRL_PREFETCH_DROP;
	}

	ret = of_property_read_u32(np, "arm,prefetch-offset", &val);
	if (ret) {
		pr_err("Syscache: cannot find arm,prefetch-offset property value\n");
	} else {
		prefetch &= ~L310_PREFETCH_CTRL_OFFSET_MASK;
		prefetch |= val & L310_PREFETCH_CTRL_OFFSET_MASK;
	}

	ret = of_property_read_u32(np, "prefetch-data", &val);
	if (ret) {
		pr_err("Syscache: cannot find prefetch-data property value\n");
	} else {
		if (val)
			prefetch |= L310_PREFETCH_CTRL_DATA_PREFETCH;
		else
			prefetch &= ~L310_PREFETCH_CTRL_DATA_PREFETCH;
	}

	ret = of_property_read_u32(np, "prefetch-instr", &val);
	if (ret) {
		pr_err("Syscache: cannot find prefetch-instr property value\n");
	} else {
		if (val)
			prefetch |= L310_PREFETCH_CTRL_INSTR_PREFETCH;
		else
			prefetch &= ~L310_PREFETCH_CTRL_INSTR_PREFETCH;
	}

	regs->prefetch_ctrl = prefetch;

	power = regs->pwr_ctrl;

	ret = of_property_read_u32(np, "arm,dynamic-clock-gating", &val);
	if (ret)
		pr_err("Syscache: cannot find arm,dynamic-clock-gating property value\n");
	else
		if (val)
			power |= L310_DYNAMIC_CLK_GATING_EN;

	ret = of_property_read_u32(np, "arm,standby-mode", &val);
	if (ret)
		pr_err("Syscache: cannot find arm,standby-mode property value\n");
	else
		if (val)
			power |= L310_STNDBY_MODE_EN;

	regs->pwr_ctrl = power;
}

static int syscache_probe(struct platform_device *pdev)
{
	struct syscache_data *pdata;
	struct resource *res;
	u32 *aux_mask;
	int ret;
	int i;

	pr_info("Mero SysCache detected: %s\n", pdev->name);

	pdata = devm_kzalloc(&pdev->dev, sizeof(*pdata), GFP_KERNEL);
	if (!pdata)
		return -ENOMEM;

	scdata = pdata;
	aux_mask = &pdata->aux_mask;

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "cvip");
	if (!res)
		return -EINVAL;

	pdata->cvip_base =
		devm_ioremap(&pdev->dev, res->start, resource_size(res));
	if (!pdata->cvip_base) {
		pr_err("SysCache: cannot map cvip regs region\n");
		return -ENOMEM;
	}

	for (i = 0; i < PL310_NR; i++) {
		pdata->pl310_res[i] =
			platform_get_resource(pdev, IORESOURCE_MEM, i);
		if (!pdata->pl310_res[i])
			return -EINVAL;

		pdata->pl310_base[i] =
			devm_ioremap_resource(&pdev->dev, pdata->pl310_res[i]);
		if (IS_ERR(pdata->pl310_base[i])) {
			pr_err("SysCache: cannot map controllors registers\n");
			return PTR_ERR(pdata->pl310_base[i]);
		}

		pdata->irqmap[i] = platform_get_irq(pdev, i);
		if (pdata->irqmap[i] < 0) {
			pr_err("SysCache: cannot get irq for syscache%d\n", i);
			return pdata->irqmap[i];
		}

		ret = devm_request_threaded_irq(&pdev->dev, pdata->irqmap[i],
						NULL, syscache_thread_fn,
						IRQF_TRIGGER_HIGH |
						IRQF_ONESHOT,
						thread[i], pdata);
		if (ret) {
			pr_err("SysCache: cannot request irq for cache%d\n", i);
			return ret;
		}
	}

	syscache_save(PL310_CTRLER0);

	/* syscache configuration can only be changed if cache is disabled */
	if (all_cache_disabled(pdata)) {
		pl310_of_parse(pdata, pdev->dev.of_node);
		__pl310_init(pdata);
	}

	platform_set_drvdata(pdev, pdata);

	setup_syscache_axcache(ARCACHE(WRBK_RDWRALC_CACHEABLE) |
			       AWCACHE(WRBK_RDWRALC_CACHEABLE));

	device_create_file(&pdev->dev, &dev_attr_syscache);

	syscache_debugfs_create(pdata->pl310_base);
	ret = syscache_dev_create(pdata->pl310_base);
	if (ret != 0) {
		pr_err("error in syscache device creation : %d\n", ret);
		return ret;
	}

	atomic_notifier_chain_register(&panic_notifier_list,
				       &syscache_panic_notifier);

	register_reboot_notifier(&syscache_reboot_notifier);

	return 0;
}

static int syscache_remove(struct platform_device *pdev)
{
	unregister_reboot_notifier(&syscache_reboot_notifier);
	atomic_notifier_chain_unregister(&panic_notifier_list,
					 &syscache_panic_notifier);
	syscache_debugfs_remove();
	syscache_dev_remove();
	device_remove_file(&pdev->dev, &dev_attr_syscache);
	syscache_enable(SYSCACHE_STATE_DIS);
	scdata = NULL;
	return 0;
}

#ifdef CONFIG_PM
static int syscache_suspend(struct platform_device *pdev, pm_message_t state)
{
	syscache_flush_all();
	syscache_enable(SYSCACHE_STATE_DIS);

	return 0;
}

static int syscache_resume(struct platform_device *pdev)
{
	struct syscache_data *pdata = platform_get_drvdata(pdev);

	pl310_configure(pdata);
	return syscache_enable(SYSCACHE_STATE_EN);
}
#else
#define syscache_suspend NULL
#define syscache_resume NULL
#endif

static const struct of_device_id syscache_ids[] __initconst = {
	{.compatible = "amd,mero-syscache",},
	{ }
};

static struct platform_driver mero_syscache_drv = {
	.driver = {
		.owner	= THIS_MODULE,
		.name	= "mero-syscache",
		.of_match_table = of_match_ptr(syscache_ids),
	},
	.probe		= syscache_probe,
	.remove		= syscache_remove,
	.suspend	= syscache_suspend,
	.resume		= syscache_resume,
};

static void __exit syscache_exit(void)
{
	platform_driver_unregister(&mero_syscache_drv);
}

static int __init syscache_init(void)
{
	return platform_driver_register(&mero_syscache_drv);
}

module_init(syscache_init);
module_exit(syscache_exit);
MODULE_DESCRIPTION("MERO System Cache Driver");
