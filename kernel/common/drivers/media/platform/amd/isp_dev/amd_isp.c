/*
 * Copyright (C) 2019-2020 Advanced Micro Devices, Inc. All rights reserved.
 */

#include <linux/compat.h>
#include <linux/slab.h>
#include <linux/firmware.h>
#include <linux/platform_device.h>
#include <media/v4l2-common.h>
#include <linux/videodev2.h>
#include <media/videobuf2-core.h>
#include <linux/cam_api_simulation.h>

#include "amd_common.h"
#include "amd_stream.h"
#include "amd_params.h"
#include "amd_isp.h"
#include "isp_module_cfg.h"
#include "isp_module_if.h"
#include "isp_cam_if.h"
#include "isp_common.h"
#include "sensor_if.h"
#include "amd_log.h"

#define ISP_FIRMWARE_RAM "isp/isp_firmware_nawb.ram"
#define ISP_FIRMWARE_CALIBDB "isp/calibdb_mem_imx208_nawb.bin"

struct isp_info *isp_hdl;

static int isp_module_load_firmware(int sensor_idx)
{
	int ret = 0;
	const struct firmware *fw, *cbd_1 = NULL;
	unsigned char *fw_path = ISP_FIRMWARE_RAM;
	unsigned char *clib_data = ISP_FIRMWARE_CALIBDB;

	logd("enter %s", __func__);

	if (request_firmware(&fw, fw_path, &platform_bus)) {
		loge("load firmware [%s] failed", fw_path);
		ret = -1;
		goto failed;
	}
	logi("load firmware [%s] success!", fw_path);

	if (request_firmware(&cbd_1, clib_data, &platform_bus)) {
		loge("load calibration data [%s] failed", clib_data);
		ret = -1;
		goto failed;
	}
	logi("load calibration data [%s] success!", clib_data);

	if (isp_hdl->intf->set_fw_bin) {
		ret = isp_hdl->intf->set_fw_bin(isp_hdl->intf->context,
						 (void *)fw->data,
						 fw->size);
		if (ret != IMF_RET_SUCCESS) {
			loge("set fw fail, ret = %d", ret);
			goto failed;
		}
		logi("set fw success");
	} else {
		loge("set fw fail");
		goto failed;
	}

	if (isp_hdl->intf->set_calib_bin) {
		ret = isp_hdl->intf->set_calib_bin(isp_hdl->intf->context,
			sensor_idx,
			(void *)cbd_1->data,
			cbd_1->size);
		if (ret != IMF_RET_SUCCESS) {
			loge("set calibration data fail, ret = %d", ret);
			goto failed;
		}
		logi("set calibration data success");
	} else {
		loge("set calibration data fail");
		goto failed;
	}

failed:
	if (fw)
		release_firmware(fw);
	if (cbd_1)
		release_firmware(cbd_1);

	return ret;
}

static int isp_module_open_sensor(struct cam_stream *s)
{
	int ret = 0;

	logd("enter %s", __func__);

	if (isp_hdl->intf->open_camera) {
		ret =
		isp_hdl->intf->open_camera(isp_hdl->intf->context,
				s->cur_cfg.sensor_id, 0, 0);
		if (ret != IMF_RET_SUCCESS)
			loge("isp module open camera failed, ret = %d", ret);
		else
			logi("isp module open sensor [%d] success",
				s->cur_cfg.sensor_id);
	} else {
		loge("isp module open camera is NULL");
		ret = -1;
	}
	return ret;
}

static int isp_module_close_sensor(int sensor_idx)
{
	int ret = 0;

	logd("enter %s", __func__);
	/* FIXME: add implementation */
	if (isp_hdl->intf->close_camera) {
		ret =
			isp_hdl->intf->close_camera(isp_hdl->intf->context,
				sensor_idx);
		if (ret != IMF_RET_SUCCESS)
			loge("isp module close camera init failed, ret = %d",
				ret);
		else
			logi("isp module close sensor init [%d] success",
				sensor_idx);
	} else {
		loge("isp module close camera init is NULL");
		ret = -1;
	}

	return ret;
}

