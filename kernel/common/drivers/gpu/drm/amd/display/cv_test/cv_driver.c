/*
 * Copyright 2019 Advanced Micro Devices, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors: AMD
 *
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/pci.h>
#include <linux/device.h>
#include <drm/drm_device.h>
#include <drm/drm_drv.h>
#include <linux/delay.h>

#include "amdgpu_dm_cv_driver.h"

struct cv_pci_device_id {
	__u32 vendor, device;
};

static const struct cv_pci_device_id pciidlist[] = {
	/* Raven */
	{0x1002, 0x15dd},
	{0x1002, 0x15d8},
	/* Navi10 */
	{0x1002, 0x7310},
	{0x1002, 0x7312},
	{0x1002, 0x7318},
	{0x1002, 0x7319},
	{0x1002, 0x731A},
	{0x1002, 0x731B},
	{0x1002, 0x731F}
};

static struct drm_device *drmdev;

/**
 * amdgpu_dm_cv_driver_callback
 *
 * @brief
 * Callback used to notify the CV driver of various events on the
 * ML headset connector which the CV driver can use to drive its
 * SDP/AUX message sending logic
 *
 * @param
 * enum amdgpu_dm_cv_driver_event amdgpu_dm_cv_driver_event- [in] Events sent to the CV driver
 *                          to indicate state changes on the display output for the ML headset
 *
 * int connector_id - [in] provided for MODE_SET
 *			indicated connector_id for MST stream #1 of the ML headset
 * int frame_length- [in] provided for VSYNC event
 *			length of a frame in microseconds
 * int scanline- [in] provided for VSYNC event
 *			current scanline
 */
void amdgpu_dm_cv_driver_callback(
	enum amdgpu_dm_cv_driver_event amdgpu_dm_cv_driver_event,
	int connector_id, int frame_length, int scanline)
{
	int address = 0x300;
	char aux_message[16] = {
		0x1f, 0x7c, 0xf0, 0xc1, 0x1f, 0x7c, 0xf0, 0xc1,
		0x1f, 0x7c, 0xf0, 0xc1, 0x1f, 0x7c, 0xf0, 0xc1
	};
	int aux_message_length = 16;
	char sdp_message[36] = { 0x0f, 0x83, 0xa5, 0xa5, 0x01, 0x02, 0x03, 0x04,
				 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c,
				 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14,
				 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c,
				 0x1d, 0x1e, 0x1f, 0x20 };
	int sdp_message_length = 36;

	switch (amdgpu_dm_cv_driver_event) {
	case MODE_SET:
		pr_info("%s : MODE_SET event.\n", __func__);
		break;
	case MODE_RESET:
		pr_info("%s : MODE_RESET event.\n", __func__);
		break;
	case DARK_MODE_ENTER:
		amdgpu_dm_send_aux_message(drmdev, connector_id, address,
					   aux_message, aux_message_length);
		pr_info("%s : DARK_MODE_ENTER event.\n", __func__);
		break;
	case DARK_MODE_EXIT:
		amdgpu_dm_send_sdp_message(drmdev, connector_id, sdp_message,
					   sdp_message_length);
		pr_info("%s : DARK_MODE_EXIT event.\n", __func__);
		break;
	case VSYNC:
		pr_info("%s : VSYNC event frame_length = %d, scanline = %d!\n",
			__func__, frame_length, scanline);
		break;
	default:
		break;
	}

	pr_info("%s : amdgpu_dm_cv_driver_event =  %d!\n", __func__,
		amdgpu_dm_cv_driver_event);
}

static struct pci_dev *find_amdgpu_pdev(void)
{
	int i = 0;
	struct pci_dev *pdev = NULL;

	for (i = 0; !pdev && i < ARRAY_SIZE(pciidlist); i++)
		pdev = pci_get_device(pciidlist[i].vendor, pciidlist[i].device, pdev);

	return pdev;
}

static int __init cv_driver_start(void)
{
	struct pci_dev *pdev = NULL;

	pr_info("%s : Enter cv driver module.\n", __func__);

	/* discover the amdgpu driver */
	pdev = find_amdgpu_pdev();

	if (pdev != NULL) {
		if (pdev->dev.driver_data != NULL) {
			drmdev = (struct drm_device *)pdev->dev.driver_data;

			if (drmdev->dev_private != NULL) {
				/* register a callback for amdgpu_dm_cv_driver_event events */
				amdgpu_dm_register_cv_driver(
					&amdgpu_dm_cv_driver_callback, drmdev);
			} else
				pr_info("%s : drmdev->dev_private == NULL\n",
					__func__);
		} else
			pr_info("%s : pdev->dev.driver_data == NULL\n",
				__func__);
	} else
		pr_info("%s : ASIC not found\n", __func__);
	return 0;
}

/*
 * Currently sample CV driver module unload is not supported, since the CV module is never expected
 * to be unloaded in the first place.
 */
static void __exit cv_driver_end(void)
{
	pr_info("%s : Exit cv driver module.\n", __func__);
}

module_init(cv_driver_start);
module_exit(cv_driver_end);

MODULE_AUTHOR("AMD Display Team");
MODULE_DESCRIPTION("AMD GPU");
MODULE_LICENSE("Dual BSD/GPL");
