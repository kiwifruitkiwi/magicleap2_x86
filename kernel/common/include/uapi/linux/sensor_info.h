/* SPDX-License-Identifier: (GPL-2.0 OR BSD-2-Clause) WITH Linux-syscall-note */
/*
 * sensor_info.h
 *
 * Copyright (C) 2020-2022 Advanced Micro Devices, Inc. All rights reserved.
 *
 * This file is licensed under the terms of the GNU General Public License
 * version 2. This program is licensed "as is" without any warranty of any
 * kind, whether express or implied.
 *
 */

#ifndef __UAPI_LINUX_SENSOR_INFO_H
#define __UAPI_LINUX_SENSOR_INFO_H

#define MAX_NAME_LEN               16
#define MAX_MODE_COUNT             16
#define MAX_PHYSICAL_CAMERA_NUM    2
#define MAX_SUPPORTED_MODES_COUNT  6

/*
 *----------------------------------------------------------------------------
 * Sensor info & profile/mode info
 *----------------------------------------------------------------------------
 */
struct sensor_indexes {
	unsigned int sensor_cnt;
	unsigned int sensor_ids[MAX_PHYSICAL_CAMERA_NUM];
};

enum sensor_position {
	SENSOR_POSITION_REAR        = 0,
	SENSOR_POSITION_FRONT_LEFT  = 1,
	SENSOR_POSITION_FRONT_RIGHT = 2,
	SENSOR_POSITION_MAX,
};

enum module_orientation {
	MODULE_ORIENTATION_0        = 0,
	MODULE_ORIENTATION_90       = 1,
	MODULE_ORIENTATION_180      = 2,
	MODULE_ORIENTATION_270      = 3,
	MODULE_ORIENTATION_MAX,
};

enum pixel_color_format {
	PIXEL_COLOR_FORMAT_INVALID  = 0,
	PIXEL_COLOR_FORMAT_RGGB     = 1,
	PIXEL_COLOR_FORMAT_GRBG     = 2,
	PIXEL_COLOR_FORMAT_GBRG     = 3,
	PIXEL_COLOR_FORMAT_BGGR     = 4,
	PIXEL_COLOR_FORMAT_PURE_IR  = 5,
	PIXEL_COLOR_FORMAT_RIGB     = 6,
	PIXEL_COLOR_FORMAT_RGIB     = 7,
	PIXEL_COLOR_FORMAT_IRBG     = 8,
	PIXEL_COLOR_FORMAT_GRBI     = 9,
	PIXEL_COLOR_FORMAT_IBRG     = 10,
	PIXEL_COLOR_FORMAT_GBRI     = 11,
	PIXEL_COLOR_FORMAT_BIGR     = 12,
	PIXEL_COLOR_FORMAT_BGIR     = 13,
	PIXEL_COLOR_FORMAT_BGRGGIGI = 14,
	PIXEL_COLOR_FORMAT_RGBGGIGI = 15,
	PIXEL_COLOR_FORMAT_MAX,
};

enum pvt_img_fmt {
	PVT_IMG_FMT_INVALID            = 0,
	PVT_IMG_FMT_YV12               = 1,
	PVT_IMG_FMT_I420               = 2,
	PVT_IMG_FMT_NV21               = 3,
	PVT_IMG_FMT_NV12               = 4,
	PVT_IMG_FMT_YUV422P            = 5,
	PVT_IMG_FMT_YUV422_SEMIPLANAR  = 6,
	PVT_IMG_FMT_YUV422_INTERLEAVED = 7,
	PVT_IMG_FMT_L8                 = 8,
	PVT_IMG_FMT_RAW8               = 9,
	PVT_IMG_FMT_RAW10              = 10,
	PVT_IMG_FMT_RAW12              = 11,
	PVT_IMG_FMT_RAW14              = 12,
	PVT_IMG_FMT_RAW16              = 13,
	PVT_IMG_FMT_Y8                 = 14,
	PVT_IMG_FMT_MAX,
};

enum hdr_mode {
	HDR_MODE_INVALID            = 0,
	HDR_MODE_SME                = 1,
	HDR_MODE_DOL2               = 2,
	HDR_MODE_DOL3               = 3,
	HDR_MODE_MAX,
};

