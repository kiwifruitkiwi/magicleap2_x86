/*
 * Thermal zone driver for AMD Mero Subsystem
 *
 * Copyright (C) 2020-2021 Advanced Micro Devices, Inc. All rights reserved.
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/acpi.h>
#include <linux/thermal.h>
#include "mero_thermal.h"

//Definition of the thermal domain, sensor and strings.
static struct thermal_zone_domain_tbl domain_tbl[] = {
	{X86_TEMP_CUR,	AMDGPU_PP_SENSOR_CPU0_TEMP_CUR,	"Mero x86 Core0",
	 DEFAULT_THERMAL_POLLING_INTERVAL},
	{X86_TEMP_CUR,	AMDGPU_PP_SENSOR_CPU1_TEMP_CUR,	"Mero x86 Core1",
	 DEFAULT_THERMAL_POLLING_INTERVAL},
	{X86_TEMP_CUR,	AMDGPU_PP_SENSOR_CPU2_TEMP_CUR,	"Mero x86 Core2",
	 DEFAULT_THERMAL_POLLING_INTERVAL},
	{X86_TEMP_CUR,	AMDGPU_PP_SENSOR_CPU3_TEMP_CUR,	"Mero x86 Core3",
	 DEFAULT_THERMAL_POLLING_INTERVAL},
	{GFX_TEMP_CUR,	AMDGPU_PP_SENSOR_GFX_TEMP_CUR,	"Mero GFX Thermal",
	 DEFAULT_THERMAL_POLLING_INTERVAL},
	{SOC_TEMP_CUR,	AMDGPU_PP_SENSOR_SOC_TEMP_CUR,	"Mero SoC Thermal",
	 DEFAULT_THERMAL_POLLING_INTERVAL},
	{CVIP_TEMP_CUR,	AMDGPU_PP_SENSOR_CVIP_TEMP_CUR,	"Mero CVIP Thermal",
	 DEFAULT_THERMAL_POLLING_INTERVAL},
};

static int mero_thermal_get_zone_temp(struct thermal_zone_device *zone,
					 int *temp)
{
	u32	size;
	struct	mero_thermal_zone *tz = zone->devdata;

	if (!tz || !tz->mero_smu_handle)
		return -EINVAL;

	if (tz->override_ops && tz->override_ops->get_temp)
		return tz->override_ops->get_temp(zone, temp);

	smu_read_sensor(tz->mero_smu_handle, tz->smu_sensor,
			temp, &size);

	zone->last_temperature = zone->temperature;
	zone->temperature =	*temp;

	return 0;
}

static int mero_thermal_get_trip_temp(struct thermal_zone_device *zone,
					 int trip, int *temp)
{
	struct mero_thermal_zone *tz = zone->devdata;

	if (!tz)
		return -EINVAL;

	if (tz->override_ops && tz->override_ops->get_trip_temp)
		return tz->override_ops->get_trip_temp(zone, trip, temp);

	if (trip == tz->crt_trip_id)
		*temp = tz->crt_temp;
	else if (trip == tz->psv_trip_id)
		*temp = tz->psv_temp;
	else if (trip == tz->hot_trip_id)
		*temp = tz->hot_temp;
	else
		return -EINVAL;

	return 0;
}

static int mero_thermal_get_trip_type(struct thermal_zone_device *zone,
					 int trip,
					 enum thermal_trip_type *type)
{
	struct mero_thermal_zone *tz = zone->devdata;

	if (!tz)
		return -EINVAL;

	if (tz->override_ops && tz->override_ops->get_trip_type)
		return tz->override_ops->get_trip_type(zone, trip, type);

	if (trip == tz->crt_trip_id)
		*type = THERMAL_TRIP_CRITICAL;
	else if (trip == tz->hot_trip_id)
		*type = THERMAL_TRIP_HOT;
	else if (trip == tz->psv_trip_id)
		*type = THERMAL_TRIP_PASSIVE;
	else
		return -EINVAL;

	return 0;
}

static int mero_thermal_set_trip_temp(struct thermal_zone_device *zone,
				      int trip, int temp)
{
	struct mero_thermal_zone *tz = zone->devdata;

	if (!tz)
		return -EINVAL;

	if (tz->override_ops && tz->override_ops->set_trip_temp)
		return tz->override_ops->set_trip_temp(zone, trip, temp);

	if (trip == tz->crt_trip_id)
		tz->crt_temp = temp;
	else if (trip == tz->psv_trip_id)
		tz->psv_temp = temp;
	else if (trip == tz->hot_trip_id)
		tz->hot_temp = temp;
	else
		return -EINVAL;

	return 0;
}

static int mero_thermal_get_zone_mode(struct thermal_zone_device *zone,
				enum thermal_device_mode *mode)
{
	struct mero_thermal_zone *tz = zone->devdata;

	if (!tz)
		return -EINVAL;

	*mode = tz->tz_enabled ? THERMAL_DEVICE_ENABLED :
		THERMAL_DEVICE_DISABLED;

	return 0;
}

static int mero_thermal_set_zone_mode(struct thermal_zone_device *zone,
				enum thermal_device_mode mode)
{
	struct mero_thermal_zone *tz = zone->devdata;

	if (!tz)
		return -EINVAL;

	/*
	 * enable/disable thermal management from ACPI thermal driver
	 */
	if (mode == THERMAL_DEVICE_ENABLED)
		tz->tz_enabled = true;
	else if (mode == THERMAL_DEVICE_DISABLED) {
		tz->tz_enabled = false;
		dev_info(&zone->device, "thermal zone will be disabled\n");
	} else
		return -EINVAL;

	return 0;
}

