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

#include "mero_xpc_test.h"

#include <linux/ktime.h>
#include <linux/timekeeping.h>

#include "cvcore_xchannel_map.h"
#include "mero_xpc_common.h"
#include "mero_xpc_configuration.h"

union xpc_test_message {
	struct __attribute__((packed)) {
		ktime_t send_time;
		ktime_t recv_time;
		ktime_t resp_time;
		uint16_t iteration_number;
	} message_timer;
	uint8_t bytes[XPC_MAX_COMMAND_LENGTH];
};

static_assert(sizeof(union xpc_test_message) == XPC_MAX_COMMAND_LENGTH);

static void xpc_test_print_message(union xpc_test_message *msg)
{
	uint64_t cycle_time_1 =
		(msg->message_timer.recv_time - msg->message_timer.send_time);
	uint64_t cycle_time_2 =
		(msg->message_timer.resp_time - msg->message_timer.recv_time);
	(void)cycle_time_1;
	(void)cycle_time_2;
}

static int xpc_test_command_handler(XpcChannel channel, XpcHandlerArgs args,
				    uint8_t *command_buffer,
				    size_t command_buffer_length,
				    uint8_t *response_buffer,
				    size_t response_buffer_length,
				    size_t *response_length)
{
	union xpc_test_message *ret_msg =
		(union xpc_test_message *)response_buffer;
	*response_length = (command_buffer_length < response_buffer_length) ?
				   command_buffer_length :
				   response_buffer_length;
	*ret_msg = *((union xpc_test_message *)command_buffer);
	ret_msg->message_timer.recv_time = ktime_get();
	return 0;
}

int xpc_test_command_response(enum xpc_test_type type)
{
	XpcAsyncTicket tickets[XPC_IPC_NUM_MAILBOXES] = { XPC_TICKET_INVALID };
	int status;
	size_t response_length;
	uint8_t response_buffer[XPC_MAX_COMMAND_LENGTH] = { 0 };
	union xpc_test_message test_message;
	XpcChannel test_channel_0 = CVCORE_XCHANNEL_TEST0;
	XpcChannel test_channel_1 = CVCORE_XCHANNEL_TEST1;
	const int kNumAsyncIterations = 20;
	const int kNumSynchIterations = 1000;
	int i = 0;
	const char *test_desc[] = { "LP", "HP" };

	test_message.bytes[0] = 0xFF;
	// Register a kernel handler for testing
	if (type == kCRHighPriority) {
		status = xpc_register_command_handler_hi(
			test_channel_0,
			(XpcCommandHandler)&xpc_test_command_handler,
			(XpcHandlerArgs)NULL);
	} else if (type == kCRNormalPriority) {
		status = xpc_register_command_handler(
			test_channel_0,
			(XpcCommandHandler)&xpc_test_command_handler,
			(XpcHandlerArgs)NULL);
	} else {
		return -EINVAL;
	}

	if (status != 0) {
		return -EIO;
	}
	xpc_debug(3, "XPC-%s: self-test phase (1) started\n", test_desc[type]);

	// Phase 1.
	// Perform a few non-blocking command sends. We'll claim the
	// responses later. Until then, kNumAsyncIterations mailboxes
	// will be held claimed.
	for (i = 0; i < kNumAsyncIterations; ++i) {
		test_message.message_timer.send_time = ktime_get();
		test_message.message_timer.iteration_number = i;

		status = xpc_command_send_async(test_channel_1, XPC_CORE_ID,
						test_message.bytes,
						XPC_MAX_COMMAND_LENGTH,
						&tickets[i]);

		if (status != 0) {
			return -EIO;
		}
	}
	xpc_debug(3, "XPC-%s: self-test phase (1) completed\n",
		  test_desc[type]);

	xpc_debug(3, "XPC-%s: self-test phase (2) started\n", test_desc[type]);

	// Phase 2.
	// Perform many synchronous command response operations. There
	// will be (NUM_MAILBOXES - kNumAsyncIterations) number of
	// mailboxes to work with.
	for (i = 0; i < kNumSynchIterations; ++i) {
		test_message.message_timer.send_time = ktime_get();
		test_message.message_timer.iteration_number = i;

		status = xpc_command_send(
			test_channel_0, XPC_CORE_ID, test_message.bytes,
			XPC_MAX_COMMAND_LENGTH, response_buffer,
			XPC_MAX_COMMAND_LENGTH, &response_length);

		if (status != 0) {
			return -EIO;
		}
		test_message.message_timer.resp_time = ktime_get();
		xpc_test_print_message(&test_message);
	}
	xpc_debug(3, "XPC-%s: self-test phase (2) completed\n",
		  test_desc[type]);

	xpc_debug(3, "XPC-%s: self-test phase (3) started\n", test_desc[type]);

	// Phase 3.
	// Process the kNumAsyncIterations number of commands
	// from Phase 1.
	for (i = 0; i < kNumAsyncIterations; ++i) {
		status = xpc_command_recv(
			test_channel_1,
			(XpcCommandHandler)&xpc_test_command_handler,
			(XpcHandlerArgs)NULL);

		if (status != 0) {
			return -EIO;
		}
	}
	xpc_debug(3, "XPC-%s: self-test phase (3) completed\n",
		  test_desc[type]);

	xpc_debug(3, "XPC-%s: self-test phase (4) started\n", test_desc[type]);

	// Phase 4.
	// Receive the kNumAsyncIterations number of responses
	// from Phase 3.
	for (i = 0; i < kNumAsyncIterations; ++i) {
		test_message.message_timer.send_time = ktime_get();
		status = xpc_command_wait_response_async(&tickets[i],
							 response_buffer,
							 XPC_MAX_COMMAND_LENGTH,
							 &response_length);

		if (status != 0) {
			return -EIO;
		}
	}
	xpc_debug(3, "XPC-%s: self-test phase (4) completed\n",
		  test_desc[type]);

	// De-register the kernel handler
	status = xpc_register_command_handler(
		test_channel_0, (XpcCommandHandler)NULL, (XpcHandlerArgs)NULL);

	return 0;
}
