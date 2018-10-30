// Copyright 2014-2015 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "eweb_context.h"

#include <string>
#include <vector>

#if defined(OS_TIZEN)
#include <dlfcn.h>
#endif

#include "base/bind.h"
#include "base/strings/string_number_conversions.h"
#include "base/synchronization/waitable_event.h"
#include "browser/tizen_extensible_host.h"
#include "components/autofill/content/browser/content_autofill_driver.h"
//#include "content/browser/memory/memory_pressure_controller_impl.h"
#include "browser/favicon/favicon_database.h"
#include "browser/webdata/web_data_service_factory.h"
#include "content/browser/push_messaging/push_messaging_service_impl_efl.h"
#include "content/public/browser/appcache_service.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/dom_storage_context.h"
#include "content/public/browser/host_zoom_map.h"
#include "content/public/browser/local_storage_usage_info.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/common/page_zoom.h"
#include "net/http/http_cache.h"
#include "net/proxy/proxy_config_service_fixed.h"
#include "net/proxy/proxy_service.h"
#include "storage/browser/database/database_quota_client.h"
#include "storage/browser/database/database_tracker.h"
#include "storage/browser/fileapi/file_system_quota_client.h"
#include "storage/browser/quota/quota_manager.h"
#include "ui/gl/gl_shared_context_efl.h"

#include "browser_context_efl.h"
#include "ewk_global_data.h"
#include "browser/password_manager/password_helper_efl.h"
#include "browser/password_manager/password_store_factory.h"
#include "browser/browsing_data_remover_efl.h"
#include "browser/vibration/vibration_provider_client.h"
#include "components/password_manager/core/browser/password_manager.h"
#include "private/ewk_favicon_database_private.h"
#include "private/ewk_security_origin_private.h"
#include "wrt/dynamicplugin.h"
#include "wrt/wrt_widget_host.h"

#if defined(OS_TIZEN)
#include "base/command_line.h"
#include "common/content_switches_efl.h"
#include "content/browser/zygote_host/zygote_communication_linux.h"
#include "content/common/content_client_export.h"
#include "content/public/browser/zygote_handle_linux.h"
#include "content/public/common/content_switches.h"
#include "content_browser_client_efl.h"
#endif

#if defined(OS_TIZEN_TV_PRODUCT)
#include "base/path_service.h"
#include "common/application_type.h"
#include "content_main_delegate_efl.h"
#include "devtools_port_manager.h"
#include "url/url_util.h"
#include "wrt/hbbtv_widget_host.h"
#endif

#if defined(OS_TIZEN_TV_PRODUCT) && defined(TIZEN_PEPPER_EXTENSIONS)
#include "content/browser/permissions/pepper_permission_challenger.h"
#endif  // defined(OS_TIZEN_TV_PRODUCT) && defined(TIZEN_PEPPER_EXTENSIONS)

using content::BrowserThread;
using content::BrowserContext;
using content::BrowserContextEfl;

using std::string;
using std::pair;
using std::map;

namespace {

#if defined(OS_TIZEN)
const char* const kDynamicPreloading = "DynamicPreloading";
typedef void (*DynamicPreloading)(void);
#endif

/**
 * @brief Helper class for obtaining WebStorage origins
 */
class WebStorageGetAllOriginsDispatcher
    : public base::RefCountedThreadSafe<WebStorageGetAllOriginsDispatcher> {
 public:
  WebStorageGetAllOriginsDispatcher(
      Ewk_Web_Storage_Origins_Get_Callback callback,
      void* user_data)
      : callback_(callback), user_data_(user_data) {  // LCOV_EXCL_LINE
    DCHECK(callback_);
    DCHECK(user_data_);
  }

  /* LCOV_EXCL_START */
  void Dispatch(
      const std::vector<content::LocalStorageUsageInfo>& local_storage) {
    DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));

    Eina_List* lorigins = NULL;

    std::vector<content::LocalStorageUsageInfo>::const_iterator it;

    for (it = local_storage.begin(); it != local_storage.end(); it++) {
      _Ewk_Security_Origin* origin = new _Ewk_Security_Origin(it->origin);
      lorigins = eina_list_append(lorigins, origin);
    }

    callback_(lorigins, user_data_);
  }
  /* LCOV_EXCL_STOP */
 private:
  Ewk_Web_Storage_Origins_Get_Callback callback_;
  void* user_data_;
};

/* LCOV_EXCL_START */
void SetProxyConfigCallbackOnIOThread(
    base::WaitableEvent* done,
    net::URLRequestContextGetter* url_request_context_getter,
    const net::ProxyConfig& proxy_config) {
  net::ProxyService* proxy_service =
      url_request_context_getter->GetURLRequestContext()->proxy_service();
  proxy_service->ResetConfigService(
      base::WrapUnique(new net::ProxyConfigServiceFixed(proxy_config)));
  done->Signal();
}

void OnOriginsWithApplicationCacheObtained(
    Ewk_Web_Application_Cache_Origins_Get_Callback callback,
    void* user_data,
    scoped_refptr<content::AppCacheInfoCollection> collection,
    int result) {
  Eina_List* origins = 0;
  for (map<GURL, content::AppCacheInfoVector>::iterator iter =
           collection->infos_by_origin.begin();
       iter != collection->infos_by_origin.end(); ++iter) {
    _Ewk_Security_Origin* origin = new _Ewk_Security_Origin(iter->first);
    origins = eina_list_append(origins, origin);
  }
  callback(origins, user_data);
}

void OnTemporaryUsageAndQuotaObtained(
    Ewk_Web_Application_Cache_Usage_For_Origin_Get_Callback callback,
    void* user_data,
    storage::QuotaStatusCode status_code,
    int64_t usage,
    int64_t quota) {
  if (status_code != storage::kQuotaStatusOk) {
    LOG(ERROR) << "Error in retrieving usage information";
    // We still trigger callback.
    usage = 0;
  }
  BrowserThread::PostTask(BrowserThread::UI, FROM_HERE,
                          base::Bind(callback, usage, user_data));
}

