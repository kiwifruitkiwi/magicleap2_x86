/*
 * Copyright (C) 2019-2020 Advanced Micro Devices, Inc. All rights reserved.
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
#include <linux/cam_api_simulation.h>
#include <linux/random.h>

#include "amd_common.h"
#include "amd_params.h"
#include "amd_stream.h"
#include "amd_isp.h"
#include "sensor_if.h"
#include "isp_common.h"
#include "isp_module_if.h"
#include "amd_log.h"

#define VIDEO_DEV_BASE		1
#define DEFAULT_WIDTH		384
#define DEFAULT_HEIGHT		288
//TODO remove it when can get real info from HW
#define PHYSICAL_CAMERA_CNT 1

#define SEC_TO_NANO_SEC(num) (num * 1000000000)

//temporary workaround for store the mc addr.
uint64_t mc_store[3][4];

/** Simu AE constants */
const int pre_capture_min_frames = 10;
const int stable_ae_max_frames = 100;
int ae_counter;

static struct amd_cam *g_cam;
struct kset *kset_ref;
struct kobject *kobj_ref;
struct kobj_type ktype_ref;
static struct v4l2_subdev *get_isp_subdev(void);

/*
 * dma buf memops
 */
void *amd_attach_dmabuf(struct device *dev, struct dma_buf *dbuf,
			unsigned long size,
			enum dma_data_direction dma_dir)
{
	struct dma_buf_attachment *dba;
	struct amd_dma_buf *buf;

	logd("enter %s", __func__);
	if (dbuf->size < size)
		return ERR_PTR(-ENOMEM);

	buf = kzalloc(sizeof(*buf), GFP_KERNEL);
	if (!buf)
		return ERR_PTR(-ENOMEM);

	dba = dma_buf_attach(dbuf, buf->dev);
	if (IS_ERR(dba)) {
		loge("failed to attach dmabuf");
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

	logd("enter %s", __func__);
	dma_buf_detach(buf->db_attach->dmabuf, buf->db_attach);
	kfree(buf);
}

int amd_map_dmabuf(void *buf_priv)
{
	logd("enter %s", __func__);

	/*
	 * FIXME: check if there need extra map here
	 */
	return 0;
}

void amd_unmap_dmabuf(void *buf_priv)
{
	logd("enter %s", __func__);
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
	struct cam_stream *s = vb2_get_drv_priv(vq);
	unsigned long size;

	logd("enter %s, stream=[%d]", __func__, s->idx);

	size = s->cur_cfg.stride * s->cur_cfg.height *
		s->cur_cfg.fmt.depth >> 8;
	if (vq) {
		if (vq->type != V4L2_BUF_TYPE_VIDEO_CAPTURE) {
			loge("currently do not support mplane:%d", vq->type);
			return -EINVAL;
		}
		/* FIXME: more parameter */
//		if (fmt->fmt.pix.sizeimage < size)
//			return -EINVAL;
//		size = fmt->fmt.pix.sizeimage;
	}

	*nbuffers = s->cur_cfg.total_buf_num;
	*nplanes = 1;
	sizes[0] = size;

#ifdef ADVANCED_DEBUG
	loge("----------------------------------------");
	loge("queue info for stream[%d]:", s->idx);
	loge("queue address [%p]", vq);
	loge("buffer number [%d]", *nbuffers);
	loge("buffer length [0x%x(%d)]", sizes[0], sizes[0]);
	loge("buffer format [%d]", s->cur_cfg.fmt.format);
	loge("----------------------------------------");
#endif
	return 0;
}

static int buffer_init(struct vb2_buffer *vb)
{
	struct stream_buf *buf = container_of(vb, struct stream_buf, vb);

	logd("enter %s", __func__);

	INIT_LIST_HEAD(&buf->list);
	buf->num_of_bufs = 0;
	buf->combined_index = 0;
	buf->virt_cam_id = 0;
	buf->bufhandle = (unsigned long) NULL;
	buf->priv = (unsigned long) NULL;

	return 0;
}

static int buffer_prepare(struct vb2_buffer *vb)
{
	logd("enter %s", __func__);

	vb2_set_plane_payload(vb, 0, 0);

	return 0;
}

static void buffer_finish(struct vb2_buffer *vb)
{
	logd("enter %s", __func__);
}

static void buffer_cleanup(struct vb2_buffer *vb)
{
	logd("enter %s", __func__);
}

static void buffer_queue(struct vb2_buffer *vb)
{
	struct cam_stream *s = vb2_get_drv_priv(vb->vb2_queue);
	struct stream_buf *buf = container_of(vb, struct stream_buf, vb);
	unsigned long flags = 0;
	struct isp_ctl_ctx ctx;
	int i;
	int ret;

	logd("enter %s, stream=[%d]", __func__, s->idx);

	spin_lock_irqsave(&s->ab_lock, flags);
	list_add_tail(&buf->list, &s->active_buf);
	loge("new=[%p], head=[%p], head->next=[%p]", &buf->list,
		&s->active_buf, s->active_buf.next);
	spin_unlock_irqrestore(&s->ab_lock, flags);
	if (s->buf_type == STREAM_BUF_VMALLOC) {
		buf->handle.virtual_addr = vb2_plane_vaddr(&buf->vb, 0);
		buf->handle.physical_addr = mc_store[s->idx][s->q_id];
		buf->handle.private_buffer_info = NULL;
		buf->handle.handle = 0;
		logd("buffer [%p] MC [0x%llx] enqueue ",
			buf->handle.virtual_addr, buf->handle.physical_addr);
	} else {
		buf->handle.virtual_addr = 0;
		buf->handle.physical_addr = 0;
		buf->handle.private_buffer_info = NULL;
		buf->handle.handle = 0;//vb->v4l2_planes[0].m.fd;
	}
	buf->handle.len = 0;
	for (i = 0; i < buf->vb.num_planes; i++)
		buf->handle.len += vb2_plane_size(&buf->vb, i);

	logd("buffer [%p] enqueue ", buf->handle.virtual_addr);

	//TO DO: Enable once issue with metadata interfaces are fixed
//	ret = set_kparam_2_fw(s);
//	if (ret)
//		loge("%s: set kparam failed with %d", __func__, ret);

	ctx.cam = NULL;
	ctx.stream = s;
	ctx.ctx = buf;
	ret = v4l2_subdev_call(s->isp_sd, core, ioctl,
				ISP_IOCTL_QUEUE_BUFFER, &ctx);
	if (ret)
		loge("isp queue buffer failed, ret = %d", ret);
	else
		loge("isp queue buffer success!");
}

static int start_streaming(struct vb2_queue *vq, unsigned int count)
{
	struct cam_stream *s = vb2_get_drv_priv(vq);

	logd("enter %s, stream=[%d]", __func__, s->idx);
	s = s;
	return 0;
}

static void stop_streaming(struct vb2_queue *vq)
{
	int i;
	struct vb2_buffer *vb;
	struct stream_buf *buf;

	logd("enter %s", __func__);
	for (i = 0; i < DEFAULT_BUF_NUM; i++) {
		vb = vq->bufs[i];
		buf = container_of(vb, struct stream_buf, vb);
		if (vb->state == VB2_BUF_STATE_ACTIVE) {
			logd("%s: buf %p is active, done it", __func__, vb);
			list_del(&buf->list);
			vb2_buffer_done(vb, VB2_BUF_STATE_ERROR);
		} else
			logd("%s: vidq[%d] is not active", __func__, vb->index);
	}
}

static void amd_vb2_wait_prepare(struct vb2_queue *vq)
{
	struct cam_stream *s = vb2_get_drv_priv(vq);

	s = s;
	logd("enter %s, stream=[%d]", __func__, s->idx);
	/* mutex_unlock(&s->s_mutex); */
}

static void amd_vb2_wait_finish(struct vb2_queue *vq)
{
	struct cam_stream *s = vb2_get_drv_priv(vq);

	s = s;
	logd("enter %s, stream=[%d]", __func__, s->idx);
	/* mutex_lock(&s->s_mutex); */
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
	.wait_prepare = amd_vb2_wait_prepare,
	.wait_finish = amd_vb2_wait_finish,
};

/*
 * IOCTL vidioc handling
 */
static long _ioctl_default(struct file *f, void *fh,
		bool valid_prio, unsigned int cmd, void *arg)
{
	int ret = 0;
	struct physical_cam *cam = NULL;
	struct kernel_request *req;
	struct kernel_result *rst;
	struct kernel_stream_indexes *info;
	struct kernel_stream_configuration *cfg = NULL;

	logd("isp %s", __func__);
	assert(fh != NULL);
	assert(arg != NULL);

	cam = container_of(fh, struct physical_cam, fh);
	if (cam == NULL) {
		loge("%s: get cam failed!", __func__);
		return -1;
	}
	logd("isp %s cam id is %d", __func__, cam->sensor_id);

	switch (cmd) {
	case VIDIOC_CONFIG: {
		cfg = (struct kernel_stream_configuration *)arg;
		logd("isp %s VIDIOC_CONFIG", __func__);
		ret = streams_config(cam, cfg);
		break;
	}
	case VIDIOC_STREAM_ON: {
		logd("isp %s VIDIOC_STREAM_ON", __func__);
		info = (struct kernel_stream_indexes *)arg;
		ret = streams_on(cam, info);
		break;
	}
	case VIDIOC_REQUEST: {
		logd("isp %s VIDIOC_REQUEST", __func__);
		req = (struct kernel_request *)arg;
		ret = enq_request(cam, req);
		break;
	}
	case VIDIOC_RESULT: {
		logd("isp %s VIDIOC_RESULT", __func__);
		rst = (struct kernel_result *)arg;
		if (!cam->shutter_done)
			ret = sent_shutter(cam, rst);
		else
			ret = sent_buf_rst(f, cam, rst);
		break;
	}
	case VIDIOC_STREAM_OFF: {
		logd("isp %s VIDIOC_STREAM_OFF", __func__);
		info = (struct kernel_stream_indexes *)arg;
		ret = streams_off(cam, info, true);
		break;
	}
	case VIDIOC_STREAM_DESTROY: {
		logd("isp %s VIDIOC_STREAM_DESTROY", __func__);
		info = (struct kernel_stream_indexes *)arg;
		ret = streams_off(cam, info, false);
		break;
	}
	default:
		loge("ioctl %s no support ctrl", __func__);
		break;
	}

	logd("isp %s DONE with ret %d!", __func__, ret);
	return ret;
}

int streams_config(struct physical_cam *cam,
	struct kernel_stream_configuration *cfg)
{
	int i = 0, ret = 0;
	struct cam_stream *s;
	struct kernel_stream_indexes info;

