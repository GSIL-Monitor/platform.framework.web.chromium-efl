// Copyright (c) 2017 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PPAPI_CPP_SAMSUNG_TEEC_SESSION_SAMSUNG_H_
#define PPAPI_CPP_SAMSUNG_TEEC_SESSION_SAMSUNG_H_

#include "ppapi/c/samsung/ppb_teec_session_samsung.h"
#include "ppapi/cpp/completion_callback.h"
#include "ppapi/cpp/resource.h"
#include "ppapi/cpp/samsung/teec_context_samsung.h"

namespace pp {

class TEECSession_Samsung : public Resource {
 public:
  TEECSession_Samsung();

  explicit TEECSession_Samsung(PP_Resource context);

  TEECSession_Samsung(PassRef, PP_Resource resource);

  /// Creates a new TEEC Session
  ///
  /// @param[in] teec_context A parent TEEC_Context for the TEEC session.
  explicit TEECSession_Samsung(const TEECContext_Samsung& teecc_context);

  TEECSession_Samsung(const TEECSession_Samsung& other);

  TEECSession_Samsung& operator=(const TEECSession_Samsung& other);

  virtual ~TEECSession_Samsung();

  /// Opens a session between the Client Application and the specified
  /// Trusted Application.
  ///
  /// @param[in] destination A pointer to a structure containing the UUID
  /// of the destination Trusted Application.
  /// @param[in] connection_method The method of connection to use.
  /// @param[in] connection_data_size A size of passed connection data.
  /// @param[in] connection_data Any necessary data required to support
  /// the connection method chosen.
  /// @param[in] operation A pointer to an Operation containing a set of
  /// Parameters to exchange with the Trusted Application, or NULL if no
  /// Parameters are to be exchanged or if the operation cannot be cancelled.
  /// @param[in] callback A <code>CompletionCallback</code> to be called upon
  /// completion.
  ///
  /// @return PP_OK on success, otherwise an error code from
  /// <code>pp_errors.h</code>. Meaning of errors:
  /// - <code>PP_ERROR_RESOURCE_FAILED</code> - if session was created for
  ///   an invalid PP_TEEC_Context.
  /// - <code>PP_ERROR_BADARGUMENT</code> - if invalid PP_TEEC_Operation
  ///   structure was passed.
  /// - <code>PP_ERROR_FAILED</code> - if session has been already opened or
  ///   an error occured in underlaying library calls. Detailed error is
  ///   returned in PP_TEEC_Result structure as an output of the completion
  ///   callback.
  int32_t Open(PP_TEEC_UUID* destination,
               uint32_t connection_method,
               uint32_t connection_data_size,
               const void* connection_data,
               PP_TEEC_Operation* operation,
               const CompletionCallbackWithOutput<PP_TEEC_Result>& callback);

  /// This function invokes a Command within the Trusted Application
  ///
  /// @param[in] command_id The identifier of the Command within the Trusted
  /// Application to invoke. The meaning of each Command Identifier must be
  /// defined in the protocol exposed by the Trusted Application
  /// @param[in] operation A pointer to an Operation containing a set of
  /// Parameters to exchange with the Trusted Application, or NULL if no
  /// Parameters are to be exchanged or if the operation cannot be cancelled.
  /// @param[in] callback A <code>CompletionCallback</code> to be called upon
  /// completion.
  ///
  /// @return PP_OK on success, otherwise an error code from
  /// <code>pp_errors.h</code>. Meaning of errors:
  /// - <code>PP_ERROR_RESOURCE_FAILED</code> - if session was created for
  ///   an invalid PP_TEEC_Context.
  /// - <code>PP_ERROR_BADARGUMENT</code> - if invalid PP_TEEC_Operation
  ///   structure was passed.
  /// - <code>PP_ERROR_FAILED</code> - if session was not opened or and error
  ///   occured in underlaying library calls. Detailed error is returned
  ///   in PP_TEEC_Result structure as an output of the completion callback.
  int32_t InvokeCommand(
      uint32_t command_id,
      PP_TEEC_Operation* operation,
      const CompletionCallbackWithOutput<PP_TEEC_Result>& callback);

  int32_t RequestCancellation(const PP_TEEC_Operation* operation,
                              const CompletionCallback& callback);
};

}  // namespace pp

#endif  // PPAPI_CPP_SAMSUNG_TEEC_SESSION_SAMSUNG_H_
