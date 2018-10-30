// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Copyright 2014-2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEB_CONTENTS_VIEW_EFL
#define WEB_CONTENTS_VIEW_EFL

#include <Evas.h>

#include "content/browser/renderer_host/render_view_host_delegate_view.h"
#include "content/browser/web_contents/web_contents_view.h"
#include "content/common/content_export.h"
#include "third_party/WebKit/public/platform/WebGestureEvent.h"
#include "third_party/WebKit/public/platform/WebInputEvent.h"
#include "third_party/skia/include/core/SkColor.h"

namespace content {

class WebContents;
class WebContentsViewDelegate;
class WebContentsViewEflDelegate;
class WebDragDestEfl;
class WebDragSourceEfl;
class WebDragDestDelegate;
struct ScreenInfo;

class CONTENT_EXPORT WebContentsViewEfl
    : public content::WebContentsView
    , public content::RenderViewHostDelegateView {
 public:
  WebContentsViewEfl(WebContents* contents,
                     WebContentsViewDelegate* delegate = NULL);
  ~WebContentsViewEfl() override;

  void SetEflDelegate(WebContentsViewEflDelegate*);
  WebContentsViewEflDelegate* GetEflDelegate();

  // content::WebContentsView implementation.
  void CreateView(const gfx::Size&, gfx::NativeView context) override;
  RenderWidgetHostViewBase* CreateViewForWidget(
      RenderWidgetHost* render_widget_host, bool is_guest_view_hack) override;
  RenderWidgetHostViewBase* CreateViewForPopupWidget(
      RenderWidgetHost* render_widget_host) override;
  void SetPageTitle(const base::string16& title) override {}
  void RenderViewCreated(RenderViewHost* host) override;
  void RenderViewSwappedIn(RenderViewHost* host) override;
  void SetOverscrollControllerEnabled(bool enabled) override;
  gfx::NativeView GetNativeView() const override;
  gfx::NativeView GetContentNativeView() const override;
  void GetScreenInfo(ScreenInfo* screen_info) const override;
  gfx::NativeWindow GetTopLevelNativeWindow() const override;
  void GetContainerBounds(gfx::Rect* out) const override;
  void SizeContents(const gfx::Size& size) override;
  void Focus() override;
  void SetInitialFocus() override;
  void StoreFocus() override;
  void RestoreFocus() override;
  DropData* GetDropData() const override;
  gfx::Rect GetViewBounds() const override;

  // content::RenderViewHostDelegateView implementation.
  void ShowContextMenu(RenderFrameHost* render_frame_host,
                       const ContextMenuParams& params) override;
  void ShowContextMenu(const ContextMenuParams& params);
  void ShowPopupMenu(RenderFrameHost* render_frame_host,
                     const gfx::Rect& bounds,
                     int item_height,
                     double item_font_size,
                     int selected_item,
                     const std::vector<MenuItem>& items,
                     bool right_aligned,
                     bool allow_multiple_selection) override;
  void HidePopupMenu() override;
  void UpdateDragDest(RenderViewHost* host);
  void StartDragging(const DropData& drop_data,
                     blink::WebDragOperationsMask allowed_ops,
                     const gfx::ImageSkia& image,
                     const gfx::Vector2d& image_offset,
                     const DragEventSourceInfo& event_info,
                     RenderWidgetHostImpl* source_rwh) override;
  void CancelContextMenu(int request_id);
  bool IsDragging() const;

  void SetOrientation(int orientation);
  int GetOrientation() const { return orientation_; }
  void SetTouchEventsEnabled(bool);
  void SetPageScaleFactor(float);

  void HandleZoomGesture(blink::WebGestureEvent& event);

  bool UseKeyPadWithoutUserAction();

  void SetBackgroundColor(SkColor color) { background_color_ = color; }
  SkColor GetBackgroundColor() const { return background_color_; }
  void OnSelectionRectReceived(const gfx::Rect& selection_rect) const;
  bool ImePanelEnabled() const;

 private:
  // Our optional, generic views wrapper.
  std::unique_ptr<WebContentsViewDelegate> delegate_;

  // Delegate specific to EFL port of chromium.
  // May be NULL in case of non EWK apps.
  std::unique_ptr<WebContentsViewEflDelegate> efl_delegate_;

  Evas_Object* native_view_;

  WebDragDestDelegate* drag_dest_delegate_;

  // Helpers handling drag source/destination related interactions with EFL.
  std::unique_ptr<WebDragSourceEfl> drag_source_;
  std::unique_ptr<WebDragDestEfl> drag_dest_;

  int orientation_;
  bool touch_enabled_;
  float page_scale_factor_;
  SkColor background_color_;

  WebContents* web_contents_;

  DISALLOW_COPY_AND_ASSIGN(WebContentsViewEfl);
};

}

#endif
