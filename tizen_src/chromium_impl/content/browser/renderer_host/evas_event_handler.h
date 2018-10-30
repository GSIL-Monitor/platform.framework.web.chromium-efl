// Copyright 2015 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EVAS_EVENT_HANDLER_H
#define EVAS_EVENT_HANDLER_H

#include "base/memory/ref_counted.h"

#include <Evas.h>

namespace content {

class EvasEventHandler: public base::RefCountedThreadSafe<EvasEventHandler> {
 public:
  virtual ~EvasEventHandler() {}

  virtual bool HandleEvent_FocusIn() { return false; }
  virtual bool HandleEvent_FocusOut() { return false; }
  virtual bool HandleEvent_KeyUp (const Evas_Event_Key_Up*) { return false; }
  virtual bool HandleEvent_KeyDown (const Evas_Event_Key_Down*) { return false; }
  virtual bool HandleEvent_MouseDown (const Evas_Event_Mouse_Down*) { return false; }
  virtual bool HandleEvent_MouseUp  (const Evas_Event_Mouse_Up*) { return false; }
  virtual bool HandleEvent_MouseMove (const Evas_Event_Mouse_Move*) { return false; }
  virtual bool HandleEvent_MouseWheel (const Evas_Event_Mouse_Wheel*) { return false; }

};

} // namespace content

#endif  // EVAS_EVENT_HANDLER_H
