/*
 * Copyright (C) 2020 Advanced Micro Devices, Inc. All rights reserved.
 */

#ifndef __AMD_IOCTL_
#define __AMD_IOCTL_

#include <linux/amd_cam_metadata.h>
#include <linux/mutex.h>
#include <media/videobuf2-vmalloc.h>
#include <linux/cam_api.h>

#include "amd_common.h"
#include "amd_params.h"
#include "isp_module_if.h"
#include "isp_module/src/log.h"

int set_clk_gating(unsigned char enable);
int set_pwr_gating(unsigned char enable);
int dump_raw_enable(enum camera_id cam_id, unsigned char enable);
extern struct isp_info *isp_hdl;
#endif
