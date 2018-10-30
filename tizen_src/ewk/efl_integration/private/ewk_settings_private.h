// Copyright 2013 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ewk_settings_private_h
#define ewk_settings_private_h

#include <Eina.h>
#include <Evas.h>

#include <content/public/common/web_preferences.h>
#include "common/web_preferences_efl.h"
#include "public/ewk_settings_internal.h"

class Ewk_Settings {
  public:
   Ewk_Settings(Evas_Object* evas_object,
                const content::WebPreferences& preferences);
   ~Ewk_Settings();
   const char* defaultTextEncoding() const {
     return m_preferences.default_encoding.c_str();
    }
    void setDefaultTextEncoding(const char*);
    bool autofillPasswordForm() const { return m_autofillPasswordForm; }
    void setAutofillPasswordForm(bool enable) { m_autofillPasswordForm = enable; }
    bool formCandidateData() const { return m_formCandidateData; }
    void setFormCandidateData(bool enable) { m_formCandidateData = enable; }
    bool autofillProfileForm() const { return m_autofillProfileForm; }
    void setAutofillProfileForm(bool enable) { m_autofillProfileForm = enable; }
    bool textSelectionEnabled() const { return m_textSelectionEnabled; }
    /* LCOV_EXCL_START */
    void setTextSelectionEnabled(bool enable) {
      m_textSelectionEnabled = enable;
    }
    /* LCOV_EXCL_STOP */
    bool autoClearTextSelection() const { return m_autoClearTextSelection; }
    /* LCOV_EXCL_START */
    void setAutoClearTextSelection(bool enable) {
      m_autoClearTextSelection = enable;
    }
    /* LCOV_EXCL_STOP */
    bool autoSelectWord() const { return m_autoSelectWord; }
    /* LCOV_EXCL_START */
    void setAutoSelectWord(bool enable) { m_autoSelectWord = enable; }
    /* LCOV_EXCL_STOP */
    bool edgeEffectEnabled() const { return m_edgeEffectEnabled; }
    void setEdgeEffectEnabled(bool enable);
    void setTextZoomEnabled(bool enable);
    bool textZoomEnabled() const { return m_preferences.text_zoom_enabled; }
    /* LCOV_EXCL_START */
    void setLoadRemoteImages(bool loadRemoteImages) {
      m_loadRemoteImages = loadRemoteImages;
    }
    /* LCOV_EXCL_STOP */
    bool loadRemoteImages() const { return m_loadRemoteImages; }
    void setCurrentLegacyFontSizeMode(Ewk_Legacy_Font_Size_Mode mode);
    Ewk_Legacy_Font_Size_Mode currentLegacyFontSizeMode() const { return m_currentLegacyFontSizeMode; }
    /* LCOV_EXCL_START */
    void setPasteImageUriEnabled(bool enable) {
      m_pasteImageUriEnabled = enable;
    }
    /* LCOV_EXCL_STOP */
    bool pasteImageUriEnabled() const {return m_pasteImageUriEnabled;}
    bool defaultKeypadEnabled() const { return m_defaultKeypadEnabled; }
    /* LCOV_EXCL_START */
    void setEnableDefaultKeypad(bool flag) { m_defaultKeypadEnabled = flag; }
    /* LCOV_EXCL_STOP */
    bool useKeyPadWithoutUserAction() const { return m_useKeyPadWithoutUserAction; }
    /* LCOV_EXCL_START */
    void setUseKeyPadWithoutUserAction(bool enable) {
      m_useKeyPadWithoutUserAction = enable;
    }
    /* LCOV_EXCL_STOP */
    bool textStyleStateState() const { return m_textStyleState; }
    /* LCOV_EXCL_START */
    void setTextStyleStateState(bool enable) { m_textStyleState = enable; }
    /* LCOV_EXCL_STOP */
    void setDetectContentsAutomatically(bool enable) { m_detectContentsAutomatically = enable; }
    bool detectContentsAutomatically() const { return m_detectContentsAutomatically; }

