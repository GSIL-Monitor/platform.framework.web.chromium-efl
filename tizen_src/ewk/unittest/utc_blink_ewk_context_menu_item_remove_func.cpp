// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/* Define those macros _before_ you include the utc_blink_ewk.h header file. */

#include "utc_blink_ewk_base.h"
#include "utc_blink_ewk_context_menu.h"

class utc_blink_ewk_context_menu_item_remove : public utc_blink_ewk_base {
public:
  utc_blink_ewk_context_menu_item_remove()
  : utc_blink_ewk_base()
  , result(EINA_FALSE)
  , is_failed(EINA_FALSE)
  {
  }

protected:
 void LoadFinished(Evas_Object* webview) override {
   feed_mouse_click(3, 100, 100, GetEwkEvas());
  }

  bool LoadError(Evas_Object* webview, Ewk_Error* error) override {
    is_failed = EINA_TRUE;
    EventLoopStop(utc_blink_ewk_base::Success);
    return false;
  }

  void PostSetUp() override {
    /* Enable mouse events to feed events directly. */
    ewk_view_mouse_events_enabled_set(GetEwkWebView(), EINA_TRUE);

    Eina_Bool result_set = ewk_view_url_set(GetEwkWebView(), GetResourceUrl("/ewk_context_menu/index.html").c_str());
    if (!result_set) {
      FAIL();
    }
    evas_object_smart_callback_add(GetEwkWebView(), "contextmenu,customize", contextmenu_customize_callback, this);
  }

  void PreTearDown() override {
    evas_object_smart_callback_del(GetEwkWebView(), "contextmenu,customize", contextmenu_customize_callback);
  }

  static void contextmenu_customize_callback(void* data, Evas_Object* webview, void* event_info)
  {
    utc_message("[contextmenu_customize_callback] :: \n");
    if (!data || !event_info) {
      FAIL();
    }
    Ewk_Context_Menu* contextmenu = static_cast<Ewk_Context_Menu*>(event_info);
    Ewk_Context_Menu_Item* item = ewk_context_menu_nth_item_get(contextmenu, 0);

    utc_blink_ewk_context_menu_item_remove *owner = static_cast<utc_blink_ewk_context_menu_item_remove*>(data);
    owner->result = ewk_context_menu_item_remove(contextmenu, item);

    owner->EventLoopStop(utc_blink_ewk_base::Success);
  }

protected:
  Eina_Bool result;
  Eina_Bool is_failed;
};


/**
 * @brief Tests whether the removing item from context menu is performed properly.
 */
TEST_F(utc_blink_ewk_context_menu_item_remove, POS_TEST)
{
  if (!is_failed) {
    utc_blink_ewk_base::MainLoopResult result_loop = EventLoopStart();
    if (result_loop != utc_blink_ewk_base::Success) {
      FAIL();
    }
  }

  if (is_failed) {
    FAIL();
  }
  EXPECT_EQ(result, EINA_TRUE);
}

/**
 * @brief Tests whether the function works properly for case Ewk_Context_Menu object is NULL.
 */
TEST_F(utc_blink_ewk_context_menu_item_remove, NEG_TEST)
{
  EXPECT_EQ(ewk_context_menu_item_remove(NULL, NULL), EINA_FALSE);
}
