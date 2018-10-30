// Copyright 2018 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ewk_app_control_private.h"

#include "base/logging.h"
#include "base/strings/string_split.h"
#include "base/strings/string_tokenizer.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "components/navigation_interception/navigation_params.h"
#include "eweb_view.h"
#include "url/url_util.h"

const char kAppControlScheme[] = "appcontrol";
const char kAppControlStart[] = "#AppControl";
const char kAppControlEnd[] = "end";
const char kOperation[] = "operation=";
const char kUri[] = "uri=";
const char kMime[] = "mime=";
const char kAppID[] = "appid=";
const char kExtraData[] = "e.";
const char kLaunchMode[] = "launch_mode=";
const char kLaunchModeGroup[] = "group";
const char kFallbackUrl[] = "fallback_url";
const char kDefaultCategory[] = "http://tizen.org/category/browsableapp";

#if defined(OS_TIZEN)
static void AppControlResultCb(app_control_h request,
                               app_control_h reply,
                               app_control_result_e result,
                               void* data) {
  auto* app_control = static_cast<_Ewk_App_Control*>(data);

  if (result == APP_CONTROL_RESULT_APP_STARTED ||
      result == APP_CONTROL_RESULT_SUCCEEDED) {
    LOG(INFO) << __FUNCTION__ << " > Success ";
  } else {
    LOG(ERROR) << __FUNCTION__
               << " > Fail or cancel app control. ERR: " << result;
    app_control->FailHandling();
  }
}
#endif

_Ewk_App_Control::_Ewk_App_Control(EWebView* webview,
                                   std::string intercepted_url)
    : webview_(webview),
#if defined(OS_TIZEN)
      app_control_(nullptr),
#endif
      intercepted_url_(intercepted_url),
      explicit_category(false) {
}

_Ewk_App_Control::~_Ewk_App_Control() {
#if defined(OS_TIZEN)
  if (app_control_)
    app_control_destroy(app_control_);
#endif
}

bool _Ewk_App_Control::Proceed() {
#if defined(OS_TIZEN)
  if (ParseNavigationParams() && MakeAppControl()) {
    // Launch request
    int ret = app_control_send_launch_request(app_control_, AppControlResultCb,
                                              reinterpret_cast<void*>(this));
    if (ret != APP_CONTROL_ERROR_NONE) {
      LOG(ERROR) << __FUNCTION__
                 << " > Fail to send launch request. ERR: " << ret;
      return FailHandling();
    }
    return true;
  }
#endif
  return false;
}

bool _Ewk_App_Control::FailHandling() {
  if (!webview_) {
    LOG(INFO) << __FUNCTION__ << " > webview object is null";
    return false;
  }

  if (!fallback_url_.empty()) {
    // Launch fallback_url!
    LOG(INFO) << __FUNCTION__ << " > fallback url : " << fallback_url_.c_str();
    webview_->SetURL(GURL(fallback_url_));
    return true;
  }

  // TODO(djmix.kim) : Connect to Tizen app store
  LOG(INFO) << __FUNCTION__ << " > Not implemented : connect to app store";
  return false;
}

