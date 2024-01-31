/*
 * Copyright (C) 2020 Advanced Micro Devices, Inc. All rights reserved.
 */

#pragma once

#include "os_base_type.h"
#include "SensorTdbDef.h"


enum _TdbStreamBinType_t {
	TDB_STREAM_BIN_TYPE_RGB = 0,
	TDB_STREAM_BIN_TYPE_IR  = 1
};

//Unified light index
enum _LightIndex_t {
	LIGHT_INDEX_INVALID            = -1,
	LIGHT_INDEX_H_2300K            = 0,
	LIGHT_INDEX_E27W_2700K         = 1,
	LIGHT_INDEX_A_2856K            = 2,
	LIGHT_INDEX_D29_2900K          = 3,
	LIGHT_INDEX_PC3_3000K          = 4,
	LIGHT_INDEX_D32_3200K          = 5,
	LIGHT_INDEX_CFL_3500K          = 6,
	LIGHT_INDEX_TL84_4000K         = 7,
	LIGHT_INDEX_CWF_4100K          = 8,
	LIGHT_INDEX_E27C_5000K_LED     = 9,
	LIGHT_INDEX_D50_5000K_DAYLIGHT = 10,
	LIGHT_INDEX_PC6_6000K          = 11,
	LIGHT_INDEX_D65_6500K          = 12,
	LIGHT_INDEX_D75_7500K          = 13,
	LIGHT_INDEX_SKYPEA_2538K       = 14,
	LIGHT_INDEX_LIGHT_15           = 15,
	LIGHT_INDEX_LIGHT_16           = 16,
	LIGHT_INDEX_LIGHT_17           = 17,
	LIGHT_INDEX_LIGHT_18           = 18,
	LIGHT_INDEX_LIGHT_19           = 19,
	LIGHT_INDEX_LIGHT_20           = 20,
	LIGHT_INDEX_LIGHT_21           = 21,
	LIGHT_INDEX_LIGHT_22           = 22,
	LIGHT_INDEX_LIGHT_23           = 23,
	LIGHT_INDEX_MAX
};


/**********************************************************/
/**
 * @brief   Matrix coefficients
 *
 *          | 0 |
 *
 * @note    Coefficients are represented as float numbers
 */
/********************************************************/
struct _Tdb1x1FloatMatrix_t {
	float fCoeff[1];
};


/**********************************************************/
/**
 * @brief   Matrix coefficients
 *
 *          | 0 | 1 | 2 |
 *
 * @note    Coefficients are represented as float numbers
 */
/**********************************************************/
struct _Tdb1x3FloatMatrix_t {
	float fCoeff[3];
};


/*************************************************************/
/**
 * @brief   Matrix coefficients
 *
 *          | 0 | 1 | ... | 4 |
 *
 * @note    Coefficients are represented as float numbers
 */
/************************************************************/
struct _Tdb1x4FloatMatrix_t {
	float fCoeff[4];
};


/***********************************************************/
/**
 * @brief   Matrix coefficients
 *
 *          | 0 | 1 |
 *
 * @note    Coefficients are represented as float numbers
 */
/************************************************************/
struct _Tdb2x1FloatMatrix_t {
	float fCoeff[2];
};


/*************************************************************/
/**
 * @brief   Matrix coefficients
 *
 *          | 0 | 1 |
 *          | 2 | 3 |
 *
 * @note    Coefficients are represented as float numbers
 */
/***************************************************************/
struct _Tdb2x2FloatMatrix_t {
	float fCoeff[4];
};


#ifndef ENABLE_ISP2P1_NEW_TUNING_DATA
/**************************************************************/
/**
 * @brief   Matrix coefficients
 *
 *          | 0 | 1 |  2 |
 *
 * @note    Coefficients are represented as float numbers
 */
/*************************************************************/
struct _Tdb3x1FloatMatrix_t {
	float fCoeff[3];
};
#endif


