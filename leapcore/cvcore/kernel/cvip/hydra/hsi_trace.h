/* SPDX-License-Identifier: GPL-2.0-only
 *
 * Copyright (c) 2022, Magic Leap, Inc. All rights reserved.
 */

#ifndef __KERNEL__
    #error "Can't include kernel header in userspace"
#endif

#undef TRACE_SYSTEM
#define TRACE_SYSTEM ml_hsi

#if !defined(_TRACE_ML_HSI_H) || defined(TRACE_HEADER_MULTI_READ)
#define _TRACE_ML_HSI_H

#include <linux/tracepoint.h>
#ifndef HSI_UID_XSPILLWAY
#define HSI_UID_XSPILLWAY   (0xdead)              // associated with dropped frames
#endif
#define HSI_TRACE_LABEL_LEN     (64)

#define UID_DECODE2(u)   ((!(u & (HSI_UID_HEADER | HSI_UID_FOOTER)))    ? "DATA" : "UNKN")
#define UID_DECODE1(u)   ((u & HSI_UID_FOOTER)    ? "FOOT" : (UID_DECODE2(u)))
#define UID_DECODE0(u)   ((u & HSI_UID_HEADER)    ? "HEAD" : (UID_DECODE1(u)))
#define UID_DECODE(u)    ((u == HSI_UID_XSPILLWAY) ? "DROP" : (UID_DECODE0(u)))


TRACE_EVENT(ml_hsi_busy_check,
            TP_PROTO(uint32_t dix, int32_t is_busy),

            TP_ARGS(dix, is_busy),

            TP_STRUCT__entry(
                __field(uint32_t, dix)
                __field(int32_t, is_busy)),

            TP_fast_assign(
               __entry->dix     = dix;
               __entry->is_busy = is_busy;),

            TP_printk("dix=%d, is_busy=%d", __entry->dix, __entry->is_busy));


TRACE_EVENT(ml_hsi_intr_top,
            TP_PROTO(uint32_t cpu, uint32_t irq, uint32_t irq_ix),

            TP_ARGS(cpu, irq, irq_ix),

            TP_STRUCT__entry(
                __field(uint32_t, cpu)
                __field(uint32_t, irq)
                __field(uint32_t, irq_ix)),

            TP_fast_assign(
               __entry->cpu    = cpu;
               __entry->irq    = irq;
               __entry->irq_ix = irq_ix;),

            TP_printk("irq(%d) (TOP):: irq=%d, irq_ix=%d\n", __entry->cpu, __entry->irq, __entry->irq_ix));


TRACE_EVENT(ml_hsi_intr0,
            TP_PROTO(uint32_t cpu, uint32_t hix, uint32_t irq, uint32_t dix, uint32_t ssix, uint32_t qhead, uint32_t qtail, uint32_t qrt),

            TP_ARGS(cpu, hix, irq, dix, ssix, qhead, qtail, qrt),

            TP_STRUCT__entry(
                __field(uint32_t, cpu)
                __field(uint32_t, hix)
                __field(uint32_t, irq)
                __field(uint32_t, dix)
                __field(uint32_t, ssix)
                __field(uint32_t, qhead)
                __field(uint32_t, qtail)
                __field(uint32_t, qrt)),

            TP_fast_assign(
                __entry->cpu   = cpu;
                __entry->hix   = hix;
                __entry->irq   = irq;
                __entry->dix   = dix;
                __entry->ssix  = ssix;
                __entry->qhead = qhead;
                __entry->qtail = qtail;
                __entry->qrt   = qrt;),

            TP_printk("softirq(%d) (BOT):: ENTER hix=%d, irq=%d, dix=%d, ssix=%d, h=%d, t=%d, qrt=%d", __entry->cpu, __entry->hix, __entry->irq, __entry->dix, __entry->ssix, __entry->qhead, __entry->qtail, __entry->qrt));


