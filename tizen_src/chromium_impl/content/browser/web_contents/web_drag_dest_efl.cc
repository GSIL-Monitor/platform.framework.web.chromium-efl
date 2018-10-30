// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/web_contents/web_drag_dest_efl.h"

#include "build/tizen_version.h"
#include "content/browser/renderer_host/render_widget_host_view_efl.h"
#include "content/browser/web_contents/web_contents_impl.h"
#include "content/public/browser/web_drag_dest_delegate.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/render_widget_host.h"
#include "ui/display/screen.h"

using blink::WebDragOperation;

namespace content {

namespace {
static void DragStateEnterCb(void* data, Evas_Object*) {
  WebDragDestEfl* web_drag_dest_efl = static_cast<WebDragDestEfl*>(data);
  web_drag_dest_efl->DragStateEnter();
}

static void DragStateLeaveCb(void* data, Evas_Object*) {
  WebDragDestEfl* web_drag_dest_efl = static_cast<WebDragDestEfl*>(data);
  web_drag_dest_efl->DragStateLeave();
}

static void DragPosCb(void* data, Evas_Object*, Evas_Coord x,
    Evas_Coord y, Elm_Xdnd_Action action) {
  WebDragDestEfl* web_drag_dest_efl = static_cast<WebDragDestEfl*>(data);
  web_drag_dest_efl->DragPos(x, y, action);
}

static Eina_Bool DragDropCb(void* data, Evas_Object*,
    Elm_Selection_Data* drop) {
  WebDragDestEfl *web_drag_dest_efl = static_cast<WebDragDestEfl*>(data);
  return web_drag_dest_efl->DragDrop(drop);
}
} // namespace

WebDragDestEfl::WebDragDestEfl(WebContents* web_contents)
    : modifier_flags_(0),
      delegate_(nullptr),
      web_contents_(web_contents),
      parent_view_(nullptr),
      drag_action_(blink::kWebDragOperationNone),
      page_scale_factor_(1.0f),
      drag_initialized_(false) {
  device_scale_factor_ = display::Screen::GetScreen()->
      GetPrimaryDisplay().device_scale_factor();
  auto rwhv = static_cast<RenderWidgetHostViewEfl*>(
      web_contents->GetRenderWidgetHostView());
  if (!rwhv)
    return;
  parent_view_ = static_cast<Evas_Object*>(rwhv->GetNativeView());
  SetDragCallbacks();
}

WebDragDestEfl::~WebDragDestEfl() {
  UnsetDragCallbacks();
}

void WebDragDestEfl::SetDragCallbacks() {
  elm_drop_target_add(parent_view_, ELM_SEL_FORMAT_TEXT,
      DragStateEnterCb, this,
      DragStateLeaveCb, this,
      DragPosCb, this,
      DragDropCb, this);
}

void WebDragDestEfl::UnsetDragCallbacks() {
  elm_drop_target_del(parent_view_, ELM_SEL_FORMAT_TEXT,
      DragStateEnterCb, this,
      DragStateLeaveCb, this,
      DragPosCb, this,
      DragDropCb, this);
}

void WebDragDestEfl::ResetDragCallbacks() {
  UnsetDragCallbacks();
  SetDragCallbacks();
}

void WebDragDestEfl::DragStateEnter() {
  if (!delegate_)
    return;
  delegate_->DragInitialize(web_contents_);
}

void WebDragDestEfl::DragStateLeave() {
  if (delegate_)
    delegate_->OnDragLeave();
}

void WebDragDestEfl::DragPos(
    Evas_Coord x, Evas_Coord y, Elm_Xdnd_Action action) {
  if (!drop_data_)
    return;

  gfx::Point screen_pt = gfx::Point(x / device_scale_factor_,
                                    y / device_scale_factor_);
  last_pointer_pos_ = screen_pt;
  gfx::Point client_pt = gfx::Point(screen_pt.x() / page_scale_factor_,
                                    screen_pt.y() / page_scale_factor_);

  if (!drag_initialized_) {
    GetRenderWidgetHost()->DragTargetDragEnter(
        *drop_data_, client_pt, screen_pt, drag_action_, modifier_flags_);

    if (delegate_)
      delegate_->OnDragEnter();

    drag_initialized_ = true;
  }

  GetRenderWidgetHost()->DragTargetDragOver(client_pt, screen_pt, drag_action_,
                                            modifier_flags_);

  if (!delegate_)
    return;
  delegate_->OnDragOver();
}

Eina_Bool WebDragDestEfl::DragDrop(Elm_Selection_Data* data) {
  if (!drag_initialized_)
    return EINA_FALSE;

  gfx::Point client_pt =
      gfx::Point(last_pointer_pos_.x() / page_scale_factor_,
                 last_pointer_pos_.y() / page_scale_factor_);

  GetRenderWidgetHost()->FilterDropData(drop_data_.get());
  GetRenderWidgetHost()->DragTargetDrop(*drop_data_, client_pt,
                                        last_pointer_pos_, modifier_flags_);

  if (delegate_)
    delegate_->OnDrop();

  drag_initialized_ = false;

  // Invoking via message loop to not mess around with target
  // from within one of its callbacks.
  base::MessageLoop::current()->task_runner()->PostTask(
      FROM_HERE,
      base::Bind(&WebDragDestEfl::ResetDragCallbacks, base::Unretained(this)));

  return EINA_TRUE;
}

void WebDragDestEfl::DragLeave() {
  GetRenderWidgetHost()->DragTargetDragLeave(gfx::Point(), gfx::Point());
  if (delegate_)
    delegate_->OnDragLeave();

  drop_data_.reset();
}

void WebDragDestEfl::SetDropData(const DropData& drop_data) {
  drop_data_.reset(new DropData(drop_data));
}

void WebDragDestEfl::SetPageScaleFactor(float scale) {
  page_scale_factor_ = scale;
}

RenderWidgetHost* WebDragDestEfl::GetRenderWidgetHost() const {
  return web_contents_->GetRenderViewHost()->GetWidget();
}

}  // namespace content
