/*
 * Copyright (C) 2019-2020 Advanced Micro Devices, Inc. All rights reserved.
 */

#pragma once

#include <os_base_type.h>
#include <SensorTdbDef.h>

#include "AeTuningParam.h"
#include "AwbTuningParam.h"
#include "PLineTuningParam.h"

// Pre-pipe calibration data starts here
struct _RgbChannel16Bits_t {
	unsigned short R;
	unsigned short G;
	unsigned short B;
};

struct _RgbChannel32Bits_t {
	unsigned int R;
	unsigned int G;
	unsigned int B;
};

struct _RgbChannel8Bits_t {
	unsigned char R;
	unsigned char G;
	unsigned char B;
};

// already defined in SensorTdb.h
#ifdef NOT_DELETED
/* Note that we have changed the BLS to 16 elements to
 * support 4x4 DIR!!! It must be set according to the current
 * CFA pattern. For example, for OV2744, the component must
 * correspond to following CFA pattern.
 * B  G  R  G
 * G  I  G  I
 * R  G  B  G
 * G  I  G  I
 * For 2X2 RGB-IR, only use the 1st 4 values
 */
struct _TdbBls_t {
	/**< black level for all 16 color components */
	struct _Tdb1x16UShortMatrix_t level;
};
#endif

struct _TdbBlsMeasure_t {
	/**< black level for all 16 color components */
	struct _Tdb1x16UShortMatrix_t level;
	unsigned short blMeasureNoExp;	//bl measure number exp components
	unsigned short blWin1StartX;		//window1 start x position
	unsigned short blWin1StartY;		//window1 start y position
	unsigned short blWin2StartX;		//window2 start x position
	unsigned short blWin2StartY;		//window1 start y position
	unsigned short blWin1SizeX;		//window1 x size
	unsigned short blWin1SizeY;		//window1 y size
	unsigned short blWin2SizeX;		//window2 x size
	unsigned short blWin2SizeY;		//window2 y size
};

// already defined in SensorTdb.h
#ifdef NOT_DELETED
struct _TdbDegamma_t {
	unsigned char     segment[TDB_DEGAMMA_CURVE_SIZE-1];
	/**< x_i segment size */
	unsigned short    red[TDB_DEGAMMA_CURVE_SIZE];        /**< red point */
	unsigned short    green[TDB_DEGAMMA_CURVE_SIZE];     /**< green point */
	unsigned short    blue[TDB_DEGAMMA_CURVE_SIZE];       /**< blue point */
	unsigned short    ir[TDB_DEGAMMA_CURVE_SIZE];       /**< ir point */
};
#endif

#ifdef NOT_DELETED
struct _TdbDir_t {
	/*
	 * All unsigned short value is bitdepth = 16 equivalent,
	 * these values need not shift.
	 * 10-bit RAW image will be left shift 2 bits by hardware.
	 */

	/* Bad Pixel */
	short DPU_TH;
	short DPA_TH;

	/* Saturation */
	unsigned short RGB_TH;
	unsigned short IR_TH;

	/* Decontamination */
	float r_dir_coeff[9];
	float g_dir_coeff[9];
	float b_dir_coeff[9];
	float rgb2ir_coeff_r;
	float rgb2ir_coeff_g;
	float rgb2ir_coeff_b;

	/* RGB Adjust */
	unsigned short RGB_UTH;
	unsigned short RB_LTH;
	float CompRatio;
	float CutRatio;
	float ARC;
	float AGC;
	float ABC;
	float ARFC;
	float AGFC;
	float ABFC;

	/* Tolerance factor for interpolation */
	int G_TF1;
	int R_TF1;
	int I_TF1;

	/* Color difference threshold for interpolation */
	int CDATH;
	int CDUTH;
};
#endif

struct _TdbDir_t {
	//Bad Pixel
	short dpuTh;
	short dpaTh;

	//Saturation
	unsigned short rgbTh;
	unsigned short irTh;
	unsigned char  diff;

	//Decontamination
	unsigned short rCoeff[9];
	unsigned short gCoeff[9];
	unsigned short bCoeff[9];
	unsigned short fgb2irRCoeff;
	unsigned short fgb2irGCoeff;
	unsigned short fgb2irBCoeff;

	//RGB Adjust
	unsigned short rgbUth;
	unsigned short rbLth;
	unsigned char compRatio;
	unsigned char cutRatio;
	unsigned short arc;
	unsigned short agc;
	unsigned short abc;
	unsigned short arfc;
	unsigned short agfc;
	unsigned short abfc;

	//Tolerance factor for 4x4 interpolation
	unsigned char rbTf;
	unsigned char gTf;
	unsigned char iTf;

	//Color difference threshold for 4x4 interpolation
	short cdAth;
	short cdUth;
};

struct _TdbPde_t {
	unsigned short pdStaXpos;
	unsigned short pdStaYpos;
	unsigned short pdEndXpos;
	unsigned short pdEndYpos;
	unsigned short pdRepeatWidth;
	unsigned short pdRepeatHeight;
	unsigned short pdNumOneUnit;
	unsigned short pdNumOneUnitLine;
	unsigned char  pdXpos[128];
	unsigned char  pdYpos[128];
	unsigned char  pdType[128];
};

struct _TdbPdc_t {
	unsigned short pdcSecXpos[17];
	unsigned short pdcSecYpos[17];
	unsigned short pdcSecXgrad[16];
	unsigned short pdcSecYgrad[16];
	unsigned short pdcLvalue[289];
	unsigned short pdcRvalue[289];
};

struct _TdbPdnc_t {
	unsigned short pdncNo;
	unsigned short pdncSecXstep;
	unsigned short pdncSecYstep;
	unsigned short pdncSecXpos[5];
	unsigned short pdncSecYpos[5];
	unsigned short pdncSecXgrad[4];
	unsigned short pdncSecYgrad[4];
	short  pdncShadowValue[20][25];
};

struct _TdbBpc_t {
	int     pdFlagEnable;
	int     pdRsel;   // select PD in B or R, 1: R
	unsigned int     pattern;
	unsigned short     bpcMethod;
	short      dpath;
	short      dputh;
	unsigned short     aratio;
	unsigned short     uratio;
	unsigned short     expratio;
	int     hdrFlag;  //1:HDR image;0:non-HDR
};

struct _TdbPdcip_t {
	unsigned int       pdRDiffRatio;
	unsigned char        pdBgWeight0;
	unsigned char        pdBgWeight1;
	unsigned char        pdBgbWeight0;
	unsigned char        pdBgbWeight1;
};

/*Struct define for H-binning block after PDCIP*/
struct _TdbHbin_t {
	int        hbinW00;
	int        hbinW10;
};

/*Struct define for ZZHDR in PDZZ Module*/
struct _TdbZzhdr_t {
	//Interpolation
	RgbChannel16Bits_t      intpDeltaLow;           //10bit * 3
	RgbChannel16Bits_t      intpDeltaHigh;          //10bit * 3
	RgbChannel8Bits_t       intpSmoothGain;         //U0.8 * 3
	RgbChannel16Bits_t      intpSmoothThre;         //10bit * 3
	RgbChannel8Bits_t       intpDeltaGain;          //U0.8 * 3
	RgbChannel16Bits_t      intpDeltaThre;          //10bit * 3
	RgbChannel16Bits_t      intpDeltaLowSat;        //16bit * 3
	RgbChannel16Bits_t      intpDeltaHighSat;       //16bit * 3
	RgbChannel8Bits_t       intpSmoothGainSat;      //U0.8 * 3
	RgbChannel16Bits_t      intpSmoothThreSat;      //16bit * 3
	RgbChannel8Bits_t       intpDeltaGainSat;       //U0.8 * 3
	RgbChannel16Bits_t      intpDeltaThreSat;       //16bit * 3

	//Lsnr calculation
	unsigned char                   lowSnrWinSel;           //2bit
	unsigned char                   lowSnrPixSel;           //1bit
	RgbChannel16Bits_t      fusionNoiseThre;        //10bit * 3
	unsigned char                   fusionNoiseGlCntThre;   //4bit
	unsigned char                   fusionNoiseGsCntThre;   //4bit
	unsigned char                   fusionNoiseRbCntThre;   //4bit

	//Saturation
	RgbChannel16Bits_t      fusionSatThre;          //12bit * 3

