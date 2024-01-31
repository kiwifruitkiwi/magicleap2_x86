/*
 * Copyright (C) 2019-2021 Advanced Micro Devices, Inc. All rights reserved.
 */

#pragma once

#include "os_base_type.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define AF_ALGO_CALIB_VER 3001200312U
#define AF_ALGO_TUNING_VER 3001200824U

struct _MotorStepParam_t {
	int motorStep;
	int motorSpeed;
};

#define MOTOR_STEP_TABLE_NUM_MAX 10
struct _LensCalibrationParam_t {
	unsigned int version;

	int lensFarMechLimit;
	int lensNearMechLimit;
	int lensFarLimit;
	int lensNearLimit;
	int initFocusPoint;
	int hyperFocusPoint;
	int macroFocusPoint;
	int motorSpeedMax;
	int focusTotalRange;

	int motorStepTableNum;
	struct _MotorStepParam_t
		motorSpeedTab[MOTOR_STEP_TABLE_NUM_MAX];

	int calibTemperature;
};

struct _AfSceneChangeConfig_t {
	int afLPFCoefficient4Primary;
	int afLPFCoefficient4Previous;
	int afLPFCoefficient4Background;
	int binObjectInstThresh;
	int objectSizeThresh;
	int sceneSteadyThresh;
	int movingDiffThresh;
	int movingObjetSizeThresh;
	int movingStableThresh;
#ifdef NEXT_VER
	int  focusLocationChangeNumber4Moving;
	char  sceneChangeDetectType;
	int  sceneChangeLvNumberThresh;
	int  objDiffThresh;
	int  focusLocationPriority;
	int  focusLocationChangeNumber;
#endif

};

struct _AfExtRangeFocus_t {
	int extRangeFarFull;
	int extRangeNearFull;
	int extRangeFarNormal;
	int extRangeNearNormal;
	int extRangeFarMacro;
	int extRangeNearMacro;
	int extRangeInf;
	int extRangeHyper;
};

struct _AfExtRangeROI_t {
	unsigned int extRangeTop;
	unsigned int extRangeBottom;
	unsigned int extRangeLeft;
	unsigned int extRangeRight;
};

struct _AfROITuning_t {
	unsigned int ROINumThresh;
	unsigned int ROIAreaThresh;
	struct _AfExtRangeROI_t extROI;
};

struct _AfSearchTuning_t {
	int intervalThresh;
	unsigned int afvReturnThreshPercent;
	unsigned int afvReturnThreshUpper;
	unsigned int afvReturnThreshLower;
	int stepSize;
	int stepSizeAweight;
	int stepSizeBweight;
	int stepSizeCweight;
	char enableSpeedCtrl;
	int finalSpeed;
	int returnSpeed;
	int runSpeed;
	unsigned int searchRange;
	int initialSearchFocusPosOffset;
	int initialSearchFocusDir;
	int wobblingNumThresh;
};

#define AFV_MIN_TABLE_NUM_MAX 12
struct _AfTuningConfig_t {
	unsigned int afVer;
	//ISO100/150/200/300/400/600/800/1600/3200/6400/12800
	unsigned long long afvMin[AFV_MIN_TABLE_NUM_MAX];
	int  afStatisticDelayConfig;
	unsigned short afStatShift;
	unsigned long long afStatBias4BroadParam;
	unsigned long long afStatBias4NarrowParam;
	unsigned long long afvAlpha;
	unsigned long long lum4Alpha;
	unsigned int afSceneChangeThreshPercent;
#ifdef NEXT_VER
	int  afSceneChangeLvNumberThresh;
	int  afSceneChangeAgcGainThresh;
	int  afSceneChangeiTimeThresh;
#endif
	int  afSceneChangeIspGainThresh;
	int  afSceneChangeDGainThresh;
	unsigned int afMonoRiseThreshPercent;
	unsigned short afStatDelaytoSearching;
	int  initialFocusPos;
	int  initialFocusDir;
	int  focusCompensatedStep;
	int  focusFinalDir;
#ifdef NEXT_VER
	int extendFocusSearch4Near;
	int extendFocusSearch4Far;
	char  afWatchStateEnable;
#endif

	struct _AfROITuning_t touchRoiParm;
	struct _AfROITuning_t faceRoiParm;
	unsigned char afStartFrameDelay;
	struct _AfSearchTuning_t searchStage1Parm;
	struct _AfSearchTuning_t searchStage2Parm;
	struct _AfSearchTuning_t searchStage3Parm;
	struct _AfSearchTuning_t searchStage4Parm;
	struct _AfSceneChangeConfig_t sceneChangeParam;
	int  noPeakLocation;
};

#ifdef NEXT_VER
struct _AfCalibProcParam_t {
	int  param[32];
};
#endif

struct _AfTuningUserParam_t {
	unsigned int version;
	struct _AfTuningConfig_t conAfConfig;
	struct _AfTuningConfig_t oneShotAfConfig;
	struct _AfExtRangeFocus_t  extRangeFocusParam;
#ifdef NEXT_VER
	struct _AfCalibProcParam_t	afCalibProcParam;
	int reservedData[96];
#else
	int reservedData[128];
#endif

};

const struct _LensCalibrationParam_t *AfAlgoGetDefaultCalibData(void);
const struct _AfTuningUserParam_t *AfAlgoGetDefaultTuningData(void);

#ifdef __cplusplus
}
#endif
