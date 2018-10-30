/*
 * Copyright (C) 2009-2010 ProFUSION embedded systems
 * Copyright (C) 2009-2016 Samsung Electronics. All rights reserved.
 * Copyright (C) 2012 Intel Corporation
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "ewk_view_product.h"

#include <algorithm>
#include <Evas.h>

#include "authentication_challenge_popup.h"
#include "base/trace_event/ttrace.h"
#include "content/public/common/web_preferences.h"
#include "content/public/browser/navigation_controller.h"
#include "cookie_manager.h"
#include "eweb_view.h"
#include "geolocation_permission_popup.h"
#include "notification_permission_popup.h"
#include "public/ewk_back_forward_list.h"
#include "public/ewk_context.h"
#include "public/ewk_enums_internal.h"
#include "public/ewk_settings.h"
#include "private/ewk_context_private.h"
#include "private/ewk_frame_private.h"
#include "private/ewk_hit_test_private.h"
#include "private/ewk_notification_private.h"
#include "private/ewk_private.h"
#include "private/ewk_quota_permission_request_private.h"
#include "private/ewk_back_forward_list_private.h"
#include "private/ewk_history_private.h"
#include "private/ewk_view_private.h"
// FIXME: removed on m63
//#include "third_party/WebKit/public/web/WebViewModeEnums.h"
#include "ui/events/gesture_detection/gesture_configuration.h"
#include "url/gurl.h"
#include "usermedia_permission_popup.h"
#include "web_contents_delegate_efl.h"

#if defined(TIZEN_VIDEO_HOLE)
#include "media/base/tizen/media_player_efl.h"
#endif

static Eina_Bool _ewk_view_default_user_media_permission(
    Evas_Object*, Ewk_User_Media_Permission_Request*, void*);

static Eina_Bool _ewk_view_default_geolocation_permission(
    Evas_Object*, Ewk_Geolocation_Permission_Request*, void*);

static Eina_Bool _ewk_view_default_notification_permission(
    Evas_Object*, Ewk_Notification_Permission_Request*, void*);

static void _ewk_view_default_authentication(
    Evas_Object*, Ewk_Auth_Challenge*, void*);

/* LCOV_EXCL_START */
Eina_Bool ewk_view_smart_class_set(Ewk_View_Smart_Class* api)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(api, false);
  return InitSmartClassInterface(*api);
}

// TODO: Evas_Object *ewk_view_smart_add(Evas *e, Evas_Smart *smart, Ewk_Context *context, Ewk_Page_Group *pageGroup)

Evas_Object* ewk_view_add_with_session_data(Evas* canvas, const char* data, unsigned length)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(canvas, NULL);

  Evas_Object* ret = ewk_view_add(canvas);

  EWebView *webView = GetWebViewFromEvasObject(ret);

  if (!webView || !data || !length)
    return ret;

  if (webView->RestoreFromSessionData(data, length))
    return ret;

  evas_object_del(ret);

  return NULL;
}
/* LCOV_EXCL_STOP */

Evas_Object* ewk_view_add_with_context(Evas* e, Ewk_Context* context)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(context, 0);
  Evas_Object* ewk_view = CreateWebViewAsEvasObject(context, e);

  if (ewk_view) {
#if !defined(OS_TIZEN_TV_PRODUCT)
    ewk_view_user_media_permission_callback_set(ewk_view,
        _ewk_view_default_user_media_permission, 0);
#endif
    ewk_view_geolocation_permission_callback_set(ewk_view,
        _ewk_view_default_geolocation_permission, 0);
    ewk_view_notification_permission_callback_set(ewk_view,
        _ewk_view_default_notification_permission, 0);
    ewk_view_authentication_callback_set(ewk_view,
        _ewk_view_default_authentication, nullptr);
  }

  return ewk_view;
}

Evas_Object* ewk_view_add(Evas* e)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(e, NULL);
  // TODO: shouldn't this function create new EWebContext for each new EWebView?
  // when using default context like that it makes unclear who should release
  // default web context. It won't be released by destroyed eweb_view because
  // ewk_context_default_get does AddRef
  Ewk_Context* context = ewk_context_default_get();
  return ewk_view_add_with_context(e, context);
}

Evas_Object* ewk_view_add_in_incognito_mode(Evas* e)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(e, nullptr);
  Ewk_Context* context = Ewk_Context::IncognitoContext();
  EINA_SAFETY_ON_NULL_RETURN_VAL(context, nullptr);
  return ewk_view_add_with_context(e, context);
}

Ewk_Context *ewk_view_context_get(const Evas_Object *view)
{
  EWK_VIEW_IMPL_GET_OR_RETURN(view, impl, 0);
  return static_cast<Ewk_Context*>(impl->context());
}

Eina_Bool ewk_view_url_set(Evas_Object* view, const char* url_string)
{
  TTRACE_WEB("ewk_view_url_set url: %s", url_string);
  EWK_VIEW_IMPL_GET_OR_RETURN(view, impl, false);
  EINA_SAFETY_ON_NULL_RETURN_VAL(url_string, false);
  GURL url(url_string);
  impl->SetURL(url);
  return true;
}

const char* ewk_view_url_get(const Evas_Object* view)
{
  EWK_VIEW_IMPL_GET_OR_RETURN(view, impl, 0);
  return impl->GetURL().possibly_invalid_spec().c_str();  // LCOV_EXCL_LINE
}

const char* ewk_view_original_url_get(const Evas_Object* view)
{
  EWK_VIEW_IMPL_GET_OR_RETURN(view, impl, 0);
  /* LCOV_EXCL_START */
  return impl->GetOriginalURL().possibly_invalid_spec().c_str();
  /* LCOV_EXCL_STOP */
}

Eina_Bool ewk_view_reload(Evas_Object *view)
{
  EWK_VIEW_IMPL_GET_OR_RETURN(view, impl, false);
  /* LCOV_EXCL_START */
  impl->Reload();
  return true;
  /* LCOV_EXCL_STOP */
}

/* LCOV_EXCL_START */
Eina_Bool ewk_view_reload_bypass_cache(Evas_Object *view)
{
  EWK_VIEW_IMPL_GET_OR_RETURN(view, impl, false);
  impl->ReloadBypassingCache();
  return true;
}
/* LCOV_EXCL_STOP */

Eina_Bool ewk_view_stop(Evas_Object* view)
{
  EWK_VIEW_IMPL_GET_OR_RETURN(view, impl, false);
  /* LCOV_EXCL_START */
  impl->Stop();
  return true;
  /* LCOV_EXCL_STOP */
}


Ewk_Settings *ewk_view_settings_get(const Evas_Object *ewkView)
{
  EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl,0);
  return impl->GetSettings();
}

const char* ewk_view_title_get(const Evas_Object* view)
{
  EWK_VIEW_IMPL_GET_OR_RETURN(view, impl, 0);
  return impl->GetTitle();  // LCOV_EXCL_LINE
}

double ewk_view_load_progress_get(const Evas_Object* view)
{
  EWK_VIEW_IMPL_GET_OR_RETURN(view, impl, -1);
  return impl->GetProgressValue();
}

Eina_Bool ewk_view_scale_set(Evas_Object* view, double scale_factor, int x, int y)
{
  EWK_VIEW_IMPL_GET_OR_RETURN(view, impl, EINA_FALSE);
  /* LCOV_EXCL_START */
  impl->SetScale(scale_factor);
  impl->SetScroll(x, y);
  return EINA_TRUE;
  /* LCOV_EXCL_STOP */
}

/* LCOV_EXCL_START */
void ewk_view_scale_changed_callback_set(Evas_Object* view,
    Ewk_View_Scale_Changed_Callback callback,
    void* user_data)
{
  EWK_VIEW_IMPL_GET_OR_RETURN(view, impl);
  impl->SetScaleChangedCallback(callback, user_data);
}
/* LCOV_EXCL_STOP */

double ewk_view_scale_get(const Evas_Object *view)
{
  EWK_VIEW_IMPL_GET_OR_RETURN(view, impl, -1);
  return impl->GetScale();  // LCOV_EXCL_LINE
}

Eina_Bool ewk_view_back(Evas_Object *view)
{
  EWK_VIEW_IMPL_GET_OR_RETURN(view, impl, false);
  return impl->GoBack();  // LCOV_EXCL_LINE
}

Eina_Bool ewk_view_forward(Evas_Object *view)
{
  EWK_VIEW_IMPL_GET_OR_RETURN(view, impl, false);
  return impl->GoForward();  // LCOV_EXCL_LINE
}

Eina_Bool ewk_view_back_possible(Evas_Object *view)
{
  EWK_VIEW_IMPL_GET_OR_RETURN(view, impl, false);
  return impl->CanGoBack();
}

Eina_Bool ewk_view_forward_possible(Evas_Object *view)
{
  EWK_VIEW_IMPL_GET_OR_RETURN(view, impl, false);
  return impl->CanGoForward();
}

/* LCOV_EXCL_START */
Eina_Bool ewk_view_web_login_request(Evas_Object* ewkView)
{
  LOG_EWK_API_MOCKUP();
  return false;
}
/* LCOV_EXCL_STOP */

