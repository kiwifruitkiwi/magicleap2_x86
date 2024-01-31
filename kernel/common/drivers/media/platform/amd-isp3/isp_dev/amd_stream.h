/*
 * Copyright (C) 2019-2022 Advanced Micro Devices, Inc. All rights reserved.
 */

#ifndef __AMD_STREAM_
#define __AMD_STREAM_

#include <linux/amd_cam_metadata.h>
#include <linux/mutex.h>
#include <media/videobuf2-vmalloc.h>
#include <linux/cam_api.h>

#include "amd_common.h"
#include "amd_params.h"
#include "isp_module_if.h"
#include "isp_module/src/log.h"

/* the frame duration of min FPS 15 */
#define MAX_FRAME_DURATION	67
#define TEAR_DOWN_TIMEOUT	msecs_to_jiffies(MAX_FRAME_DURATION * \
						 MAX_REQUEST_DEPTH)

#define SENSOR_SWITCH_DISABLE	0
#define SENSOR_SWITCH_ENABLE	1
#define CLOCK_SWITCH_DISABLE	0
#define CLOCK_SWITCH_ENABLE	1

/* switch sensor profile id*/
#define SWITCH_SENSOR_PRF_IDX	0
#define SWITCH_TUNING_DATA_IDX	1
/* If sensor profile switch from 12M@30FPS/3M@60FPS to other profiles, need to
 * low clock after all the previous frames are returned; set SWITCH_LOW_CLK_IDX
 * to CLOCK_SWITCH_ENABLE for the first frame control of the new profile, so
 * when kernel receive it from ISP, which shows can low clock now.
 */
#define SWITCH_LOW_CLK_IDX	15

enum stream_buf_type {
	STREAM_BUF_VMALLOC = 0,
	STREAM_BUF_DMA,
};

enum set_clk_base_on_prf {
	NO_NEED_LOWER_CLK = 0,
	NEED_CHECK_RESP   = 1,
	NEED_LOWER_CLK    = 2,
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
	u32 width;
	u32 height;
	struct amd_format fmt;
	void *priv;
};

struct metadata_entry {
	struct list_head entry;
	struct amd_cam_metadata *md;
};

struct stream_info {
	u32 enable;
	u32 width;
	u32 height;
	u32 y_stride;
	u32 uv_stride;
	enum pvt_img_fmt fmt;
};

struct physical_cam {
	int buf_type;/*type should be same with HAL buf type*/
	int idx;/*CAMERA_ID_REAR or CAMERA_ID_FRONR_LEFT ...*/
	bool cam_init;
	bool cam_on;
	bool tear_down;
	enum set_clk_base_on_prf clk_tag;
	wait_queue_head_t done_wq;

	//for v4l2
	struct v4l2_subdev *isp_sd;
	struct video_device *dev;
	struct mutex c_lock;/*lock for cam*/
	struct v4l2_fh fh;
	struct vb2_queue vidq;
	spinlock_t q_lock;/*spinlock for vb2q*/

	//for fc
	int fc_id;/*the fc will be enq*/
	int cnt_enq;
	int fc_id_offset;
	struct list_head enq_list;
	spinlock_t b_lock;/*spinlock for fc*/
	struct frame_control *fc;/*the fc which will be deq by HAL*/
	struct _FrameControl_t *pre_pfc;

	//for request
	struct kernel_request *req;/*the enq request from HAL*/

	//for comparison of parameters
	struct list_head md_list;
	struct metadata_entry ping;
	struct metadata_entry pong;

	//for parameters response
	struct amd_cam_metadata *md;

	//used to store the last stream info received in PFC
	//it will be used to compare with new received PFC,
	//if stream setting is changed, key log info will be output
	//and then it will be updated with the new PFC
	//enum stream_type is used as index
	struct stream_info last_stream_info[MAX_STREAM_NUM_PER_PHYSICAL_CAMERA];

