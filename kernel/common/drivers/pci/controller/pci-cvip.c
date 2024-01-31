// SPDX-License-Identifier: GPL-2.0
/*
 * Generic PCI host driver common code
 *
 * Copyright (C) 2014 ARM Limited
 *
 * Author: Will Deacon <will.deacon@arm.com>
 */
/*
 * Modifications Copyright (C) 2019-2020 Advanced Micro Devices, Inc. All rights reserved.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/of_address.h>
#include <linux/of_pci.h>
#include <linux/of_platform.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <linux/pci-ecam.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/pci.h>
#include <linux/irq.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <asm/traps.h>
#include <linux/irqdomain.h>
#include <linux/dma-direct.h>
#include <linux/dma-mapping.h>
#include <linux/smu_protocol.h>
#include <linux/cvip_event_logger.h>
#include <linux/sched/clock.h>
#include <asm/cacheflush.h>

/*
 * here is ECAM offset format:
 * (typ << 28) | (bus << 20) | (dev << 15) | (fun << 12) | (offs << 0)
 */
#define ECAM_TYPE0 (0x0 << 28)
#define ECAM_TYPE1 (0x1 << 28)
#define DEV_FUN_SHIFT 12
#define BUS_SHIFT 20

/* TODO: hardcode message address for workaround */
#define GICD_SETSPI_OFF 0x00100040

#define SEC_FUNC_DEVID (0x163d)
#define SEC_FUNC_VENDOR (0x1022)
#define SEC_FUNC_S_AXI_BAR 2
#define SEC_USB_DEVID (0x163c)
#define SEC_USB_VENDOR (0x1022)

#define FLUSH_REQ_BIT BIT(8)
#define FLUSH_DONE_BIT BIT(0)

#define SYS_CACHE_SFT_DIR_PATH 0x83c

#define CVIP_DRAM_FLUSH_REQ BIT(8)
#define CVIP_DRAM_FLUSH_DONE BIT(0)
#define MAX_FLUSH_WAIT 100

#define CVIP_PCI_EVENT_LOG_SIZE 2048 /* max 2K log size */

/*
 * PCIe DMA address will need to be updated with the the shift register value
 * to avoid CVIP FMR region crosses the 16GB region
 */
static u64 dma_addr_shift;

static int cvip_pci_log;
static u64 cvip_vc1flush_tmin = ~0UL;
static u64 cvip_vc1flush_tmax;

static void __iomem *nbif_flush_ctrl;
static bool ecam_enable;

static int __init ecam_param(char *str)
{
	ecam_enable = true;

	return 1;
}
__setup("ECAM", ecam_param);

static int cvip_pci_wait_dram_flush(u32 timeout_us);

static dma_addr_t get_dma_addr(phys_addr_t pa)
{
	/* TODO: this should be OR instead of ADD.
	 * this should be OR instead of ADD.
	 * but to pair with get_dma_pfn_offset, use ADD here.
	 */
	return pa + (dma_addr_shift << 32) - 0x800000000ULL;
}

static unsigned long get_dma_pfn_offset(void)
{
	return (0x800000000ULL - (dma_addr_shift << 32)) >> PAGE_SHIFT;
}

static void __iomem *pci_amd_ecam_map_bus(struct pci_bus *bus,
					  unsigned int devfn, int where)
{
	struct pci_config_window *cfg = bus->sysdata;
	unsigned int busn = bus->number;
	void __iomem *base, *ret;
	u32 ecam_type = ECAM_TYPE0;

	if (busn < cfg->busr.start || busn > cfg->busr.end)
		return NULL;

	/*
	 * For non-root device, i.e. devices behind a bridge, need to set
	 * up Type1 access
	 */
	if (!pci_is_root_bus(bus))
		ecam_type = ECAM_TYPE1;

	base = cfg->win + (busn << cfg->ops->bus_shift);

	ret = base + (devfn << DEV_FUN_SHIFT) + where + ecam_type;

	return ret;
}

static int pci_amd_config_read(struct pci_bus *bus, unsigned int devfn,
			       int where, int size, u32 *val)
{
	int ret;

	skip_memabort_fault(true);
	ret = pci_generic_config_read(bus, devfn, where, size, val);
	skip_memabort_fault(false);
	return ret;
}

static int pci_amd_config_write(struct pci_bus *bus, unsigned int devfn,
				int where, int size, u32 val)
{
	return pci_generic_config_write(bus, devfn, where, size, val);
}

