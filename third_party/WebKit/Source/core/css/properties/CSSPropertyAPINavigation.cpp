// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/css/properties/CSSPropertyAPINavigation.h"

#include "core/css/parser/CSSPropertyParserHelpers.h"

namespace blink {

// auto | <id> [ current | root | <target-name> ] | inherit
const CSSValue* CSSPropertyAPINavigation::ParseSingleValue(
    CSSParserTokenRange& range,
    const CSSParserContext& context,
    const CSSParserLocalContext&) const {
  if (range.Peek().Id() == CSSValueAuto)
    return CSSPropertyParserHelpers::ConsumeIdent(range);

  String selector_text = "";
  CSSParserTokenType type = range.Peek().GetType();
  if (type == kHashToken)
    selector_text.append(range.ConsumeIncludingWhitespace().Value());
  else if (type == kStringToken) {  // for round trip values
    selector_text.append(range.ConsumeIncludingWhitespace().Value());
    if (!selector_text.StartsWith("#"))
      return nullptr;
  } else
    return nullptr;

  type = range.Peek().GetType();
  CSSValueList* list = CSSValueList::CreateSpaceSeparated();
  CSSValue* id_selector = CSSStringValue::Create(selector_text);
  list->Append(*id_selector);

  CSSValue* target;
  if (type == kIdentToken)
    target = CSSPropertyParserHelpers::ConsumeIdent(range);
  else if (type == kStringToken) {
    String target_text =
        range.ConsumeIncludingWhitespace().Value().ToAtomicString();
    if (target_text == "_root")
      target = CSSIdentifierValue::Create(CSSValueRoot);
    else
      target = CSSStringValue::Create(target_text);
  } else if (range.AtEnd())
    target = CSSIdentifierValue::Create(CSSValueCurrent);
  else
    return nullptr;

  list->Append(*target);

  return list;
}

}  // namespace blink
