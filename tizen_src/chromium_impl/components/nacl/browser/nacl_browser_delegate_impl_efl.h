// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_NACL_BROWSER_NACL_BROWSER_DELEGATE_IMPL_EFL_H_
#define COMPONENTS_NACL_BROWSER_NACL_BROWSER_DELEGATE_IMPL_EFL_H_

#include <string>

#include "components/nacl/browser/nacl_browser_delegate.h"

class NaClBrowserDelegateImplEfl : public NaClBrowserDelegate {
 public:
  NaClBrowserDelegateImplEfl();
  virtual ~NaClBrowserDelegateImplEfl();

  void ShowMissingArchInfobar(int render_process_id,
                              int render_view_id) override;
  bool DialogsAreSuppressed() override;
  bool GetCacheDirectory(base::FilePath* cache_dir) override;
  bool GetPluginDirectory(base::FilePath* plugin_dir) override;
  bool GetPnaclDirectory(base::FilePath* pnacl_dir) override;
  bool GetUserDirectory(base::FilePath* user_dir) override;
  std::string GetVersionString() const override;
  ppapi::host::HostFactory* CreatePpapiHostFactory(
      content::BrowserPpapiHost* ppapi_host) override;
  bool MapUrlToLocalFilePath(const GURL& url,
                             bool is_blocking,
                             const base::FilePath& profile_directory,
                             base::FilePath* file_path) override;
  void SetDebugPatterns(const std::string& debug_patterns) override;
  bool URLMatchesDebugPatterns(const GURL& manifest_url) override;
  bool IsNonSfiModeAllowed(const base::FilePath& profile_directory,
                           const GURL& manifest_url) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(NaClBrowserDelegateImplEfl);
};


#endif  // COMPONENTS_NACL_BROWSER_NACL_BROWSER_DELEGATE_IMPL_EFL_H_
