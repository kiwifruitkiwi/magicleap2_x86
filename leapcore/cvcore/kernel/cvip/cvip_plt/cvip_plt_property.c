// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Read only API for drivers to access persistent properties.
 *
 * Copyright (C) 2022 Magic Leap, Inc. All rights reserved.
 */
#include <stddef.h>
#include <linux/string.h>
#include <linux/memremap.h>
#include <linux/io.h>
#include <linux/cvip/cvip_plt/cvip_plt.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include "cvip_plt_crc_opto.h"

#define PROPERTY_PERSIST_MAX_ENTRIES           (100)

#define PROPERTY_KEY_SIZE                      (256)
#define PROPERTY_VAL_SIZE                      (256)

#define PERSIST_PROPERTY_OFFSET_A              (0)
#define PERSIST_PROPERTY_OFFSET_B              (PERSIST_PROPERTY_TABLE_NVME_BYTES)

#define PERSIST_PROPERTY_TABLE_NVME_BYTES      (512 * 1024)
#define PERSIST_PROPERTY_TOTAL_NVME_BYTES      (PERSIST_PROPERTY_TABLE_NVME_BYTES * 2)

struct cvip_property_entry {
	uint8_t key_type; // Using uint8_t instead of the enum to force one byte
	char key[PROPERTY_KEY_SIZE + 1];
	char val[PROPERTY_VAL_SIZE + 1];
};

struct prop_persist_table {
	uint32_t persist_version;
	uint32_t sequence_number;
	struct cvip_property_entry persist_table[PROPERTY_PERSIST_MAX_ENTRIES];
	uint16_t rsvd;  // pad our crc coverage to a multiple of uint32_t
	uint16_t crc16;
};

enum property_keytypes {
	PROPERTY_KEYTYPE_UNUSED = 0,
	PROPERTY_KEYTYPE_STRING = 1
};

#define PERSIST_PROPERTY_TABLE_CHECK_COVERAGE (sizeof(struct prop_persist_table) - (2 * sizeof(uint16_t)))

// We only parse the address and size from the fm3 once, at init time.
// We need to store them so we can service sysfs requests.
static phys_addr_t property_addr;
static size_t property_size;

static uint16_t cvip_plt_property_persist_prop_calc_crc16(void *start,
	size_t count)
{
	cvip_plt_crc16_t crc;

	crc = cvip_plt_crc16_init();

	crc = cvip_plt_crc16_update(crc, (unsigned char *)start, count);

	crc = cvip_plt_crc16_finalize(crc);

	return crc;
}

phys_addr_t cvip_plt_property_get_addr(void)
{
	return property_addr;
}

size_t cvip_plt_property_get_size(void)
{
	return property_size;
}

int cvip_plt_property_get_from_fmr(void)
{
	struct device_node *prop_node;
	struct resource prop_res;
	int err;

	// Our persistent properties are stored on fmr3a.  Look up
	// the location and size in the device tree.
	prop_node = of_find_node_by_name(NULL, "fmr3a");
	if (prop_node == NULL) {
		pr_err("cvip_plt: find_node_by_name for fmr3a returned NULL\n");
		return -ENOMEM;
	}

	err = of_address_to_resource(prop_node, 0, &prop_res);
	if (err) {
		pr_err("cvip_plt: can't get resource info for region fmr3a\b");
		return -ENOMEM;
	}

	property_size = prop_res.end - prop_res.start + 1;

	if (property_size != PERSIST_PROPERTY_TOTAL_NVME_BYTES) {
		pr_err("cvip_plt: fmr3a reports size %lu, expecting size %u!\n",
			property_size, PERSIST_PROPERTY_TOTAL_NVME_BYTES);
		return -ENOMEM;
	}

	property_addr = prop_res.start;

	return 0;
}

