// Copyright 2015 Samsung Electronics Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/media/tizen/webmediaplayer_efl.h"

#include "cc/blink/web_layer_impl.h"
#include "cc/layers/video_layer.h"
#include "content/public/renderer/content_renderer_client.h"
#include "content/renderer/media/render_media_log.h"
#include "content/renderer/render_frame_impl.h"
#include "content/renderer/render_thread_impl.h"
#include "media/base/bind_to_current_loop.h"
#include "media/base/content_decryption_module.h"
#include "media/base/media_content_type.h"
#include "media/base/tizen/media_player_interface_efl.h"
#include "media/base/tizen/media_player_util_efl.h"
#include "media/base/video_frame.h"
#include "media/blink/webcontentdecryptionmodule_impl.h"
#include "media/blink/webmediaplayer_util.h"
#include "third_party/WebKit/public/platform/WebMediaPlayer.h"
#include "third_party/WebKit/public/platform/WebMediaPlayerClient.h"
#include "third_party/WebKit/public/platform/WebMediaPlayerSource.h"
#include "third_party/WebKit/public/platform/WebMediaSource.h"
#include "third_party/WebKit/public/platform/WebURL.h"
#include "third_party/libyuv/include/libyuv/planar_functions.h"
#include "tizen_src/ewk/efl_integration/renderer/tizen_extensible.h"

#if defined(TIZEN_VIDEO_HOLE)
#include "third_party/WebKit/public/web/WebLocalFrame.h"
#include "third_party/WebKit/public/web/WebView.h"
#endif

#if defined(OS_TIZEN_TV_PRODUCT)
#include "content/common/media/media_player_messages_enums_efl.h"
#include "media/blink/webinbandtexttrack_impl.h"
#include "tizen_src/ewk/efl_integration/common/application_type.h"
#if defined(TIZEN_TBM_SUPPORT)
#include "base/threading/platform_thread.h"
#include "gpu/GLES2/gl2extchromium.h"
#endif
#endif

#if defined(OS_TIZEN_TV_PRODUCT)
typedef struct {
  const std::string kind_str_;
  blink::WebMediaPlayerClient::AudioTrackKind kind_enum_;
} AudioKind;

static AudioKind audioKinds[] = {
    {"alternative", blink::WebMediaPlayerClient::kAudioTrackKindAlternative},
    {"descriptions", blink::WebMediaPlayerClient::kAudioTrackKindDescriptions},
    {"main", blink::WebMediaPlayerClient::kAudioTrackKindMain},
    {"main-desc", blink::WebMediaPlayerClient::kAudioTrackKindMainDescriptions},
    {"translation", blink::WebMediaPlayerClient::kAudioTrackKindTranslation},
    {"commentary", blink::WebMediaPlayerClient::kAudioTrackKindCommentary}};

typedef struct {
  const std::string kind_str_;
  blink::WebMediaPlayerClient::VideoTrackKind kind_enum_;
} VideoKind;

static VideoKind videoKinds[] = {
    {"alternative", blink::WebMediaPlayerClient::kVideoTrackKindAlternative},
    {"captions", blink::WebMediaPlayerClient::kVideoTrackKindCaptions},
    {"main", blink::WebMediaPlayerClient::kVideoTrackKindMain},
    {"sign", blink::WebMediaPlayerClient::kVideoTrackKindSign},
    {"subtitles", blink::WebMediaPlayerClient::kVideoTrackKindSubtitles},
    {"commentary", blink::WebMediaPlayerClient::kVideoTrackKindCommentary}};
#endif

#define BIND_TO_RENDER_LOOP(function)                   \
  (DCHECK(main_task_runner_->BelongsToCurrentThread()), \
   media::BindToCurrentLoop(base::Bind(function, AsWeakPtr())))

// Round up 'x' to Multiple of 'a' byte.
// To properly use planes from tbm, width and height should round up to 16.
#define ALIGN(x, a) (((x) + (a)-1) & ~((a)-1))

#if defined(TIZEN_TBM_SUPPORT) && defined(OS_TIZEN_TV_PRODUCT)
const base::TimeDelta kVideoResolutionChangeInterval =
    base::TimeDelta::FromMilliseconds(1000);
#ifdef SCALER_DEBUG
const base::TimeDelta kCheckGpuDelayInterval =
    base::TimeDelta::FromMilliseconds(33);
#endif
#endif

using media::VideoFrame;

namespace {
const base::TimeDelta kLayerBoundUpdateInterval =
    base::TimeDelta::FromMilliseconds(50);
}

