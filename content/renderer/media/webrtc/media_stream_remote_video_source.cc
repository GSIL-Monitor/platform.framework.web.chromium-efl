// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/media/webrtc/media_stream_remote_video_source.h"

#include <stdint.h>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/callback_helpers.h"
#include "base/location.h"
#include "base/trace_event/trace_event.h"
#include "content/renderer/media/webrtc/track_observer.h"
#include "content/renderer/media/webrtc/webrtc_video_frame_adapter.h"
#include "media/base/bind_to_current_loop.h"
#include "media/base/timestamp_constants.h"
#include "media/base/video_frame.h"
#include "media/base/video_util.h"
#include "media/capture/video_capturer_config.h"
#include "third_party/webrtc/api/video/i420_buffer.h"
#include "third_party/webrtc/api/video_codecs/video_decoder.h"
#include "third_party/webrtc/media/base/videosinkinterface.h"

namespace content {

namespace {

media::VideoRotation WebRTCToMediaVideoRotation(
    webrtc::VideoRotation rotation) {
  switch (rotation) {
    case webrtc::kVideoRotation_0:
      return media::VIDEO_ROTATION_0;
    case webrtc::kVideoRotation_90:
      return media::VIDEO_ROTATION_90;
    case webrtc::kVideoRotation_180:
      return media::VIDEO_ROTATION_180;
    case webrtc::kVideoRotation_270:
      return media::VIDEO_ROTATION_270;
  }
  return media::VIDEO_ROTATION_0;
}

}  // anonymous namespace

// Internal class used for receiving frames from the webrtc track on a
// libjingle thread and forward it to the IO-thread.
class MediaStreamRemoteVideoSource::RemoteVideoSourceDelegate
    : public base::RefCountedThreadSafe<RemoteVideoSourceDelegate>,
      public rtc::VideoSinkInterface<webrtc::VideoFrame> {
 public:
  RemoteVideoSourceDelegate(
      scoped_refptr<base::SingleThreadTaskRunner> io_task_runner,
      const VideoCaptureDeliverFrameCB& new_frame_callback,
      const VideoCaptureDecoderDataCB& decoder_data_cb,
      const SuspendingDecoderCB& suspending_decoder_cb);

 protected:
  friend class base::RefCountedThreadSafe<RemoteVideoSourceDelegate>;
  ~RemoteVideoSourceDelegate() override;

  // Implements rtc::VideoSinkInterface used for receiving video frames
  // from the PeerConnection video track. May be called on a libjingle internal
  // thread.
  void OnFrame(const webrtc::VideoFrame& frame) override;

  void OnDecoderType(const webrtc::VideoDecoderData& config) override;

  void OnSuspendingDecoder(
      std::shared_ptr<webrtc::SuspendingDecoder> suspending_decoder) override;

  void DoRenderFrameOnIOThread(
      const scoped_refptr<media::VideoFrame>& video_frame);

 private:
  scoped_refptr<base::SingleThreadTaskRunner> io_task_runner_;

  // |frame_callback_| is accessed on the IO thread.
  VideoCaptureDeliverFrameCB frame_callback_;

  VideoCaptureDecoderDataCB decoder_data_cb_;

  SuspendingDecoderCB suspending_decoder_cb_;

  // Timestamp of the first received frame.
  base::TimeDelta start_timestamp_;
  // WebRTC Chromium timestamp diff
  const base::TimeDelta time_diff_;
};

MediaStreamRemoteVideoSource::RemoteVideoSourceDelegate::
    RemoteVideoSourceDelegate(
        scoped_refptr<base::SingleThreadTaskRunner> io_task_runner,
        const VideoCaptureDeliverFrameCB& new_frame_callback,
        const VideoCaptureDecoderDataCB& decoder_data_cb,
        const SuspendingDecoderCB& suspending_decoder_cb)
    : io_task_runner_(io_task_runner),
      frame_callback_(new_frame_callback),
      decoder_data_cb_(decoder_data_cb),
      suspending_decoder_cb_(suspending_decoder_cb),
      start_timestamp_(media::kNoTimestamp),
      // TODO(qiangchen): There can be two differences between clocks: 1)
      // the offset, 2) the rate (i.e., one clock runs faster than the other).
      // See http://crbug/516700
      time_diff_(base::TimeTicks::Now() - base::TimeTicks() -
                 base::TimeDelta::FromMicroseconds(rtc::TimeMicros())) {}

MediaStreamRemoteVideoSource::
RemoteVideoSourceDelegate::~RemoteVideoSourceDelegate() {
}

namespace {
void DoNothing(const scoped_refptr<rtc::RefCountInterface>& ref) {}
}  // anonymous

media::TizenDecoderType WebRtcDecoderTypeToMediaTizenDecoderType(
    webrtc::VideoDecoderType type) {
  switch (type) {
    case webrtc::VideoDecoderType::ENCODED:
      return media::TizenDecoderType::ENCODED;
    case webrtc::VideoDecoderType::TEXTURE:
      return media::TizenDecoderType::TEXTURE;
    default:
      return media::TizenDecoderType::UNKNOWN;
  }
}

media::VideoCodec WebRtcVideoCodecToVideoCodec(webrtc::VideoCodecType type) {
  switch (type) {
    case webrtc::VideoCodecType::kVideoCodecVP8:
      return media::VideoCodec::kCodecVP8;
    case webrtc::VideoCodecType::kVideoCodecVP9:
      return media::VideoCodec::kCodecVP9;
    case webrtc::VideoCodecType::kVideoCodecH264:
      return media::VideoCodec::kCodecH264;
    default:
      LOG(ERROR) << "Unknown WebRTC video codec type: " << type;
      return media::VideoCodec::kUnknownVideoCodec;
  }
}

media::VideoDecoderConfig WebRtcVideoCodecToVideoDecoderConfig(
    const webrtc::VideoCodec& codec) {
  return media::VideoDecoderConfig{
      WebRtcVideoCodecToVideoCodec(codec.codecType),
      media::VIDEO_CODEC_PROFILE_UNKNOWN,
      media::PIXEL_FORMAT_UNKNOWN,
      media::COLOR_SPACE_UNSPECIFIED,
      gfx::Size{codec.width, codec.height},
      gfx::Rect{codec.width, codec.height},
      gfx::Size{codec.width, codec.height},
      std::vector<uint8_t>(),
      media::EncryptionScheme()};
}

scoped_refptr<media::MediaStreamVideoDecoderData>
WebRrtcDecoderConfigToCapturerDecoderData(
    const webrtc::VideoDecoderData& config) {
  return base::WrapRefCounted(new media::MediaStreamVideoDecoderData{
      WebRtcDecoderTypeToMediaTizenDecoderType(config.type),
      WebRtcVideoCodecToVideoDecoderConfig(config.codec)});
}

void MediaStreamRemoteVideoSource::RemoteVideoSourceDelegate::OnDecoderType(
    const webrtc::VideoDecoderData& config) {
  // Conversion between webrtc codec and blink coded
  decoder_data_cb_.Run(WebRrtcDecoderConfigToCapturerDecoderData(config));
}

scoped_refptr<media::SuspendingDecoder>
WebRtcSuspendingDecodertoMediaSuspendingDecoder(
    std::shared_ptr<webrtc::SuspendingDecoder> suspending_decoder) {
  return base::WrapRefCounted(
      new media::SuspendingDecoder(std::move(suspending_decoder)));
}

void MediaStreamRemoteVideoSource::RemoteVideoSourceDelegate::
    OnSuspendingDecoder(
        std::shared_ptr<webrtc::SuspendingDecoder> suspending_decoder) {
  suspending_decoder_cb_.Run(WebRtcSuspendingDecodertoMediaSuspendingDecoder(
      std::move(suspending_decoder)));
}

void MediaStreamRemoteVideoSource::RemoteVideoSourceDelegate::OnFrame(
    const webrtc::VideoFrame& incoming_frame) {
  const base::TimeDelta incoming_timestamp = base::TimeDelta::FromMicroseconds(
      incoming_frame.timestamp_us());
  const base::TimeTicks render_time =
      base::TimeTicks() + incoming_timestamp + time_diff_;

  TRACE_EVENT1("webrtc", "RemoteVideoSourceDelegate::RenderFrame",
               "Ideal Render Instant", render_time.ToInternalValue());

  CHECK_NE(media::kNoTimestamp, incoming_timestamp);
  if (start_timestamp_ == media::kNoTimestamp)
    start_timestamp_ = incoming_timestamp;
  const base::TimeDelta elapsed_timestamp =
      incoming_timestamp - start_timestamp_;

  scoped_refptr<media::VideoFrame> video_frame;
  scoped_refptr<webrtc::VideoFrameBuffer> buffer(
      incoming_frame.video_frame_buffer());

  if (buffer->type() == webrtc::VideoFrameBuffer::Type::kNative) {
    video_frame = static_cast<WebRtcVideoFrameAdapter*>(buffer.get())
                      ->getMediaVideoFrame();
    video_frame->set_timestamp(elapsed_timestamp);
  } else {
    scoped_refptr<webrtc::PlanarYuvBuffer> yuv_buffer;
    media::VideoPixelFormat pixel_format;

    if (buffer->type() == webrtc::VideoFrameBuffer::Type::kEncoded) {
      yuv_buffer = buffer->GetEncodedBufferInterface();
      gfx::Size size(yuv_buffer->width(), yuv_buffer->height());
      video_frame = media::VideoFrame::WrapExternalEncodedData(
          size, const_cast<uint8_t*>(yuv_buffer->DataY()),
          yuv_buffer->StrideY(), elapsed_timestamp);
    } else {
      if (buffer->type() == webrtc::VideoFrameBuffer::Type::kI444) {
        yuv_buffer = buffer->GetI444();
        pixel_format = media::PIXEL_FORMAT_YV24;
      } else {
        yuv_buffer = buffer->ToI420();
        pixel_format = media::PIXEL_FORMAT_YV12;
      }
      gfx::Size size(yuv_buffer->width(), yuv_buffer->height());
      // Make a shallow copy. Both |frame| and |video_frame| will share a single
      // reference counted frame buffer. Const cast and hope no one will
      // overwrite the data.
      video_frame = media::VideoFrame::WrapExternalYuvData(
          pixel_format, size, gfx::Rect(size), size, yuv_buffer->StrideY(),
          yuv_buffer->StrideU(), yuv_buffer->StrideV(),
          const_cast<uint8_t*>(yuv_buffer->DataY()),
          const_cast<uint8_t*>(yuv_buffer->DataU()),
          const_cast<uint8_t*>(yuv_buffer->DataV()), elapsed_timestamp);
    }
    if (!video_frame)
      return;
    // The bind ensures that we keep a reference to the underlying buffer.
    video_frame->AddDestructionObserver(base::BindOnce(&DoNothing, buffer));
  }
  if (incoming_frame.rotation() != webrtc::kVideoRotation_0) {
    video_frame->metadata()->SetRotation(
        media::VideoFrameMetadata::ROTATION,
        WebRTCToMediaVideoRotation(incoming_frame.rotation()));
  }
  video_frame->metadata()->SetTimeTicks(
      media::VideoFrameMetadata::REFERENCE_TIME, render_time);

  io_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&RemoteVideoSourceDelegate::DoRenderFrameOnIOThread, this,
                     video_frame));
}

