/*
 * Copyright (C) 2013-2016 Samsung Electronics. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY SAMSUNG ELECTRONICS. AND ITS CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SAMSUNG ELECTRONICS. OR ITS
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include "ewk_settings_product.h"

#include "base/strings/string_util.h"
#include "eweb_view.h"
#include "private/ewk_private.h"
#include "private/ewk_settings_private.h"
#include "private/ewk_view_private.h"
#include "text_encoding_map_efl.h"
#include "tizen/system_info.h"
#include "ui/events/gesture_detection/gesture_configuration.h"

namespace {

void ewkUpdateWebkitPreferences(Evas_Object *ewkView)
{
  EWebView* impl = EWebView::FromEvasObject(ewkView);
  assert(impl);
  impl->UpdateWebKitPreferences();
}

}

/* LCOV_EXCL_START */
Eina_Bool ewk_settings_fullscreen_enabled_set(Ewk_Settings* settings, Eina_Bool enable)
{
  LOG_EWK_API_MOCKUP("Not Supported by chromium");
  return false;
}

Eina_Bool ewk_settings_fullscreen_enabled_get(const Ewk_Settings* settings)
{
  LOG_EWK_API_MOCKUP("Not Supported by chromium");
  return false;
}
/* LCOV_EXCL_STOP */

Eina_Bool ewk_settings_javascript_enabled_set(Ewk_Settings* settings, Eina_Bool enable)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);
  settings->getPreferences().javascript_enabled = enable;
  ewkUpdateWebkitPreferences(settings->getEvasObject());
  return true;
}

Eina_Bool ewk_settings_javascript_enabled_get(const Ewk_Settings* settings)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);
  return settings->getPreferences().javascript_enabled;
}

/* LCOV_EXCL_START */
Eina_Bool ewk_settings_swipe_to_refresh_enabled_set(Ewk_Settings *settings, Eina_Bool enable)
{
  LOG_EWK_API_MOCKUP();
  return false;
}

Eina_Bool ewk_settings_swipe_to_refresh_enabled_get(const Ewk_Settings *settings)
{
  LOG_EWK_API_MOCKUP();
  return false;
}
/* LCOV_EXCL_STOP */

Eina_Bool ewk_settings_loads_images_automatically_set(Ewk_Settings* settings, Eina_Bool automatic)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);
  settings->getPreferences().loads_images_automatically = automatic;
  settings->getPreferences().images_enabled = automatic;
  ewkUpdateWebkitPreferences(settings->getEvasObject());
  return true;
}

Eina_Bool ewk_settings_loads_images_automatically_get(const Ewk_Settings* settings)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);
  return settings->getPreferences().loads_images_automatically;
}

/* LCOV_EXCL_START */
Eina_Bool ewk_settings_plugins_enabled_set(Ewk_Settings* settings, Eina_Bool enable)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);
  settings->getPreferences().plugins_enabled = enable;
  ewkUpdateWebkitPreferences(settings->getEvasObject());
  return true;
}

Eina_Bool ewk_settings_plugins_enabled_get(const Ewk_Settings* settings)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);
  return settings->getPreferences().plugins_enabled;
}
/* LCOV_EXCL_STOP */

Eina_Bool ewk_settings_auto_fitting_set(Ewk_Settings* settings, Eina_Bool enable)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);
  settings->getPreferencesEfl().shrinks_viewport_content_to_fit = enable;
  settings->getPreferences().shrinks_viewport_contents_to_fit = enable;
  ewkUpdateWebkitPreferences(settings->getEvasObject());
  return true;
}

Eina_Bool ewk_settings_auto_fitting_get(const Ewk_Settings* settings)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);
  return settings->getPreferencesEfl().shrinks_viewport_content_to_fit;
}

/* LCOV_EXCL_START */
Eina_Bool ewk_settings_force_zoom_set(Ewk_Settings* settings, Eina_Bool enable)
{
#if defined(USE_EFL)
  EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);
  settings->getPreferences().force_enable_zoom = enable;
  ewkUpdateWebkitPreferences(settings->getEvasObject());
  return true;
#else
  return false;
#endif
}

Eina_Bool ewk_settings_force_zoom_get(const Ewk_Settings* settings)
{
#if defined(USE_EFL)
  EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);
  return settings->getPreferences().force_enable_zoom;
#else
  return false;
#endif
}

Eina_Bool ewk_settings_font_default_size_set(Ewk_Settings* settings, int size)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);
  settings->getPreferences().default_font_size = size;
  ewkUpdateWebkitPreferences(settings->getEvasObject());
  return true;
}

int ewk_settings_font_default_size_get(const Ewk_Settings* settings)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);
  return settings->getPreferences().default_font_size;
}

Eina_Bool ewk_settings_scripts_window_open_set(Ewk_Settings* settings, Eina_Bool allow)
{
  return ewk_settings_scripts_can_open_windows_set(settings, allow);
}

Eina_Bool ewk_settings_scripts_window_open_get(const Ewk_Settings* settings)
{
  return ewk_settings_scripts_can_open_windows_get(settings);
}

