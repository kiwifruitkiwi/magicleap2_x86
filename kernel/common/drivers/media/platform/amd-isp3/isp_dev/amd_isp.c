/*
 * Copyright (C) 2019-2023 Advanced Micro Devices, Inc. All rights reserved.
 */

#include <linux/compat.h>
#include <linux/slab.h>
#include <linux/firmware.h>
#include <linux/platform_device.h>
#include <media/v4l2-common.h>
#include <linux/videodev2.h>
#include <media/videobuf2-core.h>
#include <linux/cam_api.h>

#include "amd_stream.h"
#include "amd_params.h"
#include "amd_isp.h"

#ifdef AMD_SUPPORT_ISP_TUNING
#include "amd_tuning_sdev.h" /* amd_tuning_register */
#endif

#include "isp_module_cfg.h"
#include "isp_module_if.h"
#include "isp_cam_if.h"
#include "sensor_if.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "[ISP][amd_isp]"

#define ISP_FIRMWARE_RAM "isp/isp_firmware_nawb_isp3.ram"
#define ISP_FIRMWARE_CALIBDB "isp/calibdb_mem_imx577_nawb_isp3.bin"

struct isp_info *isp_hdl;

static int isp_module_load_firmware(int sensor_idx)
{
	int ret = 0;
	const struct firmware *fw;
	unsigned char *fw_path = ISP_FIRMWARE_RAM;

	if (request_firmware(&fw, fw_path, &platform_bus)) {
		ISP_PR_ERR("load firmware [%s] failed", fw_path);
		ret = -1;
		goto failed;
	}
	ISP_PR_INFO("load firmware [%s] success!", fw_path);

#ifndef USING_PSP_TO_LOAD_ISP_FW
	if (isp_hdl->intf->set_fw_bin) {
		ret = isp_hdl->intf->set_fw_bin(isp_hdl->intf->context,
						 (void *)fw->data,
						 fw->size);
		if (ret != IMF_RET_SUCCESS) {
			ISP_PR_ERR("set fw fail, ret = %d", ret);
			goto failed;
		}
		ISP_PR_INFO("set fw success");
	} else {
		ISP_PR_ERR("set fw fail");
		goto failed;
	}
#endif

failed:
	if (fw)
		release_firmware(fw);

	return ret;
}

static int isp_module_load_tuning_data(int sensor_idx)
{
	int ret = 0;
	const struct firmware *cbd_1;
	unsigned char *clib_data = ISP_FIRMWARE_CALIBDB;

	switch (g_prop->sensor_id) {
	case SENSOR_IMX577:
		break;
	case SENSOR_S5K3L6:
		clib_data = "isp/calibdb_mem_S5K3L6_nawb_isp3.bin";
		break;
	default:
		ISP_PR_ERR("sensor id[%d] not supported!", g_prop->sensor_id);
	}

	if (request_firmware(&cbd_1, clib_data, &platform_bus)) {
		ISP_PR_ERR("load calibration data [%s] failed", clib_data);
		ret = -1;
		goto failed;
	}
	ISP_PR_INFO("load calibration data [%s] success!", clib_data);

	if (isp_hdl->intf->set_calib_bin) {
		ret = isp_hdl->intf->set_calib_bin(isp_hdl->intf->context,
			sensor_idx,
			(void *)cbd_1->data,
			cbd_1->size);
		if (ret != IMF_RET_SUCCESS) {
			ISP_PR_ERR("set calibration data fail, ret = %d", ret);
			goto failed;
		}
		ISP_PR_INFO("set calibration data success");
	} else {
		ISP_PR_ERR("set calibration data fail");
		goto failed;
	}

failed:
	if (cbd_1)
		release_firmware(cbd_1);

	return ret;
}

static int isp_module_open_sensor(struct physical_cam *cam,
				  enum mode_profile_name id)
{
	int ret = 0;

