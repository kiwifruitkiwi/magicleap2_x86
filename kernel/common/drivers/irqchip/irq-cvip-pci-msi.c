// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2013-2015 ARM Limited, All Rights Reserved.
 * Author: Marc Zyngier <marc.zyngier@arm.com>
 *
 * Modification Copyright (C) 2021, Advanced Micro Devices, Inc.
 * All rights reserved.
 */

#define pr_fmt(fmt) "CVIP-MSI: " fmt

#include <linux/acpi.h>
#include <linux/dma-iommu.h>
#include <linux/irq.h>
#include <linux/irqdomain.h>
#include <linux/kernel.h>
#include <linux/msi.h>
#include <linux/of_address.h>
#include <linux/of_pci.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/irqchip.h>
#include <linux/irqchip/arm-gic-v3.h>
#include <linux/irqchip/arm-gic-v4.h>
#include <linux/pci_cvip.h>

struct cvip_msi_data {
	struct list_head entry;
	struct fwnode_handle *fwnode;
	u32 *msi_start;		/* The SPI number that MSIs start */
	u32 nr_msis;		/* The number of SPIs for MSIs */
	u32 msix_start;		/* The SPI number that MSIXs start */
	u32 nr_msixs;		/* The number of SPIs for MSIXs */
	unsigned long *msi_bm;	/* MSI vector bitmap */
	unsigned long *msix_bm;	/* MSIX vector bitmap */
};

#define CVIP_MSI_MULTI_BASE	185
#define CVIP_MSI_MULTI_NUM	32
#define CVIP_SPI_BASE		32

static u32 cvip_spi_single_map[] = {
	106, 107, 108, 109, 139, 141, 144, 145
};

static LIST_HEAD(cvip_msi_nodes);
static DEFINE_SPINLOCK(cvip_msi_lock);

static void cvip_msi_mask_irq(struct irq_data *d)
{
	pci_msi_mask_irq(d);
	irq_chip_mask_parent(d);
}

static void cvip_msi_unmask_irq(struct irq_data *d)
{
	pci_msi_unmask_irq(d);
	irq_chip_unmask_parent(d);
}

static phys_addr_t cvip_pci_get_msi_addr(void)
{
	return cvip_get_sintf_base();
}

static void cvip_msi_domain_write_msg(struct irq_data *data,
				      struct msi_msg *msg)
{
	u64 addr;

	addr = cvip_pci_get_msi_addr();
	msg->address_lo		= lower_32_bits(addr);
	msg->address_hi		= upper_32_bits(addr);
	msg->data		= data->hwirq;

	iommu_dma_compose_msi_msg(irq_data_get_msi_desc(data), msg);
}

static struct irq_chip cvip_msi_irq_chip = {
	.name			= "CVIP-MSI",
	.irq_unmask		= cvip_msi_unmask_irq,
	.irq_mask		= cvip_msi_mask_irq,
	.irq_eoi		= irq_chip_eoi_parent,
	.irq_write_msi_msg	= pci_msi_domain_write_msg,
	.irq_compose_msi_msg	= cvip_msi_domain_write_msg,
	.irq_set_affinity	= irq_chip_set_affinity_parent,
};

static int cvip_irq_gic_domain_alloc(struct irq_domain *domain,
				     unsigned int virq,
				     irq_hw_number_t hwirq)
{
	struct irq_fwspec fwspec;
	struct irq_data *d;
	struct irq_desc *desc;
	int err;

	if (is_of_node(domain->parent->fwnode)) {
		fwspec.fwnode = domain->parent->fwnode;
		fwspec.param_count = 3;
		fwspec.param[0] = GIC_IRQ_TYPE_LPI;
		fwspec.param[1] = hwirq;
		fwspec.param[2] = IRQ_TYPE_EDGE_RISING;
	} else if (is_fwnode_irqchip(domain->parent->fwnode)) {
		fwspec.fwnode = domain->parent->fwnode;
		fwspec.param_count = 2;
		fwspec.param[0] = hwirq;
		fwspec.param[1] = IRQ_TYPE_EDGE_RISING;
	} else {
		return -EINVAL;
	}

	err = irq_domain_alloc_irqs_parent(domain, virq, 1, &fwspec);
	if (err)
		return err;

	/* Configure the interrupt line to be edge */
	d = irq_domain_get_irq_data(domain->parent, virq);
	d->chip->irq_set_type(d, IRQ_TYPE_EDGE_RISING);

	/* set irq affinity of the msi irq based on virq# */
	desc = irq_data_to_desc(d);
	cpumask_copy(desc->irq_common_data.affinity, cpumask_of(virq % nr_cpu_ids));
	irqd_mark_affinity_was_set(d);
	return 0;
}

