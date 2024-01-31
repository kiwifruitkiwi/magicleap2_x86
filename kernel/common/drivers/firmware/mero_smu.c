/*
 * Copyright (C) 2020-2023 Advanced Micro Devices, Inc. All rights reserved.
 *
 */
#include <linux/module.h>
#include <linux/smu_protocol.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>

#include "ipc.h"
#include "ipc_core.h"
#include "mero_scpi.h"
#include "mp_12_0_0_sh_mask.h"
#include <linux/mailbox/mero_smn_mailbox.h>

/* add smu register definition */
#define CV_RESP_REG		0x564	/* mmMP1_C2PMSG_25 */
#define CV_ARG_REG		0x560	/* mmMP1_C2PMSG_24 */
#define CV_MSG_REG		0x55c	/* mmMP1_C2PMSG_23 */

#define SMU_RESET		0x0
#define SMU_MAX_RETRY		10000
#define SMU_USLEEP_MIN		1000	/* retry minimum interval 1ms */
#define SMU_USLEEP_MAX		1100	/* retry maximum interval 1.1ms */

/* SMU message responses */
enum smu_resp {
	RESP_BUSY		= 0x0,
	RESP_OK			= 0x1,
	RESP_REJECTEDPREREQ	= 0xfd,
	RESP_REJECTEDBUSY	= 0xfc,
	RESP_UNKNOWNCMD		= 0xfe,
	RESP_FAILED		= 0xff,
};

static struct smu_dev {
	void __iomem *base;
	struct mutex smu_lock; /* mutex for thread safety */
	struct platform_device *pdev;
} mero_smu = {0};

static int smu_status(void)
{
	u16 retry = 0;
	u32 resp = RESP_BUSY;

	while (retry++ < SMU_MAX_RETRY) {
		resp = readl_relaxed(mero_smu.base + CV_RESP_REG);
		if (resp != RESP_BUSY)
			break;
	}
	return resp;
};

static bool __smu_send_single_msg_nolock(struct smu_msg *msg)
{
	int ret;

	writel_relaxed(SMU_RESET, mero_smu.base + CV_RESP_REG);
	writel_relaxed(msg->arg, mero_smu.base + CV_ARG_REG);
	smp_wmb(); /* send msg id after resp reset and msg arg */
	writel_relaxed(msg->id, mero_smu.base + CV_MSG_REG);
	smp_mb(); /* check status after registers writing */
	ret = smu_status();
	if (unlikely(ret != RESP_OK)) {
		pr_err("%s fail: id:%d arg:%d ret:0x%x\n",
		       __func__, msg->id, msg->arg, ret);
		msg->rst = false;
		return false;
	}
	msg->resp = readl_relaxed(mero_smu.base + CV_ARG_REG);
	msg->rst = true;
	pr_debug("%s: id = %u, arg = %u, resp = %u\n", __func__,
		 msg->id, msg->arg, msg->resp);
	return true;
}

static int __smu_send_msgs_nolock(struct smu_msg *base, u32 num)
{
	int i, cnt = 0;

	for (i = 0; i < num; i++) {
		if (unlikely(!__smu_send_single_msg_nolock(&base[i]))) {
			pr_err("%s failed: idx:%d\n", __func__, cnt);
			return cnt;
		}
		cnt++;
	}
	return cnt;
}

int smu_send_msgs(struct smu_msg *base, u32 num)
{
	int ret = 0;
	struct device *dev;

	if (!mero_smu.pdev)
		return -ENXIO;

	dev = &mero_smu.pdev->dev;
	if (unlikely(!base || !num)) {
		dev_err(dev, "%s: invalid args\n", __func__);
		return 0;
	}

	if (mutex_lock_interruptible(&mero_smu.smu_lock)) {
		dev_err(dev, "%s: failed to get mutex\n", __func__);
		return 0;
	}
	ret = __smu_send_msgs_nolock(base, num);
	if (unlikely(ret != num))
		dev_err(dev, "%s: failed to send NO.%u msg\n", __func__, ret);

	mutex_unlock(&mero_smu.smu_lock);
	return ret;
}
EXPORT_SYMBOL(smu_send_msgs);

bool smu_send_single_msg(struct smu_msg *msg)
{
	bool ret;
	struct device *dev = &mero_smu.pdev->dev;

	if (unlikely(!msg)) {
		dev_err(dev, "%s: invalid args\n", __func__);
		return false;
	}
	if (unlikely(mutex_lock_interruptible(&mero_smu.smu_lock))) {
		dev_err(dev, "%s: failed to get mutex\n", __func__);
		return false;
	}
	ret = __smu_send_single_msg_nolock(msg);
	mutex_unlock(&mero_smu.smu_lock);
	return ret;
}
EXPORT_SYMBOL(smu_send_single_msg);

void smu_boot_handshake(void)
{
	/* Initiate SMU boot handshake */
	writel_relaxed(SMU_RESET,
		       mero_smu.base + CV_RESP_REG);
	writel_relaxed(CVSMC_MSG_cvready,
		       mero_smu.base + CV_MSG_REG);
	smp_wmb(); /* memory barrier */
}
EXPORT_SYMBOL(smu_boot_handshake);

static int mero_smu_probe(struct platform_device *pdev)
{
	struct smu_msg msg = {0};
	struct resource *res;
	struct device *dev = &pdev->dev;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(dev, "Error in get SMU aperture\n");
		return -EINVAL;
	}

	mero_smu.base = devm_ioremap(dev, res->start, resource_size(res));
	if (!mero_smu.base) {
		dev_err(dev, "ioremap failed\n");
		return -EADDRNOTAVAIL;
	}
	mero_smu.pdev = pdev;

	smu_msg(&msg, CVSMC_MSG_getsmuversion, 0);
	if (!__smu_send_single_msg_nolock(&msg)) {
		dev_err(dev, "get firmware version failed\n");
		mero_smu.pdev = NULL;
		return -EIO;
	}
	dev_info(dev, "firmware version:%d\n", msg.resp);

	smu_boot_handshake();

	mutex_init(&mero_smu.smu_lock);
	return 0;
}

static const struct of_device_id mero_smu_of_match[] = {
	{ .compatible = "amd,mero-smu", },
	{ },
};

static struct platform_driver mero_smu_driver = {
	.probe = mero_smu_probe,
	.driver = {
		.name = "mero_smu",
		.of_match_table = mero_smu_of_match,
	},
};

static int __init mero_smu_init(void)
{
	return platform_driver_register(&mero_smu_driver);
}

static void __exit mero_smu_exit(void)
{
	platform_driver_unregister(&mero_smu_driver);
}

/**
 * subsys_initcall to make sure smu initialized
 * before mero scpi driver register
 */
subsys_initcall(mero_smu_init);
module_exit(mero_smu_exit);

MODULE_DESCRIPTION("mero smu driver");
