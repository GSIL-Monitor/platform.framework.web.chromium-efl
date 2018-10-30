// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FAVICON_DATABASE_P_H
#define FAVICON_DATABASE_P_H

#include <map>
#include <queue>
#include "url/gurl.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "third_party/sqlite/sqlite3.h"
#include "base/files/file_path.h"
#include "base/single_thread_task_runner.h"
#include "base/synchronization/lock.h"
#include "base/timer/timer.h"
#include "base/memory/weak_ptr.h"
#include "content/public/browser/browser_thread.h"
#include "favicon_database.h"
#include "private/ewk_main_private.h"

class Command;

struct FaviconDatabasePrivate : public base::RefCountedThreadSafe<FaviconDatabasePrivate> {
  FaviconDatabasePrivate();

  GURL faviconUrlForPageUrl(const GURL &pageUrl) const;
  SkBitmap bitmapForFaviconUrl(const GURL &faviconUrl) const;

  bool existsForFaviconURL(const GURL &faviconUrl) const;

  scoped_refptr<base::SingleThreadTaskRunner> taskRunner() const;
  void performSync();

  std::map<GURL, GURL> pageToFaviconUrl;
  std::map<GURL, SkBitmap> faviconUrlToBitmap;
  base::FilePath path;
  bool privateBrowsing;

  sqlite3 *sqlite;
  base::Lock mutex;
  base::RepeatingTimer timer;
  std::queue<Command *> commands;
  base::WeakPtrFactory<FaviconDatabasePrivate> weakPtrFactory;

  static const char * const pageUrlToFaviconUrlTable;
  static const char * const faviconUrlToBitmapTable;
  static const char * const pageUrlColumn;
  static const char * const faviconUrlColumn;
  static const char * const bitmapColumn;
  static const char * const defaultDirSuffix;
  static const char * const defaultFilename;
};

#endif // FAVICON_DATABASE_P_H
