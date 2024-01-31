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

#define kEasyTraceValueDontCare     ((u32)(-1))

enum cvcore_easy_trace_flags {
    kFlagsNone = 0,
    kFlagIsBeginDelimiter,
    kFlagIsBeginNoLogDelimiter,
    kFlagIsEndDelimiter,
    kFlagIsEndNoLogDelimiter,
    kFlagIsAnonEvent,
    kFlagIsPauseNoLog
};

int cvcore_easy_trace_initialize(void);
int cvcore_easy_trace_pause(uint64_t tp_name, uint32_t tp_value1, uint32_t tp_value2,
    uint32_t tp_value3, uint32_t tp_value4,
    enum cvcore_easy_trace_flags tp_flags);
int cvcore_easy_trace_resume(void);
int cvcore_easy_trace_add(uint64_t tp_name, uint32_t tp_value1, uint32_t tp_value2,
    uint32_t tp_value3, uint32_t tp_value4,
    enum cvcore_easy_trace_flags tp_flags);

#define func_cat_helper(x, y) x##y
#define func_cat(x, y) func_cat_helper(x, y)

#define tp_zero_value(func, tp_name, flag)                                  \
    func_cat_helper(cvcore_easy_trace_,func)((tp_name),                     \
    (kEasyTraceValueDontCare),                                              \
    (kEasyTraceValueDontCare),                                              \
    kEasyTraceValueDontCare,                                                \
    kEasyTraceValueDontCare,                                                \
    (enum cvcore_easy_trace_flags)(flag))

#define tp_one_value(func, tp_name, flag, value1)                           \
    func_cat_helper(cvcore_easy_trace_,func)((tp_name),                     \
    (value1),                                                               \
    kEasyTraceValueDontCare,                                                \
    kEasyTraceValueDontCare,                                                \
    kEasyTraceValueDontCare,                                                \
    (enum cvcore_easy_trace_flags)(flag))

#define tp_two_value(func, tp_name, flag, value1, value2)                   \
    func_cat_helper(cvcore_easy_trace_,func)((tp_name),                     \
    (value1),                                                               \
    (value2),                                                               \
    kEasyTraceValueDontCare,                                                \
    kEasyTraceValueDontCare,                                                \
    (enum cvcore_easy_trace_flags)(flag))

#define tp_three_value(func, tp_name, flag, value1, value2, value3)         \
    func_cat_helper(cvcore_easy_trace_,func)((tp_name),                     \
    (value1),                                                               \
    (value2),                                                               \
    (value3),                                                               \
    kEasyTraceValueDontCare,                                                \
    (enum cvcore_easy_trace_flags)(flag))

#define tp_four_value(func, tp_name, flag, value1, value2, value3, value4)  \
    func_cat_helper(cvcore_easy_trace_,func)((tp_name),                     \
    (value1),                                                               \
    (value2),                                                               \
    (value3),                                                               \
    (value4),                                                               \
    (enum cvcore_easy_trace_flags)(flag))

#define trace_GET_MACRO(_0,_1,_2,_3,_4,_5,_6,NAME,...) NAME

#define cvcore_easy_tracepoint_add_event_generic(func,tp_name,flags, ...)   \
    trace_GET_MACRO(func, tp_name, flags, ##__VA_ARGS__, tp_four_value,     \
    tp_three_value, tp_two_value, tp_one_value,                             \
    tp_zero_value)(func, tp_name, flags, ##__VA_ARGS__)

#define __cvcore_easy_tracepoint_add_event(tp_name,...) \
    cvcore_easy_tracepoint_add_event_generic(add, tp_name, kFlagsNone, ##__VA_ARGS__)

#define __cvcore_easy_tracepoint_add() \
    tp_zero_value(add, 0x0, kFlagIsAnonEvent)

#define __cvcore_easy_tracer_pause() \
    tp_zero_value(pause, 0x0, kFlagIsPauseNoLog)

#define __cvcore_easy_tracer_pause_event(tp_name,...) \
    cvcore_easy_tracepoint_add_event_generic(pause, tp_name, kFlagsNone, ##__VA_ARGS__)

#define __cvcore_easy_tracer_resume() \
    cvcore_easy_trace_resume()

#define __cvcore_easy_tracepoint_begin() \
    tp_zero_value(add, 0x0, kFlagIsBeginNoLogDelimiter)

#define __cvcore_easy_tracepoint_begin_event(tp_name,...) \
    cvcore_easy_tracepoint_add_event_generic(add, tp_name, kFlagIsBeginDelimiter, ##__VA_ARGS__)

#define __cvcore_easy_tracepoint_end() \
    tp_zero_value(add, 0x0, kFlagIsEndNoLogDelimiter)

#define __cvcore_easy_tracepoint_end_event(tp_name,...) \
    cvcore_easy_tracepoint_add_event_generic(add, tp_name, kFlagIsEndDelimiter, ##__VA_ARGS__)