static struct pci_ecam_ops pci_amd_ecam_bus_ops = {
	.bus_shift	= BUS_SHIFT,
	.pci_ops	= {
		.map_bus	= pci_amd_ecam_map_bus,
		.read		= pci_amd_config_read,
		.write		= pci_amd_config_write,
	}
};

static const struct of_device_id cvip_pci_of_match[] = {
	{ .compatible = "pci-host-ecam-amd",
	  .data = &pci_amd_ecam_bus_ops },

	{ },
};

static int cvip_pci_unregister_child(struct device *dev, void *data)
{
	struct platform_device *child = to_platform_device(dev);

	of_device_unregister(child);

	return 0;
}

u64 cvip_get_sintf_base(void)
{
	int r;
	u16 cmd;
	pci_bus_addr_t s_axi_busaddr;
	struct pci_dev *spcie = pci_get_device(SEC_FUNC_VENDOR,
					       SEC_FUNC_DEVID, NULL);

	if (!spcie) {
		pr_err("Failed to find SecFunc. abort allocating MSI\n");
		return -ENODEV;
	}

	/* hardcode the MMIO enable bit */
	r = pci_read_config_word(spcie, PCI_COMMAND, &cmd);
	WARN_ON(r);
	if (!r && !(cmd & PCI_COMMAND_MEMORY)) {
		cmd |= PCI_COMMAND_MEMORY;
		r = pci_write_config_word(spcie, PCI_COMMAND, cmd);
		WARN_ON(r != 0);
	}

	s_axi_busaddr = pci_bus_address(spcie, SEC_FUNC_S_AXI_BAR);

	return s_axi_busaddr + GICD_SETSPI_OFF;
}
EXPORT_SYMBOL(cvip_get_sintf_base);

static struct resource spcie_dma_mem;
static struct resource susb_dma_mem;
static struct resource ion_heap_mem;
static struct device *ecam_dev;

/* TODO:
 * replace this workaround with a better solution
 * Upper layer stack (fs, bio, etc.) always allocate kernel memory
 * for PCIe device DMA.
 * Instead of changing all the upper layer stack, we setup our own
 * dma_map_ops to replace the backing pages with FMR2/3 pages before
 * DMA, and copying the data back to the kernel memory after DMA.
 */
struct dma_map_node {
	dma_addr_t dma_addr;
	size_t dma_size;
	void *cpu_addr;
	union {
		struct {
			struct page *page;
			unsigned long offset;
			size_t size;
		} single;
		struct {
			struct sg_table table;
			size_t npages;
		} sg;
	} data;
	struct rb_node rbnode;
};

static struct dma_map_node*
find_map_node_by_dma_nolock(struct rb_root *root, dma_addr_t daddr)
{
	struct rb_node *node = root->rb_node;

	while (node) {
		struct dma_map_node *data = rb_entry(node,
			struct dma_map_node, rbnode);

		if (daddr >= data->dma_addr &&
		    daddr < (data->dma_addr + data->dma_size))
			return data;
		else if (daddr < data->dma_addr)
			node = node->rb_left;
		else
			node = node->rb_right;
	}
	return NULL;
}

static struct dma_map_node*
find_map_node_by_sg_nolock(struct rb_root *root,
			   struct scatterlist *sgl, int nents)
{
	dma_addr_t daddr = sg_dma_address(sgl) - sgl->offset;

	return find_map_node_by_dma_nolock(root, daddr);
}

static int
insert_map_node_nolock(struct rb_root *root, struct dma_map_node *data)
{
	struct rb_node **new = &root->rb_node, *parent = NULL;
	struct dma_map_node *this;

	while (*new) {
		this = container_of(*new, struct dma_map_node, rbnode);

		parent = *new;
		if (data->dma_addr < this->dma_addr)
			new = &((*new)->rb_left);
		else if (data->dma_addr >= (this->dma_addr + this->dma_size))
			new = &((*new)->rb_right);
		else
			return -EBUSY;
	}

	rb_link_node(&data->rbnode, parent, new);
	rb_insert_color(&data->rbnode, root);
	return 0;
}

static struct rb_root dma_map_rb_root;
static spinlock_t dma_map_lock;

#define map_dbg dev_dbg

void cvip_pci_flush_dram(unsigned int timeout)
{
	if (!nbif_flush_ctrl)
		return;

	if (cvip_pci_wait_dram_flush(timeout))
		pr_warn("Flush data to DRAM timeout!\n");
}
EXPORT_SYMBOL(cvip_pci_flush_dram);

