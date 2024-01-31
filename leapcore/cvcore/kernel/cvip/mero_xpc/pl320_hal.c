/*
 * ---------------------------------------------------------------------
 * %COPYRIGHT_BEGIN%
 *
 * \copyright
 * Copyright (c) 2020-2023 Magic Leap, Inc. (COMPANY) All Rights Reserved.
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

#include "pl320_hal.h"

#include "pl320_gsm_extensions.h"
#include "xdebug_internal.h"

#if defined(__amd64__) || defined(__i386__)
#define CURRENT_CORE_IS_X86 (1)
#else
#define CURRENT_CORE_IS_X86 (0)
#endif

#if defined(__arm__) || defined(__aarch64__)
#define CURRENT_CORE_IS_ARM (1)
#else
#define CURRENT_CORE_IS_ARM (0)
#endif

#define __NO_VERSION__
#include <linux/spinlock.h>
#include <linux/delay.h>
#include <linux/smp.h>
static DEFINE_SPINLOCK(pl320_mailbox_acquire_lock);
static unsigned long saved_irq_flags_pl320;
#define PL320_HAL_CRITICAL_SECTION_START                                       \
	do {                                                                   \
		spin_lock_irqsave(&pl320_mailbox_acquire_lock,                 \
				  saved_irq_flags_pl320);                      \
	} while (0);
#define PL320_HAL_CRITICAL_SECTION_END                                         \
	do {                                                                   \
		spin_unlock_irqrestore(&pl320_mailbox_acquire_lock,            \
				       saved_irq_flags_pl320);                 \
	} while (0);
#include <asm/io.h>

#define _readq(c) (__raw_readq(c))
#define _readl(c) (__raw_readl(c))
#define _readw(c) (__raw_readw(c))
#define _readb(c) (__raw_readb(c))

#define _writeq(c, v) ({ __raw_writeq(v, c); })
#define _writel(c, v) ({ __raw_writel(v, c); })
#define _writew(c, v) ({ __raw_writew(v, c); })
#define _writeb(c, v) ({ __raw_writeb(v, c); })

#define XPC_SOURCE(i) (i << 6)
#define XPC_DSET(i) ((uint32_t)((i << 6) + 0x4))
#define XPC_DCLEAR(i) ((i << 6) + 0x008)
#define XPC_DSTATUS(i) ((i << 6) + 0x00C)
#define XPC_MODE(i) ((i << 6) + 0x010)
#define XPC_MSET(i) ((i << 6) + 0x014)
#define XPC_MCLEAR(i) ((i << 6) + 0x018)
#define XPC_MSTATUS(i) ((i << 6) + 0x01C)
#define XPC_SEND(i) ((i << 6) + 0x020)
#define XPC_DRX(i, j) ((i << 6) + 0x024 + (j << 2))
#define XPC_MMIS(i) ((i << 3) + 0x800)
#define XPC_MMRIS(i) ((i << 3) + 0x804)

#define XPC_AUTO_ACK (0x1)
#define XPC_AUTO_LINK (0x2)
#define XPC_SEND_DEST (0x1)
#define XPC_SEND_SOURCE (0x2)

#define XPC_MAILBOX_SIZE (28)

#define XPC_HAL_NUM_CHANNELS (32)
#define XPC_HAL_NUM_HARDWARE (5)
#define XPC_HAL_NUM_MAILBOXES (32)

#define CHECK_NULLPTR
#ifdef CHECK_NULLPTR
#define CHECK_INFO_NULL(i)                                                     \
	{                                                                      \
		if (NULL == i)                                                 \
			return PL320_STATUS_FAILURE_NULL_POINTER;              \
	}
#else
#define CHECK_INFO_NULL(i)
#endif

#define IS_POWER_OF_2(x) ((x != 0) && ((x & (x - 1)) == 0))

//#define PL320_COLLECT_EXTENSION_STATS
#ifdef PL320_COLLECT_EXTENSION_STATS
#define PL320_EXT_STATS_RECORD_FAST_ACQUIRE_HIT(i)                             \
	pl320_gsm_record_mailbox_hit((i))

#define PL320_EXT_STATS_RECORD_FAST_ACQUIRE_MISS(i)                            \
	pl320_gsm_record_mailbox_miss((i))
#else
#define PL320_EXT_STATS_RECORD_FAST_ACQUIRE_HIT(i)
#define PL320_EXT_STATS_RECORD_FAST_ACQUIRE_MISS(i)
#endif

/**
 * Structure for identifying ARM standard Primecell components
 */
