// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/media/webmediaplayer_ms.h"

#include <stddef.h>
#include <limits>
#include <string>
#include <utility>

#include "base/bind.h"
#include "base/callback.h"
#include "base/threading/thread_task_runner_handle.h"
#include "build/build_config.h"
#include "cc/blink/web_layer_impl.h"
#include "cc/layers/video_frame_provider_client_impl.h"
#include "cc/layers/video_layer.h"
#include "content/child/child_process.h"
#include "content/public/renderer/media_stream_audio_renderer.h"
#include "content/public/renderer/media_stream_renderer_factory.h"
#include "content/public/renderer/media_stream_video_renderer.h"
#include "content/renderer/media/media_stream_audio_track.h"
#include "content/renderer/media/media_stream_video_track.h"
#include "content/renderer/media/web_media_element_source_utils.h"
#include "content/renderer/media/webmediaplayer_ms_compositor.h"
#include "content/renderer/media/webrtc_logging.h"
#include "content/renderer/render_frame_impl.h"
#include "content/renderer/render_thread_impl.h"
#include "media/base/bind_to_current_loop.h"
#include "media/base/media_content_type.h"
#include "media/base/media_log.h"
#include "media/base/video_frame.h"
#include "media/base/video_rotation.h"
#include "media/base/video_types.h"
#include "media/blink/webmediaplayer_util.h"
#include "services/ui/public/cpp/gpu/context_provider_command_buffer.h"
#include "third_party/WebKit/public/platform/WebMediaPlayerClient.h"
#include "third_party/WebKit/public/platform/WebMediaPlayerSource.h"
#include "third_party/WebKit/public/platform/WebRect.h"
#include "third_party/WebKit/public/platform/WebSize.h"
#include "third_party/WebKit/public/web/WebLocalFrame.h"
#include "third_party/WebKit/public/web/WebView.h"

#if defined(TIZEN_MULTIMEDIA_SUPPORT)
#include "content/renderer/media/tizen/renderer_demuxer_efl.h"
#include "content/renderer/media/tizen/renderer_media_player_manager_efl.h"
#endif

namespace {
enum class RendererReloadAction {
  KEEP_RENDERER,
  REMOVE_RENDERER,
  NEW_RENDERER
};
}  // namespace

namespace {
const base::TimeDelta kLayerBoundUpdateInterval =
    base::TimeDelta::FromMilliseconds(50);
}

namespace content {

// FrameDeliverer is responsible for delivering frames received on
// the IO thread by calling of EnqueueFrame() method of |compositor_|.
//
// It is created on the main thread, but methods should be called and class
// should be destructed on the IO thread.
class WebMediaPlayerMS::FrameDeliverer {
 public:
  FrameDeliverer(const base::WeakPtr<WebMediaPlayerMS>& player,
                 const MediaStreamVideoRenderer::RepaintCB& enqueue_frame_cb)
      : last_frame_opaque_(true),
        last_frame_rotation_(media::VIDEO_ROTATION_0),
        received_first_frame_(false),
        main_task_runner_(base::ThreadTaskRunnerHandle::Get()),
        player_(player),
        enqueue_frame_cb_(enqueue_frame_cb),
#if defined(TIZEN_MULTIMEDIA_SUPPORT)
        use_compositor_(true),
#endif
        weak_factory_(this) {
    io_thread_checker_.DetachFromThread();
  }

  ~FrameDeliverer() { DCHECK(io_thread_checker_.CalledOnValidThread()); }

  void OnVideoFrame(scoped_refptr<media::VideoFrame> frame) {
    DCHECK(io_thread_checker_.CalledOnValidThread());
#if defined(OS_ANDROID)
    if (render_frame_suspended_)
      return;
#endif  // defined(OS_ANDROID)
#if defined(TIZEN_MULTIMEDIA_SUPPORT)
    if (!use_compositor_) {
      main_task_runner_->PostTask(
          FROM_HERE,
          base::BindOnce(&WebMediaPlayerMS::OnFrame, player_, frame));
      return;
    }
#endif

    base::TimeTicks render_time;
    if (frame->metadata()->GetTimeTicks(
            media::VideoFrameMetadata::REFERENCE_TIME, &render_time)) {
      TRACE_EVENT1("webmediaplayerms", "OnVideoFrame", "Ideal Render Instant",
                   render_time.ToInternalValue());
    } else {
      TRACE_EVENT0("webmediaplayerms", "OnVideoFrame");
    }

    const bool is_opaque = media::IsOpaque(frame->format());
    media::VideoRotation video_rotation = media::VIDEO_ROTATION_0;
    ignore_result(frame->metadata()->GetRotation(
        media::VideoFrameMetadata::ROTATION, &video_rotation));

    if (!received_first_frame_) {
      received_first_frame_ = true;
      last_frame_opaque_ = is_opaque;
      last_frame_rotation_ = video_rotation;
      main_task_runner_->PostTask(
          FROM_HERE, base::BindOnce(&WebMediaPlayerMS::OnFirstFrameReceived,
                                    player_, video_rotation, is_opaque));
    } else {
      if (last_frame_opaque_ != is_opaque) {
        last_frame_opaque_ = is_opaque;
        main_task_runner_->PostTask(
            FROM_HERE, base::BindOnce(&WebMediaPlayerMS::OnOpacityChanged,
                                      player_, is_opaque));
      }
      if (last_frame_rotation_ != video_rotation) {
        last_frame_rotation_ = video_rotation;
        main_task_runner_->PostTask(
            FROM_HERE, base::BindOnce(&WebMediaPlayerMS::OnRotationChanged,
                                      player_, video_rotation, is_opaque));
      }
    }

    enqueue_frame_cb_.Run(frame);
    OnFrameRendered(player_, *main_task_runner_);
  }

#if defined(OS_ANDROID)
  void SetRenderFrameSuspended(bool render_frame_suspended) {
    DCHECK(io_thread_checker_.CalledOnValidThread());
    render_frame_suspended_ = render_frame_suspended;
  }
#endif  // defined(OS_ANDROID)

  MediaStreamVideoRenderer::RepaintCB GetRepaintCallback() {
    return base::Bind(&FrameDeliverer::OnVideoFrame,
                      weak_factory_.GetWeakPtr());
  }

  void OnDecoderTypeAvailable(
      scoped_refptr<media::MediaStreamVideoDecoderData> decoder_data) {
#if defined(TIZEN_MULTIMEDIA_SUPPORT)
    use_compositor_ = (decoder_data->type == media::TizenDecoderType::TEXTURE);
#endif
    received_first_frame_ = false;
#if defined(TIZEN_MULTIMEDIA_SUPPORT)
    LOG(INFO) << __FUNCTION__ << " use compositor " << use_compositor_;
    main_task_runner_->PostTask(
        FROM_HERE, base::BindOnce(&WebMediaPlayerMS::OnDecoderData, player_,
                                  decoder_data));
#endif
  }

