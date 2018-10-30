// Copyright 2013 The Chromium Authors. All rights reserved.
// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EWK_EFL_INTEGRATION_URL_REQUEST_INTERCEPTOR_EFL_H_
#define EWK_EFL_INTEGRATION_URL_REQUEST_INTERCEPTOR_EFL_H_

#include "net/url_request/url_request_interceptor.h"
#include "public/ewk_context.h"

namespace net {
class URLRequest;
class URLRequestJob;
class NetworkDelegate;
}  // namespace net

struct _Ewk_Intercept_Request;

namespace content {

class URLRequestInterceptorEFL : public net::URLRequestInterceptor {
 public:
  URLRequestInterceptorEFL();
  ~URLRequestInterceptorEFL() override;

  net::URLRequestJob* MaybeInterceptRequest(
      net::URLRequest* request,
      net::NetworkDelegate* network_delegate) const override;

 private:
  DISALLOW_COPY_AND_ASSIGN(URLRequestInterceptorEFL);
};

}  // namespace content

#endif  // EWK_EFL_INTEGRATION_URL_REQUEST_INTERCEPTOR_EFL_H_