#ifndef ENABLE_ISP2P1_NEW_TUNING_DATA
/*************************************************************/
/**
 * @brief   Matrix coefficients
 *
 *          | 0 | 1 |  2 |
 *          | 3 | 4 |  5 |
 *
 * @note    Coefficients are represented as float numbers
 */
/*************************************************************/
struct _Tdb3x2FloatMatrix_t {
	float fCoeff[6];
};
#endif


/**************************************************************/
/**
 * @brief   Matrix coefficients
 *
 *          | 0 | 1 |  2 |
 *          | 3 | 4 |  5 |
 *          | 6 | 7 |  8 |
 *
 * @note    Coefficients are represented as float numbers
 */
/***************************************************************/
struct _Tdb3x3FloatMatrix_t {
	float fCoeff[9];
};


/**************************************************************/
/**
 * @brief   Matrix coefficients
 *
 *          | 0 | 1 | 2 | 3 |
 *
 * @note    Coefficients are represented as short numbers
 */
/*************************************************************/
struct _Tdb1x4UShortMatrix_t {
	unsigned short uCoeff[4];
};


/*************************************************************/
/**
 * @brief   Matrix coefficients
 *
 *          | 0 | 1 | 2 | ... | 15 |
 *
 * @note    Coefficients are represented as short numbers
 */
/*************************************************************/
struct _Tdb1x16UShortMatrix_t {
	unsigned short uCoeff[16];
};


/**************************************************************/
/**
 * @brief   Matrix coefficients
 *
 *          | 0 | 1 | 2 | ... | 16 |
 *
 * @note    Coefficients are represented as short numbers
 */
/***************************************************************/
struct _Tdb1x17UShortMatrix_t {
	unsigned short uCoeff[17];
};


/*************************************************************/
/**
 * @brief   Matrix coefficients
 *
 *          |   0 |   1 |   2 |   3 |   4 |   5 |   6 |   7 | ....
 *          |  17 |  18 |  19 |  20 |  21 |  22 |  23 |  24 | ....
 *          |  34 |  35 |  36 |  37 |  38 |  39 |  40 |  41 | ....
 *          ...
 *          ...
 *          ...
 *          | 271 | 272 | 273 | 274 | 275 | 276 | 277 | 278 | .... | 288 |
 *
 * @note    Coefficients are represented as short numbers
 */
/*************************************************************************/
struct _Tdb17x17UShortMatrix_t {
	unsigned short uCoeff[17 * 17];
};


#ifndef ENABLE_ISP2P1_NEW_TUNING_DATA
struct _TdbAwbClipParm_t {
	float   rg1[TDB_CONFIG_AWB_MAX_CURVE_POINTS];
	float   maxDist1[TDB_CONFIG_AWB_MAX_CURVE_POINTS];
	float   rg2[TDB_CONFIG_AWB_MAX_CURVE_POINTS];
	float   maxDist2[TDB_CONFIG_AWB_MAX_CURVE_POINTS];
};


struct _TdbAwbGlobalFadeParm_t {
	float   globalFade1[TDB_CONFIG_AWB_MAX_CURVE_POINTS];
	float   globalGainDistance1[TDB_CONFIG_AWB_MAX_CURVE_POINTS];
	float   globalFade2[TDB_CONFIG_AWB_MAX_CURVE_POINTS];
	float   globalGainDistance2[TDB_CONFIG_AWB_MAX_CURVE_POINTS];
};


struct _TdbAwbFade2Parm_t {
	float    fadef[TDB_CONFIG_AWB_MAX_FADE2_POINTS];
	float    cbMinRegionMax[TDB_CONFIG_AWB_MAX_FADE2_POINTS];
	float    crMinRegionMax[TDB_CONFIG_AWB_MAX_FADE2_POINTS];
	float    maxCSumRegionMax[TDB_CONFIG_AWB_MAX_FADE2_POINTS];
	float    cbMinRegionMin[TDB_CONFIG_AWB_MAX_FADE2_POINTS];
	float    crMinRegionMin[TDB_CONFIG_AWB_MAX_FADE2_POINTS];
	float    maxCSumRegionMin[TDB_CONFIG_AWB_MAX_FADE2_POINTS];
};
#endif


