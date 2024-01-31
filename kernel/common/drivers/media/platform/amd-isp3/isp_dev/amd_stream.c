/*
 * Copyright (C) 2019-2023 Advanced Micro Devices, Inc. All rights reserved.
 */

#include <linux/amd_cam_metadata.h>
#include <linux/errno.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/videodev2.h>
#include <linux/kobject.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/kdev_t.h>
#include <linux/uaccess.h>
#include <media/v4l2-common.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-device.h>
#include <media/v4l2-ioctl.h>
#include <media/v4l2-event.h>
#include <media/videobuf2-memops.h>
#include <media/videobuf2-core.h>
#include <media/videobuf2-v4l2.h>
#include <media/videobuf2-vmalloc.h>
#include <linux/cam_api.h>
#include <linux/random.h>

#include "amd_params.h"
#include "amd_stream.h"
#include "amd_isp.h"
#include "amd_isp_ioctl.h"
#include "sensor_if.h"
#include "isp_module_if.h"
#include "isp_fw_if/paramtypes.h"
#include "isp_soc_adpt.h"
#include "os_advance_type.h"
#include "log.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif

#define LOG_TAG "[ISP][amd_stream]"
//#define DUMP_IMG_TO_FILE
//#define ADVANCED_DEBUG

#define SEC_TO_NANO_SEC(num) (num * 1000000000)

static struct amd_cam *g_cam;
struct kset *kset_ref;
struct kobject *kobj_ref;
struct kobj_type ktype_ref;
static struct v4l2_subdev *get_isp_subdev(void);
unsigned int g_min_exp_time;
s32 *g_s32;
/*
 * dma buf memops
 */
void *amd_attach_dmabuf(struct device *dev, struct dma_buf *dbuf,
			unsigned long size,
			enum dma_data_direction dma_dir)
{
	struct dma_buf_attachment *dba;
	struct amd_dma_buf *buf;

	ENTER();

	if (dbuf->size < size)
		return ERR_PTR(-ENOMEM);

	buf = kzalloc(sizeof(*buf), GFP_KERNEL);
	if (!buf)
		return ERR_PTR(-ENOMEM);

	dba = dma_buf_attach(dbuf, buf->dev);
	if (IS_ERR(dba)) {
		ISP_PR_ERR("failed to attach dmabuf");
		kfree(buf);
		return dba;
	}

	buf->dma_dir = dma_dir;
	buf->size = size;
	buf->db_attach = dba;

	return buf;
}

void amd_detach_dmabuf(void *buf_priv)
{
	struct amd_dma_buf *buf = buf_priv;

	ENTER();

	dma_buf_detach(buf->db_attach->dmabuf, buf->db_attach);
	kfree(buf);
}

int amd_map_dmabuf(void *buf_priv)
{
	ENTER();

	/*
	 * check if there need extra map here
	 */
	return 0;
}

void amd_unmap_dmabuf(void *buf_priv)
{
	ENTER();
}

const struct vb2_mem_ops amd_dmabuf_ops = {
	.map_dmabuf = amd_map_dmabuf,
	.unmap_dmabuf = amd_unmap_dmabuf,
	.attach_dmabuf = amd_attach_dmabuf,
	.detach_dmabuf = amd_detach_dmabuf,
};

/*
 * Videobuf operations
 */
static int queue_setup(struct vb2_queue *vq,
	unsigned int *nbuffers, unsigned int *nplanes,
	unsigned int sizes[], struct device *alloc_devs[])
{
	struct physical_cam __maybe_unused *cam = vb2_get_drv_priv(vq);

	ISP_PR_DBG("cam[%d]", cam->idx);
	if (vq && vq->type != V4L2_BUF_TYPE_VIDEO_CAPTURE) {
		ISP_PR_ERR("don't support mplane: %d", vq->type);
		return -EINVAL;
	}

	*nbuffers = MAX_REQUEST_DEPTH;
	*nplanes = 1;
	sizes[0] = sizeof(struct frame_control);

	return 0;
}

static void map_user_request(struct physical_cam *cam, struct frame_control *fc)
{
	struct _FrameControl_t *f = &fc->fw_fc.frameCtrl;
	struct _FrameControlPrivateData_t *priv = &f->privateData;
	struct kernel_request *req = cam->req;

	fc->prf_id = req->prf_switch.id;
	if (g_prop->cvip_enable == CVIP_STATUS_ENABLE &&
	    req->prf_switch.trigger) {
		priv->data[SWITCH_SENSOR_PRF_IDX] = SENSOR_SWITCH_ENABLE;
		ISP_PR_INFO("com_id:%d fcid:%d CVIP sensor prf switch to %d",
			    req->com_req_id, f->fcId, req->prf_switch.id);
	} else {
		priv->data[SWITCH_SENSOR_PRF_IDX] = SENSOR_SWITCH_DISABLE;
	}

	if (cam->clk_tag == NEED_LOWER_CLK) {
		priv->data[SWITCH_LOW_CLK_IDX] = CLOCK_SWITCH_ENABLE;
		cam->clk_tag = NO_NEED_LOWER_CLK;
	} else {
		priv->data[SWITCH_LOW_CLK_IDX] = CLOCK_SWITCH_DISABLE;
	}

	priv->data[SWITCH_TUNING_DATA_IDX] = req->tun_switch;
	ISP_PR_DBG("com_id:%d fcid:%d set tuning data to %d",
		   req->com_req_id, f->fcId, req->tun_switch);
}

static int buffer_init(struct vb2_buffer *vb)
{
	u64 addr = 0;
	struct physical_cam __maybe_unused *cam =
		vb2_get_drv_priv(vb->vb2_queue);
	struct frame_control *fc = container_of(vb, struct frame_control, vb);

	ISP_PR_DBG("cam[%d]", cam->idx);
	fc->hal_md_buf_addr = 0;
	INIT_LIST_HEAD(&fc->entry);
	fc->rst.input_buffer = NULL;
	fc->mi_mem_info = isp_gpu_mem_alloc(sizeof(struct _MetaInfo_t),
					    ISP_GPU_MEM_TYPE_NLFB);
	if (fc->mi_mem_info) {
		fc->mi = (struct _MetaInfo_t *)(fc->mi_mem_info->sys_addr);
		memset(fc->mi, 0, sizeof(*fc->mi));
	} else {
		ISP_PR_ERR("alloc metaInfo fail!");
		return -ENOMEM;
	}

	addr = (u64)fc->mi_mem_info->gpu_mc_addr;
	fc->fw_fc.frameCtrl.metadata.bufTags = 0;
	fc->fw_fc.frameCtrl.metadata.vmidSpace.bit.vmid = 0;
	fc->fw_fc.frameCtrl.metadata.vmidSpace.bit.space =
		ADDR_SPACE_TYPE_GPU_VA;
	fc->fw_fc.frameCtrl.metadata.bufBaseALo = (u32)(addr & 0xffffffff);
	fc->fw_fc.frameCtrl.metadata.bufBaseAHi = (u32)(addr >> 32);
	fc->fw_fc.frameCtrl.metadata.bufSizeA = sizeof(*fc->mi);

	fc->eng_info = isp_gpu_mem_alloc(sizeof(struct _EngineerMetaInfo_t),
					 ISP_GPU_MEM_TYPE_NLFB);
	if (fc->eng_info) {
		fc->eng = (struct _EngineerMetaInfo_t *)
			(fc->eng_info->sys_addr);
		memset(fc->eng, 0, sizeof(*fc->eng));
	} else {
		ISP_PR_ERR("alloc eng Info gpu buffer fail!");
		return -ENOMEM;
	}

	ISP_PR_DBG("suc, cam[%d]", cam->idx);
	return OK;
}

static int buffer_prepare(struct vb2_buffer *vb)
{
	int i, ret = OK;
	struct physical_cam *cam = vb2_get_drv_priv(vb->vb2_queue);
	struct frame_control *fc = container_of(vb, struct frame_control, vb);
	struct _FrameControl_t *f = &fc->fw_fc.frameCtrl;
	struct _OutputBuf_t *obuf = &fc->fw_fc.frameCtrl.preview;

	ISP_PR_DBG("cam[%d]", cam->idx);
	if (cam->pre_pfc)
		memcpy(f, cam->pre_pfc, sizeof(*f) - sizeof(f->metadata));

	for (i = 0; i < FW_STREAM_TYPE_NUM; i++, obuf++)
		obuf->enabled = false;

	ret = store_request_info(cam, fc);
	if (ret != OK)
		goto quit;

	ret = store_latest_md(cam, (struct amd_cam_metadata *)cam->req->kparam);
	if (ret != OK)
		goto quit;

	ret = map_stream_buf(cam, fc);
	if (ret != OK)
		goto quit;

	map_user_request(cam, fc);
	ret = map_metadata_2_framectrl(cam, fc);
quit:
	return ret;
}

static void buffer_queue(struct vb2_buffer *vb)
{
	int ret = OK;
	struct isp_ctl_ctx ctx;
	unsigned long flags = 0;
	struct physical_cam *cam = vb2_get_drv_priv(vb->vb2_queue);
	struct frame_control *fc = container_of(vb, struct frame_control, vb);

	ISP_PR_DBG("cam[%d]", cam->idx);
	ctx.cam = cam;
	ctx.stream = NULL;
	ctx.ctx = &fc->fw_fc;
	ret = v4l2_subdev_call(cam->isp_sd, core, ioctl,
			       ISP_IOCTL_QUEUE_FRAME_CTRL, &ctx);
	if (ret == OK) {
		ISP_PR_DBG("isp queue buffer success!");
		cam->cnt_enq++;
		spin_lock_irqsave(&cam->b_lock, flags);
		list_add_tail(&fc->entry, &cam->enq_list);
		ISP_PR_VERB("Active: new=[%p], head=[%p], head->next=[%p]",
			    &fc->entry, &cam->enq_list, cam->enq_list.next);
		spin_unlock_irqrestore(&cam->b_lock, flags);
	} else {
		ISP_PR_ERR("cam[%d] que frame ctrl fail ret:%d", cam->idx, ret);
	}
}

static void buffer_finish(struct vb2_buffer *vb)
{
	struct physical_cam *cam = vb2_get_drv_priv(vb->vb2_queue);
	struct frame_control *fc = container_of(vb, struct frame_control, vb);

	ISP_PR_DBG("cam[%d]", cam->idx);
	cam->fc = fc;
}

static void buffer_cleanup(struct vb2_buffer *vb)
{
	struct physical_cam __maybe_unused *cam =
		vb2_get_drv_priv(vb->vb2_queue);
	struct frame_control *fc = container_of(vb, struct frame_control, vb);

	ISP_PR_DBG("cam[%d]", cam->idx);
	fc->rst.input_buffer = NULL;
	fc->mi = NULL;
	fc->hal_md_buf_addr = 0;

	if (fc->mi_mem_info) {
		isp_gpu_mem_free(fc->mi_mem_info);
		fc->mi_mem_info = NULL;
	}

	if (fc->eng_info) {
		fc->eng = NULL;
		isp_gpu_mem_free(fc->eng_info);
		fc->eng_info = NULL;
	}
}

static int start_streaming(struct vb2_queue *vq, unsigned int count)
{
	struct physical_cam __maybe_unused *cam = vb2_get_drv_priv(vq);

	ISP_PR_DBG("cam[%d]", cam->idx);
	return 0;
}

static void stop_streaming(struct vb2_queue *vq)
{
	int i;
	struct vb2_buffer *vb;
	struct frame_control *fc;
	struct physical_cam __maybe_unused *cam = vb2_get_drv_priv(vq);

	ISP_PR_DBG("cam=[%d]", cam->idx);
	for (i = 0; i < MAX_REQUEST_DEPTH; i++) {
		vb = vq->bufs[i];
		fc = container_of(vb, struct frame_control, vb);
		if (vb->state == VB2_BUF_STATE_ACTIVE) {
			ISP_PR_DBG("vb2 buf %p is active, done it", vb);
			list_del(&fc->entry);
			vb2_buffer_done(vb, VB2_BUF_STATE_ERROR);
		} else
			ISP_PR_DBG("vidq[%d] is not active", vb->index);
	}
}

static struct vb2_ops amd_qops = {
	.queue_setup = queue_setup,
	.buf_init = buffer_init,
	.buf_prepare = buffer_prepare,
	.buf_finish = buffer_finish,
	.buf_cleanup = buffer_cleanup,
	.buf_queue = buffer_queue,
	.start_streaming = start_streaming,
	.stop_streaming = stop_streaming,
	.wait_prepare = vb2_ops_wait_prepare,
	.wait_finish = vb2_ops_wait_finish,
};

/*
 * IOCTL vidioc handling
 */
static long _ioctl_default(struct file *f, void *fh, bool valid_prio,
			   unsigned int cmd, void *arg)
{
	int ret = OK;
	struct physical_cam *cam = NULL;
	struct s_properties *prop;
	struct kernel_request *req;
	struct kernel_result *rst;
	struct module_profile *cfg = NULL;

	assert(fh != NULL);
	assert(arg != NULL);

	cam = container_of(fh, struct physical_cam, fh);
	if (cam == NULL) {
		ISP_PR_ERR("get cam failed!");
		return -1;
	}

	switch (cmd) {
	case VIDIOC_SET_PROPERTY_LIST: {
		ISP_PR_DBG("cam[%d] cmd VIDIOC_SET_PROPERTY_LIST", cam->idx);
		prop = (struct s_properties *)arg;
		ret = set_prop(cam, prop);
		ISP_PR_DBG("VIDIOC_SET_PROPERTY_LIST done with ret %d", ret);
		break;
	}
	case VIDIOC_CONFIG: {
		ISP_PR_DBG("cam[%d] cmd VIDIOC_CONFIG", cam->idx);
		cfg = (struct module_profile *)arg;
		ret = sensor_config(cam, cfg);
		ISP_PR_DBG("VIDIOC_CONFIG done with ret %d", ret);
		break;
	}
	case VIDIOC_REQUEST: {
		ISP_PR_DBG("cam[%d] cmd VIDIOC_REQUEST", cam->idx);
		req = (struct kernel_request *)arg;
		ret = enq_request(cam, req);
		ISP_PR_DBG("VIDIOC_REQUEST done with ret %d", ret);
		break;
	}
	case VIDIOC_RESULT: {
		ISP_PR_DBG("cam[%d] cmd VIDIOC_RESULT", cam->idx);
		rst = (struct kernel_result *)arg;
		ret = deq_result(cam, rst, f);
		ISP_PR_DBG("VIDIOC_RESULT done with ret %d", ret);
		break;
	}
	default:
		ret = -1;
		ISP_PR_ERR("cam[%d] not support cmd %x", cam->idx, cmd);
		break;
	}

	return ret;
}

static void check_isp_clk_switch(struct physical_cam *cam)
{
	struct _FrameControl_t *f = &cam->fc->fw_fc.frameCtrl;

	if (cam->clk_tag == NEED_CHECK_RESP &&
	    f->privateData.data[SWITCH_SENSOR_PRF_IDX] == SENSOR_SWITCH_ENABLE)
		cam->clk_tag = NEED_LOWER_CLK;
}

int set_prop(struct physical_cam *cam, struct s_properties *prop)
{
	struct isp_ctl_ctx ctx;
	int ret = OK;

	*g_prop = *prop;
	if (prop->depth > MAX_REQUEST_DEPTH) {
		ISP_PR_WARN("prop->depth %d over the max %d", prop->depth,
			    MAX_REQUEST_DEPTH);
		prop->depth = MAX_REQUEST_DEPTH;
	}

	ISP_PR_PC("depth[%d] sensor_id[%d] profile[%d]",
		  g_prop->depth, g_prop->sensor_id, g_prop->profile);
	ISP_PR_PC("fw_log_en[%d] level[%llx] drv_log_level[%d] prefetch[0x%x]",
		  g_prop->fw_log_enable, g_prop->fw_log_level,
		  g_prop->drv_log_level, g_prop->prefetch_enable);
	ISP_PR_PC("phy_cali_value[%d] disable_isp_power_tile[%d] aec_mode[%d]",
		  g_prop->phy_cali_value, g_prop->disable_isp_power_tile,
		  g_prop->aec_mode);

	if (g_prop->cvip_enable == CVIP_STATUS_ENABLE) {
		ISP_PR_PC("cvip en[%d] raw snum[%d] o/p snum[%d] raw fmt[%d]",
			  g_prop->cvip_enable, g_prop->cvip_raw_slicenum,
			  g_prop->cvip_output_slicenum, g_prop->cvip_raw_fmt);
		ISP_PR_PC("cvip yuv resolution[%dx%d] fmt[%d] cvip_time[%d]",
			  g_prop->cvip_yuv_w, g_prop->cvip_yuv_h,
			  g_prop->cvip_yuv_fmt, g_prop->timestamp_from_cvip);
	}

	if (!cam->cam_init && cam->idx == 0) {
		ctx.cam = cam;
		ctx.stream = NULL;
		ctx.ctx = NULL;
		ret = v4l2_subdev_call(cam->isp_sd, core, ioctl,
				       ISP_IOCTL_INIT_CAM_DEV, &ctx);
		if (ret == OK)
			cam->cam_init = true;
		else
			ISP_PR_ERR("cam[%d] init fail, ret:%d", cam->idx, ret);
	}

	return OK;
}

int sensor_config(struct physical_cam *cam, struct module_profile *cfg)
{
	int ret = OK;
	struct isp_ctl_ctx ctx;
	enum mode_profile_name pre_id = g_profile_id[cam->idx];

	assert(cfg);
	/* check system property for profile id */
	if (g_prop->profile == PROFILE_INVALID || cfg->name == PROFILE_INVALID)
		g_profile_id[cam->idx] = cfg->name;
	else
		g_profile_id[cam->idx] = g_prop->profile;

	ISP_PR_PC("set cam[%d] sensor profile id from %d to %d", cam->idx,
		  pre_id, g_profile_id[cam->idx]);

	if (g_profile_id[cam->idx] == PROFILE_INVALID) {
		ret = sensor_close(cam);
	} else if (!cam->cam_on) {
		ret = sensor_open(cam);
	} else if (pre_id != g_profile_id[cam->idx]) {
		/* ISP FW cannot support CVIP sensor profile switch until enable
		 * calib, and the calib enable must be sent after the first
		 * PFC/request and before start stream, so the calib should be
		 * disabled when the cnt_enq is 0, so restart CVIP sensor
		 * instead of switching sensor profile when the cnt_enq is 0.
		 */
		if (cam->cnt_enq && g_prop->cvip_enable == CVIP_STATUS_ENABLE) {
			ctx.cam = cam;
			ctx.stream = NULL;
			ctx.ctx = &g_profile_id[cam->idx];
			ret = v4l2_subdev_call(cam->isp_sd, core, ioctl,
					       ISP_IOCTL_SWITCH_SENSOR_PROFILE,
					       &ctx);
			if (ret != OK) {
				ISP_PR_ERR("cam[%d] switch profile fail ret:%d",
					   cam->idx, ret);
				goto quit;
			} else {
				ISP_PR_PC("cam[%d] switch profile to %d suc",
					  cam->idx, g_profile_id[cam->idx]);
			}

			memset(cam->last_stream_info, 0,
			       sizeof(cam->last_stream_info));
			memset(cam->ping.md, 0, MAX_KERN_METADATA_BUF_SIZE);
			memset(cam->pong.md, 0, MAX_KERN_METADATA_BUF_SIZE);
			memset(cam->md, 0, MAX_KERN_METADATA_BUF_SIZE);

			if ((pre_id == PROFILE_NOHDR_NOBINNING_12MP_30 ||
			     pre_id == PROFILE_NOHDR_2X2BINNING_3MP_60) &&
			    (g_profile_id[cam->idx] !=
			     PROFILE_NOHDR_NOBINNING_12MP_30 &&
			     g_profile_id[cam->idx] !=
			     PROFILE_NOHDR_2X2BINNING_3MP_60))
				cam->clk_tag = NEED_CHECK_RESP;
		} else {
			cam->tear_down = true;
			ret = sensor_close(cam);
			if (ret != OK)
				goto quit;
			ret = sensor_open(cam);
		}
	} else {
		/* sensor already open, and profile no change, do nothing */
	}

quit:
	return ret;
}

int sensor_open(struct physical_cam *cam)
{
	int ret = OK;
	struct isp_ctl_ctx ctx;

	ISP_PR_DBG("Entry! cam[%d]", cam->idx);
	ret = vb2_streamon(&cam->vidq, cam->vidq.type);
	if (ret != OK) {
		ISP_PR_ERR("cam[%d] vb2q on fail! ret: %d", cam->idx, ret);
		goto quit;
	}
	ctx.cam = cam;
	ctx.stream = NULL;
	ctx.ctx = &g_profile_id[cam->idx];
	ret = v4l2_subdev_call(cam->isp_sd, core, ioctl, ISP_IOCTL_STREAM_OPEN,
			       &ctx);
	if (ret != OK) {
		ISP_PR_ERR("cam[%d] fail! ret:%d", cam->idx, ret);
	} else {
		ISP_PR_PC("cam[%d] suc", cam->idx);
		cam->cam_on = true;
		cam->af_trigger = AMD_CAM_CONTROL_AF_TRIGGER_IDLE;
		cam->ae_trigger = AMD_CAM_CONTROL_AE_PRECAPTURE_TRIGGER_IDLE;
		cam->af_state = AMD_CAM_CONTROL_AF_STATE_INACTIVE;
		cam->cur_cc_mode = AMD_CAM_COLOR_CORRECTION_MODE_FAST;
		cam->pre_cc_mode = AMD_CAM_COLOR_CORRECTION_MODE_FAST;
	}

quit:
	RET(ret);
	return ret;
}

