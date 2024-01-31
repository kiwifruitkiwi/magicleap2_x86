/*
 * Copyright (C) 2018-2021 Advanced Micro Devices, Inc. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors: AMD
 *
 */

#undef TRACE_SYSTEM
#define TRACE_SYSTEM amdgpu_dm

#if !defined(_AMDGPU_DM_TRACE_H) || defined(TRACE_HEADER_MULTI_READ)
#define _AMDGPU_DM_TRACE_H_

#include <linux/tracepoint.h>

TRACE_EVENT(amdgpu_dc_rreg,
	TP_PROTO(unsigned long *read_count, uint32_t reg, uint32_t value),
	TP_ARGS(read_count, reg, value),
	TP_STRUCT__entry(
			__field(uint32_t, reg)
			__field(uint32_t, value)
		),
	TP_fast_assign(
			__entry->reg = reg;
			__entry->value = value;
			*read_count = *read_count + 1;
		),
	TP_printk("reg=0x%08lx, value=0x%08lx",
			(unsigned long)__entry->reg,
			(unsigned long)__entry->value)
);

TRACE_EVENT(amdgpu_dc_wreg,
	TP_PROTO(unsigned long *write_count, uint32_t reg, uint32_t value),
	TP_ARGS(write_count, reg, value),
	TP_STRUCT__entry(
			__field(uint32_t, reg)
			__field(uint32_t, value)
		),
	TP_fast_assign(
			__entry->reg = reg;
			__entry->value = value;
			*write_count = *write_count + 1;
		),
	TP_printk("reg=0x%08lx, value=0x%08lx",
			(unsigned long)__entry->reg,
			(unsigned long)__entry->value)
);


TRACE_EVENT(amdgpu_dc_performance,
	TP_PROTO(unsigned long read_count, unsigned long write_count,
		unsigned long *last_read, unsigned long *last_write,
		const char *func, unsigned int line),
	TP_ARGS(read_count, write_count, last_read, last_write, func, line),
	TP_STRUCT__entry(
			__field(uint32_t, reads)
			__field(uint32_t, writes)
			__field(uint32_t, read_delta)
			__field(uint32_t, write_delta)
			__string(func, func)
			__field(uint32_t, line)
			),
	TP_fast_assign(
			__entry->reads = read_count;
			__entry->writes = write_count;
			__entry->read_delta = read_count - *last_read;
			__entry->write_delta = write_count - *last_write;
			__assign_str(func, func);
			__entry->line = line;
			*last_read = read_count;
			*last_write = write_count;
			),
	TP_printk("%s:%d reads=%08ld (%08ld total), writes=%08ld (%08ld total)",
			__get_str(func), __entry->line,
			(unsigned long)__entry->read_delta,
			(unsigned long)__entry->reads,
			(unsigned long)__entry->write_delta,
			(unsigned long)__entry->writes)
);

TRACE_EVENT(amdgpu_dark_mode_enter_scheduled,
	TP_PROTO(int crtc),
	TP_ARGS(crtc),
	TP_STRUCT__entry(
		__field(int, crtc)
		),
	TP_fast_assign(
		__entry->crtc = crtc;
	),
	TP_printk("crtc=%d", __entry->crtc)
);

TRACE_EVENT(amdgpu_dark_mode_enter_start,
	TP_PROTO(int crtc),
	TP_ARGS(crtc),
	TP_STRUCT__entry(
		__field(int, crtc)
		),
	TP_fast_assign(
		__entry->crtc = crtc;
	),
	TP_printk("crtc=%d", __entry->crtc)
);

TRACE_EVENT(amdgpu_dark_mode_enter_done,
	TP_PROTO(int crtc),
	TP_ARGS(crtc),
	TP_STRUCT__entry(
		__field(int, crtc)
		),
	TP_fast_assign(
		__entry->crtc = crtc;
	),
	TP_printk("crtc=%d", __entry->crtc)
);

TRACE_EVENT(amdgpu_dark_mode_exit_start,
	TP_PROTO(int crtc),
	TP_ARGS(crtc),
	TP_STRUCT__entry(
		__field(int, crtc)
		),
	TP_fast_assign(
		__entry->crtc = crtc;
	),
	TP_printk("crtc=%d", __entry->crtc)
);

