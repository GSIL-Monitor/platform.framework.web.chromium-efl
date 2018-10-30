// Copyright 2014-2015 Samsung Electronics. All rights reserved.
// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/render_widget_host_view_efl.h"

#include <Ecore.h>
#include <Ecore_Evas.h>
#include <Ecore_Input.h>
#include <Elementary.h>
#include "ecore_x_wayland_wrapper.h"

#include "base/auto_reset.h"
#include "base/bind.h"
#include "base/callback_helpers.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/message_loop/message_loop.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/trace_event/trace_event.h"
#include "cc/base/switches.h"
#include "content/browser/renderer_host/disambiguation_popup_efl.h"
#include "content/browser/renderer_host/edge_effect.h"
#include "content/browser/renderer_host/event_with_latency_info.h"
#include "content/browser/renderer_host/im_context_efl.h"
#include "content/browser/renderer_host/mirrored_blur_effect.h"
#include "content/browser/renderer_host/render_view_host_impl.h"
#include "content/browser/renderer_host/render_widget_host_impl.h"
#include "content/browser/renderer_host/ui_events_helper.h"
#include "content/browser/renderer_host/web_event_factory_efl.h"
#include "content/browser/web_contents/web_contents_impl_efl.h"
#include "content/common/cursors/webcursor_efl.h"
#include "content/common/input_messages.h"
#include "content/common/view_messages.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/context_factory.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/web_contents_delegate.h"
#include "content/public/common/content_switches.h"
#include "gpu/command_buffer/service/mailbox_manager.h"
#include "gpu/config/scoped_restore_non_owned_evasgl_context.h"
#include "gpu/ipc/common/gpu_messages.h"
#include "gpu/ipc/in_process_command_buffer.h"
#include "media/base/video_frame.h"
#include "media/base/video_util.h"
#include "skia/ext/image_operations.h"
#include "third_party/WebKit/public/platform/WebInputEvent.h"
#include "third_party/WebKit/public/platform/WebScreenInfo.h"
#include "third_party/WebKit/public/platform/WebTouchPoint.h"
#include "tizen/system_info.h"
#include "ui/base/clipboard/clipboard_helper_efl.h"
#include "ui/base/layout.h"
#include "ui/events/blink/blink_event_util.h"
#include "ui/events/blink/did_overscroll_params.h"
#include "ui/events/event_switches.h"
#include "ui/events/event_utils.h"
#define private public
#include "ui/events/gestures/gesture_provider_aura.h"
#undef private
#include "ui/display/device_display_info_efl.h"
#include "ui/display/display.h"
#include "ui/display/screen.h"
#include "ui/events/gestures/gesture_recognizer_impl_efl.h"
#include "ui/gfx/geometry/dip_util.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/selection_bound.h"
#include "ui/gfx/skbitmap_operations.h"
#include "ui/gl/gl_share_group.h"
#include "ui/gl/gl_shared_context_efl.h"

#if defined(OS_TIZEN_TV_PRODUCT)
#include "common/application_type.h"
#include "content/browser/high_contrast/high_contrast_controller.h"
#include "content/common/cursors/webcursor.h"
#endif

#if defined(TIZEN_ATK_SUPPORT)
#include "content/browser/accessibility/browser_accessibility_manager_efl.h"
#endif

#define EFL_MAX_WIDTH 10000
#define EFL_MAX_HEIGHT 10000  // borrowed from GTK+ port

#define MAX_SURFACE_WIDTH_EGL 4096   // max supported Framebuffer width
#define MAX_SURFACE_HEIGHT_EGL 4096  // max supported Framebuffer height

// These two constants are redefinitions of the original constants defined
// in evas_common.c. These are not supposed to be used directly by apps,
// but we do this because of chromium uses fbo for direct rendering.
#define EVAS_GL_OPTIONS_DIRECT_MEMORY_OPTIMIZE (1 << 12)
#define EVAS_GL_OPTIONS_DIRECT_OVERRIDE (1 << 13)

namespace content {

// If the first frame is not prepared after load is finished,
// delay getting the snapshot by 100ms.
static const int kSnapshotProcessDelay = 100;

namespace {
const char* kLeftKeyName = "Left";
const char* kRightKeyName = "Right";
const char* kUpKeyName = "Up";
const char* kDownKeyName = "Down";

bool IsShiftKey(const char* key) {
  if (!key)
    return false;
  return !strcmp(key, "Shift_L") || !strcmp(key, "Shift_R");
}

bool IsArrowKey(const char* key) {
  if (!key)
    return false;

  return !strcmp(key, kLeftKeyName) || !strcmp(key, kRightKeyName) ||
         !strcmp(key, kUpKeyName) || !strcmp(key, kDownKeyName);
}

// Creates a WebGestureEvent from a ui::GestureEvent. Note that it does not
// populate the event coordinates (i.e. |x|, |y|, |globalX|, and |globalY|). So
// the caller must populate these fields.
blink::WebGestureEvent MakeWebGestureEventFromUIEvent(
    const ui::GestureEvent& event) {
  return ui::CreateWebGestureEvent(
      event.details(), event.time_stamp(), event.location_f(),
      event.root_location_f(), event.flags(), event.unique_touch_event_id());
}

}  // namespace

class ScreenshotCapturedCallback {
 public:
  ScreenshotCapturedCallback(Screenshot_Captured_Callback func, void* user_data)
      : func_(func), user_data_(user_data) {}
  void Run(Evas_Object* image) {
    if (func_ != NULL)
      (func_)(image, user_data_);
  }

