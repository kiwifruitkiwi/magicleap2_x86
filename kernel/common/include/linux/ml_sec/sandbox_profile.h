/*
 * Copyright (c) 2017, Magic Leap, Inc. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef __MLSEC_SBOX_SANDBOX_PROFILE_H
#define __MLSEC_SBOX_SANDBOX_PROFILE_H

#ifndef MLSEC_HOSTPROG  /* on host progs, conflicts with the host fd_set */
#include <linux/types.h>
#endif
#ifdef CONFIG_MLSEC_SANDBOX_DEVELOP
#include <linux/atomic.h>
#endif

/*
 * defining those macros here since the file is included by both user space
 * utility and kernel code
 */
#define TYPEDEF typedef
#define STATIC_ASSERT_PRF(COND, MSG)\
TYPEDEF char static_assertion_##MSG[(COND) ? 1 : -1]

#define SIZEOF_MEMBER(type, member) sizeof(((type *)0)->member)

typedef uint64_t sandbox_flags_t;

struct sandbox_profile {
	/* ProfileTag that identifies this sandbox profile */
	uint32_t profile_tag;

#ifdef CONFIG_MLSEC_SANDBOX_DEVELOP
	atomic_t usage;
#endif

	union {
		struct {
			/* Should NO_NEW_PRIVS be enabled */
			bool is_no_new_privs : 1;

			/* Enforce process' jail */
			bool enforce_jail : 1;

			/* Should the kernel enable mapping RWX sections */
			bool enable_jit : 1;

			/* Is syscall filtering permissive */
			bool permissive_syscalls : 1;

			/* Should the kernel allow all localhost connections */
			bool allow_local_connections : 1;

			/* options for allowing localhost connections */
			/* process with this is allowed to connect to itself */
			bool allow_local_conn_sameproc : 1;

			/* enforce syscalls filtering */
			/* This field is not in use by the kernel code, but that field
			* is written in case of dynamic profile files, therefore, it must
			* be allocated to prevent potential memory corruption.
			*/
			bool enforce_syscalls_filtering : 1;

			/**
			 * permits a client, local socket to connect/send
			 * data to group
			 */
			union {
				struct {
					bool group_A : 1;
					bool group_B : 1;
					bool group_C : 1;
					bool group_D : 1;
					bool group_E : 1;
				};
				uint8_t bits;
			} allow_local_transmit;

			/**
			 * permission bit for a server local socket to
			 * accept/receive data from group
			 */
			union {
				struct {
					bool group_A : 1;
					bool group_B : 1;
					bool group_C : 1;
					bool group_D : 1;
					bool group_E : 1;
				};
				uint8_t bits;
			} allow_local_receive;
		};
		sandbox_flags_t flags;
	} attributes;

	/* Syscalls white-list filter to be applied */
	const uint8_t *syscall_filter;
#ifdef CONFIG_COMPAT
	const uint8_t *syscall_filter_compat;
#endif
} __attribute__((aligned(16)));

/* assert that the flag field is big enough to contain all permission bits */
STATIC_ASSERT_PRF(SIZEOF_MEMBER(struct sandbox_profile, attributes.flags) ==
	SIZEOF_MEMBER(struct sandbox_profile, attributes),
	  attributes_flags_size_is_too_small);

/*
 * assert that the number of group is smaller that the number of groups that
 * local_receive_bits.bits can hold
 */
STATIC_ASSERT_PRF(SIZEOF_MEMBER(struct sandbox_profile,
	attributes.allow_local_receive) ==
	  SIZEOF_MEMBER(struct sandbox_profile,
	    attributes.allow_local_receive.bits),
	allow_local_receive_flags_field_is_too_small);

/*
 * assert that the number of group is smaller that the number of groups that
 * allow_local_transmit.bits can hold
 */
STATIC_ASSERT_PRF(SIZEOF_MEMBER(struct sandbox_profile,
	attributes.allow_local_transmit) ==
	  SIZEOF_MEMBER(struct sandbox_profile,
	attributes.allow_local_transmit.bits),
	allow_local_transmit_flags_field_is_too_small);

/*
 * number of groups in local receive must be equal to the
 * number of grous in local transmit
 */
STATIC_ASSERT_PRF(SIZEOF_MEMBER(struct sandbox_profile,
	attributes.allow_local_transmit) ==
	  SIZEOF_MEMBER(struct sandbox_profile,
	    attributes.allow_local_receive),
	num_of_receive_groups_must_match_num_of_transmit_groups);

const struct sandbox_profile *sbox_lookup_profile(uint32_t tag);

#endif	/* __MLSEC_SBOX_SANDBOX_PROFILE_H */