TRACE_EVENT(ml_hsi_intr1,
            TP_PROTO(struct tr_args_intr *v),

            TP_ARGS(v),

            TP_STRUCT__entry(
                __field(uint32_t, vix)
                __field(uint32_t, hix)
                __field(uint32_t, dix)
                __field(uint32_t, ssix)
                __field(uint32_t, bufix)
                __field(uint64_t, ref)
                __field(uint32_t, tail)
                __field(uint32_t, dlen)
                __field(uint32_t, slice)
                __field(uint32_t, xstat)
                __field(uint32_t, uhand)
                __field(uint32_t, fcnt)
                __field(uint32_t, pass)
                __field(uint32_t, uid)
                __field(uint32_t, uidr)
                __field(uint32_t, cpu)
                __field(const uint8_t *, dev_name)
                __field(uint32_t, count)),

            TP_fast_assign(
                __entry->vix    = v->vix;
                __entry->hix    = v->hix;
                __entry->dix    = v->dix;
                __entry->ssix   = v->ssix;
                __entry->bufix  = v->bufix;
                __entry->ref    = v->ref;
                __entry->tail   = v->tail;
                __entry->dlen   = v->dlen;
                __entry->slice  = v->slice;
                __entry->xstat  = v->xstat;
                __entry->uhand  = v->uhand;
                __entry->fcnt   = v->fcnt;
                __entry->pass   = v->pass;
                __entry->uid    = v->uid;
                __entry->uidr   = v->uidr;
                __entry->cpu    = v->cpu;
                __entry->dev_name= v->dev_name;
                __entry->count  = v->count;),

            TP_printk("cpu(%d) top_loop:: pass=%d, ssix=%d, hix=%d, pdix=%d, dix=%d (%s), tail=%d, ty=%s, bix=%d, ref=0x%llx, user_id=0x%x, user_handle=0x%x, vix=%d, length=%d, slice=%d",       \
                               __entry->cpu, __entry->pass, __entry->ssix,  __entry->hix, __entry->dix, __entry->dix + __entry->vix, __entry->dev_name, __entry->tail, UID_DECODE(__entry->uid),        \
                               __entry->bufix, __entry->ref, __entry->uidr,  __entry->uhand,  __entry->vix,  __entry->dlen,  __entry->slice));


TRACE_EVENT(ml_hsi_intr2,
            TP_PROTO(struct tr_args_intr *v, uint32_t ss_head_count, uint32_t ss_data_count, uint32_t ss_foot_count),

            TP_ARGS(v, ss_head_count, ss_data_count, ss_foot_count),

            TP_STRUCT__entry(
                __field(uint32_t, vix)
                __field(uint32_t, dix)
                __field(uint32_t, ssix)
                __field(uint32_t, bufix)
                __field(uint32_t, tail)
                __field(uint32_t, dlen)
                __field(uint32_t, slice)
                __field(uint32_t, xstat)
                __field(uint32_t, uhand)
                __field(uint32_t, fcnt)
                __field(uint32_t, pass)
                __field(uint32_t, uid)
                __field(uint32_t, uidr)
                __field(uint32_t, cpu)
                __field(uint32_t, count)
                __field(uint32_t, ss_head_count)
                __field(uint32_t, ss_data_count)
                __field(uint32_t, ss_foot_count)),

            TP_fast_assign(
                __entry->vix    = v->vix;
                __entry->dix    = v->dix;
                __entry->ssix   = v->ssix;
                __entry->bufix  = v->bufix;
                __entry->tail   = v->tail;
                __entry->dlen   = v->dlen;
                __entry->slice  = v->slice;
                __entry->xstat  = v->xstat;
                __entry->uhand  = v->uhand;
                __entry->fcnt   = v->fcnt;
                __entry->pass   = v->pass;
                __entry->uid    = v->uid;
                __entry->uidr   = v->uidr;
                __entry->cpu    = v->cpu;
                __entry->count  = v->count;
                __entry->ss_head_count   = ss_head_count;
                __entry->ss_data_count   = ss_data_count;
                __entry->ss_foot_count   = ss_foot_count;),

            TP_printk("intr:: bix=%d, user_id=0x%x, length=%d, slice=%d, hdf_cnt=(%d,%d,%d), fcnt=%d", \
               __entry->bufix, __entry->uid, __entry->dlen, __entry->slice, \
               __entry->ss_head_count, __entry->ss_data_count, __entry->ss_foot_count, __entry->fcnt));


TRACE_EVENT(ml_hsi_intr_meta,
            TP_PROTO(char *str, uint32_t cpu, uint32_t hix, uint32_t dix, const char *dev_name, uint32_t ssix, void *hsibuf, uint64_t ref, void *ev, uint32_t evix, uint32_t ss_count),

            TP_ARGS(str, cpu, hix, dix, dev_name, ssix, hsibuf, ref, ev, evix, ss_count),

            TP_STRUCT__entry(
                __array(char, label, HSI_TRACE_LABEL_LEN)
                __field(uint32_t, cpu)
                __field(uint32_t, hix)
                __field(uint32_t, dix)
                __field(const char*,  dev_name)
                __field(uint32_t, ssix)
                __field(uint32_t, evix)
                __field(void *,   hsibuf)
                __field(uint64_t, ref)
                __field(void *,   ev)
                __field(uint32_t, ss_count)),

            TP_fast_assign(
                strncpy(__entry->label, str, HSI_TRACE_LABEL_LEN);
                __entry->cpu             = cpu;
                __entry->hix             = hix;
                __entry->dix             = dix;
                __entry->dev_name        = dev_name;
                __entry->ssix            = ssix;
                __entry->hsibuf          = hsibuf;
                __entry->ref             = ref;
                __entry->ev              = ev;
                __entry->evix            = evix;
                __entry->ss_count        = ss_count;),

            TP_printk("intr(%d):: %s, hix=%d, dix=%d (%s), ssix=%d, hsibuf=%px, ref=0x%llx, ev=%px, evix=%d, count=%d", __entry->cpu, __entry->label, __entry->hix, __entry->dix,__entry->dev_name, __entry->ssix, __entry->hsibuf, __entry->ref, __entry->ev, __entry->evix, __entry->ss_count));