void OnGetWebDBOrigins(Ewk_Web_Database_Origins_Get_Callback callback,
                       void* user_data,
                       const std::set<GURL>& origins_ref) {
  Eina_List* origins = 0;
  for (std::set<GURL>::iterator iter = origins_ref.begin();
       iter != origins_ref.end(); ++iter) {
    _Ewk_Security_Origin* sec_origin = new _Ewk_Security_Origin(*iter);
    origins = eina_list_append(origins, sec_origin);
  }
  BrowserThread::PostTask(BrowserThread::UI, FROM_HERE,
                          base::Bind(callback, origins, user_data));
}

void GetWebDBOriginsOnDBThread(Ewk_Web_Database_Origins_Get_Callback callback,
                               void* user_data,
                               content::StoragePartition* partition) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::DB));
  storage::DatabaseQuotaClient client(partition->GetDatabaseTracker());
  client.GetOriginsForType(storage::kStorageTypeTemporary,
                           base::Bind(&OnGetWebDBOrigins, callback, user_data));
}

void OnGetWebDBUsageForOrigin(Ewk_Web_Database_Usage_Get_Callback callback,
                              void* user_data,
                              int64_t usage) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  callback(usage, user_data);
}

void OnGetFileSystemOrigins(Ewk_Local_File_System_Origins_Get_Callback callback,
                            void* user_data,
                            const std::set<GURL>& origins_ref) {
  Eina_List* origins = 0;
  for (std::set<GURL>::iterator iter = origins_ref.begin();
       iter != origins_ref.end(); ++iter) {
    _Ewk_Security_Origin* sec_origin = new _Ewk_Security_Origin(*iter);
    origins = eina_list_append(origins, sec_origin);
  }
  BrowserThread::PostTask(BrowserThread::UI, FROM_HERE,
                          base::Bind(callback, origins, user_data));
}

void GetFileSystemOriginsOnFILEThread(
    Ewk_Web_Database_Origins_Get_Callback callback,
    void* user_data,
    content::StoragePartition* partition) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::FILE));
  std::unique_ptr<storage::FileSystemQuotaClient> client(
      new storage::FileSystemQuotaClient(partition->GetFileSystemContext(),
                                         false));

  client->GetOriginsForType(
      storage::kStorageTypeTemporary,
      base::Bind(&OnGetFileSystemOrigins, callback, user_data));
}

void OnGetPasswordDataList(
    Ewk_Context_Form_Password_Data_List_Get_Callback callback,
    void* user_data,
    const std::vector<std::unique_ptr<autofill::PasswordForm>>& results) {
  Eina_List* list = nullptr;

  for (const auto& form : results) {
    auto data = new Ewk_Password_Data;
    data->url = strdup(form->origin.possibly_invalid_spec().c_str());
    // NOTE: Fingerprint is not supported yet, so it is always FALSE.
    data->useFingerprint = EINA_FALSE;

    list = eina_list_append(list, data);
  }

  callback(list, user_data);
}

} // namespace

/* LCOV_EXCL_STOP */
/* LCOV_EXCL_START */
void EwkDidStartDownloadCallback::TriggerCallback(const string& url) {
  if (callback_)
    (*callback_)(url.c_str(), user_data_);
}

bool EwkMimeOverrideCallback::TriggerCallback(const std::string& url_spec,
                                              const std::string& mime_type,
                                              char** new_mime_type) const {
  if (!callback_)
    return false;
  Eina_Bool result = (*callback_)(url_spec.c_str(), mime_type.c_str(),
                                  new_mime_type, user_data_);
  return result;
}

#if defined(OS_TIZEN_TV_PRODUCT)
bool EwkCheckAccessiblePathCallback::TriggerCallback(
    const std::string& url_spec,
    const std::string& tizen_app_id) const {
  if (!callback_)
    return true;

  Eina_Bool result =
      (*callback_)(tizen_app_id.c_str(), url_spec.c_str(), user_data_);
  return result == EINA_TRUE;
}
#endif

/* LCOV_EXCL_STOP */

void EwkPushMessageCallback::Trigger(const std::string& sender_id,
                                     const std::string& push_data) const {
  (*callback_)(sender_id.c_str(), push_data.c_str(), user_data_);
}

/* LCOV_EXCL_START */
void EWebContext::SetWidgetInfo(const std::string& tizen_app_id,
                                double scale,
                                const string& theme,
                                const string& encoded_bundle) {
  tizen_app_id_ = tizen_app_id;
  widget_scale_ = scale;
  widget_theme_ = theme;
  widget_encoded_bundle_ = encoded_bundle;

#if defined(OS_TIZEN)
  base::CommandLine& command_line = *base::CommandLine::ForCurrentProcess();
  // Add the switches in here to load injected bundle in renderer thread
  // in single process mode
  if (command_line.HasSwitch(switches::kSingleProcess)) {
    command_line.AppendSwitchASCII(switches::kInjectedBundlePath,
                                   injected_bundle_path_);
    command_line.AppendSwitchASCII(switches::kTizenAppId, tizen_app_id_);
    command_line.AppendSwitchASCII(switches::kWidgetScale,
                                   base::DoubleToString(widget_scale_));
    command_line.AppendSwitchASCII(switches::kWidgetTheme, widget_theme_);
    command_line.AppendSwitchASCII(switches::kWidgetEncodedBundle,
                                   widget_encoded_bundle_);
/* LCOV_EXCL_STOP */

#if defined(OS_TIZEN_TV_PRODUCT)
    // Drop process privillages while web app is launching
    // from WRT process pool. It should be handled in webengine side
    // in product tv profile because this profile uses the zygote process.
    content::GetGenericZygote()->DropProcessPrivileges(
        tizen_app_id_);  // LCOV_EXCL_LINE
#endif  // OS_TIZEN_TV_PRODUCT
  } else {
    // Drop process privillages while web app is launching
    // from WRT process pool. It is not necessary in single process mode,
    // because it is handled in crosswalk side in single process mode.
    content::GetGenericZygote()->DropProcessPrivileges(tizen_app_id_);
  }

  // Set DynamicPlugin to convert file scheme(file://) for Tizen Webapp.
  /* LCOV_EXCL_START */
  if (!tizen_app_id_.empty() && !injected_bundle_path_.empty()) {
    V8Widget::Type type = V8Widget::Type::WRT;
#if defined(OS_TIZEN_TV_PRODUCT)
    if (tizen_app_id_ == "org.tizen.hbbtv")
      type = V8Widget::Type::HBBTV;
#endif
    DynamicPlugin* const plugin = &DynamicPlugin::Get(type);
    if (plugin->Init(injected_bundle_path_)) {
      browser_context_->SetDynamicPlugin(plugin);
      browser_context_->GetRequestContextEfl()->SetDynamicPlugin(tizen_app_id_,
                                                                 plugin);
    }
    /* LCOV_EXCL_STOP */
  }

  // Keep tizen app id into CommandLine in order to access it
  // from all over the browser process.
  base::CommandLine::ForCurrentProcess()->AppendSwitchASCII(
      switches::kTizenAppId, tizen_app_id_);
#endif  // OS_TIZEN

#if defined(OS_TIZEN_TV_PRODUCT)
  // Since Web Browser also called ewk_context_tizen_app_id_set,
  // so need to check the application type here.
  if (application_type_ == EWK_APPLICATION_TYPE_TIZENWRT) {
    browser_context_->UpdateWebAppDiskCachePath(tizen_app_id_);
    UpdateStoragePartitionPath();
  }
#endif

#if defined(TIZEN_PEPPER_EXTENSIONS)
  if (application_type_ == EWK_APPLICATION_TYPE_TIZENWRT)
    content::PepperPermissionChallenger::GetInstance()->PopulateSwitches();
#endif
}  // LCOV_EXCL_LINE

