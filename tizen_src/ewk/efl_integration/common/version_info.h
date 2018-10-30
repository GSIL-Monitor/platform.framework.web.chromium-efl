// Copyright 2013 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef VERSION_INFO_H_
#define VERSION_INFO_H_

#include <string>
#include "base/macros.h"

namespace EflWebView {

class VersionInfo {
 public:
  // The possible channels for an installation, from most fun to most stable.
  enum Channel {
    CHANNEL_UNKNOWN = 0,  // Probably blue
    CHANNEL_CANARY,       // Yellow
    CHANNEL_DEV,          // Technicolor
    CHANNEL_BETA,         // Rainbow
    CHANNEL_STABLE        // Full-spectrum
  };

  static VersionInfo* GetInstance();
  static void DeleteInstance();

  // Default user agent
  std::string DefaultUserAgent() const;
  // Get chromium version number
  std::string GetVersionNumber() const { return CHROMIUM_VERSION; }
  // E.g. "Chrome/a.b.c.d"
  std::string VersionForUserAgent() const;
  // OS type. E.g. "Windows", "Linux", "FreeBSD", ...
  std::string OSType() const;

  std::string AppName() const { return product_name_; }

  void UpdateUserAgentWithAppName(const std::string& name);

  // To update the default user agent with restored (Session data) User Agent.
  void UpdateUserAgent(const std::string& user_agent);

  // Returns the channel for the installation. In branded builds, this will be
  // CHANNEL_STABLE, CHANNEL_BETA, CHANNEL_DEV, or CHANNEL_CANARY. In unbranded
  // builds, or in branded builds when the channel cannot be determined, this
  // will be CHANNEL_UNKNOWN.
  static Channel GetChannel()
  { return CHANNEL_DEV; }

 private:
  VersionInfo();
  ~VersionInfo() { }

  std::string CreateUserAgent() const;
  std::string product_name_;
  std::string system_info_;
  std::string user_agent_;
  static VersionInfo* version_info_;

  DISALLOW_COPY_AND_ASSIGN(VersionInfo);
};

}  // namespace EflWebView

#endif  // VERSION_INFO_H_
