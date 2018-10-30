// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EWK_EFL_INTEGRATION_WRT_WRT_DYNAMICPLUGIN_H_
#define EWK_EFL_INTEGRATION_WRT_WRT_DYNAMICPLUGIN_H_

#include <string>
#include <vector>
#include "v8/include/v8.h"
#include "wrt/dynamicplugin.h"

struct Ewk_Wrt_Message_Data;

typedef void (*OnIPCMessageFun)(const Ewk_Wrt_Message_Data& data);
#if !defined(OS_TIZEN_TV_PRODUCT)
typedef void (*ParseUrlFun)(std::string* old_url,
                            std::string* new_url,
                            const char* tizen_app_id);
#else
typedef void (*ParseUrlFun)(std::string* old_url,
                            std::string* new_url,
                            const char* tizen_app_id,
                            bool* is_encrypted_file);
typedef bool (*GetFileDecryptedDataBufferFun)(const std::string* url,
                                              std::vector<char>* data);
#endif
typedef void (*SetWidgetInfoFun)(const char* tizen_app_id);
typedef void (*DatabaseAttachFun)(int database_attach);

class WrtDynamicPlugin : public DynamicPlugin {
 public:
  // For Browser and Renderer
  bool Init(const std::string& injected_bundle_path) override;
  // Only for Renderer
  bool InitRenderer(const std::string& injected_bundle_path) override;

  void StartSession(const char* tizen_app_id,
                    v8::Handle<v8::Context> context,
                    int routing_handle,
                    const char* base_url,
                    double scale_factor,
                    const char* encoded_bundle,
                    const char* theme);
  void StopSession(const char* tizen_app_id, v8::Handle<v8::Context> context);

  void MessageReceived(const Ewk_Wrt_Message_Data& data);

  // TODO(is46.kim) : |WrtDynamicPlugin| was designed to run on the renderer
  // with the injected bundle of xwalk. But currently, the implementation is
  // changed to call from WrtUrlRequestInterceptor in browser IO Thread. File
  // scheme conversion implementations should be separated only to browser side.
  void SetWidgetInfo(const std::string& tizen_app_id) override;
  bool CanHandleParseUrl(const std::string& scheme) const override;
  void ParseURL(std::string* old_url,
                std::string* new_url,
                const char* tizen_app_id,
                bool* is_encrypted_file = nullptr) override;
#if defined(OS_TIZEN_TV_PRODUCT)
  bool GetFileDecryptedDataBuffer(const std::string* url,
                                  std::vector<char>* data) override;
#endif

  static WrtDynamicPlugin& Get();
  ~WrtDynamicPlugin() override;

 private:
  WrtDynamicPlugin();

  SetWidgetInfoFun set_widget_info_;
  DatabaseAttachFun database_attach_;
  ParseUrlFun url_parser_;
#if defined(OS_TIZEN_TV_PRODUCT)
  GetFileDecryptedDataBufferFun get_file_decrypted_data_buffer_;
#endif
  OnIPCMessageFun on_IPC_message_;
  bool widget_info_set_;

  DISALLOW_COPY_AND_ASSIGN(WrtDynamicPlugin);
};

#endif  // EWK_EFL_INTEGRATION_WRT_WRT_DYNAMICPLUGIN_H_
