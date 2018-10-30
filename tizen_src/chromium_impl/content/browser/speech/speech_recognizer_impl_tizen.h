// Copyright 2016 Samsung Electronics Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_SPEECH_SPEECH_RECOGNIZER_IMPL_TIZEN_
#define CONTENT_BROWSER_SPEECH_SPEECH_RECOGNIZER_IMPL_TIZEN_

#include <vector>

#include "base/location.h"
#include "base/strings/string16.h"
#include "content/browser/speech/speech_recognizer.h"
#include "content/public/common/speech_recognition_result.h"
#if defined(OS_TIZEN_TV_PRODUCT)
#include <stt-wrapper.h>
#else
#include <stt.h>
#endif

namespace content {

class SpeechRecognitionEventListener;
struct SpeechRecognitionError;

class CONTENT_EXPORT SpeechRecognizerImplTizen : public SpeechRecognizer {
 public:
  SpeechRecognizerImplTizen(SpeechRecognitionEventListener* listener,
                            int session_id);

  // SpeechRecognizer methods.
  void StartRecognition(const std::string& device_id) override;
  void AbortRecognition() override;
  void StopAudioCapture() override;
  bool IsActive() const override;
  bool IsCapturingAudio() const override;

 private:
  ~SpeechRecognizerImplTizen() override;

  bool Initialize();
  void StartInternal();
  void Destroy();
  stt_state_e GetCurrentState() const;
  bool ShouldStartRecognition() const;
  void RecognitionResults(bool no_speech,
                          const SpeechRecognitionResults& results);
  void HandleSttError(stt_error_e error, const base::Location& location);
  void HandleSttResultError(const std::string error);
  void RecognitionStarted();
  void AudioCaptureEnd();
  void EndRecognition(const base::Location& location);

  // Callbacks
  static bool OnSupportedLanguages(stt_h stt,
                                   const char* language,
                                   void* user_data);
  static void OnRecognitionResult(stt_h stt,
                                  stt_result_event_e event,
                                  const char** data,
                                  int data_count,
                                  const char* msg,
                                  void* user_data);
  static void OnStateChange(stt_h stt,
                            stt_state_e previous,
                            stt_state_e current,
                            void* user_data);
  static void OnError(stt_h stt, stt_error_e reason, void* user_data);

  bool IsAborted() const { return is_aborted_; }

  stt_h handle_;
  std::string language_;
  bool is_lang_supported_;
  bool continuous_;
  const char* recognition_type_;
  bool is_aborted_;
};

}  // namespace content

#endif  // BROWSER_SPEECH_SPEECH_RECOGNIZER_IMPL_TIZEN_
