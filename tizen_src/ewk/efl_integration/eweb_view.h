// Copyright 2014-2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EWEB_VIEW_H
#define EWEB_VIEW_H

#include <Ecore_Input.h>
#include <Evas.h>
#include <locale.h>
#include <map>
#include <string>
#include <vector>

#include "base/callback.h"
#include "base/containers/id_map.h"
#include "base/synchronization/waitable_event.h"
#include "browser/select_picker/select_picker_base.h"
#include "content_browser_client_efl.h"
#include "content/browser/renderer_host/event_with_latency_info.h"
#include "content/browser/selection/selection_controller_efl.h"
#include "content/common/input/input_event_ack_state.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/quota_permission_context.h"
#include "content/public/browser/web_contents_delegate.h"
#include "content/public/browser/web_contents_efl_delegate.h"
#include "content/public/common/context_menu_params.h"
#include "content/public/common/file_chooser_params.h"
#include "content_browser_client_efl.h"
#include "context_menu_controller_efl.h"
#include "eweb_context.h"
#include "eweb_view_callbacks.h"
#include "file_chooser_controller_efl.h"
#include "permission_popup_manager.h"
#include "popup_controller_efl.h"
#include "private/ewk_auth_challenge_private.h"
#include "private/ewk_back_forward_list_private.h"
#include "private/ewk_form_repost_decision_private.h"
#include "private/ewk_history_private.h"
#include "private/ewk_hit_test_private.h"
#include "private/ewk_settings_private.h"
#include "private/ewk_web_application_icon_data_private.h"
#include "public/ewk_hit_test_internal.h"
#include "public/ewk_touch_internal.h"
#include "public/ewk_view_product.h"
#include "scroll_detector.h"
#include "third_party/WebKit/public/web/WebViewModeEnums.h"
#include "ui/gfx/geometry/point.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/size.h"
#include "web_contents_delegate_efl.h"

#if defined(TIZEN_PEPPER_EXTENSIONS)
#include "ewk_extension_system_delegate.h"
#include "public/ewk_value_product.h"
#endif
#if defined(OS_TIZEN_TV_PRODUCT)
#include "public/ewk_media_playback_info.h"
#endif

namespace base {
class FilePath;
}

namespace content {
class RenderFrameHost;
class RenderViewHost;
class RenderWidgetHostViewEfl;
class WebContentsDelegateEfl;
class ContextMenuControllerEfl;
class WebContentsViewEfl;
class PopupControllerEfl;
class GinNativeBridgeDispatcherHost;
class InputPicker;
}

namespace navigation_interception {
class NavigationParams;
}

class ErrorParams;
class _Ewk_App_Control;
class _Ewk_Policy_Decision;
class _Ewk_Hit_Test;
class Ewk_Context;
class WebViewEvasEventHandler;
class _Ewk_Quota_Permission_Request;

#if defined(TIZEN_ATK_SUPPORT)
class EWebAccessibility;
#endif
#if defined(OS_TIZEN_TV_PRODUCT)
Ewk_Media_Playback_Info* ewkMediaPlaybackInfoCreate(const char* url,
                                                    const char* mime_type);
unsigned char ewk_media_playback_info_media_resource_acquired_get(
    Ewk_Media_Playback_Info* data);
const char* ewk_media_playback_info_translated_url_get(
    Ewk_Media_Playback_Info* data);
const char* ewk_media_playback_info_drm_info_get(Ewk_Media_Playback_Info* data);
void ewkMediaPlaybackInfoDelete(Ewk_Media_Playback_Info* data);
#endif

template <typename CallbackPtr, typename CallbackParameter>
class WebViewCallback {
 public:
  WebViewCallback() { Set(nullptr, nullptr); }

  void Set(CallbackPtr cb, void* data) {
    callback_ = cb;
    user_data_ = data;
  }

  bool IsCallbackSet() const { return callback_; }  // LCOV_EXCL_LINE

  /* LCOV_EXCL_START */
  Eina_Bool Run(Evas_Object* webview,
                CallbackParameter param,
                Eina_Bool* callback_result) {
    CHECK(callback_result);
    if (IsCallbackSet()) {
      *callback_result = callback_(webview, param, user_data_);
      return true;
    }
    return false;
  }

  void Run(Evas_Object* webview, CallbackParameter param) {
    if (IsCallbackSet())
      callback_(webview, param, user_data_);
  }
  /* LCOV_EXCL_STOP */

 private:
  CallbackPtr callback_;
  void* user_data_;
};

template <typename CallbackPtr, typename... CallbackParameter>
class WebViewExceededQuotaCallback {
 public:
  WebViewExceededQuotaCallback() { Set(nullptr, nullptr); }

  void Set(CallbackPtr cb, void* data) {
    callback_ = cb;
    user_data_ = data;
  }

  bool IsCallbackSet() const { return callback_; }

  /* LCOV_EXCL_START */
  void Run(Evas_Object* webview, CallbackParameter... param) {
    if (IsCallbackSet())
      callback_(webview, param..., user_data_);
  }
  /* LCOV_EXCL_STOP */

