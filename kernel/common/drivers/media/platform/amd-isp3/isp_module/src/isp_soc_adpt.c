/*
 * Copyright (C) 2019-2021 Advanced Micro Devices, Inc. All rights reserved.
 */

#include "isp_soc_adpt.h"
#include "isp_common.h"
#include "log.h"
#include "isp_fw_if.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "[ISP][isp_soc_adpt]"

struct isp_gpu_mem_info *isp_gpu_mem_alloc(unsigned int size,
					 enum isp_gpu_mem_type mem_src)
{
	struct isp_gpu_mem_info *mem_info;
	enum cgs_result result;
	enum cgs_gpu_mem_type memory_type;
	cgs_handle_t cgs_handle = 0;
	unsigned long long gpu_mc_addr;
	void *sys_addr;

	if ((size == 0) || (g_cgs_srv->ops->alloc_gpu_mem == NULL)) {
		ISP_PR_ERR("illegal para %d\n", size);
		return NULL;
	};

	switch (mem_src) {
	case ISP_GPU_MEM_TYPE_NLFB:
		memory_type =
		#if defined(ISP_BRINGUP_WORKAROUND)
			CGS_GPU_MEM_TYPE__VISIBLE_FB;
		#else
			CGS_GPU_MEM_TYPE__GART_WRITECOMBINE;
		#endif
		break;
	case ISP_GPU_MEM_TYPE_LFB:
		memory_type = CGS_GPU_MEM_TYPE__VISIBLE_FB;
		break;
	default:
		ISP_PR_ERR("bad mem_src %d\n", mem_src);
		return NULL;
	};
	/*ISP_PR_INFO("-> isp_gpu_mem_alloc,type %d\n", mem_src);*/


	mem_info = isp_sys_mem_alloc(sizeof(struct isp_gpu_mem_info));
	if (mem_info == NULL) {
		ISP_PR_ERR("fail for isp_sys_mem_alloc\n");
		return NULL;
	};



	result = g_cgs_srv->ops->alloc_gpu_mem(g_cgs_srv,
		memory_type, size, ISP_MC_ADDR_ALIGN, 0,
		(uint64_t) 256 * 1024 * 1024, &cgs_handle);


	if (result != CGS_RESULT__OK) {
		isp_sys_mem_free(mem_info);
		ISP_PR_ERR("fail for alloc_gpu_mem %d\n",
			result);
		return NULL;
	}

	result = g_cgs_srv->ops->gmap_gpu_mem(g_cgs_srv, cgs_handle,
		&gpu_mc_addr);
	if (result != CGS_RESULT__OK) {
		isp_sys_mem_free(mem_info);
		ISP_PR_ERR("fail for gmap_gpu_mem %d\n",
			result);
		return NULL;
	};

	result = g_cgs_srv->ops->kmap_gpu_mem(g_cgs_srv, cgs_handle,
		&sys_addr);
	if (result != CGS_RESULT__OK) {
		isp_sys_mem_free(mem_info);
		ISP_PR_ERR("fail for kmap_gpu_mem %d\n",
			result);
		return NULL;
	};

	mem_info->mem_src = mem_src;
	mem_info->mem_size = size;
	mem_info->handle = cgs_handle;
	mem_info->gpu_mc_addr = gpu_mc_addr;
	mem_info->sys_addr = sys_addr;
	mem_info->mem_handle = 0;
	mem_info->acc_handle = 0;

	if (size != mem_info->mem_size) {
		ISP_PR_ERR("fail size not equal %d(%llu)\n",
			size, mem_info->mem_size);
	} else {
		ISP_PR_INFO("suc, mc %llu, sysaddr %p(%llu)\n",
			mem_info->gpu_mc_addr,
			mem_info->sys_addr,
			mem_info->mem_size);
	}

	return mem_info;
};

int isp_gpu_mem_free(struct isp_gpu_mem_info *mem_info)
{
	enum cgs_result result;

	if ((mem_info == NULL) || (g_cgs_srv->ops->free_gpu_mem == NULL)) {
		ISP_PR_ERR("illegal para\n");
		return RET_FAILURE;
	};

	result =
		g_cgs_srv->ops->free_gpu_mem(g_cgs_srv, mem_info->handle);
	if (result != CGS_RESULT__OK) {
		isp_sys_mem_free(mem_info);
		ISP_PR_ERR("fail for cgs_releasegpumemory %d\n",
			result);
		return RET_FAILURE;
	}

	isp_sys_mem_free(mem_info);

	return RET_SUCCESS;
};

