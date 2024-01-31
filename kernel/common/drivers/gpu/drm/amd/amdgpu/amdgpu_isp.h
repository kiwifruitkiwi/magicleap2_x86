/*
 * Copyright (C) 2019-2020 Advanced Micro Devices, Inc. All rights reserved.
 */

#ifndef __AMDGPU_ISP_H__
#define __AMDGPU_ISP_H__

#include <linux/mfd/core.h>
#include "amdgpu_smu.h"

struct amdgpu_isp {
	struct device *parent;
	struct cgs_device *cgs_device;
	const struct firmware	*fw;	/* ISP firmware */
};

typedef int (*func_get_cvip_buf)(unsigned long *size, uint64_t *gpu_addr,
				 void **cpu_addr);
typedef int (*func_cvip_set_gpuva)(uint64_t gpuva);

// need separate TILE per power type.
//typedef int (*func_set_isp_power)(uint8_t enable, uint8_t selection);
typedef int (*func_set_isp_power)(uint8_t enable);
typedef int (*func_set_isp_clock)(unsigned int sclk, unsigned int iclk,
		unsigned int xclk);

struct swisp_services {
	func_get_cvip_buf get_cvip_buf;
	func_cvip_set_gpuva cvip_set_gpuva;
	func_set_isp_power set_isp_power;
	func_set_isp_clock set_isp_clock;
};

int isp_init(struct cgs_device *cgs_dev, void *svr);
#if defined(AMD_ISP3)
extern const struct amdgpu_ip_block_version isp_v3_0_ip_block;
#elif defined(AMD_ISP2)
extern const struct amdgpu_ip_block_version isp_v2_3_ip_block;
#endif

int amdgpu_bo_create_from_cvip(struct amdgpu_device *adev, unsigned long *size,
			       struct amdgpu_bo **bo_ptr, uint64_t *gpu_addr,
			       void **cpu_addr);

int cvip_set_gpuva(uint64_t gpuva);
extern void *mero_get_smu_context(struct smu_context *smu);


#endif /* __AMDGPU_ISP_H__ */