	//Mot calculation
	RgbChannel16Bits_t      motNoiseLongGain1;      //U2.8 * 3
	RgbChannel16Bits_t      motNoiseLongOft1;       //10bit * 3
	RgbChannel16Bits_t      motNoiseShortGain1;     //U2.8 * 3
	RgbChannel16Bits_t      motNoiseShortOft1;      //10bit * 3
	RgbChannel16Bits_t      motNoiseLongGain2;      //U2.8 * 3
	RgbChannel16Bits_t      motNoiseLongOft2;       //10bit * 3
	RgbChannel16Bits_t      motNoiseShortGain2;     //U2.8 * 3
	RgbChannel16Bits_t      motNoiseShortOft2;      //10bit * 3
	RgbChannel16Bits_t      motNoiseLongGain3;      //U2.8 * 3
	RgbChannel16Bits_t      motNoiseLongOft3;       //10bit * 3
	RgbChannel16Bits_t      motNoiseShortGain3;     //U2.8 * 3
	RgbChannel16Bits_t      motNoiseShortOft3;      //10bit * 3
	RgbChannel8Bits_t       motThreBldWgt1;         //U1.6 * 3
	RgbChannel8Bits_t       motThreBldWgt2;         //U1.6 * 3
	RgbChannel8Bits_t       motThreBldWgt3;         //U1.6 * 3
	RgbChannel16Bits_t      motThreMin;             //10bit * 3
	RgbChannel16Bits_t      motThreMax;             //10bit * 3
	RgbChannel8Bits_t       motCntThre;             //5bit * 3

	//Blending
	unsigned char                   outputMode;             //1bit
	unsigned char                   fusionLsnrWgt;          //range:0-64
	unsigned char                   fusionSatWgt;           //range:0-64
	unsigned char                   fusionMotWgt;           //range:0-64
	unsigned char                   fusionNorWgt;           //range:0-64
	unsigned char                   fusionNorLongWgt;       //range:0-64
	unsigned char                   bldLsnrWgtLow;          //range:0-64
							//if sigNsatLsnr=0,
						//Low>=High; else Low <= High
	unsigned char                   bldLsnrWgtHigh;         //range:0-64
	unsigned short                  bldLsnrThreHigh;        //12bit
	unsigned short                  bldLsnrThreLow;         //12bit
	unsigned short                  bldLsnrThreSlope;       //U1.9
	unsigned char                   bldSatWgtLow;           //range:0-64
							//if sigSatNlsnr=0,
						//Low>=High; else Low <= High
	unsigned char                   bldSatWgtHigh;          //range:0-64
	unsigned short                  bldSatThreHigh;         //12bit
	unsigned short                  bldSatThreLow;          //12bit
	unsigned short                  bldSatThreSlope;        //U1.9
	unsigned char                   bldMotWgtLow;           //range:0-64
							//if sigNsatNlsnrMot=0
						//Low>=High; else Low <= High
	unsigned char                   bldMotWgtHigh;          //range:0-64
	unsigned short                  bldMotThreHigh;         //12bit
	unsigned short                  bldMotThreLow;          //12bit
	unsigned short                  bldMotThreSlope;        //U1.9
	unsigned char                   bldNorWgtLow;           //range:0-64
						//if sigNsatNlsnrS =0
						//Low>=High; else Low <= High
	unsigned char                   bldNorWgtHigh;          //range:0-64
	unsigned short                  bldNorThreHigh;         //12bit
	unsigned short                  bldNorThreLow;          //12bit
	unsigned short                  bldNorThreSlope;        //U1.9
	unsigned char                   bldNorLongWgtLow;       //range:0-64
							//if sigNsatNlsnrL =0
						//Low>=High; else Low <= High
	unsigned char                   bldNorLongWgtHigh;      //range:0-64
	unsigned short                  bldNorLongThreHigh;     //12bit
	unsigned short                  bldNorLongThreLow;      //12bit
	unsigned short                  bldNorLongThreSlope;    //U1.9
	unsigned char                   filterMaskWgt[3];       //U7 bit
	unsigned char                   filterTransWgt[2];      //U5 bit
	unsigned char                   fusionNoiseRblCntTh;    //2bit
	unsigned char                   satWinSel;              //2bit
	unsigned char                   satPixSel;              //1bit
	unsigned char                   satGlCntTh;             //4bit
	unsigned char                   satGsCntTh;             //4bit
	unsigned char                   satRbsCntTh;            //4bit
	unsigned char                   satRblCntTh;            //4bit
	unsigned char                   bldMotSatNlsnrWl;	//range:0-64
						//if sigSatNlsnrMot=0 Wl>=Wh;
						//else Wl <= Wh
	unsigned char                   bldMotSatNlsnrWh;//range:0-64
	unsigned short                  bldMotSatNlsnrThH;//12bit
	unsigned short                  bldMotSatNlsnrThL;//12bit
	unsigned short                  bldMotSatNlsnrThS;//U1.9
	unsigned char                   fusionMotSatNlsnrWgt;//range:0-64
	unsigned char                   bldMotSatLsnrWl;        //range:0-64
				//if sigSatLsnrMot=0 Wl>=Wh; else Wl <= Wh
	unsigned char                   bldMotSatLsnrWh;//range:0-64
	unsigned short                  bldMotSatLsnrThH;//12bit
	unsigned short                  bldMotSatLsnrThL;//12bit
	unsigned short                  bldMotSatLsnrThS;//U1.9
	unsigned char                   fusionMotSatLsnrWgt;//range:0-64
	unsigned char                   bldMotNsatLsnrWl;//range:0-64
				//if sigNsatLsnrMot=0 Wl>=Wh; else Wl <= Wh
	unsigned char                   bldMotNsatLsnrWh;//range:0-64
	unsigned short                  bldMotNsatLsnrThH;//12bit
	unsigned short                  bldMotNsatLsnrThL;//12bit
	unsigned short                  bldMotNsatLsnrThS;//U1.9
	unsigned char                   fusionMotNsatLsnrWgt;//range:0-64
	unsigned char                   grayAlphaX1;        //u6.0
	unsigned char                   grayAlphaX2;        //u6.0
	unsigned char                   grayAlphaY1;        //u7.0
	unsigned char                   grayAlphaY2;        //u7.0
	unsigned short                  grayAlphaS;         //u4.9
	unsigned short                  wbRgain;            //2.14
	unsigned short                  wbGgain;            //2.14
	unsigned short                  wbBgain;            //2.14
	unsigned char                   gFiltCoefGl0;       //u8.0
	unsigned char                   gFiltCoefGl1;       //u8.0
	unsigned char                   gFiltCoefGs;        //u8.0
	unsigned char                   gFiltCoefCs0;       //u8.0
	unsigned char                   gFiltCoefCs1;       //u8.0
	unsigned char                   gFiltCoefCl0;       //u8.0
	unsigned char                   gFiltCoefCl1;       //u8.0
	unsigned char                   sigNsatLsnrMot;     //u1.0
	unsigned char                   sigSatNlsnrMot;     //u1.0
	unsigned char                   sigSatLsnrMot;      //u1.0
	unsigned char                   sigNsatNlsnrMot;    //u1.0
	unsigned char                   sigNsatLsnr;        //u1.0
	unsigned char                   sigSatNlsnr;        //u1.0
	unsigned char                   sigSatLsnr;         //u1.0
	unsigned char                   sigNsatNlsnrL;      //u1.0
	unsigned char                   sigNsatNlsnrS;      //u1.0
	unsigned char                   wgtTransCtrl;       //u1.0
	unsigned char                   grayTransCtrl;      //u1.0
};


struct _TdbPreLscGlobal_t {
	unsigned short    lscXGradTbl[8];
	unsigned short    lscYGradTbl[8];
	unsigned short    lscXSizeTbl[8];
	unsigned short    lscYSizeTbl[8];
};

struct _TdbPreLscMatrix_t {
	struct _Tdb17x17UShortMatrix_t  lscMatrix[TDB_4CH_COLOR_COMPONENT_MAX];
};

struct _TdbPreLscData_t {
	TdbPreLscGlobal_t lscGlobal;
	TdbPreLscMatrix_t lscProfile[TDB_CONFIG_PRE_LSC_MAX_ILLU_PROFILES];
};

struct _TdbPreLsc_t {
	unsigned int illuNum;//The illuminations contained for PRE LSC.
			//Should be less than
			//or equal to TDB_CONFIG_PRE_LSC_MAX_ILLU_PROFILES
	TdbPreLscData_t         data;
};

