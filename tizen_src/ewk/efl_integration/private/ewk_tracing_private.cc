// Copyright 2014 The Samsung Electronics Co., Ltd All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ewk_tracing_private.h"

#include "content/browser/tracing/tracing_controller_efl.h"

TracingControllerEfl& GetTraceController() {
  return TracingControllerEfl::GetTraceController();
}

bool ewk_start_tracing_private(const char* categories,
                               const char* trace_options,
                               const char* trace_file_name) {
  return GetTraceController().StartTracing(std::string(categories),
                                           std::string(trace_options),
                                           std::string(trace_file_name));
}

void ewk_stop_tracing_private() {
  GetTraceController().StopTracing();
}
