/* SPDX-License-Identifier: GPL-2.0-only
 *
 * Copyright (c) 2020-2022, Magic Leap, Inc. All rights reserved.
 */

#ifndef __GSM_SPINLOCK_H__
#define __GSM_SPINLOCK_H__

struct gsm_spinlock_context {
	uint32_t nibble_addr;
	uint32_t nibble_id;
};


int gsm_spinlock_create(struct gsm_spinlock_context *ctx);
int gsm_spin_lock(struct gsm_spinlock_context *ctx);
int gsm_spin_unlock(struct gsm_spinlock_context *ctx);
int gsm_spinlock_destroy(struct gsm_spinlock_context *ctx);

#endif /* GSM_SPINLOCK_H */

#ifndef __KERNEL__
#error "Can't include kernel header in userspace"
#endif
