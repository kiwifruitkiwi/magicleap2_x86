/*
 * Copyright (C) 2019-2021 Advanced Micro Devices, Inc. All rights reserved.
 */

#include <linux/firmware.h>
#include <linux/workqueue.h>
#include <linux/timer.h>
#include <asm/cacheflush.h>
#include <linux/sched/clock.h>
#include "ipc.h"

static void simulate_prepost(struct isp_ipc_device *dev, int cam_id)
{
	struct camera *cam = &dev->camera[cam_id];
	struct isp_input_frame *frame;
	u32 presize, postsize;
	u8 *presrc, *postsrc, *predest, *postdest;
	int i;
	u64 us = local_clock() / 1000;

	if (!cam->data_base)
		return;

	/* fill in test data */
	cam->pre_data.expdata.itime = 15;
	cam->pre_data.expdata.again = 32;
	cam->pre_data.expdata.dgain = 3268;
	cam->pre_data.timestampinpre.readoutstarttimestampus = us++;
	cam->pre_data.timestampinpre.centroidtimestampus = us++;
	cam->pre_data.timestampinpre.seqWintimestampus = us++;
	cam->post_data.timestampinpost.readoutendtimestampus = us;

	presize = PREDATA_SIZE;
	postsize = POSTDATA_SIZE;
	presrc = cam->data_base;
	postsrc = cam->data_base + presize;

	memcpy(presrc, &cam->pre_data, presize);
	memcpy(postsrc, &cam->post_data, postsize);
	__flush_dcache_area(presrc, presize + postsize);

	for (i = 0; i < MAX_INPUT_FRAME; i++) {
		predest = presrc + i * (presize + postsize);
		postdest = postsrc + i * (presize + postsize);
		dev_dbg(dev->dev, "frame=%d presrc=0x%llx, predest=0x%llx, postsrc=0x%llx, postdest=0x%llx\n",
			i, (u64)presrc, (u64)predest,
			(u64)postsrc, (u64)postdest);
		memcpy(predest, presrc, presize);
		memcpy(postdest, postsrc, postsize);
		__flush_dcache_area(predest, presize + postsize);
		frame = &cam->input_frames[i];
		frame->predata_offset = i * (presize + postsize);
		frame->postdata_offset = frame->predata_offset + presize;
	}
}

static int simulate_inbuf(struct isp_ipc_device *dev, int cam_id)
{
	struct camera *cam = &dev->camera[cam_id];
	struct isp_input_frame *frame;
	u32 width, height, frame_size, slice_info;
	u8 *src, *dest;
	int i;
	int size;

	dev_dbg(dev->dev, "%s: setup input frame rings\n", __func__);

	/* copy sensor data from input_base
	 * create the input buffer queue
	 */
	src = cam->input_base;
	height = cam->cam_ctrl_info.res.height;
	width = cam->cam_ctrl_info.res.width;
	if (calculate_input_slice_info(cam) < 0) {
		pr_err("%s: calculate_input_slice_info error\n", __func__);
		return -1;
	}
	slice_info = ((cam->inslice.slice_height[SLICE_MID] <<
			INSLICEMID_SHIFT) & INSLICEMID_MASK) |
			((cam->inslice.slice_height[SLICE_FIRST] <<
			INSLICE0_SHIFT) & INSLICE0_MASK);
	dev_dbg(dev->dev, "width=%d, height=%d, bitdepth=%u, slice_info=0x%x\n",
		width, height, cam->cam_ctrl_info.strm.bd, slice_info);

	/* common work for stream on:
	 * - load golden image
	 * - setup output buffer rings
	 */
	size = isp_load_image(dev, cam_id, isp_get_img_path(width, height));
	if (size < 0) {
		pr_err("%s: isp_load_image error %d\n", __func__, size);
		return size;
	}
	frame_size = size;

	for (i = 0; i < MAX_INPUT_FRAME; i++) {
		dest = src + i * frame_size;
		dev_dbg(dev->dev, "frame=%d src=0x%llx, dest=0x%llx\n",
			i, (u64)src, (u64)dest);

		if (i == 1) {
			/* for the 2nd buffer:
			 * 1. flip image for android to construct moving pic
			 * 2. use default raw image for non-android case
			 */
			if (cam->isp_ioimg_setting.type == TYPE_UBUNTU) {
				memcpy(dest, src, frame_size);
			} else {
				memcpy(dest, src + frame_size / 2,
				       frame_size / 2);
				memcpy(dest + frame_size / 2, src,
				       frame_size / 2);
			}
		} else {
			if (i % 2)
				/* use 2nd buffer as src image */
				memcpy(dest, src + frame_size, frame_size);
			else
				/* use 1st buffer as src image */
				memcpy(dest, src, frame_size);
		}
		__flush_dcache_area(dest, frame_size);

		frame = &cam->input_frames[i];
		frame->width = width;
		frame->height = height;
		frame->bw = to_bit_width(cam->cam_ctrl_info.strm.bd);
		frame->exp0_offset = i * frame_size;
		frame->slice_info = slice_info;
		frame->cur_slice = 0;
		frame->total_slices = cam->isp_ioimg_setting.raw_slice_num;
	}

	return 0;
}