enum bin_mode {
	BINNING_MODE_NONE           = 0,
	BINNING_MODE_2x2            = 1,
	BINNING_MODE_2_HORIZONTAL   = 2,
	BINNING_MODE_2_VERTICAL     = 3,
	BINNING_MODE_MAX,
};

enum test_pattern_mode {
	TEST_PATTERN_MODE_OFF                     = 0,
	TEST_PATTERN_MODE_SOLID_COLOR             = 1,
	TEST_PATTERN_MODE_COLOR_BARS              = 2,
	TEST_PATTERN_MODE_COLOR_BARS_FADE_TO_GRAY = 3,
	TEST_PATTERN_MODE_PN9                     = 4,
	TEST_PATTERN_MODE_MAX,
};

enum optical_stabilization_mode {
	OPTICAL_STABILIZATION_MODE_OFF = 0,
	OPTICAL_STABILIZATION_MODE_ON  = 1,
};

enum mode_profile_name {
	PROFILE_INVALID                 = 0,
	PROFILE_NOHDR_2X2BINNING_3MP_15 = 1,
	PROFILE_NOHDR_2X2BINNING_3MP_30 = 2,
	PROFILE_NOHDR_2X2BINNING_3MP_60 = 3,
	PROFILE_NOHDR_V2BINNING_6MP_30  = 4,
	PROFILE_NOHDR_V2BINNING_6MP_60  = 5,
	PROFILE_NOHDR_NOBINNING_12MP_30 = 6,
	PROFILE_MAX,
};

enum sensor_ae_prop_type {
	AE_PROP_TYPE_INVALID =  0,

	/* Analog gain formula: weight1 / (wiehgt2 - param1) */
	AE_PROP_TYPE_SONY    =  1,

	/*
	 * Samsung is same as OV
	 * Analog gain formula: (param1 / weight1) << shift
	 */
	AE_PROP_TYPE_OV      =  2,

	/* AE use script to adjust expo/gain settings */
	AE_PROP_TYPE_SCRIPT  =  3,
	AE_PROP_TYPE_MAX
};

struct sensor_ae_gain_formula {
	unsigned int weight1;     /* constant a */
	unsigned int weight2;     /* constant b */
	unsigned int min_shift;   /* minimum S */
	unsigned int max_shift;   /* maximum S */
	unsigned int min_param;   /* minimum X */
	unsigned int max_param;   /* maximum X */
};

struct mode_info {
	/* e.g. PROFILE_NOHDR_2X2BINNING_3MP_15 */
	enum mode_profile_name mode_id;

	/* Sensor raw format */
	enum pixel_color_format fmt_layout;
	enum pvt_img_fmt fmt_bit;

	/*
	 * ANDROID_SENSOR_INFO_ACTIVE_ARRAY_SIZE
	 * The area of the image sensor which corresponds to active pixels
	 * after any geometric distortion correction has been applied
	 */
	unsigned int active_array_size_w;
	unsigned int active_array_size_h;

	/*
	 * ANDROID_SENSOR_INFO_PIXEL_ARRAY_SIZE
	 * Dimensions of the full pixel array,
	 * possibly including black calibration pixels
	 */
	unsigned int pixel_array_size_w;
	unsigned int pixel_array_size_h;

	/*
	 * ANDROID_SENSOR_INFO_PRE_CORRECTION_ACTIVE_ARRAY_SIZE
	 * The area of the image sensor which corresponds to active pixels
	 * prior to the application of any geometric distortion correction
	 */
	unsigned int pre_correction_size_w;
	unsigned int pre_correction_size_h;

	/* HDR mode */
	enum hdr_mode hdr;

	/* Binning mode */
	enum bin_mode bin;

	/* Sensor output image width */
	unsigned int w;

	/* Sensor output image height */
	unsigned int h;

	/* Time of line (nanosecond precise) */
	unsigned int t_line;

	/* Frame length as exposure line per profile (frame duration) */
	unsigned int frame_length_lines;

	/* Minmum frame rate */
	unsigned int min_frame_rate;

	/* Maximum frame rate */
	unsigned int max_frame_rate;

	/* Bit width for each pixel */
	unsigned int bits_per_pixel;

	/* Initial itegration time, 1000-based fixed point */
	unsigned int init_itime;
};

enum sensor_lens_state {
	SENSOR_LENS_STATE_INVALID      = 0,
	SENSOR_LENS_STATE_SEARCHING    = 1,
	SENSOR_LENS_STATE_CONVERGED    = 2,
	SENSOR_LENS_STATE_MAX
};

