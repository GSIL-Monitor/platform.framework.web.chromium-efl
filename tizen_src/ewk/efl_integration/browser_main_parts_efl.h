// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_MAIN_PARTS_EFL_H_
#define CONTENT_BROWSER_MAIN_PARTS_EFL_H_

#if defined(OS_TIZEN_TV_PRODUCT)
#include <memory>
#endif

#include "base/macros.h"
#include "content/public/browser/browser_main_parts.h"

namespace content {

class DevToolsDelegateEfl;
#if defined(OS_TIZEN_TV_PRODUCT)
class IOThreadDelegateEfl;
#endif

// EFL port implementation of BrowserMainParts.
// This class contains different "stages" to be executed by BrowserMain(),
// Each stage is represented by a single BrowserMainParts method, called from
// the corresponding method in BrowserMainLoop.
class BrowserMainPartsEfl : public BrowserMainParts {
 public:
  explicit BrowserMainPartsEfl() {}
  virtual ~BrowserMainPartsEfl() {}

  // BrowserMainParts overrides.
  virtual void PreMainMessageLoopRun() override;
  virtual void PostMainMessageLoopRun() override;
  int PreCreateThreads() override;
#if defined(OS_TIZEN_TV_PRODUCT)
  void PostDestroyThreads() override;
#endif

 private:
#if defined(OS_TIZEN_TV_PRODUCT)
  std::unique_ptr<IOThreadDelegateEfl> io_thread_delegate_;
#endif

  DISALLOW_COPY_AND_ASSIGN(BrowserMainPartsEfl);
};

}  // namespace content

#endif  // CONTENT_BROWSER_MAIN_PARTS_EFL_H_
