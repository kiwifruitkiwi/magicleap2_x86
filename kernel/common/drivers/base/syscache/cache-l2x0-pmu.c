/*
 * L220/L310 cache controller support
 *
 * Copyright (C) 2016 ARM Limited
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

#include <linux/errno.h>
#include <linux/hrtimer.h>
#include <linux/io.h>
#include <linux/list.h>
#include <linux/perf_event.h>
#include <linux/printk.h>
#include <linux/slab.h>
#include <linux/types.h>

#include "cache-l2x0.h"

#define PMU_NR_COUNTERS 2
#define PMU_NAME_MAX_LEN 32

/**
 * struct l2x0_pmu - struct used for perf_event framework
 * @base: pmu struct base
 * @l2x0_base: base register address of l2x0 instance
 * @pmu_cpu: cpu mask
 * @l2x0_name: name of the sysfs node
 * @l2x0_pmu_poll_period: polling period for timer
 * @l2x0_pmu_hrtimer: timer for handling counter overflow
 * @events/event_bitmap/event_mutex: save perf_event pointer
 */
struct l2x0_pmu {
	struct pmu base;
	void __iomem *l2x0_base;
	cpumask_t pmu_cpu;
	char l2x0_name[PMU_NAME_MAX_LEN];
	ktime_t l2x0_pmu_poll_period;
	struct hrtimer l2x0_pmu_hrtimer;
	/*
	 * The L220/PL310 PMU has two equivalent counters, Counter1 and
	 * Counter0. Registers controlling these are laid out in pairs, in
	 * descending order, i.e. the register for Counter1 comes first,
	 * followed by the register for Counter0.
	 * We ensure that idx 0 -> Counter0, and idx1 -> Counter1.
	 */
	struct perf_event *events[PMU_NR_COUNTERS];
	unsigned long event_bitmap;
	struct mutex event_mutex; /* protects event_bitmap */
};

static inline void l2x0_pmu_counter_config_write(struct l2x0_pmu *pmu,
						 int idx, u32 val)
{
	void __iomem *addr = pmu->l2x0_base + L2X0_EVENT_CNT0_CFG - 4 * idx;

	writel_relaxed(val, addr);
}

static inline u32 l2x0_pmu_counter_read(struct l2x0_pmu *pmu, int idx)
{
	return readl_relaxed(pmu->l2x0_base +
		L2X0_EVENT_CNT0_VAL - 4 * idx);
}

static inline void l2x0_pmu_counter_write(struct l2x0_pmu *pmu,
					  int idx, u32 val)
{
	writel_relaxed(val, pmu->l2x0_base +
		L2X0_EVENT_CNT0_VAL - 4 * idx);
}

static inline void __l2x0_pmu_enable(struct l2x0_pmu *pmu)
{
	u32 val = readl_relaxed(pmu->l2x0_base + L2X0_EVENT_CNT_CTRL);

	val |= L2X0_EVENT_CNT_CTRL_ENABLE;
	writel_relaxed(val, pmu->l2x0_base + L2X0_EVENT_CNT_CTRL);
}

static inline void __l2x0_pmu_disable(struct l2x0_pmu *pmu)
{
	u32 val = readl_relaxed(pmu->l2x0_base + L2X0_EVENT_CNT_CTRL);

	val &= ~L2X0_EVENT_CNT_CTRL_ENABLE;
	writel_relaxed(val, pmu->l2x0_base + L2X0_EVENT_CNT_CTRL);
}

static void l2x0_pmu_enable(struct pmu *pmu)
{
	struct l2x0_pmu *l2x0_pmu = container_of(pmu,
		struct l2x0_pmu, base);

	mutex_lock(&l2x0_pmu->event_mutex);

	if (bitmap_empty(&l2x0_pmu->event_bitmap, PMU_NR_COUNTERS))
		__l2x0_pmu_enable(l2x0_pmu);
	else
		pr_err("Failed to enable PL310 PMU. Perf counter is busy\n");

	mutex_unlock(&l2x0_pmu->event_mutex);
}

static void l2x0_pmu_disable(struct pmu *pmu)
{
	struct l2x0_pmu *l2x0_pmu = container_of(pmu,
		struct l2x0_pmu, base);

	mutex_lock(&l2x0_pmu->event_mutex);

	if (bitmap_empty(&l2x0_pmu->event_bitmap, PMU_NR_COUNTERS))
		__l2x0_pmu_disable(l2x0_pmu);
	else
		pr_err("Failed to disable PL310 PMU. Perf counter is busy\n");

	mutex_unlock(&l2x0_pmu->event_mutex);
}

