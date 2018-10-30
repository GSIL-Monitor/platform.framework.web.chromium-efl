// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "utc_blink_ewk_base.h"

class utc_blink_ewk_view_send_key_event : public utc_blink_ewk_base {
 protected:
  utc_blink_ewk_view_send_key_event()
      : utc_blink_ewk_base(), space_str("space") {}

  // get Evas_Event_Key_Down
  Evas_Event_Key_Down GetKeyDown() {
    Evas_Event_Key_Down down_event;
    memset(&down_event, 0, sizeof(Evas_Event_Key_Down));
    down_event.key = space_str.c_str();
    down_event.string = space_str.c_str();
    return down_event;
  }

  // get Evas_Event_Key_Up
  Evas_Event_Key_Up GetKeyUp() {
    Evas_Event_Key_Up up_event;
    memset(&up_event, 0, sizeof(Evas_Event_Key_Up));
    up_event.key = space_str.c_str();
    up_event.string = space_str.c_str();
    return up_event;
  }

 protected:
  std::string space_str;
};

/**
 * @brief Check if send key down event is succeeds with
 * correct webview.
 */
TEST_F(utc_blink_ewk_view_send_key_event, POS_SEND_KEY_DOWN_EVENT) {
  Evas_Event_Key_Down down_event = GetKeyDown();
  void* key_event = static_cast<void*>(&down_event);
  Eina_Bool result =
      ewk_view_send_key_event(GetEwkWebView(), key_event, EINA_TRUE);
  EXPECT_EQ(EINA_TRUE, result);
}

/**
 * @brief Check if send key up event is succeeds with
 * correct webview.
 */
TEST_F(utc_blink_ewk_view_send_key_event, POS_SEND_KEY_UP_EVENT) {
  Evas_Event_Key_Up up_event = GetKeyUp();
  void* key_event = static_cast<void*>(&up_event);
  Eina_Bool result =
      ewk_view_send_key_event(GetEwkWebView(), key_event, EINA_FALSE);
  EXPECT_EQ(EINA_TRUE, result);
}

/**
 * @brief Check if send key up event is fails with
 * null webview.
 */
TEST_F(utc_blink_ewk_view_send_key_event, NEG_SEND_KEY_UP_EVENT) {
  Evas_Event_Key_Up up_event = GetKeyUp();
  void* key_event = static_cast<void*>(&up_event);
  Eina_Bool result = ewk_view_send_key_event(NULL, key_event, EINA_FALSE);
  EXPECT_EQ(EINA_FALSE, result);
}

/**
 * @brief Check if send key down event is fails with
 * null webview.
 */
TEST_F(utc_blink_ewk_view_send_key_event, NEG_SEND_KEY_DOWN_EVENT) {
  Evas_Event_Key_Down down_event = GetKeyDown();
  void* key_event = static_cast<void*>(&down_event);
  Eina_Bool result = ewk_view_send_key_event(NULL, key_event, EINA_TRUE);
  EXPECT_EQ(EINA_FALSE, result);
}

/**
 * @brief Check if send null key up event is fails with
 * null webview.
 */
TEST_F(utc_blink_ewk_view_send_key_event, NEG_SEND_KEY_UP_EVENT_WITH_NULL) {
  Eina_Bool result = ewk_view_send_key_event(NULL, NULL, EINA_FALSE);
  EXPECT_EQ(EINA_FALSE, result);
}

/**
 * @brief Check if send null key up event is fails with
 * correct webview.
 */
TEST_F(utc_blink_ewk_view_send_key_event, NEG_SEND_KEY_DOWN_EVENT_WITH_NULL) {
  Eina_Bool result = ewk_view_send_key_event(GetEwkWebView(), NULL, EINA_FALSE);
  EXPECT_EQ(EINA_FALSE, result);
}
