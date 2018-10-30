// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// From ppb_udp_socket.idl modified Wed Jan 27 17:10:16 2016.

#include <stdint.h>

#include "ppapi/c/pp_completion_callback.h"
#include "ppapi/c/pp_errors.h"
#include "ppapi/c/ppb_udp_socket.h"
#include "ppapi/shared_impl/tracked_callback.h"
#include "ppapi/thunk/enter.h"
#include "ppapi/thunk/ppapi_thunk_export.h"
#include "ppapi/thunk/ppb_udp_socket_api.h"

#if defined(TIZEN_PEPPER_EXTENSIONS)
#include "ppapi/c/samsung/ppb_udp_socket_extension_samsung.h"
#endif

namespace ppapi {
namespace thunk {

namespace {

PP_Resource Create(PP_Instance instance) {
  VLOG(4) << "PPB_UDPSocket::Create()";
  EnterResourceCreation enter(instance);
  if (enter.failed())
    return 0;
  return enter.functions()->CreateUDPSocket(instance);
}

PP_Bool IsUDPSocket(PP_Resource resource) {
  VLOG(4) << "PPB_UDPSocket::IsUDPSocket()";
  EnterResource<PPB_UDPSocket_API> enter(resource, false);
  return PP_FromBool(enter.succeeded());
}

int32_t Bind(PP_Resource udp_socket,
             PP_Resource addr,
             struct PP_CompletionCallback callback) {
  VLOG(4) << "PPB_UDPSocket::Bind()";
  EnterResource<PPB_UDPSocket_API> enter(udp_socket, callback, true);
  if (enter.failed())
    return enter.retval();
  return enter.SetResult(enter.object()->Bind(addr, enter.callback()));
}

PP_Resource GetBoundAddress(PP_Resource udp_socket) {
  VLOG(4) << "PPB_UDPSocket::GetBoundAddress()";
  EnterResource<PPB_UDPSocket_API> enter(udp_socket, true);
  if (enter.failed())
    return 0;
  return enter.object()->GetBoundAddress();
}

int32_t RecvFrom(PP_Resource udp_socket,
                 char* buffer,
                 int32_t num_bytes,
                 PP_Resource* addr,
                 struct PP_CompletionCallback callback) {
  VLOG(4) << "PPB_UDPSocket::RecvFrom()";
  EnterResource<PPB_UDPSocket_API> enter(udp_socket, callback, true);
  if (enter.failed())
    return enter.retval();
  return enter.SetResult(
      enter.object()->RecvFrom(buffer, num_bytes, addr, enter.callback()));
}

int32_t SendTo(PP_Resource udp_socket,
               const char* buffer,
               int32_t num_bytes,
               PP_Resource addr,
               struct PP_CompletionCallback callback) {
  VLOG(4) << "PPB_UDPSocket::SendTo()";
  EnterResource<PPB_UDPSocket_API> enter(udp_socket, callback, true);
  if (enter.failed())
    return enter.retval();
  return enter.SetResult(
      enter.object()->SendTo(buffer, num_bytes, addr, enter.callback()));
}

void Close(PP_Resource udp_socket) {
  VLOG(4) << "PPB_UDPSocket::Close()";
  EnterResource<PPB_UDPSocket_API> enter(udp_socket, true);
  if (enter.failed())
    return;
  enter.object()->Close();
}

int32_t SetOption_1_0(PP_Resource udp_socket,
                      PP_UDPSocket_Option name,
                      struct PP_Var value,
                      struct PP_CompletionCallback callback) {
  VLOG(4) << "PPB_UDPSocket::SetOption_1_0()";
  EnterResource<PPB_UDPSocket_API> enter(udp_socket, callback, true);
  if (enter.failed())
    return enter.retval();
  return enter.SetResult(
      enter.object()->SetOption1_0(name, value, enter.callback()));
}

int32_t SetOption_1_1(PP_Resource udp_socket,
                      PP_UDPSocket_Option name,
                      struct PP_Var value,
                      struct PP_CompletionCallback callback) {
  VLOG(4) << "PPB_UDPSocket::SetOption_1_1()";
  EnterResource<PPB_UDPSocket_API> enter(udp_socket, callback, true);
  if (enter.failed())
    return enter.retval();
  return enter.SetResult(
      enter.object()->SetOption1_1(name, value, enter.callback()));
}

int32_t SetOption(PP_Resource udp_socket,
                  PP_UDPSocket_Option name,
                  struct PP_Var value,
                  struct PP_CompletionCallback callback) {
  VLOG(4) << "PPB_UDPSocket::SetOption()";
  EnterResource<PPB_UDPSocket_API> enter(udp_socket, callback, true);
  if (enter.failed())
    return enter.retval();
  return enter.SetResult(
      enter.object()->SetOption(name, value, enter.callback()));
}

int32_t JoinGroup(PP_Resource udp_socket,
                  PP_Resource group,
                  struct PP_CompletionCallback callback) {
  VLOG(4) << "PPB_UDPSocket::JoinGroup()";
  EnterResource<PPB_UDPSocket_API> enter(udp_socket, callback, true);
  if (enter.failed())
    return enter.retval();
  return enter.SetResult(enter.object()->JoinGroup(group, enter.callback()));
}

int32_t LeaveGroup(PP_Resource udp_socket,
                   PP_Resource group,
                   struct PP_CompletionCallback callback) {
  VLOG(4) << "PPB_UDPSocket::LeaveGroup()";
  EnterResource<PPB_UDPSocket_API> enter(udp_socket, callback, true);
  if (enter.failed())
    return enter.retval();
  return enter.SetResult(enter.object()->LeaveGroup(group, enter.callback()));
}

#if defined(TIZEN_PEPPER_EXTENSIONS)
int32_t GetOptionExtension(PP_Resource udp_socket,
                           PP_UDPSocket_Option name,
                           struct PP_Var* value,
                           struct PP_CompletionCallback callback) {
  VLOG(4) << "PPB_UDPSocket::GetOptionExtension()";
  EnterResource<PPB_UDPSocket_API> enter(udp_socket, callback, true);
  if (enter.failed())
    return enter.retval();
  return enter.SetResult(
      enter.object()->GetOption(name, value, enter.callback()));
}

int32_t JoinGroupExtension(PP_Resource udp_socket,
                           const struct PP_MulticastMembership* membership,
                           struct PP_CompletionCallback callback) {
  VLOG(4) << "PPB_UDPSocket::JoinGroupExtension()";
  EnterResource<PPB_UDPSocket_API> enter(udp_socket, callback, true);
  if (enter.failed())
    return enter.retval();
  return enter.SetResult(
      enter.object()->JoinGroup(membership->interface_addr, enter.callback()));
}

int32_t LeaveGroupExtension(PP_Resource udp_socket,
                            const struct PP_MulticastMembership* membership,
                            struct PP_CompletionCallback callback) {
  VLOG(4) << "PPB_UDPSocket::LeaveGroupExtension()";
  EnterResource<PPB_UDPSocket_API> enter(udp_socket, callback, true);
  if (enter.failed())
    return enter.retval();
  return enter.SetResult(
      enter.object()->LeaveGroup(membership->interface_addr, enter.callback()));
}
#endif

const PPB_UDPSocket_1_0 g_ppb_udpsocket_thunk_1_0 = {
    &Create,   &IsUDPSocket, &Bind,  &GetBoundAddress,
    &RecvFrom, &SendTo,      &Close, &SetOption_1_0};

const PPB_UDPSocket_1_1 g_ppb_udpsocket_thunk_1_1 = {
    &Create,   &IsUDPSocket, &Bind,  &GetBoundAddress,
    &RecvFrom, &SendTo,      &Close, &SetOption_1_1};

const PPB_UDPSocket_1_2 g_ppb_udpsocket_thunk_1_2 = {
    &Create, &IsUDPSocket, &Bind,      &GetBoundAddress, &RecvFrom,
    &SendTo, &Close,       &SetOption, &JoinGroup,       &LeaveGroup};

#if defined(TIZEN_PEPPER_EXTENSIONS)
const PPB_UDPSocketExtension_Samsung_0_1
    g_ppb_udpsocket_extension_samsung_thunk_0_1 = {
        &GetOptionExtension, &JoinGroupExtension, &LeaveGroupExtension};
#endif

}  // namespace

PPAPI_THUNK_EXPORT const PPB_UDPSocket_1_0* GetPPB_UDPSocket_1_0_Thunk() {
  return &g_ppb_udpsocket_thunk_1_0;
}

PPAPI_THUNK_EXPORT const PPB_UDPSocket_1_1* GetPPB_UDPSocket_1_1_Thunk() {
  return &g_ppb_udpsocket_thunk_1_1;
}

PPAPI_THUNK_EXPORT const PPB_UDPSocket_1_2* GetPPB_UDPSocket_1_2_Thunk() {
  return &g_ppb_udpsocket_thunk_1_2;
}

#if defined(TIZEN_PEPPER_EXTENSIONS)
PPAPI_THUNK_EXPORT const PPB_UDPSocketExtension_Samsung_0_1*
GetPPB_UDPSocketExtension_Samsung_0_1_Thunk() {
  return &g_ppb_udpsocket_extension_samsung_thunk_0_1;
}
#endif

}  // namespace thunk
}  // namespace ppapi
