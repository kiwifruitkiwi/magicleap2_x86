/*
 * Copyright (C) 2019-2020 Advanced Micro Devices, Inc. All rights reserved.
 */

#include "amd_common.h"
#include "amd_params.h"
#include "isp_module_if.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "[ISP][amd_params]"

static struct amd_format amd_formats[] = {
	{
	 .name = "YUV 4:2:0 semi-planar, Y/CbCr, NV12",
	 .format = PVT_IMG_FMT_NV12,
	 .fw_fmt = IMAGE_FORMAT_NV12,
	 .depth = 12,
	 .arrays = 2,
	 .type = F_YUV,
	 },
	{
	 .name = "YUV 4:2:0 semi-planar, Y/CrCb, NV21",
	 .format = PVT_IMG_FMT_NV21,
	 .fw_fmt = IMAGE_FORMAT_NV21,
	 .depth = 12,
	 .arrays = 2,
	 .type = F_YUV,
	 },
	{
	 .name = "YUV 4:2:0 planar(YU12), Y/Cb/Cr, I420",
	 .format = PVT_IMG_FMT_I420,
	 .fw_fmt = IMAGE_FORMAT_I420,
	 .depth = 12,
	 .arrays = 3,
	 .type = F_YUV,
	 },
	{
	 .name = "YUV 4:2:0 planar(YV12), Y/Cr/Cb, YV12",
	 .format = PVT_IMG_FMT_YV12,
	 .fw_fmt = IMAGE_FORMAT_YV12,
	 .depth = 12,
	 .arrays = 3,
	 .type = F_YUV,
	 },
	{
	 .name = "YUV 4:2:2 planar, Y/Cb/Cr, YUV422P",
	 .format = PVT_IMG_FMT_YUV422P,
	 .fw_fmt = IMAGE_FORMAT_YUV422PLANAR,
	 .depth = 16,
	 .arrays = 3,
	 .type = F_YUV,
	 },
	{
	 .name = "YUV 4:2:2 semi-planar",
	 .format = PVT_IMG_FMT_YUV422_SEMIPLANAR,
	 .fw_fmt = IMAGE_FORMAT_YUV422SEMIPLANAR,
	 .depth = 16,
	 .arrays = 2,
	 .type = F_YUV,
	 },
	{
	 .name = "YUV 4:2:2 interleaved",
	 .format = PVT_IMG_FMT_YUV422_INTERLEAVED,
	 .fw_fmt = IMAGE_FORMAT_YUV422INTERLEAVED,
	 .depth = 16,
	 .arrays = 1,
	 .type = F_YUV,
	 },
	{
	 .name = "RAW 8",
	 .format = PVT_IMG_FMT_RAW8,
	 .fw_fmt = IMAGE_FORMAT_RGBBAYER8,
	 .depth = 8,
	 .arrays = 1,
	 .type = F_RAW,
	 },
	{
	 .name = "RAW 10",
	 .format = PVT_IMG_FMT_RAW10,
	 .fw_fmt = IMAGE_FORMAT_RGBBAYER10,
	 .depth = 16,
	 .arrays = 1,
	 .type = F_RAW,
	 },
	{
	 .name = "RAW 12",
	 .format = PVT_IMG_FMT_RAW12,
	 .fw_fmt = IMAGE_FORMAT_RGBBAYER12,
	 .depth = 16,
	 .arrays = 1,
	 .type = F_RAW,
	 },
	{
	 .name = "RAW 14",
	 .format = PVT_IMG_FMT_RAW14,
	 .fw_fmt = IMAGE_FORMAT_RGBBAYER14,
	 .depth = 16,
	 .arrays = 1,
	 .type = F_RAW,
	 },
	{
	 .name = "RAW 16",
	 .format = PVT_IMG_FMT_RAW16,
	 .fw_fmt = IMAGE_FORMAT_RGBBAYER16,
	 .depth = 16,
	 .arrays = 1,
	 .type = F_RAW,
	 },
};

struct amd_format *select_default_format()
{
	return &amd_formats[0];
}

struct amd_format *amd_find_format(int index)
{
	ENTER();

	if (index >= (int)ARRAY_SIZE(amd_formats))
		return NULL;

	return &amd_formats[index];
}

struct amd_format *amd_find_match(enum pvt_img_fmt fmt)
{
	int i = 0;

	ENTER();
	for (; i < ARRAY_SIZE(amd_formats); i++) {
		if (amd_formats[i].format == fmt)
			return &amd_formats[i];
	}

	ISP_PR_ERR("failed to find matched format!!");
	return NULL;
}