Eina_Bool ewk_view_html_string_load(Evas_Object* view, const char* html, const char* base_uri, const char* unreachable_uri)
{
  EWK_VIEW_IMPL_GET_OR_RETURN(view, impl, EINA_FALSE);
  EINA_SAFETY_ON_NULL_RETURN_VAL(html, EINA_FALSE);
  /* LCOV_EXCL_START */
  impl->LoadHTMLString(html, base_uri, unreachable_uri);
  return EINA_TRUE;
  /* LCOV_EXCL_STOP */
}

/* LCOV_EXCL_START */
Eina_Bool ewk_view_mouse_events_enabled_set(Evas_Object *view, Eina_Bool enabled)
{
  EWK_VIEW_IMPL_GET_OR_RETURN(view, impl, false);

  impl->SetMouseEventsEnabled(!!enabled);

  return true;
}

Eina_Bool ewk_view_mouse_events_enabled_get(const Evas_Object *view)
{
  EWK_VIEW_IMPL_GET_OR_RETURN(view, impl, false);

  return impl->MouseEventsEnabled();
}

Eina_Bool ewk_view_color_picker_color_set(Evas_Object* ewkView, int r, int g, int b, int a)
{
  LOG_EWK_API_MOCKUP();
  return false;
}

double ewk_view_text_zoom_get(const Evas_Object* view)
{
  EWK_VIEW_IMPL_GET_OR_RETURN(view, impl, -1.0);
  return impl->GetTextZoomFactor();
}

Eina_Bool ewk_view_text_zoom_set(Evas_Object* view, double text_zoom_level)
{
  EWK_VIEW_IMPL_GET_OR_RETURN(view, impl, false);
  if (ewk_settings_text_zoom_enabled_get(ewk_view_settings_get(view))) {
   impl->SetTextZoomFactor(text_zoom_level);
   return true;
  }
  return false;
}

void ewk_view_not_found_error_page_load(Evas_Object* ewkView, const char* errorUrl)
{
  EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl);
  impl->LoadNotFoundErrorPage(std::string(errorUrl));
}

void ewk_view_scale_range_get(Evas_Object* view, double* min_scale, double* max_scale)
{
  if (!min_scale && !max_scale)
    return;

  EWK_VIEW_IMPL_GET_OR_RETURN(view, impl);
  impl->GetPageScaleRange(min_scale, max_scale);
}
/* LCOV_EXCL_STOP */

void ewk_view_suspend(Evas_Object* ewkView)
{
  EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl);
  impl->Suspend();  // LCOV_EXCL_LINE
}

void ewk_view_resume(Evas_Object* ewkView)
{
  EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl);
  impl->Resume();  // LCOV_EXCL_LINE
}

Eina_Bool ewk_view_url_request_set(Evas_Object* ewkView, const char* url, Ewk_Http_Method method, Eina_Hash* headers, const char* body)
{
  EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, EINA_FALSE);
  EINA_SAFETY_ON_NULL_RETURN_VAL(url, EINA_FALSE);
  content::NavigationController::LoadURLType loadtype;
  /* LCOV_EXCL_START */
  switch (method) {
  case EWK_HTTP_METHOD_GET:
    loadtype = content::NavigationController::LOAD_TYPE_DEFAULT;
    break;
  case EWK_HTTP_METHOD_POST:
    loadtype = content::NavigationController::LOAD_TYPE_HTTP_POST;
    break;
  default:
    LOG(ERROR) << "Not supported HTTP Method.";
    return EINA_FALSE;
  }

  impl->UrlRequestSet(url, loadtype, headers, body);
  return EINA_TRUE;
  /* LCOV_EXCL_START */
}

/* LCOV_EXCL_START */
Eina_Bool ewk_view_plain_text_set(Evas_Object* view, const char* plain_text)
{
  EWK_VIEW_IMPL_GET_OR_RETURN(view, impl, EINA_FALSE);
  EINA_SAFETY_ON_NULL_RETURN_VAL(plain_text, EINA_FALSE);
  impl->LoadPlainTextString(plain_text);
  return EINA_TRUE;
}
/* LCOV_EXCL_STOP */

Eina_Bool ewk_view_contents_set(Evas_Object* view, const char* contents, size_t contents_size, char* mime_type, char* encoding, char* base_uri)
{
  EWK_VIEW_IMPL_GET_OR_RETURN(view, impl, EINA_FALSE);
  EINA_SAFETY_ON_NULL_RETURN_VAL(contents, EINA_FALSE);
  impl->LoadData(contents, contents_size, mime_type, encoding, base_uri);
  return EINA_TRUE;
}

/* LCOV_EXCL_START */
Eina_Bool ewk_view_html_contents_set(Evas_Object* view, const char* html, const char* base_uri)
{
  return ewk_view_html_string_load(view, html, base_uri, NULL);
}

Eina_Bool ewk_view_page_visibility_state_set(Evas_Object* ewkView, Ewk_Page_Visibility_State page_visibility_state, Eina_Bool initial_state)
{
  // TODO: Should we send initial_state to |Page::setVisibilityState()|?.
  // isInitialState parameter seems be true only in the constructor of WebViewImpl.
  EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, false);

  return impl->SetPageVisibility(page_visibility_state);
}
/* LCOV_EXCL_STOP */

Eina_Bool ewk_view_user_agent_set(Evas_Object* ewkView, const char* user_agent)
{
  EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, false);

  return impl->SetUserAgent(user_agent);
}

const char* ewk_view_user_agent_get(const Evas_Object* ewkView)
{
  EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, 0);
  return impl->GetUserAgent();
}

/* LCOV_EXCL_START */
Eina_Bool ewk_view_application_name_for_user_agent_set(Evas_Object* ewkView, const char* application_name)
{
  EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, false);
  return impl->SetUserAgentAppName(application_name);
}

const char* ewk_view_application_name_for_user_agent_get(const Evas_Object* ewkView)
{
  EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, 0);
  return eina_stringshare_add(impl->GetUserAgentAppName());
}

Eina_Bool ewk_view_custom_header_add(const Evas_Object* ewkView, const char* name, const char* value)
{
  EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, EINA_FALSE);
  EINA_SAFETY_ON_NULL_RETURN_VAL(name, EINA_FALSE);
  EINA_SAFETY_ON_NULL_RETURN_VAL(value, EINA_FALSE);
  EINA_SAFETY_ON_NULL_RETURN_VAL(impl->context(),EINA_FALSE);
  return impl->context()->HTTPCustomHeaderAdd(name, value);
}

Eina_Bool ewk_view_custom_header_remove(const Evas_Object* ewkView, const char* name)
{
  EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, EINA_FALSE);
  EINA_SAFETY_ON_NULL_RETURN_VAL(name, EINA_FALSE);
  EINA_SAFETY_ON_NULL_RETURN_VAL(impl->context(),EINA_FALSE);
  return impl->context()->HTTPCustomHeaderRemove(name);

}

Eina_Bool ewk_view_custom_header_clear(const Evas_Object* ewkView)
{
  EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, EINA_FALSE);
  EINA_SAFETY_ON_NULL_RETURN_VAL(impl->context(),EINA_FALSE);
  impl->context()->HTTPCustomHeaderClear();
  return EINA_TRUE;
}
/* LCOV_EXCL_STOP */

Eina_Bool ewk_view_visibility_set(Evas_Object* view, Eina_Bool enable)
{
  EWK_VIEW_IMPL_GET_OR_RETURN(view, impl, EINA_FALSE);

  return impl->SetVisibility(enable);
}

/* LCOV_EXCL_START */
Evas_Object* ewk_view_screenshot_contents_get(const Evas_Object* view, Eina_Rectangle view_area, float scale_factor, Evas* canvas)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(canvas, NULL);
  EWK_VIEW_IMPL_GET_OR_RETURN(view, impl, NULL);
  return impl->GetSnapshot(view_area, scale_factor);
}

Eina_Bool ewk_view_screenshot_contents_get_async(const Evas_Object* view, Eina_Rectangle view_area,
        float scale_factor, Evas* canvas, Ewk_Web_App_Screenshot_Captured_Callback callback, void* user_data)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(callback, EINA_FALSE);
  EWK_VIEW_IMPL_GET_OR_RETURN(view, impl, EINA_FALSE);
  return impl->GetSnapshotAsync(view_area, callback, user_data, scale_factor)
         ? EINA_TRUE
         : EINA_FALSE;
}

unsigned int ewk_view_inspector_server_start(Evas_Object* ewkView, unsigned int port)
{
  EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, false);
  return impl->StartInspectorServer(port);
}

Eina_Bool ewk_view_inspector_server_stop(Evas_Object* ewkView)
{
  EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, false);
  return impl->StopInspectorServer();
}

Eina_Bool ewk_view_cache_image_get(const Evas_Object* view, const char* image_url, Evas* canvas,
                                   Ewk_View_Cache_Image_Get_Callback callback, void* user_data)
{
  LOG_EWK_API_MOCKUP();
  return false;
}