static int cvip_plt_property_get_internal(const char *key, char *val,
	size_t buf_size)
{
	struct prop_persist_table *blob_a;
	struct prop_persist_table *blob_b;
	struct prop_persist_table *selected_blob = NULL;
	void *prop_base;
	uint32_t seq_a;
	uint32_t seq_b;
	bool blob_a_valid = false;
	bool blob_b_valid = false;
	cvip_plt_crc16_t crc_a = 0;
	cvip_plt_crc16_t crc_b = 0;
	cvip_plt_crc16_t selected_crc = 0;
	uint32_t i;
	int retval = 0;

	prop_base = (struct prop_persist_table *)memremap(
		property_addr, property_size, MEMREMAP_WB);
	if (prop_base == NULL) {
		pr_err("cvip_plt_prop: memremap failed (%llx, %lu)\n",
			property_addr, property_size);
		// We weren't able to map the space that fmr3a claims.
		// Return 0, which means the same as not finding the
		// key.
		return 0;
	}

	// We have two copies of the property data in memory.  Let's set up some
	// variables to make it easy to look at them.
	blob_a_valid = false;
	blob_a = (struct prop_persist_table *)(prop_base +
		PERSIST_PROPERTY_OFFSET_A);

	blob_b_valid = false;
	blob_b = (struct prop_persist_table *)(prop_base +
		PERSIST_PROPERTY_OFFSET_B);

	seq_a = blob_a->sequence_number;
	seq_b = blob_b->sequence_number;

	// Check if the CRC is correct for each blob.  If it's not,
	// we can't use it.
	crc_a = cvip_plt_property_persist_prop_calc_crc16(blob_a,
		PERSIST_PROPERTY_TABLE_CHECK_COVERAGE);

	crc_b = cvip_plt_property_persist_prop_calc_crc16(blob_b,
		PERSIST_PROPERTY_TABLE_CHECK_COVERAGE);

	if (blob_a->crc16 == crc_a)
		blob_a_valid = true;

	if (blob_b->crc16 == crc_b)
		blob_b_valid = true;

	// This could be more compact, but doing the comparisons this
	// way makes the logic very clean and obvious.
	// Are both valid?  If so, we (mostly) use the higher
	// sequence number.
	if ((blob_a_valid == 1) && (blob_b_valid == 1)) {
		// A few special cases for the wrap-around scenario
		if ((seq_a == (uint32_t)(-1)) && (seq_b == 0)) {
			// In this case, sequence 0 should win, since it was
			// written after UINT32_MAX.
			selected_blob = blob_b;
		} else if ((seq_a == 0) && (seq_b == (uint32_t)(-1))) {
			// In this case, sequence 0 should win, since it was
			// written after UINT32_MAX.
			selected_blob = blob_a;
		} else if (seq_a >= seq_b) {
			selected_blob = blob_a;
		} else {
			selected_blob = blob_b;
		}
	}

	// Is only sector 0 valid?
	if ((blob_a_valid == 1) && (blob_b_valid == 0)) {
		selected_blob = blob_a;
		selected_crc = crc_a;
	}

	// Is only sector 1 valid?
	if ((blob_a_valid == 0) && (blob_b_valid == 1)) {
		selected_blob = blob_b;
		selected_crc = crc_b;
	}

	// Is neither valid?  Since the kernel is read only, we can just
	// error out here and return 0, meaning not found.
	if ((blob_a_valid == 0) && (blob_b_valid == 0)) {
		memunmap(prop_base);
		return 0;
	}

	for (i = 0; i < PROPERTY_PERSIST_MAX_ENTRIES; ++i) {
		if (PROPERTY_KEYTYPE_UNUSED !=
			selected_blob->persist_table[i].key_type) {
			if (strncmp(key, selected_blob->persist_table[i].key,
			 PROPERTY_KEY_SIZE) == 0) {
				strncpy(val,
					selected_blob->persist_table[i].val,
					buf_size - 1);

				val[buf_size - 1] = '\0';
				retval = strlen(
					selected_blob->persist_table[i].val);

				// Now that we found our key/value, let's
				// double check that the CRC didn't change.
				// If it did, we'll return a negative value,
				// telling the calling function that we need
				// to try again.
				if (cvip_plt_property_persist_prop_calc_crc16(
					selected_blob,
					PERSIST_PROPERTY_TABLE_CHECK_COVERAGE
					) != selected_crc) {
					retval = -1;
				}
				break;
			}
		}
	}

	memunmap(prop_base);
	return retval;
}


size_t cvip_plt_property_get_safe(const char *key, char *val,
	size_t buf_size, const char *default_val)
{
	size_t retval;
	const int max_retries = 2;
	int attempts = 0;

	do {
		// A negative value indicates we think the properties blob
		// changed while we were reading it.  Try one more time.
		retval = cvip_plt_property_get_internal(key, val, buf_size);
		attempts++;
	} while ((attempts < max_retries) && (retval < 0));

	if (retval == 0) {
		strncpy(val, default_val, buf_size - 1);
		val[buf_size - 1] = '\0';
		retval = strlen(default_val);
	}

	return retval;
}
EXPORT_SYMBOL(cvip_plt_property_get_safe);
