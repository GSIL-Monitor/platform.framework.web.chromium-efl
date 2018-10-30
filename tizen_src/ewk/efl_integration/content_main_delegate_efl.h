// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_MAIN_DELEGATE
#define CONTENT_MAIN_DELEGATE

#include "content/public/app/content_main_delegate.h"

#include "base/compiler_specific.h"
#include "content/public/browser/content_browser_client.h"
#include "content/public/renderer/content_renderer_client.h"
#include "common/content_client_efl.h"

namespace content {

class ContentMainDelegateEfl
    : public ContentMainDelegate {
 public:
  ContentMainDelegateEfl();
  // Tells the embedder that the absolute basic startup has been done, i.e.
  // it's now safe to create singletons and check the command line. Return true
  // if the process should exit afterwards, and if so, |exit_code| should be
  // set. This is the place for embedder to do the things that must happen at
  // the start. Most of its startup code should be in the methods below.
  virtual bool BasicStartupComplete(int* exit_code) override;
  virtual void PreSandboxStartup() override;
  virtual ContentBrowserClient* CreateContentBrowserClient() override;
  virtual ContentRendererClient* CreateContentRendererClient() override;

  ContentBrowserClient* GetContentBrowserClient() const;
#if defined(OS_TIZEN_TV_PRODUCT)
  void ProcessExiting(const std::string& process_type) override;
  void SetInspectorStatus(bool enable);
  bool GetInspectorStatus();
#endif

#if BUILDFLAG(ENABLE_NACL)
  void ZygoteStarting(
        std::vector<std::unique_ptr<ZygoteForkDelegate>>* delegates) override;
#endif

 private:
  void PreSandboxStartupBrowser();

  std::unique_ptr<ContentBrowserClient> browser_client_;
  std::unique_ptr<ContentRendererClient> renderer_client_;
  ContentClientEfl content_client_;
#if defined(OS_TIZEN_TV_PRODUCT)
  bool enableInspector;
#endif
  DISALLOW_COPY_AND_ASSIGN(ContentMainDelegateEfl);
};

}

#endif