struct _TdbAwbIIR_t {
	/**< incrementer of damping coefficient */

	float    iirDampCoefAdd;
	/**< decrementer of damping coefficient */
	float    iirDampCoefSub;

	//< threshold for incrementing or
	// decrementing of damping coefficient
	float    iirDampFilterThreshold;
	/**< minmuim value of damping coefficient */
	float    iirDampingCoefMin;
	/**< maximum value of damping coefficient */

	/**< initial value of damping coefficient */
	float    iirDampingCoefMax;
	float    iirDampingCoefInit;
	/**< number of filter items */
	unsigned short   iirFilterSize;
	/**< initial value of the filter items */
	float    iirFilterInitValue;
};


#ifndef ENABLE_ISP2P1_NEW_TUNING_DATA
struct _TdbCenterLine_t {
	float f_N0_Rg; /**< Rg component of normal vector */
	float f_N0_Bg; /**< Bg component of normal vector */
	float f_d; /**< Distance of normal vector     */
};
#endif


#ifdef ENABLE_ISP2P1_NEW_TUNING_DATA
struct _TdbAwbWpRegionParm_t {
	unsigned char    maxY;
	unsigned char    refCrMaxR;
	unsigned char    minYMaxG;
	unsigned char    refCbMaxB;
	unsigned char    maxCSum;
	unsigned char    minC;
};
#endif


struct _TdbAwbGlobal_t {
#ifdef ENABLE_ISP2P1_NEW_TUNING_DATA
	float                   rgProjIndoorMin;
	float                   rgProjOutdoorMin;
	float                   rgProjMax;
	float                   rgProjMaxSky;

	struct _TdbAwbIIR_t             iir;

	struct _TdbAwbWpRegionParm_t    awbWpRegionInit;
	struct _TdbAwbWpRegionParm_t    awbWpRegionLowCCT;

	float                   indoorSensorGainThreshold;
	unsigned int                  awbConvFrameNum;
	unsigned int                  awbSceneCheckFrameNum;
	float                   awbSceneChangeWpMeanThreshold;
	float                   awbSceneChangeWpNumThreshold;
	unsigned int                  awbStepCntThreshold;
	unsigned int                  outdoorExposureThreshold;
#else
	struct _Tdb3x1FloatMatrix_t     svdMeanValue;
	struct _Tdb3x2FloatMatrix_t     pcaMatrix;
	struct _TdbCenterLine_t         centerLine;

	unsigned int                  validAwbCurvePoints;
	/**< clipping parameter in Rg/Bg space */
	struct _TdbAwbClipParm_t        awbClipParam;
	struct _TdbAwbGlobalFadeParm_t  awbGlobalFadeParam;
	struct _TdbAwbFade2Parm_t       awbFade2Param;

	float                   rgProjIndoorMin;
	float                   rgProjOutdoorMin;
	float                   rgProjMax;
	float                   rgProjMaxSky;

	//The index of the outdoor illumination.
	unsigned char outdoorClipIllu;

	float                   regionSize;
	float                   regionSizeInc;
	float                   regionSizeDec;

	struct _TdbAwbIIR_t             iir;
#endif
};

enum  _TdbAwbDoorType_t {
	TDB_AWB_DOOR_TYPE_OUTDOOR = 0,
	TDB_AWB_DOOR_TYPE_INDOOR  = 1
};




/* gaussian mixture modell */
struct _TdbAwbGMM_t {
	struct _Tdb2x1FloatMatrix_t    gaussMeanValue;
#ifndef ENABLE_ISP2P1_NEW_TUNING_DATA
	struct _Tdb2x2FloatMatrix_t    covarianceMatrix;
	struct _Tdb1x1FloatMatrix_t    gaussFactor;
	struct _Tdb2x1FloatMatrix_t    threshold;
#endif
};



