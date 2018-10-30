// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser/download_manager_delegate_efl.h"

#include "base/files/file_path.h"
#include "base/path_service.h"
#include "third_party/WebKit/common/mime_util/mime_util.h"
#include "content/common/paths_efl.h"
#include "content/public/browser/download_danger_type.h"
#include "content/public/browser/download_item.h"
#include "ewk/efl_integration/browser_context_efl.h"
#include "ewk/efl_integration/eweb_view.h"

const void* const DownloadManagerDelegateEfl::kDownloadTemporaryFile =
    &DownloadManagerDelegateEfl::kDownloadTemporaryFile;

bool DownloadManagerDelegateEfl::TriggerExternalDownloadManager(
    content::DownloadItem* item) const {
  content::BrowserContextEfl* browser_context =
      static_cast<content::BrowserContextEfl*>(item->GetBrowserContext());
  if (browser_context) {
    auto start_download_callback =
        browser_context->WebContext()->DidStartDownloadCallback();
    if (start_download_callback) {
      DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
      start_download_callback->TriggerCallback(item->GetURL().spec());
      return true;
    }
  }
  return false;
}

base::FilePath DownloadManagerDelegateEfl::GetPlatformDownloadPath(
    content::DownloadItem* item) const {
  base::FilePath path;
  if (blink::IsSupportedImageMimeType(item->GetMimeType())) {
    if (!PathService::Get(PathsEfl::DIR_DOWNLOAD_IMAGE, &path)) {
      LOG(ERROR) << "Could not get image directory.";
      return base::FilePath();
    }
  } else {
    if (!PathService::Get(PathsEfl::DIR_DOWNLOADS, &path)) {
      LOG(ERROR) << "Could not get download directory.";
      return base::FilePath();
    }
  }

  return path.Append(item->GetSuggestedFilename());
}

void DownloadManagerDelegateEfl::CancelDownload(
    const content::DownloadTargetCallback& callback) const {
  callback.Run(base::FilePath(), /* Empty file path for cancellation */
               content::DownloadItem::TARGET_DISPOSITION_OVERWRITE,
               content::DOWNLOAD_DANGER_TYPE_NOT_DANGEROUS,
               base::FilePath(),
               content::DOWNLOAD_INTERRUPT_REASON_USER_CANCELED);
}

bool DownloadManagerDelegateEfl::DetermineDownloadTarget(content::DownloadItem* item,
                                                         const content::DownloadTargetCallback& callback) {
  if (item->GetUserData(kDownloadTemporaryFile))
    return false;
  if (TriggerExternalDownloadManager(item)) {
    CancelDownload(callback);
  } else {
    callback.Run(GetPlatformDownloadPath(item),
                 content::DownloadItem::TARGET_DISPOSITION_OVERWRITE,
                 content::DOWNLOAD_DANGER_TYPE_NOT_DANGEROUS,
                 GetPlatformDownloadPath(item),
                 content::DOWNLOAD_INTERRUPT_REASON_NONE);
  }
  return true;
}

bool DownloadManagerDelegateEfl::ShouldCompleteDownload(content::DownloadItem*,
                                                        const base::Closure&) {
  return true;
}

bool DownloadManagerDelegateEfl::ShouldOpenDownload(content::DownloadItem*,
                                                    const content::DownloadOpenDelayedCallback&) {
  return true;
}

void DownloadManagerDelegateEfl::GetNextId(const content::DownloadIdCallback& callback) {
  static uint32_t next_id = content::DownloadItem::kInvalidId + 1;
  callback.Run(next_id++);
}
