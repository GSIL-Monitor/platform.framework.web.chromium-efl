// Copyright 2018 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser/network_hints_message_filter_efl.h"

#include "browser/net/predictor_efl.h"
#include "browser_context_efl.h"
#include "components/network_hints/common/network_hints_common.h"
#include "components/network_hints/common/network_hints_messages.h"
#include "url/gurl.h"

NetworkHintsMessageFilterEfl::NetworkHintsMessageFilterEfl(
    content::BrowserContextEfl* browser_context_efl)
    : content::BrowserMessageFilter(NetworkHintsMsgStart),
      predictor_efl_(browser_context_efl->GetNetworkPredictorEfl()) {}

NetworkHintsMessageFilterEfl::~NetworkHintsMessageFilterEfl() {}

bool NetworkHintsMessageFilterEfl::OnMessageReceived(
    const IPC::Message& message) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(NetworkHintsMessageFilterEfl, message)
  IPC_MESSAGE_HANDLER(NetworkHintsMsg_DNSPrefetch, OnDnsPrefetch)
  IPC_MESSAGE_HANDLER(NetworkHintsMsg_Preconnect, OnPreconnect)
  IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()
  return handled;
}

void NetworkHintsMessageFilterEfl::OnDnsPrefetch(
    const network_hints::LookupRequest& lookup_request) {
  if (predictor_efl_)
    predictor_efl_->DnsPrefetchList(lookup_request.hostname_list);
}

/* LCOV_EXCL_START */
void NetworkHintsMessageFilterEfl::OnPreconnect(const GURL& url,
                                                bool allow_credentials,
                                                int count) {
  if (count < 1) {
    LOG(WARNING) << "NetworkHintsMsg_Preconnect IPC with invalid count: "
                 << count;
    return;
  }
  if (predictor_efl_ && url.is_valid() && url.has_host() && url.has_scheme() &&
      url.SchemeIsHTTPOrHTTPS()) {
    predictor_efl_->PreconnectUrl(
        url, GURL(), chrome_browser_net::UrlInfo::EARLY_LOAD_MOTIVATED,
        allow_credentials, count);
  }
}
/* LCOV_EXCL_STOP */