void ewk_view_scroll_by(Evas_Object* ewkView, int deltaX, int deltaY)
{
  int x, y;

  if (EINA_TRUE == ewk_view_scroll_pos_get(ewkView, &x, &y)) {
    ewk_view_scroll_set(ewkView, x + deltaX, y + deltaY);
  }
}
/* LCOV_EXCL_STOP */

Eina_Bool ewk_view_scroll_pos_get(Evas_Object* ewkView, int* x, int* y)
{
  EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, false);
  return impl->GetScrollPosition(x, y);  // LCOV_EXCL_LINE
}

Eina_Bool ewk_view_scroll_set(Evas_Object* view, int x, int y)
{
  EWK_VIEW_IMPL_GET_OR_RETURN(view, impl, EINA_FALSE);
  /* LCOV_EXCL_START */
  impl->SetScroll(x, y);

  return EINA_TRUE;
  /* LCOV_EXCL_STOP */
}

/* LCOV_EXCL_START */
Eina_Bool ewk_view_scroll_size_get(const Evas_Object* view, int* width, int* height)
{
  if (width)
    *width = 0;
  if (height)
    *height = 0;
  EWK_VIEW_IMPL_GET_OR_RETURN(view, impl, EINA_FALSE);
  impl->GetScrollSize(width,height);
  return EINA_TRUE;
}

void ewk_view_password_confirm_popup_callback_set(Evas_Object* view, Ewk_View_Password_Confirm_Popup_Callback callback, void* user_data)
{
  LOG_EWK_API_MOCKUP();
}

void ewk_view_password_confirm_popup_reply(Evas_Object* ewkView, Ewk_Password_Popup_Option result)
{
  LOG_EWK_API_MOCKUP();
}

void ewk_view_javascript_alert_callback_set(Evas_Object* view, Ewk_View_JavaScript_Alert_Callback callback, void* user_data)
{
  EWK_VIEW_IMPL_GET_OR_RETURN(view, impl);
  if (callback)
    impl->SetJavaScriptAlertCallback(callback, user_data);
}

void ewk_view_javascript_alert_reply(Evas_Object* view)
{
  EWK_VIEW_IMPL_GET_OR_RETURN(view, impl);
  impl->JavaScriptAlertReply();
}

void ewk_view_javascript_confirm_callback_set(Evas_Object* view, Ewk_View_JavaScript_Confirm_Callback callback, void* user_data)
{
  EWK_VIEW_IMPL_GET_OR_RETURN(view, impl);
  if (callback)
    impl->SetJavaScriptConfirmCallback(callback, user_data);
}

void ewk_view_javascript_confirm_reply(Evas_Object* view, Eina_Bool result)
{
  EWK_VIEW_IMPL_GET_OR_RETURN(view, impl);
  impl->JavaScriptConfirmReply(result);
}

void ewk_view_javascript_prompt_callback_set(Evas_Object* view, Ewk_View_JavaScript_Prompt_Callback callback, void* user_data)
{
  EWK_VIEW_IMPL_GET_OR_RETURN(view, impl);
  if (callback)
    impl->SetJavaScriptPromptCallback(callback, user_data);
}

void ewk_view_javascript_prompt_reply(Evas_Object* view, const char* result)
{
  EWK_VIEW_IMPL_GET_OR_RETURN(view, impl);
  impl->JavaScriptPromptReply(result);
}

void ewk_view_before_unload_confirm_panel_callback_set(Evas_Object* ewkView, Ewk_View_Before_Unload_Confirm_Panel_Callback callback, void* userData)
{
  EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl);
  EINA_SAFETY_ON_NULL_RETURN(callback);
  impl->SetBeforeUnloadConfirmPanelCallback(callback, userData);
}

void ewk_view_before_unload_confirm_panel_reply(Evas_Object* ewkView, Eina_Bool result)
{
  EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl);
  impl->ReplyBeforeUnloadConfirmPanel(result);
}

Eina_Bool ewk_view_web_application_capable_get(Evas_Object* ewkView, Ewk_Web_App_Capable_Get_Callback callback, void* userData)
{
  EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, EINA_FALSE);
  EINA_SAFETY_ON_NULL_RETURN_VAL(callback, EINA_FALSE);
  return impl->WebAppCapableGet(callback, userData) ? EINA_TRUE : EINA_FALSE;
}

Eina_Bool ewk_view_web_application_icon_url_get(Evas_Object* ewkView, Ewk_Web_App_Icon_URL_Get_Callback callback, void* userData)
{
  EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, EINA_FALSE);
  EINA_SAFETY_ON_NULL_RETURN_VAL(callback, EINA_FALSE);
  return impl->WebAppIconUrlGet(callback, userData) ? EINA_TRUE : EINA_FALSE;
}

Eina_Bool ewk_view_web_application_icon_urls_get(Evas_Object* ewkView, Ewk_Web_App_Icon_URLs_Get_Callback callback, void* userData)
{
  EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, EINA_FALSE);
  EINA_SAFETY_ON_NULL_RETURN_VAL(callback, EINA_FALSE);
  return impl->WebAppIconUrlsGet(callback, userData) ? EINA_TRUE : EINA_FALSE;
}

Eina_Bool ewk_view_command_execute(Evas_Object* view, const char* command, const char* value)
{
  EWK_VIEW_IMPL_GET_OR_RETURN(view, impl, false);
  EINA_SAFETY_ON_NULL_RETURN_VAL(command, false);
  impl->ExecuteEditCommand(command, value);
  return true;
}
/* LCOV_EXCL_STOP */

Eina_Bool ewk_view_contents_size_get(const Evas_Object* view, Evas_Coord* width, Evas_Coord* height)
{
  if (width)
    *width = 0;
  if (height)
    *height = 0;

  EWK_VIEW_IMPL_GET_OR_RETURN(view, impl, EINA_FALSE);
  /* LCOV_EXCL_START */
  Eina_Rectangle contents_size = impl->GetContentsSize();

  if (width)
    *width = contents_size.w;
  if (height)
    *height = contents_size.h;

  return EINA_TRUE;
  /* LCOV_EXCL_STOP */
}

/* LCOV_EXCL_START */
Eina_Bool ewk_view_contents_pdf_get(Evas_Object* view, int width, int height, const char* fileName)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(fileName, EINA_FALSE);
  EWK_VIEW_IMPL_GET_OR_RETURN(view, impl, EINA_FALSE);
  return impl->SaveAsPdf(width, height, fileName);
}
/* LCOV_EXCL_STOP */

Eina_Bool ewk_view_script_execute(Evas_Object* ewkView, const char* script, Ewk_View_Script_Execute_Callback callback, void* user_data)
{
  EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, false);
  EINA_SAFETY_ON_NULL_RETURN_VAL(script, false);
  /* LCOV_EXCL_START */
  // callback can be null, so do not test it for null
  if (0 != strcmp(script, "")) //check for empty string
    return impl->ExecuteJavaScript(script, callback, user_data);
  return false;
  /* LCOV_EXCL_STOP */
}

/* LCOV_EXCL_START */
Eina_Bool ewk_view_script_execute_all_frames(Evas_Object *view, const char *script, Ewk_View_Script_Execute_Callback callback, void *user_data)
{
#if defined(OS_TIZEN_TV_PRODUCT)
  EWK_VIEW_IMPL_GET_OR_RETURN(view, impl, EINA_FALSE);
  EINA_SAFETY_ON_NULL_RETURN_VAL(script, EINA_FALSE);

  LOG(INFO) << __FUNCTION__ << ", view: " << view;
  // callback can be null, so do not test it with null
  if (0 != strcmp(script, ""))
    return impl->ExecuteJavaScriptInAllFrames(script, callback, user_data);
#else
  LOG_EWK_API_MOCKUP("This API is only available in Tizen TV product.");
#endif
  return EINA_FALSE;
}

void ewk_view_floating_window_state_changed(const Evas_Object *view, Eina_Bool status)
{
#if defined(OS_TIZEN_TV_PRODUCT)
  EWK_VIEW_IMPL_GET_OR_RETURN(view, impl);
  LOG(INFO) << __FUNCTION__ << ", view: "<< view << ", status: " << status;
  impl->SetFloatWindowState(status);
#else
  LOG_EWK_API_MOCKUP("This API is only available in Tizen TV product.");
#endif
}

void ewk_view_auto_login(Evas_Object *view, const char* user_name, const char* password)
{
#if defined(OS_TIZEN_TV_PRODUCT)
  EWK_VIEW_IMPL_GET_OR_RETURN(view, impl);
  LOG(INFO) << "[SPASS] "<< __FUNCTION__;
  impl->AutoLogin(user_name, password);
#else
  LOG_EWK_API_MOCKUP("This API is only available in Tizen TV product.");
#endif
}

Eina_Bool ewk_view_plain_text_get(Evas_Object* view, Ewk_View_Plain_Text_Get_Callback callback, void* user_data)
{
  EWK_VIEW_IMPL_GET_OR_RETURN(view, impl, EINA_FALSE);
  EINA_SAFETY_ON_NULL_RETURN_VAL(callback, EINA_FALSE);
  return (impl->PlainTextGet(callback, user_data));
}

