// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVTOOLS_DELEGATE_EFL_H_
#define DEVTOOLS_DELEGATE_EFL_H_

#include "base/compiler_specific.h"
#include "content/browser/devtools/devtools_http_handler.h"
#include "content/public/browser/devtools_manager_delegate.h"

namespace content {

class BrowserContext;
class DevToolsHttpHandler;

// This class is to create RemoteInspector Server(Remote Debugger) and return devtools front resources.
// This class implements DevToolsHttpHandlerDelegate interface.
// This class is similar to ShellDevToolsDelegate, which also implements DevToolsHttpHandlerDelegate interface.
class DevToolsDelegateEfl : public DevToolsManagerDelegate {
 public:
  // explicit ChromiumEflDevToolsDelegate();
  /* LCOV_EXCL_START */
  explicit DevToolsDelegateEfl() {}
  virtual ~DevToolsDelegateEfl() {}
  /* LCOV_EXCL_STOP */

  static int Start(int port = 0);
  static bool Stop();

  // ChromiumDevToolsHttpHandler::Delegate overrides.
  virtual std::string GetDiscoveryPageHTML() override;
  virtual std::string GetFrontendResource(const std::string& path) override;
#if defined(OS_TIZEN_TV_PRODUCT)
  static uint16_t getPort() { return port_; }
  void ReleasePort();
#endif

 private:
  static uint16_t port_;
};

} // namespace content

#endif  // DEVTOOLS_DELEGATE_EFL_H_
