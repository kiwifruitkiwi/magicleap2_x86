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

#include "mero_xpc_command.h"

#include <linux/uaccess.h>
#include <linux/wait.h>
#include <linux/workqueue.h>

#include "cvcore_processor_common.h"
#include "cvcore_xchannel_map.h"
#include "mero_xpc_common.h"
#include "mero_xpc_configuration.h"
#include "mero_xpc_kapi.h"
#include "mero_xpc_types.h"
#include "mero_xpc_uapi.h"
#include "pl320_hal.h"
#include "pl320_types.h"
#include "xstat_internal.h"

XpcCommandHandler command_handlers[XPC_NUM_VCHANNELS] = { NULL };
XpcHandlerArgs command_handler_args[XPC_NUM_VCHANNELS];
bool command_handlers_hi[XPC_NUM_VCHANNELS] = { false };
struct xpc_queue command_mailbox[XPC_NUM_VCHANNELS];

static void xpc_command_save_wait_point(XpcAsyncTicket ticket);
static bool xpc_command_load_wait_point(struct XpcCommandInfo *message);

static bool any_interrupted_waits = false;
spinlock_t command_waits_lock;
/////////////////////////////////////////////////
// Internal helper functions
/////////////////////////////////////////////////

bool xpc_command_channel_is_hi_priority(int channel)
{
	return ((command_handlers[channel] != NULL) &&
		command_handlers_hi[channel]);
}

static bool xpc_command_load_wait_point(struct XpcCommandInfo *message)
{
	unsigned long flags;
	bool loaded = false;
	bool interrupted_waits = false;
	int d, i, global_idx;
	struct xpc_mailbox *mb_ptr;

	spin_lock_irqsave(&command_waits_lock, flags);
	if (!any_interrupted_waits) {
		spin_unlock_irqrestore(&command_waits_lock, flags);
		return false;
	}

	message->ticket = XPC_TICKET_INVALID;

	for (d = 0; d < XPC_NUM_IPC_DEVICES; ++d) {
		if (test_bit(d, &enabled_mailboxes) == 0) {
			continue;
		}

		for (i = 0; i < XPC_IPC_NUM_MAILBOXES; ++i) {
			xpc_get_global_mailbox_idx_from_ipc_id_and_mailbox_idx(
				d, i, &global_idx);

			mb_ptr = xpc_get_global_mailbox_from_global_mailbox_idx(
				global_idx);
			/*
			 * Never expect mb_ptr to be NULL. Not much we can do
			 * to handle it, so let's BUG.
			 */
			BUG_ON(mb_ptr == NULL);

			if (mb_ptr->mailbox_info.source_state ==
			    MAILBOX_STATE_COMMAND_WAIT_INTERRUPTED) {
				/* At least one interrupted mailbox */
				interrupted_waits = true;

				if (mb_ptr->mailbox_info.source_pid ==
				    current->pid) {
					message->ticket = global_idx;
					mb_ptr->mailbox_info.source_state =
						MAILBOX_STATE_COMMAND_WAIT_RESPONSE;
					loaded = true;
					xpc_debug(1,
						  "Found restart mb[%d][%d]\n",
						  d, i);
					break;
				}
			}
		}
	}

	if (!interrupted_waits) {
		any_interrupted_waits = false;
	}
	spin_unlock_irqrestore(&command_waits_lock, flags);

	return loaded;
}

static void xpc_command_save_wait_point(XpcAsyncTicket ticket)
{
	unsigned long flags;
	struct xpc_mailbox *mb_ptr;

	mb_ptr = xpc_get_global_mailbox_from_global_mailbox_idx(ticket);
	/*
	 * Never expect mb_ptr to be NULL. Not much we can do
	 * to handle it, so let's BUG.
	 */
	BUG_ON(mb_ptr == NULL);

	mb_ptr->mailbox_info.source_state =
		MAILBOX_STATE_COMMAND_WAIT_INTERRUPTED;
	mb_ptr->mailbox_info.source_restarts++;

	spin_lock_irqsave(&command_waits_lock, flags);
	any_interrupted_waits = true;
	spin_unlock_irqrestore(&command_waits_lock, flags);
}

/**
 * xpc_send_command_internal - Handles low-level sending of
 * message across an IPC device.  Determines the
 * required IPC device, acquires an available mailbox within
 * the IPC, updates the Kernel-resident mailbox state, then
 * sends the message.
 */
