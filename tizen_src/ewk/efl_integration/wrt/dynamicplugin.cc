// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "wrt/dynamicplugin.h"

#include <dlfcn.h>

#include "base/logging.h"
#include "wrt/wrt_dynamicplugin.h"
#if defined(OS_TIZEN_TV_PRODUCT)
#include "wrt/hbbtv_dynamicplugin.h"
#endif

namespace {
const char* const kStartSessionFunction = "DynamicPluginStartSession";
const char* const kStopSessionFunction = "DynamicPluginStopSession";
const char* const kVersionFunction = "DynamicPluginVersion";

typedef unsigned int (*VersionFunction)(void);
typedef void (*StartSessionFun_v0)(const char* tizen_app_id,
                                   v8::Handle<v8::Context> context,
                                   int routing_handle,
                                   double scale_factor,
                                   const char* encoded_bundle,
                                   const char* theme,
                                   const char* baseURL);
}

DynamicPlugin::DynamicPlugin()
    : handle_(0), version_(0), start_session_(0), stop_session_(0) {}

bool DynamicPlugin::Init(const std::string& injected_bundle_path) {
  if (handle_ || injected_bundle_path.empty())
    return handle_ != nullptr;
  handle_ = dlopen(injected_bundle_path.c_str(), RTLD_LAZY);
  if (!handle_)
    LOG(ERROR) << "No handle to " << injected_bundle_path << " " << dlerror();
  return handle_ != nullptr;
}

bool DynamicPlugin::InitRenderer(const std::string& injected_bundle_path) {
  if (!handle_) {
    if (!Init(injected_bundle_path))
      return false;
  }

  VersionFunction version_function =
      reinterpret_cast<VersionFunction>(GetFunction(kVersionFunction));
  if (version_function) {
    version_ = version_function();
    if (version_ != 0 && version_ != 1) {
      LOG(ERROR) << "Unknown plugin version: " << version_ << "!\n";
      return false;
    }
  }

  start_session_ = reinterpret_cast<StartSessionFunction>(
      GetFunction(kStartSessionFunction));
  stop_session_ =
      reinterpret_cast<StopSessionFunction>(GetFunction(kStopSessionFunction));
  return start_session_ && stop_session_;
}

void DynamicPlugin::StartSession(const char* session_id,
                                 v8::Handle<v8::Context> context,
                                 int routing_handle,
                                 const void* session_blob,
                                 double scale_factor,
                                 const char* encoded_bundle,
                                 const char* theme) const {
  if (!start_session_)
    return;

  switch (version_) {
    case 0: {
      auto startSession_v0 =
          reinterpret_cast<StartSessionFun_v0>(start_session_);
      startSession_v0(session_id, context, routing_handle, scale_factor,
                      encoded_bundle, theme,
                      reinterpret_cast<const char*>(session_blob));
      break;
    }
    case 1: {
      start_session_(session_id, context, routing_handle, session_blob);
      break;
    }
    default:
      return;
  }
}

void DynamicPlugin::StopSession(const char* session_id,
                                v8::Handle<v8::Context> context) const {
  if (!stop_session_)
    return;

  stop_session_(session_id, context);
}

void* DynamicPlugin::GetFunction(const char* funcation_name) const {
  if (!handle_)
    return nullptr;

  void* function_addr = dlsym(handle_, funcation_name);
  if (!function_addr)
    LOG(ERROR) << "No " << funcation_name << " symbol found! " << dlerror();
  return function_addr;
}

DynamicPlugin::~DynamicPlugin() {
  if (handle_)
    dlclose(handle_);
}

// static
DynamicPlugin& DynamicPlugin::Get(V8Widget::Type type) {
#if defined(OS_TIZEN_TV_PRODUCT)
  if (type == V8Widget::Type::HBBTV)
    return HbbtvDynamicPlugin::Get();
#endif
  DCHECK_EQ(type, V8Widget::Type::WRT);
  return WrtDynamicPlugin::Get();
}
