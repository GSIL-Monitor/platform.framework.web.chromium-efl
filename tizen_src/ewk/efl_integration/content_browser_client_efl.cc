// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content_browser_client_efl.h"

#include "base/callback.h"
#include "base/strings/string_number_conversions.h"
#include "browser/editor_client_observer.h"
#include "browser/geolocation/access_token_store_efl.h"
#include "browser/network_hints_message_filter_efl.h"
#include "browser/notification/notification_controller_efl.h"
#include "browser/payments/payment_request_factory_efl.h"
#include "browser/quota_permission_context_efl.h"
#include "browser/render_message_filter_efl.h"
#include "browser/resource_dispatcher_host_delegate_efl.h"
#include "browser/web_view_browser_message_filter.h"
#include "browser_context_efl.h"
#include "browser_main_parts_efl.h"
#include "command_line_efl.h"
#include "common/content_switches_efl.h"
#include "common/web_contents_utils.h"
#include "components/autofill/content/browser/content_autofill_driver_factory.h"
#include "components/navigation_interception/intercept_navigation_throttle.h"
#include "components/navigation_interception/navigation_params.h"
#include "content/browser/web_contents/web_contents_view_efl.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/render_widget_host.h"
#include "content/public/browser/render_widget_host_iterator.h"
#include "content/public/browser/resource_dispatcher_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/content_features.h"
#include "content/public/common/content_switches.h"
#include "eweb_context.h"
#include "private/ewk_notification_private.h"
#include "third_party/WebKit/public/platform/modules/payments/payment_request.mojom.h"
#include "web_contents_delegate_efl.h"
#include "web_contents_view_delegate_ewk.h"

#if defined(OS_TIZEN)
#include <vconf.h>

#include "browser/geolocation/location_provider_efl.h"
#include "content/browser/speech/tts_message_filter_efl.h"
#endif

#if defined(OS_TIZEN_TV_PRODUCT)
#include "common/application_type.h"
#include "content/public/browser/client_certificate_delegate.h"
#include "ewk_global_data.h"
#include "io_thread_delegate_efl.h"
#include "net/ssl/ssl_cert_request_info.h"
#include "net/ssl/ssl_platform_key_nss.h"
#include "url/url_constants.h"
#include "web_product_tv.h"
#include "wrt/hbbtv_widget_host.h"
#endif

#if BUILDFLAG(ENABLE_NACL) || \
    (defined(TIZEN_PEPPER_EXTENSIONS) && BUILDFLAG(ENABLE_PLUGINS))
#include "content/browser/renderer_host/pepper/browser_pepper_host_factory_efl.h"
#include "content/public/browser/browser_ppapi_host.h"
#include "ppapi/host/ppapi_host.h"
#endif

#if defined(TIZEN_WEB_SPEECH_RECOGNITION)
#include "content/browser/speech/tizen_speech_recognition_manager_delegate.h"
#endif

#if BUILDFLAG(ENABLE_NACL)
#include "content/public/browser/browser_child_process_host.h"
#include "content/public/browser/child_process_data.h"
#include "components/nacl/browser/nacl_browser.h"
#include "components/nacl/browser/nacl_host_message_filter.h"
#include "components/nacl/browser/nacl_process_host.h"
#include "components/nacl/common/nacl_process_type.h"
#include "components/nacl/common/nacl_paths.h"
#include "components/nacl/zygote/nacl_fork_delegate_linux.h"
#include "components/nacl/common/nacl_constants.h"
#include "content/grit/content_resources.h"
#endif

#if defined(TIZEN_PEPPER_PLAYER)
#include "content/browser/renderer_host/pepper/media_player/pepper_media_player_message_filter.h"
#endif

using web_contents_utils::WebContentsFromFrameID;
using web_contents_utils::WebContentsFromViewID;
using web_contents_utils::WebViewFromWebContents;
using content::BrowserThread;

