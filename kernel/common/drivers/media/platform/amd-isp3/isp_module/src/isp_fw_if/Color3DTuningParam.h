/*
 * Copyright (C) 2020 Advanced Micro Devices, Inc. All rights reserved.
 */

#pragma once

#include "os_base_type.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define COLOR_ALGO_TUNING_VER 3001200809U

#define PREFERENCE_NUM 6

enum _ColorIndexType_t {
	IDX_L = 0,
	IDX_M,
	IDX_H,
	IDX_NUM,
};

struct _GlobalSaturation_t {
	unsigned short sat1;
	unsigned short sat2;
	unsigned short sat3;
	unsigned short satTarget1;
	unsigned short satTarget2;
	unsigned short satTarget3;
};

struct _Preference_t {
	unsigned short hueS;
	unsigned short hueE;
	unsigned short hue1;
	unsigned short hue2;
	unsigned short hue3;
	unsigned short hueTarget1;
	unsigned short hueTarget2;
	unsigned short hueTarget3;
	unsigned short sat1;
	unsigned short sat2;
	unsigned short sat3;
	unsigned short satTarget1;
	unsigned short satTarget2;
	unsigned short satTarget3;
};

struct _PreferenceColor_t {
	struct _GlobalSaturation_t globalSaturation;
	struct _Preference_t preference[PREFERENCE_NUM];
};

struct _FaceParam_t {
	unsigned char enable;
	unsigned short faceAreaRatioLowBound;
	unsigned short faceAreaRatioHighBound;
};


struct _FaceColor_t {
	unsigned short hueValidS;
	unsigned short hueValidE;
	unsigned short hueTarget;
	unsigned short hueRange;
	unsigned short hueSmoothRange;
	unsigned short hueMaximumStep;
	unsigned short sat1;
	unsigned short sat2;
	unsigned short sat3;
	unsigned short satTarget1;
	unsigned short satTarget2;
	unsigned short satTarget3;
};

struct _LvCtIndex_t {
	unsigned int LV[IDX_NUM];
	unsigned int CT[IDX_NUM];
};

struct _ColorConfig_t {
	unsigned short statHighBound;/*0~65535,65535*/
	unsigned short statLowBound;/*0~65535,0*/
	unsigned char faceResizeRatioH;
	unsigned char faceResizeRatioV;
	unsigned char faceCountPlus;
	unsigned char faceCountMinus;
	unsigned char faceCountMaximum;
	unsigned char faceCountValid;
	unsigned char fullProcessPeriod;
	unsigned char directTargetCount;
	unsigned char smoothFactor;/*0~100,20*/
};

struct _ColorTuningData_t {
	unsigned int version;
	struct _LvCtIndex_t preferenceIndex;
	struct _PreferenceColor_t preferenceColor[IDX_NUM][IDX_NUM];//[LV][CT]
	struct _FaceParam_t faceParam;
	struct _LvCtIndex_t faceIndex;
	struct _FaceColor_t faceColor[IDX_NUM][IDX_NUM];//[LV][CT]
	struct _ColorConfig_t config;
};

#ifdef __cplusplus
}
#endif
