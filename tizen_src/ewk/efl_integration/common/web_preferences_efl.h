// Copyright (c) 2014, 2015 Samsung Electronics Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEB_PREFERENCES_EFL_H
#define WEB_PREFERENCES_EFL_H

#include "tizen/system_info.h"

// Contains settings from Ewk_Settings that do not belong to content::WebPreferences
// and need to be sent to renderer.
struct WebPreferencesEfl {
  bool shrinks_viewport_content_to_fit =
      IsMobileProfile() || IsWearableProfile() ? true : false;
  bool javascript_can_open_windows_automatically_ewk = true;
  bool hw_keyboard_connected = false;
};

#endif // WEB_PREFERENCES_EFL_H