 private:
  CallbackPtr callback_;
  void* user_data_;
};

class BackgroundColorGetCallback {
 public:
  BackgroundColorGetCallback(Ewk_View_Background_Color_Get_Callback func,
                             void* user_data)
      : func_(func), user_data_(user_data) {}  // LCOV_EXCL_LINE

  /* LCOV_EXCL_START */
  void Run(Evas_Object* webview, int r, int g, int b, int a) {
    if (func_)
      func_(webview, r, g, b, a, user_data_);
  }
  /* LCOV_EXCL_STOP */
 private:
  Ewk_View_Background_Color_Get_Callback func_;
  void* user_data_;
};

class WebApplicationIconUrlGetCallback {
 public:
  WebApplicationIconUrlGetCallback(Ewk_Web_App_Icon_URL_Get_Callback func,
                                   void* user_data)
      : func_(func), user_data_(user_data) {}  // LCOV_EXCL_LINE
  /* LCOV_EXCL_START */
  void Run(const std::string& url) {
    if (func_) {
      (func_)(url.c_str(), user_data_);
    }
  }
  /* LCOV_EXCL_STOP */

 private:
  Ewk_Web_App_Icon_URL_Get_Callback func_;
  void* user_data_;
};

class WebApplicationIconUrlsGetCallback {
 public:
  WebApplicationIconUrlsGetCallback(Ewk_Web_App_Icon_URLs_Get_Callback func,
                                    void* user_data)
      : func_(func), user_data_(user_data) {}  // LCOV_EXCL_LINE
  /* LCOV_EXCL_START */
  void Run(const std::map<std::string, std::string>& urls) {
    if (func_) {
      Eina_List* list = NULL;
      for (std::map<std::string, std::string>::const_iterator it = urls.begin();
           it != urls.end(); ++it) {
        _Ewk_Web_App_Icon_Data* data =
            ewkWebAppIconDataCreate(it->first, it->second);
        list = eina_list_append(list, data);
      }
      (func_)(list, user_data_);
    }
  }
  /* LCOV_EXCL_STOP */

 private:
  Ewk_Web_App_Icon_URLs_Get_Callback func_;
  void* user_data_;
};

class WebApplicationCapableGetCallback {
 public:
  WebApplicationCapableGetCallback(Ewk_Web_App_Capable_Get_Callback func,
                                   void* user_data)
      : func_(func), user_data_(user_data) {}  // LCOV_EXCL_LINE
  /* LCOV_EXCL_START */
  void Run(bool capable) {
    if (func_) {
      (func_)(capable ? EINA_TRUE : EINA_FALSE, user_data_);
    }
  }
  /* LCOV_EXCL_STOP */

 private:
  Ewk_Web_App_Capable_Get_Callback func_;
  void *user_data_;
};

class DidChangeThemeColorCallback {
 public:
  DidChangeThemeColorCallback() : callback_(nullptr), user_data_(nullptr) {}

  void Set(Ewk_View_Did_Change_Theme_Color_Callback callback, void* user_data) {
    callback_ = callback;    // LCOV_EXCL_LINE
    user_data_ = user_data;  // LCOV_EXCL_LINE
  }
  /* LCOV_EXCL_START */
  void Run(Evas_Object* o, const SkColor& color) {
    if (callback_)
      callback_(o, SkColorGetR(color), SkColorGetG(color),
                SkColorGetB(color),
                SkColorGetA(color), user_data_);
  }
  /* LCOV_EXCL_STOP */

 private:
  Ewk_View_Did_Change_Theme_Color_Callback callback_;
  void* user_data_;
};

class IsVideoPlayingCallback {
 public:
  /* LCOV_EXCL_START */
  IsVideoPlayingCallback(Ewk_Is_Video_Playing_Callback func, void* user_data)
      : func_(func), user_data_(user_data) {}
  void Run(Evas_Object* obj, bool isplaying) {
    if (func_) {
      (func_)(obj, isplaying ? EINA_TRUE : EINA_FALSE, user_data_);
    }
  }
  /* LCOV_EXCL_STOP */

 private:
  Ewk_Is_Video_Playing_Callback func_;
  void* user_data_;
};

class WebViewAsyncRequestHitTestDataCallback;
class JavaScriptDialogManagerEfl;
class PermissionPopupManager;

class EWebView {
 public:
  static EWebView* FromEvasObject(Evas_Object* eo);

  EWebView(Ewk_Context*, Evas_Object* smart_object);
  ~EWebView();

  // initialize data members and activate event handlers.
  // call this once after created and before use
  void Initialize();

  void CreateNewWindow(
      content::WebContentsEflDelegate::WebContentsCreateCallback);
  static Evas_Object* GetHostWindowDelegate(const content::WebContents*);

  Ewk_Context* context() const { return context_.get(); }
  Evas_Object* evas_object() const { return evas_object_; }
  Evas_Object* native_view() const { return native_view_; }
  Evas_Object* GetElmWindow() const;
  Evas* GetEvas() const { return evas_object_evas_get(evas_object_); }
  PermissionPopupManager* GetPermissionPopupManager() const {
    return permission_popup_manager_.get(); // LCOV_EXCL_LINE
  }

