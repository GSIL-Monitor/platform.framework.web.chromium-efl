// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TIZEN_SRC_EWK_EFL_INTEGRATION_EWK_PRIVILEGE_CHECKER_H_
#define TIZEN_SRC_EWK_EFL_INTEGRATION_EWK_PRIVILEGE_CHECKER_H_

#include <string>

#include "base/macros.h"
#include "base/memory/singleton.h"

namespace content {

/*
 * EwkPrivilegeChecker is an implementation for resolving access for given
 * privilege
 */
class EwkPrivilegeChecker {
 public:
  static EwkPrivilegeChecker* GetInstance();
  bool CheckPrivilege(const std::string& privilege_name);

 private:
  EwkPrivilegeChecker() = default;
  friend struct base::DefaultSingletonTraits<EwkPrivilegeChecker>;
  DISALLOW_COPY_AND_ASSIGN(EwkPrivilegeChecker);
};

}  // namespace content

#endif  // TIZEN_SRC_EWK_EFL_INTEGRATION_EWK_PRIVILEGE_CHECKER_H_