namespace content {

media::CdmContext* GetCdmContext(blink::WebContentDecryptionModule* cdm) {
  if (!cdm) {
    LOG(INFO) << "Cdm context is null";
    return nullptr;
  }
  return media::ToWebContentDecryptionModuleImpl(cdm)
      ->GetCdm()
      ->GetCdmContext();
}

#if defined(TIZEN_TBM_SUPPORT)
static void OnTbmBufferExhausted(media::VideoFrame* video_frame,
                                 const base::Closure& cb) {
  if (!video_frame->GetTbmTexture())
    cb.Run();
}
#endif

#if defined(OS_TIZEN_TV_PRODUCT)
static blink::WebInbandTextTrack* CreateWebInbandTextTrack(
    const std::string label,
    const std::string language,
    const std::string id) {
  return new media::WebInbandTextTrackImpl(
      static_cast<media::WebInbandTextTrackImpl::Kind>(
          blink::WebInbandTextTrack::Kind::kKindSubtitles),
      blink::WebString::FromUTF8(label), blink::WebString::FromUTF8(language),
      blink::WebString::FromUTF8(id));
}

bool WebMediaPlayerEfl::hasEncryptedListenerOrCdm() const {
  return client_->HasEncryptedListener() || (cdm_context_ != nullptr);
}
#endif

blink::WebMediaPlayer* CreateWebMediaPlayerEfl(
    blink::WebLocalFrame* frame,
    content::RendererMediaPlayerManager* manager,
    blink::WebMediaPlayerClient* client,
    blink::WebMediaPlayerEncryptedMediaClient* encrypted_client,
    base::WeakPtr<media::WebMediaPlayerDelegate> delegate,
    std::unique_ptr<media::WebMediaPlayerParams> params,
    bool video_hole) {
  return new WebMediaPlayerEfl(frame, manager, client, encrypted_client,
                               delegate, std::move(params), video_hole);
}

WebMediaPlayerEfl::WebMediaPlayerEfl(
    blink::WebLocalFrame* frame,
    RendererMediaPlayerManager* manager,
    blink::WebMediaPlayerClient* client,
    blink::WebMediaPlayerEncryptedMediaClient* encrypted_client,
    base::WeakPtr<media::WebMediaPlayerDelegate> delegate,
    std::unique_ptr<media::WebMediaPlayerParams> params,
    bool video_hole)
    : frame_(frame),
      network_state_(blink::WebMediaPlayer::kNetworkStateEmpty),
      ready_state_(blink::WebMediaPlayer::kReadyStateHaveNothing),
      main_task_runner_(base::ThreadTaskRunnerHandle::Get()),
      media_task_runner_(params->media_task_runner()),
      manager_(manager),
      client_(client),
      encrypted_client_(encrypted_client),
      media_log_(params->take_media_log()),
      delegate_(delegate),
      defer_load_cb_(params->defer_load_cb()),
      context_provider_(params->context_provider()),
      // Threaded compositing isn't enabled universally yet.
      compositor_task_runner_(params->compositor_task_runner()
                                  ? params->compositor_task_runner()
                                  : base::ThreadTaskRunnerHandle::Get()),
      compositor_(base::MakeUnique<media::VideoFrameCompositor>(
#if defined(TIZEN_VIDEO_HOLE)
          BIND_TO_RENDER_LOOP(&WebMediaPlayerEfl::OnDrawableContentRectChanged),
#endif
          compositor_task_runner_,
          params->context_provider_callback())),
      player_type_(MEDIA_PLAYER_TYPE_NONE),
#if defined(OS_TIZEN_TV_PRODUCT)
      had_encrypted_listener_(false),
      is_live_stream_(false),
#endif
      video_width_(0),
      video_height_(0),
      audio_(false),
      video_(false),
      is_paused_(true),
      is_seeking_(false),
      pending_seek_(false),
      opaque_(false),
      is_fullscreen_(false),
#if defined(TIZEN_VIDEO_HOLE)
      is_draw_ready_(false),
      pending_play_(false),
      is_video_hole_(video_hole),
#endif
#if defined(TIZEN_TBM_SUPPORT) && defined(OS_TIZEN_TV_PRODUCT)
#ifdef SCALER_DEBUG
      previous_media_packet_timestamp_(base::TimeDelta()),
      frame_rate_control_timestamp_(kCheckGpuDelayInterval),
      previous_frame_time_(base::Time()),
      previous_index_(-1),
#endif
      scaler_lock_mode_(gfx::MEDIA_PACKET_NORMAL_MODE),
      scaler_buffer_max_(5),
      is_gpu_vendor_kantm2_used_(false),
#endif
      natural_size_(0, 0),
      buffered_(static_cast<size_t>(1)),
      did_loading_progress_(false),
      delegate_id_(0),
      volume_(1.0),
      volume_multiplier_(1.0),
      cdm_context_(GetCdmContext(params->initial_cdm())),
      init_data_type_(media::EmeInitDataType::UNKNOWN),
      weak_factory_(this) {
  DCHECK(manager_);
  if (delegate_)
    delegate_id_ = delegate_->AddObserver(this);
  player_id_ = manager_->RegisterMediaPlayer(this);

  if (params->initial_cdm())
    pending_cdm_ =
        std::move(media::ToWebContentDecryptionModuleImpl(params->initial_cdm())
                      ->GetCdm());

#if defined(TIZEN_VIDEO_HOLE)
  LOG(INFO) << "initial cdm_context_ is " << cdm_context_
            << ",Video Hole:" << is_video_hole_;
#endif

#if defined(OS_TIZEN_TV_PRODUCT)
  if (content::IsHbbTV() && cdm_context_) {
    SetDecryptorCb();
  }
#endif

  media_log_->AddEvent(
      media_log_->CreateEvent(media::MediaLogEvent::WEBMEDIAPLAYER_CREATED));

#if defined(TIZEN_TBM_SUPPORT) && defined(OS_TIZEN_TV_PRODUCT)
  VideoFrame::GetScalerNumber(&scaler_buffer_max_);
  // Disable scaler lock
  is_gpu_vendor_kantm2_used_ = client_->GetGpuVendorKant2Used();
  LOG(INFO) << "gpu vendor is kantm2 or not: " << is_gpu_vendor_kantm2_used_;
  VideoFrame::LockScalerDuringNextFrame(is_gpu_vendor_kantm2_used_, 0, false);
#endif
}

WebMediaPlayerEfl::~WebMediaPlayerEfl() {
#if defined(TIZEN_TBM_SUPPORT) && defined(OS_TIZEN_TV_PRODUCT)
  DeferredChangeVideoResolutionTimer().Stop();
  if (network_state_ == blink::WebMediaPlayer::kNetworkStateNetworkError ||
      network_state_ == blink::WebMediaPlayer::kNetworkStateDecodeError)
    LOG(INFO) << "network state : error -> retry webgl player.. ";
#endif

  if (manager_) {
#if defined(OS_TIZEN_TV_PRODUCT)
    if (IsSynchronousPlayerDestructionNeeded())
      manager_->DestroyPlayerSync(player_id_);
    else
#endif
      manager_->DestroyPlayer(player_id_);
    manager_->UnregisterMediaPlayer(player_id_);
  }

  if (cdm_context_) {
    cdm_context_->GetDecryptor()->RegisterNewKeyCB(
        media::Decryptor::kVideo, media::Decryptor::NewKeyCB());
    cdm_context_->GetDecryptor()->RegisterNewKeyCB(
        media::Decryptor::kAudio, media::Decryptor::NewKeyCB());
  }

  compositor_->SetVideoFrameProviderClient(NULL);
  client_->SetWebLayer(NULL);

  if (delegate_) {
    delegate_->PlayerGone(delegate_id_);
    delegate_->RemoveObserver(delegate_id_);
  }

  compositor_task_runner_->DeleteSoon(FROM_HERE, std::move(compositor_));
  if (media_source_delegate_) {
    // Part of |media_source_delegate_| needs to be stopped on the media thread.
    // Wait until |media_source_delegate_| is fully stopped before tearing
    // down other objects.
    base::WaitableEvent waiter(base::WaitableEvent::ResetPolicy::AUTOMATIC,
                               base::WaitableEvent::InitialState::NOT_SIGNALED);
    media_source_delegate_->Stop(
        base::Bind(&base::WaitableEvent::Signal, base::Unretained(&waiter)));
    waiter.Wait();
  }

#if defined(USE_NO_WAIT_SIGNAL_MEDIA_PACKET_THREAD)
  current_frame_ = NULL;
#endif
}

void WebMediaPlayerEfl::Load(LoadType load_type,
                             const blink::WebMediaPlayerSource& source,
                             CORSMode /* cors_mode */) {
  // Only URL or MSE blob URL is supported.
  DCHECK(source.IsURL());
  blink::WebURL url = source.GetAsURL();

  if (!defer_load_cb_.is_null()) {
    defer_load_cb_.Run(
        base::Bind(&WebMediaPlayerEfl::DoLoad, AsWeakPtr(), load_type, url));
    return;
  }
  DoLoad(load_type, url);
}

void WebMediaPlayerEfl::DoLoad(LoadType load_type, const blink::WebURL& url) {
  int demuxer_client_id = 0;
  blink::WebString content_mime_type =
      blink::WebString(client_->GetContentMIMEType());
  if (load_type == kLoadTypeMediaSource) {
    player_type_ = MEDIA_PLAYER_TYPE_MEDIA_SOURCE;
    RendererDemuxerEfl* demuxer = static_cast<RendererDemuxerEfl*>(
        RenderThreadImpl::current()->renderer_demuxer());
    demuxer_client_id = demuxer->GetNextDemuxerClientID();
    media_source_delegate_.reset(new MediaSourceDelegateEfl(
        demuxer, demuxer_client_id, media_task_runner_, media_log_.get()));
    media_source_delegate_->InitializeMediaSource(
        base::Bind(&WebMediaPlayerEfl::OnMediaSourceOpened,
                   weak_factory_.GetWeakPtr()),
        base::Bind(&WebMediaPlayerEfl::OnEncryptedMediaInitData,
                   weak_factory_.GetWeakPtr()),
        base::Bind(&WebMediaPlayerEfl::SetCdmReadyCB,
                   weak_factory_.GetWeakPtr()),
        base::Bind(&WebMediaPlayerEfl::SetNetworkState,
                   weak_factory_.GetWeakPtr()),
        base::Bind(&WebMediaPlayerEfl::OnDurationChange,
                   weak_factory_.GetWeakPtr()),
        base::Bind(&WebMediaPlayerEfl::OnWaitingForDecryptionKey,
                   weak_factory_.GetWeakPtr()));
  } else if (load_type == kLoadTypeURL) {
    player_type_ = MEDIA_PLAYER_TYPE_URL;
  } else {
    LOG(ERROR) << "Unsupported load type : " << load_type;
    return;
  }
#if defined(TIZEN_VIDEO_HOLE)
  LOG(INFO) << "Video Hole : " << is_video_hole_;
  if (is_video_hole_)
    player_type_ = (player_type_ == MEDIA_PLAYER_TYPE_URL)
                       ? MEDIA_PLAYER_TYPE_URL_WITH_VIDEO_HOLE
                       : MEDIA_PLAYER_TYPE_MEDIA_SOURCE_WITH_VIDEO_HOLE;
#endif

  GURL parsed_url;
  if (!GetContentClient()->renderer()->GetWrtParsedUrl(url, parsed_url))
    parsed_url = url;

  manager_->Initialize(player_id_, player_type_,
                       media::GetCleanURL(parsed_url.spec().c_str()),
                       content_mime_type.Utf8(), demuxer_client_id);

#if defined(OS_TIZEN_TV_PRODUCT)
  if (content::IsHbbTV()) {
    SetCDMPlayerType();
    if (client_ && client_->HasEncryptedListener()) {
      had_encrypted_listener_ = true;
    }
  }
#endif

/*
 * sometimes in hbbtv case evas_object_show() is later than load
 * and the default value hidden = true, it cause suspend when load
 * WebEngine comment is that: can't ensure "show" is before than "load"
 */
#if defined(OS_TIZEN_TV_PRODUCT)
  if (delegate_->IsFrameHidden() && !content::IsHbbTV()) {
#else
  if (delegate_->IsFrameHidden()) {
#endif
    LOG(INFO) << "the player is hidden";
    Suspend();
  }
}

void WebMediaPlayerEfl::OnMediaSourceOpened(
    blink::WebMediaSource* web_media_source) {
  DCHECK(client_);
  client_->MediaSourceOpened(web_media_source);
}

void WebMediaPlayerEfl::OnEncryptedMediaInitData(
    media::EmeInitDataType init_data_type,
    const std::vector<uint8_t>& init_data) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  LOG_IF(WARNING, init_data_type_ != media::EmeInitDataType::UNKNOWN &&
                      init_data_type != init_data_type_)
      << "Mixed init data type not supported. The new type is ignored.";
  if (init_data_type_ == media::EmeInitDataType::UNKNOWN)
    init_data_type_ = init_data_type;

  encrypted_client_->Encrypted(ConvertToWebInitDataType(init_data_type),
                               init_data.data(), init_data.size());
}

#if defined(OS_TIZEN_TV_PRODUCT)
bool WebMediaPlayerEfl::IsSynchronousPlayerDestructionNeeded() const {
  // If cdm module is destroyed before player, player may try to
  // decrypt frames, what can crash renderer, So player has to be
  // destroyed before cdm module.
  return (player_type_ == MEDIA_PLAYER_TYPE_URL
#if defined(TIZEN_VIDEO_HOLE)
          || player_type_ == MEDIA_PLAYER_TYPE_URL_WITH_VIDEO_HOLE
#endif
          )
             ? cdm_context_ != nullptr
             : false;
}
#endif

void WebMediaPlayerEfl::SetCdmReadyCB(
    const MediaSourceDelegateEfl::CdmReadyCB& cdm_ready_cb) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  LOG(INFO) << "Cdm context " << cdm_context_;
  if (cdm_context_) {
    cdm_ready_cb.Run(cdm_context_, base::Bind(&media::IgnoreCdmAttached));
  } else {
    LOG(INFO) << "Setting pending_cdm_ready_cb_";
    pending_cdm_ready_cb_ = cdm_ready_cb;
  }
}

