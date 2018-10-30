// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/pepper/media_player/tizen/pepper_es_data_source_private_tizen.h"

#include "base/logging.h"
#include "base/message_loop/message_loop.h"
#include "content/browser/renderer_host/pepper/media_player/tizen/pepper_audio_elementary_stream_private_tizen.h"
#include "content/browser/renderer_host/pepper/media_player/tizen/pepper_video_elementary_stream_private_tizen.h"

namespace content {

namespace {

// We should try to set EOS for all streams independently and report error
// on the end.
template <typename ElementaryStream>
int32_t SetEndOfStreamHelper(ElementaryStream* stream, int32_t previous_error) {
  int32_t pp_error = previous_error;
  if (stream && stream->IsValid()) {
    int32_t result = stream->SetEndOfStream();
    if (result != PP_OK)
      pp_error = result;
  }
  return pp_error;
}

}  // namespace

// static
scoped_refptr<PepperESDataSourcePrivateTizen>
PepperESDataSourcePrivateTizen::Create() {
  return scoped_refptr<PepperESDataSourcePrivateTizen>{
      new PepperESDataSourcePrivateTizen};
}

PepperESDataSourcePrivateTizen::PepperESDataSourcePrivateTizen()
    : duration_(0) {}

PepperESDataSourcePrivateTizen::~PepperESDataSourcePrivateTizen() {
  if (audio_stream_)
    audio_stream_->ClearOwner();
  if (video_stream_)
    video_stream_->ClearOwner();
}

PepperMediaDataSourcePrivate::PlatformContext&
PepperESDataSourcePrivateTizen::GetContext() {
  return platform_context_;
}

void PepperESDataSourcePrivateTizen::AddAudioStream(
    const base::Callback<
        void(int32_t, scoped_refptr<PepperAudioElementaryStreamPrivate>)>&
        callback) {
  if (!audio_stream_)
    audio_stream_ = PepperAudioElementaryStreamPrivateTizen::Create(this);

  int32_t ret = (audio_stream_ ? PP_OK : PP_ERROR_NOMEMORY);

  base::MessageLoop::current()->task_runner()->PostTask(
      FROM_HERE, base::Bind(callback, ret, audio_stream_));
}

void PepperESDataSourcePrivateTizen::AddVideoStream(
    const base::Callback<
        void(int32_t, scoped_refptr<PepperVideoElementaryStreamPrivate>)>&
        callback) {
  if (!video_stream_)
    video_stream_ = PepperVideoElementaryStreamPrivateTizen::Create(this);

  int32_t ret = (video_stream_ ? PP_OK : PP_ERROR_NOMEMORY);
  base::MessageLoop::current()->task_runner()->PostTask(
      FROM_HERE, base::Bind(callback, ret, video_stream_));
}

void PepperESDataSourcePrivateTizen::SetDuration(
    PP_TimeDelta duration,
    const base::Callback<void(int32_t)>& callback) {
  duration_ = duration;
  if (platform_context_.dispatcher()) {
    PepperTizenPlayerDispatcher::DispatchOperation(
        platform_context_.dispatcher(), FROM_HERE,
        base::Bind(&PepperESDataSourcePrivateTizen::SetDurationOnPlayerThread,
                   this, duration),
        callback);
  } else {
    // Just cache duration. It will be set upon SourceAttached.
    base::MessageLoop::current()->task_runner()->PostTask(
        FROM_HERE, base::Bind(callback, static_cast<int32_t>(PP_OK)));
  }
}

int32_t PepperESDataSourcePrivateTizen::SetDurationOnPlayerThread(
    PP_TimeDelta duration,
    PepperPlayerAdapterInterface* player) {
  return player->SetDuration(duration);
}

void PepperESDataSourcePrivateTizen::SourceAttached(
    const base::Callback<void(int32_t)>& callback) {
  DCHECK(platform_context_.dispatcher() != nullptr);

  int32_t pp_error = ConfigureDrmSessions();
  if (pp_error == PP_OK) {
    if (duration_ != 0.) {
      // If duration was set before attach, apply cached value. Please note
      // that some backend players may require to apply duration before attach,
      // so dispatch SetDuration before Attach.
      PepperTizenPlayerDispatcher::DispatchOperation(
          platform_context_.dispatcher(), FROM_HERE,
          base::Bind(&PepperESDataSourcePrivateTizen::SetDurationOnPlayerThread,
                     this, duration_),
          base::Bind([](int32_t) {}));
    }
    PepperTizenPlayerDispatcher::DispatchOperation(
        platform_context_.dispatcher(), FROM_HERE,
        base::Bind(&PepperESDataSourcePrivateTizen::AttachOnPlayerThread, this),
        callback);
  } else {
    ClearDrmSessions();
    base::MessageLoop::current()->task_runner()->PostTask(
        FROM_HERE, base::Bind(callback, static_cast<int32_t>(pp_error)));
  }
}

void PepperESDataSourcePrivateTizen::SourceDetached(
    PlatformDetachPolicy cleanup) {
  DetachListeners();

  if (cleanup == PlatformDetachPolicy::kDetachFromPlayer) {
    DCHECK(platform_context_.dispatcher());
    PepperTizenPlayerDispatcher::DispatchOperation(
        platform_context_.dispatcher(), FROM_HERE,
        base::Bind(&PepperESDataSourcePrivateTizen::DetachOnPlayerThread,
                   this));
  }
}

void PepperESDataSourcePrivateTizen::GetDuration(
    const base::Callback<void(int32_t, PP_TimeDelta)>& callback) {
  base::MessageLoop::current()->task_runner()->PostTask(
      FROM_HERE, base::Bind(callback, static_cast<int32_t>(PP_OK), duration_));
}

void PepperESDataSourcePrivateTizen::SetEndOfStream(
    const base::Callback<void(int32_t)>& callback) {
  if (platform_context_.dispatcher()) {
    PepperTizenPlayerDispatcher::DispatchOperation(
        platform_context_.dispatcher(), FROM_HERE,
        base::Bind(
            &PepperESDataSourcePrivateTizen::SetEndOfStreamOnPlayerThread,
            this),
        callback);
  } else {
    base::MessageLoop::current()->task_runner()->PostTask(
        FROM_HERE, base::Bind(callback, static_cast<int32_t>(PP_ERROR_FAILED)));
  }
}

int32_t PepperESDataSourcePrivateTizen::AttachOnPlayerThread(
    PepperPlayerAdapterInterface* player) {
  bool is_audio_valid = audio_stream_ ? audio_stream_->IsValid() : false;
  bool is_video_valid = video_stream_ ? video_stream_->IsValid() : false;
  int32_t pp_error = PP_OK;
  if (!is_audio_valid && !is_video_valid) {
    LOG(ERROR) << "Neither audio nor video codec configuration is set";
    pp_error = PP_ERROR_FAILED;
  }

  if (pp_error == PP_OK && is_video_valid)
    pp_error = video_stream_->SourceAttached(player);
  if (pp_error == PP_OK && is_audio_valid)
    pp_error = audio_stream_->SourceAttached(player);

  if (pp_error == PP_OK) {
    if (!callback_runner_)
      callback_runner_ = base::MessageLoop::current()->task_runner();

    pp_error = player->PrepareES();
  }

  return pp_error;
}

void PepperESDataSourcePrivateTizen::DetachOnPlayerThread(
    PepperPlayerAdapterInterface* player) {
  player->Unprepare();
}

int32_t PepperESDataSourcePrivateTizen::SetEndOfStreamOnPlayerThread(
    PepperPlayerAdapterInterface*) {
  // Returns PP_OK if no streams are attached.
  int32_t pp_error = PP_OK;

  pp_error = SetEndOfStreamHelper(audio_stream_.get(), pp_error);
  pp_error = SetEndOfStreamHelper(video_stream_.get(), pp_error);

  return pp_error;
}

int32_t PepperESDataSourcePrivateTizen::ConfigureDrmSessions() {
  int32_t pp_error = PP_OK;

  if (audio_stream_ && audio_stream_->IsValid())
    pp_error = audio_stream_->ConfigureDrm();

  if (pp_error != PP_OK)
    return pp_error;

  if (video_stream_ && video_stream_->IsValid())
    pp_error = video_stream_->ConfigureDrm();

  return pp_error;
}

void PepperESDataSourcePrivateTizen::ClearDrmSessions() {
  if (audio_stream_)
    audio_stream_->ClearDrmSession();

  if (video_stream_)
    video_stream_->ClearDrmSession();
}

void PepperESDataSourcePrivateTizen::DetachListeners() {
  if (!platform_context_.dispatcher())
    return;

  auto player_handle = platform_context_.dispatcher()->player();

  if (audio_stream_)
    audio_stream_->SourceDetached(player_handle);

  if (video_stream_)
    video_stream_->SourceDetached(player_handle);
}

}  // namespace content
