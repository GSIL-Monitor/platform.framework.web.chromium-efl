// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/files/file_util.h"
#include "favicon_database.h"
#include "favicon_database_p.h"
#include "favicon_commands.h"
#include <iostream>
#include <cstdio>

#define CHECK_OPEN if (!Open()) {\
                     return;\
                   }

#define CHECK_OPEN_RET(ret) if (!Open()) {\
                              return ret;\
                            }

const int FaviconDatabase::SYNC_DELAY = 3;

FaviconDatabase *FaviconDatabase::Instance() {
  static FaviconDatabase database;
  return &database;
}

FaviconDatabase::~FaviconDatabase() {
  base::AutoLock locker(d->mutex);
  if (IsOpen()) {
    Close();
  }
}

FaviconDatabase::FaviconDatabase()
  : d(new FaviconDatabasePrivate) {
}

/* LCOV_EXCL_START */
bool FaviconDatabase::SetPath(const base::FilePath &path) {
  if (path.EndsWithSeparator() || path.empty()) {
    return false;
  }

  base::AutoLock locker(d->mutex);
  if (d->sqlite) {
    return false;
  }

  base::FilePath dir = path.DirName();
  if (!base::DirectoryExists(dir) && !base::CreateDirectory(dir)) {
    return false;
  }

  d->path = path;
  return true;
}

void FaviconDatabase::SetPrivateBrowsingEnabled(bool enabled) {
  base::AutoLock locker(d->mutex);
  d->privateBrowsing = enabled;
}

bool FaviconDatabase::IsPrivateBrowsingEnabled() const {
  base::AutoLock locker(d->mutex);
  return d->privateBrowsing;
}

GURL FaviconDatabase::GetFaviconURLForPageURL(const GURL &pageUrl) {
  base::AutoLock locker(d->mutex);
  CHECK_OPEN_RET(GURL());

  return d->faviconUrlForPageUrl(pageUrl);
}

SkBitmap FaviconDatabase::GetBitmapForPageURL(const GURL &pageUrl) {
  base::AutoLock locker(d->mutex);
  CHECK_OPEN_RET(SkBitmap());

  GURL faviconUrl = d->faviconUrlForPageUrl(pageUrl);
  if (!faviconUrl.is_valid()) {
    return SkBitmap();
  }

  return d->bitmapForFaviconUrl(faviconUrl);
}

SkBitmap FaviconDatabase::GetBitmapForFaviconURL(const GURL &iconUrl) {
  base::AutoLock locker(d->mutex);
  CHECK_OPEN_RET(SkBitmap());

  return d->bitmapForFaviconUrl(iconUrl);
}
/* LCOV_EXCL_STOP */

void FaviconDatabase::SetFaviconURLForPageURL(const GURL &iconUrl, const GURL &pageUrl) {
  base::AutoLock locker(d->mutex);
  CHECK_OPEN;

  if (d->privateBrowsing) {
    return;
  }
  GURL &old = d->pageToFaviconUrl[pageUrl];
  if (old == iconUrl) {
    // nothing's gonna change, so just return
    return;
  }
  if (old.is_empty()) {
    // |old| is empty when it was just inserted by operator []
    // so we just assign new value to it and return
    Command *cmd = new InsertFaviconURLCommand(d.get(), pageUrl, iconUrl);
    cmd->execute();
    d->commands.push(cmd);
    ScheduleSync();
    return;
  }

  // |old| is not empty, so we have to remove it and its bitmap
  // from 'favicon url to bitmap'
  /* LCOV_EXCL_START */
  Command *cmd = new RemoveBitmapCommand(d.get(), iconUrl);
  cmd->execute();
  d->commands.push(cmd);
  // and update it in 'page url to favicon url'
  cmd = new UpdateFaviconURLCommand(d.get(), pageUrl, iconUrl);
  cmd->execute();
  d->commands.push(cmd);

  ScheduleSync();
}
/* LCOV_EXCL_STOP */

void FaviconDatabase::SetBitmapForFaviconURL(const SkBitmap &bitmap, const GURL &iconUrl) {
  base::AutoLock locker(d->mutex);
  CHECK_OPEN;

  if (d->privateBrowsing) {
    return;
  }
  if (d->faviconUrlToBitmap.find(iconUrl) != d->faviconUrlToBitmap.end()) {
    /* LCOV_EXCL_START */
    Command *cmd = new UpdateBitmapCommand(d.get(), iconUrl, bitmap);
    cmd->execute();
    d->commands.push(cmd);
    ScheduleSync();
    return;
    /* LCOV_EXCL_STOP */
  }
  Command *cmd = new InsertBitmapCommand(d.get(), iconUrl, bitmap);
  cmd->execute();
  d->commands.push(cmd);

  ScheduleSync();
}

