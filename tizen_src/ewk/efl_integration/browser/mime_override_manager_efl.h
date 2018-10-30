// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MIME_OVERRIDE_MANAGER_EFL_H_
#define MIME_OVERRIDE_MANAGER_EFL_H_

#include <map>
#include <utility>

#include "base/synchronization/lock.h"

namespace base {
template <typename T> struct DefaultSingletonTraits;
}
typedef std::map<std::string, std::string> MimeOverrideMap;

class MimeOverrideManagerEfl {
 public:
  static MimeOverrideManagerEfl* GetInstance();

  // Adds new mime to be overridden. Thread safe.
  void PushOverriddenMime(const std::string& url_spec,
                          const std::string& new_mime_type);

  // Checks if mime should be overridden and gets new mime.
  // Removes the mime that shall be overridden. Thread safe.
  bool PopOverriddenMime(const std::string& url_spec,
                         std::string& new_mime_type);

 private:
  MimeOverrideManagerEfl() {}

  friend struct base::DefaultSingletonTraits<MimeOverrideManagerEfl>;

  base::Lock lock_;
  MimeOverrideMap mime_override_map_;

  DISALLOW_COPY_AND_ASSIGN(MimeOverrideManagerEfl);
};

#endif // MIME_OVERRIDE_MANAGER_EFL_H_
