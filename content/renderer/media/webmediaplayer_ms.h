// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_MEDIA_WEBMEDIAPLAYER_MS_H_
#define CONTENT_RENDERER_MEDIA_WEBMEDIAPLAYER_MS_H_

#include <memory>
#include <string>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/single_thread_task_runner.h"
#include "base/synchronization/lock.h"
#include "base/threading/thread_checker.h"
#include "content/common/content_export.h"
#include "media/blink/webmediaplayer_delegate.h"
#include "media/blink/webmediaplayer_util.h"
#include "media/capture/video_capturer_config.h"
#include "media/renderers/skcanvas_video_renderer.h"
#include "media/video/gpu_video_accelerator_factories.h"
#include "third_party/WebKit/public/platform/WebMediaPlayer.h"
#include "third_party/WebKit/public/platform/WebMediaStream.h"
#include "url/origin.h"

#include "ui/gfx/geometry/rect_f.h"

namespace blink {
class WebLocalFrame;
class WebMediaPlayerClient;
class WebSecurityOrigin;
class WebString;
}

namespace media {
class MediaLog;
class VideoFrame;
class MediaStreamVideoDecoderData;
class SuspendingDecoder;
enum VideoRotation;
}

namespace cc_blink {
class WebLayerImpl;
}

namespace gpu {
namespace gles2 {
class GLES2Interface;
}
}

