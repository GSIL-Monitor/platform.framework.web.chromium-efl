// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/audio/audio_input_controller.h"

#include <inttypes.h>

#include <algorithm>
#include <limits>
#include <utility>

#include "base/bind.h"
#include "base/memory/ptr_util.h"
#include "base/metrics/histogram_macros.h"
#include "base/single_thread_task_runner.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"
#include "base/threading/thread_restrictions.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/time/time.h"
#include "base/trace_event/trace_event.h"
#include "media/base/user_input_monitor.h"

namespace media {
namespace {

const int kMaxInputChannels = 3;
constexpr int kCheckMutedStateIntervalSeconds = 1;

#if defined(AUDIO_POWER_MONITORING)
// Time in seconds between two successive measurements of audio power levels.
const int kPowerMonitorLogIntervalSeconds = 15;

// A warning will be logged when the microphone audio volume is below this
// threshold.
const int kLowLevelMicrophoneLevelPercent = 10;

// Logs if the user has enabled the microphone mute or not. This is normally
// done by marking a checkbox in an audio-settings UI which is unique for each
// platform. Elements in this enum should not be added, deleted or rearranged.
enum MicrophoneMuteResult {
  MICROPHONE_IS_MUTED = 0,
  MICROPHONE_IS_NOT_MUTED = 1,
  MICROPHONE_MUTE_MAX = MICROPHONE_IS_NOT_MUTED
};

void LogMicrophoneMuteResult(MicrophoneMuteResult result) {
  UMA_HISTOGRAM_ENUMERATION("Media.MicrophoneMuted",
                            result,
                            MICROPHONE_MUTE_MAX + 1);
}

// Helper method which calculates the average power of an audio bus. Unit is in
// dBFS, where 0 dBFS corresponds to all channels and samples equal to 1.0.
float AveragePower(const AudioBus& buffer) {
  const int frames = buffer.frames();
  const int channels = buffer.channels();
  if (frames <= 0 || channels <= 0)
    return 0.0f;

  // Scan all channels and accumulate the sum of squares for all samples.
  float sum_power = 0.0f;
  for (int ch = 0; ch < channels; ++ch) {
    const float* channel_data = buffer.channel(ch);
    for (int i = 0; i < frames; i++) {
      const float sample = channel_data[i];
      sum_power += sample * sample;
    }
  }

  // Update accumulated average results, with clamping for sanity.
  const float average_power =
      std::max(0.0f, std::min(1.0f, sum_power / (frames * channels)));

  // Convert average power level to dBFS units, and pin it down to zero if it
  // is insignificantly small.
  const float kInsignificantPower = 1.0e-10f;  // -100 dBFS
  const float power_dbfs = average_power < kInsignificantPower ?
      -std::numeric_limits<float>::infinity() : 10.0f * log10f(average_power);

  return power_dbfs;
}
#endif  // AUDIO_POWER_MONITORING

}  // namespace

// Private subclass of AIC that covers the state while capturing audio.
// This class implements the callback interface from the lower level audio
// layer and gets called back on the audio hw thread.
// We implement this in a sub class instead of directly in the AIC so that
// - The AIC itself is not an AudioInputCallback.
// - The lifetime of the AudioCallback is shorter than the AIC
// - How tasks are posted to the AIC from the hw callback thread, is different
//   than how tasks are posted from the AIC to itself from the main thread.
//   So, this difference is isolated to the subclass (see below).
// - The callback class can gather information on what happened during capture
//   and store it in a state that can be fetched after stopping capture
//   (received_callback, error_during_callback).
// The AIC itself must not be AddRef-ed on the hw callback thread so that we
// can be guaranteed to not receive callbacks generated by the hw callback
// thread after Close() has been called on the audio manager thread and
// the callback object deleted. To avoid AddRef-ing the AIC and to cancel
// potentially pending tasks, we use a weak pointer to the AIC instance
// when posting.
class AudioInputController::AudioCallback
    : public AudioInputStream::AudioInputCallback {
 public:
  explicit AudioCallback(AudioInputController* controller)
      : controller_(controller),
        weak_controller_(controller->weak_ptr_factory_.GetWeakPtr()) {}
  ~AudioCallback() override {}

  bool received_callback() const { return received_callback_; }
  bool error_during_callback() const { return error_during_callback_; }

 private:
  void OnData(const AudioBus* source,
              base::TimeTicks capture_time,
              double volume) override {
    TRACE_EVENT0("audio", "AC::OnData");

    received_callback_ = true;

    DeliverDataToSyncWriter(source, capture_time, volume);

#if BUILDFLAG(ENABLE_WEBRTC)
    controller_->debug_recording_helper_.OnData(source);
#endif
  }

  void OnError() override {
    error_during_callback_ = true;
    controller_->task_runner_->PostTask(
        FROM_HERE,
        base::BindOnce(&AudioInputController::DoReportError, weak_controller_));
  }

  void DeliverDataToSyncWriter(const AudioBus* source,
                               base::TimeTicks capture_time,
                               double volume) {
    bool key_pressed = controller_->CheckForKeyboardInput();
    controller_->sync_writer_->Write(source, volume, key_pressed, capture_time);

    // The way the two classes interact here, could be done in a nicer way.
    // As is, we call the AIC here to check the audio power, return  and then
    // post a task to the AIC based on what the AIC said.
    // The reason for this is to keep all PostTask calls from the hw callback
    // thread to the AIC, that use a weak pointer, in the same class.
    float average_power_dbfs;
    int mic_volume_percent;
    if (controller_->CheckAudioPower(source, volume, &average_power_dbfs,
                                     &mic_volume_percent)) {
      // Use event handler on the audio thread to relay a message to the ARIH
      // in content which does the actual logging on the IO thread.
      controller_->task_runner_->PostTask(
          FROM_HERE, base::BindOnce(&AudioInputController::DoLogAudioLevels,
                                    weak_controller_, average_power_dbfs,
                                    mic_volume_percent));
    }
  }

  AudioInputController* const controller_;
  // We do not want any pending posted tasks generated from the callback class
  // to keep the controller object alive longer than it should. So we use
  // a weak ptr whenever we post, we use this weak pointer.
  base::WeakPtr<AudioInputController> weak_controller_;
  bool received_callback_ = false;
  bool error_during_callback_ = false;
};

// static
AudioInputController::Factory* AudioInputController::factory_ = nullptr;

AudioInputController::AudioInputController(
    scoped_refptr<base::SingleThreadTaskRunner> task_runner,
    EventHandler* handler,
    SyncWriter* sync_writer,
    UserInputMonitor* user_input_monitor,
    const AudioParameters& params,
    StreamType type)
    : creator_task_runner_(base::ThreadTaskRunnerHandle::Get()),
      task_runner_(std::move(task_runner)),
      handler_(handler),
      stream_(nullptr),
      sync_writer_(sync_writer),
      type_(type),
      user_input_monitor_(user_input_monitor),
#if BUILDFLAG(ENABLE_WEBRTC)
      debug_recording_helper_(params, task_runner_, base::OnceClosure()),
#endif
      weak_ptr_factory_(this) {
  DCHECK(creator_task_runner_.get());
  DCHECK(handler_);
  DCHECK(sync_writer_);
}

AudioInputController::~AudioInputController() {
  DCHECK(!audio_callback_);
  DCHECK(!stream_);
  DCHECK(!check_muted_state_timer_.IsRunning());
}

// static
scoped_refptr<AudioInputController> AudioInputController::Create(
    AudioManager* audio_manager,
    EventHandler* event_handler,
    SyncWriter* sync_writer,
    UserInputMonitor* user_input_monitor,
    const AudioParameters& params,
    const std::string& device_id,
    bool enable_agc) {
  DCHECK(audio_manager);
  DCHECK(sync_writer);
  DCHECK(event_handler);

  if (!params.IsValid() || (params.channels() > kMaxInputChannels))
    return nullptr;

  if (factory_) {
    return factory_->Create(audio_manager->GetTaskRunner(), sync_writer,
                            audio_manager, event_handler, params,
                            user_input_monitor, ParamsToStreamType(params));
  }

  // Create the AudioInputController object and ensure that it runs on
  // the audio-manager thread.
  scoped_refptr<AudioInputController> controller(new AudioInputController(
      audio_manager->GetTaskRunner(), event_handler, sync_writer,
      user_input_monitor, params, ParamsToStreamType(params)));

  // Create and open a new audio input stream from the existing
  // audio-device thread. Use the provided audio-input device.
  if (!controller->task_runner_->PostTask(
          FROM_HERE, base::BindOnce(&AudioInputController::DoCreate, controller,
                                    base::Unretained(audio_manager), params,
                                    device_id, enable_agc))) {
    controller = nullptr;
  }

  return controller;
}

// static
scoped_refptr<AudioInputController> AudioInputController::CreateForStream(
    const scoped_refptr<base::SingleThreadTaskRunner>& task_runner,
    EventHandler* event_handler,
    AudioInputStream* stream,
    SyncWriter* sync_writer,
    UserInputMonitor* user_input_monitor,
    const AudioParameters& params) {
  DCHECK(sync_writer);
  DCHECK(stream);
  DCHECK(event_handler);

  if (factory_) {
    return factory_->Create(task_runner, sync_writer, AudioManager::Get(),
                            event_handler,
                            AudioParameters::UnavailableDeviceParams(),
                            user_input_monitor, VIRTUAL);
  }

  // Create the AudioInputController object and ensure that it runs on
  // the audio-manager thread.
  scoped_refptr<AudioInputController> controller(
      new AudioInputController(task_runner, event_handler, sync_writer,
                               user_input_monitor, params, VIRTUAL));

  if (!controller->task_runner_->PostTask(
          FROM_HERE,
          base::BindOnce(&AudioInputController::DoCreateForStream, controller,
                         stream, /*enable_agc*/ false))) {
    controller = nullptr;
  }

  return controller;
}

void AudioInputController::Record() {
  DCHECK(creator_task_runner_->BelongsToCurrentThread());
  task_runner_->PostTask(FROM_HERE,
                         base::BindOnce(&AudioInputController::DoRecord, this));
}

void AudioInputController::Close(base::OnceClosure closed_task) {
  DCHECK(!closed_task.is_null());
  DCHECK(creator_task_runner_->BelongsToCurrentThread());

  task_runner_->PostTaskAndReply(
      FROM_HERE, base::BindOnce(&AudioInputController::DoClose, this),
      std::move(closed_task));
}

void AudioInputController::SetVolume(double volume) {
  DCHECK(creator_task_runner_->BelongsToCurrentThread());
  task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&AudioInputController::DoSetVolume, this, volume));
}

