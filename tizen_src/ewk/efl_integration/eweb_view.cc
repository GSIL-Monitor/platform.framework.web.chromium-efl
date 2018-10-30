// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "eweb_view.h"

#include "base/bind.h"
#include "base/command_line.h"
#include "base/files/file_util.h"
#include "base/json/json_string_value_serializer.h"
#include "base/logging.h"
#include "base/pickle.h"
#include "base/strings/utf_string_conversions.h"
#include "browser/input_picker/input_picker.h"
#include "browser/navigation_policy_handler_efl.h"
#include "browser/quota_permission_context_efl.h"
#include "browser/scoped_allow_wait_for_legacy_web_view_api.h"
#include "browser/web_view_browser_message_filter.h"
#include "common/content_client_efl.h"
#include "common/hit_test_params.h"
#include "common/render_messages_ewk.h"
#include "common/version_info.h"
#include "common/web_contents_utils.h"
#include "components/navigation_interception/navigation_params.h"
#include "components/sessions/content/content_serialized_navigation_builder.h"
#include "components/sessions/core/serialized_navigation_entry.h"
#include "content/browser/renderer_host/im_context_efl.h"
#include "content/browser/renderer_host/render_view_host_impl.h"
#include "content/browser/renderer_host/render_widget_host_view_efl.h"
#include "content/browser/renderer_host/ui_events_helper.h"
#include "content/browser/renderer_host/web_event_factory_efl.h"
#include "content/browser/web_contents/web_contents_impl_efl.h"
#include "content/browser/web_contents/web_contents_view.h"
#include "content/browser/web_contents/web_contents_view_efl.h"
#include "content/common/content_client_export.h"
#include "content/common/frame_messages.h"
#include "content/common/view_messages.h"
#include "content/public/browser/browser_message_filter.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/host_zoom_map.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/resource_dispatcher_host.h"
#include "content/public/common/browser_controls_state.h"
#include "content/public/common/content_client.h"
#include "content/public/common/mhtml_generation_params.h"
#include "content/public/common/user_agent.h"
#include "net/base/escape.h"
#include "net/base/filename_util.h"
#include "net/base/url_util.h"
#include "private/ewk_app_control_private.h"
#include "private/ewk_back_forward_list_private.h"
#include "private/ewk_context_private.h"
#include "private/ewk_frame_private.h"
#include "private/ewk_policy_decision_private.h"
#include "private/ewk_quota_permission_request_private.h"
#include "private/ewk_settings_private.h"
#include "private/webview_delegate_ewk.h"
#include "public/platform/WebString.h"
#include "skia/ext/platform_canvas.h"
#include "third_party/WebKit/public/web/WebContextMenuData.h"
#include "third_party/WebKit/public/web/WebFindOptions.h"
#include "tizen/system_info.h"
#include "ui/base/clipboard/clipboard_helper_efl.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/display/screen.h"
#include "ui/events/event_switches.h"
#include "ui/gfx/geometry/dip_util.h"
#include "web_contents_delegate_efl.h"
#include "web_contents_efl_delegate_ewk.h"
#include "web_contents_view_efl_delegate_ewk.h"

#include "browser/javascript_interface/gin_native_bridge_dispatcher_host.h"
#include "browser/web_view_evas_handler.h"

#include <Ecore_Evas.h>
#include <Elementary.h>
#include <Eina.h>

#include <iostream>

#if defined(OS_TIZEN_TV_PRODUCT)
#include "browser/mixed_content_observer.h"
#include "common/application_type.h"
#include "devtools_port_manager.h"
#include "ewk/efl_integration/public/ewk_media_parental_rating_info.h"
#include "ewk/efl_integration/public/ewk_media_subtitle_info_product.h"
#include "third_party/WebKit/public/platform/WebMediaPlayer.h"
#endif

#if defined(TIZEN_MULTIMEDIA_SUPPORT)
#include <app_control.h>
#include "media/base/mime_util.h"
#endif

#if defined(TIZEN_PEPPER_EXTENSIONS)
#include "efl/window_factory.h"
#include "ewk/efl_integration/ewk_privilege_checker.h"
#endif  // defined(TIZEN_PEPPER_EXTENSIONS)

#if defined(USE_WAYLAND) && defined(TIZEN_PEPPER_EXTENSIONS)
#include <Ecore_Wayland.h>
#endif  // defined(USE_WAYLAND)

#if defined(TIZEN_ATK_SUPPORT)
#include "content/browser/accessibility/browser_accessibility_state_impl.h"
#include "eweb_accessibility.h"
#include "eweb_accessibility_util.h"
#endif

using namespace content;
using web_contents_utils::WebViewFromWebContents;

namespace {

const float kDelayShowContextMenuTime = 0.2f;

const int kTitleLengthMax = 80;
const base::FilePath::CharType kMHTMLFileNameExtension[] =
    FILE_PATH_LITERAL(".mhtml");
const base::FilePath::CharType kMHTMLExtension[] = FILE_PATH_LITERAL("mhtml");
const base::FilePath::CharType kDefaultFileName[] =
    FILE_PATH_LITERAL("saved_page");
const char kReplaceChars[] = " ";
const char kReplaceWith[] = "_";

#if defined(OS_TIZEN_TV_PRODUCT)
static const int kMousePressAndHoldTimeout = 500;  // long press: 500ms
#endif

#if defined(TIZEN_MULTIMEDIA_SUPPORT)
const char kRtspScheme[] = "rtsp";
const char kMimeTypeVideo[] = "video/*";
const char kAppControlCookie[] = "cookie";
const char kAppControlProxy[] = "proxy";
#endif

static const char* kRendererCrashedHTMLMessage =
    "<html><body><h1>Renderer process has crashed!</h1></body></html>";

// "visible,content,changed" is an email-app specific signal which informs
// that the web view is only partially visible.
static const char* kVisibleContentChangedSignalName = "visible,content,changed";

// email-app specific signal which informs that custom scrolling is started
const char* kCustomScrollBeginSignalName = "custom,scroll,begin";

// email-app specific signal which informs that custom scrolling is finished
const char* kCustomScrollEndSignalName = "custom,scroll,end";

inline void SetDefaultStringIfNull(const char*& variable,
                                   const char* default_string) {
  if (!variable) {
    variable = default_string;
  }
}

/* LCOV_EXCL_START */
void GetEinaRectFromGfxRect(const gfx::Rect& gfx_rect,
                            Eina_Rectangle* eina_rect) {
  eina_rect->x = gfx_rect.x();
  eina_rect->y = gfx_rect.y();
  eina_rect->w = gfx_rect.width();
  eina_rect->h = gfx_rect.height();
}

#if defined(OS_TIZEN)
static Eina_Bool RotateWindowCb(void* data, int type, void* event) {
  auto wv = static_cast<EWebView*>(data);
  wv->SetOrientation(
      ecore_evas_rotation_get(ecore_evas_ecore_evas_get(wv->GetEvas())));
  return ECORE_CALLBACK_PASS_ON;
}
/* LCOV_EXCL_STOP */

static Eina_Bool AddDeviceCb(void* data, int type, void* event) {
  if (IsMobileProfile() || IsWearableProfile()) {
    auto view = static_cast<EWebView*>(data);
    auto info = static_cast<Ecore_Event_Device_Info*>(event);
    if (info->clas == ECORE_DEVICE_CLASS_KEYBOARD)
      view->HwKeyboardStatusChanged(std::string(info->name), EINA_TRUE);
  }

  return ECORE_CALLBACK_PASS_ON;
}

/* LCOV_EXCL_START */
static Eina_Bool RemoveDeviceCb(void* data, int type, void* event) {
  if (IsMobileProfile() || IsWearableProfile()) {
    auto view = static_cast<EWebView*>(data);
    auto info = static_cast<Ecore_Event_Device_Info*>(event);
    if (info->clas == ECORE_DEVICE_CLASS_KEYBOARD)
      view->HwKeyboardStatusChanged(std::string(info->name), EINA_FALSE);
  }

  return ECORE_CALLBACK_PASS_ON;
}
#endif
/* LCOV_EXCL_STOP */

static content::WebContents* NullCreateWebContents(void*) {
  return NULL;
}

/* LCOV_EXCL_START */
base::FilePath GenerateMHTMLFilePath(const GURL& url,
                                     const std::string& title,
                                     const std::string& base_path) {
  base::FilePath file_path(base_path);

  if (base::DirectoryExists(file_path)) {
    std::string title_part(title.substr(0, kTitleLengthMax));
    base::ReplaceChars(title_part, kReplaceChars, kReplaceWith, &title_part);
    base::FilePath file_name =
        net::GenerateFileName(url, std::string(), std::string(), title_part,
                              std::string(), kDefaultFileName);
    DCHECK(!file_name.empty());
    file_path = file_path.Append(file_name);
  }

  if (file_path.Extension().empty())
    file_path = file_path.AddExtension(kMHTMLExtension);
  else if (!file_path.MatchesExtension(kMHTMLFileNameExtension))
    file_path = file_path.ReplaceExtension(kMHTMLExtension);

  if (!base::PathExists(file_path))
    return file_path;

  int uniquifier =
      base::GetUniquePathNumber(file_path, base::FilePath::StringType());
  if (uniquifier > 0) {
    return file_path.InsertBeforeExtensionASCII(
        base::StringPrintf(" (%d)", uniquifier));
  }

  return base::FilePath();
}
/* LCOV_EXCL_STOP */

}  // namespace

class WebViewAsyncRequestHitTestDataCallback {
 public:
  WebViewAsyncRequestHitTestDataCallback(int x, int y, Ewk_Hit_Test_Mode mode)
      : x_(x), y_(y), mode_(mode) {}  // LCOV_EXCL_LINE
  virtual ~WebViewAsyncRequestHitTestDataCallback(){};

  virtual void Run(_Ewk_Hit_Test* hit_test, EWebView* web_view) = 0;

 protected:
  int GetX() const { return x_; }
  int GetY() const { return y_; }
  Ewk_Hit_Test_Mode GetMode() const { return mode_; }

 private:
  int x_;
  int y_;
  Ewk_Hit_Test_Mode mode_;
};

/* LCOV_EXCL_START */
class WebViewAsyncRequestHitTestDataUserCallback
    : public WebViewAsyncRequestHitTestDataCallback {
 public:
  WebViewAsyncRequestHitTestDataUserCallback(
      int x,
      int y,
      Ewk_Hit_Test_Mode mode,
      Ewk_View_Hit_Test_Request_Callback callback,
      void* user_data)
      : WebViewAsyncRequestHitTestDataCallback(x, y, mode),
        callback_(callback),
        user_data_(user_data) {}

  void Run(_Ewk_Hit_Test* hit_test, EWebView* web_view) override {
    DCHECK(callback_);
    callback_(web_view->evas_object(), GetX(), GetY(), GetMode(), hit_test,
              user_data_);
  }
  /* LCOV_EXCL_STOP */

 private:
  Ewk_View_Hit_Test_Request_Callback callback_;
  void* user_data_;
};

int EWebView::find_request_id_counter_ = 0;
content::WebContentsEflDelegate::WebContentsCreateCallback
    EWebView::create_new_window_web_contents_cb_ =
        base::Bind(&NullCreateWebContents);

EWebView* EWebView::FromEvasObject(Evas_Object* eo) {
  return WebViewDelegateEwk::GetInstance().GetWebViewFromEvasObject(eo);
}

RenderWidgetHostViewEfl* EWebView::rwhv() const {
  return static_cast<RenderWidgetHostViewEfl*>(
      web_contents_->GetRenderWidgetHostView());
}

/* LCOV_EXCL_START */
void EWebView::OnCustomScrollBeginCallback(void* user_data,
                                           Evas_Object* /*object*/,
                                           void* /*event_info*/) {
  auto view = static_cast<EWebView*>(user_data);
  view->GetSelectionController()->SetControlsTemporarilyHidden(true, true);
}

void EWebView::OnCustomScrollEndCallback(void* user_data,
                                         Evas_Object* /*object*/,
                                         void* /*event_info*/) {
  auto view = static_cast<EWebView*>(user_data);
  view->GetSelectionController()->SetControlsTemporarilyHidden(false, true);
}

void EWebView::OnViewFocusIn(void* data, Evas*, Evas_Object*, void*) {
  auto view = static_cast<EWebView*>(data);
  view->SetFocus(EINA_TRUE);
}

void EWebView::OnViewFocusOut(void* data, Evas*, Evas_Object*, void*) {
  auto view = static_cast<EWebView*>(data);
  view->SetFocus(EINA_FALSE);
}

void EWebView::CustomEmailViewportRectChangedCallback(void* user_data,
                                                      Evas_Object* /*object*/,
                                                      void* event_info) {
  auto view = static_cast<EWebView*>(user_data);
  auto rect = static_cast<Eina_Rectangle*>(event_info);
  view->custom_email_viewport_rect_ =
      gfx::Rect(rect->x, rect->y, rect->w, rect->h);
}
/* LCOV_EXCL_STOP */

EWebView::EWebView(Ewk_Context* context, Evas_Object* object)
    : context_(context),
      evas_object_(object),
      native_view_(object),
      touch_events_enabled_(false),
      mouse_events_enabled_(false),
      text_zoom_factor_(1.0),
      current_find_request_id_(find_request_id_counter_++),
      progress_(0.0),
      hit_test_completion_(base::WaitableEvent::ResetPolicy::AUTOMATIC,
                           base::WaitableEvent::InitialState::NOT_SIGNALED),
      page_scale_factor_(1.0),
      x_delta_(0.0),
      y_delta_(0.0),
#if defined(OS_TIZEN_TV_PRODUCT)
      is_marlin_enable_(false),
      is_processing_edge_scroll_(false),
      context_menu_show_(false),
      current_time_(0.0),
      is_high_bitrate_(false),
#endif
#if defined(TIZEN_ATK_SUPPORT)
      lazy_initalize_atk_(false),
#endif
#if defined(OS_TIZEN)
      window_rotate_handler_(nullptr),
#endif
      delayed_show_context_menu_timer_(nullptr),
      is_initialized_(false),
#if defined(OS_TIZEN_TV_PRODUCT)
      use_early_rwi_(false),
      rwi_info_showed_(false),
#endif
      add_device_handler_(nullptr),
      remove_device_handler_(nullptr) {
  if (evas_object_) {
    evas_object_smart_callback_add(
        evas_object_, kVisibleContentChangedSignalName,
        CustomEmailViewportRectChangedCallback, this);
    evas_object_smart_callback_add(evas_object_, kCustomScrollBeginSignalName,
                                   OnCustomScrollBeginCallback, this);
    evas_object_smart_callback_add(evas_object_, kCustomScrollEndSignalName,
                                   OnCustomScrollEndCallback, this);
    evas_object_event_callback_add(evas_object_, EVAS_CALLBACK_FOCUS_IN,
                                   OnViewFocusIn, this);
    evas_object_event_callback_add(evas_object_, EVAS_CALLBACK_FOCUS_OUT,
                                   OnViewFocusOut, this);
#if defined(OS_TIZEN)
#if TIZEN_VERSION_AT_LEAST(5, 0, 0)
    window_rotate_handler_ =
        ecore_event_handler_add(ECORE_WL2_EVENT_WINDOW_ROTATE,
                                RotateWindowCb, this);
#else
        ecore_event_handler_add(ECORE_WL_EVENT_WINDOW_ROTATE,
                                RotateWindowCb, this);
#endif  // TIZEN_VERSION_AT_LEAST(5, 0, 0)
    if (IsMobileProfile() || IsWearableProfile()) {
      add_device_handler_ =
          ecore_event_handler_add(ECORE_EVENT_DEVICE_ADD, AddDeviceCb, this);
      remove_device_handler_ =
          ecore_event_handler_add(ECORE_EVENT_DEVICE_DEL, RemoveDeviceCb, this);
    }
#endif
  }
}

void EWebView::Initialize() {
  if (is_initialized_) {
    return;
  }

  InitializeContent();

  evas_event_handler_ = new WebViewEvasEventHandler(this);

  scroll_detector_.reset(new ScrollDetector(this));

#if defined(TIZEN_PEPPER_EXTENSIONS)
  InitializePepperExtensionSystem();
#endif

  DCHECK(web_contents_->GetRenderViewHost());
  // Settings (content::WebPreferences) will be initalized by
  // RenderViewHostImpl::ComputeWebkitPrefs() based on command line switches.
  settings_.reset(new Ewk_Settings(
      evas_object_,
      web_contents_->GetRenderViewHost()->GetWebkitPreferences()));

#if defined(TIZEN_ATK_SUPPORT)
  eweb_accessibility_.reset(new EWebAccessibility(this));
  lazy_initalize_atk_ = true;
#endif

  base::CommandLine* cmdline = base::CommandLine::ForCurrentProcess();
  if (cmdline->HasSwitch(switches::kTouchEvents))
    SetTouchEventsEnabled(true);
  else
    SetMouseEventsEnabled(true);  // LCOV_EXCL_LINE

  web_contents_->SetUserAgentOverride(
      EflWebView::VersionInfo::GetInstance()->DefaultUserAgent());

  //allow this object and its children to get a focus
  elm_object_tree_focus_allow_set(native_view_, EINA_TRUE);
  is_initialized_ = true;
  evas_object_event_callback_add(native_view_, EVAS_CALLBACK_RESIZE,
                                 EWebView::NativeViewResize, this);

  auto cbce = static_cast<ContentBrowserClientEfl*>(
      content::GetContentClientExport()->browser());
  // Initialize accept languages
  SyncAcceptLanguages(cbce->GetAcceptLangs(nullptr));
  accept_langs_changed_callback_ =
      base::Bind(&EWebView::SyncAcceptLanguages, base::Unretained(this));
  cbce->AddAcceptLangsChangedCallback(accept_langs_changed_callback_);

  // If EWebView is created by window.open, RenderView is already created
  // before initializing WebContents. So we should manually invoke
  // EWebView::RenderViewCreated here.
  if (web_contents_->GetRenderViewHost() &&
      web_contents_->GetRenderViewHost()->IsRenderViewLive())
    RenderViewCreated(web_contents_->GetRenderViewHost());
}

