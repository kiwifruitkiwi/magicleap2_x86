/* SPDX-License-Identifier: GPL-2.0-only
 *
 * \file mero_xpc.h
 *
 * \brief  mero_xpc API definitions.
 *
 * Copyright (c) 2020-2022, Magic Leap, Inc. All rights reserved.
 */

#ifndef __MERO_XPC_H
#define __MERO_XPC_H

#include <linux/types.h>

#include "mero_xpc_types.h"

 /**
 * This header defines the Cross(X) Processor Communication API, mero_xpc.
 * mero_xpx consists of three client interfaces: Command/Response, Notification
 * and Dispatch Queues.
 *
 * The Command/Response interface supports command and response interaction between a
 * a single initiator and a single recipient. A command is sent from the initiator
 * on a source core and is received by a recipient on a target core. The source core
 * and target core may be the same. Command recipients that are Linux userspace
 * actively wait on incomming commands by calling xpc_command_recv, passing
 * a delegate that will be called when a command arrives. Kernel mode recipients may
 * optionally register callbacks to be invoked asynchronously upon command arrival.
 * A recipient replies to a command by filling a response buffer passed as an argument
 * to the callback or delegate. The initiator waits on the response from the recipient.
 * The command/response interface supports a synchronous command-wait-response mode
 * as well as a split command-send/wait-response variant.
 */


 /**
 * Send a command to a core and blocking wait for a response.
 *
 * \param [in]       channel                 The xpc virtual command channel to use.
 * \param [in]       target                  The ID of the destination core.
 * \param [in]       command                 Pointer to the command buffer.
 * \param [in]       command_length          The length in bytes of the command buffer.
 * \param [out]      response                Pointer to the response buffer.
 * \param [in]       response_max_length     The max length in bytes of the response buffer.
 * \param [out]      response_length         The length in bytes of the response.
 *
 * \return   0 if a command was sent a response was successfully received.\n
 *           -ECHRNG if channel is out-of-range.\n
 *           -EINVAL if the target is invalid.\n
 *           -EINVAL if command is NULL.\n
 *           -EINVAL if command_legnth is zero.\n
 *           -EINVAL if command_length is greater than XPC_MAX_COMMAND_LENGTH
 *           -EINVAL if response is NULL.\n
 *           -EINVAL if response_max_legnth is less than XPC_MAX_RESPONSE_LENGTH.\n
 *           -EINVAL if response_legnth is NULL.\n
 *           -EIO if an internal error occurs.\n
 */
int xpc_command_send(XpcChannel channel, XpcTarget target, uint8_t* command, size_t command_length,
	uint8_t* response, size_t response_max_length, size_t* response_length);

/**
* Send a command without blocking on a response.
*
* \param [in]       channel                 The xpc virtual command channel to send the command.
* \param [in]       target                  The ID of the destination core.
* \param [in]       command                 Pointer to the command buffer.
* \param [in]       command_length          The length in bytes of the command buffer.
* \param [out]      ticket                  A ticket for future blocking wait on response.
*
* \return   0 if a command was sent successfully.\n
*           -ECHRNG if channel is out-of-range.\n
*           -EINVAL if the target is invalid.\n
*           -EINVAL if command is NULL.\n
*           -EINVAL if command_legnth is zero.\n
*           -EINVAL if command_length is greater than XPC_MAX_COMMAND_LENGTH
*           -EINVAL if ticket is NULL.\n
*           -EIO if an internal error occurs.\n
*/
int xpc_command_send_async(XpcChannel channel, XpcTarget target, uint8_t* command, size_t command_length,
	XpcAsyncTicket* ticket);

/**
* Blocking wait on a respone for a command previously sent with xpc_command_send_async.
*
* \param [in]       ticket                  The ticket returned from xpc_command_send_async.
* \param [out]      response                Pointer to the response buffer.
* \param [in]       response_max_length     The max length in bytes of the response buffer.
* \param [out]      response_length         The length in bytes of the response.
*
* \return   0 if a response was successfully received.\n
*           -EINVAL if the target is invalid.\n
*           -EINVAL if response is NULL.\n
*           -EINVAL if response_max_legnth is less than XPC_MAX_RESPONSE_LENGTH.\n
*           -EINVAL if response_legnth is NULL.\n
*           -EINVAL if ticket is invalid.\n
*           -EIO if an internal error occurs.\n
*/
int xpc_command_wait_response_async(XpcAsyncTicket* ticket, uint8_t* response,
	size_t response_max_length, size_t* response_length);

/**
* Blocking wait on arrival of a command.
*
* \param [in]       channel                 The xpc virtual command channel to wait on.
* \param [in]       handler                 An optional callback to be invoked on command arrival.
* \param [in]       args                    Optional argument to be passed to callback.
*
* \return   0 if a command was received and callback invoked.\n
*           -ECHRNG if channel is out-of-range.\n
*           -EAGAIN if resources are temporarily unavailable.\n
*           -EIO if an internal error occurs.\n
*/
int xpc_command_recv(XpcChannel channel, XpcCommandHandler handler, XpcHandlerArgs args);

