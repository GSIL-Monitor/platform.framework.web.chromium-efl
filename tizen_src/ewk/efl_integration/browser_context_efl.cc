// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser_context_efl.h"

#include "base/bind.h"
#include "base/files/file_enumerator.h"
#include "base/files/file_util.h"
#include "base/path_service.h"
#include "base/strings/string_split.h"
#include "base/synchronization/waitable_event.h"
#include "browser/autofill/personal_data_manager_factory.h"
#include "browser/geolocation/geolocation_permission_context_efl.h"
#include "browser/net/predictor_efl.h"
#include "browser/permission_manager_efl.h"
#include "browser/push_messaging/push_messaging_service_factory_efl.h"
#include "browser/scoped_allow_wait_for_legacy_web_view_api.h"
#include "browser/webdata/web_data_service_factory.h"
#include "components/autofill/core/browser/personal_data_manager.h"
#include "components/autofill/core/common/autofill_pref_names.h"
#include "components/password_manager/core/common/password_manager_pref_names.h"
#include "components/prefs/in_memory_pref_store.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/pref_service_factory.h"
#include "components/user_prefs/user_prefs.h"
#include "components/visitedlink/browser/visitedlink_master.h"
#include "content/browser/loader/resource_dispatcher_host_impl.h"
#include "content/browser/push_messaging/push_messaging_service_impl_efl.h"
#include "content/common/paths_efl.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/web_contents.h"
#include "eweb_context.h"
#include "network_delegate_efl.h"

#if defined(OS_TIZEN_TV_PRODUCT)
#include <vconf.h>
#include "base/strings/string_number_conversions.h"
#include "ewk/efl_integration/common/content_switches_efl.h"
#include "command_line_efl.h"
#include "common/application_type.h"
#endif

using namespace autofill::prefs;
using namespace password_manager::prefs;
using std::pair;

namespace {

// Shows notifications which correspond to PersistentPrefStore's reading errors.
void HandleReadError(PersistentPrefStore::PrefReadError error) {
  NOTIMPLEMENTED();
}

/* LCOV_EXCL_START */
net::CertificateList GetCertListFromFile(const base::FilePath& file_path) {
  std::string cert_data;
  net::CertificateList cert_list;
  if (base::ReadFileToString(file_path, &cert_data)) {
    cert_list = net::X509Certificate::CreateCertificateListFromBytes(
        cert_data.data(), cert_data.size(), net::X509Certificate::FORMAT_AUTO);
  } else {
    LOG(ERROR) << "Could not read file \"" << file_path.AsUTF8Unsafe()
                << "\" for loading CA certs. Please check permissions!";
  }
  return cert_list;
}
/* LCOV_EXCL_STOP */

#if defined(OS_TIZEN_TV_PRODUCT)
const char* vconf_key = NULL;
#endif
}  // namespace

namespace content {

static void CreateNetworkDelegateOnIOThread(BrowserContextEfl* context,
                                            base::WaitableEvent* completion) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));
  context->CreateNetworkDelegate();
  completion->Signal();
}

BrowserContextEfl::ResourceContextEfl::ResourceContextEfl(
    scoped_refptr<CookieManager> cookie_manager)
    : getter_(NULL), cookie_manager_(cookie_manager) {}

BrowserContextEfl::~BrowserContextEfl() {
  NotifyWillBeDestroyed(this);
#if defined(TIZEN_AUTOFILL_SUPPORT)
  autofill::PersonalDataManagerFactory::GetInstance()
      ->PersonalDataManagerRemove(this);
#endif

  ShutdownStoragePartitions();
  if (resource_context_) {
    BrowserThread::DeleteSoon(BrowserThread::IO, FROM_HERE, resource_context_);
  }
}

BrowserContextEfl::ResourceContextEfl::~ResourceContextEfl() {
  // |cookie_manager_| has access to
  // |URLRequestContextGetterEfl::GetURLRequestContext()|. So it should be
  // released before NotifyContextShuttingDown()
  cookie_manager_ = nullptr;
  getter_->NotifyContextShuttingDown();
  if (ResourceDispatcherHostImpl::Get())
    ResourceDispatcherHostImpl::Get()->CancelRequestsForContext(this);
}

/* LCOV_EXCL_START */
bool BrowserContextEfl::ResourceContextEfl::HTTPCustomHeaderAdd(
    const std::string& name,
    const std::string& value) {
  if (name.empty())
    return false;

  base::AutoLock locker(http_custom_headers_lock_);
  return http_custom_headers_.insert(std::make_pair(name, value)).second;
}
/* LCOV_EXCL_STOP */

