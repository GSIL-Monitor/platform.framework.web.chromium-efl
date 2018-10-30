// Copyright (c) 2014 Samsung Electronics Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/common/tts_utterance_request_efl.h"

namespace content {

TtsUtteranceRequest::TtsUtteranceRequest()
    : id(0),
      volume(1.0),
      rate(1.0),
      pitch(1.0) {
}

TtsVoice::TtsVoice()
    : local_service(true),
      is_default(false) {
}

TtsVoice::TtsVoice(std::string voice_uri, std::string name, std::string lang,
                   bool local_service, bool is_default)
    : voice_uri(voice_uri),
      name(name),
      lang(lang),
      local_service(local_service),
      is_default(is_default) {
}

TtsUtteranceResponse::TtsUtteranceResponse()
    : id(0) {
}

}  // namespace content
