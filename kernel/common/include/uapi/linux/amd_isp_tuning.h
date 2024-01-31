/* SPDX-License-Identifier: (GPL-2.0 OR BSD-2-Clause) WITH Linux-syscall-note */
/*
 * Advanced Micro Device ISP tuning header file.
 *
 * A v4l2 subdev named 'amd-isp-tuning' would be exposed
 * at /dev/v4l-subdevX and application shall traverse all
 * sub devices and open the one named 'amd-isp-tuning' to
 * exchange the online tuning data.
 *
 * Copyright (s) 2020 Advanced Micro Device inc.
 */
#ifndef _UAPI__LINUX_AMD_ISP_TUNING_H
#define _UAPI__LINUX_AMD_ISP_TUNING_H
#include <linux/types.h>

/*
 * ISP tuning parameter. This parameter is the argument of private IOCTL
 * of V4L2_CID_AMD_ISP_TUNE to specify tuning parameter to ISP directly
 * (on-the-fly). This is the synchornized control, do not invoke this IOCTL
 * with async call, or it would return -EINVAL immediately.
 *  @cmd: command from cmdresp.h, should be with prefix CMD_ID_*.
 *  @tuning_buf_addr_lo: lower 32 bits of address that caller prepared.
 *  @tuning_buf_addr_hi: higher 32 bits address of the buffer that caller
 *  prepared for ISP driver to read/write data from/to ISP firmware. This
 *  buffer address must be available to kernel space, no external mmap
 *  willbe performed.
 *  @tuning_buf_size: describe the buffer size.
 *  @is_set_cmd: describe the command is a set command or not.
 *  0: Not.
 *  1: Yes.
 *  @is_stream_cmd: describe the command is a stream specific command or not.
 *  Caller has no responsibiliy to provide the given stream if |is_stream_cmd|
 *  set to 1, ISP driver will retrieve it automatically by the current in using
 *  camera ID.
 *  0: Not.
 *  1: Yes.
 *  @is_direct_cmd: describe the command is a direct command or indirect
 *  command. This information must be correct according to the ISP tuning
 *  command document.
 *  0: Indirect.
 *  1: Direct.
 *  @__reserved: reserved filed, keep to 0.
 *  @result.error_code: 0 indicates to ok, otherwise refer to paramtypes.h.
 *  @result.payload: the ISP firmware returned payload, see the document for
 *  more information.
 */
struct amd_isp_tuning {
	u32 cmd;
	u32 tuning_buf_addr_lo;
	u32 tuning_buf_addr_hi;
	u32 tuning_buf_size;
	u8 is_set_cmd;
	u8 is_stream_cmd;
	u8 is_direct_cmd;
	u8 __reserved[1];
	struct {
		s32 error_code;
		u8 payload[36];
	} result;
};

/* AMD ISP tuning subdev name */
#define AMD_ISP_TUNING_SUBDEV_NAME "amd-isp-tuning"

/* private ioctls to exchange online tuning parcel */
#define V4L2_CID_AMD_ISP_TUNE                                                 \
	_IOWR('V', BASE_VIDIOC_PRIVATE, struct amd_isp_tuning)

#endif /* _UAPI__LINUX_AMD_ISP_TUNING_H */
