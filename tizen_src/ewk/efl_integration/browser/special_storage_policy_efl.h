// Copyright (c) 2018 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SPECIAL_STORAGE_POLICY_EFL_H_
#define SPECIAL_STORAGE_POLICY_EFL_H_

#include <map>

#include "base/callback.h"
#include "base/synchronization/lock.h"
#include "storage/browser/quota/special_storage_policy.h"
#include "url/gurl.h"

class SpecialStoragePolicyEfl : public storage::SpecialStoragePolicy {
 public:
  typedef base::Callback<void(const GURL& origin, int64_t quota)>
      QuotaExceededCallback;
  SpecialStoragePolicyEfl();

  // storage::SpecialStoragePolicy implementation.
  bool IsStorageProtected(const GURL& origin) override { return false; }
  bool IsStorageUnlimited(const GURL& origin) override;
  bool IsStorageSessionOnly(const GURL& origin) override { return false; }
  bool HasIsolatedStorage(const GURL& origin) override { return false; }
  bool HasSessionOnlyOrigins() override { return false; }
  bool IsStorageDurable(const GURL& origin) override { return false; }

// for quota exceeded callback
#if defined(OS_TIZEN)
  bool RequestUnlimitedStoragePolicy(
      const GURL& origin,
      int64_t quota,
      const QuotaExceededReplyCallback& callback) override;
#endif
  void SetQuotaExceededCallback(const QuotaExceededCallback& callback);
  void SetUnlimitedStoragePolicy(const GURL& origin, bool allow);

 protected:
  ~SpecialStoragePolicyEfl() override;

 private:
  base::Lock lock_;
  std::map<GURL, bool> unlimited_;
  QuotaExceededCallback quota_exceeded_callback_;
  QuotaExceededReplyCallback quota_exceeded_reply_callback_;
};

#endif  // SPECIAL_STORAGE_POLICY_EFL_H_
