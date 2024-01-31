/*
 * Copyright (C) 2019-2021 Advanced Micro Devices, Inc. All rights reserved.
 */

#pragma once

#include "os_base_type.h"
#include <SensorTdbDef.h>
#include "SensorTdb.h"

#include "AeTuningParam.h"
#include "AwbTuningParam.h"
#include "PLineTuningParam.h"
#include "AfTuningParam.h"
#include "IridixTuningParam.h"
#include "AutoContrastTuningParamCtx.h"
#include "Color3DTuningParam.h"

#define CAC_NEW_IMPLEMENT
#define SCENE_MODE_IMPLEMENT

#define SHARPEN_RATIO_ENABLE

#define CDB_COMMON_MODULATION_NUM	(8)
//Add for scene mode
#define SCENE_PREFERENCE_TUNING_PIPE_SETS_NUM	(3)
#define SCENE_PREFERENCE_TUNING_AAA_SETS_NUM	(10)

// Pre-pipe calibration data starts here

#define CDB_PRE_BLS_CHANNEL_MAX_NUM	(16)

struct _CdbBLCoeff_t {
	unsigned short gain;
	unsigned short bl;
};

struct _CdbGainToBL_t {
	unsigned int validSize;
	struct _CdbBLCoeff_t
		bl[CDB_COMMON_MODULATION_NUM];
};

struct _PreBlackLevel_t {
	struct _CdbGainToBL_t
		blackLevel[CDB_PRE_BLS_CHANNEL_MAX_NUM];
};

enum _BlsMode_t {
	BLS_MODE_BYPASS = 0,
	BLS_MODE_STATIC,
	BLS_MODE_DYNAMIC,
	BLS_MODE_MAX,
};

struct _TdbBlsMeasure_t {
	bool  enabled;
	enum _BlsMode_t mode;
	/**< black level for all 16 color components */
	struct _PreBlackLevel_t level;
	//bl measure number exp components
	unsigned short blMeasureNoExp;
	unsigned short blWin1StartX; //window1 start x position
	unsigned short blWin1StartY; //window1 start y position
	unsigned short blWin2StartX; //window2 start x position
	unsigned short blWin2StartY; //window1 start y position
	unsigned short blWin1SizeX; //window1 x size
	unsigned short blWin1SizeY; //window1 y size
	unsigned short blWin2SizeX; //window2 x size
	unsigned short blWin2SizeY; //window2 y size
};

struct _TdbRatioModulationUint8Coeff_t {
	unsigned short ratio;
	unsigned char uCoeff;
};

struct _TdbRatioModulationUint16Coeff_t {
	unsigned short ratio;
	unsigned short uCoeff;
};

struct _TdbRatioModulationInt16Coeff_t {
	unsigned short gain;
	short coeff;
};

struct _TdbGainModulationUint8Coeff_t {
	unsigned short gain;
	unsigned char uCoeff;
};

struct _TdbGainModulationInt16Coeff_t {
	unsigned short gain;
	short coeff;
};

struct _TdbGainModulationUint16Coeff_t {
	unsigned short gain;
	unsigned short uCoeff;
};

struct _TdbGainModulationUint32Coeff_t {
	unsigned short gain;
	unsigned int uCoeff;
};

struct _TdbItimeDiffModulationUint8Coeff_t {
	unsigned short itimeDiff;
	unsigned char coeff;
};

struct _TdbItimeDiffModulationUint16Coeff_t {
	unsigned short itimeDiff;
	unsigned short uCoeff;
};

struct _TdbDirGtf_t {
	unsigned int validSize;
	struct _TdbGainModulationUint8Coeff_t
		gTf[CDB_COMMON_MODULATION_NUM];
};

struct _TdbDirRbtf_t {
	unsigned int validSize;
	struct _TdbGainModulationUint8Coeff_t
		rbTf[CDB_COMMON_MODULATION_NUM];
};

struct _TdbDirItf_t {
	unsigned int validSize;
	struct _TdbGainModulationUint8Coeff_t
		rTf[CDB_COMMON_MODULATION_NUM];
};

struct _TdbDirDpuTh_t {
	unsigned int validSize;
	struct _TdbGainModulationInt16Coeff_t
		dpuTh[CDB_COMMON_MODULATION_NUM];
};

struct _TdbDirDpaTh_t {
	unsigned int validSize;
	struct _TdbGainModulationInt16Coeff_t
		dpaTh[CDB_COMMON_MODULATION_NUM];
};

struct _TdbDirRgbTh_t {
	unsigned int validSize;
	struct _TdbGainModulationUint16Coeff_t
		rgbTh[CDB_COMMON_MODULATION_NUM];
};

struct _TdbDirIrTh_t {
	unsigned int validSize;
	struct _TdbGainModulationUint16Coeff_t
		irTh[CDB_COMMON_MODULATION_NUM];
};

struct _TdbDirDiff_t {
	unsigned int validSize;
	struct _TdbGainModulationUint8Coeff_t
		diff[CDB_COMMON_MODULATION_NUM];
};

struct _TdbDirCdAth_t {
	unsigned int validSize;
	struct _TdbGainModulationInt16Coeff_t
		cdAth[CDB_COMMON_MODULATION_NUM];
};

struct _TdbDirCdUth_t {
	unsigned int validSize;
	struct _TdbGainModulationInt16Coeff_t
		cdUth[CDB_COMMON_MODULATION_NUM];
};

//Fix point DIR parameters
struct _TdbDir_t {
	bool bpcEnabled;
	bool deconEnabled;
	bool cipEnabled;
	bool rgbAdjustEnabled;
	bool isDeconIr;

	//Bad Pixel
	struct _TdbDirDpuTh_t dpuTh;
	struct _TdbDirDpaTh_t dpaTh;

	//Saturation
	struct _TdbDirRgbTh_t rgbTh;
	struct _TdbDirIrTh_t irTh;
	struct _TdbDirDiff_t  diff;

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
	struct _TdbDirRbtf_t rbTf;
	struct _TdbDirGtf_t gTf;
	struct _TdbDirItf_t iTf;

	//Color difference threshold for 4x4 interpolation
	struct _TdbDirCdAth_t cdAth;
	struct _TdbDirCdUth_t cdUth;
};

struct _TdbPde_t {
	bool enabled;
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
	bool enabled;
	unsigned short pdcSecXpos[17];
	unsigned short pdcSecYpos[17];
	unsigned short pdcSecXgrad[16];
	unsigned short pdcSecYgrad[16];
	unsigned short pdcLvalue[289];
	unsigned short pdcRvalue[289];
};

struct _TdbPdnc_t {
	bool enabled;
	unsigned short pdncNo;
	unsigned short pdncSecXstep;
	unsigned short pdncSecYstep;
	unsigned short pdncSecXpos[5];
	unsigned short pdncSecYpos[5];
	unsigned short pdncSecXgrad[4];
	unsigned short pdncSecYgrad[4];
	short  pdncShadowValue[20][25];
};

struct _TdbBpcDpath_t {
	unsigned int validSize;
	struct _TdbGainModulationInt16Coeff_t
		dpath[CDB_COMMON_MODULATION_NUM];
};

struct _TdbBpcDputh_t {
	unsigned int validSize;
	struct _TdbGainModulationInt16Coeff_t
		dputh[CDB_COMMON_MODULATION_NUM];
};

struct _TdbBpcAratio_t {
	unsigned int validSize;
	struct _TdbGainModulationUint16Coeff_t
		aratio[CDB_COMMON_MODULATION_NUM];
};

struct _TdbBpcUratio_t {
	unsigned int validSize;
	struct _TdbGainModulationUint16Coeff_t
		uratio[CDB_COMMON_MODULATION_NUM];
};

struct _TdbBpc_t {
	bool     enabled;
	bool     pdFlagEnable;
	bool     pdRsel;   // select PD in B or R, 1: R
	unsigned int     pattern;
	unsigned short     bpcMethod;
	struct _TdbBpcDpath_t      dpath;
	struct _TdbBpcDputh_t      dputh;
	struct _TdbBpcAratio_t     aratio;
	struct _TdbBpcUratio_t     uratio;
	unsigned short     expratio;
	bool     hdrFlag;  //1:HDR image;0:non-HDR
};

