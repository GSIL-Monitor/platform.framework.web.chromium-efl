// Copyright 2016 Samsung Electronics Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/speech/speech_recognizer_impl_tizen.h"

#include "base/strings/utf_string_conversions.h"
#include "content/browser/speech/speech_recognizer_impl.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/speech_recognition_event_listener.h"
#include "content/public/browser/speech_recognition_manager.h"
#include "content/public/browser/speech_recognition_session_config.h"

#define RUN_ON_BROWSER_IO_THREAD(METHOD, ...)                                \
  do {                                                                       \
    if (!BrowserThread::CurrentlyOn(BrowserThread::IO)) {                    \
      BrowserThread::PostTask(BrowserThread::IO, FROM_HERE,                  \
                              base::Bind(&SpeechRecognizerImplTizen::METHOD, \
                                         this, ##__VA_ARGS__));              \
      return;                                                                \
    }                                                                        \
  } while (0)

#define RUN_ON_BROWSER_UI_THREAD(METHOD, ...)                                \
  do {                                                                       \
    if (!BrowserThread::CurrentlyOn(BrowserThread::UI)) {                    \
      BrowserThread::PostTask(BrowserThread::UI, FROM_HERE,                  \
                              base::Bind(&SpeechRecognizerImplTizen::METHOD, \
                                         this, ##__VA_ARGS__));              \
      return;                                                                \
    }                                                                        \
  } while (0)

#define ENUM_CASE(x) \
  case x:            \
    return #x;       \
    break

#if !defined(OS_TIZEN_TV_PRODUCT)
#define stt_wrapper_get_error_message stt_get_error_message
#define stt_wrapper_get_state stt_get_state
#define stt_wrapper_destroy stt_destroy
#define stt_wrapper_unset_error_cb stt_unset_error_cb
#define stt_wrapper_unset_state_changed_cb stt_unset_state_changed_cb
#define stt_wrapper_unset_recognition_result_cb stt_unset_recognition_result_cb
#define stt_wrapper_unprepare stt_unprepare
#define stt_wrapper_cancel stt_cancel
#define stt_wrapper_start stt_start
#define stt_wrapper_set_error_cb stt_set_error_cb
#define stt_wrapper_set_state_changed_cb stt_set_state_changed_cb
#define stt_wrapper_set_recognition_result_cb stt_set_recognition_result_cb
#define stt_wrapper_create stt_create
#define stt_wrapper_stop stt_stop
#define stt_wrapper_prepare stt_prepare
#define stt_wrapper_foreach_supported_languages stt_foreach_supported_languages
#endif

namespace {

// Replace hyphen "-" in language with underscore "_". CAPI does not
// support language string with hyphen. Example: en-US -> en_US.
std::string GetLanguageString(const std::string& language) {
  std::string ret(language);
  std::replace(ret.begin(), ret.end(), '-', '_');
  return ret;
}

const char* GetErrorString(stt_error_e error) {
  switch (error) {
    ENUM_CASE(STT_ERROR_NONE);
    ENUM_CASE(STT_ERROR_OUT_OF_MEMORY);
    ENUM_CASE(STT_ERROR_IO_ERROR);
    ENUM_CASE(STT_ERROR_INVALID_PARAMETER);
    ENUM_CASE(STT_ERROR_TIMED_OUT);
    ENUM_CASE(STT_ERROR_RECORDER_BUSY);
    ENUM_CASE(STT_ERROR_OUT_OF_NETWORK);
    ENUM_CASE(STT_ERROR_PERMISSION_DENIED);
    ENUM_CASE(STT_ERROR_NOT_SUPPORTED);
    ENUM_CASE(STT_ERROR_INVALID_STATE);
    ENUM_CASE(STT_ERROR_INVALID_LANGUAGE);
    ENUM_CASE(STT_ERROR_ENGINE_NOT_FOUND);
    ENUM_CASE(STT_ERROR_OPERATION_FAILED);
    ENUM_CASE(STT_ERROR_NOT_SUPPORTED_FEATURE);
    ENUM_CASE(STT_ERROR_RECORDING_TIMED_OUT);
    ENUM_CASE(STT_ERROR_NO_SPEECH);
    ENUM_CASE(STT_ERROR_IN_PROGRESS_TO_READY);
    ENUM_CASE(STT_ERROR_IN_PROGRESS_TO_RECORDING);
    ENUM_CASE(STT_ERROR_IN_PROGRESS_TO_PROCESSING);
    ENUM_CASE(STT_ERROR_SERVICE_RESET);
  };

  NOTREACHED() << "Invalid stt_error_e! (" << error << ")";
  return "";
}

}  // namespace

namespace content {

SpeechRecognitionErrorCode GetErrorCode(stt_error_e reason) {
  switch (reason) {
    case STT_ERROR_NONE:
      return SPEECH_RECOGNITION_ERROR_NONE;
    case STT_ERROR_NO_SPEECH:
      return SPEECH_RECOGNITION_ERROR_NO_SPEECH;
    case STT_ERROR_RECORDING_TIMED_OUT:
    case STT_ERROR_IO_ERROR:
    case STT_ERROR_RECORDER_BUSY:
      return SPEECH_RECOGNITION_ERROR_AUDIO_CAPTURE;
    case STT_ERROR_OUT_OF_NETWORK:
      return SPEECH_RECOGNITION_ERROR_NETWORK;
    case STT_ERROR_PERMISSION_DENIED:
      return SPEECH_RECOGNITION_ERROR_NOT_ALLOWED;
    case STT_ERROR_INVALID_LANGUAGE:
      return SPEECH_RECOGNITION_ERROR_LANGUAGE_NOT_SUPPORTED;
    case STT_ERROR_NOT_SUPPORTED:
      return SPEECH_RECOGNITION_ERROR_SERVICE_NOT_ALLOWED;
    default:
      return SPEECH_RECOGNITION_ERROR_ABORTED;
  }
}

scoped_refptr<SpeechRecognizer> CreateSpeechRecognizerTizen(
    SpeechRecognitionEventListener* listener,
    int session_id) {
  return new SpeechRecognizerImplTizen(listener, session_id);
}

SpeechRecognizerImplTizen::SpeechRecognizerImplTizen(
    SpeechRecognitionEventListener* listener,
    int session_id)
    : SpeechRecognizer(listener, session_id),
      handle_(NULL),
      language_(""),
      is_lang_supported_(false),
      continuous_(false),
      recognition_type_(""),
      is_aborted_(false) {}

SpeechRecognizerImplTizen::~SpeechRecognizerImplTizen() {
  Destroy();
}

void SpeechRecognizerImplTizen::StartRecognition(const std::string& device_id) {
  RUN_ON_BROWSER_UI_THREAD(StartRecognition, device_id);
  LOG(INFO) << "stt session #" << session_id() << " start recognition";

  if (!Initialize())
    return;

  SpeechRecognitionSessionConfig config =
      SpeechRecognitionManager::GetInstance()->GetSessionConfig(session_id());

  recognition_type_ = config.interim_results ? STT_RECOGNITION_TYPE_FREE_PARTIAL
                                             : STT_RECOGNITION_TYPE_FREE;
  continuous_ = config.continuous;

  if (config.language.empty()) {
    LOG(ERROR) << "stt session #" << session_id()
               << " language config is empty! Use 'en_US'";
    is_lang_supported_ = true;
    language_ = "en_US";
  } else {
    language_ = GetLanguageString(config.language);
  }

  stt_wrapper_foreach_supported_languages(handle_, &OnSupportedLanguages, this);

  if (!is_lang_supported_) {
    HandleSttError(STT_ERROR_INVALID_LANGUAGE, FROM_HERE);
    return;
  }

  int err = stt_wrapper_prepare(handle_);
  if (STT_ERROR_NONE != err)
    HandleSttError(static_cast<stt_error_e>(err), FROM_HERE);
}

void SpeechRecognizerImplTizen::AbortRecognition() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  EndRecognition(FROM_HERE);
}

void SpeechRecognizerImplTizen::StopAudioCapture() {
  RUN_ON_BROWSER_UI_THREAD(StopAudioCapture);
  continuous_ = false;

  if (GetCurrentState() != STT_STATE_RECORDING)
    return;

  int err = stt_wrapper_stop(handle_);
  if (STT_ERROR_NONE != err)
    HandleSttError(static_cast<stt_error_e>(err), FROM_HERE);
}

bool SpeechRecognizerImplTizen::IsActive() const {
  return GetCurrentState() > STT_STATE_CREATED ? true : false;
}

bool SpeechRecognizerImplTizen::IsCapturingAudio() const {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  return GetCurrentState() >= STT_STATE_RECORDING ? true : false;
}

bool SpeechRecognizerImplTizen::Initialize() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  is_aborted_ = false;

  int err = stt_wrapper_create(&handle_);
  if (err != STT_ERROR_NONE) {
    HandleSttError(static_cast<stt_error_e>(err), FROM_HERE);
    return false;
  }

  err = stt_wrapper_set_recognition_result_cb(handle_, &OnRecognitionResult,
                                              this);
  if (err != STT_ERROR_NONE) {
    HandleSttError(static_cast<stt_error_e>(err), FROM_HERE);
    return false;
  }

  err = stt_wrapper_set_state_changed_cb(handle_, &OnStateChange, this);
  if (err != STT_ERROR_NONE) {
    HandleSttError(static_cast<stt_error_e>(err), FROM_HERE);
    return false;
  }

  err = stt_wrapper_set_error_cb(handle_, &OnError, this);
  if (err != STT_ERROR_NONE) {
    HandleSttError(static_cast<stt_error_e>(err), FROM_HERE);
    return false;
  }
  return true;
}

void SpeechRecognizerImplTizen::StartInternal() {
  RUN_ON_BROWSER_UI_THREAD(StartInternal);

  LOG(INFO) << "stt session #" << session_id()
            << " language used : " << language_.c_str()
            << " type : " << recognition_type_;

  int err = stt_wrapper_start(handle_, language_.c_str(), recognition_type_);
  if (STT_ERROR_NONE != err)
    HandleSttError(static_cast<stt_error_e>(err), FROM_HERE);
}

void SpeechRecognizerImplTizen::Destroy() {
  RUN_ON_BROWSER_UI_THREAD(Destroy);
  if (!handle_)
    return;

  is_lang_supported_ = false;
  continuous_ = false;
  if (GetCurrentState() >= STT_STATE_RECORDING)
    stt_wrapper_cancel(handle_);
  if (GetCurrentState() == STT_STATE_READY)
    stt_wrapper_unprepare(handle_);
  if (GetCurrentState() == STT_STATE_CREATED) {
    stt_wrapper_unset_recognition_result_cb(handle_);
    stt_wrapper_unset_state_changed_cb(handle_);
    stt_wrapper_unset_error_cb(handle_);
  }

  // Valid states to be called: created, Ready, Recording, Processing
  int err = stt_wrapper_destroy(handle_);
  if (err != STT_ERROR_NONE)
    LOG(ERROR) << "stt session #" << session_id() << " stt_destroy failed with "
               << GetErrorString(static_cast<stt_error_e>(err));
  else
    handle_ = NULL;
}

bool SpeechRecognizerImplTizen::ShouldStartRecognition() const {
  return GetCurrentState() >= STT_STATE_READY;
}

// There is no default stt state. So use |STT_STATE_CREATED|
// This will work as we will use it to determine audio capturing.
stt_state_e SpeechRecognizerImplTizen::GetCurrentState() const {
  if (!handle_ || is_aborted_)
    return STT_STATE_CREATED;

  stt_state_e state = STT_STATE_CREATED;
  int err = stt_wrapper_get_state(handle_, &state);
  if (err != STT_ERROR_NONE)
    LOG(ERROR) << "stt session #" << session_id() << " stt_get_state FAILED!";
  return state;
}

void SpeechRecognizerImplTizen::RecognitionResults(
    bool no_speech,
    const SpeechRecognitionResults& results) {
  RUN_ON_BROWSER_IO_THREAD(RecognitionResults, no_speech, results);

  if (is_aborted_) {
    LOG(INFO) << "stt engine is in error state";
    return;
  }

  if (no_speech && !continuous_) {
    HandleSttError(STT_ERROR_NO_SPEECH, FROM_HERE);
    return;
  }

  listener()->OnRecognitionResults(session_id(), results);
}

void SpeechRecognizerImplTizen::HandleSttError(stt_error_e error,
                                               const base::Location& location) {
  RUN_ON_BROWSER_IO_THREAD(HandleSttError, error, location);
  LOG(ERROR) << "stt session #" << session_id() << " error "
             << GetErrorString(error) << " from " << location.function_name()
             << "(" << location.line_number() << ")";

  if (!handle_ || is_aborted_)
    return;

  listener()->OnRecognitionError(
      session_id(),
      SpeechRecognitionError(GetErrorCode(static_cast<stt_error_e>(error))));
  EndRecognition(FROM_HERE);
}

void SpeechRecognizerImplTizen::HandleSttResultError(const std::string error) {
  RUN_ON_BROWSER_IO_THREAD(HandleSttResultError, error);
  LOG(ERROR) << "stt session #" << session_id() << " no result as " << error;

  if (!handle_ || is_aborted_)
    return;

  if (error == STT_RESULT_MESSAGE_ERROR_TOO_SOON ||
      error == STT_RESULT_MESSAGE_ERROR_TOO_SHORT ||
      error == STT_RESULT_MESSAGE_ERROR_TOO_LONG ||
      error == STT_RESULT_MESSAGE_ERROR_TOO_QUIET ||
      error == STT_RESULT_MESSAGE_ERROR_TOO_LOUD ||
      error == STT_RESULT_MESSAGE_ERROR_TOO_FAST) {
    listener()->OnRecognitionError(
        session_id(),
        SpeechRecognitionError(SPEECH_RECOGNITION_ERROR_NO_MATCH));
  }
}

void SpeechRecognizerImplTizen::RecognitionStarted() {
  RUN_ON_BROWSER_IO_THREAD(RecognitionStarted);

  listener()->OnAudioStart(session_id());
  listener()->OnSoundStart(session_id());
  listener()->OnRecognitionStart(session_id());
}

void SpeechRecognizerImplTizen::AudioCaptureEnd() {
  RUN_ON_BROWSER_IO_THREAD(AudioCaptureEnd);

  listener()->OnSoundEnd(session_id());
  listener()->OnAudioEnd(session_id());
}

void SpeechRecognizerImplTizen::EndRecognition(const base::Location& location) {
  RUN_ON_BROWSER_IO_THREAD(EndRecognition, location);

  LOG(INFO) << "stt session #" << session_id()
            << " ending recognition called from " << location.function_name()
            << "(" << location.line_number() << ")";

  if (is_aborted_)
    return;

  is_aborted_ = true;
  if (GetCurrentState() == STT_STATE_RECORDING)
    AudioCaptureEnd();

  listener()->OnRecognitionEnd(session_id());
  Destroy();
}

bool SpeechRecognizerImplTizen::OnSupportedLanguages(stt_h stt,
                                                     const char* language,
                                                     void* user_data) {
  SpeechRecognizerImplTizen* speech =
      static_cast<SpeechRecognizerImplTizen*>(user_data);
  if (!speech)
    return false;

  if (speech->language_.compare(language) == 0) {
    speech->is_lang_supported_ = true;
    return false;
  }
  return true;
}

void SpeechRecognizerImplTizen::OnRecognitionResult(stt_h stt,
                                                    stt_result_event_e event,
                                                    const char** data,
                                                    int data_count,
                                                    const char* msg,
                                                    void* user_data) {
  SpeechRecognizerImplTizen* speech =
      static_cast<SpeechRecognizerImplTizen*>(user_data);
  if (!speech)
    return;

  if (speech->IsAborted()) {
    LOG(INFO) << "stt engine is in error state";
    return;
  }

  if (event == STT_RESULT_EVENT_ERROR) {
    speech->HandleSttResultError(msg);
    return;
  }

  bool no_speech = true;
  SpeechRecognitionResults results;
  results.push_back(SpeechRecognitionResult());
  SpeechRecognitionResult& result = results.back();
  for (int i = 0; i < data_count; i++) {
    if (data[i] != NULL && data[i][0] != ' ')
      no_speech = false;
    else
      continue;
    base::string16 utterance;
    base::UTF8ToUTF16(data[i], strlen(data[i]), &utterance);
    result.hypotheses.push_back(SpeechRecognitionHypothesis(utterance, 1.0));
  }

  if (event == STT_RESULT_EVENT_FINAL_RESULT)
    result.is_provisional = false;
  else
    result.is_provisional = true;

  speech->RecognitionResults(no_speech, results);
}

void SpeechRecognizerImplTizen::OnStateChange(stt_h stt,
                                              stt_state_e previous,
                                              stt_state_e current,
                                              void* user_data) {
  SpeechRecognizerImplTizen* speech =
      static_cast<SpeechRecognizerImplTizen*>(user_data);
  if (!speech)
    return;

  if (speech->IsAborted()) {
    LOG(INFO) << "stt engine in error state";
    return;
  }

  LOG(INFO) << "stt session #" << speech->session_id()
            << " state change called. Previous : " << previous
            << " Current : " << current;

  switch (current) {
    case STT_STATE_READY:
      switch (previous) {
        case STT_STATE_CREATED:
          speech->StartInternal();
          break;
        case STT_STATE_RECORDING:
          if (!speech->is_aborted_)
            speech->HandleSttError(STT_ERROR_NO_SPEECH, FROM_HERE);
          break;
        case STT_STATE_PROCESSING:
          // If continuous attribute is true recognition shouldn't stop after
          // receiving one final result. So start the recognition again.
          if (speech->continuous_)
            speech->StartInternal();
          // |stt_recognition_result_cb| is called even when a partial result is
          // obtained. So, need to end recognition only when the state change
          // is confirmed.
          else
            speech->EndRecognition(FROM_HERE);
          break;
        default:
          break;
      }
      break;
    case STT_STATE_RECORDING:
      speech->RecognitionStarted();
      break;
    case STT_STATE_PROCESSING:
      speech->AudioCaptureEnd();
      break;
    default:
      break;
  }
}

void SpeechRecognizerImplTizen::OnError(stt_h stt,
                                        stt_error_e reason,
                                        void* user_data) {
  SpeechRecognizerImplTizen* speech =
      static_cast<SpeechRecognizerImplTizen*>(user_data);
  if (speech) {
#if defined(OS_TIZEN_TV_PRODUCT)
    char* error_msg = NULL;
    const std::string no_speech_msg = "voice_engine.error.no_speech";
    int ret = stt_wrapper_get_error_message(stt, &error_msg);
    if (ret == STT_ERROR_NONE) {
      LOG(INFO) << "stt session #" << speech->session_id()
                << "err msg : " << error_msg;
      if (!no_speech_msg.compare(error_msg))
        reason = STT_ERROR_NO_SPEECH;
      if (error_msg)
        free(error_msg);
    } else {
      LOG(ERROR) << "stt session #" << speech->session_id()
                 << " stt_wrapper_get_error_message:"
                 << GetErrorString(static_cast<stt_error_e>(ret));
    }
#endif

    speech->HandleSttError(reason, FROM_HERE);
  }
}

}  // namespace content
