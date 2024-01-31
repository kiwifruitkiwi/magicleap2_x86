/*
 * Copyright (C) 2018-2022 Advanced Micro Devices, Inc. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors: AMD
 *
 */

#include <linux/uaccess.h>
#include <linux/debugfs.h>
#include <drm/drm_debugfs.h>

#include "dc.h"
#include "amdgpu.h"
#include "amdgpu_dm.h"
#include "amdgpu_dm_debugfs.h"
#include "dm_helpers.h"
#include "dmub/dmub_srv.h"
#include "resource.h"
#include "dsc.h"
#include "dc_link_dp.h"
#include "link_hwss.h"
#include "dc/dc_dmub_srv.h"
#include "dce/dmub_dkmd.h"

struct dmub_debugfs_trace_header {
	uint32_t entry_count;
	uint32_t reserved[3];
};

struct dmub_debugfs_trace_entry {
	uint32_t trace_code;
	uint32_t tick_count;
	uint32_t param0;
	uint32_t param1;
};

static inline const char *yesno(bool v)
{
	return v ? "yes" : "no";
}

/* parse_write_buffer_into_params - Helper function to parse debugfs write buffer into an array
 *
 * Function takes in attributes passed to debugfs write entry
 * and writes into param array.
 * The user passes max_param_num to identify maximum number of
 * parameters that could be parsed.
 *
 */
static int parse_write_buffer_into_params(char *wr_buf, uint32_t wr_buf_size,
					  long *param, const char __user *buf,
					  int max_param_num,
					  uint8_t *param_nums)
{
	char *wr_buf_ptr = NULL;
	uint32_t wr_buf_count = 0;
	int r;
	char *sub_str = NULL;
	const char delimiter[3] = {' ', '\n', '\0'};
	uint8_t param_index = 0;

	*param_nums = 0;

	wr_buf_ptr = wr_buf;

	r = copy_from_user(wr_buf_ptr, buf, wr_buf_size);

		/* r is bytes not be copied */
	if (r >= wr_buf_size) {
		DRM_DEBUG_DRIVER("user data not be read\n");
		return -EINVAL;
	}

	/* check number of parameters. isspace could not differ space and \n */
	while ((*wr_buf_ptr != 0xa) && (wr_buf_count < wr_buf_size)) {
		/* skip space*/
		while (isspace(*wr_buf_ptr) && (wr_buf_count < wr_buf_size)) {
			wr_buf_ptr++;
			wr_buf_count++;
			}

		if (wr_buf_count == wr_buf_size)
			break;

		/* skip non-space*/
		while ((!isspace(*wr_buf_ptr)) && (wr_buf_count < wr_buf_size)) {
			wr_buf_ptr++;
			wr_buf_count++;
		}

		(*param_nums)++;

		if (wr_buf_count == wr_buf_size)
			break;
	}

	if (*param_nums > max_param_num)
		*param_nums = max_param_num;
;

	wr_buf_ptr = wr_buf; /* reset buf pointer */
	wr_buf_count = 0; /* number of char already checked */

	while (isspace(*wr_buf_ptr) && (wr_buf_count < wr_buf_size)) {
		wr_buf_ptr++;
		wr_buf_count++;
	}

	while (param_index < *param_nums) {
		/* after strsep, wr_buf_ptr will be moved to after space */
		sub_str = strsep(&wr_buf_ptr, delimiter);

		r = kstrtol(sub_str, 16, &(param[param_index]));

		if (r)
			DRM_DEBUG_DRIVER("string to int convert error code: %d\n", r);

		param_index++;
	}

	return 0;
}

/* function description
 * get/ set DP configuration: lane_count, link_rate, spread_spectrum
 *
 * valid lane count value: 1, 2, 4
 * valid link rate value:
 * 06h = 1.62Gbps per lane
 * 0Ah = 2.7Gbps per lane
 * 0Ch = 3.24Gbps per lane
 * 14h = 5.4Gbps per lane
 * 1Eh = 8.1Gbps per lane
 * valid link training type value:
 * 0 = LINK_TRAINING_SLOW (Normal link training)
 * 1 = LINK_TRAINING_FAST (No AUX link training)
 * 2 = LINK_TRAINING_NO_PATTERN (No AUX and Idle Pattern link training)
 *
 * debugfs is located at /sys/kernel/debug/dri/0/DP-x/link_settings
 *
 * --- to get dp configuration
 *
 * cat link_settings
 *
 * It will list current, verified, reported, preferred dp configuration.
 * current -- for current video mode
 * verified --- maximum configuration which pass link training
 * reported --- DP rx report caps (DPCD register offset 0, 1 2)
 * preferred --- user force settings
 *
 * --- set (or force) dp configuration
 * Mandatory parameters:
 * - lane_count
 * - link_rate
 * Optional parameters:
 * - link_training_type (this allows to specify, which type of link training
 * to perform with new parameters passed)
 *
 * echo <lane_count>  <link_rate>  <link_training_type> > link_settings
 *
 * for example, to force to  2 lane, 2.7GHz,
 * echo 4 0xa > link_settings
 *
 * Or, if instead of normal slow link training you want
 * to perform No AUX link training with the same parameters as previous command:
 *
 * echo 4 0xa 1 > link_settings
 *
 * spread_spectrum could not be changed dynamically.
 *
 * in case invalid lane count, link rate are force, no hw programming will be
 * done. please check link settings after force operation to see if HW get
 * programming.
 *
 * cat link_settings
 *
 * check current and preferred settings.
 *
 */
static ssize_t dp_link_settings_read(struct file *f, char __user *buf,
				 size_t size, loff_t *pos)
{
	struct amdgpu_dm_connector *connector = file_inode(f)->i_private;
	struct dc_link *link = connector->dc_link;
	char *rd_buf = NULL;
	char *rd_buf_ptr = NULL;
	const uint32_t rd_buf_size = 100;
	uint32_t result = 0;
	uint8_t str_len = 0;
	int r;

	if (*pos & 3 || size & 3)
		return -EINVAL;

	rd_buf = kcalloc(rd_buf_size, sizeof(char), GFP_KERNEL);
	if (!rd_buf)
		return 0;

	rd_buf_ptr = rd_buf;

	str_len = strlen("Current:  %d  %d  %d  ");
	snprintf(rd_buf_ptr, str_len, "Current:  %d  %d  %d  ",
			link->cur_link_settings.lane_count,
			link->cur_link_settings.link_rate,
			link->cur_link_settings.link_spread);
	rd_buf_ptr += str_len;

	str_len = strlen("Verified:  %d  %d  %d  ");
	snprintf(rd_buf_ptr, str_len, "Verified:  %d  %d  %d  ",
			link->verified_link_cap.lane_count,
			link->verified_link_cap.link_rate,
			link->verified_link_cap.link_spread);
	rd_buf_ptr += str_len;

	str_len = strlen("Reported:  %d  %d  %d  ");
	snprintf(rd_buf_ptr, str_len, "Reported:  %d  %d  %d  ",
			link->reported_link_cap.lane_count,
			link->reported_link_cap.link_rate,
			link->reported_link_cap.link_spread);
	rd_buf_ptr += str_len;

	str_len = strlen("Preferred:  %d  %d  %d  ");
	snprintf(rd_buf_ptr, str_len, "Preferred:  %d  %d  %d\n",
			link->preferred_link_setting.lane_count,
			link->preferred_link_setting.link_rate,
			link->preferred_link_setting.link_spread);

	while (size) {
		if (*pos >= rd_buf_size)
			break;

		r = put_user(*(rd_buf + result), buf);
		if (r) {
			kfree(rd_buf);
			return r; /* r = -EFAULT */
		}

		buf += 1;
		size -= 1;
		*pos += 1;
		result += 1;
	}

	kfree(rd_buf);
	return result;
}

static ssize_t dp_link_settings_write(struct file *f, const char __user *buf,
				 size_t size, loff_t *pos)
{
	struct amdgpu_dm_connector *connector = file_inode(f)->i_private;
	struct dc_link *link = connector->dc_link;
	struct dc_link_settings prefer_link_settings;
	char *wr_buf = NULL;
	const uint32_t wr_buf_size = 40;
	/*
	 * param[0]: lane_count;
	 * param[1]: link_rate;
	 * param[2]: link_training_type (optional)
	 * If link training type is non-generic (slow) training, then
	 * param[2] is need from userspace. Otherwise, LT type is slow by-default
	 */
	long param[3] = {0};
	uint8_t param_nums = 0;
	int max_param_num = 3;
	bool valid_input = false;

	if (size == 0)
		return -EINVAL;

	wr_buf = kcalloc(wr_buf_size, sizeof(char), GFP_KERNEL);
	if (!wr_buf)
		return -ENOSPC;

	pr_debug("[LT] userspace input: %s\n", buf);

	if (parse_write_buffer_into_params(wr_buf, size,
					   (long *)param, buf,
					   max_param_num,
					   &param_nums))
		return -EINVAL;

	if (param_nums <= 0) {
		kfree(wr_buf);
		DRM_DEBUG_DRIVER("user data not read\n");
		return -EINVAL;
	}

	switch (param[0]) {
	case LANE_COUNT_ONE:
	case LANE_COUNT_TWO:
	case LANE_COUNT_FOUR:
		valid_input = true;
		break;
	default:
		valid_input = false;
		break;
	}

	switch (param[1]) {
	case LINK_RATE_LOW:
	case LINK_RATE_HIGH:
	case LINK_RATE_RBR2:
	case LINK_RATE_HIGH2:
	case LINK_RATE_HIGH3:
		valid_input = true;
		break;
	default:
		valid_input = false;
		break;
	}

	if (param_nums > 2) {
		switch (param[2]) {
		case LINK_TRAINING_SLOW:
		case LINK_TRAINING_FAST:
		case LINK_TRAINING_NO_PATTERN:
			valid_input = true;
			break;
		default:
			valid_input = false;
			break;
		}
	}

	if (param[2] != LINK_TRAINING_SLOW)
		if (link->cur_link_settings.lane_count != param[0] ||
		    link->cur_link_settings.link_rate != param[1])
			valid_input = false;

	pr_debug("[LT] param # %d, lane ct %ld, link rate %ld, LT type %ld, valid input ? %d\n",
		 param_nums, param[0], param[1], param[2], valid_input);

	if (!valid_input) {
		kfree(wr_buf);
		DRM_DEBUG_DRIVER("Invalid Input value No HW will be programmed\n");
		return size;
	}

	/* save user force lane_count, link_rate to preferred settings
	 * spread spectrum will not be changed
	 */
	memset(&prefer_link_settings, 0, sizeof(struct dc_link_settings));
	prefer_link_settings.link_spread = link->cur_link_settings.link_spread;
	prefer_link_settings.lane_count = param[0];
	prefer_link_settings.link_rate = param[1];
	if (param_nums > 2)
		prefer_link_settings.training_type = param[2];
	else
		prefer_link_settings.training_type = LINK_TRAINING_SLOW;

	if (prefer_link_settings.training_type == LINK_TRAINING_SLOW) {
		link->test_link_training = true;
		dp_retrain_link_dp_test(link, &prefer_link_settings, false);
		link->test_link_training = false;
	} else {
		link->test_link_training = true;
		dc_link_dark_mode_enter(link);
		msleep(1);
		dc_link_dark_mode_exit(link, prefer_link_settings.training_type);
		msleep(10);
		link->test_link_training = false;
	}

	kfree(wr_buf);
	return size;
}

static ssize_t dp_link_active_read(struct file *f, char __user *buf,
		size_t size, loff_t *pos)
{
	struct amdgpu_dm_connector *connector = file_inode(f)->i_private;
	struct dc_link *link = connector->dc_link;
	uint32_t data = -1;
	ssize_t result = 0;
	char *rd_buf = NULL;
	char *rd_buf_ptr = NULL;
	const uint32_t rd_buf_size = 10;
	int r;

	rd_buf = kcalloc(rd_buf_size, sizeof(char), GFP_KERNEL);

	if (!rd_buf)
		return -ENOMEM;

	rd_buf_ptr = rd_buf;
	if (link->link_enc->funcs->dp_get_link_active_status)
		data = link->link_enc->funcs->dp_get_link_active_status(link->link_enc);
	else {
		DRM_DEBUG_DRIVER("error funptr null");
		goto done;
	}
	snprintf(rd_buf_ptr, sizeof(data),
		"%d\n",
		data);
	while (size) {
		if (*pos >= rd_buf_size)
			break;

		r = put_user(*(rd_buf + result), buf);
		if (r) {
			kfree(rd_buf);
			return r; /* r = -EFAULT */
		}
		buf += 1;
		size -= 1;
		*pos += 1;
		result += 1;
	}
done:
	kfree(rd_buf);
	return result;
}

/* function: get current DP PHY settings: voltage swing, pre-emphasis,
 * post-cursor2 (defined by VESA DP specification)
 *
 * valid values
 * voltage swing: 0,1,2,3
 * pre-emphasis : 0,1,2,3
 * post cursor2 : 0,1,2,3
 *
 *
 * how to use this debugfs
 *
 * debugfs is located at /sys/kernel/debug/dri/0/DP-x
 *
 * there will be directories, like DP-1, DP-2,DP-3, etc. for DP display
 *
 * To figure out which DP-x is the display for DP to be check,
 * cd DP-x
 * ls -ll
 * There should be debugfs file, like link_settings, phy_settings.
 * cat link_settings
 * from lane_count, link_rate to figure which DP-x is for display to be worked
 * on
 *
 * To get current DP PHY settings,
 * cat phy_settings
 *
 * To change DP PHY settings,
 * echo <voltage_swing> <pre-emphasis> <post_cursor2> > phy_settings
 * for examle, to change voltage swing to 2, pre-emphasis to 3, post_cursor2 to
 * 0,
 * echo 2 3 0 > phy_settings
 *
 * To check if change be applied, get current phy settings by
 * cat phy_settings
 *
 * In case invalid values are set by user, like
 * echo 1 4 0 > phy_settings
 *
 * HW will NOT be programmed by these settings.
 * cat phy_settings will show the previous valid settings.
 */
static ssize_t dp_phy_settings_read(struct file *f, char __user *buf,
				 size_t size, loff_t *pos)
{
	struct amdgpu_dm_connector *connector = file_inode(f)->i_private;
	struct dc_link *link = connector->dc_link;
	char *rd_buf = NULL;
	const uint32_t rd_buf_size = 20;
	uint32_t result = 0;
	int r;

	if (*pos & 3 || size & 3)
		return -EINVAL;

	rd_buf = kcalloc(rd_buf_size, sizeof(char), GFP_KERNEL);
	if (!rd_buf)
		return -EINVAL;

	snprintf(rd_buf, rd_buf_size, "  %d  %d  %d  ",
			link->cur_lane_setting.VOLTAGE_SWING,
			link->cur_lane_setting.PRE_EMPHASIS,
			link->cur_lane_setting.POST_CURSOR2);

	while (size) {
		if (*pos >= rd_buf_size)
			break;

		r = put_user((*(rd_buf + result)), buf);
		if (r) {
			kfree(rd_buf);
			return r; /* r = -EFAULT */
		}

		buf += 1;
		size -= 1;
		*pos += 1;
		result += 1;
	}

	kfree(rd_buf);
	return result;
}