 private:
  Screenshot_Captured_Callback func_;
  void* user_data_;
};

RenderWidgetHostViewEfl::RenderWidgetHostViewEfl(RenderWidgetHost* widget,
                                                 WebContents& web_contents)
    : host_(RenderWidgetHostImpl::From(widget)),
#if !defined(TIZEN_VD_DISABLE_GPU)
      evasgl_delegated_frame_host_(new EvasGLDelegatedFrameHost(this)),
#endif
      im_context_(NULL),
      evas_(NULL),
      smart_parent_(NULL),
      content_image_(NULL),
      content_image_elm_host_(NULL),
      evas_gl_initialized_(false),
      device_scale_factor_(1.0f),
      magnifier_(false),
      is_loading_(false),
      gesture_recognizer_(ui::GestureRecognizer::Create()),
      current_orientation_(0),
      evas_gl_(NULL),
      evas_gl_api_(NULL),
      evas_gl_config_(NULL),
      evas_gl_context_(NULL),
      evas_gl_surface_(NULL),
      handling_disambiguation_popup_gesture_(false),
      touch_events_enabled_(false),
      background_color_(SK_ColorWHITE),
#if defined(OS_TIZEN_TV_PRODUCT)
      cursor_set_by_client_(false),
      layer_inverted_(false),
      renderer_initialized_(false),
      password_input_minlength_(-1),
#endif
      radio_or_checkbox_focused_(false),
      rotation_(0),
      frame_data_output_rect_width_(0),
      was_keydown_filtered_by_platform_(false),
      was_return_keydown_filtered_by_platform_(false),
      web_contents_(web_contents),
      horizontal_panning_hold_(false),
      vertical_panning_hold_(false),
      is_scrolling_needed_(false),
      frame_rendered_(false),
      is_focused_node_editable_(false),
      is_hw_keyboard_connected_(false),
      top_controls_height_(0),
      visible_top_controls_height_(0),
      visible_top_controls_height_in_pix_(0),
      top_controls_offset_correction_(0),
      bottom_controls_height_(0),
      visible_bottom_controls_height_(0),
      visible_bottom_controls_height_in_pix_(0),
      last_resized_visible_top_controls_height_(0),
      top_controls_shown_ratio_(0.f),
      touch_start_top_controls_offset_(0),
      defer_showing_selection_controls_(false),
      touchstart_consumed_(false),
      touchend_consumed_(false),
      event_resampler_(new EventResampler(this)),
#if defined(USE_WAYLAND)
      is_content_editable_(false),
#endif
      weak_factory_(this),
      mirror_effect_image_(NULL),
      need_effect_(false) {
  main_layout_ = static_cast<Evas_Object*>(web_contents.GetNativeView());
  evas_ = evas_object_evas_get(main_layout_);
  InitializeDeviceDisplayInfo();

  SetDoubleTapSupportEnabled(touch_events_enabled_);

  device_scale_factor_ =
      display::Screen::GetScreen()->GetPrimaryDisplay().device_scale_factor();

  host_->SetView(this);

  static bool scale_factor_initialized = false;
  if (!scale_factor_initialized) {
    std::vector<ui::ScaleFactor> supported_scale_factors;
    supported_scale_factors.push_back(ui::SCALE_FACTOR_100P);
    supported_scale_factors.push_back(ui::SCALE_FACTOR_200P);
    ui::SetSupportedScaleFactors(supported_scale_factors);
    scale_factor_initialized = true;
  }

  gesture_recognizer_->AddGestureEventHelper(this);

  snapshot_timer_.reset(new base::OneShotTimer);
  snapshot_timer_noswap_.reset(new base::OneShotTimer);
  process_snapshot_request_callback_ = base::Bind(
      &RenderWidgetHostViewEfl::ProcessSnapshotRequest, base::Unretained(this), true);
#if defined(DIRECT_CANVAS)
  gpu::InProcessGpuService::SetMainThread(base::ThreadTaskRunnerHandle::Get());
#endif
}

Evas_Object* RenderWidgetHostViewEfl::ewk_view() const {
  auto wci_efl = static_cast<WebContentsImplEfl*>(&web_contents_);
  if (!wci_efl)
    return nullptr;
  return static_cast<Evas_Object*>(wci_efl->ewk_view());
}

void RenderWidgetHostViewEfl::InitAsChild(gfx::NativeView /* parent_view */) {
  content_image_elm_host_ = elm_bg_add(main_layout_);
  elm_object_part_content_set(main_layout_, "content", content_image_elm_host_);

  content_image_ = evas_object_image_filled_add(evas_);
  elm_object_part_content_set(content_image_elm_host_, "overlay",
                              content_image_);
  elm_object_focus_allow_set(content_image_elm_host_, EINA_TRUE);

  Evas_Object* smart_parent = evas_object_smart_parent_get(main_layout_);
  if (smart_parent) {
    // If our parent is a member of some smart object we also want
    // to join that group.
    smart_parent_ = smart_parent;
    evas_object_smart_member_add(content_image_elm_host_, smart_parent);
  }

  int x, y, width = 0, height = 0;
  evas_object_geometry_get(main_layout_, &x, &y, &width, &height);
  if (width == 0 || height == 0)
    width = height = 1;
  evas_object_image_size_set(content_image_, width, height);
  evas_object_geometry_set(content_image_elm_host_, x, y, width, height);

  evas_object_event_callback_add(main_layout_, EVAS_CALLBACK_RESIZE,
                                 OnParentViewResize, this);
  evas_object_event_callback_add(content_image_, EVAS_CALLBACK_MOVE, OnMove,
                                 this);
  evas_object_event_callback_add(content_image_, EVAS_CALLBACK_FOCUS_IN,
                                 OnFocusIn, this);
  evas_object_event_callback_add(content_image_, EVAS_CALLBACK_FOCUS_OUT,
                                 OnFocusOut, this);
  evas_object_smart_callback_add(content_image_elm_host_, "focused",
                                 OnHostFocusIn, this);
  evas_object_smart_callback_add(content_image_elm_host_, "unfocused",
                                 OnHostFocusOut, this);

  evas_object_event_callback_add(content_image_, EVAS_CALLBACK_MOUSE_DOWN,
                                 OnMouseDown, this);
  evas_object_event_callback_add(content_image_, EVAS_CALLBACK_MOUSE_UP,
                                 OnMouseUp, this);
  evas_object_event_callback_add(content_image_, EVAS_CALLBACK_MOUSE_MOVE,
                                 OnMouseMove, this);
  evas_object_event_callback_add(content_image_, EVAS_CALLBACK_MOUSE_OUT,
                                 OnMouseOut, this);
  evas_object_event_callback_add(content_image_, EVAS_CALLBACK_MOUSE_WHEEL,
                                 OnMouseWheel, this);
  evas_object_event_callback_add(content_image_, EVAS_CALLBACK_KEY_DOWN,
                                 OnKeyDown, this);
  evas_object_event_callback_add(content_image_, EVAS_CALLBACK_KEY_UP, OnKeyUp,
                                 this);

  evas_object_event_callback_add(content_image_, EVAS_CALLBACK_MULTI_DOWN,
                                 OnMultiTouchDownEvent, this);
  evas_object_event_callback_add(content_image_, EVAS_CALLBACK_MULTI_MOVE,
                                 OnMultiTouchMoveEvent, this);
  evas_object_event_callback_add(content_image_, EVAS_CALLBACK_MULTI_UP,
                                 OnMultiTouchUpEvent, this);

#if defined(TIZEN_VD_DISABLE_GPU)
  LOG(INFO) << "Disable IMContextEfl and evas gl for non gpu model";
#else
  // IMContext calls evas() getter on 'this' so it needs to be
  // initialized after evas_ is valid
  im_context_ = IMContextEfl::Create(this);
#if defined(OS_TIZEN)
  RenderViewHost* rvh = web_contents_.GetRenderViewHost();
  if (rvh && rvh->GetWebkitPreferences().mirror_blur_enabled) {
    LOG(INFO) << "Enable image blur effect";
    InitBlurEffect(true);
  }
#endif
  Init_EvasGL(width, height);
#if defined(OS_TIZEN)
  if (need_effect_)
    mirror_effect_ = MirroredBlurEffect::Create(evas_gl_api_);
#endif
#endif
}

void RenderWidgetHostViewEfl::InitBlurEffect(bool state) {
  if (need_effect_ == state)
    return;

  mirror_effect_image_ = evas_object_image_add(evas_);
  evas_object_image_alpha_set(mirror_effect_image_, false);
  evas_object_image_filled_set(mirror_effect_image_, true);
  evas_object_resize(mirror_effect_image_, 360, 360);
  evas_object_image_size_set(mirror_effect_image_, 360, 360);
  evas_object_image_fill_set(mirror_effect_image_, 0, 0, 360, 360);
  evas_object_pass_events_set(mirror_effect_image_, 1);
  evas_object_show(mirror_effect_image_);
  need_effect_ = state;
}

#if defined(NDEBUG)
#define GL_CHECK_HELPER(code, msg) ((code), false)
#else
static GLenum g_gl_err;
#define GL_CHECK_HELPER(code, msg)                                          \
  (((void)(code), ((g_gl_err = evas_gl_api_->glGetError()) == GL_NO_ERROR)) \
       ? false                                                              \
       : ((LOG(ERROR) << "GL Error: " << g_gl_err << "    " << msg), true))
#endif

#define GL_CHECK(code) GL_CHECK_HELPER(code, "")
#define GL_CHECK_STATUS(msg) GL_CHECK_HELPER(1, msg)

RenderWidgetHostViewEfl::~RenderWidgetHostViewEfl() {
  MakeCurrent();
  if (need_effect_) {
    mirror_effect_->ClearBlurEffectResources();
    need_effect_ = false;
  }

#if !defined(TIZEN_VD_DISABLE_GPU)
  evasgl_delegated_frame_host_.reset();
#endif
  ClearCurrent();

  if (im_context_)
    delete im_context_;

  evas_object_event_callback_del_full(main_layout_, EVAS_CALLBACK_RESIZE,
                                      OnParentViewResize, this);
  evas_object_event_callback_del(content_image_, EVAS_CALLBACK_MOVE, OnMove);
  evas_object_event_callback_del(content_image_, EVAS_CALLBACK_FOCUS_IN,
                                 OnFocusIn);
  evas_object_event_callback_del(content_image_, EVAS_CALLBACK_FOCUS_OUT,
                                 OnFocusOut);
  evas_object_smart_callback_del(content_image_elm_host_, "focused",
                                 OnHostFocusIn);
  evas_object_smart_callback_del(content_image_elm_host_, "unfocused",
                                 OnHostFocusOut);

  evas_object_event_callback_del(content_image_, EVAS_CALLBACK_MOUSE_DOWN,
                                 OnMouseDown);
  evas_object_event_callback_del(content_image_, EVAS_CALLBACK_MOUSE_UP,
                                 OnMouseUp);
  evas_object_event_callback_del(content_image_, EVAS_CALLBACK_MOUSE_MOVE,
                                 OnMouseMove);
  evas_object_event_callback_del(content_image_, EVAS_CALLBACK_MOUSE_OUT,
                                 OnMouseOut);
  evas_object_event_callback_del(content_image_, EVAS_CALLBACK_MOUSE_WHEEL,
                                 OnMouseWheel);
  evas_object_event_callback_del(content_image_, EVAS_CALLBACK_KEY_DOWN,
                                 OnKeyDown);
  evas_object_event_callback_del(content_image_, EVAS_CALLBACK_KEY_UP, OnKeyUp);

  evas_object_event_callback_del(content_image_, EVAS_CALLBACK_MULTI_DOWN,
                                 OnMultiTouchDownEvent);
  evas_object_event_callback_del(content_image_, EVAS_CALLBACK_MULTI_MOVE,
                                 OnMultiTouchMoveEvent);
  evas_object_event_callback_del(content_image_, EVAS_CALLBACK_MULTI_UP,
                                 OnMultiTouchUpEvent);

  evas_object_image_native_surface_set(mirror_effect_image_, nullptr);
  evas_object_image_pixels_get_callback_set(mirror_effect_image_, nullptr,
                                            nullptr);

#if defined(OS_TIZEN_TV_PRODUCT)
  ClearLabels();
#endif

  elm_object_part_content_unset(content_image_elm_host_, "overlay");
  evas_object_del(content_image_);

  content_image_elm_host_ = nullptr;
  content_image_ = nullptr;
  smart_parent_ = nullptr;
  main_layout_ = nullptr;
  mirror_effect_image_ = nullptr;
}

gfx::Point RenderWidgetHostViewEfl::ConvertPointInViewPix(gfx::Point point) {
  return gfx::ToFlooredPoint(
      gfx::ScalePoint(gfx::PointF(point), device_scale_factor_));
}

gfx::Rect RenderWidgetHostViewEfl::GetViewBoundsInPix() const {
  int x, y, w, h;
  evas_object_geometry_get(main_layout_, &x, &y, &w, &h);
  y += visible_top_controls_height_in_pix_;
  h -= visible_top_controls_height_in_pix_ +
       GetVisibleBottomControlsHeightInPix();

  return gfx::Rect(x, y, w, h);
}

bool RenderWidgetHostViewEfl::ClearCurrent() {
  return evas_gl_make_current(evas_gl_, 0, 0);
}

bool RenderWidgetHostViewEfl::MakeCurrent() {
  return evas_gl_make_current(evas_gl_, evas_gl_surface_, evas_gl_context_);
}

RenderFrameHostImpl* RenderWidgetHostViewEfl::GetFocusedFrame() {
  RenderViewHost* rvh = RenderViewHost::From(host_);
  if (!rvh)
    return nullptr;
  FrameTreeNode* focused_frame =
      rvh->GetDelegate()->GetFrameTree()->GetFocusedFrame();
  if (!focused_frame)
    return nullptr;

  return focused_frame->current_frame_host();
}

void RenderWidgetHostViewEfl::SetTopControlsHeight(size_t top_height,
                                                   size_t bottom_height) {
  top_controls_height_ = gfx::ToFlooredInt(top_height / device_scale_factor_);
  bottom_controls_height_ =
      gfx::ToFlooredInt(bottom_height / device_scale_factor_);
  top_controls_offset_correction_ =
      top_height -
      gfx::ToCeiledInt(top_controls_height_ * device_scale_factor_);

  if (!im_context_ || !im_context_->WebViewWillBeResized())
    host_->WasResized();

  evas_object_move(content_image_, 0, -GetVisibleBottomControlsHeightInPix());
}

bool RenderWidgetHostViewEfl::SetTopControlsState(
    BrowserControlsState constraint,
    BrowserControlsState current,
    bool animate) {
  auto rvh = web_contents_.GetRenderViewHost();
  if (!rvh)
    return false;

  return rvh->Send(new ViewMsg_UpdateBrowserControlsState(
      rvh->GetRoutingID(), constraint, current, animate));
}

bool RenderWidgetHostViewEfl::TopControlsAnimationScheduled() {
  return visible_top_controls_height_ != top_controls_height_ &&
         visible_top_controls_height_ != 0;
}

void RenderWidgetHostViewEfl::ResizeIfNeededByTopControls() {
  if (TopControlsAnimationScheduled() ||
      (last_resized_visible_top_controls_height_ ==
       visible_top_controls_height_))
    return;

  host_->WasResized();
  last_resized_visible_top_controls_height_ = visible_top_controls_height_;
}

void RenderWidgetHostViewEfl::TopControlsOffsetChanged() {
  float visible_top_controls_height =
      top_controls_height_ * top_controls_shown_ratio_;
  visible_top_controls_height_ = std::round(visible_top_controls_height);
  visible_top_controls_height_in_pix_ =
      std::round(visible_top_controls_height * device_scale_factor_);

  float visible_bottom_controls_height =
      bottom_controls_height_ * top_controls_shown_ratio_;
  visible_bottom_controls_height_ = std::round(visible_bottom_controls_height);
  visible_bottom_controls_height_in_pix_ =
      std::round(visible_bottom_controls_height * device_scale_factor_);

  // Send top controls offset to browser.
  int top_controls_offset =
      gfx::ToRoundedInt((visible_top_controls_height - top_controls_height_) *
                        device_scale_factor_);
  if (top_controls_offset)
    top_controls_offset -= top_controls_offset_correction_;
  web_contents_.GetDelegate()->DidTopControlsMove(top_controls_offset);

  if (!PointerStatePointerCount())
    ResizeIfNeededByTopControls();

  // Restore selection controls after top controls animaition.
  if (defer_showing_selection_controls_ && !TopControlsAnimationScheduled() &&
      GetSelectionController()) {
    GetSelectionController()->SetControlsTemporarilyHidden(false);
    defer_showing_selection_controls_ = false;
  }

  UpdateEdgeEffect();
  if (!im_context_ || !im_context_->IsVisible())
    evas_object_move(content_image_, 0, -GetVisibleBottomControlsHeightInPix());
}

int RenderWidgetHostViewEfl::GetVisibleBottomControlsHeightInPix() const {
  if (IsIMEShow() || !bottom_controls_height_)
    return 0;

  return visible_bottom_controls_height_in_pix_;
}

bool RenderWidgetHostViewEfl::DoBrowserControlsShrinkBlinkSize() const {
  return visible_top_controls_height_ == top_controls_height_;
}

float RenderWidgetHostViewEfl::GetTopControlsHeight() const {
  return top_controls_height_;
}

float RenderWidgetHostViewEfl::GetBottomControlsHeight() const {
  if (IsIMEShow())
    return 0.f;

  return bottom_controls_height_;
}

void RenderWidgetHostViewEfl::UpdateRotationDegrees(int rotation_degrees) {
  display::DeviceDisplayInfoEfl display_info;
  display_info.SetRotationDegrees(rotation_degrees);
  rotation_ = rotation_degrees;
  Send(new ViewMsg_UpdateRotationDegrees(host_->GetRoutingID(),
                                         display_info.GetRotationDegrees()));
}

void RenderWidgetHostViewEfl::SetClearTilesOnHide(bool enable) {
  Send(new ViewMsg_SetClearTilesOnHide(host_->GetRoutingID(), enable));
}

Evas_GL_API* RenderWidgetHostViewEfl::GetEvasGLAPI() {
  return evas_gl_api_;
}

Evas_GL* RenderWidgetHostViewEfl::GetEvasGL() {
  return evas_gl_;
}

void RenderWidgetHostViewEfl::DidMoveWebView() {
#if defined(TIZEN_VIDEO_HOLE)
  if (!on_webview_moved_callback_.is_null())
    on_webview_moved_callback_.Run();
#endif
}
void RenderWidgetHostViewEfl::DelegatedFrameHostSendReclaimCompositorResources(
    const viz::LocalSurfaceId& local_surface_id,
    const std::vector<viz::ReturnedResource>& resources) {
  renderer_compositor_frame_sink_->DidReceiveCompositorFrameAck(resources);
}

void RenderWidgetHostViewEfl::InitializeDeviceDisplayInfo() {
  static bool initialized = false;
  if (initialized)
    return;

  initialized = true;

  int display_width = 0, display_height = 0, dpi = 0;
  const Ecore_Evas* ee = ecore_evas_ecore_evas_get(evas_);
  ecore_evas_screen_geometry_get(ee, nullptr, nullptr, &display_width,
                                 &display_height);
  ecore_evas_screen_dpi_get(ee, &dpi, nullptr);

  display::DeviceDisplayInfoEfl display_info;
  display_info.Update(display_width, display_height,
                      display_info.ComputeDIPScale(dpi),
                      ecore_evas_rotation_get(ee));
}

void RenderWidgetHostViewEfl::Invalidate(bool immediate) {
#if defined(DIRECT_CANVAS)
  if (gpu::InProcessGpuService::GetDirectCanvasEnabled())
    return;
#endif
  TTRACE_WEB("RenderWidgetHostViewEfl::Invalidate");
  if (immediate)
    evas_render(evas_);
  else
    evas_object_image_pixels_dirty_set(content_image_, true);

  if (need_effect_)
    evas_object_image_pixels_dirty_set(mirror_effect_image_, true);
}

void RenderWidgetHostViewEfl::SetTileCoverAreaMultiplier(
    float cover_area_multiplier) {
  Send(new ViewMsg_SetTileCoverAreaMultiplier(host_->GetRoutingID(),
                                              cover_area_multiplier));
}

void RenderWidgetHostViewEfl::SwapBrowserFrame(
    const viz::LocalSurfaceId& local_surface_id,
    viz::CompositorFrame frame) {
  TTRACE_WEB("RenderWidgetHostViewEfl::SwapBrowserFrame");

#if !defined(TIZEN_VD_DISABLE_GPU)
  // Swap frame using evasgl delegated frame host.
  DCHECK(evasgl_delegated_frame_host_);
  evasgl_delegated_frame_host_->SwapDelegatedFrame(local_surface_id,
                                                   std::move(frame));
#endif
  web_contents_.GetDelegate()->DidRenderFrame();

#if defined(TIZEN_TBM_SUPPORT)
  if (OffscreenRenderingEnabled()) {
    RenderBrowserFrame();
    return;
  }
#endif

  Invalidate(false);
}

void RenderWidgetHostViewEfl::ClearBrowserFrame(SkColor bg_color) {
  DCHECK(evas_gl_api_);
#if defined(TIZEN_TBM_SUPPORT)
  if (OffscreenRenderingEnabled())
    return;
#endif

  if (!SkColorGetA(bg_color)) {
    WebContentsImpl* wci = static_cast<WebContentsImpl*>(&web_contents_);
    WebContentsViewEfl* wcve = static_cast<WebContentsViewEfl*>(wci->GetView());
    bg_color = wcve->GetBackgroundColor();
  }

#if defined(OS_TIZEN_TV_PRODUCT)
  // Invert the bg_color if high contrast applied
  if (layer_inverted_)
    bg_color = SK_ColorWHITE - bg_color;
#endif

  GL_CHECK(evas_gl_api_->glClearColor(
      static_cast<float>(SkColorGetR(bg_color)) / 255.0f,
      static_cast<float>(SkColorGetG(bg_color)) / 255.0f,
      static_cast<float>(SkColorGetB(bg_color)) / 255.0f,
      static_cast<float>(SkColorGetA(bg_color)) / 255.0f));
  GL_CHECK(evas_gl_api_->glClear(GL_COLOR_BUFFER_BIT));
}

void RenderWidgetHostViewEfl::RenderBrowserFrameOnMirror() {
  if (!MakeCurrent()) {
    LOG(ERROR) << "MakeCurrent() failed. Frame cannot be rendered.";
    return;
  }
  GL_CHECK(evas_gl_api_->glBindFramebuffer(GL_FRAMEBUFFER, 0));
  mirror_effect_->PaintToBackupTextureEffect();
  ClearCurrent();
}

#if defined(TIZEN_TBM_SUPPORT)
bool RenderWidgetHostViewEfl::OffscreenRenderingEnabled() {
  if (!web_contents_.GetDelegate())
    return false;
  return web_contents_.GetDelegate()->OffscreenRenderingEnabled();
}
#endif

void RenderWidgetHostViewEfl::RenderBrowserFrame() {
  TTRACE_WEB("RenderWidgetHostViewEfl::RenderBrowserFrame");
  if (!MakeCurrent()) {
    LOG(ERROR) << "RenderWidgetHostViewEfl::MakeCurrent() failed. Frame"
               << "cannot be rendered.";
    return;
  }
  frame_rendered_ = true;
  if (frame_data_output_rect_width_) {
    web_contents_.GetDelegate()->DidRenderRotatedFrame(
        rotation_, frame_data_output_rect_width_);
  }

#if defined(OS_TIZEN)
  if (need_effect_) {
    mirror_effect_->InitializeForEffect();
    GLuint mirror_effect_fbo_ = mirror_effect_->GetFBO();
    evasgl_delegated_frame_host_->SetFBOId(mirror_effect_fbo_);
  }
#endif

#if !defined(TIZEN_VD_DISABLE_GPU)
  // Render frame using evasgl delegated frame host.
  DCHECK(evasgl_delegated_frame_host_);
  evasgl_delegated_frame_host_->RenderDelegatedFrame(GetViewBoundsInPix());
#if defined(OS_TIZEN_TV_PRODUCT)
  // If direct rendering is disabled, call |glFlush| to use |glWaitSync|
  // function. |ClearCurrent| skip when indirect mode. This is the request of
  // the Evas team.
  if (content::IsHbbTV())
    evas_gl_api_->glFlush();
  else
#endif
#endif
    ClearCurrent();

  // Get snapshot for magnifier.
  if (!magnifier_snapshot_request_.is_null()) {
    magnifier_snapshot_request_.Run();
    magnifier_snapshot_request_.Reset();
  }

  if (!snapshot_task_list_.empty() && !snapshot_timer_->IsRunning()) {
#if defined(USE_TTRACE)
    traceMark(TTRACE_TAG_WEB, "Start SnapshotTimer, delay:%dms",
              kSnapshotProcessDelay);
#endif
    // Get snapshot after kSnapshotProcessDelay(100ms)
    snapshot_timer_->Start(
        FROM_HERE, base::TimeDelta::FromMilliseconds(kSnapshotProcessDelay),
        process_snapshot_request_callback_);
  }

#if defined(TIZEN_TBM_SUPPORT)
  if (OffscreenRenderingEnabled()) {
    web_contents_.GetDelegate()->DidRenderOffscreenFrame(
        evasgl_delegated_frame_host_->RenderedOffscreenBuffer());
  }
#endif
}

void RenderWidgetHostViewEfl::EvasObjectImagePixelsGetCallback(
    void* data,
    Evas_Object* obj) {
#if defined(DIRECT_CANVAS)
  if (gpu::InProcessGpuService::GetDirectCanvasEnabled())
    return;
#endif
  TTRACE_WEB("RenderWidgetHostViewEfl::EvasObjectImagePixelsGetCallback");
  RenderWidgetHostViewEfl* rwhv_efl =
      reinterpret_cast<RenderWidgetHostViewEfl*>(data);
  rwhv_efl->RenderBrowserFrame();
}

void RenderWidgetHostViewEfl::EvasObjectMirrorEffectImagePixelsGetCallback(
    void* data,
    Evas_Object* obj) {
  RenderWidgetHostViewEfl* rwhv_efl =
      reinterpret_cast<RenderWidgetHostViewEfl*>(data);
  rwhv_efl->RenderBrowserFrameOnMirror();
}

void RenderWidgetHostViewEfl::Init_EvasGL(int width, int height) {
  CHECK(width > 0 && height > 0);

  evas_gl_config_ = evas_gl_config_new();
  evas_gl_config_->options_bits = (Evas_GL_Options_Bits)(
      EVAS_GL_OPTIONS_DIRECT | EVAS_GL_OPTIONS_DIRECT_MEMORY_OPTIMIZE |
      EVAS_GL_OPTIONS_DIRECT_OVERRIDE | EVAS_GL_OPTIONS_CLIENT_SIDE_ROTATION);
  evas_gl_config_->color_format = EVAS_GL_RGBA_8888;
  evas_gl_config_->depth_bits = EVAS_GL_DEPTH_BIT_24;
  evas_gl_config_->stencil_bits = EVAS_GL_STENCIL_BIT_8;
#if defined(OS_TIZEN_TV_PRODUCT)
  if (IsHbbTV()) {
    evas_gl_config_->options_bits =
        (Evas_GL_Options_Bits)(EVAS_GL_OPTIONS_NONE);

    LOG(INFO) << "Disable depth/stencil bits when HBBTV";
    evas_gl_config_->depth_bits = EVAS_GL_DEPTH_NONE;
    evas_gl_config_->stencil_bits = EVAS_GL_STENCIL_NONE;
    // If set EVAS_GL_EGL_SYNC_ON enviroment value, the Evas_gl converted
    // glFlush to glWaitSync.
    if (setenv("EVAS_GL_EGL_SYNC_ON", "1", 1))
      LOG(ERROR) << "setenv failed ";
  } else {
    evas_gl_config_->color_format = EVAS_GL_RGB_888;
  }

#if defined(TIZEN_TBM_SUPPORT)
  if (OffscreenRenderingEnabled()) {
    evas_gl_config_->options_bits =
        (Evas_GL_Options_Bits)(EVAS_GL_OPTIONS_NONE);
    evas_gl_config_->color_format = EVAS_GL_RGBA_8888;
    evas_gl_config_->depth_bits = EVAS_GL_DEPTH_NONE;
    evas_gl_config_->stencil_bits = EVAS_GL_STENCIL_NONE;
  }
#endif
#endif

  evas_gl_ = evas_gl_new(evas_);

  Evas_GL_Context_Version context_version = EVAS_GL_GLES_2_X;
  if (GLSharedContextEfl::GetShareGroup()->GetUseGLES3())
    context_version = EVAS_GL_GLES_3_X;

  evas_gl_context_ = evas_gl_context_version_create(
      evas_gl_, GLSharedContextEfl::GetEvasGLContext(), context_version);
  if (!evas_gl_context_)
    LOG(FATAL) << "Failed to create evas gl context";

  evas_gl_api_ = evas_gl_context_api_get(evas_gl_, evas_gl_context_);

  if (width > MAX_SURFACE_WIDTH_EGL)
    width = MAX_SURFACE_WIDTH_EGL;

  if (height > MAX_SURFACE_HEIGHT_EGL)
    height = MAX_SURFACE_HEIGHT_EGL;

#if defined(TIZEN_TBM_SUPPORT)
  if (OffscreenRenderingEnabled()) {
    // In this case we draws scene to offscreen surface. Minimize width and
    // height to reduce memory.
    width = 1;
    height = 1;
  }
#endif

  ScopedRestoreNonOwnedEvasGLContext restore_context;
  CreateNativeSurface(width, height);

  MakeCurrent();
#if !defined(TIZEN_VD_DISABLE_GPU)
  evasgl_delegated_frame_host_->Initialize();
#endif
  ClearCurrent();

  evas_gl_initialized_ = true;
}

void RenderWidgetHostViewEfl::CreateNativeSurface(int width, int height) {
#if defined(TIZEN_VD_DISABLE_GPU)
  return;
#endif

  if (width == 0 || height == 0) {
    LOG(WARNING) << "Invalid surface size: " << width << "x" << height;
    return;
  }

  if (evas_gl_surface_) {
    evas_object_image_native_surface_set(content_image_, NULL);
    evas_gl_surface_destroy(evas_gl_, evas_gl_surface_);
    ClearCurrent();
  }

  evas_gl_surface_ =
      evas_gl_surface_create(evas_gl_, evas_gl_config_, width, height);
  if (!evas_gl_surface_)
    LOG(FATAL) << "Failed to create evas gl surface";

  Evas_Native_Surface nativeSurface;
  if (evas_gl_native_surface_get(evas_gl_, evas_gl_surface_, &nativeSurface)) {
    evas_object_image_native_surface_set(content_image_, &nativeSurface);
    if (need_effect_) {
      evas_object_image_native_surface_set(mirror_effect_image_,
                                           &nativeSurface);
      evas_object_image_pixels_get_callback_set(
          mirror_effect_image_, EvasObjectMirrorEffectImagePixelsGetCallback,
          this);
    }

    evas_object_image_pixels_get_callback_set(
        content_image_, EvasObjectImagePixelsGetCallback, this);
    evas_object_image_pixels_dirty_set(content_image_, true);
    if (need_effect_)
      evas_object_image_pixels_dirty_set(mirror_effect_image_, true);

  } else {
    LOG(FATAL) << "Failed to get native surface";
  }
#if defined(DIRECT_CANVAS)
  MakeCurrent();
  GLSharedEvasGL::Initialize(content_image_, evas_gl_context_, evas_gl_surface_,
                             width, height);
#endif
}

void RenderWidgetHostViewEfl::SetEvasHandler(
    scoped_refptr<EvasEventHandler> evas_event_handler) {
  evas_event_handler_ = evas_event_handler;
}

#if defined(OS_TIZEN_TV_PRODUCT)
void RenderWidgetHostViewEfl::DidFinishNavigation() {
  if (renderer_initialized_) {
    SetLayerInverted(HighContrastController::Shared().CanApplyHighContrast(
        web_contents()->GetVisibleURL()));
  }
}

void RenderWidgetHostViewEfl::SetLayerInverted(bool inverted) {
  if (layer_inverted_ == inverted)
    return;

  layer_inverted_ = inverted;
  Send(new ViewMsg_SetLayerInverted(host_->GetRoutingID(), inverted));
}

void RenderWidgetHostViewEfl::OnDidInitializeRenderer() {
  renderer_initialized_ = true;
  SetLayerInverted(HighContrastController::Shared().CanApplyHighContrast(
      web_contents()->GetVisibleURL()));
}
#endif
void RenderWidgetHostViewEfl::OnSelectionRectReceived(
    const gfx::Rect& selection_rect) const {
  auto wci = static_cast<WebContentsImpl*>(&web_contents_);
  if (wci) {
    auto wcve = static_cast<WebContentsViewEfl*>(wci->GetView());
    if (wcve)
      wcve->OnSelectionRectReceived(selection_rect);
  }
}

bool RenderWidgetHostViewEfl::OnMessageReceived(const IPC::Message& message) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(RenderWidgetHostViewEfl, message)
    IPC_MESSAGE_HANDLER(ViewHostMsg_DidResize, OnDidResize)
    IPC_MESSAGE_HANDLER(InputHostMsg_DidHandleKeyEvent, OnDidHandleKeyEvent)
    IPC_MESSAGE_HANDLER(ViewHostMsg_Snapshot, OnSnapshotDataReceived)
    IPC_MESSAGE_HANDLER(ViewHostMsg_EditableContentChanged,
                        OnEditableContentChanged)
    IPC_MESSAGE_HANDLER(ViewHostMsg_SelectionRectReceived,
                        OnSelectionRectReceived)
#if defined(OS_TIZEN_TV_PRODUCT)
    IPC_MESSAGE_HANDLER(ViewHostMsg_DidInitializeRenderer,
                        OnDidInitializeRenderer)
#endif
#if defined(OS_TIZEN)
    IPC_MESSAGE_HANDLER(ViewHostMsg_AddEdgeEffectONSCROLLTizenUIF,
                        OnEdgeEffectONSCROLLTizenUIF)
#endif
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()
  return handled;
}