void MediaStreamRemoteVideoSource::
RemoteVideoSourceDelegate::DoRenderFrameOnIOThread(
    const scoped_refptr<media::VideoFrame>& video_frame) {
  DCHECK(io_task_runner_->BelongsToCurrentThread());
  TRACE_EVENT0("webrtc", "RemoteVideoSourceDelegate::DoRenderFrameOnIOThread");
  // TODO(hclam): Give the estimated capture time.
  frame_callback_.Run(video_frame, base::TimeTicks());
}

MediaStreamRemoteVideoSource::MediaStreamRemoteVideoSource(
    std::unique_ptr<TrackObserver> observer)
    : observer_(std::move(observer)) {
  // The callback will be automatically cleared when 'observer_' goes out of
  // scope and no further callbacks will occur.
  observer_->SetCallback(base::Bind(&MediaStreamRemoteVideoSource::OnChanged,
      base::Unretained(this)));
}

MediaStreamRemoteVideoSource::~MediaStreamRemoteVideoSource() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(!observer_);
}

void MediaStreamRemoteVideoSource::OnSourceTerminated() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  StopSourceImpl();
}

void MediaStreamRemoteVideoSource::StartSourceImpl(
    const VideoCaptureDeliverFrameCB& frame_callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  StartSourceImpl(frame_callback, VideoCaptureDecoderDataCB(),
                  SuspendingDecoderCB());
}

