/*
 * Copyright (C) 2019-2020 Advanced Micro Devices, Inc. All rights reserved.
 */

#ifndef ISP_SOC_ADPT_H
#define ISP_SOC_ADPT_H

#include "isp_common.h"

struct isp_gpu_mem_info *isp_gpu_mem_alloc(unsigned int size,
	enum isp_gpu_mem_type mem_src);
int isp_gpu_mem_free(struct isp_gpu_mem_info *mem_info);

int isp_soc_fw_work_mem_rsv(struct isp_context *isp_cntxt,
				 unsigned long long *sys_addr,
				 unsigned long long *mc_addr);
int isp_fw_work_mem_free(struct isp_context *isp_cntxt);

#endif
