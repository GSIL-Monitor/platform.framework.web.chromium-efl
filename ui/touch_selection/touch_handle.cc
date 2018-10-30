// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/touch_selection/touch_handle.h"
#include "base/metrics/histogram_macros.h"

#include <algorithm>
#include <cmath>

namespace ui {

namespace {

#if defined(S_TERRACE_SUPPORT)
// Maximum duration of a fade sequence.
const double kFadeDurationMs = 100;
// duration of a scale show sequence (0 to 105 in 166 ms).
const double kScaleDurationMs = 166;
// duration of a scale hide sequence (100 to 0 in 233ms)
const double kHideDurationMs = 233;
// Max size scale up the handler in show
const float kMaxHandlerScaleSize = 1.05f;
// Maximum duration of a Touch handle scale up & down sequence
const double kMaxZoomedScaleDurationMs = 330;
// For Sine.InOut Maximum Zoomed scale for selection handler
const float kMaxZoomedHandlerScaleSize = 1.5f;
// Radius to ignore the touch handle move with drag.
const float kIgnoreRadiusForHandleMove = 0.5f;
// Maximum duration of a Touch handle to animate on path
const double kMaxPathAnimationDurationMs = 300;
#else
// Maximum duration of a fade sequence.
const double kFadeDurationMs = 200;
#endif

// Maximum amount of travel for a fade sequence. This avoids handle "ghosting"
// when the handle is moving rapidly while the fade is active.
const double kFadeDistanceSquared = 20.f * 20.f;

// Avoid using an empty touch rect, as it may fail the intersection test event
// if it lies within the other rect's bounds.
const float kMinTouchMajorForHitTesting = 1.f;

// The maximum touch size to use when computing whether a touch point is
// targetting a touch handle. This is necessary for devices that misreport
// touch radii, preventing inappropriately largely touch sizes from completely
// breaking handle dragging behavior.
const float kMaxTouchMajorForHitTesting = 36.f;

// Note that the intersection region is boundary *exclusive*.
bool RectIntersectsCircle(const gfx::RectF& rect,
                          const gfx::PointF& circle_center,
                          float circle_radius) {
  DCHECK_GT(circle_radius, 0.f);
  // An intersection occurs if the closest point between the rect and the
  // circle's center is less than the circle's radius.
  gfx::PointF closest_point_in_rect(circle_center);
  closest_point_in_rect.SetToMax(rect.origin());
  closest_point_in_rect.SetToMin(rect.bottom_right());

  gfx::Vector2dF distance = circle_center - closest_point_in_rect;
  return distance.LengthSquared() < (circle_radius * circle_radius);
}

}  // namespace

// TODO(AviD): Remove this once logging(DCHECK) supports enum class.
static std::ostream& operator<<(std::ostream& os,
                                const TouchHandleOrientation& orientation) {
  switch (orientation) {
    case TouchHandleOrientation::LEFT:
      return os << "LEFT";
    case TouchHandleOrientation::RIGHT:
      return os << "RIGHT";
    case TouchHandleOrientation::CENTER:
      return os << "CENTER";
    case TouchHandleOrientation::UNDEFINED:
      return os << "UNDEFINED";
    default:
      return os << "INVALID: " << static_cast<int>(orientation);
  }
}

// Responsible for rendering a selection or insertion handle for text editing.
TouchHandle::TouchHandle(TouchHandleClient* client,
                         TouchHandleOrientation orientation,
                         const gfx::RectF& viewport_rect)
    : drawable_(client->CreateDrawable()),
      client_(client),
      viewport_rect_(viewport_rect),
      orientation_(orientation),
      deferred_orientation_(TouchHandleOrientation::UNDEFINED),
      alpha_(0.f),
      animate_deferred_fade_(false),
#if defined(S_TERRACE_SUPPORT)
      scale_(0.f),
      is_hide_animation_in_progress_(false),
      is_orientation_changed_(false),
      is_handle_moving_with_touch_(false),
      is_path_animation_in_progress_(false),
      x_speed_top_bound_(0.f),
      x_speed_bottom_bound_(0.f),
      y_speed_bottom_bound_(0.f),
      y_speed_top_bound_(0.f),
      touch_handle_state_(TOUCH_HANDLE_STATE_UNDEFINED),
#endif
      enabled_(true),
      is_visible_(false),
      is_dragging_(false),
      is_drag_within_tap_region_(false),
      is_handle_layout_update_required_(false),
      mirror_vertical_(false),
      mirror_horizontal_(false) {
  DCHECK_NE(orientation, TouchHandleOrientation::UNDEFINED);
  drawable_->SetEnabled(enabled_);
  drawable_->SetOrientation(orientation_, false, false);
  drawable_->SetOrigin(focus_bottom_);
  drawable_->SetAlpha(alpha_);
#if defined(S_TERRACE_SUPPORT)
  drawable_->SetScale(scale_);
#endif
  handle_horizontal_padding_ = drawable_->GetDrawableHorizontalPaddingRatio();
}

TouchHandle::~TouchHandle() {
}

void TouchHandle::SetEnabled(bool enabled) {
  if (enabled_ == enabled)
    return;
#if defined(S_TERRACE_SUPPORT)
  enabled_ = enabled;
  if(orientation_ == TouchHandleOrientation::CENTER) {
    drawable_->SetEnabled(enabled);
    return;
  }
  if (enabled) {
    drawable_->SetEnabled(enabled);
    if (is_hide_animation_in_progress_)
      EndHideAnimation();
  }
  else {
    EndDrag();
    BeginAnimationOnHide();
  }
#else
  if (!enabled) {
    SetVisible(false, ANIMATION_NONE);
    EndDrag();
    EndFade();
  }
  enabled_ = enabled;
  drawable_->SetEnabled(enabled);
#endif
}

void TouchHandle::SetVisible(bool visible, AnimationStyle animation_style) {
  DCHECK(enabled_);
  if (is_visible_ == visible)
    return;

  is_visible_ = visible;
  // Handle repositioning may have been deferred while previously invisible.
  if (visible)
    SetUpdateLayoutRequired();

  bool animate = animation_style != ANIMATION_NONE;
  if (is_dragging_) {
    animate_deferred_fade_ = animate;
    return;
  }

#if defined(S_TERRACE_SUPPORT)
  if (!visible || orientation_ == TouchHandleOrientation::CENTER || !animate) {
    EndFade();
    EndScale();
  } else if (animate) {
    BeginFade();
    BeginScale();
    client_->SetNeedsAnimate();
  }
#else
  if (animate)
    BeginFade();
  else
    EndFade();
#endif
}

void TouchHandle::SetFocus(const gfx::PointF& top, const gfx::PointF& bottom) {
  DCHECK(enabled_);
  if (focus_top_ == top && focus_bottom_ == bottom)
    return;

  focus_top_ = top;
  focus_bottom_ = bottom;
  SetUpdateLayoutRequired();
}

void TouchHandle::SetViewportRect(const gfx::RectF& viewport_rect) {
  DCHECK(enabled_);
  if (viewport_rect_ == viewport_rect)
    return;

  viewport_rect_ = viewport_rect;
  SetUpdateLayoutRequired();
}

void TouchHandle::SetOrientation(TouchHandleOrientation orientation) {
  DCHECK(enabled_);
  DCHECK_NE(orientation, TouchHandleOrientation::UNDEFINED);
#if defined(S_TERRACE_SUPPORT)
  // No animation for mirrored orientation.
  if (mirror_horizontal_)
    is_orientation_changed_ = false;
  else {
    // Defer orientation for both the handles if not mirrored.
    deferred_orientation_ = orientation;
    return;
  }
#endif

  if (is_dragging_) {
    deferred_orientation_ = orientation;
    return;
  }
#if !defined(S_TERRACE_SUPPORT)
  DCHECK_EQ(deferred_orientation_, TouchHandleOrientation::UNDEFINED);
#endif
  if (orientation_ == orientation)
    return;

  orientation_ = orientation;
  SetUpdateLayoutRequired();
}

bool TouchHandle::WillHandleTouchEvent(const MotionEvent& event) {
  if (!enabled_)
    return false;

  if (!is_dragging_ && event.GetAction() != MotionEvent::ACTION_DOWN)
    return false;

  switch (event.GetAction()) {
    case MotionEvent::ACTION_DOWN: {
      if (!is_visible_)
        return false;
      const gfx::PointF touch_point(event.GetX(), event.GetY());
      const float touch_radius = std::max(
          kMinTouchMajorForHitTesting,
          std::min(kMaxTouchMajorForHitTesting, event.GetTouchMajor())) * 0.5f;
      const gfx::RectF drawable_bounds = drawable_->GetVisibleBounds();
#if defined(S_TERRACE_SUPPORT)
      // Store touch down handle bounds. This is needed to ignore the move
      float width = drawable_bounds.width()
        * (1.0f - handle_horizontal_padding_);
      float x_offset = 0.f;
      // x offset is needed only for left orintation handle to position the
      // x point at the handle starting position after the padding
      if (orientation_ == ui::TouchHandleOrientation::LEFT)
        x_offset = width;

      touch_down_drawable_bounds_ = gfx::RectF(
        focus_bottom_.x() - x_offset,
        drawable_bounds.y(),
        width,
        drawable_bounds.height());
#endif
      // Only use the touch radius for targetting if the touch is at or below
      // the drawable area. This makes it easier to interact with the line of
      // text above the drawable.
      if (touch_point.y() < drawable_bounds.y() ||
          !RectIntersectsCircle(drawable_bounds, touch_point, touch_radius)) {
        EndDrag();
        return false;
      }
      touch_down_position_ = touch_point;
      touch_drag_offset_ = focus_bottom_ - touch_down_position_;
#if defined(S_TERRACE_SUPPORT)
      if (orientation_ != ui::TouchHandleOrientation::CENTER) {
        float x_adjustment = drawable_bounds.width()
          * (1.0f - handle_horizontal_padding_) / 2;
        float x_drag = 0.f;
        if (orientation_ == ui::TouchHandleOrientation::RIGHT)
        x_drag = focus_bottom_.x() + x_adjustment;
        else if (orientation_ == ui::TouchHandleOrientation::LEFT)
          x_drag = focus_bottom_.x() - x_adjustment;
        float y_drag = focus_bottom_.y()
          + (drawable_bounds.height() * kMaxZoomedHandlerScaleSize) / 2;
        touch_drag_offset_ = focus_bottom_ - gfx::PointF(x_drag, y_drag);
      }
#endif
      touch_down_time_ = event.GetEventTime();
      BeginDrag();
#if defined(S_TERRACE_SUPPORT)
      if (orientation_ != ui::TouchHandleOrientation::CENTER)
        StartHandleZoomIn();
#endif
    } break;

    case MotionEvent::ACTION_MOVE: {
      gfx::PointF touch_move_position(event.GetX(), event.GetY());
      is_drag_within_tap_region_ &=
          client_->IsWithinTapSlop(touch_down_position_ - touch_move_position);

#if defined(S_TERRACE_SUPPORT)
      // Code for : Move Touch Handle with finger
      // To set focus for the dragged touch handle to follow touch points.
      if (orientation_ != ui::TouchHandleOrientation::CENTER) {
        if (!RectIntersectsCircle(touch_down_drawable_bounds_,
            touch_move_position, kIgnoreRadiusForHandleMove)) {
          touch_down_drawable_bounds_ = gfx::RectF(); //Reset
          float drawable_width = drawable_->GetVisibleBounds().width();
          float drawable_height = drawable_->GetVisibleBounds().height();
          float x_offset = drawable_width
            * (1.0f - handle_horizontal_padding_) / 2;
          float y_offset = (drawable_height * kMaxZoomedHandlerScaleSize) / 2;

          if (orientation_ == ui::TouchHandleOrientation::RIGHT)
            last_touch_move_position_.set_x(touch_move_position.x() - x_offset);
          else
            last_touch_move_position_.set_x(touch_move_position.x() + x_offset);
          last_touch_move_position_.set_y(touch_move_position.y() - y_offset);
          is_handle_moving_with_touch_ = true;
          SetFocus(last_touch_move_position_, last_touch_move_position_);
        } else
          SetFocus(bound_focus_top_, bound_focus_bottom_);
        UpdateHandleLayout();
      }
#endif
      // Note that we signal drag update even if we're inside the tap region,
      // as there are cases where characters are narrower than the slop length.
      client_->OnDragUpdate(*this, touch_move_position + touch_drag_offset_);
    } break;

    case MotionEvent::ACTION_UP: {
      if (is_drag_within_tap_region_ &&
          (event.GetEventTime() - touch_down_time_) <
              client_->GetMaxTapDuration()) {
        client_->OnHandleTapped(*this);
      }

      EndDrag();
    } break;

    case MotionEvent::ACTION_CANCEL:
      EndDrag();
      break;

    default:
      break;
  };
  return true;
}

bool TouchHandle::IsActive() const {
  return is_dragging_;
}

bool TouchHandle::Animate(base::TimeTicks frame_time) {
#if defined(S_TERRACE_SUPPORT)
  if (touch_handle_state_ == TOUCH_HANDLE_STATE_ZOOM_IN) { // Handler scale up
    if (scale_ >= kMaxZoomedHandlerScaleSize) {
      scale_end_time_ = base::TimeTicks();
      return false;
    }
    float scale_u = kMaxZoomedHandlerScaleSize -
        ((scale_end_time_ - frame_time).InMillisecondsF() /
        kMaxZoomedScaleDurationMs) * kMaxZoomedHandlerScaleSize;
    scale_u = 1.f + scale_u;
    if (scale_u >= kMaxZoomedHandlerScaleSize) {
      scale_end_time_ = base::TimeTicks();
      SetScale(1.5f);
      return false;
    }
    SetScale(scale_u);
    return true;
  }
  else if (touch_handle_state_ == TOUCH_HANDLE_STATE_ZOOM_OUT) {
    // Handler scale down & animate on path
    bool is_zoom_in_progress = false;
    if (scale_ <= 1.f) {
      EndScale();
      EndFade();
    } else {
      float scale_u = kMaxZoomedHandlerScaleSize -
          ((scale_end_time_ - frame_time).InMillisecondsF() /
          kMaxZoomedScaleDurationMs) * kMaxZoomedHandlerScaleSize;
      scale_u = 1.5f - scale_u;
      if (scale_u <= 1.f) {
        EndScale();
        EndFade();
      } else {
        SetScale(scale_u);
        is_zoom_in_progress = true;
      }
    }
    // Is path animation in progress
    bool is_animating_on_path = AnimateOnPath(frame_time);

    // Call end drag once handle in proper place.
    if (!is_animating_on_path)
      client_->OnDragEnd(*this);

    if (!is_animating_on_path && !is_zoom_in_progress) {
      touch_handle_state_ = TOUCH_HANDLE_STATE_UNDEFINED;
      return false;
    }
    return true;
  }

  if (std::max(fade_end_time_, scale_end_time_) == base::TimeTicks())
    return false;
  if (enabled_) { // Animation on show
    float time_u =
        1.f - (fade_end_time_ - frame_time).InMillisecondsF() / kFadeDurationMs;

    float position_u = (focus_bottom_ - fade_start_position_).LengthSquared() /
        kFadeDistanceSquared;
    float u = std::max(time_u, position_u);
    SetAlpha(is_visible_ ? u : 1.f - u);

    if (u >= 1.f)
      EndFade();

    float scale_u = kMaxHandlerScaleSize -
        ((scale_end_time_ - frame_time).InMillisecondsF() /
        kScaleDurationMs) * kMaxHandlerScaleSize;

    if (scale_u >= kMaxHandlerScaleSize) {
      float scale_bounce_u =
          kMaxHandlerScaleSize -
          (1 - ((scale_bounce_end_time_ - frame_time).InMillisecondsF() /
          kScaleDurationMs)) * kMaxHandlerScaleSize;
      SetScale(scale_bounce_u);

      if (scale_bounce_u <= 1) {
        EndScale();
        is_handle_moving_with_touch_ = false;
        return false;
      }
    } else {
      SetScale(scale_u);
    }
    return true;
  }
  // Animation on hide
  float scale_u =
      (scale_end_time_ - frame_time).InMillisecondsF() / kHideDurationMs;

  SetAlpha(is_visible_ ? scale_u : 0.f);
  SetScale(is_visible_ ? scale_u : 0.f);

  if (scale_u <= 0.f) {
    EndHideAnimation();
    return false;
  }
#else
  if (fade_end_time_ == base::TimeTicks())
    return false;

  DCHECK(enabled_);

  float time_u =
      1.f - (fade_end_time_ - frame_time).InMillisecondsF() / kFadeDurationMs;
  float position_u = (focus_bottom_ - fade_start_position_).LengthSquared() /
                     kFadeDistanceSquared;
  float u = std::max(time_u, position_u);
  SetAlpha(is_visible_ ? u : 1.f - u);

  if (u >= 1.f) {
    EndFade();
    return false;
  }
#endif
  return true;
}

gfx::RectF TouchHandle::GetVisibleBounds() const {
  if (!is_visible_ || !enabled_)
    return gfx::RectF();

  return drawable_->GetVisibleBounds();
}

void TouchHandle::UpdateHandleLayout() {
  // Suppress repositioning a handle while invisible or fading out to prevent it
  // from "ghosting" outside the visible bounds. The position will be pushed to
  // the drawable when the handle regains visibility (see |SetVisible()|).
  if (!is_visible_ || !is_handle_layout_update_required_)
    return;

  is_handle_layout_update_required_ = false;

  // Update mirror values only when dragging has stopped to prevent unwanted
  // inversion while dragging of handles.
  if (!is_dragging_) {
#if !defined(S_TERRACE_SUPPORT)
    gfx::RectF handle_bounds = drawable_->GetVisibleBounds();
    bool mirror_horizontal = false;
    bool mirror_vertical = false;

    const float handle_width =
        handle_bounds.width() * (1.0 - handle_horizontal_padding_);

    // Disabled as vertical flipping of handle in not supported in SBrowser UX
    const float handle_height = handle_bounds.height();

    const float bottom_y_unmirrored =
        focus_bottom_.y() + handle_height + viewport_rect_.y();
    const float top_y_mirrored =
        focus_top_.y() - handle_height + viewport_rect_.y();

    const float bottom_y_clipped =
        std::max(bottom_y_unmirrored - viewport_rect_.bottom(), 0.f);
    const float top_y_clipped =
        std::max(viewport_rect_.y() - top_y_mirrored, 0.f);

    mirror_vertical = top_y_clipped < bottom_y_clipped;

    const float best_y_clipped =
        mirror_vertical ? top_y_clipped : bottom_y_clipped;

    UMA_HISTOGRAM_PERCENTAGE(
        "Event.TouchSelectionHandle.BottomHandleClippingPercentage",
        static_cast<int>((bottom_y_clipped / handle_height) * 100));
    UMA_HISTOGRAM_PERCENTAGE(
        "Event.TouchSelectionHandle.BestVerticalClippingPercentage",
        static_cast<int>((best_y_clipped / handle_height) * 100));
    UMA_HISTOGRAM_BOOLEAN(
        "Event.TouchSelectionHandle.ShouldFlipHandleVertically",
        mirror_vertical);
    UMA_HISTOGRAM_PERCENTAGE(
        "Event.TouchSelectionHandle.FlippingImprovementPercentage",
        static_cast<int>(((bottom_y_clipped - best_y_clipped) / handle_height) *
                         100));

    if (orientation_ == TouchHandleOrientation::LEFT) {
      const float left_x_clipped = std::max(
          viewport_rect_.x() - (focus_bottom_.x() - handle_width), 0.f);
      UMA_HISTOGRAM_PERCENTAGE(
          "Event.TouchSelectionHandle.LeftHandleClippingPercentage",
          static_cast<int>((left_x_clipped / handle_height) * 100));
      if (left_x_clipped > 0)
        mirror_horizontal = true;
    } else if (orientation_ == TouchHandleOrientation::RIGHT) {
      const float right_x_clipped = std::max(
          (focus_bottom_.x() + handle_width) - viewport_rect_.right(), 0.f);
      UMA_HISTOGRAM_PERCENTAGE(
          "Event.TouchSelectionHandle.RightHandleClippingPercentage",
          static_cast<int>((right_x_clipped / handle_height) * 100));
      if (right_x_clipped > 0)
        mirror_horizontal = true;
    }
    if (client_->IsAdaptiveHandleOrientationEnabled()) {
      mirror_horizontal_ = mirror_horizontal;
      mirror_vertical_ = mirror_vertical;
    }
#endif
#if defined(S_TERRACE_SUPPORT)
    if (mirror_horizontal_ && !is_handle_moving_with_touch_) {
      // Change the handle orientation when mirroring
      // of touch handles is true at the edge of screen.
      if(orientation_ == TouchHandleOrientation::LEFT) {
        SetOrientation(TouchHandleOrientation::RIGHT);
      } else if(orientation_ == TouchHandleOrientation::RIGHT) {
        SetOrientation(TouchHandleOrientation::LEFT);
      }
    }
#endif
  }

  drawable_->SetOrientation(orientation_, mirror_vertical_, mirror_horizontal_);
  drawable_->SetOrigin(ComputeHandleOrigin());

#if defined(S_TERRACE_SUPPORT)
  if (is_orientation_changed_) {
    is_orientation_changed_ = false;
    PositionUpdatedHandle();
  }

#endif
}

gfx::PointF TouchHandle::ComputeHandleOrigin() const {
  gfx::PointF focus = mirror_vertical_ ? focus_top_ : focus_bottom_;
  gfx::RectF drawable_bounds = drawable_->GetVisibleBounds();
  float drawable_width = drawable_->GetVisibleBounds().width();

  // Calculate the focal offsets from origin for the handle drawable
  // based on the orientation.
  int focal_offset_x = 0;
  int focal_offset_y = mirror_vertical_ ? drawable_bounds.height() : 0;
  switch (orientation_) {
#if !defined(S_TERRACE_SUPPORT)
    case ui::TouchHandleOrientation::LEFT:
      focal_offset_x =
          mirror_horizontal_
              ? drawable_width * handle_horizontal_padding_
              : drawable_width * (1.0f - handle_horizontal_padding_);
      break;
    case ui::TouchHandleOrientation::RIGHT:
      focal_offset_x =
          mirror_horizontal_
              ? drawable_width * (1.0f - handle_horizontal_padding_)
              : drawable_width * handle_horizontal_padding_;
      break;
#else
    case ui::TouchHandleOrientation::LEFT:
      focal_offset_x = drawable_width * (1.0f - handle_horizontal_padding_);
      break;
    case ui::TouchHandleOrientation::RIGHT:
      focal_offset_x = drawable_width * handle_horizontal_padding_;
      break;
#endif
    case ui::TouchHandleOrientation::CENTER:
      focal_offset_x = drawable_width * 0.5f;
      break;
    case ui::TouchHandleOrientation::UNDEFINED:
      NOTREACHED() << "Invalid touch handle orientation.";
      break;
  };

  return focus - gfx::Vector2dF(focal_offset_x, focal_offset_y);
}

void TouchHandle::BeginDrag() {
  DCHECK(enabled_);
  if (is_dragging_)
    return;
  EndFade();
  is_dragging_ = true;
  is_drag_within_tap_region_ = true;
  client_->OnDragBegin(*this, focus_bottom());
}

void TouchHandle::EndDrag() {
#if !defined(S_TERRACE_SUPPORT)
  DCHECK(enabled_);
#endif
  if (!is_dragging_)
    return;

  is_dragging_ = false;
  is_drag_within_tap_region_ = false;
#if defined(S_TERRACE_SUPPORT)
  if (orientation_ == TouchHandleOrientation::CENTER)
    client_->OnDragEnd(*this);
  else {
    // Defer the drag end till animation is in progress.
    client_->HideMagnifier();
    BeginAnimateAndSetFocus(final_top_, final_bottom_);
    if (!enabled_)
      client_->OnDragEnd(*this);
  }
#else
  client_->OnDragEnd(*this);
#endif

#if defined(S_TERRACE_SUPPORT)
  // Deferring not needed for mirrored orientation
  if (mirror_horizontal_
      && deferred_orientation_ != TouchHandleOrientation::UNDEFINED) {
#else
  if (deferred_orientation_ != TouchHandleOrientation::UNDEFINED) {
#endif
    TouchHandleOrientation deferred_orientation = deferred_orientation_;
    deferred_orientation_ = TouchHandleOrientation::UNDEFINED;
    SetOrientation(deferred_orientation);
    // Handle layout may be deferred while the handle is dragged.
    SetUpdateLayoutRequired();
    UpdateHandleLayout();
  }

  if (animate_deferred_fade_) {
    BeginFade();
  } else {
    // As drawable visibility assignment is deferred while dragging, push the
    // change by forcing fade completion.
    EndFade();
  }
}

void TouchHandle::BeginFade() {
  DCHECK(enabled_);
  DCHECK(!is_dragging_);
  animate_deferred_fade_ = false;
  const float target_alpha = is_visible_ ? 1.f : 0.f;
  if (target_alpha == alpha_) {
    EndFade();
    return;
  }

  fade_end_time_ = base::TimeTicks::Now() +
                   base::TimeDelta::FromMillisecondsD(
                       kFadeDurationMs * std::abs(target_alpha - alpha_));
  fade_start_position_ = focus_bottom_;
#if !defined(S_TERRACE_SUPPORT)
  client_->SetNeedsAnimate();
#endif
}

void TouchHandle::EndFade() {
#if !defined(S_TERRACE_SUPPORT)
  DCHECK(enabled_);
#endif
  animate_deferred_fade_ = false;
  fade_end_time_ = base::TimeTicks();
  SetAlpha(is_visible_ ? 1.f : 0.f);
}

#if defined(S_TERRACE_SUPPORT)
void TouchHandle::SetDeferredOrientation() {
  if (mirror_horizontal_)
    return;

  if (deferred_orientation_ != TouchHandleOrientation::UNDEFINED) {
    TouchHandleOrientation deferred_orientation = deferred_orientation_;
    deferred_orientation_ = TouchHandleOrientation::UNDEFINED;

    if (orientation_ == deferred_orientation)
      return;

    orientation_ = deferred_orientation;
    // Setting change in orientation to perform animation
    is_orientation_changed_ = true;
    // Handle layout may be deferred while the handle is dragged.
    SetUpdateLayoutRequired();
    UpdateHandleLayout();
  }
}

void TouchHandle::RemoveImmediate() {
  enabled_ = false;
  SetAlpha(0.f);
  SetScale(0.f);
  EndHideAnimation();
}

void TouchHandle::PositionUpdatedHandle() {
}

void TouchHandle::BeginScale() {
  is_handle_moving_with_touch_ = false; // Reset
  // Position handler at proper place if not already
  // This is only needed for swap case.
  if (touch_handle_state_ == TOUCH_HANDLE_STATE_ZOOM_OUT) {
    SetFocus(final_top_, final_bottom_);
    // Handle layout may be deferred while the handle is dragged.
    SetUpdateLayoutRequired();
    UpdateHandleLayout();
    // Confirm drag end while swap.
    client_->OnDragEnd(*this);
  }
  // Reset to initial handle zoomed state
  touch_handle_state_ = TOUCH_HANDLE_STATE_UNDEFINED;
  scale_end_time_ = base::TimeTicks::Now() +
      base::TimeDelta::FromMillisecondsD(kScaleDurationMs);
  scale_bounce_end_time_ =
      scale_end_time_ + base::TimeDelta::FromMillisecondsD(kScaleDurationMs);
  drawable_->SetOriginForScale();
}

void TouchHandle::EndScale() {
  scale_end_time_ = base::TimeTicks();
  scale_bounce_end_time_ = base::TimeTicks();
  SetScale(1.f);
}

void TouchHandle::BeginAnimationOnHide() {
  SetScale(1.f);
  is_hide_animation_in_progress_ = true;
  touch_handle_state_ = TOUCH_HANDLE_STATE_UNDEFINED;
  scale_end_time_ = base::TimeTicks::Now() +
      base::TimeDelta::FromMillisecondsD(kHideDurationMs);
  client_->SetNeedsAnimate();
}

void TouchHandle::EndHideAnimation() {
  scale_end_time_ = base::TimeTicks::Now();
  drawable_->SetEnabled(enabled_);
  is_visible_ = false;
  is_hide_animation_in_progress_ = false;
}

bool TouchHandle::IsHideAnimationInProgress() {
  return is_hide_animation_in_progress_;
}

void TouchHandle::SetScale(float scale) {
// Changes for handler scale up
  if(touch_handle_state_ == TOUCH_HANDLE_STATE_ZOOM_IN
      || touch_handle_state_ == TOUCH_HANDLE_STATE_ZOOM_OUT)
    scale = std::max(0.f, std::min(kMaxZoomedHandlerScaleSize, scale));
  else
    scale = std::max(0.f, std::min(kMaxHandlerScaleSize, scale));
  if (scale_ == scale)
    return;
  scale_ = scale;
  drawable_->SetScale(scale);
}

void TouchHandle::StartHandleZoomIn() {
  if (touch_handle_state_ == TOUCH_HANDLE_STATE_ZOOM_IN)
    return;
  touch_handle_state_ = TOUCH_HANDLE_STATE_ZOOM_IN;
  scale_end_time_ = base::TimeTicks::Now() +
      base::TimeDelta::FromMillisecondsD(kMaxZoomedScaleDurationMs);
  client_->SetNeedsAnimate();
}
void TouchHandle::StartHandleZoomOut() {
  if (touch_handle_state_ == TOUCH_HANDLE_STATE_ZOOM_OUT
      || touch_handle_state_ == TOUCH_HANDLE_STATE_UNDEFINED)
    return;
  touch_handle_state_ = TOUCH_HANDLE_STATE_ZOOM_OUT;
  scale_end_time_ = base::TimeTicks::Now() +
      base::TimeDelta::FromMillisecondsD(kMaxZoomedScaleDurationMs);
  client_->SetNeedsAnimate();
}

void TouchHandle::BeginAnimateAndSetFocus(
      const gfx::PointF& top, const gfx::PointF& bottom) {
  if (is_handle_moving_with_touch_ == false)
    return;
  touch_handle_state_ = TOUCH_HANDLE_STATE_ZOOM_OUT;
  // Calculate line values for animating handle in the path till rect.
  CalculateHandleAnimationPath();
  drag_animation_end_time_ = base::TimeTicks::Now() +
      base::TimeDelta::FromMillisecondsD(kMaxPathAnimationDurationMs);
  client_->SetNeedsAnimate();
}

void TouchHandle::CalculateHandleAnimationPath() {
  // Distance to be travelled in x direction
  float x_dist_top_bound = final_top_.x() - last_touch_move_position_.x();
  float x_dist_bottom_bound = final_bottom_.x()
      - last_touch_move_position_.x();
  // Speed of animation w.r.t. time
  x_speed_top_bound_ = x_dist_top_bound / kMaxPathAnimationDurationMs;
  x_speed_bottom_bound_ = x_dist_bottom_bound / kMaxPathAnimationDurationMs;

  // Distance to be travelled in y direction
  float y_dist_top_bound = final_top_.y() - last_touch_move_position_.y();
  float y_dist_bottom_bound = final_bottom_.y()
      - last_touch_move_position_.y();
  // Speed of animation w.r.t. time
  y_speed_top_bound_ = y_dist_top_bound / kMaxPathAnimationDurationMs;
  y_speed_bottom_bound_ = y_dist_bottom_bound / kMaxPathAnimationDurationMs;
}

bool TouchHandle::AnimateOnPath(base::TimeTicks frame_time) {
  if (!is_handle_moving_with_touch_
      || touch_handle_state_ == TOUCH_HANDLE_STATE_UNDEFINED) {
    EndAnimationOnPath();
    return false;
  }
  float remaining_animation_time = (drag_animation_end_time_
      - frame_time).InMillisecondsF();
  if (remaining_animation_time <= 0) {
    EndAnimationOnPath();
    return false;
  }
  float elapsed_time = kMaxPathAnimationDurationMs - remaining_animation_time;
  // For top bound
  float curr_top_x = (x_speed_top_bound_ * elapsed_time)
      + last_touch_move_position_.x();
  float curr_top_y = (y_speed_top_bound_ * elapsed_time)
      + last_touch_move_position_.y();
  // For bottom bound
  float curr_bottom_x = (x_speed_bottom_bound_ * elapsed_time)
      + last_touch_move_position_.x();
  float curr_bottom_y = (y_speed_bottom_bound_ * elapsed_time)
      + last_touch_move_position_.y();

  is_path_animation_in_progress_ = true;
  SetFocus(gfx::PointF (curr_top_x, curr_top_y),
      gfx::PointF (curr_bottom_x, curr_bottom_y));
  UpdateHandleLayout();
  return true;
}

void TouchHandle::EndAnimationOnPath() {
  drag_animation_end_time_ = base::TimeTicks();
  is_handle_moving_with_touch_ = false;
  is_path_animation_in_progress_ = false;
  // Reset
  x_speed_top_bound_ = 0.f;
  x_speed_bottom_bound_ = 0.f;
  y_speed_bottom_bound_ = 0.f;
  y_speed_top_bound_ = 0.f;
  SetFocus(final_top_, final_bottom_);
  // Handle layout may be deferred while the handle is dragged.
  SetUpdateLayoutRequired();
  UpdateHandleLayout();
}

void TouchHandle::StoreFocus(const gfx::PointF& top,
                             const gfx::PointF& bottom) {
  bound_focus_top_ = top;
  bound_focus_bottom_ = bottom;
  // Ignore Touch handle path animation for fling cases.
  // In case of fling multiple OnSelectionChanged keeps
  // coming even after handle drag ends.
  if(is_handle_moving_with_touch_
      && touch_handle_state_ == TOUCH_HANDLE_STATE_ZOOM_OUT)
    is_handle_moving_with_touch_ = false;

  // Store SelectionBound for handle path animation
  final_top_ = top;
  final_bottom_ = bottom;
}
#endif

void TouchHandle::SetAlpha(float alpha) {
  alpha = std::max(0.f, std::min(1.f, alpha));
  if (alpha_ == alpha)
    return;
  alpha_ = alpha;
  drawable_->SetAlpha(alpha);
}

void TouchHandle::SetUpdateLayoutRequired() {
  // TODO(AviD): Make the layout call explicit to the caller by adding this in
  // TouchHandleClient.
  is_handle_layout_update_required_ = true;
}

}  // namespace ui
