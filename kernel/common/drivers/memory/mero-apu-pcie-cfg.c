/*
 * Copyright (C) 2019 Advanced Micro Devices, Inc. All rights reserved.
 */

#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/iommu.h>
#include <linux/interrupt.h>
#include <linux/mutex.h>

/**
 * struct apu_pcie_cfg_dev - pcie_cfg device information
 * @aperture_base: aperture base address
 * @aperture_size: aperture size
 * @map_base: mapped memory base address
 * @map_size: mapped memory size
 * @irq: irq number
 * @map_mutex: mutex lock for aperture mapping
 * @d_dir: debug fs directory
 * @dev: struct device for pcie_cfg
 */
struct apu_pcie_cfg_device {
	phys_addr_t aperture_base;
	size_t aperture_size;
	phys_addr_t map_base;
	size_t map_size;
	int irq;
	struct mutex map_mutex; /*mutex lock for aperture mapping*/
#ifdef CONFIG_MERO_APU_PCIE_CFG_DEBUGFS
	struct dentry *d_dir;
#endif
	struct device *dev;
};

static int
apu_pcie_cfg_aperture_remap(struct apu_pcie_cfg_device *pcie_cfg_dev,
			    phys_addr_t paddr, size_t size)
{
	int ret;
	struct iommu_domain *domain;

	if (!pcie_cfg_dev) {
		pr_err("NULL pcie_cfg_dev!!\n");
		return -EINVAL;
	}

	ret = mutex_lock_interruptible(&pcie_cfg_dev->map_mutex);
	if (ret) {
		pr_err("Error in mutex_lock: %d\n", ret);
		return -ERESTART;
	}

	domain = iommu_get_domain_for_dev(pcie_cfg_dev->dev);
	if (!domain) {
		pr_err("NULL iommu_domain\n");
		ret = -ENOENT;
		goto err;
	}

	if (pcie_cfg_dev->map_size > 0) {
		size_t sz = iommu_unmap(domain,
			pcie_cfg_dev->aperture_base, pcie_cfg_dev->map_size);
		if (sz != pcie_cfg_dev->map_size)
			pr_err("Unmap IOMMU returns 0x%zx, expected 0x%zx\n",
			       sz, pcie_cfg_dev->map_size);
		pcie_cfg_dev->map_size = 0;
		pcie_cfg_dev->map_base = 0;
	}

	if (size > pcie_cfg_dev->aperture_size) {
		pr_warn("Map size too large, limited to 0x%zx\n",
			pcie_cfg_dev->aperture_size);
		size = pcie_cfg_dev->aperture_size;
	}

	if (size > 0) {
		ret = iommu_map(domain,
				pcie_cfg_dev->aperture_base, paddr, size,
			IOMMU_READ | IOMMU_WRITE);
		if (ret) {
			pr_err("Error in iommu_map(0x%llx, 0x%zx): %d\n",
			       paddr, size, ret);
			goto err;
		}

		pcie_cfg_dev->map_base = paddr;
		pcie_cfg_dev->map_size = size;

		pr_info("Remapped APU pcie_cfg aperture to 0x%llx~0x%llx\n",
			paddr, paddr + size);
	} else {
		pr_warn("Map size is %zu, expecting positive number\n", size);
	}

	mutex_unlock(&pcie_cfg_dev->map_mutex);
	return 0;

err:
	mutex_unlock(&pcie_cfg_dev->map_mutex);
	return ret;
}

static irqreturn_t
apu_pcie_cfg_irq_handler(int inqno, void *dev)
{
	/* TODO: handle TBU interrupts */
	pr_debug("APU pcie_cfg interrupt\n");
	return IRQ_HANDLED;
}

#ifdef CONFIG_MERO_APU_PCIE_CFG_DEBUGFS
#include <linux/debugfs.h>
static int set_map_base(void *data, u64 base)
{
	int ret;
	struct apu_pcie_cfg_device *pcie_cfg_dev =
					(struct apu_pcie_cfg_device *)data;

	if (!pcie_cfg_dev) {
		pr_err("NULL pcie_cfg_dev!!\n");
		return -EINVAL;
	}

	if (base != pcie_cfg_dev->map_base) {
		ret = apu_pcie_cfg_aperture_remap(pcie_cfg_dev,
						  base,
						  pcie_cfg_dev->map_size);
		if (ret) {
			pr_err("Failed to remap pcie_cfg to base 0x%llx\n",
			       base);
			return ret;
		}
	}
	return 0;
}

