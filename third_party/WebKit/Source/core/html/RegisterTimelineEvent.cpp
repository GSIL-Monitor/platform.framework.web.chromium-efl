// Copyright 2018 Samsung Electronics Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/html/RegisterTimelineEvent.h"
#include "config.h"

namespace blink {

RegisterTimelineEvent::RegisterTimelineEvent() {}

RegisterTimelineEvent::RegisterTimelineEvent(
    const AtomicString& type,
    const RegisterTimelineEventInit& initializer)
    : Event(type, initializer) {
  if (initializer.hasTimelineSelector())
    timeline_selector_ = initializer.timelineSelector();
  if (initializer.hasUnitsPerTick())
    units_per_tick_ = initializer.unitsPerTick();
  if (initializer.hasUnitsPerSecond())
    units_per_second_ = initializer.unitsPerSecond();
  if (initializer.hasContentTime())
    content_time_ = initializer.contentTime();
  if (initializer.hasTimelineState())
    timeline_state_ = initializer.timelineState();
}

RegisterTimelineEvent::~RegisterTimelineEvent() {}

const AtomicString& RegisterTimelineEvent::InterfaceName() const {
  return EventNames::RegisterTimelineEvent;
}

DEFINE_TRACE(RegisterTimelineEvent) {
  Event::Trace(visitor);
}

}  // namespace blink