int enq_request(struct physical_cam *cam, struct kernel_request *req)
{
	int ret = OK;
	struct v4l2_buffer vbuf;
	struct kernel_buffer kbuf;
	struct vb2_queue *q = &cam->vidq;
	struct isp_ctl_ctx ctx;

	assert(req);
	ISP_PR_DBG("Entry! cam[%d] com_id %d", cam->idx, req->com_req_id);

	/* start CVIP sensor before send 1st PFC to ISP FW */
	if (!cam->cnt_enq && g_prop->cvip_enable == CVIP_STATUS_ENABLE) {
		ctx.cam = cam;
		ctx.stream = NULL;
		ctx.ctx = NULL;
		if (v4l2_subdev_call(cam->isp_sd, core, ioctl,
				     ISP_IOCTL_START_CVIP_SENSOR, &ctx) != OK) {
			ISP_PR_ERR("cam[%d] fail! ret:%d", cam->idx, ret);
			return -EINVAL;
		}
	}

	cam->req = req;
	memset(&vbuf, 0, sizeof(struct v4l2_buffer));
	memset(&kbuf, 0, sizeof(kbuf));
	vbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	vbuf.memory = V4L2_MEMORY_USERPTR;
	vbuf.index = cam->fc_id;
	if (!req->output_buffers[0]) {
		ISP_PR_ERR("cam[%d] com_id %d output_buffers[0] is null",
			   cam->idx, req->com_req_id);
		return -EINVAL;
	}

	ret = copy_from_user(&kbuf, req->output_buffers[0], sizeof(kbuf));
	if (ret != OK) {
		ISP_PR_ERR("cam[%d] copy_from_user fail!", cam->idx);
		return ret;
	}

	if (!kbuf.user_ptr) {
		ISP_PR_ERR("cam[%d]com_id %d output_buf[0]->user_ptr is null",
			   cam->idx, req->com_req_id);
		return -EINVAL;
	}

	vbuf.m.userptr = (unsigned long)kbuf.user_ptr;
	vbuf.length = kbuf.length;

	//enqueue the buf to vb2q
	mutex_lock(q->lock);
	ret = vb2_qbuf(q, NULL, &vbuf);
	mutex_unlock(q->lock);
	if (ret == OK) {
		cam->fc_id = (++cam->fc_id == MAX_REQUEST_DEPTH) ?
			0 : cam->fc_id;
		ISP_PR_DBG("cam[%d] com_id %d vb2_qbuf suc",
			   cam->idx, req->com_req_id);
	} else {
		ISP_PR_ERR("cam[%d] com_id %d vb2_qbuf fail! ret:%d",
			   cam->idx, req->com_req_id, ret);
	}

	return ret;
}

/**
 * Compare new received PFC with the last one stored, if stream state or stream
 * parameter changes, will log it then update the last one
 *
 * @param cam camera info, new received PFC is stored in it.
 */
void update_stream_info(struct physical_cam *cam, struct frame_control *fc)
{
	int i;
	struct kernel_buffer *kbuf = NULL;
	struct stream_info tmp[MAX_STREAM_NUM_PER_PHYSICAL_CAMERA];
	struct stream_info *last = NULL;
	struct stream_info *cur = NULL;
	char *stream_name = NULL;

	assert(fc);
	memset(tmp, 0, sizeof(tmp));
	for (i = 0; i < cam->req->num_output_buffers; i++) {
		kbuf = &fc->output_buffers[i];
		tmp[kbuf->s_type].enable = 1;
		tmp[kbuf->s_type].width = kbuf->width;
		tmp[kbuf->s_type].height = kbuf->height;
		tmp[kbuf->s_type].y_stride = kbuf->y_stride;
		tmp[kbuf->s_type].uv_stride = kbuf->uv_stride;
		tmp[kbuf->s_type].fmt = kbuf->format;
	}
	for (i = STREAM_YUV_0; i <= STREAM_RAW; i++) {
		last = &cam->last_stream_info[i];
		cur = &tmp[i];
		if (!last->enable && !cur->enable)
			continue;
		switch (i) {
		case STREAM_YUV_0:
			stream_name = "STREAM_YUV_0(preview)";
			break;
		case STREAM_YUV_1:
			stream_name = "STREAM_YUV_1(still)";
			break;
		case STREAM_YUV_2:
			if (g_prop->cvip_enable == CVIP_STATUS_ENABLE)
				stream_name = "STREAM_YUV_2(x86cv)";
			else
				stream_name = "STREAM_YUV_2(video)";
			break;
		case STREAM_CVPCV:
			stream_name = "STREAM_CVPCV(cvpcv)";
			break;
		case STREAM_RAW:
			stream_name = "STREAM_RAW(raw)";
			break;
		default:
			stream_name = "fail unknown stream";
			break;
		}
		if (last->enable && !cur->enable) {
			ISP_PR_PC("cam[%i], %s disabled",
				  cam->idx,
				  stream_name);
			*last = *cur;
			continue;
		};
		if (!last->enable && cur->enable) {
			ISP_PR_PC("cam[%i], %s enabled, %u:%u %u %u %u",
				  cam->idx,
				  stream_name,
				  cur->width,
				  cur->height,
				  cur->y_stride,
				  cur->uv_stride,
				  cur->fmt);
			*last = *cur;
			continue;
		};
		//check if parameter changes
		if (last->width != cur->width ||
		    last->height != cur->height ||
		    last->y_stride != cur->y_stride ||
		    last->fmt != cur->fmt) {
			ISP_PR_PC("cam[%i], %s changed to %u:%u %u %u %u",
				  cam->idx,
				  stream_name,
				  cur->width,
				  cur->height,
				  cur->y_stride,
				  cur->uv_stride,
				  cur->fmt);
			*last = *cur;
			continue;
		}
	}
}

int map_stream_buf(struct physical_cam *cam, struct frame_control *fc)
{
	int i;
	struct kernel_buffer *kbuf = NULL;
	struct _FrameControl_t *f = &fc->fw_fc.frameCtrl;

	ISP_PR_DBG("Entry! cam[%d] com_id %d", cam->idx, cam->req->com_req_id);
	f->controlType = FRAME_CONTROL_TYPE_FULL;
	if (cam->cnt_enq == 0)
		cam->fc_id_offset = cam->req->com_req_id;

	f->fcId = cam->req->com_req_id - cam->fc_id_offset;
	update_stream_info(cam, fc);
	for (i = 0; i < cam->req->num_output_buffers; i++) {
		kbuf = &fc->output_buffers[i];
		if (kbuf->s_type == STREAM_YUV_0) {
			ISP_PR_DBG("c_%d s_%d preview", cam->idx, kbuf->s_type);
			map_img_addr(&f->preview, kbuf);
		} else if (kbuf->s_type == STREAM_YUV_1) {
			ISP_PR_DBG("c_%d s_%d still", cam->idx, kbuf->s_type);
			map_img_addr(&f->still, kbuf);
		} else if (kbuf->s_type == STREAM_YUV_2) {
			if (g_prop->cvip_enable == CVIP_STATUS_ENABLE) {
				ISP_PR_DBG("c_%d s_%d x86cv", cam->idx,
					   kbuf->s_type);
				map_img_addr(&f->x86cv, kbuf);
			} else {
				ISP_PR_DBG("c_%d s_%d video", cam->idx,
					   kbuf->s_type);
				map_img_addr(&f->video, kbuf);
			}
		} else if (kbuf->s_type == STREAM_CVPCV) {
			ISP_PR_DBG("c_%d s_%d cvpcv", cam->idx, kbuf->s_type);
			map_img_addr(&f->cvpcv, kbuf);
		} else if (kbuf->s_type == STREAM_RAW) {
			ISP_PR_DBG("c_%d s_%d raw", cam->idx, kbuf->s_type);
			map_img_addr(&f->raw, kbuf);
		} else {
			ISP_PR_ERR("c_%d s_%d unknown stream type", cam->idx,
				   kbuf->s_type);
		}
	}

	return OK;
}

void map_img_addr(struct _OutputBuf_t *obuf, struct kernel_buffer *kbuf)
{
	u64 addr = kbuf->mc_addr;
	u32 len = kbuf->length;
	struct amd_format *fmt = NULL;

	ISP_PR_DBG("Entry! s[%d]", kbuf->s_type);
	obuf->enabled = true;

	fmt = amd_find_match(kbuf->format);
	if (!fmt) {
		ISP_PR_WARN("invalid format parameter!! use default fmt");
		fmt = select_default_format();
	}

	obuf->imageProp.imageFormat = fmt->fw_fmt;
	obuf->imageProp.width = kbuf->width;
	obuf->imageProp.height = kbuf->height;
	obuf->imageProp.lumaPitch = kbuf->y_stride;
	obuf->imageProp.chromaPitch = kbuf->uv_stride;

	ISP_PR_DBG("wxh[%ux%u], len[%u], fmt[%s], y_s[%u], uv_s[%u]",
		obuf->imageProp.width, obuf->imageProp.height, len, fmt->name,
		obuf->imageProp.lumaPitch, obuf->imageProp.chromaPitch);

	obuf->buffer.bufTags = 0;
	obuf->buffer.vmidSpace.bit.vmid = 0;
	obuf->buffer.vmidSpace.bit.space = ADDR_SPACE_TYPE_GPU_VA;

	obuf->buffer.bufBaseALo = (u32)(addr & 0xffffffff);
	obuf->buffer.bufBaseAHi = (u32)(addr >> 32);

	switch (fmt->fw_fmt) {
	case IMAGE_FORMAT_NV12:
	case IMAGE_FORMAT_NV21:
	case IMAGE_FORMAT_YUV422SEMIPLANAR:
		obuf->buffer.bufSizeA =
			obuf->imageProp.lumaPitch * obuf->imageProp.height;

		addr += obuf->buffer.bufSizeA;
		obuf->buffer.bufBaseBLo = (u32)(addr & 0xffffffff);
		obuf->buffer.bufBaseBHi = (u32)(addr >> 32);
		obuf->buffer.bufSizeB =
			obuf->imageProp.chromaPitch * obuf->imageProp.height;
		break;
	case IMAGE_FORMAT_I420:
		obuf->buffer.bufSizeA =
			obuf->imageProp.lumaPitch * obuf->imageProp.height;

		addr += obuf->buffer.bufSizeA;
		obuf->buffer.bufBaseBLo = (u32)(addr & 0xffffffff);
		obuf->buffer.bufBaseBHi = (u32)(addr >> 32);
		obuf->buffer.bufSizeB = obuf->imageProp.chromaPitch *
			obuf->imageProp.height / 2;

		addr += obuf->buffer.bufSizeB;
		obuf->buffer.bufBaseCLo = (u32)(addr & 0xffffffff);
		obuf->buffer.bufBaseCHi = (u32)(addr >> 32);
		obuf->buffer.bufSizeC = obuf->buffer.bufSizeB;
		break;
	case IMAGE_FORMAT_YV12:
		obuf->buffer.bufSizeA =
			obuf->imageProp.lumaPitch * obuf->imageProp.height;

		addr += obuf->buffer.bufSizeA;
		obuf->buffer.bufBaseCLo = (u32)(addr & 0xffffffff);
		obuf->buffer.bufBaseCHi = (u32)(addr >> 32);
		obuf->buffer.bufSizeC = obuf->imageProp.chromaPitch *
			obuf->imageProp.height / 2;

		addr += obuf->buffer.bufSizeC;
		obuf->buffer.bufBaseBLo = (u32)(addr & 0xffffffff);
		obuf->buffer.bufBaseBHi = (u32)(addr >> 32);
		obuf->buffer.bufSizeB = obuf->buffer.bufSizeC;
		break;
	case IMAGE_FORMAT_YUV422INTERLEAVED:
		obuf->buffer.bufSizeA =
			obuf->imageProp.lumaPitch * 2 * obuf->imageProp.height;
		break;
	case IMAGE_FORMAT_YUV422PLANAR:
		obuf->buffer.bufSizeA =
			obuf->imageProp.lumaPitch * obuf->imageProp.height;

		addr += obuf->buffer.bufSizeA;
		obuf->buffer.bufBaseBLo = (u32)(addr & 0xffffffff);
		obuf->buffer.bufBaseBHi = (u32)(addr >> 32);
		obuf->buffer.bufSizeB =
			obuf->imageProp.chromaPitch * obuf->imageProp.height;

		addr += obuf->buffer.bufSizeB;
		obuf->buffer.bufBaseCLo = (u32)(addr & 0xffffffff);
		obuf->buffer.bufBaseCHi = (u32)(addr >> 32);
		obuf->buffer.bufSizeC = obuf->buffer.bufSizeB;
		break;
	case IMAGE_FORMAT_RGBBAYER8:
	case IMAGE_FORMAT_RGBBAYER10:
	case IMAGE_FORMAT_RGBBAYER12:
	case IMAGE_FORMAT_RGBBAYER16:
		obuf->buffer.bufSizeA = len;
		break;
	default:
		ISP_PR_ERR("fw_fmt[%d] not supported!", fmt->fw_fmt);
	}
}

int md_list_has_2_entry(struct physical_cam *cam)
{
	return (list_is_first(&cam->pong.entry, &cam->md_list) ||
		list_is_last(&cam->pong.entry, &cam->md_list));
}

static bool is_2x2_binning_prf_mode(enum mode_profile_name profile_id)
{
	return (profile_id == PROFILE_NOHDR_2X2BINNING_3MP_15 ||
		profile_id == PROFILE_NOHDR_2X2BINNING_3MP_30 ||
		profile_id == PROFILE_NOHDR_2X2BINNING_3MP_60);
}

static bool is_v2_binning_prf_mode(enum mode_profile_name profile_id)
{
	return (profile_id == PROFILE_NOHDR_V2BINNING_6MP_30 ||
		profile_id == PROFILE_NOHDR_V2BINNING_6MP_60);
}

/*
 * The ROI from HAL is always based on the full resolution, no matter what the
 * sensor profile is, full, 2x2 binning, etc. Do ROI mapping based on sensor
 * profile in driver layer.
 */
static int map_roi_ae(struct _Window_t *win, int src[],
		      enum mode_profile_name id)
{
	if (!win || !src) {
		ISP_PR_ERR("dst or src is null pointer!");
		return -EFAULT;
	}

	win->h_offset = (u32)src[0];
	win->v_offset = (u32)src[1];
	win->h_size = (u32)(src[2] - src[0]);
	win->v_size = (u32)(src[3] - src[1]);
	ISP_PR_DBG("got ROI from hal, x:%d y:%d width:%d height:%d",
		   win->h_offset, win->v_offset, win->h_size, win->v_size);

	if (is_2x2_binning_prf_mode(id)) {
		ISP_PR_DBG("2x2 binning mode");
		win->h_offset /= 2;
		win->v_offset /= 2;
		win->h_size /= 2;
		win->v_size /= 2;
	} else if (is_v2_binning_prf_mode(id)) {
		ISP_PR_DBG("v2 binning mode");
		win->v_offset /= 2;
		win->v_size /= 2;
	} else {
		ISP_PR_DBG("none binning mode");
	}

	ISP_PR_DBG("set ROI to FW, x:%d y:%d width:%d height:%d",
		   win->h_offset, win->v_offset, win->h_size, win->v_size);
	return OK;
}

static int unmap_roi_ae(struct _Window_t *win, int dst[],
			enum mode_profile_name id)
{
	if (!win || !dst) {
		ISP_PR_ERR("dst or src is null pointer!");
		return -EFAULT;
	}

	ISP_PR_DBG("FW ROI rst x:%d y:%d width:%d height:%d",
		   win->h_offset, win->v_offset, win->h_size, win->v_size);
	dst[0] = (s32)win->h_offset;
	dst[1] = (s32)win->v_offset;
	dst[2] = (s32)win->h_size + dst[0];
	dst[3] = (s32)win->v_size + dst[1];
	if (is_2x2_binning_prf_mode(id)) {
		ISP_PR_DBG("2x2 binning mode");
		dst[0] *= 2;
		dst[1] *= 2;
		dst[2] *= 2;
		dst[3] *= 2;
	} else if (is_v2_binning_prf_mode(id)) {
		ISP_PR_DBG("v2 binning mode");
		dst[1] *= 2;
		dst[3] *= 2;
	} else {
		ISP_PR_DBG("none binning mode");
	}

	ISP_PR_DBG("ROI to HAL Xmin:%d Ymin:%d Xmax:%d Ymax:%d",
		   dst[0], dst[1], dst[2], dst[3]);
	return OK;
}

static int map_roi_af(struct _CvipAfRoiWindow_t *win, int src[],
		      enum mode_profile_name id)
{
	if (!win || !src) {
		ISP_PR_ERR("dst or src is null pointer!");
		return -EFAULT;
	}

	win->xmin = (u32)src[0];
	win->ymin = (u32)src[1];
	win->xmax = (u32)src[2];
	win->ymax = (u32)src[3];
	win->weight = (u32)src[4];
	ISP_PR_DBG("got ROI from hal xmin:%d ymin:%d xmax:%d ymax:%d weight:%d",
		   win->xmin, win->ymin, win->xmax, win->ymax, win->weight);

	if (is_2x2_binning_prf_mode(id)) {
		ISP_PR_DBG("2x2 binning mode");
		win->xmin /= 2;
		win->ymin /= 2;
		win->xmax /= 2;
		win->ymax /= 2;
	} else if (is_v2_binning_prf_mode(id)) {
		ISP_PR_DBG("v2 binning mode");
		win->ymin /= 2;
		win->ymax /= 2;
	} else {
		ISP_PR_DBG("none binning mode");
	}

	ISP_PR_DBG("set ROI to FW, x:%d y:%d width:%d height:%d",
		   win->xmin, win->ymin, win->xmax, win->ymax);
	return OK;
}

static int unmap_roi_af(struct _CvipAfRoiWindow_t *win, int dst[],
			enum mode_profile_name id)
{
	if (!win || !dst) {
		ISP_PR_ERR("dst or src is null pointer!");
		return -EFAULT;
	}

	ISP_PR_DBG("FW ROI rst xmin:%d ymin:%d xmax:%d ymax:%d weight:%d",
		   win->xmin, win->ymin, win->xmax, win->ymax, win->weight);
	dst[0] = (s32)win->xmin;
	dst[1] = (s32)win->ymin;
	dst[2] = (s32)win->xmax;
	dst[3] = (s32)win->ymax;
	dst[4] = (s32)win->weight;
	if (is_2x2_binning_prf_mode(id)) {
		ISP_PR_DBG("2x2 binning mode");
		dst[0] *= 2;
		dst[1] *= 2;
		dst[2] *= 2;
		dst[3] *= 2;
	} else if (is_v2_binning_prf_mode(id)) {
		ISP_PR_DBG("v2 binning mode");
		dst[1] *= 2;
		dst[3] *= 2;
	} else {
		ISP_PR_DBG("none binning mode");
	}

	ISP_PR_DBG("ROI to HAL Xmin:%d Ymin:%d Xmax:%d Ymax:%d",
		   dst[0], dst[1], dst[2], dst[3]);
	return OK;
}

/* Use the color correction transform and gains to do color conversion only when
 * color correction mode is matrix.
 * And color correction matrix mode only works in manual WB mode; if AWB is
 * enabled with awbMode != OFF, color correction matrix mode is ignored.
 * So color correction transform and gains only works in manual WB mode and
 * matrix color correction mode.
 */
static bool need_to_set_cc_value(struct physical_cam *cam, bool *is_matrix_mode)
{
	bool mode_change = cam->cur_awb_mode != cam->pre_awb_mode ||
			   cam->cur_cc_mode != cam->pre_cc_mode;
	*is_matrix_mode = (cam->cur_awb_mode == AWB_MODE_MANUAL) &&
			  (cam->cur_cc_mode ==
			  AMD_CAM_COLOR_CORRECTION_MODE_TRANSFORM_MATRIX);
	return *is_matrix_mode && mode_change;
}

