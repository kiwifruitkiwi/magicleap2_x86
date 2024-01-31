/*
 * EFI ioctrl driver for AMD Mero Subsystem
 *
 * Copyright (C) 2020 Advanced Micro Devices, Inc. All rights reserved.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/gfp.h>
#include <linux/uaccess.h>
#include <linux/crc32.h>
#include <linux/xz.h>
#include <linux/efi.h>
#include <asm/efi.h>
#include <linux/ucs2_string.h>
#include <linux/uuid.h>

#include <linux/delay.h>
#include "efi_ioctl.h"

#define EFI_DEVICE_NAME "efiio"
#define ISH_RUNTIME_SERVICE_GUID	\
	EFI_GUID(0x8B31A9F9, 0x208F, 0x4B26, 0x93, 0xd9, 0x31, 0xAA, 0x76, \
			0xA6, 0x8D, 0x86)
efi_char16_t efi_ish_name[] = {'I', 's', 'h', 'V', 'a', 'r'};

#define ISH_CAPSULE_UPDATE_GUID		\
	EFI_GUID(0x38605b43, 0xcc36, 0x4a28, 0x86, 0x29, 0x2f, 0x6d, 0x37, \
			0xfd, 0x4f, 0xcc)
efi_char16_t efi_capsule_name[] = {'O', 't', 'a', 'C', 'a', 'p', 's', 'u',
					'l', 'e', 'V', 'a', 'r'};

static int efi_device_major;
static struct class *efi_class;
static struct device *efi_device;

static ssize_t efiio_ioctl(struct file *file, unsigned int cmd,
			   unsigned long arg)
{
	efi_guid_t vendor_guid;
	efi_status_t status = 0;
	struct EFI_ISH *efiish;
	struct EFI_CAPSULE_METADATA *eficapdata;
	struct capsule_version *cap_bivs;
	static void __iomem *spi_reg_addr;
	unsigned long var_len;

	if (_IOC_TYPE(cmd) != IOC_MAGIC)
		return -ENOTTY;

	switch (cmd) {
	case IOCTL_GET_VARIABLE:
		var_len = sizeof(struct EFI_ISH);
		vendor_guid = ISH_RUNTIME_SERVICE_GUID;
		efiish = vmalloc(var_len);
		if (!efiish) {
			pr_err("malloc efiish memory failed\n");
			return -EFAULT;
		}

		status = efi.get_variable(efi_ish_name, &vendor_guid, NULL,
				&var_len, efiish);
		if (status != EFI_SUCCESS) {
			pr_err("efi get variable failed!\n");
			vfree(efiish);
			return -EFAULT;
		}

		if (copy_to_user((char __user *)arg, efiish, var_len)) {
			pr_err("variable pass to user fail\n");
			vfree(efiish);
			return -EFAULT;
		}

		vfree(efiish);
		break;
	case IOCTL_SET_VARIABLE:
		var_len = sizeof(struct EFI_ISH);
		vendor_guid = ISH_RUNTIME_SERVICE_GUID;
		efiish = vmalloc(var_len);
		if (!efiish) {
			pr_err("malloc efiish memory failed\n");
			return -EFAULT;
		}

		if (copy_from_user(efiish, (char __user *)arg, var_len)) {
			pr_err(" variable get from user fail\n");
			vfree(efiish);
			return -EFAULT;
		}

		efi.set_variable(efi_ish_name, &vendor_guid,
				EFI_VARIABLE_BOOTSERVICE_ACCESS |
				EFI_VARIABLE_RUNTIME_ACCESS |
				EFI_VARIABLE_NON_VOLATILE,
				var_len, (void *)efiish);

		if (status != EFI_SUCCESS) {
			pr_err("set variable failed!\n");
			vfree(efiish);
			return -EFAULT;
		}

		vfree(efiish);
		break;
	case IOCTL_CAPSULE_READY:
		var_len = sizeof(struct EFI_CAPSULE_METADATA);
		vendor_guid = ISH_CAPSULE_UPDATE_GUID;
		eficapdata = vmalloc(var_len);
		if (!eficapdata) {
			pr_err("malloc efiish memory failed\n");
			return -EFAULT;
		}

		if (copy_from_user(eficapdata, (char __user *)arg, var_len)) {
			pr_err("variable get from user fail\n");
			vfree(eficapdata);
			return -EFAULT;
		}

		status = efi.set_variable(efi_capsule_name, &vendor_guid,
				EFI_VARIABLE_BOOTSERVICE_ACCESS |
				EFI_VARIABLE_RUNTIME_ACCESS |
				EFI_VARIABLE_NON_VOLATILE,
				var_len, (void *)eficapdata);
		if (status != EFI_SUCCESS) {
			pr_err("efi set variable failed, bios update notifier failed!!\n");
			vfree(eficapdata);
			return -EFAULT;
		}

		vfree(eficapdata);
		break;
	case IOCTL_GET_VERSION:
		spi_reg_addr = ioremap(SPI_FLASH_BASE_ADDR, SPI_FLASH_SIZE);
		var_len = sizeof(struct capsule_version);
		cap_bivs = vmalloc(var_len);
		if (!cap_bivs) {
			pr_err("malloc cap_bivs memory failed\n");
			return -EFAULT;
		}

		/* copy BIVS from SPI flash to userspace */
		memcpy_fromio(&(cap_bivs->slot_a),
			      spi_reg_addr+BIOS_BIVS_OFFSET_SLOTA,
			      BIOS_BIVS_LENGTH);
		memcpy_fromio(&(cap_bivs->slot_b),
			      spi_reg_addr+BIOS_BIVS_OFFSET_SLOTB,
			      BIOS_BIVS_LENGTH);
		if (copy_to_user((char __user *)arg, cap_bivs, var_len)) {
			pr_err("variable pass to user fail\n");
			vfree(cap_bivs);
			return -EFAULT;
		}

		vfree(cap_bivs);
		break;
	default:
		pr_err("Undefined EFI ioctl command!\n");
		return -ENOTTY;
	}

	return 0;
}

static const struct file_operations fileops = {
	.owner		= THIS_MODULE,
	.unlocked_ioctl	= &efiio_ioctl,
};

static int __init efiio_init(void)
{
	efi_device_major = register_chrdev(0, EFI_DEVICE_NAME, &fileops);
	if (efi_device_major < 0) {
		pr_err("device major create failed\n");
		return efi_device_major;
	}

	efi_class = class_create(THIS_MODULE, "eficlass");
	if (IS_ERR(efi_class)) {
		pr_err("class create failed\n");
		unregister_chrdev(efi_device_major, EFI_DEVICE_NAME);
		return PTR_ERR(efi_class);
	}

	efi_device = device_create(efi_class, NULL,
				   MKDEV(efi_device_major, 0), NULL,
				   EFI_DEVICE_NAME);
	if (IS_ERR(efi_device)) {
		pr_err("device create failed\n");
		class_destroy(efi_class);
		unregister_chrdev(efi_device_major, EFI_DEVICE_NAME);
		return PTR_ERR(efi_class);
	}

	return 0;
}

static void __exit efiio_exit(void)
{
	unregister_chrdev(efi_device_major, EFI_DEVICE_NAME);
	class_destroy(efi_class);
}

module_init(efiio_init);
module_exit(efiio_exit);

MODULE_DESCRIPTION("EFI ioctrl driver");
MODULE_VERSION("1.0");
