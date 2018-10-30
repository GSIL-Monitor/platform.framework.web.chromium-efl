// Copyright 2016 Samsung Electronics Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#if defined(OS_TIZEN_TV_PRODUCT)
#include "common/application_type.h"
#endif

#include "content/browser/speech/tizen_speech_recognition_manager_delegate.h"

#include "content/public/browser/browser_thread.h"


namespace content {

void TizenSpeechRecognitionManagerDelegate::CheckRecognitionIsAllowed(
    int session_id,
    base::OnceCallback<void(bool ask_user, bool is_allowed)> callback) {
  // For tizen, we expect speech recognition to happen when requested.
  // In browser, a pop-up for permission to use microphone will show up.
  // In web app, however, the user agrees on the access to microphone
  // when the app is installed on device. So the pop-up will not show up.
#if defined(OS_TIZEN_TV_PRODUCT)
  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::BindOnce(std::move(callback), IsTIZENWRT(), false));
#else
  BrowserThread::PostTask(BrowserThread::IO, FROM_HERE,
                          base::BindOnce(std::move(callback), true, false));
#endif
}

SpeechRecognitionEventListener*
TizenSpeechRecognitionManagerDelegate::GetEventListener() {
  return NULL;
}

bool TizenSpeechRecognitionManagerDelegate::FilterProfanities(
    int render_process_id) {
  return false;
}

}  // namespace content