void AudioInputController::DoCreate(AudioManager* audio_manager,
                                    const AudioParameters& params,
                                    const std::string& device_id,
                                    bool enable_agc) {
  DCHECK(task_runner_->BelongsToCurrentThread());
  DCHECK(!stream_);
  SCOPED_UMA_HISTOGRAM_TIMER("Media.AudioInputController.CreateTime");
  handler_->OnLog(this, "AIC::DoCreate");

#if defined(AUDIO_POWER_MONITORING)
  // We only do power measurements for UMA stats for low latency streams, and
  // only if agc is requested, to avoid adding logs and UMA for non-WebRTC
  // clients.
  power_measurement_is_enabled_ = (type_ == LOW_LATENCY && enable_agc);
  last_audio_level_log_time_ = base::TimeTicks::Now();
#endif

  // MakeAudioInputStream might fail and return nullptr. If so,
  // DoCreateForStream will handle and report it.
  auto* stream = audio_manager->MakeAudioInputStream(
      params, device_id,
      base::BindRepeating(&AudioInputController::LogMessage, this));
  DoCreateForStream(stream, enable_agc);
}

void AudioInputController::DoCreateForStream(
    AudioInputStream* stream_to_control,
    bool enable_agc) {
  DCHECK(task_runner_->BelongsToCurrentThread());
  DCHECK(!stream_);
  handler_->OnLog(this, "AIC::DoCreateForStream");

  if (!stream_to_control) {
    LogCaptureStartupResult(CAPTURE_STARTUP_CREATE_STREAM_FAILED);
    handler_->OnError(this, STREAM_CREATE_ERROR);
    return;
  }

  if (!stream_to_control->Open()) {
    stream_to_control->Close();
    LogCaptureStartupResult(CAPTURE_STARTUP_OPEN_STREAM_FAILED);
    handler_->OnError(this, STREAM_OPEN_ERROR);
    return;
  }

#if defined(AUDIO_POWER_MONITORING)
  bool agc_is_supported =
      stream_to_control->SetAutomaticGainControl(enable_agc);
  // Disable power measurements on platforms that does not support AGC at a
  // lower level. AGC can fail on platforms where we don't support the
  // functionality to modify the input volume slider. One such example is
  // Windows XP.
  power_measurement_is_enabled_ &= agc_is_supported;
#else
  stream_to_control->SetAutomaticGainControl(enable_agc);
#endif

  // Finally, keep the stream pointer around, update the state and notify.
  stream_ = stream_to_control;

  // Send initial muted state along with OnCreated, to avoid races.
  is_muted_ = stream_->IsMuted();
  handler_->OnCreated(this, is_muted_);

  check_muted_state_timer_.Start(
      FROM_HERE, base::TimeDelta::FromSeconds(kCheckMutedStateIntervalSeconds),
      this, &AudioInputController::CheckMutedState);
  DCHECK(check_muted_state_timer_.IsRunning());
}

