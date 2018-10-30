// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content_main_delegate_efl.h"

#include "base/path_service.h"
#include "browser/resource_dispatcher_host_delegate_efl.h"
#include "command_line_efl.h"
#include "content/browser/gpu/gpu_main_thread_factory.h"
#include "content/browser/gpu/gpu_process_host.h"
#include "content/common/locale_efl.h"
#include "content/common/paths_efl.h"
#include "content/gpu/in_process_gpu_thread.h"
#include "content/public/common/content_switches.h"
#include "content_browser_client_efl.h"
#include "renderer/content_renderer_client_efl.h"
#include "ui/base/resource/resource_bundle.h"
#if defined(OS_TIZEN_TV_PRODUCT)
#include "devtools_port_manager.h"
#endif
#include "components/nacl/common/features.h"
#if BUILDFLAG(ENABLE_NACL)
#include "components/nacl/common/nacl_paths.h"
#include "components/nacl/zygote/nacl_fork_delegate_linux.h"
#endif

namespace content {

namespace {
std::string SubProcessPath() {
  static std::string result;
  if (!result.empty())
    return result;

  char* path_from_env = getenv("EFL_WEBPROCESS_PATH");
  if (path_from_env) {
    result = std::string(path_from_env);
    return result;
  }

  base::FilePath pak_dir;
  base::FilePath pak_file;
  PathService::Get(base::DIR_EXE, &pak_dir);
  pak_file = pak_dir.Append(FILE_PATH_LITERAL("efl_webprocess"));

  return pak_file.AsUTF8Unsafe();
}

void InitializeUserDataDir() {
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();

  base::FilePath user_data_dir = command_line->GetSwitchValuePath("user-data-dir");
  if (!user_data_dir.empty() && !PathService::OverrideAndCreateIfNeeded(
      PathsEfl::DIR_USER_DATA, user_data_dir, true, true)) {
    DLOG(WARNING) << "Could not set user data directory to " << user_data_dir.value();

    if (!PathService::Get(PathsEfl::DIR_USER_DATA, &user_data_dir))
      CHECK(false) << "Could not get default user data directory";

    command_line->AppendSwitchPath("user-data-dir", user_data_dir);
  }
}

void InitializeDiskCacheDir() {
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();

  base::FilePath disk_cache_dir = command_line->GetSwitchValuePath("disk-cache-dir");
  if (!disk_cache_dir.empty() && !PathService::OverrideAndCreateIfNeeded(
      base::DIR_CACHE, disk_cache_dir, true, true)) {
    DLOG(WARNING) << "Could not set disk cache directory to " << disk_cache_dir.value();

    if (!PathService::Get(base::DIR_CACHE, &disk_cache_dir))
      CHECK(false) << "Could not get default disk cache directory";

    command_line->AppendSwitchPath("disk-cache-dir", disk_cache_dir);
  }
}
} // namespace

ContentMainDelegateEfl::ContentMainDelegateEfl() {
#if defined(OS_TIZEN_TV_PRODUCT)
  enableInspector = false;
#endif
}

void ContentMainDelegateEfl::PreSandboxStartupBrowser() {
  LocaleEfl::Initialize();
  PathService::Override(base::FILE_EXE, base::FilePath(SubProcessPath()));
  PathService::Override(base::FILE_MODULE, base::FilePath(SubProcessPath()));

  InitializeUserDataDir();
  InitializeDiskCacheDir();

  // needed for gpu process
  base::CommandLine::ForCurrentProcess()->AppendSwitchPath(
      switches::kBrowserSubprocessPath, base::FilePath(SubProcessPath()));

  // needed for gpu thread
  content::RegisterGpuMainThreadFactory(CreateInProcessGpuThread);
}

void ContentMainDelegateEfl::PreSandboxStartup() {
  base::FilePath pak_dir;
  base::FilePath pak_file;
  PathService::Get(base::DIR_EXE, &pak_dir);
  pak_file = pak_dir.Append(FILE_PATH_LITERAL("content_shell.pak"));
  ui::ResourceBundle::InitSharedInstanceWithPakPath(pak_file);

  const base::CommandLine& command_line =
      *base::CommandLine::ForCurrentProcess();
  std::string process_type =
      command_line.GetSwitchValueASCII(switches::kProcessType);
  if (process_type.empty()) { // browser process
    PreSandboxStartupBrowser();
  }
}

ContentRendererClient* ContentMainDelegateEfl::CreateContentRendererClient() {
  renderer_client_.reset(new ContentRendererClientEfl);
  return renderer_client_.get();
}

ContentBrowserClient* ContentMainDelegateEfl::CreateContentBrowserClient() {
  browser_client_.reset(new ContentBrowserClientEfl);
  return browser_client_.get();
}

bool ContentMainDelegateEfl::BasicStartupComplete(int* /*exit_code*/) {
  content::SetContentClient(&content_client_);
  PathsEfl::Register();

#if BUILDFLAG(ENABLE_NACL)
  nacl::RegisterPathProvider();
#endif

  return false;
}

/* LCOV_EXCL_START */
ContentBrowserClient* ContentMainDelegateEfl::GetContentBrowserClient() const {
  return browser_client_.get();
}
/* LCOV_EXCL_STOP */
#if defined(OS_TIZEN_TV_PRODUCT)
void ContentMainDelegateEfl::ProcessExiting(const std::string& process_type) {
  if (!enableInspector)
    return;

  if (process_type.empty() &&
      devtools_http_handler::DevToolsPortManager::GetInstance())  // browser
    devtools_http_handler::DevToolsPortManager::GetInstance()->ReleasePort();
}
void ContentMainDelegateEfl::SetInspectorStatus(bool enable) {
  enableInspector = enable;
}

bool ContentMainDelegateEfl::GetInspectorStatus() {
  return enableInspector;
}

#endif

#if BUILDFLAG(ENABLE_NACL)
void ContentMainDelegateEfl::ZygoteStarting(
    std::vector<std::unique_ptr<ZygoteForkDelegate>>* delegates) {
  nacl::AddNaClZygoteForkDelegates(delegates);
}
#endif

} // namespace content
