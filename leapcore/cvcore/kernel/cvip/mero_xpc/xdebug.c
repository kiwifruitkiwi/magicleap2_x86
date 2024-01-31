
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

#include "xdebug_internal.h"

#include <linux/jiffies.h>
#include <linux/kthread.h>
#include <linux/ktime.h>
#include <linux/sched/task.h>

#include "mero_xpc_common.h"
#include "pl320_hal.h"
#include "pl320_types.h"

/**
  * This module implements utilization monitoring of IPC mailboxes.
  * If utilization is sampled to be higher than a configurable threshold,
  * details of the in use mailboxes are logged in dmesg.
  *
  * To enable:
  * echo "l:e:$IPC_MASK:$UTILIZATION_THRESHOLD:$SAMPLE_PERIOD_US > /dev/mero_xpc
  *
  * These flags correspond to the following:
  * l - logging
  * e - Enable
  *
  * $IPC_MASK - is a 5-bit mask and defined as [4:0]: IPC4,IPC3,IPC2,IPC1,IPC0.
  * Only IPC2 and IPC3 device masks are allowed.
  *
  * $UTILIZATION_THRESHOLD - is the number of mailboxes that have to be in use
  * when the monitor samples for an event to be logged. The minimum value is 0.
  *
  * $SAMPLE_PERIOD_US - 1/SAMPLE_PERIOD_US is the rate that the monitor
  * will sample the IPC devices for stats. The minimum value is 250us.
  *
  * To disable:
  * echo "l:d" > /dev/mero_xpc
  *
  * These flags correspond to the following
  * l - logging
  * d - disable
  *
  */

#ifdef XDEBUG_HAS_EVENT_TRACE
struct xpc_debug_event {
	uint64_t ts;
	uint32_t value;
	uint8_t event_id;
	uint8_t core_id;
};

#define MAX_XPC_DEBUG_EVENTS (32)

static int _debug_event_cnt[2][XPC_IPC_NUM_MAILBOXES] = { 0 };
static struct xpc_debug_event _debug_events[2][XPC_IPC_NUM_MAILBOXES]
					   [MAX_XPC_DEBUG_EVENTS];

#endif

struct xdebug_struct xdebug_struct;

static int xdebug_kthread(void *arg);
static int xdebug_configure(uint8_t ipc_dev_mask, int threshold, int period_us,
			    int ratelimit_event_count,
			    int ratelimit_timeout_us);

void xdebug_init(void)
{
	xdebug_struct.state = XD_THREAD_IDLE;

	xdebug_configure(IPC2_MASK | IPC3_MASK,
			 XDEBUG_DEFAULT_CONTENTION_THRESHOLD,
			 XDEBUG_SAMPLE_PERIOD_MINIMUM_USEC,
			 XDEBUG_DEFAULT_RATELIMIT_THRESHOLD,
			 XDEBUG_DEFAULT_RATELIMIT_TIMEOUT_US);
}

static void xdebug_start(void)
{
	spin_lock(&(xdebug_struct.lock));
	if (xdebug_struct.state == XD_THREAD_IDLE) {
		xdebug_struct.state = XD_THREAD_RUNNING;
		spin_unlock(&(xdebug_struct.lock));
		xdebug_struct.task =
			kthread_run(xdebug_kthread, NULL, XDEBUG_KTHREAD_NAME);
		xpc_debug(0, "Started xdebug logging\n");
	} else {
		spin_unlock(&(xdebug_struct.lock));
	}
}

void xdebug_stop(void)
{
	spin_lock(&(xdebug_struct.lock));
	if (xdebug_struct.state == XD_THREAD_RUNNING) {
		xdebug_struct.state = XD_THREAD_STOP_REQUEST;
		spin_unlock(&(xdebug_struct.lock));
		if (xdebug_struct.task) {
			kthread_stop(xdebug_struct.task);
			xpc_debug(0, "Disabled xdebug logging\n");
		}
		xdebug_struct.state = XD_THREAD_IDLE;
	} else {
		spin_unlock(&(xdebug_struct.lock));
	}
}

