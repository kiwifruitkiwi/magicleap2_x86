/**************************************************************************
 *copyright 2014~2015 advanced micro devices, inc.
 *
 *permission is hereby granted, free of charge, to any person obtaining a
 *copy of this software and associated documentation files (the "software"),
 *to deal in the software without restriction, including without limitation
 *the rights to use, copy, modify, merge, publish, distribute, sublicense,
 *and/or sell copies of the software, and to permit persons to whom the
 *software is furnished to do so, subject to the following conditions:
 *
 *the above copyright notice and this permission notice shall be included in
 *all copies or substantial portions of the software.
 *
 *THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL
 *THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 *OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 *ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 *OTHER DEALINGS IN THE SOFTWARE.
 *
 *************************************************************************
 */

#include "isp_soc_adpt.h"
#include "isp_common.h"
#include "log.h"
#include "isp_fw_if.h"

struct isp_gpu_mem_info *isp_gpu_mem_alloc(uint32 size,
					 enum isp_gpu_mem_type mem_src)
{
	struct isp_gpu_mem_info *mem_info;
	enum cgs_result result;
	enum cgs_gpu_mem_type memory_type;
	cgs_handle_t cgs_handle = 0;
	uint64 gpu_mc_addr;
	void *sys_addr;

	if ((size == 0) || (g_cgs_srv->ops->alloc_gpu_mem == NULL)) {
		isp_dbg_print_err
			("-><- %s, illegal para %d\n", __func__, size);
		return NULL;
	};

	switch (mem_src) {
	case ISP_GPU_MEM_TYPE_NLFB:
		memory_type =
			CGS_GPU_MEM_TYPE__GART_WRITECOMBINE;
		break;
	case ISP_GPU_MEM_TYPE_LFB:
		memory_type = CGS_GPU_MEM_TYPE__VISIBLE_FB;
		break;
	default:
		isp_dbg_print_err("-><- %s, bad mem_src %d\n",
			__func__, mem_src);
		return NULL;
	};
	/*isp_dbg_print_info("-> isp_gpu_mem_alloc,type %d\n", mem_src);*/


	mem_info = isp_sys_mem_alloc(sizeof(struct isp_gpu_mem_info));
	if (mem_info == NULL) {
		isp_dbg_print_err
		("-><- %s,fail for isp_sys_mem_alloc fail\n", __func__);
		return NULL;
	};



	result = g_cgs_srv->ops->alloc_gpu_mem(g_cgs_srv,
		memory_type, size, ISP_MC_ADDR_ALIGN, 0,
		(uint64)4 * 1024 * 1024 * 1024, &cgs_handle);


	if (result != CGS_RESULT__OK) {
		isp_sys_mem_free(mem_info);
		isp_dbg_print_err
		("-><- %s, fail for alloc_gpu_mem fail %d\n",
			__func__, result);
		return NULL;
	}

	result = g_cgs_srv->ops->gmap_gpu_mem(g_cgs_srv, cgs_handle,
		&gpu_mc_addr);
	if (result != CGS_RESULT__OK) {
		isp_dbg_print_err
		("-><- %s, fail for gmap_gpu_mem fail %d\n",
			__func__, result);
		return NULL;
	};

	result = g_cgs_srv->ops->kmap_gpu_mem(g_cgs_srv, cgs_handle,
		&sys_addr);
	if (result != CGS_RESULT__OK) {
		isp_dbg_print_err
		("-><- %s, fail for kmap_gpu_mem fail %d\n",
			__func__, result);
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
		isp_dbg_print_err
		("-><- %s, fail size not equal %u(%lld)\n",
			__func__, size, mem_info->mem_size);
	} else {
		isp_dbg_print_info
		("-><- %s,succ, mc %llu, sysaddr %p(%lld)\n",
		__func__, mem_info->gpu_mc_addr,
		mem_info->sys_addr, mem_info->mem_size);
	}

	return mem_info;
};

result_t isp_gpu_mem_free(struct isp_gpu_mem_info *mem_info)
{
	enum cgs_result result;

	if ((mem_info == NULL) || (g_cgs_srv->ops->free_gpu_mem == NULL)) {
		isp_dbg_print_err("-><- %s, illegal para\n", __func__);
		return RET_FAILURE;
	};

	result =
		g_cgs_srv->ops->free_gpu_mem(g_cgs_srv, mem_info->handle);
	if (result != CGS_RESULT__OK) {
		isp_sys_mem_free(mem_info);
		isp_dbg_print_err
	("<- %s, fail for cgs_releasegpumemory fail %d\n",
			__func__, result);
		return RET_FAILURE;
	}

	isp_sys_mem_free(mem_info);

	return RET_SUCCESS;
};

int32 set_preview_buf_imp_soc(pvoid context, enum camera_id cam_id,
			sys_img_buf_handle_t buf_hdl)
{
	unreferenced_parameter(context);
	unreferenced_parameter(cam_id);
	unreferenced_parameter(buf_hdl);
	return IMF_RET_FAIL;
}
