// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "favicon_database_p.h"
#include <iostream>

#include "base/logging.h"
#include "base/path_service.h"
#include "content/common/paths_efl.h"
#include "favicon_commands.h"

const char * const FaviconDatabasePrivate::pageUrlToFaviconUrlTable = "url_to_favicon_url";
const char * const FaviconDatabasePrivate::faviconUrlToBitmapTable = "favicon_url_to_bitmap";
const char * const FaviconDatabasePrivate::pageUrlColumn = "page_url";
const char * const FaviconDatabasePrivate::faviconUrlColumn = "favicon_url";
const char * const FaviconDatabasePrivate::bitmapColumn = "bitmap";
const char * const FaviconDatabasePrivate::defaultDirSuffix = ".config/chromium-efl/IconDatabase";
const char * const FaviconDatabasePrivate::defaultFilename = "WebpageIcons.db";

FaviconDatabasePrivate::FaviconDatabasePrivate()
    : privateBrowsing(false), sqlite(0), weakPtrFactory(this) {
  base::FilePath db_path;
  if (PathService::Get(PathsEfl::WEB_DATABASE_DIR, &db_path))
    path = db_path.Append(FILE_PATH_LITERAL(defaultFilename));
  else
    LOG(ERROR)                                                // LCOV_EXCL_LINE
        << "[favicon] Could not get web database directory";  // LCOV_EXCL_LINE
}

/* LCOV_EXCL_START */
GURL FaviconDatabasePrivate::faviconUrlForPageUrl(const GURL &pageUrl) const {
  std::map<GURL, GURL>::const_iterator it = pageToFaviconUrl.find(pageUrl);
  if (it != pageToFaviconUrl.end()) {
    return it->second;
    /* LCOV_EXCL_STOP */
  }
  return GURL();  // LCOV_EXCL_LINE
}

/* LCOV_EXCL_START */
SkBitmap FaviconDatabasePrivate::bitmapForFaviconUrl(const GURL &faviconUrl) const {
  std::map<GURL, SkBitmap>::const_iterator it = faviconUrlToBitmap.find(faviconUrl);
  if (it != faviconUrlToBitmap.end()) {

    return it->second;
    /* LCOV_EXCL_STOP */
  }
  return SkBitmap();  // LCOV_EXCL_LINE
}

bool FaviconDatabasePrivate::existsForFaviconURL(const GURL &faviconUrl) const {
  std::map<GURL, SkBitmap>::const_iterator it = faviconUrlToBitmap.find(faviconUrl);
  return it != faviconUrlToBitmap.end();
}

/* LCOV_EXCL_START */
scoped_refptr<base::SingleThreadTaskRunner> FaviconDatabasePrivate::taskRunner() const {
  return content::BrowserThread::GetTaskRunnerForThread(content::BrowserThread::DB);
}

void FaviconDatabasePrivate::performSync() {
  std::cerr << "[FaviconDatabasePrivate::performSync()]" << std::endl;
  base::AutoLock locker(mutex);
  timer.Stop();
  while (!commands.empty()) {
    Command *cmd = commands.front();
    if (!cmd->sqlExecute()) {
      std::cerr << "[FaviconDatabasePrivate::performSync] :: " << "Error while executing command:\n\t" << cmd->lastError() << std::endl;
      // abort or what?
    }
    commands.pop();
    delete cmd;
  }
}
/* LCOV_EXCL_STOP */