	logd("enter %s, config %d streams", __func__, cfg->num_streams);
	//TODO: set module profile according cfg->profile

	// config streams setting
	for (i = 0; i < cfg->num_streams; i++) {
		s = cam->stream_tbl[cfg->streams[i]->stream_id];
		if (s->state == STREAM_PREPARING) {
			logi("%s: reconfig s_%d", __func__, s->idx);
			info.stream_cnt = 1;
			info.stream_ids[0] = s->idx;
			ret = streams_off(cam, &info, false);
			logi("%s: stream off ret is %d", __func__, ret);
		}
		if (s->state == STREAM_UNINIT) {
			if (stream_configure(cam, s, cfg->streams[i]))
				return -1;
			if (s->cur_cfg.fmt.type == F_YUV)
				cam->yuv_s_cnt++;
		}
	}
	return 0;
}

int stream_configure(struct physical_cam *cam, struct cam_stream *s,
	struct kernel_stream *stream_cfg)
{
	struct isp_ctl_ctx ctx;
	int ret;

	logd("enter %s", __func__);
	assert(stream_cfg != NULL);
	stream_config_init(cam, s, stream_cfg);

	/* FIXME: inform isp to add a new stream */
	ctx.cam = NULL;
	ctx.stream = s;
	ctx.ctx = NULL;
	ret = v4l2_subdev_call(cam->isp_sd, core, ioctl,
			ISP_IOCTL_STREAM_OPEN, &ctx);
	if (!ret) {
		logd("%s: s_%d open success!", __func__, s->idx);
		s->state = STREAM_PREPARING;
		if (!ret) {
			ret = alloc_vb2_buf_structure(s, DEFAULT_BUF_NUM);
			if (ret)
				loge("%s: s_%d vb2 alloc buf fail, ret %d",
					__func__, s->idx, ret);
		} else
			loge("%s: s_%d setFmt fail(%d)", __func__, s->idx, ret);
	} else {
		loge("%s: s_%d open fail, ret %d", __func__, s->idx, ret);
	}

	return ret;
}

void stream_config_init(struct physical_cam *cam, struct cam_stream *s,
	struct kernel_stream *hal_cfg)
{
	struct stream_config *cfg;
	struct amd_format *fmt;

	assert(cam != NULL);
	assert(s != NULL);
	assert(hal_cfg != NULL);
	logd("enter %s", __func__);

	cfg = &s->cur_cfg;
	cfg->sensor_id = cam->sensor_id;
	cfg->total_buf_num = DEFAULT_BUF_NUM;
	cfg->width = hal_cfg->width;
	cfg->height = hal_cfg->height;
	cfg->stride = ISP_ADDR_ALIGN_UP(hal_cfg->stride, 256);
	cfg->priv = hal_cfg->priv;
	fmt = amd_find_match(hal_cfg->format);
	if (fmt == NULL) {
		loge("invalid format parameter!! use default fmt");
		fmt = select_default_format();
	}
	memcpy(&cfg->fmt, fmt, sizeof(struct amd_format));

	/** The first yuv stream named preview
	 *  The second yuv stream named ZSL
	 *  The third yuv stream named video
	 */
	if (fmt->type == F_YUV)
		s->type = cam->yuv_s_cnt;
	else if (fmt->type == F_RAW)
		logd("%s set fmt to RAW!", __func__);
	logd("%s ************* config info **************", __func__);
	logd("%s stream id = %d", __func__, s->idx);
	logd("%s stream type = %d", __func__, s->type);
	logd("%s wxh = %dx%d", __func__, cfg->width, cfg->height);
	logd("%s stride = %d", __func__, cfg->stride);
	logd("%s format = %s", __func__, cfg->fmt.name);
}

int stream_set_fmt(struct cam_stream *s)
{
	struct isp_ctl_ctx ctx;
	int ret = 0;

	assert(s != NULL);
	logd("enter %s, stream=[%d]", __func__, s->idx);
	ctx.cam = NULL;
	ctx.stream = s;
	ctx.ctx = NULL;
	ret = v4l2_subdev_call(s->isp_sd, core, ioctl,
			ISP_IOCTL_S_FMT, &ctx);
	if (ret)
		loge("%s: isp set format failed, ret = %d", __func__, ret);
	else
		logd("%s: isp set format success!", __func__);

	return ret;
}

int alloc_vb2_buf_structure(struct cam_stream *s, int count)
{
	int ret = 0;
	struct v4l2_requestbuffers req;

	assert(s != NULL);
	logd("enter %s, stream=[%d]", __func__, s->idx);

	req.count = count;
	req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory = V4L2_MEMORY_USERPTR;
	ret = vb2_reqbufs(&s->vidq, &req);
	if (ret)
		logd("%s, alloc vb2 buf for stream %d failed ret=%d",
			__func__, s->idx, ret);
	else
		logd("%s, alloc vb2 buf for stream %d suc!", __func__, s->idx);

	return ret;
}

int streams_on(struct physical_cam *cam, struct kernel_stream_indexes *info)
{
	int i = 0;
	struct cam_stream *s = NULL;

	logd("enter %s, on %d streams", __func__, info->stream_cnt);
	for (i = 0; i < info->stream_cnt; i++) {
		s = cam->stream_tbl[info->stream_ids[i]];
		if (stream_on(s, cam))
			return -1;
	}

	return 0;
}

int stream_on(struct cam_stream *s, struct physical_cam *cam)
{
	int ret;
	struct isp_ctl_ctx ctx;

	logd("enter %s, stream=[%d]", __func__, s->idx);
	if (s->state == STREAM_PREPARING) {
		ret = stream_set_fmt(s);
		if (ret) {
			loge("%s: stream setParam fail, ret %d", __func__, ret);
			return ret;
		}
		ret = vb2_streamon(&s->vidq, s->vidq.type);
		if (ret) {
			loge("%s: vb2q stream on fail, ret %d", __func__, ret);
			return ret;
		}
		logi("%s: vb2q stream on success!", __func__);

		ctx.cam = NULL;
		ctx.stream = s;
		ctx.ctx = NULL;
		ret = v4l2_subdev_call(s->isp_sd, core, ioctl,
				ISP_IOCTL_STREAM_ON, &ctx);
		if (ret) {
			loge("%s: isp stream on failed, ret = %d",
				__func__, ret);
		} else {
			logd("%s: isp stream on success!", __func__);
			s->state = STREAM_ACTIVE;
			cam->on_s_cnt++;

			//reset some info
			s->q_id = 0;
			s->dq_id = 0;
		}
	} else if (s->state == STREAM_ACTIVE) {
		logw("%s: stream has already started", __func__);
		ret = 0;
	} else {
		loge("%s: error stream state [%d]",
			__func__, s->state);
		ret = -1;
	}

	return ret;
}

int enq_request(struct physical_cam *cam, struct kernel_request *req)
{
	int i, ret = 0;
	struct cam_stream *s;
	struct v4l2_buffer vbuf;
	struct kernel_buffer *kbuf;

	logd("enter %s", __func__);
	assert(req != NULL);
	//TODO: set param according the req->param

	for (i = 0; i < req->num_output_buffers; i++) {
		kbuf = req->output_buffers[i];
		s = cam->stream_tbl[kbuf->stream_id];
		if (s->state != STREAM_ACTIVE) {
			loge("%s s_%d state is %d", __func__, s->idx, s->state);
			return -1;
		}
		memset(&vbuf, 0, sizeof(struct v4l2_buffer));
		ret = prepare_v4l2_buf(kbuf, s, &vbuf, cam);
		if (ret)
			return ret;

		mc_store[s->idx][s->q_id] = kbuf->mc_addr;

		//enqueue the buf to vb2q
		ret = vb2_qbuf(&s->vidq, NULL, &vbuf);
		if (!ret) {
			logd("%s vb2_qbuf suc, stream %d handle %p",
				__func__, s->idx, kbuf->buffer);
			store_request_info(s, req, kbuf);
			atomic_inc(&s->owned_by_stream_count);
		} else
			loge("%s s_%d vb2_qbuf fail!%d", __func__, s->idx, ret);
	}
	return ret;
}

int prepare_v4l2_buf(struct kernel_buffer *kbuf, struct cam_stream *s,
	struct v4l2_buffer *buf, struct physical_cam *cam)
{
	logd("enter %s", __func__);
	buf->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	buf->memory = V4L2_MEMORY_USERPTR;
	buf->index = s->q_id;
	//buf->m.userptr = (unsigned long)kbuf->buffer;
	//buf->length = s->cur_cfg.width *
	//(s->cur_cfg.hYeight + (s->cur_cfg.height + 1 ) / 2);//stride?
	// This is for simu camera!
	buf->m.userptr = (unsigned long) kbuf->user_ptr;
	buf->length = kbuf->length;
	memset((void *) buf->m.userptr, 0x0, buf->length);
	return 0;
}

void store_request_info(struct cam_stream *s, struct kernel_request *req,
	struct kernel_buffer *kbuf)
{
	struct stream_buf *s_buf;

