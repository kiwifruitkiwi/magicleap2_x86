/*
 * Copyright (C) 2019-2021 Advanced Micro Devices, Inc. All rights reserved.
 */

#include <linux/irqdomain.h>
#include <linux/pm_domain.h>
#include <linux/platform_device.h>
#include <sound/designware_i2s.h>
#include <sound/pcm.h>
#include <linux/firmware.h>

#include "amdgpu_smu.h"
#include "atom.h"
#include "amdgpu_isp.h"
#include "smu_internal.h"
#include "smu11_driver_if_mero.h"
#include "smu_v11_5_ppsmc.h"
#include "smu_v11_5_pmfw.h"
#define ISP_FIRMWARE_MERO		"amdgpu/mero_isp.bin"


static struct amdgpu_device *l_amdgpu_dev;

enum isp_result {
	ISP_OK = 0,		/*STATUS_SUCCESS */
	ISP_ERROR_GENERIC = 1,	/*some unknown error happen */
	ISP_ERROR_INVALIDPARAMS = 2,	/*input or output parameter error */
};

static int isp_load_fw_by_psp(struct amdgpu_device *adev)
{
	int r;
	const char *fw_name;
	const struct common_firmware_header *hdr;

	switch (adev->asic_type) {
	case CHIP_MERO:
		fw_name = ISP_FIRMWARE_MERO;
		break;
	default:
		pr_err("amdgpu_isp:not mero, no load FW\n");
		return 0;
	}
	r = request_firmware(&adev->isp.fw, fw_name, adev->dev);
	if (r) {
		if (adev->firmware.load_type == AMDGPU_FW_LOAD_PSP) {
			pr_err("amdgpu_isp:load fw %s fail with %i\n",
				fw_name,
				r);
			return -EINVAL;
		}
		pr_info("amdgpu_isp:load fw %s fail, ignore\n", fw_name);
		return 0;
	}

	hdr = (const struct common_firmware_header *)adev->isp.fw->data;

	if (adev->isp.fw->size != le32_to_cpu(hdr->size_bytes)) {
		pr_err("amdgpu_isp:bad fw size %u != %u\n",
			(uint32_t)adev->isp.fw->size,
			le32_to_cpu(hdr->size_bytes));
		return -EINVAL;
	}

	pr_err("amdgpu_isp:load %s suc, %p[%d]\n", fw_name,
		adev->isp.fw->data, (int)adev->isp.fw->size);
	adev->firmware.ucode[AMDGPU_UCODE_ID_ISP].ucode_id =
			AMDGPU_UCODE_ID_ISP;
	adev->firmware.ucode[AMDGPU_UCODE_ID_ISP].fw = adev->isp.fw;
	adev->firmware.fw_size +=
		ALIGN(le32_to_cpu(hdr->ucode_size_bytes), PAGE_SIZE);
	return 0;
}

int get_cvip_buff(unsigned long *size, uint64_t *gpu_addr, void **cpu_addr)
{
	int ret = 0;

#if	defined(EN_CVIP_SHARE_BUF_ALLOC)
	struct amdgpu_bo *bo = NULL;

	if (amdgpu_bo_create_from_cvip(l_amdgpu_dev, size, &bo, gpu_addr,
				       cpu_addr)) {
		pr_err("%s fail by amdgpu_bo_create_from_cvip fail", __func__);
		ret = -1;
	}
#else
	pr_err("%s fail, no enable cvip share buf alloc!", __func__);
	ret = -1;
#endif

	return ret;
}

int set_isp_power(uint8_t enable)
{
	struct smu_context *smu = NULL;
	int ret = 0;

	smu = mero_get_smu_context(smu);
	if (!smu)
		return -1;

	if (enable) {
		ret = smu_send_smc_msg_with_param(smu,
				SMU_MSG_PowerUpIspByTile,
				TILE_SEL_ISPX | TILE_SEL_ISPM
				| TILE_SEL_ISPC | TILE_SEL_ISPPRE
				| TILE_SEL_ISPPOST,
				NULL);
	} else {
		ret = smu_send_smc_msg_with_param(smu,
				SMU_MSG_PowerDownIspByTile,
				TILE_SEL_ISPX | TILE_SEL_ISPM
				| TILE_SEL_ISPC | TILE_SEL_ISPPRE
				| TILE_SEL_ISPPOST,
				NULL);
	}

	if (ret) {
		pr_err("%s, enable:%d, ret:%d",
			__func__, enable, ret);
	}

	return ret;
}

