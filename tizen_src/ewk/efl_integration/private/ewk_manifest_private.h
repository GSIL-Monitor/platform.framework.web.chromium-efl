// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ewk_manifest_private_h
#define ewk_manifest_private_h

#include <Eina.h>
#include <string>
#include <vector>

#include "base/strings/utf_string_conversions.h"
#include "content/public/common/manifest.h"
#include "public/ewk_manifest.h"

/// Represents current's page manifest information.
struct _Ewk_View_Request_Manifest {
  explicit _Ewk_View_Request_Manifest(const content::Manifest& manifest);
  ~_Ewk_View_Request_Manifest();

  std::string short_name;
  std::string name;
  std::string start_url;
  std::string spp_sender_id;
  Ewk_View_Orientation_Type
      orientation_type; /**> orientation type of web screen */
  Ewk_View_Web_Display_Mode web_display_mode; /**> display mode of web screen */
  int64_t theme_color;
  int64_t background_color;

  struct Size {
    int width;
    int height;
  };

  struct Icon {
    Icon();
    ~Icon();

    std::string src;
    std::string type;
    std::vector<Size> sizes;
  };
  std::vector<Icon> icons;
};
#endif  // ewk_manifest_private_h