typedef union {
	struct {
		uint32_t IPCMPCellID0 : 8;
		uint32_t : 24;
		uint32_t IPCMPCellID1 : 8;
		uint32_t : 24;
		uint32_t IPCMPCellID2 : 8;
		uint32_t : 24;
		uint32_t IPCMPCellID3 : 8;
		uint32_t : 24;
	} fields;
	uint32_t words[4];
	uint8_t bytes[16];
} primecell_id;

/**
 * Structure for ARM PL320 components
 */
typedef union {
	struct {
		uint32_t partnumber0 : 8;
		uint32_t : 24;
		uint32_t partnumber1 : 4;
		uint32_t designer0 : 4;
		uint32_t : 24;
		uint32_t designer1 : 4;
		uint32_t revision : 4;
		uint32_t : 24;
		uint32_t configuration : 8;
		uint32_t : 24;
	} fields;
	uint32_t words[4];
	uint8_t bytes[16];
} pl320_peripheral_id;

// Device Info structures for maximum number of hardware components
pl320_device_info dev_info[XPC_HAL_NUM_HARDWARE];

/////////////////////////////////////////////////
// Internal Helper functions
/////////////////////////////////////////////////

Pl320HalStatus acquire_xpc_channel(Pl320MailboxInfo *info);
Pl320HalStatus acquire_xpc_hardware(Pl320MailboxInfo *info,
				    uint32_t destination_mask);
Pl320HalStatus acquire_xpc_mailbox(Pl320MailboxInfo *info,
				   uint32_t destination_mask);
Pl320HalStatus acknowledge_mailbox(Pl320MailboxInfo *info);
Pl320HalStatus read_mailbox_irq_mask(Pl320MailboxInfo *info);
Pl320HalStatus read_mailbox_irq_mode(Pl320MailboxInfo *info);
Pl320HalStatus load_mailbox_data(Pl320MailboxInfo *info, void *data,
				 uint16_t length);
Pl320HalStatus check_xpc_mailbox(Pl320MailboxInfo *info);
Pl320HalStatus check_xpc_hardware_valid(uint64_t paddress);
void reset_xpc_hardware(pl320_device_info *dev);

static inline bool supports_fast_mailbox_acquisition(Pl320MailboxInfo *info)
{
	return dev_info[info->ipc_id].use_fast_ext;
}

static inline bool try_init_gsm_extensions(pl320_device_info *info)
{
	if (pl320_gsm_initialize(info) == PL320_STATUS_SUCCESS) {
		pr_info("GSM extensions successfully initialized for IPC 0x%02X\n",
			info->ipc_id);
		return true;
	}
	return false;
}

void reset_xpc_hardware(pl320_device_info *dev)
{
	int mbox_idx;

	for (mbox_idx = 0; mbox_idx < 32; mbox_idx++) {
		// Only reset mailboxes that we use
		if (!((dev->mailbox_enabled >> mbox_idx) & 0x1)) {
			continue;
		}

		/**
		 * No other channel handler besides the platform channel
		 * handler(vchannel 0) will be registered when this is called. So we are
		 * going to reset everything except valid platform channel
		 * command_response messages from x86->ARM.
		 *
		 * Thing we have to check.
		 * 1. Source register is one-hot encoded and has an x86 channel
		 * id(12-15)
		 * 2. Destination register is one-hot encoded and has a ARM channel
		 * id(20-31)
		 * 3. Send register is set to generate an interrupt at the
		 * destination(bit 0 = 1, bit 1 = 0)
		 * 4. The correct interrupt is being generated for the mailbox
		 * 5. The XPC message header has correct information for vchannel 0.
		 *
		 * NOTE: This will only run on ARM.
		 */
		if (CURRENT_CORE_IS_ARM) {
			uint32_t source_reg;
			uint32_t dest_reg;
			uint32_t send_reg;

			source_reg = _readl(
				(void *)(dev->paddress + XPC_SOURCE(mbox_idx)));
			dest_reg = _readl((void *)(dev->paddress +
						   XPC_DSTATUS(mbox_idx)));
			send_reg = _readl(
				(void *)(dev->paddress + XPC_SEND(mbox_idx)));

			// Check if source and destination registers are one-hot encoded
			if (IS_POWER_OF_2(source_reg) &&
			    IS_POWER_OF_2(dest_reg)) {
				uint32_t source_id;
				uint32_t dest_id;
				// ffs finds the first set bit, and the first (least
				// significant) bit is at position 1, so we subtract 1.
				source_id = ffs(source_reg) - 1;
				dest_id = ffs(dest_reg) - 1;
				// Source channel id has to come from x86, channels 12-15.
				// Dest channel id has to be set to ARM, channels 20-31.
				// Send register has set to be XPC_SEND_DEST(0x1).
				if ((source_id >= 12 && source_id <= 15) &&
				    (dest_id >= 20 && dest_id <= 31) &&
				    (send_reg == XPC_SEND_DEST)) {
					uint32_t int_mask;

					// The destination id will tell us which interrupt number to
					// check. The masked interrupt status register is 32-bit and
					// identifies which mailbox triggered the interrupt by the
					// bit set.
					int_mask = _readl(
						(void *)(dev->paddress +
							 XPC_MMIS(dest_id)));

					// Check if the current mailbox has an interrupt enabled
					if (((int_mask >> mbox_idx) & 0x1) ==
					    1) {
						// Final step is to check that the xpc header has the
						// correct information.
						uint8_t header;
						uint8_t header_vchannel;
						uint8_t header_mode;
						uint32_t db_word;
						/**
						 * The xpc header is the last byte in the last 32-bits
						 * of the data register, data reg 6. It has the
						 * following format:
						 *
						 * struct {
						 *		XpcChannel vchannel : 4;
						 *		XpcMode mode : 2;
						 *		uint8_t rsvd : 1;
						 *		uint8_t error : 1;
						 * } fields;
						 */
						db_word = _readl((
							void *)(dev->paddress +
								XPC_DRX(mbox_idx,
									6)));
						header = (db_word >> 24) & 0xFF;
						header_vchannel = header & 0xF;
						header_mode =
							(header >> 4) & 0x3;
						/**
						 * Check that the header has expected values.
						 * The vchannel should be 0 (CVCORE_XCHANNEL_PLATFORM).
						 * The mode should be 0 (XPC_MODE_COMMAND_RESPONSE).
						 */
						if (header_vchannel == 0 &&
						    header_mode == 0) {
							// Everything checks out, so skip resetting
							continue;
						}
					}
				}
			}
		}

		// Reset mailbox
		_writel((void *)(dev->paddress + XPC_SOURCE(mbox_idx)), 0);
	}
}