static ssize_t dp_phy_settings_write(struct file *f, const char __user *buf,
				 size_t size, loff_t *pos)
{
	struct amdgpu_dm_connector *connector = file_inode(f)->i_private;
	struct dc_link *link = connector->dc_link;
	struct dc *dc = (struct dc *)link->dc;
	char *wr_buf = NULL;
	uint32_t wr_buf_size = 40;
	long param[3];
	bool use_prefer_link_setting;
	int max_param_num = 3;
	uint8_t param_nums = 0;
	struct link_training_settings link_lane_settings;
	int r;

	if (size == 0)
		return -EINVAL;

	wr_buf = kcalloc(wr_buf_size, sizeof(char), GFP_KERNEL);
	if (!wr_buf)
		return -ENOSPC;

	if (parse_write_buffer_into_params(wr_buf, size,
					   (long *)param, buf,
					   max_param_num,
					   &param_nums)) {
		kfree(wr_buf);
		return -EINVAL;
	}

	if (param_nums <= 0) {
		kfree(wr_buf);
		DRM_DEBUG_DRIVER("user data not be read\n");
		return -EINVAL;
	}

	if ((param[0] > VOLTAGE_SWING_MAX_LEVEL) ||
			(param[1] > PRE_EMPHASIS_MAX_LEVEL) ||
			(param[2] > POST_CURSOR2_MAX_LEVEL)) {
		kfree(wr_buf);
		DRM_DEBUG_DRIVER("Invalid Input No HW will be programmed\n");
		return size;
	}

	/* get link settings: lane count, link rate */
	use_prefer_link_setting =
		((link->preferred_link_setting.link_rate != LINK_RATE_UNKNOWN) &&
		(link->test_pattern_enabled));

	memset(&link_lane_settings, 0, sizeof(link_lane_settings));

	if (use_prefer_link_setting) {
		link_lane_settings.link_settings.lane_count =
				link->preferred_link_setting.lane_count;
		link_lane_settings.link_settings.link_rate =
				link->preferred_link_setting.link_rate;
		link_lane_settings.link_settings.link_spread =
				link->preferred_link_setting.link_spread;
	} else {
		link_lane_settings.link_settings.lane_count =
				link->cur_link_settings.lane_count;
		link_lane_settings.link_settings.link_rate =
				link->cur_link_settings.link_rate;
		link_lane_settings.link_settings.link_spread =
				link->cur_link_settings.link_spread;
	}

	/* apply phy settings from user */
	for (r = 0; r < link_lane_settings.link_settings.lane_count; r++) {
		link_lane_settings.lane_settings[r].VOLTAGE_SWING =
				(enum dc_voltage_swing) (param[0]);
		link_lane_settings.lane_settings[r].PRE_EMPHASIS =
				(enum dc_pre_emphasis) (param[1]);
		link_lane_settings.lane_settings[r].POST_CURSOR2 =
				(enum dc_post_cursor2) (param[2]);
	}

	/* program ASIC registers and DPCD registers */
	dc_link_set_drive_settings(dc, &link_lane_settings, link);

	kfree(wr_buf);
	return size;
}

/* function description
 *
 * set PHY layer or Link layer test pattern
 * PHY test pattern is used for PHY SI check.
 * Link layer test will not affect PHY SI.
 *
 * Reset Test Pattern:
 * 0 = DP_TEST_PATTERN_VIDEO_MODE
 *
 * PHY test pattern supported:
 * 1 = DP_TEST_PATTERN_D102
 * 2 = DP_TEST_PATTERN_SYMBOL_ERROR
 * 3 = DP_TEST_PATTERN_PRBS7
 * 4 = DP_TEST_PATTERN_80BIT_CUSTOM
 * 5 = DP_TEST_PATTERN_CP2520_1
 * 6 = DP_TEST_PATTERN_CP2520_2 = DP_TEST_PATTERN_HBR2_COMPLIANCE_EYE
 * 7 = DP_TEST_PATTERN_CP2520_3
 *
 * DP PHY Link Training Patterns
 * 8 = DP_TEST_PATTERN_TRAINING_PATTERN1
 * 9 = DP_TEST_PATTERN_TRAINING_PATTERN2
 * a = DP_TEST_PATTERN_TRAINING_PATTERN3
 * b = DP_TEST_PATTERN_TRAINING_PATTERN4
 *
 * DP Link Layer Test pattern
 * c = DP_TEST_PATTERN_COLOR_SQUARES
 * d = DP_TEST_PATTERN_COLOR_SQUARES_CEA
 * e = DP_TEST_PATTERN_VERTICAL_BARS
 * f = DP_TEST_PATTERN_HORIZONTAL_BARS
 * 10= DP_TEST_PATTERN_COLOR_RAMP
 *
 * debugfs phy_test_pattern is located at /syskernel/debug/dri/0/DP-x
 *
 * --- set test pattern
 * echo <test pattern #> > test_pattern
 *
 * If test pattern # is not supported, NO HW programming will be done.
 * for DP_TEST_PATTERN_80BIT_CUSTOM, it needs extra 10 bytes of data
 * for the user pattern. input 10 bytes data are separated by space
 *
 * echo 0x4 0x11 0x22 0x33 0x44 0x55 0x66 0x77 0x88 0x99 0xaa > test_pattern
 *
 * --- reset test pattern
 * echo 0 > test_pattern
 *
 * --- HPD detection is disabled when set PHY test pattern
 *
 * when PHY test pattern (pattern # within [1,7]) is set, HPD pin of HW ASIC
 * is disable. User could unplug DP display from DP connected and plug scope to
 * check test pattern PHY SI.
 * If there is need unplug scope and plug DP display back, do steps below:
 * echo 0 > phy_test_pattern
 * unplug scope
 * plug DP display.
 *
 * "echo 0 > phy_test_pattern" will re-enable HPD pin again so that video sw
 * driver could detect "unplug scope" and "plug DP display"
 */
static ssize_t dp_phy_test_pattern_debugfs_write(struct file *f, const char __user *buf,
				 size_t size, loff_t *pos)
{
	struct amdgpu_dm_connector *connector = file_inode(f)->i_private;
	struct dc_link *link = connector->dc_link;
	char *wr_buf = NULL;
	uint32_t wr_buf_size = 100;
	long param[11] = {0x0};
	int max_param_num = 11;
	enum dp_test_pattern test_pattern = DP_TEST_PATTERN_UNSUPPORTED;
	bool disable_hpd = false;
	bool valid_test_pattern = false;
	uint8_t param_nums = 0;
	/* init with defalut 80bit custom pattern */
	uint8_t custom_pattern[10] = {
			0x1f, 0x7c, 0xf0, 0xc1, 0x07,
			0x1f, 0x7c, 0xf0, 0xc1, 0x07
			};
	struct dc_link_settings prefer_link_settings = {LANE_COUNT_UNKNOWN,
			LINK_RATE_UNKNOWN, LINK_SPREAD_DISABLED};
	struct dc_link_settings cur_link_settings = {LANE_COUNT_UNKNOWN,
			LINK_RATE_UNKNOWN, LINK_SPREAD_DISABLED};
	struct link_training_settings link_training_settings;
	int i;

	if (size == 0)
		return -EINVAL;

	wr_buf = kcalloc(wr_buf_size, sizeof(char), GFP_KERNEL);
	if (!wr_buf)
		return -ENOSPC;

	if (parse_write_buffer_into_params(wr_buf, size,
					   (long *)param, buf,
					   max_param_num,
					   &param_nums)) {
		kfree(wr_buf);
		return -EINVAL;
	}


	if (param_nums <= 0) {
		kfree(wr_buf);
		DRM_DEBUG_DRIVER("user data not be read\n");
		return -EINVAL;
	}

	test_pattern = param[0];

	switch (test_pattern) {
	case DP_TEST_PATTERN_VIDEO_MODE:
	case DP_TEST_PATTERN_COLOR_SQUARES:
	case DP_TEST_PATTERN_COLOR_SQUARES_CEA:
	case DP_TEST_PATTERN_VERTICAL_BARS:
	case DP_TEST_PATTERN_HORIZONTAL_BARS:
	case DP_TEST_PATTERN_COLOR_RAMP:
		valid_test_pattern = true;
		break;

	case DP_TEST_PATTERN_D102:
	case DP_TEST_PATTERN_SYMBOL_ERROR:
	case DP_TEST_PATTERN_PRBS7:
	case DP_TEST_PATTERN_80BIT_CUSTOM:
	case DP_TEST_PATTERN_HBR2_COMPLIANCE_EYE:
	case DP_TEST_PATTERN_TRAINING_PATTERN4:
		disable_hpd = true;
		valid_test_pattern = true;
		break;

	default:
		valid_test_pattern = false;
		test_pattern = DP_TEST_PATTERN_UNSUPPORTED;
		break;
	}

	if (!valid_test_pattern) {
		kfree(wr_buf);
		DRM_DEBUG_DRIVER("Invalid Test Pattern Parameters\n");
		return size;
	}

	if (test_pattern == DP_TEST_PATTERN_80BIT_CUSTOM) {
		for (i = 0; i < 10; i++) {
			if ((uint8_t) param[i + 1] != 0x0)
				break;
		}

		if (i < 10) {
			/* not use default value */
			for (i = 0; i < 10; i++)
				custom_pattern[i] = (uint8_t) param[i + 1];
		}
	}

	/* Usage: set DP physical test pattern using debugfs with normal DP
	 * panel. Then plug out DP panel and connect a scope to measure
	 * For normal video mode and test pattern generated from CRCT,
	 * they are visibile to user. So do not disable HPD.
	 * Video Mode is also set to clear the test pattern, so enable HPD
	 * because it might have been disabled after a test pattern was set.
	 * AUX depends on HPD * sequence dependent, do not move!
	 */
	if (!disable_hpd)
		dc_link_enable_hpd(link);

	prefer_link_settings.lane_count = link->verified_link_cap.lane_count;
	prefer_link_settings.link_rate = link->verified_link_cap.link_rate;
	prefer_link_settings.link_spread = link->verified_link_cap.link_spread;

	cur_link_settings.lane_count = link->cur_link_settings.lane_count;
	cur_link_settings.link_rate = link->cur_link_settings.link_rate;
	cur_link_settings.link_spread = link->cur_link_settings.link_spread;

	link_training_settings.link_settings = cur_link_settings;


	if (test_pattern != DP_TEST_PATTERN_VIDEO_MODE) {
		if (prefer_link_settings.lane_count != LANE_COUNT_UNKNOWN &&
			prefer_link_settings.link_rate !=  LINK_RATE_UNKNOWN &&
			(prefer_link_settings.lane_count != cur_link_settings.lane_count ||
			prefer_link_settings.link_rate != cur_link_settings.link_rate))
			link_training_settings.link_settings = prefer_link_settings;
	}

	for (i = 0; i < (unsigned int)(link_training_settings.link_settings.lane_count); i++)
		link_training_settings.lane_settings[i] = link->cur_lane_setting;

	dc_link_set_test_pattern(
		link,
		test_pattern,
		DP_TEST_PATTERN_COLOR_SPACE_RGB,
		&link_training_settings,
		custom_pattern,
		10);

	/* Usage: Set DP physical test pattern using AMDDP with normal DP panel
	 * Then plug out DP panel and connect a scope to measure DP PHY signal.
	 * Need disable interrupt to avoid SW driver disable DP output. This is
	 * done after the test pattern is set.
	 */
	if (valid_test_pattern && disable_hpd)
		dc_link_disable_hpd(link);

	kfree(wr_buf);

	return size;
}

/**
 * dp_phy_test_pattern_debugfs_read() - Print DPG status
 *
 * Currently, this simply returns 1 or 0 depending on whether DPG is enabled.
 * Can be expanded in the future.
 */
static ssize_t dp_phy_test_pattern_debugfs_read(struct file *f,
						char __user *buf,
						size_t size, loff_t *pos)
{
	struct amdgpu_dm_connector *connector = file_inode(f)->i_private;
	struct drm_device *dev = connector->base.dev;
	struct dc_link *link = connector->dc_link;
	struct dm_crtc_state *dm_crtc_state;
	struct drm_modeset_acquire_ctx ctx;
	struct pipe_ctx *pipe_ctx = NULL;
	struct drm_crtc *crtc = NULL;
	const uint32_t rd_buf_size = 10;
	uint32_t data = -1;
	ssize_t result = 0;
	char *rd_buf = NULL;
	char *rd_buf_ptr = NULL;
	int i, ret;
	bool try_again = false;

	/* Early return if read position is already at rd_buf_size */
	if (*pos >= rd_buf_size)
		return 0;

	drm_modeset_acquire_init(&ctx, 0);

	do {
		try_again = false;

		ret = drm_modeset_lock(&dev->mode_config.connection_mutex, &ctx);
		if (ret == -EDEADLK) {
			ret = drm_modeset_backoff(&ctx);
			if (!ret) {
				try_again = true;
				continue;
			}
			break;
		} else if (ret) {
			break;
		}

		/* Attempt to identify the crtc routed to the connector */
		if (!connector->base.state || !connector->base.state->crtc) {
			ret = -ENODEV;
			break;
		}

		crtc = connector->base.state->crtc;

		ret = drm_modeset_lock(&crtc->mutex, &ctx);
		if (ret == -EDEADLK) {
			ret = drm_modeset_backoff(&ctx);
			if (!ret) {
				try_again = true;
				continue;
			}
			break;
		} else if (ret) {
			break;
		}

		ret = -ENODEV;

		if (!crtc->state)
			break;

		dm_crtc_state = to_dm_crtc_state(crtc->state);
		if (!dm_crtc_state->stream)
			break;

		/* Identify the OPP resource via pipe_ctx */
		for (i = 0; i < link->dc->res_pool->pipe_count; i++) {
			pipe_ctx = &link->dc->current_state->res_ctx.pipe_ctx[i];

			if (pipe_ctx && pipe_ctx->stream == dm_crtc_state->stream)
				break;
		}

		if (!pipe_ctx->stream_res.opp->funcs->opp_get_dpg_enable)
			break;

		data = pipe_ctx->stream_res.opp->funcs->opp_get_dpg_enable(
				pipe_ctx->stream_res.opp);
	} while (try_again);

	drm_modeset_drop_locks(&ctx);
	drm_modeset_acquire_fini(&ctx);

	/* Fill user buffer */
	rd_buf = kcalloc(rd_buf_size, sizeof(char), GFP_KERNEL);

	ret = -ENOMEM;
	if (!rd_buf)
		goto unlock;
	rd_buf_ptr = rd_buf;

	snprintf(rd_buf_ptr, rd_buf_size, "%d\n", data);
	while (size) {
		if (*pos >= rd_buf_size)
			break;

		ret = put_user(*(rd_buf + result), buf);
		if (ret)
			goto unlock;
		buf += 1;
		size -= 1;
		*pos += 1;
		result += 1;
	}

	ret = result;
unlock:
	kfree(rd_buf);

	return ret;
}

static int optc_underflow_read_helper(struct amdgpu_dm_connector *connector,
					  bool clear)
{
	struct drm_device *dev = connector->base.dev;
	struct dc_link *link = connector->dc_link;
	struct dm_crtc_state *dm_crtc_state;
	struct drm_modeset_acquire_ctx ctx;
	struct pipe_ctx *pipe_ctx = NULL;
	struct drm_crtc *crtc = NULL;
	bool try_again = false;
	int i, ret;

	drm_modeset_acquire_init(&ctx, 0);

	do {
		try_again = false;
		ret = drm_modeset_lock(&dev->mode_config.connection_mutex, &ctx);
		if (ret == -EDEADLK) {
			ret = drm_modeset_backoff(&ctx);
			if (!ret) {
				try_again = true;
				continue;
			}
			break;
		} else if (ret) {
			break;
		}

		/* Attempt to identify the crtc routed to the connector */
		if (!connector->base.state || !connector->base.state->crtc) {
			ret = -ENODEV;
			break;
		}
		crtc = connector->base.state->crtc;

		ret = drm_modeset_lock(&crtc->mutex, &ctx);

		if (ret == -EDEADLK) {
			ret = drm_modeset_backoff(&ctx);
			if (!ret) {
				try_again = true;
				continue;
			}
			break;
		} else if (ret) {
			break;
		}

		ret = -ENODEV;

		if (!crtc->state)
			break;

		dm_crtc_state = to_dm_crtc_state(crtc->state);
		if (!dm_crtc_state->stream)
			break;

		/* Identify the OPP resource via pipe_ctx */
		for (i = 0; i < link->dc->res_pool->pipe_count; i++) {
			pipe_ctx = &link->dc->current_state->res_ctx.pipe_ctx[i];

			if (pipe_ctx && pipe_ctx->stream == dm_crtc_state->stream)
				break;
		}

		if (!pipe_ctx->stream_res.tg->funcs->is_optc_underflow_occurred ||
		    !pipe_ctx->stream_res.tg->funcs->clear_optc_underflow)
			break;

		ret = pipe_ctx->stream_res.tg->funcs->is_optc_underflow_occurred(
				pipe_ctx->stream_res.tg);

		if (clear)
			pipe_ctx->stream_res.tg->funcs->clear_optc_underflow(
					pipe_ctx->stream_res.tg);
	} while (try_again);

	drm_modeset_drop_locks(&ctx);
	drm_modeset_acquire_fini(&ctx);

	return ret;
}

/**
 * optc_underflow_read() - Read top pipe's OPTC underflow status, then clear it
 */
