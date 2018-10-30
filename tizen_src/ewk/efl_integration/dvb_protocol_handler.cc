// Copyright 2018 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "dvb_protocol_handler.h"

#include "net/base/net_errors.h"
#include "net/url_request/url_request_error_job.h"

namespace net {

URLRequestJob* DvbProtocolHandler::MaybeCreateJob(
    URLRequest* request, NetworkDelegate* network_delegate) const {
  return new URLRequestErrorJob(request, network_delegate,
                                ERR_UNKNOWN_URL_SCHEME);
}

bool DvbProtocolHandler::IsSafeRedirectTarget(const GURL& location) const {
  return false;
}

}  // namespace net
