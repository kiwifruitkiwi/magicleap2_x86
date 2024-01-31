/*
 * Thermal zone driver for AMD Mero Subsystem
 *
 * Copyright (C) 2020-2021 Advanced Micro Devices, Inc. All rights reserved.
 */
#ifndef __MERO_THERMAL_SYSCFG_H
#define __MERO_THERMAL_SYSCFG_H

#include <linux/platform_device.h>
#include <linux/thermal.h>

#define MERO_THERMAL_MAX_ACT_TRIP_COUNT	10
#define DEFAULT_TZ_CNT		3
#define DEFAULT_PSV_TRIP_ID	0
#define DEFAULT_PSV_TEMP	50
#define DEFAULT_HOT_TRIP_ID	1
#define DEFAULT_HOT_TEMP	70
#define DEFAULT_CRT_TRIP_ID	2
#define DEFAULT_CRT_TEMP	105

#define DEFAULT_THERMAL_POLLING_INTERVAL	0

/* the pp_sensors is the duplicated part from amd_pp_sensors */
enum pp_sensors {
	AMDGPU_PP_SENSOR_CPU0_TEMP_CUR = 0x60,
	AMDGPU_PP_SENSOR_CPU0_TEMP_AVE,
	AMDGPU_PP_SENSOR_CPU1_TEMP_CUR,
	AMDGPU_PP_SENSOR_CPU1_TEMP_AVE,
	AMDGPU_PP_SENSOR_CPU2_TEMP_CUR,
	AMDGPU_PP_SENSOR_CPU2_TEMP_AVE,
	AMDGPU_PP_SENSOR_CPU3_TEMP_CUR,
	AMDGPU_PP_SENSOR_CPU3_TEMP_AVE,
	AMDGPU_PP_SENSOR_GFX_TEMP_CUR,
	AMDGPU_PP_SENSOR_GFX_TEMP_AVE,
	AMDGPU_PP_SENSOR_SOC_TEMP_CUR,
	AMDGPU_PP_SENSOR_SOC_TEMP_AVE,
	AMDGPU_PP_SENSOR_CVIP_TEMP_CUR,
	AMDGPU_PP_SENSOR_CVIP_TEMP_AVE,
};

enum pp_sensors_domain {
	X86_TEMP_CUR,
	X86_TEMP_AVG,
	SOC_TEMP_CUR,
	SOC_TEMP_AVG,
	GFX_TEMP_CUR,
	GFX_TEMP_AVG,
	CVIP_TEMP_CUR,
	CVIP_TEMP_AVG,
};

struct thermal_zone_domain_tbl {
	enum pp_sensors_domain	domain;
	enum pp_sensors		sensor;
	const char		*string;
	int			polling_delay;
};

struct active_trip {
	int temp;	//temperature in Centigrade
	int id;		//thermal trip id
	int type;	//thermal trip type
	bool valid;	//thermal trip valid flag
};

struct mero_thermal_zone {
	struct active_trip act_trips[MERO_THERMAL_MAX_ACT_TRIP_COUNT];	//user defined trip data
	int psv_temp;							//passive temperature
	int psv_trip_id;						//passive trip id
	int crt_temp;							//critical temperature
	int crt_trip_id;						//critical trip id
	int hot_temp;							//hot temperature
	int hot_trip_id;						//hot trip id
	int tz_enabled;							//thermal zone enable flag
	struct thermal_zone_device *zone;				//thermal zone device
	struct thermal_zone_device_ops *override_ops;			//thermal zone ops override function
	void *mero_smu_handle;						//mero smu context handle
	enum pp_sensors smu_sensor;					//SMU sensor
};

struct mero_tz_cfg {
	char				*tz_trip_cfg;
	int				polling_interval;
	enum pp_sensors_domain		sensor_domain;
	struct thermal_zone_device_ops	*override_ops;
};

struct mero_thermal_zone *mero_thermal_zone_add(struct mero_tz_cfg *thermal_cfg);
void mero_thermal_zone_remove(struct mero_thermal_zone *mero_thermal_zone,
			      enum pp_sensors_domain domain);
int mero_thermal_read_trips(struct mero_thermal_zone *mero_thermal_zone,
			    struct mero_tz_cfg *thermal_cfg);
int smu_read_sensor(void *mero_smu_handle,
		    enum pp_sensors, int *temp, u32 *size);
void *mero_get_smu_context(void *mero_smu_handle);
#endif /* __MERO_THERMAL_SYSCFG_H */
