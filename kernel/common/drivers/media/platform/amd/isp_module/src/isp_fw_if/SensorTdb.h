#ifndef _SENSOR_TDB_H_
#define _SENSOR_TDB_H_

#include "SensorTdbDef.h"
#include "os_base_type.h"

typedef enum _TdbStreamBinType_t {
	TDB_STREAM_BIN_TYPE_RGB = 0,
	TDB_STREAM_BIN_TYPE_IR = 1
} TdbStreamBinType_t;

//Unified light index
typedef enum _LightIndex_t {
	LIGHT_INDEX_INVALID = -1,
	LIGHT_INDEX_H_2300K = 0,
	LIGHT_INDEX_E27W_2700K = 1,
	LIGHT_INDEX_A_2856K = 2,
	LIGHT_INDEX_D29_2900K = 3,
	LIGHT_INDEX_PC3_3000K = 4,
	LIGHT_INDEX_D32_3200K = 5,
	LIGHT_INDEX_CFL_3500K = 6,
	LIGHT_INDEX_TL84_4000K = 7,
	LIGHT_INDEX_CWF_4100K = 8,
	LIGHT_INDEX_E27C_5000K_LED = 9,
	LIGHT_INDEX_D50_5000K_DAYLIGHT = 10,
	LIGHT_INDEX_PC6_6000K = 11,
	LIGHT_INDEX_D65_6500K = 12,
	LIGHT_INDEX_D75_7500K = 13,
	LIGHT_INDEX_SKYPEA_2538K = 14,
	LIGHT_INDEX_LIGHT_15 = 15,
	LIGHT_INDEX_LIGHT_16 = 16,
	LIGHT_INDEX_LIGHT_17 = 17,
	LIGHT_INDEX_LIGHT_18 = 18,
	LIGHT_INDEX_LIGHT_19 = 19,
	LIGHT_INDEX_LIGHT_20 = 20,
	LIGHT_INDEX_LIGHT_21 = 21,
	LIGHT_INDEX_LIGHT_22 = 22,
	LIGHT_INDEX_LIGHT_23 = 23,
	LIGHT_INDEX_MAX
} LightIndex_t;


/*****************************************************************************/
/**
 * @brief   Matrix coefficients
 *
 *          | 0 |
 *
 * @note    Coefficients are represented as float numbers
 */
/*****************************************************************************/
typedef struct _Tdb1x1FloatMatrix_t {
	float fCoeff[1];
} Tdb1x1FloatMatrix_t;


/*****************************************************************************/
/**
 * @brief   Matrix coefficients
 *
 *          | 0 | 1 | 2 |
 *
 * @note    Coefficients are represented as float numbers
 */
/*****************************************************************************/
typedef struct _Tdb1x3FloatMatrix_t {
	float fCoeff[3];
} Tdb1x3FloatMatrix_t;


/*****************************************************************************/
/**
 * @brief   Matrix coefficients
 *
 *          | 0 | 1 | ... | 4 |
 *
 * @note    Coefficients are represented as float numbers
 */
/*****************************************************************************/
typedef struct _Tdb1x4FloatMatrix_t {
	float fCoeff[4];
} Tdb1x4FloatMatrix_t;


/*****************************************************************************/
/**
 * @brief   Matrix coefficients
 *
 *          | 0 | 1 |
 *
 * @note    Coefficients are represented as float numbers
 */
/*****************************************************************************/
typedef struct _Tdb2x1FloatMatrix_t {
	float fCoeff[2];
} Tdb2x1FloatMatrix_t;


/*****************************************************************************/
/**
 * @brief   Matrix coefficients
 *
 *          | 0 | 1 |
 *          | 2 | 3 |
 *
 * @note    Coefficients are represented as float numbers
 */
/*****************************************************************************/
typedef struct _Tdb2x2FloatMatrix_t {
	float fCoeff[4];
} Tdb2x2FloatMatrix_t;


/*****************************************************************************/
/**
 * @brief   Matrix coefficients
 *
 *          | 0 | 1 |  2 |
 *
 * @note    Coefficients are represented as float numbers
 */