int is_multicast(uint32_t mask)
{
	return mask & (mask - 1) ? 1 : 0;
}

Pl320HalStatus check_xpc_hardware_valid(uint64_t paddress)
{
	pl320_peripheral_id perip_id;
	primecell_id pcell_id;
	// Read the Peripheral Identification and PrimeCell Identification registers
	perip_id.words[0] = _readl((void *)(paddress + 0xFE0));
	perip_id.words[1] = _readl((void *)(paddress + 0xFE4));
	perip_id.words[2] = _readl((void *)(paddress + 0xFE8));
	perip_id.words[3] = _readl((void *)(paddress + 0xFEC));

	if (perip_id.fields.partnumber0 != 0x20 ||
	    perip_id.fields.partnumber1 != 0x03 ||
	    perip_id.fields.designer0 != 0x01 ||
	    perip_id.fields.designer1 != 0x04 ||
	    perip_id.fields.configuration != 0x00) {
		return PL320_STATUS_FAILURE_DEVICE_INVALID;
	}

	pcell_id.words[0] = _readl((void *)(paddress + 0xFF0));
	pcell_id.words[1] = _readl((void *)(paddress + 0xFF4));
	pcell_id.words[2] = _readl((void *)(paddress + 0xFF8));
	pcell_id.words[3] = _readl((void *)(paddress + 0xFFC));

	if (pcell_id.fields.IPCMPCellID0 != 0x0D ||
	    pcell_id.fields.IPCMPCellID1 != 0xF0 ||
	    pcell_id.fields.IPCMPCellID2 != 0x05 ||
	    pcell_id.fields.IPCMPCellID3 != 0xB1) {
		return PL320_STATUS_FAILURE_DEVICE_INVALID;
	}

	return PL320_STATUS_SUCCESS;
}

Pl320HalStatus acquire_xpc_channel(Pl320MailboxInfo *info)
{
	// equally use the source channels we have been assigned
	// info->pl320_channel_id = 0;
	if (info->ipc_id == 0)
		;

	return PL320_STATUS_SUCCESS;
}

