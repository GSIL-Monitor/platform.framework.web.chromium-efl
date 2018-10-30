// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEB_EVENT_FACTORY_EFL
#define WEB_EVENT_FACTORY_EFL

#include <Evas.h>

#include "content/browser/renderer_host/render_widget_host_impl.h"
#include "content/common/content_export.h"
#include "content/public/browser/native_web_keyboard_event.h"
#include "third_party/WebKit/public/platform/WebInputEvent.h"
#include "ui/events/event.h"

namespace content {

// Mouse
template <class EVT>
blink::WebMouseEvent MakeWebMouseEvent(
    blink::WebInputEvent::Type type, Evas_Object* view, const EVT*);
blink::WebMouseEvent MakeWebMouseEvent(Evas_Object* view,
    const Evas_Event_Mouse_Move* ev);
blink::WebMouseWheelEvent MakeWebMouseEvent(Evas_Object* view,
                                            const Evas_Event_Mouse_Wheel* ev);
blink::WebMouseEvent MakeWebMouseEvent(Evas_Object* view,
                                       const Evas_Event_Mouse_Out* ev);

// Keyboard
template <class EVT>
NativeWebKeyboardEvent MakeWebKeyboardEvent(bool pressed, const EVT*);
bool IsHardwareBackKey(const Evas_Event_Key_Down* event);

// Touch
// TODO(prashant.n): Make timestamp explicit argument.
CONTENT_EXPORT ui::TouchEvent MakeTouchEvent(Evas_Coord_Point pt,
                                             Evas_Touch_Point_State state,
                                             int id, Evas_Object* view,
                                             unsigned int timestamp = 0);

template <class EVT>
blink::WebGestureEvent MakeGestureEvent(blink::WebInputEvent::Type type,
    Evas_Object* view, const EVT* event);

} // namespace content

#endif
