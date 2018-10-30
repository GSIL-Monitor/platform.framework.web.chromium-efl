// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BROWSER_CONTEXT_EFL
#define BROWSER_CONTEXT_EFL

#include <string>
#include <vector>

#include "base/files/scoped_temp_dir.h"
#include "base/synchronization/lock.h"
#include "browser/download_manager_delegate_efl.h"
#include "browser/geolocation/geolocation_permission_context_efl.h"
#include "browser/special_storage_policy_efl.h"
#include "browser/ssl_host_state_delegate_efl.h"
#include "components/visitedlink/browser/visitedlink_delegate.h"
#include "components/visitedlink/browser/visitedlink_master.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/content_browser_client.h"
#include "content/public/browser/resource_context.h"
#include "content/public/browser/storage_partition.h"
#include "net/url_request/url_request_context.h"
#include "public/ewk_context_internal.h"
#include "url_request_context_getter_efl.h"
#include "wrt/dynamicplugin.h"

class CookieManager;
class PrefService;
class EWebContext;

namespace visitedlink {
class VisitedLinkMaster;
}

namespace browser_net {
class PredictorEfl;
}

typedef std::map<std::string, std::string> HTTPCustomHeadersEflMap;

namespace content {

class BrowserContextEfl : public BrowserContext,
                          public visitedlink::VisitedLinkDelegate {
 public:
  class ResourceContextEfl : public ResourceContext {
   public:
    ResourceContextEfl(scoped_refptr<CookieManager> cookie_manager);
    ~ResourceContextEfl();

    bool HTTPCustomHeaderAdd(const std::string& name, const std::string& value);
    bool HTTPCustomHeaderRemove(const std::string& name);
    void HTTPCustomHeaderClear();
    const HTTPCustomHeadersEflMap GetHTTPCustomHeadersEflMap() const;

    net::HostResolver* GetHostResolver() override;
    net::URLRequestContext* GetRequestContext() override;
    void set_url_request_context_getter(URLRequestContextGetterEfl* getter);

    scoped_refptr<CookieManager> GetCookieManager() const;

   private:
    scoped_refptr<URLRequestContextGetterEfl> getter_;
    scoped_refptr<CookieManager> cookie_manager_;
    HTTPCustomHeadersEflMap http_custom_headers_;
    mutable base::Lock http_custom_headers_lock_;

    DISALLOW_COPY_AND_ASSIGN(ResourceContextEfl);
  };

  BrowserContextEfl(EWebContext*, bool incognito = false);
  ~BrowserContextEfl();

  // BrowserContext implementation.
  net::URLRequestContextGetter* CreateMediaRequestContext() override;
  net::URLRequestContextGetter* CreateMediaRequestContextForStoragePartition(
      const base::FilePath&,
      bool) override;
  bool IsOffTheRecord() const override { return incognito_; }

  // visitedlink::VisitedLinkDelegate implementation.
  void RebuildTable(const scoped_refptr<URLEnumerator>& enumerator) override;

  ResourceContext* GetResourceContext() override;
  content::DownloadManagerDelegate* GetDownloadManagerDelegate() override {
    return &download_manager_delegate_;
  }
  BrowserPluginGuestManager* GetGuestManager() override { return 0; }
  storage::SpecialStoragePolicy* GetSpecialStoragePolicy() override;
  SpecialStoragePolicyEfl* GetSpecialStoragePolicyEfl();

  PushMessagingService* GetPushMessagingService() override;
  base::FilePath GetPath() const override;
  PermissionManager* GetPermissionManager() override;
  BackgroundSyncController* GetBackgroundSyncController() override;
  net::URLRequestContextGetter* CreateRequestContextForStoragePartition(
      const base::FilePath& partition_path,
      bool in_memory,
      ProtocolHandlerMap* protocol_handlers,
      URLRequestInterceptorScopedVector request_interceptors) override {
    return nullptr;
  }
  BackgroundFetchDelegate* GetBackgroundFetchDelegate() override;
  BrowsingDataRemoverDelegate* GetBrowsingDataRemoverDelegate() override;

  std::unique_ptr<ZoomLevelDelegate> CreateZoomLevelDelegate(
      const base::FilePath& partition_path);
#if defined(OS_TIZEN)
  void SetOffTheRecord(bool incognito) { incognito_ = incognito; }
#endif
  URLRequestContextGetterEfl* GetRequestContextEfl() {
    return request_context_getter_.get();
  }

  // These methods map to Add methods in visitedlink::VisitedLinkMaster.
  void AddVisitedURLs(const std::vector<GURL>& urls);

  // Reset visitedlink master and initialize it.
  void InitVisitedLinkMaster();
  // Reset predictorefl
  void InitPredictorEfl();
  // Returns the PredictorEfl object used for dns prefetch and preconnect.
  browser_net::PredictorEfl* GetNetworkPredictorEfl();

  ResourceContextEfl* GetResourceContextEfl();

  const GeolocationPermissionContextEfl& GetGeolocationPermissionContext()
      const;

  net::URLRequestContextGetter* CreateRequestContext(
      content::ProtocolHandlerMap* protocol_handlers,
      URLRequestInterceptorScopedVector request_interceptors);

  EWebContext* WebContext() const { return web_context_; }

  void SetCertificatePath(const std::string& certificate_path);
  const std::string& GetCertificatePath() const { return certificate_path_; }

  void CreateNetworkDelegate();
  void SetAudioLatencyMode(Ewk_Audio_Latency_Mode latency_mode);
  Ewk_Audio_Latency_Mode GetAudioLatencyMode() const;

  void SetDynamicPlugin(DynamicPlugin* dynamic_plugin) {
    dynamic_plugin_ = dynamic_plugin;
  }
  bool GetWrtParsedUrl(const GURL& url, GURL& parsed_url);

#if defined(OS_TIZEN_TV_PRODUCT)
  void UpdateWebAppDiskCachePath(const std::string& app_id);
  void SetEMPCertificatePath(const std::string& emp_ca_path,
                             const std::string& default_ca_path);
#endif

 private:
#if defined(OS_TIZEN_TV_PRODUCT)
  static bool GetCertificateVconfStatus();
  void UpdateWebAppDiskCachePathOnIOThread(const std::string& app_id);
  static void SetEMPCertificatePathOnFileThread(
      const std::string* emp_ca_path,
      const std::string* default_ca_path);
  static bool ReadEMPCertificateAndAdd(const std::string* certificate_path);
#endif
  // certificate_path should be either be a directory with CA certs, a CA cert
  // file or a colon-separated list of those. CA cert files should have *.crt
  // extension. Directories are traversed recursively.
  static void ReadCertificateAndAdd(const std::string* certificate_path);
  SSLHostStateDelegate* GetSSLHostStateDelegate() override;
  mutable std::unique_ptr<GeolocationPermissionContextEfl>
      geolocation_permission_context_;
  std::unique_ptr<visitedlink::VisitedLinkMaster> visitedlink_master_;
  std::unique_ptr<browser_net::PredictorEfl> predictor_efl_;
  ResourceContextEfl* resource_context_;
  scoped_refptr<URLRequestContextGetterEfl> request_context_getter_;
  EWebContext* web_context_;
  DownloadManagerDelegateEfl download_manager_delegate_;
  base::ScopedTempDir temp_dir_;
  bool temp_dir_creation_attempted_;
  std::unique_ptr<PrefService> user_pref_service_;
  bool incognito_;
  std::unique_ptr<net::NetworkDelegate> network_delegate_for_getter_;
  std::unique_ptr<SSLHostStateDelegateEfl> ssl_host_state_delegate_;
  std::unique_ptr<PermissionManager> permission_manager_;
  std::string certificate_path_;
  scoped_refptr<SpecialStoragePolicyEfl> special_storage_policy_efl_;
  Ewk_Audio_Latency_Mode audio_latency_mode_;
  DynamicPlugin* dynamic_plugin_;

  DISALLOW_COPY_AND_ASSIGN(BrowserContextEfl);
};
}

#endif
