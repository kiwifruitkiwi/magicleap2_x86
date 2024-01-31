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

#include "mero_xpc_queue.h"

#include <linux/uaccess.h>
#include <linux/wait.h>

#include "mero_xpc_common.h"
#include "mero_xpc_configuration.h"
#include "mero_xpc_kapi.h"
#include "mero_xpc_types.h"
#include "mero_xpc_uapi.h"
#include "pl320_hal.h"
#include "pl320_types.h"

int xpc_queue_check_subchannel_match(XpcChannel xpc_channel,
				     XpcSubChannel xpc_sub_channel,
				     int *claimed_mailbox);

struct xpc_queue queue_mailbox[XPC_NUM_VCHANNELS];

/**
 * A user and kernel process can be registered on the same
 * vchannel/schannel but multiple user processes can not
 * be registered on the same vchannel/schannel
 */
XpcQueueChannelMask registered_user_subchannels;
XpcQueueChannelMask registered_kernel_subchannels;
spinlock_t registered_queue_lock;

int xpc_send_queue_internal(struct XpcQueueInfo *message,
			    enum xpc_client_api_mode client_mode)
{
	struct xpc_mailbox_info mailbox_info[XPC_NUM_IPC_DEVICES];
	XpcMessageHeader *header;
	Pl320HalStatus status;
	struct xpc_route_solution route_solution;
	int i = 0;
	int release_id;
	int mailbox_id;

	// Let's figure out how to get where we are going (cores)
	if (xpc_compute_route(message->dst_mask, &route_solution) == 0x0) {
		return -EINVAL;
	}

	// It's possible a queue must be sent to multiple cores that may require
	// more than one IPC device to be used. This loop (a) creates a list of all
	// the required IPC, (b) sets up their channel masks, and (c) acquires a
	// mailbox resource for each.  If we are not successful in acquiring any
	// mailbox, we release all acquired resources and return a "TRY AGAIN"
	// failure.
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

	message->length = XPC_MAILBOX_DATA_SIZE;
	message->data[XPC_SUB_CHANNEL_BYTE] = message->sub_channel;
	message->ticket[0] = XPC_TICKET_INVALID;
	message->ticket[1] = XPC_TICKET_INVALID;
	message->ticket[2] = XPC_TICKET_INVALID;
	message->ticket[3] = XPC_TICKET_INVALID;
	message->ticket[4] = XPC_TICKET_INVALID;

	header = (XpcMessageHeader *)&message->data[XPC_MAILBOX_HEADER_LOC];
	header->fields.mode = XPC_MODE_QUEUE;
	header->fields.rsvd = 0;
	header->fields.error = 0;
	header->fields.vchannel = message->channel;