struct _TdbAwbGain_t {
	/**< White Balance Gains*/
	struct _Tdb1x4FloatMatrix_t    componentGain;
};


struct _TdbAwbCc_t {
	/**< CrossTalk matrix coefficients */
	struct _Tdb3x3FloatMatrix_t    crossTalkCoeff;
	/**< CrossTalk offsets */
	struct _Tdb1x3FloatMatrix_t    crossTalkOffset;
};


enum _Tdb4ChColorComponent_t {
	TDB_4CH_COLOR_COMPONENT_RED     = 0,
	TDB_4CH_COLOR_COMPONENT_GREENR  = 1,
	TDB_4CH_COLOR_COMPONENT_GREENB  = 2,
	TDB_4CH_COLOR_COMPONENT_BLUE    = 3,
	TDB_4CH_COLOR_COMPONENT_MAX
};

struct _TdbAwbLsc_t {
	struct _Tdb17x17UShortMatrix_t
		lscMatrix[TDB_4CH_COLOR_COMPONENT_MAX];
};


struct _TdbAwbLscGlobal_t {
	unsigned short    lscXGradTbl[8];
	unsigned short    lscYGradTbl[8];
	unsigned short    lscXSizeTbl[16];
	unsigned short    lscYSizeTbl[16];
};


struct _TdbAwbSensorGain2Sat_t {
	float   sensorGain[TDB_CONFIG_AWB_MAX_SATS_FOR_SENSOR_GAIN];
	float   saturation[TDB_CONFIG_AWB_MAX_SATS_FOR_SENSOR_GAIN];
};


#ifndef ENABLE_ISP2P1_NEW_TUNING_DATA
struct _TdbAwbSensorGain2Vig_t {
	float   sensorGain[TDB_CONFIG_AWB_MAX_VIGS];
	float   vignetting[TDB_CONFIG_AWB_MAX_VIGS];
};
#endif


struct _TdbAwbSat2Cc_t {
	//1~TDB_CONFIG_AWB_MAX_SATS_FOR_CC
	unsigned char        satNum;
	//in descending order.
	float        saturation[TDB_CONFIG_AWB_MAX_SATS_FOR_CC];
	//in descending order.
	struct _TdbAwbCc_t   sat2Cc[TDB_CONFIG_AWB_MAX_SATS_FOR_CC];
};


#ifndef ENABLE_ISP2P1_NEW_TUNING_DATA
struct _TdbAwbVig2Lsc_t {
	//~TDB_CONFIG_AWB_MAX_VIGS
	unsigned char        vigNum;
	//in descending order.
	float        vignetting[TDB_CONFIG_AWB_MAX_VIGS];
	//in descending order.
	struct _TdbAwbLsc_t  vig2Lsc[TDB_CONFIG_AWB_MAX_VIGS];
};
#endif


struct _TdbAe_t {
	/**< set point to hit by the ae control system */
	float   setPoint;
	float   clmTolerance;
	/**< damping coefficient for still image mode */
	float   dampOverStill;
	/**< damping coefficient for still image mode */
	float   dampUnderStill;
	/**< damping coefficient for video mode */
	float   dampOverVideo;
	/**< damping coefficient for video mode */
	float   dampUnderVideo;
#ifdef ENABLE_ISP2P1_NEW_TUNING_DATA
	unsigned int  brightToDarkExpoTh;
	unsigned int  darkToBrightExpoTh;
#endif
};


/* Note that we have changed the BLS to 16 elements in ISP2.1 to
 * support 4x4 DIR!!! It must be set according to the current
 * CFA pattern. For example, for OV2744, the component must correspond
 * to following CFA pattern.
 * B  G  R  G
 * G  I  G  I
 * R  G  B  G
 * G  I  G  I
 * For 2X2 RGB-IR, only use the 1st 4 values
 */
