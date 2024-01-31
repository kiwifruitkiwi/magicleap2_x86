/*
 * Copyright (C) 2019-2021 Advanced Micro Devices, Inc. All rights reserved.
 */

#include <linux/io.h>
#include <linux/of.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/types.h>
#include <linux/timer.h>
#include <linux/reboot.h>
#include <linux/module.h>
#include <linux/cpufreq.h>
#include <linux/uaccess.h>
#include <linux/watchdog.h>
#include <linux/interrupt.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/clk-provider.h>

#define WDT_INFO(fmt, ...) \
	pr_info("mero_wdt: %s: " fmt, __func__, ##__VA_ARGS__)
#define WDT_ERROR(fmt, ...) \
	pr_err("mero_wdt: %s: " fmt, __func__, ##__VA_ARGS__)
#define WDT_DEBUG(fmt, ...) \
	pr_debug("mero_wdt: %s: " fmt, __func__, ##__VA_ARGS__)

#define WDT_FIRMWARE_VERSION	0

/* WDT struct for mero objects */
struct mero_watchdog_device {
	struct watchdog_device wdd; /* watchdog device base */
	u32 timeout; /* timeout for counter*/
	u32 withboot; /* whether boot with kernel */
	u32 nowayout; /* whether cannot be disabled */
	u32 wdt_mode; /* mode: 0 = rst, 1 = int */
	u32 wdt_freq; /* frequency for counter */
	u32 wdt_count; /* current WDT count */
	u32 wdt_debug; /* debug: 0 = off, 1 = on */
	spinlock_t lock; /* spinlock */
	void __iomem *wdt_base; /* memory base */
	struct device *wdt_dev; /* wdt device */
	struct resource *wdt_mem; /* mem resource */
	struct resource *wdt_irq; /* irq resource */
	struct clk *wdt_clock; /* clock resource */
	struct notifier_block clk_nb; /* clock change notifier */
};

/* 0 - rst mode, 1 - int mode */
#define WATCHDOG_MODE			(0)
#define WATCHDOG_DEBUG			(0)
#define WATCHDOG_WITHBOOT		(0)
#define WATCHDOG_TIMEOUT		(20)
#define WATCHDOG_FREQ			(204800)

#define WDT_WTDAT		0x20
#define WDT_WTCNT		0x24
#define WDT_WTCON		0x28
#define WDT_WTINT		0x2c
#define WDT_WTRST		0x30
#define WDT_WTCLRINT	0x34
#define WDT_WTINT_ON	0x01

#define WDT_CNT_MAXCNT	0xffffffff

#define WDT_CON_ENABLE	(1 << 0)
#define WDT_CON_INTEN	(1 << 2)
#define WDT_CON_RSTEN	(1 << 3)

#define WDT_CON_SCALE_MASK		(0xff << 8)
#define WDT_CON_SCALE_MAX		(0xff)

#define WDT_CON_SCALE(x)		((x) << 8)

#define CLK_1MHZ	1000000

static inline void
wdt_dump(struct mero_watchdog_device *mw)
{
	u32 wtdat, wtcnt, wtcon, wtint, wtrst;
	unsigned long flags = 0;

	if (!mw->wdt_debug)
		return;

	spin_lock_irqsave(&mw->lock, flags);

	wtdat = ioread32(mw->wdt_base + WDT_WTDAT);
	wtcnt = ioread32(mw->wdt_base + WDT_WTCNT);
	wtcon = ioread32(mw->wdt_base + WDT_WTCON);
	wtint = ioread32(mw->wdt_base + WDT_WTINT);
	wtrst = ioread32(mw->wdt_base + WDT_WTRST);

	spin_unlock_irqrestore(&mw->lock, flags);

	WDT_INFO("load: 0x%08x\ncount: 0x%08x\ncontrol: 0x%08x\n"
			"interrupt status: 0x%08x\nreset status: %08x",
			wtdat, wtcnt, wtcon, wtint, wtrst);
}

static void __wdt_clear_intr_sts_nolock(struct mero_watchdog_device *mw)
{
	iowrite32(WDT_WTINT_ON, mw->wdt_base + WDT_WTINT);
}

static void
__panic_if_needed_nolock(struct mero_watchdog_device *mw)
{
	u32 wtint;

	wtint = ioread32(mw->wdt_base + WDT_WTINT);
	__wdt_clear_intr_sts_nolock(mw);
	if (wtint & WDT_WTINT_ON)
		panic("panic triggered by mero watchdog timer.\n");
}

static void
__wdt_stop_nolock(struct mero_watchdog_device *mw)
{
	u32 wtcon;

	wtcon = ioread32(mw->wdt_base + WDT_WTCON);
	wtcon &= ~(WDT_CON_ENABLE | WDT_CON_RSTEN | WDT_CON_INTEN);
	iowrite32(wtcon, mw->wdt_base + WDT_WTCON);
	__wdt_clear_intr_sts_nolock(mw);
}

static int __must_check
wdt_start(struct watchdog_device *w)
{
	u32 wtcon;
	struct mero_watchdog_device *mw;
	unsigned long flags = 0;

	if (!w) {
		WDT_ERROR("watchdog_device is null\n");
		return -EINVAL;
	}

	mw = container_of(w, struct mero_watchdog_device, wdd);

	if (!mw) {
		WDT_ERROR("mero_watchdog_device is null\n");
		return -EINVAL;
	}

	spin_lock_irqsave(&mw->lock, flags);

	__wdt_stop_nolock(mw);

	wtcon = ioread32(mw->wdt_base + WDT_WTCON);
	wtcon |= WDT_CON_ENABLE;

	if (mw->wdt_mode) {
		wtcon |= WDT_CON_INTEN;
		wtcon &= ~WDT_CON_RSTEN;
	} else {
		wtcon &= ~WDT_CON_INTEN;
		wtcon |= WDT_CON_RSTEN;
	}

	iowrite32(mw->wdt_count, mw->wdt_base + WDT_WTDAT);
	iowrite32(wtcon, mw->wdt_base + WDT_WTCON);

	iowrite32(mw->wdt_count, mw->wdt_base + WDT_WTCNT);

	spin_unlock_irqrestore(&mw->lock, flags);

	wdt_dump(mw);

	return 0;
}

static int
wdt_stop(struct watchdog_device *w)
{
	struct mero_watchdog_device *mw;
	unsigned long flags = 0;

	if (!w) {
		WDT_ERROR("watchdog_device is null\n");
		return -EINVAL;
	}

	mw = container_of(w, struct mero_watchdog_device, wdd);

	if (!mw) {
		WDT_ERROR("mero_watchdog_device is null\n");
		return -EINVAL;
	}

	spin_lock_irqsave(&mw->lock, flags);

	__wdt_stop_nolock(mw);

	spin_unlock_irqrestore(&mw->lock, flags);

	return 0;
}

static int
wdt_keepalive(struct watchdog_device *w)
{
	struct mero_watchdog_device *mw;
	unsigned long flags = 0;

	if (!w) {
		WDT_ERROR("watchdog_device is null\n");
		return -EINVAL;
	}

	mw = container_of(w, struct mero_watchdog_device, wdd);

	if (!mw) {
		WDT_ERROR("mero_watchdog_device is null\n");
		return -EINVAL;
	}

	wdt_dump(mw);

	spin_lock_irqsave(&mw->lock, flags);

	iowrite32(mw->wdt_count, mw->wdt_base + WDT_WTDAT);
	iowrite32(mw->wdt_count, mw->wdt_base + WDT_WTCNT);

	spin_unlock_irqrestore(&mw->lock, flags);
	return 0;
}

static int
wdt_set_timeout(struct watchdog_device *w, u32 timeout)
{
	struct mero_watchdog_device *mw;
	u32 freq = 0;
	u64 count;
	u32 wtcon = 0;
	u64 divisor = 1;

	if (!w) {
		WDT_ERROR("watchdog_device is null\n");
		return -EINVAL;
	}

	mw = container_of(w, struct mero_watchdog_device, wdd);

	if (!mw) {
		WDT_ERROR("mero_watchdog_device is null\n");
		return -EINVAL;
	}

	/* Timeout used by watchdog daemon to ping the hardware */
	mw->wdd.timeout = timeout;

	if (timeout < 1) {
		WDT_ERROR("timeout '%u' is too small.\n", timeout);
		return -EINVAL;
	}

	/* Get the NIC400 clk rate, if it's zero, use the default value */
	freq = (clk_get_rate(mw->wdt_clock) / 4) * CLK_1MHZ;
	if (!freq)
		freq = mw->wdt_freq;

	count = (u64)timeout * freq;

	/* check for count overflow */
	if (freq != 0 && (count / freq != timeout)) {
		WDT_ERROR("count overflow, timeout: %u , freq: %u\n",
			  timeout, freq);
		return -EINVAL;
	}

	/*
	 * WDT Count is one 32bits register, it means that MAX value
	 * is 0xFFFFFFFF. If the count value after calculated with
	 * timeout and frequency is bigger than this max value, the
	 * overflow of u32 type for WDT count register will happen.
	 * So the scaler for dividing in WDT control is needed for
	 * such circumstance to make sure that the count value is
	 * always smaller than or equal to max value of reigster.
	 */
	if (count > WDT_CNT_MAXCNT) {
		for (; divisor <= WDT_CON_SCALE_MAX; divisor++) {
			/* if the divisor is acceptable, just break */
			if (count <= (WDT_CNT_MAXCNT * divisor))
				break;
		}
		/* error if count value is still too big for register*/
		if ((count / divisor) > WDT_CNT_MAXCNT) {
			WDT_ERROR("timeout %u is too big\n", timeout);
			return -EINVAL;
		}
	}

	/* set new count by divisor */
	count /= divisor;
	mw->wdt_count = count;

	/* configure WTCON with divisor */
	wtcon = ioread32(mw->wdt_base + WDT_WTCON);
	wtcon &= ~WDT_CON_SCALE_MASK;
	wtcon |= WDT_CON_SCALE(divisor - 1);

	/* write register */
	iowrite32(count, mw->wdt_base + WDT_WTDAT);
	iowrite32(wtcon, mw->wdt_base + WDT_WTCON);

	/* write timeout */
	mw->timeout = (count * divisor) / freq;

	WDT_DEBUG("count 0x%016llx, wtcon 0x%08x, timeout %d\n",
			count, wtcon, mw->timeout);

	return 0;
}

#define WDT_WD_OPTIONS \
	(WDIOF_SETTIMEOUT | WDIOF_KEEPALIVEPING | WDIOF_MAGICCLOSE)

static const struct watchdog_info wdt_ident = {
	.options = WDT_WD_OPTIONS,
	.firmware_version = WDT_FIRMWARE_VERSION,
	.identity = "MERO Watchdog"
};

static struct watchdog_ops wdt_ops = {
	.owner	= THIS_MODULE,
	.start	= wdt_start,
	.stop	= wdt_stop,
	.ping	= wdt_keepalive,
	.set_timeout = wdt_set_timeout,
};

static int wdt_clk_event(struct notifier_block *nb,
			 unsigned long event, void *data)
{
	struct mero_watchdog_device *mw;

	mw = container_of(nb, struct mero_watchdog_device, clk_nb);

	if (mw && (watchdog_active(&mw->wdd) || watchdog_hw_running(&mw->wdd))) {
		if (event == PRE_RATE_CHANGE) {
			WDT_DEBUG("Stop WDT on PRE rate change\n");
			wdt_stop(&mw->wdd);
		} else if (event == POST_RATE_CHANGE) {
			WDT_DEBUG("Set new timeout on post rate change\n");
			if (wdt_set_timeout(&mw->wdd, mw->timeout))
				WDT_ERROR("%s: fail setting timeout\n", __func__);
			if (wdt_start(&mw->wdd))
				WDT_ERROR("%s: fail start watchdog\n", __func__);
		}
	}

	return NOTIFY_OK;
}

static irqreturn_t
wdt_irq_handler(int inqno, void *dev)
{
	struct mero_watchdog_device *mw;
	struct platform_device *pdev = (struct platform_device *)dev;

	if (!dev) {
		WDT_ERROR("platform_device is null\n");
		return -EINVAL;
	}

	mw = platform_get_drvdata(pdev);

	if (!mw) {
		WDT_ERROR("mero_watchdog_device is null\n");
		return -EINVAL;
	}

	__panic_if_needed_nolock(mw);
	return IRQ_HANDLED;
}

static int
wdt_probe(struct platform_device *pdev)
{
	int ret = -1;
	int size = 0;
	int started = 0;
	struct mero_watchdog_device *mw;
	struct device *dev = &pdev->dev;
	struct device_node *np = dev->of_node;

	/* alloc device object */
	mw = kzalloc(sizeof(struct mero_watchdog_device), GFP_KERNEL);
	if (!mw) {
		WDT_ERROR("failed to alloc for mero watchdog device.\n");
		return -ENOMEM;
	}

	spin_lock_init(&mw->lock);

	mw->wdd.info = &wdt_ident;
	mw->wdd.ops = &wdt_ops;
	mw->wdt_dev = dev;

	/* get mem resource */
	mw->wdt_mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!mw->wdt_mem) {
		WDT_ERROR("platform get memory failed !\n");
		goto ERROR_RESOURCE;
	}

	/* get irq resource */
	mw->wdt_irq = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	if (!mw->wdt_irq) {
		WDT_ERROR("platform get irq failed !\n");
		ret = -EINVAL;
		goto ERROR_RESOURCE;
	}

	/* get mem size and request for registers */
	size = resource_size(mw->wdt_mem);
	if (!request_mem_region(mw->wdt_mem->start, size, pdev->name)) {
		WDT_ERROR("request mem failed !\n");
		ret = -EBUSY;
		goto ERROR_RESOURCE;
	}

	mw->wdt_base = ioremap_nocache(mw->wdt_mem->start, size);
	if (!mw->wdt_base) {
		WDT_ERROR("ioremap failed !\n");
		ret = -EINVAL;
		goto ERROR_IOREMAP;
	}

	/* mode */
	if (device_property_read_u32(mw->wdt_dev, "mode",
		&mw->wdt_mode)) {
		WDT_DEBUG("timeout get failed, using default %d!\n",
			WATCHDOG_MODE);
		mw->wdt_mode = WATCHDOG_MODE;
	}

	/* frequency */
	if (device_property_read_u32(mw->wdt_dev, "frequency",
		&mw->wdt_freq)) {
		WDT_DEBUG("frequency get failed, using default %d!\n",
			WATCHDOG_FREQ);
		mw->wdt_freq = WATCHDOG_FREQ;
	}

	/* timeout */
	if (device_property_read_u32(mw->wdt_dev, "timeout",
		&mw->timeout)) {
		WDT_DEBUG("timeout get failed, using default %d!\n",
			WATCHDOG_TIMEOUT);
		mw->timeout = WATCHDOG_TIMEOUT;
	}

	/* withboot */
	if (device_property_read_u32(mw->wdt_dev, "withboot",
		&mw->withboot)) {
		WDT_DEBUG("withboot get failed, using default %d!\n",
			WATCHDOG_WITHBOOT);
		mw->withboot = WATCHDOG_WITHBOOT;
	}

	/* nowayout */
	if (device_property_read_u32(mw->wdt_dev, "nowayout",
		&mw->nowayout)) {
		WDT_DEBUG("nowayout get failed, using default %d!\n",
			WATCHDOG_NOWAYOUT);
		mw->nowayout = WATCHDOG_NOWAYOUT;
	}

	/* debug */
	if (device_property_read_u32(mw->wdt_dev, "debug",
		&mw->wdt_debug)) {
		WDT_DEBUG("debug get failed, using default %d!\n",
			WATCHDOG_DEBUG);
		mw->wdt_debug = WATCHDOG_DEBUG;
	}

	/* clock to get the freq value */
	mw->wdt_clock = of_clk_get(np, 0);
	if (IS_ERR(mw->wdt_clock)) {
		dev_err(dev, "Error in retrieve clk, defering probe\n");
		ret = -EPROBE_DEFER;
		goto ERROR_IRQ;
	}

	/* setup clock change notifier */
	mw->clk_nb.notifier_call = wdt_clk_event;
	mw->clk_nb.priority = INT_MAX;
	ret = clk_notifier_register(mw->wdt_clock, &mw->clk_nb);
	if (ret) {
		dev_err(dev, "failed to register the wdt_clock notifier\n");
		goto ERROR_IRQ;
	}

	/* set timeout, if failed, use default one. */
	if (wdt_set_timeout(&mw->wdd, mw->timeout)) {
		started = wdt_set_timeout(&mw->wdd, WATCHDOG_TIMEOUT);
		if (!started)
			WDT_ERROR("wdt used default time %d!\n",
				WATCHDOG_TIMEOUT);
		else
			WDT_ERROR("default time is fault, cannot start\n");
	}

	/* require irq with handler function */
	ret = request_irq(mw->wdt_irq->start,
		wdt_irq_handler, 0, pdev->name, pdev);
	if (ret != 0) {
		WDT_ERROR("request irq failed !\n");
		ret = -EINVAL;
		goto ERROR_IRQ;
	}

	/* driver data */
	watchdog_set_drvdata(&mw->wdd, mw);
	/* set nowayout */
	watchdog_set_nowayout(&mw->wdd, mw->nowayout);

	/* need boot with kernel */
	if (mw->withboot && !started) {
		WDT_INFO("starting watchdog timer...\n");
		if (wdt_start(&mw->wdd) != 0) {
			WDT_ERROR("failed to start watchdog timer.\n");
			ret = -EINVAL;
			goto ERROR_START;
		}
		set_bit(WDOG_HW_RUNNING, &mw->wdd.status);
	} else if (!mw->withboot) {
		wdt_stop(&mw->wdd);
	}

	/* register */
	ret = watchdog_register_device(&mw->wdd);
	if (ret) {
		WDT_ERROR("watchdog register failed !\n");
		goto ERROR_REGISTER;
	}

	platform_set_drvdata(pdev, mw);

	return 0;

ERROR_START:
	watchdog_unregister_device(&mw->wdd);

ERROR_REGISTER:
	free_irq(mw->wdt_irq->start, pdev);

ERROR_IRQ:
	iounmap(mw->wdt_base);

ERROR_IOREMAP:
	release_mem_region(mw->wdt_mem->start, size);

ERROR_RESOURCE:
	mw->wdt_irq = NULL;
	mw->wdt_mem = NULL;
	kfree(mw);
	mw = NULL;
	return ret;
}

static int
wdt_remove(struct platform_device *pdev)
{
	struct mero_watchdog_device *mw = platform_get_drvdata(pdev);

	watchdog_unregister_device(&mw->wdd);
	free_irq(mw->wdt_irq->start, pdev);
	iounmap(mw->wdt_base);
	release_mem_region(mw->wdt_mem->start,
		resource_size(mw->wdt_mem));
	mw->wdt_irq = NULL;
	mw->wdt_mem = NULL;
	kfree(mw);
	mw = NULL;
	return 0;
}

static void
wdt_shutdown(struct platform_device *pdev)
{
	struct mero_watchdog_device *mw = platform_get_drvdata(pdev);

	wdt_stop(&mw->wdd);
}

#ifdef CONFIG_PM_SLEEP
static int wdt_runtime_suspend(struct device *dev)
{
	struct mero_watchdog_device *mw = dev_get_drvdata(dev);

	if (watchdog_active(&mw->wdd) || watchdog_hw_running(&mw->wdd))
		if (wdt_stop(&mw->wdd) != 0) {
			WDT_ERROR("failed to stop watchdog timer.\n");
			return -EINVAL;
		}
	return 0;
}

static int wdt_runtime_resume(struct device *dev)
{
	struct mero_watchdog_device *mw = dev_get_drvdata(dev);

	if (watchdog_active(&mw->wdd) || watchdog_hw_running(&mw->wdd))
		if (wdt_start(&mw->wdd) != 0) {
			WDT_ERROR("failed to start watchdog timer.\n");
			return -EINVAL;
		}
	return 0;
}
#endif

static const struct of_device_id wdt_match[] = {
	{.compatible = "amd,mero-wdt"},
	{},
};

static const struct dev_pm_ops wdt_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(wdt_runtime_suspend, wdt_runtime_resume)
};

struct platform_driver mero_wdt_drv = {
	.driver = {
		.owner	= THIS_MODULE,
		.name	= "mero-wdt",
		.pm	= &wdt_pm_ops,
		.of_match_table = of_match_ptr(wdt_match),
	},
	.probe		= wdt_probe,
	.remove		= wdt_remove,
	.shutdown	= wdt_shutdown,
};

static void __exit
wdt_exit(void)
{
	platform_driver_unregister(&mero_wdt_drv);
}

static int __init
wdt_init(void)
{
	return platform_driver_register(&mero_wdt_drv);
}

module_init(wdt_init);
module_exit(wdt_exit);

MODULE_DESCRIPTION("MERO Watchdog Timer Driver");
