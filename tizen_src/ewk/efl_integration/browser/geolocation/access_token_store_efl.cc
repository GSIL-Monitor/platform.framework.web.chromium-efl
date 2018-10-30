// Copyright 2013 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser/geolocation/access_token_store_efl.h"

#include "base/bind.h"
#include "base/message_loop/message_loop.h"
#include "base/strings/utf_string_conversions.h"
#include "content/public/browser/browser_thread.h"
#include "eweb_context.h"
#include "ewk_global_data.h"
#include "private/ewk_context_private.h"

namespace device {

AccessTokenStoreEfl::AccessTokenStoreEfl()
    : system_request_context_(NULL)
{
}

AccessTokenStoreEfl::~AccessTokenStoreEfl()
{
}

void AccessTokenStoreEfl::LoadAccessTokens(const LoadAccessTokensCallback& callback)
{
  content::BrowserThread::PostTaskAndReply(content::BrowserThread::UI,
                                  FROM_HERE,
                                  base::Bind(&AccessTokenStoreEfl::GetRequestContextOnUIThread,
                                             this),
                                  base::Bind(&AccessTokenStoreEfl::RespondOnOriginatingThread,
                                             this,
                                             callback));
}

void AccessTokenStoreEfl::GetRequestContextOnUIThread()
{
  system_request_context_ = EwkGlobalData::GetInstance()->GetSystemRequestContextGetter();
}

void AccessTokenStoreEfl::RespondOnOriginatingThread(const LoadAccessTokensCallback& callback)
{
  AccessTokenMap access_token_map;
  callback.Run(access_token_map, system_request_context_);
}

void AccessTokenStoreEfl::SaveAccessToken(const GURL& /*server_url*/, const base::string16& /*access_token*/)
{
}

}  // namespace device
