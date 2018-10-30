// Copyright (c) 2015 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/ozone/client_native_pixmap_factory_efl.h"

#include "ui/ozone/common/stub_client_native_pixmap_factory.h"

namespace ui {

gfx::ClientNativePixmapFactory* CreateClientNativePixmapFactoryEfl() {
  return CreateStubClientNativePixmapFactory();
}

}  // namespace ui