void MediaStreamRemoteVideoSource::StartSourceImpl(
    const VideoCaptureDeliverFrameCB& frame_callback,
    const VideoCaptureDecoderDataCB& decoder_data_cb,
    const SuspendingDecoderCB& suspending_decoder_cb) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(!delegate_.get());
  delegate_ = new RemoteVideoSourceDelegate(
      io_task_runner(), frame_callback, decoder_data_cb, suspending_decoder_cb);
  scoped_refptr<webrtc::VideoTrackInterface> video_track(
      static_cast<webrtc::VideoTrackInterface*>(observer_->track().get()));
  video_track->AddOrUpdateSink(delegate_.get(), rtc::VideoSinkWants());
  OnStartDone(MEDIA_DEVICE_OK);
}

void MediaStreamRemoteVideoSource::StopSourceImpl() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  // StopSourceImpl is called either when MediaStreamTrack.stop is called from
  // JS or blink gc the MediaStreamSource object or when OnSourceTerminated()
  // is called. Garbage collection will happen after the PeerConnection no
  // longer receives the video track.
  if (!observer_)
    return;
  DCHECK(state() != MediaStreamVideoSource::ENDED);
  scoped_refptr<webrtc::VideoTrackInterface> video_track(
      static_cast<webrtc::VideoTrackInterface*>(observer_->track().get()));
  video_track->RemoveSink(delegate_.get());
  // This removes the references to the webrtc video track.
  observer_.reset();
}

rtc::VideoSinkInterface<webrtc::VideoFrame>*
MediaStreamRemoteVideoSource::SinkInterfaceForTest() {
  return delegate_.get();
}

void MediaStreamRemoteVideoSource::OnChanged(
    webrtc::MediaStreamTrackInterface::TrackState state) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  switch (state) {
    case webrtc::MediaStreamTrackInterface::kLive:
      SetReadyState(blink::WebMediaStreamSource::kReadyStateLive);
      break;
    case webrtc::MediaStreamTrackInterface::kEnded:
      SetReadyState(blink::WebMediaStreamSource::kReadyStateEnded);
      break;
    default:
      NOTREACHED();
      break;
  }
}

}  // namespace content
