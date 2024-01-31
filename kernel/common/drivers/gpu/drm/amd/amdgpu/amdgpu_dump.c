/*
 * Copyright 2019, 2020 Advanced Micro Devices, Inc.
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
 */

#include "amdgpu.h"

/**
 * amdgpu_dump_status - dump gpu register status
 *
 * @dev: drm dev pointer
 * @buf: buffer to store the gpu register status
 * @len: length of the available space in @buf
 *
 * Upon gpu hang, dump gpu register status into the @buf
 * Returns number of bytes written to @buf
 */
size_t amdgpu_dump_status(struct drm_device *dev, char *buf, size_t len)
{
	struct amdgpu_device *adev = dev->dev_private;
	size_t size = 0;

	if (adev->asic_funcs->get_asic_status)
		size = adev->asic_funcs->get_asic_status(adev, buf, len);

	return size;
}