Eina_Bool ewk_settings_compositing_borders_visible_set(Ewk_Settings* settings, Eina_Bool enable)
{
  /*
  This API is impossible to implement in same manner as it was in webkit-efl
  for few reasons: chromium made compositing-borders-visible setting global - it
  is either enabled or disabled for whole process, it requires application restart
  because it is controled by command line switch. It must be provided before
  spawning any renderer/zygote processes
  */

  LOG_EWK_API_MOCKUP("Not implement by difference of webkit2 and chromium's behavior");
  return false;
}

Eina_Bool ewk_settings_default_encoding_set(Ewk_Settings* settings, const char* encoding)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);
  EINA_SAFETY_ON_NULL_RETURN_VAL(encoding, false);
  settings->getPreferences().default_encoding = std::string(encoding);
  ewkUpdateWebkitPreferences(settings->getEvasObject());
  return true;
}

Eina_Bool ewk_settings_is_encoding_valid(const char* encoding)
{
  return TextEncodingMapEfl::GetInstance()->isTextEncodingValid(encoding);
}

const char* ewk_settings_default_encoding_get(const Ewk_Settings* settings)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(settings, 0);
  return settings->getPreferences().default_encoding.c_str();
}

Eina_Bool ewk_settings_private_browsing_enabled_set(Ewk_Settings* settings, Eina_Bool enable)
{
#if defined(OS_TIZEN_TV_PRODUCT)
  LOG_EWK_API_DEPRECATED("Use ewk_view_add_in_incognito_mode() instead.");
#elif defined(OS_TIZEN)
  if (IsMobileProfile()) {
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, EINA_FALSE);
    EWebView* impl = EWebView::FromEvasObject(settings->getEvasObject());
    return impl->SetPrivateBrowsing(enable);
  }
#endif
  return EINA_FALSE;
}

Eina_Bool ewk_settings_private_browsing_enabled_get(const Ewk_Settings* settings)
{
#if defined(OS_TIZEN_TV_PRODUCT)
  LOG_EWK_API_DEPRECATED("Use ewk_view_add_in_incognito_mode() instead.");
#elif defined(OS_TIZEN)
  if (IsMobileProfile()) {
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, EINA_FALSE);
    EWK_VIEW_IMPL_GET_OR_RETURN(const_cast<Ewk_Settings *>(settings)->getEvasObject(), webview, EINA_FALSE);
    return webview->GetPrivateBrowsing();
  }
#endif
  return EINA_FALSE;
}

Eina_Bool ewk_settings_editable_link_behavior_set(Ewk_Settings* settings, Ewk_Editable_Link_Behavior behavior)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);
  ewkUpdateWebkitPreferences(settings->getEvasObject());
  return true;
}

Eina_Bool ewk_settings_load_remote_images_set(Ewk_Settings* settings, Eina_Bool loadRemoteImages)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);
  // FIXME: To be implemented when the functionality is required.
  // To be added in WebkitPreferences -> WebSettings -> Settings
  settings->setLoadRemoteImages(loadRemoteImages);
  return true;
}

Eina_Bool ewk_settings_load_remote_images_get(const Ewk_Settings* settings)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);
  // FIXME: To be implemented when the functionality is required.
  // To be added in WebkitPreferences -> WebSettings -> Settings
  return settings->loadRemoteImages();
}

Eina_Bool ewk_settings_scan_malware_enabled_set(Ewk_Settings* settings, Eina_Bool scanMalwareEnabled)
{
  // API is postponed. Possibly chromium's mechanism may be reused.
  LOG_EWK_API_MOCKUP("Not Supported yet");
  return false;
}

Eina_Bool ewk_settings_spdy_enabled_set(Ewk_Settings* settings, Eina_Bool spdyEnabled)
{
  LOG_EWK_API_DEPRECATED("This API is deprecated, not Supported by chromium. spdy is deprecated in m50 and replace with HTTP2 handling.");
  return EINA_FALSE;
}

Eina_Bool ewk_settings_spdy_enabled_get(Ewk_Settings* settings)
{
  LOG_EWK_API_DEPRECATED("This API is deprecated, not Supported by chromium. spdy is deprecated in m50 and replace with HTTP2 handling.");
  return EINA_FALSE;
}

Eina_Bool ewk_settings_performance_features_enabled_set(Ewk_Settings* settings, Eina_Bool spdyEnabled)
{
  (void) settings;
  (void) spdyEnabled;
  // This function was used to set some libsoup settings.
  // Chromium doesn't use libsoup.
  LOG_EWK_API_MOCKUP("Not Supported by chromium");
  return EINA_FALSE;
}

Eina_Bool ewk_settings_performance_features_enabled_get(const Ewk_Settings* settings)
{
  LOG_EWK_API_MOCKUP();
  return false;
}

void ewk_settings_link_magnifier_enabled_set(Ewk_Settings* settings, Eina_Bool enabled)
{
  EINA_SAFETY_ON_NULL_RETURN(settings);
  EWK_VIEW_IMPL_GET_OR_RETURN(settings->getEvasObject(), webview);

  webview->SetLinkMagnifierEnabled(enabled);
}

Eina_Bool ewk_settings_link_magnifier_enabled_get(const Ewk_Settings *settings)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(settings, EINA_FALSE);
  EWK_VIEW_IMPL_GET_OR_RETURN(const_cast<Ewk_Settings *>(settings)->getEvasObject(), webview, EINA_FALSE);

  return webview->GetLinkMagnifierEnabled();
}