EWebView::~EWebView() {
  auto cbce = static_cast<ContentBrowserClientEfl*>(
      content::GetContentClientExport()->browser());
  cbce->RemoveAcceptLangsChangedCallback(accept_langs_changed_callback_);

  evas_object_event_callback_del(native_view_, EVAS_CALLBACK_RESIZE,
                                 EWebView::NativeViewResize);
#if defined(TIZEN_PEPPER_EXTENSIONS)
  UnregisterPepperExtensionDelegate();
#endif
#if defined(OS_TIZEN_TV_PRODUCT)
  content::BrowserContext* browser_context = nullptr;
  content::StoragePartition* partition = nullptr;

  if (web_contents_) {
    browser_context = web_contents_->GetBrowserContext();
    partition = BrowserContext::GetDefaultStoragePartition(browser_context);
  }

  if (partition) {
    // Flush cookies if existed
    net::URLRequestContextGetter* context_getter =
        partition->GetURLRequestContext();
    if (context_getter) {
      net::URLRequestContext* url_request_context =
          context_getter->GetURLRequestContext();
      if (url_request_context) {
        net::CookieStore* cookie_store = url_request_context->cookie_store();
        if (cookie_store) {
          cookie_store->FlushStore(base::Closure());
        }
      }
    }

    // Flush local storage database
    partition->Flush();
  }
#endif

#if defined(USE_WAYLAND)
  if (!IsTvProfile() || GetSettings()->getClipboardEnabled())
    ClipboardHelperEfl::GetInstance()->MaybeInvalidateActiveWebview(this);
#endif

  std::map<int64_t, WebViewAsyncRequestHitTestDataCallback*>::iterator
      hit_test_callback_iterator;
  for (hit_test_callback_iterator = hit_test_callback_.begin();
       hit_test_callback_iterator != hit_test_callback_.end();
       hit_test_callback_iterator++)
    delete hit_test_callback_iterator->second;  // LCOV_EXCL_LINE
  hit_test_callback_.clear();

  for (auto iter = delayed_messages_.begin(); iter != delayed_messages_.end();
       ++iter)
    delete *iter;

  delayed_messages_.clear();

  if (!is_initialized_) {
    return;
  }

#if defined(TIZEN_ATK_SUPPORT)
  eweb_accessibility_.reset();
#endif
  select_picker_.reset();
  context_menu_.reset();
  mhtml_callback_map_.Clear();
  is_video_playing_callback_map_.Clear();

  // Release manually those scoped pointers to
  // make sure they are released in correct order
  web_contents_.reset();
  web_contents_delegate_.reset();

  // This code must be executed after WebContents deletion
  // because WebContents depends on BrowserContext which
  // is deleted along with EwkContext.
  CHECK(!web_contents_);

  permission_popup_manager_.reset();

  gin_native_bridge_dispatcher_host_.reset();

  if (evas_object_) {
    evas_object_smart_callback_del(evas_object_,
                                   kVisibleContentChangedSignalName,
                                   CustomEmailViewportRectChangedCallback);
    evas_object_smart_callback_del(evas_object_, kCustomScrollBeginSignalName,
                                   OnCustomScrollBeginCallback);

    evas_object_smart_callback_del(evas_object_, kCustomScrollEndSignalName,
                                   OnCustomScrollEndCallback);
    evas_object_event_callback_del(evas_object_, EVAS_CALLBACK_FOCUS_IN,
                                   OnViewFocusIn);
    evas_object_event_callback_del(evas_object_, EVAS_CALLBACK_FOCUS_OUT,
                                   OnViewFocusOut);
    if (IsMobileProfile() || IsWearableProfile()) {
      if (add_device_handler_)
        ecore_event_handler_del(add_device_handler_);
      if (remove_device_handler_)
        ecore_event_handler_del(remove_device_handler_);
    }
  }
#if defined(OS_TIZEN)
  if (window_rotate_handler_)
    ecore_event_handler_del(window_rotate_handler_);
#endif
}

/* LCOV_EXCL_START */
void EWebView::NativeViewResize(void* data,
                                Evas* e,
                                Evas_Object* obj,
                                void* event_info) {
  auto thiz = static_cast<EWebView*>(data);
  if (!thiz->context_menu_)
    return;
  int native_view_x, native_view_y, native_view_width, native_view_height;
  evas_object_geometry_get(obj, &native_view_x, &native_view_y,
                           &native_view_width, &native_view_height);
  gfx::Rect webview_rect(native_view_x, native_view_y, native_view_width,
                         native_view_height);

  thiz->context_menu_->Resize(webview_rect);
}

void EWebView::ResetContextMenuController() {
  return context_menu_.reset();
}

#if defined(TIZEN_MULTIMEDIA_SUPPORT)
void EWebView::OpenMediaPlayer(const std::string& url,
                               const std::string& cookie) {
  app_control_h handle = nullptr;
  int ret;

  ret = app_control_create(&handle);
  if (ret != APP_CONTROL_ERROR_NONE || handle == nullptr) {
    LOG(ERROR) << "Fail to create app control. ERR: " << ret;
    return;
  }

  std::unique_ptr<app_control_h, void (*)(app_control_h * handle)> handle_ptr(
      &handle, [](app_control_h* handle) {
        ignore_result(app_control_destroy(*handle));
      });

  ret = app_control_set_operation(handle, APP_CONTROL_OPERATION_VIEW);
  if (ret != APP_CONTROL_ERROR_NONE) {
    LOG(ERROR) << "Fail to set operation. ERR: " << ret;
    return;
  }

  ret = app_control_set_uri(handle, url.c_str());
  if (ret != APP_CONTROL_ERROR_NONE) {
    LOG(ERROR) << "Fail to set URI. ERR: " << ret;
    return;
  }

  ret = app_control_set_mime(handle, kMimeTypeVideo);
  if (ret != APP_CONTROL_ERROR_NONE) {
    LOG(ERROR) << "Fail to set mime type. ERR: " << ret;
    return;
  }

  if (!cookie.empty()) {
    ret = app_control_add_extra_data(handle, kAppControlCookie, cookie.c_str());
    if (ret != APP_CONTROL_ERROR_NONE) {
      LOG(ERROR) << "Fail to add cookie. ERR: " << ret;
      return;
    }
  }

  const char* proxy = context_->GetImpl()->GetProxyUri();
  if (proxy && strlen(proxy)) {
    ret = app_control_add_extra_data(handle, kAppControlProxy, proxy);
    if (ret != APP_CONTROL_ERROR_NONE) {
      LOG(ERROR) << "Fail to add proxy. ERR: " << ret;
      return;
    }
  }

  ret = app_control_send_launch_request(handle, nullptr, nullptr);
  if (ret != APP_CONTROL_ERROR_NONE) {
    LOG(ERROR) << "Fail to send launch request. ERR: " << ret;
    return;
  }
}
#endif

void EWebView::SetFocus(Eina_Bool focus) {
  if (!web_contents_ || !rwhv() || HasFocus() == focus)
    return;

  if (focus)
    rwhv()->Focus();
  else
    rwhv()->Unfocus();
}
/* LCOV_EXCL_STOP */

Eina_Bool EWebView::AddJavaScriptMessageHandler(
    Evas_Object* view,
    Ewk_View_Script_Message_Cb callback,
    std::string name) {
  if (!gin_native_bridge_dispatcher_host_)
    return EINA_FALSE;

  return gin_native_bridge_dispatcher_host_->AddNamedObject(view, callback,
                                                            name);
}

bool EWebView::SetPageVisibility(
    Ewk_Page_Visibility_State page_visibility_state) {
  if (!rwhv())
    return false;

  // TODO: We should able to set 'prerender' or 'unloaded' as visibility state.
  // http://www.w3.org/TR/page-visibility/#dom-document-visibilitystate
  switch (page_visibility_state) {
    case EWK_PAGE_VISIBILITY_STATE_VISIBLE:
      rwhv()->SetPageVisibility(true);
      break;
    case EWK_PAGE_VISIBILITY_STATE_HIDDEN:
      rwhv()->SetPageVisibility(false);
      break;
    default:
      return false;
  }

  return true;
}

/* LCOV_EXCL_START */
void EWebView::CreateNewWindow(
    content::WebContentsEflDelegate::WebContentsCreateCallback cb) {
  create_new_window_web_contents_cb_ = cb;
  Evas_Object* new_object = NULL;
  SmartCallback<EWebViewCallbacks::CreateNewWindow>().call(&new_object);
  create_new_window_web_contents_cb_ = base::Bind(&NullCreateWebContents);
  DCHECK(new_object);
}

// static
Evas_Object* EWebView::GetHostWindowDelegate(const content::WebContents* wc) {
  EWebView* thiz = WebViewFromWebContents(wc);
  DCHECK(thiz->evas_object_);
  Evas_Object* parent = evas_object_above_get(thiz->evas_object_);
  if (!parent) {
    LOG(WARNING) << "Could not find and visual parents for EWK smart object!.";
    return thiz->evas_object_;
  }

  if (elm_object_widget_check(parent)) {
    Evas_Object* elm_parent = elm_object_top_widget_get(parent);
    if (elm_parent)
      return elm_parent;
    return parent;
  }

  LOG(WARNING) << "Could not find elementary parent for WebView object!";
  return thiz->evas_object_;
}
/* LCOV_EXCL_STOP */

Evas_Object* EWebView::GetElmWindow() const {
  Evas_Object* parent = elm_object_parent_widget_get(evas_object_);
  return parent ? elm_object_top_widget_get(parent) : nullptr;
}

void EWebView::SetURL(const GURL& url) {
  NavigationController::LoadURLParams params(url);
#if defined(OS_TIZEN_TV_PRODUCT)
  if (use_early_rwi_) {
    LOG(INFO) << "[FAST RWI] SetURL Replace [" << url.spec()
              << "] to [about:blank]";
    rwi_gurl_ = url;
    params.url = GURL("about:blank");
  }
#endif
  params.override_user_agent = NavigationController::UA_OVERRIDE_TRUE;
  web_contents_->GetController().LoadURLWithParams(params);
#if defined(OS_TIZEN_TV_PRODUCT)
  web_contents_->SetUserInputURLNeedNotify(true);
#endif
}

/* LCOV_EXCL_START */
const GURL& EWebView::GetURL() const {
  return web_contents_->GetVisibleURL();
}

const GURL& EWebView::GetOriginalURL() const {
  const auto entry = web_contents_->GetController().GetVisibleEntry();
  if (entry)
    return entry->GetOriginalRequestURL();

  return web_contents_->GetVisibleURL();
}

void EWebView::SetTileCoverAreaMultiplier(float cover_area_multiplier) {
  if (rwhv())
    rwhv()->SetTileCoverAreaMultiplier(cover_area_multiplier);
}

void EWebView::Reload() {
  web_contents_->GetController().Reload(content::ReloadType::NORMAL, true);
}

void EWebView::ReloadBypassingCache() {
  web_contents_->GetController().Reload(content::ReloadType::BYPASSING_CACHE, true);
}

Eina_Bool EWebView::CanGoBack() {
  return web_contents_->GetController().CanGoBack();
}

Eina_Bool EWebView::CanGoForward() {
  return web_contents_->GetController().CanGoForward();
}

Eina_Bool EWebView::HasFocus() const {
  if (!rwhv())
    return EINA_FALSE;

  return rwhv()->HasFocus() ? EINA_TRUE : EINA_FALSE;
}

Eina_Bool EWebView::GoBack() {
  if (!web_contents_->GetController().CanGoBack())
    return EINA_FALSE;

  web_contents_->GetController().GoBack();
  return EINA_TRUE;
}

void EWebView::SetClearTilesOnHide(bool enable) {
  if (rwhv())
    rwhv()->SetClearTilesOnHide(enable);
}

Eina_Bool EWebView::GoForward() {
  if (!web_contents_->GetController().CanGoForward())
    return EINA_FALSE;

  web_contents_->GetController().GoForward();
  return EINA_TRUE;
}

void EWebView::Stop() {
  if (web_contents_->IsLoading()) {
    web_contents_->Stop();

#if defined(OS_TIZEN_TV_PRODUCT)
    web_contents_->SetUserInputURLNeedNotify(false);
#endif
  }
}

void EWebView::Suspend() {
  CHECK(web_contents_);
  RenderFrameHost* rfh = web_contents_->GetMainFrame();
  CHECK(rfh);

  if (rwhv() && rwhv()->GetIMContext())
    rwhv()->GetIMContext()->OnSuspend();

  RenderViewHost* rvh = web_contents_->GetRenderViewHost();
  if (rvh)
    rvh->Send(new EwkViewMsg_SuspendScheduledTask(rvh->GetRoutingID()));
}

void EWebView::Resume() {
  CHECK(web_contents_);
  RenderFrameHost* rfh = web_contents_->GetMainFrame();
  CHECK(rfh);

  RenderViewHost* rvh = web_contents_->GetRenderViewHost();
  if (rvh)
    rvh->Send(new EwkViewMsg_ResumeScheduledTasks(rvh->GetRoutingID()));

  if (rwhv() && rwhv()->GetIMContext())
    rwhv()->GetIMContext()->OnResume();
}

#if defined(OS_TIZEN_TV_PRODUCT)
void EWebView::SuspendNetworkLoading() {
  CHECK(web_contents_);
  RenderViewHost* render_view_host = web_contents_->GetRenderViewHost();
  if (render_view_host)
    render_view_host->Send(
        new EwkViewMsg_SuspendNetworkLoading(render_view_host->GetRoutingID()));
}

void EWebView::ResumeNetworkLoading() {
  CHECK(web_contents_);
  RenderViewHost* render_view_host = web_contents_->GetRenderViewHost();
  if (render_view_host)
    render_view_host->Send(
        new EwkViewMsg_ResumeNetworkLoading(render_view_host->GetRoutingID()));
}
#endif

double EWebView::GetTextZoomFactor() const {
  if (text_zoom_factor_ < 0.0)
    return -1.0;

  return text_zoom_factor_;
}

void EWebView::SetTextZoomFactor(double text_zoom_factor) {
  if (text_zoom_factor_ == text_zoom_factor || text_zoom_factor < 0.0)
    return;

  text_zoom_factor_ = text_zoom_factor;
  RenderViewHost* render_view_host = web_contents_->GetRenderViewHost();
  if (!render_view_host)
    return;

  render_view_host->Send(new ViewMsg_SetTextZoomFactor(
      render_view_host->GetRoutingID(), text_zoom_factor));
}

double EWebView::GetPageZoomFactor() const {
  if (!web_contents_) {
    LOG(ERROR) << "GetPageZoomFactor: web content is null";
    return -1.0;
  }

  const HostZoomMap* host_zoom_map =
      content::HostZoomMap::GetForWebContents(web_contents_.get());
  if (!host_zoom_map) {
    LOG(ERROR) << "GetPageZoomFactor: host_zoom_map is null";
    return -1.0;
  }

  const NavigationEntry* entry =
      web_contents_->GetController().GetActiveEntry();
  if (!entry)
    return -1.0;

  GURL url = host_zoom_map->GetURLFromEntry(entry);
  return content::ZoomLevelToZoomFactor(
      host_zoom_map->GetZoomLevelForHostAndScheme(
          url.scheme(), net::GetHostOrSpecFromURL(url)));
}

bool EWebView::SetPageZoomFactor(double page_zoom_factor) {
  if (!web_contents_)
    return false;

  HostZoomMap* host_zoom_map =
      content::HostZoomMap::GetForWebContents(web_contents_.get());
  if (!host_zoom_map)
    return false;

  const NavigationEntry* entry =
      web_contents_->GetController().GetActiveEntry();
  if (!entry)
    return false;

  GURL url = host_zoom_map->GetURLFromEntry(entry);
  LOG(INFO) << "SetPageZoomFactor, url = " << url.possibly_invalid_spec();

  host_zoom_map->SetZoomLevelForHost(
      net::GetHostOrSpecFromURL(url),
      content::ZoomFactorToZoomLevel(page_zoom_factor));
  return true;
}

void EWebView::ExecuteEditCommand(const char* command, const char* value) {
  EINA_SAFETY_ON_NULL_RETURN(command);

  base::Optional<base::string16> optional_value;
  if(value == nullptr)
    optional_value = base::nullopt;
  else
    optional_value = base::make_optional(base::ASCIIToUTF16(value));

  auto rfh = static_cast<content::RenderFrameHostImpl*>(
      web_contents().GetFocusedFrame());
  if (rfh)
    rfh->GetFrameInputHandler()->ExecuteEditCommand(std::string(command),
                                                    optional_value);
}
/* LCOV_EXCL_STOP */

#if defined(OS_TIZEN)
void EWebView::EnterDragState() {
  if (IsMobileProfile()) {
    if (RenderViewHost* render_view_host = web_contents_->GetRenderViewHost()) {
      render_view_host->Send(
          new ViewMsg_DragStateEnter(render_view_host->GetRoutingID()));
    }
  }
}
#endif