static int isp_module_set_stream_para(struct cam_stream *s)
{
	int ret = 0;
	struct pvt_img_res_fps_pitch pvt_cfg;
	enum pvt_img_fmt fmt_cfg = PVT_IMG_FMT_NV12;

	logd("enter %s", __func__);

	pvt_cfg.width = s->cur_cfg.width;
	pvt_cfg.height = s->cur_cfg.height;
	pvt_cfg.fps = 30;//15;
	/* TODO: re-cacluate stride
	 * hard code the stride for 1080p temporarily
	 */
	pvt_cfg.luma_pitch = s->cur_cfg.stride;
	pvt_cfg.chroma_pitch = s->cur_cfg.stride;
	fmt_cfg = s->cur_cfg.fmt.format;

	if (s->type == STREAM_PREVIEW && isp_hdl->intf->set_preview_para) {
		ret = isp_hdl->intf->set_preview_para(isp_hdl->intf->context,
						s->cur_cfg.sensor_id,
						PARA_ID_DATA_RES_FPS_PITCH,
						&pvt_cfg);
		if (ret != IMF_RET_SUCCESS) {
			loge("isp module set preview res fps failed, ret=%d",
				ret);
			return -1;
		}
		ret = isp_hdl->intf->set_preview_para(isp_hdl->intf->context,
						s->cur_cfg.sensor_id,
						PARA_ID_DATA_FORMAT,
						&fmt_cfg);
		if (ret != IMF_RET_SUCCESS) {
			loge("isp module set preview format failed, ret=%d",
				ret);
			return -1;
		}
	}

	if (s->type == STREAM_ZSL && isp_hdl->intf->set_zsl_para) {
		ret = isp_hdl->intf->set_zsl_para(isp_hdl->intf->context,
				s->cur_cfg.sensor_id,
				PARA_ID_DATA_RES_FPS_PITCH,
				&pvt_cfg);

		if (ret != IMF_RET_SUCCESS) {
			loge("isp module set zsl res fps failed, ret=%d", ret);
			return -1;
		}

		ret = isp_hdl->intf->set_zsl_para(isp_hdl->intf->context,
					s->cur_cfg.sensor_id,
					PARA_ID_DATA_FORMAT,
					&fmt_cfg);

		if (ret != IMF_RET_SUCCESS) {
			loge("isp module set zsl format failed, ret=%d", ret);
			return -1;
		}
	}

	if (s->type == STREAM_VIDEO && isp_hdl->intf->set_video_para) {
		ret = isp_hdl->intf->set_video_para(isp_hdl->intf->context,
						s->cur_cfg.sensor_id,
						PARA_ID_DATA_RES_FPS_PITCH,
						&pvt_cfg);
		if (ret != IMF_RET_SUCCESS) {
			loge("isp module set record res fps  failed, ret=%d",
				ret);
			return -1;
		}
		ret = isp_hdl->intf->set_video_para(isp_hdl->intf->context,
						s->cur_cfg.sensor_id,
						PARA_ID_DATA_FORMAT,
						&fmt_cfg);
		if (ret != IMF_RET_SUCCESS) {
			loge("isp module set record format failed, ret=%d",
				ret);
			return -1;
		}
	}
	return 0;
}

static int isp_module_start_preview(int sensor_idx)
{
	int ret = 0;

	logd("enter %s", __func__);

	if (isp_hdl->intf->start_preview) {
		ret = isp_hdl->intf->start_preview(isp_hdl->intf->context,
			sensor_idx);
		if (ret != IMF_RET_SUCCESS)
			loge("isp module start preview fail, ret=%d", ret);
		else
			logi("isp module start sensor[%d] preview success",
				sensor_idx);
	} else {
		loge("isp module start_preview is NULL");
		ret = -1;
	}
	return ret;
}

static int isp_module_stop_preview(int sensor_idx, bool pause)
{
	int ret = 0;

	logd("enter %s", __func__);
	if (isp_hdl->intf->stop_preview) {
		ret =
			isp_hdl->intf->stop_preview(isp_hdl->intf->context,
						sensor_idx, pause);
		if (ret != IMF_RET_SUCCESS)
			logd("isp module stop preview fail, ret=%d", ret);
		else
			logi("isp module stop sensor[%d] preview scuuess",
				sensor_idx);
	} else {
		loge("isp module stop_preview is NULL");
		ret = -1;
	}
	return ret;
}


static int isp_module_start_recording(int sensor_idx)
{
	int ret = 0;

	logd("enter %s", __func__);

	if (isp_hdl->intf->start_video) {
		ret =
			isp_hdl->intf->start_video(isp_hdl->intf->context,
						 sensor_idx);
		if (ret != IMF_RET_SUCCESS)
			loge("isp module start record fail, ret=%d", ret);
		else
			logi("isp module start sensor[%d] record success",
				sensor_idx);
	} else {
		loge("isp module start_record is NULL");
		ret = -1;
	}
	return ret;
}

static int isp_module_stop_recording(int sensor_idx, bool pause)
{
	int ret = 0;

	logd("enter %s", __func__);
	if (isp_hdl->intf->stop_video) {
		ret =
		    isp_hdl->intf->stop_video(isp_hdl->intf->context,
						sensor_idx, pause);
		if (ret != IMF_RET_SUCCESS)
			logd("isp module stop record fail, ret=%d", ret);
		else
			logi("isp module stop sensor[%d] record scuuess",
				sensor_idx);
	} else {
		loge("isp module stop_record is NULL");
		ret = -1;
	}
	return ret;
}

static int isp_module_start_still(int sensor_idx)
{
	int ret = 0;

	logd("enter %s", __func__);
	if (isp_hdl->intf->start_zsl) {
		ret =
		    isp_hdl->intf->start_zsl(isp_hdl->intf->context,
						 sensor_idx, 0);
		if (ret != IMF_RET_SUCCESS)
			loge("%s fail, ret=%d", __func__, ret);
		else
			loge("%s start sensor[%d] still success",
				__func__, sensor_idx);
	} else {
		loge("%s start_zsl is NULL", __func__);
		ret = -1;
	}
	return ret;
}

