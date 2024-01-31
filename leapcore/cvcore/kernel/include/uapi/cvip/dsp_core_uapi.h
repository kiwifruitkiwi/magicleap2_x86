/*
 * ---------------------------------------------------------------------
 * %COPYRIGHT_BEGIN%
 *
 * \copyright
 * Copyright (c) 2020-2023 Magic Leap, Inc. (COMPANY) All Rights Reserved.
 * Magic Leap, Inc. Confidential and Proprietary
 *
 *  NOTICE:  All information contained herein is, and remains the property
 *  of COMPANY. The intellectual and technical concepts contained herein
 *  are proprietary to COMPANY and may be covered by U.S. and Foreign
 *  Patents, patents in process, and are protected by trade secret or
 *  copyright law.  Dissemination of this information or reproduction of
 *  this material is strictly forbidden unless prior written permission is
 *  obtained from COMPANY.  Access to the source code contained herein is
 *  hereby forbidden to anyone except current COMPANY employees, managers
 *  or contractors who have executed Confidentiality and Non-disclosure
 *  agreements explicitly covering such access.
 *
 *  The copyright notice above does not evidence any actual or intended
 *  publication or disclosure  of  this source code, which includes
 *  information that is confidential and/or proprietary, and is a trade
 *  secret, of  COMPANY.   ANY REPRODUCTION, MODIFICATION, DISTRIBUTION,
 *  PUBLIC  PERFORMANCE, OR PUBLIC DISPLAY OF OR THROUGH USE  OF THIS
 *  SOURCE CODE  WITHOUT THE EXPRESS WRITTEN CONSENT OF COMPANY IS
 *  STRICTLY PROHIBITED, AND IN VIOLATION OF APPLICABLE LAWS AND
 *  INTERNATIONAL TREATIES.  THE RECEIPT OR POSSESSION OF  THIS SOURCE
 *  CODE AND/OR RELATED INFORMATION DOES NOT CONVEY OR IMPLY ANY RIGHTS
 *  TO REPRODUCE, DISCLOSE OR DISTRIBUTE ITS CONTENTS, OR TO MANUFACTURE,
 *  USE, OR SELL ANYTHING THAT IT  MAY DESCRIBE, IN WHOLE OR IN PART.
 *
 * %COPYRIGHT_END%
 * --------------------------------------------------------------------
 */

#ifndef __DSP_CORE_UAPI_H
#define __DSP_CORE_UAPI_H

#include <stdalign.h>

#define DEVICE_NAME "dsp_core"
#define DSP_CORE_MAGIC 'x'
#define DSP_CORE_FILE "/dev/dsp_core"

enum built_in_task_id {
	kBuiltInTaskID_00 = 0,
	kBuiltInTaskID_01,
	kBuiltInTaskID_02,
	kBuiltInTaskID_03,
	kBuiltInTaskID_04,
	kBuiltInTaskID_05,
	kBuiltInTaskID_06,
	kBuiltInTaskID_07,
	kBuiltInTaskID_08,
	kBuiltInTaskID_09,
	kBuiltInTaskID_10,
	kBuiltInTaskID_11,
	kBuiltInTaskID_12,
	kBuiltInTaskID_13,
	kBuiltInTaskID_14,
	kBuiltInTaskID_15,
	kBuiltInTaskID_WQ_MANAGER,
	kBuiltInTaskID_WQ_EXECUTOR,
	kBuiltInTaskID_LAST,
	kBuiltInTaskID_UNDEFINED = 0xFF
};

enum built_in_delegate_id {
	kBuiltInDelegateID_00 = 0,
	kBuiltInDelegateID_01,
	kBuiltInDelegateID_02,
	kBuiltInDelegateID_03,
	kBuiltInDelegateID_04,
	kBuiltInDelegateID_05,
	kBuiltInDelegateID_06,
	kBuiltInDelegateID_07,
	kBuiltInDelegateID_08,
	kBuiltInDelegateID_09,
	kBuiltInDelegateID_10,
	kBuiltInDelegateID_11,
	kBuiltInDelegateID_12,
	kBuiltInDelegateID_13,
	kBuiltInDelegateID_14,
	kBuiltInDelegateID_15,
	kBuiltInDelegateID_LAST,
	kBuiltInDelegateID_UNDEFINED = 0xFF
};

enum task_creation_parameters {
	kTaskCreateParamsNone = 0x0,
	kCreateSuspended = 0x1 << 0,
	kCreateDetached = 0x1 << 1,
	kIsExecutiveTask = 0x1 << 2,
	kCanExecuteDelegate = 0x1 << 3,
	kIsWorkQueueManagerD2 = 0x1 << 4,
	kIsWorkQueueManagerD4 = 0x1 << 5,
	kIsWorkQueueManagerD8 = 0x1 << 6,
	kIsWorkQueueExecutor = 0x1 << 7
};

struct dsp_core_task_locator {
	/** The core on which the task executes. */
	uint8_t core_id;
	/** A core unique id that identifies an active task. */
	uint8_t unique_id;
};

enum dsp_core_task_manager_capabilities {
	kTaskManagerCapabilitiesNone = 0x0,
	kHasThreading = 0x1 << 0,
	kHasPriorities = 0x1 << 1,
	kHasTaskAbort = 0x2 << 2
};