static void isp_timer_proc(struct isp_ipc_device *dev, int id)
{
	u32 data = 0;
	u8 idx = 0;
	struct isp_input_frame *frame;
	struct camera *cam = &dev->camera[id];

	//set the data accordingly
	/* update input buff frame offset for the first slice */
	idx = cam->sensor_frame % MAX_INPUT_FRAME;
	frame = &cam->input_frames[idx];

	if (frame->cur_slice > 0 &&
	    frame->cur_slice < frame->total_slices) {
		if (test_bit(ISP_CERR_FW_TOUT_WAIT_DROP,
			     &cam->test_condition)) {
			/* stop sending new slices */
			if (cam->terr_base.drop_frame == FRAMEID_INVALID)
				cam->terr_base.drop_frame = frame->frame_id;
			if (cam->terr_base.drop_frame == frame->frame_id)
				goto update_id;
		} else if (test_bit(ISP_CERR_DROP_FRAME_WAIT,
				    &cam->test_condition)) {
			if (cam->terr_base.drop_frame == FRAMEID_INVALID) {
				cam->terr_base.drop_frame = frame->frame_id;
				writel_relaxed(frame->frame_id, dev->base
					       + CAM0_CVP_INBUF_DROP_ID + id
					       * CAM_INBUF_DROP_ID_REG_SIZE);
				smp_wmb(); /* memory barrier */
			}
			if (cam->terr_base.drop_frame == frame->frame_id)
				goto update_id;
		}
	}

	if (frame->cur_slice == 0) {
		dev_dbg(dev->dev, "frame info setup 0x%x\n", frame->frame_id);

		/* error handling: generating out-of-order frame */
		if (frame->frame_id > 2 &&
		    (frame->frame_id < (FRAMEID_MAX - 2))) {
			if (test_bit(ISP_CERR_DISORDER_WAIT_SKIP,
				     &cam->test_condition)) {
				if (cam->terr_base.drop_frame ==
				    FRAMEID_INVALID) {
					/* e.g. step1: skip frame6 */
					cam->terr_base.drop_frame =
						frame->frame_id;
					frame->cur_slice =
						frame->total_slices - 1;
					goto update_id;
				} else if (cam->terr_base.recover_frame ==
					   FRAMEID_INVALID) {
					/* e.g. step2: send frame7 */
					cam->terr_base.recover_frame =
						frame->frame_id;
				} else {
					/* e.g. step3: send frame6,
					 * which will be dropped by isp fw
					 */
					clear_bit(ISP_CERR_DISORDER_WAIT_SKIP,
						  &cam->test_condition);
					data = cam->terr_base.drop_frame;
					frame->cur_slice =
						frame->total_slices - 1;
					goto update_id1;
				}
			} else if (test_bit(ISP_CERR_DISORDER_WAIT_DROP,
					    &cam->test_condition)) {
				if (cam->terr_base.recover_frame ==
				    frame->frame_id) {
					/* e.g. step4: do not resend frame7 */
					frame->cur_slice =
						frame->total_slices - 1;
					goto update_id;
				}
			}
		}

		if (dev->test_config == ISP_HW_MODE) {
			/* setup initial frame info */
			writel_relaxed(frame->exp0_offset, dev->base
				       + id * CAM_INBUFF_PIXEL_REG_SIZE
				       + CAM0_CVP_INBUF_EXP0_ADDR_OFFSET);
			writel_relaxed((frame->bw << 28)
				       + (frame->height << 14) + frame->width,
				       dev->base
				       + id * CAM_INBUFF_PIXEL_REG_SIZE
				       + CAM0_CVP_INBUF_IMG_SIZE);
			writel_relaxed(frame->slice_info,
				       dev->base +
				       id * CAM_INBUFF_PIXEL_REG_SIZE +
				       CAM0_CVP_INBUF_SLICE_INFO);
			if (cam->cam_ctrl_resp.resp.prepost_mem ==
			    RESP_SUCCEED) {
				/* pre data */
				simulate_prepost(dev, id);
				writel_relaxed(frame->predata_offset, dev->base
					       + id * CAM_INBUFF_DATA_REG_SIZE
					       + CAM0_CVP_PREDAT_ADDR_OFFSET);
				writel_relaxed(frame->frame_id, dev->base
					       + id * CAM_INBUFF_DATA_REG_SIZE
					       + CAM0_CVP_PREDAT_FRM_ID);
			}
			smp_wmb(); /* memory barrier */
		}
	}

	data = (frame->cur_slice << 28) + frame->frame_id;

update_id1:
	if (frame->cur_slice < frame->total_slices &&
	    dev->test_config == ISP_HW_MODE) {
		writel_relaxed(data,
			       dev->base + id * CAM_INBUFF_PIXEL_REG_SIZE
			       + CAM0_CVP_INBUF_EXP0_FRM_SLICE);
		smp_wmb(); /* memory barrier */
	}

	if (frame->cur_slice == (frame->total_slices - 1) &&
	    dev->test_config == ISP_HW_MODE) {
		/* post data */
		if (cam->cam_ctrl_resp.resp.prepost_mem == RESP_SUCCEED) {
			simulate_prepost(dev, id);
			writel_relaxed(frame->postdata_offset, dev->base
				       + id * CAM_INBUFF_DATA_REG_SIZE
				       + CAM0_CVP_POSTDAT_ADDR_OFFSET);
			writel_relaxed(frame->frame_id, dev->base
				       + id * CAM_INBUFF_DATA_REG_SIZE
				       + CAM0_CVP_POSTDAT_FRM_ID);
		}
		smp_wmb(); /* memory barrier */
	}

update_id:
	frame->cur_slice++;
	if (frame->cur_slice >= frame->total_slices) {
		int sensor_frame = cam->sensor_frame;

		/* roll back case */
		if (sensor_frame < cam->isp_input_frame)
			sensor_frame += MAX_INPUT_FRAME;

		/* not to increase cur_slice to avoid rolling back */
		frame->cur_slice = frame->total_slices;

		/* check if pre/post data timeout */
		if ((sensor_frame - cam->isp_predata_frame)
		    >= (MAX_INPUT_FRAME - 1))
			dev_dbg(dev->dev, "Sensor predata overrun\n");
		if ((sensor_frame - cam->isp_postdata_frame)
		    >= (MAX_INPUT_FRAME - 1))
			dev_dbg(dev->dev, "Sensor postdata overrun\n");

		/* if input frame ring is not full, increase the frame number
		 * prepare the new frame info
		 * else report timeout error
		 */
		if ((sensor_frame - cam->isp_input_frame)
		    < (MAX_INPUT_FRAME - 1)) {
			cam->sensor_frame++;
			dev_dbg(dev->dev, "new sensor frame logged 0x%x\n",
				cam->sensor_frame);
			if (cam->sensor_frame > FRAMEID_MAX)
				cam->sensor_frame = STARTING_FRAME_NUMBER;
			idx = cam->sensor_frame % MAX_INPUT_FRAME;
			frame = &cam->input_frames[idx];
			frame->frame_id = cam->sensor_frame;
			frame->cur_slice = 0;
		} else {
			dev_err(dev->dev, "Sensor ring buffer overrun\n");
		}
	}
}