TRACE_EVENT(ml_hsi_intr_data,
            TP_PROTO(char *str, uint32_t cpu, uint32_t dix, const char *dev_name, uint32_t ssix, void *hsibuf, void *ev, uint32_t evix, uint32_t slice, uint32_t dlen, uint32_t data_offset, uint32_t ss_data_length0, uint32_t ss_data_length1),

            TP_ARGS(str, cpu, dix, dev_name, ssix, hsibuf, ev, evix, slice, dlen, data_offset, ss_data_length0, ss_data_length1),

            TP_STRUCT__entry(
                __array(char, label, HSI_TRACE_LABEL_LEN)
                __field(uint32_t, cpu)
                __field(uint32_t, dix)
                __field(const char *, dev_name)
                __field(uint32_t, ssix)
                __field(void *,   hsibuf)
                __field(void *,   ev)
                __field(uint32_t, evix)
                __field(uint32_t, slice)
                __field(uint32_t, dlen)
                __field(uint32_t, data_offset)
                __field(uint32_t, ss_data_length0)
                __field(uint32_t, ss_data_length1)),

            TP_fast_assign(
                strncpy(__entry->label, str, HSI_TRACE_LABEL_LEN);
                __entry->cpu             = cpu;
                __entry->dix             = dix;
                __entry->dev_name        = dev_name;
                __entry->ssix            = ssix;
                __entry->hsibuf          = hsibuf;
                __entry->ev              = ev;
                __entry->evix            = evix;
                __entry->data_offset     = data_offset;
                __entry->slice           = slice;
                __entry->dlen            = dlen;
                __entry->ss_data_length0 = ss_data_length0;
                __entry->ss_data_length1 = ss_data_length1;),

            TP_printk("intr(%d):: %s, dix=%d (%s), ssix=%d, hsibuf=%px, ev=%px, evix=%d, slice=%d, dlen=%d, data_cnt=%d (%d+%d)", \
                    __entry->cpu, __entry->label, __entry->dix, __entry->dev_name, __entry->ssix, __entry->hsibuf, __entry->ev, __entry->evix, __entry->slice, __entry->dlen, __entry->data_offset, __entry->ss_data_length0, __entry->ss_data_length1));


TRACE_EVENT(ml_hsi_intr_wake,
            TP_PROTO(uint32_t cpu, uint32_t dix, const char *dev_name, void *ev, uint32_t evix, uint32_t slice, uint64_t ref, void *data, uint32_t ss_meta_length0, uint32_t ss_meta_length1, uint32_t ss_data_length0, uint32_t ss_data_length1),

            TP_ARGS(cpu, dix, dev_name, ev, evix, slice, ref, data, ss_meta_length0, ss_meta_length1, ss_data_length0, ss_data_length1),

            TP_STRUCT__entry(
                __field(uint32_t, cpu)
                __field(uint32_t, dix)
                __field(const char *, dev_name)
                __field(void *,   ev)
                __field(uint32_t, evix)
                __field(uint32_t, slice)
                __field(uint64_t, ref)
                __field(void *,   data)
                __field(uint32_t, ss_meta_length0)
                __field(uint32_t, ss_meta_length1)
                __field(uint32_t, ss_data_length0)
                __field(uint32_t, ss_data_length1)),

            TP_fast_assign(
                __entry->cpu             = cpu;
                __entry->dix             = dix;
                __entry->dev_name        = dev_name;
                __entry->ev              = ev;
                __entry->evix            = evix;
                __entry->slice           = slice;
                __entry->ref             = ref;
                __entry->data            = data;
                __entry->ss_meta_length0 = ss_meta_length0;
                __entry->ss_meta_length1 = ss_meta_length1;
                __entry->ss_data_length0 = ss_data_length0;
                __entry->ss_data_length1 = ss_data_length1;),

            TP_printk("intr(%d):: Pass up evix=%d, dix=%d (%s) ref=0x%llx, ev=%px, data=%px, meta_len=%d/%d, data_len=%d/%d", \
                             __entry->cpu, __entry->evix, __entry->dix, __entry->dev_name, __entry->ref, __entry->ev, __entry->data, __entry->ss_meta_length0, __entry->ss_meta_length1, __entry->ss_data_length0, __entry->ss_data_length1));


TRACE_EVENT(ml_hsi_intr_exit,
            TP_PROTO(uint32_t passes, uint32_t ilat),

            TP_ARGS(passes, ilat),

            TP_STRUCT__entry(
                __field(uint32_t, passes)
                __field(uint32_t, ilat)),

            TP_fast_assign(
                __entry->passes = passes;
                __entry->ilat   = ilat;),

            TP_printk("intr (BOT):: EXIT %d passes, time=%d (us)", \
                             __entry->passes, __entry->ilat / 1000));