  content::WebContents& web_contents() const { return *web_contents_.get(); }

#if defined(TIZEN_ATK_SUPPORT)
  EWebAccessibility& eweb_accessibility() const {
    return *eweb_accessibility_.get();
  }
#endif

  template <EWebViewCallbacks::CallbackType callbackType>
  EWebViewCallbacks::CallBack<callbackType> SmartCallback() const {
    return EWebViewCallbacks::CallBack<callbackType>(evas_object_);
  }

  void set_magnifier(bool status);

  // ewk_view api
  void SetURL(const GURL& url);
  const GURL& GetURL() const;
  const GURL& GetOriginalURL() const;
  void SetClearTilesOnHide(bool enable);
  void Reload();
  void ReloadBypassingCache();
  Eina_Bool CanGoBack();
  Eina_Bool CanGoForward();
  Eina_Bool HasFocus() const;
  void SetFocus(Eina_Bool focus);
  void SetTileCoverAreaMultiplier(float cover_area_multiplier);
  Eina_Bool GoBack();
  Eina_Bool GoForward();
  void Suspend();
  void Resume();
  void Stop();
  double GetTextZoomFactor() const;
  void SetTextZoomFactor(double text_zoom_factor);
  double GetPageZoomFactor() const;
  bool SetPageZoomFactor(double page_zoom_factor);
  void ExecuteEditCommand(const char* command, const char* value);
#if defined(OS_TIZEN)
  void EnterDragState();
#endif
  void SetOrientation(int orientation);
  int GetOrientation();
  bool TouchEventsEnabled() const;
  void SetTouchEventsEnabled(bool enabled);
  bool MouseEventsEnabled() const;
  void SetMouseEventsEnabled(bool enabled);
  void HandleTouchEvents(Ewk_Touch_Event_Type type,
                         const Eina_List* points,
                         const Evas_Modifier* modifiers);
  void Show();
  void Hide();
  bool ExecuteJavaScript(const char* script,
                         Ewk_View_Script_Execute_Callback callback,
                         void* userdata);
  bool SetUserAgent(const char* userAgent);
  bool SetUserAgentAppName(const char* application_name);
#if defined(OS_TIZEN)
  bool SetPrivateBrowsing(bool incognito);
  bool GetPrivateBrowsing() const;
#endif
  const char* GetUserAgent() const;
  const char* GetUserAgentAppName() const;
  const char* CacheSelectedText();
  Ewk_Settings* GetSettings() { return settings_.get(); }
  _Ewk_Frame* GetMainFrame();
  void UpdateWebKitPreferences();
  void LoadHTMLString(const char* html,
                      const char* base_uri,
                      const char* unreachable_uri);
  void LoadPlainTextString(const char* plain_text);

  void LoadHTMLStringOverridingCurrentEntry(const char* html,
                                            const char* base_uri,
                                            const char* unreachable_url);
  void LoadData(const char* data,
                size_t size,
                const char* mime_type,
                const char* encoding,
                const char* base_uri,
                const char* unreachable_uri = NULL,
                bool should_replace_current_entry = false);

  void InvokeLoadError(const GURL& url, int error_code, bool is_cancellation);
  void SetViewAuthCallback(Ewk_View_Authentication_Callback callback,
                           void* user_data);
  void InvokeAuthCallback(LoginDelegateEfl* login_delegate,
                          const GURL& url,
                          const std::string& realm);

  void Find(const char* text, Ewk_Find_Options);
  void InvokeAuthCallbackOnUI(_Ewk_Auth_Challenge* auth_challenge);
  void SetContentSecurityPolicy(const char* policy, Ewk_CSP_Header_Type type);
  void HandlePopupMenu(const std::vector<content::MenuItem>& items,
                     int selectedIndex,
                     bool multiple,
                     const gfx::Rect& bounds);
  void HidePopupMenu();
  void HandleLongPressGesture(const content::ContextMenuParams&);
  void ShowContextMenu(const content::ContextMenuParams&);
  void CancelContextMenu(int request_id);
  void SetScale(double scale_factor);
  void SetScaleChangedCallback(Ewk_View_Scale_Changed_Callback callback,
      void* user_data);

  bool GetScrollPosition(int* x, int* y) const;
  void SetScroll(int x, int y);
  void UrlRequestSet(const char* url,
                     content::NavigationController::LoadURLType loadtype,
                     Eina_Hash* headers,
                     const char* body);

  /* LCOV_EXCL_START */
  SelectPickerBase* GetSelectPicker() const {
    return select_picker_.get();
  }
  content::SelectionControllerEfl* GetSelectionController() const;
  content::PopupControllerEfl* GetPopupController() const {
    return popup_controller_.get();
  }
  ScrollDetector* GetScrollDetector() const {
    return scroll_detector_.get();
  }
  /* LCOV_EXCL_STOP */
  bool SetTopControlsHeight(size_t top_height, size_t bottom_height);
  bool SetTopControlsState(Ewk_Top_Control_State constraint,
                           Ewk_Top_Control_State current,
                           bool animate);
  void MoveCaret(const gfx::Point& point);
  void SelectFocusedLink();
  bool GetSelectionRange(Eina_Rectangle* left_rect, Eina_Rectangle* right_rect);
  Eina_Bool ClearSelection();

