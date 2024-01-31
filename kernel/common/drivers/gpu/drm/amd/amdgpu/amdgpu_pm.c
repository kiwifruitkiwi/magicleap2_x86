/*
 * Copyright (C) 2017-2023 Advanced Micro Devices, Inc. All rights Reserved.
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
 * Authors: Rafał Miłecki <zajec5@gmail.com>
 *          Alex Deucher <alexdeucher@gmail.com>
 */

#include <drm/drm_debugfs.h>

#include "amdgpu.h"
#include "amdgpu_drv.h"
#include "amdgpu_pm.h"
#include "amdgpu_dpm.h"
#include "amdgpu_display.h"
#include "amdgpu_smu.h"
#include "atom.h"
#include <linux/power_supply.h>
#include <linux/pci.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/nospec.h>
#include <linux/pm_runtime.h>
#include "hwmgr.h"

#ifdef CONFIG_ML_GSM
#include <linux/cvip/gsm.h>
#include <linux/cvip/gsm_cvip.h>
#include <linux/cvip/gsm_spinlock.h>

#define GSM_NOVA_LCLK_STATE (GSM_RESERVED_LCLK_STATE)
#define GSM_CVIP_LCLK_STATE (GSM_NOVA_LCLK_STATE + 4)

#define GSM_NOVA_FCLK_STATE (GSM_RESERVED_FCLK_STATE)
#define GSM_CVIP_FCLK_STATE (GSM_NOVA_FCLK_STATE + 4)

static struct gsm_spinlock_context lclk_spinlock;
static struct gsm_spinlock_context fclk_spinlock;
#endif

#define WIDTH_4K 3840

#define AMDCPU_DPM_CCLK_MASK 0xffff
#define AMDCPU_DPM_CORE_NUM_MASK 0xfff00000
#define AMDCPU_DPM_CORE_NUM_OFFSET 20

#define AMDGPU_DPM_MAX_SCLK_MERO 1600
#define AMDCPU_DPM_MAX_CCLK_MERO 2400
#define AMDCPU_DPM_MAX_CORE_NUM_MERO 3

#define GET_FCLK_DPM_PARAMS_SIZE 3
#define SET_FCLK_DPM_CONSTANTS_SIZE 4
#define SET_LCLK_DPM_CONSTANTS_SIZE 2
#define FCLK_DPM_AC 1
#define FCLK_DPM_DC 2
#define FCLK_DPM_TH_FLAG 1
#define FCLK_DPM_ALPHA_FLAG 0
#define FCLK_DPM_MAX_TH (100*1000)
#define FCLK_DPM_MAX_ALPHA (1*1000)
#define LCLK_DPM_MAX_TH (100*1000)

static int amdgpu_debugfs_pm_init(struct amdgpu_device *adev);

static const struct cg_flag_name clocks[] = {
	{AMD_CG_SUPPORT_GFX_MGCG, "Graphics Medium Grain Clock Gating"},
	{AMD_CG_SUPPORT_GFX_MGLS, "Graphics Medium Grain memory Light Sleep"},
	{AMD_CG_SUPPORT_GFX_CGCG, "Graphics Coarse Grain Clock Gating"},
	{AMD_CG_SUPPORT_GFX_CGLS, "Graphics Coarse Grain memory Light Sleep"},
	{AMD_CG_SUPPORT_GFX_CGTS, "Graphics Coarse Grain Tree Shader Clock Gating"},
	{AMD_CG_SUPPORT_GFX_CGTS_LS, "Graphics Coarse Grain Tree Shader Light Sleep"},
	{AMD_CG_SUPPORT_GFX_CP_LS, "Graphics Command Processor Light Sleep"},
	{AMD_CG_SUPPORT_GFX_RLC_LS, "Graphics Run List Controller Light Sleep"},
	{AMD_CG_SUPPORT_GFX_3D_CGCG, "Graphics 3D Coarse Grain Clock Gating"},
	{AMD_CG_SUPPORT_GFX_3D_CGLS, "Graphics 3D Coarse Grain memory Light Sleep"},
	{AMD_CG_SUPPORT_MC_LS, "Memory Controller Light Sleep"},
	{AMD_CG_SUPPORT_MC_MGCG, "Memory Controller Medium Grain Clock Gating"},
	{AMD_CG_SUPPORT_SDMA_LS, "System Direct Memory Access Light Sleep"},
	{AMD_CG_SUPPORT_SDMA_MGCG, "System Direct Memory Access Medium Grain Clock Gating"},
	{AMD_CG_SUPPORT_BIF_MGCG, "Bus Interface Medium Grain Clock Gating"},
	{AMD_CG_SUPPORT_BIF_LS, "Bus Interface Light Sleep"},
	{AMD_CG_SUPPORT_UVD_MGCG, "Unified Video Decoder Medium Grain Clock Gating"},
	{AMD_CG_SUPPORT_VCE_MGCG, "Video Compression Engine Medium Grain Clock Gating"},
	{AMD_CG_SUPPORT_HDP_LS, "Host Data Path Light Sleep"},
	{AMD_CG_SUPPORT_HDP_MGCG, "Host Data Path Medium Grain Clock Gating"},
	{AMD_CG_SUPPORT_DRM_MGCG, "Digital Right Management Medium Grain Clock Gating"},
	{AMD_CG_SUPPORT_DRM_LS, "Digital Right Management Light Sleep"},
	{AMD_CG_SUPPORT_ROM_MGCG, "Rom Medium Grain Clock Gating"},
	{AMD_CG_SUPPORT_DF_MGCG, "Data Fabric Medium Grain Clock Gating"},

	{AMD_CG_SUPPORT_ATHUB_MGCG, "Address Translation Hub Medium Grain Clock Gating"},
	{AMD_CG_SUPPORT_ATHUB_LS, "Address Translation Hub Light Sleep"},
	{0, NULL},
};

static const struct hwmon_temp_label {
	enum PP_HWMON_TEMP channel;
	const char *label;
} temp_label[] = {
	{PP_TEMP_EDGE, "edge"},
	{PP_TEMP_JUNCTION, "junction"},
	{PP_TEMP_MEM, "mem"},
};

enum res_id {
	VOL_CPU0,
	VOL_CPU1,
	VOL_CPU2,
	VOL_CPU3,
	VOL_GFX,
	VOL_SOC,
	VOL_CVIP,
	CUR_CPU,
	CUR_GFX,
	CUR_SOC,
	CUR_CVIP,
	POWER_CPU,
	POWER_GFX,
	POWER_SOC,
	POWER_CVIP,
	TEM_CPU0,
	TEM_CPU1,
	TEM_CPU2,
	TEM_CPU3,
	TEM_GFX,
	TEM_SOC,
	TEM_CVIP,
	POWER_TIME,
	TEM_TIME,
	ACT_TIME,
	MIG_END_HYS,
	CLK_CPU0,
	CLK_CPU1,
	CLK_CPU2,
	CLK_CPU3,
	CLK_GFX,
	ACT_CPU,
	ACT_GFX,
	RES_NUM,
};

enum value_t {
	CUR_VAL,
	MIN_VAL,
	MAX_VAL,
	AVE_VAL,
	LIMT_VAL,
	TYPE_NUM,
};

void amdgpu_pm_acpi_event_handler(struct amdgpu_device *adev)
{
	if (adev->pm.dpm_enabled) {
		mutex_lock(&adev->pm.mutex);
		if (power_supply_is_system_supplied() > 0)
			adev->pm.ac_power = true;
		else
			adev->pm.ac_power = false;
		if (adev->powerplay.pp_funcs->enable_bapm)
			amdgpu_dpm_enable_bapm(adev, adev->pm.ac_power);
		mutex_unlock(&adev->pm.mutex);
	}
}

int amdgpu_dpm_read_sensor(struct amdgpu_device *adev, enum amd_pp_sensors sensor,
			   void *data, uint32_t *size)
{
	int ret = 0;

	if (!data || !size)
		return -EINVAL;

	if (is_support_sw_smu(adev))
		ret = smu_read_sensor(&adev->smu, sensor, data, size);
	else {
		if (adev->powerplay.pp_funcs && adev->powerplay.pp_funcs->read_sensor)
			ret = adev->powerplay.pp_funcs->read_sensor((adev)->powerplay.pp_handle,
								    sensor, data, size);
		else
			ret = -EINVAL;
	}

	return ret;
}

/**
 * DOC: power_dpm_state
 *
 * The power_dpm_state file is a legacy interface and is only provided for
 * backwards compatibility. The amdgpu driver provides a sysfs API for adjusting
 * certain power related parameters.  The file power_dpm_state is used for this.
 * It accepts the following arguments:
 *
 * - battery
 *
 * - balanced
 *
 * - performance
 *
 * battery
 *
 * On older GPUs, the vbios provided a special power state for battery
 * operation.  Selecting battery switched to this state.  This is no
 * longer provided on newer GPUs so the option does nothing in that case.
 *
 * balanced
 *
 * On older GPUs, the vbios provided a special power state for balanced
 * operation.  Selecting balanced switched to this state.  This is no
 * longer provided on newer GPUs so the option does nothing in that case.
 *
 * performance
 *
 * On older GPUs, the vbios provided a special power state for performance
 * operation.  Selecting performance switched to this state.  This is no
 * longer provided on newer GPUs so the option does nothing in that case.
 *
 */

static ssize_t amdgpu_get_dpm_state(struct device *dev,
				    struct device_attribute *attr,
				    char *buf)
{
	struct drm_device *ddev = dev_get_drvdata(dev);
	struct amdgpu_device *adev = ddev->dev_private;
	enum amd_pm_state_type pm;
	int ret;

	if (amdgpu_sriov_vf(adev) && !amdgpu_sriov_is_pp_one_vf(adev))
		return 0;

	ret = pm_runtime_get_sync(ddev->dev);
	if (ret < 0)
		return ret;

	if (is_support_sw_smu(adev)) {
		if (adev->smu.ppt_funcs->get_current_power_state)
			pm = smu_get_current_power_state(&adev->smu);
		else
			pm = adev->pm.dpm.user_state;
	} else if (adev->powerplay.pp_funcs->get_current_power_state) {
		pm = amdgpu_dpm_get_current_power_state(adev);
	} else {
		pm = adev->pm.dpm.user_state;
	}

	pm_runtime_mark_last_busy(ddev->dev);
	pm_runtime_put_autosuspend(ddev->dev);

	return snprintf(buf, PAGE_SIZE, "%s\n",
			(pm == POWER_STATE_TYPE_BATTERY) ? "battery" :
			(pm == POWER_STATE_TYPE_BALANCED) ? "balanced" : "performance");
}

static ssize_t amdgpu_set_dpm_state(struct device *dev,
				    struct device_attribute *attr,
				    const char *buf,
				    size_t count)
{
	struct drm_device *ddev = dev_get_drvdata(dev);
	struct amdgpu_device *adev = ddev->dev_private;
	enum amd_pm_state_type  state;
	int ret;

	if (amdgpu_sriov_vf(adev) && !amdgpu_sriov_is_pp_one_vf(adev))
		return -EINVAL;

	if (strncmp("battery", buf, strlen("battery")) == 0)
		state = POWER_STATE_TYPE_BATTERY;
	else if (strncmp("balanced", buf, strlen("balanced")) == 0)
		state = POWER_STATE_TYPE_BALANCED;
	else if (strncmp("performance", buf, strlen("performance")) == 0)
		state = POWER_STATE_TYPE_PERFORMANCE;
	else
		return -EINVAL;

	ret = pm_runtime_get_sync(ddev->dev);
	if (ret < 0)
		return ret;

	if (is_support_sw_smu(adev)) {
		mutex_lock(&adev->pm.mutex);
		adev->pm.dpm.user_state = state;
		mutex_unlock(&adev->pm.mutex);
	} else if (adev->powerplay.pp_funcs->dispatch_tasks) {
		amdgpu_dpm_dispatch_task(adev, AMD_PP_TASK_ENABLE_USER_STATE, &state);
	} else {
		mutex_lock(&adev->pm.mutex);
		adev->pm.dpm.user_state = state;
		mutex_unlock(&adev->pm.mutex);

		amdgpu_pm_compute_clocks(adev);
	}
	pm_runtime_mark_last_busy(ddev->dev);
	pm_runtime_put_autosuspend(ddev->dev);

	return count;
}


/**
 * DOC: power_dpm_force_performance_level
 *
 * The amdgpu driver provides a sysfs API for adjusting certain power
 * related parameters.  The file power_dpm_force_performance_level is
 * used for this.  It accepts the following arguments:
 *
 * - auto
 *
 * - low
 *
 * - high
 *
 * - manual
 *
 * - profile_standard
 *
 * - profile_min_sclk
 *
 * - profile_min_mclk
 *
 * - profile_peak
 *
 * auto
 *
 * When auto is selected, the driver will attempt to dynamically select
 * the optimal power profile for current conditions in the driver.
 *
 * low
 *
 * When low is selected, the clocks are forced to the lowest power state.
 *
 * high
 *
 * When high is selected, the clocks are forced to the highest power state.
 *
 * manual
 *
 * When manual is selected, the user can manually adjust which power states
 * are enabled for each clock domain via the sysfs pp_dpm_mclk, pp_dpm_sclk,
 * and pp_dpm_pcie files and adjust the power state transition heuristics
 * via the pp_power_profile_mode sysfs file.
 *
 * profile_standard
 * profile_min_sclk
 * profile_min_mclk
 * profile_peak
 *
 * When the profiling modes are selected, clock and power gating are
 * disabled and the clocks are set for different profiling cases. This
 * mode is recommended for profiling specific work loads where you do
 * not want clock or power gating for clock fluctuation to interfere
 * with your results. profile_standard sets the clocks to a fixed clock
 * level which varies from asic to asic.  profile_min_sclk forces the sclk
 * to the lowest level.  profile_min_mclk forces the mclk to the lowest level.
 * profile_peak sets all clocks (mclk, sclk, pcie) to the highest levels.
 *
 */

static ssize_t amdgpu_get_dpm_forced_performance_level(struct device *dev,
						struct device_attribute *attr,
								char *buf)
{
	struct drm_device *ddev = dev_get_drvdata(dev);
	struct amdgpu_device *adev = ddev->dev_private;
	enum amd_dpm_forced_level level = 0xff;
	int ret;

	if (amdgpu_sriov_vf(adev) && !amdgpu_sriov_is_pp_one_vf(adev))
		return 0;

	ret = pm_runtime_get_sync(ddev->dev);
	if (ret < 0)
		return ret;

	if (is_support_sw_smu(adev))
		level = smu_get_performance_level(&adev->smu);
	else if (adev->powerplay.pp_funcs->get_performance_level)
		level = amdgpu_dpm_get_performance_level(adev);
	else
		level = adev->pm.dpm.forced_level;

	pm_runtime_mark_last_busy(ddev->dev);
	pm_runtime_put_autosuspend(ddev->dev);

	return snprintf(buf, PAGE_SIZE, "%s\n",
			(level == AMD_DPM_FORCED_LEVEL_AUTO) ? "auto" :
			(level == AMD_DPM_FORCED_LEVEL_LOW) ? "low" :
			(level == AMD_DPM_FORCED_LEVEL_HIGH) ? "high" :
			(level == AMD_DPM_FORCED_LEVEL_MANUAL) ? "manual" :
			(level == AMD_DPM_FORCED_LEVEL_PROFILE_STANDARD) ? "profile_standard" :
			(level == AMD_DPM_FORCED_LEVEL_PROFILE_MIN_SCLK) ? "profile_min_sclk" :
			(level == AMD_DPM_FORCED_LEVEL_PROFILE_MIN_MCLK) ? "profile_min_mclk" :
			(level == AMD_DPM_FORCED_LEVEL_PROFILE_PEAK) ? "profile_peak" :
			"unknown");
}

static ssize_t amdgpu_set_dpm_forced_performance_level(struct device *dev,
						       struct device_attribute *attr,
						       const char *buf,
						       size_t count)
{
	struct drm_device *ddev = dev_get_drvdata(dev);
	struct amdgpu_device *adev = ddev->dev_private;
	enum amd_dpm_forced_level level;
	int ret = 0;

	if (amdgpu_sriov_vf(adev) && !amdgpu_sriov_is_pp_one_vf(adev))
		return -EINVAL;

	if (strncmp("low", buf, strlen("low")) == 0) {
		level = AMD_DPM_FORCED_LEVEL_LOW;
	} else if (strncmp("high", buf, strlen("high")) == 0) {
		level = AMD_DPM_FORCED_LEVEL_HIGH;
	} else if (strncmp("auto", buf, strlen("auto")) == 0) {
		level = AMD_DPM_FORCED_LEVEL_AUTO;
	} else if (strncmp("manual", buf, strlen("manual")) == 0) {
		level = AMD_DPM_FORCED_LEVEL_MANUAL;
	} else if (strncmp("profile_exit", buf, strlen("profile_exit")) == 0) {
		level = AMD_DPM_FORCED_LEVEL_PROFILE_EXIT;
	} else if (strncmp("profile_standard", buf, strlen("profile_standard")) == 0) {
		level = AMD_DPM_FORCED_LEVEL_PROFILE_STANDARD;
	} else if (strncmp("profile_min_sclk", buf, strlen("profile_min_sclk")) == 0) {
		level = AMD_DPM_FORCED_LEVEL_PROFILE_MIN_SCLK;
	} else if (strncmp("profile_min_mclk", buf, strlen("profile_min_mclk")) == 0) {
		level = AMD_DPM_FORCED_LEVEL_PROFILE_MIN_MCLK;
	} else if (strncmp("profile_peak", buf, strlen("profile_peak")) == 0) {
		level = AMD_DPM_FORCED_LEVEL_PROFILE_PEAK;
	}  else {
		return -EINVAL;
	}

	ret = pm_runtime_get_sync(ddev->dev);
	if (ret < 0)
		return ret;

	if (is_support_sw_smu(adev)) {
		mutex_lock(&adev->pm.stable_pstate_ctx_lock);
		ret = smu_force_performance_level(&adev->smu, level);
		if (ret) {
			pm_runtime_mark_last_busy(ddev->dev);
			pm_runtime_put_autosuspend(ddev->dev);
			mutex_unlock(&adev->pm.stable_pstate_ctx_lock);
			return -EINVAL;
		}

		/* override whatever a user ctx may have set */
		adev->pm.stable_pstate_ctx = NULL;
		mutex_unlock(&adev->pm.stable_pstate_ctx_lock);
	} else if (adev->powerplay.pp_funcs->force_performance_level) {
		mutex_lock(&adev->pm.stable_pstate_ctx_lock);
		if (adev->pm.dpm.thermal_active) {
			pm_runtime_mark_last_busy(ddev->dev);
			pm_runtime_put_autosuspend(ddev->dev);
			mutex_unlock(&adev->pm.stable_pstate_ctx_lock);
			return -EINVAL;
		}
		ret = amdgpu_dpm_force_performance_level(adev, level);
		if (ret) {
			pm_runtime_mark_last_busy(ddev->dev);
			pm_runtime_put_autosuspend(ddev->dev);
			mutex_unlock(&adev->pm.stable_pstate_ctx_lock);
			return -EINVAL;
		} else {
			adev->pm.dpm.forced_level = level;
		}
		/* override whatever a user ctx may have set */
		adev->pm.stable_pstate_ctx = NULL;
		mutex_unlock(&adev->pm.stable_pstate_ctx_lock);
	}
	pm_runtime_mark_last_busy(ddev->dev);
	pm_runtime_put_autosuspend(ddev->dev);

	return count;
}

static ssize_t amdgpu_get_pp_num_states(struct device *dev,
		struct device_attribute *attr,
		char *buf)
{
	struct drm_device *ddev = dev_get_drvdata(dev);
	struct amdgpu_device *adev = ddev->dev_private;
	struct pp_states_info data;
	int i, buf_len, ret;

	memset(&data, 0, sizeof(struct pp_states_info));

	ret = pm_runtime_get_sync(ddev->dev);
	if (ret < 0)
		return ret;

	if (is_support_sw_smu(adev)) {
		ret = smu_get_power_num_states(&adev->smu, &data);
		if (ret)
			return ret;
	} else if (adev->powerplay.pp_funcs->get_pp_num_states)
		amdgpu_dpm_get_pp_num_states(adev, &data);

	pm_runtime_mark_last_busy(ddev->dev);
	pm_runtime_put_autosuspend(ddev->dev);

	buf_len = snprintf(buf, PAGE_SIZE, "states: %d\n", data.nums);
	for (i = 0; i < data.nums; i++)
		buf_len += snprintf(buf + buf_len, PAGE_SIZE, "%d %s\n", i,
				(data.states[i] == POWER_STATE_TYPE_INTERNAL_BOOT) ? "boot" :
				(data.states[i] == POWER_STATE_TYPE_BATTERY) ? "battery" :
				(data.states[i] == POWER_STATE_TYPE_BALANCED) ? "balanced" :
				(data.states[i] == POWER_STATE_TYPE_PERFORMANCE) ? "performance" : "default");

	return buf_len;
}

static ssize_t amdgpu_get_pp_cur_state(struct device *dev,
		struct device_attribute *attr,
		char *buf)
{
	struct drm_device *ddev = dev_get_drvdata(dev);
	struct amdgpu_device *adev = ddev->dev_private;
	struct pp_states_info data;
	struct smu_context *smu = &adev->smu;
	enum amd_pm_state_type pm = 0;
	int i = 0, ret = 0;

	memset(&data, 0, sizeof(struct pp_states_info));
	if (amdgpu_sriov_vf(adev) && !amdgpu_sriov_is_pp_one_vf(adev))
		return 0;

	ret = pm_runtime_get_sync(ddev->dev);
	if (ret < 0)
		return ret;

	if (is_support_sw_smu(adev)) {
		pm = smu_get_current_power_state(smu);
		ret = smu_get_power_num_states(smu, &data);
		if (ret)
			return ret;
	} else if (adev->powerplay.pp_funcs->get_current_power_state
		 && adev->powerplay.pp_funcs->get_pp_num_states) {
		pm = amdgpu_dpm_get_current_power_state(adev);
		amdgpu_dpm_get_pp_num_states(adev, &data);
	}

	pm_runtime_mark_last_busy(ddev->dev);
	pm_runtime_put_autosuspend(ddev->dev);

	for (i = 0; i < data.nums; i++) {
		if (pm == data.states[i])
			break;
	}

	if (i == data.nums)
		i = -EINVAL;

	return snprintf(buf, PAGE_SIZE, "%d\n", i);
}

static ssize_t amdgpu_get_pp_force_state(struct device *dev,
		struct device_attribute *attr,
		char *buf)
{
	struct drm_device *ddev = dev_get_drvdata(dev);
	struct amdgpu_device *adev = ddev->dev_private;

	if (amdgpu_sriov_vf(adev) && !amdgpu_sriov_is_pp_one_vf(adev))
		return 0;

	if (adev->pp_force_state_enabled)
		return amdgpu_get_pp_cur_state(dev, attr, buf);
	else
		return snprintf(buf, PAGE_SIZE, "\n");
}

static ssize_t amdgpu_set_pp_force_state(struct device *dev,
		struct device_attribute *attr,
		const char *buf,
		size_t count)
{
	struct drm_device *ddev = dev_get_drvdata(dev);
	struct amdgpu_device *adev = ddev->dev_private;
	enum amd_pm_state_type state = 0;
	unsigned long idx;
	int ret;

	if (amdgpu_sriov_vf(adev) && !amdgpu_sriov_is_pp_one_vf(adev))
		return -EINVAL;

	if (strlen(buf) == 1)
		adev->pp_force_state_enabled = false;
	else if (is_support_sw_smu(adev))
		adev->pp_force_state_enabled = false;
	else if (adev->powerplay.pp_funcs->dispatch_tasks &&
			adev->powerplay.pp_funcs->get_pp_num_states) {
		struct pp_states_info data;

		ret = kstrtoul(buf, 0, &idx);
		if (ret || idx >= ARRAY_SIZE(data.states))
			return -EINVAL;

		idx = array_index_nospec(idx, ARRAY_SIZE(data.states));

		amdgpu_dpm_get_pp_num_states(adev, &data);
		state = data.states[idx];

		ret = pm_runtime_get_sync(ddev->dev);
		if (ret < 0)
			return ret;

		/* only set user selected power states */
		if (state != POWER_STATE_TYPE_INTERNAL_BOOT &&
		    state != POWER_STATE_TYPE_DEFAULT) {
			amdgpu_dpm_dispatch_task(adev,
					AMD_PP_TASK_ENABLE_USER_STATE, &state);
			adev->pp_force_state_enabled = true;
		}
		pm_runtime_mark_last_busy(ddev->dev);
		pm_runtime_put_autosuspend(ddev->dev);
	}

	return count;
}

/**
 * DOC: pp_table
 *
 * The amdgpu driver provides a sysfs API for uploading new powerplay
 * tables.  The file pp_table is used for this.  Reading the file
 * will dump the current power play table.  Writing to the file
 * will attempt to upload a new powerplay table and re-initialize
 * powerplay using that new table.
 *
 */

static ssize_t amdgpu_get_pp_table(struct device *dev,
		struct device_attribute *attr,
		char *buf)
{
	struct drm_device *ddev = dev_get_drvdata(dev);
	struct amdgpu_device *adev = ddev->dev_private;
	char *table = NULL;
	int size, ret;

	if (amdgpu_sriov_vf(adev) && !amdgpu_sriov_is_pp_one_vf(adev))
		return 0;

	ret = pm_runtime_get_sync(ddev->dev);
	if (ret < 0)
		return ret;

	if (is_support_sw_smu(adev)) {
		size = smu_sys_get_pp_table(&adev->smu, (void **)&table);
		pm_runtime_mark_last_busy(ddev->dev);
		pm_runtime_put_autosuspend(ddev->dev);
		if (size < 0)
			return size;
	} else if (adev->powerplay.pp_funcs->get_pp_table) {
		size = amdgpu_dpm_get_pp_table(adev, &table);
		pm_runtime_mark_last_busy(ddev->dev);
		pm_runtime_put_autosuspend(ddev->dev);
		if (size < 0)
			return size;
	} else {
		pm_runtime_mark_last_busy(ddev->dev);
		pm_runtime_put_autosuspend(ddev->dev);
		return 0;
	}

	if (size >= PAGE_SIZE)
		size = PAGE_SIZE - 1;

	memcpy(buf, table, size);

	return size;
}

