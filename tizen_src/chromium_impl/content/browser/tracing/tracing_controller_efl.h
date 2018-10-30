// Copyright 2014 The Samsung Electronics Co., Ltd All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IMPL_BROWSER_TRACING_TRACING_CONTROLLER_EFL_H_
#define IMPL_BROWSER_TRACING_TRACING_CONTROLLER_EFL_H_

#include <string>

#include "base/memory/weak_ptr.h"
#include "content/common/content_export.h"

class CONTENT_EXPORT TracingControllerEfl {
 public:
  static TracingControllerEfl& GetTraceController();
  bool StartTracing(const std::string& categories,
                    const std::string& trace_options,
                    const std::string& trace_file_name);
  void StopTracing();

 private:
  TracingControllerEfl();
  void OnTracingStopped();

  bool is_tracing_;
  base::WeakPtrFactory<TracingControllerEfl> weak_factory_;
  std::string trace_file_name_;

  DISALLOW_COPY_AND_ASSIGN(TracingControllerEfl);
};

#endif  // IMPL_BROWSER_TRACING_TRACING_CONTROLLER_EFL_H_
