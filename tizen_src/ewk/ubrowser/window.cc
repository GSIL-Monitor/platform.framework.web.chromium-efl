// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "window.h"

#include <assert.h>
#include <vector>
#include <EWebKit.h>
#include <EWebKit_internal.h>
#include <EWebKit_product.h>
#include <Elementary.h>

#include "browser.h"
#include "logger.h"
#include "window_ui.h"

#include "tizen/system_info.h"

#if defined(OS_TIZEN)
#include <efl_extension.h>
#endif

#if defined(OS_TIZEN_TV_PRODUCT)
#include "ecore_x_wayland_wrapper.h"
#include <cursor_module.h>
#endif

#if defined(OS_TIZEN)
#include "tizen_src/ewk/efl_integration/public/ewk_media_playback_info.h"
static Ewk_Application_Type application_type = EWK_APPLICATION_TYPE_OTHER;
#endif
#if defined(OS_TIZEN_TV_PRODUCT)
#include "tizen_src/ewk/efl_integration/public/ewk_media_subtitle_info.h"
#endif

// Parses tizen version string (2.4 | 2.3.1 | 3.0 etc).
// Output std::vector where
//   std::vector[0] = major,
//   std::vector[1] = minor,
//   std::vector[2] = release,
// In case of failure all values of vector are zero.
std::vector<unsigned> ParseTizenVersion(const std::string& tizen_string) {
  std::vector<unsigned> version(3, 0);
  for (unsigned i = 0, index = 0; i < tizen_string.size(); i++) {
    if ((i % 2) == 0) {
      if (isdigit(tizen_string[i]) && index < version.size())
        version[index++] = atoi(&tizen_string[i]);
      else
        return std::vector<unsigned>(3, 0);
     } else if (tizen_string[i] != '.')
        return std::vector<unsigned>(3, 0);
  }
  return version;
}