static int mero_thermal_get_trend(struct thermal_zone_device *zone,
				  int trip, enum thermal_trend *trend)
{
	struct mero_thermal_zone *tz = zone->devdata;
	int trip_temp, temp, last_temp, ret;

	if (!tz)
		return -EINVAL;

	ret = zone->ops->get_trip_temp(zone, trip, &trip_temp);
	if (ret)
		return ret;

	temp = READ_ONCE(zone->temperature);
	last_temp = READ_ONCE(zone->last_temperature);

	if (temp > trip_temp) {
		if (temp >= last_temp)
			*trend = THERMAL_TREND_RAISING;
		else
			*trend = THERMAL_TREND_STABLE;
	} else if (temp < trip_temp) {
		*trend = THERMAL_TREND_DROPPING;
	} else {
		*trend = THERMAL_TREND_STABLE;
	}

	return 0;
}

static int mero_thermal_bind(struct thermal_zone_device *zone,
			     struct thermal_cooling_device *cdev)
{
	int i, ret;

	/* Update the type or other conditions to check the target device */
	if (!strncmp("Processor", cdev->type, THERMAL_NAME_LENGTH)) {
		dev_dbg(&zone->device,
			"skip binding for %s\n",
			cdev->type);
		return 0;
	}

	for (i = 0; i < zone->trips; i++) {
		/* The lower and upper limits are using i*2 and (i*2+1) */
		ret = thermal_zone_bind_cooling_device(zone, i, cdev,
						       (i * 2 + 1),
						       i * 2,
						       THERMAL_WEIGHT_DEFAULT);
		if (ret) {
			dev_err(&zone->device,
				"binding zone %s trip %d with cdev %s failed:%d\n",
				zone->type, i, cdev->type, ret);
			return ret;
		}
	}
	return 0;
}

static int mero_thermal_unbind(struct thermal_zone_device *zone,
			       struct thermal_cooling_device *cdev)
{
	int i, ret;

	/* Update the type or other conditions to check the target device */
	if (!strncmp("Processor", cdev->type, THERMAL_NAME_LENGTH)) {
		dev_dbg(&zone->device,
			"skip unbinding for %s\n",
			cdev->type);
		return 0;
	}

