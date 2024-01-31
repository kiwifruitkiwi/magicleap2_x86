// SPDX-License-Identifier: GPL-2.0
/*
 * ION Memory Allocator carveout heap helper
 *
 * Copyright (C) 2011 Google, Inc.
 */

#include <linux/dma-mapping.h>
#include <linux/err.h>
#include <linux/genalloc.h>
#include <linux/io.h>
#include <linux/mm.h>
#include <linux/scatterlist.h>
#include <linux/slab.h>

#include "ion.h"

#define ION_CARVEOUT_ALLOCATE_FAIL	-1

struct ion_carveout_heap {
	struct ion_heap heap;
	struct gen_pool *pool;
};

static phys_addr_t ion_carveout_allocate(struct ion_heap *heap,
					 unsigned long size)
{
	struct ion_carveout_heap *carveout_heap =
		container_of(heap, struct ion_carveout_heap, heap);
	unsigned long offset = gen_pool_alloc(carveout_heap->pool, size);

	if (!offset)
		return ION_CARVEOUT_ALLOCATE_FAIL;

	return offset;
}

static void ion_carveout_free(struct ion_heap *heap, phys_addr_t addr,
			      unsigned long size)
{
	struct ion_carveout_heap *carveout_heap =
		container_of(heap, struct ion_carveout_heap, heap);

	if (addr == ION_CARVEOUT_ALLOCATE_FAIL)
		return;

	gen_pool_free(carveout_heap->pool, addr, size);
}

static int ion_carveout_heap_allocate(struct ion_heap *heap,
				      struct ion_buffer *buffer,
				      unsigned long size,
				      unsigned long flags)
{
	struct sg_table *table, *table2;
	phys_addr_t paddr;
	int ret;
	u64 page_sft = 0;
	u64 pfn;
	int b_flag_alt, b_flag_slc, b_heap_fmr;

	/* check buffer allocation flags */
	b_heap_fmr = is_ion_buffer_heap_fmr(1 << heap->id);
	b_flag_alt = flags & (ION_FLAG_CVIP_DRAM_ALT0 |
			      ION_FLAG_CVIP_DRAM_ALT1);
	b_flag_slc = is_ion_buffer_flag_syscache(flags);
	if (b_flag_alt && b_flag_slc) {
		pr_err("%s: Not supporting flags: SYSCACHE & ALT\n", __func__);
		return -EINVAL;
	}
	if (b_flag_alt && !b_heap_fmr) {
		pr_err("%s: Not supporting ALT on heap %d\n",
		       __func__, heap->id);
		return -EINVAL;
	}

	table = kmalloc(sizeof(*table), GFP_KERNEL);
	if (!table)
		return -ENOMEM;
	ret = sg_alloc_table(table, 1, GFP_KERNEL);
	if (ret)
		goto err_free;

	paddr = ion_carveout_allocate(heap, size);
	if (paddr == ION_CARVEOUT_ALLOCATE_FAIL) {
		ret = -ENOMEM;
		goto err_free_table;
	}

	pfn = PFN_DOWN(paddr);
	sg_set_page(table->sgl, pfn_to_page(pfn), size, 0);
	buffer->sg_table = table;

	page_sft = 0;
	buffer->sg_extbl = NULL;
	if (b_flag_alt) {
		if (flags & ION_FLAG_CVIP_DRAM_ALT0)
			page_sft = CVIP_ADDR_APU_DIRECT_DRAM_ALT0 - CVIP_ADDR_APU_DIRECT_DRAM;
		else if (flags & ION_FLAG_CVIP_DRAM_ALT1)
			page_sft = CVIP_ADDR_APU_DIRECT_DRAM_ALT1 - CVIP_ADDR_APU_DIRECT_DRAM;
	} else if (b_flag_slc && (heap->flags & (ION_HEAP_FLAG_HAS_SYSCACHE |
		   ION_HEAP_FLAG_HAS_SYSCACHE_SHIFT))) {
		/* check for system cache flags */
		if (flags & ION_FLAG_CVIP_SLC_CACHED_0)
			page_sft = CVIP_ADDR_SYSTEMCACHE_0 - CVIP_ADDR_APU_DIRECT_DRAM;
		else if (flags & ION_FLAG_CVIP_SLC_CACHED_1)
			page_sft = CVIP_ADDR_SYSTEMCACHE_1 - CVIP_ADDR_APU_DIRECT_DRAM;
		else if (flags & ION_FLAG_CVIP_SLC_CACHED_2)
			page_sft = CVIP_ADDR_SYSTEMCACHE_2 - CVIP_ADDR_APU_DIRECT_DRAM;
		else if (flags & ION_FLAG_CVIP_SLC_CACHED_3)
			page_sft = CVIP_ADDR_SYSTEMCACHE_3 - CVIP_ADDR_APU_DIRECT_DRAM;

		if (page_sft && (heap->flags & ION_HEAP_FLAG_HAS_SYSCACHE_SHIFT))
			page_sft -= CVIP_ADDR_16GB;
	}

	page_sft >>= PAGE_SHIFT;
	if (page_sft) {
		table2 = kmalloc(sizeof(*table2), GFP_KERNEL);
		if (!table2) {
			ret = -ENOMEM;
			goto err_free_table;
		}

		ret = sg_alloc_table(table2, 1, GFP_KERNEL);
		if (ret)
			goto free_table2;

		pfn += page_sft;
		sg_set_page(table2->sgl, pfn_to_page(pfn), size, 0);
		buffer->sg_extbl = table2;

		pr_debug("ION heap %s allocates syscache @ 0x%llX (paddr=0x%llX), size = 0x%lx\n",
			 heap->name, (u64)(paddr + page_sft), (u64)paddr, size);
	}

	if (!is_heap_clear_enable(heap->flags))
		buffer->flags |= ION_BUFFER_FLAG_NO_MEMORY_CLEAR;

	return 0;

free_table2:
	kfree(table2);
err_free_table:
	sg_free_table(table);
err_free:
	kfree(table);
	return ret;
}