bool RenderWidgetHostViewEfl::Send(IPC::Message* message) {
  return host_->Send(message);
}

void RenderWidgetHostViewEfl::OnDidResize() {
  if (is_scrolling_needed_) {
    ScrollFocusedEditableNode();
    is_scrolling_needed_ = false;
  }
}

void RenderWidgetHostViewEfl::OnSnapshotDataReceived(SkBitmap bitmap,
                                                     int snapshotId) {
  Evas_Object* image = NULL;
  if (!bitmap.empty()) {
    image = evas_object_image_filled_add(evas_);
    evas_object_image_size_set(image, bitmap.width(), bitmap.height());
    evas_object_image_data_copy_set(image, bitmap.getPixels());
  }

  ScreenshotCapturedCallback* callback =
      screen_capture_cb_map_.Lookup(snapshotId);
  if (!callback) {
    return;
  }
  callback->Run(image);
  screen_capture_cb_map_.Remove(snapshotId);
}

void RenderWidgetHostViewEfl::OnEditableContentChanged() {
  if (selection_controller_)
    selection_controller_->ClearSelection();
}

#if defined(OS_TIZEN)
void RenderWidgetHostViewEfl::OnEdgeEffectONSCROLLTizenUIF(bool top,
                                                           bool bottom,
                                                           bool right,
                                                           bool left) {
  EnsureEdgeEffect().Enable();

  if (top)
    EnsureEdgeEffect().ShowOrHide(EdgeEffect::Direction::TOP, true);
  if (bottom)
    EnsureEdgeEffect().ShowOrHide(EdgeEffect::Direction::BOTTOM, true);
  if (left)
    EnsureEdgeEffect().ShowOrHide(EdgeEffect::Direction::LEFT, true);
  if (right)
    EnsureEdgeEffect().ShowOrHide(EdgeEffect::Direction::RIGHT, true);

  EnsureEdgeEffect().Hide();
  EnsureEdgeEffect().Disable();
}
#endif

void RenderWidgetHostViewEfl::InitAsPopup(RenderWidgetHostView*,
                                          const gfx::Rect&) {
  NOTIMPLEMENTED();
}

void RenderWidgetHostViewEfl::InitAsFullscreen(RenderWidgetHostView*) {
  NOTIMPLEMENTED();
}

RenderWidgetHost* RenderWidgetHostViewEfl::GetRenderWidgetHost() const {
  return host_;
}

#if defined(USE_WAYLAND)
#if TIZEN_VERSION_AT_LEAST(5, 0, 0)
Ecore_Wl2_Window* RenderWidgetHostViewEfl::GetEcoreWlWindow() const {
  const Ecore_Evas* ee = ecore_evas_ecore_evas_get(evas_);
  return ecore_evas_wayland2_window_get(ee);
}
#else
Ecore_Wl_Window* RenderWidgetHostViewEfl::GetEcoreWlWindow() const {
  const Ecore_Evas* ee = ecore_evas_ecore_evas_get(evas_);
  return ecore_evas_wayland_window_get(ee);
}
#endif  // TIZEN_VERSION_AT_LEAST(5, 0, 0)
#else
Ecore_X_Window RenderWidgetHostViewEfl::GetEcoreXWindow() const {
  const Ecore_Evas* ee = ecore_evas_ecore_evas_get(evas_);
  return ecore_evas_gl_x11_window_get(ee);
}
#endif  // USE_WAYLAND

