// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef _PATHS_EFL_H_
#define _PATHS_EFL_H_

#include "base/base_paths.h"
#include "content/common/content_export.h"
#include "components/nacl/common/features.h"

namespace PathsEfl {
enum {
  PATH_START = 2000,
  EDJE_RESOURCE_DIR,
  IMAGE_RESOURCE_DIR,
  WEB_DATABASE_DIR,
  DIR_USER_DATA,
  DIR_DATA_PATH,
  DIR_DOWNLOADS,
  DIR_DOWNLOAD_IMAGE,
  DIR_LOCALE,
  DIR_SHARED,
  DIR_SHARED_NOTI_ICON,
#if BUILDFLAG(ENABLE_NACL)
  // Values taken from chromium/src/chrome/common/chrome_paths.h
  DIR_INTERNAL_PLUGINS,         // Directory where internal plugins reside.
  DIR_PNACL_COMPONENT,          // Full path to the latest PNaCl version
  FILE_NACL_PLUGIN,             // Fullpath to the internal NaCl plugin file.
#endif
  PATH_END
};

// Call once per each process type to register the provider for the
// path keys defined above
CONTENT_EXPORT void Register();
}

#endif // _PATHS_EFL_H_