static int set_map_size(void *data, u64 size)
{
	int ret;
	struct apu_pcie_cfg_device *pcie_cfg_dev =
			(struct apu_pcie_cfg_device *)data;

	if (!pcie_cfg_dev) {
		pr_err("NULL pcie_cfg_dev!!\n");
		return -EINVAL;
	}

	if (size != pcie_cfg_dev->map_size) {
		ret = apu_pcie_cfg_aperture_remap(pcie_cfg_dev,
						  pcie_cfg_dev->map_base, size);
		if (ret) {
			pr_err("Failed to remap pcie_cfg to size 0x%llx\n",
			       size);
			return ret;
		}
	}
	return 0;
}

static int get_map_base(void *data, u64 *val)
{
	struct apu_pcie_cfg_device *pcie_cfg_dev =
				(struct apu_pcie_cfg_device *)data;

	if (!pcie_cfg_dev) {
		pr_err("NULL pcie_cfg_dev!!\n");
		return -EINVAL;
	}

	*val = pcie_cfg_dev->map_base;
	return 0;
}

static int get_map_size(void *data, u64 *val)
{
	struct apu_pcie_cfg_device *pcie_cfg_dev =
				(struct apu_pcie_cfg_device *)data;

	if (!pcie_cfg_dev) {
		pr_err("NULL pcie_cfg_dev!!\n");
		return -EINVAL;
	}

	*val = pcie_cfg_dev->map_size;
	return 0;
}
DEFINE_SIMPLE_ATTRIBUTE(map_base_fops,
			get_map_base, set_map_base, "0x%llx\n");
DEFINE_SIMPLE_ATTRIBUTE(map_size_fops,
			get_map_size, set_map_size, "0x%llx\n");
#endif

static int apu_pcie_cfg_remove(struct platform_device *pdev)
{
	struct apu_pcie_cfg_device *pcie_cfg_dev = platform_get_drvdata(pdev);

	if (!pcie_cfg_dev) {
		pr_err("NULL pcie_cfg_dev!!\n");
		return -EINVAL;
	}

	if (apu_pcie_cfg_aperture_remap(pcie_cfg_dev, 0, 0))
		pr_err("Failed to reset pcie_cfg aperture mapping\n");

#ifdef CONFIG_MERO_APU_PCIE_CFG_DEBUGFS
	debugfs_remove_recursive(pcie_cfg_dev->d_dir);
#endif

	free_irq(pcie_cfg_dev->irq, pcie_cfg_dev);
	mutex_destroy(&pcie_cfg_dev->map_mutex);
	kfree(pcie_cfg_dev);
	return 0;
}

static int set_nbio_user_bits(struct apu_pcie_cfg_device *pcie_cfg_dev)
{
	int ret = 0;
	phys_addr_t addr;
	u32 size;
	u32 default_value;
	void __iomem    *vaddr = NULL;

	ret = device_property_read_u32(pcie_cfg_dev->dev,
				       "nbio_user_bits_size", &size);
	if (ret) {
		pr_debug("Error find \"nbio_user_bits_size\" prop: %d\n",
			 ret);
		size = 0;
	}

	ret = device_property_read_u32(pcie_cfg_dev->dev,
				       "nbio_user_bits_default_value",
				       &default_value);
	if (ret) {
		pr_debug("Error find \"nbio_user_bits_default_value\" prop: %d\n",
			 ret);
		default_value = 0;
	}

	ret = device_property_read_u64(pcie_cfg_dev->dev,
				       "nbio_user_bits_addr",
				       &addr);
	if (ret) {
		pr_debug("Error find \"nbio_user_bits_addr\" prop: %d\n",
			 ret);
	} else {
		vaddr = ioremap(addr, size);
		if (!vaddr) {
			pr_err("Error mapping NBIO_USER_BITS register !\n");
			return -ENOMEM;
		}
		writel(default_value, vaddr);
		iounmap(vaddr);
	}
	return ret;
}

static int apu_pcie_cfg_fault_handler(struct iommu_domain *domain,
				      struct device *dev,
				      unsigned long paddr, int flags,
				      void *data)
{
	struct iommu_fault_info *fault;

	fault = (struct iommu_fault_info *)domain->handler_token;
	if (!fault) {
		pr_err("%s: Error, NULL fault !\n", __func__);
		return -EINVAL;
	}
	dev_err(dev, "iova = 0x%llx, pa = 0x%llx, pte = 0x%llx, op = %s\n",
		fault->iova, fault->pa, fault->pte, flags ? "write" : "read");
	return 0;
}

static int apu_pcie_cfg_set_fault_handler(struct apu_pcie_cfg_device
					  *pcie_cfg_dev)
{
	int ret;
	struct iommu_domain *domain;

	domain = iommu_get_domain_for_dev(pcie_cfg_dev->dev);
	if (!domain) {
		pr_err("NULL iommu_domain\n");
		ret = -ENOENT;
		return ret;
	}

	iommu_set_fault_handler(domain, apu_pcie_cfg_fault_handler,
				pcie_cfg_dev);
	return 0;
}