static ssize_t optc_underflow_read(
	struct file *f,
	char __user *buf,
	size_t size,
	loff_t *pos)
{
	struct amdgpu_dm_connector *connector = file_inode(f)->i_private;
	const uint32_t rd_buf_size = 10;
	int32_t data;
	ssize_t result = 0;
	char *rd_buf = NULL;
	char *rd_buf_ptr = NULL;
	int r;

	/* Early return if read position is already at rd_buf_size */
	if (*pos >= rd_buf_size)
		return 0;

	data = optc_underflow_read_helper(connector, false);
	if (data < 0)
		return data;

	/* Fill user buffer */
	rd_buf = kcalloc(rd_buf_size, sizeof(char), GFP_KERNEL);
	if (!rd_buf)
		return -ENOMEM;

	rd_buf_ptr = rd_buf;

	snprintf(rd_buf_ptr, rd_buf_size, "%d\n", data);
	while (size) {
		if (*pos >= rd_buf_size)
			break;

		r = put_user(*(rd_buf + result), buf);
		if (r) {
			kfree(rd_buf);
			return r;
		}
		buf += 1;
		size -= 1;
		*pos += 1;
		result += 1;
	}

	kfree(rd_buf);
	return result;
}

static ssize_t optc_underflow_write(
	struct file *f,
	const char __user *buf,
	size_t size,
	loff_t *pos)
{
	struct amdgpu_dm_connector *connector = file_inode(f)->i_private;
	int r;

	if (size == 0)
		return 0;

	r = optc_underflow_read_helper(connector, true);
	if (r < 0)
		return r;

	return size;
}

/**
 * Returns the DMCUB tracebuffer contents.
 * Example usage: cat /sys/kernel/debug/dri/0/amdgpu_dm_dmub_tracebuffer
 */
static int dmub_tracebuffer_show(struct seq_file *m, void *data)
{
	struct amdgpu_device *adev = m->private;
	struct dmub_srv_fb_info *fb_info = adev->dm.dmub_fb_info;
	struct dmub_debugfs_trace_entry *entries;
	uint8_t *tbuf_base;
	uint32_t tbuf_size, max_entries, num_entries, i;

	if (!fb_info)
		return 0;

	tbuf_base = (uint8_t *)fb_info->fb[DMUB_WINDOW_5_TRACEBUFF].cpu_addr;
	if (!tbuf_base)
		return 0;

	tbuf_size = fb_info->fb[DMUB_WINDOW_5_TRACEBUFF].size;
	max_entries = (tbuf_size - sizeof(struct dmub_debugfs_trace_header)) /
		      sizeof(struct dmub_debugfs_trace_entry);

	num_entries =
		((struct dmub_debugfs_trace_header *)tbuf_base)->entry_count;

	num_entries = min(num_entries, max_entries);

	entries = (struct dmub_debugfs_trace_entry
			   *)(tbuf_base +
			      sizeof(struct dmub_debugfs_trace_header));

	for (i = 0; i < num_entries; ++i) {
		struct dmub_debugfs_trace_entry *entry = &entries[i];

		seq_printf(m,
			   "trace_code=%u tick_count=%u param0=%u param1=%u\n",
			   entry->trace_code, entry->tick_count, entry->param0,
			   entry->param1);
	}

	return 0;
}

/*
 * Returns the current and maximum output bpc for the connector.
 * Example usage: cat /sys/kernel/debug/dri/0/DP-1/output_bpc
 */
static int output_bpc_show(struct seq_file *m, void *data)
{
	struct drm_connector *connector = m->private;
	struct drm_device *dev = connector->dev;
	struct drm_crtc *crtc = NULL;
	struct dm_crtc_state *dm_crtc_state = NULL;
	int res = -ENODEV;
	unsigned int bpc;

	mutex_lock(&dev->mode_config.mutex);
	drm_modeset_lock(&dev->mode_config.connection_mutex, NULL);

	if (connector->state == NULL)
		goto unlock;

	crtc = connector->state->crtc;
	if (crtc == NULL)
		goto unlock;

	drm_modeset_lock(&crtc->mutex, NULL);
	if (crtc->state == NULL)
		goto unlock;

	dm_crtc_state = to_dm_crtc_state(crtc->state);
	if (dm_crtc_state->stream == NULL)
		goto unlock;

	switch (dm_crtc_state->stream->timing.display_color_depth) {
	case COLOR_DEPTH_666:
		bpc = 6;
		break;
	case COLOR_DEPTH_888:
		bpc = 8;
		break;
	case COLOR_DEPTH_101010:
		bpc = 10;
		break;
	case COLOR_DEPTH_121212:
		bpc = 12;
		break;
	case COLOR_DEPTH_161616:
		bpc = 16;
		break;
	default:
		goto unlock;
	}

	seq_printf(m, "Current: %u\n", bpc);
	seq_printf(m, "Maximum: %u\n", connector->display_info.bpc);
	res = 0;

unlock:
	if (crtc)
		drm_modeset_unlock(&crtc->mutex);

	drm_modeset_unlock(&dev->mode_config.connection_mutex);
	mutex_unlock(&dev->mode_config.mutex);

	return res;
}

/*
 * Returns the min and max vrr vfreq through the connector's debugfs file.
 * Example usage: cat /sys/kernel/debug/dri/0/DP-1/vrr_range
 */
static int vrr_range_show(struct seq_file *m, void *data)
{
	struct drm_connector *connector = m->private;
	struct amdgpu_dm_connector *aconnector = to_amdgpu_dm_connector(connector);

	if (connector->status != connector_status_connected)
		return -ENODEV;

	seq_printf(m, "Min: %u\n", (unsigned int)aconnector->min_vfreq);
	seq_printf(m, "Max: %u\n", (unsigned int)aconnector->max_vfreq);

	return 0;
}

#ifdef CONFIG_DRM_AMD_DC_HDCP
/*
 * Returns the HDCP capability of the Display (1.4 for now).
 *
 * NOTE* Not all HDMI displays report their HDCP caps even when they are capable.
 * Since its rare for a display to not be HDCP 1.4 capable, we set HDMI as always capable.
 *
 * Example usage: cat /sys/kernel/debug/dri/0/DP-1/hdcp_sink_capability
 *		or cat /sys/kernel/debug/dri/0/HDMI-A-1/hdcp_sink_capability
 */
static int hdcp_sink_capability_show(struct seq_file *m, void *data)
{
	struct drm_connector *connector = m->private;
	struct amdgpu_dm_connector *aconnector = to_amdgpu_dm_connector(connector);
	bool hdcp_cap, hdcp2_cap;

	DRM_DEBUG_DRIVER("display %d (conn-%d) status %d\n", connector->index, connector->base.id, connector->status);

	if (connector->status != connector_status_connected)
		return -ENODEV;

	seq_printf(m, "%s:%d HDCP version: ", connector->name, connector->base.id);

	hdcp_cap = dc_link_is_hdcp14(aconnector->dc_link);
	hdcp2_cap = dc_link_is_hdcp22(aconnector->dc_link);

	if (hdcp_cap)
		seq_printf(m, "%s ", "HDCP1.4");
	if (hdcp2_cap)
		seq_printf(m, "%s ", "HDCP2.2");
	if (!hdcp_cap && !hdcp2_cap)
		seq_printf(m, "%s ", "None");


	seq_puts(m, "\n");

	return 0;
}
#endif
/* function description
 *
 * generic SDP message access for testing
 *
 * debugfs sdp_message is located at /syskernel/debug/dri/0/DP-x
 *
 * SDP header
 * Hb0 : Secondary-Data Packet ID
 * Hb1 : Secondary-Data Packet type
 * Hb2 : Secondary-Data-packet-specific header, Byte 0
 * Hb3 : Secondary-Data-packet-specific header, Byte 1
 *
 * for using custom sdp message: input 4 bytes SDP header and 32 bytes raw data
 */
static ssize_t dp_sdp_message_debugfs_write(struct file *f, const char __user *buf,
				 size_t size, loff_t *pos)
{
	int r;
	uint8_t data[36];
	struct amdgpu_dm_connector *connector = file_inode(f)->i_private;
	struct dm_crtc_state *acrtc_state;
	uint32_t write_size = 36;

	if (connector->base.status != connector_status_connected)
		return -ENODEV;

	if (size == 0)
		return 0;

	acrtc_state = to_dm_crtc_state(connector->base.state->crtc->state);

	r = copy_from_user(data, buf, write_size);

	write_size -= r;

	dc_stream_send_dp_sdp(acrtc_state->stream, data, write_size);

	return write_size;
}

static int darkmode_on_show(struct seq_file *m, void *data)
{

	struct drm_connector *connector = m->private;
	struct drm_modeset_acquire_ctx ctx;
	struct drm_device *dev = connector->dev;
	struct amdgpu_device *adev = dev->dev_private;
	struct dc *dc = adev->dm.dc;

	struct drm_crtc *crtc = NULL;
	struct dm_crtc_state *dm_crtc_state = NULL;
	struct dc_stream_darkmode_state dark_state;

	bool try_again = false;
	int ret = -ENODEV;

	memset(&dark_state, 0, sizeof(dark_state));
	drm_modeset_acquire_init(&ctx, 0);

	do {
		try_again = false;

		ret = drm_modeset_lock(&dev->mode_config.connection_mutex,
				       &ctx);
		if (ret == -EDEADLK) {
			ret = drm_modeset_backoff(&ctx);
			if (!ret) {
				try_again = true;
				continue;
			}
			break;
		} else if (ret) {
			break;
		}

		if (!connector->state || !connector->state->crtc) {
			ret = -ENODEV;
			break;
		}

		crtc = connector->state->crtc;

		ret = drm_modeset_lock(&crtc->mutex, &ctx);

		if (ret == -EDEADLK) {
			ret = drm_modeset_backoff(&ctx);
			if (!ret) {
				try_again = true;
				continue;
			}
			break;
		} else if (ret) {
			break;
		}

		ret = -ENODEV;

		if (crtc->state == NULL)
			break;

		dm_crtc_state = to_dm_crtc_state(crtc->state);
		if (dm_crtc_state->stream == NULL)
			break;

		if (!dc->hwss.get_darkmode_state(dm_crtc_state->stream,
						 &dark_state)) {
			DRM_DEBUG_DRIVER("[CONNECTOR:%d:%s] [CRTC:%d:%s] "
					 "Failed to get darkmode state\n",
					connector->base.id, connector->name,
					crtc->base.id, crtc->name);
			break;
		}

		DRM_DEBUG_DRIVER("[CONNECTOR:%d:%s] [CRTC:%d:%s] "
				 "Darkmode state: DPP_CLK_GATED %d | "
				 "HUBP_CLK_GATED %d | HUBP_NO_OTSD_REQS %d | "
				 "CRTC_BLANKED %d | PHY_OFF %d\n",
				connector->base.id, connector->name,
				crtc->base.id, crtc->name,
				dark_state.dpp_clk_gated,
				dark_state.hubp_clk_gated,
				dark_state.hubp_no_outstanding_reqs,
				dark_state.crtc_blanked, dark_state.phy_off);

		/* TODO: Ignoring everything else except for phy for now */
		seq_printf(m, dark_state.phy_off ? "1\n" : "0\n");

		ret = 0;
	} while (try_again);

	drm_modeset_drop_locks(&ctx);
	drm_modeset_acquire_fini(&ctx);

	return ret;
}

static ssize_t dp_dpcd_address_write(struct file *f, const char __user *buf,
				 size_t size, loff_t *pos)
{
	int r;
	struct amdgpu_dm_connector *connector = file_inode(f)->i_private;

	if (size < sizeof(connector->debugfs_dpcd_address))
		return 0;

	r = copy_from_user(&connector->debugfs_dpcd_address,
			buf, sizeof(connector->debugfs_dpcd_address));

	return size - r;
}

static ssize_t dp_dpcd_size_write(struct file *f, const char __user *buf,
				 size_t size, loff_t *pos)
{
	int r;
	struct amdgpu_dm_connector *connector = file_inode(f)->i_private;

	if (size < sizeof(connector->debugfs_dpcd_size))
		return 0;

	r = copy_from_user(&connector->debugfs_dpcd_size,
			buf, sizeof(connector->debugfs_dpcd_size));

	if (connector->debugfs_dpcd_size > 256)
		connector->debugfs_dpcd_size = 0;

	return size - r;
}

static ssize_t dp_force_ignore_msa_bit_write(struct file *f, const char __user *buf,
					      size_t size, loff_t *pos)
{
	int i, r;
	char *data;
	struct amdgpu_dm_connector *aconnector = file_inode(f)->i_private;
	uint32_t write_size = 1;
	struct pipe_ctx *pipe_ctx;

	if (size < write_size)
		return 0;

	data = kzalloc(write_size, GFP_KERNEL);
	if (!data)
		return 0;

	r = copy_from_user(data, buf, write_size);

	for (i = 0; i < MAX_PIPES; i++) {
		pipe_ctx = &aconnector->dc_link->dc->current_state->res_ctx.pipe_ctx[i];
			if (pipe_ctx && pipe_ctx->stream &&
			    pipe_ctx->stream->link == aconnector->dc_link)
				break;
		pipe_ctx = NULL;
	}

	if (pipe_ctx) {
		bool flag = (int) data[0];

		if (flag != pipe_ctx->stream->force_ignore_msa_bit)
			pipe_ctx->stream->force_ignore_msa_bit = flag;
	}

	kfree(data);
	return size - r;
}

static ssize_t dp_force_ignore_msa_bit_read(struct file *f, char __user *buf,
					     size_t size, loff_t *pos)
{
	int i, r;
	char *data;
	struct amdgpu_dm_connector *aconnector = file_inode(f)->i_private;
	uint32_t read_size = 1;
	struct pipe_ctx *pipe_ctx;

	if (size < read_size)
		return 0;

	data = kzalloc(read_size, GFP_KERNEL);
	if (!data)
		return 0;

	for (i = 0; i < MAX_PIPES; i++) {
		pipe_ctx = &aconnector->dc_link->dc->current_state->res_ctx.pipe_ctx[i];

			if (pipe_ctx && pipe_ctx->stream &&
			    pipe_ctx->stream->link == aconnector->dc_link)
				break;

		pipe_ctx = NULL;
	}

	if (pipe_ctx)
		data[0] = pipe_ctx->stream->force_ignore_msa_bit;

	r = copy_to_user(buf, data, read_size);

	kfree(data);
	return read_size - r;
}

static ssize_t dp_dpcd_data_write(struct file *f, const char __user *buf,
				 size_t size, loff_t *pos)
{
	int r;
	char *data;
	struct amdgpu_dm_connector *connector = file_inode(f)->i_private;
	struct dc_link *link = connector->dc_link;
	uint32_t write_size = connector->debugfs_dpcd_size;

	if (size < write_size)
		return 0;

	data = kzalloc(write_size, GFP_KERNEL);
	if (!data)
		return 0;

	r = copy_from_user(data, buf, write_size);

	dm_helpers_dp_write_dpcd(link->ctx, link,
			connector->debugfs_dpcd_address, data, write_size - r);
	kfree(data);
	return write_size - r;
}

static ssize_t dp_dpcd_data_read(struct file *f, char __user *buf,
				 size_t size, loff_t *pos)
{
	int r;
	char *data;
	struct amdgpu_dm_connector *connector = file_inode(f)->i_private;
	struct dc_link *link = connector->dc_link;
	uint32_t read_size = connector->debugfs_dpcd_size;

	if (size < read_size)
		return 0;

	data = kzalloc(read_size, GFP_KERNEL);
	if (!data)
		return 0;

	dm_helpers_dp_read_dpcd(link->ctx, link,
			connector->debugfs_dpcd_address, data, read_size);

	r = copy_to_user(buf, data, read_size);

	kfree(data);
	return read_size - r;
}

/* function: Read link's DSC & FEC capabilities
 *
 *
 * Access it with the following command (you need to specify
 * connector like DP-1):
 *
 *	cat /sys/kernel/debug/dri/0/DP-X/dp_dsc_fec_support
 *
 */
static int dp_dsc_fec_support_show(struct seq_file *m, void *data)
{
	struct drm_connector *connector = m->private;
	struct drm_modeset_acquire_ctx ctx;
	struct drm_device *dev = connector->dev;
	struct amdgpu_dm_connector *aconnector = to_amdgpu_dm_connector(connector);
	int ret = 0;
	bool try_again = false;
	bool is_fec_supported = false;
	bool is_dsc_supported = false;
	struct dpcd_caps dpcd_caps;

	drm_modeset_acquire_init(&ctx, DRM_MODESET_ACQUIRE_INTERRUPTIBLE);
	do {
		try_again = false;
		ret = drm_modeset_lock(&dev->mode_config.connection_mutex, &ctx);
		if (ret) {
			if (ret == -EDEADLK) {
				ret = drm_modeset_backoff(&ctx);
				if (!ret) {
					try_again = true;
					continue;
				}
			}
			break;
		}
		if (connector->status != connector_status_connected) {
			ret = -ENODEV;
			break;
		}
		dpcd_caps = aconnector->dc_link->dpcd_caps;
		if (aconnector->port) {
			/* aconnector sets dsc_aux during get_modes call
			 * if MST connector has it means it can either
			 * enable DSC on the sink device or on MST branch
			 * its connected to.
			 */
			if (aconnector->dsc_aux) {
				is_fec_supported = true;
				is_dsc_supported = true;
			}
		} else {
			is_fec_supported = dpcd_caps.fec_cap.raw & 0x1;
			is_dsc_supported = dpcd_caps.dsc_caps.dsc_basic_caps.raw[0] & 0x1;
		}
	} while (try_again);

	drm_modeset_drop_locks(&ctx);
	drm_modeset_acquire_fini(&ctx);

	seq_printf(m, "FEC_Sink_Support: %s\n", yesno(is_fec_supported));
	seq_printf(m, "DSC_Sink_Support: %s\n", yesno(is_dsc_supported));

	return ret;
}


