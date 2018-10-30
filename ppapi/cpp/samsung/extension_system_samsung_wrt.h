// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PPAPI_CPP_SAMSUNG_EXTENSION_SYSTEM_SAMSUNG_WRT_H_
#define PPAPI_CPP_SAMSUNG_EXTENSION_SYSTEM_SAMSUNG_WRT_H_

#include <map>
#include <string>

#include "ppapi/cpp/samsung/extension_system_samsung_tizen.h"

/// @file
/// This file defines APIs related to extension system provided by the WRT.

namespace pp {

class ExtensionSystemSamsungWRT : public ExtensionSystemSamsungTizen {
 public:
  /// A constructor for creating a <code>ExtensionSystemSamsung</code>.
  ///
  /// @param[in] instance The instance with which this resource will be
  /// associated.
  explicit ExtensionSystemSamsungWRT(const InstanceHandle& instance);

  /// Destructor.
  ~ExtensionSystemSamsungWRT();

  /// IsValid() returns true if this object is valid in this context, i.e.
  /// current extension system is WRT.
  bool IsValid() const;

 private:
  std::map<std::string, bool> privileges_result_;
};

}  // namespace pp

#endif  // PPAPI_CPP_SAMSUNG_EXTENSION_SYSTEM_SAMSUNG_WRT_H_