TRACE_EVENT(ml_hsi_intr_spill,
            TP_PROTO(uint32_t cpu, uint32_t ssix, uint32_t dix, uint32_t length, uint32_t drop_count),

            TP_ARGS(cpu, ssix, dix, length, drop_count),

            TP_STRUCT__entry(
                __field(uint32_t, cpu)
                __field(uint32_t, ssix)
                __field(uint32_t, dix)
                __field(uint32_t, length)
                __field(uint32_t, drop_count)),

            TP_fast_assign(
                __entry->cpu        = cpu;
                __entry->ssix       = ssix;
                __entry->dix        = dix;
                __entry->length     = length;
                __entry->drop_count = drop_count;),

            TP_printk("intr(%d):: ssix=%d, dix=%d, spill #%d, length=%d", \
                             __entry->cpu, __entry->ssix, __entry->dix, __entry->drop_count, __entry->length));

TRACE_EVENT(ml_hsi_intr_bad_buf_ix,
            TP_PROTO(uint32_t cpu, uint32_t hix, uint32_t ssix, uint32_t dix, uint32_t bix),

            TP_ARGS(cpu, hix, ssix, dix, bix),

            TP_STRUCT__entry(
                __field(uint32_t, cpu)
                __field(uint32_t, hix)
                __field(uint32_t, ssix)
                __field(uint32_t, dix)
                __field(uint32_t, bix)),

            TP_fast_assign(
                __entry->cpu  = cpu;
                __entry->hix  = hix;
                __entry->ssix = ssix;
                __entry->dix  = dix;
                __entry->bix  = bix;),

            TP_printk("intr(%d):: BAD HSI_BUF INDEX, hix=%d, ssix=%d, dix=%d, bix=%d", \
                             __entry->cpu, __entry->hix, __entry->ssix, __entry->dix, __entry->bix));


TRACE_EVENT(ml_hsi_buf_no_vp,
            TP_PROTO(uint32_t hix, uint32_t ssix, uint32_t dix, uint32_t bix, void *buf, uint32_t act),

            TP_ARGS(hix, ssix, dix, bix, buf, act),

            TP_STRUCT__entry(
                __field(uint32_t, hix)
                __field(uint32_t, ssix)
                __field(uint32_t, dix)
                __field(uint32_t, bix)
                __field(void *,   buf)
                __field(uint32_t, act)),

            TP_fast_assign(
                __entry->hix  = hix;
                __entry->ssix = ssix;
                __entry->dix  = dix;
                __entry->bix  = bix;
                __entry->buf  = buf;
                __entry->act  = act;),

            TP_printk("no hsibuf vp: hix=%d, ssix=%d, dix=%d, bix=%d, buf=%px, active=%d", \
                             __entry->hix, __entry->ssix, __entry->dix, __entry->bix, __entry->buf, __entry->act));


TRACE_EVENT(ml_hsi_intr_corrupt_hsi_buf,
            TP_PROTO(uint32_t cpu, uint32_t hix, uint32_t ssix, uint32_t dix, uint32_t bix, uint32_t tail, uint32_t magic, uint32_t active),

            TP_ARGS(cpu, hix, ssix, dix, bix, tail, magic, active),

            TP_STRUCT__entry(
                __field(uint32_t, cpu)
                __field(uint32_t, hix)
                __field(uint32_t, ssix)
                __field(uint32_t, dix)
                __field(uint32_t, bix)
                __field(uint32_t, tail)
                __field(uint32_t, magic)
                __field(uint32_t, active)),

            TP_fast_assign(
                __entry->cpu    = cpu;
                __entry->hix    = hix;
                __entry->ssix   = ssix;
                __entry->dix    = dix;
                __entry->bix    = bix;
                __entry->tail   = tail;
                __entry->magic  = magic;
                __entry->active = active;),

            TP_printk("intr(%d):: CORRUPT HSI_BUF, hix=%d, ssix=%d, dix=%d, bix=%d, tail=%d, magic=0x%08X, active=%d", \
                             __entry->cpu, __entry->hix, __entry->ssix, __entry->dix, __entry->bix, __entry->tail, __entry->magic, __entry->active));


TRACE_EVENT(ml_hsi_intr_stale_hsi_buf,
            TP_PROTO(uint32_t cpu, uint32_t hix, uint32_t ssix, uint32_t dix, uint32_t bix, uint32_t tail),

            TP_ARGS(cpu, hix, ssix, dix, bix, tail),

            TP_STRUCT__entry(
                __field(uint32_t, cpu)
                __field(uint32_t, hix)
                __field(uint32_t, ssix)
                __field(uint32_t, dix)
                __field(uint32_t, bix)
                __field(uint32_t, tail)),

            TP_fast_assign(
                __entry->cpu    = cpu;
                __entry->hix    = hix;
                __entry->ssix   = ssix;
                __entry->dix    = dix;
                __entry->bix    = bix;
                __entry->tail   = tail;),

            TP_printk("intr(%d):: STALE HSI_BUF, hix=%d, ssix=%d, dix=%d, bix=%d, tail=%d", \
                             __entry->cpu, __entry->hix, __entry->ssix, __entry->dix, __entry->bix, __entry->tail));