int set_isp_clock(unsigned int sclk, unsigned int iclk,
		unsigned int xclk)
{
	struct smu_context *smu = NULL;
	int ret = 0;

	smu = mero_get_smu_context(smu);
	if (!smu)
		return -1;

	ret = smu_send_smc_msg_with_param(smu,
			SMU_MSG_SetHardMinSocclkByFreq,
			sclk, NULL);

	if (ret) {
		pr_err("set socclk fail :%d\n", ret);
		return -1;
	}

	ret = smu_send_smc_msg_with_param(smu,
			SMU_MSG_SetHardMinIspxclkByFreq,
			xclk, NULL);

	if (ret) {
		pr_err("set Isp xclk fail :%d\n", ret);
		return -1;
	}

	ret = smu_send_smc_msg_with_param(smu,
			SMU_MSG_SetHardMinIspiclkByFreq,
			iclk, NULL);

	if (ret) {
		//set iclk fail
		pr_err("set Isp iclk fail :%d\n", ret);
	}

	return ret;
}

static int isp_sw_init(void *handle)
{
	struct swisp_services svr;
	struct amdgpu_device *adev = (struct amdgpu_device *)handle;
	int ret;

	adev->isp.parent = adev->dev;
	adev->isp.cgs_device = amdgpu_cgs_create_device(adev);
	if (!adev->isp.cgs_device)
		return -EINVAL;

	l_amdgpu_dev = adev;
	svr.get_cvip_buf = get_cvip_buff;
#if	defined(EN_CVIP_SHARE_BUF_ALLOC)
	svr.cvip_set_gpuva = cvip_set_gpuva;
#else
	svr.cvip_set_gpuva = NULL;
#endif
	svr.set_isp_power = set_isp_power;
	svr.set_isp_clock = set_isp_clock;
	isp_init(adev->isp.cgs_device, (void *)&svr);
	ret = isp_load_fw_by_psp(adev);

	return ret;
}

static int isp_sw_fini(void *handle)
{
	struct amdgpu_device *adev = (struct amdgpu_device *)handle;

	if (adev->isp.cgs_device)
		amdgpu_cgs_destroy_device(adev->isp.cgs_device);
	if (adev->isp.fw)
		release_firmware(adev->isp.fw);
	return 0;
}

/**
 * isp_hw_init - start and test isp block
 *
 * @adev: amdgpu_device pointer
 *
 */
static int isp_hw_init(void *handle)
{
	return 0;
}

/**
 * isp_hw_fini - stop the hardware block
 *
 * @adev: amdgpu_device pointer
 *
 */
static int isp_hw_fini(void *handle)
{
	return 0;
}

static int isp_suspend(void *handle)
{
	return 0;
}

static int isp_resume(void *handle)
{
	return 0;
}

static int isp_early_init(void *handle)
{
	return 0;
}

static bool isp_is_idle(void *handle)
{
	return true;
}

static int isp_wait_for_idle(void *handle)
{
	return 0;
}

static int isp_soft_reset(void *handle)
{
	return 0;
}

static int isp_set_clockgating_state(void *handle,
				     enum amd_clockgating_state state)
{
	return 0;
}

static int isp_set_powergating_state(void *handle,
				     enum amd_powergating_state state)
{
	return 0;
}

static const struct amd_ip_funcs isp_ip_funcs = {
	.name = "isp_ip",
	.early_init = isp_early_init,
	.late_init = NULL,
	.sw_init = isp_sw_init,
	.sw_fini = isp_sw_fini,
	.hw_init = isp_hw_init,
	.hw_fini = isp_hw_fini,
	.suspend = isp_suspend,
	.resume = isp_resume,
	.is_idle = isp_is_idle,
	.wait_for_idle = isp_wait_for_idle,
	.soft_reset = isp_soft_reset,
	.set_clockgating_state = isp_set_clockgating_state,
	.set_powergating_state = isp_set_powergating_state,
};

#if defined(AMD_ISP3)
const struct amdgpu_ip_block_version isp_v3_0_ip_block = {
	.type = AMD_IP_BLOCK_TYPE_ISP,
	.major = 3,
	.minor = 0,
	.rev = 0,
	.funcs = &isp_ip_funcs,
};
#elif defined(AMD_ISP2)
const struct amdgpu_ip_block_version isp_v2_3_ip_block = {
	.type = AMD_IP_BLOCK_TYPE_ISP,
	.major = 2,
	.minor = 3,
	.rev = 0,
	.funcs = &isp_ip_funcs,
};
#endif
