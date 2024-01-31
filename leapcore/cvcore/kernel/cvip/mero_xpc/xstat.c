/*
 * ---------------------------------------------------------------------
 * %COPYRIGHT_BEGIN%
 *
 * \copyright
 * Copyright (c) 2020-2022 Magic Leap, Inc. (COMPANY) All Rights Reserved.
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

#include "xstat.h"

#include <linux/kthread.h>
#include <linux/device.h>
#include <base/base.h>
#include <linux/ktime.h>

#include "cvcore_xchannel_map.h"
#include "mero_xpc_common.h"
#include "mero_xpc_configuration.h"
#include "xstat_internal.h"

int xstat_get_core_info_internal(uint8_t target_core,
				 struct xstat_core_info *info);
int xstat_send_state_change(union xstat_state_changed state_changed_info);
void xstat_sysfs_notify(enum xstat_sysfs_entry entry);

struct xstat_struct xstat_struct;

static const char *const xstat_sysfs_names[XSTAT_SYSFS_ENTRY_COUNT] = {
	"cvip_state",
	"x86_state",
	"wearable_state",
};

static struct kernfs_node *xstat_sysfs_kn[XSTAT_SYSFS_ENTRY_COUNT];

int xstat_kthread(void *arg)
{
	xpc_debug(1, "%s: Starting xstate kthread\n", __func__);
	xstat_struct.state = THREAD_IDLE;

	// Change state from TASK_RUNNING to TASK_INTERRUPTIBLE
	// so that we can sleep with schedule_timeout
	set_current_state(TASK_INTERRUPTIBLE);
	while (!kthread_should_stop()) {
		// Sleep and wait for someone to wake us up, or timeout
		schedule_timeout(XSTAT_KTHREAD_SLEEP_SEC * HZ);
		spin_lock(&(xstat_struct.lock));

		xpc_debug(3, "%s: Woke up, state: %u wait count: %u\n",
			  __func__, xstat_struct.state,
			  xstat_struct.wait_count);
		// We have woken up. Check if we have a request
		if (xstat_struct.wait_count) {
			// Change state to running
			xstat_struct.state = THREAD_RUNNING;
			xpc_debug(2, "%s: Sending request to %u\n", __func__,
				  xstat_struct.req.target_core);
			spin_unlock(&(xstat_struct.lock));
			// Send request
			xstat_struct.req.ret_code =
				xstat_get_core_info_internal(
					xstat_struct.req.target_core,
					&(xstat_struct.req.core_info));
			spin_lock(&(xstat_struct.lock));
			xpc_debug(
				2,
				"%s: Got response from %u. ret:%u Core state:%u\n",
				__func__, xstat_struct.req.target_core,
				xstat_struct.req.ret_code,
				xstat_struct.req.core_info.core_state);
		}

		// Check if we have to notify of a state change
		if (xstat_struct.state_changed_info.state_changed) {
			int ret;

			xpc_debug(2, "%s: Sending state change message",
				  __func__);
			spin_unlock(&(xstat_struct.lock));
			ret = xstat_send_state_change(
				xstat_struct.state_changed_info);
			spin_lock(&(xstat_struct.lock));
			xpc_debug(2, "%s: Got state change response ret:%u\n",
				  __func__, ret);
			xstat_struct.state_changed_info.state_changed = 0;
		}

		// Set state to idle
		xstat_struct.state = THREAD_IDLE;

		xpc_debug(
			3,
			"%s: Signaling those who are waiting, wait count: %u\n",
			__func__, xstat_struct.wait_count);
		spin_unlock(&(xstat_struct.lock));

		// Wake everyone up to tell them that we are done
		wake_up_interruptible_all(&(xstat_struct.wq));

		set_current_state(TASK_INTERRUPTIBLE);
	}

	xpc_debug(1, "%s: Exiting xstat kthread...\n", __func__);

	// Close thread
	do_exit(0);

	return 0;
}

ssize_t cvip_state_show(struct class *class, struct class_attribute *attr,
			char *buf)
{
	int ret = sprintf(
		buf, "%s\n",
		xstat_state_str(xstat_struct.cvip_core_info.core_state));

	return ret < 0 ? -1 : (ssize_t)ret;
}

ssize_t __attribute__((unused))
cvip_state_store(struct class *class, struct class_attribute *attr,
		 const char *buf, size_t count)
{
	int user_state;

	// Check if this is a matching string
	user_state = sysfs_match_string(xstat_str_list, buf);
	if (user_state < 0) {
		// Not found so parse as an integer
		if (kstrtouint(buf, 10, &user_state) != 0) {
			user_state = XSTAT_STATE_INVALID;
		}
	}

	if (user_state >= XSTAT_STATE_OFFLINE &&
	    user_state < XSTAT_STATE_INVALID) {
		xstat_set_state((enum xstat_state)user_state);
	}

	return count;
}

ssize_t x86_state_show(struct class *class, struct class_attribute *attr,
		       char *buf)
{
	int ret =
		sprintf(buf, "%s\n",
			xstat_state_str(xstat_struct.x86_core_info.core_state));

	return ret < 0 ? -1 : (ssize_t)ret;
}

ssize_t __attribute__((unused))
x86_state_store(struct class *class, struct class_attribute *attr,
		const char *buf, size_t count)
{
	int user_state;

	// Check if this is a matching string
	user_state = sysfs_match_string(xstat_str_list, buf);
	if (user_state < 0) {
		// Not found so parse as an integer
		if (kstrtouint(buf, 10, &user_state) != 0) {
			user_state = XSTAT_STATE_INVALID;
		}
	}

	if (user_state >= XSTAT_STATE_OFFLINE &&
	    user_state < XSTAT_STATE_INVALID) {
		xstat_set_state((enum xstat_state)user_state);
	}

	return count;
}

ssize_t wearable_state_show(struct class *class, struct class_attribute *attr,
			    char *buf)
{
	int ret = sprintf(buf, "%s\n",
			  xstat_wearable_state_str(
				  xstat_struct.cvip_core_info.wearable_state));

	return ret < 0 ? -1 : (ssize_t)ret;
}

ssize_t __attribute__((unused))
wearable_state_store(struct class *class, struct class_attribute *attr,
		     const char *buf, size_t count)
{
	int user_state;

	// Check if this is a matching string
	user_state = sysfs_match_string(xstat_wstate_str_list, buf);
	if (user_state < 0) {
		// Not found so parse as an integer
		if (kstrtouint(buf, 10, &user_state) != 0) {
			user_state = XSTAT_WEARABLE_STATE_INVALID;
		}
	}

	if (user_state >= XSTAT_WEARABLE_STATE_OFFLINE &&
	    user_state < XSTAT_WEARABLE_STATE_INVALID) {
		xstat_set_wearable_state((enum xstat_wearable_state)user_state);
	}

	return count;
}

int xstat_message_handler(union xpc_platform_channel_message *cmd_msg,
			  union xpc_platform_channel_message *rsp_msg)
{
	int ret = 0;

	struct xstat_request *xstat_req;
	struct xstat_response *xstat_rsp;

	xstat_req = (struct xstat_request *)cmd_msg->message_data;
	xstat_rsp = (struct xstat_response *)rsp_msg->message_data;

	// Check magic
	if (xstat_req->key != PING_MAGIC_KEY) {
		// Invalid message
		xpc_debug(1, "%s: Received invalid key:0x%x\n", __func__,
			  xstat_req->key);
		ret = -1;
	} else {
		// Prep response
		rsp_msg->xpc_command_type = XPC_XSTAT_RSP;
		rsp_msg->source_core = XPC_CORE_ID;
		rsp_msg->target_core = cmd_msg->source_core;
		xstat_rsp->key = PONG_MAGIC_KEY;
		if (XPC_CORE_ID == CORE_ID_A55) {
			xstat_rsp->core_info = xstat_struct.cvip_core_info;
		} else {
			xstat_rsp->core_info = xstat_struct.x86_core_info;
		}
	}
	return ret;
}

int xstat_set_state(enum xstat_state state)
{
	enum xstat_state old_state;

	if (state >= XSTAT_STATE_INVALID) {
		xpc_debug(0, "%s: Invalid xstat state: %u\n", __func__, state);
		return -EINVAL;
	}

	if (XPC_CORE_ID == CORE_ID_A55) {
		old_state = xstat_struct.cvip_core_info.core_state;
		xstat_struct.cvip_core_info.core_state = state;
	} else {
		old_state = xstat_struct.x86_core_info.core_state;
		xstat_struct.x86_core_info.core_state = state;
	}

	xpc_debug(0, "%s: Changing xstat state from %s(%u) to %s(%u).\n",
		  __func__, xstat_state_str(old_state), old_state,
		  xstat_state_str(state), state);

	// There are potential race conditions and edge cases where we get here
	// and xpc isn't fully initialized yet, or xpc is in the process of going down.
	// So check if xpc is initialized before continuing.
	if (xpc_initialized()) {
		if (XPC_CORE_ID == CORE_ID_A55) {
			// On the cvip, so notify the cvip sysfs
			xstat_sysfs_notify(XSTAT_CVIP_SYSFS);
			// Set state change flag so that we can signal the other side
			xstat_struct.state_changed_info.cvip_state = 1;
		} else {
			// On the x86, so notify the x86 sysfs
			xstat_sysfs_notify(XSTAT_X86_SYSFS);
			// Set state change flag so that we can signal the other side
			xstat_struct.state_changed_info.x86_state = 1;
		}

		// Wake up the xstat thread
		wake_up_process(xstat_struct.task);
	}

	return 0;
}
EXPORT_SYMBOL(xstat_set_state);

const char *xstat_state_str(enum xstat_state state)
{
	switch (state) {
		XSTAT_STATE_LIST(CVCORE_ENUM_DEF_STR)
	default:
		return "INVALID";
	}
}
EXPORT_SYMBOL(xstat_state_str);

int xstat_set_wearable_state(enum xstat_wearable_state state)
{
	if (state >= XSTAT_WEARABLE_STATE_INVALID) {
		xpc_debug(0, "%s: Invalid wearable state: %u\n", __func__,
			  state);
		return -EINVAL;
	}

	xpc_debug(0, "%s: Changing wearable state from %s(%u) to %s(%u).\n",
		  __func__,
		  xstat_wearable_state_str(
			  xstat_struct.cvip_core_info.wearable_state),
		  xstat_struct.cvip_core_info.wearable_state,
		  xstat_wearable_state_str(state), state);

	xstat_struct.cvip_core_info.wearable_state = state;

	// There are potential race conditions and edge cases where we get here
	// and xpc isn't fully initialized yet, or xpc is in the process of going down.
	// So check if xpc is initialized before continuing.
	if (xpc_initialized()) {
		xstat_sysfs_notify(XSTAT_WEARABLE_SYSFS);
		// Set state change flag so that we can signal the other side
		xstat_struct.state_changed_info.wearable_state = 1;

		// Wake up the xstat thread
		wake_up_process(xstat_struct.task);
	}

	return 0;
}
EXPORT_SYMBOL(xstat_set_wearable_state);

const char *xstat_wearable_state_str(enum xstat_wearable_state state)
{
	switch (state) {
		XSTAT_WEARABLE_STATE_LIST(CVCORE_ENUM_DEF_STR)
	default:
		return "INVALID";
	}
}
EXPORT_SYMBOL(xstat_wearable_state_str);

int xstat_request_cvip_info(struct xstat_core_info *info, uint32_t timeout_ms)
{
	if (info == NULL) {
		return -EINVAL;
	}

	// Check if I'm asking about myself
	if (XPC_CORE_ID == CORE_ID_A55) {
		(*info) = xstat_struct.cvip_core_info;
		return 0;
	}

	return xstat_request_core_info(CORE_ID_A55, info, timeout_ms);
}
EXPORT_SYMBOL(xstat_request_cvip_info);

int xstat_request_x86_info(struct xstat_core_info *info, uint32_t timeout_ms)
{
	if (info == NULL) {
		return -EINVAL;
	}

	// Check if I'm asking about myself
	if (XPC_CORE_ID == CORE_ID_X86) {
		(*info) = xstat_struct.x86_core_info;
		return 0;
	}

	return xstat_request_core_info(CORE_ID_X86, info, timeout_ms);
}
EXPORT_SYMBOL(xstat_request_x86_info);

int xstat_request_core_info(uint8_t target_core, struct xstat_core_info *info,
			    uint32_t timeout_ms)
{
	int ret, status;
	ktime_t start_ts, stop_ts, elapsed_ts;

	if (info == NULL) {
		return -EINVAL;
	}

	/**
	 * The following logic is so that we only have one state request in
	 * progress at any given time.
	 *
	 * The first person that comes in will request for the xstat_kthread
	 * to kick off and send the state request command. Then it will wait for
	 * a response or timeout.
	 * Others that come in during that time will wait for that first state
	 * request to complete and will use the results of that request, or they
	 * will also timeout.
	 *
	 * TODO(sadadi): Right now this logic works because the target_core will be
	 * the same for all the requests that come in. This will have to change if
	 * we want to target different cores.
	 */
	spin_lock(&(xstat_struct.lock));
	if (xstat_struct.state == THREAD_IDLE && xstat_struct.wait_count == 0) {
		// Set target core
		xstat_struct.req.target_core = target_core;
		// Request for the thread to run
		xstat_struct.state = THREAD_RUN_REQUEST;
		xstat_struct.wait_count = 1;
		xpc_debug(2,
			  "%s: New request received, waking xstat kthread!\n",
			  __func__);
		spin_unlock(&(xstat_struct.lock));
		wake_up_process(xstat_struct.task);
	} else {
		// We do nothing, just wait
		xstat_struct.wait_count++;
		xpc_debug(
			2,
			"%s: Request pending. xstat kthread state:%u, waiting list "
			"count: %u\n",
			__func__, xstat_struct.state, xstat_struct.wait_count);
		spin_unlock(&(xstat_struct.lock));
	}

	// Wait until signaled or timeout
	if (timeout_ms == 0) {
		timeout_ms = XSTAT_DEFAULT_TIMEOUT_MS;
	}

	start_ts = ktime_get();
	status = wait_event_interruptible_timeout(
		xstat_struct.wq, xstat_struct.state == THREAD_IDLE,
		timeout_ms * HZ / 1000);
	stop_ts = ktime_get();
	elapsed_ts = ktime_sub(stop_ts, start_ts);

	spin_lock(&(xstat_struct.lock));
	// Check if done or other error or timeout
	if (xstat_struct.state == THREAD_IDLE) {
		// Thread finished
		*info = xstat_struct.req.core_info;
		ret = xstat_struct.req.ret_code;
	} else if (status == -ERESTARTSYS) {
		// We got interrupted
		ret = -ERESTARTSYS;
	} else {
		// Timed out
		ret = -ETIMEDOUT;
	}

	xstat_struct.wait_count--;
	xpc_debug(
		2,
		"%s: Done waiting, ret:%d, xstat kthread state:%u, waiting list "
		"count: %u, wait time: %lld us\n",
		__func__, ret, xstat_struct.state, xstat_struct.wait_count,
		ktime_to_us(elapsed_ts));

	spin_unlock(&(xstat_struct.lock));

	return ret;
}

