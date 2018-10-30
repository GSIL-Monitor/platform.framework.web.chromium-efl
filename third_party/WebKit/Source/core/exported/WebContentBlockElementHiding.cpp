// Copyright 2015 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "public/web/WebContentBlockElementHiding.h"

#include "core/dom/content_block/ContentBlockElementHiding.h"
#include "platform/wtf/Vector.h"
#include "platform/wtf/text/WTFString.h"

namespace blink {
namespace {

inline Vector<String> ToWTF(const std::vector<std::string>& vector) {
  Vector<String> result;
  result.ReserveInitialCapacity(vector.size());
  for (const std::string& str : vector) {
    result.push_back(String::FromUTF8(str.data(), str.length()));
  }
  return result;
}

}  // namespace

int WebContentBlockElementHiding::GetNumberOfBlockedElements() {
  return ContentBlockElementHiding::Instance().GetNumberOfBlockedElements();
}

void WebContentBlockElementHiding::SetEnabled(const bool is_default,
                                              const bool is_enabled) {
  ContentBlockElementHiding::Instance(
      is_default ? ContentBlockElementHiding::kDefault
                 : ContentBlockElementHiding::kExtension)
      .SetEnabled(is_enabled);
}

void WebContentBlockElementHiding::SetRulesForCurrentPage(
    const bool is_default,
    const std::vector<std::string>& selectors,
    const std::vector<std::string>& whitelist_selectors) {
  ContentBlockElementHiding::Instance(
      is_default ? ContentBlockElementHiding::kDefault
                 : ContentBlockElementHiding::kExtension)
      .SetRulesForCurrentPage(ToWTF(selectors), ToWTF(whitelist_selectors));
}

void WebContentBlockElementHiding::SetGlobalRules(
    const bool is_default,
    const std::vector<std::string>& selectors) {
  ContentBlockElementHiding::Instance(
      is_default ? ContentBlockElementHiding::kDefault
                 : ContentBlockElementHiding::kExtension)
      .SetGlobalRules(ToWTF(selectors));
}

}  // namespace blink
