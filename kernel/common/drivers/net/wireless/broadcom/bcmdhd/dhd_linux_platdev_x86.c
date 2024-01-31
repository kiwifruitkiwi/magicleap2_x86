// SPDX-License-Identifier: GPL-2.0
/* Copyright (c) 2020, Magic Leap, Inc. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <typedefs.h>
#include <linux/acpi.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <bcmutils.h>
#include <dhd_dbg.h>
#include <dngl_stats.h>
#include <dhd.h>
#include <dhd_bus.h>
#include <dhd_linux.h>
#include <wl_android.h>

#define WIFI_PLAT_NAME		"bcmdhd_wlan"
bcmdhd_wifi_platdata_t		*dhd_wifi_platdata;
extern uint			dhd_driver_init_done;

#ifdef GET_CUSTOM_MAC_ENABLE
extern int dhd_get_mac_addr(unsigned char *buf);
#endif

#ifdef CONFIG_BROADCOM_WIFI_RESERVED_MEM
extern int dhd_init_wlan_mem(void);
extern void *dhd_wlan_mem_prealloc(int section, unsigned long size);
extern void dhd_exit_wlan_mem(void);
#endif /* CONFIG_BROADCOM_WIFI_RESERVED_MEM */

#ifdef DHD_WIFI_SHUTDOWN
extern void wifi_plat_dev_drv_shutdown(struct platform_device *pdev);
#endif /* DHD_WIFI_SHUTDOWN */

#ifdef BCMPCIE
static int dhd_wlan_load_pcie(struct device *dev)
{
	wifi_adapter_info_t *adapter = &dhd_wifi_platdata->adapters[0];
	int rc = 0;

	dev_info(dev, "%s: enter\n", __func__);
	if (dhd_download_fw_on_driverload) {
		/* power up the adapter */
		dev_info(dev, "Power-up adapter '%s'\n", adapter->name);
		dev_info(dev, "irq %d [flags %d], firmware: %s, nvram: %s\n",
			 adapter->irq_num, adapter->intr_flags,
			 adapter->fw_path, adapter->nv_path);
		dev_info(dev, "bus type %d, bus num %d, slot num %d\n",
			 adapter->bus_type, adapter->bus_num,
			 adapter->slot_num);
		wifi_platform_set_power(adapter, TRUE, WIFI_TURNON_DELAY);
	}

	rc = dhd_bus_register();
	if (!rc) {
		dhd_driver_init_done = TRUE;
	} else {
		dev_err(dev, "%s: dhd_bus_register failed\n", __func__);
		if (dhd_download_fw_on_driverload) {
			/* power down the adapter */
			wifi_platform_set_power(adapter, FALSE,
						WIFI_TURNOFF_DELAY);
		}
	}
	return rc;
}
#endif /* BCMPCIE  */

static int dhd_wlan_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	wifi_adapter_info_t *adapter;
	int irq, rc;

	adapter = devm_kzalloc(dev, sizeof(wifi_adapter_info_t), GFP_KERNEL);
	if (!adapter)
		return -ENOMEM;

	adapter->name = "DHD generic adapter";
	adapter->bus_type = -1;
	adapter->bus_num = -1;
	adapter->slot_num = -1;
	adapter->irq_num = -1;

	adapter->wl_en = devm_gpiod_get(dev, "reset", GPIOD_OUT_HIGH);
	if (IS_ERR(adapter->wl_en)) {
		rc = PTR_ERR(adapter->wl_en);
		dev_err(dev, "Failed to obtain wl_en gpio, %d\n", rc);
		goto exit;
	}

	irq = platform_get_irq(pdev, 0);
	if (irq == -EPROBE_DEFER)
		return -EPROBE_DEFER;

	if (irq >= 0) {
		adapter->irq_num = irq;
		adapter->intr_flags = IRQF_TRIGGER_RISING | IRQF_ONESHOT;
		dev_info(dev, "OOB irq = %d\n", adapter->irq_num);
	} else {
		/* OOB irq is optional, so just warn */
		dev_warn(dev, "OOB irq is not defined\n");
	}

#ifdef CONFIG_BROADCOM_WIFI_RESERVED_MEM
	rc = dhd_init_wlan_mem();
	if (rc) {
		dev_err(dev, "failed to alloc reserved memory, %d\n", rc);
		goto exit;
	}
#endif /* CONFIG_BROADCOM_WIFI_RESERVED_MEM */

	dhd_wifi_platdata->num_adapters = 1;
	dhd_wifi_platdata->adapters = adapter;
	platform_set_drvdata(pdev, adapter);

	wl_android_init();
	rc = dhd_wlan_load_pcie(dev);
	if (rc) {
		dev_err(dev, "pcie failure, %d\n", rc);
		goto pcie_failure;
	}
	wl_android_post_init();
	dev_info(dev, "%s: done\n", __func__);
	return 0;

pcie_failure:
	wl_android_exit();
