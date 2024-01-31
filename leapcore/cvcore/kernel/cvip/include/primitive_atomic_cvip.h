/* SPDX-License-Identifier: GPL-2.0-only
 *
 * Copyright (c) 2020-2022, Magic Leap, Inc. All rights reserved.
 */

#ifndef GSM_PRIMITIVE_ATOMIC_CVIP_H
#define GSM_PRIMITIVE_ATOMIC_CVIP_H

#include "gsm.h"

/* Read the globally accessible counter (GAC) */
#define gsm_read_gac()                             GSM_READ_64(GSMI_READ_COMMAND_REGISTER, GSMI_REGISTER_GAC)

/* GSM read operations */
#define primitive_atomic_increment(addr)           GSM_READ_32(GSMI_READ_COMMAND_ATOMIC_INCREMENT, addr)
#define primitive_atomic_decrement(addr)           GSM_READ_32(GSMI_READ_COMMAND_ATOMIC_DECREMENT, addr)
#define primitive_atomic_read_current(addr)        GSM_READ_32(GSMI_READ_COMMAND_RAW, addr)
#define primitive_atomic_write(addr, val)          GSM_WRITE_32(GSMI_WRITE_COMMAND_RAW, addr, val)

#define primitive_atomic_find_and_set_32(addr)     GSM_READ_32(GSMI_READ_COMMAND_FIND_AND_SET_32, addr)

/* GSM write operations */
#define primitive_atomic_bitwise_set(addr, val)    GSM_WRITE_32(GSMI_WRITE_COMMAND_ATOMIC_BITSET, addr, val)
#define primitive_atomic_bitwise_clear(addr, val)  GSM_WRITE_32(GSMI_WRITE_COMMAND_ATOMIC_BITCLEAR, addr, val)
#define primitive_atomic_bitwise_xor(addr, val)    GSM_WRITE_32(GSMI_WRITE_COMMAND_ATOMIC_XOR, addr, val)
#define primitive_atomic_add(addr, val)            GSM_WRITE_32(GSMI_WRITE_COMMAND_ATOMIC_ADD, addr, val)
#define primitive_atomic_subtract(addr, val)       GSM_WRITE_32(GSMI_WRITE_COMMAND_ATOMIC_SUBTRACT, addr, val)

#define primitive_atomic_find_and_set_32_ordered(addr)	\
	__atomic_load_n(GSM_ADDRESS_32(GSMI_READ_COMMAND_FIND_AND_SET_32, addr), __ATOMIC_SEQ_CST)

#define primitive_atomic_increment_ordered(addr)	\
	__atomic_load_n(GSM_ADDRESS_32(GSMI_READ_COMMAND_ATOMIC_INCREMENT, addr), __ATOMIC_SEQ_CST)

#define primitive_atomic_decrement_ordered(addr)	\
	__atomic_load_n(GSM_ADDRESS_32(GSMI_READ_COMMAND_ATOMIC_DECREMENT, addr), __ATOMIC_SEQ_CST)

#define primitive_atomic_read_current_ordered(addr)	\
	__atomic_load_n(GSM_ADDRESS_32(GSMI_READ_COMMAND_RAW, addr), __ATOMIC_SEQ_CST)

#define primitive_atomic_write_ordered(addr, val)	\
	__atomic_store_n(GSM_ADDRESS_32(GSMI_WRITE_COMMAND_RAW, addr), val, __ATOMIC_SEQ_CST)

#define primitive_atomic_bitwise_set_ordered(addr, val)	\
	__atomic_store_n(GSM_ADDRESS_32(GSMI_WRITE_COMMAND_ATOMIC_BITSET, addr), val, __ATOMIC_SEQ_CST)

#define primitive_atomic_bitwise_clear_ordered(addr, val)	\
	__atomic_store_n(GSM_ADDRESS_32(GSMI_WRITE_COMMAND_ATOMIC_BITCLEAR, addr), val, __ATOMIC_SEQ_CST)

#define primitive_atomic_bitwise_xor_ordered(addr, val)	\
	__atomic_store_n(GSM_ADDRESS_32(GSMI_WRITE_COMMAND_ATOMIC_XOR, addr), val, __ATOMIC_SEQ_CST)

#define primitive_atomic_add_ordered(addr, val)	\
	__atomic_store_n(GSM_ADDRESS_32(GSMI_WRITE_COMMAND_ATOMIC_ADD, addr), val, __ATOMIC_SEQ_CST)

#define primitive_atomic_subtract_ordered(addr, val)	\
	__atomic_store_n(GSM_ADDRESS_32(GSMI_WRITE_COMMAND_ATOMIC_SUBTRACT, addr), val, __ATOMIC_SEQ_CST)

#ifndef __KERNEL__
#error "Can't include kernel header in userspace"
#endif

#endif /* GSM_PRIMITIVE_ATOMIC_CVIP_H */
