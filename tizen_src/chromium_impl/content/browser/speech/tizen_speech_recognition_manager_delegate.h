// Copyright 2016 Samsung Electronics Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_SPEECH_TIZEN_SPEECH_RECOGNITION_MANAGER_DELEGATE_H_
#define CONTENT_BROWSER_SPEECH_TIZEN_SPEECH_RECOGNITION_MANAGER_DELEGATE_H_

#include "base/bind.h"
#include "content/public/browser/speech_recognition_event_listener.h"
#include "content/public/browser/speech_recognition_manager_delegate.h"

namespace content {

// This delegate is used by the speech recognition manager to
// check for permission to record audio.
class TizenSpeechRecognitionManagerDelegate
    : public SpeechRecognitionManagerDelegate {
 public:
  TizenSpeechRecognitionManagerDelegate() {}
  ~TizenSpeechRecognitionManagerDelegate() override {}

  void CheckRecognitionIsAllowed(
      int session_id,
      base::OnceCallback<void(bool ask_user, bool is_allowed)> callback) override;
  SpeechRecognitionEventListener* GetEventListener() override;
  bool FilterProfanities(int render_process_id) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(TizenSpeechRecognitionManagerDelegate);
};

}  // namespace content

#endif  // CONTENT_BROWSER_SPEECH_TIZEN_SPEECH_RECOGNITION_MANAGER_DELEGATE_H_