static int apu_pcie_cfg_probe(struct platform_device *pdev)
{
	int ret;
	struct resource *iommu_aperture;
	struct resource *tbu_irq;
	struct apu_pcie_cfg_device *pcie_cfg_dev;
	phys_addr_t map_base;
	size_t map_size;

	iommu_aperture = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!iommu_aperture) {
		pr_err("Error getting aperture info\n");
		return -EINVAL;
	}

	tbu_irq = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	if (!tbu_irq) {
		pr_err("Error getting interrupt info\n");
		return -EINVAL;
	}

	pcie_cfg_dev = kzalloc(sizeof(*pcie_cfg_dev), GFP_KERNEL);
	if (!pcie_cfg_dev)
		return -ENOMEM;

	pcie_cfg_dev->dev = &pdev->dev;
	pcie_cfg_dev->irq = tbu_irq->start;
	mutex_init(&pcie_cfg_dev->map_mutex);

	ret = apu_pcie_cfg_set_fault_handler(pcie_cfg_dev);
	if (ret) {
		pr_err("Error in setting fault_handler\n");
		goto err;
	}

	ret = request_irq(pcie_cfg_dev->irq, apu_pcie_cfg_irq_handler, 0,
			  "apu_pcie_cfg_tbu", pcie_cfg_dev);
	if (ret) {
		pr_err("Error in request_irq: %d\n", ret);
		goto err;
	}

	pcie_cfg_dev->aperture_base = iommu_aperture->start;
	pcie_cfg_dev->aperture_size = resource_size(iommu_aperture);
	pcie_cfg_dev->map_base = 0;
	pcie_cfg_dev->map_size = 0;

	ret = set_nbio_user_bits(pcie_cfg_dev);
	if (ret) {
		pr_err("Error in set_nbio_user_bits\n");
		goto err;
	}
	ret = device_property_read_u64(pcie_cfg_dev->dev,
				       "map_base", &map_base);
	if (ret) {
		pr_debug("Error find \"map_base\" prop: %d, default to 0x0\n",
			 ret);
		map_base = 0;
	}

	ret = device_property_read_u64(pcie_cfg_dev->dev, "map_size",
				       (u64 *)&map_size);
	if (ret) {
		pr_debug("Error find \"map_size\" prop: %d, default to 0x0\n",
			 ret);
		map_size = 0;
	}

	ret = apu_pcie_cfg_aperture_remap(pcie_cfg_dev, map_base, map_size);
	if (ret)
		pr_warn("Error performing initial mapping: %d\n", ret);

	platform_set_drvdata(pdev, pcie_cfg_dev);

#ifdef CONFIG_MERO_APU_PCIE_CFG_DEBUGFS
	pcie_cfg_dev->d_dir = debugfs_create_dir("apu-pcie_cfg", NULL);
	if (IS_ERR(pcie_cfg_dev->d_dir)) {
		pr_warn("Failed to create debugfs: %ld\n",
			PTR_ERR(pcie_cfg_dev->d_dir));
		pcie_cfg_dev->d_dir = NULL;
	} else {
		debugfs_create_file("map_base", 0644, pcie_cfg_dev->d_dir,
				    pcie_cfg_dev, &map_base_fops);
		debugfs_create_file("map_size", 0644, pcie_cfg_dev->d_dir,
				    pcie_cfg_dev, &map_size_fops);
	}
#endif

	pr_info("APU pcie_cfg device initialized\n");

	return 0;

err:
#ifdef CONFIG_MERO_APU_PCIE_CFG_DEBUGFS
	debugfs_remove_recursive(pcie_cfg_dev->d_dir);
#endif
	free_irq(pcie_cfg_dev->irq, pcie_cfg_dev);
	mutex_destroy(&pcie_cfg_dev->map_mutex);
	kfree(pcie_cfg_dev);
	return ret;
}

static const struct of_device_id apu_pcie_cfg_of_match[] = {
	{ .compatible = "amd,apu-pcie-cfg", },
	{ },
};

static struct platform_driver apu_pcie_cfg_driver = {
	.probe = apu_pcie_cfg_probe,
	.remove = apu_pcie_cfg_remove,
	.driver = {
		.name = "apu-pcie-cfg",
		.of_match_table = apu_pcie_cfg_of_match,
	},
};
module_platform_driver(apu_pcie_cfg_driver);

MODULE_DESCRIPTION("MERO APU PCIE_CFG Driver");
