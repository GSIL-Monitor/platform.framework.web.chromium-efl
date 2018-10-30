// Copyright 2013 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ACCESS_TOKEN_STORE_EFL_H_
#define ACCESS_TOKEN_STORE_EFL_H_

#include "browser_context_efl.h"
#include "device/geolocation/access_token_store.h"

namespace content {
class BrowserContextEfl;
}

namespace device {

class AccessTokenStoreEfl : public device::AccessTokenStore {
 public:
  AccessTokenStoreEfl();

  // AccessTokenStore
  virtual void LoadAccessTokens(const LoadAccessTokensCallback& callback) override;
  virtual void SaveAccessToken(const GURL& server_url, const base::string16& access_token) override;

 private:
  virtual ~AccessTokenStoreEfl();

  void GetRequestContextOnUIThread();
  void RespondOnOriginatingThread(const LoadAccessTokensCallback& callback);

  net::URLRequestContextGetter* system_request_context_;

  DISALLOW_COPY_AND_ASSIGN(AccessTokenStoreEfl);
};

}  // namespace device

#endif  // ACCESS_TOKEN_STORE_EFL_H_

