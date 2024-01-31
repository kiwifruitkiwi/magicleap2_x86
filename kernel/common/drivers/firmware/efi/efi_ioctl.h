/*
 * EFI ioctrl driver for AMD Mero Subsystem
 *
 * Copyright (C) 2020 Advanced Micro Devices, Inc. All rights reserved.
 */

#ifndef __EFI_IOCTL_H__
#define __EFI_IOCTL_H__

#include <linux/ioctl.h>

struct EFI_ISH_METADATA {
	unsigned int priority;
	unsigned int update_retries;
	unsigned int glitch_retries;
};

struct EFI_ISH {
	struct EFI_ISH_METADATA ish_data[2];
};

struct EFI_CAPSULE_METADATA {
	/* 0: No update required; 1: update required */
	u8 UpdateFlag;
	/* 0: Slot A; 1: Slot B */
	u8 UpdateSlot;
};

#define IOC_MAGIC    'e'
/* get ISH variable */
#define IOCTL_GET_VARIABLE	_IOR(IOC_MAGIC, 1, struct EFI_ISH)
/* set ISH variable */
#define IOCTL_SET_VARIABLE	_IOR(IOC_MAGIC, 2, struct EFI_ISH)
/* BIOS firmware ready */
#define IOCTL_CAPSULE_READY	_IOR(IOC_MAGIC, 3, struct EFI_ISH)
/* get BIOS version */
#define IOCTL_GET_VERSION	_IOR(IOC_MAGIC, 4, struct EFI_ISH)

#define DEVPATH "/dev/efiio"

#define SPI_FLASH_BASE_ADDR	(0xff000000)
#define SPI_FLASH_SIZE		(0x01000000)
#define BIOS_BIVS_OFFSET_SLOTA	(0x87ffc0)
#define BIOS_BIVS_OFFSET_SLOTB	(0xffffc0)
#define BIOS_BIVS_LENGTH	(32)

struct BIOS_Version_Data {
	uint8_t bivs_hdr[4];
	uint8_t resver1;
	uint8_t bivs_ver[8];
	uint8_t resver2;
	uint8_t bivs_day[8];
	uint8_t resver3;
	uint8_t bivs_time[6];
	uint8_t resver4[3];
};

struct capsule_version {
	struct BIOS_Version_Data slot_a;
	struct BIOS_Version_Data slot_b;
};

#endif