void RenderWidgetHostViewEfl::SetSize(const gfx::Size& size) {
  // This is a hack. See WebContentsView::SizeContents
  int width = std::min(size.width(), EFL_MAX_WIDTH);
  int height = std::min(size.height(), EFL_MAX_HEIGHT);
  if (popup_type_ != blink::kWebPopupTypeNone) {
    // We're a popup, honor the size request.
    Ecore_Evas* ee = ecore_evas_ecore_evas_get(evas_);
    ecore_evas_resize(ee, width, height);
  }

  // Update the size of the RWH.
  // if (requested_size_.width() != width ||
  //    requested_size_.height() != height) {
  // Disabled for now, will enable it while implementing InitAsPopUp (P1) API
  host_->SendScreenRects();
  host_->WasResized();
  //}
}

void RenderWidgetHostViewEfl::SetBounds(const gfx::Rect& rect) {}

gfx::Vector2dF RenderWidgetHostViewEfl::GetLastScrollOffset() const {
  return last_scroll_offset_;
}

gfx::NativeView RenderWidgetHostViewEfl::GetNativeView() const {
  return content_image_elm_host_;
}

gfx::NativeViewId RenderWidgetHostViewEfl::GetNativeViewId() const {
  DCHECK(evas_gl_initialized_);
  Ecore_Evas* ee = ecore_evas_ecore_evas_get(evas_);
  return ecore_evas_window_get(ee);
}

gfx::NativeViewAccessible RenderWidgetHostViewEfl::GetNativeViewAccessible() {
  NOTIMPLEMENTED();
  return 0;
}

bool RenderWidgetHostViewEfl::IsSurfaceAvailableForCopy() const {
  return false;
}

void RenderWidgetHostViewEfl::Show() {
  evas_object_show(content_image_elm_host_);
  if (need_effect_)
    evas_object_show(mirror_effect_image_);
  host_->WasShown(ui::LatencyInfo());
}

void RenderWidgetHostViewEfl::Hide() {
  if (im_context_ && im_context_->IsVisible())
    im_context_->HidePanel();

  evas_object_hide(content_image_elm_host_);
  if (need_effect_)
    evas_object_hide(mirror_effect_image_);
  host_->WasHidden();
}

bool RenderWidgetHostViewEfl::IsShowing() {
  return evas_object_visible_get(content_image_elm_host_);
}

gfx::Rect RenderWidgetHostViewEfl::GetViewBounds() const {
  return gfx::ConvertRectToDIP(device_scale_factor_, GetViewBoundsInPix());
}

gfx::Size RenderWidgetHostViewEfl::GetVisibleViewportSize() const {
  if (!custom_viewport_size_.IsEmpty())
    return custom_viewport_size_;
  return GetViewBounds().size();
}

gfx::Size RenderWidgetHostViewEfl::GetPhysicalBackingSize() const {
  int w, h;
  evas_object_geometry_get(content_image_elm_host_, nullptr, nullptr, &w, &h);
  return gfx::Size(w, h);
}

bool RenderWidgetHostViewEfl::LockMouse() {
  NOTIMPLEMENTED();
  return false;
}

void RenderWidgetHostViewEfl::UnlockMouse() {
  NOTIMPLEMENTED();
}

void RenderWidgetHostViewEfl::Focus() {
  elm_object_focus_set(content_image_elm_host_, EINA_TRUE);
  evas_object_focus_set(content_image_, EINA_TRUE);
}

void RenderWidgetHostViewEfl::Unfocus() {
  elm_object_focus_set(content_image_elm_host_, EINA_FALSE);
  evas_object_focus_set(content_image_, EINA_FALSE);
}

bool RenderWidgetHostViewEfl::HasFocus() const {
  return evas_object_focus_get(content_image_);
}

void RenderWidgetHostViewEfl::OnDidHandleKeyEvent(
    const blink::WebInputEvent* input_event,
    bool processed) {
  if (!im_context_)
    return;

  im_context_->HandleKeyEvent(processed, input_event->GetType());
}

void RenderWidgetHostViewEfl::UpdateCursor(const WebCursor& webcursor) {
#if defined(OS_TIZEN_TV_PRODUCT)
  if (cursor_set_by_client_)
    return;

  CursorInfo cursor_info;
  webcursor.GetCursorInfo(&cursor_info);

  // If cursor name does not exist in theme, do not set the cursor.
  // Otherwise, no cursor will be shown. Cursor becomes invisible.
  const char* cursor_name = GetCursorName(cursor_info.type);
#if TIZEN_VERSION_AT_LEAST(5, 0, 0)
  Ecore_Wl2_Display* display = ecore_wl2_connected_display_get(NULL);
  Ecore_Wl2_Input* input = ecore_wl2_input_default_input_get(display);
  if (ecore_wl2_input_cursor_get(input, cursor_name) && HasFocus()) {
    ecore_wl2_input_cursor_from_name_set(input, cursor_name);
  }
#else
  if (ecore_wl_cursor_get(cursor_name) && HasFocus()) {
    LOG(INFO) << "UpdateCursor,cursor_name:" << cursor_name;
    ecore_wl_input_cursor_from_name_set(ecore_wl_input_get(), cursor_name);
  }
#endif  // TIZEN_VERSION_AT_LEAST(5, 0, 0)
#else
  if (is_loading_) {
// Setting native Loading cursor
#if defined(USE_WAYLAND)
#if TIZEN_VERSION_AT_LEAST(5, 0, 0)
    ecore_wl2_window_cursor_from_name_set(GetEcoreWlWindow(), "watch");
#else
    ecore_wl_window_cursor_from_name_set(GetEcoreWlWindow(), "watch");
#endif  // TIZEN_VERSION_AT_LEAST(5, 0, 0)
#else
    ecore_x_window_cursor_set(GetEcoreXWindow(),
                              ecore_x_cursor_shape_get(ECORE_X_CURSOR_CLOCK));
#endif
  } else {
    CursorInfo cursor_info;
    webcursor.GetCursorInfo(&cursor_info);

#if defined(USE_WAYLAND)
#if TIZEN_VERSION_AT_LEAST(5, 0, 0)
    ecore_wl2_window_cursor_from_name_set(GetEcoreWlWindow(),
                                          GetCursorName(cursor_info.type));
#else
    ecore_wl_window_cursor_from_name_set(GetEcoreWlWindow(),
                                         GetCursorName(cursor_info.type));
#endif  // TIZEN_VERSION_AT_LEAST(5, 0, 0)
#else
    int cursor_type = GetCursorType(cursor_info.type);
    ecore_x_window_cursor_set(GetEcoreXWindow(),
                              ecore_x_cursor_shape_get(cursor_type));
#endif
  }
#endif
}

void RenderWidgetHostViewEfl::SetIsLoading(bool is_loading) {
  is_loading_ = is_loading;
  UpdateCursor(WebCursor());
  if (disambiguation_popup_)
    disambiguation_popup_->Dismiss();
}
void RenderWidgetHostViewEfl::SetBackgroundColor(SkColor color) {
  // The renderer will feed its color back to us with the first CompositorFrame.
  // We short-cut here to show a sensible color before that happens.
  UpdateBackgroundColorFromRenderer(color);

  DCHECK(SkColorGetA(color) == SK_AlphaOPAQUE ||
         SkColorGetA(color) == SK_AlphaTRANSPARENT);
  host_->SetBackgroundOpaque(SkColorGetA(color) == SK_AlphaOPAQUE);
}

SkColor RenderWidgetHostViewEfl::background_color() const {
  return background_color_;
}

void RenderWidgetHostViewEfl::UpdateBackgroundColorFromRenderer(
    SkColor color) {
  if (color == background_color())
    return;
  background_color_ = color;
}

void RenderWidgetHostViewEfl::SetCustomViewportSize(const gfx::Size& size) {
  if (custom_viewport_size_ != size) {
    custom_viewport_size_ = size;
    // Take the view port change into account.
    host_->WasResized();
  }
}

const gfx::Size RenderWidgetHostViewEfl::GetScrollableSize() const {
  int width = 0;
  int height = 0;
  gfx::Rect viewport_size = GetViewBoundsInPix();
  if (scaled_contents_size_.width() > viewport_size.width()) {
    width = scaled_contents_size_.width() - viewport_size.width();
    // When device scale factor is larger than 1.0, content size is ceiled up
    // during converting pixel to dip in RenderWidgetHostViewEfl::GetViewBounds.
    // So we ignore when scroll size is 1. It is also same for height.
    // Please refer to https://review.tizen.org/gerrit/#/c/74044/.
    if (device_scale_factor_ > 1.0f && width == 1)
      width = 0;
  }
  if (scaled_contents_size_.height() > viewport_size.height()) {
    height = scaled_contents_size_.height() - viewport_size.height();
    if (device_scale_factor_ > 1.0f && height == 1)
      height = 0;
  }

  return gfx::Size(width, height);
}

void RenderWidgetHostViewEfl::TextInputStateChanged(
    const TextInputState& params) {
  WebContentsImpl* wci = static_cast<WebContentsImpl*>(&web_contents_);
  WebContentsViewEfl* wcve = static_cast<WebContentsViewEfl*>(wci->GetView());
  bool show_ime_if_needed = params.show_ime_if_needed;
  if ((!params.is_user_action && !wcve->UseKeyPadWithoutUserAction())
#if defined(OS_TIZEN_TV_PRODUCT)
      || !wcve->ImePanelEnabled()
#endif
      ) {
    show_ime_if_needed = false;
  }

  // Prevent scroll and zoom for autofocus'ed elements.
  if (show_ime_if_needed && params.type != ui::TEXT_INPUT_TYPE_NONE) {
    bool check_hw_keyboard = false;
    if (IsMobileProfile() || IsWearableProfile())
      check_hw_keyboard = is_hw_keyboard_connected_;

    if (im_context_ && !im_context_->IsVisible() && !check_hw_keyboard)
      is_scrolling_needed_ = true;
    else
      ScrollFocusedEditableNode();
  }

  if (GetSelectionController())
    GetSelectionController()->OnTextInputStateChanged();

  if (im_context_) {
    im_context_->SetIsInFormTag(params.is_in_form_tag);
#if defined(OS_TIZEN_MOBILE_PRODUCT)
    if (show_ime_if_needed)
      im_context_->PrevNextButtonUpdate(params.prev_status, params.next_status);
#endif

#if defined(OS_TIZEN_TV_PRODUCT)
    im_context_->UpdateInputMethodState(params.type, params.can_compose_inline,
                                        show_ime_if_needed,
                                        password_input_minlength_);
#else
    im_context_->UpdateInputMethodState(params.type, params.can_compose_inline,
                                        show_ime_if_needed);

#endif
  }
}

void RenderWidgetHostViewEfl::UpdateIMELayoutVariation(bool is_minimum_negative,
                                                       bool is_step_integer) {
  if (im_context_)
    im_context_->UpdateLayoutVariation(is_minimum_negative, is_step_integer);
}

void RenderWidgetHostViewEfl::ImeCancelComposition() {
  if (im_context_)
    im_context_->CancelComposition();
}

void RenderWidgetHostViewEfl::ImeCompositionRangeChanged(
    const gfx::Range& range,
    const std::vector<gfx::Rect>& character_bounds) {
  NOTIMPLEMENTED();
}

void RenderWidgetHostViewEfl::FocusedNodeChanged(
    bool is_editable_node,
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
    ) {
  if (im_context_ && is_focused_node_editable_ && is_editable_node) {
    // focus changed from an editable node to another editable node,
    // need reset ime context.
    // or else will cause previous preedit text be copied.
    im_context_->CancelComposition();
  }
  is_focused_node_editable_ = is_editable_node;
#if defined(OS_TIZEN_TV_PRODUCT)
  radio_or_checkbox_focused_ = is_radio_or_checkbox;
  password_input_minlength_ = password_input_minlength;
#endif
  if (GetSelectionController())
    GetSelectionController()->HideHandleAndContextMenu();

  if (im_context_ && im_context_->IsVisible() &&
      ClipboardHelperEfl::GetInstance()->IsClipboardWindowOpened()) {
    ClipboardHelperEfl::GetInstance()->CloseClipboardWindow();
  }
#if defined(USE_WAYLAND)
  is_content_editable_ = is_content_editable;
  ClipboardHelperEfl::GetInstance()->SetContentEditable(is_content_editable);
#endif
}

void RenderWidgetHostViewEfl::Destroy() {
  delete this;
}

void RenderWidgetHostViewEfl::SetTooltipText(const base::string16& text) {
  web_contents_.GetDelegate()->SetTooltipText(text);
}

void RenderWidgetHostViewEfl::SelectionChanged(const base::string16& text,
                                               size_t offset,
                                               const gfx::Range& range) {
  RenderWidgetHostViewBase::SelectionChanged(text, offset, range);

  if (im_context_ && range.start() == range.end()) {
    im_context_->SetSurroundingText(base::UTF16ToUTF8(text));
    im_context_->SetCaretPosition(range.start());
  }

  if (!GetSelectionController())
    return;

  base::string16 selectedText;
  if (!text.empty() && !range.is_empty())
    selectedText = GetSelectedText();
  GetSelectionController()->UpdateSelectionData(selectedText);
}

void RenderWidgetHostViewEfl::SelectionBoundsChanged(
    const ViewHostMsg_SelectionBounds_Params& params) {
  ViewHostMsg_SelectionBounds_Params guest_params(params);
  guest_params.anchor_rect =
      ConvertRectToPixel(device_scale_factor_, params.anchor_rect);
  guest_params.focus_rect =
      ConvertRectToPixel(device_scale_factor_, params.focus_rect);

  if (im_context_)
    im_context_->UpdateCaretBounds(
        gfx::UnionRects(guest_params.anchor_rect, guest_params.focus_rect));

  if (GetSelectionController())
    GetSelectionController()->SetIsAnchorFirst(guest_params.is_anchor_first);
}

void RenderWidgetHostViewEfl::DidStopFlinging() {
  EnsureEdgeEffect().Hide();

  SelectionControllerEfl* controller = GetSelectionController();
  if (!controller)
    return;

  // Show selection controls when scrolling with fling gesture ends.
  controller->SetControlsTemporarilyHidden(false);
}

