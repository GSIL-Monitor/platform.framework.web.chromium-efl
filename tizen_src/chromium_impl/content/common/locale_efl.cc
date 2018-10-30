// Copyright 2015 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "locale_efl.h"

#include <libintl.h>

#include <unistd.h>

#if defined(OS_TIZEN)
#include <vconf/vconf.h>
#endif
#include <string>

#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/path_service.h"
#include "content/common/paths_efl.h"
#include "content/public/common/content_switches.h"
#include "ui/base/resource/resource_bundle.h"

#if !defined(OS_TIZEN)
typedef void keynode_t;
#endif

namespace LocaleEfl {

namespace {

void ReloadCorrectPakForLocale() {
  if (!ui::ResourceBundle::HasSharedInstance())
    return;

  std::string chosen_locale("en-US");
#if defined(OS_TIZEN)
  char* lang = vconf_get_str(VCONFKEY_LANGSET);
  if (!lang) {
    LOG(WARNING)
        << "Failed to get VCONFKEY_LANGSET. Set to default as \"en-US\".";
  } else {
    chosen_locale = lang;
    free(lang);
  }
#endif

  // Note: locale from vconf is in format:
  // xy_ZW.UTF-8
  // blink pak files are not named same way, so we need to find correct pak.
  bool pak_exist = ui::ResourceBundle::GetSharedInstance().LocaleDataPakExists(
      chosen_locale);

  // xy_ZW.UTF-8 -> xy-ZW
  // Note: pak files are named after IETF_language_tag, which uses '-' not '_'
  if (!pak_exist) {
    chosen_locale = chosen_locale.substr(0, 5);
    if (chosen_locale.length() >= 3 && chosen_locale[2] == '_')
      chosen_locale[2] = '-';

    pak_exist = ui::ResourceBundle::GetSharedInstance().LocaleDataPakExists(
        chosen_locale);
  }

  // xy-ZW -> xy
  if (!pak_exist) {
    chosen_locale = chosen_locale.substr(0, 2);
    pak_exist = ui::ResourceBundle::GetSharedInstance().LocaleDataPakExists(
        chosen_locale);
  }

  if (!pak_exist) {
    LOG(WARNING) << "Unable to find any locale pak for " << chosen_locale;
    return;
  }

  ui::ResourceBundle::GetSharedInstance().ReloadLocaleResources(chosen_locale);
}

void PlatformLanguageChanged(keynode_t* keynode, void* data) {
  std::string str_lang("en");
#if defined(OS_TIZEN)
  char* lang = vconf_get_str(VCONFKEY_LANGSET);
  if (!lang) {
    LOG(WARNING) << "Failed to get VCONFKEY_LANGSET. Set to default as \"en\".";
  } else {
    str_lang = lang;
    free(lang);
  }
#endif

  if (!setlocale(LC_ALL, str_lang.c_str()))
    LOG(WARNING) << "Failed to set LC_ALL to: " << str_lang.c_str();

  // Note: in case when someone will use IDS_BR_* from WebProcess this needs
  // to be enabled or those strings have to be moved to chromium locale files.
  // bindtextdomain("browser", "/usr/apps/org.tizen.browser/res/locale");
  ReloadCorrectPakForLocale();
}
}  // namespace

void Initialize() {
  base::FilePath locale_dir;
  PathService::Get(PathsEfl::DIR_LOCALE, &locale_dir);
  bindtextdomain("WebKit", locale_dir.value().c_str());

  PlatformLanguageChanged(0, 0);
#if defined(OS_TIZEN)
  vconf_notify_key_changed(VCONFKEY_LANGSET, PlatformLanguageChanged, 0);
#endif
}
}  // namespace LocaleEfl
