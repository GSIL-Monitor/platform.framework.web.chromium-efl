// Copyright 2018 Samsung Electronics Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef STRING_CHANGE_EVENT_H
#define STRING_CHANGE_EVENT_H

#include "core/CoreExport.h"
#include "core/dom/events/Event.h"
#include "core/html/StringChangeEventInit.h"

namespace blink {

class CORE_EXPORT StringChangeEvent final : public Event {
  DEFINE_WRAPPERTYPEINFO();

 public:
  ~StringChangeEvent() override;

  static StringChangeEvent* Create() { return new StringChangeEvent; }

  static StringChangeEvent* Create(const AtomicString& type,
                                   const StringChangeEventInit& initializer) {
    return new StringChangeEvent(type, initializer);
  }

  const AtomicString& InterfaceName() const override;

  const String& str() const { return str_; }

  DECLARE_VIRTUAL_TRACE();

 private:
  StringChangeEvent();
  StringChangeEvent(const AtomicString& type,
                    const StringChangeEventInit& initializer);

  String str_;
};

}  // namespace blink

#endif  // STRING_CHANGE_EVENT_H