enum dsp_core_task_run_state {
	kTaskRunStateReady = 0x1,
	kTaskRunStateStarting,
	kTaskRunStateStarted,
	kTaskRunStateRunning,
	kTaskRunStateSuspending,
	kTaskRunStateSuspended,
	kTaskRunStateResuming,
	kTaskRunStateExiting,
	kTaskRunStateExited,
	kTaskRunStateScheduled,
	kTaskRunStateAborting,
	kTaskRunStateAborted,
	kTaskRunStateInvalid,
	kTaskRunStateUnknown
};

struct dsp_core_task_manager_info {
	struct dsp_core_task_locator task_locator_first;
	struct dsp_core_task_locator task_locator_last;
	enum dsp_core_task_manager_capabilities capabilities;
	uint8_t max_tasks;
	uint8_t num_executive_tasks;
	uint8_t num_user_tasks;
	uint8_t num_resident_executive_tasks;
	uint8_t num_resident_user_tasks;
	uint8_t num_non_resident_executive_tasks;
	uint8_t num_non_resident_user_tasks;
	uint8_t num_built_in_tasks;
	uint8_t num_built_in_delegates;
};

struct DspCoreBindRequest {
	uint32_t stream_id;
	uint16_t requested_cores;
};

struct DspCoreStreamInfoRequest {
	uint32_t stream_id;
	int num_task_managers;
	bool is_bound;
};

struct DspCoreGetInfoRequest {
	uint8_t core_id;
	struct dsp_core_task_manager_info info;
};

struct TaskManagerCreateTaskRequest {
	uint8_t core_id;
	uint64_t ptr_to_creator;
	enum built_in_task_id task_id;
	uint64_t unique_id;
	uint8_t priority;
	uint32_t stack_size;
	enum task_creation_parameters params;
};

struct TaskControlFindAndAttachRequest {
	uint16_t core_mask;
	uint64_t unique_id;
};

struct TaskControlStartRequest {
	/** If specified, a named event on the DSP that will trigger the start of the task. */
	uint64_t trigger_event_name; //8-bytes

	/** An application specific handle to arguments to pass to the task at start. */
	uint64_t arguments; //8-bytes

	/** The max time in microseconds that the task is allowed to run. Zero for indefinite.*/
	uint32_t timeout_us; //4-bytes

	/** The priority level to run the task. Priority can range from 1 to TASK_MAX_PRIORITY
	 ** with 0 being the least urgent. */
	uint8_t priority; //1-byte

	/** Task flags to use on start */
	uint8_t flags; //1-byte
};

struct TaskControlPriorityInfo {
	/** The priority level. Priority can range from 1 to TASK_MAX_PRIORITY
	 ** with 0 being the least urgent. */
	uint8_t priority;
};

struct TaskControlTaskStateInfo {
	/** The value of the exit code is application defined
	 ** and reflects the value returned from a task's run function. */
	uint32_t exit_code;

	/** The current state of the task. */
	enum dsp_core_task_run_state run_state;
};

struct TaskControlTaskWaitStateInfo {
	/** The state of the task to wait on. */
	enum dsp_core_task_run_state wait_state;

	/** The maximum time in useconds to wait. */
	uint64_t timeout_us;

	/** The value of the exit code is application defined
	 ** and reflects the value returned from a task's run function. */
	uint32_t current_exit_code;

	/** The current state of the task. */
	enum dsp_core_task_run_state current_run_state;
};

//Driver needs to check for valid stream_id
#define DSP_CORE_BIND_STREAM_ID _IOWR(DSP_CORE_MAGIC, 0xA0, struct DspCoreBindRequest)
#define DSP_CORE_STREAM_CLIENT_GET_INFO _IOR(DSP_CORE_MAGIC, 0xA1, struct DspCoreStreamInfoRequest)
#define DSP_CORE_TASK_MANAGER_GET_INFO _IOR(DSP_CORE_MAGIC, 0xA2, struct DspCoreGetInfoRequest)
#define DSP_CORE_TASK_MANAGER_CREATE_TASK _IOW(DSP_CORE_MAGIC, 0xA3, struct TaskManagerCreateTaskRequest)
#define DSP_CORE_TASK_CONTROL_FIND_AND_ATTACH_TASK _IOW(DSP_CORE_MAGIC, 0xA4, struct TaskControlFindAndAttachRequest)
#define DSP_CORE_TASK_CONTROL_START_TASK _IOW(DSP_CORE_MAGIC, 0xA5, struct TaskControlStartRequest)
#define DSP_CORE_TASK_CONTROL_WAIT_TASK_STATE _IOWR(DSP_CORE_MAGIC, 0xA6, struct TaskControlTaskWaitStateInfo)
#define DSP_CORE_TASK_CONTROL_ABORT_TASK _IO(DSP_CORE_MAGIC, 0xA7)
#define DSP_CORE_TASK_CONTROL_SET_TASK_PRIORITY _IOWR(DSP_CORE_MAGIC, 0xA8, struct TaskControlPriorityInfo)
#define DSP_CORE_TASK_CONTROL_GET_TASK_PRIORITY _IOR(DSP_CORE_MAGIC, 0xA9, struct TaskControlPriorityInfo)
#define DSP_CORE_TASK_CONTROL_GET_TASK_STATE _IOR(DSP_CORE_MAGIC, 0xAA, struct TaskControlTaskStateInfo)

#endif