static int in_ion_heap_range(phys_addr_t phys, size_t size)
{
	if (phys >= ion_heap_mem.start &&
	    phys + size <= ion_heap_mem.end)
		return 1;

	return 0;
}

static int cvip_pci_wait_dram_flush(u32 timeout_us)
{
	u32 reg;
	u64 t0, t, tout;

	t0 = local_clock();
	tout = timeout_us * 1000;

	writel_relaxed(CVIP_DRAM_FLUSH_REQ, nbif_flush_ctrl);
	/* make sure FLUSH write */
	wmb();

	do {
		reg = readl_relaxed(nbif_flush_ctrl);
		if (reg & CVIP_DRAM_FLUSH_DONE) {
			writel_relaxed(CVIP_DRAM_FLUSH_DONE, nbif_flush_ctrl);
			/* make sure FLUSH write */
			wmb();

			/* collect some stats */
			t = local_clock() - t0;
			if (t < cvip_vc1flush_tmin)
				cvip_vc1flush_tmin = t;
			if (t > cvip_vc1flush_tmax)
				cvip_vc1flush_tmax = t;
			CVIP_EVTLOG(cvip_pci_log, "min/max/cur",
				    cvip_vc1flush_tmin, cvip_vc1flush_tmax, t);
			return 0;
		}
		t = local_clock() - t0;
	} while (t < tout);

	writel_relaxed(CVIP_DRAM_FLUSH_DONE, nbif_flush_ctrl);
	/* make sure FLUSH write */
	wmb();

	CVIP_EVTLOG(cvip_pci_log, "TIMEOUT", timeout_us);
	return -ETIMEDOUT;
}

static void cvip_pci_sync_single_for_cpu(struct device *dev,
					 dma_addr_t daddr, size_t size,
					 enum dma_data_direction dir)
{
	struct page *page;
	struct dma_map_node *map_node;
	unsigned long flags;

	if (in_ion_heap_range(dma_to_phys(dev, daddr), size))
		return dma_direct_sync_single_for_cpu(dev, daddr, size, dir);

	spin_lock_irqsave(&dma_map_lock, flags);
	map_node = find_map_node_by_dma_nolock(&dma_map_rb_root, daddr);
	spin_unlock_irqrestore(&dma_map_lock, flags);

	if (WARN_ON(!map_node)) {
		dev_err(dev, "%s: failed to find map node\n", __func__);
		return;
	}

	page = map_node->data.single.page;

	/* add VC1_FLUSH here */
	if (cvip_pci_wait_dram_flush(MAX_FLUSH_WAIT))
		dev_err(dev, "Timeout waiting for DRAM flush!!\n");

	__inval_dcache_area(page_address(page) + map_node->data.single.offset,
			    map_node->data.single.size);
	map_dbg(dev, "%s PA 0x%016llx ==>> DA 0x%016llx / sz=0x%lx\n",
		__func__, page_to_phys(page), map_node->dma_addr, size);
}

static void cvip_pci_sync_single_for_device(struct device *dev,
					    dma_addr_t daddr, size_t size,
					    enum dma_data_direction dir)
{
	struct page *page;
	struct dma_map_node *map_node;
	unsigned long flags;

	if (in_ion_heap_range(dma_to_phys(dev, daddr), size))
		return dma_direct_sync_single_for_device(dev, daddr, size, dir);

	spin_lock_irqsave(&dma_map_lock, flags);
	map_node = find_map_node_by_dma_nolock(&dma_map_rb_root, daddr);
	spin_unlock_irqrestore(&dma_map_lock, flags);

	if (WARN_ON(!map_node)) {
		dev_err(dev, "%s: failed to find map node\n", __func__);
		return;
	}

	__flush_dcache_area(map_node->cpu_addr + map_node->data.single.offset,
			    map_node->data.single.size);
	page = map_node->data.single.page;
	map_dbg(dev, "%s PA 0x%016llx ==>> DA 0x%016llx / sz=0x%lx\n",
		__func__, page_to_phys(page), map_node->dma_addr, size);
}

static void cvip_pci_sync_sg_for_cpu(struct device *dev,
				     struct scatterlist *sgl, int nents,
				     enum dma_data_direction dir)
{
	dev_info(dev, "%s: 0x%016llx\n", __func__, sg_dma_address(sgl));
}

