// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_WEB_CONTENTS_WEB_DRAG_DEST_EFL_H_
#define CONTENT_BROWSER_WEB_CONTENTS_WEB_DRAG_DEST_EFL_H_

#include <Elementary.h>

#include "content/public/common/drop_data.h"
#include "third_party/WebKit/public/platform/WebDragOperation.h"
#include "ui/gfx/geometry/point.h"

namespace content {

class WebContents;
class WebDragDestDelegate;
class RenderWidgetHost;

class WebDragDestEfl {
 public:
  WebDragDestEfl(WebContents* web_contents);
  ~WebDragDestEfl();

  blink::WebDragOperationsMask GetAction() { return drag_action_; }
  WebDragDestDelegate* GetDelegate() const { return delegate_; }
  DropData* GetDropData() { return drop_data_.get(); }
  RenderWidgetHost* GetRenderWidgetHost() const;
  WebContents* GetWebContents() { return web_contents_; }

  void ResetDropData() { drop_data_.reset(); }
  void SetAction(blink::WebDragOperationsMask allowed_ops) { drag_action_ = allowed_ops; }
  void SetDelegate(WebDragDestDelegate* delegate) { delegate_ = delegate; }
  void SetDropData(const DropData& drop_data);
  void SetPageScaleFactor(float);

  // Informs the renderer when a system drag has left the render view.
  // See OnDragLeave().
  void DragLeave();

  // EFL callbacks
  void DragStateEnter();
  void DragStateLeave();
  void DragPos(Evas_Coord x, Evas_Coord y, Elm_Xdnd_Action action);
  Eina_Bool DragDrop(Elm_Selection_Data *);

 private:
  void SetDragCallbacks();
  void UnsetDragCallbacks();
  void ResetDragCallbacks();

  // Flag that indicates which modifier buttons (alt, shift, ctrl) are pressed
  const int modifier_flags_;

  // A delegate that can receive drag information about drag events.
  WebDragDestDelegate* delegate_;

  WebContents* web_contents_;
  Evas_Object* parent_view_;

  // Drag operation - copy, move or link
  blink::WebDragOperationsMask drag_action_;

  float device_scale_factor_;
  float page_scale_factor_;

  gfx::Point last_pointer_pos_;

  bool drag_initialized_;

  // The data for the current drag, or NULL if |context_| is NULL.
  std::unique_ptr<DropData> drop_data_;

  DISALLOW_COPY_AND_ASSIGN(WebDragDestEfl);
};
}  // namespace content

#endif  // CONTENT_BROWSER_WEB_CONTENTS_WEB_DRAG_DEST_EFL_H_
