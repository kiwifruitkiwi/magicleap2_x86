/*
 * Copyright (C) 2019-2020 Advanced Micro Devices, Inc. All rights reserved.
 */

#pragma once

#include "os_base_type.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define AWB_ALGO_CALIB_VER 3001191126U
#define AWB_ALGO_TUNING_VER 3002201119U

#define AWB_ALGO_REGION_GROUP_NUM 5
#define AWB_ALGO_STYLE_SHIFT_LV_NUM 3
#define AWB_ALGO_STYLE_SHIFT_CT_NUM 8

enum _AwbLVType_t {
	AWB_LV_0 = 0,
	AWB_LV_1,
	AWB_LV_2,
	AWB_LV_3,
	AWB_LV_4,
	AWB_LV_5,
	AWB_LV_6,
	AWB_LV_7,
	AWB_LV_8,
	AWB_LV_9,
	AWB_LV_10,
	AWB_LV_11,
	AWB_LV_12,
	AWB_LV_13,
	AWB_LV_14,
	AWB_LV_15,
	AWB_LV_16,
	AWB_LV_17,
	AWB_LV_NUM,
};

enum _AwbRegionType_t {
	AWB_REGION_D50 = 0,
	AWB_REGION_D65,
	AWB_REGION_D75,
	AWB_REGION_SKY,
	AWB_REGION_SKIN,
	AWB_REGION_TL84,
	AWB_REGION_U30,
	AWB_REGION_A,
	AWB_REGION_HORIZON,
	AWB_REGION_CW,
	AWB_REGION_GRASS1,
	AWB_REGION_GRASS2,
	AWB_REGION_NUM,
};

enum _AwbRegionManualType_t {
	AWB_REGION_MANUAL_SHADE,
	AWB_REGION_MANUAL_CLOUDY,
	AWB_REGION_MANUAL_SUNLIGHT,
	AWB_REGION_MANUAL_SUNSET,
	AWB_REGION_MANUAL_FLH,
	AWB_REGION_MANUAL_FLM,
	AWB_REGION_MANUAL_FLL,
	AWB_REGION_MANUAL_INCANDESCENT,
	AWB_REGION_MANUAL_CANDLE,
	AWB_REGION_MANUAL_NUM,
};
enum _AwbCTType_t {
	AWB_CT_L = 0,
	AWB_CT_M,
	AWB_CT_H,
	AWB_CT_NUM,
};

enum _AwbLessWPMethod_t {
	AWB_LESS_WP_DEFAULT = 0,
	AWB_LESS_WP_HOLD,
	AWB_LESS_WP_NUM,
};

struct _AwbRegionPoint_t {
	unsigned short x;
	unsigned short y;
};

struct _AwbRegionCoordinate_t {
	struct _AwbRegionPoint_t point[4];
};

struct _AwbCalWBGain_t {
	unsigned short goldenD65Rgain;
	unsigned short goldenD65Bgain;
	unsigned short goldenCWRgain;
	unsigned short goldenCWBgain;
	unsigned short goldenAlightRgain;
	unsigned short goldenAlightBgain;
	unsigned short calD65Rgain;
	unsigned short calD65Bgain;
	unsigned short calCWRgain;
	unsigned short calCWBgain;
	unsigned short calAlightRgain;
	unsigned short calAlightBgain;
};

struct _AwbCalRegion_t {
	struct _AwbRegionCoordinate_t detectRegion;
	struct _AwbRegionCoordinate_t whitePointRegion;
	short ccm[9];
};

struct _AwbCalManualRegion_t {
	struct _AwbRegionCoordinate_t manualRegion;
	short ccm[9];
};


struct _AwbCalibData_t {
	unsigned int version;
	struct _AwbCalWBGain_t calWBGain;
	struct _AwbCalRegion_t region[AWB_REGION_NUM];
	struct _AwbCalManualRegion_t manualRegion[AWB_REGION_MANUAL_NUM];
};