Eina_Bool ewk_settings_link_effect_enabled_set(Ewk_Settings* settings, Eina_Bool linkEffectEnabled)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);
  settings->setLinkEffectEnabled(linkEffectEnabled);
  return true;
}

Eina_Bool ewk_settings_link_effect_enabled_get(const Ewk_Settings* settings)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);
  return settings->getLinkEffectEnabled();
}

Eina_Bool ewk_settings_uses_encoding_detector_set(Ewk_Settings* settings, Eina_Bool use)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);
#if defined(USE_EFL)
  settings->getPreferences().uses_encoding_detector = use;
#endif
  ewkUpdateWebkitPreferences(settings->getEvasObject());
  return true;
}

Eina_Bool ewk_settings_uses_encoding_detector_get(const Ewk_Settings* settings)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);
#if defined(USE_EFL)
  return settings->getPreferences().uses_encoding_detector;
#else
  return false;
#endif
}

Eina_Bool ewk_settings_default_keypad_enabled_set(Ewk_Settings* settings, Eina_Bool enable)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);

    settings->setEnableDefaultKeypad(enable);

    return true;
}

Eina_Bool ewk_settings_default_keypad_enabled_get(const Ewk_Settings* settings)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);

    return settings->defaultKeypadEnabled();
}

Eina_Bool ewk_settings_uses_keypad_without_user_action_set(Ewk_Settings* settings, Eina_Bool use)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);
  settings->setUseKeyPadWithoutUserAction(use);
  return true;
}

Eina_Bool ewk_settings_uses_keypad_without_user_action_get(const Ewk_Settings* settings)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);
  return settings->useKeyPadWithoutUserAction();
}

Eina_Bool ewk_settings_text_zoom_enabled_set(Ewk_Settings* settings, Eina_Bool enable)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);
  settings->setTextZoomEnabled(enable);
  return true;
}

Eina_Bool ewk_settings_text_zoom_enabled_get(const Ewk_Settings* settings)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);
  return settings->textZoomEnabled();
}
/* LCOV_EXCL_STOP */

Eina_Bool ewk_settings_autofill_password_form_enabled_set(Ewk_Settings* settings, Eina_Bool enable)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);
  settings->setAutofillPasswordForm(enable);
  ewkUpdateWebkitPreferences(settings->getEvasObject());
  return true;
}

/* LCOV_EXCL_START */
Eina_Bool ewk_settings_autofill_password_form_enabled_get(Ewk_Settings* settings)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);
  return settings->autofillPasswordForm();
}
/* LCOV_EXCL_STOP */

Eina_Bool ewk_settings_form_candidate_data_enabled_set(Ewk_Settings* settings, Eina_Bool enable)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);
  settings->setFormCandidateData(enable);
  return true;
}

/* LCOV_EXCL_START */
Eina_Bool ewk_settings_form_candidate_data_enabled_get(Ewk_Settings* settings)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);
  return settings->formCandidateData();
}
/* LCOV_EXCL_STOP */
Eina_Bool ewk_settings_form_profile_data_enabled_set(Ewk_Settings *settings, Eina_Bool enable)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);
  settings->setAutofillProfileForm(enable);
  ewkUpdateWebkitPreferences(settings->getEvasObject());
  return true;
}

/* LCOV_EXCL_START */
Eina_Bool ewk_settings_form_profile_data_enabled_get(const Ewk_Settings *settings)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);
  return settings->autofillProfileForm();
}

Eina_Bool ewk_settings_current_legacy_font_size_mode_set(Ewk_Settings *settings, Ewk_Legacy_Font_Size_Mode mode)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);

  settings->setCurrentLegacyFontSizeMode(mode);
  ewkUpdateWebkitPreferences(settings->getEvasObject());
  return true;
}

Ewk_Legacy_Font_Size_Mode ewk_settings_current_legacy_font_size_mode_get(const Ewk_Settings *settings)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(settings, EWK_LEGACY_FONT_SIZE_MODE_ONLY_IF_PIXEL_VALUES_MATCH);
  return settings->currentLegacyFontSizeMode();
}

Eina_Bool ewk_settings_paste_image_uri_mode_set(Ewk_Settings *settings, Eina_Bool enable)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);
  // FIXME: To be implemented when the functionality is required.
  // To be added in WebkitPreferences -> WebSettings -> Settings
  settings->setPasteImageUriEnabled(enable);
  return true;
}

Eina_Bool ewk_settings_paste_image_uri_mode_get(const Ewk_Settings *settings)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);
  // FIXME: To be implemented when the functionality is required.
  // To be added in WebkitPreferences -> WebSettings -> Settings
  return settings->pasteImageUriEnabled();
}


Eina_Bool ewk_settings_text_selection_enabled_set(Ewk_Settings* settings, Eina_Bool enable)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);
  settings->setTextSelectionEnabled(enable);
  return true;
}

Eina_Bool ewk_settings_text_selection_enabled_get(const Ewk_Settings* settings)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);
  return settings->textSelectionEnabled();
}

Eina_Bool ewk_settings_clear_text_selection_automatically_set(Ewk_Settings* settings, Eina_Bool enable)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);
  settings->setAutoClearTextSelection(enable);
  return true;
}

