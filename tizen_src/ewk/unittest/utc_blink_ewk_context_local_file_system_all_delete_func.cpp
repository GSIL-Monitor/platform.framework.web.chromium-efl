// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "utc_blink_ewk_base.h"

class utc_blink_ewk_context_local_file_system_all_delete : public utc_blink_ewk_base
{
protected:
  utc_blink_ewk_context_local_file_system_all_delete()
    : utc_blink_ewk_base()
    , ctx(NULL)
    , origins(NULL)
  {
  }

  ~utc_blink_ewk_context_local_file_system_all_delete() override {
    SetOrigins(NULL);
  }

  void PreSetUp() override { AllowFileAccessFromFiles(); }

  void PostSetUp() override {
    utc_message("[PostSetUp] :: ");
    ctx = ewk_view_context_get(GetEwkWebView());
    evas_object_smart_callback_add(GetEwkWebView(), "title,changed", title_changed_cb, this);
  }

  void PreTearDown() override {
    evas_object_smart_callback_del(GetEwkWebView(), "title,changed", title_changed_cb);
  }

  static void origins_get_cb(Eina_List* origins, void* user_data)
  {
    utc_message("[origins_get_cb] :: %p, %p", origins, user_data);
    ASSERT_TRUE(user_data);
    utc_blink_ewk_context_local_file_system_all_delete* owner = static_cast<utc_blink_ewk_context_local_file_system_all_delete*>(user_data);

    owner->SetOrigins(origins);
    owner->EventLoopStop(Success);
  }

  static void title_changed_cb(void* user_data, Evas_Object* webView, void* event_info)
  {
    utc_message("[title_changed_cb] :: \n");

    ASSERT_TRUE(user_data);
    utc_blink_ewk_context_local_file_system_all_delete* owner = static_cast<utc_blink_ewk_context_local_file_system_all_delete*>(user_data);

    if (!strcmp("SUCCESS", (char*)event_info)) {
      owner->EventLoopStop(Success);
    }
    else if (!strcmp("FAILURE", (char*)event_info)) {
      owner->EventLoopStop(Failure);
    }
  }

  void SetOrigins(Eina_List* new_origins)
  {
    utc_message("[SetOrigins] :: ");
    if (new_origins != origins) {
      if (origins) {
        ewk_context_origins_free(origins);
      }
      origins = new_origins;
    }
  }

  void GetOrigins(int expected_count)
  {
    utc_message("[GetOrigins] :: ");
    // web database operations are async, we must wait for changes to propagate
    for (int i = 0; i < 3; ++i) {
      if (EINA_TRUE != ewk_context_local_file_system_origins_get(ctx, origins_get_cb, this))
        break;

      if (Success != EventLoopStart())
        break;

      if (expected_count == eina_list_count(origins))
        break;

      if (!EventLoopWait(3.0))
        break;
    }
  }

protected:
  static const char* const URL;

protected:
  Ewk_Context* ctx;
  Eina_List* origins;
};

const char* const utc_blink_ewk_context_local_file_system_all_delete::URL = "ewk_context_local_file_system/sample_context_local_file_system_write.html";

/**
* @brief Tests if can delete all origins
*/
TEST_F(utc_blink_ewk_context_local_file_system_all_delete, POS_TEST)
{
  // Create a local file system
  ASSERT_EQ(EINA_TRUE, ewk_view_url_set(GetEwkWebView(), GetResourceUrl(URL).c_str()));
  ASSERT_EQ(Success, EventLoopStart());

  // Check if any origin is loaded
  GetOrigins(1);
  ASSERT_GE(eina_list_count(origins), 1);

  // Delete the local file system
  ASSERT_EQ(EINA_TRUE, ewk_context_local_file_system_all_delete(ctx));
  // Check if there are no origins left
  GetOrigins(0);
  ASSERT_EQ(0, eina_list_count(origins));
  ASSERT_TRUE(origins == NULL);
}

/**
* @brief Tests if cannot delete all local file systems when context is null
*/
TEST_F(utc_blink_ewk_context_local_file_system_all_delete, NEG_TEST)
{
  ASSERT_EQ(EINA_FALSE, ewk_context_local_file_system_all_delete(NULL));
}
