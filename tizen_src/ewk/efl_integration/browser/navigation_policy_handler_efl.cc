// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser/navigation_policy_handler_efl.h"

#include "content/browser/web_contents/web_contents_impl.h"
#include "ui/base/window_open_disposition.h"

NavigationPolicyHandlerEfl::NavigationPolicyHandlerEfl()
    : decision_(Undecied) {}

NavigationPolicyHandlerEfl::~NavigationPolicyHandlerEfl() {
}

bool NavigationPolicyHandlerEfl::SetDecision(NavigationPolicyHandlerEfl::Decision d) {
  if (decision_ == Undecied && d != Undecied) {
    decision_ = d;
    return true;
  }

  return false;
}
