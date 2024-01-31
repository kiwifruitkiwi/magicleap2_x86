/*
 * Copyright (C) 2019-2020 Advanced Micro Devices, Inc. All rights reserved.
 */

#ifndef _ISP_WORK_ITEM_H_
#define _ISP_WORK_ITEM_H_

#include "os_base_type.h"
#include "isp_queue.h"

struct isp_context;

#define isp_work_item_get_nxt(list) \
			((struct isp_work_item *)isp_list_get_first(list))
enum isp_work_id {
	ISP_WORK_ID_SEND_PS_BUF,	/*send photo sequence buffer */
	ISP_WORK_ID_GET_SNR_INFO,
	ISP_WORK_ID_RESUME_STREAM
};

struct isp_work_item {
	struct list_node next;
	enum isp_work_id id;
	enum camera_id cam_id;
	struct isp_context *isp;
	void *para;
};

struct isp_ps_para_in {
	struct sys_img_buf_handle *main_jpg_hdl;
	struct sys_img_buf_handle *emb_thumb_hdl;
	struct sys_img_buf_handle *standalone_thumb_hdl;
	struct sys_img_buf_handle *yuv_out_hdl;
	unsigned short cam_id;
};

struct isp_work_item *isp_work_item_create(struct isp_context *isp,
					 enum isp_work_id id,
					 enum camera_id cam_id,
					 unsigned int para_size);
void isp_work_item_destroy(struct isp_work_item *item);
/*isp work item send photo sequence buffer*/
void isp_work_item_send_ps_buf(struct isp_work_item *item, int stop);
void isp_work_item_get_snr_res_fps_info(struct isp_context *isp_context,
					enum camera_id cam_id);
void isp_work_item_resume_str(struct isp_context *isp_context,
			enum camera_id cam_id);
void work_item_function(struct isp_context *isp_context, int stop);

#endif