Window::Window(Browser& browser, int width, int height, bool incognito)
    : browser_(browser)
    , is_fullscreen_(false) {
  window_ = elm_win_util_standard_add("ubrowser", "uBrowser");
  evas_object_resize(window_, width, height);
  elm_win_autodel_set(window_, EINA_TRUE);

  if (IsMobileProfile() && !IsWearableProfile()) {
    elm_win_indicator_mode_set(window_, ELM_WIN_INDICATOR_SHOW);
    elm_win_indicator_opacity_set(window_, ELM_WIN_INDICATOR_OPAQUE);
  }

  Evas_Object* conform = elm_conformant_add(window_);
  evas_object_size_hint_weight_set(conform,
                                   EVAS_HINT_EXPAND,
                                   EVAS_HINT_EXPAND);
  elm_win_resize_object_add(window_, conform);
  evas_object_show(conform);

  Evas_Object *box = elm_box_add(window_);
  elm_object_content_set(conform, box);
  evas_object_show(box);

  ui_ = new WindowUI(*this, browser_);

  elm_box_pack_end(box, ui_->GetURLBar());
  evas_object_show(ui_->GetURLBar());

  // Add elm_bg for focusing
  web_view_elm_host_ = elm_bg_add(window_);
  evas_object_size_hint_align_set(web_view_elm_host_,
      EVAS_HINT_FILL, EVAS_HINT_FILL);
  evas_object_size_hint_weight_set(web_view_elm_host_, EVAS_HINT_EXPAND,
      EVAS_HINT_EXPAND);
  elm_box_pack_end(box, web_view_elm_host_);
  elm_object_focus_allow_set(web_view_elm_host_, EINA_TRUE);
  elm_object_focus_set(web_view_elm_host_, EINA_TRUE);
  evas_object_show(web_view_elm_host_);

  Evas* evas = evas_object_evas_get(window_);

  if (incognito)
    web_view_ = ewk_view_add_in_incognito_mode(evas);
  else
    web_view_ = ewk_view_add(evas);

  evas_object_resize(web_view_, width, height);
  elm_object_part_content_set(web_view_elm_host_, "overlay", web_view_);

#if defined(TIZEN_VIDEO_HOLE)
  if (browser.IsVideoHoleEnabled())
    ewk_view_set_support_video_hole(web_view_, window_, EINA_TRUE, EINA_FALSE);
#endif

  Evas_Object* navbar = ui_->GetNavBar();
  if (navbar) {
    elm_box_pack_end(box, navbar);
    evas_object_show(navbar);
  }

  std::string ua = browser_.GetUserAgent();
  if (!ua.empty())
    SetUserAgent(ua.c_str());

  evas_object_smart_callback_add(web_view_elm_host_, "focused",
      &Window::OnHostFocusedIn, this);
  evas_object_smart_callback_add(web_view_elm_host_, "unfocused",
      &Window::OnHostFocusedOut, this);

  std::string tizen_version = browser_.GetTizenVersion();
  if (!tizen_version.empty()) {
    std::vector<unsigned> parsed_tizen_version = ParseTizenVersion(
        tizen_version);
    ewk_settings_tizen_compatibility_mode_set(GetEwkSettings(),
        parsed_tizen_version[0],
        parsed_tizen_version[1],
        parsed_tizen_version[2]);
  }

  evas_object_smart_callback_add(web_view_, "title,changed",
                                 &Window::OnTitleChanged, this);
  evas_object_smart_callback_add(web_view_, "url,changed",
                                 &Window::OnURLChanged, this);
  evas_object_smart_callback_add(web_view_, "load,started",
                                 &Window::OnLoadStarted, this);
  evas_object_smart_callback_add(web_view_, "load,finished",
                                 &Window::OnLoadFinished, this);
  evas_object_smart_callback_add(web_view_, "create,window",
                                 &Window::OnNewWindowRequest, this);
  evas_object_smart_callback_add(web_view_, "console,message",
                                 &Window::OnConsoleMessage, NULL);
#if defined(TIZEN_TBM_SUPPORT) && defined(OS_TIZEN_TV_PRODUCT)
  evas_object_smart_callback_add(web_view_, "notify,video,texturing",
                                 &Window::OnVideotexturing, NULL);
#endif
  evas_object_smart_callback_add(web_view_, "policy,newwindow,decide",
                                 &Window::OnNewWindowPolicyDecide, NULL);
  evas_object_smart_callback_add(web_view_, "back,forward,list,changed",
                                 &Window::OnBackForwardListChanged, this);
  evas_object_smart_callback_add(window_, "delete,request",
                                 &Window::OnWindowDelRequest, this);
  evas_object_smart_callback_add(web_view_, "usermedia,permission,request",
                                 &Window::OnUserMediaPermissionRequest, this);
  evas_object_smart_callback_add(web_view_, "fullscreen,enterfullscreen",
                                 &Window::OnEnterFullScreenRequest, this);
  evas_object_smart_callback_add(web_view_, "fullscreen,exitfullscreen",
                                 &Window::OnExitFullScreenRequest, this);
  evas_object_smart_callback_add(web_view_, "form,repost,warning,show",
                                 &Window::OnFormRepostWarningShow, this);
  evas_object_smart_callback_add(web_view_, "before,repost,warning,show",
                                 &Window::OnBeforeFormRepostWarningShow, this);

#if defined(OS_TIZEN)
  if (IsTvProfile()) {
    evas_object_event_callback_add(web_view_, EVAS_CALLBACK_KEY_DOWN,
                                 &Window::OnKeyDown, this);
    if (application_type == EWK_APPLICATION_TYPE_HBBTV) {
#if defined(OS_TIZEN_TV_PRODUCT)
      evas_object_smart_callback_add(web_view_, "notify,subtitle,play",
                                     &Window::On_Notify_Subtitle_Play, this);
      evas_object_smart_callback_add(web_view_, "notify,subtitle,stop",
                                     &Window::On_Notify_Subtitle_Stop, this);
      evas_object_smart_callback_add(web_view_, "notify,subtitle,pause",
                                     &Window::On_Notify_Subtitle_Pause, this);
      evas_object_smart_callback_add(web_view_, "notify,subtitle,resume",
                                     &Window::On_Notify_Subtitle_Resume, this);
      evas_object_smart_callback_add(web_view_, "notify,subtitle,seek,start",
                                     &Window::On_Notify_Subtitle_Seek_Start,
                                     this);
      evas_object_smart_callback_add(
          web_view_, "notify,subtitle,seek,completed",
          &Window::On_Notify_Subtitle_Seek_Completed, this);
      evas_object_smart_callback_add(web_view_, "on,subtitle,data",
                                     &Window::On_Subtitle_Data, this);
      evas_object_smart_callback_add(web_view_, "notification,playback,load",
                                     &Window::On_Video_Playback_Load, this);
      evas_object_smart_callback_add(web_view_, "notification,playback,ready",
                                     &Window::On_Video_Playback_Ready, this);
      evas_object_smart_callback_add(web_view_, "notification,playback,start",
                                     &Window::On_Video_Playback_Start, this);
      evas_object_smart_callback_add(web_view_, "notification,playback,finish",
                                     &Window::On_Video_Playback_Finish, this);
      evas_object_smart_callback_add(web_view_, "notification,playback,stop",
                                     &Window::On_Video_Playback_Stop, this);
      ewk_settings_media_subtitle_notification_set(GetEwkSettings(), true);
#endif
      ewk_settings_media_playback_notification_set(GetEwkSettings(), true);
#if defined(OS_TIZEN_TV_PRODUCT)
      Ewk_Context* context = ewk_view_context_get(web_view_);
      ewk_context_application_type_set(context, application_type);
#endif
    }
  }
#endif
  ewk_view_quota_permission_request_callback_set(
      web_view_, &Window::OnQuotaPermissionRequest, this);
  ewk_view_did_change_theme_color_callback_set(
      web_view_, &Window::OnThemeColorChanged, this);

  ewk_settings_form_profile_data_enabled_set(GetEwkSettings(), true);
  ewk_settings_form_candidate_data_enabled_set(GetEwkSettings(), true);
  ewk_settings_uses_keypad_without_user_action_set(GetEwkSettings(), false);

  // Auto fit is already turned on for mobile, enable it for "ubrowser --mobile" as well.
  ewk_settings_auto_fitting_set(GetEwkSettings(), !browser_.IsDesktop());

#if defined(OS_TIZEN)
  eext_object_event_callback_add(window_, EEXT_CALLBACK_BACK, &Window::OnHWBack,
                                 this);
#endif

  if (elm_win_wm_rotation_supported_get(window_)) {
    SetAutoRotate();
    evas_object_smart_callback_add(window_, "wm,rotation,changed",
                                   &Window::OnOrientationChanged, this);
  }

  if (IsTvProfile()) {
    // In tv profile, we should use MouseEvent.
    EnableMouseEvents(true);
    EnableTouchEvents(false);
  } else {
    EnableMouseEvents(browser_.IsDesktop());
    EnableTouchEvents(!browser_.IsDesktop());
  }
#if defined(OS_TIZEN_TV_PRODUCT)
  CreateMouseCursor();
#endif

  evas_object_show(window_);
}

