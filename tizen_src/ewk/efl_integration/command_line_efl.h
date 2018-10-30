// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMMAND_LINE_EFL
#define COMMAND_LINE_EFL

#include "base/command_line.h"
#include "content/public/common/main_function_params.h"

class CommandLineEfl {
public:
  static void Init(int argc, char *argv[]);

  static void Shutdown();

  // Get default set of arguments for Tizen port of chromium.
  static content::MainFunctionParams GetDefaultPortParams();

  // Append port specific command line arguments for a specific
  // chromium process type (browser, renderer, zygote, etc).
  static void AppendProcessSpecificArgs(base::CommandLine& command_line);

  static void AppendMemoryOptimizationSwitches(base::CommandLine* command_line);

  // Do not call them apart from ContentMainRunner::Initialize() since they
  // will become invalid after that.
  static const char** GetArgv() { return const_cast<const char**>(argv_); };
  static int GetArgc() { return argc_; };

 private:
  static void AppendUserArgs(base::CommandLine& command_line);

  // Original process argument array provided through EWK API, unfortunately
  // chromium has a nasty habit of messing them up. Please do not rely on those
  // values, use original_arguments_ instead.
  static int argc_;
  static char** argv_;

  // Original arguments passed by the user from command line. Added to all
  // chromium processes. They should remain unchanged until engine shutdown.
  typedef std::vector<char*> ArgumentVector;
  static ArgumentVector original_arguments_;

  static bool is_initialized_;
};

#endif // COMMAND_LINE_EFL