void EWebView::SetOrientation(int orientation) {

  if (GetOrientation() == orientation)
    return;

  if (orientation == 0   ||
      orientation == 90  ||
      orientation == 180 ||
      orientation == 270) {
    GetWebContentsViewEfl()->SetOrientation(orientation);

    int width = 0;
    int height = 0;
    const Ecore_Evas* ee = ecore_evas_ecore_evas_get(GetEvas());
    ecore_evas_screen_geometry_get(ee, nullptr, nullptr, &width, &height);
    if (orientation == 90 || orientation == 270)
      std::swap(width, height);

    if (popup_controller_)
      popup_controller_->SetPopupSize(width, height);  // LCOV_EXCL_LINE
    if (JavaScriptDialogManagerEfl* dialogMG = GetJavaScriptDialogManagerEfl())
      dialogMG->SetPopupSize(width, height);  // LCOV_EXCL_LINE
  }
}

int EWebView::GetOrientation() {
  return GetWebContentsViewEfl()->GetOrientation();
}

/* LCOV_EXCL_START */
void EWebView::Show() {
  evas_object_show(native_view_);
  web_contents_->WasShown();
}

void EWebView::Hide() {
  evas_object_hide(native_view_);
  web_contents_->WasHidden();
}

void EWebView::SetViewAuthCallback(Ewk_View_Authentication_Callback callback,
                                   void* user_data) {
  authentication_cb_.Set(callback, user_data);
}

void EWebView::InvokeAuthCallback(LoginDelegateEfl* login_delegate,
                                  const GURL& url,
                                  const std::string& realm) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));

  auth_challenge_.reset(new _Ewk_Auth_Challenge(login_delegate, url, realm));
  authentication_cb_.Run(evas_object_, auth_challenge_.get());

  if (!auth_challenge_->is_decided &&
      !auth_challenge_->is_suspended) {
    auth_challenge_->is_decided = true;
    auth_challenge_->login_delegate->Cancel();
  }
}
/* LCOV_EXCL_STOP */

void EWebView::InvokePolicyResponseCallback(
    _Ewk_Policy_Decision* policy_decision) {
  SmartCallback<EWebViewCallbacks::PolicyResponseDecide>().call(
      policy_decision);

#if defined(TIZEN_MULTIMEDIA_SUPPORT)
  if (IsMobileProfile() &&
      GetSettings()
          ->getPreferences()
          .tizen_version.TizenCompatibilityModeEnabled() &&
      media::IsSupportedMediaMimeType(policy_decision->GetResponseMime())) {
    OpenMediaPlayer(policy_decision->GetUrl(), policy_decision->GetCookie());
    policy_decision->Ignore();
  }
#endif

  if (policy_decision->isSuspended())
    return;

  if (!policy_decision->isDecided())
    policy_decision->Use();

  delete policy_decision;
}

void EWebView::InvokePolicyNavigationCallback(
    const NavigationPolicyParams params,
    bool* handled) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));

  SmartCallback<EWebViewCallbacks::SaveSessionData>().call();

  std::unique_ptr<_Ewk_Policy_Decision> policy_decision(
      new _Ewk_Policy_Decision(params));

#if defined(TIZEN_MULTIMEDIA_SUPPORT)
  std::string file_name = params.url.ExtractFileName();
  bool is_mpd_file =
      base::EndsWith(file_name, "mpd", base::CompareCase::INSENSITIVE_ASCII);
  if ((params.url.SchemeIs(kRtspScheme) || is_mpd_file)) {
    OpenMediaPlayer(params.url.possibly_invalid_spec(), params.cookie);
    *handled = true;
    policy_decision->Ignore();
    return;
  }
#endif

  SmartCallback<EWebViewCallbacks::NavigationPolicyDecision>().call(
      policy_decision.get());

  CHECK(!policy_decision->isSuspended());

  // TODO: Navigation can't be suspended
  // this aproach is synchronous and requires immediate response
  // Maybe there is different approach (like resource throttle response
  // mechanism) that allows us to
  // suspend navigation
  if (!policy_decision->isDecided())
    policy_decision->Use();

  *handled = policy_decision->GetNavigationPolicyHandler()->GetDecision() ==
             NavigationPolicyHandlerEfl::Handled;
}

/* LCOV_EXCL_START */
void EWebView::HandleTouchEvents(Ewk_Touch_Event_Type type,
                                 const Eina_List* points,
                                 const Evas_Modifier* modifiers) {
  const Eina_List* l;
  void* data;
  EINA_LIST_FOREACH(points, l, data) {
    const Ewk_Touch_Point* point = static_cast<Ewk_Touch_Point*>(data);
    if (point->state == EVAS_TOUCH_POINT_STILL) {
      // Chromium doesn't expect (and doesn't like) these events.
      continue;
    }
    if (rwhv()) {
      Evas_Coord_Point pt;
      pt.x = point->x;
      pt.y = point->y;
      ui::TouchEvent touch_event =
          MakeTouchEvent(pt, point->state, point->id, evas_object());
      rwhv()->HandleTouchEvent(&touch_event);
    }
  }
}

content::WebContentsViewEfl* EWebView::GetWebContentsViewEfl() const {
  WebContentsImpl* wc = static_cast<WebContentsImpl*>(web_contents_.get());
  return static_cast<WebContentsViewEfl*>(wc->GetView());
}

bool EWebView::TouchEventsEnabled() const {
  return touch_events_enabled_;
}
/* LCOV_EXCL_STOP */
// TODO: Touch events use the same mouse events in EFL API.
// Figure out how to distinguish touch and mouse events on touch&mice devices.
// Currently mouse and touch support is mutually exclusive.
void EWebView::SetTouchEventsEnabled(bool enabled) {
  if (touch_events_enabled_ == enabled)
    return;

  touch_events_enabled_ = enabled;
  GetWebContentsViewEfl()->SetTouchEventsEnabled(enabled);

  GetSettings()->getPreferences().touch_event_feature_detection_enabled =
      enabled;
  GetSettings()->getPreferences().double_tap_to_zoom_enabled = enabled;
  GetSettings()->getPreferences().editing_behavior =
      enabled ? content::EDITING_BEHAVIOR_ANDROID
              : content::EDITING_BEHAVIOR_UNIX;
  UpdateWebKitPreferences();
}

/* LCOV_EXCL_START */
bool EWebView::MouseEventsEnabled() const {
  return mouse_events_enabled_;
}

void EWebView::SetMouseEventsEnabled(bool enabled) {
  if (mouse_events_enabled_ == enabled)
    return;

  mouse_events_enabled_ = enabled;
  GetWebContentsViewEfl()->SetTouchEventsEnabled(!enabled);
}

namespace {

class JavaScriptCallbackDetails {
 public:
  JavaScriptCallbackDetails(Ewk_View_Script_Execute_Callback callback_func,
                            void* user_data,
                            Evas_Object* view)
      : callback_func_(callback_func), user_data_(user_data), view_(view) {}

  Ewk_View_Script_Execute_Callback callback_func_;
  void* user_data_;
  Evas_Object* view_;
};

void JavaScriptComplete(JavaScriptCallbackDetails* script_callback_data,
                        const base::Value* result) {
  if (!script_callback_data->callback_func_)
    return;

  std::string return_string;
  const char* callback_string;
  if (result->IsType(base::Value::Type::STRING)) {
    // We don't want to serialize strings with JSONStringValueSerializer
    // to avoid quotation marks.
    result->GetAsString(&return_string);
    callback_string = return_string.c_str();
  } else if (result->IsType(base::Value::Type::NONE)) {
    // Value::TYPE_NULL is for lack of value, undefined, null
    callback_string = nullptr;
  } else {
    JSONStringValueSerializer serializer(&return_string);
    serializer.Serialize(*result);
    callback_string = return_string.c_str();
  }

  script_callback_data->callback_func_(script_callback_data->view_,
                                       callback_string,
                                       script_callback_data->user_data_);
}

}  // namespace

bool EWebView::ExecuteJavaScript(const char* script,
                                 Ewk_View_Script_Execute_Callback callback,
                                 void* userdata) {
  if (!script)
    return false;

  if (!web_contents_delegate_)  // question, can I remove this check?
    return false;

  if (!web_contents_)
    return false;

  RenderFrameHost* render_frame_host = web_contents_->GetMainFrame();
  if (!render_frame_host)
    return false;

  // Note: M37. Execute JavaScript, |script| with
  // |RenderFrameHost::ExecuteJavaScript|.
  // @see also https://codereview.chromium.org/188893005 for more details.
  base::string16 js_script;
  base::UTF8ToUTF16(script, strlen(script), &js_script);
  if (callback) {
    JavaScriptCallbackDetails* script_callback_data =
        new JavaScriptCallbackDetails(callback, userdata, evas_object_);
    // In M47, it isn't possible anymore to execute javascript in the generic
    // case. We need to call ExecuteJavaScriptForTests to keep the behaviour
    // unchanged @see https://codereview.chromium.org/1123783002
    render_frame_host->ExecuteJavaScriptWithUserGestureForTests(
        js_script,
        base::Bind(&JavaScriptComplete, base::Owned(script_callback_data)));
  } else {
    // We use ExecuteJavaScriptWithUserGestureForTests instead of
    // ExecuteJavaScript because
    // ExecuteJavaScriptWithUserGestureForTests sets user_gesture to true. This
    // was the
    // behaviour is m34, and we want to keep it that way.
    render_frame_host->ExecuteJavaScriptWithUserGestureForTests(js_script);
  }

  return true;
}
/* LCOV_EXCL_STOP */

bool EWebView::SetUserAgent(const char* userAgent) {
  const content::NavigationController& controller =
      web_contents_->GetController();
  bool override = userAgent && strlen(userAgent);
  for (int i = 0; i < controller.GetEntryCount(); ++i)
    controller.GetEntryAtIndex(i)->SetIsOverridingUserAgent(override); // LCOV_EXCL_LINE

  if (override)
    web_contents_->SetUserAgentOverride(userAgent);
  else
    web_contents_->SetUserAgentOverride(std::string());

  return true;
}

/* LCOV_EXCL_START */
bool EWebView::SetUserAgentAppName(const char* application_name) {
  EflWebView::VersionInfo::GetInstance()->
      UpdateUserAgentWithAppName(application_name ? application_name : "");
  web_contents_->SetUserAgentOverride(
      EflWebView::VersionInfo::GetInstance()->DefaultUserAgent());
  return true;
}

void EWebView::set_magnifier(bool status) {
  if (rwhv())
    rwhv()->set_magnifier(status);
}

#if defined(OS_TIZEN)
bool EWebView::SetPrivateBrowsing(bool incognito) {
  if (context_->GetImpl()->browser_context()->IsOffTheRecord() == incognito)
    return false;

  context_->GetImpl()->browser_context()->SetOffTheRecord(incognito);
  return true;
}

bool EWebView::GetPrivateBrowsing() const {
  return context_->GetImpl()->browser_context()->IsOffTheRecord();
}
#endif
/* LCOV_EXCL_STOP */

const char* EWebView::GetUserAgent() const {
  if (!web_contents_->GetUserAgentOverride().empty())
    user_agent_ = web_contents_->GetUserAgentOverride();
  else
    user_agent_ = content::GetContentClientExport()->GetUserAgent();
  return user_agent_.c_str();
}

/* LCOV_EXCL_START */
const char* EWebView::GetUserAgentAppName() const {
  user_agent_app_name_ = EflWebView::VersionInfo::GetInstance()->AppName();
  return user_agent_app_name_.c_str();
}

const char* EWebView::CacheSelectedText() {
  if (!rwhv())
    return "";

  selected_text_cached_ = base::UTF16ToUTF8(rwhv()->GetSelectedText());
  return selected_text_cached_.c_str();
}

_Ewk_Frame* EWebView::GetMainFrame() {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));

  if (!frame_.get()) {
    RenderViewHost* render_view_host = web_contents_->GetRenderViewHost();

    if (render_view_host)
      frame_.reset(new _Ewk_Frame(render_view_host->GetMainFrame()));
  }

  return frame_.get();
}
/* LCOV_EXCL_STOP */

void EWebView::UpdateWebKitPreferences() {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));

  RenderViewHost* render_view_host = web_contents_->GetRenderViewHost();
  if (!render_view_host)
    return;

  web_contents_delegate_->OnUpdateSettings(settings_.get());
  render_view_host->UpdateWebkitPreferences(settings_->getPreferences());

  UpdateWebkitPreferencesEfl(render_view_host);
}

void EWebView::UpdateWebkitPreferencesEfl(RenderViewHost* render_view_host) {
  DCHECK(render_view_host);

  IPC::Message* message = new EwkSettingsMsg_UpdateWebKitPreferencesEfl(
      render_view_host->GetRoutingID(), settings_->getPreferencesEfl());

  if (render_view_host->IsRenderViewLive()) {
    render_view_host->Send(message);
  } else {
    delayed_messages_.push_back(message);
  }
}

/* LCOV_EXCL_START */
void EWebView::SetContentSecurityPolicy(const char* policy,
                                        Ewk_CSP_Header_Type type) {
  web_contents_delegate_->SetContentSecurityPolicy(
      (policy ? policy : std::string()), type);
}

void EWebView::LoadHTMLString(const char* html,
                              const char* base_uri,
                              const char* unreachable_uri) {
  LoadData(html, std::string::npos, NULL, NULL, base_uri, unreachable_uri);
}

void EWebView::LoadPlainTextString(const char* plain_text) {
  LoadData(plain_text, std::string::npos, "text/plain", NULL, NULL, NULL);
}

void EWebView::LoadHTMLStringOverridingCurrentEntry(
    const char* html,
    const char* base_uri,
    const char* unreachable_url) {
  LoadData(html, std::string::npos, NULL, NULL, base_uri, unreachable_url,
           true);
}
/* LCOV_EXCL_STOP */

void EWebView::LoadData(const char* data,
                        size_t size,
                        const char* mime_type,
                        const char* encoding,
                        const char* base_uri,
                        const char* unreachable_uri,
                        bool should_replace_current_entry) {
  SetDefaultStringIfNull(mime_type, "text/html");
  SetDefaultStringIfNull(encoding, "utf-8");
  SetDefaultStringIfNull(base_uri, "about:blank");  // Webkit2 compatible
  SetDefaultStringIfNull(unreachable_uri, "");

  std::string str_data = data;

  if (size < str_data.length())
    str_data = str_data.substr(0, size);  // LCOV_EXCL_LINE

  std::string url_str("data:");
  url_str.append(mime_type);
  url_str.append(";charset=");
  url_str.append(encoding);
  url_str.append(",");

  // GURL constructor performs canonicalization of url string, but this is not
  // enough for correctly escaping contents of "data:" url.
  url_str.append(net::EscapeUrlEncodedData(str_data, false));

  NavigationController::LoadURLParams data_params(GURL(url_str.c_str()));

  data_params.base_url_for_data_url = GURL(base_uri);
  data_params.virtual_url_for_data_url = GURL(unreachable_uri);

  data_params.load_type = NavigationController::LOAD_TYPE_DATA;
  data_params.should_replace_current_entry = should_replace_current_entry;
  data_params.override_user_agent = NavigationController::UA_OVERRIDE_TRUE;
  web_contents_->GetController().LoadURLWithParams(data_params);
}

/* LCOV_EXCL_START */
void EWebView::InvokeLoadError(const GURL& url,
                               int error_code,
                               bool is_cancellation) {
  _Ewk_Error err(error_code, is_cancellation,
                 url.possibly_invalid_spec().c_str());
#if defined(OS_TIZEN_TV_PRODUCT)
  web_contents_->SetUserInputURLNeedNotify(false);
#endif
  SmartCallback<EWebViewCallbacks::LoadError>().call(&err);
}

void EWebView::HandlePopupMenu(const std::vector<content::MenuItem>& items,
                               int selectedIndex,
                               bool multiple,
                               const gfx::Rect& bounds) {
  if (!select_picker_) {
    select_picker_.reset(SelectPickerBase::Create(this,
                                                  selectedIndex,
                                                  items,
                                                  multiple,
                                                  bounds));
#if defined(OS_TIZEN_TV_PRODUCT)
    SmartCallback<EWebViewCallbacks::PopupMenuShow>().call();
#endif
  } else {
    select_picker_->UpdatePickerData(selectedIndex, items, multiple);
  }

  select_picker_->Show();
}

void EWebView::HidePopupMenu() {
  if (!select_picker_)
     return;

  AdjustViewPortHeightToPopupMenu(false /* is_popup_menu_visible */);
#if defined(OS_TIZEN_TV_PRODUCT)
  SmartCallback<EWebViewCallbacks::PopupMenuHide>().call();
#endif
  select_picker_.reset();
}

void EWebView::HandleLongPressGesture(
    const content::ContextMenuParams& params) {
  // This menu is created in renderer process and it does not now anything about
  // view scaling factor and it has another calling sequence, so coordinates is
  // not updated.
  if (settings_ && !settings_->getPreferences().long_press_enabled)
    return;

  content::ContextMenuParams convertedParams = params;
  gfx::Point convertedPoint =
      rwhv()->ConvertPointInViewPix(gfx::Point(params.x, params.y));
  convertedParams.x = convertedPoint.x();
  convertedParams.y = convertedPoint.y();

  Evas_Coord x, y;
  evas_object_geometry_get(evas_object(), &x, &y, 0, 0);
  convertedParams.x += x;
  convertedParams.y += y;

  if (GetSelectionController() && GetSelectionController()->GetLongPressed()) {
    bool show_context_menu_now =
        !GetSelectionController()->HandleLongPressEvent(convertedPoint,
                                                        convertedParams);
    if (show_context_menu_now)
      ShowContextMenuInternal(convertedParams);
  }
}

