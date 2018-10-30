// Copyright 2017 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/pepper/pepper_teec_context_impl.h"

#include "base/task_runner.h"
#include "base/task_runner_util.h"
#include "base/task_scheduler/post_task.h"
#include "content/browser/renderer_host/pepper/browser_host_callback_helpers.h"
#include "content/public/browser/browser_ppapi_host.h"
#include "content/public/browser/browser_thread.h"
#include "ewk_privilege_checker.h"
#include "ppapi/c/samsung/pp_teec_samsung.h"
#include "ppapi/host/dispatch_host_message.h"
#include "ppapi/host/resource_host.h"
#include "ppapi/proxy/dispatch_reply_message.h"
#include "ppapi/proxy/ppapi_messages.h"

namespace {

template <typename... Args>
void ProcessReplyAdapterWithTEECResult(
    const base::Callback<void(int32_t, PP_TEEC_Result, Args...)>& callback,
    const PP_TEEC_Result& result,
    const Args&... args) {
  int32_t pp_error_code =
      (result.return_code == TEEC_SUCCESS) ? PP_OK : PP_ERROR_FAILED;

  callback.Run(pp_error_code, result, args...);
}

template <typename TaskReturnArg1, typename TaskArg2>
void TaskAdapter(const base::Callback<TaskReturnArg1(TaskArg2*)>& task,
                 std::tuple<TaskReturnArg1, TaskArg2>* result) {
  std::get<0>(*result) = task.Run(&std::get<1>(*result));
}

template <typename TaskReturnArg1,
          typename TaskArg2,
          typename ReplyArg1,
          typename ReplyArg2>
static void ReplyAdapter(
    const base::Callback<void(ReplyArg1, ReplyArg2)>& callback,
    std::tuple<TaskReturnArg1, TaskArg2>* result) {
  if (!callback.is_null())
    callback.Run(std::forward<TaskReturnArg1>(std::get<0>(*result)),
                 std::forward<TaskArg2>(std::get<1>(*result)));
}

template <typename TaskReturnArg1,
          typename TaskArg2,
          typename ReplyArg1,
          typename ReplyArg2>
bool PostTaskAndReplyWithMultipleResults(
    base::TaskRunner* task_runner,
    const base::Location& from_here,
    const base::Callback<TaskReturnArg1(TaskArg2*)>& task,
    const base::Callback<void(ReplyArg1, ReplyArg2)>& reply) {
  auto result = new std::tuple<TaskReturnArg1, TaskArg2>();
  return task_runner->PostTaskAndReply(
      from_here,
      base::Bind(&TaskAdapter<TaskReturnArg1, TaskArg2>, task, result),
      base::Bind(&ReplyAdapter<TaskReturnArg1, TaskArg2, ReplyArg1, ReplyArg2>,
                 reply, base::Owned(result)));
}

const char kTEEClientPrivilegeName[] = "http://tizen.org/privilege/tee.client";
}  // namespace

namespace content {

PepperTEECContextImpl::PepperTEECContextImpl(PP_Instance instance)
    : pp_instance_(instance),
      io_runnner_(base::CreateSequencedTaskRunnerWithTraits(
          {base::MayBlock(), base::TaskPriority::BACKGROUND,
           base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN})),
      teec_context_initialized_(false) {}

PepperTEECContextImpl::~PepperTEECContextImpl() {
  if (teec_context_initialized_.load())
    TEEC_FinalizeContext(&teec_context_);
}

void PepperTEECContextImpl::ExecuteTEECTask(
    const base::Callback<PP_TEEC_Result(TEEC_Context*)>& task,
    const base::Callback<void(int32_t, PP_TEEC_Result)>& reply) {
  base::PostTaskAndReplyWithResult(
      io_runnner_.get(), FROM_HERE, base::Bind(task, &teec_context_),
      base::Bind(&ProcessReplyAdapterWithTEECResult<>, reply));
}

void PepperTEECContextImpl::ExecuteTEECTask(
    const base::Callback<PP_TEEC_Result(TEEC_Context*, PP_TEEC_Operation*)>&
        task,
    const base::Callback<void(int32_t, PP_TEEC_Result, PP_TEEC_Operation)>&
        reply) {
  PostTaskAndReplyWithMultipleResults(
      io_runnner_.get(), FROM_HERE, base::Bind(task, &teec_context_),
      base::Bind(&ProcessReplyAdapterWithTEECResult<PP_TEEC_Operation>, reply));
}

int32_t PepperTEECContextImpl::ConvertReturnCode(TEEC_Result result) {
  return static_cast<int32_t>(result);
}

int32_t PepperTEECContextImpl::OnOpen(ppapi::host::HostMessageContext* context,
                                      const std::string& name) {
  if (teec_context_initialized_.load())
    return PP_ERROR_FAILED;

  if (!EwkPrivilegeChecker::GetInstance()->CheckPrivilege(
          kTEEClientPrivilegeName))
    return PP_ERROR_NOACCESS;

  base::PostTaskAndReplyWithResult(
      io_runnner_.get(), FROM_HERE,
      base::Bind(&PepperTEECContextImpl::Open, this, name),
      base::Bind(
          &ProcessReplyAdapterWithTEECResult<>,
          base::Bind(&ReplyCallbackWithValueOutput<
                         PpapiPluginMsg_TEECContext_OpenReply, PP_TEEC_Result>,
                     pp_instance_, context->MakeReplyMessageContext())));

  return PP_OK_COMPLETIONPENDING;
}

PP_TEEC_Result PepperTEECContextImpl::Open(const std::string& name) {
  PP_TEEC_Result result;
  const char* context_name = name.empty() ? nullptr : name.c_str();

  result.return_code =
      ConvertReturnCode(TEEC_InitializeContext(context_name, &teec_context_));
  if (result.return_code == PP_TEEC_SUCCESS) {
    teec_context_initialized_.store(true);
    result.return_origin = PP_TEEC_ORIGIN_TRUSTED_APP;
  } else {
    result.return_origin = PP_TEEC_ORIGIN_API;
  }
  return result;
}

}  // namespace content
