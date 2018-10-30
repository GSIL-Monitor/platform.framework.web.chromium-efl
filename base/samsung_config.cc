// Copyright 2017 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/samsung_config.h"

#include "base/base_switches.h"
#include "base/command_line.h"

namespace base {

// static
bool SamsungConfig::IsDexEnabled() {
  CommandLine* cmd = CommandLine::ForCurrentProcess();
  if (!cmd)
    return false;
  return cmd->GetSwitchValueASCII(switches::kEnableSamsungDex) ==
         switches::kSamsungDexEnabledValue;
}

}  // namespace cc
