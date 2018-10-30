// Copyright 2015 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_EVENTS_GESTURES_GESTURE_RECOGNIZER_IMPL_EFL_H_
#define UI_EVENTS_GESTURES_GESTURE_RECOGNIZER_IMPL_EFL_H_

#include "ui/events/gestures/gesture_recognizer_impl.h"

namespace ui {
class GestureConsumer;

class EVENTS_EXPORT GestureRecognizerImplEfl : public GestureRecognizerImpl {
 public:

  GestureProviderAura* GetGestureProviderForConsumer(
      GestureConsumer* c) override;
};

}  // namespace ui

#endif  // UI_EVENTS_GESTURES_GESTURE_RECOGNIZER_IMPL_EFL_H_
