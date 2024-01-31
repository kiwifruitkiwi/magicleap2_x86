/*
 * Debugfs interface for AMD Mero SoC CVIP system cache pmu.
 *
 * Copyright (C) 2010 ARM Ltd.
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

#include <linux/debugfs.h>
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

#define L2X0_NR				0x4
#define PMU_NR_COUNTERS			0x2
#define REG_EV_CNT_CTRL			0x200
#define REG_EV_CNT1_CFG			0x204
#define REG_EV_CNT0_CFG			0x208
#define REG_EV_CNT1			0x20C
#define REG_EV_CNT0			0x210

#define L2X0_EV_CNT_CTRL_EN		BIT(0)
#define L2X0_EV_SHIFT			0x2

struct range_buffer {
	unsigned long start;
	unsigned long end;
};

#define TYPE				'c'
#define FLUSH_ALL			_IO(TYPE, 1)
#define FLUSH_RANGE			_IOW(TYPE, 2, struct range_buffer)
#define INV_RANGE			_IOW(TYPE, 3, struct range_buffer)
#define CLEAN_RANGE			_IOW(TYPE, 4, struct range_buffer)
#define SYNC				_IO(TYPE, 5)

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
static struct dentry *syscache_debugfs_root;

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

static inline void
__l2x0_counter_write(void __iomem *base, int idx, u32 val)
{
	writel_relaxed(val, base + REG_EV_CNT0 - 4 * idx);
	mb();
}

static inline u32
__l2x0_counter_read(void __iomem *base, int idx)
{
	return readl_relaxed(base + REG_EV_CNT0 - 4 * idx);
}

static inline void
l2x0_counter_write(struct l2x0_pmu_data *l2x0, int idx, u32 val)
{
	int i;

	for (i = 0; i < L2X0_NR; i++)
		__l2x0_counter_write(l2x0->base[i], idx, val);
}

static inline u32
l2x0_counter_read(struct l2x0_pmu_data *l2x0, int idx)
{
	int i;
	u32 count = 0;

	for (i = 0; i < L2X0_NR; i++)
		count += __l2x0_counter_read(l2x0->base[i], idx);

	return count;
}

static inline void
__l2x0_counter_config_write(void __iomem *base, int idx, u32 event)
{
	void __iomem *addr = base + REG_EV_CNT0_CFG - 4 * idx;

	writel_relaxed(event << L2X0_EV_SHIFT, addr);
	mb();
}

static inline void
l2x0_counter_config_write(struct l2x0_pmu_data *l2x0, int idx, u32 event)
{
	int i;

	for (i = 0; i < L2X0_NR; i++)
		__l2x0_counter_config_write(l2x0->base[i], idx, event);
}

static inline void __l2x0_pmu_enable(void __iomem *base, u32 val)
{
	u32 reg = readl_relaxed(base + REG_EV_CNT_CTRL);

	reg &= ~L2X0_EV_CNT_CTRL_EN;
	reg |= val;
	writel_relaxed(reg, base + REG_EV_CNT_CTRL);
	mb();
}

static inline void l2x0_pmu_enable(struct l2x0_pmu_data *pd)
{
	int i;

	for (i = 0; i < L2X0_NR; i++)
		__l2x0_pmu_enable(pd->base[i], 1);
}

static inline void l2x0_pmu_disable(struct l2x0_pmu_data *pd)
{
	int i;

	for (i = 0; i < L2X0_NR; i++)
		__l2x0_pmu_enable(pd->base[i], 0);
}

static inline void warn_if_saturated(u32 count)
{
	if (unlikely(count == 0xffffffff))
		pr_warn_ratelimited("L2X0 counter saturated. Poll period too long\n");
}

static void l2x0_pmu_event_read(struct l2x0_pmu_data *l2x0_pmu, u32 idx)
{
	u64 prev_count, new_count, mask;

	do {
		prev_count = local64_read(&l2x0_pmu->prev_count[idx]);
		new_count = l2x0_counter_read(l2x0_pmu, idx);
	} while (prev_count !=
		 local64_xchg(&l2x0_pmu->prev_count[idx], new_count));

	mask = GENMASK_ULL(31, 0);
	local64_add((new_count - prev_count) & mask, &l2x0_pmu->count[idx]);

	warn_if_saturated(new_count);
}

static void l2x0_pmu_event_configure(struct l2x0_pmu_data *pmu,
				     u32 idx)
{
	local64_set(&pmu->prev_count[idx], 0);
	l2x0_counter_write(pmu, idx, 0);
}

static enum hrtimer_restart l2x0_pmu_poll(struct hrtimer *hrtimer)
{
	unsigned long flags;
	int i;
	struct l2x0_pmu_data *l2x0_pmu;

	l2x0_pmu = container_of(hrtimer, struct l2x0_pmu_data,
				l2x0_pmu_hrtimer);

	local_irq_save(flags);
	l2x0_pmu_disable(l2x0_pmu);

	for (i = 0; i < PMU_NR_COUNTERS; i++) {
		if (l2x0_pmu->event[i] == 0)
			continue;

		l2x0_pmu_event_read(l2x0_pmu, i);
		l2x0_pmu_event_configure(l2x0_pmu, i);
	}

	l2x0_pmu_enable(l2x0_pmu);
	local_irq_restore(flags);

	hrtimer_forward_now(hrtimer, l2x0_pmu->l2x0_pmu_poll_period);
	return HRTIMER_RESTART;
}

static int syscache_enable_open(struct inode *node, struct file *fp)
{
	fp->private_data = &l2c310;
	return 0;
}

static ssize_t
syscache_enable_read(struct file *fp, char __user *buf,
		     size_t size, loff_t *off)
{
	struct l2x0_pmu_data *pd = fp->private_data;
	u8 ctrl_val[L2X0_NR] = {0};
	char msg[32] = {0};
	int i;

	for (i = 0; i < L2X0_NR; i++) {
		ctrl_val[i] = readl_relaxed(pd->base[i] + REG_EV_CNT_CTRL);
		ctrl_val[i] &= L2X0_EV_CNT_CTRL_EN;
	}

	snprintf(msg, sizeof(msg), "%d %d %d %d\n",
		 ctrl_val[0], ctrl_val[1], ctrl_val[2], ctrl_val[3]);

	return simple_read_from_buffer(buf, size, off, msg, strlen(msg));
}

static ssize_t
syscache_enable_write(struct file *fp, const char __user *buf,
		      size_t size, loff_t *off)
{
	unsigned long val;
	char *msg;
	int ret;
	struct l2x0_pmu_data *pd = fp->private_data;

	msg = memdup_user_nul(buf, size);
	if (IS_ERR(msg))
		return PTR_ERR(msg);

	ret = kstrtoul(msg, 10, &val);
	if (ret)
		goto err;

	if (val != 0 && val != 1) {
		ret = -EINVAL;
		goto err;
	}

	if (val)
		l2x0_pmu_enable(pd);
	else
		l2x0_pmu_disable(pd);

	ret = size;
err:
	kfree(msg);
	return ret;
}

static int syscache_enable_close(struct inode *node, struct file *fp)
{
	fp->private_data = NULL;
	return 0;
}

static const struct file_operations syscache_enable_ops = {
	.open = syscache_enable_open,
	.read = syscache_enable_read,
	.write = syscache_enable_write,
	.release = syscache_enable_close,
};

static int syscache_ev_open(struct inode *node, struct file *fp)
{
	fp->private_data = &l2c310;
	return 0;
}

static ssize_t
syscache_ev_read(struct file *fp, char __user *buf,
		 size_t size, loff_t *off)
{
	char msg[64] = {0};
	struct l2x0_pmu_data *pd = fp->private_data;

	snprintf(msg, sizeof(msg), "event0: %s, event1: %s\n",
		 ev_list[pd->event[0]], ev_list[pd->event[1]]);

	return simple_read_from_buffer(buf, size, off, msg, strlen(msg));
}

static ssize_t
syscache_ev_write(struct file *fp, const char __user *buf,
		  size_t size, loff_t *off)
{
	unsigned long val;
	char *msg;
	int ret, idx;
	int op = -1;
	struct l2x0_pmu_data *pd = fp->private_data;

	msg = memdup_user_nul(buf, size);
	if (IS_ERR(msg))
		return PTR_ERR(msg);

	ret = kstrtoul(msg, 10, &val);
	if (ret)
		goto err;
	else
		if (val < 1 || val > 0xf) {
			ret = -ERANGE;
			goto err;
		}

	if (pd->ev_nr == 0) {
		op = 1;
		hrtimer_start(&pd->l2x0_pmu_hrtimer,
			      pd->l2x0_pmu_poll_period,
			      HRTIMER_MODE_REL_PINNED);
	} else if (pd->ev_nr == 1 || pd->ev_nr == 2) {
		if (pd->event[0] == val || pd->event[1] == val)
			op = 0;
		else
			op = 1;
	} else {
		ret = -EINVAL;
		goto err;
	}

	if (op == 0) {
		/* Delete event */
		idx = pd->event[0] == val ? 0 : 1;
		pd->event[idx] = 0;
		l2x0_counter_config_write(pd, idx, 0);
		l2x0_counter_write(pd, idx, 0);
		local64_set(&pd->count[idx], 0);
		pd->ev_nr--;
	} else if (op == 1) {
		/* Add event */
		idx = pd->event[0] == 0 ? 0 : 1;
		pd->event[idx] = val;
		l2x0_counter_config_write(pd, idx, val);
		pd->ev_nr++;
	}

	if (pd->ev_nr == 0)
		hrtimer_cancel(&pd->l2x0_pmu_hrtimer);

	ret = size;