static ssize_t amdgpu_set_pp_table(struct device *dev,
		struct device_attribute *attr,
		const char *buf,
		size_t count)
{
	struct drm_device *ddev = dev_get_drvdata(dev);
	struct amdgpu_device *adev = ddev->dev_private;
	int ret = 0;

	if (amdgpu_sriov_vf(adev) && !amdgpu_sriov_is_pp_one_vf(adev))
		return -EINVAL;

	ret = pm_runtime_get_sync(ddev->dev);
	if (ret < 0)
		return ret;

	if (is_support_sw_smu(adev)) {
		ret = smu_sys_set_pp_table(&adev->smu, (void *)buf, count);
		if (ret) {
			pm_runtime_mark_last_busy(ddev->dev);
			pm_runtime_put_autosuspend(ddev->dev);
			return ret;
		}
	} else if (adev->powerplay.pp_funcs->set_pp_table)
		amdgpu_dpm_set_pp_table(adev, buf, count);

	pm_runtime_mark_last_busy(ddev->dev);
	pm_runtime_put_autosuspend(ddev->dev);

	return count;
}

/**
 * DOC: pp_od_clk_voltage
 *
 * The amdgpu driver provides a sysfs API for adjusting the clocks and voltages
 * in each power level within a power state.  The pp_od_clk_voltage is used for
 * this.
 *
 * < For Vega10 and previous ASICs >
 *
 * Reading the file will display:
 *
 * - a list of engine clock levels and voltages labeled OD_SCLK
 *
 * - a list of memory clock levels and voltages labeled OD_MCLK
 *
 * - a list of valid ranges for sclk, mclk, and voltage labeled OD_RANGE
 *
 * To manually adjust these settings, first select manual using
 * power_dpm_force_performance_level. Enter a new value for each
 * level by writing a string that contains "s/m level clock voltage" to
 * the file.  E.g., "s 1 500 820" will update sclk level 1 to be 500 MHz
 * at 820 mV; "m 0 350 810" will update mclk level 0 to be 350 MHz at
 * 810 mV.  When you have edited all of the states as needed, write
 * "c" (commit) to the file to commit your changes.  If you want to reset to the
 * default power levels, write "r" (reset) to the file to reset them.
 *
 *
 * < For Vega20 >
 *
 * Reading the file will display:
 *
 * - minimum and maximum engine clock labeled OD_SCLK
 *
 * - maximum memory clock labeled OD_MCLK
 *
 * - three <frequency, voltage> points labeled OD_VDDC_CURVE.
 *   They can be used to calibrate the sclk voltage curve.
 *
 * - a list of valid ranges for sclk, mclk, and voltage curve points
 *   labeled OD_RANGE
 *
 * To manually adjust these settings:
 *
 * - First select manual using power_dpm_force_performance_level
 *
 * - For clock frequency setting, enter a new value by writing a
 *   string that contains "s/m index clock" to the file. The index
 *   should be 0 if to set minimum clock. And 1 if to set maximum
 *   clock. E.g., "s 0 500" will update minimum sclk to be 500 MHz.
 *   "m 1 800" will update maximum mclk to be 800Mhz. For CPU use
 *   string that contains "p core index clock" to the file. core
 *   indicates the CPU core.
 *
 *   For sclk voltage curve, enter the new values by writing a
 *   string that contains "vc point clock voltage" to the file. The
 *   points are indexed by 0, 1 and 2. E.g., "vc 0 300 600" will
 *   update point1 with clock set as 300Mhz and voltage as
 *   600mV. "vc 2 1000 1000" will update point3 with clock set
 *   as 1000Mhz and voltage 1000mV.
 *
 * - When you have edited all of the states as needed, write "c" (commit)
 *   to the file to commit your changes
 *
 * - If you want to reset to the default power levels, write "r" (reset)
 *   to the file to reset them
 *
 */

static ssize_t amdgpu_set_pp_od_clk_voltage(struct device *dev,
		struct device_attribute *attr,
		const char *buf,
		size_t count)
{
	struct drm_device *ddev = dev_get_drvdata(dev);
	struct amdgpu_device *adev = ddev->dev_private;
	int ret;
	uint32_t parameter_size = 0;
	long parameter[64];
	char buf_cpy[128];
	char *tmp_str;
	char *sub_str;
	const char delimiter[3] = {' ', '\n', '\0'};
	uint32_t type;

	if (amdgpu_sriov_vf(adev))
		return -EINVAL;

	if (count > 127)
		return -EINVAL;

	if (*buf == 's')
		type = PP_OD_EDIT_SCLK_VDDC_TABLE;
	else if (*buf == 'p')
		type = PP_OD_EDIT_CCLK_VDDC_TABLE;
	else if (*buf == 'm')
		type = PP_OD_EDIT_MCLK_VDDC_TABLE;
	else if(*buf == 'r')
		type = PP_OD_RESTORE_DEFAULT_TABLE;
	else if (*buf == 'c')
		type = PP_OD_COMMIT_DPM_TABLE;
	else if (!strncmp(buf, "vc", 2))
		type = PP_OD_EDIT_VDDC_CURVE;
	else if (!strncmp(buf, "vo", 2))
		type = PP_OD_EDIT_VDDGFX_OFFSET;
	else
		return -EINVAL;

	memcpy(buf_cpy, buf, count+1);

	tmp_str = buf_cpy;

	if ((type == PP_OD_EDIT_VDDC_CURVE) ||
	     (type == PP_OD_EDIT_VDDGFX_OFFSET))
		tmp_str++;
	while (isspace(*++tmp_str));

	while ((sub_str = strsep(&tmp_str, delimiter)) != NULL) {
		if (strlen(sub_str) == 0)
			continue;
		ret = kstrtol(sub_str, 0, &parameter[parameter_size]);
		if (ret)
			return -EINVAL;
		parameter_size++;

		while (isspace(*tmp_str))
			tmp_str++;
	}

	ret = pm_runtime_get_sync(ddev->dev);
	if (ret < 0) {
		pm_runtime_put_autosuspend(ddev->dev);
		return ret;
	}

	if (is_support_sw_smu(adev)) {
		ret = smu_od_edit_dpm_table(&adev->smu, type,
					    parameter, parameter_size);

		if (ret) {
			pm_runtime_mark_last_busy(ddev->dev);
			pm_runtime_put_autosuspend(ddev->dev);
			return -EINVAL;
		}
	} else {
		if (adev->powerplay.pp_funcs->odn_edit_dpm_table) {
			ret = amdgpu_dpm_odn_edit_dpm_table(adev, type,
						parameter, parameter_size);
			if (ret) {
				pm_runtime_mark_last_busy(ddev->dev);
				pm_runtime_put_autosuspend(ddev->dev);
				return -EINVAL;
			}
		}

		if (type == PP_OD_COMMIT_DPM_TABLE) {
			if (adev->powerplay.pp_funcs->dispatch_tasks) {
				amdgpu_dpm_dispatch_task(adev,
						AMD_PP_TASK_READJUST_POWER_STATE,
						NULL);
				pm_runtime_mark_last_busy(ddev->dev);
				pm_runtime_put_autosuspend(ddev->dev);
				return count;
			} else {
				pm_runtime_mark_last_busy(ddev->dev);
				pm_runtime_put_autosuspend(ddev->dev);
				return -EINVAL;
			}
		}
	}
	pm_runtime_mark_last_busy(ddev->dev);
	pm_runtime_put_autosuspend(ddev->dev);

	return count;
}

static ssize_t amdgpu_get_pp_od_clk_voltage(struct device *dev,
		struct device_attribute *attr,
		char *buf)
{
	struct drm_device *ddev = dev_get_drvdata(dev);
	struct amdgpu_device *adev = ddev->dev_private;
	ssize_t size;
	int ret;

	if (amdgpu_sriov_vf(adev))
		return 0;

	ret = pm_runtime_get_sync(ddev->dev);
	if (ret < 0)
		return ret;

	if (is_support_sw_smu(adev)) {
		size = smu_print_clk_levels(
				&adev->smu, SMU_OD_SCLK, buf);
		size += smu_print_clk_levels(
				&adev->smu, SMU_OD_MCLK, buf+size);
		size += smu_print_clk_levels(
				&adev->smu, SMU_OD_VDDC_CURVE, buf+size);
		size += smu_print_clk_levels(
				&adev->smu, SMU_OD_VDDGFX_OFFSET, buf+size);
		size += smu_print_clk_levels(
				&adev->smu, SMU_OD_RANGE, buf+size);
		size += smu_print_clk_levels(
				&adev->smu, SMU_OD_CCLK, buf+size);
	} else if (adev->powerplay.pp_funcs->print_clock_levels) {
		size = amdgpu_dpm_print_clock_levels(adev, OD_SCLK, buf);
		size += amdgpu_dpm_print_clock_levels(adev, OD_MCLK, buf+size);
		size += amdgpu_dpm_print_clock_levels(adev, OD_VDDC_CURVE, buf+size);
		size += amdgpu_dpm_print_clock_levels(adev, OD_RANGE, buf+size);
	} else {
		size = snprintf(buf, PAGE_SIZE, "\n");
	}
	pm_runtime_mark_last_busy(ddev->dev);
	pm_runtime_put_autosuspend(ddev->dev);

	return size;
}

/**
 * DOC: pp_features
 *
 * The amdgpu driver provides a sysfs API for adjusting what powerplay
 * features to be enabled. The file pp_features is used for this. And
 * this is only available for Vega10 and later dGPUs.
 *
 * Reading back the file will show you the followings:
 * - Current ppfeature masks
 * - List of the all supported powerplay features with their naming,
 *   bitmasks and enablement status('Y'/'N' means "enabled"/"disabled").
 *
 * To manually enable or disable a specific feature, just set or clear
 * the corresponding bit from original ppfeature masks and input the
 * new ppfeature masks.
 */
static ssize_t amdgpu_set_pp_feature_status(struct device *dev,
		struct device_attribute *attr,
		const char *buf,
		size_t count)
{
	struct drm_device *ddev = dev_get_drvdata(dev);
	struct amdgpu_device *adev = ddev->dev_private;
	uint64_t featuremask;
	int ret;

	if (amdgpu_sriov_vf(adev))
		return -EINVAL;

	ret = kstrtou64(buf, 0, &featuremask);
	if (ret)
		return -EINVAL;

	pr_debug("featuremask = 0x%llx\n", featuremask);

	ret = pm_runtime_get_sync(ddev->dev);
	if (ret < 0)
		return ret;

	if (is_support_sw_smu(adev)) {
		ret = smu_sys_set_pp_feature_mask(&adev->smu, featuremask);
		if (ret) {
			pm_runtime_mark_last_busy(ddev->dev);
			pm_runtime_put_autosuspend(ddev->dev);
			return -EINVAL;
		}
	} else if (adev->powerplay.pp_funcs->set_ppfeature_status) {
		ret = amdgpu_dpm_set_ppfeature_status(adev, featuremask);
		if (ret) {
			pm_runtime_mark_last_busy(ddev->dev);
			pm_runtime_put_autosuspend(ddev->dev);
			return -EINVAL;
		}
	}
	pm_runtime_mark_last_busy(ddev->dev);
	pm_runtime_put_autosuspend(ddev->dev);

	return count;
}

static ssize_t amdgpu_get_pp_feature_status(struct device *dev,
		struct device_attribute *attr,
		char *buf)
{
	struct drm_device *ddev = dev_get_drvdata(dev);
	struct amdgpu_device *adev = ddev->dev_private;
	ssize_t size;
	int ret;

	if (amdgpu_sriov_vf(adev))
		return 0;

	ret = pm_runtime_get_sync(ddev->dev);
	if (ret < 0)
		return ret;

	if (is_support_sw_smu(adev))
		size = smu_sys_get_pp_feature_mask(&adev->smu, buf);
	else if (adev->powerplay.pp_funcs->get_ppfeature_status)
		size = amdgpu_dpm_get_ppfeature_status(adev, buf);
	else
		size = snprintf(buf, PAGE_SIZE, "\n");

	pm_runtime_mark_last_busy(ddev->dev);
	pm_runtime_put_autosuspend(ddev->dev);

	return size;
}

/**
 * DOC: pp_dpm_sclk pp_dpm_mclk pp_dpm_socclk pp_dpm_fclk pp_dpm_dcefclk pp_dpm_pcie
 *
 * The amdgpu driver provides a sysfs API for adjusting what power levels
 * are enabled for a given power state.  The files pp_dpm_sclk, pp_dpm_mclk,
 * pp_dpm_socclk, pp_dpm_fclk, pp_dpm_dcefclk and pp_dpm_pcie are used for
 * this.
 *
 * pp_dpm_socclk and pp_dpm_dcefclk interfaces are only available for
 * Vega10 and later ASICs.
 * pp_dpm_fclk interface is only available for Vega20 and later ASICs.
 *
 * Reading back the files will show you the available power levels within
 * the power state and the clock information for those levels.
 *
 * To manually adjust these states, first select manual using
 * power_dpm_force_performance_level.
 * Secondly, enter a new value for each level by inputing a string that
 * contains " echo xx xx xx > pp_dpm_sclk/mclk/pcie"
 * E.g.,
 *
 * .. code-block:: bash
 *
 *	echo "4 5 6" > pp_dpm_sclk
 *
 * will enable sclk levels 4, 5, and 6.
 *
 * NOTE: change to the dcefclk max dpm level is not supported now
 */

static ssize_t amdgpu_get_pp_dpm_sclk(struct device *dev,
		struct device_attribute *attr,
		char *buf)
{
	struct drm_device *ddev = dev_get_drvdata(dev);
	struct amdgpu_device *adev = ddev->dev_private;
	ssize_t size;
	int ret;

	if (amdgpu_sriov_vf(adev) && !amdgpu_sriov_is_pp_one_vf(adev))
		return 0;

	ret = pm_runtime_get_sync(ddev->dev);
	if (ret < 0)
		return ret;

	if (is_support_sw_smu(adev))
		size = smu_print_clk_levels(&adev->smu, SMU_SCLK, buf);
	else if (adev->powerplay.pp_funcs->print_clock_levels)
		size = amdgpu_dpm_print_clock_levels(adev, PP_SCLK, buf);
	else
		size = snprintf(buf, PAGE_SIZE, "\n");

	pm_runtime_mark_last_busy(ddev->dev);
	pm_runtime_put_autosuspend(ddev->dev);

	return size;
}

/*
 * Worst case: 32 bits individually specified, in octal at 12 characters
 * per line (+1 for \n).
 */
#define AMDGPU_MASK_BUF_MAX	(32 * 13)

static ssize_t amdgpu_read_mask(const char *buf, size_t count, uint32_t *mask)
{
	int ret;
	long level;
	char *sub_str = NULL;
	char *tmp;
	char buf_cpy[AMDGPU_MASK_BUF_MAX + 1];
	const char delimiter[3] = {' ', '\n', '\0'};
	size_t bytes;

	*mask = 0;

	bytes = min(count, sizeof(buf_cpy) - 1);
	memcpy(buf_cpy, buf, bytes);
	buf_cpy[bytes] = '\0';
	tmp = buf_cpy;
	while (tmp[0]) {
		sub_str = strsep(&tmp, delimiter);
		if (strlen(sub_str)) {
			ret = kstrtol(sub_str, 0, &level);
			if (ret)
				return -EINVAL;
			*mask |= 1 << level;
		} else
			break;
	}

	return 0;
}

static ssize_t amdgpu_set_pp_dpm_sclk(struct device *dev,
		struct device_attribute *attr,
		const char *buf,
		size_t count)
{
	struct drm_device *ddev = dev_get_drvdata(dev);
	struct amdgpu_device *adev = ddev->dev_private;
	int ret;
	uint32_t mask = 0;

	if (amdgpu_sriov_vf(adev) && !amdgpu_sriov_is_pp_one_vf(adev))
		return -EINVAL;

	ret = amdgpu_read_mask(buf, count, &mask);
	if (ret)
		return ret;

	ret = pm_runtime_get_sync(ddev->dev);
	if (ret < 0)
		return ret;

	if (is_support_sw_smu(adev))
		ret = smu_force_clk_levels(&adev->smu, SMU_SCLK, mask, true);
	else if (adev->powerplay.pp_funcs->force_clock_level)
		ret = amdgpu_dpm_force_clock_level(adev, PP_SCLK, mask);

	pm_runtime_mark_last_busy(ddev->dev);
	pm_runtime_put_autosuspend(ddev->dev);

	if (ret)
		return -EINVAL;

	return count;
}

static ssize_t pp_dpm_min_sclk_store(struct device *dev,
				     struct device_attribute *attr,
				     const char *buf,
				     size_t count)
{
	struct drm_device *ddev = dev_get_drvdata(dev);
	struct amdgpu_device *adev = ddev->dev_private;
	int ret;
	u32 clk = 0;

	if (amdgpu_sriov_vf(adev) && !amdgpu_sriov_is_pp_one_vf(adev))
		return -EINVAL;

	ret = kstrtou32(buf, 0, &clk);
	if (ret)
		return ret;

	//max sclk is 1600M
	if (clk > AMDGPU_DPM_MAX_SCLK_MERO)
		return -EINVAL;

	ret = pm_runtime_get_sync(ddev->dev);
	if (ret < 0)
		return ret;

	if (is_support_sw_smu(adev))
		smu_set_soft_limit_min(&adev->smu, SMU_SCLK, clk);
	else
		DRM_INFO("not support yet\n");

	pm_runtime_mark_last_busy(ddev->dev);
	pm_runtime_put_autosuspend(ddev->dev);

	return count;
}

static ssize_t pp_dpm_max_sclk_store(struct device *dev,
				     struct device_attribute *attr,
				     const char *buf,
				     size_t count)
{
	struct drm_device *ddev = dev_get_drvdata(dev);
	struct amdgpu_device *adev = ddev->dev_private;
	int ret;
	u32 clk = 0;

	if (amdgpu_sriov_vf(adev) && !amdgpu_sriov_is_pp_one_vf(adev))
		return -EINVAL;

	ret = kstrtou32(buf, 0, &clk);
	if (ret)
		return ret;

	//max sclk is 1600M
	if (clk > AMDGPU_DPM_MAX_SCLK_MERO)
		return -EINVAL;

	ret = pm_runtime_get_sync(ddev->dev);
	if (ret < 0)
		return ret;

	if (is_support_sw_smu(adev))
		smu_set_soft_limit_max(&adev->smu, SMU_SCLK, clk);
	else
		DRM_INFO("not support yet\n");

	pm_runtime_mark_last_busy(ddev->dev);
	pm_runtime_put_autosuspend(ddev->dev);

	return count;
}

static ssize_t pp_dpm_min_cclk_store(struct device *dev,
				     struct device_attribute *attr,
				     const char *buf,
				     size_t count)
{
	struct drm_device *ddev = dev_get_drvdata(dev);
	struct amdgpu_device *adev = ddev->dev_private;
	int ret;
	u32 clk = 0;

	if (amdgpu_sriov_vf(adev) && !amdgpu_sriov_is_pp_one_vf(adev))
		return -EINVAL;

	ret = kstrtou32(buf, 0, &clk);
	if (ret)
		return ret;

	//max cclk is 2400M, max cpu core id is 3
	if ((clk & AMDCPU_DPM_CCLK_MASK) > AMDCPU_DPM_MAX_CCLK_MERO ||
	    ((clk & AMDCPU_DPM_CORE_NUM_MASK) >> AMDCPU_DPM_CORE_NUM_OFFSET) >
	    AMDCPU_DPM_MAX_CORE_NUM_MERO)
		return -EINVAL;

	ret = pm_runtime_get_sync(ddev->dev);
	if (ret < 0)
		return ret;

	if (is_support_sw_smu(adev))
		smu_set_soft_limit_min(&adev->smu, SMU_CPU_CLK, clk);
	else
		DRM_INFO("not support yet\n");

	pm_runtime_mark_last_busy(ddev->dev);
	pm_runtime_put_autosuspend(ddev->dev);

	return count;
}

static ssize_t pp_dpm_max_cclk_store(struct device *dev,
				     struct device_attribute *attr,
				     const char *buf,
				     size_t count)
{
	struct drm_device *ddev = dev_get_drvdata(dev);
	struct amdgpu_device *adev = ddev->dev_private;
	int ret;
	u32 clk = 0;

	if (amdgpu_sriov_vf(adev) && !amdgpu_sriov_is_pp_one_vf(adev))
		return -EINVAL;

	ret = kstrtou32(buf, 0, &clk);
	if (ret)
		return ret;

	//max cclk is 2400M, max cpu core id is 3
	if ((clk & AMDCPU_DPM_CCLK_MASK) > AMDCPU_DPM_MAX_CCLK_MERO ||
	    ((clk & AMDCPU_DPM_CORE_NUM_MASK) >> AMDCPU_DPM_CORE_NUM_OFFSET) >
	    AMDCPU_DPM_MAX_CORE_NUM_MERO)
		return -EINVAL;

	ret = pm_runtime_get_sync(ddev->dev);
	if (ret < 0)
		return ret;

	if (is_support_sw_smu(adev))
		smu_set_soft_limit_max(&adev->smu, SMU_CPU_CLK, clk);
	else
		DRM_INFO("not support yet\n");

	pm_runtime_mark_last_busy(ddev->dev);
	pm_runtime_put_autosuspend(ddev->dev);

	return count;
}

static ssize_t amdgpu_get_pp_dpm_mclk(struct device *dev,
		struct device_attribute *attr,
		char *buf)
{
	struct drm_device *ddev = dev_get_drvdata(dev);
	struct amdgpu_device *adev = ddev->dev_private;
	ssize_t size;
	int ret;

	if (amdgpu_sriov_vf(adev) && !amdgpu_sriov_is_pp_one_vf(adev))
		return 0;

	ret = pm_runtime_get_sync(ddev->dev);
	if (ret < 0)
		return ret;

	if (is_support_sw_smu(adev))
		size = smu_print_clk_levels(&adev->smu, SMU_MCLK, buf);
	else if (adev->powerplay.pp_funcs->print_clock_levels)
		size = amdgpu_dpm_print_clock_levels(adev, PP_MCLK, buf);
	else
		size = snprintf(buf, PAGE_SIZE, "\n");

	pm_runtime_mark_last_busy(ddev->dev);
	pm_runtime_put_autosuspend(ddev->dev);

	return size;
}

static ssize_t amdgpu_set_pp_dpm_mclk(struct device *dev,
		struct device_attribute *attr,
		const char *buf,
		size_t count)
{
	struct drm_device *ddev = dev_get_drvdata(dev);
	struct amdgpu_device *adev = ddev->dev_private;
	uint32_t mask = 0;
	int ret;

	if (amdgpu_sriov_vf(adev) && !amdgpu_sriov_is_pp_one_vf(adev))
			return -EINVAL;

	ret = amdgpu_read_mask(buf, count, &mask);
	if (ret)
		return ret;

	ret = pm_runtime_get_sync(ddev->dev);
	if (ret < 0)
		return ret;

	if (is_support_sw_smu(adev))
		ret = smu_force_clk_levels(&adev->smu, SMU_MCLK, mask, true);
	else if (adev->powerplay.pp_funcs->force_clock_level)
		ret = amdgpu_dpm_force_clock_level(adev, PP_MCLK, mask);

	pm_runtime_mark_last_busy(ddev->dev);
	pm_runtime_put_autosuspend(ddev->dev);

	if (ret)
		return -EINVAL;

	return count;
}

static ssize_t amdgpu_get_pp_dpm_socclk(struct device *dev,
		struct device_attribute *attr,
		char *buf)
{
	struct drm_device *ddev = dev_get_drvdata(dev);
	struct amdgpu_device *adev = ddev->dev_private;
	ssize_t size;
	int ret;

	if (amdgpu_sriov_vf(adev) && !amdgpu_sriov_is_pp_one_vf(adev))
		return 0;

	ret = pm_runtime_get_sync(ddev->dev);
	if (ret < 0)
		return ret;

	if (is_support_sw_smu(adev))
		size = smu_print_clk_levels(&adev->smu, SMU_SOCCLK, buf);
	else if (adev->powerplay.pp_funcs->print_clock_levels)
		size = amdgpu_dpm_print_clock_levels(adev, PP_SOCCLK, buf);
	else
		size = snprintf(buf, PAGE_SIZE, "\n");

	pm_runtime_mark_last_busy(ddev->dev);
	pm_runtime_put_autosuspend(ddev->dev);

	return size;
}

static ssize_t amdgpu_set_pp_dpm_socclk(struct device *dev,
		struct device_attribute *attr,
		const char *buf,
		size_t count)
{
	struct drm_device *ddev = dev_get_drvdata(dev);
	struct amdgpu_device *adev = ddev->dev_private;
	int ret;
	uint32_t mask = 0;

	if (amdgpu_sriov_vf(adev) && !amdgpu_sriov_is_pp_one_vf(adev))
		return -EINVAL;

	ret = amdgpu_read_mask(buf, count, &mask);
	if (ret)
		return ret;

	ret = pm_runtime_get_sync(ddev->dev);
	if (ret < 0)
		return ret;

	if (is_support_sw_smu(adev))
		ret = smu_force_clk_levels(&adev->smu, SMU_SOCCLK, mask, true);
	else if (adev->powerplay.pp_funcs->force_clock_level)
		ret = amdgpu_dpm_force_clock_level(adev, PP_SOCCLK, mask);
	else
		ret = 0;

	pm_runtime_mark_last_busy(ddev->dev);
	pm_runtime_put_autosuspend(ddev->dev);

	if (ret)
		return -EINVAL;

	return count;
}

static ssize_t amdgpu_get_pp_dpm_fclk(struct device *dev,
		struct device_attribute *attr,
		char *buf)
{
	struct drm_device *ddev = dev_get_drvdata(dev);
	struct amdgpu_device *adev = ddev->dev_private;
	ssize_t size;
	int ret;

	if (amdgpu_sriov_vf(adev) && !amdgpu_sriov_is_pp_one_vf(adev))
		return 0;

	ret = pm_runtime_get_sync(ddev->dev);
	if (ret < 0)
		return ret;

	if (is_support_sw_smu(adev))
		size = smu_print_clk_levels(&adev->smu, SMU_FCLK, buf);
	else if (adev->powerplay.pp_funcs->print_clock_levels)
		size = amdgpu_dpm_print_clock_levels(adev, PP_FCLK, buf);
	else
		size = snprintf(buf, PAGE_SIZE, "\n");

	pm_runtime_mark_last_busy(ddev->dev);
	pm_runtime_put_autosuspend(ddev->dev);

	return size;
}

