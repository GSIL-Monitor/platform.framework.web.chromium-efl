// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/pepper/pepper_extension_system_host.h"

#include <memory>

#include "base/json/json_string_value_serializer.h"
#include "base/logging.h"
#include "content/public/browser/browser_ppapi_host.h"
#include "content/public/browser/browser_thread.h"
#include "ewk/efl_integration/ewk_privilege_checker.h"
#include "ppapi/cpp/samsung/extension_system_samsung_tizen.h"
#include "ppapi/host/dispatch_host_message.h"
#include "ppapi/host/ppapi_host.h"
#include "ppapi/proxy/plugin_dispatcher.h"
#include "ppapi/proxy/ppapi_messages.h"

#if defined(OS_TIZEN_TV_PRODUCT)
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "ewk/efl_integration/common/web_contents_utils.h"

using web_contents_utils::WebContentsFromFrameID;
#endif

namespace content {

namespace {
const char kAddDynamicCertificatePathName[] = "add_dynamic_certificate_path";
const char kCheckPrivilegeOperationName[] = "check_ace_privilege";
const char kGetWindowIdOperationName[] = "get_window_id";
#if defined(OS_TIZEN_TV_PRODUCT)
const char kSetIMERecommendedWordsName[] = "set_ime_recommended_words";
const char kSetIMERecommendedWordsTypeName[] = "set_ime_recommended_words_type";
#endif

std::tuple<bool, std::string> GetEmbedderName(
    ExtensionSystemDelegateManager::RenderFrameID render_frame_id) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  ExtensionSystemDelegate* delegate =
      ExtensionSystemDelegateManager::GetInstance()->GetDelegateForFrame(
          render_frame_id);
  if (delegate)
    return std::make_tuple(true, delegate->GetEmbedderName());
  return std::make_tuple(false, "");
}

std::tuple<bool, std::string> GetExtensionInfo(
    ExtensionSystemDelegateManager::RenderFrameID render_frame_id) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  ExtensionSystemDelegate* delegate =
      ExtensionSystemDelegateManager::GetInstance()->GetDelegateForFrame(
          render_frame_id);
  std::string result;
  if (delegate) {
    std::unique_ptr<base::Value> res_ptr = delegate->GetExtensionInfo();
    if (!res_ptr)
      return std::make_tuple(false, "");

    JSONStringValueSerializer json_serializer(&result);
    if (!json_serializer.Serialize(*res_ptr))
      return std::make_tuple(false, "");
  }
  return std::make_tuple(true, result);
}

std::tuple<bool, std::string> HandleGenericSyncCall(
    ExtensionSystemDelegateManager::RenderFrameID render_frame_id,
    const std::string operation_name,
    base::Value* raw_pointer_data) {
  std::unique_ptr<base::Value> data(raw_pointer_data);
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  std::string result;
  ExtensionSystemDelegate* delegate =
      ExtensionSystemDelegateManager::GetInstance()->GetDelegateForFrame(
          render_frame_id);
  if (delegate) {
    std::unique_ptr<base::Value> res_ptr =
        delegate->GenericSyncCall(operation_name, *data);
    if (!res_ptr)
      return std::make_tuple(false, "");

    JSONStringValueSerializer json_serializer(&result);
    if (!json_serializer.Serialize(*res_ptr))
      return std::make_tuple(false, "");
  }
  return std::make_tuple(true, result);
}
}  // namespace

PepperExtensionSystemHost::PepperExtensionSystemHost(BrowserPpapiHost* host,
                                                     PP_Instance instance,
                                                     PP_Resource resource)
    : ResourceHost(host->GetPpapiHost(), instance, resource) {
  host->GetRenderFrameIDsForInstance(instance,
                                     &render_frame_id_.render_process_id,
                                     &render_frame_id_.render_frame_id);
  extension_function_handlers_[kAddDynamicCertificatePathName] =
      [this](const base::Value& data) {
        return AddDynamicCertificatePath(data);
      };
  extension_function_handlers_[kGetWindowIdOperationName] =
      [this](const base::Value& data) { return GetWindowId(data); };
  extension_function_handlers_[kCheckPrivilegeOperationName] =
      [this](const base::Value& data) { return CheckPrivilege(data); };
#if defined(OS_TIZEN_TV_PRODUCT)
  extension_function_handlers_[kSetIMERecommendedWordsName] =
      [this](const base::Value& data) { return SetIMERecommendedWords(data); };
  extension_function_handlers_[kSetIMERecommendedWordsTypeName] =
      [this](const base::Value& data) {
        return SetIMERecommendedWordsType(data);
      };
#endif
}

PepperExtensionSystemHost::~PepperExtensionSystemHost() {}

