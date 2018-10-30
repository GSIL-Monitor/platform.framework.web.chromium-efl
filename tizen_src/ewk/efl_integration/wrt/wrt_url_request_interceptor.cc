// Copyright (c) 2014,2015 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "wrt_url_request_interceptor.h"

#include "common/content_switches_efl.h"
#include "content/public/common/content_client.h"
#include "content/public/renderer/content_renderer_client.h"
#include "net/base/filename_util.h"
#include "net/url_request/url_request_data_job.h"
#include "net/url_request/url_request_error_job.h"
#include "net/url_request/url_request_file_dir_job.h"
#include "net/url_request/url_request_file_job.h"
#include "net/url_request/url_request_redirect_job.h"
#include "net/url_request/url_request_simple_job.h"
#include "wrt/dynamicplugin.h"

#if defined(OS_TIZEN_TV_PRODUCT)
#include "net/base/mime_util.h"
#endif

namespace net {

class WrtURLRequestDataJob : public URLRequestSimpleJob {
 public:
  /* LCOV_EXCL_START */
  WrtURLRequestDataJob(URLRequest* request,
                       NetworkDelegate* network_delegate,
                       const GURL& data_url)
      : URLRequestSimpleJob(request, network_delegate) {
    data_url_ = data_url;
  }
  /* LCOV_EXCL_STOP */

  int GetData(std::string* mime_type,
              std::string* charset,
              std::string* data,
              const CompletionCallback& callback) const override;

 private:
  ~WrtURLRequestDataJob() override {}  // LCOV_EXCL_LINE
  GURL data_url_;
};

/* LCOV_EXCL_START */
int WrtURLRequestDataJob::GetData(std::string* mime_type,
                                  std::string* charset,
                                  std::string* data,
                                  const CompletionCallback& callback) const {
  if (!data_url_.is_valid())
    return ERR_INVALID_URL;

  return URLRequestDataJob::BuildResponse(data_url_, mime_type, charset, data,
                                          nullptr);
}
/* LCOV_EXCL_STOP */

#if defined(OS_TIZEN_TV_PRODUCT)
class URLRequestEncyptedFileJob : public URLRequestSimpleJob {
 public:
  URLRequestEncyptedFileJob(URLRequest* request,
                            NetworkDelegate* network_delegate,
                            const GURL& file_url,
                            DynamicPlugin* wrt_dynamic_plugin)
      : URLRequestSimpleJob(request, network_delegate) {
    file_url_ = file_url;
    dynamic_plugin_ = wrt_dynamic_plugin;
  }

  int GetData(std::string* mime_type,
              std::string* charset,
              std::string* data,
              const CompletionCallback& callback) const override;

 private:
  ~URLRequestEncyptedFileJob() override {}
  GURL file_url_;
  DynamicPlugin* dynamic_plugin_;
};

int URLRequestEncyptedFileJob::GetData(
    std::string* mime_type,
    std::string* charset,
    std::string* data,
    const CompletionCallback& callback) const {
  if (!file_url_.is_valid())
    return ERR_INVALID_URL;

  base::FilePath file_path;
  bool is_file = FileURLToFilePath(file_url_, &file_path);
  DCHECK(is_file);

  if (file_path.MatchesExtension(FILE_PATH_LITERAL(".spm")))
    file_path = file_path.RemoveFinalExtension();

  bool mime_exist = GetMimeTypeFromFile(file_path, mime_type);
  if (!mime_exist)
    *mime_type = "text/html";

  *charset = "utf-8";

  std::vector<char> buffer;
  dynamic_plugin_->GetFileDecryptedDataBuffer(
      &file_url_.possibly_invalid_spec(), &buffer);
  *data = std::string(buffer.begin(), buffer.end());

  return OK;
}
#endif

/* LCOV_EXCL_START */
WrtUrlRequestInterceptor::WrtUrlRequestInterceptor(
    const scoped_refptr<base::TaskRunner>& file_task_runner,
    const std::string& tizen_app_id,
    DynamicPlugin* dynamic_plugin)
    : URLRequestInterceptor(),
      file_task_runner_(file_task_runner),
      tizen_app_id_(tizen_app_id),
      dynamic_plugin_(dynamic_plugin) {
  LOG(INFO) << __FUNCTION__ << "() " << tizen_app_id;
}

bool WrtUrlRequestInterceptor::GetWrtParsedUrl(const GURL& url,
                                               GURL& parsed_url) const {
  std::string url_str = url.possibly_invalid_spec();
  std::string orignal_str = url_str;
  std::string parsed_url_str;

  bool is_encrypted_file;
  dynamic_plugin_->ParseURL(&url_str, &parsed_url_str, tizen_app_id_.c_str(),
                            &is_encrypted_file);

  if (!parsed_url_str.empty() && (orignal_str != parsed_url_str)) {
    parsed_url = GURL(parsed_url_str);
    return true;
  }

  return false;
}

URLRequestJob* WrtUrlRequestInterceptor::MaybeInterceptRequest(
    URLRequest* request,
    NetworkDelegate* network_delegate) const {
#if defined(OS_TIZEN_TV_PRODUCT)
  return nullptr;
#else
  if (!dynamic_plugin_->CanHandleParseUrl(request->url().scheme()))
    return nullptr;
#endif


  GURL parsed_url;

  if (GetWrtParsedUrl(request->url(), parsed_url)) {
    // Data URI scheme for WRT encryption content
    if (parsed_url.SchemeIs(url::kDataScheme))
      return new WrtURLRequestDataJob(request, network_delegate, parsed_url);
  } else {
    // Not intercept when the parsed url is the same as original or empty.
    return nullptr;
  }

  if (parsed_url.spec().empty())
    parsed_url = request->url();

  // Only file scheme is handled here directly, others redirect.
  // GetWrtParsedUrl can return other schemes such as "about:blank".
  // the actual parsed url is exposed to WebApp side after redirection.
  if (!parsed_url.SchemeIs(url::kFileScheme)) {
    return new net::URLRequestRedirectJob(
        request, network_delegate, parsed_url,
        net::URLRequestRedirectJob::REDIRECT_307_TEMPORARY_REDIRECT,
        "Wrt Request Interceptor");
  }

  base::FilePath file_path;
  const bool is_file = FileURLToFilePath(parsed_url, &file_path);

  // Check file access permissions.
  // FIXME: The third parameter of net::URLRequestErrorJob sould be
  // absolute_path.
  if (!network_delegate ||
      !network_delegate->CanAccessFile(*request, file_path, file_path))
    return new URLRequestErrorJob(request, network_delegate, ERR_ACCESS_DENIED);

  // We need to decide whether to create URLRequestFileJob for file access or
  // URLRequestFileDirJob for directory access. To avoid accessing the
  // filesystem, we only look at the path string here.
  // The code in the URLRequestFileJob::Start() method discovers that a path,
  // which doesn't end with a slash, should really be treated as a directory,
  // and it then redirects to the URLRequestFileDirJob.
  if (is_file && file_path.EndsWithSeparator() && file_path.IsAbsolute())
    return new URLRequestFileDirJob(request, network_delegate, file_path);

  // Use a regular file request job for all non-directories (including invalid
  // file names).
  return new URLRequestFileJob(request, network_delegate, file_path,
                               file_task_runner_);
}
/* LCOV_EXCL_STOP */

}  // namespace net