void AudioInputController::DoRecord() {
  DCHECK(task_runner_->BelongsToCurrentThread());
  SCOPED_UMA_HISTOGRAM_TIMER("Media.AudioInputController.RecordTime");

  if (!stream_ || audio_callback_)
    return;

  handler_->OnLog(this, "AIC::DoRecord");

  if (user_input_monitor_) {
    user_input_monitor_->EnableKeyPressMonitoring();
    prev_key_down_count_ = user_input_monitor_->GetKeyPressCount();
  }

  stream_create_time_ = base::TimeTicks::Now();

  audio_callback_.reset(new AudioCallback(this));
  stream_->Start(audio_callback_.get());
}

void AudioInputController::DoClose() {
  DCHECK(task_runner_->BelongsToCurrentThread());
  SCOPED_UMA_HISTOGRAM_TIMER("Media.AudioInputController.CloseTime");

  if (!stream_)
    return;

  check_muted_state_timer_.AbandonAndStop();

  std::string log_string;
  static const char kLogStringPrefix[] = "AIC::DoClose:";

  // Allow calling unconditionally and bail if we don't have a stream to close.
  if (audio_callback_) {
    stream_->Stop();

    // Sometimes a stream (and accompanying audio track) is created and
    // immediately closed or discarded. In this case they are registered as
    // 'stopped early' rather than 'never got data'.
    const base::TimeDelta duration =
        base::TimeTicks::Now() - stream_create_time_;
    CaptureStartupResult capture_startup_result =
        audio_callback_->received_callback()
            ? CAPTURE_STARTUP_OK
            : (duration.InMilliseconds() < 500
                   ? CAPTURE_STARTUP_STOPPED_EARLY
                   : CAPTURE_STARTUP_NEVER_GOT_DATA);
    LogCaptureStartupResult(capture_startup_result);
    LogCallbackError();

    log_string = base::StringPrintf(
        "%s stream duration=%" PRId64 " seconds%s", kLogStringPrefix,
        duration.InSeconds(),
        audio_callback_->received_callback() ? "" : " (no callbacks received)");

    if (type_ == LOW_LATENCY) {
      if (audio_callback_->received_callback()) {
        UMA_HISTOGRAM_LONG_TIMES("Media.InputStreamDuration", duration);
      } else {
        UMA_HISTOGRAM_LONG_TIMES("Media.InputStreamDurationWithoutCallback",
                                 duration);
      }
    }

    if (user_input_monitor_)
      user_input_monitor_->DisableKeyPressMonitoring();

    audio_callback_.reset();
  } else {
    log_string =
        base::StringPrintf("%s recording never started", kLogStringPrefix);
  }

  handler_->OnLog(this, log_string);

  stream_->Close();
  stream_ = nullptr;

  sync_writer_->Close();

#if defined(AUDIO_POWER_MONITORING)
  // Send UMA stats if enabled.
  if (power_measurement_is_enabled_)
    LogSilenceState(silence_state_);
#endif

#if BUILDFLAG(ENABLE_WEBRTC)
  debug_recording_helper_.DisableDebugRecording();
#endif

  max_volume_ = 0.0;
  weak_ptr_factory_.InvalidateWeakPtrs();
}

