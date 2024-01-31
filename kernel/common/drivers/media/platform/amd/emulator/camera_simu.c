/*
 * camera_simu.c
 *
 * Copyright (C) 2020 Advanced Micro Devices, Inc. All rights reserved.
 *
 * This file is licensed under the terms of the GNU General Public License
 * version 2. This program is licensed "as is" without any warranty of any
 * kind, whether express or implied.
 *
 */

#include <linux/errno.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/videodev2.h>
#include <media/v4l2-common.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-device.h>
#include <media/v4l2-ioctl.h>
#include <media/v4l2-event.h>
#include <media/videobuf2-core.h>
#include <media/videobuf2-v4l2.h>
#include <media/videobuf2-vmalloc.h>
#include <linux/cam_api_simulation.h>
#include <linux/list.h>
#include <linux/amd_cam_metadata.h>

#include <linux/kobject.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/kdev_t.h>
#include <linux/uaccess.h>
#include <linux/random.h>

#include "camera_simulation.h"

#define SEC_TO_NANO_SEC(num) ((num) * 1000000000)

struct cam_handle {
	struct v4l2_fh fh;
};

struct buf_indexes {
	int free_buff_count;
	int free_index;
	int max_buffers;
};

struct cam_stream {
	struct vb2_queue bq;
	struct v4l2_format fmt;
	struct buf_indexes buf_status;
	int stream_id;
	bool stream_on;
	struct list_head done_list;
	struct list_head shutter_list;
	spinlock_t done_lock;	/*spinlock for stream done list */
	spinlock_t shutter_lock;	/*spinlock for stream shutter list */
	spinlock_t lock;	/*spinlock for stream */
	struct stream_info *str_done[MAX_BUFFERS_PER_STREAM];
	struct shutter_info *str_shutter[MAX_BUFFERS_PER_STREAM];
	atomic_t owned_by_stream_count;	/*Buffers owned by stream */
	wait_queue_head_t done_wq;
	struct mutex mutex;	/*Mutex for serialising the vb2_queue access */
};

struct shutter_info {
	struct list_head shutter_entry;
	__u32 combined_index;
	int num_of_bufs;
};

struct stream_info {
	struct list_head entry;
	__u32 combined_index;
	__u32 virt_cam_id;
	int num_of_bufs;
	unsigned long priv;
	unsigned long bufhandle;
	unsigned long metadata;
};

struct camera_simu {
	struct v4l2_device v4l2_dev;
	struct video_device *video_dev;
	int ref_count;
	int num_buffers;
	int num_of_reqs;
	bool is_shutter_event;
	bool is_metadata_sent;
	struct cam_stream *streams[MAX_STREAM_NUM_PER_PHYSICAL_CAMERA];
	spinlock_t lock;	/*spinlock for camera_simu object */
	int partial_results;
	u64 shutter_timestamp;
	struct algo_3a_states states;
	struct s_properties properties;
};

static struct camera_simu *g_cam;

#ifdef CONFIG_AMD_SIMU_CAMERA_ENABLE_UEVENT
struct kset *simu_kset_ref;
struct kobject *simu_kobj_ref;
struct kobj_type simu_ktype_ref;
#endif

/** Simulation AE constants */
const int pre_capture_min_frames = 10;
const int stable_ae_max_frames = 100;

int ae_counter;
bool ae_trigger_start;

static int _vb2_ops_queue_setup(struct vb2_queue *q,
				unsigned int *num_buffers,
				unsigned int *num_planes, unsigned int sizes[],
				struct device *alloc_devs[])
{
	struct cam_stream *stream = container_of(q, struct cam_stream, bq);
	struct v4l2_format *fmt = &stream->fmt;

	pr_info("simucamera: q-setup: type[%d], fmt[%d]\n", fmt->type,
		fmt->fmt.pix.pixelformat);
	switch (fmt->type) {
	case V4L2_BUF_TYPE_VIDEO_CAPTURE:
		if (fmt->fmt.pix.pixelformat == V4L2_PIX_FMT_NV12) {
			sizes[0] = fmt->fmt.pix.width *
			    (fmt->fmt.pix.height +
			     (fmt->fmt.pix.height + 1) / 2);
			*num_planes = 1;
		} else if (fmt->fmt.pix.pixelformat == V4L2_PIX_FMT_SRGGB16) {
			sizes[0] = fmt->fmt.pix.width * fmt->fmt.pix.height * 2;
			*num_planes = 1;
		} else if (fmt->fmt.pix.pixelformat == V4L2_PIX_FMT_Y8I) {
			sizes[0] = fmt->fmt.pix.width * fmt->fmt.pix.height;
			*num_planes = 1;
		} else {
			pr_err("simucamera: q-setup: invalid fmt[%d]\n",
			       fmt->fmt.pix.pixelformat);

			return -EINVAL;
		}
		break;

	default:
		pr_err("simucamera: q-setup: invalid type[%d]\n", fmt->type);

		return -EINVAL;
	}
	alloc_devs[0] = NULL;

	return 0;
}

static void _vb2_ops_buf_queue(struct vb2_buffer *vb)
{
	//To-Do
}

static const struct vb2_ops _vb2_ops = {
	.queue_setup = _vb2_ops_queue_setup,
	.buf_queue = _vb2_ops_buf_queue,
	.wait_prepare = vb2_ops_wait_prepare,
	.wait_finish = vb2_ops_wait_finish,
};

void release_resources(void)
{
	struct cam_stream *stream = NULL;
	struct stream_info *str_done = NULL;
	struct shutter_info *shutter = NULL;
	unsigned long flags;
	int i = 0, j = 0;

	for (i = 0; i < MAX_STREAM_NUM_PER_PHYSICAL_CAMERA; i++) {
		pr_info("simucamera: release-resources: stream [%d]\n", i);

		stream = g_cam->streams[i];
		if (!stream) {
			pr_warn("simucamera: release-resources");
			pr_warn("stream[%d] - invalid stream", i);
			continue;
		}

		spin_lock_irqsave(&stream->lock, flags);
		for (j = 0; j < MAX_BUFFERS_PER_STREAM; j++) {
			str_done = stream->str_done[j];
			if (!str_done) {
				pr_warn("simucamera: release-resources");
				pr_warn("stream[%d], buf[%d] - stream_done",
					i, j);
				continue;
			}
			stream->str_done[j] = NULL;
			kfree(str_done);

			shutter = stream->str_shutter[j];
			if (!shutter) {
				pr_warn("simucamera: release-resources");
				pr_warn("stream[%d], buf[%d] - shutter", i, j);
				continue;
			}
			stream->str_shutter[j] = NULL;
			kfree(shutter);
		}
		kfree(stream);
		spin_unlock_irqrestore(&stream->lock, flags);

	}
}

void wait_for_all_buffer_results(struct cam_stream *stream)
{
	pr_debug
	    ("simucamera: %s: stream[%d], buffer_count[%d]\n", __func__,
	     stream->stream_id, atomic_read(&stream->owned_by_stream_count));
	wait_event(stream->done_wq,
		   !atomic_read(&stream->owned_by_stream_count));
	pr_info("simucamera: all buffers of stream[%d] are returned",
		stream->stream_id);
}

/* file operation */
static int camera_simu_open(struct file *file)
{
	struct cam_stream *stream = NULL;
	struct cam_handle *cam = NULL;
	struct stream_info *str_done = NULL;
	struct shutter_info *shutter = NULL;
	struct video_device *vdev = video_devdata(file);
	int num_of_streams = MAX_STREAM_NUM_PER_PHYSICAL_CAMERA;
	int ref_count;
	unsigned long flags;

	cam = kzalloc(sizeof(*cam), GFP_KERNEL);
	if (!cam)
		return -ENOMEM;

	spin_lock_irqsave(&g_cam->lock, flags);
	ref_count = g_cam->ref_count++;

	pr_info("simucamera: open: ref[%d]\n", ref_count);

	if (ref_count == 0) {
		int i, j;

		memset(&g_cam->properties, 0, sizeof(struct s_properties));

		//default 3a states and modes
		g_cam->states.prv_control_mode = AMD_CAM_CONTROL_MODE_OFF;
		g_cam->states.prv_scene_mode =
		    AMD_CAM_CONTROL_SCENE_MODE_DISABLED;
		g_cam->states.prv_ae_mode = AMD_CAM_CONTROL_AE_MODE_OFF;
		g_cam->states.prv_af_mode = AMD_CAM_CONTROL_AF_MODE_OFF;

		g_cam->states.ae_state = AMD_CAM_CONTROL_AE_STATE_INACTIVE;
		g_cam->states.af_state = AMD_CAM_CONTROL_AF_STATE_INACTIVE;
		g_cam->states.awb_state = AMD_CAM_CONTROL_AWB_STATE_INACTIVE;
		ae_trigger_start = false;

		for (i = 0; i < num_of_streams; i++) {
			int ret;

			stream = kzalloc(sizeof(*stream), GFP_KERNEL);
			if (!stream)
				goto error;

			pr_info("simucamera: open: allocated stream[%d]\n", i);

			stream->stream_id = i;
			stream->stream_on = false;
			stream->bq.ops = &_vb2_ops;
			stream->bq.mem_ops = &vb2_vmalloc_memops;
			stream->bq.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			stream->bq.io_modes =
			    VB2_MMAP | VB2_USERPTR | VB2_DMABUF;
			stream->bq.timestamp_flags =
			    V4L2_BUF_FLAG_TIMESTAMP_MONOTONIC;

			mutex_init(&stream->mutex);
			stream->bq.lock = &stream->mutex;

			INIT_LIST_HEAD(&stream->done_list);
			INIT_LIST_HEAD(&stream->shutter_list);

			ret = vb2_queue_init(&stream->bq);
			if (ret) {
				pr_err
				    ("simucamera open VB2 queue init fail[%d]",
				     ret);
				goto error;
			}

			g_cam->streams[i] = stream;

			for (j = 0; j < MAX_BUFFERS_PER_STREAM; j++) {
				str_done =
				    kzalloc(sizeof(*str_done), GFP_KERNEL);
				if (!str_done) {
					pr_err("simucamera: open");
					pr_err("allocate str_done[%d] fail", j);
					goto error;
				}
				stream->str_done[j] = str_done;

				shutter = kzalloc(sizeof(*shutter), GFP_KERNEL);
				if (!shutter) {
					pr_err("simucamera: open");
					pr_err("allocate shutter[%d] fail", j);
					goto error;
				}
				stream->str_shutter[j] = shutter;
			}

			atomic_set(&stream->owned_by_stream_count, 0);
			init_waitqueue_head(&stream->done_wq);
			pr_info
			    ("simucamera: open allocated str_done and shutter");
		}
	}
	v4l2_fh_init(&cam->fh, vdev);
	file->private_data = &cam->fh;
	v4l2_fh_add(&cam->fh);
	spin_unlock_irqrestore(&g_cam->lock, flags);

	return 0;
 error:
	release_resources();
	g_cam->ref_count--;
	spin_unlock_irqrestore(&g_cam->lock, flags);
	kfree(cam);

	return -ENOMEM;
}

