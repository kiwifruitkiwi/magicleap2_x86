/*
 * Header file for IPC library message ids
 *
 * Copyright (C) 2019-2021 Advanced Micro Devices, Inc. All rights reserved.
 */

#ifndef __IPC_MESSAGE_IDS_H__
#define __IPC_MESSAGE_IDS_H__

/* define x86 <> CVIP IPC message IDs */
#define ALIVE			0x1
#define PANIC			0x2
#define REBOOT			0x3
#define INTR_TEST		0x4
#define PANIC_ACK		0x5
#define OFF_HEAD		0x6
#define ON_HEAD			0x7
#define REGISTER_BUFFER		0x1000
#define UNREGISTER_BUFFER	0x1001
#define SEND_PM_EVENT		0x1002
#define S2RAM_REQ		0x1003
#define S2RAM_ACK		0x1004
#define RING_UPDATE		0x1010
#define FENCE_UPDATE		0x1011
#define REBOOT_REQ		0x1012
#define LOW_POWER_ACK		0x1013
#define INSTALL_DUMP_KERNEL	0x1014
#define SET_GPUVA		0x1015
#define NOVA_PANIC		0x1016
#define CVIP_PANIC		0x1017
#define NOVA_ALIVE		0x1018

/* CVIP S3 state */
#define CVIP_S3_FAIL		0x01

/* define PM events */
#define DEVICE_STATE		0x0
#define LOW_POWER_MODE		0x1
#define X86_S0			0x1000
#define X86_IDLE		0x1001
#define X86_STANDBY		0x1002
#define X86_S3			0x1003

/* define isp <> CVIP IPC message IDs */
#define CAM0_CONTROL		0x2000
#define CAM1_CONTROL		0x2001
#define CAM0_IR_STAT		0x2010
#define CAM1_IR_STAT		0x2011
#define CAM0_RGB_STAT		0x2020
#define CAM1_RGB_STAT		0x2021
#define CAM0_INPUT_BUFF		0x2030
#define CAM1_INPUT_BUFF		0x2031
#define CAM0_PRE_DATA_BUFF	0x2040
#define CAM1_PRE_DATA_BUFF	0x2041
#define CAM0_POST_DATA_BUFF	0x2050
#define CAM1_POST_DATA_BUFF	0x2051
#define CAM0_OUTPUT_YUV_0	0x2060
#define CAM0_OUTPUT_YUV_1	0x2062
#define CAM0_OUTPUT_YUV_2	0x2064
#define CAM0_OUTPUT_YUV_3	0x2066
#define CAM0_OUTPUT_IR		0x2068
#define CAM1_OUTPUT_YUV_0	0x2061
#define CAM1_OUTPUT_YUV_1	0x2063
#define CAM1_OUTPUT_YUV_2	0x2065
#define CAM1_OUTPUT_YUV_3	0x2067
#define CAM1_OUTPUT_IR		0x2069
#define CAM0_WDMA_STATUS	0x2070
#define CAM1_WDMA_STATUS	0x2071

/* define ML PM x86 <> CVIP IPC message IDs */
#define CVIP_QUERY_S2IDLE	0x3000
#define CVIP_QUERY_S2IDLE_ACK	0x3001
#define CVIP_ALLOW_S2IDLE	0x0
#define CVIP_BLOCK_S2IDLE	0x1

#define STREAM_TYPE_OFF		(CAM0_OUTPUT_YUV_1 - CAM0_OUTPUT_YUV_0)
#define PRE_POST_OFF		(CAM0_POST_DATA_BUFF - CAM0_PRE_DATA_BUFF)
#define IR_RGB_STAT_OFF		(CAM0_RGB_STAT - CAM0_IR_STAT)
#endif /* __IPC_MESSAGE_IDS_H__ */
