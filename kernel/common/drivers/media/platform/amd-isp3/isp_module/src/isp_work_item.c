/*
 * Copyright (C) 2019-2020 Advanced Micro Devices, Inc. All rights reserved.
 */

#include "isp_common.h"
#include "isp_fw_if.h"
#include "log.h"
#include "isp_soc_adpt.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "[ISP][isp_work_item]"

struct isp_work_item *isp_work_item_create(struct isp_context *isp,
					enum isp_work_id id,
					enum camera_id cam_id,
					unsigned int para_size)
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
		ISP_PR_ERR("fail\n");
	}
	return ret;
}

void isp_work_item_destroy(struct isp_work_item *item)
{
	if (item)
		isp_sys_mem_free(item);
}


void isp_work_item_get_snr_res_fps_info(struct isp_context *isp_context,
					enum camera_id cam_id)
{
	if (!is_para_legal(isp_context, cam_id)) {
		ISP_PR_ERR("fail for bad para\n");
		return;
	};
	if (!isp_context->snr_res_fps_info_got[cam_id]) {
		struct sensor_res_fps_t *res_fps;

		res_fps =
		isp_context->p_sensor_ops[cam_id]->get_res_fps(cam_id);
		if (res_fps) {
			unsigned int i;

			memcpy(&isp_context->sensor_info[cam_id].res_fps,
				 res_fps, sizeof(struct sensor_res_fps_t));
			ISP_PR_INFO("cam_id:%d,count:%d\n",
				cam_id, res_fps->count);
			for (i = 0; i < res_fps->count; i++) {
				ISP_PR_INFO
				("idx:%d,fps:%d,w:%d,h:%d\n", i,
				 res_fps->res_fps[i].fps,
				 res_fps->res_fps[i].width,
				 res_fps->res_fps[i].height);
			}
		} else {
			ISP_PR_ERR("cam_id:%d, no fps\n", cam_id);
		}

		isp_context->snr_res_fps_info_got[cam_id] = 1;
	} else {
		ISP_PR_INFO("exit do nothing\n");
	}
}


void work_item_function(struct isp_context *isp_context, int stop)
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
				 *camera_id)((unsigned long long) item->para));
				 */
				break;
			default:
				ISP_PR_ERR("bad id %d\n", item->id);
				break;
			}
			isp_work_item_destroy(item);
		}
		item = isp_work_item_get_nxt(&isp_context->work_item_list);
	} while (item);
}
