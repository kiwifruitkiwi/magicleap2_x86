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

#include <linux/sched.h>

#include "mero_xpc_common.h"

#include "cvcore_xchannel_map.h"
#include "mero_xpc_kapi.h"
#include "mero_xpc_types.h"
#include "mero_xpc_uapi.h"
#include "pl320_hal.h"
#include "pl320_types.h"
#include "xstat_internal.h"

struct xpc_mailbox mailboxes[NUM_MAILBOXES];
unsigned long enabled_mailboxes = 0;

static spinlock_t indirect_client_count_lock;
static atomic_t indirect_client_count;

int xpc_hal_status_to_errno(Pl320HalStatus status)
{
	switch (status) {
	case PL320_STATUS_SUCCESS:
		return 0;
	case PL320_STATUS_FAILURE_DEVICE_UNAVAILABLE:
	case PL320_STATUS_FAILURE_CHANNEL_UNAVAILABLE:
	case PL320_STATUS_FAILURE_MAILBOX_UNAVAILABLE:
	case PL320_STATUS_FAILURE_MAILBOX_OFFLINE:
		return -EAGAIN;
	case PL320_STATUS_FAILURE_DEVICE_INVALID:
		return -ENODEV;
	case PL320_STATUS_FAILURE_NULL_POINTER:
	case PL320_STATUS_FAILURE_ALIGNEMENT:
	case PL320_STATUS_FAILURE_DATA_LENGTH:
	case PL320_STATUS_FAILURE_DATA_NULL:
	case PL320_STATUS_FAILURE_DATA_LOAD:
	case PL320_STATUS_FAILURE_INVALID_PARAMETER:
	case PL320_STATUS_FAILURE_INVALID_DESTINATION:
		return -EINVAL;
	case PL320_STATUS_FAILURE_INITIALIZATION:
		return -EUNATCH;
		break;
		/*
	case PL320_STATUS_FAILURE_TIMEOUT:
		return -ETIME;
	case PL320_STATUS_FAILURE_REMOTE_CLOSED:
		return -ESHUTDOWN;
	case PL320_STATUS_FAILURE_LOCK_NOT_ACQUIRED:
		return -ENOLCK;
		*/
	default:
		return -EIO;
	}
}

int xpc_wait_mailbox_reply(int global_mailbox_number,
			   __attribute__((unused)) int *timeout_us)
{
	int status;
	do {
		status = wait_for_completion_interruptible(
			&(mailboxes[global_mailbox_number].target_replied));
		if (status == -ERESTARTSYS) {
			return -ERESTARTSYS;
		}
	} while (status != 0);
	return 0;
}

int xpc_check_mailbox_count(struct xpc_queue *queue)
{
	return atomic_read(&(queue->mailbox_count));
}

void xpc_initialize_global_mailboxes(void)
{
	int i;
	for (i = 0; i < NUM_MAILBOXES; ++i) {
		spin_lock_init(&(mailboxes[i].mailbox_lock));
		init_waitqueue_head(&(mailboxes[i].wait_queue));
		init_completion(&(mailboxes[i].target_replied));
		mailboxes[i].mailbox_info.target_state = MAILBOX_STATE_FREE;
		mailboxes[i].mailbox_info.source_state = MAILBOX_STATE_FREE;
		mailboxes[i].mailbox_info.target_pid = XPC_INVALID_CLIENT_PID;
		mailboxes[i].mailbox_info.source_pid = XPC_INVALID_CLIENT_PID;
		mailboxes[i].mailbox_info.source_client_mode =
			XPC_CLIENT_IS_UNKNOWN;
		mailboxes[i].mailbox_info.target_client_mode =
			XPC_CLIENT_IS_UNKNOWN;
		mailboxes[i].mailbox_info.target_restarts = 0;
		mailboxes[i].mailbox_info.source_restarts = 0;
	}

	spin_lock_init(&notification_waits_lock);
	spin_lock_init(&command_waits_lock);
	spin_lock_init(&indirect_client_count_lock);
	atomic_set(&indirect_client_count, 0);
}

/**
 * xpc_close_global_mailbox - Close a mailbox and mark its state
 * and status as FREE and AVAILABLE.
 *
 * The caller of this function knows that no other shared owner
 * of this mailbox is considering the state fields.  Any non-owner
 * of this mailbox shall not consider the state fields until they
 * become an owner.  Ownership is released from current owners, and
 * available to new owners, only after calling pl320_release_mailbox().
 * We deinitialize state fields before releasing the mailbox since
 * timing cannot be guaranteed after.
 *
 */