/**
* Send a notification to one or more cores.
*
* \param [in]       channel                 The xpc virtual notification channel to send on.
* \param [in]       targets                 A mask specifying one or more destination cores.
* \param [in]       data                    Pointer to the data buffer.
* \param [in]       length                  The length in bytes of the data buffer.
* \param [in]       mode                    Mode specifying if the call blocks
*                                           (XPC_NOTIFICATION_MODE_WAIT_ACK) until all
*                                           recipients acknowledge the notice, or if the call
*                                           returns immediately (XPC_NOTIFICATION_MODE_POSTED).
*
* \return   0 if the notification was sent successfully.\n
*           -ECHRNG if channel is out-of-range.\n
*           -EINVAL if legnth is zero.\n
*           -EINVAL if legnth is greater than XPC_MAX_NOTIFICATION_LENGTH.\n
*           -EINVAL if data is NULL.\n
*           -EINVAL if the targets mask is invalid.\n
*           -EINVAL if mode is invalid.\n
*           -EAGAIN if resources are temporarily unavailable.\n
*           -ERESTARTSYS if the wait was interrupted by a signal.\n
*           -EIO if an internal error occurs.\n
*/
int xpc_notification_send(XpcChannel channel, XpcTargetMask targets, uint8_t* data, size_t length,
	XpcNotificationMode mode);

/**
* Blocking wait on arrival of a notification.
*
* \param [in]       channel                         The xpc virtual notification channel to wait on.
* \param [in]       notification_data               Pointer to a buffer to copy the notification into.
* \param [in]       notification_data_length        The max length in bytes of the notification buffer.
*
* \return   0 if a notification was received.\n
*           -ECHRNG if channel is out-of-range.\n
*           -EINVAL if notification_data is NULL.\n
*           -EINVAL if notification_data_length is less than XPC_MAX_NOTIFICATION_LENGTH.\n
*           -EIO if an internal error occurs.\n
*/
int xpc_notification_recv(XpcChannel channel, uint8_t* notification_data,
	size_t notification_data_length);

/**
* Send data on specified queue channel to one or more cores.
*
* \param [in]       channel                 The xpc virtual queue channel to send on.
* \param [in]       sub_channel             The xpc sub queue channel to wait on.
* \param [in]       targets                 A mask specifying one or more destination cores.
* \param [in]       data                    Pointer to the data buffer.
* \param [in]       length                  The length in bytes of the data buffer.
*
* \return   0 if the queue was sent successfully.\n
*           -ECHRNG if channel is out-of-range.\n
*           -EINVAL if legnth is zero.\n
*           -EINVAL if length is greater than XPC_MAX_QUEUE_LENGTH
*           -EINVAL if data is NULL.\n
*           -EINVAL if the targets mask is invalid.\n
*           -EAGAIN if resources are temporarily unavailable.\n
*           -EIO if an internal error occurs.\n
*/
int xpc_queue_send(XpcChannel channel, XpcSubChannel sub_channel, XpcTargetMask targets,
	uint8_t* data, size_t length);

/**
* Blocking wait on arrival of data on a registered queue.
*
* \param [in]       channel                     The xpc virtual queue channel to wait on.
* \param [in]       sub_channel                 The xpc sub queue channel to wait on.
* \param [out]      queue_data                  Pointer to a buffer to copy incoming data into.
* \param [in]       queue_data_length           The max length in bytes of the queue buffer.
*
* \return   0 if a queue was received.\n
*           -ECHRNG if channel is out-of-range.\n
*           -EINVAL if queue_data is NULL.\n
*           -EINVAL if queue_data_length is zero.\n
*           -EINVAL if queue_data_length is greater than XPC_MAX_QUEUE_LENGTH.\n
*           -EPERM if channel/sub-channel is not currently registered.\n
*           -ERESTARTSYS if the wait was interrupted by a signal.\n
*/
int xpc_queue_recv(XpcChannel channel, XpcSubChannel sub_channel, uint8_t* queue_data,
	size_t queue_data_length);

/**
* Register channel/sub_channel to wait on.
*
* \param [in]       channel                     The xpc virtual queue channel to register.
* \param [in]       sub_channel                 The xpc queue sub channel to register.
*
* \return   0 if register was successful.\n
*           -ECHRNG if channel is out-of-range.\n
*           -EBUSY if the channel/sub-channel is already registered.\n
*/
int xpc_queue_register(XpcChannel channel, XpcSubChannel sub_channel);

/**
* Deregister channel/sub_channel to wait on.
*
* \param [in]       channel                     The xpc virtual queue channel to deregister.
* \param [in]       sub_channel                 The xpc queue sub channel to deregister.
*
* \return   0 if deregister was successful.\n
*           -ECHRNG if channel is out-of-range.\n
*           -EPERM if channel/sub-channel is not currently registered.\n
*/
int xpc_queue_deregister(XpcChannel channel, XpcSubChannel sub_channel);

/** \} */

#ifndef __KERNEL__
#error "Can't include kernel header in userspace"
#endif

#endif
