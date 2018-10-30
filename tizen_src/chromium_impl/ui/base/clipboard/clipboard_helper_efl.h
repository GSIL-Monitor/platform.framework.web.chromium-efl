// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CLIPBOARD_HELPER_EFL_H_
#define CLIPBOARD_HELPER_EFL_H_

#include <Elementary.h>

#if defined(USE_WAYLAND)
#include "ui/base/clipboard/clipboard_helper_efl_wayland.h"
#else
#include "ui/base/clipboard/clipboard_helper_efl_X11.h"
#endif

enum class ClipboardDataTypeEfl { PLAIN_TEXT, URI_LIST, URL, MARKUP, IMAGE };

Elm_Sel_Format ClipboardFormatToElm(ClipboardDataTypeEfl type);

#endif /* CLIPBOARD_HELPER_EFL_H_ */
