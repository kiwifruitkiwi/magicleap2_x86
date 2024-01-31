/* SPDX-License-Identifier: GPL-2.0-only
 *
 * \file cvcore_status.h
 *
 * \brief CVCore status and error code definitions.
 *
 * Copyright (c) 2019-2024, Magic Leap, Inc. All rights reserved.
 */

#pragma once

#include "cvcore_typedefs.h"

/**
* CVCore error codes.
*/

#define CVCORE_STATUS_LIST(x)                                                                             \
  x(CVCORE_STATUS_RESERVED,                           "Reserved (specifically set to 0)")                 \
  x(CVCORE_STATUS_SUCCESS,                            "Operation Completed Successfully")                 \
  x(CVCORE_STATUS_IN_PROGRESS,                        "Operation in Progress")                            \
  x(CVCORE_STATUS_FAILURE_GENERAL,                    "General Failure")                                  \
  x(CVCORE_STATUS_FAILURE_UNSUPPORTED_OPERATION,      "Operation Not Supported")                          \
  x(CVCORE_STATUS_FAILURE_UNSUPPORTED_TYPE,           "Type Not Supported")                               \
  x(CVCORE_STATUS_FAILURE_INSUFFICIENT_SIZE,          "Invalid Object Size Provided")                     \
  x(CVCORE_STATUS_FAILURE_INCORRECT_MODE,             "Invalid Operational Mode")                         \
  x(CVCORE_STATUS_FAILURE_FEATURE_UNAVAILABLE,        "Feature Unavailable")                              \
  x(CVCORE_STATUS_FAILURE_TIMEOUT,                    "Operation Timeout")                                \
  x(CVCORE_STATUS_FAILURE_FEATURE_OFFLINE,            "Feature Offline")                                  \
  x(CVCORE_STATUS_FAILURE_INVALID_PARAMETER,          "Invalid Parameter")                                \
  x(CVCORE_STATUS_FAILURE_OPERATION_ERROR,            "Operation Error")                                  \
  x(CVCORE_STATUS_FAILURE_NOT_READY,                  "Feature Not Ready")                                \
  x(CVCORE_STATUS_FAILURE_SELF_TEST,                  "Self Test Failure")                                \
  x(CVCORE_STATUS_FAILURE_INITIALIZATION,             "Initialization Failure")                           \
  x(CVCORE_STATUS_FAILURE_EXISTS,                     "Item Already Exists")                              \
  x(CVCORE_STATUS_FAILURE_NOT_FOUND,                  "Item Not Found")                                   \
  x(CVCORE_STATUS_FAILURE_NO_MEMORY,                  "Memory Not Avaliable")                             \
  x(CVCORE_STATUS_FAILURE_INTERNAL_ERROR,             "Internal Error")                                   \
  x(CVCORE_STATUS_FAILURE_UNRECOGNIZED_NAME,          "Name Not Recognized")                              \
  x(CVCORE_STATUS_FAILURE_PARSE_ERROR,                "Parsing Error")                                    \
  x(CVCORE_STATUS_FAILURE_PERMISSION_DENIED,          "Permission Denied")                                \
  x(CVCORE_STATUS_FAILURE_NOT_SHAREABLE,              "Resource Not Shareable")                           \
  x(CVCORE_STATUS_FAILURE_SIZE_TOO_LARGE,             "Parameter Size Too Large")                         \
  x(CVCORE_STATUS_FAILURE_TRY_AGAIN,                  "Resource Unavailable - Try Again")                 \
  x(CVCORE_STATUS_FAILURE_QUOTA_EXCEEDED,             "Resource quota has exceeded.")                     \
  x(CVCORE_STATUS_FAILURE_QPAD_READ_INTERNAL,         "Internal qPad Read Failure")                       \
  x(CVCORE_STATUS_FAILURE_QPAD_WRITE_INTERNAL,        "Internal qPad Write Failure")                      \
  x(CVCORE_STATUS_FAILURE_QPAD_TIMESTAMP,             "qPad Read Timestamp Not Found")                    \
  x(CVCORE_STATUS_FAILURE_QPAD_EVENT_ID,              "qPad Event-ID Not Found")                          \
  x(CVCORE_STATUS_FAILURE_QPAD_READ_LONG_PREEMPTION,  "qPad Reader Preempted For Too Long")               \
  x(CVCORE_STATUS_FAILURE_SC_IPC,                     "Sensor Core IPC Failure")                          \
  x(CVCORE_STATUS_FAILURE_SC_BUSY,                    "Sensor Core Busy")                                 \
  x(CVCORE_STATUS_FAILURE_SC_OFFLINE,                 "Sensor Core Offline")                              \
  x(CVCORE_STATUS_FAILURE_SC_INVALID_CMD,             "Sensor Core Invalid Command")                      \
  x(CVCORE_STATUS_FAILURE_SC_INVALID_CLIENT,          "Sensor Core Invalid Client")                       \
  x(CVCORE_STATUS_FAILURE_SC_INVALID_STREAM,          "Sensor Core Invalid Stream")                       \
  x(CVCORE_STATUS_FAILURE_SC_INVALID_CFG,             "Sensor Core Invalid Configuration")                \
  x(CVCORE_STATUS_FAILURE_SC_INVALID_STATE,           "Sensor Core Invalid State")                        \
  x(CVCORE_STATUS_FAILURE_SC_OVERFLOW_CLIENT,         "Sensor Core Unable to add Client")                 \
  x(CVCORE_STATUS_FAILURE_VERSION_MISMATCH,           "Code version does not match data")                 \
  x(CVCORE_STATUS_FAILURE_HC_HYDRA_ID_OOR,            "Hydra Control hydra-id is out of range")           \
  x(CVCORE_STATUS_FAILURE_HC_STATE_SEM_INIT_ERR,      "Hydra Control State-Sem Init Error")               \
  x(CVCORE_STATUS_FAILURE_HC_SRAM_INIT_ERR,           "Hydra Control SRAM Init Error")                    \
  x(CVCORE_STATUS_FAILURE_HC_LOCK_INIT_ERR,           "Hydra Control Lock Init Error")                    \
  x(CVCORE_STATUS_FAILURE_HC_COMPONENT_ID_OOR,        "Hydra Control component-id is out of range")       \
  x(CVCORE_STATUS_FAILURE_HC_HCSM_OPEN_ERR,           "Hydra Control HCSM Failed to Open FD")             \
  x(CVCORE_STATUS_FAILURE_HC_HCSM_IO_FAILURE,         "Hydra Control HCSM Unknown IO Failure")            \
  x(CVCORE_STATUS_FAILURE_HC_NOT_ONLINE,              "Hydra Control Not Online")                         \
  x(CVCORE_STATUS_FAILURE_HC_REQUEST_PTR_IS_NULL,     "Hydra Control Obj-Request Ptr Is Null")            \
  x(CVCORE_STATUS_FAILURE_HC_COMPONENT_LOCK_TO,       "Hydra Control Component Lock Access Timeout")      \
  x(CVCORE_STATUS_FAILURE_HC_COMPONENT_LOCK_ERR,      "Hydra Control Unexpected error on component lock") \
  x(CVCORE_STATUS_FAILURE_HC_NOT_INITIALIZED,         "Hydra Control Not Initialized by caller")          \
  x(CVCORE_STATUS_FAILURE_HC_PROC_ACCESS_LOCKED,      "Hydra Control Access Locked By Process")           \
  x(CVCORE_STATUS_FAILURE_HC_DEPENDENCY_ACQ_ERR,      "Hydra Control Dependency Acquisition Error")       \
  x(CVCORE_STATUS_FAILURE_HC_DEPENDENCY_REL_ERR,      "Hydra Control Dependency Release Error")           \
  x(CVCORE_STATUS_FAILURE_HC_HCSM_HYDRA_TO,           "HCSM Hydra Response Timeout")                      \
  x(CVCORE_STATUS_FAILURE_HC_HCSM_XCMP_WRITE_ERR,     "HCSM Write To XCMP Failed")                        \
  x(CVCORE_STATUS_FAILURE_HC_HCSM_UNKNOWN_ERR,        "HCSM Unknown Wait-Error")                          \
  x(CVCORE_STATUS_FAILURE_HC_HCSM_INVALID_IOCTL,      "HCSM Invalid IOCTL")                               \
  x(CVCORE_STATUS_FAILURE_HC_HCSM_COMPONENT_ID_OOR,   "HCSM IOCTL Component-ID Out Of Range")             \
  x(CVCORE_STATUS_FAILURE_HC_HCSM_HYDRA_ID_OOR,       "HCSM IOCTL Hydra-ID Out Of Range")                 \
  x(CVCORE_STATUS_FAILURE_HC_ERR_LOCK,                "Loss of Hydra Hardware Detected")                  \
  x(CVCORE_STATUS_FAILURE_DSP_MPU_FAILURE,            "DSP MPU Entry Not Set")                            \
  x(CVCORE_STATUS_FAILURE_DSP_XOS_INTERNAL,           "DSP XOS Internal Failure")                         \
  x(CVCORE_STATUS_FAILURE_DSP_XTOS_INTERNAL,          "DSP XTOS Internal Failure")                        \
  x(CVCORE_STATUS_FAILURE_DSP_ALREADY_INITIALIZED,    "DSP Core API is Initialized Already")              \
  x(CVCORE_STATUS_FAILURE_DSP_OFFLINE,                "DSP Core DSP is Currently Offline")                \
  x(CVCORE_STATUS_FAILURE_DSP_IN_SLEEP_MODE,          "DSP in low power mode (WAITI)")                    \
  x(CVCORE_STATUS_FAILURE_DSP_STREAM_ID_UNAVAILABLE,  "DSP Core StreamID is Unavailable")                 \
  x(CVCORE_STATUS_FAILURE_DSP_STREAM_ID_INVALID,      "DSP Core StreamID is Invalid")                     \
  x(CVCORE_STATUS_FAILURE_DSP_STREAM_ID_ALREADY_SET,  "DSP Core StreamID has already been set")           \
  x(CVCORE_STATUS_FAILURE_DSP_PRIORITY_INVALID,       "DSP Core Priority is Out of Range")                \
  x(CVCORE_STATUS_FAILURE_DSP_STACK_SIZE_TOO_LARGE,   "DSP Core Stack is Too Large")                      \
  x(CVCORE_STATUS_FAILURE_DSP_STACK_SIZE_INVALID,     "DSP Core Stack Size Requested is Invalid")         \
  x(CVCORE_STATUS_FAILURE_DSP_TASK_NOT_STARTED,       "DSP Core Task Start Failure")                      \
  x(CVCORE_STATUS_FAILURE_DSP_TASK_RUN_NOT_FOUND,     "DSP Core Task Execution Context not found")        \
  x(CVCORE_STATUS_FAILURE_DSP_IDMA_CANNOT_START,      "DSP OS iDMA cannot start")                         \
  x(CVCORE_STATUS_FAILURE_DSP_IDMA_CANNOT_SLEEP,      "DSP OS iDMA cannot sleep")                         \
  x(CVCORE_STATUS_FAILURE_DSP_IDMA_BUSY,              "DSP OS iDMA is active or running")                 \
  x(CVCORE_STATUS_FAILURE_DSP_TASK_BLOCKED_ON_IDMA,   "DSP OS task blocked wait for iDMA")                \
  x(CVCORE_STATUS_FAILURE_DSP_IDMA_INTERNAL_ERROR,    "DSP OS iDMA internal error")                       \
  x(CVCORE_STATUS_FAILURE_DSP_IN_INTERRUPT_CONTEXT,   "DSP in interrupt context, invalid operation")      \
  x(CVCORE_STATUS_FAILURE_DSP_STACK_OVERFLOW,         "DSP thread's Stack overflow detected")             \
  x(CVCORE_STATUS_FAILURE_DSP_UNHANDLED_EXCEPTION,    "DSP unhandled exception detected")                 \
  x(CVCORE_STATUS_FAILURE_DSP_WORK_QUEUE_UNAVAILABLE, "DSP work queue feature is unavailable")            \
  x(CVCORE_STATUS_FAILURE_DSP_EXCEPTION,              "DSP exceptions detected previously")               \
  x(CVCORE_STATUS_WM_FAILURE_STATE_SEM_INIT_ERR,      "Wearable Monitor API State SEM Init Error")        \
  x(CVCORE_STATUS_WM_FAILURE_LOCK_INIT_ERR,           "Wearable Monitor API Lock Init Error")             \
  x(CVCORE_STATUS_WM_FAILURE_LOCK_TO,                 "Wearable Monitor API Lock Acquire TO")             \
  x(CVCORE_STATUS_WM_FAILURE_LOCK_ERR,                "Wearable Monitor API Lock Acquire Error")          \
  x(CVCORE_STATUS_WM_FAILURE_REQUEST_QUEUE_FULL,      "Request queue to wearable monitor is full")        \
  x(CVCORE_STATUS_WM_FAILURE_REQUEST_BROADCAST_ERR,   "Failed to broadcast request is avaliable")         \
  x(CVCORE_STATUS_WM_NO_NEW_EVENTS,                   "No new events from wearable monitor")              \
  x(CVCORE_STATUS_WM_FAILURE_EVENT_WAIT_ERR,          "Unexpected event-wait error for wearable monitor") \
  x(CVCORE_STATUS_WM_NO_NEW_REQUESTS,                 "No new requests for wearable monitor")             \
  x(CVCORE_STATUS_WM_FAILURE_REQUEST_EVENT_WAIT_ERR,  "Unexpected request-wait err for wearable monitor") \
  x(CVCORE_STATUS_WM_FAILURE_BOOTING_WEARABLE,        "Wearable failed to boot")                          \
  x(CVCORE_STATUS_WM_FAILURE_EC_GPIO_ACCESS,          "GPIO acces to EC resulted in error")               \
  x(CVCORE_STATUS_WM_LSPCI_DEVICE_NOT_FOUND,          "The lspci device wasn't found")                    \
  x(CVCORE_STATUS_WM_OLD_HARDWARE_FOUND,              "Old Wearable hardware detected")                   \
  x(CVCORE_STATUS_FAILURE_ORION_DCD_NOT_UPTODATE,     "Orion display calibration data is not up-to-date") \
  x(CVCORE_STATUS_FAILURE_SHR_GUARD_BANDS_BAD,        "Shregion guard bands have been violated")          \
  x(CVCORE_STATUS_WM_SECURITY_ISSUE,                  "Security issue prevents wearable boot")            \
  x(CVCORE_STATUS_FAILURE_ZPAD_GUARD_BANDS_BAD,       "Zpad guard bands have been violated")

enum cvcore_status {
    CVCORE_STATUS_LIST(CVCORE_ENUM_DEF)
    CVCORE_STATUS_LAST_ID = 0xffffffff
};

typedef enum cvcore_status CvcoreStatus;

const char *cvcore_status_str(CvcoreStatus status);

#ifndef __KERNEL__
#error "Can't include kernel header in userspace"
#endif
