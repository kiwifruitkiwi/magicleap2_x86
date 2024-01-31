/* SPDX-License-Identifier: GPL-2.0-only WITH Linux-syscall-note */
/*
 * Platform driver for the CVIP SOC.
 *
 * Copyright (C) 2022 Magic Leap, Inc. All rights reserved.
 */
#pragma once

#include <linux/ioctl.h>

#define CVIP_PLT_PCI_MAX_DEVICES	(9)

enum device_id {
	CVIP_PLAT_PCI_ROOT_BRIDGE,
	CVIP_PLAT_PCI_PRIMARY_UPSTREAM,
	CVIP_PLAT_PCI_PRIMARY_DW_INT,
	CVIP_PLAT_PCI_PRIMARY_DW_EXT,
	CVIP_PLAT_PCI_PRIMARY_ROOT,
	CVIP_PLAT_PCI_SECONDARY_UPSTREAM,
	CVIP_PLAT_PCI_SECONDARY_DW_INT,
	CVIP_PLAT_PCI_SECONDARY_ROOT,
	CVIP_PLAT_PCI_LAST_INDEX
};

struct pci_cap_info {
	uint32_t mask;
	uint32_t value;
	uint8_t field;
};

struct cap_info {
	struct pci_cap_info info;
	uint8_t data_width;
	uint32_t field_val;
	uint8_t dev_index;
};

struct pci_dev_info {
	uint32_t extended_cap_0;
	uint32_t extended_cap_1;
	uint16_t device_id;
	uint16_t vendor_id;
	uint16_t pci_domain;
	uint8_t pci_bus;
	uint8_t pci_dev;
	uint8_t pci_func;
	uint8_t link_speed;
	uint8_t link_enabled;
	uint8_t dl_active;
	uint8_t pci_revision_id;
	uint8_t found;
};

struct scan_info {
	struct pci_dev_info devices[CVIP_PLT_PCI_MAX_DEVICES];
};

struct cvip_plat_pci {
	uint8_t result;
	struct scan_info scan;
	struct cap_info cap;
};

struct cvip_plat_ioctl {
	struct cvip_plat_pci pci;
};

#define CVIP_PLAT_IOC_MAGIC		('F')
#define CVIP_PLAT_PCI_SCAN_REQUEST (_IOWR(CVIP_PLAT_IOC_MAGIC, 0, struct cvip_plat_ioctl))
#define CVIP_PLAT_PCI_CAP_GET	  (_IOWR(CVIP_PLAT_IOC_MAGIC, 1, struct cvip_plat_ioctl))
#define CVIP_PLAT_PCI_CAP_SET	  (_IOWR(CVIP_PLAT_IOC_MAGIC, 2, struct cvip_plat_ioctl))