  MediaStreamVideoRenderer::DecoderDataCB GetDecoderDataCallback() {
    return base::Bind(&FrameDeliverer::OnDecoderTypeAvailable,
                      weak_factory_.GetWeakPtr());
  }

  void OnSuspendingDecoder(
      scoped_refptr<media::SuspendingDecoder> suspending_decoder) {
    main_task_runner_->PostTask(
        FROM_HERE, base::BindOnce(&WebMediaPlayerMS::OnSuspendingDecoder,
                                  player_, suspending_decoder));
  }

  MediaStreamVideoRenderer::SuspendingDecoderCB GetSuspendingDecoderCallback() {
    return base::Bind(&FrameDeliverer::OnSuspendingDecoder,
                      weak_factory_.GetWeakPtr());
  }

 private:
  bool last_frame_opaque_;
  media::VideoRotation last_frame_rotation_;
  bool received_first_frame_;

#if defined(OS_ANDROID)
  bool render_frame_suspended_ = false;
#endif  // defined(OS_ANDROID)

  const scoped_refptr<base::SingleThreadTaskRunner> main_task_runner_;
  const base::WeakPtr<WebMediaPlayerMS> player_;
  const MediaStreamVideoRenderer::RepaintCB enqueue_frame_cb_;
#if defined(TIZEN_MULTIMEDIA_SUPPORT)
  bool use_compositor_;
#endif

  // Used for DCHECKs to ensure method calls are executed on the correct thread.
  base::ThreadChecker io_thread_checker_;

