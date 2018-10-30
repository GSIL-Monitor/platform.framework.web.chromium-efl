// Copyright 2015 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ContentBlockElementHiding_h
#define ContentBlockElementHiding_h

#include "core/CoreExport.h"
#include "core/css/StyleSheetContents.h"
#include "platform/heap/Handle.h"
#include "platform/wtf/text/WTFString.h"

namespace blink {

class RuleSet;

class CORE_EXPORT ContentBlockElementHiding
    : public GarbageCollectedFinalized<ContentBlockElementHiding> {
 public:
  enum Type { kDefault, kExtension };

  using RuleList = HeapVector<Member<RuleSet>, 3>;

  RuleList OrderedElementHidingRules();

  void SetEnabled(bool);

  void SetRulesForCurrentPage(Vector<String>&& selectors,
                              Vector<String>&& whitelist_selectors);
  void SetGlobalRules(Vector<String>&& selectors);

  void IncreaseNumberOfBlockedElements();
  void ResetNumberOfBlockedElements();

  int GetNumberOfBlockedElements() const { return number_of_blocked_elements_; }

  static ContentBlockElementHiding& Instance(Type = kExtension);

  DECLARE_TRACE();

 private:
  ContentBlockElementHiding(Type = kExtension);

  void ShrinkRules();
  void ReleasePageStyle();
  void ReleaseGlobalStyle();

  Vector<String> global_rules_;
  Vector<String> hiding_rules_;
  Vector<String> whitelist_rules_;

  Member<RuleSet> global_style_;
  Member<RuleSet> hiding_style_;
  Member<RuleSet> whitelist_style_;

  // Apparently a RuleSet becomes dangling (and crashes when accessed) if the
  // StyleSheetContents that was used to initialize it goes away. That is the
  // only reason we need to hold on to both.
  Member<StyleSheetContents> global_sheet_;
  Member<StyleSheetContents> hiding_sheet_;
  Member<StyleSheetContents> whitelist_sheet_;

  Type type_;

  bool is_enabled_;
  int number_of_blocked_elements_;
};

}  // namespace blink

#endif  // ContentBlockElementHiding_h
