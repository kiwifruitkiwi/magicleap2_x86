/*
 * Copyright (C) 2019-2021 Advanced Micro Devices, Inc. All rights reserved.
 */

#ifndef __AMD_COMMON_
#define __AMD_COMMON_

#include <linux/types.h>
#include <linux/cam_api.h>
#include <media/v4l2-device.h>
#include "isp_module/src/log.h"

#define DRI_VERSION_MAJOR_SHIFT          (24)
#define DRI_VERSION_MINOR_SHIFT          (16)
#define DRI_VERSION_REVISION_SHIFT       (8)
#define DRI_VERSION_SUB_REVISION_SHIFT   (0)

#define DRI_VERSION_MAJOR_MASK           (0xff << DRI_VERSION_MAJOR_SHIFT)
#define DRI_VERSION_MINOR_MASK           (0xff << DRI_VERSION_MINOR_SHIFT)
#define DRI_VERSION_REVISION_MASK        (0xff << DRI_VERSION_REVISION_SHIFT)
#define DRI_VERSION_SUB_REVISION_MASK   (0xff << DRI_VERSION_SUB_REVISION_SHIFT)

#define DRI_VERSION_MAJOR          (0x3)
#define DRI_VERSION_MINOR          (0x0)
#define DRI_VERSION_REVISION       (0x3)
#define DRI_VERSION_SUB_REVISION   (0x0)
#define DRI_VERSION_STRING "ISP Driver Version: 3.0.3.0"
#define DRI_VERSION (((DRI_VERSION_MAJOR & 0xff) << DRI_VERSION_MAJOR_SHIFT) |\
	((DRI_VERSION_MINOR & 0xff) << DRI_VERSION_MINOR_SHIFT) |\
	((DRI_VERSION_REVISION & 0xff) << DRI_VERSION_REVISION_SHIFT) |\
	((DRI_VERSION_SUB_REVISION & 0xff) << DRI_VERSION_SUB_REVISION_SHIFT))

#define OK			0
#define MAX_HW_NUM		10
#define FW_STREAM_TYPE_NUM	7
#define MAX_REQUEST_DEPTH	10
#define NORMAL_YUV_STREAM_CNT	3
#define MAX_KERN_METADATA_BUF_SIZE 56320 // 55kb
//2^16(Format 16.16=>16 bits for integer part and 16 bits for fractional part)
#define POINT_TO_FLOAT	65536
#define POINT_TO_DOUBLE	4294967296 // 2^32(Format 32.32 )
#define PHYCAM_0_CVPCV_VC_ID	2
#define PHYCAM_1_CVPCV_VC_ID	5
#define STEP_NUMERATOR		1
#define STEP_DENOMINATOR	3
#define SIZE_ALIGN		8
#define SIZE_ALIGN_DOWN(size) \
	((unsigned int)(SIZE_ALIGN) * \
	((unsigned int)(size) / (unsigned int)(SIZE_ALIGN)))
#define SHARP_MAP_CNT		8712 // 33 * 33 * 8
#define HISTOGRAM_DATA_CNT	6144 // 512 * 4 * 3
#define LSC_MAP_CNT		289 // 17 * 17

/* color correction transform values cnt */
#define TRANSFORM_CNT		9
#define BLACK_LEVEL_CNT		4
/* Constant used for mapping of ANDROID_STATISTICS_LENS_SHADING_MAP */
#define LENS_SHADING_MAP_DEFAULT	256
/* range always have max and min values, so the count is 2 */
#define RANGE_VALUES_CNT	2

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
	/* void *priv; */
};

struct cam_stream;

struct amd_cam {
	//struct v4l2_fh fh;
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