namespace content {

namespace {

const char* const kDefaultAcceptLanguages = "en-us,en";

#if defined(OS_TIZEN)
void PlatformLanguageChanged(keynode_t* keynode, void* data) {
  char* lang = vconf_get_str(VCONFKEY_LANGSET);
  if (!lang)
    return;

  std::string accept_languages(lang);
  std::replace(accept_languages.begin(), accept_languages.end(), '_', '-');

  size_t dot_position = accept_languages.find('.');
  if (dot_position != std::string::npos)
    accept_languages = accept_languages.substr(0, dot_position);

  static_cast<content::ContentBrowserClientEfl*>(data)->
      SetAcceptLangs(accept_languages);

  free(lang);
}
#endif  // defined(OS_TIZEN)

#if defined(OS_TIZEN_TV_PRODUCT)
std::map<std::string, std::string> dynamic_cert_map_;
const char kCertListFile[] =
    "/usr/share/chromium-efl/clientcert/ClientCertList";
const char kWebSDIPublicKeyDataName[] = "CFL";
const char kWebSDIPrivateKeyDataName[] = "PFL";
const char kWebSDICertChainDataName[] = "CertChain";

bool ParseClientCertList(std::string host,
                         base::FilePath* public_file_path,
                         base::FilePath* private_file_path,
                         std::string client_cert_list) {
  base::FilePath client_cert_path(client_cert_list);
  std::string client_cert_data;

  if (base::ReadFileToString(client_cert_path, &client_cert_data)) {
    std::vector<std::string> cert_lists = base::SplitString(
        client_cert_data, ",", base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL);
    for (const auto& cert_list : cert_lists) {
      if (!cert_list.empty()) {
        std::vector<std::string> paths = base::SplitString(
            cert_list, ":", base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL);
        if (paths.size() == 3) {
          if (paths[0] == "*" || (host.find(paths[0]) != std::string::npos)) {
            *public_file_path = base::FilePath::FromUTF8Unsafe(paths[1]);
            *private_file_path = base::FilePath::FromUTF8Unsafe(paths[2]);
            return true;
          }
        } else {
          LOG(ERROR) << "Could not parse \"" << client_cert_path.AsUTF8Unsafe()
                     << "\" successfully. Please check the syntax of file";
          return false;
        }
      }
    }
  } else {
    LOG(ERROR) << "Could not get certificate list of \""
               << client_cert_path.AsUTF8Unsafe() << "\"";
    return false;
  }
  return false;
}

scoped_refptr<net::X509Certificate> ImportKeyAndCreateClientCertChain(
    std::string cert_file,
    std::string key_file,
    std::string cert_buffer,
    const std::string& key_buffer) {
  net::CertificateList public_cert_list;
  if (!net::CertDatabase::GetInstance()->ImportPrivateKey(key_buffer)) {
    LOG(ERROR) << "Private Key could not be added \"" << key_file << "\"";
    return nullptr;
  }

  if (cert_buffer.empty()) {
    LOG(INFO) << "Public Key from Secure Storage is NULL, "
                 "Get it in File like previous way.";
    base::FilePath public_file_path(cert_file);
    if (!base::ReadFileToString(public_file_path, &cert_buffer)) {
      LOG(ERROR)
          << "Could not read file \"" << public_file_path.AsUTF8Unsafe()
          << "\" for parsing client cert list. Please check permissions!";
      return nullptr;
    }
  }

  public_cert_list = net::X509Certificate::CreateCertificateListFromBytes(
      cert_buffer.data(), cert_buffer.size(),
      net::X509Certificate::FORMAT_AUTO);

  std::vector<std::string> certificates;
  for (const scoped_refptr<net::X509Certificate>& cert : public_cert_list) {
    std::string der_encoded_cert;
    if (!net::X509Certificate::GetDEREncoded(cert->os_cert_handle(),
                                             &der_encoded_cert)) {
      return nullptr;
    }
    certificates.push_back(der_encoded_cert);
  }

  std::vector<base::StringPiece> cert_pieces(certificates.size());
  for (unsigned i = 0; i < certificates.size(); i++) {
    cert_pieces[i] = base::StringPiece(certificates[i]);
  }

  return net::X509Certificate::CreateFromDERCertChain(cert_pieces);
}

scoped_refptr<net::X509Certificate> MakeClientCertFromSecureStorage(
    std::string cert_file,
    std::string key_file) {
  std::string cert_buffer;
  std::string key_buffer;

  wp_tv_create();
  Wp_Bool ret = wp_tv_create_feature(FEATURE_SECURITY);
  if (ret) {
    cert_buffer = wp_tv_get_key_from_secure_storage(cert_file);
    key_buffer = wp_tv_get_key_from_secure_storage(key_file);
  } else {
    LOG(ERROR) << "wp_tv_create_feature fail";
  }

  if (key_buffer.empty()) {
    LOG(ERROR) << "Private Key from Secure Storage is NULL. \"" << key_file
               << "\"";
    return nullptr;
  }

  return ImportKeyAndCreateClientCertChain(cert_file, key_file, cert_buffer,
                                           key_buffer);
}

scoped_refptr<net::X509Certificate> MakeDynamicClientCert(std::string host) {
  scoped_refptr<net::X509Certificate> client_cert;
  std::string dynamic_cert_buffer;
  std::map<std::string, std::string>::iterator iter =
      dynamic_cert_map_.find(host);

  LOG(INFO) << "[Dynamic Cert] host : " << host;
  if (iter == dynamic_cert_map_.end())
    return nullptr;

  LOG(INFO) << "[Dynamic Cert] matched cert key path or data name for"
               " secure storage: "
            << iter->second;
  if (client_cert = MakeClientCertFromSecureStorage(iter->second, iter->second))
    return client_cert;

  LOG(INFO) << "[Dynamic Cert] Try to get cert and key from file system";
  base::FilePath dynamic_cert_path(iter->second);
  if (!base::ReadFileToString(dynamic_cert_path, &dynamic_cert_buffer)) {
    LOG(ERROR) << "[Dynamic Cert] Could not read file \"" << iter->second
               << "\" for parsing client cert list. Please check permissions!";
    return nullptr;
  }
  if (!dynamic_cert_buffer.empty()) {
    client_cert = ImportKeyAndCreateClientCertChain(
        std::string(), std::string(), dynamic_cert_buffer, dynamic_cert_buffer);
  }
  return client_cert;
}
#endif  // defined(OS_TIZEN_TV_PRODUCT)

bool ShouldIgnoreNavigationOnUIThread(
    content::WebContents* source,
    const navigation_interception::NavigationParams& params) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DCHECK(source);

