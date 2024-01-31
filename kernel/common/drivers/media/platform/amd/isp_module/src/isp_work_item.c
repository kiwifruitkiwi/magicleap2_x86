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

#include "isp_common.h"
#include "isp_fw_if.h"
#include "log.h"
#include "isp_soc_adpt.h"

struct isp_work_item *isp_work_item_create(struct isp_context *isp,
					enum isp_work_id id,
					enum camera_id cam_id,
					uint32 para_size)
{
	struct isp_work_item *ret;

	ret = (struct isp_work_item *)
	isp_sys_mem_alloc(sizeof(struct isp_work_item) + para_size);
	if (ret) {
		ret->id = id;
		ret->isp = isp;
		ret->cam_id = cam_id;
		ret->next.next = NULL;
		ret->para =
		((unsigned char *)ret) + sizeof(struct isp_work_item);
	} else {
		isp_dbg_print_err("-><- %s fail", __func__);
	}
	return ret;
}

void isp_work_item_destroy(struct isp_work_item *item)
{
	if (item)
		isp_sys_mem_free(item);
}

void isp_work_item_insert(struct isp_context *isp, struct list_node *node)
{
	if ((isp == NULL) || (node == NULL)) {
		isp_dbg_print_err
		("-><- %s fail for bad para\n", __func__);
		return;
	}
	isp_list_insert_tail(&isp->work_item_list, node);
	isp_event_signal(0, &isp->work_item_thread.wakeup_evt);
}

void isp_work_item_get_snr_res_fps_info(struct isp_context *isp_context,
					enum camera_id cam_id)
{
	if (!is_para_legal(isp_context, cam_id)) {
		isp_dbg_print_err
	    ("-><- %s fail for bad para\n", __func__);
		return;
	};
	if (!isp_context->snr_res_fps_info_got[cam_id]) {
		struct sensor_res_fps_t *res_fps;

		isp_dbg_print_info("-> %s\n", __func__);

		res_fps =
		isp_context->p_sensor_ops[cam_id]->get_res_fps(cam_id);
		if (res_fps) {
			uint32 i;

			memcpy(&isp_context->sensor_info[cam_id].res_fps,
				 res_fps, sizeof(struct sensor_res_fps_t));
			isp_dbg_print_info
		("in %s,cam_id:%d,count:%d\n",
			 __func__, cam_id, res_fps->count);
			for (i = 0; i < res_fps->count; i++) {
				isp_dbg_print_info
				("idx:%d,fps:%d,w:%d,h:%d\n", i,
				 res_fps->res_fps[i].fps,
				 res_fps->res_fps[i].width,
				 res_fps->res_fps[i].height);
			}
		} else {
			isp_dbg_print_err
		("in %s,cam_id:%d, no fps\n",
			 __func__, cam_id);
		}

		isp_context->snr_res_fps_info_got[cam_id] = 1;
		isp_dbg_print_info("<- %s\n", __func__);
	} else {
		isp_dbg_print_info
		("-><- %s, do nothing\n", __func__);
	}
}


void work_item_function(struct isp_context *isp_context, int32 stop)
{
	struct isp_work_item *item = NULL;

	if (isp_context == NULL)
		return;
	do {
		if (item) {
			switch (item->id) {
			/*
			 *case ISP_WORK_ID_SEND_PS_BUF:
			 *isp_work_item_send_ps_buf(item, stop);
			 *break;
			 */
			case ISP_WORK_ID_GET_SNR_INFO:
				if (!stop)
					isp_work_item_get_snr_res_fps_info
					(isp_context, item->cam_id);
				break;
			case ISP_WORK_ID_RESUME_STREAM:
				/*
				 *if (!stop)
				 *isp_work_item_resume_str(isp_context,
				 *(enum
				 *camera_id)((uint64) item->para));
				 */
				break;
			default:
				isp_dbg_print_info
				("in %s,bad id %d\n",
				 __func__, item->id);
				break;
			}
			isp_work_item_destroy(item);
		}
		item = isp_work_item_get_nxt(&isp_context->work_item_list);
	} while (item);
}
