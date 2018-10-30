// Copyright (c) 2014 Samsung Electronics Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BROWSER_SPEECH_TTS_TIZEN_H_
#define BROWSER_SPEECH_TTS_TIZEN_H_

#include "build/tizen_version.h"

typedef int tts_voice_type_e;

#include "content/browser/speech/tts_message_filter_efl.h"
#include "content/common/tts_utterance_request_efl.h"

#include <tts.h>
#include <vector>

namespace content {
class TtsMessageFilterEfl;

// TtsTizen is the class that actually communicate with tts engine.
// It handles tts instance, callbacks and callback returns
class TtsTizen {
 public:
  TtsTizen(TtsMessageFilterEfl* tts_message_filter_efl);
  ~TtsTizen();
  bool Init(tts_mode_e tts_mode = TTS_MODE_DEFAULT);

  // Return voice list
  const std::vector<TtsVoice>& GetVoiceList();
  void Speak(const TtsUtteranceRequest& utterance);
  void Pause();
  void Resume();
  void Cancel();
  tts_h GetTTS() { return tts_handle_; }
  TtsMessageFilterEfl* GetTtsMessageFilterEfl() {
    return tts_message_filter_efl_; }
  const TtsUtteranceRequest& GetUtterance() const { return utterance_; }
  void TtsReady();

  // Get voices one by one from tts engine, append them into a voice list
  void AddVoice(std::string language, tts_voice_type_e name);
  bool IsTtsInitialized() const { return tts_initialized_; }
  void ChangeTtsDefaultVoice(const std::string, const int);

 private:
  // Create and Set tts and tts callback. If any of these failes, tts will not work at all.
  bool SetTtsCallback();
  void StoreTtsDefaultVoice();
  void SpeakStoredUtterance();
  TtsMessageFilterEfl* tts_message_filter_efl_;
  TtsUtteranceRequest utterance_;
  tts_h tts_handle_;
  std::string default_language_;
  tts_voice_type_e default_voice_;

  // This variable stores list of voicees that tts engine support.
  std::vector<TtsVoice> voice_list_;

  // This variable check whether tts is initialized.
  // It will be true when tts engine set the variable tts_.
  bool tts_initialized_;

  // This variable check whether tts is ready, after initialized.
  // Initial value is false and it will be true when tts state change to
  // TTS_STATE_CREATED && TTS_STATE_READY.
  bool tts_state_ready_;

  // This variable check whether any utterance is currently wating to speak.
  bool utterance_waiting_;
};

}  // namespace content

#endif // BROWSER_SPEECH_TTS_TIZEN_H_