int map_metadata_2_framectrl(struct physical_cam *cam, struct frame_control *fc)
{
	int i, j, cnt;
	u8 a;
	u32 tag;
	u64 mask = 0;
	u64 addr = 0;
	int ret = OK;
	bool set = false;
	bool need_to_set_cc = false;
	bool need_to_check_cc = false;
	struct isp_ctl_ctx ctx;
	struct metadata_entry *m = NULL;
	enum sensor_test_pattern pattern;
	enum _TestPattern_t cvip_pattern;
	enum _FrameControlBitmask_t bit;
	struct _FrameControl_t *f = &fc->fw_fc.frameCtrl;
	struct amd_cam_metadata_entry e;
	struct amd_cam_metadata_entry en;
	struct amd_cam_metadata_entry old_e;
	struct amd_cam_metadata *old = NULL;
	struct amd_cam_metadata *cur = NULL;
	struct _Roi_t *ae_roi = NULL;
	struct _CvipAfRoi_t *roi = NULL;
	struct _Window_t *win = NULL;

	ISP_PR_DBG("Entry! cam[%d]", cam->idx);
	if (list_empty(&cam->md_list)) {
		ISP_PR_ERR("cam[%d] no metadata from HAL!", cam->idx);
		return -EINVAL;
	}

	m = list_entry(cam->md_list.next, struct metadata_entry, entry);
	cur = m->md;

	if (md_list_has_2_entry(cam)) {
		m = list_entry(cam->md_list.prev, struct metadata_entry, entry);
		old = m->md;
	}

	//android.control.captureIntent 1 u8
	if (entry_cmp(&mask, AMD_CAM_CONTROL_CAPTURE_INTENT,
		      FC_BIT_CAPTIRE_INTENT, cur, old, &e, &old_e)) {
		a = e.data.u8[0];
		ISP_PR_DBG("android intent %d", a);
		if (a == AMD_CAM_CONTROL_CAPTURE_INTENT_STILL_CAPTURE)
			f->captureIntent.mode = SCENARIO_MODE_STILL;
		else if (a == AMD_CAM_CONTROL_CAPTURE_INTENT_VIDEO_RECORD)
			f->captureIntent.mode = SCENARIO_MODE_VIDEO;
		else
			f->captureIntent.mode = SCENARIO_MODE_PREVIEW;
		ISP_PR_DBG("set intent mode to %d", f->captureIntent.mode);
	}

	//android.control.awbMode 1 u8
	if (entry_cmp(&mask, AMD_CAM_CONTROL_AWB_MODE,
		      FC_BIT_AWB_MODE, cur, old, &e, &old_e)) {
		mask |= FC_BIT_AWB_LIGHT_SOURCE;
		f->awb.mode = AWB_MODE_LIGHT_SOURCE;
		a = e.data.u8[0];
		if (a == AMD_CAM_CONTROL_AWB_MODE_AUTO) {
			f->awb.mode = AWB_MODE_AUTO;
			f->awb.lightSource = AWB_LIGHT_SOURCE_AUTO;
		} else if (a == AMD_CAM_CONTROL_AWB_MODE_INCANDESCENT) {
			f->awb.lightSource = AWB_LIGHT_SOURCE_INCANDESCENT;
		} else if (a == AMD_CAM_CONTROL_AWB_MODE_FLUORESCENT) {
			f->awb.lightSource = AWB_LIGHT_SOURCE_FLUORESCENT;
		} else if (a == AMD_CAM_CONTROL_AWB_MODE_WARM_FLUORESCENT) {
			f->awb.lightSource = AWB_LIGHT_SOURCE_WARM_FLUORESCENT;
		} else if (a == AMD_CAM_CONTROL_AWB_MODE_DAYLIGHT) {
			f->awb.lightSource = AWB_LIGHT_SOURCE_DAYLIGHT;
		} else if (a == AMD_CAM_CONTROL_AWB_MODE_CLOUDY_DAYLIGHT) {
			f->awb.lightSource = AWB_LIGHT_SOURCE_CLOUDY;
		} else if (a == AMD_CAM_CONTROL_AWB_MODE_TWILIGHT) {
			f->awb.lightSource = AWB_LIGHT_SOURCE_TWILIGHT;
		} else if (a == AMD_CAM_CONTROL_AWB_MODE_SHADE) {
			f->awb.lightSource = AWB_LIGHT_SOURCE_SHADE;
		} else {//AMD_CAM_CONTROL_AWB_MODE_OFF
			f->awb.mode = AWB_MODE_MANUAL;
			f->awb.lightSource = AWB_LIGHT_SOURCE_UNKNOWN;
		}

		ISP_PR_DBG("set AWB mode to %d, lightSource to %d",
			   f->awb.mode, f->awb.lightSource);
		cam->cur_awb_mode = f->awb.mode;
	}

	//android.control.awbLock 1 u8
	if (entry_cmp(&mask, AMD_CAM_CONTROL_AWB_LOCK,
		      FC_BIT_AWB_LOCK, cur, old, &e, &old_e)) {
		if (e.data.u8[0] == AMD_CAM_CONTROL_AWB_LOCK_ON)
			f->awb.lockType = LOCK_TYPE_IMMEDIATELY;
		else
			f->awb.lockType = LOCK_TYPE_UNLOCK;

		ISP_PR_DBG("set AWB lock to %d", f->awb.lockType);
	}

	//android.colorCorrection.mode 1 u8
	tag = AMD_CAM_COLOR_CORRECTION_MODE;
	if (amd_cam_find_metadata_entry(cur, tag, &e) == OK)
		cam->cur_cc_mode = e.data.u8[0];

	need_to_set_cc = need_to_set_cc_value(cam, &need_to_check_cc);

	//android.colorCorrection.gains 4 f
	tag = AMD_CAM_COLOR_CORRECTION_GAINS;
	if (need_to_set_cc) {
		if (amd_cam_find_metadata_entry(cur, tag, &e) == OK) {
			set = true;
			mask |= FC_BIT_AWB_GAIN;
		} else {
			ISP_PR_WARN("cannot find WBGain for manual mode");
		}
	}

	if (set || (need_to_check_cc && entry_cmp(&mask, tag, FC_BIT_AWB_GAIN,
						  cur, old, &e, &old_e))) {
		memcpy(&f->awb.wbGain, &e.data.f[0], sizeof(f->awb.wbGain));
#ifdef PRINT_FLOAT_INFO
		kernel_fpu_begin();
		ISP_PR_INFO("set color correction gainsx1000 R%d GR%d GB%d B%d",
			    (s32)(f->awb.wbGain.red * 1000),
			    (s32)(f->awb.wbGain.greenR * 1000),
			    (s32)(f->awb.wbGain.greenB * 1000),
			    (s32)(f->awb.wbGain.blue * 1000));
		kernel_fpu_end();
#endif
	}

	//android.colorCorrection.transform 9 f
	tag = AMD_CAM_COLOR_CORRECTION_TRANSFORM;
	if (need_to_set_cc) {
		if (amd_cam_find_metadata_entry(cur, tag, &e) == OK) {
			set = true;
			mask |= FC_BIT_AWB_CCM;
		} else {
			ISP_PR_WARN("cannot find CCM for manual mode");
			set = false;
		}
	}

	if (set || (need_to_check_cc && entry_cmp(&mask, tag, FC_BIT_AWB_CCM,
						  cur, old, &e, &old_e))) {
		memcpy(&f->awb.CCM, &e.data.f[0], sizeof(f->awb.CCM));
#ifdef PRINT_FLOAT_INFO
		kernel_fpu_begin();
		for (i = 0; i < e.count; i++)
			ISP_PR_INFO("set AWB transform[%d] to (x1000)%d",
				    i, (s32)(f->awb.CCM.coeff[i] * 1000));
		kernel_fpu_end();
#endif
	}

	cam->pre_awb_mode = cam->cur_awb_mode;
	cam->pre_cc_mode = cam->cur_cc_mode;

	//android.control.aeMode 1 u8
	//below modes cannot support now because no flash
	//AMD_CAM_CONTROL_AE_MODE_ON_AUTO_FLASH,
	//AMD_CAM_CONTROL_AE_MODE_ON_ALWAYS_FLASH,
	//AMD_CAM_CONTROL_AE_MODE_ON_AUTO_FLASH_REDEYE,
	//AMD_CAM_CONTROL_AE_MODE_ON_EXTERNAL_FLASH,
	if (entry_cmp(&mask, AMD_CAM_CONTROL_AE_MODE,
		      FC_BIT_AE_MODE, cur, old, &e, &old_e)) {
		if (e.data.u8[0] == AMD_CAM_CONTROL_AE_MODE_OFF)
			f->ae.mode = AE_MODE_MANUAL;
		else if (e.data.u8[0] == AMD_CAM_CONTROL_AE_MODE_ON)
			f->ae.mode = AE_MODE_AUTO;
		else
			f->ae.mode = AE_MODE_INVALID;

		ISP_PR_DBG("set AE mode to %d", f->ae.mode);
	}

	//android.control.aeAntibandingMode 1 u8
	//AMD_CAM_CONTROL_AE_ANTIBANDING_MODE_AUTO not support
	if (entry_cmp(&mask, AMD_CAM_CONTROL_AE_ANTIBANDING_MODE,
		      FC_BIT_AE_FLIKER, cur, old, &e, &old_e)) {
		a = e.data.u8[0];
		if (a == AMD_CAM_CONTROL_AE_ANTIBANDING_MODE_OFF)
			f->ae.flickerType = AE_FLICKER_TYPE_OFF;
		else if (a == AMD_CAM_CONTROL_AE_ANTIBANDING_MODE_50HZ)
			f->ae.flickerType = AE_FLICKER_TYPE_50HZ;
		else if (a ==  AMD_CAM_CONTROL_AE_ANTIBANDING_MODE_60HZ)
			f->ae.flickerType = AE_FLICKER_TYPE_60HZ;
		else
			f->ae.flickerType = AE_FLICKER_TYPE_INVALID;

		ISP_PR_DBG("set banding mode to %d", f->ae.flickerType);
	}

	//android.control.aeRegions 5n i32 (xmin, ymin, xmax, ymax, w[0, 1000])
	if (entry_cmp(&mask, AMD_CAM_CONTROL_AE_REGIONS,
		      FC_BIT_AE_REGIONS, cur, old, &e, &old_e)) {
		cnt = e.count / 5;
		f->ae.region.regionCnt = (u8)cnt;
		for (i = 0, j = 0; j < cnt; i++, j++) {
			ae_roi = &f->ae.region.region[i];
			ae_roi->type = ROI_TYPE_NORMAL;
			ae_roi->weight = (u32)e.data.i32[5 * j + 4];

			/* ISP FW cannot support ROI with weight zero, so just
			 * ignore it.
			 */
			if (ae_roi->weight == 0) {
				ISP_PR_INFO("ignore ROI[%d] with weight 0", j);
				f->ae.region.regionCnt--;
				i--;
				continue;
			}

			ISP_PR_DBG("set AE ROI[%d] weight to %d", i,
				   ae_roi->weight);
			win = &ae_roi->window;
			if (map_roi_ae(win, &e.data.i32[5 * j], fc->prf_id)
			    != OK) {
				ISP_PR_ERR("map AE ROI[%d] failed", j);
				mask &= ~FC_BIT_AE_REGIONS;
				break;
			}
		}
	}

	//android.control.aePrecaptureTrigger 1 u8
	if (entry_cmp(&mask, AMD_CAM_CONTROL_AE_PRECAPTURE_TRIGGER,
		      FC_BIT_AE_PRE_TRIGGER, cur, old, &e, &old_e)) {
		a = e.data.u8[0];
		if (a == AMD_CAM_CONTROL_AE_PRECAPTURE_TRIGGER_START) {
			f->ae.precaptureTrigger = AE_PRE_CAPTURE_TRIGGER_START;
			cam->ae_trigger = a;
		} else if (a == AMD_CAM_CONTROL_AE_PRECAPTURE_TRIGGER_CANCEL) {
			f->ae.precaptureTrigger = AE_PRE_CAPTURE_TRIGGER_CANCEL;
			cam->ae_trigger = a;
		} else if (cam->ae_trigger !=
			   AMD_CAM_CONTROL_AE_PRECAPTURE_TRIGGER_START) {
			f->ae.precaptureTrigger = AE_PRE_CAPTURE_TRIGGER_IDLE;
			cam->ae_trigger = a;
		} else {
			mask &= ~FC_BIT_AE_PRE_TRIGGER;
		}

		ISP_PR_DBG("set AE pre trigger to %d", f->ae.precaptureTrigger);
	}

	//android.control.aeLock 1 u8
	if (entry_cmp(&mask, AMD_CAM_CONTROL_AE_LOCK,
		      FC_BIT_AE_LOCK, cur, old, &e, &old_e)) {
		if (e.data.u8[0] == AMD_CAM_CONTROL_AE_LOCK_ON) {
			if (cam->ae_trigger ==
			    AMD_CAM_CONTROL_AE_PRECAPTURE_TRIGGER_START)
				f->ae.lockType = LOCK_TYPE_CONVERGENCE;
			else
				f->ae.lockType = LOCK_TYPE_IMMEDIATELY;
		} else {
			f->ae.lockType = LOCK_TYPE_UNLOCK;
		}

		ISP_PR_DBG("set AE lock to %d", f->ae.lockType);
	}

	//android.control.aeExposureCompensation 1 i32
	if (entry_cmp(&mask, AMD_CAM_CONTROL_AE_EXPOSURE_COMPENSATION,
		      FC_BIT_AE_EV_COMPENSATION, cur, old, &e, &old_e)) {
		f->ae.evCompensation.numerator = e.data.i32[0] *
			STEP_NUMERATOR * 100 / STEP_DENOMINATOR;
		f->ae.evCompensation.denominator = 100;
		ISP_PR_DBG("set EV to %d/%d", f->ae.evCompensation.numerator,
			   f->ae.evCompensation.denominator);
	}

	//android.control.aeTargetFpsRange 2 i32
	if (entry_cmp(&mask, AMD_CAM_CONTROL_AE_TARGET_FPS_RANGE,
		      FC_BIT_AE_FPS_RANGE, cur, old, &e, &old_e)) {
		f->ae.fpsRange.min = (u32)e.data.i32[0] *
						FPS_RANGE_SCALE_FACTOR;
		f->ae.fpsRange.max = (u32)e.data.i32[1] *
						FPS_RANGE_SCALE_FACTOR;
		ISP_PR_DBG("FPS range to [%d, %d]", f->ae.fpsRange.min,
			   f->ae.fpsRange.max);
	}

	//vendor tag exposure time upper limit 1 i64 ns, FW is us (1us = 1000ns)
	if (entry_cmp(&mask,
		      AMD_CAM_VENDOR_CONTROL_APP_EXPOSURE_TIME_UPPER_LIMIT,
		      FC_BIT_AE_ITIME_RANGE, cur, old, &e, &old_e)) {
		f->ae.iTimeRange.min = (u32)(g_min_exp_time / 1000);
		f->ae.iTimeRange.max = (u32)(e.data.i32[0] / 1000);
		ISP_PR_DBG("set iTime range to [%d,%d]", f->ae.iTimeRange.min,
			   f->ae.iTimeRange.max);
	}

	//android.control.postRawSensitivityBoost 1 i32
	if (entry_cmp(&mask, AMD_CAM_CONTROL_POST_RAW_SENSITIVITY_BOOST,
		      FC_BIT_AE_ISP_GAIN, cur, old, &e, &old_e)) {
		f->ae.ispGain = (u32)e.data.i32[0];
		ISP_PR_DBG("set isp gain to %d", f->ae.ispGain);
	}

	//android.sensor.exposureTime 1 i64 ns, FW is us (1us = 1000ns)
	if (entry_cmp(&mask, AMD_CAM_SENSOR_EXPOSURE_TIME,
		      FC_BIT_AE_ITIME, cur, old, &e, &old_e)) {
		f->ae.iTime = (u32)((e.data.i64[0] / 1000) & 0xffffffff);
		ISP_PR_DBG("set exposure time to %dus", f->ae.iTime);
	}

	//android.sensor.sensitivity 1 i32
	if (entry_cmp(&mask, AMD_CAM_SENSOR_SENSITIVITY,
		      FC_BIT_AE_AGAIN, cur, old, &e, &old_e)) {
		f->ae.aGain = (u32)e.data.i32[0];
		ISP_PR_DBG("set sensitivity to %d", f->ae.aGain);
	}

	//android.sensor.frameDuration 1 i64 ns, FW is us
	if (entry_cmp(&mask, AMD_CAM_SENSOR_FRAME_DURATION,
		      FC_BIT_AE_FRAME_DURATION, cur, old, &e, &old_e)) {
		f->ae.frameDuration = (u32)
			((e.data.i64[0] / 1000) & 0xffffffff);
		ISP_PR_DBG("set duration to %dus", f->ae.frameDuration);
	}

	//com.amd.control.ae_roi_touch_target 1 i32 & FW is uint32
	if (entry_cmp(&mask, AMD_CAM_VENDOR_CONTROL_AE_TOUCH_TARGET,
		      FC_BIT_AE_TOUCH_TARGET, cur, old, &e, &old_e)) {
		f->ae.touchTarget = (u32)e.data.i32[0];
		ISP_PR_DBG("set touch target to %d", e.data.i32[0]);
	}

	//com.amd.control.ae_roi_touch_target_weight 1 i32
	if (entry_cmp(&mask, AMD_CAM_VENDOR_CONTROL_AE_TOUCH_TARGET_WEIGHT,
		      FC_BIT_AE_TOUCH_TARGET_WEIGHT, cur, old, &e, &old_e)) {
		f->ae.touchTargetWeight = e.data.i32[0];
		ISP_PR_DBG("set touch target weight to %d", e.data.i32[0]);
	}

	//AF
	f->af.lensPos.unit = POS_UNIT_DIOPTER;
	//android.control.afMode 1 u8
	if (g_prop->cvip_enable == CVIP_STATUS_ENABLE &&
	    entry_cmp(&mask, AMD_CAM_CONTROL_AF_MODE,
		      FC_BIT_CVIP_AF_MODE, cur, old, &e, &old_e)) {
		f->cvipSensor.afData.mode = e.data.u8[0];
		ISP_PR_DBG("set cvip AF mode to %d", f->cvipSensor.afData.mode);
	} else {
		if (entry_cmp(&mask, AMD_CAM_CONTROL_AF_MODE, FC_BIT_AF_MODE,
			      cur, old, &e, &old_e)) {
			a = e.data.u8[0];
			if (a == AMD_CAM_CONTROL_AF_MODE_AUTO)
				f->af.mode = AF_MODE_AUTO;
			else if (a == AMD_CAM_CONTROL_AF_MODE_CONTINUOUS_VIDEO)
				f->af.mode = AF_MODE_CONTINUOUS_VIDEO;
			else if (a ==
				 AMD_CAM_CONTROL_AF_MODE_CONTINUOUS_PICTURE)
				f->af.mode = AF_MODE_CONTINUOUS_PICTURE;
			else
				f->af.mode = AF_MODE_MANUAL;

			ISP_PR_DBG("set AF mode to %d (1-manual 6-auto)",
				   f->af.mode);
		}
	}

	//android.control.afTrigger 1 u8 (only for CVIP)
	if (g_prop->cvip_enable == CVIP_STATUS_ENABLE &&
	    entry_cmp(&mask, AMD_CAM_CONTROL_AF_TRIGGER,
		      FC_BIT_CVIP_AF_TRIGGER, cur, old, &e, &old_e)) {
		f->cvipSensor.afData.trigger = e.data.u8[0];
		ISP_PR_DBG("set cvip AF trigger to %d",
			   f->cvipSensor.afData.trigger);
	}

	//android.control.afRegions (only for CVIP)
	if (g_prop->cvip_enable == CVIP_STATUS_ENABLE &&
	    entry_cmp(&mask, AMD_CAM_CONTROL_AF_REGIONS,
		      FC_BIT_CVIP_AF_ROI, cur, old, &e, &old_e)) {
		cnt = e.count / 5;
		roi = &f->cvipSensor.afData.cvipAfRoi;
		roi->numRoi = (u8)cnt;
		for (i = 0; i < cnt; i++) {
			if (map_roi_af(&roi->roiAf[i], &e.data.i32[5 * i],
				       fc->prf_id) != OK) {
				ISP_PR_ERR("map AF ROI[%d] failed", i);
				mask &= ~FC_BIT_CVIP_AF_ROI;
				break;
			}
		}
	}

	//android.lens.focusDistance 1 f
	if (g_prop->cvip_enable == CVIP_STATUS_ENABLE &&
	    entry_cmp(&mask, AMD_CAM_LENS_FOCUS_DISTANCE,
		      FC_BIT_CVIP_AF_DISTANCE, cur, old, &e, &old_e)) {
		memcpy(&f->cvipSensor.afData.distance, &e.data.f[0],
		       sizeof(float));
#ifdef PRINT_FLOAT_INFO
		kernel_fpu_begin();
		ISP_PR_INFO("set cvip AF distance to (x1000) %d",
			    (s32)(f->cvipSensor.afData.distance * 1000));
		kernel_fpu_end();
#endif
	} else {
		if (entry_cmp(&mask, AMD_CAM_LENS_FOCUS_DISTANCE,
			      FC_BIT_LENS_POS, cur, old, &e, &old_e)) {
			memcpy(&f->af.lensPos.position, &e.data.f[0],
			       sizeof(float));
#ifdef PRINT_FLOAT_INFO
			kernel_fpu_begin();
			ISP_PR_INFO("set len focus distance to (x1000) %d",
				    (s32)(f->af.lensPos.position * 1000));
			kernel_fpu_end();
#endif
		}
	}

	//vendor tag lens distance range 2 f, unit is diopter
	if (g_prop->cvip_enable == CVIP_STATUS_ENABLE &&
	    entry_cmp(&mask, AMD_CAM_VENDOR_CONTROL_LENS_SWEEP_DISTANCERANGE,
		      FC_BIT_CVIP_AF_LENS_RANGE, cur, old, &e, &old_e)) {
		memcpy(&f->cvipSensor.afData.distanceNear, &e.data.f[0],
		       sizeof(float) * e.count);
#ifdef PRINT_FLOAT_INFO
		kernel_fpu_begin();
		ISP_PR_INFO("set sweep range(x1000) near:%d far:%d diopter",
			    (s32)(e.data.f[0] * 1000),
			    (s32)(e.data.f[1] * 1000));
		kernel_fpu_end();
#endif
	}

	//android.control.sceneMode 1 u8
	if (entry_cmp(&mask, AMD_CAM_CONTROL_SCENE_MODE,
		      FC_BIT_SCENE_MODE, cur, old, &e, &old_e)) {
		a = e.data.u8[0];
		if (a == AMD_CAM_CONTROL_SCENE_MODE_HIGH_SPEED_VIDEO)
			f->sceneMode.mode = SCENE_MODE_HIGH_SPEED_VIDEO;
		else if (a == AMD_CAM_CONTROL_SCENE_MODE_ACTION)
			f->sceneMode.mode = SCENE_MODE_ACTION;
		else if (a == AMD_CAM_CONTROL_SCENE_MODE_PORTRAIT)
			f->sceneMode.mode = SCENE_MODE_PORTRAIT;
		else if (a == AMD_CAM_CONTROL_SCENE_MODE_LANDSCAPE)
			f->sceneMode.mode = SCENE_MODE_LANDSCAPE;
		else if (a == AMD_CAM_CONTROL_SCENE_MODE_NIGHT)
			f->sceneMode.mode = SCENE_MODE_NIGHT;
		else if (a == AMD_CAM_CONTROL_SCENE_MODE_NIGHT_PORTRAIT)
			f->sceneMode.mode = SCENE_MODE_NIGHT_PORTRAIT;
		else if (a == AMD_CAM_CONTROL_SCENE_MODE_THEATRE)
			f->sceneMode.mode = SCENE_MODE_THEATRE;
		else if (a == AMD_CAM_CONTROL_SCENE_MODE_BEACH)
			f->sceneMode.mode = SCENE_MODE_BEACH;
		else if (a == AMD_CAM_CONTROL_SCENE_MODE_SNOW)
			f->sceneMode.mode = SCENE_MODE_SNOW;
		else if (a == AMD_CAM_CONTROL_SCENE_MODE_SUNSET)
			f->sceneMode.mode = SCENE_MODE_SUNSET;
		else if (a == AMD_CAM_CONTROL_SCENE_MODE_STEADYPHOTO)
			f->sceneMode.mode = SCENE_MODE_STEADYPHOTO;
		else if (a == AMD_CAM_CONTROL_SCENE_MODE_FIREWORKS)
			f->sceneMode.mode = SCENE_MODE_FIREWORKS;
		else if (a == AMD_CAM_CONTROL_SCENE_MODE_SPORTS)
			f->sceneMode.mode = SCENE_MODE_SPORTS;
		else if (a == AMD_CAM_CONTROL_SCENE_MODE_PARTY)
			f->sceneMode.mode = SCENE_MODE_PARTY;
		else if (a == AMD_CAM_CONTROL_SCENE_MODE_CANDLELIGHT)
			f->sceneMode.mode = SCENE_MODE_CANDLELIGHT;
		else if (a == AMD_CAM_CONTROL_SCENE_MODE_BARCODE)
			f->sceneMode.mode = SCENE_MODE_BARCODE;
		else if (a == AMD_CAM_CONTROL_SCENE_MODE_HDR)
			f->sceneMode.mode = SCENE_MODE_HDR;
		else if (a == AMD_CAM_CONTROL_SCENE_MODE_CUSTOM_MEDICAL)
			f->sceneMode.mode = SCENE_MODE_MEDICAL;
		else if (a == AMD_CAM_CONTROL_SCENE_MODE_DISABLED)
			f->sceneMode.mode = SCENE_MODE_DEFAULT;
		else
			f->sceneMode.mode = SCENE_MODE_INVALID;

		ISP_PR_DBG("set scene mode to %d", f->sceneMode.mode);
	}

	//android.blackLevel.lock 1 u8
	if (entry_cmp(&mask, AMD_CAM_BLACK_LEVEL_LOCK,
		      FC_BIT_BLC, cur, old, &e, &old_e)) {
		if (e.data.u8[0] == AMD_CAM_BLACK_LEVEL_LOCK_ON)
			f->blc.lockState = BLC_LOCK_STATE_LOCKED;
		else
			f->blc.lockState = BLC_LOCK_STATE_UNLOCKED;

		ISP_PR_DBG("set black level lock %d", f->blc.lockState);
	}

	//android.hotPixel.mode 1 u8
	if (entry_cmp(&mask, AMD_CAM_HOT_PIXEL_MODE,
		      FC_BIT_DPC, cur, old, &e, &old_e)) {
		if (e.data.u8[0] == AMD_CAM_HOT_PIXEL_MODE_OFF)
			f->dpc.enabled = false;
		else
			f->dpc.enabled = true;

		ISP_PR_DBG("set hot pixel mode to %d", f->dpc.enabled);
	}

	//android.shading.mode 1 u8
	if (entry_cmp(&mask, AMD_CAM_SHADING_MODE,
		      FC_BIT_LSC, cur, old, &e, &old_e)) {
		if (e.data.u8[0] == AMD_CAM_SHADING_MODE_OFF)
			f->lsc.enabled = false;
		else
			f->lsc.enabled = true;

		ISP_PR_DBG("set shading mode to %d", f->lsc.enabled);
	}

	//android.colorCorrection.aberrationMode 1 u8 (cac)
	if (entry_cmp(&mask, AMD_CAM_COLOR_CORRECTION_ABERRATION_MODE,
		      FC_BIT_CAC, cur, old, &e, &old_e)) {
		if (e.data.u8[0] ==
		    AMD_CAM_COLOR_CORRECTION_ABERRATION_MODE_OFF)
			f->cac.enabled = false;
		else
			f->cac.enabled = true;

		ISP_PR_DBG("set color correction enable to %d", f->cac.enabled);
	}

	//android.edge.mode 1 u8
	if (entry_cmp(&mask, AMD_CAM_EDGE_MODE,
		      FC_BIT_SHARPEN, cur, old, &e, &old_e)) {
		if (e.data.u8[0] == AMD_CAM_EDGE_MODE_OFF)
			f->sharpen.enabled = false;
		else
			f->sharpen.enabled = true;

		ISP_PR_DBG("set edge enable to %d", f->sharpen.enabled);
	}

	//android.noiseReduction.mode 1 u8 range[1-10]
	if (entry_cmp(&mask, AMD_CAM_NOISE_REDUCTION_MODE,
		      FC_BIT_NR, cur, old, &e, &old_e)) {
		if (e.data.u8[0] == AMD_CAM_NOISE_REDUCTION_MODE_OFF) {
			f->nr.enabled = false;
		} else {
			f->nr.enabled = true;

			//android.noiseReduction.strength 1 u8
			tag = AMD_CAM_NOISE_REDUCTION_STRENGTH;
			if (amd_cam_find_metadata_entry(cur, tag, &e) != OK) {
				ISP_PR_ERR("enable NR mode but ");
				ISP_PR_ERR("no NOISE_REDUCTION_STRENGTH entry");
				mask &= ~FC_BIT_NR;
				f->nr.enabled = false;
			} else {
				f->nr.strength = e.data.u8[0];
				ISP_PR_DBG("set nr enable to %d strength to %d",
					   f->nr.enabled, f->nr.strength);
			}
		}
	} else {
		tag = AMD_CAM_NOISE_REDUCTION_MODE;
		if (amd_cam_find_metadata_entry(cur, tag, &e) == OK &&
		    e.data.u8[0] != AMD_CAM_NOISE_REDUCTION_MODE_OFF &&
		    entry_cmp(&mask, AMD_CAM_NOISE_REDUCTION_STRENGTH,
			      FC_BIT_NR, cur, old, &en, &old_e)) {
			//android.noiseReduction.strength 1 u8
			f->nr.enabled = true;
			f->nr.strength = en.data.u8[0];
			ISP_PR_DBG("set nr enable to %d, strength to %d",
				   f->nr.enabled, f->nr.strength);
		}
	}

	//android.tonemap.mode 0 u8
	if (entry_cmp(&mask, AMD_CAM_TONEMAP_MODE,
		      FC_BIT_GAMMA, cur, old, &e, &old_e)) {
		if (e.data.u8[0] == AMD_CAM_TONEMAP_MODE_CONTRAST_CURVE) {
			f->tonemap.mode = TONEMAP_TYPE_CONTRAST_CURVE;

			//android.tonemap.curveGreen 129 i32
			tag = AMD_CAM_TONEMAP_CURVE_GREEN;
			if (amd_cam_find_metadata_entry(cur, tag, &e) != OK) {
				ISP_PR_ERR("no curve green entry!");
			} else {
				for (i = 0; i < e.count; i++) {
					f->tonemap.curveGreen[i] =
						(u16)e.data.i32[i];
					ISP_PR_DBG("set curve green[%d] to %d",
						   i, f->tonemap.curveGreen[i]);
				}
			}
		} else if (e.data.u8[0] == AMD_CAM_TONEMAP_MODE_GAMMA_VALUE) {
			f->tonemap.mode = TONEMAP_TYPE_GAMMA;

			//android.tonemap.gamma 1 f
			tag = AMD_CAM_TONEMAP_GAMMA;
			if (amd_cam_find_metadata_entry(cur, tag, &e) != OK) {
				ISP_PR_ERR("no tonemap gamma entry!");
			} else {
				memcpy(&f->tonemap.gamma, &e.data.f[0],
				       sizeof(float));
#ifdef PRINT_FLOAT_INFO
				kernel_fpu_begin();
				ISP_PR_INFO("set gamma to (x1000) %d",
					    (s32)(f->tonemap.gamma * 1000));
				kernel_fpu_end();
#endif
			}
		} else if (e.data.u8[0] == AMD_CAM_TONEMAP_MODE_PRESET_CURVE) {
			f->tonemap.mode = TONEMAP_TYPE_PRESET_CURVE;

			//android.tonemap.presetCurve 1 u8
			tag = AMD_CAM_TONEMAP_PRESET_CURVE;
			if (amd_cam_find_metadata_entry(cur, tag, &e) != OK) {
				ISP_PR_ERR("no preset curve entry!");
			} else {
				if (e.data.u8[0] ==
				    AMD_CAM_TONEMAP_PRESET_CURVE_REC709) {
					f->tonemap.presetCurve =
					TONEMAP_PRESET_CURVE_TYPE_REC709;
				} else {
					f->tonemap.presetCurve =
						TONEMAP_PRESET_CURVE_TYPE_SRGB;
				}

				ISP_PR_DBG("set tonemap preset curve to %d",
					   f->tonemap.presetCurve);
			}
		} else {
			f->tonemap.mode = TONEMAP_TYPE_FAST;
		}

		ISP_PR_DBG("set tonemap mode to %d", f->tonemap.mode);
	} else {
		tag = AMD_CAM_TONEMAP_MODE;
		if (amd_cam_find_metadata_entry(cur, tag, &en) == OK) {
			a = en.data.u8[0];
			bit = FC_BIT_GAMMA;
			if (a == AMD_CAM_TONEMAP_MODE_PRESET_CURVE &&
			    entry_cmp(&mask, AMD_CAM_TONEMAP_PRESET_CURVE,
				      bit, cur, old, &e, &old_e)) {
				//android.tonemap.presetCurve 1 u8
				f->tonemap.mode = TONEMAP_TYPE_PRESET_CURVE;
				if (e.data.u8[0] ==
				    AMD_CAM_TONEMAP_PRESET_CURVE_REC709)
					f->tonemap.presetCurve =
					TONEMAP_PRESET_CURVE_TYPE_REC709;
				else
					f->tonemap.presetCurve =
						TONEMAP_PRESET_CURVE_TYPE_SRGB;

				ISP_PR_DBG("set tonemap preset curve to %d",
					   f->tonemap.presetCurve);
			} else if (a == AMD_CAM_TONEMAP_MODE_GAMMA_VALUE &&
				   entry_cmp(&mask, AMD_CAM_TONEMAP_GAMMA,
					     bit, cur, old, &e, &old_e)) {
				//android.tonemap.gamma 1 f
				f->tonemap.mode = TONEMAP_TYPE_GAMMA;
				memcpy(&f->tonemap.gamma, &e.data.f[0],
				       sizeof(float));
#ifdef PRINT_FLOAT_INFO
				kernel_fpu_begin();
				ISP_PR_INFO("set gamma to (x1000) %d",
					    (s32)(f->tonemap.gamma * 1000));
				kernel_fpu_end();
#endif
			} else if (a == AMD_CAM_TONEMAP_MODE_CONTRAST_CURVE &&
				   entry_cmp(&mask, AMD_CAM_TONEMAP_CURVE_GREEN,
					     bit, cur, old, &e, &old_e)) {
				//android.tonemap.curveGreen 129 i32
				f->tonemap.mode = TONEMAP_TYPE_CONTRAST_CURVE;
				for (i = 0; i < e.count; i++) {
					f->tonemap.curveGreen[i] =
						(u16)e.data.i32[i];
					ISP_PR_DBG("set curve green[%d] to %d",
						   i, f->tonemap.curveGreen[i]);
				}
			}
		}
	}

	//vendor tag color effect mode 1 u8
	if (entry_cmp(&mask, AMD_CAM_VENDOR_CONTROL_EFFECT_MODE,
		      FC_BIT_IE, cur, old, &e, &old_e)) {
		f->ie.enabled = true;
		a = e.data.u8[0];
		if (a == AMD_CAM_VENDOR_CONTROL_EFFECT_MODE_COLOR_SELECTION)
			f->ie.mode = IE_MODE_COLOR_SELECTION;
		else if (a == AMD_CAM_VENDOR_CONTROL_EFFECT_MODE_GRAYSCALE)
			f->ie.mode = IE_MODE_GRAYSCALE;
		else if (a == AMD_CAM_VENDOR_CONTROL_EFFECT_MODE_NEGATIVE)
			f->ie.mode = IE_MODE_NEGATIVE;
		else if (a == AMD_CAM_VENDOR_CONTROL_EFFECT_MODE_SEPIA)
			f->ie.mode = IE_MODE_SEPIA;
		else if (a == AMD_CAM_VENDOR_CONTROL_EFFECT_MODE_SHARPEN)
			f->ie.mode = IE_MODE_SHARPEN;
		else if (a == AMD_CAM_VENDOR_CONTROL_EFFECT_MODE_EMBOSS)
			f->ie.mode = IE_MODE_EMBOSS;
		else if (a == AMD_CAM_VENDOR_CONTROL_EFFECT_MODE_SKETCH)
			f->ie.mode = IE_MODE_SKETCH;
		else //AMD_CAM_VENDOR_CONTROL_EFFECT_MODE_OFF
			f->ie.enabled = false;

		ISP_PR_DBG("set effect enable to %d, mode to %d",
			   f->ie.enabled, f->ie.mode);
	}

	//android.statistics.histogramMode 1 u8
	tag = AMD_CAM_STATISTICS_HISTOGRAM_MODE;
	if (amd_cam_find_metadata_entry(cur, tag, &e) == OK) {
		if (e.data.u8[0] == AMD_CAM_STATISTICS_HISTOGRAM_MODE_ON)
			f->statistic.histogramEnabled = true;
		else
			f->statistic.histogramEnabled = false;

		ISP_PR_DBG("set histogram mode to %d",
			   f->statistic.histogramEnabled);
	}

	//android.statistics.sharpnessMapMode 1 u8
	tag = AMD_CAM_STATISTICS_SHARPNESS_MAP_MODE;
	if (amd_cam_find_metadata_entry(cur, tag, &e) == OK) {
		if (e.data.u8[0] == AMD_CAM_STATISTICS_SHARPNESS_MAP_MODE_ON)
			f->statistic.afEnabled = true;
		else
			f->statistic.afEnabled = false;

		ISP_PR_DBG("set sharpness map mode to %d",
			   f->statistic.afEnabled);
	}

	//android.statistics.hotPixelMapMode 1 u8
	tag = AMD_CAM_STATISTICS_HOT_PIXEL_MAP_MODE;
	if (amd_cam_find_metadata_entry(cur, tag, &e) == OK) {
		// wait FW support
		//if (e.data.u8[0] == AMD_CAM_STATISTICS_HOT_PIXEL_MAP_MODE_ON)
		//	f->statistic.hotPixelMapEnabled = true;
		//else
		//	f->statistic.hotPixelMapEnabled = false;

		//ISP_PR_DBG("set statistics hot pixel map mode to %d",
		//	   f->statistic.hotPixelMapEnabled);
	}

	//android.statistics.lensShadingMapMode 1 u8
	tag = AMD_CAM_STATISTICS_LENS_SHADING_MAP_MODE;
	if (amd_cam_find_metadata_entry(cur, tag, &e) == OK) {
		if (e.data.u8[0] ==
		    AMD_CAM_STATISTICS_LENS_SHADING_MAP_MODE_ON)
			f->statistic.lensShadingMapEnabled = true;
		else
			f->statistic.lensShadingMapEnabled = false;

		ISP_PR_DBG("set statistics lens shading map mode to %d",
			   f->statistic.lensShadingMapEnabled);
	}

	//vendor tag engineer mode 1 u8
	tag = AMD_CAM_VENDOR_CONTROL_ENGINEERING_MODE;
	if (amd_cam_find_metadata_entry(cur, tag, &e) == OK) {
		if (fc->eng_info)
			addr = (u64)fc->eng_info->gpu_mc_addr;

		if (e.data.u8[0] ==
		    AMD_CAM_VENDOR_CONTROL_ENGINEERING_MODE_ON && addr != 0 &&
		    cam->req->eng_len != 0) {
			memset(fc->eng, 0, sizeof(*fc->eng));
			f->engineerMode.enabled = true;
			f->engineerMode.bufBaseLo = (u32)(addr & 0xffffffff);
			f->engineerMode.bufBaseHi = (u32)(addr >> 32);
			f->engineerMode.bufSize = sizeof(*fc->eng);

			ISP_PR_DBG("engineer mode is %d mc addr %llx len %d",
				  e.data.u8[0], addr, f->engineerMode.bufSize);
			ISP_PR_DBG("set engineer mode to %d lo %u hi %u len %d",
				  f->engineerMode.enabled,
				  f->engineerMode.bufBaseLo,
				  f->engineerMode.bufBaseHi,
				  f->engineerMode.bufSize);
		} else {
			f->engineerMode.enabled = false;
			f->engineerMode.bufBaseLo = 0;
			f->engineerMode.bufBaseHi = 0;
			f->engineerMode.bufSize = 0;
		}
	}

	f->tags = mask;

	//android.sensor.testPatternMode 1 u8
	if (entry_cmp(NULL, AMD_CAM_SENSOR_TEST_PATTERN_MODE, 0, cur, old, &e,
		      &old_e)) {
		a = e.data.u8[0];
		if (a == AMD_CAM_SENSOR_TEST_PATTERN_MODE_OFF) {
			cvip_pattern = TEST_PATTERN_NO;
			pattern = TP_MODE_OFF;
		} else if (a == AMD_CAM_SENSOR_TEST_PATTERN_MODE_SOLID_COLOR) {
			cvip_pattern = TEST_PATTERN_1;
			pattern = TP_MODE_SOLID_COLOR;
		} else if (a == AMD_CAM_SENSOR_TEST_PATTERN_MODE_COLOR_BARS) {
			cvip_pattern = TEST_PATTERN_2;
			pattern = TP_MODE_COLOR_BARS;
		} else if (a ==
		    AMD_CAM_SENSOR_TEST_PATTERN_MODE_COLOR_BARS_FADE_TO_GRAY) {
			cvip_pattern = TEST_PATTERN_3;
			pattern = TP_MODE_COLOR_BARS_FADE_TO_GRAY;
		} else if (a == AMD_CAM_SENSOR_TEST_PATTERN_MODE_PN9) {
			cvip_pattern = TEST_PATTERN_INVALID;
			pattern = TP_MODE_PN9;
		} else {
			cvip_pattern = TEST_PATTERN_INVALID;
			pattern = TP_MODE_CUSTOM1;
		}

		if (g_prop->cvip_enable == CVIP_STATUS_ENABLE) {
			ISP_PR_DBG("set cvip test pattern %d", cvip_pattern);
			ctx.ctx = &cvip_pattern;
		} else {
			ISP_PR_DBG("set test pattern %d", pattern);
			ctx.ctx = &pattern;
		}

		ctx.cam = cam;
		ctx.stream = NULL;
		ret = v4l2_subdev_call(cam->isp_sd, core, ioctl,
				       ISP_IOCTL_SET_TEST_PATTERN, &ctx);
		if (ret != OK)
			ISP_PR_ERR("cam[%d] set test pattern fail! ret:%d",
				   cam->idx, ret);
	}

	cam->pre_pfc = f;
	return ret;
}

