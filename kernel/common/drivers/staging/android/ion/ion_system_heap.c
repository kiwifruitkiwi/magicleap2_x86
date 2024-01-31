// SPDX-License-Identifier: GPL-2.0
/*
 * ION Memory Allocator system heap exporter
 *
 * Copyright (C) 2011 Google, Inc.
 */

#include <asm/page.h>
#include <linux/dma-mapping.h>
#include <linux/err.h>
#include <linux/highmem.h>
#include <linux/mm.h>
#include <linux/scatterlist.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>

#include "ion.h"

#define NUM_ORDERS ARRAY_SIZE(orders)

static gfp_t high_order_gfp_flags = (GFP_HIGHUSER | __GFP_ZERO | __GFP_NOWARN |
				     __GFP_NORETRY) & ~__GFP_RECLAIM;
static gfp_t low_order_gfp_flags  = GFP_HIGHUSER | __GFP_ZERO;
static const unsigned int orders[] = {8, 4, 0};

static int order_to_index(unsigned int order)
{
	int i;

	for (i = 0; i < NUM_ORDERS; i++)
		if (order == orders[i])
			return i;
	BUG();
	return -1;
}

static inline unsigned int order_to_size(int order)
{
	return PAGE_SIZE << order;
}

struct ion_system_heap {
	struct ion_heap heap;
	struct ion_page_pool *pools[NUM_ORDERS];
};

static struct page *alloc_buffer_page(struct ion_system_heap *heap,
				      struct ion_buffer *buffer,
				      unsigned long order)
{
	struct ion_page_pool *pool = heap->pools[order_to_index(order)];

	return ion_page_pool_alloc(pool);
}

static void free_buffer_page(struct ion_system_heap *heap,
			     struct ion_buffer *buffer, struct page *page)
{
	struct ion_page_pool *pool;
	unsigned int order = compound_order(page);

	/* go to system */
	if (buffer->private_flags & ION_PRIV_FLAG_SHRINKER_FREE) {
		__free_pages(page, order);
		return;
	}

	pool = heap->pools[order_to_index(order)];

	ion_page_pool_free(pool, page);
}

static struct page *alloc_largest_available(struct ion_system_heap *heap,
					    struct ion_buffer *buffer,
					    unsigned long size,
					    unsigned int max_order)
{
	struct page *page;
	int i;

	for (i = 0; i < NUM_ORDERS; i++) {
		if (size < order_to_size(orders[i]))
			continue;
		if (max_order < orders[i])
			continue;

		page = alloc_buffer_page(heap, buffer, orders[i]);
		if (!page)
			continue;

		return page;
	}

	return NULL;
}

static int ion_system_heap_allocate(struct ion_heap *heap,
				    struct ion_buffer *buffer,
				    unsigned long size,
				    unsigned long flags)
{
	struct ion_system_heap *sys_heap = container_of(heap,
							struct ion_system_heap,
							heap);
	struct sg_table *table;
	struct sg_table *table2;
	struct scatterlist *sg;
	struct scatterlist *sg2 = NULL;
	struct list_head pages;
	struct page *page, *tmp_page;
	int i = 0;
	unsigned long size_remaining = PAGE_ALIGN(size);
	unsigned int max_order = orders[0];
	u64 page_sft = 0;
	u64 pfn2;
	int b_flag_alt, b_flag_slc;

	/* check buffer allocation flags */
	b_flag_alt = flags & (ION_FLAG_CVIP_DRAM_ALT0 |
			      ION_FLAG_CVIP_DRAM_ALT1);
	b_flag_slc = is_ion_buffer_flag_syscache(flags);
	if (b_flag_alt && b_flag_slc) {
		pr_err("%s: Not supporting flags: SYSCACHE & ALT\n", __func__);
		return -EINVAL;
	}

	if (size / PAGE_SIZE > totalram_pages() / 2)
		return -ENOMEM;

	INIT_LIST_HEAD(&pages);
	while (size_remaining > 0) {
		page = alloc_largest_available(sys_heap, buffer, size_remaining,
					       max_order);
		if (!page)
			goto free_pages;
		list_add_tail(&page->lru, &pages);
		size_remaining -= page_size(page);
		max_order = compound_order(page);
		i++;
	}
	table = kmalloc(sizeof(*table), GFP_KERNEL);
	if (!table)
		goto free_pages;

	if (sg_alloc_table(table, i, GFP_KERNEL))
		goto free_table;

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
		if (!table2)
			goto free_sg_table;

		if (sg_alloc_table(table2, i, GFP_KERNEL))
			goto free_table2;

		sg2 = table2->sgl;
	}

	sg = table->sgl;
	list_for_each_entry_safe(page, tmp_page, &pages, lru) {
		sg_set_page(sg, page, page_size(page), 0);
		sg = sg_next(sg);

		if (page_sft) {
			pfn2 = page_to_pfn(page);
			pr_debug("ION heap %s allocates syscache @ 0x%llX (paddr=0x%llX), size = 0x%lx\n",
				 heap->name, PFN_PHYS(pfn2 + page_sft),
				 PFN_PHYS(pfn2), size);
			pfn2 += page_sft;
			sg_set_page(sg2, pfn_to_page(pfn2), page_size(page), 0);
			sg2 = sg_next(sg2);
		}

		list_del(&page->lru);
	}

	buffer->sg_table = table;
	if (page_sft)
		buffer->sg_extbl = table2;
	else
		buffer->sg_extbl = NULL;

	if (!is_heap_clear_enable(heap->flags))
		buffer->flags |= ION_BUFFER_FLAG_NO_MEMORY_CLEAR;

	return 0;