void WebMediaPlayerEfl::OnWaitingForDecryptionKey() {
  encrypted_client_->DidBlockPlaybackWaitingForKey();

  // TODO(jrummell): didResumePlaybackBlockedForKey() should only be called
  // when a key has been successfully added (e.g. OnSessionKeysChange() with
  // |has_additional_usable_key| = true). http://crbug.com/461903
  encrypted_client_->DidResumePlaybackBlockedForKey();
}

void WebMediaPlayerEfl::Play() {
  LOG(INFO) << "[" << player_id_ << "] " << __FUNCTION__;

#if defined(TIZEN_VIDEO_HOLE) && !defined(OS_TIZEN_TV_PRODUCT)
  if (is_video_hole_) {
    if (HasVideo() && !is_draw_ready_) {
      pending_play_ = true;
      return;
    }
    pending_play_ = false;
  }
#endif

  manager_->Play(player_id_);
  // Has to be updated from |MediaPlayerEfl| but IPC causes delay.
  // There are cases were play - pause are fired successively and would fail.
  OnPauseStateChange(false);
}

void WebMediaPlayerEfl::PauseInternal(bool is_media_related_action) {
  LOG(INFO) << "[" << player_id_ << "] " << __FUNCTION__
            << " media_related:" << is_media_related_action;
#if defined(TIZEN_VIDEO_HOLE) && !defined(OS_TIZEN_TV_PRODUCT)
  if (is_video_hole_)
    pending_play_ = false;
#endif
  manager_->Pause(player_id_, is_media_related_action);
  // Has to be updated from |MediaPlayerEfl| but IPC causes delay.
  // There are cases were play - pause are fired successively and would fail.
  OnPauseStateChange(true);
}

void WebMediaPlayerEfl::Pause() {
  LOG(INFO) << "[" << player_id_ << "] " << __FUNCTION__;
  PauseInternal(false);
}

void WebMediaPlayerEfl::ReleaseMediaResource() {
  LOG(INFO) << "Player[" << player_id_ << "]";
  manager_->Suspend(player_id_);
}

void WebMediaPlayerEfl::InitializeMediaResource() {
  LOG(INFO) << "Player[" << player_id_ << "] suspend_time : " << current_time_;
  manager_->Resume(player_id_);
}

void WebMediaPlayerEfl::RequestPause() {
  LOG(INFO) << "[" << player_id_ << "] " << __FUNCTION__;
  switch (network_state_) {
    // Pause the media player and inform Blink if the player is in a good
    // shape.
    case blink::WebMediaPlayer::kNetworkStateIdle:
    case blink::WebMediaPlayer::kNetworkStateLoading:
    case blink::WebMediaPlayer::kNetworkStateLoaded:
      PauseInternal(false);
      client_->PlaybackStateChanged();
      break;
    // If a WebMediaPlayer instance has entered into other then above states,
    // the internal network state in HTMLMediaElement could be set to empty.
    // And calling playbackStateChanged() could get this object deleted.
    default:
      break;
  }
}

bool WebMediaPlayerEfl::SupportsSave() const {
  // FIXME: Allow saving media element type.
  return false;
}

void WebMediaPlayerEfl::Seek(double seconds) {
  LOG(INFO) << "WebMediaPlayerEfl::seek() seconds : " << seconds;
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  base::TimeDelta new_seek_time = base::TimeDelta::FromSecondsD(seconds);
#if defined(OS_TIZEN_TV_PRODUCT)
  if (is_live_stream_) {
    LOG(INFO) << "min_seekable_time:" << min_seekable_time_.InSecondsF()
              << ",max_seekable_time:" << max_seekable_time_.InSecondsF();
    if (new_seek_time < min_seekable_time_)
      new_seek_time = min_seekable_time_;
    else if (new_seek_time > max_seekable_time_)
      new_seek_time = max_seekable_time_;
  }
#endif

  new_seek_time.SetResolution(base::TimeDelta::Resolution::MILLISECONDS);
  if (is_seeking_) {
    if (new_seek_time == seek_time_) {
      if (media_source_delegate_) {
        // Don't suppress any redundant in-progress MSE seek. There could have
        // been changes to the underlying buffers after seeking the demuxer and
        // before receiving OnSeekComplete() for currently in-progress seek.
        LOG(INFO) << "Detected MediaSource seek to same time as to : "
                  << seek_time_;
      } else {
        // Suppress all redundant seeks if unrestricted by media source
        // demuxer API.
        pending_seek_ = false;
        return;
      }
    }

    pending_seek_ = true;
    pending_seek_time_ = new_seek_time;
    if (media_source_delegate_)
      media_source_delegate_->CancelPendingSeek(pending_seek_time_);
    // Later, OnSeekComplete will trigger the pending seek.
    return;
  }

  is_seeking_ = true;
  seek_time_ = new_seek_time;

  // Once Chunk demuxer seeks |MediaPlayerEfl| seek will be intiated.
  if (media_source_delegate_)
    media_source_delegate_->StartWaitingForSeek(seek_time_);
  manager_->Seek(player_id_, seek_time_);
}

void WebMediaPlayerEfl::SetRate(double rate) {
  manager_->SetRate(player_id_, rate);
}

void WebMediaPlayerEfl::SetVolume(double volume) {
  manager_->SetVolume(player_id_, volume);
}

void WebMediaPlayerEfl::SetSinkId(
    const blink::WebString& sink_id,
    const blink::WebSecurityOrigin& security_origin,
    blink::WebSetSinkIdCallbacks* web_callback) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  std::unique_ptr<blink::WebSetSinkIdCallbacks> callback(web_callback);
  callback->OnError(blink::WebSetSinkIdError::kNotSupported);
}

void WebMediaPlayerEfl::SetPreload(blink::WebMediaPlayer::Preload preload) {
  NOTIMPLEMENTED();
}

blink::WebTimeRanges WebMediaPlayerEfl::Buffered() const {
  if (media_source_delegate_)
    return media_source_delegate_->Buffered();
  return buffered_;
}

blink::WebTimeRanges WebMediaPlayerEfl::Seekable() const {
  if (ready_state_ < WebMediaPlayer::kReadyStateHaveMetadata)
    return blink::WebTimeRanges();

#if defined(OS_TIZEN_TV_PRODUCT)
  if (is_live_stream_) {
    const blink::WebTimeRange seekable_range(min_seekable_time_.InSecondsF(),
                                             max_seekable_time_.InSecondsF());
    return blink::WebTimeRanges(&seekable_range, 1);
  }
#endif

  // TODO(dalecurtis): Technically this allows seeking on media which return an
  // infinite duration.  While not expected, disabling this breaks semi-live
  // players, http://crbug.com/427412.
  const blink::WebTimeRange seekable_range(0.0, Duration());
  return blink::WebTimeRanges(&seekable_range, 1);
}