static ssize_t amdgpu_set_pp_dpm_fclk(struct device *dev,
		struct device_attribute *attr,
		const char *buf,
		size_t count)
{
	struct drm_device *ddev = dev_get_drvdata(dev);
	struct amdgpu_device *adev = ddev->dev_private;
	int ret;
	uint32_t mask = 0;

	if (amdgpu_sriov_vf(adev) && !amdgpu_sriov_is_pp_one_vf(adev))
		return -EINVAL;

	ret = amdgpu_read_mask(buf, count, &mask);
	if (ret)
		return ret;

	ret = pm_runtime_get_sync(ddev->dev);
	if (ret < 0)
		return ret;

	if (is_support_sw_smu(adev))
		ret = smu_force_clk_levels(&adev->smu, SMU_FCLK, mask, true);
	else if (adev->powerplay.pp_funcs->force_clock_level)
		ret = amdgpu_dpm_force_clock_level(adev, PP_FCLK, mask);
	else
		ret = 0;

	pm_runtime_mark_last_busy(ddev->dev);
	pm_runtime_put_autosuspend(ddev->dev);

	if (ret)
		return -EINVAL;

	return count;
}

/**
 * DOC: fclk_dpm_activity_monitor
 *
 * The amdgpu driver provides a sysfs API for adjusting certain power
 * related parameters.  The file fclk_dpm_activity_monitor is used for this.
 * It accepts the following input argument:
 *
 * 0: For FCLK DPM to use activity-based control.
 * 1: To override activity-based control and use active-level to dictate Pstate.
 * Mero PMFW has activity-level method as default.
 */
static ssize_t fclk_dpm_activity_monitor_store(struct device *dev,
				     struct device_attribute *attr,
				     const char *buf,
				     size_t count)
{
	struct drm_device *ddev = dev_get_drvdata(dev);
	struct amdgpu_device *adev = ddev->dev_private;
	int ret;
	u32 value = 0;

	if (amdgpu_sriov_vf(adev) && !amdgpu_sriov_is_pp_one_vf(adev))
		return -EINVAL;

	ret = kstrtou32(buf, 10, &value);
	if (ret)
		return ret;

	ret = pm_runtime_get_sync(ddev->dev);
	if (ret < 0)
		return ret;

	if (is_support_sw_smu(adev))
		ret = smu_clk_dpm_tuning_enable(&adev->smu, SMU_FCLK, value);
	else
		DRM_INFO("fclk dpm tuning is not supported yet\n");

	pm_runtime_mark_last_busy(ddev->dev);
	pm_runtime_put_autosuspend(ddev->dev);

	if (ret)
		return -EINVAL;

	return count;
}

static ssize_t amdgpu_get_pp_fclk_dpm_params(struct device *dev,
		struct device_attribute *attr,
		char *buf)
{
	struct drm_device *ddev = dev_get_drvdata(dev);
	struct amdgpu_device *adev = ddev->dev_private;
	ssize_t size;
	int ret;

	if (amdgpu_sriov_vf(adev) && !amdgpu_sriov_is_pp_one_vf(adev))
		return 0;

	ret = pm_runtime_get_sync(ddev->dev);
	if (ret < 0)
		return ret;

	if (is_support_sw_smu(adev))
		size = snprintf(buf, PAGE_SIZE, "0x%x\n", adev->smu.fclk_actual_dpm_params);
	else
		size = snprintf(buf, PAGE_SIZE, "\n");

	pm_runtime_mark_last_busy(ddev->dev);
	pm_runtime_put_autosuspend(ddev->dev);

	return size;
}

static uint32_t amdgpu_calculate_dpm_params(const char *buf, size_t count, uint32_t *mask, uint32_t valid_size)
{
	uint32_t parameter[64], parameter_size = 0;
	char buf_cpy[128], *tmp_str, *sub_str;
	const char delimiter[3] = {' ', '\n', '\0'};
	int ret = 0;

	memcpy(buf_cpy, buf, count+1);
	tmp_str = buf_cpy;

	while ((sub_str = strsep(&tmp_str, delimiter)) != NULL) {
		if (strlen(sub_str) == 0)
			continue;
		ret = kstrtou32(sub_str, 0, &parameter[parameter_size]);
		if (ret)
			return -EINVAL;
		pr_debug("clk dpm parameter[%d] is 0x%x.\n", parameter_size, parameter[parameter_size]);
		parameter_size++;

		while (isspace(*tmp_str))
			tmp_str++;
	}

	if (parameter_size != valid_size) {
		pr_debug("Something wrong with fclk dpm mask!\n");
		return -EINVAL;
	}

	if (parameter_size == GET_FCLK_DPM_PARAMS_SIZE) {
		if ((parameter[0] != FCLK_DPM_AC) && (parameter[0] != FCLK_DPM_DC)) {
			pr_err("fclk dpm setting is invalid inputs, the first parameter must be 1(AC) or 2(DC)!\n");
			return -EINVAL;
		}

		*mask = (parameter[0]<<30) | (parameter[1]<<27) | (parameter[2]<<25);
	}
	else if (parameter_size == SET_FCLK_DPM_CONSTANTS_SIZE) {
		/* Real threshold or Alpha value are multiplied by 1000.
		 * Threshold is any value between 0 and 100. e.g. 0.01, 0.05
		 * Alpha is any fractional value between 0 and 1. e.g. 25, 41.25
		 */
		if ((parameter[2] == FCLK_DPM_TH_FLAG) && (parameter[3] > FCLK_DPM_MAX_TH)) {
			pr_debug("fclk dpm threshold value error!\n");
			return -EINVAL;
		}

		if ((parameter[2] == FCLK_DPM_ALPHA_FLAG) && (parameter[3] > FCLK_DPM_MAX_ALPHA)) {
			pr_debug("fclk dpm alpha value error!\n");
			return -EINVAL;
		}

		*mask = (parameter[0]<<30) | (parameter[1]<<27) | (parameter[2]<<25) | parameter[3];
	} else if (parameter_size == SET_LCLK_DPM_CONSTANTS_SIZE) {
		/* Real threshold value is multiplied by 1000.
		 * Threshold is any value between 0 and 100. e.g. 0.01, 0.05
		 */
		if (parameter[1] > LCLK_DPM_MAX_TH) {
			pr_debug("lclk dpm threshold value error!\n");
			return -EINVAL;
		}
		*mask = (parameter[0]<<30) | parameter[1];
	}

	pr_debug("fclk/lclk dpm mask is 0x%x.\n", *mask);
	return 0;
}

static ssize_t amdgpu_set_pp_fclk_dpm_params(struct device *dev,
		struct device_attribute *attr,
		const char *buf,
		size_t count)
{
	struct drm_device *ddev = dev_get_drvdata(dev);
	struct amdgpu_device *adev = ddev->dev_private;
	int ret;
	uint32_t mask = 0;

	if (amdgpu_sriov_vf(adev) && !amdgpu_sriov_is_pp_one_vf(adev))
		return -EINVAL;

	ret = amdgpu_calculate_dpm_params(buf, count, &mask, GET_FCLK_DPM_PARAMS_SIZE);
	if (ret)
		return ret;

	ret = pm_runtime_get_sync(ddev->dev);
	if (ret < 0)
		return ret;

	if (is_support_sw_smu(adev))
		adev->smu.fclk_actual_dpm_params = mask;
	else
		DRM_INFO("fclk dpm tuning is not supported yet\n");

	pm_runtime_mark_last_busy(ddev->dev);
	pm_runtime_put_autosuspend(ddev->dev);

	return count;
}

static ssize_t amdgpu_get_pp_dpm_tuning_fclk(struct device *dev,
		struct device_attribute *attr,
		char *buf)
{
	struct drm_device *ddev = dev_get_drvdata(dev);
	struct amdgpu_device *adev = ddev->dev_private;
	ssize_t size;
	int ret;

	if (amdgpu_sriov_vf(adev) && !amdgpu_sriov_is_pp_one_vf(adev))
		return 0;

	ret = pm_runtime_get_sync(ddev->dev);
	if (ret < 0)
		return ret;

	if (is_support_sw_smu(adev))
		size = smu_print_dpm_contants(&adev->smu, SMU_FCLK, buf);
	else
		size = snprintf(buf, PAGE_SIZE, "\n");

	pm_runtime_mark_last_busy(ddev->dev);
	pm_runtime_put_autosuspend(ddev->dev);

	return size;
}

/**
 * DOC: pp_dpm_tuning_fclk
 *
 * The amdgpu driver provides a sysfs API for adjusting certain power
 * related parameters.  The file pp_dpm_tuning_fclk is used for this.
 * It accepts the following arguments:
 *
 * argument 1: select which threshold/alpha to update – AC, DC or both.
 *		1 = update AC only, 2 = update DC only, 3 = update both.
 * argument 2: selects client
 *		0 = GFX, 1 = DRAM, 2 = CCX, 3 = CVIP, 4 = IO
 * argument 3: selects Aplpha or Threshold
 *		0 = update Alpha, 1 = update Threshold, 2 = update LowerThreshold
 * argument 4: Threshold or Alpha value multiplied by 1000.
 *
 * e.g.
 * To set Threshold value of 41.25 for CCX client for AC power source
 * echo "1 2 1 41250" > /sys/class/drm/card0/device/pp_dpm_tuning_fclk
 */
static ssize_t amdgpu_set_pp_dpm_tuning_fclk(struct device *dev,
		struct device_attribute *attr,
		const char *buf,
		size_t count)
{
	struct drm_device *ddev = dev_get_drvdata(dev);
	struct amdgpu_device *adev = ddev->dev_private;
	int ret;
	uint32_t mask = 0;

	if (amdgpu_sriov_vf(adev) && !amdgpu_sriov_is_pp_one_vf(adev))
		return -EINVAL;

	ret = amdgpu_calculate_dpm_params(buf, count, &mask, SET_FCLK_DPM_CONSTANTS_SIZE);
	if (ret)
		return ret;

	ret = pm_runtime_get_sync(ddev->dev);
	if (ret < 0)
		return ret;

	if (is_support_sw_smu(adev))
		ret = smu_set_dpm_contants(&adev->smu, SMU_FCLK, mask);
	else
		DRM_INFO("fclk dpm tuning is not supported yet\n");

	pm_runtime_mark_last_busy(ddev->dev);
	pm_runtime_put_autosuspend(ddev->dev);

	if (ret)
		return -EINVAL;

	return count;
}

/**
 * DOC: lclk_busy_calc_from_iohc
 *
 * The amdgpu driver provides a sysfs API for adjusting certain power
 * related parameters.  The file lclk_busy_calc_from_iohc is used for this.
 * It accepts the following input argument:
 *
 * 0: To un-set the flag and use default mission mode method
 * 1: For calculating LCLK Busy only based on IOHC busy.
 * Mero PMFW has mission mode method as default.
 */
static ssize_t lclk_busy_calc_from_iohc_store(struct device *dev,
				     struct device_attribute *attr,
				     const char *buf,
				     size_t count)
{
	struct drm_device *ddev = dev_get_drvdata(dev);
	struct amdgpu_device *adev = ddev->dev_private;
	int ret;
	u32 value = 0;
	__maybe_unused uint32_t cvip_lclk_state;

	if (amdgpu_sriov_vf(adev) && !amdgpu_sriov_is_pp_one_vf(adev))
		return -EINVAL;

	ret = kstrtou32(buf, 10, &value);
	if (ret)
		return ret;

	ret = pm_runtime_get_sync(ddev->dev);
	if (ret < 0)
		return ret;

	if (is_support_sw_smu(adev)) {
#ifdef CONFIG_ML_GSM
		ret = gsm_spin_lock(&lclk_spinlock);
		if (ret)
			DRM_INFO("lclk is not initialized on CVIP yet\n");
		else {
			cvip_lclk_state = gsm_raw_read_32(GSM_CVIP_LCLK_STATE);
			if (cvip_lclk_state)
#endif
				ret = smu_clk_dpm_tuning_enable(&adev->smu,
						SMU_LCLK, value);
			adev->lclk_sysfs_en = value;
			DRM_INFO("lclk enable=%d\n", adev->lclk_sysfs_en);
#ifdef CONFIG_ML_GSM
			gsm_raw_write_32(GSM_NOVA_LCLK_STATE, value);
			gsm_spin_unlock(&lclk_spinlock);
		}
#endif
	} else
		DRM_INFO("lclk dpm tuning is not supported yet\n");

	pm_runtime_mark_last_busy(ddev->dev);
	pm_runtime_put_autosuspend(ddev->dev);

	if (ret)
		return -EINVAL;

	return count;
}

static ssize_t lclk_busy_calc_from_iohc_show(struct device *dev,
		struct device_attribute *attr,
		char *buf)
{
	struct drm_device *ddev = dev_get_drvdata(dev);
	struct amdgpu_device *adev = ddev->dev_private;

	return snprintf(buf, PAGE_SIZE, "%d\n", adev->lclk_sysfs_en);
}

/**
 * DOC: pp_dpm_tuning_lclk
 *
 * The amdgpu driver provides a sysfs API for adjusting certain power
 * related parameters.  The file pp_dpm_tuning_lclk is used for this.
 * It accepts the following arguments:
 *
 * argument 1: select which threshold to update – AC, DC or both.
 *		1 = update AC only, 2 = update DC only, 3 = update both.
 * argument 2: Threshold value multiplied by 1000.
 *		For 17.25, the value is 17250
 *
 * e.g.
 * To updates both AC and DC thresholds to 17.15
 * echo "3 17150" > /sys/class/drm/card0/device/pp_dpm_tuning_lclk
 */
static ssize_t pp_dpm_tuning_lclk_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf,
		size_t count)
{
	struct drm_device *ddev = dev_get_drvdata(dev);
	struct amdgpu_device *adev = ddev->dev_private;
	int ret;
	uint32_t mask = 0;
	const char delimiter[3] = {' ', '\n', '\0'};
	char buf_cpy[128], *tmp, *sub_str;
	int i;

	if (amdgpu_sriov_vf(adev) && !amdgpu_sriov_is_pp_one_vf(adev))
		return -EINVAL;

	ret = amdgpu_calculate_dpm_params(buf, count, &mask, SET_LCLK_DPM_CONSTANTS_SIZE);
	if (ret)
		return ret;

	ret = pm_runtime_get_sync(ddev->dev);
	if (ret < 0)
		return ret;

	if (is_support_sw_smu(adev)) {
		memcpy(buf_cpy, buf, count + 1);
		tmp = buf_cpy;
		for (i = 0; i < AMDGPU_MAX_LCLK_PARAMS; i++) {
			sub_str = strsep(&tmp, delimiter);
			ret = kstrtou32(sub_str, 10,
				&adev->lclk_tuning_params[i]);
			if (ret)
				return -EINVAL;
		}
		DRM_INFO("lclk params set to %d %d\n",
			  adev->lclk_tuning_params[0],
			  adev->lclk_tuning_params[1]);
		ret = smu_set_dpm_contants(&adev->smu, SMU_LCLK, mask);

	} else
		DRM_INFO("lclk dpm tuning is not supported yet\n");

	pm_runtime_mark_last_busy(ddev->dev);
	pm_runtime_put_autosuspend(ddev->dev);

	if (ret)
		return -EINVAL;

	return count;
}

static ssize_t pp_dpm_tuning_lclk_show(struct device *dev,
		struct device_attribute *attr,
		char *buf)
{
	struct drm_device *ddev = dev_get_drvdata(dev);
	struct amdgpu_device *adev = ddev->dev_private;

	return snprintf(buf, PAGE_SIZE, "%d %d\n", adev->lclk_tuning_params[0],
		adev->lclk_tuning_params[1]);
}

static ssize_t amdgpu_get_pp_dpm_dcefclk(struct device *dev,
		struct device_attribute *attr,
		char *buf)
{
	struct drm_device *ddev = dev_get_drvdata(dev);
	struct amdgpu_device *adev = ddev->dev_private;
	ssize_t size;
	int ret;

	if (amdgpu_sriov_vf(adev))
		return 0;

	ret = pm_runtime_get_sync(ddev->dev);
	if (ret < 0)
		return ret;

	if (is_support_sw_smu(adev))
		size = smu_print_clk_levels(&adev->smu, SMU_DCEFCLK, buf);
	else if (adev->powerplay.pp_funcs->print_clock_levels)
		size = amdgpu_dpm_print_clock_levels(adev, PP_DCEFCLK, buf);
	else
		size = snprintf(buf, PAGE_SIZE, "\n");

	pm_runtime_mark_last_busy(ddev->dev);
	pm_runtime_put_autosuspend(ddev->dev);

	return size;
}

static ssize_t amdgpu_set_pp_dpm_dcefclk(struct device *dev,
		struct device_attribute *attr,
		const char *buf,
		size_t count)
{
	struct drm_device *ddev = dev_get_drvdata(dev);
	struct amdgpu_device *adev = ddev->dev_private;
	int ret;
	uint32_t mask = 0;

	if (amdgpu_sriov_vf(adev))
		return -EINVAL;

	ret = amdgpu_read_mask(buf, count, &mask);
	if (ret)
		return ret;

	ret = pm_runtime_get_sync(ddev->dev);
	if (ret < 0)
		return ret;

	if (is_support_sw_smu(adev))
		ret = smu_force_clk_levels(&adev->smu, SMU_DCEFCLK, mask, true);
	else if (adev->powerplay.pp_funcs->force_clock_level)
		ret = amdgpu_dpm_force_clock_level(adev, PP_DCEFCLK, mask);
	else
		ret = 0;

	pm_runtime_mark_last_busy(ddev->dev);
	pm_runtime_put_autosuspend(ddev->dev);

	if (ret)
		return -EINVAL;

	return count;
}

static ssize_t pp_dpm_vclk_show(struct device *dev,
		struct device_attribute *attr,
		char *buf)
{
	struct drm_device *ddev = dev_get_drvdata(dev);
	struct amdgpu_device *adev = ddev->dev_private;
	ssize_t size;
	int ret;

	ret = pm_runtime_get_sync(ddev->dev);
	if (ret < 0) {
		pm_runtime_put_autosuspend(ddev->dev);
		return ret;
	}

	if (is_support_sw_smu(adev))
		size = smu_print_clk_levels(&adev->smu, SMU_VCLK, buf);
	else
		size = snprintf(buf, PAGE_SIZE, "\n");

	pm_runtime_mark_last_busy(ddev->dev);
	pm_runtime_put_autosuspend(ddev->dev);

	return size;
}

static ssize_t pp_dpm_vclk_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf,
		size_t count)
{
	struct drm_device *ddev = dev_get_drvdata(dev);
	struct amdgpu_device *adev = ddev->dev_private;
	int ret;
	uint32_t mask = 0;


	ret = amdgpu_read_mask(buf, count, &mask);
	if (ret)
		return ret;

	ret = pm_runtime_get_sync(ddev->dev);
	if (ret < 0) {
		pm_runtime_put_autosuspend(ddev->dev);
		return ret;
	}

	if (is_support_sw_smu(adev))
		ret = smu_force_clk_levels(&adev->smu, SMU_VCLK, mask, true);
	else
		ret = 0;

	pm_runtime_mark_last_busy(ddev->dev);
	pm_runtime_put_autosuspend(ddev->dev);

	if (ret)
		return -EINVAL;

	return count;
}

static ssize_t pp_dpm_dclk_show(struct device *dev,
		struct device_attribute *attr,
		char *buf)
{
	struct drm_device *ddev = dev_get_drvdata(dev);
	struct amdgpu_device *adev = ddev->dev_private;
	ssize_t size;
	int ret;


	ret = pm_runtime_get_sync(ddev->dev);
	if (ret < 0) {
		pm_runtime_put_autosuspend(ddev->dev);
		return ret;
	}

	if (is_support_sw_smu(adev))
		size = smu_print_clk_levels(&adev->smu, SMU_DCLK, buf);
	else
		size = snprintf(buf, PAGE_SIZE, "\n");

	pm_runtime_mark_last_busy(ddev->dev);
	pm_runtime_put_autosuspend(ddev->dev);

	return size;
}

static ssize_t pp_dpm_dclk_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf,
		size_t count)
{
	struct drm_device *ddev = dev_get_drvdata(dev);
	struct amdgpu_device *adev = ddev->dev_private;
	int ret;
	uint32_t mask = 0;

	ret = amdgpu_read_mask(buf, count, &mask);
	if (ret)
		return ret;

	ret = pm_runtime_get_sync(ddev->dev);
	if (ret < 0) {
		pm_runtime_put_autosuspend(ddev->dev);
		return ret;
	}

	if (is_support_sw_smu(adev))
		ret = smu_force_clk_levels(&adev->smu, SMU_DCLK, mask, true);
	else
		ret = 0;

	pm_runtime_mark_last_busy(ddev->dev);
	pm_runtime_put_autosuspend(ddev->dev);

	if (ret)
		return -EINVAL;

	return count;
}


static ssize_t amdgpu_get_pp_dpm_pcie(struct device *dev,
		struct device_attribute *attr,
		char *buf)
{
	struct drm_device *ddev = dev_get_drvdata(dev);
	struct amdgpu_device *adev = ddev->dev_private;
	ssize_t size;
	int ret;

	if (amdgpu_sriov_vf(adev) && !amdgpu_sriov_is_pp_one_vf(adev))
		return 0;

	ret = pm_runtime_get_sync(ddev->dev);
	if (ret < 0)
		return ret;

	if (is_support_sw_smu(adev))
		size = smu_print_clk_levels(&adev->smu, SMU_PCIE, buf);
	else if (adev->powerplay.pp_funcs->print_clock_levels)
		size = amdgpu_dpm_print_clock_levels(adev, PP_PCIE, buf);
	else
		size = snprintf(buf, PAGE_SIZE, "\n");

	pm_runtime_mark_last_busy(ddev->dev);
	pm_runtime_put_autosuspend(ddev->dev);

	return size;
}

static ssize_t amdgpu_set_pp_dpm_pcie(struct device *dev,
		struct device_attribute *attr,
		const char *buf,
		size_t count)
{
	struct drm_device *ddev = dev_get_drvdata(dev);
	struct amdgpu_device *adev = ddev->dev_private;
	int ret;
	uint32_t mask = 0;

	if (amdgpu_sriov_vf(adev) && !amdgpu_sriov_is_pp_one_vf(adev))
		return -EINVAL;

	ret = amdgpu_read_mask(buf, count, &mask);
	if (ret)
		return ret;

	ret = pm_runtime_get_sync(ddev->dev);
	if (ret < 0)
		return ret;

	if (is_support_sw_smu(adev))
		ret = smu_force_clk_levels(&adev->smu, SMU_PCIE, mask, true);
	else if (adev->powerplay.pp_funcs->force_clock_level)
		ret = amdgpu_dpm_force_clock_level(adev, PP_PCIE, mask);
	else
		ret = 0;

	pm_runtime_mark_last_busy(ddev->dev);
	pm_runtime_put_autosuspend(ddev->dev);

	if (ret)
		return -EINVAL;

	return count;
}

static ssize_t amdgpu_get_pp_sclk_od(struct device *dev,
		struct device_attribute *attr,
		char *buf)
{
	struct drm_device *ddev = dev_get_drvdata(dev);
	struct amdgpu_device *adev = ddev->dev_private;
	uint32_t value = 0;
	int ret;

	if (amdgpu_sriov_vf(adev))
		return 0;

	ret = pm_runtime_get_sync(ddev->dev);
	if (ret < 0)
		return ret;

	if (is_support_sw_smu(adev))
		value = smu_get_od_percentage(&(adev->smu), SMU_OD_SCLK);
	else if (adev->powerplay.pp_funcs->get_sclk_od)
		value = amdgpu_dpm_get_sclk_od(adev);

	pm_runtime_mark_last_busy(ddev->dev);
	pm_runtime_put_autosuspend(ddev->dev);

	return snprintf(buf, PAGE_SIZE, "%d\n", value);
}

static ssize_t amdgpu_set_pp_sclk_od(struct device *dev,
		struct device_attribute *attr,
		const char *buf,
		size_t count)
{
	struct drm_device *ddev = dev_get_drvdata(dev);
	struct amdgpu_device *adev = ddev->dev_private;
	int ret;
	long int value;

	if (amdgpu_sriov_vf(adev))
		return -EINVAL;

	ret = kstrtol(buf, 0, &value);

	if (ret)
		return -EINVAL;

	ret = pm_runtime_get_sync(ddev->dev);
	if (ret < 0)
		return ret;

	if (is_support_sw_smu(adev)) {
		value = smu_set_od_percentage(&(adev->smu), SMU_OD_SCLK, (uint32_t)value);
	} else {
		if (adev->powerplay.pp_funcs->set_sclk_od)
			amdgpu_dpm_set_sclk_od(adev, (uint32_t)value);

		if (adev->powerplay.pp_funcs->dispatch_tasks) {
			amdgpu_dpm_dispatch_task(adev, AMD_PP_TASK_READJUST_POWER_STATE, NULL);
		} else {
			adev->pm.dpm.current_ps = adev->pm.dpm.boot_ps;
			amdgpu_pm_compute_clocks(adev);
		}
	}

	pm_runtime_mark_last_busy(ddev->dev);
	pm_runtime_put_autosuspend(ddev->dev);

	return count;
}

static ssize_t amdgpu_get_pp_mclk_od(struct device *dev,
		struct device_attribute *attr,
		char *buf)
{
	struct drm_device *ddev = dev_get_drvdata(dev);
	struct amdgpu_device *adev = ddev->dev_private;
	uint32_t value = 0;
	int ret;

	if (amdgpu_sriov_vf(adev))
		return 0;

	ret = pm_runtime_get_sync(ddev->dev);
	if (ret < 0)
		return ret;

	if (is_support_sw_smu(adev))
		value = smu_get_od_percentage(&(adev->smu), SMU_OD_MCLK);
	else if (adev->powerplay.pp_funcs->get_mclk_od)
		value = amdgpu_dpm_get_mclk_od(adev);

	pm_runtime_mark_last_busy(ddev->dev);
	pm_runtime_put_autosuspend(ddev->dev);

	return snprintf(buf, PAGE_SIZE, "%d\n", value);
}

