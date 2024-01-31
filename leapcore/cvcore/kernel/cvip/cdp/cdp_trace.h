/* SPDX-License-Identifier: GPL-2.0-only
 *
 * Copyright (c) 2022, Magic Leap, Inc. All rights reserved.
 */

#ifndef __KERNEL__
    #error "Can't include kernel header in userspace"
#endif

#undef TRACE_SYSTEM
#define TRACE_SYSTEM ml_cdp

#if !defined(_TRACE_ML_CDP_H) || defined(TRACE_HEADER_MULTI_READ)
#define _TRACE_ML_CDP_H

#include <linux/tracepoint.h>


TRACE_EVENT(ml_cdp_intr,
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

            TP_printk("intr: irq=%d, irq_ix=%d", __entry->irq, __entry->irq_ix));


TRACE_EVENT(ml_cdp_start,
            TP_PROTO(dma_addr_t ioaddr_inp0, dma_addr_t ioaddr_inp1, dma_addr_t ioaddr_inp2),

            TP_ARGS(ioaddr_inp0, ioaddr_inp1, ioaddr_inp2),

            TP_STRUCT__entry(
                __field(dma_addr_t, ioaddr_inp0)
                __field(dma_addr_t, ioaddr_inp1)
                __field(dma_addr_t, ioaddr_inp2)),

            TP_fast_assign(
               __entry->ioaddr_inp0 = ioaddr_inp0;
               __entry->ioaddr_inp1 = ioaddr_inp1;
               __entry->ioaddr_inp2 = ioaddr_inp2;),

            TP_printk("start: inp0=0x%llx, inp1=0x%llx, inp2=0x%llx", __entry->ioaddr_inp0, __entry->ioaddr_inp1, __entry->ioaddr_inp2));


#endif

#undef TRACE_INCLUDE_PATH
#define TRACE_INCLUDE_PATH .
#undef TRACE_INCLUDE_FILE
#define TRACE_INCLUDE_FILE cdp_trace
#include <trace/define_trace.h>