bool entry_cmp(u64 *mask, u32 tag, enum _FrameControlBitmask_t bit,
	       struct amd_cam_metadata *new, struct amd_cam_metadata *old,
	       struct amd_cam_metadata_entry *e,
	       struct amd_cam_metadata_entry *old_e)
{
	size_t size;
	bool is_tag_change = false;

	ISP_PR_VERB("Entry! tag[%x] %s", tag,
		    amd_cam_get_metadata_tag_name(tag));
	if (amd_cam_find_metadata_entry(new, tag, e) != OK) {
		ISP_PR_VERB("new md from HAL no entry for %s tag %x",
			    amd_cam_get_metadata_tag_name(tag), tag);
		goto quit;
	}

	if (!old || amd_cam_find_metadata_entry(old, tag, old_e) != OK) {
		ISP_PR_VERB("old md from HAL no entry for %s tag %x",
			    amd_cam_get_metadata_tag_name(tag), tag);
		is_tag_change = true;
	} else {
		size = calc_amd_md_entry_data_size(e->type, e->count);
		if (memcmp(e->data.u8, old_e->data.u8, size) != 0)
			is_tag_change = true;
	}

	if (is_tag_change && mask)
		*mask |= bit;
quit:
	return is_tag_change;
}

int store_request_info(struct physical_cam *cam, struct frame_control *fc)
{
	int i;
	int ret = OK;
	struct kernel_buffer *src = NULL;
	struct kernel_buffer *dst = NULL;
	struct kernel_request *req = cam->req;
	struct kernel_result *rst = &fc->rst;

	ISP_PR_DBG("Entry! cam[%d] com_id %d", cam->idx, req->com_req_id);
	assert(cam && rst);
	init_rst(rst, req->com_req_id, true, 0, req->num_output_buffers);
	fc->hal_md_buf_addr = (uintptr_t)req->kparam;
	fc->eng_ptr = req->eng_ptr;

	for (i = 0; i < req->num_output_buffers; i++) {
		src = req->output_buffers[i];
		dst = &fc->output_buffers[i];
		ret = copy_from_user(dst, src, sizeof(*src));
		if (ret != OK) {
			ISP_PR_ERR("cam[%d] copy_from_user fail!", cam->idx);
			return ret;
		}

		fc->output_buffers[i].status = BUFFER_STATUS_OK;
#ifdef ADVANCED_DEBUG
		ISP_PR_DBG("---------- the enq request info ----------");
		ISP_PR_DBG("virtual cam id = %d", dst->vcam_id);
		ISP_PR_DBG("stream id = %d", dst->stream_id);
		ISP_PR_DBG("buf handle = %p", dst->buffer);
		ISP_PR_DBG("buf MC addr = 0x%llx", dst->mc_addr);
		ISP_PR_DBG("buf CPU VA = %p", dst->user_ptr);
		ISP_PR_DBG("length = %d", dst->length);
		ISP_PR_DBG("stream type = %d", dst->s_type);
		ISP_PR_DBG("wxh = %dx%d", dst->width, dst->height);
		ISP_PR_DBG("stride = %d", dst->y_stride);
		ISP_PR_DBG("format = %s", dst->format);
		ISP_PR_DBG("----------------- end --------------------");
#endif
	}

	return ret;
}

void init_rst(struct kernel_result *rst, u32 id, bool shutter,
	      u64 timestamp, u64 buf_cnt)
{
	rst->num_output_buffers = buf_cnt;
	rst->partial_result = 1;
	rst->status = ERROR_NONE;
	rst->input_buffer = NULL;
	rst->com_req_id = id;
	rst->is_shutter_event = shutter;
	rst->data.timestamp = timestamp;
}

int deq_result(struct physical_cam *cam, struct kernel_result *rst,
	       struct file *f)
{
	int ret = OK;
	struct v4l2_buffer vbuf;
	unsigned long flags = 0;
	struct vb2_queue *q = &cam->vidq;

	ISP_PR_DBG("Entry! cam[%d]", cam->idx);
	assert(rst);
	if (!cam->fc) {
		vbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		vbuf.memory = V4L2_MEMORY_USERPTR;
		vbuf.flags &= ~V4L2_BUF_FLAG_LAST;
		do {
			mutex_lock(q->lock);
			ret = vb2_dqbuf(q, &vbuf, f->f_flags & O_NONBLOCK);
			mutex_unlock(q->lock);
		} while(ret == -ERESTARTSYS);
		if (ret == OK && cam->fc) {
			send_shutter(cam, rst);
		} else {
			ISP_PR_ERR("cam[%d] vb2_dq fail ret:%d", cam->idx, ret);
			rst->status = ERROR_INVALIDOPERATION;
		}
	} else {
		ret = send_buf_rst(cam, rst);
		if (ret != OK) {
			ISP_PR_ERR("cam[%d] send rst fail %d", cam->idx, ret);
			rst->status = ERROR_BADPOINTER;
		}

		check_isp_clk_switch(cam);
		cam->fc = NULL;
		spin_lock_irqsave(&cam->b_lock, flags);
		if (cam->tear_down && list_empty(&cam->enq_list)) {
			cam->tear_down = false;
			spin_unlock_irqrestore(&cam->b_lock, flags);
			ISP_PR_INFO("cam[%d] wake up sensor close", cam->idx);
			wake_up(&cam->done_wq);
		} else {
			spin_unlock_irqrestore(&cam->b_lock, flags);
		}
	}

	return ret;
}

void send_shutter(struct physical_cam *cam, struct kernel_result *kr)
{
	u64 timestamp = 0;
	struct timespec time;
	struct frame_control *tmp = NULL;
	struct frame_control *tmp_fc = NULL;
	struct frame_control *fc = cam->fc;

	ISP_PR_DBG("Entry! com_id:%d poc:%d fcid:%d", fc->rst.com_req_id,
		   fc->mi->poc, fc->mi->fcId);
	if (fc->mc != fc->mi_mem_info->gpu_mc_addr) {
		ISP_PR_ERR("cam[%d] result out-of-order!", cam->idx);
		fc->rst.status = ERROR_BADPOINTER;
		list_for_each_entry_safe(tmp_fc, tmp, &cam->enq_list, entry) {
			if (fc->mc == tmp_fc->mi_mem_info->gpu_mc_addr) {
				ISP_PR_ERR("cam[%d] expect %d, but got %d",
					   cam->idx, fc->mi->fcId,
					   tmp_fc->mi->fcId);
				fc->mi = tmp_fc->mi;
				break;
			}
		}
	}

	//get the system time as the timestamp.
	ktime_get_ts(&time);
	timestamp = SEC_TO_NANO_SEC(time.tv_sec);
	timestamp += time.tv_nsec;

	fc->rst.data.timestamp = timestamp;
	init_rst(kr, fc->rst.com_req_id, true, fc->rst.data.timestamp, 0);
	ISP_PR_DBG("suc for com_id %d timestamp[%llu]",
		kr->com_req_id, kr->data.timestamp);
}

