// Copyright 2018 Samsung Electronics Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/media/tizen/media_storage_helper.h"

#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/path_service.h"
#include "base/strings/string_util.h"
#include "content/browser/fileapi/browser_file_system_helper.h"
#include "content/browser/renderer_host/render_process_host_impl.h"
#include "content/common/paths_efl.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/browser/web_contents.h"
#include "net/cookies/cookie_store.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_context_getter.h"

#if defined(OS_TIZEN_TV_PRODUCT)
#include "content/browser/blob_storage/chrome_blob_storage_context.h"
#include "content/browser/resource_context_impl.h"
#include "storage/browser/blob/blob_data_handle.h"
#include "storage/browser/blob/blob_data_item.h"
#include "storage/browser/blob/blob_data_snapshot.h"
#include "storage/browser/blob/blob_storage_context.h"
#endif

namespace media {

using content::BrowserThread;

#if defined(OS_TIZEN_TV_PRODUCT)
std::string ResolveBlobUrl(const content::WebContents* web_contents,
                           const GURL& url) {
  if (!web_contents)
    return std::string();

  content::BrowserContext* browser_contents = web_contents->GetBrowserContext();
  if (!browser_contents)
    return std::string();

  content::ResourceContext* resource_contents =
      browser_contents->GetResourceContext();
  if (!resource_contents)
    return std::string();

  content::ChromeBlobStorageContext* blob_storage_context =
      GetChromeBlobStorageContextForResourceContext(resource_contents);
  if (!blob_storage_context)
    return std::string();

  std::unique_ptr<storage::BlobDataHandle> handle =
      blob_storage_context->context()->GetBlobDataFromPublicURL(url);
  if (!handle)
    return std::string();

  LOG(INFO) << "Blob item, uuid :" << handle->uuid()
            << ", content type : " << handle->content_type();

  std::unique_ptr<storage::BlobDataSnapshot> data = handle->CreateSnapshot();
  if (!data)
    return std::string();

  const std::vector<scoped_refptr<storage::BlobDataItem>>& items =
      data->items();
  LOG(INFO) << "Original blob url = " << url.spec().c_str();

  if (items.size() && !items[0]->path().empty()) {
    std::string blob_url = items[0]->path().value();
    blob_url.insert(0, "file://");

    LOG(INFO) << "Real url resolved from blob url : " << blob_url.c_str();

    return blob_url;
  }

  return std::string();
}
#endif

static void ReturnResultOnCallerThread(
    const base::Callback<void(const std::string&)>& callback,
    const BrowserThread::ID caller_thread_id,
    const std::string& result) {
  BrowserThread::PostTask(caller_thread_id, FROM_HERE,
                          base::Bind(callback, result));
}

static void RequestPlaformPathFromFileSystemURL(
    const GURL& url,
    int render_process_id,
    scoped_refptr<storage::FileSystemContext> file_system_context,
    const GetPlatformPathCB& callback,
    const BrowserThread::ID caller_thread_id) {
  DCHECK_CURRENTLY_ON(BrowserThread::FILE);
  base::FilePath platform_path;
  content::SyncGetPlatformPath(file_system_context.get(), render_process_id,
                               url, &platform_path);

  LOG(INFO) << "RequestPlaformPathFromFileSystemURL platform_path "
            << platform_path.value();

  base::FilePath data_storage_path;
  PathService::Get(PathsEfl::DIR_USER_DATA, &data_storage_path);
  LOG(INFO) << "PATH SERVICE : DIR_USER_DATA " << data_storage_path.value();

  if (data_storage_path.IsParent(platform_path))
    ReturnResultOnCallerThread(callback, caller_thread_id,
                               platform_path.value());
  else
    ReturnResultOnCallerThread(callback, caller_thread_id, std::string());
}

static void CreateFile(const std::string& data,
                       const SaveDataToFileCB& callback,
                       const BrowserThread::ID caller_thread_id) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  char temp[32] = "/tmp/WebmediaXXXXXX";
  errno = 0;
  int fd = mkstemp(temp);
  if (fd < 0) {
    LOG(ERROR) << "create file fail,errno:" << errno;
    ReturnResultOnCallerThread(callback, caller_thread_id, std::string());
    return;
  }

  LOG(INFO) << "fd:" << fd << ",temp file name:" << temp;
  errno = 0;
  if (-1 == write(fd, data.c_str(), data.size())) {
    LOG(ERROR) << "write data to file fail,errno:" << errno;
    close(fd);
    ReturnResultOnCallerThread(callback, caller_thread_id, std::string());
    return;
  }
  close(fd);

  std::string url(temp);
  ReturnResultOnCallerThread(callback, caller_thread_id, url);
}

void GetPlatformPathFromURL(const content::WebContents* web_contents,
                            const GURL& url,
                            const GetPlatformPathCB& callback) {
  DCHECK(url.SchemeIsFileSystem());

  if (!web_contents) {
    LOG(ERROR) << __FUNCTION__ << " ERROR : web_contents is NULL";
    callback.Run(std::string());
    return;
  }

  content::RenderProcessHost* host = web_contents->GetRenderProcessHost();
  if (!host) {
    LOG(ERROR) << __FUNCTION__ << " ERROR : RenderProcessHost is NULL";
    callback.Run(std::string());
    return;
  }

  content::StoragePartition* partition = host->GetStoragePartition();
  storage::FileSystemContext* file_system_context =
      partition ? partition->GetFileSystemContext() : nullptr;

  scoped_refptr<storage::FileSystemContext> context(file_system_context);

  BrowserThread::ID caller_thread_id;
  if (!BrowserThread::GetCurrentThreadIdentifier(&caller_thread_id)) {
    callback.Run(std::string());
    return;
  }

  BrowserThread::PostTask(
      BrowserThread::FILE, FROM_HERE,
      base::Bind(&RequestPlaformPathFromFileSystemURL, url, host->GetID(),
                 context, callback, caller_thread_id));
}

bool GetCookiesAsync(const content::WebContents* webcontents,
                     const GURL& url,
                     const GetCookiesAsyncCB& callback) {
  if (!webcontents)
    return false;
  content::BrowserContext* browsercontent = webcontents->GetBrowserContext();
  if (!browsercontent)
    return false;

  net::CookieOptions options;
  options.set_include_httponly();
  net::URLRequestContextGetter* context_getter =
      content::BrowserContext::GetDefaultStoragePartition(browsercontent)
          ->GetURLRequestContext();
  if (!context_getter)
    return false;
  net::CookieStore* cookie_store =
      context_getter->GetURLRequestContext()->cookie_store();
  if (cookie_store) {
    BrowserThread::ID caller_thread_id;
    if (BrowserThread::GetCurrentThreadIdentifier(&caller_thread_id)) {
      cookie_store->GetCookiesWithOptionsAsync(
          url, options,
          base::Bind(&ReturnResultOnCallerThread, callback, caller_thread_id));
      return true;
    }
  }

  return false;
}

#if defined(OS_TIZEN_TV_PRODUCT)
void SaveDataToFile(const std::string& data, const SaveDataToFileCB& callback) {
  BrowserThread::ID caller_thread_id;
  if (!BrowserThread::GetCurrentThreadIdentifier(&caller_thread_id)) {
    callback.Run(std::string());
    return;
  }

  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::Bind(&CreateFile, data, callback, caller_thread_id));
}
#endif

}  // namespace media
