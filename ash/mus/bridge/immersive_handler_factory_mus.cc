// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/mus/bridge/immersive_handler_factory_mus.h"

#include <memory>

#include "ash/public/cpp/immersive/immersive_focus_watcher.h"
#include "ash/public/cpp/immersive/immersive_gesture_handler.h"
#include "base/logging.h"

namespace ash {
namespace mus {

ImmersiveHandlerFactoryMus::ImmersiveHandlerFactoryMus() {}

ImmersiveHandlerFactoryMus::~ImmersiveHandlerFactoryMus() {}

std::unique_ptr<ImmersiveFocusWatcher>
ImmersiveHandlerFactoryMus::CreateFocusWatcher(
    ImmersiveFullscreenController* controller) {
  // ImmersiveFocusWatcher does two interesting things:
  // . It listens for focus in the top view and triggers reveal.
  // . It watches for transient windows that are bubbles anchored to the
  //   top view.
  // ImmersiveFullscreenController, when used in mus, does not have any
  // focusable views in the top view and the client can not create bubbles
  // anchored to the top view (the client does not have access to the top view).
  // This means ImmersiveFocusWatcher is not applicable here, and null is
  // returned.
  return nullptr;
}

std::unique_ptr<ImmersiveGestureHandler>
ImmersiveHandlerFactoryMus::CreateGestureHandler(
    ImmersiveFullscreenController* controller) {
  // http://crbug.com/640394.
  NOTIMPLEMENTED();
  return nullptr;
}

}  // namespace mus
}  // namespace ash