#define SENSOR_AF_ROI_MAX_NUMBER 3

struct sensor_af_window {
	unsigned int xmin;
	unsigned int ymin;
	unsigned int xmax;
	unsigned int ymax;
	unsigned int weight;
};

struct sensor_af_roi {
	unsigned int num_roi;
	struct sensor_af_window roi_af[SENSOR_AF_ROI_MAX_NUMBER];
};

struct cvip_exp_data {
	unsigned int integration_time;
	unsigned int analog_gain;
	unsigned int digital_gain;
};

struct cvip_timestamp_inpre {
	unsigned long long readout_start_ts_us;
	unsigned long long centroid_ts_us;
	unsigned long long seq_win_ts_us;
};

enum sensor_af_mode {
	SENSOR_AF_CONTROL_MODE_OFF,
	SENSOR_AF_CONTROL_MODE_AUTO,
	SENSOR_AF_CONTROL_MODE_MACRO,
	SENSOR_AF_CONTROL_MODE_CONTINUOUS_VIDEO,
	SENSOR_AF_CONTROL_MODE_CONTINUOUS_PICTURE,
};

enum sensor_af_trigger {
	SENSOR_AF_CONTROL_TRIGGER_IDLE,
	SENSOR_AF_CONTROL_TRIGGER_START,
	SENSOR_AF_CONTROL_TRIGGER_CANCEL,
};

enum sensor_af_state {
	SENSOR_AF_STATE_INACTIVE,
	SENSOR_AF_STATE_PASSIVE_SCAN,
	SENSOR_AF_STATE_PASSIVE_FOCUSED,
	SENSOR_AF_STATE_ACTIVE_SCAN,
	SENSOR_AF_STATE_FOCUSED_LOCKED,
	SENSOR_AF_STATE_NOT_FOCUSED_LOCKED,
	SENSOR_AF_STATE_PASSIVE_UNFOCUSED,
};

enum sensor_af_scene_change {
	SENSOR_AF_SCENE_CHANGE_NOT_DETECTED,
	SENSOR_AF_SCENE_CHANGE_DETECTED,
};

struct cvip_meta_af {
	enum sensor_af_mode mode;
	enum sensor_af_trigger trigger;
	enum sensor_af_state state;
	enum sensor_af_scene_change scene_change;
	struct sensor_af_roi af_roi;
	unsigned int af_frameid;
	enum sensor_lens_state lens_state;
	float distance;
	float distance_near;
	float distance_far;
	float focus_range_near;
	float focus_range_far;
};

struct cvip_predata {
	struct cvip_exp_data exp_data;
	struct cvip_timestamp_inpre ts_in_pre;
	struct cvip_meta_af af_metadata;
};

struct cvip_timestamp_inpost {
	//readout end timestamp in Us
	unsigned long long readout_stop_ts_us;
};

struct cvip_postdata {
	//Timestamp in post data
	struct cvip_timestamp_inpost  ts_in_post;
};

struct s_info {
	/*
	 * as the left and right sensors are same, so they use the same s_info,
	 * no need sensor_id now, but put it here for the future extension.
	 */
	unsigned int sensor_id;
	char sensor_name[MAX_NAME_LEN];		/* e.g. S5K3L6XX */
	char sensor_manufacturer[MAX_NAME_LEN];	/* e.g. Samsung */
	char module_part_number[MAX_NAME_LEN];	/* e.g. OHP1908 */
	char module_manufacture[MAX_NAME_LEN];	/* e.g. O-Film */
	/* e.g. SENSOR_POSITION_FRONT_LEFT */
	enum sensor_position s_position;
	enum module_orientation m_ori;		/* e.g. SENSOR_ORIENTATION_0 */

	/* If the size is 0, means that not supported */
	unsigned int pre_embedded_size;
	unsigned int post_embedded_size;

	/* One Time Programmable data buffer size and addr */
	unsigned int otp_data_size;
	void *otp_buffer;

	/*
	 * ANDROID_LENS_INFO_AVAILABLE_APERTURES
	 * float x apertures_cnt
	 * If the camera device doesn't support a variable lens aperture, this
	 * list will contain only one value, which is the fixed aperture size.
	 * If the camera device supports a variable aperture, the aperture
	 * values in this list will be sorted in ascending order.
	 */
	unsigned int apertures_cnt;
	float apertures[MAX_MODE_COUNT];

