// Copyright 2018 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TIZEN_SRC_EWK_EFL_INTEGRATION_NETWORK_HINTS_BROWSER_MESSAGE_FILTER_EFL_H_
#define TIZEN_SRC_EWK_EFL_INTEGRATION_NETWORK_HINTS_BROWSER_MESSAGE_FILTER_EFL_H_

#include "base/macros.h"
#include "content/public/browser/browser_message_filter.h"

class GURL;

namespace network_hints {
struct LookupRequest;
};

namespace browser_net {
class PredictorEfl;
}

namespace content {
class BrowserContextEfl;
}

// Simple browser-side handler for DNS prefetch and preconnect requests.
// Passes prefetch requests to the provided net::HostResolver.
// Handles preconnect requests on io thread.
// Each renderer process requires its own filter.
class NetworkHintsMessageFilterEfl : public content::BrowserMessageFilter {
 public:
  explicit NetworkHintsMessageFilterEfl(
      content::BrowserContextEfl* browser_context_efl);

  // content::BrowserMessageFilter implementation:
  bool OnMessageReceived(const IPC::Message& message) override;

 private:
  ~NetworkHintsMessageFilterEfl() override;

  void OnDnsPrefetch(const network_hints::LookupRequest& lookup_request);
  void OnPreconnect(const GURL& url, bool allow_credentials, int count);

  // The Predictor for the associated Profile. It is stored so that it can be
  // used on the IO thread.
  browser_net::PredictorEfl* predictor_efl_;

  DISALLOW_COPY_AND_ASSIGN(NetworkHintsMessageFilterEfl);
};

#endif  // TIZEN_SRC_EWK_EFL_INTEGRATION_NETWORK_HINTS_BROWSER_MESSAGE_FILTER_EFL_H_