void RenderWidgetHostViewEfl::EvasToBlinkCords(int x,
                                               int y,
                                               int* view_x,
                                               int* view_y) {
  gfx::Rect view_bounds = GetViewBoundsInPix();
  if (view_x) {
    *view_x = x - view_bounds.x();
    *view_x /= device_scale_factor_;
  }
  if (view_y) {
    *view_y = y - view_bounds.y();
    *view_y /= device_scale_factor_;
  }
}

void RenderWidgetHostViewEfl::SelectClosestWord(const gfx::Point& touch_point) {
  int view_x, view_y;
  EvasToBlinkCords(touch_point.x(), touch_point.y(), &view_x, &view_y);

  Send(new ViewMsg_SelectClosestWord(host_->GetRoutingID(), view_x, view_y));
}

void RenderWidgetHostViewEfl::ShowDisambiguationPopup(
    const gfx::Rect& rect_pixels,
    const SkBitmap& zoomed_bitmap) {
  handling_disambiguation_popup_gesture_ = true;
  if (!disambiguation_popup_)
    disambiguation_popup_.reset(
        new DisambiguationPopupEfl(content_image_, this));

  disambiguation_popup_->Show(rect_pixels, zoomed_bitmap);
}

void RenderWidgetHostViewEfl::DisambiguationPopupDismissed() {
  handling_disambiguation_popup_gesture_ = false;
}

void RenderWidgetHostViewEfl::HandleDisambiguationPopupMouseDownEvent(
    Evas_Event_Mouse_Down* evas_event) {
  blink::WebGestureEvent tap_down_event = MakeGestureEvent(
      blink::WebInputEvent::kGestureTapDown, content_image_, evas_event);
  tap_down_event.data.tap.width = 0;
  tap_down_event.data.tap.tap_count = 1;
  SendGestureEvent(tap_down_event);
}

void RenderWidgetHostViewEfl::HandleDisambiguationPopupMouseUpEvent(
    Evas_Event_Mouse_Up* evas_event) {
  blink::WebGestureEvent tap_event = MakeGestureEvent(
      blink::WebInputEvent::kGestureTap, content_image_, evas_event);
  tap_event.data.tap.width = 0;
  tap_event.data.tap.tap_count = 1;
  SendGestureEvent(tap_event);
}

bool RenderWidgetHostViewEfl::CanDispatchToConsumer(
    ui::GestureConsumer* consumer) {
  return this == consumer;
}

void RenderWidgetHostViewEfl::DispatchSyntheticTouchEvent(
    ui::TouchEvent* event) {}

void RenderWidgetHostViewEfl::DispatchGestureEvent(
    GestureConsumer* raw_input_consumer,
    ui::GestureEvent* event) {
  HandleGesture(event);
}

void RenderWidgetHostViewEfl::ProcessSnapshotRequest(bool no_swap) {
  // Process all snapshot requests pending now.
  if (no_swap || (!snapshot_timer_->IsRunning() && frame_rendered_)) {
    for (auto& task : snapshot_task_list_) {
      if (!task.is_null()) {
        task.Run();
        task.Reset();
      }
    }
  }
  // Stop now and clear task list
  snapshot_task_list_.clear();
}

Evas_Object* RenderWidgetHostViewEfl::GetSnapshot(
    const gfx::Rect& snapshot_area,
    float scale_factor,
    bool is_magnifier) {
#if defined(USE_TTRACE)
  TTRACE(TTRACE_TAG_WEB, "RenderWidgetHostViewEfl::GetSnapshot");
#endif
  if (scale_factor == 0.0)
    return nullptr;

  int rotation =
      display::Screen::GetScreen()->GetPrimaryDisplay().RotationAsDegree();

  bool is_evasgl_direct_landscape = false;
  // For EvasGL Direct Rendering in landscape mode
  if (!is_magnifier) {
    is_evasgl_direct_landscape = ((rotation == 90 || rotation == 270) &&
                                  !evas_gl_rotation_get(evas_gl_));
  }

  const gfx::Rect window_rect(
      ConvertRectToPixel(device_scale_factor_, GetBoundsInRootWindow()));
  // |view_rect| is absolute coordinate of webview.
  gfx::Rect view_rect = GetViewBoundsInPix();

  // the gl coordinate system is based on screen rect for direct rendering.
  // if direct rendering is disabled, the coordinate is based on webview.
  // TODO(is46.kim) : Even though evas direct rendering flag is set, direct
  // rendering may be disabled in the runtime depend on environment.
  // There is no way to get current rendering mode. |is_direct_rendering|
  // flag should be changed by current rendering mode.
  bool is_direct_rendering = !is_magnifier;
  if (!is_direct_rendering) {
    view_rect.set_x(0);
    view_rect.set_y(0);
  }
  const gfx::Size surface_size =
      (is_direct_rendering) ? window_rect.size() : view_rect.size();

  // |snapshot_area| is relative coordinate in webview.
  int x = snapshot_area.x();
  int y = snapshot_area.y();
  int width = snapshot_area.width();
  int height = snapshot_area.height();

  // Convert |snapshot_area| to absolute coordinate.
  x += view_rect.x();
  y += view_rect.y();

  // Limit snapshot rect by webview size
  if (x + width > view_rect.right())
    width = view_rect.right() - x;
  if (y + height > view_rect.bottom())
    height = view_rect.bottom() - y;

  // Convert coordinate system for OpenGL.
  // (0,0) is top left corner in webview coordinate system.
  if (is_evasgl_direct_landscape) {
    if (rotation == 270) {
      x = surface_size.width() - x - width;
      y = surface_size.height() - y - height;
    }
    std::swap(x, y);
    std::swap(width, height);
  } else {
    // (0,0) is bottom left corner in opengl coordinate system.
    y = surface_size.height() - y - height;
  }

  int size = width * height * sizeof(GLuint);
  GLuint* data = (GLuint*)malloc(size);
  if (!data) {
    LOG(ERROR) << "Failed to allocate memory for snapshot";
    return nullptr;
  }

  {
#if defined(USE_TTRACE)
    TTRACE(TTRACE_TAG_WEB, "RenderWidgetHostViewEfl::GetSnapshot(ReadPixels)");
#endif
    if (!MakeCurrent()) {
      LOG(ERROR) << "MakeCurrent() failed.";
      free(data);
      return nullptr;
    }
    Evas_GL_API* gl_api = evasGlApi();
    DCHECK(gl_api);
    gl_api->glReadPixels(x, y, width, height, GL_RGBA, GL_UNSIGNED_BYTE, data);
    ClearCurrent();
  }

  if (!is_evasgl_direct_landscape && (rotation == 90 || rotation == 270))
    std::swap(width, height);

#if _DUMP_SNAPSHOT_
  static int frame_number = 0;
  frame_number++;
  {
    char filename[150];
    sprintf(filename, "/opt/share/dump/snapshot%02d-%d_%dx%d_rgba8888.raw",
            frame_number, rotation, width, height);
    FILE* fp = fopen(filename, "w");
    if (fp) {
      fwrite(data, width * height * 4, 1, fp);
      fflush(fp);
      fclose(fp);
    } else {
      LOG(ERROR) << "Unable to open file";
    }
  }
#endif

  // flip the Y axis and change color format from RGBA to BGRA
  for (int j = 0; j < height / 2; j++) {
    for (int i = 0; i < width; i++) {
      GLuint& px1 = data[(j * width) + i];
      GLuint& px2 = data[((height - 1) - j) * width + i];
      px1 = ((px1 & 0xff) << 16) | ((px1 >> 16) & 0xff) | (px1 & 0xff00ff00);
      px2 = ((px2 & 0xff) << 16) | ((px2 >> 16) & 0xff) | (px2 & 0xff00ff00);
#if defined(OS_TIZEN_TV_PRODUCT)
      // Invert the snapshot if highcontrast applied
      if (!is_magnifier && layer_inverted_) {
        px1 = (~px1 & 0xffffff) | (px1 & 0xff000000);
        px2 = (~px2 & 0xffffff) | (px2 & 0xff000000);
      }
#endif
      std::swap(px1, px2);
    }
  }

  SkBitmap bitmap;
  if (scale_factor != 1.0 || rotation) {
    bitmap.setInfo(SkImageInfo::MakeN32Premul(width, height));
    bitmap.setPixels(data);

    // scaling bitmap
    if (scale_factor != 1.0) {
      width = scale_factor * width;
      height = scale_factor * height;
      bitmap = skia::ImageOperations::Resize(
          bitmap, skia::ImageOperations::RESIZE_GOOD, width, height);
    }

    if (rotation == 90) {
      bitmap = SkBitmapOperations::Rotate(bitmap,
                                          SkBitmapOperations::ROTATION_90_CW);
    } else if (rotation == 180) {
      bitmap = SkBitmapOperations::Rotate(bitmap,
                                          SkBitmapOperations::ROTATION_180_CW);
    } else if (rotation == 270) {
      bitmap = SkBitmapOperations::Rotate(bitmap,
                                          SkBitmapOperations::ROTATION_270_CW);
    }
  }

  if (rotation == 90 || rotation == 270)
    std::swap(width, height);

  Evas_Object* image = evas_object_image_filled_add(evas());
  if (image) {
    evas_object_image_size_set(image, width, height);
    evas_object_image_alpha_set(image, EINA_TRUE);
    evas_object_image_data_copy_set(image,
                                    bitmap.empty() ? data : bitmap.getPixels());
    evas_object_resize(image, width, height);
#if _DUMP_SNAPSHOT_
    char filename[150];
    sprintf(filename, "/opt/share/dump/snapshot%02d-%d_%dx%d.png", frame_number,
            rotation, width, height);
    evas_object_image_save(image, filename, NULL, "quality=100");
#endif
  }
  free(data);

  return image;
}

void RenderWidgetHostViewEfl::RunGetSnapshotOnMainThread(
    const gfx::Rect snapshot_area,
    int request_id,
    float scale_factor) {
#if defined(USE_TTRACE)
  TTRACE(TTRACE_TAG_WEB, "RenderWidgetHostViewEfl::RunGetSnapshotOnMainThread");
#endif
  ScreenshotCapturedCallback* callback =
      screen_capture_cb_map_.Lookup(request_id);

  if (!callback)
    return;

  callback->Run(GetSnapshot(snapshot_area, scale_factor));
  screen_capture_cb_map_.Remove(request_id);
}

bool RenderWidgetHostViewEfl::RequestSnapshotAsync(
    const gfx::Rect& snapshot_area,
    Screenshot_Captured_Callback callback,
    void* user_data,
    float scale_factor) {
  TTRACE_WEB("RenderWidgetHostViewEfl::RequestSnapshotAsync");

  ScreenshotCapturedCallback* cb =
      new ScreenshotCapturedCallback(callback, user_data);

  int callback_id = screen_capture_cb_map_.Add(base::WrapUnique(cb));

  if (IsShowing()) {
    // Create a snapshot task that will be executed after frame
    // is generated and drawn to surface.
    snapshot_task_list_.push_back(base::Bind(
        &RenderWidgetHostViewEfl::RunGetSnapshotOnMainThread,
        base::Unretained(this), snapshot_area, callback_id, scale_factor));
    // create a timer to request snapshot immediately when there is no swap
    // request from renderer.
    snapshot_timer_noswap_->Start(
        FROM_HERE, base::TimeDelta::FromMilliseconds(4 * kSnapshotProcessDelay),
        base::Bind(&RenderWidgetHostViewEfl::ProcessSnapshotRequest,
                   base::Unretained(this), false));
    return true;
  } else {
    // Sends message to View to capture the content snapshot
    return Send(new ViewMsg_CaptureRendererContentSnapShot(
        host_->GetRoutingID(), snapshot_area, scale_factor, callback_id));
  }
}

void RenderWidgetHostViewEfl::SetNeedsBeginFrames(bool needs_begin_frames) {}

void RenderWidgetHostViewEfl::GetMagnifierSnapshot(
    gfx::Rect snapshot_area,
    float scale_factor,
    std::unique_ptr<ScreenshotCapturedCallback> cb) {
#if defined(USE_TTRACE)
  TTRACE(TTRACE_TAG_WEB, "RenderWidgetHostViewEfl::GetMagnifierSnapshot");
#endif
  // GetSnapshot might be replaced with something designed for magnifier.
  Evas_Object* image = GetSnapshot(snapshot_area, scale_factor, true);

  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE,
      base::Bind(&ScreenshotCapturedCallback::Run, base::Owned(cb.release()),
                 base::Unretained(image)));
}

void RenderWidgetHostViewEfl::RequestMagnifierSnapshotAsync(
    const Eina_Rectangle rect,
    Screenshot_Captured_Callback callback,
    void* user_data,
    float scale_factor) {
  gfx::Rect snapshot_area(rect.x, rect.y, rect.w, rect.h);
  gfx::Rect view_rect = GetViewBoundsInPix();

  // Adjust snapshot rect for email app
  auto visible_viewport_rect = selection_controller_->GetVisibleViewportRect();
  int hidden_view_width = visible_viewport_rect.x() - view_rect.x();
  int hidden_view_height = visible_viewport_rect.y() - view_rect.y();
  if (hidden_view_height || hidden_view_width) {
    snapshot_area.set_x(snapshot_area.x() + hidden_view_width);
    snapshot_area.set_y(snapshot_area.y() + hidden_view_height);
  }

  magnifier_snapshot_request_ =
      base::Bind(&RenderWidgetHostViewEfl::GetMagnifierSnapshot,
                 base::Unretained(this), snapshot_area, scale_factor,
                 base::Passed(std::unique_ptr<ScreenshotCapturedCallback>(
                     new ScreenshotCapturedCallback(callback, user_data))));
}

void RenderWidgetHostViewEfl::DidOverscroll(
    const ui::DidOverscrollParams& params) {
  const gfx::Vector2dF& accumulated_overscroll = params.accumulated_overscroll;
  const gfx::Vector2dF& latest_overscroll_delta =
      params.latest_overscroll_delta;

  if (!touch_events_enabled_)
    return;

  if (GetScrollableSize().width() > 0) {
    if (latest_overscroll_delta.x() < 0 && (int)accumulated_overscroll.x() < 0)
      EnsureEdgeEffect().ShowOrHide(EdgeEffect::Direction::LEFT, true);
    if (latest_overscroll_delta.x() > 0 && (int)accumulated_overscroll.x() > 0)
      EnsureEdgeEffect().ShowOrHide(EdgeEffect::Direction::RIGHT, true);
  }

  if (GetScrollableSize().height() > 0) {
    if (latest_overscroll_delta.y() < 0 &&
        (int)accumulated_overscroll.y() < 0) {
      EnsureEdgeEffect().ShowOrHide(EdgeEffect::Direction::TOP, true);
    }
    if (latest_overscroll_delta.y() > 0 && (int)accumulated_overscroll.y() > 0)
      EnsureEdgeEffect().ShowOrHide(EdgeEffect::Direction::BOTTOM, true);
  }
}