bool BrowserContextEfl::ResourceContextEfl::HTTPCustomHeaderRemove(
    const std::string& name) {
  base::AutoLock locker(http_custom_headers_lock_);
  return http_custom_headers_.erase(name);
}

void BrowserContextEfl::ResourceContextEfl::HTTPCustomHeaderClear() {
  base::AutoLock locker(http_custom_headers_lock_);
  http_custom_headers_.clear();
}

const HTTPCustomHeadersEflMap
BrowserContextEfl::ResourceContextEfl::GetHTTPCustomHeadersEflMap() const {
  base::AutoLock locker(http_custom_headers_lock_);
  return http_custom_headers_;
}

net::HostResolver* BrowserContextEfl::ResourceContextEfl::GetHostResolver() {
  CHECK(getter_.get());
  return getter_->host_resolver();
}

net::URLRequestContext*
BrowserContextEfl::ResourceContextEfl::GetRequestContext() {
  CHECK(getter_.get());
  return getter_->GetURLRequestContext();
}

net::URLRequestContextGetter* BrowserContextEfl::CreateMediaRequestContext() {
  return request_context_getter_.get();
}

/* LCOV_EXCL_START */
net::URLRequestContextGetter*
BrowserContextEfl::CreateMediaRequestContextForStoragePartition(
    const base::FilePath&,
    bool) {
  return nullptr;
}
/* LCOV_EXCL_STOP */

void BrowserContextEfl::ResourceContextEfl::set_url_request_context_getter(
    URLRequestContextGetterEfl* getter) {
  getter_ = getter;
}

scoped_refptr<CookieManager>
BrowserContextEfl::ResourceContextEfl::GetCookieManager() const {
  return cookie_manager_;
}

BrowserContextEfl::BrowserContextEfl(EWebContext* web_context, bool incognito)
    : resource_context_(NULL),
      web_context_(web_context),
      temp_dir_creation_attempted_(false),
      incognito_(incognito),
      audio_latency_mode_(EWK_AUDIO_LATENCY_MODE_MID),
      dynamic_plugin_(nullptr) {
  InitVisitedLinkMaster();
  InitPredictorEfl();

  PathService::Clear();

  PrefRegistrySimple* pref_registry = new PrefRegistrySimple();

  pref_registry->RegisterBooleanPref(kAutofillEnabled, true);
  pref_registry->RegisterBooleanPref(kAutofillWalletImportEnabled, true);
  pref_registry->RegisterBooleanPref(kCredentialsEnableService, true);
  pref_registry->RegisterBooleanPref(kAutofillCreditCardEnabled, true);

  PrefServiceFactory pref_service_factory;
  pref_service_factory.set_user_prefs(base::MakeRefCounted<InMemoryPrefStore>());
  pref_service_factory.set_read_error_callback(base::Bind(&HandleReadError));
  user_pref_service_ = std::move(pref_service_factory.Create(pref_registry));

  user_prefs::UserPrefs::Set(this, user_pref_service_.get());

  BrowserContext::Initialize(this, GetPath());
  PushMessagingServiceImplEfl::InitializeForContext(this);
}

std::unique_ptr<ZoomLevelDelegate> BrowserContextEfl::CreateZoomLevelDelegate(
    const base::FilePath&) {
  return std::unique_ptr<ZoomLevelDelegate>();
}

ResourceContext* BrowserContextEfl::GetResourceContext() {
  return GetResourceContextEfl();
}

BrowserContextEfl::ResourceContextEfl*
BrowserContextEfl::GetResourceContextEfl() {
  if (!resource_context_) {
    resource_context_ = new ResourceContextEfl(web_context_->cookieManager());
  }
  return resource_context_;
}

// TODO Can this API be called from multiple threads?
base::FilePath BrowserContextEfl::GetPath() const {
  if (IsOffTheRecord()) {
    // Empty path indicates in memory storage. All data that would be persistent
    // are stored in memory and are gone when closing browser, what is a
    // requirement for the incognito mode (being off the record)
    return base::FilePath();
  }

  base::FilePath path;
  PathService::Get(PathsEfl::DIR_USER_DATA, &path);
  return path;
}

/* LCOV_EXCL_START */
PermissionManager* BrowserContextEfl::GetPermissionManager() {
  if (!permission_manager_.get()) {
    permission_manager_.reset(new PermissionManagerEfl());
  }
  return permission_manager_.get();
}
/* LCOV_EXCL_STOP */

BackgroundSyncController* BrowserContextEfl::GetBackgroundSyncController() {
  return nullptr;
}

