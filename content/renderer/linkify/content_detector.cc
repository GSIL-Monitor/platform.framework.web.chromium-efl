// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/linkify/content_detector.h"

#include "base/logging.h"
#include "third_party/WebKit/public/platform/WebPoint.h"
#include "third_party/WebKit/public/web/WebHitTestResult.h"
#include "third_party/WebKit/public/web/WebSurroundingText.h"

using blink::WebURL;
using blink::WebHitTestResult;
using blink::WebSurroundingText;

namespace content {

WebURL ContentDetector::FindTappedContent(const WebHitTestResult& hit_test) {
  if (hit_test.IsNull())
    return WebURL();

  std::string content_text;
  if (!FindContentRange(hit_test, &content_text))
    return WebURL();

  return GetIntentURL(content_text);
}

bool ContentDetector::FindContentRange(const WebHitTestResult& hit_test,
                                       std::string* content_text) {
  // As the surrounding text extractor looks at maxLength/2 characters on
  // either side of the hit point, we need to double max content length here.
  WebSurroundingText surrounding_text;
  // FIXME: m62_3202 bringup, needs to be checked by author
  // https://chromium-review.googlesource.com/c/chromium/src/+/625543
  //surrounding_text.Initialize(hit_test.GetNode(), hit_test.LocalPoint(),
  //                            GetMaximumContentLength() * 2);
  if (surrounding_text.IsNull())
    return false;

  base::string16 content = surrounding_text.TextContent().Utf16();
  if (content.empty())
    return false;

  size_t selected_offset = surrounding_text.HitOffsetInTextContent();
  for (size_t start_offset = 0; start_offset < content.length();) {
    size_t relative_start, relative_end;
    if (!FindContent(content.begin() + start_offset,
        content.end(), &relative_start, &relative_end, content_text)) {
      break;
    } else {
      size_t content_start = start_offset + relative_start;
      size_t content_end = start_offset + relative_end;
      DCHECK(content_end <= content.length());

      if (selected_offset >= content_start && selected_offset < content_end)
        return true;
      else
        start_offset += relative_end;
    }
  }

  return false;
}

}  // namespace content
