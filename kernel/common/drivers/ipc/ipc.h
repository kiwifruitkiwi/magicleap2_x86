/*
 * Main IPC library header file
 *
 * Copyright (C) 2019-2021 Advanced Micro Devices, Inc. All rights reserved.
 */

#ifndef __IPC_LIB_H__
#define __IPC_LIB_H__

#include <linux/slab.h>
#include <linux/kthread.h>
#include <linux/mailbox_client.h>
#include <linux/io.h>
#include "ipc_types.h"
#include "ipc_messageid.h"
#include "ipc_core.h"
#include "ipc_config.h"
#include "ipc_queue.h"
#include "ipc_sysfs.h"
#include "ipc_isp.h"
#include "ipc_smu.h"
#include "ipc_sensor.h"

#if defined(CONFIG_CVIP_IPC) && defined(CONFIG_X86_IPC)
#error "CVIP_IPC and X86_IPC can not coexist"
#endif

#define IPC_X86_SAVE_DATA_OK	1
#define IPC_X86_SAVE_DATA_ERR	0

enum chan_type {
	ISP_CHAN	= 0,
	IPC_CHAN	= 1,
};

int ipc_deinit(struct ipc_context *ipc);
int ipc_register_client_callback(struct ipc_context *ipc,
				 struct ipc_message_range *range,
				 client_callback_t cb);
int ipc_unregister_client_callback(struct ipc_context *ipc,
				   struct ipc_message_range *range);

bool is_ipc_channel_ready(void);

#if defined(CONFIG_CVIP_IPC)
void isp_data_u2f(u32 fixed_data, float *float_data);
void change_lens_pos(struct isp_ipc_device *dev, int cam_id);
#else
static inline void isp_data_u2f(u32 fixed_data, float *float_data) { }

static inline void change_lens_pos(struct isp_ipc_device *dev, int cam_id) { }
#endif

#if defined(CONFIG_CVIP_IPC) || defined(CONFIG_X86_IPC)
void ipc_eventfd_signal(int val);
bool is_smu_pm_enabled(void);
int ipc_init(struct ipc_context **ipc);
int ipc_sendmsg_timeout(struct ipc_context *ipc,
			struct ipc_message *msg,
			unsigned long timeout);
int ipc_sendmsg(struct ipc_context *ipc, struct ipc_message *msg);
void ipc_mboxes_cleanup(struct ipc_context *ipc, enum chan_type type);
#else
static inline void ipc_eventfd_signal(int val)
{
}

static inline bool is_smu_pm_enabled(void)
{
	return false;
}

static inline int ipc_init(struct ipc_context **ipc)
{
	return 0;
}

static inline int ipc_sendmsg(struct ipc_context *ipc, struct ipc_message *msg)
{
	return 0;
}

static inline void ipc_mboxes_cleanup(struct ipc_context *ipc, enum chan_type type) { }
#endif
#endif /* __IPC_LIB_H__ */