bool EWebContext::SetAppVersion(const std::string& tizen_app_version) {
  if (tizen_app_id_.empty()) {
    LOG(ERROR) << "Please set tizen_app_id first";
    return false;
  }

  tizen_app_version_ = tizen_app_version;

#if defined(OS_TIZEN)
  base::CommandLine& command_line = *base::CommandLine::ForCurrentProcess();
  command_line.AppendSwitchASCII(switches::kTizenAppVersion, tizen_app_version);

  return true;
#else
  DLOG(WARNING) << "OS_TIZEN is not enabled";
  return false;
#endif
}

/* LCOV_EXCL_START */
void EWebContext::SendWrtMessage(const Ewk_Wrt_Message_Data& data) {
  WrtWidgetHost::Get()->SendWrtMessage(data);
}

void EWebContext::SetMimeOverrideCallback(
    Ewk_Context_Override_Mime_For_Url_Callback callback,
    void* user_data) {
  mime_override_callback_.reset(
      new EwkMimeOverrideCallback(callback, user_data));
}
/* LCOV_EXCL_STOP */

bool EWebContext::OverrideMimeForURL(const std::string& url_spec,
                                     const std::string& mime_type,
                                     std::string& new_mime_type) const {
  if (!mime_override_callback_)
    return false;
  /* LCOV_EXCL_START */
  char* new_mime_type_string = NULL;
  bool overridden = mime_override_callback_->TriggerCallback(
      url_spec, mime_type, &new_mime_type_string);
  if (overridden) {
    DCHECK(new_mime_type_string);
    new_mime_type.assign(new_mime_type_string);
    ::free(new_mime_type_string);
    return true;
  }
  /* LCOV_EXCL_STOP */
  return false;
}

EWebContext::EWebContext(bool incognito, const std::string& injectedBundlePath)
    : injected_bundle_path_(injectedBundlePath),
      cache_enable_(false),
#if defined(OS_TIZEN)
      injected_bundle_handle_(nullptr),
#endif
      widget_scale_(0),
      m_pixmap(0) {
#if defined(OS_TIZEN_TV_PRODUCT)
  m_isShowingMappingInfo = false;
  application_type_ = EWK_APPLICATION_TYPE_WEBBROWSER;
#endif
#if defined(OS_TIZEN)
  // In order to support process pool for WRT, injected bundle should be
  // preloaded on zygote process after first intialization of EwkGlobalData.
  /* LCOV_EXCL_START */
  bool should_preload_injected_bundle =
      (!injected_bundle_path_.empty() && !EwkGlobalData::IsInitialized());
#endif
  CHECK(EwkGlobalData::GetInstance());

  // WRT does not really care about incognito, so set it to false
  browser_context_.reset(new BrowserContextEfl(this, incognito));

  // This indirectly creates RequestContext, ResourceContextEfl
  // and CookieManager instances associated to this context.
  // These objects end up getting created as part of the
  // EWebView::InitializeContents routine anyways.
  // The reason on why it is manually triggered here is because
  // there are ewk_cookie_manager APIs that can be called before a
  // WebView instance is created. In such circumstances, APIs including
  // ewk_cookie_manager_persistent_storage_set fail to execute.
  BrowserContext::GetDefaultStoragePartition(browser_context())
      ->GetURLRequestContext();

  // Notification Service gets init in BrowserMainRunner init,
  // so cache manager can register for notifications only after that.
  web_cache_manager_.reset(new WebCacheManagerEfl(browser_context_.get()));
  ewk_favicon_database_.reset(new EwkFaviconDatabase(this));
  notification_cb_.reset(
      new EWebContextNotificationCallback(nullptr, nullptr, nullptr, nullptr));

  TizenExtensibleHost::GetInstance()->Initialize();

#if defined(OS_TIZEN)
  if (should_preload_injected_bundle) {
    base::CommandLine& command_line = *base::CommandLine::ForCurrentProcess();
    if (command_line.HasSwitch(switches::kSingleProcess)) {
      // Preload injected bundle on here for process pool,
      // because the zygote process doesn't exist in single process mode.
      // The loaded handle must be closed on termination.
      injected_bundle_handle_ =
          dlopen(injected_bundle_path_.c_str(), RTLD_LAZY);
      if (!injected_bundle_handle_) {
        LOG(ERROR) << "No handle to " << injected_bundle_path_.c_str()
                   << " error " << dlerror();
        return;
      }

      DynamicPreloading dp = reinterpret_cast<DynamicPreloading>(
          dlsym(injected_bundle_handle_, kDynamicPreloading));
      if (dp) {
        dp();
      } else {
        LOG(ERROR) << "Fail to load symbol '" << kDynamicPreloading
                   << "', error " << dlerror();
      }
    } else {
      // Preload injected bundle on zygote process for process pool.
      content::GetGenericZygote()
          ->LoadInjectedBundlePath(injected_bundle_path_);
      /* LCOV_EXCL_STOP */
    }
  }
#endif
}