Eina_Bool ewk_view_mhtml_data_get(Evas_Object *view, Ewk_View_MHTML_Data_Get_Callback callback, void *user_data)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(callback, EINA_FALSE);
  EWK_VIEW_IMPL_GET_OR_RETURN(view, impl, EINA_FALSE);
  return impl->GetMHTMLData(callback, user_data);
}

Ewk_Hit_Test* ewk_view_hit_test_new(Evas_Object* ewkView, int x, int y, int hit_test_mode)
{
  EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, 0);
  return impl->RequestHitTestDataAt(x, y,
      static_cast<Ewk_Hit_Test_Mode>(hit_test_mode));
}

Eina_Bool ewk_view_hit_test_request(Evas_Object* o, int x, int y, int hit_test_mode, Ewk_View_Hit_Test_Request_Callback callback, void* user_data)
{
  EWK_VIEW_IMPL_GET_OR_RETURN(o, impl, EINA_FALSE);
  return impl->AsyncRequestHitTestDataAt(x, y, static_cast<Ewk_Hit_Test_Mode>(hit_test_mode), callback, user_data);
}

Ewk_History* ewk_view_history_get(Evas_Object* ewkView)
{
  EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, NULL);
  return impl->GetBackForwardHistory();
}

Eina_Bool ewk_view_notification_closed(Evas_Object* ewkView, Eina_List* notification_list)
{
  return EINA_FALSE;
}

Eina_Bool ewk_view_popup_menu_select(Evas_Object* ewkView, unsigned int selectedIndex)
{
  LOG_EWK_API_MOCKUP();
  return false;
}

Eina_Bool ewk_view_popup_menu_multiple_select(Evas_Object* ewkView, Eina_Inarray* changeList)
{
  LOG_EWK_API_MOCKUP();
  return false;
}
/* LCOV_EXCL_STOP */

void ewk_view_orientation_send(Evas_Object* ewkView, int orientation)
{
  EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl);
  impl->SetOrientation(orientation);
}

/* LCOV_EXCL_START */
void ewk_view_encoding_custom_set(Evas_Object* ewkView, const char* encoding)
{
  // Chromium does not support this feature hence the comment
  LOG_EWK_API_MOCKUP("Not Supported by chromium");
}

Eina_Bool ewk_view_text_selection_range_get(const Evas_Object* view, Eina_Rectangle* left_rect, Eina_Rectangle* right_rect)
{
  EWK_VIEW_IMPL_GET_OR_RETURN(view, impl, EINA_FALSE);
  if (left_rect && right_rect && impl->GetSelectionRange(left_rect, right_rect)) {
    Evas_Coord x, y;
    evas_object_geometry_get(view, &x, &y, 0, 0);
    left_rect->x += x;
    left_rect->y += y;
    right_rect->x += x;
    right_rect->y += y;
    return true;
  }
  return false;
}
/* LCOV_EXCL_STOP */

Eina_Bool ewk_view_text_selection_clear(Evas_Object *view)
{
  EWK_VIEW_IMPL_GET_OR_RETURN(view, impl, EINA_FALSE);
  return impl->ClearSelection();  // LCOV_EXCL_LINE
}

const char* ewk_view_text_selection_text_get(Evas_Object* view)
{
  EWK_VIEW_IMPL_GET_OR_RETURN(view, impl, NULL);
  return impl->CacheSelectedText();  // LCOV_EXCL_LINE
}

/* LCOV_EXCL_START */
void ewk_view_focused_input_element_value_set(Evas_Object* ewkView, const char* value)
{
  LOG_EWK_API_MOCKUP();
}

const char* ewk_view_focused_input_element_value_get(Evas_Object* ewkView)
{
  LOG_EWK_API_MOCKUP();
  return NULL;
}

Eina_Bool ewk_view_horizontal_panning_hold_get(Evas_Object* ewkView)
{
  EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, EINA_FALSE);
  return impl->GetHorizontalPanningHold();
}

void ewk_view_horizontal_panning_hold_set(Evas_Object* ewkView, Eina_Bool hold)
{
  EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl);
  impl->SetHorizontalPanningHold(hold);
}

Eina_Bool ewk_view_vertical_panning_hold_get(Evas_Object* ewkView)
{
  EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, EINA_FALSE);
  return impl->GetVerticalPanningHold();
}

void ewk_view_vertical_panning_hold_set(Evas_Object* view, Eina_Bool hold)
{
  EWK_VIEW_IMPL_GET_OR_RETURN(view, impl);
  impl->SetVerticalPanningHold(hold);
}

void ewk_view_orientation_lock_callback_set(Evas_Object* ewkView, Ewk_Orientation_Lock_Cb func, void* data)
{
  LOG_EWK_API_MOCKUP("This API is deprecated, to use orientation lock/unlock "
      "API please use functions lock_orientation/unlock_orientation provided "
      "by Ewk_View_Smart_Class.");
}

void ewk_view_back_forward_list_clear(const Evas_Object *view)
{
  EWK_VIEW_IMPL_GET_OR_RETURN(view, impl);
  impl->BackForwardListClear();
}

Eina_Bool ewk_view_feed_touch_event(Evas_Object *view, Ewk_Touch_Event_Type type, const Eina_List *points, const Evas_Modifier *modifiers)
{
  EWK_VIEW_IMPL_GET_OR_RETURN(view, impl, false);

  impl->HandleTouchEvents(type, points, modifiers);

  return true;
}

Eina_Bool ewk_view_touch_events_enabled_set(Evas_Object *view, Eina_Bool enabled)
{
  EWK_VIEW_IMPL_GET_OR_RETURN(view, impl, false);

  impl->SetTouchEventsEnabled(!!enabled);

  return true;
}

Eina_Bool ewk_view_touch_events_enabled_get(const Evas_Object *view)
{
  EWK_VIEW_IMPL_GET_OR_RETURN(view, impl, false);

  return impl->TouchEventsEnabled();
}

Ewk_Frame_Ref ewk_view_main_frame_get(Evas_Object* o)
{
  EWK_VIEW_IMPL_GET_OR_RETURN(o, impl, NULL);
  return static_cast<Ewk_Frame_Ref>(impl->GetMainFrame());
}

void ewk_view_content_security_policy_set(Evas_Object* ewkView, const char* policy, Ewk_CSP_Header_Type type)
{
  EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl);
  EINA_SAFETY_ON_NULL_RETURN(policy);
  impl->SetContentSecurityPolicy(policy, type);
}

void ewk_view_application_cache_permission_callback_set(Evas_Object* ewkView, Ewk_View_Applicacion_Cache_Permission_Callback callback, void* userData)
{
  // Chromium does not support this feature hence the comment
  LOG_EWK_API_MOCKUP("Not Supported by chromium");
}

void ewk_view_application_cache_permission_reply(Evas_Object* ewkView, Eina_Bool allow)
{
  // Chromium does not support this feature hence the comment
  LOG_EWK_API_MOCKUP("Not Supported by chromium");
}

void ewk_view_exceeded_indexed_database_quota_callback_set(Evas_Object* view, Ewk_View_Exceeded_Indexed_Database_Quota_Callback callback, void* user_data)
{
  // Chromium does not support quota for Indexed DB only.
  // IndexedDB uses temporary storage that is shared
  // between other features.
  EWK_VIEW_IMPL_GET_OR_RETURN(view, impl);
  impl->SetExceededIndexedDatabaseQuotaCallback(callback, user_data);
}

void ewk_view_exceeded_indexed_database_quota_reply(Evas_Object* view, Eina_Bool allow)
{
  // Chromium does not support quota for Indexed DB only.
  // IndexedDB uses temporary storage that is shared
  // between other features.
  EWK_VIEW_IMPL_GET_OR_RETURN(view, impl);
  impl->ExceededIndexedDatabaseQuotaReply(allow);
}

Eina_Bool ewk_view_text_find(Evas_Object *view, const char *text, Ewk_Find_Options options, unsigned int max_match_count)
{
  // FIXME: We need to implement next options in Ewk_Find_Options struct. (refer to ewk_view.h)
  //         - EWK_FIND_OPTIONS_AT_WORD_STARTS
  //         - EWK_FIND_OPTIONS_TREAT_MEDIAL_CAPITAL_AS_WORD_START
  //         - EWK_FIND_OPTIONS_WRAP_AROUND
  //         - EWK_FIND_OPTIONS_SHOW_OVERLAY
  //         - EWK_FIND_OPTIONS_SHOW_FIND_INDICATOR
  //         - EWK_FIND_OPTIONS_SHOW_HIGHLIGHT (Currently there is no way to control this option. so it is always set)

  // FIXME: Updating of max_match_count is not implemented.

  EWK_VIEW_IMPL_GET_OR_RETURN(view, impl, EINA_FALSE);
  EINA_SAFETY_ON_NULL_RETURN_VAL(text, EINA_FALSE);
  impl->Find(text, options);
  return EINA_TRUE;
}

Eina_Bool ewk_view_text_find_highlight_clear(Evas_Object *view)
{
  EWK_VIEW_IMPL_GET_OR_RETURN(view, impl, EINA_FALSE);
  impl->StopFinding();
  return EINA_TRUE;
}