/*
 * in order to achieve a more accurate FPS, it's necessary to adjust the fake
 * frequency, because this is a SW fake sensor solution, some extra time will
 * be wasted somehow.
 * adjust for 30FPS and 60FPS, which are mandatory required.
 * adjust only for specific slice number: 1, 4, 16
 * unit: jiffies
 */
static const int sub[2][3] = {
	{10, 4, 2}, /* 30FPS: 1, 4, 16 */
	{7, 2, 2}   /* 60FPS: 1, 4, 16 */
};

static inline int cal_interval(struct camera *cam)
{
	int fps = cam->cam_ctrl_info.fpks / 1000;
	int j = usecs_to_jiffies(USEC_PER_SEC / fps /
				 cam->isp_ioimg_setting.raw_slice_num);

	switch (fps) {
	case 30:
		switch (cam->isp_ioimg_setting.raw_slice_num) {
		case 1: return (j - sub[0][0]);
		case 4: return (j - sub[0][1]);
		case 16: return (j - sub[0][2]);
		default: return (j - 1);
		}
	case 60:
		switch (cam->isp_ioimg_setting.raw_slice_num) {
		case 1: return (j - sub[1][0]);
		case 4: return (j - sub[1][1]);
		case 16: return (j - sub[1][2]);
		default: return (j - 1);
		}
	default:
		return (j - 1);
	}
}

