// Copyright (c) 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PPAPI_CPP_SAMSUNG_SOCKET_EXTENSION_SAMSUNG_H_
#define PPAPI_CPP_SAMSUNG_SOCKET_EXTENSION_SAMSUNG_H_

#include "ppapi/c/samsung/ppb_udp_socket_extension_samsung.h"
#include "ppapi/cpp/udp_socket.h"

/// @file
/// This file defines the API used to handle gesture input events.
/// This file defines the API providing additional sockets functionality
/// for PPB_UDPSocket.
/// To use this interface, one have to create standard UDPSocket object.
///
/// This API is deprecated since Samsung Pepper 47.
/// Please use PPB_UDPSocket API instead.
namespace pp {

class UDPSocketExtensionSamsung : public UDPSocket {
 public:
  /// Constructs an is_null() gesture input event object.
  UDPSocketExtensionSamsung();

  /// This constructor constructs a extended socket object from the
  /// provided generic UDP socket. If the given socket is itself
  /// is_null() or is not a UDPSocket, the extended socket object will be
  /// is_null().
  ///
  /// @param[in] socket A generic UDP socket instance.
  explicit UDPSocketExtensionSamsung(const InstanceHandle& instance);

  /// Get a socket option on the associated socket.
  /// Please see the <code>PP_UDPSocket_Option</code> description for option
  /// names, value types and allowed values.
  /// Socket need to be already bound and method will fail with
  /// <code>PP_ERROR_FAILED</code> if it's not.
  ///
  /// @param[in] udp_socket A <code>PP_Resource</code> corresponding to a UDP
  /// socket.
  /// @param[in] name The option to get.
  /// @param[out] Value of requested option.
  /// @param[in] callback A <code>PP_CompletionCallback</code> to be called upon
  /// completion.
  ///
  /// @return An int32_t containing an error code from <code>pp_errors.h</code>
  int32_t GetOption(PP_UDPSocket_Option name,
                    const CompletionCallbackWithOutput<Var>& callback);

  /// Join a multicast group. Use optional <code>source_addr</code> to allow
  /// receiving data only from a specified source. This option can be used
  /// multiple times to allow receiving data from more than one source.
  /// Socket need to be already bound and method will fail with
  /// <code>PP_ERROR_FAILED</code> if it's not.
  ///
  /// @param[in] udp_socket A <code>PP_Resource</code> corresponding to a UDP
  /// socket.
  /// @param[in] membership A <code>PP_MulticastMembership</code> multicast
  /// group and optional source IP.
  /// @param[in] callback  A <code>PP_CompletionCallback</code> to be called
  /// upon completion.
  ///
  /// @return An int32_t containing an error code from <code>pp_errors.h</code>
  int32_t JoinMulticast(const struct PP_MulticastMembership& membership,
                        const CompletionCallback& callback);

  /// Leave a multicast group. Use optional member <code>source_addr</code>
  /// to stop receiving data from multicast group that come from a given source.
  /// If the application has subscribed to multiple sources within the same
  /// group, data from the remaining sources will still be delivered.
  /// Socket need to be already bound and method will fail with
  /// <code>PP_ERROR_FAILED</code> if it's not.
  ///
  /// @param[in] udp_socket A <code>PP_Resource</code> corresponding to a UDP
  /// socket.
  /// @param[in] membership A <code>PP_MulticastMembership</code> multicast
  /// group and optional source IP.
  /// @param[in] callback A <code>PP_CompletionCallback</code> to be called upon
  /// completion.
  ///
  /// @return An int32_t containing an error code from <code>pp_errors.h</code>
  int32_t LeaveMulticast(const struct PP_MulticastMembership& membership,
                         const CompletionCallback& callback);
};

}  // namespace pp

#endif  // PPAPI_CPP_SAMSUNG_SOCKET_EXTENSION_SAMSUNG_H_