	for (i = 0; i < zone->trips; i++) {
		ret = thermal_zone_unbind_cooling_device(zone, i, cdev);
		if (ret) {
			dev_err(&zone->device,
				"unbinding zone %s trip %d with cdev %s failed:%d\n",
				zone->type, i, cdev->type, ret);
			return ret;
		}
	}
	return 0;
}

static struct thermal_zone_device_ops mero_thermal_zone_ops = {
	.bind		= mero_thermal_bind,
	.unbind		= mero_thermal_unbind,
	.get_temp	= mero_thermal_get_zone_temp,
	.get_mode	= mero_thermal_get_zone_mode,
	.set_mode	= mero_thermal_set_zone_mode,
	.get_trip_temp	= mero_thermal_get_trip_temp,
	.get_trip_type	= mero_thermal_get_trip_type,
	.set_trip_temp	= mero_thermal_set_trip_temp,
	.get_trend	= mero_thermal_get_trend,
};

/**
 * mero_parse_thermal_config - Parse the tz_cfg module parameter
 *
 *
 * The configure format is type0.temperature0, type1.temperature1, ...
 * Refer to thermal_trip_type for the type value
 */
static int mero_parse_thermal_config(struct mero_thermal_zone *mero_zone,
				     struct mero_tz_cfg *thermal_cfg)
{
	int type, temperature;
	int count;
	const char *p;

	count = 0;
	if (!thermal_cfg->tz_trip_cfg || !*thermal_cfg->tz_trip_cfg)
		return count;

	p = thermal_cfg->tz_trip_cfg;
	for (;;) {
		char *next;
		int ret = sscanf(p, "%u.%u", &type, &temperature);

		if (ret < 2)
			return count;

		if (type <= THERMAL_TRIP_CRITICAL) {
			mero_zone->act_trips[count].temp = temperature*1000;
			mero_zone->act_trips[count].type = type;
			mero_zone->act_trips[count].id = count;
			mero_zone->act_trips[count].valid = true;
			switch (type) {
			case THERMAL_TRIP_PASSIVE:
				mero_zone->psv_trip_id = count;
				mero_zone->psv_temp = mero_zone->act_trips[count].temp;
				break;
			case THERMAL_TRIP_HOT:
				mero_zone->hot_trip_id = count;
				mero_zone->hot_temp = mero_zone->act_trips[count].temp;
				break;
			case THERMAL_TRIP_CRITICAL:
				mero_zone->crt_trip_id = count;
				mero_zone->crt_temp = mero_zone->act_trips[count].temp;
				break;
			default:
				break;
			}
			count++;
		}
		next = strchr(p, ',');
		if (!next)
			break;
		p = next + 1;
	}

	return count;
}

static int mero_thermal_get_trip_config(struct mero_thermal_zone *mero_zone,
					struct mero_tz_cfg *thermal_cfg)
{
	int count;
	int thermal_cfg_cnt;

	thermal_cfg_cnt = mero_parse_thermal_config(mero_zone, thermal_cfg);
	if (thermal_cfg_cnt != 0)
		count = thermal_cfg_cnt;
	else {
		//default tz trip value
		mero_zone->psv_trip_id = DEFAULT_PSV_TRIP_ID;
		mero_zone->psv_temp = DEFAULT_PSV_TEMP*1000;
		mero_zone->hot_trip_id = DEFAULT_HOT_TRIP_ID;
		mero_zone->hot_temp = DEFAULT_HOT_TEMP*1000;
		mero_zone->crt_trip_id = DEFAULT_CRT_TRIP_ID;
		mero_zone->crt_temp = DEFAULT_CRT_TEMP*1000;
		count = DEFAULT_TZ_CNT;
	}

	return count;
}

int mero_thermal_read_trips(struct mero_thermal_zone *mero_zone,
			    struct mero_tz_cfg *thermal_cfg)
{
	int trip_cnt;

	trip_cnt = mero_thermal_get_trip_config(mero_zone, thermal_cfg);

	return trip_cnt;
}