static int camera_simu_release(struct file *file)
{
	struct v4l2_fh *fh = file->private_data;
	struct cam_handle *cam = container_of(fh, struct cam_handle, fh);
	struct cam_stream *stream = NULL;
	struct stream_info *str_done = NULL;
	struct shutter_info *shutter = NULL;
	struct kernel_stream_indexes stream_off;
	int num_of_streams = MAX_STREAM_NUM_PER_PHYSICAL_CAMERA;
	unsigned long flags;
	int i, ref_count;

	stream_off.stream_cnt = num_of_streams;
	for (i = 0; i < num_of_streams; i++) {
		stream_off.stream_ids[i] = i;
	}
	ioctl_simu_streamdestroy(file, &stream_off);

	spin_lock_irqsave(&g_cam->lock, flags);
	g_cam->is_shutter_event = false;
	g_cam->is_metadata_sent = false;
	g_cam->num_of_reqs = 0;
	g_cam->shutter_timestamp = 0;

	ref_count = --g_cam->ref_count;
	pr_info("simucamera: release: ref[%d]\n", g_cam->ref_count);

	//default 3a states and modes
	g_cam->states.prv_control_mode = AMD_CAM_CONTROL_MODE_OFF;
	g_cam->states.prv_scene_mode = AMD_CAM_CONTROL_SCENE_MODE_DISABLED;
	g_cam->states.prv_ae_mode = AMD_CAM_CONTROL_AE_MODE_OFF;
	g_cam->states.prv_af_mode = AMD_CAM_CONTROL_AF_MODE_OFF;

	g_cam->states.ae_state = AMD_CAM_CONTROL_AE_STATE_INACTIVE;
	g_cam->states.af_state = AMD_CAM_CONTROL_AF_STATE_INACTIVE;
	g_cam->states.awb_state = AMD_CAM_CONTROL_AWB_STATE_INACTIVE;
	spin_unlock_irqrestore(&g_cam->lock, flags);

	if (ref_count == 0) {
		int j;

		for (i = 0; i < num_of_streams; i++) {
			spin_lock_irqsave(&g_cam->lock, flags);

			stream = g_cam->streams[i];
			g_cam->streams[stream->stream_id] = NULL;
			spin_unlock_irqrestore(&g_cam->lock, flags);

			spin_lock_irqsave(&stream->lock, flags);
			vb2_queue_release(&stream->bq);

			stream->bq.lock = NULL;
			mutex_destroy(&stream->mutex);

			INIT_LIST_HEAD(&stream->done_list);
			INIT_LIST_HEAD(&stream->shutter_list);

			for (j = 0; j < MAX_BUFFERS_PER_STREAM; j++) {
				str_done = stream->str_done[j];
				stream->str_done[j] = NULL;
				kfree(str_done);

				shutter = stream->str_shutter[j];
				stream->str_shutter[j] = NULL;
				kfree(shutter);

			}

			kfree(stream);

			spin_unlock_irqrestore(&stream->lock, flags);

			pr_info("simucamera: release: freed stream[%d]", i);
			pr_info("str_done and shutter\n");
		}
	}

	v4l2_fh_del(&cam->fh);
	v4l2_fh_exit(&cam->fh);

	kfree(cam);

	return 0;
}

static const struct v4l2_file_operations file_fops = {
	.owner = THIS_MODULE,
	.open = camera_simu_open,
	.release = camera_simu_release,
	.unlocked_ioctl = video_ioctl2,
#ifdef CONFIG_COMPAT
	.compat_ioctl32 = video_ioctl2,
#endif
	.mmap = NULL,
	.poll = NULL,
};

static int _ioctl_subscribe_event(struct v4l2_fh *fh,
				  const struct v4l2_event_subscription *sub)
{
	if (sub->type == V4L2_EVENT_CTRL)
		return v4l2_event_subscribe(fh, sub, 32, &v4l2_ctrl_sub_ev_ops);
	else
		return v4l2_event_subscribe(fh, sub, 32, NULL);
}

int get_free_index(struct cam_stream *stream)
{
	int temp_index = -1;

	struct buf_indexes *idx_info = &stream->buf_status;

	if (idx_info->free_buff_count > 0) {
		if (idx_info->free_index >= idx_info->max_buffers)
			idx_info->free_index = 0;
		temp_index = idx_info->free_index;
		idx_info->free_buff_count--;
		idx_info->free_index++;
	}
	pr_debug("simucamera: stream[%d], free index[%d]\n", stream->stream_id,
		 temp_index);

	return temp_index;
}

int process_simu_3a(struct amd_cam_metadata *metadata)
{
	int res;
	u8 control_mode;
	struct amd_cam_metadata_entry entry;
	struct algo_3a_states *states_3a = &g_cam->states;

	res =
	    amd_cam_find_metadata_entry(metadata, AMD_CAM_CONTROL_MODE, &entry);
	if (res) {
		pr_err("simucamera: 3A: CONTROL_MODE tag not found\n");
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
		update_simu_3a(metadata);
		return 0;
	}
	// if controlMode == AUTO,
	// Process individual 3A controls
	res = do_simu_ae(metadata);
	if (res) {
		pr_err("simucamera: 3A: do_simu_ae failed[%d]\n", res);
		return res;
	}
	// As we do not support AF, ignoring the doSimuAF return value.
	res = do_simu_af(metadata);
	if (res) {
		pr_err("simucamera: 3A: do_simu_af failed[%d]\n", res);
		return res;
	}

	res = do_simu_awb(metadata);
	if (res) {
		pr_err("simucamera: 3A: do_simu_awb failed[%d]\n", res);
		return res;
	}

	update_simu_3a(metadata);

	//Storing the mode values for next usage
	g_cam->states.prv_control_mode = control_mode;

	if (control_mode == AMD_CAM_CONTROL_MODE_USE_SCENE_MODE) {
		res =
		    amd_cam_find_metadata_entry(metadata,
						AMD_CAM_CONTROL_SCENE_MODE,
						&entry);
		if (res) {
			pr_err
			    ("simucamera: 3A CONTROL_SCENE_MODE tag not found");
			return res;
		}
		g_cam->states.prv_scene_mode = entry.data.u8[0];
	}

	res =
	    amd_cam_find_metadata_entry(metadata, AMD_CAM_CONTROL_AE_MODE,
					&entry);
	if (res) {
		pr_err("simucamera: 3A: CONTROL_AE_MODE tag not found\n");
		return res;
	}
	g_cam->states.prv_ae_mode = entry.data.u8[0];

	res =
	    amd_cam_find_metadata_entry(metadata, AMD_CAM_CONTROL_AF_MODE,
					&entry);
	if (res) {
		pr_err("simucamera: 3A: CONTROL_AF_MODE tag not found\n");
		return res;
	}
	g_cam->states.prv_af_mode = entry.data.u8[0];

	return res;
}

