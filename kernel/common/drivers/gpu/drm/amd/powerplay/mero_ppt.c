/*
 * Copyright (C) 2020-2022 Advanced Micro Devices, Inc. All rights reserved.
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

#include "pp_debug.h"
#include <linux/firmware.h>
#include "amdgpu.h"
#include "amdgpu_smu.h"
#include "smu_internal.h"
#include "atomfirmware.h"
#include "amdgpu_atomfirmware.h"
#include "smu_v11_0.h"
#include "smu11_driver_if_mero.h"
#include "soc15_common.h"
#include "atom.h"
#include "mero_ppt.h"
#include "smu_v11_5_ppsmc.h"
#include "smu_v11_5_pmfw.h"
#include "asic_reg/mp/mp_11_0_sh_mask.h"
#include <linux/smu_protocol.h>
#include "asic_reg/gc/gc_10_3_0_offset.h"
#include "asic_reg/gc/gc_10_3_0_sh_mask.h"

#define FEATURE_MASK(feature) (1ULL << (feature))

#define SMC_DPM_FEATURE ( \
	FEATURE_MASK(FEATURE_CCLK_DPM_BIT) | \
	FEATURE_MASK(FEATURE_VCN_DPM_BIT)	 | \
	FEATURE_MASK(FEATURE_FCLK_DPM_BIT)	 | \
	FEATURE_MASK(FEATURE_SOCCLK_DPM_BIT)	 | \
	FEATURE_MASK(FEATURE_MP0CLK_DPM_BIT)	 | \
	FEATURE_MASK(FEATURE_LCLK_DPM_BIT)	 | \
	FEATURE_MASK(FEATURE_SHUBCLK_DPM_BIT)	 | \
	FEATURE_MASK(FEATURE_DCFCLK_DPM_BIT) | \
	FEATURE_MASK(FEATURE_GFX_DPM_BIT))

#define MSG_MAP(msg, index) \
	[SMU_MSG_##msg] = {1, (index)}

static struct smu_13_0_cmn2aisc_mapping
mero_message_map[SMU_MSG_MAX_COUNT] = {
	MSG_MAP(TestMessage,               PPSMC_MSG_TestMessage),
	MSG_MAP(GetSmuVersion,             PPSMC_MSG_GetSmuVersion),
	MSG_MAP(GetDriverIfVersion,        PPSMC_MSG_GetDriverIfVersion),
	MSG_MAP(DisableGfxOff,             PPSMC_MSG_DisableGfxOff),
	MSG_MAP(EnableGfxOff,              PPSMC_MSG_EnableGfxOff),
	MSG_MAP(PowerDownIspByTile,        PPSMC_MSG_PowerDownIspByTile),
	MSG_MAP(PowerUpIspByTile,          PPSMC_MSG_PowerUpIspByTile),
	MSG_MAP(PowerDownVcn,              PPSMC_MSG_PowerDownVcn),
	MSG_MAP(PowerUpVcn,                PPSMC_MSG_PowerUpVcn),
	MSG_MAP(RlcPowerNotify,            PPSMC_MSG_RlcPowerNotify),
	MSG_MAP(SetHardMinVcn,             PPSMC_MSG_SetHardMinVcn),
	MSG_MAP(SetSoftMinGfxclk,          PPSMC_MSG_SetSoftMinGfxclk),
	MSG_MAP(ActiveProcessNotify,       PPSMC_MSG_ActiveProcessNotify),
	MSG_MAP(SetHardMinIspiclkByFreq,   PPSMC_MSG_SetHardMinIspiclkByFreq),
	MSG_MAP(SetHardMinIspxclkByFreq,   PPSMC_MSG_SetHardMinIspxclkByFreq),
	MSG_MAP(SetDriverDramAddrHigh,     PPSMC_MSG_SetDriverDramAddrHigh),
	MSG_MAP(SetDriverDramAddrLow,      PPSMC_MSG_SetDriverDramAddrLow),
	MSG_MAP(TransferTableSmu2Dram,     PPSMC_MSG_TransferTableSmu2Dram),
	MSG_MAP(TransferTableDram2Smu,     PPSMC_MSG_TransferTableDram2Smu),
	MSG_MAP(GfxDeviceDriverReset,      PPSMC_MSG_GfxDeviceDriverReset),
	MSG_MAP(GetEnabledSmuFeatures,     PPSMC_MSG_GetEnabledSmuFeatures),
	MSG_MAP(Spare1,                    PPSMC_MSG_spare1),
	MSG_MAP(SetHardMinSocclkByFreq,    PPSMC_MSG_SetHardMinSocclkByFreq),
	MSG_MAP(SetSoftMinFclk,            PPSMC_MSG_SetSoftMinFclk),
	MSG_MAP(SetSoftMinVcn,             PPSMC_MSG_SetSoftMinVcn),
	MSG_MAP(EnablePostCode,            PPSMC_MSG_EnablePostCode),
	MSG_MAP(GetGfxclkFrequency,        PPSMC_MSG_GetGfxclkFrequency),
	MSG_MAP(GetFclkFrequency,          PPSMC_MSG_GetFclkFrequency),
	MSG_MAP(AllowGfxOff,               PPSMC_MSG_AllowGfxOff),
	MSG_MAP(DisallowGfxOff,            PPSMC_MSG_DisallowGfxOff),
	MSG_MAP(SetSoftMaxGfxClk,          PPSMC_MSG_SetSoftMaxGfxClk),
	MSG_MAP(SetHardMinGfxClk,          PPSMC_MSG_SetHardMinGfxClk),
	MSG_MAP(SetSoftMaxSocclkByFreq,    PPSMC_MSG_SetSoftMaxSocclkByFreq),
	MSG_MAP(SetSoftMaxFclkByFreq,      PPSMC_MSG_SetSoftMaxFclkByFreq),
	MSG_MAP(SetSoftMaxVcn,             PPSMC_MSG_SetSoftMaxVcn),
	MSG_MAP(Spare2,                    PPSMC_MSG_spare2),
	MSG_MAP(SetPowerLimitPercentage,   PPSMC_MSG_SetPowerLimitPercentage),
	MSG_MAP(PowerDownJpeg,             PPSMC_MSG_PowerDownJpeg),
	MSG_MAP(PowerUpJpeg,               PPSMC_MSG_PowerUpJpeg),
	MSG_MAP(SetHardMinFclkByFreq,      PPSMC_MSG_SetHardMinFclkByFreq),
	MSG_MAP(SetSoftMinSocclkByFreq,    PPSMC_MSG_SetSoftMinSocclkByFreq),
	MSG_MAP(PowerUpCvip,               PPSMC_MSG_PowerUpCvip),
	MSG_MAP(PowerDownCvip,             PPSMC_MSG_PowerDownCvip),
	MSG_MAP(GetPptLimit,               PPSMC_MSG_GetPptLimit),
	MSG_MAP(GetThermalLimit,           PPSMC_MSG_GetThermalLimit),
	MSG_MAP(GetCurrentTemperature,     PPSMC_MSG_GetCurrentTemperature),
	MSG_MAP(GetCurrentPower,           PPSMC_MSG_GetCurrentPower),
	MSG_MAP(GetCurrentVoltage,         PPSMC_MSG_GetCurrentVoltage),
	MSG_MAP(GetCurrentCurrent,         PPSMC_MSG_GetCurrentCurrent),
	MSG_MAP(GetAverageCpuActivity,     PPSMC_MSG_GetAverageCpuActivity),
	MSG_MAP(GetAverageGfxActivity,     PPSMC_MSG_GetAverageGfxActivity),
	MSG_MAP(GetAveragePower,           PPSMC_MSG_GetAveragePower),
	MSG_MAP(GetAverageTemperature,     PPSMC_MSG_GetAverageTemperature),
	MSG_MAP(SetAveragePowerTimeConstant,
		PPSMC_MSG_SetAveragePowerTimeConstant),
	MSG_MAP(SetAverageActivityTimeConstant,
		PPSMC_MSG_SetAverageActivityTimeConstant),
	MSG_MAP(SetAverageTemperatrureTimeConstant,
		PPSMC_MSG_SetAverageTemperatureTimeConstant),
	MSG_MAP(SetMitigationEndHysteresis,
		PPSMC_MSG_SetMitigationEndHysteresis),
	MSG_MAP(GetCurrentFreq,            PPSMC_MSG_GetCurrentFreq),
	MSG_MAP(SetReducedPptLimit,        PPSMC_MSG_SetReducedPptLimit),
	MSG_MAP(SetReducedThermalLimit,    PPSMC_MSG_SetReducedThermalLimit),
	MSG_MAP(DramLogSetDramAddrLow,     PPSMC_MSG_DramLogSetDramAddrLow),
	MSG_MAP(StartDramLogging,          PPSMC_MSG_StartDramLogging),
	MSG_MAP(StopDramLogging,           PPSMC_MSG_StopDramLogging),
	MSG_MAP(SetSoftMinCclk,            PPSMC_MSG_SetSoftMinCclk),
	MSG_MAP(SetSoftMaxCclk,            PPSMC_MSG_SetSoftMaxCclk),
	MSG_MAP(SetDfPstateActiveLevel,    PPSMC_MSG_SetDfPstateActiveLevel),
	MSG_MAP(SetDfPstateSoftMinLevel,   PPSMC_MSG_SetDfPstateSoftMinLevel),
	MSG_MAP(SetCclkPolicy,             PPSMC_MSG_SetCclkPolicy),
	MSG_MAP(DramLogSetDramAddrHigh,    PPSMC_MSG_DramLogSetDramAddrHigh),
	MSG_MAP(DramLogSetDramSize,        PPSMC_MSG_DramLogSetDramBufferSize),
	MSG_MAP(RequestActiveWgp,          PPSMC_MSG_RequestActiveWgp),
	MSG_MAP(QueryActiveWgp,            PPSMC_MSG_QueryActiveWgp),
	MSG_MAP(SetFastPPTLimit,           PPSMC_MSG_SetFastPPTLimit),
	MSG_MAP(SetSlowPPTLimit,           PPSMC_MSG_SetSlowPPTLimit),
	MSG_MAP(GetFastPPTLimit,           PPSMC_MSG_GetFastPPTLimit),
	MSG_MAP(GetSlowPPTLimit,           PPSMC_MSG_GetSlowPPTLimit),
	MSG_MAP(ReadSvi3Register,          PPSMC_MSG_ReadSvi3Register),
	MSG_MAP(WriteSvi3Register,         PPSMC_MSG_WriteSvi3Register),
	MSG_MAP(DriverIHAlive,             PPSMC_MSG_DriverIHAlive),
	MSG_MAP(SetOverrideActivityMonitor,
		PPSMC_MSG_SetOverrideActivityMonitor),
	MSG_MAP(SetFclkDpmConstants,       PPSMC_MSG_SetFclkDpmConstants),
	MSG_MAP(GetFclkDpmConstants,       PPSMC_MSG_GetFclkDpmConstants),
	MSG_MAP(CalculateLclkBusyfromIohcOnly,
		PPSMC_MSG_CalculateLclkBusyfromIohcOnly),
	MSG_MAP(UpdateLclkDpmConstants,    PPSMC_MSG_UpdateLclkDpmConstants),
};

static struct smu_13_0_cmn2aisc_mapping
mero_feature_mask_map[SMU_FEATURE_COUNT] = {
	FEA_MAP(PPT),
	FEA_MAP(TDC),
	FEA_MAP(THERMAL),
	FEA_MAP(DS_GFXCLK),
	FEA_MAP(DS_SOCCLK),
	FEA_MAP(DS_LCLK),
	FEA_MAP(DS_FCLK),
	FEA_MAP(DS_MP1CLK),
	FEA_MAP(DS_MP0CLK),
	FEA_MAP(ATHUB_PG),
	FEA_MAP(CCLK_DPM),
	FEA_MAP(FAN_CONTROLLER),
	FEA_MAP(ULV),
	FEA_MAP(VCN_DPM),
	FEA_MAP(LCLK_DPM),
	FEA_MAP(SHUBCLK_DPM),
	FEA_MAP(DCFCLK_DPM),
	FEA_MAP(DS_DCFCLK),
	FEA_MAP(S0I2),
	FEA_MAP(SMU_LOW_POWER),
	FEA_MAP(GFX_DEM),
	FEA_MAP(PSI),
	FEA_MAP(PROCHOT),
	FEA_MAP(CPUOFF),
	FEA_MAP(STAPM),
	FEA_MAP(S0I3),
	FEA_MAP(DF_CSTATES),
	FEA_MAP(PERF_LIMIT),
	FEA_MAP(CORE_DLDO),
	FEA_MAP(RSMU_LOW_POWER),
	FEA_MAP(SMN_LOW_POWER),
	FEA_MAP(THM_LOW_POWER),
	FEA_MAP(SMUIO_LOW_POWER),
	FEA_MAP(MP1_LOW_POWER),
	FEA_MAP(DS_VCN),
	FEA_MAP(CPPC),
	FEA_MAP(OS_CSTATES),
	FEA_MAP(ISP_DPM),
	FEA_MAP(A55_DPM),
	FEA_MAP(CVIP_DSP_DPM),
	FEA_MAP(MSMU_LOW_POWER),
	FEA_MAP(SOC_VOLTAGE_MON),
	FEA_MAP(MSMU_LOW_POWER),
	FEA_MAP(ATHUB_PG),
	FEA_MAP(CC6),
};

static struct smu_13_0_cmn2aisc_mapping
mero_table_map[SMU_TABLE_COUNT] = {
	TAB_MAP(WATERMARKS),
	TAB_MAP(SMU_METRICS),
	TAB_MAP(CUSTOM_DPM),
	TAB_MAP(DPMCLOCKS),
};

static struct smu_13_0_cmn2aisc_mapping
mero_workload_map[PP_SMC_POWER_PROFILE_COUNT] = {
	WORKLOAD_MAP(PP_SMC_POWER_PROFILE_BOOTUP_DEFAULT,
		     WORKLOAD_PPLIB_VIDEO_BIT),
	WORKLOAD_MAP(PP_SMC_POWER_PROFILE_FULLSCREEN3D,
		     WORKLOAD_PPLIB_FULL_SCREEN_3D_BIT),
	WORKLOAD_MAP(PP_SMC_POWER_PROFILE_VIDEO,
		     WORKLOAD_PPLIB_VIDEO_BIT),
	WORKLOAD_MAP(PP_SMC_POWER_PROFILE_VR,
		     WORKLOAD_PPLIB_VR_BIT),
	WORKLOAD_MAP(PP_SMC_POWER_PROFILE_COMPUTE,
		     WORKLOAD_PPLIB_COMPUTE_BIT),
	WORKLOAD_MAP(PP_SMC_POWER_PROFILE_CUSTOM,
		     WORKLOAD_PPLIB_CUSTOM_BIT),
};

enum CLOCK_IDs_e {
	CLOCK_SMNCLK = 0,
	CLOCK_SOCCLK,
	CLOCK_MP0CLK,
	CLOCK_MP1CLK,
	CLOCK_MP2CLK,
	CLOCK_VCLK,
	CLOCK_LCLK,
	CLOCK_DCLK,
	CLOCK_ACLK,
	CLOCK_ISPICLK,
	CLOCK_ISPXCLK,
	CLOCK_SHUBCLK,
	CLOCK_DISPCLK,
	CLOCK_DPPCLK,
	CLOCK_DPREFCLK,
	CLOCK_DCFCLK,
	CLOCK_FCLK,
	CLOCK_UMCCLK,
	CLOCK_GFXCLK,
	CLOCK_COUNT,
};

static struct smu_11_0_cmn2aisc_mapping mero_clk_map[SMU_CLK_COUNT] = {
	CLK_MAP(GFXCLK, CLOCK_GFXCLK),
	CLK_MAP(DCLK,	CLOCK_DCLK),
	CLK_MAP(SOCCLK, CLOCK_SOCCLK),
	CLK_MAP(VCLK, CLOCK_VCLK),
	CLK_MAP(MCLK, CLOCK_UMCCLK),
};

static int mero_get_smu_feature_index(struct smu_context *smc, u32 index)
{
	struct smu_13_0_cmn2aisc_mapping mapping;

	if (index >= SMU_FEATURE_COUNT)
		return -EINVAL;

	if (index == SMU_FEATURE_DPM_GFXCLK_BIT)
		return FEATURE_GFX_DPM_BIT;

	if (index == SMU_FEATURE_DPM_SOCCLK_BIT)
		return FEATURE_SOCCLK_DPM_BIT;

	if (index == SMU_FEATURE_DPM_MP0CLK_BIT)
		return FEATURE_MP0CLK_DPM_BIT;

	if (index == SMU_FEATURE_DPM_FCLK_BIT
			|| index == SMU_FEATURE_DPM_UCLK_BIT)
		return FEATURE_FCLK_DPM_BIT;

	mapping = mero_feature_mask_map[index];
	if (!(mapping.valid_mapping))
		return -EINVAL;

	return mapping.map_to;
}

static int mero_get_smu_table_index(struct smu_context *smc, u32 index)
{
	struct smu_13_0_cmn2aisc_mapping mapping;

	if (index >= SMU_TABLE_COUNT)
		return -EINVAL;

	mapping = mero_table_map[index];
	if (!(mapping.valid_mapping))
		return -EINVAL;

	return mapping.map_to;
}

static int mero_get_workload_type(struct smu_context *smu,
				     enum PP_SMC_POWER_PROFILE profile)
{
	struct smu_13_0_cmn2aisc_mapping mapping;

	if (profile > PP_SMC_POWER_PROFILE_CUSTOM)
		return -EINVAL;

	mapping = mero_workload_map[profile];
	if (!(mapping.valid_mapping))
		return -EINVAL;

	return mapping.map_to;
}

static int mero_tables_init(struct smu_context *smu,
			       struct smu_table *tables)
{
	struct smu_table_context *smu_table = &smu->smu_table;
	struct amdgpu_device *adev = smu->adev;
	uint32_t if_version;
	uint32_t ret = 0;

	ret = smu_get_smc_version(smu, &if_version, NULL);
	if (ret) {
		dev_err(adev->dev, "Failed to get smu if version!\n");
		return ret;
	}

	SMU_TABLE_INIT(tables, SMU_TABLE_WATERMARKS,
		       sizeof(struct Watermarks_t),
		       PAGE_SIZE, AMDGPU_GEM_DOMAIN_VRAM);
	SMU_TABLE_INIT(tables, SMU_TABLE_DPMCLOCKS,
		       sizeof(struct DpmClocks_t),
		       PAGE_SIZE, AMDGPU_GEM_DOMAIN_VRAM);
	SMU_TABLE_INIT(tables, SMU_TABLE_PMSTATUSLOG, SMU11_TOOL_SIZE,
		       PAGE_SIZE, AMDGPU_GEM_DOMAIN_VRAM);
	SMU_TABLE_INIT(tables, SMU_TABLE_ACTIVITY_MONITOR_COEFF,
		       sizeof(struct DpmActivityMonitorCoeffInt_t),
		       PAGE_SIZE, AMDGPU_GEM_DOMAIN_VRAM);

	if (if_version < 0x3) {
		SMU_TABLE_INIT(tables, SMU_TABLE_SMU_METRICS,
				sizeof(struct SmuMetrics_legacy),
				PAGE_SIZE, AMDGPU_GEM_DOMAIN_VRAM);
		smu_table->metrics_table = kzalloc(
				sizeof(struct SmuMetrics_legacy),
				GFP_KERNEL);
	} else {
		SMU_TABLE_INIT(tables, SMU_TABLE_SMU_METRICS,
				sizeof(struct SmuMetrics),
				PAGE_SIZE, AMDGPU_GEM_DOMAIN_VRAM);
		smu_table->metrics_table = kzalloc(sizeof(struct SmuMetrics),
				GFP_KERNEL);
	}

	if (!smu_table->metrics_table)
		return -ENOMEM;
	smu_table->metrics_time = 0;

	smu_table->gpu_metrics_table_size = sizeof(struct gpu_metrics_v2_1);
	smu_table->gpu_metrics_table = kzalloc(
			smu_table->gpu_metrics_table_size,
			GFP_KERNEL);
	if (!smu_table->gpu_metrics_table) {
		kfree(smu_table->metrics_table);
		return -ENOMEM;
	}

	smu_table->clocks_table = kzalloc(sizeof(struct DpmClocks_t),
					  GFP_KERNEL);
	if (!smu_table->clocks_table) {
		kfree(smu_table->gpu_metrics_table);
		kfree(smu_table->metrics_table);
		return -ENOMEM;
	}

	return 0;
}

static int mero_get_smu_clk_index(struct smu_context *smc, uint32_t index)
{
	struct smu_11_0_cmn2aisc_mapping mapping;

	if (index >= SMU_CLK_COUNT)
		return -EINVAL;

	mapping = mero_clk_map[index];
	if (!(mapping.valid_mapping))
		return -EINVAL;

	return mapping.map_to;
}

static int mero_get_metrics_table_locked(struct smu_context *smu,
		void *metrics_table, bool bypass_cache)
{
	struct smu_table_context *smu_table = &smu->smu_table;
	uint32_t table_size = smu_table->tables[SMU_TABLE_SMU_METRICS].size;
	int ret = 0;

	if (bypass_cache ||
		!smu_table->metrics_time ||
		time_after(jiffies,
			smu_table->metrics_time + msecs_to_jiffies(100))) {
		ret = smu_update_table(smu, SMU_TABLE_SMU_METRICS, 0,
				(void *)smu_table->metrics_table, false);
		if (ret) {
			pr_info("Failed to export SMU metrics table!\n");
			return ret;
		}
		smu_table->metrics_time = jiffies;
	}

	if (metrics_table)
		memcpy(metrics_table, smu_table->metrics_table, table_size);

	return ret;
}

static int mero_allocate_dpm_context(struct smu_context *smu)
{
	struct smu_dpm_context *smu_dpm = &smu->smu_dpm;

	if (smu_dpm->dpm_context)
		return -EINVAL;

	smu_dpm->dpm_context = kzalloc(sizeof(*smu_dpm->dpm_context),
				       GFP_KERNEL);
	if (!smu_dpm->dpm_context)
		return -ENOMEM;

	smu_dpm->dpm_context_size = sizeof(struct smu_11_0_dpm_context);

	return 0;
}

static int mero_dpm_set_uvd_enable(struct smu_context *smu, bool enable)
{
	struct smu_power_context *smu_power = &smu->smu_power;
	struct smu_power_gate *power_gate = &smu_power->power_gate;
	int ret = 0;

	if (enable) {
		/* vcn dpm on is a prerequisite for vcn power gate messages */
		ret = smu_send_smc_msg_with_param(smu,
				SMU_MSG_PowerUpVcn, 1, NULL);
		if (ret)
			return ret;
		power_gate->vcn_gated = false;
	} else {
		ret = smu_send_smc_msg(smu, SMU_MSG_PowerDownVcn);
		if (ret)
			return ret;
		power_gate->vcn_gated = true;
	}

	return ret;
}

