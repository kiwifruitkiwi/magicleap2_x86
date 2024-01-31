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
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include "amd_common.h"
#include "amd_params.h"

#define LOG_TAG "AMD_PARAM"
#define LOG_DBG 1
#include "amd_log.h"

#include "isp_module_if.h"

static struct amd_format amd_formats[] = {
	{
	 .name = "YUV 4:2:2 planar, Y/Cb/Cr",
	 .format = PVT_IMG_FMT_YUV422P,
	 .depth = 16,
	 .type = F_YUV,
	 },
	{
	 .name = "YUV 4:2:0 semi-planar, Y/CbCr",
	 .format = PVT_IMG_FMT_NV12,
	 .depth = 12,
	 .type = F_YUV,
	 },
	{
	 .name = "YUV 4:2:0 semi-planar, Y/CrCb",
	 .format = PVT_IMG_FMT_NV21,
	 .depth = 12,
	 .type = F_YUV,
	 },
	{
	 .name = "YUV 4:2:0 planar(YU12), Y/Cb/Cr",
	 .format = PVT_IMG_FMT_I420,
	 .depth = 12,
	 .type = F_YUV,
	 },
	{
	 .name = "YUV 4:2:0 planar(YV12), Y/Cr/Cb",
	 .format = PVT_IMG_FMT_YV12,
	 .depth = 12,
	 .type = F_YUV,
	 },
	{
	 .name = "YUV 4:2:2 interleaved",
	 .format = PVT_IMG_FMT_YUV422_INTERLEAVED,
	 .depth = 16,
	 .type = F_YUV,
	 },
	{
	 .name = "YUV 4:2:2 semiplanar",
	 .format = PVT_IMG_FMT_YUV422_SEMIPLANAR,
	 .depth = 16,
	 .type = F_YUV,
	 },
/*
 *	{
 *	.name = "JPEG encoded data",
 *	.format = PVT_IMG_FMT_L8,
 *	.depth = 8,
 *	.type = F_JPEG,
 *	},
 *	{
 *	.name = "RAW data",
 *	.format = V4L2_PIX_FMT_SRGGB10,
 *	.depth = 16,
 *	.type = F_RAW,
 *	}
 */
};

struct amd_format *select_default_format()
{
	return &amd_formats[1];
}

struct amd_format *amd_find_format(int index)
{
	logd("enter %s", __func__);

	if (index >= (int)ARRAY_SIZE(amd_formats))
		return NULL;

	return &amd_formats[index];
}

struct amd_format *amd_find_match(enum pvt_img_fmt fmt)
{
	int i = 0;

	logd("enter %s", __func__);
	for (; i < ARRAY_SIZE(amd_formats); i++) {
		if (amd_formats[i].format == fmt)
			return &amd_formats[i];
	}

	loge("failed to find matched format!!");
	return NULL;
}

int translate_ctrl_id(int ctrl_id)
{
	int id;

	switch (ctrl_id) {
	case V4L2_CID_BRIGHTNESS:
		logd("brightness");
		id = PARA_ID_BRIGHTNESS;
		break;
	case V4L2_CID_CONTRAST:
		logd("contrast");
		id = PARA_ID_CONTRAST;
		break;
	case V4L2_CID_SATURATION:
		logd("saturation");
		id = PARA_ID_SATURATION;
		break;
	case V4L2_CID_HUE:
		logd("hue");
		id = PARA_ID_HUE;
		break;
	case V4L2_CID_HFLIP:
		logd("DO NOT SUPPORT hflip");
		id = -1;
		break;
	case V4L2_CID_VFLIP:
		logd("DO NOT SUPPORT vflip");
		id = -1;
		break;
	case V4L2_CID_POWER_LINE_FREQUENCY:
		logd("banding");
		id = PARA_ID_ANTI_BANDING;
		break;
	case V4L2_CID_AUTO_WHITE_BALANCE:
		logd("TODO: CID_AUTO_WHITE_BALANCE");
		id = -1;
		break;
		/*
		 * case V4L2_CID_AUTO_DO_WHITE_BALANCE:
		 *      logd("get do awb: should not be called!");
		 *      break;
		 */
	case V4L2_CID_WHITE_BALANCE_TEMPERATURE:
		logd("TODO: CID wb temperature");
		id = -1;
		break;
	case V4L2_CID_SHARPNESS:
		logd("TODO: sharpness");
		id = -1;
		break;
	case V4L2_CID_COLORFX:
		logd("TODO: color effects");
		id = -1;
		break;
	case V4L2_CID_COLORFX_CBCR:
		logd("TODO: color effects_cbcr");
		id = -1;
		break;
	case V4L2_CID_GAMMA:
		logd("TODO:garmma");
		id = -1;
		break;
	case V4L2_CID_EXPOSURE:
		logd("TODO: exposure");
		id = -1;
		break;
	case V4L2_CID_GAIN:
		logd("TODO: gain");
		id = -1;
		break;
	case V4L2_CID_AUTOGAIN:
		logd("TODO: autogain");
		id = -1;
		break;
	default:
		loge("unknown commands");
		id = -1;
		break;
	}
	return id;

}

void translate_ctrl_value(struct v4l2_ext_control *ctl, void *param)
{
	switch (ctl->id) {
	case V4L2_CID_BRIGHTNESS:
	case V4L2_CID_CONTRAST:
	case V4L2_CID_SATURATION:
	case V4L2_CID_HUE:
		*((int *)param) = ctl->value;
		break;
	case V4L2_CID_POWER_LINE_FREQUENCY:
		if (ctl->value == V4L2_CID_POWER_LINE_FREQUENCY_DISABLED) {
			*((enum anti_banding_mode *)param) =
				ANTI_BANDING_MODE_DISABLE;
		} else if (ctl->value == V4L2_CID_POWER_LINE_FREQUENCY_50HZ) {
			*((enum anti_banding_mode *)param) =
				ANTI_BANDING_MODE_50HZ;
		} else if (ctl->value == V4L2_CID_POWER_LINE_FREQUENCY_60HZ) {
			*((enum anti_banding_mode *)param) =
				ANTI_BANDING_MODE_60HZ;
		} else {
			loge("can't convert to isp known parameter!");
		}
		break;
	}
}

void translate_param_value(struct v4l2_ext_control *ctl, void *param)
{
	switch (ctl->id) {
	case V4L2_CID_BRIGHTNESS:
	case V4L2_CID_CONTRAST:
	case V4L2_CID_SATURATION:
	case V4L2_CID_HUE:
		ctl->value = *((int *)param);
		break;
	case V4L2_CID_POWER_LINE_FREQUENCY:
		if (*((enum anti_banding_mode *)param) ==
			ANTI_BANDING_MODE_DISABLE) {
			ctl->value = V4L2_CID_POWER_LINE_FREQUENCY_DISABLED;
		} else if (*((enum anti_banding_mode *)param) ==
			ANTI_BANDING_MODE_50HZ) {
			ctl->value = V4L2_CID_POWER_LINE_FREQUENCY_50HZ;
		} else if (*((enum anti_banding_mode *)param) ==
			ANTI_BANDING_MODE_60HZ) {
			ctl->value = V4L2_CID_POWER_LINE_FREQUENCY_60HZ;
		} else {
			loge("can't convert to isp known parameter!");
		}
		break;
	}
}