TRACE_EVENT(amdgpu_dark_mode_exit_done,
	TP_PROTO(int crtc),
	TP_ARGS(crtc),
	TP_STRUCT__entry(
		__field(int, crtc)
		),
	TP_fast_assign(
		__entry->crtc = crtc;
	),
	TP_printk("crtc=%d", __entry->crtc)
);

TRACE_EVENT(amdgpu_dark_mode_high_irq,
	TP_PROTO(int crtc),
	TP_ARGS(crtc),
	TP_STRUCT__entry(
		__field(int, crtc)
		),
	TP_fast_assign(
		__entry->crtc = crtc;
	),
	TP_printk("crtc=%d", __entry->crtc)
);

TRACE_EVENT(amdgpu_dmub_trace_high_irq,
	TP_PROTO(uint32_t trace_code, uint32_t tick_count, uint32_t param0,
		 uint32_t param1),
	TP_ARGS(trace_code, tick_count, param0, param1),
	TP_STRUCT__entry(
		__field(uint32_t, trace_code)
		__field(uint32_t, tick_count)
		__field(uint32_t, param0)
		__field(uint32_t, param1)
		),
	TP_fast_assign(
		__entry->trace_code = trace_code;
		__entry->tick_count = tick_count;
		__entry->param0 = param0;
		__entry->param1 = param1;
	),
	TP_printk("trace_code=%u tick_count=%u param0=0x%x param1=0x%x",
		  __entry->trace_code, __entry->tick_count,
		  __entry->param0, __entry->param1)
);

TRACE_EVENT(amdgpu_dark_mode_txcntl_start,
	TP_PROTO(int crtc),
	TP_ARGS(crtc),
	TP_STRUCT__entry(
		__field(int, crtc)
		),
	TP_fast_assign(
		__entry->crtc = crtc;
	),
	TP_printk("stream=%d", __entry->crtc)
);

TRACE_EVENT(amdgpu_dark_mode_txcntl_end,
	TP_PROTO(int crtc),
	TP_ARGS(crtc),
	TP_STRUCT__entry(
		__field(int, crtc)
		),
	TP_fast_assign(
		__entry->crtc = crtc;
	),
	TP_printk("stream=%d", __entry->crtc)
);

TRACE_EVENT(amdgpu_crtc_high_irq,
	TP_PROTO(int crtc),
	TP_ARGS(crtc),
	TP_STRUCT__entry(
		__field(int, crtc)
		),
	TP_fast_assign(
		__entry->crtc = crtc;
	),
	TP_printk("crtc=%d", __entry->crtc)
);

TRACE_EVENT(amdgpu_vupdate_high_irq,
	TP_PROTO(int crtc),
	TP_ARGS(crtc),
	TP_STRUCT__entry(
		__field(int, crtc)
		),
	TP_fast_assign(
		__entry->crtc = crtc;
	),
	TP_printk("crtc=%d", __entry->crtc)
);

TRACE_EVENT(amdgpu_pflip_high_irq,
	TP_PROTO(int crtc),
	TP_ARGS(crtc),
	TP_STRUCT__entry(
		__field(int, crtc)
		),
	TP_fast_assign(
		__entry->crtc = crtc;
	),
	TP_printk("crtc=%d", __entry->crtc)
);

TRACE_EVENT(amdgpu_pipe_lock,
	TP_PROTO(int top_pipe, int hubp_inst, int otg_inst, bool lock),
	TP_ARGS(top_pipe, hubp_inst, otg_inst, lock),
	TP_STRUCT__entry(
		__field(int, top_pipe)
		__field(int, hubp_inst)
		__field(int, otg_inst)
		__field(bool, lock)
		),
	TP_fast_assign(
		__entry->top_pipe = top_pipe;
		__entry->hubp_inst = hubp_inst;
		__entry->otg_inst = otg_inst;
		__entry->lock = lock;
	),
	TP_printk("top_pipe=%d hubp=%d otg=%d is_locking=%d",
		  __entry->top_pipe, __entry->hubp_inst, __entry->otg_inst,
		  __entry->lock)
);

