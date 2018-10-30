// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef RENDER_WIDGET_HOST_VIEW_EFL
#define RENDER_WIDGET_HOST_VIEW_EFL

#include <Ecore_Evas.h>
#include <Ecore_IMF_Evas.h>
#include <Evas.h>
#include <Evas_GL.h>

#include "base/callback.h"
#include "base/containers/id_map.h"
#include "base/format_macros.h"
#include "chromium_impl/build/tizen_version.h"
#include "components/viz/common/frame_sinks/copy_output_request.h"
#include "components/viz/common/frame_sinks/copy_output_result.h"
#include "components/viz/common/quads/compositor_frame.h"
#include "components/viz/common/resources/single_release_callback.h"
#include "content/browser/accessibility/browser_accessibility_manager.h"
#include "content/browser/compositor/evasgl_delegated_frame_host.h"
#include "content/browser/renderer_host/evas_event_handler.h"
#include "content/browser/renderer_host/event_resampler.h"
#include "content/browser/renderer_host/render_widget_host_view_base.h"
#include "content/browser/selection/selection_controller_efl.h"
#include "content/browser/web_contents/web_contents_impl.h"
#include "content/browser/web_contents/web_contents_view_efl.h"
#include "content/common/content_export.h"
#include "content/public/common/browser_controls_state.h"
#include "ipc/ipc_sender.h"
#include "ui/base/ime/composition_text.h"
#include "ui/base/ime/text_input_client.h"
#include "ui/events/gestures/gesture_recognizer.h"
#include "ui/events/gestures/gesture_types.h"
#include "ui/events/gestures/motion_event_aura.h"

// Before, it was separated by EVAS_GL_API_VERSION and each value was;
// TV/DESKTOP = 1, MOBILE/WEARABLE = 4

// After Desktop build EFL upversion to 1.18,
// EVAS_GL_API_VERSION of DESKTOP is 5 and
// it would be more legible to seperate by profiles.
#if defined(OS_TIZEN)
typedef EvasGLint64 GLint64;
typedef EvasGLuint64 GLuint64;
#else
typedef struct __GLsync* GLsync;
typedef signed long GLint64;
typedef unsigned long GLuint64;
#endif

struct TextInputState;

namespace ui {
class GestureEvent;
class TouchEvent;
struct DidOverscrollParams;
}  // namespace ui

namespace blink {
struct WebScreenInfo;
}