Pl320HalStatus acknowledge_mailbox(Pl320MailboxInfo *info)
{
	uint64_t dev_paddr = dev_info[info->ipc_id].paddress;

	xdebug_event_record(info->ipc_id, info->mailbox_idx, XPC_EVT_REPLY, 0);

	if (info->mailbox_mode == PL320_MAILBOX_MODE_DEFAULT) {
		xdebug_event_record(info->ipc_id, info->mailbox_idx,
				    XPC_EVT_CLR_DEST_START, 0);

		_writel((void *)(dev_paddr + XPC_DCLEAR(info->mailbox_idx)),
			(1 << info->ipc_irq_id));

		xdebug_event_record(info->ipc_id, info->mailbox_idx,
				    XPC_EVT_CLR_DEST_END, 0);

		smp_store_release(
			(uint32_t *)(dev_paddr + XPC_SEND(info->mailbox_idx)),
			XPC_SEND_SOURCE);
	} else {
		xdebug_event_record(info->ipc_id, info->mailbox_idx,
				    XPC_EVT_CLR_DEST_START, 0);

		smp_store_release(
			(uint32_t *)(dev_paddr + XPC_DCLEAR(info->mailbox_idx)),
			(1 << info->ipc_irq_id));

		xdebug_event_record(info->ipc_id, info->mailbox_idx,
				    XPC_EVT_CLR_DEST_END, 0);
	}

	return PL320_STATUS_SUCCESS;
}

Pl320HalStatus read_mailbox_irq_mask(Pl320MailboxInfo *info)
{
	info->mailbox_irq_mask =
		smp_load_acquire((uint32_t *)(dev_info[info->ipc_id].paddress +
					      XPC_MMIS(info->ipc_irq_id)));

	return PL320_STATUS_SUCCESS;
}

Pl320HalStatus read_mailbox_irq_mode(Pl320MailboxInfo *info)
{
	info->mailbox_mode = (Pl320MailboxAckMode)_readl(
		(void *)(dev_info[info->ipc_id].paddress +
			 XPC_MODE(info->mailbox_idx)));
	return PL320_STATUS_SUCCESS;
}

