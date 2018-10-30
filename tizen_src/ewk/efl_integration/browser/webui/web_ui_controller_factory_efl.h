// Copyright (c) 2018 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BROWSER_WEBUI_WEB_UI_CONTROLLER_FACTORY_EFL_H_
#define BROWSER_WEBUI_WEB_UI_CONTROLLER_FACTORY_EFL_H_

#include "base/memory/singleton.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_controller_factory.h"

class WebUIControllerFactoryEfl : public content::WebUIControllerFactory {
 public:
  content::WebUI::TypeID GetWebUIType(content::BrowserContext* browser_context,
                                      const GURL& url) const override;
  bool UseWebUIForURL(content::BrowserContext* browser_context,
                      const GURL& url) const override;
  bool UseWebUIBindingsForURL(content::BrowserContext* browser_context,
                              const GURL& url) const override;
  content::WebUIController* CreateWebUIControllerForURL(
      content::WebUI* web_ui,
      const GURL& url) const override;

  static WebUIControllerFactoryEfl* GetInstance();

 protected:
  WebUIControllerFactoryEfl();
  ~WebUIControllerFactoryEfl() override;

 private:
  friend struct base::DefaultSingletonTraits<WebUIControllerFactoryEfl>;

  DISALLOW_COPY_AND_ASSIGN(WebUIControllerFactoryEfl);
};

#endif  // BROWSER_WEBUI_WEB_UI_CONTROLLER_FACTORY_EFL_H_