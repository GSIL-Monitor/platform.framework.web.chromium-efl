// Copyright (c) 2014 Samsung Electronics Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/speech/tts_tizen.h"

#include <vector>

#include "base/logging.h"

namespace content {
namespace {
const char kUriPrefix[] = "localhost:";
const std::string kAuto = "Auto";
const std::string kMale = "Male";
const std::string kFemale = "Female";
const std::string kChild = "Child";

std::string VoiceTypeToName(tts_voice_type_e type) {
  switch (type) {
    case TTS_VOICE_TYPE_AUTO:
      return kAuto;
    case TTS_VOICE_TYPE_MALE:
      return kMale;
    case TTS_VOICE_TYPE_FEMALE:
      return kFemale;
    case TTS_VOICE_TYPE_CHILD:
      return kChild;
  }
  return kAuto;
}

tts_voice_type_e VoiceNameToType(std::string name) {
  if (name == kAuto)
    return TTS_VOICE_TYPE_AUTO;
  else if (name == kMale)
    return TTS_VOICE_TYPE_MALE;
  else if (name == kFemale)
    return TTS_VOICE_TYPE_FEMALE;
  else if (name == kChild)
    return TTS_VOICE_TYPE_CHILD;
  else
    return TTS_VOICE_TYPE_AUTO;
}

const std::string ErrorToString(tts_error_e error) {
  switch (error) {
    case TTS_ERROR_NONE:
      return "Successful";
    case TTS_ERROR_OUT_OF_MEMORY:
      return "Out of Memory";
    case TTS_ERROR_IO_ERROR:
      return "I/O error";
    case TTS_ERROR_INVALID_PARAMETER:
      return "Invalid parameter";
    case TTS_ERROR_OUT_OF_NETWORK:
      return "Out of network";
    case TTS_ERROR_INVALID_STATE:
      return "Invalid state";
    case TTS_ERROR_INVALID_VOICE:
      return "Invalid voice";
    case TTS_ERROR_ENGINE_NOT_FOUND:
      return "No available engine";
    case TTS_ERROR_TIMED_OUT:
      return "No answer from the daemon";
    case TTS_ERROR_OPERATION_FAILED:
      return "Operation failed";
    case TTS_ERROR_AUDIO_POLICY_BLOCKED:
      return "Audio policy blocked";
    default:
      return "Unknown Error";
  }
}

void TtsStateChangedCallback(tts_h tts_handle,
                             tts_state_e previous,
                             tts_state_e current,
                             void* user_data) {
  TtsTizen* _this = static_cast<TtsTizen*>(user_data);

  if (TTS_STATE_CREATED == previous && TTS_STATE_READY == current)
    _this->TtsReady();
  else if (TTS_STATE_PAUSED == previous &&
       TTS_STATE_PLAYING == current && _this->GetUtterance().id)
    _this->GetTtsMessageFilterEfl()->DidResumeSpeaking(_this->GetUtterance().id);
  else if (TTS_STATE_PLAYING == previous &&
       TTS_STATE_PAUSED == current && _this->GetUtterance().id)
    _this->GetTtsMessageFilterEfl()->DidPauseSpeaking(_this->GetUtterance().id);
  // When playing interrupted by external module such as volume changing,
  // notify speaking finish event, or else can not speak utterance anymore
  else if (TTS_STATE_PLAYING == previous && TTS_STATE_READY == current &&
           _this->GetUtterance().id) {
    _this->GetTtsMessageFilterEfl()->DidFinishSpeaking(
        _this->GetUtterance().id);
  }
}

void TtsUtteranceStartedCallback(tts_h tts_handle,
                                 int utteranceId,
                                 void* user_data) {
  TtsTizen* _this = static_cast<TtsTizen*>(user_data);
  if (_this->GetUtterance().id)
    _this->GetTtsMessageFilterEfl()->
         DidStartSpeaking(_this->GetUtterance().id);
}

void TtsUtteranceCompletedCallback(tts_h tts_handle,
                                   int utteranceId,
                                   void* user_data) {
  TtsTizen* _this = static_cast<TtsTizen*>(user_data);
  if (_this->GetUtterance().id)
    _this->GetTtsMessageFilterEfl()->
      DidFinishSpeaking(_this->GetUtterance().id);
}

void TtsErrorCallback(tts_h tts_handle,
                      int utteranceId,
                      tts_error_e reason,
                      void* user_data) {
  TtsTizen* _this = static_cast<TtsTizen*>(user_data);
  if (_this->GetUtterance().id)
    _this->GetTtsMessageFilterEfl()->
      SpeakingErrorOccurred(_this->GetUtterance().id, ErrorToString(reason));
  LOG(ERROR) << "TTS: Error - " << ErrorToString(reason);
}

bool TtsSupportedVoiceCallback(tts_h tts_handle,
                                const char* language,
                                int voice_type, void* user_data) {
  TtsTizen* _this = static_cast<TtsTizen*>(user_data);
  int currentVoiceType = voice_type;
  _this->AddVoice(std::string(language), currentVoiceType);
  return true;
}

void TtsDefaultVoiceChangedCallback(tts_h tts_handle,
                                    const char* previous_language,
                                    int previous_voice_type,
                                    const char* current_language,
                                    int current_voice_type,
                                    void* user_data) {
  TtsTizen* _this = static_cast<TtsTizen*>(user_data);
  _this->ChangeTtsDefaultVoice(current_language, current_voice_type);
}

int GetTtsVoiceSpeed(tts_h tts_handle, float rate) {
  // TTS engine dependent capability.
  const float kRateMin = 0.6f, kRateMax = 2.0f, kRateNormal = 1.0f;

  int tts_speed_min = 0, tts_speed_normal = 0, tts_speed_max = 0;
  int ret = tts_get_speed_range(tts_handle,
                &tts_speed_min, &tts_speed_normal, &tts_speed_max);
  if (ret != TTS_ERROR_NONE) {
    LOG(ERROR) << "TTS: tts_get_speed_range() failed";
    return TTS_SPEED_AUTO;
  }

#if defined(OS_TIZEN_TV_PRODUCT)
  // Epsilon value used to compare float values to zero
  const float kEpsilon = 1e-8f;
  if (abs(rate - kRateNormal) < kEpsilon)
    return TTS_SPEED_AUTO;
#endif

  if (rate <= kRateMin)
    return tts_speed_min;

  if (rate >= kRateMax)
    return tts_speed_max;

  // Piecewise linear interpolation from |rate| to TTS internal speed value.
  if (rate < kRateNormal)
    return static_cast<int>(tts_speed_min + (rate - kRateMin)
        * ((tts_speed_normal - tts_speed_min) / (kRateNormal - kRateMin)));
  else
    return static_cast<int>(tts_speed_normal + (rate - kRateNormal)
        * ((tts_speed_max - tts_speed_normal) / (kRateMax - kRateNormal)));
}

}  // namespace

TtsTizen::TtsTizen(TtsMessageFilterEfl* tts_message_filter_efl)
  : tts_message_filter_efl_(tts_message_filter_efl),
    tts_handle_(NULL),
    default_voice_(TTS_VOICE_TYPE_AUTO),
    tts_initialized_(false),
    tts_state_ready_(false),
    utterance_waiting_(false) {
}

TtsTizen::~TtsTizen() {
  if (tts_initialized_) {
    int ret = tts_unprepare(tts_handle_);
    if (ret != TTS_ERROR_NONE)
      LOG(ERROR) << "TTS: Fail to unprepare - "
                 << ErrorToString(static_cast<tts_error_e>(ret));
  }
  if (tts_handle_) {
    int ret = tts_unset_state_changed_cb(tts_handle_);
    if (ret != TTS_ERROR_NONE)
      LOG(ERROR) << "TTS: Fail to tts_unset_state_changed_cb - "
                 << ErrorToString(static_cast<tts_error_e>(ret));

    ret = tts_unset_utterance_completed_cb(tts_handle_);
    if (ret != TTS_ERROR_NONE)
      LOG(ERROR) << "TTS: Fail to tts_unset_utterance_completed_cb - "
                 << ErrorToString(static_cast<tts_error_e>(ret));

    ret = tts_unset_utterance_started_cb(tts_handle_);
    if (ret != TTS_ERROR_NONE)
      LOG(ERROR) << "TTS: Fail to tts_unset_utterance_started_cb - "
                 << ErrorToString(static_cast<tts_error_e>(ret));

    ret = tts_unset_error_cb(tts_handle_);
    if (ret != TTS_ERROR_NONE)
      LOG(ERROR) << "TTS: Fail to tts_unset_error_cb - "
                 << ErrorToString(static_cast<tts_error_e>(ret));

    ret = tts_unset_default_voice_changed_cb(tts_handle_);
    if (ret != TTS_ERROR_NONE)
      LOG(ERROR) << "TTS: Fail to tts_unset_default_voice_changed_cb - "
                 << ErrorToString(static_cast<tts_error_e>(ret));

    tts_destroy(tts_handle_);
    tts_handle_ = NULL;
  }
  tts_initialized_ = false;
}

bool TtsTizen::Init(tts_mode_e tts_mode) {
  int ret = tts_create(&tts_handle_);
  if (ret != TTS_ERROR_NONE) {
    LOG(ERROR) << "TTS: Fail to create - "
               << ErrorToString(static_cast<tts_error_e>(ret));
    return false;
  }

  StoreTtsDefaultVoice();

  // Set callbacks
  if (!SetTtsCallback())
    return false;

  ret = tts_set_mode(tts_handle_, tts_mode);
  if (ret != TTS_ERROR_NONE) {
    LOG(ERROR) << "TTS: Fail to set mode - "
               << ErrorToString(static_cast<tts_error_e>(ret));
  }
  ret = tts_prepare(tts_handle_);
  if (ret != TTS_ERROR_NONE) {
    LOG(ERROR) << "TTS: Fail to prepare - "
               << ErrorToString(static_cast<tts_error_e>(ret));
    return false;
  }
  tts_initialized_ = true;
  return true;
}

void TtsTizen::StoreTtsDefaultVoice() {
  char* language = NULL;
  int voice_type;
  int ret = tts_get_default_voice(tts_handle_, &language, &voice_type);
  if (ret != TTS_ERROR_NONE) {
    LOG(ERROR) << "TTS: Fail to get default voice";
    // Even if ret != TTS_ERROR_NONE, do not return here.
    // Set the default values.
    const char kDefaultLanguage[] = "en_GB";
    default_language_ = kDefaultLanguage;
    default_voice_ = TTS_VOICE_TYPE_AUTO;
  } else {
    DCHECK(language);
    default_language_ = language;
    free(language);
    default_voice_ = voice_type;
  }
}

void TtsTizen::ChangeTtsDefaultVoice(const std::string language,
                                     const int voice_type) {
  default_language_ = language;
  default_voice_ = voice_type;
}

bool TtsTizen::SetTtsCallback() {
  int ret =
      tts_set_state_changed_cb(tts_handle_, TtsStateChangedCallback, this);
  if (ret != TTS_ERROR_NONE) {
    LOG(ERROR) << "TTS: Unable to set state callback - "
               << ErrorToString(static_cast<tts_error_e>(ret));
    return false;
  }

  ret = tts_set_utterance_started_cb(tts_handle_, TtsUtteranceStartedCallback,
                                     this);
  if (ret != TTS_ERROR_NONE) {
    LOG(ERROR) << "TTS: Fail to set utterance started callback - "
               << ErrorToString(static_cast<tts_error_e>(ret));
    return false;
  }

  ret = tts_set_utterance_completed_cb(tts_handle_,
                                       TtsUtteranceCompletedCallback, this);
  if (ret != TTS_ERROR_NONE) {
    LOG(ERROR) << "TTS: Fail to set utterance completed callback - "
               << ErrorToString(static_cast<tts_error_e>(ret));
    return false;
  }

  ret = tts_set_error_cb(tts_handle_, TtsErrorCallback, this);
  if (ret != TTS_ERROR_NONE) {
    LOG(ERROR) << "TTS: Fail to set error callback - "
               << ErrorToString(static_cast<tts_error_e>(ret));
    return false;
  }

  ret = tts_foreach_supported_voices(tts_handle_, TtsSupportedVoiceCallback,
                                     this);
  if (ret != TTS_ERROR_NONE) {
    LOG(ERROR) << "TTS: Fail to get supported voices - "
               << ErrorToString(static_cast<tts_error_e>(ret));
    return false;
  }

  ret = tts_set_default_voice_changed_cb(tts_handle_,
                                         TtsDefaultVoiceChangedCallback, this);
  if (ret != TTS_ERROR_NONE) {
    LOG(ERROR) << "TTS: Fail to set default voice changed callback - "
               << ErrorToString(static_cast<tts_error_e>(ret));
    return false;
  }

  return true;
}

const std::vector<TtsVoice>& TtsTizen::GetVoiceList() {
  return voice_list_;
}

void TtsTizen::TtsReady() {
  tts_state_ready_ = true;
  if (utterance_waiting_)
    SpeakStoredUtterance();
  utterance_waiting_ = false;
}

void TtsTizen::Speak(const TtsUtteranceRequest& utterance) {
  utterance_ = utterance;
  if (!tts_state_ready_) {
    utterance_waiting_ = true;
    return;
  }
  SpeakStoredUtterance();
}

void TtsTizen::SpeakStoredUtterance() {
  if (utterance_.text.empty()) {
    return;
  }
  std::string current_language = default_language_;
  if (!utterance_.lang.empty())
    current_language = utterance_.lang;

  int voiceType = static_cast<int>(default_voice_);
  if (!utterance_.voice.empty())
    voiceType = static_cast<int>(VoiceNameToType(utterance_.voice));
  int textSpeed = GetTtsVoiceSpeed(tts_handle_, utterance_.rate);
  int utteranceId = utterance_.id;
  int ret = tts_add_text(tts_handle_, utterance_.text.c_str(),
                         current_language.c_str(), voiceType, textSpeed,
                         &utteranceId);
  if (ret != TTS_ERROR_NONE) {
    LOG(ERROR) << "TTS: Fail to add text";
    GetTtsMessageFilterEfl()->SpeakingErrorOccurred(
        utterance_.id, ErrorToString(static_cast<tts_error_e>(ret)));
    return;
  }
  tts_state_e current_state;
  tts_get_state(tts_handle_, &current_state);
  if (TTS_STATE_PLAYING != current_state)
    ret = tts_play(tts_handle_);

  if (ret != TTS_ERROR_NONE) {
    LOG(ERROR) << "TTS: Play Error occured";
    GetTtsMessageFilterEfl()->SpeakingErrorOccurred(
        utterance_.id, ErrorToString(static_cast<tts_error_e>(ret)));
    return;
  }
}

void TtsTizen::Pause() {
  if (!utterance_.id)
    return;

  int ret = tts_pause(tts_handle_);
  if (ret != TTS_ERROR_NONE) {
    LOG(ERROR) << "TTS: Fail to pause #tts_pause";
    GetTtsMessageFilterEfl()->SpeakingErrorOccurred(
        utterance_.id, ErrorToString(static_cast<tts_error_e>(ret)));
  }
}

void TtsTizen::Resume() {
  if (!utterance_.id)
    return;

  int ret = tts_play(tts_handle_);
  if (ret != TTS_ERROR_NONE) {
    LOG(ERROR) << "TTS: Fail to resume";
    GetTtsMessageFilterEfl()->SpeakingErrorOccurred(
        utterance_.id, ErrorToString(static_cast<tts_error_e>(ret)));
  }
}

void TtsTizen::Cancel() {
  int ret = tts_stop(tts_handle_);
  if (ret != TTS_ERROR_NONE) {
    LOG(ERROR) << "TTS: Fail to cancel";
    GetTtsMessageFilterEfl()->SpeakingErrorOccurred(
        utterance_.id, ErrorToString(static_cast<tts_error_e>(ret)));
  }
  GetTtsMessageFilterEfl()->DidFinishSpeaking(utterance_.id);
}

void TtsTizen::AddVoice(std::string language, tts_voice_type_e type) {
  TtsVoice voice;
  std::string uri(kUriPrefix);
  uri.append(VoiceTypeToName(type));
  uri.append("/");
  uri.append(language);

  voice.voice_uri = uri ;
  voice.name = VoiceTypeToName(type);
  voice.lang = language;
  voice.is_default = (language == default_language_);
  voice_list_.push_back(voice);
}

}  // namespace content