namespace content {
class MediaStreamAudioRenderer;
class MediaStreamRendererFactory;
class MediaStreamVideoRenderer;
class WebMediaPlayerMSCompositor;
#if defined(TIZEN_MULTIMEDIA_SUPPORT)
class RendererMediaPlayerManager;
class RendererDemuxerEfl;
#endif

// WebMediaPlayerMS delegates calls from WebCore::MediaPlayerPrivate to
// Chrome's media player when "src" is from media stream.
//
// All calls to WebMediaPlayerMS methods must be from the main thread of
// Renderer process.
//
// WebMediaPlayerMS works with multiple objects, the most important ones are:
//
// MediaStreamVideoRenderer
//   provides video frames for rendering.
//
// blink::WebMediaPlayerClient
//   WebKit client of this media player object.
class CONTENT_EXPORT WebMediaPlayerMS
    : public blink::WebMediaStreamObserver,
      public blink::WebMediaPlayer,
      public media::WebMediaPlayerDelegate::Observer,
      public base::SupportsWeakPtr<WebMediaPlayerMS> {
 public:
  // Construct a WebMediaPlayerMS with reference to the client, and
  // a MediaStreamClient which provides MediaStreamVideoRenderer.
  // |delegate| must not be null.
  WebMediaPlayerMS(
      blink::WebLocalFrame* frame,
      blink::WebMediaPlayerClient* client,
      media::WebMediaPlayerDelegate* delegate,
      std::unique_ptr<media::MediaLog> media_log,
      std::unique_ptr<MediaStreamRendererFactory> factory,
#if defined(TIZEN_MULTIMEDIA_SUPPORT)
      RendererMediaPlayerManager* manager,
#endif
      scoped_refptr<base::SingleThreadTaskRunner> io_task_runner_,
      scoped_refptr<base::SingleThreadTaskRunner> compositor_task_runner,
      scoped_refptr<base::SingleThreadTaskRunner> media_task_runner,
      scoped_refptr<base::TaskRunner> worker_task_runner,
      media::GpuVideoAcceleratorFactories* gpu_factories,
      const blink::WebString& sink_id,
      const blink::WebSecurityOrigin& security_origin);

  ~WebMediaPlayerMS() override;

  void Load(LoadType load_type,
            const blink::WebMediaPlayerSource& source,
            CORSMode cors_mode) override;
  void OnSuspendingDecoder(
      scoped_refptr<media::SuspendingDecoder> suspending_decoder);
  void OnDecoderData(
      scoped_refptr<media::MediaStreamVideoDecoderData> decoder_data);
  void OnFrame(scoped_refptr<media::VideoFrame> frame);

#if defined(TIZEN_MULTIMEDIA_SUPPORT)
  void Suspend() override;
  void Resume() override;
#endif

  // Playback controls.
  void Play() override;
  void Pause() override;
  bool SupportsSave() const override;
  void Seek(double seconds) override;
  void SetRate(double rate) override;
  void SetVolume(double volume) override;
  void SetSinkId(const blink::WebString& sink_id,
                 const blink::WebSecurityOrigin& security_origin,
                 blink::WebSetSinkIdCallbacks* web_callback) override;
  void SetPreload(blink::WebMediaPlayer::Preload preload) override;
  blink::WebTimeRanges Buffered() const override;
  blink::WebTimeRanges Seekable() const override;

  // Methods for painting.
  void Paint(blink::WebCanvas* canvas,
             const blink::WebRect& rect,
             cc::PaintFlags& flags,
             int already_uploaded_id,
             VideoFrameUploadMetadata* out_metadata) override;
  media::SkCanvasVideoRenderer* GetSkCanvasVideoRenderer();
  void ResetCanvasCache();

  // Methods to trigger resize event.
  void TriggerResize();

  // True if the loaded media has a playable video/audio track.
  bool HasVideo() const override;
  bool HasAudio() const override;

  // Dimensions of the video.
  blink::WebSize NaturalSize() const override;

  blink::WebSize VisibleRect() const override;

  // Getters of playback state.
  bool Paused() const override;
  bool Seeking() const override;
  double Duration() const override;
  double CurrentTime() const override;

  // Internal states of loading and network.
  blink::WebMediaPlayer::NetworkState GetNetworkState() const override;
  blink::WebMediaPlayer::ReadyState GetReadyState() const override;
  blink::WebMediaPlayer::DecoderType GetDecoderType() const override;

  blink::WebString GetErrorMessage() const override;
  bool DidLoadingProgress() override;

  bool HasSingleSecurityOrigin() const override;
  bool DidPassCORSAccessCheck() const override;

  double MediaTimeForTimeValue(double timeValue) const override;

  unsigned DecodedFrameCount() const override;
  unsigned DroppedFrameCount() const override;
  size_t AudioDecodedByteCount() const override;
  size_t VideoDecodedByteCount() const override;

  // WebMediaPlayerDelegate::Observer implementation.
  void OnFrameHidden() override;
  void OnFrameClosed() override;
  void OnFrameShown() override;
  void OnIdleTimeout() override;
  void OnPlay() override;
  void OnPause() override;
  void OnVolumeMultiplierUpdate(double multiplier) override;
  void OnBecamePersistentVideo(bool value) override;

  bool CopyVideoTextureToPlatformTexture(
      gpu::gles2::GLES2Interface* gl,
      unsigned target,
      unsigned int texture,
      unsigned internal_format,
      unsigned format,
      unsigned type,
      int level,
      bool premultiply_alpha,
      bool flip_y,
      int already_uploaded_id,
      VideoFrameUploadMetadata* out_metadata) override;

  bool TexImageImpl(TexImageFunctionID functionID,
                    unsigned target,
                    gpu::gles2::GLES2Interface* gl,
                    unsigned int texture,
                    int level,
                    int internalformat,
                    unsigned format,
                    unsigned type,
                    int xoffset,
                    int yoffset,
                    int zoffset,
                    bool flip_y,
                    bool premultiply_alpha) override;

  // blink::WebMediaStreamObserver implementation
  void TrackAdded(const blink::WebMediaStreamTrack& track) override;
  void TrackRemoved(const blink::WebMediaStreamTrack& track) override;

  void OnMediaDataChange(int width, int height, int media) override;
  void OnTimeUpdate(base::TimeDelta current_time) override;

#if defined(TIZEN_VIDEO_HOLE)
  // Video hole related implementation. It calculates a postion of video hole
  // frame and passes it to the player that renders on video frame.
  void FrameReady(const scoped_refptr<media::VideoFrame>& frame);
  // Creates a trasparent frame that is rendered over an actual video hole
  void CreateVideoHoleFrame();

  // Determines if video hole frame should be created
  bool ShouldCreateVideoHoleFrame() const;

  // Timer that checks if compositor boundaries have been changed.
  void StartLayerBoundUpdateTimer();
  void StopLayerBoundUpdateTimer();
  void OnLayerBoundUpdateTimerFired();

  // Calculates a current compositor boundaries. Returns true if boundaries has
  // changed.
  bool UpdateBoundaryRectangle();

  // Gets a last calculated compositor boundaries.
  gfx::RectF GetBoundaryRectangle() const;

  // Callback called when position frame compositor changes
  void OnDrawableContentRectChanged(gfx::Rect rect, bool is_video);
#endif

  void OnPlayerDestroyed() override;

#if defined(OS_TIZEN_TV_PRODUCT)
  void OnVideoSlotsAvailableChange(unsigned slots_available) final;
#endif

 private:
  friend class WebMediaPlayerMSTest;

  void SuspendInternal(bool external_suspend = true);
  void ResumeInternal(bool external_resume = true);

  void OnFirstFrameReceived(media::VideoRotation video_rotation,
                            bool is_opaque);
  void OnOpacityChanged(bool is_opaque);
  void OnRotationChanged(media::VideoRotation video_rotation, bool is_opaque);

  // Need repaint due to state change.
  void RepaintInternal();

  // The callback for source to report error.
  void OnSourceError();

  // Helpers that set the network/ready state and notifies the client if
  // they've changed.
  void SetNetworkState(blink::WebMediaPlayer::NetworkState state);
  void SetReadyState(blink::WebMediaPlayer::ReadyState state);
  void SetReadyStateInternal(blink::WebMediaPlayer::ReadyState state);
  void UpgradeReadyState();
  void SetDecoderType(blink::WebMediaPlayer::DecoderType state);

  // Getter method to |client_|.
  blink::WebMediaPlayerClient* get_client() { return client_; }

  // To be run when tracks are added or removed.
  void Reload();
  void ReloadVideo();
  void ReloadAudio();

  blink::WebLocalFrame* const frame_;

  blink::WebMediaPlayer::NetworkState network_state_;
  blink::WebMediaPlayer::ReadyState ready_state_;
  blink::WebMediaPlayer::DecoderType decoder_type_;

  const blink::WebTimeRanges buffered_;

  blink::WebMediaPlayerClient* const client_;

  // WebMediaPlayer notifies the |delegate_| of playback state changes using
  // |delegate_id_|; an id provided after registering with the delegate.  The
  // WebMediaPlayer may also receive directives (play, pause) from the delegate
  // via the WebMediaPlayerDelegate::Observer interface after registration.
  //
  // NOTE: HTMLMediaElement is a Blink::SuspendableObject, and will receive a
  // call to contextDestroyed() when Blink::Document::shutdown() is called.
  // Document::shutdown() is called before the frame detaches (and before the
  // frame is destroyed). RenderFrameImpl owns of |delegate_|, and is guaranteed
  // to outlive |this|. It is therefore safe use a raw pointer directly.
  media::WebMediaPlayerDelegate* delegate_;
  int delegate_id_;

  // Inner class used for transfering frames on compositor thread to
  // |compositor_|.
  class FrameDeliverer;
  std::unique_ptr<FrameDeliverer> frame_deliverer_;

  static void OnFrameRendered_Noop(const base::WeakPtr<WebMediaPlayerMS>&,
                                   base::SingleThreadTaskRunner&) {}
  static void OnFrameRendered_PauseAfterDelivery(
      const base::WeakPtr<WebMediaPlayerMS>&,
      base::SingleThreadTaskRunner&);
  void OnFrameRendered_DoPause();

  static bool WaitingToPause_Always() { return true; }
  static bool WaitingToPause_Never() { return true; }

  static void ResumeOnPlay_IfPossible(WebMediaPlayerMS*);
  static void ResumeOnPlay_Never(WebMediaPlayerMS*) {}

  static void UpgradeDecoder_UpgradeToHardware(WebMediaPlayerMS*);
  static void UpgradeDecoder_StayWithSoftware(WebMediaPlayerMS*) {}

  // To have "true" VTable, we should later-on have a call context with
  // virtual dispatch, through either a reference or a pointer. Which would
  // mean we would have to store a dynamic value to be able to easily
  // achieve this (think: heap, new and delete).
  //
  // Instead, this code opts for a simple pointer to a free-function. This
  // has an advantage over the compiler-provided VTable of not needing a
  // dynamic value to get the virtual dispatch -- it's just a struct of
  // addresses to jump to.
  //
  // A free function getting the "this" pointer explicitly (as in
  // "void (*ptr)(WebMediaPlayerMS*)"), instead of implicit "this" passed
  // through pointer to a member function (as in "void (WebMediaPlayerMS::*
  // ptr)()") has nearly the same semantics, slightly more readable syntax
  // (compare
  //     (this ->* frames_handler_.OnFrameRendered)();
  // with
  //     frames_handler_.OnFrameRendered(this);
  // ) and finally, calling through pointer to free function should be quicker,
  // as code at the calling site, when extracting address to call from pointer
  // to member function variable, must take into account that the current value
  // of the variable points not to any function directly, but rather holds
  // offset into VTable, with one additional branch for the test and possibly
  // one additional dereference (one from member function variable, another
  // from VTable).

  struct FrameHandlerIface {
    void (*OnFrameRendered)(const base::WeakPtr<WebMediaPlayerMS>&,
                            base::SingleThreadTaskRunner&);
    bool (*WaitingToPause)();
    void (*ResumeOnPlay)(WebMediaPlayerMS*);
  };

  static constexpr FrameHandlerIface DefaultFrameHandler() {
    return {OnFrameRendered_Noop, WaitingToPause_Never, ResumeOnPlay_Never};
  }

  static constexpr FrameHandlerIface WaitingForImageFrameHandler() {
    return {OnFrameRendered_PauseAfterDelivery, WaitingToPause_Always,
            ResumeOnPlay_Never};
  }

  static constexpr FrameHandlerIface ResumeOnPlayFrameHandler() {
    return {OnFrameRendered_Noop, WaitingToPause_Never,
            ResumeOnPlay_IfPossible};
  }

  FrameHandlerIface frames_handler_ = DefaultFrameHandler();

  void (*upgrade_decoder_)(WebMediaPlayerMS*) =
      UpgradeDecoder_UpgradeToHardware;
  inline void UpgradeDecoder() { upgrade_decoder_(this); }

  static void OnFrameRendered(const base::WeakPtr<WebMediaPlayerMS>& self,
                              base::SingleThreadTaskRunner& runner) {
    if (self)
      self->frames_handler_.OnFrameRendered(self, runner);
  }

  scoped_refptr<MediaStreamVideoRenderer> video_frame_provider_; // Weak

  std::unique_ptr<cc_blink::WebLayerImpl> video_weblayer_;

  scoped_refptr<MediaStreamAudioRenderer> audio_renderer_; // Weak
  media::SkCanvasVideoRenderer video_renderer_;
  bool received_first_frame_{false};
  bool paused_;
  media::VideoRotation video_rotation_;
  base::TimeDelta current_time_;

  std::unique_ptr<media::MediaLog> media_log_;

  std::unique_ptr<MediaStreamRendererFactory> renderer_factory_;


  const scoped_refptr<base::SingleThreadTaskRunner> io_task_runner_;
  const scoped_refptr<base::SingleThreadTaskRunner> compositor_task_runner_;

#if defined(TIZEN_MULTIMEDIA_SUPPORT)
  // FrameObserver to handle player page visibility
  RendererMediaPlayerManager* manager_;
  int player_id_;
  content::RendererDemuxerEfl* demuxer_{nullptr};
  int demuxer_client_id_;
#endif

  void SetMediaGeometry(const gfx::RectF& rect);

  media::TizenDecoderType GetTizenDecoderType() const;

  const scoped_refptr<base::SingleThreadTaskRunner> media_task_runner_;
  const scoped_refptr<base::TaskRunner> worker_task_runner_;
  media::GpuVideoAcceleratorFactories* gpu_factories_;

  // Used for DCHECKs to ensure methods calls executed in the correct thread.
  base::ThreadChecker thread_checker_;

  scoped_refptr<WebMediaPlayerMSCompositor> compositor_;
  scoped_refptr<media::MediaStreamVideoDecoderData> decoder_data_;
  scoped_refptr<media::SuspendingDecoder> suspending_decoder_;

  const std::string initial_audio_output_device_id_;
  const url::Origin initial_security_origin_;

  gfx::Size natural_size_ = {0, 0};

#if defined(TIZEN_VIDEO_HOLE)
  bool is_video_hole_ = false;
  gfx::RectF last_computed_rect_;
  base::RepeatingTimer layer_bound_update_timer_;
#endif

  // The last volume received by setVolume() and the last volume multiplier from
  // OnVolumeMultiplierUpdate().  The multiplier is typical 1.0, but may be less
  // if the WebMediaPlayerDelegate has requested a volume reduction (ducking)
  // for a transient sound.  Playout volume is derived by volume * multiplier.
  double volume_;
  double volume_multiplier_;

  // True if playback should be started upon the next call to OnShown(). Only
  // used on Android.
  bool should_play_upon_shown_;
  blink::WebMediaStream web_stream_;
  // IDs of the tracks currently played.
  blink::WebString current_video_track_id_;
  blink::WebString current_audio_track_id_;

  bool suspended_{false};
  bool is_decoder_suspending_{false};
  media::TizenDecoderType decoder_type_before_suspend_{
      media::TizenDecoderType::UNKNOWN};

  DISALLOW_COPY_AND_ASSIGN(WebMediaPlayerMS);
};

}  // namespace content

#endif  // CONTENT_RENDERER_MEDIA_WEBMEDIAPLAYER_MS_H_