struct _TdbPdcipPdRDiffRatio_t {
	unsigned int validSize;
	struct _TdbGainModulationUint16Coeff_t
		pdRDiffRatio[CDB_COMMON_MODULATION_NUM];
};

struct _TdbPdcipPdBgWeight0_t {
	unsigned int validSize;
	struct _TdbGainModulationUint8Coeff_t
		pdBgWeight0[CDB_COMMON_MODULATION_NUM];
};

struct _TdbPdcipPdBgWeight1_t {
	unsigned int validSize;
	struct _TdbGainModulationUint8Coeff_t
		pdBgWeight1[CDB_COMMON_MODULATION_NUM];
};

struct _TdbPdcipPdBgbWeight0_t {
	unsigned int validSize;
	struct _TdbGainModulationUint8Coeff_t
		pdBgbWeight0[CDB_COMMON_MODULATION_NUM];
};

struct _TdbPdcipPdBgbWeight1_t {
	unsigned int validSize;
	struct _TdbGainModulationUint8Coeff_t
		pdBgbWeight1[CDB_COMMON_MODULATION_NUM];
};

struct _TdbPdcip_t {
	bool	enabled;
	struct _TdbPdcipPdRDiffRatio_t	pdRDiffRatio;
	struct _TdbPdcipPdBgWeight0_t	pdBgWeight0;
	struct _TdbPdcipPdBgWeight1_t	pdBgWeight1;
	struct _TdbPdcipPdBgbWeight0_t	pdBgbWeight0;
	struct _TdbPdcipPdBgbWeight1_t	pdBgbWeight1;
};

/*Struct define for H-binning block after PDCIP*/
struct _TdbHbin_t {
	bool     enabled;
	int        hbinW00;
	int        hbinW10;
};

#define CDB_COMMON_MODULATION_ITIME_NUM          (4)
#define CDB_COMMON_MODULATION_RATIO_NUM          (6)

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

/*Struct define for ZZHDR in PDZZ Module*/
struct _TdbZzhdrRatioParams_t {
	//Interpolation
	struct _RgbChannel16Bits_t      intpDeltaLow;           //10bit * 3
	struct _RgbChannel16Bits_t      intpDeltaHigh;          //10bit * 3
	struct _RgbChannel8Bits_t       intpSmoothGain;         //U0.8 * 3
	struct _RgbChannel16Bits_t      intpSmoothThre;         //10bit * 3
	struct _RgbChannel8Bits_t       intpDeltaGain;          //U0.8 * 3
	struct _RgbChannel16Bits_t      intpDeltaThre;          //10bit * 3
	struct _RgbChannel16Bits_t      intpDeltaLowSat;        //16bit * 3
	struct _RgbChannel16Bits_t      intpDeltaHighSat;       //16bit * 3
	struct _RgbChannel8Bits_t       intpSmoothGainSat;      //U0.8 * 3
	struct _RgbChannel16Bits_t      intpSmoothThreSat;      //16bit * 3
	struct _RgbChannel8Bits_t       intpDeltaGainSat;       //U0.8 * 3
	struct _RgbChannel16Bits_t      intpDeltaThreSat;       //16bit * 3

	//Lsnr calculation
	unsigned char                   lowSnrWinSel;           //2bit
	unsigned char                   lowSnrPixSel;           //1bit
	struct _RgbChannel16Bits_t      fusionNoiseThre;        //10bit * 3
	unsigned char                   fusionNoiseGlCntThre;   //4bit
	unsigned char                   fusionNoiseGsCntThre;   //4bit
	unsigned char                   fusionNoiseRbCntThre;   //4bit

	//Saturation
	struct _RgbChannel16Bits_t      fusionSatThre;          //12bit * 3

	unsigned char                   fusionNoiseRblCntTh;    //2bit
	unsigned char                   satWinSel;              //2bit
	unsigned char                   satPixSel;              //1bit
	unsigned char                   satGlCntTh;             //4bit
	unsigned char                   satGsCntTh;             //4bit
	unsigned char                   satRbsCntTh;            //4bit
	unsigned char                   satRblCntTh;            //4bit
	unsigned short                  wbRgain;            //2.14
	unsigned short                  wbGgain;            //2.14
	unsigned short                  wbBgain;            //2.14
};

struct _TdbRatioModulationZzhdr_t {
	unsigned short ratio;
	struct _TdbZzhdrRatioParams_t zzhdrParams;
};

struct _TdbZzhdrByRatio_t {
	unsigned int validSize;
	struct _TdbRatioModulationZzhdr_t
		params[CDB_COMMON_MODULATION_RATIO_NUM];
};

struct _TdbZzhdrItimeDiffParams_t {
	//Mot calculation
	struct _RgbChannel16Bits_t      motNoiseLongGain1; //U2.8 * 3
	struct _RgbChannel16Bits_t      motNoiseLongOft1; //10bit * 3
	struct _RgbChannel16Bits_t      motNoiseShortGain1; //U2.8 * 3
	struct _RgbChannel16Bits_t      motNoiseShortOft1; //10bit * 3
	struct _RgbChannel16Bits_t      motNoiseLongGain2; //U2.8 * 3
	struct _RgbChannel16Bits_t      motNoiseLongOft2; //10bit * 3
	struct _RgbChannel16Bits_t      motNoiseShortGain2; //U2.8 * 3
	struct _RgbChannel16Bits_t      motNoiseShortOft2; //10bit * 3
	struct _RgbChannel16Bits_t      motNoiseLongGain3; //U2.8 * 3
	struct _RgbChannel16Bits_t      motNoiseLongOft3; //10bit * 3
	struct _RgbChannel16Bits_t      motNoiseShortGain3; //U2.8 * 3
	struct _RgbChannel16Bits_t      motNoiseShortOft3; //10bit * 3
	struct _RgbChannel8Bits_t       motThreBldWgt1; //U1.6 * 3
	struct _RgbChannel8Bits_t       motThreBldWgt2; //U1.6 * 3
	struct _RgbChannel8Bits_t       motThreBldWgt3; //U1.6 * 3
	struct _RgbChannel16Bits_t      motThreMin; //10bit * 3
	struct _RgbChannel16Bits_t      motThreMax; //10bit * 3
	struct _RgbChannel8Bits_t       motCntThre; //5bit * 3
	unsigned char                   fusionLsnrWgt; //range:0-64
	unsigned char                   fusionSatWgt; //range:0-64
	unsigned char                   fusionMotWgt; //range:0-64
	unsigned char                   fusionNorWgt; //range:0-64
	unsigned char                   fusionNorLongWgt; //range:0-64
	//range:0-64   if sigNsatLsnr=0, Low>=High; else Low <= High
	unsigned char                   bldLsnrWgtLow;
	unsigned char                   bldLsnrWgtHigh; //range:0-64
	unsigned short                  bldLsnrThreHigh; //12bit
	unsigned short                  bldLsnrThreLow; //12bit
	unsigned short                  bldLsnrThreSlope; //U1.9
	// if sigSatNlsnr=0, Low>=High; else Low <= High
	unsigned char                   bldSatWgtLow; //range:0-64
	unsigned char                   bldSatWgtHigh; //range:0-64
	unsigned short                  bldSatThreHigh; //12bit
	unsigned short                  bldSatThreLow; //12bit
	unsigned short                  bldSatThreSlope; //U1.9
	// if sigNsatNlsnrMot=0 Low>=High; else Low <= High
	unsigned char                   bldMotWgtLow; //range:0-64
	unsigned char                   bldMotWgtHigh; //range:0-64
	unsigned short                  bldMotThreHigh; //12bit
	unsigned short                  bldMotThreLow; //12bit
	unsigned short                  bldMotThreSlope; //U1.9
	//range:0-64  if sigNsatNlsnrS =0 Low>=High; else Low <= High
	unsigned char                   bldNorWgtLow;
	unsigned char                   bldNorWgtHigh; //range:0-64
	unsigned short                  bldNorThreHigh; //12bit
	unsigned short                  bldNorThreLow; //12bit
	unsigned short                  bldNorThreSlope; //U1.9
	//range:0-64  if sigNsatNlsnrL =0 Low>=High; else Low <= High
	unsigned char                   bldNorLongWgtLow;
	//range:0-64
	unsigned char                   bldNorLongWgtHigh;
	//12bit
	unsigned short                  bldNorLongThreHigh;
	//12bit
	unsigned short                  bldNorLongThreLow;
	//U1.9
	unsigned short                  bldNorLongThreSlope;
	//U7 bit
	unsigned char                   filterMaskWgt[3];
	//U5 bit
	unsigned char                   filterTransWgt[2];
	//range:0-64  if sigSatNlsnrMot=0 Wl>=Wh; else Wl <= Wh
	unsigned char                   bldMotSatNlsnrWl;
	//range:0-64
	unsigned char                   bldMotSatNlsnrWh;
	//12bit
	unsigned short                  bldMotSatNlsnrThH;
	//12bit
	unsigned short                  bldMotSatNlsnrThL;
	//U1.9
	unsigned short                  bldMotSatNlsnrThS;
	//range:0-64
	unsigned char                   fusionMotSatNlsnrWgt;
	//range:0-64   if sigSatLsnrMot=0 Wl>=Wh; else Wl <= Wh
	unsigned char                   bldMotSatLsnrWl;
	//range:0-64
	unsigned char                   bldMotSatLsnrWh;
	//12bit
	unsigned short                  bldMotSatLsnrThH;
	//12bit
	unsigned short                  bldMotSatLsnrThL;
	//U1.9
	unsigned short                  bldMotSatLsnrThS;
	//range:0-64
	unsigned char                   fusionMotSatLsnrWgt;
	//range:0-64  if sigNsatLsnrMot=0 Wl>=Wh; else Wl <= Wh
	unsigned char                   bldMotNsatLsnrWl;
	//range:0-64
	unsigned char                   bldMotNsatLsnrWh;
	//12bit
	unsigned short                  bldMotNsatLsnrThH;
	//12bit
	unsigned short                  bldMotNsatLsnrThL;
	//U1.9
	unsigned short                  bldMotNsatLsnrThS;
	//range:0-64
	unsigned char					fusionMotNsatLsnrWgt;
	unsigned char                   grayAlphaX1;        //u6.0
	unsigned char                   grayAlphaX2;        //u6.0
	unsigned char                   grayAlphaY1;        //u7.0
	unsigned char                   grayAlphaY2;        //u7.0
	unsigned short                  grayAlphaS;         //u4.9
	unsigned char                   wgtTransCtrl;       //u1.0
	unsigned char                   grayTransCtrl;      //u1.0
};

