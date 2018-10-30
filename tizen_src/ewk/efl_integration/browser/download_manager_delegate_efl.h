// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DOWNLOAD_MANAGER_DELEGATE_EFL_H
#define DOWNLOAD_MANAGER_DELEGATE_EFL_H

#include "content/public/browser/download_manager_delegate.h"

// This is the EFL side helper for the download system.
class DownloadManagerDelegateEfl : public content::DownloadManagerDelegate {
public:
    virtual ~DownloadManagerDelegateEfl() { }
    static const void* const kDownloadTemporaryFile;

    // content::DownloadManagerDelegate implementation.
    virtual bool DetermineDownloadTarget(
        content::DownloadItem*,
        const content::DownloadTargetCallback&) override;
    virtual bool ShouldCompleteDownload(
        content::DownloadItem*,
        const base::Closure&) override;
    virtual bool ShouldOpenDownload(
        content::DownloadItem*,
        const content::DownloadOpenDelayedCallback&) override;
    virtual void GetNextId(const content::DownloadIdCallback&) override;
private:
    // If the external download manager is available, trigger download
    // with it and return true, otherwise return false.
    bool TriggerExternalDownloadManager(content::DownloadItem* item) const;

    base::FilePath GetPlatformDownloadPath(content::DownloadItem* item) const;

    void CancelDownload(const content::DownloadTargetCallback&) const;
};

#endif // DOWNLOAD_MANAGER_DELEGATE_EFL_H
