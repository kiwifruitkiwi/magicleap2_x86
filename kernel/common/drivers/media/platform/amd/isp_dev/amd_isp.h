/*
 * copyright 2014~2015 advanced micro devices, inc.
 *
 * permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "software"),
 * to deal in the software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the software, and to permit persons to whom the
 * software is furnished to do so, subject to the following conditions:
 *
 * the above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef __AMD_ISP_H_
#define __AMD_ISP_H_

#include "isp_module/inc/isp_module_if.h"
#include "AMDCamera3DrvIspInterface.h"

enum isp_type {
	INTEGRATED = 0,
	DISCRETE = 1,
};

enum isp_state {
	ISP_AVAILABLE = 1,
	ISP_READY = 2,
	ISP_RUNNING = 3,
	ISP_ERROR = 4,
};

enum isp_zsl_mode {
	NON_ZSL_MODE = 0,
	ZSL_MODE = 1,
};

struct isp_info {
	struct list_head active_streams;
	/* it is a quirk to access preview stream and jpeg stream */
	struct cam_stream *s_preview;
	struct cam_stream *s_video;
	struct cam_stream *s_jpeg;
	struct cam_stream *s_zsl;
	struct cam_stream *s_metadata;
	unsigned int running_cnt;
	int type;
	int state;
	int zsl_mode;
	struct mutex mutex;
	struct isp_isp_module_if *intf;
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

int register_isp_subdev(struct v4l2_subdev *sd, struct v4l2_device *v4l2_dev);

void unregister_isp_subdev(struct v4l2_subdev *sd);

#endif /* amd_isp.h */
