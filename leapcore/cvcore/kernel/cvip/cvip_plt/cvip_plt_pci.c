// SPDX-License-Identifier: GPL-2.0-only
/*
 * Platform driver for the CVIP SOC.
 *
 * Copyright (C) 2022 Magic Leap, Inc. All rights reserved.
 */

#include <linux/types.h>
#include <linux/io.h>
#include <linux/sort.h>
#include <linux/pci.h>
#include <uapi/linux/cvip/cvip_plat_ioc.h>

#define PCIE_NWL_VID               (0x19aa)
#define PCIE_ML_VID                (0x1e65)
#define PCIE_AMD_VID               (0X1022)
#define PCIE_ROOT_BRIDGE_DID       (0X1647)
#define PCIE_US_DID                (0x5000)
#define PCIE_DSI_DID               (0x5001)
#define PCIE_DSE_DID               (0x5002)
#define PCIE_EP_MLH_DEFAULT_DID    (0xb0ba)
#define PCIE_EP_MLH_BOOTLOADER_DID (0xb0bc)
#define PCIE_EP_MLH_MAIN_FW_DID    (0xb0bb)

static uint32_t cvip_plt_pci_decode_attribute(uint32_t mask, uint32_t value)
{
	uint32_t count = 32;

	// the mask indicates the start of the field's position
	// we need to shift that amount so it lines up on bit 0 for easy use
	uint32_t target = mask;

	target &= ~target + 1;

	if (target)
		count--;

	if (target & 0x0000FFFF)
		count -= 16;

	if (target & 0x00FF00FF)
		count -= 8;

	if (target & 0x0F0F0F0F)
		count -= 4;

	if (target & 0x33333333)
		count -= 2;

	if (target & 0x55555555)
		count -= 1;

	return (mask & value) >> count;
}

static uint32_t cvip_plt_pci_encode_attribute(uint32_t mask, uint32_t value)
{
	uint32_t count = 32;

	// the mask indicates the start of the field's position
	// we need to shift that amount so it lines up on bit 0 for easy use
	uint32_t target = mask;

	target &= ~target + 1;

	if (target)
		count--;

	if (target & 0x0000FFFF)
		count -= 16;

	if (target & 0x00FF00FF)
		count -= 8;

	if (target & 0x0F0F0F0F)
		count -= 4;

	if (target & 0x33333333)
		count -= 2;

	if (target & 0x55555555)
		count -= 1;

	return (value << count) & mask;
}

static int cvip_plt_pci_sort_compare(const void *c0r, const void *c1r)
{
	struct pci_dev *pci0 = *((struct pci_dev **) c0r);
	struct pci_dev *pci1 = *((struct pci_dev **) c1r);

	// note - we are safe from having null pointers here because
	// index (see where this function is used by sort) is dynamically
	// calculated and only non-null devices will appear in the list

	// check bus
	if (pci0->bus->number > pci1->bus->number)
		return 1;

	if (pci0->bus->number < pci1->bus->number)
		return -1;

	// then device number
	if (PCI_SLOT(pci0->devfn) > PCI_SLOT(pci1->devfn))
		return 1;

	if (PCI_SLOT(pci0->devfn) < PCI_SLOT(pci1->devfn))
		return -1;

	// and function
	if (PCI_FUNC(pci0->devfn) > PCI_FUNC(pci1->devfn))
		return 1;

	if (PCI_FUNC(pci0->devfn) < PCI_FUNC(pci1->devfn))
		return -1;

	pr_err("two devices have identical pci bus-dev-func assignments!");

	return 0;
}

static void cvip_plt_pci_release_all(struct pci_dev **devices)
{
	int i;

	for (i = 0; i < CVIP_PLT_PCI_MAX_DEVICES; i++) {
		if (devices[i])
			pci_dev_put(devices[i]);
	}
}

