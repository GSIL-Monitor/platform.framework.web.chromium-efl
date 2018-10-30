// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "paths_efl.h"

#include <cstdlib>

#include "base/command_line.h"
#include "base/environment.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/memory/free_deleter.h"
#include "base/nix/xdg_util.h"
#include "base/path_service.h"
#include "build/tizen_version.h"
#include "common/content_switches_efl.h"

#if BUILDFLAG(ENABLE_NACL)
#include "components/nacl/common/nacl_constants.h"
#endif

#if defined(OS_TIZEN)
#include <appfw/app.h>
#include <tzplatform_config.h>
#endif

namespace {
#if defined(OS_TIZEN)
const base::FilePath::CharType kDataPath[] = FILE_PATH_LITERAL(DATA_DIR);
const base::FilePath::CharType kExePath[] = FILE_PATH_LITERAL(EXE_DIR);
const base::FilePath::CharType kApplicationDataDir[] = FILE_PATH_LITERAL("data");
const base::FilePath::CharType kApplicationCacheDir[] = FILE_PATH_LITERAL("cache");
const base::FilePath::CharType kNotificationIconDir[] = FILE_PATH_LITERAL("noti");
#endif
const base::FilePath::CharType kLocalePath[] = FILE_PATH_LITERAL(LOCALE_DIR);
const base::FilePath::CharType kEdjePath[] = FILE_PATH_LITERAL(EDJE_DIR);
const base::FilePath::CharType kImagePath[] = FILE_PATH_LITERAL(IMAGE_DIR);
const base::FilePath::CharType kApplicationName[] = FILE_PATH_LITERAL("chromium-efl");
const base::FilePath::CharType kApplicationDataBaseDir[] = FILE_PATH_LITERAL("db");

#if BUILDFLAG(ENABLE_NACL)
const base::FilePath::CharType kPNaClComponentDirName[] =
    FILE_PATH_LITERAL("pnacl");

// Gets the path for internal plugins.
bool GetInternalPluginsDirectory(base::FilePath* result) {
  // The rest of the world expects plugins in the module directory.
#if defined(OS_TIZEN)
  return PathService::Get(base::DIR_EXE, result);
#else
  return PathService::Get(base::DIR_MODULE, result);
#endif
}
#endif

#if defined(OS_TIZEN)
bool GetDirAppDataTizen(base::FilePath& result) {
  char* buffer;

  if (!(buffer = app_get_data_path()))
    return false;

  result = base::FilePath(buffer);
  free(buffer);
  return true;
}

bool GetDirSharedDataTizen(base::FilePath& result) {
  char* buffer;

  if (!(buffer = app_get_shared_data_path()))
    return false;

  result = base::FilePath(buffer);
  free(buffer);
  return true;
}

bool GetDirChromiumPrivateTizen(base::FilePath& result) {
  std::unique_ptr<char, base::FreeDeleter> data_path(app_get_data_path());
  if (data_path) {
    result = base::FilePath(data_path.get());
  } else if (!GetDirAppDataTizen(result)) {
    return false;
  }

  const base::CommandLine& command_line =
      *base::CommandLine::ForCurrentProcess();
  if (command_line.HasSwitch(switches::kBrowserInstanceId)) {
    std::string instance_id =
        command_line.GetSwitchValueASCII(switches::kBrowserInstanceId);
    if (!instance_id.empty())
      result = result.Append(instance_id);
  }

  result = result.Append(kApplicationName);

  return true;
}

base::FilePath GetDirUserDataTizen() {
  base::FilePath path;
  if (GetDirChromiumPrivateTizen(path))
    path = path.Append(kApplicationDataDir);
  return path;
}

base::FilePath GetDirCacheTizen() {
  base::FilePath path;
  if (GetDirChromiumPrivateTizen(path))
    path = path.Append(kApplicationCacheDir);
  return path;
}
#endif

base::FilePath GetDirDownloads() {
#if !defined(OS_TIZEN)
  return base::FilePath(FILE_PATH_LITERAL("/tmp/"));
#else
  return base::FilePath(FILE_PATH_LITERAL(tzplatform_getenv(TZ_USER_DOWNLOADS)));
#endif
}

base::FilePath GetDirImages() {
#if !defined(OS_TIZEN)
  return base::FilePath(FILE_PATH_LITERAL("/tmp/"));
#else
  return base::FilePath(FILE_PATH_LITERAL(tzplatform_getenv(TZ_USER_IMAGES)));
#endif
}

void GetDirUserData(base::FilePath& result) {
#if defined(OS_TIZEN)
  result = GetDirUserDataTizen();
  if (result.empty()) {
    std::unique_ptr<base::Environment> env(base::Environment::Create());
    base::FilePath config_dir(base::nix::GetXDGDirectory(env.get(),
                              base::nix::kXdgConfigHomeEnvVar,
                              base::nix::kDotConfigDir));
    char* app_id = NULL;
    if (APP_ERROR_NONE == app_get_id(&app_id)) {
      std::unique_ptr<char, base::FreeDeleter> app_name(app_id);
      result = config_dir.Append(app_name.get());
    } else {
      result = config_dir.Append(kApplicationName);
    }
  }
#else
  std::unique_ptr<base::Environment> env(base::Environment::Create());
  base::FilePath config_dir(base::nix::GetXDGDirectory(env.get(),
                            base::nix::kXdgConfigHomeEnvVar,
                            base::nix::kDotConfigDir));
  result = config_dir.Append(kApplicationName);
#endif
}

void GetDirCache(base::FilePath& result) {
#if defined(OS_TIZEN)
  result = GetDirCacheTizen();
  if (result.empty()) {
    std::unique_ptr<base::Environment> env(base::Environment::Create());
    base::FilePath cache_dir(base::nix::GetXDGDirectory(env.get(),
                                                        "XDG_CACHE_HOME",
                                                        ".cache"));
    char* app_id = NULL;
    if (APP_ERROR_NONE == app_get_id(&app_id)) {
      std::unique_ptr<char, base::FreeDeleter> app_name(app_id);
      result = cache_dir.Append(app_name.get());
    } else {
      result = cache_dir.Append(kApplicationName);
    }
  }
#else
  std::unique_ptr<base::Environment> env(base::Environment::Create());
  base::FilePath cache_dir(base::nix::GetXDGDirectory(env.get(),
                                                      "XDG_CACHE_HOME",
                                                      ".cache"));
  result = cache_dir.Append(kApplicationName);
#endif
}
} // namespace

