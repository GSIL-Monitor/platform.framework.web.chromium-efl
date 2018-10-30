// Copyright 2013 The Chromium Authors. All rights reserved.
// Copyright 2014-2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "url_request_context_getter_efl.h"

#include "base/command_line.h"
#include "base/files/file_util.h"
#include "base/memory/ptr_util.h"
#include "base/strings/string_number_conversions.h"
#include "base/task_scheduler/post_task.h"
#include "base/threading/sequenced_worker_pool.h"
#include "components/certificate_transparency/tree_state_tracker.h"
#include "components/network_session_configurator/common/network_switches.h"
#include "content/public/browser/cookie_store_factory.h"
#include "content/public/common/content_switches.h"
#include "content/public/common/url_constants.h"
#include "http_user_agent_settings_efl.h"
#include "net/base/cache_type.h"
#include "net/cert/cert_verifier.h"
#include "net/cert/ct_known_logs.h"
#include "net/cert/ct_log_verifier.h"
#include "net/cert/ct_policy_enforcer.h"
#include "net/cert/multi_log_ct_verifier.h"
#include "net/dns/host_resolver.h"
#include "net/dns/mapped_host_resolver.h"
#include "net/extras/sqlite/sqlite_persistent_cookie_store.h"
#include "net/http/http_auth_handler_factory.h"
#include "net/http/http_cache.h"
#include "net/http/http_server_properties_impl.h"
#include "net/proxy/proxy_service.h"
#include "net/ssl/channel_id_service.h"
#include "net/ssl/default_channel_id_store.h"
#include "net/ssl/ssl_config_service_defaults.h"
#include "net/url_request/data_protocol_handler.h"
#include "net/url_request/file_protocol_handler.h"
#include "net/url_request/static_http_user_agent_settings.h"
#include "net/url_request/url_request_intercepting_job_factory.h"
#include "net/url_request/url_request_job_factory_impl.h"
#include "network_delegate_efl.h"
#include "url/url_constants.h"
#include "url_request_interceptor_efl.h"
#include "wrt/dynamicplugin.h"
#include "wrt/wrt_url_request_interceptor.h"

#if defined(OS_TIZEN_TV_PRODUCT)
#include "dvb_protocol_handler.h"
#include "mmf_protocol_handler.h"
#include "net/disk_cache/cache_util.h"
#endif

#if defined(USE_NSS_CERTS) && defined(OS_TIZEN_TV_PRODUCT)
#include "net/cert_net/nss_ocsp.h"
#endif

using net::SQLitePersistentCookieStore;

namespace content {

namespace {

void InstallProtocolHandlers(net::URLRequestJobFactoryImpl* job_factory,
                             ProtocolHandlerMap* protocol_handlers) {
  for (ProtocolHandlerMap::iterator it = protocol_handlers->begin();
       it != protocol_handlers->end(); ++it) {
    bool set_protocol = job_factory->SetProtocolHandler(
        it->first, base::WrapUnique(it->second.release()));
    DCHECK(set_protocol);
  }
  protocol_handlers->clear();
}

}  // namespace

URLRequestContextGetterEfl::URLRequestContextGetterEfl(
    std::unique_ptr<net::NetworkDelegate> network_delegate,
    bool ignore_certificate_errors,
    const base::FilePath& base_path,
#if defined(OS_TIZEN_TV_PRODUCT)
    int cache_max_bytes,
#endif
    const scoped_refptr<base::SingleThreadTaskRunner>& io_task_runner,
    const scoped_refptr<base::SingleThreadTaskRunner>& file_task_runner,
    ProtocolHandlerMap* protocol_handlers,
    URLRequestInterceptorScopedVector request_interceptors,
    net::NetLog* net_log)
    : ignore_certificate_errors_(ignore_certificate_errors),
      shut_down_(false),
      base_path_(base_path),
#if defined(OS_TIZEN_TV_PRODUCT)
      cache_max_bytes_(cache_max_bytes),
#endif
      io_task_runner_(io_task_runner),
      file_task_runner_(file_task_runner),
      net_log_(net_log),
      network_delegate_(std::move(network_delegate)),
      request_interceptors_(std::move(request_interceptors)),
      weak_ptr_factory_(this) {
  // Must first be created on the UI thread.
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));

  if (protocol_handlers)
    std::swap(protocol_handlers_, *protocol_handlers);

  cert_verifier_ = net::CertVerifier::CreateDefault();

  proxy_config_service_ =
      net::ProxyService::CreateSystemProxyConfigService(io_task_runner);
}

