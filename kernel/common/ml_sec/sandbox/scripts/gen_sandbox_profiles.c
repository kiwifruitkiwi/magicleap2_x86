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


/*
 * This file is compiled only for the host, and is built into a small program,
 * that generates the source code for the hardcoded sandbox profiles and
 * syscall filters (as part of the kernel sandbox module build process).
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <limits.h>
#include <ctype.h>
#include <errno.h>
#include <unistd.h>
#include <dirent.h>


/*****************************************************************************/
#include "kernel_utils.h"
/*
 * The following C files contain the code for parsing the
 * .sandbox.profile and .syscalls.profile files.
 * These files are shared between the kernel and the host app.
 *
 * API used:
 * - sbox_create_sandbox_filter_from_file()
 * - sbox_create_syscall_filter_from_file()
 */
#define HOSTPROG
#include "../dev/profile_file.c"
#include "../dev/sandbox_profile_file.c"
#include "../dev/syscalls_profile_file.c"
#include "../syscall_name.c"

#if defined(CONFIG_COMPAT) && defined(CONFIG_ARM64)
#undef __SYSCALL
#include "../syscall_name_compat.c"
#endif
/*****************************************************************************/


struct host_sandbox_profile {
	struct host_sandbox_profile *next;
	struct sandbox_profile profile;

	uint8_t syscall_filter[SBOX_SYS_FILTER_SIZE(__X_NR_syscalls)];
#if defined(CONFIG_COMPAT) && defined(CONFIG_ARM64)
	uint8_t syscall_filter_compat[SBOX_SYS_FILTER_SIZE(__X_NR_compat_syscalls)];
#endif
};

#define TAG_SIZE	(sizeof(uint32_t))

static const char *profiles_path;
static const char *output_path;
static int fd_sandbox_profiles = -1;
static int fd_syscall_filters = -1;
static int fd_syscall_filters_compat = -1;
static struct sbox_parse_state *parse_state;


static struct sandbox_profile *parse_sandbox_profile(const char *path,
						     const char *tagname);

static struct host_sandbox_profile *sandbox_profiles;

static bool sbox_profile_exist(const char *tagname)
{
	uint32_t tag;
	struct host_sandbox_profile *profile;

	memcpy((char *)&tag, tagname, TAG_SIZE);
	for (profile = sandbox_profiles; profile; profile = profile->next)
		if (profile->profile.profile_tag == tag)
			return true;
	return false;
}

const struct sandbox_profile *sbox_lookup_profile(uint32_t tag)
{
	char tagname[TAG_SIZE + 1];
	struct host_sandbox_profile *profile;

	for (profile = sandbox_profiles; profile; profile = profile->next)
		if (profile->profile.profile_tag == tag)
			return &profile->profile;

	/* If the profile is not parsed already, try to parse it. */
	memcpy(tagname, (char *)&tag, TAG_SIZE);
	tagname[TAG_SIZE] = '\0';
	return parse_sandbox_profile(profiles_path, tagname);
}

/* Add a new sandbox_profile to the list, to be used with the given tag. */
static struct sandbox_profile *sbox_add_profile(uint32_t tag)
{
	struct host_sandbox_profile *profile;
	struct host_sandbox_profile **tail_ptr = &sandbox_profiles;

	while ((profile = *tail_ptr)) {
		if (profile->profile.profile_tag == tag)
			return NULL;

		tail_ptr = &profile->next;
	}

	profile = safe_malloc(sizeof(struct host_sandbox_profile));
	memset(profile, 0, sizeof(struct host_sandbox_profile));
	profile->profile.syscall_filter = profile->syscall_filter;
#if defined(CONFIG_COMPAT) && defined(CONFIG_ARM64)
	profile->profile.syscall_filter_compat = profile->syscall_filter_compat;
#endif

	*tail_ptr = profile;
	return &profile->profile;
}

/*
 * Lookup a profile with the exact same syscalls filters, as the given one.
 */
static struct sandbox_profile *lookup_similar_profile(const struct sandbox_profile *orig)
{
	struct host_sandbox_profile *profile;