	for (i = 0; i < XPC_NUM_IPC_DEVICES; i++) {
		if (mailbox_info[i].pl320_info.ipc_id != XPC_IPC_ID_INVALID) {
			message->ticket[i] = mailbox_id =
				xpc_get_global_mailbox(&(mailbox_info[i]));

			/*
				 * Initialize a global mailbox struct with our local initializations.
				 */
			reinit_completion(
				&(mailboxes[mailbox_id].target_replied));

			mailboxes[mailbox_id].mailbox_info.source_state =
				MAILBOX_STATE_QUEUE_SENT;

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
	return 0;
}

int xpc_acknowledge_queue_internal(int mailbox_id)
{
	Pl320HalStatus status;
	// The mailbox is free from the perspective of the target
	mailboxes[mailbox_id].mailbox_info.target_state = MAILBOX_STATE_FREE;

	xpc_clr_global_mailbox_pid(mailbox_id, XPC_REQUEST_IS_TARGET);
	status = pl320_reply(&(mailboxes[mailbox_id].mailbox_info.pl320_info),
			     NULL, 0);

	return xpc_hal_status_to_errno(status);
}

// Find a mailbox that is ready for this channel/sub_channel
int xpc_queue_check_and_claim_mailbox(struct xpc_queue *queue,
				      XpcChannel vchannel,
				      XpcSubChannel schannel,
				      int *claimed_mailbox)
{
	int status;
	status = xpc_check_mailbox_count(queue);
	if (status == 0) {
		// No pending mailboxes for this vchannel
		return 0;
	}
	status = xpc_queue_check_subchannel_match(vchannel, schannel,
						  claimed_mailbox);
	if (status == 0) {
		// No pending mailboxes for this schannel
		return 0;
	}
	// Claimed mailbox that is pending for this vchannel/schannel
	return 1;
}

// Find a mailbox that is ready for this channel/sub_channel
int xpc_queue_kernel_check_and_claim_mailbox(struct xpc_queue *queue,
					     XpcChannel vchannel,
					     XpcSubChannel schannel,
					     int *claimed_mailbox)
{
	int status;

	if (!xpc_queue_check_kernel_registration(vchannel, schannel)) {
		// Not registered to receive on this channel
		pr_err("Queue is no longer registered\n");
		*claimed_mailbox = -EPERM;
		return 1;
	}

	status = xpc_check_mailbox_count(queue);
	if (status == 0) {
		// No pending mailboxes for this vchannel
		return 0;
	}
	status = xpc_queue_check_subchannel_match(vchannel, schannel,
						  claimed_mailbox);
	if (status == 0) {
		// No pending mailboxes for this schannel
		return 0;
	}

	// Claimed mailbox that is pending for this vchannel/schannel
	return 1;
}

// Find and claim mailbox for this channel/sub_channel if it exists
int xpc_queue_check_subchannel_match(XpcChannel vchannel,
				     XpcSubChannel schannel,
				     int *claimed_mailbox)
{
	int i, m, d = 0;
	unsigned long flags;
	spinlock_t *mb_lock = NULL;
	struct xpc_mailbox_info *mb_info = NULL;

	for (d = 0; d < XPC_NUM_IPC_DEVICES; ++d) {
		if (test_bit(d, &enabled_mailboxes) == 0) {
			continue;
		}

		for (i = 0; i < XPC_IPC_NUM_MAILBOXES; ++i) {
			m = d * XPC_IPC_NUM_MAILBOXES + i;

			mb_lock = &mailboxes[m].mailbox_lock;
			mb_info = &mailboxes[m].mailbox_info;

			spin_lock_irqsave(mb_lock, flags);

			if (mb_info->xpc_channel != vchannel) {
				spin_unlock_irqrestore(mb_lock, flags);
				continue;
			}

			if (mailboxes[m].mailbox_data.u8[XPC_SUB_CHANNEL_BYTE] !=
			    schannel) {
				spin_unlock_irqrestore(mb_lock, flags);
				continue;
			}

			if (mb_info->target_state !=
			    MAILBOX_STATE_QUEUE_PENDING) {
				spin_unlock_irqrestore(mb_lock, flags);
				continue;
			}

			// Found mailbox with matching channel/sub_channel that has
			// pending status might be redundant information here
			mb_info->target_state = MAILBOX_STATE_QUEUE_CLAIMED;

			spin_unlock_irqrestore(mb_lock, flags);
			*claimed_mailbox = m;
			return 1;
		}
	}

	*claimed_mailbox = -1;
	return 0;
}

// Determine if user process is registered on vchannel/schannel for queue mode
int xpc_queue_check_user_registration(XpcChannel vchannel,
				      XpcSubChannel schannel,
				      uint8_t *process_subchannels)
{
	int check;

	if (vchannel < XPC_NUM_VCHANNELS) {
		check = (process_subchannels[CHANNEL_MASK_OFFSET(vchannel,
								 schannel)] &
			 SUB_CHANNEL_MASK(schannel));

		return check;
	}

	return 0;
}

// Register user process on vchannel/schannel for queue mode
int xpc_queue_register_user(XpcChannel vchannel, XpcSubChannel schannel,
			    uint8_t *process_subchannels)
{
	unsigned long flags;
	uint8_t p_registered;
	uint8_t u_registered;

	if (vchannel >= XPC_NUM_VCHANNELS) {
		return -ECHRNG;
	}

	// Don't register if another user process is already registered for this
	// vchannel/schannel
	spin_lock_irqsave(&(registered_queue_lock), flags);

	u_registered = registered_user_subchannels[vchannel]
						  [SUB_CHANNEL_BYTE(schannel)] &
		       SUB_CHANNEL_MASK(schannel);

	if (!u_registered) {
		// Not registered by any other process
		// Update global userspace registry
		registered_user_subchannels[vchannel]
					   [SUB_CHANNEL_BYTE(schannel)] |=
			SUB_CHANNEL_MASK(schannel);

		// Update this process/thread's registry
		process_subchannels[CHANNEL_MASK_OFFSET(vchannel, schannel)] |=
			SUB_CHANNEL_MASK(schannel);

		spin_unlock_irqrestore(&(registered_queue_lock), flags);
		return 0;
	} else {
		// check if this user process is the existing owner.
		// if so, treat as success
		p_registered = process_subchannels[CHANNEL_MASK_OFFSET(
				       vchannel, schannel)] &
			       SUB_CHANNEL_MASK(schannel);
		if (p_registered) {
			spin_unlock_irqrestore(&(registered_queue_lock), flags);
			return 0;
		}
	}

	spin_unlock_irqrestore(&(registered_queue_lock), flags);

	// The vchannel/schannel queue is owned by another
	return -EPERM;
}

// Deregister user process on vchannel/schannel for queue mode
int xpc_queue_deregister_user(XpcChannel vchannel, XpcSubChannel schannel,
			      uint8_t *process_subchannels)
{
	unsigned long flags;
	uint8_t k_registered;
	uint8_t p_registered;
	int mailbox = -1;

	if (vchannel >= XPC_NUM_VCHANNELS) {
		return -ECHRNG;
	}

	// Don't deregister if this user process is not already registered
	// for this vchannel/schannel
	spin_lock_irqsave(&(registered_queue_lock), flags);

	p_registered =
		process_subchannels[CHANNEL_MASK_OFFSET(vchannel, schannel)] &
		SUB_CHANNEL_MASK(schannel);

	if (p_registered) {
		registered_user_subchannels[vchannel]
					   [SUB_CHANNEL_BYTE(schannel)] &=
			~SUB_CHANNEL_MASK(schannel);

		process_subchannels[CHANNEL_MASK_OFFSET(vchannel, schannel)] &=
			~SUB_CHANNEL_MASK(schannel);

		// Check if another kernel driver is registered before releasing any
		// mailboxes associated with this vchannel/schannel
		k_registered =
			registered_kernel_subchannels[vchannel][SUB_CHANNEL_BYTE(
				schannel)] &
			SUB_CHANNEL_MASK(schannel);

		if (!k_registered) {
			while (xpc_queue_check_subchannel_match(
				       vchannel, schannel, &mailbox) != 0) {
				mailboxes[mailbox].mailbox_info.target_state =
					MAILBOX_STATE_FREE;
				xpc_acknowledge_queue_internal(mailbox);
			}
		}
		spin_unlock_irqrestore(&(registered_queue_lock), flags);
		return 0;
	}

	spin_unlock_irqrestore(&(registered_queue_lock), flags);

	// The vchannel/schannel queue is owned by another
	return -EPERM;
}

// Determine if kernel is registered on vchannel/schannel for queue mode
int xpc_queue_check_kernel_registration(XpcChannel vchannel,
					XpcSubChannel schannel)
{
	int check;

	if (vchannel < XPC_NUM_VCHANNELS) {
		check = (registered_kernel_subchannels[vchannel]
						      [SUB_CHANNEL_BYTE(
							      schannel)] &
			 SUB_CHANNEL_MASK(schannel));

		return check;
	}
	return 0;
}

int xpc_queue_register(XpcChannel vchannel, XpcSubChannel schannel)
{
	unsigned long flags;
	uint8_t k_registered;

	if (vchannel >= XPC_NUM_VCHANNELS) {
		return -ECHRNG;
	}

	// Don't register if another kernel driver is already registered for this
	// vchannel/schannel
	spin_lock_irqsave(&(registered_queue_lock), flags);

	k_registered =
		registered_kernel_subchannels[vchannel]
					     [SUB_CHANNEL_BYTE(schannel)] &
		SUB_CHANNEL_MASK(schannel);

	if (!k_registered) {
		registered_kernel_subchannels[vchannel]
					     [SUB_CHANNEL_BYTE(schannel)] |=
			SUB_CHANNEL_MASK(schannel);

		spin_unlock_irqrestore(&(registered_queue_lock), flags);
		return 0;
	}

	spin_unlock_irqrestore(&(registered_queue_lock), flags);
	// The channel is already in use
	return -EBUSY;
}
EXPORT_SYMBOL(xpc_queue_register);

int xpc_queue_deregister(XpcChannel vchannel, XpcSubChannel schannel)
{
	unsigned long flags;
	int mailbox = -1;
	uint8_t k_registered;
	uint8_t u_registered;
	unsigned long wq_flags;
	struct xpc_queue *ch_queue;

	if (vchannel >= XPC_NUM_VCHANNELS) {
		return -ECHRNG;
	}

	// Don't deregister if this kernel driver is not registered for this
	// vchannel/schannel
	spin_lock_irqsave(&(registered_queue_lock), flags);

	k_registered =
		registered_kernel_subchannels[vchannel]
					     [SUB_CHANNEL_BYTE(schannel)] &
		SUB_CHANNEL_MASK(schannel);

	if (k_registered) {
		registered_kernel_subchannels[vchannel]
					     [SUB_CHANNEL_BYTE(schannel)] &=
			~SUB_CHANNEL_MASK(schannel);

		// Check if another user process is registered before releasing any
		// mailboxes associated with this vchannel/schannel
		u_registered =
			registered_user_subchannels[vchannel][SUB_CHANNEL_BYTE(
				schannel)] &
			SUB_CHANNEL_MASK(schannel);

		if (!u_registered) {
			while (xpc_queue_check_subchannel_match(
				       vchannel, schannel, &mailbox) != 0) {
				mailboxes[mailbox].mailbox_info.target_state =
					MAILBOX_STATE_FREE;
				xpc_acknowledge_queue_internal(mailbox);
			}
		}

		// Wake up any waiters
		ch_queue = &queue_mailbox[vchannel];

		spin_lock_irqsave(&ch_queue->wait_queue.lock, wq_flags);
		wake_up_locked(&ch_queue->wait_queue);
		spin_unlock_irqrestore(&ch_queue->wait_queue.lock, wq_flags);

		spin_unlock_irqrestore(&(registered_queue_lock), flags);

		return 0;
	}
	spin_unlock_irqrestore(&(registered_queue_lock), flags);
	// The vchannel/schannel queue is not registered
	return -EPERM;
}
EXPORT_SYMBOL(xpc_queue_deregister);

int xpc_queue_send(XpcChannel channel, XpcSubChannel sub_channel,
		   XpcTargetMask targets, uint8_t *data, size_t length)
{
	struct XpcQueueInfo req;

	if (channel >= XPC_NUM_VCHANNELS) {
		return -ECHRNG;
	}

	if (0x0 == length || length > XPC_MAX_QUEUE_LENGTH) {
		return -EINVAL;
	}

	if (data == NULL) {
		return -EINVAL;
	}

	req.channel = channel;
	req.sub_channel = sub_channel;
	req.src_id = -1;
	req.dst_mask = XPC_REMASK(targets);
	req.length = length;

	memcpy((void *)req.data, data, req.length);

	return xpc_send_queue_internal(&req, XPC_CLIENT_IS_INDIRECT);
}
EXPORT_SYMBOL(xpc_queue_send);

int xpc_queue_recv(XpcChannel channel, XpcSubChannel sub_channel,
		   uint8_t *queue_buffer, size_t queue_buffer_size)
{
	struct xpc_info info;
	int status = 0;
	int claimed_mailbox = -1;
	unsigned long flags;

	if (channel >= XPC_NUM_VCHANNELS) {
		return -ECHRNG;
	}

	if ((queue_buffer == NULL) || (queue_buffer_size <= 0) ||
	    (queue_buffer_size > XPC_MAX_QUEUE_LENGTH)) {
		return -EINVAL;
	}

	info.xpc_mode = XPC_MODE_QUEUE;
	info.xpc_channel = channel;
	info.xpc_sub_channel = sub_channel;

	spin_lock_irqsave(&(registered_queue_lock), flags);
	if (!xpc_queue_check_kernel_registration(info.xpc_channel,
						 info.xpc_sub_channel)) {
		spin_unlock_irqrestore(&(registered_queue_lock), flags);
		// Not registered to receive on this channel
		return -EPERM;
	}
	spin_unlock_irqrestore(&(registered_queue_lock), flags);

	do {
		spin_lock_irq(&queue_mailbox[info.xpc_channel].wait_queue.lock);

		status = wait_event_interruptible_locked_irq(
			queue_mailbox[info.xpc_channel].wait_queue,
			(xpc_queue_kernel_check_and_claim_mailbox(
				 &queue_mailbox[info.xpc_channel],
				 info.xpc_channel, info.xpc_sub_channel,
				 &claimed_mailbox) != 0));

		if (status == -ERESTARTSYS) {
			spin_unlock_irq(&queue_mailbox[info.xpc_channel]
						 .wait_queue.lock);
			xpc_debug(
				1,
				"xpc_queue_recv on channel %d subchannel %d - restart_sys\n",
				channel, sub_channel);
			return -ERESTARTSYS;
		}

		if (claimed_mailbox == -EPERM) {
			// We lost registration of this queue
			spin_unlock_irq(&queue_mailbox[info.xpc_channel]
						 .wait_queue.lock);
			return -EPERM;
		} else if (claimed_mailbox == -ECANCELED) {
			atomic_dec(&(
				queue_mailbox[info.xpc_channel].mailbox_count));
		} else if (claimed_mailbox >= 0) {
			atomic_dec(&(
				queue_mailbox[info.xpc_channel].mailbox_count));
		}

		spin_unlock_irq(
			&queue_mailbox[info.xpc_channel].wait_queue.lock);
	} while (claimed_mailbox < 0);

	//atomic_dec(&(queue_mailbox[info.xpc_channel].mailbox_count));

	BUG_ON(mailboxes[claimed_mailbox].mailbox_info.target_state !=
	       MAILBOX_STATE_QUEUE_CLAIMED);

	memcpy(queue_buffer, &(mailboxes[claimed_mailbox].mailbox_data.u8[0]),
	       queue_buffer_size);

	/**
	 * We received a message so start tracking the
	 * mailbox to PID association.
	 */
	xpc_set_global_mailbox_pid(claimed_mailbox, current->pid,
				   XPC_REQUEST_IS_TARGET,
				   XPC_CLIENT_IS_INDIRECT);

	// Send the acknowledgement
	status = xpc_acknowledge_queue_internal(claimed_mailbox);
	if (status < 0) {
		return -EIO;
	}

	return 0;
}
EXPORT_SYMBOL(xpc_queue_recv);
