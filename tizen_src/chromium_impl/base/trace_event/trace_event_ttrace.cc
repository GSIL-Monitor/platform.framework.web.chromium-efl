// Copyright 2017 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ttrace.h"

#if defined(USE_TTRACE)
#include <string.h>
#include "base/strings/string_tokenizer.h"

bool TraceEventTTraceBegin(uint64_t tag,
                           const char* category_group,
                           const char* name) {
  std::string category_group_filter =
      "!disabled|!debug|blink|cc|gpu|media|sandbox_ipc|skia|v8";
  base::StringTokenizer tokens(category_group_filter, "|");
  bool result = false;
  while (tokens.GetNext()) {
    const char* category_group_token = tokens.token().c_str();
    if (category_group_token[0] == '!') {
      category_group_token++;
      if ((*category_group_token == '*') ||
          strstr(category_group, category_group_token))
        return false;
    } else {
      if ((*category_group_token == '*') ||
          strstr(category_group, category_group_token)) {
        result = true;
        break;
      }
    }
  }
  if (result)
    traceBegin(tag, "[%s]%s", category_group, name);
  return result;
}
#endif