free_table2:
	kfree(table2);
free_sg_table:
	sg_free_table(table);
free_table:
	kfree(table);
free_pages:
	list_for_each_entry_safe(page, tmp_page, &pages, lru)
		free_buffer_page(sys_heap, buffer, page);
	return -ENOMEM;
}

static void ion_system_heap_free(struct ion_buffer *buffer)
{
	struct ion_system_heap *sys_heap = container_of(buffer->heap,
							struct ion_system_heap,
							heap);
	struct sg_table *table = buffer->sg_table;
	struct sg_table *table2 = buffer->sg_extbl;
	struct scatterlist *sg;
	int i;

	/* zero the buffer before goto page pool */
	if (!(buffer->private_flags & ION_PRIV_FLAG_SHRINKER_FREE) &&
	    is_buffer_clear_enable(buffer->flags))
		ion_heap_buffer_zero(buffer);

	for_each_sg(table->sgl, sg, table->nents, i)
		free_buffer_page(sys_heap, buffer, sg_page(sg));
	sg_free_table(table);
	kfree(table);

	if (table2) {
		sg_free_table(table2);
		kfree(table2);
	}
}

static int ion_system_heap_shrink(struct ion_heap *heap, gfp_t gfp_mask,
				  int nr_to_scan)
{
	struct ion_page_pool *pool;
	struct ion_system_heap *sys_heap;
	int nr_total = 0;
	int i, nr_freed;
	int only_scan = 0;

	sys_heap = container_of(heap, struct ion_system_heap, heap);

	if (!nr_to_scan)
		only_scan = 1;

	for (i = 0; i < NUM_ORDERS; i++) {
		pool = sys_heap->pools[i];

		if (only_scan) {
			nr_total += ion_page_pool_shrink(pool,
							 gfp_mask,
							 nr_to_scan);

		} else {
			nr_freed = ion_page_pool_shrink(pool,
							gfp_mask,
							nr_to_scan);
			nr_to_scan -= nr_freed;
			nr_total += nr_freed;
			if (nr_to_scan <= 0)
				break;
		}
	}
	return nr_total;
}

static struct ion_heap_ops system_heap_ops = {
	.allocate = ion_system_heap_allocate,
	.free = ion_system_heap_free,
	.map_kernel = ion_heap_map_kernel,
	.unmap_kernel = ion_heap_unmap_kernel,
	.map_user = ion_heap_map_user,
	.shrink = ion_system_heap_shrink,
};

static void ion_system_heap_destroy_pools(struct ion_page_pool **pools)
{
	int i;

	for (i = 0; i < NUM_ORDERS; i++)
		if (pools[i])
			ion_page_pool_destroy(pools[i]);
}

static int ion_system_heap_create_pools(struct ion_page_pool **pools)
{
	int i;

	for (i = 0; i < NUM_ORDERS; i++) {
		struct ion_page_pool *pool;
		gfp_t gfp_flags = low_order_gfp_flags;

		if (orders[i] > 4)
			gfp_flags = high_order_gfp_flags;

		pool = ion_page_pool_create(gfp_flags, orders[i]);
		if (!pool)
			goto err_create_pool;
		pools[i] = pool;
	}

	return 0;

err_create_pool:
	ion_system_heap_destroy_pools(pools);
	return -ENOMEM;
}

void ion_system_heap_destroy(struct ion_heap *heap)
{
	struct ion_system_heap *sys_heap = container_of(heap,
							struct ion_system_heap,
							heap);
	ion_system_heap_destroy_pools(sys_heap->pools);
	kfree(sys_heap->pools);
	kfree(sys_heap);
}

