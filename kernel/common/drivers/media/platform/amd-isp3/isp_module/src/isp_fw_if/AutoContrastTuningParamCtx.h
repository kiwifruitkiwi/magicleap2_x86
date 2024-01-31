/*
 * Copyright (C) 2020 Advanced Micro Devices, Inc. All rights reserved.
 */

#pragma once

#include "os_base_type.h"


#ifdef __cplusplus
extern "C"
{
#endif

#define AUTOCONTRAST_ALGO_TUNING_VER 3001200908U

struct _autoContrastTuningParamT {
	unsigned int version;

	int isFixFlare;
	int fixFlareValue;
	int maxFlareLimit;

	int gamma_lv_th[2];
	int gammaSmoothRatio;

	int flare_offset_permille;

	int flare_offset_discount_lv_weight;
	int flare_offset_discount_stddev_weight;

	int flare_offset_discount_stddev_x[3];
	int flare_offset_discount_stddev_y[3];

	int flare_offset_discount_lv_lut_x[3];
	int flare_offset_discount_lv_lut_y[3];

	int flare_smooth_strength_delta_ev_thr;

	int flare_smooth_stable_range;
	int flare_smooth_step_slow;
	int flare_smooth_stpe_fast;
	//  int flare_offset_smooth_filter[10];
};

#ifdef __cplusplus
}
#endif
