// Copyright 2013 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef selection_magnifier_efl_h
#define selection_magnifier_efl_h

#include <Ecore.h>
#include <Eina.h>
#include <Evas.h>

#include "ui/gfx/image/image_skia.h"
#include "ui/gfx/geometry/point.h"

namespace content {

class SelectionControllerEfl;

class SelectionMagnifierEfl {
 public:
  SelectionMagnifierEfl(content::SelectionControllerEfl* controller);
  ~SelectionMagnifierEfl();

  void HandleLongPress(const gfx::Point& touch_point, bool is_in_edit_field);
  void UpdateLocation(const gfx::Point& location);
  void Move(const gfx::Point& location);
  void Show();
  void Hide();
  bool IsShowing() { return shown_; }
  void OnAnimatorMove();
  void OnAnimatorUp();

 private:
  void DisconnectCallbacks();

  static void OnAnimatorMove(void* data, Evas*, Evas_Object*, void*);
  static void OnAnimatorUp(void* data, Evas*, Evas_Object*, void*);
  static void MagnifierGetSnapshotCb(Evas_Object* image, void* user_data);

  // Parent to send back mouse events
  SelectionControllerEfl* controller_;

  // Magnifier
  Evas_Object* container_;

  // Image displayed on popup
  Evas_Object* content_image_;

  // Magnifier Height
  int height_;

  // Magnifier width
  int width_;

  // Is magnifier showing
  bool shown_;
};

}

#endif
