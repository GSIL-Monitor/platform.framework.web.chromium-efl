// Copyright 2013 The Chromium Authors. All rights reserved.
// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef _URL_REQUEST_CONTEXT_GETTER_EFL_H_
#define _URL_REQUEST_CONTEXT_GETTER_EFL_H_

#include <string>

#include "base/memory/weak_ptr.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/content_browser_client.h"
#include "net/base/network_delegate.h"
#include "net/proxy/proxy_config_service.h"
#include "net/url_request/url_request_context_getter.h"
#include "net/url_request/url_request_context_storage.h"
#include "net/url_request/url_request_job_factory_impl.h"
#include "public/ewk_context.h"
#include "public/ewk_cookie_manager_internal.h"

class DynamicPlugin;

namespace certificate_transparency {
class TreeStateTracker;
}

namespace net {
class CertVerifier;
class HostResolver;
class NetLog;
class URLRequestInterceptingJobFactory;
}

namespace content {

class URLRequestContextGetterEfl : public net::URLRequestContextGetter {
 public:
  URLRequestContextGetterEfl(std::unique_ptr<net::NetworkDelegate> network_delegate = std::unique_ptr<net::NetworkDelegate>(),
      bool ignore_certificate_errors = false,
      const base::FilePath& base_path = base::FilePath(),
#if defined(OS_TIZEN_TV_PRODUCT)
      // Zero means that the cache will determine the value to use.
      int cache_max_bytes = 0,
#endif
      const scoped_refptr<base::SingleThreadTaskRunner> &io_task_runner = BrowserThread::GetTaskRunnerForThread(BrowserThread::IO),
      const scoped_refptr<base::SingleThreadTaskRunner> &file_task_runner = BrowserThread::GetTaskRunnerForThread(BrowserThread::FILE),
      ProtocolHandlerMap* protocol_handlers = NULL,
      URLRequestInterceptorScopedVector request_interceptors = URLRequestInterceptorScopedVector(),
      net::NetLog* net_log = NULL);

  // net::URLRequestContextGetter implementation.
  virtual net::URLRequestContext* GetURLRequestContext() override;
  virtual scoped_refptr<base::SingleThreadTaskRunner>
      GetNetworkTaskRunner() const override;

  net::HostResolver* host_resolver();

  void SetCookieStoragePath(const base::FilePath& path,
                            bool persist_session_cookies,
                            Ewk_Cookie_Persistent_Storage file_storage);

  base::WeakPtr<URLRequestContextGetterEfl> GetWeakPtr() {
    return weak_ptr_factory_.GetWeakPtr();
  }

  void NotifyContextShuttingDown();
  void SetInterceptRequestCallback(
      Ewk_Context* ewk_context,
      Ewk_Context_Intercept_Request_Callback callback,
      void* user_data);

#if defined(OS_TIZEN_TV_PRODUCT)
  void SetInterceptRequestCancelCallback(
      Ewk_Context* ewk_context,
      Ewk_Context_Intercept_Request_Callback callback,
      void* user_data);
#endif

  void SetDynamicPlugin(const std::string& tizen_app_id,
                        DynamicPlugin* dynamic_plugin);

#if defined(OS_TIZEN_TV_PRODUCT)
  void UpdateWebAppDiskCachePath(const std::string& app_id);
#endif

 protected:
  virtual ~URLRequestContextGetterEfl();

 private:
  void CreateSQLitePersistentCookieStore(
      const base::FilePath& path,
      bool persist_session_cookies);
  void CreatePersistentCookieStore(
      const base::FilePath& path,
      bool persist_session_cookies);

  bool ignore_certificate_errors_;
  bool shut_down_;
  base::FilePath base_path_;
#if defined(OS_TIZEN_TV_PRODUCT)
  int cache_max_bytes_;
#endif
  const scoped_refptr<base::SingleThreadTaskRunner>& io_task_runner_;
  const scoped_refptr<base::SingleThreadTaskRunner>& file_task_runner_;
  net::NetLog* net_log_;

  base::FilePath cookie_store_path_;
  std::unique_ptr<net::CookieStore> cookie_store_;

  std::unique_ptr<net::ProxyConfigService> proxy_config_service_;
  std::unique_ptr<net::NetworkDelegate> network_delegate_;
  std::unique_ptr<net::URLRequestContextStorage> storage_;
  std::unique_ptr<net::URLRequestContext> url_request_context_;
  std::unique_ptr<net::CertVerifier> cert_verifier_;
  ProtocolHandlerMap protocol_handlers_;
  URLRequestInterceptorScopedVector request_interceptors_;
  std::unique_ptr<certificate_transparency::TreeStateTracker> ct_tree_tracker_;

  std::unique_ptr<net::URLRequestInterceptor> wrt_url_interceptor_;
  std::unique_ptr<net::URLRequestInterceptingJobFactory>
      wrt_url_intercepting_job_factory_;

  base::WeakPtrFactory<URLRequestContextGetterEfl> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(URLRequestContextGetterEfl);
};

}; // namespace content

#endif  // _URL_REQUEST_CONTEXT_GETTER_EFL_H_
