// Copyright 2015 Samsung Electronics Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_MEDIA_TIZEN_WEBMEDIAPLAYER_EFL_H_
#define CONTENT_RENDERER_MEDIA_TIZEN_WEBMEDIAPLAYER_EFL_H_

#include <vector>

#include "base/memory/shared_memory.h"
#include "base/message_loop/message_loop.h"
#include "cc/layers/video_frame_provider_client_impl.h"
#include "content/renderer/media/tizen/media_source_delegate_efl.h"
#include "content/renderer/media/tizen/renderer_media_player_manager_efl.h"
#include "media/base/cdm_context.h"
#include "media/base/content_decryption_module.h"
#include "media/base/tizen/media_player_efl.h"
#include "media/base/video_frame.h"
#include "media/blink/video_frame_compositor.h"
#include "media/blink/webmediaplayer_delegate.h"
#include "media/blink/webmediaplayer_params.h"
#include "media/blink/webmediaplayer_util.h"
#include "media/renderers/skcanvas_video_renderer.h"
#include "third_party/WebKit/public/platform/WebAudioSourceProvider.h"
#include "third_party/WebKit/public/platform/WebMediaPlayer.h"
#include "third_party/WebKit/public/platform/WebMediaPlayerEncryptedMediaClient.h"
#include "third_party/WebKit/public/web/WebLocalFrame.h"

#if defined(OS_TIZEN_TV_PRODUCT)
#define USE_NO_WAIT_SIGNAL_MEDIA_PACKET_THREAD 1
#endif

namespace blink {
class WebFrame;
class WebMediaPlayerClient;
}  // namespace blink

namespace cc_blink {
class WebLayerImpl;
}

namespace media {
class MediaLog;
class WebAudioSourceProviderImpl;
class WebMediaPlayerDelegate;
class WebMediaPlayerEncryptedMediaClient;
}  // namespace media