  // Callback OnCopyFromBackingStore will be called once we get the snapshot
  // from render
  void OnCopyFromBackingStore(bool success, const SkBitmap& bitmap);

  void OnFocusIn();
  void OnFocusOut();

  void UpdateContextMenu(bool is_password_input);

  void RenderViewCreated(content::RenderViewHost* render_view_host);

  /**
   * Creates a snapshot of given rectangle from EWebView
   *
   * @param rect rectangle of EWebView which will be taken into snapshot
   * @param scale_factor scale factor
   * @return created snapshot or NULL if error occured.
   * @note ownership of snapshot is passed to caller
   */
  Evas_Object* GetSnapshot(Eina_Rectangle rect, float scale_factor);

  bool GetSnapshotAsync(Eina_Rectangle rect,
                        Ewk_Web_App_Screenshot_Captured_Callback callback,
                        void* user_data,
                        float scale_factor);
  void InvokePolicyResponseCallback(_Ewk_Policy_Decision* policy_decision);
  void InvokePolicyNavigationCallback(NavigationPolicyParams params,
                                      bool* handled);
  void UseSettingsFont();

  _Ewk_Hit_Test* RequestHitTestDataAt(int x, int y, Ewk_Hit_Test_Mode mode);
  Eina_Bool AsyncRequestHitTestDataAt(int x,
                                      int y,
                                      Ewk_Hit_Test_Mode mode,
                                      Ewk_View_Hit_Test_Request_Callback,
                                      void* user_data);
  _Ewk_Hit_Test* RequestHitTestDataAtBlinkCoords(int x,
                                                 int y,
                                                 Ewk_Hit_Test_Mode mode);
  Eina_Bool AsyncRequestHitTestDataAtBlinkCords(
      int x,
      int y,
      Ewk_Hit_Test_Mode mode,
      Ewk_View_Hit_Test_Request_Callback,
      void* user_data);
  void DispatchAsyncHitTestData(const Hit_Test_Params& params,
                                int64_t request_id);
  void UpdateHitTestData(const Hit_Test_Params& params);

  int current_find_request_id() const { return current_find_request_id_; }
  bool PlainTextGet(Ewk_View_Plain_Text_Get_Callback callback, void* user_data);
  void InvokePlainTextGetCallback(const std::string& content_text,
                                  int plain_text_get_callback_id);
  int SetEwkViewPlainTextGetCallback(Ewk_View_Plain_Text_Get_Callback callback,
                                     void* user_data);
  void SetViewGeolocationPermissionCallback(
      Ewk_View_Geolocation_Permission_Callback callback,
      void* user_data);
  bool InvokeViewGeolocationPermissionCallback(
      _Ewk_Geolocation_Permission_Request*
          geolocation_permission_request_context,
      Eina_Bool* result);
  void SetViewUserMediaPermissionCallback(
      Ewk_View_User_Media_Permission_Callback callback,
      void* user_data);
  bool InvokeViewUserMediaPermissionCallback(
      _Ewk_User_Media_Permission_Request* user_media_permission_request_context,
      Eina_Bool* result);
  void SetViewUnfocusAllowCallback(Ewk_View_Unfocus_Allow_Callback callback,
                                   void* user_data);
  bool InvokeViewUnfocusAllowCallback(Ewk_Unfocus_Direction direction,
                                      Eina_Bool* result);
  void DidChangeContentsSize(int width, int height);
  const Eina_Rectangle GetContentsSize() const;
  void GetScrollSize(int* w, int* h);
  void StopFinding();
  void SetProgressValue(double progress);
  double GetProgressValue();
  const char* GetTitle();
  bool SaveAsPdf(int width, int height, const std::string& file_name);
  void BackForwardListClear();
  _Ewk_Back_Forward_List* GetBackForwardList() const;
  void InvokeBackForwardListChangedCallback();
  _Ewk_History* GetBackForwardHistory() const;
  bool WebAppCapableGet(Ewk_Web_App_Capable_Get_Callback callback,
                        void* userData);
  bool WebAppIconUrlGet(Ewk_Web_App_Icon_URL_Get_Callback callback,
                        void* userData);
  bool WebAppIconUrlsGet(Ewk_Web_App_Icon_URLs_Get_Callback callback,
                         void* userData);
  void InvokeWebAppCapableGetCallback(bool capable, int callbackId);
  void InvokeWebAppIconUrlGetCallback(const std::string& iconUrl,
                                      int callbackId);
  void InvokeWebAppIconUrlsGetCallback(
      const std::map<std::string, std::string>& iconUrls,
      int callbackId);
  void SetNotificationPermissionCallback(
      Ewk_View_Notification_Permission_Callback callback,
      void* user_data);
  bool IsNotificationPermissionCallbackSet() const;
  bool InvokeNotificationPermissionCallback(
      Ewk_Notification_Permission_Request* request);