void EWebView::ShowContextMenu(const content::ContextMenuParams& params) {
  // This menu is created in renderer process and it does not now anything about
  // view scaling factor and it has another calling sequence, so coordinates is
  // not updated.
  content::ContextMenuParams convertedParams = params;
  gfx::Point convertedPoint =
      rwhv()->ConvertPointInViewPix(gfx::Point(params.x, params.y));
  convertedParams.x = convertedPoint.x();
  convertedParams.y = convertedPoint.y();

  Evas_Coord x, y;
  evas_object_geometry_get(evas_object(), &x, &y, 0, 0);
  convertedParams.x += x;
  convertedParams.y += y;

  context_menu_position_ = gfx::Point(convertedParams.x, convertedParams.y);

  ShowContextMenuInternal(convertedParams);
}

void EWebView::ShowContextMenuInternal(
    const content::ContextMenuParams& params) {
  // We don't want to show 'cut' or 'copy' for inputs of type 'password' in
  // selection context menu, but params.input_field_type might not be set
  // correctly, because in some cases params is received from SelectionBoxEfl,
  // not from FrameHostMsg_ContextMenu message. In SelectionBoxEfl
  // ContextMenuParams is constructed on our side with limited information and
  // input_field_type is not set.
  //
  // To work around this, we query for input type and set it separately. In
  // response to EwkViewMsg_QueryInputType we run ::UpdateContextMenu with
  // information about input's type.
  //
  // FIXME: the way we get ContextMenuParams for context menu is flawed in some
  //        cases. This should be fixed by restructuring context menu code.
  //        Context menu creation should be unified to always have
  //        ContextMenuParams received from FrameHostMsg_ContextMenu.
  //        Tracked at: http://suprem.sec.samsung.net/jira/browse/TWF-1640
  if (params.is_editable) {
    saved_context_menu_params_ = params;
    auto render_view_host = web_contents_->GetRenderViewHost();
    render_view_host->Send(
        new EwkViewMsg_QueryInputType(render_view_host->GetRoutingID()));
  } else {
    UpdateContextMenuWithParams(params);
  }
}

void EWebView::UpdateContextMenu(bool is_password_input) {
  if (is_password_input) {
    saved_context_menu_params_.input_field_type =
        blink::WebContextMenuData::kInputFieldTypePassword;
  }
  UpdateContextMenuWithParams(saved_context_menu_params_);
}


void EWebView::UpdateContextMenuWithParams(
    const content::ContextMenuParams& params) {
  context_menu_.reset(
      new content::ContextMenuControllerEfl(this, *web_contents_.get()));

  if (IsMobileProfile()) {
    if (delayed_show_context_menu_timer_) {
      ecore_timer_del(delayed_show_context_menu_timer_);
      delayed_show_context_menu_timer_ = nullptr;
    }
    saved_context_menu_params_ = params;
    delayed_show_context_menu_timer_ = ecore_timer_add(
        kDelayShowContextMenuTime, DelayedPopulateAndShowContextMenu, this);
  } else {
    if (!context_menu_->PopulateAndShowContextMenu(params)) {
      context_menu_.reset();
      if (GetSelectionController())
        GetSelectionController()->HideHandles();
    }
  }
}

Eina_Bool EWebView::DelayedPopulateAndShowContextMenu(void* data) {
  if (IsMobileProfile()) {
    EWebView* view = static_cast<EWebView*>(data);
    if (view) {
      if (view->context_menu_ &&
          !(view->context_menu_->PopulateAndShowContextMenu(
              view->saved_context_menu_params_))) {
        view->context_menu_.reset();
      }
      view->delayed_show_context_menu_timer_ = nullptr;
    }
  }
  return ECORE_CALLBACK_CANCEL;
}
/* LCOV_EXCL_STOP */

void EWebView::CancelContextMenu(int request_id) {
  if (context_menu_)
    context_menu_->HideContextMenu(); // LCOV_EXCL_LINE
}

/* LCOV_EXCL_START */
void EWebView::Find(const char* text, Ewk_Find_Options find_options) {
  base::string16 find_text = base::UTF8ToUTF16(text);
  bool find_next = (previous_text_ == find_text);

  if (!find_next) {
    current_find_request_id_ = find_request_id_counter_++;
    previous_text_ = find_text;
  }

  blink::WebFindOptions web_find_options;
  web_find_options.forward = !(find_options & EWK_FIND_OPTIONS_BACKWARDS);
  web_find_options.match_case =
      !(find_options & EWK_FIND_OPTIONS_CASE_INSENSITIVE);
  web_find_options.find_next = find_next;

  web_contents_->Find(current_find_request_id_, find_text, web_find_options);
}

void EWebView::SetScale(double scale_factor) {
  RenderViewHost* render_view_host = web_contents_->GetRenderViewHost();
  // Do not cache |scale_factor| here as it may be discarded by Blink's
  // minimumPageScaleFactor and maximumPageScaleFactor.
  // |scale_factor| is cached as responde to DidChangePageScaleFactor.
  render_view_host->Send(new ViewMsg_SetPageScale(
      render_view_host->GetRoutingID(), scale_factor));
}

void EWebView::SetScaleChangedCallback(Ewk_View_Scale_Changed_Callback callback,
    void* user_data) {
  scale_changed_cb_.Set(callback, user_data);
}

void EWebView::ScrollFocusedNodeIntoView() {
  RenderViewHost* render_view_host = web_contents_->GetRenderViewHost();
  if (render_view_host)
    render_view_host->Send(new EwkViewMsg_ScrollFocusedNodeIntoView(render_view_host->GetRoutingID()));
}

void EWebView::AdjustViewPortHeightToPopupMenu(bool is_popup_menu_visible) {
  if (!IsMobileProfile() ||
      settings_->getPreferences()
          .tizen_version.TizenCompatibilityModeEnabled())
    return;

  int picker_height = select_picker_->GetGeometryDIP().height();
  gfx::Rect screen_rect =
      display::Screen::GetScreen()->GetPrimaryDisplay().bounds();
  gfx::Rect view_rect = rwhv()->GetViewBounds();
  int bottom_height =
      screen_rect.height() - (view_rect.y() + view_rect.height());

  rwhv()->SetCustomViewportSize(
      is_popup_menu_visible
          ? gfx::Size(view_rect.width(),
                      view_rect.height() - picker_height + bottom_height)
          : gfx::Size());
}

bool EWebView::GetScrollPosition(int* x, int* y) const {
  if (!rwhv()) {
    LOG(ERROR) << "rwhv() returns nullptr";
    return false;
  }

  if (scroll_detector_->IsScrollOffsetChanged()) {
    if (x)
      *x = previous_scroll_position_.x();
    if (y)
      *y = previous_scroll_position_.y();
  } else {
    const gfx::Vector2d scroll_position_dip =
        scroll_detector_->GetLastScrollPosition();
    const float device_scale_factor = display::Screen::GetScreen()
                                          ->GetPrimaryDisplay()
                                          .device_scale_factor();
    if (x) {
      *x = gfx::ToRoundedInt((scroll_position_dip.x() - x_delta_) *
                             device_scale_factor);
    }
    if (y) {
      *y = gfx::ToRoundedInt((scroll_position_dip.y() - y_delta_) *
                             device_scale_factor);
    }
  }
  return true;
}

void EWebView::ChangeScroll(int& x, int& y) {
  if (!rwhv()) {
    LOG(ERROR) << "rwhv() returns nullptr";
    return;
  }

  int max_x = 0;
  int max_y = 0;
  GetScrollSize(&max_x, &max_y);
  previous_scroll_position_.set_x(std::min(std::max(x, 0), max_x));
  previous_scroll_position_.set_y(std::min(std::max(y, 0), max_y));

  const float device_scale_factor = display::Screen::GetScreen()
                                        ->GetPrimaryDisplay()
                                        .device_scale_factor();
  int x_input = x;
  int y_input = y;

  x = gfx::ToCeiledInt(x / device_scale_factor);
  y = gfx::ToCeiledInt(y / device_scale_factor);

  x_delta_ = x - (x_input / device_scale_factor);
  y_delta_ = y - (y_input / device_scale_factor);

  scroll_detector_->SetScrollOffsetChanged();
}

void EWebView::SetScroll(int x, int y) {
  RenderViewHost* render_view_host = web_contents_->GetRenderViewHost();
  if (!render_view_host)
    return;

  ChangeScroll(x, y);
  render_view_host->Send(
      new EwkViewMsg_SetScroll(render_view_host->GetRoutingID(), x, y));
}

void EWebView::UseSettingsFont() {
  RenderViewHost* render_view_host = web_contents_->GetRenderViewHost();
  if (render_view_host)
    render_view_host->Send(
        new EwkViewMsg_UseSettingsFont(render_view_host->GetRoutingID()));
}
/* LCOV_EXCL_STOP */

void EWebView::DidChangeContentsSize(int width, int height) {
  contents_size_ = gfx::Size(width, height);
  SmartCallback<EWebViewCallbacks::ContentsSizeChanged>().call();
  SetScaledContentsSize();

#if defined(TIZEN_AUTOFILL_SUPPORT)
  if (web_contents_delegate_)
    web_contents_delegate_->UpdateAutofillIfRequired();
#endif
}

const Eina_Rectangle EWebView::GetContentsSize() const {  // LCOV_EXCL_LINE
  Eina_Rectangle rect;
  EINA_RECTANGLE_SET(&rect, 0, 0, contents_size_.width(),  // LCOV_EXCL_LINE
                     contents_size_.height());
  return rect;  // LCOV_EXCL_LINE
}

void EWebView::SetScaledContentsSize() {
  if (!rwhv())
    return;

  const float device_scale_factor =
      display::Screen::GetScreen()->GetPrimaryDisplay().device_scale_factor();
  gfx::Size scaled_contents_size = gfx::ConvertSizeToPixel(
      device_scale_factor * page_scale_factor_, contents_size_);
  rwhv()->SetScaledContentSize(scaled_contents_size);
}

/* LCOV_EXCL_START */
void EWebView::GetScrollSize(int* width, int* height) {
  int w = 0, h = 0;
  if (rwhv()) {
    gfx::Size scrollable_size = rwhv()->GetScrollableSize();
    w = scrollable_size.width();
    h = scrollable_size.height();
  }

  if (width)
    *width = w;
  if (height)
    *height = h;
}

bool EWebView::SetTopControlsHeight(size_t top_height, size_t bottom_height) {
  if (!rwhv())
    return false;

  rwhv()->SetTopControlsHeight(top_height, bottom_height);

  return true;
}

bool EWebView::SetTopControlsState(Ewk_Top_Control_State constraint,
                                   Ewk_Top_Control_State current,
                                   bool animate) {
  if (!rwhv())
    return false;

  return rwhv()->SetTopControlsState(
      static_cast<content::BrowserControlsState>(constraint),
      static_cast<content::BrowserControlsState>(current), animate);
}

void EWebView::MoveCaret(const gfx::Point& point) {
  if (rwhv())
    rwhv()->MoveCaret(point);
}

SelectionControllerEfl* EWebView::GetSelectionController() const {
  RenderViewHost* render_view_host = web_contents_->GetRenderViewHost();
  RenderWidgetHostViewEfl* view = static_cast<RenderWidgetHostViewEfl*>(
      render_view_host->GetWidget()->GetView());
  return view ? view->GetSelectionController() : 0;
}

void EWebView::SelectFocusedLink() {
  auto rvh = web_contents_->GetRenderViewHost();
  rvh->Send(new ViewMsg_SelectFocusedLink(rvh->GetRoutingID()));
}

bool EWebView::GetSelectionRange(Eina_Rectangle* left_rect,
                                 Eina_Rectangle* right_rect) {
  if (left_rect && right_rect) {
    gfx::Rect left, right;
    if (GetSelectionController()) {
      GetSelectionController()->GetSelectionBounds(&left, &right);
      GetEinaRectFromGfxRect(left, left_rect);
      GetEinaRectFromGfxRect(right, right_rect);
      return true;
    }
  }
  return false;
}

void EWebView::OnSelectionRectReceived(const gfx::Rect& selection_rect) const {
  if (context_menu_)
    context_menu_->OnSelectionRectReceived(selection_rect);
}

Eina_Bool EWebView::ClearSelection() {
  if (!rwhv())
    return EINA_FALSE;

  ResetContextMenuController();
  rwhv()->SelectionChanged(base::string16(), 0, gfx::Range());

  if (GetSelectionController())
    return GetSelectionController()->ClearSelectionViaEWebView();

  return EINA_FALSE;
}

_Ewk_Hit_Test* EWebView::RequestHitTestDataAt(int x,
                                              int y,
                                              Ewk_Hit_Test_Mode mode) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));

  int view_x, view_y;
  EvasToBlinkCords(x, y, &view_x, &view_y);

  return RequestHitTestDataAtBlinkCoords(view_x, view_y, mode);
}

Eina_Bool EWebView::AsyncRequestHitTestDataAt(
    int x,
    int y,
    Ewk_Hit_Test_Mode mode,
    Ewk_View_Hit_Test_Request_Callback callback,
    void* user_data) {
  int view_x, view_y;
  EvasToBlinkCords(x, y, &view_x, &view_y);
  return AsyncRequestHitTestDataAtBlinkCords(
      view_x, view_y, mode, new WebViewAsyncRequestHitTestDataUserCallback(
                                x, y, mode, callback, user_data));
}
/* LCOV_EXCL_STOP */

Eina_Bool EWebView::AsyncRequestHitTestDataAtBlinkCords(
    int x,
    int y,
    Ewk_Hit_Test_Mode mode,
    Ewk_View_Hit_Test_Request_Callback callback,
    void* user_data) {
  return AsyncRequestHitTestDataAtBlinkCords(
      x, y, mode, new WebViewAsyncRequestHitTestDataUserCallback(
                      x, y, mode, callback, user_data));
}

Eina_Bool EWebView::AsyncRequestHitTestDataAtBlinkCords(
    int x,
    int y,
    Ewk_Hit_Test_Mode mode,
    WebViewAsyncRequestHitTestDataCallback* cb) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  DCHECK(cb);

  static int64_t request_id = 1;

  if (cb) {  // LCOV_EXCL_LINE
    RenderViewHost* render_view_host =
        web_contents_->GetRenderViewHost();  // LCOV_EXCL_LINE
    DCHECK(render_view_host);

    if (render_view_host) {  // LCOV_EXCL_LINE
      render_view_host->Send(
          new EwkViewMsg_DoHitTestAsync(render_view_host->GetRoutingID(), x, y,
                                        mode, request_id));  // LCOV_EXCL_LINE
      hit_test_callback_[request_id] = cb;                   // LCOV_EXCL_LINE
      ++request_id;                                          // LCOV_EXCL_LINE
      return EINA_TRUE;
    }
  }

  // if failed we delete callback as it is not needed anymore
  delete cb;  // LCOV_EXCL_LINE
  return EINA_FALSE;
}

void EWebView::DispatchAsyncHitTestData(
    const Hit_Test_Params& params,  // LCOV_EXCL_LINE
    int64_t request_id) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));

  std::map<int64_t, WebViewAsyncRequestHitTestDataCallback*>::iterator it =
      hit_test_callback_.find(request_id);  // LCOV_EXCL_LINE

  /* LCOV_EXCL_START */
  if (it == hit_test_callback_.end())
    return;
  std::unique_ptr<_Ewk_Hit_Test> hit_test(new _Ewk_Hit_Test(params));

  it->second->Run(hit_test.get(), this);
  delete it->second;
  hit_test_callback_.erase(it);
  /* LCOV_EXCL_STOP */
}

_Ewk_Hit_Test* EWebView::RequestHitTestDataAtBlinkCoords(  // LCOV_EXCL_LINE
    int x,
    int y,
    Ewk_Hit_Test_Mode mode) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));

  RenderViewHost* render_view_host = web_contents_->GetRenderViewHost();

  if (!render_view_host)
    return nullptr;

  if (!render_view_host->Send(new EwkViewMsg_DoHitTest(
      render_view_host->GetRoutingID(), x, y, mode)))
    return nullptr;

  // We wait on UI thread till hit test data is updated.
  ScopedAllowWaitForLegacyWebViewApi allow_wait;
  hit_test_completion_.Wait();

  return new _Ewk_Hit_Test(hit_test_params_);
}

/* LCOV_EXCL_START */
void EWebView::EvasToBlinkCords(int x, int y, int* view_x, int* view_y) {
  if (!rwhv())
    return;

  DCHECK(display::Screen::GetScreen());
  gfx::Rect view_bounds = rwhv()->GetViewBoundsInPix();

  if (view_x) {
    *view_x = x - view_bounds.x();
    *view_x /= display::Screen::GetScreen()->
        GetPrimaryDisplay().device_scale_factor();
  }

  if (view_y) {
    *view_y = y - view_bounds.y();
    *view_y /= display::Screen::GetScreen()->
        GetPrimaryDisplay().device_scale_factor();
  }
}

void EWebView::UpdateHitTestData(const Hit_Test_Params& params) {
  DCHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::IO));
  hit_test_params_ = params;
  hit_test_completion_.Signal();
}

void EWebView::OnCopyFromBackingStore(bool success, const SkBitmap& bitmap) {}

void EWebView::OnFocusIn() {
  SmartCallback<EWebViewCallbacks::FocusIn>().call();
#if defined(USE_WAYLAND)
  if (!IsTvProfile() || GetSettings()->getClipboardEnabled()) {
    ClipboardHelperEfl::GetInstance()->OnWebviewFocusIn(
        this, content_image_elm_host(),
        rwhv() ? rwhv()->IsFocusedNodeContentEditable() : false,
        base::Bind(&EWebView::ExecuteEditCommand, base::Unretained(this)));
  }
#endif
}