URLRequestContextGetterEfl::~URLRequestContextGetterEfl() {
  // NotifyContextShuttingDown() must have been called.
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  DCHECK(shut_down_);
#if defined(USE_NSS_CERTS) && defined(OS_TIZEN_TV_PRODUCT)
  net::ShutdownNSSHttpIO();
#endif
}

void URLRequestContextGetterEfl::NotifyContextShuttingDown() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  shut_down_ = true;

  if (url_request_context_)
    url_request_context_->cert_transparency_verifier()->SetObserver(nullptr);
  ct_tree_tracker_.reset();

  URLRequestContextGetter::NotifyContextShuttingDown();
}

net::URLRequestContext* URLRequestContextGetterEfl::GetURLRequestContext() {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));

  if (shut_down_)
    return nullptr;

  if (!url_request_context_) {
    const base::CommandLine& command_line =
        *base::CommandLine::ForCurrentProcess();

    // TODO: Use net::URLRequestContextBuilder to create net::URLRequestContext.
    url_request_context_.reset(new net::URLRequestContext());
    if (net_log_) {
      url_request_context_->set_net_log(net_log_);  // LCOV_EXCL_LINE
    }

    if (!network_delegate_.get())
      network_delegate_.reset(  // LCOV_EXCL_LINE
          new net::NetworkDelegateEfl(
              base::WeakPtr<CookieManager>()));  // LCOV_EXCL_LINE

    url_request_context_->set_network_delegate(network_delegate_.get());

    // Create CTVerifier
    std::unique_ptr<net::MultiLogCTVerifier> ct_verifier =
        base::MakeUnique<net::MultiLogCTVerifier>();

    std::vector<scoped_refptr<const net::CTLogVerifier>> ct_logs(
        net::ct::CreateLogVerifiersForKnownLogs());

    ct_tree_tracker_.reset(
        new certificate_transparency::TreeStateTracker(ct_logs, net_log_));

    // Add built-in logs
    ct_verifier->AddLogs(ct_logs);

    // Register the ct_tree_tracker_ as observer for verified SCTs.
    ct_verifier->SetObserver(ct_tree_tracker_.get());

    storage_.reset(
        new net::URLRequestContextStorage(url_request_context_.get()));
    storage_->set_cookie_store(CreateCookieStore(CookieStoreConfig()));
    storage_->set_channel_id_service(base::WrapUnique(
        new net::ChannelIDService(new net::DefaultChannelIDStore(NULL))));
    storage_->set_http_user_agent_settings(
        base::WrapUnique(new HttpUserAgentSettingsEfl()));

    std::unique_ptr<net::HostResolver> host_resolver(
        net::HostResolver::CreateDefaultResolver(
            url_request_context_->net_log()));

    storage_->set_cert_verifier(std::move(cert_verifier_));
    storage_->set_transport_security_state(
        base::WrapUnique(new net::TransportSecurityState));
    storage_->set_cert_transparency_verifier(std::move(ct_verifier));
    storage_->set_ct_policy_enforcer(
        base::WrapUnique(new net::CTPolicyEnforcer));

    // TODO(jam): use v8 if possible, look at chrome code.
    storage_->set_proxy_service(
        net::ProxyService::CreateUsingSystemProxyResolver(
            std::move(proxy_config_service_), url_request_context_->net_log()));

    storage_->set_ssl_config_service(new net::SSLConfigServiceDefaults);
    storage_->set_http_auth_handler_factory(
        net::HttpAuthHandlerFactory::CreateDefault(host_resolver.get()));
    storage_->set_http_server_properties(
        std::unique_ptr<net::HttpServerProperties>(
            new net::HttpServerPropertiesImpl()));

    net::HttpCache::DefaultBackend* main_backend = NULL;
    if (base_path_.empty()) {                             // LCOV_EXCL_LINE
      main_backend = new net::HttpCache::DefaultBackend(  // LCOV_EXCL_LINE
          net::MEMORY_CACHE, net::CACHE_BACKEND_DEFAULT,
          base::FilePath(),  // LCOV_EXCL_LINE
#if defined(OS_TIZEN_TV_PRODUCT)
          cache_max_bytes_);
#else
          0);  // LCOV_EXCL_LINE
#endif
    } else {
      // Although main_backend is created with the cache_path, it can be changed
      // in case of web apps (once tizen app id has set).
      base::FilePath cache_path = base_path_.Append(FILE_PATH_LITERAL("Cache"));

      main_backend = new net::HttpCache::DefaultBackend(
          net::DISK_CACHE, net::CACHE_BACKEND_DEFAULT, cache_path,
#if defined(OS_TIZEN_TV_PRODUCT)
          cache_max_bytes_);
#else
          0);