static ssize_t amdgpu_set_pp_mclk_od(struct device *dev,
		struct device_attribute *attr,
		const char *buf,
		size_t count)
{
	struct drm_device *ddev = dev_get_drvdata(dev);
	struct amdgpu_device *adev = ddev->dev_private;
	int ret;
	long int value;

	if (amdgpu_sriov_vf(adev))
		return 0;

	ret = kstrtol(buf, 0, &value);

	if (ret)
		return -EINVAL;

	ret = pm_runtime_get_sync(ddev->dev);
	if (ret < 0)
		return ret;

	if (is_support_sw_smu(adev)) {
		value = smu_set_od_percentage(&(adev->smu), SMU_OD_MCLK, (uint32_t)value);
	} else {
		if (adev->powerplay.pp_funcs->set_mclk_od)
			amdgpu_dpm_set_mclk_od(adev, (uint32_t)value);

		if (adev->powerplay.pp_funcs->dispatch_tasks) {
			amdgpu_dpm_dispatch_task(adev, AMD_PP_TASK_READJUST_POWER_STATE, NULL);
		} else {
			adev->pm.dpm.current_ps = adev->pm.dpm.boot_ps;
			amdgpu_pm_compute_clocks(adev);
		}
	}

	pm_runtime_mark_last_busy(ddev->dev);
	pm_runtime_put_autosuspend(ddev->dev);

	return count;
}

/**
 * DOC: pp_power_profile_mode
 *
 * The amdgpu driver provides a sysfs API for adjusting the heuristics
 * related to switching between power levels in a power state.  The file
 * pp_power_profile_mode is used for this.
 *
 * Reading this file outputs a list of all of the predefined power profiles
 * and the relevant heuristics settings for that profile.
 *
 * To select a profile or create a custom profile, first select manual using
 * power_dpm_force_performance_level.  Writing the number of a predefined
 * profile to pp_power_profile_mode will enable those heuristics.  To
 * create a custom set of heuristics, write a string of numbers to the file
 * starting with the number of the custom profile along with a setting
 * for each heuristic parameter.  Due to differences across asic families
 * the heuristic parameters vary from family to family.
 *
 */

static ssize_t amdgpu_get_pp_power_profile_mode(struct device *dev,
		struct device_attribute *attr,
		char *buf)
{
	struct drm_device *ddev = dev_get_drvdata(dev);
	struct amdgpu_device *adev = ddev->dev_private;
	ssize_t size;
	int ret;

	if (amdgpu_sriov_vf(adev) && !amdgpu_sriov_is_pp_one_vf(adev))
		return 0;

	ret = pm_runtime_get_sync(ddev->dev);
	if (ret < 0)
		return ret;

	if (is_support_sw_smu(adev))
		size = smu_get_power_profile_mode(&adev->smu, buf);
	else if (adev->powerplay.pp_funcs->get_power_profile_mode)
		size = amdgpu_dpm_get_power_profile_mode(adev, buf);
	else
		size = snprintf(buf, PAGE_SIZE, "\n");

	pm_runtime_mark_last_busy(ddev->dev);
	pm_runtime_put_autosuspend(ddev->dev);

	return size;
}


static ssize_t amdgpu_set_pp_power_profile_mode(struct device *dev,
		struct device_attribute *attr,
		const char *buf,
		size_t count)
{
	int ret = 0xff;
	struct drm_device *ddev = dev_get_drvdata(dev);
	struct amdgpu_device *adev = ddev->dev_private;
	uint32_t parameter_size = 0;
	long parameter[64];
	char *sub_str, buf_cpy[128];
	char *tmp_str;
	uint32_t i = 0;
	char tmp[4];
	long int profile_mode = 0;
	const char delimiter[3] = {' ', '\n', '\0'};

	tmp[0] = *(buf);
	tmp[1] = '\0';
	ret = kstrtol(tmp, 0, &profile_mode);
	if (ret)
		return -EINVAL;

	if (amdgpu_sriov_vf(adev) && !amdgpu_sriov_is_pp_one_vf(adev))
		return -EINVAL;

	if (profile_mode == PP_SMC_POWER_PROFILE_CUSTOM) {
		if (count < 2 || count > 127)
			return -EINVAL;
		while (isspace(*++buf))
			i++;
		memcpy(buf_cpy, buf, count-i);
		tmp_str = buf_cpy;
		while (tmp_str[0]) {
			sub_str = strsep(&tmp_str, delimiter);
			ret = kstrtol(sub_str, 0, &parameter[parameter_size]);
			if (ret)
				return -EINVAL;
			parameter_size++;
			while (isspace(*tmp_str))
				tmp_str++;
		}
	}
	parameter[parameter_size] = profile_mode;

	ret = pm_runtime_get_sync(ddev->dev);
	if (ret < 0)
		return ret;

	if (is_support_sw_smu(adev))
		ret = smu_set_power_profile_mode(&adev->smu, parameter, parameter_size, true);
	else if (adev->powerplay.pp_funcs->set_power_profile_mode)
		ret = amdgpu_dpm_set_power_profile_mode(adev, parameter, parameter_size);

	pm_runtime_mark_last_busy(ddev->dev);
	pm_runtime_put_autosuspend(ddev->dev);

	if (!ret)
		return count;

	return -EINVAL;
}

static ssize_t amdgpu_get_pp_df_softmin_level(struct device *dev,
		struct device_attribute *attr,
		char *buf)
{
	struct drm_device *ddev = dev_get_drvdata(dev);
	struct amdgpu_device *adev = ddev->dev_private;

	return snprintf(buf, PAGE_SIZE, "%d\n", adev->fclk_softmin_lvl);
}

/**
 * DOC: df_softmin_level
 *
 * The amdgpu driver provides a sysfs API for set the df softmin level
 * when receive the active df level, SMU firmware change df active level
 * accordingly when system is busy
 */
static ssize_t amdgpu_set_pp_df_softmin_level(struct device *dev,
					      struct device_attribute *attr,
					      const char *buf,
					      size_t count)
{
	struct drm_device *ddev = dev_get_drvdata(dev);
	struct amdgpu_device *adev = ddev->dev_private;
	int r;
	enum smu_df_state_t state = SMU_DF_STATE_IDLE;
	enum smu_df_level_t level = SMU_DF_LEVEL_LOW;
	u32 value;
	__maybe_unused uint32_t cvip_fclk_state;

	if (amdgpu_sriov_vf(adev) && !amdgpu_sriov_is_pp_one_vf(adev))
		return 0;

	r = kstrtou32(buf, 10, &value);
	if (r)
		return r;

	if (value != SMU_DF_LEVEL_HIGH &&
	    value != SMU_DF_LEVEL_LOW)
		return -EINVAL;

	level = value;

	r = pm_runtime_get_sync(ddev->dev);
	if (r < 0)
		return r;

	if (is_support_sw_smu(adev)) {
#ifdef CONFIG_ML_GSM
		r = gsm_spin_lock(&fclk_spinlock);
		if (r)
			DRM_INFO("fclk is not initialized on CVIP yet\n");
		else {
			cvip_fclk_state = gsm_raw_read_32(GSM_CVIP_FCLK_STATE);
			if (cvip_fclk_state)
#endif
				r = smu_set_df_state_level(&adev->smu,
					   state, level);
			adev->fclk_softmin_lvl = level;
#ifdef CONFIG_ML_GSM
			gsm_raw_write_32(GSM_NOVA_FCLK_STATE, level);
			gsm_spin_unlock(&fclk_spinlock);
		}
#endif

	} else
		return -EINVAL;

	pm_runtime_mark_last_busy(ddev->dev);
	pm_runtime_put_autosuspend(ddev->dev);

	if (!r)
		return count;

	return -EINVAL;
}

static ssize_t amdgpu_get_pp_df_active_level(struct device *dev,
		struct device_attribute *attr,
		char *buf)
{
	struct drm_device *ddev = dev_get_drvdata(dev);
	struct amdgpu_device *adev = ddev->dev_private;

	return snprintf(buf, PAGE_SIZE, "%d\n", adev->fclk_active_lvl);
}

/**
 * DOC: df_active_level
 *
 * The amdgpu driver provides a sysfs API for set the df active level
 * when receive the active df level, SMU firmware change df active level
 * accordingly when system is busy
 */
static ssize_t amdgpu_set_pp_df_active_level(struct device *dev,
					     struct device_attribute *attr,
					     const char *buf,
					     size_t count)
{
	struct drm_device *ddev = dev_get_drvdata(dev);
	struct amdgpu_device *adev = ddev->dev_private;
	int r;
	enum smu_df_state_t state = SMU_DF_STATE_ACTIVE;
	enum smu_df_level_t level = SMU_DF_LEVEL_HIGH;
	u32 value;

	if (amdgpu_sriov_vf(adev) && !amdgpu_sriov_is_pp_one_vf(adev))
		return 0;

	r = kstrtou32(buf, 10, &value);
	if (r)
		return r;

	if (value != SMU_DF_LEVEL_HIGH &&
	    value != SMU_DF_LEVEL_LOW)
		return -EINVAL;

	level = value;

	r = pm_runtime_get_sync(ddev->dev);
	if (r < 0)
		return r;

	if (is_support_sw_smu(adev)) {
		r = smu_set_df_state_level(&adev->smu,
					   state,
					   level);
		adev->fclk_active_lvl = level;
	}
	else
		return -EINVAL;

	pm_runtime_mark_last_busy(ddev->dev);
	pm_runtime_put_autosuspend(ddev->dev);

	if (!r)
		return count;

	return -EINVAL;
}

/**
 * DOC: cclk_policy
 *
 * The amdgpu driver provides a sysfs API for set the cclk power policy
 * when receive the policy change request, SMU firmware change cclk power
 * policy between pstate control and r2idle
 */
static ssize_t amdgpu_set_pp_cclk_policy(struct device *dev,
					 struct device_attribute *attr,
					 const char *buf,
					 size_t count)
{
	struct drm_device *ddev = dev_get_drvdata(dev);
	struct amdgpu_device *adev = ddev->dev_private;
	int r;
	enum smu_cclk_profile_t mode = SMU_PD_PROFILE;
	u32 value;

	if (amdgpu_sriov_vf(adev) && !amdgpu_sriov_is_pp_one_vf(adev))
		return 0;

	r = kstrtou32(buf, 10, &value);
	if (r)
		return r;

	if (value != SMU_PD_PROFILE &&
	    value != SMU_R2IDLE_PROFILE)
		return -EINVAL;

	mode = value;

	r = pm_runtime_get_sync(ddev->dev);
	if (r < 0)
		return r;

	if (is_support_sw_smu(adev))
		r = smu_set_cclk_power_profile_mode(&adev->smu,
						    mode);
	else
		return -EINVAL;

	pm_runtime_mark_last_busy(ddev->dev);
	pm_runtime_put_autosuspend(ddev->dev);

	if (!r)
		return count;

	return -EINVAL;
}

/**
 * DOC: busy_percent
 *
 * The amdgpu driver provides a sysfs API for reading how busy the GPU
 * is as a percentage.  The file gpu_busy_percent is used for this.
 * The SMU firmware computes a percentage of load based on the
 * aggregate activity level in the IP cores.
 */
static ssize_t amdgpu_get_busy_percent(struct device *dev,
		struct device_attribute *attr,
		char *buf)
{
	struct drm_device *ddev = dev_get_drvdata(dev);
	struct amdgpu_device *adev = ddev->dev_private;
	int r, value, size = sizeof(value);

	if (amdgpu_sriov_vf(adev) && !amdgpu_sriov_is_pp_one_vf(adev))
		return 0;

	r = pm_runtime_get_sync(ddev->dev);
	if (r < 0)
		return r;

	/* read the IP busy sensor */
	r = amdgpu_dpm_read_sensor(adev, AMDGPU_PP_SENSOR_GPU_LOAD,
				   (void *)&value, &size);

	pm_runtime_mark_last_busy(ddev->dev);
	pm_runtime_put_autosuspend(ddev->dev);

	if (r)
		return r;

	return snprintf(buf, PAGE_SIZE, "%d\n", value);
}

/**
 * DOC: mem_busy_percent
 *
 * The amdgpu driver provides a sysfs API for reading how busy the VRAM
 * is as a percentage.  The file mem_busy_percent is used for this.
 * The SMU firmware computes a percentage of load based on the
 * aggregate activity level in the IP cores.
 */
static ssize_t amdgpu_get_memory_busy_percent(struct device *dev,
		struct device_attribute *attr,
		char *buf)
{
	struct drm_device *ddev = dev_get_drvdata(dev);
	struct amdgpu_device *adev = ddev->dev_private;
	int r, value, size = sizeof(value);

	if (amdgpu_sriov_vf(adev) && !amdgpu_sriov_is_pp_one_vf(adev))
		return 0;

	r = pm_runtime_get_sync(ddev->dev);
	if (r < 0)
		return r;

	/* read the IP busy sensor */
	r = amdgpu_dpm_read_sensor(adev, AMDGPU_PP_SENSOR_MEM_LOAD,
				   (void *)&value, &size);

	pm_runtime_mark_last_busy(ddev->dev);
	pm_runtime_put_autosuspend(ddev->dev);

	if (r)
		return r;

	return snprintf(buf, PAGE_SIZE, "%d\n", value);
}

/**
 * DOC: pcie_bw
 *
 * The amdgpu driver provides a sysfs API for estimating how much data
 * has been received and sent by the GPU in the last second through PCIe.
 * The file pcie_bw is used for this.
 * The Perf counters count the number of received and sent messages and return
 * those values, as well as the maximum payload size of a PCIe packet (mps).
 * Note that it is not possible to easily and quickly obtain the size of each
 * packet transmitted, so we output the max payload size (mps) to allow for
 * quick estimation of the PCIe bandwidth usage
 */
static ssize_t amdgpu_get_pcie_bw(struct device *dev,
		struct device_attribute *attr,
		char *buf)
{
	struct drm_device *ddev = dev_get_drvdata(dev);
	struct amdgpu_device *adev = ddev->dev_private;
	uint64_t count0, count1;
	int ret;

	if (amdgpu_sriov_vf(adev) && !amdgpu_sriov_is_pp_one_vf(adev))
		return 0;

	ret = pm_runtime_get_sync(ddev->dev);
	if (ret < 0)
		return ret;

	amdgpu_asic_get_pcie_usage(adev, &count0, &count1);

	pm_runtime_mark_last_busy(ddev->dev);
	pm_runtime_put_autosuspend(ddev->dev);

	return snprintf(buf, PAGE_SIZE,	"%llu %llu %i\n",
			count0, count1, pcie_get_mps(adev->pdev));
}

/**
 * DOC: unique_id
 *
 * The amdgpu driver provides a sysfs API for providing a unique ID for the GPU
 * The file unique_id is used for this.
 * This will provide a Unique ID that will persist from machine to machine
 *
 * NOTE: This will only work for GFX9 and newer. This file will be absent
 * on unsupported ASICs (GFX8 and older)
 */
static ssize_t amdgpu_get_unique_id(struct device *dev,
		struct device_attribute *attr,
		char *buf)
{
	struct drm_device *ddev = dev_get_drvdata(dev);
	struct amdgpu_device *adev = ddev->dev_private;

	if (amdgpu_sriov_vf(adev) && !amdgpu_sriov_is_pp_one_vf(adev))
		return 0;

	if (adev->unique_id)
		return snprintf(buf, PAGE_SIZE, "%016llx\n", adev->unique_id);

	return 0;
}

/**
 * DOC: gpu_metrics
 *
 * The amdgpu driver provides a sysfs API for retrieving current gpu
 * metrics data. The file gpu_metrics is used for this. Reading the
 * file will dump all the current gpu metrics data.
 *
 * These data include temperature, frequency, engines utilization,
 * power consume, throttler status, fan speed and cpu core statistics(
 * available for APU only). That's it will give a snapshot of all sensors
 * at the same time.
 */
static ssize_t gpu_metrics_show(struct device *dev,
				      struct device_attribute *attr,
				      char *buf)
{
	struct drm_device *ddev = dev_get_drvdata(dev);
	struct amdgpu_device *adev = ddev->dev_private;
	void *gpu_metrics;
	ssize_t size = 0;
	int ret;

	if (adev->in_gpu_reset)
		return -EPERM;

	if (adev->in_suspend)
		return -EPERM;

	ret = pm_runtime_get_sync(ddev->dev);
	if (ret < 0) {
		pm_runtime_put_autosuspend(ddev->dev);
		return ret;
	}

	if (is_support_sw_smu(adev))
		size = smu_get_gpu_metrics(&adev->smu, &gpu_metrics);

	if (size <= 0)
		goto out;

	if (size >= PAGE_SIZE)
		size = PAGE_SIZE - 1;

	memcpy(buf, gpu_metrics, size);

out:
	pm_runtime_mark_last_busy(ddev->dev);
	pm_runtime_put_autosuspend(ddev->dev);

	return size;
}

static DEVICE_ATTR_WO(pp_dpm_max_cclk);
static DEVICE_ATTR_WO(pp_dpm_max_sclk);
static DEVICE_ATTR_WO(pp_dpm_min_cclk);
static DEVICE_ATTR_WO(pp_dpm_min_sclk);
static DEVICE_ATTR(pp_dpm_cclk_policy, S_IWUSR,
		   NULL,
		   amdgpu_set_pp_cclk_policy);
static DEVICE_ATTR(pp_dpm_df_active_level, 0644,
		   amdgpu_get_pp_df_active_level,
		   amdgpu_set_pp_df_active_level);
static DEVICE_ATTR(pp_dpm_df_softmin_level, 0644,
		   amdgpu_get_pp_df_softmin_level,
		   amdgpu_set_pp_df_softmin_level);
static DEVICE_ATTR(power_dpm_state, S_IRUGO | S_IWUSR, amdgpu_get_dpm_state, amdgpu_set_dpm_state);
static DEVICE_ATTR(power_dpm_force_performance_level, S_IRUGO | S_IWUSR,
		   amdgpu_get_dpm_forced_performance_level,
		   amdgpu_set_dpm_forced_performance_level);
static DEVICE_ATTR(pp_num_states, S_IRUGO, amdgpu_get_pp_num_states, NULL);
static DEVICE_ATTR(pp_cur_state, S_IRUGO, amdgpu_get_pp_cur_state, NULL);
static DEVICE_ATTR(pp_force_state, S_IRUGO | S_IWUSR,
		amdgpu_get_pp_force_state,
		amdgpu_set_pp_force_state);
static DEVICE_ATTR(pp_table, S_IRUGO | S_IWUSR,
		amdgpu_get_pp_table,
		amdgpu_set_pp_table);
static DEVICE_ATTR(pp_dpm_sclk, S_IRUGO | S_IWUSR,
		amdgpu_get_pp_dpm_sclk,
		amdgpu_set_pp_dpm_sclk);
static DEVICE_ATTR(pp_dpm_mclk, S_IRUGO | S_IWUSR,
		amdgpu_get_pp_dpm_mclk,
		amdgpu_set_pp_dpm_mclk);
static DEVICE_ATTR(pp_dpm_socclk, S_IRUGO | S_IWUSR,
		amdgpu_get_pp_dpm_socclk,
		amdgpu_set_pp_dpm_socclk);
static DEVICE_ATTR_RW(pp_dpm_vclk);
static DEVICE_ATTR_RW(pp_dpm_dclk);
static DEVICE_ATTR(pp_dpm_fclk, S_IRUGO | S_IWUSR,
		amdgpu_get_pp_dpm_fclk,
		amdgpu_set_pp_dpm_fclk);
static DEVICE_ATTR_WO(fclk_dpm_activity_monitor);
static DEVICE_ATTR(fclk_dpm_param, 0644,
		amdgpu_get_pp_fclk_dpm_params,
		amdgpu_set_pp_fclk_dpm_params);
static DEVICE_ATTR(pp_dpm_tuning_fclk, 0644,
		amdgpu_get_pp_dpm_tuning_fclk,
		amdgpu_set_pp_dpm_tuning_fclk);
static DEVICE_ATTR_RW(lclk_busy_calc_from_iohc);
static DEVICE_ATTR_RW(pp_dpm_tuning_lclk);
static DEVICE_ATTR(pp_dpm_dcefclk, S_IRUGO | S_IWUSR,
		amdgpu_get_pp_dpm_dcefclk,
		amdgpu_set_pp_dpm_dcefclk);
static DEVICE_ATTR(pp_dpm_pcie, S_IRUGO | S_IWUSR,
		amdgpu_get_pp_dpm_pcie,
		amdgpu_set_pp_dpm_pcie);
static DEVICE_ATTR(pp_sclk_od, S_IRUGO | S_IWUSR,
		amdgpu_get_pp_sclk_od,
		amdgpu_set_pp_sclk_od);
static DEVICE_ATTR(pp_mclk_od, S_IRUGO | S_IWUSR,
		amdgpu_get_pp_mclk_od,
		amdgpu_set_pp_mclk_od);
static DEVICE_ATTR(pp_power_profile_mode, S_IRUGO | S_IWUSR,
		amdgpu_get_pp_power_profile_mode,
		amdgpu_set_pp_power_profile_mode);
static DEVICE_ATTR(pp_od_clk_voltage, S_IRUGO | S_IWUSR,
		amdgpu_get_pp_od_clk_voltage,
		amdgpu_set_pp_od_clk_voltage);
static DEVICE_ATTR(gpu_busy_percent, S_IRUGO,
		amdgpu_get_busy_percent, NULL);
static DEVICE_ATTR(mem_busy_percent, S_IRUGO,
		amdgpu_get_memory_busy_percent, NULL);
static DEVICE_ATTR(pcie_bw, S_IRUGO, amdgpu_get_pcie_bw, NULL);
static DEVICE_ATTR(pp_features, S_IRUGO | S_IWUSR,
		amdgpu_get_pp_feature_status,
		amdgpu_set_pp_feature_status);
static DEVICE_ATTR(unique_id, S_IRUGO, amdgpu_get_unique_id, NULL);
static DEVICE_ATTR_RO(gpu_metrics);

static ssize_t amdgpu_hwmon_show_temp(struct device *dev,
				      struct device_attribute *attr,
				      char *buf)
{
	struct amdgpu_device *adev = dev_get_drvdata(dev);
	int channel = to_sensor_dev_attr(attr)->index;
	int r, temp = 0, size = sizeof(temp);

	if (channel >= PP_TEMP_MAX)
		return -EINVAL;

	r = pm_runtime_get_sync(adev->ddev->dev);
	if (r < 0)
		return r;

	switch (channel) {
	case PP_TEMP_JUNCTION:
		/* get current junction temperature */
		r = amdgpu_dpm_read_sensor(adev, AMDGPU_PP_SENSOR_HOTSPOT_TEMP,
					   (void *)&temp, &size);
		break;
	case PP_TEMP_EDGE:
		/* get current edge temperature */
		r = amdgpu_dpm_read_sensor(adev, AMDGPU_PP_SENSOR_EDGE_TEMP,
					   (void *)&temp, &size);
		break;
	case PP_TEMP_MEM:
		/* get current memory temperature */
		r = amdgpu_dpm_read_sensor(adev, AMDGPU_PP_SENSOR_MEM_TEMP,
					   (void *)&temp, &size);
		break;
	default:
		r = -EINVAL;
		break;
	}

	pm_runtime_mark_last_busy(adev->ddev->dev);
	pm_runtime_put_autosuspend(adev->ddev->dev);

	if (r)
		return r;

	return snprintf(buf, PAGE_SIZE, "%d\n", temp);
}

static ssize_t amdgpu_hwmon_show_temp_thresh(struct device *dev,
					     struct device_attribute *attr,
					     char *buf)
{
	struct amdgpu_device *adev = dev_get_drvdata(dev);
	int hyst = to_sensor_dev_attr(attr)->index;
	int temp;

	if (hyst)
		temp = adev->pm.dpm.thermal.min_temp;
	else
		temp = adev->pm.dpm.thermal.max_temp;

	return snprintf(buf, PAGE_SIZE, "%d\n", temp);
}

static ssize_t amdgpu_hwmon_show_hotspot_temp_thresh(struct device *dev,
					     struct device_attribute *attr,
					     char *buf)
{
	struct amdgpu_device *adev = dev_get_drvdata(dev);
	int hyst = to_sensor_dev_attr(attr)->index;
	int temp;

	if (hyst)
		temp = adev->pm.dpm.thermal.min_hotspot_temp;
	else
		temp = adev->pm.dpm.thermal.max_hotspot_crit_temp;

	return snprintf(buf, PAGE_SIZE, "%d\n", temp);
}

static ssize_t amdgpu_hwmon_show_mem_temp_thresh(struct device *dev,
					     struct device_attribute *attr,
					     char *buf)
{
	struct amdgpu_device *adev = dev_get_drvdata(dev);
	int hyst = to_sensor_dev_attr(attr)->index;
	int temp;

	if (hyst)
		temp = adev->pm.dpm.thermal.min_mem_temp;
	else
		temp = adev->pm.dpm.thermal.max_mem_crit_temp;

	return snprintf(buf, PAGE_SIZE, "%d\n", temp);
}

static ssize_t amdgpu_hwmon_show_temp_label(struct device *dev,
					     struct device_attribute *attr,
					     char *buf)
{
	int channel = to_sensor_dev_attr(attr)->index;

	if (channel >= PP_TEMP_MAX)
		return -EINVAL;

	return snprintf(buf, PAGE_SIZE, "%s\n", temp_label[channel].label);
}

static ssize_t amdgpu_hwmon_show_temp_emergency(struct device *dev,
					     struct device_attribute *attr,
					     char *buf)
{
	struct amdgpu_device *adev = dev_get_drvdata(dev);
	int channel = to_sensor_dev_attr(attr)->index;
	int temp = 0;

	if (channel >= PP_TEMP_MAX)
		return -EINVAL;

	switch (channel) {
	case PP_TEMP_JUNCTION:
		temp = adev->pm.dpm.thermal.max_hotspot_emergency_temp;
		break;
	case PP_TEMP_EDGE:
		temp = adev->pm.dpm.thermal.max_edge_emergency_temp;
		break;
	case PP_TEMP_MEM:
		temp = adev->pm.dpm.thermal.max_mem_emergency_temp;
		break;
	}

	return snprintf(buf, PAGE_SIZE, "%d\n", temp);
}

