// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gtest/gtest.h"

#include <glib-object.h>
#include <Elementary.h>

#include "ewk_main.h"

int argc_;
char** argv_;

int main(int argc, char* argv[])
{
  ewk_init();
  setenv("ELM_ENGINE", "gl", 1);
  elm_init(0, NULL);

  // Whenever a Google Test flag is seen, it is removed from argv
  // and argc is decremented. Remeber argc_ and argv_ after this call
  // not to propagate --gtest_filter or --gtest_output to chromium.
  testing::InitGoogleTest(&argc, argv);
  argc_ = argc;
  argv_ = argv;

  int retval = RUN_ALL_TESTS();

  /* 2. Closing whole EWK */
  ewk_shutdown();
  return retval;
}