struct _AwbRegionConfig_t {
	unsigned short weighting[AWB_LV_NUM];/*0~1024,256*/
	unsigned char rGainShift[AWB_LV_NUM];/*50~200,100*/
	unsigned char bGainShift[AWB_LV_NUM];/*50~200,100*/
	unsigned short colorTemperature;/*2000~10000,5000*/
};

struct _AwbManualRegionConfig_t {
	unsigned char validPercentage;/*1~100,100*/
	unsigned short colorTemperature;/*2000~10000,5000*/
};

struct _AwbRegionGroup_t {
	unsigned char highBound;/*1~100,1*/
	unsigned char lowBound;/*1~100,1*/
	unsigned char valid[AWB_REGION_NUM];/*0~1,1*/
};

struct _AwbDaylightShift_t {
	unsigned short lvx100HighBound;/*0~1700,0*/
	unsigned short lvx100LowBound;/*0~1700,0*/
	unsigned short strength;/*0~1024,0*/
	unsigned char valid[AWB_REGION_NUM];/*0~1,0*/
};

struct _AwbCCMShift_t {
	unsigned short lvx100HighBound;/*0~1700,0*/
	unsigned short lvx100LowBound;/*0~1700,0*/
	unsigned short strength;/*0~1024,1024*/
};
struct _AwbFaceShield_t {
	unsigned char rGainShift;/*50~200,100*/
	unsigned char bGainShift;/*50~200,100*/
	unsigned short weightingStrength;/*0~1024,1024*/
};

struct _AwbFacePredictbyCT_t {
	unsigned int CT;
	unsigned char rGainShift;/*50~200,100*/
	unsigned char bGainShift;/*50~200,100*/
};

struct _AwbFaceWB_t {
	unsigned char enable;
	struct _AwbFaceShield_t faceShield;
	struct _AwbFacePredictbyCT_t facePredict[AWB_CT_NUM];
	unsigned short faceAreaRatioLowBound;
	unsigned short faceAreaRatioHighBound;
	unsigned short faceWBStrength;/*0~1024,1024*/
};

struct _AwbDefault_t {
	unsigned char rGainShiftInitial;/*50~200,100*/
	unsigned char bGainShiftInitial;/*50~200,100*/
	unsigned char rGainShift[AWB_LV_NUM];/*50~200,100*/
	unsigned char bGainShift[AWB_LV_NUM];/*50~200,100*/
};

struct _AwbConfig_t {
	unsigned short statHighBound;/*0~65535,65535*/
	unsigned short statLowBound;/*0~65535,0*/
	unsigned char fullProcessPeriod;/*0~255,0*/
	unsigned char smoothFactor;/*0~100,20*/
	unsigned char directTargetCount; /*0~255,3*/
	enum _AwbLessWPMethod_t lessWPMethod;/*0*/
	unsigned char faceResizeRatioH;
	unsigned char faceResizeRatioV;
	unsigned char faceCountPlus;
	unsigned char faceCountMinus;
	unsigned char faceCountMaximum;
	unsigned char faceCountValid;
};

struct _AwbTuningData_t {
	unsigned int version;
	struct _AwbRegionConfig_t regionConfig[AWB_REGION_NUM];
	struct _AwbManualRegionConfig_t
	manualRegionConfig[AWB_REGION_MANUAL_NUM];
	struct _AwbRegionGroup_t regionGroup[AWB_ALGO_REGION_GROUP_NUM];
	struct _AwbDaylightShift_t daylightShift;
	struct _AwbCCMShift_t ccmShift;
	struct _AwbFaceWB_t faceWB;
	struct _AwbDefault_t defaultWB;
	struct _AwbConfig_t config;
};

const struct _AwbCalibData_t *AwbAlgoGetDefaultCalibData(void);
const struct _AwbTuningData_t *AwbAlgoGetDefaultTuningData(void);

#ifdef __cplusplus
}
#endif