Eina_Bool ewk_settings_clear_text_selection_automatically_get(const Ewk_Settings* settings)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);
  return settings->autoClearTextSelection();
}

Eina_Bool ewk_settings_text_autosizing_enabled_set(Ewk_Settings* settings, Eina_Bool enable)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);
  settings->getPreferences().text_autosizing_enabled = enable;
  ewkUpdateWebkitPreferences(settings->getEvasObject());
  return true;
}

Eina_Bool ewk_settings_text_autosizing_enabled_get(const Ewk_Settings* settings)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);
  return settings->getPreferences().text_autosizing_enabled;
}

Eina_Bool ewk_settings_text_autosizing_font_scale_factor_set(Ewk_Settings* settings, double factor)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);
  if (factor <= 0.0) {
    return false;
  }
  settings->getPreferences().font_scale_factor = factor;
  ewkUpdateWebkitPreferences(settings->getEvasObject());
  return true;
}

double ewk_settings_text_autosizing_font_scale_factor_get(const Ewk_Settings* settings)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(settings, -1.0);
  return settings->getPreferences().font_scale_factor;
}

Eina_Bool ewk_settings_edge_effect_enabled_set(Ewk_Settings* settings, Eina_Bool enable)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);
  settings->setEdgeEffectEnabled(enable);
  return true;
}

Eina_Bool ewk_settings_edge_effect_enabled_get(const Ewk_Settings* settings)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);
  return settings->edgeEffectEnabled();
}

void ewk_settings_text_style_state_enabled_set(Ewk_Settings* settings, Eina_Bool enabled)
{
  EINA_SAFETY_ON_NULL_RETURN(settings);
  settings->setTextStyleStateState(enabled);
}

Eina_Bool ewk_settings_text_style_state_enabled_get(const Ewk_Settings* settings)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);
  return settings->textStyleStateState();
}

Eina_Bool ewk_settings_select_word_automatically_set(Ewk_Settings* settings, Eina_Bool enable)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);
  settings->setAutoSelectWord(enable);
  ewkUpdateWebkitPreferences(settings->getEvasObject());
  return true;
}

Eina_Bool ewk_settings_select_word_automatically_get(const Ewk_Settings* settings)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);
  return settings->autoSelectWord();
}

Ewk_List_Style_Position ewk_settings_initial_list_style_position_get(const Ewk_Settings* settings)
{
  return EWK_LIST_STYLE_POSITION_OUTSIDE;
}

Eina_Bool ewk_settings_initial_list_style_position_set(Ewk_Settings* settings, Ewk_List_Style_Position style)
{
  return EINA_FALSE;
}

Eina_Bool ewk_settings_webkit_text_size_adjust_enabled_set(Ewk_Settings* settings, Eina_Bool enabled)
{
  // Chromium does not support CSS property "-webkit-text-size-adjust"
  // and this API was created to disable this property.
  // There is no need to implement this API.
  LOG_EWK_API_MOCKUP("Not Supported by chromium");
  return false;
}

// TODO: this API is gone in newer versions of webkit-efl
void ewk_settings_detect_contents_automatically_set(Ewk_Settings* settings, Eina_Bool enable)
{
  EINA_SAFETY_ON_NULL_RETURN(settings);
  settings->setDetectContentsEnabled(enable);
}

Eina_Bool ewk_settings_detect_contents_automatically_get(const Ewk_Settings* settings)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);
  return settings->getDetectContentsEnabled();
}

void ewk_settings_cache_builder_enabled_set(Ewk_Settings *settings, Eina_Bool enabled)
{
  // This API is deprecated because cache builder is available on Webkit only.
  LOG_EWK_API_MOCKUP("Not Supported by chromium");
}
/* LCOV_EXCL_STOP */
int ewk_settings_default_font_size_get(const Ewk_Settings* settings)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(settings, 0);
  return settings->getPreferences().default_font_size;
}

Eina_Bool ewk_settings_default_font_size_set(Ewk_Settings* settings, int size)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);
  settings->getPreferences().default_font_size = size;
  return true;
}

const char* ewk_settings_default_text_encoding_name_get(const Ewk_Settings* settings)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(settings, NULL);
  return settings->defaultTextEncoding();
}

Eina_Bool ewk_settings_default_text_encoding_name_set(Ewk_Settings* settings, const char* encoding)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);
  EINA_SAFETY_ON_NULL_RETURN_VAL(encoding, false);
  settings->setDefaultTextEncoding(encoding);
  ewkUpdateWebkitPreferences(settings->getEvasObject());
  return true;
}

Eina_Bool ewk_settings_scripts_can_open_windows_set(Ewk_Settings* settings, Eina_Bool enable)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);
  settings->getPreferencesEfl().javascript_can_open_windows_automatically_ewk = enable;
  ewkUpdateWebkitPreferences(settings->getEvasObject());
  return true;
}

Eina_Bool ewk_settings_scripts_can_open_windows_get(const Ewk_Settings* settings)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);
  return settings->getPreferencesEfl().javascript_can_open_windows_automatically_ewk;
}

