// SPDX-License-Identifier: GPL-2.0-or-later
/* Internal memory accounting for AMD GPUs
 *
 * Copyright (C) 2020 MagicLeap, All Rights Reserved.
 * Written by Leonid Moiseichuk (lmoiseichuk@magicleap.com)
 */

#include <linux/mm_types_task.h>
#include <linux/seq_file.h>
#include "internal.h"

/*
 * Counters to be represented in meminfo file
 */
static atomic_long_t global_meminfo_pages[AMDGFX_GEM_MM_COUNT];

void amdgfx_update_meminfo_pages(int counter, unsigned long pages, int inc_dec)
{
	if (counter >= 0 && counter < AMDGFX_GEM_MM_COUNT) {
		atomic_long_t *metric = &global_meminfo_pages[counter];

		if (inc_dec > 0)
			atomic_long_add(pages, metric);
		else {
			if (atomic_long_read(metric) <= pages)
				atomic_long_set(metric, 0);
			else
				atomic_long_sub(pages, metric);
		}
	}
}
EXPORT_SYMBOL(amdgfx_update_meminfo_pages);

static void show_val_kb(struct seq_file *m, const char *s, unsigned long num)
{
	seq_printf(m, "%s\t%8lu kB\n", s, num << (PAGE_SHIFT - 10));
}

void amdgfx_report_meminfo(struct seq_file *m)
{
	int gfx_idx;
	unsigned long gfx_total = 0;
	static const char *gfx_titles[AMDGFX_GEM_MM_COUNT + 1] = {
		[AMDGFX_GEM_MM_VRAM]  = "GfxVRamUsed:",
		[AMDGFX_GEM_MM_USER]  = "GfxUserUsed:",
		[AMDGFX_GEM_MM_SWAP]  = "GfxSwapUsed:",
		[AMDGFX_GEM_MM_COUNT] = "GfxUsed:"
	};

	for (gfx_idx = 0; gfx_idx < AMDGFX_GEM_MM_COUNT; gfx_idx++) {
		unsigned long value;

		value = atomic_long_read(&global_meminfo_pages[gfx_idx]);
		show_val_kb(m, gfx_titles[gfx_idx], value);
		gfx_total += value;
	}
	show_val_kb(m, gfx_titles[AMDGFX_GEM_MM_COUNT], gfx_total);
}