Window::~Window() {
  assert(!window_);
  // deleting view will release context
  evas_object_del(web_view_);
  delete ui_;

#if defined(OS_TIZEN)
  eext_object_event_callback_del(window_, EEXT_CALLBACK_BACK,
                                 &Window::OnHWBack);
#endif

}

Ewk_Settings* Window::GetEwkSettings() const {
  return ewk_view_settings_get(web_view_);
}

void Window::LoadURL(std::string url) {
  const static std::string http = "http://";
  const static std::string https = "https://";
  const static std::string file = "file://";
  const static std::string about = "about:";
  const static std::string chrome = "chrome://";
  if (url.compare(0, 1, "/") == 0) {
    url = "file://" + url;
  } else if (url.compare(0, http.length(), http) != 0 &&
      url.compare(0, https.length(), https) != 0 &&
      url.compare(0, file.length(), file) != 0 &&
      url.compare(0, about.length(), about) != 0 &&
      url.compare(0, chrome.length(), chrome) != 0)  {
    url = "http://" + url;
  }
  log_info("Loading URL: %s", url.c_str());
  ewk_view_url_set(web_view_, url.c_str());
  ui_->OnURLChanged(url.c_str());
}

const char* Window::GetURL() const {
  return ewk_view_url_get(web_view_);
}

void Window::Activate() {
  log_trace("%s", __PRETTY_FUNCTION__);
  elm_win_activate(window_);
}

void Window::Close() {
  log_trace("%s", __PRETTY_FUNCTION__);
  evas_object_del(window_);
  browser_.OnWindowDestroyed(Id());
  window_ = NULL;
}

void Window::Show() {
  log_trace("%s", __PRETTY_FUNCTION__);
  evas_object_show(window_);
}

void Window::Hide() {
  log_trace("%s", __PRETTY_FUNCTION__);
  evas_object_hide(window_);
}

void Window::Back() {
  log_trace("%s", __PRETTY_FUNCTION__);
  if (is_fullscreen_) {
    is_fullscreen_ = false;
    ewk_view_fullscreen_exit(web_view_);
    return;
  }
  ewk_view_back(web_view_);
}

void Window::Forward() {
  log_trace("%s", __PRETTY_FUNCTION__);
  ewk_view_forward(web_view_);
}