	/*
	 * ANDROID_LENS_INFO_AVAILABLE_FILTER_DENSITIES
	 * float x filter_densities_cnt
	 * If a neutral density filter is not supported by this camera device,
	 * this list will contain only 0. Otherwise, this list will include
	 * every filter density supported by the device, in ascending order.
	 */
	unsigned int filter_densities_cnt;
	float filter_densities[MAX_MODE_COUNT];

	/*
	 * ANDROID_LENS_INFO_AVAILABLE_FOCAL_LENGTHS
	 * float x focal_lengths_cnt
	 * If optical zoom is not supported, this list will only contain a
	 * single value corresponding to the fixed focal length of the device.
	 * Otherwise, this list will include every focal length supported by
	 * the camera device, in ascending order.
	 */
	unsigned int focal_lengths_cnt;
	float focal_lengths[MAX_MODE_COUNT];

	/*
	 * ANDROID_LENS_INFO_AVAILABLE_OPTICAL_STABILIZATION
	 * byte x optical_stabilization_mode_cnt (OFF, ON)
	 * If OIS is not supported by a given camera device, this list will
	 * contain only OFF.
	 */
	unsigned int optical_stabilization_mode_cnt;
	enum optical_stabilization_mode optical_stabilization[MAX_MODE_COUNT];

	/*
	 * ANDROID_LENS_INFO_HYPERFOCAL_DISTANCE
	 * float
	 * If the lens is not fixed focus, the camera device will report this
	 * field when android.lens.info.focusDistanceCalibration is APPROXIMATE
	 * or CALIBRATED.
	 */
	float hyperfocal_distance;

	/*
	 * ANDROID_LENS_INFO_MINIMUM_FOCUS_DISTANCE
	 * float
	 * If the lens is fixed-focus, this will be 0.
	 */
	float min_focus_distance;

	/*
	 * amd.control.lens.minIncrement
	 * float
	 * Increment of distance which results in new lens position
	 */
	float min_increment;

	/*
	 * ANDROID_SENSOR_INFO_PHYSICAL_SIZE
	 * The physical dimensions of the full pixel array
	 * the micron, eg: 3900umx4200um
	 */
	unsigned int physical_size_w;
	unsigned int physical_size_h;

	/*
	 * ANDROID_SENSOR_BLACK_LEVEL_PATTERN
	 * A fixed black level offset for each of the color
	 * filter arrangement (CFA) mosaic channels
	 */
	unsigned int black_level_pattern[4];

	/*
	 * ANDROID_SENSOR_OPTICAL_BLACK_REGIONS
	 * List of disjoint rectangles indicating the sensor
	 * optically shielded black pixel regions
	 */
	unsigned int optical_black_regions_cnt;
	unsigned int optical_black_regions[12];

	/*
	 * ANDROID_SENSOR_AVAILABLE_TEST_PATTERN_MODES
	 * List of sensor test pattern modes
	 */
	unsigned int test_pattern_modes_cnt;
	unsigned int test_pattern_modes[10];

	/* Sensor property for Analog gain calculation */
	enum sensor_ae_prop_type ae_type;

	/* Minimum exposure line */
	unsigned int min_expo_line;

	/* Maximum exposure line */
	unsigned int max_expo_line;

	/*
	 * exposure line alpha for correct frame rate
	 * i.e. expo line < frame line - alpha
	 */
	unsigned int expo_line_alpha;

	/* Minimum analog gain, 1000-based fixed point */
	unsigned int min_again;

	/* Maximum analog gain, 1000-based fixed point */
	unsigned int max_again;

	/* Initial Again, 1000-based fixed point */
	unsigned int init_again;

	/* HDR LE/SE share same analog gain */
	unsigned int shared_again;

	/* Formula for AE gain calculation */
	struct sensor_ae_gain_formula formula;

	/* Minimum digital gain */
	unsigned int min_dgain;

	/* Maximum digital gain */
	unsigned int max_dgain;

	/* How many ISO is equal to 1.x gain */
	unsigned int base_iso;

	/* Count of sensor profiles/modes */
	unsigned int modes_cnt;
	struct mode_info modes[MAX_SUPPORTED_MODES_COUNT];
};

#endif