static int xdebug_configure(uint8_t ipc_dev_mask, int threshold, int period_us,
			    int ratelimit_event_count, int ratelimit_timeout_us)
{
	if (ipc_dev_mask & ~(IPC2_MASK | IPC3_MASK)) {
		xpc_debug(0, "Failure: IPC device mask invalid\n");
		return -EINVAL;
	}

	if (threshold < 0) {
		xpc_debug(0, "Failure: contention threshold out of range\n");
		return -EINVAL;
	}

	if (period_us < XDEBUG_SAMPLE_PERIOD_MINIMUM_USEC) {
		xpc_debug(0, "Failure: sample period must be at least %dus\n",
			  XDEBUG_SAMPLE_PERIOD_MINIMUM_USEC);
		return -EINVAL;
	}

	if (ratelimit_event_count < XDEBUG_RATELIMIT_THRESHOLD_MINIMUM) {
		xpc_debug(
			0,
			"Failure: ratelimit event threshold must be at least %d\n",
			XDEBUG_RATELIMIT_THRESHOLD_MINIMUM);
		return -EINVAL;
	}

	if (ratelimit_timeout_us < XDEBUG_RATELIMIT_TIMEOUT_MINIMUM_US) {
		xpc_debug(0,
			  "Failure: ratelimit timeout must be at least %dus\n",
			  XDEBUG_RATELIMIT_TIMEOUT_MINIMUM_US);
		return -EINVAL;
	}

	atomic_set(&(xdebug_struct.ipc_dev_mask), ipc_dev_mask);
	atomic_set(&(xdebug_struct.contention_thresh), threshold);
	atomic_set(&(xdebug_struct.sample_period_us), period_us);
	atomic_set(&(xdebug_struct.ratelimit_event_count),
		   ratelimit_event_count);
	atomic_set(&(xdebug_struct.ratelimit_timeout_us), ratelimit_timeout_us);

	xpc_debug(0,
		  "Configured xdebug: IPC=0x%02X, Threshold=%d, Sample=%dus,"
		  " Rate-limit=%d, Rate-limit Timeout=%dus\n",
		  ipc_dev_mask, threshold, period_us, ratelimit_event_count,
		  ratelimit_timeout_us);
	return 0;
}

int xdebug_parse_command(const char *buf)
{
	int ret;
	char cmd;
	char sub_cmd;
	int opt1;
	int opt2;
	int opt3;
	int opt4;
	int opt5;

	int ipc_dev_mask;
	int thresh;
	int period_us;
	int rl_event_count;
	int rl_timeout_us;

	ret = sscanf(buf, "%c:%c:%i:%i:%i:%i:%i", &cmd, &sub_cmd, &opt1, &opt2,
		     &opt3, &opt4, &opt5);

	if (cmd != 'l' && (sub_cmd != 'e' || sub_cmd != 'd')) {
		return -EINVAL;
	}

	if (sub_cmd == 'e') {
		if (ret != 2 && ret != 3 && ret != 4 && ret != 5 && ret != 6 &&
		    ret != 7) {
			return -EINVAL;
		}

		ipc_dev_mask = atomic_read(&(xdebug_struct.ipc_dev_mask));
		if (ret >= 3) {
			ipc_dev_mask = opt1;
			xpc_debug(0, "Setting ipc_dev_mask to 0x%02X\n",
				  ipc_dev_mask);
		}

		thresh = atomic_read(&(xdebug_struct.contention_thresh));
		if (ret >= 4) {
			thresh = opt2;
			xpc_debug(0, "Setting contention threshold to %d\n",
				  thresh);
		}

		period_us = atomic_read(&(xdebug_struct.sample_period_us));
		if (ret >= 5) {
			period_us = opt3;
			xpc_debug(0, "Setting sample period to %d us\n",
				  period_us);
		}

		rl_event_count =
			atomic_read(&(xdebug_struct.ratelimit_event_count));
		if (ret >= 6) {
			rl_event_count = opt4;
			xpc_debug(0, "Setting rate limit event count to %d\n",
				  rl_event_count);
		}

		rl_timeout_us =
			atomic_read(&(xdebug_struct.ratelimit_timeout_us));
		if (ret >= 7) {
			rl_timeout_us = opt5;
			xpc_debug(0, "Setting rate limit timeout to %d us\n",
				  rl_timeout_us);
		}

		if (xdebug_configure(ipc_dev_mask, thresh, period_us,
				     rl_event_count, rl_timeout_us) == 0) {
			xdebug_start();
		}
	} else if (sub_cmd == 'd') {
		xdebug_stop();
	}
	return 0;
}