TRACE_EVENT(ml_hsi_bad_state,
            TP_PROTO(uint32_t cpu, uint32_t hix, uint32_t ssix, uint32_t dix, uint32_t cur_state, uint32_t exp_state, uint32_t is_vcam),

            TP_ARGS(cpu, hix, ssix, dix, cur_state, exp_state, is_vcam),

            TP_STRUCT__entry(
                __field(uint32_t, cpu)
                __field(uint32_t, hix)
                __field(uint32_t, ssix)
                __field(uint32_t, dix)
                __field(uint32_t, cur_state)
                __field(uint32_t, exp_state)
                __field(uint32_t, is_vcam)),

            TP_fast_assign(
                __entry->cpu       = cpu;
                __entry->hix       = hix;
                __entry->ssix      = ssix;
                __entry->dix       = dix;
                __entry->cur_state = cur_state;
                __entry->exp_state = exp_state;
                __entry->is_vcam   = is_vcam;),

            TP_printk("intr(%d):: BAD STREAM STATE, hix=%d, ssix=%d, dix=%d, cur_state=%d, exp_state=%d, is_vcam=%d", \
                             __entry->cpu, __entry->hix, __entry->ssix, __entry->dix, __entry->cur_state, __entry->exp_state, __entry->is_vcam));


TRACE_EVENT(ml_hsi_intr_orphaned_hsi_buf,
            TP_PROTO(uint32_t cpu, uint32_t hix, uint32_t ssix, uint32_t dix, const char *dev_name, uint32_t bix, uint32_t dlen, uint32_t tail),

            TP_ARGS(cpu, hix, ssix, dix, dev_name, bix, dlen, tail),

            TP_STRUCT__entry(
                __field(uint32_t, cpu)
                __field(uint32_t, hix)
                __field(uint32_t, ssix)
                __field(uint32_t, dix)
                __array(char,     dev_name, HSI_TRACE_LABEL_LEN)
                __field(uint32_t, bix)
                __field(uint32_t, dlen)
                __field(uint32_t, tail)),

            TP_fast_assign(
                __entry->cpu    = cpu;
                __entry->hix    = hix;
                __entry->ssix   = ssix;
                __entry->dix    = dix;
                strncpy(__entry->dev_name, dev_name, HSI_TRACE_LABEL_LEN);
                __entry->bix    = bix;
                __entry->dlen   = dlen;
                __entry->tail   = tail;),

            TP_printk("intr(%d):: DATA FROM CLOSED STREAM, dev=%s, hix=%d, ssix=%d, dix=%d, bix=%d, len=%d, tail=%d", \
                             __entry->cpu, __entry->dev_name, __entry->hix, __entry->ssix, __entry->dix, __entry->bix, __entry->dlen, __entry->tail));


TRACE_EVENT(ml_hsi_ioctl_submit,
            TP_PROTO(uint32_t hix, uint32_t dix, const char *dev_name, uint64_t ref, uint32_t daddr, uint32_t maddr, void *hsip, uint32_t bix, void *vp, uint32_t qhead, uint32_t bufs_left),

            TP_ARGS(hix, dix, dev_name, ref, daddr, maddr, hsip, bix, vp, qhead, bufs_left),

            TP_STRUCT__entry(
                __field(uint32_t, hix)
                __field(uint32_t, dix)
                __field(const char *, dev_name)
                __field(uint64_t, ref)
                __field(uint32_t, daddr)
                __field(uint32_t, maddr)
                __field(void *,   hsip)
                __field(uint32_t, bix)
                __field(void *,   vp)
                __field(uint32_t, qhead)
                __field(uint32_t, bufs_left)),

            TP_fast_assign(
                __entry->hix      = hix;
                __entry->dix      = dix;
                __entry->dev_name = dev_name;
                __entry->ref      = ref;
                __entry->maddr    = maddr;
                __entry->daddr    = daddr;
                __entry->hsip     = hsip;
                __entry->bix      = bix;
                __entry->vp       = vp;
                __entry->qhead    = qhead;
                __entry->bufs_left= bufs_left;),

            TP_printk("hix=%x dix=%d (%s), bix=%d, ref=0x%llx, vp=0x%px, daddr=0x%08X, maddr=0x%08X, head=%d, bufs_left=%d",
                      __entry->hix, __entry->dix, __entry->dev_name, __entry->bix, __entry->ref, __entry->vp, __entry->daddr, __entry->maddr, __entry->qhead, __entry->bufs_left));