int32_t PepperExtensionSystemHost::OnResourceMessageReceived(
    const IPC::Message& msg,
    ppapi::host::HostMessageContext* context) {
  PPAPI_BEGIN_MESSAGE_MAP(PepperExtensionSystemHost, msg)
  PPAPI_DISPATCH_HOST_RESOURCE_CALL_0(
      PpapiHostMsg_ExtensionSystem_GetEmbedderName, OnHostMsgGetEmbedderName)
  PPAPI_DISPATCH_HOST_RESOURCE_CALL_0(
      PpapiHostMsg_ExtensionSystem_GetCurrentExtensionInfo,
      OnHostMsgGetCurrentExtensionInfo)
  PPAPI_DISPATCH_HOST_RESOURCE_CALL(
      PpapiHostMsg_ExtensionSystem_GenericSyncCall, OnHostMsgGenericSyncCall)
  PPAPI_END_MESSAGE_MAP()
  return PP_ERROR_FAILED;
}

int32_t PepperExtensionSystemHost::OnHostMsgGetEmbedderName(
    ppapi::host::HostMessageContext* context) {
  BrowserThread::PostTaskAndReplyWithResult(
      BrowserThread::UI, FROM_HERE,
      base::Bind(&GetEmbedderName, render_frame_id_),
      base::Bind(&PepperExtensionSystemHost::DidGetEmbedderName, AsWeakPtr(),
                 context->MakeReplyMessageContext()));
  return PP_OK_COMPLETIONPENDING;
}

int32_t PepperExtensionSystemHost::OnHostMsgGetCurrentExtensionInfo(
    ppapi::host::HostMessageContext* context) {
  BrowserThread::PostTaskAndReplyWithResult(
      BrowserThread::UI, FROM_HERE,
      base::Bind(&GetExtensionInfo, render_frame_id_),
      base::Bind(&PepperExtensionSystemHost::DidGetExtensionInfo, AsWeakPtr(),
                 context->MakeReplyMessageContext()));
  return PP_OK_COMPLETIONPENDING;
}

int32_t PepperExtensionSystemHost::OnHostMsgGenericSyncCall(
    ppapi::host::HostMessageContext* context,
    const std::string& operation_name,
    const std::string& operation_data) {

    base::StringPiece data_str(operation_data);
    JSONStringValueDeserializer json_deserializer(data_str);

    std::unique_ptr<base::Value> data(json_deserializer.Deserialize(nullptr,
                                                                    nullptr));
    if (!data) {
      LOG(ERROR) << "Couldn`t deserialize data";
      return PP_ERROR_FAILED;
    }

    bool is_handled;
    std::unique_ptr<base::Value> res_ptr =
        TryHandleInternally(operation_name, *data, &is_handled);
    if (!is_handled) {
      BrowserThread::PostTaskAndReplyWithResult(
          BrowserThread::UI, FROM_HERE,
          base::Bind(&HandleGenericSyncCall, render_frame_id_, operation_name,
                     data.release()),
          base::Bind(&PepperExtensionSystemHost::DidHandledGenericSyncCall,
                     AsWeakPtr(), context->MakeReplyMessageContext()));
      return PP_OK_COMPLETIONPENDING;
    }

    if (!res_ptr) {
      LOG(ERROR);
      return PP_ERROR_FAILED;
    }

    std::string result;
    JSONStringValueSerializer json_serializer(&result);
    if (!json_serializer.Serialize(*res_ptr)) {
      LOG(WARNING) << "Couldn`t serialize result of GenericSyncCall";
      return PP_ERROR_FAILED;
  }

  context->reply_msg =
      PpapiPluginMsg_ExtensionSystem_GenericSyncCallReply(result);
  return PP_OK;
}

void PepperExtensionSystemHost::DidGetEmbedderName(
    ppapi::host::ReplyMessageContext reply_context,
    std::tuple<bool, std::string> embedder_name) {
  if (std::get<0>(embedder_name))
    reply_context.params.set_result(PP_OK);
  else
    reply_context.params.set_result(PP_ERROR_FAILED);
  host()->SendReply(reply_context,
                    PpapiPluginMsg_ExtensionSystem_GetEmbedderNameReply(
                        std::get<1>(embedder_name)));
}

void PepperExtensionSystemHost::DidGetExtensionInfo(
    ppapi::host::ReplyMessageContext reply_context,
    std::tuple<bool, std::string> serialized_data) {
  if (std::get<0>(serialized_data))
    reply_context.params.set_result(PP_OK);
  else
    reply_context.params.set_result(PP_ERROR_FAILED);
  host()->SendReply(reply_context,
                    PpapiPluginMsg_ExtensionSystem_GetCurrentExtensionInfoReply(
                        std::get<1>(serialized_data)));
}

