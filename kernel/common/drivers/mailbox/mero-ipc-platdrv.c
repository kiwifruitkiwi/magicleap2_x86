/*
 * Mailbox platform driver for AMD Mero SoC CVIP Subsystem.
 *
 * Copyright (C) 2019-2021 Advanced Micro Devices, Inc. All rights reserved.
 */

#include <linux/types.h>
#include <linux/err.h>
#include <linux/delay.h>
#include <linux/export.h>
#include <linux/io.h>
#include <linux/interrupt.h>
#include <linux/mutex.h>
#include <linux/device.h>
#include <linux/mailbox_controller.h>
#include <linux/mailbox_client.h>
#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/of_irq.h>

#include "mero_ipc_i.h"

#ifdef CONFIG_PM_SLEEP
static int mero_ipc_resume(struct device *dev)
{
	struct platform_device *pdev = container_of(dev, struct platform_device, dev);
	struct mero_ipc_dev *ipc_dev = platform_get_drvdata(pdev);

	dev_dbg(dev, "Resuming ipc device\n");
	return mero_ipc_restore(ipc_dev);
}

static const struct dev_pm_ops mero_ipc_pm_ops = {
	.resume_noirq = mero_ipc_resume,
};

#define MERO_IPC_PM_OPS	(&mero_ipc_pm_ops)
#else
#define MERO_IPC_PM_OPS	NULL
#endif

static int mero_ipc_probe(struct platform_device *pdev)
{
	int ret = -ENODEV, num;
	struct resource *res;
	struct device *dev = &pdev->dev;
	struct device_node *node = pdev->dev.of_node;
	struct mero_ipc_dev *ipc_dev;
	unsigned int i;

	ipc_dev = devm_kzalloc(dev, sizeof(*ipc_dev), GFP_KERNEL);
	if (!ipc_dev)
		return -ENOMEM;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (res == NULL) {
		dev_err(dev, "no memory resource defined\n");
		ret = -ENODEV;
		goto kfree_err;
	}
	ipc_dev->dev = dev;
	ipc_dev->base = ioremap(res->start, resource_size(res));
	if (ipc_dev->base == NULL) {
		dev_err(dev, "ioremap failed.\n");
		ret = -ENOMEM;
		goto kfree_err;
	}

	num = of_property_read_variable_u32_array(node, "parameters", ipc_dev->param,
						  PARAM_CELLS,
						  IPC_CHN_NUM * PARAM_CELLS);
	if ((num <= 0) || (num % PARAM_CELLS != 0)) {
		dev_err(dev, "of: failed to read parameters property\n");
		goto unmap_err;
	}

	ipc_dev->nr_mbox = num / PARAM_CELLS;
	for (i = 0; i < IPC_CHN_NUM; i++)
		ipc_dev->vecs[i] = platform_get_irq(pdev, i);

	ret = mero_mailbox_init(ipc_dev);
	if (ret) {
		dev_err(dev, "Failed to initialize mailboxes\n");
		goto kfree_err;
	}

	platform_set_drvdata(pdev, ipc_dev);
	dev_info(dev, "Successfully register mailbox controller\n");

	return 0;

unmap_err:
	iounmap(ipc_dev->base);
kfree_err:
	return ret;
}

static int mero_ipc_remove(struct platform_device *pdev)
{
	struct mero_ipc_dev *ipc_dev = platform_get_drvdata(pdev);
	struct mero_ipc *ipc = ipc_dev->ipc;

	iounmap(ipc->base);

	return 0;
}

static const struct of_device_id mero_ipc_of_match[] = {
	{ .compatible = "amd,mero-ipc", },
	{},
};
MODULE_DEVICE_TABLE(of, mero_ipc_of_match);

static struct platform_driver mero_ipc_driver = {
	.probe		= mero_ipc_probe,
	.remove		= mero_ipc_remove,
	.driver = {
		.name = "meroipc",
		.of_match_table = mero_ipc_of_match,
		.pm = MERO_IPC_PM_OPS,
	},
};

static int __init mero_ipc_init(void)
{
	if (ipc_dev_pre_init())
		pr_warn("ipc_dev_pre_init err\n");

	return platform_driver_register(&mero_ipc_driver);
}

static void __exit mero_ipc_exit(void)
{
	ipc_dev_cleanup();
	platform_driver_unregister(&mero_ipc_driver);
}

core_initcall(mero_ipc_init);
module_exit(mero_ipc_exit);
