// Copyright 2018 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Define all the application type.
#ifndef APPLICATION_TYPE_H_
#define APPLICATION_TYPE_H_

namespace content {

enum class ApplicationType : unsigned {
  WEBBROWSER = 0,
  HBBTV,
  TIZENWRT,
  OTHER
};

void SetApplicationType(const ApplicationType app_type);
bool IsWebBrowser();
bool IsHbbTV();
bool IsTIZENWRT();
}  // namespace content
#endif  // APPLICATION_TYPE_H_