static void cvip_pci_sync_sg_for_device(struct device *dev,
					struct scatterlist *sgl, int nents,
					enum dma_data_direction dir)
{
	dev_info(dev, "%s: 0x%016llx\n", __func__, sg_dma_address(sgl));
}

static dma_addr_t cvip_pci_map_page(struct device *dev, struct page *page,
				    unsigned long offset, size_t size,
				    enum dma_data_direction dir,
				    unsigned long attrs)
{
	int r;
	struct dma_map_node *map_node = NULL;
	size_t alloc_size = PAGE_ALIGN(offset + size);
	unsigned long flags;

	if (in_ion_heap_range((phys_addr_t)page_address(page) + offset, size))
		return dma_direct_map_page(dev, page, offset, size, dir, attrs);

	map_node = kzalloc(sizeof(*map_node), GFP_KERNEL);
	if (WARN_ON(!map_node)) {
		dev_err(dev, "%s: failed to kmalloc map node\n", __func__);
		return DMA_MAPPING_ERROR;
	}

	map_node->cpu_addr = dma_alloc_coherent(dev, alloc_size,
						&map_node->dma_addr,
						GFP_KERNEL);
	if (WARN_ON(!map_node->cpu_addr)) {
		kfree(map_node);
		dev_err(dev, "%s failed to alloc dma page\n", __func__);
		return DMA_MAPPING_ERROR;
	}
	map_node->dma_size = alloc_size;
	map_node->data.single.page = page;
	map_node->data.single.offset = offset;
	map_node->data.single.size = size;

	spin_lock_irqsave(&dma_map_lock, flags);
	r = insert_map_node_nolock(&dma_map_rb_root, map_node);
	spin_unlock_irqrestore(&dma_map_lock, flags);

	if (WARN_ON(r)) {
		dma_free_coherent(dev, alloc_size,
				  map_node->cpu_addr,
				  map_node->dma_addr);
		kfree(map_node);
		dev_err(dev, "%s failed to add map node\n", __func__);
		return DMA_MAPPING_ERROR;
	}

	if (dir != DMA_FROM_DEVICE)
		memcpy(map_node->cpu_addr + offset,
		       page_address(page) + offset, size);

	map_dbg(dev, "%s PA 0x%016llx ==>> DA 0x%016llx / size:0x%lx\n",
		__func__, page_to_phys(page), map_node->dma_addr, size);
	return map_node->dma_addr + offset;
}

static void cvip_pci_unmap_page(struct device *dev, dma_addr_t daddr,
				size_t size, enum dma_data_direction dir,
				unsigned long attrs)
{
	struct dma_map_node *map_node = NULL;
	void *src, *dst;
	size_t free_size;
	unsigned long flags;

	if (in_ion_heap_range(dma_to_phys(dev, daddr), size))
		return dma_direct_unmap_page(dev, daddr, size, dir, attrs);

	spin_lock_irqsave(&dma_map_lock, flags);
	map_node = find_map_node_by_dma_nolock(&dma_map_rb_root, daddr);

	if (map_node)
		rb_erase(&map_node->rbnode, &dma_map_rb_root);

	if (WARN_ON(!map_node)) {
		dev_err(dev, "%s failed to unmap DA 0x%016llx, sz 0x%zx\n",
			__func__, daddr, size);
		spin_unlock_irqrestore(&dma_map_lock, flags);
		return;
	}

	dst = page_address(map_node->data.single.page)
		+ map_node->data.single.offset;
	src = map_node->cpu_addr + map_node->data.single.offset;

	if (dir != DMA_TO_DEVICE)
		/* add VC1_FLUSH here */
		if (cvip_pci_wait_dram_flush(MAX_FLUSH_WAIT))
			dev_err(dev, "Timeout waiting for DRAM flush!!\n");
	spin_unlock_irqrestore(&dma_map_lock, flags);

	if (dir != DMA_TO_DEVICE)
		memcpy(dst, src, map_node->data.single.size);

	free_size = map_node->data.single.offset + map_node->data.single.size;
	free_size = PAGE_ALIGN(free_size);
	dma_free_coherent(dev, free_size,
			  map_node->cpu_addr, map_node->dma_addr);
	kfree(map_node);
	map_dbg(dev, "%s <<== DA 0x%016llx / sz:0x%lx\n", __func__,
		daddr, size);
}

