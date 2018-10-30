// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ewk_global_data.h"

#include "base/cpu.h"
#include "base/logging.h"
#include "base/message_loop/message_loop.h"
#include "base/path_service.h"
#include "browser/autofill/personal_data_manager_factory.h"
#include "command_line_efl.h"
#include "content/browser/gpu/gpu_main_thread_factory.h"
#include "content/browser/gpu/gpu_process_host.h"
#include "content/browser/renderer_host/render_process_host_impl.h"
#include "content/browser/utility_process_host_impl.h"
#include "content/common/content_client_export.h"
#include "content/gpu/in_process_gpu_thread.h"
#include "content/public/app/content_main.h"
#include "content/public/app/content_main_runner.h"
#include "content/public/browser/browser_main_runner.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/utility_process_host.h"
#include "content/public/common/content_switches.h"
#include "content/renderer/in_process_renderer_thread.h"
#include "content/utility/in_process_utility_thread.h"
#include "content_browser_client_efl.h"
#include "content_main_delegate_efl.h"
#include "efl/window_factory.h"
#include "eweb_view.h"
#include "message_pump_for_ui_efl.h"
#include "mojo/edk/embedder/embedder.h"
#include "renderer/content_renderer_client_efl.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/display/screen_efl.h"
#include "ui/ozone/public/ozone_platform.h"

#ifdef OS_TIZEN
#include <dlfcn.h>
void* EflExtensionHandle = 0;
#endif

#if defined(USE_WAYLAND)
#include "ui/base/clipboard/clipboard_helper_efl_wayland.h"
#endif

using base::MessageLoop;
using content::BrowserMainRunner;
using content::BrowserThread;
using content::ContentMainDelegateEfl;
using content::ContentMainRunner;
using content::GpuProcessHost;
using content::RenderProcessHost;
using content::UtilityProcessHost;

EwkGlobalData* EwkGlobalData::instance_ = NULL;

namespace {

std::unique_ptr<base::MessagePump> MessagePumpFactory() {
  return std::unique_ptr<base::MessagePump>(new base::MessagePumpForUIEfl);
}

}  // namespace

EwkGlobalData::EwkGlobalData()
    : content_main_runner_(ContentMainRunner::Create()),
      browser_main_runner_(BrowserMainRunner::Create()),
      content_main_delegate_efl_(nullptr) {}

EwkGlobalData::~EwkGlobalData() {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  // URLRequestContextGetterEfl needs to be deleted before loop stops
  system_request_context_ = NULL;

  // We need to pretend that message loop was stopped so chromium unwinds
  // correctly
  if (base::RunLoop::IsRunningOnCurrentThread())
    base::RunLoop::QuitCurrentDeprecated();

  // Since we forcibly shutdown our UI message loop, it is possible
  // that important pending Tasks are simply not executed.
  // Among these, the destruction of RenderProcessHostImpl class
  // (scheduled with DeleteSoon) performs various process umplumbing
  // stuff and also holds strong references of lots of other objects
  // (e.g. URLRequestContextGetterEfl, RenderWidgetHelper,
  // ChildProcessLauncher). Not performing the proper destruction
  // of them might result in crashes or other undefined problems.
  //
  // Line below gives the message loop a extra run ('till it gets idle)
  // to perform such pending tasks.
  base::RunLoop().RunUntilIdle();

  // Shutdown in-process-renderer properly before browser context is deleted.
  // Sometimes renderer's ipc handler refers browser context during shutdown.
  if (content::RenderProcessHost::run_renderer_in_process()) {
    /* LCOV_EXCL_START */
    content::ContentRendererClient* crc =
        content::GetContentClientExport()->renderer();
    auto crce = static_cast<ContentRendererClientEfl*>(crc);
    crce->set_shutting_down(true);
    content::RenderProcessHostImpl::ShutDownInProcessRenderer();

    // In single process mode, at-exit callbacks are disabled, so we should run
    // shutdown sequence manually if it is needed.
#if defined(USE_WAYLAND)
    ClipboardHelperEfl::ShutdownIfNeeded();
#endif
    /* LCOV_EXCL_STOP */
  }

  // Delete browser context for browser context cleanup that use
  // PostShutdownBlockingTask() if single process mode is enabled.
  // (browser context is not deleted in EWebContext destructor.)
  // Shutdown block tasks will run before browser main runner is deleted.
  content::ContentBrowserClient* cbc =
      content::GetContentClientExport()->browser();
  auto cbce = static_cast<content::ContentBrowserClientEfl*>(cbc);
  cbce->CleanUpBrowserContext();

  // browser_main_runner must be deleted first as it depends on
  // content_main_runner
  delete browser_main_runner_;
  delete content_main_runner_;

  // content_main_delegate_efl_ must be deleted after content_main_runner_
  delete content_main_delegate_efl_;
}