void EWebView::OnFocusOut() {
#if defined(TIZEN_ATK_SUPPORT)
  if (IsMobileProfile())
    eweb_accessibility_->OnFocusOut();
#endif
  SmartCallback<EWebViewCallbacks::FocusOut>().call();
#if defined(OS_TIZEN_TV_PRODUCT)
// FIXME: popup menu should be hidden when focus out.
  HidePopupMenu();
#endif
#if defined(USE_WAYLAND)
  if (!IsTvProfile() || GetSettings()->getClipboardEnabled())
    ClipboardHelperEfl::GetInstance()->MaybeInvalidateActiveWebview(this);
#endif
}

const gfx::Rect& EWebView::GetCustomEmailViewportRect() const {
  return custom_email_viewport_rect_;
}
/* LCOV_EXCL_STOP */

void EWebView::RenderViewCreated(RenderViewHost* render_view_host) {
  SendDelayedMessages(render_view_host);
  UpdateWebkitPreferencesEfl(render_view_host);
  if (rwhv()) {
    rwhv()->SetEvasHandler(evas_event_handler_);
    rwhv()->SetFocusInOutCallbacks(
        base::Bind(&EWebView::OnFocusIn, base::Unretained(this)),
        base::Bind(&EWebView::OnFocusOut, base::Unretained(this)));
    rwhv()->SetGetCustomEmailViewportRectCallback(base::Bind(
        &EWebView::GetCustomEmailViewportRect, base::Unretained(this)));
  }

  if (render_view_host) {
#if defined(OS_TIZEN_TV_PRODUCT)
    // FIXME: RenderWidgetHostView get from rwhv() isn't which I needed
    RenderWidgetHostViewEfl* view = static_cast<RenderWidgetHostViewEfl*>(
        render_view_host->GetWidget()->GetView());
    view->SetMouseEventCallbacks(
        base::Bind(&EWebView::OnMouseDown, base::Unretained(this)),
        base::Bind(&EWebView::OnMouseUp, base::Unretained(this)),
        base::Bind(&EWebView::OnMouseMove, base::Unretained(this)));
#endif
    WebContents* content = WebContents::FromRenderViewHost(render_view_host);
    if (content) {
      RenderProcessHost* host = render_view_host->GetProcess();
      if (host)
        host->AddFilter(new WebViewBrowserMessageFilter(content));
    }
  }
}

void EWebView::SetQuotaPermissionRequestCallback(
    Ewk_Quota_Permission_Request_Callback callback,
    void* user_data) {
  quota_request_callback_.Set(callback, user_data);
}

Evas_Object* EWebView::content_image_elm_host() const {
  return (rwhv() ? rwhv()->content_image_elm_host() : nullptr);
}

void EWebView::InvokeQuotaPermissionRequest(
    _Ewk_Quota_Permission_Request* request,
    const content::QuotaPermissionContext::PermissionCallback& cb) {
  quota_permission_request_map_[request] = cb;
  request->setView(evas_object());
  if (quota_request_callback_.IsCallbackSet())
    quota_request_callback_.Run(evas_object(), request);
  else
    QuotaRequestCancel(request);
}
/* LCOV_EXCL_STOP */

void EWebView::QuotaRequestReply(
    const _Ewk_Quota_Permission_Request* request,  // LCOV_EXCL_LINE
    bool allow) {
  DCHECK(quota_permission_request_map_.find(request) !=
         quota_permission_request_map_.end());  // LCOV_EXCL_LINE

  QuotaPermissionContext::PermissionCallback cb =
      quota_permission_request_map_[request];

  if (allow)  // LCOV_EXCL_LINE
    QuotaPermissionContextEfl::DispatchCallback(
        cb, QuotaPermissionContext::
                QUOTA_PERMISSION_RESPONSE_ALLOW);  // LCOV_EXCL_LINE
  else
    QuotaPermissionContextEfl::DispatchCallback(
        cb, QuotaPermissionContext::
                QUOTA_PERMISSION_RESPONSE_DISALLOW);  // LCOV_EXCL_LINE

  quota_permission_request_map_.erase(request);  // LCOV_EXCL_LINE
  delete request;                                // LCOV_EXCL_LINE
}  // LCOV_EXCL_LINE

void EWebView::QuotaRequestCancel(  // LCOV_EXCL_LINE
    const _Ewk_Quota_Permission_Request* request) {
  DCHECK(quota_permission_request_map_.find(request) !=
         quota_permission_request_map_.end());

  QuotaPermissionContext::PermissionCallback cb =
      quota_permission_request_map_[request];  // LCOV_EXCL_LINE
  QuotaPermissionContextEfl::DispatchCallback(
      cb, QuotaPermissionContext::QUOTA_PERMISSION_RESPONSE_CANCELLED);
  quota_permission_request_map_.erase(request);  // LCOV_EXCL_LINE
  delete request;                                // LCOV_EXCL_LINE
}  // LCOV_EXCL_LINE

/* LCOV_EXCL_START */
bool EWebView::GetLinkMagnifierEnabled() const {
  return web_contents_->GetMutableRendererPrefs()
             ->tap_multiple_targets_strategy ==
         TAP_MULTIPLE_TARGETS_STRATEGY_POPUP;
}

void EWebView::SetLinkMagnifierEnabled(bool enabled) {
  web_contents_->GetMutableRendererPrefs()->tap_multiple_targets_strategy =
      enabled ? TAP_MULTIPLE_TARGETS_STRATEGY_POPUP
              : TAP_MULTIPLE_TARGETS_STRATEGY_NONE;
  web_contents_->GetRenderViewHost()->SyncRendererPrefs();
}

bool EWebView::GetSnapshotAsync(
    Eina_Rectangle rect,
    Ewk_Web_App_Screenshot_Captured_Callback callback,
    void* user_data,
    float scale_factor) {
  if (!rwhv())
    return false;

  return rwhv()->RequestSnapshotAsync(gfx::Rect(rect.x, rect.y, rect.w, rect.h),
                                      callback, user_data, scale_factor);
}

Evas_Object* EWebView::GetSnapshot(Eina_Rectangle rect, float scale_factor) {
  if (!rwhv())
    return nullptr;

  return rwhv()->GetSnapshot(gfx::Rect(rect.x, rect.y, rect.w, rect.h),
                             scale_factor);
}

void EWebView::BackForwardListClear() {
  content::NavigationController& controller = web_contents_->GetController();

  int entry_count = controller.GetEntryCount();
  bool entry_removed = false;

  for (int i = 0; i < entry_count; i++) {
    if (controller.RemoveEntryAtIndex(i)) {
      entry_removed = true;
      entry_count = controller.GetEntryCount();
      i--;
    }
  }

  if (entry_removed) {
    back_forward_list_->ClearCache();
    InvokeBackForwardListChangedCallback();
  }
}
/* LCOV_EXCL_STOP */

_Ewk_Back_Forward_List* EWebView::GetBackForwardList() const {
  return back_forward_list_.get();
}

void EWebView::InvokeBackForwardListChangedCallback() {
  SmartCallback<EWebViewCallbacks::BackForwardListChange>().call();
}

/* LCOV_EXCL_START */
_Ewk_History* EWebView::GetBackForwardHistory() const {
  return new _Ewk_History(web_contents_->GetController());
}

bool EWebView::WebAppCapableGet(Ewk_Web_App_Capable_Get_Callback callback,
                                void* userData) {
  RenderViewHost* renderViewHost = web_contents_->GetRenderViewHost();
  if (!renderViewHost) {
    return false;
  }
  WebApplicationCapableGetCallback* cb =
      new WebApplicationCapableGetCallback(callback, userData);
  int callbackId = web_app_capable_get_callback_map_.Add(cb);
  return renderViewHost->Send(new EwkViewMsg_WebAppCapableGet(
      renderViewHost->GetRoutingID(), callbackId));
}

bool EWebView::WebAppIconUrlGet(Ewk_Web_App_Icon_URL_Get_Callback callback,
                                void* userData) {
  RenderViewHost* renderViewHost = web_contents_->GetRenderViewHost();
  if (!renderViewHost) {
    return false;
  }
  WebApplicationIconUrlGetCallback* cb =
      new WebApplicationIconUrlGetCallback(callback, userData);
  int callbackId = web_app_icon_url_get_callback_map_.Add(cb);
  return renderViewHost->Send(new EwkViewMsg_WebAppIconUrlGet(
      renderViewHost->GetRoutingID(), callbackId));
}

bool EWebView::WebAppIconUrlsGet(Ewk_Web_App_Icon_URLs_Get_Callback callback,
                                 void* userData) {
  RenderViewHost* renderViewHost = web_contents_->GetRenderViewHost();
  if (!renderViewHost) {
    return false;
  }
  WebApplicationIconUrlsGetCallback* cb =
      new WebApplicationIconUrlsGetCallback(callback, userData);
  int callbackId = web_app_icon_urls_get_callback_map_.Add(cb);
  return renderViewHost->Send(new EwkViewMsg_WebAppIconUrlsGet(
      renderViewHost->GetRoutingID(), callbackId));
}

void EWebView::InvokeWebAppCapableGetCallback(bool capable, int callbackId) {
  WebApplicationCapableGetCallback* callback =
      web_app_capable_get_callback_map_.Lookup(callbackId);
  if (!callback)
    return;
  callback->Run(capable);
  web_app_capable_get_callback_map_.Remove(callbackId);
}

void EWebView::InvokeWebAppIconUrlGetCallback(const std::string& iconUrl,
                                              int callbackId) {
  WebApplicationIconUrlGetCallback* callback =
      web_app_icon_url_get_callback_map_.Lookup(callbackId);
  if (!callback)
    return;
  callback->Run(iconUrl);
  web_app_icon_url_get_callback_map_.Remove(callbackId);
}

void EWebView::InvokeWebAppIconUrlsGetCallback(const StringMap& iconUrls,
                                               int callbackId) {
  WebApplicationIconUrlsGetCallback* callback =
      web_app_icon_urls_get_callback_map_.Lookup(callbackId);
  if (!callback) {
    return;
  }
  callback->Run(iconUrls);
  web_app_icon_urls_get_callback_map_.Remove(callbackId);
}
/* LCOV_EXCL_STOP */

void EWebView::SetNotificationPermissionCallback(
    Ewk_View_Notification_Permission_Callback callback,
    void* user_data) {
  notification_permission_callback_.Set(callback, user_data);
}

/* LCOV_EXCL_START */
bool EWebView::IsNotificationPermissionCallbackSet() const {
  return notification_permission_callback_.IsCallbackSet();
}

bool EWebView::InvokeNotificationPermissionCallback(
    Ewk_Notification_Permission_Request* request) {
  Eina_Bool ret = EINA_FALSE;
  notification_permission_callback_.Run(evas_object_, request, &ret);
  return ret;
}

int EWebView::SetEwkViewPlainTextGetCallback(
    Ewk_View_Plain_Text_Get_Callback callback,
    void* user_data) {
  EwkViewPlainTextGetCallback* view_plain_text_callback_ptr =
      new EwkViewPlainTextGetCallback;
  view_plain_text_callback_ptr->Set(callback, user_data);
  return plain_text_get_callback_map_.Add(view_plain_text_callback_ptr);
}

bool EWebView::PlainTextGet(Ewk_View_Plain_Text_Get_Callback callback,
                            void* user_data) {
  RenderViewHost* render_view_host = web_contents_->GetRenderViewHost();
  if (!render_view_host)
    return false;
  int plain_text_get_callback_id =
      SetEwkViewPlainTextGetCallback(callback, user_data);
  return render_view_host->Send(new EwkViewMsg_PlainTextGet(
      render_view_host->GetRoutingID(), plain_text_get_callback_id));
}

void EWebView::InvokePlainTextGetCallback(const std::string& content_text,
                                          int plain_text_get_callback_id) {
  EwkViewPlainTextGetCallback* view_plain_text_callback_invoke_ptr =
      plain_text_get_callback_map_.Lookup(plain_text_get_callback_id);
  view_plain_text_callback_invoke_ptr->Run(evas_object(), content_text.c_str());
  plain_text_get_callback_map_.Remove(plain_text_get_callback_id);
}

void EWebView::SetViewGeolocationPermissionCallback(
    Ewk_View_Geolocation_Permission_Callback callback,
    void* user_data) {
  geolocation_permission_cb_.Set(callback, user_data);
}

bool EWebView::InvokeViewGeolocationPermissionCallback(
    _Ewk_Geolocation_Permission_Request* permission_context,
    Eina_Bool* callback_result) {
  return geolocation_permission_cb_.Run(evas_object_, permission_context,
                                        callback_result);
}
/* LCOV_EXCL_STOP */

void EWebView::SetViewUserMediaPermissionCallback(
    Ewk_View_User_Media_Permission_Callback callback,
    void* user_data) {
  user_media_permission_cb_.Set(callback, user_data);
}

/* LCOV_EXCL_START */
bool EWebView::InvokeViewUserMediaPermissionCallback(
    _Ewk_User_Media_Permission_Request* permission_context,
    Eina_Bool* callback_result) {
  return user_media_permission_cb_.Run(evas_object_, permission_context,
                                       callback_result);
}

void EWebView::SetViewUnfocusAllowCallback(
    Ewk_View_Unfocus_Allow_Callback callback,
    void* user_data) {
  unfocus_allow_cb_.Set(callback, user_data);
}

bool EWebView::InvokeViewUnfocusAllowCallback(Ewk_Unfocus_Direction direction,
                                              Eina_Bool* callback_result) {
  return unfocus_allow_cb_.Run(evas_object_, direction, callback_result);
}

void EWebView::StopFinding() {
  previous_text_.clear();
  web_contents_->StopFinding(content::STOP_FIND_ACTION_CLEAR_SELECTION);
}

void EWebView::SetProgressValue(double progress) {
  progress_ = progress;
}

double EWebView::GetProgressValue() {
  return progress_;
}

const char* EWebView::GetTitle() {
  if (web_contents_->GetMainFrame())
    title_ = base::UTF16ToUTF8(web_contents_->GetTitle());
  else
    title_.clear();
  return title_.c_str();
}

bool EWebView::SaveAsPdf(int width, int height, const std::string& filename) {
  RenderViewHost* render_view_host = web_contents_->GetRenderViewHost();
  if (!render_view_host)
    return false;

  return render_view_host->Send(
      new EwkViewMsg_PrintToPdf(render_view_host->GetRoutingID(), width, height,
                                base::FilePath(filename)));
}

bool EWebView::GetMHTMLData(Ewk_View_MHTML_Data_Get_Callback callback,
                            void* user_data) {
  RenderViewHost* render_view_host = web_contents_->GetRenderViewHost();
  if (!render_view_host)
    return false;

  MHTMLCallbackDetails* callback_details = new MHTMLCallbackDetails;
  callback_details->Set(callback, user_data);
  int mhtml_callback_id = mhtml_callback_map_.Add(callback_details);
  return render_view_host->Send(new EwkViewMsg_GetMHTMLData(
      render_view_host->GetRoutingID(), mhtml_callback_id));
}

void EWebView::OnMHTMLContentGet(const std::string& mhtml_content,
                                 int callback_id) {
  MHTMLCallbackDetails* callback_details =
      mhtml_callback_map_.Lookup(callback_id);
  callback_details->Run(evas_object(), mhtml_content.c_str());
  mhtml_callback_map_.Remove(callback_id);
}

bool EWebView::GetBackgroundColor(
    Ewk_View_Background_Color_Get_Callback callback,
    void* user_data) {
  RenderViewHost* render_view_host = web_contents_->GetRenderViewHost();
  if (!render_view_host)
    return false;

  BackgroundColorGetCallback* cb =
      new BackgroundColorGetCallback(callback, user_data);
  int callback_id = background_color_get_callback_map_.Add(cb);
  return render_view_host->Send(new EwkViewMsg_GetBackgroundColor(
      render_view_host->GetRoutingID(), callback_id));
}

void EWebView::OnGetBackgroundColor(int r,
                                    int g,
                                    int b,
                                    int a,
                                    int callback_id) {
  BackgroundColorGetCallback* cb =
      background_color_get_callback_map_.Lookup(callback_id);

  if (!cb)
    return;

  cb->Run(evas_object(), r, g, b, a);
  background_color_get_callback_map_.Remove(callback_id);
}

void EWebView::SetReaderMode(Eina_Bool enable) {
  if (web_contents_delegate_)
    web_contents_delegate_->SetReaderMode(enable);
}

bool EWebView::SavePageAsMHTML(const std::string& path,
                               Ewk_View_Save_Page_Callback callback,
                               void* user_data) {
  if (!web_contents_)
    return false;

  GURL url(web_contents_->GetLastCommittedURL());
  base::string16 title(web_contents_->GetTitle());

  // Post function that has file access to blocking task runner.
  return base::PostTaskAndReplyWithResult(
      base::CreateSequencedTaskRunnerWithTraits(
          {base::MayBlock(), base::TaskPriority::BACKGROUND,
           base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN})
          .get(),
      FROM_HERE,
      base::Bind(&GenerateMHTMLFilePath, url, base::UTF16ToUTF8(title), path),
      base::Bind(&EWebView::GenerateMHTML, base::Unretained(this), callback,
                 user_data));
}

void EWebView::GenerateMHTML(Ewk_View_Save_Page_Callback callback,
                             void* user_data,
                             const base::FilePath& file_path) {
  if (file_path.empty()) {
    LOG(ERROR) << "Generating file path was failed";
    callback(evas_object_, nullptr, user_data);
    return;
  }

  MHTMLGenerationParams params(file_path);
  web_contents_->GenerateMHTML(
      params, base::Bind(&EWebView::MHTMLGenerated, base::Unretained(this),
                         callback, user_data, file_path));
}