#endif
    }

    if (command_line.HasSwitch(switches::kHostResolverRules)) {
      /* LCOV_EXCL_START */
      std::unique_ptr<net::MappedHostResolver> mapped_host_resolver(
          new net::MappedHostResolver(std::move(host_resolver)));
      mapped_host_resolver->SetRulesFromString(
          command_line.GetSwitchValueASCII(switches::kHostResolverRules));
      host_resolver = std::move(mapped_host_resolver);
    }
    /* LCOV_EXCL_STOP */

    // Give |storage_| ownership at the end in case it's |mapped_host_resolver|.
    storage_->set_host_resolver(std::move(host_resolver));

    net::HttpNetworkSession::Context network_session_context;
    network_session_context.cert_verifier =
        url_request_context_->cert_verifier();
    network_session_context.transport_security_state =
        url_request_context_->transport_security_state();
    network_session_context.cert_transparency_verifier =
        url_request_context_->cert_transparency_verifier();
    network_session_context.ct_policy_enforcer =
        url_request_context_->ct_policy_enforcer();
    network_session_context.channel_id_service =
        url_request_context_->channel_id_service();
    network_session_context.ssl_config_service =
        url_request_context_->ssl_config_service();
    network_session_context.http_auth_handler_factory =
        url_request_context_->http_auth_handler_factory();
    network_session_context.net_log = url_request_context_->net_log();
    network_session_context.host_resolver =
        url_request_context_->host_resolver();
    network_session_context.proxy_service =
        url_request_context_->proxy_service();
    network_session_context.http_server_properties =
        url_request_context_->http_server_properties();

    net::HttpNetworkSession::Params network_session_params;
    network_session_params.ignore_certificate_errors =
        ignore_certificate_errors_;
    if (command_line.HasSwitch(switches::kTestingFixedHttpPort)) {
      int value;
      /* LCOV_EXCL_START */
      base::StringToInt(
          command_line.GetSwitchValueASCII(switches::kTestingFixedHttpPort),
          &value);
      network_session_params.testing_fixed_http_port = value;
      /* LCOV_EXCL_STOP */
    }
    if (command_line.HasSwitch(switches::kTestingFixedHttpsPort)) {
      int value;
      /* LCOV_EXCL_START */
      base::StringToInt(
          command_line.GetSwitchValueASCII(switches::kTestingFixedHttpsPort),
          &value);
      network_session_params.testing_fixed_https_port = value;
      /* LCOV_EXCL_STOP */
    }

#if defined(OS_TIZEN_TV_PRODUCT) // TizenTV supports QUIC protocol.
    network_session_params.enable_quic = true;
#endif

    storage_->set_http_network_session(
        base::WrapUnique(new net::HttpNetworkSession(network_session_params,
                                                     network_session_context)));

    storage_->set_http_transaction_factory(base::WrapUnique(new net::HttpCache(
        storage_->http_network_session(), base::WrapUnique(main_backend),
        true /* is_main_cache */)));

    std::unique_ptr<net::URLRequestJobFactoryImpl> job_factory(
        new net::URLRequestJobFactoryImpl());
    // Keep ProtocolHandlers added in sync with
    // ShellContentBrowserClient::IsHandledURL().
    InstallProtocolHandlers(job_factory.get(), &protocol_handlers_);
    bool set_protocol = job_factory->SetProtocolHandler(
        url::kDataScheme, base::WrapUnique(new net::DataProtocolHandler));
    DCHECK(set_protocol);

    set_protocol = job_factory->SetProtocolHandler(
        url::kFileScheme,
        base::WrapUnique(new net::FileProtocolHandler(
            base::CreateTaskRunnerWithTraits(
                {base::MayBlock(), base::TaskPriority::BACKGROUND,
                 base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN})
                .get())));
    DCHECK(set_protocol);