EWebContext::~EWebContext() {
#if defined(OS_TIZEN)
  if (injected_bundle_handle_)
    dlclose(injected_bundle_handle_);  // LCOV_EXCL_LINE

  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kSingleProcess)) {
    // On single process mode, renderer thread works until ewk_shutdown.
    // So we have to remain BrowserContextEfl include of ResourceContextEfl.
    // It will be delete once ContentBrowserClientEfl is destroyed.
    /* LCOV_EXCL_START */
    content::ContentBrowserClient* cbc =
        content::GetContentClientExport()->browser();
    auto cbce = static_cast<content::ContentBrowserClientEfl*>(cbc);
    cbce->SetBrowserContext(browser_context_.release());
    /* LCOV_EXCL_STOP */
  }
#endif
}

void EWebContext::ClearNetworkCache() {
  BrowsingDataRemoverEfl* remover =
      BrowsingDataRemoverEfl::CreateForUnboundedRange(browser_context_.get());
  remover->ClearNetworkCache();
  // remover deletes itself once it is done with clearing operation.
  return;
}

void EWebContext::ClearWebkitCache() {
  if (web_cache_manager_)
    web_cache_manager_->ClearCache();
}

void EWebContext::SetCacheModel(Ewk_Cache_Model model) {
  if (web_cache_manager_)
    web_cache_manager_->SetCacheModel(model);
}

Ewk_Cache_Model EWebContext::GetCacheModel() const {
  return web_cache_manager_->GetCacheModel();
}

/* LCOV_EXCL_START */
void EWebContext::SetNetworkCacheEnableOnIOThread(
    bool enable,
    net::URLRequestContextGetter* url_context) {
  if (!url_context)
    return;

  net::HttpTransactionFactory* transaction_factory =
      url_context->GetURLRequestContext()->http_transaction_factory();
  if (!transaction_factory)
    return;

  net::HttpCache* http_cache = transaction_factory->GetCache();
  if (!http_cache)
    return;

  http_cache->set_mode(enable ? net::HttpCache::NORMAL
                              : net::HttpCache::DISABLE);
}

void EWebContext::SetNetworkCacheEnable(bool enable) {
  if (enable == cache_enable_)
    return;

  cache_enable_ = enable;

  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::Bind(&EWebContext::SetNetworkCacheEnableOnIOThread,
                 base::Unretained(this), cache_enable_,
                 base::RetainedRef(BrowserContext::GetDefaultStoragePartition(
                                       browser_context())
                                       ->GetURLRequestContext())));
}

void EWebContext::SetCertificatePath(const std::string& certificate_path) {
  browser_context_->SetCertificatePath(certificate_path);
}

const std::string& EWebContext::GetCertificatePath() const {
  return browser_context_->GetCertificatePath();
}

void EWebContext::NotifyLowMemory() {
  // FIXME(a1.gomes): ChromeOS, Win and Mac have monitors that listen to system
  // OOO notification, and act accordingly.
  // Also, newer chromium rebases (m49+) have a new API for this named
  // MemoryPressureController::SendPressureNotification. Use it when available.
  DLOG(WARNING) << "Releasing as much memory as possible.";
  base::MemoryPressureListener::SetNotificationsSuppressed(
      base::MemoryPressureListener::MEMORY_PRESSURE_LEVEL_CRITICAL);
}

bool EWebContext::HTTPCustomHeaderAdd(const std::string& name,
                                      const std::string& value) {
  BrowserContextEfl::ResourceContextEfl* rc =
      browser_context_->GetResourceContextEfl();
  if (!rc)
    return false;
  return rc->HTTPCustomHeaderAdd(name, value);
}

bool EWebContext::HTTPCustomHeaderRemove(const std::string& name) {
  BrowserContextEfl::ResourceContextEfl* rc =
      browser_context_->GetResourceContextEfl();
  if (!rc)
    return false;
  return rc->HTTPCustomHeaderRemove(name);
}

void EWebContext::HTTPCustomHeaderClear() {
  BrowserContextEfl::ResourceContextEfl* rc =
      browser_context_->GetResourceContextEfl();
  if (!rc)
    return;
  rc->HTTPCustomHeaderClear();
}

void EWebContext::SetProxy(const char* uri, const char* bypass_rule) {
  net::ProxyConfig config;
  proxy_uri_ = (uri != nullptr) ? string(uri) : string();
  proxy_bypass_rule_ =
      (bypass_rule != nullptr) ? string(bypass_rule) : string();

  config.proxy_rules().ParseFromString(proxy_uri_);
  config.proxy_rules().bypass_rules.ParseFromString(proxy_bypass_rule_);

  base::WaitableEvent done(base::WaitableEvent::ResetPolicy::AUTOMATIC,
                           base::WaitableEvent::InitialState::NOT_SIGNALED);
  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::Bind(&SetProxyConfigCallbackOnIOThread, &done,
                 base::RetainedRef(BrowserContext::GetDefaultStoragePartition(
                                       browser_context())
                                       ->GetURLRequestContext()),
                 config));
  done.Wait();
}
/* LCOV_EXCL_STOP */

#if defined(OS_TIZEN_TV_PRODUCT)
void EWebContext::SetProxyDefaultAuth(const char* username,
                                      const char* password) {
  proxy_username_ = (username != nullptr) ? string(username) : string();
  proxy_password_ = (password != nullptr) ? string(password) : string();
}

bool EWebContext::SetPWAInfo(const std::string& pwa_storage_path) {
  content::StoragePartition* storage_partition =
      BrowserContext::GetDefaultStoragePartition(browser_context_.get());
  if (!storage_partition) {
    LOG(INFO) << "no storage_partition";
    return false;
  }
  base::FilePath data_path = base::FilePath(pwa_storage_path);
  base::FilePath origin_path = browser_context_->GetPath();
  if (data_path.value() == origin_path.value()) {
    LOG(INFO) << "It is already set to that path.";
    return true;
  }

  PathService::Clear();
  LOG(INFO) << "origin_path: " << origin_path.AsUTF8Unsafe()
            << ", pwa_storage_path: " << data_path.AsUTF8Unsafe();
  storage_partition->UpdatePath(data_path);
  return true;
}
#endif

