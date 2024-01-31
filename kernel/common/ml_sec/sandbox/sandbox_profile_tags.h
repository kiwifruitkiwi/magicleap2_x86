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

#ifndef __MLSEC_SBOX_SANDBOX_PROFILE_TAGS_H
#define __MLSEC_SBOX_SANDBOX_PROFILE_TAGS_H

#include <linux/ml_sec/tail_data.h>
#pragma GCC diagnostic ignored "-Wmultichar"

#define SBOX_PROFILE_TAG_USER TAG32('USER')
#define SBOX_PROFILE_TAG_MSCR TAG32('MSCR')
#define SBOX_PROFILE_TAG__ML_ TAG32('_ML_')
#define SBOX_PROFILE_TAG_JALR TAG32('JALR')
/* dummy default tag. Not really used as a tag in executables */
#define SBOX_PROFILE_TAG_UNKN TAG32('UNKN')

/*
 * return true if a profile is used by any kind of application
 */
static inline bool is_app_profile(uint32_t profile_tag)
{
	switch (profile_tag) {
	case SBOX_PROFILE_TAG_USER:
	case SBOX_PROFILE_TAG_MSCR:
		return true;
	default:
		return false;
	}
}


#endif /* __MLSEC_SBOX_SANDBOX_PROFILE_TAGS_H */
