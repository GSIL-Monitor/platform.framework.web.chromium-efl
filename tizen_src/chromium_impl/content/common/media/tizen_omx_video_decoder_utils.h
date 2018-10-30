// Copyright 2018 Samsung Electronics Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_COMMON_MEDIA_TIZEN_OMX_VIDEO_DECODER_UTILS_H_
#define CONTENT_COMMON_MEDIA_TIZEN_OMX_VIDEO_DECODER_UTILS_H_

#include "OMX_Core.h"
#include "OMX_Video.h"

#include "media/base/video_codecs.h"

namespace content {

OMX_VIDEO_CODINGTYPE VideoCodecToOmxCodingType(const media::VideoCodec codec);

const char* OmxCmdStr(OMX_COMMANDTYPE cmd);
const char* OmxEventStr(OMX_EVENTTYPE ev);
const char* OmxStateStr(OMX_STATETYPE state);
const char* OmxErrStr(OMX_ERRORTYPE err);

}  // namespace content

#endif  // CONTENT_COMMON_MEDIA_TIZEN_OMX_VIDEO_DECODER_UTILS_H_