static int cvip_plt_pci_scan_worker(struct pci_dev **devices)
{
	int i;
	int index = 1;
	struct pci_dev *dev;

	//
	// pci_get_device(...) will auto-increment refcount on each successful
	// return on top of that, if the search is continued from a previous dev
	// point, THAT dev pointer will be auto-decremented.
	//
	// to keep to somewhat sane behavior, we artificially increment the
	// counter (via pci_dev_get) since the pointer is still technically
	// active/referenced during this scan/cap behavior. once we finish up
	// with whatever it was we were doing, either just a scan or
	// cap-activity, we 'release' all the acquired devices.

	// look for root bridge on its own
	dev = pci_get_device(0x1022, 0x1647, NULL);
	devices[0] = dev;
	pci_dev_get(dev);

	// up/dw endpoints have quirks, just look for 'em all
	for (i = index; i < CVIP_PLT_PCI_MAX_DEVICES; i++, index++) {
		dev = pci_get_device(0x19aa, PCI_ANY_ID, dev);
		devices[index] = dev;
		if (dev) {
			if (i+1 != CVIP_PLT_PCI_MAX_DEVICES)
				pci_dev_get(dev);
			continue;
		}
		break;
	}

	// then finally, look for root nodes (again, with quirks)
	// reset back to the start as well
	dev = NULL;
	for (i = index; i < CVIP_PLT_PCI_MAX_DEVICES; i++, index++) {
		dev = pci_get_device(0x1e65, PCI_ANY_ID, dev);
		devices[index] = dev;
		if (dev) {
			if (i+1 != CVIP_PLT_PCI_MAX_DEVICES)
				pci_dev_get(dev);
			continue;
		}
		break;
	}

	// after the scan is complete, we sort everything;
	// this is necessary because the scan doesn't return
	// things in the order we expect
	sort(devices, index, sizeof(struct pci_dev *),
		&cvip_plt_pci_sort_compare, NULL);

	return index;
}

static int cvip_plt_pci_get_capability(struct pci_dev *dev,
	struct cap_info *cap)
{
	int result;
	uint16_t tmp16;

	if (dev == NULL) {
		pr_err("dev is null");
		return 1;
	}

	if (cap == NULL) {
		pr_err("cap is null");
		return 1;
	}

	switch (cap->info.field) {
	case PCI_EXP_LNKCAP:
		cap->data_width = 32;
		break;
	case PCI_EXP_LNKCTL:
	case PCI_EXP_LNKSTA:
	case PCI_EXP_LNKCTL2:
		cap->data_width = 16;
		break;
	default:
		pr_err("unknown read-field requested [0x%0X]",
				cap->info.field);
		return 1;
	}

	// assume were only dealing with express capabilities

	switch (cap->data_width) {
	case 8:
		result = pcie_capability_read_word(dev, cap->info.field,
			&tmp16);
		cap->field_val = (uint8_t) tmp16;
		break;
	case 16:
		result = pcie_capability_read_word(dev, cap->info.field,
			&tmp16);
		cap->field_val = tmp16;
		break;
	case 32:
		result = pcie_capability_read_dword(dev, cap->info.field,
					&(cap->field_val));
		break;
	default:
		pr_err("internal error, unknown data-width setting [%d]",
				cap->data_width);
		return 1;
	}

	if (result)
		pr_err("failed to read capability [0x%X]\n", cap->info.field);

	// user expects value to be set instead of the internal field, so decode
	// and set appropriately doesn't matter if the read itself failed
	cap->info.value = cvip_plt_pci_decode_attribute(cap->info.mask,
						cap->field_val);

	return result;
}

static int cvip_plt_pci_set_capability(struct pci_dev *dev,
	struct cap_info *cap)
{
	struct cap_info current_cap;
	uint16_t tmp16;
	int result = 0;

	if (dev == NULL) {
		pr_err("dev ptr is null\n");
		return 1;
	}

	memcpy(&current_cap, cap, sizeof(struct cap_info));

	if (cvip_plt_pci_get_capability(dev, &current_cap)) {
		pr_err("failed to get-cap for set-cap\n");
		return 1;
	}

	// clear the existing setting
	current_cap.field_val = (current_cap.field_val &
		(~current_cap.info.mask));

	// apply what the user wants
	current_cap.field_val |=
		cvip_plt_pci_encode_attribute(current_cap.info.mask,
			cap->info.value);

	switch (current_cap.data_width) {
	case 8:
	case 16:
		tmp16 = (uint16_t) current_cap.field_val;
		result = pcie_capability_write_word(dev, cap->info.field,
					tmp16);
		break;
	case 32:
		result = pcie_capability_write_dword(dev, cap->info.field,
					current_cap.field_val);
		break;
	default:
		pr_err("internal error, unknown data-width setting [%d]\n",
			cap->data_width);
		return 1;
	}

	if (result) {
		pr_err("failed to write id [0x%0X] with value [0x%0X]",
			cap->info.field, current_cap.field_val);
		return 1;
	}

	return 0;
}

static int cvip_plt_pci_filter(struct pci_dev **devices, int count,
	struct cvip_plat_pci *data)
{
	uint8_t i, index;
	uint8_t ports = 1;
	uint8_t err_occurred = 0;
	int cap_pos;

