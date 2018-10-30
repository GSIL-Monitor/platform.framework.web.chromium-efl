// Copyright 2014 The Samsung Electronics Co., Ltd All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/tracing/tracing_controller_efl.h"

#include "base/json/json_writer.h"
#include "base/logging.h"
#include "base/path_service.h"
#include "base/trace_event/trace_event.h"
#include "base/trace_event/trace_event_impl.h"
#include "content/public/browser/tracing_controller.h"

using namespace content;

namespace {

std::string GenerateUniqueFilename() {
  // TODO(prashant.n): Add algorithm to generate unique filename from timestamp.
  std::string filename = "traces.json";
#if defined(OS_TIZEN)
  base::FilePath homedir;
  PathService::Get(base::DIR_HOME, &homedir);
  return homedir.Append(filename).value();
#else
  // For other tizen platforms (e.g. desktop builds), create trace file in
  // current directory.
  return filename;
#endif
}

}  // namespace

TracingControllerEfl::TracingControllerEfl()
    : is_tracing_(false), weak_factory_(this) {}

TracingControllerEfl& TracingControllerEfl::GetTraceController() {
  static TracingControllerEfl tracing_controller_efl;
  return tracing_controller_efl;
}

bool TracingControllerEfl::StartTracing(const std::string& categories,
                                        const std::string& trace_options,
                                        const std::string& trace_file_name) {
  if (is_tracing_) {
    LOG(INFO) << "Traces: Recording is in progress.";
    return false;
  }

  trace_file_name_ = trace_file_name;

  if (trace_file_name_.empty())
    trace_file_name_ = GenerateUniqueFilename();

  // TODO(prashant.n): Validate filename.

  LOG(INFO) << "Traces: Recording started, categories = [" << categories
            << "], trace options = [" << trace_options << "]";

  base::trace_event::TraceConfig config_(categories, "record-continuously");

  is_tracing_ = TracingController::GetInstance()->StartTracing(
      config_, TracingController::StartTracingDoneCallback());

  return is_tracing_;
}

void TracingControllerEfl::StopTracing() {
  if (!is_tracing_) {
    LOG(INFO) << "Traces: Recording is not started.";
    return;
  }

  is_tracing_ = false;

  base::FilePath file_path(trace_file_name_);
  if (!TracingController::GetInstance()->StopTracing(
          ::TracingController::CreateFileEndpoint(
              file_path, base::Bind(&TracingControllerEfl::OnTracingStopped,
                                    weak_factory_.GetWeakPtr())))) {
    LOG(ERROR) << "Traces: Recording failed.";
    OnTracingStopped();
  }
}

void TracingControllerEfl::OnTracingStopped() {
  LOG(INFO) << "Traces: Recording finished. Traces are recorded in "
            << trace_file_name_ << "";
}