int xpc_send_command_internal(struct XpcCommandInfo *message,
			      bool expect_response,
			      enum xpc_client_api_mode client_mode)
{
	struct xpc_mailbox_info mailbox_info;
	XpcMessageHeader *header;
	Pl320HalStatus pl320_status;
	int status;
	struct xpc_route_solution route_solution;
	int mb_idx;

	if (message->length > XPC_MAX_COMMAND_LENGTH) {
		return -EINVAL;
	}

	if (xpc_command_load_wait_point(message)) {
		/**
		 * A previous wait was restarted so return immediately,
		 * skipping the send portion of command initiation.
		 */
		return 0;
	}

	// Set THIS core as the source of the command
	message->src_id = XPC_CORE_ID;

	if (xpc_compute_route(message->dst_mask, &route_solution) == 0) {
		return -EINVAL;
	}

	/* Initialize a pl320 struct so we can acquire a mailbox. */
	PL320_MAILBOX_INFO_STRUCT_INIT(mailbox_info.pl320_info);
	mailbox_info.pl320_info.mailbox_mode = PL320_MAILBOX_MODE_DEFAULT;
	mailbox_info.pl320_info.ipc_id = route_solution.first_route.ipc_id;
	mailbox_info.pl320_info.ipc_irq_id =
		mask_32b_to_idx(route_solution.first_route.ipc_source_irq_mask);

	/* Attempt to acquire a pl320 mailbox */
	pl320_status = pl320_acquire_mailbox(
		&mailbox_info.pl320_info,
		route_solution.first_route.ipc_target_irq_mask);

	// Convert to Linux errno
	status = xpc_hal_status_to_errno(pl320_status);
	if (status) {
		return status;
	}

	header = (XpcMessageHeader *)&(message->data[XPC_MAILBOX_HEADER_LOC]);
	header->fields.mode = XPC_MODE_COMMAND_RESPONSE;
	header->fields.rsvd = 0;
	header->fields.error = 0;
	header->fields.vchannel = message->channel;
	message->length = XPC_MAILBOX_DATA_SIZE;

	/**
	 * Store the global mailbox id into the ticket field.  Userspace
	 * calls will reference this ticket number in subsequent calls
	 * related to this send.
	 */
	message->ticket = mb_idx = xpc_get_global_mailbox(&mailbox_info);

	// Prepare to be notified on completion
	reinit_completion(&(mailboxes[mb_idx].target_replied));

	// Initialize a global mailbox struct with our local initializations.
	mailboxes[mb_idx].mailbox_info.pl320_info = mailbox_info.pl320_info;
	mailboxes[mb_idx].mailbox_info.source_state =
		expect_response ? MAILBOX_STATE_COMMAND_SENT :
				  MAILBOX_STATE_COMMAND_WAIT_NO_RESPONSE;

	xpc_set_global_mailbox_pid(mb_idx, current->pid, XPC_REQUEST_IS_SOURCE,
				   client_mode);

	// Finally we send through the hardware mailbox!
	pl320_status =
		pl320_send(&(mailboxes[mb_idx].mailbox_info.pl320_info),
			   route_solution.first_route.ipc_target_irq_mask,
			   message->data, message->length);

	return xpc_hal_status_to_errno(pl320_status);
}

/**
 * xpc_wait_command_response_internal - Handles low-level
 * sleep waiting on the arrival of a response from a previous
 * command send. The association of the response and the
 * previous send is through the "global_mailbox_number".
 * This routine copies response data into a buffer for access
 * by a client, taking care of userspace access as necessary.
 */
int xpc_wait_command_response_internal(struct XpcResponseInfo *message)
{
	int status;
	int mb_idx;
	unsigned long flags;
	XpcMessageHeader header;
	uint16_t length;
	mb_idx = message->ticket;

	// Record the we're in a state waiting for a response from
	// a previously issued command
	mailboxes[mb_idx].mailbox_info.source_state =
		MAILBOX_STATE_COMMAND_WAIT_RESPONSE;
	/*
	 * Only one client process/thread should be waiting on a
	 * response (i.e. the one that sent the command). This
	 * call sleep waits until a response message arrives.
	 */
	status = xpc_wait_mailbox_reply(mb_idx, &(message->timeout_us));
	if (status == -ERESTARTSYS) {
		xpc_command_save_wait_point(message->ticket);
		return -ERESTARTSYS;
	}

	// TODO(kirick): consider the removal of this spinlock since there
	// should only be a single waiter of the response.
	spin_lock_irqsave(&(mailboxes[mb_idx].mailbox_lock), flags);

	if (status == 0) {
		// Copy mailbox contents into a command response buffer
		memcpy((void *)message->data,
		       (void *)mailboxes[mb_idx].mailbox_data.u8,
		       XPC_MAX_RESPONSE_LENGTH);
		message->length = XPC_MAX_RESPONSE_LENGTH;
	} else if (status == -ETIME) {
		xpc_debug(
			0,
			"Failed wating for reply - forcefully closing mailbox\n");
	}

	// Check for error bit in header
	pl320_read_mailbox_data(&(mailboxes[mb_idx].mailbox_info.pl320_info),
				XPC_MAILBOX_HEADER_LOC,
				//Last byte in the mailbox is the header
				(uint8_t *)&header, sizeof(XpcMessageHeader),
				&length);

	xpc_clr_global_mailbox_pid(mb_idx, XPC_REQUEST_IS_SOURCE);

	// Close up shop, we're finished with the mailbox.
	xpc_close_global_mailbox(mb_idx);

	spin_unlock_irqrestore(&(mailboxes[mb_idx].mailbox_lock), flags);

	return header.fields.error ? -EREMOTEIO : status;
}

