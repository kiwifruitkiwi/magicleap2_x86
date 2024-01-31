/*
 * Copyright (C) 2019-2020 Advanced Micro Devices, Inc. All rights reserved.
 */

#ifndef BUFFER_MGR_H
#define BUFFER_MGR_H

enum isp_buffer_status {
	ISP_BUFFER_STATUS_FREE,
	ISP_BUFFER_STATUS_IN_FW,
	ISP_BUFFER_STATUS_MAX
};

struct isp_img_buf_info {
	struct isp_img_buf_info *next;
	void *virtual_addr;
	unsigned long long phy_addr;
	unsigned int size;
	unsigned short camera_id;
	unsigned short stream_id;
	enum isp_buffer_status buf_status;
	/*struct sys_img_buf_handle *typed */
	struct sys_img_buf_handle *sys_img_buf_hdl;
	/*struct isp_img_buf_info *next; */
};

#endif
