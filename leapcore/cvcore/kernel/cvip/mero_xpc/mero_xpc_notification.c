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

#include "mero_xpc_notification.h"

#include <linux/uaccess.h>
#include <linux/wait.h>

#include "cvcore_xchannel_map.h"
#include "mero_xpc_common.h"
#include "mero_xpc_configuration.h"
#include "mero_xpc_kapi.h"
#include "mero_xpc_types.h"
#include "mero_xpc_uapi.h"
#include "pl320_hal.h"
#include "pl320_types.h"
#include "xdebug_internal.h"

XpcNotificationHandler notification_handlers[XPC_NUM_VCHANNELS] = { NULL };
XpcHandlerArgs notification_handler_args[XPC_NUM_VCHANNELS];
bool notification_handlers_hi[XPC_NUM_VCHANNELS] = { false };
struct xpc_queue notification_mailbox[XPC_NUM_VCHANNELS];

static int
xpc_wait_notification_complete_internal(struct XpcNotificationInfo *message);
static void
xpc_notification_save_wait_point(struct XpcNotificationInfo *message);
static bool
xpc_notification_load_wait_point(struct XpcNotificationInfo *message);

static bool any_interrupted_waits = false;
spinlock_t notification_waits_lock;
/////////////////////////////////////////////////
// Internal helper functions
/////////////////////////////////////////////////

bool xpc_notification_channel_is_hi_priority(int channel)
{
	return ((notification_handlers[channel] != NULL) &&
		notification_handlers_hi[channel]);
}

