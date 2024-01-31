/*
 * Copyright (C) 2020 Advanced Micro Devices, Inc. All rights reserved.
 *
 */

#include <linux/of.h>
#include <linux/io.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/cma.h>

#ifdef pr_fmt
#undef pr_fmt
#endif

#define pr_fmt(fmt) "recovery_dump: " fmt

static struct page *pages;

static const struct of_device_id rd_device_id[] = {
	{ .compatible = "amd,mero-recovery-dump", },
	{},
};

static int recovery_dump_cma(struct cma *cma, void *data)
{
	unsigned int region_size = *(unsigned int *)data;
	size_t nr_pages;
	unsigned int align;

	align = get_order(region_size);
	if (align > CONFIG_CMA_ALIGNMENT)
		align = CONFIG_CMA_ALIGNMENT;

	nr_pages = region_size >> PAGE_SHIFT;
	if (nr_pages == 0)
		nr_pages = 1;

	pages = cma_alloc(cma, nr_pages, align, false);
	if (!pages) {
		pr_err("Failed to alloc %ld pages\n", nr_pages);
		return -ENOMEM;
	}

	pr_info("Allocated %ld pages at 0x%llx\n", nr_pages,
		page_to_phys(pages));

	return 0;
}

static int __init recovery_dump_init(void)
{
	struct device_node *np;
	void __iomem *base;
	unsigned int region_size;
	u64 reg[2];
	int ret;

	np = of_find_matching_node(NULL, rd_device_id);
	if (!np || !of_device_is_available(np)) {
		pr_err("Not found valid device node\n");
		return -EINVAL;
	}

	ret = of_property_read_variable_u64_array(np, "reg", reg,
						  ARRAY_SIZE(reg),
						  ARRAY_SIZE(reg));
	if (ret < 0) {
		pr_err("Not found reg property\n");
		return -EINVAL;
	}

	if (of_property_read_u32(np, "region-size", &region_size)) {
		pr_err("Not found region-size property\n");
		return -EINVAL;
	}

	base = ioremap(reg[0], reg[1]);
	if (!base) {
		pr_err("cannot map sram region\n");
		return -ENOMEM;
	}

	cma_for_each_area(recovery_dump_cma, &region_size);

	if (pages) {
		writeq(page_to_phys(pages), base);
		writeq(region_size, base + 0x8);
	} else {
		iounmap(base);
		return -EPROBE_DEFER;
	}

	pr_info("Reserved %d MiB at 0x%llx for recovery dumping\n",
		region_size / SZ_1M, page_to_phys(pages));

	return 0;
}

core_initcall(recovery_dump_init);

