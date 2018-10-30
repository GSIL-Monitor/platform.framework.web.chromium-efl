// Copyright 2018 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/event_resampler.h"

#include "content/browser/renderer_host/render_widget_host_view_efl.h"
#include "ui/events/event_constants.h"

namespace {
// Currently we can resample two fingers only.
const int kIdentifiableTouchCount = 2;

// Two latest touchmove events are used for resampling.
const unsigned kTouchMoveEventQueueSize = 2;

// Latency added during resampling.  A few milliseconds doesn't hurt much but
// reduces the impact of mispredicted touch positions.
const float kResampleLatency = 5;

// Minimum time difference between consecutive samples before attempting to
// resample.
const float kResampleMinDelta = 2;

// Maximum time to predict forward from the last known state, to avoid
// predicting too far into the future.  This time is further bounded by 50% of
// the last time delta.
const float kResampleMaxPrediction = 8;
}  // namespace

namespace content {

EventResampler::EventResampler(EventResamplerClient* client)
    : client_(client),
      vsync_animator_(nullptr),
      has_pending_scroll_event_(false),
      has_pending_pinch_event_(false),
      is_scroll_processed_in_last_callback_(false),
      is_pinch_processed_in_last_callback_(false),
      pending_scroll_delta_(gfx::PointF()),
      pending_pinch_scale_(0.0) {}

EventResampler::~EventResampler() {
  if (!vsync_animator_)
    return;

  ecore_animator_del(vsync_animator_);
  vsync_animator_ = nullptr;
}

bool EventResampler::HandleTouchMoveEvent(ui::TouchEvent event) {
#if defined(TIZEN_TBM_SUPPORT)
  if (!client_->UseEventResampler())
    return false;
#endif

  if (event.type() != ui::EventType::ET_TOUCH_MOVED)
    return false;

  int id = event.pointer_details().id;
  if (id >= kIdentifiableTouchCount)
    return false;

  if (!vsync_animator_) {
    vsync_animator_ = ecore_animator_add(VsyncAnimatorCallback, this);
    return false;
  }

  touch_move_event_queue_[id].push(event);
  if (touch_move_event_queue_[id].size() > kTouchMoveEventQueueSize)
    touch_move_event_queue_[id].pop();

  has_pending_touch_move_event_[id] = true;

  return true;
}

bool EventResampler::HandleGestureEvent(blink::WebGestureEvent& event) {
#if defined(TIZEN_TBM_SUPPORT)
  if (!client_->UseEventResampler())
    return false;
#endif

  switch (event.GetType()) {
    case blink::WebInputEvent::kGestureScrollBegin:
      has_pending_scroll_event_ = false;
      pending_scroll_delta_.SetPoint(0.0, 0.0);
      break;
    case blink::WebInputEvent::kGestureScrollUpdate:
      last_scroll_update_event_ = event;
      pending_scroll_delta_.Offset(event.data.scroll_update.delta_x,
                                   event.data.scroll_update.delta_y);
      has_pending_scroll_event_ = true;
      if (!is_scroll_processed_in_last_callback_) {
        ResampleScrollUpdate();
        is_scroll_processed_in_last_callback_ = true;
      }
      return true;
    case blink::WebInputEvent::kGesturePinchBegin:
      has_pending_pinch_event_ = false;
      pending_pinch_scale_ = 1.0;
      break;
    case blink::WebInputEvent::kGesturePinchUpdate:
      last_pinch_update_event_ = event;
      pending_pinch_scale_ *= event.data.pinch_update.scale;
      has_pending_pinch_event_ = true;

      if (!is_pinch_processed_in_last_callback_) {
        ResamplePinchUpdate();
        is_pinch_processed_in_last_callback_ = true;
      }
      return true;
    case blink::WebInputEvent::kGestureFlingStart:
      if (has_pending_scroll_event_)
        ResampleScrollUpdate();
      break;
    default:
      break;
  }

  return false;
}

// static
Eina_Bool EventResampler::VsyncAnimatorCallback(void* data) {
  auto resampler = static_cast<EventResampler*>(data);
  for (unsigned i = 0; i < kIdentifiableTouchCount; i++) {
    if (resampler->has_pending_touch_move_event_[i])
      resampler->ResampleTouchMove(i);
  }

  resampler->is_scroll_processed_in_last_callback_ = false;
  if (resampler->has_pending_scroll_event_) {
    resampler->ResampleScrollUpdate();
    resampler->is_scroll_processed_in_last_callback_ = true;
  }

  resampler->is_pinch_processed_in_last_callback_ = false;
  if (resampler->has_pending_pinch_event_) {
    resampler->ResamplePinchUpdate();
    resampler->is_pinch_processed_in_last_callback_ = true;
  }

  if (resampler->CanDeleteAnimator()) {
    resampler->ClearTouchMoveEventQueue();
    resampler->vsync_animator_ = nullptr;
    return ECORE_CALLBACK_CANCEL;
  }

  return ECORE_CALLBACK_RENEW;
}

bool EventResampler::CanDeleteAnimator() {
  if (client_->TouchPointCount() > 0)
    return false;

  for (unsigned i = 0; i < kIdentifiableTouchCount; i++) {
    if (has_pending_touch_move_event_[i])
      return false;
  }

  return !has_pending_scroll_event_ && !has_pending_pinch_event_ &&
         !is_scroll_processed_in_last_callback_ &&
         !is_pinch_processed_in_last_callback_;
}

void EventResampler::ResampleTouchMove(int id) {
  if (touch_move_event_queue_[id].empty())
    return;

  has_pending_touch_move_event_[id] = false;

  ui::TouchEvent last_event = touch_move_event_queue_[id].back();
  if (touch_move_event_queue_[id].size() < kTouchMoveEventQueueSize) {
    client_->ForwardTouchEvent(&last_event);
    return;
  }

  ui::TouchEvent first_event = touch_move_event_queue_[id].front();

  float last_event_timestamp =
      (last_event.time_stamp() - base::TimeTicks()).InMillisecondsF();
  float first_event_timestamp =
      (first_event.time_stamp() - base::TimeTicks()).InMillisecondsF();

  float delta = last_event_timestamp - first_event_timestamp;
  if (delta < kResampleMinDelta) {
    client_->ForwardTouchEvent(&last_event);
    return;
  }

  float x, y;
  float sample_time = ecore_loop_time_get() * 1000 - kResampleLatency;
  if (last_event_timestamp > sample_time) {
    // Interpolation.
    float alpha = (sample_time - first_event_timestamp) / delta;
    x = first_event.location().x() +
        (last_event.location().x() - first_event.location().x()) * alpha;
    y = first_event.location().y() +
        (last_event.location().y() - first_event.location().y()) * alpha;
  } else {
    // Extrapolation.
    float max_predict =
        last_event_timestamp + std::min(delta / 2, kResampleMaxPrediction);
    sample_time = std::min(sample_time, max_predict);

    float alpha = (last_event_timestamp - sample_time) / delta;
    x = last_event.location().x() +
        (first_event.location().x() - last_event.location().x()) * alpha;
    y = last_event.location().y() +
        (first_event.location().y() - last_event.location().y()) * alpha;
  }

  last_event.set_location_f(gfx::PointF(x, y));
  client_->ForwardTouchEvent(&last_event);
}

void EventResampler::ResampleScrollUpdate() {
  blink::WebGestureEvent event = last_scroll_update_event_;

  double scroll_delta_x = pending_scroll_delta_.x();
  double scroll_delta_y = pending_scroll_delta_.y();

  // Convert delta type to Int for accuracy of top controls offset.
  if (client_->TouchPointCount() == 1) {
    scroll_delta_x = (scroll_delta_x > 0) ? std::floor(scroll_delta_x)
                                          : std::ceil(scroll_delta_x);

    scroll_delta_y = (scroll_delta_y > 0) ? std::floor(scroll_delta_y)
                                          : std::ceil(scroll_delta_y);
  }

  pending_scroll_delta_.Offset(-scroll_delta_x, -scroll_delta_y);
  event.data.scroll_update.delta_x = scroll_delta_x;
  event.data.scroll_update.delta_y = scroll_delta_y;
  has_pending_scroll_event_ = false;

  client_->ForwardGestureEvent(event);
}

void EventResampler::ResamplePinchUpdate() {
  blink::WebGestureEvent event = last_pinch_update_event_;
  event.data.pinch_update.scale = pending_pinch_scale_;
  pending_pinch_scale_ = 1.0;
  has_pending_pinch_event_ = false;

  client_->ForwardGestureEvent(event);
}

void EventResampler::ClearTouchMoveEventQueue() {
  for (unsigned i = 0; i < kIdentifiableTouchCount; i++) {
    std::queue<ui::TouchEvent> empty;
    std::swap(touch_move_event_queue_[i], empty);
  }
}

}  // namespace content