/* function: Trigger virtual HPD redetection on connector
 *
 * This function will perform link rediscovery, link disable
 * and enable, and dm connector state update.
 *
 * Retrigger HPD on an existing connector by echoing 1 into
 * its respectful "trigger_hotplug" debugfs entry:
 *
 *	echo 1 > /sys/kernel/debug/dri/0/DP-X/trigger_hotplug
 *
 * This function can perform HPD unplug:
 *
 *	echo 0 > /sys/kernel/debug/dri/0/DP-X/trigger_hotplug
 *
 */
static ssize_t dp_trigger_hotplug(struct file *f, const char __user *buf,
			      size_t size, loff_t *pos)
{
	struct amdgpu_dm_connector *aconnector = file_inode(f)->i_private;
	struct drm_connector *connector = &aconnector->base;
	struct dc_link *link = NULL;
	struct drm_device *dev = connector->dev;
	enum dc_connection_type new_connection_type = dc_connection_none;
	char *wr_buf = NULL;
	uint32_t wr_buf_size = 42;
	int max_param_num = 1;
	long param[1] = {0};
	uint8_t param_nums = 0;

	if (!aconnector || !aconnector->dc_link)
		return -EINVAL;

	if (size == 0)
		return -EINVAL;

	wr_buf = kcalloc(wr_buf_size, sizeof(char), GFP_KERNEL);

	if (!wr_buf) {
		DRM_DEBUG_DRIVER("no memory to allocate write buffer\n");
		return -ENOSPC;
	}

	if (parse_write_buffer_into_params(wr_buf, size,
					   (long *)param, buf,
					   max_param_num,
					   &param_nums)) {
		kfree(wr_buf);
		return -EINVAL;
	}

	if (param_nums <= 0) {
		DRM_DEBUG_DRIVER("user data not be read\n");
		kfree(wr_buf);
		return -EINVAL;
	}

	mutex_lock(&aconnector->hpd_lock);

	if (param[0] == 1) {
		if (!dc_link_detect_sink(aconnector->dc_link, &new_connection_type) &&
		    new_connection_type != dc_connection_none)
			goto unlock;

		if (!dc_link_detect(aconnector->dc_link, DETECT_REASON_HPD))
			goto unlock;

		amdgpu_dm_update_connector_after_detect(aconnector);

		drm_modeset_lock_all(dev);
		dm_restore_drm_connector_state(dev, connector);
		drm_modeset_unlock_all(dev);

		drm_kms_helper_hotplug_event(dev);
	} else if (param[0] == 0) {

		if (!aconnector->dc_link)
			goto unlock;

		link = aconnector->dc_link;

		if (link->local_sink) {
			dc_sink_release(link->local_sink);
			link->local_sink = NULL;
		}

		link->dpcd_sink_count = 0;
		link->type = dc_connection_none;
		link->dongle_max_pix_clk = 0;

		amdgpu_dm_update_connector_after_detect(aconnector);

		drm_modeset_lock_all(dev);
		dm_restore_drm_connector_state(dev, connector);
		drm_modeset_unlock_all(dev);

		drm_kms_helper_hotplug_event(dev);
	}

unlock:
	mutex_unlock(&aconnector->hpd_lock);
	kfree(wr_buf);
	return size;
}


/* function: read DSC status on the connector
 *
 * The read function: dp_dsc_clock_en_read
 * returns current status of DSC clock on the connector.
 * The return is a boolean flag: 1 or 0.
 *
 * Access it with the following command (you need to specify
 * connector like DP-1):
 *
 *	cat /sys/kernel/debug/dri/0/DP-X/dsc_clock_en
 *
 * Expected output:
 * 1 - means that DSC is currently enabled
 * 0 - means that DSC is disabled
 */
static ssize_t dp_dsc_clock_en_read(struct file *f, char __user *buf,
				    size_t size, loff_t *pos)
{
	char *rd_buf = NULL;
	char *rd_buf_ptr = NULL;
	struct amdgpu_dm_connector *aconnector = file_inode(f)->i_private;
	struct display_stream_compressor *dsc;
	struct dcn_dsc_state dsc_state = {0};
	const uint32_t rd_buf_size = 10;
	struct pipe_ctx *pipe_ctx;
	ssize_t result = 0;
	int i, r, str_len = 30;

	rd_buf = kcalloc(rd_buf_size, sizeof(char), GFP_KERNEL);

	if (!rd_buf)
		return -ENOMEM;

	rd_buf_ptr = rd_buf;

	for (i = 0; i < MAX_PIPES; i++) {
		pipe_ctx = &aconnector->dc_link->dc->current_state->res_ctx.pipe_ctx[i];
			if (pipe_ctx && pipe_ctx->stream &&
			    pipe_ctx->stream->link == aconnector->dc_link)
				break;
	}

	if (!pipe_ctx) {
		kfree(rd_buf);
		return -ENXIO;
	}

	dsc = pipe_ctx->stream_res.dsc;
	if (dsc)
		dsc->funcs->dsc_read_state(dsc, &dsc_state);

	snprintf(rd_buf_ptr, str_len,
		"%d\n",
		dsc_state.dsc_clock_en);
	rd_buf_ptr += str_len;

	while (size) {
		if (*pos >= rd_buf_size)
			break;

		r = put_user(*(rd_buf + result), buf);
		if (r) {
			kfree(rd_buf);
			return r; /* r = -EFAULT */
		}

		buf += 1;
		size -= 1;
		*pos += 1;
		result += 1;
	}

	kfree(rd_buf);
	return result;
}

/* function: write force DSC on the connector
 *
 * The write function: dp_dsc_clock_en_write
 * enables to force DSC on the connector.
 * User can write to either force enable or force disable DSC
 * on the next modeset or set it to driver default
 *
 * Accepted inputs:
 * 0 - default DSC enablement policy
 * 1 - force enable DSC on the connector
 * 2 - force disable DSC on the connector (might cause fail in atomic_check)
 *
 * Writing DSC settings is done with the following command:
 * - To force enable DSC (you need to specify
 * connector like DP-1):
 *
 *	echo 0x1 > /sys/kernel/debug/dri/0/DP-X/dsc_clock_en
 *
 * - To return to default state set the flag to zero and
 * let driver deal with DSC automatically
 * (you need to specify connector like DP-1):
 *
 *	echo 0x0 > /sys/kernel/debug/dri/0/DP-X/dsc_clock_en
 *
 */
static ssize_t dp_dsc_clock_en_write(struct file *f, const char __user *buf,
				     size_t size, loff_t *pos)
{
	struct amdgpu_dm_connector *aconnector = file_inode(f)->i_private;
	struct drm_connector *connector = &aconnector->base;
	struct drm_device *dev = connector->dev;
	struct drm_crtc *crtc = NULL;
	struct dm_crtc_state *dm_crtc_state = NULL;
	struct pipe_ctx *pipe_ctx;
	int i;
	char *wr_buf = NULL;
	uint32_t wr_buf_size = 42;
	int max_param_num = 1;
	long param[1] = {0};
	uint8_t param_nums = 0;

	if (size == 0)
		return -EINVAL;

	wr_buf = kcalloc(wr_buf_size, sizeof(char), GFP_KERNEL);

	if (!wr_buf) {
		DRM_DEBUG_DRIVER("no memory to allocate write buffer\n");
		return -ENOSPC;
	}

	if (parse_write_buffer_into_params(wr_buf, size,
					   (long *)param, buf,
					   max_param_num,
					   &param_nums)) {
		kfree(wr_buf);
		return -EINVAL;
	}

	if (param_nums <= 0) {
		DRM_DEBUG_DRIVER("user data not be read\n");
		kfree(wr_buf);
		return -EINVAL;
	}

	for (i = 0; i < MAX_PIPES; i++) {
		pipe_ctx = &aconnector->dc_link->dc->current_state->res_ctx.pipe_ctx[i];
			if (pipe_ctx && pipe_ctx->stream &&
			    pipe_ctx->stream->link == aconnector->dc_link)
				break;
	}

	if (!pipe_ctx || !pipe_ctx->stream)
		goto done;

	// Safely get CRTC state
	mutex_lock(&dev->mode_config.mutex);
	drm_modeset_lock(&dev->mode_config.connection_mutex, NULL);

	if (connector->state == NULL)
		goto unlock;

	crtc = connector->state->crtc;
	if (crtc == NULL)
		goto unlock;

	drm_modeset_lock(&crtc->mutex, NULL);
	if (crtc->state == NULL)
		goto unlock;

	dm_crtc_state = to_dm_crtc_state(crtc->state);
	if (dm_crtc_state->stream == NULL)
		goto unlock;

	if (param[0] == 1)
		aconnector->dsc_settings.dsc_force_enable = DSC_CLK_FORCE_ENABLE;
	else if (param[0] == 2)
		aconnector->dsc_settings.dsc_force_enable = DSC_CLK_FORCE_DISABLE;
	else
		aconnector->dsc_settings.dsc_force_enable = DSC_CLK_FORCE_DEFAULT;

	dm_crtc_state->dsc_force_changed = true;

unlock:
	if (crtc)
		drm_modeset_unlock(&crtc->mutex);
	drm_modeset_unlock(&dev->mode_config.connection_mutex);
	mutex_unlock(&dev->mode_config.mutex);

done:
	kfree(wr_buf);
	return size;
}

/* function: read DSC slice width parameter on the connector
 *
 * The read function: dp_dsc_slice_width_read
 * returns dsc slice width used in the current configuration
 * The return is an integer: 0 or other positive number
 *
 * Access the status with the following command:
 *
 *	cat /sys/kernel/debug/dri/0/DP-X/dsc_slice_width
 *
 * 0 - means that DSC is disabled
 *
 * Any other number more than zero represents the
 * slice width currently used by DSC in pixels
 *
 */
static ssize_t dp_dsc_slice_width_read(struct file *f, char __user *buf,
				    size_t size, loff_t *pos)
{
	char *rd_buf = NULL;
	char *rd_buf_ptr = NULL;
	struct amdgpu_dm_connector *aconnector = file_inode(f)->i_private;
	struct display_stream_compressor *dsc;
	struct dcn_dsc_state dsc_state = {0};
	const uint32_t rd_buf_size = 100;
	struct pipe_ctx *pipe_ctx;
	ssize_t result = 0;
	int i, r, str_len = 30;

	rd_buf = kcalloc(rd_buf_size, sizeof(char), GFP_KERNEL);

	if (!rd_buf)
		return -ENOMEM;

	rd_buf_ptr = rd_buf;

	for (i = 0; i < MAX_PIPES; i++) {
		pipe_ctx = &aconnector->dc_link->dc->current_state->res_ctx.pipe_ctx[i];
			if (pipe_ctx && pipe_ctx->stream &&
			    pipe_ctx->stream->link == aconnector->dc_link)
				break;
	}

	if (!pipe_ctx) {
		kfree(rd_buf);
		return -ENXIO;
	}

	dsc = pipe_ctx->stream_res.dsc;
	if (dsc)
		dsc->funcs->dsc_read_state(dsc, &dsc_state);

	snprintf(rd_buf_ptr, str_len,
		"%d\n",
		dsc_state.dsc_slice_width);
	rd_buf_ptr += str_len;

	while (size) {
		if (*pos >= rd_buf_size)
			break;

		r = put_user(*(rd_buf + result), buf);
		if (r) {
			kfree(rd_buf);
			return r; /* r = -EFAULT */
		}

		buf += 1;
		size -= 1;
		*pos += 1;
		result += 1;
	}

	kfree(rd_buf);
	return result;
}

/* function: write DSC slice width parameter
 *
 * The write function: dp_dsc_slice_width_write
 * overwrites automatically generated DSC configuration
 * of slice width.
 *
 * The user has to write the slice width divisible by the
 * picture width.
 *
 * Also the user has to write width in hexidecimal
 * rather than in decimal.
 *
 * Writing DSC settings is done with the following command:
 * - To force overwrite slice width: (example sets to 1920 pixels)
 *
 *	echo 0x780 > /sys/kernel/debug/dri/0/DP-X/dsc_slice_width
 *
 *  - To stop overwriting and let driver find the optimal size,
 * set the width to zero:
 *
 *	echo 0x0 > /sys/kernel/debug/dri/0/DP-X/dsc_slice_width
 *
 */
static ssize_t dp_dsc_slice_width_write(struct file *f, const char __user *buf,
				     size_t size, loff_t *pos)
{
	struct amdgpu_dm_connector *aconnector = file_inode(f)->i_private;
	struct pipe_ctx *pipe_ctx;
	struct drm_connector *connector = &aconnector->base;
	struct drm_device *dev = connector->dev;
	struct drm_crtc *crtc = NULL;
	struct dm_crtc_state *dm_crtc_state = NULL;
	int i;
	char *wr_buf = NULL;
	uint32_t wr_buf_size = 42;
	int max_param_num = 1;
	long param[1] = {0};
	uint8_t param_nums = 0;

	if (size == 0)
		return -EINVAL;

	wr_buf = kcalloc(wr_buf_size, sizeof(char), GFP_KERNEL);

	if (!wr_buf) {
		DRM_DEBUG_DRIVER("no memory to allocate write buffer\n");
		return -ENOSPC;
	}

	if (parse_write_buffer_into_params(wr_buf, size,
					   (long *)param, buf,
					   max_param_num,
					   &param_nums)) {
		kfree(wr_buf);
		return -EINVAL;
	}

	if (param_nums <= 0) {
		DRM_DEBUG_DRIVER("user data not be read\n");
		kfree(wr_buf);
		return -EINVAL;
	}

	for (i = 0; i < MAX_PIPES; i++) {
		pipe_ctx = &aconnector->dc_link->dc->current_state->res_ctx.pipe_ctx[i];
			if (pipe_ctx && pipe_ctx->stream &&
			    pipe_ctx->stream->link == aconnector->dc_link)
				break;
	}

	if (!pipe_ctx || !pipe_ctx->stream)
		goto done;

	// Safely get CRTC state
	mutex_lock(&dev->mode_config.mutex);
	drm_modeset_lock(&dev->mode_config.connection_mutex, NULL);

	if (connector->state == NULL)
		goto unlock;

	crtc = connector->state->crtc;
	if (crtc == NULL)
		goto unlock;

	drm_modeset_lock(&crtc->mutex, NULL);
	if (crtc->state == NULL)
		goto unlock;

	dm_crtc_state = to_dm_crtc_state(crtc->state);
	if (dm_crtc_state->stream == NULL)
		goto unlock;

	dm_crtc_state = to_dm_crtc_state(connector->state->crtc->state);

	if (param[0] > 0)
		aconnector->dsc_settings.dsc_num_slices_h = DIV_ROUND_UP(
								pipe_ctx->stream->timing.h_addressable,
								param[0]);
	else
		aconnector->dsc_settings.dsc_num_slices_h = 0;

	dm_crtc_state->dsc_force_changed = true;

unlock:
	if (crtc)
		drm_modeset_unlock(&crtc->mutex);
	drm_modeset_unlock(&dev->mode_config.connection_mutex);
	mutex_unlock(&dev->mode_config.mutex);


done:
	kfree(wr_buf);
	return size;
}

/* function: read DSC slice height parameter on the connector
 *
 * The read function: dp_dsc_slice_height_read
 * returns dsc slice height used in the current configuration
 * The return is an integer: 0 or other positive number
 *
 * Access the status with the following command:
 *
 *	cat /sys/kernel/debug/dri/0/DP-X/dsc_slice_height
 *
 * 0 - means that DSC is disabled
 *
 * Any other number more than zero represents the
 * slice height currently used by DSC in pixels
 *
 */
