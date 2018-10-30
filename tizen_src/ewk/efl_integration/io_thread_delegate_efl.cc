// Copyright 2018 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "io_thread_delegate_efl.h"
#include "content/public/browser/browser_thread.h"

#if defined(USE_NSS_CERTS)
#include "net/cert_net/nss_ocsp.h"
#endif

namespace content {

IOThreadDelegateEfl::IOThreadDelegateEfl() {
  BrowserThread::SetIOThreadDelegate(this);
}

IOThreadDelegateEfl::~IOThreadDelegateEfl() {
  BrowserThread::SetIOThreadDelegate(NULL);
}

void IOThreadDelegateEfl::Init() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

#if defined(USE_NSS_CERTS)
  net::SetMessageLoopForNSSHttpIO();
#endif
}

void IOThreadDelegateEfl::CleanUp() {
#if defined(USE_NSS_CERTS)
  net::ShutdownNSSHttpIO();
#endif
}
}  // namespace content