/* LCOV_EXCL_START */
void EWebContext::SetDidStartDownloadCallback(
    Ewk_Context_Did_Start_Download_Callback callback,
    void* user_data) {
  DCHECK(start_download_callback_.get() == NULL);
  start_download_callback_.reset(
      new EwkDidStartDownloadCallback(callback, user_data));
}

EwkDidStartDownloadCallback* EWebContext::DidStartDownloadCallback() {
  return start_download_callback_.get();
}
/* LCOV_EXCL_STOP */

Ewk_Cookie_Manager* EWebContext::ewkCookieManager() {
  if (!ewk_cookie_manager_)
    ewk_cookie_manager_.reset(Ewk_Cookie_Manager::create());
  return ewk_cookie_manager_.get();
}

/* LCOV_EXCL_START */
void EWebContext::DeleteAllApplicationCache() {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  BrowsingDataRemoverEfl* remover =
      BrowsingDataRemoverEfl::CreateForUnboundedRange(browser_context_.get());
  remover->RemoveImpl(BrowsingDataRemoverEfl::REMOVE_APPCACHE, GURL());
}
/* LCOV_EXCL_STOP */

/* LCOV_EXCL_START */
void EWebContext::DeleteApplicationCacheForSite(const GURL& site) {
  content::StoragePartition* partition =
      BrowserContext::GetStoragePartitionForSite(browser_context_.get(), site);
  partition->ClearDataForOrigin(
      content::StoragePartition::REMOVE_DATA_MASK_APPCACHE,
      content::StoragePartition::QUOTA_MANAGED_STORAGE_MASK_ALL, site,
      partition->GetURLRequestContext(), base::Bind(&base::DoNothing));
}

void EWebContext::GetAllOriginsWithApplicationCache(
    Ewk_Web_Application_Cache_Origins_Get_Callback callback,
    void* user_data) {
  content::StoragePartition* partition =
      BrowserContext::GetStoragePartition(browser_context_.get(), NULL);

  scoped_refptr<content::AppCacheInfoCollection> collection(
      new content::AppCacheInfoCollection());
  // As per comments on AppCacheService,
  // there is only one instance of AppCacheService per profile.(i.e. context in
  // our case).
  // So, we don't need to iterate over all StoragePartitions.
  partition->GetAppCacheService()->GetAllAppCacheInfo(
      collection.get(), base::Bind(&OnOriginsWithApplicationCacheObtained,
                                   callback, user_data, collection));
}

void EWebContext::GetApplicationCacheUsage(
    const GURL& url,
    Ewk_Web_Application_Cache_Usage_For_Origin_Get_Callback callback,
    void* user_data) {
  content::StoragePartition* partition =
      BrowserContext::GetStoragePartition(browser_context_.get(), NULL);

  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::Bind(
          &storage::QuotaManager::GetUsageAndQuota,
          partition->GetQuotaManager(), url, storage::kStorageTypeTemporary,
          base::Bind(&OnTemporaryUsageAndQuotaObtained, callback, user_data)));
}
/* LCOV_EXCL_STOP */

void EWebContext::GetLocalStorageUsageForOrigin(  // LCOV_EXCL_LINE
    const GURL& origin,
    Ewk_Web_Storage_Usage_Get_Callback callback,
    void* user_data) {
  content::StoragePartition* partition = BrowserContext::GetStoragePartition(
      browser_context_.get(), NULL);  // LCOV_EXCL_LINE

  /* LCOV_EXCL_START */
  partition->GetDOMStorageContext()->GetLocalStorageUsage(
      base::Bind(&EWebContext::LocalStorageUsageForOrigin, origin,
                 base::Unretained(callback), base::Unretained(user_data)));
}
/* LCOV_EXCL_STOP */

void EWebContext::
    LocalStorageUsageForOrigin(/* LCOV_EXCL_START */
                               const GURL& origin,
                               Ewk_Web_Storage_Usage_Get_Callback callback,
                               void* user_data,
                               const std::vector<
                                   content::LocalStorageUsageInfo>&
                                   local_storage) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));

  /* LCOV_EXCL_START */
  for (const auto& elem : local_storage) {
    if (elem.origin == origin) {
      callback(elem.data_size, user_data);
      return;
      /* LCOV_EXCL_STOP */
    }
  }
  callback(0, user_data); /* LCOV_EXCL_START */
}

void EWebContext::WebStorageDelete() {
  if (!BrowserThread::CurrentlyOn(BrowserThread::UI)) {
    /* LCOV_EXCL_START */
    BrowserThread::PostTask(
        BrowserThread::UI, FROM_HERE,
        base::Bind(&EWebContext::WebStorageDelete, base::Unretained(this)));
    return;
    /* LCOV_EXCL_STOP */
  }
  BrowsingDataRemoverEfl* remover =
      BrowsingDataRemoverEfl::CreateForUnboundedRange(browser_context_.get());
  remover->RemoveImpl(BrowsingDataRemoverEfl::REMOVE_LOCAL_STORAGE, GURL());
  remover->RemoveImpl(BrowsingDataRemoverEfl::REMOVE_SESSION_STORAGE, GURL());
}

/* LCOV_EXCL_START */
void EWebContext::WebStorageDeleteForOrigin(const GURL& origin) {
  if (!BrowserThread::CurrentlyOn(BrowserThread::UI)) {
    BrowserThread::PostTask(BrowserThread::UI, FROM_HERE,
                            base::Bind(&EWebContext::WebStorageDeleteForOrigin,
                                       base::Unretained(this), origin));
    return;
  }
  content::StoragePartition* partition =
      BrowserContext::GetStoragePartition(browser_context_.get(), NULL);

  partition->GetDOMStorageContext()->DeleteLocalStorage(origin);
}

void EWebContext::WebStorageOriginsAllGet(
    Ewk_Web_Storage_Origins_Get_Callback callback,
    void* user_data) {
  content::StoragePartition* partition =
      BrowserContext::GetStoragePartition(browser_context_.get(), NULL);

  WebStorageGetAllOriginsDispatcher* dispatcher =
      new WebStorageGetAllOriginsDispatcher(callback, user_data);

  partition->GetDOMStorageContext()->GetLocalStorageUsage(
      base::Bind(&WebStorageGetAllOriginsDispatcher::Dispatch, dispatcher));
}
/* LCOV_EXCL_STOP */

