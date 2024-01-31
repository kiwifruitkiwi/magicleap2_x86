/*
 * Mero ION shared memory allocator implementation
 *
 * Copyright (c) 2011-2016, The Linux Foundation. All rights reserved.
 * Modification Copyright (c) 2019-2020, Advanced Micro Devices, Inc.
 * All rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <asm/page.h>
#include <linux/dma-mapping.h>
#include <linux/err.h>
#include <linux/highmem.h>
#include <linux/mm.h>
#include <linux/scatterlist.h>
#include <linux/seq_file.h>
#include <linux/slab.h>
#include <linux/dma-mapping.h>
#include <linux/dma-contiguous.h>
#include <linux/vmalloc.h>
#include <linux/cma.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/of_address.h>

#include "../ion.h"
#include "../../uapi/mero_ion.h"

#define ION_COMPAT_STR	"amd,mero-ion"

static int num_heaps;
static struct ion_heap **mero_heaps;
static bool b_heap_xport_created;

struct ion_heap_desc {
	unsigned int id;
	enum ion_heap_type type;
	const char *name;
	unsigned int permission_type;
};

static struct ion_heap_desc ion_heap_meta[] = {
	{
		.id	= ION_HEAP_CVIP_DDR_PRIVATE_ID,
		.name = ION_SYSTEM_HEAP_NAME,
	},
	{
		.id	= ION_HEAP_CVIP_SRAM_Q6_S0_ID,
		.name = "dsp_q6s0",
	},
	{
		.id	= ION_HEAP_CVIP_SRAM_C5_S0_ID,
		.name = "dsp_c5s0",
	},
	{
		.id	= ION_HEAP_CVIP_SRAM_C5_S1_ID,
		.name = "dsp_c5s1",
	},
	{
		.id	= ION_HEAP_CVIP_SRAM_Q6_M0_ID,
		.name = "dsp_q6m0",
	},
	{
		.id	= ION_HEAP_CVIP_SRAM_Q6_M1_ID,
		.name = "dsp_q6m1",
	},
	{
		.id	= ION_HEAP_CVIP_SRAM_Q6_M2_ID,
		.name = "dsp_q6m2",
	},
	{
		.id	= ION_HEAP_CVIP_SRAM_Q6_M3_ID,
		.name = "dsp_q6m3",
	},
	{
		.id	= ION_HEAP_CVIP_SRAM_Q6_M4_ID,
		.name = "dsp_q6m4",
	},
	{
		.id	= ION_HEAP_CVIP_SRAM_Q6_M5_ID,
		.name = "dsp_q6m5",
	},
	{
		.id	= ION_HEAP_CVIP_SRAM_C5_M0_ID,
		.name = "dsp_c5m0",
	},
	{
		.id	= ION_HEAP_CVIP_SRAM_C5_M1_ID,
		.name = "dsp_c5m1",
	},
	{
		.id	= ION_HEAP_CVIP_SRAM_A55_ID,
		.name = "a55_boot",
	},
	{
		.id	= ION_HEAP_CVIP_ISPIN_FMR5_ID,
		.name = ION_FMR5_HEAP_NAME,
	},
	{
		.id	= ION_HEAP_CVIP_HYDRA_FMR2_ID,
		.name = ION_FMR2_HEAP_NAME,
	},
	{
		.id	= ION_HEAP_CVIP_ISPOUT_FMR8_ID,
		.name = ION_FMR8_HEAP_NAME,
	},
	{
		.id	= ION_HEAP_CVIP_X86_SPCIE_FMR2_10_ID,
		.name = ION_FMR2_10_HEAP_NAME,
	},
	{
		.id	= ION_HEAP_CVIP_X86_READONLY_FMR10_ID,
		.name = ION_FMR10_HEAP_NAME,
	},
	{
		.id	= ION_HEAP_CVIP_READONLY_FMR4_ID,
		.name = ION_FMR4_HEAP_NAME,
	},
};

static struct ion_heap *mero_ion_heap_create(struct ion_platform_heap
							*heap_data)
{
	struct ion_heap *heap = NULL;

	heap = ion_heap_create(heap_data);

	if (IS_ERR_OR_NULL(heap)) {
		pr_err("%s: error creating heap %s type %d base %pa size %zu\n",
		       __func__, heap_data->name, heap_data->type,
		       &heap_data->base, heap_data->size);
		return ERR_PTR(-EINVAL);
	}

	heap->name = heap_data->name;
	heap->id = heap_data->id;
	heap->type = heap_data->type;
	return heap;
}

static void mero_ion_heap_destroy(struct ion_heap *heap)
{
	if (!heap) {
		pr_err("%s: Invalid heap pointer (null)\n", __func__);
		return;
	}

	/* remove it from ion device heap node plist */
	ion_device_del_heap(heap);

	ion_heap_destroy(heap);
}