	ISP_PR_PC("cam[%d], profile[%d]", cam->idx, id);
	if (isp_hdl->intf->open_camera) {
		ret = isp_hdl->intf->open_camera(isp_hdl->intf->context,
				cam->idx, id, 0);
		if (ret != IMF_RET_SUCCESS) {
			ISP_PR_ERR("sensor[%d] open_camera failed, ret = %d",
				cam->idx, ret);
		} else {
			ISP_PR_INFO("sensor[%d] open_camera suc\n", cam->idx);
			isp_hdl->mask = isp_hdl->mask | 1 << cam->idx;
		}
	} else {
		ISP_PR_ERR("open_camera not supported");
		ret = -1;
	}
	return ret;
}

static int isp_module_close_sensor(int sensor_idx)
{
	int ret = 0;

	if (isp_hdl->intf->close_camera) {
		ret = isp_hdl->intf->close_camera(
			isp_hdl->intf->context, sensor_idx);
		if (ret != IMF_RET_SUCCESS) {
			ISP_PR_ERR("sensor[%d] close_camera failed, ret = %d",
				sensor_idx, ret);
		} else {
			ISP_PR_INFO("sensor[%d] close_camera suc", sensor_idx);
			isp_hdl->mask = isp_hdl->mask & ~(1 << sensor_idx);
		}
	} else {
		ISP_PR_ERR("close_camera not support\n");
		ret = -1;
	}

	return ret;
}

/*
 * utility function for 32bit userspace
 */
struct v4l2_buffer32 {
	__u32	index;
	__u32	type;
	__u32	bytesused;
	__u32	flags;
	__u32	field;
	struct compat_timeval timestamp;
	struct v4l2_timecode	timecode;
	__u32	sequence;
	__u32	memory;
	union {
		__u32	offset;
		compat_long_t	userptr;
		compat_caddr_t	planes;
		__s32	fd;
	} m;
	__u32	length;
	__u32	reserverd2;
	__u32	reserverd;
};

static int isp_ioctl_set_test_pattern(struct physical_cam *cam, void *param)
{
	int ret = 0;

	ENTER();
	if (isp_hdl->intf->set_test_pattern) {
		ret = isp_hdl->intf->set_test_pattern(isp_hdl->intf->context,
						      cam->idx, param);
		if (ret != IMF_RET_SUCCESS) {
			//add this line to trick CP
			ISP_PR_ERR("cam[%d] fail ret=%d", cam->idx, ret);
		} else {
			//add this line to trick CP
			ISP_PR_INFO("cam[%d] suc", cam->idx);
		}
	} else {
		ISP_PR_ERR("not supported");
		ret = -1;
	}

	return ret;
}

static int isp_ioctl_stream_open(struct physical_cam *cam, void *priv)
{
	struct isp_info *isp = v4l2_get_subdevdata(cam->isp_sd);
	int ret = 0;

	ENTER();
	ISP_PR_INFO("isp state [%d]", isp_hdl->state);
	mutex_lock(&isp->mutex);
	if (isp_hdl->state == ISP_READY) {
		/* first time to initialize isp hardware */
		ret = isp_module_load_firmware(cam->idx);
		if (ret) {
			ISP_PR_ERR("fail to load firmware! ret[%d]", ret);
			mutex_unlock(&isp->mutex);
			goto quit;
		}
		isp_hdl->state = ISP_RUNNING;
#ifdef AMD_SUPPORT_ISP_TUNING
		amd_tuning_notify_isp_online(isp->tuning_sd,
				cam->idx);
#endif
	} else if (isp_hdl->state == ISP_RUNNING) {
		ISP_PR_INFO("hardware is already stream on.");
	} else {
		ISP_PR_ERR("fail, isp is in error state [%d]!", isp_hdl->state);
		ret = -1;
		mutex_unlock(&isp->mutex);
		goto quit;
	}

	mutex_unlock(&isp->mutex);
	if (cam->cnt_enq == 0) {
		ret = isp_module_load_tuning_data(cam->idx);
		if (ret) {
			ISP_PR_ERR("fail to load tuning data! ret[%d]", ret);
			goto quit;
		}

		ret = isp_module_open_sensor(cam,
					     *(enum mode_profile_name *)priv);
		if (ret) {
			ISP_PR_ERR("fail to open sensor! ret[%d]", ret);
			goto quit;
		}

		if (isp_hdl->intf->start_stream) {
			ret = isp_hdl->intf->start_stream
				(isp_hdl->intf->context, cam->idx);
			if (ret != IMF_RET_SUCCESS) {
				ISP_PR_ERR("cam[%d] fail ret=%d",
						cam->idx, ret);
			} else {
				//add this line to trick CP
				ISP_PR_INFO("cam[%d] suc", cam->idx);
			}
		} else {
			ISP_PR_ERR("start_stream not supported");
			ret = -1;
		}
	}

quit:
	return ret;
}

