// Copyright (c) 2014,2015 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WRT_URL_REQUEST_INTERCEPTOR
#define WRT_URL_REQUEST_INTERCEPTOR

#include "base/memory/ref_counted.h"
#include "net/url_request/url_request_interceptor.h"

class GURL;
class DynamicPlugin;

namespace base {
class TaskRunner;
}

namespace net {

class NetworkDelegate;
class URLRequestJob;

class WrtUrlRequestInterceptor : public URLRequestInterceptor {
 public:
  explicit WrtUrlRequestInterceptor(
      const scoped_refptr<base::TaskRunner>& file_task_runner,
      const std::string& tizen_app_id,
      DynamicPlugin* dynamic_plugin);
  ~WrtUrlRequestInterceptor() override {}  // LCOV_EXCL_LINE
  URLRequestJob* MaybeInterceptRequest(
      URLRequest* request,
      NetworkDelegate* network_delegate) const override;

 private:
  bool GetWrtParsedUrl(const GURL& url, GURL& parsed_url) const;

  const scoped_refptr<base::TaskRunner> file_task_runner_;
  const std::string tizen_app_id_;
  DynamicPlugin* dynamic_plugin_;
  DISALLOW_COPY_AND_ASSIGN(WrtUrlRequestInterceptor);
};

}  // namespace net

#endif  // WRT_URL_REQUEST_INTERCEPTOR