  EWebView* wv = WebViewFromWebContents(source);
  if (wv)
    return wv->ShouldIgnoreNavigation(params);

  return false;
}

}  // namespace

#if defined(OS_TIZEN_TV_PRODUCT)
void ContentBrowserClientEfl::SelectClientCertificate(
    WebContents* web_contents,
    net::SSLCertRequestInfo* cert_request_info,
    net::ClientCertIdentityList client_certs,
    std::unique_ptr<ClientCertificateDelegate> delegate) {
  scoped_refptr<net::X509Certificate> client_cert;
  scoped_refptr<net::SSLPrivateKey> private_key;
  base::FilePath public_file_path;
  base::FilePath private_file_path;

  LOG(INFO) << "[Client Authentication] SelectClientCertificate";
  client_cert = MakeDynamicClientCert(cert_request_info->host_and_port.host());
  if (!client_cert && enabled_websdi_) {
    LOG(INFO) << "[WebSDI] Get a Client Cert for WebSDI";
    client_cert = MakeClientCertFromSecureStorage(kWebSDIPublicKeyDataName,
                                                  kWebSDIPrivateKeyDataName);
  }

  if (!client_cert) {
    // We use this flow in case of below.
    //
    // 1. App do not want to use WebSDI for client authentication.
    //    Then, Engine parse a kCertListFile and
    //    check there is matched host name with current host name.
    //    If there is, create a client certificate with specified cert and key.
    //    Otherwise, create a default client certificate matched to wildcard(*)
    //    host name.
    // 2. If it fail to get a Key for WebSDI (ex. problem of secure storage),
    //    client_cert will be null.
    //    In that case,  Engine has to create a default client certificate.
    //    It is a policy of VD.
    LOG(INFO) << "[Default Cert] Try to Load Default Client Certificate";
    if (!ParseClientCertList(cert_request_info->host_and_port.host(),
                             &public_file_path, &private_file_path,
                             kCertListFile))
      return;

    client_cert = MakeClientCertFromSecureStorage(public_file_path.value(),
                                                  private_file_path.value());
  }

  if (!client_cert) {
    LOG(ERROR) << "[Client Authentication] client_cert is null, "
                  "Client Authentication will be failed";
    return;
  }

  int err_code =
      net::CertDatabase::GetInstance()->CheckUserCert(client_cert.get());
  if (net::OK != err_code) {
    LOG(ERROR) << "User certificate is not valid. Error code : " << err_code;
    return;
  }

  err_code = net::CertDatabase::GetInstance()->AddUserCert(client_cert.get());
  if (net::OK != err_code) {
    LOG(ERROR) << "User certificate could not be added. Error code : "
               << err_code;
    return;
  }
  private_key = net::FetchClientCertPrivateKey(
      client_cert.get(), client_cert.get()->os_cert_handle(), nullptr);
  delegate->ContinueWithCertificate(std::move(client_cert),
                                    std::move(private_key));
}

