/* SPDX-License-Identifier: GPL-2.0
 * Copyright (c) 2020, Magic Leap, Inc. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#undef TRACE_SYSTEM
#define TRACE_SYSTEM fan_mlmux

#if !defined(_TRACE_FAN_MLMUX_H) || defined(TRACE_HEADER_MULTI_READ)
#define _TRACE_FAN_MLMUX_H

#include <linux/tracepoint.h>

DECLARE_EVENT_CLASS(fan_mlmux_status,
	TP_PROTO(uint8_t id, uint16_t rpm, bool dir),
	TP_ARGS(id, rpm, dir),

	TP_STRUCT__entry(
		__field(uint8_t, id)
		__field(uint16_t, rpm)
		__field(bool, dir)
	),

	TP_fast_assign(
		__entry->id = id;
		__entry->rpm = rpm;
		__entry->dir = dir;
	),

	TP_printk("%u rpm:%u dir:%s",
		  __entry->id,
		  __entry->rpm,
		  __entry->dir ? "REVERSE" : "FORWARD"
	)
);

DEFINE_EVENT(fan_mlmux_status, fan_mlmux_status,
	TP_PROTO(uint8_t id, uint16_t rpm, bool dir),
	TP_ARGS(id, rpm, dir)
);

DECLARE_EVENT_CLASS(fan_mlmux_set,
	TP_PROTO(uint8_t id, uint16_t rpm, unsigned long state),
	TP_ARGS(id, rpm, state),

	TP_STRUCT__entry(
		__field(uint8_t, id)
		__field(uint16_t, rpm)
		__field(unsigned long, state)
	),

	TP_fast_assign(
		__entry->id = id;
		__entry->rpm = rpm;
		__entry->state = state;
	),

	TP_printk("%u rpm:%u state:%lu",
		  __entry->id,
		  __entry->rpm,
		  __entry->state
	)
);

DEFINE_EVENT(fan_mlmux_set, fan_mlmux_set,
	TP_PROTO(uint8_t id, uint16_t rpm, unsigned long state),
	TP_ARGS(id, rpm, state)
);

#endif /* _TRACE_FAN_MLMUX_H */

#undef TRACE_INCLUDE_PATH
#define TRACE_INCLUDE_PATH .

#undef TRACE_INCLUDE_FILE
#define TRACE_INCLUDE_FILE fan-mlmux-trace
#include <trace/define_trace.h>