	enum _AfMode_t af_mode;
	enum camera_metadata_enum_amd_cam_control_af_trigger af_trigger;
	enum camera_metadata_enum_amd_cam_control_af_state af_state;
	enum camera_metadata_enum_amd_cam_control_ae_precapture_trigger
		ae_trigger;
	enum _AwbMode_t cur_awb_mode;
	enum _AwbMode_t pre_awb_mode;
	enum camera_metadata_enum_amd_cam_color_correction_mode cur_cc_mode;
	enum camera_metadata_enum_amd_cam_color_correction_mode pre_cc_mode;
};

/* buffer struct for a frame control */
struct frame_control {
	struct vb2_buffer vb;
	struct list_head entry;
	struct kernel_result rst;/*store the HAL buf info*/
	struct kernel_buffer output_buffers[MAX_STREAM_NUM_PER_PHYSICAL_CAMERA];
	uintptr_t hal_md_buf_addr;/*store HAL metadata buffer user space addr*/
	struct _CmdFrameCtrl_t fw_fc;/*store the frame control info for FW*/
	struct isp_gpu_mem_info *mi_mem_info;
	struct _MetaInfo_t *mi;/*store the meta info (result) from FW*/
	unsigned long long mc;/*store the meta info mc addr from FW*/
	void *eng_ptr;
	struct isp_gpu_mem_info *eng_info;
	struct _EngineerMetaInfo_t *eng;
	enum mode_profile_name prf_id;
};

int set_prop(struct physical_cam *cam, struct s_properties *prop);
int sensor_config(struct physical_cam *cam, struct module_profile *cfg);
int sensor_open(struct physical_cam *cam);
int enq_request(struct physical_cam *cam, struct kernel_request *req);
int map_stream_buf(struct physical_cam *cam, struct frame_control *fc);
void map_img_addr(struct _OutputBuf_t *obuf, struct kernel_buffer *kbuf);
int map_metadata_2_framectrl(struct physical_cam *cam,
				struct frame_control *fc);
void get_curve(struct amd_cam_metadata_entry e, struct _FrameControl_t *f);
void get_slope(s32 *p1, s32 *p2, s32 *b, s32 *c, s32 *x1, s32 *x2, s32 *y1,
	       s32 *y2);
bool entry_cmp(u64 *mask, u32 tag, enum _FrameControlBitmask_t bit,
	       struct amd_cam_metadata *new, struct amd_cam_metadata *old,
	       struct amd_cam_metadata_entry *e,
	       struct amd_cam_metadata_entry *old_e);
int store_request_info(struct physical_cam *cam, struct frame_control *fc);
void init_rst(struct kernel_result *rst, u32 id, bool shutter,
	      u64 timestamp, u64 buf_cnt);
int deq_result(struct physical_cam *cam, struct kernel_result *rst,
	       struct file *f);
void send_shutter(struct physical_cam *cam, struct kernel_result *kr);
int send_buf_rst(struct physical_cam *cam, struct kernel_result *kr);
int unmap_metadata_2_framectrl(struct physical_cam *cam);
int store_latest_md(struct physical_cam *cam, struct amd_cam_metadata *src);
void check_buf_status(struct _BufferMetaInfo_t *src, struct kernel_buffer *dst,
		      int cid, int sid);
int sensor_close(struct physical_cam *cam);
int init_frame_control(struct physical_cam *cam);
int init_video_dev(struct physical_cam *cam, struct v4l2_device *vdev, int id);
int init_vb2q(struct physical_cam *cam);
int alloc_vb2_buf_structure(struct physical_cam *cam, int count);
void physical_cam_destroy(struct physical_cam *cam);
void destroy_all_md(struct physical_cam *cam);
int hot_plug_event(struct kernel_hotplug_info *hot_plug_info);
int get_sensor_info(struct s_info *info);
extern struct isp_gpu_mem_info *isp_mem_map_info;
extern struct isp_gpu_mem_info *isp_mem_map_info_still;
extern struct isp_gpu_mem_info *isp_mem_map_info_video;
int create_isp_node(void);
extern int amd_pci_init(void);
extern void amd_pci_exit(void);
#endif /* amd_stream.h */
