// Copyright 2013 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "version_info.h"

#include "base/command_line.h"
#include "content/public/common/content_switches.h"
#include "content/public/common/user_agent.h"
#include "tizen/system_info.h"

#if defined(OS_TIZEN)
#include <system_info.h>
#endif

namespace EflWebView {

const char kOSTypeLinux[] = "Linux";

VersionInfo* VersionInfo::version_info_ = nullptr;

VersionInfo* VersionInfo::GetInstance() {
  if (!version_info_)
    version_info_ = new VersionInfo();
  return version_info_;
}

/* LCOV_EXCL_START */
void VersionInfo::DeleteInstance() {
  if (version_info_)
    delete version_info_;
  version_info_ = nullptr;
}
/* LCOV_EXCL_STOP */

VersionInfo::VersionInfo()
  : product_name_(std::string()),
    system_info_(OSType()),
    user_agent_(CreateUserAgent()) {
}

std::string VersionInfo::CreateUserAgent() const {
  if (IsTvProfile()) {
    // FIXME : The hard-coded user agent for tizen tv
    return "Mozilla/5.0 (SMART-TV; LINUX; Tizen 5.0) AppleWebKit/537.36 "
           "(KHTML, like Gecko) Version/5.0 TV Safari/537.36";  // LCOV_EXCL_LINE
  }
  std::string version = VersionForUserAgent();
  std::string product = product_name_;
  if (!product.empty())
    product += " ";  // LCOV_EXCL_LINE

  product += version;

  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
      switches::kUseMobileUserAgent)) {
    product += " Mobile";
  }

  return content::BuildUserAgentFromOSAndProduct(system_info_, product);
}

std::string VersionInfo::OSType() const {
#if defined(OS_TIZEN)
  char *device_model = nullptr;
  char *tizen_version = nullptr;
  char *platform_name = nullptr;
  std::string device_model_str;
  std::string tizen_version_str;
  std::string platform_name_str;
  int result = system_info_get_platform_string(
      "http://tizen.org/feature/platform.version", &tizen_version);
  if (result == SYSTEM_INFO_ERROR_NONE) {
    tizen_version_str.assign(tizen_version, 3);
    free(tizen_version);
  }

  result = system_info_get_platform_string(
      "http://tizen.org/system/platform.name", &platform_name);
  if (result == SYSTEM_INFO_ERROR_NONE) {
    platform_name_str.assign(platform_name);
    free(platform_name);
  }

  result = system_info_get_platform_string(
      "http://tizen.org/system/model_name", &device_model);
  if (result == SYSTEM_INFO_ERROR_NONE) {
    device_model_str.assign(device_model);
    device_model_str.insert(0, "SAMSUNG ");
    free(device_model);
  }

  return std::string(kOSTypeLinux) + ("; ") +
      platform_name_str + (" ") + tizen_version_str + ("; ") + device_model_str;
#else
  return kOSTypeLinux;
#endif
}

/* LCOV_EXCL_START */
void VersionInfo::UpdateUserAgentWithAppName(const std::string& name) {
  if (name.empty())
    return;
  product_name_ = name;
  user_agent_ = CreateUserAgent();
}

void VersionInfo::UpdateUserAgent(const std::string& user_agent) {
  if (user_agent.empty())
    return;
  user_agent_ = user_agent;
}
/* LCOV_EXCL_STOP */

std::string VersionInfo::VersionForUserAgent() const {  // LCOV_EXCL_LINE
  // Some WebRTC web-sites need the Chrome version number to check
  // if the browser supports the WebRTC feature.
  // TODO(max koo): Do we need to open our real version number
  // or just use Chrome/aa.bb.cc.dd as Chromium/Chrome do?
  LOG(INFO) << "CHROMIUM_VERSION:" << CHROMIUM_VERSION;
  return std::string("Chrome/") + CHROMIUM_VERSION;
}

std::string VersionInfo::DefaultUserAgent() const {
  char* override_ua = getenv("CHROMIUM_EFL_OVERRIDE_UA_STRING");
  if (override_ua)
    return override_ua;  // LCOV_EXCL_LINE

  return user_agent_;
}

} //namespace EflWebView
