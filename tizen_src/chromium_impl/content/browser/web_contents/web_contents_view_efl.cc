// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Copyright 2014-2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/web_contents/web_contents_view_efl.h"

#include <Elementary.h>

#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/logging.h"
#include "base/path_service.h"
#include "base/strings/utf_string_conversions.h"
#include "content/browser/renderer_host/render_widget_host_view_efl.h"
#include "content/browser/web_contents/web_contents_impl.h"
#include "content/browser/web_contents/web_drag_dest_efl.h"
#include "content/browser/web_contents/web_drag_source_efl.h"
#include "content/common/paths_efl.h"
#include "content/public/browser/interstitial_page.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/web_contents_view_delegate.h"
#include "content/public/browser/web_contents_view_efl_delegate.h"
#include "efl/window_factory.h"
#include "tizen/system_info.h"
#include "ui/display/screen.h"
#include "ui/events/event_switches.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/gl/gl_shared_context_efl.h"

namespace content {

// static
void WebContentsView::GetDefaultScreenInfo(ScreenInfo* results) {
  const display::Display display =
      display::Screen::GetScreen()->GetPrimaryDisplay();

  results->rect = display.bounds();
  results->available_rect = display.work_area();
  results->device_scale_factor = display.device_scale_factor();
  results->orientation_angle = display.RotationAsDegree();

  if (IsMobileProfile() || IsWearableProfile()) {
    results->orientation_type =
        RenderWidgetHostViewBase::GetOrientationTypeForMobile(display);
  } else {
    results->orientation_type =
        RenderWidgetHostViewBase::GetOrientationTypeForDesktop(display);
  }

  // TODO(derat|oshima): Don't hardcode this. Get this from display object.
  results->depth = 24;
  results->depth_per_component = 8;
}

WebContentsView* CreateWebContentsView(
    WebContentsImpl* web_contents,
    WebContentsViewDelegate* delegate,
    RenderViewHostDelegateView** render_view_host_delegate_view) {
  WebContentsViewEfl* view = new WebContentsViewEfl(web_contents, delegate);
  *render_view_host_delegate_view = view;
  return view;
}

WebContentsViewEfl::WebContentsViewEfl(WebContents* contents,
                                       WebContentsViewDelegate* delegate)
    : delegate_(delegate),
      native_view_(NULL),
      drag_dest_delegate_(NULL),
      orientation_(0),
      touch_enabled_(IsMobileProfile() || IsWearableProfile()),
      page_scale_factor_(1.0f),
      background_color_(SK_ColorWHITE),
      web_contents_(contents) {
  base::CommandLine* cmdline = base::CommandLine::ForCurrentProcess();
  if (cmdline->HasSwitch(switches::kTouchEvents)) {
    touch_enabled_ = cmdline->GetSwitchValueASCII(switches::kTouchEvents) ==
                     switches::kTouchEventsEnabled;
  }
}

WebContentsViewEfl::~WebContentsViewEfl() {}

void WebContentsViewEfl::OnSelectionRectReceived(
    const gfx::Rect& selection_rect) const {
  delegate_->OnSelectionRectReceived(selection_rect);
}

void WebContentsViewEfl::SetEflDelegate(WebContentsViewEflDelegate* delegate) {
  efl_delegate_.reset(delegate);
}

WebContentsViewEflDelegate* WebContentsViewEfl::GetEflDelegate() {
  return efl_delegate_.get();
}

void WebContentsViewEfl::CreateView(const gfx::Size& initial_size,
                                    gfx::NativeView context) {
  Evas_Object* root_window = efl::WindowFactory::GetHostWindow(web_contents_);

  native_view_ = elm_layout_add(root_window);

  base::FilePath edj_dir;
  PathService::Get(PathsEfl::EDJE_RESOURCE_DIR, &edj_dir);

  base::FilePath main_edj = edj_dir.Append(FILE_PATH_LITERAL("MainLayout.edj"));
  elm_layout_file_set(native_view_, main_edj.AsUTF8Unsafe().c_str(),
                      "main_layout");

  GLSharedContextEfl::Initialize(root_window);

  if (delegate_)
    drag_dest_delegate_ = delegate_->GetDragDestDelegate();
}

RenderWidgetHostViewBase* WebContentsViewEfl::CreateViewForWidget(
    RenderWidgetHost* render_widget_host,
    bool is_guest_view_hack) {
  RenderWidgetHostViewEfl* view =
      new RenderWidgetHostViewEfl(render_widget_host, *web_contents_);
  view->InitAsChild(nullptr);
  view->SetTouchEventsEnabled(touch_enabled_);
  view->Show();

#if defined(TIZEN_VD_LFD_ROTATE)
  orientation_ =
      ecore_evas_rotation_get(ecore_evas_ecore_evas_get(view->evas()));
#endif

  SetOrientation(orientation_);

  RenderViewHost* render_view_host = web_contents_->GetRenderViewHost();
  UpdateDragDest(render_view_host);

  return view;
}

RenderWidgetHostViewBase* WebContentsViewEfl::CreateViewForPopupWidget(
    RenderWidgetHost* render_widget_host) {
  return new RenderWidgetHostViewEfl(render_widget_host, *web_contents_);
}

void WebContentsViewEfl::CancelContextMenu(int request_id) {
  if (efl_delegate_)
    efl_delegate_->CancelContextMenu(request_id);
}

void WebContentsViewEfl::UpdateDragDest(RenderViewHost* host) {
  // Drag-and-drop is entirely managed by BrowserPluginGuest for guest
  // processes in a largely platform independent way. WebDragDestEfl
  // will result in spurious messages being sent to the guest process which
  // will violate assumptions.
  if (host->GetProcess() && host->GetProcess()->IsForGuestsOnly()) {
    DCHECK(!drag_dest_);
    return;
  }

  RenderWidgetHostViewEfl* view = static_cast<RenderWidgetHostViewEfl*>(
      web_contents_->GetRenderWidgetHostView());
  if (!view)
    return;

  // Create the new drag_dest_.
  drag_dest_.reset(new WebDragDestEfl(web_contents_));
  drag_dest_->SetPageScaleFactor(page_scale_factor_);

  if (delegate_)
    drag_dest_->SetDelegate(delegate_->GetDragDestDelegate());
}

void WebContentsViewEfl::RenderViewCreated(RenderViewHost* host) {
  DCHECK(host->GetProcess());
  DCHECK(web_contents_);
}

void WebContentsViewEfl::RenderViewSwappedIn(RenderViewHost* host) {
  UpdateDragDest(host);
}

void WebContentsViewEfl::SetOverscrollControllerEnabled(bool enabled) {}

gfx::NativeView WebContentsViewEfl::GetNativeView() const {
  return native_view_;
}

gfx::NativeView WebContentsViewEfl::GetContentNativeView() const {
  RenderWidgetHostView* rwhv = web_contents_->GetRenderWidgetHostView();
  return rwhv ? rwhv->GetNativeView() : NULL;
}

void WebContentsViewEfl::GetScreenInfo(ScreenInfo* screen_info) const {
  WebContentsView::GetDefaultScreenInfo(screen_info);
}

gfx::NativeWindow WebContentsViewEfl::GetTopLevelNativeWindow() const {
  NOTIMPLEMENTED();
  return 0;
}

void WebContentsViewEfl::GetContainerBounds(gfx::Rect* out) const {
  RenderWidgetHostViewEfl* view = static_cast<RenderWidgetHostViewEfl*>(
      web_contents_->GetRenderWidgetHostView());
  if (view)
    *out = view->GetBoundsInRootWindow();
}

void WebContentsViewEfl::SizeContents(const gfx::Size& size) {
  RenderWidgetHostView* rwhv = web_contents_->GetRenderWidgetHostView();
  if (rwhv)
    rwhv->SetSize(size);
}

void WebContentsViewEfl::Focus() {
  if (web_contents_->GetInterstitialPage()) {
    web_contents_->GetInterstitialPage()->Focus();
    return;
  }

  if (delegate_.get() && delegate_->Focus())
    return;

  RenderWidgetHostView* rwhv =
      web_contents_->GetFullscreenRenderWidgetHostView();
  if (!rwhv)
    rwhv = web_contents_->GetRenderWidgetHostView();
  if (rwhv)
    rwhv->Focus();
}

void WebContentsViewEfl::SetInitialFocus() {
  NOTIMPLEMENTED();
}

void WebContentsViewEfl::StoreFocus() {
  NOTIMPLEMENTED();
}

void WebContentsViewEfl::RestoreFocus() {
  NOTIMPLEMENTED();
}

DropData* WebContentsViewEfl::GetDropData() const {
  if (drag_dest_.get())
    return drag_dest_->GetDropData();
  return nullptr;
}

gfx::Rect WebContentsViewEfl::GetViewBounds() const {
  int x, y, w, h;
  evas_object_geometry_get(native_view_, &x, &y, &w, &h);
  return gfx::Rect(x, y, w, h);
}

void WebContentsViewEfl::ShowContextMenu(RenderFrameHost* render_frame_host,
                                         const ContextMenuParams& params) {
  if (delegate_)
    delegate_->ShowContextMenu(render_frame_host, params);
}

// Note: This is a EFL specific extension to WebContentsView responsible
// for actually displaying a context menu UI.
// On the other side, its overload method (above) is called by chromium
// in response to a long press gesture. It is up to the embedder to define
// how this long press gesture will be
// handled, e.g. showing context menu right away or afte user lifts up his
// finger.
void WebContentsViewEfl::ShowContextMenu(const ContextMenuParams& params) {
  if (efl_delegate_)
    efl_delegate_->ShowContextMenu(params);
}

void WebContentsViewEfl::ShowPopupMenu(RenderFrameHost* render_frame_host,
                                       const gfx::Rect& bounds,
                                       int item_height,
                                       double item_font_size,
                                       int selected_item,
                                       const std::vector<MenuItem>& items,
                                       bool right_aligned,
                                       bool allow_multiple_selection) {
  if (efl_delegate_)
    efl_delegate_->ShowPopupMenu(render_frame_host, bounds, item_height,
                                 item_font_size, selected_item, items,
                                 right_aligned, allow_multiple_selection);
}

void WebContentsViewEfl::HidePopupMenu() {
  if (efl_delegate_)
    efl_delegate_->HidePopupMenu();
}

void WebContentsViewEfl::StartDragging(const DropData& drop_data,
                                       blink::WebDragOperationsMask allowed_ops,
                                       const gfx::ImageSkia& image,
                                       const gfx::Vector2d& image_offset,
                                       const DragEventSourceInfo& event_info,
                                       RenderWidgetHostImpl* source_rwh) {
  if (drag_dest_.get()) {
    drag_dest_->SetDropData(drop_data);
    drag_dest_->SetAction(allowed_ops);
  }

  if (!drag_source_) {
    drag_source_.reset(new WebDragSourceEfl(web_contents_));
    drag_source_->SetPageScaleFactor(page_scale_factor_);
  }

  if (!drag_source_->StartDragging(drop_data, allowed_ops,
                                   event_info.event_location, *image.bitmap(),
                                   image_offset, source_rwh)) {
    web_contents_->SystemDragEnded(source_rwh);
  }
}

bool WebContentsViewEfl::IsDragging() const {
  if (!drag_source_)
    return false;
  return drag_source_->IsDragging();
}

void WebContentsViewEfl::SetOrientation(int orientation) {
  RenderWidgetHostViewEfl* rwhv = static_cast<RenderWidgetHostViewEfl*>(
      web_contents_->GetRenderWidgetHostView());

  if (rwhv) {
    WebContentsImpl& contents_impl =
        static_cast<WebContentsImpl&>(*web_contents_);

    rwhv->UpdateScreenInfo(rwhv->GetNativeView());
    contents_impl.OnScreenOrientationChange();
    rwhv->UpdateRotationDegrees(orientation);
  }

  orientation_ = orientation;
}

void WebContentsViewEfl::SetTouchEventsEnabled(bool enabled) {
  touch_enabled_ = enabled;
  RenderWidgetHostViewEfl* rwhv = static_cast<RenderWidgetHostViewEfl*>(
      web_contents_->GetRenderWidgetHostView());
  if (rwhv)
    rwhv->SetTouchEventsEnabled(enabled);
}

void WebContentsViewEfl::SetPageScaleFactor(float scale) {
  page_scale_factor_ = scale;
  if (drag_source_)
    drag_source_->SetPageScaleFactor(scale);
  if (drag_dest_)
    drag_dest_->SetPageScaleFactor(scale);
}

void WebContentsViewEfl::HandleZoomGesture(blink::WebGestureEvent& event) {
  if (efl_delegate_)
    efl_delegate_->HandleZoomGesture(event);
}

bool WebContentsViewEfl::UseKeyPadWithoutUserAction() {
  if (efl_delegate_)
    return efl_delegate_->UseKeyPadWithoutUserAction();
  return false;
}

bool WebContentsViewEfl::ImePanelEnabled() const {
  if (efl_delegate_)
    return efl_delegate_->ImePanelEnabled();
  return true;
}

}  // namespace content