static bool
xpc_notification_load_wait_point(struct XpcNotificationInfo *message)
{
	unsigned long flags;
	bool loaded = false;
	bool interrupted_waits = false;
	int d, i, global_idx;
	struct xpc_mailbox *mb_ptr;

	spin_lock_irqsave(&notification_waits_lock, flags);
	if (!any_interrupted_waits) {
		spin_unlock_irqrestore(&notification_waits_lock, flags);
		return false;
	}

	for (d = 0; d < XPC_NUM_IPC_DEVICES; ++d) {
		message->ticket[d] = XPC_TICKET_INVALID;

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
			    MAILBOX_STATE_NOTIFICATION_WAIT_INTERRUPTED) {
				/* At least one interrupted mailbox */
				interrupted_waits = true;

				if (mb_ptr->mailbox_info.source_pid ==
				    current->pid) {
					message->ticket[d] = global_idx;
					mb_ptr->mailbox_info.source_state =
						MAILBOX_STATE_NOTIFICATION_WAIT_NON_POSTED;
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
	spin_unlock_irqrestore(&notification_waits_lock, flags);

	return loaded;
}

static void
xpc_notification_save_wait_point(struct XpcNotificationInfo *message)
{
	int i = 0;
	unsigned long flags;
	struct xpc_mailbox *mb_ptr;

	for (i = 0; i < XPC_NUM_IPC_DEVICES; i++) {
		if (message->ticket[i] == XPC_TICKET_INVALID)
			continue;

		mb_ptr = xpc_get_global_mailbox_from_global_mailbox_idx(
			message->ticket[i]);
		/*
		 * Never expect mb_ptr to be NULL. Not much we can do
		 * to handle it, so let's BUG.
		 */
		BUG_ON(mb_ptr == NULL);

		mb_ptr->mailbox_info.source_state =
			MAILBOX_STATE_NOTIFICATION_WAIT_INTERRUPTED;
		mb_ptr->mailbox_info.source_restarts++;
	}

	spin_lock_irqsave(&notification_waits_lock, flags);
	any_interrupted_waits = true;
	spin_unlock_irqrestore(&notification_waits_lock, flags);
}

int xpc_send_notification_internal(struct XpcNotificationInfo *message,
				   enum xpc_client_api_mode client_mode)
{
	struct xpc_mailbox_info mailbox_info[XPC_NUM_IPC_DEVICES];
	XpcMessageHeader *header;
	Pl320HalStatus status;
	struct xpc_route_solution route_solution;
	int i = 0;
	int release_id;
	int mailbox_id;

	if (message == NULL) {
		return -EINVAL;
	}

	if (xpc_notification_load_wait_point(message)) {
		// A previous wait was restarted so jump to the tail
		// end of notification handling
		return xpc_wait_notification_complete_internal(message);
	}

	// Let's figure out how to get where we are going
	if (xpc_compute_route(message->dst_mask, &route_solution) == 0x0) {
		return -EINVAL;
	}

	// It's possible a notification must be sent to multiple cores that may
	// require more than one IPC device to be used. This loop (a) creates a list
	// of all the required IPC, (b) sets up their channel masks, and (c)
	// acquires a mailbox resource for each.  If we are not successful in
	// acquiring any mailbox, we release all acquired resources and return a
	// "TRY AGAIN" failure.
	for (i = 0; i < XPC_NUM_IPC_DEVICES; ++i) {
		/* Initialize a pl320 struct so we can acquire a mailbox. */
		PL320_MAILBOX_INFO_STRUCT_INIT(mailbox_info[i].pl320_info);
		if ((0x1 << i) & route_solution.valid_routes) {
			mailbox_info[i].pl320_info.ipc_id = i;
			mailbox_info[i].pl320_info.ipc_irq_id = mask_32b_to_idx(
				route_solution.routes[i].ipc_source_irq_mask);
			mailbox_info[i].pl320_info.mailbox_mode =
				PL320_MAILBOX_MODE_AUTO_ACK;

			if (pl320_acquire_mailbox(
				    &mailbox_info[i].pl320_info,
				    route_solution.routes[i]
					    .ipc_target_irq_mask) !=
			    PL320_STATUS_SUCCESS) {
				// release mailboxes we acquired thus far
				for (release_id = 0; release_id < i;
				     release_id++) {
					mailbox_id = xpc_get_global_mailbox(
						&(mailbox_info[release_id]));
					xpc_close_global_mailbox(mailbox_id);
				}
				return -EAGAIN;
			}
		}
	}

	header = (XpcMessageHeader *)&(message->data[XPC_MAILBOX_HEADER_LOC]);
	header->fields.mode = XPC_MODE_NOTIFICATION;
	header->fields.rsvd = 0;
	header->fields.error = 0;
	header->fields.vchannel = message->channel;
	message->length = XPC_MAILBOX_DATA_SIZE;
	message->ticket[0] = XPC_TICKET_INVALID;
	message->ticket[1] = XPC_TICKET_INVALID;
	message->ticket[2] = XPC_TICKET_INVALID;
	message->ticket[3] = XPC_TICKET_INVALID;
	message->ticket[4] = XPC_TICKET_INVALID;

	for (i = 0; i < XPC_NUM_IPC_DEVICES; i++) {
		if (mailbox_info[i].pl320_info.ipc_id != XPC_IPC_ID_INVALID) {
			message->ticket[i] = mailbox_id =
				xpc_get_global_mailbox(&mailbox_info[i]);

			/*
			 * Initialize a global mailbox struct with our local initializations.
			 */
			reinit_completion(
				&(mailboxes[mailbox_id].target_replied));

			if (message->mode == XPC_NOTIFICATION_MODE_POSTED) {
				// The main IRQ handler will clear the mailboxes when the acks are
				// received since we don't care to wait.
				mailboxes[mailbox_id].mailbox_info.source_state =
					MAILBOX_STATE_NOTIFICATION_WAIT_POSTED;
			} else {
				mailboxes[mailbox_id].mailbox_info.source_state =
					MAILBOX_STATE_NOTIFICATION_WAIT_NON_POSTED;
			}

			mailboxes[mailbox_id].mailbox_info.pl320_info =
				mailbox_info[i].pl320_info;

			xpc_set_global_mailbox_pid(mailbox_id, current->pid,
						   XPC_REQUEST_IS_SOURCE,
						   client_mode);

			status = pl320_send(
				&(mailboxes[mailbox_id].mailbox_info.pl320_info),
				route_solution.routes[i].ipc_target_irq_mask,
				message->data, message->length);

			if (status != PL320_STATUS_SUCCESS) {
				// Release all acquired mailboxes and fail!
				// This may be catastrophic as some recipients may have
				// been notified while others haven't
				for (release_id = 0;
				     release_id < XPC_NUM_IPC_DEVICES;
				     release_id++) {
					if (mailbox_info[release_id]
						    .pl320_info.ipc_id !=
					    XPC_IPC_ID_INVALID) {
						mailbox_id = xpc_get_global_mailbox(
							&(mailbox_info
								  [release_id]));
						xpc_clr_global_mailbox_pid(
							mailbox_id,
							XPC_REQUEST_IS_SOURCE);
						xpc_close_global_mailbox(
							mailbox_id);
					}
				}
				return -EIO;
			}
		}
	}

	if (message->mode == XPC_NOTIFICATION_MODE_WAIT_ACK) {
		return xpc_wait_notification_complete_internal(message);
	}

	return 0;
}

static int
xpc_wait_notification_complete_internal(struct XpcNotificationInfo *message)
{
	int timeout_us;
	Pl320HalStatus status;
	int had_failure = 0;
	int i;
	uint16_t length;
	XpcMessageHeader header;
	int ipc_id;
	int mailbox_idx;

	timeout_us = message->timeout_us;
	// We collect the acknowledgements for all ipc devices
	// here then clear the mailboxes since the client
	// requested end-to-end acknowledgement.
	// Note: An IPC device returns an ACK only after all
	// destinations have individually acked
	for (i = 0; i < XPC_NUM_IPC_DEVICES; i++) {
		if (message->ticket[i] == XPC_TICKET_INVALID)
			continue;

		xpc_get_ipc_id_and_mailbox(message->ticket[i], &ipc_id,
					   &mailbox_idx);

		xdebug_event_record(
			ipc_id, mailbox_idx,
			XPC_EVT_NOTIFICATION_WAIT_FOR_COMPLETION_START,
			message->ticket[i]);

		// After this call, timeout_us will have the remaining time
		// we have for waiting on remaining mailboxes
		status =
			xpc_wait_mailbox_reply(message->ticket[i], &timeout_us);
		if (status == -ERESTARTSYS) {
			// TODO(kirick): once timeout is implemented this
			// needs to be updated to return ERESTART_RESTARTBLOCK
			// with the restart block reflecting the remaining
			// timeout value and any state that needs to persist
			// across syscall re-invocation.
			xdebug_event_record(
				ipc_id, mailbox_idx,
				XPC_EVT_NOTIFICATION_WAIT_FOR_COMPLETION_RESTARTSYS,
				message->ticket[i]);

			xpc_notification_save_wait_point(message);

			return -ERESTARTSYS;
		}

		xdebug_event_record(
			ipc_id, mailbox_idx,
			XPC_EVT_NOTIFICATION_WAIT_FOR_COMPLETION_END,
			message->ticket[i]);

		if (status != 0) {
			// At least one failure
			// this needs to be in the message struct
			had_failure = 1;
		}
		// Check for error bit in header
		pl320_read_mailbox_data(
			&(mailboxes[message->ticket[i]].mailbox_info.pl320_info),
			XPC_MAILBOX_HEADER_LOC,
			//Last byte in the mailbox is the header
			(uint8_t *)&header, sizeof(XpcMessageHeader), &length);

		if (header.fields.error) {
			had_failure = 1;
		}
		xpc_clr_global_mailbox_pid(message->ticket[i],
					   XPC_REQUEST_IS_SOURCE);
		xpc_close_global_mailbox(message->ticket[i]);
	}
	return had_failure ? -EREMOTEIO : 0;
}

int xpc_acknowledge_notification_internal(int mailbox_id)
{
	Pl320HalStatus status;
	// The mailbox is free from the perspective of the target
	mailboxes[mailbox_id].mailbox_info.target_state = MAILBOX_STATE_FREE;

	xpc_clr_global_mailbox_pid(mailbox_id, XPC_REQUEST_IS_TARGET);

	status = pl320_reply(&(mailboxes[mailbox_id].mailbox_info.pl320_info),
			     NULL, 0);

	return xpc_hal_status_to_errno(status);
}

int xpc_register_notification_handler(XpcChannel channel,
				      XpcNotificationHandler handler,
				      XpcHandlerArgs args)
{
	if (channel >= XPC_NUM_VCHANNELS) {
		return -ECHRNG;
	}

	notification_handlers_hi[(int)channel] = false;
	notification_handlers[(int)channel] = handler;
	notification_handler_args[(int)channel] = args;
	return 0;
}
EXPORT_SYMBOL(xpc_register_notification_handler);

int xpc_register_notification_handler_hi(XpcChannel channel,
					 XpcNotificationHandler handler,
					 XpcHandlerArgs args)
{
	int status;
	status = xpc_register_notification_handler(channel, handler, args);
	if (status == 0) {
		notification_handlers_hi[(int)channel] =
			(handler == NULL) ? false : true;
	}
	return status;
}
EXPORT_SYMBOL(xpc_register_notification_handler_hi);

int xpc_notification_send_timed(XpcChannel channel, XpcTargetMask targets,
				uint8_t *data, size_t length,
				XpcNotificationMode mode, int timeout_us)
{
	struct XpcNotificationInfo notice;

	if (channel >= XPC_NUM_VCHANNELS) {
		return -ECHRNG;
	}

	// TODO(kirick):Consider zero sized notifications
	if (0x0 == length || length > XPC_MAX_NOTIFICATION_LENGTH) {
		return -EINVAL;
	}

	if (data == NULL) {
		return -EINVAL;
	}

	notice.channel = channel;
	notice.mode = mode;
	notice.src_id = -1;
	notice.dst_mask = XPC_REMASK(targets);
	notice.length = min((int)length, XPC_MAX_NOTIFICATION_LENGTH);

	memcpy((void *)notice.data, data, notice.length);

	return xpc_send_notification_internal(&notice, XPC_CLIENT_IS_INDIRECT);
}
EXPORT_SYMBOL(xpc_notification_send_timed);

int xpc_notification_send(XpcChannel channel, XpcTargetMask targets,
			  uint8_t *data, size_t length,
			  XpcNotificationMode mode)
{
	return xpc_notification_send_timed(channel, targets, data, length, mode,
					   TIMEOUT_INFINITE);
}
EXPORT_SYMBOL(xpc_notification_send);

int xpc_notification_recv(XpcChannel channel, uint8_t *notification_buffer,
			  size_t notification_buffer_size)
{
	struct xpc_info info;
	int status = 0;
	int claimed_mailbox = -1;

	if ((notification_buffer == NULL) || (notification_buffer_size == 0) ||
	    (notification_buffer_size < XPC_MAX_NOTIFICATION_LENGTH)) {
		return -EINVAL;
	}

	info.xpc_mode = XPC_MODE_NOTIFICATION;
	info.xpc_channel = channel;

	do {
		// Grab a lock on the wait queue to protect against changes
		// to the mailbox_count while evaluating the predicate. An
		// incomming message has to, first, grab this lock before
		// updating the mailbox count.
		spin_lock_irq(&notification_mailbox[channel].wait_queue.lock);

		status = wait_event_interruptible_locked_irq(
			notification_mailbox[channel].wait_queue,
			xpc_check_mailbox_count(
				&notification_mailbox[channel]) != 0);

		if (status == -ERESTARTSYS) {
			spin_unlock_irq(
				&notification_mailbox[channel].wait_queue.lock);
			xpc_debug(
				1,
				"xpc_notification_recv on channel %d - restart_sys\n",
				channel);
			return -ERESTARTSYS;
		}

		claimed_mailbox = xpc_claim_global_mailbox(info);
		if (claimed_mailbox == -ECANCELED) {
			atomic_dec(
				&(notification_mailbox[channel].mailbox_count));
		} else if (claimed_mailbox >= 0) {
			atomic_dec(
				&(notification_mailbox[channel].mailbox_count));
		}

		spin_unlock_irq(&notification_mailbox[channel].wait_queue.lock);
	} while (claimed_mailbox < 0);

	BUG_ON(mailboxes[claimed_mailbox].mailbox_info.target_state !=
	       MAILBOX_STATE_NOTIFICATION_CLAIMED);

	memcpy(notification_buffer,
	       &(mailboxes[claimed_mailbox].mailbox_data.u8[0]),
	       XPC_MAX_NOTIFICATION_LENGTH);

	/**
	 * We received a message so start tracking the
	 * mailbox to PID association.
	 */
	xpc_set_global_mailbox_pid(claimed_mailbox, current->pid,
				   XPC_REQUEST_IS_TARGET,
				   XPC_CLIENT_IS_INDIRECT);

	// Send the acknowledgement
	status = xpc_acknowledge_notification_internal(claimed_mailbox);
	if (status < 0) {
		return -EIO;
	}

	return 0;
}
EXPORT_SYMBOL(xpc_notification_recv);