static ssize_t amdgpu_hwmon_get_pwm1_enable(struct device *dev,
					    struct device_attribute *attr,
					    char *buf)
{
	struct amdgpu_device *adev = dev_get_drvdata(dev);
	u32 pwm_mode = 0;
	int ret;

	ret = pm_runtime_get_sync(adev->ddev->dev);
	if (ret < 0)
		return ret;

	if (is_support_sw_smu(adev)) {
		pwm_mode = smu_get_fan_control_mode(&adev->smu);
	} else {
		if (!adev->powerplay.pp_funcs->get_fan_control_mode) {
			pm_runtime_mark_last_busy(adev->ddev->dev);
			pm_runtime_put_autosuspend(adev->ddev->dev);
			return -EINVAL;
		}

		pwm_mode = amdgpu_dpm_get_fan_control_mode(adev);
	}

	pm_runtime_mark_last_busy(adev->ddev->dev);
	pm_runtime_put_autosuspend(adev->ddev->dev);

	return sprintf(buf, "%i\n", pwm_mode);
}

static ssize_t amdgpu_hwmon_set_pwm1_enable(struct device *dev,
					    struct device_attribute *attr,
					    const char *buf,
					    size_t count)
{
	struct amdgpu_device *adev = dev_get_drvdata(dev);
	int err, ret;
	int value;

	err = kstrtoint(buf, 10, &value);
	if (err)
		return err;

	ret = pm_runtime_get_sync(adev->ddev->dev);
	if (ret < 0)
		return ret;

	if (is_support_sw_smu(adev)) {
		smu_set_fan_control_mode(&adev->smu, value);
	} else {
		if (!adev->powerplay.pp_funcs->set_fan_control_mode) {
			pm_runtime_mark_last_busy(adev->ddev->dev);
			pm_runtime_put_autosuspend(adev->ddev->dev);
			return -EINVAL;
		}

		amdgpu_dpm_set_fan_control_mode(adev, value);
	}

	pm_runtime_mark_last_busy(adev->ddev->dev);
	pm_runtime_put_autosuspend(adev->ddev->dev);

	return count;
}

static ssize_t amdgpu_hwmon_get_pwm1_min(struct device *dev,
					 struct device_attribute *attr,
					 char *buf)
{
	return sprintf(buf, "%i\n", 0);
}

static ssize_t amdgpu_hwmon_get_pwm1_max(struct device *dev,
					 struct device_attribute *attr,
					 char *buf)
{
	return sprintf(buf, "%i\n", 255);
}

static ssize_t amdgpu_hwmon_set_pwm1(struct device *dev,
				     struct device_attribute *attr,
				     const char *buf, size_t count)
{
	struct amdgpu_device *adev = dev_get_drvdata(dev);
	int err;
	u32 value;
	u32 pwm_mode;

	err = pm_runtime_get_sync(adev->ddev->dev);
	if (err < 0)
		return err;

	if (is_support_sw_smu(adev))
		pwm_mode = smu_get_fan_control_mode(&adev->smu);
	else
		pwm_mode = amdgpu_dpm_get_fan_control_mode(adev);

	if (pwm_mode != AMD_FAN_CTRL_MANUAL) {
		pr_info("manual fan speed control should be enabled first\n");
		pm_runtime_mark_last_busy(adev->ddev->dev);
		pm_runtime_put_autosuspend(adev->ddev->dev);
		return -EINVAL;
	}

	err = kstrtou32(buf, 10, &value);
	if (err) {
		pm_runtime_mark_last_busy(adev->ddev->dev);
		pm_runtime_put_autosuspend(adev->ddev->dev);
		return err;
	}

	value = (value * 100) / 255;

	if (is_support_sw_smu(adev))
		err = smu_set_fan_speed_percent(&adev->smu, value);
	else if (adev->powerplay.pp_funcs->set_fan_speed_percent)
		err = amdgpu_dpm_set_fan_speed_percent(adev, value);
	else
		err = -EINVAL;

	pm_runtime_mark_last_busy(adev->ddev->dev);
	pm_runtime_put_autosuspend(adev->ddev->dev);

	if (err)
		return err;

	return count;
}

static ssize_t amdgpu_hwmon_get_pwm1(struct device *dev,
				     struct device_attribute *attr,
				     char *buf)
{
	struct amdgpu_device *adev = dev_get_drvdata(dev);
	int err;
	u32 speed = 0;

	err = pm_runtime_get_sync(adev->ddev->dev);
	if (err < 0)
		return err;

	if (is_support_sw_smu(adev))
		err = smu_get_fan_speed_percent(&adev->smu, &speed);
	else if (adev->powerplay.pp_funcs->get_fan_speed_percent)
		err = amdgpu_dpm_get_fan_speed_percent(adev, &speed);
	else
		err = -EINVAL;

	pm_runtime_mark_last_busy(adev->ddev->dev);
	pm_runtime_put_autosuspend(adev->ddev->dev);

	if (err)
		return err;

	speed = (speed * 255) / 100;

	return sprintf(buf, "%i\n", speed);
}

static ssize_t amdgpu_hwmon_get_fan1_input(struct device *dev,
					   struct device_attribute *attr,
					   char *buf)
{
	struct amdgpu_device *adev = dev_get_drvdata(dev);
	int err;
	u32 speed = 0;

	err = pm_runtime_get_sync(adev->ddev->dev);
	if (err < 0)
		return err;

	if (is_support_sw_smu(adev))
		err = smu_get_fan_speed_rpm(&adev->smu, &speed);
	else if (adev->powerplay.pp_funcs->get_fan_speed_rpm)
		err = amdgpu_dpm_get_fan_speed_rpm(adev, &speed);
	else
		err = -EINVAL;

	pm_runtime_mark_last_busy(adev->ddev->dev);
	pm_runtime_put_autosuspend(adev->ddev->dev);

	if (err)
		return err;

	return sprintf(buf, "%i\n", speed);
}

static ssize_t amdgpu_hwmon_get_fan1_min(struct device *dev,
					 struct device_attribute *attr,
					 char *buf)
{
	struct amdgpu_device *adev = dev_get_drvdata(dev);
	u32 min_rpm = 0;
	u32 size = sizeof(min_rpm);
	int r;

	r = pm_runtime_get_sync(adev->ddev->dev);
	if (r < 0)
		return r;

	r = amdgpu_dpm_read_sensor(adev, AMDGPU_PP_SENSOR_MIN_FAN_RPM,
				   (void *)&min_rpm, &size);

	pm_runtime_mark_last_busy(adev->ddev->dev);
	pm_runtime_put_autosuspend(adev->ddev->dev);

	if (r)
		return r;

	return snprintf(buf, PAGE_SIZE, "%d\n", min_rpm);
}

static ssize_t amdgpu_hwmon_get_fan1_max(struct device *dev,
					 struct device_attribute *attr,
					 char *buf)
{
	struct amdgpu_device *adev = dev_get_drvdata(dev);
	u32 max_rpm = 0;
	u32 size = sizeof(max_rpm);
	int r;

	r = pm_runtime_get_sync(adev->ddev->dev);
	if (r < 0)
		return r;

	r = amdgpu_dpm_read_sensor(adev, AMDGPU_PP_SENSOR_MAX_FAN_RPM,
				   (void *)&max_rpm, &size);

	pm_runtime_mark_last_busy(adev->ddev->dev);
	pm_runtime_put_autosuspend(adev->ddev->dev);

	if (r)
		return r;

	return snprintf(buf, PAGE_SIZE, "%d\n", max_rpm);
}

static ssize_t amdgpu_hwmon_get_fan1_target(struct device *dev,
					   struct device_attribute *attr,
					   char *buf)
{
	struct amdgpu_device *adev = dev_get_drvdata(dev);
	int err;
	u32 rpm = 0;

	err = pm_runtime_get_sync(adev->ddev->dev);
	if (err < 0)
		return err;

	if (is_support_sw_smu(adev))
		err = smu_get_fan_speed_rpm(&adev->smu, &rpm);
	else if (adev->powerplay.pp_funcs->get_fan_speed_rpm)
		err = amdgpu_dpm_get_fan_speed_rpm(adev, &rpm);
	else
		err = -EINVAL;

	pm_runtime_mark_last_busy(adev->ddev->dev);
	pm_runtime_put_autosuspend(adev->ddev->dev);

	if (err)
		return err;

	return sprintf(buf, "%i\n", rpm);
}

static ssize_t amdgpu_hwmon_set_fan1_target(struct device *dev,
				     struct device_attribute *attr,
				     const char *buf, size_t count)
{
	struct amdgpu_device *adev = dev_get_drvdata(dev);
	int err;
	u32 value;
	u32 pwm_mode;

	err = pm_runtime_get_sync(adev->ddev->dev);
	if (err < 0)
		return err;

	if (is_support_sw_smu(adev))
		pwm_mode = smu_get_fan_control_mode(&adev->smu);
	else
		pwm_mode = amdgpu_dpm_get_fan_control_mode(adev);

	if (pwm_mode != AMD_FAN_CTRL_MANUAL) {
		pm_runtime_mark_last_busy(adev->ddev->dev);
		pm_runtime_put_autosuspend(adev->ddev->dev);
		return -ENODATA;
	}

	err = kstrtou32(buf, 10, &value);
	if (err) {
		pm_runtime_mark_last_busy(adev->ddev->dev);
		pm_runtime_put_autosuspend(adev->ddev->dev);
		return err;
	}

	if (is_support_sw_smu(adev))
		err = smu_set_fan_speed_rpm(&adev->smu, value);
	else if (adev->powerplay.pp_funcs->set_fan_speed_rpm)
		err = amdgpu_dpm_set_fan_speed_rpm(adev, value);
	else
		err = -EINVAL;

	pm_runtime_mark_last_busy(adev->ddev->dev);
	pm_runtime_put_autosuspend(adev->ddev->dev);

	if (err)
		return err;

	return count;
}

static ssize_t amdgpu_hwmon_get_fan1_enable(struct device *dev,
					    struct device_attribute *attr,
					    char *buf)
{
	struct amdgpu_device *adev = dev_get_drvdata(dev);
	u32 pwm_mode = 0;
	int ret;

	ret = pm_runtime_get_sync(adev->ddev->dev);
	if (ret < 0)
		return ret;

	if (is_support_sw_smu(adev)) {
		pwm_mode = smu_get_fan_control_mode(&adev->smu);
	} else {
		if (!adev->powerplay.pp_funcs->get_fan_control_mode) {
			pm_runtime_mark_last_busy(adev->ddev->dev);
			pm_runtime_put_autosuspend(adev->ddev->dev);
			return -EINVAL;
		}

		pwm_mode = amdgpu_dpm_get_fan_control_mode(adev);
	}

	pm_runtime_mark_last_busy(adev->ddev->dev);
	pm_runtime_put_autosuspend(adev->ddev->dev);

	return sprintf(buf, "%i\n", pwm_mode == AMD_FAN_CTRL_AUTO ? 0 : 1);
}

static ssize_t amdgpu_hwmon_set_fan1_enable(struct device *dev,
					    struct device_attribute *attr,
					    const char *buf,
					    size_t count)
{
	struct amdgpu_device *adev = dev_get_drvdata(dev);
	int err;
	int value;
	u32 pwm_mode;

	err = kstrtoint(buf, 10, &value);
	if (err)
		return err;

	if (value == 0)
		pwm_mode = AMD_FAN_CTRL_AUTO;
	else if (value == 1)
		pwm_mode = AMD_FAN_CTRL_MANUAL;
	else
		return -EINVAL;

	err = pm_runtime_get_sync(adev->ddev->dev);
	if (err < 0)
		return err;

	if (is_support_sw_smu(adev)) {
		smu_set_fan_control_mode(&adev->smu, pwm_mode);
	} else {
		if (!adev->powerplay.pp_funcs->set_fan_control_mode) {
			pm_runtime_mark_last_busy(adev->ddev->dev);
			pm_runtime_put_autosuspend(adev->ddev->dev);
			return -EINVAL;
		}
		amdgpu_dpm_set_fan_control_mode(adev, pwm_mode);
	}

	pm_runtime_mark_last_busy(adev->ddev->dev);
	pm_runtime_put_autosuspend(adev->ddev->dev);

	return count;
}

static ssize_t amdgpu_hwmon_show_vddgfx(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	struct amdgpu_device *adev = dev_get_drvdata(dev);
	u32 vddgfx;
	int r, size = sizeof(vddgfx);

	r = pm_runtime_get_sync(adev->ddev->dev);
	if (r < 0)
		return r;

	/* get the voltage */
	r = amdgpu_dpm_read_sensor(adev, AMDGPU_PP_SENSOR_VDDGFX,
				   (void *)&vddgfx, &size);

	pm_runtime_mark_last_busy(adev->ddev->dev);
	pm_runtime_put_autosuspend(adev->ddev->dev);

	if (r)
		return r;

	return snprintf(buf, PAGE_SIZE, "%d\n", vddgfx);
}

static ssize_t amdgpu_hwmon_show_vddgfx_label(struct device *dev,
					      struct device_attribute *attr,
					      char *buf)
{
	return snprintf(buf, PAGE_SIZE, "vddgfx\n");
}

static ssize_t amdgpu_hwmon_show_vddnb(struct device *dev,
				       struct device_attribute *attr,
				       char *buf)
{
	struct amdgpu_device *adev = dev_get_drvdata(dev);
	u32 vddnb;
	int r, size = sizeof(vddnb);

	/* only APUs have vddnb */
	if  (!(adev->flags & AMD_IS_APU))
		return -EINVAL;

	r = pm_runtime_get_sync(adev->ddev->dev);
	if (r < 0)
		return r;

	/* get the voltage */
	r = amdgpu_dpm_read_sensor(adev, AMDGPU_PP_SENSOR_VDDNB,
				   (void *)&vddnb, &size);

	pm_runtime_mark_last_busy(adev->ddev->dev);
	pm_runtime_put_autosuspend(adev->ddev->dev);

	if (r)
		return r;

	return snprintf(buf, PAGE_SIZE, "%d\n", vddnb);
}

static ssize_t amdgpu_hwmon_show_vddnb_label(struct device *dev,
					      struct device_attribute *attr,
					      char *buf)
{
	return snprintf(buf, PAGE_SIZE, "vddnb\n");
}

static ssize_t amdgpu_hwmon_show_power_avg(struct device *dev,
					   struct device_attribute *attr,
					   char *buf)
{
	struct amdgpu_device *adev = dev_get_drvdata(dev);
	u32 query = 0;
	int r, size = sizeof(u32);
	unsigned uw;

	r = pm_runtime_get_sync(adev->ddev->dev);
	if (r < 0)
		return r;

	/* get the voltage */
	r = amdgpu_dpm_read_sensor(adev, AMDGPU_PP_SENSOR_GPU_POWER,
				   (void *)&query, &size);

	pm_runtime_mark_last_busy(adev->ddev->dev);
	pm_runtime_put_autosuspend(adev->ddev->dev);

	if (r)
		return r;

	/* convert to microwatts */
	if (adev->asic_type == CHIP_MERO)
		uw = query * 1000;
	else
		uw = (query >> 8) * 1000000 + (query & 0xff) * 1000;

	return snprintf(buf, PAGE_SIZE, "%u\n", uw);
}

static ssize_t amdgpu_hwmon_show_power_cap_min(struct device *dev,
					 struct device_attribute *attr,
					 char *buf)
{
	return sprintf(buf, "%i\n", 0);
}

static ssize_t amdgpu_hwmon_show_power_cap_max(struct device *dev,
					 struct device_attribute *attr,
					 char *buf)
{
	struct amdgpu_device *adev = dev_get_drvdata(dev);
	uint32_t limit = 0;
	ssize_t size;
	int r;

	r = pm_runtime_get_sync(adev->ddev->dev);
	if (r < 0)
		return r;

	if (is_support_sw_smu(adev)) {
		smu_get_power_limit(&adev->smu, &limit, true, true);
		size = snprintf(buf, PAGE_SIZE, "%u\n", limit * 1000000);
	} else if (adev->powerplay.pp_funcs && adev->powerplay.pp_funcs->get_power_limit) {
		adev->powerplay.pp_funcs->get_power_limit(adev->powerplay.pp_handle, &limit, true);
		size = snprintf(buf, PAGE_SIZE, "%u\n", limit * 1000000);
	} else {
		size = snprintf(buf, PAGE_SIZE, "\n");
	}

	pm_runtime_mark_last_busy(adev->ddev->dev);
	pm_runtime_put_autosuspend(adev->ddev->dev);

	return size;
}

static ssize_t amdgpu_hwmon_power_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)

{
	struct amdgpu_device *adev = dev_get_drvdata(dev);
	u32 uw, query = 0;
	int r, size = sizeof(u32);
	struct sensor_device_attribute_2 *sattr = to_sensor_dev_attr_2(attr);

	if (unlikely(sattr->index != CUR_VAL && sattr->index != AVE_VAL)) {
		DRM_ERROR("invalid value type %d\n", sattr->index);
		return -EINVAL;
	}

	r = pm_runtime_get_sync(adev->ddev->dev);
	if (r < 0)
		return r;

	switch (sattr->nr) {
	case POWER_CPU:
		if (sattr->index == CUR_VAL)
			r = amdgpu_dpm_read_sensor(adev,
				AMDGPU_PP_SENSOR_CPU_POWER_CUR,
				(void *)&query, &size);
		else
			r = amdgpu_dpm_read_sensor(adev,
				AMDGPU_PP_SENSOR_CPU_POWER_AVE,
				(void *)&query, &size);
		break;
	case POWER_GFX:
		if (sattr->index == CUR_VAL)
			r = amdgpu_dpm_read_sensor(adev,
				AMDGPU_PP_SENSOR_GFX_POWER_CUR,
				(void *)&query, &size);
		else
			r = amdgpu_dpm_read_sensor(adev,
				AMDGPU_PP_SENSOR_GFX_POWER_AVE,
				(void *)&query, &size);
		break;
	case POWER_SOC:
		if (sattr->index == CUR_VAL)
			r = amdgpu_dpm_read_sensor(adev,
				AMDGPU_PP_SENSOR_SOC_POWER_CUR,
				(void *)&query, &size);
		else
			r = amdgpu_dpm_read_sensor(adev,
				AMDGPU_PP_SENSOR_SOC_POWER_AVE,
				(void *)&query, &size);
		break;
	case POWER_CVIP:
		if (sattr->index == CUR_VAL)
			r = amdgpu_dpm_read_sensor(adev,
				AMDGPU_PP_SENSOR_CVIP_POWER_CUR,
				(void *)&query, &size);
		else
			r = amdgpu_dpm_read_sensor(adev,
				AMDGPU_PP_SENSOR_CVIP_POWER_AVE,
				(void *)&query, &size);
		break;
	default:
		DRM_ERROR("invalid power id %d.\n", sattr->nr);
		r = -EINVAL;
	}

	pm_runtime_mark_last_busy(adev->ddev->dev);
	pm_runtime_put_autosuspend(adev->ddev->dev);
	/* Only mero or later support this, other asics will fail  */
	if (r)
		return r;
	/* convert to microwatts */
	uw = query * 1000;
	return snprintf(buf, PAGE_SIZE, "%u\n", uw);
}

static ssize_t amdgpu_hwmon_voltage_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)

{
	struct amdgpu_device *adev = dev_get_drvdata(dev);
	u32 query = 0;
	int r, size = sizeof(u32);
	struct sensor_device_attribute_2 *sattr = to_sensor_dev_attr_2(attr);

	r = pm_runtime_get_sync(adev->ddev->dev);
	if (r < 0)
		return r;

	switch (sattr->nr) {
	case VOL_CPU0:
		r = amdgpu_dpm_read_sensor(adev,
				AMDGPU_PP_SENSOR_CPU0_VOL,
				(void *)&query, &size);
		break;
	case VOL_CPU1:
		r = amdgpu_dpm_read_sensor(adev,
				AMDGPU_PP_SENSOR_CPU1_VOL,
				(void *)&query, &size);
		break;
	case VOL_CPU2:
		r = amdgpu_dpm_read_sensor(adev,
				AMDGPU_PP_SENSOR_CPU2_VOL,
				(void *)&query, &size);
		break;
	case VOL_CPU3:
		r = amdgpu_dpm_read_sensor(adev,
				AMDGPU_PP_SENSOR_CPU3_VOL,
				(void *)&query, &size);
		break;
	case VOL_GFX:
		r = amdgpu_dpm_read_sensor(adev,
				AMDGPU_PP_SENSOR_GFX_VOL,
				(void *)&query, &size);
		break;
	case VOL_SOC:
		r = amdgpu_dpm_read_sensor(adev,
				AMDGPU_PP_SENSOR_SOC_VOL,
				(void *)&query, &size);
		break;
	case VOL_CVIP:
		r = amdgpu_dpm_read_sensor(adev,
				AMDGPU_PP_SENSOR_CVIP_VOL,
				(void *)&query, &size);
		break;
	default:
		DRM_ERROR("invalid voltage id %d.\n", sattr->nr);
		r = -EINVAL;
	}

	pm_runtime_mark_last_busy(adev->ddev->dev);
	pm_runtime_put_autosuspend(adev->ddev->dev);
	if (r)
		return r;
	return snprintf(buf, PAGE_SIZE, "%u\n", query);
}

static ssize_t amdgpu_hwmon_current_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)

{
	struct amdgpu_device *adev = dev_get_drvdata(dev);
	u32 query = 0;
	int r, size = sizeof(u32);
	struct sensor_device_attribute_2 *sattr = to_sensor_dev_attr_2(attr);

	r = pm_runtime_get_sync(adev->ddev->dev);
	if (r < 0)
		return r;

	switch (sattr->nr) {
	case CUR_CPU:
		r = amdgpu_dpm_read_sensor(adev,
			AMDGPU_PP_SENSOR_CPU_CUR,
			(void *)&query, &size);
		break;
	case CUR_GFX:
		r = amdgpu_dpm_read_sensor(adev,
			AMDGPU_PP_SENSOR_GFX_CUR,
			(void *)&query, &size);
		break;
	case CUR_SOC:
		r = amdgpu_dpm_read_sensor(adev,
			AMDGPU_PP_SENSOR_SOC_CUR,
			(void *)&query, &size);
		break;
	case CUR_CVIP:
		r = amdgpu_dpm_read_sensor(adev,
			AMDGPU_PP_SENSOR_CVIP_CUR,
			(void *)&query, &size);
		break;
	default:
		DRM_ERROR("invalid current id %d.\n", sattr->nr);
		r = -EINVAL;
	}

	pm_runtime_mark_last_busy(adev->ddev->dev);
	pm_runtime_put_autosuspend(adev->ddev->dev);
	if (r)
		return r;
	return snprintf(buf, PAGE_SIZE, "%u\n", query);
}

static ssize_t amdgpu_hwmon_temperature_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)

{
	struct amdgpu_device *adev = dev_get_drvdata(dev);
	int r, query = 0, size = sizeof(query);
	struct sensor_device_attribute_2 *sattr = to_sensor_dev_attr_2(attr);

	if (unlikely(sattr->index != CUR_VAL && sattr->index != AVE_VAL)) {
		DRM_ERROR("invalid value type %d\n", sattr->index);
		return -EINVAL;
	}

	r = pm_runtime_get_sync(adev->ddev->dev);
	if (r < 0)
		return r;

	switch (sattr->nr) {
	case TEM_CPU0:
		if (sattr->index == CUR_VAL)
			r = amdgpu_dpm_read_sensor(adev,
				AMDGPU_PP_SENSOR_CPU0_TEMP_CUR,
				(void *)&query, &size);
		else
			r = amdgpu_dpm_read_sensor(adev,
				AMDGPU_PP_SENSOR_CPU0_TEMP_AVE,
				(void *)&query, &size);
		break;
	case TEM_CPU1:
		if (sattr->index == CUR_VAL)
			r = amdgpu_dpm_read_sensor(adev,
				AMDGPU_PP_SENSOR_CPU1_TEMP_CUR,
				(void *)&query, &size);
		else
			r = amdgpu_dpm_read_sensor(adev,
				AMDGPU_PP_SENSOR_CPU1_TEMP_AVE,
				(void *)&query, &size);
		break;
	case TEM_CPU2:
		if (sattr->index == CUR_VAL)
			r = amdgpu_dpm_read_sensor(adev,
				AMDGPU_PP_SENSOR_CPU2_TEMP_CUR,
				(void *)&query, &size);
		else
			r = amdgpu_dpm_read_sensor(adev,
				AMDGPU_PP_SENSOR_CPU2_TEMP_AVE,
				(void *)&query, &size);
		break;
	case TEM_CPU3:
		if (sattr->index == CUR_VAL)
			r = amdgpu_dpm_read_sensor(adev,
				AMDGPU_PP_SENSOR_CPU3_TEMP_CUR,
				(void *)&query, &size);
		else
			r = amdgpu_dpm_read_sensor(adev,
				AMDGPU_PP_SENSOR_CPU3_TEMP_AVE,
				(void *)&query, &size);
		break;
	case TEM_GFX:
		if (sattr->index == CUR_VAL)
			r = amdgpu_dpm_read_sensor(adev,
				AMDGPU_PP_SENSOR_GFX_TEMP_CUR,
				(void *)&query, &size);
		else
			r = amdgpu_dpm_read_sensor(adev,
				AMDGPU_PP_SENSOR_GFX_TEMP_AVE,
				(void *)&query, &size);
		break;
	case TEM_SOC:
		if (sattr->index == CUR_VAL)
			r = amdgpu_dpm_read_sensor(adev,
				AMDGPU_PP_SENSOR_SOC_TEMP_CUR,
				(void *)&query, &size);
		else
			r = amdgpu_dpm_read_sensor(adev,
				AMDGPU_PP_SENSOR_SOC_TEMP_AVE,
				(void *)&query, &size);
		break;
	case TEM_CVIP:
		if (sattr->index == CUR_VAL)
			r = amdgpu_dpm_read_sensor(adev,
				AMDGPU_PP_SENSOR_CVIP_TEMP_CUR,
				(void *)&query, &size);
		else
			r = amdgpu_dpm_read_sensor(adev,
				AMDGPU_PP_SENSOR_CVIP_TEMP_AVE,
				(void *)&query, &size);
		break;
	default:
		DRM_ERROR("invalid temperature id %d.\n", sattr->nr);
		r = -EINVAL;
	}

	pm_runtime_mark_last_busy(adev->ddev->dev);
	pm_runtime_put_autosuspend(adev->ddev->dev);
	if (r)
		return r;
	return snprintf(buf, PAGE_SIZE, "%d\n", query);
}

