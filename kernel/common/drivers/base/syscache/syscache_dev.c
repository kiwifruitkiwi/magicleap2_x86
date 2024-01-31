/*
 * Char driver interface for AMD Mero SoC CVIP system cache pmu.
 *
 * Copyright (C) 2010 ARM Ltd.
 * Modification Copyright (c) 2020-2021, Advanced Micro Devices, Inc.
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
 */

#include <linux/errno.h>
#include <linux/hrtimer.h>
#include <linux/list.h>
#include <linux/types.h>
#include <linux/atomic.h>
#include <asm-generic/local64.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/resource.h>
#include <linux/platform_device.h>
#include <linux/syscache.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <linux/sysfs.h>
#include <linux/fs.h>
#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/syscache_ioctl.h>

#define SYSCACHE_DEV_NAME	"mero_syscache_pmu"
static int syscache_dev_major;

#define L2X0_NR				0x4
#define PMU_NR_COUNTERS			0x2
#define REG_EV_CNT_CTRL			0x200
#define REG_EV_CNT1_CFG			0x204
#define REG_EV_CNT0_CFG			0x208
#define REG_EV_CNT1			0x20C
#define REG_EV_CNT0			0x210

#define L2X0_EV_CNT_CTRL_EN		BIT(0)
#define L2X0_EV_SHIFT			0x2

/**
 * struct l2x0_pmu_data - information for l2c310 events pmu
 * @base: array of base register address of l2c310 instance
 * @event: array of index of events in counting
 * @count: the events count value
 * @prev_count: the last observed hardware counter value
 * @ev_nr: number of events in counted
 * @l2x0_pmu_poll_period: polling period for timer
 * @l2x0_pmu_hrtimer: timer for handling counter overflow
 */
struct l2x0_pmu_data {
	void __iomem **base;
	u32 event[PMU_NR_COUNTERS];
	local64_t count[PMU_NR_COUNTERS];
	local64_t prev_count[PMU_NR_COUNTERS];
	u32 ev_nr;
	ktime_t l2x0_pmu_poll_period;
	struct hrtimer l2x0_pmu_hrtimer;
};

static struct l2x0_pmu_data l2c310;
static struct class *syscache_class;
static int syscache_dev_major;

static char *ev_list[] = {
	"[0]null",
	"[1]co",
	"[2]drhit",
	"[3]drreq",
	"[4]dwhit",
	"[5]dwreq",
	"[6]dwtreq",
	"[7]irhit",
	"[8]irreq",
	"[9]wa",
	"[10]ipfalloc",
	"[11]epfhit",
	"[12]epfalloc",
	"[13]srrcvd",
	"[14]srconf",
	"[15]epfrcvd",
};

static int syscache_pmu_open(struct inode *node, struct file *fp)
{
	pr_debug("%s opened\n", SYSCACHE_DEV_NAME);
	return 0;
}

static int syscache_pmu_release(struct inode *node, struct file *fp)
{
	pr_debug("%s closed\n", SYSCACHE_DEV_NAME);
	return 0;
}

static ssize_t
syscache_pmu_read(struct file *fp, char __user *buf,
		  size_t size, loff_t *off)
{
	pr_debug("%s read\n", SYSCACHE_DEV_NAME);
	return 0;
}

static ssize_t
syscache_pmu_write(struct file *fp, const char __user *buf,
		   size_t size, loff_t *off)
{
	char msg[256];
	int ret;

	memset(msg, 0, 256);
	ret = simple_write_to_buffer(msg, size, off, buf, size);
	pr_debug("written %s to device = %s\n", msg, SYSCACHE_DEV_NAME);

	return 0;
}

static inline void __l2x0_pmu_enable(void __iomem *base, u32 val)
{
	u32 reg = readl_relaxed(base + REG_EV_CNT_CTRL);

	reg &= ~L2X0_EV_CNT_CTRL_EN;
	reg |= val;
	writel_relaxed(reg, base + REG_EV_CNT_CTRL);

	/* memory barrier */
	mb();
}

static inline int l2x0_pmu_enable(void)
{
	int i;

	for (i = 0; i < L2X0_NR; i++)
		__l2x0_pmu_enable(l2c310.base[i], 1);

	return 0;
}

static inline int l2x0_pmu_disable(void)
{
	int i;

	for (i = 0; i < L2X0_NR; i++)
		__l2x0_pmu_enable(l2c310.base[i], 0);

	return 0;
}

static inline void
__l2x0_counter_config_write(void __iomem *base, int idx, u32 event)
{
	void __iomem *addr = base + REG_EV_CNT0_CFG - 4 * idx;

	writel_relaxed(event << L2X0_EV_SHIFT, addr);

	/* memory barrier */
	mb();
}

static inline void
l2x0_counter_config_write(int idx, u32 event)
{
	int i;

	for (i = 0; i < L2X0_NR; i++)
		__l2x0_counter_config_write(l2c310.base[i], idx, event);
}

static inline u32
__l2x0_counter_config_read(void __iomem *base, int idx)
{
	u32 config_reg = readl_relaxed(base + REG_EV_CNT0_CFG - 4 * idx);

	return config_reg;
}

static inline int l2x0_pmu_get_event_sources(struct ev_src *events)
{
	u32 src0;
	u32 src1;

	src0 = __l2x0_counter_config_read(l2c310.base[0], 0);
	src0 = src0 >> L2X0_EV_SHIFT;

	src1 = __l2x0_counter_config_read(l2c310.base[0], 1);
	src1 = src1 >> L2X0_EV_SHIFT;

	events->src0 = src0;
	events->src1 = src1;

	return 0;
}