/*****************************************************************************/
typedef struct _Tdb3x1FloatMatrix_t {
	float fCoeff[3];
} Tdb3x1FloatMatrix_t;


/*****************************************************************************/
/**
 * @brief   Matrix coefficients
 *
 *          | 0 | 1 |  2 |
 *          | 3 | 4 |  5 |
 *
 * @note    Coefficients are represented as float numbers
 */
/*****************************************************************************/
typedef struct _Tdb3x2FloatMatrix_t {
	float fCoeff[6];
} Tdb3x2FloatMatrix_t;


/*****************************************************************************/
/**
 * @brief   Matrix coefficients
 *
 *          | 0 | 1 |  2 |
 *          | 3 | 4 |  5 |
 *          | 6 | 7 |  8 |
 *
 * @note    Coefficients are represented as float numbers
 */
/*****************************************************************************/
typedef struct _Tdb3x3FloatMatrix_t {
	float fCoeff[9];
} Tdb3x3FloatMatrix_t;


/*****************************************************************************/
/**
 * @brief   Matrix coefficients
 *
 *          | 0 | 1 | 2 | ... | 4 |
 *
 * @note    Coefficients are represented as short numbers
 */
/*****************************************************************************/
typedef struct _Tdb1x4UShortMatrix_t {
	uint16 uCoeff[4];
} Tdb1x4UShortMatrix_t;


/*****************************************************************************/
/**
 * @brief   Matrix coefficients
 *
 *          | 0 | 1 | 2 | ... | 16 |
 *
 * @note    Coefficients are represented as short numbers
 */
/*****************************************************************************/
typedef struct _Tdb1x17UShortMatrix_t {
	uint16 uCoeff[17];
} Tdb1x17UShortMatrix_t;


/*****************************************************************************/
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
/*****************************************************************************/
typedef struct _Tdb17x17UShortMatrix_t {
	uint16 uCoeff[17 * 17];
} Tdb17x17UShortMatrix_t;


#ifndef SUPPORT_NEW_AWB

typedef struct _TdbAwbClipParm_t {
	float rg1[TDB_CONFIG_AWB_MAX_CURVE_POINTS];
	float maxDist1[TDB_CONFIG_AWB_MAX_CURVE_POINTS];
	float rg2[TDB_CONFIG_AWB_MAX_CURVE_POINTS];
	float maxDist2[TDB_CONFIG_AWB_MAX_CURVE_POINTS];
} TdbAwbClipParm_t;


typedef struct _TdbAwbGlobalFadeParm_t {
	float globalFade1[TDB_CONFIG_AWB_MAX_CURVE_POINTS];
	float globalGainDistance1[TDB_CONFIG_AWB_MAX_CURVE_POINTS];
	float globalFade2[TDB_CONFIG_AWB_MAX_CURVE_POINTS];
	float globalGainDistance2[TDB_CONFIG_AWB_MAX_CURVE_POINTS];
} TdbAwbGlobalFadeParm_t;


typedef struct _TdbAwbFade2Parm_t {
	float fadef[TDB_CONFIG_AWB_MAX_FADE2_POINTS];
	float cbMinRegionMax[TDB_CONFIG_AWB_MAX_FADE2_POINTS];
	float crMinRegionMax[TDB_CONFIG_AWB_MAX_FADE2_POINTS];
	float maxCSumRegionMax[TDB_CONFIG_AWB_MAX_FADE2_POINTS];
	float cbMinRegionMin[TDB_CONFIG_AWB_MAX_FADE2_POINTS];
	float crMinRegionMin[TDB_CONFIG_AWB_MAX_FADE2_POINTS];
	float maxCSumRegionMin[TDB_CONFIG_AWB_MAX_FADE2_POINTS];
} TdbAwbFade2Parm_t;

