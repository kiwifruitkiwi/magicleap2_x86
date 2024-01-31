/* SPDX-License-Identifier: GPL-2.0-only
 *
 * Copyright (c) 2020-2022, Magic Leap, Inc. All rights reserved.
 */

#ifndef __PL320_HAL_H
#define __PL320_HAL_H

#include "pl320_types.h"

#define PL320_NUM_MAILBOXES (32)

 /**
  * Initialize a PL320 hardware instance.
  *
  * \param [in]       ipc_id              The id of the hardware to initialize.
  * \param [in]       paddress            The physical address of the hardware instance.
  * \param [in]       source_channels     The mask of source channels to use.
  * \param [in]       mailbox_enable      The mask of mailbox resources to use.
  *
  * \return HAL_SUCCESS if initialization was successfull.\n
  *         PL320_STATUS_FAILURE_DEVICE_INVALID if the id or paddress do\n
  *         not point to a valid PL320 device.\n
  */
Pl320HalStatus pl320_initialize_hardware(uint8_t ipc_id, uint64_t paddress,
	uint32_t source_channels,
	uint32_t mailbox_enable);

/**
 * Initialize a PL320 hardware instance.
 *
 * \param [in]       ipc_id              The id of the hardware to initialize.
 *
 * \return HAL_SUCCESS if de-initialization was successfull.\n
 *         PL320_STATUS_FAILURE_DEVICE_INVALID if the id does\n
 *         not point to a valid PL320 device.\n
 */
Pl320HalStatus pl320_deinitialize_hardware(uint8_t ipc_id);

/**
 * Acquire pl320 resources (channel, mailbox) but do not send.
 *
 * \param [in]       info               Mailbox descriptor to be initialized by function.
 * \param [in]       destination_mask   Destinations cores.
 *
 * \return HAL_SUCCESS if resource acquisition was successfull.\n
 *         HAL_FAILURE_CHANNEL_UNAVAILABLE if a channel cannont be allocated.\n
 *         HAL_FAILURE_MAILBOX_UNAVAILABLE if a mailbox cannot be allocated.\n
 */
Pl320HalStatus pl320_acquire_mailbox(Pl320MailboxInfo* info,
	uint32_t destination_mask);

/**
 * Send data to one or more destinations.
 *
 * \param [in]       info                Mailbox descriptor initialized by caller.
 * \param [in]       destination_mask    Destinations for the mailbox.
 * \param [in]       data                Pointer to data to send.
 * \param [in]       length              Length of data to send.
 *
 * \return HAL_SUCCESS if send was successfull.\n
 *         HAL_FAILURE_DATA_LENGTH if the data length is greater than IPC_MAILBOX_SIZE.\n
 *         HAL_FAILURE_DATA_NULL if length is non-zero and data is NULL.\n
 *         HAL_FAILURE_CHANNEL_UNAVAILABLE if a channel cannont be allocated.\n
 *         HAL_FAILURE_MAILBOX_UNAVAILABLE if a mailbox cannot be allocated.\n
 *         HAL_FAILURE_DATA_LOAD if a data load error occurs.\n
 */
Pl320HalStatus pl320_send(Pl320MailboxInfo* info, uint32_t destination_mask,
	void* data, uint16_t length);

/**
 * Reply to an incomming message with or without data. Caller must initialize
 * the channel_id, ipc_id and mailbox_idx to reply on.  This info would
 * typically be initialized by an incomming message handler.
 *
 * \param [in]       info                Mailbox descriptor initialized by caller.
 * \param [in]       data                Pointer to data to send as reply.
 * \param [in]       length              Length of data to send.
 *
 * \return HAL_SUCCESS if the reply was sent successfully.\n
 *         HAL_FAILURE_NULL_POINTER if info is NULL.\n
 *         HAL_FAILURE_DATA_LENGTH if the data length is greater than IPC_MAILBOX_SIZE.\n
 *         HAL_FAILURE_DATA_NULL if length is non-zero and data is NULL.\n
 *         HAL_FAILURE_DATA_LOAD if a data load error occurs.\n
 */
Pl320HalStatus pl320_reply(Pl320MailboxInfo* info, void* data, uint16_t length);

/**
 * Read data from a mailbox.
 *
 * \param [in]       info                Mailbox descriptor initialized by caller.
 * \param [in]       offset              A 4-byte aligned offset to start reading.
 * \param [out]      buffer              Pointer to buffer to fill.
 * \param [in]       buffer_len          Maximum length of buffer in bytes.
 * \param [out]      length              Number of bytes filled in buffer.
 *
 * \return HAL_SUCCESS if the mailbox data was read successfully.\n
 *         HAL_FAILURE_NULL_POINTER if info or length is NULL.\n
 *         HAL_FAILURE_DATA_LENGTH if the data length is greater than IPC_MAILBOX_SIZE.\n
 *         HAL_FAILURE_DATA_NULL if length is non-zero and data is NULL.\n
 */
Pl320HalStatus pl320_read_mailbox_data(Pl320MailboxInfo* info, uint8_t offset,
	uint8_t* buffer, uint16_t buffer_len,
	uint16_t* length);

/**
 * Retrieve interrupt info for a specificed ipc mailbox. Caller must initialize
 * the channel_id and ipc_id.
 *
 * \param [in,out]   info                Caller initialized mailbox descriptor
 *                                       to be filled.
 *
 * \return HAL_SUCCESS if the irq info was retrieved successfully.\n
 *         HAL_FAILURE_NULL_POINTER if info or length is NULL.\n
 */
Pl320HalStatus pl320_read_mailbox_irq(Pl320MailboxInfo* info);

/**
 * Clear interrupt for a specificed ipc mailbox. Caller must initialize
 * the channel_id, ipc_id and mailbox id.
 *
 * \param [in,out]   info                Caller initialized mailbox descriptor
 *                                       to be filled.
 *
 * \return HAL_SUCCESS if the irq info was cleared successfully.\n
 *         HAL_FAILURE_NULL_POINTER if info or length is NULL.\n
 */
Pl320HalStatus pl320_clear_mailbox_irq(Pl320MailboxInfo* info);

/**
 * Release a specified ipc mailbox.
 *
 * \param [in]       info                Caller initialized mailbox descriptor to be released.
 *
 * \return HAL_SUCCESS if the release was successful.\n
 *         HAL_FAILURE_NULL_POINTER if info or length is NULL.\n
 */
Pl320HalStatus pl320_release_mailbox(Pl320MailboxInfo* info);

/**
 * Collect debug info for a given IPC device.
 *
 * \param [in]       dinfo               Debug info structure to be filled.
 * \param [in]       ipc_id              The id of the IPC device to capture debug info from.
 *
 * \return HAL_SUCCESS if debug capture was successful.\n
 *         HAL_FAILURE_NULL_POINTER if info is NULL.\n
 */
Pl320HalStatus pl320_get_debug_info(pl320_debug_info* dinfo, uint8_t ipc_id);

#ifndef __KERNEL__
#error "Can't include kernel header in userspace"
#endif

#endif
