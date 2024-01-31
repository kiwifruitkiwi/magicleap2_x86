/*
 * ---------------------------------------------------------------------
 * %COPYRIGHT_BEGIN%
 *
 * \copyright
 * Copyright (c) 2020-2021 Magic Leap, Inc. (COMPANY) All Rights Reserved.
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

#include <linux/types.h>

#include "gsm.h"
#include "gsm_cvip.h"
#include "primitive_atomic.h"
#include "pl320_gsm_extensions.h"
#include "pl320_types.h"

#define FAST_CORE_MASK_START (0)
#define SAFE_CORE_MASK_START (16)

#if !defined(__i386__) && !defined(__amd64__)
#define CURRENT_CORE_ID (8)
#else
#define CURRENT_CORE_ID (9)
#endif

#define PL320_USE_GSM_FAST_EXTENSIONS (1)
#ifndef PL320_USE_GSM_FAST_EXTENSIONS
#define PL320_USE_GSM_FAST_EXTENSIONS (0)
#endif

#ifndef PL320_USE_GSM_SAFE_EXTENSIONS
#define PL320_USE_GSM_SAFE_EXTENSIONS (0)
#endif

static uintptr_t ipc_ext_global_baseaddr[5] = {
	0x0, 0x0, GSM_RESERVED_IPC_IPC2_GLOBAL_EXT_INFO_START,
	GSM_RESERVED_IPC_IPC3_GLOBAL_EXT_INFO_START, 0x0
};

static uintptr_t ipc_ext_mailbox_baseaddr[5] = {
	0x0, 0x0, GSM_RESERVED_IPC_IPC2_MAILBOX_EXT_INFO_START,
	GSM_RESERVED_IPC_IPC3_MAILBOX_EXT_INFO_START, 0x0
};

struct pl320_gsm_ipc_global_info_s {
	uint32_t mailbox_status;
	uint32_t use_fast_extensions;
	uint32_t use_safe_extensions;
	uint32_t mailbox_hint_hit;
	uint32_t mailbox_hint_miss;
	uint32_t mailbox_hint_miss_dsp;
	uint32_t core_mask;
};

Pl320HalStatus pl320_gsm_initialize(pl320_device_info *info)
{
	struct pl320_gsm_ipc_global_info_s *ext_p;

	if (info == NULL || (info->ipc_id >= 5)) {
		return PL320_STATUS_FAILURE_INVALID_PARAMETER;
	}

	info->use_fast_ext = false;
	info->use_safe_ext = false;

	if (!gsm_core_is_cvip_up()) {
		// GSM is not ready so extensions
		// won't be availble yet.
		return PL320_STATUS_FAILURE_DEVICE_UNAVAILABLE;
	}

	info->ipc_ext_global_base = ipc_ext_global_baseaddr[info->ipc_id];
	info->ipc_ext_mailbox_base = ipc_ext_mailbox_baseaddr[info->ipc_id];

	if (info->ipc_ext_global_base == 0 || info->ipc_ext_mailbox_base == 0) {
		// Non supported IPC device
		return PL320_STATUS_FAILURE_DEVICE_INVALID;
	}

	ext_p = (struct pl320_gsm_ipc_global_info_s *)info->ipc_ext_global_base;

#if !defined(__i386__) && !defined(__amd64__)
	{
		int i = 0;

		for (i = 0;
		     i < GSM_IPC_NUM_MAILBOXES * GSM_IPC_MAILBOX_EXT_INFO_SIZE;
		     i = i + 4) {
			gsm_raw_write_32(info->ipc_ext_mailbox_base + i,
					 0x00000000);
		}

		gsm_raw_write_32(&(ext_p->mailbox_status),
				 ~(info->mailbox_enabled));
		// ARM dictates whether all cores will use gsm extensions
		gsm_raw_write_32(&(ext_p->use_fast_extensions),
				 PL320_USE_GSM_FAST_EXTENSIONS);
		gsm_raw_write_32(&(ext_p->use_safe_extensions),
				 PL320_USE_GSM_SAFE_EXTENSIONS);
		gsm_raw_write_32(&(ext_p->mailbox_hint_hit), 0);
		gsm_raw_write_32(&(ext_p->mailbox_hint_miss), 0);
		gsm_raw_write_32(&(ext_p->mailbox_hint_miss_dsp), 0);
		gsm_raw_write_32(&(ext_p->core_mask), 0);
	}
#endif

	// Determine if the master (ARM) has designated the use of GSM fast extensions
	info->use_fast_ext =
		gsm_raw_read_32(&(ext_p->use_fast_extensions)) == 0 ? false :
								      true;

	if (info->use_fast_ext) {
		// Signify that this core has initialized pl320 gsm fast extensions
		primitive_atomic_bitwise_set(
			&(ext_p->core_mask),
			0x1 << (FAST_CORE_MASK_START + CURRENT_CORE_ID));
	}

	// Determine if the master (ARM) has designated the use of GSM safe extensions
	info->use_safe_ext =
		gsm_raw_read_32(&(ext_p->use_safe_extensions)) == 0 ? false :
								      true;

	if (info->use_safe_ext) {
		// Signify that this core has initialized pl320 gsm safe extensions
		primitive_atomic_bitwise_set(
			&(ext_p->core_mask),
			0x1 << (SAFE_CORE_MASK_START + CURRENT_CORE_ID));
	}

	return PL320_STATUS_SUCCESS;
}

Pl320HalStatus pl320_gsm_get_mailbox_hint(pl320_device_info *info, int *mb_idx)
{
	struct pl320_gsm_ipc_global_info_s *ext_p =
		(struct pl320_gsm_ipc_global_info_s *)info->ipc_ext_global_base;

	*mb_idx = primitive_atomic_find_and_set_32(&(ext_p->mailbox_status));
	if (*mb_idx == PRIMITIVE_ATOMIC_FIND_AND_SET_ERROR) {
		return PL320_STATUS_FAILURE_MAILBOX_UNAVAILABLE;
	}

	return PL320_STATUS_SUCCESS;
}

void pl320_gsm_set_mailbox_hint(pl320_device_info *info, int mb_idx)
{
	struct pl320_gsm_ipc_global_info_s *ext_p =
		(struct pl320_gsm_ipc_global_info_s *)info->ipc_ext_global_base;

	primitive_atomic_bitwise_set(&(ext_p->mailbox_status), 0x1 << mb_idx);
}

void pl320_gsm_clr_mailbox_hint(pl320_device_info *info, int mb_idx)
{
	struct pl320_gsm_ipc_global_info_s *ext_p =
		(struct pl320_gsm_ipc_global_info_s *)info->ipc_ext_global_base;

	primitive_atomic_bitwise_clear(&(ext_p->mailbox_status), 0x1 << mb_idx);
}

void pl320_gsm_record_mailbox_hit(pl320_device_info *info)
{
	struct pl320_gsm_ipc_global_info_s *ext_p =
		(struct pl320_gsm_ipc_global_info_s *)info->ipc_ext_global_base;

	primitive_atomic_increment(&(ext_p->mailbox_hint_hit));
}

void pl320_gsm_record_mailbox_miss(pl320_device_info *info)
{
	struct pl320_gsm_ipc_global_info_s *ext_p =
		(struct pl320_gsm_ipc_global_info_s *)info->ipc_ext_global_base;

	primitive_atomic_increment(&(ext_p->mailbox_hint_miss));
}
