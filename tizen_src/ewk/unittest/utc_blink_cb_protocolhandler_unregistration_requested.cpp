// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "utc_blink_ewk_base.h"

class utc_blink_cb_protocolhandler_unregistration_requested : public utc_blink_ewk_base
{
protected:
  utc_blink_cb_protocolhandler_unregistration_requested()
    : handler_target_(NULL)
    , handler_base_url_(NULL)
    , handler_url_(NULL)
  {}

  ~utc_blink_cb_protocolhandler_unregistration_requested() override {
    if (handler_target_) free(handler_target_);
    if (handler_base_url_) free(handler_base_url_);
    if (handler_url_) free(handler_url_);
  }

  void LoadFinished(Evas_Object *) override
  {
    EventLoopStop(Failure);
  }

  void PostSetUp() override
  {
    evas_object_smart_callback_add(GetEwkWebView(), "protocolhandler,unregistration,requested", cb_protocolhandler_unregistration_requested, this);
  }

  void PreTearDown() override
  {
    evas_object_smart_callback_del(GetEwkWebView(), "protocolhandler,unregistration,requested", cb_protocolhandler_unregistration_requested);
  }

  static void cb_protocolhandler_unregistration_requested(void *data, Evas_Object *, void *info)
  {
    ASSERT_TRUE(data != NULL);
    utc_message("protocol handler unregistered");
    utc_blink_cb_protocolhandler_unregistration_requested *owner;
    OwnerFromVoid(data, &owner);
    EXPECT_TRUE(info);
    Ewk_Custom_Handlers_Data *handler_data_ = static_cast<Ewk_Custom_Handlers_Data *>(info);
    owner->handler_target_ = ewk_custom_handlers_data_target_get(handler_data_) ? strdup(ewk_custom_handlers_data_target_get(handler_data_)) : 0;
    owner->handler_base_url_ = ewk_custom_handlers_data_base_url_get(handler_data_) ? strdup(ewk_custom_handlers_data_base_url_get(handler_data_)) : 0;
    owner->handler_url_ = ewk_custom_handlers_data_url_get(handler_data_) ? strdup(ewk_custom_handlers_data_url_get(handler_data_)) : 0;
    owner->EventLoopStop(Success);
  }

protected:
  char *handler_target_;
  char *handler_base_url_;
  char *handler_url_;
};

TEST_F(utc_blink_cb_protocolhandler_unregistration_requested, MAILTO_PROTOCOL_UNREGISTRATION)
{
  std::string url = GetResourceUrl("protocol_handler/unregister_protocol_handler.html");
  ASSERT_EQ(EINA_TRUE, ewk_view_url_set(GetEwkWebView(), url.c_str()));
  ASSERT_EQ(Success, EventLoopStart());
  ASSERT_STREQ("mailto", handler_target_);
  ASSERT_STREQ("", handler_base_url_);
  ASSERT_STREQ((url + "?url=%s").c_str(), handler_url_);
}
