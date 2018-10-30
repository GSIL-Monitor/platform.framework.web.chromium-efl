// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RENDERER_HOST_PEPPER_MEDIA_PLAYER_TIZEN_PEPPER_TIZEN_PLAYER_DISPATCHER_H_
#define CONTENT_BROWSER_RENDERER_HOST_PEPPER_MEDIA_PLAYER_TIZEN_PEPPER_TIZEN_PLAYER_DISPATCHER_H_

#include <memory>
#include <tuple>
#include <vector>

#include "base/callback.h"
#include "base/memory/ref_counted.h"
#include "base/message_loop/message_loop.h"
#include "base/task_runner_util.h"
#include "base/threading/thread.h"
#include "content/browser/renderer_host/pepper/media_player/tizen/pepper_player_adapter_interface.h"
#include "content/browser/renderer_host/pepper/media_player/tizen/pepper_tizen_drm_manager.h"
#include "ppapi/c/pp_errors.h"
#include "ppapi/c/samsung/pp_media_player_samsung.h"

namespace base {
class Location;
}  // namespace base

namespace content {

class ElementaryStreamListenerPrivate;
class ElementaryStreamListenerPrivateWrapper;

class PepperTizenPlayerDispatcher {
 public:
  ~PepperTizenPlayerDispatcher();

  static std::unique_ptr<PepperTizenPlayerDispatcher> Create(
      std::unique_ptr<PepperPlayerAdapterInterface> player);

  // Dispatches operation not returning any value to the player thread
  template <typename... Args>
  static void DispatchOperation(
      PepperTizenPlayerDispatcher* dispatcher,
      const base::Location& from_here,
      const base::Callback<void(PepperPlayerAdapterInterface*, Args...)>& task,
      const Args&... args) {
    if (!dispatcher)
      return;

    dispatcher->PlayerTaskRunner()->PostTask(
        from_here, base::Bind(task, dispatcher->player(), args...));
  }

  template <typename... Args>
  static void DispatchOperationOnAppendThread(
      PepperTizenPlayerDispatcher* dispatcher,
      const base::Location& from_here,
      const base::Callback<void(PepperPlayerAdapterInterface*)>& task,
      const Args&... args) {
    if (!dispatcher)
      return;

    dispatcher->PlayerAppendPackedTaskRunner()->PostTask(
        from_here, base::Bind(task, dispatcher->player(), args...));
  }

  // Handles simple operations that return only error code. Callback is run
  // after player_function is executed.
  static void DispatchOperation(
      PepperTizenPlayerDispatcher* dispatcher,
      const base::Location& from_here,
      const base::Callback<int32_t(PepperPlayerAdapterInterface*)>& task,
      const base::Callback<void(int32_t)>& callback) {
    if (FailIfNoDispatcher(dispatcher, from_here, callback))
      return;

    base::PostTaskAndReplyWithResult(dispatcher->PlayerTaskRunner(), from_here,
                                     base::Bind(task, dispatcher->player()),
                                     callback);
  }

  // Handles simple operations that return only error code. Callback is run
  // after player_function is executed.
  static void DispatchOperationOnAppendThread(
      PepperTizenPlayerDispatcher* dispatcher,
      const base::Location& from_here,
      const base::Callback<int32_t(PepperPlayerAdapterInterface*)>& task,
      const base::Callback<void(int32_t)>& callback) {
    if (FailIfNoDispatcher(dispatcher, from_here, callback))
      return;

    base::PostTaskAndReplyWithResult(
        dispatcher->PlayerAppendPackedTaskRunner(), from_here,
        base::Bind(task, dispatcher->player()), callback);
  }

  // See DispatchOperation that takes base::Callback as a task.
  template <typename... Args>
  static void DispatchOperation(
      PepperTizenPlayerDispatcher* dispatcher,
      const base::Location& from_here,
      base::Callback<int32_t(PepperPlayerAdapterInterface*, Args...)>
          player_function,
      const base::Callback<void(int32_t)>& callback,
      const Args&... args) {
    if (FailIfNoDispatcher(dispatcher, from_here, callback))
      return;

    base::PostTaskAndReplyWithResult(
        dispatcher->PlayerTaskRunner(), from_here,
        base::Bind(player_function, dispatcher->player(), args...), callback);
  }

  // Handles operations that return a value. Following functor signature is
  // supported: int32_t(PepperPlayerAdapterInterface*, ReturnValue*)
  //
  // Calee must provide handler that will be run on the IO thread. The handler
  // takes player error code as a first argument and platform player returned
  // value as a second argument. Interpretation of the arguments is done
  // by the handler.
  template <typename TaskReturnType, typename ReplyArgType>
  static void DispatchOperationWithResult(
      PepperTizenPlayerDispatcher* dispatcher,
      const base::Location& from_here,
      const base::Callback<int32_t(PepperPlayerAdapterInterface*,
                                   TaskReturnType*)>& task,
      const base::Callback<void(int32_t, ReplyArgType)>& callback) {
    if (FailIfNoDispatcher(dispatcher, from_here, callback, ReplyArgType{}))
      return;

    dispatcher->PostTaskAndReplyWithMultipleResults(
        from_here, base::Bind(task, dispatcher->player()), callback);
  }