void WebMediaPlayerEfl::Paint(blink::WebCanvas* canvas,
                              const blink::WebRect& rect,
                              cc::PaintFlags& flags,
                              int already_uploaded_id,
                              VideoFrameUploadMetadata* out_metadata) {
#if defined(TIZEN_TBM_SUPPORT) && defined(OS_TIZEN_TV_PRODUCT)
  if (compositor_ && compositor_->isStartedOESTexture())
    return;
#endif

  scoped_refptr<VideoFrame> video_frame = GetCurrentFrameFromCompositor();
  if (!video_frame.get())
    return;

#if defined(TIZEN_TBM_SUPPORT) && defined(OS_TIZEN_TV_PRODUCT)
  if (!video_frame->GetTbmTexture())
    return;
#endif

  media::Context3D context_3d;
#if defined(TIZEN_TBM_SUPPORT)
  if (video_frame->IsTBMBackend()) {
#else
  if (video_frame->HasTextures()) {
#endif
    if (context_provider_) {
      context_3d = media::Context3D(context_provider_->ContextGL(),
                                    context_provider_->GrContext());
    }
    if (!context_3d.gl)
      return;  // Unable to get/create a shared main thread context.
    if (!context_3d.gr_context)
      return;  // The context has been lost since and can't setup a GrContext.
  }

#if defined(TIZEN_TBM_SUPPORT)
  video_frame->CreateTbmTextureIfNeeded(context_3d.gl);

#if defined(_DEBUG_TBM_VIDEO_RENDERING_FPS) && _DEBUG_TBM_VIDEO_RENDERING_FPS
  static media::VideoFrame* prev_frame = 0;
  static int new_frame_cnt = 0;
  static int frame_cnt = 0;
  static base::TimeTicks last_tick = base::TimeTicks::Now();
  const base::TimeTicks now = base::TimeTicks::Now();
  const base::TimeDelta interval = now - last_tick;

  if (prev_frame != video_frame.get())
    new_frame_cnt++;
  prev_frame = video_frame.get();
  frame_cnt++;
  if (interval >= base::TimeDelta::FromSeconds(1)) {
    printf("VideoTBM[CpSk] > [FPS]%.1f/%.1f\n",
           new_frame_cnt / interval.InSecondsF(),
           frame_cnt / interval.InSecondsF());
    LOG(INFO) << "VideoTBM[CpSk] > [FPS]"
              << (new_frame_cnt / interval.InSecondsF()) << "/"
              << (frame_cnt / interval.InSecondsF());
    last_tick = now;
    frame_cnt = 0;
    new_frame_cnt = 0;
  }
#endif
#endif
  gfx::RectF gfx_rect(rect);
  skcanvas_video_renderer_.Paint(video_frame, canvas, gfx_rect, flags,
                                 media::VIDEO_ROTATION_0, context_3d);
}

bool WebMediaPlayerEfl::HasVideo() const {
  return video_;
}

bool WebMediaPlayerEfl::HasAudio() const {
  return audio_;
}

void WebMediaPlayerEfl::EnabledAudioTracksChanged(
    const blink::WebVector<blink::WebMediaPlayer::TrackId>& enabledTrackIds) {
  NOTIMPLEMENTED();
}

void WebMediaPlayerEfl::SelectedVideoTrackChanged(
    blink::WebMediaPlayer::TrackId* selectedTrackId) {
  NOTIMPLEMENTED();
}

blink::WebSize WebMediaPlayerEfl::NaturalSize() const {
  return blink::WebSize(natural_size_);
}

blink::WebSize WebMediaPlayerEfl::VisibleRect() const {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  scoped_refptr<VideoFrame> video_frame = GetCurrentFrameFromCompositor();
  if (!video_frame)
    return blink::WebSize();

  const gfx::Rect& visible_rect = video_frame->visible_rect();
  return blink::WebSize(visible_rect.width(), visible_rect.height());
}

bool WebMediaPlayerEfl::Paused() const {
  return is_paused_;
}

bool WebMediaPlayerEfl::Seeking() const {
  return is_seeking_;
}

double WebMediaPlayerEfl::Duration() const {
  return blink::TimeDeltaToWebTime(duration_);
}

double WebMediaPlayerEfl::CurrentTime() const {
  if (Seeking())
    return blink::TimeDeltaToWebTime(pending_seek_ ? pending_seek_time_
                                                   : seek_time_);
  return blink::TimeDeltaToWebTime(current_time_);
}

double WebMediaPlayerEfl::GetStartDate() const {
#if defined(OS_TIZEN_TV_PRODUCT)
  // Currently, this API is only for hbbtv.
  if (IsHbbTV())
    return manager_->GetStartDate(player_id_);
#endif

  return std::numeric_limits<double>::quiet_NaN();
}

#if defined(TIZEN_MULTIMEDIA_SUPPORT)
void WebMediaPlayerEfl::Suspend() {
  LOG(INFO) << "Player[" << player_id_ << "]" << __FUNCTION__;

#if defined(TIZEN_TBM_SUPPORT) && defined(OS_TIZEN_TV_PRODUCT)
  video_handles_map_.clear();
  current_frame_ = nullptr;
  LOG(INFO) << "Suspend! remove map: " << video_handles_map_.size();
  scaler_lock_mode_ = gfx::MEDIA_PACKET_NORMAL_MODE;
  ControlMediaPacket(scaler_lock_mode_);
#endif

  if (player_type_ == MEDIA_PLAYER_TYPE_NONE) {
    // TODO(m.debski): This should not happen as HTMLMediaElement is handling a
    // load deferral.
    LOG(ERROR) << "Player type is not set, load has not occured and there is "
                  "no player yet the player should suspend.";
    return;
  }
  // We need to suspend the element if it has not yet reported media type
  if ((!HasVideo() && !HasAudio()) || HasVideo() || HasAudio() ||
      (TizenExtensible::GetInstance()->GetExtensibleAPI("background,music") ==
       false)) {
    if (!is_paused_) {
      OnPauseStateChange(true);
    }
    client_->MediaPlayerHidden();
    ReleaseMediaResource();
  }
}

void WebMediaPlayerEfl::Resume() {
  LOG(INFO) << "Player[" << player_id_ << "]";
  client_->MediaPlayerShown();
  InitializeMediaResource();
}

void WebMediaPlayerEfl::Deactivate() {
  LOG(INFO) << "Player[" << player_id_ << "] " << __FUNCTION__;
  manager_->Deactivate(player_id_);
}

void WebMediaPlayerEfl::Activate() {
  LOG(INFO) << "Player[" << player_id_ << "] " << __FUNCTION__;
  manager_->Activate(player_id_);
}
#endif

blink::WebMediaPlayer::NetworkState WebMediaPlayerEfl::GetNetworkState() const {
  return network_state_;
}

blink::WebMediaPlayer::ReadyState WebMediaPlayerEfl::GetReadyState() const {
  return ready_state_;
}

blink::WebString WebMediaPlayerEfl::GetErrorMessage() const {
  return blink::WebString::FromUTF8(media_log_->GetErrorMessage());
}

bool WebMediaPlayerEfl::DidLoadingProgress() {
  if (did_loading_progress_) {
    did_loading_progress_ = false;
    return true;
  }
  return false;
}

bool WebMediaPlayerEfl::HasSingleSecurityOrigin() const {
  return true;
}

bool WebMediaPlayerEfl::DidPassCORSAccessCheck() const {
  return false;
}

double WebMediaPlayerEfl::MediaTimeForTimeValue(double timeValue) const {
  return base::TimeDelta::FromSecondsD(timeValue).InSecondsF();
}

unsigned WebMediaPlayerEfl::DecodedFrameCount() const {
#if defined(OS_TIZEN_TV_PRODUCT)
  return manager_->GetDecodedFrameCount(player_id_);
#else
  return 0;
#endif
}

unsigned WebMediaPlayerEfl::DroppedFrameCount() const {
#if defined(OS_TIZEN_TV_PRODUCT)
  return manager_->GetDroppedFrameCount(player_id_);
#else
  return 0;
#endif
}

size_t WebMediaPlayerEfl::AudioDecodedByteCount() const {
  return 0;
}

size_t WebMediaPlayerEfl::VideoDecodedByteCount() const {
  return 0;
};

bool WebMediaPlayerEfl::CopyVideoTextureToPlatformTexture(
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
    VideoFrameUploadMetadata* out_metadata) {
#if defined(USE_TTRACE)
  TTRACE_WEB("WebMediaPlayerEfl::copyVideoTextureToPlatformTexture");
#endif

#if defined(OS_TIZEN_TV_PRODUCT)
  if (compositor_)
    compositor_->SetStartedOESTexture(false);
#endif

  scoped_refptr<VideoFrame> video_frame = GetCurrentFrameFromCompositor();
  if (!video_frame.get())
    return false;

  media::Context3D context_3d;
#if defined(TIZEN_TBM_SUPPORT)
  if (video_frame->IsTBMBackend()) {
#else
  if (video_frame->HasTextures()) {
#endif
    if (context_provider_) {
      context_3d = media::Context3D(context_provider_->ContextGL(),
                                    context_provider_->GrContext());
    }
    if (!context_3d.gl)
      return false;  // Unable to get/create a shared main thread context.
    if (!context_3d.gr_context)
      return false;  // The context has been lost since and can't setup a
                     // GrContext.
  }

#if defined(TIZEN_TBM_SUPPORT)
  video_frame->CreateTbmTextureIfNeeded(context_3d.gl);

#if defined(_DEBUG_TBM_VIDEO_RENDERING_FPS) && _DEBUG_TBM_VIDEO_RENDERING_FPS
  static media::VideoFrame* prev_frame = 0;
  static int new_frame_cnt = 0;
  static int frame_cnt = 0;
  static base::TimeTicks last_tick = base::TimeTicks::Now();
  const base::TimeTicks now = base::TimeTicks::Now();
  const base::TimeDelta interval = now - last_tick;

  if (prev_frame != video_frame.get())
    new_frame_cnt++;
  prev_frame = video_frame.get();
  frame_cnt++;
  if (interval >= base::TimeDelta::FromSeconds(1)) {
    printf("VideoTBM[CpGL] > [FPS]%.1f/%.1f\n",
           new_frame_cnt / interval.InSecondsF(),
           frame_cnt / interval.InSecondsF());
    LOG(INFO) << "VideoTBM[CpGL] > [FPS]"
              << (new_frame_cnt / interval.InSecondsF()) << "/"
              << (frame_cnt / interval.InSecondsF());
    last_tick = now;
    frame_cnt = 0;
    new_frame_cnt = 0;
  }
#endif

  media::SkCanvasVideoRenderer::CopyVideoFrameSingleTextureToGLTexture(
      gl, video_frame.get(), target, texture, internal_format, format, type,
      level, premultiply_alpha, flip_y);
  return true;
#else
  return false;
#endif
}

#if defined(TIZEN_TBM_SUPPORT) && defined(OS_TIZEN_TV_PRODUCT)
base::Timer& WebMediaPlayerEfl::DeferredChangeVideoResolutionTimer(void) {
  static base::Timer deferred_change_resolution_timer_(true, false);
  return deferred_change_resolution_timer_;
}

void WebMediaPlayerEfl::DeferredChangeVideoResolution(
    WebMediaPlayerEfl* web_player) {
  if (web_player)
    web_player->ControlMediaPacket(gfx::MEDIA_PACKET_NORMAL_MODE);
}

bool WebMediaPlayerEfl::RenderRingBuffer(gpu::gles2::GLES2Interface* gl,
                                         unsigned& image_id) {
  uint32_t scaler_value;
  VideoFrame::GetScalerIndex(is_gpu_vendor_kantm2_used_, &scaler_value);
  // next frame
  int scaler_idx = (scaler_value + scaler_buffer_max_ + 1) % scaler_buffer_max_;

  if (scaler_lock_mode_ == 0) {
    scaler_lock_mode_ = gfx::MEDIA_PACKET_RING_BUFFER_MODE;
    ControlMediaPacket(scaler_lock_mode_);
    VideoFrame::LockScalerDuringNextFrame(is_gpu_vendor_kantm2_used_,
                                          scaler_idx, true);
  }

#ifdef SCALER_DEBUG
  LOG(INFO) << "current render idx: " << scaler_idx
            << " scaler: " << scaler_value
            << " fc-ts: " << frame_rate_control_timestamp_.InMilliseconds();
#endif

  for (auto iter = video_handles_map_.begin(); iter != video_handles_map_.end();
       ++iter) {
    if (!iter->second.get() ||
        scaler_idx != iter->second->GetScalerPhysicalId())
      continue;

#ifdef SCALER_DEBUG
    previous_media_packet_timestamp_ = iter->second->timestamp();
#endif
    image_id = iter->second->GetImageID();
    const uint32 target = GL_TEXTURE_EXTERNAL_OES;
    VideoFrame::LockScalerDuringNextFrame(is_gpu_vendor_kantm2_used_,
                                          scaler_idx, true);
    gl->BindTexImage2DCHROMIUM(target, image_id);
    gl->Flush();

    const GLuint64 fence_sync = gl->InsertFenceSyncCHROMIUM();
    gl->ShallowFlushCHROMIUM();
    gpu::SyncToken sync_token;
    gl->GenSyncTokenCHROMIUM(fence_sync, sync_token.GetData());
    if (sync_token.HasData())
      gl->WaitSyncTokenCHROMIUM(sync_token.GetConstData());

#ifdef SCALER_DEBUG
    struct timeval tv0;
    unsigned long cur_time;
    gettimeofday(&tv0, 0);
    cur_time = (tv0.tv_sec * 1000000L) + tv0.tv_usec;
    LOG(INFO) << " img : " << image_id << " curtime: " << cur_time;
    previous_frame_time_ = base::Time::NowFromSystemTime();
    previous_index_ = scaler_idx;
#endif
    return true;
  }

  return false;
}

bool WebMediaPlayerEfl::ZeroCopyVideoTbmToPlatformTexture(
    gpu::gles2::GLES2Interface* gl,
    unsigned texture,
    unsigned& image_id) {
#if defined(USE_TTRACE)
  traceMark(TTRACE_TAG_WEB, "GetCurrentFrameNoSignal");
#endif

  if (video_handles_map_.size() == scaler_buffer_max_ &&
      RenderRingBuffer(gl, image_id)) {
    return true;
  }

#if defined(USE_NO_WAIT_SIGNAL_MEDIA_PACKET_THREAD)
  scoped_refptr<VideoFrame> video_frame = GetCurrentFrame();
#else
  scoped_refptr<VideoFrame> video_frame = GetCurrentFrameFromCompositor();
#endif

  if (!video_frame.get() || !compositor_) {
    LOG(INFO) << "no video frame !";
    return false;
  }

  compositor_->SetStartedOESTexture(true);
  compositor_->CreateTbmTextureIfNeeded(gl, texture);

#ifdef SCALER_DEBUG
  LOG(INFO) << "EGL Image - time : " << video_frame->timestamp()
            << " frame: " << (void*)video_frame.get()
            << " img: " << video_frame->GetImageID()
            << " scaler: " << video_frame->GetScalerPhysicalId();

  if (previous_media_packet_timestamp_ != base::TimeDelta() &&
      previous_index_ != video_frame->GetScalerPhysicalId()) {
    frame_rate_control_timestamp_ =
        std::min(video_frame->timestamp() - previous_media_packet_timestamp_,
                 frame_rate_control_timestamp_);
  }
#else
  LOG(INFO) << "EGL Image - time : " << video_frame->timestamp()
            << " img: " << video_frame->GetImageID();
#endif

  image_id = video_frame->GetImageID();
  if (!image_id || !video_frame->GetTbmTexture()) {
    LOG(ERROR) << "Fail: GetImageID = " << image_id;
#ifdef SCALER_DEBUG
    previous_media_packet_timestamp_ = video_frame->timestamp();
#endif
    return false;
  }

  scoped_refptr<VideoFrame> cached_frame = video_handles_map_[image_id];
  if (cached_frame != video_frame)
    video_handles_map_[image_id] = video_frame;

#ifdef SCALER_DEBUG
  previous_media_packet_timestamp_ = video_frame->timestamp();
  previous_index_ = video_frame->GetScalerPhysicalId();
#endif
  VideoFrame::LockScalerDuringNextFrame(
      is_gpu_vendor_kantm2_used_, video_frame->GetScalerPhysicalId(), true);

  return true;
}

void WebMediaPlayerEfl::DeleteVideoFrame(unsigned image_id) {
  scoped_refptr<VideoFrame> current_frame = video_handles_map_[image_id];
  if (!current_frame.get())
    return;

  for (auto iter = video_handles_map_.begin(); iter != video_handles_map_.end();
       ++iter) {
    if (iter->first != image_id && current_frame->GetScalerPhysicalId() ==
                                       iter->second->GetScalerPhysicalId()) {
#ifdef SCALER_DEBUG
      LOG(INFO) << "same frame! :" << iter->second->GetScalerPhysicalId();
#endif
      video_handles_map_.erase(image_id);
      return;
    }
  }
  if (video_handles_map_.size() > scaler_buffer_max_)
    video_handles_map_.erase(image_id);
#ifdef SCALER_DEBUG
  else {
    LOG(INFO) << "Keep scaler id : " << current_frame->GetScalerPhysicalId();
  }
#endif
}
#endif

void WebMediaPlayerEfl::ComputeFrameUploadMetadata(
    VideoFrame* frame,
    int already_uploaded_id,
    VideoFrameUploadMetadata* out_metadata) {
  NOTIMPLEMENTED();
}

blink::WebAudioSourceProvider* WebMediaPlayerEfl::GetAudioSourceProvider() {
  NOTIMPLEMENTED();
  return nullptr;
}

void WebMediaPlayerEfl::SetContentDecryptionModule(
    blink::WebContentDecryptionModule* cdm,
    blink::WebContentDecryptionModuleResult result) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  // Once the CDM is set it can't be cleared as there may be frames being
  // decrypted on other threads. So fail this request.
  // http://crbug.com/462365#c7.
  auto new_cdm_context_ = GetCdmContext(cdm);

  if (!new_cdm_context_) {
    result.CompleteWithError(
        blink::kWebContentDecryptionModuleExceptionNotSupportedError, 0,
        "The existing MediaKeys object cannot be removed.");
    return;
  }

#if defined(OS_TIZEN_TV_PRODUCT)
  // If URL source was set and if there is no encyrpted listener it means
  // that player was prepared without DRM settings required for standalone EME.
  // It's too late to set media keys now as player and IEME instance (about to
  // be created) won't be compatible. Report and error as this scenario is not
  // currently supported by standalone EME feature.

  bool hasUrlSource = (player_type_ == MEDIA_PLAYER_TYPE_URL) ||
                      (player_type_ == MEDIA_PLAYER_TYPE_URL_WITH_VIDEO_HOLE);

  if (IsHbbTV() && hasUrlSource && !had_encrypted_listener_) {
    result.CompleteWithError(
        blink::kWebContentDecryptionModuleExceptionInvalidStateError, 0,
        "MediaKeys cannot be set after source is assigned.");
    return;
  }
#endif

  pending_cdm_ =
      std::move(media::ToWebContentDecryptionModuleImpl(cdm)->GetCdm());
  setCdm(new_cdm_context_);

#if defined(OS_TIZEN_TV_PRODUCT)
  SetDecryptorCb();
#endif
  result.Complete();

  if (!pending_cdm_ready_cb_.is_null()) {
    LOG(INFO) << "Sending pending_cdm_ready_cb_";
    pending_cdm_ready_cb_.Run(cdm_context_,
                              base::Bind(&media::IgnoreCdmAttached));
  }
}

void WebMediaPlayerEfl::setCdm(media::CdmContext* cdm_context) {
  cdm_context_ = cdm_context;

#if defined(OS_TIZEN_TV_PRODUCT)
  if (content::IsHbbTV()) {
    SetCDMPlayerType();
  }
#endif
}

#if defined(OS_TIZEN_TV_PRODUCT)
void WebMediaPlayerEfl::SetDecryptorCb() {
  cdm_context_->GetDecryptor()->RegisterNewKeyCB(
      media::Decryptor::kVideo, base::Bind(&WebMediaPlayerEfl::OnNewKeyAdded,
                                           weak_factory_.GetWeakPtr()));
  cdm_context_->GetDecryptor()->RegisterNewKeyCB(
      media::Decryptor::kAudio, base::Bind(&WebMediaPlayerEfl::OnNewKeyAdded,
                                           weak_factory_.GetWeakPtr()));
}

void WebMediaPlayerEfl::SetCDMPlayerType() const {
  if (cdm_context_) {
    if ((cdm_context_->GetPlayerType() != MEDIA_PLAYER_TYPE_NONE) &&
        (cdm_context_->GetPlayerType() != player_type_)) {
      LOG(ERROR) << "player type mismatch, CDM created for: "
                 << cdm_context_->GetPlayerType()
                 << ", player is: " << player_type_;
      manager_->SetDecryptorHandle(player_id_, 0);
    } else {
      cdm_context_->SetPlayerType(player_type_);
    }
  }
}

void WebMediaPlayerEfl::SetDecryptorHandle() const {
  if (cdm_context_ && cdm_context_->GetDecryptor()) {
    auto decryptor_handle = cdm_context_->GetDecryptor()->GetDecryptorHandle();
    LOG(INFO) << "Get decryptor handle ["
              << reinterpret_cast<void*>(decryptor_handle)
              << "] from cdm context";
    manager_->SetDecryptorHandle(player_id_, decryptor_handle);
  }
}

void WebMediaPlayerEfl::OnNewKeyAdded() const {
  LOG(INFO) << "New key notified";
  SetDecryptorHandle();
}
#endif

bool WebMediaPlayerEfl::SupportsOverlayFullscreenVideo() {
  return false;
}

void WebMediaPlayerEfl::EnteredFullscreen() {
  if (is_fullscreen_)
    return;

  is_fullscreen_ = true;
#if defined(TIZEN_VD_MULTIPLE_MIXERDECODER)
  if (!content::IsWebBrowser())
    manager_->EnteredFullscreen(player_id_);
#endif

#if defined(TIZEN_VIDEO_HOLE) && !defined(OS_TIZEN_TV_PRODUCT)
  manager_->EnteredFullscreen(player_id_);
  if (HasVideo() && is_video_hole_) {
    CreateVideoHoleFrame();
  }
#endif
}

void WebMediaPlayerEfl::ExitedFullscreen() {
  if (!is_fullscreen_)
    return;

  is_fullscreen_ = false;
#if defined(TIZEN_VIDEO_HOLE) && !defined(OS_TIZEN_TV_PRODUCT)
  if (HasVideo() && is_video_hole_) {
    gfx::Size size(video_width_, video_height_);
    scoped_refptr<VideoFrame> video_frame = VideoFrame::CreateBlackFrame(size);
    FrameReady(video_frame);
  }

  manager_->ExitedFullscreen(player_id_);
  client_->Repaint();
#endif

#if defined(TIZEN_VD_MULTIPLE_MIXERDECODER)
  if (!content::IsWebBrowser())
    manager_->ExitedFullscreen(player_id_);
#endif
}

void WebMediaPlayerEfl::BecameDominantVisibleContent(bool isDominant) {
  NOTIMPLEMENTED();
}

void WebMediaPlayerEfl::SetIsEffectivelyFullscreen(
    bool isEffectivelyFullscreen) {
  NOTIMPLEMENTED();
}

void WebMediaPlayerEfl::OnHasNativeControlsChanged(bool has_native_controls) {
  NOTIMPLEMENTED();
}

void WebMediaPlayerEfl::OnDisplayTypeChanged(
    blink::WebMediaPlayer::DisplayType display_type) {
  NOTIMPLEMENTED();
}

void WebMediaPlayerEfl::OnFrameHidden() {
  LOG(INFO) << "[" << player_id_ << "] " << __FUNCTION__;
#if defined(OS_TIZEN_TV_PRODUCT)
  client_->MediaPlayerHidden();
#else
  Suspend();
#endif
}

void WebMediaPlayerEfl::OnFrameClosed() {
  LOG(INFO) << "[" << player_id_ << "] " << __FUNCTION__;
  Suspend();
}

void WebMediaPlayerEfl::OnFrameShown() {
  LOG(INFO) << "[" << player_id_ << "] " << __FUNCTION__;
#if defined(OS_TIZEN_TV_PRODUCT)
  client_->MediaPlayerShown();
#else
  Resume();
#endif
}

void WebMediaPlayerEfl::OnIdleTimeout() {
  NOTIMPLEMENTED();
}

void WebMediaPlayerEfl::OnPlay() {
  LOG(INFO) << "[" << player_id_ << "] " << __FUNCTION__;
  Play();
}

void WebMediaPlayerEfl::OnPause() {
  LOG(INFO) << "[" << player_id_ << "] " << __FUNCTION__;
  Pause();
}

void WebMediaPlayerEfl::OnVolumeMultiplierUpdate(double multiplier) {
  volume_multiplier_ = multiplier;
  SetVolume(volume_);
}

void WebMediaPlayerEfl::OnBecamePersistentVideo(bool value) {
  NOTIMPLEMENTED();
}

void WebMediaPlayerEfl::RequestRemotePlaybackDisabled(bool disabled) {
  NOTIMPLEMENTED();
}

void WebMediaPlayerEfl::SetReadyState(blink::WebMediaPlayer::ReadyState state) {
  ready_state_ = state;
#if defined(OS_TIZEN_TV_PRODUCT)
  if ((player_type_ == MEDIA_PLAYER_TYPE_MEDIA_SOURCE
#if defined(TIZEN_VIDEO_HOLE)
       || player_type_ == MEDIA_PLAYER_TYPE_MEDIA_SOURCE_WITH_VIDEO_HOLE
#endif
       ) && is_seeking_ && state > ReadyState::kReadyStateHaveCurrentData)
    return;
#endif
  client_->ReadyStateChanged();
}

void WebMediaPlayerEfl::SetNetworkState(
    blink::WebMediaPlayer::NetworkState state) {
  network_state_ = state;
  client_->NetworkStateChanged();
}

void WebMediaPlayerEfl::OnNewFrameAvailable(base::SharedMemoryHandle handle,
                                            uint32 yuv_size,
                                            base::TimeDelta timestamp) {
  base::SharedMemory shared_memory(handle, false);
  if (!shared_memory.Map(yuv_size)) {
    LOG(ERROR) << "Failed to map shared memory for size " << yuv_size;
    return;
  }

  uint8* const yuv_buffer = static_cast<uint8*>(shared_memory.memory());
  gfx::Size size(video_width_, video_height_);
  scoped_refptr<VideoFrame> video_frame = VideoFrame::CreateFrame(
      media::PIXEL_FORMAT_YV12, size, gfx::Rect(size), size, timestamp);

  uint8* video_buf = yuv_buffer;
  const uint c_frm_size = yuv_size / 6;
  const uint y_frm_size = c_frm_size << 2;  // * 4;

  // U Plane buffer.
  uint8* video_buf_u = video_buf + y_frm_size;

  // V Plane buffer.
  uint8* video_buf_v = video_buf_u + c_frm_size;

  libyuv::I420Copy(
      video_buf, video_frame.get()->stride(VideoFrame::kYPlane), video_buf_u,
      video_frame.get()->stride(VideoFrame::kYPlane) / 2, video_buf_v,
      video_frame.get()->stride(VideoFrame::kYPlane) / 2,
      video_frame.get()->data(VideoFrame::kYPlane),
      video_frame.get()->stride(VideoFrame::kYPlane),
      video_frame.get()->data(VideoFrame::kUPlane),
      video_frame.get()->stride(VideoFrame::kUPlane),
      video_frame.get()->data(VideoFrame::kVPlane),
      video_frame.get()->stride(VideoFrame::kVPlane), video_width_,
      video_height_);
  FrameReady(video_frame);
}

#if defined(TIZEN_TBM_SUPPORT)
void WebMediaPlayerEfl::OnNewTbmBufferAvailable(
    const gfx::TbmBufferHandle& tbm_handle,
#if defined(OS_TIZEN_TV_PRODUCT)
    int scaler_physical_id,
#endif
    base::TimeDelta timestamp,
    const base::Closure& cb) {
#if defined(USE_TTRACE)
  TTRACE_WEB("WebMediaPlayerEfl::OnNewTbmBufferAvailable");
#endif
  gfx::Size size(video_width_, video_height_);
  scoped_refptr<VideoFrame> video_frame =
      VideoFrame::WrapTBMSurface(size, timestamp, tbm_handle);
  video_frame->AddDestructionObserver(base::Bind(
      &content::OnTbmBufferExhausted, base::Unretained(video_frame.get()), cb));
#if defined(OS_TIZEN_TV_PRODUCT)
  video_frame->SetScalerPhysicalId(scaler_physical_id);
#endif
  FrameReady(video_frame);
#if defined(USE_NO_WAIT_SIGNAL_MEDIA_PACKET_THREAD)
  SetCurrentFrameInternal(video_frame);
#endif
}

#if defined(OS_TIZEN_TV_PRODUCT)
void WebMediaPlayerEfl::ControlMediaPacket(int mode) {
  if (manager_)
    manager_->ControlMediaPacket(player_id_, mode);
}
#endif
#endif

void WebMediaPlayerEfl::FrameReady(const scoped_refptr<VideoFrame>& frame) {
  compositor_->PaintSingleFrame(frame);
}

#if defined(TIZEN_VIDEO_HOLE)
void WebMediaPlayerEfl::CreateVideoHoleFrame() {
  gfx::Size size(video_width_, video_height_);

  scoped_refptr<VideoFrame> video_frame = VideoFrame::CreateHoleFrame(size);
  if (video_frame)
    FrameReady(video_frame);
}

void WebMediaPlayerEfl::OnDrawableContentRectChanged(gfx::Rect rect,
                                                     bool is_video) {
  LOG(INFO) << "SetMediaGeometry: " << rect.ToString();
  is_draw_ready_ = true;

  StopLayerBoundUpdateTimer();
  gfx::RectF rect_f = static_cast<gfx::RectF>(rect);
  if (manager_)
    manager_->SetMediaGeometry(player_id_, rect_f);

#if !defined(OS_TIZEN_TV_PRODUCT)
  if (pending_play_)
    Play();
#endif
}

bool WebMediaPlayerEfl::UpdateBoundaryRectangle() {
  if (!video_weblayer_)
    return false;

  // Compute the geometry of video frame layer.
  cc::Layer* layer = video_weblayer_->layer();
  gfx::RectF rect(gfx::SizeF(layer->bounds()));
  while (layer) {
    rect.Offset(layer->position().OffsetFromOrigin());
    rect.Offset(layer->scroll_offset().x() * (-1),
                layer->scroll_offset().y() * (-1));
    layer = layer->parent();
  }

  rect.Scale(frame_->View()->PageScaleFactor());

  // Return false when the geometry hasn't been changed from the last time.
  if (last_computed_rect_ == rect)
    return false;

  // Store the changed geometry information when it is actually changed.
  last_computed_rect_ = rect;
  return true;
}

gfx::RectF WebMediaPlayerEfl::GetBoundaryRectangle() const {
  LOG(INFO) << "rect : " << last_computed_rect_.ToString();
  return last_computed_rect_;
}

void WebMediaPlayerEfl::StartLayerBoundUpdateTimer() {
  if (layer_bound_update_timer_.IsRunning())
    return;

  layer_bound_update_timer_.Start(
      FROM_HERE, kLayerBoundUpdateInterval, this,
      &WebMediaPlayerEfl::OnLayerBoundUpdateTimerFired);
}

void WebMediaPlayerEfl::StopLayerBoundUpdateTimer() {
  if (layer_bound_update_timer_.IsRunning())
    layer_bound_update_timer_.Stop();
}

void WebMediaPlayerEfl::OnLayerBoundUpdateTimerFired() {
  if (UpdateBoundaryRectangle()) {
    if (manager_) {
      manager_->SetMediaGeometry(player_id_, GetBoundaryRectangle());
      StopLayerBoundUpdateTimer();
    }
  }
}

bool WebMediaPlayerEfl::ShouldCreateVideoHoleFrame() const {
  if (HasVideo() && is_video_hole_)
    return true;
  return false;
}
#endif

void WebMediaPlayerEfl::OnMediaDataChange(int width, int height, int media) {
#if defined(TIZEN_TBM_SUPPORT) && defined(OS_TIZEN_TV_PRODUCT)
  if ((video_width_ != width || video_height_ != height) && scaler_lock_mode_) {
    video_handles_map_.clear();
    current_frame_ = nullptr;
    LOG(INFO) << "Change video size! remove map: " << video_handles_map_.size();
    scaler_lock_mode_ = gfx::MEDIA_PACKET_NORMAL_MODE;

    // Disable scaler lock
    VideoFrame::LockScalerDuringNextFrame(is_gpu_vendor_kantm2_used_, 0, false);
    DeferredChangeVideoResolutionTimer().Start(
        FROM_HERE, kVideoResolutionChangeInterval,
        base::Bind(&WebMediaPlayerEfl::DeferredChangeVideoResolution,
                   base::Unretained(this)));
  }
#endif

  video_height_ = height;
  video_width_ = width;
  audio_ = media::MediaTypeFlags(media).test(media::MediaType::Audio);
  video_ = media::MediaTypeFlags(media).test(media::MediaType::Video);
  natural_size_ = gfx::Size(width, height);
  if (HasVideo() && !video_weblayer_) {
    video_weblayer_.reset(new cc_blink::WebLayerImpl(
        cc::VideoLayer::Create(compositor_.get(), media::VIDEO_ROTATION_0)));
    video_weblayer_->layer()->SetContentsOpaque(opaque_);
    video_weblayer_->SetContentsOpaqueIsFixed(true);
    client_->SetWebLayer(video_weblayer_.get());
  }
#if defined(TIZEN_VIDEO_HOLE)
  if (ShouldCreateVideoHoleFrame()) {
    CreateVideoHoleFrame();
    StartLayerBoundUpdateTimer();
  }
#endif
}

void WebMediaPlayerEfl::OnTimeChanged() {
  client_->TimeChanged();
}

void WebMediaPlayerEfl::OnDurationChange(base::TimeDelta Duration) {
  duration_ = Duration;

#if defined(OS_TIZEN_TV_PRODUCT)
  if (ready_state_ == blink::WebMediaPlayer::kReadyStateHaveNothing)
    return;
#endif

  client_->DurationChanged();
}

void WebMediaPlayerEfl::OnNaturalSizeChanged(gfx::Size size) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  DCHECK_NE(ready_state_, blink::WebMediaPlayer::kReadyStateHaveNothing);
  media_log_->AddEvent(
      media_log_->CreateVideoSizeSetEvent(size.width(), size.height()));
  natural_size_ = size;

  client_->SizeChanged();
}

