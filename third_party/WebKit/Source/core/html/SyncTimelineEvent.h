// Copyright 2018 Samsung Electronics Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYNC_TIMELINE_EVENT_H
#define SYNC_TIMELINE_EVENT_H

#include "core/CoreExport.h"
#include "core/dom/events/Event.h"
#include "core/html/SyncTimelineEventInit.h"

namespace blink {

class CORE_EXPORT SyncTimelineEvent final : public Event {
  DEFINE_WRAPPERTYPEINFO();

 public:
  ~SyncTimelineEvent() override;

  static SyncTimelineEvent* Create() { return new SyncTimelineEvent; }

  static SyncTimelineEvent* Create(const AtomicString& type,
                                   const SyncTimelineEventInit& initializer) {
    return new SyncTimelineEvent(type, initializer);
  }

  const AtomicString& InterfaceName() const override;

  const String& timelineSelector() const { return timeline_selector_; }
  unsigned short sync() const { return sync_; }

  DECLARE_VIRTUAL_TRACE();

 private:
  SyncTimelineEvent();
  SyncTimelineEvent(const AtomicString& type,
                    const SyncTimelineEventInit& initializer);

  String timeline_selector_;
  unsigned short sync_;
};

}  // namespace blink

#endif  // SYNC_TIMELINE_EVENT_H