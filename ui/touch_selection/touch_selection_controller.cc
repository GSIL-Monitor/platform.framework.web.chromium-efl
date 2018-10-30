// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/touch_selection/touch_selection_controller.h"

#include "base/auto_reset.h"
#include "base/logging.h"
#include "base/metrics/histogram_macros.h"
#include "base/metrics/user_metrics.h"

namespace ui {
namespace {

gfx::Vector2dF ComputeLineOffsetFromBottom(const gfx::SelectionBound& bound) {
  gfx::Vector2dF line_offset =
      gfx::ScaleVector2d(bound.edge_top() - bound.edge_bottom(), 0.5f);
  // An offset of 8 DIPs is sufficient for most line sizes. For small lines,
  // using half the line height avoids synthesizing a point on a line above
  // (or below) the intended line.
  const gfx::Vector2dF kMaxLineOffset(8.f, 8.f);
  line_offset.SetToMin(kMaxLineOffset);
  line_offset.SetToMax(-kMaxLineOffset);
  return line_offset;
}

TouchHandleOrientation ToTouchHandleOrientation(
    gfx::SelectionBound::Type type) {
  switch (type) {
    case gfx::SelectionBound::LEFT:
      return TouchHandleOrientation::LEFT;
    case gfx::SelectionBound::RIGHT:
      return TouchHandleOrientation::RIGHT;
    case gfx::SelectionBound::CENTER:
      return TouchHandleOrientation::CENTER;
    case gfx::SelectionBound::EMPTY:
      return TouchHandleOrientation::UNDEFINED;
  }
  NOTREACHED() << "Invalid selection bound type: " << type;
  return TouchHandleOrientation::UNDEFINED;
}

}  // namespace

TouchSelectionController::Config::Config()
    : max_tap_duration(base::TimeDelta::FromMilliseconds(300)),
      tap_slop(8),
      enable_adaptive_handle_orientation(false),
      enable_longpress_drag_selection(false) {}

TouchSelectionController::Config::~Config() {
}

TouchSelectionController::TouchSelectionController(
    TouchSelectionControllerClient* client,
    const Config& config)
    : client_(client),
      config_(config),
      response_pending_input_event_(INPUT_EVENT_TYPE_NONE),
      start_orientation_(TouchHandleOrientation::UNDEFINED),
      end_orientation_(TouchHandleOrientation::UNDEFINED),
      active_status_(INACTIVE),
      temporarily_hidden_(false),
      anchor_drag_to_selection_start_(false),
      longpress_drag_selector_(this),
      selection_handle_dragged_(false),
      consume_touch_sequence_(false),
      show_touch_handles_(true)
{
  DCHECK(client_);
}

TouchSelectionController::~TouchSelectionController() {
}

void TouchSelectionController::OnSelectionBoundsChanged(
    const gfx::SelectionBound& start,
    const gfx::SelectionBound& end) {
  if (!ShouldUpdateSelectionBounds(start, end))
    return;

#if defined(S_TERRACE_SUPPORT)
  // Swap the Handles when the start and end selection points cross each other.
  if (active_status_ == SELECTION_ACTIVE) {
    if ((start_selection_handle_->IsActive() &&
         end_.edge_bottom() == start.edge_bottom()) ||
        (end_selection_handle_->IsActive() &&
         end.edge_bottom() == start_.edge_bottom())) {
      start_selection_handle_.swap(end_selection_handle_);
    } else {
      // Swap the handles when SelectionBound become visible
      // from not visible state during scroll.
      if ((start_selection_handle_->IsActive() &&
           !end_.visible() && end.visible()) ||
          (end_selection_handle_->IsActive() &&
           !start_.visible() && start.visible())) {
        start_selection_handle_.swap(end_selection_handle_);
      }
    }
  }
#else
  // Swap the Handles when the start and end selection points cross each other.
  if (active_status_ == SELECTION_ACTIVE) {
    if ((start_selection_handle_->IsActive() &&
         end_.edge_bottom() == start.edge_bottom()) ||
        (end_selection_handle_->IsActive() &&
         end.edge_bottom() == start_.edge_bottom())) {
      start_selection_handle_.swap(end_selection_handle_);
    }
  }
#endif

  start_ = start;
  end_ = end;
  start_orientation_ = ToTouchHandleOrientation(start_.type());
  end_orientation_ = ToTouchHandleOrientation(end_.type());

  // Ensure that |response_pending_input_event_| is cleared after the method
  // completes, while also making its current value available for the duration
  // of the call.
  InputEventType causal_input_event = response_pending_input_event_;
  response_pending_input_event_ = INPUT_EVENT_TYPE_NONE;
  base::AutoReset<InputEventType> auto_reset_response_pending_input_event(
      &response_pending_input_event_, causal_input_event);

  if ((start_orientation_ == TouchHandleOrientation::LEFT ||
       start_orientation_ == TouchHandleOrientation::RIGHT) &&
      (end_orientation_ == TouchHandleOrientation::RIGHT ||
       end_orientation_ == TouchHandleOrientation::LEFT)) {
    OnSelectionChanged();
    return;
  }

  if (start_orientation_ == TouchHandleOrientation::CENTER) {
    OnInsertionChanged();
    return;
  }

  HideHandles();
}

void TouchSelectionController::OnViewportChanged(
    const gfx::RectF viewport_rect) {
  // Trigger a force update if the viewport is changed, so that
  // it triggers a call to change the mirror values if required.
  if (viewport_rect_ == viewport_rect)
    return;

  viewport_rect_ = viewport_rect;

  if (active_status_ == INACTIVE)
    return;

  if (active_status_ == INSERTION_ACTIVE) {
    DCHECK(insertion_handle_);
    insertion_handle_->SetViewportRect(viewport_rect);
  } else if (active_status_ == SELECTION_ACTIVE) {
    DCHECK(start_selection_handle_);
    DCHECK(end_selection_handle_);
    start_selection_handle_->SetViewportRect(viewport_rect);
    end_selection_handle_->SetViewportRect(viewport_rect);
  }

  // Update handle layout after setting the new Viewport size.
  UpdateHandleLayoutIfNecessary();
}

bool TouchSelectionController::WillHandleTouchEvent(const MotionEvent& event) {
  bool handled = WillHandleTouchEventImpl(event);
  // If ACTION_DOWN is consumed, the rest of touch sequence should be consumed,
  // too, regardless of value of |handled|.
  // TODO(mohsen): This will consume touch events until the next ACTION_DOWN.
  // Ideally we should consume until the final ACTION_UP/ACTION_CANCEL.
  // But, apparently, we can't reliably determine the final ACTION_CANCEL in a
  // multi-touch scenario. See https://crbug.com/653212.
  if (event.GetAction() == MotionEvent::ACTION_DOWN)
    consume_touch_sequence_ = handled;
  return handled || consume_touch_sequence_;
}

void TouchSelectionController::HandleTapEvent(const gfx::PointF& location,
                                                  int tap_count) {
  if (tap_count > 1) {
    response_pending_input_event_ = REPEATED_TAP;
  } else {
    response_pending_input_event_ = TAP;
#if defined(S_TERRACE_SUPPORT)
    show_touch_handles_ = true;
#endif
  }
}

void TouchSelectionController::HandleLongPressEvent(
    base::TimeTicks event_time,
    const gfx::PointF& location) {
#if defined(S_TERRACE_SUPPORT)
  show_touch_handles_ = true;
#endif
  longpress_drag_selector_.OnLongPressEvent(event_time, location);
  response_pending_input_event_ = LONG_PRESS;
}

void TouchSelectionController::OnScrollBeginEvent() {
  // When there is an active selection, if the user performs a long-press that
  // does not trigger a new selection (e.g. a long-press on an empty area) and
  // then scrolls, the scroll will move the selection. In this case we will
  // think incorrectly that the selection change was due to the long-press and
  // will activate touch selection and start long-press drag gesture (see
  // ActivateInsertionIfNecessary()). To prevent this, we need to reset the
  // state of touch selection controller and long-press drag selector.
  // TODO(mohsen): Remove this workaround when we have enough information about
  // the cause of a selection change (see https://crbug.com/571897).
  longpress_drag_selector_.OnScrollBeginEvent();
  response_pending_input_event_ = INPUT_EVENT_TYPE_NONE;
}

void TouchSelectionController::HideHandles() {
  response_pending_input_event_ = INPUT_EVENT_TYPE_NONE;
  DeactivateInsertion();
  DeactivateSelection();
  start_ = gfx::SelectionBound();
  end_ = gfx::SelectionBound();
  start_orientation_ = ToTouchHandleOrientation(start_.type());
  end_orientation_ = ToTouchHandleOrientation(end_.type());
}

void TouchSelectionController::HideAndDisallowShowingAutomatically() {
  HideHandles();
  show_touch_handles_ = false;
}

void TouchSelectionController::SetTemporarilyHidden(bool hidden) {
  if (temporarily_hidden_ == hidden)
    return;
  temporarily_hidden_ = hidden;
  RefreshHandleVisibility();
}

bool TouchSelectionController::Animate(base::TimeTicks frame_time) {
  if (active_status_ == INSERTION_ACTIVE)
    return insertion_handle_->Animate(frame_time);

  if (active_status_ == SELECTION_ACTIVE) {
    bool needs_animate = start_selection_handle_->Animate(frame_time);
    needs_animate |= end_selection_handle_->Animate(frame_time);
    return needs_animate;
  }

  return false;
}

gfx::RectF TouchSelectionController::GetRectBetweenBounds() const {
  // Short-circuit for efficiency.
  if (active_status_ == INACTIVE)
    return gfx::RectF();
#if !defined(S_TERRACE_SUPPORT)
  if (start_.visible() && !end_.visible())
    return gfx::BoundingRect(start_.edge_top(), start_.edge_bottom());

  if (end_.visible() && !start_.visible())
    return gfx::BoundingRect(end_.edge_top(), end_.edge_bottom());
#endif
  // If both handles are visible, or both are invisible, use the entire rect.
  return RectFBetweenSelectionBounds(start_, end_);
}

gfx::RectF TouchSelectionController::GetStartHandleRect() const {
  if (active_status_ == INSERTION_ACTIVE)
    return insertion_handle_->GetVisibleBounds();
  if (active_status_ == SELECTION_ACTIVE)
    return start_selection_handle_->GetVisibleBounds();
  return gfx::RectF();
}

gfx::RectF TouchSelectionController::GetEndHandleRect() const {
  if (active_status_ == INSERTION_ACTIVE)
    return insertion_handle_->GetVisibleBounds();
  if (active_status_ == SELECTION_ACTIVE)
    return end_selection_handle_->GetVisibleBounds();
  return gfx::RectF();
}

const gfx::PointF& TouchSelectionController::GetStartPosition() const {
  return start_.edge_bottom();
}

const gfx::PointF& TouchSelectionController::GetEndPosition() const {
  return end_.edge_bottom();
}

bool TouchSelectionController::WillHandleTouchEventImpl(
    const MotionEvent& event) {
  show_touch_handles_ = true;
  if (config_.enable_longpress_drag_selection &&
      longpress_drag_selector_.WillHandleTouchEvent(event)) {
    return true;
  }

  if (active_status_ == INSERTION_ACTIVE) {
    DCHECK(insertion_handle_);
    return insertion_handle_->WillHandleTouchEvent(event);
  }

  if (active_status_ == SELECTION_ACTIVE) {
    DCHECK(start_selection_handle_);
    DCHECK(end_selection_handle_);
    if (start_selection_handle_->IsActive())
      return start_selection_handle_->WillHandleTouchEvent(event);

    if (end_selection_handle_->IsActive())
      return end_selection_handle_->WillHandleTouchEvent(event);

    const gfx::PointF event_pos(event.GetX(), event.GetY());
    if ((event_pos - GetStartPosition()).LengthSquared() <=
        (event_pos - GetEndPosition()).LengthSquared()) {
      return start_selection_handle_->WillHandleTouchEvent(event);
    }
    return end_selection_handle_->WillHandleTouchEvent(event);
  }

  return false;
}

void TouchSelectionController::OnDragBegin(
    const TouchSelectionDraggable& draggable,
    const gfx::PointF& drag_position) {
  if (&draggable == insertion_handle_.get()) {
    DCHECK_EQ(active_status_, INSERTION_ACTIVE);
    client_->OnSelectionEvent(INSERTION_HANDLE_DRAG_STARTED);
    anchor_drag_to_selection_start_ = true;
    return;
  }

  DCHECK_EQ(active_status_, SELECTION_ACTIVE);

  if (&draggable == start_selection_handle_.get()) {
    anchor_drag_to_selection_start_ = true;
  } else if (&draggable == end_selection_handle_.get()) {
    anchor_drag_to_selection_start_ = false;
  } else {
    DCHECK_EQ(&draggable, &longpress_drag_selector_);
    anchor_drag_to_selection_start_ =
        (drag_position - GetStartPosition()).LengthSquared() <
        (drag_position - GetEndPosition()).LengthSquared();
  }

  gfx::PointF base = GetStartPosition() + GetStartLineOffset();
  gfx::PointF extent = GetEndPosition() + GetEndLineOffset();
  if (anchor_drag_to_selection_start_)
    std::swap(base, extent);

  // If this is the first drag, log an action to allow user action sequencing.
  if (!selection_handle_dragged_)
    base::RecordAction(base::UserMetricsAction("SelectionChanged"));
  selection_handle_dragged_ = true;

  // When moving the handle we want to move only the extent point. Before doing
  // so we must make sure that the base point is set correctly.
  client_->SelectBetweenCoordinates(base, extent);
  client_->OnSelectionEvent(SELECTION_HANDLE_DRAG_STARTED);
}

void TouchSelectionController::OnDragUpdate(
    const TouchSelectionDraggable& draggable,
    const gfx::PointF& drag_position) {
  // As the position corresponds to the bottom left point of the selection
  // bound, offset it to some reasonable point on the current line of text.
  gfx::Vector2dF line_offset = anchor_drag_to_selection_start_
                                   ? GetStartLineOffset()
                                   : GetEndLineOffset();
  gfx::PointF line_position = drag_position + line_offset;
  if (&draggable == insertion_handle_.get())
    client_->MoveCaret(line_position);
  else
    client_->MoveRangeSelectionExtent(line_position);
}

void TouchSelectionController::OnDragEnd(
    const TouchSelectionDraggable& draggable) {
  if (&draggable == insertion_handle_.get())
    client_->OnSelectionEvent(INSERTION_HANDLE_DRAG_STOPPED);
  else
    client_->OnSelectionEvent(SELECTION_HANDLE_DRAG_STOPPED);
}

bool TouchSelectionController::IsWithinTapSlop(
    const gfx::Vector2dF& delta) const {
  return delta.LengthSquared() <
         (static_cast<double>(config_.tap_slop) * config_.tap_slop);
}

void TouchSelectionController::OnHandleTapped(const TouchHandle& handle) {
  if (insertion_handle_ && &handle == insertion_handle_.get())
    client_->OnSelectionEvent(INSERTION_HANDLE_TAPPED);
}

void TouchSelectionController::SetNeedsAnimate() {
  client_->SetNeedsAnimate();
}

std::unique_ptr<TouchHandleDrawable>
TouchSelectionController::CreateDrawable() {
  return client_->CreateDrawable();
}

base::TimeDelta TouchSelectionController::GetMaxTapDuration() const {
  return config_.max_tap_duration;
}

bool TouchSelectionController::IsAdaptiveHandleOrientationEnabled() const {
  return config_.enable_adaptive_handle_orientation;
}

void TouchSelectionController::OnLongPressDragActiveStateChanged() {
  // The handles should remain hidden for the duration of a longpress drag,
  // including the time between a longpress and the start of drag motion.
  RefreshHandleVisibility();
}

gfx::PointF TouchSelectionController::GetSelectionStart() const {
  return GetStartPosition();
}

gfx::PointF TouchSelectionController::GetSelectionEnd() const {
  return GetEndPosition();
}

void TouchSelectionController::OnInsertionChanged() {
  DeactivateSelection();

  const bool activated = ActivateInsertionIfNecessary();

  const TouchHandle::AnimationStyle animation = GetAnimationStyle(!activated);
  insertion_handle_->SetFocus(start_.edge_top(), start_.edge_bottom());
  insertion_handle_->SetVisible(GetStartVisible(), animation);

  UpdateHandleLayoutIfNecessary();

  client_->OnSelectionEvent(activated ? INSERTION_HANDLE_SHOWN
                                      : INSERTION_HANDLE_MOVED);
}

void TouchSelectionController::OnSelectionChanged() {
  DeactivateInsertion();

  const bool activated = ActivateSelectionIfNecessary();
#if defined(S_TERRACE_SUPPORT)
  // Code for : Move Touch Handle with finger
  // To set focus for the dragged touch handle to follow touch points.
  const TouchHandle::AnimationStyle animation = GetAnimationStyle(true);

  // Ignore setting of touch handle focus while being dragged to
  // enable dragged touch handle to follow finger touch point
  // irrespective of selection bound
  if (!start_selection_handle_->IsActive())
    start_selection_handle_->SetFocus(start_.edge_top(), start_.edge_bottom());
  if (!end_selection_handle_->IsActive())
    end_selection_handle_->SetFocus(end_.edge_top(), end_.edge_bottom());
#else
  const TouchHandle::AnimationStyle animation = GetAnimationStyle(!activated);

  start_selection_handle_->SetFocus(start_.edge_top(), start_.edge_bottom());
  end_selection_handle_->SetFocus(end_.edge_top(), end_.edge_bottom());
#endif
  start_selection_handle_->SetOrientation(start_orientation_);
  end_selection_handle_->SetOrientation(end_orientation_);

  start_selection_handle_->SetVisible(GetStartVisible(), animation);
  end_selection_handle_->SetVisible(GetEndVisible(), animation);

  UpdateHandleLayoutIfNecessary();

  if (!longpress_drag_selector_.IsActive()) {
    client_->OnSelectionEvent(activated ? SELECTION_HANDLES_SHOWN
                                        : SELECTION_HANDLES_MOVED);
  }
}

bool TouchSelectionController::ActivateInsertionIfNecessary() {
  DCHECK_NE(SELECTION_ACTIVE, active_status_);

  if (!insertion_handle_) {
    insertion_handle_.reset(
        new TouchHandle(this, TouchHandleOrientation::CENTER, viewport_rect_));
  }

  if (active_status_ == INACTIVE || response_pending_input_event_ == TAP ||
      response_pending_input_event_ == LONG_PRESS) {
    active_status_ = INSERTION_ACTIVE;
    insertion_handle_->SetEnabled(true);
    insertion_handle_->SetViewportRect(viewport_rect_);
    response_pending_input_event_ = INPUT_EVENT_TYPE_NONE;
    return true;
  }
  return false;
}

void TouchSelectionController::DeactivateInsertion() {
  if (active_status_ != INSERTION_ACTIVE)
    return;
  DCHECK(insertion_handle_);
  active_status_ = INACTIVE;
  insertion_handle_->SetEnabled(false);
  client_->OnSelectionEvent(INSERTION_HANDLE_CLEARED);
}

bool TouchSelectionController::ActivateSelectionIfNecessary() {
  DCHECK_NE(INSERTION_ACTIVE, active_status_);

  if (!start_selection_handle_) {
    start_selection_handle_.reset(
        new TouchHandle(this, start_orientation_, viewport_rect_));
  } else {
    start_selection_handle_->SetEnabled(true);
    start_selection_handle_->SetViewportRect(viewport_rect_);
  }

  if (!end_selection_handle_) {
    end_selection_handle_.reset(
        new TouchHandle(this, end_orientation_, viewport_rect_));
  } else {
    end_selection_handle_->SetEnabled(true);
    end_selection_handle_->SetViewportRect(viewport_rect_);
  }

  // As a long press received while a selection is already active may trigger
  // an entirely new selection, notify the client but avoid sending an
  // intervening SELECTION_HANDLES_CLEARED update to avoid unnecessary state
  // changes.
  if (active_status_ == INACTIVE ||
      response_pending_input_event_ == LONG_PRESS ||
      response_pending_input_event_ == REPEATED_TAP) {
    if (active_status_ == SELECTION_ACTIVE) {
      // The active selection session finishes with the start of the new one.
      LogSelectionEnd();
    }
    active_status_ = SELECTION_ACTIVE;
    selection_handle_dragged_ = false;
    selection_start_time_ = base::TimeTicks::Now();
    response_pending_input_event_ = INPUT_EVENT_TYPE_NONE;
    longpress_drag_selector_.OnSelectionActivated();
    return true;
  }
  return false;
}

void TouchSelectionController::DeactivateSelection() {
  if (active_status_ != SELECTION_ACTIVE)
    return;
  DCHECK(start_selection_handle_);
  DCHECK(end_selection_handle_);
  LogSelectionEnd();
  longpress_drag_selector_.OnSelectionDeactivated();
  start_selection_handle_->SetEnabled(false);
  end_selection_handle_->SetEnabled(false);
  active_status_ = INACTIVE;
  client_->OnSelectionEvent(SELECTION_HANDLES_CLEARED);
}

void TouchSelectionController::UpdateHandleLayoutIfNecessary() {
  if (active_status_ == INSERTION_ACTIVE) {
    DCHECK(insertion_handle_);
    insertion_handle_->UpdateHandleLayout();
  } else if (active_status_ == SELECTION_ACTIVE) {
    DCHECK(start_selection_handle_);
    DCHECK(end_selection_handle_);
    start_selection_handle_->UpdateHandleLayout();
    end_selection_handle_->UpdateHandleLayout();
  }
}

void TouchSelectionController::RefreshHandleVisibility() {
  TouchHandle::AnimationStyle animation_style = GetAnimationStyle(true);
  if (active_status_ == SELECTION_ACTIVE) {
    start_selection_handle_->SetVisible(GetStartVisible(), animation_style);
    end_selection_handle_->SetVisible(GetEndVisible(), animation_style);
  }
  if (active_status_ == INSERTION_ACTIVE)
    insertion_handle_->SetVisible(GetStartVisible(), animation_style);

  // Update handle layout if handle visibility is explicitly changed.
  UpdateHandleLayoutIfNecessary();
}

gfx::Vector2dF TouchSelectionController::GetStartLineOffset() const {
  return ComputeLineOffsetFromBottom(start_);
}

gfx::Vector2dF TouchSelectionController::GetEndLineOffset() const {
  return ComputeLineOffsetFromBottom(end_);
}

bool TouchSelectionController::GetStartVisible() const {
  if (!start_.visible())
    return false;

  return !temporarily_hidden_ && !longpress_drag_selector_.IsActive();
}

bool TouchSelectionController::GetEndVisible() const {
  if (!end_.visible())
    return false;

  return !temporarily_hidden_ && !longpress_drag_selector_.IsActive();
}

TouchHandle::AnimationStyle TouchSelectionController::GetAnimationStyle(
    bool was_active) const {
  return was_active && client_->SupportsAnimation()
             ? TouchHandle::ANIMATION_SMOOTH
             : TouchHandle::ANIMATION_NONE;
}

void TouchSelectionController::LogSelectionEnd() {
  // TODO(mfomitchev): Once we are able to tell the difference between
  // 'successful' and 'unsuccessful' selections - log
  // Event.TouchSelection.Duration instead and get rid of
  // Event.TouchSelectionD.WasDraggeduration.
  if (selection_handle_dragged_) {
    base::TimeDelta duration = base::TimeTicks::Now() - selection_start_time_;
    UMA_HISTOGRAM_CUSTOM_TIMES("Event.TouchSelection.WasDraggedDuration",
                               duration,
                               base::TimeDelta::FromMilliseconds(500),
                               base::TimeDelta::FromSeconds(60),
                               60);
  }
}

const gfx::SelectionBound& TouchSelectionController::GetStartSelectionBound()  const {
  return start_;
}

const gfx::SelectionBound& TouchSelectionController::GetEndSelectionBound()  const {
  return end_;
}

TouchHandleOrientation
    TouchSelectionController::GetStartHandleOrientation() const {
  return start_orientation_;
}

TouchHandleOrientation
    TouchSelectionController::GetEndHandleOrientation() const {
  return end_orientation_;
}

void TouchSelectionController::SetStartHandleOrientation(
      TouchHandleOrientation orientation) {
  start_orientation_ = orientation;
}

void TouchSelectionController::SetEndHandleOrientation(
      TouchHandleOrientation orientation) {
  end_orientation_ = orientation;
}

const TouchHandle* TouchSelectionController::GetStartSelectionHandle() const {
  if (!start_selection_handle_)
    return NULL;
  return start_selection_handle_.get();
}

const TouchHandle* TouchSelectionController::GetEndSelectionHandle() const {
  if (!end_selection_handle_)
    return NULL;
  return end_selection_handle_.get();
}

void TouchSelectionController::CreateStartSelectionHandle() {
  start_selection_handle_.reset(
      new TouchHandle(this, start_orientation_, viewport_rect_));
}

void TouchSelectionController::CreateEndSelectionHandle() {
  end_selection_handle_.reset(
      new TouchHandle(this, end_orientation_, viewport_rect_));
}

bool TouchSelectionController::IsSelectionActive() const {
  return (active_status_ == SELECTION_ACTIVE);
}

bool TouchSelectionController::IsStartVisible() const {
  return GetStartVisible();
}

bool TouchSelectionController::IsEndVisible() const {
  return GetEndVisible();
}

bool TouchSelectionController::IsTemporarilyHidden() const {
  return temporarily_hidden_;
}

void TouchSelectionController::RefreshHandleOrientation() {
  DCHECK(start_selection_handle_);
  DCHECK(end_selection_handle_);

  start_selection_handle_->SetOrientation(start_orientation_);
  end_selection_handle_->SetOrientation(end_orientation_);
}

bool TouchSelectionController::ShouldUpdateSelectionBounds(
    const gfx::SelectionBound& start,
    const gfx::SelectionBound& end) {
  if (start == start_ && end_ == end)
    return false;

  // Notify if selection bounds have just been established or dissolved.
  if (start.type() != gfx::SelectionBound::EMPTY &&
      start_.type() == gfx::SelectionBound::EMPTY) {
    client_->OnSelectionEvent(SELECTION_ESTABLISHED);
  } else if (start.type() == gfx::SelectionBound::EMPTY &&
             start_.type() != gfx::SelectionBound::EMPTY) {
    client_->OnSelectionEvent(SELECTION_DISSOLVED);
  } else if (start.type() != gfx::SelectionBound::CENTER) {
    if (start.edge_bottom() != start_.edge_bottom() ||
        end.edge_bottom() != end_.edge_bottom()) {
      client_->OnSelectionEvent(SELECTION_IN_PROGRESS);
    }
  } else if (start.type() == gfx::SelectionBound::CENTER &&
             (start_.type() == gfx::SelectionBound::LEFT ||
              start_.type() == gfx::SelectionBound::RIGHT)) {
    // Selection collapsed inside an editable field, because there is no
    // selection clear in editable field.
    client_->OnSelectionEvent(SELECTION_DISSOLVED);
  }
  return true;
}

}  // namespace ui
