// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/base/cdm_context.h"

namespace media {

CdmContext::CdmContext() {}

CdmContext::~CdmContext() {}

void IgnoreCdmAttached(bool /* success */) {}

void* CdmContext::GetClassIdentifier() const {
  return nullptr;
}

#if defined(OS_TIZEN_TV_PRODUCT)
void CdmContext::SetPlayerType(MediaPlayerHostMsg_Initialize_Type player_type) {
  player_type_ = player_type;
}

MediaPlayerHostMsg_Initialize_Type CdmContext::GetPlayerType() const {
  return player_type_;
}
#endif

}  // namespace media