/* LCOV_EXCL_START */
Eina_Bool ewk_settings_encoding_detector_enabled_set(Ewk_Settings* settings, Eina_Bool enable)
{
  LOG_EWK_API_MOCKUP("for Tizen TV Browser");
  return false;
}
/* LCOV_EXCL_STOP */
namespace {
#define EXTRA_FEATURE_FUNCTIONS(VALNAME) \
  void Ewk_Extra_Feature_Set_ ## VALNAME(Ewk_Settings* settings, Eina_Bool value) \
  { \
    EINA_SAFETY_ON_NULL_RETURN(settings); \
    settings->set ## VALNAME ## Enabled(value == EINA_TRUE); \
  } \
  Eina_Bool Ewk_Extra_Feature_Get_ ## VALNAME(const Ewk_Settings* settings) \
  { \
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, EINA_FALSE); \
    return settings->get ## VALNAME ## Enabled() ? EINA_TRUE : EINA_FALSE; \
  }

#define EWK_EXTRA_FEATURE(NAME, VALNAME) {NAME, Ewk_Extra_Feature_Set_## VALNAME, Ewk_Extra_Feature_Get_ ## VALNAME}

  EXTRA_FEATURE_FUNCTIONS(LongPress)
  EXTRA_FEATURE_FUNCTIONS(LinkMagnifier)
  EXTRA_FEATURE_FUNCTIONS(DetectContents)
  EXTRA_FEATURE_FUNCTIONS(WebLogin)
  EXTRA_FEATURE_FUNCTIONS(DoubleTap)
  EXTRA_FEATURE_FUNCTIONS(Zoom)
  EXTRA_FEATURE_FUNCTIONS(OpenPanel)
  EXTRA_FEATURE_FUNCTIONS(AllowRestrictedURL)
  EXTRA_FEATURE_FUNCTIONS(URLBarHide)

  void Ewk_Extra_Feature_Set_ViewportQuirk(Ewk_Settings* settings, Eina_Bool value)
  {
    EINA_SAFETY_ON_NULL_RETURN(settings);
    settings->getPreferences().viewport_meta_non_user_scalable_quirk = value;
    ewkUpdateWebkitPreferences(settings->getEvasObject());
  }

  Eina_Bool Ewk_Extra_Feature_Get_ViewportQuirk(const Ewk_Settings* settings)
  {
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);
    return settings->getPreferences().viewport_meta_non_user_scalable_quirk;
  }

  typedef struct {
    const char* name;
    void (*set)(Ewk_Settings*, Eina_Bool enable);
    Eina_Bool (*get)(const Ewk_Settings*);
  } Ewk_Extra_Feature;

  static Ewk_Extra_Feature extra_features[] = {
    EWK_EXTRA_FEATURE("longpress,enable", LongPress),
    EWK_EXTRA_FEATURE("link,magnifier", LinkMagnifier),
    EWK_EXTRA_FEATURE("detect,contents", DetectContents),
    EWK_EXTRA_FEATURE("web,login", WebLogin),
    EWK_EXTRA_FEATURE("doubletap,enable", DoubleTap),
    EWK_EXTRA_FEATURE("zoom,enable", Zoom),
    EWK_EXTRA_FEATURE("openpanel,enable", OpenPanel),
    EWK_EXTRA_FEATURE("allow,restrictedurl", AllowRestrictedURL),
    EWK_EXTRA_FEATURE("urlbar,hide", URLBarHide),
    EWK_EXTRA_FEATURE("wrt,viewport,quirk", ViewportQuirk),
    {NULL, NULL, NULL}
  };

  Ewk_Extra_Feature* find_extra_feature(const char*feature)
  {
    for(Ewk_Extra_Feature *feat = extra_features; feat->name != NULL; feat++) {
      if (strcasecmp(feat->name, feature) == 0) {
        return feat;
      }
    }

    return NULL;
  }
}

/* LCOV_EXCL_START */
Eina_Bool ewk_settings_extra_feature_get(const Ewk_Settings* settings, const char* feature)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(settings, EINA_FALSE);
  EINA_SAFETY_ON_NULL_RETURN_VAL(feature, EINA_FALSE);

  std::string feature_name(feature);
  if (base::EqualsCaseInsensitiveASCII(feature_name, "edge,enable"))
    return settings->edgeEffectEnabled();
  else if (base::EqualsCaseInsensitiveASCII(feature_name, "zoom,enable"))
    return settings->textZoomEnabled();
  else if (base::EqualsCaseInsensitiveASCII(feature_name,
                                            "selection,magnifier"))
    return settings->getPreferences().selection_magnifier_enabled;
  else if (base::EqualsCaseInsensitiveASCII(feature_name, "longpress,enable"))
    return settings->getPreferences().long_press_enabled;
  else if (base::EqualsCaseInsensitiveASCII(feature_name, "doubletap,enable"))
    return settings->getPreferences().double_tap_to_zoom_enabled;
  return EINA_FALSE;
}

