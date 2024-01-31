/* SPDX-License-Identifier: (GPL-2.0 OR BSD-2-Clause)
 *
 * copyright 2020 advanced micro devices, inc.
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
#ifndef __AMD_TUNING_SDEV_
#define __AMD_TUNING_SDEV_

#include <linux/amd_isp_tuning.h>
#include <linux/videodev2.h>
#include <media/v4l2-common.h>
#include <media/videobuf2-core.h>

#include "amd_isp.h" /* struct isp_info */

/*
 * Notify tuning subdev that ISP firmware is online.
 * @sd: The subdev instance of tuning sdev.
 * @sensor_id: the opened sensor ID (camera_id)
 */
int amd_tuning_notify_isp_online(struct v4l2_subdev *sd,
		u32 sensor_id);

/*
 * Notify tuning subdev that ISP firmware was offline.
 * @sd: The subdev instance of tuning sdev.
 */
int amd_tuning_notify_isp_offline(struct v4l2_subdev *sd);


/*
 * Register tuning subdev. If this function return -ENOSYS, it means the
 * current ISP driver doesn't support tuning feature.
 *  @return 0 indicates OK, -ENOSYS for not supported.
 */
int amd_tuning_register(struct v4l2_device *dev, struct v4l2_subdev *sd,
		struct isp_info *hdl);

/*
 * Unregister tuning subdev and release all resource.
 *  @sb: the subdev which has been registered.
 */
void amd_tuning_unregister(struct v4l2_subdev *sb);

#endif /* amd_tuning_sdev.h */

