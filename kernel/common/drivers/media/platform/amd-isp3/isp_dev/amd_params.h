/*
 * Copyright (C) 2019-2020 Advanced Micro Devices, Inc. All rights reserved.
 */

#include <linux/cam_api.h>
#include "isp_fw_if/paramtypes.h"

#ifndef __AMD_PARAMS_H_
#define __AMD_PARAMS_H_

enum format_type {
	F_YUV = 0,
	F_RAW,
	F_RGB,
	F_JPEG,
};

/*
 * define amd_format struct
 * @depth: number of bits per pixel
 */

struct amd_format {
	const char *name;
	enum pvt_img_fmt format;
	enum _ImageFormat_t fw_fmt;
	u8 depth;
	u8 arrays;
	int type;
};

struct amd_format *select_default_format(void);
struct amd_format *amd_find_format(int index);
struct amd_format *amd_find_match(enum pvt_img_fmt fmt);

int translate_ctrl_id(int ctrl_id);
void translate_ctrl_value(struct v4l2_ext_control *ctl, void *param);
void translate_param_value(struct v4l2_ext_control *ctl, void *param);

#endif /* amd_params.h */