int send_buf_rst(struct physical_cam *cam, struct kernel_result *kr)
{
	ulong len;
	int i, ret = OK;
	struct kernel_buffer *b = NULL;
	struct kernel_result *rst = &cam->fc->rst;
	struct _MetaInfo_t *mi = cam->fc->mi;
#ifdef DUMP_IMG_TO_FILE
	struct file *fp = NULL;
#endif

	ISP_PR_DBG("Entry! cam[%d] com_id %d", cam->idx, rst->com_req_id);
	//basic info
	init_rst(kr, rst->com_req_id, false, rst->data.timestamp,
		 rst->num_output_buffers);

	//parameters rst info
	kr->data.kparam_rst = (void *)cam->fc->hal_md_buf_addr;
	ret = unmap_metadata_2_framectrl(cam);
	cam->fc->hal_md_buf_addr = 0;
	if (ret != OK)
		goto quit;

	//img buf info
	for (i = 0; i < rst->num_output_buffers; i++) {
		if (!kr->output_buffers[i]) {
			ISP_PR_ERR("fail! no cam[%d] buf[%d]", cam->idx, i);
			ret = -EINVAL;
			goto quit;
		}

		b = &cam->fc->output_buffers[i];
		len = b->length;
		if (b->s_type == STREAM_YUV_0) {
			check_buf_status(&mi->preview, b, cam->idx, b->s_type);
#ifdef DUMP_IMG_TO_FILE
			if (mi->fcId % 19 == 5)
				fp = filp_open("/data/dump/k-yuv-preview.yuv",
					       O_RDWR | O_TRUNC | O_CREAT,
					       0666);
#endif
		} else if (b->s_type == STREAM_YUV_1) {
			check_buf_status(&mi->still, b, cam->idx, b->s_type);
#ifdef DUMP_IMG_TO_FILE
			if (mi->fcId % 19 == 7)
				fp = filp_open("/data/dump/k-yuv-still.yuv",
					       O_RDWR | O_TRUNC | O_CREAT,
					       0666);
#endif
		} else if (b->s_type == STREAM_YUV_2) {
			if (g_prop->cvip_enable == CVIP_STATUS_ENABLE) {
				check_buf_status(&mi->cv, b, cam->idx,
						 b->s_type);
#ifdef DUMP_IMG_TO_FILE
				fp = filp_open("/data/dump/k-yuv-x86cv.yuv",
					       O_RDWR | O_TRUNC | O_CREAT,
					       0666);
#endif
			} else {
				check_buf_status(&mi->video, b, cam->idx,
						 b->s_type);
#ifdef DUMP_IMG_TO_FILE
				fp = filp_open("/data/dump/k-yuv-video.yuv",
					       O_RDWR | O_TRUNC | O_CREAT,
					       0666);
#endif
			}
		} else if (b->s_type == STREAM_CVPCV) {
			check_buf_status(&mi->cvpcv, b, cam->idx, b->s_type);
		} else if (b->s_type == STREAM_RAW) {
			check_buf_status(&mi->raw, b, cam->idx, b->s_type);
#ifdef DUMP_IMG_TO_FILE
			if (mi->fcId % 19 == 10)
				fp = filp_open("/data/dump/k-yuv-raw.yuv",
					       O_RDWR | O_TRUNC | O_CREAT,
					       0666);
#endif
		} else {
			ISP_PR_ERR("c_%d s_%d unknown stream type", cam->idx,
				   b->s_type);
		}

#ifdef DUMP_IMG_TO_FILE
		if (fp && !IS_ERR(fp)) {
			isp_write_file_test(fp, b->user_ptr, &len);
			ISP_PR_DBG("dump s[%d] buf done for com_id %d",
				   b->s_type, rst->com_req_id);
			filp_close(fp, NULL);
			fp = NULL;
		}
#endif

		ret = copy_to_user(kr->output_buffers[i], b, sizeof(*b));
		if (ret != OK) {
			ISP_PR_ERR("cam[%d] copy_to_user fail!", cam->idx);
			goto quit;
		}

#ifdef ADVANCED_DEBUG
		ISP_PR_DBG("=========== stream & buffer info =========");
		ISP_PR_DBG("com_id = %d", rst->com_req_id);
		ISP_PR_DBG("virtual cam id = %d", b->vcam_id);
		ISP_PR_DBG("stream type = %d", b->s_type);
		ISP_PR_DBG("wxh = %dx%d", b->width, b->height);
		ISP_PR_DBG("stride = %d", b->y_stride);
		ISP_PR_DBG("buf handle = %p", b->buffer);
		ISP_PR_DBG("buf MC addr = 0x%llx", b->mc_addr);
		ISP_PR_DBG("buf CPU VA = %p", b->user_ptr);
		ISP_PR_DBG("==========================================");
#endif
		memset(b, 0x0, sizeof(*b));
	}

	ISP_PR_DBG("for cam[%d] com_id %d suc", cam->idx, kr->com_req_id);
quit:
	return ret;
}