#ifdef CONFIG_BROADCOM_WIFI_RESERVED_MEM
	dhd_exit_wlan_mem();
#endif /* CONFIG_BROADCOM_WIFI_RESERVED_MEM */
exit:
	return rc;
}

static int dhd_wlan_remove(struct platform_device *pdev)
{
	dev_info(&pdev->dev, "%s\n", __func__);
	wl_android_exit();
	dhd_bus_unregister();
	wifi_platform_set_power(&dhd_wifi_platdata->adapters[0], FALSE, 0);
#ifdef CONFIG_BROADCOM_WIFI_RESERVED_MEM
	dhd_exit_wlan_mem();
#endif /* CONFIG_BROADCOM_WIFI_RESERVED_MEM */
	dhd_wifi_platdata->num_adapters = 0;
	dhd_wifi_platdata->adapters = NULL;
	return 0;
}

#ifdef CONFIG_ACPI
static const struct acpi_device_id dhd_wlan_acpi_match[] = {
	{"DHD4375", 0},
	{"", 0},
};
MODULE_DEVICE_TABLE(acpi, dhd_wlan_acpi_match);
#endif

static struct platform_driver wifi_platform_dev_driver = {
	.probe = dhd_wlan_probe,
	.remove = dhd_wlan_remove,
	.driver = {
		.name = WIFI_PLAT_NAME,
#ifdef CONFIG_ACPI
		.acpi_match_table = ACPI_PTR(dhd_wlan_acpi_match),
#endif
	}
};

int dhd_wifi_platform_register_drv(void)
{
	if (!dhd_wifi_platdata) {
		dhd_wifi_platdata = kzalloc(sizeof(bcmdhd_wifi_platdata_t),
					    GFP_KERNEL);
		if (!dhd_wifi_platdata)
			return -ENOMEM;
	}
	return platform_driver_register(&wifi_platform_dev_driver);
}

void dhd_wifi_platform_unregister_drv(void)
{
	platform_driver_unregister(&wifi_platform_dev_driver);
	kfree(dhd_wifi_platdata);
	dhd_wifi_platdata = NULL;
}

void *wifi_platform_prealloc(wifi_adapter_info_t *adapter, int section,
			     unsigned long size)
{
#ifdef CONFIG_BROADCOM_WIFI_RESERVED_MEM
	void *alloc_ptr = dhd_wlan_mem_prealloc(section, size);

	if (alloc_ptr) {
		DHD_INFO(("success alloc section %d\n", section));
		if (size != 0L)
			bzero(alloc_ptr, size);
		return alloc_ptr;
	}
	DHD_ERROR(("%s: failed to alloc static mem section %d\n",
		   __func__, section));
#endif
	return NULL;
}

int wifi_platform_get_irq_number(wifi_adapter_info_t *adapter,
				 unsigned long *irq_flags_ptr)
{
	if (!adapter)
		return -1;

	if (irq_flags_ptr)
		*irq_flags_ptr = adapter->intr_flags;
	return adapter->irq_num;
}

int wifi_platform_set_power(wifi_adapter_info_t *adapter, bool on,
			    unsigned long msec)
{
	DHD_INFO(("%s(%d)\n", __func__, on));
	if (gpiod_cansleep(adapter->wl_en))
		gpiod_set_value_cansleep(adapter->wl_en, on);
	else
		gpiod_set_value(adapter->wl_en, on);

	if (msec)
		OSL_SLEEP(msec);
	return 0;
}

int wifi_platform_get_mac_addr(wifi_adapter_info_t *adapter, unsigned char *buf)
{
#ifdef GET_CUSTOM_MAC_ENABLE
	DHD_INFO(("%s\n", __func__));
	return dhd_get_mac_addr(buf);
#else
	DHD_INFO(("%s: custom WLAN MAC is not supported\n", __func__));
	return -EOPNOTSUPP;
#endif
}

#ifdef	CUSTOM_COUNTRY_CODE
void *wifi_platform_get_country_code(wifi_adapter_info_t *adapter, char *ccode,
				     u32 flags)
#else
void *wifi_platform_get_country_code(wifi_adapter_info_t *adapter, char *ccode)
#endif /* CUSTOM_COUNTRY_CODE */
{
	return NULL;
}

wifi_adapter_info_t *dhd_wifi_platform_get_adapter(uint32 bus_type,
						   uint32 bus_num,
						   uint32 slot_num)
{
	wifi_adapter_info_t *adapter;

	if (!dhd_wifi_platdata)
		return NULL;

	adapter = &dhd_wifi_platdata->adapters[0];
	if ((adapter->bus_type == -1 || adapter->bus_type == bus_type) &&
	    (adapter->bus_num == -1 || adapter->bus_num == bus_num) &&
	    (adapter->slot_num == -1 || adapter->slot_num == slot_num)) {
		DHD_TRACE(("found adapter info '%s'\n", adapter->name));
		return adapter;
	}
	return NULL;
}

