// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PPAPI_CPP_SAMSUNG_EXTENSION_SYSTEM_SAMSUNG_TIZEN_H_
#define PPAPI_CPP_SAMSUNG_EXTENSION_SYSTEM_SAMSUNG_TIZEN_H_

#include <map>
#include <string>

#include "ppapi/cpp/samsung/extension_system_samsung.h"

/// @file
/// This file defines APIs related to extension system provided by the Tizen.

namespace pp {

class ExtensionSystemSamsungTizen : public ExtensionSystemSamsung {
 public:
  /// A constructor for creating a <code>ExtensionSystemSamsung</code>.
  ///
  /// @param[in] instance The instance with which this resource will be
  /// associated.
  explicit ExtensionSystemSamsungTizen(const InstanceHandle& instance);

  /// Destructor.
  ~ExtensionSystemSamsungTizen();

  /// CheckPrivilege() returns true if the current extension has given
  /// privilege, false otherwise.
  bool CheckPrivilege(const Var& privilege);

  /// SetIMERecommendedWords() returns true if setting recommended words
  /// was successful, false otherwise.
  ///
  /// @param[in] words Var containing std::string with words to set.
  bool SetIMERecommendedWords(const Var& words);

  /// SetIMERecommendedWordsType() returns true if setting specified
  /// IME Recommended Words type was successful, false otherwise.
  ///
  /// @param[in] should_enable bool indicating if
  /// IMERecommendedWordsType should be enabled or disabled.
  bool SetIMERecommendedWordsType(bool should_enable);

  /// GetWindowId() returns the X window Id for the current window.
  int32_t GetWindowId();

 private:
  std::map<std::string, bool> privileges_result_;
};

}  // namespace pp

#endif  // PPAPI_CPP_SAMSUNG_EXTENSION_SYSTEM_SAMSUNG_TIZEN_H_