int xstat_get_core_info_internal(uint8_t target_core,
				 struct xstat_core_info *info)
{
	int result;
	union xpc_platform_channel_message cmd_msg;
	union xpc_platform_channel_message rsp_msg;
	struct xstat_request *xstat_req;
	struct xstat_response *xstat_rsp;
	uint8_t source_core;
	size_t rsp_length;

	if (info == NULL) {
		return -EINVAL;
	}

	// Interpret generic message payload as xstat request/response
	xstat_req = (struct xstat_request *)cmd_msg.message_data;
	xstat_rsp = (struct xstat_response *)rsp_msg.message_data;

	// Set source/target based on current core
	source_core = XPC_CORE_ID;

	// Fill out state request message
	cmd_msg.xpc_command_type = XPC_XSTAT_REQ;
	cmd_msg.source_core = source_core;
	cmd_msg.target_core = target_core;
	xstat_req->key = PING_MAGIC_KEY;

	// Send message
	result = xpc_command_send(
		CVCORE_XCHANNEL_PLATFORM, target_core, cmd_msg.bytes,
		sizeof(union xpc_platform_channel_message), rsp_msg.bytes,
		sizeof(union xpc_platform_channel_message), &rsp_length);
	if (result != 0) {
		xpc_debug(
			1,
			"%s: Failed to send state request message to %u. Status:%u\n",
			__func__, target_core, result);
		return -ECOMM;
	}

