// Copyright 2018 Samsung Electronics Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_SYNC_PARA_H
#define MEDIA_SYNC_PARA_H

#include "core/CoreExport.h"
#include "platform/bindings/ScriptWrappable.h"
#include "platform/heap/Handle.h"

namespace blink {

class CORE_EXPORT MediaSyncPara final : public GarbageCollected<MediaSyncPara>,
                                        public ScriptWrappable {
  DEFINE_WRAPPERTYPEINFO();

 public:
  static MediaSyncPara* create(uint32_t units_per_tick,
                               uint32_t units_per_second,
                               int64_t content_time,
                               bool paused) {
    return new MediaSyncPara(units_per_tick, units_per_second, content_time,
                             paused);
  }

  uint32_t unitsPerTick() const { return units_per_tick_; }
  uint32_t unitsPerSecond() const { return units_per_second_; }
  int64_t contentTime() const { return content_time_; }
  bool paused() const { return paused_; }

  DEFINE_INLINE_TRACE() {}

 private:
  MediaSyncPara(uint32_t units_per_tick,
                uint32_t units_per_second,
                int64_t content_time,
                bool paused)
      : units_per_tick_(units_per_tick),
        units_per_second_(units_per_second),
        content_time_(content_time),
        paused_(paused) {}

  uint32_t units_per_tick_;
  uint32_t units_per_second_;
  int64_t content_time_;
  bool paused_;
};

}  // namespace blink

#endif  // MEDIA_SYNC_PARA_H
