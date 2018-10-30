// Copyright 2018 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ewk_form_repost_decision_private.h"

#include "common/web_contents_utils.h"
#include "eweb_view.h"
#include "public/ewk_form_repost_decision_product.h"

using web_contents_utils::WebViewFromWebContents;

_Ewk_Form_Repost_Decision_Request* _Ewk_Form_Repost_Decision_Request::Create(
    content::WebContents* web_contents) {
  EWebView* web_view = WebViewFromWebContents(web_contents);
  if (web_view)
    return new _Ewk_Form_Repost_Decision_Request(web_contents);
  return nullptr;
}

_Ewk_Form_Repost_Decision_Request::_Ewk_Form_Repost_Decision_Request(
    content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents) {
  EWebView* web_view = WebViewFromWebContents(web_contents);
  if (web_view)
    web_view->InvokeFormRepostWarningShowCallback(this);
}

void _Ewk_Form_Repost_Decision_Request::OnAccepted() {
  if (web_contents())
    web_contents()->GetController().ContinuePendingReload();
}

void _Ewk_Form_Repost_Decision_Request::OnCanceled() {
  if (web_contents())
    web_contents()->GetController().CancelPendingReload();
}

void _Ewk_Form_Repost_Decision_Request::BeforeFormRepostWarningShow() {
  // Close the dialog if we show an additional dialog, to avoid them
  // stacking up.
  EWebView* web_view = WebViewFromWebContents(web_contents());
  if (web_view)
    web_view->InvokeBeforeFormRepostWarningShowCallback(this);
}

void _Ewk_Form_Repost_Decision_Request::WebContentsDestroyed() {
  Observe(nullptr);
  base::ThreadTaskRunnerHandle::Get()->DeleteSoon(FROM_HERE, this);
}
