/**************************************************************************
 *copyright 2014~2015 advanced micro devices, inc.
 *
 *permission is hereby granted, free of charge, to any person obtaining a
 *copy of this software and associated documentation files (the "software"),
 *to deal in the software without restriction, including without limitation
 *the rights to use, copy, modify, merge, publish, distribute, sublicense,
 *and/or sell copies of the software, and to permit persons to whom the
 *software is furnished to do so, subject to the following conditions:
 *
 *the above copyright notice and this permission notice shall be included in
 *all copies or substantial portions of the software.
 *
 *THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL
 *THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 *OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 *ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 *OTHER DEALINGS IN THE SOFTWARE.
 *
 *************************************************************************
 */

/*****************************************************************************
 *this header defines ISP driver to firmware interface. this interface header
 *file is intended to be used by tensilica C compiler and compatible compilers.
 ******************************************************************************
 */
#ifndef _DRV_ISP_IF_H
#define _DRV_ISP_IF_H

#include "isp_fw_if/cmdresp.h"
#include "isp_fw_if/paramtypes.h"
#include "isp_fw_if/ispcalib_pkg_def.h"

typedef struct _isp_sensor_exposure_t {
	uint32 min_gain;	/*min analog gain */
	uint32 max_gain;	/*max analog gain */
	uint32 min_digi_gain;	/*min digital gain */
	uint32 max_digi_gain;	/*max digital gain */
	uint32 min_integration_time;	/*min integration time */
	uint32 max_integration_time;	/*max integration time */
} isp_sensor_exposure_t;

/*to be changed to real definition later*/

#define mmISP_MP_ISP_CTRL 0
#define mmISP_SP_ISP_CTRL 0
#define mmISP_MP_VI_IRCL 0
#define mmISP_SP_VI_IRCL 0
#define ISP_CCPU_CNTL__CLK_EN_MASK 1
#define AE_ISO_AUTO 0

typedef struct _isp_lens_pos_config_t {
	uint32 min_pos;		/*min lens position */
	uint32 max_pos;		/*max lens position */
	uint32 init_pos;	/*current lens position */
} isp_lens_pos_config_t;

typedef struct _isp_sensor_prop_t {
	SensorProp_t prop;
	SensorHdrProp_t hdrProp;
	window_t window;	/*the valid picture window. */
	uint32 frame_rate;	/*frame rate, multiplied by 1000 */
	isp_sensor_exposure_t exposure;
	isp_lens_pos_config_t lens_pos;
	uint8 hdr_valid; /**if hdr is supported*/
	uint32 mipi_bitrate;	/*mbps */
	isp_sensor_exposure_t exposure_orig;
} isp_sensor_prop_t;

#endif /*_DRV_ISP_IF_H*/