  base::WeakPtrFactory<FrameDeliverer> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(FrameDeliverer);
};

WebMediaPlayerMS::WebMediaPlayerMS(
    blink::WebLocalFrame* frame,
    blink::WebMediaPlayerClient* client,
    media::WebMediaPlayerDelegate* delegate,
    std::unique_ptr<media::MediaLog> media_log,
    std::unique_ptr<MediaStreamRendererFactory> factory,
#if defined(TIZEN_MULTIMEDIA_SUPPORT)
    RendererMediaPlayerManager* manager,
#endif
    scoped_refptr<base::SingleThreadTaskRunner> io_task_runner,
    scoped_refptr<base::SingleThreadTaskRunner> compositor_task_runner,
    scoped_refptr<base::SingleThreadTaskRunner> media_task_runner,
    scoped_refptr<base::TaskRunner> worker_task_runner,
    media::GpuVideoAcceleratorFactories* gpu_factories,
    const blink::WebString& sink_id,
    const blink::WebSecurityOrigin& security_origin)
    : frame_(frame),
      network_state_(WebMediaPlayer::kNetworkStateEmpty),
      ready_state_(WebMediaPlayer::kReadyStateHaveNothing),
      decoder_type_(WebMediaPlayer::kDecoderTypeNone),
      buffered_(static_cast<size_t>(0)),
      client_(client),
      delegate_(delegate),
      delegate_id_(0),
      paused_(true),
      video_rotation_(media::VIDEO_ROTATION_0),
      media_log_(std::move(media_log)),
      renderer_factory_(std::move(factory)),
      io_task_runner_(io_task_runner),
      compositor_task_runner_(compositor_task_runner),
#if defined(TIZEN_MULTIMEDIA_SUPPORT)
      manager_(manager),
#endif
      media_task_runner_(media_task_runner),
      worker_task_runner_(worker_task_runner),
      gpu_factories_(gpu_factories),
      initial_audio_output_device_id_(sink_id.Utf8()),
      initial_security_origin_(security_origin.IsNull()
                                   ? url::Origin()
                                   : url::Origin(security_origin)),
      volume_(1.0),
      volume_multiplier_(1.0),
      should_play_upon_shown_(false) {
  DVLOG(1) << __func__;
  DCHECK(client);
  DCHECK(delegate_);
  delegate_id_ = delegate_->AddObserver(this);

  media_log_->AddEvent(
      media_log_->CreateEvent(media::MediaLogEvent::WEBMEDIAPLAYER_CREATED));

#if defined(TIZEN_MULTIMEDIA_SUPPORT)
  player_id_ = manager_->RegisterMediaPlayer(this);
#endif
}

WebMediaPlayerMS::~WebMediaPlayerMS() {
  DVLOG(1) << __func__;
  DCHECK(thread_checker_.CalledOnValidThread());

  if (!web_stream_.IsNull())
    web_stream_.RemoveObserver(this);

  // Destruct compositor resources in the proper order.
  get_client()->SetWebLayer(nullptr);
  if (video_weblayer_)
    static_cast<cc::VideoLayer*>(video_weblayer_->layer())->StopUsingProvider();

  if (frame_deliverer_)
    io_task_runner_->DeleteSoon(FROM_HERE, frame_deliverer_.release());

  if (compositor_)
    compositor_->StopUsingProvider();

  if (video_frame_provider_)
    video_frame_provider_->Stop();

  if (audio_renderer_)
    audio_renderer_->Stop();

  media_log_->AddEvent(
      media_log_->CreateEvent(media::MediaLogEvent::WEBMEDIAPLAYER_DESTROYED));

  delegate_->PlayerGone(delegate_id_);
  delegate_->RemoveObserver(delegate_id_);

#if defined(TIZEN_MULTIMEDIA_SUPPORT)
  if (manager_) {
    manager_->DestroyPlayer(player_id_);
    manager_->UnregisterMediaPlayer(player_id_);
  }
#endif
}

void WebMediaPlayerMS::Load(LoadType load_type,
                            const blink::WebMediaPlayerSource& source,
                            CORSMode /*cors_mode*/) {
  DVLOG(1) << __func__;
  DCHECK(thread_checker_.CalledOnValidThread());

  // TODO(acolwell): Change this to DCHECK_EQ(load_type, LoadTypeMediaStream)
  // once Blink-side changes land.
  DCHECK_NE(load_type, kLoadTypeMediaSource);
  web_stream_ = GetWebMediaStreamFromWebMediaPlayerSource(source);
  if (!web_stream_.IsNull())
    web_stream_.AddObserver(this);

  compositor_ = new WebMediaPlayerMSCompositor(
#if defined(TIZEN_VIDEO_HOLE)
      media::BindToCurrentLoop(base::Bind(
          &WebMediaPlayerMS::OnDrawableContentRectChanged, AsWeakPtr())),
#endif
      compositor_task_runner_, io_task_runner_, web_stream_, AsWeakPtr());

  SetNetworkState(WebMediaPlayer::kNetworkStateLoading);
  SetReadyStateInternal(WebMediaPlayer::kReadyStateHaveNothing);
  std::string stream_id =
      web_stream_.IsNull() ? std::string() : web_stream_.Id().Utf8();
  media_log_->AddEvent(media_log_->CreateLoadEvent(stream_id));

  frame_deliverer_.reset(new WebMediaPlayerMS::FrameDeliverer(
      AsWeakPtr(),
      base::Bind(&WebMediaPlayerMSCompositor::EnqueueFrame, compositor_)));
  video_frame_provider_ = renderer_factory_->GetVideoRenderer(
      web_stream_,
      media::BindToCurrentLoop(
          base::Bind(&WebMediaPlayerMS::OnSourceError, AsWeakPtr())),
      frame_deliverer_->GetRepaintCallback(),
      frame_deliverer_->GetDecoderDataCallback(),
      frame_deliverer_->GetSuspendingDecoderCallback(), io_task_runner_,
      media_task_runner_, worker_task_runner_, gpu_factories_);

  RenderFrame* const frame = RenderFrame::FromWebFrame(frame_);

  int routing_id = MSG_ROUTING_NONE;
  GURL url = source.IsURL() ? GURL(source.GetAsURL()) : GURL();

  if (frame) {
    // Report UMA and RAPPOR metrics.
    media::ReportMetrics(load_type, url, frame_->GetSecurityOrigin(),
                         media_log_.get());
    routing_id = frame->GetRoutingID();
  }

  audio_renderer_ = renderer_factory_->GetAudioRenderer(
      web_stream_, routing_id, initial_audio_output_device_id_,
      initial_security_origin_);

  if (!audio_renderer_)
    WebRtcLogMessage("Warning: Failed to instantiate audio renderer.");

  if (!video_frame_provider_ && !audio_renderer_) {
    SetNetworkState(WebMediaPlayer::kNetworkStateNetworkError);
    return;
  }

  if (audio_renderer_) {
    audio_renderer_->SetVolume(volume_);
    audio_renderer_->Start();

    // Store the ID of audio track being played in |current_video_track_id_|
    blink::WebVector<blink::WebMediaStreamTrack> audio_tracks;
    if (!web_stream_.IsNull()) {
      web_stream_.AudioTracks(audio_tracks);
      DCHECK_GT(audio_tracks.size(), 0U);
      current_audio_track_id_ = audio_tracks[0].Id();
    }
  }

  if (video_frame_provider_) {
    video_frame_provider_->Start();

    // Store the ID of video track being played in |current_video_track_id_|
    if (!web_stream_.IsNull()) {
      blink::WebVector<blink::WebMediaStreamTrack> video_tracks;
      web_stream_.VideoTracks(video_tracks);
      DCHECK_GT(video_tracks.size(), 0U);
      current_video_track_id_ = video_tracks[0].Id();
    }
  }
  // When associated with an <audio> element, we don't want to wait for the
  // first video fram to become available as we do for <video> elements
  // (<audio> elements can also be assigned video tracks).
  // For more details, see crbug.com/738379
  if (audio_renderer_ &&
      (client_->IsAudioElement() || !video_frame_provider_)) {
    // This is audio-only mode.
    UpgradeReadyState();
  }
}

void WebMediaPlayerMS::OnSuspendingDecoder(
    scoped_refptr<media::SuspendingDecoder> suspending_decoder) {
  suspending_decoder_ = suspending_decoder;
}

#if defined(TIZEN_MULTIMEDIA_SUPPORT)
void WebMediaPlayerMS::OnDecoderData(
    scoped_refptr<media::MediaStreamVideoDecoderData> decoder_data) {
  auto HasDecoderChanged =
      [](scoped_refptr<media::MediaStreamVideoDecoderData>& old_data,
         scoped_refptr<media::MediaStreamVideoDecoderData>& new_data) -> bool {
    if (!old_data)
      return true;
    return old_data->type != new_data->type;
  };

  if (HasDecoderChanged(decoder_data_, decoder_data)) {
    switch (decoder_data->type) {
      case media::TizenDecoderType::ENCODED:
        SetDecoderType(blink::WebMediaPlayer::kDecoderTypeHardware);
        break;
      case media::TizenDecoderType::TEXTURE:
        SetDecoderType(blink::WebMediaPlayer::kDecoderTypeSoftware);
        break;
      default:
        break;
    }

    if (decoder_data->type == media::TizenDecoderType::ENCODED) {
      // Set current time equal to the time reported by the previous
      // sink to keep timestamps consistent
      OnTimeUpdate(base::TimeDelta::FromSecondsD(CurrentTime()));
      received_first_frame_ = false;
      OnMediaDataChange(natural_size_.width(), natural_size_.height(), 1);
      if (!demuxer_) {
        demuxer_ = static_cast<content::RendererDemuxerEfl*>(
            RenderThreadImpl::current()->renderer_demuxer());
        demuxer_client_id_ = demuxer_->GetNextDemuxerClientID();
        bool ret = manager_->InitializeSync(
            player_id_, /*player_type_*/
            MEDIA_PLAYER_TYPE_WEBRTC_MEDIA_STREAM_WITH_VIDEO_HOLE,
            media::GetCleanURL(""),
            blink::WebString(client_->GetContentMIMEType()).Utf8(),
            demuxer_client_id_);
        if (!ret) {
          demuxer_ = nullptr;
          LOG(WARNING) << "Failed to initialize WebRTC player."
                          " Switching to SW decoding";

          // Retry asynchronously with a placeholder for SVAC messages
          manager_->Initialize(
              player_id_, MEDIA_PLAYER_TYPE_WEBRTC_PLACEHOLDER,
              media::GetCleanURL(""),
              blink::WebString(client_->GetContentMIMEType()).Utf8(),
              demuxer_client_id_);

          // To be able to Resume if the HW decoding becomes available in the
          // future
          if (suspending_decoder_) {
            is_decoder_suspending_ = true;
            suspending_decoder_->Suspend();
          }
        }
      } else {
        manager_->Resume(player_id_);
      }
    } else {
      is_decoder_suspending_ = false;
      LOG(INFO) << player_id_ << ": No info for browser manager";
    }
  }
  decoder_data_ = decoder_data;
  if (!suspended_)
    decoder_type_before_suspend_ = decoder_data->type;
}

void WebMediaPlayerMS::OnFrame(scoped_refptr<media::VideoFrame> frame) {
  if (!received_first_frame_) {
    received_first_frame_ = true;
    if (demuxer_ && decoder_data_) {
      media::DemuxerConfigs configs;
      configs.video_codec = decoder_data_->config.codec();
      configs.video_size = frame->coded_size();
      LOG(INFO) << "Send config. Codec:" << configs.video_codec
                << " size: " << configs.video_size.ToString();
      // Passing infinite duration and buffered ranges to prevent playback stall
      demuxer_->DurationChanged(demuxer_client_id_, base::TimeDelta::Max());
      media::Ranges<base::TimeDelta> ranges;
      ranges.Add(base::TimeDelta(), base::TimeDelta::Max());
      demuxer_->DemuxerBufferedChanged(demuxer_client_id_, ranges);
      demuxer_->DemuxerReady(demuxer_client_id_, configs);
    }
  }

  uint32_t shared_memory_size = -1;
  base::SharedMemory shared_memory;
  base::SharedMemoryHandle foreign_memory_handle;
  media::DemuxedBufferMetaData meta_data;

  shared_memory_size = frame->stride(0);
  if (!shared_memory.CreateAndMapAnonymous(shared_memory_size)) {
    LOG(ERROR) << "Shared Memory creation failed.";
    return;
  }

  foreign_memory_handle = shared_memory.handle().Duplicate();
  if (!foreign_memory_handle.IsValid()) {
    LOG(ERROR) << "Shared Memory handle could not be obtained";
    return;
  }

  UpgradeReadyState();
  memcpy(shared_memory.memory(), reinterpret_cast<void*>(frame->data(0)),
         shared_memory_size);
  meta_data.size = shared_memory_size;
  meta_data.timestamp = frame->timestamp();
  meta_data.type = media::DemuxerStream::VIDEO;
  meta_data.status = media::DemuxerStream::kOk;
  meta_data.time_duration = base::TimeDelta::FromMilliseconds(50);
  meta_data.end_of_stream = false;

  if (demuxer_)
    demuxer_->ReadFromDemuxerAck(demuxer_client_id_, foreign_memory_handle,
                                 meta_data);
}
#endif

void WebMediaPlayerMS::TrackAdded(const blink::WebMediaStreamTrack& track) {
  Reload();
}

void WebMediaPlayerMS::TrackRemoved(const blink::WebMediaStreamTrack& track) {
  Reload();
}

void WebMediaPlayerMS::Reload() {
  DCHECK(thread_checker_.CalledOnValidThread());
  if (web_stream_.IsNull())
    return;

  ReloadVideo();
  ReloadAudio();
}

void WebMediaPlayerMS::ReloadVideo() {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(!web_stream_.IsNull());
  blink::WebVector<blink::WebMediaStreamTrack> video_tracks;
  // VideoTracks() is a getter.
  web_stream_.VideoTracks(video_tracks);

  RendererReloadAction renderer_action = RendererReloadAction::KEEP_RENDERER;
  if (video_tracks.IsEmpty()) {
    if (video_frame_provider_)
      renderer_action = RendererReloadAction::REMOVE_RENDERER;
    current_video_track_id_ = blink::WebString();
  } else if (video_tracks[0].Id() != current_video_track_id_) {
    renderer_action = RendererReloadAction::NEW_RENDERER;
    current_video_track_id_ = video_tracks[0].Id();
  }

  switch (renderer_action) {
    case RendererReloadAction::NEW_RENDERER:
      if (video_frame_provider_)
        video_frame_provider_->Stop();

      video_frame_provider_ = renderer_factory_->GetVideoRenderer(
          web_stream_,
          media::BindToCurrentLoop(
              base::Bind(&WebMediaPlayerMS::OnSourceError, AsWeakPtr())),
          frame_deliverer_->GetRepaintCallback(),
          frame_deliverer_->GetDecoderDataCallback(),
          frame_deliverer_->GetSuspendingDecoderCallback(), io_task_runner_,
          media_task_runner_, worker_task_runner_, gpu_factories_);
      DCHECK(video_frame_provider_);
      video_frame_provider_->Start();
      break;

    case RendererReloadAction::REMOVE_RENDERER:
      video_frame_provider_->Stop();
      video_frame_provider_ = nullptr;
      break;

    default:
      return;
  }

  DCHECK_NE(renderer_action, RendererReloadAction::KEEP_RENDERER);
  if (!paused_)
    delegate_->DidPlayerSizeChange(delegate_id_, NaturalSize());
}

void WebMediaPlayerMS::ReloadAudio() {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(!web_stream_.IsNull());
  RenderFrame* const frame = RenderFrame::FromWebFrame(frame_);
  if (!frame)
    return;

  blink::WebVector<blink::WebMediaStreamTrack> audio_tracks;
  // AudioTracks() is a getter.
  web_stream_.AudioTracks(audio_tracks);

  RendererReloadAction renderer_action = RendererReloadAction::KEEP_RENDERER;
  if (audio_tracks.IsEmpty()) {
    if (audio_renderer_)
      renderer_action = RendererReloadAction::REMOVE_RENDERER;
    current_audio_track_id_ = blink::WebString();
  } else if (audio_tracks[0].Id() != current_video_track_id_) {
    renderer_action = RendererReloadAction::NEW_RENDERER;
    current_audio_track_id_ = audio_tracks[0].Id();
  }

  switch (renderer_action) {
    case RendererReloadAction::NEW_RENDERER:
      if (audio_renderer_)
        audio_renderer_->Stop();

      audio_renderer_ = renderer_factory_->GetAudioRenderer(
          web_stream_, frame->GetRoutingID(), initial_audio_output_device_id_,
          initial_security_origin_);
      audio_renderer_->SetVolume(volume_);
      audio_renderer_->Start();
      audio_renderer_->Play();
      break;

    case RendererReloadAction::REMOVE_RENDERER:
      audio_renderer_->Stop();
      audio_renderer_ = nullptr;
      break;

    default:
      break;
  }
}

void WebMediaPlayerMS::Play() {
  DVLOG(1) << __func__;
  DCHECK(thread_checker_.CalledOnValidThread());

  media_log_->AddEvent(media_log_->CreateEvent(media::MediaLogEvent::PLAY));
  if (!paused_)
    return;
#if defined(TIZEN_MULTIMEDIA_SUPPORT)
  manager_->Play(player_id_);
#endif

  if (GetTizenDecoderType() == media::TizenDecoderType::TEXTURE) {
    if (video_frame_provider_)
      video_frame_provider_->Resume();

    compositor_->StartRendering();
  }

  frames_handler_.ResumeOnPlay(this);

  if (audio_renderer_)
    audio_renderer_->Play();

  if (HasVideo())
    delegate_->DidPlayerSizeChange(delegate_id_, NaturalSize());

  // |delegate_| expects the notification only if there is at least one track
  // actually playing. A media stream might have none since tracks can be
  // removed from the stream.
  if (HasAudio() || HasVideo()) {
    // TODO(perkj, magjed): We use OneShot focus type here so that it takes
    // audio focus once it starts, and then will not respond to further audio
    // focus changes. See http://crbug.com/596516 for more details.
    delegate_->DidPlay(delegate_id_, HasVideo(), HasAudio(),
                       media::MediaContentType::OneShot);
  }

  delegate_->SetIdle(delegate_id_, false);
  paused_ = false;
}

void WebMediaPlayerMS::Pause() {
  DVLOG(1) << __func__;
  DCHECK(thread_checker_.CalledOnValidThread());

  should_play_upon_shown_ = false;
  media_log_->AddEvent(media_log_->CreateEvent(media::MediaLogEvent::PAUSE));
  if (paused_ && !frames_handler_.WaitingToPause())
    return;
#if defined(TIZEN_MULTIMEDIA_SUPPORT)
  manager_->Pause(player_id_, false);
#endif

  auto decoder_type = GetTizenDecoderType();
  if (decoder_type == media::TizenDecoderType::TEXTURE) {
    if (video_frame_provider_)
      video_frame_provider_->Pause();

    compositor_->StopRendering();
    compositor_->ReplaceCurrentFrameWithACopy();
  } else if (decoder_type == media::TizenDecoderType::ENCODED) {
    LOG(INFO) << "[OnFrameRendered] Switching to SUSPEND(HW)";
    frames_handler_ = WaitingForImageFrameHandler();
    SuspendInternal(false);
  }

  if (audio_renderer_)
    audio_renderer_->Pause();

  delegate_->DidPause(delegate_id_);
  delegate_->SetIdle(delegate_id_, true);

  paused_ = true;
}

void WebMediaPlayerMS::OnFrameRendered_PauseAfterDelivery(
    const base::WeakPtr<WebMediaPlayerMS>& self,
    base::SingleThreadTaskRunner& task_runner) {
  LOG(INFO) << "[OnFrameRendered] Posting a task...";
  task_runner.PostTask(
      FROM_HERE,
      base::BindOnce(&WebMediaPlayerMS::OnFrameRendered_DoPause, self));
}

void WebMediaPlayerMS::OnFrameRendered_DoPause() {
  LOG(INFO) << "[OnFrameRendered] Pausing";
  frames_handler_ = ResumeOnPlayFrameHandler();
  Pause();
}

void WebMediaPlayerMS::ResumeOnPlay_IfPossible(WebMediaPlayerMS* self) {
  self->frames_handler_ = DefaultFrameHandler();
  // effectively logical-and:
  // if (ResumeOnPlay_IfPossible && UpgradeDecoder_UpgradeToHardware)
  //   self->Resume();
  LOG(INFO) << self << " :: [VSAC][WebRTC] ResumeOnPlay_IfPossible";
  self->UpgradeDecoder();
}

void WebMediaPlayerMS::UpgradeDecoder_UpgradeToHardware(
    WebMediaPlayerMS* self) {
  LOG(INFO) << self << " :: [VSAC][WebRTC] UpgradeDecoder_UpgradeToHardware";
  self->ResumeInternal(false);
}

bool WebMediaPlayerMS::SupportsSave() const {
  DCHECK(thread_checker_.CalledOnValidThread());
  return false;
}

void WebMediaPlayerMS::Seek(double seconds) {
  DCHECK(thread_checker_.CalledOnValidThread());
}

void WebMediaPlayerMS::SetRate(double rate) {
  DCHECK(thread_checker_.CalledOnValidThread());
}

void WebMediaPlayerMS::SetVolume(double volume) {
  DVLOG(1) << __func__ << "(volume=" << volume << ")";
  DCHECK(thread_checker_.CalledOnValidThread());
  volume_ = volume;
  if (audio_renderer_.get())
    audio_renderer_->SetVolume(volume_ * volume_multiplier_);
  delegate_->DidPlayerMutedStatusChange(delegate_id_, volume == 0.0);
}

void WebMediaPlayerMS::SetSinkId(
    const blink::WebString& sink_id,
    const blink::WebSecurityOrigin& security_origin,
    blink::WebSetSinkIdCallbacks* web_callback) {
  DVLOG(1) << __func__;
  DCHECK(thread_checker_.CalledOnValidThread());
  const media::OutputDeviceStatusCB callback =
      media::ConvertToOutputDeviceStatusCB(web_callback);
  if (audio_renderer_) {
    audio_renderer_->SwitchOutputDevice(sink_id.Utf8(), security_origin,
                                        callback);
  } else {
    callback.Run(media::OUTPUT_DEVICE_STATUS_ERROR_INTERNAL);
  }
}

void WebMediaPlayerMS::SetPreload(WebMediaPlayer::Preload preload) {
  DCHECK(thread_checker_.CalledOnValidThread());
}

bool WebMediaPlayerMS::HasVideo() const {
  DCHECK(thread_checker_.CalledOnValidThread());
  return (video_frame_provider_.get() != nullptr);
}

bool WebMediaPlayerMS::HasAudio() const {
  DCHECK(thread_checker_.CalledOnValidThread());
  return (audio_renderer_.get() != nullptr);
}

blink::WebSize WebMediaPlayerMS::NaturalSize() const {
  DCHECK(thread_checker_.CalledOnValidThread());
  if (!video_frame_provider_)
    return blink::WebSize();

  if (video_rotation_ == media::VIDEO_ROTATION_90 ||
      video_rotation_ == media::VideoRotation::VIDEO_ROTATION_270) {
    const gfx::Size& current_size = compositor_->GetCurrentSize();
    return blink::WebSize(current_size.height(), current_size.width());
  }
  return blink::WebSize(compositor_->GetCurrentSize());
}

blink::WebSize WebMediaPlayerMS::VisibleRect() const {
  DCHECK(thread_checker_.CalledOnValidThread());
  scoped_refptr<media::VideoFrame> video_frame =
      compositor_->GetCurrentFrameWithoutUpdatingStatistics();
  if (!video_frame)
    return blink::WebSize();

  const gfx::Rect& visible_rect = video_frame->visible_rect();
  if (video_rotation_ == media::VIDEO_ROTATION_90 ||
      video_rotation_ == media::VideoRotation::VIDEO_ROTATION_270) {
    return blink::WebSize(visible_rect.height(), visible_rect.width());
  }
  return blink::WebSize(visible_rect.width(), visible_rect.height());
}

bool WebMediaPlayerMS::Paused() const {
  DCHECK(thread_checker_.CalledOnValidThread());
  return paused_;
}

bool WebMediaPlayerMS::Seeking() const {
  DCHECK(thread_checker_.CalledOnValidThread());
  return false;
}

double WebMediaPlayerMS::Duration() const {
  DCHECK(thread_checker_.CalledOnValidThread());
  return std::numeric_limits<double>::infinity();
}

double WebMediaPlayerMS::CurrentTime() const {
  DCHECK(thread_checker_.CalledOnValidThread());
  // For encoded frame current data is fetched from MM player using
  // OnTimeUpdate event.
  if (GetTizenDecoderType() == media::TizenDecoderType::TEXTURE) {
    const base::TimeDelta current_time = compositor_->GetCurrentTime();
    if (current_time.ToInternalValue() != 0)
      return current_time.InSecondsF();
    else if (audio_renderer_.get())
      return audio_renderer_->GetCurrentRenderTime().InSecondsF();
  }
  return current_time_.InSecondsF();
}

#if defined(TIZEN_MULTIMEDIA_SUPPORT)
void WebMediaPlayerMS::Suspend() {
  if (suspended_)
    return;
  suspended_ = true;
  if (decoder_type_before_suspend_ == media::TizenDecoderType::TEXTURE)
    return;
  SuspendInternal();
}

void WebMediaPlayerMS::Resume() {
  if (!suspended_)
    return;
  suspended_ = false;

  if (decoder_type_before_suspend_ == media::TizenDecoderType::TEXTURE)
    return;
  ResumeInternal();
}
#endif

void WebMediaPlayerMS::SuspendInternal(bool external_suspend) {
  if (GetTizenDecoderType() == media::TizenDecoderType::ENCODED) {
    if (suspending_decoder_) {
      is_decoder_suspending_ = true;
      suspending_decoder_->Suspend();
    }
#if defined(TIZEN_MULTIMEDIA_SUPPORT)
    external_suspend ? manager_->Suspend(player_id_)
                     : manager_->Deactivate(player_id_);
#endif
  }
}

void WebMediaPlayerMS::ResumeInternal(bool external_resume) {
  if (suspending_decoder_)
    suspending_decoder_->Resume();
#if defined(TIZEN_MULTIMEDIA_SUPPORT)
  external_resume ? manager_->Resume(player_id_)
                  : manager_->Activate(player_id_);
#endif
}

blink::WebMediaPlayer::NetworkState WebMediaPlayerMS::GetNetworkState() const {
  DVLOG(1) << __func__ << ", state:" << network_state_;
  DCHECK(thread_checker_.CalledOnValidThread());
  return network_state_;
}

blink::WebMediaPlayer::ReadyState WebMediaPlayerMS::GetReadyState() const {
  DVLOG(1) << __func__ << ", state:" << ready_state_;
  DCHECK(thread_checker_.CalledOnValidThread());
  return ready_state_;
}

blink::WebMediaPlayer::DecoderType WebMediaPlayerMS::GetDecoderType() const {
  DVLOG(1) << __func__ << ", type:" << decoder_type_;
  DCHECK(thread_checker_.CalledOnValidThread());
  return decoder_type_;
}

blink::WebString WebMediaPlayerMS::GetErrorMessage() const {
  return blink::WebString::FromUTF8(media_log_->GetErrorMessage());
}

blink::WebTimeRanges WebMediaPlayerMS::Buffered() const {
  DCHECK(thread_checker_.CalledOnValidThread());
  return buffered_;
}

blink::WebTimeRanges WebMediaPlayerMS::Seekable() const {
  DCHECK(thread_checker_.CalledOnValidThread());
  return blink::WebTimeRanges();
}

bool WebMediaPlayerMS::DidLoadingProgress() {
  DCHECK(thread_checker_.CalledOnValidThread());
  return true;
}

void WebMediaPlayerMS::Paint(blink::WebCanvas* canvas,
                             const blink::WebRect& rect,
                             cc::PaintFlags& flags,
                             int already_uploaded_id,
                             VideoFrameUploadMetadata* out_metadata) {
  DVLOG(3) << __func__;
  DCHECK(thread_checker_.CalledOnValidThread());

  const scoped_refptr<media::VideoFrame> frame =
      compositor_->GetCurrentFrameWithoutUpdatingStatistics();

  media::Context3D context_3d;
  if (frame && frame->HasTextures()) {
    auto* provider =
        RenderThreadImpl::current()->SharedMainThreadContextProvider().get();
    // GPU Process crashed.
    if (!provider)
      return;
    context_3d = media::Context3D(provider->ContextGL(), provider->GrContext());
    DCHECK(context_3d.gl);
  }
  const gfx::RectF dest_rect(rect.x, rect.y, rect.width, rect.height);
  video_renderer_.Paint(frame, canvas, dest_rect, flags, video_rotation_,
                        context_3d);
}

bool WebMediaPlayerMS::HasSingleSecurityOrigin() const {
  DCHECK(thread_checker_.CalledOnValidThread());
  return true;
}

bool WebMediaPlayerMS::DidPassCORSAccessCheck() const {
  DCHECK(thread_checker_.CalledOnValidThread());
  return true;
}

double WebMediaPlayerMS::MediaTimeForTimeValue(double timeValue) const {
  return base::TimeDelta::FromSecondsD(timeValue).InSecondsF();
}

unsigned WebMediaPlayerMS::DecodedFrameCount() const {
  DCHECK(thread_checker_.CalledOnValidThread());
  return compositor_->total_frame_count();
}

unsigned WebMediaPlayerMS::DroppedFrameCount() const {
  DCHECK(thread_checker_.CalledOnValidThread());
  return compositor_->dropped_frame_count();
}

size_t WebMediaPlayerMS::AudioDecodedByteCount() const {
  DCHECK(thread_checker_.CalledOnValidThread());
  NOTIMPLEMENTED();
  return 0;
}

size_t WebMediaPlayerMS::VideoDecodedByteCount() const {
  DCHECK(thread_checker_.CalledOnValidThread());
  NOTIMPLEMENTED();
  return 0;
}

void WebMediaPlayerMS::OnFrameHidden() {
#if defined(OS_ANDROID)
  DCHECK(thread_checker_.CalledOnValidThread());

  // Method called when the RenderFrame is sent to background and suspended
  // (android). Substitute the displayed VideoFrame with a copy to avoid
  // holding on to it unnecessarily.
  //
  // During undoable tab closures OnHidden() may be called back to back, so we
  // can't rely on |render_frame_suspended_| being false here.
  if (frame_deliverer_) {
    io_task_runner_->PostTask(
        FROM_HERE, base::Bind(&FrameDeliverer::SetRenderFrameSuspended,
                              base::Unretained(frame_deliverer_.get()), true));
  }

  if (!paused_)
    compositor_->ReplaceCurrentFrameWithACopy();
#endif  // defined(OS_ANDROID)
}

void WebMediaPlayerMS::OnFrameClosed() {
#if defined(OS_ANDROID)
  if (!paused_) {
    Pause();
    should_play_upon_shown_ = true;
  }

  delegate_->PlayerGone(delegate_id_);

  if (frame_deliverer_) {
    io_task_runner_->PostTask(
        FROM_HERE, base::Bind(&FrameDeliverer::SetRenderFrameSuspended,
                              base::Unretained(frame_deliverer_.get()), true));
  }
#endif  // defined(OS_ANDROID)
}

void WebMediaPlayerMS::OnFrameShown() {
#if defined(OS_ANDROID)
  DCHECK(thread_checker_.CalledOnValidThread());

  if (frame_deliverer_) {
    io_task_runner_->PostTask(
        FROM_HERE, base::Bind(&FrameDeliverer::SetRenderFrameSuspended,
                              base::Unretained(frame_deliverer_.get()), false));
  }

  // Resume playback on visibility. play() clears |should_play_upon_shown_|.
  if (should_play_upon_shown_)
    Play();
#endif  // defined(OS_ANDROID)
}

void WebMediaPlayerMS::OnIdleTimeout() {}

void WebMediaPlayerMS::OnPlay() {
  // TODO(perkj, magjed): It's not clear how WebRTC should work with an
  // MediaSession, until these issues are resolved, disable session controls.
  // http://crbug.com/595297.
}

void WebMediaPlayerMS::OnPause() {
  // TODO(perkj, magjed): See TODO in OnPlay().
}

void WebMediaPlayerMS::OnVolumeMultiplierUpdate(double multiplier) {
  // TODO(perkj, magjed): See TODO in OnPlay().
}

void WebMediaPlayerMS::OnBecamePersistentVideo(bool value) {
  get_client()->OnBecamePersistentVideo(value);
}

bool WebMediaPlayerMS::CopyVideoTextureToPlatformTexture(
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
    VideoFrameUploadMetadata* out_metadata) {
  TRACE_EVENT0("webmediaplayerms", "copyVideoTextureToPlatformTexture");
  DCHECK(thread_checker_.CalledOnValidThread());

  scoped_refptr<media::VideoFrame> video_frame =
      compositor_->GetCurrentFrameWithoutUpdatingStatistics();

  if (!video_frame.get() || !video_frame->HasTextures())
    return false;

  media::Context3D context_3d;
  auto* provider =
      RenderThreadImpl::current()->SharedMainThreadContextProvider().get();
  // GPU Process crashed.
  if (!provider)
    return false;
  context_3d = media::Context3D(provider->ContextGL(), provider->GrContext());
  DCHECK(context_3d.gl);
  return video_renderer_.CopyVideoFrameTexturesToGLTexture(
      context_3d, gl, video_frame.get(), target, texture, internal_format,
      format, type, level, premultiply_alpha, flip_y);
}

bool WebMediaPlayerMS::TexImageImpl(TexImageFunctionID functionID,
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
                                    bool premultiply_alpha) {
  TRACE_EVENT0("webmediaplayerms", "texImageImpl");
  DCHECK(thread_checker_.CalledOnValidThread());

  const scoped_refptr<media::VideoFrame> video_frame =
      compositor_->GetCurrentFrameWithoutUpdatingStatistics();
  if (!video_frame || !video_frame->IsMappable() ||
      video_frame->HasTextures() ||
      video_frame->format() != media::PIXEL_FORMAT_Y16) {
    return false;
  }

  if (functionID == kTexImage2D) {
    auto* provider =
        RenderThreadImpl::current()->SharedMainThreadContextProvider().get();
    // GPU Process crashed.
    if (!provider)
      return false;
    return media::SkCanvasVideoRenderer::TexImage2D(
        target, texture, gl, provider->ContextCapabilities(), video_frame.get(),
        level, internalformat, format, type, flip_y, premultiply_alpha);
  } else if (functionID == kTexSubImage2D) {
    return media::SkCanvasVideoRenderer::TexSubImage2D(
        target, gl, video_frame.get(), level, format, type, xoffset, yoffset,
        flip_y, premultiply_alpha);
  }
  return false;
}

void WebMediaPlayerMS::OnMediaDataChange(int width, int height, int media) {
  DCHECK(thread_checker_.CalledOnValidThread());
  LOG(INFO) << "Media data changed. Width: " << width << ", height: " << height
            << ", media: " << media;
  natural_size_ = {width, height};

  if (HasVideo() && !video_weblayer_.get()) {
    video_weblayer_.reset(new cc_blink::WebLayerImpl(
        cc::VideoLayer::Create(compositor_.get(), media::VIDEO_ROTATION_0)));
    video_weblayer_->layer()->SetContentsOpaque(false);
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

void WebMediaPlayerMS::OnTimeUpdate(base::TimeDelta current_time) {
  // currentTime in mediastream must increase linearly when the stream is
  // plaing, but when changing decoder to hardware decoder it may be set to
  // invalid time if player doesn't have any media packets
  if (current_time < current_time_) {
    LOG(WARNING) << __FUNCTION__ << " Ignored time update to " << current_time
                 << ", current_time_: " << current_time_;
    return;
  }
  current_time_ = current_time;
}

void WebMediaPlayerMS::OnPlayerDestroyed() {
  SuspendInternal(false);
}

#if defined(TIZEN_VIDEO_HOLE)
void WebMediaPlayerMS::CreateVideoHoleFrame() {
  scoped_refptr<media::VideoFrame> video_frame =
      media::VideoFrame::CreateHoleFrame(natural_size_);
  if (video_frame)
    FrameReady(video_frame);
}

void WebMediaPlayerMS::FrameReady(
    const scoped_refptr<media::VideoFrame>& frame) {
  compositor_->EnqueueFrame(frame);
}

bool WebMediaPlayerMS::ShouldCreateVideoHoleFrame() const {
  if (HasVideo()) {
    DLOG(INFO) << "Video hole should be created.";
    return true;
  }
  DLOG(INFO) << "Video hole should NOT be created.";
  return false;
}

void WebMediaPlayerMS::StartLayerBoundUpdateTimer() {
  LOG(INFO) << "Layer bound update timer started";
  if (layer_bound_update_timer_.IsRunning())
    return;

  layer_bound_update_timer_.Start(
      FROM_HERE, kLayerBoundUpdateInterval, this,
      &WebMediaPlayerMS::OnLayerBoundUpdateTimerFired);
}

void WebMediaPlayerMS::StopLayerBoundUpdateTimer() {
  LOG(INFO) << "Layer bound update timer stopped";
  if (layer_bound_update_timer_.IsRunning())
    layer_bound_update_timer_.Stop();
}

void WebMediaPlayerMS::OnLayerBoundUpdateTimerFired() {
  if (UpdateBoundaryRectangle()) {
    LOG(INFO) << "Updating player geometry";
    SetMediaGeometry(GetBoundaryRectangle());
    StopLayerBoundUpdateTimer();
  }
}

gfx::RectF WebMediaPlayerMS::GetBoundaryRectangle() const {
  return last_computed_rect_;
}

bool WebMediaPlayerMS::UpdateBoundaryRectangle() {
  DCHECK(thread_checker_.CalledOnValidThread());
  if (!video_weblayer_) {
    DLOG(INFO) << "Video weblayer is not created!";
    return false;
  }

  // Compute the geometry of video frame layer.
  cc::Layer* layer = video_weblayer_->layer();
  gfx::RectF rect(gfx::SizeF(layer->bounds()));
  while (layer) {
    rect.Offset(layer->position().OffsetFromOrigin());
    rect.Offset(layer->scroll_offset().x() * (-1),
                layer->scroll_offset().y() * (-1));
    layer = layer->parent();
  }

  // Compute the real pixs if frame scaled.
  rect.Scale(frame_->View()->PageScaleFactor());

  // Return false when the geometry hasn't been changed from the last time.
  if (last_computed_rect_ == rect) {
    return false;
  }

  // Store the changed geometry information when it is actually changed.
  DLOG(INFO) << "Bound rectangle changed from "
             << last_computed_rect_.ToString() << " to " << rect.ToString();
  last_computed_rect_ = rect;
  return true;
}

void WebMediaPlayerMS::OnDrawableContentRectChanged(gfx::Rect rect,
                                                    bool /*is_video*/) {
  DCHECK(thread_checker_.CalledOnValidThread());
  SetMediaGeometry(static_cast<gfx::RectF>(rect));
}

#endif

#if defined(OS_TIZEN_TV_PRODUCT)
void WebMediaPlayerMS::OnVideoSlotsAvailableChange(unsigned slots_available) {
  LOG(INFO) << this << " :: [VSAC][WebRTC] got event: " << slots_available;

  upgrade_decoder_ = slots_available ? UpgradeDecoder_UpgradeToHardware
                                     : UpgradeDecoder_StayWithSoftware;

  // Now:
  // 1. if there are slots available...
  // 2. and the currently used decoder is software...
  // 3. and if we are actually playing...
  // 4. call the upgrade-to-hardware.

  auto decoder_type = GetTizenDecoderType();

  if (!slots_available) {
    LOG(INFO) << this << " :: [VSAC][WebRTC] Will STAY with current";
    return;
  }

  if (paused_) {
    LOG(INFO) << this << " :: [VSAC][WebRTC] Paused, will upgrade later";
    return;
  }

  // Either 2 or 3 are false, bail out
  if (decoder_type != media::TizenDecoderType::TEXTURE &&
      !is_decoder_suspending_) {
    LOG(INFO) << this << " :: [VSAC][WebRTC] Will upgrade later";
    return;
  }

  if (suspended_) {
    LOG(INFO) << this << " :: [VSAC][WebRTC] Suspended will upgrade later";
    return;
  }

  // (hopefully, this turns into tail-call)
  LOG(INFO) << this << " :: [VSAC][WebRTC] Will upgrade now";
  UpgradeDecoder_UpgradeToHardware(this);
}
#endif  // defined OS_TIZEN_TV_PRODUCT

void WebMediaPlayerMS::OnFirstFrameReceived(media::VideoRotation video_rotation,
                                            bool is_opaque) {
  DVLOG(1) << __func__;
  DCHECK(thread_checker_.CalledOnValidThread());
  UpgradeReadyState();
  OnRotationChanged(video_rotation, is_opaque);
}

void WebMediaPlayerMS::OnOpacityChanged(bool is_opaque) {
  DVLOG(1) << __func__;
  DCHECK(thread_checker_.CalledOnValidThread());

  // Opacity can be changed during the session without resetting
  // |video_weblayer_|.
  video_weblayer_->layer()->SetContentsOpaque(is_opaque);
}

void WebMediaPlayerMS::OnRotationChanged(media::VideoRotation video_rotation,
                                         bool is_opaque) {
  DVLOG(1) << __func__;
  DCHECK(thread_checker_.CalledOnValidThread());
  video_rotation_ = video_rotation;

  std::unique_ptr<cc_blink::WebLayerImpl> rotated_weblayer =
      base::WrapUnique(new cc_blink::WebLayerImpl(
          cc::VideoLayer::Create(compositor_.get(), video_rotation)));
  rotated_weblayer->layer()->SetContentsOpaque(is_opaque);
  rotated_weblayer->SetContentsOpaqueIsFixed(true);
  get_client()->SetWebLayer(rotated_weblayer.get());
  video_weblayer_ = std::move(rotated_weblayer);
}

void WebMediaPlayerMS::RepaintInternal() {
  DVLOG(1) << __func__;
  DCHECK(thread_checker_.CalledOnValidThread());
  get_client()->Repaint();
}

void WebMediaPlayerMS::OnSourceError() {
  DCHECK(thread_checker_.CalledOnValidThread());
  SetNetworkState(WebMediaPlayer::kNetworkStateFormatError);
  RepaintInternal();
}

void WebMediaPlayerMS::SetNetworkState(WebMediaPlayer::NetworkState state) {
  DCHECK(thread_checker_.CalledOnValidThread());
  network_state_ = state;
  // Always notify to ensure client has the latest value.
  get_client()->NetworkStateChanged();
}

void WebMediaPlayerMS::SetReadyStateInternal(WebMediaPlayer::ReadyState state) {
  DCHECK(thread_checker_.CalledOnValidThread());
  ready_state_ = state;
  // Always notify to ensure client has the latest value.
  get_client()->ReadyStateChanged();
}

void WebMediaPlayerMS::UpgradeReadyState() {
  if (GetReadyState() == WebMediaPlayer::kReadyStateHaveNothing) {
    SetReadyStateInternal(WebMediaPlayer::kReadyStateHaveMetadata);
    SetReadyStateInternal(WebMediaPlayer::kReadyStateHaveEnoughData);
  }
}

// readyState reporting for media capture streams is simplified
// and incompatible with usual readyState algorithm.
// We make this method empty so there is no external infuence
// on readyState
void WebMediaPlayerMS::SetReadyState(WebMediaPlayer::ReadyState state) {}

void WebMediaPlayerMS::SetDecoderType(WebMediaPlayer::DecoderType type) {
  DCHECK(thread_checker_.CalledOnValidThread());

  if (decoder_type_ == type)
    return;

  decoder_type_ = type;
  // Always notify to ensure client has the latest value.
  LOG(INFO) << "type: " << static_cast<int>(type);
  get_client()->DecoderTypeChanged();
}

void WebMediaPlayerMS::SetMediaGeometry(const gfx::RectF& rect) {
  DCHECK(thread_checker_.CalledOnValidThread());
#if defined(TIZEN_MULTIMEDIA_SUPPORT)
  if (manager_)
    manager_->SetMediaGeometry(player_id_, rect);
#endif
}

media::SkCanvasVideoRenderer* WebMediaPlayerMS::GetSkCanvasVideoRenderer() {
  return &video_renderer_;
}

void WebMediaPlayerMS::ResetCanvasCache() {
  DCHECK(thread_checker_.CalledOnValidThread());
  video_renderer_.ResetCache();
}

void WebMediaPlayerMS::TriggerResize() {
  if (HasVideo())
    get_client()->SizeChanged();

  delegate_->DidPlayerSizeChange(delegate_id_, NaturalSize());
}

media::TizenDecoderType WebMediaPlayerMS::GetTizenDecoderType() const {
  return (decoder_data_ ? decoder_data_->type
                        : media::TizenDecoderType::TEXTURE);
}

}  // namespace content