static ssize_t dp_dsc_slice_height_read(struct file *f, char __user *buf,
				    size_t size, loff_t *pos)
{
	char *rd_buf = NULL;
	char *rd_buf_ptr = NULL;
	struct amdgpu_dm_connector *aconnector = file_inode(f)->i_private;
	struct display_stream_compressor *dsc;
	struct dcn_dsc_state dsc_state = {0};
	const uint32_t rd_buf_size = 100;
	struct pipe_ctx *pipe_ctx;
	ssize_t result = 0;
	int i, r, str_len = 30;

	rd_buf = kcalloc(rd_buf_size, sizeof(char), GFP_KERNEL);

	if (!rd_buf)
		return -ENOMEM;

	rd_buf_ptr = rd_buf;

	for (i = 0; i < MAX_PIPES; i++) {
		pipe_ctx = &aconnector->dc_link->dc->current_state->res_ctx.pipe_ctx[i];
			if (pipe_ctx && pipe_ctx->stream &&
			    pipe_ctx->stream->link == aconnector->dc_link)
				break;
	}

	if (!pipe_ctx) {
		kfree(rd_buf);
		return -ENXIO;
	}

	dsc = pipe_ctx->stream_res.dsc;
	if (dsc)
		dsc->funcs->dsc_read_state(dsc, &dsc_state);

	snprintf(rd_buf_ptr, str_len,
		"%d\n",
		dsc_state.dsc_slice_height);
	rd_buf_ptr += str_len;

	while (size) {
		if (*pos >= rd_buf_size)
			break;

		r = put_user(*(rd_buf + result), buf);
		if (r) {
			kfree(rd_buf);
			return r; /* r = -EFAULT */
		}

		buf += 1;
		size -= 1;
		*pos += 1;
		result += 1;
	}

	kfree(rd_buf);
	return result;
}

/* function: write DSC slice height parameter
 *
 * The write function: dp_dsc_slice_height_write
 * overwrites automatically generated DSC configuration
 * of slice height.
 *
 * The user has to write the slice height divisible by the
 * picture height.
 *
 * Also the user has to write height in hexidecimal
 * rather than in decimal.
 *
 * Writing DSC settings is done with the following command:
 * - To force overwrite slice height (example sets to 128 pixels):
 *
 *	echo 0x80 > /sys/kernel/debug/dri/0/DP-X/dsc_slice_height
 *
 *  - To stop overwriting and let driver find the optimal size,
 * set the height to zero:
 *
 *	echo 0x0 > /sys/kernel/debug/dri/0/DP-X/dsc_slice_height
 *
 */
static ssize_t dp_dsc_slice_height_write(struct file *f, const char __user *buf,
				     size_t size, loff_t *pos)
{
	struct amdgpu_dm_connector *aconnector = file_inode(f)->i_private;
	struct drm_connector *connector = &aconnector->base;
	struct drm_device *dev = connector->dev;
	struct drm_crtc *crtc = NULL;
	struct dm_crtc_state *dm_crtc_state = NULL;
	struct pipe_ctx *pipe_ctx;
	int i;
	char *wr_buf = NULL;
	uint32_t wr_buf_size = 42;
	int max_param_num = 1;
	uint8_t param_nums = 0;
	long param[1] = {0};

	if (size == 0)
		return -EINVAL;

	wr_buf = kcalloc(wr_buf_size, sizeof(char), GFP_KERNEL);

	if (!wr_buf) {
		DRM_DEBUG_DRIVER("no memory to allocate write buffer\n");
		return -ENOSPC;
	}

	if (parse_write_buffer_into_params(wr_buf, size,
					   (long *)param, buf,
					   max_param_num,
					   &param_nums)) {
		kfree(wr_buf);
		return -EINVAL;
	}

	if (param_nums <= 0) {
		DRM_DEBUG_DRIVER("user data not be read\n");
		kfree(wr_buf);
		return -EINVAL;
	}

	for (i = 0; i < MAX_PIPES; i++) {
		pipe_ctx = &aconnector->dc_link->dc->current_state->res_ctx.pipe_ctx[i];
			if (pipe_ctx && pipe_ctx->stream &&
			    pipe_ctx->stream->link == aconnector->dc_link)
				break;
	}

	if (!pipe_ctx || !pipe_ctx->stream)
		goto done;

	// Safely get CRTC state
	mutex_lock(&dev->mode_config.mutex);
	drm_modeset_lock(&dev->mode_config.connection_mutex, NULL);

	if (connector->state == NULL)
		goto unlock;

	crtc = connector->state->crtc;
	if (crtc == NULL)
		goto unlock;

	drm_modeset_lock(&crtc->mutex, NULL);
	if (crtc->state == NULL)
		goto unlock;

	dm_crtc_state = to_dm_crtc_state(crtc->state);
	if (dm_crtc_state->stream == NULL)
		goto unlock;

	dm_crtc_state = to_dm_crtc_state(connector->state->crtc->state);

	if (param[0] > 0)
		aconnector->dsc_settings.dsc_num_slices_v = DIV_ROUND_UP(
								pipe_ctx->stream->timing.v_addressable,
								param[0]);
	else
		aconnector->dsc_settings.dsc_num_slices_v = 0;

	dm_crtc_state->dsc_force_changed = true;

unlock:
	if (crtc)
		drm_modeset_unlock(&crtc->mutex);
	drm_modeset_unlock(&dev->mode_config.connection_mutex);
	mutex_unlock(&dev->mode_config.mutex);

done:
	kfree(wr_buf);
	return size;
}

/* function: read DSC target rate on the connector in bits per pixel
 *
 * The read function: dp_dsc_bits_per_pixel_read
 * returns target rate of compression in bits per pixel
 * The return is an integer: 0 or other positive integer
 *
 * Access it with the following command:
 *
 *	cat /sys/kernel/debug/dri/0/DP-X/dsc_bits_per_pixel
 *
 *  0 - means that DSC is disabled
 */
static ssize_t dp_dsc_bits_per_pixel_read(struct file *f, char __user *buf,
				    size_t size, loff_t *pos)
{
	char *rd_buf = NULL;
	char *rd_buf_ptr = NULL;
	struct amdgpu_dm_connector *aconnector = file_inode(f)->i_private;
	struct display_stream_compressor *dsc;
	struct dcn_dsc_state dsc_state = {0};
	const uint32_t rd_buf_size = 100;
	struct pipe_ctx *pipe_ctx;
	ssize_t result = 0;
	int i, r, str_len = 30;

	rd_buf = kcalloc(rd_buf_size, sizeof(char), GFP_KERNEL);

	if (!rd_buf)
		return -ENOMEM;

	rd_buf_ptr = rd_buf;

	for (i = 0; i < MAX_PIPES; i++) {
		pipe_ctx = &aconnector->dc_link->dc->current_state->res_ctx.pipe_ctx[i];
			if (pipe_ctx && pipe_ctx->stream &&
			    pipe_ctx->stream->link == aconnector->dc_link)
				break;
	}

	if (!pipe_ctx) {
		kfree(rd_buf);
		return -ENXIO;
	}

	dsc = pipe_ctx->stream_res.dsc;
	if (dsc)
		dsc->funcs->dsc_read_state(dsc, &dsc_state);

	snprintf(rd_buf_ptr, str_len,
		"%d\n",
		dsc_state.dsc_bits_per_pixel);
	rd_buf_ptr += str_len;

	while (size) {
		if (*pos >= rd_buf_size)
			break;

		r = put_user(*(rd_buf + result), buf);
		if (r) {
			kfree(rd_buf);
			return r; /* r = -EFAULT */
		}

		buf += 1;
		size -= 1;
		*pos += 1;
		result += 1;
	}

	kfree(rd_buf);
	return result;
}


/* function: write DSC target rate in bits per pixel
 *
 * The write function: dp_dsc_bits_per_pixel_write
 * overwrites automatically generated DSC configuration
 * of DSC target bit rate.
 *
 * Also the user has to write bpp in hexidecimal
 * rather than in decimal.
 *
 * Writing DSC settings is done with the following command:
 * - To force overwrite rate (example sets to 256 bpp x 1/16):
 *
 *	echo 0x100 > /sys/kernel/debug/dri/0/DP-X/dsc_bits_per_pixel
 *
 *  - To stop overwriting and let driver find the optimal rate,
 * set the rate to zero:
 *
 *	echo 0x0 > /sys/kernel/debug/dri/0/DP-X/dsc_bits_per_pixel
 *
 */
static ssize_t dp_dsc_bits_per_pixel_write(struct file *f, const char __user *buf,
				     size_t size, loff_t *pos)
{
	struct amdgpu_dm_connector *aconnector = file_inode(f)->i_private;
	struct drm_connector *connector = &aconnector->base;
	struct drm_device *dev = connector->dev;
	struct drm_crtc *crtc = NULL;
	struct dm_crtc_state *dm_crtc_state = NULL;
	struct pipe_ctx *pipe_ctx;
	int i;
	char *wr_buf = NULL;
	uint32_t wr_buf_size = 42;
	int max_param_num = 1;
	long param[1] = {0};
	uint8_t param_nums = 0;

	if (size == 0)
		return -EINVAL;

	wr_buf = kcalloc(wr_buf_size, sizeof(char), GFP_KERNEL);

	if (!wr_buf) {
		DRM_DEBUG_DRIVER("no memory to allocate write buffer\n");
		return -ENOSPC;
	}

	if (parse_write_buffer_into_params(wr_buf, size,
					   (long *)param, buf,
					   max_param_num,
					   &param_nums)) {
		kfree(wr_buf);
		return -EINVAL;
	}

	if (param_nums <= 0) {
		DRM_DEBUG_DRIVER("user data not be read\n");
		kfree(wr_buf);
		return -EINVAL;
	}

	for (i = 0; i < MAX_PIPES; i++) {
		pipe_ctx = &aconnector->dc_link->dc->current_state->res_ctx.pipe_ctx[i];
			if (pipe_ctx && pipe_ctx->stream &&
			    pipe_ctx->stream->link == aconnector->dc_link)
				break;
	}

	if (!pipe_ctx || !pipe_ctx->stream)
		goto done;

	// Safely get CRTC state
	mutex_lock(&dev->mode_config.mutex);
	drm_modeset_lock(&dev->mode_config.connection_mutex, NULL);

	if (connector->state == NULL)
		goto unlock;

	crtc = connector->state->crtc;
	if (crtc == NULL)
		goto unlock;

	drm_modeset_lock(&crtc->mutex, NULL);
	if (crtc->state == NULL)
		goto unlock;

	dm_crtc_state = to_dm_crtc_state(crtc->state);
	if (dm_crtc_state->stream == NULL)
		goto unlock;

	dm_crtc_state = to_dm_crtc_state(connector->state->crtc->state);

	aconnector->dsc_settings.dsc_bits_per_pixel = param[0];

	dm_crtc_state->dsc_force_changed = true;

unlock:
	if (crtc)
		drm_modeset_unlock(&crtc->mutex);
	drm_modeset_unlock(&dev->mode_config.connection_mutex);
	mutex_unlock(&dev->mode_config.mutex);

done:
	kfree(wr_buf);
	return size;
}

/* function: read DSC picture width parameter on the connector
 *
 * The read function: dp_dsc_pic_width_read
 * returns dsc picture width used in the current configuration
 * It is the same as h_addressable of the current
 * display's timing
 * The return is an integer: 0 or other positive integer
 * If 0 then DSC is disabled.
 *
 * Access it with the following command:
 *
 *	cat /sys/kernel/debug/dri/0/DP-X/dsc_pic_width
 *
 * 0 - means that DSC is disabled
 */
static ssize_t dp_dsc_pic_width_read(struct file *f, char __user *buf,
				    size_t size, loff_t *pos)
{
	char *rd_buf = NULL;
	char *rd_buf_ptr = NULL;
	struct amdgpu_dm_connector *aconnector = file_inode(f)->i_private;
	struct display_stream_compressor *dsc;
	struct dcn_dsc_state dsc_state = {0};
	const uint32_t rd_buf_size = 100;
	struct pipe_ctx *pipe_ctx;
	ssize_t result = 0;
	int i, r, str_len = 30;

	rd_buf = kcalloc(rd_buf_size, sizeof(char), GFP_KERNEL);

	if (!rd_buf)
		return -ENOMEM;

	rd_buf_ptr = rd_buf;

	for (i = 0; i < MAX_PIPES; i++) {
		pipe_ctx = &aconnector->dc_link->dc->current_state->res_ctx.pipe_ctx[i];
			if (pipe_ctx && pipe_ctx->stream &&
			    pipe_ctx->stream->link == aconnector->dc_link)
				break;
	}

	if (!pipe_ctx) {
		kfree(rd_buf);
		return -ENXIO;
	}

	dsc = pipe_ctx->stream_res.dsc;
	if (dsc)
		dsc->funcs->dsc_read_state(dsc, &dsc_state);

	snprintf(rd_buf_ptr, str_len,
		"%d\n",
		dsc_state.dsc_pic_width);
	rd_buf_ptr += str_len;

	while (size) {
		if (*pos >= rd_buf_size)
			break;

		r = put_user(*(rd_buf + result), buf);
		if (r) {
			kfree(rd_buf);
			return r; /* r = -EFAULT */
		}

		buf += 1;
		size -= 1;
		*pos += 1;
		result += 1;
	}

	kfree(rd_buf);
	return result;
}

/* function: read DSC picture height parameter on the connector
 *
 * The read function: dp_dsc_pic_height_read
 * returns dsc picture height used in the current configuration
 * It is the same as v_addressable of the current
 * display's timing
 * The return is an integer: 0 or other positive integer
 * If 0 then DSC is disabled.
 *
 * Access it with the following command:
 *
 *	cat /sys/kernel/debug/dri/0/DP-X/dsc_pic_height
 *
 * 0 - means that DSC is disabled
 */
static ssize_t dp_dsc_pic_height_read(struct file *f, char __user *buf,
				    size_t size, loff_t *pos)
{
	char *rd_buf = NULL;
	char *rd_buf_ptr = NULL;
	struct amdgpu_dm_connector *aconnector = file_inode(f)->i_private;
	struct display_stream_compressor *dsc;
	struct dcn_dsc_state dsc_state = {0};
	const uint32_t rd_buf_size = 100;
	struct pipe_ctx *pipe_ctx;
	ssize_t result = 0;
	int i, r, str_len = 30;

	rd_buf = kcalloc(rd_buf_size, sizeof(char), GFP_KERNEL);

	if (!rd_buf)
		return -ENOMEM;

	rd_buf_ptr = rd_buf;

	for (i = 0; i < MAX_PIPES; i++) {
		pipe_ctx = &aconnector->dc_link->dc->current_state->res_ctx.pipe_ctx[i];
			if (pipe_ctx && pipe_ctx->stream &&
			    pipe_ctx->stream->link == aconnector->dc_link)
				break;
	}

	if (!pipe_ctx) {
		kfree(rd_buf);
		return -ENXIO;
	}

	dsc = pipe_ctx->stream_res.dsc;
	if (dsc)
		dsc->funcs->dsc_read_state(dsc, &dsc_state);

	snprintf(rd_buf_ptr, str_len,
		"%d\n",
		dsc_state.dsc_pic_height);
	rd_buf_ptr += str_len;

	while (size) {
		if (*pos >= rd_buf_size)
			break;

		r = put_user(*(rd_buf + result), buf);
		if (r) {
			kfree(rd_buf);
			return r; /* r = -EFAULT */
		}

		buf += 1;
		size -= 1;
		*pos += 1;
		result += 1;
	}

	kfree(rd_buf);
	return result;
}

/* function: read DSC chunk size parameter on the connector
 *
 * The read function: dp_dsc_chunk_size_read
 * returns dsc chunk size set in the current configuration
 * The value is calculated automatically by DSC code
 * and depends on slice parameters and bpp target rate
 * The return is an integer: 0 or other positive integer
 * If 0 then DSC is disabled.
 *
 * Access it with the following command:
 *
 *	cat /sys/kernel/debug/dri/0/DP-X/dsc_chunk_size
 *
 * 0 - means that DSC is disabled
 */
