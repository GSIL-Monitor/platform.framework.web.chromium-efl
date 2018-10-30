// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/browser/native_web_keyboard_event.h"

#include "ui/events/base_event_utils.h"
#include "ui/events/blink/web_input_event.h"
#include "ui/events/event.h"
#include "ui/events/keycodes/dom/keycode_converter.h"

namespace content {

ui::Event* CopyEvent(const ui::Event* event) {
  return event ? ui::Event::Clone(*event).release() : nullptr;
}

NativeWebKeyboardEvent::NativeWebKeyboardEvent(blink::WebInputEvent::Type type,
                                               int modifiers,
                                               base::TimeTicks timestamp)
    : NativeWebKeyboardEvent(type,
                             modifiers,
                             ui::EventTimeStampToSeconds(timestamp)) {}

NativeWebKeyboardEvent::NativeWebKeyboardEvent(blink::WebInputEvent::Type type,
                                               int modifiers,
                                               double timestampSeconds)
    : WebKeyboardEvent(type, modifiers, timestampSeconds),
      os_event(nullptr),
      skip_in_browser(false) {}

NativeWebKeyboardEvent::NativeWebKeyboardEvent(
    const blink::WebKeyboardEvent& web_event,
    gfx::NativeView native_view)
    : WebKeyboardEvent(web_event), os_event(nullptr), skip_in_browser(false) {}

NativeWebKeyboardEvent::NativeWebKeyboardEvent(gfx::NativeEvent native_event)
    : NativeWebKeyboardEvent(static_cast<ui::KeyEvent&>(*native_event)) {}

NativeWebKeyboardEvent::NativeWebKeyboardEvent(const ui::KeyEvent& key_event)
    : WebKeyboardEvent(ui::MakeWebKeyboardEvent(key_event)),
      os_event(CopyEvent(&key_event)),
      skip_in_browser(false) {
}

NativeWebKeyboardEvent::NativeWebKeyboardEvent(
    const NativeWebKeyboardEvent& other)
    : WebKeyboardEvent(other),
      os_event(CopyEvent(other.os_event)),
      skip_in_browser(other.skip_in_browser) {}

NativeWebKeyboardEvent& NativeWebKeyboardEvent::operator=(
    const NativeWebKeyboardEvent& other) {
  WebKeyboardEvent::operator=(other);
  delete os_event;
  os_event = CopyEvent(other.os_event);
  skip_in_browser = other.skip_in_browser;

  return *this;
}

NativeWebKeyboardEvent::~NativeWebKeyboardEvent() {
  delete os_event;
}

}  // namespace content