void EWebContext::IndexedDBDelete() {
  if (!BrowserThread::CurrentlyOn(BrowserThread::UI)) {
    /* LCOV_EXCL_START */
    BrowserThread::PostTask(
        BrowserThread::UI, FROM_HERE,
        base::Bind(&EWebContext::IndexedDBDelete, base::Unretained(this)));
    return;
    /* LCOV_EXCL_STOP */
  }
  BrowsingDataRemoverEfl* remover =
      BrowsingDataRemoverEfl::CreateForUnboundedRange(browser_context_.get());
  remover->RemoveImpl(BrowsingDataRemoverEfl::REMOVE_INDEXEDDB, GURL());
}

/* LCOV_EXCL_START */
void EWebContext::WebDBDelete(const GURL& host) {
  if (!BrowserThread::CurrentlyOn(BrowserThread::UI)) {
    BrowserThread::PostTask(
        BrowserThread::UI, FROM_HERE,
        base::Bind(&EWebContext::WebDBDelete, base::Unretained(this), host));
    return;
  }
  BrowsingDataRemoverEfl* remover =
      BrowsingDataRemoverEfl::CreateForUnboundedRange(browser_context_.get());
  remover->RemoveImpl(BrowsingDataRemoverEfl::REMOVE_WEBSQL, host);
}

void EWebContext::GetAllOriginsWithWebDB(
    Ewk_Web_Database_Origins_Get_Callback callback,
    void* user_data) {
  content::StoragePartition* partition =
      BrowserContext::GetStoragePartition(browser_context_.get(), NULL);
  BrowserThread::PostTask(
      BrowserThread::DB, FROM_HERE,
      base::Bind(&GetWebDBOriginsOnDBThread, callback, user_data, partition));
}

void EWebContext::GetWebDBUsageForOrigin(
    const GURL& origin,
    Ewk_Web_Database_Usage_Get_Callback callback,
    void* user_data) {
  content::StoragePartition* partition =
      BrowserContext::GetStoragePartition(browser_context_.get(), nullptr);
  storage::DatabaseQuotaClient client(partition->GetDatabaseTracker());
  client.GetOriginUsage(
      origin, storage::kStorageTypeTemporary,
      base::Bind(&OnGetWebDBUsageForOrigin, callback, user_data));
}

void EWebContext::FileSystemDelete(const GURL& host) {
  if (!BrowserThread::CurrentlyOn(BrowserThread::UI)) {
    BrowserThread::PostTask(BrowserThread::UI, FROM_HERE,
                            base::Bind(&EWebContext::FileSystemDelete,
                                       base::Unretained(this), host));
    return;
  }
  BrowsingDataRemoverEfl* remover =
      BrowsingDataRemoverEfl::CreateForUnboundedRange(browser_context_.get());
  remover->RemoveImpl(BrowsingDataRemoverEfl::REMOVE_FILE_SYSTEMS, host);
}

void EWebContext::GetAllOriginsWithFileSystem(
    Ewk_Local_File_System_Origins_Get_Callback callback,
    void* user_data) const {
  content::StoragePartition* partition =
      BrowserContext::GetStoragePartition(browser_context_.get(), NULL);
  BrowserThread::PostTask(BrowserThread::FILE, FROM_HERE,
                          base::Bind(&GetFileSystemOriginsOnFILEThread,
                                     callback, user_data, partition));
}

bool EWebContext::SetFaviconDatabasePath(const base::FilePath& path) {
  return FaviconDatabase::Instance()->SetPath(path);
}
/* LCOV_EXCL_STOP */

Evas_Object* EWebContext::AddFaviconObject(const char* uri,  // LCOV_EXCL_LINE
                                           Evas* canvas) const {
  if (uri == NULL || canvas == NULL) {  // LCOV_EXCL_LINE
    return NULL;
  }
  SkBitmap bitmap = FaviconDatabase::Instance()->GetBitmapForPageURL(
      GURL(uri));         // LCOV_EXCL_LINE
  if (bitmap.isNull()) {  // LCOV_EXCL_LINE
    return NULL;
  }

  /* LCOV_EXCL_START */
  Evas_Object* favicon = evas_object_image_filled_add(canvas);
  evas_object_image_size_set(favicon, bitmap.width(), bitmap.height());
  evas_object_image_colorspace_set(favicon, EVAS_COLORSPACE_ARGB8888);
  evas_object_image_fill_set(favicon, 0, 0, bitmap.width(), bitmap.height());
  evas_object_image_filled_set(favicon, EINA_TRUE);
  evas_object_image_alpha_set(favicon, EINA_TRUE);
  evas_object_image_data_copy_set(favicon, bitmap.getPixels());

  return favicon;
  /* LCOV_EXCL_STOP */
}

void EWebContext::ClearCandidateData() {
#if defined(TIZEN_AUTOFILL_SUPPORT)
  if (!BrowserThread::CurrentlyOn(BrowserThread::UI)) {
    /* LCOV_EXCL_START */
    BrowserThread::PostTask(
        BrowserThread::UI, FROM_HERE,
        base::Bind(&EWebContext::ClearCandidateData, base::Unretained(this)));
    return;
    /* LCOV_EXCL_STOP */
  }
  WebDataServiceFactory* webDataServiceInstance =
      WebDataServiceFactory::GetInstance();
  scoped_refptr<autofill::AutofillWebDataService> autofillWebDataService =
      webDataServiceInstance->GetAutofillWebDataForProfile();
  if (autofillWebDataService.get()) {
    // RemoveFormElementsAddedBetween will schedule task on proper thread,
    // it is done in WebDatabaseService::ScheduleDBTask
    autofillWebDataService->RemoveFormElementsAddedBetween(base::Time(),
                                                           base::Time::Max());
  } else {
    DLOG(WARNING) << "AutofillWebDataService is NULL";
  }
#else
  DLOG(WARNING) << "TIZEN_AUTOFILL_SUPPORT is not enabled";
#endif
}

