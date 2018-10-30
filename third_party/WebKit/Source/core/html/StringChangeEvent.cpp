// Copyright 2018 Samsung Electronics Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/html/StringChangeEvent.h"
#include "config.h"

namespace blink {

StringChangeEvent::StringChangeEvent() {}

StringChangeEvent::StringChangeEvent(const AtomicString& type,
                                     const StringChangeEventInit& initializer)
    : Event(type, initializer) {
  if (initializer.hasStr())
    str_ = initializer.str();
}

StringChangeEvent::~StringChangeEvent() {}

const AtomicString& StringChangeEvent::InterfaceName() const {
  return EventNames::StringChangeEvent;
}

DEFINE_TRACE(StringChangeEvent) {
  Event::Trace(visitor);
}

}  // namespace blink
