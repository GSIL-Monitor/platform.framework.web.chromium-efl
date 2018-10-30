// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PPAPI_THUNK_AUDIO_CONFIG_API_H_
#define PPAPI_THUNK_AUDIO_CONFIG_API_H_

#include <stdint.h>

#include "ppapi/c/ppb_audio_config.h"
#include "ppapi/thunk/ppapi_thunk_export.h"

#if defined(TIZEN_PEPPER_EXTENSIONS)
#include "ppapi/c/samsung/ppb_audio_config_samsung.h"
#endif

namespace ppapi {
namespace thunk {

class PPAPI_THUNK_EXPORT PPB_AudioConfig_API {
 public:
  virtual ~PPB_AudioConfig_API() {}

  virtual PP_AudioSampleRate GetSampleRate() = 0;
  virtual uint32_t GetSampleFrameCount() = 0;

#if defined(TIZEN_PEPPER_EXTENSIONS)
  virtual int32_t SetAudioMode(PP_AudioMode) = 0;
  virtual PP_AudioMode GetAudioMode() = 0;
#endif
};

}  // namespace thunk
}  // namespace ppapi

#endif  // PPAPI_THUNK_AUDIO_CONFIG_API_H_
