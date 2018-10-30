// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ppapi/proxy/udp_socket_resource_base.h"

#include <cstring>

#include "base/logging.h"
#include "ppapi/c/pp_bool.h"
#include "ppapi/c/pp_errors.h"
#include "ppapi/proxy/error_conversion.h"
#include "ppapi/proxy/plugin_globals.h"
#include "ppapi/proxy/ppapi_messages.h"
#include "ppapi/proxy/udp_socket_resource_constants.h"
#include "ppapi/thunk/enter.h"

namespace ppapi {
namespace proxy {

namespace {

void RunCallback(scoped_refptr<TrackedCallback> callback,
                 int32_t pp_result,
                 bool private_api) {
  callback->Run(ConvertNetworkAPIErrorForCompatibility(pp_result, private_api));
}

void PostAbortIfNecessary(const scoped_refptr<TrackedCallback>& callback) {
  if (TrackedCallback::IsPending(callback))
    callback->PostAbort();
}

}  // namespace

UDPSocketResourceBase::UDPSocketResourceBase(Connection connection,
                                             PP_Instance instance,
                                             bool private_api)
    : PluginResource(connection, instance),
      private_api_(private_api),
      bind_called_(false),
      bound_(false),
      closed_(false),
      recv_filter_(PluginGlobals::Get()->udp_socket_filter()),
      bound_addr_() {
  recv_filter_->AddUDPResource(
      pp_instance(), pp_resource(), private_api,
      base::Bind(&UDPSocketResourceBase::SlotBecameAvailable, pp_resource()));
  if (private_api)
    SendCreate(BROWSER, PpapiHostMsg_UDPSocket_CreatePrivate());
  else
    SendCreate(BROWSER, PpapiHostMsg_UDPSocket_Create());
}

UDPSocketResourceBase::~UDPSocketResourceBase() {
  CloseImpl();
}

int32_t UDPSocketResourceBase::SetOptionImpl(
    PP_UDPSocket_Option name,
    const PP_Var& value,
    bool check_bind_state,
    scoped_refptr<TrackedCallback> callback) {
  if (closed_) {
    LOG(ERROR) << "UDP Socket is closed";
    return PP_ERROR_FAILED;
  }

// Backward compatibility with Tizen 2.4
#if defined(TIZEN_PEPPER_EXTENSIONS)
  switch (name) {
    case PP_UDPSOCKET_EXT_OPTION_MULTICAST_LOOP:
      name = PP_UDPSOCKET_OPTION_MULTICAST_LOOP;
      break;
    case PP_UDPSOCKET_EXT_OPTION_MULTICAST_TTL:
      name = PP_UDPSOCKET_OPTION_MULTICAST_TTL;
      break;
    default:
      break;
  }
#endif

  // Check if socket is expected to be bound or not according to the option.
  switch (name) {
    case PP_UDPSOCKET_OPTION_ADDRESS_REUSE:
    case PP_UDPSOCKET_OPTION_BROADCAST:
    case PP_UDPSOCKET_OPTION_MULTICAST_LOOP:
    case PP_UDPSOCKET_OPTION_MULTICAST_TTL: {
      if ((check_bind_state || name == PP_UDPSOCKET_OPTION_ADDRESS_REUSE) &&
          bind_called_) {
        // SetOption should fail in this case in order to give predictable
        // behavior while binding. Note that we use |bind_called_| rather
        // than |bound_| since the latter is only set on successful completion
        // of Bind().
        LOG(ERROR) << "Cannot set this option before Bind";
        return PP_ERROR_FAILED;
      }
      break;
    }
    case PP_UDPSOCKET_OPTION_SEND_BUFFER_SIZE:
    case PP_UDPSOCKET_OPTION_RECV_BUFFER_SIZE: {
      if (check_bind_state && !bound_) {
        LOG(ERROR) << "To use these options socket should be bound";
        return PP_ERROR_FAILED;
      }
      break;
    }
#if defined(TIZEN_PEPPER_EXTENSIONS)
    case PP_UDPSOCKET_EXT_OPTION_MULTICAST_LOOP:
    case PP_UDPSOCKET_EXT_OPTION_MULTICAST_TTL:
      NOTREACHED();
      break;
#endif
    default:
      NOTREACHED();
      break;
  }

  SocketOptionData option_data;
  switch (name) {
    case PP_UDPSOCKET_OPTION_ADDRESS_REUSE:
    case PP_UDPSOCKET_OPTION_BROADCAST:
    case PP_UDPSOCKET_OPTION_MULTICAST_LOOP: {
      if (value.type != PP_VARTYPE_BOOL) {
        LOG(ERROR) << "Wrong value type";
        return PP_ERROR_BADARGUMENT;
      }
      option_data.SetBool(PP_ToBool(value.value.as_bool));
      break;
    }
    case PP_UDPSOCKET_OPTION_SEND_BUFFER_SIZE:
    case PP_UDPSOCKET_OPTION_RECV_BUFFER_SIZE: {
      if (value.type != PP_VARTYPE_INT32) {
        LOG(ERROR) << "Wrong value type";
        return PP_ERROR_BADARGUMENT;
      }
      option_data.SetInt32(value.value.as_int);
      break;
    }
    case PP_UDPSOCKET_OPTION_MULTICAST_TTL: {
      int32_t ival = value.value.as_int;
      if (value.type != PP_VARTYPE_INT32 && (ival < 0 || ival > 255)) {
        LOG(ERROR) << "Wrong value type";
        return PP_ERROR_BADARGUMENT;
      }
      option_data.SetInt32(ival);
      break;
    }
    default: {
      LOG(ERROR);
      NOTREACHED();
      return PP_ERROR_BADARGUMENT;
    }
  }

  Call<PpapiPluginMsg_UDPSocket_SetOptionReply>(
      BROWSER,
      PpapiHostMsg_UDPSocket_SetOption(name, option_data),
      base::Bind(&UDPSocketResourceBase::OnPluginMsgGeneralReply,
                 base::Unretained(this),
                 callback),
      callback);
  return PP_OK_COMPLETIONPENDING;
}

int32_t UDPSocketResourceBase::BindImpl(
    const PP_NetAddress_Private* addr,
    scoped_refptr<TrackedCallback> callback) {
  if (!addr) {
    LOG(ERROR) << "Address is NULL";
    return PP_ERROR_BADARGUMENT;
  }
  if (bound_ || closed_) {
    LOG(ERROR) << "Socket already bound or closed";
    return PP_ERROR_FAILED;
  }
  if (TrackedCallback::IsPending(bind_callback_)) {
    LOG(ERROR) << "Bind is pending";
    return PP_ERROR_INPROGRESS;
  }

  bind_called_ = true;
  bind_callback_ = callback;

  // Send the request, the browser will call us back via BindReply.
  Call<PpapiPluginMsg_UDPSocket_BindReply>(
      BROWSER,
      PpapiHostMsg_UDPSocket_Bind(*addr),
      base::Bind(&UDPSocketResourceBase::OnPluginMsgBindReply,
                 base::Unretained(this)),
      callback);
  return PP_OK_COMPLETIONPENDING;
}

PP_Bool UDPSocketResourceBase::GetBoundAddressImpl(
    PP_NetAddress_Private* addr) {
  if (!addr || !bound_ || closed_)
    return PP_FALSE;

  *addr = bound_addr_;
  return PP_TRUE;
}

int32_t UDPSocketResourceBase::RecvFromImpl(
    char* buffer_out,
    int32_t num_bytes,
    PP_Resource* addr,
    scoped_refptr<TrackedCallback> callback) {
  if (!bound_) {
    LOG(ERROR) << "Socket not bound";
    return PP_ERROR_FAILED;
  }
  return recv_filter_->RequestData(pp_resource(), num_bytes, buffer_out, addr,
                                   callback);
}

PP_Bool UDPSocketResourceBase::GetRecvFromAddressImpl(
    PP_NetAddress_Private* addr) {
  if (!addr)
    return PP_FALSE;
  *addr = recv_filter_->GetLastAddrPrivate(pp_resource());
  return PP_TRUE;
}

int32_t UDPSocketResourceBase::SendToImpl(
    const char* buffer,
    int32_t num_bytes,
    const PP_NetAddress_Private* addr,
    scoped_refptr<TrackedCallback> callback) {
  if (!buffer || num_bytes <= 0 || !addr) {
    LOG(ERROR) << "Buffer is NULL or Address is NULL or num_bytes <=0";
    return PP_ERROR_BADARGUMENT;
  }
  if (!bound_) {
    LOG(ERROR) << "Socket not bound";
    return PP_ERROR_FAILED;
  }
  if (sendto_callbacks_.size() ==
      UDPSocketResourceConstants::kPluginSendBufferSlots) {
    LOG(ERROR) << "SendTo size cannot be equal to kPluginSendBufferSlots";
    return PP_ERROR_INPROGRESS;
  }

  if (num_bytes > UDPSocketResourceConstants::kMaxWriteSize)
    num_bytes = UDPSocketResourceConstants::kMaxWriteSize;

  sendto_callbacks_.push(callback);

  // Send the request, the browser will call us back via SendToReply.
  Call<PpapiPluginMsg_UDPSocket_SendToReply>(
      BROWSER,
      PpapiHostMsg_UDPSocket_SendTo(std::string(buffer, num_bytes), *addr),
      base::Bind(&UDPSocketResourceBase::OnPluginMsgSendToReply,
                 base::Unretained(this)),
      callback);
  return PP_OK_COMPLETIONPENDING;
}

void UDPSocketResourceBase::CloseImpl() {
  if(closed_)
    return;

  bound_ = false;
  closed_ = true;

  Post(BROWSER, PpapiHostMsg_UDPSocket_Close());

  PostAbortIfNecessary(bind_callback_);
  while (!sendto_callbacks_.empty()) {
    scoped_refptr<TrackedCallback> callback = sendto_callbacks_.front();
    sendto_callbacks_.pop();
    PostAbortIfNecessary(callback);
  }
  recv_filter_->RemoveUDPResource(pp_resource());
}

int32_t UDPSocketResourceBase::JoinGroupImpl(
    const PP_NetAddress_Private *group,
    scoped_refptr<TrackedCallback> callback) {
  DCHECK(group);

  Call<PpapiPluginMsg_UDPSocket_JoinGroupReply>(
      BROWSER,
      PpapiHostMsg_UDPSocket_JoinGroup(*group),
      base::Bind(&UDPSocketResourceBase::OnPluginMsgGeneralReply,
                 base::Unretained(this),
                 callback),
      callback);
  return PP_OK_COMPLETIONPENDING;
}

int32_t UDPSocketResourceBase::LeaveGroupImpl(
    const PP_NetAddress_Private *group,
    scoped_refptr<TrackedCallback> callback) {
  DCHECK(group);

  Call<PpapiPluginMsg_UDPSocket_LeaveGroupReply>(
      BROWSER,
      PpapiHostMsg_UDPSocket_LeaveGroup(*group),
      base::Bind(&UDPSocketResourceBase::OnPluginMsgGeneralReply,
                 base::Unretained(this),
                 callback),
      callback);
  return PP_OK_COMPLETIONPENDING;
}

void UDPSocketResourceBase::OnPluginMsgGeneralReply(
    scoped_refptr<TrackedCallback> callback,
    const ResourceMessageReplyParams& params) {
  if (TrackedCallback::IsPending(callback))
    RunCallback(callback, params.result(), private_api_);
}

void UDPSocketResourceBase::OnPluginMsgBindReply(
    const ResourceMessageReplyParams& params,
    const PP_NetAddress_Private& bound_addr) {
  // It is possible that |bind_callback_| is pending while |closed_| is true:
  // CloseImpl() has been called, but a BindReply came earlier than the task to
  // abort |bind_callback_|. We don't want to update |bound_| or |bound_addr_|
  // in that case.
  if (!TrackedCallback::IsPending(bind_callback_) || closed_)
    return;

  if (params.result() == PP_OK)
    bound_ = true;
  bound_addr_ = bound_addr;
  RunCallback(bind_callback_, params.result(), private_api_);
}

void UDPSocketResourceBase::OnPluginMsgSendToReply(
    const ResourceMessageReplyParams& params,
    int32_t bytes_written) {
  // This can be empty if the socket was closed, but there are still tasks
  // to be posted for this resource.
  if (sendto_callbacks_.empty())
    return;

  scoped_refptr<TrackedCallback> callback = sendto_callbacks_.front();
  sendto_callbacks_.pop();
  if (!TrackedCallback::IsPending(callback))
    return;

  if (params.result() == PP_OK)
    RunCallback(callback, bytes_written, private_api_);
  else
    RunCallback(callback, params.result(), private_api_);
}

// static
void UDPSocketResourceBase::SlotBecameAvailable(PP_Resource resource) {
  ProxyLock::AssertAcquired();
  UDPSocketResourceBase* thiz = nullptr;
  // We have to try to enter all subclasses of UDPSocketResourceBase. Currently,
  // these are the public and private resources.
  thunk::EnterResourceNoLock<thunk::PPB_UDPSocket_API> enter(resource, false);
  if (enter.succeeded()) {
    thiz = static_cast<UDPSocketResourceBase*>(enter.resource());
  } else {
    thunk::EnterResourceNoLock<thunk::PPB_UDPSocket_Private_API> enter_private(
        resource, false);
    if (enter_private.succeeded())
      thiz = static_cast<UDPSocketResourceBase*>(enter_private.resource());
  }

  if (thiz && !thiz->closed_)
    thiz->Post(BROWSER, PpapiHostMsg_UDPSocket_RecvSlotAvailable());
}

#if defined(TIZEN_PEPPER_EXTENSIONS)
int32_t UDPSocketResourceBase::GetOptionImpl(
    PP_UDPSocket_Option name,
    struct PP_Var* value,
    scoped_refptr<TrackedCallback> callback) {
  if (!value) {
    LOG(ERROR) << "value is NULL";
    return PP_ERROR_BADARGUMENT;
  }

  if (TrackedCallback::IsPending(get_option_callback_)) {
    LOG(ERROR) << "GetOption is pending";
    return PP_ERROR_INPROGRESS;
  }

  get_option_callback_ = callback;

  // Backward compatibility with Tizen 2.4
#if defined(TIZEN_PEPPER_EXTENSIONS)
  switch (name) {
    case PP_UDPSOCKET_EXT_OPTION_MULTICAST_LOOP:
      name = PP_UDPSOCKET_OPTION_MULTICAST_LOOP;
      break;
    case PP_UDPSOCKET_EXT_OPTION_MULTICAST_TTL:
      name = PP_UDPSOCKET_OPTION_MULTICAST_TTL;
      break;
    default:
      break;
  }
#endif

  // Send the request, the browser will call us back via BindReply.
  Call<PpapiPluginMsg_UDPSocket_GetOptionReply>(
      BROWSER, PpapiHostMsg_UDPSocket_GetOption(name),
      base::Bind(&UDPSocketResourceBase::OnPluginMsgGetOptionReply,
                 base::Unretained(this), value),
      callback);
  return PP_OK_COMPLETIONPENDING;
}

void UDPSocketResourceBase::OnPluginMsgGetOptionReply(
    PP_Var* return_value,
    const ResourceMessageReplyParams& params,
    const SocketOptionData& socket_option_data) {
  if (!TrackedCallback::IsPending(get_option_callback_)) {
    LOG(ERROR) << "GetOption not pending";
    return;
  }

  if (params.result() == PP_OK) {
    switch (socket_option_data.GetType()) {
    case SocketOptionData::TYPE_BOOL: {
      bool result;
      socket_option_data.GetBool(&result);
      *return_value = PP_MakeBool(PP_FromBool(result));
      break;
    }
    case SocketOptionData::TYPE_INT32: {
      int32_t result;
      socket_option_data.GetInt32(&result);
      *return_value = PP_MakeInt32(result);
      break;
    }
    default:
      LOG(ERROR) << "Received invalid socket option data";
      NOTREACHED();
      break;
    }
  }

  RunCallback(get_option_callback_, params.result(), private_api_);
}
#endif

}  // namespace proxy
}  // namespace ppapi
