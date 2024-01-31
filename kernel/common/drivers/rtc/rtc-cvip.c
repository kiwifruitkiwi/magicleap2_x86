/*
 *  linux/drivers/rtc/rtc-cvip.c
 *
 *  Copyright (C) 2018-2020 Advanced Micro Devices, Inc. All rights reserved.
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/io.h>
#include <linux/rtc.h>
#include <linux/clk.h>
#include <linux/bcd.h>
#include <linux/bitfield.h>
#include <linux/interrupt.h>
#include <linux/pm_wakeirq.h>

#define OFFSET_SEC		0x00
#define OFFSET_MIN		0x02
#define OFFSET_HR		0x04
#define OFFSET_WDAY		0x06
#define OFFSET_MDAY		0x07
#define OFFSET_MON		0x08
#define OFFSET_YEAR		0x09

#define OFFSET_LOW32		0x0
#define OFFSET_HIGH32		0x4

struct cvip_rtc {
	struct rtc_device *rtc_dev;
	void __iomem *regs;
	void __iomem *regs2;
};

u64 cvip_read_secure_cnt (struct device *dev)
{
	u64 result;
	u32 low32, high32;
	struct cvip_rtc *crtc = dev_get_drvdata(dev);

	low32 = readl(crtc->regs2 + OFFSET_LOW32);
	high32 = readl(crtc->regs2 + OFFSET_HIGH32);

	result = high32;
	result <<= 32;
	result |= low32;
	return result;
}
EXPORT_SYMBOL(cvip_read_secure_cnt);

/*read from regs base addr*/
static int cvip_rtc_read_time(struct device *dev, struct rtc_time *tm)
{
	struct cvip_rtc *crtc = dev_get_drvdata(dev);
	u8 sec_reg, min_reg, hr_reg, wday_reg, mday_reg, mon_reg, year_reg;

	sec_reg = readb(crtc->regs + OFFSET_SEC);
	min_reg = readb(crtc->regs + OFFSET_MIN);
	hr_reg = readb(crtc->regs + OFFSET_HR);
	wday_reg = readb(crtc->regs + OFFSET_WDAY);
	mday_reg = readb(crtc->regs + OFFSET_MDAY);
	mon_reg = readb(crtc->regs + OFFSET_MON);
	year_reg = readb(crtc->regs + OFFSET_YEAR);

	tm->tm_sec  = bcd2bin(sec_reg);
	tm->tm_min  = bcd2bin(min_reg);
	tm->tm_hour = bcd2bin(hr_reg);
	tm->tm_wday = bcd2bin(wday_reg) - 1;
	tm->tm_mday = bcd2bin(mday_reg);
	tm->tm_mon = bcd2bin(mon_reg) - 1;
	tm->tm_year = bcd2bin(year_reg) + 100;

	return 0;
}

/*store rtc read time operation*/
static const struct rtc_class_ops cvip_rtc_ops = {
	.read_time	= cvip_rtc_read_time,
};

/*init driver probe function*/
static int cvip_rtc_probe(struct platform_device *pdev)
{
	struct cvip_rtc *crtc;
	struct resource *res;
	int ret;

	crtc = devm_kzalloc(&pdev->dev, sizeof(*crtc), GFP_KERNEL);
	if (!crtc)
		return -ENOMEM;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	crtc->regs = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(crtc->regs))
		return PTR_ERR(crtc->regs);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	crtc->regs2 = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(crtc->regs2))
		return PTR_ERR(crtc->regs2);

	/*allocate rtc device*/
	crtc->rtc_dev = devm_rtc_allocate_device(&pdev->dev);
	if (IS_ERR(crtc->rtc_dev)) {
		ret = PTR_ERR(crtc->rtc_dev);
		dev_err(&pdev->dev,
			"Failed to allocate the RTC device, %d\n", ret);
		return ret;
	}

	platform_set_drvdata(pdev, crtc);

	/*setting driver operations*/
	crtc->rtc_dev->ops = &cvip_rtc_ops;
	device_init_wakeup(&pdev->dev, true);

	/*register rtc device*/
	ret = rtc_register_device(crtc->rtc_dev);
	if (ret) {
		dev_err(&pdev->dev,
			"Failed to register the RTC device, %d\n", ret);
		goto err_disable_wakeup;
	}

	return 0;

err_disable_wakeup:
	device_init_wakeup(&pdev->dev, false);

	return ret;
}

/*device shutdown may need to be modified*/
static int cvip_rtc_remove(struct platform_device *pdev)
{
	device_init_wakeup(&pdev->dev, 0);

	return 0;
}

/*store compatible string for driver*/
static const struct of_device_id cvip_rtc_of_match[] = {
	{ .compatible = "amd,cvip-rtc" },
	{ },
};

MODULE_DEVICE_TABLE(of, cvip_rtc_of_match);

/*platform driver contains of_match_table compatible string*/
static struct platform_driver cvip_rtc_driver = {
	.driver = {
		.name = "cvip-rtc",
		.of_match_table = cvip_rtc_of_match,
	},
	.probe = cvip_rtc_probe,
	.remove = cvip_rtc_remove,
};
module_platform_driver(cvip_rtc_driver);

MODULE_DESCRIPTION("CVIP RTC Driver");