void WebMediaPlayerEfl::OnOpacityChanged(bool opaque) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  DCHECK_NE(ready_state_, blink::WebMediaPlayer::kReadyStateHaveNothing);

  opaque_ = opaque;
  if (video_weblayer_)
    video_weblayer_->layer()->SetContentsOpaque(opaque_);
}

static void GetCurrentFrameAndSignal(media::VideoFrameCompositor* compositor,
                                     scoped_refptr<VideoFrame>* video_frame_out,
                                     base::WaitableEvent* event) {
  *video_frame_out = compositor->GetCurrentFrame();
  event->Signal();
#if defined(USE_TTRACE)
  TTRACE(TTRACE_TAG_WEB, "GetCurrentFrameAndSignal");
#endif
}

#if defined(USE_NO_WAIT_SIGNAL_MEDIA_PACKET_THREAD)
void WebMediaPlayerEfl::SetCurrentFrameInternal(
    scoped_refptr<media::VideoFrame>& video_frame) {
  current_frame_ = video_frame;
}

scoped_refptr<media::VideoFrame> WebMediaPlayerEfl::GetCurrentFrame() {
  return current_frame_;
}
#endif

scoped_refptr<VideoFrame> WebMediaPlayerEfl::GetCurrentFrameFromCompositor()
    const {
  if (compositor_task_runner_->BelongsToCurrentThread()) {
    scoped_refptr<VideoFrame> video_frame =
        compositor_->GetCurrentFrameAndUpdateIfStale();
    if (!video_frame) {
      return nullptr;
    }

    return video_frame;
  }

  // Use a posted task and waitable event instead of a lock otherwise
  // WebGL/Canvas can see different content than what the compositor is seeing.
  scoped_refptr<VideoFrame> video_frame;
  base::WaitableEvent event(base::WaitableEvent::ResetPolicy::AUTOMATIC,
                            base::WaitableEvent::InitialState::NOT_SIGNALED);
  compositor_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&GetCurrentFrameAndSignal, base::Unretained(compositor_.get()),
                 &video_frame, &event));
  event.Wait();
  if (!video_frame) {
    return nullptr;
  }

  return video_frame;
}