static inline void warn_if_saturated(u32 count)
{
	if (unlikely(count == 0xffffffff))
		pr_warn_ratelimited("L2X0 counter saturated. Poll period too long\n");
}

static void l2x0_pmu_event_read(struct perf_event *event)
{
	struct l2x0_pmu *l2x0_pmu = container_of(event->pmu,
		struct l2x0_pmu, base);

	struct hw_perf_event *hw = &event->hw;

	u64 prev_count, new_count, mask;

	do {
		prev_count = local64_read(&hw->prev_count);
		new_count = l2x0_pmu_counter_read(l2x0_pmu, hw->idx);
	} while (local64_xchg(&hw->prev_count, new_count) != prev_count);

	mask = GENMASK_ULL(31, 0);
	local64_add((new_count - prev_count) & mask, &event->count);

	warn_if_saturated(new_count);
}

static void l2x0_pmu_event_configure(struct l2x0_pmu *pmu,
				     struct perf_event *event)
{
	struct hw_perf_event *hw = &event->hw;

	/*
	 * The L2X0 counters saturate at 0xffffffff rather than wrapping, so we
	 * will *always* lose some number of events when a counter saturates,
	 * and have no way of detecting how many were lost.
	 *
	 * To minimize the impact of this, we try to maximize the period by
	 * always starting counters at zero. To ensure that group ratios are
	 * representative, we poll periodically to avoid counters saturating.
	 * See l2x0_pmu_poll().
	 */
	local64_set(&hw->prev_count, 0);
	l2x0_pmu_counter_write(pmu, hw->idx, 0);
}

static enum hrtimer_restart l2x0_pmu_poll(struct hrtimer *hrtimer)
{
	unsigned long flags;
	int i;
	struct l2x0_pmu *l2x0_pmu = container_of(hrtimer, struct l2x0_pmu,
		l2x0_pmu_hrtimer);


	local_irq_save(flags);
	__l2x0_pmu_disable(l2x0_pmu);

	for (i = 0; i < PMU_NR_COUNTERS; i++) {
		struct perf_event *event = l2x0_pmu->events[i];

		if (!event)
			continue;

		l2x0_pmu_event_read(event);
		l2x0_pmu_event_configure(l2x0_pmu, event);
	}

	__l2x0_pmu_enable(l2x0_pmu);
	local_irq_restore(flags);

	hrtimer_forward_now(hrtimer, l2x0_pmu->l2x0_pmu_poll_period);
	return HRTIMER_RESTART;
}


static inline void __l2x0_pmu_event_enable(struct l2x0_pmu *pmu,
					   int idx, u32 event)
{
	u32 val = event << L2X0_EVENT_CNT_CFG_SRC_SHIFT;
	val |= L2X0_EVENT_CNT_CFG_INT_DISABLED;
	l2x0_pmu_counter_config_write(pmu, idx, val);
}

static void l2x0_pmu_event_start(struct perf_event *event, int flags)
{
	struct hw_perf_event *hw = &event->hw;

	struct l2x0_pmu *l2x0_pmu = container_of(event->pmu,
		struct l2x0_pmu, base);

	if (WARN_ON_ONCE(!(event->hw.state & PERF_HES_STOPPED)))
		return;

	if (flags & PERF_EF_RELOAD) {
		WARN_ON_ONCE(!(hw->state & PERF_HES_UPTODATE));
		l2x0_pmu_event_configure(l2x0_pmu, event);
	}

	hw->state = 0;

	__l2x0_pmu_event_enable(l2x0_pmu, hw->idx, hw->config_base);
}

static inline void __l2x0_pmu_event_disable(struct l2x0_pmu *pmu,
					    int idx)
{
	u32 val = L2X0_EVENT_CNT_CFG_SRC_DISABLED;

	val <<= L2X0_EVENT_CNT_CFG_SRC_SHIFT;
	val |= L2X0_EVENT_CNT_CFG_INT_DISABLED;
	l2x0_pmu_counter_config_write(pmu, idx, val);
}

static void l2x0_pmu_event_stop(struct perf_event *event, int flags)
{
	struct hw_perf_event *hw = &event->hw;

	struct l2x0_pmu *l2x0_pmu = container_of(event->pmu,
		struct l2x0_pmu, base);

	if (WARN_ON_ONCE(event->hw.state & PERF_HES_STOPPED))
		return;

	__l2x0_pmu_event_disable(l2x0_pmu, hw->idx);

	hw->state |= PERF_HES_STOPPED;

	if (flags & PERF_EF_UPDATE) {
		l2x0_pmu_event_read(event);
		hw->state |= PERF_HES_UPTODATE;
	}
}

