// Copyright 2018 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EWK_EFL_INTEGRATION_WRT_HBBTV_DYNAMICPLUGIN_H_
#define EWK_EFL_INTEGRATION_WRT_HBBTV_DYNAMICPLUGIN_H_

#include <string>
#include <vector>
#include "wrt/dynamicplugin.h"

typedef void (*HbbtvParseUrlFun)(std::string* old_url,
                                 std::string* new_url,
                                 const char* tizen_app_id,
                                 bool* is_encrypted_file);
typedef bool (*HbbtvGetFileDecryptedDataBufferFun)(const std::string* url,
                                                   std::vector<char>* data);

class HbbtvDynamicPlugin : public DynamicPlugin {
 public:
  bool Init(const std::string& injected_bundle_path) override;
  bool CanHandleParseUrl(const std::string& scheme) const override {
    return true;
  }
  void ParseURL(std::string* old_url,
                std::string* new_url,
                const char* tizen_app_id,
                bool* is_encrypted_file = nullptr) override;

  bool GetFileDecryptedDataBuffer(const std::string* url,
                                  std::vector<char>* data) override;

  static HbbtvDynamicPlugin& Get();
  ~HbbtvDynamicPlugin() override;

 private:
  HbbtvDynamicPlugin();

  HbbtvParseUrlFun parse_hbbtv_url_;
  HbbtvGetFileDecryptedDataBufferFun get_file_decrypted_data_buffer_;

  DISALLOW_COPY_AND_ASSIGN(HbbtvDynamicPlugin);
};

#endif  // EWK_EFL_INTEGRATION_WRT_WRT_DYNAMICPLUGIN_H_