void ewk_view_exceeded_database_quota_callback_set(Evas_Object* ewkView, Ewk_View_Exceeded_Database_Quota_Callback callback, void* userData)
{
  // According to chromium source code:
  // src/third_party/WebKit/Source/modules/webdatabase/SQLTransactionClient.cpp line 67
  // Chromium does not allow users to manually change the quota for an origin (for now, at least).
  // This API is impossible to implement right now
  LOG_EWK_API_MOCKUP("Not Supported by chromium");
}

void ewk_view_exceeded_database_quota_reply(Evas_Object* ewkView, Eina_Bool allow)
{
  // According to chromium source code:
  // src/third_party/WebKit/Source/modules/webdatabase/SQLTransactionClient.cpp line 67
  // Chromium does not allow users to manually change the quota for an origin (for now, at least).
  // This API is impossible to implement right now
  LOG_EWK_API_MOCKUP("Not Supported by chromium");
}

void ewk_view_exceeded_local_file_system_quota_callback_set(Evas_Object* ewkView, Ewk_View_Exceeded_Indexed_Database_Quota_Callback callback, void* userData)
{
  LOG_EWK_API_MOCKUP();
}

void ewk_view_exceeded_local_file_system_quota_reply(Evas_Object* ewkView, Eina_Bool allow)
{
  LOG_EWK_API_MOCKUP();
}

void ewk_view_unfocus_allow_callback_set(Evas_Object* ewkView, Ewk_View_Unfocus_Allow_Callback callback, void* user_data)
{
  EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl);
  impl->SetViewUnfocusAllowCallback(callback, user_data);
}

void ewk_view_geolocation_permission_callback_set(Evas_Object* ewk_view, Ewk_View_Geolocation_Permission_Callback callback, void* user_data)
{
  EWK_VIEW_IMPL_GET_OR_RETURN(ewk_view, impl);
  impl->SetViewGeolocationPermissionCallback(callback, user_data);
}

static Eina_Bool _ewk_view_default_geolocation_permission(
    Evas_Object* ewk_view,
    Ewk_Geolocation_Permission_Request* request,
    void* user_data)
{
  EWK_VIEW_IMPL_GET_OR_RETURN(ewk_view, impl, EINA_FALSE);
  std::string application_name = std::string(impl->GetUserAgentAppName());
  std::string value = std::string(ewk_security_origin_host_get(
      ewk_geolocation_permission_request_origin_get(request)));
  std::string message = std::string(
      dgettext("WebKit", "IDS_WEBVIEW_POP_P1SS_HP2SS_IS_REQUESTING_PERMISSION_TO_ACCESS_YOUR_LOCATION"));
  std::string replace_str = std::string("%1$s");
  size_t pos = message.find(replace_str.c_str());
  if (pos != std::string::npos)
    message.replace(pos, replace_str.length(), application_name.c_str());

  replace_str = std::string("%2$s");
  pos = message.find(replace_str.c_str());
  if (pos != std::string::npos)
    message.replace(pos, replace_str.length(), value.c_str());

  // add for suspending
  ewk_geolocation_permission_request_suspend(request);

  GeolocationPermissionPopup* popup = new GeolocationPermissionPopup(request,
      ewk_geolocation_permission_request_origin_get(request), message);

  impl->GetPermissionPopupManager()->AddPermissionRequest(popup);

  return EINA_TRUE;
}
/* LCOV_EXCL_STOP */

void ewk_view_user_media_permission_callback_set(Evas_Object* ewk_view,
    Ewk_View_User_Media_Permission_Callback callback,
    void* user_data)
{
  EWK_VIEW_IMPL_GET_OR_RETURN(ewk_view, impl);
  impl->SetViewUserMediaPermissionCallback(callback, user_data);
}

static Eina_Bool _ewk_view_default_user_media_permission(  // LCOV_EXCL_LINE
    Evas_Object* ewk_view,
    Ewk_User_Media_Permission_Request* user_media_permission_request,
    void* user_data) {
  EWK_VIEW_IMPL_GET_OR_RETURN(ewk_view, impl, EINA_FALSE);  // LCOV_EXCL_LINE

  const char msgid_both[]
      = "IDS_WEBVIEW_POP_P1SS_HP2SS_IS_REQUESTING_PERMISSION_TO_USE_YOUR_CAMERA_AND_MICROPHONE";
  const char msgid_camera[]
      = "IDS_WEBVIEW_POP_P1SS_HP2SS_IS_REQUESTING_PERMISSION_TO_USE_YOUR_CAMERA";
  const char msgid_mic[]
      = "IDS_WEBVIEW_POP_P1SS_HP2SS_IS_REQUESTING_PERMISSION_TO_USE_YOUR_MICROPHONE";

  std::string value = std::string(ewk_security_origin_host_get(
      ewk_user_media_permission_request_origin_get(
          user_media_permission_request)));

  bool isAudioRequested = user_media_permission_request->IsAudioRequested();
  bool isVideoRequested = user_media_permission_request->IsVideoRequested();

  const char* msgid =
      (isAudioRequested && !isVideoRequested) ? msgid_mic :
      (!isAudioRequested && isVideoRequested) ? msgid_camera :
      msgid_both;

  std::string message = dgettext("WebKit", msgid);

  std::string replace_str = std::string("%1$s");
  size_t pos = message.find(replace_str.c_str());
  if (pos != std::string::npos)
    message.replace(pos, replace_str.length(), value.c_str());

  replace_str = std::string("(%2$s)");
  pos = message.find(replace_str.c_str());
  if (pos != std::string::npos)
    message.replace(pos, replace_str.length(), "");

  // add for suspending
  ewk_user_media_permission_request_suspend(user_media_permission_request);

  UserMediaPermissionPopup* popup = new UserMediaPermissionPopup(
      user_media_permission_request,
      ewk_user_media_permission_request_origin_get(
          user_media_permission_request), message);

  impl->GetPermissionPopupManager()->AddPermissionRequest(popup);

  return EINA_TRUE;
}

void ewk_view_use_settings_font(Evas_Object* ewkView)
{
  EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl);
  impl->UseSettingsFont();
}

char* ewk_view_get_cookies_for_url(Evas_Object* view, const char* url)
{
  EWK_VIEW_IMPL_GET_OR_RETURN(view, impl, 0);
  EINA_SAFETY_ON_NULL_RETURN_VAL(url, 0);
  std::string cookiesForURL;
  if (impl->context())
    cookiesForURL = impl->context()->cookieManager()->GetCookiesForURL(std::string(url));
  if (cookiesForURL.empty())
    return NULL;
  return strndup(cookiesForURL.c_str(), cookiesForURL.length());
}

Eina_Bool ewk_view_fullscreen_exit(Evas_Object* view)
{
  EWK_VIEW_IMPL_GET_OR_RETURN(view, impl, EINA_FALSE);
  impl->ExitFullscreen();
  return EINA_TRUE;
}

Eina_Bool ewk_view_draws_transparent_background_set(Evas_Object *view, Eina_Bool enabled)
{
  EWK_VIEW_IMPL_GET_OR_RETURN(view, impl, EINA_FALSE);
  if (enabled)
    return impl->SetBackgroundColor(0, 0, 0, 0);
  return impl->SetBackgroundColor(255, 255, 255, 255);
}

void ewk_view_browser_font_set(Evas_Object* ewkView)
{
  EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl);
  impl->SetBrowserFont();
}

void ewk_view_session_data_get(Evas_Object* ewkView, const char** data, unsigned* length)
{
  EINA_SAFETY_ON_NULL_RETURN(data);
  EINA_SAFETY_ON_NULL_RETURN(length);

  EWebView* impl = GetWebViewFromEvasObject(ewkView);
  if (!impl) {
    *data = NULL;
    *length = 0;
    return;
  }

  impl->GetSessionData(data, length);
}

Eina_Bool ewk_view_mode_set(Evas_Object* ewkView, Ewk_View_Mode view_mode)
{
  EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, EINA_FALSE);

  if (view_mode == EWK_VIEW_MODE_WINDOWED) {
    impl->SetViewMode(blink::WebViewModeWindowed);
    return EINA_TRUE;
  } else if (view_mode == EWK_VIEW_MODE_FULLSCREEN) {
    impl->SetViewMode(blink::WebViewModeFullscreen);
    return EINA_TRUE;
  } else {
    return EINA_FALSE;
  }
}

Eina_Bool ewk_view_split_scroll_overflow_enabled_set(Evas_Object* ewkView, const Eina_Bool enabled)
{
  LOG_EWK_API_MOCKUP("for browser");
  return false;
}
/* LCOV_EXCL_STOP */

Ewk_Back_Forward_List* ewk_view_back_forward_list_get(const Evas_Object* ewkView)
{
  EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, NULL);
  return impl->GetBackForwardList();
}

void ewk_view_notification_permission_callback_set(Evas_Object *o, Ewk_View_Notification_Permission_Callback callback, void *user_data)
{
  EWK_VIEW_IMPL_GET_OR_RETURN(o, impl);
  impl->SetNotificationPermissionCallback(callback, user_data);
}