static ssize_t dp_dsc_chunk_size_read(struct file *f, char __user *buf,
				    size_t size, loff_t *pos)
{
	char *rd_buf = NULL;
	char *rd_buf_ptr = NULL;
	struct amdgpu_dm_connector *aconnector = file_inode(f)->i_private;
	struct display_stream_compressor *dsc;
	struct dcn_dsc_state dsc_state = {0};
	const uint32_t rd_buf_size = 100;
	struct pipe_ctx *pipe_ctx;
	ssize_t result = 0;
	int i, r, str_len = 30;

	rd_buf = kcalloc(rd_buf_size, sizeof(char), GFP_KERNEL);

	if (!rd_buf)
		return -ENOMEM;

	rd_buf_ptr = rd_buf;

	for (i = 0; i < MAX_PIPES; i++) {
		pipe_ctx = &aconnector->dc_link->dc->current_state->res_ctx.pipe_ctx[i];
			if (pipe_ctx && pipe_ctx->stream &&
			    pipe_ctx->stream->link == aconnector->dc_link)
				break;
	}

	if (!pipe_ctx) {
		kfree(rd_buf);
		return -ENXIO;
	}

	dsc = pipe_ctx->stream_res.dsc;
	if (dsc)
		dsc->funcs->dsc_read_state(dsc, &dsc_state);

	snprintf(rd_buf_ptr, str_len,
		"%d\n",
		dsc_state.dsc_chunk_size);
	rd_buf_ptr += str_len;

	while (size) {
		if (*pos >= rd_buf_size)
			break;

		r = put_user(*(rd_buf + result), buf);
		if (r) {
			kfree(rd_buf);
			return r; /* r = -EFAULT */
		}

		buf += 1;
		size -= 1;
		*pos += 1;
		result += 1;
	}

	kfree(rd_buf);
	return result;
}

/* function: read DSC slice bpg offset on the connector
 *
 * The read function: dp_dsc_slice_bpg_offset_read
 * returns dsc bpg slice offset set in the current configuration
 * The value is calculated automatically by DSC code
 * and depends on slice parameters and bpp target rate
 * The return is an integer: 0 or other positive integer
 * If 0 then DSC is disabled.
 *
 * Access it with the following command:
 *
 *	cat /sys/kernel/debug/dri/0/DP-X/dsc_slice_bpg_offset
 *
 * 0 - means that DSC is disabled
 */
static ssize_t dp_dsc_slice_bpg_offset_read(struct file *f, char __user *buf,
				    size_t size, loff_t *pos)
{
	char *rd_buf = NULL;
	char *rd_buf_ptr = NULL;
	struct amdgpu_dm_connector *aconnector = file_inode(f)->i_private;
	struct display_stream_compressor *dsc;
	struct dcn_dsc_state dsc_state = {0};
	const uint32_t rd_buf_size = 100;
	struct pipe_ctx *pipe_ctx;
	ssize_t result = 0;
	int i, r, str_len = 30;

	rd_buf = kcalloc(rd_buf_size, sizeof(char), GFP_KERNEL);

	if (!rd_buf)
		return -ENOMEM;

	rd_buf_ptr = rd_buf;

	for (i = 0; i < MAX_PIPES; i++) {
		pipe_ctx = &aconnector->dc_link->dc->current_state->res_ctx.pipe_ctx[i];
			if (pipe_ctx && pipe_ctx->stream &&
			    pipe_ctx->stream->link == aconnector->dc_link)
				break;
	}

	if (!pipe_ctx) {
		kfree(rd_buf);
		return -ENXIO;
	}

	dsc = pipe_ctx->stream_res.dsc;
	if (dsc)
		dsc->funcs->dsc_read_state(dsc, &dsc_state);

	snprintf(rd_buf_ptr, str_len,
		"%d\n",
		dsc_state.dsc_slice_bpg_offset);
	rd_buf_ptr += str_len;

	while (size) {
		if (*pos >= rd_buf_size)
			break;

		r = put_user(*(rd_buf + result), buf);
		if (r) {
			kfree(rd_buf);
			return r; /* r = -EFAULT */
		}

		buf += 1;
		size -= 1;
		*pos += 1;
		result += 1;
	}

	kfree(rd_buf);
	return result;
}

static ssize_t dp_max_bpc_read(struct file *f, char __user *buf,
		size_t size, loff_t *pos)
{
	struct amdgpu_dm_connector *aconnector = file_inode(f)->i_private;
	struct drm_connector *connector = &aconnector->base;
	struct drm_device *dev = connector->dev;
	struct dm_connector_state *state;
	ssize_t result = 0;
	char *rd_buf = NULL;
	char *rd_buf_ptr = NULL;
	const uint32_t rd_buf_size = 10;
	int r;

	rd_buf = kcalloc(rd_buf_size, sizeof(char), GFP_KERNEL);

	if (!rd_buf)
		return -ENOMEM;

	mutex_lock(&dev->mode_config.mutex);
	drm_modeset_lock(&dev->mode_config.connection_mutex, NULL);

	if (connector->state == NULL) {
		goto unlock;
	}

	state = to_dm_connector_state(connector->state);

	rd_buf_ptr = rd_buf;
	snprintf(rd_buf_ptr, rd_buf_size,
		"%u\n",
		state->base.max_requested_bpc);

	while (size) {
		if (*pos >= rd_buf_size)
			break;

		r = put_user(*(rd_buf + result), buf);
		if (r) {
			result = r; /* r = -EFAULT */
			goto unlock;
		}
		buf += 1;
		size -= 1;
		*pos += 1;
		result += 1;
	}
unlock:
	drm_modeset_unlock(&dev->mode_config.connection_mutex);
	mutex_unlock(&dev->mode_config.mutex);
	kfree(rd_buf);
	return result;
}

static ssize_t dp_max_bpc_write(struct file *f, const char __user *buf,
				     size_t size, loff_t *pos)
{
	struct amdgpu_dm_connector *aconnector = file_inode(f)->i_private;
	struct drm_connector *connector = &aconnector->base;
	struct dm_connector_state *state;
	struct drm_device *dev = connector->dev;
	char *wr_buf = NULL;
	uint32_t wr_buf_size = 42;
	int max_param_num = 1;
	long param[1] = {0};
	uint8_t param_nums = 0;

	if (size == 0)
		return -EINVAL;

	wr_buf = kcalloc(wr_buf_size, sizeof(char), GFP_KERNEL);

	if (!wr_buf) {
		DRM_DEBUG_DRIVER("no memory to allocate write buffer\n");
		return -ENOSPC;
	}

	if (parse_write_buffer_into_params(wr_buf, size,
					   (long *)param, buf,
					   max_param_num,
					   &param_nums)) {
		kfree(wr_buf);
		return -EINVAL;
	}

	if (param_nums <= 0) {
		DRM_DEBUG_DRIVER("user data not be read\n");
		kfree(wr_buf);
		return -EINVAL;
	}

	if (param[0] < 6 || param[0] > 16) {
		DRM_DEBUG_DRIVER("bad max_bpc value\n");
		kfree(wr_buf);
		return -EINVAL;
	}

	mutex_lock(&dev->mode_config.mutex);
	drm_modeset_lock(&dev->mode_config.connection_mutex, NULL);

	if (connector->state == NULL) {
		goto unlock;
	}

	state = to_dm_connector_state(connector->state);
	state->base.max_requested_bpc = param[0];
unlock:
	drm_modeset_unlock(&dev->mode_config.connection_mutex);
	mutex_unlock(&dev->mode_config.mutex);

	kfree(wr_buf);
	return size;
}

/* function: read Packed Display Transport information
 *	number_of_active_lines
 *	number_of_offset_lines
 *	dynamic framebuffer metadata
 *
 * Access it with the following command:
 *	cat /sys/kernel/debug/dri/0/DP-X/pdt_info
 *
 */
static int pdt_info_show(struct seq_file *m, void *data)
{
	struct drm_connector *connector = m->private;
	struct drm_device *dev = connector->dev;
	struct drm_crtc *crtc = NULL;
	struct dm_crtc_state *dm_crtc_state = NULL;
	struct dm_connector_state *dm_connector_state = NULL;
	int res = 0;

	mutex_lock(&dev->mode_config.mutex);
	drm_modeset_lock(&dev->mode_config.connection_mutex, NULL);

	if (connector->state == NULL)
		goto unlock;

	dm_connector_state = to_dm_connector_state(connector->state);

	crtc = connector->state->crtc;
	if (crtc == NULL)
		goto unlock;

	drm_modeset_lock(&crtc->mutex, NULL);
	if (crtc->state == NULL)
		goto unlock;

	dm_crtc_state = to_dm_crtc_state(crtc->state);
	if (dm_crtc_state->stream == NULL)
		goto unlock;

	seq_printf(m, "num_active_lines %d\n", dm_crtc_state->num_active_lines);
	seq_printf(m, "num_offset_lines %d\n", dm_crtc_state->num_offset_lines);
	seq_printf(m, "dmdata_fb_id %u\n", dm_connector_state->dmdata_fb_id);
	if (dm_connector_state->dmdatafb != NULL) {
		seq_printf(m, "dmdata_fb->width %u\n", dm_connector_state->dmdatafb->width);
		seq_printf(m, "dmdata_fb->height %u\n", dm_connector_state->dmdatafb->height);
	}

unlock:
	if (crtc)
		drm_modeset_unlock(&crtc->mutex);

	drm_modeset_unlock(&dev->mode_config.connection_mutex);
	mutex_unlock(&dev->mode_config.mutex);

	return res;
}


#ifdef DEFINE_SHOW_ATTRIBUTE
DEFINE_SHOW_ATTRIBUTE(dmub_tracebuffer);
DEFINE_SHOW_ATTRIBUTE(output_bpc);
DEFINE_SHOW_ATTRIBUTE(vrr_range);
DEFINE_SHOW_ATTRIBUTE(darkmode_on);
#ifdef CONFIG_DRM_AMD_DC_HDCP
DEFINE_SHOW_ATTRIBUTE(hdcp_sink_capability);
#endif
#endif
DEFINE_SHOW_ATTRIBUTE(dp_dsc_fec_support);
DEFINE_SHOW_ATTRIBUTE(pdt_info);

static const struct file_operations dp_dsc_clock_en_debugfs_fops = {
	.owner = THIS_MODULE,
	.read = dp_dsc_clock_en_read,
	.write = dp_dsc_clock_en_write,
	.llseek = default_llseek
};

static const struct file_operations dp_dsc_slice_width_debugfs_fops = {
	.owner = THIS_MODULE,
	.read = dp_dsc_slice_width_read,
	.write = dp_dsc_slice_width_write,
	.llseek = default_llseek
};

static const struct file_operations dp_dsc_slice_height_debugfs_fops = {
	.owner = THIS_MODULE,
	.read = dp_dsc_slice_height_read,
	.write = dp_dsc_slice_height_write,
	.llseek = default_llseek
};

static const struct file_operations dp_dsc_bits_per_pixel_debugfs_fops = {
	.owner = THIS_MODULE,
	.read = dp_dsc_bits_per_pixel_read,
	.write = dp_dsc_bits_per_pixel_write,
	.llseek = default_llseek
};

static const struct file_operations dp_dsc_pic_width_debugfs_fops = {
	.owner = THIS_MODULE,
	.read = dp_dsc_pic_width_read,
	.llseek = default_llseek
};

static const struct file_operations dp_dsc_pic_height_debugfs_fops = {
	.owner = THIS_MODULE,
	.read = dp_dsc_pic_height_read,
	.llseek = default_llseek
};

static const struct file_operations dp_dsc_chunk_size_debugfs_fops = {
	.owner = THIS_MODULE,
	.read = dp_dsc_chunk_size_read,
	.llseek = default_llseek
};

static const struct file_operations dp_dsc_slice_bpg_offset_debugfs_fops = {
	.owner = THIS_MODULE,
	.read = dp_dsc_slice_bpg_offset_read,
	.llseek = default_llseek
};
static const struct file_operations dp_link_settings_debugfs_fops = {
	.owner = THIS_MODULE,
	.read = dp_link_settings_read,
	.write = dp_link_settings_write,
	.llseek = default_llseek
};

static const struct file_operations dp_link_active_debugfs_fops = {
	.owner = THIS_MODULE,
	.read = dp_link_active_read,
	.llseek = default_llseek
};

static const struct file_operations dp_phy_settings_debugfs_fop = {
	.owner = THIS_MODULE,
	.read = dp_phy_settings_read,
	.write = dp_phy_settings_write,
	.llseek = default_llseek
};

static const struct file_operations dp_phy_test_pattern_fops = {
	.owner = THIS_MODULE,
	.read = dp_phy_test_pattern_debugfs_read,
	.write = dp_phy_test_pattern_debugfs_write,
	.llseek = default_llseek
};

static const struct file_operations optc_underflow_fops = {
	.owner = THIS_MODULE,
	.read = optc_underflow_read,
	.write = optc_underflow_write,
	.llseek = default_llseek
};

static const struct file_operations sdp_message_fops = {
	.owner = THIS_MODULE,
	.write = dp_sdp_message_debugfs_write,
	.llseek = default_llseek
};

static const struct file_operations dp_dpcd_address_debugfs_fops = {
	.owner = THIS_MODULE,
	.write = dp_dpcd_address_write,
	.llseek = default_llseek
};

static const struct file_operations dp_dpcd_size_debugfs_fops = {
	.owner = THIS_MODULE,
	.write = dp_dpcd_size_write,
	.llseek = default_llseek
};

static const struct file_operations dp_force_ignore_msa_bit_debugfs_fops = {
	.owner = THIS_MODULE,
	.read = dp_force_ignore_msa_bit_read,
	.write = dp_force_ignore_msa_bit_write,
	.llseek = default_llseek
};

static const struct file_operations dp_dpcd_data_debugfs_fops = {
	.owner = THIS_MODULE,
	.read = dp_dpcd_data_read,
	.write = dp_dpcd_data_write,
	.llseek = default_llseek
};

static const struct file_operations dp_trigger_hotplug_debugfs_fops = {
	.owner = THIS_MODULE,
	.write = dp_trigger_hotplug,
	.llseek = default_llseek
};

static const struct file_operations dp_max_bpc_debugfs_fops = {
	.owner = THIS_MODULE,
	.read = dp_max_bpc_read,
	.write = dp_max_bpc_write,
	.llseek = default_llseek
};

/**
 * Single param debugfs interface to manipulate DMUB phy (link) status toggle count
 *
 * get command to interact with DMUB FW to get multiplexed phy toggle count
 * MSB 16 bits: phy toggle up count (link wakeup)
 * LSB 16 bits: phy toggle down count (link shutdown)
 *
 * cat /sys/kernel/debug/dri/0/DP-x/phy_toggle_count
 *
 *
 * set command to interact with DMUB FW
 * valid input args #: 1
 * valid input args:
 * 0 = inform DMUB FW to start counting the PHY status toggle
 * 1 = inform DMUB FW to stop counting the PHY status toggle
 * 2 = inform DMUB FW to reset the count of the PHY status toggle
 * 3 = inform DMUB FW to save the count of the PHY status toggle to SCRATCH11 reg
 *
 * debugfs interface 'phy_toggle_count' is located at /sys/kernel/debug/dri/0/DP-x
 *
 * Example:
 * tell DMUB FW to start PHY status toggle counting
 *
 * echo 0 > /sys/kernel/debug/dri/0/DP-x/phy_toggle_count
 */
static int dp_phy_toggle_count_get(void *data, u64 *val)
{
	struct amdgpu_dm_connector *aconn = (struct amdgpu_dm_connector *) data;
	struct dc_link *link = aconn->dc_link;
	struct dc *dc = (struct dc *) link->dc;
	struct drm_device *dev = aconn->base.dev;
	struct amdgpu_device *adev = dev->dev_private;
	struct dmub_dkmd *dkmd = NULL;
	uint16_t phy_toggle_up_ct = 0;
	uint16_t phy_toggle_down_ct = 0;

	*val = 0;

	/* check dc and dmub pointers valid */
	if (!dc) {
		pr_debug("[DMUB_DKMD] Null dc pointer!, line %d\n", __LINE__);
		goto tail;
	}

	dkmd = dc->res_pool->dkmd;
	if (!dkmd) {
		pr_debug("[DMUB_DKMD] dmub dkmd instance not existed!, line %d\n", __LINE__);
		goto tail;
	}

	/* require DMUB ver no later than ver. 3.0.B */
	if (adev->dm.dmcub_fw_version < 0x0300000B) {
		pr_debug("[DMUB_DKMD] dmub NOT support phy toggle count! ver. 0x%08X\n", adev->dm.dmcub_fw_version);
		goto tail;
	}

	/* send command to DMUB to save phy toggle count to SCRATCH11 and read back */
	if (!dmub_dkmd_read_phy_toggle_count(dkmd, &phy_toggle_up_ct, &phy_toggle_down_ct))
		goto tail;

	/* multiplex into 4-bytes output */
	*val = (((u64) phy_toggle_up_ct) << 16) + phy_toggle_down_ct;
tail:
	return 0;
}