#ifdef ENABLE_ISP2P1_NEW_TUNING_DATA
struct _TdbBls_t {
	/**< black level for all 16 color components */
	struct _Tdb1x16UShortMatrix_t level;
};
#else
struct _TdbBls_t {
	/**< black level for all 4 color components */
	struct _Tdb1x4UShortMatrix_t level;
};
#endif


struct _TdbDegamma_t {
	bool	  enabled;
	/**< x_i segment size */
	unsigned char     segment[TDB_DEGAMMA_CURVE_SIZE-1];
	unsigned short    red[TDB_DEGAMMA_CURVE_SIZE];
	unsigned short    green[TDB_DEGAMMA_CURVE_SIZE];
	unsigned short    blue[TDB_DEGAMMA_CURVE_SIZE];
	unsigned short    ir[TDB_DEGAMMA_CURVE_SIZE];
};


enum _TdbGammaCurveType_t {
	TDB_GAMMA_CURVE_TYPE_LOG  = 0,
	TDB_GAMMA_CURVE_TYPE_EQU  = 1
};


#ifdef ENABLE_ISP2P1_NEW_TUNING_DATA
struct _TdbGammaCurve_t {
	unsigned short    gammaY[TDB_GAMMA_CURVE_SIZE];
};
#endif


struct _TdbGamma_t {
	enum _TdbGammaCurveType_t curveType;
#ifdef ENABLE_ISP2P1_NEW_TUNING_DATA
	unsigned char activeGammaCurveNum;
	struct _TdbGammaCurve_t gammaCurve[TDB_GAMMA_CURVE_MAX_NUM];
	unsigned int exposureTheshold[TDB_GAMMA_CURVE_MAX_NUM - 1];
	float dampingCoef;
#else
	unsigned short gammaY[TDB_GAMMA_CURVE_SIZE];
#endif
};

struct _TdbWdr_t {
	unsigned short xVal[TDB_WDR_CURVE_SIZE];
	unsigned short yVal[TDB_WDR_CURVE_SIZE];
};


struct _TdbIspFilter_t {
	unsigned short denoiseFilter[TDB_ISP_FILTER_DENOISE_SIZE];
	//denoiseFilter[0] => STAGE1_SELECT
	//denoiseFilter[1] => FILT_H_MODE
	//denoiseFilter[2] => FILT_V_MODE
	//denoiseFilter[3] => ISP_FILTER_THRESH_SH0
	//denoiseFilter[4] => ISP_FILTER_THRESH_SH1
	//denoiseFilter[5] => ISP_FILTER_THRESH_BL0
	//denoiseFilter[6] => ISP_FILTER_THRESH_BL1

	unsigned char  sharpFilter[TDB_ISP_FILTER_SHARP_SIZE];
	//sharpFilter[0] => ISP_FILTER_FAC_SH0
	//sharpFilter[1] => ISP_FILTER_FAC_SH1
	//sharpFilter[2] => ISP_FILTER_FAC_MID
	//sharpFilter[3] => ISP_FILTER_FAC_BL0
	//sharpFilter[4] => ISP_FILTER_FAC_BL1
};


struct _TdbIspFilterDenoiseFilterTbl_t {
	unsigned short denoiseFilter[TDB_ISP_FILTER_DENOISE_SIZE];
	//denoiseFilter[0] => STAGE1_SELECT
	//denoiseFilter[1] => FILT_H_MODE
	//denoiseFilter[2] => FILT_V_MODE
	//denoiseFilter[3] => ISP_FILTER_THRESH_SH0
	//denoiseFilter[4] => ISP_FILTER_THRESH_SH1
	//denoiseFilter[5] => ISP_FILTER_THRESH_BL0
	//denoiseFilter[6] => ISP_FILTER_THRESH_BL1
};


