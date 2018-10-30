// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_CAPTURE_VIDEO_CAPTURER_SOURCE_H_
#define MEDIA_CAPTURE_VIDEO_CAPTURER_SOURCE_H_

#include <string>
#include <vector>

#include "base/callback.h"
#include "base/memory/ref_counted.h"
#include "base/time/time.h"
#include "media/capture/capture_export.h"
#include "media/capture/video_capture_types.h"

namespace media {

class VideoFrame;
class MediaStreamVideoDecoderData;
class SuspendingDecoder;

// VideoCapturerSource is an interface representing the source for captured
// video.  An implementation will periodically call the frame callback with new
// video frames.
class CAPTURE_EXPORT VideoCapturerSource {
 public:
  virtual ~VideoCapturerSource();

  // This callback is used to deliver video frames.
  //
  // |estimated_capture_time| - The capture time of the delivered video
  // frame. This field represents the local time at which either: 1) the frame
  // was generated, if it was done so locally; or 2) the targeted play-out time
  // of the frame, if it was generated from a remote source. Either way, an
  // implementation should not present the frame before this point-in-time. This
  // value is NOT a high-resolution timestamp, and so it should not be used as a
  // presentation time; but, instead, it should be used for buffering playback
  // and for A/V synchronization purposes. NOTE: It is possible for this value
  // to be null if the current implementation lacks this timing information.
  //
  // |video_frame->timestamp()| gives the presentation timestamp of the video
  // frame relative to the first frame generated by the corresponding source.
  // Because a source can start generating frames before a subscriber is added,
  // the first video frame delivered may not have timestamp equal to 0.
  using VideoCaptureDeliverFrameCB =
      base::Callback<void(const scoped_refptr<media::VideoFrame>& video_frame,
                          base::TimeTicks estimated_capture_time)>;

  using VideoCaptureDeviceFormatsCB =
      base::Callback<void(const media::VideoCaptureFormats&)>;

  using VideoCaptureDecoderDataCB =
      base::Callback<void(const scoped_refptr<MediaStreamVideoDecoderData>&)>;

  using SuspendingDecoderCB =
      base::Callback<void(const scoped_refptr<SuspendingDecoder>&)>;

  using RunningCallback = base::Callback<void(bool)>;

  // Returns formats that are preferred and can currently be used. May be empty
  // if no formats are available or known.
  virtual VideoCaptureFormats GetPreferredFormats() = 0;

  // Starts capturing frames using the capture |params|. |new_frame_callback| is
  // triggered when a new video frame is available.
  // If capturing is started successfully then |running_callback| will be
  // called with a parameter of true. Note that some implementations may
  // simply reject StartCapture (by calling running_callback with a false
  // argument) if called with the wrong task runner.
  // If capturing fails to start or stopped due to an external event then
  // |running_callback| will be called with a parameter of false.
  // |running_callback| will always be called on the same thread as the
  // StartCapture.
  virtual void StartCapture(
      const media::VideoCaptureParams& params,
      const VideoCaptureDeliverFrameCB& new_frame_callback,
      const RunningCallback& running_callback) = 0;

  // Asks source to send a refresh frame. In cases where source does not provide
  // a continuous rate of new frames (e.g. canvas capture, screen capture where
  // the screen's content has not changed in a while), consumers may request a
  // "refresh frame" to be delivered. For instance, this would be needed when
  // a new sink is added to a MediaStreamTrack.
  //
  // The default implementation is a no-op and implementations are not required
  // to honor this request. If they decide to and capturing is started
  // successfully, then |new_frame_callback| should be called with a frame.
  //
  // Note: This should only be called after StartCapture() and before
  // StopCapture(). Otherwise, its behavior is undefined.
  virtual void RequestRefreshFrame() {}

  // Optionally suspends frame delivery. The source may or may not honor this
  // request. Thus, the caller cannot assume frame delivery will actually
  // stop. Even if frame delivery is suspended, this might not take effect
  // immediately.
  //
  // The purpose of this is to allow minimizing resource usage while
  // there are no frame consumers present.
  //
  // Note: This should only be called after StartCapture() and before
  // StopCapture(). Otherwise, its behavior is undefined.
  virtual void MaybeSuspend() {}

  // Resumes frame delivery, if it was suspended. If frame delivery was not
  // suspended, this is a no-op, and frame delivery will continue.
  //
  // Note: This should only be called after StartCapture() and before
  // StopCapture(). Otherwise, its behavior is undefined.
  virtual void Resume() {}

  // Stops capturing frames and clears all callbacks including the
  // SupportedFormatsCallback callback. Note that queued frame callbacks
  // may still occur after this call, so the caller must take care to
  // use refcounted or weak references in |new_frame_callback|.
  virtual void StopCapture() = 0;
};

}  // namespace media

#endif  // MEDIA_CAPTURE_VIDEO_CAPTURER_SOURCE_H_
