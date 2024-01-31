/*
 * Copyright 2020-2021 Advanced Micro Devices, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 */
#ifndef __SMU11_DRIVER_IF_MERO_H__
#define __SMU11_DRIVER_IF_MERO_H__

// *** IMPORTANT ***
// SMU TEAM: Always increment the interface version if
// any structure is changed in this file
#define SMU13_DRIVER_IF_VERSION 3
#define FEATURE_DPM_MP0CLK_BIT  7

struct FloatInIntFormat_t {
	s32 value;
	u32 numFractionalBits;
};

enum DSPCLK_e {
	DSPCLK_DCFCLK = 0,
	DSPCLK_DISPCLK,
	DSPCLK_PIXCLK,
	DSPCLK_PHYCLK,
	DSPCLK_COUNT,
};

struct DisplayClockTable_t {
	u16 Freq; // in MHz
	u16 Vid;  // min voltage in SVI2 VID
};

struct WatermarkRowGeneric_t {
	u16 MinClock; // This is either DCFCLK or SOCCLK (in MHz)
	u16 MaxClock; // This is either DCFCLK or SOCCLK (in MHz)
	u16 MinMclk;
	u16 MaxMclk;
	u8  WmSetting;
	u8  WmType;  // Used for normal pstate change or memory retraining
	u8  Padding[2];
};

#define NUM_WM_RANGES 4
#define WM_PSTATE_CHG 0
#define WM_RETRAINING 1

enum WM_CLOCK_e {
	WM_SOCCLK = 0,
	WM_DCFCLK,
	WM_COUNT,
};

struct Watermarks_t {
	// Watermarks
	struct WatermarkRowGeneric_t WatermarkRow[WM_COUNT][NUM_WM_RANGES];
	u32 MmHubPadding[7]; // SMU internal use
};

enum CUSTOM_DPM_SETTING_e {
	CUSTOM_DPM_SETTING_GFXCLK,
	CUSTOM_DPM_SETTING_CCLK,
	CUSTOM_DPM_SETTING_FCLK_CCX,
	CUSTOM_DPM_SETTING_FCLK_GFX,
	CUSTOM_DPM_SETTING_FCLK_STALLS,
	CUSTOM_DPM_SETTING_LCLK,
	CUSTOM_DPM_SETTING_COUNT,
};

struct DpmActivityMonitorCoeffExt_t {
	u8 ActiveHystLimit;
	u8 IdleHystLimit;
	u8 FPS;
	u8 MinActiveFreqType;
	struct FloatInIntFormat_t MinActiveFreq;
	struct FloatInIntFormat_t PD_Data_limit;
	struct FloatInIntFormat_t PD_Data_time_constant;
	struct FloatInIntFormat_t PD_Data_error_coeff;
	struct FloatInIntFormat_t PD_Data_error_rate_coeff;
};

struct CustomDpmSettings_t {
	struct DpmActivityMonitorCoeffExt_t
	DpmActivityMonitorCoeff[CUSTOM_DPM_SETTING_COUNT];
};

struct DpmActivityMonitorCoeffInt_t {
	u8   Gfx_ActiveHystLimit;
	u8   Gfx_IdleHystLimit;
	u8   Gfx_FPS;
	u8   Gfx_MinActiveFreqType;
	u8   Gfx_BoosterFreqType;
	u8   Gfx_UseRlcBusy;
	u16  Gfx_MinActiveFreq;
	u16  Gfx_BoosterFreq;
	u16  Gfx_PD_Data_time_constant;
	u32  Gfx_PD_Data_limit_a;
	u32  Gfx_PD_Data_limit_b;
	u32  Gfx_PD_Data_limit_c;
	u32  Gfx_PD_Data_error_coeff;
	u32  Gfx_PD_Data_error_rate_coeff;
	u8   Soc_ActiveHystLimit;
	u8   Soc_IdleHystLimit;
	u8   Soc_FPS;
	u8   Soc_MinActiveFreqType;
	u8   Soc_BoosterFreqType;
	u8   Soc_UseRlcBusy;
	u16  Soc_MinActiveFreq;
	u16  Soc_BoosterFreq;
	u16  Soc_PD_Data_time_constant;
	u32  Soc_PD_Data_limit_a;
	u32  Soc_PD_Data_limit_b;
	u32  Soc_PD_Data_limit_c;
	u32  Soc_PD_Data_error_coeff;
	u32  Soc_PD_Data_error_rate_coeff;
	u8   Mem_ActiveHystLimit;
	u8   Mem_IdleHystLimit;
	u8   Mem_FPS;
	u8   Mem_MinActiveFreqType;
	u8   Mem_BoosterFreqType;
	u8   Mem_UseRlcBusy;
	u16  Mem_MinActiveFreq;
	u16  Mem_BoosterFreq;
	u16  Mem_PD_Data_time_constant;
	u32  Mem_PD_Data_limit_a;
	u32  Mem_PD_Data_limit_b;
	u32  Mem_PD_Data_limit_c;
	u32  Mem_PD_Data_error_coeff;
	u32  Mem_PD_Data_error_rate_coeff;
	u8   Fclk_ActiveHystLimit;
	u8   Fclk_IdleHystLimit;
	u8   Fclk_FPS;
	u8   Fclk_MinActiveFreqType;
	u8   Fclk_BoosterFreqType;
	u8   Fclk_UseRlcBusy;
	u16  Fclk_MinActiveFreq;
	u16  Fclk_BoosterFreq;
	u16  Fclk_PD_Data_time_constant;
	u32  Fclk_PD_Data_limit_a;
	u32  Fclk_PD_Data_limit_b;
	u32  Fclk_PD_Data_limit_c;
	u32  Fclk_PD_Data_error_coeff;
	u32  Fclk_PD_Data_error_rate_coeff;
};