/*Struct define for Pre Pipe Overall*/
struct _PrePipeCdb_t {
	TdbBlsMeasure_t blsMeasure;
	struct _TdbDegamma_t degamma;
	struct _TdbDir_t dir;
	TdbPde_t pde;
	TdbPdc_t pdc;
	TdbPdnc_t pdnc;
	TdbBpc_t bpc;
	TdbPdcip_t pdcip;
	//TdbHbin_t hbin; //Currently not used
	TdbZzhdr_t zzhdr;
	TdbPreLsc_t preLsc;
};
// Pre-pipe calibration data starts here

// 3rd party pipeline calibration data starts here
#define CDB_LIGHT_SRC_NUM             (4)
#define CDB_MESH_NUM                  (15)
#define CDB_GAIN_TO_BL_NUM            (9)
#define CDB_AWB_SCENE_PRESETS_NUM     (7)
#define CDB_CA_MEM_NUM                (4)

//Dynamic calibration data
#define CDB_EXPO_RATIO_ADJUSTMENT_NUM           (4)
#define CDB_SINTER_STRENGTH_MC_CONTRAST_NUM     (1)
#define CDB_SINTER_STRENGTH_NUM                 (10)
#define CDB_SINTER_STRENGTH_1_NUM               (11)
#define CDB_SINTER_THRESH_1_NUM                 (7)
#define CDB_SINTER_THRESH_4_NUM                 (7)
#define CDB_SINTER_INTCONFIG_NUM                (7)
#define CDB_SINTER_SAD_NUM                      (7)
#define CDB_TEMPER_STRENGTH_NUM                 (10)
#define CDB_SHARP_ALT_D_NUM                     (11)
#define CDB_SHARP_ALT_UD_NUM                    (11)
#define CDB_SHARP_ALT_DU_NUM                    (11)
#define CDB_SHARPEN_FR_STRENGTH_NUM             (10)
#define CDB_DEMOSAIC_NP_OFFSET_NUM              (9)
#define CDB_MESH_SHAIDNG_STRENGTH_NUM           (1)
#define CDB_SATUREATION_STRENGTH_NUM            (11)
#define CDB_AWB_BG_MAX_GAIN_NUM                 (3)
#define CDB_CMOS_EXPOSURE_PATITION_LUT_NUM      (2)
#define CDB_DP_SLOP_NUM                         (9)
#define CDP_DP_THRESHOLD_NUM                    (9)
#define CDB_STITCHING_LM_NIOSE_TH_NUM           (3)
#define CDB_STITCHING_LM_MOV_MULT_NUM           (10)
#define CDB_STITCHING_MS_MOV_MULT_NUM           (3)
#define CDB_STITCHING_SVS_MOV_MULT_NUM          (3)
#define CDB_STITCHING_LM_NP_NUM                 (9)
#define CDB_STITCHING_MS_NP_NUM                 (3)
#define CDB_STITCHING_SVS_NP_NUM                (3)
#define CDB_AE_CONTROL_HDR_TARGET_NUM           (8)
#define CDB_CNR_UV_DELTA12_SLOP                 (10)
#define CDB_COMMON_MODULATION_NUM               (7)
//Notice: temp change to 9000 for enable tuning seq
#define CDB_CUST_SETTINGS_NUM                   (9000)
enum _CalibDbModuleId_t {
	CALIB_INVALID = -1,
	//TODO: reserved for pre process
	//CALIB_PREPIPE
	CALIB_COREPIPE_DECOMPANDER,
	CALIB_COREPIPE_WDR_NP,
	CALIB_COREPIPE_STITCHING,
	CALIB_COREPIPE_DP_CORRECTION,
	CALIB_COREPIPE_SINTER,
	CALIB_COREPIPE_TEMPER,
	CALIB_COREPIPE_CAC,
	CALIB_COREPIPE_BLACKLEVEL,
	CALIB_COREPIPE_RADIAL_SHADING,
	CALIB_COREPIPE_MESH_SHADING,
	CALIB_COREPIPE_IRIDIX,
	CALIB_COREPIPE_DEMOSAIC,
	CALIB_COREPIPE_SHARP_DEMOSAIC,
	CALIB_COREPIPE_PF,
	CALIB_COREPIPE_CCM,
	CALIB_COREPIPE_CNR,
	CALIB_COREPIPE_LUT_3D,
	CALIB_COREPIPE_AUTOLEVEL_CONTROL,
	CALIB_COREPIPE_GAMMA,
	CALIB_COREPIPE_SHARPEN_FR,
	CALIB_COREPIPE_RGB2YUV_CONV,
	CALIB_COREPIPE_NOISE_PROFILE,
	CALIB_COREPIPE_CUST_SETTINGS,
	//TODO: reserved for post process
	//CALIB_POSTPIPE
	//TODO: reserved for AMD 3A
	//CALIB_3A
	CALIB_MAX
};

enum _Cdb3ChColorComponent_t {
	CDB_3CH_COLOR_COMPONENT_RED   = 0,
	CDB_3CH_COLOR_COMPONENT_GREEN = 1,
	CDB_3CH_COLOR_COMPONENT_BLUE  = 2,
	CDB_3CH_COLOR_COMPONENT_IR    = 3,
	CDB_3CH_COLOR_COMPONENT_MAX
};

enum _Cdb4ChColorComponent_t {
	CDB_4CH_COLOR_COMPONENT_RED    = 0,
	CDB_4CH_COLOR_COMPONENT_GREENR = 1,
	CDB_4CH_COLOR_COMPONENT_GREENB = 2,
	CDB_4CH_COLOR_COMPONENT_BLUE   = 3,
	CDB_4CH_COLOR_COMPONENT_MAX
};

enum _CdbCcmComponent_t {
	CDB_CCM_COMPONENT_A   = 0,
	CDB_CCM_COMPONENT_D40 = 1,
	CDB_CCM_COMPONENT_D50 = 2,
	CDB_CCM_COMPONENT_MAX
};

enum _CdbShadingComponent_t {
	CDB_SHADING_COMPONENT_A    = 0,
	CDB_SHADING_COMPONENT_TL84 = 1,
	CDB_SHADING_COMPONENT_D65  = 2,
	CDB_SHADING_COMPONENT_MAX
};

enum _CdbAwbWarmingComponent_t {
	CDB_AWB_WARMING_COMPONENT_A   = 0,
	CDB_AWB_WARMING_COMPONENT_D50 = 1,
	CDB_AWB_WARMING_COMPONENT_D75 = 2,
	CDB_AWB_WARMING_COMPONENT_MAX
};

struct _CdbGainModulationCoeff_t {
	unsigned short gain;
	unsigned short uCoeff;
};

struct _CdbGainModulationU8Coeff_t {
	unsigned short gain;
	unsigned char uCoeff;
};

struct _CdbContrastModulationCoeff_t {
	unsigned short contrast;
	unsigned short uCoeff;
};

struct _CdbCustSettingsValue_t {
	unsigned int reg;
	unsigned int value;
};

struct _CdbShadingRadialCoeff_t {
	unsigned short uCoeff[129];
};

struct _CdbShadingRadial_t {
	CdbShadingRadialCoeff_t shading[CDB_3CH_COLOR_COMPONENT_MAX];
};

struct _CdbShadingRadialCentreAndMultCoeff_t {
	unsigned short uCoeff[16];
};

struct _CdbShadingRadialCentreAndMult_t {
	CdbShadingRadialCentreAndMultCoeff_t centreAndMult;
};

// @ModuleId: CALIB_COREPIPE_RADIAL_SHADING
struct _CdbRadialShading_t {
	CdbShadingRadial_t      shadingRadial;
	CdbShadingRadialCentreAndMult_t shadingRadialCentreAndMult;
};

// @ModuleId: CALIB_COREPIPE_DECOMPANDER
struct _CdbDecompanderMem_t {
	unsigned int decompander0[33];
	unsigned int decompander1[257];
};

struct _CdbLightSrcUCoeff_t {
	unsigned short x;
	unsigned short y;
};

struct _CdbLightSrc_t {
	unsigned int validSize;
	CdbLightSrcUCoeff_t src[CDB_LIGHT_SRC_NUM];
};

struct _CdbRgBgPos_t {
	unsigned short pos[15];
};

struct _CdbMeshCoeff_t {
	unsigned short uCoeff[15];
};

struct _CdbMesh_t {
	unsigned int validSize;
	CdbMeshCoeff_t mesh[CDB_MESH_NUM];
};

struct _CdbWbStrength_t {
	unsigned short uCoeff[3];
};

struct _CdbColorTemp_t {
	unsigned short uCoeff[7];
};

struct _CdbEvToLuxLut_t {
	unsigned int uCoeff[17];
};