  bool GetMHTMLData(Ewk_View_MHTML_Data_Get_Callback callback, void* user_data);
  void OnMHTMLContentGet(const std::string& mhtml_content, int callback_id);
  void SetReaderMode(Eina_Bool enable);
  bool SavePageAsMHTML(const std::string& path,
                       Ewk_View_Save_Page_Callback callback,
                       void* user_data);
  bool IsFullscreen();
  void ExitFullscreen();
  double GetScale();
  void DidChangePageScaleFactor(double scale_factor);
  void SetJavaScriptAlertCallback(Ewk_View_JavaScript_Alert_Callback callback,
                                  void* user_data);
  void JavaScriptAlertReply();
  void SetJavaScriptConfirmCallback(
      Ewk_View_JavaScript_Confirm_Callback callback,
      void* user_data);
  void JavaScriptConfirmReply(bool result);
  void SetJavaScriptPromptCallback(Ewk_View_JavaScript_Prompt_Callback callback,
                                   void* user_data);
  void JavaScriptPromptReply(const char* result);
  void set_renderer_crashed();
  void GetPageScaleRange(double* min_scale, double* max_scale);
  bool GetDrawsTransparentBackground();
  bool GetBackgroundColor(Ewk_View_Background_Color_Get_Callback callback,
                          void* user_data);
  void OnGetBackgroundColor(int r, int g, int b, int a, int callback_id);
  bool SetBackgroundColor(int red, int green, int blue, int alpha);
  void GetSessionData(const char** data, unsigned* length) const;
  bool RestoreFromSessionData(const char* data, unsigned length);
  void ShowFileChooser(content::RenderFrameHost* render_frame_host,
                       const content::FileChooserParams&);
  void SetBrowserFont();
  bool IsDragging() const;

  bool SetMainFrameScrollbarVisible(bool visible);
  bool GetMainFrameScrollbarVisible(
      Ewk_View_Main_Frame_Scrollbar_Visible_Get_Callback callback,
      void* user_data);
  void InvokeMainFrameScrollbarVisibleCallback(bool visible, int callbackId);

  void RequestColorPicker(int r, int g, int b, int a);
  bool SetColorPickerColor(int r, int g, int b, int a);
  void InputPickerShow(ui::TextInputType input_type, double input_value);

  void ShowContentsDetectedPopup(const char*);

  // Returns TCP port number with Inspector, or 0 if error.
  int StartInspectorServer(int port = 0);
  bool StopInspectorServer();

  void LoadNotFoundErrorPage(const std::string& invalidUrl);
  static std::string GetPlatformLocale();
  bool GetLinkMagnifierEnabled() const;
  void SetLinkMagnifierEnabled(bool enabled);

  bool GetHorizontalPanningHold() const;
  void SetHorizontalPanningHold(bool hold);
  bool GetVerticalPanningHold() const;
  void SetVerticalPanningHold(bool hold);

  void SetQuotaPermissionRequestCallback(
      Ewk_Quota_Permission_Request_Callback callback,
      void* user_data);
  void InvokeQuotaPermissionRequest(
      _Ewk_Quota_Permission_Request* request,
      const content::QuotaPermissionContext::PermissionCallback& cb);
  void QuotaRequestReply(const _Ewk_Quota_Permission_Request* request,
                         bool allow);
  void QuotaRequestCancel(const _Ewk_Quota_Permission_Request* request);

  void SetViewMode(blink::WebViewMode view_mode);

  gfx::Point GetContextMenuPosition() const;

  content::ContextMenuControllerEfl* GetContextMenuController() {
    return context_menu_.get();
  }
  void ResetContextMenuController();

#if defined(TIZEN_VIDEO_HOLE)
  void SetVideoHoleSupport(bool enable);
#endif
#if defined(TIZEN_MULTIMEDIA_SUPPORT)
  void OpenMediaPlayer(const std::string& url, const std::string& cookie);
#endif

  Eina_Bool AddJavaScriptMessageHandler(Evas_Object* view,
                                        Ewk_View_Script_Message_Cb callback,
                                        std::string name);

  content::GinNativeBridgeDispatcherHost* GetGinNativeBridgeDispatcherHost()
      const {
    return gin_native_bridge_dispatcher_host_.get();
  }

  bool SetPageVisibility(Ewk_Page_Visibility_State page_visibility_state);

  void SetExceededIndexedDatabaseQuotaCallback(
      Ewk_View_Exceeded_Indexed_Database_Quota_Callback callback,
      void* user_data);
  void InvokeExceededIndexedDatabaseQuotaCallback(const GURL& origin,
                                                  int64_t current_quota);
  void ExceededIndexedDatabaseQuotaReply(bool allow);

  /// ---- Event handling
  bool HandleShow();
  bool HandleHide();
  bool HandleMove(int x, int y);
  bool HandleResize(int width, int height);
  bool HandleTextSelectionDown(int x, int y);
  bool HandleTextSelectionUp(int x, int y);

