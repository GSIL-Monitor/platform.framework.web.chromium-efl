// Copyright (c) 2017 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PPAPI_CPP_SAMSUNG_TEEC_CONTEXT_SAMSUNG_H_
#define PPAPI_CPP_SAMSUNG_TEEC_CONTEXT_SAMSUNG_H_

#include <string>

#include "ppapi/c/samsung/ppb_teec_context_samsung.h"
#include "ppapi/cpp/completion_callback.h"
#include "ppapi/cpp/pass_ref.h"
#include "ppapi/cpp/resource.h"

namespace pp {

class InstanceHandle;

class TEECContext_Samsung : public Resource {
 public:
  TEECContext_Samsung();

  /// Creates a TEEC Context associated with given pp::Instance.
  explicit TEECContext_Samsung(const InstanceHandle& instance);

  explicit TEECContext_Samsung(PP_Resource resource);

  TEECContext_Samsung(PassRef, PP_Resource resource);

  TEECContext_Samsung(const TEECContext_Samsung& other);

  TEECContext_Samsung& operator=(const TEECContext_Samsung& other);

  virtual ~TEECContext_Samsung();

  /// Initializes this TEEC Context, forming a connection between this
  /// Client Application and the TEE identified by the string
  /// identifier name.
  ///
  /// @param[in] name A string that describes the TEE to connect to.
  /// If this parameter is an empty string the default TEE is selected
  /// @param[in] callback A <code>CompletionCallback</code> to be called upon
  /// completion.
  ///
  /// @return PP_OK on success, otherwise an error code from
  /// <code>pp_errors.h</code>. Meaning of errors:
  /// - <code>PP_ERROR_NOACCESS</code> - if application had no permission to
  ///   access TEE Client Api.
  /// - <code>PP_ERROR_FAILED</code> - if context has been already opened or an
  ///   error occured in underlaying library calls. Detailed error is returned
  ///   in PP_TEEC_Result structure as an output of the completion callback.
  int32_t Open(const std::string& context_name,
               const CompletionCallbackWithOutput<PP_TEEC_Result>& callback);
};

}  // namespace pp

#endif  // PPAPI_CPP_SAMSUNG_TEEC_CONTEXT_SAMSUNG_H_