struct _CdbBLCoeff_t {
	unsigned short gain;
	unsigned short bl;
};

struct _CdbGainToBL_t {
	unsigned int validSize;
	CdbBLCoeff_t bl[CDB_GAIN_TO_BL_NUM];
};

// @ModuleId: CALIB_COREPIPE_BLACKLEVEL
struct _CdbBlackLevel_t {
	CdbGainToBL_t blackLevel[CDB_4CH_COLOR_COMPONENT_MAX];
};

struct _CdbStaticWb_t {
	unsigned short uCoeff[4];
};

struct _CdbCcmCoeff_t {
	unsigned short uCoeff[9];
};

struct _CdbAbsoluteCcm_t {
	unsigned int validSize;
	CdbCcmCoeff_t ccm[CDB_CCM_COMPONENT_MAX];
};

struct _CdbShadingChannelCoeff_t {
	unsigned char uCoeff[1024];
};

struct _CdbShadingChannel_t {
	CdbShadingCoeff_t shadingCoeff[CDB_3CH_COLOR_COMPONENT_IR];
};

struct _CdbMeshShadingLUT_t {
	unsigned int validSize;
	CdbShadingChannel_t shadingChannel[CDB_SHADING_COMPONENT_MAX];
};

struct _CdbMeshShadingStrength_t {
	unsigned int validSize;
	CdbGainModulationCoeff_t strength[CDB_MESH_SHAIDNG_STRENGTH_NUM];
};

// @ModuleId: CALIB_COREPIPE_MESH_SHADING
struct _CdbShading_t {
	CdbMeshShadingStrength_t        meshShadingStrength;
	CdbMeshShadingLUT_t             meshShadingLut;
};

struct _CdbAwbWarmingCoeff_t {
	unsigned short uCoeff[3];
};

struct _CdbAwbWarming_t {
	unsigned int validSize;
	CdbAwbWarmingCoeff_t awb[CDB_AWB_WARMING_COMPONENT_MAX];
};

struct _CdbAwbWarmingCctCoeff_t {
	unsigned short uCoeff[4];
};

struct _CdbAwbWarmingCct_t {
	CdbAwbWarmingCctCoeff_t warmingCct;
};

//The sinter and temper only has 32 lut registers, but calib data has
//128 params. Each register should set 4 values to 4 fields.
//The wdrNp has l/m/s/vs 4 channels, each channel has 32 params.
struct _CdbLut_t {
	unsigned char uCoeff[128];
};

// @ModuleId: CALIB_COREPIPE_GAMMA
struct _CdbGamma_t {
	unsigned short uCoeff[129];
};

struct _CdbIrdixAsym_t {
	unsigned int uCoeff[65];
};

struct _CdbAwbScenePresetsCoeff_t {
	unsigned short cr;
	unsigned short cb;
};

struct _CdbAwbScenePresets_t {
	unsigned int validSize;
	CdbAwbScenePresetsCoeff_t coeff[CDB_AWB_SCENE_PRESETS_NUM];
};

struct _CdbCaFilterMem_t {
	unsigned int uCoeff[32];
};

struct _CdbCaCorrection_t {
	unsigned short uCoeff[3];
};

struct _CdbCaCorrectionMemCoeff_t {
	unsigned short uCoeff[10];
};

struct _CdbCaCorrectionMem_t {
	unsigned int validSize;
	CdbCaCorrectionMemCoeff_t caMemCoeff[CDB_CA_MEM_NUM];
};

// @ModuleId: CALIB_COREPIPE_CAC
struct _CdbCAC_t {
	CdbCaFilterMem_t        caFilterMem;        //cac
	CdbCaCorrection_t       caCorrection;       //cac
	CdbCaCorrectionMem_t    caCorrectionMem;    //cac
};

// @ModuleId: CALIB_COREPIPE_LUT_3D
struct _CdbLut3d_t {
	unsigned int uCoeff[1000];
};

struct _CdbExposureRatioAdjust_t {
	unsigned int validSize;
	struct _CdbContrastModulationCoeff_t
		ratio[CDB_EXPO_RATIO_ADJUSTMENT_NUM];
};

//sinter strength master contrast
struct _CdbSinterStrengthMCContrast_t {
	unsigned int validSize;
	struct _CdbContrastModulationCoeff_t
		strength[CDB_SINTER_STRENGTH_MC_CONTRAST_NUM];
};

struct _CdbSinterRadialLut_t {
	unsigned char lut[33];
};

struct _CdbSinterRadialParams_t {
	unsigned short rmEnable;
	unsigned short rmCenterX;
	unsigned short rmCenterY;
	unsigned short rmOffCentreMult; //rm_off_centre_mult:
			//round((2^31)/((rm_centre_x^2)+(rm_centre_y^2)))
};

struct _CdbSinterStrength_t {
	unsigned int validSize;
	CdbGainModulationCoeff_t strength[CDB_SINTER_STRENGTH_NUM];
};

struct _CdbSinterStrength1_t {
	unsigned int validSize;
	CdbGainModulationCoeff_t strength[CDB_SINTER_STRENGTH_1_NUM];
};

struct _CdbSinterThresh1_t {
	unsigned int validSize;
	CdbGainModulationCoeff_t thresh1[CDB_SINTER_THRESH_1_NUM];
};

struct _CdbSinterThresh4_t {
	unsigned int validSize;
	CdbGainModulationCoeff_t thresh4[CDB_SINTER_THRESH_4_NUM];
};

struct _CdbSinterIntconfig_t {
	unsigned int validSize;
	CdbGainModulationCoeff_t intconfig[CDB_SINTER_INTCONFIG_NUM];
};

struct _CdbSinterSad_t {
	unsigned int validSize;
	CdbGainModulationCoeff_t sad[CDB_SINTER_SAD_NUM];
};

// @ModuleId: CALIB_COREPIPE_SINTER
struct _CdbSinter_t {
	CdbSinterStrengthMCContrast_t   strengthMCContrast;
	CdbSinterRadialLut_t            radialLut;
	CdbSinterRadialParams_t         radialParams;
	CdbSinterStrength_t             strength;
	CdbSinterStrength1_t            strength1;
	CdbSinterThresh1_t              thresh1;
	CdbSinterThresh4_t              thresh4;
	CdbSinterIntconfig_t            intconfig;
	CdbSinterSad_t                  sad;
};

// @ModuleId: CALIB_COREPIPE_TEMPER
struct _CdbTemperStrength_t {
	unsigned int validSize;
	CdbGainModulationCoeff_t strength[CDB_TEMPER_STRENGTH_NUM];
};

struct _CdbSharpAltD_t {
	unsigned int validSize;
	CdbGainModulationCoeff_t altD[CDB_SHARP_ALT_D_NUM];
};

struct _CdbSharpAltUd_t {
	unsigned int validSize;
	CdbGainModulationCoeff_t altUd[CDB_SHARP_ALT_UD_NUM];
};

struct _CdbSharpAltDu_t {
	unsigned int validSize;
	CdbGainModulationCoeff_t altDu[CDB_SHARP_ALT_DU_NUM];
};

// @ModuleId: CALIB_COREPIPE_SHARP_DEMOSAIC
struct _CdbSharp_t {
	CdbSharpAltD_t  altD;
	CdbSharpAltUd_t altUd;
	CdbSharpAltDu_t altDu;
};

// @ModuleId: CALIB_COREPIPE_SHARPEN_FR
struct _CdbSharpenFrStrength_t {
	unsigned int validSize;
	CdbGainModulationCoeff_t strength[CDB_SHARPEN_FR_STRENGTH_NUM];
};

struct _CdbDemosaicNpOffset_t {
	unsigned int validSize;
	CdbGainModulationCoeff_t npOffset[CDB_DEMOSAIC_NP_OFFSET_NUM];
};

struct _CdbDemosaicUuSlope_t {
	unsigned int validSize;
	CdbGainModulationU8Coeff_t uuSlop[CDB_COMMON_MODULATION_NUM];
};

struct _CdbDemosaicSharpAltLu_t {
	unsigned int validSize;
	CdbGainModulationU8Coeff_t altLu[CDB_COMMON_MODULATION_NUM];
};

struct _CdbDemosaicSharpAltLdu_t {
	unsigned int validSize;
	CdbGainModulationU8Coeff_t altLdu[CDB_COMMON_MODULATION_NUM];
};

struct _CdbDemosaicSharpAltLd_t {
	unsigned int validSize;
	CdbGainModulationU8Coeff_t altLd[CDB_COMMON_MODULATION_NUM];
};