	// Do some checking
	if (rsp_msg.xpc_command_type != XPC_XSTAT_RSP) {
		xpc_debug(0, "%s: Received invalid message type:%u\n", __func__,
			  rsp_msg.xpc_command_type);
		return -EBADMSG;
	}
	if (rsp_msg.target_core != source_core) {
		xpc_debug(
			0,
			"%s: Received message with invalid target. Expected:%u Got:%u\n",
			__func__, source_core, rsp_msg.target_core);
		return -EBADMSG;
	}
	if (rsp_msg.source_core != target_core) {
		xpc_debug(
			0,
			"%s: Received message with invalid source Expected:%u Got:%u\n",
			__func__, target_core, rsp_msg.source_core);
		return -EBADMSG;
	}
	if (xstat_rsp->key != PONG_MAGIC_KEY) {
		xpc_debug(0, "%s: Received invalid key:0x%x\n", __func__,
			  xstat_rsp->key);
		return -EBADMSG;
	}

	xpc_debug(1, "%s: Got state response from %u, core_state: %u\n",
		  __func__, rsp_msg.source_core,
		  xstat_rsp->core_info.core_state);

	*info = xstat_rsp->core_info;

	return 0;
}

int xstat_state_change_handler(union xpc_platform_channel_message *cmd_msg,
			       union xpc_platform_channel_message *rsp_msg)
{
	int ret = 0;
	enum xstat_sysfs_entry xstat_sysfs_entry;

