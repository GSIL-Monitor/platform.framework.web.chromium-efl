/*
 * Copyright (C) 2015 Samsung Electronics
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "content/browser/renderer_host/edge_effect.h"

#include <Edje.h>

#include "base/files/file_path.h"
#include "base/logging.h"
#include "base/path_service.h"
#include "content/common/paths_efl.h"
#include "tizen/system_info.h"

namespace content {

namespace {
bool g_force_disable_edge_effect = false;

void viewShowCallback(void* data, Evas*, Evas_Object* view, void*) {
  static_cast<EdgeEffect*>(data)->ShowObject();
}

void viewHideCallback(void* data, Evas*, Evas_Object* view, void*) {
  static_cast<EdgeEffect*>(data)->HideObject();
}
}

EdgeEffect::EdgeEffect(Evas_Object* parent)
    : parent_view_(parent) {
  DCHECK(parent);

  edge_object_ = edje_object_add(evas_object_evas_get(parent));
  CHECK(edge_object_);

  base::FilePath edj_dir;
  base::FilePath edge_edj;
  PathService::Get(PathsEfl::EDJE_RESOURCE_DIR, &edj_dir);
  edge_edj = edj_dir.Append(FILE_PATH_LITERAL("Edge.edj"));
  CHECK(edje_object_file_set(edge_object_,
      edge_edj.AsUTF8Unsafe().c_str(), "edge_effect"));

  int x, y, width, height;
  evas_object_geometry_get(parent, &x, &y, &width, &height);
  evas_object_move(edge_object_, x, y);
  evas_object_resize(edge_object_, width, height);
  evas_object_show(edge_object_);
  evas_object_pass_events_set(edge_object_, EINA_TRUE);

  evas_object_event_callback_add(parent,
      EVAS_CALLBACK_SHOW, &viewShowCallback, this);
  evas_object_event_callback_add(parent,
      EVAS_CALLBACK_HIDE, &viewHideCallback, this);
}

EdgeEffect::~EdgeEffect() {
  evas_object_del(edge_object_);
  evas_object_event_callback_del(parent_view_,
      EVAS_CALLBACK_SHOW, viewShowCallback);
  evas_object_event_callback_del(parent_view_,
      EVAS_CALLBACK_HIDE, viewHideCallback);
}

//static
void EdgeEffect::EnableGlobally(bool enable) {
  g_force_disable_edge_effect = !enable;
}

const std::string EdgeEffect::GetSourceString(const Direction direction) {
  switch (direction) {
    case TOP:
      return std::string("edge,top");
    case BOTTOM:
      return std::string("edge,bottom");
    case LEFT:
      return std::string("edge,left");
    case RIGHT:
      return std::string("edge,right");
    default:
      return std::string();
  }
}

void EdgeEffect::ShowOrHide(const Direction direction, bool show) {
  if (direction < TOP || direction > RIGHT)
    return;

  if (show && (!enabled_ || g_force_disable_edge_effect))
    return;

  if ((show && visible_[direction]) || (!show && !visible_[direction]))
    return;

  std::string msg;
  visible_[direction] = show;
  if (IsWearableProfile())
    msg = show ? "edge,show,wearable" : "edge,hide,wearable";
  else
    msg = show ? "edge,show" : "edge,hide";

  edje_object_signal_emit(edge_object_, msg.c_str(),
                          GetSourceString(direction).c_str());
  if (IsWearableProfile())
    edje_object_signal_emit(edge_object_, show ? "glow,show" : "glow,hide",
                            GetSourceString(direction).c_str());
}

void EdgeEffect::Hide() {
  std::string msg = IsWearableProfile() ? "edge,hide,wearable" : "edge,hide";
  for (int i = TOP; i <= RIGHT; i++) {
    if (visible_[i]) {
      edje_object_signal_emit(
          edge_object_, msg.c_str(),
          GetSourceString(static_cast<Direction>(i)).c_str());
      if (IsWearableProfile())
        edje_object_signal_emit(
            edge_object_, "glow,hide",
            GetSourceString(static_cast<Direction>(i)).c_str());
      visible_[i] = false;
    }
  }
}

void EdgeEffect::Enable() {
  enabled_ = true;
}

void EdgeEffect::ShowObject() {
  DCHECK(edge_object_);
  evas_object_show(edge_object_);
}

void EdgeEffect::HideObject() {
  DCHECK(edge_object_);
  evas_object_hide(edge_object_);
}

void EdgeEffect::Disable() {
  Hide();
  enabled_ = false;
}

void EdgeEffect::UpdatePosition(int top_height, int bottom_height) {
  int x, y, w, h;
  evas_object_geometry_get(parent_view_, &x, &y, &w, &h);

  auto top_edge = y + top_height + bottom_height;
  auto view_height = h - top_height - bottom_height;

  evas_object_move(edge_object_, x, top_edge);
  evas_object_resize(edge_object_, w, view_height);
}

} // namespace content