void ContentBrowserClientEfl::AddDynamicCertificatePath(
    const std::string& host,
    const std::string& cert_path) {
  dynamic_cert_map_[host] = cert_path;
}

void ContentBrowserClientEfl::GetGeolocationRequestContext(
    base::OnceCallback<void(scoped_refptr<net::URLRequestContextGetter>)>
        callback) {
  BrowserThread::PostTaskAndReply(
      BrowserThread::UI, FROM_HERE,
      base::Bind(&ContentBrowserClientEfl::GetRequestContextOnUIThread,
                 base::Unretained(this)),
      base::Bind(&ContentBrowserClientEfl::RespondOnOriginatingThread,
                 base::Unretained(this), base::Passed(std::move(callback))));
}

void ContentBrowserClientEfl::GetRequestContextOnUIThread() {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  system_request_context_ =
      EwkGlobalData::GetInstance()->GetSystemRequestContextGetter();
}

void ContentBrowserClientEfl::RespondOnOriginatingThread(
    base::OnceCallback<void(scoped_refptr<net::URLRequestContextGetter>)>
        callback) {
  std::move(callback).Run(system_request_context_);
}
#endif  // defined(OS_TIZEN_TV_PRODUCT)

#if defined(OS_TIZEN)
void ContentBrowserClientEfl::OpenURLInTab(
    const OpenURLParams& params,
    int worker_process_id,
    const base::Callback<void(WebContents*)>& callback) {
  RenderProcessHost* render_process_host =
      RenderProcessHost::FromID(worker_process_id);
  if (!render_process_host)
    return;

  std::unique_ptr<RenderWidgetHostIterator> widgets(
      RenderWidgetHost::GetRenderWidgetHosts());
  RenderWidgetHost* widget;
  RenderWidgetHost* widget_fallback = nullptr;

  while ((widget = widgets->GetNextHost())) {
    if (widget->GetProcess()->GetID() == render_process_host->GetID())
      break;
    if (!widget_fallback)
      widget_fallback = widget;
  }
  if (!widget) {
    if (!widget_fallback)
      return;
    widget = widget_fallback;
  }

  RenderViewHost* vh = RenderViewHost::From(widget);
  if (!vh)
    return;

  WebContents* content = WebContents::FromRenderViewHost(vh);
  if (!content)
    return;

  callback.Run(content->GetDelegate()->OpenURLFromTab(content, params));
}
#endif

ContentBrowserClientEfl::ContentBrowserClientEfl()
    : browser_main_parts_efl_(nullptr),
      notification_controller_(new NotificationControllerEfl),
#if defined(OS_TIZEN_TV_PRODUCT)
      enabled_websdi_(false),
      tts_mode_(TTS_MODE_SCREEN_READER),
      system_request_context_(nullptr),
#elif defined(OS_TIZEN)
      tts_mode_(TTS_MODE_DEFAULT),
#endif
      accept_langs_(kDefaultAcceptLanguages),
      shutting_down_(false),
      enabled_app_control(true) {
#if defined(OS_TIZEN)
  PlatformLanguageChanged(nullptr, this);
  vconf_notify_key_changed(VCONFKEY_LANGSET, PlatformLanguageChanged, this);
#endif
}

ContentBrowserClientEfl::~ContentBrowserClientEfl() {
#if defined(OS_TIZEN)
  vconf_ignore_key_changed(VCONFKEY_LANGSET, PlatformLanguageChanged);
#endif

#if defined(OS_TIZEN_TV_PRODUCT)
  wp_tv_destroy_feature(FEATURE_SECURITY);
#endif
}

void ContentBrowserClientEfl::SetBrowserContext(BrowserContext* context) {
  browser_context_.reset(context);
}

BrowserMainParts* ContentBrowserClientEfl::CreateBrowserMainParts(
    const MainFunctionParams& parameters) {
  browser_main_parts_efl_ = new BrowserMainPartsEfl();
  return browser_main_parts_efl_;
}

void ContentBrowserClientEfl::GetQuotaSettings(
    content::BrowserContext* context,
    content::StoragePartition* partition,
    storage::OptionalQuotaSettingsCallback callback) {
  storage::GetNominalDynamicSettings(
      partition->GetPath(), context->IsOffTheRecord(), std::move(callback));
}

