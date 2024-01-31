/*
 * camera_simu.h
 *
 * Copyright (C) 2020 Advanced Micro Devices
 *
 * This file is licensed under the terms of the GNU General Public License
 * version 2. This program is licensed "as is" without any warranty of any
 * kind, whether express or implied.
 *
 */

#ifndef __UAPI_LINUX_SIMUCAM_H
#define __UAPI_LINUX_SIMUCAM_H

#include <linux/videodev2.h>

enum simu_event {
	V4L2_EVENT_SIMU_CAM = V4L2_EVENT_PRIVATE_START + 100,
};

enum simu_id {
	SIMU_SET_FMT,
	SIMU_STREAM_ON,
	SIMU_SET_PARAM,
	SIMU_QBUF,
	SIMU_REQUEST,
};

struct simu_cam_fmt {
	__u32 type;
	__u32 width;
	__u32 height;
	__u32 pixelformat;
	__u32 sid;
};

struct simu_cam_on {
	__u32 on;
	__u32 sid;
};

struct simu_cam_param {
	__u32 ctrl_id;
	void *ctrl_data;
	__u32 sid;
};

struct simu_cam_buf {
	__u32 index;
	__u32 sid;
	void *data;
	__u32 combined_index;
	__u32 virt_cam_id;
	unsigned long priv;
	unsigned long bufhandle;
	unsigned long metadata;
};

struct simu_evt_request {
	__u64 request;
	__u32 buf_num;
	__u8 buf_idx;
	__u32 sid;
	__u32 combined_index;
	__u32 virt_cam_id;
	unsigned long priv;
	unsigned long bufhandle;
	unsigned long metadata;
};
#endif