err:
	kfree(msg);
	return ret;
}

static int syscache_ev_close(struct inode *node, struct file *fp)
{
	fp->private_data = NULL;
	return 0;
}

static const struct file_operations syscache_ev_ops = {
	.open = syscache_ev_open,
	.read = syscache_ev_read,
	.write = syscache_ev_write,
	.release = syscache_ev_close,
};

static int syscache_counter_open(struct inode *node, struct file *fp)
{
	fp->private_data = &l2c310;
	return 0;
}

static ssize_t
syscache_counter_read(struct file *fp, char __user *buf,
		      size_t size, loff_t *off)
{
	char msg[64] = {0};
	struct l2x0_pmu_data *pd = fp->private_data;

	snprintf(msg, sizeof(msg), "%s: %ld, %s: %ld\n",
		 ev_list[pd->event[0]], local64_read(&pd->count[0]),
		 ev_list[pd->event[1]], local64_read(&pd->count[1]));

	return simple_read_from_buffer(buf, size, off, msg, strlen(msg));
}

static int syscache_counter_close(struct inode *node, struct file *fp)
{
	fp->private_data = NULL;
	return 0;
}

static const struct file_operations syscache_counter_ops = {
	.open = syscache_counter_open,
	.read = syscache_counter_read,
	.release = syscache_counter_close,
};