static int cvip_pci_map_sg(struct device *dev, struct scatterlist *sgl,
			   int nents, enum dma_data_direction dir,
			   unsigned long attrs)
{
	int r, i;
	struct dma_map_node *map_node = NULL;
	struct scatterlist *sg, *new_sg;
	unsigned long addr_offset;
	void *dst;
	size_t size_all;
	unsigned long flags;

	if (in_ion_heap_range(page_to_phys(sg_page(sgl)), sgl->length))
		return dma_direct_map_sg(dev, sgl, nents, dir, attrs);

	map_node = kzalloc(sizeof(*map_node), GFP_KERNEL);
	if (WARN_ON(!map_node)) {
		dev_err(dev, "%s: failed to kmalloc map node\n", __func__);
		return 0;
	}

	r = sg_alloc_table(&map_node->data.sg.table,
			   nents, GFP_KERNEL);
	if (WARN_ON(r)) {
		dev_err(dev, "%s: failed to alloc sg table\n", __func__);
		kfree(map_node);
		return 0;
	}

	size_all = 0;
	for_each_sg(sgl, sg, nents, i)
		size_all += PAGE_ALIGN(sg->offset + sg->length);
	map_node->data.sg.npages = size_all >> PAGE_SHIFT;
	map_node->dma_size = size_all;

	map_node->cpu_addr = dma_alloc_coherent(dev, size_all,
						&map_node->dma_addr,
						GFP_KERNEL);
	if (WARN_ON(!map_node->cpu_addr)) {
		dev_err(dev, "%s failed to alloc dma page\n", __func__);
		sg_free_table(&map_node->data.sg.table);
		kfree(map_node);
		return 0;
	}

	new_sg = map_node->data.sg.table.sgl;
	addr_offset = 0;
	dst = map_node->cpu_addr;

	map_dbg(dev, "%s begin\n", __func__);
	for_each_sg(sgl, sg, nents, i) {
		size_t size_aligned = PAGE_ALIGN(sg->offset + sg->length);
		phys_addr_t pa = page_to_phys(sg_page(sg));

		sg_dma_address(sg) = map_node->dma_addr
			+ addr_offset + sg->offset;
		sg_dma_len(sg) = sg->length;
		map_dbg(dev, "%s  [%d] PA 0x%llx ==>> DA 0x%llx, len 0x%x\n",
			__func__, i, pa, sg_dma_address(sg), sg_dma_len(sg));
		if (dir != DMA_FROM_DEVICE)
			memcpy(dst + sg->offset, sg_virt(sg), sg->length);
		addr_offset += size_aligned;
		dst += size_aligned;
		*new_sg = *sg;
		new_sg = sg_next(new_sg);
	}

	map_dbg(dev, "%s end\n", __func__);

	spin_lock_irqsave(&dma_map_lock, flags);
	r = insert_map_node_nolock(&dma_map_rb_root, map_node);
	spin_unlock_irqrestore(&dma_map_lock, flags);

	if (WARN_ON(r)) {
		dev_err(dev, "%s failed to add map node\n", __func__);
		dma_free_coherent(dev, map_node->data.sg.npages << PAGE_SHIFT,
				  map_node->cpu_addr, map_node->dma_addr);
		sg_free_table(&map_node->data.sg.table);
		kfree(map_node);
		return 0;
	}
	return nents;
}

static void cvip_pci_unmap_sg(struct device *dev,
			      struct scatterlist *sgl, int nents,
			      enum dma_data_direction dir,
			      unsigned long attrs)
{
	int i;
	struct scatterlist *sg;
	void *src;
	struct dma_map_node *map_node;
	unsigned long flags;

	if (in_ion_heap_range(page_to_phys(sg_page(sgl)), sgl->length))
		return dma_direct_unmap_sg(dev, sgl, nents, dir, attrs);

	spin_lock_irqsave(&dma_map_lock, flags);
	map_node = find_map_node_by_sg_nolock(&dma_map_rb_root, sgl, nents);
	if (map_node)
		rb_erase(&map_node->rbnode, &dma_map_rb_root);
	spin_unlock_irqrestore(&dma_map_lock, flags);

	if (WARN_ON(!map_node)) {
		dev_err(dev, "%s failed to find map node ()\n", __func__);
		return;
	}

	map_dbg(dev, "%s begin\n", __func__);
	src = map_node->cpu_addr;

	if (dir != DMA_TO_DEVICE)
		/* add VC1_FLUSH here */
		if (cvip_pci_wait_dram_flush(MAX_FLUSH_WAIT))
			dev_err(dev, "Timeout waiting for DRAM flush!!\n");

	for_each_sg(sgl, sg, nents, i) {
		phys_addr_t pa = page_to_phys(sg_page(sg));

		map_dbg(dev, "%s   [%d] PA 0x%llx <<== DA 0x%llx, len 0x%x\n",
			__func__, i, pa, sg_dma_address(sg), sg_dma_len(sg));
		if (dir != DMA_TO_DEVICE)
			memcpy(sg_virt(sg), src + sg->offset, sg->length);
		sg_dma_address(sg) = 0;
		src += PAGE_ALIGN(sg->offset + sg->length);
	}
	map_dbg(dev, "%s end\n", __func__);

	sg_free_table(&map_node->data.sg.table);
	dma_free_coherent(dev, map_node->data.sg.npages << PAGE_SHIFT,
			  map_node->cpu_addr, map_node->dma_addr);
	kfree(map_node);
}