static int isp_ioctl_stream_close(struct physical_cam *cam, void *priv)
{
	struct isp_info *isp = v4l2_get_subdevdata(cam->isp_sd);
	int ret = 0;

	ENTER();
	if (isp_hdl->mask & 1 << cam->idx) {
		ret = isp_module_close_sensor(cam->idx);
#ifdef AMD_SUPPORT_ISP_TUNING
		amd_tuning_notify_isp_offline(isp->tuning_sd);
#endif
		INIT_LIST_HEAD(&cam->enq_list);
		INIT_LIST_HEAD(&cam->md_list);
		INIT_LIST_HEAD(&cam->ping.entry);
		INIT_LIST_HEAD(&cam->pong.entry);
		memset(cam->last_stream_info, 0, sizeof(cam->last_stream_info));
		if (cam->ping.md)
			memset(cam->ping.md, 0, MAX_KERN_METADATA_BUF_SIZE);
		if (cam->pong.md)
			memset(cam->pong.md, 0, MAX_KERN_METADATA_BUF_SIZE);
	} else {
		ISP_PR_WARN("sensor[%d] not open yet!", cam->idx);
	}

	mutex_lock(&isp->mutex);
	if (!isp_hdl->mask)
		isp_hdl->state = ISP_READY;
	mutex_unlock(&isp->mutex);
	RET(ret);
	return ret;
}

static int isp_ioctl_s_fmt(struct cam_stream *s)
{
	int ret = 0;

	return ret;
}

static int isp_ioctl_stream_on(struct cam_stream *s, bool *priv)
{
	int ret = 0;

	return ret;
}

static int isp_ioctl_stream_off(struct cam_stream *s, bool *priv)
{
	int ret = 0;

	return ret;
}

struct jpeg_header {
	uint32_t jpeg_buffer_max_size;
	uint32_t jfif_header_size;
	uint32_t jpeg_offset;
	uint32_t jpeg_payload;
	uint32_t thumb_offset;
	uint32_t thumb_payload;
};

static int isp_ioctl_queue_frame_ctrl(struct physical_cam *c,
				      struct _CmdFrameCtrl_t *f)
{
	struct isp_info *isp = v4l2_get_subdevdata(c->isp_sd);
	int ret = 0;

	ENTER();
	ISP_PR_DBG("cam[%d]", c->idx);
	if (!isp->cam[c->idx])
		isp->cam[c->idx] = c;

	ret = isp->intf->set_frame_ctrl(isp->intf->context, c->idx, f);
	if (ret != IMF_RET_SUCCESS)
		ISP_PR_ERR("cam[%d] fail! ret[%d]", c->idx, ret);

	return ret;
}

static int isp_ioctl_switch_profile(struct physical_cam *cam, void *priv)
{
	int ret = 0;
	unsigned int prf_id = *(unsigned int *)priv;
	struct isp_info *isp = v4l2_get_subdevdata(cam->isp_sd);

	ENTER();
	ret = isp->intf->switch_profile(isp->intf->context, cam->idx, prf_id);
	if (ret != IMF_RET_SUCCESS) {
		ISP_PR_ERR("sensor[%d] switch_profile to %d failed, ret = %d",
			   cam->idx, prf_id, ret);
	} else {
		ISP_PR_INFO("sensor[%d] switch_profile to %d suc\n",
			    cam->idx, prf_id);
	}

	return ret;
}