static void ion_carveout_heap_free(struct ion_buffer *buffer)
{
	struct ion_heap *heap = buffer->heap;
	struct sg_table *table = buffer->sg_table;
	struct sg_table *table2 = buffer->sg_extbl;
	struct page *page = sg_page(table->sgl);
	phys_addr_t paddr = PFN_PHYS(page_to_pfn(page));

	if (is_buffer_clear_enable(buffer->flags))
		ion_heap_buffer_zero(buffer);

	ion_carveout_free(heap, paddr, buffer->size);
	sg_free_table(table);
	kfree(table);

	if (table2) {
		sg_free_table(table2);
		kfree(table2);
	}
}

static struct ion_heap_ops carveout_heap_ops = {
	.allocate = ion_carveout_heap_allocate,
	.free = ion_carveout_heap_free,
	.map_user = ion_heap_map_user,
	.map_kernel = ion_heap_map_kernel,
	.unmap_kernel = ion_heap_unmap_kernel,
};

struct ion_heap *ion_carveout_heap_create(struct ion_platform_heap *heap_data)
{
	struct ion_carveout_heap *carveout_heap;
	struct page *page;
	int ret;

	if (is_heap_clear_enable(heap_data->heap_flags)) {
		page = pfn_to_page(PFN_DOWN(heap_data->base));
		ret = ion_heap_pages_zero(page, heap_data->size,
					  pgprot_writecombine(PAGE_KERNEL));
		if (ret)
			return ERR_PTR(ret);
	}

	carveout_heap = kzalloc(sizeof(*carveout_heap), GFP_KERNEL);
	if (!carveout_heap)
		return ERR_PTR(-ENOMEM);

	carveout_heap->pool = gen_pool_create(PAGE_SHIFT, -1);
	if (!carveout_heap->pool) {
		kfree(carveout_heap);
		return ERR_PTR(-ENOMEM);
	}
	gen_pool_add(carveout_heap->pool, heap_data->base, heap_data->size, -1);
	carveout_heap->heap.ops = &carveout_heap_ops;
	carveout_heap->heap.type = ION_HEAP_TYPE_CARVEOUT;
	carveout_heap->heap.flags = 0;

	if (!is_heap_clear_enable(heap_data->heap_flags))
		carveout_heap->heap.flags |= ION_HEAP_FLAG_INIT_NO_MEMORY_CLEAR;

	if (is_heap_has_syscache(heap_data->heap_flags))
		carveout_heap->heap.flags |= ION_HEAP_FLAG_HAS_SYSCACHE;
	else if (is_heap_has_syscache_shift(heap_data->heap_flags))
		carveout_heap->heap.flags |= ION_HEAP_FLAG_HAS_SYSCACHE_SHIFT;

	return &carveout_heap->heap;
}

void ion_carveout_heap_destroy(struct ion_heap *heap)
{
	struct ion_carveout_heap *carveout_heap =
	     container_of(heap, struct  ion_carveout_heap, heap);

	gen_pool_destroy(carveout_heap->pool);
	kfree(carveout_heap);
}