static const struct dma_map_ops cvip_pci_dma_ops = {
	.map_page = cvip_pci_map_page,
	.unmap_page = cvip_pci_unmap_page,
	.map_sg = cvip_pci_map_sg,
	.unmap_sg = cvip_pci_unmap_sg,
	.sync_single_for_cpu = cvip_pci_sync_single_for_cpu,
	.sync_single_for_device = cvip_pci_sync_single_for_device,
	.sync_sg_for_cpu = cvip_pci_sync_sg_for_cpu,
	.sync_sg_for_device = cvip_pci_sync_sg_for_device,
};

void cvip_pci_setup_dma_mem(struct pci_dev *pdev)
{
	int err;
	dma_addr_t daddr;
	size_t mem_size;

	if (pdev->subsystem_vendor == SEC_USB_VENDOR &&
	    pdev->subsystem_device == SEC_USB_DEVID) {
		mem_size = susb_dma_mem.end - susb_dma_mem.start;
		daddr = get_dma_addr(susb_dma_mem.start);
		err = dma_declare_coherent_memory(&pdev->dev,
						  susb_dma_mem.start,
						  daddr, mem_size);
	} else {
		/* TODO:
		 * there will possibly be issues when removing devices.
		 * since there is no refcount control in the coherent memory.
		 * But we could ignore it for we don't remove devices now.
		 */
		err = dma_attach_coherent_memory(&pdev->dev, ecam_dev);
	}
	pdev->dev.dma_pfn_offset = get_dma_pfn_offset();
	set_dma_ops(&pdev->dev, &cvip_pci_dma_ops);
	WARN_ON(err);
}
EXPORT_SYMBOL(cvip_pci_setup_dma_mem);

static void gen_pci_unmap_cfg(void *ptr)
{
	pci_ecam_free((struct pci_config_window *)ptr);
}

static struct
pci_config_window *gen_pci_init(struct device *dev,
				struct list_head *resources,
				struct pci_ecam_ops *ops)
{
	int err;
	struct resource cfgres;
	struct resource *bus_range = NULL;
	struct pci_config_window *cfg;

	/* Parse our PCI ranges and request their resources */
	err = pci_parse_request_of_pci_ranges(dev, resources, &bus_range);
	if (err)
		return ERR_PTR(err);

	err = of_address_to_resource(dev->of_node, 0, &cfgres);
	if (err) {
		dev_err(dev, "missing \"reg\" property\n");
		goto err_out;
	}

	cfg = pci_ecam_create(dev, &cfgres, bus_range, ops);
	if (IS_ERR(cfg)) {
		err = PTR_ERR(cfg);
		goto err_out;
	}

	err = devm_add_action(dev, gen_pci_unmap_cfg, cfg);
	if (err) {
		gen_pci_unmap_cfg(cfg);
		goto err_out;
	}
	return cfg;

err_out:
	pci_free_resource_list(resources);
	return ERR_PTR(err);
}

static int cvip_pci_host_probe(struct platform_device *pdev,
			       struct pci_ecam_ops *ops)
{
	struct device *dev = &pdev->dev;
	struct pci_host_bridge *bridge;
	struct pci_config_window *cfg;
	struct list_head resources;
	int ret;

	bridge = devm_pci_alloc_host_bridge(dev, 0);
	if (!bridge)
		return -ENOMEM;

	of_pci_check_probe_only();

	/* Parse and map our Configuration Space windows */
	cfg = gen_pci_init(dev, &resources, ops);
	if (IS_ERR(cfg))
		return PTR_ERR(cfg);