void EWebView::MHTMLGenerated(Ewk_View_Save_Page_Callback callback,
                              void* user_data,
                              const base::FilePath& file_path,
                              int64_t file_size) {
  callback(evas_object_, file_size > 0 ? file_path.value().c_str() : nullptr,
           user_data);
}

bool EWebView::IsFullscreen() {
  return web_contents_delegate_->IsFullscreenForTabOrPending(
      web_contents_.get());
}
/* LCOV_EXCL_STOP */

void EWebView::ExitFullscreen() {
  WebContentsImpl* wci = static_cast<WebContentsImpl*>(web_contents_.get());
  wci->ExitFullscreen(false);
}

/* LCOV_EXCL_START */
double EWebView::GetScale() {
  return page_scale_factor_;
}

void EWebView::DidChangePageScaleFactor(double scale_factor) {
  page_scale_factor_ = scale_factor;
  GetWebContentsViewEfl()->SetPageScaleFactor(scale_factor);
  SetScaledContentsSize();

  // Notify app about the scale change.
  scale_changed_cb_.Run(evas_object_, scale_factor);
}

inline JavaScriptDialogManagerEfl* EWebView::GetJavaScriptDialogManagerEfl() {
  return static_cast<JavaScriptDialogManagerEfl*>(
      web_contents_delegate_->GetJavaScriptDialogManager(web_contents_.get()));
}

void EWebView::SetJavaScriptAlertCallback(
    Ewk_View_JavaScript_Alert_Callback callback,
    void* user_data) {
  GetJavaScriptDialogManagerEfl()->SetAlertCallback(callback, user_data);
}

void EWebView::JavaScriptAlertReply() {
  GetJavaScriptDialogManagerEfl()->ExecuteDialogClosedCallBack(true,
                                                               std::string());
  SmartCallback<EWebViewCallbacks::PopupReplyWaitFinish>().call(0);
}

void EWebView::SetJavaScriptConfirmCallback(
    Ewk_View_JavaScript_Confirm_Callback callback,
    void* user_data) {
  GetJavaScriptDialogManagerEfl()->SetConfirmCallback(callback, user_data);
}

void EWebView::JavaScriptConfirmReply(bool result) {
  GetJavaScriptDialogManagerEfl()->ExecuteDialogClosedCallBack(result,
                                                               std::string());
  SmartCallback<EWebViewCallbacks::PopupReplyWaitFinish>().call(0);
}

void EWebView::SetJavaScriptPromptCallback(
    Ewk_View_JavaScript_Prompt_Callback callback,
    void* user_data) {
  GetJavaScriptDialogManagerEfl()->SetPromptCallback(callback, user_data);
}

void EWebView::JavaScriptPromptReply(const char* result) {
  GetJavaScriptDialogManagerEfl()->ExecuteDialogClosedCallBack(
      true, (std::string(result)));
  SmartCallback<EWebViewCallbacks::PopupReplyWaitFinish>().call(0);
}

void EWebView::GetPageScaleRange(double* min_scale, double* max_scale) {
  RenderViewHost* render_view_host = web_contents_->GetRenderViewHost();
  if (!render_view_host) {
    if (min_scale)
      *min_scale = -1.f;
    if (max_scale)
      *max_scale = -1.f;
    return;
  }

  WebPreferences prefs = render_view_host->GetWebkitPreferences();
  if (min_scale)
    *min_scale = prefs.default_minimum_page_scale_factor;
  if (max_scale)
    *max_scale = prefs.default_maximum_page_scale_factor;
}

bool EWebView::GetDrawsTransparentBackground() {
  return GetWebContentsViewEfl()->GetBackgroundColor() == SK_ColorTRANSPARENT;
}

bool EWebView::SetBackgroundColor(int red, int green, int blue, int alpha) {
  RenderViewHost* render_view_host = web_contents_->GetRenderViewHost();
  if (!render_view_host || !rwhv())
    return false;

  if (!alpha) {
    red = 0;
    green = 0;
    blue = 0;
  }

  elm_object_style_set(rwhv()->content_image_elm_host(),
                       alpha < 255 ? "transparent" : "default");
  evas_object_image_alpha_set(rwhv()->content_image(), alpha < 255);

  GetWebContentsViewEfl()->SetBackgroundColor(
      SkColorSetARGB(alpha, red, green, blue));

  render_view_host->Send(new EwkViewMsg_SetBackgroundColor(
      render_view_host->GetRoutingID(), red, green, blue, alpha));

  rwhv()->SetBackgroundColor(SkColorSetARGB(alpha, red, green, blue));

  return true;
}

void EWebView::GetSessionData(const char** data, unsigned* length) const {
  static const int MAX_SESSION_ENTRY_SIZE = std::numeric_limits<int>::max();

  NavigationController& navigationController = web_contents_->GetController();
  base::Pickle sessionPickle;
  const int itemCount = navigationController.GetEntryCount();

  sessionPickle.WriteInt(itemCount);
  sessionPickle.WriteInt(navigationController.GetCurrentEntryIndex());

  for (int i = 0; i < itemCount; i++) {
    NavigationEntry* navigationEntry = navigationController.GetEntryAtIndex(i);
    sessions::SerializedNavigationEntry serializedEntry =
        sessions::ContentSerializedNavigationBuilder::FromNavigationEntry(
            i, *navigationEntry);
    serializedEntry.WriteToPickle(MAX_SESSION_ENTRY_SIZE, &sessionPickle);
  }

  *data = static_cast<char*>(malloc(sizeof(char) * sessionPickle.size()));
  if (!(*data)) {
    LOG(ERROR) << "Failed to allocate memory for session data";
    *data = nullptr;
    *length = 0;
    return;
  }
  memcpy(const_cast<char*>(*data), sessionPickle.data(), sessionPickle.size());
  *length = sessionPickle.size();
}

bool EWebView::RestoreFromSessionData(const char* data, unsigned length) {
  base::Pickle sessionPickle(data, length);
  base::PickleIterator pickleIterator(sessionPickle);
  int entryCount;
  int currentEntry;

  if (!pickleIterator.ReadInt(&entryCount))
    return false;
  if (!pickleIterator.ReadInt(&currentEntry))
    return false;

  std::vector<sessions::SerializedNavigationEntry> serializedEntries;
  serializedEntries.resize(entryCount);
  for (int i = 0; i < entryCount; ++i) {
    if (!serializedEntries.at(i).ReadFromPickle(&pickleIterator))
      return false;
  }

  if (!entryCount)
    return true;

  if (!context())
    return false;

  std::vector<std::unique_ptr<content::NavigationEntry>> scopedEntries =
      sessions::ContentSerializedNavigationBuilder::ToNavigationEntries(
          serializedEntries, context()->browser_context());

  NavigationController& navigationController = web_contents_->GetController();

  if (currentEntry < 0)
    currentEntry = 0;

  if (currentEntry >= static_cast<int>(scopedEntries.size()))
    currentEntry = scopedEntries.size() - 1;

  navigationController.Restore(
      currentEntry, RestoreType::LAST_SESSION_EXITED_CLEANLY, &scopedEntries);
  return true;
}

void EWebView::SetBrowserFont() {
  RenderViewHost* render_view_host = web_contents_->GetRenderViewHost();
  if (render_view_host) {
    IPC::Message* message =
        new EwkViewMsg_SetBrowserFont(render_view_host->GetRoutingID());

    if (render_view_host->IsRenderViewLive())
      render_view_host->Send(message);
    else
      delayed_messages_.push_back(message);
  }
}

bool EWebView::IsDragging() const {
  return GetWebContentsViewEfl()->IsDragging();
}

bool EWebView::SetMainFrameScrollbarVisible(bool visible) {
  RenderViewHost* const render_view_host = web_contents_->GetRenderViewHost();
  if (!render_view_host)
    return false;

  IPC::Message* message = new EwkViewMsg_SetMainFrameScrollbarVisible(
      render_view_host->GetRoutingID(), visible);

  if (render_view_host->IsRenderViewLive())
    return render_view_host->Send(message);

  delayed_messages_.push_back(message);
  return true;
}

bool EWebView::GetMainFrameScrollbarVisible(
    Ewk_View_Main_Frame_Scrollbar_Visible_Get_Callback callback,
    void* user_data) {
  RenderViewHost* const render_view_host = web_contents_->GetRenderViewHost();
  if (!render_view_host)
    return false;

  MainFrameScrollbarVisibleGetCallback* callback_ptr =
      new MainFrameScrollbarVisibleGetCallback;
  callback_ptr->Set(callback, user_data);
  int callback_id =
      main_frame_scrollbar_visible_callback_map_.Add(callback_ptr);
  return render_view_host->Send(new EwkViewMsg_GetMainFrameScrollbarVisible(
      render_view_host->GetRoutingID(), callback_id));
}

void EWebView::InvokeMainFrameScrollbarVisibleCallback(bool visible,
                                                       int callback_id) {
  if (!BrowserThread::CurrentlyOn(BrowserThread::UI)) {
    BrowserThread::PostTask(
        BrowserThread::UI, FROM_HERE,
        base::Bind(&EWebView::InvokeMainFrameScrollbarVisibleCallback,
                   base::Unretained(this), visible, callback_id));
    return;
  }

  MainFrameScrollbarVisibleGetCallback* callback =
      main_frame_scrollbar_visible_callback_map_.Lookup(callback_id);
  if (!callback)
    return;

  callback->Run(evas_object(), visible);
  main_frame_scrollbar_visible_callback_map_.Remove(callback_id);
}

void EWebView::ShowFileChooser(content::RenderFrameHost* render_frame_host,
                               const content::FileChooserParams& params) {
#if defined(OS_TIZEN_TV_PRODUCT)
  SmartCallback<EWebViewCallbacks::FileChooserRequest>().call(nullptr);
  render_frame_host->FilesSelectedInChooser(
      std::vector<content::FileChooserFileInfo>(), params.mode);
#else
  file_chooser_.reset(
      new content::FileChooserControllerEfl(render_frame_host, &params));
  file_chooser_->Open();
#endif
}

void EWebView::SetViewMode(blink::WebViewMode view_mode) {
  RenderViewHost* render_view_host = web_contents_->GetRenderViewHost();
  if (!render_view_host)
    return;

  IPC::Message* message =
      new ViewMsg_SetViewMode(render_view_host->GetRoutingID(), view_mode);
  if (render_view_host->IsRenderViewLive()) {
    render_view_host->Send(message);
  } else {
    delayed_messages_.push_back(message);
  }
}

gfx::Point EWebView::GetContextMenuPosition() const {
  return context_menu_position_;
}

void EWebView::ShowContentsDetectedPopup(const char* message) {
  popup_controller_.reset(new PopupControllerEfl(this));
  popup_controller_->openPopup(message);
}

void EWebView::RequestColorPicker(int r, int g, int b, int a) {
  input_picker_.reset(new InputPicker(this));
  input_picker_->ShowColorPicker(r, g, b, a);
}

bool EWebView::SetColorPickerColor(int r, int g, int b, int a) {
  web_contents_->DidChooseColorInColorChooser(SkColorSetARGB(a, r, g, b));
  return true;
}

void EWebView::InputPickerShow(ui::TextInputType input_type,
                               double input_value) {
  input_picker_.reset(new InputPicker(this));
  input_picker_->ShowDatePicker(input_type, input_value);
}

void EWebView::LoadNotFoundErrorPage(const std::string& invalidUrl) {
  RenderFrameHost* render_frame_host = web_contents_->GetMainFrame();
  if (render_frame_host)
    render_frame_host->Send(new EwkFrameMsg_LoadNotFoundErrorPage(
        render_frame_host->GetRoutingID(), invalidUrl));
}
/* LCOV_EXCL_STOP */

std::string EWebView::GetPlatformLocale() {
  char* local_default = setlocale(LC_CTYPE, 0);
  if (!local_default)
    return std::string("en-US");  // LCOV_EXCL_LINE
  std::string locale = std::string(local_default);
  size_t position = locale.find('_');
  if (position != std::string::npos)
    locale.replace(position, 1, "-");
  position = locale.find('.');
  if (position != std::string::npos)
    locale = locale.substr(0, position);
  return locale;
}

int EWebView::StartInspectorServer(int port) {  // LCOV_EXCL_LINE
#if defined(OS_TIZEN_TV_PRODUCT)
  if (IsTIZENWRT()) {
    use_early_rwi_ = true;
    rwi_info_showed_ = false;
  }
  if (!context_->GetImpl()->GetInspectorServerState()) {
    int validPort = 0;
    if (!devtools_http_handler::DevToolsPortManager::GetInstance()
             ->GetValidPort(validPort))
      return 0;

    port = validPort;
  }
#endif

  return context_->InspectorServerStart(port);  // LCOV_EXCL_LINE
}

/* LCOV_EXCL_START */
bool EWebView::StopInspectorServer() {
#if defined(OS_TIZEN_TV_PRODUCT)
  if (IsTIZENWRT()) {
    use_early_rwi_ = false;
    rwi_info_showed_ = false;
  }
#endif
  return context_->InspectorServerStop();
}

void EWebView::InvokeWebProcessCrashedCallback() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  const GURL last_url = GetURL();
  bool callback_handled = false;
  SmartCallback<EWebViewCallbacks::TooltipTextUnset>().call();
  SmartCallback<EWebViewCallbacks::WebProcessCrashed>().call(&callback_handled);
  if (!callback_handled)
    LoadHTMLString(kRendererCrashedHTMLMessage, NULL,
                   last_url.possibly_invalid_spec().c_str());
}

void EWebView::HandleRendererProcessCrash() {
  BrowserThread::PostTask(BrowserThread::UI, FROM_HERE,
                          base::Bind(&EWebView::InvokeWebProcessCrashedCallback,
                                     base::Unretained(this)));
}
/* LCOV_EXCL_STOP */

#if defined(OS_TIZEN_TV_PRODUCT)
void EWebView::InitInspectorServer() {
  if (devtools_http_handler::DevToolsPortManager::GetInstance()
          ->ProcessCompare()) {
    int res = StartInspectorServer(0);
    if (res)
      devtools_http_handler::DevToolsPortManager::GetInstance()->SetPort(res);
  }
}
#endif

void EWebView::InitializeContent() {
#if defined(OS_TIZEN_TV_PRODUCT)
  // When initialize content init inspector server
  InitInspectorServer();
#endif
  int width, height;
  evas_object_geometry_get(evas_object_, 0, 0, &width, &height);

  WebContents* new_contents = create_new_window_web_contents_cb_.Run(this);
  if (!new_contents) {
    WebContents::CreateParams params(context_->browser_context());
    params.initial_size = gfx::Size(width, height);
    web_contents_.reset(
        new WebContentsImplEfl(context_->browser_context(), this));
    static_cast<WebContentsImpl*>(web_contents_.get())->Init(params);
    VLOG(1) << "Initial WebContents size: " << params.initial_size.ToString();
  } else {
    web_contents_.reset(new_contents);  // LCOV_EXCL_LINE

    // When a new webview is created in response to a request from the
    // engine, the BrowserContext instance of the originator WebContents
    // is used by the newly created WebContents object.
    // See more in WebContentsImplEfl::HandleNewWebContentsCreate.
    //
    // Hence, if as part of the WebView creation, the embedding APP
    // passes in a Ewk_Context instance that wraps a different instance of
    // BrowserContext than the one the originator WebContents holds,
    // undefined behavior can be seen.
    //
    // This is a snippet code that illustrate the scenario:
    //
    // (..)
    // evas_object_smart_callback_add(web_view_, "create,window",
    //                                &OnNewWindowRequest, this);
    // (..)
    //
    // void OnNewWindowRequest(void *data, Evas_Object*, void* out_view) {
    //   (..)
    //   EvasObject* new_web_view = ewk_view_add_with_context(GetEvas(),
    //   ewk_context_new());
    //   *static_cast<Evas_Object**>(out_view) = new_web_view;
    //   (..)
    // }
    //
    // The new Ewk_Context object created and passed in as parameter to
    // ewk_view_add_with_context wraps a different instance of BrowserContext
    // than the one the new WebContents object will hold.
    //
    // CHECK below aims at catching misuse of this API.
    bool should_crash =
        context_->GetImpl()->browser_context() !=  // LCOV_EXCL_LINE
        web_contents_->GetBrowserContext();        // LCOV_EXCL_LINE
    if (should_crash) {                            // LCOV_EXCL_LINE
      CHECK(false)
          << "BrowserContext of new WebContents does not match EWebView's. "
          << "Please see 'ewk_view_add*' documentation. "
          << "Aborting execution ...";  // LCOV_EXCL_LINE
    }
  }
  web_contents_delegate_.reset(new WebContentsDelegateEfl(this));
  web_contents_->SetDelegate(web_contents_delegate_.get());
  GetWebContentsViewEfl()->SetEflDelegate(
      new WebContentsViewEflDelegateEwk(this));
  WebContentsImplEfl* wc_efl =
      static_cast<WebContentsImplEfl*>(web_contents_.get());
  wc_efl->SetEflDelegate(new WebContentsEflDelegateEwk(this));
  back_forward_list_.reset(
      new _Ewk_Back_Forward_List(web_contents_->GetController()));

  permission_popup_manager_.reset(new PermissionPopupManager(evas_object_));
  gin_native_bridge_dispatcher_host_.reset(
      new content::GinNativeBridgeDispatcherHost(web_contents_.get()));

  native_view_ = static_cast<Evas_Object*>(web_contents_->GetNativeView());
  evas_object_smart_member_add(native_view_, evas_object_);
}

