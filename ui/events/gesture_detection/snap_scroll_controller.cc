// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/events/gesture_detection/snap_scroll_controller.h"

#include <algorithm>
#include <cmath>

#include "ui/events/gesture_detection/motion_event.h"

namespace ui {
namespace {

// Size of the snap rail relative to the initial snap bound threshold.
const float kSnapBoundToChannelMultiplier = 1.5f;

float CalculateChannelDistance(float snap_bound,
                               const gfx::SizeF& display_size) {
  const float kMinChannelDistance = snap_bound * kSnapBoundToChannelMultiplier;
  const float kMaxChannelDistance = kMinChannelDistance * 3.f;
  const float kSnapChannelDipsPerScreenDip = kMinChannelDistance / 480.f;
  if (display_size.IsEmpty())
    return kMinChannelDistance;

  float screen_size =
      std::abs(hypot(static_cast<float>(display_size.width()),
                     static_cast<float>(display_size.height())));

  float snap_channel_distance = screen_size * kSnapChannelDipsPerScreenDip;
  return std::max(kMinChannelDistance,
                  std::min(kMaxChannelDistance, snap_channel_distance));
}

}  // namespace

SnapScrollController::SnapScrollController(float snap_bound,
                                           const gfx::SizeF& display_size)
    : snap_bound_(snap_bound),
      channel_distance_(CalculateChannelDistance(snap_bound, display_size)),
      mode_(SNAP_NONE) {
  // The distance based approach may result in wrong scroll direction.
  // And, due to patent issue, it could be risky to use a linear function.
  // Thus, the quadratic function based approach is propossed as below.
  // Coefficients for the quadratic function:
  // a*dx^2 + c = dy.
  // a and c are named coeff_a_ and coeff_c_, respectively.
  // And, they can be experimentally determined.
  // From internal tests, this function is considered that pass through
  // (0, snap_bound/2) and (snap_bound/2, snap_bound).
  coeff_a_ = 2.f / snap_bound;
  coeff_c_ = snap_bound / 2.f;
}

SnapScrollController::~SnapScrollController() {
}

void SnapScrollController::SetSnapScrollMode(
    const MotionEvent& event,
    bool is_scale_gesture_detection_in_progress) {
  switch (event.GetAction()) {
    case MotionEvent::ACTION_DOWN:
      mode_ = SNAP_PENDING;
      down_position_.set_x(event.GetX());
      down_position_.set_y(event.GetY());
      break;
    case MotionEvent::ACTION_MOVE: {
      if (is_scale_gesture_detection_in_progress)
        break;

      if (mode_ != SNAP_PENDING)
        break;

      // Set scrolling mode to SNAP_X if scroll exceeds |snap_bound_| and the
      // ratio of x movement to y movement is sufficiently large. Similarly for
      // SNAP_Y and y movement.
      float dx = std::abs(event.GetX() - down_position_.x());
      float dy = std::abs(event.GetY() - down_position_.y());
      if (dx * dx + dy * dy > snap_bound_ * snap_bound_) {
        if (coeff_a_ * dy * dy + coeff_c_ < dx)
          mode_ = SNAP_HORIZ;
        else if (coeff_a_ * dx * dx + coeff_c_ < dy)
          mode_ = SNAP_VERT;
        else
          mode_ = SNAP_NONE;
      }
    } break;
    case MotionEvent::ACTION_UP:
    case MotionEvent::ACTION_CANCEL:
      down_position_ = gfx::PointF();
      accumulated_distance_ = gfx::Vector2dF();
      break;
    default:
      break;
  }
}

void SnapScrollController::UpdateSnapScrollMode(float distance_x,
                                                float distance_y) {
  if (!IsSnappingScrolls())
    return;

  accumulated_distance_ +=
      gfx::Vector2dF(std::abs(distance_x), std::abs(distance_y));
  if (mode_ == SNAP_HORIZ) {
    if (accumulated_distance_.y() > channel_distance_)
      mode_ = SNAP_NONE;
    else if (accumulated_distance_.x() > channel_distance_)
      accumulated_distance_ = gfx::Vector2dF();
  } else if (mode_ == SNAP_VERT) {
    if (accumulated_distance_.x() > channel_distance_)
      mode_ = SNAP_NONE;
    else if (accumulated_distance_.y() > channel_distance_)
      accumulated_distance_ = gfx::Vector2dF();
  }
}

bool SnapScrollController::IsSnapVertical() const {
  return mode_ == SNAP_VERT;
}

bool SnapScrollController::IsSnapHorizontal() const {
  return mode_ == SNAP_HORIZ;
}

bool SnapScrollController::IsSnappingScrolls() const {
  return IsSnapHorizontal() || IsSnapVertical();
}

}  // namespace ui
