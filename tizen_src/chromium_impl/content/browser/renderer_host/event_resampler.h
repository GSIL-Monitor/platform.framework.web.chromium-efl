// Copyright 2018 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EVENT_RESAMPLER
#define EVENT_RESAMPLER

#include <Ecore.h>
#include <queue>

#include "third_party/WebKit/public/platform/WebGestureEvent.h"
#include "ui/gfx/geometry/point_f.h"

#define MAX_TOUCH_COUNT 2

namespace ui {
class TouchEvent;
}

namespace content {

class RenderWidgetHostViewEfl;

class EventResamplerClient {
 public:
  // Forward resampled touch event.
  virtual void ForwardTouchEvent(ui::TouchEvent*) {}

  // Forward resampled gesture event.
  virtual void ForwardGestureEvent(blink::WebGestureEvent) {}

  // Return number of touch point.
  virtual unsigned TouchPointCount() const { return 0; }

#if defined(TIZEN_TBM_SUPPORT)
  virtual bool UseEventResampler() { return true; }
#endif
};

class EventResampler {
 public:
  explicit EventResampler(EventResamplerClient* client);
  ~EventResampler();

  bool HandleTouchMoveEvent(ui::TouchEvent event);
  bool HandleGestureEvent(blink::WebGestureEvent& event);

 private:
  static Eina_Bool VsyncAnimatorCallback(void* data);

  void ResampleTouchMove(int id);
  void ResampleScrollUpdate();
  void ResamplePinchUpdate();

  void ClearTouchMoveEventQueue();
  bool CanDeleteAnimator();

  EventResamplerClient* client_;

  Ecore_Animator* vsync_animator_;
  bool has_pending_touch_move_event_[MAX_TOUCH_COUNT] = {
      false,
  };
  bool has_pending_scroll_event_;
  bool has_pending_pinch_event_;

  // To process without delay if event isn't processed in last callback.
  bool is_scroll_processed_in_last_callback_;
  bool is_pinch_processed_in_last_callback_;

  std::queue<ui::TouchEvent> touch_move_event_queue_[MAX_TOUCH_COUNT];
  blink::WebGestureEvent last_scroll_update_event_;
  blink::WebGestureEvent last_pinch_update_event_;

  gfx::PointF pending_scroll_delta_;
  float pending_pinch_scale_;
};

}  // namespace content

#endif  // EVENT_RESAMPLER