/*
 * unregister the x86 shared heap created by mero_ion_heap_xport_create()
 */
int mero_ion_heap_xport_destroy(void)
{
	if (!b_heap_xport_created) {
		pr_err("%s: x-port heap not existed\n", __func__);
		return -1;
	}

	mero_ion_heap_destroy(mero_heaps[num_heaps]);
	pr_info("ION heap xport has been removed\n");

	b_heap_xport_created = false;
	return 0;
}
EXPORT_SYMBOL(mero_ion_heap_xport_destroy);

/*
 * create/register a x86 shared buffer heap at x-interface in run-time
 * caller to ensure address paddr is in x-interface aperture
 */
int mero_ion_heap_xport_create(phys_addr_t paddr, size_t size)
{
	struct ion_platform_heap heap_data;

	if (b_heap_xport_created) {
		pr_err("%s: error x-port heap already created\n",
		       __func__);
		return -1;
	}

	/* check address and size for x-interface aperture */
	if (paddr < MERO_CVIP_ADDR_XPORT_BASE ||
	    paddr >= (MERO_CVIP_ADDR_XPORT_BASE
		      + MERO_CVIP_XPORT_APERTURE_SIZE)) {
		pr_err("%s: error base paddr %pa not within x-port aperture\n",
		       __func__, &paddr);
		return -1;
	} else if ((paddr + size) > (MERO_CVIP_ADDR_XPORT_BASE
		   + MERO_CVIP_XPORT_APERTURE_SIZE)) {
		phys_addr_t end_paddr = paddr + size - 1;

		pr_err("%s: error end address %pa not within x-port aperture\n",
		       __func__, &end_paddr);
		return -1;
	}

	memset(&heap_data, 0, sizeof(heap_data));
	heap_data.id   = ION_HEAP_X86_CVIP_SHARED_ID;
	heap_data.name = ION_XPORT_HEAP_NAME;
	heap_data.type = ION_HEAP_TYPE_CARVEOUT;
	heap_data.base = paddr;
	heap_data.size = size;

	mero_heaps[num_heaps] = mero_ion_heap_create(&heap_data);

	if (IS_ERR_OR_NULL(mero_heaps[num_heaps])) {
		mero_heaps[num_heaps] = NULL;
		goto err_out;
	}

	pr_info("ION heap %s created at %pa with size %zx\n",
		heap_data.name, &heap_data.base, heap_data.size);
	ion_device_add_heap(mero_heaps[num_heaps]);

	b_heap_xport_created = true;
	return 0;

err_out:
	return -1;
}
EXPORT_SYMBOL(mero_ion_heap_xport_create);