	logd("enter %s", __func__);
	assert(s != NULL && req != NULL && kbuf != NULL);
	s_buf = container_of(s->vidq.bufs[s->q_id],
		struct stream_buf, vb);
	s_buf->combined_index = req->com_req_id;
	s_buf->num_of_bufs = req->num_output_buffers;
	s_buf->kparam = (unsigned long)req->kparam;
	s_buf->virt_cam_id = kbuf->vcam_id;
	s_buf->bufhandle = (unsigned long) kbuf->buffer;
	s_buf->priv = (unsigned long) kbuf->priv;
//#ifdef ADVANCED_DEBUG
	logd("%s ------------ stream info ----------------", __func__);
	logd("%s sensor id = %d", __func__, s->cur_cfg.sensor_id);
	//logd("%s virtual cam id = %d", __func__, kbuf->vcam_id);
	//logd("%s stream type = %d", __func__, s->type);
	logd("%s stream id = %d", __func__, s->idx);
	//logd("%s wxh = %dx%d", __func__, s->cur_cfg.width, s->cur_cfg.height);
	//logd("%s stride = %d", __func__, s->cur_cfg.stride);
	//logd("%s format = %s", __func__, s->cur_cfg.fmt.name);
	logd("%s ---------enque buffer info --------------", __func__);
	logd("%s enq id = %d", __func__, s->q_id);
	logd("%s dq id = %d", __func__, s->dq_id);
	logd("%s combined id = %d", __func__, req->com_req_id);
	logd("%s output_buf_cnt = %d", __func__, req->num_output_buffers);
	logd("%s framework buf = %p", __func__, kbuf->buffer);
	logd("%s HAL buf = %p", __func__, kbuf->user_ptr);
	logd("%s vaddr[%p]", __func__, s_buf->handle.virtual_addr);
	logd("%s priv[%lu]", __func__, s_buf->priv);
	logd("%s md[%lu]", __func__, s_buf->kparam);
	logd("%s ---------------------------------------", __func__);
//#endif
	s->q_id = (++s->q_id == DEFAULT_BUF_NUM) ? 0 : s->q_id;
	logd("%s update current buf index to %d", __func__, s->q_id);
}

int sent_shutter(struct physical_cam *cam, struct kernel_result *rst)
{
	int i = 0;
	struct timespec timestamp;
	uint64_t time_nsec;
	struct stream_buf *s_buf = NULL;
	struct cam_stream *s = NULL;
	struct kernel_buffer *kbuf;

	logd("enter %s", __func__);
	assert(cam != NULL);
	assert(rst != NULL);

	for (i = 0; i < MAX_STREAM_NUM_PER_PHYSICAL_CAMERA; i++) {
		kbuf = rst->output_buffers[i];
		if (kbuf == NULL) {
			logd("dq continue %d", i);
			continue;
		}

		s = cam->stream_tbl[kbuf->stream_id];
		s_buf = container_of(s->vidq.bufs[s->dq_id], struct stream_buf,
			vb);
		if (s_buf->num_of_bufs > 0)
			break;
	}

	if (i == MAX_STREAM_NUM_PER_PHYSICAL_CAMERA) {
		loge("%s no request in kernel!", __func__);
		return -1;
	}

	rst->com_req_id = s_buf->combined_index;
	rst->partial_result = 1;
	rst->status = ERROR_NONE;
	rst->input_buffer = NULL;
	rst->num_output_buffers = 0;
	rst->is_shutter_event = true;

	ktime_get_ts(&timestamp);
	time_nsec = SEC_TO_NANO_SEC(timestamp.tv_sec);
	time_nsec += timestamp.tv_nsec;
	rst->data.timestamp = time_nsec;

	cam->shutter_timestamp = time_nsec;
	cam->shutter_done = true;
	logd("%s for com_id %d suc, and output buf cnt is %d", __func__,
		s_buf->combined_index, s_buf->num_of_bufs);

	return 0;
}

int sent_buf_rst(struct file *f, struct physical_cam *cam,
	struct kernel_result *rst)
{
	int i, ret = 0, num_buffers = 0;
	int output_buf_cnt = 0;
	struct v4l2_buffer vbuf;
	struct stream_buf *s_buf = NULL;
	struct cam_stream *s = NULL;
	struct kernel_buffer *kbuf;
	struct stream_config cfg;

	assert(cam != NULL);
	assert(rst != NULL);
	logd("enter %s", __func__);

	cam->partial_results++;
	rst->partial_result = cam->partial_results;
	rst->is_shutter_event = false;
	rst->status = ERROR_NONE;
	rst->input_buffer = NULL;
	rst->data.kparam_rst = NULL;
	rst->num_output_buffers = 0;
	for (i = 0; i < MAX_STREAM_NUM_PER_PHYSICAL_CAMERA; i++) {
		kbuf = rst->output_buffers[i];
		if (!kbuf) {
			logd("dq continue %d", i);
			continue;
		}

		s = cam->stream_tbl[kbuf->stream_id];
		if (!s) {
			rst->status = ERROR_INVALIDOPERATION;
			return -EINVAL;
		}

		loge("%s dq from stream %d", __func__, s->idx);
		s_buf = container_of(s->vidq.bufs[s->dq_id], struct stream_buf,
			vb);

		if (cam->is_metadata_sent)
			s_buf->kparam = 0;

		cfg = s->cur_cfg;
		output_buf_cnt = s_buf->num_of_bufs;
		rst->com_req_id = s_buf->combined_index;
		rst->num_output_buffers++;
		kbuf->priv = (void *)s_buf->priv;
		kbuf->buffer = (void *)s_buf->bufhandle;

		if (s->state == STREAM_ACTIVE) {
			vbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			vbuf.memory = V4L2_MEMORY_USERPTR;
			ret = vb2_dqbuf(&s->vidq, &vbuf,
				f->f_flags & O_NONBLOCK);
			if (!ret) {
				logd("%s vb2_dqbuf success!", __func__);
				kbuf->status = BUFFER_STATUS_OK;
				// This is for simu
				kbuf->length = vbuf.length;
				kbuf->user_ptr = (void *) vbuf.m.userptr;
				num_buffers = atomic_dec_return
					(&s->owned_by_stream_count);
				if (!num_buffers) {
					pr_info("%s stream %d wakeup",
						__func__, s->idx);
					wake_up(&s->done_wq);
				}
			} else {
				loge("%s vb2_dqbuf fail ret %d", __func__, ret);
				if (s->state != STREAM_ACTIVE) {
					loge("%s s_%d is already off", __func__,
						s->idx);
					kbuf->status = BUFFER_STATUS_ERROR;
					kbuf->user_ptr = NULL;
					ret = 0;
				} else {
					rst->status = ERROR_INVALIDOPERATION;
				}
			}
		} else {
			// should never go here
			loge("error stream %d state [%d]", s->idx, s->state);
			kbuf->status = BUFFER_STATUS_ERROR;
		}

		if (!cam->is_metadata_sent) {
			struct amd_cam_metadata *metadata;

			logd("add md in result %lu\n", s_buf->kparam);
			cam->is_metadata_sent = true;
			rst->data.kparam_rst = (void *)s_buf->kparam;
			metadata =
				(struct amd_cam_metadata *)rst->data.kparam_rst;
			ret = metadata_update_sensor_timestamp
				(metadata,
				 &cam->shutter_timestamp);
			if (ret)
				loge("sensor timestamp updation failed\n");

			//TO DO: Enable once issue with metadata interfaces are fixed
//			ret = update_kparam(s);
//			if (ret)
//				loge("update_metadata failed\n");

			//TO DO: remove the simu 3a.
			ret = process_3a(metadata, cam);
			if (ret)
				loge("process 3A failed\n");
		}
//#ifdef ADVANCED_DEBUG
		logd("%s =========== stream info ==========", __func__);
		logd("%s sensor id = %d", __func__, cfg.sensor_id);
		//logd("%s virtual cam id = %d", __func__, kbuf->vcam_id);
		//logd("%s stream type = %d", __func__, s->type);
		logd("%s stream id = %d", __func__, s->idx);
		//logd("%s wxh = %dx%d", __func__, cfg.width, cfg.height);
		//logd("%s stride = %d", __func__, s->cur_cfg.stride);
		//logd("%s format = %s", __func__, s->cur_cfg.fmt.name);
		logd("%s ====== rst buffer info suc =======", __func__);
		logd("%s enq id = %d", __func__, s->q_id);
		logd("%s dq id = %d", __func__, s->dq_id);
		logd("%s combined id = %d", __func__, rst->com_req_id);
		logd("%s framework buf = %p", __func__, kbuf->buffer);
		logd("%s HAL buf = %p", __func__, kbuf->user_ptr);
		logd("%s priv = %p", __func__, kbuf->priv);
		logd("%s ==================================", __func__);
//#endif
		if (!ret) {
			cam->dq_buf_cnt++;
			s_buf->num_of_bufs = 0;
			s_buf->combined_index = 0;
			s_buf->virt_cam_id = 0;
			s_buf->bufhandle = (unsigned long) NULL;
			s_buf->priv = (unsigned long) NULL;
			s->dq_id = ++s->dq_id == DEFAULT_BUF_NUM ? 0 : s->dq_id;
		}
	}