	struct xstat_state_change_cmd *xstat_cmd;
	struct xstat_state_change_rsp *xstat_rsp;

	xstat_cmd = (struct xstat_state_change_cmd *)cmd_msg->message_data;
	xstat_rsp = (struct xstat_state_change_rsp *)rsp_msg->message_data;

	// Check magic
	if (xstat_cmd->key != PING_MAGIC_KEY) {
		// Invalid message
		xpc_debug(1, "%s: Received invalid key:0x%x\n", __func__,
			  xstat_cmd->key);
		ret = -1;
	} else {
		xpc_debug(1, "%s: Received state change message from %s.\n",
			  __func__,
			  cmd_msg->source_core == CORE_ID_A55 ? "cvip" : "x86");

		// Prep response
		rsp_msg->xpc_command_type = XPC_XSTAT_STATE_CHANGE_RSP;
		rsp_msg->source_core = XPC_CORE_ID;
		rsp_msg->target_core = cmd_msg->source_core;
		xstat_rsp->key = PONG_MAGIC_KEY;

		if (XPC_CORE_ID == CORE_ID_A55 &&
		    xstat_cmd->state_changed_info.x86_state) {
			// On the cvip, so set the x86 state and notify the x86 sysfs
			xstat_struct.x86_core_info.core_state =
				xstat_cmd->core_info.core_state;
			xstat_sysfs_entry = XSTAT_X86_SYSFS;
			xpc_debug(1, "%s: x86 state changed to %s(%u).\n",
				  __func__,
				  xstat_state_str(
					  xstat_cmd->core_info.core_state),
				  xstat_cmd->core_info.core_state);
		}
		if (XPC_CORE_ID == CORE_ID_X86 &&
		    xstat_cmd->state_changed_info.cvip_state) {
			// On the x86, so set the cvip state and notify the cvip sysfs
			xstat_struct.cvip_core_info.core_state =
				xstat_cmd->core_info.core_state;
			xstat_sysfs_entry = XSTAT_CVIP_SYSFS;
			xpc_debug(1, "%s: cvip state changed to %s(%u).\n",
				  __func__,
				  xstat_state_str(
					  xstat_cmd->core_info.core_state),
				  xstat_cmd->core_info.core_state);
		}
		if (xstat_cmd->state_changed_info.wearable_state) {
			// Set the wearable state and notify the wearable sysfs
			xstat_struct.cvip_core_info.wearable_state =
				xstat_cmd->core_info.wearable_state;
			xstat_sysfs_entry = XSTAT_WEARABLE_SYSFS;
			xpc_debug(1, "%s: wearable state changed to %s(%u).\n",
				  __func__,
				  xstat_wearable_state_str(
					  xstat_cmd->core_info.wearable_state),
				  xstat_cmd->core_info.wearable_state);
		}

		// There are potential race conditions and edge cases where we get here
		// and xpc isn't fully initialized yet, or xpc is in the process of going down.
		// So check if xpc is initialized before signaling.
		if (xpc_initialized()) {
			xstat_sysfs_notify(xstat_sysfs_entry);
		}
	}
	return ret;
}

