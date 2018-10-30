// Copyright 2013 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "selection_box_efl.h"

#include "base/trace_event/trace_event.h"
#include "content/browser/renderer_host/render_widget_host_view_efl.h"

namespace content {

SelectionBoxEfl::SelectionBoxEfl(RenderWidgetHostViewEfl* rwhv)
    : status_(false),
      is_anchor_first_(false),
      context_params_(new ContextMenuParams()),
      rwhv_(rwhv) {
}

SelectionBoxEfl::~SelectionBoxEfl() {}

void SelectionBoxEfl::UpdateSelectStringData(const base::string16& text) {
  context_params_->selection_text = text;
}

void SelectionBoxEfl::ClearRectData() {
  TRACE_EVENT0("selection,efl", __PRETTY_FUNCTION__);
  left_rect_ = right_rect_ = gfx::Rect(0, 0, 0, 0);
  context_params_->x = context_params_->y = 0;
}

bool SelectionBoxEfl::UpdateRectData(const gfx::Rect& left_rect, const gfx::Rect& right_rect) {
  TRACE_EVENT2("selection,efl", __PRETTY_FUNCTION__,
               "left_rect", left_rect.ToString(),
               "right_rect", right_rect.ToString());
  bool ret = false;
  if (left_rect_ != left_rect || right_rect_ != right_rect)
      ret = true;

  left_rect_ = left_rect;
  right_rect_ = right_rect;

  //context params suppose to be global - related to evas not the web view
  gfx::Rect view_bounds = rwhv_->GetViewBoundsInPix();
  ret |= (context_params_->x != (left_rect_.x() + view_bounds.x()));
  ret |= (context_params_->y != (left_rect_.y() + view_bounds.y()));
  context_params_->x = left_rect_.x() + view_bounds.x();
  context_params_->y = left_rect_.y() + view_bounds.y();

  return ret;
}

bool SelectionBoxEfl::IsInEditField() const {
  return (GetEditable() && !(left_rect_.width() && !(left_rect_.height())));
}

void SelectionBoxEfl::SetStatus(bool enable) {
  bool invokeCbAtEnd = status_ != enable;
  status_ = enable;

  // invoke CB only if mode actually changed
  if (invokeCbAtEnd && rwhv_->ewk_view()) {
    evas_object_smart_callback_call(rwhv_->ewk_view(),
                                    "textselection,mode", &status_);
  }
}

}