/* LCOV_EXCL_START */
bool FaviconDatabase::ExistsForPageURL(const GURL &pageUrl) {
  base::AutoLock locker(d->mutex);
  CHECK_OPEN_RET(false);

  std::map<GURL, GURL>::const_iterator it = d->pageToFaviconUrl.find(pageUrl);
  if (it == d->pageToFaviconUrl.end()) {
    return false;
  }
  return d->existsForFaviconURL(it->second);
}
/* LCOV_EXCL_STOP */

bool FaviconDatabase::ExistsForFaviconURL(const GURL &iconUrl) {
  base::AutoLock locker(d->mutex);
  CHECK_OPEN_RET(false);

  return d->existsForFaviconURL(iconUrl);
}

/* LCOV_EXCL_START */
void FaviconDatabase::Clear() {
  base::AutoLock locker(d->mutex);
  CHECK_OPEN;

  Command *cmd = new ClearDatabaseCommand(d.get());
  cmd->execute();
  d->commands.push(cmd);

  ScheduleSync();
}
/* LCOV_EXCL_STOP */

bool FaviconDatabase::IsOpen() const {  // LCOV_EXCL_LINE
  return d->sqlite != 0;
}

bool FaviconDatabase::Open() {
  if (d->sqlite) {
    return true;
  }
  if (d->path.empty())
    return false;
  base::FilePath dir = d->path.DirName();
  if (!base::DirectoryExists(dir) && !base::CreateDirectory(dir))
    return false;
  int result = sqlite3_open(d->path.value().c_str(), &d->sqlite);
  if (result != SQLITE_OK) {
    /* LCOV_EXCL_START */
    LOG(ERROR)
        << "[FaviconDatabase] :: Error opening SQLite database ("
        << result << ")!";
    return false;
    /* LCOV_EXCL_STOP */
  }
  if (!IsDatabaseInitialized() && !InitDatabase()) {
      return false;
  }
  if (!LoadDatabase()) {
    return false;
  }
  return true;
}

void FaviconDatabase::Close() {
  if (!d->sqlite) {
    return;
  }
  sqlite3_close(d->sqlite);
  d->sqlite = 0;
}

/* LCOV_EXCL_START */
void FaviconDatabase::SyncSQLite() {
  scoped_refptr<base::SingleThreadTaskRunner> ptr = content::BrowserThread::GetTaskRunnerForThread(content::BrowserThread::DB);
  ptr->PostTask(FROM_HERE, base::Bind(&FaviconDatabasePrivate::performSync, d->weakPtrFactory.GetWeakPtr()));
}
/* LCOV_EXCL_STOP */

void FaviconDatabase::ScheduleSync() {
  if (!d->timer.IsRunning()) {
    d->timer.Start(FROM_HERE, base::TimeDelta::FromSeconds(SYNC_DELAY), this, &FaviconDatabase::SyncSQLite);
  }
}

bool FaviconDatabase::IsDatabaseInitialized() {
  std::string query("SELECT name FROM sqlite_master WHERE type='table' AND (name = ? OR name = ?);");

  sqlite3_stmt *stmt;
  int result = sqlite3_prepare_v2(d->sqlite, query.c_str(), query.size(), &stmt, 0);
  if (result != SQLITE_OK) {
    return false;
  }

  size_t stringLength = d->pageUrlToFaviconUrlTable ? strlen(d->pageUrlToFaviconUrlTable) : 0;
  result = sqlite3_bind_text(stmt, 1, d->pageUrlToFaviconUrlTable, stringLength, SQLITE_STATIC);
  if (result != SQLITE_OK) {
    /* LCOV_EXCL_START */
    sqlite3_finalize(stmt);
    return false;
    /* LCOV_EXCL_STOP */
  }
  stringLength = d->faviconUrlToBitmapTable ? strlen(d->faviconUrlToBitmapTable) : 0;
  result = sqlite3_bind_text(stmt, 2, d->faviconUrlToBitmapTable, stringLength, SQLITE_STATIC);
  if (result != SQLITE_OK) {
    /* LCOV_EXCL_START */
    sqlite3_finalize(stmt);
    return false;
    /* LCOV_EXCL_STOP */
  }

  int count = 0;
  while ((result = sqlite3_step(stmt)) == SQLITE_ROW) {
    ++count;
  }

  if (result != SQLITE_DONE || count != 2) {
    /* LCOV_EXCL_START */
    sqlite3_finalize(stmt);
    return false;
    /* LCOV_EXCL_STOP */
  }

  sqlite3_finalize(stmt);
  return true;
}

/* LCOV_EXCL_START */
bool FaviconDatabase::InitDatabase() {
  InitDatabaseCommand initCmd(d.get());
  return initCmd.execute();
}
/* LCOV_EXCL_STOP */

bool FaviconDatabase::LoadDatabase() {
  LoadDatabaseCommand loadCmd(d.get());
  return loadCmd.execute();
}