int xstat_send_state_change(union xstat_state_changed state_changed_info)
{
	int result;
	union xpc_platform_channel_message cmd_msg;
	union xpc_platform_channel_message rsp_msg;
	struct xstat_state_change_cmd *xstat_cmd;
	struct xstat_state_change_rsp *xstat_rsp;
	uint8_t source_core;
	uint8_t target_core;
	size_t rsp_length;

	// Interpret generic message payload as xstat request/response
	xstat_cmd = (struct xstat_state_change_cmd *)cmd_msg.message_data;
	xstat_rsp = (struct xstat_state_change_rsp *)rsp_msg.message_data;

	// Set source/target based on current core
	source_core = XPC_CORE_ID;
	if (source_core == CORE_ID_A55) {
		target_core = CORE_ID_X86;
		xstat_cmd->core_info = xstat_struct.cvip_core_info;
	} else {
		target_core = CORE_ID_A55;
		xstat_cmd->core_info = xstat_struct.x86_core_info;
	}

	// Fill out state request message
	cmd_msg.xpc_command_type = XPC_XSTAT_STATE_CHANGE_CMD;
	cmd_msg.source_core = source_core;
	cmd_msg.target_core = target_core;
	xstat_cmd->key = PING_MAGIC_KEY;
	xstat_cmd->state_changed_info = state_changed_info;

	// Send message
	result = xpc_command_send(
		CVCORE_XCHANNEL_PLATFORM, target_core, cmd_msg.bytes,
		sizeof(union xpc_platform_channel_message), rsp_msg.bytes,
		sizeof(union xpc_platform_channel_message), &rsp_length);
	if (result != 0) {
		xpc_debug(
			1,
			"%s: Failed to send state change message to %u. Status:%u\n",
			__func__, target_core, result);
		return -ECOMM;
	}

	// Do some checking
	if (rsp_msg.xpc_command_type != XPC_XSTAT_STATE_CHANGE_RSP) {
		xpc_debug(0, "%s: Received invalid message type:%u\n", __func__,
			  rsp_msg.xpc_command_type);
		return -EBADMSG;
	}
	if (rsp_msg.target_core != source_core) {
		xpc_debug(
			0,
			"%s: Received message with invalid target. Expected:%u Got:%u\n",
			__func__, source_core, rsp_msg.target_core);
		return -EBADMSG;
	}
	if (rsp_msg.source_core != target_core) {
		xpc_debug(
			0,
			"%s: Received message with invalid source Expected:%u Got:%u\n",
			__func__, target_core, rsp_msg.source_core);
		return -EBADMSG;
	}
	if (xstat_rsp->key != PONG_MAGIC_KEY) {
		xpc_debug(0, "%s: Received invalid key:0x%x\n", __func__,
			  xstat_rsp->key);
		return -EBADMSG;
	}

	xpc_debug(1, "%s: Got state change response from %u\n", __func__,
		  rsp_msg.source_core);

	return 0;
}