void BrowserContextEfl::CreateNetworkDelegate() {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));
  DCHECK(!network_delegate_for_getter_.get());
  network_delegate_for_getter_.reset(
      new net::NetworkDelegateEfl(web_context_->cookieManager()->GetWeakPtr()));
}

net::URLRequestContextGetter* BrowserContextEfl::CreateRequestContext(
    content::ProtocolHandlerMap* protocol_handlers,
    URLRequestInterceptorScopedVector request_interceptors) {
  // TODO: Implement support for chromium network log

  base::FilePath cache_base_path;
  if (!PathService::Get(base::DIR_CACHE, &cache_base_path)) {
    LOG(ERROR)
        << "Could not retrieve path to the cache directory";  // LCOV_EXCL_LINE
    return NULL;
  }

#if defined(OS_TIZEN_TV_PRODUCT)
  int cache_max_size = 0;
  if (!base::StringToInt(
          base::CommandLine::ForCurrentProcess()->GetSwitchValueASCII(
              switches::kDiskCacheSize),
          &cache_max_size)) {
    // Fallback when StringToInt failed as it could modify the output.
    cache_max_size = 0;
  }
#endif

  if (!network_delegate_for_getter_.get()) {
    // NetWorkDelegate must be created on IO thread
    base::WaitableEvent done(base::WaitableEvent::ResetPolicy::AUTOMATIC,
                             base::WaitableEvent::InitialState::NOT_SIGNALED);
    BrowserThread::PostTask(
        BrowserThread::IO, FROM_HERE,
        base::Bind(CreateNetworkDelegateOnIOThread, this, &done));
    ScopedAllowWaitForLegacyWebViewApi allow_wait;
    done.Wait();
  }

  request_context_getter_ = new URLRequestContextGetterEfl(
      std::move(network_delegate_for_getter_), false, cache_base_path,
#if defined(OS_TIZEN_TV_PRODUCT)
      cache_max_size,
#endif
      BrowserThread::GetTaskRunnerForThread(BrowserThread::IO),
      BrowserThread::GetTaskRunnerForThread(BrowserThread::FILE),
      protocol_handlers, std::move(request_interceptors), NULL);
  web_context_->cookieManager()->SetRequestContextGetter(
      request_context_getter_);
  resource_context_->set_url_request_context_getter(
      request_context_getter_.get());
  return request_context_getter_.get();
}

#if defined(OS_TIZEN_TV_PRODUCT)
void BrowserContextEfl::UpdateWebAppDiskCachePathOnIOThread(
    const std::string& app_id) {
  CHECK(request_context_getter_.get());
  request_context_getter_->UpdateWebAppDiskCachePath(app_id);
}

void BrowserContextEfl::UpdateWebAppDiskCachePath(const std::string& app_id) {
  if (request_context_getter_.get()) {
    BrowserThread::PostTask(
        BrowserThread::IO, FROM_HERE,
        base::Bind(&BrowserContextEfl::UpdateWebAppDiskCachePathOnIOThread,
                   base::Unretained(this), app_id));
  }
}

/* LCOV_EXCL_START */
bool BrowserContextEfl::GetCertificateVconfStatus() {
  int vconfRet;
  int ctl_loaded = 0;

  if (IsWebBrowser())
    vconf_key = "memory/ctl_loaded/webbrowser";
  else if (IsTIZENWRT())
    vconf_key = "memory/ctl_loaded/webapp";
  else if (IsHbbTV())
    vconf_key = "memory/ctl_loaded/hbbtv";
  if (NULL != vconf_key) {
    vconfRet = vconf_get_bool(vconf_key, &ctl_loaded);

    if (vconfRet != 0) {
      LOG(ERROR) << "Fail to get vconf key. key[" << vconf_key << "],"
                 << "value[" << ctl_loaded << "], ret[" << vconfRet << "]";
      return false;
    }

    if (ctl_loaded) {
      LOG(INFO) << "Certificate files was already imported."
                << " key[" << vconf_key << "]";
      return true;
    }
  }
  return false;
}
#endif

void BrowserContextEfl::SetCertificatePath(
    const std::string& certificate_path) {
#if defined(OS_TIZEN_TV_PRODUCT)
  if (GetCertificateVconfStatus())
    return;
#endif
  certificate_path_ = certificate_path;
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  BrowserThread::PostTask(BrowserThread::FILE, FROM_HERE,
      base::Bind(&BrowserContextEfl::ReadCertificateAndAdd,
                 base::Owned(new std::string(certificate_path))));
}

