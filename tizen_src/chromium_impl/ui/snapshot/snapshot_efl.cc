// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/snapshot/snapshot.h"

#include "base/bind.h"
#include "base/callback.h"
#include "base/logging.h"
#include "base/numerics/safe_conversions.h"
#include "base/task_runner_util.h"
#include "ui/display/display.h"
#include "ui/gfx/image/image.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/display/screen.h"

namespace ui {

bool GrabViewSnapshot(gfx::NativeView view,
                      const gfx::Rect& snapshot_bounds,
                      gfx::Image* image) {
  return GrabWindowSnapshot(view, snapshot_bounds, image);
}

bool GrabWindowSnapshot(gfx::NativeWindow window,
                        const gfx::Rect& snapshot_bounds,
                        gfx::Image* image) {
  // Not supported in EFL port. Callers should fall back to the async version.
  return false;
}

void GrabWindowSnapshotAsync(gfx::NativeWindow window,
                             const gfx::Rect& source_rect,
                             const GrabWindowSnapshotAsyncCallback& callback) {
  NOTIMPLEMENTED();
}

void GrabViewSnapshotAsync(gfx::NativeView view,
                           const gfx::Rect& source_rect,
                           const GrabWindowSnapshotAsyncCallback& callback) {
  GrabWindowSnapshotAsync(view, source_rect, callback);
}

}  // namespace ui