/* LCOV_EXCL_START */
void EWebView::UrlRequestSet(
    const char* url,
    content::NavigationController::LoadURLType loadtype,
    Eina_Hash* headers,
    const char* body) {
  content::NavigationController::LoadURLParams params =
      content::NavigationController::LoadURLParams(GURL(url));
  params.load_type = loadtype;
  params.override_user_agent = NavigationController::UA_OVERRIDE_TRUE;

  if (body) {
    std::string s(body);
    params.post_data =
        content::ResourceRequestBody::CreateFromBytes(s.data(), s.size());
  }

  net::HttpRequestHeaders header;
  if (headers) {
    Eina_Iterator* it = eina_hash_iterator_tuple_new(headers);
    Eina_Hash_Tuple* t;
    while (eina_iterator_next(it, reinterpret_cast<void**>(&t))) {
      if (t->key) {
        const char* value_str =
            t->data ? static_cast<const char*>(t->data) : "";
        base::StringPiece name = static_cast<const char*>(t->key);
        base::StringPiece value = value_str;
        header.SetHeader(name, value);
        // net::HttpRequestHeaders.ToString() returns string with newline
        params.extra_headers += header.ToString();
      }
    }
    eina_iterator_free(it);
  }

  web_contents_->GetController().LoadURLWithParams(params);
}

#if defined(TIZEN_VIDEO_HOLE)
void EWebView::SetVideoHoleSupport(bool enable) {
  GetSettings()->getPreferences().video_hole_enabled = enable;
  UpdateWebKitPreferences();

  RenderViewHost* render_view_host = web_contents_->GetRenderViewHost();

  if (!render_view_host)
    return;

  IPC::Message* message =
      new EwkViewMsg_SetVideoHole(render_view_host->GetRoutingID(), enable);

  if (render_view_host->IsRenderViewLive())
    render_view_host->Send(message);
  else
    delayed_messages_.push_back(message);
}
#endif
bool EWebView::HandleShow() {
  if (!native_view_)
    return false;

  Show();
  return true;
}

bool EWebView::HandleHide() {
  if (!native_view_)
    return false;

  Hide();
  return true;
}
/* LCOV_EXCL_STOP */

bool EWebView::HandleMove(int x, int y) {
  if (!native_view_)
    return false;
  evas_object_move(native_view_, x, y);

  if (context_menu_)
    context_menu_->Move(x, y);  // LCOV_EXCL_LINE

  auto rwhv = static_cast<content::RenderWidgetHostViewEfl*>(
      web_contents_.get()->GetRenderWidgetHostView());
  if (rwhv)
    rwhv->DidMoveWebView();

  return true;
}

bool EWebView::HandleResize(int width, int height) {
  if (!native_view_)
    return false;
  evas_object_resize(native_view_, width, height);

  if (select_picker_) {
    AdjustViewPortHeightToPopupMenu(
        true /* is_popup_menu_visible */);  // LCOV_EXCL_LINE
    ScrollFocusedNodeIntoView();            // LCOV_EXCL_LINE
  }

  return true;
}

/* LCOV_EXCL_START */
bool EWebView::HandleTextSelectionDown(int x, int y) {
  if (!GetSelectionController())
    return false;
  return GetSelectionController()->TextSelectionDown(x, y);
}

bool EWebView::HandleTextSelectionUp(int x, int y) {
  if (!GetSelectionController())
    return false;
  return GetSelectionController()->TextSelectionUp(x, y);
}

void EWebView::HandleTapGestureForSelection(bool is_content_editable) {
  if (!GetSelectionController())
    return;

  GetSelectionController()->PostHandleTapGesture(is_content_editable);
}

void EWebView::HandleZoomGesture(blink::WebGestureEvent& event) {
  blink::WebInputEvent::Type event_type = event.GetType();
  if (event_type == blink::WebInputEvent::kGestureDoubleTap ||
      event_type == blink::WebInputEvent::kGesturePinchBegin) {
    SmartCallback<EWebViewCallbacks::ZoomStarted>().call();
  }
  if (event_type == blink::WebInputEvent::kGestureDoubleTap ||
      event_type == blink::WebInputEvent::kGesturePinchEnd) {
    SmartCallback<EWebViewCallbacks::ZoomFinished>().call();
  }
}

bool EWebView::GetHorizontalPanningHold() const {
  if (!rwhv())
    return false;
  return rwhv()->GetHorizontalPanningHold();
}

void EWebView::SetHorizontalPanningHold(bool hold) {
  if (rwhv())
    rwhv()->SetHorizontalPanningHold(hold);
}

bool EWebView::GetVerticalPanningHold() const {
  if (!rwhv())
    return false;
  return rwhv()->GetVerticalPanningHold();
}

void EWebView::SetVerticalPanningHold(bool hold) {
  if (rwhv())
    rwhv()->SetVerticalPanningHold(hold);
}

void EWebView::SendDelayedMessages(RenderViewHost* render_view_host) {
  DCHECK(render_view_host);

  if (!BrowserThread::CurrentlyOn(BrowserThread::UI)) {
    BrowserThread::PostTask(
        BrowserThread::UI, FROM_HERE,
        base::Bind(&EWebView::SendDelayedMessages, base::Unretained(this),
                   render_view_host));
    return;
  }

  for (auto iter = delayed_messages_.begin(); iter != delayed_messages_.end();
       ++iter) {
    IPC::Message* message = *iter;
    message->set_routing_id(render_view_host->GetRoutingID());
    render_view_host->Send(message);
  }

  delayed_messages_.clear();
}

void EWebView::ClosePage() {
  web_contents_->ClosePage();
}

void EWebView::SetSessionTimeout(unsigned long timeout) {
  RenderViewHost* render_view_host = web_contents_->GetRenderViewHost();
  IPC::Message* message =
      new EwkViewMsg_SetLongPollingGlobalTimeout(
          render_view_host->GetRoutingID(), timeout);

  if (render_view_host->IsRenderViewLive()) {
    render_view_host->Send(message);
  } else {
    delayed_messages_.push_back(message);
  }
}

void EWebView::RequestManifest(Ewk_View_Request_Manifest_Callback callback,
                               void* user_data) {
  web_contents_delegate_->RequestManifestInfo(callback, user_data);
}

void EWebView::DidRespondRequestManifest(
    _Ewk_View_Request_Manifest* manifest,
    Ewk_View_Request_Manifest_Callback callback,
    void* user_data) {
  callback(evas_object_, manifest, user_data);
}

void EWebView::SetBeforeUnloadConfirmPanelCallback(
    Ewk_View_Before_Unload_Confirm_Panel_Callback callback,
    void* user_data) {
  GetJavaScriptDialogManagerEfl()->SetBeforeUnloadConfirmPanelCallback(
      callback, user_data);
}

void EWebView::ReplyBeforeUnloadConfirmPanel(Eina_Bool result) {
  GetJavaScriptDialogManagerEfl()->ReplyBeforeUnloadConfirmPanel(result);
}
/* LCOV_EXCL_STOP */

bool EWebView::SetVisibility(bool enable) {
  if (!web_contents_)
    return false;

  if (enable)
    web_contents_->WasShown();
  else
    web_contents_->WasHidden();  // LCOV_EXCL_LINE

  return true;
}

/* LCOV_EXCL_START */
bool EWebView::IsVideoPlaying(Ewk_Is_Video_Playing_Callback callback,
                              void* user_data) {
  RenderViewHost* rvh = web_contents_->GetRenderViewHost();
  if (!rvh) {
    return false;
  }
  IsVideoPlayingCallback* cb = new IsVideoPlayingCallback(callback, user_data);
  int callback_id = is_video_playing_callback_map_.Add(cb);
  return rvh->Send(
      new EwkViewMsg_IsVideoPlaying(rvh->GetRoutingID(), callback_id));
}

void EWebView::InvokeIsVideoPlayingCallback(bool is_playing, int callback_id) {
  IsVideoPlayingCallback* callback =
      is_video_playing_callback_map_.Lookup(callback_id);
  if (!callback)
    return;
  callback->Run(evas_object(), is_playing);
  is_video_playing_callback_map_.Remove(callback_id);
}
/* LCOV_EXCL_STOP */

#if defined(OS_TIZEN_TV_PRODUCT)
void EWebView::SetMarlinEnable(bool is_enable) {
  LOG(INFO) << "is_marlin_enable: " << (is_enable ? "true" : "false");
  is_marlin_enable_ = is_enable;
}

bool EWebView::GetMarlinEnable() {
  return is_marlin_enable_;
}

void EWebView::SetActiveDRM(const char* drm_system_id) {
  active_drm_ = drm_system_id ? drm_system_id : "";
  LOG(INFO) << "active DRM: \"" << active_drm_ << "\"";
}

const std::string& EWebView::GetActiveDRM() const {
  return active_drm_;
}

std::vector<std::string> EWebView::NotifyPlaybackState(int state,
                                                       const char* url,
                                                       const char* mime_type) {
  std::vector<std::string> data;
  Ewk_Media_Playback_Info* playback_info =
      ewkMediaPlaybackInfoCreate(url, mime_type);

  LOG(INFO) << "state: " << state
            << "(0-load : 1-ready : 2-start : 3-finish : 4-stop)";
  switch (state) {
    case blink::WebMediaPlayer::kPlaybackLoad:
      SmartCallback<EWebViewCallbacks::PlaybackLoad>().call(
          static_cast<void*>(playback_info));
      break;
    case blink::WebMediaPlayer::kPlaybackReady:
      SmartCallback<EWebViewCallbacks::PlaybackReady>().call(
          static_cast<void*>(playback_info));
      break;
    case blink::WebMediaPlayer::kPlaybackStart:
      SmartCallback<EWebViewCallbacks::PlaybackStart>().call();
      break;
    case blink::WebMediaPlayer::kPlaybackFinish:
      SmartCallback<EWebViewCallbacks::PlaybackFinish>().call();
      break;
    case blink::WebMediaPlayer::kPlaybackStop:
      SmartCallback<EWebViewCallbacks::PlaybackStop>().call(
          static_cast<void*>(playback_info));
      break;
    default:
      NOTREACHED();
      data.push_back("");
      ewkMediaPlaybackInfoDelete(playback_info);
      return data;
  }

  bool media_resource_acquired =
      ewk_media_playback_info_media_resource_acquired_get(playback_info)
          ? true
          : false;
  data.push_back(media_resource_acquired ? "mediaResourceAcquired" : "");
  const char* translated_url =
      ewk_media_playback_info_translated_url_get(playback_info);
  data.push_back(translated_url ? std::string(translated_url) : "");
  const char* drm_info = ewk_media_playback_info_drm_info_get(playback_info);
  data.push_back(drm_info ? std::string(drm_info) : "");

  LOG(INFO) << "evasObject: " << native_view_
            << ", media_resource_acquired :" << media_resource_acquired
            << ", translated_url:" << translated_url
            << ", drm_info:" << drm_info;
  ewkMediaPlaybackInfoDelete(playback_info);
  return data;
}

void EWebView::NotifySubtitleState(int state, double time_stamp) {
  LOG(INFO) << "subtitle state: " << state
            << ",(0-Play : 1-Pause : 2-SeekStart : 3-SeekComplete : 4-Stop "
               ":5-Resume)";
  switch (state) {
    case blink::WebMediaPlayer::kSubtitlePause:
      SmartCallback<EWebViewCallbacks::SubtitlePause>().call();
      break;
    case blink::WebMediaPlayer::kSubtitleStop:
      SmartCallback<EWebViewCallbacks::SubtitleStop>().call();
      break;
    case blink::WebMediaPlayer::kSubtitleResume:
      SmartCallback<EWebViewCallbacks::SubtitleResume>().call();
      break;
    case blink::WebMediaPlayer::kSubtitleSeekStart: {
      double ts = time_stamp;
      SmartCallback<EWebViewCallbacks::SubtitleSeekStart>().call(&ts);
    } break;
    case blink::WebMediaPlayer::kSubtitleSeekComplete:
      SmartCallback<EWebViewCallbacks::SubtitleSeekComplete>().call();
      break;
    default:
      NOTREACHED();
      break;
  }
}

void EWebView::NotifySubtitlePlay(int active_track_id,
                                  const char* url,
                                  const char* lang) {
  LOG(INFO) << "id:" << active_track_id << ",url:" << url << ",lang:" << lang;
  Ewk_Media_Subtitle_Info* subtitle_info =
      ewkMediaSubtitleInfoCreate(active_track_id, url, lang, 0);
  SmartCallback<EWebViewCallbacks::SubtitlePlay>().call(
      static_cast<void*>(subtitle_info));
  ewkMediaSubtitleInfoDelete(subtitle_info);
}

void EWebView::NotifySubtitleData(int track_id,
                                  double time_stamp,
                                  const std::string& data,
                                  unsigned int size) {
  const void* buffer = static_cast<const void*>(data.c_str());
  Ewk_Media_Subtitle_Data* subtitle_data =
      ewkMediaSubtitleDataCreate(track_id, time_stamp, buffer, size);
  SmartCallback<EWebViewCallbacks::SubtitleNotifyData>().call(
      static_cast<void*>(subtitle_data));
  ewkMediaSubtitleDataDelete(subtitle_data);
}

void EWebView::NotifyParentalRatingInfo(const char* info, const char* url) {
  LOG(INFO) << "info:" << info << ",url:" << url;
  Ewk_Media_Parental_Rating_Info* data =
      ewkMediaParentalRatingInfoCreate(info, url);
  SmartCallback<EWebViewCallbacks::ParentalRatingInfo>().call(
      static_cast<void*>(data));
  ewkMediaParentalRatingInfoDelete(data);
}

void EWebView::UpdateCurrentTime(double current_time) {
  current_time_ = current_time;
}

bool EWebView::SetTranslatedURL(const char* url) {
  DCHECK(web_contents_);

  bool translated = false;
  RenderViewHost* render_view_host = web_contents_->GetRenderViewHost();
  if (render_view_host)
    render_view_host->Send(new EwkViewMsg_TranslatedUrlSet(
        render_view_host->GetRoutingID(), std::string(url), &translated));
  LOG(INFO) << "evasObject: " << native_view_ << ", url:" << url;

  return translated;
}

void EWebView::SetSubtitleLang(const char* lang_list) {
  LOG(INFO) << "SetSubtitleLang: " << lang_list;
  const std::string lang(lang_list ? lang_list : "");
  RenderViewHost* render_view_host = web_contents_->GetRenderViewHost();
  if (render_view_host)
    render_view_host->Send(new EwkViewMsg_SetPreferTextLang(
        render_view_host->GetRoutingID(), lang));
}

void EWebView::SetHighBitRate(Eina_Bool is_high_bitrate) {
  LOG(INFO) << "is_high_bitrate: " << (is_high_bitrate ? "true" : "false");
  is_high_bitrate_ = is_high_bitrate;
}

bool EWebView::IsHighBitRate() {
  return is_high_bitrate_;
}

void EWebView::SetParentalRatingResult(const char* url, bool is_pass) {
  CHECK(web_contents_);

  LOG(INFO) << "SetParentalRatingResult,url:" << url
            << ",pass:" << (is_pass ? "true" : "false");
  RenderViewHost* render_view_host = web_contents_->GetRenderViewHost();
  if (render_view_host)
    render_view_host->Send(new EwkViewMsg_ParentalRatingResultSet(
        render_view_host->GetRoutingID(), std::string(url), is_pass));
}

void EWebView::ClearAllTilesResources() {
  if (rwhv())
    rwhv()->ClearAllTilesResources();
}

void EWebView::GetMousePosition(gfx::Point& mouse_position) {
  int mouse_x, mouse_y;
  evas_pointer_output_xy_get(GetEvas(), &mouse_x, &mouse_y);
  Evas_Coord x, y, width, height;
  evas_object_geometry_get(evas_object(), &x, &y, &width, &height);

  gfx::Rect frame_rect(x, y, width, height);
  if (!frame_rect.Contains(mouse_x, mouse_y)) {
    mouse_x = frame_rect.x();
    mouse_y = frame_rect.y();
  }
  mouse_x -= x;
  mouse_y -= y;

  mouse_x /=
      display::Screen::GetScreen()->GetPrimaryDisplay().device_scale_factor();
  mouse_y /=
      display::Screen::GetScreen()->GetPrimaryDisplay().device_scale_factor();

  mouse_position.set_x(mouse_x);
  mouse_position.set_y(mouse_y);
}

bool EWebView::EdgeScrollBy(int delta_x, int delta_y) {
  if ((delta_x == 0 && delta_y == 0) || is_processing_edge_scroll_)
    return false;

  RenderViewHost* render_view_host = web_contents_->GetRenderViewHost();
  if (!render_view_host)
    return false;

  RenderWidgetHostViewEfl* view = static_cast<RenderWidgetHostViewEfl*>(
      render_view_host->GetWidget()->GetView());
  if (!view)
    return false;

  gfx::Point offset = gfx::Point(delta_x, delta_y);

  gfx::Point mouse_position;
  GetMousePosition(mouse_position);

  is_processing_edge_scroll_ = true;
  return render_view_host->Send(new EwkViewMsg_EdgeScrollBy(
      render_view_host->GetRoutingID(), offset, mouse_position));
}

void EWebView::InvokeEdgeScrollByCallback(const gfx::Point& offset,
                                          bool handled) {
  is_processing_edge_scroll_ = false;
  if (offset.x() < 0)
    SmartCallback<EWebViewCallbacks::EdgeScrollLeft>().call(&handled);
  else if (offset.x() > 0)
    SmartCallback<EWebViewCallbacks::EdgeScrollRight>().call(&handled);

  if (offset.y() < 0)
    SmartCallback<EWebViewCallbacks::EdgeScrollTop>().call(&handled);
  else if (offset.y() > 0)
    SmartCallback<EWebViewCallbacks::EdgeScrollBottom>().call(&handled);
}

