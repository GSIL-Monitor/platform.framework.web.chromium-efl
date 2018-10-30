// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_GFX_TBM_BUFFER_HANDLE_H
#define UI_GFX_TBM_BUFFER_HANDLE_H

#include <stddef.h>

namespace gfx {

struct TbmBufferHandle {
  TbmBufferHandle() : tbm_surface(NULL), media_packet(NULL){};
  void* tbm_surface;
  void* media_packet;
};

}  // namespace gfx

#endif