void xpc_close_global_mailbox(int mailbox_idx)
{
	mailboxes[mailbox_idx].mailbox_info.target_state = MAILBOX_STATE_FREE;
	mailboxes[mailbox_idx].mailbox_info.source_state = MAILBOX_STATE_FREE;
	mailboxes[mailbox_idx].mailbox_info.target_restarts = 0;
	mailboxes[mailbox_idx].mailbox_info.source_restarts = 0;
	pl320_release_mailbox(
		&(mailboxes[mailbox_idx].mailbox_info.pl320_info));
}

void xpc_close_ipc_mailbox(int ipc_id, int mailbox_idx)
{
	int global_mailbox_idx = ipc_id * XPC_IPC_NUM_MAILBOXES + mailbox_idx;
	if (mailboxes[global_mailbox_idx].mailbox_info.pl320_info.ipc_id ==
		    ipc_id &&
	    mailboxes[global_mailbox_idx].mailbox_info.pl320_info.mailbox_idx ==
		    mailbox_idx) {
		xpc_close_global_mailbox(global_mailbox_idx);
	}
}

int xpc_get_global_mailbox(struct xpc_mailbox_info *info)
{
	info->global_mailbox_number =
		(info->pl320_info.ipc_id * XPC_IPC_NUM_MAILBOXES) +
		info->pl320_info.mailbox_idx;
	return info->global_mailbox_number;
}

void xpc_print_global_mailbox(int global_mailbox_idx, bool is_target)
{
	int ipc_id;
	int mailbox_idx;

	xpc_get_ipc_id_and_mailbox(global_mailbox_idx, &ipc_id, &mailbox_idx);

	printk("%s IPC[%02u][%02u] - target_pid %d "
	       "and source_pid %d - target_state %d and source_state %d\n",
	       is_target ? "RECV" : "SEND", ipc_id, mailbox_idx,
	       mailboxes[global_mailbox_idx].mailbox_info.target_pid,
	       mailboxes[global_mailbox_idx].mailbox_info.source_pid,
	       mailboxes[global_mailbox_idx].mailbox_info.target_state,
	       mailboxes[global_mailbox_idx].mailbox_info.source_state);
}

void xpc_set_global_mailbox_pid(int global_mailbox_idx, pid_t pid,
				bool is_target,
				enum xpc_client_api_mode client_mode)
{
	unsigned long flags;

	if (is_target) {
		if (mailboxes[global_mailbox_idx].mailbox_info.target_pid >=
		    XPC_VALID_CLIENT_PID) {
			xpc_debug(
				0,
				"target mb %d already had target_pid %d"
				"and source_pid %d - target_state %d and source_state %d\n",
				global_mailbox_idx,
				mailboxes[global_mailbox_idx]
					.mailbox_info.target_pid,
				mailboxes[global_mailbox_idx]
					.mailbox_info.source_pid,
				mailboxes[global_mailbox_idx]
					.mailbox_info.target_state,
				mailboxes[global_mailbox_idx]
					.mailbox_info.source_state);
		}
		mailboxes[global_mailbox_idx].mailbox_info.target_pid = pid;
		mailboxes[global_mailbox_idx].mailbox_info.target_restarts = 0;
		mailboxes[global_mailbox_idx].mailbox_info.target_client_mode =
			client_mode;
	} else {
		if (mailboxes[global_mailbox_idx].mailbox_info.source_pid >=
		    XPC_VALID_CLIENT_PID) {
			xpc_debug(
				0,
				"source mb %d already had source_pid %d"
				"and target_pid %d - source_state %d and target_state %d\n",
				global_mailbox_idx,
				mailboxes[global_mailbox_idx]
					.mailbox_info.source_pid,
				mailboxes[global_mailbox_idx]
					.mailbox_info.target_pid,
				mailboxes[global_mailbox_idx]
					.mailbox_info.source_state,
				mailboxes[global_mailbox_idx]
					.mailbox_info.target_state);
		}
		mailboxes[global_mailbox_idx].mailbox_info.source_pid = pid;
		mailboxes[global_mailbox_idx].mailbox_info.source_restarts = 0;
		mailboxes[global_mailbox_idx].mailbox_info.source_client_mode =
			client_mode;
	}

	if (client_mode == XPC_CLIENT_IS_INDIRECT) {
		spin_lock_irqsave(&indirect_client_count_lock, flags);
		atomic_inc(&indirect_client_count);
		spin_unlock_irqrestore(&indirect_client_count_lock, flags);
	}
}

