/*
 * Copyright (C) 2019-2020 Advanced Micro Devices, Inc. All rights reserved.
 */

#ifndef __AMD_COMMON_
#define __AMD_COMMON_

#include <linux/types.h>
#include <media/v4l2-device.h>

#define MAX_HW_NUM	10
#define MAX_STREAM_NUM	12
#define MAX_PHYSICAL_CAMERA_NUM	2
#define MAX_STREAM_NUM_PER_PHYSICAL_CAMERA	6
#define DEFAULT_BUF_NUM		4
#define SIZE_ALIGN		64
#define SIZE_ALIGN_DOWN(size) \
	((unsigned int)(SIZE_ALIGN) * \
	((unsigned int)(size) / (unsigned int)(SIZE_ALIGN)))

/* camera hardware device information */
enum cam_hw_type {
	ISP = 0,
	CSI = 1,
	SENSOR = 2,
	VCM = 3,
	FLASH = 4,
};

enum sensor_type {
	SENSOR_SOC = 0,
	SENSOR_RAW = 1,
};

enum sensor_idx {
	CAM_IDX_BACK = 0,
	CAM_IDX_FRONT_L = 1,
	CAM_IDX_FRONT_R = 2,
	CAM_IDX_MAX = 3,
};

struct cam_hw_info {
	int id;
	u32 type;
	struct v4l2_subdev sd;
	/* void * priv; */
};

struct cam_stream;

struct amd_cam {
	struct v4l2_fh fh;
	struct video_device *dev;
	struct v4l2_device v4l2_dev;
	struct cam_hw_info *hw_tbl[MAX_HW_NUM];
	struct physical_cam *phycam_tbl[MAX_PHYSICAL_CAMERA_NUM];

	unsigned int long cam_bit;
	u32 cam_num;
};

struct pipeline_node {
	struct list_head node;
	struct cam_hw_info *hw;
};

#endif /* amd_common.h */
