// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "url_request_interceptor_efl.h"

#include "base/supports_user_data.h"
#include "content/public/browser/browser_thread.h"
#include "network_delegate_efl.h"
#include "net/http/http_request_headers.h"
#include "net/url_request/url_request_job.h"
#include "private/ewk_intercept_request_private.h"
#include "url/gurl.h"
#include "url_request_job_efl.h"

class WebContents;

namespace content {

namespace {

const void* const kRequestAlreadyHasJobDataKey = &kRequestAlreadyHasJobDataKey;

}  // namespace

URLRequestInterceptorEFL::URLRequestInterceptorEFL() {}

URLRequestInterceptorEFL::~URLRequestInterceptorEFL() {}

net::URLRequestJob* URLRequestInterceptorEFL::MaybeInterceptRequest(
    net::URLRequest* request,
    net::NetworkDelegate* network_delegate) const {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  // MaybeInterceptRequest can be called multiple times for the same request.
  if (request->GetUserData(kRequestAlreadyHasJobDataKey))
    return nullptr;

  auto network_delegate_efl =
      static_cast<net::NetworkDelegateEfl*>(network_delegate);

  if (!network_delegate_efl->HasInterceptRequestCallback())
    return nullptr;

  /* LCOV_EXCL_START */
  auto intercept_request = new _Ewk_Intercept_Request(request);
  network_delegate_efl->RunInterceptRequestCallback(intercept_request);
  intercept_request->callback_ended();
  if (intercept_request->is_ignored()) {
    delete intercept_request;
    return nullptr;
  }

  GURL referrer(request->referrer());
  if (referrer.is_valid() &&
      (!request->is_pending() || request->is_redirecting())) {
    request->SetExtraRequestHeaderByName(net::HttpRequestHeaders::kReferer,
                                         referrer.spec(), true);
  }
  request->SetUserData(
      kRequestAlreadyHasJobDataKey,
      std::unique_ptr<base::SupportsUserData::Data>(
          new base::SupportsUserData::Data()));

  return new URLRequestJobEFL(request, network_delegate,
                              intercept_request);
  /* LCOV_EXCL_STOP */
}

}  // namespace content
