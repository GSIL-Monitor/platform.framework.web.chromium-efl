// Copyright 2015 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/dom/content_block/ContentBlockElementHiding.h"

#include "core/css/MediaQueryEvaluator.h"
#include "core/css/RuleSet.h"
#include "core/dom/Element.h"
#include "platform/wtf/text/StringBuffer.h"

namespace blink {
namespace {
// We have many selectors for just one rule (i.e. "display: none"). Blink is not
// optimized for this scenario and performs poorly. We break selectors into
// groups to mitigate this issue.
// TODO: this should be chosen on an empirical basis for best performance.
// Current value (20) is the same that is used by the ABP extension.
static const size_t kSelectorGroupSize = 20;

void MakeRules(const Vector<String>& selectors,
               const String& rule_body,
               Vector<String>& result) {
  result.clear();
  result.ReserveCapacity(selectors.size() / kSelectorGroupSize +
                         ((selectors.size() % kSelectorGroupSize) ? 1 : 0));
  size_t selector_index = 0;
  while (selector_index < selectors.size()) {
    const size_t group_size =
        std::min(kSelectorGroupSize, selectors.size() - selector_index);
    unsigned rule_length = 0;
    for (size_t k = selector_index; k < selector_index + group_size; ++k) {
      rule_length += selectors[k].length();
    }
    rule_length += group_size - 1;  // commas
    rule_length += rule_body.length();

    StringBuffer<LChar> buffer(rule_length);
    LChar* characters = buffer.Characters();
    for (size_t k = selector_index; k < selector_index + group_size; ++k) {
      const String& selector = selectors[k];
      memcpy(characters, selector.Characters8(), selector.length());
      characters += selector.length();
      if (k < selector_index + group_size - 1) {
        *characters++ = static_cast<LChar>(',');
      }
    }
    DCHECK(characters + rule_body.length() ==
           buffer.Characters() + buffer.length());
    memcpy(characters, rule_body.Characters8(), rule_body.length());

    result.push_back(buffer.Release());
    selector_index += group_size;
  }
}

const MediaQueryEvaluator& ScreenMediaQueryEvaluator() {
  DEFINE_STATIC_LOCAL(Persistent<MediaQueryEvaluator>, screen_evaluator,
                      (new MediaQueryEvaluator("screen")));
  return *screen_evaluator;
}

RuleSet* ParseRules(const Vector<String>& rules,
                    Member<StyleSheetContents>& style_sheet) {
  if (rules.IsEmpty())
    return nullptr;

  // Use UA-mode not to manipulate use-counters.
  const CSSParserMode parser_mode = kUASheetMode;
  style_sheet =
      StyleSheetContents::Create(CSSParserContext::Create(parser_mode));
  for (const String& rule : rules) {
    style_sheet->ParseString(rule);
  }

  RuleSet* result = RuleSet::Create();
  result->AddRulesFromSheet(style_sheet.Get(), ScreenMediaQueryEvaluator());
  return result;
}

}  // namespace

void ContentBlockElementHiding::SetEnabled(bool enabled) {
  is_enabled_ = enabled;
  if (!enabled) {
    ShrinkRules();
    ReleasePageStyle();
    ReleaseGlobalStyle();
  }
}

ContentBlockElementHiding& ContentBlockElementHiding::Instance(Type type) {
  if (kDefault == type) {
    DEFINE_STATIC_LOCAL(Persistent<ContentBlockElementHiding>,
                        s_default_instance,
                        (new ContentBlockElementHiding(kDefault)));
    return *s_default_instance;
  }
  DEFINE_STATIC_LOCAL(Persistent<ContentBlockElementHiding>,
                      s_extension_instance,
                      (new ContentBlockElementHiding(kExtension)));
  return *s_extension_instance;
}

ContentBlockElementHiding::ContentBlockElementHiding(Type type)
    : type_(type), is_enabled_(false), number_of_blocked_elements_(0) {}

void ContentBlockElementHiding::ShrinkRules() {
  global_rules_.clear();
  hiding_rules_.clear();
  whitelist_rules_.clear();
}

void ContentBlockElementHiding::ReleasePageStyle() {
  hiding_style_ = nullptr;
  hiding_sheet_ = nullptr;
  whitelist_style_ = nullptr;
  whitelist_sheet_ = nullptr;
}

void ContentBlockElementHiding::ReleaseGlobalStyle() {
  global_style_ = nullptr;
  global_sheet_ = nullptr;
}

void ContentBlockElementHiding::SetRulesForCurrentPage(
    Vector<String>&& selectors,
    Vector<String>&& whitelist_selectors) {
  ReleasePageStyle();
  MakeRules(selectors, "{display:none !important}", hiding_rules_);
  MakeRules(whitelist_selectors, "{display:block !important}",
            whitelist_rules_);
}

void ContentBlockElementHiding::SetGlobalRules(Vector<String>&& selectors) {
  ReleaseGlobalStyle();
  MakeRules(selectors, "{display:none !important}", global_rules_);
}

void ContentBlockElementHiding::IncreaseNumberOfBlockedElements() {
  ++number_of_blocked_elements_;
}

void ContentBlockElementHiding::ResetNumberOfBlockedElements() {
  number_of_blocked_elements_ = 0;
}

ContentBlockElementHiding::RuleList
ContentBlockElementHiding::OrderedElementHidingRules() {
  RuleList result;
  if (!is_enabled_)
    return result;

  if (!global_style_) {
    global_style_ = ParseRules(global_rules_, global_sheet_);
    if (type_ == kExtension && global_style_) {
      global_style_->SetContentBlockerRules(true);
    }
  }
  if (!hiding_style_) {
    hiding_style_ = ParseRules(hiding_rules_, hiding_sheet_);
    if (type_ == kExtension && hiding_style_) {
      hiding_style_->SetContentBlockerRules(true);
    }
  }
  if (!whitelist_style_)
    whitelist_style_ = ParseRules(whitelist_rules_, whitelist_sheet_);

  ShrinkRules();

  if (global_style_)
    result.push_back(global_style_.Get());
  if (hiding_style_)
    result.push_back(hiding_style_.Get());
  if (whitelist_style_)
    result.push_back(whitelist_style_.Get());

  return result;
}

DEFINE_TRACE(ContentBlockElementHiding) {
  visitor->Trace(global_style_);
  visitor->Trace(hiding_style_);
  visitor->Trace(whitelist_style_);
  visitor->Trace(global_sheet_);
  visitor->Trace(hiding_sheet_);
  visitor->Trace(whitelist_sheet_);
}

}  // namespace blink