int do_simu_ae(struct amd_cam_metadata *metadata)
{
	int res;
	u8 control_mode, ae_mode, ae_state;
	u8 prv_control_mode;
	u8 precapture_trigger_val = AMD_CAM_CONTROL_AE_PRECAPTURE_TRIGGER_IDLE;
	bool ae_locked = false, mode_switch = false;
	bool precapture_trigger = false;
	struct amd_cam_metadata_entry entry;
	struct algo_3a_states *states_3a = &g_cam->states;

	res =
	    amd_cam_find_metadata_entry(metadata, AMD_CAM_CONTROL_MODE, &entry);
	if (res) {
		pr_err("simucamera: AE: CONTROL_MODE tag not found\n");
		return res;
	}
	control_mode = entry.data.u8[0];

	prv_control_mode = states_3a->prv_control_mode;

	if (control_mode != prv_control_mode) {
		pr_debug("simucamera: AE: control mode switch\n");
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
			pr_err
			    ("simucamera: AE CONTROL_SCENE_MODE tag not found");
			return res;
		}

		if (entry.data.u8[0] != states_3a->prv_scene_mode) {
			pr_debug("simucamera: AE: scene mode switch\n");
			mode_switch = true;
			goto mode_switch_case;
		}
	}

	res =
	    amd_cam_find_metadata_entry(metadata, AMD_CAM_CONTROL_AE_MODE,
					&entry);
	if (res) {
		pr_err("simucamera: AE: CONTROL_AE_MODE tag not found\n");
		return res;
	}
	ae_mode = entry.data.u8[0];

	if (ae_mode != states_3a->prv_ae_mode) {
		pr_debug("simucamera: AE: ae mode switch\n");
		mode_switch = true;
		goto mode_switch_case;
	}

	ae_state = states_3a->ae_state;

	switch (ae_mode) {
	case AMD_CAM_CONTROL_AE_MODE_OFF:
		// AE is OFF
		states_3a->ae_state = AMD_CAM_CONTROL_AE_STATE_INACTIVE;
		ae_trigger_start = false;
		return 0;
	case AMD_CAM_CONTROL_AE_MODE_ON:
		// OK for AUTO modes
		break;
	default:
		// Mostly silently ignore unsupported modes
		pr_warn("simucamera: AE: doesn't support AE mode %d\n",
			ae_mode);
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
		precapture_trigger = true;
		precapture_trigger_val = entry.data.u8[0];
		if (AMD_CAM_CONTROL_AE_PRECAPTURE_TRIGGER_START ==
		    precapture_trigger_val) {
			ae_trigger_start = true;
		}

		if (AMD_CAM_CONTROL_AE_PRECAPTURE_TRIGGER_CANCEL ==
		    precapture_trigger_val) {
			ae_trigger_start = false;
		}
	}

	if (ae_locked) {
		// AE is locked
		pr_debug("simucamera: AE: aeLock[%d]\n", ae_locked);
		ae_state = AMD_CAM_CONTROL_AE_STATE_LOCKED;
	} else if (precapture_trigger &&
		   AMD_CAM_CONTROL_AE_PRECAPTURE_TRIGGER_CANCEL ==
		   precapture_trigger_val) {
		// Precpture cancel received, set to inactive
		ae_state = AMD_CAM_CONTROL_AE_STATE_INACTIVE;
	} else if ((precapture_trigger &&
		   AMD_CAM_CONTROL_AE_PRECAPTURE_TRIGGER_START ==
		    precapture_trigger_val) ||
		   ae_state == AMD_CAM_CONTROL_AE_STATE_PRECAPTURE) {
		// Run precapture sequence
		if (ae_state != AMD_CAM_CONTROL_AE_STATE_PRECAPTURE)
			ae_counter = 0;

		if (ae_counter > pre_capture_min_frames) {
			// Done with precapture
			ae_counter = 0;
			ae_state = AMD_CAM_CONTROL_AE_STATE_CONVERGED;
		} else {
			// Converge some more
			ae_counter++;
			ae_state = AMD_CAM_CONTROL_AE_STATE_PRECAPTURE;
		}
	} else if ((precapture_trigger &&
		    AMD_CAM_CONTROL_AE_PRECAPTURE_TRIGGER_START ==
		    precapture_trigger_val) && !ae_locked) {
		ae_state = AMD_CAM_CONTROL_AE_STATE_PRECAPTURE;
	} else if (!ae_locked && ae_trigger_start) {
		// Run standard occasional AE scan
		switch (ae_state) {
		case AMD_CAM_CONTROL_AE_STATE_PRECAPTURE:
			ae_counter++;
			if (ae_counter > pre_capture_min_frames) {
				// Close enough
				ae_state = AMD_CAM_CONTROL_AE_STATE_CONVERGED;
				ae_counter = 0;
			}
			break;
		case AMD_CAM_CONTROL_AE_STATE_CONVERGED:
			ae_counter++;
			if (ae_counter > stable_ae_max_frames)
				ae_state = AMD_CAM_CONTROL_AE_STATE_PRECAPTURE;
			break;
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
			pr_err("simucamera: AE: In unexpected AE state %d\n",
			       ae_state);
			return -EINVAL;
		}
	}

 mode_switch_case:
	if (mode_switch) {
		pr_debug("simucamera: AE: mode switch detected\n");
		ae_state = AMD_CAM_CONTROL_AE_STATE_INACTIVE;
		ae_trigger_start = false;
	}

	states_3a->ae_state = ae_state;
	pr_debug("simucamera: AE: ae_state:%d, ae_lock:%d, ae_trigger:%d\n",
		 ae_state, ae_locked, precapture_trigger);

	return 0;
}

int do_simu_af(struct amd_cam_metadata *metadata)
{
	int res, rand_num;
	u8 af_state = AMD_CAM_CONTROL_AF_STATE_INACTIVE, af_mode, af_trigger;
	u8 control_mode, prv_control_mode;
	bool af_trigger_start = false;
	bool mode_switch = false;
	struct amd_cam_metadata_entry entry;
	struct algo_3a_states *states_3a = &g_cam->states;

	res =
	    amd_cam_find_metadata_entry(metadata, AMD_CAM_CONTROL_MODE, &entry);
	if (res) {
		pr_err("simucamera: AF: CONTROL_MODE tag not found\n");
		return res;
	}
	control_mode = entry.data.u8[0];

	prv_control_mode = states_3a->prv_control_mode;

	if (control_mode != prv_control_mode) {
		pr_debug("simucamera: AF: control mode switch\n");
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
			pr_err
			    ("simucamera: AF CONTROL_SCENE_MODE tag not found");
			return res;
		}

		if (entry.data.u8[0] != states_3a->prv_scene_mode) {
			pr_debug("simucamera: AF: scene mode switch\n");
			mode_switch = true;
			goto mode_switch_case;
		}
	}

	res = amd_cam_find_metadata_entry(metadata, AMD_CAM_CONTROL_AF_MODE,
					  &entry);
	if (res) {
		pr_err("simucamera: AF: CONTROL_AF_MODE tag not found\n");
		return res;
	}

	af_mode = entry.data.u8[0];

	res = amd_cam_find_metadata_entry(metadata, AMD_CAM_CONTROL_AF_TRIGGER,
					  &entry);
	if (res)
		af_trigger = AMD_CAM_CONTROL_AF_TRIGGER_IDLE;
	else
		af_trigger = entry.data.u8[0];

	af_state = states_3a->af_state;

	switch (af_mode) {
	case AMD_CAM_CONTROL_AF_MODE_OFF:
	case AMD_CAM_CONTROL_AF_MODE_EDOF:
		af_state = AMD_CAM_CONTROL_AF_STATE_INACTIVE;
		states_3a->af_state = af_state;
		return 0;
	case AMD_CAM_CONTROL_AF_MODE_AUTO:
	case AMD_CAM_CONTROL_AF_MODE_MACRO:
	case AMD_CAM_CONTROL_AF_MODE_CONTINUOUS_VIDEO:
	case AMD_CAM_CONTROL_AF_MODE_CONTINUOUS_PICTURE:
		break;
	default:
		pr_err("simucamera: AF: doesn't support AF mode %d\n", af_mode);
		return -EINVAL;
	}

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
		// Cancel trigger always transitions into INACTIVE
		af_state = AMD_CAM_CONTROL_AF_STATE_INACTIVE;
		states_3a->af_state = af_state;
		pr_debug
		    ("simucamera: AF: AF State transition to STATE_INACTIVE\n");

		return 0;
	default:
		pr_err("simucamera: AF: Unknown AF trigger value %d\n",
		       af_trigger);

		return -EINVAL;
	}

	if (af_mode != states_3a->prv_af_mode && !af_trigger_start) {
		pr_debug("simucamera: AF: af mode switch\n");
		mode_switch = true;
		goto mode_switch_case;
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
			switch (af_mode) {
			case AMD_CAM_CONTROL_AF_MODE_CONTINUOUS_VIDEO:
				// fall-through
			case AMD_CAM_CONTROL_AF_MODE_CONTINUOUS_PICTURE:
				af_state =
				    AMD_CAM_CONTROL_AF_STATE_PASSIVE_SCAN;
				break;
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
		else {
			// Randomly transition to passive focus/unfocus
			get_random_bytes(&rand_num, sizeof(rand_num));
			if (rand_num % 3 == 0) {
				af_state =
				    AMD_CAM_CONTROL_AF_STATE_PASSIVE_FOCUSED;
			} else {
				af_state =
				    AMD_CAM_CONTROL_AF_STATE_PASSIVE_UNFOCUSED;
			}
		}
		break;

	case AMD_CAM_CONTROL_AF_STATE_PASSIVE_FOCUSED:
		// Randomly transition to passive focus/unfocus
		get_random_bytes(&rand_num, sizeof(rand_num));
		if (af_trigger_start) {
			// immediate transition to focused locked
			af_state = AMD_CAM_CONTROL_AF_STATE_FOCUSED_LOCKED;
		} else if (rand_num % 3 == 0) {
			af_state = AMD_CAM_CONTROL_AF_STATE_PASSIVE_SCAN;
		}
		break;

	case AMD_CAM_CONTROL_AF_STATE_PASSIVE_UNFOCUSED:
		if (af_trigger_start) {
			// immediate transition to not focused locked
			af_state = AMD_CAM_CONTROL_AF_STATE_NOT_FOCUSED_LOCKED;
		} else {
			af_state = AMD_CAM_CONTROL_AF_STATE_PASSIVE_SCAN;
		}
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
		pr_err("simucamera: AF: Bad AF state %d\n", af_state);
	}

 mode_switch_case:
	if (mode_switch) {
		pr_debug("simucamera: AF: mode switch detected\n");
		af_state = AMD_CAM_CONTROL_AF_STATE_INACTIVE;
	}

	states_3a->af_state = af_state;
	return 0;
}

