/*
 * Copyright (c) 2016 Samsung Electronics, Visual Display Division.
 * All Rights Reserved.
 *
 * @author  Michal Jurkiewicz <m.jurkiewicz@samsung.com>
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the Chromium-LICENSE file.
 */

#ifndef CONTENT_BROWSER_PERMISSIONS_PEPPER_PERMISSION_CHALLENGER_H_
#define CONTENT_BROWSER_PERMISSIONS_PEPPER_PERMISSION_CHALLENGER_H_

#include <string>
#include <vector>

#include "base/macros.h"
#include "base/memory/singleton.h"
#include "components/nacl/common/features.h"

namespace content {

class PepperPermissionChallenger {
 public:
  static PepperPermissionChallenger* GetInstance();
  void PopulateSwitches();

 private:
  void AppendTestingSwitch(std::vector<std::string>&);
#if BUILDFLAG(ENABLE_NACL)
  void AppendNaClDebugSwitch(std::vector<std::string>&);
  void AppendNaClNonSFIVariantSwitch(std::vector<std::string>&);
#endif
  PepperPermissionChallenger() = default;
  ~PepperPermissionChallenger() = default;
  friend struct base::DefaultSingletonTraits<PepperPermissionChallenger>;

  DISALLOW_COPY_AND_ASSIGN(PepperPermissionChallenger);
};

}  // namespace content

#endif  // CONTENT_BROWSER_PERMISSIONS_PEPPER_PERMISSION_CHALLENGER_H_