Pl320HalStatus load_mailbox_data(Pl320MailboxInfo *info, void *data,
				 uint16_t length)
{
	uint64_t dev_paddr = 0x0;
	uint32_t db_word[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
	uint32_t i;

	if (0x0 == length) {
		return PL320_STATUS_SUCCESS;
	}

	dev_paddr = dev_info[info->ipc_id].paddress;

	for (i = 0; i < length; i++) {
		((uint8_t *)db_word)[i] = ((uint8_t *)data)[i];
	}

	for (i = 0; i < 7; i++) {
		_writel((void *)(dev_paddr + XPC_DRX(info->mailbox_idx, i)),
			db_word[i]);
	}

	return PL320_STATUS_SUCCESS;
}

Pl320HalStatus check_xpc_mailbox(Pl320MailboxInfo *info)
{
	if (info->ipc_id >= XPC_HAL_NUM_HARDWARE) {
		return PL320_STATUS_FAILURE_DEVICE_INVALID;
	}

	if (0x0 == dev_info[info->ipc_id].is_enabled) {
		return PL320_STATUS_FAILURE_DEVICE_UNAVAILABLE;
	}

	if (0x0 == (dev_info[info->ipc_id].mailbox_enabled &
		    (0x1 << info->mailbox_idx))) {
		return PL320_STATUS_FAILURE_MAILBOX_OFFLINE;
	}

	return PL320_STATUS_SUCCESS;
}

Pl320HalStatus acquire_xpc_hardware(Pl320MailboxInfo *info,
				    uint32_t destination_mask)
{
	if (0x0 == destination_mask) {
		return PL320_STATUS_FAILURE_INVALID_PARAMETER;
	}

	if (is_multicast(destination_mask)) {
		info->mailbox_mode = PL320_MAILBOX_MODE_AUTO_ACK;
	}

	info->ipc_paddress = dev_info[info->ipc_id].paddress;

	return PL320_STATUS_SUCCESS;
}

Pl320HalStatus acquire_xpc_mailbox(Pl320MailboxInfo *info,
				   uint32_t destination_mask)
{
	uint32_t i, j;
	uint32_t source_mask;
	pl320_device_info *device;
	Pl320HalStatus status;
	int mb_hint = 0;
	bool mb_hint_is_valid = false;

	if (info->mailbox_idx != 0xFF) {
		return PL320_STATUS_SUCCESS;
	}

	if ((status = acquire_xpc_hardware(info, destination_mask)) !=
	    PL320_STATUS_SUCCESS) {
		return status;
	}

	source_mask = (0x1U << info->ipc_irq_id);

	device = &dev_info[info->ipc_id];

	if (supports_fast_mailbox_acquisition(info)) {
		mb_hint_is_valid = true;
		if (pl320_gsm_get_mailbox_hint(device, &mb_hint) !=
		    PL320_STATUS_SUCCESS) {
			mb_hint = 0;
			mb_hint_is_valid = false;
		}
	}

	status = PL320_STATUS_FAILURE_MAILBOX_UNAVAILABLE;
	PL320_HAL_CRITICAL_SECTION_START

	for (j = 0; j < PL320_NUM_MAILBOXES; ++j) {
		i = (mb_hint + j) % PL320_NUM_MAILBOXES;
		if (!((device->mailbox_enabled >> i) & 0x1)) {
			continue;
		}
		info->mailbox_idx = i;
		// Check that this channel_id is not already locking this source
		if (((device->channel_id_mask[info->ipc_irq_id] >> i) & 0x1) ==
		    0x0) {
			// Try to claim mailbox (i) by source_id
			_writel((void *)(device->paddress +
					 (uintptr_t)XPC_SOURCE(i)),
				source_mask);
			// check that we got the mailbox
			if (_readl((void *)(device->paddress +
					    (uintptr_t)XPC_SOURCE(i))) ==
			    source_mask) {
				xdebug_event_record(info->ipc_id,
						    info->mailbox_idx,
						    XPC_EVT_MAILBOX_ACQ,
						    source_mask);

				device->channel_id_mask[info->ipc_irq_id] |=
					0x1 << i;
				status = PL320_STATUS_SUCCESS;
				break;
			}
		}
	}

	if (supports_fast_mailbox_acquisition(info)) {
		if (status != PL320_STATUS_SUCCESS) {
			if (mb_hint_is_valid) {
				/*
				 * No mailbox was acquired but our hint suggested
				 * that a mailbox (the hinted mailbox) was available.
				 * Leave the mailbox hint bit as occupied since it
				 * matches our most recent knowledge of the mailbox.
				 * Update statistics to account for the hint miss.
				 */
				PL320_EXT_STATS_RECORD_FAST_ACQUIRE_MISS(
					device);
			}
		} else {
			/*
			 * A mailbox was acquired so record it as occupied. This
			 * covers two cases: (1) the hinted mailbox matches the
			 * acquired mailbox, in which case setting the occupied
			 * bit has no net effect; (2) the hinted mailbox did not
			 * match the acquired mailbox, in which case the occupied
			 * bit of the acquired mailbox will be set.
			 */
			pl320_gsm_set_mailbox_hint(device, info->mailbox_idx);
			if (mb_hint_is_valid) {
				// A hint was provided. Was the hint useful or not?
				if (info->mailbox_idx != mb_hint) {
					/*
					* Although we got a mailbox, our hint was wrong.
					* This implies that the hinted mailbox was already
					* in use. Leave the mailbox hint bit as occupied
					* since it matches our most recent knowledge of
					* the mailbox. Update statistics to account for
					* the hint miss.
					*/
					PL320_EXT_STATS_RECORD_FAST_ACQUIRE_MISS(
						device);
				} else {
					// Update our stats to reflect the hint hit.
					PL320_EXT_STATS_RECORD_FAST_ACQUIRE_HIT(
						device);
				}
			} else {
				/*
				 * The hint suggested that no mailboxes were available.
				 * The hint was at least incorrect for the mailbox that
				 * was successfully acquired.  Since we already updated
				 * the occupied state of the acquired mailbox, there is
				 * nothing else to do except update our statistics to
				 * reflect the hint miss.
				 */
				PL320_EXT_STATS_RECORD_FAST_ACQUIRE_MISS(
					device);
			}
		}
	}

	PL320_HAL_CRITICAL_SECTION_END
	return status;
}

/////////////////////////////////////////////////
// API functions
/////////////////////////////////////////////////

Pl320HalStatus pl320_initialize_hardware(uint8_t ipc_id, uint64_t paddress,
					 uint32_t source_channels,
					 uint32_t mailbox_enable)
{
	Pl320HalStatus status;

	// x86 will not be able to access the PL320s until CVIP boots.
	if (CURRENT_CORE_IS_ARM) {
		status = check_xpc_hardware_valid(paddress);

		if (status != PL320_STATUS_SUCCESS) {
			return status;
		}
	}

	dev_info[ipc_id].ipc_id = ipc_id;
	dev_info[ipc_id].paddress = paddress;
	dev_info[ipc_id].mailbox_enabled = mailbox_enable;
	dev_info[ipc_id].source_channels = source_channels;
	dev_info[ipc_id].is_enabled = 1;
	dev_info[ipc_id].use_fast_ext = false;
	dev_info[ipc_id].use_safe_ext = false;

	pr_info("Mailbox %d has enable mask 0x%08X", ipc_id,
		dev_info[ipc_id].mailbox_enabled);
	if (CURRENT_CORE_IS_ARM) {
		// TODO(kirick): Handle this better without hard associations of IPC ID.
		// Perhaps pass an argument from an upper layer obtained from the DTS.
		if (ipc_id == 0x2) {
			dev_info[ipc_id].use_fast_ext = true;
			dev_info[ipc_id].use_safe_ext = false;
		}
		reset_xpc_hardware(&dev_info[ipc_id]);
	}

	dev_info[ipc_id].gsm_ext_inited =
		try_init_gsm_extensions(&dev_info[ipc_id]);

	return PL320_STATUS_SUCCESS;
}

Pl320HalStatus pl320_deinitialize_hardware(uint8_t ipc_id)
{
	if (dev_info[ipc_id].is_enabled == 1) {
		iounmap((void *)(dev_info[ipc_id].paddress));
		dev_info[ipc_id].ipc_id = 0xFF;
		dev_info[ipc_id].paddress = 0x0;
		dev_info[ipc_id].mailbox_enabled = 0x0;
		dev_info[ipc_id].source_channels = 0x0;
		dev_info[ipc_id].is_enabled = 0;
	}
	return PL320_STATUS_SUCCESS;
}
Pl320HalStatus pl320_acquire_mailbox(Pl320MailboxInfo *info,
				     uint32_t destination_mask)
{
	Pl320HalStatus status;

	CHECK_INFO_NULL(info);

	if (acquire_xpc_channel(info) != PL320_STATUS_SUCCESS) {
		return PL320_STATUS_FAILURE_CHANNEL_UNAVAILABLE;
	}

	if (acquire_xpc_mailbox(info, destination_mask) !=
	    PL320_STATUS_SUCCESS) {
		return PL320_STATUS_FAILURE_MAILBOX_UNAVAILABLE;
	}
	if ((status = check_xpc_mailbox(info)) != PL320_STATUS_SUCCESS) {
		return status;
	}

	return PL320_STATUS_SUCCESS;
}

Pl320HalStatus pl320_send(Pl320MailboxInfo *info, uint32_t destination_mask,
			  void *data, uint16_t length)
{
	uint64_t dev_paddr;
	Pl320HalStatus status;

	CHECK_INFO_NULL(info);

	if ((status = check_xpc_mailbox(info)) != PL320_STATUS_SUCCESS) {
		return status;
	}

	if (length > XPC_MAILBOX_SIZE) {
		return PL320_STATUS_FAILURE_DATA_LENGTH;
	}

	if ((length != 0) && (data == NULL)) {
		return PL320_STATUS_FAILURE_DATA_NULL;
	}

	if (acquire_xpc_channel(info) != PL320_STATUS_SUCCESS) {
		return PL320_STATUS_FAILURE_CHANNEL_UNAVAILABLE;
	}

	if (acquire_xpc_mailbox(info, destination_mask) !=
	    PL320_STATUS_SUCCESS) {
		return PL320_STATUS_FAILURE_MAILBOX_UNAVAILABLE;
	}

	if (load_mailbox_data(info, data, length) != PL320_STATUS_SUCCESS) {
		return PL320_STATUS_FAILURE_DATA_LOAD;
	}

	dev_paddr = dev_info[info->ipc_id].paddress;

	// Set up control registers
	_writel((void *)(dev_paddr + XPC_DSET(info->mailbox_idx)),
		destination_mask);

	_writel((void *)(dev_paddr + XPC_MSET(info->mailbox_idx)),
		(destination_mask | (0x1U << info->ipc_irq_id)));

	_writel((void *)(dev_paddr + XPC_MODE(info->mailbox_idx)),
		info->mailbox_mode);

	xdebug_event_record(info->ipc_id, info->mailbox_idx, XPC_EVT_SEND,
			    destination_mask);

	// Ensure cross-core "read after" ordering
	smp_store_release((uint32_t *)(dev_paddr + XPC_SEND(info->mailbox_idx)),
			  XPC_SEND_DEST);

	// If gsm extensions weren't inited already, let's try now
	if (!dev_info[info->ipc_id].gsm_ext_inited) {
		dev_info[info->ipc_id].gsm_ext_inited =
			try_init_gsm_extensions(&dev_info[info->ipc_id]);
	}

	return PL320_STATUS_SUCCESS;
}

Pl320HalStatus pl320_reply(Pl320MailboxInfo *info, void *data, uint16_t length)
{
	Pl320HalStatus status;

	CHECK_INFO_NULL(info)

	if ((status = check_xpc_mailbox(info)) != PL320_STATUS_SUCCESS) {
		return status;
	}

	if ((info->mailbox_mode != PL320_MAILBOX_MODE_AUTO_ACK) &&
	    (info->mailbox_mode != PL320_MAILBOX_MODE_AUTO_LINK)) {
		if (length > XPC_MAILBOX_SIZE) {
			return PL320_STATUS_FAILURE_DATA_LENGTH;
		}

		if ((length != 0) && (data == NULL)) {
			return PL320_STATUS_FAILURE_DATA_NULL;
		}

		if (load_mailbox_data(info, data, length) !=
		    PL320_STATUS_SUCCESS) {
			return PL320_STATUS_FAILURE_DATA_LOAD;
		}
	}

	return acknowledge_mailbox(info);
}

Pl320HalStatus pl320_read_mailbox_data(Pl320MailboxInfo *info, uint8_t offset,
				       uint8_t *buffer, uint16_t buffer_len,
				       uint16_t *length)
{
	uint32_t i, j;
	uint32_t db_word;
	uint32_t db_start_aligned;
	uint32_t db_end;
	uint8_t len_bytes;
	uint8_t len_dw;
	uint8_t buf_idx = 0;
	Pl320HalStatus status;

	CHECK_INFO_NULL(info)

	status = check_xpc_mailbox(info);
	if (status != PL320_STATUS_SUCCESS) {
		return status;
	}

	if (NULL == buffer) {
		return PL320_STATUS_FAILURE_DATA_NULL;
	}

	if (NULL == length) {
		return PL320_STATUS_FAILURE_NULL_POINTER;
	}

	if (0x0 == buffer_len) {
		return PL320_STATUS_FAILURE_DATA_LENGTH;
	}

	len_bytes = XPC_MAILBOX_SIZE - offset;
	len_bytes = (len_bytes > buffer_len) ? buffer_len : len_bytes;
	len_dw = (len_bytes >> 2) + ((len_bytes & 0x3) ? 0x1 : 0x0);

	db_start_aligned = offset >> 2;
	db_end = db_start_aligned + len_dw - 1;

	for (i = db_start_aligned; i <= db_end; i++) {
		db_word = _readl((void *)(dev_info[info->ipc_id].paddress +
					  XPC_DRX(info->mailbox_idx, i)));
		// Peforming byte wise access in case architecture doesn't support
		// unaligned writes
		for (j = (offset & 0x3); (j < 4) && (buf_idx < len_bytes);
		     j++) {
			buffer[buf_idx++] =
				(db_word >> ((uint8_t)j << 3)) & 0xFF;
		}
		offset = 0x0;
	}
	*length = buf_idx;

	return PL320_STATUS_SUCCESS;
}

Pl320HalStatus pl320_read_mailbox_irq(Pl320MailboxInfo *info)
{
	uint32_t curr_idx;
	uint32_t send_reg;
	uint32_t i;
	Pl320HalStatus status;
	uint32_t irq_mask;

	CHECK_INFO_NULL(info)

	/* Removed when mailbox_enable started to be used
		at this point the mailbox that raised the interrupt is unknown */
	// status = check_xpc_mailbox(info);
	// if (status != PL320_STATUS_SUCCESS) {
	//    return status;
	//}

	status = read_mailbox_irq_mask(info);
	if (status != PL320_STATUS_SUCCESS) {
		return status;
	}

	irq_mask = info->mailbox_irq_mask;
	if (0x0 == irq_mask) {
		info->irq_cause = PL320_IRQ_REASON_NONE;
		return status;
	}

	curr_idx = (info->mailbox_idx + 1) % XPC_HAL_NUM_CHANNELS;

	for (i = 0; i < 32; i++) {
		if (info->mailbox_irq_mask & (0x1 << curr_idx)) {
			info->mailbox_idx = curr_idx;

			// Ensure cross-core "read after" ordering
			send_reg = smp_load_acquire(
				(uint32_t *)(dev_info[info->ipc_id].paddress +
					     XPC_SEND(info->mailbox_idx)));

			if (send_reg == 0x1) {
				info->irq_cause =
					PL320_IRQ_REASON_DESTINATION_INT;
			} else if (send_reg == 0x2) {
				info->irq_cause = PL320_IRQ_REASON_SOURCE_ACK;
			} else {
				info->irq_cause = PL320_IRQ_REASON_UNKNOWN;
			}

			_writel((void *)(dev_info[info->ipc_id].paddress +
					 (uintptr_t)XPC_MCLEAR(
						 info->mailbox_idx)),
				(1 << info->ipc_irq_id));

			xdebug_event_record(info->ipc_id, info->mailbox_idx,
					    XPC_EVT_CLR_MASK_END,
					    (1 << info->ipc_irq_id));
			break;
		}
		curr_idx = (curr_idx + 1) % XPC_HAL_NUM_CHANNELS;
	}

	status = read_mailbox_irq_mode(info);
	if (status != PL320_STATUS_SUCCESS) {
		return status;
	}

	return status;
}

Pl320HalStatus pl320_release_mailbox(Pl320MailboxInfo *info)
{
	pl320_device_info *device = NULL;
	Pl320HalStatus status;
	int i;

	CHECK_INFO_NULL(info)

	status = check_xpc_mailbox(info);
	if (status != PL320_STATUS_SUCCESS) {
		return status;
	}

	PL320_HAL_CRITICAL_SECTION_START
	device = &dev_info[info->ipc_id];

	xdebug_event_record(info->ipc_id, info->mailbox_idx,
			    XPC_EVT_RELEASE_START, 0);

	_writel((void *)(device->paddress + XPC_SOURCE(info->mailbox_idx)),
		0x0);

	xdebug_event_record(info->ipc_id, info->mailbox_idx,
			    XPC_EVT_RELEASE_END, 0);

	for (i = 0; i < 32; i++) {
		if ((device->channel_id_mask[i] & (0x1 << info->mailbox_idx))) {
			device->channel_id_mask[i] &=
				~(0x1 << info->mailbox_idx);
			break;
		}
	}

	if (supports_fast_mailbox_acquisition(info)) {
		pl320_gsm_clr_mailbox_hint(device, info->mailbox_idx);
	}

	PL320_HAL_CRITICAL_SECTION_END

	return PL320_STATUS_SUCCESS;
}

Pl320HalStatus pl320_clear_mailbox_irq(Pl320MailboxInfo *info)
{
	uint64_t dev_paddr = dev_info[info->ipc_id].paddress;

	xdebug_event_record(info->ipc_id, info->mailbox_idx,
			    XPC_EVT_CLR_MASK_START, (1 << info->ipc_irq_id));

	_writel((void *)(dev_paddr + XPC_MCLEAR(info->mailbox_idx)),
		(1 << info->ipc_irq_id));

	xdebug_event_record(info->ipc_id, info->mailbox_idx,
			    XPC_EVT_CLR_MASK_END, (1 << info->ipc_irq_id));
	return PL320_STATUS_SUCCESS;
}

Pl320HalStatus pl320_get_debug_info(pl320_debug_info *dinfo, uint8_t ipc_id)
{
	int i;
	uint64_t irq_status_base;
	uint64_t mbox_base;

	CHECK_INFO_NULL(dinfo);
	dinfo->ipc_id = ipc_id;

	irq_status_base = dev_info[ipc_id].paddress + 0x800;

	for (i = 0; i < XPC_HAL_NUM_CHANNELS; ++i) {
		dinfo->irq_mask[i] =
			*((uint32_t *)(irq_status_base + i * 8 + 0x0));
		dinfo->irq_raw[i] =
			*((uint32_t *)(irq_status_base + i * 8 + 0x4));
		dinfo->channel_id_mask[i] = dev_info[ipc_id].channel_id_mask[i];
	}

	mbox_base = dev_info[ipc_id].paddress;

	dinfo->num_in_use = 0;

	for (i = 0; i < XPC_HAL_NUM_MAILBOXES; ++i) {
		dinfo->active_source[i] =
			*((uint32_t *)(mbox_base + i * 64 + 0x0));
		dinfo->active_target[i] = 0;
		dinfo->data_reg_0_1_2_6[i][0] = 0;
		dinfo->data_reg_0_1_2_6[i][1] = 0;
		dinfo->data_reg_0_1_2_6[i][2] = 0;
		dinfo->data_reg_0_1_2_6[i][3] = 0;

		if (dinfo->active_source[i] != 0) {
			dinfo->active_target[i] =
				*((uint32_t *)(mbox_base + i * 64 + 0x0C));
			dinfo->mode_reg[i] =
				*((uint32_t *)(mbox_base + i * 64 + 0x10));
			dinfo->mstatus_reg[i] =
				*((uint32_t *)(mbox_base + i * 64 + 0x1C));
			dinfo->send_reg[i] =
				*((uint32_t *)(mbox_base + i * 64 + 0x20));

			dinfo->data_reg_0_1_2_6[i][0] =
				*((uint32_t *)(mbox_base + i * 64 + 0x24));
			dinfo->data_reg_0_1_2_6[i][1] =
				*((uint32_t *)(mbox_base + i * 64 + 0x28));
			dinfo->data_reg_0_1_2_6[i][2] =
				*((uint32_t *)(mbox_base + i * 64 + 0x2C));
			dinfo->data_reg_0_1_2_6[i][3] =
				*((uint32_t *)(mbox_base + i * 64 + 0x3C));

			dinfo->num_in_use++;
		}
	}
	return PL320_STATUS_SUCCESS;
}
