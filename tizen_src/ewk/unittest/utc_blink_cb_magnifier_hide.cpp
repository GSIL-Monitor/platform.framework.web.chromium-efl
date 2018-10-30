// Copyright 2014-2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "utc_blink_ewk_base.h"

#define URL  ("http://google.com")

class utc_blink_cb_magnifier_hide : public utc_blink_ewk_base
{
public:
    /* Callback for "load,finished" */
 void LoadFinished(Evas_Object* webview) override {
   EventLoopStop(utc_blink_ewk_base::Success);
    }

    void PostSetUp() override {
      evas_object_smart_callback_add(GetEwkWebView(), "magnifier,show", magnifier_show_cb, this);
      evas_object_smart_callback_add(GetEwkWebView(), "magnifier,hide", magnifier_hide_cb, this);
      touch_events_enabled = ewk_view_touch_events_enabled_get(GetEwkWebView());
      ewk_view_touch_events_enabled_set(GetEwkWebView(), EINA_TRUE);
    }

    void PreTearDown() override {
      ewk_view_touch_events_enabled_set(GetEwkWebView(), touch_events_enabled);
      evas_object_smart_callback_del(GetEwkWebView(), "magnifier,show", magnifier_show_cb);
      evas_object_smart_callback_del(GetEwkWebView(), "magnifier,hide", magnifier_hide_cb);
    }

    static void magnifier_show_cb(void* data, Evas_Object* webview, void* event_info)
    {
      utc_blink_cb_magnifier_hide *owner = NULL;
      OwnerFromVoid(data, &owner);
      Ewk_View_Smart_Data* ewk_smart_data = static_cast<Ewk_View_Smart_Data*>(evas_object_smart_data_get(owner->GetEwkWebView()));
      ewk_smart_data->api->text_selection_up(ewk_smart_data, 10, 10);
    }

    static void magnifier_hide_cb(void* data, Evas_Object* webview, void* event_info)
    {
      utc_blink_cb_magnifier_hide *owner = NULL;
      OwnerFromVoid(data, &owner);
      owner->EventLoopStop(Success);
    }

    static Eina_Bool do_text_selection_down(void* data)
    {
      utc_blink_cb_magnifier_hide *owner = NULL;
      OwnerFromVoid(data, &owner);
      Ewk_View_Smart_Data* ewk_smart_data = static_cast<Ewk_View_Smart_Data*>(evas_object_smart_data_get(owner->GetEwkWebView()));
      ewk_smart_data->api->text_selection_down(ewk_smart_data, 10, 10);
      return ECORE_CALLBACK_CANCEL;
    }

    static void job_do_text_selection_down(utc_blink_ewk_base* data)
    {
      utc_blink_cb_magnifier_hide* owner =
        static_cast<utc_blink_cb_magnifier_hide*>(data);
      ecore_timer_add(1.0, do_text_selection_down, owner);
    }

 protected:
  utc_blink_cb_magnifier_hide()
      : touch_events_enabled(EINA_FALSE) {}

  Eina_Bool touch_events_enabled;
};

/**
 * @brief Tests "magnifier,hide" callback
 */
TEST_F(utc_blink_cb_magnifier_hide, callback)
{
    utc_message("Loading url: %s", URL);
    ASSERT_EQ(EINA_TRUE, ewk_view_url_set(GetEwkWebView(), URL));
    ASSERT_EQ(Success, EventLoopStart());

    SetTestJob(utc_blink_cb_magnifier_hide::job_do_text_selection_down);
    ASSERT_EQ(Success, EventLoopStart());
}