void EWebView::InvokeRunArrowScrollCallback() {
  SmartCallback<EWebViewCallbacks::RunArrowScroll>().call();
}

void EWebView::SendKeyEvent(Evas_Object* ewk_view,
                            void* key_event,
                            bool is_press) {
  if (rwhv())
    rwhv()->SendKeyEvent(ewk_view, key_event, is_press);
}

void EWebView::SetKeyEventsEnabled(bool enabled) {
  if (rwhv())
    rwhv()->SetKeyEventsEnabled(enabled);
}

void EWebView::SetCursorByClient(bool enable) {
  if (rwhv())
    rwhv()->SetCursorByClient(enable);
}

void EWebView::AddItemToBackForwardList(
    const _Ewk_Back_Forward_List_Item* item) {
  content::NavigationController& controller = web_contents_->GetController();
  int previous_entry_index = controller.GetLastCommittedEntryIndex();
  controller.InsertEntry(item->GetURL(), item->GetOriginalURL(),
                         item->GetTitle());

  if (controller.GetLastCommittedEntry())
    back_forward_list_->NewPageCommited(previous_entry_index,
                                        controller.GetLastCommittedEntry());
  InvokeBackForwardListChangedCallback();
}

void EWebView::OnMouseLongPressed(int x, int y) {
  RenderViewHost* render_view_host = web_contents_->GetRenderViewHost();
  if (render_view_host) {
    render_view_host->Send(new EwkViewMsg_RequestHitTestForMouseLongPress(
        render_view_host->GetRoutingID(),
        render_view_host->GetProcess()->GetID(), x, y));
  }
}

void EWebView::OnMouseDown(int x, int y) {
  mouse_long_press_timer_.Start(
      FROM_HERE, base::TimeDelta::FromMilliseconds(kMousePressAndHoldTimeout),
      base::Bind(&EWebView::OnMouseLongPressed, base::Unretained(this), x, y));
}

bool EWebView::OnMouseUp() {
  mouse_long_press_timer_.Stop();

  if (context_menu_show_) {
    context_menu_show_ = false;
    return true;
  }
  return false;
}

void EWebView::OnMouseMove() {
  mouse_long_press_timer_.Stop();
}

void EWebView::ShowContextMenuWithLongPressed(const GURL& url, int x, int y) {
  content::ContextMenuParams convertedParams;
  convertedParams.x = x;
  convertedParams.y = y;
  convertedParams.link_url = url;
  context_menu_.reset(
      new content::ContextMenuControllerEfl(this, *web_contents_.get()));
  if (context_menu_->PopulateAndShowContextMenu(convertedParams))
    context_menu_show_ = true;
  else
    context_menu_.reset();
}

bool EWebView::SetMixedContents(bool allow) {
  MixedContentObserver* mixed_content_observer =
      MixedContentObserver::FromWebContents(web_contents_.get());
  return mixed_content_observer->MixedContentReply(allow);
}

Eina_Bool EWebView::SetTTSMode(tts_mode_e mode) {
  LOG(INFO) << "SetTTSMode: " << (int)mode;
  if (content::GetContentClientExport()) {
    auto cbce = static_cast<ContentBrowserClientEfl*>(
        content::GetContentClientExport()->browser());
    if (cbce) {
      cbce->SetTTSMode(mode);
      return EINA_TRUE;
    }
  }
  return EINA_FALSE;
}

void EWebView::AddDynamicCertificatePath(const std::string& host,
                                         const std::string& cert_path) {
  web_contents_->AddDynamicCertificatePath(host, cert_path);
}

void EWebView::ExecuteJSInFrame(base::string16 script,
                                Ewk_View_Script_Execute_Callback callback,
                                void* userdata,
                                RenderFrameHost* render_frame_host) {
  if (!render_frame_host)
    return;

  if (callback) {
    JavaScriptCallbackDetails* script_callback_data =
        new JavaScriptCallbackDetails(callback, userdata, evas_object_);
    render_frame_host->ExecuteJavaScriptWithUserGestureForTests(
        script,
        base::Bind(&JavaScriptComplete, base::Owned(script_callback_data)));
  } else {
    render_frame_host->ExecuteJavaScriptWithUserGestureForTests(script);
  }
}

bool EWebView::ExecuteJavaScriptInAllFrames(
    const char* script,
    Ewk_View_Script_Execute_Callback callback,
    void* userdata) {
  if (!web_contents_)
    return false;

  base::string16 js_script;
  base::UTF8ToUTF16(script, strlen(script), &js_script);
  web_contents_->ForEachFrame(base::Bind(&EWebView::ExecuteJSInFrame,
                                         base::Unretained(this), js_script,
                                         callback, userdata));

  return true;
}

void EWebView::SetFloatWindowState(bool enabled) {
  if (!web_contents_)
    return;

  RenderViewHost* render_view_host = web_contents_->GetRenderViewHost();
  if (!render_view_host)
    return;

  IPC::Message* message = new EwkViewMsg_SetFloatVideoWindowState(
      render_view_host->GetRoutingID(), enabled);
  if (render_view_host->IsRenderViewLive())
    render_view_host->Send(message);
  else
    delayed_messages_.push_back(message);
}

void EWebView::AutoLogin(const char* user_name, const char* password) {
  EINA_SAFETY_ON_NULL_RETURN(user_name);
  EINA_SAFETY_ON_NULL_RETURN(password);
  if (strlen(user_name) == 0)
    return;

  if (web_contents_delegate_)
    web_contents_delegate_->AutoLogin(user_name, password);
}
#endif

#if defined(TIZEN_ATK_FEATURE_VD)
void EWebView::MoveFocusToBrowser(int direction) {
  SmartCallback<EWebViewCallbacks::AtkKeyEventNotHandled>().call(&direction);
}

void EWebView::DeactivateAtk(bool deactivated) {
  EWebAccessibilityUtil::GetInstance()->Deactivate(deactivated);
}
#endif

/* LCOV_EXCL_START */
void EWebView::SetDoNotTrack(Eina_Bool enable) {
  // enable: 0 User tend to allow tracking on the target site.
  // enable: 1 User tend to not be tracked on the target site.

  if (web_contents_->GetMutableRendererPrefs()->enable_do_not_track ==
      enable)
    return;

  // Set navigator.doNotTrack attribute
  web_contents_->GetMutableRendererPrefs()->enable_do_not_track = enable;
  web_contents_->GetRenderViewHost()->SyncRendererPrefs();

  // Set or remove DNT HTTP header, the effects will depend on design of target
  // site.
  if (!context())
    return;

  if (enable)
    context()->HTTPCustomHeaderAdd("DNT", "1");
  else
    context()->HTTPCustomHeaderRemove("DNT");
}
/* LCOV_EXCL_STOP */

#if defined(TIZEN_PEPPER_EXTENSIONS)
void EWebView::InitializePepperExtensionSystem() {
  RegisterPepperExtensionDelegate();
  SetWindowId();
}

EwkExtensionSystemDelegate* EWebView::GetExtensionDelegate() {
  RenderFrameHost* render_frame_host = web_contents_->GetMainFrame();
  if (!render_frame_host)
    return nullptr;

  ExtensionSystemDelegateManager::RenderFrameID id;
  id.render_process_id = render_frame_host->GetProcess()->GetID();
  id.render_frame_id = render_frame_host->GetRoutingID();
  return static_cast<EwkExtensionSystemDelegate*>(
      ExtensionSystemDelegateManager::GetInstance()->GetDelegateForFrame(id));
}

void EWebView::SetWindowId() {
  EwkExtensionSystemDelegate* delegate = GetExtensionDelegate();
  if (!delegate) {
    LOG(WARNING) << "No delegate is available to set window id";
    return;
  }
  Evas_Object* main_wind =
      efl::WindowFactory::GetHostWindow(web_contents_.get());
  if (!main_wind) {
    LOG(ERROR) << "Can`t get main window";
    return;
  }
  delegate->SetWindowId(main_wind);
}

void EWebView::SetPepperExtensionWidgetInfo(Ewk_Value widget_pepper_ext_info) {
  EwkExtensionSystemDelegate* delegate = GetExtensionDelegate();
  if (!delegate) {
    LOG(WARNING) << "No delegate is available to set extension info";
    return;
  }
  delegate->SetExtensionInfo(widget_pepper_ext_info);
}

void EWebView::SetPepperExtensionCallback(Generic_Sync_Call_Callback cb,
                                          void* data) {
  EwkExtensionSystemDelegate* delegate = GetExtensionDelegate();
  if (!delegate) {
    LOG(WARNING) << "No delegate is available to set generic callback";
    return;
  }
  delegate->SetGenericSyncCallback(cb, data);
}

void EWebView::RegisterPepperExtensionDelegate() {
  RenderFrameHost* render_frame_host = web_contents_->GetMainFrame();
  if (!render_frame_host) {
    LOG(WARNING) << "render_frame_host is nullptr, can't register delegate";
    return;
  }

  ExtensionSystemDelegateManager::RenderFrameID id;
  id.render_process_id = render_frame_host->GetProcess()->GetID();
  id.render_frame_id = render_frame_host->GetRoutingID();

  EwkExtensionSystemDelegate* delegate = new EwkExtensionSystemDelegate;
  ExtensionSystemDelegateManager::GetInstance()->RegisterDelegate(
      id, std::unique_ptr<EwkExtensionSystemDelegate>{delegate});
}

void EWebView::UnregisterPepperExtensionDelegate() {
  if (!web_contents_) {
    LOG(WARNING) << "web_contents_ is nullptr, can't unregister delegate";
    return;
  }
  RenderFrameHost* render_frame_host = web_contents_->GetMainFrame();
  if (!render_frame_host) {
    LOG(WARNING) << "render_frame_host is nullptr, can't unregister delegate";
    return;
  }

  ExtensionSystemDelegateManager::RenderFrameID id;
  id.render_process_id = render_frame_host->GetProcess()->GetID();
  id.render_frame_id = render_frame_host->GetRoutingID();

  if (!ExtensionSystemDelegateManager::GetInstance()->UnregisterDelegate(id))
    LOG(WARNING) << "Unregistering pepper extension delegate failed";
}
#endif

/* LCOV_EXCL_START */
void EWebView::InvokeFormRepostWarningShowCallback(
    _Ewk_Form_Repost_Decision_Request* request) {
  SmartCallback<EWebViewCallbacks::FormRepostWarningShow>().call(request);
}

void EWebView::InvokeBeforeFormRepostWarningShowCallback(
    _Ewk_Form_Repost_Decision_Request* request) {
  SmartCallback<EWebViewCallbacks::BeforeFormRepostWarningShow>().call(request);
}

void EWebView::HwKeyboardStatusChanged(const std::string& device_name,
                                       Eina_Bool enable) {
  if (!IsMobileProfile() || !IsWearableProfile())
    return;
  auto view = rwhv();
  if (!view)
    return;

  if (device_name.find("Keyboard") != std::string::npos) {
    view->UpdateHwKeyboardStatus(enable);
#if defined(TIZEN_ATK_SUPPORT)
    view->UpdateSpatialNavigationStatus(enable);
#endif

    if (!enable && view->GetIMContext() && HasFocus() &&
        view->IsFocusedNodeEditable())
      view->GetIMContext()->ShowPanel();

    auto render_view_host = web_contents_->GetRenderViewHost();
    if (render_view_host) {
      settings_->getPreferencesEfl().hw_keyboard_connected = enable;
      UpdateWebkitPreferencesEfl(render_view_host);
    }
  }
}

void EWebView::SetDidChangeThemeColorCallback(
    Ewk_View_Did_Change_Theme_Color_Callback callback,
    void* user_data) {
  did_change_theme_color_callback_.Set(callback, user_data);
}

void EWebView::DidChangeThemeColor(const SkColor& color) {
  did_change_theme_color_callback_.Run(evas_object_, color);
}

#if defined(TIZEN_ATK_SUPPORT)
void EWebView::UpdateSpatialNavigationStatus(Eina_Bool enable) {
  if (settings_->getPreferences().spatial_navigation_enabled == enable)
    return;

  settings_->getPreferences().spatial_navigation_enabled = enable;
  auto view = rwhv();
  if (view)
    view->UpdateSpatialNavigationStatus(enable);
}

void EWebView::InitAtk() {
  if (IsDesktopProfile())
    return;

  int result = 0;
  if (vconf_get_bool(VCONFKEY_SETAPPL_ACCESSIBILITY_TTS, &result) != 0)
    LOG(ERROR)
        << "Could not read VCONFKEY_SETAPPL_ACCESSIBILITY_TTS key value.";
  else
#if defined(TIZEN_ATK_FEATURE_VD)
    EWebAccessibilityUtil::GetInstance()->ToggleBrowserAccessibility(
        result && !EWebAccessibilityUtil::GetInstance()->IsDeactivatedByApp(),
        this);
#else
    EWebAccessibilityUtil::GetInstance()->ToggleBrowserAccessibility(result,
                                                                     this);
#endif

  lazy_initalize_atk_ = false;
}

bool EWebView::GetAtkStatus() {
  auto state = content::BrowserAccessibilityStateImpl::GetInstance();
  if (!state)
    return false;
  return state->IsAccessibleBrowser();
}
/* LCOV_EXCL_STOP */
#endif

void EWebView::SetExceededIndexedDatabaseQuotaCallback(
    Ewk_View_Exceeded_Indexed_Database_Quota_Callback callback,
    void* user_data) {
  exceeded_indexed_db_quota_callback_.Set(callback, user_data);
  content::BrowserContextEfl* browser_context =
      static_cast<content::BrowserContextEfl*>(
          web_contents_->GetBrowserContext());
  if (browser_context)
    browser_context->GetSpecialStoragePolicyEfl()->SetQuotaExceededCallback(
        base::Bind(&EWebView::InvokeExceededIndexedDatabaseQuotaCallback,
                   base::Unretained(this)));
}

void EWebView::InvokeExceededIndexedDatabaseQuotaCallback(
    const GURL& origin,
    int64_t current_quota) {
  if (!BrowserThread::CurrentlyOn(BrowserThread::UI)) {
    BrowserThread::PostTask(
        BrowserThread::UI, FROM_HERE,
        base::Bind(&EWebView::InvokeExceededIndexedDatabaseQuotaCallback,
                   base::Unretained(this), origin, current_quota));
    return;
  }
  LOG(INFO) << __func__ << "()" << origin << ", " << current_quota;
  CHECK(!exceeded_indexed_db_quota_origin_.get());
  exceeded_indexed_db_quota_origin_.reset(new Ewk_Security_Origin(origin));
  exceeded_indexed_db_quota_callback_.Run(
      evas_object_, exceeded_indexed_db_quota_origin_.get(), current_quota);
}

void EWebView::ExceededIndexedDatabaseQuotaReply(bool allow) {
  if (!exceeded_indexed_db_quota_origin_.get()) {
    LOG(WARNING) << __func__ << "() : callback is not invoked!";
    return;
  }
  LOG(INFO) << __func__ << "()" << exceeded_indexed_db_quota_origin_->GetURL()
            << ", " << allow;
  content::BrowserContextEfl* browser_context =
      static_cast<content::BrowserContextEfl*>(
          web_contents_->GetBrowserContext());
  if (browser_context)
    browser_context->GetSpecialStoragePolicyEfl()->SetUnlimitedStoragePolicy(
        exceeded_indexed_db_quota_origin_->GetURL(), allow);
  exceeded_indexed_db_quota_origin_.reset();
}

#if defined(OS_TIZEN_TV_PRODUCT)
void EWebView::DrawLabel(Evas_Object* image, Eina_Rectangle rect) {
  if (rwhv())
    rwhv()->DrawLabel(image, rect);
}

void EWebView::ClearLabels() {
  if (rwhv())
    rwhv()->ClearLabels();
}

void EWebView::InvokeScrollbarThumbFocusChangedCallback(
    Ewk_Scrollbar_Orientation orientation,
    bool focused) {
  Ewk_Scrollbar_Data data;
  data.orientation = orientation;
  data.focused = focused;
  SmartCallback<EWebViewCallbacks::DidChagneScrollbarsThumbFocus>().call(&data);
}
#endif
#if defined(OS_TIZEN_TV_PRODUCT)
void EWebView::OnDialogClosed() {
  if (use_early_rwi_) {
    use_early_rwi_ = false;
    LOG(INFO) << "[FAST RWI] SetURL Restore [" << rwi_gurl_.spec()
              << "] from [about:blank]";
    SetURL(rwi_gurl_);
    rwi_gurl_ = GURL();
    rwi_info_showed_ = true;
  }
}
#endif

void EWebView::SetAppNeedEffect(Eina_Bool state) {
#if defined(OS_TIZEN)
  GetSettings()->getPreferences().mirror_blur_enabled = state;
  if (rwhv())
    rwhv()->InitBlurEffect(state);
#endif
}

void EWebView::SyncAcceptLanguages(const std::string& accept_languages) {
  web_contents_->GetMutableRendererPrefs()->accept_languages = accept_languages;
  web_contents_->GetRenderViewHost()->SyncRendererPrefs();
}

#if defined(TIZEN_TBM_SUPPORT)
void EWebView::SetOffscreenRendering(bool enable) {
  if (web_contents_delegate_)
    web_contents_delegate_->SetOffscreenRendering(enable);
}
#endif

bool EWebView::ShouldIgnoreNavigation(
    const navigation_interception::NavigationParams& params) {
  if (!params.url().is_valid() || !params.url().SchemeIs(kAppControlScheme) ||
      (!params.has_user_gesture() && !params.is_redirect()))
    return false;

  _Ewk_App_Control app_control(this, params.url().possibly_invalid_spec());
  return app_control.Proceed();
}