long mero_ion_custom_ioctl(struct ion_client *client,
			   unsigned int cmd,
			   unsigned long arg)
{
	int ret;
	struct ion_heap_xport_data xport_data;
	struct ion_buf_physdata buf_data;

	switch (cmd) {
	case ION_IOC_MERO_REGISTER_XPORT_HEAP:
	{
		phys_addr_t paddr;

		if (_IOC_SIZE(cmd) > sizeof(xport_data)) {
			return -EINVAL;
		} else if (_IOC_DIR(cmd) & _IOC_WRITE) {
			if (copy_from_user(&xport_data,
					   (void __user *)arg,
					   _IOC_SIZE(cmd)))
			return -EFAULT;
		}

		/* shift address to x-interface aperture */
		paddr = xport_data.paddr + MERO_CVIP_ADDR_XPORT_BASE;
		ret = mero_ion_heap_xport_create(paddr, xport_data.size);
		if (ret)
			return ret;

		if (_IOC_DIR(cmd) & _IOC_READ) {
			if (copy_to_user((void __user *)arg,
					 &xport_data, _IOC_SIZE(cmd)))
			return -EFAULT;
		}
		break;
	}
	case ION_IOC_MERO_UNREGISTER_XPORT_HEAP:
		ret = mero_ion_heap_xport_destroy();
		if (ret)
			return ret;
		break;
	case ION_IOC_MERO_GET_BUFFER_PADDR:
	{
		struct dma_buf *dma_buf;
		struct sg_table *sg;

		if (_IOC_SIZE(cmd) > sizeof(buf_data)) {
			return -EINVAL;
		} else if (_IOC_DIR(cmd) & _IOC_WRITE) {
			if (copy_from_user(&buf_data,
					   (void __user *)arg,
					   _IOC_SIZE(cmd)))
			return -EFAULT;
		}

		dma_buf = dma_buf_get(buf_data.fd);
		if (IS_ERR(dma_buf)) {
			pr_err("%s: failed to get dma buffer\n", __func__);
			return -EFAULT;
		}

		sg = ion_sg_table(dma_buf);
		if (IS_ERR(sg)) {
			pr_err("%s: failed to get sg_table\n", __func__);
			dma_buf_put(dma_buf);
			return -EFAULT;
		}

		buf_data.paddr = page_to_phys(sg_page(sg->sgl));
		dma_buf_put(dma_buf);

		if (_IOC_DIR(cmd) & _IOC_READ) {
			if (copy_to_user((void __user *)arg,
					 &buf_data, _IOC_SIZE(cmd)))
			return -EFAULT;
		}
		break;
	}
	default:
		return -ENOTTY;
	}

	return 0;
}

struct ion_heap *get_ion_heap(int heap_id)
{
	int i;
	struct ion_heap *heap;

	for (i = 0; i < num_heaps; i++) {
		heap = mero_heaps[i];
		if (heap->id == heap_id)
			return heap;
	}

	if (b_heap_xport_created) {
		heap = mero_heaps[num_heaps];
		if (heap->id == heap_id)
			return heap;
	}

	pr_err("%s: heap_id %d not found\n", __func__, heap_id);
	return NULL;
}

#define MAKE_HEAP_TYPE_MAPPING(h) { .name = #h, \
			.heap_type = ION_HEAP_TYPE_##h, }

static struct heap_types_info {
	const char *name;
	int heap_type;
} heap_types_info[] = {
	MAKE_HEAP_TYPE_MAPPING(SYSTEM),
	MAKE_HEAP_TYPE_MAPPING(SYSTEM_CONTIG),
	MAKE_HEAP_TYPE_MAPPING(CARVEOUT),
	MAKE_HEAP_TYPE_MAPPING(CHUNK),
	MAKE_HEAP_TYPE_MAPPING(DMA),
};

static int mero_ion_get_heap_type_from_dt_node(struct device_node *node,
					       int *heap_type)
{
	const char *name;
	int i, ret = -EINVAL;

	ret = of_property_read_string(node, "mero,ion-heap-type", &name);
	if (ret)
		goto out;
	for (i = 0; i < ARRAY_SIZE(heap_types_info); ++i) {
		if (!strcmp(heap_types_info[i].name, name)) {
			*heap_type = heap_types_info[i].heap_type;
			ret = 0;
			goto out;
		}
	}
	WARN(1, "Unknown heap type: %s. You might need to update heap_types_info in %s",
	     name, __FILE__);
out:
	return ret;
}

static int mero_ion_populate_heap(struct device_node *node,
				  struct ion_platform_heap *heap)
{
	unsigned int i;
	int ret = -EINVAL, heap_type = -1;
	unsigned int len = ARRAY_SIZE(ion_heap_meta);

