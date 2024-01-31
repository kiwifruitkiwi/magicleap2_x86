/*
 * Copyright (C) 2020-2021 Advanced Micro Devices, Inc. All rights reserved.
 */

#ifndef __IPC_SENSOR_H__
#define __IPC_SENSOR_H__

/* isp fw requires frame id starting from 1 */
#define STARTING_FRAME_NUMBER 1

#ifdef CONFIG_ISP_SENSOR_SIM
void sensor_simulation(struct isp_ipc_device *dev, enum isp_camera cam_id,
		       enum reg_stream_status mode, bool resume);
#else
static inline
void sensor_simulation(struct isp_ipc_device *dev, enum isp_camera cam_id,
		       enum reg_stream_status mode, bool resume)
{
}
#endif

#endif /* __IPC_SENSOR_H__ */
