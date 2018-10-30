// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PPAPI_THUNK_PPB_UDP_SOCKET_API_H_
#define PPAPI_THUNK_PPB_UDP_SOCKET_API_H_

#include <stdint.h>

#include "base/memory/ref_counted.h"
#include "ppapi/c/ppb_udp_socket.h"
#include "ppapi/thunk/ppapi_thunk_export.h"

#if defined(TIZEN_PEPPER_EXTENSIONS)
#include "ppapi/c/samsung/ppb_udp_socket_extension_samsung.h"
#endif

namespace ppapi {

class TrackedCallback;

namespace thunk {

class PPAPI_THUNK_EXPORT PPB_UDPSocket_API {
 public:
  virtual ~PPB_UDPSocket_API() {}

  virtual int32_t Bind(PP_Resource addr,
                       scoped_refptr<TrackedCallback> callback) = 0;
  virtual PP_Resource GetBoundAddress() = 0;
  virtual int32_t RecvFrom(char* buffer,
                           int32_t num_bytes,
                           PP_Resource* addr,
                           scoped_refptr<TrackedCallback> callback) = 0;
  virtual int32_t SendTo(const char* buffer,
                         int32_t num_bytes,
                         PP_Resource addr,
                         scoped_refptr<TrackedCallback> callback) = 0;
  virtual void Close() = 0;
  virtual int32_t SetOption1_0(PP_UDPSocket_Option name,
                               const PP_Var& value,
                               scoped_refptr<TrackedCallback> callback) = 0;
  virtual int32_t SetOption1_1(PP_UDPSocket_Option name,
                               const PP_Var& value,
                               scoped_refptr<TrackedCallback> callback) = 0;
  virtual int32_t SetOption(PP_UDPSocket_Option name,
                            const PP_Var& value,
                            scoped_refptr<TrackedCallback> callback) = 0;
  virtual int32_t JoinGroup(PP_Resource group,
                            scoped_refptr<TrackedCallback> callback) = 0;
  virtual int32_t LeaveGroup(PP_Resource group,
                            scoped_refptr<TrackedCallback> callback) = 0;
#if defined(TIZEN_PEPPER_EXTENSIONS)
  virtual int32_t GetOption(PP_UDPSocket_Option name,
                            struct PP_Var* value,
                            scoped_refptr<TrackedCallback> callback) = 0;
#endif
};

}  // namespace thunk
}  // namespace ppapi

#endif  // PPAPI_THUNK_PPB_UDP_SOCKET_API_H_