	struct cap_info cap_info;

	for (i = 0, index = 0; i < count; i++) {

		int add_dev = -1;
		uint8_t get_link_enabled = 0;
		uint8_t get_dl_active = 0;
		uint8_t get_extended = 0;
		struct pci_dev *dev = devices[i];

		if (dev == NULL)
			continue;

		switch (dev->vendor) {
		case PCIE_AMD_VID:
			switch (dev->device) {
			case PCIE_ROOT_BRIDGE_DID:
				add_dev = 1;
				get_link_enabled = 1;
				get_dl_active = 1;
				break;
			default:
				break;
			}
			break;
		case PCIE_ML_VID:
			switch (dev->device) {
			case PCIE_EP_MLH_DEFAULT_DID:
				add_dev = 1;
				break;
			case PCIE_EP_MLH_BOOTLOADER_DID:
			case PCIE_EP_MLH_MAIN_FW_DID:
				add_dev = 1;
				get_extended = 1;
				break;
			default:
				pr_err("Unexpected enumeration found [%04X]",
					dev->device);
				err_occurred = 1;
				break;
			}
			break;
		case PCIE_NWL_VID:
			switch (dev->device) {
			case PCIE_US_DID:
			case PCIE_DSI_DID:
			case PCIE_DSE_DID:
				switch (ports) {
				case 1:
					add_dev = 1;
					break;
				case 2:
					add_dev = 1;
					get_dl_active = 1;
					break;
				case 3:
					add_dev = 1;
					get_dl_active = 1;
					break;
				case 4:
					add_dev = 1;
					break;
				case 5:
					add_dev = 1;
					get_dl_active = 1;
					break;
				case 6:
					break;
				default:
					pr_err("Found unexpected int/ext port [%d]...",
						ports);
					err_occurred = 1;
					break;
				}
				ports += 1;
				break;
			default:
				pr_err("Unexpected int/ext port found [%04X]",
					dev->device);
				err_occurred = 1;
				break;
			}
			break;
		default:
			break;
		}

		if (add_dev == 1) {
			memset(&cap_info, 0, sizeof(cap_info));

			data->scan.devices[index].found = 1;
			data->scan.devices[index].vendor_id	= dev->vendor;
			data->scan.devices[index].device_id = dev->device;
			data->scan.devices[index].pci_domain =
				dev->bus->domain_nr;
			data->scan.devices[index].pci_bus =
				dev->bus->number;
			data->scan.devices[index].pci_dev =
				PCI_SLOT(dev->devfn);
			data->scan.devices[index].pci_func =
				PCI_FUNC(dev->devfn);
			data->scan.devices[index].pci_revision_id =
				dev->revision;

			if (get_link_enabled) {
				cap_info.info.field = PCI_EXP_LNKCTL;
				cap_info.info.mask  = PCI_EXP_LNKCTL_LD;
				if (cvip_plt_pci_get_capability(dev, &cap_info)
					) {
					pr_err("failed to get enable/disable flag\n");
					cap_info.info.value = 0;
					err_occurred = 1;
				}
				// 1 or 0, but it's backwards (1 is disabled)
				data->scan.devices[index].link_enabled =
					!cap_info.info.value;
				memset(&cap_info, 0, sizeof(cap_info));
			}

			// get dl_active if supported
			if (get_dl_active) {
				cap_info.info.field = PCI_EXP_LNKCAP;
				cap_info.info.mask  = PCI_EXP_LNKCAP_DLLLARC;
				if (
				cvip_plt_pci_get_capability(dev, &cap_info)
				) {
					pr_err("failed to get dl-active flag\n");
					cap_info.info.value = 0;
					err_occurred = 1;
				}
				// 1 or 0
				data->scan.devices[index].dl_active =
					cap_info.info.value;
				memset(&cap_info, 0, sizeof(cap_info));
			}

			// get extended dsn/id
			if (get_extended) {
				cap_pos = pci_find_ext_capability(dev,
					PCI_EXT_CAP_ID_DSN);
				if (cap_pos) {
					cap_pos += 4;
					pci_read_config_dword(dev, cap_pos,
						&(data->scan.devices[index].extended_cap_0));
					pci_read_config_dword(dev,
						cap_pos + 4,
						&(data->scan.devices[index].extended_cap_1));
				} else {
					pr_err("failed to read dsn\n");
					err_occurred = 1;
				}
			}

			// get the reported link speed
			cap_info.info.field = PCI_EXP_LNKSTA;
			cap_info.info.mask  = PCI_EXP_LNKSTA_CLS;
			if (cvip_plt_pci_get_capability(dev, &cap_info)) {
				pr_err("failed to get link speed\n");
				cap_info.info.value = 0x07;
				err_occurred = 1;
			}
			data->scan.devices[index].link_speed = cap_info.info.value;

			index += 1;
		}
	}

