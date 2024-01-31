/*
 * Defines for mailbox userspace control in mero SOC.
 *
 * Copyright (C) 2019-2021 Advanced Micro Devices, Inc. All rights reserved.
 */

#ifndef _UAPI_LINUX_MERO_IPCDEV_H_
#define _UAPI_LINUX_MERO_IPCDEV_H_

#include <linux/types.h>
#include <linux/ioctl.h>

#if defined(__cplusplus)
extern "C" {
#endif

#define DATA_SIZE (4 * 7)
#define IPC_LOOPBACK 3

struct ipc_config {
	__u8 srcid;
	union {
		__u8 destid;
		__u32 destmask;
	};
	__u8 slave;
	__u8 soft_link;
	__u8 rx;
	__u8 rx_ackbyuser;
	__u8 tx_block;
	__u8 auto_ack;
	__u64 tx_tout;
};

/* ioctl definitions
 * IPC_IOCCONFIG: userspace has to config hardware registers first
 */
#define IPC_IOC_MAGIC 'm'
#define IPC_IOCCONFIG	_IOW(IPC_IOC_MAGIC, 1, struct ipc_config)

#if defined(__cplusplus)
}
#endif

#endif //_UAPI_LINUX_MERO_IPCDEV_H_
