/* SPDX-License-Identifier: GPL-2.0-only
 *
 * Copyright (c) 2020-2022, Magic Leap, Inc. All rights reserved.
 */

#ifndef __MERO_XPC_TEST_H
#define __MERO_XPC_TEST_H

#define ENABLE_STARTUP_TEST

enum xpc_test_type {
	kCRNormalPriority = 0,
	kCRHighPriority
};
/**
 * Perform a self-test of the xpc command/response
 *
 * \param [in]   type    The type of test to run
 *
 * \return       0 on success\n
 *               -EIO on test failure
 */
int xpc_test_command_response(enum xpc_test_type type);

#ifndef __KERNEL__
#error "Can't include kernel header in userspace"
#endif

#endif