/* LCOV_EXCL_START */
static Eina_Bool _ewk_view_default_notification_permission(
    Evas_Object* ewk_view,
    Ewk_Notification_Permission_Request* request,
    void* user_data)
{
  EWK_VIEW_IMPL_GET_OR_RETURN(ewk_view, impl, EINA_FALSE);
  std::string application_name = std::string(impl->GetUserAgentAppName());

  std::string value = std::string(ewk_security_origin_host_get(
      ewk_notification_permission_request_origin_get(request)));
  std::string message =
  std::string( dgettext("WebKit",
      "IDS_WEBVIEW_POP_P1SS_HP2SS_IS_REQUESTING_PERMISSION_TO_SHOW_NOTIFICATIONS"));
  std::string replace_str = std::string("%1$s");

  size_t pos = message.find(replace_str.c_str());
  if (pos != std::string::npos)
    message.replace(pos, replace_str.length(), application_name.c_str());
  replace_str = std::string("%2$s");
  pos = message.find(replace_str.c_str());
  if (pos != std::string::npos)
    message.replace(pos, replace_str.length(), value.c_str());

  // add for suspending
  ewk_notification_permission_request_suspend(request);

  NotificationPermissionPopup* popup = new NotificationPermissionPopup(request,
      ewk_notification_permission_request_origin_get(request), message);

  impl->GetPermissionPopupManager()->AddPermissionRequest(popup);

  return EINA_TRUE;
}

void ewk_view_draw_focus_ring_enable_set(Evas_Object* ewkView, Eina_Bool enable)
{
  LOG_EWK_API_MOCKUP("This API is deprecated");
}

double ewk_view_page_zoom_get(const Evas_Object* ewkView)
{
  EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, -1.0);
  return impl->GetPageZoomFactor();
}

Eina_Bool ewk_view_page_zoom_set(Evas_Object* ewkView, double zoomFactor)
{
  EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, EINA_FALSE);
  LOG(INFO) << "ewkView: " << ewkView << "; zoomFactor: " << zoomFactor;
  return impl->SetPageZoomFactor(zoomFactor);
}

Evas_Object* ewk_view_smart_add(Evas* canvas, Evas_Smart* smart, Ewk_Context* context, Ewk_Page_Group* pageGroup)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(context, 0);
  return CreateWebViewAsEvasObject(context, canvas, smart);
}

void ewk_view_quota_permission_request_callback_set(Evas_Object* ewkView, Ewk_Quota_Permission_Request_Callback callback, void* user_data)
{
  EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl);
  impl->SetQuotaPermissionRequestCallback(callback, user_data);
}

void ewk_view_quota_permission_request_reply(const Ewk_Quota_Permission_Request* request, const Eina_Bool allow)
{
  EINA_SAFETY_ON_NULL_RETURN(request);
  Evas_Object* ewkView = request->getView();
  EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl);
  impl->QuotaRequestReply(request, allow == EINA_TRUE);
}

void ewk_view_quota_permission_request_cancel(const Ewk_Quota_Permission_Request* request)
{
  EINA_SAFETY_ON_NULL_RETURN(request);
  Evas_Object* ewkView = request->getView();
  EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl);
  impl->QuotaRequestCancel(request);
}
/* LCOV_EXCL_STOP */

Eina_Bool ewk_view_focus_set(const Evas_Object* view, Eina_Bool focused)
{
  EWK_VIEW_IMPL_GET_OR_RETURN(view, impl, EINA_FALSE);
  /* LCOV_EXCL_START */
  impl->SetFocus(focused);
  return EINA_TRUE;
  /* LCOV_EXCL_STOP */
}

Eina_Bool ewk_view_focus_get(const Evas_Object* view)
{
  EWK_VIEW_IMPL_GET_OR_RETURN(view, impl, EINA_FALSE);
  return impl->HasFocus();  // LCOV_EXCL_LINE
}

/* LCOV_EXCL_START */
Eina_Bool ewk_view_page_close(Evas_Object* o)
{
  EWK_VIEW_IMPL_GET_OR_RETURN(o, impl, EINA_FALSE);

  impl->ClosePage();
  return EINA_TRUE;
}

void ewk_view_session_timeout_set(Evas_Object* o, unsigned long timeout)
{
  EWK_VIEW_IMPL_GET_OR_RETURN(o, impl);
  impl->SetSessionTimeout(timeout);
}

void ewk_view_request_manifest(Evas_Object* o, Ewk_View_Request_Manifest_Callback callback, void* user_data)
{
  EWK_VIEW_IMPL_GET_OR_RETURN(o, impl);
  impl->RequestManifest(callback, user_data);
}

Evas_Object* ewk_view_widget_get(Evas_Object* view)
{
  EWK_VIEW_IMPL_GET_OR_RETURN(view, impl, nullptr);
  return impl->native_view();
}

Eina_Bool ewk_view_draws_transparent_background_get(Evas_Object* o)
{
  EWK_VIEW_IMPL_GET_OR_RETURN(o, impl, EINA_FALSE);
  return impl->GetDrawsTransparentBackground();
}

Eina_Bool ewk_view_split_scroll_overflow_enabled_get(const Evas_Object* o)
{
  LOG_EWK_API_MOCKUP();
  return false;
}

Eina_Bool ewk_view_did_change_theme_color_callback_set(
    Evas_Object* o, Ewk_View_Did_Change_Theme_Color_Callback callback,
    void* user_data) {
  EWK_VIEW_IMPL_GET_OR_RETURN(o, impl, EINA_FALSE);
  impl->SetDidChangeThemeColorCallback(callback, user_data);
  return EINA_TRUE;
}

Eina_Bool ewk_view_save_page_as_mhtml(Evas_Object* o, const char* path, Ewk_View_Save_Page_Callback callback, void* user_data)
{
  EWK_VIEW_IMPL_GET_OR_RETURN(o, impl, EINA_FALSE);
  EINA_SAFETY_ON_NULL_RETURN_VAL(path, EINA_FALSE);
  EINA_SAFETY_ON_NULL_RETURN_VAL(callback, EINA_FALSE);
  return impl->SavePageAsMHTML(path, callback, user_data);
}

void ewk_view_reader_mode_set(Evas_Object* ewk_view, Eina_Bool enable)
{
  EWK_VIEW_IMPL_GET_OR_RETURN(ewk_view, impl);
  impl->SetReaderMode(enable);
}

Eina_Bool ewk_view_top_controls_height_set(Evas_Object* ewk_view,
                                           size_t top_height,
                                           size_t bottom_height) {
  EWK_VIEW_IMPL_GET_OR_RETURN(ewk_view, impl, EINA_FALSE);
  return impl->SetTopControlsHeight(top_height, bottom_height);
}

Eina_Bool ewk_view_top_controls_state_set(Evas_Object* ewk_view,
                                          Ewk_Top_Control_State constraint,
                                          Ewk_Top_Control_State current,
                                          Eina_Bool animation) {
  EWK_VIEW_IMPL_GET_OR_RETURN(ewk_view, impl, EINA_FALSE);
  return impl->SetTopControlsState(constraint, current, animation);
}

Eina_Bool ewk_view_main_frame_scrollbar_visible_set(Evas_Object* ewk_view, Eina_Bool visible)
{
  EWK_VIEW_IMPL_GET_OR_RETURN(ewk_view, impl, EINA_FALSE);
  return impl->SetMainFrameScrollbarVisible(visible);
}

Eina_Bool ewk_view_main_frame_scrollbar_visible_get(
    Evas_Object* ewk_view,
    Ewk_View_Main_Frame_Scrollbar_Visible_Get_Callback callback,
    void* user_data) {
  EINA_SAFETY_ON_NULL_RETURN_VAL(callback, EINA_FALSE);
  EWK_VIEW_IMPL_GET_OR_RETURN(ewk_view, impl, EINA_FALSE);

  return impl->GetMainFrameScrollbarVisible(callback, user_data);
}

Eina_Bool ewk_view_set_custom_device_pixel_ratio(Evas_Object* ewkView, Eina_Bool enabled)
{
  LOG_EWK_API_MOCKUP();
  return false;
}

Eina_Bool ewk_media_translated_url_set(Evas_Object* ewkView, const char* url)
{
#if defined(OS_TIZEN_TV_PRODUCT)
  EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, EINA_FALSE);
  EINA_SAFETY_ON_NULL_RETURN_VAL(url, EINA_FALSE);
  impl->SetTranslatedURL(url);
  return EINA_TRUE;
#else
  LOG_EWK_API_MOCKUP("Only for Tizen TV.");
  return false;
#endif
}

Eina_Bool ewk_view_app_preload_set(Evas_Object* ewkView, Eina_Bool is_preload)
{
  LOG_EWK_API_MOCKUP();
  return false;
}

Eina_Bool ewk_view_marlin_enable_set(Evas_Object* ewkView, Eina_Bool is_enable)
{
#if defined(OS_TIZEN_TV_PRODUCT)
  EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, EINA_FALSE);
  impl->SetMarlinEnable(is_enable);
  return EINA_TRUE;
#else
  LOG_EWK_API_MOCKUP("Only for Tizen TV.");
  return EINA_FALSE;
#endif

}

