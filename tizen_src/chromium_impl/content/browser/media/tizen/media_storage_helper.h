// Copyright 2018 Samsung Electronics Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_MEDIA_TIZEN_MEDIA_STORAGE_HELPER_H_
#define CONTENT_BROWSER_MEDIA_TIZEN_MEDIA_STORAGE_HELPER_H_

#include "content/public/browser/web_contents.h"

namespace media {

// Callback to get the platform path. Args: platform path.
typedef base::Callback<void(const std::string&)> GetPlatformPathCB;

typedef base::Callback<void(const std::string&)> GetCookiesAsyncCB;

typedef base::Callback<void(const std::string&)> SaveDataToFileCB;

#if defined(OS_TIZEN_TV_PRODUCT)
std::string ResolveBlobUrl(const content::WebContents* web_contents,
                           const GURL& url);

void SaveDataToFile(const std::string& data, const SaveDataToFileCB& callback);
#endif

void GetPlatformPathFromURL(const content::WebContents* web_contents,
                            const GURL& url,
                            const GetPlatformPathCB& callback);

bool GetCookiesAsync(const content::WebContents* webcontents,
                     const GURL& url,
                     const GetCookiesAsyncCB& callback);

}  // namespace media

#endif  // CONTENT_BROWSER_MEDIA_TIZEN_MEDIA_STORAGE_HELPER_H_
