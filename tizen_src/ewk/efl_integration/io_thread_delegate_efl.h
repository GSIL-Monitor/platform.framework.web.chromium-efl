// Copyright 2018 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_IO_THREAD_DELEGATE_EFL_H_
#define CONTENT_IO_THREAD_DELEGATE_EFL_H_

#include "content/public/browser/browser_thread_delegate.h"

namespace content {

class IOThreadDelegateEfl : public content::BrowserThreadDelegate {
 public:
  IOThreadDelegateEfl();
  ~IOThreadDelegateEfl() override;

  // BrowserThreadDelegate implementation, runs on the IO thread.
  // This handles initialization and destruction of state that must
  // live on the IO thread.
  void Init() override;
  void CleanUp() override;
};

}  // namespace content

#endif  // CONTENT_IO_THREAD_DELEGATE_EFL_H_
