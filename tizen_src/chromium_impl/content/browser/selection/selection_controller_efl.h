// Copyright 2013-2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef selection_controller_h
#define selection_controller_h

#include <map>

#include "base/strings/string16.h"
#include "content/browser/selection/selection_box_efl.h"
#include "content/browser/selection/selection_handle_efl.h"
#include "content/browser/selection/selection_magnifier_efl.h"
#include "content/common/content_export.h"
#include "third_party/WebKit/public/platform/WebGestureEvent.h"
#include "third_party/WebKit/public/platform/WebInputEvent.h"
#include "ui/events/event_constants.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/range/range.h"
#include "ui/gfx/selection_bound.h"

#if defined(OS_TIZEN)
#include "vconf/vconf.h"
#endif

#include <libintl.h>

namespace content {
class WebContents;
class WebContentsImpl;
class RenderWidgetHostViewEfl;

// Controls the selection after long tap.
// This handles long tap down, long tap move, long tap up.
// On long tap down touch point is sent to engine by SendGestureEvent and magnifier is shown.
// On long tap move touch events are sent to engine by SelectClosestWord and magnifier is shown.
// On long tap up selection handlers are shown and context menu event is sent.
// Hnadlers are shown to extent selection, On handler move touch points are sent to engine
// by SelectRange to extend the selection
class CONTENT_EXPORT SelectionControllerEfl {

 public:
  explicit SelectionControllerEfl(RenderWidgetHostViewEfl* rwhv);
  ~SelectionControllerEfl();

  // Functions that handle long press, long press move and release
  bool HandleLongPressEvent(const gfx::Point& touch_point,
      const content::ContextMenuParams& params);
  void HandleLongPressMoveEvent(const gfx::Point& touch_point);
  void HandleLongPressEndEvent();
  bool HandleBeingDragged() const { return handle_being_dragged_; }

  void PostHandleTapGesture(bool is_content_editable);

  // Set if selection is valid
  void SetSelectionStatus(bool enable);
  bool GetSelectionStatus() const;

  // Set if selection is in edit field
  void SetSelectionEditable(bool enable);
  bool GetSelectionEditable() const;

  void SetSelectionEmpty(bool value) { selection_on_empty_form_control_ = value; }

  bool GetCaretSelectionStatus() const;

  // Set if the selection base (or anchor) comes logically
  // first than its respective extent.
  void SetIsAnchorFirst(bool value);

  // To update the selection string
  void UpdateSelectionData(const base::string16& text);

  void GetSelectionBounds(gfx::Rect* left, gfx::Rect* right);

  // Handles the touch press, move and relase events on selection handles
  void HandleDragBeginNotification(SelectionHandleEfl*);
  void HandleDragUpdateNotification(SelectionHandleEfl*);
  void HandleDragEndNotification();

  // Clears the selection and hides context menu and handles
  void ClearSelection();
  bool ClearSelectionViaEWebView();
  void HideHandles();
  void HideHandleAndContextMenu();
  bool IsAnyHandleVisible() const;

  void SetControlsTemporarilyHidden(bool hidden,
                                    bool from_custom_scroll_callback = false);

  gfx::Rect GetLeftRect() const;
  gfx::Rect GetRightRect() const;

  void OnSelectionChanged(const gfx::SelectionBound&, const gfx::SelectionBound&);
  void OnTextInputStateChanged();

  bool GetLongPressed() { return long_mouse_press_; }

  bool TextSelectionDown(int x, int y);
  bool TextSelectionUp(int x, int y);

  // 'Reason' enum class enumerates all reasons that can cause
  // text selection changes that require changes on the status
  // of selection controls (handles and context menu).
  enum class Reason {
    ScrollOrZoomGestureEnded = 0,
    HandleDragged,
    HandleReleased,
    LongPressStarted,
    LongPressMoved,
    LongPressEnded,
    Tap,
    RequestedByContextMenu,
    CaretModeForced,
    Irrelevant,
  };
  void SetWaitsForRendererSelectionChanges(
      Reason reason) { selection_change_reason_ = reason; }

  // Gesture handlers.
  void HandlePostponedGesture(int x, int y, ui::EventType type);
  void HandleGesture(blink::WebGestureEvent& event);

