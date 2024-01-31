/* SPDX-License-Identifier: GPL-2.0-only
 *
 * \file cvcore_trace.h
 *
 * \brief CVCore trace definitions.
 *
 * Copyright (c) 2019-2022, Magic Leap, Inc. All rights reserved.
 */

#pragma once

#include <linux/types.h>

#include "gsm.h"
#include "cvcore_trace_easy.h"

#define CVCORE_TRACE_MAX_BUFFER_SIZE (4 * 4096)
#define CVCORE_DSP_DATA_REGION1_VADDR (0x00C00000U)
#define CVCORE_DSP_DATA_REGION1_SIZE (0xC3400000U)

#define CVCORE_TRACE_BUFFER_TOTAL_SIZE                                         \
	(GSM_MAX_NUM_CVCORE_TRACERS * CVCORE_TRACE_MAX_BUFFER_SIZE)
#define CVCORE_TRACE_BUFFER_END                                                \
	(CVCORE_DSP_DATA_REGION1_VADDR + CVCORE_DSP_DATA_REGION1_SIZE)
#define CVCORE_TRACE_BUFFER_DSP_VADDR                                          \
	(CVCORE_TRACE_BUFFER_END - CVCORE_TRACE_BUFFER_TOTAL_SIZE)

struct cvcore_trace_descriptor {
	uint64_t trace_unique_id; // 8
	uint32_t trace_cnt; // 4
	uint16_t event_size; // 2
	uint16_t max_events; // 2
	// 16
};

struct cvcore_trace_ctx {
	uint64_t idx_ctx;
	uintptr_t trace_buffer_vaddr;
	uint32_t event_size;
	uint32_t max_events;
	uint16_t trace_idx;
	uint8_t is_early_trace;
};

int cvcore_trace_attach(uint64_t unique_id, struct cvcore_trace_ctx* ctx);
int cvcore_trace_reset(struct cvcore_trace_ctx* ctx);
int cvcore_trace_add_event(const struct cvcore_trace_ctx* ctx,
	const uint8_t* data);
int cvcore_trace_get_events(const struct cvcore_trace_ctx* ctx, uint8_t* buf,
	uint32_t buf_size, uint32_t* num_events);
int cvcore_trace_pause(const struct cvcore_trace_ctx* ctx, const uint8_t* data);
int cvcore_trace_resume(const struct cvcore_trace_ctx* ctx);
int cvcore_trace_create(uint64_t unique_id, uint32_t event_size,
	uint32_t max_events);
int cvcore_trace_destroy(uint64_t unique_id);

#define cvcore_easy_tracepoint_add_event(tp_name,...) \
    __cvcore_easy_tracepoint_add_event(tp_name,##__VA_ARGS__)

#define cvcore_easy_tracepoint_add() \
    __cvcore_easy_tracepoint_add()

#define cvcore_easy_tracer_pause() \
    __cvcore_easy_tracer_pause()

#define cvcore_easy_tracer_pause_event(tp_name,...) \
    __cvcore_easy_tracer_pause_event(tp_name,##__VA_ARGS__)

#define cvcore_easy_tracer_resume() \
    __cvcore_easy_tracer_resume()

#define cvcore_easy_tracepoint_begin() \
    __cvcore_easy_tracepoint_begin()

#define cvcore_easy_tracepoint_begin_event(tp_name,...)  \
    __cvcore_easy_tracepoint_begin_event(tp_name,##__VA_ARGS__)

#define cvcore_easy_tracepoint_end() \
    __cvcore_easy_tracepoint_end()

#define cvcore_easy_tracepoint_end_event(tp_name,...) \
    __cvcore_easy_tracepoint_end_event(tp_name,##__VA_ARGS__)
