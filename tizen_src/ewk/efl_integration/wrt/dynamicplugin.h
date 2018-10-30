// Copyright 2014, 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EWK_EFL_INTEGRATION_WRT_DYNAMICPLUGIN_H_
#define EWK_EFL_INTEGRATION_WRT_DYNAMICPLUGIN_H_

#include <string>
#include "base/macros.h"
#include "v8/include/v8.h"
#include "wrt/v8widget.h"

typedef void (*StartSessionFunction)(const char* session_id,
                                     v8::Handle<v8::Context> context,
                                     int routing_handle,
                                     const void* session_blob);

typedef void (*StopSessionFunction)(const char* session_id,
                                    v8::Handle<v8::Context> context);

class DynamicPlugin {
 public:
  virtual ~DynamicPlugin();

  virtual bool Init(const std::string& injectedBundlePath);
  virtual bool InitRenderer(const std::string& injected_bundle_path);

  // Interface for WebApp URL Conversion
  /* LCOV_EXCL_START */
  virtual void SetWidgetInfo(const std::string& tizen_app_id) {}
  virtual bool CanHandleParseUrl(const std::string& scheme) const {
    return false;
  }
  /* LCOV_EXCL_STOP */
  virtual void ParseURL(std::string* old_url,
                        std::string* new_url,
                        const char* tizen_app_id,
                        bool* is_encrypted_file = nullptr) = 0;
#if defined(OS_TIZEN_TV_PRODUCT)
  virtual bool GetFileDecryptedDataBuffer(const std::string* url,
                                          std::vector<char>* data) = 0;
#endif

  void StartSession(const char* session_id,
                    v8::Handle<v8::Context> context,
                    int routing_handle,
                    const void* session_blob,
                    double scale_factor = 1.0f,
                    const char* encoded_bundle = nullptr,
                    const char* theme = nullptr) const;
  void StopSession(const char* session_id,
                   v8::Handle<v8::Context> context) const;
  void* GetFunction(const char* funcation_name) const;
  void* handle() const { return handle_; }

  static DynamicPlugin& Get(V8Widget::Type type);

 protected:
  DynamicPlugin();

 private:
  void* handle_;
  unsigned int version_;
  StartSessionFunction start_session_;
  StopSessionFunction stop_session_;
};

#endif  // EWK_EFL_INTEGRATION_WRT_DYNAMICPLUGIN_H_