bool RenderWidgetHostViewEfl::HasAcceleratedSurface(const gfx::Size&) {
  return false;
}

gfx::Rect RenderWidgetHostViewEfl::GetBoundsInRootWindow() {
  Ecore_Evas* ee = ecore_evas_ecore_evas_get(evas_);
  int x, y, w, h;
  ecore_evas_geometry_get(ee, &x, &y, &w, &h);
  if (current_orientation_ == 90 || current_orientation_ == 270)
    return gfx::ConvertRectToDIP(device_scale_factor_, gfx::Rect(x, y, h, w));
  return gfx::ConvertRectToDIP(device_scale_factor_, gfx::Rect(x, y, w, h));
}

void RenderWidgetHostViewEfl::RenderProcessGone(base::TerminationStatus,
                                                int error_code) {
  Destroy();
}

void RenderWidgetHostViewEfl::OnParentViewResize(void* data,
                                                 Evas*,
                                                 Evas_Object* obj,
                                                 void*) {
  RenderWidgetHostViewEfl* thiz = static_cast<RenderWidgetHostViewEfl*>(data);
  int x, y, width, height;
  evas_object_geometry_get(obj, &x, &y, &width, &height);
  evas_object_geometry_set(thiz->content_image_elm_host_, x, y, width, height);
  evas_object_image_size_set(thiz->content_image_, width, height);

  if (thiz->im_context_ && !thiz->im_context_->IsVisible())
    thiz->im_context_->SetDefaultViewSize(gfx::Size(width, height));

  if (thiz->GetVisibleBottomControlsHeightInPix()) {
    evas_object_move(
         thiz->content_image_, 0, -thiz->GetVisibleBottomControlsHeightInPix());
  }

  ScopedRestoreNonOwnedEvasGLContext restore_context;
  restore_context.WillDestorySurface(thiz->evas_gl_surface_);
  thiz->CreateNativeSurface(width, height);

  // NOTE: Call RenderWidgetHost::WasResized only when custom viewport is empty.
  // If custom viewport is being used, it has to be recalculated with
  // resized view bounds.
  // After that, RenderWidgetHost::WasResized will be called.
  if (thiz->custom_viewport_size_.IsEmpty())
    thiz->host_->WasResized();
  thiz->UpdateEdgeEffect();
}

void RenderWidgetHostViewEfl::UpdateEdgeEffect() {
  EnsureEdgeEffect().UpdatePosition(visible_top_controls_height_in_pix_,
                                    GetVisibleBottomControlsHeightInPix());
}

void RenderWidgetHostViewEfl::OnMove(void* data, Evas*, Evas_Object*, void*) {
  auto* thiz = static_cast<RenderWidgetHostViewEfl*>(data);
  thiz->UpdateEdgeEffect();
}

void RenderWidgetHostViewEfl::SetFocusInOutCallbacks(
    const base::Callback<void(void)>& on_focus_in,
    const base::Callback<void(void)>& on_focus_out) {
  on_focus_in_callback_ = on_focus_in;
  on_focus_out_callback_ = on_focus_out;
}

void RenderWidgetHostViewEfl::OnFocusIn(void* data,
                                        Evas*,
                                        Evas_Object*,
                                        void*) {
  RenderWidgetHostViewEfl* thiz = static_cast<RenderWidgetHostViewEfl*>(data);
  if (thiz->evas_event_handler_.get())
    if (thiz->evas_event_handler_->HandleEvent_FocusIn())
      return;

  thiz->host_->SetActive(true);
  thiz->host_->GotFocus();
  thiz->host_->WasShown(ui::LatencyInfo());

  Eina_Bool r = EINA_TRUE;
  r &=
      evas_object_key_grab(thiz->content_image_, kLeftKeyName, 0, 0, EINA_TRUE);
  r &= evas_object_key_grab(thiz->content_image_, kRightKeyName, 0, 0,
                            EINA_TRUE);
  r &= evas_object_key_grab(thiz->content_image_, kUpKeyName, 0, 0, EINA_TRUE);
  r &=
      evas_object_key_grab(thiz->content_image_, kDownKeyName, 0, 0, EINA_TRUE);
  DCHECK(r) << "Failed to grab arrow keys!";

  if (IsMobileProfile() && thiz->GetSelectionController()) {
    thiz->GetSelectionController()->ShowHandleAndContextMenuIfRequired();
  }
  if (!thiz->on_focus_in_callback_.is_null())
    thiz->on_focus_in_callback_.Run();

  if (thiz->im_context_)
    thiz->im_context_->OnFocusIn();
}

void RenderWidgetHostViewEfl::OnFocusOut(void* data,
                                         Evas*,
                                         Evas_Object*,
                                         void*) {
  RenderWidgetHostViewEfl* thiz = static_cast<RenderWidgetHostViewEfl*>(data);
  if (thiz->evas_event_handler_.get())
    if (thiz->evas_event_handler_->HandleEvent_FocusOut())
      return;

  if (thiz->im_context_ && thiz->im_context_->IsVisible())
    thiz->im_context_->HidePanel();
  else if (thiz->im_context_)
    thiz->im_context_->OnFocusOut();

  thiz->host_->SetActive(false);
  thiz->host_->LostCapture();
  thiz->host_->Blur();

  evas_object_key_ungrab(thiz->content_image_, kLeftKeyName, 0, 0);
  evas_object_key_ungrab(thiz->content_image_, kRightKeyName, 0, 0);
  evas_object_key_ungrab(thiz->content_image_, kUpKeyName, 0, 0);
  evas_object_key_ungrab(thiz->content_image_, kDownKeyName, 0, 0);

  if (IsMobileProfile() && thiz->GetSelectionController()) {
    thiz->GetSelectionController()->HideHandleAndContextMenu();
  }
  if (thiz->im_context_)
    thiz->im_context_->DoNotShowAfterResume();

  if (!thiz->on_focus_out_callback_.is_null())
    thiz->on_focus_out_callback_.Run();
}

void RenderWidgetHostViewEfl::OnHostFocusIn(void* data, Evas_Object*, void*) {
  RenderWidgetHostViewEfl* thiz = static_cast<RenderWidgetHostViewEfl*>(data);
  thiz->Focus();
  evas_object_focus_set(thiz->content_image_, EINA_TRUE);
}

void RenderWidgetHostViewEfl::OnHostFocusOut(void* data, Evas_Object*, void*) {
  RenderWidgetHostViewEfl* thiz = static_cast<RenderWidgetHostViewEfl*>(data);
  thiz->host_->Blur();
  evas_object_focus_set(thiz->content_image_, EINA_FALSE);
}

void RenderWidgetHostViewEfl::OnMouseDown(void* data,
                                          Evas* evas,
                                          Evas_Object* obj,
                                          void* event_info) {
  RenderWidgetHostViewEfl* rwhv = static_cast<RenderWidgetHostViewEfl*>(data);
  if (rwhv->evas_event_handler_.get())
    if (rwhv->evas_event_handler_->HandleEvent_MouseDown(
            static_cast<Evas_Event_Mouse_Down*>(event_info)))
      return;

  if (!rwhv->HasFocus())
    rwhv->Focus();

#if defined(OS_TIZEN_TV_PRODUCT)
  rwhv->ClearLabels();
#endif

  if (!rwhv->touch_events_enabled_) {
    blink::WebMouseEvent event =
        MakeWebMouseEvent(blink::WebInputEvent::kMouseDown, obj,
                          static_cast<Evas_Event_Mouse_Down*>(event_info));
#if defined(OS_TIZEN_TV_PRODUCT)
    if (!rwhv->on_mouse_down_callback_.is_null())
      rwhv->on_mouse_down_callback_.Run(event.PositionInWidget().x,
                                        event.PositionInWidget().y);
#endif

    rwhv->host_->ForwardMouseEvent(event);
  } else {
    rwhv->ProcessTouchEvents(
        static_cast<Evas_Event_Mouse_Down*>(event_info)->timestamp);
  }
}

void RenderWidgetHostViewEfl::OnMouseUp(void* data,
                                        Evas* evas,
                                        Evas_Object* obj,
                                        void* event_info) {
  RenderWidgetHostViewEfl* rwhv = static_cast<RenderWidgetHostViewEfl*>(data);
  if (rwhv->evas_event_handler_.get())
    if (rwhv->evas_event_handler_->HandleEvent_MouseUp(
            static_cast<Evas_Event_Mouse_Up*>(event_info)))
      return;

  if (!rwhv->touch_events_enabled_) {
#if defined(OS_TIZEN_TV_PRODUCT)
    if (!rwhv->on_mouse_up_callback_.is_null() &&
        rwhv->on_mouse_up_callback_.Run())
      return;
#endif

    blink::WebMouseEvent event =
        MakeWebMouseEvent(blink::WebInputEvent::kMouseUp, obj,
                          static_cast<Evas_Event_Mouse_Up*>(event_info));
    rwhv->host_->ForwardMouseEvent(event);
  } else {
    rwhv->ProcessTouchEvents(
        static_cast<Evas_Event_Mouse_Up*>(event_info)->timestamp);
  }
}

void RenderWidgetHostViewEfl::OnMouseMove(void* data,
                                          Evas* evas,
                                          Evas_Object* obj,
                                          void* event_info) {
  RenderWidgetHostViewEfl* rwhv = static_cast<RenderWidgetHostViewEfl*>(data);
  if (rwhv->evas_event_handler_.get())
    if (rwhv->evas_event_handler_->HandleEvent_MouseMove(
            static_cast<Evas_Event_Mouse_Move*>(event_info)))
      return;

  if (!rwhv->touch_events_enabled_) {
#if defined(OS_TIZEN_TV_PRODUCT)
    if (!rwhv->on_mouse_move_callback_.is_null())
      rwhv->on_mouse_move_callback_.Run();
#endif

    blink::WebMouseEvent event =
        MakeWebMouseEvent(obj, static_cast<Evas_Event_Mouse_Move*>(event_info));
    rwhv->host_->ForwardMouseEvent(event);
  } else {
    rwhv->ProcessTouchEvents(
        static_cast<Evas_Event_Mouse_Move*>(event_info)->timestamp);
  }
}

void RenderWidgetHostViewEfl::OnMouseOut(void* data,
                                         Evas* evas,
                                         Evas_Object* obj,
                                         void* event_info) {
  RenderWidgetHostViewEfl* rwhv = static_cast<RenderWidgetHostViewEfl*>(data);
  blink::WebMouseEvent event =
      MakeWebMouseEvent(obj, static_cast<Evas_Event_Mouse_Out*>(event_info));
  rwhv->host_->ForwardMouseEvent(event);
}

void RenderWidgetHostViewEfl::OnMultiTouchDownEvent(void* data,
                                                    Evas* evas,
                                                    Evas_Object* obj,
                                                    void* event_info) {
  RenderWidgetHostViewEfl* rwhv = static_cast<RenderWidgetHostViewEfl*>(data);
  CHECK(rwhv->touch_events_enabled_);
  rwhv->ProcessTouchEvents(
      static_cast<Evas_Event_Multi_Down*>(event_info)->timestamp, true);
}

void RenderWidgetHostViewEfl::OnMultiTouchMoveEvent(void* data,
                                                    Evas* evas,
                                                    Evas_Object* obj,
                                                    void* event_info) {
  RenderWidgetHostViewEfl* rwhv = static_cast<RenderWidgetHostViewEfl*>(data);
  CHECK(rwhv->touch_events_enabled_);
  rwhv->ProcessTouchEvents(
      static_cast<Evas_Event_Multi_Move*>(event_info)->timestamp, true);
}

void RenderWidgetHostViewEfl::RequestSelectionRect() const {
  auto rvh = web_contents_.GetRenderViewHost();
  if (rvh)
    rvh->Send(new ViewMsg_RequestSelectionRect(rvh->GetRoutingID()));
}

void RenderWidgetHostViewEfl::OnMultiTouchUpEvent(void* data,
                                                  Evas* evas,
                                                  Evas_Object* obj,
                                                  void* event_info) {
  RenderWidgetHostViewEfl* rwhv = static_cast<RenderWidgetHostViewEfl*>(data);
  CHECK(rwhv->touch_events_enabled_);
  rwhv->ProcessTouchEvents(
      static_cast<Evas_Event_Multi_Up*>(event_info)->timestamp, true);
}

void RenderWidgetHostViewEfl::OnKeyDown(void* data,
                                        Evas* evas,
                                        Evas_Object* obj,
                                        void* event_info) {
  if (!obj || !event_info)
    return;

  Evas_Event_Key_Down* key_down = static_cast<Evas_Event_Key_Down*>(event_info);
  RenderWidgetHostViewEfl* rwhv = static_cast<RenderWidgetHostViewEfl*>(data);
  if (rwhv->evas_event_handler_.get())
    if (rwhv->evas_event_handler_->HandleEvent_KeyDown(key_down))
      return;

  // This key shouldn't be processed as keydown event.
  // It is processed to EEXT_CALLBACK_MORE as H/W More key.
  // Do not forward XF86Menu key to prevent processing as keydown event.
  if (IsMobileProfile()) {
    if (!strcmp(key_down->key, "XF86Menu"))
      return;

    // If external keyboard is connected, clears selection's data whenever a
    // user presses arrow keys to prevent the engine from showing selection's
    // elements.
    if (rwhv->is_hw_keyboard_connected_ && rwhv->GetSelectionController() &&
        IsArrowKey(key_down->key))
      rwhv->GetSelectionController()->ClearSelection();

    // If external keyboard was already connected before launching application,
    // enables focus outline when first key down event is come.
    if (evas_device_name_get(key_down->dev)) {
      if (!rwhv->is_hw_keyboard_connected_ &&
          evas_device_class_get(key_down->dev) == EVAS_DEVICE_CLASS_KEYBOARD &&
          strstr(evas_device_name_get(key_down->dev), "Keyboard")) {
        rwhv->UpdateSpatialNavigationStatus(true);
      }
    }
  }

  if (rwhv->im_context_) {
    bool was_filtered = false;
    rwhv->im_context_->HandleKeyDownEvent(key_down, &was_filtered);
    // It is for checking whether key is filtered or not about keyUp event.
    // Because the process related with key is processed at
    // EVAS_CALLBACK_KEY_DOWN although key is processed by Ecore_IMF, it isn't
    // filtered by ecore_imf_context_filter_event about EVAS_CALLBACK_KEY_UP.
    rwhv->was_keydown_filtered_by_platform_ = was_filtered;

    if (IsTvProfile() && !strcmp(key_down->key, "Return"))
      rwhv->was_return_keydown_filtered_by_platform_ = was_filtered;

    if (was_filtered)
      return;
  }

  if (IsMobileProfile() || IsWearableProfile()) {
    if (!strcmp(key_down->key, "Return") && rwhv->im_context_ &&
        !rwhv->im_context_->IsDefaultReturnKeyType())
      rwhv->im_context_->HidePanel();
  }

  // When upper case letter is entered there are two events
  // Shift + letter key
  // Do not forward shift key to prevent shift keydown event.
  if (IsShiftKey(key_down->key))
    return;

  if (IsTvProfile())
    rwhv->ConvertEnterToSpaceIfNeeded(key_down);

  NativeWebKeyboardEvent n_event = MakeWebKeyboardEvent(true, key_down);

  if (!rwhv->im_context_ ||
      rwhv->im_context_->ShouldHandleImmediately(key_down->key)) {
    rwhv->host_->ForwardKeyboardEvent(n_event);
    return;
  }

  rwhv->im_context_->PushToKeyDownEventQueue(n_event);
}

