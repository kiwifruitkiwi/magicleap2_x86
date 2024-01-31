/* SPDX-License-Identifier: GPL-2.0-only
 *
* \file nibble.h
*
* \brief  Common definitions needed to make gsm work
 *
 * Copyright (c) 2020-2022, Magic Leap, Inc. All rights reserved.
 */

#ifndef GSM_NIBBLE_H
#define GSM_NIBBLE_H

#include "gsm.h"

// Addr must be 32-bit aligned.  Nibble is a count of which nibble to modify, with 0 being the
// least significant nibble.

// Attempt to claim the nibble - will succeed only if the current value is 0.
// You should check the return value.  A 0 indicates you successfully made the switch.
// Non-0 indicates failure, along with the AXI-ID of the current owner.
// Note that it's possible for another thread on the same AXI-ID to own the nibble,
// so it is not valid to just read the new value after the operation to determine ownership.
#define nibble_test_and_set_axi(addr, nibble) GSM_READ_32((GSMI_READ_COMMAND_NIBBLE_TEST_AND_SET_BASE + nibble), addr)

/**
* Same as nibble_test_and_set_axi except provides memory ordering guarantee that ensures loads/stores
* that occur after a call to this function are not "hoisted" and executed before the call to this function.
*/
CVCORE_GSM_ALWAYS_INLINE uint32_t nibble_test_and_set_axi_sync(uint32_t addr, int nibble) {
	volatile uint32_t* tas_addr = GSM_ADDRESS_32((GSMI_READ_COMMAND_NIBBLE_TEST_AND_SET_BASE + (uint32_t)nibble), addr);
	return __atomic_load_n(tas_addr, __ATOMIC_ACQUIRE);
}

// Raw write to the given nibble.  Used to clear the AXI-ID during the release process.
#define nibble_write(addr, nibble, val) GSM_WRITE_32((GSMI_WRITE_COMMAND_NIBBLE_WRITE_BASE + nibble), addr, val)

/**
* Same as nibble write except provides memory ordering guarantee that ensures loads/stores that occur
* before a call to this function have retired and DO NOT "sink" below the call to this function.
*/
CVCORE_GSM_ALWAYS_INLINE void nibble_write_sync(uint32_t addr, int nibble, uint32_t val) {
	volatile uint32_t* tas_addr = GSM_ADDRESS_32((GSMI_WRITE_COMMAND_NIBBLE_WRITE_BASE + (uint32_t)nibble), addr);
	__atomic_store_n(tas_addr, val, __ATOMIC_RELEASE);
}

#ifndef __KERNEL__
#error "Can't include kernel header in userspace"
#endif

#endif /* GSM_NIBBLE_H */
