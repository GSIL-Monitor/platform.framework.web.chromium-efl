// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/events/gesture_detection/gesture_configuration.h"

#include <Elementary.h>

#include "base/command_line.h"
#include "base/memory/singleton.h"
#include "ui/events/event_switches.h"

namespace ui {
namespace {

const float kDefaultMaxFlingVelocity = 4500;

class GestureConfigurationEfl : public GestureConfiguration {
 public:
  ~GestureConfigurationEfl() override {
  }

  static GestureConfigurationEfl* GetInstance() {
    return base::Singleton<GestureConfigurationEfl>::get();
  }

 private:
  GestureConfigurationEfl() : GestureConfiguration() {
    set_gesture_begin_end_types_enabled(true);
    set_long_press_time_in_ms(elm_config_longpress_timeout_get() * 1000);
    set_max_tap_count(1);
    set_max_distance_between_taps_for_double_tap(
        elm_config_finger_size_get());
    set_max_touch_move_in_pixels_for_click(
        elm_config_scroll_thumbscroll_threshold_get());
    set_max_fling_velocity(kDefaultMaxFlingVelocity);
  }

  friend struct base::DefaultSingletonTraits<GestureConfigurationEfl>;
  DISALLOW_COPY_AND_ASSIGN(GestureConfigurationEfl);
};

}  // namespace

// Create a GestureConfigurationEfl singleton instance when using aura.
GestureConfiguration* GestureConfiguration::GetPlatformSpecificInstance() {
  return GestureConfigurationEfl::GetInstance();
}

}  // namespace ui