int unmap_metadata_2_framectrl(struct physical_cam *cam)
{
	int i, cnt, ret, result = OK;
	u32 tag;
	u8 u8;
	s64 i64;
	u64 u64;
	float mipi_focus_distance;
	s32 *p_s32, *p_r, *p_gr, *p_gb, *p_b;
	s32 i32[258] = {0};
	const void *data;
	struct _FrameControl_t *f = &cam->fc->fw_fc.frameCtrl;
	struct _MetaInfo_t *mi = cam->fc->mi;
	struct kernel_result *rst = &cam->fc->rst;
	struct amd_cam_metadata *md = cam->md;
	struct amd_cam_metadata *hal_md;
	struct amd_cam_metadata_entry e;
	s64 timestamps[4] = {0};
	struct _CvipAfRoi_t *af_roi = NULL;
	struct _Window_t *win = NULL;

	ENTER();
	hal_md = (struct amd_cam_metadata *)cam->fc->hal_md_buf_addr;
	memset(md, 0, MAX_KERN_METADATA_BUF_SIZE);
	ret = copy_from_user(md, hal_md, MAX_KERN_METADATA_BUF_SIZE);
	if (ret != OK) {
		ISP_PR_ERR("copy_from_user HAL metadata fail!");
		return ret;
	}

	//android.sensor.timestamp
	u64 = rst->data.timestamp;
	tag = AMD_CAM_SENSOR_TIMESTAMP;
	data = (const void *)&u64;
	if (amd_cam_update_tag(md, tag, data, 1, NULL) != OK)
		ISP_PR_WARN("update sensor timestamp fail!");

	//android.control.captureIntent 1 u8
	ISP_PR_DBG("intent mode rst %d", mi->captureIntend.mode);
	if (mi->captureIntend.mode != f->captureIntent.mode) {
		if (mi->captureIntend.mode == SCENARIO_MODE_STILL)
			u8 = AMD_CAM_CONTROL_CAPTURE_INTENT_STILL_CAPTURE;
		else
			u8 = AMD_CAM_CONTROL_CAPTURE_INTENT_VIDEO_RECORD;

		tag = AMD_CAM_CONTROL_CAPTURE_INTENT;
		data = (const void *)&u8;
		if (amd_cam_update_tag(md, tag, data, 1, NULL) != OK)
			ISP_PR_WARN("update intent mode fail!");
	}

	//AE
	//android.control.aeMode 1 u8
	ISP_PR_DBG("AE mode rst %d", mi->ae.mode);
	if (mi->ae.mode != f->ae.mode) {
		if  (mi->ae.mode == AE_MODE_MANUAL)
			u8 = AMD_CAM_CONTROL_AE_MODE_OFF;
		else
			u8 = AMD_CAM_CONTROL_AE_MODE_ON;

		tag = AMD_CAM_CONTROL_AE_MODE;
		data = (const void *)&u8;
		if (amd_cam_update_tag(md, tag, data, 1, NULL) != OK)
			ISP_PR_WARN("update AE mode fail!");
	}

	//android.control.aeAntibandingMode 1 u8
	ISP_PR_DBG("banding mode rst %d", mi->ae.flickerType);
	if (mi->ae.flickerType != f->ae.flickerType) {
		if (mi->ae.flickerType == AE_FLICKER_TYPE_50HZ)
			u8 = AMD_CAM_CONTROL_AE_ANTIBANDING_MODE_50HZ;
		else if (mi->ae.flickerType == AE_FLICKER_TYPE_60HZ)
			u8 = AMD_CAM_CONTROL_AE_ANTIBANDING_MODE_60HZ;
		else
			u8 = AMD_CAM_CONTROL_AE_ANTIBANDING_MODE_OFF;

		tag = AMD_CAM_CONTROL_AE_ANTIBANDING_MODE;
		data = (const void *)&u8;
		if (amd_cam_update_tag(md, tag, data, 1, NULL) != OK)
			ISP_PR_WARN("update banding mode fail!");
	}

	//android.control.aeRegions 5n i32 (xmin, ymin, xmax, ymax, w[0, 1000])
	cnt = mi->ae.region.regionCnt;
	if (cnt > 0) {
		for (i = 0; i < cnt; i++) {
			i32[5 * i + 4] = (s32)mi->ae.region.region[i].weight;
			win = &mi->ae.region.region[i].window;
			ISP_PR_DBG("FW AE ROI[%d] rst, weight is %d",
				   i, i32[5 * i + 4]);
			if (unmap_roi_ae(win, &i32[5 * i], cam->fc->prf_id)
			    != OK) {
				ISP_PR_ERR("unmap AE ROI[%d] failed", i);
				cnt = 1;
				memset(i32, 0x0, 5 * sizeof(i32[0]));
				break;
			}
		}
	} else {
		//set the default values when ROI num is 0 from FW
		ISP_PR_DBG("AE ROI rst num from FW is 0, set to default value");
		cnt = 1;
		memset(i32, 0x0, 5 * sizeof(i32[0]));
	}

	tag = AMD_CAM_CONTROL_AE_REGIONS;
	data = (const void *)i32;
	if (amd_cam_update_tag(md, tag, data, 5 * cnt, NULL) != OK)
		ISP_PR_WARN("update AE ROI fail!");

	//android.control.aeLock 1 u8
	ISP_PR_DBG("AE lock rst %d", mi->ae.lockState);
	if (mi->ae.lockState == AE_LOCK_STATE_LOCKED)
		u8 = AMD_CAM_CONTROL_AE_LOCK_ON;
	else
		u8 = AMD_CAM_CONTROL_AE_LOCK_OFF;

	tag = AMD_CAM_CONTROL_AE_LOCK;
	data = (const void *)&u8;
	if (amd_cam_update_tag(md, tag, data, 1, NULL) != OK)
		ISP_PR_WARN("update AE lock fail!");

	//android.control.aeState 1 u8
	//not support the state AMD_CAM_CONTROL_AE_STATE_FLASH_REQUIRED
	ISP_PR_DBG("AE state rst %d", mi->ae.searchState);
	if (mi->ae.searchState == AE_SEARCH_STATE_SEARCHING) {
		if (mi->ae.lockState == AE_LOCK_STATE_LOCKED)
			u8 = AMD_CAM_CONTROL_AE_STATE_LOCKED;
		else
			u8 = AMD_CAM_CONTROL_AE_STATE_SEARCHING;
	} else if (mi->ae.searchState == AE_SEARCH_STATE_PRECAPTURE) {
		u8 = AMD_CAM_CONTROL_AE_STATE_PRECAPTURE;
	} else if (mi->ae.searchState == AE_SEARCH_STATE_CONVERGED) {
		if (mi->ae.lockState == AE_LOCK_STATE_LOCKED)
			u8 = AMD_CAM_CONTROL_AE_STATE_LOCKED;
		else
			u8 = AMD_CAM_CONTROL_AE_STATE_CONVERGED;
	} else {//AE_SEARCH_STATE_INVALID
		u8 = AMD_CAM_CONTROL_AE_STATE_INACTIVE;
	}

	tag = AMD_CAM_CONTROL_AE_STATE;
	data = (const void *)&u8;
	if (amd_cam_update_tag(md, tag, data, 1, NULL) != OK)
		ISP_PR_WARN("update AE state fail!");

	//android.control.aeExposureCompensation 1 i32
	ISP_PR_DBG("EV rst %d/%d", mi->ae.compensateEv.numerator,
		   mi->ae.compensateEv.denominator);
	if (mi->ae.compensateEv.denominator != 0 &&
	    f->ae.evCompensation.denominator != 0 &&
	    f->ae.evCompensation.numerator != mi->ae.compensateEv.numerator) {
		i32[0] = mi->ae.compensateEv.numerator * STEP_DENOMINATOR /
			 (mi->ae.compensateEv.denominator * STEP_NUMERATOR);
		tag = AMD_CAM_CONTROL_AE_EXPOSURE_COMPENSATION;
		data = (const void *)i32;
		if (amd_cam_update_tag(md, tag, data, 1, NULL) != OK)
			ISP_PR_WARN("update EV fail!");
	}

	//android.sensor.exposureTime 1 i64 ns & FW is us (1us = 1000ns)
	ISP_PR_DBG("exposure time rst %d", mi->ae.itime[0]);
	if (mi->ae.itime[0] != f->ae.iTime) {
		i64 = (s64)mi->ae.itime[0] * 1000;
		tag = AMD_CAM_SENSOR_EXPOSURE_TIME;
		data = (const void *)&i64;
		if (amd_cam_update_tag(md, tag, data, 1, NULL) != OK)
			ISP_PR_WARN("update exposure time fail!");
	}

	//android.sensor.sensitivity 1 i32
	ISP_PR_DBG("sensitivity rst %d", mi->ae.aGainIso);
	if (mi->ae.aGainIso != f->ae.aGain) {
		i32[0] = (s32)mi->ae.aGainIso;
		tag = AMD_CAM_SENSOR_SENSITIVITY;
		data = (const void *)i32;
		if (amd_cam_update_tag(md, tag, data, 1, NULL) != OK)
			ISP_PR_WARN("update sensitivity fail!");
	}

	//android.control.postRawSensitivityBoost 1 i32
	ISP_PR_DBG("isp gain rst %d", mi->ae.ispGainIso);
	if (mi->ae.ispGainIso != f->ae.ispGain) {
		i32[0] = (s32)mi->ae.ispGainIso;
		tag = AMD_CAM_CONTROL_POST_RAW_SENSITIVITY_BOOST;
		data = (const void *)i32;
		if (amd_cam_update_tag(md, tag, data, 1, NULL) != OK)
			ISP_PR_WARN("update isp gain fail!");
	}

	//android.sensor.frameDuration 1 i64 ns, FW is us
	ISP_PR_DBG("frame duration rst %dus", mi->ae.frameDuration);
	if (mi->ae.frameDuration != f->ae.frameDuration) {
		i64 = (s64)mi->ae.frameDuration * 1000;
		tag = AMD_CAM_SENSOR_FRAME_DURATION;
		data = (const void *)&i64;
		if (amd_cam_update_tag(md, tag, data, 1, NULL) != OK)
			ISP_PR_WARN("update frame duration fail!");
	}

	//AWB
	//android.control.awbMode 1 u8
	ISP_PR_DBG("AWB mode rst %d, light %d", mi->awb.mode,
		   mi->awb.lightSource);
	if (mi->awb.mode != f->awb.mode ||
	    mi->awb.lightSource != f->awb.lightSource) {
		if (mi->awb.mode == AWB_MODE_AUTO)
			u8 = AMD_CAM_CONTROL_AWB_MODE_AUTO;
		else if (mi->awb.lightSource == AWB_LIGHT_SOURCE_INCANDESCENT)
			u8 = AMD_CAM_CONTROL_AWB_MODE_INCANDESCENT;
		else if (mi->awb.lightSource == AWB_LIGHT_SOURCE_FLUORESCENT)
			u8 = AMD_CAM_CONTROL_AWB_MODE_FLUORESCENT;
		else if (mi->awb.lightSource ==
			AWB_LIGHT_SOURCE_WARM_FLUORESCENT)
			u8 = AMD_CAM_CONTROL_AWB_MODE_WARM_FLUORESCENT;
		else if (mi->awb.lightSource == AWB_LIGHT_SOURCE_DAYLIGHT)
			u8 = AMD_CAM_CONTROL_AWB_MODE_DAYLIGHT;
		else if (mi->awb.lightSource == AWB_LIGHT_SOURCE_CLOUDY)
			u8 = AMD_CAM_CONTROL_AWB_MODE_CLOUDY_DAYLIGHT;
		else if (mi->awb.lightSource == AWB_LIGHT_SOURCE_TWILIGHT)
			u8 = AMD_CAM_CONTROL_AWB_MODE_TWILIGHT;
		else if (mi->awb.lightSource == AWB_LIGHT_SOURCE_SHADE)
			u8 = AMD_CAM_CONTROL_AWB_MODE_SHADE;
		else
			u8 = AMD_CAM_CONTROL_AWB_MODE_OFF;

		tag = AMD_CAM_CONTROL_AWB_MODE;
		data = (const void *)&u8;
		if (amd_cam_update_tag(md, tag, data, 1, NULL) != OK)
			ISP_PR_WARN("update AWB mode fail!");
	}

	//android.control.awbLock 1 u8
	ISP_PR_DBG("AWB lock rst %d", mi->awb.lockState);
	if ((mi->awb.lockState == AWB_LOCK_STATE_LOCKED &&
	     f->awb.lockType == LOCK_TYPE_UNLOCK) ||
	    (mi->awb.lockState != AWB_LOCK_STATE_LOCKED &&
	     f->awb.lockType == LOCK_TYPE_IMMEDIATELY)) {
		if (mi->awb.lockState == AWB_LOCK_STATE_LOCKED) {
			u8 = AMD_CAM_CONTROL_AWB_LOCK_ON;
		} else {
			u8 = AMD_CAM_CONTROL_AWB_LOCK_OFF;
		}

		tag = AMD_CAM_CONTROL_AWB_LOCK;
		data = (const void *)&u8;
		if (amd_cam_update_tag(md, tag, data, 1, NULL) != OK)
			ISP_PR_WARN("update AWB lock fail!");
	}

	//android.control.awbState 1 u8
	ISP_PR_DBG("AWB state rst %d", mi->awb.searchState);
	if (mi->awb.searchState == AWB_SEARCH_STATE_SEARCHING) {
		u8 = AMD_CAM_CONTROL_AWB_STATE_SEARCHING;
	} else if (mi->awb.searchState == AWB_SEARCH_STATE_CONVERGED) {
		if (mi->awb.lockState == AWB_LOCK_STATE_LOCKED)
			u8 = AMD_CAM_CONTROL_AWB_STATE_LOCKED;
		else
			u8 = AMD_CAM_CONTROL_AWB_STATE_CONVERGED;
	} else {
		u8 = AMD_CAM_CONTROL_AWB_STATE_INACTIVE;
	}

	tag = AMD_CAM_CONTROL_AWB_STATE;
	data = (const void *)&u8;
	if (amd_cam_update_tag(md, tag, data, 1, NULL) != OK)
		ISP_PR_WARN("update AWB state fail!");

	//android.colorCorrection.gains 4 f
#ifdef PRINT_FLOAT_INFO
	kernel_fpu_begin();
	ISP_PR_INFO("AWB color correction gains rst (x1000) R%d GR%d GB%d B%d",
		    (s32)(mi->awb.wbGain.red * 1000),
		    (s32)(mi->awb.wbGain.greenR * 1000),
		    (s32)(mi->awb.wbGain.greenB * 1000),
		    (s32)(mi->awb.wbGain.blue * 1000));
	kernel_fpu_end();
#endif
	ret = memcmp(&mi->awb.wbGain, &f->awb.wbGain, sizeof(struct _WbGain_t));
	if (ret) {
		tag = AMD_CAM_COLOR_CORRECTION_GAINS;
		data = (const void *)&mi->awb.wbGain;
		if (amd_cam_update_tag(md, tag, data, 4, NULL) != OK)
			ISP_PR_WARN("update color correction gains fail");
	}

	//android.colorCorrection.transform 9 f
#ifdef PRINT_FLOAT_INFO
	kernel_fpu_begin();
	for (i = 0; i < TRANSFORM_CNT; i++)
		ISP_PR_INFO("AWB color transform rst[%d] (x1000)%d", i,
			    (s32)(mi->awb.ccMatrix.coeff[i] * 1000));
	kernel_fpu_end();
#endif
	ret = memcmp(&mi->awb.ccMatrix, &f->awb.CCM,
		     sizeof(struct _CcMatrix_t));
	if (ret) {
		tag = AMD_CAM_COLOR_CORRECTION_TRANSFORM;
		data = (const void *)&mi->awb.ccMatrix;
		if (amd_cam_update_tag(md, tag, data, TRANSFORM_CNT, NULL) !=
		    OK)
			ISP_PR_WARN("update color transform fail!");
	}

	//com.amd.control.ae_roi_touch_target 1 i32 & FW is uint32
	ISP_PR_DBG("touch target rst %d", mi->ae.touchTarget);
	if (mi->ae.touchTarget != f->ae.touchTarget) {
		i32[0] = (s32)mi->ae.touchTarget;
		tag = AMD_CAM_VENDOR_CONTROL_AE_TOUCH_TARGET;
		data = (const void *)i32;
		if (amd_cam_update_tag(md, tag, data, 1, NULL) != OK)
			ISP_PR_WARN("update touch target fail!");
	}

	//com.amd.control.ae_roi_touch_target_weight 1 i32
	ISP_PR_DBG("touch target weight rst %d", mi->ae.touchTargetWeight);
	if (mi->ae.touchTargetWeight != f->ae.touchTargetWeight) {
		i32[0] = mi->ae.touchTargetWeight;
		tag = AMD_CAM_VENDOR_CONTROL_AE_TOUCH_TARGET_WEIGHT;
		data = (const void *)i32;
		if (amd_cam_update_tag(md, tag, data, 1, NULL) != OK)
			ISP_PR_WARN("update touch target weight fail!");
	}

	//AF
	//android.control.afMode 1 u8
	data = NULL;
	if (g_prop->cvip_enable == CVIP_STATUS_ENABLE) {
		ISP_PR_DBG("cvip AF mode rst %d", mi->preData.afMetaData.mode);
		if (mi->preData.afMetaData.mode != f->cvipSensor.afData.mode) {
			u8 = mi->preData.afMetaData.mode;
			data = (const void *)&u8;
		}
	} else {
		ISP_PR_DBG("AF mode rst %d (1-manual 6-auto)", mi->af.mode);
		if (mi->af.mode != f->af.mode) {
			if (mi->af.mode == AF_MODE_AUTO)
				u8 = AMD_CAM_CONTROL_AF_MODE_AUTO;
			else if (mi->af.mode == AF_MODE_CONTINUOUS_VIDEO)
				u8 = AMD_CAM_CONTROL_AF_MODE_CONTINUOUS_VIDEO;
			else if (mi->af.mode == AF_MODE_CONTINUOUS_PICTURE)
				u8 = AMD_CAM_CONTROL_AF_MODE_CONTINUOUS_PICTURE;
			else
				u8 = AMD_CAM_CONTROL_AF_MODE_OFF;
			data = (const void *)&u8;
		}
	}

	if (data) {
		tag = AMD_CAM_CONTROL_AF_MODE;
		if (amd_cam_update_tag(md, tag, data, 1, NULL) != OK)
			ISP_PR_WARN("update af mode fail!");
	}

	//android.control.afRegions (only for CVIP)
	if (g_prop->cvip_enable == CVIP_STATUS_ENABLE) {
		tag = AMD_CAM_CONTROL_AF_REGIONS;
		af_roi = &mi->preData.afMetaData.cvipAfRoi;
		cnt = af_roi->numRoi;
		if (cnt > 0) {
			for (i = 0; i < cnt; i++) {
				if (unmap_roi_af(&af_roi->roiAf[i], &i32[5 * i],
						 cam->fc->prf_id) != OK) {
					ISP_PR_ERR("unmap AF ROI[%d] fail", i);
					cnt = 1;
					memset(i32, 0x0, 5 * sizeof(i32[0]));
					break;
				}
			}
		} else if (amd_cam_find_metadata_entry(md, tag, &e) == OK) {
			//set to the default values when ROI num is 0 from FW,
			//and the ROI num is not 0 from HAL.
			ISP_PR_INFO("no AF ROI from FW, set to default value");
			cnt = 1;
			memset(i32, 0x0, 5 * sizeof(i32[0]));
		} else {
			//do nothing when HAL doesn't set AF ROI
		}

		if (cnt > 0) {
			data = (const void *)i32;
			if (amd_cam_update_tag(md, tag, data, 5 * cnt, NULL) !=
			    OK)
				ISP_PR_ERR("update AF ROI fail!");
		}
	}

	//android.lens.focusDistance 1 f
	if (g_prop->cvip_enable == CVIP_STATUS_ENABLE) {
#ifdef PRINT_FLOAT_INFO
		kernel_fpu_begin();
		ISP_PR_INFO("lens focus distance rst (x1000) %d diopter",
			    (s32)(mi->preData.afMetaData.distance * 1000));
		kernel_fpu_end();
#endif
		data = (const void *)&mi->preData.afMetaData.distance;
	} else {
		/* MIPI sensor algo cannot support diopter focus distance,
		 * so the unit is length, and the data type is unsigned int;
		 * which are all different from CVIP's, so print it for tuning
		 * guys, and return 0.0 float value in result to HAL.
		 */
		ISP_PR_INFO("lens focus distance rst %d", mi->af.lensPos);
		memset(&mipi_focus_distance, 0, sizeof(float));
		data = (const void *)&mipi_focus_distance;
	}

	tag = AMD_CAM_LENS_FOCUS_DISTANCE;
	if (amd_cam_update_tag(md, tag, data, 1, NULL) != OK)
		ISP_PR_WARN("update focus distance fail!");

	//android.lens.focusRange 2 f
#ifdef PRINT_FLOAT_INFO
	kernel_fpu_begin();
	if (g_prop->cvip_enable == CVIP_STATUS_ENABLE) {
		ISP_PR_INFO("focus range rst (x1000) near:%d far:%d diopter",
			    (s32)(mi->preData.afMetaData.focusRangeNear * 1000),
			    (s32)(mi->preData.afMetaData.focusRangeFar * 1000));
	} else {
		ISP_PR_INFO("focus range rst (x1000) near:%d far:%d diopter",
			    (s32)(mi->Lens.focusRangeNear * 1000),
			    (s32)(mi->Lens.focusRangeFar * 1000));
	}
	kernel_fpu_end();
#endif

	tag = AMD_CAM_LENS_FOCUS_RANGE;
	data = (g_prop->cvip_enable == CVIP_STATUS_ENABLE) ?
	       (const void *)&mi->preData.afMetaData.focusRangeNear :
	       (const void *)&mi->Lens.focusRangeNear;
	if (amd_cam_update_tag(md, tag, data, RANGE_VALUES_CNT, NULL) != OK)
		ISP_PR_WARN("update focus range fail!");

	//android.control.afTrigger 1 u8
	if (g_prop->cvip_enable == CVIP_STATUS_ENABLE) {
		if (mi->preData.afMetaData.trigger !=
		    f->cvipSensor.afData.trigger) {
			tag = AMD_CAM_CONTROL_AF_TRIGGER;
			u8 = mi->preData.afMetaData.trigger;
			ISP_PR_DBG("CVIP AF trigger rst %d", u8);
			data = (const void *)&u8;
			if (amd_cam_update_tag(md, tag, data, 1, NULL) != OK)
				ISP_PR_WARN("update cvip af trigger fail!");
		}
	} else {
		if (cam->af_mode != f->af.mode)
			cam->af_trigger = AMD_CAM_CONTROL_AF_TRIGGER_IDLE;

		tag = AMD_CAM_CONTROL_AF_TRIGGER;
		if (amd_cam_find_metadata_entry(md, tag, &e) == OK) {
			if (e.data.u8[0] > AMD_CAM_CONTROL_AF_TRIGGER_IDLE)
				cam->af_trigger = e.data.u8[0];
		} else {
			e.data.u8[0] = AMD_CAM_CONTROL_AF_TRIGGER_IDLE;
		}
	}

	//android.control.afState 1 u8
	if (g_prop->cvip_enable == CVIP_STATUS_ENABLE) {
		u8 = mi->preData.afMetaData.state;
		ISP_PR_DBG("CVIP AF state rst %d", u8);
	} else {
		if (cam->af_trigger == AMD_CAM_CONTROL_AF_TRIGGER_CANCEL) {
			u8 = AMD_CAM_CONTROL_AF_STATE_INACTIVE;
			cam->af_trigger = AMD_CAM_CONTROL_AF_TRIGGER_IDLE;
		} else if (f->af.mode == AF_MODE_MANUAL) {
			u8 = AMD_CAM_CONTROL_AF_STATE_INACTIVE;
		} else if (e.data.u8[0] == AMD_CAM_CONTROL_AF_TRIGGER_START &&
			   f->af.mode == AF_MODE_AUTO) {
			u8 = AMD_CAM_CONTROL_AF_STATE_ACTIVE_SCAN;
		} else if (e.data.u8[0] == AMD_CAM_CONTROL_AF_TRIGGER_START &&
			   cam->af_state == AMD_CAM_CONTROL_AF_STATE_INACTIVE &&
			   f->af.mode >= AF_MODE_CONTINUOUS_PICTURE) {
			u8 = AMD_CAM_CONTROL_AF_STATE_NOT_FOCUSED_LOCKED;
		} else if (mi->af.searchState == AF_SEARCH_STATE_SEARCHING) {
			if (cam->af_state ==
			    AMD_CAM_CONTROL_AF_STATE_FOCUSED_LOCKED &&
			    f->af.mode >= AF_MODE_CONTINUOUS_PICTURE)
				u8 = AMD_CAM_CONTROL_AF_STATE_INACTIVE;
			else if (e.data.u8[0] ==
				 AMD_CAM_CONTROL_AF_TRIGGER_START &&
				 f->af.mode == AF_MODE_CONTINUOUS_VIDEO)
				u8 = AMD_CAM_CONTROL_AF_STATE_FOCUSED_LOCKED;
			else
				u8 = (f->af.mode == AF_MODE_AUTO) ?
				     AMD_CAM_CONTROL_AF_STATE_ACTIVE_SCAN :
				     AMD_CAM_CONTROL_AF_STATE_PASSIVE_SCAN;
		} else if (mi->af.searchState == AF_SEARCH_STATE_CONVERGED) {
			if (cam->af_trigger == AMD_CAM_CONTROL_AF_TRIGGER_START)
				u8 = AMD_CAM_CONTROL_AF_STATE_FOCUSED_LOCKED;
			else
				u8 = (f->af.mode == AF_MODE_AUTO) ?
				     AMD_CAM_CONTROL_AF_STATE_FOCUSED_LOCKED :
				     AMD_CAM_CONTROL_AF_STATE_PASSIVE_FOCUSED;
		} else if (mi->af.searchState == AF_SEARCH_STATE_FAILED) {
			u8 = AMD_CAM_CONTROL_AF_STATE_NOT_FOCUSED_LOCKED;
		} else {
			u8 = AMD_CAM_CONTROL_AF_STATE_INACTIVE;
		}

		cam->af_state = u8;
		cam->af_mode = f->af.mode;
		ISP_PR_DBG("AF state ISP rst %d HAL rst %d",
			   mi->af.searchState, u8);
	}

	tag = AMD_CAM_CONTROL_AF_STATE;
	data = (const void *)&u8;
	if (amd_cam_update_tag(md, tag, data, 1, NULL) != OK)
		ISP_PR_WARN("update AF state fail!");

	//android.control.afSceneChange 1 u8 (only for CVIP)
	if (g_prop->cvip_enable == CVIP_STATUS_ENABLE) {
		u8 = mi->preData.afMetaData.scene_change;
		ISP_PR_DBG("CVIP AF scene change rst %d", u8);
		tag = AMD_CAM_CONTROL_AF_SCENE_CHANGE;
		data = (const void *)&u8;
		if (amd_cam_update_tag(md, tag, data, 1, NULL) != OK)
			ISP_PR_WARN("update AF scene change fail!");
	}

	//vendor tag sweep distance range 2 f, unit is diopter (only for CVIP)
	if (g_prop->cvip_enable == CVIP_STATUS_ENABLE) {
#ifdef PRINT_FLOAT_INFO
		kernel_fpu_begin();
		ISP_PR_INFO("sweep dis range rst(x1000) near:%d far:%d diopter",
			    (s32)(mi->preData.afMetaData.distanceNear * 1000),
			    (s32)(mi->preData.afMetaData.distanceFar * 1000));
		kernel_fpu_end();
#endif
		if (memcmp(&mi->preData.afMetaData.distanceNear,
			   &f->cvipSensor.afData.distanceNear,
			   sizeof(float) * RANGE_VALUES_CNT)) {
			tag = AMD_CAM_VENDOR_CONTROL_LENS_SWEEP_DISTANCERANGE;
			data = (const void *)
				&mi->preData.afMetaData.distanceNear;
			if (amd_cam_update_tag(md, tag, data, RANGE_VALUES_CNT,
					       NULL) != OK)
				ISP_PR_WARN("update lens distance range fail!");
		}
	}

	//vendor tag cvip timestamps 4 i64
	if (g_prop->cvip_enable == CVIP_STATUS_ENABLE) {
		ISP_PR_DBG("cvip timestamps [%lld, %lld, %lld, %lld]",
			mi->preData.timestampInPre.readoutStartTimestampUs,
			mi->preData.timestampInPre.centroidTimestampUs,
			mi->preData.timestampInPre.seqWinTimestampUs,
			mi->postData.timestampInPost.readoutEndTimestampUs);
		tag = AMD_CAM_VENDOR_CONTROL_CVIP_TIMESTAMPS;
		// convert from microseconds to nanoseconds
		timestamps[0] =
			mi->preData.timestampInPre.readoutStartTimestampUs *
				1000;
		timestamps[1] =
			mi->preData.timestampInPre.centroidTimestampUs *
				1000;
		timestamps[2] =
			mi->preData.timestampInPre.seqWinTimestampUs *
				1000;
		timestamps[3] =
			mi->postData.timestampInPost.readoutEndTimestampUs *
				1000;
		data = (const void *)&timestamps;
		if (amd_cam_update_tag(md, tag, data, 4, NULL) != OK)
			ISP_PR_ERR("update cvip timestamps fail!");
	}

	//android.lens.state 1 u8 (only for CVIP)
	if (g_prop->cvip_enable == CVIP_STATUS_ENABLE) {
		ISP_PR_DBG("CVIP lens state rst %d",
			   mi->preData.afMetaData.lensState);
		if (mi->preData.afMetaData.lensState ==
		    CVIP_LENS_STATE_SEARCHING)
			u8 = AMD_CAM_LENS_STATE_MOVING;
		else
			u8 = AMD_CAM_LENS_STATE_STATIONARY;

		tag = AMD_CAM_LENS_STATE;
		data = (const void *)&u8;
		if (amd_cam_update_tag(md, tag, data, 1, NULL) != OK)
			ISP_PR_WARN("update lens state fail!");
	}

	//android.control.sceneMode 1 u8
	ISP_PR_DBG("scene mode rst %d", mi->sceneMode.mode);
	if (mi->sceneMode.mode != f->sceneMode.mode) {
		if (mi->sceneMode.mode == SCENE_MODE_DEFAULT)
			u8 = AMD_CAM_CONTROL_SCENE_MODE_DISABLED;
		else if (mi->sceneMode.mode == SCENE_MODE_ACTION)
			u8 = AMD_CAM_CONTROL_SCENE_MODE_ACTION;
		else if (mi->sceneMode.mode == SCENE_MODE_PORTRAIT)
			u8 = AMD_CAM_CONTROL_SCENE_MODE_PORTRAIT;
		else if (mi->sceneMode.mode == SCENE_MODE_LANDSCAPE)
			u8 = AMD_CAM_CONTROL_SCENE_MODE_LANDSCAPE;
		else if (mi->sceneMode.mode == SCENE_MODE_NIGHT)
			u8 = AMD_CAM_CONTROL_SCENE_MODE_NIGHT;
		else if (mi->sceneMode.mode == SCENE_MODE_NIGHT_PORTRAIT)
			u8 = AMD_CAM_CONTROL_SCENE_MODE_NIGHT_PORTRAIT;
		else if (mi->sceneMode.mode == SCENE_MODE_THEATRE)
			u8 = AMD_CAM_CONTROL_SCENE_MODE_THEATRE;
		else if (mi->sceneMode.mode == SCENE_MODE_BEACH)
			u8 = AMD_CAM_CONTROL_SCENE_MODE_BEACH;
		else if (mi->sceneMode.mode == SCENE_MODE_SNOW)
			u8 = AMD_CAM_CONTROL_SCENE_MODE_SNOW;
		else if (mi->sceneMode.mode == SCENE_MODE_SUNSET)
			u8 = AMD_CAM_CONTROL_SCENE_MODE_SUNSET;
		else if (mi->sceneMode.mode == SCENE_MODE_STEADYPHOTO)
			u8 = AMD_CAM_CONTROL_SCENE_MODE_STEADYPHOTO;
		else if (mi->sceneMode.mode == SCENE_MODE_FIREWORKS)
			u8 = AMD_CAM_CONTROL_SCENE_MODE_FIREWORKS;
		else if (mi->sceneMode.mode == SCENE_MODE_SPORTS)
			u8 = AMD_CAM_CONTROL_SCENE_MODE_SPORTS;
		else if (mi->sceneMode.mode == SCENE_MODE_PARTY)
			u8 = AMD_CAM_CONTROL_SCENE_MODE_PARTY;
		else if (mi->sceneMode.mode == SCENE_MODE_CANDLELIGHT)
			u8 = AMD_CAM_CONTROL_SCENE_MODE_CANDLELIGHT;
		else if (mi->sceneMode.mode == SCENE_MODE_BARCODE)
			u8 = AMD_CAM_CONTROL_SCENE_MODE_BARCODE;
		else if (mi->sceneMode.mode == SCENE_MODE_HIGH_SPEED_VIDEO)
			u8 = AMD_CAM_CONTROL_SCENE_MODE_HIGH_SPEED_VIDEO;
		else if (mi->sceneMode.mode == SCENE_MODE_HDR)
			u8 = AMD_CAM_CONTROL_SCENE_MODE_HDR;
		else if (mi->sceneMode.mode == SCENE_MODE_INVALID)
			u8 = AMD_CAM_CONTROL_SCENE_MODE_DISABLED;

		tag = AMD_CAM_CONTROL_SCENE_MODE;
		data = (const void *)&u8;
		if (amd_cam_update_tag(md, tag, data, 1, NULL) != OK)
			ISP_PR_WARN("update scene mode fail!");
	}

	//BLC
	//android.blackLevel.lock 1 u8
	ISP_PR_DBG("black level lock rst %d", mi->blc.lockState);
	if (mi->blc.lockState != f->blc.lockState) {
		if (mi->blc.lockState == BLC_LOCK_STATE_LOCKED)
			u8 = AMD_CAM_BLACK_LEVEL_LOCK_ON;
		else
			u8 = AMD_CAM_BLACK_LEVEL_LOCK_OFF;

		tag = AMD_CAM_BLACK_LEVEL_LOCK;
		data = (const void *)&u8;
		if (amd_cam_update_tag(md, tag, data, 1, NULL) != OK)
			ISP_PR_WARN("update black level lock fail!");
	}

	//android.sensor.dynamicBlackLevel 4 i32
	for (i = 0; i < BLACK_LEVEL_CNT; i++) {
		i32[i] = (s32)mi->blc.blackLevel.blsValue[i];
		ISP_PR_DBG("dynamic black level rst[%d]%d", i,
			   mi->blc.blackLevel.blsValue[i]);
	}
	tag = AMD_CAM_SENSOR_DYNAMIC_BLACK_LEVEL;
	data = (const void *)i32;
	if (amd_cam_update_tag(md, tag, data, BLACK_LEVEL_CNT, NULL) != OK)
		ISP_PR_WARN("update dynamic black level fail!");

	//android.sensor.dynamicWhiteLevel 1 i32
	i32[0] = (s32)(1023 - mi->blc.blackLevel.blsValue[0]);
	ISP_PR_DBG("dynamic white level rst %d", i32[0]);
	tag = AMD_CAM_SENSOR_DYNAMIC_WHITE_LEVEL;
	data = (const void *)i32;
	if (amd_cam_update_tag(md, tag, data, 1, NULL) != OK)
		ISP_PR_WARN("update dynamic white level fail!");

	//DPC
	//android.hotPixel.mode 1 u8
	ISP_PR_DBG("hot pixel mode rst %d", mi->dpc.enabled);
	if (mi->dpc.enabled != f->dpc.enabled) {
		if (mi->dpc.enabled)
			u8 = AMD_CAM_HOT_PIXEL_MODE_FAST;
		else
			u8 = AMD_CAM_HOT_PIXEL_MODE_OFF;

		tag = AMD_CAM_HOT_PIXEL_MODE;
		data = (const void *)&u8;
		if (amd_cam_update_tag(md, tag, data, 1, NULL) != OK)
			ISP_PR_WARN("update hot pixel fail!");
	}

	//LSC
	//android.shading.mode 1 u8
	ISP_PR_DBG("shading mode rst %d", mi->lsc.enabled);
	if (mi->lsc.enabled != f->lsc.enabled) {
		if (mi->lsc.enabled)
			u8 = AMD_CAM_SHADING_MODE_FAST;
		else
			u8 = AMD_CAM_SHADING_MODE_OFF;

		tag = AMD_CAM_SHADING_MODE;
		data = (const void *)&u8;
		if (amd_cam_update_tag(md, tag, data, 1, NULL) != OK)
			ISP_PR_WARN("update shading mode fail!");
	}

	//CAC
	//android.colorCorrection.aberrationMode 1 u8
	ISP_PR_DBG("cac mode rst %d", mi->cac.enabled);
	if (mi->cac.enabled != f->cac.enabled) {
		if (mi->cac.enabled)
			u8 = AMD_CAM_COLOR_CORRECTION_ABERRATION_MODE_FAST;
		else
			u8 = AMD_CAM_COLOR_CORRECTION_ABERRATION_MODE_OFF;

		tag = AMD_CAM_COLOR_CORRECTION_ABERRATION_MODE;
		data = (const void *)&u8;
		if (amd_cam_update_tag(md, tag, data, 1, NULL) != OK)
			ISP_PR_WARN("update cac mode fail!");
	}

	//SHARPEN
	//android.edge.mode 1 u8
	ISP_PR_DBG("edge mode rst %d", mi->sharpen.enabled);
	if (mi->sharpen.enabled != f->sharpen.enabled) {
		if (mi->sharpen.enabled)
			u8 = AMD_CAM_EDGE_MODE_FAST;
		else
			u8 = AMD_CAM_EDGE_MODE_OFF;

		tag = AMD_CAM_EDGE_MODE;
		data = (const void *)&u8;
		if (amd_cam_update_tag(md, tag, data, 1, NULL) != OK)
			ISP_PR_WARN("update edge mode fail!");
	}

	//android.tonemap.mode 1 u8
	ISP_PR_DBG("tonemap mode rst %d", mi->tonemap.mode);
	if (mi->tonemap.mode != f->tonemap.mode) {
		if (mi->tonemap.mode == TONEMAP_TYPE_CONTRAST_CURVE)
			u8 = AMD_CAM_TONEMAP_MODE_CONTRAST_CURVE;
		else if (mi->tonemap.mode == TONEMAP_TYPE_PRESET_CURVE)
			u8 = AMD_CAM_TONEMAP_MODE_PRESET_CURVE;
		else if (mi->tonemap.mode == TONEMAP_TYPE_GAMMA)
			u8 = AMD_CAM_TONEMAP_MODE_GAMMA_VALUE;
		else
			u8 = AMD_CAM_TONEMAP_MODE_FAST;

		tag = AMD_CAM_TONEMAP_MODE;
		data = (const void *)&u8;
		if (amd_cam_update_tag(md, tag, data, 1, NULL) != OK)
			ISP_PR_WARN("update tonemap mode fail!");
	}

	//android.tonemap.curveGreen 129 i32
	if (mi->tonemap.mode == TONEMAP_TYPE_CONTRAST_CURVE) {
		cnt = CURVE_POINTS_NUM;
		for (i = 0; i < cnt; i++) {
			i32[i] = (s32)mi->tonemap.curveGreen[i];
#ifdef ADVANCED_DEBUG
			ISP_PR_DBG("curve green rst (%d, %d)", i,
				   mi->tonemap.curveGreen[i]);
#endif
		}

		data = (const void *)i32;
		tag = AMD_CAM_TONEMAP_CURVE_GREEN;
		if (amd_cam_update_tag(md, tag, data, cnt, NULL) != OK)
			ISP_PR_WARN("update tonemap curve green fail!");
	}

	//android.tonemap.presetCurve 1 u8
	ISP_PR_DBG("tonemap preset curve rst %d", mi->tonemap.presetCurve);
	if (mi->tonemap.mode == TONEMAP_TYPE_PRESET_CURVE &&
	    mi->tonemap.presetCurve != f->tonemap.presetCurve) {
		if (mi->tonemap.presetCurve == TONEMAP_PRESET_CURVE_TYPE_REC709)
			u8 = AMD_CAM_TONEMAP_PRESET_CURVE_REC709;
		else
			u8 = AMD_CAM_TONEMAP_PRESET_CURVE_SRGB;

		tag = AMD_CAM_TONEMAP_PRESET_CURVE;
		data = (const void *)&u8;
		if (amd_cam_update_tag(md, tag, data, 1, NULL) != OK)
			ISP_PR_WARN("update tonemap preset curve fail!");
	}

	//android.tonemap.gamma 1 f
#ifdef PRINT_FLOAT_INFO
	kernel_fpu_begin();
	ISP_PR_INFO("gamma rst (x1000)%d", (s32)(mi->tonemap.gamma * 1000));
	kernel_fpu_end();
#endif
	if (mi->tonemap.mode == TONEMAP_TYPE_GAMMA &&
	    memcmp(&mi->tonemap.gamma, &f->tonemap.gamma, sizeof(float))) {
		tag = AMD_CAM_TONEMAP_GAMMA;
		data = (const void *)&mi->tonemap.gamma;
		if (amd_cam_update_tag(md, tag, data, 1, NULL) != OK)
			ISP_PR_WARN("update tonemap gamma fail!");
	}

	//NR
	//android.noiseReduction.mode 1 u8
	ISP_PR_DBG("nr rst %d", mi->nr.enabled);
	if (mi->nr.enabled != f->nr.enabled) {
		if (!mi->nr.enabled)
			u8 = AMD_CAM_NOISE_REDUCTION_MODE_OFF;
		else
			u8 = AMD_CAM_NOISE_REDUCTION_MODE_FAST;

		tag = AMD_CAM_NOISE_REDUCTION_MODE;
		data = (const void *)&u8;
		if (amd_cam_update_tag(md, tag, data, 1, NULL) != OK)
			ISP_PR_WARN("update nr fail!");
	}

	//android.noiseReduction.strength 1 u8
	ISP_PR_DBG("nr strength rst %d", mi->nr.strength);
	if (mi->nr.enabled && mi->nr.strength != f->nr.strength) {
		u8 = mi->nr.strength;
		tag = AMD_CAM_NOISE_REDUCTION_STRENGTH;
		data = (const void *)&u8;
		if (amd_cam_update_tag(md, tag, data, 1, NULL) != OK)
			ISP_PR_WARN("update nr strength fail!");
	}

	//android.scaler.cropRegion 4 i32
	ISP_PR_DBG("scaler cropRegion rst (%d, %d, %d, %d)",
		   mi->zoomWindow.h_offset, mi->zoomWindow.v_offset,
		   mi->zoomWindow.h_size, mi->zoomWindow.v_size);
	ret = memcmp(&mi->zoomWindow, &f->crop.window,
		     sizeof(struct _Window_t));
	if (ret != 0) {
		i32[0] = (s32)mi->zoomWindow.h_offset;
		i32[1] = (s32)mi->zoomWindow.v_offset;
		i32[2] = (s32)mi->zoomWindow.h_size + i32[0];
		i32[3] = (s32)mi->zoomWindow.v_size + i32[1];
		/*
		 *There is no logic in HAL layer to handle ROI for different
		 *sensor profiles, so the ROI from HAL is always based on the
		 *full resolution, no matter what the sensor profile is, full,
		 *2x2 binning, etc.
		 */
		if (is_2x2_binning_prf_mode(cam->fc->prf_id)) {
			ISP_PR_DBG("2x2 binning mode");
			i32[0] *= 2;
			i32[1] *= 2;
			i32[2] *= 2;
			i32[3] *= 2;
		} else if (is_v2_binning_prf_mode(cam->fc->prf_id)) {
			ISP_PR_DBG("v2 binning mode");
			i32[1] *= 2;
			i32[3] *= 2;
		} else {
			ISP_PR_DBG("none binning mode");
		}

		ISP_PR_DBG("scaler cropRegion rst to HAL (%d, %d, %d, %d)",
			   i32[0], i32[1], i32[2], i32[3]);
		tag = AMD_CAM_SCALER_CROP_REGION;
		data = (const void *)i32;
		if (amd_cam_update_tag(md, tag, data, 4, NULL) != OK)
			ISP_PR_WARN("update scaler cropRegion fail!");
	}

	//vendor tag color effect mode 1 u8
	ISP_PR_DBG("effect mode en:%d rst:%d", mi->ie.enabled, mi->ie.mode);
	if (mi->ie.enabled != f->ie.enabled || mi->ie.mode != f->ie.mode) {
		if (!mi->ie.enabled)
			u8 = AMD_CAM_VENDOR_CONTROL_EFFECT_MODE_OFF;
		else if (mi->ie.mode == IE_MODE_GRAYSCALE)
			u8 = AMD_CAM_VENDOR_CONTROL_EFFECT_MODE_GRAYSCALE;
		else if (mi->ie.mode == IE_MODE_NEGATIVE)
			u8 = AMD_CAM_VENDOR_CONTROL_EFFECT_MODE_NEGATIVE;
		else if (mi->ie.mode == IE_MODE_SEPIA)
			u8 = AMD_CAM_VENDOR_CONTROL_EFFECT_MODE_SEPIA;
		else if (mi->ie.mode == IE_MODE_COLOR_SELECTION)
			u8 = AMD_CAM_VENDOR_CONTROL_EFFECT_MODE_COLOR_SELECTION;
		else if (mi->ie.mode == IE_MODE_EMBOSS)
			u8 = AMD_CAM_VENDOR_CONTROL_EFFECT_MODE_EMBOSS;
		else if (mi->ie.mode == IE_MODE_SKETCH)
			u8 = AMD_CAM_VENDOR_CONTROL_EFFECT_MODE_SKETCH;
		else //IE_MODE_SHARPEN
			u8 = AMD_CAM_VENDOR_CONTROL_EFFECT_MODE_SHARPEN;

		tag = AMD_CAM_VENDOR_CONTROL_EFFECT_MODE;
		data = (const void *)&u8;
		if (amd_cam_update_tag(md, tag, data, 1, NULL) != OK)
			ISP_PR_WARN("update effect mode fail!");
	}

	//statistics
	//android.statistics.histogram 3n i32
	if (mi->statistics.mode.histogramEnabled) {
		cnt = HISTOGRAM_DATA_CNT;
		p_s32 = g_s32;
		for (i = 0; i < cnt; i++, p_s32++) {
			*p_s32 = (s32)mi->statistics.histogram.aeHistogram[i];
			ISP_PR_VERB("histogram rst[%d] = %d", i, *p_s32);
		}

		tag = AMD_CAM_STATISTICS_HISTOGRAM;
		data = (const void *)g_s32;
		if (amd_cam_update_tag(md, tag, data, cnt, NULL) != OK)
			ISP_PR_WARN("update histogram fail!");
	}

	//android.statistics.sharpnessMap n*m*3 i32
	if (mi->statistics.mode.afEnabled) {
		cnt = SHARP_MAP_CNT;
		p_s32 = g_s32;
		for (i = 0; i < cnt; i++, p_s32++) {
			*p_s32 = (s32)mi->statistics.af.metering[i];
			ISP_PR_VERB("sharpness rst map[%d] = %d", i, *p_s32);
		}

		tag = AMD_CAM_STATISTICS_SHARPNESS_MAP;
		data = (const void *)g_s32;
		if (amd_cam_update_tag(md, tag, data, cnt, NULL) != OK)
			ISP_PR_WARN("update sharpness map fail!");
	}

	//android.statistics.lensShadingMap n*m*4 i32
	if (mi->statistics.mode.lensShadingMapEnabled) {
		cnt = LSC_MAP_CNT;
		if (!mi->lsc.enabled) {
			ISP_PR_DBG("set lsc rst to default when lsc mode off");
			p_s32 = g_s32;
			for (i = 0; i < cnt * 4; i++, p_s32++)
				*p_s32 = LENS_SHADING_MAP_DEFAULT;
		} else {
			p_r = g_s32;
			p_gr = p_r + cnt;
			p_gb = p_gr + cnt;
			p_b = p_gb + cnt;
			for (i = 0; i < cnt; i++) {
				*(p_r++) = (s32)
					mi->statistics.lscMap.dataR[i];
				*(p_gr++) = (s32)
					mi->statistics.lscMap.dataGR[i];
				*(p_gb++) = (s32)
					mi->statistics.lscMap.dataGB[i];
				*(p_b++) = (s32)
					mi->statistics.lscMap.dataB[i];

				ISP_PR_VERB("lsc rst map[%d] = %d", i,
					    *(p_r - 1));
				ISP_PR_VERB("lsc rst map[%d] = %d", i + cnt,
					    *(p_gr - 1));
				ISP_PR_VERB("lsc rst map[%d] = %d", i + cnt * 2,
					    *(p_gb - 1));
				ISP_PR_VERB("lsc rst map[%d] = %d", i + cnt * 3,
					    *(p_b - 1));
			}
		}

		tag = AMD_CAM_STATISTICS_LENS_SHADING_MAP;
		data = (const void *)g_s32;
		if (amd_cam_update_tag(md, tag, data, cnt * 4, NULL) != OK)
			ISP_PR_WARN("update lsc map fail!");
	}

	//com.amd.control.raw_frame_id 1 i32
	tag = AMD_CAM_VENDOR_CONTROL_RAW_FRAME_ID;
	i32[0] = mi->poc;
	ISP_PR_DBG("raw frame id %d", i32[0]);
	data = (const void *)i32;
	if (amd_cam_update_tag(md, tag, data, 1, NULL) != OK)
		ISP_PR_WARN("update raw frame id fail!");

	if (f->engineerMode.enabled) {
		ISP_PR_DBG("result engineer mode is %d, lo %u, hi %u, len %d",
			  mi->engineerMode.enabled, mi->engineerMode.bufBaseLo,
			  mi->engineerMode.bufBaseHi, mi->engineerMode.bufSize);
		if (cam->fc->eng && cam->fc->eng_ptr) {
			result = copy_to_user(cam->fc->eng_ptr, cam->fc->eng,
					      sizeof(*cam->fc->eng));
			if (result != OK) {
				ISP_PR_ERR("copy eng buf to HAL buf %p fail!",
					  cam->fc->eng_ptr);
			} else {
				ISP_PR_DBG("copy eng buf to HAL buf %p done",
					  cam->fc->eng_ptr);
			}
		}
	}

	//com.amd.control.cvip_af_frame_id 1 i32 (only for CVIP)
	if (g_prop->cvip_enable == CVIP_STATUS_ENABLE) {
		tag = AMD_CAM_VENDOR_CONTROL_CVIP_AF_FRAME_ID;
		i32[0] = mi->preData.afMetaData.afFrameId;
		ISP_PR_DBG("cvip af frame id %d", i32[0]);
		data = (const void *)i32;
		if (amd_cam_update_tag(md, tag, data, 1, NULL) != OK)
			ISP_PR_WARN("update cvip af frame id fail!");
	}

	result = copy_to_user(hal_md, md, MAX_KERN_METADATA_BUF_SIZE);
	if (result != OK)
		ISP_PR_ERR("copy the metadata to HAL %p fail!", hal_md);

	return result;
}