void ContentBrowserClientEfl::AppendInjectedBundleCommandLine(
    const EWebContext& context,
    base::CommandLine* command_line) {
  const std::string& injectedBundlePath = context.GetInjectedBundlePath();
  if (injectedBundlePath.empty())
    return;

  command_line->AppendSwitchASCII(switches::kInjectedBundlePath,
                                  injectedBundlePath);

  const std::string& tizen_app_id = context.GetTizenAppId();
  command_line->AppendSwitchASCII(switches::kTizenAppId, tizen_app_id);

  double scale = context.GetWidgetScale();
  command_line->AppendSwitchASCII(switches::kWidgetScale,
                                  base::DoubleToString(scale));

  const std::string& widget_theme = context.GetWidgetTheme();
  command_line->AppendSwitchASCII(switches::kWidgetTheme, widget_theme);

  const std::string& widget_encoded_bundle = context.GetWidgetEncodedBundle();
  command_line->AppendSwitchASCII(switches::kWidgetEncodedBundle,
                                  widget_encoded_bundle);

  const std::string& tizen_app_version = context.GetTizenAppVersion();
  command_line->AppendSwitchASCII(switches::kTizenAppVersion,
                                  tizen_app_version);
}

void ContentBrowserClientEfl::AppendExtraCommandLineSwitchesInternal(
    base::CommandLine* command_line,
    int child_process_id) {
  if (!command_line->HasSwitch(switches::kProcessType))
    return;

  if (command_line->GetSwitchValueASCII(switches::kProcessType) !=
      switches::kRendererProcess)
    return;

  const content::RenderProcessHost* const host =
      content::RenderProcessHost::FromID(child_process_id);
  if (!host)
    return;

  const EWebContext* const context =
      static_cast<BrowserContextEfl*>(host->GetBrowserContext())->WebContext();
  if (!context)
    return;

#if defined(OS_TIZEN_TV_PRODUCT)
  Ewk_Application_Type application_type = context->GetApplicationType();
  command_line->AppendSwitchASCII(
      switches::kApplicationType,
      base::IntToString(static_cast<int>(application_type)));

  const std::string& cors_enabled_url_schemes =
      HbbtvWidgetHost::Get().GetCORSEnabledURLSchemes();
  if (!cors_enabled_url_schemes.empty()) {
    command_line->AppendSwitchASCII(switches::kCORSEnabledURLSchemes,
                                    cors_enabled_url_schemes);
  }

  const std::string& mime_types = HbbtvWidgetHost::Get().GetJSPluginMimeTypes();
  if (!mime_types.empty()) {
    command_line->AppendSwitchASCII(switches::kJSPluginMimeTypes, mime_types);
  }

  const double time_offset = HbbtvWidgetHost::Get().GetTimeOffset();
  if (time_offset) {
    command_line->AppendSwitchASCII(switches::kTimeOffset,
                                    base::DoubleToString(time_offset));
  }

  command_line->AppendSwitchASCII(switches::kMaxUrlCharacters,
                                  base::SizeTToString(url::kMaxURLChars));
#endif

  AppendInjectedBundleCommandLine(*context, command_line);
}

void ContentBrowserClientEfl::AppendExtraCommandLineSwitches(
    base::CommandLine* command_line,
    int child_process_id) {
  AppendExtraCommandLineSwitchesInternal(command_line, child_process_id);
  CommandLineEfl::AppendProcessSpecificArgs(*command_line);
}

/* LCOV_EXCL_START */
bool ContentBrowserClientEfl::CanCreateWindow(
    RenderFrameHost* opener,
    const GURL& opener_url,
    const GURL& opener_top_level_frame_url,
    const GURL& source_origin,
    content::mojom::WindowContainerType container_type,
    const GURL& target_url,
    const Referrer& referrer,
    const std::string& frame_name,
    WindowOpenDisposition disposition,
    const blink::mojom::WindowFeatures& features,
    bool user_gesture,
    bool opener_suppressed,
    bool* no_javascript_access) {
  if (!user_gesture && opener->GetProcess() && opener->GetRenderViewHost()) {
    *no_javascript_access = true;
    BrowserThread::PostTask(
        BrowserThread::UI, FROM_HERE,
        base::Bind(&ContentBrowserClientEfl::DispatchPopupBlockedOnUIThread,
                   opener->GetProcess()->GetID(),
                   opener->GetRenderViewHost()->GetRoutingID(), target_url));
    return false;
  }

  // TODO: shouldn't we call policy,decision,new,window smart callback here?

  // User_gesture is modified on renderer side to true if javascript
  // is allowed to open windows without user gesture. If we've got user gesture
  // we also allow javascript access.
  *no_javascript_access = false;
  return true;
}

