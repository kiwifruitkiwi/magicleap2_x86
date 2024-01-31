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
 *FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 *THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 *OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 *ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 *OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

#ifndef _ISP_WORK_ITEM_H_
#define _ISP_WORK_ITEM_H_

#include "os_base_type.h"
#include "isp_queue.h"

struct isp_context;
void isp_work_item_insert(struct isp_context *isp, struct list_node *node);
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
	sys_img_buf_handle_t main_jpg_hdl;
	sys_img_buf_handle_t emb_thumb_hdl;
	sys_img_buf_handle_t standalone_thumb_hdl;
	sys_img_buf_handle_t yuv_out_hdl;
	uint16 cam_id;
};

struct isp_work_item *isp_work_item_create(struct isp_context *isp,
					 enum isp_work_id id,
					 enum camera_id cam_id,
					 uint32 para_size);
void isp_work_item_destroy(struct isp_work_item *item);
/*isp work item send photo sequence buffer*/
void isp_work_item_send_ps_buf(struct isp_work_item *item, int32 stop);
void isp_work_item_get_snr_res_fps_info(struct isp_context *isp_context,
					enum camera_id cam_id);
void isp_work_item_resume_str(struct isp_context *isp_context,
			enum camera_id cam_id);
void work_item_function(struct isp_context *isp_context, int32 stop);

#endif