Eina_Bool ewk_view_active_drm_set(Evas_Object* view, const char* drm_system_id)
{
#if defined(OS_TIZEN_TV_PRODUCT)
    EWK_VIEW_IMPL_GET_OR_RETURN(view, impl, EINA_FALSE);
    impl->SetActiveDRM(drm_system_id);
    return EINA_TRUE;
#else
    LOG_EWK_API_MOCKUP("Only for Tizen TV.");
    return EINA_FALSE;
#endif
}

void ewk_media_set_subtitle_lang(Evas_Object* ewkView, const char* lang_list)
{
#if defined(OS_TIZEN_TV_PRODUCT)
  EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl);
  impl->SetSubtitleLang(lang_list);
#else
  LOG_EWK_API_MOCKUP("Only for Tizen TV.");
#endif
}

void ewk_media_set_parental_rating_result(Evas_Object* ewkView, const char* url, Eina_Bool is_pass){
#if defined(OS_TIZEN_TV_PRODUCT)
  EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl);
  impl->SetParentalRatingResult(url, is_pass);
#else
  LOG_EWK_API_MOCKUP("Only for Tizen TV.");
#endif
}

void ewk_media_start_with_high_bit_rate(Evas_Object* ewkView, Eina_Bool is_high_bitrate)
{
#if defined(OS_TIZEN_TV_PRODUCT)
  EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl);
  impl->SetHighBitRate(is_high_bitrate);
#else
  LOG_EWK_API_MOCKUP("Only for Tizen TV.");
#endif
}

Eina_Bool ewk_view_html_string_override_current_entry_load(Evas_Object* view, const char* html, const char* base_uri, const char* unreachable_url)
{
  EWK_VIEW_IMPL_GET_OR_RETURN(view, impl, EINA_FALSE);
  EINA_SAFETY_ON_NULL_RETURN_VAL(html, EINA_FALSE);
  impl->LoadHTMLStringOverridingCurrentEntry(html, base_uri, unreachable_url);
  return EINA_TRUE;
}

Eina_Bool ewk_view_text_matches_count(Evas_Object* o, const char* text, Ewk_Find_Options options, unsigned max_match_count)
{
  LOG_EWK_API_MOCKUP();
  return false;
}

void ewk_view_add_dynamic_certificate_path(const Evas_Object *ewkView, const char* host, const char* cert_path)
{
#if defined(OS_TIZEN_TV_PRODUCT)
  EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl);
  impl->AddDynamicCertificatePath(std::string(host), std::string(cert_path));
#else
  LOG_EWK_API_MOCKUP();
#endif
}

void ewk_view_atk_deactivation_by_app(Evas_Object* view, Eina_Bool enable)
{
#if defined(TIZEN_ATK_FEATURE_VD)
  EWK_VIEW_IMPL_GET_OR_RETURN(view, impl);
  impl->DeactivateAtk(enable == EINA_FALSE);
#else
  LOG_EWK_API_MOCKUP();
#endif
}

Eina_Bool ewk_view_tts_mode_set(Evas_Object* view, ewk_tts_mode tts_mode)
{
#if defined(OS_TIZEN_TV_PRODUCT)
  EWK_VIEW_IMPL_GET_OR_RETURN(view, impl, EINA_FALSE);
  switch (tts_mode) {
    case EWK_TTS_MODE_DEFAULT:
      return impl->SetTTSMode(TTS_MODE_DEFAULT);
    case EWK_TTS_MODE_NOTIFICATION:
      return impl->SetTTSMode(TTS_MODE_NOTIFICATION);
    case EWK_TTS_MODE_SCREEN_READER:
      return impl->SetTTSMode(TTS_MODE_SCREEN_READER);
    default:
      LOG(ERROR) << "Not supported TTS mode: " << (int)tts_mode;
      return EINA_FALSE;
  }
#else
  LOG_EWK_API_MOCKUP();
  return EINA_FALSE;
#endif
}

char* ewk_view_cookies_get(Evas_Object* o, const char* url)
{
  LOG_EWK_API_MOCKUP();
  return NULL;
}

void ewk_view_notification_show_callback_set(Evas_Object *o, Ewk_View_Notification_Show_Callback show_callback, void *user_data)
{
  LOG_EWK_API_MOCKUP();
}

void ewk_view_notification_cancel_callback_set(Evas_Object *o, Ewk_View_Notification_Cancel_Callback cancel_callback, void *user_data)
{
  LOG_EWK_API_MOCKUP();
}

const char* ewk_view_custom_encoding_get(const Evas_Object* o)
{
  LOG_EWK_API_MOCKUP();
  return NULL;
}

Eina_Bool ewk_view_custom_encoding_set(Evas_Object* o, const char* encoding)
{
  LOG_EWK_API_MOCKUP();
  return false;
}

void ewk_view_force_layout(const Evas_Object* o)
{
  LOG_EWK_API_MOCKUP();
}

double ewk_view_media_current_time_get(const Evas_Object *o)
{
#if defined(OS_TIZEN_TV_PRODUCT)
  EWK_VIEW_IMPL_GET_OR_RETURN(o, impl, 0);
  return impl->GetCurrentTime();
#else
  LOG_EWK_API_MOCKUP("Only for Tizen TV.");
  return 0;
#endif

}
/* LCOV_EXCL_STOP */

Eina_Bool ewk_view_javascript_message_handler_add(
    Evas_Object* view,
    Ewk_View_Script_Message_Cb callback,
    const char* name) {
  EWK_VIEW_IMPL_GET_OR_RETURN(view, impl, EINA_FALSE);
  EINA_SAFETY_ON_NULL_RETURN_VAL(name, EINA_FALSE);
  return impl->AddJavaScriptMessageHandler(view, callback, std::string(name));
}

/* LCOV_EXCL_START */
Eina_Bool ewk_view_evaluate_javascript(Evas_Object* view,
                                       const char* name,
                                       const char* result) {
  EWK_VIEW_IMPL_GET_OR_RETURN(view, impl, EINA_FALSE);
  EINA_SAFETY_ON_NULL_RETURN_VAL(result, EINA_FALSE);
  EINA_SAFETY_ON_NULL_RETURN_VAL(name, EINA_FALSE);

  std::string function(name);
  std::string data(result);
  std::string pattern("\n");
  std::string replace("</br>");
  std::string::size_type pos = 0;
  std::string::size_type offset = 0;
  while ((pos = data.find(pattern, offset)) != std::string::npos) {
    data.replace(data.begin() + pos, data.begin() + pos + pattern.size(),
                 replace);
    offset = pos + replace.size();
  }

  // For JSON Object or Array.
  if (result[0] == '{' || result[0] == '[') {
    data = "javascript:" + function + "(JSON.parse('" + data + "'))";
  } else {
    data = "javascript:" + function + "('" + data + "')";
  }

  return impl->ExecuteJavaScript(data.c_str(), NULL, 0);
}

void ewk_view_request_manifest_from_url(Evas_Object* o, Ewk_View_Request_Manifest_Callback callback, void* user_data, const char* host_url, const char* manifest_url)
{
  LOG_EWK_API_MOCKUP();
}

void ewk_view_app_installation_request_callback_set(Evas_Object* o, Ewk_App_Installation_Request_Callback callback, void* user_data)
{
  LOG_EWK_API_MOCKUP();
}

Eina_Bool ewk_view_bg_color_set(Evas_Object* o, int r, int g, int b, int a)
{
  EWK_VIEW_IMPL_GET_OR_RETURN(o, impl, EINA_FALSE);
  if (r < 0 || r > 255 || g < 0 || g > 255 || b < 0 || b > 255 || a < 0 ||
      a > 255) {
    LOG(ERROR) << "Invalid background color";
    return EINA_FALSE;
  }
  return impl->SetBackgroundColor(r, g, b, a);
}

Eina_Bool ewk_view_bg_color_get(Evas_Object *view, Ewk_View_Background_Color_Get_Callback callback, void *user_data)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(callback, EINA_FALSE);
  EWK_VIEW_IMPL_GET_OR_RETURN(view, impl, EINA_FALSE);

  return impl->GetBackgroundColor(callback, user_data);
}

Eina_Bool ewk_view_send_key_event(Evas_Object* ewk_view,
    void* key_event, Eina_Bool is_press)
{
#if defined(OS_TIZEN_TV_PRODUCT)
  EWK_VIEW_IMPL_GET_OR_RETURN(ewk_view, impl, EINA_FALSE);
  EINA_SAFETY_ON_NULL_RETURN_VAL(key_event, EINA_FALSE);
  impl->SendKeyEvent(ewk_view, key_event, is_press);
  return EINA_TRUE;
#else
  LOG_EWK_API_MOCKUP("This API is only available in Tizen TV product.");
  return EINA_FALSE;
#endif

}

Eina_Bool ewk_view_key_events_enabled_set(Evas_Object *o, Eina_Bool enabled)
{
#if defined(OS_TIZEN_TV_PRODUCT)
  EWK_VIEW_IMPL_GET_OR_RETURN(o, impl, EINA_FALSE);
  impl->SetKeyEventsEnabled(!!enabled);
  return EINA_TRUE;
#else
  LOG_EWK_API_MOCKUP("This API is only available in Tizen TV product.");
  return EINA_FALSE;
#endif

}