static struct thermal_zone_params mero_thermal_params = {
	.governor_name = "user_space",
	.no_hwmon = true,
};

struct mero_thermal_zone *mero_thermal_zone_add(struct mero_tz_cfg *thermal_cfg)
{
	struct mero_thermal_zone *mero_zone, *tz_data;
	int trip_cnt, i;
	int trip_mask = 0;
	int ret;
	int tz_num = 0;

	if (!thermal_cfg)
		return ERR_PTR(-EINVAL);

	if (thermal_cfg->sensor_domain < X86_TEMP_CUR ||
	    thermal_cfg->sensor_domain > CVIP_TEMP_AVG)
		return ERR_PTR(-EINVAL);

	/* Search for the SMU sensor number*/
	for (i = 0; i < sizeof(domain_tbl) / sizeof(struct thermal_zone_domain_tbl); i++) {
		if (domain_tbl[i].domain == thermal_cfg->sensor_domain)
			tz_num++;
	}
	//Allocate memory thermal zone data multiply zone number
	mero_zone = kzalloc(sizeof(*mero_zone) * tz_num, GFP_KERNEL);
	if (!mero_zone)
		return ERR_PTR(-ENOMEM);

	tz_data = mero_zone;

	if (!thermal_cfg->override_ops)
		mero_zone->override_ops = thermal_cfg->override_ops;

	/* Register the thermal zone according to sensor number*/
	for (i = 0;  i < sizeof(domain_tbl) / sizeof(struct thermal_zone_domain_tbl); i++) {
		//Search for the target sensor domain
		if (domain_tbl[i].domain == thermal_cfg->sensor_domain) {
			mero_zone->mero_smu_handle = mero_get_smu_context(NULL);
			trip_cnt = mero_thermal_read_trips(mero_zone, thermal_cfg);
			//Save the sensor name in thermal zone data
			mero_zone->smu_sensor = domain_tbl[i].sensor;
			//Enable the zone by default
			mero_zone->tz_enabled = true;
			domain_tbl[i].polling_delay =
				thermal_cfg->polling_interval;

			pr_debug("sensor domain = %d, polling_delay = %d\n",
				 domain_tbl[i].domain,
				 domain_tbl[i].polling_delay);

			mero_zone->zone = thermal_zone_device_register(domain_tbl[i].string,
								       trip_cnt, trip_mask,
								       mero_zone,
								       &mero_thermal_zone_ops,
								       &mero_thermal_params,
								       0, domain_tbl[i].polling_delay);
			if (IS_ERR(mero_zone->zone)) {
				ret = PTR_ERR(mero_zone->zone);
				goto err_thermal_zone;
			}
			//Point to the next thermal zone data
			mero_zone++;
		}
	}

	return tz_data;

err_thermal_zone:
	kfree(tz_data);
	return ERR_PTR(ret);
}
EXPORT_SYMBOL_GPL(mero_thermal_zone_add);

void mero_thermal_zone_remove(struct mero_thermal_zone
				 *mero_thermal_zone, enum pp_sensors_domain domain)
{
	int	i;
	int tz_num = 0;
	struct mero_thermal_zone *tz_data;

	if (domain < X86_TEMP_CUR || domain > CVIP_TEMP_AVG)
		return;

	/* Search for the SMU sensor number*/
	tz_data = mero_thermal_zone;
	for (i = 0; i < sizeof(domain_tbl) / sizeof(struct thermal_zone_domain_tbl); i++) {
		if (domain_tbl[i].domain == domain)
			tz_num++;
	}
	/* Unregister the thermal zone*/
	for (i = 0; i < tz_num; i++) {
		if (!tz_data && !tz_data->zone)
			thermal_zone_device_unregister(tz_data->zone);

		//point to the next thermal zone data.
		tz_data++;
	}
	kfree(mero_thermal_zone);
}
EXPORT_SYMBOL_GPL(mero_thermal_zone_remove);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("Mero Thermal Zone driver");