void RenderWidgetHostViewEfl::OnKeyUp(void* data,
                                      Evas* evas,
                                      Evas_Object* obj,
                                      void* event_info) {
  if (!obj || !event_info)
    return;

  Evas_Event_Key_Up* key_up = static_cast<Evas_Event_Key_Up*>(event_info);
  RenderWidgetHostViewEfl* rwhv = static_cast<RenderWidgetHostViewEfl*>(data);
  if (rwhv->evas_event_handler_.get())
    if (rwhv->evas_event_handler_->HandleEvent_KeyUp(key_up))
      return;

  if (rwhv->im_context_) {
    bool was_filtered = false;
    rwhv->im_context_->HandleKeyUpEvent(key_up, &was_filtered);
  }

  // Both selected key and retrun key events are emitted together
  // while typing a single key on OSK by using remote controller.
  // The redundant retrun key event needs to be ignored here.
  if (IsTvProfile() && evas_device_name_get(key_up->dev)) {
    if (!strstr(evas_device_name_get(key_up->dev), "ime") &&
        rwhv->im_context_ && rwhv->im_context_->IsVisible() &&
        !strcmp(key_up->key, "Return"))
      return;
  }

  // When IME was focused out in keydown event handler,
  // 'was_filtered' will not give the right value.
  // Refer to 'was_return_keydown_filtered_by_platform_' in this case.
  if (IsTvProfile() && !strcmp(key_up->key, "Return") &&
      rwhv->was_return_keydown_filtered_by_platform_) {
    rwhv->was_return_keydown_filtered_by_platform_ = false;
    return;
  }

  if (rwhv->was_keydown_filtered_by_platform_) {
    rwhv->was_keydown_filtered_by_platform_ = false;
    return;
  }

  // When upper case letter is entered there are two events
  // Shift + letter key
  // Do not forward shift key to prevent shift keyup event.
  if (IsShiftKey(key_up->key))
    return;

  if (IsTvProfile()) {
    // For TV IME "Select" and "Cancel" key
    if (rwhv->im_context_ && rwhv->im_context_->IsVisible()) {
      if (!strcmp(key_up->key, "Cancel"))
        rwhv->im_context_->CancelComposition();
      if (!strcmp(key_up->key, "Select") || !strcmp(key_up->key, "Cancel"))
        rwhv->im_context_->HidePanel();
    }
    rwhv->ConvertEnterToSpaceIfNeeded(key_up);
  }

  NativeWebKeyboardEvent n_event = MakeWebKeyboardEvent(false, key_up);

  if (!rwhv->im_context_ ||
      rwhv->im_context_->ShouldHandleImmediately(key_up->key)) {
    rwhv->host_->ForwardKeyboardEvent(n_event);
    return;
  }

  rwhv->im_context_->PushToKeyUpEventQueue(n_event.windows_key_code);
}

void RenderWidgetHostViewEfl::OnMouseWheel(
    void* data, Evas* evas, Evas_Object* obj, void* event_info) {
  RenderWidgetHostViewEfl* rwhv = static_cast<RenderWidgetHostViewEfl*>(data);
  if (rwhv->evas_event_handler_.get())
    if (rwhv->evas_event_handler_->HandleEvent_MouseWheel(
            static_cast<Evas_Event_Mouse_Wheel*>(event_info)))
      return;

  if (!rwhv->touch_events_enabled_) {
    blink::WebMouseWheelEvent event = MakeWebMouseEvent(
        obj, static_cast<Evas_Event_Mouse_Wheel*>(event_info));
    rwhv->host_->ForwardWheelEvent(event);
  }
}

void RenderWidgetHostViewEfl::ProcessTouchEvents(unsigned int timestamp,
                                                 bool is_multi_touch) {
  // These constants are used to map multi touch's touch id(s).
  // The poorly-written Tizen API document says:
  //  "0 for Mouse Event and device id for Multi Event."
  //  "The point which comes from Mouse Event has id 0 and"
  //  "The point which comes from Multi Event has id that is same as Multi
  //  Event's device id."
  // This constant is to map touch id 0 to 0, or [0] -> [0]
  static const int kMultiTouchIDMapPart0SingleIndex = 0;
  // This constant is to map [13, 23] -> [1, 11]
  static const int kMultiTouchIDMapPart1StartIndex = 13;
  // This constant is to map [13, 23] -> [1, 11]
  static const int kMultiTouchIDMapPart1EndIndex = 23;
  // 13 - 1 = 12, 23 - 11 = 12
  static const int kMultiTouchIDMapPart1DiffValue = 12;

  unsigned count = evas_touch_point_list_count(evas_);
  if (!count) {
    return;
  }

  int id;
  Evas_Coord_Point pt;
  Evas_Touch_Point_State state;
  for (unsigned i = 0; i < count; ++i) {
    // evas_touch_point_list_nth_id_get returns [0] or [13, )
    // Multi touch's touch id [[0], [13, 23]] should be mapped to [[0], [1, 11]]
    // Internet Blame URL:
    //   https://groups.google.com/d/msg/mailing-enlightenment-devel/-R-ezCzpkTk/HJ0KBCdz6CgJ
    id = evas_touch_point_list_nth_id_get(evas_, i);
    DCHECK(id == kMultiTouchIDMapPart0SingleIndex ||
           id >= kMultiTouchIDMapPart1StartIndex);

    if (id >= kMultiTouchIDMapPart1StartIndex &&
        id <= kMultiTouchIDMapPart1EndIndex) {
      id -= kMultiTouchIDMapPart1DiffValue;
    } else if (id > kMultiTouchIDMapPart1EndIndex) {
      LOG(WARNING) << "evas_touch_point_list_nth_id_get() returned a value "
                      "greater than ("
                   << kMultiTouchIDMapPart1EndIndex << ").";
    }
    evas_touch_point_list_nth_xy_get(evas_, i, &pt.x, &pt.y);
    state = evas_touch_point_list_nth_state_get(evas_, i);

    if (count == 1) {
      if (state == EVAS_TOUCH_POINT_DOWN)
        touch_start_top_controls_offset_ = visible_top_controls_height_in_pix_;
      else if (state == EVAS_TOUCH_POINT_UP)
        ResizeIfNeededByTopControls();
    }

    pt.y -= touch_start_top_controls_offset_;
    pt.y -= GetVisibleBottomControlsHeightInPix();

    ui::TouchEvent touch_event =
        MakeTouchEvent(pt, state, id, content_image_, timestamp);

    if (state == EVAS_TOUCH_POINT_MOVE) {
      if ((id == 0 && is_multi_touch) || (id == 1 && !is_multi_touch))
        continue;

      if (event_resampler_->HandleTouchMoveEvent(touch_event))
        return;
    }

    HandleTouchEvent(&touch_event);
  }
}

void RenderWidgetHostViewEfl::SetDoubleTapSupportEnabled(bool enabled) {
  ui::GestureRecognizerImplEfl* gesture_recognizer_efl =
      static_cast<ui::GestureRecognizerImplEfl*>(gesture_recognizer_.get());
  DCHECK(gesture_recognizer_efl);
  ui::GestureProviderAura* gesture_provider_aura =
      gesture_recognizer_efl->GetGestureProviderForConsumer(this);
  gesture_provider_aura->filtered_gesture_provider_
      .SetDoubleTapSupportForPlatformEnabled(enabled);
}

void RenderWidgetHostViewEfl::SetTouchEventsEnabled(bool enabled) {
  if (enabled == touch_events_enabled_)
    return;

  touch_events_enabled_ = enabled;
  SetDoubleTapSupportEnabled(enabled);

  if (enabled) {
    selection_controller_.reset(new SelectionControllerEfl(this));
  } else {
    selection_controller_.reset();
  }
}

void RenderWidgetHostViewEfl::set_magnifier(bool status) {
  magnifier_ = status;
}

ui::LatencyInfo CreateLatencyInfo(const blink::WebInputEvent& event) {
  ui::LatencyInfo latency_info;
  // The latency number should only be added if the timestamp is valid.
  if (event.TimeStampSeconds()) {
    const int64_t time_micros = static_cast<int64_t>(
        event.TimeStampSeconds() * base::Time::kMicrosecondsPerSecond);
    latency_info.AddLatencyNumberWithTimestamp(
        ui::INPUT_EVENT_LATENCY_ORIGINAL_COMPONENT, 0, 0,
        base::TimeTicks() + base::TimeDelta::FromMicroseconds(time_micros), 1);
  }
  return latency_info;
}

void RenderWidgetHostViewEfl::SendGestureEvent(blink::WebGestureEvent& event) {
  HandleGesture(event);
  blink::WebInputEvent::Type event_type = event.GetType();
  if (magnifier_ && event_type == blink::WebInputEvent::kGestureScrollUpdate)
    return;

  // TODO: In this case, double tap should be filtered in
  // TouchDispositionGestureFilter. (See http://goo.gl/5G8PWJ)
  if (event.GetType() == blink::WebInputEvent::kGestureDoubleTap &&
      touchend_consumed_) {
    return;
  }

  if (event_resampler_->HandleGestureEvent(event))
    return;

  if (host_ && event_type != blink::WebInputEvent::kUndefined)
    host_->ForwardGestureEventWithLatencyInfo(event, CreateLatencyInfo(event));
}

void RenderWidgetHostViewEfl::HandleGestureBegin() {
  EnsureEdgeEffect().Enable();
}

void RenderWidgetHostViewEfl::HandleGestureEnd() {
  // Edge effect should be disabled upon scroll end/fling start.
  // Gesture end comes just after those events, so it's disabled here.
  EnsureEdgeEffect().Disable();
}

void RenderWidgetHostViewEfl::HandleGesture(blink::WebGestureEvent& event) {
  blink::WebInputEvent::Type event_type = event.GetType();
  // Do not send ScrollEnd event to selection controller If actual scrolling
  // isn't finished by top control animation.
  if (event_type == blink::WebInputEvent::kGestureScrollEnd &&
      TopControlsAnimationScheduled()) {
    defer_showing_selection_controls_ = true;
  }

  SelectionControllerEfl* controller = GetSelectionController();
  if (controller && !defer_showing_selection_controls_)
    controller->HandleGesture(event);

  RenderViewHost* render_view_host = web_contents_.GetRenderViewHost();
  if (!render_view_host)
    return;

  if (!render_view_host->GetWebkitPreferences().text_zoom_enabled &&
      (event_type == blink::WebInputEvent::kGesturePinchBegin ||
       event_type == blink::WebInputEvent::kGesturePinchEnd)) {
    return;
  }

  if (!render_view_host->GetWebkitPreferences().double_tap_to_zoom_enabled &&
      event_type == blink::WebInputEvent::kGestureDoubleTap) {
    return;
  }

  if (event_type == blink::WebInputEvent::kGestureDoubleTap ||
      event_type == blink::WebInputEvent::kGesturePinchBegin ||
      event_type == blink::WebInputEvent::kGesturePinchEnd) {
    WebContentsImpl* wci = static_cast<WebContentsImpl*>(&web_contents_);
    WebContentsViewEfl* wcve = static_cast<WebContentsViewEfl*>(wci->GetView());
    wcve->HandleZoomGesture(event);
  }

  if ((event_type == blink::WebInputEvent::kGestureTap ||
       event_type == blink::WebInputEvent::kGestureTapCancel) &&
      !handling_disambiguation_popup_gesture_) {
    float size = 32.0f;  // Default value

#if defined(OS_TIZEN)
    if (IsMobileProfile() || IsWearableProfile()) {
      size = elm_config_finger_size_get() / device_scale_factor_;
    }
#endif

    event.data.tap.width = size;
    event.data.tap.height = size;
  }

  if (event_type == blink::WebInputEvent::kGestureTapDown) {
    // Webkit does not stop a fling-scroll on tap-down. So explicitly send an
    // event to stop any in-progress flings.
    blink::WebGestureEvent fling_cancel = event;
    fling_cancel.SetType(blink::WebInputEvent::kGestureFlingCancel);
    fling_cancel.source_device = blink::kWebGestureDeviceTouchscreen;
    SendGestureEvent(fling_cancel);
  } else if (event_type == blink::WebInputEvent::kGestureScrollUpdate) {
    if (event.data.scroll_update.delta_x < 0)
      EnsureEdgeEffect().ShowOrHide(EdgeEffect::Direction::LEFT, false);
    else if (event.data.scroll_update.delta_x > 0)
      EnsureEdgeEffect().ShowOrHide(EdgeEffect::Direction::RIGHT, false);
    if (event.data.scroll_update.delta_y < 0)
      EnsureEdgeEffect().ShowOrHide(EdgeEffect::Direction::TOP, false);
    else if (event.data.scroll_update.delta_y > 0)
      EnsureEdgeEffect().ShowOrHide(EdgeEffect::Direction::BOTTOM, false);
  } else if (event_type == blink::WebInputEvent::kGesturePinchBegin) {
    EnsureEdgeEffect().Disable();
  } else if (event_type == blink::WebInputEvent::kGesturePinchEnd) {
    EnsureEdgeEffect().Enable();
  } else if (event_type == blink::WebInputEvent::kGestureFlingStart) {
    if (vertical_panning_hold_)
      event.data.fling_start.velocity_y = 0.0;
    if (horizontal_panning_hold_)
      event.data.fling_start.velocity_x = 0.0;
  }
}