namespace content {
class RendererMediaPlayerManager;

// This class implements blink::WebMediaPlayer by keeping the efl
// media player in the browser process. It listens to all the status changes
// sent from the browser process and sends playback controls to the media
// player.
class WebMediaPlayerEfl : public blink::WebMediaPlayer,
                          public base::SupportsWeakPtr<WebMediaPlayerEfl>,
                          public media::WebMediaPlayerDelegate::Observer {
 public:
  // Construct a WebMediaPlayerEfl object. This class communicates
  // with the WebMediaPlayerEfl object in the browser process through
  // |proxy|.
  WebMediaPlayerEfl(blink::WebLocalFrame* web_frame,
                    RendererMediaPlayerManager* manager,
                    blink::WebMediaPlayerClient* client,
                    blink::WebMediaPlayerEncryptedMediaClient* encrypted_client,
                    base::WeakPtr<media::WebMediaPlayerDelegate> delegate,
                    std::unique_ptr<media::WebMediaPlayerParams> params,
                    bool video_hole);
  ~WebMediaPlayerEfl() override;

  void Load(LoadType load_type,
            const blink::WebMediaPlayerSource& source,
            CORSMode cors_mode) override;

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
  void SetPoster(const blink::WebURL& poster) override{};
  void SetPreload(blink::WebMediaPlayer::Preload preload) override;
  blink::WebTimeRanges Buffered() const override;
  blink::WebTimeRanges Seekable() const override;

  // paint() the current video frame into |canvas|. This is used to support
  // various APIs and functionalities, including but not limited to: <canvas>,
  // WebGL texImage2D, ImageBitmap, printing and capturing capabilities.
  void Paint(blink::WebCanvas* canvas,
             const blink::WebRect& rect,
             cc::PaintFlags& flags,
             int already_uploaded_id,
             VideoFrameUploadMetadata* out_metadata) override;

  // True if the loaded media has a playable video/audio track.
  bool HasVideo() const override;
  bool HasAudio() const override;

  void EnabledAudioTracksChanged(
      const blink::WebVector<blink::WebMediaPlayer::TrackId>& enabledTrackIds)
      override;
  void SelectedVideoTrackChanged(
      blink::WebMediaPlayer::TrackId* selectedTrackId) override;

  // Dimensions of the video.
  blink::WebSize NaturalSize() const override;

  blink::WebSize VisibleRect() const override;

  // Getters of playback state.
  bool Paused() const override;
  bool Seeking() const override;
  double Duration() const override;
  double CurrentTime() const override;
  double GetStartDate() const override;
#if defined(TIZEN_MULTIMEDIA_SUPPORT)
  void Suspend() override;
  void Resume() override;
  void Activate() override;
  void Deactivate() override;
#endif

#if defined(OS_TIZEN_TV_PRODUCT)
  void GetPlayTrackInfo(int id);
  void OnDrmError() override;
  void OnSeekableTimeChange(base::TimeDelta min_time,
                            base::TimeDelta max_time) override;
  bool RegisterTimeline(const std::string& timeline_selector) override;
  bool UnRegisterTimeline(const std::string& timeline_selector) override;
  void GetTimelinePositions(const std::string& timeline_selector,
                            uint32_t* units_per_tick,
                            uint32_t* units_per_second,
                            int64_t* content_time,
                            bool* paused) override;
  double GetSpeed() override;
  std::string GetMrsUrl() override;
  std::string GetContentId() override;
  bool SyncTimeline(const std::string& timeline_selector,
                    int64_t timeline_pos,
                    int64_t wallclock_pos,
                    int tolerance) override;
  bool SetWallClock(const std::string& wallclock_url) override;

  void OnSyncTimeline(const std::string& timeline_selector, int sync);
  void OnRegisterTimeline(
      const blink::WebMediaPlayer::register_timeline_cb_info_s& info);
  void OnMrsUrlChange(const std::string& url);
  void OnContentIdChange(const std::string& id);
  void OnAddAudioTrack(
      const blink::WebMediaPlayer::audio_video_track_info_s& info);
  void OnAddVideoTrack(
      const blink::WebMediaPlayer::audio_video_track_info_s& info);
  void OnAddTextTrack(const std::string& label,
                      const std::string& language,
                      const std::string& id);
  void SetActiveTextTrack(int id, bool is_in_band) override;
  void SetActiveAudioTrack(int index) override;
  void SetActiveVideoTrack(int index) override;
  void OnInbandEventTextTrack(const std::string& info, int action) override;
  void OnInbandEventTextCue(const std::string& info,
                            unsigned int id,
                            long long int start_time,
                            long long int end_time) override;
  void NotifyElementRemove() override;
  void OnPlayerStarted(bool player_started) override;
  void SetParentalRatingResult(bool is_pass) override;
  bool hasEncryptedListenerOrCdm() const override;
  void SetCDMPlayerType() const;
  void SetDecryptorHandle() const;
  void OnNewKeyAdded() const;
  bool IsSynchronousPlayerDestructionNeeded() const;
#endif

  // Internal states of loading and network.
  // TODO(hclam): Ask the pipeline about the state rather than having reading
  // them from members which would cause race conditions.
  blink::WebMediaPlayer::NetworkState GetNetworkState() const override;
  blink::WebMediaPlayer::ReadyState GetReadyState() const override;

  blink::WebString GetErrorMessage() const override;
  bool DidLoadingProgress() override;

  bool HasSingleSecurityOrigin() const override;
  bool DidPassCORSAccessCheck() const override;

  double MediaTimeForTimeValue(double timeValue) const override;

  unsigned DecodedFrameCount() const override;
  unsigned DroppedFrameCount() const override;
  size_t AudioDecodedByteCount() const override;
  size_t VideoDecodedByteCount() const override;

  bool CopyVideoTextureToPlatformTexture(
      gpu::gles2::GLES2Interface* gl,
      unsigned int target,
      unsigned int texture,
      unsigned internal_format,
      unsigned format,
      unsigned type,
      int level,
      bool premultiply_alpha,
      bool flip_y,
      int already_uploaded_id,
      VideoFrameUploadMetadata* out_metadata) override;

#if defined(TIZEN_TBM_SUPPORT) && defined(OS_TIZEN_TV_PRODUCT)
  static base::Timer& DeferredChangeVideoResolutionTimer(void);
  static void DeferredChangeVideoResolution(WebMediaPlayerEfl* web_player);
  bool RenderRingBuffer(gpu::gles2::GLES2Interface* gl, unsigned& image_id);
  bool ZeroCopyVideoTbmToPlatformTexture(gpu::gles2::GLES2Interface*,
                                         unsigned,
                                         unsigned&) override;
  void DeleteVideoFrame(unsigned) override;
#endif

#if defined(USE_NO_WAIT_SIGNAL_MEDIA_PACKET_THREAD)
  void SetCurrentFrameInternal(scoped_refptr<media::VideoFrame>& frame);
  scoped_refptr<media::VideoFrame> GetCurrentFrame();
#endif

  static void ComputeFrameUploadMetadata(
      media::VideoFrame* frame,
      int already_uploaded_id,
      VideoFrameUploadMetadata* out_metadata);

  blink::WebAudioSourceProvider* GetAudioSourceProvider() override;

  void SetContentDecryptionModule(
      blink::WebContentDecryptionModule* cdm,
      blink::WebContentDecryptionModuleResult result) override;

  bool SupportsOverlayFullscreenVideo() override;
  void EnteredFullscreen() override;
  void ExitedFullscreen() override;
  void BecameDominantVisibleContent(bool isDominant) override;
  void SetIsEffectivelyFullscreen(bool isEffectivelyFullscreen) override;
  void OnHasNativeControlsChanged(bool) override;
  void OnDisplayTypeChanged(WebMediaPlayer::DisplayType) override;

  // WebMediaPlayerDelegate::Observer implementation.
  void OnFrameHidden() override;
  void OnFrameClosed() override;
  void OnFrameShown() override;
  void OnIdleTimeout() override;
  void OnPlay() override;
  void OnPause() override;
  void OnVolumeMultiplierUpdate(double multiplier) override;
  void OnBecamePersistentVideo(bool value) override;

  void RequestRemotePlaybackDisabled(bool disabled) override;

  void SetReadyState(WebMediaPlayer::ReadyState state) override;
  void SetNetworkState(WebMediaPlayer::NetworkState state) override;

  void OnNewFrameAvailable(base::SharedMemoryHandle foreign_memory_handle,
                           uint32_t length,
                           base::TimeDelta timestamp) override;

#if defined(TIZEN_TBM_SUPPORT)
  void OnNewTbmBufferAvailable(const gfx::TbmBufferHandle& tbm_handle,
#if defined(OS_TIZEN_TV_PRODUCT)
                               int scaler_physical_id,
#endif
                               base::TimeDelta timestamp,
                               const base::Closure& cb) override;
#if defined(OS_TIZEN_TV_PRODUCT)
  void ControlMediaPacket(int mode);
#endif
#endif

#if defined(TIZEN_VIDEO_HOLE)
  void OnDrawableContentRectChanged(gfx::Rect rect, bool is_video);
#endif
  void OnMediaDataChange(int width, int height, int media) override;
  void OnDurationChange(base::TimeDelta duration) override;
  void OnTimeUpdate(base::TimeDelta current_time) override;
  void OnBufferUpdate(base::TimeDelta current_time);
  void OnBufferUpdate(int percentage) override;
  void OnTimeChanged() override;
  void OnPauseStateChange(bool state) override;
  void OnSeekComplete() override;
  void OnRequestSeek(base::TimeDelta seek_time) override;

  void OnMediaSourceOpened(blink::WebMediaSource* web_media_source);
  void OnEncryptedMediaInitData(media::EmeInitDataType init_data_type,
                                const std::vector<uint8_t>& init_data);
  void SetCdmReadyCB(const MediaSourceDelegateEfl::CdmReadyCB& cdm_ready_cb);
  void OnDemuxerSeekDone();

  void RequestPause() override;
  void ReleaseMediaResource();
  void InitializeMediaResource();
  void OnPlayerSuspend(bool is_preempted) override;
  void OnPlayerResumed(bool is_preempted) override;

  // Called when a decoder detects that the key needed to decrypt the stream
  // is not available.
  void OnWaitingForDecryptionKey();
#if defined(TIZEN_VIDEO_HOLE)
  void CreateVideoHoleFrame();

  // Calculate the boundary rectangle of the media player (i.e. location and
  // size of the video frame).
  // Returns true if the geometry has been changed since the last call.
  bool UpdateBoundaryRectangle();
  gfx::RectF GetBoundaryRectangle() const;
  void StartLayerBoundUpdateTimer();
  void StopLayerBoundUpdateTimer();
  void OnLayerBoundUpdateTimerFired();
  bool ShouldCreateVideoHoleFrame() const;
#endif
 private:
  void setCdm(media::CdmContext* cdm_reference);
  void SetDecryptorCb();

  // Called after |defer_load_cb_| has decided to allow the load. If
  // |defer_load_cb_| is null this is called immediately.
  void DoLoad(LoadType load_type, const blink::WebURL& url);
  void PauseInternal(bool is_media_related_action);

  void OnNaturalSizeChanged(gfx::Size size);
  void OnOpacityChanged(bool opaque);

  // Returns the current video frame from |compositor_|. Blocks until the
  // compositor can return the frame.
  scoped_refptr<media::VideoFrame> GetCurrentFrameFromCompositor() const;

  // Called whenever there is new frame to be painted.
  void FrameReady(const scoped_refptr<media::VideoFrame>& frame);

  blink::WebLocalFrame* const frame_;

  blink::WebMediaPlayer::NetworkState network_state_;
  blink::WebMediaPlayer::ReadyState ready_state_;

  // Message loops for posting tasks on Chrome's main thread. Also used
  // for DCHECKs so methods calls won't execute in the wrong thread.
  const scoped_refptr<base::SingleThreadTaskRunner> main_task_runner_;
  scoped_refptr<base::SingleThreadTaskRunner> media_task_runner_;

  // Manager for managing this object and for delegating method calls on
  // Render Thread.
  content::RendererMediaPlayerManager* manager_;

  blink::WebMediaPlayerClient* client_;
  blink::WebMediaPlayerEncryptedMediaClient* encrypted_client_;

  std::unique_ptr<media::MediaLog> media_log_;

  base::WeakPtr<media::WebMediaPlayerDelegate> delegate_;

  media::WebMediaPlayerParams::DeferLoadCB defer_load_cb_;
  scoped_refptr<viz::ContextProvider> context_provider_;
  // Video rendering members.
  // The |compositor_| runs on the compositor thread, or if
  // kEnableSurfaceLayerForVideo is enabled, the media thread. This task runner
  // posts tasks for the |compositor_| on the correct thread.
  scoped_refptr<base::SingleThreadTaskRunner> compositor_task_runner_;

  // Deleted on |compositor_task_runner_|.
  std::unique_ptr<media::VideoFrameCompositor> compositor_;
  media::SkCanvasVideoRenderer skcanvas_video_renderer_;

  // The compositor layer for displaying the video content when using composited
  // playback.
  std::unique_ptr<cc_blink::WebLayerImpl> video_weblayer_;

  std::unique_ptr<content::MediaSourceDelegateEfl> media_source_delegate_;
  MediaPlayerHostMsg_Initialize_Type player_type_;

#if defined(OS_TIZEN_TV_PRODUCT)
  bool had_encrypted_listener_;
  bool is_live_stream_;
  base::TimeDelta min_seekable_time_;
  base::TimeDelta max_seekable_time_;
  bool force_update_playback_position_;
#endif

  // References the CDM while player is alive (keeps cdm_context_ valid)
  scoped_refptr<media::ContentDecryptionModule> pending_cdm_;

#if defined(TIZEN_TBM_SUPPORT) && defined(OS_TIZEN_TV_PRODUCT)
  typedef base::hash_map<unsigned, scoped_refptr<media::VideoFrame>>
      VideoHandlesMap;
  VideoHandlesMap video_handles_map_;
#endif

#if defined(USE_NO_WAIT_SIGNAL_MEDIA_PACKET_THREAD)
  // The video frame object used for rendering by the compositor.
  scoped_refptr<media::VideoFrame> current_frame_;
  base::Lock current_frame_lock_;
#endif

  // Player ID assigned by the |manager_|.
  int player_id_;

  int video_width_;
  int video_height_;

  bool audio_;
  bool video_;

  base::TimeDelta current_time_;
  base::TimeDelta duration_;
  bool is_paused_;

  bool is_seeking_;
  base::TimeDelta seek_time_;
  bool pending_seek_;
  base::TimeDelta pending_seek_time_;

  // Whether the video is known to be opaque or not.
  bool opaque_;
  bool is_fullscreen_;

#if defined(TIZEN_VIDEO_HOLE)
  bool is_draw_ready_;
  bool pending_play_;
  bool is_video_hole_;

  // A rectangle represents the geometry of video frame, when computed last
  // time.
  gfx::RectF last_computed_rect_;
  base::RepeatingTimer layer_bound_update_timer_;
#endif

#if defined(TIZEN_TBM_SUPPORT) && defined(OS_TIZEN_TV_PRODUCT)
  bool is_video_texturing_;
  bool timeout_lock_flag_;
#ifdef SCALER_DEBUG
  base::TimeDelta previous_media_packet_timestamp_;
  base::TimeDelta frame_rate_control_timestamp_;
  base::Time previous_frame_time_;
#endif
  int previous_index_;
  int scaler_lock_mode_;
  uint32_t scaler_buffer_max_;
  bool is_gpu_vendor_kantm2_used_;
#endif

  gfx::Size natural_size_;
  blink::WebTimeRanges buffered_;
  mutable bool did_loading_progress_;
  int delegate_id_;

  // The last volume received by setVolume() and the last volume multiplier from
  // OnVolumeMultiplierUpdate(). The multiplier is typical 1.0, but may be less
  // if the WebMediaPlayerDelegate has requested a volume reduction (ducking)
  // for a transient sound.  Playout volume is derived by volume * multiplier.
  double volume_;
  double volume_multiplier_;

  // Non-owned pointer to the CdmContext. Updated in the constructor
  // or setContentDecryptionModule().
  media::CdmContext* cdm_context_;
  media::EmeInitDataType init_data_type_;
  MediaSourceDelegateEfl::CdmReadyCB pending_cdm_ready_cb_;

  base::WeakPtrFactory<WebMediaPlayerEfl> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(WebMediaPlayerEfl);
};

}  // namespace content

#endif  // CONTENT_RENDERER_MEDIA_TIZEN_WEBMEDIAPLAYER_EFL_H_
