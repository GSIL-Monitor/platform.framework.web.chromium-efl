// Copyright 2018 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WebApplicationType_h
#define WebApplicationType_h

namespace blink {

enum class ApplicationType : unsigned {
  WEBBROWSER = 0,
  HBBTV,
  TIZENWRT,
  OTHER
};

void SetApplicationType(const ApplicationType appType);
bool IsWebBrowser();
bool IsHbbTV();
bool IsTIZENWRT();

}  // namespace blink

#endif
