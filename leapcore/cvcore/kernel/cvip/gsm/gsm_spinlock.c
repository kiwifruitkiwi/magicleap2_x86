// SPDX-License-Identifier: GPL-2.0
/*
 * (C) Copyright 2020
 * Magic Leap, Inc. (COMPANY)
 */

#include <linux/types.h>
#include <linux/delay.h>
#include <linux/errno.h>
#define GSM_SPINLOCK_SLEEP udelay

#include "gsm_spinlock.h"
#include "gsm_cvip.h"
#include "nibble.h"
#include "primitive_atomic_cvip.h"
#include "primitive_atomic.h"


#ifndef U32_MAX
#define U32_MAX		((int)~0U)
#endif

#define MAX_SPINWAIT_ITERS			(3000ul)
#define SPIN_WAIT_DELAY_US			(100)

#define SPINLOCK_NIBBLES_PER_BYTE	(2)

static int gsm_find_tas_nibble(struct gsm_spinlock_context *ctx)
{
	uint32_t index;
	uint32_t nibble_addr;
	uint32_t spinlock_ctrl_word;
	uint32_t value = 0;

	if (!ctx)
		return EINVAL;

	for (index = 0; index < SPINLOCK_CTRL_WORDS; index++) {
		spinlock_ctrl_word = GSM_RESERVED_CVIP_SPINLOCK_BITMAP_BASE + (index * SPINLOCK_CTRL_WORD_SIZE);
		value = primitive_atomic_find_and_set_32(spinlock_ctrl_word);
		if (value != PRIMITIVE_ATOMIC_FIND_AND_SET_ERROR) {
			/**
			* Found a nibble that is not in use.
			* Determine the nibble word address based on the control word index.
			*/
			nibble_addr = GSM_RESERVED_CVIP_SPINLOCK_NIBBLE_BASE + (index *
			((SPINLOCK_NIBBLE_WORD_SIZE * SPINLOCK_NIBBLES_PER_WORD) / SPINLOCK_NIBBLES_PER_BYTE));
			nibble_addr += ((value / SPINLOCK_NIBBLES_PER_WORD) * SPINLOCK_NIBBLE_WORD_SIZE);
			/**
			* Determine the nibble in the nibble word.
			* Return the nibble address location and nibble index.
			*/
			ctx->nibble_addr = nibble_addr;
			ctx->nibble_id   = (value % SPINLOCK_NIBBLES_PER_WORD);
			/**
			* Clear the TAS nibble to make sure it is in the unlocked state.
			*/
			gsm_spin_unlock(ctx);
			return 0;
		}
	}
	return ENODEV;
}

int gsm_spinlock_create(struct gsm_spinlock_context *ctx)
{
	if (!ctx)
		return EINVAL;
	/**
	* Obtain a GSM TAS nibble that is not in use
	*/
	if (gsm_find_tas_nibble(ctx))
		return ENOMEM;

	return 0;
}
EXPORT_SYMBOL(gsm_spinlock_create);

// Release a CVIP spinlock TAS nibble.
static int gsm_spinlock_clear_tas_nibble(struct gsm_spinlock_context *ctx)
{
	uint32_t nibble_index;
	uint32_t nibble_ctrl_word;
	uint32_t nibble_bit_shift;

	if (!ctx)
		return EINVAL;

	// Clear the spinlock nibble control bit
	nibble_index     = (ctx->nibble_addr - GSM_RESERVED_CVIP_SPINLOCK_NIBBLE_BASE) / SPINLOCK_NIBBLE_WORD_SIZE;
	nibble_ctrl_word = GSM_RESERVED_CVIP_SPINLOCK_BITMAP_BASE +
			((nibble_index / SPINLOCK_NIBBLE_WORD_SIZE) * SPINLOCK_CTRL_WORD_SIZE);
	nibble_bit_shift = (nibble_index % SPINLOCK_CTRL_WORD_SIZE);
	primitive_atomic_bitwise_clear(nibble_ctrl_word,
		(1 << ((nibble_bit_shift * SPINLOCK_NIBBLES_PER_WORD) + ctx->nibble_id)));

	return 0;
}

//De-initialize a cvip_spinlock instance
int gsm_spinlock_destroy(struct gsm_spinlock_context *ctx)
{
	int ret;

	if (!ctx)
		return EINVAL;

	// Release the spinlock nibble
	gsm_spin_unlock(ctx);

	// Reset the control bit in the control word to mark the nibble as not in use.
	ret = gsm_spinlock_clear_tas_nibble(ctx);
	if (!ret) {
		//De-initialize spinlock context by clearing its contents.
		ctx->nibble_addr = 0;
		ctx->nibble_id = 0;
	}

	return ret;
}
EXPORT_SYMBOL(gsm_spinlock_destroy);

int gsm_spin_lock(struct gsm_spinlock_context *ctx)
{
	uint32_t value;
	uint32_t counter;

	if (!ctx)
		return EINVAL;
	/**
	* Spin/wait to acquire the lock.
	* If the value returned by nibble_test_and_set_axi() is non-zero,
	* another process or thread owns the lock, and we will spin wait.
	* If the value returned is 0, the lock was not owned by another process or
	* thread, and we have succesfully acquired the lock.
	*/
	value = U32_MAX;
	do {
		for (counter = 0; counter < MAX_SPINWAIT_ITERS; counter++) {
			value = nibble_test_and_set_axi_sync(ctx->nibble_addr, ctx->nibble_id);
			if (value == 0)
				break;
		}
		/**
		* If we didn't acquire the lock after spin/waiting for a while, sleep a little
		* before resuming the spin/wait.
		*/
		if (value != 0) {
			// Short sleeps are done by busy looping. They are to be discouraged.
			// Use udelay() but for no more than 1000 microseconds.
                        GSM_SPINLOCK_SLEEP(SPIN_WAIT_DELAY_US);
		}
	} while (value != 0);

	return 0;
}
EXPORT_SYMBOL(gsm_spin_lock);

int gsm_spin_unlock(struct gsm_spinlock_context *ctx)
{
	if (!ctx)
		return EINVAL;

	nibble_write_sync(ctx->nibble_addr, ctx->nibble_id, 0);

	return 0;
}
EXPORT_SYMBOL(gsm_spin_unlock);
