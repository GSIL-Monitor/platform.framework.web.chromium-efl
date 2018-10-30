// Copyright 2014 Samsung Electronics Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/media/tizen/media_source_delegate_efl.h"

#include "base/callback_helpers.h"
#include "base/process/process.h"
#include "media/base/bind_to_current_loop.h"
#include "media/base/media_log.h"
#include "media/base/tizen/demuxer_stream_player_params_efl.h"
#include "media/blink/webmediaplayer_util.h"
#include "media/blink/webmediasource_impl.h"
#include "media/filters/chunk_demuxer.h"
#include "media/filters/decrypting_demuxer_stream.h"

namespace content {

MediaSourceDelegateEfl::MediaSourceDelegateEfl(
    RendererDemuxerEfl* demuxer_client,
    int demuxer_client_id,
    const scoped_refptr<base::SingleThreadTaskRunner>& media_task_runner,
    media::MediaLog* media_log)
    : main_loop_(base::ThreadTaskRunnerHandle::Get()),
      main_weak_factory_(this),
      main_weak_this_(main_weak_factory_.GetWeakPtr()),
      media_task_runner_(media_task_runner),
      media_weak_factory_(this),
      demuxer_client_(demuxer_client),
      demuxer_client_id_(demuxer_client_id),
      media_log_(media_log),
      audio_stream_(nullptr),
      video_stream_(nullptr),
      cdm_context_(nullptr),
      seek_time_(media::kNoTimestamp),
      pending_seek_(false),
      is_seeking_(false),
      seeking_pending_seek_(false),
      is_demuxer_seek_done_(false),
      pending_seek_time_(media::kNoTimestamp),
      is_audio_read_fired_(false),
      is_video_read_fired_(false),
      is_demuxer_ready_(false),
      video_key_frame_(media::kNoTimestamp) {
  DCHECK(!chunk_demuxer_);
}

MediaSourceDelegateEfl::~MediaSourceDelegateEfl() {
  DCHECK(main_loop_->BelongsToCurrentThread());
  DCHECK(!chunk_demuxer_);
  DCHECK(!audio_stream_);
  DCHECK(!video_stream_);
  DCHECK(!audio_decrypting_demuxer_stream_);
  DCHECK(!video_decrypting_demuxer_stream_);
}

void MediaSourceDelegateEfl::InitializeMediaSource(
    const MediaSourceOpenedCB& media_source_opened_cb,
    const media::Demuxer::EncryptedMediaInitDataCB& emedia_init_data_cb,
    const SetCdmReadyCB& set_cdm_ready_cb,
    const UpdateNetworkStateCB& update_network_state_cb,
    const DurationChangeCB& duration_change_cb,
    const base::Closure& waiting_for_decryption_key_cb) {
  DCHECK(main_loop_->BelongsToCurrentThread());
  DCHECK(!media_source_opened_cb.is_null());
  DCHECK(!emedia_init_data_cb.is_null());
  DCHECK(!set_cdm_ready_cb.is_null());
  DCHECK(!update_network_state_cb.is_null());
  DCHECK(!duration_change_cb.is_null());
  DCHECK(!waiting_for_decryption_key_cb.is_null());
  media_source_opened_cb_ = media_source_opened_cb;
  emedia_init_data_cb_ = emedia_init_data_cb;
  set_cdm_ready_cb_ = media::BindToCurrentLoop(set_cdm_ready_cb);
  update_network_state_cb_ = media::BindToCurrentLoop(update_network_state_cb);
  duration_change_cb_ = duration_change_cb;
  waiting_for_decryption_key_cb_ =
      media::BindToCurrentLoop(waiting_for_decryption_key_cb);
  chunk_demuxer_.reset(new media::ChunkDemuxer(
      media::BindToCurrentLoop(base::Bind(
          &MediaSourceDelegateEfl::OnDemuxerOpened, main_weak_this_)),
      media::BindToCurrentLoop(base::Bind(
          &MediaSourceDelegateEfl::OnDemuxerProgress, main_weak_this_)),
      media::BindToCurrentLoop(base::Bind(
          &MediaSourceDelegateEfl::OnEncryptedMediaInitData, main_weak_this_)),
      media_log_));
  media_task_runner_->PostTask(
      FROM_HERE, base::Bind(&MediaSourceDelegateEfl::InitializeDemuxer,
                            base::Unretained(this)));
}

void MediaSourceDelegateEfl::OnBufferedTimeRangesChanged(
    const media::Ranges<base::TimeDelta>& ranges) {
  buffered_time_ranges_ = ranges;
  demuxer_client_->DemuxerBufferedChanged(demuxer_client_id_,
                                          buffered_time_ranges_);
}

blink::WebTimeRanges MediaSourceDelegateEfl::Buffered() const {
  return media::ConvertToWebTimeRanges(buffered_time_ranges_);
}

void MediaSourceDelegateEfl::InitializeDemuxer() {
  DCHECK(media_task_runner_->BelongsToCurrentThread());
  demuxer_client_->AddDelegate(demuxer_client_id_, this);
  chunk_demuxer_->Initialize(
      this,
      base::Bind(&MediaSourceDelegateEfl::OnDemuxerInitDone,
                 media_weak_factory_.GetWeakPtr()),
      false);
#if defined(OS_TIZEN_TV_PRODUCT)
  chunk_demuxer_->SetFramerateCallback(
      base::Bind(&MediaSourceDelegateEfl::OnFramerateSet,
                 media_weak_factory_.GetWeakPtr()));

#endif
}

void MediaSourceDelegateEfl::OnEncryptedMediaInitData(
    media::EmeInitDataType init_data_type,
    const std::vector<uint8_t>& init_data) {
  DCHECK(main_loop_->BelongsToCurrentThread());
  if (emedia_init_data_cb_.is_null()) {
    return;
  }
  emedia_init_data_cb_.Run(init_data_type, init_data);
}

void MediaSourceDelegateEfl::OnDemuxerProgress() {
  // NOTIMPLEMENTED() << "EWK_BRINGUP: handle ready state";
}

void MediaSourceDelegateEfl::OnDemuxerOpened() {
  DCHECK(main_loop_->BelongsToCurrentThread());
  if (media_source_opened_cb_.is_null())
    return;
  media_source_opened_cb_.Run(
      new media::WebMediaSourceImpl(chunk_demuxer_.get()));
}

void MediaSourceDelegateEfl::OnDemuxerError(media::PipelineStatus status) {
  if (status != media::PIPELINE_OK && !update_network_state_cb_.is_null())
    update_network_state_cb_.Run(PipelineErrorToNetworkState(status));
}

void MediaSourceDelegateEfl::AddTextStream(
    media::DemuxerStream* /* text_stream */,
    const media::TextTrackConfig& /* config */) {
  NOTIMPLEMENTED();
}

void MediaSourceDelegateEfl::RemoveTextStream(
    media::DemuxerStream* /* text_stream */) {
  NOTIMPLEMENTED();
}

bool MediaSourceDelegateEfl::CanNotifyDemuxerReady() {
  DCHECK(media_task_runner_->BelongsToCurrentThread());
  return is_demuxer_ready_;
}

#if defined(OS_TIZEN_TV_PRODUCT)
void MediaSourceDelegateEfl::OnFramerateSet(
    const media::StreamFramerate::Framerate& framerate) {
  if (framerate.toDouble() > 0) {
    if (IsFramerateChanged(framerate)) {
      framerate_ = framerate;
      NotifyDemuxerStateChanged();
    }
  }
}

bool MediaSourceDelegateEfl::IsFramerateChanged(
    const media::StreamFramerate::Framerate& framerate) {
  /* SOC framerate change detection & decision table
    Conditions priorities      Detected framerate            Conversion value
    1st
                           23900 <= framerate < 24000             23976
                           24000 <= framerate < 24100             24000
                           24900 <= framerate < 25100             25000
                           29900 <= framerate < 30000             29970
                           30000 <= framerate < 30100             30000
                           49900 <= framerate < 50100             50000
                           59900 <= framerate < 60000             59940
                           60000 <= framerate < 60100             60000
    2nd
                                 framerate < 30000                30000
                                 framerate < 60000                60000
                                   Other case                   Not Support
  */

  constexpr const double kDetectedThreshold = 0.01;
  if (std::abs(framerate_.toDouble() - framerate.toDouble()) >
      kDetectedThreshold) {
    const double kFramerateDetectionTable[] = {0.0,  23.9, 24, 24.1, 24.9,
                                               25.1, 29.9, 30, 30.1, 49.9,
                                               50.1, 59.9, 60, 60.1};

    int detect_index = 0;
    int previous_frame_detect_index = 0;
    int new_frame_detect_index = 0;
    const int len =
        sizeof(kFramerateDetectionTable) / sizeof(kFramerateDetectionTable[0]);

    for (detect_index = 1; detect_index < len - 1; ++detect_index) {
      if (!new_frame_detect_index &&
          framerate.toDouble() >= kFramerateDetectionTable[detect_index] &&
          framerate.toDouble() < kFramerateDetectionTable[detect_index + 1])
        new_frame_detect_index = detect_index;
      if (!previous_frame_detect_index &&
          framerate_.toDouble() >= kFramerateDetectionTable[detect_index] &&
          framerate_.toDouble() < kFramerateDetectionTable[detect_index + 1])
        previous_frame_detect_index = detect_index;

      if (new_frame_detect_index && previous_frame_detect_index)
        break;
    }
    // Framerate is changed.
    if (new_frame_detect_index != previous_frame_detect_index) {
      LOG(INFO) << "Framerate is changed";
      return true;
    }
  }

  return false;
}
#endif

void MediaSourceDelegateEfl::OnDemuxerInitDone(media::PipelineStatus status) {
  DCHECK(media_task_runner_->BelongsToCurrentThread());
  DCHECK(chunk_demuxer_);
  if (status != media::PIPELINE_OK) {
    OnDemuxerError(status);
    return;
  }
  media_task_runner_->PostTask(
      FROM_HERE, base::Bind(&MediaSourceDelegateEfl::GetDemuxerStreamInfo,
                            base::Unretained(this)));
}

void MediaSourceDelegateEfl::GetDemuxerStreamInfo() {
  DCHECK(media_task_runner_->BelongsToCurrentThread());
  DCHECK(chunk_demuxer_);

  audio_stream_ = GetStreamByType(media::DemuxerStream::AUDIO);
  video_stream_ = GetStreamByType(media::DemuxerStream::VIDEO);
  DCHECK(audio_stream_ || video_stream_);

  if (HasEncryptedStream()) {
    set_cdm_ready_cb_.Run(BindToCurrentLoop(base::Bind(
        &MediaSourceDelegateEfl::SetCdm, media_weak_factory_.GetWeakPtr())));
    return;
  }

  // Notify demuxer ready when both streams are not encrypted.
  NotifyDemuxerReady(false);
}

bool MediaSourceDelegateEfl::HasEncryptedStream() {
  DCHECK(media_task_runner_->BelongsToCurrentThread());
  DCHECK(audio_stream_ || video_stream_);

  return (audio_stream_ &&
          audio_stream_->audio_decoder_config().is_encrypted()) ||
         (video_stream_ &&
          video_stream_->video_decoder_config().is_encrypted());
}

void MediaSourceDelegateEfl::SetCdm(
    media::CdmContext* cdm_context,
    const media::CdmAttachedCB& cdm_attached_cb) {
  DCHECK(media_task_runner_->BelongsToCurrentThread());
  DCHECK(cdm_context);
  DCHECK(!cdm_attached_cb.is_null());
  DCHECK(!is_demuxer_ready_);
  DCHECK(HasEncryptedStream());

  cdm_context_ = cdm_context;
  pending_cdm_attached_cb_ = cdm_attached_cb;

  if (audio_stream_ && audio_stream_->audio_decoder_config().is_encrypted()) {
    InitAudioDecryptingDemuxerStream();
    return;
  }
  if (video_stream_ && video_stream_->video_decoder_config().is_encrypted()) {
    InitVideoDecryptingDemuxerStream();
    return;
  }
  NOTREACHED() << "No encrytped stream.";
}

void MediaSourceDelegateEfl::InitAudioDecryptingDemuxerStream() {
  DCHECK(media_task_runner_->BelongsToCurrentThread());
  DVLOG(1) << __FUNCTION__ << " : " << demuxer_client_id_;
  DCHECK(cdm_context_);

  audio_decrypting_demuxer_stream_.reset(new media::DecryptingDemuxerStream(
      media_task_runner_, media_log_, waiting_for_decryption_key_cb_));

  audio_decrypting_demuxer_stream_->Initialize(
      audio_stream_, cdm_context_,
      base::Bind(
          &MediaSourceDelegateEfl::OnAudioDecryptingDemuxerStreamInitDone,
          media_weak_factory_.GetWeakPtr()));
}

void MediaSourceDelegateEfl::InitVideoDecryptingDemuxerStream() {
  DCHECK(media_task_runner_->BelongsToCurrentThread());
  DVLOG(1) << __FUNCTION__ << " : " << demuxer_client_id_;
  DCHECK(cdm_context_);

  video_decrypting_demuxer_stream_.reset(new media::DecryptingDemuxerStream(
      media_task_runner_, media_log_, waiting_for_decryption_key_cb_));

  video_decrypting_demuxer_stream_->Initialize(
      video_stream_, cdm_context_,
      base::Bind(
          &MediaSourceDelegateEfl::OnVideoDecryptingDemuxerStreamInitDone,
          media_weak_factory_.GetWeakPtr()));
}

void MediaSourceDelegateEfl::OnAudioDecryptingDemuxerStreamInitDone(
    media::PipelineStatus status) {
  DCHECK(media_task_runner_->BelongsToCurrentThread());
  DCHECK(chunk_demuxer_);

  is_audio_read_fired_ = false;
  if (status != media::PIPELINE_OK) {
    audio_decrypting_demuxer_stream_.reset();
    // Different CDMs are supported differently. For CDMs that support a
    // Decryptor, we'll try to use DecryptingDemuxerStream in the render side.
    // Otherwise, we'll try to use the CDMs in the browser side. Therefore, if
    // DecryptingDemuxerStream initialization failed, it's still possible that
    // we can handle the audio with a CDM in the browser. Declare demuxer ready
    // now to try that path. Note there's no need to try DecryptingDemuxerStream
    // for video here since it is impossible to handle audio in the browser and
    // handle video in the render process.
    NotifyDemuxerReady(false);
    return;
  }
  audio_stream_ = audio_decrypting_demuxer_stream_.get();

  if (video_stream_ && video_stream_->video_decoder_config().is_encrypted()) {
    InitVideoDecryptingDemuxerStream();
    return;
  }

  // Try to notify demuxer ready when audio DDS initialization finished and
  // video is not encrypted.
  NotifyDemuxerReady(true);
}

void MediaSourceDelegateEfl::OnVideoDecryptingDemuxerStreamInitDone(
    media::PipelineStatus status) {
  DCHECK(media_task_runner_->BelongsToCurrentThread());
  DCHECK(chunk_demuxer_);

  bool success = status == media::PIPELINE_OK;
  is_video_read_fired_ = false;
  if (!success)
    video_decrypting_demuxer_stream_.reset();
  else
    video_stream_ = video_decrypting_demuxer_stream_.get();

  // Try to notify demuxer ready when video DDS initialization finished.
  NotifyDemuxerReady(success);
}

#if defined(OS_TIZEN_TV_PRODUCT)
void MediaSourceDelegateEfl::NotifyDemuxerStateChanged() {
  DCHECK(media_task_runner_->BelongsToCurrentThread());
  if (!CanNotifyDemuxerReady()) {
    LOG(INFO) << "Cannot notify about demuxer configs yet. Either the demuxer "
                 "is not ready or did not yet receive all configs.";
    return;
  }

  if (!demuxer_client_) {
    OnDemuxerError(media::PIPELINE_ERROR_INITIALIZATION_FAILED);
    return;
  }

  media::DemuxerConfigs configs;
  if (video_stream_) {
    media::VideoDecoderConfig video_config =
        video_stream_->video_decoder_config();
    // Reprot part of video config info needed by DemuxerStateChanged.
    configs.video_codec = video_config.codec();
    configs.video_size = video_config.natural_size();

    configs.framerate_num = framerate_.num;
    configs.framerate_den = framerate_.den;
    configs.is_framerate_changed = true;
  }
  demuxer_client_->DemuxerStateChanged(demuxer_client_id_, configs);
}
#endif

void MediaSourceDelegateEfl::NotifyDemuxerReady(bool is_cdm_attached) {
  DCHECK(media_task_runner_->BelongsToCurrentThread());
  DVLOG(1) << __FUNCTION__ << " : " << demuxer_client_id_
           << ", is_cdm_attached: " << is_cdm_attached;
  DCHECK(is_demuxer_ready_);

  is_demuxer_ready_ = true;

  if (!pending_cdm_attached_cb_.is_null())
    main_loop_->PostTask(
        FROM_HERE, base::Bind(base::ResetAndReturn(&pending_cdm_attached_cb_),
                              is_cdm_attached));

  if (!demuxer_client_ || (!audio_stream_ && !video_stream_)) {
    OnDemuxerError(media::PIPELINE_ERROR_INITIALIZATION_FAILED);
    return;
  }
  std::unique_ptr<media::DemuxerConfigs> configs(new media::DemuxerConfigs());
  if (audio_stream_) {
    media::AudioDecoderConfig audio_config =
        audio_stream_->audio_decoder_config();
    configs->audio_codec = audio_config.codec();
    configs->audio_channels =
        media::ChannelLayoutToChannelCount(audio_config.channel_layout());
    configs->audio_sampling_rate = audio_config.samples_per_second();
    configs->is_audio_encrypted = GetStreamByType(media::DemuxerStream::AUDIO)
                                      ->audio_decoder_config()
                                      .is_encrypted();
    configs->audio_extra_data = audio_config.extra_data();
    configs->audio_bit_rate = audio_config.bytes_per_channel() *
                              audio_config.samples_per_second() * 8;
  }
  if (video_stream_) {
    media::VideoDecoderConfig video_config =
        video_stream_->video_decoder_config();
    configs->video_codec = video_config.codec();
    configs->video_size = video_config.natural_size();
    configs->is_video_encrypted = GetStreamByType(media::DemuxerStream::VIDEO)
                                      ->video_decoder_config()
                                      .is_encrypted();
    configs->video_extra_data = video_config.extra_data();
#if defined(OS_TIZEN_TV_PRODUCT)
    configs->webm_hdr_info = video_config.GetWebMHdrInfo();
    if (framerate_.num && framerate_.den) {
      configs->framerate_num = framerate_.num;
      configs->framerate_den = framerate_.den;
    } else {
      configs->framerate_num = 0;
      configs->framerate_den = 0;
    }
#endif
  }
  demuxer_client_->DemuxerReady(demuxer_client_id_, *configs);
}

void MediaSourceDelegateEfl::OnReadFromDemuxer(
    media::DemuxerStream::Type type) {
  DCHECK(media_task_runner_->BelongsToCurrentThread());

  if (is_seeking_)
    return;

  if ((type == media::DemuxerStream::AUDIO) && audio_stream_ &&
      !is_audio_read_fired_) {
    is_audio_read_fired_ = true;
    audio_stream_->Read(base::Bind(&MediaSourceDelegateEfl::OnBufferReady,
                                   media_weak_factory_.GetWeakPtr(), type));
  }

  if ((type == media::DemuxerStream::VIDEO) && video_stream_ &&
      !is_video_read_fired_) {
    is_video_read_fired_ = true;
    video_stream_->Read(base::Bind(&MediaSourceDelegateEfl::OnBufferReady,
                                   media_weak_factory_.GetWeakPtr(), type));
  }
}

void MediaSourceDelegateEfl::Stop(const base::Closure& stop_cb) {
  DCHECK(main_loop_->BelongsToCurrentThread());

  if (!chunk_demuxer_) {
    DCHECK(!demuxer_client_);
    return;
  }

  duration_change_cb_.Reset();
  update_network_state_cb_.Reset();
  media_source_opened_cb_.Reset();

  main_weak_factory_.InvalidateWeakPtrs();
  DCHECK(!main_weak_factory_.HasWeakPtrs());

  // 1. shutdown demuxer.
  // 2. On media thread, call stop demuxer.
  chunk_demuxer_->Shutdown();
  media_task_runner_->PostTask(FROM_HERE,
                               base::Bind(&MediaSourceDelegateEfl::StopDemuxer,
                                          base::Unretained(this), stop_cb));
}

void MediaSourceDelegateEfl::StopDemuxer(const base::Closure& stop_cb) {
  DCHECK(media_task_runner_->BelongsToCurrentThread());
  DCHECK(chunk_demuxer_);

  demuxer_client_->RemoveDelegate(demuxer_client_id_);
  demuxer_client_ = NULL;
  audio_stream_ = NULL;
  video_stream_ = NULL;
  audio_decrypting_demuxer_stream_.reset();
  video_decrypting_demuxer_stream_.reset();

  media_weak_factory_.InvalidateWeakPtrs();
  DCHECK(!media_weak_factory_.HasWeakPtrs());

  chunk_demuxer_->Stop();
  chunk_demuxer_.reset();

  stop_cb.Run();
}

void MediaSourceDelegateEfl::SeekInternal(const base::TimeDelta& seek_time) {
  DCHECK(media_task_runner_->BelongsToCurrentThread());

  chunk_demuxer_->AbortPendingReads();
  chunk_demuxer_->Seek(
      seek_time, media::BindToCurrentLoop(
                     base::Bind(&MediaSourceDelegateEfl::OnDemuxerSeekDone,
                                media_weak_factory_.GetWeakPtr())));
}

void MediaSourceDelegateEfl::OnBufferReady(
    media::DemuxerStream::Type type,
    media::DemuxerStream::Status status,
    const scoped_refptr<media::DecoderBuffer>& buffer) {
  uint32_t shared_memory_size = -1;
  base::SharedMemory shared_memory;
  base::SharedMemoryHandle foreign_memory_handle;
  std::unique_ptr<media::DemuxedBufferMetaData> meta_data(
      new media::DemuxedBufferMetaData());
  meta_data->status = status;
  meta_data->type = type;

  if (type == media::DemuxerStream::AUDIO)
    is_audio_read_fired_ = false;
  if (type == media::DemuxerStream::VIDEO)
    is_video_read_fired_ = false;

  switch (status) {
    case media::DemuxerStream::kAborted:
      LOG(ERROR) << "[RENDER] : DemuxerStream::kAborted type:" << type;
      return;

    case media::DemuxerStream::kConfigChanged:
      // When switching between clean and encrypted content decrypting
      // demuxer stream has to  be initialized. It will be done only once
      // per stream type (audio/video) because is_encrypted() method of
      // decrypting_demuxer_stream config's always returns false. Decrypting
      // demuxer stream can handle clean streams, so there's no need to
      // switch it back when clean content appears.
      if (HasEncryptedStream()) {
        if (type == media::DemuxerStream::AUDIO)
          is_audio_read_fired_ = true;
        else if (type == media::DemuxerStream::VIDEO)
          is_video_read_fired_ = true;
        set_cdm_ready_cb_.Run(
            BindToCurrentLoop(base::Bind(&MediaSourceDelegateEfl::SetCdm,
                                         media_weak_factory_.GetWeakPtr())));
      } else {
        NotifyDemuxerReady(false);
      }
      return;

    case media::DemuxerStream::kOk:
      if (buffer.get()->end_of_stream()) {
        meta_data->end_of_stream = true;
        break;
      }
#if defined(OS_TIZEN_TV_PRODUCT)
      if (buffer->is_tz_handle_valid()) {
        auto tz_handle = buffer->pass_tz_handle();
        meta_data->tz_handle = tz_handle.handle();
        meta_data->size = tz_handle.size();
      } else
#endif
      {
        shared_memory_size = buffer.get()->data_size();
        if (!shared_memory.CreateAndMapAnonymous(shared_memory_size)) {
          LOG(ERROR) << "Shared Memory creation failed.";
          return;
        }

        foreign_memory_handle = shared_memory.handle().Duplicate();
        if (!foreign_memory_handle.IsValid()) {
          LOG(ERROR) << "Shared Memory handle could not be obtained";
          return;
        }

        memcpy(shared_memory.memory(), (void*)buffer.get()->writable_data(),
               shared_memory_size);
        meta_data->size = shared_memory_size;
      }
      meta_data->timestamp = buffer.get()->timestamp();
      meta_data->time_duration = buffer.get()->duration();
      break;
    default:
      NOTREACHED();
  }

  if (!demuxer_client_ ||
      !demuxer_client_->ReadFromDemuxerAck(demuxer_client_id_,
                                           foreign_memory_handle, *meta_data)) {
    LOG(ERROR) << "demuxer client is null or ReadFromDemuxerAck failed";
    if (base::SharedMemory::IsHandleValid(foreign_memory_handle))
      base::SharedMemory::CloseHandle(foreign_memory_handle);
#if defined(OS_TIZEN_TV_PRODUCT)
    meta_data->Release();
#endif
  }
}

#if defined(OS_TIZEN_TV_PRODUCT)
media::RelevantSeekValues MediaSourceDelegateEfl::GetRelevantSeekValues(
    const base::TimeDelta timestamp) {
  return chunk_demuxer_->GetRelevantSeekValues(timestamp);
}
#endif

void MediaSourceDelegateEfl::StartWaitingForSeek(
    const base::TimeDelta& seek_time) {
  DCHECK(main_loop_->BelongsToCurrentThread());

  if (!chunk_demuxer_)
    return;

  {
    base::AutoLock auto_lock(seeking_lock_);
    // Called from |webmediaplayerefl| only.
    is_demuxer_seek_done_ = false;
    seeking_pending_seek_ = false;
    is_seeking_ = true;
  }
  chunk_demuxer_->StartWaitingForSeek(seek_time);
}

void MediaSourceDelegateEfl::CancelPendingSeek(
    const base::TimeDelta& seek_time) {
  DCHECK(main_loop_->BelongsToCurrentThread());
  if (!chunk_demuxer_)
    return;

  bool cancel_browser_seek = true;
  {
    base::AutoLock auto_lock(seeking_lock_);
    is_seeking_ = true;
    pending_seek_ = true;
    pending_seek_time_ = seek_time;
    if (is_demuxer_seek_done_) {
      // Since we already requested gstreamer to seek. And there are no pending
      // seeks in |chunk_demuxer|. Cancelling pending seek makes no sense.
      //
      // This block will handle when |gstreamer| is seeking and new seek came in
      // between.
      is_demuxer_seek_done_ = false;
      pending_seek_ = false;
      cancel_browser_seek = false;
    }
  }

  if (cancel_browser_seek)
    chunk_demuxer_->CancelPendingSeek(seek_time);
  else {
    chunk_demuxer_->StartWaitingForSeek(seek_time);
    media_task_runner_->PostTask(
        FROM_HERE, base::Bind(&MediaSourceDelegateEfl::StartSeek,
                              base::Unretained(this), seek_time, true));
  }
}

void MediaSourceDelegateEfl::StartSeek(const base::TimeDelta& seek_time,
                                       bool is_seeking_pending_seek) {
  DCHECK(media_task_runner_->BelongsToCurrentThread());

  if (!chunk_demuxer_)
    return;

  base::TimeDelta true_seek_time = seek_time;
  {
    base::AutoLock auto_lock(seeking_lock_);
    is_seeking_ = true;
    is_demuxer_seek_done_ = false;
    if (is_seeking_pending_seek)
      seeking_pending_seek_ = is_seeking_pending_seek;
    else if (seeking_pending_seek_)
      return;

#if defined(OS_TIZEN_TV_PRODUCT)
    media::RelevantSeekValues relevant_seek_values =
        chunk_demuxer_->GetRelevantSeekValues(seek_time);
    true_seek_time = std::get<0>(relevant_seek_values);
    video_key_frame_ = std::get<1>(relevant_seek_values);
#endif

    seek_time_ = true_seek_time;
  }

  SeekInternal(true_seek_time);
}

void MediaSourceDelegateEfl::OnDemuxerSeekDone(
    base::TimeDelta demuxer_seek_time,
    media::PipelineStatus status) {
  DCHECK(media_task_runner_->BelongsToCurrentThread());
  if (status != media::PIPELINE_OK) {
    OnDemuxerError(status);
    return;
  }

#if defined(OS_TIZEN_TV_PRODUCT)
  seek_time_ = demuxer_seek_time;
  media::RelevantSeekValues relevant_seek_values =
      chunk_demuxer_->GetRelevantSeekValues(seek_time_);
  const base::TimeDelta true_seek_time = std::get<0>(relevant_seek_values);
  video_key_frame_ = std::get<1>(relevant_seek_values);

  if (seek_time_ != true_seek_time) {
    StartSeek(true_seek_time, true);
    return;
  }
#endif

  bool pending_seek = false;
  {
    base::AutoLock auto_lock(seeking_lock_);
    if (pending_seek_) {
      pending_seek_ = false;
      seek_time_ = pending_seek_time_;
      pending_seek = true;
    } else {
      seeking_pending_seek_ = false;
      is_seeking_ = false;
      is_demuxer_seek_done_ = true;
    }
  }
  if (pending_seek)
    StartSeek(pending_seek_time_, true);
  else
    ResetAudioDecryptingDemuxerStream();
}

void MediaSourceDelegateEfl::ResetAudioDecryptingDemuxerStream() {
  DCHECK(media_task_runner_->BelongsToCurrentThread());
  if (audio_decrypting_demuxer_stream_) {
    audio_decrypting_demuxer_stream_->Reset(
        base::Bind(&MediaSourceDelegateEfl::ResetVideoDecryptingDemuxerStream,
                   media_weak_factory_.GetWeakPtr()));
    return;
  }
  ResetVideoDecryptingDemuxerStream();
}

void MediaSourceDelegateEfl::ResetVideoDecryptingDemuxerStream() {
  DCHECK(media_task_runner_->BelongsToCurrentThread());
  if (video_decrypting_demuxer_stream_) {
    video_decrypting_demuxer_stream_->Reset(base::Bind(
        &MediaSourceDelegateEfl::FinishResettingDecryptingDemuxerStreams,
        media_weak_factory_.GetWeakPtr()));
    return;
  }
  FinishResettingDecryptingDemuxerStreams();
}

void MediaSourceDelegateEfl::FinishResettingDecryptingDemuxerStreams() {
  DCHECK(media_task_runner_->BelongsToCurrentThread());
  DCHECK(is_seeking_);
  is_seeking_ = false;
  demuxer_client_->DemuxerSeekDone(demuxer_client_id_, seek_time_,
                                   video_key_frame_);
}

media::DemuxerStream* MediaSourceDelegateEfl::GetStreamByType(
    media::DemuxerStream::Type type) {
  return chunk_demuxer_->GetFirstStream(type);
}

void MediaSourceDelegateEfl::SetDuration(base::TimeDelta duration) {
  DCHECK(main_loop_->BelongsToCurrentThread());
  main_loop_->PostTask(FROM_HERE,
                       base::Bind(&MediaSourceDelegateEfl::OnDurationChanged,
                                  main_weak_this_, duration));
}

void MediaSourceDelegateEfl::OnDurationChanged(
    const base::TimeDelta& duration) {
  DCHECK(main_loop_->BelongsToCurrentThread());
  if (demuxer_client_)
    demuxer_client_->DurationChanged(demuxer_client_id_, duration);

  if (!duration_change_cb_.is_null())
    duration_change_cb_.Run(duration);
}

}  // namespace content