	return err_occurred;
}

static int cvip_plt_pci_always_scan(struct cvip_plat_pci *data, struct pci_dev **devices)
{
	int count;

	memset(data, 0, sizeof(struct cvip_plat_pci));

	// scan for known devices - note this is a raw list we get back
	count = cvip_plt_pci_scan_worker(devices);

	return cvip_plt_pci_filter(devices, count, data);
}

int cvip_plt_pci_scan(struct cvip_plat_pci *data)
{
	int result;
	struct pci_dev *devices[CVIP_PLT_PCI_MAX_DEVICES];

	memset(devices, 0, sizeof(devices));

	result = cvip_plt_pci_always_scan(data, devices);

	data->result = result;

	cvip_plt_pci_release_all(devices);

	return result;
}

int cvip_plt_pci_cap_set(struct cvip_plat_pci *data)
{
	int i, result;
	struct pci_dev_info *info;
	struct cvip_plat_pci scan;
	struct pci_dev *devices[CVIP_PLT_PCI_MAX_DEVICES];

	memset(devices, 0, sizeof(devices));

	result = cvip_plt_pci_always_scan(&scan, devices);

	if (result) {
		pr_err("scan failed for cap-set\n");
		data->result = result;
		cvip_plt_pci_release_all(devices);
		return result;
	}

	if (data->cap.dev_index >= CVIP_PLAT_PCI_LAST_INDEX) {
		pr_err("requested cap-set on dev index [%d] is out of range\n",
			data->cap.dev_index);
		data->result = 1;
		cvip_plt_pci_release_all(devices);
		return result;
	}

	info = &(scan.scan.devices[data->cap.dev_index]);

	if (!info->found) {
		pr_err("device [%d] not found for set-cap request\n", data->cap.dev_index);
		data->result = 1;
		cvip_plt_pci_release_all(devices);
		return 1;
	}

	for (i = 0; i < CVIP_PLT_PCI_MAX_DEVICES; i++) {
		if (devices[i]) {
			if (info->pci_bus == devices[i]->bus->number) {
				if (info->pci_dev == PCI_SLOT(devices[i]->devfn)) {
					if (info->pci_func == PCI_FUNC(devices[i]->devfn)) {
						data->result =
							cvip_plt_pci_set_capability(
								devices[i], &(data->cap));
					}
				}
			}
		}
	}

	cvip_plt_pci_release_all(devices);

	return data->result;
}

int cvip_plt_pci_cap_get(struct cvip_plat_pci *data)
{
	int i, result;
	struct pci_dev_info *info;
	struct cvip_plat_pci scan;
	struct pci_dev *devices[CVIP_PLT_PCI_MAX_DEVICES];

	memset(devices, 0, sizeof(devices));

	result = cvip_plt_pci_always_scan(&scan, devices);

	if (result) {
		pr_err("scan failed for cap-get\n");
		data->result = result;
		cvip_plt_pci_release_all(devices);
		return result;
	}

	if (data->cap.dev_index >= CVIP_PLAT_PCI_LAST_INDEX) {
		pr_err("requested cap-get on dev index [%d] is out of range\n",
			data->cap.dev_index);
		data->result = 1;
		cvip_plt_pci_release_all(devices);
		return result;
	}

	info = &(scan.scan.devices[data->cap.dev_index]);

	if (!info->found) {
		pr_err("device [%d] not found for set-cap request\n",
			data->cap.dev_index);
		data->result = 1;
		cvip_plt_pci_release_all(devices);
		return 1;
	}

	for (i = 0; i < CVIP_PLT_PCI_MAX_DEVICES; i++) {
		if (devices[i]) {
			if (info->pci_bus == devices[i]->bus->number) {
				if (info->pci_dev == PCI_SLOT(devices[i]->devfn)) {
					if (info->pci_func == PCI_FUNC(devices[i]->devfn)) {
						data->result =
							cvip_plt_pci_get_capability(
								devices[i], &(data->cap));
					}
				}
			}
		}
	}

	cvip_plt_pci_release_all(devices);

	return data->result;
}
