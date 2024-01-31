/*
 * Copyright 2019 Advanced Micro Devices, Inc.
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

#ifndef __AMDGPU_DM_CV_DRIVER_H__
#define __AMDGPU_DM_CV_DRIVER_H__

/**
 * Events sent to the CV driver to indicate state changes on the display output
 * for the ML headset
 * @MODE_SET: signals enablement of display output to the ML headset
 * @MODE_RESET: signals disablement of display output to the ML headset
 * @DARK_MODE_ENTER: signalled after MODE_SET until MODE_RESET
 * @DARK_MODE_EXIT: signals exit of dark mode (DP link bringup)
 * @VSYNC: signalled in the vblank period after MODE_SET and until MODE_RESET
 */
enum amdgpu_dm_cv_driver_event {
	MODE_SET = 1,
	MODE_RESET,
	DARK_MODE_ENTER,
	DARK_MODE_EXIT,
	VSYNC
};

/* function pointer to a callback function to handle various events */
typedef void (*amdgpu_dm_cv_driver_callback)(
				enum amdgpu_dm_cv_driver_event amdgpu_dm_cv_driver_event,
				int connector_id, int frame_length, int scanline );

#endif /* __AMDGPU_DM_CV_DRIVER_H__ */