#if defined(OS_TIZEN_TV_PRODUCT)
    set_protocol = job_factory->SetProtocolHandler(
        url::kDvbScheme, base::WrapUnique(new net::DvbProtocolHandler));
    DCHECK(set_protocol);

    set_protocol = job_factory->SetProtocolHandler(
        url::kMmfScheme, base::WrapUnique(new net::MmfProtocolHandler));
    DCHECK(set_protocol);

    set_protocol = job_factory->SetProtocolHandler(
        url::kTVKeyScheme, base::WrapUnique(new net::DvbProtocolHandler));
    DCHECK(set_protocol);

    set_protocol = job_factory->SetProtocolHandler(
        url::kOpAppScheme, base::WrapUnique(new net::DvbProtocolHandler));
    DCHECK(set_protocol);

    set_protocol = job_factory->SetProtocolHandler(
        url::kHbbTVCarouselScheme, base::WrapUnique(new net::DvbProtocolHandler));
    DCHECK(set_protocol);

    set_protocol = job_factory->SetProtocolHandler(
        url::kCIScheme, base::WrapUnique(new net::DvbProtocolHandler));
    DCHECK(set_protocol);
#endif

    // Set up interceptors in the reverse order.
    std::unique_ptr<net::URLRequestJobFactory> top_job_factory =
        std::move(job_factory);
    request_interceptors_.push_back(
        base::WrapUnique(new URLRequestInterceptorEFL()));
    for (auto i = request_interceptors_.rbegin();
         i != request_interceptors_.rend(); ++i) {
      top_job_factory.reset(new net::URLRequestInterceptingJobFactory(
          std::move(top_job_factory), std::move(*i)));
    }
    request_interceptors_.clear();

    storage_->set_job_factory(std::move(top_job_factory));
  }

  return url_request_context_.get();
}

scoped_refptr<base::SingleThreadTaskRunner>
URLRequestContextGetterEfl::GetNetworkTaskRunner() const {
  return BrowserThread::GetTaskRunnerForThread(BrowserThread::IO);
}

/* LCOV_EXCL_START */
net::HostResolver* URLRequestContextGetterEfl::host_resolver() {
  return url_request_context_->host_resolver();
}
/* LCOV_EXCL_STOP */

void URLRequestContextGetterEfl::SetCookieStoragePath(
    const base::FilePath& path,
    bool persist_session_cookies,
    Ewk_Cookie_Persistent_Storage file_storage) {
  if (url_request_context_->cookie_store() && (cookie_store_path_ == path)) {
    // The path has not changed so don't do anything.
    return;
  }

  if (!BrowserThread::CurrentlyOn(BrowserThread::IO)) {
    /* LCOV_EXCL_START */
    BrowserThread::PostTask(
        BrowserThread::IO, FROM_HERE,
        base::Bind(&URLRequestContextGetterEfl::SetCookieStoragePath, this,
                   path, persist_session_cookies, file_storage));
    return;
    /* LCOV_EXCL_STOP */
  }

  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));
  if (file_storage == EWK_COOKIE_PERSISTENT_STORAGE_SQLITE) {
    CreateSQLitePersistentCookieStore(path, persist_session_cookies);
  } else {
    CreatePersistentCookieStore(path, persist_session_cookies);
  }
}

void URLRequestContextGetterEfl::CreateSQLitePersistentCookieStore(
    const base::FilePath& path,
    bool persist_session_cookies) {
  using content::CookieStoreConfig;
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));
  scoped_refptr<net::SQLitePersistentCookieStore> persistent_store;

  if (path.empty())
    return;  // LCOV_EXCL_LINE
  base::ThreadRestrictions::ScopedAllowIO allow_io;
  if (base::DirectoryExists(path) || base::CreateDirectory(path)) {
    const base::FilePath& cookie_path = path.AppendASCII("Cookies");
    persistent_store = new net::SQLitePersistentCookieStore(
        cookie_path, BrowserThread::GetTaskRunnerForThread(BrowserThread::IO),
        BrowserThread::GetTaskRunnerForThread(BrowserThread::DB),
        persist_session_cookies, NULL);
  } else {
    NOTREACHED() << "The cookie storage directory could not be created";
    return;
  }
  // Set the new cookie store that will be used for all new requests. The old
  // cookie store, if any, will be automatically flushed and closed when no
  // longer referenced.
  std::unique_ptr<net::CookieMonster> cookie_monster(
      new net::CookieMonster(persistent_store.get(), NULL));

  if (persistent_store.get() && persist_session_cookies)
    cookie_monster->SetPersistSessionCookies(true);

  std::vector<std::string> schemes;
  for (int i = 0; i < net::CookieMonster::kDefaultCookieableSchemesCount; ++i)
    schemes.push_back(
        std::string(net::CookieMonster::kDefaultCookieableSchemes[i]));
  schemes.push_back(url::kFileScheme);
  cookie_monster->SetCookieableSchemes(schemes);

  storage_->set_cookie_store(std::move(cookie_monster));

  cookie_store_path_ = path;
}