static void xdebug_print_stats(pl320_debug_info *dinfo)
{
	int i = 0;

	/**
	 * Print header
	 */
	xpc_debug(0,
		  "--- START IPC[%u]"
		  "------------------------------------------------"
		  "-------\n",
		  dinfo->ipc_id);

	/**
	 * Print mailbox usage
	 */
	xpc_debug(0, "%u Mailboxes in use - \n", dinfo->num_in_use);
	for (i = 0; i < XPC_IPC_NUM_MAILBOXES; ++i) {
		if (dinfo->active_source[i] != 0) {
			xpc_debug(0,
				  "MB[%2u] - S=%08X,T=%08X,MD=%X,MS=%08X,SD=%X,"
				  "D0=%08X,D1=%08X,D2=%08X,D6=%08X\n",
				  i, dinfo->active_source[i],
				  dinfo->active_target[i], dinfo->mode_reg[i],
				  dinfo->mstatus_reg[i], dinfo->send_reg[i],
				  dinfo->data_reg_0_1_2_6[i][0],
				  dinfo->data_reg_0_1_2_6[i][1],
				  dinfo->data_reg_0_1_2_6[i][2],
				  dinfo->data_reg_0_1_2_6[i][3]);
		}
	}

	/**
	 * Print global IRQ info
	 */
	xpc_debug(0, "    - IRQ Info -    \n");
	for (i = 0; i < XPC_IPC_NUM_CHANNELS; ++i) {
		xpc_debug(0, "M[%2u] = 0x%08X R[%2u] = 0x%08X\n", i,
			  dinfo->irq_mask[i], i, dinfo->irq_raw[i]);
	}

	/**
	 * Print trailer
	 */
	xpc_debug(0,
		  "--- END IPC[%u]"
		  "--------------------------------------------"
		  "--------"
		  "-----\n\n",
		  dinfo->ipc_id);
}

static int xdebug_kthread(void *arg)
{
	int ipc;
	int ipc_dev_mask;
	int ratelimit_threshold;
	bool rate_limited[XPC_NUM_IPC_DEVICES] = { false };
	int rl_event_count[XPC_NUM_IPC_DEVICES] = { 0 };
	u64 rl_window_start_ns[XPC_NUM_IPC_DEVICES] = { 0 };
	u64 now_ns;
	unsigned long jiffies;
	u64 ratelimit_window_us;
	u64 ratelimit_window_ns;

	pl320_debug_info dinfo;

	while (!kthread_should_stop()) {
		// Sleep
		jiffies = usecs_to_jiffies(
			atomic_read(&(xdebug_struct.sample_period_us)));
		// Change state from TASK_RUNNING to TASK_INTERRUPTIBLE
		// so that we can sleep with schedule_timeout
		set_current_state(TASK_INTERRUPTIBLE);
		schedule_timeout(jiffies);

		ipc_dev_mask = atomic_read(&(xdebug_struct.ipc_dev_mask));
		ratelimit_threshold =
			atomic_read(&(xdebug_struct.ratelimit_event_count));
		ratelimit_window_us =
			atomic_read(&(xdebug_struct.ratelimit_timeout_us));
		ratelimit_window_ns = ratelimit_window_us * 1000;

		for (ipc = 0; ipc < XPC_NUM_IPC_DEVICES; ++ipc) {
			if ((ipc_dev_mask >> ipc) & 0x1) {
				if (rate_limited[ipc]) {
					/* Is it time to reset the limiter? */
					now_ns = ktime_get_ns();
					if ((now_ns - rl_window_start_ns[ipc]) >
					    ratelimit_window_ns) {
						rl_event_count[ipc] = 0;
						rate_limited[ipc] = false;
						xpc_debug(
							0,
							"************ Re-enabled logging for IPC[%u] "
							"***********\n",
							ipc);
					}
				}

				if (!rate_limited[ipc]) {
					/* Check if we've logged too much this period */
					if (rl_event_count[ipc] >=
					    ratelimit_threshold) {
						rate_limited[ipc] = true;
						rl_window_start_ns[ipc] =
							ktime_get_ns();
						xpc_debug(
							0,
							"************ Rate limiting "
							"IPC[%u] logs for %lld usecs ***********\n",
							ipc,
							ratelimit_window_us);
					}
				}

				if (!rate_limited[ipc]) {
					// Grab usage stats from PL320
					pl320_get_debug_info(&dinfo, ipc);

					// Log only if the mailbox utilization is above
					// the configured contention threshold
					if (dinfo.num_in_use >
					    atomic_read(&(
						    xdebug_struct
							    .contention_thresh))) {
						rl_event_count[ipc]++;
						xdebug_print_stats(&dinfo);
					}
				}
			}
		}
	}
	// Close thread
	do_exit(0);

	return 0;
}