struct _CdbDemosaicFcAliasSlop_t {
	unsigned int validSize;
	CdbGainModulationU8Coeff_t fcAliasSlop[CDB_COMMON_MODULATION_NUM];
};

struct _CdbDemosaicFcSlop_t {
	unsigned int validSize;
	CdbGainModulationU8Coeff_t fcSlop[CDB_COMMON_MODULATION_NUM];
};

// @ModuleId: CALIB_COREPIPE_DEMOSAIC
struct _CdbDemosaic_t {
#ifndef CONFIG_DISABLE_TDB_NEW_ELEMENT
	unsigned int                      enabled;
	CdbDemosaicUuSlope_t        uuSlop;
	unsigned char                       vaSlope;
	unsigned char                       aaSlope;
	unsigned char                       vhSlope;
	unsigned char                       satSlope;
	unsigned short                      aaThresh;
	unsigned short                      vhThresh;
	unsigned short                      uuThresh;
	unsigned short                      vaThresh;
	unsigned short                      aaOffset;
	unsigned short                      vhOffset;
	unsigned short                      uuOffset;
	unsigned short                      vaOffset;
	unsigned char                       config;
	CdbDemosaicFcAliasSlop_t    fcAliasSlop;
	CdbDemosaicFcSlop_t         fcSlop;
	CdbDemosaicSharpAltLu_t     sharpAltLu;
	CdbDemosaicSharpAltLdu_t    sharpAltLdu;
	CdbDemosaicSharpAltLd_t     sharpAltLd;
	unsigned char                       greyDetThresh;
	unsigned char                       lgDetThresh;
	unsigned short                      greyDetSlop;
	unsigned short                      lgDetSlop;
	unsigned char                       lumaOffsetLowD;
	unsigned short                      lumaThreshLowD;
	unsigned int                      lumaSlopLowD;
	unsigned short                      lumaThreshHighD;
	unsigned int                      lumaSlophighD;
	unsigned char                       lumaOffsetLowUd;
	unsigned short                      lumaThreshLowUd;
	unsigned int                      lumaSlopLowUd;
	unsigned short                      lumaThreshHighUd;
	unsigned int                      lumaSlophighUd;
	CdbLut_t                    dmsc;
#else
	CdbLut_t                    dmsc;
	CdbDemosaicNpOffset_t       demosaicNpOffset;//noise profile offset
#endif
};

struct _CdbSaturationStrength_t {
	unsigned int validSize;
	CdbGainModulationCoeff_t strength[CDB_SATUREATION_STRENGTH_NUM];
};

// @ModuleId: CALIB_COREPIPE_CCM
struct _CdbCcm_t {
	unsigned short                      ccmOneGainThreshold;//for ccm switch
	CdbAbsoluteCcm_t            absoluteCcm;
	CdbSaturationStrength_t     saturationStrength;         //for ccm
};

struct _CdbAwbColourPreference_t {
	short preference[4];
};

struct _CdbAwbMixLightParams_t {
	unsigned int enabled;
	unsigned int lutLowBoundary;
	unsigned int lutHighBoundary;
	unsigned int contrashThreshold;
	unsigned int bgThreshold;
	unsigned int bgWeight;
	unsigned int rgHighLutMax;
	unsigned int rgHighLutMin;
	unsigned int printDebug;
};

struct  _CdbAwbBgMaxGain_t {
	unsigned int validSize;
	CdbGainModulationCoeff_t maxGain[CDB_AWB_BG_MAX_GAIN_NUM];
};

struct _CdbPfRadialLut_t {
	unsigned char lut[33];
};

struct _CdbPfRadialParams_t {
	unsigned short rmCenterX;
	unsigned short rmCenterY;
	unsigned short rmOffCentreMult;
};

// @ModuleId: CALIB_COREPIPE_PF
struct _CdbPfRadial_t {
	CdbPfRadialLut_t    radialLut;
	CdbPfRadialParams_t radialParams;
};

//strength dark enhancement control
struct  _CdbIridix8StrengthDkEnhCtl_t {
	unsigned int darkPrc;                // dark_prc
	unsigned int brightPrc;              // bright_prc
	unsigned int minDk;// min_dk: minimum dark enhancement
	unsigned int maxDk;           // max_dk: maximum dark enhancement
	unsigned int pdCutMin;// pD_cut_min: minimum intensity cut for dark
				// regions in which dk_enh will be applied
	unsigned int pdCutMax;// pD_cut_max: maximum intensity cut for dark
				//  regions in which dk_enh will be applied
	unsigned int darkContrastMin;        // dark contrast min
	unsigned int darkContrastMax;        // dark contrast max
	unsigned int minStr;// min_str: iridix strength in percentage
	unsigned int maxStr;// max_str: iridix strength in percentage:
				// 50 = 1x gain. 100 = 2x gain
	unsigned int darkPrcGainTarget;
				// dark_prc_gain_target: target in histogram
				// (percentage) for dark_prc after iridix is
				// applied
	unsigned int contrastMin;// contrast_min: clip factor of strength for
				// LDR scenes.
	unsigned int contrastMax;// contrast_max: clip factor of strength for
				// HDR scenes.
	unsigned int maxIridixGain;          // max iridix gain
	unsigned int printDebug;             // print debug
};

// @ModuleId: CALIB_COREPIPE_IRIDIX
struct CdbIridix_t {
	unsigned char                           avgCoef;
	unsigned short                          minMaxStr;
	unsigned int                          evLimFullStr;
	unsigned int                          evLimNoStr[2];
	unsigned char                           strengthMaximum;
	CdbIrdixAsym_t                  irdixAsym;
	CdbIridix8StrengthDkEnhCtl_t    strengthDkEnhCtl;
};

// exposure_partitions has {integration_time, gain } pairs,
// so if i is even number, such as 0, 2, means int_time, odd num means gain.
struct _CdbCmosExpoPartitionLutsCoeff_t {
	unsigned short iTime0;
	unsigned short gain0;
	unsigned short iime1;
	unsigned short gain1;
	unsigned short iTime2;
	unsigned short gain2;
	unsigned short iTime3;
	unsigned short gain3;
	unsigned short iTime4;
	unsigned short gain4;
};

struct _CdbCmosExpoPartitionLuts_t {
	unsigned int validSize;
	CdbCmosExpoPartitionLutsCoeff_t lut[CDB_CMOS_EXPOSURE_PATITION_LUT_NUM];
};

struct _CdbCmosControl_t {
	unsigned int antiflickerEnable;   // enable antiflicker
	unsigned int antiflickerFre;      // antiflicker frequency
	unsigned int manualITime;         // manual integration time
	unsigned int manualSensorAGain;   // manual sensor analog gain
	unsigned int manualSensorDGain;   // manual sensor digital gain
	unsigned int manualIspDGainl;     // manual isp digital gain
	unsigned int manualMaxItime;      // manual max integration time
	unsigned int maxItime;            // max integration time
	unsigned int maxSensorAG;         // max sensor AG
	unsigned int maxSensorDG;         // max sensor DG
	unsigned int maxIspDG;            // 159 max isp DG
	unsigned int maxExpoRation;       // max exposure ratio
	unsigned int iTime;               // integration time.
	unsigned int sensorAgain;// sensor analog gain. log2 fixed - 5 bits
	unsigned int sensorDgain;
	// sensor digital gain. log2 fixed - 5 bits
	unsigned int ispDgain;
	// isp digital gain. log2 fixed - 5 bits
	unsigned int aGainLastPriority;   // analog_gain_last_priority
	unsigned int aGainReserve;        // analog_gain_reserve
};

struct _CdbCmos_t {
	CdbCmosControl_t            cmosControl;
	CdbCmosExpoPartitionLuts_t  expoPartitionLuts;
};

struct _CdbCalibraitonStatusInfo_t {
	unsigned int sysTotalGainLog2;    // sys.total_gain_log2
	unsigned int sysExpoLog2;         // sys.expsoure_log2
	unsigned int awbMixLightContrash; // awb.mix_light_contrast
	unsigned int afCurLensPos;        // af.cur_lens_pos
	unsigned int afCurFocusValue;     // af.cur_focus_value
};

// @ModuleId: CALIB_COREPIPE_AUTOLEVEL_CONTROL
struct _CdbAutoLevelControl_t {
	unsigned int blackPercentage;
	unsigned int whitePercentage;
	unsigned int autoBlackMin;
	unsigned int autoBlackMax;
	unsigned int autoWhitePrc;
	unsigned int avgCoeff;
	unsigned int enableAutoLevel;
};