struct _TdbIspFilterSharpFilterTbl_t {
	unsigned char  sharpFilter[TDB_ISP_FILTER_SHARP_SIZE];
	//sharpFilter[0] => ISP_FILTER_FAC_SH0
	//sharpFilter[1] => ISP_FILTER_FAC_SH1
	//sharpFilter[2] => ISP_FILTER_FAC_MID
	//sharpFilter[3] => ISP_FILTER_FAC_BL0
	//sharpFilter[4] => ISP_FILTER_FAC_BL1
};


struct _TdbIspFilterTuning_t {
	float aGain;
	float noiseReducLevel;
	float sharpLevel;
};


#define ISP_FILTER_MAX_TABLE_ENTRIES         10
struct _TdbIspFilterTuningTbl_t {
	bool enabled;
	int                  numEntries;
	struct _TdbIspFilterTuning_t
		entries[ISP_FILTER_MAX_TABLE_ENTRIES];
	struct _TdbIspFilter_t defaultIspFilter;
};


struct _TdbCac_t {
	unsigned char                 xNormShift;
	unsigned char                 xNormFactor;
	unsigned char                 yNormShift;
	unsigned char                 yNormFactor;
	/**< coeffciencts A, B and C for red */
	struct _Tdb1x3FloatMatrix_t   red;
	/**< coeffciencts A, B and C for blue */
	struct _Tdb1x3FloatMatrix_t   blue;

	short                 xOffset;
	short                 yOffset;
};

struct _TdbResolution_t {
	unsigned int width;
	unsigned int height;
};


enum _TdbDpfNllSegmentation_t {
	TDB_DPF_NLL_SEGMENTATION_EQU = 0,
	TDB_DPF_NLL_SEGMENTATION_LOG = 1
};


struct _TdbDpf_t {
	enum _TdbDpfNllSegmentation_t nllSegmentaion;
	struct _Tdb1x17UShortMatrix_t   nllCoeff;

	unsigned short                  sigmaGreen;
	unsigned short                  sigmaRedBlue;
	float                   gradient;
	float                   offset;
	struct _Tdb1x4FloatMatrix_t     nfGains;
};


struct _TdbDpcc_t {
	unsigned int   isp_dpcc_mode;
	unsigned int   isp_dpcc_output_mode;
	unsigned int   isp_dpcc_set_use;
	unsigned int   isp_dpcc_methods_set_1;
	unsigned int   isp_dpcc_methods_set_2;
	unsigned int   isp_dpcc_methods_set_3;
	unsigned int   isp_dpcc_line_thresh_1;
	unsigned int   isp_dpcc_line_mad_fac_1;
	unsigned int   isp_dpcc_pg_fac_1;
	unsigned int   isp_dpcc_rnd_thresh_1;
	unsigned int   isp_dpcc_rg_fac_1;
	unsigned int   isp_dpcc_line_thresh_2;
	unsigned int   isp_dpcc_line_mad_fac_2;
	unsigned int   isp_dpcc_pg_fac_2;
	unsigned int   isp_dpcc_rnd_thresh_2;
	unsigned int   isp_dpcc_rg_fac_2;
	unsigned int   isp_dpcc_line_thresh_3;
	unsigned int   isp_dpcc_line_mad_fac_3;
	unsigned int   isp_dpcc_pg_fac_3;
	unsigned int   isp_dpcc_rnd_thresh_3;
	unsigned int   isp_dpcc_rg_fac_3;
	unsigned int   isp_dpcc_ro_limits;
	unsigned int   isp_dpcc_rnd_offs;
};


enum _TdbSnrLutSegMode_t {
	TDB_SNR_LUT_SEG_MODE_LOG = 0,
	TDB_SNR_LUT_SEG_MODE_EQU = 1
};


struct _TdbSnr_t {
	unsigned char           gamma;
	//k, b are used to calculate the strength using the fomulat:
	//strength = sqrt((k * Gain) + b);
	float           k;
	float           b;
	unsigned char sigmaCoeff[TDB_SNR_SIGMA_COEFFS_SIZE];
	unsigned char           fupThrh;
	unsigned char           fupFacTh;
	unsigned char           fupFacTl;
	unsigned char           fudThrh;
	unsigned char           fudFacTh;
	unsigned char           fudFacTl;
	unsigned char           rsb;
};

