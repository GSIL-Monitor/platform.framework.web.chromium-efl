/* Copyright (c) 2015 Samsung Electronics. All rights reserved.
 */

/* From samsung/ppb_udp_socket_extension_samsung.idl,
 *   modified Fri Jul 28 16:25:06 2017.
 */

#ifndef PPAPI_C_SAMSUNG_PPB_UDP_SOCKET_EXTENSION_SAMSUNG_H_
#define PPAPI_C_SAMSUNG_PPB_UDP_SOCKET_EXTENSION_SAMSUNG_H_

#include "ppapi/c/pp_bool.h"
#include "ppapi/c/pp_completion_callback.h"
#include "ppapi/c/pp_macros.h"
#include "ppapi/c/pp_resource.h"
#include "ppapi/c/pp_stdint.h"
#include "ppapi/c/pp_var.h"
#include "ppapi/c/ppb_udp_socket.h"

#define PPB_UDPSOCKETEXTENSION_SAMSUNG_INTERFACE_0_1 \
    "PPB_UDPSocketExtension_Samsung;0.1"
#define PPB_UDPSOCKETEXTENSION_SAMSUNG_INTERFACE \
    PPB_UDPSOCKETEXTENSION_SAMSUNG_INTERFACE_0_1

/**
 * @file
 * This file defines the <code>PPB_UDPSocketExtension_Samsung</code> interface.
 */


/**
 * @addtogroup Structs
 * @{
 */
/**
 * Multicast membership addresses.
 */
struct PP_MulticastMembership {
  /**
   * A <code>PPB_NetAddress</code> resource holding the IP multicast group
   * address.
   */
  PP_Resource multicast_addr;
  /**
   * A <code>PPB_NetAddress</code> resource holding the IP address of local
   * interface.
   */
  PP_Resource interface_addr;
  /**
   * (Optional) A <code>PPB_NetAddress</code> resource holding the IP address of
   * multicast source.
   * Specify to join/leave only given source on multicast address.
   */
  PP_Resource source_addr;
};
/**
 * @}
 */

/**
 * @addtogroup Interfaces
 * @{
 */
/**
 * The <code>PPB_UDPSocketExtension_Samsung</code> interface provides
 * additional sockets functionality for PPB_UDPSocket. To use this
 * interface, one have to create standard UDPSocket object.
 *
 * Permissions: This interface does not require additional permissions
 * in relation to standard sockets.
 *
 * This API is deprecated since Samsung Pepper 47.
 * Please use PPB_UDPSocket API instead.
 */
struct PPB_UDPSocketExtension_Samsung_0_1 {
  /**
   * Get a socket option on the associated socket.
   * Please see the <code>PP_UDPSocket_Option</code> description for option
   * names, value types and allowed values.
   * Socket need to be already bound and method will fail with
   * <code>PP_ERROR_FAILED</code> if it's not.
   *
   * @param[in] udp_socket A <code>PP_Resource</code> corresponding to a UDP
   * socket.
   * @param[in] name The option to get.
   * @param[out] Value of requested option.
   * @param[in] callback A <code>PP_CompletionCallback</code> to be called upon
   * completion.
   *
   * @return An int32_t containing an error code from <code>pp_errors.h</code>.
   */
  int32_t (*GetOption)(PP_Resource udp_socket,
                       PP_UDPSocket_Option name,
                       struct PP_Var* value,
                       struct PP_CompletionCallback callback);
  /**
   * Join a multicast group. Use optional <code>source_addr</code> to allow
   * receiving data only from a specified source. This option can be used
   * multiple times to allow receiving data from more than one source.
   * Socket need to be already bound and method will fail with
   * <code>PP_ERROR_FAILED</code> if it's not.
   *
   * @param[in] udp_socket A <code>PP_Resource</code> corresponding to a UDP
   * socket.
   * @param[in] membership A <code>PP_MulticastMembership</code> multicast
   * group and optional source IP.
   * @param[in] callback  A <code>PP_CompletionCallback</code> to be called upon
   * completion.
   *
   * @return An int32_t containing an error code from <code>pp_errors.h</code>.
   */
  int32_t (*JoinMulticast)(PP_Resource udp_socket,
                           const struct PP_MulticastMembership* membership,
                           struct PP_CompletionCallback callback);
  /**
   * Leave a multicast group. Use optional member <code>source_addr</code>
   * to stop receiving data from multicast group that come from a given source.
   * If the application has subscribed to multiple sources within the same
   * group, data from the remaining sources will still be delivered.
   * Socket need to be already bound and method will fail with
   * <code>PP_ERROR_FAILED</code> if it's not.
   *
   * @param[in] udp_socket A <code>PP_Resource</code> corresponding to a UDP
   * socket.
   * @param[in] membership A <code>PP_MulticastMembership</code> multicast
   * group and optional source IP.
   * @param[in] callback A <code>PP_CompletionCallback</code> to be called upon
   * completion.
   *
   * @return An int32_t containing an error code from <code>pp_errors.h</code>.
   */
  int32_t (*LeaveMulticast)(PP_Resource udp_socket,
                            const struct PP_MulticastMembership* membership,
                            struct PP_CompletionCallback callback);
};

typedef struct PPB_UDPSocketExtension_Samsung_0_1
    PPB_UDPSocketExtension_Samsung;
/**
 * @}
 */

#endif  /* PPAPI_C_SAMSUNG_PPB_UDP_SOCKET_EXTENSION_SAMSUNG_H_ */