struct _CdbDpSlop_t {
	unsigned int validSize;
	CdbGainModulationCoeff_t slop[CDB_DP_SLOP_NUM];
};

struct _CdbDpThreshold_t {
	unsigned int validSize;
	CdbGainModulationCoeff_t threshold[CDP_DP_THRESHOLD_NUM];
};

// @ModuleId: CALIB_COREPIPE_DP_CORRECTION
struct _CdbDPCorrection_t {
	CdbDpSlop_t         dpSlop;
	CdbDpThreshold_t    dpThreshold;
};

struct _CdbStitchingLMMedNoiseIntensityThresh_t {
	unsigned int validSize;
	CdbGainModulationCoeff_t thresh[CDB_STITCHING_LM_NIOSE_TH_NUM];
};

struct _CdbStitchingLmMovMult_t {
	unsigned int validSize;
	CdbGainModulationCoeff_t lmMovMult[CDB_STITCHING_LM_MOV_MULT_NUM];
};

struct _CdbStitchingMsMovMult_t {
	unsigned int validSize;
	CdbGainModulationCoeff_t msMovMult[CDB_STITCHING_MS_MOV_MULT_NUM];
};

struct _CdbStitchingSvsMovMult_t {
	unsigned int validSize;
	CdbGainModulationCoeff_t svsMovMult[CDB_STITCHING_SVS_MOV_MULT_NUM];
};

struct _CdbStitchingLmNp_t {
	unsigned int validSize;
	CdbGainModulationCoeff_t lmNp[CDB_STITCHING_LM_NP_NUM];
};

struct _CdbStitchingMsNp_t {
	unsigned int validSize;
	CdbGainModulationCoeff_t msNp[CDB_STITCHING_MS_NP_NUM];
};

struct _CdbStitchingSvsNp_t {
	unsigned int validSize;
	CdbGainModulationCoeff_t svsNp[CDB_STITCHING_SVS_NP_NUM];
};

// @ModuleId: CALIB_COREPIPE_STITCHING
struct _CdbStitching_t {
	CdbStitchingLMMedNoiseIntensityThresh_t     lmMedNoiseIntensityTh;
	CdbStitchingLmMovMult_t                     lmMovMult;
	CdbStitchingLmNp_t                          lmNp;
	CdbStitchingMsMovMult_t                     msMovMult;
	CdbStitchingMsNp_t                          msNp;
	CdbStitchingSvsMovMult_t                    svsMovMult;
	CdbStitchingSvsNp_t                         svsNp;
	unsigned short                                      fsMcOff;
};

struct _CdbAeCorrection_t {
	unsigned char uCoeff[12];
};

struct _CdbAeExpoCorrection_t {
	unsigned int uCoeff[12];
};

struct _CdbCalibraitonAeControl_t {
	unsigned int aeConvergance;   // AE convergance
	unsigned int ldrAeTarget;// LDR AE target -> this should match the 18%
				// grey of the output gamma
	unsigned int aeTailWeight;    // AE tail weight
	unsigned int clippedPerLong;// WDR mode only: Max percentage of clipped
				// pixels for long exposure: WDR mode only:
				// 256 = 100% clipped pixels
	unsigned int timeFilter;// WDR mode only: Time filter for exposure ratio
	unsigned int brightPrc;
	// control for clipping: bright percentage of
				// pixels that should be below hi_target_prc
	unsigned int hiTargetPrc;
	// control for clipping: highlights percentage
				// (hi_target_prc): target for tail of histogram
	unsigned int irGdgEnable;
	// 1:0 enable | disable iridix global gain.
	unsigned int aeTolerance;     // AE tolerance
};

struct _CdbAeControlHdrTarget_t {
	unsigned int validSize;
	CdbGainModulationCoeff_t target[CDB_AE_CONTROL_HDR_TARGET_NUM];
};

// @ModuleId: CALIB_COREPIPE_RGB2YUV_CONV
struct _CdbRgb2YuvConversion_t {
	unsigned short uCoeff[12];
};

struct _CdbAfLms_t {
	unsigned int downFarEnd;          // Down_FarEnd
	unsigned int horFarEnd;           // Hor_FarEnd
	unsigned int upFarEnd;            // Up_FarEnd
	unsigned int downInfinity;        // Down_Infinity
	unsigned int horInfinity;         // Hor_Infinity
	unsigned int upInfinity;          // Up_Infinity
	unsigned int downMacro;           // Down_Macro
	unsigned int horMacro;            // Hor_Macro
	unsigned int upMacro;             // Up_Macro
	unsigned int downEnd;             // Down_NearEnd
	unsigned int hornearEnd;          // Hor_NearEnd
	unsigned int upNearEnd;           // Up_NearEnd
	unsigned int stepNum;             // step_num
	unsigned int skipFramesInit;      // skip_frames_init
	unsigned int skipFramesMove;      // skip_frames_move
	unsigned int dynamicRangeTh;      // dynamic_range_th
	unsigned int spotTolerance;
	// 2 <<( LOG2_GAIN_SHIFT - 2 ),spot_tolerance
	unsigned int exitTh;// 1 << ( LOG2_GAIN_SHIFT - 1 ),   exit_th
	unsigned int cafTriggerTh;
	// 16 <<(LOG2_GAIN_SHIFT - 4 ),caf_trigger_th
	unsigned int cafStableTh;// 4 <<(LOG2_GAIN_SHIFT - 4 ),   caf_stable_th
	unsigned int printDebug;          // print_debug
};

struct _CdbAfZoneWghtCoeff_t {
	unsigned short uCoeff[17];
};

struct _CdbAeZoneWghtCoeff_t {
	unsigned short uCoeff[32];
};

struct _CdbAwbZoneWghtCoeff_t {
	unsigned short uCoeff[32];
};

// @ModuleId: CALIB_COREPIPE_CNR
struct _CdbCnrUvDelta12Slop_t {
	unsigned int validSize;
	CdbGainModulationCoeff_t slop[CDB_CNR_UV_DELTA12_SLOP];
};

// @ModuleId: CALIB_COREPIPE_WDR_NP
struct _CdbWdrNoiseProfile_t {
	CdbLut_t     wdrNp;
};

// @ModuleId: CALIB_COREPIPE_NOISE_PROFILE
struct _CdbNoiseProfile_t {
	CdbLut_t     np;
};

// @ModuleId: CALIB_COREPIPE_CUST_SETTINGS
struct _CdbCustSettings_t {
	unsigned int validSize;
	CdbCustSettingsValue_t     custSetting[CDB_CUST_SETTINGS_NUM];
};

struct _CorePipeCdb_t {
	CdbDecompanderMem_t             decompanderMem;
	CdbWdrNoiseProfile_t            wdrNp;          //stitch niose profile
	CdbStitching_t                  stitching;
	CdbDPCorrection_t               dPCorrection;
	CdbSinter_t                     sinter;
	CdbTemperStrength_t             temperStrength;
	CdbCAC_t                        cac;
	CdbBlackLevel_t                 blackLevel;
	CdbRadialShading_t              radiaShading;
	CdbShading_t                    meshShading;  //mesh shading for
					// diff light source
	CdbIridix_t                     iridix;
	CdbDemosaic_t                   demosaic;
#ifdef CONFIG_DISABLE_TDB_NEW_ELEMENT
	CdbSharp_t                      sharp; //notice: sharpening for demosaic
#endif
	CdbPfRadial_t                   pfRadial;
	CdbCcm_t                        ccm;
	//uv delta1 slop; uv delta2 slop
	CdbCnrUvDelta12Slop_t           cnrUvDelta12Slop;
	CdbLut3d_t                      lut3d;
	CdbAutoLevelControl_t           autoLevelControl;  //for fr rgb gamma
	CdbGamma_t                      gamma;              //fr rgb gamma
	CdbSharpenFrStrength_t          sharpenFrStrength;
	//color space conversion
	CdbRgb2YuvConversion_t          rgb2YuvConversion;
	CdbNoiseProfile_t               noiseProfile;
	//Below are 3A calirations
	CdbAwbScenePresets_t    awbScenePresets;
	CdbAwbWarmingCct_t      awbWarmingCct;
	CdbAwbWarming_t         awbWarming;
	CdbStaticWb_t           staticWb;
	CdbLightSrc_t           lightSrc;
	CdbRgBgPos_t            rgPos;
	CdbRgBgPos_t            bgPos;
	CdbMesh_t               meshRgBgWeight;
	CdbMesh_t               meshLsWeight;
	CdbMesh_t               meshColorTemp;
	CdbWbStrength_t         wbStrength;
	unsigned short                  skyLuxTh;
	CdbColorTemp_t          ctRgPosCal;
	CdbColorTemp_t          ctBgPosCal;
	CdbColorTemp_t          colorTemp;
	unsigned short                  ct65Pos;
	unsigned short                  ct40Pos;
	unsigned short                  ct30Pos;
	CdbEvToLuxLut_t         evLut;
	CdbEvToLuxLut_t         luxLut;
	unsigned char                           evtoluxProbabilityEnable;
	CdbAwbColourPreference_t        awbColourPreference;
	CdbAwbMixLightParams_t          awbMixLightParams;
	CdbAwbBgMaxGain_t               awbBgMaxGain;
	unsigned char                           awbAvgCoeff;
	CdbAwbZoneWghtCoeff_t           awbZoneWghtHor;
	CdbAwbZoneWghtCoeff_t           awbZoneWghtVer;
	CdbExposureRationAdjust_t       expoRationAdjust;
	CdbCmos_t                       cmos;
	CdbAeCorrection_t               aeCorrection;
	CdbAeExpoCorrection_t           aeExpoCorrection;
	CdbCalibraitonAeControl_t       aeControl;
	CdbAeControlHdrTarget_t         aeCtlHdrTarget;
	CdbAeZoneWghtCoeff_t            aeZoneWghtHor;
	CdbAeZoneWghtCoeff_t            aeZoneWghtVer;
	CdbAfLms_t                      afLms;
	CdbAfZoneWghtCoeff_t            afZoneWghtHor;
	CdbAfZoneWghtCoeff_t            afZoneWghtver;
	CdbCalibraitonStatusInfo_t      calibStatus; //record realtime info
	//End 3A calibrations
	CdbCustSettings_t               custSettings;
};
// 3rd party pipeline calibration data ends here