void BrowserContextEfl::ReadCertificateAndAdd(
    const std::string* certificate_path) {
  std::vector<std::string> paths = base::SplitString(*certificate_path,
      ":", base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL);
  for (const auto& path : paths) {
    base::FilePath file_path(path);
    base::File file(file_path, base::File::FLAG_OPEN);
    base::File::Info file_info;
    // Check if path is accesible
    if (file.IsValid() && file.GetInfo(&file_info)) {
      if (file_info.is_directory) {
        base::FileEnumerator files(file_path, true,
            base::FileEnumerator::FILES | base::FileEnumerator::SHOW_SYM_LINKS,
            "@(*.crt|*.pem)");
        files.SetExtMatchToFlag();
        for (base::FilePath cert_file = files.Next(); !cert_file.empty();
            cert_file = files.Next()) {
          for (const auto& ca_cert : GetCertListFromFile(cert_file)) {
            net::CertDatabase::GetInstance()->ImportCACert(ca_cert.get());
          }
        }
      } else {
        for (const auto& ca_cert : GetCertListFromFile(file_path)) {
          net::CertDatabase::GetInstance()->ImportCACert(ca_cert.get());
        }
      }
#if defined(OS_TIZEN_TV_PRODUCT)
      if (NULL != vconf_key) {
        vconf_set_bool(vconf_key, true);
        LOG(INFO) << "Certificate loading is done. Set vconf to true. key["
                  << vconf_key << "], path[" << *certificate_path << "]";
      }
#endif
    } else {  // Stat returned non-zero, path not accesible
      LOG(ERROR) << "Could not access path \"" << path
                  << "\" for loading CA certs. Please check permissions!";
    }
  }
}
/* LCOV_EXCL_STOP */

#if defined(OS_TIZEN_TV_PRODUCT)
void BrowserContextEfl::SetEMPCertificatePath(
    const std::string& emp_ca_path,
    const std::string& default_ca_path) {
  if (GetCertificateVconfStatus())
    return;
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  BrowserThread::PostTask(
      BrowserThread::FILE, FROM_HERE,
      base::Bind(&BrowserContextEfl::SetEMPCertificatePathOnFileThread,
                 base::Owned(new std::string(emp_ca_path)),
                 base::Owned(new std::string(default_ca_path))));
}

void BrowserContextEfl::SetEMPCertificatePathOnFileThread(
    const std::string* emp_ca_path,
    const std::string* default_ca_path) {
  if (emp_ca_path)
    LOG(INFO) << "[CaBox] empCAPath : " << *emp_ca_path;
  if (default_ca_path)
    LOG(INFO) << "[CaBox] defaultCAPath : " << *default_ca_path;
  LOG(INFO) << "[CaBox] Try to add emp certificate to NSS DB";
  if (!ReadEMPCertificateAndAdd(emp_ca_path)) {
    LOG(INFO) << "[CaBox] Try to add default certificate to NSS DB";
    ReadEMPCertificateAndAdd(default_ca_path);
  }
}

bool BrowserContextEfl::ReadEMPCertificateAndAdd(
    const std::string* certificate_path) {
  if (!certificate_path) {
    LOG(ERROR) << "[CaBox] certificate path is null";
    return false;
  }
  std::vector<std::string> paths = base::SplitString(
      *certificate_path, ":", base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL);
  for (const auto& path : paths) {
    base::FilePath file_path(path);
    base::File file(file_path, base::File::FLAG_OPEN);
    base::File::Info file_info;
    net::CertificateList cert_list;
    // Check if path is accesible
    if (!(file.IsValid() && file.GetInfo(&file_info))) {
      LOG(INFO) << "[CaBox] Could not access path \"" << path
                << "\" for loading CA certs. Please check permissions!";
      return false;
    }
    if (file_info.is_directory) {
      base::FileEnumerator files(
          file_path, true,
          base::FileEnumerator::FILES | base::FileEnumerator::SHOW_SYM_LINKS,
          "@(*.crt|*.pem)");
      files.SetExtMatchToFlag();

      base::FilePath cert_file = files.Next();
      if (cert_file.empty()) {
        LOG(ERROR) << "[CaBox] There is no downloaded certificate files";
        return false;
      }

      for (; !cert_file.empty(); cert_file = files.Next()) {
        cert_list = GetCertListFromFile(cert_file);
        if (cert_list.empty()) {
          LOG(ERROR) << "[CaBox] Fail to make a Certificate List because of "
                        "invalid certificate files";
          return false;
        }
        for (size_t i = 0; i < cert_list.size(); ++i)
          net::CertDatabase::GetInstance()->ImportCACert(cert_list[i].get());
      }
    } else {
      cert_list = GetCertListFromFile(file_path);
      if (cert_list.empty()) {
        LOG(ERROR) << "[CaBox] Fail to make a Certificate List because of "
                      "invalid certificate files";
        return false;
      }
      for (size_t i = 0; i < cert_list.size(); ++i)
        net::CertDatabase::GetInstance()->ImportCACert(cert_list[i].get());
    }
  }
  if (NULL != vconf_key) {
    vconf_set_bool(vconf_key, true);
    LOG(INFO) << "Certificate loading is done. Set vconf to true. key["
              << vconf_key << "], path[" << *certificate_path << "]";
  }
  return true;
}
#endif