void Window::Reload() {
  log_trace("%s", __PRETTY_FUNCTION__);
  ewk_view_reload(web_view_);
}

void Window::Stop() {
  log_trace("%s", __PRETTY_FUNCTION__);
  ewk_view_stop(web_view_);
}

void Window::SetUserAgent(const char* new_ua) {
  log_trace("%s: %s", __PRETTY_FUNCTION__, new_ua);
  ewk_view_user_agent_set(web_view_, new_ua);
}

const char* Window::GetUserAgent() const {
  return ewk_view_user_agent_get(web_view_);
}

void Window::FakeRotate() {
  log_trace("%s", __PRETTY_FUNCTION__);
  int x, y, width, height;
  evas_object_geometry_get(window_, &x, &y, &width, &height);
  evas_object_move(window_, x, y);
  evas_object_resize(window_, height, width);
  if (width > height)
    ewk_view_orientation_send(web_view_, 0);
  else
    ewk_view_orientation_send(web_view_, 90);
}

void Window::Resize(int width, int height) {
  int x, y;
  log_trace("%s: %dx%d", __PRETTY_FUNCTION__, width, height);
  evas_object_geometry_get(window_, &x, &y, 0, 0);
  evas_object_move(window_, x, y);
  evas_object_resize(window_, width, height);
  ewk_view_orientation_send(web_view_, 0);
}

void Window::EnableTouchEvents(bool enable) {
  log_trace("%s: %d", __PRETTY_FUNCTION__, enable);
  ewk_view_touch_events_enabled_set(web_view_, enable);
}

void Window::EnableMouseEvents(bool enable) {
  log_trace("%s: %d", __PRETTY_FUNCTION__, enable);
  ewk_view_mouse_events_enabled_set(web_view_, enable);
}

bool Window::AreTouchEventsEnabled() const {
  log_trace("%s", __PRETTY_FUNCTION__);
  return ewk_view_touch_events_enabled_get(web_view_);
}

bool Window::IsRememberFormDataEnabled() const {
  log_trace("%s", __PRETTY_FUNCTION__);
  Ewk_Settings* settings = GetEwkSettings();
  return ewk_settings_form_candidate_data_enabled_get(settings);
}

bool Window::IsRememberPasswordEnabled() const {
  log_trace("%s", __PRETTY_FUNCTION__);
  Ewk_Settings* settings = GetEwkSettings();
  return ewk_settings_autofill_password_form_enabled_get(settings);
}

bool Window::IsFormProfileEnabled() const {
  log_trace("%s", __PRETTY_FUNCTION__);
  Ewk_Settings* settings = GetEwkSettings();
  return ewk_settings_form_profile_data_enabled_get(settings);
}

double Window::GetScale() const {
  log_trace("%s", __PRETTY_FUNCTION__);
  return ewk_view_scale_get(web_view_);
}

void Window::GetScaleRange(double* minScale, double* maxScale) const {
  log_trace("%s", __PRETTY_FUNCTION__);
  ewk_view_scale_range_get(web_view_, minScale, maxScale);
}

void Window::SetScale(double scale) {
  log_trace("%s", __PRETTY_FUNCTION__);
  ewk_view_scale_set(web_view_, scale, 0, 0);
}

void Window::ShowInspectorURL(const char* url) {
  ui_->ShowInspectorURLPopup(url);
}

Window::IdType Window::Id() const {
  return window_;
}

void Window::SetAutoRotate() {
  static const int rots[] = {0, 90, 180, 270};
  elm_win_wm_rotation_available_rotations_set(window_, rots,
                                              (sizeof(rots) / sizeof(int)));
}

#if defined(OS_TIZEN_TV_PRODUCT)
void Window::On_Video_Playback_Load(void* data,
                                    Evas_Object*,
                                    void* event_info) {
  log_trace("%s", __PRETTY_FUNCTION__);
  Ewk_Media_Playback_Info* playback_info = (Ewk_Media_Playback_Info*)event_info;
  const char* video_url = ewk_media_playback_info_media_url_get(playback_info);
  const char* mime_type = ewk_media_playback_info_mime_type_get(playback_info);
  char translated_url[] =
      "http://www.w3schools.com/html/mov_bbb.mp4";  // for test
  log_trace(
      ">> video playback start: video_url(%s),  mime_type(%s), "
      "translated_url(%s)\n",
      video_url, mime_type, translated_url);
  ewk_media_playback_info_translated_url_set(playback_info, translated_url);
}