void ContentBrowserClientEfl::DispatchPopupBlockedOnUIThread(
    int render_process_id,
    int opener_render_view_id,
    const GURL& target_url) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));

  WebContents* content =
      WebContentsFromViewID(render_process_id, opener_render_view_id);

  if (!content)
    return;

  EWebView* wv = WebViewFromWebContents(content);
  wv->SmartCallback<EWebViewCallbacks::PopupBlocked>().call(
      target_url.spec().c_str());
}
/* LCOV_EXCL_STOP */

void ContentBrowserClientEfl::ResourceDispatcherHostCreated() {
  ResourceDispatcherHost* host = ResourceDispatcherHost::Get();
  resource_disp_host_del_efl_.reset(new ResourceDispatcherHostDelegateEfl());
  host->SetDelegate(resource_disp_host_del_efl_.get());
}

#if defined(TIZEN_WEB_SPEECH_RECOGNITION)
SpeechRecognitionManagerDelegate*
ContentBrowserClientEfl::CreateSpeechRecognitionManagerDelegate() {
  return new TizenSpeechRecognitionManagerDelegate();
}
#endif

/* LCOV_EXCL_START */
void ContentBrowserClientEfl::AllowCertificateError(
    content::WebContents* web_contents,
    int cert_error,
    const net::SSLInfo& ssl_info,
    const GURL& request_url,
    ResourceType resource_type,
    bool strict_enforcement,
    bool expired_previous_decision,
    const base::Callback<void(CertificateRequestResultType)>& callback) {
#if defined(OS_TIZEN_TV_PRODUCT)
  if (IsTIZENWRT() && resource_type != RESOURCE_TYPE_MAIN_FRAME) {
    // A sub-resource has a certificate error. The user doesn't really
    // have a context for making the right decision, so block the
    // request hard.
    callback.Run(content::CERTIFICATE_REQUEST_RESULT_TYPE_DENY);
    return;
  }
#endif
  DCHECK(web_contents);
  WebContentsDelegate* delegate = web_contents->GetDelegate();
  if (!delegate) {
    callback.Run(content::CERTIFICATE_REQUEST_RESULT_TYPE_DENY);
    return;
  }
  static_cast<content::WebContentsDelegateEfl*>(delegate)->
     RequestCertificateConfirm(web_contents, cert_error, ssl_info, request_url,
         resource_type, strict_enforcement, callback);
}
/* LCOV_EXCL_STOP */

PlatformNotificationService*
ContentBrowserClientEfl::GetPlatformNotificationService() {
  return notification_controller_.get();
}

/* LCOV_EXCL_START */
NotificationControllerEfl* ContentBrowserClientEfl::GetNotificationController()
    const {
  return notification_controller_.get();
}
/* LCOV_EXCL_STOP */

bool ContentBrowserClientEfl::AllowGetCookie(const GURL& url,
                                             const GURL& first_party,
                                             const net::CookieList& cookie_list,
                                             content::ResourceContext* context,
                                             int render_process_id,
                                             int render_frame_id) {
  if (shutting_down_)
    return false;

  BrowserContextEfl::ResourceContextEfl* rc =
      static_cast<BrowserContextEfl::ResourceContextEfl*>(context);
  if (!rc)
    return false;

  scoped_refptr<CookieManager> cookie_manager = rc->GetCookieManager();
  if (!cookie_manager.get())
    return false;

  return cookie_manager->AllowGetCookie(url, first_party, cookie_list, context,
                                        render_process_id, render_frame_id);
}

bool ContentBrowserClientEfl::AllowSetCookie(
    const GURL& url,
    const GURL& first_party,
    const std::string& cookie_line,
    content::ResourceContext* context,
    int render_process_id,
    int render_frame_id,
    const net::CookieOptions& options) {
  if (shutting_down_)
    return false;

  BrowserContextEfl::ResourceContextEfl* rc =
      static_cast<BrowserContextEfl::ResourceContextEfl*>(context);
  if (!rc)
    return false;

  scoped_refptr<CookieManager> cookie_manager = rc->GetCookieManager();
  if (!cookie_manager.get())
    return false;

  return cookie_manager->AllowSetCookie(url, first_party, cookie_line, context,
                                        render_process_id, render_frame_id,
                                        options);
}