void URLRequestContextGetterEfl::CreatePersistentCookieStore(
    const base::FilePath& path,
    bool persist_session_cookies) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));
  cookie_store_path_ = path;

  CookieStoreConfig config(
      cookie_store_path_.Append(FILE_PATH_LITERAL("Cookies")),
      CookieStoreConfig::RESTORED_SESSION_COOKIES, NULL);
  cookie_store_ = CreateCookieStore(config);
  net::CookieMonster* cookie_monster =
      static_cast<net::CookieMonster*>(cookie_store_.get());
  cookie_monster->SetPersistSessionCookies(persist_session_cookies);

  std::vector<std::string> schemes;
  for (int i = 0; i < net::CookieMonster::kDefaultCookieableSchemesCount; ++i)
    schemes.push_back(
        std::string(net::CookieMonster::kDefaultCookieableSchemes[i]));
  schemes.push_back(url::kFileScheme);
  cookie_monster->SetCookieableSchemes(schemes);

  DCHECK(url_request_context_);
  url_request_context_->set_cookie_store(cookie_store_.get());
}

void URLRequestContextGetterEfl::SetInterceptRequestCallback(  // LCOV_EXCL_LINE
    Ewk_Context* ewk_context,
    Ewk_Context_Intercept_Request_Callback callback,
    void* user_data) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));
  /* LCOV_EXCL_START */
  if (network_delegate_.get()) {
    (static_cast<net::NetworkDelegateEfl*>(network_delegate_.get()))->
        SetInterceptRequestCallback(ewk_context, callback, user_data);
  }
  /* LCOV_EXCL_STOP */
}

#if defined(OS_TIZEN_TV_PRODUCT)
void URLRequestContextGetterEfl::SetInterceptRequestCancelCallback(
    Ewk_Context* ewk_context,
    Ewk_Context_Intercept_Request_Callback callback,
    void* user_data) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));

  if (network_delegate_.get()) {
    (static_cast<net::NetworkDelegateEfl*>(network_delegate_.get()))
        ->SetInterceptRequestCancelCallback(ewk_context, callback, user_data);
  }
}
#endif

/* LCOV_EXCL_START */
void URLRequestContextGetterEfl::SetDynamicPlugin(
    const std::string& tizen_app_id,
    DynamicPlugin* dynamic_plugin) {
  if (!BrowserThread::CurrentlyOn(BrowserThread::IO)) {
    BrowserThread::PostTask(
        BrowserThread::IO, FROM_HERE,
        base::Bind(&URLRequestContextGetterEfl::SetDynamicPlugin, this,
                   tizen_app_id, dynamic_plugin));
    return;
  }

  net::URLRequestContext* url_request_context = GetURLRequestContext();
  CHECK(url_request_context) << "url_request_context is NULL!!";

  if (wrt_url_intercepting_job_factory_)
    return;
  // Set WrtUrlRequestInterceptor for WebAPP URL Conversion
  wrt_url_interceptor_.reset(new net::WrtUrlRequestInterceptor(
      base::CreateTaskRunnerWithTraits(
          {base::MayBlock(), base::TaskPriority::BACKGROUND,
           base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN}),
      tizen_app_id, dynamic_plugin));
  wrt_url_intercepting_job_factory_.reset(
      new net::URLRequestInterceptingJobFactory(
          const_cast<net::URLRequestJobFactory*>(
              url_request_context->job_factory()),
          wrt_url_interceptor_.get()));
  url_request_context->set_job_factory(wrt_url_intercepting_job_factory_.get());
}
/* LCOV_EXCL_STOP */

#if defined(OS_TIZEN_TV_PRODUCT)
void URLRequestContextGetterEfl::UpdateWebAppDiskCachePath(
    const std::string& app_id) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));
  if (base_path_.empty())
    return;

  // The GetURLRequestContext was called when EWebContext create,
  // so in IO thread at this point the url_request_context_ has been created.
  DCHECK(url_request_context_);
  if (url_request_context_->http_transaction_factory()) {
    net::HttpCache* http_cache =
        url_request_context_->http_transaction_factory()->GetCache();
    if (http_cache)
      http_cache->UpdateWebAppDiskCachePath(
          base_path_.AppendASCII(disk_cache::kTizenAppCache).Append(app_id));
  }
}
#endif

};  // namespace content