static int isp_ioctl_init_cam_dev(struct physical_cam *cam, void *priv)
{
	struct isp_info *isp = v4l2_get_subdevdata(cam->isp_sd);

	ISP_PR_PC("Init cam[%d] sensor[%s]", cam->idx,
		(g_prop->sensor_id) ? "S5K3L6" : "IMX577");
	init_camera_devices(isp->intf, 0);

	isp_hdl->intf->reg_snr_op(isp_hdl->intf->context, cam->idx,
			get_sensor_interface());
	if (!g_done_isp_pwroff_aft_boot) {
		int ret;

		ret = isp_ip_power_set_by_smu(false);
		ret |= isp_set_clock_by_smu(0, 0, 0);
		g_done_isp_pwroff_aft_boot = 1;
		//just print an error message and ignore it
		//so that camera can work normally
		if (ret)
			ISP_PR_ERR("pwr gate isp after boot fail by %d", ret)
		else
			ISP_PR_INFO("pwr gate isp after boot suc");
	};
	return 0;
}

static int isp_ioctl_get_caps(struct physical_cam *cam, void *priv)
{
	int ret = 0;

	ISP_PR_DBG("cam[%d]", cam->idx);

	if (isp_hdl->intf->get_caps) {
		ret = isp_hdl->intf->get_caps(isp_hdl->intf->context, cam->idx);
		if (ret != IMF_RET_SUCCESS) {
			//add this line to trick CP
			ISP_PR_ERR("sensor[%d] failed ret = %d", cam->idx, ret);
		} else {
			//add this line to trick CP
			ISP_PR_INFO("sensor[%d] success", cam->idx);
		}
	} else {
		ISP_PR_ERR("get_caps not supported");
		ret = -1;
	}

	return ret;
}

static int isp_ioctl_start_cvip_sensor(struct physical_cam *cam)
{
	int ret = 0;

	ISP_PR_DBG("cam[%d]", cam->idx);

	if (isp_hdl->intf->start_cvip_sensor) {
		ret = isp_hdl->intf->start_cvip_sensor(isp_hdl->intf->context,
						       cam->idx);
		if (ret != IMF_RET_SUCCESS) {
			//add this line to trick CP
			ISP_PR_ERR("sensor[%d] failed ret = %d", cam->idx, ret);
		} else {
			//add this line to trick CP
			ISP_PR_INFO("sensor[%d] success", cam->idx);
		}
	} else {
		ISP_PR_ERR("start_cvip_sensor not supported");
		ret = -1;
	}

	return ret;
}

static long  isp_ioctl(struct v4l2_subdev *sd, unsigned int cmd, void *arg)
{
	struct isp_ctl_ctx *ctx = (struct isp_ctl_ctx *)arg;

	ISP_PR_DBG("cmd [%d]", cmd);
	switch (cmd) {
	case ISP_IOCTL_SET_TEST_PATTERN:
		return isp_ioctl_set_test_pattern(ctx->cam, (void *)ctx->ctx);
	case ISP_IOCTL_STREAM_OPEN:
		return isp_ioctl_stream_open(ctx->cam, (void *)ctx->ctx);
	case ISP_IOCTL_S_FMT:
		return isp_ioctl_s_fmt(ctx->stream);
	//case ISP_IOCTL_S_INPUT:
		//return isp_ioctl_s_input
		//		(ctx->stream, (unsigned int *)ctx->ctx);
	case ISP_IOCTL_STREAM_ON:
		return isp_ioctl_stream_on(ctx->stream, (void *)ctx->ctx);
	case ISP_IOCTL_STREAM_OFF:
		return isp_ioctl_stream_off(ctx->stream, (bool *)ctx->ctx);
	//case ISP_IOCTL_QUEUE_BUFFER:
	//	return isp_ioctl_queue_buffer(ctx->stream,
	//				(struct stream_buf *)ctx->ctx);
	case ISP_IOCTL_QUEUE_FRAME_CTRL:
		return isp_ioctl_queue_frame_ctrl(ctx->cam,
					(struct _CmdFrameCtrl_t *)ctx->ctx);
	case ISP_IOCTL_STREAM_CLOSE:
		return isp_ioctl_stream_close(ctx->cam, (void *)ctx->ctx);
	case ISP_IOCTL_SWITCH_SENSOR_PROFILE:
		return isp_ioctl_switch_profile(ctx->cam, (void *)ctx->ctx);
	case ISP_IOCTL_INIT_CAM_DEV:
		return isp_ioctl_init_cam_dev(ctx->cam, (void *)ctx->ctx);
	case ISP_IOCTL_GET_CAPS:
		return isp_ioctl_get_caps(ctx->cam, (void *)ctx->ctx);
	case ISP_IOCTL_START_CVIP_SENSOR:
		return isp_ioctl_start_cvip_sensor(ctx->cam);
	default:
		break;
	}
	ISP_PR_ERR("unknown cmd [%d]", cmd);
	return -EINVAL;
}

