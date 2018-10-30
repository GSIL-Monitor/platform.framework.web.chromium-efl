// Copyright 2017 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef _TIZEN_TTRACE_H_
#define _TIZEN_TTRACE_H_

#ifdef USE_TTRACE
#include <TTraceWrapper.h>

// Temporarily disable macros on TV
// because there are some performance issues on the TV
#if !defined(OS_TIZEN_TV_PRODUCT)
#define USE_TTRACE_TRACE_EVENT_SKIA
#define USE_TTRACE_TRACE_EVENT_V8
#define USE_TTRACE_TRACE_EVENT_WEBCORE
#endif

bool TraceEventTTraceBegin(uint64_t tag,
                           const char* category_group,
                           const char* name);

#define TTRACE_WEB_SCOPE(...) \
  TTraceWrapper ttrace##__LINE__(TTRACE_TAG_WEB, __VA_ARGS__)
#define TTRACE_WEB_MARK(...) traceMark(TTRACE_TAG_WEB, __VA_ARGS__)
#define TTRACE_WEB_ASYNC_BEGIN(uid, ...) \
  traceAsyncBegin(TTRACE_TAG_WEB, uid, __VA_ARGS__)
#define TTRACE_WEB_ASYNC_END(uid, ...) \
  traceAsyncEnd(TTRACE_TAG_WEB, uid, __VA_ARGS__)

#define TTRACE_WEB_TRACE_EVENT_BEGIN(category_group, name) \
  TraceEventTTraceBegin(TTRACE_TAG_WEB, category_group, name)
#define TTRACE_WEB_TRACE_EVENT_END() traceEnd(TTRACE_TAG_WEB)
#else
#define TTRACE_WEB_SCOPE(...)
#define TTRACE_WEB_MARK(...)
#define TTRACE_WEB_ASYNC_BEGIN(...)
#define TTRACE_WEB_ASYNC_END(...)
#endif

#define TTRACE_WEB TTRACE_WEB_SCOPE

#endif  // _TIZEN_TTRACE_H_