static inline void
__l2x0_counter_write(void __iomem *base, int idx, u32 val)
{
	writel_relaxed(val, base + REG_EV_CNT0 - 4 * idx);

	/* memory barrier */
	mb();
}

static inline u32
__l2x0_counter_read(void __iomem *base, int idx)
{
	return readl_relaxed(base + REG_EV_CNT0 - 4 * idx);
}

static inline u64
l2x0_counter_read(int idx)
{
	int i;
	u64 count = 0;

	for (i = 0; i < L2X0_NR; i++)
		count += __l2x0_counter_read(l2c310.base[i], idx);

	return count;
}

static inline void
l2x0_counter_write(int idx, u32 val)
{
	int i;

	for (i = 0; i < L2X0_NR; i++)
		__l2x0_counter_write(l2c310.base[i], idx, val);
}

static void l2x0_counter_reset(int idx)
{
	l2x0_counter_write(idx, 0);
}

static inline void warn_if_saturated(u32 count)
{
	if (unlikely(count == 0xffffffff))
		pr_warn_ratelimited("L2X0 counter saturated. Poll too long\n");
}

static inline int l2x0_pmu_set_event_sources(struct ev_src events)
{
	unsigned long flags;

	l2c310.event[0] = events.src0;
	l2x0_counter_config_write(0, events.src0);

	l2c310.event[1] = events.src1;
	l2x0_counter_config_write(1, events.src1);

	local_irq_save(flags);
	l2x0_pmu_disable();
	l2x0_counter_reset(0);
	l2x0_counter_reset(1);
	l2x0_pmu_enable();
	local_irq_restore(flags);

	return 0;
}

static long syscache_pmu_ops_ioctl(struct file *fp, unsigned int cmd,
				   unsigned long arg)
{
	struct ev_src __user *arg_ev_sources;
	struct ev_src events;
	struct ev_src events_r;

	char __user *buf;
	char msg[256] = {0};
	u64 cnt[2] = {0};
	int err = -EFAULT;

	switch (cmd) {
	case SYSCACHE_ENABLE_PMU:
		err = l2x0_pmu_enable();
		break;

	case SYSCACHE_DISABLE_PMU:
		err = l2x0_pmu_disable();
		break;

	case SYSCACHE_SET_EV_SOURCES:
		arg_ev_sources = (struct ev_src __user *)arg;
		err = copy_from_user(&events, arg_ev_sources,
				     sizeof(struct ev_src));
		if (err)
			return err;
		pr_debug(" %s: ev0 = %s :: ev1 = %s\n", __func__,
			 ev_list[events.src0], ev_list[events.src1]);
		err = l2x0_pmu_set_event_sources(events);
		l2x0_pmu_get_event_sources(&events_r);
		break;

	case SYSCACHE_GET_EV_COUNT:
		buf = (char __user *)arg;
		cnt[0] = l2x0_counter_read(0);
		cnt[1] = l2x0_counter_read(1);
		snprintf(msg, sizeof(msg), "%s: %lld, %s: %lld\n",
			 ev_list[l2c310.event[0]], cnt[0],
			 ev_list[l2c310.event[1]], cnt[1]);

		return copy_to_user(buf, msg, strlen(msg)) ? -EFAULT : 0;

	case SYSCACHE_FLUSH_ALL:
		err = syscache_flush_all();
		break;

	case SYSCACHE_SYNC:
		err = syscache_sync();
		break;

	case SYSCACHE_INV_ALL:
		err = syscache_inv_all();
		break;

	default:
		pr_err("Incorrect ioctl command: %u\n", cmd);
		err = -EINVAL;
	}

	return err;
}

static const struct file_operations syscache_pmu_ops = {
	.owner = THIS_MODULE,
	.read = syscache_pmu_read,
	.write = syscache_pmu_write,
	.open = syscache_pmu_open,
	.unlocked_ioctl = syscache_pmu_ops_ioctl,
	.release = syscache_pmu_release,
};

int syscache_dev_create(void __iomem **base)
{
	struct device *syscache_device;

	memset(&l2c310, 0, sizeof(struct l2x0_pmu_data));

	l2c310.base = base;

	syscache_dev_major = register_chrdev(0, SYSCACHE_DEV_NAME,
					     &syscache_pmu_ops);
	if (syscache_dev_major < 0) {
		pr_err("%s dev creation failed\n", SYSCACHE_DEV_NAME);
		goto err;
	}

	syscache_class = class_create(THIS_MODULE, SYSCACHE_DEV_NAME);
	if (IS_ERR(syscache_class)) {
		pr_err("device class file already in use\n");
		goto err_class;
	}

	syscache_device = device_create(syscache_class, NULL,
					MKDEV(syscache_dev_major, 0),
					NULL,
					"%s", SYSCACHE_DEV_NAME);
	if (IS_ERR(syscache_device)) {
		pr_err("failed to create device\n");
		goto err_device;
	}

	pr_info("%s device created! -- major = %d\n", SYSCACHE_DEV_NAME,
		syscache_dev_major);
	return 0;

err_device:
	class_destroy(syscache_class);
err_class:
	unregister_chrdev(syscache_dev_major, SYSCACHE_DEV_NAME);
err:
	return -EINVAL;
}

void syscache_dev_remove(void)
{
	device_destroy(syscache_class, MKDEV(syscache_dev_major, 0));
	class_destroy(syscache_class);
	unregister_chrdev(syscache_dev_major, SYSCACHE_DEV_NAME);

	pr_info("%s exited!\n", SYSCACHE_DEV_NAME);
}