#define NUM_DCFCLK_DPM_LEVELS 7
#define NUM_DISPCLK_DPM_LEVELS 7
#define NUM_DPPCLK_DPM_LEVELS 7
#define NUM_SOCCLK_DPM_LEVELS 7
#define NUM_ISPICLK_DPM_LEVELS 7
#define NUM_ISPXCLK_DPM_LEVELS 7
#define NUM_VCN_DPM_LEVELS 5
#define NUM_FCLK_DPM_LEVELS 4
#define NUM_SOC_VOLTAGE_LEVELS 8

struct df_pstate_t {
	u32 fclk;
	u32 memclk;
	u32 voltage;
};

struct vcn_clk_t {
	u32 vclk;
	u32 dclk;
};

//Freq in MHz
//Voltage in milli volts with 2 fractional bits

struct DpmClocks_t {
	u32 DcfClocks[NUM_DCFCLK_DPM_LEVELS];
	u32 DispClocks[NUM_DISPCLK_DPM_LEVELS];
	u32 DppClocks[NUM_DPPCLK_DPM_LEVELS];
	u32 SocClocks[NUM_SOCCLK_DPM_LEVELS];
	u32 IspiClocks[NUM_ISPICLK_DPM_LEVELS];
	u32 IspxClocks[NUM_ISPXCLK_DPM_LEVELS];
	struct vcn_clk_t VcnClocks[NUM_VCN_DPM_LEVELS];

	u32 SocVoltage[NUM_SOC_VOLTAGE_LEVELS];

	struct df_pstate_t DfPstateTable[NUM_FCLK_DPM_LEVELS];

	u32 MinGfxClk;
	u32 MaxGfxClk;

	u8 NumDfPstatesEnabled;
	u8 NumDcfclkLevelsEnabled;
	u8 NumDispClkLevelsEnabled;	// applies to both dispclk and dppclk
	u8 NumSocClkLevelsEnabled;

	u8 IspClkLevelsEnabled;		// applies to both ispiclk and ispxclk
	u8 VcnClkLevelsEnabled;		// applies to both vclk/dclk

	u8 spare[2];
};

// Throttler Status Bitmask
#define THROTTLER_STATUS_BIT_SPL 0
#define THROTTLER_STATUS_BIT_FPPT 1
#define THROTTLER_STATUS_BIT_SPPT 2
#define THROTTLER_STATUS_BIT_SPPT_APU 3
#define THROTTLER_STATUS_BIT_THM_CORE 4
#define THROTTLER_STATUS_BIT_THM_GFX 5
#define THROTTLER_STATUS_BIT_THM_SOC 6
#define THROTTLER_STATUS_BIT_TDC_VDD 7
#define THROTTLER_STATUS_BIT_TDC_SOC 8
#define THROTTLER_STATUS_BIT_TDC_GFX 9
#define THROTTLER_STATUS_BIT_TDC_CVIP 10

struct SmuMetrics_legacy {
	u16 GfxclkFrequency;      //[MHz]
	u16 SocclkFrequency;      //[MHz]
	u16 VclkFrequency;        //[MHz]
	u16 DclkFrequency;        //[MHz]
	u16 MemclkFrequency;      //[MHz]
	u16 spare;
	u16 GfxActivity; //[centi]
	u16 UvdActivity; //[centi]
	// [mV] indices: VDDCR_VDD, VDDCR_SOC, VDDCR_GFX
	u16 Voltage[3];
	// [mA] indices: VDDCR_VDD, VDDCR_SOC, VDDCR_GFX
	u16 Current[3];
	// [mW] indices: VDDCR_VDD, VDDCR_SOC, VDDCR_GFX
	u16 Power[3];
	u16 CurrentSocketPower; //[mW]
	//3rd party tools in Windows need this info in the case of APUs
	u16 CoreFrequency[8];   //[MHz]
	u16 CorePower[8];       //[mW]
	u16 CoreTemperature[8]; //[centi-Celsius]
	u16 L3Frequency[2];     //[MHz]
	u16 L3Temperature[2];   //[centi-Celsius]
	u16 GfxTemperature; //[centi-Celsius]
	u16 SocTemperature; //[centi-Celsius]
	u16 EdgeTemperature;
	u16 ThrottlerStatus;
};

