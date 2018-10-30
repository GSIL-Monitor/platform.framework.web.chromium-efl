// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/palette/palette_utils.h"

#include "ash/palette_delegate.h"
#include "ash/public/cpp/stylus_utils.h"
#include "ash/session/session_controller.h"
#include "ash/shelf/shelf.h"
#include "ash/shell.h"
#include "ash/system/palette/palette_tray.h"
#include "ash/system/status_area_widget.h"
#include "ui/display/display.h"
#include "ui/gfx/geometry/point.h"

namespace ash {
namespace palette_utils {

bool ShouldShowPalette() {
  return stylus_utils::HasStylusInput() &&
         (display::Display::HasInternalDisplay() ||
          stylus_utils::IsPaletteEnabledOnEveryDisplay()) &&
         Shell::Get()->palette_delegate() &&
         Shell::Get()->palette_delegate()->ShouldShowPalette();
}

bool PaletteContainsPointInScreen(const gfx::Point& point) {
  for (aura::Window* window : Shell::GetAllRootWindows()) {
    PaletteTray* palette_tray =
        Shelf::ForWindow(window)->GetStatusAreaWidget()->palette_tray();
    if (palette_tray && palette_tray->ContainsPointInScreen(point))
      return true;
  }

  return false;
}

bool IsInUserSession() {
  SessionController* session_controller = Shell::Get()->session_controller();
  return !session_controller->IsUserSessionBlocked() &&
         session_controller->GetSessionState() ==
             session_manager::SessionState::ACTIVE &&
         !session_controller->IsRunningInAppMode();
}

}  // namespace palette_utils
}  // namespace ash