void RenderWidgetHostViewEfl::HandleGesture(ui::GestureEvent* event) {
  blink::WebGestureEvent gesture = MakeWebGestureEventFromUIEvent(*event);
  gesture.x = event->x();
  gesture.y = event->y();

  const gfx::Point root_point = event->root_location();
  gesture.global_x = root_point.x();
  gesture.global_y = root_point.y();

  if (event->type() == ui::ET_GESTURE_BEGIN)
    HandleGestureBegin();
  else if (event->type() == ui::ET_GESTURE_END)
    HandleGestureEnd();

  SendGestureEvent(gesture);
  event->SetHandled();
}

// Based on render_widget_host_view_aura.cc::OnTouchEvent
void RenderWidgetHostViewEfl::HandleTouchEvent(ui::TouchEvent* event) {
  if (!gesture_recognizer_->ProcessTouchEventPreDispatch(event, this)) {
    event->StopPropagation();
    return;
  }

  // Update the touch event first.
  if (!pointer_state_.OnTouch(*event)) {
    event->StopPropagation();
    return;
  }

  blink::WebTouchEvent touch_event = ui::CreateWebTouchEventFromMotionEvent(
      pointer_state_, event->may_cause_scrolling());
  pointer_state_.CleanupRemovedTouchPoints(*event);

  event->StopPropagation();
  host_->ForwardTouchEventWithLatencyInfo(touch_event, *event->latency());
}

void RenderWidgetHostViewEfl::ProcessAckedTouchEvent(
    const TouchEventWithLatencyInfo& touch,
    InputEventAckState ack_result) {
  ui::EventResult result = (ack_result == INPUT_EVENT_ACK_STATE_CONSUMED)
                               ? ui::ER_HANDLED
                               : ui::ER_UNHANDLED;
  ui::GestureRecognizer::Gestures gestures = gesture_recognizer_->AckTouchEvent(
      touch.event.unique_touch_event_id, result, false,
      static_cast<GestureConsumer*>(this));
  if (touch.event.GetType() == blink::WebInputEvent::kTouchStart)
    touchstart_consumed_ = (result == ui::ER_HANDLED);

  if (touch.event.GetType() == blink::WebInputEvent::kTouchEnd)
    touchend_consumed_ = (result == ui::ER_HANDLED);

  // Handle Gestures for all the elements in the Gestures vector
  for (const auto& event : gestures)
    HandleGesture(event.get());
}

void RenderWidgetHostViewEfl::ForwardTouchEvent(ui::TouchEvent* event) {
  HandleTouchEvent(event);
}

void RenderWidgetHostViewEfl::ForwardGestureEvent(
    blink::WebGestureEvent event) {
  if (event.GetType() == blink::WebInputEvent::kGestureScrollUpdate) {
    if (vertical_panning_hold_)
      event.data.scroll_update.delta_y = 0;
    if (horizontal_panning_hold_)
      event.data.scroll_update.delta_x = 0;
  }

  if (host_ && event.GetType() != blink::WebInputEvent::kUndefined)
    host_->ForwardGestureEventWithLatencyInfo(event, CreateLatencyInfo(event));
}

unsigned RenderWidgetHostViewEfl::TouchPointCount() const {
  return PointerStatePointerCount();
}

#if defined(TIZEN_TBM_SUPPORT)
bool RenderWidgetHostViewEfl::UseEventResampler() {
  // If offscreen rendering is enabled, touch event will be fed by EWK API. So
  // |EventResampler| shouldn't handle gesture event.
  return !OffscreenRenderingEnabled();
}
#endif

EdgeEffect& RenderWidgetHostViewEfl::EnsureEdgeEffect() {
  if (!edge_effect_) {
    edge_effect_ = base::WrapUnique(new EdgeEffect(content_image_));
    edge_effect_->UpdatePosition(visible_top_controls_height_in_pix_,
                                 GetVisibleBottomControlsHeightInPix());
  }

  return *edge_effect_.get();
}

void RenderWidgetHostViewEfl::OnOrientationChangeEvent(int orientation) {
  current_orientation_ = orientation;
}

void RenderWidgetHostViewEfl::MoveCaret(const gfx::Point& point) {
  host_->delegate()->MoveCaret(gfx::Point(point.x() / device_scale_factor_,
                                          point.y() / device_scale_factor_));
}

void RenderWidgetHostViewEfl::SetComposition(
    const ui::CompositionText& composition_text) {
  const std::vector<ui::ImeTextSpan>& underlines =
      reinterpret_cast<const std::vector<ui::ImeTextSpan>&>(
          composition_text.ime_text_spans);

  // ECORE_IMF_CALLBACK_PREEDIT_CHANGED is came about H/W BackKey
  // in KOREAN ISF engine.
  // It removes selected text because '' is came as composition text.
  // Do not request ImeSetComposition to prevent delivering empty string
  // if text is selected status.
  if (composition_text.text.length() == 0 && GetSelectionController() &&
      GetSelectionController()->ExistsSelectedText())
    return;

  host_->ImeSetComposition(
      composition_text.text, underlines, gfx::Range::InvalidRange(),
      composition_text.selection.start(), composition_text.selection.end());
}

void RenderWidgetHostViewEfl::ConfirmComposition(base::string16& text) {
  if (text.length())
    host_->ImeCommitText(text, std::vector<ui::ImeTextSpan>(),
                         gfx::Range::InvalidRange(), 0);

  host_->ImeFinishComposingText(false);
}

void RenderWidgetHostViewEfl::DidCreateNewRendererCompositorFrameSink(
    viz::mojom::CompositorFrameSinkClient* renderer_compositor_frame_sink) {
  renderer_compositor_frame_sink_ = renderer_compositor_frame_sink;
}

void RenderWidgetHostViewEfl::SubmitCompositorFrame(
    const viz::LocalSurfaceId& local_surface_id,
    viz::CompositorFrame frame) {
  last_scroll_offset_ = frame.metadata.root_scroll_offset;
  if (GetSelectionController()) {
#if !defined(EWK_BRINGUP)
    GetSelectionController()->SetSelectionEditable(
        frame.metadata.selection.is_editable);
    GetSelectionController()->SetSelectionEmpty(
        frame.metadata.selection.is_empty_text_form_control);
#endif
    GetSelectionController()->OnSelectionChanged(frame.metadata.selection.start,
                                                 frame.metadata.selection.end);
  }
  if (frame.metadata.top_controls_shown_ratio != top_controls_shown_ratio_) {
    top_controls_shown_ratio_ = frame.metadata.top_controls_shown_ratio;
    TopControlsOffsetChanged();
  }

  frame_data_output_rect_width_ =
      frame.render_pass_list.back()->output_rect.width();

  SwapBrowserFrame(local_surface_id, std::move(frame));
}

void RenderWidgetHostViewEfl::ClearCompositorFrame() {}

void RenderWidgetHostViewEfl::ScrollFocusedEditableNode() {
  // If long-pressing, do not perform zoom to focused element.
  if (GetSelectionController() && !GetSelectionController()->GetLongPressed()) {
    // Call setEditableSelectionOffsets, setCompositionFromExistingText and
    // extendSelectionAndDelete on WebFrame instead of WebView.
    RenderFrameHostImpl* rfh = GetFocusedFrame();
    if (rfh) {
      rfh->GetFrameInputHandler()->ScrollFocusedEditableNodeIntoRect(
          gfx::Rect());
    }
  }
}

bool RenderWidgetHostViewEfl::HasSelectableText() {
  // If the last character of textarea is '\n', We can assume an extra '\n'.
  // Actually when you insert line break by ENTER key, '\n\n' is stored in
  // textarea. And If you press delete key, only a '\n' character will be stored
  // although there is no visible and selectable character in textarea. That's
  // why we should check whether selection_text contains only one line break.
  // Bug: http://suprem.sec.samsung.net/jira/browse/TSAM-2230
  //
  // Please see below commit for more information.
  // https://codereview.chromium.org/1785603002
  base::string16 selection_text = GetSelectedText();
  return !selection_text.empty() &&
         base::UTF16ToUTF8(selection_text).compare("\n") != 0;
}

bool RenderWidgetHostViewEfl::IsIMEShow() const {
  if (!im_context_)
    return false;

  // If webview isn't resized yet, return previous IME state.
  if (im_context_->WebViewWillBeResized())
    return !im_context_->IsVisible();

  return im_context_->IsVisible();
}

#if defined(OS_TIZEN_TV_PRODUCT)
void RenderWidgetHostViewEfl::ClearAllTilesResources() {
#if !defined(TIZEN_VD_DISABLE_GPU)
  if (evasgl_delegated_frame_host_)
    evasgl_delegated_frame_host_->ClearAllTilesResources();
#endif
}

void RenderWidgetHostViewEfl::SendKeyEvent(Evas_Object* ewk_view,
                                           void* event_info,
                                           bool is_press) {
  if (is_press) {
    RenderWidgetHostViewEfl::OnKeyDown(static_cast<void*>(this), nullptr,
                                       ewk_view, event_info);
  } else {
    RenderWidgetHostViewEfl::OnKeyUp(static_cast<void*>(this), nullptr,
                                     ewk_view, event_info);
  }
}

void RenderWidgetHostViewEfl::SetKeyEventsEnabled(bool enabled) {
  if (enabled) {
    evas_object_event_callback_add(content_image_, EVAS_CALLBACK_KEY_DOWN,
                                   OnKeyDown, this);
    evas_object_event_callback_add(content_image_, EVAS_CALLBACK_KEY_UP,
                                   OnKeyUp, this);
  } else {
    evas_object_event_callback_del(content_image_, EVAS_CALLBACK_KEY_DOWN,
                                   OnKeyDown);
    evas_object_event_callback_del(content_image_, EVAS_CALLBACK_KEY_UP,
                                   OnKeyUp);
  }
}

void RenderWidgetHostViewEfl::SetMouseEventCallbacks(
    const base::Callback<void(int, int)>& on_mouse_down,
    const base::Callback<bool(void)>& on_mouse_up,
    const base::Callback<void(void)>& on_mouse_move) {
  on_mouse_down_callback_ = on_mouse_down;
  on_mouse_up_callback_ = on_mouse_up;
  on_mouse_move_callback_ = on_mouse_move;
}
#endif
// Convert Enter to Space when focusing on a radio button or a checkbox if the
// input device is a remote controller (not keyboard)
template <typename EVT>
void RenderWidgetHostViewEfl::ConvertEnterToSpaceIfNeeded(EVT* evt) {
  if (evas_device_name_get(evt->dev) && !strcmp(evt->key, "Return") &&
      radio_or_checkbox_focused_) {
    const char* input_device_name = evas_device_name_get(evt->dev);
    if (!strstr(input_device_name, "keyboard") &&
        !strstr(input_device_name, "Keyboard") &&
        !strstr(input_device_name, "key board")) {
      evt->key = "space";
      evt->keycode = 65; /*space keycode*/
      evt->string = " ";
      LOG(INFO) << "Enter key is converted to space key for radio button"
                   " or checkbox input!";
    }
  }
}

void RenderWidgetHostViewEfl::UpdateSpatialNavigationStatus(bool enable) {
  auto rvh = web_contents_.GetRenderViewHost();
  if (!rvh)
    return;
  auto pref = rvh->GetWebkitPreferences();
  pref.spatial_navigation_enabled = enable;

  rvh->UpdateWebkitPreferences(pref);
}

#if defined(TIZEN_ATK_SUPPORT)
BrowserAccessibilityManager*
RenderWidgetHostViewEfl::CreateBrowserAccessibilityManager(
    BrowserAccessibilityDelegate* delegate,
    bool for_root_frame) {
  return BrowserAccessibilityManager::Create(
      BrowserAccessibilityManagerAuraLinux::GetEmptyDocument(), delegate);
}
#endif

void RenderWidgetHostViewEfl::SetIMERecommendedWords(
    const std::string& recommended_words) {
  if (IsTvProfile() && im_context_)
    im_context_->SetIMERecommendedWords(recommended_words);
}

void RenderWidgetHostViewEfl::SetIMERecommendedWordsType(bool should_enable) {
  if (IsTvProfile() && im_context_)
    im_context_->SetIMERecommendedWordsType(should_enable);
}

#if defined(TIZEN_VIDEO_HOLE)
void RenderWidgetHostViewEfl::SetWebViewMovedCallback(
    const base::Closure& on_webview_moved) {
  on_webview_moved_callback_ = on_webview_moved;
  LOG(INFO) << "Added position callback!";
}
#endif

#if defined(OS_TIZEN_TV_PRODUCT)
void RenderWidgetHostViewEfl::DrawLabel(Evas_Object* image,
                                        Eina_Rectangle rect) {
  // check view boundary
  if (rect.x < 0 || rect.y < 0 || rect.x > GetViewBoundsInPix().width() ||
      rect.y > GetViewBoundsInPix().height())
    return;

  evas_object_move(image, rect.x, rect.y);
  evas_object_show(image);

  voice_manager_labels_.push_back(image);
}

void RenderWidgetHostViewEfl::ClearLabels() {
  for (size_t i = 0; i < voice_manager_labels_.size(); i++) {
    evas_object_hide(voice_manager_labels_[i]);
    evas_object_del(voice_manager_labels_[i]);
  }
  voice_manager_labels_.clear();
}
#endif

void RenderWidgetHostViewEfl::SetGetCustomEmailViewportRectCallback(
    const base::Callback<const gfx::Rect&(void)>& callback) {
  get_custom_email_viewport_rect_callback_ = callback;
}

gfx::Rect RenderWidgetHostViewEfl::GetCustomEmailViewportRect() const {
  if (get_custom_email_viewport_rect_callback_.is_null())
    return gfx::Rect();
  return get_custom_email_viewport_rect_callback_.Run();
}

void RenderWidgetHostViewEfl::SetPageVisibility(bool visible) {
  if (visible)
    host_->WasShown(ui::LatencyInfo());
  else
    host_->WasHidden();
}

}  // namespace content
