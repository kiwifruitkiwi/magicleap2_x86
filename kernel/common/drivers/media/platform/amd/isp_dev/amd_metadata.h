/*
 *Copyright © 2020 Advanced Micro Devices, Inc. All rights reserved.
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
#include <linux/random.h>
#include <linux/cam_api_simulation.h>

#include "amd_common.h"
#include "amd_params.h"
#include "amd_stream.h"
#include "amd_isp.h"
#include "sensor_if.h"
#include "isp_common.h"
#include "isp_module_if.h"
#include "amd_log.h"


int set_ae_lock(struct isp_info *isp, struct cam_stream *s,
		struct amd_cam_metadata *metadata);
int set_ae_mode(struct isp_info *isp, struct cam_stream *s,
		struct amd_cam_metadata *metadata);
int set_itime(struct isp_info *isp, struct cam_stream *s,
		struct amd_cam_metadata *metadata);
int set_again(struct isp_info *isp, struct cam_stream *s,
		struct amd_cam_metadata *metadata);
int set_3a(struct isp_info *isp, struct cam_stream *s,
		struct amd_cam_metadata *metadata);
int set_flicker_mode(struct isp_info *isp, struct cam_stream *s,
		struct amd_cam_metadata *metadata);
int set_ae_regions(struct isp_info *isp, struct cam_stream *s,
		struct amd_cam_metadata *metadata);
int set_ev_compensation(struct isp_info *isp, struct cam_stream *s,
		struct amd_cam_metadata *metadata);
int set_scene_mode(struct isp_info *isp, struct cam_stream *s,
		struct amd_cam_metadata *metadata);

extern int set_kparam_2_fw(struct cam_stream *s);
extern int update_kparam(struct cam_stream *s);

