/*
 * Copyright (C) 2019-2021 Advanced Micro Devices, Inc. All rights reserved.
 */


#pragma once

#include "os_base_type.h"

//Below size must be 4byte aligned
#ifdef __cplusplus
extern "C"
{
#endif
#define PREP_AE_METER_MAX_SIZE              (128 * 128 * 4)

//128x128 (zones) x 4 (1 register for each zone, 4 bytes), zone number up to
//128x128 and down to 12x9, FW set to 128x128 in AeMgr



#define PREP_AE_HIST_LONG_STATS_SIZE        (1024 * 2 * 3)

//1024 (hist bin number) x 2 (2 bytes for each bin, 1 register for 2 bins) x 3
//(R&G&B channel), IR will reuse the long G channel



#define PREP_AE_HIST_SHORT_STATS_SIZE       (1024 * 2 * 3)

//1024 (hist bin number) x 2 (2 bytes for each bin, 1 register for 2 bins) x 3
//(R&G&B channel)



#define PREP_AWB_METER_MAX_SIZE             (32 * 32 * 2 * 3)

//32x32 (zones) x 2 (2 bytes for each zone, 1 register for 2 zones) x 3
//(R&G&B channel), zone number is fixed 32x32



#define COMMON_IRIDIX_HIST_STATS_SIZE       (1024 * 4)

//1024 (hist bin number) x 4 (1 register for each bin, 4 bytes)



#define COMMON_AEXP_HIST_STATS_SIZE         (1024 * 4)

//1024 (hist bin number) x 4 (1 register for each bin, 4 bytes)



#define PING_PONG_AF_STATS_SIZE             (33 * 33 * 8)

//33x33 (zones) x 2 (2 registers for each zone) x 4 (4 bytes), zone number up
//to 33x33 and configurable, Ryan advices to set 33x33 and FW set to 33x33
//by hard code



#define PING_PONG_AWB_STATS_SIZE            (33 * 33 * 8)

#define PING_PONG_LUMVAR_STATS_SIZE		    (512 * 4)
/* 32x16 (zones) x  4 (4 bytes), zone number fixed to 32 * 16;
 * Each location contains 10-bit (LSBs) of mean information and 12 bit (MSBs)
 * of the variance of the luminance mean information for each zone
 * Minimum frame resolution required for this stat module to give usable result
 * is 512x256.
 */

#define PING_PONG_PD_TYPE_1_STATS_SIZE (2048)
//Type1 PD data size

//The total size is 109584 bytes

struct _AaaStatsData_t {

	//PRE AE block metering

	unsigned int aeMetering
		[PREP_AE_METER_MAX_SIZE / 4] IDMA_COPY_ADDR_ALIGN;

	//PRE AE long histogram

	unsigned int aeLongHist
		[PREP_AE_HIST_LONG_STATS_SIZE / 4] IDMA_COPY_ADDR_ALIGN;

	//PRE AE short histogram

	unsigned int aeShortHist
		[PREP_AE_HIST_SHORT_STATS_SIZE / 4] IDMA_COPY_ADDR_ALIGN;

	//PRE AWB metering

	unsigned int preAwbMetering
		[PREP_AWB_METER_MAX_SIZE / 4] IDMA_COPY_ADDR_ALIGN;

	//3rd party AF metering
	unsigned int afMetering
		[PING_PONG_AF_STATS_SIZE / 4] IDMA_COPY_ADDR_ALIGN;

	//3rd party AWB metering
	unsigned int awbMetering
		[PING_PONG_AWB_STATS_SIZE / 4] IDMA_COPY_ADDR_ALIGN;

	//3rd party IRIDIX histogram

	unsigned int iridixHist
		[COMMON_IRIDIX_HIST_STATS_SIZE / 4] IDMA_COPY_ADDR_ALIGN;

	//3rd party AE HIST

	unsigned int aeHist
		[COMMON_AEXP_HIST_STATS_SIZE / 4] IDMA_COPY_ADDR_ALIGN;

	unsigned int  lumvarMetering
		[PING_PONG_LUMVAR_STATS_SIZE / 4] IDMA_COPY_ADDR_ALIGN;

	unsigned int pdType1Metering
		[PING_PONG_PD_TYPE_1_STATS_SIZE / 4] IDMA_COPY_ADDR_ALIGN;
};

#ifdef __cplusplus
}
#endif