void ewk_settings_extra_feature_set(Ewk_Settings* settings, const char* feature, Eina_Bool enable)
{
  EINA_SAFETY_ON_NULL_RETURN(settings);
  EINA_SAFETY_ON_NULL_RETURN(feature);

  std::string feature_name(feature);
  if (base::EqualsCaseInsensitiveASCII(feature_name, "edge,enable"))
    settings->setEdgeEffectEnabled(enable);
  else if (base::EqualsCaseInsensitiveASCII(feature_name, "zoom,enable"))
    settings->setTextZoomEnabled(enable);
  else if (base::EqualsCaseInsensitiveASCII(feature_name,
                                            "selection,magnifier"))
    settings->getPreferences().selection_magnifier_enabled = enable;
  else if (base::EqualsCaseInsensitiveASCII(feature_name, "longpress,enable"))
    settings->getPreferences().long_press_enabled = enable;
  else if (base::EqualsCaseInsensitiveASCII(feature_name, "doubletap,enable"))
    settings->getPreferences().double_tap_to_zoom_enabled = enable;
}

Eina_Bool ewk_settings_tizen_compatibility_mode_set(Ewk_Settings* settings, unsigned major, unsigned minor, unsigned release)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);
  settings->getPreferences().tizen_version.major = major;
  settings->getPreferences().tizen_version.minor = minor;
  settings->getPreferences().tizen_version.release = release;
  ewkUpdateWebkitPreferences(settings->getEvasObject());
  return true;
}

Eina_Bool ewk_settings_webkit_text_size_adjust_enabled_get(const Ewk_Settings* settings)
{
  LOG_EWK_API_MOCKUP();
  return false;
}

Eina_Bool ewk_settings_scan_malware_enabled_get(const Ewk_Settings* settings)
{
  LOG_EWK_API_MOCKUP();
  return false;
}

void ewk_settings_default_mixed_contents_policy_set(Ewk_Settings* settings, Eina_Bool enable)
{
#if defined(OS_TIZEN_TV_PRODUCT)
  EINA_SAFETY_ON_NULL_RETURN(settings);
  settings->getPreferences().allow_running_insecure_content = enable;
  ewkUpdateWebkitPreferences(settings->getEvasObject());
#else
  LOG_EWK_API_MOCKUP();
#endif
}

void ewk_settings_disable_webgl_set(Ewk_Settings* settings, Eina_Bool disable)
{
#if defined(OS_TIZEN_TV_PRODUCT)
  EINA_SAFETY_ON_NULL_RETURN(settings);
  settings->getPreferences().webgl1_enabled = !disable;
  settings->getPreferences().webgl2_enabled = !disable;
  ewkUpdateWebkitPreferences(settings->getEvasObject());
#else
  LOG_EWK_API_MOCKUP("Only for Tizen TV");
#endif
}

Eina_Bool ewk_settings_uses_arrow_scroll_set(Ewk_Settings* settings, Eina_Bool enabled)
{
#if defined(OS_TIZEN_TV_PRODUCT)
  EINA_SAFETY_ON_NULL_RETURN_VAL(settings, EINA_FALSE);
  settings->getPreferences().use_arrow_scroll = !!enabled;
  ewkUpdateWebkitPreferences(settings->getEvasObject());
  return EINA_TRUE;
#else
  LOG_EWK_API_MOCKUP("Only for Tizen TV");
  return EINA_FALSE;
#endif
}

Eina_Bool ewk_settings_uses_arrow_scroll_get(Ewk_Settings* settings)
{
#if defined(OS_TIZEN_TV_PRODUCT)
  EINA_SAFETY_ON_NULL_RETURN_VAL(settings, EINA_FALSE);
  return settings->getPreferences().use_arrow_scroll;
#else
  LOG_EWK_API_MOCKUP("Only for Tizen TV");
  return EINA_FALSE;
#endif
}

Eina_Bool ewk_settings_uses_scrollbar_thumb_focus_notifications_set(Ewk_Settings* settings, Eina_Bool use)
{
#if defined(OS_TIZEN_TV_PRODUCT)
  EINA_SAFETY_ON_NULL_RETURN_VAL(settings, EINA_FALSE);
  settings->getPreferences().use_scrollbar_thumb_focus_notifications = use;
  ewkUpdateWebkitPreferences(settings->getEvasObject());
  return EINA_TRUE;
#else
  NOTIMPLEMENTED() << "This API is only available in Tizen TV product.";
  return EINA_FALSE;
#endif
}

void ewk_settings_media_playback_notification_set(Ewk_Settings* settings, Eina_Bool enabled)
{
#if defined(OS_TIZEN_TV_PRODUCT)
  EINA_SAFETY_ON_NULL_RETURN(settings);
  LOG(INFO) << " set enabled:" << enabled;
  settings->getPreferences().media_playback_notification_enabled = enabled;
  ewkUpdateWebkitPreferences(settings->getEvasObject());
#else
  LOG_EWK_API_MOCKUP();
#endif
}

Eina_Bool ewk_settings_media_playback_notification_get(const Ewk_Settings* settings)
{
#if defined(OS_TIZEN_TV_PRODUCT)
  EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);
  return settings->getPreferences().media_playback_notification_enabled;
#else
  LOG_EWK_API_MOCKUP();
  return false;
#endif
}

void ewk_settings_media_subtitle_notification_set(Ewk_Settings *settings, Eina_Bool enabled)
{
#if defined(OS_TIZEN_TV_PRODUCT)
  LOG(INFO) << " set enabled:" << (bool)enabled;
  EINA_SAFETY_ON_NULL_RETURN(settings);
  settings->getPreferences().media_subtitle_notification_enabled = (bool)enabled;
  ewkUpdateWebkitPreferences(settings->getEvasObject());
#else
  LOG_EWK_API_MOCKUP("Only for Tizen TV");
#endif
}