#ifdef XDEBUG_HAS_EVENT_TRACE
static const char *xpc_debug_evt_str[] = {
	"XPC_EVT_NONE",
	"XPC_EVT_SET_SOURCE",
	"XPC_EVT_SET_DEST",
	"XPC_EVT_SET_MASK",
	"XPC_EVT_CLR_MASK_START",
	"XPC_EVT_CLR_MASK_END",
	"XPC_EVT_SET_MODE",
	"XPC_EVT_SEND",
	"XPC_EVT_ACK",
	"XPC_EVT_RELEASE_START",
	"XPC_EVT_RELEASE_END",
	"XPC_EVT_CLR_DEST_START",
	"XPC_EVT_CLR_DEST_END",
	"XPC_EVT_RD_MASK_START",
	"XPC_EVT_RD_MASK_END",
	"XPC_EVT_RD_MODE_START",
	"XPC_EVT_RD_MODE_END",
	"XPC_EVT_MAILBOX_ACQ",
	"XPC_EVT_RELEASE_CID_MASK_START",
	"XPC_EVT_RELEASE_CID_MASK_END",
	"XPC_EVT_SET_CID_MASK_START",
	"XPC_EVT_SET_CID_MASK_END",
	"XPC_EVT_IRQ_HANDLER_EXIT_EARLY",
	"XPC_EVT_IRQ_COMMAND_ACK_SOURCE_COMPLETED_START",
	"XPC_EVT_IRQ_COMMAND_ACK_SOURCE_COMPLETED_END",
	"XPC_EVT_IRQ_NOTIFICATION_ACK_SOURCE_CLOSED",
	"XPC_EVT_IRQ_NOTIFICATION_ACK_SOURCE_COMPLETED_START",
	"XPC_EVT_IRQ_NOTIFICATION_ACK_SOURCE_COMPLETED_END",
	"XPC_EVT_IRQ_QUEUE_ACK_SOURCE_CLOSED",
	"XPC_EVT_IRQ_COMMAND_IN_FAST_HANDLE_START",
	"XPC_EVT_IRQ_COMMAND_IN_FAST_HANDLE_END",
	"XPC_EVT_IRQ_NOTIFICATION_IN_FAST_HANDLE_START",
	"XPC_EVT_IRQ_NOTIFICATION_IN_FAST_HANDLE_END",
	"XPC_EVT_IRQ_WORK_SCHEDULED",
	"XPC_EVT_IRQ_ENTER",
	"XPC_EVT_NOTIFICATION_WAIT_FOR_COMPLETION_START",
	"XPC_EVT_NOTIFICATION_WAIT_FOR_COMPLETION_END",
	"XPC_EVT_NOTIFICATION_WAIT_FOR_COMPLETION_RESTARTSYS",
	"XPC_EVT_TASKLET_START",
	"XPC_EVT_REPLY",
	"XPC_EVT_NOTIFICATION_MAILBOX_ACQ",
	"XPC_EVT_CMD_WAKEUP_QUEUE",
	"XPC_EVT_TASKLET_END",
	"XPC_EVT_CMD_WAITER_WOKE"
};

void __xdebug_event_record_timed(uint8_t ipc_idx, uint8_t mb_idx,
				 uint8_t event_id, uint32_t value, uint64_t ts)
{
	uint8_t ipc_idx_adj = 0;
	// Restricting logging to the IPC devices we use for
	// ARM, DSP, and x86
	if (ipc_idx != IPC2_IDX && ipc_idx != IPC3_IDX)
		return;

	if (mb_idx >= XPC_IPC_NUM_MAILBOXES)
		return;

	if (ipc_idx == IPC2_IDX)
		ipc_idx_adj = 0;
	else if (ipc_idx == IPC3_IDX)
		ipc_idx_adj = 1;

	_debug_events[ipc_idx_adj][mb_idx][_debug_event_cnt[ipc_idx_adj][mb_idx] %
					   MAX_XPC_DEBUG_EVENTS]
		.ts = ts;
	_debug_events[ipc_idx_adj][mb_idx][_debug_event_cnt[ipc_idx_adj][mb_idx] %
					   MAX_XPC_DEBUG_EVENTS]
		.event_id = event_id;
	_debug_events[ipc_idx_adj][mb_idx][_debug_event_cnt[ipc_idx_adj][mb_idx] %
					   MAX_XPC_DEBUG_EVENTS]
		.value = value;
	_debug_events[ipc_idx_adj][mb_idx][_debug_event_cnt[ipc_idx_adj][mb_idx] %
					   MAX_XPC_DEBUG_EVENTS]
		.core_id = smp_processor_id();

	_debug_event_cnt[ipc_idx_adj][mb_idx]++;
}

