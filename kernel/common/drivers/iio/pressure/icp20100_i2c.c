// SPDX-License-Identifier: GPL-2.0-only
/* Copyright (c) 2021, Magic Leap, Inc. All rights reserved.
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

#include <linux/module.h>
#include <linux/regmap.h>
#include <linux/i2c.h>
#include <linux/acpi.h>
#include "icp20100.h"

/*
 * read_flag_mask:
 *   - address bit 7 must be set to request a register read operation
 */
static const struct regmap_config icp20100_regmap_i2c_config = {
	.reg_bits       = 8,
	.val_bits       = 8,
	.max_register   = TEMP_DATA_2,
	.cache_type     = REGCACHE_NONE,
};

static const struct i2c_device_id icp20100_i2c_id[] = {
		{ "icp20100", 0 },
		{ },
};
MODULE_DEVICE_TABLE(i2c, icp20100_i2c_id);

#if defined(CONFIG_ACPI)
static struct acpi_device_id icp20100_acpi_match[] = {
		{ "ICP20100", 0 },
		{ },
};
MODULE_DEVICE_TABLE(acpi, icp20100_acpi_match);
#endif

static int icp20100_probe_i2c(struct i2c_client *client,
			      const struct i2c_device_id *i2c_id)
{
	struct regmap *regmap;

	regmap = devm_regmap_init_i2c(client, &icp20100_regmap_i2c_config);
	if (IS_ERR(regmap)) {
		dev_err(&client->dev, "failed to init registers map");
		return PTR_ERR(regmap);
	}

	dev_dbg(&client->dev, "device name: %s, irq pin: %u",
		icp20100_i2c_id[0].name, client->irq);

	return icp20100_probe(&client->dev,
			      icp20100_i2c_id[0].name,
			      client->irq,
			      (unsigned int)ICP20100_DEVICE_ID,
			      regmap);
}

static int icp20100_remove_i2c(struct i2c_client *client)
{
	icp20100_remove(&client->dev);
	return 0;
}

static void icp20100_shutdown_i2c(struct i2c_client *client)
{
	icp20100_shutdown(&client->dev);
}

static struct i2c_driver icp20100_i2c_driver = {
	.driver = {
		.name           = "icp20100-i2c",
#if defined(CONFIG_ACPI)
		.acpi_match_table = ACPI_PTR(icp20100_acpi_match),
#endif
	},
	.probe    = icp20100_probe_i2c,
	.remove   = icp20100_remove_i2c,
	.shutdown = icp20100_shutdown_i2c,
	.id_table = icp20100_i2c_id,
};
module_i2c_driver(icp20100_i2c_driver);

MODULE_AUTHOR("Siddarth Kare <skare@magicleap.com>");
MODULE_DESCRIPTION("I2C driver for Invensense ICP20100 pressure sensor");
MODULE_LICENSE("GPL v2");
