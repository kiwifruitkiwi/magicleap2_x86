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
#include "sandbox_profile_file.h"
#include "profile_file.h"

#define SANDBOX_PROFILE_EXT "sandbox.profile"


#define CONN_GROUP_MAX_DESC_LEN (30)
#define CONN_GROUP_MAX_GROUPS (5) /* see sandbox_profile.h */
static const char CONN_PREFIX[] = "allow_local_";
static const size_t CONN_PREFIX_LEN = sizeof(CONN_PREFIX) - 1;

static const char GROUP_PREFIX[] = "group_";
static const size_t GROUP_PREFIX_LEN = sizeof(GROUP_PREFIX) - 1;

static const char RECEIVE_WORD[] = "receive_";
static const size_t RECEIVE_WORD_LEN = sizeof(RECEIVE_WORD) - 1;

static const char TRANSMIT_WORD[] = "transmit_";
static const size_t TRANSMIT_WORD_LEN = sizeof(TRANSMIT_WORD) - 1;

#define MAX_CONTYPE_LEN (10)
#define MAX_ENTRY_LEN (80)


/*** processing of allow_local_ROLE_group_X_YYY entries
 * There are 5 groups called A to E currently and a process can participate in
 * multiple groups.
 * ROLE - process role can get one of these two values:
 *        transmit: allowed to transmit to a receiver in the group.
 *        receive: allow the process to receive from a member in the group
 * a process can be both transmitter and receiver, for that two entries needs
 * to be added.
 * `X` -  identifies the group and needs to be in the range [A-E]
 * To aid in the self-documentation of the sandbox-profiles, when
 * specifying this line, it also needs to set a description `YYY` to the group.
 * The description needs to be the same on all instances of this this group.
 * This is checked in process_conn_group()
 * struct sbox_parse_state is used for keeping track on the names of the groups
 */
struct sbox_parse_state {
	char group_desc[CONN_GROUP_MAX_GROUPS][CONN_GROUP_MAX_DESC_LEN + 1];
};
#ifdef MLSEC_HOSTPROG
/* only in the host since only gen_sandbox_profiles.c uses this
 * when a profile is read in runtime by the kernel (in userdebug) the names are
 * not checked
 */
struct sbox_parse_state *sandbox_filter_init_parse_state(void)
{
	void *buf = malloc(sizeof(struct sbox_parse_state));
	memset(buf, 0, sizeof(struct sbox_parse_state));
	return (struct sbox_parse_state *)buf;
}

void sandbox_filter_free_parse_state(struct sbox_parse_state *s)
{
	free(s);
}
#endif

static bool process_conn_group_endpoint(const char **pname,
	char *tmp_modified_name,
	struct sbox_parse_state *state)
{
	const char *name = *pname, *ptr_name = *pname;
	size_t name_len;
	int letter, letter_index, conn_type_len;
	int net_size;

	name_len = strlen(name);
	if ((name_len < CONN_PREFIX_LEN) ||
		  strncmp(name, CONN_PREFIX, CONN_PREFIX_LEN) != 0)
		return true; /* continue parsing other options */

	name_len -= CONN_PREFIX_LEN;
	ptr_name += CONN_PREFIX_LEN;

	if ((name_len >= RECEIVE_WORD_LEN) && (strncmp(ptr_name, RECEIVE_WORD, RECEIVE_WORD_LEN) == 0))
		conn_type_len = RECEIVE_WORD_LEN;
	else if ((name_len >= TRANSMIT_WORD_LEN) && (strncmp(ptr_name, TRANSMIT_WORD, TRANSMIT_WORD_LEN) == 0))
		conn_type_len = TRANSMIT_WORD_LEN;
	else
		return true; /* continue parsing other options */

	name_len -= conn_type_len;
	ptr_name += conn_type_len;

	if ((name_len < GROUP_PREFIX_LEN + 1) ||
	  strncmp(ptr_name, GROUP_PREFIX, GROUP_PREFIX_LEN)) {
		pr_err("group transmition format: allow_local_<transmit|receive>_group_<G>. Received %s\n",
			name);
		return false;
	}

	name_len -= GROUP_PREFIX_LEN;
	ptr_name += GROUP_PREFIX_LEN;

	letter = *ptr_name;
	letter_index = letter - 'A';

	if (letter_index < 0 || letter_index >= CONN_GROUP_MAX_GROUPS) {
		pr_err("allow_local_conn_group_ outside allowed range [A-%c], got %c (%d)\n", 'A' + CONN_GROUP_MAX_GROUPS-1, letter, letter_index);
		return false;
	}

	if (state != NULL) {
		const char *full_desc = ptr_name; /* including the <letter>_ */

		if (state->group_desc[letter_index][0] != 0 && strcmp(state->group_desc[letter_index], full_desc) != 0) {
			pr_err("allow_local_conn_group_ different description for the same group got `%s` previous:`%s`\n", full_desc, state->group_desc[letter_index]);
			return false;
		}
		strncpy(state->group_desc[letter_index], full_desc, CONN_GROUP_MAX_DESC_LEN);
	}
	++ptr_name;
	net_size = ptr_name - name;
	/* new name - just the prefix, going to be checked against the flags in the caller */
	strncpy(tmp_modified_name, name, net_size);
	*pname = tmp_modified_name;

	return true;
}

/* pass args through sbox_parse_profile_file() */
struct parse_args {
	struct sbox_parse_state *state;
	sandbox_flags_t *filter;
};

static bool parse_filter_entry(const char *name, void *in_args)
{
	struct parse_args *args = (struct parse_args *)in_args;
	char tmp_modified_name[MAX_ENTRY_LEN] = {0};

	if (!process_conn_group_endpoint(&name, tmp_modified_name, args->state))
		return false;

#define _SANDBOX_FILTER(fname, str)	\
	if (strcmp(name, str) == 0) {				\
		*args->filter |= SANDBOX_FILTER_##fname;	\
		return true;					\
	}
#define _SANDBOX_FILTER_ALIAS(name, str) _SANDBOX_FILTER(name, str)
#include "sandbox_filters.def"
#undef _SANDBOX_FILTER
#undef _SANDBOX_FILTER_ALIAS

	pr_err("did not find sandbox filter entry %s\n", name);
	return false;
}

int sbox_create_sandbox_filter_from_file(struct path *exec_path, sandbox_flags_t *filter, struct sbox_parse_state *parse_state)
{
	struct parse_args args = { .state = parse_state, .filter = filter };
	int ret = sbox_parse_profile_file(exec_path, SANDBOX_PROFILE_EXT, parse_filter_entry, &args);

	if (ret < 0)
		return ret;
	/* otherwise it returns a positive and we want 0 for no error */
	return 0;
}

bool sbox_parse_sandbox_filter_to_string(int filter, char *buf, size_t size)
{
	bool retval = true;

	if (buf == NULL || size == 0)
		return false;

#define _SANDBOX_FILTER(name, str)	\
	if (filter & SANDBOX_FILTER_##name) {			\
		size_t str_len = sizeof(str) - 1;		\
		if ((str_len + 1) <= size) {			\
			memcpy(buf, str, str_len);		\
			buf += str_len;				\
			*buf++ = '|';				\
			size -= str_len + 1;			\
		}						\
		else						\
			retval = false;				\
	}
#define _SANDBOX_FILTER_ALIAS(name, str)
#include "sandbox_filters.def"
#undef _SANDBOX_FILTER
#undef _SANDBOX_FILTER_ALIAS

	if (filter)
		buf--;
	*buf = '\0';
	return retval;
}
