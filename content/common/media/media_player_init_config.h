// Copyright 2018 Samsung Electronics Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_COMMON_MEDIA_MEDIA_PLAYER_INIT_DATA_H_
#define CONTENT_COMMON_MEDIA_MEDIA_PLAYER_INIT_DATA_H_

#include <string>

#include "content/common/media/media_player_messages_enums_efl.h"
#include "url/gurl.h"

namespace content {

struct MediaPlayerInitConfig {
  MediaPlayerHostMsg_Initialize_Type type;
  GURL url;
  std::string mime_type;
  int demuxer_client_id;
  bool has_encrypted_listener_or_cdm;
};

}  // namespace content

#endif  // CONTENT_COMMON_MEDIA_MEDIA_PLAYER_INIT_DATA_H_
