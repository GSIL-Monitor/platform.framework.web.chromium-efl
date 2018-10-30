// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Define OpenNewXDisplay and GetXDisplay to some unused name to be able to
// replace it with our own implementation.
#define GetXDisplay  GetXDisplayOverride

#include "ui/gfx/x/x11_types.cc"

#undef GetXDisplay

#include <Ecore_X.h>

namespace gfx {

XDisplay* GetXDisplay() {
  static Display* display = static_cast<Display*>(ecore_x_display_get());
  return display;
}

}
