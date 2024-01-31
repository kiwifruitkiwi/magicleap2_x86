/*
 * Copyright (C) 2019-2020 Advanced Micro Devices, Inc. All rights reserved.
 */

#ifndef __AMD_STREAM_
#define __AMD_STREAM_

#include <linux/mutex.h>
#include <media/videobuf2-vmalloc.h>
#include <linux/cam_api_simulation.h>

#include "amd_common.h"
#include "amd_params.h"
#include "isp_module_if.h"
#include "amd_metadata.h"

enum stream_type {
	STREAM_PREVIEW = 0,
	STREAM_ZSL,
	STREAM_VIDEO,
	STREAM_JPEG,
	STREAM_METADATA,
};

//enum kernel_stream_type {
//	STREAM_OUTPUT = 0,
//	STREAM_INPUT,
//	STREAM_BIDIRECTIONAL,
//};

/* camera stream creation */
enum stream_state {
	STREAM_UNINIT = 0,
	STREAM_PREPARING = 1,
	/* STREAM_PREPARED = 2, */
	STREAM_ACTIVE = 3,
	/* STREAM_STOP = 4, */
};

enum stream_buf_type {
	STREAM_BUF_VMALLOC = 0,
	STREAM_BUF_DMA,
};

/* buffer struct for a frame */
struct stream_buf {
	/* common v4l buffer stuff -- must be first */
	int buf_type;
	struct vb2_buffer vb;
	struct list_head list;
	struct sys_img_buf_handle handle;

	//store the hal buf info
	__u32 combined_index;
	__u32 virt_cam_id;
	int num_of_bufs;
	unsigned long priv;
	unsigned long bufhandle;
	unsigned long kparam;
	MetaInfo_t *m;
};

struct amd_dma_buf {
	struct device *dev;
	enum dma_data_direction dma_dir;
	struct dma_buf_attachment *db_attach;
	uint64_t dma_fd;
	atomic_t refcount;
	size_t size;
};

struct stream_config {
	u32 sensor_id;
	u32 total_buf_num;
	u32 width;
	u32 height;
	u32 stride;
	struct amd_format fmt;
	void *priv;
};

struct algo_3a_states {
	u8 prv_control_mode;
	u8 prv_scene_mode;
	u8 prv_ae_mode;
	u8 prv_af_mode;
	u8 ae_state;
	u8 af_state;
	u8 awb_state;
};

struct physical_cam {
	u32 sensor_id;
	u32 on_s_cnt;
	u32 yuv_s_cnt;
	u32 partial_results;
	u32 dq_buf_cnt;
	bool shutter_done;
	bool is_metadata_sent;
	u64 shutter_timestamp;
	struct mutex c_mutex;
	spinlock_t s_lock;
	struct v4l2_fh fh;
	struct video_device *dev;
	struct v4l2_subdev *isp_sd;
	struct cam_stream *stream_tbl[MAX_STREAM_NUM_PER_PHYSICAL_CAMERA];
	struct algo_3a_states states;
};

struct cam_stream {
	struct list_head stream_list;
	struct list_head active_buf;
	spinlock_t ab_lock;
	spinlock_t b_lock;
	//u32 buf_cnt;
	struct stream_buf *input_buf;

	struct v4l2_subdev *isp_sd;
	int idx;
	int type;
	int buf_type;
	int q_id;//the buf with q_id should store the enq info from HAL
	int dq_id;//the buf with dq_id should be deq by HAL

	/* stream status */
	int state;

	struct mutex s_mutex;

	/* video capture buffer queue */
	struct vb2_queue vidq;
	spinlock_t q_lock;

	/* input buffer queue */
	struct vb2_queue inq;
	spinlock_t iq_lock;

	struct stream_config cur_cfg;

	atomic_t owned_by_stream_count;	/*Buffers owned by stream */
	wait_queue_head_t done_wq;
};

struct cam_stream *stream_create(struct physical_cam *cam, int id);
void destroy_one_stream(struct cam_stream *stream);
void destroy_all_stream(struct physical_cam *cam);
void store_request_info(struct cam_stream *s, struct kernel_request *req,
	struct kernel_buffer *kbuf);
int streams_config(struct physical_cam *cam,
	struct kernel_stream_configuration *cfg);
int stream_configure(struct physical_cam *cam, struct cam_stream *s,
	struct kernel_stream *stream_cfg);
int stream_vidq_init(struct vb2_queue *q, int buf_type, void *priv);
void stream_config_init(struct physical_cam *cam,
	struct cam_stream *s, struct kernel_stream *hal_cfg);
int stream_set_fmt(struct cam_stream *s);
int alloc_vb2_buf_structure(struct cam_stream *s, int count);
int streams_on(struct physical_cam *cam, struct kernel_stream_indexes *info);
int stream_on(struct cam_stream *s, struct physical_cam *cam);
int stream_off(struct physical_cam *cam, struct cam_stream *s, bool pause);
int enq_request(struct physical_cam *cam, struct kernel_request *req);
int prepare_v4l2_buf(struct kernel_buffer *kbuf, struct cam_stream *s,
	struct v4l2_buffer *buf, struct physical_cam *cam);
int sent_shutter(struct physical_cam *cam, struct kernel_result *rst);
int sent_buf_rst(struct file *f, struct physical_cam *cam,
	struct kernel_result *rst);
int metadata_update_sensor_timestamp(struct amd_cam_metadata *metadata,
				     u64 *timestamp);
int streams_off(struct physical_cam *cam, struct kernel_stream_indexes *info,
	bool pause);
int create_isp_node(void);
int simu_hotplugevent(struct kernel_hotplug_info *hot_plug_info);
int get_sensor_info(struct s_info *info);
void get_imx577_info(struct s_info *info);
int process_3a(struct amd_cam_metadata *metadata, struct physical_cam *cam);
int do_ae(struct amd_cam_metadata *metadata, struct physical_cam *cam);
int do_af(struct amd_cam_metadata *metadata, struct physical_cam *cam);
int do_awb(struct amd_cam_metadata *metadata, struct physical_cam *cam);
void update_3a(struct amd_cam_metadata *metadata, struct physical_cam *cam);
void update_3a_region(u32 tag, struct amd_cam_metadata *metadata);

#endif /* amd_stream.h */