void WebMediaPlayerEfl::OnTimeUpdate(base::TimeDelta current_time) {
  current_time_ = current_time;
#if defined(OS_TIZEN_TV_PRODUCT)
  if (force_update_playback_position_) {
    force_update_playback_position_ = false;
    client_->ForceSettingOfficialPlaybackPosition();
  }
#endif
}

void WebMediaPlayerEfl::OnBufferUpdate(int percentage) {
  buffered_[0].end = Duration() * percentage / 100;
  did_loading_progress_ = true;
}

void WebMediaPlayerEfl::OnPauseStateChange(bool state) {
  if (is_paused_ == state)
    return;

  is_paused_ = state;
#if defined(OS_TIZEN_TV_PRODUCT)
  if (GetNetworkState() >= blink::WebMediaPlayer::kNetworkStateFormatError)
    return;
#endif
  client_->PlaybackStateChanged();
  if (!delegate_)
    return;

  if (is_paused_) {
    delegate_->DidPause(delegate_id_);
  } else {
    delegate_->DidPlay(delegate_id_, HasVideo(), HasAudio(),
                       media::DurationToMediaContentType(duration_));
  }
}

void WebMediaPlayerEfl::OnPlayerSuspend(bool is_preempted) {
  if (!is_paused_ && is_preempted) {
#if defined(OS_TIZEN_TV_PRODUCT)
    manager_->Pause(player_id_, true);
#endif
    OnPauseStateChange(true);
  }

  if (!delegate_)
    return;
  delegate_->PlayerGone(delegate_id_);
}

