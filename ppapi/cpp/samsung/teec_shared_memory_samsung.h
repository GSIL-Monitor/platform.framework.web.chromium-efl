// Copyright (c) 2017 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PPAPI_CPP_SAMSUNG_TEEC_SHARED_MEMORY_SAMSUNG_H_
#define PPAPI_CPP_SAMSUNG_TEEC_SHARED_MEMORY_SAMSUNG_H_

#include "ppapi/c/samsung/ppb_teec_shared_memory_samsung.h"
#include "ppapi/cpp/completion_callback.h"
#include "ppapi/cpp/resource.h"
#include "ppapi/cpp/samsung/teec_context_samsung.h"

namespace pp {

class TEECSharedMemory_Samsung : public Resource {
 public:
  TEECSharedMemory_Samsung();

  explicit TEECSharedMemory_Samsung(PP_Resource resource);

  TEECSharedMemory_Samsung(PassRef, PP_Resource resource);

  /// Creates a new TEEC Shared Memory.
  ///
  /// @param[in] teec_context A parent TEEC_Context for the TEEC shared memory.
  /// @param[in] flags describing if memory can be used as input and/or output.
  /// See the PP_TEEC_MemoryType type for more details.
  TEECSharedMemory_Samsung(const TEECContext_Samsung& teec_context,
                           uint32_t flags);

  TEECSharedMemory_Samsung(const TEECSharedMemory_Samsung& other);

  TEECSharedMemory_Samsung& operator=(const TEECSharedMemory_Samsung& other);

  virtual ~TEECSharedMemory_Samsung();

  /// Registers a block of existing Client Application memory as
  /// a block of Shared Memory
  ///
  /// @param[in] buffer A pointer to a buffer that should be registered.
  /// @param[in] size A size of memory that should be registered
  /// @param[in] callback A <code>CompletionCallback</code> to be called upon
  /// completion.
  ///
  /// @return PP_OK on success, otherwise an error code from
  /// <code>pp_errors.h</code>. Meaning of errors:
  /// - <code>PP_ERROR_RESOURCE_FAILED</code> - if shared memory was created for
  ///   an invalid PP_TEEC_Context.
  /// - <code>PP_ERROR_FAILED</code> - if shared memory has been already
  ///   allocated or registered, or an error occured in underlaying library
  ///   calls. Detailed error is returned in PP_TEEC_Result structure as
  ///   an output of the completion callback.
  int32_t Register(
      const void* buffer,
      uint32_t size,
      const CompletionCallbackWithOutput<PP_TEEC_Result>& callback);

  /// Allocates a new block of memory as a block of Shared Memory
  ///
  /// @param[in] size A size of memory that should be allocated
  /// @param[in] callback A <code>CompletionCallback</code> to be called upon
  /// completion.
  ///
  /// @return PP_OK on success, otherwise an error code from
  /// <code>pp_errors.h</code>. Meaning of errors:
  /// - <code>PP_ERROR_RESOURCE_FAILED</code> - if shared memory was created for
  ///   an invalid PP_TEEC_Context.
  /// - <code>PP_ERROR_NOMEMORY</code> - if the allocation could not be
  ///   completed due to resource constraints.
  /// - <code>PP_ERROR_FAILED</code> - if shared memory has been already
  ///   allocated or registered, or an error occured in underlaying library
  ///   calls. Detailed error is returned in PP_TEEC_Result structure as
  ///   an output of the completion callback.
  int32_t Allocate(
      uint32_t size,
      const CompletionCallbackWithOutput<PP_TEEC_Result>& callback);

  /// Maps shared memory into Client Application address space
  ///
  /// @return A pointer to mapped buffer or nullptr when mapping failed.
  void* Map();
};

}  // namespace pp

#endif  // PPAPI_CPP_SAMSUNG_TEEC_SHARED_MEMORY_SAMSUNG_H_