    Evas_Object* getEvasObject() { return m_evas_object; }

    content::WebPreferences& getPreferences() { return m_preferences; }
    const content::WebPreferences& getPreferences() const { return m_preferences; }

    WebPreferencesEfl& getPreferencesEfl() { return m_preferences_efl; }
    const WebPreferencesEfl& getPreferencesEfl() const { return m_preferences_efl; }

    void setCacheBuilderEnabled(bool enable) { m_cacheBuilderEnabled = enable; }

    /* ewk_extra_features related */
    void setLongPressEnabled(bool enable) { m_longPressEnabled = enable; }
    bool getLongPressEnabled() const { return m_longPressEnabled; }
    void setLinkMagnifierEnabled(bool enable) { m_linkMagnifierEnabled = enable; }
    bool getLinkMagnifierEnabled() const { return m_linkMagnifierEnabled; }
    void setDetectContentsEnabled(bool enable) { m_detectContentsEnabled = enable; }
    bool getDetectContentsEnabled() const { return m_detectContentsEnabled; }
    void setWebLoginEnabled(bool enable) { m_webLoginEnabled = enable; }
    bool getWebLoginEnabled() const { return m_webLoginEnabled; }
    void setDoubleTapEnabled(bool enable) { m_doubleTapEnabled = enable; }
    bool getDoubleTapEnabled() const { return m_doubleTapEnabled; }
    void setZoomEnabled(bool enable) { m_zoomEnabled = enable; }
    bool getZoomEnabled() const { return m_zoomEnabled; }
    void setOpenPanelEnabled(bool enable) { m_openPanelEnabled = enable; }
    bool getOpenPanelEnabled() const { return m_openPanelEnabled; }
    void setAllowRestrictedURLEnabled(bool enable) { m_allowRestrictedURL = enable; }
    bool getAllowRestrictedURLEnabled() const { return m_allowRestrictedURL; }
    void setURLBarHideEnabled(bool enable) { m_URLBarHide = enable; }
    bool getURLBarHideEnabled() const { return m_URLBarHide; }
    /* LCOV_EXCL_START */
    void setLinkEffectEnabled(bool enable) { m_linkEffectEnabled = enable; }
    /* LCOV_EXCL_STOP */
    bool getLinkEffectEnabled() const { return m_linkEffectEnabled; }

    void setImePanelEnabled(bool enable) { m_imePanelEnabled = enable; }
    bool imePanelEnabled() const { return m_imePanelEnabled; }
    void setClipboardEnabled(bool enable) { m_clipboardEnabled = enable; }
    bool getClipboardEnabled() const { return m_clipboardEnabled; }

  private:
    content::WebPreferences m_preferences;
    WebPreferencesEfl m_preferences_efl;

    bool m_autofillPasswordForm;
    bool m_formCandidateData;
    bool m_autofillProfileForm;
    bool m_textSelectionEnabled;
    bool m_autoClearTextSelection;
    bool m_autoSelectWord;
    bool m_edgeEffectEnabled;
    bool m_loadRemoteImages;
    Ewk_Legacy_Font_Size_Mode m_currentLegacyFontSizeMode;
    bool m_pasteImageUriEnabled;
    bool m_defaultKeypadEnabled;
    bool m_useKeyPadWithoutUserAction;
    bool m_textStyleState;
    bool m_detectContentsAutomatically;

    Evas_Object* m_evas_object;
    bool m_cacheBuilderEnabled;

    /* ewk_extra_features related */
    bool m_longPressEnabled;
    bool m_linkMagnifierEnabled;
    bool m_detectContentsEnabled;
    bool m_webLoginEnabled;
    bool m_doubleTapEnabled;
    bool m_zoomEnabled;
    bool m_openPanelEnabled;
    bool m_allowRestrictedURL;
    bool m_URLBarHide;
    bool m_linkEffectEnabled;
    bool m_imePanelEnabled;
    bool m_clipboardEnabled;
};

#endif // ewk_settings_private_h