void Window::On_Video_Playback_Ready(void* data,
                                     Evas_Object*,
                                     void* event_info) {
  log_trace("%s", __PRETTY_FUNCTION__);
}

void Window::On_Video_Playback_Start(void* data,
                                     Evas_Object*,
                                     void* event_info) {
  log_trace("%s", __PRETTY_FUNCTION__);

  Ewk_Media_Playback_Info* playback_info = (Ewk_Media_Playback_Info*)event_info;
  const char* video_url = ewk_media_playback_info_media_url_get(playback_info);
  const char* mime_type = ewk_media_playback_info_mime_type_get(playback_info);
  char translated_url[] =
      "http://www.w3schools.com/html/mov_bbb.mp4";  // for test

  log_trace(
      ">> video playback start: video_url(%s),  mime_type(%s), "
      "translated_url(%s)\n",
      video_url, mime_type, translated_url);
  ewk_media_playback_info_media_resource_acquired_set(playback_info, 1);
}

void Window::On_Video_Playback_Finish(void* data,
                                      Evas_Object*,
                                      void* event_info) {
  log_trace("%s", __PRETTY_FUNCTION__);
}

void Window::On_Video_Playback_Stop(void* data,
                                    Evas_Object*,
                                    void* event_info) {
  log_trace("%s", __PRETTY_FUNCTION__);

  Ewk_Media_Playback_Info* playback_info = (Ewk_Media_Playback_Info*)event_info;
  const char* video_url = ewk_media_playback_info_media_url_get(playback_info);
  log_trace("<< video playback stop : video_url(%s)\n", video_url);

  Window* thiz = static_cast<Window*>(data);
  char translated_url[] =
      "http://www.w3schools.com/html/mov_bbb.mp4";  // for test
  ewk_media_translated_url_set(thiz->web_view_, translated_url);
}

#if defined(TIZEN_TBM_SUPPORT)
void Window::OnVideotexturing(void* data,
                                Evas_Object*,
                                void* event_info) {
  log_message(INFO, true, "%s", __PRETTY_FUNCTION__);
}
#endif
#endif  // defined(OS_TIZEN_TV_PRODUCT)

#if defined(OS_TIZEN_TV_PRODUCT)
void Window::On_Notify_Subtitle_Play(void* data,
                                     Evas_Object*,
                                     void* event_info) {
  log_trace("%s", __PRETTY_FUNCTION__);

  Ewk_Media_Subtitle_Info* subtitleInfo = (Ewk_Media_Subtitle_Info*)event_info;
  int id = ewk_media_subtitle_info_id_get(subtitleInfo);
  const char* url = ewk_media_subtitle_info_url_get(subtitleInfo);
  const char* lang = ewk_media_subtitle_info_lang_get(subtitleInfo);

  log_trace("<< notify subtitle play : id(%d), url(%s), lang(%s)\n", id, url,
            lang);
  Window* thiz = static_cast<Window*>(data);
  double current_time = ewk_view_media_current_time_get(thiz->web_view_);
  log_info("On_Notify_Subtitle_Play   current_time: %f", current_time);
}

void Window::On_Notify_Subtitle_Stop(void* data,
                                     Evas_Object*,
                                     void* event_info) {
  log_trace("%s", __PRETTY_FUNCTION__);

  Window* thiz = static_cast<Window*>(data);
  double current_time = ewk_view_media_current_time_get(thiz->web_view_);
  log_info("On_Notify_Subtitle_Stop   current_time: %f", current_time);
}

void Window::On_Notify_Subtitle_Pause(void* data,
                                      Evas_Object*,
                                      void* event_info) {
  log_trace("%s", __PRETTY_FUNCTION__);

  Window* thiz = static_cast<Window*>(data);
  double current_time = ewk_view_media_current_time_get(thiz->web_view_);
  log_info("On_Notify_Subtitle_Pause   current_time: %f", current_time);
}

void Window::On_Notify_Subtitle_Resume(void* data,
                                       Evas_Object*,
                                       void* event_info) {
  log_trace("%s", __PRETTY_FUNCTION__);
  Window* thiz = static_cast<Window*>(data);
  double current_time = ewk_view_media_current_time_get(thiz->web_view_);
  log_info("On_Notify_Subtitle_Resume   current_time: %f", current_time);
}

void Window::On_Notify_Subtitle_Seek_Start(void* data,
                                           Evas_Object*,
                                           void* event_info) {
  log_trace("%s", __PRETTY_FUNCTION__);
  double timestamp = *(double*)event_info;

  log_trace("notify video seek start timestamp%s", timestamp);
  Window* thiz = static_cast<Window*>(data);
  double current_time = ewk_view_media_current_time_get(thiz->web_view_);
  log_info("On_Notify_Subtitle_Seek_Start   current_time: %f", current_time);
}