int do_simu_awb(struct amd_cam_metadata *metadata)
{
	int res;
	u8 awb_mode, awb_state;
	bool awb_locked = false;
	struct amd_cam_metadata_entry entry;
	struct algo_3a_states *states_3a = &g_cam->states;

	res = amd_cam_find_metadata_entry(metadata, AMD_CAM_CONTROL_AWB_MODE,
					  &entry);
	if (res) {
		pr_err("simucamera: AWB: CONTROL_AWB_MODE tag not found\n");
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
	case AMD_CAM_CONTROL_AWB_MODE_CLOUDY_DAYLIGHT:
	case AMD_CAM_CONTROL_AWB_MODE_TWILIGHT:
		// Always magically right, or locked
		awb_state = awb_locked ? AMD_CAM_CONTROL_AWB_STATE_LOCKED
		    : AMD_CAM_CONTROL_AWB_STATE_CONVERGED;
		break;
	default:
		pr_err("simucamera: AWB: doesn't support AWB mode %d\n",
		       awb_mode);
		return -EINVAL;
	}

	states_3a->awb_state = awb_state;
	return 0;
}

void update_simu_3a_region(u32 tag, struct amd_cam_metadata *metadata)
{
	int res;
	s32 crop_region[4], *a_region;
	struct amd_cam_metadata_entry entry;

	if (tag != AMD_CAM_CONTROL_AE_REGIONS &&
	    tag != AMD_CAM_CONTROL_AF_REGIONS &&
	    tag != AMD_CAM_CONTROL_AWB_REGIONS) {
		pr_warn("simucamera: 3A: invalid tag 0x%x\n", tag);
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

void update_simu_3a(struct amd_cam_metadata *metadata)
{
	size_t data_count = 1;
	u8 ae_state = g_cam->states.ae_state;
	u8 af_state = g_cam->states.af_state;
	u8 awb_state = g_cam->states.awb_state;
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

	update_simu_3a_region(AMD_CAM_CONTROL_AE_REGIONS, metadata);
	update_simu_3a_region(AMD_CAM_CONTROL_AF_REGIONS, metadata);
	update_simu_3a_region(AMD_CAM_CONTROL_AWB_REGIONS, metadata);
}

int metadata_update_sensor_timestamp(struct amd_cam_metadata *metadata,
				     u64 *timestamp)
{
	int res = 0;
	size_t data_count = 1;
	struct amd_cam_metadata *cam_metadata =
	    (struct amd_cam_metadata *)metadata;

	res =
	    amd_cam_update_tag(cam_metadata, AMD_CAM_SENSOR_TIMESTAMP,
			       (const void *)timestamp, data_count, NULL);
	if (res) {
		pr_err("updation of sensor timestamp tag failed\n");
		return -EINVAL;
	}
	return res;
}

int set_properties(struct s_properties *prop)
{
	int res = 0;
	unsigned long flags;
	struct s_properties *g_prop;

	if (prop->depth > MAX_REQ_DEPTH) {
		pr_err
		    ("simucamera: set-prop: Invalid queue depth prop-%d\n",
		     prop->depth);
		res = -EINVAL;
		goto exit;
	}

	spin_lock_irqsave(&g_cam->lock, flags);
	g_prop = &g_cam->properties;

	g_prop->depth = prop->depth;
	g_prop->sensor_id = prop->sensor_id;
	g_prop->profile = prop->profile;
	g_prop->fw_log_enable = prop->fw_log_enable;
	g_prop->fw_log_level = prop->fw_log_level;
	g_prop->drv_log_level = prop->drv_log_level;
	g_prop->cvip_enable = prop->cvip_enable;
	g_prop->cvip_raw_slicenum = prop->cvip_raw_slicenum;
	g_prop->cvip_output_slicenum = prop->cvip_output_slicenum;
	g_prop->cvip_raw_fmt = prop->cvip_raw_fmt;
	g_prop->cvip_yuv_w = prop->cvip_yuv_w;
	g_prop->cvip_yuv_h = prop->cvip_yuv_h;
	g_prop->cvip_yuv_fmt = prop->cvip_yuv_fmt;

	pr_debug
	    ("simucamera: set-prop: depth[%d], sensor_id[%d], profile[%d]\n",
	     g_prop->depth, g_prop->sensor_id, g_prop->profile);
	pr_debug("simucamera: set-prop: fw_log_en[%d], level[%llx]\n",
		 g_prop->fw_log_enable, g_prop->fw_log_level);
	pr_debug("simucamera: set-prop: drv_log_level[%d], cvip en[%d]\n",
		 g_prop->drv_log_level, g_prop->cvip_enable);
	pr_debug
	    ("simucamera: set-prop: raw slice[%d], o/p slice[%d], rawfmt[%d]\n",
	     g_prop->cvip_raw_slicenum, g_prop->cvip_output_slicenum,
	     g_prop->cvip_raw_fmt);
	pr_debug("simucamera: set-prop: cvip yuv resolution[%dx%d] fmt[%d]",
		 g_prop->cvip_yuv_w, g_prop->cvip_yuv_h, g_prop->cvip_yuv_fmt);

	spin_unlock_irqrestore(&g_cam->lock, flags);

 exit:
	return res;
}

int ioctl_simu_done_buf(struct simu_cam_buf *done_buf)
{
	struct cam_stream *stream;
	struct vb2_queue *bq;
	struct vb2_buffer *vb;
	struct stream_info *str_done = NULL;
	void *vaddr = NULL;
	unsigned long flags, flags1;
	u8 match = 0;
	struct shutter_info *shutter1 = NULL;
	struct shutter_info *tmp1 = NULL;

	spin_lock_irqsave(&g_cam->lock, flags);
	stream = g_cam->streams[done_buf->sid];
	spin_unlock_irqrestore(&g_cam->lock, flags);

	spin_lock_irqsave(&stream->lock, flags1);

	bq = &stream->bq;
	vb = bq->bufs[done_buf->index];

	/* ignore all results for combined_index which doesn't
	 * exist in shutter_list
	 */
	spin_lock_irqsave(&stream->shutter_lock, flags);
	if (!list_empty(&stream->shutter_list)) {
		list_for_each_entry_safe(shutter1, tmp1, &stream->shutter_list,
					 shutter_entry) {
			int com_req_id = shutter1->combined_index;

			if (com_req_id == done_buf->combined_index) {
				match = 1;
				break;
			}
		}
	}
	spin_unlock_irqrestore(&stream->shutter_lock, flags);

	if (match == 0)
		pr_err("simucamera: done-buf: cam[%d], stream[%d], index[%d]",
		       stream->str_done[done_buf->index]->virt_cam_id,
		       done_buf->sid,
		       stream->str_done[done_buf->index]->combined_index);
		pr_err("no match found");

	if (stream->stream_on && match) {
		unsigned int count, copied;

		spin_lock_irqsave(&stream->done_lock, flags);

		str_done = stream->str_done[done_buf->index];
		str_done->combined_index = done_buf->combined_index;
		str_done->virt_cam_id = done_buf->virt_cam_id;
		str_done->priv = done_buf->priv;
		str_done->bufhandle = done_buf->bufhandle;
		str_done->metadata = done_buf->metadata;

		list_add_tail(&str_done->entry, &stream->done_list);

		spin_unlock_irqrestore(&stream->done_lock, flags);

		if (stream->fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_NV12) {
			count =
			    stream->fmt.fmt.pix.width *
			    stream->fmt.fmt.pix.height +
			    stream->fmt.fmt.pix.width *
			    ((stream->fmt.fmt.pix.height + 1) / 2);
		} else if (stream->fmt.fmt.pix.pixelformat ==
			   V4L2_PIX_FMT_SRGGB16) {
			count =
			    stream->fmt.fmt.pix.width *
			    stream->fmt.fmt.pix.height * 2;
		} else if (stream->fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_Y8I) {
			count =
			    stream->fmt.fmt.pix.width *
			    stream->fmt.fmt.pix.height;
		} else {
			spin_unlock_irqrestore(&stream->lock, flags1);
			pr_err
			    ("simucamera done-buf cam[%d] stream[%d] index[%d]",
			     str_done->virt_cam_id, done_buf->sid,
			     str_done->combined_index);
			pr_err("invalid fmt[%d]",
			       stream->fmt.fmt.pix.pixelformat);
			return -EINVAL;
		}

		vaddr = bq->mem_ops->vaddr(vb->planes[0].mem_priv);

		copied = copy_from_user(vaddr, done_buf->data, count);

		vb->planes[0].bytesused = copied;

		spin_unlock_irqrestore(&stream->lock, flags1);

		mutex_lock(bq->lock);
		vb2_buffer_done(vb, VB2_BUF_STATE_DONE);
		mutex_unlock(bq->lock);

		spin_lock_irqsave(&stream->lock, flags1);

		pr_debug
		    ("simucamera: done-buf: cam[%d], stream[%d], index[%d]\n",
		     str_done->virt_cam_id, done_buf->sid,
		     str_done->combined_index);
	} else {
		pr_warn("simucamera: done-buf: stream[%d] - invalid state\n",
			done_buf->sid);
	}

	spin_unlock_irqrestore(&stream->lock, flags1);

	return 0;
}

void helper_update_shutter_queue(struct cam_stream *str1, int cmb_idx)
{
	struct cam_stream *str2 = NULL;
	struct shutter_info *shutter = NULL, *tmp = NULL;
	unsigned long flags;
	int i;

	for (i = 0; i < MAX_STREAM_NUM_PER_PHYSICAL_CAMERA; i++) {
		if (i != str1->stream_id) {
			spin_lock_irqsave(&g_cam->lock, flags);
			str2 = g_cam->streams[i];
			spin_unlock_irqrestore(&g_cam->lock, flags);
			if (!str2)
				continue;

			spin_lock_irqsave(&str2->shutter_lock, flags);
			if (!list_empty(&str2->shutter_list)) {
				list_for_each_entry_safe(shutter, tmp,
							 &str2->shutter_list,
							 shutter_entry) {
					if (shutter->combined_index ==
					    cmb_idx &&
					    shutter->num_of_bufs > 0) {
						shutter->num_of_bufs--;
					}
				}
			}
			spin_unlock_irqrestore(&str2->shutter_lock, flags);
		}
	}
}

void update_stream_shutter_queue(struct cam_stream *str1)
{
	struct shutter_info *shutter1 = NULL;
	struct shutter_info *tmp1 = NULL;

	if (!list_empty(&str1->shutter_list)) {
		list_for_each_entry_safe(shutter1, tmp1, &str1->shutter_list,
					 shutter_entry) {
			int com_req_id = shutter1->combined_index;

			if (shutter1->num_of_bufs == 1) {
				unsigned long flags;

				spin_lock_irqsave(&g_cam->lock, flags);
				g_cam->num_of_reqs--;
				spin_unlock_irqrestore(&g_cam->lock, flags);
				continue;
			}
			helper_update_shutter_queue(str1, com_req_id);
		}
	}
}

void remove_shutter_queue_entry(struct cam_stream *stream, u32 cmb_idx)
{
	struct shutter_info *shutter = NULL, *tmp = NULL;
	unsigned long flags;

	spin_lock_irqsave(&stream->shutter_lock, flags);
	if (!list_empty(&stream->shutter_list)) {
		list_for_each_entry_safe(shutter, tmp, &stream->shutter_list,
					 shutter_entry) {
			if (shutter->combined_index == cmb_idx) {
				list_del(&(shutter)->shutter_entry);
				pr_debug("simucamera remove shutter stream[%d]",
					 stream->stream_id);
				pr_debug("removed entry");
				break;
			}
		}
	} else {
		pr_warn
		    ("simucamera remove shutter stream[%d] empty shutter list",
		     stream->stream_id);
	}
	spin_unlock_irqrestore(&stream->shutter_lock, flags);
}

int clear_local_queue_entries(struct kernel_request *request, int stream_id)
{
	int i;
	u32 cmb_idx = request->com_req_id;
	for (i = 0; i < request->num_output_buffers; i++) {
		struct kernel_buffer *stream_buf = request->output_buffers[i];
		struct cam_stream *stream = NULL;
		unsigned long flags;

		if (!stream_buf) {
			pr_err
			("simucamera clear queue invalid kernel buf received");
			return -EINVAL;
		}

		spin_lock_irqsave(&g_cam->lock, flags);
		stream = g_cam->streams[stream_buf->stream_id];
		spin_unlock_irqrestore(&g_cam->lock, flags);

		if (!stream) {
			pr_err
			    ("simucamera clear queue stream[%d] invalid stream",
			     stream_buf->stream_id);
			return -EINVAL;
		}

		if (stream->stream_id == stream_id) {
			remove_shutter_queue_entry(stream, cmb_idx);
		} else {
			spin_lock_irqsave(&stream->lock, flags);
			remove_shutter_queue_entry(stream, cmb_idx);
			spin_unlock_irqrestore(&stream->lock, flags);
		}
	}

	return 0;
}

int ioctl_simu_streamon(struct file *file,
			struct kernel_stream_indexes *stream_on)
{
	struct cam_stream *str1 = NULL;
	struct v4l2_event ev;
	int i;

	for (i = 0; i < stream_on->stream_cnt; i++) {
		struct vb2_queue *bq;
		struct simu_cam_on *simu_on;
		int ret, stream_id = stream_on->stream_ids[i];
		unsigned long flags;

		spin_lock_irqsave(&g_cam->lock, flags);
		str1 = g_cam->streams[stream_id];
		spin_unlock_irqrestore(&g_cam->lock, flags);

		if (!str1) {
			pr_err
			    ("simucamera stream-on stream[%d] - invalid stream",
			     stream_id);

			return -EINVAL;
		}

		spin_lock_irqsave(&str1->lock, flags);
		bq = &str1->bq;

		ev.type = V4L2_EVENT_SIMU_CAM;
		ev.id = SIMU_STREAM_ON;
		simu_on = (struct simu_cam_on *)ev.u.data;
		simu_on->on = 1;
		simu_on->sid = str1->stream_id;
		v4l2_event_queue(g_cam->video_dev, &ev);

		ret = vb2_streamon(bq, V4L2_BUF_TYPE_VIDEO_CAPTURE);
		if (ret != 0) {
			spin_unlock_irqrestore(&str1->lock, flags);

			pr_err
			("simucamera stream-on s[%d] VB2 stream on fail[%d]",
			 stream_id, ret);

			return ret;
		}

		str1->stream_on = true;
		spin_unlock_irqrestore(&str1->lock, flags);

		pr_info("simucamera: stream-on: stream[%d]\n", stream_id);
	}

	return 0;
}

int ioctl_simu_streamoff(struct file *file,
			 struct kernel_stream_indexes *stream_off)
{
	struct cam_stream *str1 = NULL;
	struct v4l2_event ev;
	int i;
	unsigned long flags;

	for (i = 0; i < stream_off->stream_cnt; i++) {
		struct vb2_queue *bq;
		struct simu_cam_on *simu_on;
		int stream_id = stream_off->stream_ids[i];
		int ret;

		spin_lock_irqsave(&g_cam->lock, flags);
		str1 = g_cam->streams[stream_id];
		spin_unlock_irqrestore(&g_cam->lock, flags);

		if (!str1) {
			pr_err
			    ("simucamera stream-off stream[%d] invalid stream",
			     stream_id);

			return -EINVAL;
		}

		spin_lock_irqsave(&str1->lock, flags);

		if (!str1->stream_on)
			goto already_streamoff;

		bq = &str1->bq;

		spin_unlock_irqrestore(&str1->lock, flags);

		pr_info
		    ("simucamera stream-off stream[%d] waiting for all buffers",
		     stream_id);
		wait_for_all_buffer_results(str1);

		pr_debug
		    ("simucamera: stream-off stream[%d] - all buffers dequeued",
		     stream_id);
		spin_lock_irqsave(&str1->lock, flags);

		ev.type = V4L2_EVENT_SIMU_CAM;
		ev.id = SIMU_STREAM_ON;
		simu_on = (struct simu_cam_on *)ev.u.data;
		simu_on->on = 0;
		simu_on->sid = str1->stream_id;
		v4l2_event_queue(g_cam->video_dev, &ev);

		ret = vb2_streamoff(bq, V4L2_BUF_TYPE_VIDEO_CAPTURE);
		if (ret != 0) {
			spin_unlock_irqrestore(&str1->lock, flags);

			pr_err("simucamera: stream-off: stream[%d]", stream_id);
			pr_err("VB2 stream off failed[%d]\n", ret);

			return ret;
		}

		str1->stream_on = false;

		INIT_LIST_HEAD(&str1->done_list);
		atomic_set(&str1->owned_by_stream_count, 0);

 already_streamoff:
		spin_unlock_irqrestore(&str1->lock, flags);

		pr_info("simucamera: stream-off: stream[%d]\n", stream_id);
	}

	return 0;
}

int ioctl_simu_streamdestroy(struct file *file,
			     struct kernel_stream_indexes *stream_destroy)
{
	struct cam_stream *str1 = NULL;
	struct v4l2_event ev;
	int i, loop;
	unsigned long flags, flags1;

	for (i = 0; i < stream_destroy->stream_cnt; i++) {
		struct vb2_queue *bq = NULL;
		struct simu_cam_on *simu_on = NULL;
		int stream_id = stream_destroy->stream_ids[i];
		int ret;

		spin_lock_irqsave(&g_cam->lock, flags);
		str1 = g_cam->streams[stream_id];
		spin_unlock_irqrestore(&g_cam->lock, flags);

		if (!str1) {
			pr_err("simucamera: stream-destroy: stream[%d]",
			       stream_id);
			pr_err("invalid stream");

			return -EINVAL;
		}

		spin_lock_irqsave(&str1->lock, flags);

		bq = &str1->bq;

		if (!str1->stream_on)
			goto already_streamoff;

		spin_unlock_irqrestore(&str1->lock, flags);
		pr_info("simucamera: stream-destroy: stream[%d] - waiting for buffers\n",
			stream_id);

		for (loop = 0; loop < bq->num_buffers; ++loop) {
			if (bq->bufs[loop]->state == VB2_BUF_STATE_ACTIVE) {
				pr_warn("fakecamera: leaving buf %p in active state\n",
					bq->bufs[loop]);
				vb2_buffer_done(bq->bufs[loop],
						VB2_BUF_STATE_ERROR);
			}
		}
		pr_debug
		    ("simucamera stream-destroy stream[%d] all bufs dequeued",
		     stream_id);

		atomic_set(&str1->owned_by_stream_count, 0);
		wake_up_all(&str1->done_wq);

		spin_lock_irqsave(&str1->lock, flags);

		ev.type = V4L2_EVENT_SIMU_CAM;
		ev.id = SIMU_STREAM_ON;
		simu_on = (struct simu_cam_on *)ev.u.data;
		simu_on->on = 0;
		simu_on->sid = str1->stream_id;
		v4l2_event_queue(g_cam->video_dev, &ev);

		ret = vb2_streamoff(bq, V4L2_BUF_TYPE_VIDEO_CAPTURE);
		if (ret != 0) {
			spin_unlock_irqrestore(&str1->lock, flags);

			pr_err("simucamera: stream-destroy: stream[%d]",
			       stream_id);
			pr_err("VB2 stream off failed[%d]\n", ret);

			return ret;
		}

		str1->stream_on = false;

 already_streamoff:
		pr_debug
		    ("simucamera: stream-destroy: stream[%d] - release queue\n",
		     stream_id);
		vb2_core_queue_release(bq);

		spin_lock_irqsave(&str1->shutter_lock, flags1);
		update_stream_shutter_queue(str1);
		spin_unlock_irqrestore(&str1->shutter_lock, flags1);

		INIT_LIST_HEAD(&str1->done_list);
		INIT_LIST_HEAD(&str1->shutter_list);

		spin_unlock_irqrestore(&str1->lock, flags);

		pr_info("simucamera: stream-destroy: stream[%d]\n", stream_id);
	}

	return 0;
}

int ioctl_simu_config(struct file *file,
		      struct kernel_stream_configuration *config)
{
	struct v4l2_event ev, ev1;
	struct cam_stream *stream = NULL;
	struct v4l2_format fmt;
	unsigned long flags;
	int i, count;

	pr_info("simucamera: config: num streams[%d]\n", config->num_streams);
	pr_info("simucamera: ************* Sensor Profile **************\n");
	pr_info("simucamera: config: width[%d]\n", config->profile->width);
	pr_info("simucamera: config: height[%d]\n", config->profile->height);
	pr_info("simucamera: config: binning_mode[%d]\n",
		config->profile->binning_mode);
	pr_info("simucamera: config: fps[%d]\n", config->profile->fps);
	pr_info("simucamera: config: hdr_state[%d]\n",
		config->profile->hdr_state);

	for (i = 0; i < config->num_streams; i++) {
		struct kernel_stream *config_stream = config->streams[i];
		struct vb2_queue *bq = NULL;
		struct simu_cam_fmt *simu_fmt = NULL;
		struct simu_cam_on *simu_on = NULL;
		int ret;

		if (!config_stream) {
			pr_err
			    ("simucamera config invalid kernel stream receive");
			return -EINVAL;
		}

		spin_lock_irqsave(&g_cam->lock, flags);
		stream = g_cam->streams[config_stream->stream_id];
		count = g_cam->properties.depth;
		spin_unlock_irqrestore(&g_cam->lock, flags);
		if (!stream) {
			pr_err("simucamera: config: num streams[%d] stream[%d]",
			       config->num_streams, config_stream->stream_id);
			pr_err("invalid stream\n");

			return -EINVAL;
		}

		spin_lock_irqsave(&stream->lock, flags);

		ev.type = V4L2_EVENT_SIMU_CAM;
		ev.id = SIMU_STREAM_ON;
		simu_on = (struct simu_cam_on *)ev.u.data;
		simu_on->on = 0;
		simu_on->sid = stream->stream_id;
		v4l2_event_queue(g_cam->video_dev, &ev);

		bq = &stream->bq;

		ret = vb2_streamoff(bq, V4L2_BUF_TYPE_VIDEO_CAPTURE);
		if (ret != 0) {
			spin_unlock_irqrestore(&stream->lock, flags);

			pr_err("simucamera: config: num streams[%d] stream[%d]",
			       config->num_streams, config_stream->stream_id);
			pr_err("VB2 stream on failed[%d]", ret);

			return ret;
		}

		stream->stream_on = false;

		fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		fmt.fmt.pix.width = config_stream->width;
		fmt.fmt.pix.height = config_stream->height;
		if (config_stream->format == PVT_IMG_FMT_NV12) {
			fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_NV12;
		} else if (config_stream->format == PVT_IMG_FMT_RAW16) {
			fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_SRGGB16;
		} else if (config_stream->format == PVT_IMG_FMT_Y8) {
			fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_Y8I;
		} else {
			spin_unlock_irqrestore(&stream->lock, flags);

			pr_err("simucamera: config: num streams[%d] stream[%d]",
			       config->num_streams, config_stream->stream_id);
			pr_err("invalid fmt[%d]", config_stream->format);

			return -EINVAL;
		}

		ev1.type = V4L2_EVENT_SIMU_CAM;
		ev1.id = SIMU_SET_FMT;
		simu_fmt = (struct simu_cam_fmt *)ev1.u.data;
		simu_fmt->type = fmt.type;
		simu_fmt->width = fmt.fmt.pix.width;
		simu_fmt->height = fmt.fmt.pix.height;
		simu_fmt->pixelformat = fmt.fmt.pix.pixelformat;
		simu_fmt->sid = stream->stream_id;

		v4l2_event_queue(g_cam->video_dev, &ev1);
		stream->fmt = fmt;

		ret =
		    vb2_verify_memory_type(bq, V4L2_MEMORY_USERPTR,
					   V4L2_BUF_TYPE_VIDEO_CAPTURE);
		if (ret) {
			spin_unlock_irqrestore(&stream->lock, flags);

			pr_err("simucamera: config: num streams[%d] stream[%d]",
			       config->num_streams, config_stream->stream_id);
			pr_err("invalid memory type");

			return ret;
		}

		spin_unlock_irqrestore(&stream->lock, flags);

		mutex_lock(bq->lock);
		ret = vb2_core_reqbufs(bq, V4L2_MEMORY_USERPTR, &count);
		if (ret == 0) {
			bq->owner = count ? file->private_data : NULL;
		} else {
			mutex_unlock(bq->lock);

			pr_err("simucamera: config: num streams[%d] stream[%d]",
			       config->num_streams, config_stream->stream_id);
			pr_err("VB2 request bufs failed[%d]", ret);

			return ret;
		}

		mutex_unlock(bq->lock);

		spin_lock_irqsave(&stream->lock, flags);

		stream->buf_status.free_buff_count = count;
		stream->buf_status.max_buffers = count;
		stream->buf_status.free_index = 0;
		spin_unlock_irqrestore(&stream->lock, flags);

		pr_info
		    ("simucamera config num streams[%d] stream[%d] format[%d]",
		     config->num_streams, config_stream->stream_id,
		     config_stream->format);
	}

	return 0;
}

int ioctl_simu_request(struct kernel_request *request)
{
	struct cam_stream *stream = NULL;
	struct shutter_info *shutter = NULL;
	unsigned long flags, flags1;
	int i;

	if (!request) {
		pr_err("simucamera: request: invalid kernel_request pointer\n");
		return -EINVAL;
	}

	if (!request->num_output_buffers) {
		pr_err
		    ("simucamera: request invalid number of output buffers[%d]",
		     request->num_output_buffers);
		return -EINVAL;
	}
	//Validate stream buffers and stream state
	for (i = 0; i < request->num_output_buffers; i++) {
		struct kernel_buffer *stream_buf = request->output_buffers[i];

		if (!stream_buf) {
			pr_err("simucamera request invalid kernel bufreceived");
			return -EINVAL;
		}

		spin_lock_irqsave(&g_cam->lock, flags);
		stream = g_cam->streams[stream_buf->stream_id];
		spin_unlock_irqrestore(&g_cam->lock, flags);
		if (!stream) {
			pr_err("simucamera request stream[%d] - invalid stream",
			       stream_buf->stream_id);

			return -EINVAL;
		}

		spin_lock_irqsave(&stream->lock, flags1);
		if (!stream->stream_on) {
			spin_unlock_irqrestore(&stream->lock, flags1);
			pr_err("simucamera request stream[%d] - invalid state",
			       stream_buf->stream_id);

			return -EINVAL;
		}
		spin_unlock_irqrestore(&stream->lock, flags1);
	}

	spin_lock_irqsave(&g_cam->lock, flags);

	if (g_cam->num_of_reqs >= g_cam->properties.depth) {
		spin_unlock_irqrestore(&g_cam->lock, flags);

		pr_warn("simucamera: request: queue depth reached\n");

		return -EAGAIN;
	}
	g_cam->num_of_reqs++;
	spin_unlock_irqrestore(&g_cam->lock, flags);

	pr_debug("simucamera request bufs[%d] index[%d] reqs[%d] metadata[%p]",
		 request->num_output_buffers, request->com_req_id,
		 g_cam->num_of_reqs, request->kparam);

	for (i = 0; i < request->num_output_buffers; i++) {
		struct kernel_buffer *stream_buf = request->output_buffers[i];
		struct simu_evt_request *req_evt = NULL;
		struct v4l2_event ev;
		struct vb2_queue *bq = NULL;
		struct v4l2_buffer buffer;
		int index, ret;

		if (!stream_buf) {
			pr_err("simucamera request invalid kernel buf receive");
			return -EINVAL;
		}

		spin_lock_irqsave(&g_cam->lock, flags);
		stream = g_cam->streams[stream_buf->stream_id];
		spin_unlock_irqrestore(&g_cam->lock, flags);
		if (!stream) {
			pr_err("simucamera: request: stream[%d] invalid stream",
			       stream_buf->stream_id);

			return -EINVAL;
		}

		spin_lock_irqsave(&stream->lock, flags1);
		if (!stream->stream_on) {
			spin_unlock_irqrestore(&stream->lock, flags1);
			pr_err("simucamera request stream[%d] - invalid state",
			       stream_buf->stream_id);

			return -EINVAL;
		}

		ev.type = V4L2_EVENT_SIMU_CAM;
		ev.id = SIMU_REQUEST;
		req_evt = (struct simu_evt_request *)ev.u.data;
		req_evt->buf_num = 1;
		req_evt->combined_index = request->com_req_id;

		memset(&buffer, 0, sizeof(struct v4l2_buffer));
		buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buffer.memory = V4L2_MEMORY_USERPTR;
		index = get_free_index(stream);
		if (index == -1) {
			spin_unlock_irqrestore(&stream->lock, flags1);

			pr_err("simucamera request s[%d] invalid free index",
			       stream_buf->stream_id);

			return -EAGAIN;
		}

		spin_lock_irqsave(&stream->shutter_lock, flags);

		shutter = stream->str_shutter[index];
		shutter->combined_index = request->com_req_id;
		shutter->num_of_bufs = request->num_output_buffers;
		list_add_tail(&shutter->shutter_entry, &stream->shutter_list);
		spin_unlock_irqrestore(&stream->shutter_lock, flags);
		pr_debug("simucamera request s[%d] id[%d] bufs[%d] add shutter",
			 stream->stream_id, shutter->combined_index,
			 shutter->num_of_bufs);

		buffer.index = index;
		buffer.m.userptr = (unsigned long)stream_buf->user_ptr;
		buffer.length = stream_buf->length;
		req_evt->sid = stream->stream_id;
		pr_debug("simucamera request stream[%d] userptr[%p] length[%d]",
			 stream->stream_id, stream_buf->user_ptr,
			 stream_buf->length);

		req_evt->buf_idx = index;
		req_evt->virt_cam_id = stream_buf->vcam_id;
		req_evt->priv = (unsigned long)stream_buf->priv;
		req_evt->bufhandle = (unsigned long)stream_buf->buffer;
		req_evt->metadata = (unsigned long)request->kparam;

		bq = &stream->bq;

		spin_unlock_irqrestore(&stream->lock, flags1);

		mutex_lock(bq->lock);
		ret = vb2_qbuf(bq, NULL, &buffer);
		mutex_unlock(bq->lock);

		spin_lock_irqsave(&stream->lock, flags1);

		if (ret) {
			clear_local_queue_entries(request, stream->stream_id);
			spin_unlock_irqrestore(&stream->lock, flags1);

			pr_err("simucamera request stream[%d] VB2qbuf fail[%d]",
			       stream_buf->stream_id, ret);

			return ret;
		}

		atomic_inc(&stream->owned_by_stream_count);

		v4l2_event_queue(g_cam->video_dev, &ev);
		spin_unlock_irqrestore(&stream->lock, flags1);

		pr_debug("simucamera: request: stream[%d], index[%d], id[%d]",
			 stream->stream_id, index, stream_buf->vcam_id);
		pr_debug("priv[%p] buf[%p] user[%p] len[%d] md[%p] free[%d]",
			 stream_buf->priv, stream_buf->buffer,
			 stream_buf->user_ptr, stream_buf->length,
			 request->kparam, stream->buf_status.free_buff_count);
	}

	return 0;
}

int ioctl_simu_result(struct kernel_result *result)
{
	struct kernel_buffer *stream_buf = NULL;
	struct cam_stream *stream;
	struct v4l2_buffer dq_buf;
	struct timespec timestamp;
	struct buf_indexes *idx_info = NULL;
	struct shutter_info *shutter = NULL;
	u64 time_nsec;
	unsigned long flags, flags1;
	u32 partial_results = 0;
	bool is_shutter = false;

	if (!result) {
		pr_err("simucamera: result: invalid kernel_result pointer\n");
		return -EINVAL;
	}

	stream_buf = result->output_buffers[0];

	if (!stream_buf) {
		pr_err("simucamera: result: invalid kernel buffer received\n");
		return -EINVAL;
	}

	pr_debug("simucamera: result: stream[%d], id[%d]\n",
		 stream_buf->stream_id, stream_buf->vcam_id);

	spin_lock_irqsave(&g_cam->lock, flags);
	stream = g_cam->streams[stream_buf->stream_id];
	spin_unlock_irqrestore(&g_cam->lock, flags);
	if (!stream) {
		result->status = ERROR_INVALIDOPERATION;

		pr_err("simucamera result stream[%d] id[%d] - invalid stream",
		       stream_buf->stream_id, stream_buf->vcam_id);

		return -EINVAL;
	}

	spin_lock_irqsave(&stream->shutter_lock, flags);
	if (!list_empty(&stream->shutter_list)) {
		shutter =
		    list_first_entry(&stream->shutter_list, struct shutter_info,
				     shutter_entry);
	} else {
		spin_unlock_irqrestore(&stream->shutter_lock, flags);

		pr_err("simucamera result stream[%d] id[%d] empty shutter list",
		       stream_buf->stream_id, stream_buf->vcam_id);

		return -EAGAIN;
	}
	spin_unlock_irqrestore(&stream->shutter_lock, flags);

	spin_lock_irqsave(&g_cam->lock, flags);
	is_shutter = g_cam->is_shutter_event;
	spin_unlock_irqrestore(&g_cam->lock, flags);

	if (!is_shutter) {
		ktime_get_ts(&timestamp);
		time_nsec = SEC_TO_NANO_SEC(timestamp.tv_sec);
		time_nsec += timestamp.tv_nsec;
		spin_lock_irqsave(&g_cam->lock, flags);
		g_cam->is_shutter_event = true;
		g_cam->num_buffers = shutter->num_of_bufs;
		g_cam->shutter_timestamp = time_nsec;
		spin_unlock_irqrestore(&g_cam->lock, flags);
		result->data.timestamp = time_nsec;
		result->com_req_id = shutter->combined_index;
		result->is_shutter_event = true;
		result->num_output_buffers = 0;
		result->input_buffer = NULL;
		partial_results = 1;

		pr_debug("simucamera result stream[%d] id[%d] index[%d]",
			 stream->stream_id, stream_buf->vcam_id,
			 shutter->combined_index);
		pr_debug("bufs[%d] - shutter[%llu]",
			 shutter->num_of_bufs, result->data.timestamp);
	} else {
		struct stream_info *str_done = NULL;
		struct vb2_queue *bq = NULL;
		int ret = 0, num_buffers = 0;

		result->data.kparam_rst = NULL;
		result->is_shutter_event = false;
		result->input_buffer = NULL;

		spin_lock_irqsave(&stream->lock, flags);
		if (!stream->stream_on) {
			pr_err
			    ("simucamera: result: stream[%d] - invalid state\n",
			     stream_buf->stream_id);
			spin_unlock_irqrestore(&stream->lock, flags);
			return -EINVAL;
		}
		spin_unlock_irqrestore(&stream->lock, flags);

		memset(&dq_buf, 0, sizeof(struct v4l2_buffer));
		dq_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		dq_buf.memory = V4L2_MEMORY_USERPTR;

		bq = &stream->bq;

		mutex_lock(bq->lock);
		ret = vb2_dqbuf(bq, &dq_buf, false);
		mutex_unlock(bq->lock);

		if (ret != 0) {
			spin_lock_irqsave(&stream->lock, flags);
			if (!stream->stream_on) {
				pr_err("simucamera result s[%d] invalid state",
				       stream_buf->stream_id);
				spin_unlock_irqrestore(&stream->lock, flags);
				return -EINVAL;
			}
			spin_unlock_irqrestore(&stream->lock, flags);

			result->status = ERROR_INVALIDOPERATION;

			pr_err("simucamera result stream[%d] VB2dqbuf fail[%d]",
			       stream_buf->stream_id, ret);

			return -EINVAL;
		}
		stream_buf->status = BUFFER_STATUS_OK;
		if (dq_buf.flags & V4L2_BUF_FLAG_ERROR)
			stream_buf->status = BUFFER_STATUS_ERROR;

		spin_lock_irqsave(&stream->done_lock, flags);
		if (!list_empty(&stream->done_list)) {
			str_done =
			    list_first_entry(&stream->done_list,
					     struct stream_info, entry);
			list_del(&(str_done)->entry);
		} else {
			spin_unlock_irqrestore(&stream->done_lock, flags);

			pr_err("simucamera result s[%d] id[%d] empty done list",
			       stream_buf->stream_id, stream_buf->vcam_id);

			return -EINVAL;
		}

		result->com_req_id = str_done->combined_index;
		spin_unlock_irqrestore(&stream->done_lock, flags);

		result->num_output_buffers = 1;

		spin_lock_irqsave(&g_cam->lock, flags);
		if (!g_cam->is_metadata_sent) {
			struct amd_cam_metadata *metadata = NULL;

			g_cam->is_metadata_sent = true;
			spin_lock_irqsave(&stream->done_lock, flags1);
			result->data.kparam_rst = (void *)str_done->metadata;
			spin_unlock_irqrestore(&stream->done_lock, flags1);
			metadata =
			    (struct amd_cam_metadata *)result->data.kparam_rst;
			time_nsec = g_cam->shutter_timestamp;
			ret =
			    metadata_update_sensor_timestamp(metadata,
							     &time_nsec);
			if (ret) {
				spin_unlock_irqrestore(&g_cam->lock, flags);

				pr_err("simucamera result stream[%d] id[%d]",
				       stream_buf->stream_id,
				       stream_buf->vcam_id);
				pr_err("timestamp update fail[%d]", ret);

				return -EINVAL;
			}

			ret = process_simu_3a(metadata);
			if (ret) {
				spin_unlock_irqrestore(&g_cam->lock, flags);

				pr_err("simucamera result stream[%d] id[%d]",
				       stream_buf->stream_id,
				       stream_buf->vcam_id);
				pr_err("process 3A fail[%d]", ret);

				return -EINVAL;
			}
		}
		spin_unlock_irqrestore(&g_cam->lock, flags);

		stream_buf->user_ptr = (void *)dq_buf.m.userptr;
		stream_buf->length = dq_buf.length;
		spin_lock_irqsave(&stream->done_lock, flags);

		stream_buf->buffer = (void *)str_done->bufhandle;
		stream_buf->priv = (void *)str_done->priv;

		spin_unlock_irqrestore(&stream->done_lock, flags);

		spin_lock_irqsave(&stream->lock, flags);

		idx_info = &stream->buf_status;
		idx_info->free_buff_count++;
		spin_unlock_irqrestore(&stream->lock, flags);

		spin_lock_irqsave(&g_cam->lock, flags);
		g_cam->partial_results++;
		partial_results = g_cam->partial_results;
		g_cam->num_buffers--;
		if (g_cam->num_buffers == 0) {
			g_cam->is_shutter_event = false;
			g_cam->is_metadata_sent = false;
			g_cam->shutter_timestamp = 0;
			g_cam->partial_results = 0;
			g_cam->num_of_reqs--;
		}
		spin_unlock_irqrestore(&g_cam->lock, flags);

		spin_lock_irqsave(&stream->shutter_lock, flags);

		list_del(&(shutter)->shutter_entry);
		spin_unlock_irqrestore(&stream->shutter_lock, flags);

		spin_lock_irqsave(&stream->lock, flags);
		num_buffers = atomic_dec_return(&stream->owned_by_stream_count);
		pr_debug
		    ("simucamera: result: stream[%d], remaining buffers[%d]\n",
		     stream_buf->stream_id, num_buffers);
		if (!num_buffers) {
			pr_info("simucamera: stream[%d] - wakeup\n",
				stream_buf->stream_id);
			wake_up(&stream->done_wq);
		}
		spin_unlock_irqrestore(&stream->lock, flags);

		pr_debug("simucamera result stream[%d] index[%d] id[%d]",
			 stream->stream_id, result->com_req_id,
			 stream_buf->vcam_id);
		pr_debug("priv[%p] buf[%p] user[%p] len[%d] md[%p] free[%d]",
			 stream_buf->priv, stream_buf->buffer,
			 stream_buf->user_ptr, stream_buf->length,
			 result->data.kparam_rst, idx_info->free_buff_count);
	}
	result->partial_result = partial_results;
	result->status = ERROR_NONE;

	return 0;
}

int ioctl_simu_hotplugevent(struct kernel_hotplug_info *hot_plug_info)
{
#ifdef CONFIG_AMD_SIMU_CAMERA_ENABLE_UEVENT
	bool plug_state = hot_plug_info->hot_plug_state;

	if (!simu_kset_ref) {
		simu_kset_ref =
		    kset_create_and_add("hotplug_camera", NULL, kernel_kobj);

		if (!simu_kset_ref)
			goto error_kobject;

		simu_kobj_ref = kzalloc(sizeof(*simu_kobj_ref), GFP_KERNEL);

		if (!simu_kobj_ref) {
			kset_unregister(simu_kset_ref);
			goto error_kobject;
		}

		simu_kobj_ref->kset = simu_kset_ref;

		if (kobject_init_and_add
		    (simu_kobj_ref, &simu_ktype_ref, NULL, "%s", "camState")) {
			kobject_del(simu_kobj_ref);
			kobject_put(simu_kobj_ref);
			kset_unregister(simu_kset_ref);
		}
	}

	if (plug_state)
		kobject_uevent(simu_kobj_ref, KOBJ_ADD);
	else
		kobject_uevent(simu_kobj_ref, KOBJ_REMOVE);

	return 0;

 error_kobject:
	pr_warn("simucamera: hotplug: unable to send hotplug event\n");

	return -1;
#else
	pr_warn("simucamera: hotplug: Invalid hotplug event\n");
	return -1;
#endif

}

long _ioctl_default(struct file *file, void *fh,
		    bool valid_prio, unsigned int cmd, void *arg)
{
	pr_debug("simucamera: ioctl: cmd[%d]\n", cmd);

	switch (cmd) {
	case VIDIOC_SET_PROPERTY_LIST:{
			struct s_properties *prop = (struct s_properties *)arg;

			return set_properties(prop);
		}
		break;

	case VIDIOC_DONE_BUF:{
			struct simu_cam_buf *done_buf =
			    (struct simu_cam_buf *)arg;
			return ioctl_simu_done_buf(done_buf);
		}
		break;

	case VIDIOC_STREAM_ON:{
			struct kernel_stream_indexes *stream_on =
			    (struct kernel_stream_indexes *)arg;
			return ioctl_simu_streamon(file, stream_on);
		}
		break;

	case VIDIOC_STREAM_OFF:{
			struct kernel_stream_indexes *stream_off =
			    (struct kernel_stream_indexes *)arg;
			return ioctl_simu_streamoff(file, stream_off);
		}
		break;

	case VIDIOC_STREAM_DESTROY:{
			struct kernel_stream_indexes *stream_destroy =
			    (struct kernel_stream_indexes *)arg;
			return ioctl_simu_streamdestroy(file, stream_destroy);
		}
		break;

	case VIDIOC_CONFIG:{
			struct kernel_stream_configuration *config =
			    (struct kernel_stream_configuration *)arg;
			return ioctl_simu_config(file, config);
		}
		break;

	case VIDIOC_REQUEST:{
			struct kernel_request *request =
			    (struct kernel_request *)arg;
			return ioctl_simu_request(request);
		}
		break;

	case VIDIOC_RESULT:{
			struct kernel_result *result =
			    (struct kernel_result *)arg;
			return ioctl_simu_result(result);
		}
		break;

	case VIDIOC_HOTPLUG:{
			struct kernel_hotplug_info *hot_plug_info =
			    (struct kernel_hotplug_info *)arg;
			return ioctl_simu_hotplugevent(hot_plug_info);
		}
		break;

	default:
		pr_warn("simucamera: ioctl: invalid IOCTL\n");

		return -EINVAL;
	}

	return 0;
}

static const struct v4l2_ioctl_ops ioctl_ops = {
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
	.vidioc_subscribe_event = _ioctl_subscribe_event,
	.vidioc_unsubscribe_event = v4l2_event_unsubscribe,
	.vidioc_default = _ioctl_default,
};

static int __init camera_simu_init(void)
{
	int ret = 0;
	struct v4l2_device *v4l2_dev = NULL;
	struct video_device *video_dev = NULL;

	/* driver private data allocation */
	g_cam = kzalloc(sizeof(*g_cam), GFP_KERNEL);
	if (!g_cam) {
		ret = -ENOMEM;

		pr_err("simucamera: init: g_cam alloc failed\n");

		goto on_error;
	}
	v4l2_dev = &g_cam->v4l2_dev;

	snprintf(v4l2_dev->name, V4L2_DEVICE_NAME_SIZE, "v4l2-camera-simu");

	/* register v4l2 dev */
	ret = v4l2_device_register(NULL, v4l2_dev);
	if (ret) {
		pr_err("simucamera: init: v4l2 register failed\n");

		goto on_error;
	}

	/* video device allocation */
	video_dev = video_device_alloc();
	g_cam->video_dev = video_dev;

	if (!video_dev) {
		ret = -ENOMEM;

		pr_err("simucamera: init: video device alloc failed\n");

		goto on_error;
	}

	video_dev->lock = NULL;
	video_dev->v4l2_dev = v4l2_dev;
	video_dev->fops = &file_fops;
	video_dev->ioctl_ops = &ioctl_ops;
	video_dev->release = video_device_release;
	video_dev->device_caps =
	    V4L2_CAP_VIDEO_CAPTURE | V4L2_CAP_READWRITE | V4L2_CAP_STREAMING;

	snprintf(video_dev->name, 32, "camera-simu-device");
	ret = video_register_device(video_dev, VFL_TYPE_GRABBER, 2);
	if (ret) {
		pr_err("simucamera: init: video device register failed\n");

		goto on_error;
	}
#ifdef CONFIG_AMD_SIMU_CAMERA_ENABLE_UEVENT
	simu_kset_ref = kset_create_and_add("hotplug_camera", NULL,
					    kernel_kobj);
	if (!simu_kset_ref) {
		pr_err("simucamera: init: simu_kset_ref alloc failed\n");
		goto out;
	}

	simu_kobj_ref = kzalloc(sizeof(*simu_kobj_ref), GFP_KERNEL);
	if (!simu_kobj_ref)
		goto out_kset;

	simu_kobj_ref->kset = simu_kset_ref;
	if (kobject_init_and_add(simu_kobj_ref, &simu_ktype_ref, NULL, "%s",
				 "camState")) {
		pr_err("simucamera: init: simu_kset_ref init failed\n");

		goto out_kobj;
	}
#endif

	pr_info("simucamera: init\n");

	return 0;

 on_error:
	if (video_dev)
		video_device_release(video_dev);

	if (g_cam) {
		v4l2_device_unregister(v4l2_dev);
		kfree(g_cam);
		g_cam = NULL;
	}

	return ret;

#ifdef CONFIG_AMD_SIMU_CAMERA_ENABLE_UEVENT
 out_kobj:
	kobject_put(simu_kobj_ref);
	kobject_del(simu_kobj_ref);

 out_kset:
	kset_unregister(simu_kset_ref);

 out:
	return -1;
#endif
}

static void __exit camera_simu_exit(void)
{
#ifdef CONFIG_AMD_SIMU_CAMERA_ENABLE_UEVENT
	if (simu_kobj_ref) {
		kobject_put(simu_kobj_ref);
		kobject_del(simu_kobj_ref);
	}

	if (simu_kset_ref)
		kset_unregister(simu_kset_ref);
#endif
	if (!g_cam)
		return;

	video_unregister_device(g_cam->video_dev);
	v4l2_device_unregister(&g_cam->v4l2_dev);

	kfree(g_cam);

	g_cam = NULL;

	pr_info("simucamera: exit\n");
}

module_init(camera_simu_init);
module_exit(camera_simu_exit);
MODULE_LICENSE("GPL");