void BrowserContextEfl::InitVisitedLinkMaster() {
  if (!IsOffTheRecord()) {
    visitedlink_master_.reset(
        new visitedlink::VisitedLinkMaster(this, this, false));
    visitedlink_master_->Init();
  }
}

void BrowserContextEfl::AddVisitedURLs(const std::vector<GURL>& urls) {
  if (!IsOffTheRecord()) {
    DCHECK(visitedlink_master_);
    visitedlink_master_->AddURLs(urls);
  }
}

void BrowserContextEfl::InitPredictorEfl() {
  predictor_efl_.reset(new browser_net::PredictorEfl());
  predictor_efl_->InitNetworkPredictor(this);
}

browser_net::PredictorEfl* BrowserContextEfl::GetNetworkPredictorEfl() {
  return predictor_efl_.get();
}

void BrowserContextEfl::RebuildTable(
    const scoped_refptr<URLEnumerator>& enumerator) {
  if (!IsOffTheRecord()) {
    // WebView rebuilds from WebChromeClient.getVisitedHistory. The client
    // can change in the lifetime of this WebView and may not yet be set here.
    // Therefore this initialization path is not used.
    enumerator->OnComplete(true);
  }
}

SSLHostStateDelegate* BrowserContextEfl::GetSSLHostStateDelegate() {
  if (!ssl_host_state_delegate_.get())
    ssl_host_state_delegate_.reset(new SSLHostStateDelegateEfl());

  return ssl_host_state_delegate_.get();
}

/* LCOV_EXCL_START */
const GeolocationPermissionContextEfl&
BrowserContextEfl::GetGeolocationPermissionContext() const {
  if (!geolocation_permission_context_.get()) {
    geolocation_permission_context_.reset(
        new GeolocationPermissionContextEfl());
  }

  return *(geolocation_permission_context_.get());
}
/* LCOV_EXCL_STOP */

BackgroundFetchDelegate* BrowserContextEfl::GetBackgroundFetchDelegate() {
  return nullptr;
}

/* LCOV_EXCL_START */
BrowsingDataRemoverDelegate* BrowserContextEfl::GetBrowsingDataRemoverDelegate() {
  return nullptr;
}
/* LCOV_EXCL_STOP */

/* LCOV_EXCL_START */
void BrowserContextEfl::SetAudioLatencyMode(
    Ewk_Audio_Latency_Mode latency_mode) {
  audio_latency_mode_ = latency_mode;
}
/* LCOV_EXCL_STOP */

Ewk_Audio_Latency_Mode BrowserContextEfl::GetAudioLatencyMode() const {
  return audio_latency_mode_;
}

bool BrowserContextEfl::GetWrtParsedUrl(const GURL& url, GURL& parsed_url) {
  if (dynamic_plugin_) {
    DCHECK(!web_context_->GetTizenAppId().empty());
    bool is_decrypted_file = false;
    std::string parsed_url_str;
    dynamic_plugin_->ParseURL(
        &const_cast<std::string&>(url.spec()), &parsed_url_str,
        web_context_->GetTizenAppId().c_str(), &is_decrypted_file);
    if (!parsed_url_str.empty()) {
      parsed_url = GURL(parsed_url_str);
      if (is_decrypted_file)
        LOG(WARNING) << "Not implemented for encrypted file." << parsed_url_str;
      return true;
    }
  }
  return false;
}

storage::SpecialStoragePolicy* BrowserContextEfl::GetSpecialStoragePolicy() {
  return GetSpecialStoragePolicyEfl();
}

SpecialStoragePolicyEfl* BrowserContextEfl::GetSpecialStoragePolicyEfl() {
  if (!special_storage_policy_efl_.get())
    special_storage_policy_efl_ = new SpecialStoragePolicyEfl();
  return special_storage_policy_efl_.get();
}

PushMessagingService* BrowserContextEfl::GetPushMessagingService() {
  return static_cast<PushMessagingService*>(
      PushMessagingServiceFactoryEfl::GetForContext(this));
}
}
