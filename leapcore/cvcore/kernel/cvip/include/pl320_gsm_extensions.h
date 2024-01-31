/* SPDX-License-Identifier: GPL-2.0-only
 *
 * Copyright (c) 2020-2022, Magic Leap, Inc. All rights reserved.
 */
#ifndef __PL320_GSM_EXTENSIONS_H
#define __PL320_GSM_EXTENSIONS_H

#include <linux/types.h>
#include "pl320_types.h"

/**
 * Initialize the gsm memory used for pl320 bookkeeping
 *
 * \return 0 on success, or -1 on failure.\n
 */
Pl320HalStatus pl320_gsm_initialize(pl320_device_info * info);

Pl320HalStatus pl320_gsm_get_mailbox_hint(pl320_device_info * info, int *mb_idx);
void pl320_gsm_set_mailbox_hint(pl320_device_info * info, int mb_idx);
void pl320_gsm_clr_mailbox_hint(pl320_device_info * info, int mb_idx);
void pl320_gsm_record_mailbox_hit(pl320_device_info * info);
void pl320_gsm_record_mailbox_miss(pl320_device_info * info);

#define GSM_IPC_OFFSET_REPLY_MUTEX (0X0000U)
#define GSM_IPC_OFFSET_CLOSE_MUTEX (0X0008U)
#define GSM_IPC_OFFSET_SEQ_ID (0X0010U)
#define GSM_IPC_OFFSET_REPLY_STATE (0X0014U)

#ifndef __KERNEL__
#error "Can't include kernel header in userspace"
#endif

#endif
