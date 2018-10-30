// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mime_override_manager_efl.h"
#include "base/memory/singleton.h"

MimeOverrideManagerEfl* MimeOverrideManagerEfl::GetInstance() {
  return base::Singleton<MimeOverrideManagerEfl>::get();
}

/* LCOV_EXCL_START */
void MimeOverrideManagerEfl::PushOverriddenMime(
    const std::string& url_spec, const std::string& new_mime_type) {
  base::AutoLock locker(lock_);
  MimeOverrideMap::iterator it = mime_override_map_.find(url_spec);
  if (it == mime_override_map_.end()) {
    mime_override_map_.insert(std::pair<std::string,
                                        std::string>(url_spec, new_mime_type));
  } else {
    it->second.assign(new_mime_type);
  }
}
/* LCOV_EXCL_STOP */

bool MimeOverrideManagerEfl::PopOverriddenMime(const std::string& url_spec,
                                               std::string& new_mime_type) {
  base::AutoLock locker(lock_);
  bool result = false;
  MimeOverrideMap::iterator it = mime_override_map_.find(url_spec);
  if (it != mime_override_map_.end()) {
    /* LCOV_EXCL_START */
    new_mime_type.assign(it->second);
    mime_override_map_.erase(it);
    result = true;
    /* LCOV_EXCL_STOP */
  }
  return result;
}
