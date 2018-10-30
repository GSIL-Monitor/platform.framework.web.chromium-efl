// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/nacl/browser/nacl_browser_delegate_impl_efl.h"

#include "base/path_service.h"
#include "base/threading/thread_restrictions.h"
#include "content/browser/renderer_host/pepper/browser_pepper_host_factory_efl.h"
#include "content/common/paths_efl.h"
#include "content/public/browser/browser_thread.h"

NaClBrowserDelegateImplEfl::NaClBrowserDelegateImplEfl() {
}

NaClBrowserDelegateImplEfl::~NaClBrowserDelegateImplEfl() {
}

void NaClBrowserDelegateImplEfl::ShowMissingArchInfobar(int render_process_id,
                                                        int render_view_id) {
}

bool NaClBrowserDelegateImplEfl::DialogsAreSuppressed() {
  return true;
}

bool NaClBrowserDelegateImplEfl::GetCacheDirectory(base::FilePath* cache_dir) {
  base::ThreadRestrictions::ScopedAllowIO allow_io;
  return PathService::Get(PathsEfl::DIR_DATA_PATH, cache_dir);
}

bool NaClBrowserDelegateImplEfl::GetPluginDirectory(
    base::FilePath* plugin_dir) {
  base::ThreadRestrictions::ScopedAllowIO allow_io;
  return PathService::Get(PathsEfl::DIR_INTERNAL_PLUGINS, plugin_dir);
}

bool NaClBrowserDelegateImplEfl::GetPnaclDirectory(base::FilePath* pnacl_dir) {
  base::ThreadRestrictions::ScopedAllowIO allow_io;
  return PathService::Get(PathsEfl::DIR_PNACL_COMPONENT, pnacl_dir);
}

bool NaClBrowserDelegateImplEfl::GetUserDirectory(base::FilePath* user_dir) {
  base::ThreadRestrictions::ScopedAllowIO allow_io;
  return PathService::Get(PathsEfl::DIR_DATA_PATH, user_dir);
}

std::string NaClBrowserDelegateImplEfl::GetVersionString() const {
  // @FIXME(dennis.oh) proper version string is...?
  return "chromium-efl";
}

ppapi::host::HostFactory* NaClBrowserDelegateImplEfl::CreatePpapiHostFactory(
    content::BrowserPpapiHost* ppapi_host) {
  return new BrowserPepperHostFactoryEfl(ppapi_host);
}

void NaClBrowserDelegateImplEfl::SetDebugPatterns(
    const std::string& debug_patterns) {
}

bool NaClBrowserDelegateImplEfl::URLMatchesDebugPatterns(
    const GURL& manifest_url) {
  return true;
}

// This function is security sensitive.  Be sure to check with a security
// person before you modify it.
bool NaClBrowserDelegateImplEfl::MapUrlToLocalFilePath(
    const GURL& file_url,
    bool use_blocking_api,
    const base::FilePath& profile_directory,
    base::FilePath* file_path) {
  return false;
}

bool NaClBrowserDelegateImplEfl::IsNonSfiModeAllowed(
    const base::FilePath& profile_directory,
    const GURL& manifest_url) {
  return false;
}