void WebMediaPlayerEfl::OnPlayerResumed(bool is_preempted) {
#if defined(OS_TIZEN_TV_PRODUCT)
  force_update_playback_position_ = true;
#endif

  if (!delegate_)
    return;

  if (is_paused_)
    delegate_->DidPause(delegate_id_);
  else
    delegate_->DidPlay(delegate_id_, HasVideo(), HasAudio(),
                       media::DurationToMediaContentType(duration_));
}

void WebMediaPlayerEfl::OnSeekComplete() {
  LOG(INFO) << "Seek completed to " << seek_time_.InSecondsF();

  is_seeking_ = false;
  seek_time_ = base::TimeDelta();

  // Handling pending seek for ME. For MSE |CancelPendingSeek|
  // will handle the pending seeks.
  if (!media_source_delegate_ && pending_seek_) {
    pending_seek_ = false;
    Seek(pending_seek_time_.InSecondsF());
    pending_seek_time_ = base::TimeDelta();
    return;
  }
#if defined(TIZEN_VIDEO_HOLE)
  if (ShouldCreateVideoHoleFrame())
    CreateVideoHoleFrame();
#endif
#if defined(OS_TIZEN_TV_PRODUCT)
  client_->ReadyStateChanged();
#endif
  client_->TimeChanged();
}

