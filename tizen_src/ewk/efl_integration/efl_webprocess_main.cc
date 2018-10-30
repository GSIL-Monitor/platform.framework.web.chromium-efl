// Copyright 2015 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "efl_webprocess_main.h"

#include "base/logging.h"
#include "content_main_delegate_efl.h"
#include "content/public/app/content_main.h"

/* LCOV_EXCL_START */
int efl_webprocess_main(int argc, const char **argv) {
  LOG(INFO) << "web process launching...";

  content::ContentMainDelegateEfl content_main_delegate;
  content::ContentMainParams params(&content_main_delegate);

  params.argc = argc;
  params.argv = argv;

  return content::ContentMain(params);
}
/* LCOV_EXCL_STOP */