#endif
typedef struct _TdbAwbIIR_t {
	float iirDampCoefAdd;	/**< incrementer of damping coefficient */
	float iirDampCoefSub;	/**< decrementer of damping coefficient */
	//< threshold for incrementing or decrementing of damping coefficient
	float iirDampFilterThreshold;

	/**< minmuim value of damping coefficient */
	// for WpRegion A/E27W/PC3/SkypeA damping (0.01f)
	float iirDampingCoefMin;
	/**< maximum value of damping coefficient */
	 // for WpRegion currentUnion2or3 and PC6 damping (0.7f)
	float iirDampingCoefMax;
	/**< initial value of damping coefficient */
	// for all the normal damping (0.5f)
	float iirDampingCoefInit;

	uint16 iirFilterSize;	/**< number of filter items */
	float iirFilterInitValue;/**< initial value of the filter items */
} TdbAwbIIR_t;

#ifdef SUPPORT_NEW_AWB
typedef struct _TdbAwbWpRegionParm_t {
	uint8 maxY;
	uint8 refCrMaxR;
	uint8 minYMaxG;
	uint8 refCbMaxB;
	uint8 maxCSum;
	uint8 minC;
} TdbAwbWpRegionParm_t;
#else
typedef struct _TdbCenterLine_t {
	float f_N0_Rg; /**< Rg component of normal vector */
	float f_N0_Bg; /**< Bg component of normal vector */
	float f_d; /**< Distance of normal vector */
} TdbCenterLine_t;
#endif

typedef struct _TdbAwbGlobal_t {
#ifndef SUPPORT_NEW_AWB
	Tdb3x1FloatMatrix_t svdMeanValue;
	Tdb3x2FloatMatrix_t pcaMatrix;
	TdbCenterLine_t centerLine;

	uint32 validAwbCurvePoints;
	/**< clipping parameter in Rg/Bg space */
	TdbAwbClipParm_t awbClipParam;
	TdbAwbGlobalFadeParm_t awbGlobalFadeParam;
	TdbAwbFade2Parm_t awbFade2Param;
#endif
	float rgProjIndoorMin;
	float rgProjOutdoorMin;
	float rgProjMax;
	float rgProjMaxSky;
#ifndef SUPPORT_NEW_AWB
	//The index of the outdoor illumination.
	uint8 outdoorClipIllu;

	float regionSize;
	float regionSizeInc;
	float regionSizeDec;
#endif
	TdbAwbIIR_t iir;
#ifdef SUPPORT_NEW_AWB
	TdbAwbWpRegionParm_t awbWpRegionInit;
	TdbAwbWpRegionParm_t awbWpRegionLowCCT;

	float indoorSensorGainThreshold;
	uint32 awbConvFrameNum;
	uint32 awbSceneCheckFrameNum;
	float awbSceneChangeWpMeanThreshold;
	float awbSceneChangeWpNumThreshold;
	uint32 awbStepCntThreshold;
	uint32 outdoorExposureThreshold;
#ifdef CONFIG_ENABLE_WINSTON_NEW_TUNING_DATA
	float validWhiteCntRatio;
	uint32 minValidWhiteCnt;
#endif
#endif
} TdbAwbGlobal_t;

typedef enum _TdbAwbDoorType_t {
	TDB_AWB_DOOR_TYPE_OUTDOOR = 0,
	TDB_AWB_DOOR_TYPE_INDOOR = 1
} TdbAwbDoorType_t;




/*gaussian mixture modell */
typedef struct _TdbAwbGMM_t {
	Tdb2x1FloatMatrix_t gaussMeanValue;
#ifndef SUPPORT_NEW_AWB
	Tdb2x2FloatMatrix_t covarianceMatrix;
	Tdb1x1FloatMatrix_t gaussFactor;
	Tdb2x1FloatMatrix_t threshold;
#endif
} TdbAwbGMM_t;



typedef struct _TdbAwbGain_t {
	Tdb1x4FloatMatrix_t componentGain; /**< White Balance Gains*/
} TdbAwbGain_t;




typedef struct _TdbAwbCc_t {
	/**< CrossTalk matrix coefficients */
	Tdb3x3FloatMatrix_t crossTalkCoeff;
	Tdb1x3FloatMatrix_t crossTalkOffset; /**< CrossTalk offsets */
} TdbAwbCc_t;


