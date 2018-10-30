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

#ifndef VideoPlaybackQuality_h
#define VideoPlaybackQuality_h

#include "platform/bindings/ScriptWrappable.h"
#include "platform/heap/Handle.h"

namespace blink {

class Document;

class VideoPlaybackQuality : public GarbageCollected<VideoPlaybackQuality>,
                             public ScriptWrappable {
  DEFINE_WRAPPERTYPEINFO();

 public:
  static VideoPlaybackQuality* Create(const Document&,
                                      unsigned total_video_frames,
                                      unsigned dropped_video_frames,
                                      unsigned corrupted_video_frames,
                                      double total_frame_delay);

  double creationTime() const { return creation_time_; }
  unsigned totalVideoFrames() const { return total_video_frames_; }
  unsigned droppedVideoFrames() const { return dropped_video_frames_; }
  unsigned corruptedVideoFrames() const { return corrupted_video_frames_; }
  double totalFrameDelay() const { return total_frame_delay_; }

  DEFINE_INLINE_TRACE() {}

 private:
  VideoPlaybackQuality(const Document&,
                       unsigned total_video_frames,
                       unsigned dropped_video_frames,
                       unsigned corrupted_video_frames,
                       double total_frame_delay);

  double creation_time_;
  unsigned total_video_frames_;
  unsigned dropped_video_frames_;
  unsigned corrupted_video_frames_;
  double total_frame_delay_;
};

}  // namespace blink

#endif  // VideoPlaybackQuality_h