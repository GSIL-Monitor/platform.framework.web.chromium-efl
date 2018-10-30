// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_NACL_ZYGOTE_NACL_FORK_CUSTOM_RESOURCES_TIZEN_H_
#define COMPONENTS_NACL_ZYGOTE_NACL_FORK_CUSTOM_RESOURCES_TIZEN_H_

#if defined(OS_TIZEN_TV_PRODUCT)

#include "base/posix/global_descriptors.h"
#include "base/process/launch.h"
#include "content/public/common/content_descriptors.h"

namespace nacl {

class CustomTizenFD {
 public:
  // We use highest calculated known FD and increment it by 5 to lower
  // chances of conflicts in mapping.
  static const int kTizenSafeDescriptor =
      base::GlobalDescriptors::kBaseDescriptor + kContentIPCDescriptorMax + 5;
  enum Descriptors {
    // Dlog descriptors numbers to be mapped on NaCl Process side
    kLogConfDesc = kTizenSafeDescriptor,
    kLogMainDesc,
    kLogRadioDesc,
    kLogSystemDesc,
  };

  // Allows to pass custom Tizen specific FD to NaCl Process.
  static void AddCustomTizenFD(base::FileHandleMappingVector*);

  // This method is called after fork in parent process (main Zygote).
  // It`s used to close any unneded FD.
  static void CleanupTizenFD();

  // We need to check if FD have been opened properly to init Dlog in NaCl.
  static bool IsDlogFDInitialized();

 private:
  // Opens dlog devices and insert them in to mapping.
  static void AddDlogFD(base::FileHandleMappingVector* mapping_vector);
};
}  // namespace nacl

#endif  // defined(OS_TIZEN_TV_PRODUCT)

#endif  // COMPONENTS_NACL_ZYGOTE_NACL_FORK_CUSTOM_RESOURCES_TIZEN_H_
