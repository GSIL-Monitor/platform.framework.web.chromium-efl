// Copyright (c) 2018 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "special_storage_policy_efl.h"

#include "base/stl_util.h"
#include "content/public/browser/browser_thread.h"

using content::BrowserThread;

SpecialStoragePolicyEfl::SpecialStoragePolicyEfl() {}

SpecialStoragePolicyEfl::~SpecialStoragePolicyEfl() {}

bool SpecialStoragePolicyEfl::IsStorageUnlimited(const GURL& origin) {
  base::AutoLock locker(lock_);
  if (!base::ContainsKey(unlimited_, origin))
    return false;
  return unlimited_.find(origin)->second;
}

#if defined(OS_TIZEN)
bool SpecialStoragePolicyEfl::RequestUnlimitedStoragePolicy(
    const GURL& origin,
    int64_t quota,
    const QuotaExceededReplyCallback& callback) {
  {
    base::AutoLock locker(lock_);
    if (base::ContainsKey(unlimited_, origin) ||
        quota_exceeded_callback_.is_null())
      return false;
  }
  quota_exceeded_callback_.Run(origin, quota);
  quota_exceeded_reply_callback_ = callback;
  return true;
}
#endif

void SpecialStoragePolicyEfl::SetQuotaExceededCallback(
    const QuotaExceededCallback& callback) {
  quota_exceeded_callback_ = callback;
}

void SpecialStoragePolicyEfl::SetUnlimitedStoragePolicy(const GURL& origin,
                                                        bool allow) {
  if (!BrowserThread::CurrentlyOn(BrowserThread::IO)) {
    BrowserThread::PostTask(
        BrowserThread::IO, FROM_HERE,
        base::Bind(&SpecialStoragePolicyEfl::SetUnlimitedStoragePolicy, this,
                   origin, allow));
    return;
  }
  bool policy_changed = IsStorageUnlimited(origin) != allow;
  LOG(INFO) << __func__ << "() allow:" << allow
            << ", changed:" << policy_changed;
  {
    base::AutoLock locker(lock_);
    unlimited_[origin] = allow;
  }
  if (policy_changed) {
    if (allow)
      NotifyGranted(origin, SpecialStoragePolicy::STORAGE_UNLIMITED);
    else
      NotifyRevoked(origin, SpecialStoragePolicy::STORAGE_UNLIMITED);
  }
  if (!quota_exceeded_reply_callback_.is_null()) {
    quota_exceeded_reply_callback_.Run(policy_changed);
    quota_exceeded_reply_callback_.Reset();
  }
}