bool _Ewk_App_Control::ParseNavigationParams() {
  // check start : "#AppControl"
  base::StringTokenizer str_tok(intercepted_url_, ";");
  if (!str_tok.GetNext() || str_tok.token() != kAppControlStart) {
    LOG(ERROR) << __FUNCTION__ << " > #AppControl is missing! ";
    return false;
  }

  bool check_end = false;
  bool check_operation = false;
  bool check_uri = false;
  bool check_mime = false;
  bool check_app_id = false;
  bool check_launch_mode = false;
  while (str_tok.GetNext()) {
    const std::string& token = str_tok.token();

    // check end : "end"
    if (token == kAppControlEnd) {
      check_end = true;
      break;
    }

    std::string buf = GetValue(token);
    if (base::StartsWith(token, kOperation,
                         base::CompareCase::INSENSITIVE_ASCII)) {
      if (check_operation) {
        LOG(ERROR) << __FUNCTION__ << " > Invalid operation";
        return false;
      }
      operation_ = DecodeUrl(buf);
      check_operation = true;
    } else if (base::StartsWith(token, kUri,
                                base::CompareCase::INSENSITIVE_ASCII)) {
      if (check_uri) {
        LOG(ERROR) << __FUNCTION__ << " > Invalid uri";
        return false;
      }
      uri_ = DecodeUrl(buf);
      check_uri = true;
    } else if (base::StartsWith(token, kMime,
                                base::CompareCase::INSENSITIVE_ASCII)) {
      if (check_mime) {
        LOG(ERROR) << __FUNCTION__ << " > Invalid mime";
        return false;
      }
      mime_ = DecodeUrl(buf);
      check_mime = true;
    } else if (base::StartsWith(token, kAppID,
                                base::CompareCase::INSENSITIVE_ASCII)) {
      if (check_app_id) {
        LOG(ERROR) << __FUNCTION__ << " > Invalid app id";
        return false;
      }
      app_id_ = buf;
      check_app_id = true;
    } else if (base::StartsWith(token, kLaunchMode,
                                base::CompareCase::INSENSITIVE_ASCII)) {
      if (check_launch_mode) {
        LOG(ERROR) << __FUNCTION__ << " > Invalid launch mode";
        return false;
      }
      launch_mode_ = buf;
      check_launch_mode = true;
    } else if (base::StartsWith(token, kExtraData,
                                base::CompareCase::INSENSITIVE_ASCII)) {
      SetExtraData(token);
    }
  }

  if (!check_end) {
    LOG(ERROR) << __FUNCTION__ << " > end; is missing! ";
    return false;
  }

  ShowParseNavigationParams();
  return true;
}

std::string _Ewk_App_Control::GetValue(std::string token) {
  std::string full_str;
  std::vector<std::string> values = base::SplitString(
      token, "=", base::KEEP_WHITESPACE, base::SPLIT_WANT_ALL);

  if (values.size() <= 1)
    return std::string("");

  for (int i = 1; i < values.size(); i++) {
    if (i > 1)
      full_str += "=";
    full_str += values[i];
  }

  return full_str;
}

void _Ewk_App_Control::SetExtraData(std::string token) {
  std::string full_str;
  std::vector<std::string> values = base::SplitString(
      token, "=", base::KEEP_WHITESPACE, base::SPLIT_WANT_ALL);

  if (values.size() > 0) {
    for (int i = 1; i < values.size(); i++) {
      if (i > 1)
        full_str += "=";
      full_str += values[i];
    }

    std::vector<std::string> keys = base::SplitString(
        values[0], ".", base::KEEP_WHITESPACE, base::SPLIT_WANT_ALL);

    if (keys.size() == 1)
      return;

    if (keys[1] == kFallbackUrl)
      fallback_url_ = DecodeUrl(full_str);
    else
      extra_data_[keys[1]] = full_str;
  }
}