  void HandleRendererProcessCrash();
  void InvokeWebProcessCrashedCallback();

  void HandleTapGestureForSelection(bool is_content_editable);
  void HandleZoomGesture(blink::WebGestureEvent& event);
  void ClosePage();

  void SetSessionTimeout(unsigned long timeout);

  void RequestManifest(Ewk_View_Request_Manifest_Callback callback,
                       void* user_data);
  void DidRespondRequestManifest(_Ewk_View_Request_Manifest* manifest,
                                 Ewk_View_Request_Manifest_Callback callback,
                                 void* user_data);
  void SetBeforeUnloadConfirmPanelCallback(
      Ewk_View_Before_Unload_Confirm_Panel_Callback callback,
      void* user_data);
  void ReplyBeforeUnloadConfirmPanel(Eina_Bool result);
  bool SetVisibility(bool enable);

  bool IsVideoPlaying(Ewk_Is_Video_Playing_Callback callback, void* user_data);
  void InvokeIsVideoPlayingCallback(bool is_playing, int callback_id);

#if defined(OS_TIZEN_TV_PRODUCT)
  void SuspendNetworkLoading();
  void ResumeNetworkLoading();
  bool SetMixedContents(bool allow);
  void SetMarlinEnable(bool is_enable);
  bool GetMarlinEnable();
  void SetActiveDRM(const char* drm_system_id);
  const std::string& GetActiveDRM() const;
  std::vector<std::string> NotifyPlaybackState(int state,
                                               const char* url,
                                               const char* mime_type);
  Eina_Bool SetTTSMode(tts_mode_e mode);
  void NotifySubtitleState(int state, double time_stamp);
  void NotifySubtitlePlay(int active_track_id,
                          const char* url,
                          const char* lang);
  void NotifySubtitleData(int track_id,
                          double time_stamp,
                          const std::string& data,
                          unsigned int size);
  void NotifyParentalRatingInfo(const char* info, const char* url);
  void SetParentalRatingResult(const char* info, bool is_pass);
  void UpdateCurrentTime(double current_time);
  double GetCurrentTime() { return current_time_; }
  bool SetTranslatedURL(const char* url);
  void SetSubtitleLang(const char* lang_list);
  void SetHighBitRate(Eina_Bool is_high_bitrate);
  bool IsHighBitRate();
  void ClearAllTilesResources();
  bool EdgeScrollBy(int delta_x, int delta_y);
  void InvokeEdgeScrollByCallback(const gfx::Point&, bool);
  void InvokeScrollbarThumbFocusChangedCallback(
      Ewk_Scrollbar_Orientation orientation,
      bool focused);
  void InvokeRunArrowScrollCallback();
  void SendKeyEvent(Evas_Object* ewk_view, void* key_event, bool is_press);
  void SetKeyEventsEnabled(bool enabled);
  void SetCursorByClient(bool enable);
  void AddItemToBackForwardList(const _Ewk_Back_Forward_List_Item* item);
  void OnMouseLongPressed(int x, int y);
  void OnMouseDown(int x, int y);
  bool OnMouseUp();
  void OnMouseMove();
  void ShowContextMenuWithLongPressed(const GURL& url, int x, int y);
  void AddDynamicCertificatePath(const std::string& host,
                                 const std::string& cert_path);
  void DrawLabel(Evas_Object* image, Eina_Rectangle rect);
  void ClearLabels();
  void ExecuteJSInFrame(base::string16 script,
                        Ewk_View_Script_Execute_Callback callback,
                        void* userdata,
                        content::RenderFrameHost* render_frame_host);
  bool ExecuteJavaScriptInAllFrames(const char* script,
                                    Ewk_View_Script_Execute_Callback callback,
                                    void* userdata);
  void SetFloatWindowState(bool enabled);
  void AutoLogin(const char* user_name, const char* password);
#endif

#if defined(TIZEN_ATK_FEATURE_VD)
  void MoveFocusToBrowser(int direction);
  void DeactivateAtk(bool deactivated);
#endif

  void SetDoNotTrack(Eina_Bool);
  void HwKeyboardStatusChanged(const std::string& device_name,
                               Eina_Bool enable);

  void SetDidChangeThemeColorCallback(
      Ewk_View_Did_Change_Theme_Color_Callback callback,
      void* user_data);
  void DidChangeThemeColor(const SkColor& color);

#if defined(TIZEN_ATK_SUPPORT)
  void UpdateSpatialNavigationStatus(Eina_Bool enable);
  bool CheckLazyInitializeAtk() {
    return is_initialized_ && lazy_initalize_atk_;
  }
  void InitAtk();
  bool GetAtkStatus();
#endif

#if defined(TIZEN_PEPPER_EXTENSIONS)
  void InitializePepperExtensionSystem();
  EwkExtensionSystemDelegate* GetExtensionDelegate();
  void SetWindowId();
  void SetPepperExtensionWidgetInfo(Ewk_Value widget_pepper_ext_info);
  void SetPepperExtensionCallback(Generic_Sync_Call_Callback cb, void* data);
  void RegisterPepperExtensionDelegate();
  void UnregisterPepperExtensionDelegate();
#endif