typedef enum _Tdb4ChColorComponent_t {
	TDB_4CH_COLOR_COMPONENT_RED = 0,
	TDB_4CH_COLOR_COMPONENT_GREENR = 1,
	TDB_4CH_COLOR_COMPONENT_GREENB = 2,
	TDB_4CH_COLOR_COMPONENT_BLUE = 3,

	TDB_4CH_COLOR_COMPONENT_MAX
} Tdb4ChColorComponent_t;

typedef struct _TdbAwbLsc_t {
	Tdb17x17UShortMatrix_t lscMatrix[TDB_4CH_COLOR_COMPONENT_MAX];
} TdbAwbLsc_t;


typedef struct _TdbAwbLscGlobal_t {
	uint16 lscXGradTbl[8];
	uint16 lscYGradTbl[8];
	uint16 lscXSizeTbl[16];
	uint16 lscYSizeTbl[16];
} TdbAwbLscGlobal_t;


typedef struct _TdbAwbSensorGain2Sat_t {
	float sensorGain[TDB_CONFIG_AWB_MAX_SATS_FOR_SENSOR_GAIN];
	float saturation[TDB_CONFIG_AWB_MAX_SATS_FOR_SENSOR_GAIN];
} TdbAwbSensorGain2Sat_t;

#ifndef SUPPORT_NEW_AWB
typedef struct _TdbAwbSensorGain2Vig_t {
	float sensorGain[TDB_CONFIG_AWB_MAX_VIGS];
	float vignetting[TDB_CONFIG_AWB_MAX_VIGS];
} TdbAwbSensorGain2Vig_t;
#endif
typedef struct _TdbAwbSat2Cc_t {
	uint8 satNum; //1~TDB_CONFIG_AWB_MAX_SATS_FOR_CC
	//in descending order.
	float saturation[TDB_CONFIG_AWB_MAX_SATS_FOR_CC];
	//in descending order.
	TdbAwbCc_t sat2Cc[TDB_CONFIG_AWB_MAX_SATS_FOR_CC];
} TdbAwbSat2Cc_t;
#ifndef SUPPORT_NEW_AWB
typedef struct _TdbAwbVig2Lsc_t {
	uint8 vigNum; //1 ~TDB_CONFIG_AWB_MAX_VIGS
	float vignetting[TDB_CONFIG_AWB_MAX_VIGS]; //in descending order.
	TdbAwbLsc_t vig2Lsc[TDB_CONFIG_AWB_MAX_VIGS];//in descending order.
} TdbAwbVig2Lsc_t;
#endif
typedef struct _TdbAe_t {
	float setPoint;	/**< set point to hit by the ae control system */
	float clmTolerance;
	float dampOverStill; /**< damping coefficient for still image mode */
	float dampUnderStill;/**< damping coefficient for still image mode */
	float dampOverVideo; /**< damping coefficient for video mode */
	float dampUnderVideo;/**< damping coefficient for video mode */
#ifdef CONFIG_ENABLE_WINSTON_NEW_TUNING_DATA
	uint32 brightToDarkExpoTh;
	uint32 darkToBrightExpoTh;
#endif
} TdbAe_t;

typedef struct _TdbBls_t {
	/**< black level for all 4 color components */
	Tdb1x4UShortMatrix_t level;
} TdbBls_t;


typedef struct _TdbDegamma_t {
	uint8 segment[TDB_DEGAMMA_CURVE_SIZE-1];/**< x_i segment size */
	uint16 red[TDB_DEGAMMA_CURVE_SIZE];	/**< red point */
	uint16 green[TDB_DEGAMMA_CURVE_SIZE];/**< green point */
	uint16 blue[TDB_DEGAMMA_CURVE_SIZE]; /**< blue point */
} TdbDegamma_t;


typedef enum _TdbGammaCurveType_t {
	TDB_GAMMA_CURVE_TYPE_LOG = 0,
	TDB_GAMMA_CURVE_TYPE_EQU = 1
} TdbGammaCurveType_t;


#ifdef CONFIG_ENABLE_WINSTON_NEW_TUNING_DATA
typedef struct _TdbGammaCurve_t {
	uint16 gammaY[TDB_GAMMA_CURVE_SIZE];
} TdbGammaCurve_t;
#endif