void xpc_clr_global_mailbox_pid(int global_mailbox_idx, bool is_target)
{
	unsigned long flags;
	bool is_indirect = false;

	if (is_target) {
		mailboxes[global_mailbox_idx].mailbox_info.target_pid =
			XPC_INVALID_CLIENT_PID;
		mailboxes[global_mailbox_idx].mailbox_info.target_restarts = 0;
		if (mailboxes[global_mailbox_idx]
			    .mailbox_info.target_client_mode ==
		    XPC_CLIENT_IS_INDIRECT) {
			is_indirect = true;
		}
		mailboxes[global_mailbox_idx].mailbox_info.target_client_mode =
			XPC_CLIENT_IS_UNKNOWN;
	} else {
		mailboxes[global_mailbox_idx].mailbox_info.source_pid =
			XPC_INVALID_CLIENT_PID;
		mailboxes[global_mailbox_idx].mailbox_info.source_restarts = 0;
		if (mailboxes[global_mailbox_idx]
			    .mailbox_info.source_client_mode ==
		    XPC_CLIENT_IS_INDIRECT) {
			is_indirect = true;
		}
		mailboxes[global_mailbox_idx].mailbox_info.source_client_mode =
			XPC_CLIENT_IS_UNKNOWN;
	}

	if (is_indirect) {
		spin_lock_irqsave(&indirect_client_count_lock, flags);
		atomic_dec(&indirect_client_count);
		spin_unlock_irqrestore(&indirect_client_count_lock, flags);
	}
}

bool xpc_check_indirect_clients_active(void)
{
	unsigned long flags;
	bool is_active = false;

	spin_lock_irqsave(&indirect_client_count_lock, flags);
	if (atomic_read(&indirect_client_count) != 0) {
		is_active = true;
	}
	spin_unlock_irqrestore(&indirect_client_count_lock, flags);

	return is_active;
}

bool xpc_check_global_mailbox_pid(int global_mailbox_idx, pid_t pid,
				  bool is_target)
{
	if (is_target)
		return mailboxes[global_mailbox_idx].mailbox_info.target_pid ==
		       pid;
	else
		return mailboxes[global_mailbox_idx].mailbox_info.source_pid ==
		       pid;
}

void xpc_get_ipc_id_and_mailbox(int global_mailbox_idx, int *ipc_id,
				int *mailbox_idx)
{
	*ipc_id = global_mailbox_idx / XPC_IPC_NUM_MAILBOXES;
	*mailbox_idx = global_mailbox_idx % XPC_IPC_NUM_MAILBOXES;
}

void xpc_get_global_mailbox_idx_from_ipc_id_and_mailbox_idx(
	int ipc_id, int mailbox_idx, int *global_mailbox_idx)
{
	if (global_mailbox_idx == NULL) {
		return;
	}

	if (mailbox_idx >= XPC_IPC_NUM_MAILBOXES ||
	    ipc_id >= XPC_NUM_IPC_DEVICES) {
		*global_mailbox_idx = -1;
	} else {
		*global_mailbox_idx =
			ipc_id * XPC_IPC_NUM_MAILBOXES + mailbox_idx;
	}
}

struct xpc_mailbox *
xpc_get_global_mailbox_from_global_mailbox_idx(int global_mailbox_idx)
{
	if (global_mailbox_idx >= (XPC_IPC_NUM_MAILBOXES * XPC_NUM_IPC_DEVICES))
		return NULL;

	return &mailboxes[global_mailbox_idx];
}

struct xpc_mailbox *
xpc_get_global_mailbox_from_ipc_id_and_mailbox_idx(int ipc_id, int mailbox_idx)
{
	int global_idx;

	xpc_get_global_mailbox_idx_from_ipc_id_and_mailbox_idx(
		ipc_id, mailbox_idx, &global_idx);

	return xpc_get_global_mailbox_from_global_mailbox_idx(global_idx);
}

int xpc_claim_global_mailbox(struct xpc_info info)
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

			if (mb_info->xpc_channel != info.xpc_channel) {
				spin_unlock_irqrestore(mb_lock, flags);
				continue;
			}

			if (info.xpc_mode != XPC_MODE_NOTIFICATION &&
			    info.xpc_mode != XPC_MODE_COMMAND) {
				spin_unlock_irqrestore(mb_lock, flags);
				continue;
			}

			if (info.xpc_mode == XPC_MODE_NOTIFICATION &&
			    mb_info->target_state !=
				    MAILBOX_STATE_NOTIFICATION_PENDING) {
				spin_unlock_irqrestore(mb_lock, flags);
				continue;
			}

			if (info.xpc_mode == XPC_MODE_COMMAND &&
			    mb_info->target_state !=
				    MAILBOX_STATE_COMMAND_PENDING) {
				spin_unlock_irqrestore(mb_lock, flags);
				continue;
			}

			mb_info->target_state =
				info.xpc_mode == XPC_MODE_COMMAND ?
					MAILBOX_STATE_COMMAND_CLAIMED :
					MAILBOX_STATE_NOTIFICATION_CLAIMED;

			spin_unlock_irqrestore(mb_lock, flags);
			return m;
		}
	}
	// someone else has already claimed mailbox
	return -1;
}
