// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/disk_cache/cache_util.h"

#include "base/files/file_enumerator.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/strings/string_util.h"

#if defined(OS_TIZEN_TV_PRODUCT)
#include "net/disk_cache/blockfile/disk_format.h"
#endif

namespace disk_cache {

bool MoveCache(const base::FilePath& from_path, const base::FilePath& to_path) {
#if defined(OS_CHROMEOS)
  // For ChromeOS, we don't actually want to rename the cache
  // directory, because if we do, then it'll get recreated through the
  // encrypted filesystem (with encrypted names), and we won't be able
  // to see these directories anymore in an unmounted encrypted
  // filesystem, so we just move each item in the cache to a new
  // directory.
  if (!base::CreateDirectory(to_path)) {
    LOG(ERROR) << "Unable to create destination cache directory.";
    return false;
  }
  base::FileEnumerator iter(from_path, false /* not recursive */,
      base::FileEnumerator::DIRECTORIES | base::FileEnumerator::FILES);
  for (base::FilePath name = iter.Next(); !name.value().empty();
       name = iter.Next()) {
    base::FilePath destination = to_path.Append(name.BaseName());
    if (!base::Move(name, destination)) {
      LOG(ERROR) << "Unable to move cache item.";
      return false;
    }
  }
  return true;
#else
  return base::Move(from_path, to_path);
#endif
}

bool DeleteCacheFile(const base::FilePath& name) {
  return base::DeleteFile(name, false);
}

#if defined(OS_TIZEN_TV_PRODUCT)
/*
  Device life time table : return value of BSP API, life_time
  0x00 : Not defined
  0x01 :  0% -  10% device life time used
  0x02 : 10% -  20% device life time used
  0x03 : 20% -  30% device life time used
  0x04 : 30% -  40% device life time used
  0x05 : 40% -  50% device life time used
  0x06 : 50% -  60% device life time used
  0x07 : 60% -  70% device life time used
  0x08 : 70% -  80% device life time used
  0x09 : 80% -  90% device life time used
  0x0A : 90% - 100% device life time used
  0x0B : Exceeded its maximum estimated device life time
  *IMPORTANT* The disk cache should be used having the device
  life time value from 0x01 to 0x05.
*/
int GetMemoryLifeTime() {
  base::FilePath file_path(FILE_PATH_LITERAL("/sys/block/mmcblk0/life_time"));

  base::File file(file_path, base::File::FLAG_OPEN | base::File::FLAG_READ);
  if (!file.IsValid())
    return -1;

  std::string buffer;
  if (!base::ReadFileToString(file_path, &buffer))
    return -1;

  int life_time = static_cast<int>(strtol(buffer.c_str(), 0, 16));
  return life_time;
}

// Reads the |header_size| bytes from the beginning of file |name|.
bool ReadHeader(const base::FilePath& name, char* header, int header_size) {
  base::File file(name, base::File::FLAG_OPEN | base::File::FLAG_READ);
  if (!file.IsValid())
    return false;

  int read = file.Read(0, header, header_size);
  if (read != header_size)
    return false;

  return true;
}

int CalculateWebAppCacheSize(const base::FilePath& path, net::CacheType type) {
  if (type != net::DISK_CACHE)
    return -1;

  // Only need to calculate the web app cache size, so skip the non web app.
  if (path.value().find(kTizenAppCache) == std::string::npos)
    return -1;

  int total = 0;
  base::FileEnumerator iter(path.DirName(), false,
                            base::FileEnumerator::DIRECTORIES);
  for (base::FilePath app_name = iter.Next(); !app_name.value().empty();
       app_name = iter.Next()) {
    base::FilePath index_path = app_name.Append(kIndexName);
    disk_cache::IndexHeader header;
    if (!ReadHeader(index_path, reinterpret_cast<char*>(&header),
                    sizeof(header)))
      continue;
    total += header.num_bytes;
  }

  return total;
}
#endif

}  // namespace disk_cache