struct _TdbItimeDiffModulationZzhdr_t {
	unsigned short itimeDiff;
	struct _TdbZzhdrItimeDiffParams_t zzhdrParams;
};

struct _TdbZzhdrByItimeDiff_t {
	unsigned int validSize;
	struct _TdbItimeDiffModulationZzhdr_t
		params[CDB_COMMON_MODULATION_ITIME_NUM];
};

struct _TdbZzhdr_t {
	bool enabled;
	bool pdEnabled;
	//0: dynamic alg  1: fixed
	unsigned char                   outputMode;         //1bit
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
	// must set sigSatNlsnr =  sigSatLsnr
	unsigned char                   sigSatNlsnr;        //u1.0
	unsigned char                   sigSatLsnr;         //u1.0
	unsigned char                   sigNsatNlsnrL;      //u1.0
	unsigned char                   sigNsatNlsnrS;      //u1.0
	struct _TdbZzhdrByRatio_t		paramsByRatio;
	struct _TdbZzhdrByItimeDiff_t	paramsByItimeDiff;
};

struct _TdbPreLscGlobal_t {
	unsigned short    lscXGradTbl[8];
	unsigned short    lscYGradTbl[8];
	unsigned short    lscXSizeTbl[8];
	unsigned short    lscYSizeTbl[8];
};

struct _TdbPreLscMatrix_t {
	struct _Tdb17x17UShortMatrix_t
		lscMatrix[TDB_4CH_COLOR_COMPONENT_MAX];
};

struct _TdbPreLscData_t {
	struct _TdbPreLscGlobal_t
		lscGlobal;
	struct _TdbPreLscMatrix_t
		lscProfile[TDB_CONFIG_PRE_LSC_MAX_ILLU_PROFILES];
};

struct _TdbPreLsc_t {
	bool enabled;
	//The illuminations contained for PRE LSC. Should be less than
	//or equal to TDB_CONFIG_PRE_LSC_MAX_ILLU_PROFILES
	unsigned int illuNum;
	struct _TdbPreLscData_t         data;
};

