/* SPDX-License-Identifier: GPL-2.0-only
 *
 * Copyright (c) 2022, Magic Leap, Inc. All rights reserved.
 */

#undef TRACE_SYSTEM
#define TRACE_SYSTEM shregion

#if !defined(_SHREGION_TRACE_H) || defined(TRACE_HEADER_MULTI_READ)
#define _SHREGION_TRACE_H
#include <linux/tracepoint.h>

DECLARE_EVENT_CLASS(meta_template,

	TP_PROTO(struct shregion_metadata_drv *meta_drv, int retval),

	TP_ARGS(meta_drv, retval),

	TP_STRUCT__entry(
		__field(        uint64_t,  name                 )
		__field(        uint32_t,  size                 )
		__field(        uint32_t,  flags                )
		__field(        uint8_t,   type                 )
		__field(        uint64_t,  phys_addr            )
		__field(        int,       retval               )
	),

	TP_fast_assign(
		__entry->name           = meta_drv->meta.name;
		__entry->size           = meta_drv->meta.size_bytes;
		__entry->flags          = meta_drv->meta.flags;
		__entry->type           = meta_drv->meta.type;
		__entry->phys_addr      = meta_drv->phys_addr;
		__entry->retval      	= retval;
	),

	TP_printk("name=0x%llx size=%u flags=0x%x type=0x%x phys=0x%llx ret=%d",
				  __entry->name, __entry->size, __entry->flags,
				  __entry->type, __entry->phys_addr, __entry->retval)
);

DEFINE_EVENT(meta_template, register,
	TP_PROTO(struct shregion_metadata_drv *meta_drv, int retval),
	TP_ARGS(meta_drv, retval));

DEFINE_EVENT(meta_template, create,
	TP_PROTO(struct shregion_metadata_drv *meta_drv, int retval),
	TP_ARGS(meta_drv, retval));

DEFINE_EVENT(meta_template, query,
	TP_PROTO(struct shregion_metadata_drv *meta_drv, int retval),
	TP_ARGS(meta_drv, retval));

DEFINE_EVENT(meta_template, get_phys_addr,
	TP_PROTO(struct shregion_metadata_drv *meta_drv, int retval),
	TP_ARGS(meta_drv, retval));

DEFINE_EVENT(meta_template, register_cdp_dcprs,
	TP_PROTO(struct shregion_metadata_drv *meta_drv, int retval),
	TP_ARGS(meta_drv, retval));

DEFINE_EVENT(meta_template, free,
	TP_PROTO(struct shregion_metadata_drv *meta_drv, int retval),
	TP_ARGS(meta_drv, retval));

DEFINE_EVENT(meta_template, get_metadata,
	TP_PROTO(struct shregion_metadata_drv *meta_drv, int retval),
	TP_ARGS(meta_drv, retval));


TRACE_EVENT(dsp_metadata_update,

	TP_PROTO(struct dsp_shregion_metadata_drv *meta_drv, int retval),

	TP_ARGS(meta_drv, retval),

	TP_STRUCT__entry(
		__field(	uint64_t,	name	)
		__field(	uint32_t,	size	)
		__field(	int,		retval	)
	),

	TP_fast_assign(
		__entry->name = meta_drv->shr_name;
		__entry->size = meta_drv->shr_size_bytes;
		__entry->retval = retval;
	),

	TP_printk("name=0x%llx size=%u ret=%d",
		__entry->name, __entry->size, __entry->retval)
);

TRACE_EVENT(get_dsp_vaddr,

	TP_PROTO(struct dsp_shregion_vaddr_meta *meta_drv, int retval),

	TP_ARGS(meta_drv, retval),

	TP_STRUCT__entry(
		__field(	uint64_t,	name	)
		__field(	uint64_t,	addr	)
		__field(	int,		retval	)
	),

	TP_fast_assign(
		__entry->name = meta_drv->shr_name;
		__entry->addr = meta_drv->shr_dsp_vaddr;
		__entry->retval = retval;
	),

	TP_printk("name=0x%llx addr=0x%llx ret=%d",
		__entry->name, __entry->addr, __entry->retval)
);

TRACE_EVENT(get_shr_cdp_dcprs_ioaddr,

	TP_PROTO(struct cdp_dcprs_shr_ioaddr_meta *meta_drv, int retval),

	TP_ARGS(meta_drv, retval),

	TP_STRUCT__entry(
		__field(	uint64_t,	name	)
		__field(	uint64_t,	addr	)
		__field(	int,		retval	)
	),

	TP_fast_assign(
		__entry->name = meta_drv->shr_name;
		__entry->addr = meta_drv->shr_cdp_dcprs_ioaddr;
		__entry->retval = retval;
	),

	TP_printk("name=0x%llx addr=0x%llx ret=%d",
		__entry->name, __entry->addr, __entry->retval)
);

TRACE_EVENT(register_sid_meta,

	TP_PROTO(unsigned long arg, int retval),

	TP_ARGS(arg, retval),

	TP_STRUCT__entry(
		__field(	unsigned long,	arg	)
		__field(	int,		retval	)
	),

	TP_fast_assign(
		__entry->arg = arg;
		__entry->retval = retval;
	),

	TP_printk("arg=0x%lx ret=%d", __entry->arg, __entry->retval)
);

TRACE_EVENT(slc_sync,

	TP_PROTO(struct shr_slc_sync_meta *meta_drv, int retval),

	TP_ARGS(meta_drv, retval),

	TP_STRUCT__entry(
		__field(	uint64_t,	name	)
		__field(	uint64_t,	addr	)
		__field(	uint32_t,	size	)
		__field(	int,		retval	)
	),

	TP_fast_assign(
		__entry->name = meta_drv->name;
		__entry->addr = meta_drv->start_addr;
		__entry->size = meta_drv->size_bytes;
		__entry->retval = retval;
	),

	TP_printk("name=0x%llx addr=0x%llx size=%u ret=%d",
		__entry->name, __entry->addr, __entry->size, __entry->retval)
);

TRACE_EVENT(deregister_sid,

	TP_PROTO(pid_t pid, int retval),

	TP_ARGS(pid, retval),

	TP_STRUCT__entry(
		__field(	pid_t,		pid	)
		__field(	int,		retval	)
	),

	TP_fast_assign(
		__entry->pid = pid;
		__entry->retval = retval;
	),

	TP_printk("pid=%d ret=%d", __entry->pid, __entry->retval)
);

#endif
#undef TRACE_INCLUDE_PATH
#define TRACE_INCLUDE_PATH ../include/
#define TRACE_INCLUDE_FILE shregion_trace
#include <trace/define_trace.h>

#ifndef __KERNEL__
#error "Can't include kernel header in userspace"
#endif