TRACE_EVENT(ml_hsi_ioctl_wake_event,
            TP_PROTO(uint32_t hix, uint32_t dix, const char *dev_name, uint32_t daddr, uint32_t slice, uint32_t size, uint64_t ref, void *ev, uint32_t evix),

            TP_ARGS(hix, dix, dev_name, daddr, slice, size, ref, ev, evix),

            TP_STRUCT__entry(
                __field(uint32_t, hix)
                __field(uint32_t, dix)
                __field(const char *, dev_name)
                __field(uint32_t, daddr)
                __field(uint32_t, slice)
                __field(uint32_t, size)
                __field(uint64_t, ref)
                __field(void  *,  ev)
                __field(uint32_t, evix)),

            TP_fast_assign(
                __entry->hix      = hix;
                __entry->dix      = dix;
                __entry->dev_name = dev_name;
                __entry->daddr    = daddr;
                __entry->slice    = slice;
                __entry->size     = size;
                __entry->ref      = ref;
                __entry->evix     = evix;
                __entry->ev       = ev;),

            TP_printk("hix=%x dix=%d (%s), daddr=0x%08X, slice=%d, size=%d, ref=0x%llx, ev=%px, evix=%d",
                      __entry->hix, __entry->dix, __entry->dev_name, __entry->daddr, __entry->slice, __entry->size, __entry->ref, __entry->ev, __entry->evix));


TRACE_EVENT(ml_hsi_ioctl_wait_event,
            TP_PROTO(uint32_t hix, uint32_t dix, const char *dev_name),

            TP_ARGS(hix, dix, dev_name),

            TP_STRUCT__entry(
                __field(uint32_t, hix)
                __field(uint32_t, dix)
                __field(const char *, dev_name)),

            TP_fast_assign(
                __entry->hix   = hix;
                __entry->dix   = dix;
                __entry->dev_name = dev_name;),

            TP_printk("hix=%d dix=%d (%s)",
                      __entry->hix, __entry->dix, __entry->dev_name));


TRACE_EVENT(ml_hsi_ioctl_wait_event_timeout,
            TP_PROTO(uint32_t hix, uint32_t dix),

            TP_ARGS(hix, dix),

            TP_STRUCT__entry(
                __field(uint32_t, hix)
                __field(uint32_t, dix)),

            TP_fast_assign(
                __entry->hix   = hix;
                __entry->dix   = dix;),

            TP_printk("hix=%d dix=%d",
                      __entry->hix, __entry->dix));


TRACE_EVENT(ml_hsi_hsibuf_alloc,
            TP_PROTO(void *bufp, uint32_t hix, uint32_t bix, uint32_t active, const char *dev_name, uint64_t ref),

            TP_ARGS(bufp, hix, bix, active, dev_name, ref),

            TP_STRUCT__entry(
                __field(void *,   bufp)
                __field(uint32_t, hix)
                __field(uint32_t, bix)
                __field(uint32_t, active)
                __field(const char *, dev_name)
                __field(uint64_t, ref)),

            TP_fast_assign(
                __entry->bufp  = bufp;
                __entry->hix   = hix;
                __entry->bix   = bix;
                __entry->active= active;
                __entry->dev_name= dev_name;
                __entry->ref   = ref;),

            TP_printk("hix=%d bufp=%px, bix=%d, active=%d, dev=(%s), ref=0x%llx",
                      __entry->hix, __entry->bufp, __entry->bix, __entry->active, __entry->dev_name, __entry->ref));


TRACE_EVENT(ml_hsi_hsibuf_free,
            TP_PROTO(void *bufp, uint32_t hix, uint32_t bix, uint32_t active, const char *dev_name, uint64_t ref, void *vp, uint32_t cxt),

            TP_ARGS(bufp, hix, bix, active, dev_name, ref, vp, cxt),

            TP_STRUCT__entry(
                __field(void *,   bufp)
                __field(uint32_t, hix)
                __field(uint32_t, bix)
                __field(uint32_t, active)
                __field(const char *, dev_name)
                __field(uint64_t, ref)
                __field(void *,   vp)
                __field(uint32_t, cxt)),

            TP_fast_assign(
                __entry->bufp  = bufp;
                __entry->hix   = hix;
                __entry->bix   = bix;
                __entry->active= active;
                __entry->dev_name= dev_name;
                __entry->ref   = ref;
                __entry->vp    = vp;
                __entry->cxt   = cxt;),

            TP_printk("hix=%d bufp=%px, bix=%d, active=%d, dev=(%s), ref=0x%llx, vp=0x%px, cxt=%d",
                      __entry->hix, __entry->bufp, __entry->bix, __entry->active, __entry->dev_name, __entry->ref, __entry->vp, __entry->cxt));


TRACE_EVENT(ml_hsi_open,
            TP_PROTO(const char *dev_name, uint32_t hix, uint32_t dix, char *app_name),

            TP_ARGS(dev_name, hix, dix, app_name),

            TP_STRUCT__entry(
                __array(char, dev_name, HSI_TRACE_LABEL_LEN)
                __field(uint32_t, hix)
                __field(uint32_t, dix)
                __array(char, app_name, HSI_TRACE_LABEL_LEN)),

            TP_fast_assign(
                strncpy(__entry->dev_name, dev_name, HSI_TRACE_LABEL_LEN);
                __entry->hix   = hix;
                __entry->dix   = dix;
                strncpy(__entry->app_name, app_name, HSI_TRACE_LABEL_LEN);),

            TP_printk("open:: dev=%s, hix=%d dix=%d, app=%s",
                      __entry->dev_name, __entry->hix, __entry->dix, __entry->app_name));


