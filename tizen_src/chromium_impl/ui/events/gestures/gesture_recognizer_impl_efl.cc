// Copyright 2015 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/events/gestures/gesture_recognizer_impl_efl.h"

#include "ui/events/gestures/gesture_recognizer_impl.h"
#include "ui/events/gestures/gesture_recognizer.h"

namespace ui {

// The two methods below are originally defined in
// ui/events/gestures/gesture_recognizer_impl.cc. The reason
// for EFL port to redefine them are:
// 1) ::Create method instantiates our own GestureRegnizerImplEfl,
//    instead of its parent GestureRecognizerImpl.
//    See gesture_recognizer_impl_override.cc.
// 2) In GestureRegnizerImplEfl, ::GetGestureProviderForConsumer method
//    is overloaded and made public. In practice, it just calls its
//    parent class implementation.
//
// We use the getter to access the instance of GestureProviderAura
// object associated with GestureRecognizer{Impl}. Further customizations
// are then possible, including enabling double tap support.

GestureProviderAura* GestureRecognizerImplEfl::GetGestureProviderForConsumer(
    GestureConsumer* c) {
  return GestureRecognizerImpl::GetGestureProviderForConsumer(c);
}

// GestureRecognizer, static
GestureRecognizer* GestureRecognizer::Create() {
  return new GestureRecognizerImplEfl();
}

} // namespace ui

