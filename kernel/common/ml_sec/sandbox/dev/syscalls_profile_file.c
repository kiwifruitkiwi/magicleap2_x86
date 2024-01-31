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

#ifndef MLSEC_HOSTPROG
#include <linux/string.h>
#include <linux/printk.h>
#endif

#include "../inc_unistd.h"
#include <linux/ml_sec/sandbox.h>
#include "syscalls_profile_file.h"
#include "profile_file.h"
#include "../syscall_name.h"
#include "../sandbox_profile_tags.h"

#define SYSCALLS_PROFILE_EXT "syscalls.profile"

static inline void set_syscall_action(uint8_t *filter, int nr, uint8_t action)
{
	uint8_t *filter_elem = &filter[SBOX_SYS_FILTER_ELEM_INDEX(nr)];
	int offset = SBOX_SYS_FILTER_ELEM_OFFSET(nr);
	*filter_elem = (*filter_elem & ~(SANDBOX_ACTIONS_MASK << offset)) |
		       (action << offset);
}

static inline void merge_filter_element(uint8_t *dst, const uint8_t *src)
{
	uint8_t d = *dst;
	uint8_t s = *src;
	uint8_t mask;

	if (d == s)
		return;

	/*
	 * Make a mask for every 2 bits that have at least 1 of them set.
	 * For example, for the number 0b01100001, the resulting mask is:
	 * 0b11110011.
	 */
	mask = ((s >> 1) & 0x55) | ((s << 1) & 0xAA);
	*dst = (d & ~mask) | s;
}

static void merge_syscall_filters(uint8_t *dst, const uint8_t *src, size_t n)
{
	size_t i;

	for (i = 0; i < n; ++i, ++dst, ++src)
		merge_filter_element(dst, src);
}

static void merge_syscalls(struct sandbox_profile *dst_profile,
			   const char *src_profile_name)
{
	const struct sandbox_profile *src_profile;
	uint32_t src_profile_tag;

	memcpy((char *)&src_profile_tag, src_profile_name, sizeof(uint32_t));
	src_profile = sbox_lookup_profile(src_profile_tag);
	if (!src_profile)
		return;

	merge_syscall_filters((uint8_t *)dst_profile->syscall_filter,
			      src_profile->syscall_filter,
			      SBOX_SYS_FILTER_SIZE(__X_NR_syscalls));
#if defined(CONFIG_COMPAT) && defined(CONFIG_ARM64)
	merge_syscall_filters((uint8_t *)dst_profile->syscall_filter_compat,
			      src_profile->syscall_filter_compat,
			      SBOX_SYS_FILTER_SIZE(__X_NR_compat_syscalls));
#endif
}

#ifndef CONFIG_ARM64
static const char* arm_only_names[] = {
	"fstat64", "llseek", "lstat64", "stat64", "statfs64", "fstatat64", "sync_file_range2", "fcntl64",
	"fadvise64_64", "ftruncate64", "truncate64",
	"getgroups16", "setgroups16",
	"getegid16", "geteuid16", "getgid16", "getuid16", "setgid16", "setuid16",
	"getresgid16", "getresuid16", "setresgid16", "setresuid16",
	"chown16", "fchown16", "lchown16",
	"mmap2", "recv", "send", "sendfile",
	"set_tls", "cacheflush",
	"sigaction", "sigpending", "sigprocmask", "sigreturn", "sigsuspend"
};

static bool is_arm_only_syscall(const char* name) {
	int i;
	for(i = 0; i < (sizeof(arm_only_names)/sizeof(arm_only_names[0])); ++i) {
		if (strcmp(name, arm_only_names[i]) == 0)
			return true;
	}
	return false;
}
#endif

static bool parse_syscall_entry(const char *name, void *data)
{
	bool ret = false;
	uint8_t action = SANDBOX_ACTION_ALLOW;
	int nr;
	struct sandbox_profile *sandbox_profile = (struct sandbox_profile *)data;

	if (*name == '#')  /* comment */
		return true;

	if (*name == ':') {
		name++;
		merge_syscalls(sandbox_profile, name);
		return true;
	}

	if (*name == '+') {
		action = SANDBOX_ACTION_INSPECT;
		name++;
	}

	nr = lookup_syscall_nr(name);
	if (nr != -1) {
		set_syscall_action((uint8_t *)sandbox_profile->syscall_filter, nr, action);
		ret = true;
	}

#if defined(CONFIG_COMPAT) && defined(CONFIG_ARM64)
	nr = lookup_syscall_nr_compat(name);
	if (nr != -1) {
		set_syscall_action((uint8_t *)sandbox_profile->syscall_filter_compat, nr, action);
		ret = true;
	}
#endif
#ifndef CONFIG_ARM64
	if (is_arm_only_syscall(name)) {
		/* these names appear in the profiles for arm64 and we don't want to produce an
		 * error because they're not x86 syscalls */
		ret = true;
	}
#endif
	if (!ret)
		pr_err("did not find syscall `%s`\n", name);
	return ret;
}

int sbox_create_syscall_filter_from_file(struct path *exec_path, struct sandbox_profile *sandbox_profile)
{
	int retval;

	retval = sbox_parse_profile_file(exec_path,
					 SYSCALLS_PROFILE_EXT,
					 parse_syscall_entry,
					 sandbox_profile);
	return (retval < 0) ? retval : 0;
}