static int syscache_ev_list_open(struct inode *node, struct file *fp)
{
	return 0;
}

static ssize_t
syscache_ev_list_read(struct file *fp, char __user *buf,
		      size_t size, loff_t *off)
{
	int i, len;
	char msg[256] = {0};
	char *ptr = msg;

	for (i = 1; i < ARRAY_SIZE(ev_list); i++) {
		len = strlen(ev_list[i]);
		memcpy(ptr, ev_list[i], len);
		ptr += len;
		*ptr = '\n';
		ptr++;
	}

	return simple_read_from_buffer(buf, size, off, msg, strlen(msg));
}

static int syscache_ev_list_close(struct inode *node, struct file *fp)
{
	return 0;
}

static const struct file_operations syscache_ev_list_ops = {
	.open = syscache_ev_list_open,
	.read = syscache_ev_list_read,
	.release = syscache_ev_list_close,
};

static int syscache_runtime_ops_open(struct inode *node, struct file *fp)
{
	return 0;
}

static long syscache_runtime_ops_ioctl(struct file *fp, unsigned int cmd,
				       unsigned long arg)
{
	struct range_buffer __user *argp;
	struct range_buffer range;
	int err = -EFAULT;

	switch (cmd) {
	case FLUSH_ALL:
		err = syscache_flush_all();
		break;
	case FLUSH_RANGE:
		argp = (struct range_buffer __user *)arg;
		err = copy_from_user(&range, argp, sizeof(struct range_buffer));
		if (err)
			return err;
		err = syscache_flush_range(range.start, range.end);
		break;
	case INV_RANGE:
		argp = (struct range_buffer __user *)arg;
		err = copy_from_user(&range, argp, sizeof(struct range_buffer));
		if (err)
			return err;
		err = syscache_inv_range(range.start, range.end);
		break;
	case CLEAN_RANGE:
		argp = (struct range_buffer __user *)arg;
		err = copy_from_user(&range, argp, sizeof(struct range_buffer));
		if (err)
			return err;
		err = syscache_clean_range(range.start, range.end);
		break;
	case SYNC:
		err = syscache_sync();
		break;
	default:
		err = -ERANGE;
	}

	return err;
}

static int syscache_runtime_ops_close(struct inode *node, struct file *fp)
{
	return 0;
}

static const struct file_operations syscache_runtime_ops = {
	.open = syscache_runtime_ops_open,
	.unlocked_ioctl = syscache_runtime_ops_ioctl,
	.release = syscache_runtime_ops_close,
};

static int syscache_mode_open(struct inode *node, struct file *fp)
{
	return 0;
}

static ssize_t
syscache_mode_write(struct file *fp, const char __user *buf,
		    size_t size, loff_t *off)
{
	char *msg;
	int ret;

	msg = memdup_user_nul(buf, size);
	if (IS_ERR(msg))
		return PTR_ERR(msg);

	if (!strncmp(msg, "wt", strlen("wt")))
		ret = setup_syscache_axcache(ARCACHE(WRTHR_RDWRALC_CACHEABLE) |
			       AWCACHE(WRTHR_RDWRALC_CACHEABLE));
	else if (!strncmp(msg, "wb", strlen("wb")))
		ret = setup_syscache_axcache(ARCACHE(WRBK_RDWRALC_CACHEABLE) |
			       AWCACHE(WRBK_RDWRALC_CACHEABLE));
	else
		ret = -EINVAL;

	if (ret)
		goto err;

	ret = size;
err:
	kfree(msg);
	return ret;
}

