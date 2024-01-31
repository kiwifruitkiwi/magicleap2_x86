/*
 * Copyright (C) 2021 Advanced Micro Devices, Inc. All rights reserved.
 */

#include <linux/types.h>
#include <linux/err.h>
#include <linux/export.h>
#include <linux/io.h>
#include <linux/device.h>
#include <linux/pci.h>
#include "cvip_s_intf.h"

#define CVIP_S_INTF_BAR	2

static struct cvip_s_intf_region cvip_s_region[SINTF_MAX] = {
	{ SINTF_DATA0, S_INTF_DATA0_TYPE, DATA0_OFFSET, DATA0_SIZE, 0 },
	{ SINTF_IPC,   S_INTF_IPC_TYPE,   IPC_OFFSET,   IPC_SIZE,   0 },
	{ SINTF_DATA1, S_INTF_DATA1_TYPE, DATA1_OFFSET, DATA1_SIZE, 0 },
	{ SINTF_GSM,   S_INTF_GSM_TYPE,   GSM_OFFSET,   GSM_SIZE,   0 },
	{ SINTF_DATA2, S_INTF_DATA2_TYPE, DATA2_OFFSET, DATA2_SIZE, 0 },
};

int cvip_s_intf_ioremap(struct pci_dev *pci_dev)
{
	resource_size_t start;
	resource_size_t len;
	u32 total_mapsize = 0;
	int i;
	void __iomem *base;

	start = pci_resource_start(pci_dev, CVIP_S_INTF_BAR);
	len = pci_resource_len(pci_dev, CVIP_S_INTF_BAR);

	if (!start || !len) {
		dev_err(&pci_dev->dev, "Failed to obtain S-INTF BAR info\n");
		return -EINVAL;
	}

	for (i = 0; i < ARRAY_SIZE(cvip_s_region); i++) {
		if (cvip_s_region[i].type == MAP_WC_TYPE)
			base = ioremap_wc(cvip_s_region[i].offset + start,
					  cvip_s_region[i].size);
		else
			base = ioremap(cvip_s_region[i].offset + start,
				       cvip_s_region[i].size);

		if (!base) {
			dev_err(&pci_dev->dev, "failed ioremap region[%d]\n", i);
			return -ENOMEM;
		}

		cvip_s_region[i].base = base;
		total_mapsize += cvip_s_region[i].size;
		if (total_mapsize > len) {
			dev_err(&pci_dev->dev, "exceed total BAR size\n");
			return -ENOMEM;
		}
	}

	return 0;
}
EXPORT_SYMBOL(cvip_s_intf_ioremap);

void *get_sintf_base(int id)
{
	if (id >= SINTF_MAX)
		return NULL;

	return cvip_s_region[id].base;
}
EXPORT_SYMBOL(get_sintf_base);

struct cvip_s_intf_region *get_sintf_region(int id)
{
	if (id >= SINTF_MAX)
		return NULL;

	return &cvip_s_region[id];
}
EXPORT_SYMBOL(get_sintf_region);
