// Copyright 2017 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RENDERER_HOST_PEPPER_MEDIA_PLAYER_TIZEN_PEPPER_PLAYER_UTILS_TIZEN_H_
#define CONTENT_BROWSER_RENDERER_HOST_PEPPER_MEDIA_PLAYER_TIZEN_PEPPER_PLAYER_UTILS_TIZEN_H_

#include <cstring>
#include <string>
#include <vector>

#include "content/browser/renderer_host/pepper/media_player/tizen/pepper_player_adapter_base.h"

namespace content {

namespace pepper_player_utils {

bool IsValid(PP_ElementaryStream_Type_Samsung type);

std::string ToString(PP_ElementaryStream_Type_Samsung type);
std::string ToHexString(size_t, const uint8_t* data);
std::string ToHexString(const std::vector<uint8_t>& data);
std::string ToHexString(size_t, const void* data);

unsigned GetChannelsCount(PP_ChannelLayout_Samsung);

const char* GetCodecMimeType(PP_VideoCodec_Type_Samsung);
const char* GetCodecMimeType(PP_AudioCodec_Type_Samsung);

unsigned GetCodecVersion(PP_VideoCodec_Type_Samsung);
unsigned GetCodecVersion(PP_AudioCodec_Type_Samsung);

void GetSampleFormat(PP_SampleFormat_Samsung pp_format,
                     unsigned* width,
                     unsigned* depth,
                     unsigned* endianness,
                     bool* signedness);

// NOLINTNEXTLINE(runtime/int)
PP_TimeTicks MilisecondsToPPTimeTicks(int mm_player_time_ticks_ms);

// NOLINTNEXTLINE(runtime/int)
PP_TimeTicks MilisecondsToPPTimeTicks(unsigned long mm_player_time_ticks_ms);

PP_TimeTicks NanosecondsToPPTimeTicks(
    unsigned long long mm_player_time_ticks_ns /* NOLINT(runtime/int) */);

template <size_t N>
inline void CopyStringToArray(char (&dst)[N], const std::string& src) {
  strncpy(dst, src.c_str(), std::min(N, src.size()));
  dst[N - 1] = '\0';
}

}  // namespace pepper_player_utils

}  // namespace content

#endif  // CONTENT_BROWSER_RENDERER_HOST_PEPPER_MEDIA_PLAYER_TIZEN_PEPPER_PLAYER_UTILS_TIZEN_H_