	for (profile = sandbox_profiles; profile; profile = profile->next) {
		if (profile->profile.profile_tag == orig->profile_tag)
			break;

		if (!memcmp(orig->syscall_filter,
			    profile->syscall_filter,
			    SBOX_SYS_FILTER_SIZE(__X_NR_syscalls))
#if defined(CONFIG_COMPAT) && defined(CONFIG_ARM64)
			    &&
		    !memcmp(orig->syscall_filter_compat,
			    profile->syscall_filter_compat,
			    SBOX_SYS_FILTER_SIZE(__X_NR_compat_syscalls))
#endif
			    )
			return &profile->profile;
	}
	return NULL;
}

static char *join_path(const char *path, const char *name)
{
	size_t lenp;
	size_t lenn;
	char *str;

	if (!path || !*path)
		path = ".";

	lenp = strlen(path);
	lenn = strlen(name);

	if (path[lenp - 1] == '/')
		lenp--;

	str = safe_malloc(lenp + 1 + lenn + 1);
	memcpy(str, path, lenp);
	str[lenp] = '/';
	memcpy(str + lenp + 1, name, lenn + 1);
	return str;
}

#define RECEIVER_AND_TRANMITTER_IN_GROUP(flags, g) \
	((flags & SANDBOX_FILTER_ALLOW_LOCAL_TRANSMIT_GROUP_##g) &&\
	(flags & SANDBOX_FILTER_ALLOW_LOCAL_RECEIVE_GROUP_##g))

static inline bool is_local_host_groups(sandbox_flags_t flags)
{
	return RECEIVER_AND_TRANMITTER_IN_GROUP(flags, A) ||
		RECEIVER_AND_TRANMITTER_IN_GROUP(flags, B) ||
		RECEIVER_AND_TRANMITTER_IN_GROUP(flags, C) ||
		RECEIVER_AND_TRANMITTER_IN_GROUP(flags, D) ||
		RECEIVER_AND_TRANMITTER_IN_GROUP(flags, E);
}

static bool valid_flags_combination(sandbox_flags_t flags, const char *filename)
{
	bool has_localhost_groups = is_local_host_groups(flags);
	bool has_localhost_sameproc = (flags & SANDBOX_FILTER_ALLOW_LOCAL_CONN_SAMEPROC) != 0;
	bool has_localhost_all = (flags & SANDBOX_FILTER_ALLOW_LOCAL_CONNECTIONS) != 0;

	if (has_localhost_groups && has_localhost_sameproc) {
		fprintf(stderr, "allow_local_conn_groups_X supercedes allow_local_conn_sameproc. Don't add both of them in %s\n", filename);
		return false;
	}
	if (has_localhost_all && has_localhost_sameproc) {
		fprintf(stderr, "allow_local_connections supercedes allow_local_conn_sameproc. Don't add both of them in %s\n", filename);
		return false;
	}
	if (has_localhost_all && has_localhost_groups) {
		fprintf(stderr, "allow_local_connections supercedes allow_local_conn_groups_X. Don't add both of them in %s\n", filename);
		return false;
	}
	return true;
}

/* Initializing the data of the sandbox_profile struct. */
static int construct_sandbox_profile_from_file(const char *filename, struct sandbox_profile *profile)
{
	int retval;
	sandbox_flags_t filter_flags = 0;
	bool syscalls;
	struct path path;

	vfs_path_lookup(NULL, NULL, filename, 0, &path);

	/* Parse the sandbox runtime filter file */
	retval = sbox_create_sandbox_filter_from_file(&path, &filter_flags, parse_state);

	if (retval == -ENOENT) {
		/* .sandbox.profile is missing
		 * use default sandbox flags from _ML_ profile.
		 */
		const struct sandbox_profile *ml_profile;
		ml_profile = sbox_lookup_profile(SBOX_PROFILE_TAG__ML_);
		if (!ml_profile)
			return -ENOENT;
		filter_flags = ml_profile->attributes.flags;
	} else if (retval != 0)
		return -EINVAL; /* file content parsing error */

	/* Note: even the bit flags in filter_flags do not correspond to the correct positions of the bits
	 * in the union in struct sandbox_profile. In this generator we read and write the bits according
	 * to sandbox_filters.def, not according to the union. sandbox_filters.def has bits that are not
	 * present in the union (syscalls)
	 * This is why we shouldn't use the union members for accessing bits
	 */
	profile->attributes.flags = filter_flags;
	if (!valid_flags_combination(profile->attributes.flags, filename))
		return -EINVAL;


	syscalls = (filter_flags & (SANDBOX_FILTER_SYSCALLS | SANDBOX_FILTER_SYSCALLS_PERMISSIVE)) != 0;

	/* Check for syscalls runtime filter file - and parse */
	if (syscalls) {
		int error = sbox_create_syscall_filter_from_file(&path, profile);
		if (error) {
			/* If we already had an error parsing the .sandbox.profile file.
			 * Means this tag was missing both files, that's bad
			 */
			if (retval)
				return retval;

			/* If a syscall filter for this profile is not found,
			 * then just default to ML internal. */
			if (error == -ENOENT) {
				const struct sandbox_profile *ml_profile;

				ml_profile = sbox_lookup_profile(SBOX_PROFILE_TAG__ML_);
				if (!ml_profile)
					return -ENOENT;

				memcpy((uint8_t *)profile->syscall_filter,
				       ml_profile->syscall_filter,
				       SBOX_SYS_FILTER_SIZE(__X_NR_syscalls));
#if defined(CONFIG_COMPAT) && defined(CONFIG_ARM64)
				memcpy((uint8_t *)profile->syscall_filter_compat,
				       ml_profile->syscall_filter_compat,
				       SBOX_SYS_FILTER_SIZE(__X_NR_compat_syscalls));
#endif
				error = 0;
			} else
				fprintf(stderr, "Failed to parse %s.syscalls.profile!\n", filename);
		}
		retval = error;
	}
	return retval;
}

static int create_file(const char *path, const char *name)
{
	int fd;
	char *filename = join_path(path, name);
	fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (fd == -1)
		fprintf(stderr, "Cannot create file '%s': %s\n", filename, strerror(errno));
	free(filename);
	return fd;
}

static void delete_file(const char *path, const char *name)
{
	char *filename = join_path(path, name);
	if (unlink(filename) != 0)
		fprintf(stderr, "Cannot unlink file '%s': %s\n", filename, strerror(errno));
	free(filename);
}

static int write_format(int fd, const char *format, ...)
{
	char buf[256];
	int retval;
	va_list args;
	va_start(args, format);
	retval = vsnprintf(buf, sizeof(buf), format, args);
	if ((size_t)retval >= sizeof(buf)) {
		fprintf(stderr, "Failed to format string for write (requires buffer size of %d).\n", retval);
		return -1;
	}
	retval = write(fd, buf, strlen(buf));
	va_end(args);
	return retval;
}

static void write_syscall_filter(int fd, const uint8_t *filter, size_t len, const char *tagname, const char *prefix)
{
	const uint8_t *end = filter + len;

	write_format(fd,
		     "static const uint8_t sbox%s_syscall_filter_mask_%s[%u] __attribute__((aligned(16))) = {%u",
		     prefix, tagname, (unsigned int)len, *filter);
	while (++filter < end)
		write_format(fd, ",%u", *filter);
	write_format(fd, "};\n");
}

static void write_sandbox_profile(int fd, struct sandbox_profile *profile,
				  const char *tagname, const char *sys_tagname)
{
	sandbox_flags_t filter_flags = profile->attributes.flags;

	/* add a comment for readability */
	write_format(fd, "/* %s */ {.attributes.flags = 0, ", tagname);

	if ((filter_flags & SANDBOX_FILTER_SYSCALLS_PERMISSIVE) != 0)
		write_format(fd, "USAGE_INIT(true) ");
	else
		write_format(fd, "USAGE_INIT(false) ");

	if ((filter_flags & SANDBOX_FILTER_NO_NEW_PRIVS) != 0)
		write_format(fd, ".attributes.is_no_new_privs = true, ");

	if ((filter_flags & SANDBOX_FILTER_JAIL) != 0)
		write_format(fd, ".attributes.enforce_jail = true, ");

	if ((filter_flags & SANDBOX_FILTER_ENABLE_JIT) != 0)
		write_format(fd, ".attributes.enable_jit = true, ");

	if ((filter_flags & (SANDBOX_FILTER_SYSCALLS | SANDBOX_FILTER_SYSCALLS_PERMISSIVE)) != 0) {
		write_format(fd, ".syscall_filter = sbox_syscall_filter_mask_%s, ", sys_tagname);
#if defined(CONFIG_COMPAT) && defined(CONFIG_ARM64)
		write_format(fd, ".syscall_filter_compat = sbox_compat_syscall_filter_mask_%s, ", sys_tagname);
#endif
	}

	if ((filter_flags & SANDBOX_FILTER_ALLOW_LOCAL_CONNECTIONS) != 0)
		write_format(fd,
			".attributes.allow_local_connections = true, ");
	if ((filter_flags & SANDBOX_FILTER_ALLOW_LOCAL_CONN_SAMEPROC) != 0)
		write_format(fd,
			".attributes.allow_local_conn_sameproc = true, ");

	if ((filter_flags & SANDBOX_FILTER_ALLOW_LOCAL_TRANSMIT_GROUP_A) != 0)
		write_format(fd,
			".attributes.allow_local_transmit.group_A = true, ");
	if ((filter_flags & SANDBOX_FILTER_ALLOW_LOCAL_TRANSMIT_GROUP_B) != 0)
		write_format(fd,
			".attributes.allow_local_transmit.group_B = true, ");
	if ((filter_flags & SANDBOX_FILTER_ALLOW_LOCAL_TRANSMIT_GROUP_C) != 0)
		write_format(fd,
			".attributes.allow_local_transmit.group_C = true, ");
	if ((filter_flags & SANDBOX_FILTER_ALLOW_LOCAL_TRANSMIT_GROUP_D) != 0)
		write_format(fd,
			".attributes.allow_local_transmit.group_D = true, ");
	if ((filter_flags & SANDBOX_FILTER_ALLOW_LOCAL_TRANSMIT_GROUP_E) != 0)
		write_format(fd,
			".attributes.allow_local_transmit.group_E = true, ");

	if ((filter_flags & SANDBOX_FILTER_ALLOW_LOCAL_RECEIVE_GROUP_A) != 0)
		write_format(fd,
			".attributes.allow_local_receive.group_A = true, ");
	if ((filter_flags & SANDBOX_FILTER_ALLOW_LOCAL_RECEIVE_GROUP_B) != 0)
		write_format(fd,
			".attributes.allow_local_receive.group_B = true, ");
	if ((filter_flags & SANDBOX_FILTER_ALLOW_LOCAL_RECEIVE_GROUP_C) != 0)
		write_format(fd,
			".attributes.allow_local_receive.group_C = true, ");
	if ((filter_flags & SANDBOX_FILTER_ALLOW_LOCAL_RECEIVE_GROUP_D) != 0)
		write_format(fd,
			".attributes.allow_local_receive.group_D = true, ");
	if ((filter_flags & SANDBOX_FILTER_ALLOW_LOCAL_RECEIVE_GROUP_E) != 0)
		write_format(fd,
			".attributes.allow_local_receive.group_E = true, ");

	write_format(fd, ".profile_tag = 0x%X},\n", profile->profile_tag);
}

/* The main function for parsing and writing a single sandbox profile. */
static struct sandbox_profile *parse_sandbox_profile(const char *path, const char *tagname)
{
	int error;
	uint32_t tag;
	struct sandbox_profile *profile;
	struct sandbox_profile *similar_profile;
	char *filename;

	memcpy((char *)&tag, tagname, TAG_SIZE);

	profile = sbox_add_profile(tag);
	if (!profile)
		return NULL;

	filename = join_path(path, tagname);
	error = construct_sandbox_profile_from_file(filename, profile);
	profile->profile_tag = tag;
	free(filename);

	if (error < 0) {
		fprintf(stderr, "failed creating `%s` err=%d\n", tagname, error);
		return NULL;
	}

	/*
	 * To save (memory) space we can share the syscall filters between
	 * different sandbox profiles (if they are the same, of course).
	 */
	similar_profile = lookup_similar_profile(profile);
	if (similar_profile) {
		char similar_tagname[TAG_SIZE + 1];

		/* Prepare the tag-name of the similar profile. */
		memcpy(similar_tagname,
		       (char *) &similar_profile->profile_tag,
		       TAG_SIZE);
		similar_tagname[TAG_SIZE] = '\0';

		/* While writing the sandbox profile, use the new tag-name
		 * for the syscall filters. */
		write_sandbox_profile(fd_sandbox_profiles,
				      profile,
				      tagname, similar_tagname);
		return profile;
	}

	/*
	 * If we haven't found a similar profile, then write a completely new one.
	 */

	write_sandbox_profile(fd_sandbox_profiles,
			      profile,
			      tagname, tagname);

	write_syscall_filter(fd_syscall_filters,
			     profile->syscall_filter,
			     SBOX_SYS_FILTER_SIZE(__X_NR_syscalls),
			     tagname,
			     "");

#if defined(CONFIG_COMPAT) && defined(CONFIG_ARM64)
	write_syscall_filter(fd_syscall_filters_compat,
			     profile->syscall_filter_compat,
			     SBOX_SYS_FILTER_SIZE(__X_NR_compat_syscalls),
			     tagname,
			     "_compat");
#endif
	return profile;
}

/* Extract the tag name from the sandbox/syscalls profile file name. */
static size_t extract_profile_tag(const char *filename, char *tagname)
{
	const char *ext;

	ext = strstr(filename, "." SANDBOX_PROFILE_EXT);
	if (!ext)
		ext = strstr(filename, "." SYSCALLS_PROFILE_EXT);

	if ((filename + TAG_SIZE) != ext)
		return 0;

	memcpy(tagname, filename, TAG_SIZE);
	tagname[TAG_SIZE] = '\0';
	return TAG_SIZE;
}

/* List all tag names in the directory. */
static char *list_tagnames(DIR *dir, size_t *tags_count)
{
	struct dirent *de;
	size_t count;
	char tagname[TAG_SIZE + 1];
	char *tagnames; /* a NULL-separated string of all tag names. */
	char *curr_tagname;

	/*
	 * First pass is for counting the tag names...
	 */

	count = 0;
	while ((de = readdir(dir)) != NULL) {
		if (de->d_type != DT_REG)
			continue;

		if (!extract_profile_tag(de->d_name, tagname))
			continue;

		count++;
	}

	*tags_count = count;
	if (count == 0)
		return NULL;

	tagnames = safe_malloc(count * (TAG_SIZE + 1));
	curr_tagname = tagnames;

	/*
	 * Second pass is for extracting the tag names.
	 */

	rewinddir(dir);
	while ((de = readdir(dir)) != NULL) {
		if (de->d_type != DT_REG)
			continue;

		if (!extract_profile_tag(de->d_name, curr_tagname))
			continue;

		curr_tagname += TAG_SIZE + 1; /* Skip to the next tag name. */
	}

	/*
	 * The order if the listed file names is not deterministic.
	 * Sort it to make it so.
	 */
	qsort(tagnames, count, TAG_SIZE + 1, (int (*)(const void *, const void *))strcmp);
	return tagnames;
}

static void usage(char *argv[])
{
	const char *exec_name = strrchr(argv[0], '/');
	if (exec_name == NULL)
		exec_name = argv[0];

	fprintf(stderr, "Usage: %s <profiles_dir> <output_dir>\n", exec_name);
	exit(1);
}

int main(int argc, char *argv[])
{
	int retval = 1;
	DIR *profiles_dir;
	char *tagnames = NULL;
	char *curr_tagname = NULL;
	size_t tags_count = 0;
	struct sandbox_profile *profile = NULL;
	bool has_error = false;

	if (argc != 3)
		usage(argv);

	profiles_path = argv[1];
	output_path = argv[2];

	profiles_dir = opendir(profiles_path);
	if (!profiles_dir) {
		fprintf(stderr, "Cannot open profiles directory '%s': %s\n", profiles_path, strerror(errno));
		goto out;
	}

	fd_sandbox_profiles = create_file(output_path, "sandbox_profiles.inc");
	if (fd_sandbox_profiles == -1)
		goto out;

	fd_syscall_filters = create_file(output_path, "syscall_filters.c");
	if (fd_syscall_filters == -1)
		goto out;

#if defined(CONFIG_COMPAT) && defined(CONFIG_ARM64)
	fd_syscall_filters_compat = create_file(output_path, "syscall_filters_compat.c");
	if (fd_syscall_filters_compat == -1)
		goto out;
#endif

	parse_state = sandbox_filter_init_parse_state();

	/*
	 * OPTIMIZATION:
	 * The USER, _ML_, TOYS, and SAPP profiles are parsed first (in that
	 * order), as these profiles have the most extensive usage, and the,
	 * later, profile lookup function should encounter these first in the
	 * list.
	 *
	 * For example, if USER was somewhere in the middle, or even worse,
	 * the last one, then every time a user app is loaded, and the USER
	 * sandbox profile is requested (looked-up), then we would have to
	 * go through all the preceding profiles in the list, until we find it.
	 */
	profile = parse_sandbox_profile(profiles_path, "USER");
	if (!profile) { /* USER profile must exist! */
		fprintf(stderr, "Failed to parse 'USER' sandbox profile!\n");
		has_error = true;
	}

	profile = parse_sandbox_profile(profiles_path, "_ML_");
	if (!profile) { /* _ML_ profile must exist! */
		fprintf(stderr, "Failed to parse '_ML_' sandbox profile!\n");
		has_error = true;
	}

	retval = 0;

	profile = parse_sandbox_profile(profiles_path, "TOYS");
	if (!profile) {
		fprintf(stderr, "Failed to parse 'TOYS' sandbox profile!\n");
		has_error = true;
	}
	profile = parse_sandbox_profile(profiles_path, "SAPP");
	if (!profile) {
		fprintf(stderr, "Failed to parse 'SAPP' sandbox profile!\n");
		has_error = true;
	}

	tagnames = list_tagnames(profiles_dir, &tags_count);
	curr_tagname = tagnames;
	for (; tags_count; tags_count--, curr_tagname += TAG_SIZE + 1) { /* Skip to the next tag name. */
		if (!sbox_profile_exist(curr_tagname)) { /* the tagname list contains duplicates since it just goes over the files in the sandbox profiles folder */
#ifndef CONFIG_MLSEC_WITH_TEST_PROFILES /* in user-build, exclude test profiles */
			if (curr_tagname[0] == 't') {
				fprintf(stderr, "skipping test profile %s\n", curr_tagname);
				continue;
			}
#endif
			profile = parse_sandbox_profile(profiles_path, curr_tagname);
			if (!profile) {
				fprintf(stderr, "Failed to parse '%s' sandbox profile!\n", curr_tagname);
				has_error = true;
			}
		}
	}

out:
	if (tagnames)
		free(tagnames);
	if (fd_syscall_filters_compat != -1)
		close(fd_syscall_filters_compat);
	if (fd_syscall_filters != -1)
		close(fd_syscall_filters);
	if (fd_sandbox_profiles != -1)
		close(fd_sandbox_profiles);
	if (profiles_dir)
		closedir(profiles_dir);
	if (parse_state)
		sandbox_filter_free_parse_state(parse_state);
	if (has_error) {
		fprintf(stderr, "Deleting output due to errors\n");
		delete_file(output_path, "sandbox_profiles.inc");
		delete_file(output_path, "syscall_filters.c");
	}
	return retval;
}