void ContentBrowserClientEfl::OverrideWebkitPrefs(
    RenderViewHost* render_view_host,
    WebPreferences* prefs) {
  WebContents* contents = WebContents::FromRenderViewHost(render_view_host);
  if (contents) {
    EWebView* wv = WebViewFromWebContents(contents);
    if (wv->GetSettings()) {
      // Since Ewk_Settings has a separate store for EFL specific preferences
      // they are handled by EWebView::RenderViewCreated to make sure they will
      // not go to outgoing process.
      *prefs = wv->GetSettings()->getPreferences(); // LCOV_EXCL_LINE
    }
  }
  // TODO(dennis.oh): See http://107.108.218.239/bugzilla/show_bug.cgi?id=9507
  //                  This pref should be set to false again someday.
  // This pref is set to true by default
  // because some content providers such as YouTube use plain http requests
  // to retrieve media data chunks while running in a https page. This pref
  // should be disabled once all the content providers are no longer doing that.
#if !defined(OS_TIZEN_TV_PRODUCT)
  prefs->allow_running_insecure_content = true;
#endif
}

void ContentBrowserClientEfl::RenderProcessWillLaunch(
    content::RenderProcessHost* host) {
  host->AddFilter(new RenderMessageFilterEfl(host->GetID()));
  host->AddFilter(new editing::EditorClientObserver(host->GetID()));
#if defined(OS_TIZEN)
  host->AddFilter(new TtsMessageFilterEfl(tts_mode_));
#endif
#if BUILDFLAG(ENABLE_NACL)
  int id = host->GetID();
  net::URLRequestContextGetter* context =
      host->GetStoragePartition()->GetURLRequestContext();
  host->AddFilter(new nacl::NaClHostMessageFilter(
    id,
    // @FIXME(dennis.oh) Check if IsOffTheRecord is set?
    false,
    host->GetBrowserContext()->GetPath(),
    context));
#endif
#if defined(TIZEN_PEPPER_PLAYER)
  host->AddFilter(new PepperMediaPlayerMessageFilter(host->GetID()));
#endif
  host->AddFilter(new NetworkHintsMessageFilterEfl(
      static_cast<BrowserContextEfl*>(host->GetBrowserContext())));
}

#if BUILDFLAG(ENABLE_NACL)
content::BrowserPpapiHost*
    ContentBrowserClientEfl::GetExternalBrowserPpapiHost(
        int plugin_process_id) {
  BrowserChildProcessHostIterator iter(PROCESS_TYPE_NACL_LOADER);
  while (!iter.Done()) {
    nacl::NaClProcessHost* host = static_cast<nacl::NaClProcessHost*>(
        iter.GetDelegate());
    if (host->process() &&
        host->process()->GetData().id == plugin_process_id) {
      // Found the plugin.
      return host->browser_ppapi_host();
    }
    ++iter;
  }
  return NULL;
}
#endif

#if BUILDFLAG(ENABLE_NACL) || \
    (defined(TIZEN_PEPPER_EXTENSIONS) && BUILDFLAG(ENABLE_PLUGINS))
void ContentBrowserClientEfl::DidCreatePpapiPlugin(
    content::BrowserPpapiHost* browser_host) {
  browser_host->GetPpapiHost()->AddHostFactoryFilter(
      std::unique_ptr<ppapi::host::HostFactory>(
          new BrowserPepperHostFactoryEfl(browser_host)));
}
#endif

/* LCOV_EXCL_START */
content::DevToolsManagerDelegate*
ContentBrowserClientEfl::GetDevToolsManagerDelegate() {
  return new DevToolsDelegateEfl();
}
/* LCOV_EXCL_STOP */

std::string ContentBrowserClientEfl::GetApplicationLocale() {
  char* local_default = setlocale(LC_CTYPE, 0);
  if (!local_default)
    return "en-US"; // LCOV_EXCL_LINE

  std::string locale(local_default);
  size_t pos = locale.find('_');
  if (pos != std::string::npos)
    locale.replace(pos, 1, "-");

  pos = locale.find('.');
  if (pos != std::string::npos)
    locale = locale.substr(0, pos);

  return locale;
}

WebContentsViewDelegate* ContentBrowserClientEfl::GetWebContentsViewDelegate(
    WebContents* web_contents) {
  return new WebContentsViewDelegateEwk(WebViewFromWebContents(web_contents));
}

