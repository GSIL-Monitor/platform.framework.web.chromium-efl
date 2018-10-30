// Copyright 2018 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/WebKit/public/platform/WebApplicationType.h"

namespace blink {

namespace {
ApplicationType s_applicationType = ApplicationType::WEBBROWSER;
}  // namespace

void SetApplicationType(const ApplicationType appType) {
  s_applicationType = appType;
}

bool IsWebBrowser() {
  return s_applicationType == ApplicationType::WEBBROWSER;
}

bool IsHbbTV() {
  return s_applicationType == ApplicationType::HBBTV;
}

bool IsTIZENWRT() {
  return s_applicationType == ApplicationType::TIZENWRT;
}

}  // namespace blink