	if (cam->dq_buf_cnt == output_buf_cnt) {
		cam->partial_results = 0;
		cam->dq_buf_cnt = 0;
		cam->shutter_done = false;
		cam->is_metadata_sent = false;
		cam->shutter_timestamp = 0;
	}
	return ret;
}

int process_3a(struct amd_cam_metadata *metadata, struct physical_cam *cam)
{
	int res;
	u8 control_mode;
	struct amd_cam_metadata_entry entry;
	struct algo_3a_states *states_3a = &cam->states;

	res =
	    amd_cam_find_metadata_entry(metadata, AMD_CAM_CONTROL_MODE, &entry);
	if (res) {
		loge("3A: CONTROL_MODE tag not found\n");
		return res;
	}

	control_mode = entry.data.u8[0];

	if (control_mode == AMD_CAM_CONTROL_MODE_OFF) {
		states_3a->prv_control_mode = AMD_CAM_CONTROL_MODE_OFF;
		states_3a->prv_scene_mode = AMD_CAM_CONTROL_SCENE_MODE_DISABLED;
		states_3a->prv_ae_mode = AMD_CAM_CONTROL_AE_MODE_OFF;
		states_3a->prv_af_mode = AMD_CAM_CONTROL_AF_MODE_OFF;

		states_3a->ae_state = AMD_CAM_CONTROL_AE_STATE_INACTIVE;
		states_3a->af_state = AMD_CAM_CONTROL_AF_STATE_INACTIVE;
		states_3a->awb_state = AMD_CAM_CONTROL_AWB_STATE_INACTIVE;
		update_3a(metadata, cam);
		return 0;
	}
	// if controlMode == AUTO,
	// Process individual 3A controls
	res = do_ae(metadata, cam);
	if (res) {
		loge("3A: do_ae failed[%d]\n", res);
		return res;
	}
	// As we do not support AF, ignoring the doSimuAF return value.
	do_af(metadata, cam);

	res = do_awb(metadata, cam);
	if (res) {
		loge("3A: do_awb failed[%d]\n", res);
		return res;
	}

	update_3a(metadata, cam);

	//Storing the mode values for next usage
	cam->states.prv_control_mode = control_mode;

	if (control_mode == AMD_CAM_CONTROL_MODE_USE_SCENE_MODE) {
		res =
		    amd_cam_find_metadata_entry(metadata,
						AMD_CAM_CONTROL_SCENE_MODE,
						&entry);
		if (res) {
			loge("3A: CONTROL_SCENE_MODE tag not found\n");
			return res;
		}
		cam->states.prv_scene_mode = entry.data.u8[0];
	}

	res =
	    amd_cam_find_metadata_entry(metadata, AMD_CAM_CONTROL_AE_MODE,
					&entry);
	if (res) {
		loge("3A: CONTROL_AE_MODE tag not found\n");
		return res;
	}
	cam->states.prv_ae_mode = entry.data.u8[0];

	res =
	    amd_cam_find_metadata_entry(metadata, AMD_CAM_CONTROL_AF_MODE,
					&entry);
	if (res) {
		loge("3A: CONTROL_AF_MODE tag not found\n");
		return res;
	}
	cam->states.prv_af_mode = entry.data.u8[0];

	return res;
}

int do_ae(struct amd_cam_metadata *metadata, struct physical_cam *cam)
{
	int res;
	u8 control_mode, ae_mode, ae_state;
	u8 prv_control_mode;
	bool ae_locked = false, mode_switch = false;
	bool precapture_trigger = false;
	struct amd_cam_metadata_entry entry;
	struct algo_3a_states *states_3a = &cam->states;

	res =
	    amd_cam_find_metadata_entry(metadata, AMD_CAM_CONTROL_MODE, &entry);
	if (res) {
		loge("AE: CONTROL_MODE tag not found\n");
		return res;
	}
	control_mode = entry.data.u8[0];

	prv_control_mode = states_3a->prv_control_mode;

	if (control_mode != prv_control_mode) {
		loge("AE: control mode switch\n");
		mode_switch = true;
		goto mode_switch_case;
	}

	if (control_mode == AMD_CAM_CONTROL_MODE_USE_SCENE_MODE &&
	    prv_control_mode == control_mode) {
		res =
		    amd_cam_find_metadata_entry(metadata,
						AMD_CAM_CONTROL_SCENE_MODE,
						&entry);
		if (res) {
			loge("AE: CONTROL_SCENE_MODE tag not found\n");
			return res;
		}

		if (entry.data.u8[0] != states_3a->prv_scene_mode) {
			loge("AE: scene mode switch\n");
			mode_switch = true;
			goto mode_switch_case;
		}
	}

	res =
	    amd_cam_find_metadata_entry(metadata, AMD_CAM_CONTROL_AE_MODE,
					&entry);
	if (res) {
		loge("AE: CONTROL_AE_MODE tag not found\n");
		return res;
	}
	ae_mode = entry.data.u8[0];

	if (ae_mode != states_3a->prv_ae_mode) {
		loge("AE: ae mode switch\n");
		mode_switch = true;
		goto mode_switch_case;
	}

	ae_state = states_3a->ae_state;

	switch (ae_mode) {
	case AMD_CAM_CONTROL_AE_MODE_OFF:
		// AE is OFF
		states_3a->ae_state = AMD_CAM_CONTROL_AE_STATE_INACTIVE;
		return 0;
	case AMD_CAM_CONTROL_AE_MODE_ON:
		// OK for AUTO modes
		break;
	default:
		// Mostly silently ignore unsupported modes
		logw("AE: doesn't support AE mode %d\n", ae_mode);
		break;
	}

	res =
	    amd_cam_find_metadata_entry(metadata, AMD_CAM_CONTROL_AE_LOCK,
					&entry);
	if (!res)
		ae_locked = (entry.data.u8[0] == AMD_CAM_CONTROL_AE_LOCK_ON);

	res =
	    amd_cam_find_metadata_entry(metadata,
					AMD_CAM_CONTROL_AE_PRECAPTURE_TRIGGER,
					&entry);
	if (!res) {
		precapture_trigger =
		    (entry.data.u8[0] ==
		     AMD_CAM_CONTROL_AE_PRECAPTURE_TRIGGER_START);
	}

	if (ae_locked) {
		// AE is locked
		loge("AE: aeLock[%d]\n", ae_locked);
		ae_state = AMD_CAM_CONTROL_AE_STATE_LOCKED;
	} else if (precapture_trigger ||
		   ae_state == AMD_CAM_CONTROL_AE_STATE_SEARCHING) {
		// Run precapture sequence
		if (ae_state != AMD_CAM_CONTROL_AE_STATE_SEARCHING)
			ae_counter = 0;

		if (ae_counter > pre_capture_min_frames) {
			// Done with precapture
			ae_counter = 0;
			ae_state = AMD_CAM_CONTROL_AE_STATE_CONVERGED;
		} else {
			// Converge some more
			ae_counter++;
			ae_state = AMD_CAM_CONTROL_AE_STATE_SEARCHING;
		}
	} else if (!ae_locked) {
		// Run standard occasional AE scan
		switch (ae_state) {
		case AMD_CAM_CONTROL_AE_STATE_INACTIVE:
			ae_state = AMD_CAM_CONTROL_AE_STATE_SEARCHING;
			break;
		case AMD_CAM_CONTROL_AE_STATE_CONVERGED:
			ae_counter++;
			if (ae_counter > stable_ae_max_frames)
				ae_state = AMD_CAM_CONTROL_AE_STATE_SEARCHING;
			break;
		case AMD_CAM_CONTROL_AE_STATE_SEARCHING:
			ae_counter++;
			if (ae_counter > pre_capture_min_frames) {
				// Close enough
				ae_state = AMD_CAM_CONTROL_AE_STATE_CONVERGED;
				ae_counter = 0;
			}
			break;
		case AMD_CAM_CONTROL_AE_STATE_LOCKED:
			ae_state = AMD_CAM_CONTROL_AE_STATE_CONVERGED;
			ae_counter = 0;
			break;
		default:
			loge("AE: In unexpected AE state %d\n",
			     ae_state);
			return -EINVAL;
		}
	}

 mode_switch_case:
	if (mode_switch) {
		loge("AE: mode switch detected\n");
		ae_state = AMD_CAM_CONTROL_AE_STATE_INACTIVE;
	}

	states_3a->ae_state = ae_state;
	loge("AE: ae_state:%d, ae_lock:%d, ae_trigger:%d\n",
	     ae_state, ae_locked, precapture_trigger);

	return 0;
}

int do_af(struct amd_cam_metadata *metadata, struct physical_cam *cam)
{
	int res, rand_num;
	u8 af_state, af_mode, af_trigger;
	bool af_trigger_start = false;
	bool af_trigger_cancel = false, af_mode_changed = false;
	struct amd_cam_metadata_entry entry;
	struct algo_3a_states *states_3a = &cam->states;

	af_state = AMD_CAM_CONTROL_AF_STATE_INACTIVE;

	res = amd_cam_find_metadata_entry(metadata, AMD_CAM_CONTROL_AF_MODE,
					  &entry);
	if (res) {
		loge("AF: CONTROL_AF_MODE tag not found\n");
		return res;
	}

	af_mode = entry.data.u8[0];

	res = amd_cam_find_metadata_entry(metadata, AMD_CAM_CONTROL_AF_TRIGGER,
					  &entry);
	if (res)
		af_trigger = AMD_CAM_CONTROL_AF_TRIGGER_IDLE;
	else
		af_trigger = entry.data.u8[0];

	switch (af_mode) {
	case AMD_CAM_CONTROL_AF_MODE_OFF:
		af_state = AMD_CAM_CONTROL_AF_STATE_INACTIVE;
		states_3a->af_state = af_state;
		return 0;
	case AMD_CAM_CONTROL_AF_MODE_AUTO:
	case AMD_CAM_CONTROL_AF_MODE_MACRO:
	case AMD_CAM_CONTROL_AF_MODE_CONTINUOUS_VIDEO:
	case AMD_CAM_CONTROL_AF_MODE_CONTINUOUS_PICTURE:
		break;
	default:
		loge("AF: doesn't support AF mode %d\n", af_mode);
		return -EINVAL;
	}

	af_mode_changed = states_3a->prv_af_mode != af_mode;
	states_3a->prv_af_mode = af_mode;

	/*
	 * Simulate AF triggers. Transition at most 1 state per frame.
	 * - Focusing always succeeds (goes into locked, or PASSIVE_SCAN)
	 */

	switch (af_trigger) {
	case AMD_CAM_CONTROL_AF_TRIGGER_IDLE:
		break;
	case AMD_CAM_CONTROL_AF_TRIGGER_START:
		af_trigger_start = true;
		break;
	case AMD_CAM_CONTROL_AF_TRIGGER_CANCEL:
		af_trigger_cancel = true;
		// Cancel trigger always transitions into INACTIVE
		af_state = AMD_CAM_CONTROL_AF_STATE_INACTIVE;

		loge("AF: AF State transition to STATE_INACTIVE\n");

		return 0;
	default:
		loge("AF: Unknown AF trigger value %d\n",
		     af_trigger);

		return -EINVAL;
	}

	// If we get down here, we're either in an autofocus mode
	//  or in a continuous focus mode (and no other modes)

	switch (af_state) {
	case AMD_CAM_CONTROL_AF_STATE_INACTIVE:
		if (af_trigger_start) {
			switch (af_mode) {
			case AMD_CAM_CONTROL_AF_MODE_AUTO:
				// fall-through
			case AMD_CAM_CONTROL_AF_MODE_MACRO:
				af_state = AMD_CAM_CONTROL_AF_STATE_ACTIVE_SCAN;
				break;
			case AMD_CAM_CONTROL_AF_MODE_CONTINUOUS_VIDEO:
				// fall-through
			case AMD_CAM_CONTROL_AF_MODE_CONTINUOUS_PICTURE:
				af_state =
				    AMD_CAM_CONTROL_AF_STATE_NOT_FOCUSED_LOCKED;
				break;
			}
		} else {
			// At least one frame stays in INACTIVE
			if (!af_mode_changed) {
				switch (af_mode) {
				case AMD_CAM_CONTROL_AF_MODE_CONTINUOUS_VIDEO:
					// fall-through
				case AMD_CAM_CONTROL_AF_MODE_CONTINUOUS_PICTURE:
					af_state =
					AMD_CAM_CONTROL_AF_STATE_PASSIVE_SCAN;
					break;
				}
			}
		}
		break;

	case AMD_CAM_CONTROL_AF_STATE_PASSIVE_SCAN:
		/*
		 * When the AF trigger is activated, the algorithm should finish
		 * its PASSIVE_SCAN if active, and then transition into
		 * AF_FOCUSED or AF_NOT_FOCUSED as appropriate
		 */
		if (af_trigger_start) {
			// Randomly transition to focused or not focused
			get_random_bytes(&rand_num, sizeof(rand_num));
			if (rand_num % 3)
				af_state =
				    AMD_CAM_CONTROL_AF_STATE_FOCUSED_LOCKED;
			else
				af_state =
				    AMD_CAM_CONTROL_AF_STATE_NOT_FOCUSED_LOCKED;
		}
		/*
		 * When the AF trigger is not involved, the AF algorithm should
		 * start in INACTIVE state, and then transition into
		 * PASSIVE_SCAN and PASSIVE_FOCUSED states
		 */
		else if (!af_trigger_cancel) {
			// Randomly transition to passive focus
			get_random_bytes(&rand_num, sizeof(rand_num));
			if (rand_num % 3 == 0) {
				af_state =
				    AMD_CAM_CONTROL_AF_STATE_PASSIVE_FOCUSED;
			}
		}
		break;

	case AMD_CAM_CONTROL_AF_STATE_PASSIVE_FOCUSED:
		if (af_trigger_start) {
			// Randomly transition to focused or not focused
			get_random_bytes(&rand_num, sizeof(rand_num));
			if (rand_num % 3) {
				af_state =
				    AMD_CAM_CONTROL_AF_STATE_FOCUSED_LOCKED;
			} else {
				af_state =
				    AMD_CAM_CONTROL_AF_STATE_NOT_FOCUSED_LOCKED;
			}
		}
		// TODO: initiate passive scan (PASSIVE_SCAN)
		break;

	case AMD_CAM_CONTROL_AF_STATE_ACTIVE_SCAN:
		// Simulate AF sweep completing instantaneously

		// Randomly transition to focused or not focused
		get_random_bytes(&rand_num, sizeof(rand_num));
		if (rand_num % 3)
			af_state = AMD_CAM_CONTROL_AF_STATE_FOCUSED_LOCKED;
		else
			af_state = AMD_CAM_CONTROL_AF_STATE_NOT_FOCUSED_LOCKED;

		break;

	case AMD_CAM_CONTROL_AF_STATE_FOCUSED_LOCKED:
		if (af_trigger_start) {
			switch (af_mode) {
			case AMD_CAM_CONTROL_AF_MODE_AUTO:
				// fall-through
			case AMD_CAM_CONTROL_AF_MODE_MACRO:
				af_state = AMD_CAM_CONTROL_AF_STATE_ACTIVE_SCAN;
				break;
			case AMD_CAM_CONTROL_AF_MODE_CONTINUOUS_VIDEO:
				// fall-through
			case AMD_CAM_CONTROL_AF_MODE_CONTINUOUS_PICTURE:
				// continuous autofocus => trigger start has no effect
				break;
			}
		}
		break;

	case AMD_CAM_CONTROL_AF_STATE_NOT_FOCUSED_LOCKED:
		if (af_trigger_start) {
			switch (af_mode) {
			case AMD_CAM_CONTROL_AF_MODE_AUTO:
				// fall-through
			case AMD_CAM_CONTROL_AF_MODE_MACRO:
				af_state = AMD_CAM_CONTROL_AF_STATE_ACTIVE_SCAN;
				break;
			case AMD_CAM_CONTROL_AF_MODE_CONTINUOUS_VIDEO:
				// fall-through
			case AMD_CAM_CONTROL_AF_MODE_CONTINUOUS_PICTURE:
				// continuous autofocus => trigger start has no effect
				break;
			}
		}
		break;

	default:
		loge("AF: Bad AF state %d\n", af_state);
	}

	states_3a->af_state = af_state;

	return 0;
}

int do_awb(struct amd_cam_metadata *metadata, struct physical_cam *cam)
{
	int res;
	u8 awb_mode, awb_state;
	bool awb_locked = false;
	struct amd_cam_metadata_entry entry;
	struct algo_3a_states *states_3a = &cam->states;

	res = amd_cam_find_metadata_entry(metadata, AMD_CAM_CONTROL_AWB_MODE,
					  &entry);
	if (res) {
		loge("AWB: CONTROL_AWB_MODE tag not found\n");
		return res;
	}

	awb_mode = entry.data.u8[0];

	res = amd_cam_find_metadata_entry(metadata, AMD_CAM_CONTROL_AWB_LOCK,
					  &entry);
	if (!res)
		awb_locked = (entry.data.u8[0] == AMD_CAM_CONTROL_AWB_LOCK_ON);

	switch (awb_mode) {
	case AMD_CAM_CONTROL_AWB_MODE_OFF:
		awb_state = AMD_CAM_CONTROL_AWB_STATE_INACTIVE;
		break;
	case AMD_CAM_CONTROL_AWB_MODE_AUTO:
	case AMD_CAM_CONTROL_AWB_MODE_INCANDESCENT:
	case AMD_CAM_CONTROL_AWB_MODE_FLUORESCENT:
	case AMD_CAM_CONTROL_AWB_MODE_DAYLIGHT:
	case AMD_CAM_CONTROL_AWB_MODE_SHADE:
		// Always magically right, or locked
		awb_state = awb_locked ? AMD_CAM_CONTROL_AWB_STATE_LOCKED
		    : AMD_CAM_CONTROL_AWB_STATE_CONVERGED;
		break;
	default:
		loge("AWB: doesn't support AWB mode %d\n",
		     awb_mode);
		return -EINVAL;
	}

	states_3a->awb_state = awb_state;
	return 0;
}

void update_3a_region(u32 tag, struct amd_cam_metadata *metadata)
{
	int res;
	s32 crop_region[4], *a_region;
	struct amd_cam_metadata_entry entry;

	if (tag != AMD_CAM_CONTROL_AE_REGIONS &&
	    tag != AMD_CAM_CONTROL_AF_REGIONS &&
	    tag != AMD_CAM_CONTROL_AWB_REGIONS) {
		logw("3A: invalid tag 0x%x\n", tag);
		return;
	}

	res = amd_cam_find_metadata_entry(metadata, AMD_CAM_SCALER_CROP_REGION,
					  &entry);
	if (res)
		return;

	crop_region[0] = entry.data.i32[0];
	crop_region[1] = entry.data.i32[1];
	crop_region[2] = (entry.data.i32[2] + crop_region[0]);
	crop_region[3] = (entry.data.i32[3] + crop_region[1]);

	res = amd_cam_find_metadata_entry(metadata, tag, &entry);
	if (res)
		return;

	a_region = entry.data.i32;
	// calculate the intersection of AE/AF/AWB and CROP regions
	if (a_region[0] < crop_region[2] && crop_region[0] < a_region[2] &&
	    a_region[1] < crop_region[3] && crop_region[1] < a_region[3]) {
		s32 inter_sect[5];
		size_t data_count = 5;

		inter_sect[0] = max(a_region[0], crop_region[0]);
		inter_sect[1] = max(a_region[1], crop_region[1]);
		inter_sect[2] = min(a_region[2], crop_region[2]);
		inter_sect[3] = min(a_region[3], crop_region[3]);
		inter_sect[4] = a_region[4];

		amd_cam_update_tag(metadata, tag, (const void *)&inter_sect[0],
				   data_count, NULL);
	}
}

void update_3a(struct amd_cam_metadata *metadata, struct physical_cam *cam)
{
	size_t data_count = 1;
	u8 ae_state = cam->states.ae_state;
	u8 af_state = cam->states.af_state;
	u8 awb_state = cam->states.awb_state;
	u8 lens_state;

	amd_cam_update_tag(metadata, AMD_CAM_CONTROL_AE_STATE,
			   (const void *)&ae_state, data_count, NULL);

	amd_cam_update_tag(metadata, AMD_CAM_CONTROL_AF_STATE,
			   (const void *)&af_state, data_count, NULL);

	amd_cam_update_tag(metadata, AMD_CAM_CONTROL_AWB_STATE,
			   (const void *)&awb_state, data_count, NULL);

	switch (af_state) {
	case AMD_CAM_CONTROL_AF_STATE_PASSIVE_SCAN:
	case AMD_CAM_CONTROL_AF_STATE_ACTIVE_SCAN:
		lens_state = AMD_CAM_LENS_STATE_MOVING;
		break;
	case AMD_CAM_CONTROL_AF_STATE_INACTIVE:
	case AMD_CAM_CONTROL_AF_STATE_PASSIVE_FOCUSED:
	case AMD_CAM_CONTROL_AF_STATE_FOCUSED_LOCKED:
	case AMD_CAM_CONTROL_AF_STATE_NOT_FOCUSED_LOCKED:
	case AMD_CAM_CONTROL_AF_STATE_PASSIVE_UNFOCUSED:
	default:
		lens_state = AMD_CAM_LENS_STATE_STATIONARY;
		break;
	}

	amd_cam_update_tag(metadata, AMD_CAM_LENS_STATE,
			   (const void *)&lens_state, data_count, NULL);

	update_3a_region(AMD_CAM_CONTROL_AE_REGIONS, metadata);
	update_3a_region(AMD_CAM_CONTROL_AF_REGIONS, metadata);
	update_3a_region(AMD_CAM_CONTROL_AWB_REGIONS, metadata);
}

int metadata_update_sensor_timestamp(struct amd_cam_metadata *metadata,
				     u64 *timestamp)
{
	int ret = 0;
	size_t data_count = 1;
	struct amd_cam_metadata *cam_metadata =
		(struct amd_cam_metadata *)metadata;

	ret = amd_cam_update_tag
		(cam_metadata,
		AMD_CAM_SENSOR_TIMESTAMP,
		(const void *)timestamp,
		data_count, NULL);
	if (ret)
		loge("updation of sensor timestamp tag failed\n");

	return ret;
}

int streams_off(struct physical_cam *cam, struct kernel_stream_indexes *info,
	bool pause)
{
	int i = 0, cnt = 0;
	int ret = 0;
	struct cam_stream *s = NULL;
	struct isp_ctl_ctx ctx;
	struct isp_info *isp = v4l2_get_subdevdata(cam->isp_sd);

	logd("enter %s", __func__);
	cnt = info ? info->stream_cnt : MAX_STREAM_NUM_PER_PHYSICAL_CAMERA;
	logd("%s close %d streams", __func__, cnt);
	for (i = 0; i < cnt; i++) {
		if (info)
			s = cam->stream_tbl[info->stream_ids[i]];
		else
			s = cam->stream_tbl[i];

		if (s->state == STREAM_UNINIT) {
			logd("%s s_%d do none for not config/on",
				__func__, s->idx);
			continue;
		}

		if (stream_off(cam, s, pause)) {
			logd("%s s_%d stream off failed!", __func__, s->idx);
			return -1;
		}

		if (!pause) {
			// inform isp to del a stream
			if (!list_empty(&s->stream_list)) {
				mutex_lock(&isp->mutex);
				list_del(&s->stream_list);
				mutex_unlock(&isp->mutex);
			}

			if (alloc_vb2_buf_structure(s, 0)) {
				logd("%s s_%d destroy vb2 buf failed!",
					__func__, s->idx);
				return -1;
			}

			s->state = STREAM_UNINIT;
			if (s->cur_cfg.fmt.type == F_YUV)
				cam->yuv_s_cnt--;
		}

		logd("%s s_%d stream off suc! (pause is %d)",
			__func__, s->idx, pause);
	}

	if (cam->on_s_cnt == 0) {
		ctx.cam = cam;
		ctx.stream = NULL;
		ctx.ctx = NULL;
		ret = v4l2_subdev_call(cam->isp_sd, core, ioctl,
			ISP_IOCTL_STREAM_CLOSE, &ctx);
		if (ret)
			loge("%s failed to close ISP module [%d], ret %d",
				__func__, cam->sensor_id, ret);
		else
			logi("%s no stream for module %d, close it success!",
				__func__, cam->sensor_id);
	} else
		logi("%s module still has %d streams", __func__, cam->on_s_cnt);

	return ret;
}

int stream_off(struct physical_cam *cam, struct cam_stream *s, bool pause)
{
	int ret = 0;
	struct isp_ctl_ctx ctx;

	assert(s != NULL);
	logd("enter %s, stream[%d]", __func__, s->idx);
	if (s->state == STREAM_ACTIVE) {
		wait_event(s->done_wq, !atomic_read(&s->owned_by_stream_count));
		ctx.cam = NULL;
		ctx.stream = s;
		ctx.ctx = (void *) &pause;
		ret = v4l2_subdev_call(s->isp_sd, core, ioctl,
				ISP_IOCTL_STREAM_OFF, &ctx);
		if (ret) {
			loge("%s s_%d failed, ret %d", __func__, s->idx, ret);
			return ret;
		}

		logd("%s s_%d success!", __func__, s->idx);
		s->state = STREAM_PREPARING;
		ret = vb2_streamoff(&s->vidq, s->vidq.type);
		if (ret)
			loge("%s vb2 fail, ret %d", __func__, ret);

		cam->on_s_cnt--;
	} else {
		logd("isp has already stopped!, state = [%d]", s->state);
	}

	return ret;
}

/*
 * file operation for each physical camera
 */
static int amd_cam_open(struct file *f)
{
	int ret = 0;
	struct physical_cam *cam = NULL;
	struct video_device *dev = video_devdata(f);
	int cam_id = g_cam->cam_num;

	logd("enter %s", __func__);

	cam_id = find_first_zero_bit(&g_cam->cam_bit, g_cam->cam_num);
	if (cam_id >= g_cam->cam_num) {
		cam_id = 0;
		g_cam->cam_bit = 0;
		loge("already open all cams! re-open from: %d", cam_id);
	} else
		logd("%s: gcam->cam_bit: 0x%lx, camId: %d", __func__,
			g_cam->cam_bit, cam_id);

	g_cam->cam_bit |= 1 << cam_id;
	logd("%s: update gcam->cam_bit: 0x%lx", __func__, g_cam->cam_bit);

	cam = g_cam->phycam_tbl[cam_id];

	//mutex_init(&cam->c_mutex);
	v4l2_fh_init(&cam->fh, dev);
	f->private_data = &cam->fh;
	v4l2_fh_add(&cam->fh);

	logi("open physical camera [%d]", cam->sensor_id);
	return ret;
}

static int amd_cam_release(struct file *f)
{
	int ret = 0;
	struct v4l2_fh *fh = f->private_data;
	struct physical_cam *cam = container_of(fh, struct physical_cam, fh);

	logd("enter %s", __func__);
	// destroy all streams
	ret = streams_off(cam, NULL, false);
	//v4l2_fh_del(&cam->fh);
	//v4l2_fh_exit(&cam->fh);
	logi("%s amd close physical camera result %d", __func__, ret);
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
	/* TODO: we need check if this function is OK */
	.release = video_device_release,
};

struct physical_cam *physical_cam_create(int idx, struct v4l2_device *vdev)
{
	int i = 0;
	struct physical_cam *phycam;
	struct video_device *vd;
	struct cam_stream *s;

