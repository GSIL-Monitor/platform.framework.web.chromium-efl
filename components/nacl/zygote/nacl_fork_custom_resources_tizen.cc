// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#if defined(OS_TIZEN_TV_PRODUCT)

#include "components/nacl/zygote/nacl_fork_custom_resources_tizen.h"

#include <fcntl.h>

#include <vector>

#include "base/files/file.h"
#include "base/logging.h"
#include "base/posix/eintr_wrapper.h"

namespace nacl {

namespace {

struct DlogResources {
  const char* path;
  base::PlatformFile mapped_fd;
  int open_flag;
  int fd;
};

DlogResources kDlogResources[] = {
    {"/opt/etc/dlog.conf", CustomTizenFD::kLogConfDesc, O_RDONLY, -1},
    {"/dev/log_main", CustomTizenFD::kLogMainDesc, O_WRONLY, -1},
    {"/dev/log_radio", CustomTizenFD::kLogRadioDesc, O_WRONLY, -1},
    {"/dev/log_system", CustomTizenFD::kLogSystemDesc, O_WRONLY, -1},
};
}  // namespace

void CustomTizenFD::AddCustomTizenFD(
    base::FileHandleMappingVector* mapping_vector) {
  if (!mapping_vector) {
    LOG(ERROR) << "Pointer to mapping_vector is null!";
    return;
  }
  AddDlogFD(mapping_vector);
}

void CustomTizenFD::CleanupTizenFD() {
  // Closing opened dlog FD.
  for (auto& res : kDlogResources) {
    if (res.fd != -1 && HANDLE_EINTR(close(res.fd)) != -1)
      res.fd = -1;
  }
}

bool CustomTizenFD::IsDlogFDInitialized() {
  // All FD are always either opened or closed, thus we need only to check
  // if first one is opened.
  return kDlogResources[0].fd != -1;
}

// This function will open FD for Dlog.
// The dlog FD will be closed after fork in CleanupTizenFD method.
void CustomTizenFD::AddDlogFD(base::FileHandleMappingVector* mapping_vector) {
  // First check if our mapping wont interfere with other mappings
  for (auto& res : kDlogResources)
    for (auto& v : *mapping_vector)
      CHECK_NE(res.mapped_fd, v.second);

  for (auto& res : kDlogResources) {
    if (res.fd == -1)
      res.fd = HANDLE_EINTR(open(res.path, res.open_flag));
    if (res.fd == -1) {
      LOG(ERROR) << "Failed to open dlog resource: " << res.path
                 << ", Dlog will not work in NaCl process.";
      // We close all Dlog resources on fail.
      CleanupTizenFD();
      return;
    }
  }
  // We add only FD to mapping only if all resources were opened successfully
  for (auto& res : kDlogResources)
    mapping_vector->push_back(std::make_pair(res.fd, res.mapped_fd));
}

}  // namespace nacl

#endif  // defined(OS_TIZEN_TV_PRODUCT)