namespace PathsEfl {
bool PathProvider(int key, base::FilePath* result) {
  DCHECK(result);
  base::FilePath cur;
  switch (key) {
#ifdef OS_TIZEN
    case base::DIR_EXE:
    case base::DIR_MODULE:
      *result = base::FilePath(kExePath);
      return true;
#endif
    case DIR_LOCALE:
      *result = base::FilePath(kLocalePath);
      return true;
    case EDJE_RESOURCE_DIR:
      *result = base::FilePath(kEdjePath);
      return true;
    case DIR_DOWNLOADS:
      *result = GetDirDownloads();
      return true;
    case DIR_DOWNLOAD_IMAGE:
      *result = GetDirImages();
      return true;
    case DIR_USER_DATA:
      GetDirUserData(cur);
      break;
    case base::DIR_CACHE:
      GetDirCache(cur);
      break;
    case WEB_DATABASE_DIR:
      if (!PathService::Get(DIR_USER_DATA, &cur))
        return false;
      cur = cur.Append(kApplicationDataBaseDir);
      break;
    case IMAGE_RESOURCE_DIR:
      *result = base::FilePath(kImagePath);
      return true;
#if defined(OS_TIZEN)
    case DIR_SHARED:
      GetDirSharedDataTizen(cur);
      break;
    case DIR_SHARED_NOTI_ICON:
      GetDirSharedDataTizen(cur);
      cur = cur.Append(kNotificationIconDir);
      break;
#endif
#if BUILDFLAG(ENABLE_NACL)
    case PathsEfl::DIR_INTERNAL_PLUGINS:
      if (!GetInternalPluginsDirectory(&cur))
        return false;
      break;
    case PathsEfl::DIR_PNACL_COMPONENT:
      if (!GetInternalPluginsDirectory(&cur))
        return false;
      cur = cur.Append(kPNaClComponentDirName);
      break;
    case PathsEfl::FILE_NACL_PLUGIN:
      if (!GetInternalPluginsDirectory(&cur))
        return false;
      *result = cur.Append(nacl::kInternalNaClPluginFileName);
      return true;
#endif
    default:
      return false;
  }

  if (!base::DirectoryExists(cur) && !base::CreateDirectory(cur))
    return false;

  *result = cur;
  return true;
}

void Register() {
  PathService::RegisterProvider(PathProvider, PATH_START, PATH_END);
}
} // namespace PathsEfl