namespace content {

typedef void (*Screenshot_Captured_Callback)(Evas_Object* image,
                                             void* user_data);
typedef base::Closure SnapshotTask;

class DisambiguationPopupEfl;
class EdgeEffect;
class IMContextEfl;
class RenderWidgetHostImpl;
class RenderWidgetHostView;
class WebContents;
class ScreenshotCapturedCallback;

#if defined(TIZEN_ATK_SUPPORT)
class BrowserAccessibilityDelegate;
class BrowserAccessibilityManager;
#endif
class MirroredBlurEffect;

// RenderWidgetHostView class hierarchy described in render_widget_host_view.h.
class CONTENT_EXPORT RenderWidgetHostViewEfl
    : public RenderWidgetHostViewBase,
      public EvasGLDelegatedFrameHostClient,
      public EventResamplerClient,
      public ui::GestureConsumer,
      public ui::GestureEventHelper,
      public base::SupportsWeakPtr<RenderWidgetHostViewEfl>,
      public IPC::Sender {
 public:
  explicit RenderWidgetHostViewEfl(RenderWidgetHost*,
                                   WebContents& web_contents);

  // RenderWidgetHostViewBase implementation.
  void InitAsChild(gfx::NativeView) override;
  void InitAsPopup(RenderWidgetHostView*, const gfx::Rect&) override;
  void InitAsFullscreen(RenderWidgetHostView*) override;
  RenderWidgetHost* GetRenderWidgetHost() const override;
  void SetSize(const gfx::Size&) override;
  void SetBounds(const gfx::Rect&) override;
  gfx::Vector2dF GetLastScrollOffset() const override;
  gfx::NativeView GetNativeView() const override;
  gfx::NativeViewAccessible GetNativeViewAccessible() override;
  bool IsSurfaceAvailableForCopy() const override;
  void Show() override;
  void Hide() override;
  bool IsShowing() override;
  gfx::Rect GetViewBounds() const override;
  // Returns view size unless |custom_viewport_size_| is set.
  gfx::Size GetVisibleViewportSize() const override;
  gfx::Size GetPhysicalBackingSize() const override;
  bool LockMouse() override;
  void UnlockMouse() override;
  void Focus() override;
  void Unfocus();
  bool HasFocus() const override;
  void UpdateCursor(const WebCursor&) override;
  void SetIsLoading(bool) override;
  void SetBackgroundColor(SkColor color) override;
  SkColor background_color() const override;
  void UpdateBackgroundColorFromRenderer(SkColor color);
  bool DoBrowserControlsShrinkBlinkSize() const override;
  float GetTopControlsHeight() const override;
  float GetBottomControlsHeight() const override;

  void TextInputStateChanged(const TextInputState& params) override;

  void UpdateIMELayoutVariation(bool is_minimum_negative, bool is_step_integer);

  void ImeCancelComposition() override;
  void ImeCompositionRangeChanged(const gfx::Range&,
                                  const std::vector<gfx::Rect>&) override;

  void FocusedNodeChanged(bool is_editable_node,
                          const gfx::Rect& node_bounds_in_screen
#if defined(OS_TIZEN_TV_PRODUCT)
                          ,
                          bool is_radio_or_checkbox,
                          int password_input_minlength
#endif
#if defined(USE_WAYLAND)
                          ,
                          bool is_content_editable
#endif
                          ) override;

  void Destroy() override;
  void SetTooltipText(const base::string16&) override;
  void SelectionChanged(const base::string16&,
                        size_t,
                        const gfx::Range&) override;
  void SelectionBoundsChanged(
      const ViewHostMsg_SelectionBounds_Params&) override;
  void SetNeedsBeginFrames(bool needs_begin_frames) override;
  void DidOverscroll(const ui::DidOverscrollParams& params) override;
  bool HasAcceleratedSurface(const gfx::Size&) override;
  gfx::Rect GetBoundsInRootWindow() override;
  void RenderProcessGone(base::TerminationStatus, int) override;
  bool OnMessageReceived(const IPC::Message&) override;

  void OnFilteredMessageReceived(const IPC::Message&);

  void ProcessAckedTouchEvent(const TouchEventWithLatencyInfo&,
                              InputEventAckState) override;
  void DidStopFlinging() override;

  void ShowDisambiguationPopup(const gfx::Rect& rect_pixels,
                               const SkBitmap& zoomed_bitmap) override;
  void DisambiguationPopupDismissed();
  void HandleDisambiguationPopupMouseDownEvent(Evas_Event_Mouse_Down*);
  void HandleDisambiguationPopupMouseUpEvent(Evas_Event_Mouse_Up*);

  void DidCreateNewRendererCompositorFrameSink(
      viz::mojom::CompositorFrameSinkClient*
          renderer_compositor_frame_sink) override;
  void SubmitCompositorFrame(const viz::LocalSurfaceId& local_surface_id,
                             viz::CompositorFrame frame) override;
  void ClearCompositorFrame() override;

  // ui::GestureEventHelper implementation.
  bool CanDispatchToConsumer(ui::GestureConsumer* consumer) override;
  void DispatchSyntheticTouchEvent(ui::TouchEvent* event) override;
  void DispatchGestureEvent(GestureConsumer* raw_input_consumer,
                            ui::GestureEvent*) override;

#if defined(TIZEN_ATK_SUPPORT)
  BrowserAccessibilityManager* CreateBrowserAccessibilityManager(
      BrowserAccessibilityDelegate*,
      bool for_root_frame) override;
#endif

  // IPC::Sender implementation:
  bool Send(IPC::Message*) override;

  void FilterInputMotion(const blink::WebGestureEvent& gesture_event);

  void DidMoveWebView();

  void EvasToBlinkCords(int x, int y, int* view_x, int* view_y);
  void SelectClosestWord(const gfx::Point& touch_point);
  void SetTileCoverAreaMultiplier(float cover_area_multiplier);

  Evas* evas() const {
    DCHECK(evas_);
    return evas_;
  }
  Evas_Object* ewk_view() const;
  Evas_Object* smart_parent() const { return smart_parent_; }
  Evas_Object* content_image() const { return content_image_; }
  WebContents* web_contents() const { return &web_contents_; }

  void set_magnifier(bool status);

  void Init_EvasGL(int width, int height);
  void CreateNativeSurface(int width, int height);

  void SetEvasHandler(scoped_refptr<EvasEventHandler> evas_event_handler);

  void HandleGestureBegin();
  void HandleGestureEnd();
  void HandleGesture(ui::GestureEvent*);
  void HandleGesture(blink::WebGestureEvent&);
  void HandleTouchEvent(ui::TouchEvent*);

  bool GetHorizontalPanningHold() const { return horizontal_panning_hold_; }
  void SetHorizontalPanningHold(bool hold) { horizontal_panning_hold_ = hold; }
  bool GetVerticalPanningHold() const { return vertical_panning_hold_; }
  void SetVerticalPanningHold(bool hold) { vertical_panning_hold_ = hold; }

  size_t visible_top_controls_height() const {
    return visible_top_controls_height_;
  }

  Evas_GL_API* evasGlApi() { return evas_gl_api_; }
  gfx::Point ConvertPointInViewPix(gfx::Point point);
  gfx::Rect GetViewBoundsInPix() const;
  void SetCustomViewportSize(const gfx::Size& size);

  const gfx::Size GetScrollableSize() const;
  void SetScaledContentSize(const gfx::Size& size) {
    scaled_contents_size_ = size;
  }

  void MoveCaret(const gfx::Point& point);
  void SetComposition(const ui::CompositionText& composition_text);
  void ConfirmComposition(base::string16& text);
  void SendGestureEvent(blink::WebGestureEvent& event);

  void SetTouchEventsEnabled(bool enabled);

  RenderFrameHostImpl* GetFocusedFrame();
  void ScrollFocusedEditableNode();

  bool HasSelectableText();

  void SetClearTilesOnHide(bool enable);

  // |snapshot_area| is relative coordinate system based on Webview.
  // (0,0) is top left corner.
  Evas_Object* GetSnapshot(const gfx::Rect& snapshot_area,
                           float scale_factor,
                           bool is_magnifier = false);
  bool RequestSnapshotAsync(const gfx::Rect& snapshot_area,
                            Screenshot_Captured_Callback callback,
                            void* user_data,
                            float scale_factor = 1.0);
  void RequestMagnifierSnapshotAsync(const Eina_Rectangle rect,
                                     Screenshot_Captured_Callback callback,
                                     void* user_data,
                                     float scale_factor = 1.0f);
  void SetGetCustomEmailViewportRectCallback(
      const base::Callback<const gfx::Rect&(void)>& callback);
  gfx::Rect GetCustomEmailViewportRect() const;
  void SetPageVisibility(bool visible);
  void OnSnapshotDataReceived(SkBitmap bitmap, int snapshotId);
  void OnEditableContentChanged();
#if defined(OS_TIZEN)
  void OnEdgeEffectONSCROLLTizenUIF(bool, bool, bool, bool);
#endif
  bool ClearCurrent();

  void SetTopControlsHeight(size_t top_height, size_t bottom_height);
  bool SetTopControlsState(BrowserControlsState constraint,
                           BrowserControlsState current,
                           bool animate);
  bool TopControlsAnimationScheduled();
  void ResizeIfNeededByTopControls();
  void TopControlsOffsetChanged();

  int GetVisibleBottomControlsHeightInPix() const;

  SelectionControllerEfl* GetSelectionController() const {
    return selection_controller_.get();
  }

  unsigned PointerStatePointerCount() const {
    return pointer_state_.GetPointerCount();
  }

  // Sets rotation degrees. Expected values are one of { 0, 90, 180, 270 }.
  void UpdateRotationDegrees(int rotation_degrees);

  // EvasGLDelegatedFrameHostClient implementation.
  Evas_GL_API* GetEvasGLAPI() override;
  Evas_GL* GetEvasGL() override;
  void DelegatedFrameHostSendReclaimCompositorResources(
      const viz::LocalSurfaceId& local_surface_id,
      const std::vector<viz::ReturnedResource>& resources) override;
  bool MakeCurrent() override;
  void ClearBrowserFrame(SkColor) override;
#if defined(TIZEN_TBM_SUPPORT)
  bool OffscreenRenderingEnabled() override;
#endif

#if defined(TIZEN_VIDEO_HOLE)
  void SetWebViewMovedCallback(const base::Closure& on_webview_moved);
#endif

  void SetFocusInOutCallbacks(const base::Callback<void(void)>& on_focus_in,
                              const base::Callback<void(void)>& on_focus_out);

#if defined(OS_TIZEN_TV_PRODUCT)
  void DidFinishNavigation();
  void SetLayerInverted(bool inverted);
  void OnDidInitializeRenderer();
  void ClearAllTilesResources();
  void DrawLabel(Evas_Object* image, Eina_Rectangle rect);
  void ClearLabels();

  void SendKeyEvent(Evas_Object* ewk_view, void* event_info, bool is_press);
  void SetKeyEventsEnabled(bool enabled);
  void SetMouseEventCallbacks(
      const base::Callback<void(int, int)>& on_mouse_down,
      const base::Callback<bool(void)>& on_mouse_up,
      const base::Callback<void(void)>& on_mouse_move);
  void SetCursorByClient(bool enable) { cursor_set_by_client_ = enable; }
#endif
  template <typename EVT>
  void ConvertEnterToSpaceIfNeeded(EVT* evt);

  bool IsFocusedNodeEditable() const { return is_focused_node_editable_; }
#if defined(USE_WAYLAND)
  bool IsFocusedNodeContentEditable() const { return is_content_editable_; }
#endif
  void UpdateSpatialNavigationStatus(bool enable);
  void UpdateHwKeyboardStatus(bool status) {
    is_hw_keyboard_connected_ = status;
  }

  void SetIMERecommendedWords(const std::string& recommended_words);
  void SetIMERecommendedWordsType(bool should_enable);

  IMContextEfl* GetIMContext() const { return im_context_; }
  Evas_Object* content_image_elm_host() const {
    return content_image_elm_host_;
  }

  void RequestSelectionRect() const;
  void OnSelectionRectReceived(const gfx::Rect& selection_rect) const;
  bool IsTouchstartConsumed() const { return touchstart_consumed_; }
  bool IsTouchendConsumed() const { return touchend_consumed_; }

  void InitBlurEffect(bool state);
  bool NeedBlurEffect() { return need_effect_; }

 protected:
  friend class RenderWidgetHostView;

  // EventResamplerClient implementation.
  void ForwardTouchEvent(ui::TouchEvent*) override;
  void ForwardGestureEvent(blink::WebGestureEvent) override;
  unsigned TouchPointCount() const override;
#if defined(TIZEN_TBM_SUPPORT)
  bool UseEventResampler() override;
#endif

 private:
  ~RenderWidgetHostViewEfl() override;

  void InitializeDeviceDisplayInfo();
  gfx::NativeViewId GetNativeViewId() const;

  void OnDidResize();

  static void OnParentViewResize(void* data, Evas*, Evas_Object*, void*);
  static void OnMove(void* data, Evas*, Evas_Object*, void*);
  static void OnFocusIn(void* data, Evas*, Evas_Object*, void*);
  static void OnFocusOut(void* data, Evas*, Evas_Object*, void*);
  static void OnHostFocusIn(void* data, Evas_Object*, void*);
  static void OnHostFocusOut(void* data, Evas_Object*, void*);

  static void OnMotionEnable(void* data, Evas_Object*, void*);
  static void OnMotionMove(void* data, Evas_Object*, void*);
  static void OnMotionZoom(void* data, Evas_Object*, void*);

  static void OnMultiTouchDownEvent(void* data, Evas*, Evas_Object*, void*);
  static void OnMultiTouchMoveEvent(void* data, Evas*, Evas_Object*, void*);
  static void OnMultiTouchUpEvent(void* data, Evas*, Evas_Object*, void*);

  static void OnMouseDown(void* data, Evas*, Evas_Object*, void*);
  static void OnMouseUp(void* data, Evas*, Evas_Object*, void*);
  static void OnMouseMove(void* data, Evas*, Evas_Object*, void*);
  static void OnMouseOut(void* data, Evas*, Evas_Object*, void*);
  static void OnMouseWheel(void* data, Evas*, Evas_Object*, void*);
  static void OnKeyDown(void*, Evas*, Evas_Object*, void*);
  static void OnKeyUp(void*, Evas*, Evas_Object*, void*);

  void ProcessTouchEvents(unsigned int timestamp, bool is_multi_touch = false);
  void SetDoubleTapSupportEnabled(bool enabled);

  void OnOrientationChangeEvent(int);
  void OnDidHandleKeyEvent(const blink::WebInputEvent* input_event,
                           bool processed);
  bool IsIMEShow() const;

  void Invalidate(bool immediate);
  // TODO(prashant.n): Implement delegate to handle multiple frame_hosts.
  void SwapBrowserFrame(const viz::LocalSurfaceId& local_surface_id,
                        viz::CompositorFrame frame);
  void RenderBrowserFrame();
  void RunGetSnapshotOnMainThread(const gfx::Rect snapshot_area,
                                  int request_id,
                                  float scale_factor);
  static void EvasObjectImagePixelsGetCallback(void*, Evas_Object*);
  void ProcessSnapshotRequest(bool no_swap);
  void GetMagnifierSnapshot(gfx::Rect snapshot_area,
                            float scale_factor,
                            std::unique_ptr<ScreenshotCapturedCallback> cb);

  EdgeEffect& EnsureEdgeEffect();
  void UpdateEdgeEffect();

  static void EvasObjectMirrorEffectImagePixelsGetCallback(void*, Evas_Object*);
  void RenderBrowserFrameOnMirror();

#if defined(USE_WAYLAND)
#if TIZEN_VERSION_AT_LEAST(5, 0, 0)
  Ecore_Wl2_Window* GetEcoreWlWindow() const;
#else
  Ecore_Wl_Window* GetEcoreWlWindow() const;
#endif  // TIZEN_VERSION_AT_LEAST(5, 0, 0)
#else
  Ecore_X_Window GetEcoreXWindow() const;
#endif

  base::Callback<void(void)> on_focus_in_callback_;
  base::Callback<void(void)> on_focus_out_callback_;
  base::Callback<const gfx::Rect&(void)>
      get_custom_email_viewport_rect_callback_;

#if defined(TIZEN_VIDEO_HOLE)
  base::Closure on_webview_moved_callback_;
#endif
  RenderWidgetHostImpl* host_;
  std::unique_ptr<EvasGLDelegatedFrameHost> evasgl_delegated_frame_host_;
  IMContextEfl* im_context_;
  Evas* evas_;
  Evas_Object* main_layout_;
  Evas_Object* smart_parent_;
  Evas_Object* content_image_;
  Evas_Object* content_image_elm_host_;
  bool evas_gl_initialized_;
  float device_scale_factor_;
#if defined(OS_TIZEN_TV_PRODUCT)
  std::vector<Evas_Object*> voice_manager_labels_;
  base::Callback<void(int, int)> on_mouse_down_callback_;
  base::Callback<bool(void)> on_mouse_up_callback_;
  base::Callback<void(void)> on_mouse_move_callback_;
#endif
  bool magnifier_;

  // Whether we are currently loading.
  bool is_loading_;

  // If set, it's usually smaller than the view size not to allow top Elm widgets
  // (select picker) to overlap web content.
  gfx::Size custom_viewport_size_;

  scoped_refptr<EvasEventHandler> evas_event_handler_;

  // Stores the current state of the active pointers targeting this
  // object.
  ui::MotionEventAura pointer_state_;

  // The gesture recognizer for this view.
  // In Aura GestureRecognizer is global. Should we follow that?
  std::unique_ptr<ui::GestureRecognizer> gesture_recognizer_;

  std::unique_ptr<DisambiguationPopupEfl> disambiguation_popup_;
  std::unique_ptr<EdgeEffect> edge_effect_;

  gfx::Size scaled_contents_size_;

  int current_orientation_;

  Evas_GL* evas_gl_;
  Evas_GL_API* evas_gl_api_;
  Evas_GL_Config* evas_gl_config_;
  Evas_GL_Context* evas_gl_context_;
  Evas_GL_Surface* evas_gl_surface_;

  bool handling_disambiguation_popup_gesture_;
  bool touch_events_enabled_;
  // The background color of the web content.
  SkColor background_color_;

#if defined(OS_TIZEN_TV_PRODUCT)
  // When WebBrowser sets their own cursor, set the flag
  // not to set the WebPage cursor
  bool cursor_set_by_client_;

  bool layer_inverted_;
  bool renderer_initialized_;
  int password_input_minlength_;
#endif

  bool radio_or_checkbox_focused_;
  int rotation_;
  int frame_data_output_rect_width_;
  bool was_keydown_filtered_by_platform_;
  bool was_return_keydown_filtered_by_platform_;

  // The last scroll offset of the view.
  gfx::Vector2dF last_scroll_offset_;

  WebContents& web_contents_;

  bool horizontal_panning_hold_;
  bool vertical_panning_hold_;
  bool is_scrolling_needed_;
  bool frame_rendered_;
  bool is_focused_node_editable_;
  bool is_hw_keyboard_connected_;
  int top_controls_height_;
  int visible_top_controls_height_;
  int visible_top_controls_height_in_pix_;
  int top_controls_offset_correction_;

  int bottom_controls_height_;
  int visible_bottom_controls_height_;
  int visible_bottom_controls_height_in_pix_;

  int last_resized_visible_top_controls_height_;
  float top_controls_shown_ratio_;

  int touch_start_top_controls_offset_;
  bool defer_showing_selection_controls_;

  bool touchstart_consumed_;
  bool touchend_consumed_;

  std::list<SnapshotTask> snapshot_task_list_;
  base::IDMap<std::unique_ptr<ScreenshotCapturedCallback>>
      screen_capture_cb_map_;
  // Magnifier snapshot requests are not added to snapshot_task_list_, because
  // we only care about the last one if they piled up - we can only display
  // one anyway.
  SnapshotTask magnifier_snapshot_request_;

  std::unique_ptr<SelectionControllerEfl> selection_controller_;
  std::unique_ptr<EventResampler> event_resampler_;
  viz::mojom::CompositorFrameSinkClient* renderer_compositor_frame_sink_ =
      nullptr;

  std::unique_ptr<base::OneShotTimer> snapshot_timer_;
  std::unique_ptr<base::OneShotTimer> snapshot_timer_noswap_;
  base::Closure process_snapshot_request_callback_;

#if defined(USE_WAYLAND)
  bool is_content_editable_;
#endif

  base::WeakPtrFactory<RenderWidgetHostViewEfl> weak_factory_;
  std::unique_ptr<MirroredBlurEffect> mirror_effect_;
  Evas_Object* mirror_effect_image_;
  bool need_effect_;

  DISALLOW_COPY_AND_ASSIGN(RenderWidgetHostViewEfl);
};

}  // namespace content

#endif