  void InvokeFormRepostWarningShowCallback(
      _Ewk_Form_Repost_Decision_Request* request);
  void InvokeBeforeFormRepostWarningShowCallback(
      _Ewk_Form_Repost_Decision_Request* request);

  const gfx::Rect& GetCustomEmailViewportRect() const;

  void OnSelectionRectReceived(const gfx::Rect& selection_rect) const;

  // Changes viewport without resizing Evas_Object representing webview
  // and its corresponding RWHV to let Blink renders custom viewport
  // while showing picker.
  void AdjustViewPortHeightToPopupMenu(bool is_popup_menu_visible);

  void ScrollFocusedNodeIntoView();
#if defined(OS_TIZEN_TV_PRODUCT)
  bool UseEarlyRWI() { return use_early_rwi_; }
  bool RWIInfoShowed() { return rwi_info_showed_; }
  GURL RWIURL() { return rwi_gurl_; }
  void OnDialogClosed();
#endif
  void SetAppNeedEffect(Eina_Bool);

  void SyncAcceptLanguages(const std::string& accept_languages);
  bool ShouldIgnoreNavigation(
      const navigation_interception::NavigationParams& params);

#if defined(TIZEN_TBM_SUPPORT)
  void SetOffscreenRendering(bool enable);
#endif

 private:
  static void NativeViewResize(void* data,
                               Evas* e,
                               Evas_Object* obj,
                               void* event_info);
  void InitializeContent();
  void SendDelayedMessages(content::RenderViewHost* render_view_host);

  void EvasToBlinkCords(int x, int y, int* view_x, int* view_y);
  Eina_Bool AsyncRequestHitTestDataAtBlinkCords(
      int x,
      int y,
      Ewk_Hit_Test_Mode mode,
      WebViewAsyncRequestHitTestDataCallback* cb);

  content::WebContentsViewEfl* GetWebContentsViewEfl() const;

  content::RenderWidgetHostViewEfl* rwhv() const;

  JavaScriptDialogManagerEfl* GetJavaScriptDialogManagerEfl();

  void ShowContextMenuInternal(const content::ContextMenuParams&);

  void UpdateWebkitPreferencesEfl(content::RenderViewHost*);

  static void CustomEmailViewportRectChangedCallback(void* user_data,
      Evas_Object* object, void* event_info);

  static void OnCustomScrollBeginCallback(void* user_data,
                                          Evas_Object* object,
                                          void* event_info);

  static void OnCustomScrollEndCallback(void* user_data,
                                        Evas_Object* object,
                                        void* event_info);

  void ChangeScroll(int& x, int& y);
  void UpdateContextMenuWithParams(const content::ContextMenuParams& params);

  static void OnViewFocusIn(void* data, Evas*, Evas_Object*, void*);
  static void OnViewFocusOut(void* data, Evas*, Evas_Object*, void*);

  void GenerateMHTML(Ewk_View_Save_Page_Callback callback,
                     void* user_data,
                     const base::FilePath& file_path);
  void MHTMLGenerated(Ewk_View_Save_Page_Callback callback,
                      void* user_data,
                      const base::FilePath& file_path,
                      int64_t file_size);

#if defined(OS_TIZEN_TV_PRODUCT)
  void GetMousePosition(gfx::Point&);
  void InitInspectorServer();
#endif

  void SetScaledContentsSize();
  Evas_Object* content_image_elm_host() const;
  static Eina_Bool DelayedPopulateAndShowContextMenu(void* data);

  scoped_refptr<WebViewEvasEventHandler> evas_event_handler_;
  scoped_refptr<Ewk_Context> context_;
  std::unique_ptr<content::WebContents> web_contents_;
  std::unique_ptr<content::WebContentsDelegateEfl> web_contents_delegate_;
  std::string pending_url_request_;
  std::unique_ptr<Ewk_Settings> settings_;
  std::unique_ptr<_Ewk_Frame> frame_;
  std::unique_ptr<_Ewk_Policy_Decision> window_policy_;
  Evas_Object* evas_object_;
  Evas_Object* native_view_;
  bool touch_events_enabled_;
  bool mouse_events_enabled_;
  double text_zoom_factor_;
  mutable std::string user_agent_;
  mutable std::string user_agent_app_name_;
  std::unique_ptr<_Ewk_Auth_Challenge> auth_challenge_;
  std::string selected_text_cached_;

  std::unique_ptr<content::ContextMenuControllerEfl> context_menu_;
  std::unique_ptr<content::FileChooserControllerEfl> file_chooser_;
  std::unique_ptr<content::PopupControllerEfl> popup_controller_;
  base::string16 previous_text_;
  int current_find_request_id_;
  static int find_request_id_counter_;

  typedef WebViewCallback<Ewk_View_Plain_Text_Get_Callback, const char*>
      EwkViewPlainTextGetCallback;
  base::IDMap<EwkViewPlainTextGetCallback*> plain_text_get_callback_map_;