Eina_Bool ewk_settings_media_subtitle_notification_get(const Ewk_Settings *settings)
{
#if defined(OS_TIZEN_TV_PRODUCT)
  EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);
  return settings->getPreferences().media_subtitle_notification_enabled;
#else
  LOG_EWK_API_MOCKUP("Only for Tizen TV");
  return false;
#endif
}

Eina_Bool ewk_settings_text_autosizing_scale_factor_set(Ewk_Settings* settings, double factor)
{
  LOG_EWK_API_MOCKUP();
  return false;
}

double ewk_settings_text_autosizing_scale_factor_get(const Ewk_Settings* settings)
{
  LOG_EWK_API_MOCKUP();
  return 0;
}

Eina_Bool ewk_settings_legacy_font_size_enabled_set(Ewk_Settings* settings, Eina_Bool enabled)
{
  LOG_EWK_API_MOCKUP();
  return false;
}

Eina_Bool ewk_settings_legacy_font_size_enabled_get(Ewk_Settings* settings)
{
  LOG_EWK_API_MOCKUP();
  return false;
}

Eina_Bool ewk_settings_use_system_font_set(Ewk_Settings* settings, Eina_Bool use)
{
  if (IsMobileProfile() || IsWearableProfile()) {
    EWebView* impl = EWebView::FromEvasObject(settings->getEvasObject());
    if (impl) {
      if (use)
        impl->UseSettingsFont();
      else
        impl->SetBrowserFont();
      return EINA_TRUE;
    }
  }
  return EINA_FALSE;
}

Eina_Bool ewk_settings_use_system_font_get(Ewk_Settings* settings)
{
  LOG_EWK_API_MOCKUP();
  return false;
}

void ewk_settings_selection_handle_enabled_set(Ewk_Settings* settings, Eina_Bool enable)
{
  LOG_EWK_API_MOCKUP();
}

Eina_Bool ewk_settings_selection_handle_enabled_get(const Ewk_Settings* settings)
{
  LOG_EWK_API_MOCKUP();
  return false;
}

void ewk_settings_disclose_set_cookie_headers_enabled(Ewk_Settings* settings, Eina_Bool Enabled)
{
#if defined(OS_TIZEN_TV_PRODUCT)
  EINA_SAFETY_ON_NULL_RETURN(settings);
  net::HttpResponseHeaders::set_disclose_set_cookie_enabled(Enabled);
#else
  LOG_EWK_API_MOCKUP("Only for Tizen TV");
#endif
}

Eina_Bool ewk_settings_allow_file_access_from_external_url_set(Ewk_Settings* settings, Eina_Bool allow)
{
#if defined(OS_TIZEN_TV_PRODUCT)
  LOG(INFO) << " - allow:" << allow;
  EINA_SAFETY_ON_NULL_RETURN_VAL(settings, EINA_FALSE);
  settings->getPreferences().allow_file_access_from_external_urls = allow;
  ewkUpdateWebkitPreferences(settings->getEvasObject());
  return EINA_TRUE;
#else
  LOG_EWK_API_MOCKUP("Only for Tizen TV");
  return false;
#endif

}

Eina_Bool ewk_settings_allow_file_access_from_external_url_get(const Ewk_Settings* settings)
{
#if defined(OS_TIZEN_TV_PRODUCT)
  EINA_SAFETY_ON_NULL_RETURN_VAL(settings, EINA_FALSE);
  return settings->getPreferences().allow_file_access_from_external_urls;
#else
  LOG_EWK_API_MOCKUP("Only for Tizen TV");
  return false;
#endif

}

Eina_Bool ewk_settings_do_not_track_set(Ewk_Settings* settings, Eina_Bool enabled)
{
  EWK_VIEW_IMPL_GET_OR_RETURN(
      const_cast<Ewk_Settings*>(settings)->getEvasObject(), webview,
      EINA_FALSE);
  webview->SetDoNotTrack(enabled);
  return true;
}

Eina_Bool ewk_settings_mixed_contents_set(const Ewk_Settings* settings, Eina_Bool allow)
{
#if defined(OS_TIZEN_TV_PRODUCT)
  EINA_SAFETY_ON_NULL_RETURN_VAL(settings, EINA_FALSE);
  LOG(INFO)<<"Set mixed contents: " << allow;
  EWK_VIEW_IMPL_GET_OR_RETURN(
    const_cast<Ewk_Settings*>(settings)->getEvasObject(), webview, EINA_FALSE);
  return webview->SetMixedContents(allow);
#else
  LOG_EWK_API_MOCKUP("Only for Tizen TV");
  return EINA_FALSE;
#endif
}

void ewk_settings_focus_ring_enabled_set(Ewk_Settings *settings, Eina_Bool enabled)
{
  // This API is deprecated because it is exactly same
  // as ewk_view_draw_focus_ring_enable_set.
  LOG_EWK_API_MOCKUP("This API is deprecated, not Supported by chromium");
}

void ewk_settings_cache_builder_extension_enabled_set(Ewk_Settings* settings, Eina_Bool enabled)
{
  // This API is deprecated because cache builder is available on Webkit only.
  LOG_EWK_API_MOCKUP("This API is deprecated, not Supported by chromium");
}