void Window::On_Notify_Subtitle_Seek_Completed(void* data,
                                               Evas_Object*,
                                               void* event_info) {
  log_trace("%s", __PRETTY_FUNCTION__);
  Window* thiz = static_cast<Window*>(data);
  double current_time = ewk_view_media_current_time_get(thiz->web_view_);
  log_info("On_Notify_Subtitle_Seek_Completed   current_time: %f",
           current_time);
}

void Window::On_Subtitle_Data(void* data, Evas_Object*, void* event_info) {
  log_trace("%s", __PRETTY_FUNCTION__);
  Ewk_Media_Subtitle_Data* subtitleData = (Ewk_Media_Subtitle_Data*)event_info;
  int id = ewk_media_subtitle_data_id_get(subtitleData);
  double timestamp = ewk_media_subtitle_data_timestamp_get(subtitleData);
  unsigned size = ewk_media_subtitle_data_size_get(subtitleData);
  const char* subtitile_data =
      (const char*)(ewk_media_subtitle_data_get(subtitleData));
  log_trace(
      "<< notify subtitle data : id(%d), ts(%f), size(%d), "
      "subtitile_data(%s)\n",
      id, timestamp, size, subtitile_data);
}
#endif

void Window::OnEnterFullScreenRequest(void* data, Evas_Object*,
                                      void* event_info) {
  log_trace("%s", __PRETTY_FUNCTION__);
  Window* thiz = static_cast<Window*>(data);
  thiz->is_fullscreen_ = true;
}

void Window::OnExitFullScreenRequest(void* data, Evas_Object*,
                                     void* event_info) {
  log_trace("%s", __PRETTY_FUNCTION__);
  Window* thiz = static_cast<Window*>(data);
  thiz->is_fullscreen_ = false;
}

void Window::OnBeforeFormRepostWarningShow(void* data,
                                           Evas_Object*,
                                           void* event_info) {
  log_trace("%s", __PRETTY_FUNCTION__);
  Window* thiz = static_cast<Window*>(data);
  thiz->ui_->ClosePopup();
}

void Window::OnUserFormRepostDecisionTaken(bool decision, void* data) {
  log_trace("%s", __PRETTY_FUNCTION__);
  Ewk_Form_Repost_Decision_Request* request =
      static_cast<Ewk_Form_Repost_Decision_Request*>(data);
  ewk_form_repost_decision_request_reply(request, decision);
}

void Window::OnFormRepostWarningShow(void* data,
                                     Evas_Object*,
                                     void* event_info) {
  log_trace("%s", __PRETTY_FUNCTION__);
  Ewk_Form_Repost_Decision_Request* request =
      static_cast<Ewk_Form_Repost_Decision_Request*>(event_info);
  Window* thiz = static_cast<Window*>(data);
  thiz->ui_->ShowPermissionPopup(
      "form repost warning",
      "The page that you're looking for used information that you entered. "
      "Returning to that page might cause any action that you took to be "
      "repeated. Do you want to continue?",
      OnUserFormRepostDecisionTaken, request);
}

#if defined(OS_TIZEN)
void Window::OnKeyDown(void* data,
                       Evas_Object*,
                       Evas_Object* obj,
                       void* event_info) {
  if (IsTvProfile()) {
    log_trace("%s", __PRETTY_FUNCTION__);
    Window* thiz = static_cast<Window*>(data);

    if (!event_info)
      return;

    const Evas_Event_Key_Down* key_down =
        static_cast<Evas_Event_Key_Down*>(event_info);

    if (!key_down->key)
      return;
    if (!strcmp(key_down->key, "XF86Back"))
      thiz->Back();
  }
}
#endif