static int dp_phy_toggle_count_set(void *data, u64 val)
{
	struct amdgpu_dm_connector *aconn = (struct amdgpu_dm_connector *) data;
	struct dc_link *link = aconn->dc_link;
	struct dc *dc = (struct dc *) link->dc;
	struct drm_device *dev = aconn->base.dev;
	struct amdgpu_device *adev = dev->dev_private;
	struct dmub_dkmd *dkmd = NULL;
	enum dmub_phy_toggle_count_cmd {
		START_PHY_TOGGLE_COUNT = 0,
		STOP_PHY_TOGGLE_COUNT,
		RESET_PHY_TOGGLE_COUNT,
		GET_PHY_TOGGLE_COUNT
	};

	if (!dc) {
		pr_debug("[DMUB_DKMD] Null dc pointer!, line %d\n", __LINE__);
		goto tail;
	}

	dkmd = dc->res_pool->dkmd;
	if (!dkmd) {
		pr_debug("[DMUB_DKMD] dmub dkmd instance not existed!, line %d\n", __LINE__);
		goto tail;
	}

	/* require DMUB ver no later than ver. 3.0.B */
	if (adev->dm.dmcub_fw_version < 0x0300000B) {
		pr_debug("[DMUB_DKMD] dmub NOT support phy toggle count! ver. 0x%08X\n", adev->dm.dmcub_fw_version);
		goto tail;
	}

	/* hooks should be defined */
	if (!dkmd->funcs->dkmd_start_phy_toggle_ct || !dkmd->funcs->dkmd_stop_phy_toggle_ct ||
	    !dkmd->funcs->dkmd_reset_phy_toggle_ct || !dkmd->funcs->dkmd_get_phy_toggle_ct) {
		pr_debug("[DMUB_DKMD] phy toggle count hook func NOT defined!\n");
		goto tail;
	}

	if (val > GET_PHY_TOGGLE_COUNT) {
		pr_debug("[DMUB_DKMD] debugfs input arg INVALID: %d (should be ranged betwen 0 to 3)\n", val);
		goto tail;
	}

	/* send command to DMUB */
	switch(val) {
	case START_PHY_TOGGLE_COUNT:
		dkmd->funcs->dkmd_start_phy_toggle_ct(dkmd);
		break;
	case STOP_PHY_TOGGLE_COUNT:
		dkmd->funcs->dkmd_stop_phy_toggle_ct(dkmd);
		break;
	case RESET_PHY_TOGGLE_COUNT:
		dkmd->funcs->dkmd_reset_phy_toggle_ct(dkmd);
		break;
	case GET_PHY_TOGGLE_COUNT:
		dkmd->funcs->dkmd_get_phy_toggle_ct(dkmd);
		break;
	}
tail:
	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(dp_phy_toggle_count_fops,
	dp_phy_toggle_count_get, dp_phy_toggle_count_set, "%llu\n");

static int dp_dkmd_enable_get(void *data, u64 *val)
{
	struct amdgpu_dm_connector *aconn = (struct amdgpu_dm_connector *) data;
	struct dc_link *link = aconn->dc_link;
	struct dc *dc = (struct dc *) link->dc;
	struct drm_device *dev = aconn->base.dev;
	struct amdgpu_device *adev = dev->dev_private;
	struct dmub_dkmd *dkmd = NULL;
	struct dmub_srv *dmub;
	enum dmub_dkmd_enable {
		DKMD_DISABLE = 0,
		DKMD_FULL_ENABLE  = 1,
		DKMD_VBLANK_SHUTDOWN = 2,
	};

	if (!dc) {
		pr_debug("[DMUB_DKMD] Null dc pointer!, line %d\n", __LINE__);
		goto tail;
	}

	dkmd = dc->res_pool->dkmd;
	if (!dkmd) {
		pr_debug("[DMUB_DKMD] dmub dkmd instance not existed!, line %d\n", __LINE__);
		goto tail;
	}

	/* require DMUB ver no later than ver. 3.0.B */
	if (adev->dm.dmcub_fw_version < 0x0300000B) {
		pr_debug("[DMUB_DKMD] dmub NOT support phy toggle count! ver. 0x%08X\n", adev->dm.dmcub_fw_version);
		goto tail;
	}

	dmub = dkmd->ctx->dmub_srv->dmub;
	dkmd->funcs->dkmd_get_status(dkmd);
	udelay(10);
	*val = (u64)dmub->hw_funcs.get_scratch9(dmub);
	/* scratch9 bitfields:
	* bit 0-1: dark mode current mode
	* bit 2-3: dark mode request mode
	* bit 4-7: dark mode status
	*/
	*val = *val & 0x03;

	switch (*val) {
	case DKMD_DISABLE:
		printk(KERN_INFO, "[DMUB_DKMD]: DARK MODE state is DISABLE");
		break;
	case DKMD_FULL_ENABLE:
		printk(KERN_INFO, "[DMUB_DKMD]: FULL DARK MODE state is ENABLE");
		break;
	case DKMD_VBLANK_SHUTDOWN:
		printk(KERN_INFO, "[DMUB_DKMD]: VBLANK_SHUTDOWN DARK MODE state is ENABLE");
		break;
	}
tail:
	return 0;
}

static int dp_dkmd_enable_set(void *data, u64 val)
{
	struct amdgpu_dm_connector *aconn = (struct amdgpu_dm_connector *) data;
	struct dc_link *link = aconn->dc_link;
	struct dc *dc = (struct dc *) link->dc;
	struct drm_device *dev = aconn->base.dev;
	struct amdgpu_device *adev = dev->dev_private;
	struct dmub_dkmd *dkmd = NULL;
	enum dmub_dkmd_enable {
		DKMD_DISABLE = 0,
		DKMD_FULL_ENABLE  = 1,
		DKMD_VBLANK_SHUTDOWN = 2,
	};

	if (!dc) {
		pr_debug("[DMUB_DKMD] Null dc pointer!, line %d\n", __LINE__);
		goto tail;
	}

	dkmd = dc->res_pool->dkmd;
	if (!dkmd) {
		pr_debug("[DMUB_DKMD] dmub dkmd instance not existed!, line %d\n", __LINE__);
		goto tail;
	}

	/* require DMUB ver no later than ver. 3.0.B */
	if (adev->dm.dmcub_fw_version < 0x0300000B) {
		pr_debug("[DMUB_DKMD] dmub NOT support phy toggle count! ver. 0x%08X\n", adev->dm.dmcub_fw_version);
		goto tail;
	}

	/*
	 *	send DARK MODE Enable/VBLANK_SHUTDOWN/Disable
	 *	command to DMUB firmware
	 */
	switch (val) {
	case DKMD_DISABLE:
		dkmd->funcs->dkmd_request(dkmd, DMUB_DKMD_MODE_DISABLE);
		break;
	case DKMD_FULL_ENABLE:
		dkmd->funcs->dkmd_request(dkmd, DMUB_DKMD_MODE_FULL);
		break;
	case DKMD_VBLANK_SHUTDOWN:
		dkmd->funcs->dkmd_request(dkmd, DMUB_DKMD_MODE_VBLANK_ONLY);
		break;
	}

tail:
	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(dp_dkmd_enable_fops,
	dp_dkmd_enable_get, dp_dkmd_enable_set, "%llu\n");

static const struct {
	char *name;
	const struct file_operations *fops;
} dp_debugfs_entries[] = {
		{"trigger_hotplug", &dp_trigger_hotplug_debugfs_fops},
		{"link_settings", &dp_link_settings_debugfs_fops},
		{"phy_settings", &dp_phy_settings_debugfs_fop},
		{"test_pattern", &dp_phy_test_pattern_fops},
		{"output_bpc", &output_bpc_fops},
		{"vrr_range", &vrr_range_fops},
		{"darkmode_on", &darkmode_on_fops},
#ifdef CONFIG_DRM_AMD_DC_HDCP
		{"hdcp_sink_capability", &hdcp_sink_capability_fops},
#endif
		{"sdp_message", &sdp_message_fops},
		{"aux_dpcd_address", &dp_dpcd_address_debugfs_fops},
		{"aux_dpcd_size", &dp_dpcd_size_debugfs_fops},
		{"dp_force_ignore_msa_bit", &dp_force_ignore_msa_bit_debugfs_fops},
		{"aux_dpcd_data", &dp_dpcd_data_debugfs_fops},
		{"dsc_clock_en", &dp_dsc_clock_en_debugfs_fops},
		{"dsc_slice_width", &dp_dsc_slice_width_debugfs_fops},
		{"dsc_slice_height", &dp_dsc_slice_height_debugfs_fops},
		{"dsc_bits_per_pixel", &dp_dsc_bits_per_pixel_debugfs_fops},
		{"dsc_pic_width", &dp_dsc_pic_width_debugfs_fops},
		{"dsc_pic_height", &dp_dsc_pic_height_debugfs_fops},
		{"dsc_chunk_size", &dp_dsc_chunk_size_debugfs_fops},
		{"dsc_slice_bpg", &dp_dsc_slice_bpg_offset_debugfs_fops},
		{"dp_dsc_fec_support", &dp_dsc_fec_support_fops},
		{"link_active", &dp_link_active_debugfs_fops},
		{"max_bpc", &dp_max_bpc_debugfs_fops},
		{"optc_underflow", &optc_underflow_fops},
		{"phy_toggle_count", &dp_phy_toggle_count_fops},
		{"pdt_info", &pdt_info_fops},
		{"dp_dkmd_enable", &dp_dkmd_enable_fops}
};

#ifdef CONFIG_DRM_AMD_DC_HDCP
static const struct {
	char *name;
	const struct file_operations *fops;
} hdmi_debugfs_entries[] = {
		{"hdcp_sink_capability", &hdcp_sink_capability_fops}
};
#endif

/*
 * Force YUV420 output if available from the given mode
 */
static int force_yuv420_output_set(void *data, u64 val)
{
	struct amdgpu_dm_connector *connector = data;

	connector->force_yuv420_output = (bool)val;

	return 0;
}

/*
 * Check if YUV420 is forced when available from the given mode
 */
static int force_yuv420_output_get(void *data, u64 *val)
{
	struct amdgpu_dm_connector *connector = data;

	*val = connector->force_yuv420_output;

	return 0;
}

#ifdef DEFINE_DEBUGFS_ATTRIBUTE
DEFINE_DEBUGFS_ATTRIBUTE(force_yuv420_output_fops, force_yuv420_output_get,
			 force_yuv420_output_set, "%llu\n");
#endif

/*
 *  Read PSR state
 */
static int psr_get(void *data, u64 *val)
{
	struct amdgpu_dm_connector *connector = data;
	struct dc_link *link = connector->dc_link;
	uint32_t psr_state = 0;

	dc_link_get_psr_state(link, &psr_state);

	*val = psr_state;

	return 0;
}


#ifdef DEFINE_DEBUGFS_ATTRIBUTE
DEFINE_DEBUGFS_ATTRIBUTE(psr_fops, psr_get, NULL, "%llu\n");
#endif

void connector_debugfs_init(struct amdgpu_dm_connector *connector)
{
	int i;
	struct dentry *dir = connector->base.debugfs_entry;

	if (connector->base.connector_type == DRM_MODE_CONNECTOR_DisplayPort ||
	    connector->base.connector_type == DRM_MODE_CONNECTOR_eDP) {
		for (i = 0; i < ARRAY_SIZE(dp_debugfs_entries); i++) {
			debugfs_create_file(dp_debugfs_entries[i].name,
					    0644, dir, connector,
					    dp_debugfs_entries[i].fops);
		}
	}

#ifdef DEFINE_DEBUGFS_ATTRIBUTE
	if (connector->base.connector_type == DRM_MODE_CONNECTOR_eDP)
		debugfs_create_file_unsafe("psr_state", 0444, dir, connector, &psr_fops);
#endif

#ifdef DEFINE_DEBUGFS_ATTRIBUTE
	debugfs_create_file_unsafe("force_yuv420_output", 0644, dir, connector,
				   &force_yuv420_output_fops);
#endif

#ifdef CONFIG_DRM_AMD_DC_HDCP
	if (connector->base.connector_type == DRM_MODE_CONNECTOR_HDMIA) {
		for (i = 0; i < ARRAY_SIZE(hdmi_debugfs_entries); i++) {
			debugfs_create_file(hdmi_debugfs_entries[i].name,
					    0644, dir, connector,
					    hdmi_debugfs_entries[i].fops);
		}
	}
#endif
}

/*
 * Writes DTN log state to the user supplied buffer.
 * Example usage: cat /sys/kernel/debug/dri/0/amdgpu_dm_dtn_log
 */
static ssize_t dtn_log_read(
	struct file *f,
	char __user *buf,
	size_t size,
	loff_t *pos)
{
	struct amdgpu_device *adev = file_inode(f)->i_private;
	struct dc *dc = adev->dm.dc;
	struct dc_log_buffer_ctx log_ctx = { 0 };
	ssize_t result = 0;

	if (!buf || !size)
		return -EINVAL;

	if (!dc->hwss.log_hw_state)
		return 0;

	dc->hwss.log_hw_state(dc, &log_ctx);

	if (*pos < log_ctx.pos) {
		size_t to_copy = log_ctx.pos - *pos;

		to_copy = min(to_copy, size);

		if (!copy_to_user(buf, log_ctx.buf + *pos, to_copy)) {
			*pos += to_copy;
			result = to_copy;
		}
	}

	kfree(log_ctx.buf);

	return result;
}

/*
 * Writes DTN log state to dmesg when triggered via a write.
 * Example usage: echo 1 > /sys/kernel/debug/dri/0/amdgpu_dm_dtn_log
 */
static ssize_t dtn_log_write(
	struct file *f,
	const char __user *buf,
	size_t size,
	loff_t *pos)
{
	struct amdgpu_device *adev = file_inode(f)->i_private;
	struct dc *dc = adev->dm.dc;

	/* Write triggers log output via dmesg. */
	if (size == 0)
		return 0;

	if (dc->hwss.log_hw_state)
		dc->hwss.log_hw_state(dc, NULL);

	return size;
}

/*
 * Sets the force_timing_sync debug option from the given string.
 * All connected displays will be force synchronized immediately.
 * Usage: echo 1 0 1 > /sys/kernel/debug/dri/0/amdgpu_dm_force_timing_sync
 */
static ssize_t force_timing_sync_set(
	struct file *f,
	const char __user *buf,
	size_t size,
	loff_t *pos)
{
	struct amdgpu_device *adev = file_inode(f)->i_private;
	struct dc *dc = adev->dm.dc;
	char *wr_buf = NULL;
	const uint32_t wr_buf_size = 40;
	/* 0: lane_count; 1: link_rate */
	long param[3];
	uint8_t param_nums = 0;
	int max_param_num = 3;

	if (size == 0)
		return 0;

	wr_buf = kcalloc(wr_buf_size, sizeof(char), GFP_KERNEL);
	if (!wr_buf)
		return -ENOSPC;

	if (parse_write_buffer_into_params(wr_buf, size,
					   (long *)param, buf,
					   max_param_num,
					   &param_nums)) {
		kfree(wr_buf);
		return -EINVAL;
	}

	if (param_nums <= 0) {
		kfree(wr_buf);
		DRM_DEBUG_DRIVER("user data not read\n");
		return -EINVAL;
	}

	if (param_nums > 2)
		dc->debug.mdvsync_settings.event = (bool)param[2];
	if (param_nums > 1)
		dc->debug.mdvsync_settings.delay = (bool)param[1];

	adev->dm.force_timing_sync = (bool)param[0];

	amdgpu_dm_trigger_timing_sync(adev->ddev);

	kfree(wr_buf);
	return size;
}


/*
 * Gets the force_timing_sync debug option value into the given buffer.
 * Usage: cat /sys/kernel/debug/dri/0/amdgpu_dm_force_timing_sync
 */
static ssize_t force_timing_sync_get(
	struct file *f,
	char __user *buf,
	size_t size,
	loff_t *pos)
{
	int r, i;
	struct amdgpu_device *adev = file_inode(f)->i_private;
	struct dc *dc = adev->dm.dc;
	const uint32_t rd_buf_size = 32;
	int num_of_params = 3;
	int offset = 0;
	char *rd_buf = NULL;
	uint32_t result = 0;
	int data[3] = {0};

	struct dc_stream_state *stream;
	struct dc_state *context = dc->current_state;

	if (context->stream_count >= 2)
		for (i = 0; i < context->stream_count; i++) {
			stream = context->streams[i];

			if (!stream)
				continue;

			if (stream->triggered_crtc_reset.enabled) {
				data[0] = true;
				data[1] = stream->triggered_crtc_reset.delay;
				data[2] = stream->triggered_crtc_reset.event;
				break;
			}
		}

	rd_buf = kcalloc(rd_buf_size, sizeof(char), GFP_KERNEL);
	if (!rd_buf)
		return -ENOMEM;

	for (i = 0; i < num_of_params; i++)
		offset += snprintf(rd_buf + offset, rd_buf_size - offset,
				   "%d  ", data[i]);
	rd_buf[strlen(rd_buf)] = '\n';

	while (size) {
		if (*pos >= rd_buf_size)
			break;
		r = put_user(*(rd_buf + result), buf);
		if (r) {
			kfree(rd_buf);
			return r; /* r = -EFAULT */
		}
		buf += 1;
		size -= 1;
		*pos += 1;
		result += 1;
	}

	kfree(rd_buf);

	return result;
}

/*
 * Backlight at this moment.  Read only.
 * As written to display, taking ABM and backlight lut into account.
 * Ranges from 0x0 to 0x10000 (= 100% PWM)
 */
static int current_backlight_read(struct seq_file *m, void *data)
{
	struct drm_info_node *node = (struct drm_info_node *)m->private;
	struct drm_device *dev = node->minor->dev;
	struct amdgpu_device *adev = dev->dev_private;
	struct dc *dc = adev->dm.dc;
	unsigned int backlight = dc_get_current_backlight_pwm(dc);

	seq_printf(m, "0x%x\n", backlight);
	return 0;
}

/*
 * Backlight value that is being approached.  Read only.
 * As written to display, taking ABM and backlight lut into account.
 * Ranges from 0x0 to 0x10000 (= 100% PWM)
 */
static int target_backlight_read(struct seq_file *m, void *data)
{
	struct drm_info_node *node = (struct drm_info_node *)m->private;
	struct drm_device *dev = node->minor->dev;
	struct amdgpu_device *adev = dev->dev_private;
	struct dc *dc = adev->dm.dc;
	unsigned int backlight = dc_get_target_backlight_pwm(dc);

	seq_printf(m, "0x%x\n", backlight);
	return 0;
}

static int mst_topo(struct seq_file *m, void *unused)
{
	struct drm_info_node *node = (struct drm_info_node *)m->private;
	struct drm_device *dev = node->minor->dev;
	struct drm_connector *connector;
#ifdef HAVE_DRM_CONNECTOR_LIST_ITER_BEGIN
	struct drm_connector_list_iter conn_iter;
#endif
	struct amdgpu_dm_connector *aconnector;

#ifdef HAVE_DRM_CONNECTOR_LIST_ITER_BEGIN
	drm_connector_list_iter_begin(dev, &conn_iter);
	drm_for_each_connector_iter(connector, &conn_iter) {
#else
	drm_for_each_connector(connector, dev) {
#endif
		if (connector->connector_type != DRM_MODE_CONNECTOR_DisplayPort)
			continue;

		aconnector = to_amdgpu_dm_connector(connector);

		seq_printf(m, "\nMST topology for connector %d\n", aconnector->connector_id);
		drm_dp_mst_dump_topology(m, &aconnector->mst_mgr);
	}
#ifdef HAVE_DRM_CONNECTOR_LIST_ITER_BEGIN
	drm_connector_list_iter_end(&conn_iter);
#endif

	return 0;
}

static const struct drm_info_list amdgpu_dm_debugfs_list[] = {
	{"amdgpu_current_backlight_pwm", &current_backlight_read},
	{"amdgpu_target_backlight_pwm", &target_backlight_read},
	{"amdgpu_mst_topology", &mst_topo},
};

/*
 * Sets trigger hpd for MST topologies.
 * All connected connectors will be rediscovered and re started as needed if val of 1 is sent.
 * All topologies will be disconnected if val of 0 is set .
 * Usage to enable topologies: echo 1 > /sys/kernel/debug/dri/0/amdgpu_dm_trigger_hpd_mst
 * Usage to disable topologies: echo 0 > /sys/kernel/debug/dri/0/amdgpu_dm_trigger_hpd_mst
 */
static int trigger_hpd_mst_set(void *data, u64 val)
{
	struct amdgpu_device *adev = data;
	struct drm_device *dev = adev->ddev;
	struct drm_connector_list_iter iter;
	struct amdgpu_dm_connector *aconnector;
	struct drm_connector *connector;
	struct dc_link *link = NULL;

	if (val == 1) {
		drm_connector_list_iter_begin(dev, &iter);
		drm_for_each_connector_iter(connector, &iter) {
			aconnector = to_amdgpu_dm_connector(connector);
			if (aconnector->dc_link->type == dc_connection_mst_branch &&
			    aconnector->mst_mgr.aux) {
				dc_link_detect(aconnector->dc_link, DETECT_REASON_HPD);
				drm_dp_mst_topology_mgr_set_mst(&aconnector->mst_mgr, true);
			}
		}
	} else if (val == 0) {
		drm_connector_list_iter_begin(dev, &iter);
		drm_for_each_connector_iter(connector, &iter) {
			aconnector = to_amdgpu_dm_connector(connector);
			if (!aconnector->dc_link)
				continue;

			if (!(aconnector->port && (bool)(&aconnector->mst_port->mst_mgr)))
				continue;

			link = aconnector->dc_link;

			dp_receiver_power_ctrl(link, false);
			drm_dp_mst_topology_mgr_set_mst(&aconnector->mst_port->mst_mgr, false);
			link->mst_stream_alloc_table.stream_count = 0;
			memset(link->mst_stream_alloc_table.stream_allocations, 0, sizeof(link->mst_stream_alloc_table.stream_allocations));
		}
	} else {
		return 0;
	}
	drm_kms_helper_hotplug_event(dev);

	return 0;
}

/*
 * The interface doesn't need get function, so it will return the
 * value of zero
 * Usage: cat /sys/kernel/debug/dri/0/amdgpu_dm_trigger_hpd_mst
 */
static int trigger_hpd_mst_get(void *data, u64 *val)
{
	*val = 0;
	return 0;
}

DEFINE_DEBUGFS_ATTRIBUTE(trigger_hpd_mst_ops, trigger_hpd_mst_get,
			 trigger_hpd_mst_set, "%llu\n");

/*
 * Set dmcub trace event IRQ enable or disable.
 * Usage to enable dmcub trace event IRQ: echo 1 > /sys/kernel/debug/dri/0/amdgpu_dm_dmcub_trace_event_en
 * Usage to disable dmcub trace event IRQ: echo 0 > /sys/kernel/debug/dri/0/amdgpu_dm_dmcub_trace_event_en
 */
static int dmcub_trace_event_state_set(void *data, u64 val)
{
	struct amdgpu_device *adev = data;

	if (val == 1 || val == 0) {
		dc_dmub_trace_event_control(adev->dm.dc, val);
		adev->dm.dmcub_trace_event_en = (bool)val;
	} else
		return 0;

	return 0;
}

/*
 * The interface doesn't need get function, so it will return the
 * value of zero
 * Usage: cat /sys/kernel/debug/dri/0/amdgpu_dm_dmcub_trace_event_en
 */
static int dmcub_trace_event_state_get(void *data, u64 *val)
{
	struct amdgpu_device *adev = data;

	*val = adev->dm.dmcub_trace_event_en;
	return 0;
}

DEFINE_DEBUGFS_ATTRIBUTE(dmcub_trace_event_state_fops, dmcub_trace_event_state_get,
			 dmcub_trace_event_state_set, "%llu\n");

/*
 * Disables all HPD and HPD RX interrupt handling in the
 * driver when set to 1. Default is 0.
 */
static int disable_hpd_set(void *data, u64 val)
{
	struct amdgpu_device *adev = data;

	adev->dm.disable_hpd_irq = (bool)val;

	return 0;
}


/*
 * Returns 1 if HPD and HPRX interrupt handling is disabled,
 * 0 otherwise.
 */
static int disable_hpd_get(void *data, u64 *val)
{
	struct amdgpu_device *adev = data;

	*val = adev->dm.disable_hpd_irq;

	return 0;
}

DEFINE_DEBUGFS_ATTRIBUTE(disable_hpd_ops, disable_hpd_get,
			 disable_hpd_set, "%llu\n");

/*
 * Sets the DC visual confirm debug option from the given string.
 * Example usage: echo 1 > /sys/kernel/debug/dri/0/amdgpu_visual_confirm
 */
static int visual_confirm_set(void *data, u64 val)
{
	struct amdgpu_device *adev = data;

	adev->dm.dc->debug.visual_confirm = (enum visual_confirm)val;

	return 0;
}

/*
 * Reads the DC visual confirm debug option value into the given buffer.
 * Example usage: cat /sys/kernel/debug/dri/0/amdgpu_dm_visual_confirm
 */
static int visual_confirm_get(void *data, u64 *val)
{
	struct amdgpu_device *adev = data;

	*val = adev->dm.dc->debug.visual_confirm;

	return 0;
}

#ifdef DEFINE_DEBUGFS_ATTRIBUTE
DEFINE_DEBUGFS_ATTRIBUTE(visual_confirm_fops, visual_confirm_get,
			 visual_confirm_set, "%llu\n");
#endif

/*
 * Dumps the DCC_EN bit for each pipe.
 * Example usage: cat /sys/kernel/debug/dri/0/amdgpu_dm_dcc_en
 */
static ssize_t dcc_en_bits_read(
	struct file *f,
	char __user *buf,
	size_t size,
	loff_t *pos)
{
	struct amdgpu_device *adev = file_inode(f)->i_private;
	struct dc *dc = adev->dm.dc;
	char *rd_buf = NULL;
	const uint32_t rd_buf_size = 32;
	uint32_t result = 0;
	int offset = 0;
	int num_pipes = dc->res_pool->pipe_count;
	int *dcc_en_bits;
	int i, r;

	dcc_en_bits = kcalloc(num_pipes, sizeof(int), GFP_KERNEL);
	if (!dcc_en_bits)
		return -ENOMEM;

	if (!dc->hwss.get_dcc_en_bits) {
		kfree(dcc_en_bits);
		return 0;
	}

	dc->hwss.get_dcc_en_bits(dc, dcc_en_bits);

	rd_buf = kcalloc(rd_buf_size, sizeof(char), GFP_KERNEL);
	if (!rd_buf) {
		kfree(dcc_en_bits);
		return -ENOMEM;
	}

	for (i = 0; i < num_pipes; i++)
		offset += snprintf(rd_buf + offset, rd_buf_size - offset,
				   "%d  ", dcc_en_bits[i]);
	rd_buf[strlen(rd_buf)] = '\n';

	kfree(dcc_en_bits);

	while (size) {
		if (*pos >= rd_buf_size)
			break;
		r = put_user(*(rd_buf + result), buf);
		if (r) {
			kfree(rd_buf);
			return r; /* r = -EFAULT */
		}
		buf += 1;
		size -= 1;
		*pos += 1;
		result += 1;
	}

	kfree(rd_buf);
	return result;
}

/*
 * Readout HUBP TMZ state for each pipe.
 * Example usage: cat /sys/kernel/debug/dri/0/amdgpu_dm_tmz_en
 */
static ssize_t tmz_state_read(struct file *f, char __user *buf,
			      size_t size, loff_t *pos)
{
	struct amdgpu_device *adev = file_inode(f)->i_private;
	struct dc *dc = adev->dm.dc;
	char *rd_buf = NULL;
	const uint32_t rd_buf_size = 20;
	uint32_t result = 0;
	int offset = 0;
	int num_pipes = dc->res_pool->pipe_count;
	int *tmz_state;
	int i, r;

	tmz_state = kcalloc(num_pipes, sizeof(int), GFP_KERNEL);
	if (!tmz_state)
		return -ENOMEM;

	if (!dc->hwss.get_tmz_state) {
		kfree(tmz_state);
		return 0;
	}

	dc->hwss.get_tmz_state(dc, tmz_state);

	rd_buf = kcalloc(rd_buf_size, sizeof(char), GFP_KERNEL);
	if (!rd_buf) {
		kfree(tmz_state);
		return -ENOMEM;
	}

	for (i = 0; i < num_pipes; i++)
		offset += snprintf(rd_buf + offset, rd_buf_size - offset,
				   "%d  ", tmz_state[i]);
	rd_buf[strlen(rd_buf)] = '\n';

	kfree(tmz_state);

	while (size) {
		if (*pos >= rd_buf_size)
			break;
		r = put_user(*(rd_buf + result), buf);
		if (r) {
			kfree(rd_buf);
			return r; /* r = -EFAULT */
		}
		buf += 1;
		size -= 1;
		*pos += 1;
		result += 1;
	}

	kfree(rd_buf);
	return result;
}

/*
 * Readout HUBP Blanking state of each HUBP pipe.
 * The returned value is an integer, each bit (starting from LSB) represents
 * for either hubp blanked or not for each pipe.
 *
 * Example usage: cat /sys/kernel/debug/dri/0/amdgpu_dm_hubp_blank_state
 * returned value is 15 (in decimal, or 0xF in hex), means all the 4 pipes
 * hubp are blanked.
 */
static int hubp_blank_state_get(void *data, u64 *val)
{
	struct amdgpu_device *adev = (struct amdgpu_device *) data;
	struct dc *dc = adev->dm.dc;
	int n_hubp_pipes = dc->res_pool->pipe_count;
	bool *hubp_blanked;
	int i;

	hubp_blanked = kcalloc(n_hubp_pipes, sizeof(bool), GFP_KERNEL);
	if (!hubp_blanked)
		return -ENOMEM;

	if (!dc->hwss.get_hubp_blank_state) {
		kfree(hubp_blanked);
		return 0;
	}

	dc->hwss.get_hubp_blank_state(dc, hubp_blanked);

	for (*val = 0, i = 0; i < n_hubp_pipes; ++i) {
		if (hubp_blanked[i])
			*val |= (1 << i);
	}

	kfree(hubp_blanked);
	return 0;
}

DEFINE_DEBUGFS_ATTRIBUTE(hubp_blank_state_fops,
	hubp_blank_state_get, NULL, "%llu\n");

int dtn_debugfs_init(struct amdgpu_device *adev)
{
	static const struct file_operations dtn_log_fops = {
		.owner = THIS_MODULE,
		.read = dtn_log_read,
		.write = dtn_log_write,
		.llseek = default_llseek
	};
	static const struct file_operations dcc_en_bits_fops = {
		.owner = THIS_MODULE,
		.read = dcc_en_bits_read,
		.llseek = default_llseek
	};
	static const struct file_operations tmz_state_fops = {
		.owner = THIS_MODULE,
		.read = tmz_state_read,
		.llseek = default_llseek
	};
	static const struct file_operations force_timing_sync_ops = {
		.owner = THIS_MODULE,
		.read = force_timing_sync_get,
		.write = force_timing_sync_set,
		.llseek = default_llseek
	};

	struct drm_minor *minor = adev->ddev->primary;
	struct dentry *root = minor->debugfs_root;
	int ret;

	ret = amdgpu_debugfs_add_files(adev, amdgpu_dm_debugfs_list,
				ARRAY_SIZE(amdgpu_dm_debugfs_list));
	if (ret)
		return ret;

	debugfs_create_file("amdgpu_dm_dtn_log", 0644, root, adev,
			    &dtn_log_fops);

#ifdef DEFINE_DEBUGFS_ATTRIBUTE
	debugfs_create_file_unsafe("amdgpu_dm_visual_confirm", 0644, root, adev,
				   &visual_confirm_fops);

	debugfs_create_file_unsafe("amdgpu_dm_dmub_tracebuffer", 0644, root,
				   adev, &dmub_tracebuffer_fops);
#endif
	debugfs_create_file_unsafe("amdgpu_dm_force_timing_sync", 0644, root,
				   adev, &force_timing_sync_ops);

	debugfs_create_file_unsafe("amdgpu_dm_trigger_hpd_mst", 0644, root,
				   adev, &trigger_hpd_mst_ops);

	debugfs_create_file_unsafe("amdgpu_dm_dcc_en", 0644, root, adev,
				   &dcc_en_bits_fops);

	debugfs_create_file_unsafe("amdgpu_dm_tmz_en", 0644, root, adev,
				   &tmz_state_fops);

	debugfs_create_file_unsafe("amdgpu_dm_hubp_blank_state", 0644, root, adev,
				   &hubp_blank_state_fops);

	debugfs_create_file_unsafe("amdgpu_dm_dmcub_trace_event_en", 0644, root, adev,
				   &dmcub_trace_event_state_fops);

	debugfs_create_file_unsafe("amdgpu_dm_disable_hpd", 0644, root, adev,
				   &disable_hpd_ops);

	return 0;
}