  static void DispatchToTaskRunner(
      const scoped_refptr<base::SingleThreadTaskRunner>& runner,
      const base::Callback<void(int32_t)>& task,
      int32_t code) {
    runner->PostTask(FROM_HERE, base::Bind(task, code));
  }

  // Dispatch a task that will complete asynchronously on a player thread.
  static void DispatchAsyncOperation(
      PepperTizenPlayerDispatcher* dispatcher,
      const base::Location& from_here,
      const base::Callback<void(PepperPlayerAdapterInterface*,
                                const base::Callback<void(int32_t)>&)>&
          async_task,
      const base::Callback<void(int32_t)>& callback) {
    if (FailIfNoDispatcher(dispatcher, from_here, callback))
      return;

    dispatcher->PlayerTaskRunner()->PostTask(
        from_here,
        base::Bind(
            async_task, dispatcher->player(),
            base::Bind(&PepperTizenPlayerDispatcher::DispatchToTaskRunner,
                       base::MessageLoop::current()->task_runner(), callback)));
  }

  // Check whether current thread is the player thread. You want to check every
  // task that should execute on player thread using the following statement:
  // DCHECK(dispatcher->IsRunningOnPlayerThread())
  bool IsRunningOnPlayerThread() const {
    return player_thread_.message_loop()
        ->task_runner()
        ->BelongsToCurrentThread();
  }

  PepperPlayerAdapterInterface* player() { return player_.get(); }

  bool ResetPlayer();

 private:
  PepperTizenPlayerDispatcher(
      std::unique_ptr<PepperPlayerAdapterInterface> player);

  bool Initialize();
  void Destroy();
  bool DestroyPlayer();

  // Gets the main platform player task runner. Most player operations should be
  // called here runner.
  base::SingleThreadTaskRunner* PlayerTaskRunner() {
    auto task_runner = player_thread_.message_loop()->task_runner().get();
    DCHECK(task_runner != nullptr);
    return task_runner;
  }

  base::SingleThreadTaskRunner* PlayerAppendPackedTaskRunner() {
    auto task_runner =
        append_packet_thread_.message_loop()->task_runner().get();
    DCHECK(task_runner != nullptr);
    return task_runner;
  }

  // Returns false if there is no need to fail (i.e. dispatcher exists)
  // or runs a callback with an error and then returns true.
  template <typename Functor, typename... Args>
  static bool FailIfNoDispatcher(PepperTizenPlayerDispatcher* dispatcher,
                                 const base::Location& from_here,
                                 const Functor& callback,
                                 const Args&... args) {
    if (dispatcher)
      return false;

    base::MessageLoop::current()->task_runner()->PostTask(
        from_here,
        base::Bind(callback, static_cast<int32_t>(PP_ERROR_RESOURCE_FAILED),
                   args...));
    return true;
  }

  // As PostTaskAndReplyWithResult except task can return extra value using an
  // argument, so it can both return status code and produce a result.
  //
  // This is a low level operation, prefer DispatchOperation when applicable.
  //
  // TODO(p.balut): Change to parameter pack when it's worth the effort (i.e.
  //                needed somwewhere).
  template <typename TaskReturnArg1,
            typename TaskArg2,
            typename ReplyArg1,
            typename ReplyArg2>
  bool PostTaskAndReplyWithMultipleResults(
      const base::Location& from_here,
      const base::Callback<TaskReturnArg1(TaskArg2*)>& task,
      const base::Callback<void(ReplyArg1, ReplyArg2)>& reply) {
    auto result = new std::tuple<TaskReturnArg1, TaskArg2>();
    return PlayerTaskRunner()->PostTaskAndReply(
        from_here,
        base::Bind(
            &PepperTizenPlayerDispatcher::TaskAdapter<TaskReturnArg1, TaskArg2>,
            task, result),
        base::Bind(
            &PepperTizenPlayerDispatcher::ReplyAdapter<TaskReturnArg1, TaskArg2,
                                                       ReplyArg1, ReplyArg2>,
            reply, base::Owned(result)));
  }

  template <typename TaskReturnArg1, typename TaskArg2>
  static void TaskAdapter(const base::Callback<TaskReturnArg1(TaskArg2*)>& task,
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

  // Threads that runs player task runner.
  base::Thread player_thread_;
  base::Thread append_packet_thread_;

  std::unique_ptr<PepperTizenDRMManager> drm_manager_;

  std::unique_ptr<PepperPlayerAdapterInterface> player_;

  DISALLOW_COPY_AND_ASSIGN(PepperTizenPlayerDispatcher);
};

}  // namespace content

#endif  // CONTENT_BROWSER_RENDERER_HOST_PEPPER_MEDIA_PLAYER_TIZEN_PEPPER_TIZEN_PLAYER_DISPATCHER_H_
