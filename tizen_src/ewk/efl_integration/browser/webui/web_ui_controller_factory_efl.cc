// Copyright (c) 2018 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser/webui/web_ui_controller_factory_efl.h"

#include "browser/webui/version_ui_efl.h"
#include "common/url_constants_efl.h"
#include "content/public/common/url_constants.h"
#include "url/gurl.h"

using content::BrowserContext;
using content::WebUI;
using content::WebUIController;

namespace {
WebUI::TypeID kChromeUIVersionEflID = &kChromeUIVersionEflID;
}

WebUI::TypeID WebUIControllerFactoryEfl::GetWebUIType(
    BrowserContext* browser_context,
    const GURL& url) const {
  if (!url.SchemeIs(content::kChromeUIScheme))
    return WebUI::kNoWebUI;

  if (url.host() == kChromeUIVersionEflHost)
    return kChromeUIVersionEflID;

  return WebUI::kNoWebUI;
}
bool WebUIControllerFactoryEfl::UseWebUIForURL(
    content::BrowserContext* browser_context,
    const GURL& url) const {
  return GetWebUIType(browser_context, url) != content::WebUI::kNoWebUI;
}

bool WebUIControllerFactoryEfl::UseWebUIBindingsForURL(
    content::BrowserContext* browser_context,
    const GURL& url) const {
  return UseWebUIForURL(browser_context, url);
}

WebUIController* WebUIControllerFactoryEfl::CreateWebUIControllerForURL(
    WebUI* web_ui,
    const GURL& url) const {
  if (!url.SchemeIs(content::kChromeUIScheme))
    return nullptr;

  if (url.host() == kChromeUIVersionEflHost)
    return new VersionUIEfl(web_ui);

  return nullptr;
}

// static
WebUIControllerFactoryEfl* WebUIControllerFactoryEfl::GetInstance() {
  return base::Singleton<WebUIControllerFactoryEfl>::get();
}

WebUIControllerFactoryEfl::WebUIControllerFactoryEfl() {}

WebUIControllerFactoryEfl::~WebUIControllerFactoryEfl() {}