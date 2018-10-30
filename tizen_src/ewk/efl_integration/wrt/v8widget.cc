// Copyright 2014-2018,  Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "wrt/v8widget.h"

#include "base/logging.h"
#include "wrt/dynamicplugin.h"
#include "wrt/wrtwidget.h"

#if defined(OS_TIZEN_TV_PRODUCT)
#include "wrt/hbbtv_dynamicplugin.h"
#include "wrt/hbbtv_widget.h"
#endif
#include "wrt/wrt_dynamicplugin.h"

bool V8Widget::ParseUrl(const GURL& url,
                        GURL& new_url,
                        bool* is_encrypted_file) {
  if (id_.empty())
    return false;

  std::string old_url = url.possibly_invalid_spec();
  std::string str_new_url;
  if (type_ == V8Widget::Type::WRT)
    WrtDynamicPlugin::Get().ParseURL(&old_url, &str_new_url, id_.c_str(),
                                     is_encrypted_file);

#if defined(OS_TIZEN_TV_PRODUCT)
  if (type_ == V8Widget::Type::HBBTV)
    HbbtvDynamicPlugin::Get().ParseURL(&old_url, &str_new_url, id_.c_str(),
                                       is_encrypted_file);
#endif

  if (!str_new_url.empty()) {
    new_url = GURL(str_new_url);
    return true;
  }

  return false;
}

#if defined(OS_TIZEN_TV_PRODUCT)
bool V8Widget::GetFileDecryptedDataBuffer(const GURL& url,
                                          std::vector<char>* data) {
  if (id_.empty())
    return false;

  std::string str_url = url.possibly_invalid_spec();
  if (type_ == V8Widget::Type::WRT)
    return WrtDynamicPlugin::Get().GetFileDecryptedDataBuffer(&str_url, data);

  if (type_ == V8Widget::Type::HBBTV)
    return HbbtvDynamicPlugin::Get().GetFileDecryptedDataBuffer(&str_url, data);
  return false;
}
#endif

// static
V8Widget* V8Widget::CreateWidget(Type type,
                                 const base::CommandLine& command_line) {
#if defined(OS_TIZEN_TV_PRODUCT)
  if (type == Type::HBBTV)
    return new HbbtvWidget(command_line);
#endif
  return new WrtWidget(command_line);
}

void V8Widget::StartSession(v8::Handle<v8::Context> context,
                            int routing_handle,
                            const char* session_blob) {
  if (plugin_ && !id_.empty() && !context.IsEmpty())
    plugin_->StartSession(id_.c_str(), context, routing_handle, session_blob);
}

void V8Widget::StopSession(v8::Handle<v8::Context> context) {
  if (plugin_ && !id_.empty() && !context.IsEmpty())
    plugin_->StopSession(id_.c_str(), context);
}
