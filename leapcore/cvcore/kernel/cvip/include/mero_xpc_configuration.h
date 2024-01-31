/* SPDX-License-Identifier: GPL-2.0-only
 *
 * Copyright (c) 2020-2023, Magic Leap, Inc. All rights reserved.
 */

#ifndef __MERO_XPC_CONFIGURATION_H
#define __MERO_XPC_CONFIGURATION_H

#include "mero_xpc_common.h"
#include "mero_xpc_types.h"

#define IPC3_USEABLE_MAILBOX_MASK (0x3FFFFFE0)

#if defined(__arm__) || defined(__aarch64__)
#define XPC_CORE_ID CORE_ID_A55
#elif defined(__i386__) || defined(__amd64__)
#define XPC_CORE_ID CORE_ID_X86
#else
#error Unknown core type
#endif

void set_core_channel_mask(int core_id, int ipc_id, uint32_t mask);
uint32_t get_core_channel_mask(int core_id, int ipc_id);
int xpc_compute_route(uint32_t destinations, struct xpc_route_solution* route);

#ifndef __KERNEL__
#error "Can't include kernel header in userspace"
#endif

#endif
