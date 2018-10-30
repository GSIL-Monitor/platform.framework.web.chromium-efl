// Copyright 2018 Samsung Electronics Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/html/SyncTimelineEvent.h"
#include "config.h"

namespace blink {

SyncTimelineEvent::SyncTimelineEvent() {}

SyncTimelineEvent::SyncTimelineEvent(const AtomicString& type,
                                     const SyncTimelineEventInit& initializer)
    : Event(type, initializer) {
  if (initializer.hasTimelineSelector())
    timeline_selector_ = initializer.timelineSelector();
  if (initializer.hasSync())
    sync_ = initializer.sync();
}

SyncTimelineEvent::~SyncTimelineEvent() {}

const AtomicString& SyncTimelineEvent::InterfaceName() const {
  return EventNames::SyncTimelineEvent;
}

DEFINE_TRACE(SyncTimelineEvent) {
  Event::Trace(visitor);
}

}  // namespace blink
