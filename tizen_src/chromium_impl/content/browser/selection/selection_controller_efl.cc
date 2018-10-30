// Copyright 2013-2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "selection_controller_efl.h"

#include <Elementary.h>

#include "base/trace_event/trace_event.h"
#include "content/browser/renderer_host/render_view_host_impl.h"
#include "content/browser/renderer_host/render_widget_host_impl.h"
#include "content/browser/renderer_host/render_widget_host_view_efl.h"
#include "content/browser/web_contents/web_contents_impl.h"
#include "content/browser/web_contents/web_contents_impl_efl.h"
#include "content/common/view_messages.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/context_menu_params.h"
#include "tizen/system_info.h"
#include "ui/base/clipboard/clipboard_helper_efl.h"
#include "ui/display/device_display_info_efl.h"
#include "ui/display/screen.h"
#include "ui/gfx/geometry/dip_util.h"

namespace content {

// The space between selection menu and selection area.
static const int kMenuPadding = 10;

// The height of the large handle.
static const int kLargeHandleHeight = 50;

// The height of the selection range handle.
static const int kRangeHandleHeight = 50;

// Scroll step when selection handler is moving out of viewport.
static const int textSelectionScrollSize = 50;

bool IsRectEmpty(const gfx::Rect& rect) {
  return rect == gfx::Rect();
}

gfx::Rect IntersectRects(const gfx::Rect& r1,
                         const gfx::Rect& r2,
                         bool ignore_width = false) {
  if (!ignore_width)
    return gfx::IntersectRects(r1, r2);

  int rx = std::max(r1.x(), r2.x());
  int ry = std::max(r1.y(), r2.y());
  int rb = std::min(r1.bottom(), r2.bottom());

  if (ry >= rb)
    return gfx::Rect();
  return gfx::Rect(rx, ry, 0, rb - ry);
}

gfx::Vector2dF ComputeLineOffsetFromBottom(const gfx::SelectionBound& bound) {
  gfx::Vector2dF line_offset =
      gfx::ScaleVector2d(bound.edge_top() - bound.edge_bottom(), 0.5f);
  // An offset of 8 DIPs is sufficient for most line sizes. For small lines,
  // using half the line height avoids synthesizing a point on a line above
  // (or below) the intended line.
  const gfx::Vector2dF kMaxLineOffset(8.f, 8.f);
  line_offset.SetToMin(kMaxLineOffset);
  line_offset.SetToMax(-kMaxLineOffset);
  return line_offset;
}

content::WebContents* SelectionControllerEfl::web_contents() const {
  return rwhv_->web_contents();
}

SelectionControllerEfl::SelectionControllerEfl(RenderWidgetHostViewEfl* rwhv)
    : rwhv_(rwhv),
      controls_temporarily_hidden_(false),
      selection_change_reason_(Reason::Irrelevant),
      long_mouse_press_(false),
      selection_data_(new SelectionBoxEfl(rwhv_)),
      start_handle_(
          new SelectionHandleEfl(*this, SelectionHandleEfl::HANDLE_TYPE_LEFT)),
      end_handle_(
          new SelectionHandleEfl(*this, SelectionHandleEfl::HANDLE_TYPE_RIGHT)),
      input_handle_(
          new SelectionHandleEfl(*this, SelectionHandleEfl::HANDLE_TYPE_INPUT)),
      magnifier_(new SelectionMagnifierEfl(this)),
      dragging_handle_(nullptr),
      stationary_handle_(nullptr),
      handle_being_dragged_(false),
      selection_on_empty_form_control_(false),
      is_caret_mode_forced_(false),
      ecore_events_filter_(nullptr),
      dragged_handle_(nullptr),
      context_menu_status_(ContextMenuStatus::NONE),
      triggered_selection_change_(false),
      selection_mode_(SelectionMode::NONE) {
  evas_object_event_callback_add(rwhv_->content_image(), EVAS_CALLBACK_MOVE,
                                 &EvasParentViewMoveCallback, this);
#if defined(OS_TIZEN)
  vconf_notify_key_changed(VCONFKEY_LANGSET, PlatformLanguageChanged, this);
#endif
#if defined(OS_TIZEN_TV_PRODUCT)
  // FIXME: To check who will enable touch event on TV
  LOG(ERROR) << "Touch event enabled, is it what you want?";
#endif
}

SelectionControllerEfl::~SelectionControllerEfl() {
  if (ecore_events_filter_)
    ecore_event_filter_del(ecore_events_filter_);

#if defined(OS_TIZEN)
  vconf_ignore_key_changed(VCONFKEY_LANGSET, PlatformLanguageChanged);
#endif

  if (GetSelectionStatus())
    ClearSelectionViaEWebView();
  HideHandleAndContextMenu();

  evas_object_event_callback_del(rwhv_->content_image(), EVAS_CALLBACK_MOVE,
                                 &EvasParentViewMoveCallback);
}

void SelectionControllerEfl::SetSelectionStatus(bool enable) {
  TRACE_EVENT1("selection,efl", __PRETTY_FUNCTION__, "enable", enable);
  selection_data_->SetStatus(enable);
}

bool SelectionControllerEfl::GetSelectionStatus() const {
  TRACE_EVENT1("selection,efl", __PRETTY_FUNCTION__,
               "status", selection_data_->GetStatus());
  return selection_data_->GetStatus();
}

#if defined(OS_TIZEN)
void SelectionControllerEfl::PlatformLanguageChanged(keynode_t* keynode, void* data) {
  SelectionControllerEfl* selection_controller = static_cast<SelectionControllerEfl*>(data);
  if (!selection_controller || !selection_controller->rwhv_)
    return;

  if (selection_controller->GetSelectionStatus())
    selection_controller->ClearSelectionViaEWebView();
  selection_controller->HideHandleAndContextMenu();
}
#endif

void SelectionControllerEfl::SetSelectionEditable(bool enable) {
  TRACE_EVENT1("selection,efl", __PRETTY_FUNCTION__, "enable", enable);
  selection_data_->SetEditable(enable);
}

bool SelectionControllerEfl::GetSelectionEditable() const {
  TRACE_EVENT1("selection,efl", __PRETTY_FUNCTION__,
               "editable", selection_data_->GetEditable());
  return selection_data_->GetEditable();
}

bool SelectionControllerEfl::GetCaretSelectionStatus() const {
  TRACE_EVENT1("selection,efl", __PRETTY_FUNCTION__, "caret selection",
               selection_mode_ == SelectionMode::CARET);
  return selection_mode_ == SelectionMode::CARET;
}

void SelectionControllerEfl::SetControlsTemporarilyHidden(
    bool value,
    bool from_custom_scroll_callback) {
  TRACE_EVENT1("selection,efl", __PRETTY_FUNCTION__,
               "controls are hidden:", value);
  if (controls_temporarily_hidden_ == value)
    return;

  // Make sure to show selection controls only when no finger
  // is left touching the screen.
  if (!value && rwhv_->PointerStatePointerCount() &&
      !from_custom_scroll_callback)
    return;

  controls_temporarily_hidden_ = value;
  if (value) {
    Clear();
    CancelContextMenu(0);
  } else {
    ShowHandleAndContextMenuIfRequired(Reason::ScrollOrZoomGestureEnded);
  }
}

void SelectionControllerEfl::SetIsAnchorFirst(bool value) {
  selection_data_->SetIsAnchorFirst(value);
}

void SelectionControllerEfl::TriggerOnSelectionChange() {
  triggered_selection_change_ = true;
  OnSelectionChanged(start_selection_, end_selection_);
}

void SelectionControllerEfl::OnSelectionChanged(
    const gfx::SelectionBound& start, const gfx::SelectionBound& end) {
  if (start_selection_ == start && end_selection_ == end &&
      !triggered_selection_change_)
    return;

  if (selection_change_reason_ != Reason::HandleDragged)
    HideHandleAndContextMenu();

  start_selection_ = start;
  end_selection_ = end;

  gfx::Rect truncated_start(start.edge_top_rounded(),
                            gfx::Size(0, start.GetHeight()));
  gfx::Rect truncated_end(end.edge_top_rounded(),
                          gfx::Size(0, end.GetHeight()));

  static float device_scale_factor =
      display::Screen::GetScreen()->GetPrimaryDisplay().device_scale_factor();
  truncated_start = ConvertRectToPixel(device_scale_factor, truncated_start);
  truncated_end = ConvertRectToPixel(device_scale_factor, truncated_end);

  if (handle_being_dragged_ && dragging_handle_ &&
      dragging_handle_->Type() != SelectionHandleEfl::HANDLE_TYPE_INPUT) {
    dragging_handle_->MoveObject(selection_data_->GetIsAnchorFirst()
                                     ? truncated_end.bottom_right()
                                     : truncated_start.bottom_left());
  }

  bool finger_down = handle_being_dragged_ || long_mouse_press_;
  bool show = (selection_change_reason_ != Reason::Irrelevant) && !finger_down;
  UpdateSelectionDataAndShow(
      truncated_start, truncated_end, show);
  triggered_selection_change_ = false;
  selection_change_reason_ = Reason::Irrelevant;

  // In case of the selected text contains only line break and no other
  // characters, we should use caret selection mode.
  if (GetSelectionEditable() && !handle_being_dragged_ &&
      rwhv_->GetSelectedText() == base::UTF8ToUTF16("\n")) {
    rwhv_->MoveCaret(selection_data_->GetLeftRect().origin());
    is_caret_mode_forced_ = true;
  }
}

void SelectionControllerEfl::OnTextInputStateChanged() {
  // In order to confirm if a long press gesture is ongoing,
  // we just needed to check "long_mouse_press_".
  // However, when long press happens on a link or image,
  // long_mouse_press_ is set to false in ::HandleLongPressEvent,
  // although finger is still down.
  // To compensate this, add an extra check asking from the engine
  // if a finger is still touching down.
  bool is_touch_down = rwhv_->PointerStatePointerCount() > 0;

  // If on an editable field and in caret selection mode, only
  // show the large handle if:
  // 1) we are dragging the insertion handle around;
  // 2) we are expecting an "update" from the engine, e.g.
  //    handling "tap" gesture.
  bool finger_down = handle_being_dragged_ || long_mouse_press_ || is_touch_down;

  if (GetSelectionEditable() && GetCaretSelectionStatus() && !finger_down &&
      !controls_temporarily_hidden_ &&
      selection_change_reason_ == Reason::Irrelevant) {
    HideHandleAndContextMenu();
    ClearSelection();
  }

  if (selection_change_reason_ == Reason::CaretModeForced) {
    is_caret_mode_forced_ = false;
    selection_change_reason_ = Reason::Irrelevant;
  }
}

void SelectionControllerEfl::UpdateSelectionData(const base::string16& text) {
  selection_data_->UpdateSelectStringData(text);
}

bool SelectionControllerEfl::ClearSelectionViaEWebView() {
  if (!GetSelectionStatus() || !web_contents()->GetRenderViewHost() ||
      web_contents()->IsBeingDestroyed()) {
    return false;
  }

  RenderWidgetHostDelegate* host_delegate =
      RenderWidgetHostImpl::From(
          web_contents()->GetRenderViewHost()->GetWidget())
          ->delegate();
  if (host_delegate) {
    host_delegate->ExecuteEditCommand("Unselect", base::nullopt);
    return true;
  }
  return false;
}

void SelectionControllerEfl::SetSelectionMode(enum SelectionMode mode) {
  selection_mode_ = mode;
}

void SelectionControllerEfl::ToggleCaretAfterSelection() {
  if (!GetCaretSelectionStatus() && GetSelectionEditable()) {
    rwhv_->MoveCaret(selection_data_->GetRightRect().origin());
    ClearSelection();
  }
}

void SelectionControllerEfl::DetermineSelectionMode(
    const gfx::Rect& left_rect,
    const gfx::Rect& right_rect) {

  if (left_rect == gfx::Rect() && right_rect == gfx::Rect())
    SetSelectionMode(SelectionMode::NONE);
  else if (left_rect == right_rect && GetSelectionEditable())
    SetSelectionMode(SelectionMode::CARET);
  else
    SetSelectionMode(SelectionMode::RANGE);
}

bool SelectionControllerEfl::UpdateSelectionDataAndShow(
    const gfx::Rect& left_rect,
    const gfx::Rect& right_rect,
    bool /* show */) {
  TRACE_EVENT0("selection,efl", __PRETTY_FUNCTION__);

  DetermineSelectionMode(left_rect, right_rect);
  bool selection_changed =
      selection_data_->UpdateRectData(left_rect, right_rect);

  if ((selection_change_reason_ == Reason::Irrelevant) && !IsSelectionValid(left_rect, right_rect)) {
    if (!GetCaretSelectionStatus())
      ClearSelection();
    return false;
  }

  if (selection_on_empty_form_control_ &&
      (!ClipboardHelperEfl::GetInstance()->CanPasteFromSystemClipboard() ||
       selection_change_reason_ == Reason::Tap)) {
    ClearSelection();
    return false;
  }

  if (selection_changed || triggered_selection_change_) {
    ShowHandleAndContextMenuIfRequired();
  }

  return true;
}

float SelectionControllerEfl::device_scale_factor() const {
  return display::Screen::GetScreen()
      ->GetPrimaryDisplay()
      .device_scale_factor();
}

bool SelectionControllerEfl::IsCaretMode() const {
  return selection_data_->IsInEditField() && GetLeftRect() == GetRightRect();
}

void SelectionControllerEfl::ShowHandleAndContextMenuIfRequired(
    Reason explicit_reason) {
  TRACE_EVENT0("selection,efl", __PRETTY_FUNCTION__);

  Reason saved_reason = selection_change_reason_;
  Reason effective_reason = selection_change_reason_;

  if (explicit_reason != Reason::Irrelevant)
    effective_reason = explicit_reason;

  // TODO(a1.gomes): Is not in selection mode, maybe we should not be this far
  // to begin with.
  if (!selection_data_->GetStatus())
    return;

  if (controls_temporarily_hidden_ || long_mouse_press_)
    return;

  gfx::Rect left, right;
  left = selection_data_->GetLeftRect();
  right = selection_data_->GetRightRect();

  if (left.x() == 0 && left.y() == 0 && right.x() == 0 && right.y() == 0) {
    selection_data_->ClearRectData();
    return;
  }

  bool is_start_selection_visible = start_selection_.visible();
  bool is_end_selection_visible = end_selection_.visible();

  // When the view is only partially visible we need to check if
  // the visible part contains selection rects.
  if (!rwhv_->GetCustomEmailViewportRect().IsEmpty() ||
      rwhv_->GetTopControlsHeight() > 0) {
    auto visible_viewport_rect = GetVisibleViewportRect();
    auto view_offset = rwhv_->GetViewBoundsInPix().OffsetFromOrigin();
    is_start_selection_visible =
        is_start_selection_visible &&
        !IsRectEmpty(IntersectRects(visible_viewport_rect, left + view_offset,
                                    true /*ignore_width*/));
    is_end_selection_visible =
        is_end_selection_visible &&
        !IsRectEmpty(IntersectRects(visible_viewport_rect, right + view_offset,
                                    true /*ignore_width*/));
  }

  // Is in edit field and no text is selected. show only single handle
  if (selection_data_->IsInEditField() && left == right) {
    if (!GetSelectionEditable())
      return;

    CHECK(start_selection_ == end_selection_);

    if (GetCaretSelectionStatus() && is_start_selection_visible) {
      input_handle_->SetBasePosition(gfx::Point(left.x(), left.y()));
      input_handle_->Move(left.bottom_right());
      input_handle_->Show();
    }

    bool show_context_menu =
        input_handle_->IsVisible() &&
        effective_reason != Reason::ScrollOrZoomGestureEnded &&
        effective_reason != Reason::Tap &&
        effective_reason != Reason::Irrelevant;
    if (!handle_being_dragged_ && show_context_menu) {
      selection_change_reason_ = saved_reason;
      ShowContextMenu();
    }
    // In order to keep selection controlls showing up,
    // Reason::ForceCaretMode should be used as a reason of selection change.
    if (is_caret_mode_forced_)
      selection_change_reason_ = Reason::CaretModeForced;

    return;
  }
  input_handle_->Hide();

  SelectionHandleEfl* start_handle = start_handle_.get();
  SelectionHandleEfl* end_handle = end_handle_.get();
  if (handle_being_dragged_) {
    bool is_anchor_first = selection_data_->GetIsAnchorFirst();
    if (is_anchor_first) {
      start_handle = stationary_handle_;
      end_handle = dragging_handle_;
    } else {
      start_handle = dragging_handle_;
      end_handle = stationary_handle_;
    }
  }

  start_handle_->SetBasePosition(left.bottom_left());
  start_handle->Move(left.bottom_left());
  if (is_start_selection_visible)
    start_handle->Show();
  else
    start_handle->Hide();

  end_handle_->SetBasePosition(right.bottom_right());
  end_handle->Move(right.bottom_right());
  if (is_end_selection_visible)
    end_handle->Show();
  else
    end_handle->Hide();

  // Do not show the context menu during selection extend and
  // offscreen selection.
  if (!handle_being_dragged_ && selection_mode_ != SelectionMode::NONE) {
    selection_change_reason_ = saved_reason;
    ShowContextMenu();
  }
}

void SelectionControllerEfl::ShowContextMenu() {
  if (IsMobileProfile() &&
      context_menu_status_ == ContextMenuStatus::INPROGRESS)
    return;
  else if (IsMobileProfile())
    context_menu_status_ = ContextMenuStatus::INPROGRESS;

  content::ContextMenuParams convertedParams = *(selection_data_->GetContextMenuParams());

  if (rwhv_) {
    int blinkX, blinkY;
    rwhv_->EvasToBlinkCords(convertedParams.x, convertedParams.y, &blinkX,
                            &blinkY);
    convertedParams.x = blinkX;
    convertedParams.y = blinkY;

    // TODO(a1.gomes): In case of EWK apps, the call below end up calling
    // EWebView::ShowContextMenu. We have to make sure parameters
    // are correct.
    auto wci = static_cast<WebContentsImpl*>(web_contents());
    WebContentsViewEfl* wcve = static_cast<WebContentsViewEfl*>(wci->GetView());
    wcve->ShowContextMenu(convertedParams);
  }
}

void SelectionControllerEfl::CancelContextMenu(int request_id) {
  if (IsMobileProfile() && (context_menu_status_ == ContextMenuStatus::HIDDEN ||
                            context_menu_status_ == ContextMenuStatus::NONE))
    return;

  auto wci = static_cast<WebContentsImpl*>(web_contents());
  WebContentsViewEfl* wcve = static_cast<WebContentsViewEfl*>(wci->GetView());

  wcve->CancelContextMenu(request_id);
}

void SelectionControllerEfl::HideHandles() {
  Clear();
}

void SelectionControllerEfl::HideHandleAndContextMenu() {
  CancelContextMenu(0);
  HideHandles();
}

bool SelectionControllerEfl::IsAnyHandleVisible() const {
  return (start_handle_->IsVisible() ||
          end_handle_->IsVisible() ||
          input_handle_->IsVisible());
}

void SelectionControllerEfl::Clear() {
  start_handle_->Hide();
  end_handle_->Hide();
  input_handle_->Hide();
}

Eina_Bool SelectionControllerEfl::EcoreEventFilterCallback(void* user_data,
                                                           void* /*loop_data*/,
                                                           int type,
                                                           void* event_info) {
  if (type != ECORE_EVENT_MOUSE_MOVE)
    return ECORE_CALLBACK_PASS_ON;

  auto controller = static_cast<SelectionControllerEfl*>(user_data);
  if (!controller || !controller->dragged_handle_)
    return ECORE_CALLBACK_PASS_ON;

  // Point from Ecore_Event_Mouse_Move event is not rotation aware, so
  // we have to rotate it manually. We use display dimensions here,
  // because both point from the event, and selection handle's position
  // are in display coordinates (not webview).
  auto event = static_cast<Ecore_Event_Mouse_Move*>(event_info);
  display::DeviceDisplayInfoEfl display_info;
  int rotation = display_info.GetRotationDegrees();
  int display_width = display_info.GetDisplayWidth();
  int display_height = display_info.GetDisplayHeight();
  gfx::Point converted_point;
  switch (rotation) {
    case 0:
      converted_point = gfx::Point(event->x, event->y);
      break;
    case 90:
      converted_point = gfx::Point(display_height - event->y, event->x);
      break;
    case 180:
      converted_point =
          gfx::Point(display_width - event->x, display_height - event->y);
      break;
    case 270:
      converted_point = gfx::Point(event->y, display_width - event->x);
      break;
    default:
      LOG(ERROR) << "Unexpected rotation value: " << rotation;
      converted_point = gfx::Point(event->x, event->y);
  }
  controller->dragged_handle_->UpdatePosition(converted_point);

  return ECORE_CALLBACK_CANCEL;
}

void SelectionControllerEfl::HandleDragBeginNotification(
    SelectionHandleEfl* handle) {
  selection_change_reason_ = Reason::HandleDragged;
  dragged_handle_ = handle;
  ecore_events_filter_ =
      ecore_event_filter_add(nullptr, EcoreEventFilterCallback, nullptr, this);

  if (!ecore_events_filter_) {
    LOG(ERROR) << __PRETTY_FUNCTION__
               << " : Unable to create ecore events filter";
  }

  // Hide context menu on mouse down
  CancelContextMenu(0);

  handle_being_dragged_ = true;

  if (handle == input_handle_.get())
    return;

  gfx::Vector2dF base_offset, extent_offset;

  // When moving the handle we want to move only the extent point.
  // Before doing so, we must make sure that the base point is set correctly.
  if (handle == start_handle_.get()) {
    dragging_handle_ = start_handle_.get();
    stationary_handle_ = end_handle_.get();

    base_offset = ComputeLineOffsetFromBottom(end_selection_);
    extent_offset = ComputeLineOffsetFromBottom(start_selection_);
  } else {
    dragging_handle_ = end_handle_.get();
    stationary_handle_ = start_handle_.get();

    base_offset = ComputeLineOffsetFromBottom(start_selection_);
    extent_offset = ComputeLineOffsetFromBottom(end_selection_);
  }

  float device_scale_factor =
      display::Screen::GetScreen()->GetPrimaryDisplay().device_scale_factor();
  gfx::PointF base = gfx::ScalePoint(
      gfx::PointF(stationary_handle_->GetBasePosition()), 1 / device_scale_factor);
  gfx::PointF extent = gfx::ScalePoint(
      gfx::PointF(dragging_handle_->GetBasePosition()), 1 / device_scale_factor);

  base += base_offset;
  extent += extent_offset;

  WebContentsImpl* wci = static_cast<WebContentsImpl*>(web_contents());
  wci->SelectRange(gfx::ToFlooredPoint(base), gfx::ToFlooredPoint(extent));
}

void SelectionControllerEfl::HandleDragUpdateNotification(SelectionHandleEfl* handle) {
  // FIXME : Check the text Direction later
  selection_change_reason_ = Reason::HandleDragged;

  switch (handle->Type()) {
    case SelectionHandleEfl::HANDLE_TYPE_INPUT: {
      rwhv_->MoveCaret(handle->GetBasePosition());
      return;
    }
    case SelectionHandleEfl::HANDLE_TYPE_LEFT:
    case SelectionHandleEfl::HANDLE_TYPE_RIGHT: {
      float device_scale_factor =
          display::Screen::GetScreen()->GetPrimaryDisplay().device_scale_factor();
      gfx::PointF extent = gfx::ScalePoint(
          gfx::PointF(handle->GetBasePosition()), 1 / device_scale_factor);

      bool is_anchor_first = selection_data_->GetIsAnchorFirst();
      gfx::Vector2dF line_offset = is_anchor_first
                                   ? ComputeLineOffsetFromBottom(start_selection_)
                                   : ComputeLineOffsetFromBottom(end_selection_);
      extent += line_offset;

      WebContentsImpl* wci = static_cast<WebContentsImpl*>(web_contents());
      wci->MoveRangeSelectionExtent(gfx::ToFlooredPoint(extent));
      return;
    }
  }
}

void SelectionControllerEfl::HandleDragEndNotification() {
  dragged_handle_ = nullptr;

  if (ecore_events_filter_)
    ecore_event_filter_del(ecore_events_filter_);
  ecore_events_filter_ = nullptr;

  selection_change_reason_ = Reason::HandleReleased;
  start_handle_->SetBasePosition(selection_data_->GetLeftRect().bottom_left());
  end_handle_->SetBasePosition(selection_data_->GetRightRect().bottom_right());
  handle_being_dragged_ = false;
  ShowHandleAndContextMenuIfRequired();
}

void SelectionControllerEfl::GetSelectionBounds(gfx::Rect* left,
                                                gfx::Rect* right) {
  if (left)
    *left = selection_data_->GetLeftRect();
  if (right)
    *right = selection_data_->GetRightRect();
}

void SelectionControllerEfl::HandleGesture(blink::WebGestureEvent& event) {
  blink::WebInputEvent::Type event_type = event.GetType();
  if (event_type == blink::WebInputEvent::kGestureTap) {
    HandlePostponedGesture(event.x, event.y, ui::ET_GESTURE_TAP);
  } else if (event_type == blink::WebInputEvent::kGestureShowPress) {
    HandlePostponedGesture(
        event.x, event.y, ui::ET_GESTURE_SHOW_PRESS);
  } else if (event_type == blink::WebInputEvent::kGestureLongPress) {
    selection_change_reason_ = Reason::LongPressStarted;
    HandlePostponedGesture(
        event.x, event.y, ui::ET_GESTURE_LONG_PRESS);
    long_mouse_press_ = true;
  }

  RenderViewHost* render_view_host = web_contents()->GetRenderViewHost();
  if (!render_view_host)
    return;

  if (!render_view_host->GetWebkitPreferences().text_zoom_enabled &&
      (event_type == blink::WebInputEvent::kGesturePinchBegin ||
       event_type == blink::WebInputEvent::kGesturePinchEnd)) {
    return;
  }

  if (event_type == blink::WebInputEvent::kGestureScrollBegin ||
      event_type == blink::WebInputEvent::kGesturePinchBegin) {
    SetControlsTemporarilyHidden(true);
  } else if (event_type == blink::WebInputEvent::kGestureScrollEnd ||
             event_type == blink::WebInputEvent::kGesturePinchEnd) {
    SetControlsTemporarilyHidden(false);
  }
}

void SelectionControllerEfl::HandlePostponedGesture(int x,
                                                    int y,
                                                    ui::EventType type) {
  DVLOG(0) << "HandlePostponedGesture :: " << type;

  gfx::Point point = gfx::Point(x, y);
  point = rwhv_->ConvertPointInViewPix(point);

  switch (type) {
    case ui::ET_GESTURE_LONG_PRESS: {
      ClearSelectionViaEWebView();
      HideHandleAndContextMenu();
      // Long press data will call to WebContentsViewDelegateEwk.
      // It is called by the chain that handles FrameHostMsg_ContextMenu.
      break;
    }
    case ui::ET_GESTURE_TAP:
    case ui::ET_GESTURE_SHOW_PRESS: {
      // Do not do anything.
      break;
    }
    default:
      ClearSelectionViaEWebView();
      break;
  }
}

bool SelectionControllerEfl::HandleLongPressEvent(
    const gfx::Point& touch_point,
    const content::ContextMenuParams& params) {

  if (params.is_editable) {
    // If one long press on an empty form, do not enter selection mode.
    // Instead, context menu will be shown if needed for clipboard actions.
    if (params.selection_text.empty() &&
        !(rwhv_ && rwhv_->HasSelectableText()) &&
        !ClipboardHelperEfl::GetInstance()->CanPasteFromSystemClipboard()) {
      long_mouse_press_ = false;
      selection_change_reason_ = Reason::Irrelevant;
      return false;
    }

    SetSelectionStatus(true);
    SetSelectionEditable(true);
    if (selection_mode_ == SelectionMode::NONE)
      SetSelectionMode(SelectionMode::CARET);
    HandleLongPressEventPrivate(touch_point);
    return true;
  } else if (params.link_url.is_empty() && params.src_url.is_empty() &&
             (params.is_text_node || !params.selection_text.empty())) {
    // If user is long pressing on a content with
    // -webkit-user-select: none, we should bail and not enter
    // selection neither show magnigier class or context menu.
    if (params.selection_text.empty()) {
      long_mouse_press_ = false;
      return false;
    }
    SetSelectionStatus(true);
    SetSelectionEditable(false);
    HandleLongPressEventPrivate(touch_point);
    DVLOG(1) << __PRETTY_FUNCTION__ << ":: !link, !image, !media, text";
    return true;
  } else if (!params.src_url.is_empty() && params.has_image_contents) {
    DVLOG(1) << __PRETTY_FUNCTION__ << ":: IMAGE";
    long_mouse_press_ = false;
    return false;
  } else if (!params.link_url.is_empty()) {
    DVLOG(1) << __PRETTY_FUNCTION__ << ":: LINK";
    long_mouse_press_ = false;
    return false;
  }
  // Default return is true, because if 'params' did not
  // fall through any of the switch cases above, no further
  // action should be taken by the callee.
  return true;
}

void SelectionControllerEfl::HandleLongPressEventPrivate(
    const gfx::Point& touch_point) {
  Clear();

  if (rwhv_) {
    gfx::Rect view_bounds = rwhv_->GetViewBoundsInPix();
    magnifier_->HandleLongPress(gfx::Point(touch_point.x() + view_bounds.x(),
                                           touch_point.y() + view_bounds.y()),
                                selection_data_->IsInEditField());
  }
}

void SelectionControllerEfl::HandleLongPressMoveEvent(
    const gfx::Point& touch_point) {
  Clear();
  selection_change_reason_ = Reason::LongPressMoved;
  if (rwhv_)
    rwhv_->SelectClosestWord(touch_point);
}

void SelectionControllerEfl::HandleLongPressEndEvent() {
  long_mouse_press_ = false;
  selection_change_reason_ = Reason::LongPressEnded;
  ShowHandleAndContextMenuIfRequired();
}

void SelectionControllerEfl::PostHandleTapGesture(bool is_content_editable) {
  SetSelectionEditable(is_content_editable);
  if (is_content_editable) {
    DVLOG(1) << "PostHandleTapGesture :: Editable";
    SetSelectionStatus(true);
    selection_change_reason_ = Reason::Tap;
  }
}

bool SelectionControllerEfl::IsSelectionValid(const gfx::Rect& left_rect,
                                              const gfx::Rect& right_rect) {
  TRACE_EVENT2("selection,efl", __PRETTY_FUNCTION__,
               "left_rect", left_rect.ToString(),
               "right_rect", right_rect.ToString());
  // For all normal cases the widht will be 0 and we want to check empty which Implies
  // x, y, h w all to be 0
  if ((IsRectEmpty(left_rect) || IsRectEmpty(right_rect))) {
    SetSelectionStatus(false);
    return false;
  }

  // The most of sites return width of each rects as 0 when text is selected.
  // However, some websites that have viewport attributes on meta tag v
  // return width 0 right after they are loaded, even though text is not selected.
  // Thus the width is not sufficient for checking selection condition.
  // Further invesitigation showed left_rect and right_rect always have the same x,y values
  // for such cases. So, the equality for x and y rather than width should be tested.
  if (left_rect.x() == right_rect.x() && left_rect.y() == right_rect.y() &&
      !selection_data_->IsInEditField()  && !handle_being_dragged_ &&
      selection_change_reason_ == Reason::Irrelevant) {
    SetSelectionStatus(false);
    return false;
  }

  return true;
}

void SelectionControllerEfl::ClearSelection() {
  TRACE_EVENT0("selection,efl", __PRETTY_FUNCTION__);
  Clear();
  CancelContextMenu(0);
  SetSelectionStatus(false);
  SetSelectionEditable(false);
  SetSelectionMode(SelectionMode::NONE);
  selection_change_reason_ = Reason::Irrelevant;
}

void SelectionControllerEfl::OnParentParentViewMove() {
  TRACE_EVENT0("selection,efl", __PRETTY_FUNCTION__);
  // Update positions of large handle and selection range handles
  input_handle_->Move(selection_data_->GetLeftRect().bottom_left());
  start_handle_->Move(start_handle_->GetBasePosition());
  end_handle_->Move(end_handle_->GetBasePosition());
}

gfx::Rect SelectionControllerEfl::GetLeftRect() const {
  return selection_data_->GetLeftRect();
}

gfx::Rect SelectionControllerEfl::GetRightRect() const {
  return selection_data_->GetRightRect();
}

gfx::Rect SelectionControllerEfl::GetForbiddenRegionRect(
    const gfx::Rect& selection_rect) const {
  gfx::Rect forbidden_region;
  auto visible_viewport_rect = GetVisibleViewportRect();
  auto left_rect = GetLeftRect();
  auto right_rect = GetRightRect();
  left_rect.Offset(visible_viewport_rect.x(), visible_viewport_rect.y());
  right_rect.Offset(visible_viewport_rect.x(), visible_viewport_rect.y());

  if (IsCaretMode()) {
    forbidden_region.set_x(left_rect.x());
    forbidden_region.set_y(input_handle_->IsTop()
                               ? left_rect.y() - kLargeHandleHeight
                               : left_rect.y());
    forbidden_region.set_height(left_rect.height() + kLargeHandleHeight);
  } else {
    forbidden_region = selection_rect;
    int start_handle_y, start_handle_bottom, end_handle_y, end_handle_bottom;
    if (start_handle_->IsTop()) {
      start_handle_y = left_rect.y() - kRangeHandleHeight;
      start_handle_bottom = left_rect.y();
    } else {
      start_handle_y = left_rect.bottom();
      start_handle_bottom = left_rect.bottom() + kRangeHandleHeight;
    }

    if (end_handle_->IsTop()) {
      end_handle_y = right_rect.y() - kRangeHandleHeight;
      end_handle_bottom = right_rect.y();
    } else {
      end_handle_y = right_rect.bottom();
      end_handle_bottom = right_rect.bottom() + kRangeHandleHeight;
    }

    int forbidden_region_y =
        std::min(forbidden_region.y(), std::min(start_handle_y, end_handle_y));
    int forbidden_region_bottom = std::max(
        forbidden_region.y(), std::max(start_handle_bottom, end_handle_bottom));

    forbidden_region.set_y(forbidden_region_y);
    forbidden_region.set_height(forbidden_region_bottom - forbidden_region_y);
  }

  // Inflate y by the menu padding
  forbidden_region.set_y(forbidden_region.y() - kMenuPadding);
  forbidden_region.set_height(forbidden_region.height() + 2 * kMenuPadding);

  return IntersectRects(forbidden_region, visible_viewport_rect,
                        IsCaretMode() /*ignore_width*/);
}

gfx::Rect SelectionControllerEfl::GetVisibleViewportRect() const {
  auto visible_viewport_rect = rwhv_->GetViewBoundsInPix();
  auto custom_email_viewport_rect = rwhv_->GetCustomEmailViewportRect();
  if (custom_email_viewport_rect.IsEmpty())
    return visible_viewport_rect;

  return gfx::IntersectRects(visible_viewport_rect, custom_email_viewport_rect);
}

bool SelectionControllerEfl::TextSelectionDown(int x, int y) {
  /*
   * According to webkit-efl textSelectionDown is used on long press gesture, we already
   * have implementation for handling this gesture in SelectionControllerEfl so we just
   * fallback into it. Although I'm not totally sure that this is expected behaviour as there
   * is no clear explanation what should this API do.
   *
   * Reference from webkit-efl:
   * Source/WebKit2/UIProcess/API/efl/ewk_view.cpp line 614
   */
  if (!long_mouse_press_) {
    long_mouse_press_ = true;
    HandleLongPressEventPrivate(gfx::Point(x, y));
    return true;
  }

  return false;
}

bool SelectionControllerEfl::TextSelectionUp(int /*x*/, int /*y*/) {
  /*
   * According to webkit-efl textSelectionUp is used when MouseUp event occurs. We already
   * have implementation for handling MouseUp after long press in SelectionMagnifierEfl so we just
   * fallback into it. Although I'm not totally sure that this is expected behaviour as there
   * is no clear explanation what should this API do.
   *
   * Reference from webkit-efl:
   * Source/WebKit2/UIProcess/API/efl/tizen/TextSelection.cpp line 807
   */

  if (long_mouse_press_) {
    magnifier_->OnAnimatorUp();
    return true;
  }

  return false;
}

bool SelectionControllerEfl::ExistsSelectedText() {
  // Is in edit field and text is selected.
  if (selection_data_->IsInEditField() &&
      (selection_data_->GetLeftRect() != selection_data_->GetRightRect()))
    return true;
  return false;
}

void SelectionControllerEfl::SetVisiblityStatus(bool visiblity) {
  if (!visiblity && !handle_being_dragged_ && !is_caret_mode_forced_)
    selection_change_reason_ = Reason::Irrelevant;
  else if (selection_change_reason_ == Reason::Irrelevant)
    selection_change_reason_ = Reason::RequestedByContextMenu;
}

void SelectionControllerEfl::ContextMenuStatusHidden() {
  if (IsMobileProfile())
    context_menu_status_ = ContextMenuStatus::HIDDEN;
  SetVisiblityStatus(false);
}

void SelectionControllerEfl::ContextMenuStatusVisible() {
  if (IsMobileProfile())
    context_menu_status_ = ContextMenuStatus::VISIBLE;
  SetVisiblityStatus(true);
}
}
