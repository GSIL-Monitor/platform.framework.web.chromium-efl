// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "http_user_agent_settings_efl.h"

#include "content/common/content_client_export.h"
#include "content/public/browser/content_browser_client.h"

HttpUserAgentSettingsEfl::HttpUserAgentSettingsEfl() {
}

HttpUserAgentSettingsEfl::~HttpUserAgentSettingsEfl() {
}

std::string HttpUserAgentSettingsEfl::GetAcceptLanguage() const {
  return content::GetContentClientExport()->browser()->GetAcceptLangs(nullptr);
}

std::string HttpUserAgentSettingsEfl::GetUserAgent() const {
  return content::GetContentClientExport()->GetUserAgent();
}