int store_latest_md(struct physical_cam *cam, struct amd_cam_metadata *src)
{
	int ret = OK;
	struct metadata_entry *m = NULL;
	struct metadata_entry *dst = NULL;

	ISP_PR_DBG("Entry! cam[%d]", cam->idx);
	if (list_empty(&cam->md_list)) {
		dst = &cam->ping;
		ret = copy_from_user(dst->md, src, MAX_KERN_METADATA_BUF_SIZE);
	} else if (!md_list_has_2_entry(cam)) {
		dst = &cam->pong;
		ret = copy_from_user(dst->md, src, MAX_KERN_METADATA_BUF_SIZE);
	} else {
		m = list_entry(cam->md_list.prev, struct metadata_entry, entry);
		ret = copy_from_user(m->md, src, MAX_KERN_METADATA_BUF_SIZE);
		if (ret == OK)
			list_swap(&cam->ping.entry, &cam->pong.entry);
	}

	if (dst && ret == OK)
		list_add(&dst->entry, &cam->md_list);
	if (ret != OK)
		ISP_PR_ERR("cam[%d] copy_from_user fail!", cam->idx);

	return ret;
}

void check_buf_status(struct _BufferMetaInfo_t *src, struct kernel_buffer *dst,
		      int cid, int sid)
{
	if (!src->enabled) {
		ISP_PR_ERR("cam[%d]s[%d] buf is disable!", cid, sid);
		dst->status = BUFFER_STATUS_ERROR;
	} else if (src->status != BUFFER_STATUS_DONE) {
		ISP_PR_ERR("cam[%d]s[%d] buf status %d", cid, sid, src->status);
		dst->status = BUFFER_STATUS_ERROR;
	} else {
		ISP_PR_DBG("cam[%d]s[%d] buf status is ok", cid, sid);
	}
}

int sensor_close(struct physical_cam *cam)
{
	int ret = OK;
	int timeout = 0;
	struct isp_ctl_ctx ctx;
	unsigned long flags = 0;

	ISP_PR_PC("Entry! cam[%d]", cam->idx);
	ctx.cam = cam;
	ctx.stream = NULL;
	spin_lock_irqsave(&cam->b_lock, flags);
	if (cam->tear_down && !list_empty(&cam->enq_list)) {
		spin_unlock_irqrestore(&cam->b_lock, flags);
		ISP_PR_INFO("cam[%d] release all rst before close!", cam->idx);
		timeout = wait_event_interruptible_timeout(cam->done_wq,
							   !cam->tear_down,
							   TEAR_DOWN_TIMEOUT);
		if (timeout <= 0) {
			ISP_PR_ERR("cam[%d] wait timeout or -ERESTARTSYS %d",
				   cam->idx, timeout);
			ret = (timeout == 0) ? -ETIMEDOUT : -EINTR;
			goto quit;
		} else {
			ISP_PR_INFO("cam[%d] wait tear down done, close now",
				    cam->idx);
		}
	} else {
		spin_unlock_irqrestore(&cam->b_lock, flags);
	}

	ctx.ctx = NULL;
	ret = v4l2_subdev_call(cam->isp_sd, core, ioctl, ISP_IOCTL_STREAM_CLOSE,
			       &ctx);
	if (ret != OK) {
		ISP_PR_ERR("cam[%d] fail! ret:%d", cam->idx, ret);
	} else {
		ISP_PR_PC("cam[%d] suc", cam->idx);
		cam->cnt_enq = 0;
		cam->cam_on = false;
		cam->clk_tag = NO_NEED_LOWER_CLK;
		cam->tear_down = false;
		memset(cam->last_stream_info, 0, sizeof(cam->last_stream_info));
		memset(cam->ping.md, 0, MAX_KERN_METADATA_BUF_SIZE);
		memset(cam->pong.md, 0, MAX_KERN_METADATA_BUF_SIZE);
		memset(cam->md, 0, MAX_KERN_METADATA_BUF_SIZE);
	}
	ret = vb2_streamoff(&cam->vidq, cam->vidq.type);
quit:
	RET(ret);
	return ret;
}

/*
 * file operation for each physical camera
 */
static int amd_cam_open(struct file *f)
{
	int ret = OK;
	int cam_id;
	struct physical_cam *cam = NULL;
	struct video_device *dev = video_devdata(f);

	ENTER();

	cam_id = find_first_zero_bit(&g_cam->cam_bit, g_cam->cam_num);
	if (cam_id >= g_cam->cam_num) {
		cam_id = 0;
		g_cam->cam_bit = 0;
		ISP_PR_DBG("already open all cams, re-open from: %d", cam_id);
	} else {
		ISP_PR_DBG("gcam->cam_bit: 0x%lx, camId: %d", g_cam->cam_bit,
			   cam_id);
	}

	g_cam->cam_bit |= 1 << cam_id;
	ISP_PR_DBG("update gcam->cam_bit: 0x%lx", g_cam->cam_bit);

	cam = g_cam->phycam_tbl[cam_id];
	if (!cam) {
		ISP_PR_ERR("cam[%d] is null!", cam_id);
		goto error;
	}

	ret = init_frame_control(cam);
	if (ret != OK)
		goto error;

	ret = alloc_vb2_buf_structure(cam, MAX_REQUEST_DEPTH);
	if (ret != OK) {
		ISP_PR_ERR("alloc vb2Buf fail! ret: %d", ret);
		goto error;
	}

	v4l2_fh_init(&cam->fh, dev);
	f->private_data = &cam->fh;
	v4l2_fh_add(&cam->fh);
	goto quit;
error:
	physical_cam_destroy(cam);
quit:
	RET(ret);
	return ret;
}

static int amd_cam_release(struct file *f)
{
	int ret = OK;
	struct v4l2_fh *fh = f->private_data;
	struct physical_cam *cam = container_of(fh, struct physical_cam, fh);

	ISP_PR_DBG("Entry! cam[%d]", cam->idx);
	ret = sensor_close(cam);
	if (ret != OK)
		ISP_PR_ERR("vb2 fail! ret:%d", ret);

	ret = alloc_vb2_buf_structure(cam, 0);
	v4l2_fh_del(&cam->fh);
	v4l2_fh_exit(&cam->fh);
	g_cam->cam_bit = 0;
	ISP_PR_INFO("close cam[%d] ret:%d", cam->idx, ret);

	return ret;
}

static const struct v4l2_ioctl_ops amd_cam_ioctl_ops = {
	.vidioc_querycap = NULL,
	.vidioc_enum_fmt_vid_cap = NULL,
	.vidioc_g_fmt_vid_cap = NULL,
	.vidioc_s_fmt_vid_cap = NULL,
	.vidioc_s_parm = NULL,
	.vidioc_reqbufs = NULL,
	.vidioc_qbuf = NULL,
	.vidioc_dqbuf = NULL,
	.vidioc_streamon = NULL,
	.vidioc_streamoff = NULL,
	.vidioc_queryctrl = NULL,
	.vidioc_s_ctrl = NULL,
	.vidioc_enum_input = NULL,
	.vidioc_g_ctrl = NULL,
	.vidioc_g_input = NULL,
	.vidioc_s_input = NULL,
	.vidioc_g_ext_ctrls = NULL,
	.vidioc_s_ext_ctrls = NULL,
	.vidioc_default = _ioctl_default,
};

static const struct v4l2_file_operations amd_cam_fops = {
	.owner = THIS_MODULE,
	.open = amd_cam_open,
	.release = amd_cam_release,
	.unlocked_ioctl = video_ioctl2,
#ifdef CONFIG_COMPAT
	.compat_ioctl32 = video_ioctl2,
#endif
	.mmap = NULL,
	.poll = NULL,
};

static struct video_device amd_camera_template = {
	.name = "amd_cam",
	.fops = &amd_cam_fops,
	.ioctl_ops = &amd_cam_ioctl_ops,
	.release = video_device_release,
};

struct physical_cam *physical_cam_create(int idx, struct v4l2_device *vdev)
{
	struct physical_cam *cam;

	ENTER();
	cam = kzalloc(sizeof(*cam), GFP_KERNEL);
	if (!cam) {
		ISP_PR_ERR("alloc cam fail!");
		goto quit;
	}

	memset(cam, 0, sizeof(*cam));
#ifdef USING_DMA_BUF
	cam->buf_type = STREAM_BUF_DMA;
#else
	cam->buf_type = STREAM_BUF_VMALLOC;
#endif
	/* get physical camera info from hw detection table */
	if (idx == 0) {
		cam->idx = CAMERA_ID_REAR;
	} else if (idx == 1) {
		cam->idx = CAMERA_ID_FRONT_LEFT;
	} else if (idx == 2) {
		cam->idx = CAMERA_ID_FRONT_RIGHT;
	} else {
		ISP_PR_ERR("Unsupported! idx");
	}

	cam->isp_sd = get_isp_subdev();
	cam->cam_init = false;
	cam->cam_on = false;
	cam->clk_tag = NO_NEED_LOWER_CLK;
	cam->tear_down = false;
	init_waitqueue_head(&cam->done_wq);

	if (init_video_dev(cam, vdev, idx) != OK || init_vb2q(cam) != OK)
		physical_cam_destroy(cam);
quit:
	return cam;
}

int init_frame_control(struct physical_cam *cam)
{
	int ret = OK;

	ISP_PR_DBG("Entry! cam[%d]", cam->idx);
	cam->fc_id = 0;
	cam->fc_id_offset = 0;
	cam->cnt_enq = 0;
	cam->req = NULL;
	cam->fc = NULL;
	cam->pre_pfc = NULL;
	cam->md = NULL;
	INIT_LIST_HEAD(&cam->enq_list);
	spin_lock_init(&cam->b_lock);
	INIT_LIST_HEAD(&cam->md_list);
	INIT_LIST_HEAD(&cam->ping.entry);
	INIT_LIST_HEAD(&cam->pong.entry);
	memset(cam->last_stream_info, 0, sizeof(cam->last_stream_info));
	if (!cam->ping.md)
		cam->ping.md = kzalloc(MAX_KERN_METADATA_BUF_SIZE, GFP_KERNEL);
	if (!cam->ping.md)
		goto error;

	if (!cam->pong.md)
		cam->pong.md = kzalloc(MAX_KERN_METADATA_BUF_SIZE, GFP_KERNEL);
	if (!cam->pong.md)
		goto error;

	if (!cam->md)
		cam->md = kzalloc(MAX_KERN_METADATA_BUF_SIZE, GFP_KERNEL);
	if (!cam->md)
		goto error;

	memset(cam->ping.md, 0, MAX_KERN_METADATA_BUF_SIZE);
	memset(cam->pong.md, 0, MAX_KERN_METADATA_BUF_SIZE);
	memset(cam->md, 0, MAX_KERN_METADATA_BUF_SIZE);
	goto quit;
error:
	ISP_PR_ERR("cam[%d] create md ping/pong fail!", cam->idx);
	ret = -ENOMEM;
quit:
	return ret;
}

int init_video_dev(struct physical_cam *cam, struct v4l2_device *vdev, int id)
{
	int ret = OK;
	struct video_device *vd;

	ISP_PR_DBG("Entry! cam[%d]", cam->idx);
	vd = video_device_alloc();
	if (!vd) {
		ISP_PR_ERR("alloc video_device fail!");
		ret = -ENOMEM;
		goto quit;
	}

	cam->dev = vd;
	*vd = amd_camera_template;
	vd->lock = NULL;
	vd->v4l2_dev = vdev;
	vd->index = id;
	vd->device_caps = V4L2_CAP_VIDEO_CAPTURE | V4L2_CAP_READWRITE |
			V4L2_CAP_STREAMING;
	video_set_drvdata(vd, cam);

	ret = video_register_device(vd, VFL_TYPE_GRABBER, VIDEO_DEV_BASE + id);
	if (ret != OK)
		ISP_PR_ERR("cam[%d] register device fail!", cam->idx);
quit:
	return ret;
}