static ssize_t amdgpu_hwmon_clock_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)

{
	struct amdgpu_device *adev = dev_get_drvdata(dev);
	u32 query = 0;
	int r, size = sizeof(u32);
	struct sensor_device_attribute_2 *sattr = to_sensor_dev_attr_2(attr);

	r = pm_runtime_get_sync(adev->ddev->dev);
	if (r < 0)
		return r;

	switch (sattr->nr) {
	case CLK_CPU0:
		r = amdgpu_dpm_read_sensor(adev,
				AMDGPU_PP_SENSOR_CPU0_CLK,
				(void *)&query, &size);
		break;
	case CLK_CPU1:
		r = amdgpu_dpm_read_sensor(adev,
				AMDGPU_PP_SENSOR_CPU1_CLK,
				(void *)&query, &size);
		break;
	case CLK_CPU2:
		r = amdgpu_dpm_read_sensor(adev,
				AMDGPU_PP_SENSOR_CPU2_CLK,
				(void *)&query, &size);
		break;
	case CLK_CPU3:
		r = amdgpu_dpm_read_sensor(adev,
				AMDGPU_PP_SENSOR_CPU3_CLK,
				(void *)&query, &size);
		break;
	case CLK_GFX:
		r = amdgpu_dpm_read_sensor(adev,
				AMDGPU_PP_SENSOR_GFX_CLK,
				(void *)&query, &size);
		break;
	default:
		DRM_ERROR("invalid clock id %d.\n", sattr->nr);
		r = -EINVAL;
	}

	pm_runtime_mark_last_busy(adev->ddev->dev);
	pm_runtime_put_autosuspend(adev->ddev->dev);
	if (r)
		return r;
	return snprintf(buf, PAGE_SIZE, "%u\n", query * 10 * 1000);
}

static ssize_t amdgpu_hwmon_time_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf,
		size_t count)
{
	struct amdgpu_device *adev = dev_get_drvdata(dev);
	int ret;
	u32 value;
	struct sensor_device_attribute_2 *sattr = to_sensor_dev_attr_2(attr);
	enum amd_pp_sensors sensor_id;

	if (amdgpu_sriov_vf(adev))
		return -EINVAL;

	ret = kstrtou32(buf, 10, &value);
	if (ret)
		return ret;

	switch (sattr->nr) {
	case POWER_TIME:
		sensor_id = AMDGPU_PP_SENSOR_POWER_TIME;
		break;
	case TEM_TIME:
		sensor_id = AMDGPU_PP_SENSOR_TEM_TIME;
		break;
	case ACT_TIME:
		sensor_id = AMDGPU_PP_SENSOR_ACT_TIME;
		break;
	case MIG_END_HYS:
		sensor_id = AMDGPU_PP_SENSOR_MIG_END_HYS;
		break;
	default:
		DRM_ERROR("invalid time id %d.\n", sattr->nr);
		ret = -EINVAL;
	}
	if (ret)
		return ret;

	ret = pm_runtime_get_sync(adev->ddev->dev);
	if (ret < 0)
		return ret;

	if (is_support_sw_smu(adev))
		ret = smu_set_time_limit(&adev->smu, sensor_id, value);
	else
		ret = -EINVAL;

	pm_runtime_mark_last_busy(adev->ddev->dev);
	pm_runtime_put_autosuspend(adev->ddev->dev);
	if (ret)
		return ret;

	return count;
}

static ssize_t amdgpu_hwmon_activity_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)

{
	struct amdgpu_device *adev = dev_get_drvdata(dev);
	u32 query = 0;
	int r, size = sizeof(u32);
	struct sensor_device_attribute_2 *sattr = to_sensor_dev_attr_2(attr);

	if (amdgpu_sriov_vf(adev))
		return 0;

	r = pm_runtime_get_sync(adev->ddev->dev);
	if (r < 0)
		return r;

	switch (sattr->nr) {
	case ACT_CPU:
		r = amdgpu_dpm_read_sensor(adev,
				AMDGPU_PP_SENSOR_CPU_LOAD,
				(void *)&query, &size);
		break;
	case ACT_GFX:
		r = amdgpu_dpm_read_sensor(adev,
				AMDGPU_PP_SENSOR_GPU_LOAD,
				(void *)&query, &size);
		break;
	default:
		DRM_ERROR("invalid clock id %d.\n", sattr->nr);
		r = -EINVAL;
	}

	pm_runtime_mark_last_busy(adev->ddev->dev);
	pm_runtime_put_autosuspend(adev->ddev->dev);
	if (r)
		return r;
	if (sattr->nr == ACT_CPU)
		return snprintf(buf, PAGE_SIZE, "0x%08x\n", query);
	else
		return snprintf(buf, PAGE_SIZE, "%u\n", query);
}

static ssize_t amdgpu_hwmon_thermal_limit_show(struct device *dev,
					 struct device_attribute *attr,
					 char *buf)
{
	struct amdgpu_device *adev = dev_get_drvdata(dev);
	uint32_t limit = 0;
	ssize_t size;
	int r;

	if (amdgpu_sriov_vf(adev))
		return 0;

	r = pm_runtime_get_sync(adev->ddev->dev);
	if (r < 0)
		return r;

	if (is_support_sw_smu(adev)) {
		smu_get_thermal_limit(&adev->smu, &limit, false,  true);
		size = snprintf(buf, PAGE_SIZE, "%u\n", limit);
	} else {
		size = snprintf(buf, PAGE_SIZE, "\n");
	}

	pm_runtime_mark_last_busy(adev->ddev->dev);
	pm_runtime_put_autosuspend(adev->ddev->dev);

	return size;
}

static ssize_t amdgpu_hwmon_thermal_limit_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf,
		size_t count)
{
	struct amdgpu_device *adev = dev_get_drvdata(dev);
	int err;
	u32 value;

	if (amdgpu_sriov_vf(adev))
		return -EINVAL;

	err = kstrtou32(buf, 10, &value);
	if (err)
		return err;

	err = pm_runtime_get_sync(adev->ddev->dev);
	if (err < 0)
		return err;

	if (is_support_sw_smu(adev))
		err = smu_set_thermal_limit(&adev->smu, value);
	else
		err = -EINVAL;

	pm_runtime_mark_last_busy(adev->ddev->dev);
	pm_runtime_put_autosuspend(adev->ddev->dev);

	if (err)
		return err;

	return count;
}



static ssize_t amdgpu_hwmon_show_power_cap(struct device *dev,
					 struct device_attribute *attr,
					 char *buf)
{
	struct amdgpu_device *adev = dev_get_drvdata(dev);
	uint32_t limit = 0;
	ssize_t size;
	int r;

	r = pm_runtime_get_sync(adev->ddev->dev);
	if (r < 0)
		return r;

	if (is_support_sw_smu(adev)) {
		smu_get_power_limit(&adev->smu, &limit, false,  true);
		size = snprintf(buf, PAGE_SIZE, "%u\n", limit * 1000000);
	} else if (adev->powerplay.pp_funcs && adev->powerplay.pp_funcs->get_power_limit) {
		adev->powerplay.pp_funcs->get_power_limit(adev->powerplay.pp_handle, &limit, false);
		size = snprintf(buf, PAGE_SIZE, "%u\n", limit * 1000000);
	} else {
		size = snprintf(buf, PAGE_SIZE, "\n");
	}

	pm_runtime_mark_last_busy(adev->ddev->dev);
	pm_runtime_put_autosuspend(adev->ddev->dev);

	return size;
}


static ssize_t amdgpu_hwmon_set_power_cap(struct device *dev,
		struct device_attribute *attr,
		const char *buf,
		size_t count)
{
	struct amdgpu_device *adev = dev_get_drvdata(dev);
	int err;
	u32 value;

	if (amdgpu_sriov_vf(adev))
		return -EINVAL;

	err = kstrtou32(buf, 10, &value);
	if (err)
		return err;

	value = value / 1000000; /* convert to Watt */


	err = pm_runtime_get_sync(adev->ddev->dev);
	if (err < 0)
		return err;

	if (is_support_sw_smu(adev))
		err = smu_set_power_limit(&adev->smu, value);
	else if (adev->powerplay.pp_funcs && adev->powerplay.pp_funcs->set_power_limit)
		err = adev->powerplay.pp_funcs->set_power_limit(adev->powerplay.pp_handle, value);
	else
		err = -EINVAL;

	pm_runtime_mark_last_busy(adev->ddev->dev);
	pm_runtime_put_autosuspend(adev->ddev->dev);

	if (err)
		return err;

	return count;
}

static ssize_t amdgpu_hwmon_show_sclk(struct device *dev,
				      struct device_attribute *attr,
				      char *buf)
{
	struct amdgpu_device *adev = dev_get_drvdata(dev);
	uint32_t sclk;
	int r, size = sizeof(sclk);

	r = pm_runtime_get_sync(adev->ddev->dev);
	if (r < 0)
		return r;

	/* get the sclk */
	r = amdgpu_dpm_read_sensor(adev, AMDGPU_PP_SENSOR_GFX_SCLK,
				   (void *)&sclk, &size);

	pm_runtime_mark_last_busy(adev->ddev->dev);
	pm_runtime_put_autosuspend(adev->ddev->dev);

	if (r)
		return r;

	return snprintf(buf, PAGE_SIZE, "%d\n", sclk * 10 * 1000);
}

static ssize_t amdgpu_hwmon_show_sclk_label(struct device *dev,
					    struct device_attribute *attr,
					    char *buf)
{
	return snprintf(buf, PAGE_SIZE, "sclk\n");
}

static ssize_t amdgpu_hwmon_show_mclk(struct device *dev,
				      struct device_attribute *attr,
				      char *buf)
{
	struct amdgpu_device *adev = dev_get_drvdata(dev);
	uint32_t mclk;
	int r, size = sizeof(mclk);

	r = pm_runtime_get_sync(adev->ddev->dev);
	if (r < 0)
		return r;

	/* get the sclk */
	r = amdgpu_dpm_read_sensor(adev, AMDGPU_PP_SENSOR_GFX_MCLK,
				   (void *)&mclk, &size);

	pm_runtime_mark_last_busy(adev->ddev->dev);
	pm_runtime_put_autosuspend(adev->ddev->dev);

	if (r)
		return r;

	return snprintf(buf, PAGE_SIZE, "%d\n", mclk * 10 * 1000);
}

static ssize_t amdgpu_hwmon_show_mclk_label(struct device *dev,
					    struct device_attribute *attr,
					    char *buf)
{
	return snprintf(buf, PAGE_SIZE, "mclk\n");
}

/**
 * DOC: hwmon
 *
 * The amdgpu driver exposes the following sensor interfaces:
 *
 * - GPU temperature (via the on-die sensor)
 *
 * - GPU voltage
 *
 * - Northbridge voltage (APUs only)
 *
 * - GPU power
 *
 * - GPU fan
 *
 * - GPU gfx/compute engine clock
 *
 * - GPU memory clock (dGPU only)
 *
 * hwmon interfaces for GPU temperature:
 *
 * - temp[1-3]_input: the on die GPU temperature in millidegrees Celsius
 *   - temp2_input and temp3_input are supported on SOC15 dGPUs only
 *
 * - temp[1-3]_label: temperature channel label
 *   - temp2_label and temp3_label are supported on SOC15 dGPUs only
 *
 * - temp[1-3]_crit: temperature critical max value in millidegrees Celsius
 *   - temp2_crit and temp3_crit are supported on SOC15 dGPUs only
 *
 * - temp[1-3]_crit_hyst: temperature hysteresis for critical limit in millidegrees Celsius
 *   - temp2_crit_hyst and temp3_crit_hyst are supported on SOC15 dGPUs only
 *
 * - temp[1-3]_emergency: temperature emergency max value(asic shutdown) in millidegrees Celsius
 *   - these are supported on SOC15 dGPUs only
 *
 * hwmon interfaces for GPU voltage:
 *
 * - in0_input: the voltage on the GPU in millivolts
 *
 * - in1_input: the voltage on the Northbridge in millivolts
 *
 * hwmon interfaces for GPU power:
 *
 * - power1_average: average power used by the GPU in microWatts
 *
 * - power1_cap_min: minimum cap supported in microWatts
 *
 * - power1_cap_max: maximum cap supported in microWatts
 *
 * - power1_cap: selected power cap in microWatts
 *
 * hwmon interfaces for GPU fan:
 *
 * - pwm1: pulse width modulation fan level (0-255)
 *
 * - pwm1_enable: pulse width modulation fan control method (0: no fan speed control, 1: manual fan speed control using pwm interface, 2: automatic fan speed control)
 *
 * - pwm1_min: pulse width modulation fan control minimum level (0)
 *
 * - pwm1_max: pulse width modulation fan control maximum level (255)
 *
 * - fan1_min: an minimum value Unit: revolution/min (RPM)
 *
 * - fan1_max: an maxmum value Unit: revolution/max (RPM)
 *
 * - fan1_input: fan speed in RPM
 *
 * - fan[1-\*]_target: Desired fan speed Unit: revolution/min (RPM)
 *
 * - fan[1-\*]_enable: Enable or disable the sensors.1: Enable 0: Disable
 *
 * hwmon interfaces for GPU clocks:
 *
 * - freq1_input: the gfx/compute clock in hertz
 *
 * - freq2_input: the memory clock in hertz
 *
 * You can use hwmon tools like sensors to view this information on your system.
 *
 */

static SENSOR_DEVICE_ATTR(temp1_input, S_IRUGO, amdgpu_hwmon_show_temp, NULL, PP_TEMP_EDGE);
static SENSOR_DEVICE_ATTR(temp1_crit, S_IRUGO, amdgpu_hwmon_show_temp_thresh, NULL, 0);
static SENSOR_DEVICE_ATTR(temp1_crit_hyst, S_IRUGO, amdgpu_hwmon_show_temp_thresh, NULL, 1);
static SENSOR_DEVICE_ATTR(temp1_emergency, S_IRUGO, amdgpu_hwmon_show_temp_emergency, NULL, PP_TEMP_EDGE);
static SENSOR_DEVICE_ATTR(temp2_input, S_IRUGO, amdgpu_hwmon_show_temp, NULL, PP_TEMP_JUNCTION);
static SENSOR_DEVICE_ATTR(temp2_crit, S_IRUGO, amdgpu_hwmon_show_hotspot_temp_thresh, NULL, 0);
static SENSOR_DEVICE_ATTR(temp2_crit_hyst, S_IRUGO, amdgpu_hwmon_show_hotspot_temp_thresh, NULL, 1);
static SENSOR_DEVICE_ATTR(temp2_emergency, S_IRUGO, amdgpu_hwmon_show_temp_emergency, NULL, PP_TEMP_JUNCTION);
static SENSOR_DEVICE_ATTR(temp3_input, S_IRUGO, amdgpu_hwmon_show_temp, NULL, PP_TEMP_MEM);
static SENSOR_DEVICE_ATTR(temp3_crit, S_IRUGO, amdgpu_hwmon_show_mem_temp_thresh, NULL, 0);
static SENSOR_DEVICE_ATTR(temp3_crit_hyst, S_IRUGO, amdgpu_hwmon_show_mem_temp_thresh, NULL, 1);
static SENSOR_DEVICE_ATTR(temp3_emergency, S_IRUGO, amdgpu_hwmon_show_temp_emergency, NULL, PP_TEMP_MEM);
static SENSOR_DEVICE_ATTR(temp1_label, S_IRUGO, amdgpu_hwmon_show_temp_label, NULL, PP_TEMP_EDGE);
static SENSOR_DEVICE_ATTR(temp2_label, S_IRUGO, amdgpu_hwmon_show_temp_label, NULL, PP_TEMP_JUNCTION);
static SENSOR_DEVICE_ATTR(temp3_label, S_IRUGO, amdgpu_hwmon_show_temp_label, NULL, PP_TEMP_MEM);
static SENSOR_DEVICE_ATTR(pwm1, S_IRUGO | S_IWUSR, amdgpu_hwmon_get_pwm1, amdgpu_hwmon_set_pwm1, 0);
static SENSOR_DEVICE_ATTR(pwm1_enable, S_IRUGO | S_IWUSR, amdgpu_hwmon_get_pwm1_enable, amdgpu_hwmon_set_pwm1_enable, 0);
static SENSOR_DEVICE_ATTR(pwm1_min, S_IRUGO, amdgpu_hwmon_get_pwm1_min, NULL, 0);
static SENSOR_DEVICE_ATTR(pwm1_max, S_IRUGO, amdgpu_hwmon_get_pwm1_max, NULL, 0);
static SENSOR_DEVICE_ATTR(fan1_input, S_IRUGO, amdgpu_hwmon_get_fan1_input, NULL, 0);
static SENSOR_DEVICE_ATTR(fan1_min, S_IRUGO, amdgpu_hwmon_get_fan1_min, NULL, 0);
static SENSOR_DEVICE_ATTR(fan1_max, S_IRUGO, amdgpu_hwmon_get_fan1_max, NULL, 0);
static SENSOR_DEVICE_ATTR(fan1_target, S_IRUGO | S_IWUSR, amdgpu_hwmon_get_fan1_target, amdgpu_hwmon_set_fan1_target, 0);
static SENSOR_DEVICE_ATTR(fan1_enable, S_IRUGO | S_IWUSR, amdgpu_hwmon_get_fan1_enable, amdgpu_hwmon_set_fan1_enable, 0);
static SENSOR_DEVICE_ATTR(in0_input, S_IRUGO, amdgpu_hwmon_show_vddgfx, NULL, 0);
static SENSOR_DEVICE_ATTR(in0_label, S_IRUGO, amdgpu_hwmon_show_vddgfx_label, NULL, 0);
static SENSOR_DEVICE_ATTR(in1_input, S_IRUGO, amdgpu_hwmon_show_vddnb, NULL, 0);
static SENSOR_DEVICE_ATTR(in1_label, S_IRUGO, amdgpu_hwmon_show_vddnb_label, NULL, 0);
static SENSOR_DEVICE_ATTR(power1_average, S_IRUGO, amdgpu_hwmon_show_power_avg, NULL, 0);
static SENSOR_DEVICE_ATTR(power1_cap_max, S_IRUGO, amdgpu_hwmon_show_power_cap_max, NULL, 0);
static SENSOR_DEVICE_ATTR(power1_cap_min, S_IRUGO, amdgpu_hwmon_show_power_cap_min, NULL, 0);
static SENSOR_DEVICE_ATTR(power1_cap, S_IRUGO | S_IWUSR, amdgpu_hwmon_show_power_cap, amdgpu_hwmon_set_power_cap, 0);
static SENSOR_DEVICE_ATTR(freq1_input, S_IRUGO, amdgpu_hwmon_show_sclk, NULL, 0);
static SENSOR_DEVICE_ATTR(freq1_label, S_IRUGO, amdgpu_hwmon_show_sclk_label, NULL, 0);
static SENSOR_DEVICE_ATTR(freq2_input, S_IRUGO, amdgpu_hwmon_show_mclk, NULL, 0);
static SENSOR_DEVICE_ATTR(freq2_label, S_IRUGO, amdgpu_hwmon_show_mclk_label, NULL, 0);

/* mero sepcific chagnes*/
static SENSOR_DEVICE_ATTR_2_RO(cur_pow_CPU, amdgpu_hwmon_power, POWER_CPU, CUR_VAL);
static SENSOR_DEVICE_ATTR_2_RO(ave_pow_CPU, amdgpu_hwmon_power, POWER_CPU, AVE_VAL);
static SENSOR_DEVICE_ATTR_2_RO(cur_pow_GFX, amdgpu_hwmon_power, POWER_GFX, CUR_VAL);
static SENSOR_DEVICE_ATTR_2_RO(ave_pow_GFX, amdgpu_hwmon_power, POWER_GFX, AVE_VAL);
static SENSOR_DEVICE_ATTR_2_RO(cur_pow_SOC, amdgpu_hwmon_power, POWER_SOC, CUR_VAL);
static SENSOR_DEVICE_ATTR_2_RO(ave_pow_SOC, amdgpu_hwmon_power, POWER_SOC, AVE_VAL);
static SENSOR_DEVICE_ATTR_2_RO(cur_pow_CVIP, amdgpu_hwmon_power, POWER_CVIP, CUR_VAL);
static SENSOR_DEVICE_ATTR_2_RO(ave_pow_CVIP, amdgpu_hwmon_power, POWER_CVIP, AVE_VAL);
static SENSOR_DEVICE_ATTR_2_RO(cur_vol_CPU0, amdgpu_hwmon_voltage, VOL_CPU0, CUR_VAL);
static SENSOR_DEVICE_ATTR_2_RO(cur_vol_CPU1, amdgpu_hwmon_voltage, VOL_CPU1, CUR_VAL);
static SENSOR_DEVICE_ATTR_2_RO(cur_vol_CPU2, amdgpu_hwmon_voltage, VOL_CPU2, CUR_VAL);
static SENSOR_DEVICE_ATTR_2_RO(cur_vol_CPU3, amdgpu_hwmon_voltage, VOL_CPU3, CUR_VAL);
static SENSOR_DEVICE_ATTR_2_RO(cur_vol_GFX, amdgpu_hwmon_voltage, VOL_GFX, CUR_VAL);
static SENSOR_DEVICE_ATTR_2_RO(cur_vol_SOC, amdgpu_hwmon_voltage, VOL_SOC, CUR_VAL);
static SENSOR_DEVICE_ATTR_2_RO(cur_vol_CVIP, amdgpu_hwmon_voltage, VOL_CVIP, CUR_VAL);
static SENSOR_DEVICE_ATTR_2_RO(cur_cur_CPU, amdgpu_hwmon_current, CUR_CPU, CUR_VAL);
static SENSOR_DEVICE_ATTR_2_RO(cur_cur_GFX, amdgpu_hwmon_current, CUR_GFX, CUR_VAL);
static SENSOR_DEVICE_ATTR_2_RO(cur_cur_SOC, amdgpu_hwmon_current, CUR_SOC, CUR_VAL);
static SENSOR_DEVICE_ATTR_2_RO(cur_cur_CVIP, amdgpu_hwmon_current, CUR_CVIP, CUR_VAL);
static SENSOR_DEVICE_ATTR_2_RO(cur_tem_CPU0, amdgpu_hwmon_temperature, TEM_CPU0, CUR_VAL);
static SENSOR_DEVICE_ATTR_2_RO(ave_tem_CPU0, amdgpu_hwmon_temperature, TEM_CPU0, AVE_VAL);
static SENSOR_DEVICE_ATTR_2_RO(cur_tem_CPU1, amdgpu_hwmon_temperature, TEM_CPU1, CUR_VAL);
static SENSOR_DEVICE_ATTR_2_RO(ave_tem_CPU1, amdgpu_hwmon_temperature, TEM_CPU1, AVE_VAL);
static SENSOR_DEVICE_ATTR_2_RO(cur_tem_CPU2, amdgpu_hwmon_temperature, TEM_CPU2, CUR_VAL);
static SENSOR_DEVICE_ATTR_2_RO(ave_tem_CPU2, amdgpu_hwmon_temperature, TEM_CPU2, AVE_VAL);
static SENSOR_DEVICE_ATTR_2_RO(cur_tem_CPU3, amdgpu_hwmon_temperature, TEM_CPU3, CUR_VAL);
static SENSOR_DEVICE_ATTR_2_RO(ave_tem_CPU3, amdgpu_hwmon_temperature, TEM_CPU3, AVE_VAL);
static SENSOR_DEVICE_ATTR_2_RO(cur_tem_GFX, amdgpu_hwmon_temperature, TEM_GFX, CUR_VAL);
static SENSOR_DEVICE_ATTR_2_RO(ave_tem_GFX, amdgpu_hwmon_temperature, TEM_GFX, AVE_VAL);
static SENSOR_DEVICE_ATTR_2_RO(cur_tem_SOC, amdgpu_hwmon_temperature, TEM_SOC, CUR_VAL);
static SENSOR_DEVICE_ATTR_2_RO(ave_tem_SOC, amdgpu_hwmon_temperature, TEM_SOC, AVE_VAL);
static SENSOR_DEVICE_ATTR_2_RO(cur_tem_CVIP, amdgpu_hwmon_temperature, TEM_CVIP, CUR_VAL);
static SENSOR_DEVICE_ATTR_2_RO(ave_tem_CVIP, amdgpu_hwmon_temperature, TEM_CVIP, AVE_VAL);
static SENSOR_DEVICE_ATTR_2_RO(cur_clk_CPU0, amdgpu_hwmon_clock, CLK_CPU0, CUR_VAL);
static SENSOR_DEVICE_ATTR_2_RO(cur_clk_CPU1, amdgpu_hwmon_clock, CLK_CPU1, CUR_VAL);
static SENSOR_DEVICE_ATTR_2_RO(cur_clk_CPU2, amdgpu_hwmon_clock, CLK_CPU2, CUR_VAL);
static SENSOR_DEVICE_ATTR_2_RO(cur_clk_CPU3, amdgpu_hwmon_clock, CLK_CPU3, CUR_VAL);
static SENSOR_DEVICE_ATTR_2_RO(cur_clk_GFX, amdgpu_hwmon_clock, CLK_GFX, CUR_VAL);
static SENSOR_DEVICE_ATTR_2_RO(ave_act_CPU, amdgpu_hwmon_activity, ACT_CPU, CUR_VAL);
static SENSOR_DEVICE_ATTR_2_RO(ave_act_GFX, amdgpu_hwmon_activity, ACT_GFX, CUR_VAL);
static SENSOR_DEVICE_ATTR_2_RW(thermal_cap, amdgpu_hwmon_thermal_limit, RES_NUM, TYPE_NUM);
static SENSOR_DEVICE_ATTR_2_WO(ave_pow_time, amdgpu_hwmon_time, POWER_TIME, TYPE_NUM);
static SENSOR_DEVICE_ATTR_2_WO(ave_tem_time, amdgpu_hwmon_time, TEM_TIME, TYPE_NUM);
static SENSOR_DEVICE_ATTR_2_WO(ave_act_time, amdgpu_hwmon_time, ACT_TIME, TYPE_NUM);
static SENSOR_DEVICE_ATTR_2_WO(mtg_end_hys, amdgpu_hwmon_time, MIG_END_HYS, TYPE_NUM);

