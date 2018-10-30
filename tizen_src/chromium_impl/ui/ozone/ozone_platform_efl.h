// Copyright (c) 2015 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef OZONE_OZONE_PLATFORM_EFL_H_
#define OZONE_OZONE_PLATFORM_EFL_H_

namespace ui {

class OzonePlatform;

// Constructor hook for use in ozone_platform_list.cc
OzonePlatform* CreateOzonePlatformEfl();

}  // namespace ui

#endif // OZONE_OZONE_PLATFORM_EFL_H_
