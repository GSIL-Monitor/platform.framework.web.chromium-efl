// Copyright 2018 Samsung Electronics Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REGISTER_TIMELINE_EVENT_H
#define REGISTER_TIMELINE_EVENT_H

#include "core/CoreExport.h"
#include "core/dom/events/Event.h"
#include "core/html/RegisterTimelineEventInit.h"

namespace blink {

class CORE_EXPORT RegisterTimelineEvent final : public Event {
  DEFINE_WRAPPERTYPEINFO();

 public:
  ~RegisterTimelineEvent() override;

  static RegisterTimelineEvent* Create() { return new RegisterTimelineEvent; }

  static RegisterTimelineEvent* Create(
      const AtomicString& type,
      const RegisterTimelineEventInit& initializer) {
    return new RegisterTimelineEvent(type, initializer);
  }

  const AtomicString& InterfaceName() const override;

  String timelineSelector() const { return timeline_selector_; }
  uint32_t unitsPerTick() const { return units_per_tick_; }
  uint32_t unitsPerSecond() const { return units_per_second_; }
  uint64_t contentTime() const { return content_time_; }
  unsigned short timelineState() const { return timeline_state_; }
  DECLARE_VIRTUAL_TRACE();

 private:
  RegisterTimelineEvent();
  RegisterTimelineEvent(const AtomicString& type,
                        const RegisterTimelineEventInit& initializer);

  String timeline_selector_;
  uint32_t units_per_tick_;
  uint32_t units_per_second_;
  uint64_t content_time_;
  unsigned short timeline_state_;
};

}  // namespace blink

#endif  // REGISTER_TIMELINE_EVENT_H