	/* Do not reassign resources if probe only */
	if (!pci_has_flag(PCI_PROBE_ONLY))
		pci_add_flags(PCI_REASSIGN_ALL_BUS);

	list_splice_init(&resources, &bridge->windows);
	bridge->dev.parent = dev;
	bridge->sysdata = cfg;
	bridge->busnr = cfg->busr.start;
	bridge->ops = &ops->pci_ops;
	bridge->map_irq = of_irq_parse_and_map_pci;
	bridge->swizzle_irq = pci_common_swizzle;
	bridge->msi = NULL;

	ret = pci_host_probe(bridge);
	if (ret < 0) {
		pci_free_resource_list(&resources);
		return ret;
	}

	platform_set_drvdata(pdev, bridge->bus);
	return 0;
}

static irqreturn_t irq_thread(int irq, void *dev_id)
{
	unsigned int detect_pin = (u64)dev_id;
	struct smu_msg msg;

	/*
	 * There will be a pulse generated by MCU on gpio320 to
	 * trigger a interrupt when the gpio is pulled down. So
	 * it's safe to get the gpio states after 10ms in the
	 * interrupt thread.
	 */
	mdelay(10);

	if (gpio_get_value(detect_pin)) {
		msg.id = CVSMC_MSG_pciehotremove;
		pr_info("PCIe hot-remove event happened\n");
	} else {
		msg.id = CVSMC_MSG_pciehotadd;
		pr_info("PCIe hot-add event happened\n");
	}

	smu_send_msgs(&msg, 1);

	return IRQ_HANDLED;
}

static int cvip_setup_hotplug(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct resource *res;
	int detect_pin;
	int err;

	res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	if (!res) {
		dev_err(dev, "Failed to get pcie hotplug irq\n");
		return -EINVAL;
	}

	detect_pin = of_get_named_gpio(dev->of_node, "detect_pin", 0);
	if (!gpio_is_valid(detect_pin)) {
		dev_err(dev, "Failed to get detect pin\n");
		return -ENODEV;
	}

	err = devm_gpio_request_one(dev, detect_pin, GPIOF_IN, "pcie_hp");
	if (err) {
		dev_err(dev, "Failed to request gpio%d\n", detect_pin);
		return err;
	}

	err = devm_request_threaded_irq(&pdev->dev, res->start,
					NULL, irq_thread,
					IRQF_ONESHOT,
					"PCIe Hotplug",
					(void *)(u64)detect_pin);
	if (err) {
		dev_err(dev, "Failed to request handler for IRQ%lld\n",
			res->start);
		return err;
	}

	return 0;
}

static void cvip_pci_hardcode_smn(phys_addr_t addr, u32 val, u32 mask)
{
	const phys_addr_t smn_base = 0x100000000ULL;
	phys_addr_t smn_addr = smn_base + addr;
	phys_addr_t map_base = smn_addr & PAGE_MASK;
	void __iomem *p = ioremap(map_base, PAGE_SIZE);
	u32 __iomem *reg = (u32 __iomem *)(p + (smn_addr - map_base));
	u32 v = *reg;
	u32 new_v = (v & (~mask)) | (val);

	*reg = new_v;
	iounmap(p);
	pr_info("Hardcoded SMN reg 0x%016llx from 0x%08x to 0x%08x (mask 0x%08x)\n",
		addr, v, new_v, mask);
}

