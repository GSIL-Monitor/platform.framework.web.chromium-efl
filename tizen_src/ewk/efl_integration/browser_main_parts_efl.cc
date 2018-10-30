// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser_main_parts_efl.h"

#include "browser/webui/web_ui_controller_factory_efl.h"
#include "devtools_delegate_efl.h"

#include "base/base_switches.h"
#include "base/command_line.h"
#include "content/public/common/content_switches.h"
#include "components/nacl/common/features.h"

#if BUILDFLAG(ENABLE_NACL)
#include "components/nacl/browser/nacl_browser.h"
#include "components/nacl/browser/nacl_browser_delegate_impl_efl.h"
#include "components/nacl/browser/nacl_process_host.h"
#include "content/public/browser/browser_thread.h"
#endif

#if defined(OS_TIZEN)
#include "browser/geolocation/location_provider_efl.h"
#include "device/geolocation/geolocation_provider.h"
#endif

#if defined(OS_TIZEN_TV_PRODUCT)
#include "io_thread_delegate_efl.h"
#endif

using content::DevToolsDelegateEfl;

namespace content {

int BrowserMainPartsEfl::PreCreateThreads() {
#if defined(OS_TIZEN)
  device::GeolocationProvider::SetGeolocationDelegate(
      new device::GeolocationDelegateEfl());
#endif

#if defined(OS_TIZEN_TV_PRODUCT)
  io_thread_delegate_.reset(new IOThreadDelegateEfl());
#endif
  return 0;
}

#if defined(OS_TIZEN_TV_PRODUCT)
void BrowserMainPartsEfl::PostDestroyThreads() {
  io_thread_delegate_.reset();
}
#endif

void BrowserMainPartsEfl::PreMainMessageLoopRun() {
  content::WebUIControllerFactory::RegisterFactory(
      WebUIControllerFactoryEfl::GetInstance());

#if BUILDFLAG(ENABLE_NACL)
  nacl::NaClBrowser::SetDelegate(base::MakeUnique<NaClBrowserDelegateImplEfl>());

  content::BrowserThread::PostTask(
      content::BrowserThread::IO,
      FROM_HERE,
      base::Bind(nacl::NaClProcessHost::EarlyStartup));
#endif

  // PreMainMessageLoopRun is called just before the main message loop is run.
  // This is to create DevToolsDelegateEfl instance at PreMainMessageLoopRun
  const base::CommandLine& command_line = *base::CommandLine::ForCurrentProcess();
  if (command_line.HasSwitch(switches::kRemoteDebuggingPort))
    DevToolsDelegateEfl::Start();  // LCOV_EXCL_LINE
}

void BrowserMainPartsEfl::PostMainMessageLoopRun() {
  DevToolsDelegateEfl::Stop();  // LCOV_EXCL_LINE
}

}  // namespace content