void EWebContext::ClearPasswordData() {
#if defined(TIZEN_AUTOFILL_SUPPORT)
  password_helper::RemoveLogins(
      password_manager::PasswordStoreFactory::GetPasswordStore());
#else
  DLOG(WARNING) << "TIZEN_AUTOFILL_SUPPORT is not enabled";
#endif
}

void EWebContext::ClearPasswordDataForUrl(const char* url) {
#if defined(TIZEN_AUTOFILL_SUPPORT)
  password_helper::RemoveLogin(
      password_manager::PasswordStoreFactory::GetPasswordStore(), GURL(url));
#else
  DLOG(WARNING) << "TIZEN_AUTOFILL_SUPPORT is not enabled";
#endif
}

void EWebContext::GetPasswordDataList(
    Ewk_Context_Form_Password_Data_List_Get_Callback callback,
    void* user_data) {
#if defined(TIZEN_AUTOFILL_SUPPORT)
  password_helper::GetLogins(
      password_manager::PasswordStoreFactory::GetPasswordStore(),
      base::Bind(&OnGetPasswordDataList, base::Unretained(callback),
                 base::Unretained(user_data)));
#else
  DLOG(WARNING) << "TIZEN_AUTOFILL_SUPPORT is not enabled";
#endif
}

#if defined(OS_TIZEN_TV_PRODUCT)
bool EWebContext::GetInspectorServerState() {
  if (content::DevToolsDelegateEfl::getPort())
    return true;
  else
    return false;
}

bool EWebContext::ShowInspectorPortInfoState() {
  if (m_isShowingMappingInfo) {
    m_isShowingMappingInfo = false;
    if (content::DevToolsDelegateEfl::getPort())
      return true;
  }

  return false;
}
#endif

unsigned int EWebContext::InspectorServerStart(
    unsigned int port) {  // LCOV_EXCL_LINE
#if defined(OS_TIZEN_TV_PRODUCT)
  m_isShowingMappingInfo = true;
  EwkGlobalData::GetInstance()->GetContentMainDelegatEfl().SetInspectorStatus(
      true);
  if (content::DevToolsDelegateEfl::getPort())
    return content::DevToolsDelegateEfl::getPort();
#endif
  InspectorServerStop();                             // LCOV_EXCL_LINE
  return content::DevToolsDelegateEfl::Start(port);  // LCOV_EXCL_LINE
}

bool EWebContext::InspectorServerStop() {  // LCOV_EXCL_LINE
#if defined(OS_TIZEN_TV_PRODUCT)
  if (!content::DevToolsDelegateEfl::getPort())
    return false;
  m_isShowingMappingInfo = false;
  EwkGlobalData::GetInstance()->GetContentMainDelegatEfl().SetInspectorStatus(
      false);
#endif
  return content::DevToolsDelegateEfl::Stop();  // LCOV_EXCL_LINE
}

/* LCOV_EXCL_START */
void EWebContext::SetNotificationCallbacks(
    Ewk_Context* context,
    Ewk_Context_Notification_Show_Callback show_callback,
    Ewk_Context_Notification_Cancel_Callback cancel_callback,
    void* user_data) {
  notification_cb_.reset(new EWebContextNotificationCallback(
      context, show_callback, cancel_callback, user_data));
}

bool EWebContext::HasNotificationCallbacks() const {
  return notification_cb_->HasShowCallback() &&
         notification_cb_->HasCancelCallback();
}

bool EWebContext::NotificationShowCallback(Ewk_Notification* notification) {
  return notification_cb_->RunShowCallback(notification);
}

bool EWebContext::NotificationCancelCallback(uint64_t notification_id) {
  return notification_cb_->RunCancelCallback(notification_id);
}
/* LCOV_EXCL_STOP */

bool EWebContext::SetExtensibleAPI(const std::string& api_name, bool enable) {
  return TizenExtensibleHost::GetInstance()->SetExtensibleAPI(api_name, enable);
}

bool EWebContext::GetExtensibleAPI(const std::string& api_name) {
  return TizenExtensibleHost::GetInstance()->GetExtensibleAPI(api_name);
}

/* LCOV_EXCL_START */
void EWebContext::SetDefaultZoomFactor(double zoom_factor) {
  content::HostZoomMap* const host_zoom_map =
      content::HostZoomMap::GetDefaultForBrowserContext(browser_context_.get());
  if (host_zoom_map) {
    host_zoom_map->SetDefaultZoomLevel(
        content::ZoomFactorToZoomLevel(zoom_factor));
  }
}

double EWebContext::GetDefaultZoomFactor() const {
  const content::HostZoomMap* const host_zoom_map =
      content::HostZoomMap::GetDefaultForBrowserContext(browser_context_.get());
  if (host_zoom_map)
    return content::ZoomLevelToZoomFactor(host_zoom_map->GetDefaultZoomLevel());

  return -1.0;
}

void EWebContext::SetAudioLatencyMode(Ewk_Audio_Latency_Mode latency_mode) {
  browser_context_->SetAudioLatencyMode(latency_mode);
}

Ewk_Audio_Latency_Mode EWebContext::GetAudioLatencyMode() const {
  return browser_context_->GetAudioLatencyMode();
}
/* LCOV_EXCL_STOP */

#if defined(OS_TIZEN_TV_PRODUCT)
bool EWebContext::ShouldAllowRequest(const net::URLRequest& request) {
  if (tizen_app_id_.empty() || !check_accessible_path_callback_)
    return true;
  return check_accessible_path_callback_->TriggerCallback(request.url().spec(),
                                                          tizen_app_id_);
}

void EWebContext::SetCheckAccessiblePathCallback(
    Ewk_Context_Check_Accessible_Path_Callback callback,
    void* user_data) {
  check_accessible_path_callback_.reset(
      new EwkCheckAccessiblePathCallback(callback, user_data));
}

