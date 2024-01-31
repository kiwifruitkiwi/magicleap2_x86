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

#ifndef __MLSEC_SBOX_DEV_SANDBOX_PROFILE_FILE_H
#define __MLSEC_SBOX_DEV_SANDBOX_PROFILE_FILE_H


#include <linux/path.h>
#ifndef MLSEC_HOSTPROG
#include <linux/types.h> // for bool
#include <linux/ml_sec/sandbox_profile.h>
#endif
#include "sandbox_filters.h"

struct sbox_parse_state;

#ifdef MLSEC_HOSTPROG
struct sbox_parse_state *sandbox_filter_init_parse_state(void);
void sandbox_filter_free_parse_state(struct sbox_parse_state *s);
#endif
int sbox_create_sandbox_filter_from_file(struct path *exec_path, sandbox_flags_t *filter, struct sbox_parse_state *parse_state);
bool sbox_parse_sandbox_filter_to_string(int filter, char *buf, size_t size);

#endif	/* __MLSEC_SBOX_DEV_SANDBOX_PROFILE_FILE_H */