static int cvip_pci_probe(struct platform_device *pdev)
{
	const struct of_device_id *of_id;
	struct pci_ecam_ops *ops;
	struct device_node *np;
	char child_name[128];
	struct platform_device *child;
	struct device *dev = &pdev->dev;
	struct resource *res;
	void __iomem *cvip_base;
	int err;
	struct device_node *dma_node;
	size_t mem_size;

	cvip_pci_log = CVIP_EVTLOGINIT(CVIP_PCI_EVENT_LOG_SIZE);

	of_id = of_match_node(cvip_pci_of_match, pdev->dev.of_node);
	ops = (struct pci_ecam_ops *)of_id->data;

	/* Create platform devs for dts child nodes */
	for_each_available_child_of_node(dev->of_node, np) {
		snprintf(child_name, sizeof(child_name), "%s-%s",
			 dev_name(dev), np->name);
		child = of_platform_device_create(np, child_name, dev);
		if (!child) {
			dev_err(dev, "failed to create child %s dev\n",
				child_name);
			err = -EINVAL;
			goto exit;
		}
	}

	dma_map_rb_root = RB_ROOT;
	spin_lock_init(&dma_map_lock);
	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "cvip");
	if (!res) {
		err = -EINVAL;
		goto exit;
	}

	cvip_base = devm_ioremap(&pdev->dev, res->start, resource_size(res));
	if (!cvip_base) {
		err = -ENOMEM;
		goto exit;
	}

	dma_addr_shift = readl_relaxed(cvip_base + SYS_CACHE_SFT_DIR_PATH);

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "flushctl");
	if (!res) {
		err = -EINVAL;
		goto exit;
	}

	nbif_flush_ctrl = devm_ioremap(&pdev->dev, res->start,
				       resource_size(res));
	if (!nbif_flush_ctrl) {
		err = -ENOMEM;
		goto exit;
	}

	dma_node = of_parse_phandle(pdev->dev.of_node, "secure-dma-region", 0);
	if (!dma_node) {
		pr_err("No spcie memory-region specified\n");
		err = -EINVAL;
		goto exit;
	}
	err = of_address_to_resource(dma_node, 0, &spcie_dma_mem);
	if (err) {
		err = -EINVAL;
		goto exit;
	}

	mem_size = spcie_dma_mem.end - spcie_dma_mem.start;
	err = dma_declare_coherent_memory(&pdev->dev, spcie_dma_mem.start,
					  get_dma_addr(spcie_dma_mem.start),
					  mem_size);
	if (err) {
		err = -EINVAL;
		goto exit;
	}
	ecam_dev  = &pdev->dev;
	pdev->dev.dma_pfn_offset = get_dma_pfn_offset();
	set_dma_ops(&pdev->dev, &cvip_pci_dma_ops);

	dma_node = of_parse_phandle(pdev->dev.of_node, "secure-dma-region", 1);
	if (!dma_node) {
		pr_err("No susb memory-region specified\n");
		err = -EINVAL;
		goto exit;
	}
	err = of_address_to_resource(dma_node, 0, &susb_dma_mem);
	if (err) {
		err = -EINVAL;
		goto exit;
	}

	dma_node = of_parse_phandle(pdev->dev.of_node, "secure-dma-region", 2);
	if (!dma_node) {
		pr_err("No ion heap memory-region specified\n");
		err = -EINVAL;
		goto exit;
	}
	err = of_address_to_resource(dma_node, 0, &ion_heap_mem);
	if (err) {
		err = -EINVAL;
		goto exit;
	}

	err = cvip_setup_hotplug(pdev);
	if (err)
		goto exit;

	/* NBIFMM:INTR_LINE_ENABLE */
	cvip_pci_hardcode_smn(0x1013a008, 0x000100ff, 0x000100ff);

	err = cvip_pci_host_probe(pdev, ops);

	return err;

exit:
	device_for_each_child(dev, NULL, cvip_pci_unregister_child);
	ecam_dev = NULL;
	return err;
}

static int cvip_pci_remove(struct platform_device *pdev)
{
	int ret_a, ret_b;
	struct device *dev = &pdev->dev;

	ecam_dev = NULL;
	ret_a = pci_host_common_remove(pdev);
	if (ret_a != 0)
		dev_warn(dev, "failed(%d) to remove pdev\n", ret_a);

	ret_b = device_for_each_child(dev, NULL, cvip_pci_unregister_child);
	if (ret_b != 0)
		dev_warn(dev, "failed(%d) to unregister child devices\n",
			 ret_b);

	memset(&spcie_dma_mem, 0, sizeof(spcie_dma_mem));
	memset(&susb_dma_mem, 0, sizeof(susb_dma_mem));

	return ret_a | ret_b;
}

static struct platform_driver cvip_pci_driver = {
	.driver = {
		.name = "cvip-pci-root",
		.of_match_table = cvip_pci_of_match,
		.suppress_bind_attrs = true,
	},
	.probe = cvip_pci_probe,
	.remove = cvip_pci_remove,
};

static int __init cvip_pci_init(void)
{
	if (ecam_enable)
		return platform_driver_register(&cvip_pci_driver);
	else
		return -ENODEV;
}

static void __exit cvip_pci_exit(void)
{
	if (ecam_enable)
		platform_driver_unregister(&cvip_pci_driver);
	else
		return;
}

late_initcall(cvip_pci_init);
module_exit(cvip_pci_exit);