	for (i = 0; i < len; ++i) {
		if (ion_heap_meta[i].id == heap->id) {
			heap->name = ion_heap_meta[i].name;
			ret = mero_ion_get_heap_type_from_dt_node(node,
								  &heap_type);
			if (ret)
				break;
			heap->type = heap_type;
			break;
		}
	}
	if (ret)
		pr_err("%s: Unable to populate heap, error: %d", __func__, ret);
	return ret;
}

static void free_pdata(const struct ion_platform_data *pdata)
{
	unsigned int i;

	for (i = 0; i < pdata->nr; ++i)
		kfree(pdata->heaps[i].extra_data);
	kfree(pdata->heaps);
	kfree(pdata);
}

static void mero_ion_get_heap_dt_data(struct device_node *node,
				      struct ion_platform_heap *heap)
{
	struct device_node *pnode;
	const char *heap_clear_type;
	u64 offset;
	int ret;
	u32 *syscache_shift = NULL;

	pnode = of_parse_phandle(node, "memory-region", 0);
	if (pnode) {
		const __be32 *basep;
		u64 size;
		u64 base;

		basep = of_get_address(pnode,  0, &size, NULL);
		if (!basep) {
			base = cma_get_base(dev_get_cma_area(heap->priv));
			size = cma_get_size(dev_get_cma_area(heap->priv));
		} else {
			base = of_translate_address(pnode, basep);
			WARN(base == OF_BAD_ADDR, "Failed to parse DT node for heap %s\n",
			     heap->name);
		}
		heap->base = base;
		heap->size = size;
		of_node_put(pnode);
	}

	if (of_property_read_string(node, "mero,ion-heap-init-type",
				    &heap_clear_type) == 0) {
		if (!strcmp(heap_clear_type, "memory-no-clear"))
			heap->heap_flags |= ION_HEAP_FLAG_INIT_NO_MEMORY_CLEAR;
	}

	ret = of_property_read_u64(node, "mero,ion-heap-has-syscache", &offset);
	if (!ret) {
		heap->has_outer_cache = 1;
		syscache_shift = ioremap(offset, 0x4);
		if (syscache_shift) {
			u32 data = readl_relaxed(syscache_shift);

			if (data == 0x4)
				heap->heap_flags |= ION_HEAP_FLAG_HAS_SYSCACHE_SHIFT;
			else if (data == 0x0)
				heap->heap_flags |= ION_HEAP_FLAG_HAS_SYSCACHE;
			else
				pr_err("un-support syscache shift value!!\n");
			iounmap(syscache_shift);
		} else {
			heap->heap_flags |= ION_HEAP_FLAG_HAS_SYSCACHE;
		}
		pr_info("ION heap [%s] supports %s\n", heap->name,
			heap->heap_flags & ION_HEAP_FLAG_HAS_SYSCACHE_SHIFT ?
			"syscache with shift" : "syscache");
	}

}

static void mero_ion_allocate(struct ion_platform_heap *heap)
{
	if (!heap->base && heap->extra_data) {
		WARN(1, "Specifying carveout heaps without a base is deprecated. Convert to the DMA heap type instead");
		return;
	}
}

static struct ion_platform_data *mero_ion_parse_dt(struct platform_device *pdev)
{
	struct ion_platform_data *pdata = 0;
	struct ion_platform_heap *heaps = NULL;
	struct device_node *node;
	struct platform_device *new_dev = NULL;
	const struct device_node *dt_node = pdev->dev.of_node;
	const __be32 *val;
	int ret = -EINVAL;
	u32 num_heaps = 0;
	int idx = 0;

	for_each_available_child_of_node(dt_node, node)
		num_heaps++;

	if (!num_heaps)
		return ERR_PTR(-EINVAL);

	pdata = kzalloc(sizeof(*pdata), GFP_KERNEL);
	if (!pdata)
		return ERR_PTR(-ENOMEM);

	heaps = kcalloc(num_heaps, sizeof(struct ion_platform_heap),
			GFP_KERNEL);
	if (!heaps) {
		kfree(pdata);
		return ERR_PTR(-ENOMEM);
	}

	pdata->heaps = heaps;
	pdata->nr = num_heaps;