typedef struct _TdbGamma_t {
	TdbGammaCurveType_t curveType;
#ifdef CONFIG_ENABLE_WINSTON_NEW_TUNING_DATA
	uint8 activeGammaCurveNum;
	TdbGammaCurve_t gammaCurve[TDB_GAMMA_CURVE_MAX_NUM];
	uint32 exposureTheshold[TDB_GAMMA_CURVE_MAX_NUM - 1];
	float dampingCoef;
#else
	uint16 gammaY[TDB_GAMMA_CURVE_SIZE];
#endif
} TdbGamma_t;


typedef struct _TdbWdr_t {
	uint16 xVal[TDB_WDR_CURVE_SIZE];
	uint16 yVal[TDB_WDR_CURVE_SIZE];
} TdbWdr_t;


typedef struct _TdbIspFilter_t {
	uint16 denoiseFilter[TDB_ISP_FILTER_DENOISE_SIZE];
	//denoiseFilter[0] => STAGE1_SELECT
	//denoiseFilter[1] => FILT_H_MODE
	//denoiseFilter[2] => FILT_V_MODE
	//denoiseFilter[3] => ISP_FILTER_THRESH_SH0
	//denoiseFilter[4] => ISP_FILTER_THRESH_SH1
	//denoiseFilter[5] => ISP_FILTER_THRESH_BL0
	//denoiseFilter[6] => ISP_FILTER_THRESH_BL1

	uint8 sharpFilter[TDB_ISP_FILTER_SHARP_SIZE];
	//sharpFilter[0] => ISP_FILTER_FAC_SH0
	//sharpFilter[1] => ISP_FILTER_FAC_SH1
	//sharpFilter[2] => ISP_FILTER_FAC_MID
	//sharpFilter[3] => ISP_FILTER_FAC_BL0
	//sharpFilter[4] => ISP_FILTER_FAC_BL1
} TdbIspFilter_t;


typedef struct _TdbIspFilterDenoiseFilterTbl_t {
	uint16 denoiseFilter[TDB_ISP_FILTER_DENOISE_SIZE];
	//denoiseFilter[0] => STAGE1_SELECT
	//denoiseFilter[1] => FILT_H_MODE
	//denoiseFilter[2] => FILT_V_MODE
	//denoiseFilter[3] => ISP_FILTER_THRESH_SH0
	//denoiseFilter[4] => ISP_FILTER_THRESH_SH1
	//denoiseFilter[5] => ISP_FILTER_THRESH_BL0
	//denoiseFilter[6] => ISP_FILTER_THRESH_BL1
} TdbIspFilterDenoiseFilterTbl_t;


typedef struct _TdbIspFilterSharpFilterTbl_t {
	uint8 sharpFilter[TDB_ISP_FILTER_SHARP_SIZE];
	//sharpFilter[0] => ISP_FILTER_FAC_SH0
	//sharpFilter[1] => ISP_FILTER_FAC_SH1
	//sharpFilter[2] => ISP_FILTER_FAC_MID
	//sharpFilter[3] => ISP_FILTER_FAC_BL0
	//sharpFilter[4] => ISP_FILTER_FAC_BL1
} TdbIspFilterSharpFilterTbl_t;


typedef struct _TdbIspFilterTuning_t {
	float aGain;
	float noiseReducLevel;
	float sharpLevel;
} TdbIspFilterTuning_t;


#define ISP_FILTER_MAX_TABLE_ENTRIES		10
typedef struct _TdbIspFilterTuningTbl_t {
	bool_t enabled;
	int numEntries;
	TdbIspFilterTuning_t entries[ISP_FILTER_MAX_TABLE_ENTRIES];
	TdbIspFilter_t defaultIspFilter;
} TdbIspFilterTuningTbl_t;


