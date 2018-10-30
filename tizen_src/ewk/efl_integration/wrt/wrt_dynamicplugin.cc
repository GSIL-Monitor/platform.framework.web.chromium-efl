// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "wrt/wrt_dynamicplugin.h"

#include <dlfcn.h>

#include "base/command_line.h"
#include "base/logging.h"
#include "common/content_switches_efl.h"

namespace {
const char* const kSetWidgetInfoFunction = "DynamicSetWidgetInfo";
const char* const kDatabaseAttachFunction = "DynamicDatabaseAttach";
const char* const kOnIpcMessageFunction = "DynamicOnIPCMessage";
#if !defined(OS_TIZEN_TV_PRODUCT)
const char* const kUrlParsingFunction = "DynamicUrlParsing";
#else
const char* const kUrlParsingFunction = "DynamicTVUrlParsing";
const char* const kGetFileDecryptedDataBufferFunction =
    "DynamicGetFileDecryptedDataBuffer";
#endif
}  // namespace

WrtDynamicPlugin::WrtDynamicPlugin()
    : DynamicPlugin(),
      set_widget_info_(0),
      database_attach_(0),
      url_parser_(0),
#if defined(OS_TIZEN_TV_PRODUCT)
      get_file_decrypted_data_buffer_(0),
#endif
      on_IPC_message_(0),
      widget_info_set_(false) {
}

bool WrtDynamicPlugin::Init(const std::string& injected_bundle_path) {
  if (!DynamicPlugin::Init(injected_bundle_path))
    return false;
  set_widget_info_ =
      reinterpret_cast<SetWidgetInfoFun>(GetFunction(kSetWidgetInfoFunction));
  url_parser_ = reinterpret_cast<ParseUrlFun>(GetFunction(kUrlParsingFunction));
#if defined(OS_TIZEN_TV_PRODUCT)
  get_file_decrypted_data_buffer_ =
      reinterpret_cast<GetFileDecryptedDataBufferFun>(
          GetFunction(kGetFileDecryptedDataBufferFunction));
  if (!get_file_decrypted_data_buffer_)
    return false;
#endif
  return set_widget_info_ && url_parser_;
}

bool WrtDynamicPlugin::InitRenderer(const std::string& injected_bundle_path) {
  if (!DynamicPlugin::InitRenderer(injected_bundle_path))
    return false;
  database_attach_ =
      reinterpret_cast<DatabaseAttachFun>(GetFunction(kDatabaseAttachFunction));
  if (!database_attach_)
    return false;

  on_IPC_message_ =
      reinterpret_cast<OnIPCMessageFun>(GetFunction(kOnIpcMessageFunction));

  database_attach_(1);
  return on_IPC_message_ && database_attach_;
}

void WrtDynamicPlugin::StartSession(const char* tizen_app_id,
                                    v8::Handle<v8::Context> context,
                                    int routing_handle,
                                    const char* base_url,
                                    double scale_factor,
                                    const char* encoded_bundle,
                                    const char* theme) {
  if (!database_attach_)
    return;
  DynamicPlugin::StartSession(tizen_app_id, context, routing_handle, base_url,
                              scale_factor, encoded_bundle, theme);
}

void WrtDynamicPlugin::StopSession(const char* tizen_app_id,
                                   v8::Handle<v8::Context> context) {
  if (!database_attach_)
    return;

  DynamicPlugin::StopSession(tizen_app_id, context);
}

bool WrtDynamicPlugin::CanHandleParseUrl(const std::string& scheme) const {
  // xwalk handles only file and app scheme.
  if (scheme == url::kFileScheme || scheme == "app")
    return true;
  return false;
}

void WrtDynamicPlugin::ParseURL(std::string* old_url,
                                std::string* new_url,
                                const char* tizen_app_id,
                                bool* is_encrypted_file) {
  if (!set_widget_info_)
    return;

  if (!widget_info_set_) {
    // When a web app is launched for first time after reboot, SetWidgetInfo is
    // called later than ParseURL as RenderThread is not yet ready due to webkit
    // initialization taking more time. WRT expects tizen app id to be set
    // before requesting for URL parsing. So we manually set widget info for
    // first time.
    SetWidgetInfo(tizen_app_id);
  }

  if (url_parser_) {
#if !defined(OS_TIZEN_TV_PRODUCT)
    url_parser_(old_url, new_url, tizen_app_id);
#else
    url_parser_(old_url, new_url, tizen_app_id, is_encrypted_file);
#endif
  }
}

#if defined(OS_TIZEN_TV_PRODUCT)
bool WrtDynamicPlugin::GetFileDecryptedDataBuffer(const std::string* url,
                                                  std::vector<char>* data) {
  if (!get_file_decrypted_data_buffer_)
    return false;

  return get_file_decrypted_data_buffer_(url, data);
}
#endif

void WrtDynamicPlugin::SetWidgetInfo(const std::string& tizen_app_id) {
  if (!set_widget_info_)
    return;

  if (widget_info_set_) {
    LOG(INFO) << "Widget info is already set!";
    return;
  }

  set_widget_info_(tizen_app_id.c_str());
  widget_info_set_ = true;
}

WrtDynamicPlugin::~WrtDynamicPlugin() {
  if (database_attach_)
    database_attach_(0);
}

WrtDynamicPlugin& WrtDynamicPlugin::Get() {
  static WrtDynamicPlugin dynamicPlugin;
  return dynamicPlugin;
}

void WrtDynamicPlugin::MessageReceived(const Ewk_Wrt_Message_Data& data) {
  if (!on_IPC_message_)
    return;

  on_IPC_message_(data);
}
