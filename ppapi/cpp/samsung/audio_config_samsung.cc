// Copyright (c) 2017 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ppapi/cpp/samsung/audio_config_samsung.h"

#include "ppapi/c/pp_errors.h"
#include "ppapi/cpp/instance_handle.h"
#include "ppapi/cpp/module.h"
#include "ppapi/cpp/module_impl.h"

namespace pp {

namespace {

template <>
const char* interface_name<PPB_AudioConfig_Samsung_1_0>() {
  return PPB_AUDIO_CONFIG_SAMSUNG_INTERFACE_1_0;
}

}  // namespace

AudioConfigSamsung::AudioConfigSamsung() : AudioConfig() {}

AudioConfigSamsung::AudioConfigSamsung(const InstanceHandle& instance,
                                       PP_AudioSampleRate sample_rate,
                                       uint32_t sample_frame_count)
    : AudioConfig(instance, sample_rate, sample_frame_count) {}

int32_t AudioConfigSamsung::SetAudioMode(PP_AudioMode audio_mode) {
  if (has_interface<PPB_AudioConfig_Samsung_1_0>()) {
    return get_interface<PPB_AudioConfig_Samsung_1_0>()->SetAudioMode(
        pp_resource(), audio_mode);
  }
  return PP_ERROR_NOINTERFACE;
}

}  // namespace pp
