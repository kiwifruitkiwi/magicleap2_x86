/*
 * Copyright (C) 2021 Advanced Micro Devices, Inc. All rights reserved.
 */
#ifndef __CVIP_S_INTF_H
#define __CVIP_S_INTF_H

#define MAP_WC_TYPE     1
#define MAP_DEVICE_TYPE 2

#define S_INTF_DATA0_TYPE MAP_WC_TYPE
#define S_INTF_IPC_TYPE   MAP_DEVICE_TYPE
#define S_INTF_DATA1_TYPE MAP_WC_TYPE
#define S_INTF_GSM_TYPE   MAP_DEVICE_TYPE
#define S_INTF_DATA2_TYPE MAP_WC_TYPE

#define DATA0_SIZE	0x400000
#define IPC_SIZE	0x001000
#define DATA1_SIZE	0x0FF000
#define GSM_SIZE	0x100000
#define DATA2_SIZE	0xA00000

#define DATA0_OFFSET	0x000000
#define IPC_OFFSET	(DATA0_OFFSET + DATA0_SIZE)
#define DATA1_OFFSET	(IPC_OFFSET + IPC_SIZE)
#define GSM_OFFSET	(DATA1_OFFSET + DATA1_SIZE)
#define DATA2_OFFSET	(GSM_OFFSET + GSM_SIZE)

enum {
	SINTF_DATA0,
	SINTF_IPC,
	SINTF_DATA1,
	SINTF_GSM,
	SINTF_DATA2,
	SINTF_MAX,
};

struct cvip_s_intf_region {
	u32 id;
	u32 type;
	u32 offset;
	u32 size;
	void __iomem *base;
};

int cvip_s_intf_ioremap(struct pci_dev *pci_dev);
void *get_sintf_base(int id);
struct cvip_s_intf_region *get_sintf_region(int id);

#endif