/**
 * xpc_send_command_response_internal - Handles low-level
 * formatting and sending of response messages.
 *
 * The client response portion of the reply data is always
 * expected to be XPC_MAX_RESPONSE_LENGTH even if the client
 * doesn't fill all of the bytes. The last byte just after the
 * response portion represents the XPC header. The length sent
 * down to the pl320 layer is thus XPC_MAX_RESPONSE_LENGTH + 1
 *
 */
int xpc_send_command_response_internal(struct XpcResponseInfo *message)
{
	Pl320HalStatus status;
	XpcMessageHeader *header;
	int mb_idx = message->ticket;

	// Validate parameters
	if (message->length > XPC_MAX_RESPONSE_LENGTH) {
		return -EINVAL;
	}

	if (mb_idx > NUM_MAILBOXES) {
		return -EINVAL;
	}

	header = (XpcMessageHeader *)&message->data[XPC_MAILBOX_HEADER_LOC];
	header->byte_ = 0;
	header->fields.rsvd = 0;
	header->fields.error = message->has_error ? 1 : 0;
	header->fields.vchannel = mailboxes[mb_idx].mailbox_info.xpc_channel;
	header->fields.mode = XPC_MODE_COMMAND_RESPONSE;

	mailboxes[mb_idx].mailbox_info.target_state =
		MAILBOX_STATE_COMMAND_RESPONDED;

	xpc_clr_global_mailbox_pid(mb_idx, XPC_REQUEST_IS_TARGET);

	status = pl320_reply(&(mailboxes[mb_idx].mailbox_info.pl320_info),
			     message->data, XPC_MAILBOX_DATA_SIZE);

	return xpc_hal_status_to_errno(status);
}

/**
 * xpc_platform_channel_command_handler - This platform level
 * command handler is called for platform channel messages.
 * Certain platform channel messages (i.e. XSTAT) are handled
 * by this driver. If the message type is not handled, then
 * this routine returns an error.  The caller then propagates
 * the message to other registered handlers, if any.
 */
int xpc_platform_channel_command_handler(
	XpcChannel channel, XpcHandlerArgs args, uint8_t *command_buffer,
	size_t command_buffer_length, uint8_t *response_buffer,
	size_t response_buffer_length, size_t *response_length)
{
	int ret = 0;
	union xpc_platform_channel_message *cmd_msg =
		(union xpc_platform_channel_message *)command_buffer;
	union xpc_platform_channel_message *rsp_msg =
		(union xpc_platform_channel_message *)response_buffer;

	xpc_debug(1, "%s: Got message on channel: %u, type: %u\n", __func__,
		  channel, cmd_msg->xpc_command_type);

	switch (cmd_msg->xpc_command_type) {
	case XPC_XSTAT_REQ:
		ret = xstat_message_handler(cmd_msg, rsp_msg);
		break;
	case XPC_XSTAT_STATE_CHANGE_CMD:
		ret = xstat_state_change_handler(cmd_msg, rsp_msg);
		break;
	default:
		xpc_debug(1, "%s: Got message of unknown type:%u\n", __func__,
			  cmd_msg->xpc_command_type);
		ret = -1;
		break;
	}

	*response_length = sizeof(union xpc_platform_channel_message);
	return ret;
}

/////////////////////////////////////////////////
// Public API functions
/////////////////////////////////////////////////