static int mero_dpm_set_jpeg_enable(struct smu_context *smu, bool enable)
{
	struct smu_power_context *smu_power = &smu->smu_power;
	struct smu_power_gate *power_gate = &smu_power->power_gate;
	int ret = 0;

	if (enable) {
		ret = smu_send_smc_msg(smu, SMU_MSG_PowerUpJpeg);
		if (ret)
			return ret;
		power_gate->jpeg_gated = false;
	} else {
		ret = smu_send_smc_msg(smu, SMU_MSG_PowerDownJpeg);
		if (ret)
			return ret;
		power_gate->jpeg_gated = true;
	}

	return ret;
}

bool mero_clk_dpm_is_enabled(struct smu_context *smu,
				enum smu_clk_type clk_type)
{
	enum smu_feature_mask feature_id = 0;

	switch (clk_type) {
	case SMU_MCLK:
	case SMU_UCLK:
	case SMU_FCLK:
		feature_id = SMU_FEATURE_DPM_FCLK_BIT;
		break;
	case SMU_GFXCLK:
	case SMU_SCLK:
		feature_id = SMU_FEATURE_DPM_GFXCLK_BIT;
		break;
	case SMU_SOCCLK:
		feature_id = SMU_FEATURE_DPM_SOCCLK_BIT;
		break;
	case SMU_VCLK:
	case SMU_DCLK:
		feature_id = SMU_FEATURE_VCN_DPM_BIT;
		break;
	default:
		return true;
	}

	if (!smu_feature_is_enabled(smu, feature_id))
		return false;

	return true;
}

static void mero_save_user_clk_info(struct smu_context *smu,
		   enum smu_clk_type clk_type,
		   uint32_t min_freq,
		   uint32_t max_freq)
{
	struct smu_dpm_freq_info *dpm_freq;

	/*
	 * we have different name for same clock, use common
	 * name internally
	 */
	if (clk_type == SMU_SCLK)
		clk_type = SMU_GFXCLK;

	if (clk_type == SMU_UCLK)
		clk_type = SMU_MCLK;

	/* modify if already exist */
	list_for_each_entry(dpm_freq, &smu->dpm_freq_info_list, node) {
		if (dpm_freq->clk_type == clk_type) {
			/* do not update value if frequency is zero */
			dpm_freq->min_freq = min_freq ?
					min_freq : dpm_freq->min_freq;
			dpm_freq->max_freq = max_freq ?
					max_freq : dpm_freq->max_freq;
			return;
		}
	}

	dpm_freq = kzalloc(sizeof(struct smu_dpm_freq_info), GFP_KERNEL);
	if (dpm_freq == NULL)
		return;
	dpm_freq->clk_type = clk_type;
	/* do not update value if frequency is zero */
	dpm_freq->min_freq = min_freq ? min_freq : dpm_freq->min_freq;
	dpm_freq->max_freq = max_freq ? max_freq : dpm_freq->max_freq;
	list_add(&dpm_freq->node, &smu->dpm_freq_info_list);
}

static int mero_get_user_clk_info(struct smu_context *smu,
		   enum smu_clk_type clk_type,
		   uint32_t *min_freq,
		   uint32_t *max_freq)
{
	struct smu_dpm_freq_info *dpm_freq;

	/*
	 * we have different name for same clock, use common
	 * name internally
	 */
	if (clk_type == SMU_SCLK)
		clk_type = SMU_GFXCLK;

	if (clk_type == SMU_UCLK)
		clk_type = SMU_MCLK;

	list_for_each_entry(dpm_freq, &smu->dpm_freq_info_list, node) {
		if (dpm_freq->clk_type == clk_type) {
			*min_freq = dpm_freq->min_freq;
			*max_freq = dpm_freq->max_freq;
			return 0;
		}
	}

	return -ENOENT;
}

