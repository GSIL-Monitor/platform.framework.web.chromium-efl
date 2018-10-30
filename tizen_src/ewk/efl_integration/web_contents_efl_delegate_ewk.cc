// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "web_contents_efl_delegate_ewk.h"

#include "eweb_view.h"

WebContentsEflDelegateEwk::WebContentsEflDelegateEwk(EWebView* wv)
    : web_view_(wv) {
}

/* LCOV_EXCL_START */
bool WebContentsEflDelegateEwk::ShouldCreateWebContentsAsync(
    content::WebContentsEflDelegate::NewWindowDecideCallback callback,
    const GURL& target_url) {
  // this method is called ONLY when creating new window - no matter what type
  std::unique_ptr<_Ewk_Policy_Decision> pd(
      new _Ewk_Policy_Decision(web_view_, callback));
  pd->ParseUrl(target_url);
  web_view_->SmartCallback<EWebViewCallbacks::NewWindowPolicyDecision>().call(pd.get());

  if (pd->isSuspended()) {
    // it will be deleted later after it's used/ignored/downloaded
    ignore_result(pd.release());
  } else if (!pd->isDecided()) {
    pd->Use();
  }
  return true;
}

bool WebContentsEflDelegateEwk::WebContentsCreateAsync(
    content::WebContentsEflDelegate::WebContentsCreateCallback callback) {
  web_view_->CreateNewWindow(callback);
  return true;
}

void WebContentsEflDelegateEwk::SetUpSmartObject(void* platform_data, void* native_view) {
  EWebView* new_web_view = static_cast<EWebView*>(platform_data);
  Evas_Object* _native_view = static_cast<Evas_Object*>(native_view);
  evas_object_smart_member_add(_native_view, new_web_view->evas_object());
}
/* LCOV_EXCL_STOP */