#if defined(OS_TIZEN_TV_PRODUCT)
void Window::CreateMouseCursor() {
  log_trace("%s", __PRETTY_FUNCTION__);
#if TIZEN_VERSION_AT_LEAST(5, 0, 0)
  Ecore_Wl2_Display* wl2_display = ecore_wl2_connected_display_get(NULL);
  struct wl_display* display = ecore_wl2_display_get(wl2_display);
  Eina_Iterator* globals = ecore_wl2_display_globals_get(wl2_display);
  struct wl_registry* registry = ecore_wl2_display_registry_get(wl2_display);
  struct wl_seat* seat =
      ecore_wl2_input_seat_get(ecore_wl2_input_default_input_get(wl2_display));

  Ecore_Wl2_Global* global;
  EINA_ITERATOR_FOREACH(globals, global) {
    if (!strcmp(global->interface, "tizen_cursor"))
      if (!CursorModule_Initialize(display, registry, seat, global->id))
        log_error("CursorModule_Initialize() Failed!\n");
  }
  eina_iterator_free(globals);

  struct wl_surface* const surface =
      ecore_wl2_window_surface_get(ecore_evas_wayland2_window_get(
          ecore_evas_ecore_evas_get(evas_object_evas_get(window_))));
  ecore_wl2_sync();

  if (!Cursor_Set_Config(surface, TIZEN_CURSOR_CONFIG_CURSOR_AVAILABLE, NULL))
    log_error("Cursor_Set_Config() Failed!\n");
  CursorModule_Finalize();
#else
  Ecore_Wl_Global* global_data = NULL;
  unsigned int cursormgr_id = 0;

  EINA_INLIST_FOREACH(ecore_wl_globals_get(), global_data) {
    if (0 == strcmp(global_data->interface, "tizen_cursor")) {
      cursormgr_id = global_data->id;  // to get the id of cursormgr object
      break;
    }
  }

  wl_surface* surface =
      ecore_wl_window_surface_get(elm_win_wl_window_get(window_));
  ecore_wl_sync();

  int ret_cursormod = CursorModule_Initialize(
      ecore_wl_display_get(), ecore_wl_registry_get(),
      ecore_wl_input_seat_get(ecore_wl_input_get()), cursormgr_id);
  if (ret_cursormod) {
    Cursor_Set_Config(surface, TIZEN_CURSOR_CONFIG_CURSOR_AVAILABLE, NULL);
  }
  CursorModule_Finalize();
#endif  // TIZEN_VERSION_AT_LEAST(5, 0, 0)
}
#endif

void Window::OnUserMediaPermissionRequest(void* data,
                                          Evas_Object*,
                                          void* event_info) {
  log_trace("%s", __PRETTY_FUNCTION__);
  Window* thiz = static_cast<Window*>(data);

  Ewk_User_Media_Permission_Request* permissionRequest =
      (Ewk_User_Media_Permission_Request*)event_info;

  ewk_user_media_permission_request_suspend(permissionRequest);

  thiz->ui_->ShowPermissionPopup(
      "Media permission request",
      "Do you want to allow this page to access Media Resources?",
      OnUserMediaPermissionDecisionTaken,
      permissionRequest);
}

void Window::OnUserMediaPermissionDecisionTaken(bool decision,
                                                void* data) {
  log_trace("%s", __PRETTY_FUNCTION__);

  Ewk_User_Media_Permission_Request* permissionRequest =
      (Ewk_User_Media_Permission_Request*)data;

  ewk_user_media_permission_request_set(permissionRequest, decision);
}

void Window::OnWindowDelRequest(void* data, Evas_Object*, void*) {
  Window* thiz = static_cast<Window*>(data);
  thiz->browser_.OnWindowDestroyed(thiz->Id());
}

void Window::OnNewWindowRequest(void *data, Evas_Object*, void* out_view) {
  log_trace("%s", __PRETTY_FUNCTION__);
  Window* thiz = static_cast<Window*>(data);
  Window& new_window = thiz->browser_.CreateWindow();
  *static_cast<Evas_Object**>(out_view) = new_window.web_view_;
}

void Window::OnTitleChanged(void *data, Evas_Object *obj, void *arg) {
  Window* thiz = static_cast<Window*>(data);
  elm_win_title_set(thiz->window_, static_cast<char*>(arg));
  log_trace("%s: %s", __PRETTY_FUNCTION__, static_cast<char*>(arg));
  thiz->browser_.OnWindowTitleChanged(thiz->Id(), static_cast<char *>(arg));
}

void Window::OnURLChanged(void *data, Evas_Object *obj, void *arg) {
  Window* thiz = static_cast<Window*>(data);
  log_trace("%s: %s", __PRETTY_FUNCTION__, static_cast<char*>(arg));
  elm_win_title_set(thiz->window_, static_cast<char*>(arg));
  thiz->ui_->OnURLChanged(static_cast<char*>(arg));
}

void Window::OnLoadStarted(void* data, Evas_Object*, void*) {
  Window* thiz = static_cast<Window*>(data);
  log_trace("%s", __PRETTY_FUNCTION__);
  thiz->browser_.OnWindowLoadStarted(thiz->Id());
  thiz->ui_->OnLoadingStarted();
}