static int l2x0_pmu_event_add(struct perf_event *event, int flags)
{
	struct l2x0_pmu *l2x0_pmu = container_of(event->pmu,
		struct l2x0_pmu, base);
	struct hw_perf_event *hw = &event->hw;
	unsigned long idx;
	int ret;

	ret = mutex_lock_interruptible(&l2x0_pmu->event_mutex);
	if (ret)
		return -ERESTARTSYS;

	idx = find_first_zero_bit(&l2x0_pmu->event_bitmap, PMU_NR_COUNTERS);
	if (idx >= PMU_NR_COUNTERS) {
		mutex_unlock(&l2x0_pmu->event_mutex);
		return -EAGAIN;
	}

	/*
	 * Pin the timer, so that the overflows are handled by the chosen
	 * event->cpu (this is the same one as presented in "cpumask"
	 * attribute).
	 */
	if (bitmap_empty(&l2x0_pmu->event_bitmap, PMU_NR_COUNTERS))
		hrtimer_start(&l2x0_pmu->l2x0_pmu_hrtimer,
			      l2x0_pmu->l2x0_pmu_poll_period,
						HRTIMER_MODE_REL_PINNED);

	bitmap_set(&l2x0_pmu->event_bitmap, idx, 1);

	l2x0_pmu->events[idx] = event;
	hw->idx = idx;

	l2x0_pmu_event_configure(l2x0_pmu, event);

	hw->state = PERF_HES_STOPPED | PERF_HES_UPTODATE;

	if (flags & PERF_EF_START)
		l2x0_pmu_event_start(event, 0);

	mutex_unlock(&l2x0_pmu->event_mutex);

	return 0;
}

static void l2x0_pmu_event_del(struct perf_event *event,
			       int flags)
{
	struct hw_perf_event *hw = &event->hw;
	struct l2x0_pmu *l2x0_pmu = container_of(event->pmu,
		struct l2x0_pmu, base);

	mutex_lock(&l2x0_pmu->event_mutex);

	l2x0_pmu_event_stop(event, PERF_EF_UPDATE);

	if (hw->idx >= 0 && hw->idx < PMU_NR_COUNTERS) {
		bitmap_clear(&l2x0_pmu->event_bitmap, hw->idx, 1);
		l2x0_pmu->events[hw->idx] = NULL;
	}

	hw->idx = -1;

	if (bitmap_empty(&l2x0_pmu->event_bitmap, PMU_NR_COUNTERS))
		hrtimer_cancel(&l2x0_pmu->l2x0_pmu_hrtimer);

	mutex_unlock(&l2x0_pmu->event_mutex);
}

static bool l2x0_pmu_group_is_valid(struct perf_event *event)
{
	struct pmu *pmu = event->pmu;

	struct perf_event *leader = event->group_leader;

	struct perf_event *sibling;

	int num_hw = 0;

	if (leader->pmu == pmu)
		num_hw++;
	else if (!is_software_event(leader))
		return false;

	for_each_sibling_event(sibling, leader) {
		if (sibling->pmu == pmu)
			num_hw++;
		else if (!is_software_event(sibling))
			return false;
	}

	return num_hw <= PMU_NR_COUNTERS;
}

static int l2x0_pmu_event_init(struct perf_event *event)
{
	struct hw_perf_event *hw = &event->hw;

	struct l2x0_pmu *l2x0_pmu = container_of(event->pmu,
		struct l2x0_pmu, base);

	if (event->attr.type != l2x0_pmu->base.type)
		return -ENOENT;

	if (is_sampling_event(event) ||
	    event->attach_state & PERF_ATTACH_TASK)
		return -EINVAL;

	if (event->attr.exclude_user   ||
	    event->attr.exclude_kernel ||
	    event->attr.exclude_hv     ||
	    event->attr.exclude_idle   ||
	    event->attr.exclude_host   ||
	    event->attr.exclude_guest)
		return -EINVAL;

	if (event->cpu < 0)
		return -EINVAL;

	if (event->attr.config & ~L2X0_EVENT_CNT_CFG_SRC_MASK)
		return -EINVAL;

	hw->config_base = event->attr.config;

	if (!l2x0_pmu_group_is_valid(event))
		return -EINVAL;

	event->cpu = cpumask_first(&l2x0_pmu->pmu_cpu);

	return 0;
}

