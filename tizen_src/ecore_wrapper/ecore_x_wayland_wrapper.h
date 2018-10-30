// Copyright 2015 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ECORE_X_WAYLAND_WRAPPER_H_
#define ECORE_X_WAYLAND_WRAPPER_H_
#include "chromium_impl/build/tizen_version.h"

#if defined(USE_WAYLAND)
#if TIZEN_VERSION_AT_LEAST(5, 0, 0)
#include <Ecore_Wl2.h>
#else
#include <Ecore_Wayland.h>
#endif  // TIZEN_VERSION_AT_LEAST(5, 0, 0)
#else
#include <Ecore_X.h>
#endif

#endif