/*Struct define for Pre Pipe Overall*/
struct _PrePipeTdb_t {
	struct _TdbBlsMeasure_t	blsMeasure;
	struct _TdbDegamma_t	degamma;
	struct _TdbDir_t	dir;
	struct _TdbPde_t	pde;
	struct _TdbPdc_t	pdc;
	struct _TdbPdnc_t	pdnc;
	struct _TdbBpc_t	bpc;
	struct _TdbPdcip_t	pdcip;
	struct _TdbHbin_t	hbin;
	struct _TdbZzhdr_t	zzhdr;
	//TODO: update when janice has result
	struct _TdbPreLsc_t	preLsc;
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
#define CDB_CMOS_EXPOSURE_PATITION_LUT_NUM      (2)
#define CDB_AE_CONTROL_HDR_TARGET_NUM           (8)

//Dynamic Shading
#define CDB_SHADING_CCT_TH_NUM           (8)

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

#ifdef ENABLE_SCR
struct _CdbCustSettingsValue_t {
	unsigned int reg;
	unsigned int value;
};
#endif

struct _CdbShadingRadialCoeff_t {
	unsigned short uCoeff[129];
};

struct _CdbShadingRadial_t {
	struct _CdbShadingRadialCoeff_t
		shading[CDB_3CH_COLOR_COMPONENT_MAX];
};

struct _CdbShadingRadialCentreAndMult_t {
	unsigned short centerRx;
	unsigned short centerRy;
	unsigned short offCenterMultRx;
	unsigned short offCenterMultRy;
	unsigned short centerGx;
	unsigned short centerGy;
	unsigned short offCenterMultGx;
	unsigned short offCenterMultGy;
	unsigned short centerBx;
	unsigned short centerBy;
	unsigned short offCenterMultBx;
	unsigned short offCenterMultBy;
	unsigned short centerIrX;
	unsigned short centerIrY;
	unsigned short offCenterMultIrX;
	unsigned short offCenterMultIRY;
};

// @ModuleId: CALIB_COREPIPE_RADIAL_SHADING
struct _CdbRadialShading_t {
	struct _CdbShadingRadial_t
		shadingRadial;
	struct _CdbShadingRadialCentreAndMult_t
		shadingRadialCentreAndMult;
};

// @ModuleId: CALIB_COREPIPE_DECOMPANDER
struct _CdbDecompanderMem_t {
	unsigned int decompander0[33];
	unsigned int decompander1[257];
};


// @ModuleId: CALIB_COREPIPE_BLACKLEVEL
struct _CdbBlackLevel_t {
	struct _CdbGainToBL_t
		blackLevel[CDB_4CH_COLOR_COMPONENT_MAX];
};

struct _CdbCcmCoeff_t {
	unsigned short
		uCoeff[9];
};

struct _CdbAbsoluteCcm_t {
	unsigned int validSize;
	struct _CdbCcmCoeff_t
		ccm[CDB_CCM_COMPONENT_MAX];
};

struct _CdbShadingChannelCoeff_t {
	unsigned char
		uCoeff[1024];
};

struct _CdbShadingChannel_t {
	struct _CdbShadingChannelCoeff_t
		shadingCoeff[CDB_3CH_COLOR_COMPONENT_IR];
};

struct _CdbMeshShadingLUT_t {
	unsigned int validSize;
	struct _CdbShadingChannel_t
		shadingChannel[CDB_SHADING_COMPONENT_MAX];
};

struct _CdbMeshShadingStrength_t {
	unsigned int validSize;
	struct _CdbGainModulationCoeff_t
		strength[CDB_COMMON_MODULATION_NUM];
};
struct _CdbCctThreshold_t {
	unsigned int validSize;
	unsigned short threshold[CDB_SHADING_CCT_TH_NUM];
};

struct _CdbMeshShadingSmoothRatio_t {
	unsigned char ratio;
};

// @ModuleId: CALIB_COREPIPE_MESH_SHADING
struct _CdbShading_t {
	struct _CdbCctThreshold_t cctThreshold;
	unsigned char meshAlphaMode;
	unsigned char meshScale;
	unsigned char meshWidth;
	unsigned char meshHeight;
	struct _CdbMeshShadingStrength_t
		meshShadingStrength;
#ifdef SCENE_MODE_IMPLEMENT
	struct _CdbMeshShadingSmoothRatio_t	smoothRatio;
#endif
	struct _CdbMeshShadingLUT_t
		meshShadingLut;
};

struct _CdbAwbWarmingCoeff_t {
	unsigned short uCoeff[3];
};


struct _CdbAwbWarmingCctCoeff_t {
	unsigned short uCoeff[4];
};


//The sinter and temper only has 32 lut registers,
//but calib data has 128 params. Each register should
//set 4 values to 4 fields.
//The wdrNp has l/m/s/vs 4 channels, each channel has 32 params.
struct _CdbLut_t {
	unsigned char uCoeff[128];
};

// @ModuleId: CALIB_COREPIPE_GAMMA
struct _CdbGamma_t {
	unsigned short lvThreshod[5];
	//This is co-work with lvThreshod to decide which gamma curve be used
	unsigned short gammaCurveIdx[5];
	unsigned short gammaCurveEv1[129]; //non Face, LV1
	unsigned short gammaCurveEv2[129]; //non Face, LV2
	unsigned short gammaCurveFaceEv1[129]; //Face, LV1
	unsigned short gammaCurveFaceEv2[129]; //Face, LV2
};

struct _CdbIrdixAsym_t {
	unsigned int uCoeff[65];
};

struct _CdbAwbScenePresetsCoeff_t {
	unsigned short cr;
	unsigned short cb;
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
#ifndef CAC_NEW_IMPLEMENT
	unsigned int validSize;
	struct _CdbCaCorrectionMemCoeff_t
		caMemCoeff[CDB_CA_MEM_NUM];
#else
	unsigned int caMemCoeff[4096];
#endif
};

// @ModuleId: CALIB_COREPIPE_CAC
struct _CdbCAC_t {
	unsigned char	meshScale;
	unsigned char	meshHeight;
	unsigned char	meshWidth;
	unsigned short	planeOffset;
	unsigned short	lineOffset;
#ifndef CAC_NEW_IMPLEMENT
	unsigned short	caMinCorrection;
#endif
	struct _CdbCaFilterMem_t	caFilterMem;
	struct _CdbCaCorrectionMem_t	caCorrectionMem;
};

// @ModuleId: CALIB_COREPIPE_LUT_3D
struct _CdbLut3d_t {
	unsigned int uCoeff[1000];
};

struct _CdbNonEquGamma_t {
	unsigned short uCoeff[65];
};

struct _CdbSinterRadialLut_t {
	unsigned char lut[33];
};

struct _CdbSinterStrength4_t {
	unsigned int validSize;
	struct _CdbGainModulationCoeff_t
		strength4[CDB_COMMON_MODULATION_NUM];
};

struct _CdbSinterStrength1_t {
	unsigned int validSize;
	struct _CdbGainModulationCoeff_t
		strength1[CDB_COMMON_MODULATION_NUM];
};

struct _CdbSinterThresh1H_t {
	unsigned int validSize;
	struct _CdbGainModulationCoeff_t
		thresh1H[CDB_COMMON_MODULATION_NUM];
};

struct _CdbSinterThresh1V_t {
	unsigned int validSize;
	struct _CdbGainModulationCoeff_t
		thresh1V[CDB_COMMON_MODULATION_NUM];
};

struct _CdbSinterThresh4H_t {
	unsigned int validSize;
	struct _CdbGainModulationCoeff_t
		thresh4H[CDB_COMMON_MODULATION_NUM];
};

struct _CdbSinterThresh4V_t {
	unsigned int validSize;
	struct _CdbGainModulationCoeff_t
		thresh4V[CDB_COMMON_MODULATION_NUM];
};

struct _CdbSinterIntconfig_t {
	unsigned int validSize;
	struct _CdbGainModulationCoeff_t
		intconfig[CDB_COMMON_MODULATION_NUM];
};

struct _CdbSinterSad_t {
	unsigned int validSize;
	struct _CdbGainModulationCoeff_t
		sad[CDB_COMMON_MODULATION_NUM];
};

struct _CdbSinterGlobalOffset_t {
	unsigned int validSize;
	struct _CdbGainModulationCoeff_t
		globalOffset[CDB_COMMON_MODULATION_NUM];
};

struct _CdbSinterNoiseLevel_t {
	unsigned char noiseLevel0;
	unsigned char noiseLevel1;
	unsigned char noiseLevel2;
	unsigned char noiseLevel3;
};

// @ModuleId: CALIB_COREPIPE_SINTER
struct _CdbSinter_t {
	bool rmEnable;
	struct _CdbSinterIntconfig_t            intconfig;
	struct _CdbSinterSad_t                  sad;
	struct _CdbSinterThresh1H_t             thresh1H;
	struct _CdbSinterThresh4H_t             thresh4H;
	struct _CdbSinterThresh1V_t             thresh1V;
	struct _CdbSinterThresh4V_t             thresh4V;
	struct _CdbSinterStrength1_t            strength1;
	struct _CdbSinterStrength4_t            strength4;
	struct _CdbSinterGlobalOffset_t         globalOffset;
	struct _CdbSinterNoiseLevel_t           noiseLevel;
	struct _CdbSinterRadialLut_t            radialLut;
};

// @ModuleId: CALIB_COREPIPE_TEMPER
struct _CdbTemperGlobalOffset_t {
	unsigned int validSize;
	struct _CdbGainModulationCoeff_t
		globalOffset[CDB_COMMON_MODULATION_NUM];
};

enum _TemperMode_t {
	TEMPER_MODE_INVALID = -1,
	TEMPER_MODE_3,
	TEMPER_MODE_2,
	TEMPER_MODE_MAX
};

struct _CdbTemper_t {
	enum _TemperMode_t mode;
	unsigned char recLim;
	struct _CdbTemperGlobalOffset_t globalOffset;
};

struct _CdbSharpAltD_t {
	unsigned int validSize;
	struct _CdbGainModulationCoeff_t
		altD[CDB_COMMON_MODULATION_NUM];
};

struct _CdbSharpAltUd_t {
	unsigned int validSize;
	struct _CdbGainModulationCoeff_t
		altUd[CDB_COMMON_MODULATION_NUM];
};

struct _CdbSharpAltDu_t {
	unsigned int validSize;
	struct _CdbGainModulationCoeff_t
		altDu[CDB_COMMON_MODULATION_NUM];
};

// @ModuleId: CALIB_COREPIPE_SHARP_DEMOSAIC
struct _CdbSharp_t {
	struct _CdbSharpAltD_t  altD;
	struct _CdbSharpAltUd_t altUd;
	struct _CdbSharpAltDu_t altDu;
};

// @ModuleId: CALIB_COREPIPE_SHARPEN_FR
struct _CdbSharpenFrStrength_t {
	unsigned int validSize;
	struct _CdbGainModulationCoeff_t
		strength[CDB_COMMON_MODULATION_NUM];
};

struct _CdbDemosaicNpOffset_t {
	unsigned int validSize;
	struct _CdbGainModulationCoeff_t
		npOffset[CDB_COMMON_MODULATION_NUM];
};

struct _CdbDemosaicUuSlope_t {
	unsigned int validSize;
	struct _CdbGainModulationU8Coeff_t
		uuSlop[CDB_COMMON_MODULATION_NUM];
};

struct _CdbDemosaicSharpAltLu_t {
	unsigned int validSize;
	struct _CdbGainModulationU8Coeff_t
		altLu[CDB_COMMON_MODULATION_NUM];
};

struct _CdbDemosaicSharpAltLdu_t {
	unsigned int validSize;
	struct _CdbGainModulationU8Coeff_t
		altLdu[CDB_COMMON_MODULATION_NUM];
};

struct _CdbDemosaicSharpAltLd_t {
	unsigned int validSize;
	struct _CdbGainModulationU8Coeff_t
		altLd[CDB_COMMON_MODULATION_NUM];
};

struct _CdbDemosaicFcAliasSlop_t {
	unsigned int validSize;
	struct _CdbGainModulationU8Coeff_t
		fcAliasSlop[CDB_COMMON_MODULATION_NUM];
};

struct _CdbDemosaicFcSlop_t {
	unsigned int validSize;
	struct _CdbGainModulationU8Coeff_t
		fcSlop[CDB_COMMON_MODULATION_NUM];
};
struct _CdbDemosaicUuShSlope_t {
	unsigned int validSize;
	struct _CdbGainModulationU8Coeff_t
		uuShSlop[CDB_COMMON_MODULATION_NUM];
};

// @ModuleId: CALIB_COREPIPE_DEMOSAIC
struct _CdbDemosaic_t {
#ifndef CONFIG_DISABLE_TDB_NEW_ELEMENT
	struct _CdbDemosaicUuSlope_t        uuSlop;
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
	struct _CdbDemosaicFcAliasSlop_t    fcAliasSlop;
	struct _CdbDemosaicFcSlop_t         fcSlop;
	struct _CdbDemosaicSharpAltLu_t     sharpAltLu;
	struct _CdbDemosaicSharpAltLdu_t    sharpAltLdu;
	struct _CdbDemosaicSharpAltLd_t     sharpAltLd;
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
#ifdef SCENE_MODE_IMPLEMENT
	struct _CdbDemosaicUuShSlope_t	uuShSlop;//Add per ALGO request
#endif
	struct _CdbLut_t                    dmsc;
#else
	struct _CdbLut_t                    dmsc;
	//noise profile offset
	struct _CdbDemosaicNpOffset_t       demosaicNpOffset;
#endif
};

struct _CdbSaturationStrength_t {
	unsigned int validSize;
	struct _CdbGainModulationCoeff_t
		strength[CDB_COMMON_MODULATION_NUM];
};

// @ModuleId: CALIB_COREPIPE_CCM
struct _CdbCcm_t {
	unsigned short ccmOneGainThreshold; //for ccm switch
	struct _CdbAbsoluteCcm_t absoluteCcm;
	struct _CdbSaturationStrength_t saturationStrength;  //for ccm
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
struct _CdbPf_t {
	unsigned short hueStrength;
	unsigned short lumaStrength;
	unsigned short satStrength;
	unsigned short saturationStrength;
	unsigned short purpleStrength;
	unsigned short sadOffset;
	unsigned short sadSlope;
	unsigned short sadThresh;
	unsigned short hueLowOffset;
	unsigned short hueLowSlope;
	unsigned short hueLowThresh;
	unsigned short hueHighOffset;
	unsigned short hueHighSlope;
	unsigned short hueHighThresh;
	unsigned short satLowOffset;
	unsigned short satLowSlope;
	unsigned short satLowThresh;
	unsigned short satHighOffset;
	unsigned short satHighSlope;
	unsigned short satHighThresh;
	unsigned short luma1LowOffset;
	unsigned short luma1LowSlope;
	unsigned short luma1LowThresh;
	unsigned short luma1HighOffset;
	unsigned short luma1HighSlope;
	unsigned short luma1HighThresh;
	unsigned short luma2LowOffset;
	unsigned short luma2LowSlope;
	unsigned short luma2LowThresh;
	unsigned short luma2HighOffset;
	unsigned short luma2HighSlope;
	unsigned short luma2HighThresh;
	unsigned short hslOffset;
	unsigned short hslSlope;
	unsigned short hslThresh;
	unsigned char debugSel;
	struct _CdbPfRadialLut_t    radialLut;
};

//strength dark enhancement control
struct  _CdbIridix8StrengthDkEnhCtl_t {
	unsigned int darkPrc; // dark_prc
	unsigned int brightPrc; // bright_prc
	unsigned int minDk; // min_dk: minimum dark enhancement
	unsigned int maxDk; // max_dk: maximum dark enhancement
	// pD_cut_min: minimum intensity cut for dark regions
	// in which dk_enh will be applied
	unsigned int pdCutMin;
	// pD_cut_max: maximum intensity cut for dark
	// regions in which dk_enh will be applied
	unsigned int pdCutMax;
	unsigned int darkContrastMin; // dark contrast min
	unsigned int darkContrastMax; // dark contrast max
	unsigned int minStr; // min_str: iridix strength in percentage
	// max_str: iridix strength in percentage:
	// 50 = 1x gain. 100 = 2x gain
	unsigned int maxStr;
	// dark_prc_gain_target: target in histogram (percentage)
	// for dark_prc after iridix is applied
	unsigned int darkPrcGainTarget;
	// contrast_min: clip factor of strength for LDR scenes.
	unsigned int contrastMin;
	// contrast_max: clip factor of strength for HDR scenes.
	unsigned int contrastMax;
	unsigned int maxIridixGain;  // max iridix gain
	unsigned int printDebug; // print debug
};

// @ModuleId: CALIB_COREPIPE_IRIDIX
struct _CdbIridixReg_t {
	unsigned short	gainVale;
	unsigned char	slopMin;
	unsigned char	slopMax;
	unsigned char	varInt;
	unsigned char	varSpace;
	unsigned int	blackLevel;
	unsigned int	whiteLevel;
	unsigned short	strengthInroi;
	unsigned char	revPerceptCtrl;
	unsigned char	fwdPerceptCtrl;
	unsigned short	strengthOutroi;
	unsigned short	roiHorStart;
	unsigned short	roiHorEnd;
	unsigned short	roiVerStart;
	unsigned short	roiVerEnd;
	unsigned char	contrast;
	unsigned char	brightPr;
	unsigned char	svariance;
	unsigned short	darkEnh;
	unsigned char	GtmSel;
	struct _CdbIrdixAsym_t	irdixAsym;
};

struct _CdbIridix_t {
	unsigned char	avgCoef;
	unsigned short	minMaxStr;
	unsigned int	evLimFullStr;
	unsigned int	evLimNoStr[2];
	unsigned char	strengthMaximum;
	struct _CdbIrdixAsym_t	irdixAsym;
	struct _CdbIridix8StrengthDkEnhCtl_t	strengthDkEnhCtl;
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
	struct _CdbGainModulationCoeff_t
		slop[CDB_COMMON_MODULATION_NUM];
};

struct _CdbDpThreshold_t {
	unsigned int validSize;
	struct _CdbGainModulationCoeff_t
		threshold[CDB_COMMON_MODULATION_NUM];
};

// @ModuleId: CALIB_COREPIPE_DP_CORRECTION
struct _CdbDPCorrection_t {
	struct _CdbDpSlop_t         dpSlop;
	struct _CdbDpThreshold_t    dpThreshold;
};

struct _CdbStitchingLMMedNoiseIntensityThresh_t {
	unsigned int validSize;
	struct _CdbGainModulationCoeff_t
		thresh[CDB_COMMON_MODULATION_NUM];
};

struct _CdbStitchingLmMovMult_t {
	unsigned int validSize;
	struct _CdbGainModulationCoeff_t
		lmMovMult[CDB_COMMON_MODULATION_NUM];
};

struct _CdbStitchingMsMovMult_t {
	unsigned int validSize;
	struct _CdbGainModulationCoeff_t
		msMovMult[CDB_COMMON_MODULATION_NUM];
};

struct _CdbStitchingSvsMovMult_t {
	unsigned int validSize;
	struct _CdbGainModulationCoeff_t
		svsMovMult[CDB_COMMON_MODULATION_NUM];
};

struct _CdbStitchingLmNp_t {
	unsigned int validSize;
	struct _CdbGainModulationCoeff_t
		lmNp[CDB_COMMON_MODULATION_NUM];
};

struct _CdbStitchingMsNp_t {
	unsigned int validSize;
	struct _CdbGainModulationCoeff_t
		msNp[CDB_COMMON_MODULATION_NUM];
};

struct _CdbStitchingSvsNp_t {
	unsigned int validSize;
	struct _CdbGainModulationCoeff_t
		svsNp[CDB_COMMON_MODULATION_NUM];
};

// @ModuleId: CALIB_COREPIPE_STITCHING
struct _CdbStitching_t {
	struct _CdbStitchingLMMedNoiseIntensityThresh_t
		lmMedNoiseIntensityTh;
	struct _CdbStitchingLmMovMult_t                     lmMovMult;
	struct _CdbStitchingLmNp_t                          lmNp;
	struct _CdbStitchingMsMovMult_t                     msMovMult;
	struct _CdbStitchingMsNp_t                          msNp;
	struct _CdbStitchingSvsMovMult_t                    svsMovMult;
	struct _CdbStitchingSvsNp_t                         svsNp;
	unsigned short                                      fsMcOff;
};


struct _CdbCalibraitonAeControl_t {
	unsigned int aeConvergance;   // AE convergance
	// LDR AE target -> this should match the 18% grey of
	//the output gamma
	unsigned int ldrAeTarget;
	unsigned int aeTailWeight;    // AE tail weight
	// WDR mode only: Max percentage of clipped pixels for long exposure:
	//WDR mode only: 256 = 100% clipped pixels
	unsigned int clippedPerLong;
	// WDR mode only: Time filter for exposure ratio
	unsigned int timeFilter;
	// control for clipping: bright percentage of pixels that should
	// be below hi_target_prc
	unsigned int brightPrc;
	// control for clipping: highlights percentage (hi_target_prc):
	// target for tail of histogram
	unsigned int hiTargetPrc;
	// 1:0 enable | disable iridix global gain.
	unsigned int irGdgEnable;
	// AE tolerance
	unsigned int aeTolerance;
};

// @ModuleId: CALIB_COREPIPE_RGB2YUV_CONV
struct _CdbRgb2YuvConversion_t {
	unsigned short csMatrix[9];
};


// @ModuleId: CALIB_COREPIPE_CNR
struct _CdbCnrUvDelta12Slop_t {
	unsigned int validSize;
	struct _CdbGainModulationCoeff_t
		slop[CDB_COMMON_MODULATION_NUM];
};

// @ModuleId: CALIB_COREPIPE_WDR_NP
struct _CdbWdrNoiseProfile_t {
	struct _CdbLut_t     wdrNp;
};

// @ModuleId: CALIB_COREPIPE_NOISE_PROFILE
struct _CdbNoiseProfile_t {
	struct _CdbLut_t     np;
};

#ifdef ENABLE_SCR
// @ModuleId: CALIB_COREPIPE_CUST_SETTINGS
struct _CdbCustSettings_t {
	unsigned int validSize;
	struct _CdbCustSettingsValue_t
		custSetting[CDB_CUST_SETTINGS_NUM];
};
#endif

enum _CdbCFAPattern_t {
	CDB_CFA_PATTERN_INVALID      = -1,
	CDB_CFA_PATTERN_RGGB         = 0,
	CDB_CFA_PATTERN_RESERVED     = 1,
	CDB_CFA_PATTERN_RIRGB        = 2,
	CDB_CFA_PATTERN_RGIRB        = 3,
	CDB_CFA_PATTERN_MAX
};

enum _CdbRggbPattern_t {
	PATTERN_INVALID     = -1,
	PATTERN_RGGB		 = 0,
	PATTERN_GRBG		 = 1,
	PATTERN_GBRG		 = 2,
	PATTERN_BGGR		 = 3,
	PATTERN_MAX
};

struct _CdbTopCfg_t {
	unsigned short width;
	unsigned short height;
	unsigned char linearDataSrc;
	enum _CdbCFAPattern_t cfaPattern;
	enum _CdbRggbPattern_t startPreMirror;
	enum _CdbRggbPattern_t startPostMirror;
	bool bypassInputFormatter;
	bool bypassDecompander;
	bool bypassSensorOffsetWd;
	bool bypassGainWdr;
	bool bypassFrameStitch;
	bool byPassDigitalGain;
	bool byPassDefectPixel;
	bool byPassGE;
	bool byPassDPC;
	bool byPassCaCorrection;
	bool byPassTemper;
	bool byPassSinter;
	bool byPassMeshShading;
	bool byPassRadialShading;
	bool byPassWhiteBalance;
	bool byPassIridix;
	bool byPassIridixGain;
	bool bypassDemosaic;
	bool byPassPfCorrection;
	bool byPassCCM;
	bool bypass3DLUT;
	bool bypassNonequGamma;
	bool bypassFrRgbGamma;
	bool bypassFrShapen;
	bool bypassCsConv;
};

struct _CdbFrGamma_t {
	unsigned short gainR;
	unsigned short gainG;
	unsigned short gainB;
	unsigned short offsetR;
	unsigned short offsetG;
	unsigned short offsetB;
};

struct _CdbFrSharpen_t {
	unsigned char alphaUndershoot;
	unsigned char controlB;
	unsigned char controlR;
	unsigned char  lumaoffsetLow;
	unsigned short lumaThreshLow;
	unsigned short lumaThreshHigh;
	unsigned short lumaSlopeLow;
	unsigned short lumaSlopeHigh;
	unsigned char  lumaoffsetHigh;
	unsigned short clipStrMax;
	unsigned short clipStrMin;
	unsigned char debugSel;
	struct _CdbSharpenFrStrength_t strength;
};

struct _CdbGreenEqualization_t {
	unsigned char   geStrength;
	unsigned char   geSens;
	unsigned short  geThreshold;
	unsigned short  geSlop;
	unsigned short  debugSel;
};

struct _CorePipeCdb_t {
	struct _CdbTopCfg_t topCfg;
	struct _CdbDecompanderMem_t             decompanderMem;
	//stitch niose profile
	struct _CdbWdrNoiseProfile_t            wdrNp;
	struct _CdbStitching_t                  stitching;
	struct _CdbDPCorrection_t               dPCorrection;
	struct _CdbGreenEqualization_t			ge;
	struct _CdbSinter_t                     sinter;
	struct _CdbTemper_t                     temper;
	struct _CdbCAC_t                        cac;
	struct _CdbBlackLevel_t                 blackLevel;
	struct _CdbRadialShading_t              radiaShading;
	//mesh shading   for diff light source
	struct _CdbShading_t                    meshShading;
#ifndef SCENE_MODE_IMPLEMENT
	struct _CdbIridix_t                     iridix;
	struct _CdbIridixReg_t					iridixReg;
#endif
	struct _CdbDemosaic_t                   demosaic;
#ifdef CONFIG_DISABLE_TDB_NEW_ELEMENT
	//notice: sharpening for demosaic
	struct _CdbSharp_t                      sharp;
#endif
	struct _CdbPf_t                         pf;
#ifndef SCENE_MODE_IMPLEMENT

