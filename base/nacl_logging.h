// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_NACL_LOGGING_H_
#define BASE_NACL_LOGGING_H_

#if defined(OS_TIZEN_TV_PRODUCT)
#include <string>
#include "native_client/src/public/nacl_app.h"

namespace logging {

#if !defined(OS_NACL)
void SetNaClLoggingEnabled(bool enabled);
void InitializeNaClLogging(NaClApp* nap);
#elif !defined(OS_NACL_NONSFI)
bool NaClLogMessageHandler(int severity, const std::string&);
#endif  // !defined(OS_NACL)
}  // namespace logging
#endif  // defined(OS_TIZEN_TV_PRODUCT)

#endif  // BASE_NACL_LOGGING_H_
