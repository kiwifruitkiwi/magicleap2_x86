/*
 * Copyright (C) 2019-2020 Advanced Micro Devices, Inc. All rights reserved.
 */

#pragma once

// need refine later
#define TDB_CONFIG_PRE_LSC_MAX_ILLU_PROFILES           (7)
#define TDB_CONFIG_PRE_LSC_MAX_ILLU_PROFILES_FOR_IR    (3)

//These macros define the maximum numbers for storing
//a kind of tuning data. They can be reconfigured if the
//number is not enough to get a good image quality after
//getting consensus between firmawre/tuning/driver team.
#ifdef ENABLE_ISP2P1_NEW_TUNING_DATA
#define TDB_CONFIG_AWB_MAX_ILLU_PROFILES           (12)
#else
#define TDB_CONFIG_AWB_MAX_ILLU_PROFILES           (7)
#endif
#define TDB_CONFIG_AWB_MAX_CURVE_POINTS            (32)
#define TDB_CONFIG_AWB_MAX_FADE2_POINTS            (6)
#define TDB_CONFIG_AWB_MAX_SATS_FOR_SENSOR_GAIN    (4)
#define TDB_CONFIG_AWB_MAX_SATS_FOR_CC             (2)
#ifndef ENABLE_ISP2P1_NEW_TUNING_DATA
#define TDB_CONFIG_AWB_MAX_VIGS                    (2)
#endif

//Two tuning data profiles for IR stream. One is on and
//the other is off. AWB is disabled for IR stream and
//there is only LSC.
#ifdef ENABLE_ISP2P1_NEW_TUNING_DATA
#define TDB_CONFIG_LSC_MAX_ILLU_PROFILES_FOR_IR    (3)
#else
#define TDB_CONFIG_LSC_MAX_ILLU_PROFILES_FOR_IR    (2)
#endif

//These macros define the size of a kind array. They
//are mapped to hardware and only should be changed when
//the hardware changed.
#define TDB_DEGAMMA_CURVE_SIZE                     (17)
#define TDB_GAMMA_CURVE_SIZE                       (17)
#ifdef ENABLE_ISP2P1_NEW_TUNING_DATA
#define TDB_GAMMA_CURVE_MAX_NUM                    (8)
#endif
#define TDB_WDR_CURVE_SIZE                         (33)
#define TDB_ISP_FILTER_DENOISE_SIZE                (7)
#define TDB_ISP_FILTER_SHARP_SIZE                  (5)
#define TDB_SNR_SIGMA_COEFFS_SIZE                  (17)
#define TDB_SNR_CALC_COEFFS_SIZE                   (17)
#define TDB_TNR_ALPHA_CONST_SIZE                   (3)

#define TDB_DITHERING_Y_TABLE_SIZE                 (32)
#define TDB_DITHERING_C_TABLE_SIZE                 (32)