static void isp_timer0func(struct timer_list *t)
{
	struct camera *cam = from_timer(cam, t, isp_data_timer);
	struct isp_ipc_device *dev =
			container_of(cam, struct isp_ipc_device, camera[0]);

	if (cam->cam_ctrl_info.strm.stream != STREAM_ON)
		return;

	mod_timer(&cam->isp_data_timer, jiffies + cal_interval(cam));
	isp_timer_proc(dev, 0);
}

static void isp_timer1func(struct timer_list *t)
{
	struct camera *cam = from_timer(cam, t, isp_data_timer);
	struct isp_ipc_device *dev =
			container_of(cam, struct isp_ipc_device, camera[1]);

	if (cam->cam_ctrl_info.strm.stream != STREAM_ON)
		return;

	mod_timer(&cam->isp_data_timer, jiffies + cal_interval(cam));
	isp_timer_proc(dev, 1);
}

static void streamon_work_proc(struct isp_ipc_device *dev, int cam_id)
{
	struct camera *cam = &dev->camera[cam_id];

	if (simulate_inbuf(dev, cam_id))
		return;

	simulate_prepost(dev, cam_id);

	mod_timer(&cam->isp_data_timer, jiffies + cal_interval(cam));
}

static void streamon_work0func(struct work_struct *work)
{
	struct isp_ipc_device *dev =
	  container_of(work, struct isp_ipc_device, camera[0].streamon_work);

	streamon_work_proc(dev, 0);
}

static void streamon_work1func(struct work_struct *work)
{
	struct isp_ipc_device *dev =
	  container_of(work, struct isp_ipc_device, camera[1].streamon_work);

	streamon_work_proc(dev, 1);
}

static inline void reset_status(struct isp_ipc_device *dev, int cam_id)
{
	struct camera *cam = &dev->camera[cam_id];
	int i;

	/* reset counters */
	cam->sensor_frame = STARTING_FRAME_NUMBER;
	cam->isp_input_frame = STARTING_FRAME_NUMBER;
	cam->isp_predata_frame = STARTING_FRAME_NUMBER;
	cam->isp_postdata_frame = STARTING_FRAME_NUMBER;
	cam->isp_output_frame = STARTING_FRAME_NUMBER;
	cam->isp_stat_frame = STARTING_FRAME_NUMBER;

	/* reset input frame index */
	for (i = 1; i < MAX_INPUT_FRAME; i++)
		cam->input_frames[i].frame_id = i;
}

void sensor_simulation(struct isp_ipc_device *dev, enum isp_camera cam_id,
		       enum reg_stream_status mode, bool resume)
{
	void (*tfunc)(struct timer_list *t);
	work_func_t wfunc;
	struct camera *cam = &dev->camera[cam_id];

	pr_debug("%s: cam%d stream %s\n", __func__, cam_id,
		 (mode == STREAM_ON) ? (resume ? "resume" : "on") :
		 (mode == STREAM_OFF) ? "off" :
		 (mode == STREAM_STDBY) ? "standby" : "NA");

	if (!cam->fake_sensor_initialised) {
		if (cam_id == CAM0) {
			tfunc = isp_timer0func;
			wfunc = streamon_work0func;
		} else {
			tfunc = isp_timer1func;
			wfunc = streamon_work1func;
		}
		timer_setup(&cam->isp_data_timer, tfunc, 0);
		INIT_WORK(&cam->streamon_work, wfunc);
		cam->fake_sensor_initialised = true;
	}

	switch (mode) {
	case STREAM_ON:
		queue_work(system_long_wq, &cam->streamon_work);
		break;
	case STREAM_STDBY:
		if (!resume)
			reset_status(dev, cam_id);
	/* fallthrough */
	case STREAM_OFF:
		cancel_work_sync(&cam->streamon_work);
		del_timer(&cam->isp_data_timer);
		break;
	default:
		return;
	}
}