struct _TdbTnr_t {
	unsigned short tnrConst[TDB_TNR_ALPHA_CONST_SIZE];
};


struct _TdbIrCropWindow_t {
	unsigned int hOffset;
	unsigned int vOffset;
	unsigned int hSize;
	unsigned int vSize;
};


struct _TdbRgbirAeWeight_t {
	float           rgbAeWeight;
	float           irAeWeight;
};


struct _TdbIsoFactor_t {
	//Factor to ISO100. Default is 1.0.
	float           factor;
};


#ifdef ENABLE_ISP2P1_NEW_TUNING_DATA
struct _TdbAwbProfile_t {
	char                    awbIlluMap;
	//Color temperature of every
	//light source in awbIlluMap[]. Should be put in ascending order.
	unsigned int                  colorTemp;
	bool                  useGmmReplaceMwbGain;
	bool                  idxIntplSpecial; // D50/PC6/E27C
	bool                  idxWpRegionDampSpecial; // A/E27W/PC3/SkypeA

	float                   sigmaA; //ellipse long axis covariance
	float                   sigmaB; //ellipse short axis covariance
	//ellipse long axis vector rotation angle cosine value
	float                   cosTheta;
	//ellipse long axis vector rotation angle sine value
	float                   sinTheta;
	float                   a99; //ellipse long axis length

	struct _TdbAwbGMM_t             awbGMM;
	struct _TdbAwbGain_t            awbGain;
	enum _TdbAwbDoorType_t        awbDoorType;

	struct _TdbAwbSensorGain2Sat_t  awbSensorGain2Sat;
	struct _TdbAwbSat2Cc_t          awbSat2Cc;

	struct _TdbAwbLsc_t             awbLsc;

	struct _TdbAwbWpRegionParm_t    awbWpRegion;
};
#else
struct _TdbAwbProfiler_t {
	char                    awbIlluMap;
	//Color temperature of every
	//light source in awbIlluMap[]. Should be put in ascending order.
	unsigned int                  colorTemp;
	struct _TdbAwbGMM_t             awbGMM;
	struct _TdbAwbGain_t            awbGain;
	enum _TdbAwbDoorType_t        awbDoorType;

	struct _TdbAwbSensorGain2Sat_t  awbSensorGain2Sat;
	struct _TdbAwbSat2Cc_t          awbSat2Cc;

	struct _TdbAwbSensorGain2Vig_t  awbSensorGain2Vig;
	struct _TdbAwbVig2Lsc_t         awbVig2Lsc;
};
#endif


#ifdef ENABLE_ISP2P1_NEW_TUNING_DATA
struct _TdbAwbIrProfile_t {
	unsigned int              exposureThreshold[2];
	struct _TdbAwbLsc_t         awbLsc;
};
#else
struct _TdbAwbIrProfiler_t {
	struct _TdbAwbSensorGain2Vig_t  awbSensorGain2Vig;
	struct _TdbAwbVig2Lsc_t         awbVig2Lsc;
};
#endif

struct _TdbAwbRgbData_t {
	struct _TdbAwbGlobal_t          awbGlobal;
	struct _TdbAwbLscGlobal_t       awbLscGlobal;
#ifdef ENABLE_ISP2P1_NEW_TUNING_DATA
	struct _TdbAwbProfile_t
		awbProfile[TDB_CONFIG_AWB_MAX_ILLU_PROFILES];
#else
	struct _TdbAwbProfiler_t
		awbProfiler[TDB_CONFIG_AWB_MAX_ILLU_PROFILES];
#endif
};


