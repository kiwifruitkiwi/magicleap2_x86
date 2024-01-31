/*
 * Copyright (C) 2019-2020 Advanced Micro Devices, Inc. All rights reserved.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/mero/cvip/cvip_dsp.h>
#include <linux/clk.h>
#include <dsp_manager_internal.h>

struct dsp_dev {
	struct list_head list;
	int tbu_irq_0;
	int tbu_irq_1;
	struct clk *clk;
	struct platform_device *pdev;
	int bypass_pd;
};

struct dsp_list {
	struct mutex mutex; /*protect dsp list*/
	struct list_head list;
};

static struct dsp_list dsps;

static int cvip_dsp_match_child(struct device *dev, void *name)
{
	if (!dev || !name)
		return 0;

	if (!strcmp(dev_name(dev), (char *)name))
		return 1;

	return 0;
}

struct device *cvip_dsp_get_channel_dev_byname(char *name)
{
	struct dsp_dev *dsp;
	struct device *dev;

	mutex_lock(&dsps.mutex);
	list_for_each_entry(dsp, &dsps.list, list) {
		dev = device_find_child(&dsp->pdev->dev, name,
					cvip_dsp_match_child);
		if (dev) {
			put_device(dev);
			break;
		}
	}
	mutex_unlock(&dsps.mutex);
	return dev;
}
EXPORT_SYMBOL_GPL(cvip_dsp_get_channel_dev_byname);

struct clk *cvip_dsp_get_clk_byname(char *name)
{
	struct dsp_dev *dsp;
	struct clk *clk = NULL;

	if (!name)
		return NULL;

	mutex_lock(&dsps.mutex);
	list_for_each_entry(dsp, &dsps.list, list) {
		if (!strcmp(dev_name(&dsp->pdev->dev), name)) {
			clk = dsp->clk;
			break;
		}
	}
	mutex_unlock(&dsps.mutex);
	return clk;
}
EXPORT_SYMBOL_GPL(cvip_dsp_get_clk_byname);

int cvip_dsp_get_pdbypass_byname(char *name)
{
	struct dsp_dev *dsp;
	int pd_bypass = 0;

	if (!name)
		return 0;

	mutex_lock(&dsps.mutex);
	list_for_each_entry(dsp, &dsps.list, list) {
		if (!strcmp(dev_name(&dsp->pdev->dev), name)) {
			pd_bypass = dsp->bypass_pd;
			break;
		}
	}
	mutex_unlock(&dsps.mutex);
	return pd_bypass;
}
EXPORT_SYMBOL_GPL(cvip_dsp_get_pdbypass_byname);

static int cvip_dsp_unregister_child(struct device *dev, void *data)
{
	struct platform_device *child = to_platform_device(dev);

	of_device_unregister(child);
	return 0;
}

static int cvip_dsp_probe(struct platform_device *pdev)
{
	int ret;
	struct device *dev = &pdev->dev;
	struct device_node *np = dev->of_node;
	char child_name[128];
	struct platform_device *child;
	struct dsp_dev *dsp;

	dsp = devm_kzalloc(dev, sizeof(*dsp), GFP_KERNEL);
	if (!dsp) {
		ret = -ENOMEM;
		goto err_0;
	}

	dsp->pdev = pdev;

	dsp->clk = of_clk_get(np, 0);
	if (IS_ERR(dsp->clk)) {
		dev_err(dev, "Error in retrieve clk\n");
		ret = -EPROBE_DEFER;
		goto err_3;
	}
	/* put DSP clk to 0 */
	ret = clk_set_rate(dsp->clk, 0);
	if (ret) {
		dev_err(dev, "Failed to set DSP[%s] clk to 0\n", np->name);
		goto err_3;
	}

	if (of_property_read_bool(np, "bypass_pd_control"))
		dsp->bypass_pd = 1;

	/* Create platform devs for dts child nodes */
	for_each_available_child_of_node(dev->of_node, np) {
		snprintf(child_name, sizeof(child_name), "%s-%s",
			 dev_name(dev), np->name);
		child = of_platform_device_create(np, child_name, dev);
		if (!child) {
			dev_err(dev, "failed to create child %s dev\n",
				child_name);
			ret = -EINVAL;
			goto err_4;
		}
	}

	mutex_lock(&dsps.mutex);
	list_add(&dsp->list, &dsps.list);
	mutex_unlock(&dsps.mutex);
	return 0;

err_4:
	device_for_each_child(dev, NULL, cvip_dsp_unregister_child);
err_3:
err_0:
	return ret;
}

static int cvip_dsp_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct dsp_dev *dsp = container_of(&pdev, struct dsp_dev, pdev);

	list_del(&dsp->list);
	device_for_each_child(dev, NULL, cvip_dsp_unregister_child);
	return 0;
}

#ifdef CONFIG_PM
static int cvip_dsp_suspend(struct platform_device *pdev, pm_message_t state)
{
	return dsp_suspend();
}

static int cvip_dsp_resume(struct platform_device *pdev)
{
	return dsp_resume();
}
#endif

static const struct of_device_id cvip_dsp_of_match[] = {
	{ .compatible = "amd,cvip-dsp", },
	{ },
};

static struct platform_driver cvip_dsp_driver = {
	.probe = cvip_dsp_probe,
	.remove = cvip_dsp_remove,
	.driver = {
		.name = "cvip_dsp",
		.of_match_table = cvip_dsp_of_match,
	},
#ifdef CONFIG_PM
	.suspend = cvip_dsp_suspend,
	.resume = cvip_dsp_resume,
#endif
};

static int __init cvip_dsp_init(void)
{
	mutex_init(&dsps.mutex);
	INIT_LIST_HEAD(&dsps.list);
	return platform_driver_register(&cvip_dsp_driver);
}

static void __exit cvip_dsp_exit(void)
{
	platform_driver_unregister(&cvip_dsp_driver);
}

/* late_initcall to let smmu initialize first */
late_initcall(cvip_dsp_init);
module_exit(cvip_dsp_exit);
MODULE_DESCRIPTION("cvip dsp driver");