typedef struct _TdbCac_t {
	uint8 xNormShift;
	uint8 xNormFactor;
	uint8 yNormShift;
	uint8 yNormFactor;

	Tdb1x3FloatMatrix_t red;/**< coeffciencts A, B and C for red */
	Tdb1x3FloatMatrix_t blue;/**< coeffciencts A, B and C for blue */

	int16 xOffset;
	int16 yOffset;
} TdbCac_t;


typedef struct _TdbResolution_t {
	uint32 width;
	uint32 height;
} TdbResolution_t;


typedef enum _TdbDpfNllSegmentation_t {
	TDB_DPF_NLL_SEGMENTATION_EQU = 0,
	TDB_DPF_NLL_SEGMENTATION_LOG = 1
} TdbDpfNllSegmentation_t;


typedef struct _TdbDpf_t {
	TdbDpfNllSegmentation_t nllSegmentaion;
	Tdb1x17UShortMatrix_t nllCoeff;

	uint16 sigmaGreen;
	uint16 sigmaRedBlue;
	float gradient;
	float offset;
	Tdb1x4FloatMatrix_t nfGains;
} TdbDpf_t;


typedef struct _TdbDpcc_t {
	uint32 isp_dpcc_mode;
	uint32 isp_dpcc_output_mode;
	uint32 isp_dpcc_set_use;
	uint32 isp_dpcc_methods_set_1;
	uint32 isp_dpcc_methods_set_2;
	uint32 isp_dpcc_methods_set_3;
	uint32 isp_dpcc_line_thresh_1;
	uint32 isp_dpcc_line_mad_fac_1;
	uint32 isp_dpcc_pg_fac_1;
	uint32 isp_dpcc_rnd_thresh_1;
	uint32 isp_dpcc_rg_fac_1;
	uint32 isp_dpcc_line_thresh_2;
	uint32 isp_dpcc_line_mad_fac_2;
	uint32 isp_dpcc_pg_fac_2;
	uint32 isp_dpcc_rnd_thresh_2;
	uint32 isp_dpcc_rg_fac_2;
	uint32 isp_dpcc_line_thresh_3;
	uint32 isp_dpcc_line_mad_fac_3;
	uint32 isp_dpcc_pg_fac_3;
	uint32 isp_dpcc_rnd_thresh_3;
	uint32 isp_dpcc_rg_fac_3;
	uint32 isp_dpcc_ro_limits;
	uint32 isp_dpcc_rnd_offs;
} TdbDpcc_t;


typedef enum _TdbSnrLutSegMode_t {
	TDB_SNR_LUT_SEG_MODE_LOG = 0,
	TDB_SNR_LUT_SEG_MODE_EQU = 1
} TdbSnrLutSegMode_t;


typedef struct _TdbStnr_t {
	uint8 gamma;
	//k, b are used to calculate the strength using the fomulat:
	//strength = sqrt((k *Gain) + b);
	float k;
	float b;
	uint8 sigmaCoeff[TDB_SNR_SIGMA_COEFFS_SIZE];
	uint8 fupThrh;
	uint8 fupFacTh;
	uint8 fupFacTl;
	uint8 fudThrh;
	uint8 fudFacTh;
	uint8 fudFacTl;
	uint8 rsb;
	uint16 tnrConst[TDB_TNR_ALPHA_CONST_SIZE];
} TdbStnr_t;


typedef struct _TdbIrCropWindow_t {
	uint32 hOffset;
	uint32 vOffset;
	uint32 hSize;
	uint32 vSize;
} TdbIrCropWindow_t;


typedef struct _TdbRgbirAeWeight_t {
	float rgbAeWeight;
	float irAeWeight;
} TdbRgbirAeWeight_t;


typedef struct _TdbIsoFactor_t {
	float factor; //Factor to ISO100. Default is 1.0.
} TdbIsoFactor_t;

