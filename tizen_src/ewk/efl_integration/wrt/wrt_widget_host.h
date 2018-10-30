// Copyright (c) 2014,2015 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WRT_HOST_H
#define WRT_HOST_H

#include <map>
#include <string>

#include "base/atomic_sequence_num.h"
#include "base/callback.h"
#include "content/public/browser/browser_message_filter.h"

namespace net {
class URLRequest;
}

class GURL;
class Ewk_Wrt_Message_Data;

class WrtWidgetHostMessageFilter;

class WrtWidgetHost {  // LCOV_EXCL_LINE
 public:
  static WrtWidgetHost* Get();

  void GetUrlForRequest(net::URLRequest* request,
                        base::Callback<void(const GURL&)> callback);

  void SendWrtMessage(const Ewk_Wrt_Message_Data& message);

  bool InWrt() const;

  std::string TizenAppId() const;

  /* LCOV_EXCL_START */
  void EnablePowerLock(bool enable) {
    enable_power_lock_ = enable;
  }
  /* LCOV_EXCL_STOP */
  bool IsPowerLockEnabled() const { return enable_power_lock_; }

 private:
  friend class WrtWidgetHostMessageFilter;

  typedef std::map<int, base::Callback<void(const GURL&)> > callbacks_type;

  WrtWidgetHost();

  void OnUrlRetrieved(int callback_id, const GURL& url);

  scoped_refptr<WrtWidgetHostMessageFilter> message_filter_;
  base::AtomicSequenceNumber callback_id_generator_;
  callbacks_type callbacks_;

  bool enable_power_lock_;

  DISALLOW_COPY_AND_ASSIGN(WrtWidgetHost);
};


#endif