TRACE_EVENT(ml_hsi_close,
            TP_PROTO(const char *dev_name, uint32_t hix, uint32_t dix),

            TP_ARGS(dev_name, hix, dix),

            TP_STRUCT__entry(
                __array(char, dev_name, HSI_TRACE_LABEL_LEN)
                __field(uint32_t, hix)
                __field(uint32_t, dix)),

            TP_fast_assign(
                strncpy(__entry->dev_name, dev_name, HSI_TRACE_LABEL_LEN);
                __entry->hix   = hix;
                __entry->dix   = dix;),

            TP_printk("close:: dev=%s, hix=%d dix=%d",
                      __entry->dev_name, __entry->hix, __entry->dix));


TRACE_EVENT(ml_hsi_intr_dma_err,
            TP_PROTO(uint32_t hix, uint32_t dix, uint32_t xstat, uint32_t tail, uint32_t bufix),

            TP_ARGS(hix, dix, xstat, tail, bufix),

            TP_STRUCT__entry(
                __field(uint32_t, hix)
                __field(uint32_t, dix)
                __field(uint32_t, xstat)
                __field(uint32_t, tail)
                __field(uint32_t, bufix)),

            TP_fast_assign(
                __entry->hix    = hix;
                __entry->dix    = dix;
                __entry->xstat  = xstat;
                __entry->tail   = tail;
                __entry->bufix  = bufix;),

            TP_printk("DMA error: hix=%d dix=%d, xstat=0x%x, tail=%d, bix=%d",
                      __entry->hix, __entry->dix, __entry->xstat, __entry->tail, __entry->bufix));


TRACE_EVENT(ml_hsi_ev_alloc_fail,
            TP_PROTO(uint32_t hix, uint32_t dix, uint64_t ref, uint32_t data_length, uint32_t xstat, uint16_t uid),

            TP_ARGS(hix, dix, ref, data_length, xstat, uid),

            TP_STRUCT__entry(
                __field(uint32_t, hix)
                __field(uint32_t, dix)
                __field(uint64_t, ref)
                __field(uint32_t, data_length)
                __field(uint32_t, xstat)
                __field(uint16_t, uid)),

            TP_fast_assign(
                __entry->hix          = hix;
                __entry->dix          = dix;
                __entry->ref          = ref;
                __entry->data_length  = data_length;
                __entry->uid          = uid;
                __entry->xstat        = xstat;),

            TP_printk("EV_ALLOC_FAIL:: hix=%d, dix=%d, ref=0x%llx, length=%d, xstat=0x%x, uid=0x%x, t=%s",\
             __entry->hix, __entry->dix, __entry->ref, __entry->data_length, __entry->xstat, __entry->uid, UID_DECODE(__entry->uid)));


DECLARE_EVENT_CLASS(ml_hsi_ev_free,
            TP_PROTO(uint32_t hix, uint32_t dix, void *ev),

            TP_ARGS(hix, dix, ev),

            TP_STRUCT__entry(
                __field(uint32_t, hix)
                __field(uint16_t, dix)
                __field(void *,   ev)),

            TP_fast_assign(
                __entry->hix = hix;
                __entry->dix = dix;
                __entry->ev  = ev;),

            TP_printk("ev_free:: hix=%d, dix=%d, ev=%px", __entry->hix, __entry->dix, __entry->ev));

DEFINE_EVENT(ml_hsi_ev_free, ml_hsi_ev_free_rel,
            TP_PROTO(uint32_t hix, uint32_t dix, void *ev),
            TP_ARGS(hix, dix, ev));
DEFINE_EVENT(ml_hsi_ev_free, ml_hsi_ev_free_int,
            TP_PROTO(uint32_t hix, uint32_t dix, void *ev),
            TP_ARGS(hix, dix, ev));


TRACE_EVENT(ml_hsi_print,
            TP_PROTO(uint32_t cpu, uint32_t ssix, char *str),

            TP_ARGS(cpu, ssix, str),

            TP_STRUCT__entry(
                __field(uint32_t, cpu)
                __field(uint32_t, ssix)
                __array(char, msg, HSI_TRACE_LABEL_LEN)),

            TP_fast_assign(
                __entry->cpu   = cpu;
                __entry->ssix  = ssix;
                strncpy(__entry->msg, str, HSI_TRACE_LABEL_LEN);),

            TP_printk("print(%d):: %s, ssix=%d", __entry->cpu, __entry->msg, __entry->ssix));


