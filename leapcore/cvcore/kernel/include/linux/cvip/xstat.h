/* SPDX-License-Identifier: GPL-2.0-only
 *
 * \file xstat.h
 *
 * \brief  xstat API definitions.
 *
 * Copyright (c) 2021-2022, Magic Leap, Inc. All rights reserved.
 */

#ifndef __XSTAT_H
#define __XSTAT_H

#include "cvcore_typedefs.h"

/**
 * xstat state list
 */
#define XSTAT_STATE_LIST(x)                                    \
    x(XSTAT_STATE_OFFLINE, "OFFLINE")                          \
    x(XSTAT_STATE_INITIALIZING, "INITIALIZING")                \
    x(XSTAT_STATE_MODULES_LOADED, "MODULES_LOADED")            \
    x(XSTAT_STATE_NETWORK_READY, "NETWORK_READY")              \
    x(XSTAT_STATE_BOOT_VERIFIED, "BOOT_VERIFIED")              \
    x(XSTAT_STATE_OPERATIONAL, "OPERATIONAL")                  \
    x(XSTAT_STATE_INVALID, "INVALID")

enum xstat_state { XSTAT_STATE_LIST(CVCORE_ENUM_DEF) };

/**
 * Wearable state list
 *  This is meant as a 'short-term' implementation to get low-level
 *  wearable state to x86 but in all likelihood will survive longer
 *  then originally intended. States defined here should match
 *  what's defined in the PIL.
 */
#define XSTAT_WEARABLE_STATE_LIST(x)                            \
    x(XSTAT_WEARABLE_STATE_OFFLINE, "OFFLINE")                  \
    x(XSTAT_WEARABLE_STATE_HOTPLUG_REMOVE, "HOTPLUG_REMOVE")    \
    x(XSTAT_WEARABLE_STATE_HOTPLUG_ADD, "HOTPLUG_ADD")          \
    x(XSTAT_WEARABLE_STATE_ONLINE, "ONLINE")                    \
    x(XSTAT_WEARABLE_STATE_SHUTDOWN, "SHUTDOWN")                \
    x(XSTAT_WEARABLE_STATE_REBOOT_REQUEST, "REBOOT_REQUEST")    \
    x(XSTAT_WEARABLE_STATE_INVALID, "INVALID")

enum xstat_wearable_state { XSTAT_WEARABLE_STATE_LIST(CVCORE_ENUM_DEF) };

/**
 * xstat info structure
 *
 * This struct will hold xstat information for a core.
 *
 */
struct __attribute__((packed)) xstat_core_info {
    // Core state
    enum xstat_state core_state;
    // Wearable state
    enum xstat_wearable_state wearable_state;
};

/**
 * Request CVIP xstat information
 *
 * This function will send a xstat request to the CVIP and wait for a response.
 * The request will timeout if a response is not received within timeout_ms.
 *
 * \param [out]  info        location to store xstat information
 * \param [in]   timeout_ms  timeout in milliseconds to wait for a response.\n
 *                           If 0, default timeout of 1 second will be used.
 *
 * \return       0 on success\n
 *               -ETIMEDOUT if timeout occurred while retrieving the
 * information\n -EINVAL if a parameter is invalid -ECOMM if there is an issue
 * sending the request -EBADMSG if an invalid message was received
 */
int xstat_request_cvip_info(struct xstat_core_info *info, uint32_t timeout_ms);

/**
 * Request x86 xstat information
 *
 * This function will send a xstat request to the x86 and wait for a response.
 * The request will timeout if a response is not received within timeout_ms.
 *
 * \param [out]  info        location to store xstat information
 * \param [in]   timeout_ms  timeout in milliseconds to wait for a response.\n
 *                           If 0, default timeout of 1 second will be used.
 *
 * \return       0 on success\n
 *               -ETIMEDOUT if timeout occurred while retrieving the
 * information\n -EINVAL if a parameter is invalid -ECOMM if there is an issue
 * sending the request -EBADMSG if an invalid message was received
 */
int xstat_request_x86_info(struct xstat_core_info *info, uint32_t timeout_ms);

/**
 * Set xstat state for the current core
 *
 * \param [in]   state  xstat_state enum
 *
 * \return       0 on success\n
 *               -EINVAL if a parameter is invalid
 */
int xstat_set_state(enum xstat_state state);

/**
 * Get a string representation of xstat_state
 *
 * \param [in]   state  xstat_state enum
 *
 * \return       String representation of xstat_state on success\n
 *               INVALID if xstat_state enum is invalid
 */
const char *xstat_state_str(enum xstat_state state);

/**
 * Set xstat wearable state
 *
 * \param [in]   state  xstat_wearable_state enum
 *
 * \return       0 on success\n
 *               -EINVAL if a parameter is invalid
 */
int xstat_set_wearable_state(enum xstat_wearable_state state);

/**
 * Get a string representation of xstat_wearable_state
 *
 * \param [in]   state  xstat_wearable_state enum
 *
 * \return       String representation of xstat_wearable_state on success\n
 *               INVALID if xstat_wearable_state enum is invalid
 */
const char *xstat_wearable_state_str(enum xstat_wearable_state state);

#ifndef __KERNEL__
#error "Can't include kernel header in userspace"
#endif

#endif
