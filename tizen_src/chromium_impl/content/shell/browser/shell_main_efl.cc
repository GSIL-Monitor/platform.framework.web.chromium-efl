// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/app/content_main.h"

#include "base/at_exit.h"
#include "content/shell/app/shell_main_delegate.h"
#include "efl/init.h"

int main(int argc, const char* argv[]) {
  base::AtExitManager at_exit;
  if (efl::Initialize(argc, argv))
    return 1;

  content::ShellMainDelegate delegate;

  content::ContentMainParams prams(&delegate);
  prams.argc = argc;
  prams.argv = argv;
  return content::ContentMain(prams);
}