void AudioInputController::DoReportError() {
  DCHECK(task_runner_->BelongsToCurrentThread());
  handler_->OnError(this, STREAM_ERROR);
}

void AudioInputController::DoSetVolume(double volume) {
  DCHECK(task_runner_->BelongsToCurrentThread());
  DCHECK_GE(volume, 0);
  DCHECK_LE(volume, 1.0);

  if (!stream_)
    return;

  // Only ask for the maximum volume at first call and use cached value
  // for remaining function calls.
  if (!max_volume_) {
    max_volume_ = stream_->GetMaxVolume();
  }

  if (max_volume_ == 0.0) {
    DLOG(WARNING) << "Failed to access input volume control";
    return;
  }

  // Set the stream volume and scale to a range matched to the platform.
  stream_->SetVolume(max_volume_ * volume);
}

void AudioInputController::DoLogAudioLevels(float level_dbfs,
                                            int microphone_volume_percent) {
#if defined(AUDIO_POWER_MONITORING)
  DCHECK(task_runner_->BelongsToCurrentThread());
  if (!stream_)
    return;

  // Detect if the user has enabled hardware mute by pressing the mute
  // button in audio settings for the selected microphone.
  const bool microphone_is_muted = stream_->IsMuted();
  if (microphone_is_muted) {
    LogMicrophoneMuteResult(MICROPHONE_IS_MUTED);
    handler_->OnLog(this, "AIC::OnData: microphone is muted!");
    // Return early if microphone is muted. No need to adding logs and UMA stats
    // of audio levels if we know that the micropone is muted.
    return;
  }

  LogMicrophoneMuteResult(MICROPHONE_IS_NOT_MUTED);

  std::string log_string = base::StringPrintf(
      "AIC::OnData: average audio level=%.2f dBFS", level_dbfs);
  static const float kSilenceThresholdDBFS = -72.24719896f;
  if (level_dbfs < kSilenceThresholdDBFS)
    log_string += " <=> low audio input level!";
  handler_->OnLog(this, log_string);

  UpdateSilenceState(level_dbfs < kSilenceThresholdDBFS);

  UMA_HISTOGRAM_PERCENTAGE("Media.MicrophoneVolume", microphone_volume_percent);
  log_string = base::StringPrintf(
      "AIC::OnData: microphone volume=%d%%", microphone_volume_percent);
  if (microphone_volume_percent < kLowLevelMicrophoneLevelPercent)
    log_string += " <=> low microphone level!";
  handler_->OnLog(this, log_string);
#endif
}