struct l2x0_event_attribute {
	struct device_attribute attr;
	unsigned int config;
	bool pl310_only;
};

#define L2X0_EVENT_ATTR(_name, _config, _pl310_only)				\
	(&((struct l2x0_event_attribute[]) {{					\
		.attr = __ATTR(_name, S_IRUGO, l2x0_pmu_event_show, NULL),	\
		.config = _config,						\
		.pl310_only = _pl310_only,					\
	}})[0].attr.attr)

#define L220_PLUS_EVENT_ATTR(_name, _config)					\
	L2X0_EVENT_ATTR(_name, _config, false)

#define PL310_EVENT_ATTR(_name, _config)					\
	L2X0_EVENT_ATTR(_name, _config, true)

static ssize_t l2x0_pmu_event_show(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	struct l2x0_event_attribute *lattr;

	lattr = container_of(attr, typeof(*lattr), attr);
	return snprintf(buf, PAGE_SIZE, "config=0x%x\n", lattr->config);
}

static umode_t l2x0_pmu_event_attr_is_visible(struct kobject *kobj,
					      struct attribute *attr,
					      int unused)
{
	struct device *dev = kobj_to_dev(kobj);
	struct pmu *pmu = dev_get_drvdata(dev);
	struct l2x0_event_attribute *lattr;

	lattr = container_of(attr, typeof(*lattr), attr.attr);

	if (!lattr->pl310_only || strcmp("l2c_310", pmu->name) == 0)
		return attr->mode;

	return 0;
}

static struct attribute *l2x0_pmu_event_attrs[] = {
	L220_PLUS_EVENT_ATTR(co,	0x1),
	L220_PLUS_EVENT_ATTR(drhit,	0x2),
	L220_PLUS_EVENT_ATTR(drreq,	0x3),
	L220_PLUS_EVENT_ATTR(dwhit,	0x4),
	L220_PLUS_EVENT_ATTR(dwreq,	0x5),
	L220_PLUS_EVENT_ATTR(dwtreq,	0x6),
	L220_PLUS_EVENT_ATTR(irhit,	0x7),
	L220_PLUS_EVENT_ATTR(irreq,	0x8),
	L220_PLUS_EVENT_ATTR(wa,	0x9),
	PL310_EVENT_ATTR(ipfalloc,	0xa),
	PL310_EVENT_ATTR(epfhit,	0xb),
	PL310_EVENT_ATTR(epfalloc,	0xc),
	PL310_EVENT_ATTR(srrcvd,	0xd),
	PL310_EVENT_ATTR(srconf,	0xe),
	PL310_EVENT_ATTR(epfrcvd,	0xf),
	NULL
};

static struct attribute_group l2x0_pmu_event_attrs_group = {
	.name = "events",
	.attrs = l2x0_pmu_event_attrs,
	.is_visible = l2x0_pmu_event_attr_is_visible,
};

static ssize_t l2x0_pmu_cpumask_show(struct device *dev,
				     struct device_attribute *attr, char *buf)
{
	struct cpumask pmu_cpu = {0};

	return cpumap_print_to_pagebuf(true, buf, &pmu_cpu);
}

static struct device_attribute l2x0_pmu_cpumask_attr =
		__ATTR(cpumask, S_IRUGO, l2x0_pmu_cpumask_show, NULL);

static struct attribute *l2x0_pmu_cpumask_attrs[] = {
	&l2x0_pmu_cpumask_attr.attr,
	NULL,
};

static struct attribute_group l2x0_pmu_cpumask_attr_group = {
	.attrs = l2x0_pmu_cpumask_attrs,
};

static const struct attribute_group *l2x0_pmu_attr_groups[] = {
	&l2x0_pmu_event_attrs_group,
	&l2x0_pmu_cpumask_attr_group,
	NULL,
};

static void l2x0_pmu_reset(struct l2x0_pmu *pmu)
{
	int i;

	__l2x0_pmu_disable(pmu);

	for (i = 0; i < PMU_NR_COUNTERS; i++)
		__l2x0_pmu_event_disable(pmu, i);
}

int l2x0_pmu_suspend(struct pmu *pmu)
{
	int i;
	struct l2x0_pmu *l2x0_pmu;

	if (!pmu) {
		pr_err("Suspending a null pmu struct.\n");
		return -EINVAL;
	}

	l2x0_pmu = container_of(pmu, struct l2x0_pmu, base);

	l2x0_pmu_disable(&l2x0_pmu->base);

	for (i = 0; i < PMU_NR_COUNTERS; i++) {
		if (l2x0_pmu->events[i])
			l2x0_pmu_event_stop(l2x0_pmu->events[i],
					    PERF_EF_UPDATE);
	}
	return 0;
}

