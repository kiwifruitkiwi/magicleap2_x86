/*
 * Copyright (C) 2020 Advanced Micro Devices, Inc. All rights reserved.
 */

#ifndef __IPC_SMU_H__
#define __IPC_SMU_H__

/* Message IDs from SMU */
#define SMU_M1SEND_SEND_READY				0x2
#define SMU_M1SEND_THROTTLE_STATUS			0x1

/* defined for throttling status */
enum cvip_throttling_status {
	THROTTLING_END		= 0x0,
	THROTTLING_START	= 0x1,
};

#endif /* __IPC_SMU_H__ */