DECLARE_EVENT_CLASS(ml_hsi_utrace,
            TP_PROTO(const char *devname, uint64_t p0, uint64_t p1, uint64_t p2, uint64_t p3),

            TP_ARGS(devname, p0, p1, p2, p3),

            TP_STRUCT__entry(
                __array(char, name, HSI_TRACE_LABEL_LEN)
                __field(uint64_t, p0)
                __field(uint64_t, p1)
                __field(uint64_t, p2)
                __field(uint64_t, p3)),

            TP_fast_assign(
                strncpy(__entry->name, devname, HSI_TRACE_LABEL_LEN);
                __entry->p0 = p0;
                __entry->p1 = p1;
                __entry->p2 = p2;
                __entry->p3 = p3;),

            TP_printk("UTRACE [%s] (%lld, %lld, %lld, %lld) / (0x%llx, 0x%llx, 0x%llx, 0x%llx)",\
                              __entry->name,
                              __entry->p0, __entry->p1, __entry->p2, __entry->p3,          \
                              __entry->p0, __entry->p1, __entry->p2, __entry->p3));


DEFINE_EVENT(ml_hsi_utrace, ml_hsi_utrace0,
            TP_PROTO(const char *devname, uint64_t p0, uint64_t p1, uint64_t p2, uint64_t p3),
            TP_ARGS(devname, p0, p1, p2, p3));


DEFINE_EVENT(ml_hsi_utrace, ml_hsi_utrace1,
            TP_PROTO(const char *devname, uint64_t p0, uint64_t p1, uint64_t p2, uint64_t p3),
            TP_ARGS(devname, p0, p1, p2, p3));


DEFINE_EVENT(ml_hsi_utrace, ml_hsi_utrace2,
            TP_PROTO(const char *devname, uint64_t p0, uint64_t p1, uint64_t p2, uint64_t p3),
            TP_ARGS(devname, p0, p1, p2, p3));


DEFINE_EVENT(ml_hsi_utrace, ml_hsi_utrace3,
            TP_PROTO(const char *devname, uint64_t p0, uint64_t p1, uint64_t p2, uint64_t p3),
            TP_ARGS(devname, p0, p1, p2, p3));


TRACE_EVENT(ml_hsi_bkpt_err_intr,
            TP_PROTO(uint32_t code, uint32_t status),

            TP_ARGS(code, status),

            TP_STRUCT__entry(
                __field(uint32_t, code)
                __field(uint32_t, status)),

            TP_fast_assign(
                __entry->code   = code;
                __entry->status = status;),

            TP_printk("BKPT intr error:: code=%d, status=%d",  __entry->code, __entry->status));


TRACE_EVENT(ml_hsi_vcam_nobufs,
            TP_PROTO(uint32_t p0, uint32_t p1),

            TP_ARGS(p0, p1),

            TP_STRUCT__entry(
                __field(uint32_t, p0)
                __field(uint32_t, p1)),

            TP_fast_assign(
                __entry->p0 = p0;
                __entry->p1 = p1;),

            TP_printk("No VCAM bufs:: p0=%d, p1=%d",  __entry->p0, __entry->p1));


DECLARE_EVENT_CLASS(ml_hsi_vcam_buf,
            TP_PROTO(void *vp, void *buf_g, void *buf_rb, void *buf_dst),

            TP_ARGS(vp, buf_g, buf_rb, buf_dst),

            TP_STRUCT__entry(
                __field(void *, vp)
                __field(void *, buf_g)
                __field(void *, buf_rb)
                __field(void *, buf_dst)),

            TP_fast_assign(
                __entry->vp      = vp;
                __entry->buf_g   = buf_g;
                __entry->buf_rb  = buf_rb;
                __entry->buf_dst = buf_dst;),

            TP_printk("vp=%px, buf_g=%px, buf_rb=%px, dst=%px",  __entry->vp, __entry->buf_g, __entry->buf_rb, __entry->buf_dst));

DEFINE_EVENT(ml_hsi_vcam_buf, ml_hsi_vcam_buf_alloc,
            TP_PROTO(void *vp, void *buf_g, void *buf_rb, void *buf_dst),
            TP_ARGS(vp, buf_g, buf_rb, buf_dst));

DEFINE_EVENT(ml_hsi_vcam_buf, ml_hsi_vcam_buf_free,
            TP_PROTO(void *vp, void *buf_g, void *buf_rb, void *buf_dst),
            TP_ARGS(vp, buf_g, buf_rb, buf_dst));

DEFINE_EVENT(ml_hsi_vcam_buf, ml_hsi_vcam_buf_free_all,
            TP_PROTO(void *vp, void *buf_g, void *buf_rb, void *buf_dst),
            TP_ARGS(vp, buf_g, buf_rb, buf_dst));
DEFINE_EVENT(ml_hsi_vcam_buf, ml_hsi_vcam_buf_free_in_use,
            TP_PROTO(void *vp, void *buf_g, void *buf_rb, void *buf_dst),
            TP_ARGS(vp, buf_g, buf_rb, buf_dst));
#endif

#undef TRACE_INCLUDE_PATH
#define TRACE_INCLUDE_PATH .
#undef TRACE_INCLUDE_FILE
#define TRACE_INCLUDE_FILE hsi_trace
#include <trace/define_trace.h>