static struct attribute *hwmon_attributes[] = {
	&sensor_dev_attr_temp1_input.dev_attr.attr,
	&sensor_dev_attr_temp1_crit.dev_attr.attr,
	&sensor_dev_attr_temp1_crit_hyst.dev_attr.attr,
	&sensor_dev_attr_temp2_input.dev_attr.attr,
	&sensor_dev_attr_temp2_crit.dev_attr.attr,
	&sensor_dev_attr_temp2_crit_hyst.dev_attr.attr,
	&sensor_dev_attr_temp3_input.dev_attr.attr,
	&sensor_dev_attr_temp3_crit.dev_attr.attr,
	&sensor_dev_attr_temp3_crit_hyst.dev_attr.attr,
	&sensor_dev_attr_temp1_emergency.dev_attr.attr,
	&sensor_dev_attr_temp2_emergency.dev_attr.attr,
	&sensor_dev_attr_temp3_emergency.dev_attr.attr,
	&sensor_dev_attr_temp1_label.dev_attr.attr,
	&sensor_dev_attr_temp2_label.dev_attr.attr,
	&sensor_dev_attr_temp3_label.dev_attr.attr,
	&sensor_dev_attr_pwm1.dev_attr.attr,
	&sensor_dev_attr_pwm1_enable.dev_attr.attr,
	&sensor_dev_attr_pwm1_min.dev_attr.attr,
	&sensor_dev_attr_pwm1_max.dev_attr.attr,
	&sensor_dev_attr_fan1_input.dev_attr.attr,
	&sensor_dev_attr_fan1_min.dev_attr.attr,
	&sensor_dev_attr_fan1_max.dev_attr.attr,
	&sensor_dev_attr_fan1_target.dev_attr.attr,
	&sensor_dev_attr_fan1_enable.dev_attr.attr,
	&sensor_dev_attr_in0_input.dev_attr.attr,
	&sensor_dev_attr_in0_label.dev_attr.attr,
	&sensor_dev_attr_in1_input.dev_attr.attr,
	&sensor_dev_attr_in1_label.dev_attr.attr,
	&sensor_dev_attr_power1_average.dev_attr.attr,
	&sensor_dev_attr_power1_cap_max.dev_attr.attr,
	&sensor_dev_attr_power1_cap_min.dev_attr.attr,
	&sensor_dev_attr_power1_cap.dev_attr.attr,
	&sensor_dev_attr_freq1_input.dev_attr.attr,
	&sensor_dev_attr_freq1_label.dev_attr.attr,
	&sensor_dev_attr_freq2_input.dev_attr.attr,
	&sensor_dev_attr_freq2_label.dev_attr.attr,
	&sensor_dev_attr_cur_pow_CPU.dev_attr.attr,
	&sensor_dev_attr_ave_pow_CPU.dev_attr.attr,
	&sensor_dev_attr_cur_pow_GFX.dev_attr.attr,
	&sensor_dev_attr_ave_pow_GFX.dev_attr.attr,
	&sensor_dev_attr_cur_pow_SOC.dev_attr.attr,
	&sensor_dev_attr_ave_pow_SOC.dev_attr.attr,
	&sensor_dev_attr_cur_pow_CVIP.dev_attr.attr,
	&sensor_dev_attr_ave_pow_CVIP.dev_attr.attr,
	&sensor_dev_attr_cur_vol_CPU0.dev_attr.attr,
	&sensor_dev_attr_cur_vol_CPU1.dev_attr.attr,
	&sensor_dev_attr_cur_vol_CPU2.dev_attr.attr,
	&sensor_dev_attr_cur_vol_CPU3.dev_attr.attr,
	&sensor_dev_attr_cur_vol_GFX.dev_attr.attr,
	&sensor_dev_attr_cur_vol_SOC.dev_attr.attr,
	&sensor_dev_attr_cur_vol_CVIP.dev_attr.attr,
	&sensor_dev_attr_cur_cur_CPU.dev_attr.attr,
	&sensor_dev_attr_cur_cur_GFX.dev_attr.attr,
	&sensor_dev_attr_cur_cur_SOC.dev_attr.attr,
	&sensor_dev_attr_cur_cur_CVIP.dev_attr.attr,
	&sensor_dev_attr_cur_tem_CPU0.dev_attr.attr,
	&sensor_dev_attr_ave_tem_CPU0.dev_attr.attr,
	&sensor_dev_attr_cur_tem_CPU1.dev_attr.attr,
	&sensor_dev_attr_ave_tem_CPU1.dev_attr.attr,
	&sensor_dev_attr_cur_tem_CPU2.dev_attr.attr,
	&sensor_dev_attr_ave_tem_CPU2.dev_attr.attr,
	&sensor_dev_attr_cur_tem_CPU3.dev_attr.attr,
	&sensor_dev_attr_ave_tem_CPU3.dev_attr.attr,
	&sensor_dev_attr_cur_tem_GFX.dev_attr.attr,
	&sensor_dev_attr_ave_tem_GFX.dev_attr.attr,
	&sensor_dev_attr_cur_tem_SOC.dev_attr.attr,
	&sensor_dev_attr_ave_tem_SOC.dev_attr.attr,
	&sensor_dev_attr_cur_tem_CVIP.dev_attr.attr,
	&sensor_dev_attr_ave_tem_CVIP.dev_attr.attr,
	&sensor_dev_attr_cur_clk_CPU0.dev_attr.attr,
	&sensor_dev_attr_cur_clk_CPU1.dev_attr.attr,
	&sensor_dev_attr_cur_clk_CPU2.dev_attr.attr,
	&sensor_dev_attr_cur_clk_CPU3.dev_attr.attr,
	&sensor_dev_attr_cur_clk_GFX.dev_attr.attr,
	&sensor_dev_attr_ave_act_CPU.dev_attr.attr,
	&sensor_dev_attr_ave_act_GFX.dev_attr.attr,
	&sensor_dev_attr_thermal_cap.dev_attr.attr,
	&sensor_dev_attr_ave_pow_time.dev_attr.attr,
	&sensor_dev_attr_ave_tem_time.dev_attr.attr,
	&sensor_dev_attr_ave_act_time.dev_attr.attr,
	&sensor_dev_attr_mtg_end_hys.dev_attr.attr,
	NULL
};

static umode_t hwmon_attributes_visible(struct kobject *kobj,
					struct attribute *attr, int index)
{
	struct device *dev = kobj_to_dev(kobj);
	struct amdgpu_device *adev = dev_get_drvdata(dev);
	umode_t effective_mode = attr->mode;

	/* under multi-vf mode, the hwmon attributes are all not supported */
	if (amdgpu_sriov_vf(adev) && !amdgpu_sriov_is_pp_one_vf(adev))
		return 0;

	/* there is no fan under pp one vf mode */
	if (amdgpu_sriov_is_pp_one_vf(adev) &&
	    (attr == &sensor_dev_attr_pwm1.dev_attr.attr ||
	     attr == &sensor_dev_attr_pwm1_enable.dev_attr.attr ||
	     attr == &sensor_dev_attr_pwm1_max.dev_attr.attr ||
	     attr == &sensor_dev_attr_pwm1_min.dev_attr.attr ||
	     attr == &sensor_dev_attr_fan1_input.dev_attr.attr ||
	     attr == &sensor_dev_attr_fan1_min.dev_attr.attr ||
	     attr == &sensor_dev_attr_fan1_max.dev_attr.attr ||
	     attr == &sensor_dev_attr_fan1_target.dev_attr.attr ||
	     attr == &sensor_dev_attr_fan1_enable.dev_attr.attr))
		return 0;

	/* Skip fan attributes if fan is not present */
	if (adev->pm.no_fan && (attr == &sensor_dev_attr_pwm1.dev_attr.attr ||
	    attr == &sensor_dev_attr_pwm1_enable.dev_attr.attr ||
	    attr == &sensor_dev_attr_pwm1_max.dev_attr.attr ||
	    attr == &sensor_dev_attr_pwm1_min.dev_attr.attr ||
	    attr == &sensor_dev_attr_fan1_input.dev_attr.attr ||
	    attr == &sensor_dev_attr_fan1_min.dev_attr.attr ||
	    attr == &sensor_dev_attr_fan1_max.dev_attr.attr ||
	    attr == &sensor_dev_attr_fan1_target.dev_attr.attr ||
	    attr == &sensor_dev_attr_fan1_enable.dev_attr.attr))
		return 0;

	/* Skip fan attributes on APU */
	if ((adev->flags & AMD_IS_APU) &&
	    (attr == &sensor_dev_attr_pwm1.dev_attr.attr ||
	     attr == &sensor_dev_attr_pwm1_enable.dev_attr.attr ||
	     attr == &sensor_dev_attr_pwm1_max.dev_attr.attr ||
	     attr == &sensor_dev_attr_pwm1_min.dev_attr.attr ||
	     attr == &sensor_dev_attr_fan1_input.dev_attr.attr ||
	     attr == &sensor_dev_attr_fan1_min.dev_attr.attr ||
	     attr == &sensor_dev_attr_fan1_max.dev_attr.attr ||
	     attr == &sensor_dev_attr_fan1_target.dev_attr.attr ||
	     attr == &sensor_dev_attr_fan1_enable.dev_attr.attr))
		return 0;

	/* Skip limit attributes if DPM is not enabled */
	if (!adev->pm.dpm_enabled &&
	    (attr == &sensor_dev_attr_temp1_crit.dev_attr.attr ||
	     attr == &sensor_dev_attr_temp1_crit_hyst.dev_attr.attr ||
	     attr == &sensor_dev_attr_pwm1.dev_attr.attr ||
	     attr == &sensor_dev_attr_pwm1_enable.dev_attr.attr ||
	     attr == &sensor_dev_attr_pwm1_max.dev_attr.attr ||
	     attr == &sensor_dev_attr_pwm1_min.dev_attr.attr ||
	     attr == &sensor_dev_attr_fan1_input.dev_attr.attr ||
	     attr == &sensor_dev_attr_fan1_min.dev_attr.attr ||
	     attr == &sensor_dev_attr_fan1_max.dev_attr.attr ||
	     attr == &sensor_dev_attr_fan1_target.dev_attr.attr ||
	     attr == &sensor_dev_attr_fan1_enable.dev_attr.attr))
		return 0;

	if (!is_support_sw_smu(adev)) {
		/* mask fan attributes if we have no bindings for this asic to expose */
		if ((!adev->powerplay.pp_funcs->get_fan_speed_percent &&
		     attr == &sensor_dev_attr_pwm1.dev_attr.attr) || /* can't query fan */
		    (!adev->powerplay.pp_funcs->get_fan_control_mode &&
		     attr == &sensor_dev_attr_pwm1_enable.dev_attr.attr)) /* can't query state */
			effective_mode &= ~S_IRUGO;

		if ((!adev->powerplay.pp_funcs->set_fan_speed_percent &&
		     attr == &sensor_dev_attr_pwm1.dev_attr.attr) || /* can't manage fan */
		    (!adev->powerplay.pp_funcs->set_fan_control_mode &&
		     attr == &sensor_dev_attr_pwm1_enable.dev_attr.attr)) /* can't manage state */
			effective_mode &= ~S_IWUSR;
	}

	if (((adev->flags & AMD_IS_APU) ||
	     adev->family == AMDGPU_FAMILY_SI) &&   /* not implemented yet */
	    (attr == &sensor_dev_attr_power1_average.dev_attr.attr ||
	     attr == &sensor_dev_attr_power1_cap_max.dev_attr.attr ||
	     attr == &sensor_dev_attr_power1_cap_min.dev_attr.attr))
		return 0;

	if ((((adev->flags & AMD_IS_APU) &&
	     adev->asic_type != CHIP_MERO) ||
	     adev->family == AMDGPU_FAMILY_SI) &&   /* not implemented yet */
	     attr == &sensor_dev_attr_power1_cap.dev_attr.attr)
		return 0;

	if (!is_support_sw_smu(adev)) {
		/* hide max/min values if we can't both query and manage the fan */
		if ((!adev->powerplay.pp_funcs->set_fan_speed_percent &&
		     !adev->powerplay.pp_funcs->get_fan_speed_percent) &&
		     (!adev->powerplay.pp_funcs->set_fan_speed_rpm &&
		     !adev->powerplay.pp_funcs->get_fan_speed_rpm) &&
		    (attr == &sensor_dev_attr_pwm1_max.dev_attr.attr ||
		     attr == &sensor_dev_attr_pwm1_min.dev_attr.attr))
			return 0;

		if ((!adev->powerplay.pp_funcs->set_fan_speed_rpm &&
		     !adev->powerplay.pp_funcs->get_fan_speed_rpm) &&
		    (attr == &sensor_dev_attr_fan1_max.dev_attr.attr ||
		     attr == &sensor_dev_attr_fan1_min.dev_attr.attr))
			return 0;
	}

	if ((adev->family == AMDGPU_FAMILY_SI ||	/* not implemented yet */
	     adev->family == AMDGPU_FAMILY_KV) &&	/* not implemented yet */
	    (attr == &sensor_dev_attr_in0_input.dev_attr.attr ||
	     attr == &sensor_dev_attr_in0_label.dev_attr.attr))
		return 0;

	/* only APUs have vddnb */
	if (!(adev->flags & AMD_IS_APU) &&
	    (attr == &sensor_dev_attr_in1_input.dev_attr.attr ||
	     attr == &sensor_dev_attr_in1_label.dev_attr.attr))
		return 0;

	/* no mclk on APUs */
	if ((adev->flags & AMD_IS_APU) &&
	    (attr == &sensor_dev_attr_freq2_input.dev_attr.attr ||
	     attr == &sensor_dev_attr_freq2_label.dev_attr.attr))
		return 0;

	/* only SOC15 dGPUs support hotspot and mem temperatures */
	if (((adev->flags & AMD_IS_APU) ||
	     adev->asic_type < CHIP_VEGA10) &&
	    (attr == &sensor_dev_attr_temp2_crit.dev_attr.attr ||
	     attr == &sensor_dev_attr_temp2_crit_hyst.dev_attr.attr ||
	     attr == &sensor_dev_attr_temp3_crit.dev_attr.attr ||
	     attr == &sensor_dev_attr_temp3_crit_hyst.dev_attr.attr ||
	     attr == &sensor_dev_attr_temp1_emergency.dev_attr.attr ||
	     attr == &sensor_dev_attr_temp2_emergency.dev_attr.attr ||
	     attr == &sensor_dev_attr_temp3_emergency.dev_attr.attr ||
	     attr == &sensor_dev_attr_temp2_input.dev_attr.attr ||
	     attr == &sensor_dev_attr_temp3_input.dev_attr.attr ||
	     attr == &sensor_dev_attr_temp2_label.dev_attr.attr ||
	     attr == &sensor_dev_attr_temp3_label.dev_attr.attr))
		return 0;

	return effective_mode;
}

static const struct attribute_group hwmon_attrgroup = {
	.attrs = hwmon_attributes,
	.is_visible = hwmon_attributes_visible,
};

static const struct attribute_group *hwmon_groups[] = {
	&hwmon_attrgroup,
	NULL
};

void amdgpu_dpm_thermal_work_handler(struct work_struct *work)
{
	struct amdgpu_device *adev =
		container_of(work, struct amdgpu_device,
			     pm.dpm.thermal.work);
	/* switch to the thermal state */
	enum amd_pm_state_type dpm_state = POWER_STATE_TYPE_INTERNAL_THERMAL;
	int temp, size = sizeof(temp);

	if (!adev->pm.dpm_enabled)
		return;

	if (!amdgpu_dpm_read_sensor(adev, AMDGPU_PP_SENSOR_GPU_TEMP,
				    (void *)&temp, &size)) {
		if (temp < adev->pm.dpm.thermal.min_temp)
			/* switch back the user state */
			dpm_state = adev->pm.dpm.user_state;
	} else {
		if (adev->pm.dpm.thermal.high_to_low)
			/* switch back the user state */
			dpm_state = adev->pm.dpm.user_state;
	}
	mutex_lock(&adev->pm.mutex);
	if (dpm_state == POWER_STATE_TYPE_INTERNAL_THERMAL)
		adev->pm.dpm.thermal_active = true;
	else
		adev->pm.dpm.thermal_active = false;
	adev->pm.dpm.state = dpm_state;
	mutex_unlock(&adev->pm.mutex);

	amdgpu_pm_compute_clocks(adev);
}

static struct amdgpu_ps *amdgpu_dpm_pick_power_state(struct amdgpu_device *adev,
						     enum amd_pm_state_type dpm_state)
{
	int i;
	struct amdgpu_ps *ps;
	u32 ui_class;
	bool single_display = (adev->pm.dpm.new_active_crtc_count < 2) ?
		true : false;

	/* check if the vblank period is too short to adjust the mclk */
	if (single_display && adev->powerplay.pp_funcs->vblank_too_short) {
		if (amdgpu_dpm_vblank_too_short(adev))
			single_display = false;
	}

	/* certain older asics have a separare 3D performance state,
	 * so try that first if the user selected performance
	 */
	if (dpm_state == POWER_STATE_TYPE_PERFORMANCE)
		dpm_state = POWER_STATE_TYPE_INTERNAL_3DPERF;
	/* balanced states don't exist at the moment */
	if (dpm_state == POWER_STATE_TYPE_BALANCED)
		dpm_state = POWER_STATE_TYPE_PERFORMANCE;

restart_search:
	/* Pick the best power state based on current conditions */
	for (i = 0; i < adev->pm.dpm.num_ps; i++) {
		ps = &adev->pm.dpm.ps[i];
		ui_class = ps->class & ATOM_PPLIB_CLASSIFICATION_UI_MASK;
		switch (dpm_state) {
		/* user states */
		case POWER_STATE_TYPE_BATTERY:
			if (ui_class == ATOM_PPLIB_CLASSIFICATION_UI_BATTERY) {
				if (ps->caps & ATOM_PPLIB_SINGLE_DISPLAY_ONLY) {
					if (single_display)
						return ps;
				} else
					return ps;
			}
			break;
		case POWER_STATE_TYPE_BALANCED:
			if (ui_class == ATOM_PPLIB_CLASSIFICATION_UI_BALANCED) {
				if (ps->caps & ATOM_PPLIB_SINGLE_DISPLAY_ONLY) {
					if (single_display)
						return ps;
				} else
					return ps;
			}
			break;
		case POWER_STATE_TYPE_PERFORMANCE:
			if (ui_class == ATOM_PPLIB_CLASSIFICATION_UI_PERFORMANCE) {
				if (ps->caps & ATOM_PPLIB_SINGLE_DISPLAY_ONLY) {
					if (single_display)
						return ps;
				} else
					return ps;
			}
			break;
		/* internal states */
		case POWER_STATE_TYPE_INTERNAL_UVD:
			if (adev->pm.dpm.uvd_ps)
				return adev->pm.dpm.uvd_ps;
			else
				break;
		case POWER_STATE_TYPE_INTERNAL_UVD_SD:
			if (ps->class & ATOM_PPLIB_CLASSIFICATION_SDSTATE)
				return ps;
			break;
		case POWER_STATE_TYPE_INTERNAL_UVD_HD:
			if (ps->class & ATOM_PPLIB_CLASSIFICATION_HDSTATE)
				return ps;
			break;
		case POWER_STATE_TYPE_INTERNAL_UVD_HD2:
			if (ps->class & ATOM_PPLIB_CLASSIFICATION_HD2STATE)
				return ps;
			break;
		case POWER_STATE_TYPE_INTERNAL_UVD_MVC:
			if (ps->class2 & ATOM_PPLIB_CLASSIFICATION2_MVC)
				return ps;
			break;
		case POWER_STATE_TYPE_INTERNAL_BOOT:
			return adev->pm.dpm.boot_ps;
		case POWER_STATE_TYPE_INTERNAL_THERMAL:
			if (ps->class & ATOM_PPLIB_CLASSIFICATION_THERMAL)
				return ps;
			break;
		case POWER_STATE_TYPE_INTERNAL_ACPI:
			if (ps->class & ATOM_PPLIB_CLASSIFICATION_ACPI)
				return ps;
			break;
		case POWER_STATE_TYPE_INTERNAL_ULV:
			if (ps->class2 & ATOM_PPLIB_CLASSIFICATION2_ULV)
				return ps;
			break;
		case POWER_STATE_TYPE_INTERNAL_3DPERF:
			if (ps->class & ATOM_PPLIB_CLASSIFICATION_3DPERFORMANCE)
				return ps;
			break;
		default:
			break;
		}
	}
	/* use a fallback state if we didn't match */
	switch (dpm_state) {
	case POWER_STATE_TYPE_INTERNAL_UVD_SD:
		dpm_state = POWER_STATE_TYPE_INTERNAL_UVD_HD;
		goto restart_search;
	case POWER_STATE_TYPE_INTERNAL_UVD_HD:
	case POWER_STATE_TYPE_INTERNAL_UVD_HD2:
	case POWER_STATE_TYPE_INTERNAL_UVD_MVC:
		if (adev->pm.dpm.uvd_ps) {
			return adev->pm.dpm.uvd_ps;
		} else {
			dpm_state = POWER_STATE_TYPE_PERFORMANCE;
			goto restart_search;
		}
	case POWER_STATE_TYPE_INTERNAL_THERMAL:
		dpm_state = POWER_STATE_TYPE_INTERNAL_ACPI;
		goto restart_search;
	case POWER_STATE_TYPE_INTERNAL_ACPI:
		dpm_state = POWER_STATE_TYPE_BATTERY;
		goto restart_search;
	case POWER_STATE_TYPE_BATTERY:
	case POWER_STATE_TYPE_BALANCED:
	case POWER_STATE_TYPE_INTERNAL_3DPERF:
		dpm_state = POWER_STATE_TYPE_PERFORMANCE;
		goto restart_search;
	default:
		break;
	}

	return NULL;
}

static void amdgpu_dpm_change_power_state_locked(struct amdgpu_device *adev)
{
	struct amdgpu_ps *ps;
	enum amd_pm_state_type dpm_state;
	int ret;
	bool equal = false;

	/* if dpm init failed */
	if (!adev->pm.dpm_enabled)
		return;

	if (adev->pm.dpm.user_state != adev->pm.dpm.state) {
		/* add other state override checks here */
		if ((!adev->pm.dpm.thermal_active) &&
		    (!adev->pm.dpm.uvd_active))
			adev->pm.dpm.state = adev->pm.dpm.user_state;
	}
	dpm_state = adev->pm.dpm.state;

	ps = amdgpu_dpm_pick_power_state(adev, dpm_state);
	if (ps)
		adev->pm.dpm.requested_ps = ps;
	else
		return;

	if (amdgpu_dpm == 1 && adev->powerplay.pp_funcs->print_power_state) {
		printk("switching from power state:\n");
		amdgpu_dpm_print_power_state(adev, adev->pm.dpm.current_ps);
		printk("switching to power state:\n");
		amdgpu_dpm_print_power_state(adev, adev->pm.dpm.requested_ps);
	}

	/* update whether vce is active */
	ps->vce_active = adev->pm.dpm.vce_active;
	if (adev->powerplay.pp_funcs->display_configuration_changed)
		amdgpu_dpm_display_configuration_changed(adev);

	ret = amdgpu_dpm_pre_set_power_state(adev);
	if (ret)
		return;

	if (adev->powerplay.pp_funcs->check_state_equal) {
		if (0 != amdgpu_dpm_check_state_equal(adev, adev->pm.dpm.current_ps, adev->pm.dpm.requested_ps, &equal))
			equal = false;
	}

	if (equal)
		return;

	amdgpu_dpm_set_power_state(adev);
	amdgpu_dpm_post_set_power_state(adev);

	adev->pm.dpm.current_active_crtcs = adev->pm.dpm.new_active_crtcs;
	adev->pm.dpm.current_active_crtc_count = adev->pm.dpm.new_active_crtc_count;

	if (adev->powerplay.pp_funcs->force_performance_level) {
		if (adev->pm.dpm.thermal_active) {
			enum amd_dpm_forced_level level = adev->pm.dpm.forced_level;
			/* force low perf level for thermal */
			amdgpu_dpm_force_performance_level(adev, AMD_DPM_FORCED_LEVEL_LOW);
			/* save the user's level */
			adev->pm.dpm.forced_level = level;
		} else {
			/* otherwise, user selected level */
			amdgpu_dpm_force_performance_level(adev, adev->pm.dpm.forced_level);
		}
	}
}

void amdgpu_dpm_enable_uvd(struct amdgpu_device *adev, bool enable)
{
	int ret = 0;

	ret = amdgpu_dpm_set_powergating_by_smu(adev, AMD_IP_BLOCK_TYPE_UVD, !enable);
	if (ret)
		DRM_ERROR("Dpm %s uvd failed, ret = %d. \n",
			  enable ? "enable" : "disable", ret);

	/* enable/disable Low Memory PState for UVD (4k videos) */
	if (adev->asic_type == CHIP_STONEY &&
		adev->uvd.decode_image_width >= WIDTH_4K) {
		struct pp_hwmgr *hwmgr = adev->powerplay.pp_handle;

		if (hwmgr && hwmgr->hwmgr_func &&
		    hwmgr->hwmgr_func->update_nbdpm_pstate)
			hwmgr->hwmgr_func->update_nbdpm_pstate(hwmgr,
							       !enable,
							       true);
	}
}

void amdgpu_dpm_enable_vce(struct amdgpu_device *adev, bool enable)
{
	int ret = 0;

	ret = amdgpu_dpm_set_powergating_by_smu(adev, AMD_IP_BLOCK_TYPE_VCE, !enable);
	if (ret)
		DRM_ERROR("Dpm %s vce failed, ret = %d. \n",
			  enable ? "enable" : "disable", ret);
}

