/*
 * Thermal zone driver for AMD Mero Subsystem
 *
 * Copyright (C) 2020-2021 Advanced Micro Devices, Inc. All rights reserved.
 */
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/thermal.h>
#include <linux/acpi.h>
#include "mero_thermal.h"

char *mero_tz_trip_cfg;
int mero_polling_interval = DEFAULT_THERMAL_POLLING_INTERVAL;

MODULE_PARM_DESC(tz_cfg, "MERO thermal zone config(type.temperature,...)");
module_param_named(tz_cfg, mero_tz_trip_cfg, charp, 0444);
MODULE_PARM_DESC(polling_interval, "MERO thermal device polling interval (ms)");
module_param_named(polling_interval, mero_polling_interval, int, 0444);

static int mero_soc_thermal_probe(struct platform_device *pdev)
{
	struct mero_thermal_zone	*mero_soc_thermal_zone;
	struct mero_tz_cfg			thermal_cfg;

	//Initialize the thermal configuration
	thermal_cfg.tz_trip_cfg = mero_tz_trip_cfg;
	thermal_cfg.polling_interval = mero_polling_interval;
	thermal_cfg.sensor_domain = SOC_TEMP_CUR;
	thermal_cfg.override_ops = NULL;

	mero_soc_thermal_zone = mero_thermal_zone_add(&thermal_cfg);

	platform_set_drvdata(pdev, mero_soc_thermal_zone);

	return 0;
}

static int mero_soc_thermal_remove(struct platform_device *pdev)
{
	struct mero_thermal_zone *mero_soc_thermal_zone = platform_get_drvdata(pdev);

	mero_thermal_zone_remove(mero_soc_thermal_zone, SOC_TEMP_CUR);

	return 0;
}

static const struct acpi_device_id mero_soc_thermal_match[] = {
	{"AMDI0066", 0},
	{}
};

MODULE_DEVICE_TABLE(acpi, mero_soc_thermal_match);

static struct platform_driver mero_thermal_driver = {
	.probe = mero_soc_thermal_probe,
	.remove = mero_soc_thermal_remove,
	.driver = {
		   .name = "mero soc thermal",
		   .acpi_match_table = ACPI_PTR(mero_soc_thermal_match),
		   },
};

module_platform_driver(mero_thermal_driver);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("Mero SOC Thermal driver");
