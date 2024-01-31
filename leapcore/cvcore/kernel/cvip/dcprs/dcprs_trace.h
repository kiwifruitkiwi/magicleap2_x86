/* SPDX-License-Identifier: GPL-2.0-only
 *
 * Copyright (c) 2022, Magic Leap, Inc. All rights reserved.
 */

#ifndef __KERNEL__
    #error "Can't include kernel header in userspace"
#endif

#undef TRACE_SYSTEM
#define TRACE_SYSTEM ml_dcprs

#if !defined(_TRACE_ML_DCPRS_H) || defined(TRACE_HEADER_MULTI_READ)
#define _TRACE_ML_DCPRS_H

#include <linux/tracepoint.h>

#ifndef _TRIRQTAB
#define _TRIRQTAB
#ifdef CONFIG_DEBUG_FS
static char *tr_irq_tab[] = {
    "axi_err_interrupt_reg_freeze",
    "axi_err_interrupt_reg_halt",
    "axi_err_interrupt_reg_intr",
    "mp0_func_event_reg_freeze",
    "mp0_func_event_reg_halt",
    "mp0_func_event_reg_intr",
    "mp0_err_interrupt_reg_freeze",
    "mp0_err_interrupt_reg_halt",
    "mp0_err_interrupt_reg_intr",
    "mp1_func_event_reg_freeze",
    "mp1_func_event_reg_halt",
    "mp1_func_event_reg_intr",
    "mp1_err_interrupt_reg_freeze",
    "mp1_err_interrupt_reg_halt",
    "mp1_err_interrupt_reg_intr"
};
#endif
#endif

TRACE_EVENT(ml_dcprs_intr,
            TP_PROTO(uint32_t irq, uint32_t irq_ix, uint32_t ctr),

            TP_ARGS(irq, irq_ix, ctr),

            TP_STRUCT__entry(
                __field(uint32_t, irq)
                __field(uint32_t, irq_ix)
                __field(uint32_t, ctr)),

            TP_fast_assign(
               __entry->irq    = irq;
               __entry->irq_ix = irq_ix;
               __entry->ctr    = ctr;),

            TP_printk("intr: irq=%d, irq_ix=%d ctr=%d [%s]", __entry->irq, __entry->irq_ix, __entry->ctr, tr_irq_tab[__entry->irq_ix]));


TRACE_EVENT(ml_dcprs_start,
            TP_PROTO(dma_addr_t ioaddr_dst, dma_addr_t ioaddr_g, dma_addr_t ioaddr_rb, uint32_t len_g, uint32_t len_rb, uint32_t len_slice),

            TP_ARGS(ioaddr_dst, ioaddr_g, ioaddr_rb, len_g, len_rb, len_slice),

            TP_STRUCT__entry(
                __field(dma_addr_t, ioaddr_dst)
                __field(dma_addr_t, ioaddr_g)
                __field(dma_addr_t, ioaddr_rb)
                __field(uint32_t,   len_g)
                __field(uint32_t,   len_rb)
                __field(uint32_t,   len_slice)),

            TP_fast_assign(
               __entry->ioaddr_dst = ioaddr_dst;
               __entry->ioaddr_g   = ioaddr_g;
               __entry->ioaddr_rb  = ioaddr_rb;
               __entry->len_g      = len_g;
               __entry->len_rb     = len_rb;
               __entry->len_slice  = len_slice;),

            TP_printk("start: ioaddr_dst=0x%llx, ioaddr_g=0x%llx, ioaddr_rb=0x%llx, len_g=%d, len_rb=%d, len_slice=%d", __entry->ioaddr_dst, __entry->ioaddr_g, __entry->ioaddr_rb, __entry->len_g, __entry->len_rb, __entry->len_slice));


TRACE_EVENT(ml_dcprs_start_busy,
            TP_PROTO(uint32_t p0),

            TP_ARGS(p0),

            TP_STRUCT__entry(
                __field(uint32_t, p0)),

            TP_fast_assign(
               __entry->p0 = p0;),

            TP_printk("BUSY: p0=%d", __entry->p0));


TRACE_EVENT(ml_dcprs_hdr_err,
            TP_PROTO(uint32_t eng, uint32_t exp, uint32_t got),

            TP_ARGS(eng, exp, got),

            TP_STRUCT__entry(
                __field(uint32_t, eng)
                __field(uint32_t, exp)
                __field(uint32_t, got)),

            TP_fast_assign(
               __entry->eng  = eng;
               __entry->exp  = exp;
               __entry->got  = got;),

            TP_printk("SOF missing: chan=%s, exp=0x%08X, got=0x%08X", __entry->eng == 0 ? "G" : "RB", __entry->exp, __entry->got));


TRACE_EVENT(ml_dcprs_end_wait_begin,
            TP_PROTO(uint32_t p0),

            TP_ARGS(p0),

            TP_STRUCT__entry(
                __field(uint32_t, p0)),

            TP_fast_assign(
               __entry->p0 = p0;),

            TP_printk("end_wait_begin: p0=%d", __entry->p0));


TRACE_EVENT(ml_dcprs_end_wait_done,
            TP_PROTO(int to, uint32_t ctr),

            TP_ARGS(to, ctr),

            TP_STRUCT__entry(
                __field(uint32_t, to)
                __field(uint32_t, ctr)),

            TP_fast_assign(
               __entry->to  = to;
               __entry->ctr = ctr;),

            TP_printk("end_wait_done: to=%d ctr=%d", __entry->to, __entry->ctr));


TRACE_EVENT(ml_dcprs_sect_next,
            TP_PROTO(uint32_t eng_ix, uint32_t len, uint32_t tot),

            TP_ARGS(eng_ix, len, tot),

            TP_STRUCT__entry(
                __field(uint32_t, eng_ix)
                __field(uint32_t, len)
                __field(uint32_t, tot)),

            TP_fast_assign(
               __entry->eng_ix = eng_ix;
               __entry->len    = len;
               __entry->tot    = tot;),

            TP_printk("next_sect: eng=%d, len=%d tot=%d", __entry->eng_ix, __entry->len, __entry->tot));

#endif

#undef TRACE_INCLUDE_PATH
#define TRACE_INCLUDE_PATH .
#undef TRACE_INCLUDE_FILE
#define TRACE_INCLUDE_FILE dcprs_trace
#include <trace/define_trace.h>