  typedef WebViewCallback<Ewk_View_MHTML_Data_Get_Callback, const char*>
      MHTMLCallbackDetails;
  base::IDMap<MHTMLCallbackDetails*> mhtml_callback_map_;

  typedef WebViewCallback<Ewk_View_Main_Frame_Scrollbar_Visible_Get_Callback,
                          bool>
      MainFrameScrollbarVisibleGetCallback;
  base::IDMap<MainFrameScrollbarVisibleGetCallback*>
      main_frame_scrollbar_visible_callback_map_;

  base::IDMap<BackgroundColorGetCallback*> background_color_get_callback_map_;

  gfx::Size contents_size_;
  double progress_;
  mutable std::string title_;
  Hit_Test_Params hit_test_params_;
  base::WaitableEvent hit_test_completion_;
  double page_scale_factor_;
  double x_delta_;
  double y_delta_;

  WebViewCallback<Ewk_View_Geolocation_Permission_Callback,
                  _Ewk_Geolocation_Permission_Request*>
      geolocation_permission_cb_;
  WebViewCallback<Ewk_View_User_Media_Permission_Callback,
                  _Ewk_User_Media_Permission_Request*>
      user_media_permission_cb_;
  WebViewCallback<Ewk_View_Unfocus_Allow_Callback, Ewk_Unfocus_Direction>
      unfocus_allow_cb_;
  WebViewCallback<Ewk_View_Notification_Permission_Callback,
                  Ewk_Notification_Permission_Request*>
      notification_permission_callback_;
  WebViewCallback<Ewk_Quota_Permission_Request_Callback,
                  const _Ewk_Quota_Permission_Request*> quota_request_callback_;
  WebViewCallback<Ewk_View_Scale_Changed_Callback, double> scale_changed_cb_;
  WebViewCallback<Ewk_View_Authentication_Callback, _Ewk_Auth_Challenge*>
      authentication_cb_;

  DidChangeThemeColorCallback did_change_theme_color_callback_;

  WebViewExceededQuotaCallback<
      Ewk_View_Exceeded_Indexed_Database_Quota_Callback,
      Ewk_Security_Origin*,
      long long>
      exceeded_indexed_db_quota_callback_;
  std::unique_ptr<Ewk_Security_Origin> exceeded_indexed_db_quota_origin_;

  std::unique_ptr<content::InputPicker> input_picker_;
  base::IDMap<WebApplicationIconUrlGetCallback*>
      web_app_icon_url_get_callback_map_;
  base::IDMap<WebApplicationIconUrlsGetCallback*>
      web_app_icon_urls_get_callback_map_;
  base::IDMap<WebApplicationCapableGetCallback*>
      web_app_capable_get_callback_map_;

  base::IDMap<IsVideoPlayingCallback*> is_video_playing_callback_map_;
  std::unique_ptr<PermissionPopupManager> permission_popup_manager_;
  std::unique_ptr<ScrollDetector> scroll_detector_;

  // Manages injecting native objects.
  std::unique_ptr<content::GinNativeBridgeDispatcherHost>
      gin_native_bridge_dispatcher_host_;

  std::map<const _Ewk_Quota_Permission_Request*,
           content::QuotaPermissionContext::PermissionCallback>
      quota_permission_request_map_;

#if defined(OS_TIZEN_TV_PRODUCT)
  bool is_marlin_enable_;
  std::string active_drm_;
  bool is_processing_edge_scroll_;
  bool context_menu_show_;
  double current_time_;
  bool is_high_bitrate_;
  base::OneShotTimer mouse_long_press_timer_;
#endif

#if defined(OS_TIZEN)
  Ecore_Event_Handler* window_rotate_handler_;
#endif

  bool is_initialized_;

#if defined(OS_TIZEN_TV_PRODUCT)
  bool use_early_rwi_;
  bool rwi_info_showed_;
  GURL rwi_gurl_;
#endif

  std::unique_ptr<_Ewk_Back_Forward_List> back_forward_list_;

  static content::WebContentsEflDelegate::WebContentsCreateCallback
      create_new_window_web_contents_cb_;

  gfx::Vector2d previous_scroll_position_;

  gfx::Point context_menu_position_;

  gfx::Rect custom_email_viewport_rect_;

  content::ContextMenuParams saved_context_menu_params_;

  std::vector<IPC::Message*> delayed_messages_;

  std::map<int64_t, WebViewAsyncRequestHitTestDataCallback*> hit_test_callback_;

  std::unique_ptr<SelectPickerBase> select_picker_;

  Ecore_Event_Handler* add_device_handler_;
  Ecore_Event_Handler* remove_device_handler_;
#if defined(TIZEN_ATK_SUPPORT)
  std::unique_ptr<EWebAccessibility> eweb_accessibility_;
  bool lazy_initalize_atk_;
#endif

  Ecore_Timer* delayed_show_context_menu_timer_;

  content::ContentBrowserClientEfl::AcceptLangsChangedCallback
      accept_langs_changed_callback_;
};

const unsigned int g_default_tilt_motion_sensitivity = 3;

#endif
