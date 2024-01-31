/*
 * Copyright (C) 2019-2020 Advanced Micro Devices, Inc. All rights reserved.
 */

#pragma once
#include "os_base_type.h"

#ifdef __cplusplus
extern "C" {
#endif

#define IRIDIX_ALGO_CALIB_VER 3001200311U
#define IRIDIX_ALGO_TUNING_VER 3001201119U

struct _IridixTuningData_t {
	unsigned int version;
	// statistic related
	int drkPrc;
	int mdkPrc;
	int briPrc;
	int drkTarget;
	int mdkTarget;

	// strength
	int strMin;
	int strMax;
	int strCtrMin;
	int strCtrMax;
	int strLvMin;
	int strLvMax;

	// darken
	int dkenMin;
	int dkenMax;
	int dcutMin;
	int dcutMax;
	int dkenCtrMin;
	int dkenCtrMax;
	int mkenCtrMin;
	int mkenCtrMax;
	int dkenLvMin;
	int dkenLvMax;
	int dkenFdtRatioX1;
	int dkenFdtRatioX2;
	int dkenFaceRatioY1;
	int dkenFaceRatioY2;

	// contrast
	int contFdtRatioX1;
	int contFdtRatioX2;
	int contFaceY1;
	int contFaceY2;

	// brightpr
	int bprFdtRatioX1;
	int bprFdtRatioX2;
	int bprFdtRatioY1;
	int bprFdtRatioY2;
	int bprDkenX1;
	int bprDkenX2;
	int bprDkenY1;
	int bprDkenY2;

	// wlv/blv
	int blackLevelLvX1;
	int blackLevelLvX2;
	int blackLevelLvY1;
	int blackLevelLvY2;
	int whiteLevelLvX1;
	int whiteLevelLvX2;
	int whiteLevelLvY1;
	int whiteLevelLvY2;
	int whiteLevelFaceX1;
	int whiteLevelFaceX2;
	int whiteLevelFaceY1;
	int whiteLevelFaceY2;

	//  others
	int iridixMaxGain;
	int targetAe;
	int globalDigitalGainEn;

	// smooth
	int smoothOn;
	int stableRangeStrIn;
	int stableRangeStrOut;
	int stableRangeDkenIn;
	int stableRangeDkenOut;
	int stableRangeContIn;
	int stableRangeContOut;
	int stableRangeBprIn;
	int stableRangeBprOut;
	int stableRangeWlvIn;
	int stableRangeWlvOut;
	int stableRangeBlvIn;
	int stableRangeBlvOut;
	int deltaEvX1;
	int deltaEvX2;
	int movingRatioY1;
	int movingRatioY2;

	// default
	int defaultStr;
	int defaultDken;
	int defaultCont;
	int defaultBpr;
	int defaultWlv;
	int defaultBlv;
	// v32 switch
	int algFixDReg;
};

struct _IridixCalibData_t {
	unsigned int version;
	int iridixLut[65];
};

#ifdef IRIDIX_SIMULATOR
struct _IridixCalibData_t
		*IridixAlgoGetDefaultCalibData(void);
struct _IridixTuningData_t
		*IridixAlgoGetDefaultTuningData(void);
#else
const struct _IridixCalibData_t
		*IridixAlgoGetDefaultCalibData(void);
const struct _IridixTuningData_t
		*IridixAlgoGetDefaultTuningData(void);
#endif


#ifdef __cplusplus
	}
#endif
