// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ewk_settings_private.h"

#include "net/http/http_stream_factory.h"
#include "content/browser/renderer_host/edge_effect.h"

Ewk_Settings::Ewk_Settings(Evas_Object* evas_object,
                           const content::WebPreferences& preferences)
    : m_preferences(preferences),
      m_autofillPasswordForm(false),
      m_formCandidateData(false),
      m_autofillProfileForm(false),
      m_textSelectionEnabled(true),
      m_autoClearTextSelection(true),
      m_autoSelectWord(false),
      m_edgeEffectEnabled(true),
      m_loadRemoteImages(true),
      m_currentLegacyFontSizeMode(
          EWK_LEGACY_FONT_SIZE_MODE_ONLY_IF_PIXEL_VALUES_MATCH),
      m_pasteImageUriEnabled(true),
      m_defaultKeypadEnabled(true),
      m_useKeyPadWithoutUserAction(true),
      m_textStyleState(true),
      m_detectContentsAutomatically(true),
      m_evas_object(evas_object),
      m_cacheBuilderEnabled(false),
      m_longPressEnabled(true),
      m_linkMagnifierEnabled(false),
      m_detectContentsEnabled(false),
      m_webLoginEnabled(false),
      m_doubleTapEnabled(true),
      m_zoomEnabled(true),
      m_openPanelEnabled(true),
      m_allowRestrictedURL(true),
      m_URLBarHide(false),
      m_linkEffectEnabled(true),
      m_imePanelEnabled(true),
      m_clipboardEnabled(false) {}

Ewk_Settings::~Ewk_Settings() {}

/* LCOV_EXCL_START */
void Ewk_Settings::setCurrentLegacyFontSizeMode(Ewk_Legacy_Font_Size_Mode mode) {
  m_currentLegacyFontSizeMode = mode;
}
/* LCOV_EXCL_STOP */

void Ewk_Settings::setDefaultTextEncoding(const char* encoding) {
  m_preferences.uses_encoding_detector = !(encoding && *encoding);
  if (encoding)
    m_preferences.default_encoding = encoding;
}

/* LCOV_EXCL_START */
void Ewk_Settings::setTextZoomEnabled(bool enable) {
  m_preferences.text_zoom_enabled = enable;
}

void Ewk_Settings::setEdgeEffectEnabled(bool enable) {
  m_edgeEffectEnabled = enable;
  content::EdgeEffect::EnableGlobally(enable);
}
/* LCOV_EXCL_STOP */
