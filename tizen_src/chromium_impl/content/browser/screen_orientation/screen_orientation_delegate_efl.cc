// Copyright 2014 The Chromium Authors. All rights reserved.
// Copyright 2015 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/screen_orientation/screen_orientation_delegate_efl.h"

#include "chromium_impl/content/browser/web_contents/web_contents_impl_efl.h"
#include "chromium_impl/content/browser/web_contents/web_contents_view_efl.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_delegate.h"
#include "ewk/efl_integration/web_contents_view_efl_delegate_ewk.h"

namespace content {

ScreenOrientationDelegate* CreateScreenOrientationDelegateEfl() {
   return new ScreenOrientationDelegateEfl();
}

ScreenOrientationDelegateEfl::ScreenOrientationDelegateEfl() {
}

ScreenOrientationDelegateEfl::~ScreenOrientationDelegateEfl() {
}

bool ScreenOrientationDelegateEfl::FullScreenRequired(
    WebContents* web_contents) {
  // TODO: Always required on EFL?
  return true;
}

void ScreenOrientationDelegateEfl::Lock(
    WebContents* web_contents,
    blink::WebScreenOrientationLockType lock_orientation) {
  WebContentsImpl* wci = static_cast<WebContentsImpl*>(web_contents);
  WebContentsViewEfl* wcve = static_cast<WebContentsViewEfl*>(wci->GetView());
  if (wcve->GetEflDelegate())
    wcve->GetEflDelegate()->OrientationLock(lock_orientation);
}

bool ScreenOrientationDelegateEfl::ScreenOrientationProviderSupported() {
  // TODO: Always supported on EFL?
  return true;
}

void ScreenOrientationDelegateEfl::Unlock(WebContents* web_contents) {
  WebContentsImpl* wci = static_cast<WebContentsImpl*>(web_contents);
  WebContentsViewEfl* wcve = static_cast<WebContentsViewEfl*>(wci->GetView());
  if (wcve->GetEflDelegate())
    wcve->GetEflDelegate()->OrientationUnlock();
}

} // namespace content