	for_each_available_child_of_node(dt_node, node) {
		new_dev = of_platform_device_create(node, NULL, &pdev->dev);
		if (!new_dev) {
			pr_err("Failed to create device %s\n", node->name);
			goto free_heaps;
		}

		pdata->heaps[idx].priv = &new_dev->dev;
		val = of_get_address(node, 0, NULL, NULL);
		if (!val) {
			pr_err("%s: Unable to find reg key", __func__);
			goto free_heaps;
		}
		pdata->heaps[idx].id = (u32)of_read_number(val, 1);

		ret = mero_ion_populate_heap(node, &pdata->heaps[idx]);
		if (ret)
			goto free_heaps;

		mero_ion_get_heap_dt_data(node, &pdata->heaps[idx]);

		++idx;
	}
	return pdata;

free_heaps:
	free_pdata(pdata);
	return ERR_PTR(ret);
}

static int mero_ion_probe(struct platform_device *pdev)
{
	struct ion_platform_data *pdata;
	unsigned int pdata_needs_to_be_freed;
	int err = -1;
	int i;

	if (pdev->dev.of_node) {
		pdata = mero_ion_parse_dt(pdev);
		if (IS_ERR(pdata))
			return PTR_ERR(pdata);
		pdata_needs_to_be_freed = 1;
	} else {
		pdata = pdev->dev.platform_data;
		pdata_needs_to_be_freed = 0;
	}

	num_heaps = pdata->nr;

	/* have extra one for x-port heap that may register later at run-time */
	mero_heaps = kcalloc(pdata->nr + 1,
			     sizeof(struct ion_heap *), GFP_KERNEL);
	b_heap_xport_created = false;

	if (!mero_heaps) {
		err = -ENOMEM;
		goto out;
	}

	err = ion_device_create(mero_ion_custom_ioctl);
	if (err) {
		/*
		 * set this to the ERR to indicate to the clients
		 * that Ion failed to probe.
		 */
		//err = PTR_ERR(new_dev);
		goto out;
	}

	/* create the heaps as specified in the board file */
	for (i = 0; i < num_heaps; i++) {
		struct ion_platform_heap *heap_data = &pdata->heaps[i];

		mero_ion_allocate(heap_data);

		heap_data->has_outer_cache = pdata->has_outer_cache;
		mero_heaps[i] = mero_ion_heap_create(heap_data);
		if (IS_ERR_OR_NULL(mero_heaps[i])) {
			mero_heaps[i] = NULL;
			continue;
		} else {
			if (heap_data->size)
				pr_info("ION heap %s created at %pa with size %zx\n",
					heap_data->name,
					&heap_data->base,
					heap_data->size);
			else
				pr_info("ION heap %s created\n",
					heap_data->name);
		}

		ion_device_add_heap(mero_heaps[i]);
	}
	if (pdata_needs_to_be_freed)
		free_pdata(pdata);

	return 0;

out:
	if (!mero_heaps)
		kfree(mero_heaps);
	if (pdata_needs_to_be_freed)
		free_pdata(pdata);
	return err;
}

static int mero_ion_remove(struct platform_device *pdev)
{
	int i;

	for (i = 0; i < num_heaps; i++)
		mero_ion_heap_destroy(mero_heaps[i]);

	if (b_heap_xport_created) {
		mero_ion_heap_destroy(mero_heaps[num_heaps]);
		b_heap_xport_created = false;
	}

	ion_device_destroy();
	kfree(mero_heaps);
	return 0;
}

static const struct of_device_id mero_ion_match_table[] = {
	{.compatible = ION_COMPAT_STR},
	{},
};

static struct platform_driver mero_ion_driver = {
	.probe = mero_ion_probe,
	.remove = mero_ion_remove,
	.driver = {
		.name = "ion-mero",
		.of_match_table = mero_ion_match_table,
	},
};

static int __init mero_ion_init(void)
{
	return platform_driver_register(&mero_ion_driver);
}

static void __exit mero_ion_exit(void)
{
	platform_driver_unregister(&mero_ion_driver);
}

subsys_initcall(mero_ion_init);
module_exit(mero_ion_exit);
