// Copyright 2015 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WebContentBlockElementHiding_h
#define WebContentBlockElementHiding_h

#include "public/platform/WebCommon.h"

#include <string>
#include <vector>

namespace blink {

class BLINK_EXPORT WebContentBlockElementHiding {
 public:
  static int GetNumberOfBlockedElements();
  static void SetEnabled(const bool is_default, const bool is_enabled);
  static void SetRulesForCurrentPage(
      const bool is_default,
      const std::vector<std::string>& selectors,
      const std::vector<std::string>& whitelist_selectors);
  static void SetGlobalRules(const bool is_default,
                             const std::vector<std::string>& selectors);
};

}  // namespace blink

#endif  // WebContentBlockElementHiding_h
