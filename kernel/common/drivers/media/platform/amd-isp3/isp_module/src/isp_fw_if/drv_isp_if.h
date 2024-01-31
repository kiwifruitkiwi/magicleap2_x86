/*
 * Copyright (C) 2019-2020 Advanced Micro Devices, Inc. All rights reserved.
 */

#ifndef _DRV_ISP_IF_H
#define _DRV_ISP_IF_H

#include "isp_fw_if/cmdresp.h"
#include "isp_fw_if/paramtypes.h"
#include "isp_fw_if/ispcalib_pkg_def.h"

struct _isp_sensor_exposure_t {
	unsigned int min_gain;	/*min analog gain */
	unsigned int max_gain;	/*max analog gain */
	unsigned int min_digi_gain;	/*min digital gain */
	unsigned int max_digi_gain;	/*max digital gain */
	unsigned int min_integration_time;	/*min integration time */
	unsigned int max_integration_time;	/*max integration time */
};

/*to be changed to real definition later*/

#define mmISP_MP_ISP_CTRL 0
#define mmISP_SP_ISP_CTRL 0
#define mmISP_MP_VI_IRCL 0
#define mmISP_SP_VI_IRCL 0
#define ISP_CCPU_CNTL__CLK_EN_MASK 1
#define AE_ISO_AUTO 0

struct _isp_lens_pos_config_t {
	unsigned int min_pos;		/*min lens position */
	unsigned int max_pos;		/*max lens position */
	unsigned int init_pos;	/*current lens position */
};

struct _isp_sensor_prop_t {
	struct _SensorProp_t prop;
	struct _SensorHdrProp_t hdrProp;
	struct _Window_t window;	/*the valid picture window. */
	unsigned int frame_rate;	/*frame rate, multiplied by 1000 */
	struct _isp_sensor_exposure_t exposure;
	struct _isp_lens_pos_config_t lens_pos;
	unsigned char hdr_valid; /**if hdr is supported*/
	unsigned int mipi_bitrate;	/*mbps */
	struct _isp_sensor_exposure_t exposure_orig;
};

#endif /*_DRV_ISP_IF_H*/