int l2x0_pmu_resume(struct pmu *pmu)
{
	int i;
	struct l2x0_pmu *l2x0_pmu;

	if (!pmu) {
		pr_err("Resuming a null pmu struct.\n");
		return -EINVAL;
	}

	l2x0_pmu = container_of(pmu, struct l2x0_pmu, base);

	l2x0_pmu_reset(l2x0_pmu);

	for (i = 0; i < PMU_NR_COUNTERS; i++) {
		if (l2x0_pmu->events[i])
			l2x0_pmu_event_start(l2x0_pmu->events[i],
					     PERF_EF_RELOAD);
	}

	l2x0_pmu_enable(&l2x0_pmu->base);
	return 0;
}

void l2x0_pmu_unregister(struct pmu *pmu)
{
	struct l2x0_pmu *l2x0_pmu;

	if (!pmu) {
		pr_err("Unregistering a null pmu struct, skip.\n");
		return;
	}

	l2x0_pmu = container_of(pmu, struct l2x0_pmu, base);

	perf_pmu_unregister(pmu);
	kfree(l2x0_pmu);
}

struct pmu *l2x0_pmu_register(void __iomem *base, u32 part)
{
	struct l2x0_pmu *pmu;
	int ret;

	/*
	 * Determine whether we support the PMU, and choose the name for sysfs.
	 * This is also used by l2x0_pmu_event_attr_is_visible to determine
	 * which events to display, as the PL310 PMU supports a superset of
	 * L220 events.
	 *
	 * The L210 PMU has a different programmer's interface, and is not
	 * supported by this driver.
	 */

	if (!base) {
		pr_err("Invalid input reg base address, skip registering PMU\n");
		return ERR_PTR(-EINVAL);
	}

	pmu = kzalloc(sizeof(*pmu), GFP_KERNEL);

	if (!pmu)
		return ERR_PTR(-ENOMEM);

	pmu->base.task_ctx_nr = perf_invalid_context;
	pmu->base.pmu_enable = l2x0_pmu_enable;
	pmu->base.pmu_disable = l2x0_pmu_disable;
	pmu->base.read = l2x0_pmu_event_read;
	pmu->base.start = l2x0_pmu_event_start;
	pmu->base.stop = l2x0_pmu_event_stop;
	pmu->base.add = l2x0_pmu_event_add;
	pmu->base.del = l2x0_pmu_event_del;
	pmu->base.event_init = l2x0_pmu_event_init;
	pmu->base.attr_groups = l2x0_pmu_attr_groups;
	pmu->l2x0_base = base;

	bitmap_zero(&pmu->event_bitmap, PMU_NR_COUNTERS);
	mutex_init(&pmu->event_mutex);

	l2x0_pmu_reset(pmu);

	/*
	 * We always use a hrtimer rather than an interrupt.
	 * See comments in l2x0_pmu_event_configure and l2x0_pmu_poll.
	 *
	 * Polling once a second allows the counters to fill up to 1/128th on a
	 * quad-core test chip with cores clocked at 400MHz. Hopefully this
	 * leaves sufficient headroom to avoid overflow on production silicon
	 * at higher frequencies.
	 */
	pmu->l2x0_pmu_poll_period = ms_to_ktime(1000);
	hrtimer_init(&pmu->l2x0_pmu_hrtimer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	pmu->l2x0_pmu_hrtimer.function = l2x0_pmu_poll;

	cpumask_set_cpu(0, &pmu->pmu_cpu);

	switch (part & L2X0_CACHE_ID_PART_MASK) {
	case L2X0_CACHE_ID_PART_L220:
		snprintf(pmu->l2x0_name, sizeof(pmu->l2x0_name),
			 "syscache-pl220@%lx", (unsigned long)base);
		break;
	case L2X0_CACHE_ID_PART_L310:
		snprintf(pmu->l2x0_name, sizeof(pmu->l2x0_name),
			 "syscache-pl310@%lx", (unsigned long)base);
		break;
	default:
		pr_err("No matching hardware support for part ID 0x%x\n", part);
		ret = -EINVAL;
		goto fail;
	}

	ret = perf_pmu_register(&pmu->base, pmu->l2x0_name, -1);
	if (ret)
		goto fail;

	return &pmu->base;

fail:
	kfree(pmu);
	return ERR_PTR(ret);

}

