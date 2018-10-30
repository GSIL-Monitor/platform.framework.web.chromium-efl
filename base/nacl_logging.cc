// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#if defined(OS_TIZEN_TV_PRODUCT)

#include "components/nacl/zygote/nacl_fork_custom_resources_tizen.h"

#include "base/nacl_logging.h"

#include "native_client/src/public/chrome_main.h"

#if !defined(OS_NACL)
#include "base/logging.h"
#include "native_client/src/public/nacl_desc_custom.h"
#include "native_client/src/trusted/service_runtime/include/sys/errno.h"
#endif  // !defined(OS_NACL)

#if defined(OS_NACL) && !defined(OS_NACL_NONSFI)
#include "native_client/src/include/build_config.h"
#include "native_client/src/public/imc_syscalls.h"
#include "native_client/src/public/imc_types.h"
#endif  // defined(OS_NACL)

namespace logging {
namespace {
#if !defined(OS_NACL_NONSFI)
// Chromium is using not named file descriptors, calculated by using
// variable NACL_CHROME_DESC_BASE as base for it.
// Currently the highest non named descriptor for NaCl, which is calculated
// is NACL_CHROME_DESC_BASE + 2.
// Using following assumption, we are giving our selfs a small error margin
// of 5 descriptors for Tizen custom descriptors.
static const int KLogIMCDesc = NACL_CHROME_DESC_BASE + 5;
#endif  // !defined(OS_NACL_NONSFI)

#if !defined(OS_NACL)
static bool g_nacl_logging_enabled = false;

void NaClLoggingDescDestroy(void* handle) {}

ssize_t NaClLoggingDescSendMsg(void* handle,
                               const struct NaClImcTypedMsgHdr* msg,
                               int flags) {
  if (msg->iov_length != 1)
    return -NACL_ABI_EINVAL;

  char* srcBuffer = static_cast<char*>(msg->iov[0].base);
  size_t srcBufferSize = msg->iov[0].length;
  if (srcBufferSize < 2)
    return -NACL_ABI_EINVAL;

  // The string must be null-terminated, enforce it for safety since the data
  // comes from untrusted code.
  srcBuffer[srcBufferSize - 1] = '\0';
  int level = static_cast<int>(srcBuffer[0]);
  ++srcBuffer;
  if (g_nacl_logging_enabled)
    logging::LogMessage::DlogLogMessage(level, srcBuffer);
  return srcBufferSize;
}

ssize_t NaClLoggingDescRecvMsg(void* handle,
                               struct NaClImcTypedMsgHdr* msg,
                               int flags) {
  return -NACL_ABI_EOPNOTSUPP;
}

#endif  // !defined(OS_NACL)
}  // namespace

#if defined(OS_NACL) && !defined(OS_NACL_NONSFI)
bool NaClLogMessageHandler(int severity, const std::string& str) {
  // +1 for log level and +1 for null character.
  char* buffer = new char[str.length() + 2];
  // First byte is severity level, the rest is log message.
  buffer[0] = severity;
  memcpy(buffer + 1, str.c_str(), str.length());
  NaClAbiNaClImcMsgHdr message;
  memset(&message, 0, sizeof(message));
  NaClAbiNaClImcMsgIoVec iov[1];
  memset(&iov, 0, sizeof(iov));
  message.iov = iov;
  message.iov_length = 1;
  iov[0].base = buffer;
  // +1 for log level and +1 for null character (for safety will be
  // forcefully added on trusted size).
  iov[0].length = str.length() + 2;
  int result = imc_sendmsg(KLogIMCDesc, &message, 0);
  delete[] buffer;
  return result > -1;
}
#endif

#if !defined(OS_NACL)
void SetNaClLoggingEnabled(bool enabled) {
  g_nacl_logging_enabled = enabled;
}

void InitializeNaClLogging(NaClApp* nap) {
  NaClDescCustomFuncs funcs = NACL_DESC_CUSTOM_FUNCS_INITIALIZER;
  funcs.Destroy = NaClLoggingDescDestroy;
  funcs.SendMsg = NaClLoggingDescSendMsg;
  funcs.RecvMsg = NaClLoggingDescRecvMsg;
  NaClDesc* desc = NaClDescMakeCustomDesc(nullptr, &funcs);
  NaClAppSetDesc(nap, KLogIMCDesc, desc);
}
#endif  // !defined(OS_NACL)

}  // namespace logging

#endif  // defined(OS_TIZEN_TV_PRODUCT)
