// Copyright 2018 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMIUM_IMPL_NET_URL_REQUEST_DVB_PROTOCOL_HANDLER_H_
#define CHROMIUM_IMPL_NET_URL_REQUEST_DVB_PROTOCOL_HANDLER_H_

#include "base/macros.h"
#include "net/url_request/url_request_job_factory.h"

namespace net {

class URLRequestJob;

// Implements a ProtocolHandler for Dvb jobs.
class DvbProtocolHandler: public URLRequestJobFactory::ProtocolHandler {
 public:
  DvbProtocolHandler() = default;
  URLRequestJob* MaybeCreateJob(
      URLRequest* request,
      NetworkDelegate* network_delegate) const override;
  bool IsSafeRedirectTarget(const GURL& location) const override;

 private:
  DISALLOW_COPY_AND_ASSIGN(DvbProtocolHandler);
};
}  // namespace net

#endif // CHROMIUM_IMPL_NET_URL_REQUEST_DVB_PROTOCOL_HANDLER_H_
