// Copyright 2018 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file

#ifndef ewk_form_repost_decision_private_h
#define ewk_form_repost_decision_private_h

#include "content/public/browser/web_contents_observer.h"

// This class is used to continue or cancel a pending reload when the
// repost form warning is shown. It is owned by the platform-specific
struct _Ewk_Form_Repost_Decision_Request : public content::WebContentsObserver {
 public:
  static _Ewk_Form_Repost_Decision_Request* Create(
      content::WebContents* web_contents);
  void OnAccepted();
  void OnCanceled();

 private:
  explicit _Ewk_Form_Repost_Decision_Request(
      content::WebContents* web_contents);
  // content::WebContentsObserver methods:
  void BeforeFormRepostWarningShow() override;
  void WebContentsDestroyed() override;
};

#endif  // ewk_form_repost_decision_private_h