static void finish_frame_ctrl(void *param, enum camera_id cid)
{
	unsigned long flags = 0;
	struct frame_control *fc;
	struct physical_cam *cam = isp_hdl->cam[cid];

	if (!cam) {
		ISP_PR_DBG("no frame ctrl enq, should no finish!");
		return;
	}

	spin_lock_irqsave(&cam->b_lock, flags);
	if (list_empty(&cam->enq_list)) {
		ISP_PR_ERR("no active frame control in cam[%d]", cam->idx);
		spin_unlock_irqrestore(&cam->b_lock, flags);
		return;
	}

	fc = list_entry(cam->enq_list.next, struct frame_control, entry);
	list_del(&fc->entry);
	spin_unlock_irqrestore(&cam->b_lock, flags);

	fc->mc = (unsigned long long)param;
	vb2_buffer_done(&fc->vb, VB2_BUF_STATE_DONE);
	ISP_PR_INFO("com_id_%d done", fc->rst.com_req_id);
}

int32_t func_lib_callback(void *context, enum cb_evt_id event,
		void *param, enum camera_id cid)
{

	ISP_PR_DBG("event=[%d]", event);

	switch (event) {
	case CB_EVT_ID_PREV_BUF_DONE:
		break;
	case CB_EVT_ID_STILL_BUF_DONE:
		break;
	case CB_EVT_ID_REC_BUF_DONE:
		break;
	case CB_EVT_ID_TAKE_ONE_PIC_DONE:
		break;
	case CB_EVT_ID_METADATA_DONE:
		break;
	case CB_EVT_ID_ZSL_BUF_DONE:
		break;
	case CB_EVT_ID_RAW_BUF_DONE:
		break;
	case CB_EVT_ID_FRAME_CTRL_DONE:
		finish_frame_ctrl(param, cid);
		break;
	default:
		ISP_PR_ERR("unknown event [%d]!", event);
		return -1;
	}

	RET(0);
	return 0;
}

extern struct isp_isp_module_if *get_isp_module_interface(void);

