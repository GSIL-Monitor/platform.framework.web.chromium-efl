// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "renderer/tizen_extensible.h"

/* LCOV_EXCL_START */
TizenExtensible::TizenExtensible() :
    extensible_api_table_ {
        { "background,music", true },
        { "background,vibration", false },
        { "block,multimedia,on,call", false},
        { "csp", false },
        { "encrypted,database", false },
        { "fullscreen", false },
        { "mediastream,record", false },
        { "media,volume,control", false },
        { "prerendering,for,rotation", false },
        { "rotate,camera,view", true },
        { "rotation,lock", false },
        { "sound,mode", false },
        { "support,fullscreen", true },
        { "visibility,suspend", false },
        { "xwindow,for,fullscreen,video", false },
        { "support,multimedia", true },
    },
    initialized_(false) {
}

TizenExtensible::~TizenExtensible() {
  extensible_api_table_.clear();
}

void TizenExtensible::UpdateTizenExtensible(
    const std::map<std::string, bool>& params) {
  extensible_api_table_.clear();
  extensible_api_table_ = params;
  initialized_ = true;
}

bool TizenExtensible::SetExtensibleAPI(const std::string& api_name,
                                       bool enable) {
  auto it = extensible_api_table_.find(api_name);
  if (it == extensible_api_table_.end())
    return false;
  it->second = enable;

  return true;
}

bool TizenExtensible::GetExtensibleAPI(const std::string& api_name) const {
  if (!initialized_)
    return false;

  auto it = extensible_api_table_.find(api_name);
  if (it == extensible_api_table_.end())
    return false;

  return it->second;
}
/* LCOV_EXCL_STOP */