int init_vb2q(struct physical_cam *cam)
{
	int ret = OK;
	struct vb2_queue *q = &cam->vidq;

	ISP_PR_DBG("Entry! cam[%d]", cam->idx);
	memset(q, 0, sizeof(*q));
	q->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	q->io_modes = VB2_MMAP | VB2_USERPTR | VB2_READ | VB2_DMABUF;
	q->timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_MONOTONIC;
	q->buf_struct_size = sizeof(struct frame_control);
	q->drv_priv = cam;
	q->ops = &amd_qops;
	if (cam->buf_type == STREAM_BUF_VMALLOC)
		q->mem_ops = &vb2_vmalloc_memops;
	else
		q->mem_ops = &amd_dmabuf_ops;
	q->lock = &cam->c_lock;
	mutex_init(q->lock);

	ret = vb2_queue_init(q);
	if (ret != OK) {
		//add this line to trick CP
		ISP_PR_ERR("cam[%d] init vidq fail! ret: %d", cam->idx, ret);
	} else {
		//add this line to trick CP
		spin_lock_init(&cam->q_lock);
	}

	return ret;
}

int alloc_vb2_buf_structure(struct physical_cam *cam, int count)
{
	int ret = OK;
	struct v4l2_requestbuffers req;

	ISP_PR_DBG("Entry! cam[%d], cnt %d", cam->idx, count);
	req.count = count;
	req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory = V4L2_MEMORY_USERPTR;
	ret = vb2_reqbufs(&cam->vidq, &req);

	RET(ret);
	return ret;
}

void physical_cam_destroy(struct physical_cam *cam)
{
	ISP_PR_DBG("Entry! cam[%d]", cam->idx);
	if (cam == NULL)
		return;

	cam->isp_sd = NULL;
	cam->req = NULL;
	cam->fc = NULL;
	cam->pre_pfc = NULL;
	vb2_queue_release(&cam->vidq);
	destroy_all_md(cam);
	if (cam->dev) {
		video_unregister_device(cam->dev);
		video_device_release(cam->dev);
	}

	kfree(cam);
	cam = NULL;

	EXIT();
}

void destroy_all_md(struct physical_cam *cam)
{
	ISP_PR_DBG("Entry! cam[%d]", cam->idx);
	kfree(cam->ping.md);
	kfree(cam->pong.md);
	kfree(cam->md);
}

/*
 * Hardware detection
 */
static int detect_hardware(struct amd_cam *g_cam)
{
	int ret = OK;
	struct cam_hw_info *hw;

	ENTER();

	hw = kzalloc(sizeof(*hw) * MAX_HW_NUM, GFP_KERNEL);
	if (!hw) {
		ret = -ENOMEM;
		goto quit;
	}

	/*
	 * Currently hard code here
	 */
	hw[0].id = 0;
	hw[0].type = ISP;
	/*
	 * add priv info here
	 */
	g_cam->hw_tbl[0] = &hw[0];

	hw[1].id = 1;
	hw[1].type = SENSOR;
	g_cam->hw_tbl[1] = &hw[1];
quit:
	RET(ret);
	return ret;
}

static int register_hardware(struct cam_hw_info *hw)
{
	int ret = OK;
	struct v4l2_subdev *sd = &hw->sd;

	ENTER();

	assert(sd != NULL && hw != NULL);

	if (hw->type == ISP)
		ret = register_isp_subdev(sd, &g_cam->v4l2_dev);

	/*
	 * add more hardware here
	 */

	RET(ret);
	return ret;
}

static struct v4l2_subdev *get_isp_subdev(void)
{
	/*
	 * it is only quirk
	 */
	return &g_cam->hw_tbl[0]->sd;
}

/* file operation */
static int isp_node_open(struct file __maybe_unused *file)
{
	//struct video_device *vdev = video_devdata(file);

	ENTER();

	//v4l2_fh_init(&g_cam->fh, vdev);
	//file->private_data = &g_cam->fh;
	//v4l2_fh_add(&g_cam->fh);

	RET(0);
	return 0;
}

static int isp_node_release(struct file __maybe_unused *file)
{
	//struct v4l2_fh *fh = file->private_data;
	//struct amd_cam *gcam = container_of(fh, struct amd_cam, fh);

	ENTER();

	//v4l2_fh_del(&gcam->fh);
	//v4l2_fh_exit(&gcam->fh);

	RET(0);
	return 0;
}

static const struct v4l2_file_operations isp_fops = {
	.owner = THIS_MODULE,
	.open = isp_node_open,
	.release = isp_node_release,
	.unlocked_ioctl = video_ioctl2,
#ifdef CONFIG_COMPAT
	.compat_ioctl32 = video_ioctl2,
#endif
	.mmap = NULL,
	.poll = NULL,
};

long _isp_ioctl_default(struct file *file, void *fh,
		    bool valid_prio, unsigned int cmd, void *arg)
{
	int ret = OK;
	struct kernel_hotplug_info *hot_plug_info;
	struct sensor_indexes *ids;
	struct s_info *info;

	ENTER();
	switch (cmd) {
	case VIDIOC_HOTPLUG: {
		hot_plug_info = (struct kernel_hotplug_info *)arg;
		ISP_PR_DBG("VIDIOC_HOTPLUG");
		ret = hot_plug_event(hot_plug_info);
		break;
	}
	case VIDIOC_GET_SENSOR_LIST: {
		ids = (struct sensor_indexes *)arg;
		ISP_PR_DBG("VIDIOC_GET_SENSOR_LIST");
		/* add real sensor id here */
		ids->sensor_cnt = 1;
		ids->sensor_ids[0] = 0;
		break;
	}
	case VIDIOC_GET_SENSOR_INFO: {
		info = (struct s_info *)arg;
		ISP_PR_DBG("VIDIOC_GET_SENSOR_INFO");
		ret = get_sensor_info(info);
		break;
	}
	case VIDIOC_SET_CLK_GATING: {
		unsigned char *enable;

		enable = (unsigned char *)arg;
		ISP_PR_DBG("VIDIOC_SET_CLK_GATING, enable:%d", *enable);
		ret = set_clk_gating(*enable);
		break;
	}
	case VIDIOC_SET_PWR_GATING: {
		unsigned char *enable;

		enable = (unsigned char *)arg;
		ISP_PR_DBG("VIDIOC_SET_PWR_GATING, enable:%d", *enable);
		ret = set_pwr_gating(*enable);
		break;
	}
	case VIDIOC_DUMP_RAW_ENABLE: {
		unsigned char *enable;

		enable = (unsigned char *)arg;
		ISP_PR_DBG("VIDIOC_DUMP_RAW_ENABLE, enable:%d", *enable);
		ret = dump_raw_enable(CAMERA_ID_REAR, *enable);
		break;
	}
	case VIDIOC_GET_VER_INFO: {
		struct version_info *ver;

		ver = (struct version_info *)arg;
		ISP_PR_DBG("VIDIOC_GET_VERSIONS");
		ver->dri_ver = (unsigned int)DRI_VERSION;
		ver->fw_ver = (unsigned int)FW_VERSION;
		break;
	}
	default:
		pr_err("invalid ioctl");
		ret = -EINVAL;
	}

	RET(ret);
	return ret;
}

int hot_plug_event(struct kernel_hotplug_info *hot_plug_info)
{
	int i = 0;
	int ret = OK;
	struct isp_ctl_ctx ctx;
	struct s_info *info = NULL;
	bool plug_state = hot_plug_info->hot_plug_state;
	bool has_otp = false;
	unsigned int otp_data_size = 0;

	ENTER();
	info = hot_plug_info->info;
	if (!kset_ref) {
		kset_ref =
		    kset_create_and_add("hotplug_camera", NULL, kernel_kobj);

		if (!kset_ref)
			goto error_kobject;

		kobj_ref = kzalloc(sizeof(*kobj_ref), GFP_KERNEL);

		if (!kobj_ref) {
			kset_unregister(kset_ref);
			goto error_kobject;
		}

		kobj_ref->kset = kset_ref;

		if (kobject_init_and_add
		    (kobj_ref, &ktype_ref, NULL, "%s", "camState")) {
			kobject_del(kobj_ref);
			kobject_put(kobj_ref);
			kset_unregister(kset_ref);
		}
	}

	if (plug_state && info) {
		ret = copy_from_user(&g_snr_info[0], info, sizeof(*info));
		if (ret != OK) {
			ISP_PR_ERR("copy_from_user fail!");
			goto event;
		}

		otp_data_size = g_snr_info[0].otp_data_size;
		has_otp = otp_data_size > 0 && g_snr_info[0].otp_buffer;

		if (has_otp && otp_data_size != sizeof(g_otp[0])) {
			ISP_PR_ERR
			("CVIP send otp data with incorrect size %d should %d",
			 otp_data_size, sizeof(g_otp[0]));
			has_otp = false;
		}

		if (has_otp) {
			if (copy_from_user(&g_otp[0], g_snr_info[0].otp_buffer,
					   sizeof(g_otp[0])) != OK) {
				ISP_PR_ERR("get otp data fail %d", ret);
				g_snr_info[0].otp_buffer = NULL;
				g_snr_info[0].otp_data_size = 0;
				has_otp = false;
			} else {
				ISP_PR_DBG("get otp data suc");
				g_snr_info[0].otp_buffer = &g_otp[0];
			}
		}

		for (i = 1; i < PHYSICAL_CAMERA_CNT; i++) {
			memcpy(&g_snr_info[i], &g_snr_info[0], sizeof(*info));
			if (has_otp) {
				memcpy(&g_otp[i], &g_otp[0], sizeof(g_otp[0]));
				g_snr_info[i].otp_buffer = &g_otp[i];
			} else {
				g_snr_info[i].otp_buffer = NULL;
				g_snr_info[i].otp_data_size = 0;
			}
		}

		if (!g_cam)
			goto event;

		/* sync sensor_cfg for FW to the s_info from CVIP sensor */
		ctx.stream = NULL;
		ctx.ctx = NULL;
		for (i = 0; i < PHYSICAL_CAMERA_CNT; i++) {
			if (!g_cam->phycam_tbl[i])
				continue;

			ctx.cam = g_cam->phycam_tbl[i];
			ret = v4l2_subdev_call(ctx.cam->isp_sd, core, ioctl,
					       ISP_IOCTL_GET_CAPS, &ctx);
			if (ret != OK)
				ISP_PR_ERR("sync CVIP caps fail %d", ret);
		}
	}
event:
	if (plug_state) {
		kobject_uevent(kobj_ref, KOBJ_ADD);
		ISP_PR_DBG("camera add");
	} else {
		kobject_uevent(kobj_ref, KOBJ_REMOVE);
		ISP_PR_DBG("camera remove");
	}

	RET(ret);
	return ret;

 error_kobject:
	ISP_PR_ERR("Unable to send hotplug event\n");
	return -1;
}

int cvip_power_hot_plug_request(bool hot_plug_state)
{
	struct kernel_hotplug_info hot_plug_info;

	hot_plug_info.hot_plug_state = hot_plug_state;
	hot_plug_info.info = NULL;

	return hot_plug_event(&hot_plug_info);
}
EXPORT_SYMBOL(cvip_power_hot_plug_request);

int get_sensor_info(struct s_info *info)
{
	int i = 0;
	int ret = OK;
	struct isp_ctl_ctx ctx;
	struct s_info *src;
	void *hal_otp_buf;
	bool send_otp = false;

	ISP_PR_INFO("sensor[%s]", g_prop->sensor_id ? "S5K3L6" : "IMX577");
	/* if no sensor info from CVIP, get caps from sensor driver;
	 * if has, update sensor driver caps according to the CVIP info;
	 */
	ctx.stream = NULL;
	ctx.ctx = NULL;
	if (g_cam && g_cam->phycam_tbl[info->sensor_id]) {
		ctx.cam = g_cam->phycam_tbl[info->sensor_id];
		ret = v4l2_subdev_call(ctx.cam->isp_sd, core, ioctl,
				       ISP_IOCTL_GET_CAPS, &ctx);
	} else {
		ISP_PR_ERR("Unable to get physical cam %d", info->sensor_id);
		return -EINVAL;
	}

	src = &g_snr_info[info->sensor_id];
	hal_otp_buf = info->otp_buffer;
	send_otp = hal_otp_buf && src->otp_buffer && src->otp_data_size > 0;
	if (send_otp && info->otp_data_size < src->otp_data_size) {
		ISP_PR_ERR("HAL otp buf size is too small %d, should be %d",
			   info->otp_data_size, src->otp_data_size);
		send_otp = false;
	}

	memcpy(info, src, sizeof(*info));
	info->otp_buffer = hal_otp_buf;
	if (send_otp) {
		if (copy_to_user(hal_otp_buf, &g_otp[info->sensor_id],
				 sizeof(g_otp[info->sensor_id])) != OK) {
			ISP_PR_ERR("copy otp to HAL fail!");
			info->otp_data_size = 0;
		}
	} else {
		info->otp_data_size = 0;
	}

	if (g_prop->cvip_enable == CVIP_STATUS_ENABLE &&
	    g_prop->sensor_id == SENSOR_IMX577) {
		//set this info from S5K3L6 so that sweep can
		//be enabled in CVIP case
		info->hyperfocal_distance = 0.5;
		info->min_focus_distance = 5;
		info->min_increment = 0.125;
		ISP_PR_INFO("change hyperfoc to %u,minfoc to %u,min_inc to %u",
			    (unsigned int)(info->hyperfocal_distance * 1000),
			    (unsigned int)(info->min_focus_distance * 1000),
			    (unsigned int)(info->min_focus_distance * 1000));
	}

	g_min_exp_time = info->modes[0].t_line;
	for (i = 1; i < info->modes_cnt; i++) {
		g_min_exp_time = (g_min_exp_time < info->modes[i].t_line) ?
					g_min_exp_time : info->modes[i].t_line;
	}

	g_min_exp_time *= info->min_expo_line;
	RET(ret);
	return ret;
}

static const struct v4l2_ioctl_ops isp_ioctl_ops = {
	.vidioc_querycap = NULL,
	.vidioc_enum_fmt_vid_cap = NULL,
	.vidioc_g_fmt_vid_cap = NULL,
	.vidioc_s_fmt_vid_cap = NULL,
	.vidioc_s_parm = NULL,
	.vidioc_reqbufs = NULL,
	.vidioc_qbuf = NULL,
	.vidioc_dqbuf = NULL,
	.vidioc_querybuf = NULL,
	.vidioc_expbuf = NULL,
	.vidioc_streamon = NULL,
	.vidioc_streamoff = NULL,
	.vidioc_enum_input = NULL,
	.vidioc_g_input = NULL,
	.vidioc_s_input = NULL,
	.vidioc_queryctrl = NULL,
	.vidioc_query_ext_ctrl = NULL,
	.vidioc_g_ctrl = NULL,
	.vidioc_s_ctrl = NULL,
	.vidioc_g_ext_ctrls = NULL,
	.vidioc_s_ext_ctrls = NULL,
	.vidioc_try_ext_ctrls = NULL,
	.vidioc_querymenu = NULL,
	.vidioc_subscribe_event = NULL,//_ioctl_subscribe_event,
	.vidioc_unsubscribe_event = NULL,//v4l2_event_unsubscribe,
	.vidioc_default = _isp_ioctl_default,
};

int create_isp_node(void)
{
	int ret = OK;
	struct video_device *video_dev = NULL;

	ENTER();
	// video device allocation
	video_dev = video_device_alloc();
	g_cam->dev = video_dev;

	if (!video_dev) {
		ret = -ENOMEM;
		goto error;
	}

	video_dev->lock = NULL;
	video_dev->v4l2_dev = &g_cam->v4l2_dev;
	video_dev->fops = &isp_fops;
	video_dev->ioctl_ops = &isp_ioctl_ops;
	video_dev->release = video_device_release;
	video_dev->device_caps = V4L2_CAP_VIDEO_CAPTURE | V4L2_CAP_READWRITE |
			V4L2_CAP_STREAMING;

	snprintf(video_dev->name, 32, "isp-node");
	// register ISP Node
	ret = video_register_device(video_dev, VFL_TYPE_GRABBER,
				    VIDEO_DEV_BASE - 1);
	if (ret)
		goto error;

	kset_ref = kset_create_and_add("hotplug_camera", NULL, kernel_kobj);
	if (!kset_ref) {
		ISP_PR_ERR("Unable to allocate memory for kset_ref\n");
		goto out;
	}

	kobj_ref = kzalloc(sizeof(*kobj_ref), GFP_KERNEL);
	if (!kobj_ref)
		goto out_kset;

	kobj_ref->kset = kset_ref;
	ret = kobject_init_and_add(kobj_ref, &ktype_ref, NULL, "%s",
				   "camState");
	if (ret != OK) {
		ISP_PR_ERR("Unable to initialize the kobj_ref\n");
		goto out_kobj;
	}

	RET(ret);
	return ret;
 error:
	if (video_dev)
		video_device_release(video_dev);
	return ret;
 out_kobj:
	kobject_put(kobj_ref);
	kobject_del(kobj_ref);

 out_kset:
	kset_unregister(kset_ref);

 out:
	return -1;
}

/*
 * amd capture module
 */
static int amd_capture_probe(struct platform_device *pdev)
{
	int i, ret = OK;
	struct v4l2_subdev *sd;
	struct physical_cam *phycam;
	int physical_camera_cnt;

	ENTER();

	/* collect hardware information */
	if (!g_cam) {
		g_cam = kzalloc(sizeof(*g_cam), GFP_KERNEL);
		if (!g_cam)
			return -ENOMEM;

		memset(g_cam, 0, sizeof(*g_cam));
		detect_hardware(g_cam);
	}

	if (!g_s32) {
		g_s32 = kzalloc(sizeof(*g_s32) * SHARP_MAP_CNT, GFP_KERNEL);
		if (!g_s32) {
			ret = -ENOMEM;
			goto free_dev;
		}

		memset(g_s32, 0, sizeof(*g_s32) * SHARP_MAP_CNT);
	}

	if (!g_prop) {
		g_prop = kzalloc(sizeof(*g_prop), GFP_KERNEL);
		if (!g_prop) {
			ret = -ENOMEM;
			goto free_dev;
		}

		memset(g_prop, 0, sizeof(*g_prop));
		g_prop->drv_log_level = 1;
	}

	for (i = 0; i < CAM_IDX_MAX; i++) {
		g_profile_id[i] = 0;
		memset(&g_snr_info[i], 0, sizeof(g_snr_info[i]));
		memset(&g_otp[i], 0, sizeof(g_otp[i]));
	}

	/* register v4l2 device */
	snprintf(g_cam->v4l2_dev.name, sizeof(g_cam->v4l2_dev.name),
		 "AMD-V4L2-ROOT");
	if (v4l2_device_register(NULL, &g_cam->v4l2_dev)) {
		ISP_PR_ERR("failed to register v4l2 device");
		goto free_dev;
	}

	ISP_PR_PC("AMD ISP v4l2 device registered");
	ISP_PR_PC("%s", DRI_VERSION_STRING);
	ISP_PR_PC("%s", FW_VERSION_STRING);

	for (i = 0; i < MAX_HW_NUM; i++) {
		if (g_cam->hw_tbl[i]) {
			sd = &g_cam->hw_tbl[i]->sd;
			register_hardware(g_cam->hw_tbl[i]);
			/* add error processing here */
		}
	}

	/* get physical cameras count and info from hw detection table */
	physical_camera_cnt = PHYSICAL_CAMERA_CNT;

	for (i = 0; i < physical_camera_cnt; i++) {
		phycam = physical_cam_create(i, &g_cam->v4l2_dev);
		if (!phycam)
			goto clean_physical_cam;
		g_cam->phycam_tbl[i] = phycam;
		g_cam->cam_num++;
	}

	ret = create_isp_node();
	if (ret)
		goto clean_physical_cam;

	/* register all subdev in v4l2_device to nodes if it supports */
	ret = v4l2_device_register_subdev_nodes(&g_cam->v4l2_dev);
	if (ret != 0) {
		ISP_PR_WARN("register subdev as nodes failed (%d)", ret);
		ret = 0;
	}

	RET(ret);
	return 0;

clean_physical_cam:
	for (i = 0; i < physical_camera_cnt; i++)
		physical_cam_destroy(g_cam->phycam_tbl[i]);
	g_cam->cam_num = 0;
	v4l2_device_unregister(&g_cam->v4l2_dev);
	ISP_PR_ERR("failed to create phy cam, destroy and exit\n");
free_dev:
	for (i = 0; i < MAX_HW_NUM; i++) {
		if (!g_cam->hw_tbl[i])
			continue;
		kfree(g_cam->hw_tbl[i]);
		g_cam->hw_tbl[i] = NULL;
	}

	kfree(g_cam);
	kfree(g_s32);
	kfree(g_prop);
	g_prop = NULL;
	g_cam = NULL;
	RET(ret);
	return ret;
}

static int amd_capture_remove(struct platform_device *pdev)
{
	int i = 0;

	ENTER();
	for (i = 0; i < PHYSICAL_CAMERA_CNT; i++)
		physical_cam_destroy(g_cam->phycam_tbl[i]);
	g_cam->cam_num = 0;
	v4l2_device_unregister(&g_cam->v4l2_dev);
	ISP_PR_PC("AMD ISP v4l2 device unregistered");
	for (i = 0; i < MAX_HW_NUM; i++) {
		if (!g_cam->hw_tbl[i])
			continue;
		kfree(g_cam->hw_tbl[i]);
		g_cam->hw_tbl[i] = NULL;
	}

	kfree(g_cam);
	kfree(g_s32);
	kfree(g_prop);
	g_prop = NULL;
	g_cam = NULL;
	return 0;
}

static void amd_pdev_release(struct device __maybe_unused *dev)
{
}

static struct platform_device amd_capture_dev = {
	.name = "amd_capture",
	.dev.release = amd_pdev_release,
};


static struct platform_driver amd_capture_drv = {
	.probe = amd_capture_probe,
	.remove = amd_capture_remove,
	.driver = {
		.name = "amd_capture",
		.owner = THIS_MODULE,
	}
};

static int __init amd_capture_init(void)
{
	int ret = OK;

	ENTER();

	ret = platform_device_register(&amd_capture_dev);
	if (ret) {
		ISP_PR_ERR("register platform device fail!");
		goto quit;
	}

	ret = platform_driver_register(&amd_capture_drv);
	if (ret)
		ISP_PR_ERR("register platform driver fail!");

#if	!defined(ISP3_SILICON)
	amd_pci_init();
#endif
quit:
	RET(ret);
	return ret;
}

static void __exit amd_capture_exit(void)
{
	ENTER();
#if	!defined(ISP3_SILICON)
	amd_pci_exit();
#endif
	platform_driver_unregister(&amd_capture_drv);

	/* TODO: should not call directly, remove it later */
	amd_capture_remove(NULL);
	EXIT();
}

module_init(amd_capture_init);
module_exit(amd_capture_exit);