void __xdebug_event_record_all(uint8_t ipc_idx, uint8_t event_id,
			       uint32_t value)
{
	uint64_t ts;
	uint8_t mb_idx;

	// Restricting logging to the IPC devices we use for
	// ARM, DSP, and x86
	if (ipc_idx != IPC2_IDX && ipc_idx != IPC3_IDX)
		return;

	ts = xdebug_event_get_ts();

	for (mb_idx = 0; mb_idx < XPC_IPC_NUM_MAILBOXES; mb_idx++) {
		xdebug_event_record_timed(ipc_idx, mb_idx, event_id, value, ts);
	}
}

void __xdebug_event_record(uint8_t ipc_idx, uint8_t mb_idx, uint8_t event_id,
			   uint32_t value)
{
	uint64_t ts = 0;

	ts = xdebug_event_get_ts();
	xdebug_event_record_timed(ipc_idx, mb_idx, event_id, value, ts);
}

static int xdebug_event_sort(uint8_t ipc_idx, uint8_t mb_idx,
			     struct xpc_debug_event sorted_events[])
{
	int i;
	int j;
	struct xpc_debug_event item;
	int num_events;

	num_events = _debug_event_cnt[ipc_idx][mb_idx] > MAX_XPC_DEBUG_EVENTS ?
			     MAX_XPC_DEBUG_EVENTS :
			     _debug_event_cnt[ipc_idx][mb_idx];

	for (i = 0; i < num_events; i++) {
		sorted_events[i] = _debug_events[ipc_idx][mb_idx][i];
	}

	for (i = 1; i < num_events; i++) {
		item = sorted_events[i];
		j = i - 1;

		while (j >= 0 && sorted_events[j].ts > item.ts) {
			sorted_events[j + 1] = sorted_events[j];
			j = j - 1;
		}
		sorted_events[j + 1] = item;
	}

	return num_events;
}

void xdebug_event_dump(uint8_t ipc_idx, uint8_t mb_idx)
{
	int i;
	int num_events;
	uint8_t ipc_idx_adj = 0;
	struct xpc_debug_event sorted_events[MAX_XPC_DEBUG_EVENTS];

	// Restricting logging to the IPC devices we use for
	// ARM, DSP, and x86
	if (ipc_idx != IPC2_IDX && ipc_idx != IPC3_IDX)
		return;

	if (mb_idx >= XPC_IPC_NUM_MAILBOXES)
		return;

	if (ipc_idx == IPC2_IDX)
		ipc_idx_adj = 0;
	else if (ipc_idx == IPC3_IDX)
		ipc_idx_adj = 1;

	num_events = xdebug_event_sort(ipc_idx_adj, mb_idx, sorted_events);

	for (i = 0; i < num_events; i++) {
		if (sorted_events[i].event_id == XPC_EVT_MAILBOX_ACQ) {
			printk("--------------------------------------------------------------------\n");
		}
		printk("IPC_%u - mailbox[%02u] - core %d - ts = %llu - value = 0x%08X - evt = %s\n",
		       ipc_idx, mb_idx, sorted_events[i].core_id,
		       sorted_events[i].ts, sorted_events[i].value,
		       xpc_debug_evt_str[sorted_events[i].event_id]);
		if (sorted_events[i].event_id == XPC_EVT_SEND) {
			printk("----------------------------------------------------------------->>>\n");
		}

		if (sorted_events[i].event_id == XPC_EVT_REPLY) {
			printk("----------------------------------------------------------------->>>\n");
		}
	}
}
#else
void xdebug_event_dump(uint8_t ipc_idx, uint8_t mb_idx)
{
	(void)ipc_idx;
	(void)mb_idx;
}
#endif