int xpc_register_command_handler(XpcChannel channel, XpcCommandHandler handler,
				 XpcHandlerArgs args)
{
	if (channel >= XPC_NUM_VCHANNELS) {
		return -EINVAL;
	}

	command_handlers_hi[(int)channel] = false;
	command_handlers[(int)channel] = handler;
	command_handler_args[(int)channel] = args;
	return 0;
}
EXPORT_SYMBOL(xpc_register_command_handler);

int xpc_register_command_handler_hi(XpcChannel channel,
				    XpcCommandHandler handler,
				    XpcHandlerArgs args)
{
	int status;
	status = xpc_register_command_handler(channel, handler, args);
	if (status == 0) {
		command_handlers_hi[(int)channel] =
			(handler == NULL) ? false : true;
	}
	return status;
}
EXPORT_SYMBOL(xpc_register_command_handler_hi);

int xpc_command_wait_response_async_timed(XpcAsyncTicket *ticket,
					  uint8_t *response_buffer,
					  size_t response_buffer_length,
					  size_t *response_length,
					  int timeout_us)
{
	int status = 0;
	struct XpcResponseInfo response;

	if (response_buffer_length < XPC_MAX_RESPONSE_LENGTH) {
		return -EINVAL;
	}

	if (response_length == NULL || response_buffer == NULL) {
		return -EINVAL;
	}

	if (ticket == NULL || *ticket < 0 || *ticket >= NUM_MAILBOXES) {
		return -EINVAL;
	}

	response.ticket = *ticket;
	response.timeout_us = timeout_us;
	response.length = response_buffer_length;

	status = xpc_wait_command_response_internal(&response);
	if (status == -1) {
		return -ETIMEDOUT;
	}

	if (status == -ERESTARTSYS) {
		xpc_debug(
			1,
			"xpc_command_wait_response_async_timed - restart_sys\n");
		return -ERESTARTSYS;
	}

	memcpy(response_buffer, (void *)response.data, response.length);

	*response_length = XPC_MAX_RESPONSE_LENGTH;
	return 0;
}
EXPORT_SYMBOL(xpc_command_wait_response_async_timed);

int xpc_command_wait_response_async(XpcAsyncTicket *ticket,
				    uint8_t *response_buffer,
				    size_t response_buffer_length,
				    size_t *response_length)
{
	return xpc_command_wait_response_async_timed(ticket, response_buffer,
						     response_buffer_length,
						     response_length,
						     TIMEOUT_INFINITE);
}
EXPORT_SYMBOL(xpc_command_wait_response_async);

int xpc_command_send_async(XpcChannel channel, XpcTarget target,
			   uint8_t *command, size_t command_length,
			   XpcAsyncTicket *ticket)
{
	struct XpcCommandInfo request;
	int status = 0;

	if (channel >= XPC_NUM_VCHANNELS) {
		return -ECHRNG;
	}

	if (command == NULL || command_length > XPC_MAX_COMMAND_LENGTH) {
		return -EINVAL;
	}

	if (ticket == NULL) {
		return -EINVAL;
	}

	request.channel = channel;
	request.src_id = -1;
	request.dst_mask = 0x1 << CVCORE_DSP_RENUMBER_C5Q6_TO_Q6C5(target);
	request.length = min((int)command_length, XPC_MAX_COMMAND_LENGTH);

	memcpy((void *)request.data, command, request.length);

	status = xpc_send_command_internal(&request, true,
					   XPC_CLIENT_IS_INDIRECT);
	if (status == 0) {
		*ticket = request.ticket;
	}
	return status;
}
EXPORT_SYMBOL(xpc_command_send_async);

int xpc_command_send_timed(XpcChannel channel, XpcTarget target,
			   uint8_t *command, size_t command_length,
			   uint8_t *response_buffer,
			   size_t response_buffer_length,
			   size_t *response_length, int timeout_us)
{
	int status;
	XpcAsyncTicket cmd_async_ticket = XPC_TICKET_INVALID;

	status =
		xpc_command_send_async(channel, target, command, command_length,
				       (XpcAsyncTicket *)&cmd_async_ticket);

	if (status != 0) {
		return status;
	}

	status = xpc_command_wait_response_async_timed(
		(XpcAsyncTicket *)&cmd_async_ticket, response_buffer,
		response_buffer_length, response_length, timeout_us);

	return status;
}
EXPORT_SYMBOL(xpc_command_send_timed);

int xpc_command_send(XpcChannel channel, XpcTarget target, uint8_t *command,
		     size_t command_length, uint8_t *response_buffer,
		     size_t response_buffer_length, size_t *response_length)
{
	return xpc_command_send_timed(channel, target, command, command_length,
				      response_buffer, response_buffer_length,
				      response_length, TIMEOUT_INFINITE);
}
EXPORT_SYMBOL(xpc_command_send);

