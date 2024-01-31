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

#ifndef ISP_MC_ADDR_MGR_H
#define ISP_MC_ADDR_MGR_H

typedef void *isp_fw_work_buf_handle;
/*don't use 0 as start*/

/*the work buffer has the fixed size of 2M*/
isp_fw_work_buf_handle isp_fw_work_buf_init(uint64 sys_addr, uint64 mc_addr);
void isp_fw_work_buf_unit(isp_fw_work_buf_handle handle);
void isp_fw_buf_get_code_base(isp_fw_work_buf_handle hdl, uint64 *sys_addr,
			uint64 *mc_addr, uint32 *len);
void isp_fw_buf_get_stack_base(isp_fw_work_buf_handle hdl, uint64 *sys_addr,
			uint64 *mc_addr, uint32 *len);
void isp_fw_buf_get_heap_base(isp_fw_work_buf_handle hdl, uint64 *sys_addr,
			uint64 *mc_addr, uint32 *len);
void isp_fw_buf_get_trace_base(isp_fw_work_buf_handle hdl, uint64 *sys_addr,
			uint64 *mc_addr, uint32 *len);
void isp_fw_buf_get_cmd_base(isp_fw_work_buf_handle hdl,
			enum fw_cmd_resp_stream_id id,
			uint64 *sys_addr, uint64 *mc_addr, uint32 *len);
void isp_fw_buf_get_resp_base(isp_fw_work_buf_handle hdl,
			enum fw_cmd_resp_stream_id id,
			uint64 *sys_addr, uint64 *mc_addr,
			uint32 *len);
void isp_fw_buf_get_cmd_pl_base(isp_fw_work_buf_handle hdl, uint64 *sys_addr,
				uint64 *mc_addr, uint32 *len);

result_t isp_fw_get_nxt_cmd_pl(isp_fw_work_buf_handle hdl, uint64 *sys_addr,
			uint64 *mc_addr, uint32 *len);
result_t isp_fw_ret_pl(isp_fw_work_buf_handle hdl, uint64 mc_addr);

uint64 isp_fw_buf_get_sys_from_mc(isp_fw_work_buf_handle hdl, uint64 mc_addr);

#endif