	pr_info("JJJJJ:%s enter", __func__);

	phycam = kzalloc(sizeof(*phycam), GFP_KERNEL);
	if (!phycam) {
		loge("%s:failed to alloc phycam", __func__);
		return NULL;
	}

	memset(phycam, 0x0, sizeof(*phycam));
	//spin_lock_init(&phycam->s_lock);
	phycam->on_s_cnt = 0;
	phycam->yuv_s_cnt = 0;
	phycam->dq_buf_cnt = 0;
	phycam->partial_results = 0;
	phycam->shutter_done = false;
	phycam->is_metadata_sent = false;
	phycam->shutter_timestamp = 0;
	phycam->isp_sd = get_isp_subdev();
	/* FIXME: get physical camera info from hw detection table */
	phycam->sensor_id = CAM_IDX_BACK;//CAM_IDX_FRONT_L;//
	phycam->states.prv_control_mode = AMD_CAM_CONTROL_MODE_OFF;
	phycam->states.prv_scene_mode =
	    AMD_CAM_CONTROL_SCENE_MODE_DISABLED;
	phycam->states.prv_ae_mode = AMD_CAM_CONTROL_AE_MODE_OFF;
	phycam->states.prv_af_mode = AMD_CAM_CONTROL_AF_MODE_OFF;
	phycam->states.ae_state = AMD_CAM_CONTROL_AE_STATE_INACTIVE;
	phycam->states.af_state = AMD_CAM_CONTROL_AF_STATE_INACTIVE;
	phycam->states.awb_state = AMD_CAM_CONTROL_AWB_STATE_INACTIVE;