static struct ion_heap *__ion_system_heap_create(void)
{
	struct ion_system_heap *heap;

	heap = kzalloc(sizeof(*heap), GFP_KERNEL);
	if (!heap)
		return ERR_PTR(-ENOMEM);
	heap->heap.ops = &system_heap_ops;
	heap->heap.type = ION_HEAP_TYPE_SYSTEM;
	heap->heap.flags = 0;

	if (ion_system_heap_create_pools(heap->pools))
		goto free_heap;

	return &heap->heap;

free_heap:
	kfree(heap);
	return ERR_PTR(-ENOMEM);
}

struct ion_heap *ion_system_heap_create(struct ion_platform_heap *heap_data)
{
	struct ion_heap *heap;

	heap = __ion_system_heap_create();
	if (IS_ERR(heap))
		return ERR_PTR(-ENOMEM);  //PTR_ERR(heap);

	heap->name = "ion_system_heap";

	if (!is_heap_clear_enable(heap_data->heap_flags))
		heap->flags |= ION_HEAP_FLAG_INIT_NO_MEMORY_CLEAR;

	if (is_heap_has_syscache(heap_data->heap_flags))
		heap->flags |= ION_HEAP_FLAG_HAS_SYSCACHE;
	else if (is_heap_has_syscache_shift(heap_data->heap_flags))
		heap->flags |= ION_HEAP_FLAG_HAS_SYSCACHE_SHIFT;

	return heap;
}

static int ion_system_contig_heap_allocate(struct ion_heap *heap,
					   struct ion_buffer *buffer,
					   unsigned long len,
					   unsigned long flags)
{
	int order = get_order(len);
	struct page *page;
	struct sg_table *table;
	unsigned long i;
	int ret;

	page = alloc_pages(low_order_gfp_flags | __GFP_NOWARN, order);
	if (!page)
		return -ENOMEM;

	split_page(page, order);

	len = PAGE_ALIGN(len);
	for (i = len >> PAGE_SHIFT; i < (1 << order); i++)
		__free_page(page + i);

	table = kmalloc(sizeof(*table), GFP_KERNEL);
	if (!table) {
		ret = -ENOMEM;
		goto free_pages;
	}

	ret = sg_alloc_table(table, 1, GFP_KERNEL);
	if (ret)
		goto free_table;

	sg_set_page(table->sgl, page, len, 0);

	buffer->sg_table = table;

	return 0;

free_table:
	kfree(table);
free_pages:
	for (i = 0; i < len >> PAGE_SHIFT; i++)
		__free_page(page + i);

	return ret;
}

static void ion_system_contig_heap_free(struct ion_buffer *buffer)
{
	struct sg_table *table = buffer->sg_table;
	struct page *page = sg_page(table->sgl);
	unsigned long pages = PAGE_ALIGN(buffer->size) >> PAGE_SHIFT;
	unsigned long i;

	for (i = 0; i < pages; i++)
		__free_page(page + i);
	sg_free_table(table);
	kfree(table);
}

static struct ion_heap_ops kmalloc_ops = {
	.allocate = ion_system_contig_heap_allocate,
	.free = ion_system_contig_heap_free,
	.map_kernel = ion_heap_map_kernel,
	.unmap_kernel = ion_heap_unmap_kernel,
	.map_user = ion_heap_map_user,
};

static struct ion_heap *__ion_system_contig_heap_create(void)
{
	struct ion_heap *heap;

	heap = kzalloc(sizeof(*heap), GFP_KERNEL);
	if (!heap)
		return ERR_PTR(-ENOMEM);
	heap->ops = &kmalloc_ops;
	heap->type = ION_HEAP_TYPE_SYSTEM_CONTIG;
	heap->name = "ion_system_contig_heap";

	return heap;
}

struct ion_heap *ion_system_contig_heap_create(void)
{
	struct ion_heap *heap;

	heap = __ion_system_contig_heap_create();
	if (IS_ERR(heap))
		return ERR_PTR(-ENOMEM);

	ion_device_add_heap(heap);

	return heap;
}

#if defined(CONFIG_X86_AMD_PLATFORM_DEVICE)
int ion_add_system_heaps(void)
{
	struct ion_heap *heap;

	heap = __ion_system_heap_create();
	if (IS_ERR(heap))
		return PTR_ERR(heap);

	heap->name = "ion_system_heap";
	heap->id = ION_HEAP_X86_SYSTEM_ID;

	ion_device_add_heap(heap);

	return 0;
}
device_initcall(ion_add_system_heaps);
#endif