void Window::OnLoadFinished(void* data, Evas_Object*, void*) {
  Window* thiz = static_cast<Window*>(data);
  log_trace("%s", __PRETTY_FUNCTION__);
  thiz->browser_.OnWindowLoadFinished(thiz->Id());
  thiz->ui_->OnLoadingFinished();
}

void Window::OnConsoleMessage(void*, Evas_Object*, void* event_info) {
  Ewk_Console_Message* msg = static_cast<Ewk_Console_Message*>(event_info);

  MESSAGE_TYPE type;
  switch (ewk_console_message_level_get(msg)) {
  case EWK_CONSOLE_MESSAGE_LEVEL_ERROR:
    type = ERROR;
    break;
  case EWK_CONSOLE_MESSAGE_LEVEL_WARNING:
    type = WARNING;
    break;
  case EWK_CONSOLE_MESSAGE_LEVEL_DEBUG:
    type = DEBUG;
    break;
  default:
    type = INFO;
  }

  log_message(type, true, "[JS Console] [%s:%d] %s",
              ewk_console_message_source_get(msg),
              ewk_console_message_line_get(msg),
              ewk_console_message_text_get(msg));
}

void Window::OnOrientationChanged(void* data, Evas_Object *obj, void* event) {
  Window* thiz = static_cast<Window*>(data);
  log_trace("%s", __PRETTY_FUNCTION__);

  int rotation = elm_win_rotation_get(thiz->window_);
  // ewk_view_orientation_send expects angles: 0, 90, -90, 180.
  if (rotation == 270)
    rotation = 90;
  else if(rotation == 90)
    rotation = -90;
  ewk_view_orientation_send(thiz->web_view_, rotation);
}

#if defined(OS_TIZEN)
void Window::OnHWBack(void* data, Evas_Object*, void*) {
  Window* thiz = static_cast<Window*>(data);
  log_trace("%s", __PRETTY_FUNCTION__);

  thiz->Back();
}
#endif

void Window::OnNewWindowPolicyDecide(void*, Evas_Object*, void* policy) {
  log_trace("%s", __PRETTY_FUNCTION__);
  Ewk_Policy_Decision *policy_decision = static_cast<Ewk_Policy_Decision*>(policy);
  const char* url = ewk_policy_decision_url_get(policy_decision);
  log_info("Allowing new window for: %s", url);
  ewk_policy_decision_use(policy_decision);
}

void Window::OnBackForwardListChanged(void* data, Evas_Object*, void* policy) {
  log_trace("%s", __PRETTY_FUNCTION__);
  Window* thiz = static_cast<Window*>(data);
  thiz->ui_->EnableBackButton(ewk_view_back_possible(thiz->web_view_));
  thiz->ui_->EnableForwardButton(ewk_view_forward_possible(thiz->web_view_));
}

void Window::OnQuotaPermissionRequest(Evas_Object*, const Ewk_Quota_Permission_Request* request, void*) {
  log_trace("%s", __PRETTY_FUNCTION__);
  ewk_view_quota_permission_request_reply(request, true);
}

void Window::OnThemeColorChanged(Evas_Object* o,
                                 int r,
                                 int g,
                                 int b,
                                 int a,
                                 void* data) {
  Window* thiz = static_cast<Window*>(data);
  thiz->ui_->OnThemeColorChanged(r, g, b, a);
}

void Window::OnHostFocusedIn(void* data, Evas_Object*, void*) {
  log_trace("%s", __PRETTY_FUNCTION__);
  Window* thiz = static_cast<Window*>(data);
  ewk_view_focus_set(thiz->web_view_, EINA_TRUE);
}

void Window::OnHostFocusedOut(void* data, Evas_Object*, void*) {
  log_trace("%s", __PRETTY_FUNCTION__);
  Window* thiz = static_cast<Window*>(data);
  ewk_view_focus_set(thiz->web_view_, EINA_FALSE);
}


void Window::SetGoogleDataProxyHeaders() const {
  ewk_view_custom_header_add(
      web_view_, "Chrome-Proxy",
      "ps=1477491020-304034222-240280515-829909815, "
      "sid=b37590d308108e0b7a229352dee265d8, b=2704, p=103, c=linux");
  ewk_view_custom_header_add(web_view_, "accept", "image/webp,*/*;q=0.8");
}

void Window::Exit() const {
  browser_.Exit();
}