void AudioInputController::EnableDebugRecording(
    const base::FilePath& file_name) {
#if BUILDFLAG(ENABLE_WEBRTC)
  DCHECK(creator_task_runner_->BelongsToCurrentThread());
  task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&AudioInputController::DoEnableDebugRecording,
                                this, file_name));
#endif
}

void AudioInputController::DisableDebugRecording() {
#if BUILDFLAG(ENABLE_WEBRTC)
  DCHECK(creator_task_runner_->BelongsToCurrentThread());
  task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&AudioInputController::DoDisableDebugRecording, this));
#endif
}

#if defined(AUDIO_POWER_MONITORING)
void AudioInputController::UpdateSilenceState(bool silence) {
  if (silence) {
    if (silence_state_ == SILENCE_STATE_NO_MEASUREMENT) {
      silence_state_ = SILENCE_STATE_ONLY_SILENCE;
    } else if (silence_state_ == SILENCE_STATE_ONLY_AUDIO) {
      silence_state_ = SILENCE_STATE_AUDIO_AND_SILENCE;
    } else {
      DCHECK(silence_state_ == SILENCE_STATE_ONLY_SILENCE ||
             silence_state_ == SILENCE_STATE_AUDIO_AND_SILENCE);
    }
  } else {
    if (silence_state_ == SILENCE_STATE_NO_MEASUREMENT) {
      silence_state_ = SILENCE_STATE_ONLY_AUDIO;
    } else if (silence_state_ == SILENCE_STATE_ONLY_SILENCE) {
      silence_state_ = SILENCE_STATE_AUDIO_AND_SILENCE;
    } else {
      DCHECK(silence_state_ == SILENCE_STATE_ONLY_AUDIO ||
             silence_state_ == SILENCE_STATE_AUDIO_AND_SILENCE);
    }
  }
}

void AudioInputController::LogSilenceState(SilenceState value) {
  UMA_HISTOGRAM_ENUMERATION("Media.AudioInputControllerSessionSilenceReport",
                            value,
                            SILENCE_STATE_MAX + 1);
}
#endif