struct SmuMetricsTable {
	uint16_t GfxclkFrequency;      //[MHz]
	uint16_t SocclkFrequency;      //[MHz]
	uint16_t VclkFrequency;        //[MHz]
	uint16_t DclkFrequency;        //[MHz]
	uint16_t MemclkFrequency;      //[MHz]
	uint16_t spare;

	uint16_t GfxActivity;          //[centi]
	uint16_t UvdActivity;          //[centi]
	uint16_t C0Residency[4];       //percentage

	// [mV] indices: VDDCR_VDD, VDDCR_SOC, VDDCR_GFX
	uint16_t Voltage[3];
	// [mA] indices: VDDCR_VDD, VDDCR_SOC, VDDCR_GFX
	uint16_t Current[3];
	// [mW] indices: VDDCR_VDD, VDDCR_SOC, VDDCR_GFX
	uint16_t Power[3];
	uint16_t CurrentSocketPower;   //[mW]

	//3rd party tools in Windows need info in the case of APUs
	uint16_t CoreFrequency[4];     //[MHz]
	uint16_t CorePower[4];         //[mW]
	uint16_t CoreTemperature[4];   //[centi-Celsius]
	uint16_t L3Frequency[1];       //[MHz]
	uint16_t L3Temperature[1];     //[centi-Celsius]

	uint16_t GfxTemperature;       //[centi-Celsius]
	uint16_t SocTemperature;       //[centi-Celsius]
	uint16_t EdgeTemperature;
	uint16_t ThrottlerStatus;
};

struct SmuMetrics {
	struct SmuMetricsTable Current;
	struct SmuMetricsTable Average;
	// uint32_t AccCnt;
	uint32_t SampleStartTime;
	uint32_t SampleStopTime;
};

// Workload bits
#define WORKLOAD_PPLIB_FULL_SCREEN_3D_BIT 0
#define WORKLOAD_PPLIB_VIDEO_BIT 2
#define WORKLOAD_PPLIB_VR_BIT 3
#define WORKLOAD_PPLIB_COMPUTE_BIT 4
#define WORKLOAD_PPLIB_CUSTOM_BIT 5
#define WORKLOAD_PPLIB_COUNT 6

#define TABLE_BIOS_IF            0 // Called by BIOS
#define TABLE_WATERMARKS         1 // Called by DAL through VBIOS
#define TABLE_CUSTOM_DPM         2 // Called by Driver
#define TABLE_SPARE1             3
#define TABLE_DPMCLOCKS          4 // Called by Driver
#define TABLE_SPARE2             5 // Called by Tools
#define TABLE_MODERN_STDBY       6 // Called by Tools for Modern Standby Log
#define TABLE_SMU_METRICS        7 // Called by Driver
#define TABLE_COUNT              8

//ISP tile definitions
enum TILE_NUM_e {
	TILE_ISPX = 0, // ISPX
	TILE_ISPM,     // ISPM
	TILE_ISPC,  // ISPCORE
	TILE_ISPPRE,   // ISPPRE
	TILE_ISPPOST,  // ISPPOST
	TILE_MAX
};

// Tile Selection (Based on arguments)
#define TILE_SEL_ISPX       (1 << (TILE_ISPX))
#define TILE_SEL_ISPM       (1 << (TILE_ISPM))
#define TILE_SEL_ISPC       (1 << (TILE_ISPC))
#define TILE_SEL_ISPPRE     (1 << (TILE_ISPPRE))
#define TILE_SEL_ISPPOST    (1 << (TILE_ISPPOST))

// Mask for ISP tiles in PGFSM PWR Status Registers
//Bit[1:0] maps to ISPX, (ISPX)
//Bit[3:2] maps to ISPM, (ISPM)
//Bit[5:4] maps to ISPCORE, (ISPCORE)
//Bit[7:6] maps to ISPPRE, (ISPPRE)
//Bit[9:8] maps to POST, (ISPPOST
#define TILE_ISPX_MASK      ((1 << 0) | (1 << 1))
#define TILE_ISPM_MASK      ((1 << 2) | (1 << 3))
#define TILE_ISPC_MASK      ((1 << 4) | (1 << 5))
#define TILE_ISPPRE_MASK    ((1 << 6) | (1 << 7))
#define TILE_ISPPOST_MASK   ((1 << 8) | (1 << 9))

#endif
