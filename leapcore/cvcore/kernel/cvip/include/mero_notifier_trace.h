/* SPDX-License-Identifier: GPL-2.0-only
 *
 * Copyright (c) 2020-2022, Magic Leap, Inc. All rights reserved.
 */

#undef TRACE_SYSTEM
#define TRACE_SYSTEM ml_mero_notifier

#if !defined(_TRACE_ML_MERO_NOTIFIER_H) || defined(TRACE_HEADER_MULTI_READ)
#define _TRACE_ML_MERO_NOTIFIER_H

#include <linux/tracepoint.h>


TRACE_EVENT(ml_mero_notifier_ioctl_wait_sleep,
            TP_PROTO(uint32_t cmd, uint32_t arg, uint32_t timed),

            TP_ARGS(cmd, arg, timed),

            TP_STRUCT__entry(
                __field(uint32_t, cmd)
                __field(uint32_t, arg)
                __field(uint32_t, timed)),

            TP_fast_assign(
                __entry->cmd   = cmd;
                __entry->arg   = arg;
                __entry->timed = timed;),

            TP_printk("cmd=0x%08X, arg=0x%08X, timed=%d",
                      __entry->cmd, __entry->arg, __entry->timed));


TRACE_EVENT(ml_mero_notifier_ioctl_wait_wake,
            TP_PROTO(uint32_t cmd, uint32_t arg, uint32_t retval),

            TP_ARGS(cmd, arg, retval),

            TP_STRUCT__entry(
                __field(uint32_t, cmd)
                __field(uint32_t, arg)
                __field(uint32_t, retval)),

            TP_fast_assign(
                __entry->cmd    = cmd;
                __entry->arg    = arg;
                __entry->retval = retval;),

            TP_printk("cmd=0x%08X, arg=0x%08X, retval=%d",
                      __entry->cmd, __entry->arg, __entry->retval));


TRACE_EVENT(ml_mero_notifier_ioctl_signal,
            TP_PROTO(uint32_t cmd, uint32_t arg, uint32_t user_call),

            TP_ARGS(cmd, arg, user_call),

            TP_STRUCT__entry(
                __field(uint32_t, cmd)
                __field(uint32_t, arg)
                __field(uint32_t, user_call)),

            TP_fast_assign(
                __entry->cmd = cmd;
                __entry->arg   = arg;
                __entry->user_call = user_call;),


            TP_printk("cmd=0x%08X, arg=0x%08X user_call=%d",
                __entry->cmd, __entry->arg, __entry->user_call));

#endif

#undef TRACE_INCLUDE_PATH
#define TRACE_INCLUDE_PATH .
#undef TRACE_INCLUDE_FILE
#define TRACE_INCLUDE_FILE mero_notifier_trace
#include <trace/define_trace.h>

#ifndef __KERNEL__
#error "Can't include kernel header in userspace"
#endif