	/* initialize video device structure */
	vd = video_device_alloc();
	if (!vd) {
		loge("%s: failed to alloc video_device", __func__);
		goto err_alloc;
	}

	phycam->dev = vd;
	*vd = amd_camera_template;
	//vd->debug = debug;
	vd->lock = NULL;//&phycam->c_mutex;
	vd->v4l2_dev = vdev;
	vd->index = idx;
	vd->device_caps = V4L2_CAP_VIDEO_CAPTURE | V4L2_CAP_READWRITE |
			V4L2_CAP_STREAMING;
	//video_set_drvdata(vd, phycam);

	if (video_register_device(vd, VFL_TYPE_GRABBER, VIDEO_DEV_BASE + idx)) {
		loge("%s: failed to register device", __func__);
		goto err_register;
	}

	for (i = 0; i < MAX_STREAM_NUM_PER_PHYSICAL_CAMERA; i++) {
		s = stream_create(phycam, i);
		if (!s)
			goto err_stream;
		phycam->stream_tbl[i] = s;
	}
	return phycam;

err_stream:
	loge("%s create stream %d fail!", __func__, i);
	destroy_all_stream(phycam);
err_register:
	video_device_release(vd);
err_alloc:
	kfree(phycam);
	phycam = NULL;
	return phycam;
}

struct cam_stream *stream_create(struct physical_cam *cam, int id)
{
	struct cam_stream *s;

	logd("enter %s create stream %d", __func__, id);
	s = kzalloc(sizeof(struct cam_stream), GFP_KERNEL);
	if (!s)
		goto fail_alloc;
	/* FIXME: do hardware initialization at the first time */
#ifdef USING_DMA_BUF
	s->buf_type = STREAM_BUF_DMA;
#else
	s->buf_type = STREAM_BUF_VMALLOC;
#endif
	//mutex_init(&s->s_mutex);
	//spin_lock_init(&s->iq_lock);
	//spin_lock_init(&s->q_lock);
	spin_lock_init(&s->ab_lock);
	spin_lock_init(&s->b_lock);
	INIT_LIST_HEAD(&s->active_buf);
	INIT_LIST_HEAD(&s->stream_list);
	s->idx = id;
	s->q_id = 0;
	s->dq_id = 0;
	s->isp_sd = get_isp_subdev();
	s->input_buf = NULL;
	atomic_set(&s->owned_by_stream_count, 0);
	init_waitqueue_head(&s->done_wq);
	if (stream_vidq_init(&s->vidq, s->buf_type, s)) {
		loge("%s: failed to init vidq for stream[%d]", __func__, id);
		goto fail_alloc;
	}

