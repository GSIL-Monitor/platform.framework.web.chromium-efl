// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PPAPI_CPP_SAMSUNG_EXTENSION_SYSTEM_SAMSUNG_H_
#define PPAPI_CPP_SAMSUNG_EXTENSION_SYSTEM_SAMSUNG_H_

#include <string>

#include "ppapi/cpp/instance_handle.h"
#include "ppapi/cpp/var.h"

/// @file
/// This file defines APIs related to extension system provided by the browser.

namespace pp {

class ExtensionSystemSamsung {
 public:
  /// GetEmbedderName() returns name of the extension system provider.
  std::string GetEmbedderName() const;

  /// GetCurrentExtensionInfo() returns information about current extension.
  /// If browser does not support extension system, it will return undefined
  /// value.
  Var GetCurrentExtensionInfo();

  /// GenericSyncCall() communicates synchronously with extension system.
  /// The extension system will execute operation |operation_name| with provided
  /// arguments |operation_data|. See embedder documentation to know what
  /// operations are possible.
  ///
  /// @param[in] operation_name The name of operation to execute.
  /// @param[in] operation_data Additional arguments for operation execution.
  Var GenericSyncCall(const Var& operation_name, const Var& operation_data);

 protected:
  /// A constructor for creating a <code>ExtensionSystemSamsung</code>.
  ///
  /// @param[in] instance The instance with which this resource will be
  /// associated.
  explicit ExtensionSystemSamsung(const InstanceHandle& instance);

  /// Destructor.
  ~ExtensionSystemSamsung();

 private:
  InstanceHandle instance_;
};

}  // namespace pp

#endif  // PPAPI_CPP_SAMSUNG_EXTENSION_SYSTEM_SAMSUNG_H_
