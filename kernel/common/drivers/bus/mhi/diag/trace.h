/* SPDX-License-Identifier: BSD-3-Clause-Clear */
/**
 * Copyright (c) 2020 The Linux Foundation. All rights reserved.
 */

#if !defined(_TRACE_H_) || defined(TRACE_HEADER_MULTI_READ)

#include <linux/tracepoint.h>
//#include "../core.h"

#define _TRACE_H_

#undef TRACE_SYSTEM
#define TRACE_SYSTEM ath11k

TRACE_EVENT(ath11k_diag_fwlog,
	    TP_PROTO(struct device *dev, const void *data, size_t len),

	TP_ARGS(dev, data, len),

	TP_STRUCT__entry(
		__string(device, dev_name(dev))
		__string(driver, dev_driver_string(dev))
		__field(u16, len)
		__dynamic_array(u8, fwlog, len)
	),

	TP_fast_assign(
		__assign_str(device, dev_name(dev));
		__assign_str(driver, dev_driver_string(dev));
		__entry->len = len;
		memcpy(__get_dynamic_array(fwlog), data, len);
	),

	TP_printk(
		"%s %s fwlog len %d",
		__get_str(driver),
		__get_str(device),
		__entry->len
	)
);

#endif /* _TRACE_H_ || TRACE_HEADER_MULTI_READ*/

/* we don't want to use include/trace/events */
#undef TRACE_INCLUDE_PATH
#define TRACE_INCLUDE_PATH .
#undef TRACE_INCLUDE_FILE
#define TRACE_INCLUDE_FILE trace

/* This part must be outside protection */
#include <trace/define_trace.h>
