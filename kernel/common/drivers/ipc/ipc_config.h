/*
 * Header file for IPC library mailbox configuration
 *
 * Copyright (C) 2019 Advanced Micro Devices, Inc. All rights reserved.
 */

#ifndef __IPC_CONFIGS_H__
#define __IPC_CONFIGS_H__

#include "ipc_types.h"

int ipc_chans_setup(struct device *dev, struct ipc_context *ipc);
int ipc_chans_cleanup(struct ipc_context *ipc);
int isp_chans_setup(struct device *dev, struct ipc_context *ipc);

#endif /* __IPC_CONFIGS_H__ */