static void cvip_msi_unalloc_msi(struct cvip_msi_data *cvip_msi,
				 unsigned int hwirq,
				 int nr_irqs)
{
	int i;

	spin_lock(&cvip_msi_lock);
	for (i = 0; i < cvip_msi->nr_msis; i++) {
		if (hwirq == cvip_msi->msi_start[i] + CVIP_SPI_BASE) {
			bitmap_release_region(cvip_msi->msi_bm, i,
					      get_count_order(nr_irqs));
			spin_unlock(&cvip_msi_lock);
			return;
		}
	}

	for (i = 0; i < cvip_msi->nr_msixs; i++) {
		if (hwirq == cvip_msi->msix_start + i) {
			bitmap_release_region(cvip_msi->msix_bm, i,
					      get_count_order(nr_irqs));
			break;
		}
	}
	spin_unlock(&cvip_msi_lock);
}

static int cvip_msi_irq_domain_alloc(struct irq_domain *domain,
				     unsigned int virq,
				     unsigned int nr_irqs,
				     void *args)
{
	msi_alloc_info_t *info = args;
	struct cvip_msi_data *cvip_msi = NULL, *tmp;
	irq_hw_number_t hwirq;
	int pos = 0, err, i;

	spin_lock(&cvip_msi_lock);
	if (info->desc->msi_attrib.is_msix) {
		list_for_each_entry(tmp, &cvip_msi_nodes, entry) {
			pos = bitmap_find_free_region(tmp->msix_bm,
						      tmp->nr_msixs,
						      get_count_order(nr_irqs));
			if (pos >= 0) {
				cvip_msi = tmp;
				break;
			}
		}
		if (!cvip_msi) {
			spin_unlock(&cvip_msi_lock);
			return -ENOSPC;
		}

		hwirq = cvip_msi->msix_start + pos;
	} else {
		list_for_each_entry(tmp, &cvip_msi_nodes, entry) {
			pos = bitmap_find_free_region(tmp->msi_bm, tmp->nr_msis,
						      get_count_order(nr_irqs));
			if (pos >= 0) {
				cvip_msi = tmp;
				break;
			}
		}
		if (!cvip_msi) {
			spin_unlock(&cvip_msi_lock);
			return -ENOSPC;
		}
		hwirq = cvip_msi->msi_start[pos] + CVIP_SPI_BASE;
	}
	spin_unlock(&cvip_msi_lock);

	pr_info("setup MSI, hwirq#=%ld, virq = %d, nvecs = %d\n",
		hwirq, virq, nr_irqs);

	err = iommu_dma_prepare_msi(info->desc,
				    cvip_pci_get_msi_addr());
	if (err)
		return err;

	for (i = 0; i < nr_irqs; i++) {
		err = cvip_irq_gic_domain_alloc(domain, virq + i, hwirq + i);
		if (err)
			goto fail;

		irq_domain_set_hwirq_and_chip(domain, virq + i, hwirq + i,
					      &cvip_msi_irq_chip, cvip_msi);
	}

	return 0;

fail:
	irq_domain_free_irqs_parent(domain, virq, nr_irqs);
	cvip_msi_unalloc_msi(cvip_msi, hwirq, nr_irqs);
	return err;
}

static void cvip_msi_irq_domain_free(struct irq_domain *domain,
				     unsigned int virq,
				     unsigned int nr_irqs)
{
	struct irq_data *d = irq_domain_get_irq_data(domain, virq);
	struct cvip_msi_data *cvip_msi = irq_data_get_irq_chip_data(d);

	cvip_msi_unalloc_msi(cvip_msi, d->hwirq, nr_irqs);
	irq_domain_free_irqs_parent(domain, virq, nr_irqs);
}

static const struct irq_domain_ops cvip_msi_domain_ops = {
	.alloc = cvip_msi_irq_domain_alloc,
	.free = cvip_msi_irq_domain_free,
};

