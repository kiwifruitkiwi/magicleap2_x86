/*
 * Copyright (C) 2019-2023 Advanced Micro Devices, Inc. All rights reserved.
 */

#ifndef __AMD_ISP_H_
#define __AMD_ISP_H_

#include "isp_module/inc/isp_module_if.h"
#include "AMDCamera3DrvIspInterface.h"
#include "isp_module/src/log.h"

#define VIDEO_DEV_BASE		1
// remove it when can get real info from HW
#define PHYSICAL_CAMERA_CNT 2

enum isp_type {
	INTEGRATED = 0,
	DISCRETE = 1,
};

enum isp_state {
	ISP_READY = 1,
	ISP_RUNNING = 2,
	ISP_ERROR = 3,
};

enum isp_zsl_mode {
	NON_ZSL_MODE = 0,
	ZSL_MODE = 1,
};

struct isp_info {
	struct physical_cam *cam[PHYSICAL_CAMERA_CNT];
	int state;
	struct mutex mutex;
	struct isp_isp_module_if *intf;

	/* tuning sub device, if not support, this field will be NULL */
	struct v4l2_subdev *tuning_sd;
	__u32 mask;
};

/* isp control command */
struct isp_ctl_ctx {
	struct physical_cam *cam;
	struct cam_stream *stream;
	void *ctx;
};

enum roi_type {
	ROI_TYPE_AF = 1,
	ROI_TYPE_AE,
	ROI_TYPE_AWB,
};

struct roi_region {
	int roi_type;
	int left;
	int top;
	int w;
	int h;
};

#define ISP_IOCTL_STREAM_OPEN	_IOWR('I', 0, struct isp_ctl_ctx)
#define ISP_IOCTL_S_FMT		_IOWR('I', 1, struct isp_ctl_ctx)
#define ISP_IOCTL_S_INPUT	_IOWR('I', 2, struct isp_ctl_ctx)
#define ISP_IOCTL_STREAM_ON	_IOWR('I', 3, struct isp_ctl_ctx)
#define ISP_IOCTL_STREAM_OFF	_IOWR('I', 4, struct isp_ctl_ctx)
#define ISP_IOCTL_QUEUE_BUFFER	_IOWR('I', 5, struct isp_ctl_ctx)
#define ISP_IOCTL_STREAM_CLOSE	_IOWR('I', 6, struct isp_ctl_ctx)
#define ISP_IOCTL_S_CTRL	_IOWR('I', 7, struct isp_ctl_ctx)
#define ISP_IOCTL_G_CTRL	_IOWR('I', 8, struct isp_ctl_ctx)
#define ISP_IOCTL_S_ROI		_IOWR('I', 9, struct isp_ctl_ctx)
#define ISP_IOCTL_QUEUE_FRAME_CTRL	_IOWR('I', 10, struct isp_ctl_ctx)
#define ISP_IOCTL_SET_TEST_PATTERN	_IOWR('I', 11, struct isp_ctl_ctx)
#define ISP_IOCTL_SWITCH_SENSOR_PROFILE	_IOWR('I', 12, struct isp_ctl_ctx)
#define ISP_IOCTL_INIT_CAM_DEV	_IOWR('I', 13, struct isp_ctl_ctx)
#define ISP_IOCTL_GET_CAPS	_IOWR('I', 14, struct isp_ctl_ctx)
#define ISP_IOCTL_START_CVIP_SENSOR	_IOWR('I', 15, struct isp_ctl_ctx)

int register_isp_subdev(struct v4l2_subdev *sd, struct v4l2_device *v4l2_dev);

void unregister_isp_subdev(struct v4l2_subdev *sd);
extern struct s_properties *g_prop;
extern int g_done_isp_pwroff_aft_boot;
int isp_ip_power_set_by_smu(uint8_t enable);
int isp_set_clock_by_smu(unsigned int sclk, unsigned int iclk,
			 unsigned int xclk);
#endif /* amd_isp.h */
