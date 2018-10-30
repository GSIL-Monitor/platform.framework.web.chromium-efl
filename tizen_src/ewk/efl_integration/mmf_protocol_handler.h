// Copyright 2018 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TIZEN_SRC_EWK_EFL_INTEGRATION_MMF_PROTOCOL_HANDLER_H_
#define TIZEN_SRC_EWK_EFL_INTEGRATION_MMF_PROTOCOL_HANDLER_H_

#include "base/macros.h"
#include "net/url_request/url_request_job_factory.h"

namespace net {

class URLRequestJob;

// Implements a ProtocolHandler for Mmf jobs.
class MmfProtocolHandler : public URLRequestJobFactory::ProtocolHandler {
 public:
  MmfProtocolHandler() = default;
  URLRequestJob* MaybeCreateJob(
      URLRequest* request,
      NetworkDelegate* network_delegate) const override;
  bool IsSafeRedirectTarget(const GURL& location) const override;

 private:
  DISALLOW_COPY_AND_ASSIGN(MmfProtocolHandler);
};

}  // namespace net

#endif  // TIZEN_SRC_EWK_EFL_INTEGRATION_MMF_PROTOCOL_HANDLER_H_