void xstat_sysfs_notify(enum xstat_sysfs_entry entry)
{
	if (xstat_sysfs_kn[entry] != NULL) {
		// Note: sysfs_notify_dirent is safe to call from irq context
		sysfs_notify_dirent(xstat_sysfs_kn[entry]);
	}
}

int xstat_init(struct class *xpc_class)
{
	int ret = 0;
	int i;

	// We need the kobj that the sysfs attribute file was created with in order
	// to call sysfs_get_dirent.
	// The struct class object has a private structure called subsys_private
	// that has a kset(subsys) that contains this kobj.
	// I'm not sure if there's a cleaner way to get a reference to that kobj.
	struct kernfs_node *class_kn = xpc_class->p->subsys.kobj.sd;

	// Grab the kernel nodes for each of our entries
	for (i = 0; i < XSTAT_SYSFS_ENTRY_COUNT; i++) {
		xstat_sysfs_kn[i] =
			sysfs_get_dirent(class_kn, xstat_sysfs_names[i]);
		if (xstat_sysfs_kn[i] == NULL) {
			// Error getting the kernel node
			xpc_debug(0, "%s: sysfs_get_dirent is NULL for %s\n",
				  __func__, xstat_sysfs_names[i]);
			ret = -1;
			break;
		}
	}

	// sysfs_get_dirent will grab a reference to the sysfs if it succeeds,
	// so put back the reference if we encountered an error.
	if (ret) {
		for (i = 0; i < XSTAT_SYSFS_ENTRY_COUNT; i++) {
			if (xstat_sysfs_kn[i] != NULL) {
				sysfs_put(xstat_sysfs_kn[i]);
				xstat_sysfs_kn[i] = NULL;
			}
		}
	}

	return ret;
}
