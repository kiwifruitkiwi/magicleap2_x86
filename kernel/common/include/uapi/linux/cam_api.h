/*
 * cam_api.h
 *
 * Copyright (C) 2020-2021 Advanced Micro Devices, Inc. All rights reserved.
 *
 * This file is licensed under the terms of the GNU General Public License
 * version 2. This program is licensed "as is" without any warranty of any
 * kind, whether express or implied.
 *
 */

#ifndef __UAPI_LINUX_CAM_H
#define __UAPI_LINUX_CAM_H

#include <linux/videodev2.h>
#include <linux/sensor_info.h>

#define MAX_BUFFERS_PER_STREAM 6
#define MAX_STREAM_NUM_PER_PHYSICAL_CAMERA 6
#define MAX_PHYSICAL_CAMERA_NUM 2
#define AMD_MAGIC_NUM_0 0xAFAFAFAFAFAFAFA0 //for vcam_2
#define AMD_MAGIC_NUM_1 0xAFAFAFAFAFAFAFA1 //for vcam_5

//Driver interface
struct module_profile {
	enum mode_profile_name name;
	__u32 width;
	__u32 height;
	bool binning_mode;
	bool hdr_state;
	__u32 fps;
};

enum stream_rotation {
	STREAM_ROTATION_0   = 0,
	STREAM_ROTATION_90  = 1,
	STREAM_ROTATION_180 = 2,
	STREAM_ROTATION_270 = 3,
};

enum stream_type {
	STREAM_YUV_0 = 0,
	STREAM_YUV_1 = 1,
	STREAM_YUV_2 = 2,
	STREAM_CVPCV = 3,
	STREAM_RAW   = 4,
};

struct profile_switch {
	bool trigger;
	enum mode_profile_name id;
};

struct kernel_request {
	__u32 com_req_id;
	void *kparam;
	void *eng_ptr;
	uint64_t eng_mc;
	__u32 eng_len;
	struct profile_switch prf_switch;
	__u8 tun_switch;
	struct kernel_buffer *input_buffer;
	__u32 num_output_buffers;
	struct kernel_buffer
	*output_buffers[MAX_STREAM_NUM_PER_PHYSICAL_CAMERA];
};

enum kernel_buffer_status {
	BUFFER_STATUS_OK    = 0,
	BUFFER_STATUS_ERROR = 1,
};

struct kernel_buffer {
	enum kernel_buffer_status status;
	int vcam_id;
	enum stream_type s_type;
	void *priv;
	int stream_id;
	void *buffer;
	void *user_ptr;
	uint64_t mc_addr;
	int length;
	__u32 width;
	__u32 height;
	__u32 y_stride;
	__u32 uv_stride;
	enum pvt_img_fmt format;
	enum stream_rotation rotation;
};

enum result_status {
	ERROR_NONE,
	ERROR_BADPOINTER,
	ERROR_INVALIDOPERATION,
	ERROR_OUTOFMEMORY,
	CANCEL,
	 /*TBD*/
};

struct kernel_result {
	__u32 com_req_id;
	struct kernel_buffer *input_buffer;
	__u32 num_output_buffers;
	struct kernel_buffer
	*output_buffers[MAX_STREAM_NUM_PER_PHYSICAL_CAMERA];
	__u32 partial_result;
	enum result_status status;
	bool is_shutter_event;
	union {
		__u64 timestamp;
		void *kparam_rst;
	} data;
};

struct kernel_hotplug_info {
	bool hot_plug_state;
	struct s_info *info;
};

/*
 *----------------------------------------------------------------------------
 * system property from HAL
 *----------------------------------------------------------------------------
 */
enum cvip_status {
	CVIP_STATUS_DISABLE = 0,
	CVIP_STATUS_ENABLE  = 1,
	CVIP_STATUS_MAX,
};

enum available_sensors {
	SENSOR_IMX577 = 0,
	SENSOR_S5K3L6 = 1,
	SENSOR_MAX,
};

enum fw_log_status {
	FW_LOG_STATUS_DISABLE = 0,
	FW_LOG_STATUS_ENABLE  = 1,
	FW_LOG_STATUS_MAX,
};

enum cvip_raw_pkt_fmt {
	CVIP_RAW_PKT_FMT_0 = 0,
	CVIP_RAW_PKT_FMT_1 = 1,
	CVIP_RAW_PKT_FMT_2 = 2,
	CVIP_RAW_PKT_FMT_3 = 3,
	CVIP_RAW_PKT_FMT_4 = 4,
	CVIP_RAW_PKT_FMT_5 = 5,
	CVIP_RAW_PKT_FMT_6 = 6,
	CVIP_RAW_PKT_FMT_MAX,
};