static int on_isp_registered(struct v4l2_subdev *sd)
{
	enum camera_id cid;
	int ret = 0;

	ENTER();

	isp_hdl = kzalloc(sizeof(*isp_hdl), GFP_KERNEL);
	if (!isp_hdl)
		return -ENOMEM;

	/* add isp information init here */
	isp_hdl->state = ISP_READY;
	isp_hdl->intf = get_isp_module_interface();
	if (!isp_hdl->intf) {
		ISP_PR_ERR("failed to get isp module interface");
		ret = -ENODEV;
		goto err_clean;
	}

	init_camera_devices(isp_hdl->intf, 0);

	for (cid = CAMERA_ID_REAR; cid < CAMERA_ID_MAX; cid++) {

		if (isp_hdl->intf->reg_notify_cb)
			isp_hdl->intf->reg_notify_cb(isp_hdl->intf->context,
					cid, func_lib_callback, NULL);
		if (isp_hdl->intf->reg_snr_op)
			isp_hdl->intf->reg_snr_op(isp_hdl->intf->context, cid,
					get_sensor_interface());
		if (isp_hdl->intf->reg_fw_parser)
			isp_hdl->intf->reg_fw_parser(isp_hdl->intf->context,
					cid, get_fw_parser_interface());
	}
#ifdef AMD_SUPPORT_ISP_TUNING
	isp_hdl->tuning_sd = kzalloc(sizeof(struct v4l2_subdev), GFP_KERNEL);
	if (!isp_hdl->tuning_sd)
		return -ENOMEM;

	amd_tuning_register(sd->v4l2_dev, isp_hdl->tuning_sd, isp_hdl);
#else
	isp_hdl->tuning_sd = NULL;
#endif

	v4l2_set_subdevdata(sd, isp_hdl);

	return 0;
err_clean:
	kfree(isp_hdl->tuning_sd);
	kfree(isp_hdl);
	return -1;
}

void test_camera_release(void)
{
	struct amdisp_sensor_init init;

	init.sensor_info.sensor_id = 0;
	get_sensor_interface()->ioctl(SENSOR_IOCTL_RELEASE, &init);
/*comment out since not necessary for mero.*/
//	get_vcm_interface()->ioctl(VCM_IOCTL_RELEASE, &init);
//	get_flash_interface()->ioctl(FLASH_IOCTL_RELEASE, &init);
//	get_storage_interface()->ioctl(STORAGE_IOCTL_INIT, &init);
}

static void on_isp_unregistered(struct v4l2_subdev *sd)
{
	struct isp_info *isp = v4l2_get_subdevdata(sd);

	ENTER();

	/* do deinitialization accroding to state */

	test_camera_release();

	/* uninit_at_module_exit(); */
	if (isp->intf->unreg_snr_op)
		isp->intf->unreg_snr_op(isp->intf->context, 1);
	//if (isp->intf->unreg_vcm_op)
		//isp->intf->unreg_vcm_op(isp->intf->context, 1);
	//if (isp->intf->unreg_flash_op)
		//isp->intf->unreg_flash_op(isp->intf->context, 1);
	if (isp->intf->unreg_notify_cb)
		isp->intf->unreg_notify_cb(isp->intf->context, 1);

	/* do some hw uninitializtion work */
#ifdef AMD_SUPPORT_ISP_TUNING
	if (isp->tuning_sd)
		amd_tuning_unregister(isp->tuning_sd);
#endif
	kfree(isp->tuning_sd);
	kfree(isp);
}

static struct v4l2_subdev_core_ops isp_core_ops = {
	.ioctl = isp_ioctl,
};

static const struct v4l2_subdev_ops isp_ops = {
	.core = &isp_core_ops,
};

static const struct v4l2_subdev_internal_ops isp_internal_ops = {
	.registered = on_isp_registered,
	.unregistered = on_isp_unregistered,
};

int register_isp_subdev(struct v4l2_subdev *sd, struct v4l2_device *v4l2_dev)
{
	int ret = 0;

	ENTER();

	v4l2_subdev_init(sd, &isp_ops);
	sd->owner = THIS_MODULE;
	snprintf(sd->name, sizeof(sd->name), "%s", "AMD-ISP");
	sd->internal_ops = &isp_internal_ops;

	ret = v4l2_device_register_subdev(v4l2_dev, sd);
	if (!ret) {
		//add this line to trick CP
		ISP_PR_INFO("register subdev [%s]", sd->name);
	} else {
		//add this line to trick CP
		ISP_PR_ERR("failed register subdev [%s]", sd->name);
	}
	return ret;
}

void unregister_isp_subdev(struct v4l2_subdev *sd)
{
	ENTER();

	v4l2_device_unregister_subdev(sd);
}