void amdgpu_pm_print_power_states(struct amdgpu_device *adev)
{
	int i;

	if (adev->powerplay.pp_funcs->print_power_state == NULL)
		return;

	for (i = 0; i < adev->pm.dpm.num_ps; i++)
		amdgpu_dpm_print_power_state(adev, &adev->pm.dpm.ps[i]);

}

void amdgpu_dpm_enable_jpeg(struct amdgpu_device *adev, bool enable)
{
	int ret = 0;

	ret = amdgpu_dpm_set_powergating_by_smu(adev, AMD_IP_BLOCK_TYPE_JPEG, !enable);
	if (ret)
		DRM_ERROR("Dpm %s jpeg failed, ret = %d. \n",
			  enable ? "enable" : "disable", ret);
}

int amdgpu_pm_load_smu_firmware(struct amdgpu_device *adev, uint32_t *smu_version)
{
	int r;

	if (adev->powerplay.pp_funcs && adev->powerplay.pp_funcs->load_firmware) {
		r = adev->powerplay.pp_funcs->load_firmware(adev->powerplay.pp_handle);
		if (r) {
			pr_err("smu firmware loading failed\n");
			return r;
		}
		*smu_version = adev->pm.fw_version;
	}
	return 0;
}

int amdgpu_dpm_vclk_read(struct amdgpu_device *adev, char *buf)
{
	int ret = 0;

	ret = pp_dpm_vclk_show(adev->dev, NULL, buf);
	return ret;
}

int amdgpu_dpm_vclk_write(struct amdgpu_device *adev, char *buf, size_t count)
{
	int ret = 0;

	ret = pp_dpm_vclk_store(adev->dev, NULL, buf, count);
	return ret;
}

int amdgpu_dpm_dclk_read(struct amdgpu_device *adev, char *buf)
{
	int ret = 0;

	ret = pp_dpm_dclk_show(adev->dev, NULL, buf);
	return ret;
}

int amdgpu_dpm_dclk_write(struct amdgpu_device *adev, char *buf, size_t count)
{
	int ret = 0;

	ret = pp_dpm_dclk_store(adev->dev, NULL, buf, count);
	return ret;
}

int amdgpu_pm_sysfs_init(struct amdgpu_device *adev)
{
	struct pp_hwmgr *hwmgr = adev->powerplay.pp_handle;
	int ret;

	if (adev->pm.sysfs_initialized)
		return 0;

	if (adev->pm.dpm_enabled == 0)
		return 0;

	adev->pm.int_hwmon_dev = hwmon_device_register_with_groups(adev->dev,
								   DRIVER_NAME, adev,
								   hwmon_groups);
	if (IS_ERR(adev->pm.int_hwmon_dev)) {
		ret = PTR_ERR(adev->pm.int_hwmon_dev);
		dev_err(adev->dev,
			"Unable to register hwmon device: %d\n", ret);
		return ret;
	}

	ret = device_create_file(adev->dev, &dev_attr_pp_dpm_min_sclk);
	if (ret) {
		DRM_ERROR("failed to create device file pp_dpm_min_sclk\n");
		return ret;
	}

	ret = device_create_file(adev->dev, &dev_attr_pp_dpm_max_sclk);
	if (ret) {
		DRM_ERROR("failed to create device file pp_dpm_max_sclk\n");
		return ret;
	}

	ret = device_create_file(adev->dev, &dev_attr_pp_dpm_min_cclk);
	if (ret) {
		DRM_ERROR("failed to create device file pp_dpm_min_cclk\n");
		return ret;
	}

	ret = device_create_file(adev->dev, &dev_attr_pp_dpm_max_cclk);
	if (ret) {
		DRM_ERROR("failed to create device file pp_dpm_max_cclk\n");
		return ret;
	}

	ret = device_create_file(adev->dev, &dev_attr_power_dpm_state);
	if (ret) {
		DRM_ERROR("failed to create device file for dpm state\n");
		return ret;
	}
	ret = device_create_file(adev->dev, &dev_attr_power_dpm_force_performance_level);
	if (ret) {
		DRM_ERROR("failed to create device file for dpm state\n");
		return ret;
	}

	ret = device_create_file(adev->dev, &dev_attr_pp_num_states);
	if (ret) {
		DRM_ERROR("failed to create device file pp_num_states\n");
		return ret;
	}
	ret = device_create_file(adev->dev, &dev_attr_pp_cur_state);
	if (ret) {
		DRM_ERROR("failed to create device file pp_cur_state\n");
		return ret;
	}
	ret = device_create_file(adev->dev, &dev_attr_pp_force_state);
	if (ret) {
		DRM_ERROR("failed to create device file pp_force_state\n");
		return ret;
	}
	ret = device_create_file(adev->dev, &dev_attr_pp_table);
	if (ret) {
		DRM_ERROR("failed to create device file pp_table\n");
		return ret;
	}

	ret = device_create_file(adev->dev, &dev_attr_pp_dpm_sclk);
	if (ret) {
		DRM_ERROR("failed to create device file pp_dpm_sclk\n");
		return ret;
	}

	ret = device_create_file(adev->dev, &dev_attr_pp_dpm_vclk);
	if (ret) {
		DRM_ERROR("failed to create device file pp_dpm_vclk\n");
		return ret;
	}
	ret = device_create_file(adev->dev, &dev_attr_pp_dpm_dclk);
	if (ret) {
		DRM_ERROR("failed to create device file pp_dpm_dclk\n");
		return ret;
	}

	ret = device_create_file(adev->dev, &dev_attr_pp_dpm_df_active_level);
	if (ret) {
		DRM_ERROR("failed to create device file pp_df_active\n");
		return ret;
	}

	ret = device_create_file(adev->dev, &dev_attr_pp_dpm_df_softmin_level);
	if (ret) {
		DRM_ERROR("failed to create device file pp_df_softmin\n");
		return ret;
	}

	ret = device_create_file(adev->dev, &dev_attr_pp_dpm_cclk_policy);
	if (ret) {
		DRM_ERROR("failed to create device file pp_cclk_policy\n");
		return ret;
	}

	/* Arcturus does not support standalone mclk/socclk/fclk level setting */
	if (adev->asic_type == CHIP_ARCTURUS) {
		dev_attr_pp_dpm_mclk.attr.mode &= ~S_IWUGO;
		dev_attr_pp_dpm_mclk.store = NULL;

		dev_attr_pp_dpm_socclk.attr.mode &= ~S_IWUGO;
		dev_attr_pp_dpm_socclk.store = NULL;

		dev_attr_pp_dpm_fclk.attr.mode &= ~S_IWUGO;
		dev_attr_pp_dpm_fclk.store = NULL;
	}

	ret = device_create_file(adev->dev, &dev_attr_pp_dpm_mclk);
	if (ret) {
		DRM_ERROR("failed to create device file pp_dpm_mclk\n");
		return ret;
	}
	if (adev->asic_type >= CHIP_VEGA10) {
		ret = device_create_file(adev->dev, &dev_attr_pp_dpm_socclk);
		if (ret) {
			DRM_ERROR("failed to create device file pp_dpm_socclk\n");
			return ret;
		}
		if (adev->asic_type != CHIP_ARCTURUS) {
			ret = device_create_file(adev->dev, &dev_attr_pp_dpm_dcefclk);
			if (ret) {
				DRM_ERROR("failed to create device file pp_dpm_dcefclk\n");
				return ret;
			}
		}
	}
	if (adev->asic_type >= CHIP_VEGA20) {
		ret = device_create_file(adev->dev, &dev_attr_pp_dpm_fclk);
		if (ret) {
			DRM_ERROR("failed to create device file pp_dpm_fclk\n");
			return ret;
		}
	}
	if (adev->asic_type != CHIP_ARCTURUS) {
		ret = device_create_file(adev->dev, &dev_attr_pp_dpm_pcie);
		if (ret) {
			DRM_ERROR("failed to create device file pp_dpm_pcie\n");
			return ret;
		}
	}
	ret = device_create_file(adev->dev, &dev_attr_pp_sclk_od);
	if (ret) {
		DRM_ERROR("failed to create device file pp_sclk_od\n");
		return ret;
	}
	ret = device_create_file(adev->dev, &dev_attr_pp_mclk_od);
	if (ret) {
		DRM_ERROR("failed to create device file pp_mclk_od\n");
		return ret;
	}
	ret = device_create_file(adev->dev,
			&dev_attr_pp_power_profile_mode);
	if (ret) {
		DRM_ERROR("failed to create device file	"
				"pp_power_profile_mode\n");
		return ret;
	}
	if ((is_support_sw_smu(adev) && adev->smu.od_enabled) ||
	    (!is_support_sw_smu(adev) && hwmgr->od_enabled)) {
		ret = device_create_file(adev->dev,
				&dev_attr_pp_od_clk_voltage);
		if (ret) {
			DRM_ERROR("failed to create device file	"
					"pp_od_clk_voltage\n");
			return ret;
		}
	}
	ret = device_create_file(adev->dev,
			&dev_attr_gpu_busy_percent);
	if (ret) {
		DRM_ERROR("failed to create device file	"
				"gpu_busy_level\n");
		return ret;
	}
	/* APU does not have its own dedicated memory */
	if (!(adev->flags & AMD_IS_APU) &&
	     (adev->asic_type != CHIP_VEGA10)) {
		ret = device_create_file(adev->dev,
				&dev_attr_mem_busy_percent);
		if (ret) {
			DRM_ERROR("failed to create device file	"
					"mem_busy_percent\n");
			return ret;
		}
	}
	/* PCIe Perf counters won't work on APU nodes */
	if (!(adev->flags & AMD_IS_APU)) {
		ret = device_create_file(adev->dev, &dev_attr_pcie_bw);
		if (ret) {
			DRM_ERROR("failed to create device file pcie_bw\n");
			return ret;
		}
	}
	if (adev->unique_id)
		ret = device_create_file(adev->dev, &dev_attr_unique_id);
	if (ret) {
		DRM_ERROR("failed to create device file unique_id\n");
		return ret;
	}

	ret = device_create_file(adev->dev,
			&dev_attr_gpu_metrics);
	if (ret) {
		DRM_ERROR("failed to create device file	gpu_metrics\n");
		return ret;
	}

	ret = amdgpu_debugfs_pm_init(adev);
	if (ret) {
		DRM_ERROR("Failed to register debugfs file for dpm!\n");
		return ret;
	}

	if ((adev->asic_type >= CHIP_VEGA10) &&
	    !(adev->flags & AMD_IS_APU)) {
		ret = device_create_file(adev->dev,
				&dev_attr_pp_features);
		if (ret) {
			DRM_ERROR("failed to create device file	"
					"pp_features\n");
			return ret;
		}
	}

	if (adev->asic_type == CHIP_MERO) {
		ret = device_create_file(adev->dev, &dev_attr_fclk_dpm_activity_monitor);
		if (ret) {
			DRM_ERROR("failed to create device file fclk_dpm_activity_monitor\n");
			return ret;
		}

		ret = device_create_file(adev->dev, &dev_attr_fclk_dpm_param);
		if (ret) {
			DRM_ERROR("failed to create device file fclk_dpm_param\n");
			return ret;
		}

		ret = device_create_file(adev->dev, &dev_attr_pp_dpm_tuning_fclk);
		if (ret) {
			DRM_ERROR("failed to create device file pp_dpm_tuning_fclk\n");
			return ret;
		}

		ret = device_create_file(adev->dev, &dev_attr_lclk_busy_calc_from_iohc);
		if (ret) {
			DRM_ERROR("failed to create device file lclk_busy_calc_from_iohc\n");
			return ret;
		}

		ret = device_create_file(adev->dev, &dev_attr_pp_dpm_tuning_lclk);
		if (ret) {
			DRM_ERROR("failed to create device file pp_dpm_tuning_lclk\n");
			return ret;
		}
	}

	adev->pm.sysfs_initialized = true;
	kobject_uevent(&adev->dev->kobj, KOBJ_CHANGE);

#ifdef CONFIG_ML_GSM
		/* get lclk spinlock */
		lclk_spinlock.nibble_addr =
			GSM_RESERVED_PREDEF_CVCORE_LOCKS_X86;
		lclk_spinlock.nibble_id = GSM_PREDEF_LCLK_STATE_LOCK_NIBBLE;

		/* get fclk spinlock */
		fclk_spinlock.nibble_addr =
			GSM_RESERVED_PREDEF_CVCORE_LOCKS_X86;
		fclk_spinlock.nibble_id = GSM_PREDEF_FCLK_STATE_LOCK_NIBBLE;
#endif

	return 0;
}

void amdgpu_pm_sysfs_fini(struct amdgpu_device *adev)
{
	struct pp_hwmgr *hwmgr = adev->powerplay.pp_handle;

	if (adev->pm.dpm_enabled == 0)
		return;

	if (adev->pm.int_hwmon_dev)
		hwmon_device_unregister(adev->pm.int_hwmon_dev);
	device_remove_file(adev->dev, &dev_attr_power_dpm_state);
	device_remove_file(adev->dev, &dev_attr_power_dpm_force_performance_level);

	device_remove_file(adev->dev, &dev_attr_pp_num_states);
	device_remove_file(adev->dev, &dev_attr_pp_cur_state);
	device_remove_file(adev->dev, &dev_attr_pp_force_state);
	device_remove_file(adev->dev, &dev_attr_pp_table);

	device_remove_file(adev->dev, &dev_attr_pp_dpm_sclk);
	device_remove_file(adev->dev, &dev_attr_pp_dpm_min_sclk);
	device_remove_file(adev->dev, &dev_attr_pp_dpm_min_cclk);
	device_remove_file(adev->dev, &dev_attr_pp_dpm_max_sclk);
	device_remove_file(adev->dev, &dev_attr_pp_dpm_max_cclk);
	device_remove_file(adev->dev, &dev_attr_pp_dpm_mclk);
	device_remove_file(adev->dev, &dev_attr_pp_dpm_cclk_policy);
	device_remove_file(adev->dev, &dev_attr_pp_dpm_df_active_level);
	device_remove_file(adev->dev, &dev_attr_pp_dpm_df_softmin_level);
	if (adev->asic_type >= CHIP_VEGA10) {
		device_remove_file(adev->dev, &dev_attr_pp_dpm_socclk);
		if (adev->asic_type != CHIP_ARCTURUS)
			device_remove_file(adev->dev, &dev_attr_pp_dpm_dcefclk);
	}
	if (adev->asic_type != CHIP_ARCTURUS)
		device_remove_file(adev->dev, &dev_attr_pp_dpm_pcie);
	if (adev->asic_type >= CHIP_VEGA20)
		device_remove_file(adev->dev, &dev_attr_pp_dpm_fclk);
	device_remove_file(adev->dev, &dev_attr_pp_sclk_od);
	device_remove_file(adev->dev, &dev_attr_pp_mclk_od);
	device_remove_file(adev->dev,
			&dev_attr_pp_power_profile_mode);
	if ((is_support_sw_smu(adev) && adev->smu.od_enabled) ||
	    (!is_support_sw_smu(adev) && hwmgr->od_enabled))
		device_remove_file(adev->dev,
				&dev_attr_pp_od_clk_voltage);
	device_remove_file(adev->dev, &dev_attr_gpu_busy_percent);
	if (!(adev->flags & AMD_IS_APU) &&
	     (adev->asic_type != CHIP_VEGA10))
		device_remove_file(adev->dev, &dev_attr_mem_busy_percent);
	if (!(adev->flags & AMD_IS_APU))
		device_remove_file(adev->dev, &dev_attr_pcie_bw);
	if (adev->unique_id)
		device_remove_file(adev->dev, &dev_attr_unique_id);
	device_remove_file(adev->dev, &dev_attr_gpu_metrics);
	if ((adev->asic_type >= CHIP_VEGA10) &&
	    !(adev->flags & AMD_IS_APU))
		device_remove_file(adev->dev, &dev_attr_pp_features);
	if (adev->asic_type == CHIP_MERO) {
		device_remove_file(adev->dev, &dev_attr_fclk_dpm_activity_monitor);
		device_remove_file(adev->dev, &dev_attr_fclk_dpm_param);
		device_remove_file(adev->dev, &dev_attr_pp_dpm_tuning_fclk);
		device_remove_file(adev->dev, &dev_attr_lclk_busy_calc_from_iohc);
		device_remove_file(adev->dev, &dev_attr_pp_dpm_tuning_lclk);
	}
}

void amdgpu_pm_compute_clocks(struct amdgpu_device *adev)
{
	int i = 0;

	if (!adev->pm.dpm_enabled)
		return;

	if (adev->mode_info.num_crtc)
		amdgpu_display_bandwidth_update(adev);

	for (i = 0; i < AMDGPU_MAX_RINGS; i++) {
		struct amdgpu_ring *ring = adev->rings[i];
		if (ring && ring->sched.ready)
			amdgpu_fence_wait_empty(ring);
	}

	if (is_support_sw_smu(adev)) {
		struct smu_dpm_context *smu_dpm = &adev->smu.smu_dpm;
		smu_handle_task(&adev->smu,
				smu_dpm->dpm_level,
				AMD_PP_TASK_DISPLAY_CONFIG_CHANGE,
				true);
	} else {
		if (adev->powerplay.pp_funcs->dispatch_tasks) {
			if (!amdgpu_device_has_dc_support(adev)) {
				mutex_lock(&adev->pm.mutex);
				amdgpu_dpm_get_active_displays(adev);
				adev->pm.pm_display_cfg.num_display = adev->pm.dpm.new_active_crtc_count;
				adev->pm.pm_display_cfg.vrefresh = amdgpu_dpm_get_vrefresh(adev);
				adev->pm.pm_display_cfg.min_vblank_time = amdgpu_dpm_get_vblank_time(adev);
				/* we have issues with mclk switching with refresh rates over 120 hz on the non-DC code. */
				if (adev->pm.pm_display_cfg.vrefresh > 120)
					adev->pm.pm_display_cfg.min_vblank_time = 0;
				if (adev->powerplay.pp_funcs->display_configuration_change)
					adev->powerplay.pp_funcs->display_configuration_change(
									adev->powerplay.pp_handle,
									&adev->pm.pm_display_cfg);
				mutex_unlock(&adev->pm.mutex);
			}
			amdgpu_dpm_dispatch_task(adev, AMD_PP_TASK_DISPLAY_CONFIG_CHANGE, NULL);
		} else {
			mutex_lock(&adev->pm.mutex);
			amdgpu_dpm_get_active_displays(adev);
			amdgpu_dpm_change_power_state_locked(adev);
			mutex_unlock(&adev->pm.mutex);
		}
	}
}

/*
 * Debugfs info
 */
#if defined(CONFIG_DEBUG_FS)

static int amdgpu_debugfs_pm_info_pp(struct seq_file *m, struct amdgpu_device *adev)
{
	uint32_t value;
	uint32_t query = 0;
	int size;
	uint64_t value64 = 0;

	/* GPU Clocks */
	size = sizeof(value);
	seq_printf(m, "GFX Clocks and Power:\n");
	if (!amdgpu_dpm_read_sensor(adev, AMDGPU_PP_SENSOR_GFX_MCLK, (void *)&value, &size))
		seq_printf(m, "\t%u MHz (MCLK)\n", value/100);
	if (!amdgpu_dpm_read_sensor(adev, AMDGPU_PP_SENSOR_GFX_SCLK, (void *)&value, &size))
		seq_printf(m, "\t%u MHz (SCLK)\n", value/100);
	if (!amdgpu_dpm_read_sensor(adev, AMDGPU_PP_SENSOR_STABLE_PSTATE_SCLK, (void *)&value, &size))
		seq_printf(m, "\t%u MHz (PSTATE_SCLK)\n", value/100);
	if (!amdgpu_dpm_read_sensor(adev, AMDGPU_PP_SENSOR_STABLE_PSTATE_MCLK, (void *)&value, &size))
		seq_printf(m, "\t%u MHz (PSTATE_MCLK)\n", value/100);
	if (!amdgpu_dpm_read_sensor(adev, AMDGPU_PP_SENSOR_VDDGFX, (void *)&value, &size))
		seq_printf(m, "\t%u mV (VDDGFX)\n", value);
	if (!amdgpu_dpm_read_sensor(adev, AMDGPU_PP_SENSOR_VDDNB, (void *)&value, &size))
		seq_printf(m, "\t%u mV (VDDNB)\n", value);
	size = sizeof(uint32_t);
	if (!amdgpu_dpm_read_sensor(adev, AMDGPU_PP_SENSOR_GPU_POWER, (void *)&query, &size))
		seq_printf(m, "\t%u.%u W (average GPU)\n", query >> 8, query & 0xff);
	size = sizeof(value);
	seq_printf(m, "\n");

	/* GPU Temp */
	if (!amdgpu_dpm_read_sensor(adev, AMDGPU_PP_SENSOR_GPU_TEMP, (void *)&value, &size))
		seq_printf(m, "GPU Temperature: %u C\n", value/1000);

	/* GPU Load */
	if (!amdgpu_dpm_read_sensor(adev, AMDGPU_PP_SENSOR_GPU_LOAD, (void *)&value, &size))
		seq_printf(m, "GPU Load: %u %%\n", value);
	/* MEM Load */
	if (!amdgpu_dpm_read_sensor(adev, AMDGPU_PP_SENSOR_MEM_LOAD, (void *)&value, &size))
		seq_printf(m, "MEM Load: %u %%\n", value);

	seq_printf(m, "\n");

	/* SMC feature mask */
	if (!amdgpu_dpm_read_sensor(adev, AMDGPU_PP_SENSOR_ENABLED_SMC_FEATURES_MASK, (void *)&value64, &size))
		seq_printf(m, "SMC Feature Mask: 0x%016llx\n", value64);

	if (adev->asic_type > CHIP_VEGA20) {
		/* VCN clocks */
		if (!amdgpu_dpm_read_sensor(adev, AMDGPU_PP_SENSOR_VCN_POWER_STATE, (void *)&value, &size)) {
			if (!value) {
				seq_printf(m, "VCN: Disabled\n");
			} else {
				seq_printf(m, "VCN: Enabled\n");
				if (!amdgpu_dpm_read_sensor(adev, AMDGPU_PP_SENSOR_UVD_DCLK, (void *)&value, &size))
					seq_printf(m, "\t%u MHz (DCLK)\n", value/100);
				if (!amdgpu_dpm_read_sensor(adev, AMDGPU_PP_SENSOR_UVD_VCLK, (void *)&value, &size))
					seq_printf(m, "\t%u MHz (VCLK)\n", value/100);
			}
		}
		seq_printf(m, "\n");
	} else {
		/* UVD clocks */
		if (!amdgpu_dpm_read_sensor(adev, AMDGPU_PP_SENSOR_UVD_POWER, (void *)&value, &size)) {
			if (!value) {
				seq_printf(m, "UVD: Disabled\n");
			} else {
				seq_printf(m, "UVD: Enabled\n");
				if (!amdgpu_dpm_read_sensor(adev, AMDGPU_PP_SENSOR_UVD_DCLK, (void *)&value, &size))
					seq_printf(m, "\t%u MHz (DCLK)\n", value/100);
				if (!amdgpu_dpm_read_sensor(adev, AMDGPU_PP_SENSOR_UVD_VCLK, (void *)&value, &size))
					seq_printf(m, "\t%u MHz (VCLK)\n", value/100);
			}
		}
		seq_printf(m, "\n");

		/* VCE clocks */
		if (!amdgpu_dpm_read_sensor(adev, AMDGPU_PP_SENSOR_VCE_POWER, (void *)&value, &size)) {
			if (!value) {
				seq_printf(m, "VCE: Disabled\n");
			} else {
				seq_printf(m, "VCE: Enabled\n");
				if (!amdgpu_dpm_read_sensor(adev, AMDGPU_PP_SENSOR_VCE_ECCLK, (void *)&value, &size))
					seq_printf(m, "\t%u MHz (ECCLK)\n", value/100);
			}
		}
	}

	return 0;
}

static void amdgpu_parse_cg_state(struct seq_file *m, u32 flags)
{
	int i;

	for (i = 0; clocks[i].flag; i++)
		seq_printf(m, "\t%s: %s\n", clocks[i].name,
			   (flags & clocks[i].flag) ? "On" : "Off");
}

static int amdgpu_debugfs_pm_info(struct seq_file *m, void *data)
{
	struct drm_info_node *node = (struct drm_info_node *) m->private;
	struct drm_device *dev = node->minor->dev;
	struct amdgpu_device *adev = dev->dev_private;
	u32 flags = 0;
	int r;

	r = pm_runtime_get_sync(dev->dev);
	if (r < 0)
		return r;

	amdgpu_device_ip_get_clockgating_state(adev, &flags);
	seq_printf(m, "Clock Gating Flags Mask: 0x%x\n", flags);
	amdgpu_parse_cg_state(m, flags);
	seq_printf(m, "\n");

	if (!adev->pm.dpm_enabled) {
		seq_printf(m, "dpm not enabled\n");
		pm_runtime_mark_last_busy(dev->dev);
		pm_runtime_put_autosuspend(dev->dev);
		return 0;
	}

	if (!is_support_sw_smu(adev) &&
	    adev->powerplay.pp_funcs->debugfs_print_current_performance_level) {
		mutex_lock(&adev->pm.mutex);
		if (adev->powerplay.pp_funcs->debugfs_print_current_performance_level)
			adev->powerplay.pp_funcs->debugfs_print_current_performance_level(adev, m);
		else
			seq_printf(m, "Debugfs support not implemented for this asic\n");
		mutex_unlock(&adev->pm.mutex);
		r = 0;
	} else {
		r = amdgpu_debugfs_pm_info_pp(m, adev);
	}

	pm_runtime_mark_last_busy(dev->dev);
	pm_runtime_put_autosuspend(dev->dev);

	return r;
}

static const struct drm_info_list amdgpu_pm_info_list[] = {
	{"amdgpu_pm_info", amdgpu_debugfs_pm_info, 0, NULL},
};
#endif

static int amdgpu_debugfs_pm_init(struct amdgpu_device *adev)
{
#if defined(CONFIG_DEBUG_FS)
	return amdgpu_debugfs_add_files(adev, amdgpu_pm_info_list, ARRAY_SIZE(amdgpu_pm_info_list));
#else
	return 0;
#endif
}