	s->state = STREAM_UNINIT;
	logd("%s create stream %d suc!", __func__, s->idx);
	return s;

fail_alloc:
	loge("%s create stream %d failed!", __func__, id);
	destroy_one_stream(s);
	s = NULL;
	return s;
}

int stream_vidq_init(struct vb2_queue *q, int buf_type, void *priv)
{
	int ret;

	logd("enter %s", __func__);
	if (!q)
		return -1;

	memset(q, 0, sizeof(struct vb2_queue));
	q->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	q->io_modes = VB2_MMAP | VB2_USERPTR | VB2_READ | VB2_DMABUF;
	q->timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_MONOTONIC;
	q->buf_struct_size = sizeof(struct stream_buf);
	q->drv_priv = priv;
	q->ops = &amd_qops;
	if (buf_type == STREAM_BUF_VMALLOC)
		q->mem_ops = &vb2_vmalloc_memops;
	else
		q->mem_ops = &amd_dmabuf_ops;
	ret = vb2_queue_init(q);
	return ret;
}

void physical_cam_destroy(struct physical_cam *cam)
{
	logd("enter %s", __func__);
	if (cam == NULL)
		return;

	cam->isp_sd = NULL;
	destroy_all_stream(cam);
	if (cam->dev) {
		// TODO: need add lock?
		video_unregister_device(cam->dev);
		video_device_release(cam->dev);
	}

	/* TODO: check other data member */
	kfree(cam);
	cam = NULL;
}

void destroy_all_stream(struct physical_cam *cam)
{
	int i = 0;
	struct cam_stream *stream;

	logd("enter %s module %d", __func__, cam->sensor_id);
	for (i = 0; i < MAX_STREAM_NUM_PER_PHYSICAL_CAMERA; i++) {
		stream = cam->stream_tbl[i];
		destroy_one_stream(stream);
		cam->stream_tbl[i] = NULL;
	}

	loge("%s delete streams done!", __func__);
}

void destroy_one_stream(struct cam_stream *stream)
{
	logd("enter %s", __func__);

	if (stream == NULL)
		return;

	/* TODO: check other data member */
	atomic_set(&stream->owned_by_stream_count, 0);
	wake_up_all(&stream->done_wq);
	vb2_queue_release(&stream->vidq);
	kfree(stream);
}

/*
 * Hardware detection
 */
static int detect_hardware(struct amd_cam *g_cam)
{
	int ret = 0;
	struct cam_hw_info *hw;

	logd("enter %s", __func__);

	hw = kzalloc(sizeof(*hw) * MAX_HW_NUM, GFP_KERNEL);
	if (!hw)
		return -ENOMEM;

	/*
	 * FIXME: Currently hard code here
	 */
	hw[0].id = 0;
	hw[0].type = ISP;
	/*
	 * FIXME: add priv info here
	 */
	g_cam->hw_tbl[0] = &hw[0];

	hw[1].id = 1;
	hw[1].type = SENSOR;
	g_cam->hw_tbl[1] = &hw[1];

	return ret;
}

static int register_hardware(struct cam_hw_info *hw)
{
	int ret = 0;
	struct v4l2_subdev *sd = &hw->sd;

	logd("enter %s", __func__);

	assert(sd != NULL && hw != NULL);

	if (hw->type == ISP)
		ret = register_isp_subdev(sd, &g_cam->v4l2_dev);

	/*
	 * FIXME: add more hardware here
	 */

	return ret;
}

static struct v4l2_subdev *get_isp_subdev(void)
{
	/*
	 * FIXME: it is only quirk
	 */
	return &g_cam->hw_tbl[0]->sd;
}

/* file operation */
static int isp_node_open(struct file *file)
{
	struct video_device *vdev = video_devdata(file);

	logd("enter %s", __func__);
	v4l2_fh_init(&g_cam->fh, vdev);
	file->private_data = &g_cam->fh;
	v4l2_fh_add(&g_cam->fh);

	return 0;
}

static int isp_node_release(struct file *file)
{
	struct v4l2_fh *fh = file->private_data;
	struct amd_cam *gcam = container_of(fh, struct amd_cam, fh);

	logd("enter %s", __func__);
	v4l2_fh_del(&gcam->fh);
	v4l2_fh_exit(&gcam->fh);

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
/*
 *	static int _ioctl_subscribe_event(struct v4l2_fh *fh,
 *			const struct v4l2_event_subscription *sub)
 *	{
 *		logd("enter %s", __func__);
 *		if (sub->type == V4L2_EVENT_CTRL)
 *			return v4l2_event_subscribe(fh, sub,
 *					32, &v4l2_ctrl_sub_ev_ops);
 *		else
 *			return v4l2_event_subscribe(fh, sub, 32, NULL);
 *	}
 */

long _isp_ioctl_default(struct file *file, void *fh,
		    bool valid_prio, unsigned int cmd, void *arg)
{
	struct kernel_hotplug_info *hot_plug_info;
	struct sensor_indexes *ids;
	struct s_info *info;

	logd("enter %s", __func__);
	switch (cmd) {
	case VIDIOC_HOTPLUG: {
		hot_plug_info = (struct kernel_hotplug_info *)arg;
		logd("VIDIOC_HOTPLUG");
		return simu_hotplugevent(hot_plug_info);
	}
	break;

	case VIDIOC_GET_SENSOR_LIST: {
		ids = (struct sensor_indexes *)arg;
		logd("VIDIOC_GET_SENSOR_LIST");
		/* FIXME: add real sensor id here */
		ids->sensor_cnt = 1;
		ids->sensor_ids[0] = 0;
	}
	break;

	case VIDIOC_GET_SENSOR_INFO: {
		info = (struct s_info *)arg;
		logd("VIDIOC_GET_SENSOR_INFO");
		return get_sensor_info(info);
	}
	break;

	default:
		pr_err("invalid ioctl\n");
		return -EINVAL;
	}
	return 0;
}

int simu_hotplugevent(struct kernel_hotplug_info *hot_plug_info)
{
	bool plug_state = hot_plug_info->hot_plug_state;

	logd("enter %s", __func__);
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

	if (plug_state) {
		kobject_uevent(kobj_ref, KOBJ_ADD);
		logd("%s camera add", __func__);
	} else {
		kobject_uevent(kobj_ref, KOBJ_REMOVE);
		logd("%s camera remove", __func__);
	}

	return 0;

 error_kobject:
	pr_info("Unable to send hotplug event\n");
	return -1;
}

int get_sensor_info(struct s_info *info)
{
	logd("enter %s", __func__);
	/*
	 * TODO: remove this when CVIP ready
	 */
	logd("sensor id is %d(0:IMX577)", info->sensor_id);

	switch (info->sensor_id) {
	case 0:
		get_imx577_info(info);
		break;
	default:
		loge("%s sensor id %d not support!", __func__, info->sensor_id);
		return -EINVAL;
	}

	return 0;
}

void get_imx577_info(struct s_info *info)
{
	unsigned int align_w;
	unsigned int align_h;

	strcpy(info->sensor_name, "IMX577");
	strcpy(info->sensor_manufacturer, "S0NY");
	strcpy(info->module_part_number, "0000");
	strcpy(info->module_manufacture, "O-Film");
	info->s_position = SENSOR_POSITION_FRONT_LEFT;
	info->m_ori = MODULE_ORIENTATION_270;

	info->pre_embedded_size = 0x100;
	info->post_embedded_size = 0x100;
	info->otp_data_size = 0;
	info->otp_buffer = NULL;
	info->apertures[0] = 0;
	info->filter_densities[0] = 0;
	info->focal_lengths[0] = 0;
	info->optical_stabilization[0] = OPTICAL_STABILIZATION_MODE_OFF;
	info->hyperfocal_distance = 0;
	info->min_focus_distance = 0;
	info->min_increment = 0;

	info->physical_size_w = 6287;
	info->physical_size_h = 4715;
	info->black_level_pattern[0] = 0x40;
	info->black_level_pattern[1] = 0x40;
	info->black_level_pattern[2] = 0x40;
	info->black_level_pattern[3] = 0x40;
	info->optical_black_regions_cnt = 0;
	info->test_pattern_modes_cnt = 2;
	info->test_pattern_modes[0] = TEST_PATTERN_MODE_OFF;
	info->test_pattern_modes[1] = TEST_PATTERN_MODE_COLOR_BARS;

	info->ae_type = AE_PROP_TYPE_SONY;
	info->min_expo_line = 1;
	info->max_expo_line = 65513;
	info->expo_line_alpha = 22;

	/*
	 * below again value is based on android standard, and FW standard is
	 * 1000-based, so need * 10 before send to FW
	 */
	info->min_again = 100;
	info->max_again = 2230;
	info->init_again = 100;
	info->shared_again = 0;

	info->formula.weight1 = 1024;
	info->formula.weight2 = 1024;
	info->formula.min_shift = 0;
	info->formula.max_shift = 0;
	info->formula.min_param = 0;
	info->formula.max_param = 978;

	/*
	 * below dgain value is based on android standard, and FW standard is
	 * 1000-based, so need * 10 before send to FW
	 */
	info->min_dgain = 100;
	info->max_dgain = 1600;

	info->base_iso = 100;
	info->modes_cnt = 6;

	info->modes[0].mode_id = PROFILE_NOHDR_2X2BINNING_3MP_15;
	info->modes[0].w = 2024;
	info->modes[0].h = 1520;
	align_w = SIZE_ALIGN_DOWN(info->modes[0].w);
	align_h = SIZE_ALIGN_DOWN(info->modes[0].h);
	info->modes[0].active_array_size_w = align_w;
	info->modes[0].active_array_size_h = align_h;
	info->modes[0].pixel_array_size_w = align_w;
	info->modes[0].pixel_array_size_h = align_h;
	info->modes[0].pre_correction_size_w = align_w;
	info->modes[0].pre_correction_size_h = align_h;
	info->modes[0].t_line = 35919;
	info->modes[0].frame_length_lines = 1856;
	info->modes[0].min_frame_rate = 15;
	info->modes[0].max_frame_rate = 15;
	info->modes[0].init_itime = 66;
	info->modes[0].bits_per_pixel = 10;
	info->modes[0].fmt_layout = PIXEL_COLOR_FORMAT_RGGB;
	info->modes[0].fmt_bit = PVT_IMG_FMT_RAW12;
	info->modes[0].hdr = HDR_MODE_INVALID;
	info->modes[0].bin = BINNING_MODE_2x2;

	info->modes[1].mode_id = PROFILE_NOHDR_2X2BINNING_3MP_30;
	info->modes[1].w = 2024;
	info->modes[1].h = 1520;
	align_w = SIZE_ALIGN_DOWN(info->modes[1].w);
	align_h = SIZE_ALIGN_DOWN(info->modes[1].h);
	info->modes[1].active_array_size_w = align_w;
	info->modes[1].active_array_size_h = align_h;
	info->modes[1].pixel_array_size_w = align_w;
	info->modes[1].pixel_array_size_h = align_h;
	info->modes[1].pre_correction_size_w = align_w;
	info->modes[1].pre_correction_size_h = align_h;
	info->modes[1].t_line = 9689;
	info->modes[1].frame_length_lines = 3440;
	info->modes[1].min_frame_rate = 30;
	info->modes[1].max_frame_rate = 30;
	info->modes[1].init_itime = 33;
	info->modes[1].bits_per_pixel = 10;
	info->modes[1].fmt_layout = PIXEL_COLOR_FORMAT_RGGB;
	info->modes[1].fmt_bit = PVT_IMG_FMT_RAW12;
	info->modes[1].hdr = HDR_MODE_INVALID;
	info->modes[1].bin = BINNING_MODE_2x2;

	info->modes[2].mode_id = PROFILE_NOHDR_2X2BINNING_3MP_60;
	info->modes[2].w = 2024;
	info->modes[2].h = 1520;
	align_w = SIZE_ALIGN_DOWN(info->modes[2].w);
	align_h = SIZE_ALIGN_DOWN(info->modes[2].h);
	info->modes[2].active_array_size_w = align_w;
	info->modes[2].active_array_size_h = align_h;
	info->modes[2].pixel_array_size_w = align_w;
	info->modes[2].pixel_array_size_h = align_h;
	info->modes[2].pre_correction_size_w = align_w;
	info->modes[2].pre_correction_size_h = align_h;
	info->modes[2].t_line = 9689;
	info->modes[2].frame_length_lines = 1720;
	info->modes[2].min_frame_rate = 60;
	info->modes[2].max_frame_rate = 60;
	info->modes[2].init_itime = 16;
	info->modes[2].bits_per_pixel = 10;
	info->modes[2].fmt_layout = PIXEL_COLOR_FORMAT_RGGB;
	info->modes[2].fmt_bit = PVT_IMG_FMT_RAW12;
	info->modes[2].hdr = HDR_MODE_INVALID;
	info->modes[2].bin = BINNING_MODE_2x2;

	info->modes[3].mode_id = PROFILE_NOHDR_V2BINNING_6MP_30;
	info->modes[3].w = 4048;
	info->modes[3].h = 1520;
	align_w = SIZE_ALIGN_DOWN(info->modes[3].w);
	align_h = SIZE_ALIGN_DOWN(info->modes[3].h);
	info->modes[3].active_array_size_w = align_w;
	info->modes[3].active_array_size_h = align_h;
	info->modes[3].pixel_array_size_w = align_w;
	info->modes[3].pixel_array_size_h = align_h;
	info->modes[3].pre_correction_size_w = align_w;
	info->modes[3].pre_correction_size_h = align_h;
	info->modes[3].t_line = 10416;
	info->modes[3].frame_length_lines = 3200;
	info->modes[3].min_frame_rate = 30;
	info->modes[3].max_frame_rate = 30;
	info->modes[3].init_itime = 33;
	info->modes[3].bits_per_pixel = 10;
	info->modes[3].fmt_layout = PIXEL_COLOR_FORMAT_RGGB;
	info->modes[3].fmt_bit = PVT_IMG_FMT_RAW12;
	info->modes[3].hdr = HDR_MODE_INVALID;
	info->modes[3].bin = BINNING_MODE_2_VERTICAL;

	info->modes[4].mode_id = PROFILE_NOHDR_V2BINNING_6MP_60;
	info->modes[4].w = 4048;
	info->modes[4].h = 1520;
	align_w = SIZE_ALIGN_DOWN(info->modes[4].w);
	align_h = SIZE_ALIGN_DOWN(info->modes[4].h);
	info->modes[4].active_array_size_w = align_w;
	info->modes[4].active_array_size_h = align_h;
	info->modes[4].pixel_array_size_w = align_w;
	info->modes[4].pixel_array_size_h = align_h;
	info->modes[4].pre_correction_size_w = align_w;
	info->modes[4].pre_correction_size_h = align_h;
	info->modes[4].t_line = 10416;
	info->modes[4].frame_length_lines = 1600;
	info->modes[4].min_frame_rate = 60;
	info->modes[4].max_frame_rate = 60;
	info->modes[4].init_itime = 16;
	info->modes[4].bits_per_pixel = 10;
	info->modes[4].fmt_layout = PIXEL_COLOR_FORMAT_RGGB;
	info->modes[4].fmt_bit = PVT_IMG_FMT_RAW12;
	info->modes[4].hdr = HDR_MODE_INVALID;
	info->modes[4].bin = BINNING_MODE_2_VERTICAL;

	info->modes[5].mode_id = PROFILE_NOHDR_NOBINNING_12MP_30;
	info->modes[5].w = 4056;
	info->modes[5].h = 3040;
	align_w = SIZE_ALIGN_DOWN(info->modes[5].w);
	align_h = SIZE_ALIGN_DOWN(info->modes[5].h);
	info->modes[5].active_array_size_w = align_w;
	info->modes[5].active_array_size_h = align_h;
	info->modes[5].pixel_array_size_w = align_w;
	info->modes[5].pixel_array_size_h = align_h;
	info->modes[5].pre_correction_size_w = align_w;
	info->modes[5].pre_correction_size_h = align_h;
	info->modes[5].t_line = 10742;
	info->modes[5].frame_length_lines = 3102;
	info->modes[5].min_frame_rate = 30;
	info->modes[5].max_frame_rate = 30;
	info->modes[5].init_itime = 33;
	info->modes[5].bits_per_pixel = 10;
	info->modes[5].fmt_layout = PIXEL_COLOR_FORMAT_RGGB;
	info->modes[5].fmt_bit = PVT_IMG_FMT_RAW12;
	info->modes[5].hdr = HDR_MODE_INVALID;
	info->modes[5].bin = BINNING_MODE_NONE;
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
	int ret = 0;
	struct video_device *video_dev = NULL;

	logd("enter %s", __func__);
	/* video device allocation */
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
	/* register ISP Node*/
	ret = video_register_device(video_dev, VFL_TYPE_GRABBER, 0);
	if (ret)
		goto error;

	kset_ref = kset_create_and_add("hotplug_camera", NULL, kernel_kobj);
	if (!kset_ref) {
		pr_info("Unable to allocate memory for kset_ref\n");
		goto out;
	}

	kobj_ref = kzalloc(sizeof(*kobj_ref), GFP_KERNEL);
	if (!kobj_ref)
		goto out_kset;

	kobj_ref->kset = kset_ref;
	if (kobject_init_and_add
		(kobj_ref, &ktype_ref, NULL, "%s", "camState")) {
		pr_info("Unable to initialize the kobj_ref\n");
		goto out_kobj;
	}

	pr_info("Device Driver Insert...Done!!!\n");
	return ret;
 error:
	if (video_dev)
		video_device_release(video_dev);
	return -1;
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
	int i = 3;
	int ret = 0;
	struct v4l2_subdev *sd;
	struct physical_cam *phycam;
	int physical_camera_cnt;

	logd("enter %s", __func__);

	/* collect hardware information */
	if (!g_cam) {
		g_cam = kzalloc(sizeof(*g_cam), GFP_KERNEL);
		if (!g_cam)
			return -ENOMEM;

		memset(g_cam, 0, sizeof(struct amd_cam));
		/* TODO: collect hardware information */
		detect_hardware(g_cam);
	}

	/* register v4l2 device */
	snprintf(g_cam->v4l2_dev.name, sizeof(g_cam->v4l2_dev.name),
		 "AMD-V4L2-ROOT");
	if (v4l2_device_register(NULL, &g_cam->v4l2_dev)) {
		loge("%s: failed to register v4l2 device", __func__);
		goto free_dev;
	}
	logd("amd camera v4l2 device registered");

	for (i = 0; i < MAX_HW_NUM; i++) {
		if (g_cam->hw_tbl[i]) {
			sd = &g_cam->hw_tbl[i]->sd;
			register_hardware(g_cam->hw_tbl[i]);
			/* FIXME: add error processing here */
		}
	}

	/* FIXME: get physical cameras count and info from hw detection table */
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

	logi("amd capture device driver probe success!");
	return 0;

clean_physical_cam:
	for (i = 0; i < physical_camera_cnt; i++)
		physical_cam_destroy(g_cam->phycam_tbl[i]);
	g_cam->cam_num = 0;
	v4l2_device_unregister(&g_cam->v4l2_dev);

free_dev:
	for (i = 0; i < MAX_HW_NUM; i++) {
		if (!g_cam->hw_tbl[i])
			continue;
		kfree(g_cam->hw_tbl[i]);
		g_cam->hw_tbl[i] = NULL;
	}

	kfree(g_cam);
	g_cam = NULL;
	return -1;
}

static int amd_capture_remove(struct platform_device *pdev)
{
	if (pdev)
		pdev = pdev;
	logd("enter %s", __func__);
	return 0;
}

static void amd_pdev_release(struct device *dev)
{}

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
	int ret;

	logi("enter %s", __func__);

	ret = platform_device_register(&amd_capture_dev);
	if (ret)
		return ret;

	ret = platform_driver_register(&amd_capture_drv);
	if (ret)
		loge("failed to register platform driver, err[%d]", ret);

	/* TODO: should not call directly, remove it later */
//	amd_capture_probe(NULL);
	return ret;
}

static void __exit amd_capture_exit(void)
{
	logi("enter %s", __func__);

	platform_driver_unregister(&amd_capture_drv);

	/* TODO: should not call directly, remove it later */
	amd_capture_remove(NULL);
}

module_init(amd_capture_init);
module_exit(amd_capture_exit);