EwkGlobalData* EwkGlobalData::GetInstance() {
  if (instance_) {
    /* LCOV_EXCL_START */
    CHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::UI));
    return instance_;
    /* LCOV_EXCL_STOP */
  }

  // Workaround for cpu info logging asserting if executed on the wrong thread
  // during cpu info lazy instance initialization.
  base::CPU cpu;
  DCHECK(cpu.cpu_brand() != "");

  instance_ = new EwkGlobalData();

  // TODO: Check if service_manager::Main should be used instead.
  // The Mojo EDK must be initialized before using IPC.
  mojo::edk::Init();

  bool message_pump_overridden =
      base::MessageLoop::InitMessagePumpForUIFactory(&MessagePumpFactory);
  DCHECK(message_pump_overridden);

  efl::WindowFactory::SetDelegate(&EWebView::GetHostWindowDelegate);

  instance_->content_main_delegate_efl_ = new ContentMainDelegateEfl();

  // Call to CommandLineEfl::GetDefaultPortParams() should be before content
  // main runner initialization in order to pass command line parameters
  // for current process that are used in content main runner initialization.
  content::MainFunctionParams main_funtion_params =
      CommandLineEfl::GetDefaultPortParams();
  content::ContentMainParams params(instance_->content_main_delegate_efl_);
  params.argc = CommandLineEfl::GetArgc();
  params.argv = CommandLineEfl::GetArgv();
  instance_->content_main_runner_->Initialize(params);

  // ScreenEfl should be installed only after CommandLine has been initialized.
  ui::InstallScreenInstance();

#if defined(USE_OZONE)
  ui::OzonePlatform::InitParams init_params;
  init_params.single_process = true;
  ui::OzonePlatform::InitializeForUI(init_params);
#endif

  instance_->browser_main_runner_->Initialize(main_funtion_params);

  base::ThreadRestrictions::SetIOAllowed(true);

  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kSingleProcess)) {
    content::UtilityProcessHostImpl::RegisterUtilityMainThreadFactory(
        content::CreateInProcessUtilityThread);  // LCOV_EXCL_LINE
    content::RenderProcessHostImpl::RegisterRendererMainThreadFactory(
        content::CreateInProcessRendererThread);  // LCOV_EXCL_LINE
    content::RegisterGpuMainThreadFactory(
        reinterpret_cast<content::GpuMainThreadFactoryFunction>(
            content::CreateInProcessGpuThread));  // LCOV_EXCL_LINE
  }

#ifndef NDEBUG
  logging::LoggingSettings settings;
  settings.logging_dest = logging::LOG_TO_SYSTEM_DEBUG_LOG;
  logging::InitLogging(settings);
  logging::SetLogItems(true, true, true, true);
#endif

#ifdef OS_TIZEN
  if (!EflExtensionHandle)
    EflExtensionHandle = dlopen("/usr/lib/libefl-extension.so.0", RTLD_LAZY);
#endif

  return instance_;
}

/* LCOV_EXCL_START */
content::URLRequestContextGetterEfl*
EwkGlobalData::GetSystemRequestContextGetter() {
  if (!system_request_context_.get()) {
    system_request_context_ = new content::URLRequestContextGetterEfl();
  }
  return system_request_context_.get();
}
/* LCOV_EXCL_STOP */

void EwkGlobalData::Delete() {
  delete instance_;
  instance_ = NULL;
}

/* LCOV_EXCL_START */
// static
bool EwkGlobalData::IsInitialized() {
  return instance_ != NULL;
}
/* LCOV_EXCL_STOP */