struct _TdbAwbIrData_t {
	//AWB tuning data for IR stream. Only LSC.
	struct _TdbAwbLscGlobal_t awbLscGlobal;
#ifdef ENABLE_ISP2P1_NEW_TUNING_DATA
	struct _TdbAwbIrProfile_t
		awbIrProfile[TDB_CONFIG_LSC_MAX_ILLU_PROFILES_FOR_IR];
#else
	struct _TdbAwbIrProfiler_t
		awbIrProfiler[TDB_CONFIG_LSC_MAX_ILLU_PROFILES_FOR_IR];
#endif
};


union _TdbAwbData_t {
	struct _TdbAwbRgbData_t rgb;
	struct _TdbAwbIrData_t ir;
};


struct _TdbAwb_t {
	//The illuminations contained for AWB. Should be less than
	//or equal to TDB_CONFIG_AWB_MAX_ILLU_PROFILES
	unsigned int illuNum;
	enum _TdbStreamBinType_t binType;
	union _TdbAwbData_t data;
};


struct _TdbSensorGainComp_t {
	bool enabled;
	float decayFactor;
};

enum _TdbIeSharpenYUVRange_t {
	TDB_IE_SHARPEN_YUV_RANGE_BT601 = 0,
	TDB_IE_SHARPEN_YUV_RANGE_FULL  = 1
};

enum _TdbSharpenYUVRange_t {
	TDB_SHARPEN_YUV_RANGE_BT601 = 0,
	TDB_SHARPEN_YUV_RANGE_FULL  = 1
};

enum _IeYUVRange_t {
	IE_YUV_RANGE_BT601 = 0,
	IE_YUV_RANGE_FULL  = 1
};


struct _TdbIeSharpen_t {
	bool isValid;
	enum _TdbIeSharpenYUVRange_t yuvRange;
	char coeff[9];
	unsigned char factor;
	unsigned char threshold;
};

struct _Sharpen_t {
	bool isValid;
	enum _TdbSharpenYUVRange_t yuvRange;
	char coeff[9];
	unsigned char factor;
	unsigned char threshold;
};

enum _IeMode_t {
	IE_MODE_GRAYSCALE,
	IE_MODE_NEGATIVE,
	IE_MODE_SEPIA,
	IE_MODE_COLOR_SELECTION,
	IE_MODE_EMBOSS,
	IE_MODE_SKETCH,
	IE_MODE_SHARPEN,
};

enum _IeColorSelection_t {
	IE_COLOR_SELECTION_INVALID = -1,
	IE_COLOR_SELECTION_RGB,
	IE_COLOR_SELECTION_B,
	IE_COLOR_SELECTION_G,
	IE_COLOR_SELECTION_GB,
	IE_COLOR_SELECTION_R,
	IE_COLOR_SELECTION_RB,
	IE_COLOR_SELECTION_RG,
	IE_COLOR_SELECTION_MAX,
};

struct _IeConfig_t {
	bool          useHwDefaultData;
	enum _IeMode_t        mode;
	union _ModeConfig_t {
		struct _Sepia_t {
			unsigned short TintCb;
			unsigned short TintCr;
		} Sepia;

		struct _ColorSelection_t {
			enum _IeColorSelection_t col_selection;
			unsigned short col_threshold;
		} ColorSelection;

		struct _Emboss_t {
			char coeff[9];
		} Emboss;

		struct _Sketch_t {
			char coeff[9];
			unsigned short thres_low;
			unsigned short thres_high;
		} Sketch;

		struct _IeSharpen_t {
			char coeff[9];
			unsigned char factor;
			unsigned short threshold;
		} Sharpen;
	} ModeConfig;
};

struct _TdbDitheringTable_t {
	bool isValid;
	unsigned int yTable[TDB_DITHERING_Y_TABLE_SIZE];
	unsigned int cTable[TDB_DITHERING_C_TABLE_SIZE];
};


struct _DitheringTable_t {
	unsigned int yTable[TDB_DITHERING_Y_TABLE_SIZE];
	unsigned int cTable[TDB_DITHERING_C_TABLE_SIZE];
};
