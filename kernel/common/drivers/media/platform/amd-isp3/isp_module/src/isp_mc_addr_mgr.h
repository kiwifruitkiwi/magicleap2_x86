/*
 * Copyright (C) 2019-2020 Advanced Micro Devices, Inc. All rights reserved.
 */

#ifndef ISP_MC_ADDR_MGR_H
#define ISP_MC_ADDR_MGR_H


/*don't use 0 as start*/

/*the work buffer has the fixed size of 2M*/
void *isp_fw_work_buf_init(
	unsigned long long sys_addr, unsigned long long mc_addr);
void isp_fw_work_buf_unit(void *handle);
void isp_fw_buf_get_code_base(
	void *hdl, unsigned long long *sys_addr,
	unsigned long long *mc_addr, unsigned int *len);
void isp_fw_buf_get_stack_base(
	void *hdl, unsigned long long *sys_addr,
			unsigned long long *mc_addr, unsigned int *len);
void isp_fw_buf_get_heap_base(
	void *hdl, unsigned long long *sys_addr,
	unsigned long long *mc_addr, unsigned int *len);
void isp_fw_buf_get_trace_base(
	void *hdl, unsigned long long *sys_addr,
	unsigned long long *mc_addr, unsigned int *len);
void isp_fw_buf_get_cmd_base(
	void *hdl,
	enum fw_cmd_resp_stream_id id,
	unsigned long long *sys_addr, unsigned long long *mc_addr,
	unsigned int *len);
void isp_fw_buf_get_resp_base(
	void *hdl,
	enum fw_cmd_resp_stream_id id,
	unsigned long long *sys_addr, unsigned long long *mc_addr,
	unsigned int *len);
void isp_fw_buf_get_cmd_pl_base(
	void *hdl, unsigned long long *sys_addr,
	unsigned long long *mc_addr, unsigned int *len);

int isp_fw_get_nxt_cmd_pl(
	void *hdl, unsigned long long *sys_addr,
	unsigned long long *mc_addr, unsigned int *len);
int isp_fw_ret_pl(void *hdl, unsigned long long mc_addr);

unsigned long long isp_fw_buf_get_sys_from_mc(
	void *hdl, unsigned long long mc_addr);

#endif
