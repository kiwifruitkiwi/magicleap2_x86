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
 **************************************************************************/

#ifndef ISP_SOC_ADPT_H
#define ISP_SOC_ADPT_H

#include "isp_common.h"

struct isp_gpu_mem_info *isp_gpu_mem_alloc(uint32 size,
	enum isp_gpu_mem_type mem_src);
result_t isp_gpu_mem_free(struct isp_gpu_mem_info *mem_info);

result_t isp_soc_fw_work_mem_rsv(struct isp_context *isp_cntxt,
				 uint64 *sys_addr, uint64 *mc_addr);
result_t isp_fw_work_mem_free(struct isp_context *isp_cntxt);

int32 set_preview_buf_imp_soc(pvoid context, enum camera_id cam_id,
			sys_img_buf_handle_t buf_hdl);

#endif