static int isp_module_stop_still(int sensor_idx, bool pause)
{
	int ret = 0;

	logd("enter %s", __func__);
	if (isp_hdl->intf->stop_zsl) {
		ret = isp_hdl->intf->stop_zsl(isp_hdl->intf->context,
						sensor_idx, pause);
		if (ret != IMF_RET_SUCCESS)
			logd("isp module stop still fail, ret=%d", ret);
		else
			loge("isp module stop sensor[%d] still scuuess",
				sensor_idx);
	} else {
		loge("isp module stop_still is NULL");
		ret = -1;
	}
	return ret;
}

static int isp_module_start_metadata(int sensor_idx)
{
	logd("enter %s", __func__);
//to do later
	return 0;
}

static int isp_module_stop_metadata(int sensor_idx)
{
	int ret = 0;

	logd("enter %s", __func__);
//to do later
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

static int isp_ioctl_stream_open(struct cam_stream *s, void *priv)
{
	struct isp_info *isp = v4l2_get_subdevdata(s->isp_sd);
	int ret = 0;

	logd("enter %s[%d]", __func__, isp->state);

	if (isp->state == ISP_AVAILABLE) {

		/* record active stream */
		mutex_lock(&isp->mutex);
		list_add_tail(&s->stream_list, &isp->active_streams);
		isp->state = ISP_READY;
		mutex_unlock(&isp->mutex);
	} else if (isp->state == ISP_READY) {
		/* initialized but no active stream */
		mutex_lock(&isp->mutex);
		list_add_tail(&s->stream_list, &isp->active_streams);
		mutex_unlock(&isp->mutex);
	} else if (isp->state == ISP_RUNNING) {
		/** FIXME:
		 * 1. do we need stop streams already existed?
		 *TODO : add more detail here
		 */
		mutex_lock(&isp->mutex);
		list_add_tail(&s->stream_list, &isp->active_streams);
		mutex_unlock(&isp->mutex);
	} else {
		/* error process */
		goto err_open;
	}
	return 0;

err_open:
	mutex_lock(&isp->mutex);
	isp->state = ISP_ERROR;
	mutex_unlock(&isp->mutex);
	return ret;
}

static int isp_ioctl_stream_close(struct physical_cam *cam, void *priv)
{
	struct isp_info *isp = v4l2_get_subdevdata(cam->isp_sd);
	int ret = 0;

	logd("enter %s", __func__);

	if (isp->state == ISP_READY)
		isp_module_close_sensor(cam->sensor_id);
	else
		logw("sensor[%d] not open yet!\n", cam->sensor_id);

	return ret;
}

static int isp_ioctl_s_fmt(struct cam_stream *s)
{
	int ret = 0;

	logd("enter %s(%d)", __func__, s->idx);

	/** FIXME:
	 *set format and resolution to ISP module
	 */
	if (s->state == ISP_AVAILABLE || s->state == ISP_READY) {
		if (s->type == STREAM_PREVIEW ||
		s->type == STREAM_VIDEO ||
		s->type == STREAM_ZSL) {
			ret = isp_module_set_stream_para(s);
			if (ret)
				loge("isp module set stream param failed!");
			else
				logd("isp module set stream param success!");
		}
	} else if (s->state == ISP_RUNNING) {
		loge("s_fmt running");
		/* TODO: add implementation here */
	} else {
		loge("s_fmt error");
		/* ERROR state process */
	}

	return ret;
}

static int isp_ioctl_stream_on(struct cam_stream *s, void *priv)
{
	struct isp_info *isp = v4l2_get_subdevdata(s->isp_sd);
	int ret = 0;

	logd("enter %s, stream type %d", __func__, s->type);


	mutex_lock(&isp->mutex);
	if (isp->state == ISP_AVAILABLE || isp->state == ISP_READY) {
		/* first time to initialize isp hardware */
		if (isp_module_load_firmware(s->cur_cfg.sensor_id)) {
			loge("failed to load firmware!");
			mutex_unlock(&isp->mutex);
			return -1;
		}
		if (isp_module_open_sensor(s)) {
			loge("failed to open sensor!");
			mutex_unlock(&isp->mutex);
			return -1;
		}
		isp->state = ISP_RUNNING;
		/* isp->running_cnt++; */
	} else if (isp->state == ISP_RUNNING) {
		/* TODO: add implementation here */
		logw("hardware is already stream on!");
	} else {
		/* ERROR state process */
		loge("isp is in error state [%d]!!", isp->state);
	}
	mutex_unlock(&isp->mutex);

	switch (s->type) {
	case STREAM_PREVIEW:
		logd("register new preview stream");
		if (isp->s_preview == NULL) {
			isp->s_preview = s;
			//isp_module_set_stream_para(s);
			ret = isp_module_start_preview(s->cur_cfg.sensor_id);
			//ret = isp_module_start_still(s->cur_cfg.sensor_id);
			isp->running_cnt++;
		}
		break;
	case STREAM_VIDEO:
		if (isp->s_video == NULL) {
			logd("register new video stream");
			isp->s_video = s;
			ret = isp_module_start_recording(s->cur_cfg.sensor_id);
			isp->running_cnt++;
		}
		break;
	case STREAM_JPEG:
		if (isp->s_jpeg == NULL) {
			logd("register new JPEG stream");
			isp->s_jpeg = s;
			isp->running_cnt++;
		}

		if (isp->zsl_mode == ZSL_MODE)
			vb2_streamon(&s->inq, s->inq.type);

		break;
	case STREAM_ZSL:
		if (isp->s_zsl == NULL) {
			logd("register new ZSL stream");
			ret = isp_module_start_still(s->cur_cfg.sensor_id);
			isp->s_zsl = s;
			isp->running_cnt++;
		}
		break;
	case STREAM_METADATA:
		if (isp->s_metadata == NULL) {
			logd("register new metadata stream");
			ret = isp_module_start_metadata(s->cur_cfg.sensor_id);
			isp->s_metadata = s;
			isp->running_cnt++;
		}
		break;
	default:
		loge("invalid stream type");
		return -1;
	}

	return ret;
}

static int isp_ioctl_stream_off(struct cam_stream *s, bool *priv)
{
	struct isp_info *isp = v4l2_get_subdevdata(s->isp_sd);

	logd("enter %s: sid(%d)", __func__, s->type);

	if (isp->state == ISP_RUNNING) {
		switch (s->type) {
		case STREAM_PREVIEW:
			isp_module_stop_preview(s->cur_cfg.sensor_id, *priv);
			isp->s_preview = NULL;
			break;
		case STREAM_VIDEO:
			isp_module_stop_recording(s->cur_cfg.sensor_id, *priv);
			isp->s_video = NULL;
			break;
		case STREAM_JPEG:
			isp->s_jpeg = NULL;
			if (isp->zsl_mode == ZSL_MODE)
				vb2_streamoff(&s->inq, s->inq.type);
			break;
		case STREAM_ZSL:
			isp_module_stop_still(s->cur_cfg.sensor_id, *priv);
			isp->s_zsl = NULL;
			break;
		case STREAM_METADATA:
			isp_module_stop_metadata(s->cur_cfg.sensor_id);
			isp->s_metadata = NULL;
			break;
		}

		mutex_lock(&isp->mutex);
		isp->running_cnt--;
		if (isp->running_cnt == 0)
			isp->state = ISP_READY;
		mutex_unlock(&isp->mutex);
	}
	return 0;
}

static struct sys_img_buf_handle g_main_jpeg_buf;
static struct sys_img_buf_handle g_thumbnail_buf;

struct jpeg_header {
	uint32_t jpeg_buffer_max_size;
	uint32_t jfif_header_size;
	uint32_t jpeg_offset;
	uint32_t jpeg_payload;
	uint32_t thumb_offset;
	uint32_t thumb_payload;
};

static int isp_ioctl_queue_buffer(struct cam_stream *s, struct stream_buf *b)
{
	struct isp_info *isp = v4l2_get_subdevdata(s->isp_sd);
	sys_img_buf_handle_t main_jpeg_buf = NULL;
	sys_img_buf_handle_t thumbnail_buf = NULL;
	struct pvt_img_size img_size;
	struct jpeg_header *jpeg_head;
	int ret = 0;

	logd("enter %s, stream=[%d]", __func__, s->idx);
	if (s->type == STREAM_JPEG) {
		/*
		 * RAW capture for tuning support
		 */
		if (s->cur_cfg.fmt.type == F_RAW) {
			logd("take raw picture!");
			ret = isp->intf->take_raw_pic(isp->intf->context,
					s->cur_cfg.sensor_id,
					&b->handle, 0);
			if (ret)
				loge("isp module take one picture failed!");
			else
				return ret;
		}
		if (!isp->intf->take_one_pic) {
			loge("isp module take one picture is NULL");
			return -1;
		}

		img_size.width = s->cur_cfg.width;
		img_size.height = s->cur_cfg.height;
		if (isp->intf->set_zsl_para) {
			struct pvt_img_res_fps_pitch para;

			ret = isp->intf->set_zsl_para(isp->intf->context,
						s->cur_cfg.sensor_id,
						PARA_ID_IMAGE_SIZE,
						&img_size);
			/*
			 * For zsl capture only, which using YUV422I
			 */
			para.width = img_size.width;
			para.height = img_size.height;
			para.luma_pitch = img_size.width; //img_size.width * 2;
			para.fps = 30; // add for test
			para.chroma_pitch = img_size.width; //0;
			ret =
				isp->intf->set_zsl_para(isp->intf->context,
						s->cur_cfg.sensor_id,
						PARA_ID_DATA_RES_FPS_PITCH,
						&para);
		}
		if (ret != IMF_RET_SUCCESS) {
			loge("failed to set jpeg size");
			return ret;
		}

		img_size.width = 320;
		img_size.height = 240;
		if (isp->intf->set_zsl_para) {
			ret =
			isp->intf->set_zsl_para(isp->intf->context,
						s->cur_cfg.sensor_id,
						PARA_ID_JPEG_THUMBNAIL_SIZE,
						&img_size);
		}
		if (ret != IMF_RET_SUCCESS) {
			loge("failed to set jpeg thumbnail size");
			return ret;
		}

		main_jpeg_buf = &g_main_jpeg_buf;
		thumbnail_buf = &g_thumbnail_buf;

		/*
		 * FIXME: if we use dmabuf, virtual address is not
		 * available for v4l2 layer, isp module need to do some
		 * workaround to avoid this.
		 */
		jpeg_head = (struct jpeg_header *)b->handle.virtual_addr;

		main_jpeg_buf->len =
			jpeg_head->thumb_offset - jpeg_head->jpeg_offset;
		main_jpeg_buf->physical_addr = 0;
		main_jpeg_buf->physical_addr = 0;
		main_jpeg_buf->private_buffer_info = 0;
		main_jpeg_buf->virtual_addr =
			(u8 *)b->handle.virtual_addr + jpeg_head->jpeg_offset;

		thumbnail_buf->len = b->handle.len - jpeg_head->thumb_offset;
		thumbnail_buf->physical_addr = 0;
		thumbnail_buf->private_buffer_info = 0;
		thumbnail_buf->virtual_addr = (u8 *)b->handle.virtual_addr +
			jpeg_head->thumb_offset;
		logd("main jpeg: len: %u, vaddr:%p",
			main_jpeg_buf->len, main_jpeg_buf->virtual_addr);
		logd("thumbnail: len: %u, vaddr:%p", thumbnail_buf->len,
			thumbnail_buf->virtual_addr);
		logd("jpeg_offset:%u, thumb_offset:%u\n",
			jpeg_head->jpeg_offset,
			jpeg_head->thumb_offset);

		if (isp->zsl_mode == NON_ZSL_MODE) {
			struct take_one_pic_para imagePara;

			memset(&imagePara, 0, sizeof(struct take_one_pic_para));
			imagePara.fmt = PVT_IMG_FMT_NV12;
			imagePara.width = s->cur_cfg.width;
			imagePara.height = s->cur_cfg.height;
			imagePara.luma_pitch = imagePara.width * 2;
			imagePara.chroma_pitch = 0;
			imagePara.use_precapture = 0;
			imagePara.focus_first = 1;//pDevice->FocusPriority;
			logd("take jpeg picture");
			msleep(500);
			ret = isp->intf->take_one_pic(isp->intf->context,
					s->cur_cfg.sensor_id,
					&imagePara, &b->handle);
			if (ret != IMF_RET_SUCCESS)
				loge("failed to take picture!");

		} else {
			/*
			 * in zsl capture mode, check input buffer first
			 */
			logd("take zsl jpeg picture");
			msleep(200);
			if (s->input_buf && isp->intf->take_one_pp_jpeg) {
				ret = isp->intf->take_one_pp_jpeg(
					isp->intf->context,
					s->cur_cfg.sensor_id,
					&(s->input_buf->handle),
					main_jpeg_buf,
					thumbnail_buf,
					NULL);
				if (ret != IMF_RET_SUCCESS)
					loge("failed to take pp jpeg");
			} else {
				ret = -1;
				loge("failed to find input buffer");
			}
		}
	} else if (s->type == STREAM_PREVIEW) {
		if (isp->intf->set_preview_buf) {
			logd("set preview buff 0x%p(%d)",
				b->handle.virtual_addr, b->handle.len);
			/*
			 * msleep(100);
			 * memset(b->handle.virtual_addr,0,b->handle.len);
			 */
			ret = isp->intf->set_preview_buf(isp->intf->context,
							 s->cur_cfg.sensor_id,
							 &b->handle);
			if (ret != IMF_RET_SUCCESS)
				loge("failed to set preview buffer");
		}
	} else if (s->type == STREAM_VIDEO) {
		if (isp->intf->set_video_buf) {
			logd("set recording buffer");
			ret = isp->intf->set_video_buf(isp->intf->context,
							s->cur_cfg.sensor_id,
							&b->handle);
			if (ret != IMF_RET_SUCCESS)
				loge("failed to set recording buffer");
		}
	} else if (s->type == STREAM_ZSL) {
		if (isp->intf->set_zsl_buf) {
			logd("set zsl buffer\n");
			ret = isp->intf->set_zsl_buf(isp->intf->context,
							s->cur_cfg.sensor_id,
							&b->handle);
			if (ret != IMF_RET_SUCCESS)
				loge("failed to set zsl buffer");
		}
	} else if (s->type == STREAM_METADATA) {
		if (isp->intf->set_metadata_buf) {
			logd("set metadata buff 0x%p(%d)",
				b->handle.virtual_addr, b->handle.len);
			ret =
				isp->intf->set_metadata_buf(isp->intf->context,
							s->cur_cfg.sensor_id,
							&b->handle);
			if (ret != IMF_RET_SUCCESS)
				loge("failed to metadata buffer");
		}
	}
	return ret;
}

static long  isp_ioctl(struct v4l2_subdev *sd, unsigned int cmd, void *arg)
{
	struct isp_ctl_ctx *ctx = (struct isp_ctl_ctx *)arg;

	logd("enter %s", __func__);
	switch (cmd) {
	case ISP_IOCTL_STREAM_OPEN:
		return isp_ioctl_stream_open(ctx->stream, (void *)ctx->ctx);
	case ISP_IOCTL_S_FMT:
		return isp_ioctl_s_fmt(ctx->stream);
	//case ISP_IOCTL_S_INPUT:
		//return isp_ioctl_s_input
		//		(ctx->stream, (unsigned int *)ctx->ctx);
	case ISP_IOCTL_STREAM_ON:
		return isp_ioctl_stream_on(ctx->stream, (void *)ctx->ctx);
	case ISP_IOCTL_STREAM_OFF:
		return isp_ioctl_stream_off(ctx->stream, (bool *)ctx->ctx);
	case ISP_IOCTL_QUEUE_BUFFER:
		return isp_ioctl_queue_buffer(ctx->stream,
					(struct stream_buf *)ctx->ctx);
	case ISP_IOCTL_STREAM_CLOSE:
		return isp_ioctl_stream_close(ctx->cam, (void *)ctx->ctx);
	default:
		break;
	}
	loge("unknown cmd [%d]", cmd);
	return -EINVAL;
}

static unsigned long get_buf_size(u32 w, u32 h, enum pvt_img_fmt format)
{
	unsigned long size;

	switch (format) {
	case PVT_IMG_FMT_YV12:
	case PVT_IMG_FMT_I420:
	case PVT_IMG_FMT_NV21:
	case PVT_IMG_FMT_NV12:
		size = w * h * 3 / 2;
		break;
	case PVT_IMG_FMT_YUV422P:
	case PVT_IMG_FMT_YUV422_SEMIPLANAR:
	case PVT_IMG_FMT_YUV422_INTERLEAVED:
		size = w * h * 2;
		break;
/*
 *	case V4L2_PIX_FMT_JPEG:
 *		size = w * h;
 *		break;
 */
	default:
		size = 0;
		break;
	}

	return size;

}

/* JPEG function */
static void finish_picture_buf(void *param)
{
//to do later

}

static void finish_raw_pic(struct take_raw_pic_cb_para *param)
{

	struct cam_stream *s = isp_hdl->s_jpeg;
	struct stream_buf *buf;
	unsigned long flags = 0;
	unsigned long size = 0;
	void *vbuf = NULL;
	int plane_num = 0;

	logd("enter %s", __func__);

	spin_lock_irqsave(&s->b_lock, flags);
	if (list_empty(&s->active_buf)) {
		loge("%s: no active buffer in stream[%d]", __func__, s->idx);
		spin_unlock_irqrestore(&s->b_lock, flags);
		return;
	}
	logd("raw list not empty");

	buf = list_entry(s->active_buf.next, struct stream_buf, list);

	logd("get a buf in raw list[0x%p]", buf->handle.virtual_addr);
	list_del(&buf->list);
	spin_unlock_irqrestore(&s->b_lock, flags);

	plane_num = buf->vb.num_planes;
	if (plane_num > 0) {
		vbuf = vb2_plane_vaddr(&buf->vb, 0);
		if (vbuf) {
			size = param->raw_data_buf->len;
			vb2_set_plane_payload(&buf->vb, 0, size);
		}
	}
	//buf->vb.v4l2_buf.field = V4L2_FIELD_NONE;
	/* stream_info->field_count++; */
	//buf->vb.v4l2_buf.sequence = 0;
	if (buf->kparam)
		buf->m = &((struct frame_done_cb_para *)param)->metaInfo;
	vb2_buffer_done(&buf->vb, VB2_BUF_STATE_DONE);
	//logd("%s, [%p/%d/%u] done", __func__, buf, buf->vb.v4l2_buf.index,
		//(unsigned int)size);
}

/**
 * FIXME: this function implementation should be moved to amd_stream.c
 * make it a callback function of amd_stream
 */
static void finish_preview_buf(void *param)
{
	struct cam_stream *s = isp_hdl->s_preview;

	/* struct vb2_queue *vidq = &isp_hdl->s_preview->vidq; */
	/* struct stream_buf *dump_buf; */

	struct stream_buf *buf;

	unsigned long flags = 0;
	unsigned long size;
	void *vbuf;
	int plane_num = 0;

	logd("%s list head=[%p], next=[%p]", __func__, &s->active_buf,
		(&s->active_buf)->next);
	/* vb2_buffer_done(&buf->vb, VB2_BUF_STATE_DONE); return; */

	spin_lock_irqsave(&s->b_lock, flags);
	if (list_empty(&s->active_buf)) {
		loge("%s: no active buffer in stream[%d]", __func__, s->idx);
		spin_unlock_irqrestore(&s->b_lock, flags);
		return;
	}

	loge("preview list not empty");
	buf = list_entry(s->active_buf.next, struct stream_buf, list);

	/* vb2_buffer_done(&buf->vb, VB2_BUF_STATE_DONE); */
	logd("get a buf in preview list[0x%p]", buf->handle.virtual_addr);
	list_del(&buf->list);
	spin_unlock_irqrestore(&s->b_lock, flags);

	/* logd("enter %s %d", __func__, __LINE__); */
	//do_gettimeofday(&buf->vb.timestamp);

	/* fill buffer */
	plane_num = buf->vb.num_planes;
	logd("enter %s plane_num %d %d", __func__, plane_num, __LINE__);
	if ((plane_num > 0)) {
		vbuf = vb2_plane_vaddr(&buf->vb, 0);
		if (vbuf) {
			size = get_buf_size(s->cur_cfg.width,
				s->cur_cfg.height,
				s->cur_cfg.fmt.format);
			vb2_set_plane_payload(&buf->vb, 0, size);
		}
	}


	//buf->vb.v4l2_buf.field = V4L2_FIELD_NONE;
	/* stream_info->field_count++; */
	//buf->vb.v4l2_buf.sequence = 0;

	//buf->vb.state = VB2_BUF_STATE_ACTIVE;

	if (buf->kparam)
		buf->m = &((struct frame_done_cb_para *)param)->metaInfo;
	vb2_buffer_done(&buf->vb, VB2_BUF_STATE_DONE);
	//logd("%s, [%p/%d/%u] done", __func__, buf, buf->vb.v4l2_buf.index,
		//(unsigned int)size);

}


static void finish_recording_buf(void *param)
{
	struct cam_stream *s = isp_hdl->s_video;

	/* struct vb2_queue *vidq = &isp_hdl->s_preview->vidq; */
	/* struct stream_buf *dump_buf; */

	struct stream_buf *buf;

	unsigned long flags = 0;
	unsigned long size;
	void *vbuf;
	int plane_num = 0;

	logd("%s list head=[%p], next=[%p]", __func__, &s->active_buf,
		(&s->active_buf)->next);
	/* vb2_buffer_done(&buf->vb, VB2_BUF_STATE_DONE); return; */

	spin_lock_irqsave(&s->b_lock, flags);
	if (list_empty(&s->active_buf)) {
		loge("%s: no active buffer in stream[%d]", __func__, s->idx);
		spin_unlock_irqrestore(&s->b_lock, flags);
		return;
	}

	logd("video list not empty");
	buf = list_entry(s->active_buf.next, struct stream_buf, list);

	/* vb2_buffer_done(&buf->vb, VB2_BUF_STATE_DONE); */
	logd("get a buf in video list[0x%p]", buf->handle.virtual_addr);
	list_del(&buf->list);
	spin_unlock_irqrestore(&s->b_lock, flags);

	/* logd("enter %s %d", __func__, __LINE__); */
	//do_gettimeofday(&buf->vb.timestamp);

	/* fill buffer */
	plane_num = buf->vb.num_planes;
	logd("enter %s plane_num %d %d", __func__, plane_num, __LINE__);
	if ((plane_num > 0)) {
		vbuf = vb2_plane_vaddr(&buf->vb, 0);
		if (vbuf) {
			size = get_buf_size(s->cur_cfg.width,
				s->cur_cfg.height,
				s->cur_cfg.fmt.format);
			vb2_set_plane_payload(&buf->vb, 0, size);
		}
	}


	//buf->vb.v4l2_buf.field = V4L2_FIELD_NONE;
	/* stream_info->field_count++; */
	//buf->vb.v4l2_buf.sequence = 0;

	//buf->vb.state = VB2_BUF_STATE_ACTIVE;

	if (buf->kparam)
		buf->m = &((struct frame_done_cb_para *)param)->metaInfo;
	vb2_buffer_done(&buf->vb, VB2_BUF_STATE_DONE);
	//logd("%s, [%p/%d/%u] done", __func__, buf, buf->vb.v4l2_buf.index,
		//(unsigned int)size);

}

static void finish_zsl_buf(void *param)
{
	struct cam_stream *s = isp_hdl->s_zsl;
	struct stream_buf *buf;
	unsigned long flags = 0;
	unsigned long size;
	int plane_num = 0;

	logd("enter %s", __func__);

	spin_lock_irqsave(&s->b_lock, flags);
	if (list_empty(&s->active_buf)) {
		loge("%s: no active buffer in stream[%d]", __func__, s->idx);
		spin_unlock_irqrestore(&s->b_lock, flags);
		return;
	}

	buf = list_entry(s->active_buf.next, struct stream_buf, list);
	logd("get a buf in zsl list[0x%p], len %u", buf->handle.virtual_addr,
		s->cur_cfg.width * s->cur_cfg.height * 2);
	list_del(&buf->list);
	spin_unlock_irqrestore(&s->b_lock, flags);
	//do_gettimeofday(&buf->vb.timestamp);

	/* fill buffer */
	plane_num = buf->vb.num_planes;
	if ((plane_num > 0)) {
		if (vb2_plane_vaddr(&buf->vb, 0)) {
			size = get_buf_size(s->cur_cfg.width,
				s->cur_cfg.height,
				s->cur_cfg.fmt.format);
			vb2_set_plane_payload(&buf->vb, 0, size);
		}
	}
	//buf->vb.v4l2_buf.field = V4L2_FIELD_NONE;
	/* stream_info->field_count++; */
	//buf->vb.v4l2_buf.sequence = 0;

	if (buf->kparam)
		buf->m = &((struct frame_done_cb_para *)param)->metaInfo;
	vb2_buffer_done(&buf->vb, VB2_BUF_STATE_DONE);
	//logd("%s,[%p/%d/%u]done", __func__, buf, buf->vb.v4l2_buf.index,
		//(unsigned int)size);
}


int32_t func_lib_callback(void *context, enum cb_evt_id event, void *param)
{

	logd("enter %s, event=[%d]", __func__, event);

	switch (event) {
	case CB_EVT_ID_PREV_BUF_DONE:
		finish_preview_buf(param);
		break;
	case CB_EVT_ID_STILL_BUF_DONE:
		finish_picture_buf(param);
		break;
	case CB_EVT_ID_REC_BUF_DONE:
		finish_recording_buf(param);
		break;
	case CB_EVT_ID_TAKE_ONE_PIC_DONE:
		finish_picture_buf(param);
		break;
	case CB_EVT_ID_TAKE_RAW_PIC_DONE:
		finish_raw_pic(param);
		break;
	case CB_EVT_ID_METADATA_DONE:
		//finish_metadata_buf(param);
		break;
	case CB_EVT_ID_ZSL_BUF_DONE:
		finish_zsl_buf(param);
		break;
	default:
		loge("unknown event [%d]!", event);
		return -1;
	}

	return 0;
}

extern struct isp_isp_module_if *get_isp_module_interface(void);

static int on_isp_registered(struct v4l2_subdev *sd)
{
	enum camera_id cid;
	int ret = 0;

	logd("enter %s", __func__);

	isp_hdl = kzalloc(sizeof(*isp_hdl), GFP_KERNEL);
	if (!isp_hdl)
		return -ENOMEM;

	/* FIXME: add isp information init here */
	isp_hdl->type = INTEGRATED;
	isp_hdl->state = ISP_AVAILABLE;
	isp_hdl->zsl_mode = NON_ZSL_MODE;
	INIT_LIST_HEAD(&isp_hdl->active_streams);
	mutex_init(&isp_hdl->mutex);
	isp_hdl->running_cnt = 0;

	isp_hdl->intf = get_isp_module_interface();
	if (!isp_hdl->intf) {
		loge("%s failed to get isp module interface", __func__);
		ret = -ENODEV;
		goto err_clean;
	}

	init_camera_devices(isp_hdl->intf, 0);

//	cid = CAM_IDX_FRONT_L;//CAM_IDX_BACK;
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
	v4l2_set_subdevdata(sd, isp_hdl);

	return 0;
err_clean:
	kfree(isp_hdl);
	return -1;
}

void test_camera_release(void)
{
	struct amdisp_sensor_init init;

	init.sensor_info.sensor_id = 0;
	get_sensor_interface()->ioctl(SENSOR_IOCTL_RELEASE, &init);
/*comment out since not neccesary for mero.*/
//	get_vcm_interface()->ioctl(VCM_IOCTL_RELEASE, &init);
//	get_flash_interface()->ioctl(FLASH_IOCTL_RELEASE, &init);
//	get_storage_interface()->ioctl(STORAGE_IOCTL_INIT, &init);
}

static void on_isp_unregistered(struct v4l2_subdev *sd)
{
	struct isp_info *isp = v4l2_get_subdevdata(sd);

	logd("enter %s", __func__);

	/* FIXME: do deinitialization accroding to state */

	test_camera_release();

	/* uninit_at_module_exit(); */
	if (isp->intf->unreg_snr_op)
		isp->intf->unreg_snr_op(isp->intf->context, 1);
	if (isp->intf->unreg_vcm_op)
		isp->intf->unreg_vcm_op(isp->intf->context, 1);
	if (isp->intf->unreg_flash_op)
		isp->intf->unreg_flash_op(isp->intf->context, 1);
	if (isp->intf->unreg_notify_cb)
		isp->intf->unreg_notify_cb(isp->intf->context, 1);

	/* FIXME: do some hw uninitializtion work */
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

	logd("enter %s", __func__);

	v4l2_subdev_init(sd, &isp_ops);
	sd->owner = THIS_MODULE;
	snprintf(sd->name, sizeof(sd->name), "%s", "AMD-ISP");
	sd->internal_ops = &isp_internal_ops;

	ret = v4l2_device_register_subdev(v4l2_dev, sd);
	if (!ret)
		logi("register subdev [%s]", sd->name);
	else
		loge("failed register subdev [%s]", sd->name);
	return ret;
}

void unregister_isp_subdev(struct v4l2_subdev *sd)
{
	logd("enter %s", __func__);

	v4l2_device_unregister_subdev(sd);
}
