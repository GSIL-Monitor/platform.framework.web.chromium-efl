// Copyright 2018 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ewk_app_control_private_h
#define ewk_app_control_private_h

#include <map>
#include <string>

#if defined(OS_TIZEN)
#include <app_control_internal.h>
#include <app_manager.h>
#include <pkgmgr-info.h>
#endif

extern const char kAppControlScheme[];
extern const char kAppControlStart[];
extern const char kAppControlEnd[];
extern const char kOperation[];
extern const char kUri[];
extern const char kMime[];
extern const char kAppID[];
extern const char kExtraData[];
extern const char kLaunchMode[];
extern const char kLaunchModeGroup[];
extern const char kFallbackUrl[];
extern const char kDefaultOperation[];

class EWebView;
namespace navigation_interception {
class NavigationParams;
}

class _Ewk_App_Control {
 public:
  explicit _Ewk_App_Control(EWebView* webview, std::string intercepted_url);
  ~_Ewk_App_Control();

  bool Proceed();
  const char* GetOperation() const;
  const char* GetUri() const;
  const char* GetMime() const;
  const char* GetAppId() const;
#if defined(OS_TIZEN)
  app_control_launch_mode_e GetLaunchMode() const;
#endif
  bool FailHandling();

 private:
  bool MakeAppControl();
  bool ParseNavigationParams();
  std::string GetValue(std::string token);
  void SetExtraData(std::string token);
  void ShowParseNavigationParams();
  void CheckResultCb();
  std::string DecodeUrl(std::string url);

  EWebView* webview_;
#if defined(OS_TIZEN)
  app_control_h app_control_;
#endif
  std::string intercepted_url_;
  std::string operation_;
  std::string uri_;
  std::string mime_;
  std::string app_id_;
  std::string launch_mode_;
  std::string fallback_url_;
  bool explicit_category;
  std::map<std::string, std::string> extra_data_;
};

#endif  // ewk_app_control_private_h