void WebMediaPlayerEfl::OnRequestSeek(base::TimeDelta seek_time) {
  client_->RequestSeek(seek_time.InSecondsF());
}

#if defined(OS_TIZEN_TV_PRODUCT)
void WebMediaPlayerEfl::GetPlayTrackInfo(int id) {
  blink::WebString url;
  blink::WebString lang;
  // get track info
  if (!client_ || !client_->GetTextTrackInfo(id, url, lang))
    return;

  LOG(INFO) << "id= " << id << ", url= " << url.Utf8()
            << ", lang= " << lang.Utf8();
  // notify PlayTrack...
  manager_->NotifySubtitlePlay(player_id_, id, url.Utf8(), lang.Utf8());
}

void WebMediaPlayerEfl::OnDrmError() {
  if (content::IsHbbTV() && client_) {
    LOG(ERROR) << "Drm error reported from HBBTV";
    client_->OnDrmError();
  }
}

void WebMediaPlayerEfl::OnSeekableTimeChange(base::TimeDelta min_time,
                                             base::TimeDelta max_time) {
  if (!is_live_stream_)
    is_live_stream_ = true;

  min_seekable_time_ = min_time;
  max_seekable_time_ = max_time;
}

bool WebMediaPlayerEfl::RegisterTimeline(const std::string& timeline_selector) {
  return manager_->RegisterTimeline(player_id_, timeline_selector);
}

bool WebMediaPlayerEfl::UnRegisterTimeline(
    const std::string& timeline_selector) {
  return manager_->UnRegisterTimeline(player_id_, timeline_selector);
}

void WebMediaPlayerEfl::GetTimelinePositions(
    const std::string& timeline_selector,
    uint32_t* units_per_tick,
    uint32_t* units_per_second,
    int64_t* content_time,
    bool* paused) {
  manager_->GetTimelinePositions(player_id_, timeline_selector, units_per_tick,
                                 units_per_second, content_time, paused);
}

double WebMediaPlayerEfl::GetSpeed() {
  return manager_->GetSpeed(player_id_);
}

std::string WebMediaPlayerEfl::GetMrsUrl() {
  return manager_->GetMrsUrl(player_id_);
}

std::string WebMediaPlayerEfl::GetContentId() {
  return manager_->GetContentId(player_id_);
}

bool WebMediaPlayerEfl::SyncTimeline(const std::string& timeline_selector,
                                     int64_t timeline_pos,
                                     int64_t wallclock_pos,
                                     int tolerance) {
  return manager_->SyncTimeline(player_id_, timeline_selector, timeline_pos,
                                wallclock_pos, tolerance);
}

bool WebMediaPlayerEfl::SetWallClock(const std::string& wallclock_url) {
  return manager_->SetWallClock(player_id_, wallclock_url);
}

void WebMediaPlayerEfl::OnSyncTimeline(const std::string& timeline_selector,
                                       int sync) {
  if (client_)
    client_->OnSyncTimeline(timeline_selector, sync);
}

void WebMediaPlayerEfl::OnRegisterTimeline(
    const blink::WebMediaPlayer::register_timeline_cb_info_s& info) {
  if (client_)
    client_->OnRegisterTimeline(info);
}

void WebMediaPlayerEfl::OnMrsUrlChange(const std::string& url) {
  if (client_)
    client_->OnMrsUrlChange(url);
}

void WebMediaPlayerEfl::OnContentIdChange(const std::string& id) {
  if (client_)
    client_->OnContentIdChange(id);
}

void WebMediaPlayerEfl::OnAddAudioTrack(
    const blink::WebMediaPlayer::audio_video_track_info_s& info) {
  LOG(INFO) << "WebMediaPlayerEfl::OnAddAudioTrack Info - id: " << info.id
            << ", kind: " << info.kind.c_str()
            << ", label: " << info.label.c_str()
            << ", lang: " << info.language.c_str();

  DCHECK(main_task_runner_->BelongsToCurrentThread());
  blink::WebMediaPlayerClient::AudioTrackKind audioTrackKind =
      blink::WebMediaPlayerClient::kAudioTrackKindNone;

  for (const auto& i : audioKinds) {
    if (info.kind.compare(i.kind_str_) == 0) {
      audioTrackKind = i.kind_enum_;
      break;
    }
  }

  LOG(INFO) << "audio track kind type is:" << audioTrackKind;
  if (client_)
    client_->AddAudioTrack(blink::WebString::FromUTF8(info.id), audioTrackKind,
                           blink::WebString::FromUTF8(info.label),
                           blink::WebString::FromUTF8(info.language),
                           info.enabled);
}

void WebMediaPlayerEfl::OnAddVideoTrack(
    const blink::WebMediaPlayer::audio_video_track_info_s& info) {
  LOG(INFO) << "WebMediaPlayerEfl::OnAddVideoTrack Info - id: " << info.id
            << ", kind: " << info.kind.c_str()
            << ", label: " << info.label.c_str()
            << ", lang: " << info.language.c_str();
  DCHECK(main_task_runner_->BelongsToCurrentThread());

  blink::WebMediaPlayerClient::VideoTrackKind videoTrackKind =
      blink::WebMediaPlayerClient::kVideoTrackKindNone;

  for (const auto& i : videoKinds) {
    if (info.kind.compare(i.kind_str_) == 0) {
      videoTrackKind = i.kind_enum_;
      break;
    }
  }

  LOG(INFO) << "video track kind type is:" << videoTrackKind;
  if (client_)
    client_->AddVideoTrack(blink::WebString::FromUTF8(info.id), videoTrackKind,
                           blink::WebString::FromUTF8(info.label),
                           blink::WebString::FromUTF8(info.language),
                           info.enabled);
}

void WebMediaPlayerEfl::OnAddTextTrack(const std::string& label,
                                       const std::string& language,
                                       const std::string& id) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  blink::WebInbandTextTrack* inbandTextTrack =
      CreateWebInbandTextTrack(label, language, id);
  if (client_)
    client_->AddTextTrack(inbandTextTrack);
}

void WebMediaPlayerEfl::SetActiveTextTrack(int id, bool is_in_band) {
  manager_->SetActiveTextTrack(player_id_, id, is_in_band);
}

void WebMediaPlayerEfl::SetActiveAudioTrack(int index) {
  manager_->SetActiveAudioTrack(player_id_, index);
}

void WebMediaPlayerEfl::SetActiveVideoTrack(int index) {
  manager_->SetActiveVideoTrack(player_id_, index);
}

void WebMediaPlayerEfl::OnInbandEventTextTrack(const std::string& info,
                                               int action) {
  if (client_)
    client_->OnInbandTextTrack(info, action);
}

void WebMediaPlayerEfl::OnInbandEventTextCue(const std::string& info,
                                             unsigned int id,
                                             long long int start_time,
                                             long long int end_time) {
  if (client_)
    client_->AddInbandCue(info, id, start_time, end_time);
}

void WebMediaPlayerEfl::NotifyElementRemove() {
  manager_->NotifyElementRemove(player_id_);
}

void WebMediaPlayerEfl::SetParentalRatingResult(bool is_pass) {
  manager_->SetParentalRatingResult(player_id_, is_pass);
}

void WebMediaPlayerEfl::OnPlayerStarted(bool play_started) {
  if (client_)
    client_->OnPlayerStarted(play_started);
}
#endif

}  // namespace content