void ContentBrowserClientEfl::InitFrameInterfaces() {
  frame_interfaces_ = base::MakeUnique<
      service_manager::BinderRegistryWithArgs<content::RenderFrameHost*>>();

  frame_interfaces_->AddInterface(
      base::Bind(&autofill::ContentAutofillDriverFactory::BindAutofillDriver));
  if (base::FeatureList::IsEnabled(features::kWebPayments)) {
    frame_interfaces_->AddInterface(
        base::Bind(&payments::CreatePaymentRequestForWebContents));
  }
}

void ContentBrowserClientEfl::BindInterfaceRequestFromFrame(
    content::RenderFrameHost* render_frame_host,
    const std::string& interface_name,
    mojo::ScopedMessagePipeHandle interface_pipe) {
  if (!frame_interfaces_.get())
    InitFrameInterfaces();

  frame_interfaces_->TryBindInterface(interface_name, &interface_pipe,
                                      render_frame_host);
}

std::vector<std::unique_ptr<content::NavigationThrottle>>
ContentBrowserClientEfl::CreateThrottlesForNavigation(
    content::NavigationHandle* handle) {
  std::vector<std::unique_ptr<content::NavigationThrottle>> throttles;

  if (enabled_app_control && handle->IsInMainFrame()) {
    throttles.push_back(
        base::MakeUnique<navigation_interception::InterceptNavigationThrottle>(
            handle, base::Bind(&ShouldIgnoreNavigationOnUIThread)));
  }

  return throttles;
}

QuotaPermissionContext*
ContentBrowserClientEfl::CreateQuotaPermissionContext() {
  return new QuotaPermissionContextEfl();
}

void ContentBrowserClientEfl::CleanUpBrowserContext() {
  browser_context_.reset(nullptr);
  shutting_down_ = true;
}

#if defined(TIZEN_PEPPER_EXTENSIONS)
bool ContentBrowserClientEfl::AllowPepperSocketAPI(
    BrowserContext* browser_context,
    const GURL& url,
    bool private_api,
    const SocketPermissionRequest* params) {
  return true;
}

bool ContentBrowserClientEfl::IsPluginAllowedToUseDevChannelAPIs(
    BrowserContext* browser_context,
    const GURL& url) {
  return true;
}
#endif

std::vector<content::ContentBrowserClient::ServiceManifestInfo>
ContentBrowserClientEfl::GetExtraServiceManifests() {
  return std::vector<content::ContentBrowserClient::ServiceManifestInfo>({
#if BUILDFLAG(ENABLE_NACL)
    {nacl::kNaClLoaderServiceName, IDR_NACL_LOADER_MANIFEST},
#endif  // BUILDFLAG(ENABLE_NACL)
  });
}

std::string ContentBrowserClientEfl::GetAcceptLangs(BrowserContext* context) {
  if (!preferred_langs_.empty())
    return preferred_langs_;

  return accept_langs_;
}

void ContentBrowserClientEfl::SetAcceptLangs(const std::string& accept_langs) {
  if (accept_langs.empty() || accept_langs_ == accept_langs)
    return;

  accept_langs_ = accept_langs;

  if (preferred_langs_.empty())
    NotifyAcceptLangsChanged();
}

void ContentBrowserClientEfl::SetPreferredLangs(
    const std::string& preferred_langs) {
  if (preferred_langs_ == preferred_langs)
    return;

  preferred_langs_ = preferred_langs;

  NotifyAcceptLangsChanged();
}

void ContentBrowserClientEfl::AddAcceptLangsChangedCallback(
    const AcceptLangsChangedCallback& callback) {
  accept_langs_changed_callbacks_.push_back(callback);
}

void ContentBrowserClientEfl::RemoveAcceptLangsChangedCallback(
    const AcceptLangsChangedCallback& callback) {
  for (size_t i = 0; i < accept_langs_changed_callbacks_.size(); ++i) {
    if (accept_langs_changed_callbacks_[i].Equals(callback)) {
      accept_langs_changed_callbacks_.erase(
          accept_langs_changed_callbacks_.begin() + i);
      return;
    }
  }
}

void ContentBrowserClientEfl::NotifyAcceptLangsChanged() {
  for (const AcceptLangsChangedCallback& callback :
       accept_langs_changed_callbacks_) {
    if (!callback.is_null()) {
      if (preferred_langs_.empty())
        callback.Run(accept_langs_);
      else
        callback.Run(preferred_langs_);
    }
  }
}

}  // namespace content