void PepperExtensionSystemHost::DidHandledGenericSyncCall(
    ppapi::host::ReplyMessageContext reply_context,
    std::tuple<bool, std::string> operation_result) {
  if (std::get<0>(operation_result))
    reply_context.params.set_result(PP_OK);
  else
    reply_context.params.set_result(PP_ERROR_FAILED);
  host()->SendReply(reply_context,
                    PpapiPluginMsg_ExtensionSystem_GenericSyncCallReply(
                        std::get<1>(operation_result)));
}

std::unique_ptr<base::Value> PepperExtensionSystemHost::TryHandleInternally(
    const std::string& operation_name,
    const base::Value& data,
    bool* was_handled) {
  *was_handled = false;

  auto it = extension_function_handlers_.find(operation_name);
  if (it == extension_function_handlers_.end())
    return base::MakeUnique<base::Value>();
  *was_handled = true;
  return it->second(data);
}

std::unique_ptr<base::Value>
PepperExtensionSystemHost::AddDynamicCertificatePath(const base::Value& data) {
#if defined(OS_TIZEN_TV_PRODUCT)
  const base::DictionaryValue* dict;
  if (!data.GetAsDictionary(&dict)) {
    LOG(ERROR) << "Provided data for adding dynamic "
               << "certificate path is not a dictionary!";
    return base::MakeUnique<base::Value>(false);
  }

  base::DictionaryValue::Iterator it(*dict);
  std::string value;

  auto frame_host = RenderFrameHost::FromID(render_frame_id_.render_process_id,
                                            render_frame_id_.render_frame_id);
  if (!frame_host) {
    LOG(ERROR) << "RenderFrameHost is nullptr!";
    return base::MakeUnique<base::Value>(false);
  }

  auto web_contents = WebContents::FromRenderFrameHost(frame_host);
  if (!web_contents) {
    LOG(ERROR) << "WebContents is nullptr!";
    return base::MakeUnique<base::Value>(false);
  }

  while (!it.IsAtEnd()) {
    if (!it.value().GetAsString(&value)) {
      LOG(ERROR) << "Provided value for key " << it.key()
                 << " is not a string!";
      return base::MakeUnique<base::Value>(false);
    }

    web_contents->AddDynamicCertificatePath(it.key(), value);

    it.Advance();
  }
  return base::MakeUnique<base::Value>(true);
#else
  LOG(ERROR) << "AddDynamicCertificatePath functionality"
             << " is available only on Tizen TV Products!";
  return base::MakeUnique<base::Value>(false);
#endif  // defined(OS_TIZEN_TV_PRODUCT)
}

std::unique_ptr<base::Value> PepperExtensionSystemHost::GetWindowId(
    const base::Value& data) {
  int id = -1;
  ExtensionSystemDelegate* delegate =
      ExtensionSystemDelegateManager::GetInstance()->GetDelegateForFrame(
          render_frame_id_);
  if (delegate)
    id = delegate->GetWindowId();
  return std::unique_ptr<base::Value>(new base::Value(id));
}

std::unique_ptr<base::Value> PepperExtensionSystemHost::CheckPrivilege(
    const base::Value& data) {
  std::string privilege;
  if (!data.GetAsString(&privilege)) {
    LOG(ERROR) << "Provided data for checking privilege is not a string!";
    return base::MakeUnique<base::Value>(false);
  }

  bool result = EwkPrivilegeChecker::GetInstance()->CheckPrivilege(privilege);
  LOG_IF(WARNING, !result) << "Privilege " << privilege << " is not granted";
  return base::MakeUnique<base::Value>(result);
}

#if defined(OS_TIZEN_TV_PRODUCT)
std::unique_ptr<base::Value> PepperExtensionSystemHost::SetIMERecommendedWords(
    const base::Value& data) {
  WebContents* web_contents = WebContentsFromFrameID(
      render_frame_id_.render_process_id, render_frame_id_.render_frame_id);
  std::string ime_data;
  if (web_contents && data.GetAsString(&ime_data)) {
    LOG(INFO) << "Set recommended words : " << ime_data;
    web_contents->SetIMERecommendedWords(ime_data);
    return std::make_unique<base::Value>(true);
  }
  return std::make_unique<base::Value>(false);
}

std::unique_ptr<base::Value>
PepperExtensionSystemHost::SetIMERecommendedWordsType(const base::Value& data) {
  WebContents* web_contents = WebContentsFromFrameID(
      render_frame_id_.render_process_id, render_frame_id_.render_frame_id);
  bool enable;
  if (web_contents && data.GetAsBoolean(&enable)) {
    LOG(INFO) << "Set recommended word enable : " << enable;
    web_contents->SetIMERecommendedWordsType(enable);
    return std::make_unique<base::Value>(true);
  }
  return std::make_unique<base::Value>(false);
}
#endif

}  // namespace content
