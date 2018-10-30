// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "utc_blink_ewk_base.h"
#include <fstream>
#include <iostream>

class utc_blink_ewk_context_icon_database_path_set : public utc_blink_ewk_base
{
protected:
  void PostSetUp() override
  {
    evas_object_smart_callback_add(GetEwkWebView(), "icon,received", cb_icon_received, this);
  }

  void PreTearDown() override
  {
    evas_object_smart_callback_del(GetEwkWebView(), "icon,received", cb_icon_received);
  }

  static void cb_icon_received(void *data, Evas_Object *, void *)
  {
    ASSERT_TRUE(data != NULL);
    utc_blink_ewk_context_icon_database_path_set *owner = static_cast<utc_blink_ewk_context_icon_database_path_set *>(data);
    owner->EventLoopStop(Success);
  }

  static bool exists(const char *path) {
    std::ifstream fs(path);
    return fs.good();
  }
};

TEST_F(utc_blink_ewk_context_icon_database_path_set, CORRECT_PATH_SET)
{
  const char *path = "/tmp/utc_blink_favicon_database_test_correct.db";
  ASSERT_EQ(EINA_TRUE, ewk_context_icon_database_path_set(ewk_context_default_get(), path));
  ASSERT_EQ(EINA_TRUE, ewk_view_url_set(GetEwkWebView(), GetResourceUrl("common/favicon.html").c_str()));
  ASSERT_EQ(Success, EventLoopStart());
  ASSERT_TRUE(exists(path));
}

TEST_F(utc_blink_ewk_context_icon_database_path_set, INCORRECT_PATH_SET)
{
  const char *path = "/tmp/utc_blink_favicon_database_test_incorrect.db";
  ASSERT_EQ(EINA_TRUE, ewk_view_url_set(GetEwkWebView(), GetResourceUrl("common/favicon.html").c_str()));
  ASSERT_EQ(Success, EventLoopStart());
  ASSERT_EQ(EINA_FALSE, ewk_context_icon_database_path_set(ewk_context_default_get(), path));
}
