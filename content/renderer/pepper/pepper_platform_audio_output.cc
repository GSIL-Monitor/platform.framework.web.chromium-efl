// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/pepper/pepper_platform_audio_output.h"

#include "base/bind.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "build/build_config.h"
#include "content/child/child_process.h"
#include "content/renderer/media/audio_ipc_factory.h"
#include "content/renderer/pepper/audio_helper.h"
#include "ppapi/shared_impl/ppb_audio_config_shared.h"

#if defined(TIZEN_PEPPER_EXTENSIONS) && defined(OS_TIZEN_TV_PRODUCT)
// TODO: Remove these includes after removing a Spotify hack in Initialize
// method.
#include "base/command_line.h"
#include "ewk/efl_integration/common/content_switches_efl.h"
#endif  // defined(TIZEN_PEPPER_EXTENSIONS) && defined(OS_TIZEN_TV_PRODUCT)

namespace content {

// static
PepperPlatformAudioOutput* PepperPlatformAudioOutput::Create(
#if defined(TIZEN_PEPPER_EXTENSIONS)
    PP_AudioMode audio_mode,
#endif
    int sample_rate,
    int frames_per_buffer,
    int source_render_frame_id,
    AudioHelper* client) {
  scoped_refptr<PepperPlatformAudioOutput> audio_output(
      new PepperPlatformAudioOutput());
  if (audio_output->Initialize(
#if defined(TIZEN_PEPPER_EXTENSIONS)
          audio_mode,
#endif
          sample_rate, frames_per_buffer, source_render_frame_id, client)) {
    // Balanced by Release invoked in
    // PepperPlatformAudioOutput::ShutDownOnIOThread().
    audio_output->AddRef();
    return audio_output.get();
  }
  return NULL;
}

bool PepperPlatformAudioOutput::StartPlayback() {
  if (ipc_) {
    io_task_runner_->PostTask(
        FROM_HERE,
        base::BindOnce(&PepperPlatformAudioOutput::StartPlaybackOnIOThread,
                       this));
    return true;
  }
  return false;
}

bool PepperPlatformAudioOutput::StopPlayback() {
  if (ipc_) {
    io_task_runner_->PostTask(
        FROM_HERE,
        base::BindOnce(&PepperPlatformAudioOutput::StopPlaybackOnIOThread,
                       this));
    return true;
  }
  return false;
}

bool PepperPlatformAudioOutput::SetVolume(double volume) {
  if (ipc_) {
    io_task_runner_->PostTask(
        FROM_HERE,
        base::BindOnce(&PepperPlatformAudioOutput::SetVolumeOnIOThread, this,
                       volume));
    return true;
  }
  return false;
}

void PepperPlatformAudioOutput::ShutDown() {
  // Called on the main thread to stop all audio callbacks. We must only change
  // the client on the main thread, and the delegates from the I/O thread.
  client_ = NULL;
  io_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&PepperPlatformAudioOutput::ShutDownOnIOThread, this));
}

void PepperPlatformAudioOutput::OnError() {}

void PepperPlatformAudioOutput::OnDeviceAuthorized(
    media::OutputDeviceStatus device_status,
    const media::AudioParameters& output_params,
    const std::string& matched_device_id) {
  NOTREACHED();
}

void PepperPlatformAudioOutput::OnStreamCreated(
    base::SharedMemoryHandle handle,
    base::SyncSocket::Handle socket_handle) {
  DCHECK(handle.IsValid());
#if defined(OS_WIN)
  DCHECK(socket_handle);
#else
  DCHECK_NE(-1, socket_handle);
#endif
  DCHECK(handle.GetSize());

  if (base::ThreadTaskRunnerHandle::Get().get() == main_task_runner_.get()) {
    // Must dereference the client only on the main thread. Shutdown may have
    // occurred while the request was in-flight, so we need to NULL check.
    if (client_)
      client_->StreamCreated(handle, handle.GetSize(), socket_handle);
  } else {
    main_task_runner_->PostTask(
        FROM_HERE, base::BindOnce(&PepperPlatformAudioOutput::OnStreamCreated,
                                  this, handle, socket_handle));
  }
}

