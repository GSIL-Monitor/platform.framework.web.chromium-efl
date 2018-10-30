// Copyright 2014-2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "utc_blink_ewk_base.h"
#include <inttypes.h>

class utc_blink_ewk_context_local_file_system_delete
    : public utc_blink_ewk_base {
 protected:
  utc_blink_ewk_context_local_file_system_delete()
      : utc_blink_ewk_base(),
        ctx(nullptr),
        origins(nullptr),
        origin_to_delete(nullptr) {}

  ~utc_blink_ewk_context_local_file_system_delete() override {
    SetOrigins(nullptr);
  }

  void PreSetUp() override { AllowFileAccessFromFiles(); }

  void PostSetUp() override {
    ctx = ewk_view_context_get(GetEwkWebView());
    evas_object_smart_callback_add(GetEwkWebView(), "title,changed",
                                   title_changed_cb, this);
  }

  void PreTearDown() override {
    evas_object_smart_callback_del(GetEwkWebView(), "title,changed",
                                   title_changed_cb);
  }

  static void origins_get_cb(Eina_List* origins, void* user_data)
  {
    utc_message("[origins_get_cb] :: %p, %p", origins, user_data);
    ASSERT_TRUE(user_data);
    utc_blink_ewk_context_local_file_system_delete* owner = static_cast<utc_blink_ewk_context_local_file_system_delete*>(user_data);
    owner->SetOrigins(origins);
    owner->EventLoopStop(Success);
  }

  void SetOrigins(Eina_List* new_origins) {
    utc_message("[SetOrigins] :: ");
    if (new_origins != origins) {
      if (origins) {
        ewk_context_origins_free(origins);
      }
      origins = new_origins;
    }
  }

  Eina_Bool FindOriginToDelete(const char* expected_protocol,
                               const char* expected_host,
                               uint16_t expected_port) {
    Eina_List* list = nullptr;
    void* list_data = nullptr;
    EINA_LIST_FOREACH(origins, list, list_data) {
      Ewk_Security_Origin* origin = (Ewk_Security_Origin*)(list_data);
      const char* protocol = ewk_security_origin_protocol_get(origin);
      const char* host = ewk_security_origin_host_get(origin);
      uint16_t port = ewk_security_origin_port_get(origin);
      utc_message(
          "[FindOriginToDelete] :: "
          "protocol: %s :: host: %s :: port: %" PRIu16,
          protocol, host, port);
      if (!strcmp(protocol, expected_protocol) &&
          !strcmp(host, expected_host) && (port == expected_port)) {
        origin_to_delete = origin;
        return true;
      }
    }
    return false;
  }

  void GetOrigins(int expected_count) {
    utc_message("[GetOrigins] :: %d", eina_list_count(origins));
    // web database operations are async, we must wait for changes to propagate
    ASSERT_EQ(EINA_TRUE, ewk_context_local_file_system_origins_get(
                             ctx, origins_get_cb, this));
    ASSERT_EQ(Success, EventLoopStart());
    ASSERT_EQ(expected_count, eina_list_count(origins));
  }

  static void origins_get_cb(Eina_List* origins, void* data) {
    utc_message("[origins_get_cb] :: origins: %p, data: %p", origins, data);
    if (data) {
      auto owner =
          static_cast<utc_blink_ewk_context_local_file_system_delete*>(data);
      owner->SetOrigins(origins);
      owner->EventLoopStop(Success);
    }
  }

  static void title_changed_cb(void* data,
                               Evas_Object* webView,
                               void* event_info) {
    utc_message("[title_changed_cb] :: data: %p", data);
    if (data) {
      auto owner =
          static_cast<utc_blink_ewk_context_local_file_system_delete*>(data);
      if (!strcmp("SUCCESS", static_cast<char*>(event_info))) {
        owner->EventLoopStop(Success);
      } else if (!strcmp("FAILURE", static_cast<char*>(event_info))) {
        owner->EventLoopStop(Failure);
      }
    }
  }

  Ewk_Context* ctx;
  Eina_List* origins;
  Ewk_Security_Origin* origin_to_delete;
};

/**
 * @brief Tests if there is possibility to get local file system origins
**/
TEST_F(utc_blink_ewk_context_local_file_system_delete, POS) {
  std::string url = GetResourceUrl(
      "ewk_context_local_file_system/"
      "sample_context_local_file_system_write.html");
  const char* const expected_protocol = "file";
  const char* const expected_host = "";
  const uint16_t expected_port = 0;
  // Delete the local file system and check if there are no origins left
  ASSERT_EQ(EINA_TRUE, ewk_context_local_file_system_all_delete(ctx));
  GetOrigins(0);

  // Create a local file system
  ASSERT_EQ(EINA_TRUE, ewk_view_url_set(GetEwkWebView(), url.c_str()));
  ASSERT_EQ(Success, EventLoopStart());
  EventLoopWait(2);

  // Check if origin is loaded
  GetOrigins(1);
  ASSERT_TRUE(
      FindOriginToDelete(expected_protocol, expected_host, expected_port));
  ASSERT_TRUE(origin_to_delete);

  // Delete selected origin
  ASSERT_EQ(EINA_TRUE,
            ewk_context_local_file_system_delete(ctx, origin_to_delete));
  EventLoopWait(2);

  // Check if there is no expected origin
  GetOrigins(0);
  ASSERT_FALSE(
      FindOriginToDelete(expected_protocol, expected_host, expected_port));
}

/**
 * @brief Negative test for ewk_context_local_file_system_delete(). Checking
 *        whether function works properly with null origin value.
**/
TEST_F(utc_blink_ewk_context_local_file_system_delete, NEG_ORIGIN_NULL) {
  ASSERT_EQ(EINA_FALSE, ewk_context_local_file_system_delete(ctx, nullptr));
}

/**
 * @brief Negative test for ewk_context_local_file_system_delete(). Checking
 *        whether function works properly with null context and origin value.
**/
TEST_F(utc_blink_ewk_context_local_file_system_delete,
       NEG_ORIGIN_AND_CONTEXT_NULL) {
  ASSERT_EQ(EINA_FALSE, ewk_context_local_file_system_delete(nullptr, nullptr));
}
