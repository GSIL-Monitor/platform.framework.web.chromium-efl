// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "utc_blink_ewk_base.h"

#include <cstring>
#include <string>

class utc_blink_ewk_favicon_database_icon_get : public utc_blink_ewk_base {
 protected:
  void PostSetUp() override {
    evas_object_smart_callback_add(GetEwkWebView(), "icon,received",
                                   cb_icon_received, this);
  }

  void PreTearDown() override {
    evas_object_smart_callback_del(GetEwkWebView(), "icon,received",
                                   cb_icon_received);
  }

  static void cb_icon_received(void* data, Evas_Object*, void*) {
    ASSERT_TRUE(data);
    auto owner = static_cast<utc_blink_ewk_favicon_database_icon_get*>(data);
    owner->EventLoopStop(Success);
  }
};

TEST_F(utc_blink_ewk_favicon_database_icon_get, POS) {
  std::string page_path =
      GetResourceUrl("ewk_favicon_database_icon_get/pos.html");
  std::string icon_path =
      GetResourcePath("ewk_favicon_database_icon_get/pos.bmp");

  ASSERT_EQ(EINA_TRUE, ewk_view_url_set(GetEwkWebView(), page_path.c_str()));
  ASSERT_EQ(Success, EventLoopStart());
  Ewk_Favicon_Database* favicon_db =
      ewk_context_favicon_database_get(ewk_context_default_get());
  ASSERT_TRUE(favicon_db);
  Evas_Object* favicon_received = ewk_favicon_database_icon_get(
      favicon_db, page_path.c_str(), GetEwkEvas());
  ASSERT_TRUE(favicon_received);
  int w_received, h_received;
  evas_object_image_size_get(favicon_received, &w_received, &h_received);

  Evas_Object* favicon_expected = evas_object_image_filled_add(GetEwkEvas());
  ASSERT_TRUE(favicon_expected);
  evas_object_image_file_set(favicon_expected, icon_path.c_str(), NULL);
  int w_expected, h_expected;
  evas_object_image_size_get(favicon_expected, &w_expected, &h_expected);

  ASSERT_EQ(w_expected, w_received);
  ASSERT_EQ(h_expected, h_received);

  auto favicon_expected_image_data =
      evas_object_image_data_get(favicon_expected, EINA_FALSE);
  ASSERT_TRUE(favicon_expected_image_data);
  auto favicon_received_image_data =
      evas_object_image_data_get(favicon_received, EINA_FALSE);
  ASSERT_TRUE(favicon_received_image_data);

  const int pixel_size = 4;
  ASSERT_EQ(0, memcmp(favicon_expected_image_data, favicon_received_image_data,
                      w_expected * h_expected * pixel_size));
}

TEST_F(utc_blink_ewk_favicon_database_icon_get, NEG_NO_ICON) {
  std::string page_path =
      GetResourceUrl("ewk_favicon_database_icon_get/neg_no_icon.html");

  Ewk_Favicon_Database* favicon_db =
      ewk_context_favicon_database_get(ewk_context_default_get());
  ASSERT_TRUE(favicon_db);
  Evas_Object* favicon = ewk_favicon_database_icon_get(
      favicon_db, page_path.c_str(), GetEwkEvas());
  ASSERT_FALSE(favicon);
}

TEST_F(utc_blink_ewk_favicon_database_icon_get, NEG_INVALID_DATABASE_PARAM) {
  std::string page_path = GetResourceUrl(
      "ewk_favicon_database_icon_get/neg_invalid_database_param.html");

  ASSERT_EQ(EINA_TRUE, ewk_view_url_set(GetEwkWebView(), page_path.c_str()));
  ASSERT_EQ(Success, EventLoopStart());
  Evas_Object* favicon =
      ewk_favicon_database_icon_get(NULL, page_path.c_str(), GetEwkEvas());
  ASSERT_FALSE(favicon);
}

TEST_F(utc_blink_ewk_favicon_database_icon_get, NEG_INVALID_PAGE_URL_PARAM) {
  Ewk_Favicon_Database* favicon_db =
      ewk_context_favicon_database_get(ewk_context_default_get());
  ASSERT_TRUE(favicon_db);
  Evas_Object* favicon =
      ewk_favicon_database_icon_get(favicon_db, NULL, GetEwkEvas());
  ASSERT_FALSE(favicon);
}

TEST_F(utc_blink_ewk_favicon_database_icon_get, NEG_INVALID_EVAS_PARAM) {
  std::string page_path = GetResourceUrl(
      "ewk_favicon_database_icon_get/neg_invalid_evas_param.html");

  ASSERT_EQ(EINA_TRUE, ewk_view_url_set(GetEwkWebView(), page_path.c_str()));
  ASSERT_EQ(Success, EventLoopStart());
  Ewk_Favicon_Database* favicon_db =
      ewk_context_favicon_database_get(ewk_context_default_get());
  ASSERT_TRUE(favicon_db);
  Evas_Object* favicon =
      ewk_favicon_database_icon_get(favicon_db, page_path.c_str(), NULL);
  ASSERT_FALSE(favicon);
}