int xpc_command_send_no_response(XpcChannel channel, XpcTarget target,
				 uint8_t *command, size_t command_length)
{
	struct XpcCommandInfo request;

	if (channel >= XPC_NUM_VCHANNELS) {
		return -ECHRNG;
	}

	if (command == NULL || command_length > XPC_MAX_COMMAND_LENGTH) {
		return -EINVAL;
	}

	request.channel = channel;
	request.src_id = -1;
	request.dst_mask = 0x1 << CVCORE_DSP_RENUMBER_C5Q6_TO_Q6C5(target);
	request.length = min((int)command_length, XPC_MAX_COMMAND_LENGTH);

	memcpy((void *)request.data, command, request.length);

	return xpc_send_command_internal(&request, false,
					 XPC_CLIENT_IS_INDIRECT);
}
EXPORT_SYMBOL(xpc_command_send_no_response);

int xpc_command_recv(XpcChannel channel, XpcCommandHandler handler,
		     XpcHandlerArgs args)
{
	struct xpc_info info;
	int status = 0;
	int claimed_mailbox = -1;
	long unsigned int length = XPC_MAX_RESPONSE_LENGTH;
	struct XpcResponseInfo response;

	uint8_t command_data[XPC_MAILBOX_DATA_SIZE];

	if (channel >= XPC_NUM_VCHANNELS) {
		return -ECHRNG;
	}

	info.xpc_mode = XPC_MODE_COMMAND;
	info.xpc_channel = channel;

	do {
		/**
		 * Grab a lock on the wait queue to protect against changes
		 * to the mailbox_count while evaluating the predicate. An
		 * incomming message has to, first, grab this lock before
		 * updating the mailbox count.
		 */
		spin_lock_irq(&(command_mailbox[channel].wait_queue.lock));

		status = wait_event_interruptible_locked_irq(
			command_mailbox[channel].wait_queue,
			xpc_check_mailbox_count(&command_mailbox[channel]) !=
				0);

		if (status == -ERESTARTSYS) {
			spin_unlock_irq(
				&(command_mailbox[channel].wait_queue.lock));
			xpc_debug(
				1,
				"xpc_command_recv on channel %d - restart_sys\n",
				channel);
			return -ERESTARTSYS;
		}

		if (status == 0) {
			/**
			 * Since multiple processes/threads may have been
			 * waiting and are now woken, atomically claim the
			 * mailbox such that only a single process receives
			 * the command.
			 *
			 * TODO(kirick): consider using exclusive wait_queue flag
			 * above such that the kernel only wakes a single process
			 * for an incomming command.
			 */
			claimed_mailbox = xpc_claim_global_mailbox(info);
			if (claimed_mailbox == -ECANCELED) {
				atomic_dec(&(
					command_mailbox[channel].mailbox_count));
			} else if (claimed_mailbox >= 0) {
				atomic_dec(&(
					command_mailbox[channel].mailbox_count));
			}
		}
		spin_unlock_irq(&(command_mailbox[channel].wait_queue.lock));

	} while (status != 0 || claimed_mailbox < 0);

	BUG_ON(mailboxes[claimed_mailbox].mailbox_info.target_state !=
	       MAILBOX_STATE_COMMAND_CLAIMED);

	memcpy(command_data, &(mailboxes[claimed_mailbox].mailbox_data.u8[0]),
	       XPC_MAX_COMMAND_LENGTH);

	response.has_error = 0;

	/**
	 * We received a message so start tracking the
	 * mailbox to PID association.
	 */
	xpc_set_global_mailbox_pid(claimed_mailbox, current->pid,
				   XPC_REQUEST_IS_TARGET,
				   XPC_CLIENT_IS_INDIRECT);

	// Call the handler, if provided
	if (handler != NULL) {
		status = handler(channel, args, command_data,
				 XPC_MAX_COMMAND_LENGTH, response.data,
				 XPC_MAX_RESPONSE_LENGTH, &length);

		if (status < 0) {
			response.has_error = 1;
		}
	}

	response.ticket = claimed_mailbox;
	response.length = XPC_MAX_RESPONSE_LENGTH;

	// Send the response
	status = xpc_send_command_response_internal(&response);
	if (status < 0) {
		return -EIO;
	}

	return 0;
}
EXPORT_SYMBOL(xpc_command_recv);
