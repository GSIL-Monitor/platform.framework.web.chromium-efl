// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "resource_dispatcher_host_delegate_efl.h"

#include "browser/login_delegate_efl.h"
#include "browser/mime_override_manager_efl.h"
#include "browser_context_efl.h"
#include "content/public/browser/resource_request_info.h"
#include "content/public/common/resource_response.h"
#include "content/public/common/resource_type.h"
#include "net/url_request/url_request.h"
#include "resource_throttle_efl.h"
#include "tizen/system_info.h"

namespace content {


void ResourceDispatcherHostDelegateEfl::RequestBeginning(
    net::URLRequest* request,
    content::ResourceContext* resource_context,
    content::AppCacheService* appcache_service,
    content::ResourceType resource_type,
    std::vector<std::unique_ptr<content::ResourceThrottle>>* throttles) {
  // Add throttle for http, https and file protocol.
  bool is_resource_type_frame = true;
  if (IsMobileProfile() || IsWearableProfile())
    is_resource_type_frame = IsResourceTypeFrame(resource_type);

  if ((request->url().SchemeIsHTTPOrHTTPS() || request->url().SchemeIsFile()) &&
      is_resource_type_frame)
    throttles->push_back(
        base::MakeUnique<ResourceThrottleEfl>(*request));

  // policy response and custom headers should be probably only for HTTP and
  // HTTPs
  if (request->url().SchemeIsHTTPOrHTTPS()) {
    BrowserContextEfl::ResourceContextEfl* resource_context_efl =
        static_cast<BrowserContextEfl::ResourceContextEfl*>(resource_context);
    if (!resource_context_efl)
      return;  // LCOV_EXCL_LINE

    HTTPCustomHeadersEflMap header_map =
        resource_context_efl->GetHTTPCustomHeadersEflMap();
    for (HTTPCustomHeadersEflMap::iterator it = header_map.begin();
         it != header_map.end(); ++it)
      request->SetExtraRequestHeaderByName(it->first, it->second, true);
  }
}

/* LCOV_EXCL_START */
ResourceDispatcherHostLoginDelegate*
ResourceDispatcherHostDelegateEfl::CreateLoginDelegate(
    net::AuthChallengeInfo* auth_info,
    net::URLRequest* request) {
  return new LoginDelegateEfl(auth_info, request);
}
/* LCOV_EXCL_STOP */

bool ResourceDispatcherHostDelegateEfl::ShouldOverrideMimeType(
    const GURL& url,
    std::string& new_mime_type) const {
  MimeOverrideManagerEfl* mime_override_manager_efl =
      MimeOverrideManagerEfl::GetInstance();
  return mime_override_manager_efl->PopOverriddenMime(url.spec(),
                                                      new_mime_type);
}

}  // namespace content