#ifdef SUPPORT_NEW_AWB
typedef struct _TdbAwbProfile_t
#else
typedef struct _TdbAwbProfiler_t
#endif
{
	int8 awbIlluMap;
	//Color temperature of every light source in
	//awbIlluMap[]. Should be put in ascending order.
	uint32 colorTemp;
#ifdef SUPPORT_NEW_AWB
	bool_t useGmmReplaceMwbGain;
	bool_t idxIntplSpecial; // D50/PC6/E27C
	bool_t idxWpRegionDampSpecial; // A/E27W/PC3/SkypeA

	float sigmaA; //ellipse long axis covariance
	float sigmaB; //ellipse short axis covariance
	//ellipse long axis vector rotation angle cosine value
	float cosTheta;
	//ellipse long axis vector rotation angle sine value
	float sinTheta;
	float a99; //ellipse long axis length
#endif
	TdbAwbGMM_t awbGMM;
	TdbAwbGain_t awbGain;
	TdbAwbDoorType_t awbDoorType;

	TdbAwbSensorGain2Sat_t awbSensorGain2Sat;
	TdbAwbSat2Cc_t awbSat2Cc;
#ifdef SUPPORT_NEW_AWB
	TdbAwbLsc_t awbLsc;

	TdbAwbWpRegionParm_t awbWpRegion;
} TdbAwbProfile_t;
#else
	TdbAwbSensorGain2Vig_t awbSensorGain2Vig;
	TdbAwbVig2Lsc_t awbVig2Lsc;
} TdbAwbProfiler_t;
#endif

#ifdef SUPPORT_NEW_AWB
typedef struct _TdbAwbIrProfile_t {
	uint32 exposureThreshold[2];
	TdbAwbLsc_t awbLsc;
} TdbAwbIrProfile_t;
#else
typedef struct _TdbAwbIrProfiler_t {
	TdbAwbSensorGain2Vig_t awbSensorGain2Vig;
	TdbAwbVig2Lsc_t awbVig2Lsc;
} TdbAwbIrProfiler_t;
#endif


typedef struct _TdbAwbRgbData_t {
	TdbAwbGlobal_t awbGlobal;
	TdbAwbLscGlobal_t awbLscGlobal;
#ifdef SUPPORT_NEW_AWB
	TdbAwbProfile_t awbProfile[TDB_CONFIG_AWB_MAX_ILLU_PROFILES];
#else
	TdbAwbProfiler_t awbProfiler[TDB_CONFIG_AWB_MAX_ILLU_PROFILES];
#endif

} TdbAwbRgbData_t;

typedef struct _TdbAwbIrData_t {
	//AWB tuning data for IR stream. Only LSC.
	TdbAwbLscGlobal_t awbLscGlobal;
#ifdef SUPPORT_NEW_AWB
	TdbAwbIrProfile_t
		awbIrProfile[TDB_CONFIG_LSC_MAX_ILLU_PROFILES_FOR_IR];
#else
	TdbAwbIrProfiler_t
		awbIrProfiler[TDB_CONFIG_LSC_MAX_ILLU_PROFILES_FOR_IR];
#endif
} TdbAwbIrData_t;

typedef union _TdbAwbData_t {
	TdbAwbRgbData_t rgb;
	TdbAwbIrData_t ir;
} TdbAwbData_t;

typedef struct _TdbAwb_t {
	//The illuminations contained for AWB. Should be less than
	uint32 illuNum;
	//or equal to TDB_CONFIG_AWB_MAX_ILLU_PROFILES
	TdbStreamBinType_t binType;
	TdbAwbData_t data;
} TdbAwb_t;

typedef struct _TdbSensorGainComp_t {
	bool_t enabled;
	float decayFactor;
} TdbSensorGainComp_t;

typedef struct _SensorTdb_t {
	TdbResolution_t resolution;
	TdbStnr_t stnr;
	TdbBls_t bls;
	TdbDegamma_t degamma;
	TdbDpcc_t dpcc;
	TdbDpf_t dpf;
	TdbCac_t cac;
	TdbIspFilterTuningTbl_t ispFilterTuning;
	TdbWdr_t wdr;
	TdbGamma_t gamma;

	TdbAe_t ae;
	TdbIsoFactor_t isoFactor;
	TdbSensorGainComp_t sensorGainComp;

	TdbIrCropWindow_t irCropWindow;

	TdbRgbirAeWeight_t aeWeight;

	TdbAwb_t awb;
	uint32 reserved[10];
} SensorTdb_t;


#endif