static struct msi_domain_info cvip_msi_domain_info = {
	.flags	= (MSI_FLAG_USE_DEF_DOM_OPS | MSI_FLAG_USE_DEF_CHIP_OPS |
		   MSI_FLAG_MULTI_PCI_MSI | MSI_FLAG_PCI_MSIX),
	.chip	= &cvip_msi_irq_chip,
};

static const struct of_device_id cvip_msi_device_id[] = {
	{	.compatible	= "amd,cvip-pci-msi",	},
	{},
};

static int cvip_msi_allocate_domains(struct irq_domain *parent,
				     const char *name)
{
	struct irq_domain *inner_domain, *pci_domain;
	struct cvip_msi_data *cvip_msi;

	cvip_msi = list_first_entry_or_null(&cvip_msi_nodes,
					    struct cvip_msi_data, entry);
	if (!cvip_msi)
		return -EINVAL;

	inner_domain = irq_domain_create_tree(cvip_msi->fwnode,
					      &cvip_msi_domain_ops, cvip_msi);
	if (!inner_domain) {
		pr_err("Failed to create cvip_msi domain\n");
		return -ENOMEM;
	}

	irq_domain_update_bus_token(inner_domain, DOMAIN_BUS_NEXUS);
	inner_domain->parent = parent;
	pci_domain = pci_msi_create_irq_domain(cvip_msi->fwnode,
					       &cvip_msi_domain_info,
					       inner_domain);
	if (!pci_domain) {
		pr_err("Failed to create MSI domains %s\n", name);
		irq_domain_remove(inner_domain);
		return -ENOMEM;
	}

	return 0;
}

static int __init cvip_msi_init_one(struct fwnode_handle *fwnode)
{
	struct cvip_msi_data *cvip_msi;
	int ret;

	cvip_msi = kzalloc(sizeof(*cvip_msi), GFP_KERNEL);
	if (!cvip_msi)
		return -ENOMEM;

	INIT_LIST_HEAD(&cvip_msi->entry);
	cvip_msi->fwnode = fwnode;

	cvip_msi->msi_start = cvip_spi_single_map;
	cvip_msi->nr_msis = ARRAY_SIZE(cvip_spi_single_map);
	cvip_msi->msix_start = CVIP_MSI_MULTI_BASE;
	cvip_msi->nr_msixs = CVIP_MSI_MULTI_NUM;

	cvip_msi->msi_bm = kcalloc(BITS_TO_LONGS(cvip_msi->nr_msis),
				   sizeof(long), GFP_KERNEL);
	if (!cvip_msi->msi_bm) {
		ret = -ENOMEM;
		goto err;
	}

	cvip_msi->msix_bm = kcalloc(BITS_TO_LONGS(cvip_msi->nr_msixs),
				    sizeof(long), GFP_KERNEL);
	if (!cvip_msi->msix_bm) {
		ret = -ENOMEM;
		goto err2;
	}

	list_add_tail(&cvip_msi->entry, &cvip_msi_nodes);

	return 0;

err2:
	kfree(cvip_msi->msi_bm);
err:
	kfree(cvip_msi);
	return ret;
}

static int __init cvip_msi_of_init(struct fwnode_handle *parent_handle,
				   struct irq_domain *parent)
{
	int ret = -1;
	struct device_node *node = to_of_node(parent_handle);
	struct device_node *child;

	for (child = of_find_matching_node(node, cvip_msi_device_id); child;
	     child = of_find_matching_node(child, cvip_msi_device_id)) {
		if (!of_device_is_available(child))
			continue;
		if (!of_property_read_bool(child, "msi-controller"))
			continue;

		ret = cvip_msi_init_one(of_node_to_fwnode(child));
		if (ret < 0)
			continue;
		else
			break;
	}

	if (!ret) {
		ret = cvip_msi_allocate_domains(parent, child->full_name);
		if (ret < 0)
			pr_err("Failed to create %pOF domain\n", child);
		else
			pr_info("CVIP PCI/MSI: %pOF domain created\n", child);
	}

	return ret;
}

int __init cvip_msi_init(struct fwnode_handle *parent_handle,
			 struct irq_domain *parent)
{
	if (is_of_node(parent_handle))
		return cvip_msi_of_init(parent_handle, parent);

	return -1;
}

