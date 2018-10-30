// Copyright 2018 Samsung Electronics Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/html/track/InbandCue.h"

namespace blink {

InbandCue::InbandCue(double start, double end, DOMArrayBuffer* data)
    : TextTrackCue(start, end), data_(data) {}

InbandCue::~InbandCue() {}

#ifndef NDEBUG
String InbandCue::ToString() const {
  return String::Format("%p id=%s interval=%f-->%f cue=%s)", this,
                        id().utf8().data(), startTime(), endTime(),
                        data().utf8().data());
}
#endif

void InbandCue::setData(DOMArrayBuffer* data) {
  if (data_ == data)
    return;

  CueWillChange();
  data_ = data;
  CueDidChange();
}

DEFINE_TRACE(InbandCue) {
  visitor->Trace(data_);
  TextTrackCue::Trace(visitor);
}

}  // namespace blink
