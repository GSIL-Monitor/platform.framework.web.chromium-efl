// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ewk_view_private_h
#define ewk_view_private_h

#include <Evas.h>

#include "eweb_view.h"
#include "ewk_view.h"

class Ewk_Context;

// Create WebView Evas Object
Evas_Object* CreateWebViewAsEvasObject(Ewk_Context* context, Evas* canvas, Evas_Smart* smart = 0);
bool InitSmartClassInterface(Ewk_View_Smart_Class& api);

// EwkView's Smart Class Name
const char EwkViewSmartClassName[] = "EWebView";

// type conversion utility
bool IsWebViewObject(const Evas_Object* evas_object);
Ewk_View_Smart_Data* GetEwkViewSmartDataFromEvasObject(const Evas_Object* evas_object);
EWebView* GetWebViewFromSmartData(const Ewk_View_Smart_Data* smartData);
EWebView* GetWebViewFromEvasObject(const Evas_Object* eo);

// helper macro
// TODO: "EINA_LOG_ERR" should be changed to "EINA_LOG_CRIT"
// (http://web.sec.samsung.net/bugzilla/show_bug.cgi?id=15311)
#define EWK_VIEW_IMPL_GET_OR_RETURN(evas_object, impl, ...)           \
  EWebView* impl = GetWebViewFromEvasObject(evas_object);             \
  do {                                                                \
    if (!impl) {                                                      \
      EINA_LOG_ERR("Evas Object %p is not Ewk WebView", evas_object); \
      return __VA_ARGS__;                                             \
    }                                                                 \
  } while (0)

#endif // ewk_view_private_h