// Post-pipe calibration data ends here
#define RESIZE_TBL_SIZE        (65)
#define TDB_YUV_RESIZE_SET_NUM (10)
struct _YuvResizeTable_t {
	short coeff[RESIZE_TBL_SIZE];
};

// Post pipe blocks

// *********************************update for CDB2.0********************

#define CDB_2_0_COMMON_MODULATION_NUM               (8)

/*Struct define for Post Pipe Module YUVNR Y plane*/
struct _TdbYuvNrY_t {
	unsigned char   yuvNrStrength1;     //0~32    6bits
	unsigned char   yuvNrStrength2;     //0~32    6bits
	unsigned char   yuvNrStrength3;     //0~32    6bits
	unsigned char   yuvNrStrength4;     //0~32    6bits
	unsigned char   yuvNrStrengthCntTh1;        //0~625    10bits
	unsigned char   yuvNrStrengthCntTh2;    //0~625    10bits
	unsigned char   yuvNrStrengthCntTh3;        //0~625    10bits

	unsigned char   yuvNrSigmaCntMin;   //0~255
	unsigned char   yuvNrSigmaCntMax;   //0~255
	unsigned char   yuvNrSigmaBndMin;   //0~255 FW
	unsigned char   yuvNrSigmaBndMax;   //0~255 FW
	unsigned char   yuvNrSigmaMinStp;    //0~1023
	unsigned char   yuvNrSigmaMaxStp;    //0~1023

	unsigned char   yuvNrSigmaBnd1;     //0~255
	unsigned char   yuvNrSigmaBnd2;      //0~255

	unsigned char   yuvNrSigmaPred;        //0~3

	unsigned char   yuvNrSigmaTh1;         //0~15
	unsigned char   yuvNrSigmaTh2;         //0~15
	unsigned char   yuvNrSigmaTh3;         //0~15
	unsigned char   yuvNrSigmaTh4;        //0~15
	unsigned char   yuvNrSigmaTh1Weight;  //0~3
	unsigned char   yuvNrSigmaTh2Weight;  //0~3
	unsigned char   yuvNrSigmaTh3Weight;  //0~3
	unsigned char   yuvNrSigmaTh4Weight;  //0~3

	unsigned char   yuvNrSigmaHistStr;    //0~3
	unsigned char   yuvNrSigmaHistMode;   //0~2
	unsigned char   yuvNrSigmaShift;       //0~3
	unsigned char   yuvNrSigmaHistAvg;    //0~2
	unsigned short  yuvNrSigmaHistMaxCnt; //0~625
	unsigned char   yuvNrSigmaHistPeakMin;   //0~15
	unsigned char   yuvNrSigmaHistPeakTh;    //0~15
	unsigned char   yuvNrSigmaHistDiff;  // 0 1 2
	unsigned char   yuvNrSigmaWeight;        //0~16

	unsigned char   yuvNrSigmaDarkTh;       //0~255 8bits
	unsigned char   yuvNrSigmaDarkShift;       //0~3
	unsigned char   yuvNrSigmaDarkHistAvg;    //0~2

	unsigned char   yuvNrSigmaRange;         //0~8

	unsigned char   yuvNrInkMode;           //0,1,2,3,4
	unsigned char   yuvNrInkSigmaShift;   //0~3
	unsigned char   yuvNrInkCntShift;       //0~3
};

/*Struct define for Post Pipe Module YUVNR UV plane*/
struct _TdbYuvNrUv_t {
	unsigned char   yuvNrStrength;      //0~32    6bits
	unsigned char   yuvNrSigmaCntMin;   //0~255
	unsigned char   yuvNrSigmaCntMax;   //0~255
	unsigned char   yuvNrSigmaBndMin;   //0~255 FW
	unsigned char   yuvNrSigmaBndMax;   //0~255 FW
	unsigned char   yuvNrSigmaMinStp;    //0~1023
	unsigned char   yuvNrSigmaMaxStp;    //0~1023

	unsigned char   yuvNrSigmaBnd1;     //0~255
	unsigned char   yuvNrSigmaBnd2;      //0~255

	unsigned char   yuvNrSigmaPred;        //0~3

	unsigned char   yuvNrSigmaTh1;         //0~15
	unsigned char   yuvNrSigmaTh2;         //0~15
	unsigned char   yuvNrSigmaTh3;         //0~15
	unsigned char   yuvNrSigmaTh4;        //0~15
	unsigned char   yuvNrSigmaTh1Weight;  //0~3
	unsigned char   yuvNrSigmaTh2Weight;  //0~3
	unsigned char   yuvNrSigmaTh3Weight;  //0~3
	unsigned char   yuvNrSigmaTh4Weight;  //0~3

	unsigned char   yuvNrSigmaHistStr;    //0~3
	unsigned char   yuvNrSigmaHistMode;   //0~2
	unsigned char   yuvNrSigmaShift;       //0~3
	unsigned char   yuvNrSigmaHistAvg;    //0~2
	unsigned short  yuvNrSigmaHistMaxCnt; //0~625
	unsigned char   yuvNrSigmaHistPeakMin;   //0~15
	unsigned char   yuvNrSigmaHistPeakTh;    //0~15
	unsigned char   yuvNrSigmaHistDiff;  // 0 1 2
	unsigned char   yuvNrSigmaWeight;        //0~16

	unsigned char   yuvNrSigmaRange;         //0~8

	unsigned char   yuvNrInkMode;           //0,1,2,3,4
	unsigned char   yuvNrInkSigmaShift;   //0~3
	unsigned char   yuvNrInkCntShift;       //0~3
};
struct _TdbGainModulationYuvNrY_t {
	unsigned short gain;
	TdbYuvNrY_t	yuvNrY;
};

struct _TdbGainModulationYuvNrUv_t {
	unsigned short			gain;
	TdbYuvNrUv_t	yuvNrUv;
};

struct _TdbYuvNrYByGain_t {
	unsigned int validSize;
	TdbGainModulationYuvNrY_t params[CDB_2_0_COMMON_MODULATION_NUM];
};

struct _TdbYuvNrUvByGain_t {
	unsigned int validSize;
	TdbGainModulationYuvNrUv_t params[CDB_2_0_COMMON_MODULATION_NUM];
};

struct _TdbYuvNr_t {
	int				enabled;
	TdbYuvNrYByGain_t	yuvNrY;
	TdbYuvNrUvByGain_t	yuvNrUv;
};

/*Struct define for Post Pipe Module Sharpen*/