void AudioInputController::LogCaptureStartupResult(
    CaptureStartupResult result) {
  switch (type_) {
    case LOW_LATENCY:
      UMA_HISTOGRAM_ENUMERATION("Media.LowLatencyAudioCaptureStartupSuccess",
                                result, CAPTURE_STARTUP_RESULT_MAX + 1);
      break;
    case HIGH_LATENCY:
      UMA_HISTOGRAM_ENUMERATION("Media.HighLatencyAudioCaptureStartupSuccess",
                                result, CAPTURE_STARTUP_RESULT_MAX + 1);
      break;
    case VIRTUAL:
      UMA_HISTOGRAM_ENUMERATION("Media.VirtualAudioCaptureStartupSuccess",
                                result, CAPTURE_STARTUP_RESULT_MAX + 1);
      break;
    default:
      break;
  }
}

void AudioInputController::LogCallbackError() {
  bool error_during_callback = audio_callback_->error_during_callback();
  switch (type_) {
    case LOW_LATENCY:
      UMA_HISTOGRAM_BOOLEAN("Media.Audio.Capture.LowLatencyCallbackError",
                            error_during_callback);
      break;
    case HIGH_LATENCY:
      UMA_HISTOGRAM_BOOLEAN("Media.Audio.Capture.HighLatencyCallbackError",
                            error_during_callback);
      break;
    case VIRTUAL:
      UMA_HISTOGRAM_BOOLEAN("Media.Audio.Capture.VirtualCallbackError",
                            error_during_callback);
      break;
    default:
      break;
  }
}

#if BUILDFLAG(ENABLE_WEBRTC)
void AudioInputController::DoEnableDebugRecording(
    const base::FilePath& file_name) {
  DCHECK(task_runner_->BelongsToCurrentThread());
  debug_recording_helper_.EnableDebugRecording(file_name);
}

void AudioInputController::DoDisableDebugRecording() {
  DCHECK(task_runner_->BelongsToCurrentThread());
  debug_recording_helper_.DisableDebugRecording();
}
#endif  // BUILDFLAG(ENABLE_WEBRTC)

void AudioInputController::LogMessage(const std::string& message) {
  DCHECK(task_runner_->BelongsToCurrentThread());
  handler_->OnLog(this, message);
}

bool AudioInputController::CheckForKeyboardInput() {
  if (!user_input_monitor_)
    return false;

  const size_t current_count = user_input_monitor_->GetKeyPressCount();
  const bool key_pressed = current_count != prev_key_down_count_;
  prev_key_down_count_ = current_count;
  DVLOG_IF(6, key_pressed) << "Detected keypress.";

  return key_pressed;
}

bool AudioInputController::CheckAudioPower(const AudioBus* source,
                                           double volume,
                                           float* average_power_dbfs,
                                           int* mic_volume_percent) {
#if defined(AUDIO_POWER_MONITORING)
  // Only do power-level measurements if DoCreate() has been called. It will
  // ensure that logging will mainly be done for WebRTC and WebSpeech
  // clients.
  if (!power_measurement_is_enabled_)
    return false;

  // Perform periodic audio (power) level measurements.
  const auto now = base::TimeTicks::Now();
  if ((now - last_audio_level_log_time_).InSeconds() <=
      kPowerMonitorLogIntervalSeconds) {
    return false;
  }

  *average_power_dbfs = AveragePower(*source);
  *mic_volume_percent = static_cast<int>(100.0 * volume);

  last_audio_level_log_time_ = now;

  return true;
#else
  return false;
#endif
}

void AudioInputController::CheckMutedState() {
  DCHECK(task_runner_->BelongsToCurrentThread());
  DCHECK(stream_);
  const bool new_state = stream_->IsMuted();
  if (new_state != is_muted_) {
    is_muted_ = new_state;
    // We don't log OnMuted here, but leave that for AudioInputRendererHost.
    handler_->OnMuted(this, is_muted_);
  }
}

// static
AudioInputController::StreamType AudioInputController::ParamsToStreamType(
    const AudioParameters& params) {
  switch (params.format()) {
    case AudioParameters::Format::AUDIO_PCM_LINEAR:
      return AudioInputController::StreamType::HIGH_LATENCY;
    case AudioParameters::Format::AUDIO_PCM_LOW_LATENCY:
      return AudioInputController::StreamType::LOW_LATENCY;
    default:
      // Currently, the remaining supported type is fake. Reconsider if other
      // formats become supported.
      return AudioInputController::StreamType::FAKE;
  }
}

}  // namespace media