  gfx::Rect GetVisibleViewportRect() const;

  bool IsCaretModeForced() const { return is_caret_mode_forced_; }
  void TriggerOnSelectionChange();
  void ToggleCaretAfterSelection();
  bool ExistsSelectedText();
  void ShowHandleAndContextMenuIfRequired(
      Reason explicit_reason = Reason::Irrelevant);

  RenderWidgetHostViewEfl* rwhv() const { return rwhv_; }
  content::WebContents* web_contents() const;

  bool IsCaretMode() const;

  void ContextMenuStatusHidden();
  void ContextMenuStatusVisible();

  // The area where selection menu shouldn't be shown if possible
  // (selection area, handles, menu padding).
  gfx::Rect GetForbiddenRegionRect(const gfx::Rect& selection_rect) const;

 private:
  enum class SelectionMode { NONE = 0, CARET, RANGE };

  enum class ContextMenuStatus { NONE = 0, HIDDEN, INPROGRESS, VISIBLE };

  void DetermineSelectionMode(const gfx::Rect& left_rect, const gfx::Rect& right_rect);
  void SetSelectionMode(enum SelectionMode);

  // sets selection_change_reason_ based on visiblity
  void SetVisiblityStatus(bool visiblity);

  // To update the selection bounds
  // returns false if rects are invalid, otherwise true
  bool UpdateSelectionDataAndShow(const gfx::Rect& left_rect,
      const gfx::Rect& right_rect, bool show = true);

  void HandleLongPressEventPrivate(const gfx::Point& touch_point);


  void Clear();
  bool IsSelectionValid(const gfx::Rect& left_rect, const gfx::Rect& right_rect);

  void ShowContextMenu();
  void CancelContextMenu(int request_id);

  static void EvasParentViewMoveCallback(void* data,
                                         Evas* e,
                                         Evas_Object* obj,
                                         void* event_info) {
    reinterpret_cast<SelectionControllerEfl*>(data)->OnParentParentViewMove();
  }

  float device_scale_factor() const;

#if defined(OS_TIZEN)
  static void PlatformLanguageChanged(keynode_t* keynode, void* data);
#endif

  void OnParentParentViewMove();

  static Eina_Bool EcoreEventFilterCallback(void* user_data,
                                            void* loop_data,
                                            int type,
                                            void* event_info);

  // Is required to send back selction points and range extenstion co-ordinates
  RenderWidgetHostViewEfl* rwhv_;

  // Saves state so that selection controls are temporarily hidden due
  // to scrolling or zooming.
  bool controls_temporarily_hidden_;

  // Cache the reason of why browser has requested a text selection
  // change to the renderer. At the next composited selection update
  // the cache is handled and reset.
  Reason selection_change_reason_;

  // Saves state so that handlers and context menu is not shown when seletion
  // change event occurs.
  bool long_mouse_press_;

  // Saves the data that are required to draw handle and context menu
  std::unique_ptr<SelectionBoxEfl> selection_data_;

  // Points to start of the selection for extending selection
  std::unique_ptr<SelectionHandleEfl> start_handle_;

  // Points to the end of the selection for extending selection
  std::unique_ptr<SelectionHandleEfl> end_handle_;

  // Points to the caret in edit box where cursor is present
  std::unique_ptr<SelectionHandleEfl> input_handle_;

  // Points to show the contents magnified
  std::unique_ptr<SelectionMagnifierEfl> magnifier_;

  // Helper pointer to the handle being dragged.
  SelectionHandleEfl* dragging_handle_;

  // Helper pointer to the stationary handle (during dragging).
  SelectionHandleEfl* stationary_handle_;

  bool handle_being_dragged_;

  bool selection_on_empty_form_control_;

  bool is_caret_mode_forced_;
  Ecore_Event_Filter* ecore_events_filter_;

  SelectionHandleEfl* dragged_handle_;

  enum ContextMenuStatus context_menu_status_;
  bool triggered_selection_change_;

  enum SelectionMode selection_mode_;
  gfx::SelectionBound start_selection_;
  gfx::SelectionBound end_selection_;
};

} // namespace content
#endif