void PepperPlatformAudioOutput::OnIPCClosed() { ipc_.reset(); }

PepperPlatformAudioOutput::~PepperPlatformAudioOutput() {
  // Make sure we have been shut down. Warning: this will usually happen on
  // the I/O thread!
  DCHECK(!ipc_);
  DCHECK(!client_);
}

PepperPlatformAudioOutput::PepperPlatformAudioOutput()
    : client_(NULL),
      main_task_runner_(base::ThreadTaskRunnerHandle::Get()),
      io_task_runner_(ChildProcess::current()->io_task_runner()) {
}

bool PepperPlatformAudioOutput::Initialize(
#if defined(TIZEN_PEPPER_EXTENSIONS)
    PP_AudioMode audio_mode,
#endif
    int sample_rate,
    int frames_per_buffer,
    int source_render_frame_id,
    AudioHelper* client) {
  DCHECK(client);
  client_ = client;

  ipc_ = AudioIPCFactory::get()->CreateAudioOutputIPC(source_render_frame_id);
  CHECK(ipc_);

  media::AudioParameters::Format format =
      media::AudioParameters::AUDIO_PCM_LOW_LATENCY;

#if defined(TIZEN_PEPPER_EXTENSIONS)
#if defined(OS_TIZEN_TV_PRODUCT)
  {
    // Temporarily use PP_AUDIOMODE_MUSIC audio mode for Spotify app,
    // to let QA team test new backend.
    const base::CommandLine& command_line =
        *base::CommandLine::ForCurrentProcess();
    std::string tizen_app_id =
        command_line.GetSwitchValueASCII(switches::kTizenAppId);
    if (tizen_app_id == "rJeHak5zRg.Spotify") {
      if (audio_mode == PP_AUDIOMODE_AUTO)
        audio_mode = PP_AUDIOMODE_MUSIC;
    }
  }
#endif  // OS_TIZEN_TV_PRODUCT

  switch (audio_mode) {
    case PP_AUDIOMODE_AUTO:  // fallthrough
    case PP_AUDIOMODE_GAME:
      format = media::AudioParameters::AUDIO_PCM_LOW_LATENCY;
      break;
    case PP_AUDIOMODE_MUSIC:
      format = media::AudioParameters::AUDIO_PCM_LOW_LATENCY_PPAPI;
      break;
    default:
      LOG(ERROR) << "Unknown audio mode enumeration value";
      NOTREACHED();
      break;
  }
#endif  // TIZEN_PEPPER_EXTENSIONS

  media::AudioParameters params(format, media::CHANNEL_LAYOUT_STEREO,
                                sample_rate, ppapi::kBitsPerAudioOutputSample,
                                frames_per_buffer);

  io_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&PepperPlatformAudioOutput::InitializeOnIOThread, this,
                     params));
  return true;
}

void PepperPlatformAudioOutput::InitializeOnIOThread(
    const media::AudioParameters& params) {
  DCHECK(io_task_runner_->BelongsToCurrentThread());
  if (ipc_)
    ipc_->CreateStream(this, params);
}

void PepperPlatformAudioOutput::StartPlaybackOnIOThread() {
  DCHECK(io_task_runner_->BelongsToCurrentThread());
  if (ipc_)
    ipc_->PlayStream();
}

void PepperPlatformAudioOutput::SetVolumeOnIOThread(double volume) {
  DCHECK(io_task_runner_->BelongsToCurrentThread());
  if (ipc_)
    ipc_->SetVolume(volume);
}

void PepperPlatformAudioOutput::StopPlaybackOnIOThread() {
  DCHECK(io_task_runner_->BelongsToCurrentThread());
  if (ipc_)
    ipc_->PauseStream();
}

void PepperPlatformAudioOutput::ShutDownOnIOThread() {
  DCHECK(io_task_runner_->BelongsToCurrentThread());

  // Make sure we don't call shutdown more than once.
  if (!ipc_)
    return;

  ipc_->CloseStream();
  ipc_.reset();

  Release();  // Release for the delegate, balances out the reference taken in
              // PepperPlatformAudioOutput::Create.
}

}  // namespace content
