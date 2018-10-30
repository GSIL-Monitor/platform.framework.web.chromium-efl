// Copyright 2018 Samsung Electronics. All rights resemicPlugin::ParseURL
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "wrt/hbbtv_dynamicplugin.h"

namespace {
const char* const kHbbtvUrlParsingFunction = "DynamicHbbTVUrlParsing";
const char* const kHbbtvGetFileDecryptedDataBuffer =
    "DynamicHbbTVGetFileDecryptedDataBuffer";
}  // namespace

HbbtvDynamicPlugin::HbbtvDynamicPlugin()
    : DynamicPlugin(),
      parse_hbbtv_url_(nullptr),
      get_file_decrypted_data_buffer_(nullptr) {}

bool HbbtvDynamicPlugin::Init(const std::string& injected_bundle_path) {
  if (!DynamicPlugin::Init(injected_bundle_path))
    return false;
  parse_hbbtv_url_ =
      reinterpret_cast<HbbtvParseUrlFun>(GetFunction(kHbbtvUrlParsingFunction));
  get_file_decrypted_data_buffer_ =
      reinterpret_cast<HbbtvGetFileDecryptedDataBufferFun>(
          GetFunction(kHbbtvGetFileDecryptedDataBuffer));
  return parse_hbbtv_url_ && get_file_decrypted_data_buffer_;
}

void HbbtvDynamicPlugin::ParseURL(std::string* old_url,
                                  std::string* new_url,
                                  const char* tizen_app_id,
                                  bool* is_encrypted_file) {
  if (!parse_hbbtv_url_)
    return;

  parse_hbbtv_url_(old_url, new_url, tizen_app_id, is_encrypted_file);
}

bool HbbtvDynamicPlugin::GetFileDecryptedDataBuffer(const std::string* url,
                                                    std::vector<char>* data) {
  if (!get_file_decrypted_data_buffer_)
    return false;

  return get_file_decrypted_data_buffer_(url, data);
}

HbbtvDynamicPlugin::~HbbtvDynamicPlugin() {}

HbbtvDynamicPlugin& HbbtvDynamicPlugin::Get() {
  static HbbtvDynamicPlugin dynamicPlugin;
  return dynamicPlugin;
}
