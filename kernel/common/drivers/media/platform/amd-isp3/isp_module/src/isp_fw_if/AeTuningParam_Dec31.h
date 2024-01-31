/*
 * Copyright (C) 2019-2020 Advanced Micro Devices, Inc. All rights reserved.
 */

#pragma once

#include "os_base_type.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define AE_ALGO_CALIB_VER 3001191205U
#define AE_ALGO_TUNING_VER 3001191205U

struct _ae_lv_calibration_param_t {
	unsigned int ae_cal_lv; //10 base, e.g. 115 = LV11.5
	unsigned int ae_cal_lv_value;
	unsigned int ae_cal_lv_iTime;
	unsigned int ae_cal_lv_aGain;
	unsigned int ae_cal_lv_dGain;
};

struct _ae_calibration_param_t {
	unsigned int version;
	struct _ae_lv_calibration_param_t lv_cal_param;
};

enum _AE_EV_BASE_ENUM {
	AE_EV_1EV_AS_100,
	AE_EV_1EV_AS_10
};

enum _AE_SMOOTH_DEBUG_ENUM {
	AE_SMOOTH_DEBUG_DISABLE,
	AE_SMOOTH_DEBUG_FIX_EXPOSURE,
	AE_SMOOTH_DEBUG_SWITCH_2_EXPOSURE,
	AE_SMOOTH_DEBUG_SKIP_SMOOTH,
};

struct _ae_smooth_cfg_str {
	enum _AE_SMOOTH_DEBUG_ENUM eSmoothDebug;
	enum _AE_EV_BASE_ENUM eEvBase;
	int aeCycle;

	float inverseProtectRatio;

	int smallRangeThd;
	int extremeOverPerframeRatio;
	int extremeOverNonlinearCwvRatioX[2];
	int extremeOverNonlinearCwvRatioY[2];

	int extremeUnderPerframeRatio;
	int extremeUnderNonlinearCwvRatioX[2];
	int extremeUnderNonlinearCwvRatioY[2];

	int overLimitDeltaEv;
	int overLvThreshold;
	int overHighLvPerframeRatio;
	int overLowLvPerframeRatio;
	int overHighNonLinearSpeedupRatioX[2];
	int overHighNonLinearSpeedupRatioY[2];
	int overProbRatioX[2];
	int overProbRatioY[2];
	int overNonlinearCwvRatioX[2];
	int overNonlinearCwvRatioY[2];
	int overNonlinearAvgRatioX[2];
	int overNonlinearAvgRatioY[2];

	int normalLvThreshold;
	int normalPerameRatioD2tOutdoor;
	int normalPerameRatioD2t;
	int normalPerameRatioB2tOutdoor;
	int normalPerameRatioB2t;
	int nomralNonLinearCwvRatioX[2];
	int nomralNonLinearCwvRatioY[2];
	int nomralNonLinearAvgRatioX[2];
	int nomralNonLinearAvgRatioY[2];
	int nomralUnderNonLinearRatioX[2];
	int nomralUnderNonLinearRatioY[2];
	int nomralMeterHsRatioX[2];
	int nomralMeterHsRatioY[2];
	int nomralMeterAblRatioX[2];
	int nomralMeterAblRatioY[2];

	//int hsLowLightPercentage;

	int inStableThd;
	int outStableThd;
};


struct _ae_hdr_cfg_str {
	//Meter
	int hdrBrightCutPcnt;
	int hdrTargetPcntByLvX[2];
	int hdrTargetPcntByLvY[2];
	int hdrExtremeBrightCutPcnt;
	int hdrCtrstDarkPrc;
	int hdrCtrstBrightPrc;

	int hdrCwtCtrstIntpX[2];
	int hdrCwtExtremeBrightIntpX[2];

	//Smooth
	float hdrInverseProtectRatio;
	int hdrSmallRangeThd;
	int hdrInStableThd;
	int hdrOutStableThd;
	int hdrNormalLvThreshold;
	int hdrNormalPerframeRatioD2tOutdoor;
	int hdrNormalPerframeRatioD2t;
	int hdrNormalPerframeRatioB2tOutdoor;
	int hdrNormalPerframeRatioB2t;
	int hdrNomralNonLinearCwvRatioX[2];
	int hdrNomralNonLinearCwvRatioY[2];
	int hdrNomralNonLinearAvgRatioX[2];
	int hdrNomralNonLinearAvgRatioY[2];
	int hdrNomralUnderNonLinearRatioX[2];
	int hdrNomralUnderNonLinearRatioY[2];

