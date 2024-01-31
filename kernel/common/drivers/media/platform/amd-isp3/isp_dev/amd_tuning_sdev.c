// SPDX-License-Identifier: (GPL-2.0 OR BSD-2-Clause)
/*
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
#include "amd_tuning_sdev.h"

#include "amd_isp.h" /* struct isp_info */

#include <media/v4l2-device.h>
#include <media/v4l2-subdev.h>

#include <linux/amd_isp_tuning.h>  /* uapi */
#include <linux/platform_device.h>
#include <linux/types.h>  /* u32 */

/* checksum constant */
#define AMD_TUNING_CONTEXT_CHECKSUM 0xa0b0c0d0

#define my_logd(f, ...) \
	{ ISP_PR_DBG("amd_tuning_sdev:%s: " f, __func__, ##__VA_ARGS__); }

#define my_logi(f, ...) \
	{ ISP_PR_INFO("amd_tuning_sdev:%s: " f, __func__, ##__VA_ARGS__); }

#define my_logw(f, ...) \
	{ ISP_PR_WARN("amd_tuning_sdev:%s: " f, __func__, ##__VA_ARGS__); }

#define my_loge(f, ...) \
	{ ISP_PR_ERR("amd_tuning_sdev:%s: " f, __func__, ##__VA_ARGS__); }

/*
 * Tuning subdev state machine. After registered, the default status is
 * invalid which means the operations of online tuning will be rejected.
 * @AMD_TUNING_SDEV_STAT_INVALID: Invalid state, all operations from userspace
 *      will be rejected.
 * @AMD_TUNING_SDEV_STAT_OK: Notified by caller (amd_isp.c), it's ready to
 *      online tuning.
 */
enum amd_tuning_sdev_status {
	AMD_TUNING_SDEV_STAT_INVALID = 0,
	AMD_TUNING_SDEV_STAT_OK
};


/*
 * Tuning context, saving resource for the registered tuning subdev. All users
 * will share the same amd_tuning_context.
 * @status: The tuning subdev status, see enum amd_tuning_sdev_status for
 * more information.
 * @isp_hdl: struct isp_info instance passed by ISP subdev.
 * @mutex: resource lock of tuning context.
 * @sensorid: the opened sensor ID, will be updated by function
 * `amd_tuning_notify_isp_online`, default is -1.
 * @checksum: constant equals to AMD_TUNING_CONTEXT_CHECKSUM, for checking
 *      usage.
 */
struct amd_tuning_context {
	enum amd_tuning_sdev_status status;
	struct isp_info *isp_hdl;
	struct mutex mutex;
	s32 sensorid;
	u32 checksum;
};


/*
 * check the given context is valid or not.
 * @ctx: Context to be checked.
 * @return: 0 for invalid, 1 for valid.
 */
inline int is_valid_context(const struct amd_tuning_context *ctx)
{
	if (ctx == NULL) {
		my_loge("ctx is NULL");
		return 0;
	}
	if (ctx->checksum != AMD_TUNING_CONTEXT_CHECKSUM) {
		my_loge("checksum doesn't match");
		return 0;
	}
	return 1;
}


/*
 * mmap userspace's address to view of kernel and ISP firmware
 * @src: pointer of the source address from userspace.
 * @addr_lo: lower 32 bits of address.
 * @addr_hi: higher 32 bits of address.
 * @target: target view for kernel
 * @return: 0 indicates to ok, otherwise checks the error code.
 */
static int mmap_userspace_addr(u32 addr_lo, u32 addr_hi, u64 *target)
{
	*target = (u64)addr_lo | ((u64)addr_hi << 32);
	return 0;
}


/*
 * ummap userspace's address.
 * @src: address has been mmapped.
 */
static int unmap_userspace_addr(u32 addr_lo, u32 addr_hi)
{
	/* no need to ummap */
	return 0;
}


/*
 * The function to communicate to ISP firmware for tuning.
 * @ctx: Context of this tuning subdev.
 * @p: The tuning parcel sent from userspace.
 */
static int isp_online_tune(
		struct amd_tuning_context *ctx,
		struct amd_isp_tuning *p)
{
	u64 p_mmap_data;
	u8  *temp_memory;
	u8  *resp_payload;
	u32 resp_size;
	int ret;

	/* mmap userspace's addr first */
	ret = mmap_userspace_addr(
			p->tuning_buf_addr_lo,
			p->tuning_buf_addr_hi,
			&p_mmap_data);

	if (ret != 0) {
		my_loge("mmap address (hi=%#x, lo=%#x) from caller failed",
				p->tuning_buf_addr_hi,
				p->tuning_buf_addr_lo);
		return -EFAULT;
	}

	/* allocate memory */
	temp_memory = vmalloc(p->tuning_buf_size);
	if (!temp_memory) {
		my_loge("allocate virtual memory failed");
		return -ENOMEM;
	}

	/* response payload size was fixed at 36 bytes */
	resp_payload = p->result.payload;
	resp_size = sizeof(p->result.payload);

	/* copy data from user space */
	memcpy(temp_memory, (void *)p_mmap_data, p->tuning_buf_size);

	/* acquire context lock */
	mutex_lock(&ctx->mutex);
	if (ctx->status == AMD_TUNING_SDEV_STAT_INVALID) {
		my_loge("device is not ready");
		ret = -EBUSY;
		goto lb_exit;
	}

	if (ctx->isp_hdl->intf->online_isp_tune) {
		int result = 0;

		result = ctx->isp_hdl->intf->online_isp_tune(
				ctx->isp_hdl->intf->context,  /* context */
				(enum camera_id)ctx->sensorid,  /* sensor id */
				p->cmd,  /* tuning command */
				(unsigned int)p->is_set_cmd,
				(unsigned short)p->is_stream_cmd,
				(unsigned int)p->is_direct_cmd,
				(void *)temp_memory,  /* data chunk */
				(unsigned int *)&p->tuning_buf_size,
				(void *)resp_payload,
				resp_size);

		p->result.error_code = (s32)result;
		if (result != 0)
			my_logw("online_isp_tune failed, errcode=%d", result);

		my_logi("online_isp_tune returns %d", result);
	} else {
		my_loge("not support online tuning (no func ptr)");
		ret = -EINVAL;
		goto lb_exit;
	}


lb_exit:
	mutex_unlock(&ctx->mutex);
	if (temp_memory) {
		/* if ok, copy data back if it's not a set cmd. */
		if (ret == 0 && p->is_set_cmd == 0) {
			memcpy((void *)p_mmap_data, temp_memory,
					p->tuning_buf_size);
		}
	}

	unmap_userspace_addr(p->tuning_buf_addr_lo,
			p->tuning_buf_addr_hi);

	/* always free temp memory */
	vfree(temp_memory);
	return ret;
}


/*
 * Implementation of private ioctl ops.
 */
static long tuning_ioctl(struct v4l2_subdev *sd, unsigned int cmd, void *arg)
{
	long ret = -EINVAL;
	struct amd_tuning_context *ctx = NULL;

	ctx = v4l2_get_subdev_hostdata(sd);
	if (!is_valid_context(ctx)) {
		my_loge("invalid amd_tuning_context, can't execute cmd %d",
			cmd);
		return -EINVAL;
	}

	my_logd("[+]");
	switch (cmd) {
	case V4L2_CID_AMD_ISP_TUNE:
		ret = (long)isp_online_tune(ctx,
				(struct amd_isp_tuning *)(arg));
		my_logd("[-]");
		return ret;

	default:
		break;
	}

	my_loge("unknown cmd [%d]", cmd);
	my_logd("[-]");
	return ret;
}


static struct v4l2_subdev_core_ops s_core_ops = {
	.ioctl = tuning_ioctl,
};


static const struct v4l2_subdev_ops s_ops = {
	.core = &s_core_ops,
};


static int internal_ops_registered(struct v4l2_subdev *sd)
{
	my_logi("v4l2 tuning subdev (%p) has been registered", sd);
	return 0;
}


static void internal_ops_unregistered(struct v4l2_subdev *sd)
{
	my_logi("v4l2 tuning subdev (%p) has been unregistered", sd);
}


static int internal_ops_open(struct v4l2_subdev *sd, struct v4l2_subdev_fh *fh)
{
	my_logi("v4l2 tuning subdev (%p) is being opened", sd);
	return 0;
}


static int internal_ops_close(struct v4l2_subdev *sd, struct v4l2_subdev_fh *fh)
{
	my_logi("v4l2 tuning subdev (%p) is being closed", sd);
	return 0;
}


static const struct v4l2_subdev_internal_ops s_internal_ops = {
	.registered = internal_ops_registered,
	.unregistered = internal_ops_unregistered,
	.open = internal_ops_open,
	.close = internal_ops_close,
};


int amd_tuning_notify_isp_online(struct v4l2_subdev *sd, u32 sensor_id)
{
	struct amd_tuning_context *ctx = v4l2_get_subdev_hostdata(sd);

	if (!is_valid_context(ctx)) {
		my_loge("failed due to invalid context");
		return -EINVAL;
	}

	mutex_lock(&ctx->mutex);
	ctx->status = AMD_TUNING_SDEV_STAT_OK;
	ctx->sensorid = (s32)sensor_id;
	mutex_unlock(&ctx->mutex);

	return 0;
}


int amd_tuning_notify_isp_offline(struct v4l2_subdev *sd)
{
	struct amd_tuning_context *ctx = v4l2_get_subdev_hostdata(sd);

	if (!is_valid_context(ctx)) {
		my_loge("failed due to invalid context");
		return -EINVAL;
	}

	mutex_lock(&ctx->mutex);
	ctx->status = AMD_TUNING_SDEV_STAT_INVALID;
	ctx->sensorid = -1;
	mutex_unlock(&ctx->mutex);

	return 0;
}


int amd_tuning_register(struct v4l2_device *dev, struct v4l2_subdev *sd,
		struct isp_info *hdl)
{
	int ret = 0;

	/* init subdev as a core device */
	v4l2_subdev_init(sd, &s_ops);
	/* expose as a /dev/v4l-subdevX */
	sd->flags = V4L2_SUBDEV_FL_HAS_DEVNODE;
	sd->owner = THIS_MODULE;
	sd->internal_ops = &s_internal_ops;
	snprintf(sd->name, sizeof(sd->name), "%s", AMD_ISP_TUNING_SUBDEV_NAME);

	/* register this subdev to the given v4l2_device |dev| */
	ret = v4l2_device_register_subdev(dev, sd);
	if (!ret) {
		struct amd_tuning_context *ctx;

		/* allocate amd_tuning_context */
		ctx = kzalloc(sizeof(struct amd_tuning_context), GFP_KERNEL);
		if (!ctx) {
			my_loge("cannot kzalloc mem for amd_tuning_context");
			return -ENOMEM;
		}
		/* init amd_tuning_context */
		mutex_init(&ctx->mutex);
		ctx->isp_hdl = hdl;
		ctx->status = AMD_TUNING_SDEV_STAT_INVALID;
		ctx->checksum = AMD_TUNING_CONTEXT_CHECKSUM;
		ctx->sensorid = -1;

		/* set the context to the corresponding v4l2 subdev data */
		v4l2_set_subdev_hostdata(sd, ctx);

		my_logi("register subdev(%p)%s,ctx=%p,hotstdata=%p",
			sd, sd->name, ctx,
			v4l2_get_subdev_hostdata(sd));
	} else
		my_loge("failed to register subdev [%s]", sd->name);


	return ret;
}


void amd_tuning_unregister(struct v4l2_subdev *sd)
{
	struct amd_tuning_context *ctx;

	ctx = v4l2_get_subdev_hostdata(sd);
	/* free context memory */
	kfree(ctx);
	v4l2_device_unregister_subdev(sd);
}
