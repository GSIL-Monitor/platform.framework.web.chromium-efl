// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "command_line_efl.h"

#include <string>
#include <cstring>
#include <sys/types.h>
#include <unistd.h>

#include "base/base_switches.h"
#include "base/files/file_path.h"
#include "cc/base/switches.h"
#include "common/content_switches_efl.h"
#include "components/autofill/core/common/autofill_switches.h"
#include "content/public/common/content_client.h"
#include "content/public/common/content_switches.h"
#include "efl/init.h"
#include "tizen/system_info.h"
#include "ui/base/ui_base_switches.h"
#include "ui/events/event_switches.h"
#include "ui/gl/gl_switches.h"
#include "url/gurl.h"

int CommandLineEfl::argc_ = 0;
char** CommandLineEfl::argv_ = nullptr;
CommandLineEfl::ArgumentVector CommandLineEfl::original_arguments_;
bool CommandLineEfl::is_initialized_ = false;

void CommandLineEfl::Init(int argc, char *argv[]) {
  if (CommandLineEfl::is_initialized_) {
    LOG(ERROR)
        << "CommandLineEfl::Init should not be called more than once";  // LCOV_EXCL_LINE
  }
  base::CommandLine::Init(argc, argv);

  argc_ = argc;
  argv_ = argv;

  // Unfortunately chromium modifies application's argument vector.
  // during initialization. This means that chromium after initialization
  // argv will only contain one valid entry, argv[0]. To be able to use
  // user provided arguments after initialization we need to make a copy
  // of them.
  // See: chromium/src/content/common/set_process_title_linux.cc
  for (int i = 0; i < argc; ++i)
    original_arguments_.push_back(strdup(argv[i]));  // LCOV_EXCL_LINE
  CommandLineEfl::is_initialized_ = true;
}

content::MainFunctionParams CommandLineEfl::GetDefaultPortParams() {
  if (!CommandLineEfl::is_initialized_) {
    CommandLineEfl::Init(0, NULL);
  }
  base::CommandLine* p_command_line = base::CommandLine::ForCurrentProcess();

  efl::AppendPortParams(*p_command_line);

  p_command_line->AppendSwitch(switches::kNoSandbox);

  // TODO: Disable browser side navigation (aka PlzNavigation) in order to
  // compatibility of loading related callbacks such as 'load,error'.
  // But we can enable it to increase loading speed after resolving
  // compatibility related problems.
  // See: crbug.com/368813
  p_command_line->AppendSwitch(switches::kDisableBrowserSideNavigation);

  AppendMemoryOptimizationSwitches(p_command_line);

#if defined(OS_TIZEN)
  p_command_line->AppendSwitch(switches::kMainFrameResizesAreOrientationChanges);
  p_command_line->AppendSwitch(switches::kForceAccelerated2dCanvas);

  // Append |kDisableGpuEarlyInit| switch except for prelaunched wrt-loader.
  // The GPU thread will be created early from |BrowserMainLoop| without
  // |kDisableGpuEarlyInit| switch.
  if (p_command_line->GetProgram().BaseName().value().compare("wrt-loader"))
    p_command_line->AppendSwitch(switches::kDisableGpuEarlyInit);

  // For optimizing discardable memory. [limit:MB, delay:ms]
  if (!p_command_line->HasSwitch(switches::kDiscardableMemoryLimit))
    p_command_line->AppendSwitchASCII(switches::kDiscardableMemoryLimit,
                                      IsWearableProfile() ? "5" : "20");

  if (!p_command_line->HasSwitch(switches::kDiscardableMemoryPurgeDelay))
    p_command_line->AppendSwitchASCII(switches::kDiscardableMemoryPurgeDelay,
                                      IsTvProfile() ? "5000" : "500");
#endif

  if (IsDesktopProfile()) {
    // Enable Spatial Navigation by default for desktop only
    p_command_line->AppendSwitch(
        switches::kEnableSpatialNavigation);  // LCOV_EXCL_LINE
  }

  if (IsMobileProfile() || IsWearableProfile()) {
    p_command_line->AppendSwitchASCII(switches::kTouchEvents,
                                      switches::kTouchEventsEnabled);
    p_command_line->AppendSwitch(switches::kEnablePinch);
    p_command_line->AppendSwitch(switches::kUseMobileUserAgent);
    // [M42_2231] FIXME: Need Parallel Canvas patch for S-Chromium/S-Blink/S-Skia
    p_command_line->AppendSwitchASCII(
        switches::kAcceleratedCanvas2dMSAASampleCount, "4");
    p_command_line->AppendSwitch(switches::kEnableTouchDragDrop);
  }

  // Disable async worker context.
  p_command_line->AppendSwitch(switches::kDisableGpuAsyncWorkerContext);

  p_command_line->AppendSwitch(switches::kDisableGpuVsync);

  // XXX: Skia benchmarking should be only used for testing,
  // when enabled the following warning is printed to stderr:
  // "Enabling unsafe Skia benchmarking extension."
  // p_command_line->AppendSwitch(switches::kEnableSkiaBenchmarking);

  AppendUserArgs(*p_command_line);

  return content::MainFunctionParams(*p_command_line);
}

void CommandLineEfl::Shutdown() {
  for (ArgumentVector::iterator it = original_arguments_.begin();
       it != original_arguments_.end(); ++it) {
    free(*it);
  }
  original_arguments_.clear();
}

void CommandLineEfl::AppendProcessSpecificArgs(base::CommandLine& command_line) {
  AppendUserArgs(command_line);

  if (IsMobileProfile() || IsWearableProfile()) {
    std::string process_type =
        command_line.GetSwitchValueASCII(switches::kProcessType);
    if (process_type == switches::kRendererProcess)
      command_line.AppendSwitch(autofill::switches::kEnableSingleClickAutofill);
  }
}

void CommandLineEfl::AppendUserArgs(base::CommandLine& command_line) {
  for (ArgumentVector::const_iterator it = original_arguments_.begin();
       it != original_arguments_.end(); ++it) {
    command_line.AppendSwitch(*it);  // LCOV_EXCL_LINE
  }
}

void CommandLineEfl::AppendMemoryOptimizationSwitches(
    base::CommandLine* command_line) {

  // For reuse of timers with message pump
  command_line->AppendSwitch(switches::kLimitMemoryAllocationInScheduleDelayedWork);

}
