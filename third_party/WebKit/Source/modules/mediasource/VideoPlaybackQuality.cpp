/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "modules/mediasource/VideoPlaybackQuality.h"

#include "core/dom/Document.h"
#include "core/frame/LocalDOMWindow.h"
#include "core/timing/DOMWindowPerformance.h"
#include "core/timing/Performance.h"

namespace blink {

VideoPlaybackQuality* VideoPlaybackQuality::Create(
    const Document& document,
    unsigned total_video_frames,
    unsigned dropped_video_frames,
    unsigned corrupted_video_frames,
    double total_frame_delay) {
  return new VideoPlaybackQuality(document, total_video_frames,
                                  dropped_video_frames, corrupted_video_frames,
                                  total_frame_delay);
}

VideoPlaybackQuality::VideoPlaybackQuality(const Document& document,
                                           unsigned total_video_frames,
                                           unsigned dropped_video_frames,
                                           unsigned corrupted_video_frames,
                                           double total_frame_delay)
    : creation_time_(0),
      total_video_frames_(total_video_frames),
      dropped_video_frames_(dropped_video_frames),
      corrupted_video_frames_(corrupted_video_frames),
      total_frame_delay_(total_frame_delay) {
  if (document.domWindow())
    creation_time_ =
        DOMWindowPerformance::performance(*(document.domWindow()))->now();
}

}  // namespace blink