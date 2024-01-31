/*
 * Copyright (C) 2020 Advanced Micro Devices, Inc. All rights reserved.
 */

#include <linux/amd_cam_metadata.h>
#include <linux/errno.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/videodev2.h>
#include <linux/kobject.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/kdev_t.h>
#include <linux/uaccess.h>
#include <media/v4l2-common.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-device.h>
#include <media/v4l2-ioctl.h>
#include <media/v4l2-event.h>
#include <media/videobuf2-memops.h>
#include <media/videobuf2-core.h>
#include <media/videobuf2-v4l2.h>
#include <media/videobuf2-vmalloc.h>
#include <linux/cam_api.h>
#include <linux/random.h>

#include "amd_params.h"
#include "amd_stream.h"
#include "amd_isp.h"
#include "sensor_if.h"
#include "isp_module_if.h"
#include "isp_fw_if/paramtypes.h"
#include "isp_soc_adpt.h"
#include "os_advance_type.h"
#include "amd_isp_ioctl.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif

#define LOG_TAG "[ISP][amd_ioctl]"

int set_clk_gating(unsigned char enable)
{
	int ret = OK;

	if (isp_hdl->intf->clk_gating_enable) {
		ret = isp_hdl->intf->clk_gating_enable(isp_hdl->intf->context,
							CAMERA_ID_REAR, enable);
		if (ret != IMF_RET_SUCCESS) {
			//fail to set clock gating
			ISP_PR_ERR("fail to set clk gating, ret = %d", ret);
		} else {
			//success to set clock gating
			ISP_PR_ERR("set clk gating success");
		}
	} else {
		ISP_PR_ERR("fail, no interface to set clk gating");
		ret = -1;
	}

	return ret;
}

int set_pwr_gating(unsigned char enable)
{
	int ret = OK;

	if (isp_hdl->intf->power_gating_enable) {
		ret = isp_hdl->intf->power_gating_enable(isp_hdl->intf->context,
							CAMERA_ID_REAR, enable);
		if (ret != IMF_RET_SUCCESS) {
			//fail to set power gating
			ISP_PR_ERR("fail to set pwr gating, ret = %d", ret);
		} else {
			//success to set power gating
			ISP_PR_ERR("set pwr gating success");
		}
	} else {
		ISP_PR_ERR("fail, no interface to set pwr gating");
		ret = -1;
	}

	return ret;
}

int dump_raw_enable(enum camera_id cam_id, unsigned char enable)
{
	int ret = OK;

	if (isp_hdl->intf->dump_raw_enable) {
		ret = isp_hdl->intf->dump_raw_enable(isp_hdl->intf->context,
							cam_id, enable);
		if (ret != IMF_RET_SUCCESS) {
			//fail to enable dump raw
			ISP_PR_ERR("fail to enable dump raw, ret = %d", ret);
		} else {
			//success to enable dump raw
			ISP_PR_ERR("enable dump raw success, enable: %d",
					enable);
		}
	} else {
		ISP_PR_ERR("fail, no interface to dump raw enable");
		ret = -1;
	}

	return ret;
}

