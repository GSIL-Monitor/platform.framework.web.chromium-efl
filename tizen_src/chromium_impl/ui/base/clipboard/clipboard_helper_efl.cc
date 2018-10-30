// Copyright 2018 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "clipboard_helper_efl.h"

Elm_Sel_Format ClipboardFormatToElm(ClipboardDataTypeEfl type) {
  switch (type) {
    case ClipboardDataTypeEfl::PLAIN_TEXT:
    case ClipboardDataTypeEfl::URI_LIST:
    case ClipboardDataTypeEfl::URL:
      return ELM_SEL_FORMAT_TEXT;
    case ClipboardDataTypeEfl::IMAGE:
      return ELM_SEL_FORMAT_IMAGE;
    case ClipboardDataTypeEfl::MARKUP:
      return ELM_SEL_FORMAT_HTML;
    default:
      return ELM_SEL_FORMAT_TEXT;
  }
}
