// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remote_controller_wrt.h"

#include <tuple>
#include <unordered_map>
#include <vector>

#include "base/bind.h"
#include "base/callback.h"
#include "base/logging.h"
#include "base/memory/ref_counted.h"
#include "content/public/browser/browser_ppapi_host.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/extension_system_delegate.h"
#include "ppapi/c/pp_errors.h"

namespace content {

namespace {

static const char kOperationNameRegister[] = "RegisterKey";
static const char kOperationNameUnRegister[] = "UnRegisterKey";

class RemoteControllerWRT
    : public content::PepperRemoteControllerHost::PlatformDelegate {
 public:
  RemoteControllerWRT(int render_process_id, int render_frame_id);
  ~RemoteControllerWRT() override = default;

  void RegisterKeys(const std::vector<std::string>& keys,
                    const base::Callback<void(int32_t)>& cb) override;

  void UnRegisterKeys(const std::vector<std::string>& keys,
                      const base::Callback<void(int32_t)>& cb) override;

 private:
  // Class which will execute requests on UI Thread
  class TaskExecutor;
  scoped_refptr<TaskExecutor> executor_;

  DISALLOW_COPY_AND_ASSIGN(RemoteControllerWRT);
};

class RemoteControllerWRT::TaskExecutor
    : public base::RefCountedThreadSafe<TaskExecutor> {
 public:
  TaskExecutor(int render_process_id, int render_frame_id);

  int32_t RegisterKeys(const std::vector<std::string>& keys);

  int32_t UnRegisterKeys(const std::vector<std::string>& keys);

 private:
  friend class base::RefCountedThreadSafe<TaskExecutor>;
  ~TaskExecutor() = default;

  int32_t DispatchToGenericSyncCall(const std::string& operation_name,
                                    const std::vector<std::string>& keys);

  ExtensionSystemDelegateManager::RenderFrameID render_frame_id_;

  DISALLOW_COPY_AND_ASSIGN(TaskExecutor);
};

RemoteControllerWRT::TaskExecutor::TaskExecutor(int render_process_id,
                                                int render_frame_id) {
  render_frame_id_.render_frame_id = render_frame_id;
  render_frame_id_.render_process_id = render_process_id;
}

int32_t RemoteControllerWRT::TaskExecutor::RegisterKeys(
    const std::vector<std::string>& keys) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  int32_t ret = DispatchToGenericSyncCall(kOperationNameRegister, keys);
  LOG_IF(ERROR, ret != PP_OK) << "Registering keys failed";
  return ret;
}

int32_t RemoteControllerWRT::TaskExecutor::UnRegisterKeys(
    const std::vector<std::string>& keys) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  int32_t ret = DispatchToGenericSyncCall(kOperationNameUnRegister, keys);
  LOG_IF(ERROR, ret != PP_OK) << "UnRegistering Keys failed";
  return ret;
}

int32_t RemoteControllerWRT::TaskExecutor::DispatchToGenericSyncCall(
    const std::string& operation_name,
    const std::vector<std::string>& keys) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  ExtensionSystemDelegate* delegate =
      ExtensionSystemDelegateManager::GetInstance()->GetDelegateForFrame(
          render_frame_id_);
  if (!delegate) {
    LOG(ERROR) << "Failed to acquire delegate can`t perform " << operation_name;
    return PP_ERROR_FAILED;
  }

  // We can`t tell if key that we try to (Un)Register have failed because
  // it have been already (Un)Registered. So if at least one key have
  // been successfully processed, we assume that other keys that failed have
  // been already registered before the call.
  // This logic will fail when WRT removes support for specific key and we
  // try to register on it. But this can be quickly checked basing on printed
  // below log and looking in to sources of
  // (crosswalk_tizen)/runtime/browser/input_device_manager.cc
  bool registered_any_key = false;
  for (const std::string& key_platform_name : keys) {
    if (key_platform_name.empty()) {
      LOG(ERROR) << "Key platform name is empty.";
      return PP_ERROR_BADARGUMENT;
    }
    std::unique_ptr<base::Value> key_name =
        std::unique_ptr<base::Value>(new base::Value(key_platform_name));
    std::unique_ptr<base::Value> res_ptr =
        delegate->GenericSyncCall(operation_name, *key_name);
    bool sync_call_result = false;
    bool parse = res_ptr->GetAsBoolean(&sync_call_result);
    if (!parse) {
      LOG(ERROR) << "Callback returned unexpected result type: "
                 << res_ptr->GetType();
      return PP_ERROR_FAILED;
    }
    if (sync_call_result) {
      registered_any_key = true;
    } else {
      LOG(WARNING) << "Result of " << operation_name << ": "
                   << key_platform_name << ", returned false. Either "
                   << operation_name << " failed or Key have been already "
                   << "processed before call. Continuing...";
    }
  }
  return (registered_any_key ? PP_OK : PP_ERROR_FAILED);
}

RemoteControllerWRT::RemoteControllerWRT(int render_process_id,
                                         int render_frame_id)
    : executor_{new TaskExecutor(render_process_id, render_frame_id)} {}

void RemoteControllerWRT::RegisterKeys(
    const std::vector<std::string>& keys,
    const base::Callback<void(int32_t)>& cb) {
  BrowserThread::PostTaskAndReplyWithResult(
      BrowserThread::UI, FROM_HERE,
      base::Bind(&RemoteControllerWRT::TaskExecutor::RegisterKeys, executor_,
                 keys),
      cb);
}

void RemoteControllerWRT::UnRegisterKeys(
    const std::vector<std::string>& keys,
    const base::Callback<void(int32_t)>& cb) {
  BrowserThread::PostTaskAndReplyWithResult(
      BrowserThread::UI, FROM_HERE,
      base::Bind(&RemoteControllerWRT::TaskExecutor::UnRegisterKeys, executor_,
                 keys),
      cb);
}

}  // namespace

std::unique_ptr<PepperRemoteControllerHost::PlatformDelegate>
CreateRemoteControllerWRT(PP_Instance instance,
                          content::BrowserPpapiHost* host) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  int render_process_id, render_frame_id;
  if (!host->GetRenderFrameIDsForInstance(instance, &render_process_id,
                                          &render_frame_id)) {
    LOG(ERROR) << "Can't get process_id and frame_id";
    return {};
  }

  return std::unique_ptr<PepperRemoteControllerHost::PlatformDelegate>{
      new RemoteControllerWRT(render_process_id, render_frame_id)};
}

}  // namespace content