Eina_Bool ewk_settings_viewport_meta_tag_set(Ewk_Settings *settings, Eina_Bool enable)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);
  // NOTE: WebPreferences::viewport_enabled is decided according to
  // WebPreferences::viewport_meta_enabled
  // at RenderViewHostImpl::ComputeWebkitPrefs once ewk_settings is created.
  // After that, it cannot be changed. So we also change it here by force.
  settings->getPreferences().viewport_enabled = enable;
  settings->getPreferences().viewport_meta_enabled = enable;
  ewkUpdateWebkitPreferences(settings->getEvasObject());
  return true;
}

Eina_Bool ewk_settings_viewport_meta_tag_get(const Ewk_Settings *settings)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);
  return settings->getPreferences().viewport_enabled &&
         settings->getPreferences().viewport_meta_enabled;
}

Eina_Bool ewk_settings_web_security_enabled_set(Ewk_Settings* settings, Eina_Bool enable)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(settings, EINA_FALSE);
  settings->getPreferences().web_security_enabled = enable;
  ewkUpdateWebkitPreferences(settings->getEvasObject());
  return EINA_TRUE;
}

Eina_Bool ewk_settings_web_security_enabled_get(const Ewk_Settings* settings)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(settings, EINA_FALSE);
  return settings->getPreferences().web_security_enabled;
}

void ewk_settings_spatial_navigation_enabled_set(Ewk_Settings* settings, Eina_Bool Enabled)
{
#if defined(TIZEN_ATK_FEATURE_VD)
  EINA_SAFETY_ON_NULL_RETURN(settings);
  settings->getPreferences().spatial_navigation_enabled = !!Enabled;
  ewkUpdateWebkitPreferences(settings->getEvasObject());
#else
  LOG_EWK_API_MOCKUP();
#endif
}

Eina_Bool ewk_settings_javascript_can_access_clipboard_set(Ewk_Settings* settings, Eina_Bool enable)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(settings, EINA_FALSE);
  settings->getPreferences().javascript_can_access_clipboard = enable;
  ewkUpdateWebkitPreferences(settings->getEvasObject());
  return EINA_TRUE;
}

Eina_Bool ewk_settings_javascript_can_access_clipboard_get(const Ewk_Settings* settings)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(settings, EINA_FALSE);
  return settings->getPreferences().javascript_can_access_clipboard;
}

Eina_Bool ewk_settings_dom_paste_allowed_set(Ewk_Settings* settings, Eina_Bool enable)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(settings, EINA_FALSE);
  settings->getPreferences().dom_paste_enabled = enable;
  ewkUpdateWebkitPreferences(settings->getEvasObject());
  return EINA_TRUE;
}

Eina_Bool ewk_settings_dom_paste_allowed_get(const Ewk_Settings* settings)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(settings, EINA_FALSE);
  return settings->getPreferences().dom_paste_enabled;
}

void ewk_settings_ime_panel_enabled_set(Ewk_Settings* settings, Eina_Bool Enabled)
{
  EINA_SAFETY_ON_NULL_RETURN(settings);
  LOG(INFO)<<"ewk_settings_ime_panel_enabled_set,Enabled:"<<(bool)Enabled;
  settings->setImePanelEnabled(Enabled);
}

Eina_Bool ewk_settings_ime_panel_enabled_get(const Ewk_Settings* settings)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);
  return settings->imePanelEnabled();
}

void ewk_settings_drag_drop_enabled_set(Ewk_Settings* settings, Eina_Bool enabled)
{
#if defined(OS_TIZEN_TV_PRODUCT)
  EINA_SAFETY_ON_NULL_RETURN(settings);
  settings->getPreferences().drag_drop_enabled = !!enabled;
  ewkUpdateWebkitPreferences(settings->getEvasObject());
#else
    LOG_EWK_API_MOCKUP("Only for Tizen TV");
#endif
}

Eina_Bool ewk_settings_drag_drop_enabled_get(const Ewk_Settings* settings)
{
#if defined(OS_TIZEN_TV_PRODUCT)
  EINA_SAFETY_ON_NULL_RETURN_VAL(settings, EINA_FALSE);
  return settings->getPreferences().drag_drop_enabled;
#else
  LOG_EWK_API_MOCKUP("Only for Tizen TV");
  return EINA_FALSE;
#endif
}

void ewk_settings_clipboard_enabled_set(Ewk_Settings* settings, Eina_Bool enabled)
{
#if defined(OS_TIZEN_TV_PRODUCT)
  EINA_SAFETY_ON_NULL_RETURN(settings);
  settings->setClipboardEnabled(!!enabled);
#else
  LOG_EWK_API_MOCKUP("Only for Tizen TV");
#endif
}

Eina_Bool ewk_settings_clipboard_enabled_get(const Ewk_Settings* settings)
{
#if defined(OS_TIZEN_TV_PRODUCT)
  EINA_SAFETY_ON_NULL_RETURN_VAL(settings, EINA_FALSE);
  return settings->getClipboardEnabled();
#else
  LOG_EWK_API_MOCKUP("Only for Tizen TV");
  return EINA_FALSE;
#endif
}

/* LCOV_EXCL_STOP */