	int hdrOneShotPerframeRatioD2tOutdoor;
	int hdrOneShotPerframeRatioD2t;
	int hdrOneShotPerframeRatioB2tOutdoor;
	int hdrOneShotPerframeRatioB2t;

	float hdrDeltaEvRounding;
};


struct _ae_tuning_param_t {
	unsigned int version;
	int ae_set_point;

	//TOP LEVEL WEIGHT
	int ae_meter_spatial_target_weight;
	int ae_meter_hist_target_weight;

	int spa_evdiff_dark_ratio;
	int spa_evdiff_bright_ratio;
	int spa_weight_discount_lut_x[3];
	int spa_weight_discount_lut_y[3];
	int ae_hist_weight_discount_x[3];
	int ae_hist_weight_discount_y[3];


	//Hist weight
	int abl_weight_x[4];
	int abl_weight_y[4];

	int ce_weight_x[4];
	int ce_weight_y[4];

	int hs_weight_x[4];
	int hs_weight_y[4];


	//SP - Central target parameters
	int spa_central_lv_lut_x[3]; //LV
	int spa_central_lv_lut_y[3]; //Target

	//SP - Color sat parameters
	int color_sat_enable;
	int color_sat_target;
	int color_sat_lut_x[2]; //sat level (%)
	int color_sat_lut_y[2]; //weight

	//SP - Night scene parameters
	int ns_enable;
	int ns_target;
	int ns_prob_lv_lut_x[3]; //LV
	int ns_prob_lv_lut_y[3]; //Prob
	int ns_prob_lv_weight; //ns prob weight between LV and CDF
	int ns_cdf_lv_th;  //LV
	int ns_prob_cdf_lut_x[2];
	int ns_prob_cdf_lut_y[2];


	//Hist - ABL
	int abl_enable;
	int abl_lv_lut_x[4]; //LV
	int abl_lv_lut_y[4]; //Prob
	int abl_evdiff_lut_x[4]; //EVDiff
	int abl_evdiff_lut_y[4]; //Prob
	int abl_evdiff_bright_ratio;
	int abl_evdiff_dark_ratio;
	int abl_strength_backlight_percentage;
	int abl_strength_backlight_target;
	int abl_limit_gain;

	//Hist - CE
	int ce_enable;
	int ceBrightTh;
	int ce_prob_lut_bright_x[4];
	int ce_prob_lut_bright_y[4];
	int ce_prob_evdiff_dark_percentage;
	int ce_prob_evdiff_bright_percentage;
	int ce_prob_evdiff_lut_x[4];
	int ce_prob_evdiff_lut_y[4];
	int ce_strength_bright_percentage;
	int ce_strength_bright_target;



	//Hist - HS
	int hs_enable;
	int hs_oeest_lv_lut_x[4];
	int hs_oeest_lv_lut_y[4];
	int hs_oeest_oeratio_lut_x[4];
	int hs_oeest_oeratio_lut_y[4];
	int hs_highlight_percentage;
	int hs_highlight_target;
	int hs_dark_bin_lut_x[3]; //LV
	int hs_dark_bin_lut_y[3]; //Bound percentage
	int hs_gain_discount_lut_x[4]; //LV
	int hs_gain_discount_lut_y[4]; //Bound percentage


	//Face
	int face_enable;
	int face_shrink_ratio;
	int face_size_ratio_lut_x[4];
	int face_size_ratio_lut_y[4];
	int face_distance_lut_x[2];
	int face_distance_lut_y[2];
	int face_area_target_lv_lut_x[2];
	int face_area_target_lv_lut_y[2];
	int face_gain_discount_lut_x[4];
	int face_gain_discount_lut_y[4];
	int faceLimitGain;

	//Touch
	int touch_enable;
	int touchLimitGain;

	int touchTarget_lv_lut_x[4];
	int touchTarget_lv_lut_y[4];


	int touchWeight;

	//Weighting table
	int ae_weight_table[256];

	//EV Comp table
	int ev_compensation_table[61];

	// AE Smooth
	struct _ae_smooth_cfg_str ae_smooth_cfg;
#ifdef AE_SIMULATOR
	struct _ae_hdr_cfg_str ae_hdr_cfg;
#endif

};


const struct _ae_calibration_param_t *AeAlgoGetDefaultCalibData(void);
const struct _ae_tuning_param_t *AeAlgoGetDefaultTuningData(void);

#ifdef __cplusplus
}
#endif
