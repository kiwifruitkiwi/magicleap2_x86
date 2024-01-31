/*
 * Copyright (C) 2020 Advanced Micro Devices, Inc. All rights reserved.
 */

#pragma once
#include "os_base_type.h"

#define LSC_ALGO_TUNING_VER 3001201013U
#define LSC_ALGO_CALIB_VER 3001201013U

#define LSC_ALGO_RADIAL_LUT_SIZE 129
#define LSC_ALGO_MESH_MAX_SIZE 16
#define LSC_MESH_BLEND_THR_NUM 4
#define LSC_MESH_STRENGTH_NUM 8
#define LSC_ALGO_PRELSC_SECTOR_NUM 8
#define LSC_ALGO_PRELSC_TBL_NUM 17
#define LSC_ALGO_PRELSC_TBL_MAX_SIZE \
	(LSC_ALGO_PRELSC_TBL_NUM * LSC_ALGO_PRELSC_TBL_NUM)

//ALSO ALGO
#define AL_STAT_H_NUM 16
#define AL_STAT_W_NUM 16
#define AL_COMPONENT_NUM 8
#define AL_COLOR_RTO_NUM 2
#define AL_TBLPROC_POLYFIT_DEG 3
#define AL_TBLPROC_WIN_SZ 15

#ifdef __cplusplus
extern "C" {
#endif

enum _LSC_ALGO_CH_ {
	LSC_ALGO_CH_R = 0,
	LSC_ALGO_CH_G = 1,
	LSC_ALGO_CH_B = 2,
	LSC_ALGO_CH_MAX
};

struct _LscMeshStrength_t {
	unsigned short iso;
	unsigned short ratio;
};

struct _LscAlgoRadialLut_t {
	unsigned short Coeff[LSC_ALGO_RADIAL_LUT_SIZE];
};

struct _LscAlgoRadial_t {
	unsigned short radialNodeNum;
	unsigned short lutTblSize;
	unsigned short chromaStrength;
	unsigned short lumaStrength;
	unsigned short caliRawW;
	unsigned short caliRawH;
	unsigned short centerX[LSC_ALGO_CH_MAX];
	unsigned short centerY[LSC_ALGO_CH_MAX];
	unsigned short offCenterX[LSC_ALGO_CH_MAX];
	unsigned short offCenterY[LSC_ALGO_CH_MAX];
	unsigned short o_bR;
	unsigned short obGr;
	unsigned short obGb;
	unsigned short obB;
	struct _LscAlgoRadialLut_t radialLut[LSC_ALGO_CH_MAX];
};

struct _LscAlgoMeshLut_t {
	unsigned char Coeff[LSC_ALGO_MESH_MAX_SIZE * LSC_ALGO_MESH_MAX_SIZE];
};

struct _LscAlgoMesh_t {
	unsigned int meshAlphaMode;
	unsigned int meshScale;
	unsigned short meshWidth;
	unsigned short meshHeight;
	unsigned short o_bR;
	unsigned short obGr;
	unsigned short obGb;
	unsigned short obB;
	struct _LscAlgoMeshLut_t LutMCct[LSC_ALGO_CH_MAX];
	struct _LscAlgoMeshLut_t LutHCct[LSC_ALGO_CH_MAX];
	struct _LscAlgoMeshLut_t LutLCct[LSC_ALGO_CH_MAX];
};

struct _LscAlgoPreLscOut_t {
	unsigned short sectSizeX[LSC_ALGO_PRELSC_SECTOR_NUM];
	unsigned short sectSizeY[LSC_ALGO_PRELSC_SECTOR_NUM];
	unsigned short gradX[LSC_ALGO_PRELSC_SECTOR_NUM];
	unsigned short gradY[LSC_ALGO_PRELSC_SECTOR_NUM];
	unsigned short dataR[LSC_ALGO_PRELSC_TBL_MAX_SIZE];
	unsigned short dataGR[LSC_ALGO_PRELSC_TBL_MAX_SIZE];
	unsigned short dataGB[LSC_ALGO_PRELSC_TBL_MAX_SIZE];
	unsigned short dataB[LSC_ALGO_PRELSC_TBL_MAX_SIZE];
};

struct _LscTuningData_t {
	unsigned int version;
	unsigned char  default_cct_idx; // 0/1/2: high/mid/low
	unsigned int meshBlendThr[LSC_MESH_BLEND_THR_NUM];
	struct _LscMeshStrength_t meshStrength[LSC_MESH_STRENGTH_NUM];
	unsigned short meshDefStrength;
	unsigned short meshStrengthSmoothRto;
	unsigned int gradientThr;
	unsigned int colorHighThr;
	unsigned int colorLowThr;
	unsigned int confHighThr;
	unsigned int confLowThr;
	unsigned int lamdaTh1;
	unsigned int lamdaTh2;
	unsigned char  speed;
};

struct _LscCalibData_t {
	unsigned int version;
	unsigned int shdCom[AL_COMPONENT_NUM][AL_STAT_H_NUM * AL_STAT_W_NUM];
	struct _LscAlgoRadial_t radial;
	struct _LscAlgoMesh_t   mesh;
	struct _LscAlgoPreLscOut_t preLsc;
	float coefMatrix[AL_TBLPROC_POLYFIT_DEG + 1][AL_TBLPROC_WIN_SZ];
	unsigned int mean[AL_COLOR_RTO_NUM][AL_COMPONENT_NUM];
	unsigned int std[AL_COLOR_RTO_NUM][AL_COMPONENT_NUM];
	unsigned int coefUb[AL_COLOR_RTO_NUM][AL_COMPONENT_NUM];
	unsigned int coefLb[AL_COLOR_RTO_NUM][AL_COMPONENT_NUM];
};

const struct _LscCalibData_t *LscAlgoGetDefaultCalibData(void);
const struct _LscTuningData_t *LscAlgoGetDefaultTuningData(void);

#ifdef __cplusplus
}
#endif
