// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_BROWSER_WEB_CONTENTS_EFL_DELEGATE_EWK_H_
#define CONTENT_PUBLIC_BROWSER_WEB_CONTENTS_EFL_DELEGATE_EWK_H_

#include "content/public/browser/web_contents_efl_delegate.h"

class EWebView;

class WebContentsEflDelegateEwk : public content::WebContentsEflDelegate {
 public:
  WebContentsEflDelegateEwk(EWebView*);

  /* LCOV_EXCL_START */
  bool ShouldCreateWebContentsAsync(
      NewWindowDecideCallback, const GURL&) override;

  bool WebContentsCreateAsync(WebContentsCreateCallback) override;

  void SetUpSmartObject(void*platform_data, void* native_view) override;
  /* LCOV_EXCL_STOP */

 private:
  EWebView* web_view_;

  DISALLOW_COPY_AND_ASSIGN(WebContentsEflDelegateEwk);
};

#endif //  CONTENT_PUBLIC_BROWSER_WEB_CONTENTS_EFL_DELEGATE_EWK_H_
