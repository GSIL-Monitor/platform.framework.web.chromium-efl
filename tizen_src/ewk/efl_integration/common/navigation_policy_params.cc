// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "common/navigation_policy_params.h"

#include "third_party/WebKit/public/web/WebNavigationPolicy.h"

NavigationPolicyParams::NavigationPolicyParams()
    : render_view_id(-1)
    , policy(blink::kWebNavigationPolicyIgnore)
    , type(blink::kWebNavigationTypeOther)
    , should_replace_current_entry(false)
    , is_main_frame(false)
    , is_redirect(false) {
}

NavigationPolicyParams::~NavigationPolicyParams() {
}
