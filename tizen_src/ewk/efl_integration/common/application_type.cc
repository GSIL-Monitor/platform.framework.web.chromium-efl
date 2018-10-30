// Copyright 2018 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "common/application_type.h"

#include "third_party/WebKit/public/platform/WebApplicationType.h"

namespace content {

// Make sure the enum values of content::ApplicationType and
// blink::ApplicationType match.
#define STATIC_ASSERT_ENUM_MATCH(name)                                     \
  static_assert(                                                           \
      content::name == static_cast<content::ApplicationType>(blink::name), \
      #name " value must match in content and blink.")

STATIC_ASSERT_ENUM_MATCH(ApplicationType::WEBBROWSER);
STATIC_ASSERT_ENUM_MATCH(ApplicationType::HBBTV);
STATIC_ASSERT_ENUM_MATCH(ApplicationType::TIZENWRT);
STATIC_ASSERT_ENUM_MATCH(ApplicationType::OTHER);

void SetApplicationType(const ApplicationType app_type) {
  blink::SetApplicationType(static_cast<blink::ApplicationType>(app_type));
}

bool IsWebBrowser() {
  return blink::IsWebBrowser();
}

bool IsHbbTV() {
  return blink::IsHbbTV();
}

bool IsTIZENWRT() {
  return blink::IsTIZENWRT();
}
}