Eina_Bool ewk_view_set_support_video_hole(Evas_Object* ewkView, Evas_Object* window, Eina_Bool enable, Eina_Bool isVideoWindow)
{
#if defined(TIZEN_VIDEO_HOLE)
  EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, EINA_FALSE);
  impl->SetVideoHoleSupport(enable);

  media::MediaPlayerEfl::SetSharedVideoWindowHandle(window, isVideoWindow);
  return EINA_TRUE;
#else
  LOG_EWK_API_MOCKUP("Video Hole feature was not enabled");
  return EINA_FALSE;
#endif
}

Eina_Bool ewk_view_set_support_canvas_hole(Evas_Object* ewkView, const char* url)
{
  LOG_EWK_API_MOCKUP();
  return false;
}

void ewk_view_authentication_callback_set(
    Evas_Object* ewk_view,
    Ewk_View_Authentication_Callback callback,
    void* user_data)
{
  EWK_VIEW_IMPL_GET_OR_RETURN(ewk_view, impl);
  impl->SetViewAuthCallback(callback, user_data);

}

static void _ewk_view_default_authentication(Evas_Object* ewk_view,
                                             Ewk_Auth_Challenge* auth_challenge,
                                             void* user_data)
{
    AuthenticationChallengePopup::CreateAndShow(auth_challenge, ewk_view);
}

void ewk_view_poweroff_suspend(Evas_Object *item)
{
  LOG_EWK_API_MOCKUP();
}

void ewk_view_resume_network_loading(Evas_Object* ewkView)
{
#if defined(OS_TIZEN_TV_PRODUCT)
  EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl);
  impl->ResumeNetworkLoading();
#else
  LOG_EWK_API_MOCKUP("Only for Tizen TV Browser");
#endif
}

void ewk_view_suspend_network_loading(Evas_Object* ewkView)
{
#if defined(OS_TIZEN_TV_PRODUCT)
  EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl);
  impl->SuspendNetworkLoading();
#else
  LOG_EWK_API_MOCKUP("Only for Tizen TV Browser");
#endif
}

Eina_Bool ewk_view_edge_scroll_by(Evas_Object *ewkView, int dx, int dy)
{
#if defined(OS_TIZEN_TV_PRODUCT)
  EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, EINA_FALSE);
  return impl->EdgeScrollBy(dx, dy);
#else
  LOG_EWK_API_MOCKUP("Only for Tizen TV");
  return EINA_FALSE;
#endif
}

void ewk_view_set_cursor_by_client(Evas_Object* ewkView, Eina_Bool enable)
{
#if defined(OS_TIZEN_TV_PRODUCT)
  EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl);
  impl->SetCursorByClient(enable);
#else
  LOG_EWK_API_MOCKUP("Only for Tizen TV Browser");
#endif

}

void ewk_view_tile_cover_area_multiplier_set(Evas_Object* ewkView, float cover_area_multiplier)
{
  EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl);
  impl->SetTileCoverAreaMultiplier(cover_area_multiplier);
}

Eina_Bool ewk_view_is_video_playing(Evas_Object* o, Ewk_Is_Video_Playing_Callback callback, void* user_data)
{
  EWK_VIEW_IMPL_GET_OR_RETURN(o, impl, EINA_FALSE);
  EINA_SAFETY_ON_NULL_RETURN_VAL(callback, EINA_FALSE);
  return impl->IsVideoPlaying(callback, user_data) ? EINA_TRUE : EINA_FALSE;
}

Eina_Bool ewk_view_stop_video(Evas_Object* o, Ewk_Stop_Video_Callback callback, void* user_data)
{
  LOG_EWK_API_MOCKUP();
  return false;
}

Eina_Bool ewk_view_add_item_to_back_forward_list(Evas_Object* o, const Ewk_Back_Forward_List_Item* item)
{
#if defined(OS_TIZEN_TV_PRODUCT)
  EWK_VIEW_IMPL_GET_OR_RETURN(o, impl, EINA_FALSE);
  EINA_SAFETY_ON_NULL_RETURN_VAL(item, EINA_FALSE);
  impl->AddItemToBackForwardList(item);
  return EINA_TRUE;
#else
  LOG_EWK_API_MOCKUP("Only for Tizen TV");
  return EINA_FALSE;
#endif
}

void ewk_view_clear_tiles_on_hide_enabled_set(Evas_Object* ewkView, Eina_Bool enable)
{
  EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl);
  impl->SetClearTilesOnHide(enable);
}

void ewk_view_run_mixed_content_confirm_callback_set(Evas_Object* ewkView, Ewk_View_Run_Mixed_Content_Confirm_Callback callback, void* user_data)
{
  LOG_EWK_API_MOCKUP();
}

Evas_Object* ewk_view_favicon_get(const Evas_Object* ewkView)
{
  EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, NULL);
  const char* url = ewk_view_url_get(ewkView);
  if(impl->context())
    return impl->context()->AddFaviconObject(url, impl->GetEvas());

  return NULL;
}

void ewk_view_clear_all_tiles_resources(Evas_Object* ewkView)
{
#if defined(OS_TIZEN_TV_PRODUCT)
  EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl);
  impl->ClearAllTilesResources();
#else
  LOG_EWK_API_MOCKUP("Only for Tizen TV");
#endif
}

void ewk_view_request_canvas_fullscreen(Evas_Object* ewkView)
{
  LOG_EWK_API_MOCKUP();
}

void ewk_view_360video_play(Evas_Object* ewkView)
{
  LOG_EWK_API_MOCKUP();
}

void ewk_view_360video_pause(Evas_Object* ewkView)
{
  LOG_EWK_API_MOCKUP();
}

void ewk_view_360video_duration(Evas_Object* ewkView, Ewk_360_Video_Duration_Callback callback, void* user_data)
{
  LOG_EWK_API_MOCKUP();
}

void ewk_view_360video_current_time(Evas_Object* ewkView, Ewk_360_Video_CurrentTime_Callback callback, void* user_data)
{
  LOG_EWK_API_MOCKUP();
}

void ewk_view_360video_set_current_time(Evas_Object* ewkView, double current_time)
{
  LOG_EWK_API_MOCKUP();
}

void ewk_view_widget_pepper_extension_callback_set(Evas_Object* ewk_view, Generic_Sync_Call_Callback cb, void* user_data)
{
#if defined(TIZEN_PEPPER_EXTENSIONS) && defined(OS_TIZEN_TV_PRODUCT)
  EWK_VIEW_IMPL_GET_OR_RETURN(ewk_view, impl);
  impl->SetPepperExtensionCallback(cb, user_data);
#else
  NOTIMPLEMENTED() << "This API is only available with TIZEN_PEPPER_EXTENSIONS"
                   << " macro on TIZEN_TV products.";
#endif
}

void ewk_view_widget_pepper_extension_info_set(Evas_Object* ewk_view, Ewk_Value widget_pepper_ext_info)
{
#if defined(TIZEN_PEPPER_EXTENSIONS) && defined(OS_TIZEN_TV_PRODUCT)
  EWK_VIEW_IMPL_GET_OR_RETURN(ewk_view, impl);
  impl->SetPepperExtensionWidgetInfo(widget_pepper_ext_info);
#else
  NOTIMPLEMENTED() << "This API is only available with TIZEN_PEPPER_EXTENSIONS"
                   << " macro on TIZEN_TV products.";
#endif
}

void ewk_view_voicemanager_label_draw(Evas_Object* view, Evas_Object* image, Eina_Rectangle rect)
{
#if defined(OS_TIZEN_TV_PRODUCT)
  EINA_SAFETY_ON_NULL_RETURN(image);
  EWK_VIEW_IMPL_GET_OR_RETURN(view, impl);
  impl->DrawLabel(image, rect);
#else
  LOG_EWK_API_MOCKUP("Only for Tizen TV.");
#endif
}

void ewk_view_voicemanager_labels_clear(Evas_Object* view)
{
#if defined(OS_TIZEN_TV_PRODUCT)
  EWK_VIEW_IMPL_GET_OR_RETURN(view, impl);
  impl->ClearLabels();
#else
  LOG_EWK_API_MOCKUP("Only for Tizen TV.");
#endif
}

void ewk_view_mirrored_blur_set(Evas_Object* o, Eina_Bool state)
{
#if defined(OS_TIZEN)
  if (IsWearableProfile()) {
    EWK_VIEW_IMPL_GET_OR_RETURN(o, impl);
    impl->SetAppNeedEffect(state);
  } else {
    LOG_EWK_API_MOCKUP();
  }
#else
  LOG_EWK_API_MOCKUP();
#endif
}

void ewk_view_offscreen_rendering_enabled_set(Evas_Object* view, Eina_Bool enabled)
{
#if defined(TIZEN_TBM_SUPPORT)
  EWK_VIEW_IMPL_GET_OR_RETURN(view, impl);
  impl->SetOffscreenRendering(enabled);
#else
  LOG_EWK_API_MOCKUP();
#endif
}

/* LCOV_EXCL_STOP */