	//should not be used in AMD solution
	struct _CdbCcm_t                        ccm;
#endif
	//uv delta1 slop; uv delta2 slop
	struct _CdbCnrUvDelta12Slop_t           cnrUvDelta12Slop;
	struct _CdbLut3d_t                      lut3d;
	struct _CdbNonEquGamma_t				equGamma;
	//for fr rgb gamma
	struct _CdbAutoLevelControl_t           autoLevelControl;
	//fr rgb gamma
	struct _CdbGamma_t                      gamma;
	struct _CdbFrGamma_t                    FrGamma;
	struct _CdbFrSharpen_t                  FrSharpen;
	//color space conversion
	struct _CdbRgb2YuvConversion_t          rgb2YuvConversion;
	struct _CdbNoiseProfile_t               noiseProfile;
#ifndef SCENE_MODE_IMPLEMENT

	//for iridix
	struct _CdbCalibraitonAeControl_t       aeControl;
#endif
#ifdef ENABLE_SCR
	//remove to reduce the mem size
	struct _CdbCustSettings_t               custSettings;
#endif
};
// 3rd party pipeline calibration data ends here

// Post-pipe calibration data ends here
#define RESIZE_TBL_SIZE        (65)
#define TDB_YUV_RESIZE_SET_NUM (10)
#define RESIZE_CHANNEL_NUM     (4)
struct _YuvResizeTable_t {
	short coeff[RESIZE_TBL_SIZE];
};

//Notice: this struct should align with IspPipeOutCh_t
//in ParamsType.h
enum _CdbIspPipeOutCh_t {
	CDB_ISP_PIPE_OUT_CH_PREVIEW = 0,
	CDB_ISP_PIPE_OUT_CH_VIDEO,
	CDB_ISP_PIPE_OUT_CH_STILL,
	CDB_ISP_PIPE_OUT_CH_CVP_CV,
	CDB_ISP_PIPE_OUT_CH_X86_CV,
	CDB_ISP_PIPE_OUT_CH_IR, //X86 or CVP
	CDB_ISP_PIPE_OUT_CH_STILL_FULL,
	//RAW CH won't need sharpen and yuv scale
	CDB_ISP_PIPE_OUT_CH_RAW,
	CDB_ISP_PIPE_OUT_CH_MAX,
};

#define ISP_PIPE_OUT_CH_NUM     (8)
struct _YuvResize_t {
	//RAW CH won't need sharpen and yuv scale, so minus 1
	struct _YuvResizeTable_t
	table[CDB_ISP_PIPE_OUT_CH_MAX - 1];
};

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
	unsigned short   yuvNrSigmaMinStp;    //0~1023
	unsigned short   yuvNrSigmaMaxStp;    //0~1023

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
	unsigned char   yuvNrSigmaHistMaxCnt; //0~625
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
	unsigned short   yuvNrSigmaMinStp;    //0~1023
	unsigned short   yuvNrSigmaMaxStp;    //0~1023

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
	struct _TdbYuvNrY_t	yuvNrY;
};

struct _TdbGainModulationYuvNrUv_t {
	unsigned short gain;
	struct _TdbYuvNrUv_t yuvNrUv;
};

struct _TdbYuvNrYByGain_t {
	unsigned int validSize;
	struct _TdbGainModulationYuvNrY_t
		params[CDB_COMMON_MODULATION_NUM];
};

struct _TdbYuvNrUvByGain_t {
	unsigned int validSize;
	struct _TdbGainModulationYuvNrUv_t
		params[CDB_COMMON_MODULATION_NUM];
};

struct _YuvNrDynamicAlgoParam_t {
	unsigned char GainPreidctionMode;
	float GainPredictFactor1;
	float GainPredictFactor2;
	float DthPredictFactor1;
	float DthPredictFactor2;
	unsigned int GainLevelMin;
	unsigned int DarkMin;
};

struct _TdbYuvNr_t {
	bool	enabled;
	struct _YuvNrDynamicAlgoParam_t yuvNrAlgoParam;
	struct _TdbYuvNrYByGain_t		yuvNrY;
	struct _TdbYuvNrUvByGain_t		yuvNrUv;
};

/*Struct define for Post Pipe Module Sharpen*/

/*
 * The following tables are the grouped coefficients for
 * high-pass band and middle-pass band.
 * Default settings are 4 for both highBand and MidBand.
 *
 *	highBand_0= [0,-1,-2,-4,-5,-1,3,16,24,16,5,-12,-93,-156,772]
 *	highBand_1= [0,-1,-4,-9,-11,-4,0,18,30,22,18,0,-94,-171,736]
 *	highBand_2= [0,0,-1,-1,-2,-1,-3,-6,-7,4,34,55,-54,-183,568]
 *	highBand_3= [0,0,1,1,2,0,-4,-13,-18,-2,37,69,-42,-185,528]
 *	highBand_4= [-1,2,1,0,4,-2,-11,-10,-5,13,41,25,-33,-188,564]
 *	highBand_5= [0,1,2,4,4,1,-4,-17,-25,-7,38,78,-34,-185,480]
 *	highBand_6= [0,1,3,6,8,2,-4,-21,-32,-11,37,82,-25,-177,436]

 *	MidBand_0= [-2,-1,1,0,-4,-7,-22,-33,-36,-52,-22,7,67,127,216]
 *	MidBand_1= [-3,0,2,1,-3,-4,-15,-32,-41,-46,-31,-3,65,129,224]
 *	MidBand_2= [-2,-1,0,1,0,-1,-9,-30,-41,-35,-37,-22,61,131,244]
 *	MidBand_3= [1,1,-1,0,3,6,3,-17,-27,-21,-56,-58,30,130,304]
 *	MidBand_4= [2,0,-1,-1,-2,5,7,-3,-10,-24,-71,-68,0,135,400]
 *	MidBand_5= [0, 1,1,4,3,0,8,10,6,-17,-65,-72,-35,103,392]
 *	MidBand_6= [0,0,1,3,5,-3,4,13,11,-6,-57,-87,-47,89,440]
 *	MidBand_7= [-1,1,2,1,2,-3,4,13,12,11,-46,-88,-70,70,468]
 *	MidBand_8= [0,0,1,1,1,3,12,28,37,6,-76,-142,-62,83,568]
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
	enum _HighBandIndex_t highBandIndex;

	//middle-passs band index
	enum _MidBandIndex_t midBandIndex;

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
	struct _TdbSharpReg_t sharpReg;
};

struct _TdbSharpRegByRatio_t {
	unsigned int validSize;
	struct _TdbRatioModulationSharp_t
		params[CDB_COMMON_MODULATION_NUM];
};

struct _TdbGainModulationSharp_t {
	unsigned short Gain;
	struct _TdbSharpReg_t sharpReg;
};

struct _TdbSharpRegByGain_t {
	unsigned int validSize;
	struct _TdbGainModulationSharp_t
		params[CDB_COMMON_MODULATION_NUM];
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
#ifdef SHARPEN_RATIO_ENABLE
	//Blending params for ByGain and ByRatio
	//range is 0~256
	unsigned short weight;
#endif
	struct _TdbSharpRegByGain_t sharpByGain;
	struct _TdbSharpRegByRatio_t sharpByRatio;
};

struct _TdbPostSharp_t {
	//RAW CH won't need sharpen and yuv scale, so minus 1
	struct _TdbSharpRegByChannel_t
		sharpen[CDB_ISP_PIPE_OUT_CH_MAX-1];
};

/*Struct define for Post Pipe Overall*/
struct _PostPipeTdb_t {
	struct _YuvResize_t yuvResizeTable;
	struct _TdbYuvNr_t yuvNr;
	struct _TdbPostSharp_t postSharp;
};
// Post-pipe calibration data ends here

#ifndef CONFIG_DISABLE_TDB_NEW_ELEMENT
/*Struct define for AE algorithm*/
struct _AeCdb_t {
#ifdef SCENE_MODE_IMPLEMENT
	struct _ae_tuning_param_t
	tuningData[SCENE_PREFERENCE_TUNING_AAA_SETS_NUM];
#else
	struct _ae_tuning_param_t tuningData;
#endif
	struct _ae_calibration_param_t calibrationData;
};

/*Struct define for AWB algorithm*/
struct _AwbCdb_t {
#ifdef SCENE_MODE_IMPLEMENT
	struct _AwbTuningData_t
	tuningData[SCENE_PREFERENCE_TUNING_AAA_SETS_NUM];
#else
	struct _AwbTuningData_t tuningData;
#endif
	struct _AwbCalibData_t calibrationData;
};

/*Struct define for AF algorithm*/
struct _AfCdb_t {
#ifdef SCENE_MODE_IMPLEMENT
	struct _AfTuningUserParam_t
	tuningData[SCENE_PREFERENCE_TUNING_AAA_SETS_NUM];
#else
	struct _AfTuningUserParam_t tuningData;
#endif
	struct _LensCalibrationParam_t calibData;
};

/*Struct define for Iridix algorithm*/
struct _IridixCdb_t {
#ifdef SCENE_MODE_IMPLEMENT
	struct _IridixTuningData_t
	tuningData[SCENE_PREFERENCE_TUNING_AAA_SETS_NUM];
#else
	struct _IridixTuningData_t tuningData;
#endif
	struct _IridixCalibData_t calibData;
};

#endif // CONFIG_DISABLE_TDB_NEW_ELEMENT

struct _Version_t {
	unsigned int	toolVersion;
	unsigned int	tdbVersion;
	unsigned int	tuningParamVersion;
	unsigned int	reserved[16];
};
enum _SceneMode_t {
	SCENE_MODE_INVALID = -1,
	SCENE_MODE_DEFAULT = 0,
	SCENE_MODE_ACTION,
	SCENE_MODE_PORTRAIT,
	SCENE_MODE_LANDSCAPE,
	SCENE_MODE_NIGHT,
	SCENE_MODE_NIGHT_PORTRAIT,
	SCENE_MODE_THEATRE,
	SCENE_MODE_BEACH,
	SCENE_MODE_SNOW,
	SCENE_MODE_SUNSET,
	SCENE_MODE_STEADYPHOTO,
	SCENE_MODE_FIREWORKS,
	SCENE_MODE_SPORTS,
	SCENE_MODE_PARTY,
	SCENE_MODE_CANDLELIGHT,
	SCENE_MODE_BARCODE,
	SCENE_MODE_HIGH_SPEED_VIDEO,
	SCENE_MODE_HDR,
	SCENE_MODE_MACRO,
	SCENE_MODE_BACK_LIGHT,
	SCENE_MODE_FACE_PRIORITY,
	SCENE_MODE_MEDICAL,
	SCENE_MODE_MAX
};
enum _ScenarioMode_t {
	SCENARIO_MODE_PREVIEW = 0,
	SCENARIO_MODE_STILL,
	SCENARIO_MODE_VIDEO,
	SCENARIO_MODE_MAX,
};

struct _Lut3D_t {
	bool enabled;
	//3D LUT
	struct _ColorTuningData_t
		colorCdb[SCENE_PREFERENCE_TUNING_AAA_SETS_NUM];
};

struct _AutoContrast_t {
	bool enabled;
	struct _autoContrastTuningParamT
		autoContrastCdb[SCENE_PREFERENCE_TUNING_AAA_SETS_NUM];
};

#ifndef SCENE_MODE_IMPLEMENT
struct _SensorTdb_t {
	struct _Version_t	version;
	struct _PrePipeTdb_t	prePipeTdb;
	struct _CorePipeCdb_t	corePipeCdb;
	struct _PostPipeTdb_t	postPipeTdb;
#ifndef CONFIG_DISABLE_TDB_NEW_ELEMENT
	struct _AeCdb_t	aeCdb;
	struct _PLineTable_t	plineCdb;
	struct _AwbCdb_t	awbCdb;
	struct _AfCdb_t	afCdb;
	struct _IridixCdb_t	iridixCdb;
#endif // CONFIG_DISABLE_TDB_NEW_ELEMENT
	//SceneMode_t	sceneMode;
};

#else	//SCENE_MODE_IMPLEMENT

#include "LscTuningParam.h"

struct _SceneModePipeDelta_t {
	struct _CdbSinter_t
		sinter[SCENE_PREFERENCE_TUNING_PIPE_SETS_NUM];
	struct _CdbTemper_t
		temper[SCENE_PREFERENCE_TUNING_PIPE_SETS_NUM];
	struct _TdbYuvNr_t
		yuvNr[SCENE_PREFERENCE_TUNING_PIPE_SETS_NUM];
	struct _TdbPostSharp_t
		postSharp[SCENE_PREFERENCE_TUNING_PIPE_SETS_NUM];
};

struct _SceneMappingPipeDelta_t {
	bool	   bSinterUpdate;
    //sinterIdx < SCENE_PREFERENCE_TUNING_PIPE_SETS_NUM
	unsigned int	   sinterIdx;
	bool	   bTemperUpdate;
    //temperIdx < SCENE_PREFERENCE_TUNING_PIPE_SETS_NUM
	unsigned int	   temperIdx;
	bool	   bSharpenUpdate;
    //sharpenIdx < SCENE_PREFERENCE_TUNING_PIPE_SETS_NUM
	unsigned int	   sharpenIdx;
	bool	   bYuvNrUpdate;
    //yuvNrIdx < SCENE_PREFERENCE_TUNING_PIPE_SETS_NUM
	unsigned int	   yuvNrIdx;
};

struct _PipeMappingDelta_t {
	struct _SceneMappingPipeDelta_t     sceneMappingPipe[SCENE_MODE_MAX];
};

struct _SceneMappingAaa_t {
    //All Idx should<SCENE_PREFERENCE_TUNING_AAA_SETS_NUM
	unsigned int	   aeTableIdx;
	unsigned int	   afTableIdx;
	unsigned int	   awbTableIdx;
	unsigned int	   iridixTableIdx;
	unsigned int	   plineTableIdx;
	unsigned int	   color3DLutTableIdx;
	unsigned int	   autoContrastTableIdx;
	unsigned int	   lscTableIdx;

};

struct _MappingAaa_t {
	struct _SceneMappingAaa_t
		sceneMapping[SCENE_MODE_MAX];
};
struct _LscCdb_t {
	struct _LscCalibData_t    lscCalibData;
#ifdef SCENE_MODE_IMPLEMENT
	struct _LscTuningData_t
	lscTuningData[SCENE_PREFERENCE_TUNING_AAA_SETS_NUM];
#else
	struct _LscTuningData_t   lscTuningData;
#endif
};
struct _SensorTdb_t {
	struct _Version_t         version;
	struct _PrePipeTdb_t      prePipeTdb;
	struct _CorePipeCdb_t     corePipeCdb;
	struct _PostPipeTdb_t     postPipeTdb;
#ifndef CONFIG_DISABLE_TDB_NEW_ELEMENT
	struct _AeCdb_t           aeCdb;
	struct _PLineTable_t
		plineCdb[SCENE_PREFERENCE_TUNING_AAA_SETS_NUM];
	struct _AwbCdb_t          awbCdb;
	struct _AfCdb_t           afCdb;
	struct _IridixCdb_t       iridixCdb;
	struct _Lut3D_t           lut3D;
	struct _AutoContrast_t    autoContrast;
	struct _LscCdb_t          lscCdb;
	struct _MappingAaa_t	mappingAaa[SCENARIO_MODE_MAX];
#endif // CONFIG_DISABLE_TDB_NEW_ELEMENT
	struct _SceneModePipeDelta_t sceneModePipe;
	struct _PipeMappingDelta_t      pipeMappingDelta[SCENARIO_MODE_MAX];
};
#endif

struct _SensorTdbAlign_t {
	struct _SensorTdb_t tdb;
};

enum _M2M3ChColorComponent_t {
	M2M_3CH_COLOR_COMPONENT_RED   = 0,
	M2M_3CH_COLOR_COMPONENT_GREEN = 1,
	M2M_3CH_COLOR_COMPONENT_BLUE  = 2,
	M2M_3CH_COLOR_COMPONENT_MAX
};

struct _M2MAwbValue_t {
	unsigned short R;
	unsigned short GR;
	unsigned short GB;
	unsigned short B;
};

struct _M2MAwb_t {
	unsigned int sectionInfo;
	struct _M2MAwbValue_t goldenValueMCct;
	struct _M2MAwbValue_t goldenValueHCct;
	struct _M2MAwbValue_t goldenValueLCct;
	struct _M2MAwbValue_t unitValueMCct;
	struct _M2MAwbValue_t unitValueHCct;
	struct _M2MAwbValue_t unitValueLCct;
};

struct _M2MRadialShadingLUT_t {
	unsigned short uCoeff[130];
};

struct _M2MRadialShading_t {
	unsigned int sectionInfo;
	unsigned short radialNodeNum;
	unsigned short lutSize;
	unsigned short chromaStrength;
	unsigned short lumaStrength;
	unsigned short rawWidth;
	unsigned short rawHeight;
	unsigned short centerRx;
	unsigned short centerRy;
	unsigned short centerGx;
	unsigned short centerGy;
	unsigned short centerBx;
	unsigned short centerBy;
	unsigned short offCenterRx;
	unsigned short offCenterRy;
	unsigned short offCenterGx;
	unsigned short offCenterGy;
	unsigned short offCenterBx;
	unsigned short offCenterBy;
	unsigned short blR;
	unsigned short blGr;
	unsigned short blGb;
	unsigned short blB;
	struct _M2MRadialShadingLUT_t
		radialShadingLut[M2M_3CH_COLOR_COMPONENT_MAX];
};

struct _M2MMeshShadingLUT_t {
	unsigned char uCoeff[16 * 16];
};

struct _M2MMeshShading_t {
	unsigned int sectionInfo;
	unsigned int meshScale;
	unsigned short meshWidth;
	unsigned short meshHeight;
	unsigned short blR;
	unsigned short blGr;
	unsigned short blGb;
	unsigned short blB;
	struct _M2MMeshShadingLUT_t
		meshShadingLutMCct[M2M_3CH_COLOR_COMPONENT_MAX];
	struct _M2MMeshShadingLUT_t
		meshShadingLutHCct[M2M_3CH_COLOR_COMPONENT_MAX];
	struct _M2MMeshShadingLUT_t
		meshShadingLutLCct[M2M_3CH_COLOR_COMPONENT_MAX];
};

struct _M2MAF_t {
	unsigned int sectionInfo;
	unsigned short temperature;
	unsigned short direction;
	unsigned short infinityPos;
	unsigned short nearestPos;
	unsigned short middlePos;
};

struct _M2MTdb_t {
	struct _M2MAwb_t awb;
	struct _M2MRadialShading_t radialShading;
	struct _M2MMeshShading_t meshShading;
	struct _M2MAF_t af;
};