/*
 *	The following tables are the grouped coefficients for high-pass
 *	band and middle-pass band.
 *	Default settings are 4 for both highBand and MidBand.
 *
 *	highBand_0= [0, -1, -2, -4, -5, -1, 3,16,24, 16, 5, -12, -93, -156, 772]
 *	highBand_1= [0, -1, -4, -9, -11, -4, 0, 18, 30, 22,18, 0, -94,-171, 736]
 *	highBand_2= [0, 0, -1, -1, -2, -1, -3, -6, -7, 4, 34, 55, -54, -183,568]
 *	highBand_3= [0, 0, 1, 1, 2, 0, -4, -13, -18, -2, 37, 69, -42, -185, 528]
 *	highBand_4= [-1, 2, 1, 0, 4, -2, -11, -10, -5, 13, 41,25, -33, -188,564]
 *	highBand_5= [0, 1, 2, 4, 4, 1, -4, -17, -25, -7, 38, 78, -34, -185, 480]
 *	highBand_6= [0, 1, 3, 6, 8, 2, -4, -21, -32, -11, 37, 82, -25, -177,436]

 *	MidBand_0= [-2, -1, 1, 0, -4, -7, -22, -33, -36,-52, -22, 7, 67,127,216]
 *	MidBand_1= [-3, 0, 2, 1, -3, -4, -15, -32, -41, -46, -31,-3,65, 129,224]
 *	MidBand_2= [-2, -1, 0, 1, 0, -1, -9, -30,-41, -35,-37, -22,61, 131, 244]
 *	MidBand_3= [1, 1, -1, 0, 3, 6, 3, -17, -27, -21, -56, -58, 30, 130, 304]
 *	MidBand_4= [2, 0, -1, -1, -2, 5, 7, -3, -10, -24, -71, -68, 0, 135, 400]
 *	MidBand_5= [0, -1, 1, 4, 3, 0, 8, 10, 6, -17, -65, -72, -35, 103, 392]
 *	MidBand_6= [0, 0, 1, 3, 5, -3, 4, 13, 11, -6, -57, -87, -47, 89, 440]
 *	MidBand_7= [-1, 1, 2, 1, 2, -3, 4, 13, 12, 11, -46, -88, -70, 70, 468]
 *	MidBand_8= [0, 0, 1, 1, 1, 3, 12, 28, 37, 6, -76, -142, -62, 83, 568]
 *
 *
 */

//The detal band talble please refer to up table
enum _HighBandIndex_t {
	HIGH_BAND_INDEX_INVALID = -1,
	HIGH_BAND_0 = 0,
	HIGH_BAND_1 = 1,
	HIGH_BAND_2 = 2,
	HIGH_BAND_3 = 3,
	HIGH_BAND_4 = 4,
	HIGH_BAND_5 = 5,
	HIGH_BAND_6 = 6,
	HIGH_BAND_INDEX_MAX
};

enum _MidBandIndex_t {
	MID_BAND_INDEX_INVALID = -1,
	MID_BAND_0 = 0,
	MID_BAND_1 = 1,
	MID_BAND_2 = 2,
	MID_BAND_3 = 3,
	MID_BAND_4 = 4,
	MID_BAND_5 = 5,
	MID_BAND_6 = 6,
	MID_BAND_7 = 7,
	MID_BAND_8 = 8,
	MID_BAND_INDEX_MAX
};

struct _TdbSharpReg_t {
	unsigned char frequencyBlendingWeight;

	unsigned char peakingStrength;
	unsigned char idist;
	unsigned char lf;
	unsigned char rf;
	unsigned short dir16Max;
	unsigned char dir16Step1;
	unsigned char dir16Step2;
	unsigned char sinWaveT;
	unsigned char sinWaveChange;

	//high-pass band index
	HighBandIndex_t highBandIndex;

	//middle-passs band index
	MidBandIndex_t midBandIndex;

	unsigned char bandpassCoringTh;
	unsigned char bandpassSlowGainZoneTh;
	unsigned char bandpassSlowGain;
	unsigned char bandpassGain;
	unsigned char bandpassLimitTh;
	unsigned char bandpassLimitGain;
	unsigned char bandpassAcBound;

	unsigned char highpassCoringTh;
	unsigned char highpassSlowGainZoneTh;
	unsigned char highpassSlowGain;
	unsigned char highpassGain;
	unsigned char highpassLimitTh;
	unsigned char highpassLimitGain;
	unsigned char highpassAcBound;

	unsigned char coringTh;
	unsigned char slowGainZoneTh;
	unsigned char slowGain;
	unsigned char gain;
	unsigned char limitTh;
	unsigned char limitGain;
	unsigned char acBound;

	unsigned char yCenterWeight;
	unsigned char yGain0;
	unsigned char yGain16;
	unsigned char yGain32;
	unsigned char yGain48;
	unsigned char yGain64;
	unsigned char yGain80;
	unsigned char yGain96;
	unsigned char yGain112;
	unsigned char yGain128;
	unsigned char yGain144;
	unsigned char yGain160;
	unsigned char yGain176;
	unsigned char yGain192;
	unsigned char yGain208;
	unsigned char yGain224;
	unsigned char yGain240;
	unsigned char yGain256;

	unsigned char overDecreaseRatio;
	unsigned char acSmoothNeighborWeight;
};

struct _TdbRatioModulationSharp_t {
	unsigned short ratio;
	TdbSharpReg_t sharpReg;
};

struct _TdbSharpRegByRatio_t {
	unsigned int validSize;
	struct _TdbRatioModulationSharp_t
		params[CDB_2_0_COMMON_MODULATION_NUM];
};

struct _TdbGainModulationSharp_t {
	unsigned short Gain;
	TdbSharpReg_t sharpReg;
};

struct _TdbSharpRegByGain_t {
	unsigned int validSize;
	struct _TdbRatioModulationSharp_t
		params[CDB_2_0_COMMON_MODULATION_NUM];
};

struct _TdbSharpRegByChannel_t {
	unsigned char sharpEn;
	unsigned char smoothModeEn;
	unsigned char outputMode;
	unsigned char bandpassEn;
	unsigned char highpassEn;
	unsigned char yGainEn;
	unsigned char saturateProtectEn;
	unsigned char acSmoothEn;
	struct _TdbSharpRegByGain_t         sharpByGain;
	struct _TdbSharpRegByRatio_t        sharpByRatio;
};

enum _CdbIspPipeOutCh_t {
	CDB_ISP_PIPE_OUT_CH_PREVIEW = 0,
	CDB_ISP_PIPE_OUT_CH_VIDEO,
	CDB_ISP_PIPE_OUT_CH_STILL,
	CDB_ISP_PIPE_OUT_CH_CVP_CV,
	CDB_ISP_PIPE_OUT_CH_X86_CV,
	CDB_ISP_PIPE_OUT_CH_IR, //X86 or CVP
	CDB_ISP_PIPE_OUT_CH_STILL_FULL,
	CDB_ISP_PIPE_OUT_CH_RAW,//RAW CH won't need sharpen and yuv scale
	CDB_ISP_PIPE_OUT_CH_MAX,
};

struct _TdbPostSharp_t {//RAW CH won't need sharpen and yuv scale, so minus 1
	struct _TdbSharpRegByChannel_t   sharpen[CDB_ISP_PIPE_OUT_CH_MAX-1];
};

// *****************************end update for CDB2.0*********************
// Struct define for Post Pipe Overall
struct _PostPipeCdb_t {
	YuvResizeTable_t yuvResizeTable;
	TdbYuvNr_t	yuvNr;
	struct _TdbPostSharp_t	postSharp;
};
// Post-pipe calibration data ends here

#ifndef CONFIG_DISABLE_TDB_NEW_ELEMENT
/*Struct define for AE algorithm*/
struct _AeCdb_t {
	struct _ae_tuning_param_t tuningData;
	struct _ae_calibration_param_t calibData;
};

/*Struct define for AWB algorithm*/
struct _AwbCdb_t {
	struct _AwbTuningData_t tuningData;
	struct _AwbCalibData_t calibData;
};
#endif // CONFIG_DISABLE_TDB_NEW_ELEMENT

struct _SensorTdb_t {
	struct _PrePipeCdb_t      prePipeCdb;
	struct _CorePipeCdb_t     corePipeCdb;
	struct _PostPipeCdb_t     postPipeCdb;
#ifndef CONFIG_DISABLE_TDB_NEW_ELEMENT
	struct _AeCdb_t           aeCdb;
	struct _PLineTable_t      plineCdb;
	struct _AwbCdb_t          awbCdb;
#endif // CONFIG_DISABLE_TDB_NEW_ELEMENT
	//SceneMode_t       sceneMode;
};

struct _M2MTdb_t {
	//TODO
};