static int mero_get_dpm_clk_limited(struct smu_context *smu, enum smu_clk_type clk_type,
						uint32_t dpm_level, uint32_t *freq)
{
	struct DpmClocks_t *clk_table = smu->smu_table.clocks_table;

	if (!clk_table || clk_type >= SMU_CLK_COUNT)
		return -EINVAL;

	switch (clk_type) {
	case SMU_SOCCLK:
		if (dpm_level >= clk_table->NumSocClkLevelsEnabled)
			return -EINVAL;
		*freq = clk_table->SocClocks[dpm_level];
		break;
	case SMU_VCLK:
		if (dpm_level >= clk_table->VcnClkLevelsEnabled)
			return -EINVAL;
		*freq = clk_table->VcnClocks[dpm_level].vclk;
		break;
	case SMU_DCLK:
		if (dpm_level >= clk_table->VcnClkLevelsEnabled)
			return -EINVAL;
		*freq = clk_table->VcnClocks[dpm_level].dclk;
		break;
	case SMU_UCLK:
	case SMU_MCLK:
		if (dpm_level >= clk_table->NumDfPstatesEnabled)
			return -EINVAL;
		*freq = clk_table->DfPstateTable[dpm_level].memclk;

		break;
	case SMU_FCLK:
		if (dpm_level >= clk_table->NumDfPstatesEnabled)
			return -EINVAL;
		*freq = clk_table->DfPstateTable[dpm_level].fclk;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int mero_get_profiling_clk_mask(struct smu_context *smu,
					 enum amd_dpm_forced_level level,
					 uint32_t *vclk_mask,
					 uint32_t *dclk_mask,
					 uint32_t *mclk_mask,
					 uint32_t *fclk_mask,
					 uint32_t *soc_mask)
{
	struct DpmClocks_t *clk_table = smu->smu_table.clocks_table;

	if (level == AMD_DPM_FORCED_LEVEL_PROFILE_MIN_SCLK) {
		if (soc_mask)
			*soc_mask = 0;
	} else if (level == AMD_DPM_FORCED_LEVEL_PROFILE_MIN_MCLK) {
		if (mclk_mask)
			/* mclk levels are in reverse order */
			*mclk_mask = clk_table->NumDfPstatesEnabled - 1;
		if (fclk_mask)
			/* fclk levels are in reverse order */
			*fclk_mask = clk_table->NumDfPstatesEnabled - 1;
	} else if (level == AMD_DPM_FORCED_LEVEL_PROFILE_STANDARD) {
		if (mclk_mask)
			*mclk_mask = 0;
		if (fclk_mask)
			*fclk_mask = 0;
		if (soc_mask)
			*soc_mask = 1;
		if (vclk_mask)
			*vclk_mask = 1;
		if (dclk_mask)
			*dclk_mask = 1;

	} else if (level == AMD_DPM_FORCED_LEVEL_PROFILE_PEAK) {
		if (mclk_mask)
			/* mclk levels are in reverse order */
			*mclk_mask = 0;
		if (fclk_mask)
			/* fclk levels are in reverse order */
			*fclk_mask = 0;
		if (soc_mask)
			*soc_mask = clk_table->NumSocClkLevelsEnabled - 1;
		if (vclk_mask)
			*vclk_mask = clk_table->VcnClkLevelsEnabled - 1;
		if (dclk_mask)
			*dclk_mask = clk_table->VcnClkLevelsEnabled - 1;
	} else if (level == AMD_DPM_FORCED_LEVEL_LOW) {
		if (mclk_mask)
			/*
			 * mclk levels are in reverse order, we keep
			 * MCLK to max to avoid system hang
			 */
			*mclk_mask = 0;
		if (fclk_mask)
			/*
			 * fclk levels are in reverse order, we keep
			 * FCLK to max to avoid system hang
			 */
			*fclk_mask = 0;
		if (soc_mask)
			*soc_mask = 0;
		if (vclk_mask)
			*vclk_mask = 0;
		if (dclk_mask)
			*dclk_mask = 0;
	}

	return 0;
}

static int mero_get_dpm_ultimate_freq(struct smu_context *smu,
					enum smu_clk_type clk_type,
					uint32_t *min,
					uint32_t *max)
{
	int ret = 0;
	uint32_t soc_mask = NUM_SOCCLK_DPM_LEVELS;
	uint32_t vclk_mask = NUM_VCN_DPM_LEVELS;
	uint32_t dclk_mask = NUM_VCN_DPM_LEVELS;
	uint32_t mclk_mask = NUM_FCLK_DPM_LEVELS;
	uint32_t fclk_mask = NUM_FCLK_DPM_LEVELS;

	if (max) {
		ret = mero_get_profiling_clk_mask(smu,
				AMD_DPM_FORCED_LEVEL_PROFILE_PEAK,
				&vclk_mask,
				&dclk_mask,
				&mclk_mask,
				&fclk_mask,
				&soc_mask);
		if (ret)
			goto failed;

		switch (clk_type) {
		case SMU_GFXCLK:
		case SMU_SCLK:
			*max = smu->gfx_default_soft_max_freq;
			break;
		case SMU_UCLK:
		case SMU_MCLK:
			ret = mero_get_dpm_clk_limited(smu,
				clk_type, mclk_mask, max);
			if (ret)
				goto failed;
			break;
		case SMU_SOCCLK:
			ret = mero_get_dpm_clk_limited(smu,
				clk_type, soc_mask, max);
			if (ret)
				goto failed;
			break;
		case SMU_FCLK:
			ret = mero_get_dpm_clk_limited(smu,
				clk_type, fclk_mask, max);
			if (ret)
				goto failed;
			break;
		case SMU_VCLK:
			ret = mero_get_dpm_clk_limited(smu,
				clk_type, vclk_mask, max);
			if (ret)
				goto failed;
			break;
		case SMU_DCLK:
			ret = mero_get_dpm_clk_limited(smu,
				clk_type, dclk_mask, max);
			if (ret)
				goto failed;
			break;
		default:
			ret = -EINVAL;
			goto failed;
		}
	}
	if (min) {
		ret = mero_get_profiling_clk_mask(smu,
				AMD_DPM_FORCED_LEVEL_LOW,
				&vclk_mask,
				&dclk_mask,
				&mclk_mask,
				&fclk_mask,
				&soc_mask);
		if (ret)
			goto failed;

		switch (clk_type) {
		case SMU_GFXCLK:
		case SMU_SCLK:
			*min = smu->gfx_default_hard_min_freq;
			break;
		case SMU_UCLK:
		case SMU_MCLK:
			ret = mero_get_dpm_clk_limited(smu,
				clk_type, mclk_mask, min);
			if (ret)
				goto failed;
			break;
		case SMU_SOCCLK:
			ret = mero_get_dpm_clk_limited(smu,
				clk_type, soc_mask, min);
			if (ret)
				goto failed;
			break;
		case SMU_FCLK:
			ret = mero_get_dpm_clk_limited(smu,
				clk_type, fclk_mask, min);
			if (ret)
				goto failed;
			break;
		case SMU_VCLK:
			ret = mero_get_dpm_clk_limited(smu,
				clk_type, vclk_mask, min);
			if (ret)
				goto failed;
			break;
		case SMU_DCLK:
			ret = mero_get_dpm_clk_limited(smu,
				clk_type, dclk_mask, min);
			if (ret)
				goto failed;
			break;
		default:
			ret = -EINVAL;
			goto failed;
		}
	}
failed:
	return ret;
}

static int mero_set_soft_freq_limited_range(struct smu_context *smu,
					  enum smu_clk_type clk_type,
					  uint32_t min,
					  uint32_t max)
{
	int ret = 0;

	if (!mero_clk_dpm_is_enabled(smu, clk_type))
		return 0;

	pr_debug("set clock %d %d-%d\n", clk_type, min, max);
	switch (clk_type) {
	case SMU_GFXCLK:
	case SMU_SCLK:
		ret = smu_send_smc_msg_with_param(smu,
					SMU_MSG_SetHardMinGfxClk,
					min, NULL);
		if (ret)
			return ret;

		ret = smu_send_smc_msg_with_param(smu,
					SMU_MSG_SetSoftMaxGfxClk,
					max, NULL);
		if (ret)
			return ret;
		break;
	case SMU_FCLK:
	case SMU_MCLK:
		ret = smu_send_smc_msg_with_param(smu,
					SMU_MSG_SetHardMinFclkByFreq,
					min, NULL);
		if (ret)
			return ret;

		ret = smu_send_smc_msg_with_param(smu,
					SMU_MSG_SetSoftMaxFclkByFreq,
					max, NULL);
		if (ret)
			return ret;
		break;
	case SMU_SOCCLK:
		ret = smu_send_smc_msg_with_param(smu,
					SMU_MSG_SetHardMinSocclkByFreq,
					min, NULL);
		if (ret)
			return ret;

		ret = smu_send_smc_msg_with_param(smu,
					SMU_MSG_SetSoftMaxSocclkByFreq,
					max, NULL);
		if (ret)
			return ret;
		break;
	case SMU_VCLK:
		ret = smu_send_smc_msg_with_param(smu,
					SMU_MSG_SetHardMinVcn,
					min << 16, NULL);
		if (ret)
			return ret;
		ret = smu_send_smc_msg_with_param(smu,
					SMU_MSG_SetSoftMaxVcn,
					max << 16, NULL);
		if (ret)
			return ret;
		break;
	case SMU_DCLK:
		ret = smu_send_smc_msg_with_param(smu,
					SMU_MSG_SetHardMinVcn,
					min, NULL);
		if (ret)
			return ret;
		ret = smu_send_smc_msg_with_param(smu,
					SMU_MSG_SetSoftMaxVcn,
					max, NULL);
		if (ret)
			return ret;
		break;
	default:
		return -EINVAL;
	}

	return ret;
}

static int mero_force_dpm_limit_value(struct smu_context *smu, bool highest)
{
	int ret = 0, i = 0;
	uint32_t min_freq, max_freq, force_freq;
	enum smu_clk_type clk_type;

	enum smu_clk_type clks[] = {
		SMU_SOCCLK,
		SMU_VCLK,
		SMU_DCLK,
		SMU_MCLK,
		SMU_FCLK,
	};

	for (i = 0; i < ARRAY_SIZE(clks); i++) {
		clk_type = clks[i];
		ret = mero_get_dpm_ultimate_freq(smu,
				clk_type, &min_freq, &max_freq);
		if (ret)
			return ret;

		force_freq = highest ? max_freq : min_freq;
		ret = mero_set_soft_freq_limited_range(smu,
				clk_type, force_freq, force_freq);
		if (ret)
			return ret;
	}

	return ret;
}

static int mero_unforce_dpm_level(struct smu_context *smu, enum smu_clk_type clk_type)
{
	int i, ret = 0;
	uint32_t min_freq, max_freq;

	struct clk_feature_map {
		enum smu_clk_type clk_type;
		uint32_t	feature;
	} clk_feature_map[] = {
		{SMU_MCLK, SMU_FEATURE_DPM_FCLK_BIT},
		{SMU_FCLK, SMU_FEATURE_DPM_FCLK_BIT},
		{SMU_SOCCLK, SMU_FEATURE_DPM_SOCCLK_BIT},
		{SMU_VCLK, SMU_FEATURE_VCN_DPM_BIT},
		{SMU_DCLK, SMU_FEATURE_VCN_DPM_BIT},
		{SMU_GFXCLK, SMU_FEATURE_DPM_GFXCLK_BIT},
	};

	for (i = 0; i < ARRAY_SIZE(clk_feature_map); i++)
		if (clk_feature_map[i].clk_type == clk_type)
			break;

	if (i == ARRAY_SIZE(clk_feature_map))
		return ret;

	if (!smu_feature_is_enabled(smu, clk_feature_map[i].feature)) {
		pr_err("clk_type[%d]: smu feature is not enabled!", clk_type);
		return ret;
	}

	ret = mero_get_dpm_ultimate_freq(smu,
			clk_type, &min_freq, &max_freq);

	if (ret)
		return ret;

	if (clk_type == SMU_GFXCLK) {
		smu->gfx_actual_hard_min_freq = min_freq;
		smu->gfx_actual_soft_max_freq = max_freq;
	}

	ret = mero_set_soft_freq_limited_range(smu,
			clk_type, min_freq, max_freq);

	mero_save_user_clk_info(smu, clk_type, min_freq, max_freq);

	return ret;
}

static int mero_unforce_dpm_levels(struct smu_context *smu, bool restore_user)
{
	int ret = 0, i = 0;
	uint32_t min_freq, max_freq, user_min_freq, user_max_freq;
	enum smu_clk_type clk_type;

	struct clk_feature_map {
		enum smu_clk_type clk_type;
		uint32_t	feature;
	} clk_feature_map[] = {
		{SMU_MCLK, SMU_FEATURE_DPM_FCLK_BIT},
		{SMU_FCLK, SMU_FEATURE_DPM_FCLK_BIT},
		{SMU_SOCCLK, SMU_FEATURE_DPM_SOCCLK_BIT},
		{SMU_VCLK, SMU_FEATURE_VCN_DPM_BIT},
		{SMU_DCLK, SMU_FEATURE_VCN_DPM_BIT},
		{SMU_GFXCLK, SMU_FEATURE_DPM_GFXCLK_BIT},
	};

	for (i = 0; i < ARRAY_SIZE(clk_feature_map); i++) {

		if (!smu_feature_is_enabled(smu, clk_feature_map[i].feature))
			continue;

		clk_type = clk_feature_map[i].clk_type;

		ret = mero_get_dpm_ultimate_freq(smu,
				clk_type, &min_freq, &max_freq);

		if (ret)
			return ret;

		if (restore_user) {
			ret = mero_get_user_clk_info(smu, clk_type,
					&user_min_freq, &user_max_freq);
			if (ret == 0) {
				min_freq = user_min_freq ?
						user_min_freq : min_freq;
				max_freq = user_max_freq ?
						user_max_freq : max_freq;
			}
		}

		if (clk_type == SMU_GFXCLK) {
			smu->gfx_actual_hard_min_freq = min_freq;
			smu->gfx_actual_soft_max_freq = max_freq;
		}

		ret = mero_set_soft_freq_limited_range(smu,
				clk_type, min_freq, max_freq);

		if (ret)
			return ret;
	}

	return ret;
}

static int mero_get_smu_msg_index(struct smu_context *smc, uint32_t index)
{
	struct smu_13_0_cmn2aisc_mapping mapping;

	if (index >= SMU_MSG_MAX_COUNT)
		return -EINVAL;

	mapping = mero_message_map[index];
	if (!(mapping.valid_mapping))
		return -EINVAL;

	return mapping.map_to;
}

static int mero_get_legacy_smu_metrics_data(struct smu_context *smu,
				       enum MetricsMember member,
				       uint32_t *value)
{
	struct smu_table_context *smu_table = &smu->smu_table;
	struct SmuMetrics_legacy *metrics =
			(struct SmuMetrics_legacy *)smu_table->metrics_table;
	int ret = 0;

	mutex_lock(&smu->metrics_lock);

	ret = mero_get_metrics_table_locked(smu,
			NULL, false);

	if (ret) {
		mutex_unlock(&smu->metrics_lock);
		return ret;
	}

	switch (member) {
	case METRICS_AVERAGE_GFXCLK:
		*value = metrics->GfxclkFrequency;
		break;
	case METRICS_AVERAGE_SOCCLK:
		*value = metrics->SocclkFrequency;
		break;
	case METRICS_AVERAGE_VCLK:
		*value = metrics->VclkFrequency;
		break;
	case METRICS_AVERAGE_DCLK:
		*value = metrics->DclkFrequency;
		break;
	case METRICS_AVERAGE_UCLK:
		*value = metrics->MemclkFrequency;
		break;
	case METRICS_AVERAGE_GFXACTIVITY:
		*value = metrics->GfxActivity / 100;
		break;
	case METRICS_AVERAGE_VCNACTIVITY:
		*value = metrics->UvdActivity;
		break;
	case METRICS_AVERAGE_SOCKETPOWER:
		*value = (metrics->CurrentSocketPower << 8) /
		1000;
		break;
	case METRICS_TEMPERATURE_EDGE:
		*value = metrics->GfxTemperature / 100 *
		SMU_TEMPERATURE_UNITS_PER_CENTIGRADES;
		break;
	case METRICS_TEMPERATURE_HOTSPOT:
		*value = metrics->SocTemperature / 100 *
		SMU_TEMPERATURE_UNITS_PER_CENTIGRADES;
		break;
	case METRICS_THROTTLER_STATUS:
		*value = metrics->ThrottlerStatus;
		break;
	case METRICS_VOLTAGE_VDDGFX:
		*value = metrics->Voltage[2];
		break;
	case METRICS_VOLTAGE_VDDSOC:
		*value = metrics->Voltage[1];
		break;
	case METRICS_AVERAGE_CPUCLK:
		memcpy(value, &metrics->CoreFrequency[0],
		       smu->cpu_core_num * sizeof(uint16_t));
		break;
	default:
		*value = UINT_MAX;
		break;
	}

	mutex_unlock(&smu->metrics_lock);

	return ret;
}

static int mero_get_smu_metrics_data(struct smu_context *smu,
				       enum MetricsMember member,
				       uint32_t *value)
{
	struct smu_table_context *smu_table = &smu->smu_table;
	struct SmuMetrics *metrics =
			(struct SmuMetrics *)smu_table->metrics_table;
	int ret = 0;

	mutex_lock(&smu->metrics_lock);

	ret = mero_get_metrics_table_locked(smu,
			NULL, false);

	if (ret) {
		mutex_unlock(&smu->metrics_lock);
		return ret;
	}

	switch (member) {
	case METRICS_AVERAGE_GFXCLK:
		*value = metrics->Current.GfxclkFrequency;
		break;
	case METRICS_AVERAGE_SOCCLK:
		*value = metrics->Current.SocclkFrequency;
		break;
	case METRICS_AVERAGE_VCLK:
		*value = metrics->Current.VclkFrequency;
		break;
	case METRICS_AVERAGE_DCLK:
		*value = metrics->Current.DclkFrequency;
		break;
	case METRICS_AVERAGE_UCLK:
		*value = metrics->Current.MemclkFrequency;
		break;
	case METRICS_AVERAGE_GFXACTIVITY:
		*value = metrics->Current.GfxActivity;
		break;
	case METRICS_AVERAGE_VCNACTIVITY:
		*value = metrics->Current.UvdActivity;
		break;
	case METRICS_AVERAGE_SOCKETPOWER:
		*value = (metrics->Current.CurrentSocketPower << 8) /
		1000;
		break;
	case METRICS_TEMPERATURE_EDGE:
		*value = metrics->Current.GfxTemperature / 100 *
		SMU_TEMPERATURE_UNITS_PER_CENTIGRADES;
		break;
	case METRICS_TEMPERATURE_HOTSPOT:
		*value = metrics->Current.SocTemperature / 100 *
		SMU_TEMPERATURE_UNITS_PER_CENTIGRADES;
		break;
	case METRICS_THROTTLER_STATUS:
		*value = metrics->Current.ThrottlerStatus;
		break;
	case METRICS_VOLTAGE_VDDGFX:
		*value = metrics->Current.Voltage[2];
		break;
	case METRICS_VOLTAGE_VDDSOC:
		*value = metrics->Current.Voltage[1];
		break;
	case METRICS_AVERAGE_CPUCLK:
		memcpy(value, &metrics->Current.CoreFrequency[0],
		       smu->cpu_core_num * sizeof(uint16_t));
		break;
	default:
		*value = UINT_MAX;
		break;
	}

	mutex_unlock(&smu->metrics_lock);

	return ret;
}

static int mero_common_get_smu_metrics_data(struct smu_context *smu,
				       enum MetricsMember member,
				       uint32_t *value)
{
	struct amdgpu_device *adev = smu->adev;
	uint32_t if_version;
	int ret = 0;

	ret = smu_get_smc_version(smu, &if_version, NULL);
	if (ret) {
		dev_err(adev->dev, "Failed to get smu if version!\n");
		return ret;
	}

	if (if_version < 0x3)
		ret = mero_get_legacy_smu_metrics_data(smu, member, value);
	else
		ret = mero_get_smu_metrics_data(smu, member, value);

	return ret;
}

static void mero_set_gpu_metric_header(struct gpu_metrics_v2_1 *gpu_metrics)
{
	struct metrics_table_header *header =
			(struct metrics_table_header *)gpu_metrics;

	memset(header, 0xFF, sizeof(struct gpu_metrics_v2_1));
	header->format_revision = 2;
	header->content_revision = 1;
	header->structure_size = sizeof(struct gpu_metrics_v2_1);
}

static ssize_t mero_get_legacy_gpu_metrics(struct smu_context *smu,
				      void **table)
{
	struct smu_table_context *smu_table = &smu->smu_table;
	struct gpu_metrics_v2_1 *gpu_metrics =
		(struct gpu_metrics_v2_1 *)smu_table->gpu_metrics_table;
	struct SmuMetrics_legacy metrics;
	int ret = 0;

	mutex_lock(&smu->metrics_lock);
	ret = mero_get_metrics_table_locked(smu,
			&metrics, true);
	mutex_unlock(&smu->metrics_lock);

	if (ret)
		return ret;

	mero_set_gpu_metric_header(gpu_metrics);

	gpu_metrics->temperature_gfx = metrics.GfxTemperature;
	gpu_metrics->temperature_soc = metrics.SocTemperature;
	memcpy(&gpu_metrics->temperature_core[0],
		&metrics.CoreTemperature[0],
		sizeof(uint16_t) * 4);
	gpu_metrics->temperature_l3[0] = metrics.L3Temperature[0];

	gpu_metrics->average_gfx_activity = metrics.GfxActivity;
	gpu_metrics->average_mm_activity = metrics.UvdActivity;

	gpu_metrics->average_socket_power = metrics.CurrentSocketPower;
	gpu_metrics->average_cpu_power = metrics.Power[0];
	gpu_metrics->average_soc_power = metrics.Power[1];
	gpu_metrics->average_gfx_power = metrics.Power[2];
	memcpy(&gpu_metrics->average_core_power[0],
		&metrics.CorePower[0],
		sizeof(uint16_t) * 4);

	gpu_metrics->average_gfxclk_frequency = metrics.GfxclkFrequency;
	gpu_metrics->average_socclk_frequency = metrics.SocclkFrequency;
	gpu_metrics->average_uclk_frequency = metrics.MemclkFrequency;
	gpu_metrics->average_fclk_frequency = metrics.MemclkFrequency;
	gpu_metrics->average_vclk_frequency = metrics.VclkFrequency;
	gpu_metrics->average_dclk_frequency = metrics.DclkFrequency;

	memcpy(&gpu_metrics->current_coreclk[0],
		&metrics.CoreFrequency[0],
		sizeof(uint16_t) * 4);
	gpu_metrics->current_l3clk[0] = metrics.L3Frequency[0];

	gpu_metrics->throttle_status = metrics.ThrottlerStatus;

	gpu_metrics->system_clock_counter = ktime_get_boottime_ns();

	*table = (void *)gpu_metrics;

	return sizeof(struct gpu_metrics_v2_1);
}

static ssize_t mero_get_gpu_metrics(struct smu_context *smu,
				      void **table)
{
	struct smu_table_context *smu_table = &smu->smu_table;
	struct gpu_metrics_v2_1 *gpu_metrics =
		(struct gpu_metrics_v2_1 *)smu_table->gpu_metrics_table;
	struct SmuMetrics metrics;
	int ret = 0;

	mutex_lock(&smu->metrics_lock);
	ret = mero_get_metrics_table_locked(smu,
			&metrics, true);
	mutex_unlock(&smu->metrics_lock);

	if (ret)
		return ret;

	mero_set_gpu_metric_header(gpu_metrics);

	gpu_metrics->temperature_gfx = metrics.Current.GfxTemperature;
	gpu_metrics->temperature_soc = metrics.Current.SocTemperature;
	memcpy(&gpu_metrics->temperature_core[0],
		&metrics.Current.CoreTemperature[0],
		sizeof(uint16_t) * 4);
	gpu_metrics->temperature_l3[0] = metrics.Current.L3Temperature[0];

	gpu_metrics->average_gfx_activity = metrics.Current.GfxActivity;
	gpu_metrics->average_mm_activity = metrics.Current.UvdActivity;

	gpu_metrics->average_socket_power = metrics.Current.CurrentSocketPower;
	gpu_metrics->average_cpu_power = metrics.Current.Power[0];
	gpu_metrics->average_soc_power = metrics.Current.Power[1];
	gpu_metrics->average_gfx_power = metrics.Current.Power[2];
	memcpy(&gpu_metrics->average_core_power[0],
		&metrics.Average.CorePower[0],
		sizeof(uint16_t) * 4);

	gpu_metrics->average_gfxclk_frequency = metrics.Average.GfxclkFrequency;
	gpu_metrics->average_socclk_frequency = metrics.Average.SocclkFrequency;
	gpu_metrics->average_uclk_frequency = metrics.Average.MemclkFrequency;
	gpu_metrics->average_fclk_frequency = metrics.Average.MemclkFrequency;
	gpu_metrics->average_vclk_frequency = metrics.Average.VclkFrequency;
	gpu_metrics->average_dclk_frequency = metrics.Average.DclkFrequency;

	gpu_metrics->current_gfxclk = metrics.Current.GfxclkFrequency;
	gpu_metrics->current_socclk = metrics.Current.SocclkFrequency;
	gpu_metrics->current_uclk = metrics.Current.MemclkFrequency;
	gpu_metrics->current_fclk = metrics.Current.MemclkFrequency;
	gpu_metrics->current_vclk = metrics.Current.VclkFrequency;
	gpu_metrics->current_dclk = metrics.Current.DclkFrequency;

	memcpy(&gpu_metrics->current_coreclk[0],
		&metrics.Current.CoreFrequency[0],
		sizeof(uint16_t) * 4);
	gpu_metrics->current_l3clk[0] = metrics.Current.L3Frequency[0];

	gpu_metrics->throttle_status = metrics.Current.ThrottlerStatus;

	gpu_metrics->system_clock_counter = ktime_get_boottime_ns();

	*table = (void *)gpu_metrics;

	return sizeof(struct gpu_metrics_v2_1);
}

static ssize_t mero_common_get_gpu_metrics(struct smu_context *smu,
				      void **table)
{
	struct amdgpu_device *adev = smu->adev;
	uint32_t if_version;
	int ret = 0;

	ret = smu_get_smc_version(smu, &if_version, NULL);
	if (ret) {
		dev_err(adev->dev, "Failed to get smu if version!\n");
		return ret;
	}

	if (if_version < 0x3)
		ret = mero_get_legacy_gpu_metrics(smu, table);
	else
		ret = mero_get_gpu_metrics(smu, table);

	return ret;
}

static int mero_get_gpu_power(struct smu_context *smu, uint32_t *value)
{
	int ret = 0;

	if (!value)
		return -EINVAL;

	ret = mero_common_get_smu_metrics_data(smu,
			METRICS_AVERAGE_SOCKETPOWER, value);

	return ret;
}

static int mero_get_gpu_temperature(struct smu_context *smu, uint32_t *value)
{
	int ret = 0;

	if (!value)
		return -EINVAL;

	ret = mero_common_get_smu_metrics_data(smu,
			METRICS_TEMPERATURE_EDGE, value);

	return ret;
}

static int smu_v11_0_mode2_reset(struct smu_context *smu)
{
	return smu_send_smc_msg_with_param(smu,
					     SMU_MSG_GfxDeviceDriverReset,
					     SMU_RESET_MODE_2, NULL);
}

static int mero_read_sensor(struct smu_context *smu,
				 enum amd_pp_sensors sensor,
				 void *data, uint32_t *size)
{
	int ret = 0;
	u32 value = 0;

	if (!data || !size)
		return -EINVAL;

	mutex_lock(&smu->sensor_lock);
	switch (sensor) {
	case AMDGPU_PP_SENSOR_CPU_POWER_CUR:
		ret = smu_send_smc_msg_with_param(smu,
			SMU_MSG_GetCurrentPower, RAIL_CPU,
			&value);
		break;
	case AMDGPU_PP_SENSOR_CPU_POWER_AVE:
		ret = smu_send_smc_msg_with_param(smu,
			SMU_MSG_GetAveragePower, RAIL_CPU,
			&value);
		break;
	case AMDGPU_PP_SENSOR_GFX_POWER_CUR:
		ret = smu_send_smc_msg_with_param(smu,
			SMU_MSG_GetCurrentPower, RAIL_GFX,
			&value);
		break;
	case AMDGPU_PP_SENSOR_GFX_POWER_AVE:
		ret = smu_send_smc_msg_with_param(smu,
			SMU_MSG_GetAveragePower, RAIL_GFX,
			&value);
		break;
	case AMDGPU_PP_SENSOR_SOC_POWER_CUR:
		ret = smu_send_smc_msg_with_param(smu,
			SMU_MSG_GetCurrentPower, RAIL_SOC,
			&value);
		break;
	case AMDGPU_PP_SENSOR_SOC_POWER_AVE:
		ret = smu_send_smc_msg_with_param(smu,
			SMU_MSG_GetAveragePower, RAIL_SOC,
			&value);
		break;
	case AMDGPU_PP_SENSOR_CVIP_POWER_CUR:
		ret = smu_send_smc_msg_with_param(smu,
			SMU_MSG_GetCurrentPower, RAIL_CVIP,
			&value);
		break;
	case AMDGPU_PP_SENSOR_CVIP_POWER_AVE:
		ret = smu_send_smc_msg_with_param(smu,
			SMU_MSG_GetAveragePower, RAIL_CVIP,
			&value);
		break;
	case AMDGPU_PP_SENSOR_CPU0_VOL:
		ret = smu_send_smc_msg_with_param(smu,
			SMU_MSG_GetCurrentVoltage, IP_CPU0,
			&value);
		break;
	case AMDGPU_PP_SENSOR_CPU1_VOL:
		ret = smu_send_smc_msg_with_param(smu,
			SMU_MSG_GetCurrentVoltage, IP_CPU1,
			&value);
		break;
	case AMDGPU_PP_SENSOR_CPU2_VOL:
		ret = smu_send_smc_msg_with_param(smu,
			SMU_MSG_GetCurrentVoltage, IP_CPU2,
			&value);
		break;
	case AMDGPU_PP_SENSOR_CPU3_VOL:
		ret = smu_send_smc_msg_with_param(smu,
			SMU_MSG_GetCurrentVoltage, IP_CPU3,
			&value);
		break;
	case AMDGPU_PP_SENSOR_GFX_VOL:
		ret = smu_send_smc_msg_with_param(smu,
			SMU_MSG_GetCurrentVoltage, IP_GFX,
			&value);
		break;
	case AMDGPU_PP_SENSOR_SOC_VOL:
		ret = smu_send_smc_msg_with_param(smu,
			SMU_MSG_GetCurrentVoltage, IP_SOC,
			&value);
		break;
	case AMDGPU_PP_SENSOR_CVIP_VOL:
		ret = smu_send_smc_msg_with_param(smu,
			SMU_MSG_GetCurrentVoltage, IP_CVIP,
			&value);
		break;
	case AMDGPU_PP_SENSOR_CPU_CUR:
		ret = smu_send_smc_msg_with_param(smu,
			SMU_MSG_GetCurrentCurrent, RAIL_CPU,
			&value);
		break;
	case AMDGPU_PP_SENSOR_GFX_CUR:
		ret = smu_send_smc_msg_with_param(smu,
			SMU_MSG_GetCurrentCurrent, RAIL_GFX,
			&value);
		break;
	case AMDGPU_PP_SENSOR_SOC_CUR:
		ret = smu_send_smc_msg_with_param(smu,
			SMU_MSG_GetCurrentCurrent, RAIL_SOC,
			&value);
		break;
	case AMDGPU_PP_SENSOR_CVIP_CUR:
		ret = smu_send_smc_msg_with_param(smu,
			SMU_MSG_GetCurrentCurrent, RAIL_CVIP,
			&value);
		break;
	case AMDGPU_PP_SENSOR_CPU0_TEMP_CUR:
		ret = smu_send_smc_msg_with_param(smu,
			SMU_MSG_GetCurrentTemperature, IP_CPU0,
			&value);
		break;
	case AMDGPU_PP_SENSOR_CPU0_TEMP_AVE:
		ret = smu_send_smc_msg_with_param(smu,
			SMU_MSG_GetAverageTemperature, IP_CPU0,
			&value);
		break;
	case AMDGPU_PP_SENSOR_CPU1_TEMP_CUR:
		ret = smu_send_smc_msg_with_param(smu,
			SMU_MSG_GetCurrentTemperature, IP_CPU1,
			&value);
		break;
	case AMDGPU_PP_SENSOR_CPU1_TEMP_AVE:
		ret = smu_send_smc_msg_with_param(smu,
			SMU_MSG_GetAverageTemperature, IP_CPU1,
			&value);
		break;
	case AMDGPU_PP_SENSOR_CPU2_TEMP_CUR:
		ret = smu_send_smc_msg_with_param(smu,
			SMU_MSG_GetCurrentTemperature, IP_CPU2,
			&value);
		break;
	case AMDGPU_PP_SENSOR_CPU2_TEMP_AVE:
		ret = smu_send_smc_msg_with_param(smu,
			SMU_MSG_GetAverageTemperature, IP_CPU2,
			&value);
		break;
	case AMDGPU_PP_SENSOR_CPU3_TEMP_CUR:
		ret = smu_send_smc_msg_with_param(smu,
			SMU_MSG_GetCurrentTemperature, IP_CPU3,
			&value);
		break;
	case AMDGPU_PP_SENSOR_CPU3_TEMP_AVE:
		ret = smu_send_smc_msg_with_param(smu,
			SMU_MSG_GetAverageTemperature, IP_CPU3,
			&value);
		break;
	case AMDGPU_PP_SENSOR_GFX_TEMP_CUR:
		ret = smu_send_smc_msg_with_param(smu,
			SMU_MSG_GetCurrentTemperature, IP_GFX,
			&value);
		break;
	case AMDGPU_PP_SENSOR_GFX_TEMP_AVE:
		ret = smu_send_smc_msg_with_param(smu,
			SMU_MSG_GetAverageTemperature, IP_GFX,
			&value);
		break;
	case AMDGPU_PP_SENSOR_SOC_TEMP_CUR:
		ret = smu_send_smc_msg_with_param(smu,
			SMU_MSG_GetCurrentTemperature, IP_SOC,
			&value);
		break;
	case AMDGPU_PP_SENSOR_SOC_TEMP_AVE:
		ret = smu_send_smc_msg_with_param(smu,
			SMU_MSG_GetAverageTemperature, IP_SOC,
			&value);
		break;
	case AMDGPU_PP_SENSOR_CVIP_TEMP_CUR:
		ret = smu_send_smc_msg_with_param(smu,
			SMU_MSG_GetCurrentTemperature, IP_CVIP,
			&value);
		break;
	case AMDGPU_PP_SENSOR_CVIP_TEMP_AVE:
		ret = smu_send_smc_msg_with_param(smu,
			SMU_MSG_GetAverageTemperature, IP_CVIP,
			&value);
		break;
	case AMDGPU_PP_SENSOR_CPU0_CLK:
		ret = smu_send_smc_msg_with_param(smu,
			SMU_MSG_GetCurrentFreq, CPU_CORE0CLK_ID,
			&value);
		break;
	case AMDGPU_PP_SENSOR_CPU1_CLK:
		ret = smu_send_smc_msg_with_param(smu,
			SMU_MSG_GetCurrentFreq, CPU_CORE1CLK_ID,
			&value);
		break;
	case AMDGPU_PP_SENSOR_CPU2_CLK:
		ret = smu_send_smc_msg_with_param(smu,
			SMU_MSG_GetCurrentFreq, CPU_CORE2CLK_ID,
			&value);
		break;
	case AMDGPU_PP_SENSOR_CPU3_CLK:
		ret = smu_send_smc_msg_with_param(smu,
			SMU_MSG_GetCurrentFreq, CPU_CORE3CLK_ID,
			&value);
		break;
	case AMDGPU_PP_SENSOR_GFX_CLK:
		ret = smu_send_smc_msg_with_param(smu,
			SMU_MSG_GetCurrentFreq, GFXCLK_ID,
			&value);
		break;
	case AMDGPU_PP_SENSOR_GPU_LOAD:
		ret = smu_send_smc_msg_with_param(smu,
			SMU_MSG_GetAverageGfxActivity, 0,
			&value);
		break;
	case AMDGPU_PP_SENSOR_CPU_LOAD:
		ret = smu_send_smc_msg_with_param(smu,
				SMU_MSG_GetAverageCpuActivity, 0,
				&value);
		break;
	case AMDGPU_PP_SENSOR_GPU_POWER:
		ret = mero_get_gpu_power(smu, (uint32_t *)data);
		*size = 4;
		break;
	case AMDGPU_PP_SENSOR_GPU_TEMP:
		ret = mero_get_gpu_temperature(smu, (uint32_t *)data);
		*size = 4;
		break;
	case AMDGPU_PP_SENSOR_STABLE_PSTATE_SCLK:
		/* caller expect value in 10k unit, hence multiply by 100 */
		*((uint32_t *)data) = MERO_UMD_PSTATE_STANDARD_GFXCLK * 100;
		*size = 4;
		break;
	case AMDGPU_PP_SENSOR_STABLE_PSTATE_MCLK:
		*((uint32_t *)data) = MERO_UMD_PSTATE_STANDARD_FCLK * 100;
		*size = 4;
		break;
	default:
		ret = smu_v11_0_read_sensor(smu, sensor, data, size);
	}
	/* mero sensors */
	if (!ret && sensor >= AMDGPU_PP_SENSOR_GPU_LOAD &&
		sensor < AMDGPU_PP_SENSOR_MERO_MAX) {
		*size = 4;
		*(uint32_t *)data = value;

		/* temperature sensors */
		if (sensor >= AMDGPU_PP_SENSOR_CPU0_TEMP_CUR &&
			sensor <= AMDGPU_PP_SENSOR_CVIP_TEMP_AVE) {
			*(uint32_t *)data = value *
				SMU_TEMPERATURE_UNITS_PER_CENTIGRADES;
		}

		/* clock sensors */
		if (sensor >= AMDGPU_PP_SENSOR_CPU0_CLK &&
			sensor <= AMDGPU_PP_SENSOR_GFX_CLK) {
			*(uint32_t *)data = value * 100;
		}

		if (sensor == AMDGPU_PP_SENSOR_GPU_LOAD)
			*(uint32_t *)data = value > 100 ? 100 : value;
	}
	mutex_unlock(&smu->sensor_lock);

	return ret;
}

static int mero_get_power_limit(struct smu_context *smu,
				     uint32_t *limit,
				     bool def_limit)
{
	int ret = 0;

	if (def_limit) {
		*limit = smu->default_power_limit;
		return 0;
	}

	if (!smu->power_limit) {
		ret = smu_send_smc_msg_with_param(smu, SMU_MSG_GetPptLimit,
						  0, &smu->power_limit);
		if (ret)
			return ret;
	}
	*limit = smu->power_limit;

	return 0;
}

int mero_set_power_limit(struct smu_context *smu, uint32_t n)
{
	int ret = 0;

	if (n == 0)
		n = smu->default_power_limit;
	if (n == smu->power_limit)
		return 0;
	if (n > smu->default_power_limit)
		return -EINVAL;

	ret = smu_send_smc_msg_with_param(smu, SMU_MSG_SetReducedPptLimit, n,
					  NULL);
	if (!ret)
		smu->power_limit = n;
	return ret;
}

static int mero_get_thermal_limit(struct smu_context *smu,
				     uint32_t *limit,
				     bool def_limit)
{
	int ret = 0;

	if (def_limit) {
		*limit = smu->default_thermal_limit;
		return 0;
	}

	if (!smu->thermal_limit) {
		ret = smu_send_smc_msg_with_param(smu, SMU_MSG_GetThermalLimit,
						  0, &smu->thermal_limit);
		if (ret)
			return ret;
	}
	*limit = smu->thermal_limit;

	return 0;
}

static int mero_set_soft_limit_min(struct smu_context *smu,
				   enum smu_clk_type clk_type,
				   uint32_t n)
{
	int ret = 0;

	DRM_DEBUG("set min clk(%d) to be %u\n", clk_type, n);

	switch (clk_type) {
	case SMU_SCLK:
		ret = smu_send_smc_msg_with_param(smu, SMU_MSG_SetSoftMinGfxclk,
						  n, NULL);
		break;

	case SMU_CPU_CLK:
		ret = smu_send_smc_msg_with_param(smu, SMU_MSG_SetSoftMinCclk,
						  n, NULL);
		break;

	default:
		DRM_INFO("clk_type %d is not supported\n", clk_type);
		ret = -EINVAL;
	}

	if (ret)
		return ret;

	mero_save_user_clk_info(smu, clk_type, n, 0);
	return 0;
}

static int mero_set_soft_limit_max(struct smu_context *smu,
				   enum smu_clk_type clk_type,
				   uint32_t n)
{
	int ret = 0;

	DRM_DEBUG("set max clk(%d) to be %u\n", clk_type, n);

	switch (clk_type) {
	case SMU_SCLK:
		ret = smu_send_smc_msg_with_param(smu, SMU_MSG_SetSoftMaxGfxClk,
						  n, NULL);
		break;

	case SMU_CPU_CLK:
		ret = smu_send_smc_msg_with_param(smu, SMU_MSG_SetSoftMaxCclk,
						  n, NULL);
		break;

	default:
		DRM_INFO("clk_type %d is not supported\n", clk_type);
		ret = -EINVAL;
	}

	if (ret)
		return ret;

	mero_save_user_clk_info(smu, clk_type, 0, n);
	return 0;
}

int mero_set_thermal_limit(struct smu_context *smu, uint32_t n)
{
	int ret = 0;

	if (n == 0)
		n = smu->default_thermal_limit;
	if (n == smu->thermal_limit)
		return 0;
	if (n > smu->default_thermal_limit)
		return -EINVAL;

	ret = smu_send_smc_msg_with_param(smu, SMU_MSG_SetReducedThermalLimit,
						n, NULL);
	if (!ret)
		smu->thermal_limit = n;
	return ret;
}

int mero_set_time_limit(struct smu_context *smu, enum amd_pp_sensors sensor_id,
			uint32_t limit)
{
	int ret = 0;
	enum smu_message_type msg;

	switch (sensor_id) {
	case AMDGPU_PP_SENSOR_POWER_TIME:
		msg = SMU_MSG_SetAveragePowerTimeConstant;
		break;
	case AMDGPU_PP_SENSOR_TEM_TIME:
		msg = SMU_MSG_SetAverageTemperatrureTimeConstant;
		break;
	case AMDGPU_PP_SENSOR_ACT_TIME:
		msg = SMU_MSG_SetAverageActivityTimeConstant;
		break;
	case AMDGPU_PP_SENSOR_MIG_END_HYS:
		msg = SMU_MSG_SetMitigationEndHysteresis;
		break;
	default:
		ret = -EINVAL;
	}
	if (ret)
		return ret;

	ret = smu_send_smc_msg_with_param(smu, msg, limit, NULL);
	return ret;
}

static bool mero_is_dpm_running(struct smu_context *smu)
{
	int ret = 0;
	u32 feature_mask[2] = {0xffffffff, 0xffffffff};

	ret = smu_feature_get_enabled_mask(smu, feature_mask, 2);

	return !!(feature_mask[0] & SMC_DPM_FEATURE);
}

static enum amd_pm_state_type
mero_get_current_power_state(struct smu_context *smu)
{
	enum amd_pm_state_type pm_type;
	struct smu_dpm_context *smu_dpm_ctx = &(smu->smu_dpm);
	enum smu_state_classification_flag  flags;
	struct smu_power_state *power_state;

	if (!smu_dpm_ctx->dpm_context ||
			!smu_dpm_ctx->dpm_current_power_state)
		return -EINVAL;

	power_state = smu_dpm_ctx->dpm_current_power_state;
	switch (power_state->classification.ui_label) {
	case SMU_STATE_UI_LABEL_BATTERY:
		pm_type = POWER_STATE_TYPE_BATTERY;
		break;
	case SMU_STATE_UI_LABEL_BALLANCED:
		pm_type = POWER_STATE_TYPE_BALANCED;
		break;
	case SMU_STATE_UI_LABEL_PERFORMANCE:
		pm_type = POWER_STATE_TYPE_PERFORMANCE;
		break;
	default:
		flags = power_state->classification.flags;
		if (flags & SMU_STATE_CLASSIFICATION_FLAG_BOOT)
			pm_type = POWER_STATE_TYPE_INTERNAL_BOOT;
		else
			pm_type = POWER_STATE_TYPE_DEFAULT;
		break;
	}

	return pm_type;
}

static int mero_get_power_profile_mode(struct smu_context *smu,
		char *buf)
{
	static const char *profile_name[7] = {
		"BOOTUP_DEFAULT",
		"3D_FULL_SCREEN",
		"POWER_SAVING",
		"VIDEO",
		"VR",
		"COMPUTE",
		"CUSTOM"};
	uint32_t i, size = 0;
	int16_t workload_type = 0;

	if (!smu->pm_enabled || !buf)
		return -EINVAL;

	for (i = 0; i <= PP_SMC_POWER_PROFILE_CUSTOM; i++) {
		/*
		 * Conv PP_SMC_POWER_PROFILE* to WORKLOAD_PPLIB_*_BIT
		 * Not all profile modes are supported on mero.
		 */
		workload_type = smu_workload_get_type(smu, i);
		if (workload_type < 0)
			continue;

		size += sprintf(buf + size, "%2d %14s%s\n",
				i, profile_name[i],
				(i == smu->power_profile_mode) ? "*" : " ");
	}

	return size;
}

static int mero_set_watermarks_table(struct smu_context *smu,
		void *watermarks,
		struct dm_pp_wm_sets_with_clock_ranges_soc15 *clock_ranges)
{
	int i;
	int ret = 0;
	struct Watermarks_t *table = watermarks;
	struct dm_pp_clock_range_for_dmif_wm_set_soc15 *range_dmif;
	struct dm_pp_clock_range_for_mcif_wm_set_soc15 *range_mcif;

	if (!table || !clock_ranges)
		return -EINVAL;

	if (clock_ranges->num_wm_dmif_sets > 4 ||
			clock_ranges->num_wm_mcif_sets > 4)
		return -EINVAL;

	for (i = 0; i < clock_ranges->num_wm_dmif_sets; i++) {
		range_dmif = &clock_ranges->wm_dmif_clocks_ranges[i];

		table->WatermarkRow[1][i].MinClock =
			cpu_to_le16((uint16_t)
				    (range_dmif->wm_min_dcfclk_clk_in_khz /
				     1000));
		table->WatermarkRow[1][i].MaxClock =
			cpu_to_le16((uint16_t)
				    (range_dmif->wm_max_dcfclk_clk_in_khz /
				     1000));
		table->WatermarkRow[1][i].MinMclk =
			cpu_to_le16((uint16_t)
				    (range_dmif->wm_min_mem_clk_in_khz /
				     1000));
		table->WatermarkRow[1][i].MaxMclk =
			cpu_to_le16((uint16_t)
				    (range_dmif->wm_max_mem_clk_in_khz /
				     1000));
		table->WatermarkRow[1][i].WmSetting = (uint8_t)
			range_dmif->wm_set_id;
	}

	for (i = 0; i < clock_ranges->num_wm_mcif_sets; i++) {
		range_mcif = &clock_ranges->wm_mcif_clocks_ranges[i];

		table->WatermarkRow[0][i].MinClock =
			cpu_to_le16((uint16_t)
				    (range_mcif->wm_min_socclk_clk_in_khz /
				     1000));
		table->WatermarkRow[0][i].MaxClock =
			cpu_to_le16((uint16_t)
				    (range_mcif->wm_max_socclk_clk_in_khz /
				     1000));
		table->WatermarkRow[0][i].MinMclk =
			cpu_to_le16((uint16_t)
				    (range_mcif->wm_min_mem_clk_in_khz /
				     1000));
		table->WatermarkRow[0][i].MaxMclk =
			cpu_to_le16((uint16_t)
				    (range_mcif->wm_max_mem_clk_in_khz /
				     1000));
		table->WatermarkRow[0][i].WmSetting = (uint8_t)
			range_mcif->wm_set_id;
	}

	smu->watermarks_bitmap |= WATERMARKS_EXIST;

	/* pass data to smu controller */
	if (!(smu->watermarks_bitmap & WATERMARKS_LOADED)) {
		ret = smu_write_watermarks_table(smu);
		if (ret) {
			pr_err("Failed to update WMTABLE!");
			return ret;
		}
		smu->watermarks_bitmap |= WATERMARKS_LOADED;
	}

	return 0;
}

static int mero_print_clk_levels(struct smu_context *smu,
		enum smu_clk_type clk_type, char *buf)
{
	int i, size = 0, ret = 0;
	uint32_t cur_value = 0, value = 0, count = 0, min = 0, max = 0;
	struct DpmClocks_t *clk_table = smu->smu_table.clocks_table;
	struct smu_dpm_context *smu_dpm_ctx = &(smu->smu_dpm);
	bool cur_value_match_level = false;

	if (!clk_table || clk_type >= SMU_CLK_COUNT)
		return -EINVAL;

	switch (clk_type) {
	case SMU_OD_SCLK:
		if (smu_dpm_ctx->dpm_level == AMD_DPM_FORCED_LEVEL_MANUAL) {
			size = sprintf(buf, "%s:\n", "OD_SCLK");
			size += sprintf(buf + size, "0: %10uMhz\n",
			(smu->gfx_actual_hard_min_freq > 0) ?
					smu->gfx_actual_hard_min_freq :
					smu->gfx_default_hard_min_freq);
			size += sprintf(buf + size, "1: %10uMhz\n",
			(smu->gfx_actual_soft_max_freq > 0) ?
					smu->gfx_actual_soft_max_freq :
					smu->gfx_default_soft_max_freq);
		}
		break;
	case SMU_OD_CCLK:
		if (smu_dpm_ctx->dpm_level == AMD_DPM_FORCED_LEVEL_MANUAL) {
			size = sprintf(buf, "CCLK_RANGE in Core%d:\n",
					smu->cpu_core_id_select);
			size += sprintf(buf + size, "0: %10uMhz\n",
			(smu->cpu_actual_soft_min_freq > 0) ?
					smu->cpu_actual_soft_min_freq :
					smu->cpu_default_soft_min_freq);
			size += sprintf(buf + size, "1: %10uMhz\n",
			(smu->cpu_actual_soft_max_freq > 0) ?
					smu->cpu_actual_soft_max_freq :
					smu->cpu_default_soft_max_freq);
		}
		break;
	case SMU_OD_RANGE:
		if (smu_dpm_ctx->dpm_level == AMD_DPM_FORCED_LEVEL_MANUAL) {
			size = sprintf(buf, "%s:\n", "OD_RANGE");
			size += sprintf(buf + size, "SCLK: %7uMhz %10uMhz\n",
				smu->gfx_default_hard_min_freq,
				smu->gfx_default_soft_max_freq);
			size += sprintf(buf + size, "CCLK: %7uMhz %10uMhz\n",
				smu->cpu_default_soft_min_freq,
				smu->cpu_default_soft_max_freq);
		}
		break;
	case SMU_SOCCLK:
		count = clk_table->NumSocClkLevelsEnabled;
		ret = mero_common_get_smu_metrics_data(smu,
				METRICS_AVERAGE_SOCCLK, &cur_value);
		break;
	case SMU_DCEFCLK:
	case SMU_DCLK:
		if (clk_type == SMU_DCEFCLK)
			clk_type = SMU_DCLK;
		count = clk_table->VcnClkLevelsEnabled;
		ret = mero_common_get_smu_metrics_data(smu,
				METRICS_AVERAGE_DCLK, &cur_value);
		break;
	case SMU_VCLK:
		count = clk_table->VcnClkLevelsEnabled;
		ret = mero_common_get_smu_metrics_data(smu,
				METRICS_AVERAGE_VCLK, &cur_value);
		break;
	case SMU_MCLK:
		count = clk_table->NumDfPstatesEnabled;
		ret = mero_common_get_smu_metrics_data(smu,
				METRICS_AVERAGE_UCLK, &cur_value);
		break;
	case SMU_FCLK:
		count = clk_table->NumDfPstatesEnabled;
		ret = smu_send_smc_msg_with_param(smu,
				SMU_MSG_GetFclkFrequency, 0, &cur_value);
		if (ret)
			return ret;
		break;
	default:
		break;
	}

	switch (clk_type) {
	case SMU_GFXCLK:
	case SMU_SCLK:
		/* retrieve table returned parameters unit is MHz */
		ret = mero_common_get_smu_metrics_data(smu,
				METRICS_AVERAGE_GFXCLK, &cur_value);

		if (cur_value == 0)
			cur_value = MERO_UMD_PSTATE_GFXCLK;

		pr_info(" %uMhz\n", cur_value);

		min = smu->gfx_actual_hard_min_freq > 0 ?
				smu->gfx_actual_hard_min_freq :
				smu->gfx_default_hard_min_freq;
		max = smu->gfx_actual_soft_max_freq > 0 ?
				smu->gfx_actual_soft_max_freq :
				smu->gfx_default_soft_max_freq;

		size += sprintf(buf + size, "0: %uMhz\n", min);
		size += sprintf(buf + size, "1: %uMhz *\n", cur_value);
		size += sprintf(buf + size, "2: %uMhz\n", max);
		break;

	case SMU_SOCCLK:
	case SMU_VCLK:
	case SMU_DCLK:
	case SMU_MCLK:
	case SMU_FCLK:
		for (i = 0; i < count; i++) {
			/* MCLK/FCLK DPM levels are in reverse order */
			if (clk_type == SMU_MCLK || clk_type == SMU_FCLK)
				ret = mero_get_dpm_clk_limited(smu,
					clk_type, ((count - 1) - i), &value);
			else
				ret = mero_get_dpm_clk_limited(smu,
					clk_type, i, &value);
			if (ret)
				return ret;
			if (!value)
				continue;
			size += sprintf(buf + size, "%d: %uMhz %s\n", i, value,
					cur_value == value ? "*" : "");
			if (cur_value == value)
				cur_value_match_level = true;
		}

		if (!cur_value_match_level)
			size += sprintf(buf + size, "   %uMhz *\n", cur_value);
		break;

	default:
		break;
	}

	return size;
}

static int mero_get_current_clk_freq_by_table(struct smu_context *smu,
		enum smu_clk_type clk_type,
		uint32_t *value)
{
	int ret;

	switch (clk_type) {
	case SMU_GFXCLK:
	case SMU_SCLK:
		ret = mero_common_get_smu_metrics_data(smu,
				METRICS_AVERAGE_GFXCLK, value);
		/* caller expect value in 10k unit, hence multiply by 100 */
		*value = *value * 100;
		break;

	case SMU_SOCCLK:
		ret = mero_common_get_smu_metrics_data(smu,
				METRICS_AVERAGE_SOCCLK, value);
		/* caller expect value in 10k unit, hence multiply by 100 */
		*value = *value * 100;
		break;

	case SMU_VCLK:
		ret = mero_common_get_smu_metrics_data(smu,
				METRICS_AVERAGE_VCLK, value);
		/* caller expect value in 10k unit, hence multiply by 100 */
		*value = *value * 100;
		break;

	case SMU_DCLK:
		ret = mero_common_get_smu_metrics_data(smu,
				METRICS_AVERAGE_DCLK, value);
		/* caller expect value in 10k unit, hence multiply by 100 */
		*value = *value * 100;
		break;

	case SMU_UCLK:
	case SMU_MCLK:
		ret = mero_common_get_smu_metrics_data(smu,
				METRICS_AVERAGE_UCLK, value);
		/* caller expect value in 10k unit, hence multiply by 100 */
		*value = *value * 100;
		break;

	default:
		pr_info("Invalid Clock Type!");
		ret = -EINVAL;
		break;
	}

	return ret;
}

static int mero_force_clk_levels(struct smu_context *smu,
				   enum smu_clk_type clk_type, uint32_t mask)
{
	uint32_t soft_min_level = 0, soft_max_level = 0;
	uint32_t min_freq = 0, max_freq = 0;
	int ret = 0;

	if (!mero_clk_dpm_is_enabled(smu, clk_type))
		return 0;

	soft_min_level = mask ? (ffs(mask) - 1) : 0;
	soft_max_level = mask ? (fls(mask) - 1) : 0;

	switch (clk_type) {
	case SMU_SOCCLK:
		ret = mero_get_dpm_clk_limited(smu, clk_type,
						soft_min_level, &min_freq);
		if (ret)
			return ret;
		ret = mero_get_dpm_clk_limited(smu, clk_type,
						soft_max_level, &max_freq);
		if (ret)
			return ret;
		ret = smu_send_smc_msg_with_param(smu,
				SMU_MSG_SetSoftMaxSocclkByFreq,
				max_freq, NULL);
		if (ret)
			return ret;
		ret = smu_send_smc_msg_with_param(smu,
				SMU_MSG_SetHardMinSocclkByFreq,
				min_freq, NULL);
		if (ret)
			return ret;
		break;
	case SMU_MCLK:
	case SMU_FCLK:
		ret = mero_get_dpm_clk_limited(smu,
							clk_type, soft_min_level, &min_freq);
		if (ret)
			return ret;
		ret = mero_get_dpm_clk_limited(smu,
							clk_type, soft_max_level, &max_freq);
		if (ret)
			return ret;
		ret = smu_send_smc_msg_with_param(smu,
						  SMU_MSG_SetSoftMaxFclkByFreq,
						  max_freq, NULL);
		if (ret)
			return ret;
		ret = smu_send_smc_msg_with_param(smu,
						  SMU_MSG_SetHardMinFclkByFreq,
						  min_freq, NULL);
		if (ret)
			return ret;
		break;
	case SMU_VCLK:
		ret = mero_get_dpm_clk_limited(smu,
							clk_type, soft_min_level, &min_freq);
		if (ret)
			return ret;
		ret = mero_get_dpm_clk_limited(smu,
							clk_type, soft_max_level, &max_freq);
		if (ret)
			return ret;
		ret = smu_send_smc_msg_with_param(smu, SMU_MSG_SetSoftMaxVcn,
						  max_freq << 16, NULL);
		if (ret)
			return ret;
		ret = smu_send_smc_msg_with_param(smu, SMU_MSG_SetHardMinVcn,
						  min_freq << 16, NULL);
		if (ret)
			return ret;
		break;
	case SMU_DCLK:
		ret = mero_get_dpm_clk_limited(smu,
							clk_type, soft_min_level, &min_freq);
		if (ret)
			return ret;
		ret = mero_get_dpm_clk_limited(smu,
							clk_type, soft_max_level, &max_freq);
		if (ret)
			return ret;
		ret = smu_send_smc_msg_with_param(smu, SMU_MSG_SetSoftMaxVcn,
						  max_freq, NULL);
		if (ret)
			return ret;
		ret = smu_send_smc_msg_with_param(smu, SMU_MSG_SetHardMinVcn,
						  min_freq, NULL);
		if (ret)
			return ret;
		break;
	default:
		break;
	}

	return 0;
}

static int mero_force_clk_levels_user(struct smu_context *smu,
		enum smu_clk_type clk_type,
		uint32_t user_mask)
{
	struct DpmClocks_t *clk_table = smu->smu_table.clocks_table;
	uint32_t soft_min_level = 0, soft_max_level = 0;
	uint32_t min_freq = 0, max_freq = 0;
	uint32_t mask = user_mask;
	int ret = 0, cnt;

	if (!mero_clk_dpm_is_enabled(smu, clk_type))
		return 0;

	if (clk_type == SMU_MCLK || clk_type == SMU_FCLK ||
			clk_type == SMU_UCLK) {
		/*
		 * clk levels are in the reverse order for
		 * the memory clocks, reverse the mask before
		 * we use it further
		 */
		mask = 0;
		for (cnt = 0; cnt < clk_table->NumDfPstatesEnabled; cnt++) {
			mask = mask << 1;
			if (user_mask & 0x1)
				mask = mask | 1;
			user_mask = user_mask >> 1;
		}
	}

	soft_min_level = mask ? (ffs(mask) - 1) : 0;
	soft_max_level = mask ? (fls(mask) - 1) : 0;

	ret = mero_get_dpm_clk_limited(smu, clk_type,
					soft_min_level, &min_freq);
	if (ret)
		return ret;

	ret = mero_get_dpm_clk_limited(smu, clk_type,
					soft_max_level, &max_freq);
	if (ret)
		return ret;

	ret = mero_set_soft_freq_limited_range(smu, clk_type,
			((max_freq > min_freq) ? min_freq : max_freq),
			((max_freq > min_freq) ? max_freq : min_freq));

	if (ret)
		return ret;

	/*
	 * save for restoring the user programmed
	 * frequency range in future
	 */
	mero_save_user_clk_info(smu, clk_type,
			min_freq, max_freq);

	return 0;
}

static int mero_clk_dpm_tuning_enable(struct smu_context *smu, enum smu_clk_type clk_type, uint32_t value)
{
	int ret = 0;

	if (!mero_clk_dpm_is_enabled(smu, clk_type))
		return 0;

	pr_debug("set clk %d dpm tuning enablement = %d\n", clk_type, value);
	switch (clk_type) {
	case SMU_FCLK:
		ret = smu_send_smc_msg_with_param(smu,
					SMU_MSG_SetOverrideActivityMonitor,
					value, NULL);
		if (ret)
			return ret;
		break;
	case SMU_LCLK:
		ret = smu_send_smc_msg_with_param(smu,
					SMU_MSG_CalculateLclkBusyfromIohcOnly,
					value, NULL);
		if (ret)
			return ret;
		break;
	default:
		return -EINVAL;
	}

	return ret;
}

static int mero_print_dpm_contants(struct smu_context *smu,
		enum smu_clk_type clk_type, char *data)
{
	int ret = 0, size = 0;
	uint32_t value = 0;

	if (clk_type >= SMU_CLK_COUNT)
		return -EINVAL;

	switch (clk_type) {
	case SMU_FCLK:
		ret = smu_send_smc_msg_with_param(smu,
				SMU_MSG_GetFclkDpmConstants, smu->fclk_actual_dpm_params, &value);
		size = sprintf(data, "%s:\n", "FCLK dpm constants");
		if (ret)
			return size;
		break;
	default:
		break;
	}

	pr_debug("Print FCLK DPM constants: 0x%x\n", value);
	size += sprintf(data + size, "0x%x\n", value);
	return size;
}

static int mero_set_dpm_contants(struct smu_context *smu,
		enum smu_clk_type clk_type,
		uint32_t mask)
{
	int ret = 0;

	if (!mero_clk_dpm_is_enabled(smu, clk_type))
		return 0;

	pr_debug("set clk %d dpm contants 0x%x\n", clk_type, mask);
	switch (clk_type) {
	case SMU_FCLK:
		ret = smu_send_smc_msg_with_param(smu,
					SMU_MSG_SetFclkDpmConstants,
					mask, NULL);
		if (ret)
			return ret;
		break;
	case SMU_LCLK:
		ret = smu_send_smc_msg_with_param(smu,
					SMU_MSG_UpdateLclkDpmConstants,
					mask, NULL);
		if (ret)
			return ret;
		break;
	default:
		return -EINVAL;
	}

	return ret;
}
static int mero_set_performance_level(struct smu_context *smu,
					enum amd_dpm_forced_level level)
{
	int ret = 0;
	uint32_t soc_mask = 0;
	uint32_t mclk_mask = 0;
	uint32_t fclk_mask = 0;
	uint32_t vclk_mask = 0;
	uint32_t dclk_mask = 0;
	uint32_t gpu_freq = 0;

	switch (level) {
	case AMD_DPM_FORCED_LEVEL_HIGH:
		ret = mero_force_dpm_limit_value(smu, true);
		break;
	case AMD_DPM_FORCED_LEVEL_LOW:
		ret = mero_force_dpm_limit_value(smu, false);
		break;
	case AMD_DPM_FORCED_LEVEL_AUTO:
		smu->gfx_actual_hard_min_freq = smu->gfx_default_hard_min_freq;
		smu->gfx_actual_soft_max_freq = smu->gfx_default_soft_max_freq;
		ret = mero_unforce_dpm_levels(smu, false);
		break;
	case AMD_DPM_FORCED_LEVEL_PROFILE_MIN_SCLK:
		break;
	case AMD_DPM_FORCED_LEVEL_PROFILE_MIN_MCLK:
		ret = mero_get_profiling_clk_mask(smu, level,
							NULL,
							NULL,
							&mclk_mask,
							&fclk_mask,
							&soc_mask);
		if (ret)
			return ret;
		mero_force_clk_levels(smu, SMU_MCLK, 1 << mclk_mask);
		mero_force_clk_levels(smu, SMU_FCLK, 1 << fclk_mask);
		mero_force_clk_levels(smu, SMU_SOCCLK, 1 << soc_mask);
		break;

	case AMD_DPM_FORCED_LEVEL_PROFILE_STANDARD:
	case AMD_DPM_FORCED_LEVEL_PROFILE_PEAK:
		smu->gfx_actual_hard_min_freq = smu->gfx_default_hard_min_freq;
		smu->gfx_actual_soft_max_freq = smu->gfx_default_soft_max_freq;

		if (mero_clk_dpm_is_enabled(smu, SMU_GFXCLK)) {

			if (level == AMD_DPM_FORCED_LEVEL_PROFILE_STANDARD)
				gpu_freq = MERO_UMD_PSTATE_STANDARD_GFXCLK;
			else
				gpu_freq = MERO_UMD_PSTATE_PEAK_GFXCLK;

			ret = smu_send_smc_msg_with_param(smu,
					SMU_MSG_SetHardMinGfxClk,
					gpu_freq, NULL);
			if (ret)
				return ret;

			ret = smu_send_smc_msg_with_param(smu,
					SMU_MSG_SetSoftMaxGfxClk,
					gpu_freq, NULL);
			if (ret)
				return ret;
		}

		ret = mero_get_profiling_clk_mask(smu, level,
							&vclk_mask,
							&dclk_mask,
							&mclk_mask,
							&fclk_mask,
							&soc_mask);

		mero_force_clk_levels(smu, SMU_MCLK, 1 << mclk_mask);
		mero_force_clk_levels(smu, SMU_FCLK, 1 << fclk_mask);
		mero_force_clk_levels(smu, SMU_SOCCLK, 1 << soc_mask);
		mero_force_clk_levels(smu, SMU_VCLK, 1 << vclk_mask);
		mero_force_clk_levels(smu, SMU_DCLK, 1 << dclk_mask);
		break;

	case AMD_DPM_FORCED_LEVEL_MANUAL:
		ret = mero_unforce_dpm_levels(smu, true);
		break;

	case AMD_DPM_FORCED_LEVEL_PROFILE_EXIT:
	default:
		break;
	}
	return ret;
}

static int mero_set_fine_grain_gfx_freq_parameters(struct smu_context *smu)
{
	/*
	 * Get min and max value from clock table
	 * struct DpmClocks_t *clk_table = smu->smu_table.clocks_table;
	 * clk_table->MinGfxClk;
	 * clk_table->MaxGfxClk;
	 */
	smu->gfx_default_hard_min_freq = 200;
	smu->gfx_default_soft_max_freq = MERO_UMD_PSTATE_PEAK_GFXCLK;
	smu->gfx_actual_hard_min_freq = 0;
	smu->gfx_actual_soft_max_freq = 0;

	smu->cpu_default_soft_min_freq = 1400;
	smu->cpu_default_soft_max_freq = 3500;
	smu->cpu_actual_soft_min_freq = 0;
	smu->cpu_actual_soft_max_freq = 0;

	return 0;
}

static int mero_od_edit_dpm_table(struct smu_context *smu,
		enum PP_OD_DPM_TABLE_COMMAND type,
		long input[], uint32_t size)
{
	int ret = 0;
	int i;
	struct smu_dpm_context *smu_dpm_ctx = &(smu->smu_dpm);

	if (!(smu_dpm_ctx->dpm_level == AMD_DPM_FORCED_LEVEL_MANUAL)) {
		dev_warn(smu->adev->dev, "Fine grain is not enabled!\n");
		return -EINVAL;
	}

	switch (type) {
	case PP_OD_EDIT_CCLK_VDDC_TABLE:
		if (size != 3) {
			dev_err(smu->adev->dev,
					"Input parameter number not correct (should be 4 for processor)\n");
			return -EINVAL;
		}
		if (input[0] >= smu->cpu_core_num) {
			dev_err(smu->adev->dev, "core index is overflow, should be less than %d\n",
				smu->cpu_core_num);
		}
		smu->cpu_core_id_select = input[0];
		if (input[1] == 0) {
			if (input[2] < smu->cpu_default_soft_min_freq) {
				dev_warn(smu->adev->dev,
					"Fine grain setting minimum cclk (%ld) MHz is less than the minimum allowed (%d) MHz\n",
					input[2],
					smu->cpu_default_soft_min_freq);
				return -EINVAL;
			}
			smu->cpu_actual_soft_min_freq = input[2];
		} else if (input[1] == 1) {
			if (input[2] > smu->cpu_default_soft_max_freq) {
				dev_warn(smu->adev->dev,
					"Fine grain setting maximum cclk (%ld) MHz is greater than the maximum allowed (%d) MHz\n",
					input[2],
					smu->cpu_default_soft_max_freq);
				return -EINVAL;
			}
			smu->cpu_actual_soft_max_freq = input[2];
		} else {
			return -EINVAL;
		}
		break;
	case PP_OD_EDIT_SCLK_VDDC_TABLE:
		if (size != 2) {
			dev_err(smu->adev->dev,
				"Input parameter number not correct\n");
			return -EINVAL;
		}

		if (input[0] == 0) {
			if (input[1] < smu->gfx_default_hard_min_freq) {
				dev_warn(smu->adev->dev,
					"Fine grain setting minimum sclk (%ld) MHz is less than the minimum allowed (%d) MHz\n",
					input[1],
					smu->gfx_default_hard_min_freq);
				return -EINVAL;
			}
			smu->gfx_actual_hard_min_freq = input[1];
		} else if (input[0] == 1) {
			if (input[1] > smu->gfx_default_soft_max_freq) {
				dev_warn(smu->adev->dev,
					"Fine grain setting maximum sclk (%ld) MHz is greater than the maximum allowed (%d) MHz\n",
					input[1],
					smu->gfx_default_soft_max_freq);
				return -EINVAL;
			}
			smu->gfx_actual_soft_max_freq = input[1];
		} else {
			return -EINVAL;
		}
		break;
	case PP_OD_RESTORE_DEFAULT_TABLE:
		if (size != 0) {
			dev_err(smu->adev->dev,
				"Input parameter number not correct\n");
			return -EINVAL;
		}
		smu->gfx_actual_hard_min_freq = smu->gfx_default_hard_min_freq;
		smu->gfx_actual_soft_max_freq = smu->gfx_default_soft_max_freq;
		smu->cpu_actual_soft_min_freq = smu->cpu_default_soft_min_freq;
		smu->cpu_actual_soft_max_freq = smu->cpu_default_soft_max_freq;

		ret = smu_send_smc_msg_with_param(smu,
				SMU_MSG_SetHardMinGfxClk,
				smu->gfx_actual_hard_min_freq, NULL);
		if (ret) {
			dev_err(smu->adev->dev,
				"Restore the default hard min sclk failed!");
			return ret;
		}

		ret = smu_send_smc_msg_with_param(smu,
				SMU_MSG_SetSoftMaxGfxClk,
				smu->gfx_actual_soft_max_freq, NULL);
		if (ret) {
			dev_err(smu->adev->dev,
					"Restore the default soft max sclk failed!");
			return ret;
		}

		mero_save_user_clk_info(smu,
				SMU_GFXCLK,
				smu->gfx_actual_hard_min_freq,
				smu->gfx_actual_soft_max_freq);

		if (smu->adev->pm.fw_version < 0x43f1b00) {
			dev_warn(smu->adev->dev,
				"CPUSoftMax/CPUSoftMin are not supported, please update SBIOS!\n");
			break;
		}

		for (i = 0; i < smu->cpu_core_num; i++) {
			ret = smu_send_smc_msg_with_param(
				smu, SMU_MSG_SetSoftMinCclk,
				(i << 20) | smu->cpu_actual_soft_min_freq,
				NULL);
			if (ret) {
				dev_err(smu->adev->dev,
					"Set hard min cclk failed!");
				return ret;
			}

			ret = smu_send_smc_msg_with_param(
				smu, SMU_MSG_SetSoftMaxCclk,
				(i << 20) | smu->cpu_actual_soft_max_freq,
				NULL);
			if (ret) {
				dev_err(smu->adev->dev,
					"Set soft max cclk failed!");
				return ret;
			}
		}
		break;
	case PP_OD_COMMIT_DPM_TABLE:
		if (size != 0) {
			dev_err(smu->adev->dev,
				"Input parameter number not correct\n");
			return -EINVAL;
		}
		if (smu->gfx_actual_hard_min_freq >
				smu->gfx_actual_soft_max_freq) {
			dev_err(smu->adev->dev,
				"The setting minimun sclk (%d) MHz is greater than the setting maximum sclk (%d) MHz\n",
				smu->gfx_actual_hard_min_freq,
				smu->gfx_actual_soft_max_freq);
			return -EINVAL;
		}

		ret = smu_send_smc_msg_with_param(smu,
			SMU_MSG_SetHardMinGfxClk,
			smu->gfx_actual_hard_min_freq, NULL);
		if (ret) {
			dev_err(smu->adev->dev,
				"Set hard min sclk failed!");
			return ret;
		}

		ret = smu_send_smc_msg_with_param(smu,
			SMU_MSG_SetSoftMaxGfxClk,
			smu->gfx_actual_soft_max_freq, NULL);
		if (ret) {
			dev_err(smu->adev->dev,
				"Set soft max sclk failed!");
			return ret;
		}

		mero_save_user_clk_info(smu,
				SMU_GFXCLK,
				smu->gfx_actual_hard_min_freq,
				smu->gfx_actual_soft_max_freq);

		if (smu->adev->pm.fw_version < 0x43f1b00) {
			dev_warn(smu->adev->dev, "CPUSoftMax/CPUSoftMin are not supported, please update SBIOS!\n");
			break;
		}

		ret = smu_send_smc_msg_with_param(smu,
			SMU_MSG_SetSoftMinCclk,
			((smu->cpu_core_id_select << 20)
			 | smu->cpu_actual_soft_min_freq), NULL);
		if (ret) {
			dev_err(smu->adev->dev,
				"Set hard min cclk failed!");
			return ret;
		}

		ret = smu_send_smc_msg_with_param(smu,
			SMU_MSG_SetSoftMaxCclk,
			((smu->cpu_core_id_select << 20)
			 | smu->cpu_actual_soft_max_freq), NULL);
		if (ret) {
			dev_err(smu->adev->dev,
				"Set soft max cclk failed!");
			return ret;
		}
		break;
	default:
		return -EINVAL;
	}

	return ret;
}

static int mero_set_default_dpm_tables(struct smu_context *smu)
{
	struct smu_table_context *smu_table = &smu->smu_table;
	int ret;

	ret = smu_update_table(smu, SMU_TABLE_DPMCLOCKS,
			0, smu_table->clocks_table, false);
	return ret;
}

static int mero_set_cclk_power_profile_mode(struct smu_context *smu,
				     enum smu_cclk_profile_t id)
{
	int ret = 0;
	enum smu_message_type msg = SMU_MSG_SetCclkPolicy;

	ret = smu_send_smc_msg_with_param(smu, msg, id, NULL);
	pr_info("cclk power profile ret = %d msg = %d, id =%d\n", ret, msg, id);
	return ret;
}

static int mero_set_df_state(struct smu_context *smu,
		      enum smu_df_state_t state,
		      enum smu_df_level_t level)
{
	int ret = 0;
	enum smu_message_type msg;

	msg = (state == SMU_DF_STATE_ACTIVE) ? SMU_MSG_SetDfPstateActiveLevel :
					     SMU_MSG_SetDfPstateSoftMinLevel;

	ret = smu_send_smc_msg_with_param(smu, msg, level, NULL);
	pr_info("set df state ret = %d msg = %d, id =%d\n", ret, msg, level);
	return ret;
}

int mero_get_enabled_mask(struct smu_context *smu,
			  u32 *feature_mask, u32 num)
{
	struct smu_feature *feature = &smu->smu_feature;
	uint32_t smu_feature_mask[2];
	int ret = 0;

	if (!feature_mask || num < 2)
		return -EINVAL;

	if (bitmap_empty(feature->enabled, feature->feature_num)) {
		ret = smu_send_smc_msg_with_param(smu,
						  SMU_MSG_GetEnabledSmuFeatures,
						  0, &smu_feature_mask[0]);
		if (ret)
			return ret;

		ret = smu_send_smc_msg_with_param(smu,
						  SMU_MSG_GetEnabledSmuFeatures,
						  1, &smu_feature_mask[1]);
		if (ret)
			return ret;

		bitmap_copy(feature->enabled,
				(unsigned long *)&smu_feature_mask,
			    feature->feature_num);
	}

	bitmap_copy((unsigned long *)feature_mask, feature->enabled,
			feature->feature_num);
	return ret;
}

static int mero_system_features_control(struct smu_context *smu, bool en)
{
	uint32_t smu_version;
	int ret = 0;

	ret = smu_get_smc_version(smu, NULL, &smu_version);
	if (ret)
		return ret;

	if (smu_version >= 0x43F1500 && !en) {
		/* The off message is supported after pmfw 4.63.21.0 */
		return smu_send_smc_msg_with_param(smu, SMU_MSG_RlcPowerNotify,
						   RLC_STATUS_OFF, NULL);
	}

	return ret;
}

static int mero_post_smu_init(struct smu_context *smu)
{
	struct amdgpu_device *adev = smu->adev;
	u32 tmp;
	u8 aon_bits = 0;
	/* Two CUs in one WGP */
	u32 req_active_wgps = adev->gfx.cu_info.number / 2;
	u32 total_cu = adev->gfx.config.max_cu_per_sh *
		adev->gfx.config.max_sh_per_se *
		adev->gfx.config.max_shader_engines;

	/* if all CUs are active, no need to power off any WGPs */
	if (total_cu == adev->gfx.cu_info.number)
		return 0;

	/*
	 * Calculate the total bits number of always on WGPs for all SA/SEs in
	 * RLC_PG_ALWAYS_ON_WGP_MASK.
	 */
	tmp = RREG32_KIQ(SOC15_REG_OFFSET(GC, 0, mmRLC_PG_ALWAYS_ON_WGP_MASK));
	tmp &= RLC_PG_ALWAYS_ON_WGP_MASK__AON_WGP_MASK_MASK;

	aon_bits = hweight32(tmp) * adev->gfx.config.max_sh_per_se *
		   adev->gfx.config.max_shader_engines;
	pr_debug("req_active_wgps =0x%x, aon_bits=0x%x\n",
		 req_active_wgps, aon_bits);
	/* Do not request any WGPs less than set in the AON_WGP_MASK */
	if (aon_bits > req_active_wgps) {
		dev_warn(adev->dev, "Invalid WGP configuration\n");
		return 0;
	} else {
		return smu_send_smc_msg_with_param(smu,
						   SMU_MSG_RequestActiveWgp,
						   req_active_wgps, NULL);
	}
}

static const struct pptable_funcs mero_ppt_funcs = {
	.tables_init = mero_tables_init,
	.get_smu_msg_index = mero_get_smu_msg_index,
	.get_smu_clk_index = mero_get_smu_clk_index,
	.get_smu_table_index = mero_get_smu_table_index,
	.check_fw_status = smu_v11_0_check_fw_status,
	.check_fw_version = smu_v11_0_check_fw_version,
	.init_smc_tables = smu_v11_0_init_smc_tables,
	.fini_smc_tables = smu_v11_0_fini_smc_tables,
	.init_power = smu_v11_0_init_power,
	.fini_power = smu_v11_0_fini_power,
	.register_irq_handler = smu_v11_0_register_irq_handler,
	.notify_memory_pool_location = smu_v11_0_notify_memory_pool_location,
	.send_smc_msg_with_param = smu_v11_0_send_msg_with_param,
	.alloc_dpm_context = mero_allocate_dpm_context,
	.get_smu_feature_index = mero_get_smu_feature_index,
	.get_workload_type = mero_get_workload_type,
	.dpm_set_uvd_enable = mero_dpm_set_uvd_enable,
	.dpm_set_jpeg_enable = mero_dpm_set_jpeg_enable,
	.get_power_limit = mero_get_power_limit,
	.set_power_limit = mero_set_power_limit,
	.get_thermal_limit = mero_get_thermal_limit,
	.set_thermal_limit = mero_set_thermal_limit,
	.set_time_limit = mero_set_time_limit,
	.is_dpm_running = mero_is_dpm_running,
	.read_sensor = mero_read_sensor,
	.print_clk_levels = mero_print_clk_levels,
	.get_workload_type = mero_get_workload_type,
	.get_current_power_state = mero_get_current_power_state,
	.get_power_profile_mode = mero_get_power_profile_mode,
	.set_watermarks_table = mero_set_watermarks_table,
	.set_driver_table_location = smu_v11_0_set_driver_table_location,
	.get_current_clk_freq_by_table = mero_get_current_clk_freq_by_table,
	.get_current_clk_freq = smu_v11_0_get_current_clk_freq,
	.read_smc_arg = smu_v11_0_read_arg,
	.set_cclk_power_profile_mode = mero_set_cclk_power_profile_mode,
	.set_df_state = mero_set_df_state,
	.get_dpm_ultimate_freq = mero_get_dpm_ultimate_freq,
	.set_soft_limit_min = mero_set_soft_limit_min,
	.set_soft_limit_max = mero_set_soft_limit_max,
	.gfx_off_control = smu_v11_0_gfx_off_control,
	.get_enabled_mask = mero_get_enabled_mask,
	.system_features_control = mero_system_features_control,
	.mode2_reset = smu_v11_0_mode2_reset,
	.populate_smc_tables = smu_v11_0_populate_smc_pptable,
	.set_default_dpm_table = mero_set_default_dpm_tables,
	.force_clk_levels = mero_force_clk_levels_user,
	.clk_dpm_tuning_enable = mero_clk_dpm_tuning_enable,
	.print_dpm_contants = mero_print_dpm_contants,
	.set_dpm_contants = mero_set_dpm_contants,
	.set_performance_level = mero_set_performance_level,
	.set_fine_grain_gfx_freq_parameters =
			mero_set_fine_grain_gfx_freq_parameters,
	.od_edit_dpm_table = mero_od_edit_dpm_table,
	.post_init = mero_post_smu_init,
	.get_gpu_metrics = mero_common_get_gpu_metrics,
	.unforce_clk_level = mero_unforce_dpm_level,
};

void *mero_get_smu_context(struct smu_context *smu)
{
	static void *context_handle;

	/* invalid value when smu and context_handle are both NULL*/
	if (!smu && !context_handle)
		return NULL;

	if (!context_handle)
		context_handle = (void *)smu;

	return context_handle;
}
EXPORT_SYMBOL(mero_get_smu_context);

void mero_set_ppt_funcs(struct smu_context *smu)
{
	smu->ppt_funcs = &mero_ppt_funcs;
	smu->is_apu = true;
	mero_get_smu_context(smu);
}

int smu_send_smc_msg_with_param(struct smu_context *smu,
					enum smu_message_type msg,
					uint32_t parameter,
					uint32_t *resp)
{
	int ret = 0;

	mutex_lock(&smu->message_lock);

	ret = smu_send_smc_msg_with_param_internal(smu, msg, parameter);
	if (ret) {
		mutex_unlock(&smu->message_lock);
		return ret;
	}

	if (resp)
		smu_read_smc_arg_internal(smu, resp);

	mutex_unlock(&smu->message_lock);

	return ret;
}

