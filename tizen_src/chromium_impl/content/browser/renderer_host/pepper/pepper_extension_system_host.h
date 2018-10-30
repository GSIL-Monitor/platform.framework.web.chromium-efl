// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RENDERER_HOST_PEPPER_PEPPER_EXTENSION_SYSTEM_HOST_H_
#define CONTENT_BROWSER_RENDERER_HOST_PEPPER_PEPPER_EXTENSION_SYSTEM_HOST_H_

#include <functional>
#include <string>
#include <tuple>
#include <unordered_map>

#include "base/memory/weak_ptr.h"
#include "content/public/browser/extension_system_delegate.h"
#include "ppapi/c/pp_instance.h"
#include "ppapi/c/pp_var.h"
#include "ppapi/host/host_message_context.h"
#include "ppapi/host/resource_host.h"
#include "ppapi/proxy/serialized_var.h"

namespace content {

class BrowserPpapiHost;

class PepperExtensionSystemHost
    : public ppapi::host::ResourceHost,
      public base::SupportsWeakPtr<PepperExtensionSystemHost> {
 public:
  PepperExtensionSystemHost(BrowserPpapiHost* host,
                            PP_Instance instance,
                            PP_Resource resource);
  ~PepperExtensionSystemHost() override;

 protected:
  // ppapi::host::ResourceHost override.
  int32_t OnResourceMessageReceived(
      const IPC::Message& msg,
      ppapi::host::HostMessageContext* context) override;

 private:
  int32_t OnHostMsgGenericSyncCall(ppapi::host::HostMessageContext* context,
                                   const std::string& operation_name,
                                   const std::string& operation_data);
  int32_t OnHostMsgGetEmbedderName(ppapi::host::HostMessageContext* context);
  int32_t OnHostMsgGetCurrentExtensionInfo(
      ppapi::host::HostMessageContext* context);

  void DidGetEmbedderName(ppapi::host::ReplyMessageContext,
                          std::tuple<bool, std::string> embedder_name);
  void DidGetExtensionInfo(ppapi::host::ReplyMessageContext,
                           std::tuple<bool, std::string> serialized_data);
  void DidHandledGenericSyncCall(
      ppapi::host::ReplyMessageContext,
      std::tuple<bool, std::string> operation_result);

  std::unique_ptr<base::Value> TryHandleInternally(
      const std::string& operation_name,
      const base::Value& data,
      bool* was_handled);
  std::unique_ptr<base::Value> AddDynamicCertificatePath(
      const base::Value& data);
  std::unique_ptr<base::Value> GetWindowId(const base::Value& data);
  std::unique_ptr<base::Value> CheckPrivilege(const base::Value& data);
#if defined(OS_TIZEN_TV_PRODUCT)
  std::unique_ptr<base::Value> SetIMERecommendedWords(const base::Value& data);
  std::unique_ptr<base::Value> SetIMERecommendedWordsType(
      const base::Value& data);
#endif

  using FunctionHandler =
      std::function<std::unique_ptr<base::Value>(const base::Value&)>;
  std::unordered_map<std::string, FunctionHandler> extension_function_handlers_;

  ExtensionSystemDelegateManager::RenderFrameID render_frame_id_;
};

}  // namespace content

#endif  // CONTENT_BROWSER_RENDERER_HOST_PEPPER_PEPPER_EXTENSION_SYSTEM_HOST_H_