bool _Ewk_App_Control::MakeAppControl() {
#if defined(OS_TIZEN)
  if (app_control_)
    app_control_destroy(app_control_);

  int ret = app_control_create(&app_control_);
  if (ret != APP_CONTROL_ERROR_NONE) {
    LOG(ERROR) << __FUNCTION__ << " > app_control_create is failed with err "
               << ret;
    return false;
  }

  // Set category to http://tizen.org/category/browsableapp as default.
  ret = app_control_set_category(app_control_, kDefaultCategory);
  if (ret != APP_CONTROL_ERROR_NONE) {
    LOG(ERROR) << __FUNCTION__
               << " > app_control_set_category is failed with err " << ret;
    return FailHandling();
  }

  // Check app_id
  if (!app_id_.empty()) {  // explicit launch
    explicit_category = false;
    ret = app_control_set_app_id(app_control_, app_id_.c_str());
    if (ret != APP_CONTROL_ERROR_NONE) {
      LOG(ERROR) << __FUNCTION__
                 << " > app_control_set_app_id is failed with err " << ret;
      return FailHandling();
    }

    pkgmgrinfo_appinfo_h handle;
    ret = pkgmgrinfo_appinfo_get_appinfo(app_id_.c_str(), &handle);
    if (ret != PMINFO_R_OK) {
      LOG(ERROR) << __FUNCTION__
                 << " > pkgmgrinfo_appinfo_get_appinfo failed!!! : " << ret;
      return FailHandling();
    }

    ret = pkgmgrinfo_appinfo_foreach_category(
        handle,
        [](const char* category, void* user_data) -> int {
          auto app_control = static_cast<_Ewk_App_Control*>(user_data);
          if ((nullptr != category) &&
              std::string(category) == kDefaultCategory) {
            app_control->explicit_category = true;
          }
          return 0;
        },
        this);
    if (ret != PMINFO_R_OK) {
      LOG(ERROR) << __FUNCTION__
                 << " > pkgmgrinfo_appinfo_foreach_category failed!!! : "
                 << ret;
      return FailHandling();
    }

    if (!explicit_category) {
      LOG(ERROR) << __FUNCTION__
                 << "> Explicit launch : This is not a browsable app";
      return FailHandling();
    }

    pkgmgrinfo_appinfo_destroy_appinfo(handle);
  }

  // operation
  if (!operation_.empty()) {
    ret = app_control_set_operation(app_control_, operation_.c_str());
    if (ret != APP_CONTROL_ERROR_NONE) {
      LOG(ERROR) << __FUNCTION__
                 << " > app_control_set_operation is failed with err " << ret;
      return FailHandling();
    }
  }

  // uri
  if (!uri_.empty()) {
    ret = app_control_set_uri(app_control_, uri_.c_str());
    if (ret != APP_CONTROL_ERROR_NONE) {
      LOG(ERROR) << __FUNCTION__ << " > app_control_set_uri is failed with err "
                 << ret;
      return FailHandling();
    }
  }

  // mime
  if (!mime_.empty()) {
    ret = app_control_set_mime(app_control_, mime_.c_str());
    if (ret != APP_CONTROL_ERROR_NONE) {
      LOG(ERROR) << __FUNCTION__
                 << " > app_control_set_mime is failed with err " << ret;
      return FailHandling();
    }
  }

  // launch_mode
  if (!launch_mode_.empty()) {
    ret = app_control_set_launch_mode(app_control_, GetLaunchMode());
    if (ret != APP_CONTROL_ERROR_NONE) {
      LOG(ERROR) << __FUNCTION__
                 << " > app_control_set_launch_mode is failed with err " << ret;
      return FailHandling();
    }
  }

  // extra_data
  if (!extra_data_.empty()) {
    std::map<std::string, std::string>::iterator it;
    for (it = extra_data_.begin(); it != extra_data_.end(); it++) {
      app_control_add_extra_data(app_control_, it->first.c_str(),
                                 it->second.c_str());
    }
  }

  ret = app_control_enable_app_started_result_event(app_control_);
  if (ret != APP_CONTROL_ERROR_NONE) {
    LOG(ERROR)
        << __FUNCTION__
        << " > app_control_enable_app_started_result_event is failed with err "
        << ret;
    return FailHandling();
  }

  return true;
#else
  return false;
#endif
}

void _Ewk_App_Control::ShowParseNavigationParams() {
  LOG(INFO) << __FUNCTION__;
  LOG(INFO) << "operation : " << GetOperation();
  LOG(INFO) << "uri : " << GetUri();
  LOG(INFO) << "mime : " << GetMime();
  LOG(INFO) << "appid : " << GetAppId();
#if defined(OS_TIZEN)
  LOG(INFO) << "launch_mode : " << GetLaunchMode();
#endif
  LOG(INFO) << "fallback_url : " << fallback_url_.c_str();

  std::map<std::string, std::string>::iterator it;
  for (it = extra_data_.begin(); it != extra_data_.end(); it++) {
    LOG(INFO) << "extra_data : key=" << it->first.c_str()
              << " value=" << it->second.c_str();
  }
}

const char* _Ewk_App_Control::GetOperation() const {
  return operation_.c_str();
}

const char* _Ewk_App_Control::GetUri() const {
  return uri_.c_str();
}

const char* _Ewk_App_Control::GetMime() const {
  return mime_.c_str();
}

const char* _Ewk_App_Control::GetAppId() const {
  return app_id_.c_str();
}

#if defined(OS_TIZEN)
app_control_launch_mode_e _Ewk_App_Control::GetLaunchMode() const {
  return (launch_mode_ == kLaunchModeGroup) ? APP_CONTROL_LAUNCH_MODE_GROUP
                                            : APP_CONTROL_LAUNCH_MODE_SINGLE;
}
#endif

std::string _Ewk_App_Control::DecodeUrl(std::string url) {
  std::string output;
  url::RawCanonOutputT<base::char16> decode;
  url::DecodeURLEscapeSequences(url.data(), url.size(), &decode);
  base::UTF16ToUTF8(decode.data(), decode.length(), &output);
  return output;
}
