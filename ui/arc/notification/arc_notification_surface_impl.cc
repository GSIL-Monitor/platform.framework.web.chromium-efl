// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/arc/notification/arc_notification_surface_impl.h"

#include "base/logging.h"
#include "components/exo/notification_surface.h"
#include "components/exo/surface.h"
#include "ui/aura/window.h"
#include "ui/aura/window_delegate.h"
#include "ui/base/cursor/cursor.h"
#include "ui/base/hit_test.h"
#include "ui/views/controls/native/native_view_host.h"
#include "ui/views/widget/widget.h"

namespace arc {

namespace {

class CustomWindowDelegate : public aura::WindowDelegate {
 public:
  explicit CustomWindowDelegate(exo::NotificationSurface* notification_surface)
      : notification_surface_(notification_surface) {}
  ~CustomWindowDelegate() override {}

  // Overridden from aura::WindowDelegate:
  gfx::Size GetMinimumSize() const override { return gfx::Size(); }
  gfx::Size GetMaximumSize() const override { return gfx::Size(); }
  void OnBoundsChanged(const gfx::Rect& old_bounds,
                       const gfx::Rect& new_bounds) override {}
  gfx::NativeCursor GetCursor(const gfx::Point& point) override {
    return notification_surface_->GetCursor(point);
  }
  int GetNonClientComponent(const gfx::Point& point) const override {
    return HTNOWHERE;
  }
  bool ShouldDescendIntoChildForEventHandling(
      aura::Window* child,
      const gfx::Point& location) override {
    return true;
  }
  bool CanFocus() override { return true; }
  void OnCaptureLost() override {}
  void OnPaint(const ui::PaintContext& context) override {}
  void OnDeviceScaleFactorChanged(float old_device_scale_factor,
                                  float new_device_scale_factor) override {}
  void OnWindowDestroying(aura::Window* window) override {}
  void OnWindowDestroyed(aura::Window* window) override { delete this; }
  void OnWindowTargetVisibilityChanged(bool visible) override {}
  bool HasHitTestMask() const override {
    return notification_surface_->HasHitTestMask();
  }
  void GetHitTestMask(gfx::Path* mask) const override {
    notification_surface_->GetHitTestMask(mask);
  }
  void OnKeyEvent(ui::KeyEvent* event) override {
    // Propagates the key event upto the top-level views Widget so that we can
    // trigger proper events in the views/ash level there. Event handling for
    // Surfaces is done in a post event handler in keyboard.cc.
    views::Widget* widget = views::Widget::GetTopLevelWidgetForNativeView(
        notification_surface_->host_window());
    if (widget)
      widget->OnKeyEvent(event);
  }

 private:
  exo::NotificationSurface* const notification_surface_;

  DISALLOW_COPY_AND_ASSIGN(CustomWindowDelegate);
};

}  // namespace

// Handles notification surface role of a given surface.
ArcNotificationSurfaceImpl::ArcNotificationSurfaceImpl(
    exo::NotificationSurface* surface)
    : surface_(surface) {
  DCHECK(surface);
  native_view_ =
      base::MakeUnique<aura::Window>(new CustomWindowDelegate(surface));
  native_view_->SetType(aura::client::WINDOW_TYPE_CONTROL);
  native_view_->set_owned_by_parent(false);
  native_view_->Init(ui::LAYER_NOT_DRAWN);
  native_view_->AddChild(surface_->host_window());
  native_view_->Show();
}

ArcNotificationSurfaceImpl::~ArcNotificationSurfaceImpl() = default;

gfx::Size ArcNotificationSurfaceImpl::GetSize() const {
  return surface_->GetContentSize();
}

void ArcNotificationSurfaceImpl::Attach(
    views::NativeViewHost* native_view_host) {
  DCHECK(!native_view_host_);
  DCHECK(native_view_host);
  native_view_host_ = native_view_host;
  native_view_host->Attach(native_view_.get());
}

void ArcNotificationSurfaceImpl::Detach() {
  DCHECK(native_view_host_);
  DCHECK_EQ(native_view_.get(), native_view_host_->native_view());
  native_view_host_->Detach();
  native_view_host_ = nullptr;
}

bool ArcNotificationSurfaceImpl::IsAttached() const {
  return native_view_host_;
}

views::NativeViewHost* ArcNotificationSurfaceImpl::GetAttachedHost() const {
  return native_view_host_;
}

aura::Window* ArcNotificationSurfaceImpl::GetWindow() const {
  return native_view_.get();
}

aura::Window* ArcNotificationSurfaceImpl::GetContentWindow() const {
  DCHECK(surface_->host_window());
  return surface_->host_window();
}

const std::string& ArcNotificationSurfaceImpl::GetNotificationKey() const {
  return surface_->notification_key();
}

void ArcNotificationSurfaceImpl::FocusSurfaceWindow() {
  DCHECK(surface_->root_surface());
  DCHECK(surface_->root_surface()->window());

  // Focus the surface window manually to handle key events for notification.
  // Message center is unactivatable by default, but we make it activatable when
  // user is about to use Direct Reply. In that case, we also need to focus the
  // surface window manually to send events to Android.
  return surface_->root_surface()->window()->Focus();
}

void ArcNotificationSurfaceImpl::SetAXTreeId(int32_t ax_tree_id) {
  ax_tree_id_ = ax_tree_id;
}

int32_t ArcNotificationSurfaceImpl::GetAXTreeId() const {
  return ax_tree_id_;
}

}  // namespace arc