void EWebContext::SetApplicationType(
    const Ewk_Application_Type application_type) {
  application_type_ = application_type;
  switch (application_type) {
    case EWK_APPLICATION_TYPE_WEBBROWSER:
      content::SetApplicationType(content::ApplicationType::WEBBROWSER);
      break;
    case EWK_APPLICATION_TYPE_HBBTV:
      content::SetApplicationType(content::ApplicationType::HBBTV);
      break;
    case EWK_APPLICATION_TYPE_TIZENWRT:
      content::SetApplicationType(content::ApplicationType::TIZENWRT);
      if (!tizen_app_id_.empty())
        browser_context_->UpdateWebAppDiskCachePath(tizen_app_id_);
      break;
    case EWK_APPLICATION_TYPE_OTHER:
      content::SetApplicationType(content::ApplicationType::OTHER);
      break;
    default:
      LOG(ERROR) << "Invalid application type. set application type WEBBROWSER "
                    "as default !!";
      application_type_ = EWK_APPLICATION_TYPE_WEBBROWSER;
      content::SetApplicationType(content::ApplicationType::WEBBROWSER);
      break;
  }
}

void EWebContext::RegisterURLSchemesAsCORSEnabled(
    const Eina_List* schemes_list) {
  std::string schemes;
  const Eina_List* it;
  const void* scheme;
  EINA_LIST_FOREACH(schemes_list, it, scheme) {
    schemes += static_cast<const char*>(scheme);
    schemes += ",";
  }

  HbbtvWidgetHost::Get().RegisterURLSchemesAsCORSEnabled(schemes);
}

void EWebContext::RegisterJSPluginMimeTypes(const Eina_List* mime_types_list) {
  std::string mime_types;
  const Eina_List* it;
  const void* mime_type;
  EINA_LIST_FOREACH(mime_types_list, it, mime_type) {
    mime_types += static_cast<const char*>(mime_type);
    mime_types += ",";
  }
  HbbtvWidgetHost::Get().RegisterJSPluginMimeTypes(mime_types);
}

void EWebContext::SetTimeOffset(double time_offset) {
  HbbtvWidgetHost::Get().SetTimeOffset(time_offset);
}

void EWebContext::SetTimeZoneOffset(double time_zone_offset,
                                    double daylight_saving_time) {
  HbbtvWidgetHost::Get().SetTimeZoneOffset(time_zone_offset,
                                           daylight_saving_time);
}

void EWebContext::SetEMPCertificatePath(const std::string& emp_ca_path,
                                        const std::string& default_ca_path) {
  browser_context_->SetEMPCertificatePath(emp_ca_path, default_ca_path);
}

void EWebContext::UpdateStoragePartitionPath() {
  content::StoragePartition* storage_partition =
      BrowserContext::GetDefaultStoragePartition(browser_context_.get());
  if (!storage_partition)
    return;

  base::FilePath orgin_path = browser_context_->GetPath();
  PathService::Clear();
  base::FilePath data_path = browser_context_->GetPath();
  LOG(INFO) << "orgin_path: " << orgin_path.AsUTF8Unsafe()
            << " data_path: " << data_path.AsUTF8Unsafe();

  if (data_path.empty() || data_path == orgin_path)
    return;

  storage_partition->UpdatePath(data_path);
}

/* LCOV_EXCL_START */
void EWebContext::SetMaxURLChars(size_t max_chars) {
  if (!injected_bundle_path_.empty())
    url::SetMaxURLCharSize(max_chars);
}
#endif

void EWebContext::SetInterceptRequestCallback(
    Ewk_Context* ewk_context,
    Ewk_Context_Intercept_Request_Callback callback,
    void* user_data) {
  content::StoragePartition* storage_partition =
      BrowserContext::GetDefaultStoragePartition(browser_context_.get());
  auto request_context = static_cast<content::URLRequestContextGetterEfl*>(
      storage_partition->GetURLRequestContext());
  content::BrowserThread::PostTask(
      content::BrowserThread::IO, FROM_HERE,
      base::Bind(
          &content::URLRequestContextGetterEfl::SetInterceptRequestCallback,
          base::Unretained(request_context), base::Unretained(ewk_context),
          base::Unretained(callback), base::Unretained(user_data)));
}

#if defined(OS_TIZEN_TV_PRODUCT)
void EWebContext::SetInterceptRequestCancelCallback(
    Ewk_Context* ewk_context,
    Ewk_Context_Intercept_Request_Callback callback,
    void* user_data) {
  content::StoragePartition* storage_partition =
      BrowserContext::GetDefaultStoragePartition(browser_context_.get());
  auto request_context = static_cast<content::URLRequestContextGetterEfl*>(
      storage_partition->GetURLRequestContext());
  content::BrowserThread::PostTask(
      content::BrowserThread::IO, FROM_HERE,
      base::Bind(&content::URLRequestContextGetterEfl::
                     SetInterceptRequestCancelCallback,
                 base::Unretained(request_context),
                 base::Unretained(ewk_context), base::Unretained(callback),
                 base::Unretained(user_data)));
}
#endif

/* LCOV_EXCL_START */

void EWebContext::SetPushMessageCallback(Ewk_Push_Message_Cb callback,
                                         void* user_data) {
  if (!callback)
    push_message_callback_.reset();
  else
    push_message_callback_.reset(
        new EwkPushMessageCallback(callback, user_data));
}

bool EWebContext::HasPushMessageCallback() {
  return !!push_message_callback_.get();
}

bool EWebContext::NotifyPushMessage(const std::string& sender_id,
                                    const std::string& push_data) {
  if (!push_message_callback_)
    return false;
  push_message_callback_->Trigger(sender_id, push_data);
  return true;
}

bool EWebContext::DeliverPushMessage(const std::string& push_data) {
  auto push_service = static_cast<PushMessagingServiceImplEfl*>(
      browser_context_->GetPushMessagingService());

  if (!push_service)
    return false;

  push_service->DeliverMessage(push_data);
  return true;
}

void EWebContext::EnableAppControl(bool enabled) {
#if defined(OS_TIZEN)
  content::ContentBrowserClient* cbc =
      content::GetContentClientExport()->browser();
  auto cbce = static_cast<content::ContentBrowserClientEfl*>(cbc);
  cbce->EnableAppControl(enabled);
#endif
}

void EWebContext::SetMaxRefreshRate(int max_refresh_rate) {
#if defined(OS_TIZEN)
  base::CommandLine& command_line = *base::CommandLine::ForCurrentProcess();
  command_line.AppendSwitchASCII(switches::kMaxRefreshRate,
                                 base::IntToString(max_refresh_rate));
#endif
}
