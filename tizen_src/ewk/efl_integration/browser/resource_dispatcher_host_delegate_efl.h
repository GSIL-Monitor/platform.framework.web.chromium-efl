// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef RESOURCE_DISPATCHER_HOST_DELEGATE_EFL_H
#define RESOURCE_DISPATCHER_HOST_DELEGATE_EFL_H

#include "base/compiler_specific.h"
#include "content/public/browser/resource_dispatcher_host_delegate.h"

namespace net {
class AuthChallengeInfo;
class URLRequest;
}

namespace content {

class ResourceDispatcherHostLoginDelegate;

class ResourceDispatcherHostDelegateEfl
    : public ResourceDispatcherHostDelegate {
 public:
  ResourceDispatcherHostDelegateEfl() {}

  // ResourceDispatcherHostDelegate implementation.
  //
  // Called after ShouldBeginRequest to allow the embedder to add resource
  // throttles.
  void RequestBeginning(net::URLRequest* request,
                        content::ResourceContext* resource_context,
                        content::AppCacheService* appcache_service,
                        content::ResourceType resource_type,
                        std::vector<std::unique_ptr<content::ResourceThrottle>>*
                            throttles) override;

  // Create login delegate.
  content::ResourceDispatcherHostLoginDelegate* CreateLoginDelegate(
      net::AuthChallengeInfo* auth_info,
      net::URLRequest* request) override;


  // Returns true if mime type should be overridden, otherwise returns false.
  bool ShouldOverrideMimeType(const GURL& url,
                              std::string& new_mime_type) const override;

 private:
  DISALLOW_COPY_AND_ASSIGN(ResourceDispatcherHostDelegateEfl);

};

}  // namespace net

#endif  // RESOURCE_DISPATCHER_HOST_DELEGATE_EFL_H
