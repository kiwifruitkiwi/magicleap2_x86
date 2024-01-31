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
 **************************************************************************/

#ifndef BUFFER_MGR_H
#define BUFFER_MGR_H

enum isp_buffer_status {
	ISP_BUFFER_STATUS_FREE,
	ISP_BUFFER_STATUS_IN_FW,
	ISP_BUFFER_STATUS_MAX
};

struct isp_img_buf_info {
	struct isp_img_buf_info *next;
	pvoid virtual_addr;
	uint64 phy_addr;
	uint32 size;
	uint16 camera_id;
	uint16 stream_id;
	enum isp_buffer_status buf_status;
	sys_img_buf_handle_t sys_img_buf_hdl;	/*sys_img_buf_handle_t typed */
	/*struct isp_img_buf_info *next; */
};

#endif
