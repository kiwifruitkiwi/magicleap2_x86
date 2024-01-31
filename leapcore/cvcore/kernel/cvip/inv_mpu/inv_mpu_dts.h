/* SPDX-License-Identifier: GPL-2.0-only
 *
 * Copyright (C) 2012-2017 InvenSense, Inc.
 */

#ifndef _INV_MPU_DTS_H_
#define _INV_MPU_DTS_H_

#include <linux/kernel.h>
#include <linux/iio/imu/mpu.h>

#ifdef CONFIG_OF
int invensense_mpu_parse_dt(struct device *dev,
			    struct mpu_platform_data *pdata);
#endif

#ifndef __KERNEL__
#error "Can't include kernel header in userspace"
#endif

#endif /* #ifndef _INV_MPU_DTS_H_ */