static int syscache_mode_close(struct inode *node, struct file *fp)
{
	return 0;
}

static const struct file_operations syscache_mode_ops = {
	.open = syscache_mode_open,
	.write = syscache_mode_write,
	.release = syscache_mode_close,
};

static int syscache_flushall_open(struct inode *node, struct file *fp)
{
	fp->private_data = &l2c310;
	return 0;
}

static ssize_t
syscache_flushall_write(struct file *fp, const char __user *buf,
			size_t size, loff_t *off)
{
	unsigned long val;
	char *msg;
	int ret;
	int err = -EFAULT;

	msg = memdup_user_nul(buf, size);
	if (IS_ERR(msg))
		return PTR_ERR(msg);

	ret = kstrtoul(msg, 10, &val);
	if (ret)
		goto err;

	if (val != 1) {
		ret = -EINVAL;
		goto err;
	}

	err = syscache_flush_all();
	if (err) {
		ret = err;
		goto err;
	}

	ret = size;
err:
	kfree(msg);
	return ret;
}

static int syscache_flushall_close(struct inode *node, struct file *fp)
{
	fp->private_data = NULL;
	return 0;
}

static const struct file_operations syscache_flushall_ops = {
	.open = syscache_flushall_open,
	.write = syscache_flushall_write,
	.release = syscache_flushall_close,
};

static int syscache_sync_open(struct inode *node, struct file *fp)
{
	fp->private_data = &l2c310;
	return 0;
}

static ssize_t
syscache_sync_write(struct file *fp, const char __user *buf,
		    size_t size, loff_t *off)
{
	unsigned long val;
	char *msg;
	int ret;
	int err = -EFAULT;

	msg = memdup_user_nul(buf, size);
	if (IS_ERR(msg))
		return PTR_ERR(msg);

	ret = kstrtoul(msg, 10, &val);
	if (ret)
		goto err;

	if (val != 1) {
		ret = -EINVAL;
		goto err;
	}

	err = syscache_sync();
	if (err) {
		ret = err;
		goto err;
	}

	ret = size;
err:
	kfree(msg);
	return ret;
}

static int syscache_sync_close(struct inode *node, struct file *fp)
{
	fp->private_data = NULL;
	return 0;
}

static const struct file_operations syscache_sync_ops = {
	.open = syscache_sync_open,
	.write = syscache_sync_write,
	.release = syscache_sync_close,
};

int syscache_debugfs_create(void __iomem **base)
{
	memset(&l2c310, 0, sizeof(struct l2x0_pmu_data));

	l2c310.base = base;
	l2c310.l2x0_pmu_poll_period = ms_to_ktime(1000);
	hrtimer_init(&l2c310.l2x0_pmu_hrtimer,
		     CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	l2c310.l2x0_pmu_hrtimer.function = l2x0_pmu_poll;

	syscache_debugfs_root = debugfs_create_dir("syscache", NULL);
	if (!syscache_debugfs_root) {
		pr_err("SysCache: can not create debugfs dir\n");
		return -ENOMEM;
	}

	debugfs_create_file("enable", 0660, syscache_debugfs_root,
			    &l2c310, &syscache_enable_ops);
	debugfs_create_file("event", 0660, syscache_debugfs_root,
			    &l2c310, &syscache_ev_ops);
	debugfs_create_file("counter", 0440, syscache_debugfs_root,
			    &l2c310, &syscache_counter_ops);
	debugfs_create_file("event_list", 0440, syscache_debugfs_root,
			    &l2c310, &syscache_ev_list_ops);
	debugfs_create_file("runtime_ops", 0660, syscache_debugfs_root,
			    &l2c310, &syscache_runtime_ops);
	debugfs_create_file("mode", 0660, syscache_debugfs_root,
			    &l2c310, &syscache_mode_ops);
	debugfs_create_file("flushall", 0660, syscache_debugfs_root,
			    &l2c310, &syscache_flushall_ops);
	debugfs_create_file("sync", 0660, syscache_debugfs_root,
			    &l2c310, &syscache_sync_ops);
	return 0;
}

void syscache_debugfs_remove(void)
{
	if (syscache_debugfs_root && !IS_ERR(syscache_debugfs_root))
		debugfs_remove_recursive(syscache_debugfs_root);

	memset(&l2c310, 0, sizeof(struct l2x0_pmu_data));
}