TRACE_EVENT(amdgpu_pdt_programming,
	TP_PROTO(int otg_index, int num_active_lines),
	TP_ARGS(otg_index, num_active_lines),
	TP_STRUCT__entry(
		__field(int, otg_index)
		__field(int, num_active_lines)
		),
	TP_fast_assign(
		__entry->otg_index = otg_index;
		__entry->num_active_lines = num_active_lines;
	),
	TP_printk("otg=%d num_active_lines=%d", __entry->otg_index,
		  __entry->num_active_lines)
);

TRACE_EVENT(amdgpu_dark_mode_irq_programming,
	TP_PROTO(int otg_index, int start_line),
	TP_ARGS(otg_index, start_line),
	TP_STRUCT__entry(
		__field(int, otg_index)
		__field(int, start_line)
		),
	TP_fast_assign(
		__entry->otg_index = otg_index;
		__entry->start_line = start_line;
	),
	TP_printk("otg=%d start_line=%d", __entry->otg_index,
		  __entry->start_line)
);

TRACE_EVENT(amdgpu_dmdata_programming,
	TP_PROTO(int hubp_inst, int64_t dmdata_addr, int dmdata_size,
		 int dmdata_repeat, int dmdata_mode),
	TP_ARGS(hubp_inst, dmdata_addr, dmdata_size, dmdata_repeat, dmdata_mode),
	TP_STRUCT__entry(
		__field(int, hubp_inst)
		__field(int64_t, dmdata_addr)
		__field(int, dmdata_size)
		__field(int, dmdata_repeat)
		__field(int, dmdata_mode)
		),
	TP_fast_assign(
		__entry->hubp_inst = hubp_inst;
		__entry->dmdata_addr = dmdata_addr;
		__entry->dmdata_size = dmdata_size;
		__entry->dmdata_repeat = dmdata_repeat;
		__entry->dmdata_mode = dmdata_mode;
	),
	TP_printk("hubp=%d addr=%llx size=%d repeat=%d mode=%d",
		  __entry->hubp_inst, __entry->dmdata_addr,
		  __entry->dmdata_size, __entry->dmdata_repeat,
		  __entry->dmdata_mode)
);

TRACE_EVENT(amdgpu_grph_addr_programming,
	TP_PROTO(int hubp_inst, int64_t grph_addr),
	TP_ARGS(hubp_inst, grph_addr),
	TP_STRUCT__entry(
		__field(int, hubp_inst)
		__field(int64_t, grph_addr)
		),
	TP_fast_assign(
		__entry->hubp_inst = hubp_inst;
		__entry->grph_addr = grph_addr;
	),
	TP_printk("hubp=%d grph_addr=%llx", __entry->hubp_inst,
		  __entry->grph_addr)
);

TRACE_EVENT(dm_dp_link_training,
	TP_PROTO(const char *reason),
	TP_ARGS(reason),
	TP_STRUCT__entry(
		__string(reason, reason)
	),
	TP_fast_assign(
		__assign_str(reason, reason);
	),
	TP_printk("amdgpu dp link training %s", __get_str(reason))
);

TRACE_EVENT(dm_resume,
	TP_PROTO(bool in),
	TP_ARGS(in),
	TP_STRUCT__entry(
		__field(bool, in)
	),
	TP_fast_assign(
		__entry->in = in;
	),
	TP_printk("amdgpu dm resume %5s", __entry->in ? "entry" : "end")
);

TRACE_EVENT(dcn_fpu,
	    TP_PROTO(bool begin, const char *function, const int line, const int ref_count),
	    TP_ARGS(begin, function, line, ref_count),

	    TP_STRUCT__entry(
			     __field(bool, begin)
			     __field(const char *, function)
			     __field(int, line)
			     __field(int, ref_count)
	    ),
	    TP_fast_assign(
			   __entry->begin = begin;
			   __entry->function = function;
			   __entry->line = line;
			   __entry->ref_count = ref_count;
	    ),
	    TP_printk("%s: ref_count: %d: %s()+%d:",
		      __entry->begin ? "begin" : "end",
		      __entry->ref_count,
		      __entry->function,
		      __entry->line
	    )
);

#endif /* _AMDGPU_DM_TRACE_H_ */

#undef TRACE_INCLUDE_PATH
#define TRACE_INCLUDE_PATH .
#define TRACE_INCLUDE_FILE amdgpu_dm_trace
#include <trace/define_trace.h>