enum trace_log_level {
	TRACE_LOG_LEVEL_NONE    = 0,
	TRACE_LOG_LEVEL_ERROR   = 1,
	TRACE_LOG_LEVEL_WARNING = 2,
	TRACE_LOG_LEVEL_INFO    = 3,
	TRACE_LOG_LEVEL_DEBUG   = 4,
	TRACE_LOG_LEVEL_VERBOSE = 5,
	TRACE_LOG_LEVEL_MAX,
};

enum ae_control_mode {
	/* driver will choose the mode based on sensor source;
	 * async for MIPI sensor, sync for CVIP sensor.
	 */
	AE_CONTROL_MODE_DEFAULT = 0,
	/* set AE control mode to async */
	AE_CONTROL_MODE_ASYNC = 1,
	/* set AE control mode to sync */
	AE_CONTROL_MODE_SYNC = 2,
};

struct s_properties {
	int depth;
	enum available_sensors sensor_id;
	enum cvip_status cvip_enable;
	enum mode_profile_name profile;
	enum fw_log_status fw_log_enable;
	__u64 fw_log_level;
	enum trace_log_level drv_log_level;
	int cvip_raw_slicenum;
	int cvip_output_slicenum;
	int cvip_yuv_w;
	int cvip_yuv_h;
	enum pvt_img_fmt cvip_yuv_fmt;
	enum cvip_raw_pkt_fmt cvip_raw_fmt;
	__u32 phy_cali_value;
	__u32 disable_isp_power_tile;
	bool timestamp_from_cvip;
	enum ae_control_mode aec_mode;
	__u32 prefetch_enable;
};

struct amd_cam_metadata_rational {
	__s32 numerator;
	__s32 denominator;
};

struct amd_cam_metadata_buffer_entry {
	__u32 tag;		// Tag
	__u32 count;		// No.of values of tag of 'type'
	union {
		__u32 offset;	// If count * sizeof(type) > 4, offset to data
		__u8 value[4];	// Tag values
	} data;			// Contains either values of tag or offset to data
	__u32 type;		// Data type of tag
};

/*
 * Metadata Buffer data
 * --------------------------------------------------------------------------
 * |header(amd_cam_metadata)|entries (amd_cam_metadata_buffer_entry)|data|
 ----------------------------------------------------------------------------
 */
struct amd_cam_metadata {
	__u32 size;		// Size of data buffer
	__u32 entry_count;	// Total no.of entries
	__u32 entry_capacity;	// Maximum entries that can be supported
	__u32 entries_start;	// Offset from amd_cam_metadata
	__u32 data_count;	// Size of data in bytes
	__u32 data_capacity;	// Maximum size of data that can be supported
	__u32 data_start;	// Offset from amd_cam_metadata + (entry_capacity *
				// sizeof(amd_cam_metadata_buffer_entry))
};

struct version_info {
	unsigned int fw_ver;
	unsigned int dri_ver;
};

#define VIDIOC_DONE_BUF                                                        \
	_IOWR('V', BASE_VIDIOC_PRIVATE, struct simu_cam_buf)

#define VIDIOC_CONFIG                                                          \
	_IOWR('V', BASE_VIDIOC_PRIVATE + 1, struct module_profile)

#define VIDIOC_REQUEST                                                         \
	_IOWR('V', BASE_VIDIOC_PRIVATE + 2, struct kernel_request)

#define VIDIOC_RESULT                                                          \
	_IOWR('V', BASE_VIDIOC_PRIVATE + 3, struct kernel_result)

#define VIDIOC_HOTPLUG                                                         \
	_IOWR('V', BASE_VIDIOC_PRIVATE + 4, struct kernel_hotplug_info)

#define VIDIOC_GET_SENSOR_LIST                                                 \
	_IOWR('V', BASE_VIDIOC_PRIVATE + 5, struct sensor_indexes)

#define VIDIOC_GET_SENSOR_INFO                                                 \
	_IOWR('V', BASE_VIDIOC_PRIVATE + 6, struct s_info)

#define VIDIOC_SET_CLK_GATING                                                  \
	_IOWR('V', BASE_VIDIOC_PRIVATE + 7, unsigned char)

#define VIDIOC_SET_PWR_GATING                                                  \
	_IOWR('V', BASE_VIDIOC_PRIVATE + 8, unsigned char)

#define VIDIOC_DUMP_RAW_ENABLE                                                 \
	_IOWR('V', BASE_VIDIOC_PRIVATE + 9, unsigned char)

#define VIDIOC_SET_PROPERTY_LIST                                               \
	_IOWR('V', BASE_VIDIOC_PRIVATE + 10, struct s_properties)

#define VIDIOC_GET_VER_INFO                                                    \
	_IOWR('V', BASE_VIDIOC_PRIVATE + 11, struct version_info)
#endif
