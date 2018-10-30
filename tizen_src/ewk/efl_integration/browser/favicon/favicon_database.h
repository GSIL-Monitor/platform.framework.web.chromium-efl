// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FAVICON_DATABASE_H
#define FAVICON_DATABASE_H

#include <string>
#include "third_party/skia/include/core/SkBitmap.h"
#include "third_party/sqlite/sqlite3.h"
#include "url/gurl.h"
#include "base/files/file_path.h"
#include "base/memory/weak_ptr.h"
#include "base/macros.h"

struct FaviconDatabasePrivate;

class FaviconDatabase {
 public:
  static FaviconDatabase *Instance();

  virtual ~FaviconDatabase();

  bool SetPath(const base::FilePath &path);
  void SetPrivateBrowsingEnabled(bool enabled);
  bool IsPrivateBrowsingEnabled() const;

  GURL GetFaviconURLForPageURL(const GURL &pageUrl);
  SkBitmap GetBitmapForPageURL(const GURL &pageUrl);
  SkBitmap GetBitmapForFaviconURL(const GURL &iconUrl);

  void SetFaviconURLForPageURL(const GURL &iconUrl, const GURL &pageUrl);
  void SetBitmapForFaviconURL(const SkBitmap &bitmap, const GURL &iconUrl);
  bool ExistsForPageURL(const GURL &pageUrl);
  bool ExistsForFaviconURL(const GURL &iconUrl);
  void Clear();

 private:
  FaviconDatabase();
  DISALLOW_COPY_AND_ASSIGN(FaviconDatabase);

  bool IsOpen() const;
  bool Open();
  void Close();
  void SyncSQLite();
  void ScheduleSync();
  bool IsDatabaseInitialized();
  bool InitDatabase();
  bool LoadDatabase();

  scoped_refptr<FaviconDatabasePrivate> d;
  static const int SYNC_DELAY;
};

#endif // FAVICON_DATABASE_H
