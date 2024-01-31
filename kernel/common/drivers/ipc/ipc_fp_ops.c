/*
 * IPC floating point operation functions
 *
 * Copyright (C) 2021-2022 Advanced Micro Devices, Inc. All rights reserved.
 */

#include "ipc.h"

/* from isp fw aidt */
/* sign: 0, integer_bits: 8, decimal_bits: 16 */
#define ISP_DIV16 (0.000015258789063) /* (1 / (1 << 16)) */
void isp_data_u2f(u32 fixed_data, float *float_data)
{
	u32 lltemp = fixed_data & (((u32)1 << (8 + 16)) - 1);
	double tmp0, tmp1;

	/*
	 * explicitly save D0 and D1 register used in the multiply ops below
	 */
	asm("str d0, [%0]\n"
	    "str d1, [%1]\n"
	    :: "r" (&tmp0), "r" (&tmp1) : "memory");

	*float_data = lltemp * ISP_DIV16;

	/* restore D0 and D1 */
	asm("ldr d0, [%0]\n"
	    "ldr d1, [%1]\n"
	    :: "r" (&tmp0), "r" (&tmp1) : "memory");
}

void change_lens_pos(struct isp_ipc_device *dev, int cam_id)
{
	struct camera *cam = &dev->camera[cam_id];

	/* no real sensor, so set the near pos to isp fw */
	cam->pre_data.afmetadata.distance =
		cam->pre_data.afmetadata.distancenear;
}
